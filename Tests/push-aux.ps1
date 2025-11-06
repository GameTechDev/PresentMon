# Parameters
$Owner = "planetchili"                    # AuxTestData GitHub organization
$Repo = "IPMAuxTestData"                  # The AuxTestData repo
$MainRepoPath = ".\"                      # Path to the main repo (should contain the aux-test-data.lock.json)
$AuxDataRepoPath = ".\AuxData"            # Path to the AuxTestData repo (ignored by main repo)
$DataSubdirName = "Data"                  # Subdirectory inside AuxTestData that holds BOTH CSVs and .etl files
$LockFile = "aux-data.lock.json"     # Lock file in the main repo that pins the testdata commit (commit hash ONLY)
$NewReleaseTag = "etl-update-$(Get-Date -Format 'yyyyMMddHHmmss')"  # New release tag based on the current timestamp

# Load GitHub token from environment variable
$GitHubToken = $env:AUXTESTDATA_GITHUB_TOKEN
if (-not $GitHubToken) {
    Write-Error "Environment variable AUXTESTDATA_GITHUB_TOKEN is not set. Aborting."
    exit 1
}
$Headers = @{ Authorization = "Bearer $GitHubToken" }

# Preserve original location; resolve paths
$origLocation = Get-Location
try {
    # Resolve paths to absolute
    $MainRepoPath = (Resolve-Path $MainRepoPath).Path
    $AuxDataRepoPath = (Resolve-Path $AuxDataRepoPath).Path

    # Ensure we are in the correct working directory for the test data repo
    Set-Location -Path $AuxDataRepoPath

    # Make sure fetch works anonymously (HTTPS remote)
    $curUrl = git remote get-url origin 2>$null
    if ($curUrl -match '^git@github\.com:') {
        git remote set-url origin https://github.com/$Owner/$Repo.git
        git config --unset-all remote.origin.pushurl 2>$null
        git remote set-url --push origin https://github.com/$Owner/$Repo.git
    }

    # Fetch the latest changes from the test data repo
    Write-Host "Fetching the latest changes from the test data repository..."
    git fetch origin

    # Load the test data lock file to get the pinned commit hash
    $lockFilePath = Join-Path $MainRepoPath $LockFile
    if (-not (Test-Path $lockFilePath)) {
        Write-Warning "Lock file not found. Bootstrapping from origin/master..."
        $seedCommit = (git rev-parse origin/master).Trim()
        if (-not $seedCommit) { Write-Error "Failed to resolve origin/master for bootstrap."; exit 1 }
        Write-Host "Checking out seed commit $seedCommit..."
        git checkout $seedCommit
        $lock = @{ pinnedCommitHash = $seedCommit }
        $lock | ConvertTo-Json -Depth 3 | Set-Content -Path $lockFilePath
    } else {
        $lock = Get-Content -Path $lockFilePath | ConvertFrom-Json
    }
    $pinnedCommit = $lock.pinnedCommitHash  # Commit hash from the main repo to pin to

    # Ensure we're on a local branch (master) based on the pinned commit (avoid detached HEAD)
    git rev-parse --verify master > $null 2>&1
    if ($LASTEXITCODE -ne 0) {
        git checkout -B master $pinnedCommit
    } else {
        git checkout master
        git reset --hard $pinnedCommit
    }
    git branch --set-upstream-to=origin/master master > $null 2>&1

    # Paths inside the AuxData repo for actual data
    $dataRoot = Join-Path $AuxDataRepoPath $DataSubdirName

    # Load the manifest (stored at repo root, not in Data)
    $manifestFilePath = Join-Path $AuxDataRepoPath "etl-manifest.json"
    if (Test-Path $manifestFilePath) {
        $manifest = Get-Content -Path $manifestFilePath | ConvertFrom-Json
    } else {
        $manifest = @{ etlFiles = @() }
    }

    # Collect current ETL files in the single data directory
    if (-not (Test-Path $dataRoot)) { New-Item -ItemType Directory -Path $dataRoot | Out-Null }
    $currentETLs = Get-ChildItem -Path $dataRoot -Filter "*.etl"

    # Prepare a staging folder for gzipped ETLs that need to be uploaded (kept under the repo root)
    $stagingFolder = Join-Path $AuxDataRepoPath "etl-staging"
    if (Test-Path $stagingFolder) {
        Remove-Item -Recurse -Force $stagingFolder
    }
    New-Item -ItemType Directory -Path $stagingFolder | Out-Null

    # Track new files to upload
    $filesToUpload = @()

    # Map existing manifest entries by name for quick lookup
    $manifestByName = @{}
    foreach ($e in $manifest.etlFiles) { $manifestByName[$e.name] = $e }

    # Compute deltas and update manifest entries IN-PLACE (prevents duplicates)
    $deletedETLs = @()
    $addedETLs = @()

    # Detect deletions: anything in manifest not present in filesystem
    foreach ($k in $manifestByName.Keys) {
        if (-not ($currentETLs.Name -contains $k)) {
            $deletedETLs += $manifestByName[$k]
        }
    }

    # Process current ETLs: update existing or mark as added
    foreach ($etl in $currentETLs) {
        $name = $etl.Name
        $newHash = (Get-FileHash $etl.FullName -Algorithm SHA256).Hash
        if ($manifestByName.ContainsKey($name)) {
            $entry = $manifestByName[$name]
            if ($entry.hash -ne $newHash) {
                # Update hash + bump releaseTag to new one (changed content)
                $entry.hash = $newHash
                $entry.releaseTag = $NewReleaseTag
            }
        } else {
            # New file → create new entry with this release tag
            $addedETLs += $etl
            $manifest.etlFiles += @(
                @{
                    name = $name
                    hash = $newHash
                    releaseTag = $NewReleaseTag
                }
            )
        }
    }

    # Remove deleted entries from manifest
    if ($deletedETLs.Count -gt 0) {
        $manifest.etlFiles = $manifest.etlFiles | Where-Object { $deletedETLs -notcontains $_ }
    }

    # Save the updated manifest (at repo root)
    $manifest | ConvertTo-Json -Depth 3 | Set-Content -Path $manifestFilePath

    # --- Generic repo commit for any detected changes (Data or manifest) ---
    $repoChangesCommitted = $false
    git add -- $dataRoot $manifestFilePath
    git diff --cached --quiet
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Repository changes detected (Data and/or manifest). Committing..."
        git commit -m "Update data (CSVs/ETLs) and manifest"
        # Push to branch (not a commit hash)
        git push origin master
        $repoChangesCommitted = $true
    } else {
        Write-Host "No changes to Data or manifest; nothing to commit."
    }

    # Prepare the new release (if there are any new/changed ETLs to upload)
    # 'Changed' are those whose manifest entry got updated in-place (releaseTag == $NewReleaseTag) + newly added ETLs
    $etlsNeedingUpload = @()
    foreach ($etl in $currentETLs) {
        $entry = $manifest.etlFiles | Where-Object { $_.name -eq $etl.Name }
        if ($entry -and $entry.releaseTag -eq $NewReleaseTag) {
            $etlsNeedingUpload += $etl
        }
    }

    if ($etlsNeedingUpload.Count -gt 0) {
        Write-Host "Preparing new release for ETLs..."

        # Deduplicate just in case
        $toPackage = $etlsNeedingUpload | Select-Object -Property Name,FullName -Unique
        foreach ($etl in $toPackage) {
            $tarFile = Join-Path $stagingFolder "$($etl.Name).tar.gz"
            Write-Host "Creating tarball for ETL: $($etl.Name)..."
            tar -czf $tarFile -C $dataRoot $etl.Name
            $filesToUpload += $tarFile
        }

        # Current HEAD (after potential commit/push)
        $currentHead = (git rev-parse HEAD).Trim()

        # Create new GitHub release for the ETLs (tag pointing to the new commit)
        $releaseUrl = "https://api.github.com/repos/$Owner/$Repo/releases"
        $releaseBody = @{
            tag_name          = $NewReleaseTag
            target_commitish  = $currentHead
            name              = "ETL Update $NewReleaseTag"
            body              = "Upload of updated ETLs."
            draft             = $false
            prerelease        = $false
        } | ConvertTo-Json -Depth 3

        $releaseResponse = Invoke-RestMethod -Uri $releaseUrl -Method Post -Body $releaseBody -Headers $Headers
        $releaseId = $releaseResponse.id

        # Upload ETLs as release assets (manifest is NOT uploaded)
        foreach ($file in $filesToUpload) {
            $uploadUrl = "https://uploads.github.com/repos/$Owner/$Repo/releases/$releaseId/assets?name=" + [System.Net.WebUtility]::UrlEncode([System.IO.Path]::GetFileName($file))
            Invoke-RestMethod -Uri $uploadUrl -Method Post -Headers $Headers -InFile $file -ContentType "application/gzip"
        }

        Write-Host "New ETL release created and uploaded."
    }

    # Write the updated lock file with the new pinned commit hash (no commit to main repo here)
    if ($repoChangesCommitted -or ($etlsNeedingUpload.Count -gt 0)) {
        Write-Host "Writing updated lock file (no commit to main repo will be made)..."
        $lock.pinnedCommitHash = (git rev-parse HEAD)
        # Lock contains ONLY the commit hash; release tags are resolved from the manifest when pulling.
        $lock | ConvertTo-Json -Depth 3 | Set-Content -Path $lockFilePath
        Write-Host "Lock file written to $lockFilePath."
    }

    # Cleanup: remove staging folder after run
    if (Test-Path $stagingFolder) {
        Remove-Item -Recurse -Force $stagingFolder
    }
}
finally {
    # Restore original working directory
    Set-Location $origLocation
}

Write-Host "Push process complete."
