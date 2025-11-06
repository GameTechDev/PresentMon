param(
    [string]$Owner = "planetchili",
    [string]$Repo = "IPMAuxTestData",
    [string]$MainRepoPath = ".\",
    [string]$AuxDataRepoPath = ".\AuxData",
    [string]$DataSubdirName = "Data",
    [string]$LockFile = "aux-data.lock.json"
)

$ErrorActionPreference = "Stop"

# Preserve original location and prep temp download folder
$origLocation = Get-Location
$dlTemp = Join-Path $env:TEMP ("auxdata-pull-" + ([guid]::NewGuid().ToString("N")))

try {
    # ---- Resolve paths relative to the script location (PS5-compatible) ----
    if (-not [System.IO.Path]::IsPathRooted($AuxDataRepoPath)) {
        $AuxDataRepoPath = Join-Path $PSScriptRoot $AuxDataRepoPath
    }
    try {
        $resolvedAux = Resolve-Path -LiteralPath $AuxDataRepoPath -ErrorAction Stop
        $AuxDataRepoPath = $resolvedAux.Path
    } catch {
        # Keep as intended path (may not exist yet; cloning will create it)
        $AuxDataRepoPath = $AuxDataRepoPath
    }

    if (-not [System.IO.Path]::IsPathRooted($MainRepoPath)) {
        $MainRepoPath = Join-Path $PSScriptRoot $MainRepoPath
    }
    $MainRepoPath = (Resolve-Path -LiteralPath $MainRepoPath).Path

    # --- 1) Load lock file ---
    $lockFilePath = Join-Path $MainRepoPath $LockFile
    if (-not (Test-Path -LiteralPath $lockFilePath)) {
        throw "Lock file not found at '$lockFilePath'. Aborting."
    }
    $lock = Get-Content -LiteralPath $lockFilePath | ConvertFrom-Json
    if (-not $lock.pinnedCommitHash) {
        throw "Lock file missing 'pinnedCommitHash'. Aborting."
    }
    $pinnedCommit = "$($lock.pinnedCommitHash)".Trim()

    # --- 2) Ensure Aux repo exists (clone if not present), prefer HTTPS remote ---
    if (-not (Test-Path -LiteralPath $AuxDataRepoPath)) {
        Write-Host "AuxData repo not found at '$AuxDataRepoPath'. Cloning https://github.com/$Owner/$Repo.git ..."
        git clone "https://github.com/$Owner/$Repo.git" "$AuxDataRepoPath" | Out-Null
    }

    # Make sure it’s a valid git repo (has .git)
    if (-not (Test-Path -LiteralPath (Join-Path $AuxDataRepoPath ".git"))) {
        throw "'$AuxDataRepoPath' exists but is not a git repository."
    }

    Set-Location $AuxDataRepoPath

    # Force HTTPS for both fetch and push (push not used here, but keeps repo clean)
    $curUrl = git remote get-url origin 2>$null
    if ($curUrl -match '^git@github\.com:') {
        git remote set-url origin "https://github.com/$Owner/$Repo.git" | Out-Null
        git config --unset-all remote.origin.pushurl 2>$null
        git remote set-url --push origin "https://github.com/$Owner/$Repo.git" | Out-Null
    }

    # --- 3) Fetch and checkout pinned commit (detached HEAD is fine for pull) ---
    Write-Host "Fetching origin..."
    git fetch origin --tags | Out-Null

    Write-Host "Checking out pinned commit $pinnedCommit..."
    git checkout --detach $pinnedCommit | Out-Null

    # --- 4) Read manifest ---
    $manifestPath = Join-Path $AuxDataRepoPath "etl-manifest.json"
    if (-not (Test-Path -LiteralPath $manifestPath)) {
        throw "Manifest file not found at '$manifestPath' for pinned commit $pinnedCommit."
    }
    $manifest = Get-Content -LiteralPath $manifestPath | ConvertFrom-Json
    if (-not $manifest.etlFiles) {
        Write-Host "Manifest has no 'etlFiles' entries. Nothing to pull."
        return
    }

    # Paths
    $dataRoot = Join-Path $AuxDataRepoPath $DataSubdirName
    if (-not (Test-Path -LiteralPath $dataRoot)) { New-Item -ItemType Directory -Path $dataRoot | Out-Null }

    # Temp folder for downloads
    New-Item -ItemType Directory -Path $dlTemp | Out-Null

    # --- 5) Helper: cache releases by tag to avoid repeated API calls ---
    $releaseCache = @{}  # tag -> release JSON

    function Get-ReleaseByTag([string]$tag) {
        if ($releaseCache.ContainsKey($tag)) { return $releaseCache[$tag] }
        $url = "https://api.github.com/repos/$Owner/$Repo/releases/tags/$tag"
        $headers = @{
            "User-Agent" = "auxdata-pull-script"
            "Accept"     = "application/vnd.github+json"
        }
        try {
            $rel = Invoke-RestMethod -Uri $url -Headers $headers -Method Get
        } catch {
            throw "Failed to fetch release for tag '$tag' from $url. $_"
        }
        $releaseCache[$tag] = $rel
        return $rel
    }

    # --- 6) Iterate manifest entries and ensure ETLs present & valid ---
    foreach ($entry in $manifest.etlFiles) {
        $etlName = $entry.name
        $expectedHash = $entry.hash
        $tag = $entry.releaseTag

        if (-not $etlName -or -not $expectedHash -or -not $tag) {
            Write-Warning "Skipping malformed manifest entry (name/hash/tag missing): $($entry | ConvertTo-Json -Depth 3)"
            continue
        }

        $etlPath = Join-Path $dataRoot $etlName

        $needsDownload = $true
        if (Test-Path -LiteralPath $etlPath) {
            try {
                $actualHash = (Get-FileHash -LiteralPath $etlPath -Algorithm SHA256).Hash
                if ($actualHash -ieq $expectedHash) {
                    Write-Host "OK  : $etlName already present and matches hash."
                    $needsDownload = $false
                } else {
                    Write-Host "HASH: $etlName present but hash mismatch; will re-fetch."
                }
            } catch {
                Write-Host "WARN: Failed to hash existing file $etlName; will re-fetch."
            }
        }

        if (-not $needsDownload) { continue }

        Write-Host "GET : $etlName from release tag '$tag'..."

        # Fetch release once per tag
        $release = Get-ReleaseByTag -tag $tag

        # Find asset named "<etlName>.tar.gz"
        $assetName = "$etlName.tar.gz"
        $asset = $null
        foreach ($a in $release.assets) {
            if ($a.name -ieq $assetName) { $asset = $a; break }
        }
        if (-not $asset) {
            throw "Asset '$assetName' not found in release '$tag'."
        }

        $downloadUrl = $asset.browser_download_url
        if (-not $downloadUrl) {
            throw "browser_download_url missing for asset '$assetName' in release '$tag'."
        }

        # Download tar.gz to temp
        $archivePath = Join-Path $dlTemp $assetName
        Write-Host "DL  : $assetName ..."
        try {
            Invoke-WebRequest -Uri $downloadUrl -OutFile $archivePath
        } catch {
            throw "Failed to download '$assetName' from $downloadUrl. $_"
        }

        # Extract into Data directory (overwrites file if present)
        Write-Host "XT  : Extracting $assetName ..."
        tar -xzf $archivePath -C $dataRoot

        # Verify hash post-extract
        if (-not (Test-Path -LiteralPath $etlPath)) {
            throw "Extraction did not produce expected file '$etlName' in '$dataRoot'."
        }
        $postHash = (Get-FileHash -LiteralPath $etlPath -Algorithm SHA256).Hash
        if ($postHash -ine $expectedHash) {
            throw "Hash mismatch after extract for '$etlName'. Expected $expectedHash, got $postHash."
        }

        Write-Host "OK  : $etlName downloaded and verified."
    }

    Write-Host "All ETLs are present and verified."
}
finally {
    # Cleanup temp and restore working directory
    if (Test-Path -LiteralPath $dlTemp) {
        Remove-Item -Recurse -Force $dlTemp
    }
    Set-Location $origLocation
}
