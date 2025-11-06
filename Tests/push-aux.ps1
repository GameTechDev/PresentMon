# Parameters
$Owner = "planetchili"                    # AuxTestData GitHub organization
$Repo = "IPMAuxTestData"                  # The AuxTestData repo
$MainRepoPath = ".\"                      # Path to the main repo (should contain the aux-test-data.lock.json)
$TestDataPath = ".\AuxData"           # Path to the test AuxTestData repo
$LockFile = "aux-test-data.lock.json"     # Lock file in the main repo that pins the testdata commit
$NewReleaseTag = "etl-update-$(Get-Date -Format 'yyyyMMddHHmmss')"  # New release tag based on the current timestamp

# Load GitHub token from environment variable
$GitHubToken = $env:AUXTESTDATA_GITHUB_TOKEN
if (-not $GitHubToken) {
    Write-Error "Environment variable AUXTESTDATA_GITHUB_TOKEN is not set. Aborting."
    exit 1
}
$Headers = @{ Authorization = "Bearer $GitHubToken" }

# Ensure we are in the correct working directory for the test data repo
Set-Location -Path $TestDataPath

# Fetch the latest changes from the test data repo
Write-Host "Fetching the latest changes from the test data repository..."
git fetch origin

# Load the test data lock file to get the pinned commit hash
$lock = Get-Content -Path $MainRepoPath\$LockFile | ConvertFrom-Json
$pinnedCommit = $lock.pinnedCommitHash  # Commit hash from the main repo to pin to

# Checkout the pinned commit in the test data repo
Write-Host "Checking out pinned commit $pinnedCommit..."
git checkout $pinnedCommit

# Load the manifest to check for existing ETL file metadata (hashes)
$manifestFilePath = Join-Path $TestDataPath "etl-manifest.json"
if (Test-Path $manifestFilePath) {
    $manifest = Get-Content -Path $manifestFilePath | ConvertFrom-Json
} else {
    $manifest = @{ etlFiles = @() }
}

# Collect current ETL files in the test data repo
$etlFolder = Join-Path $TestDataPath "etl"
$currentETLs = Get-ChildItem -Path $etlFolder -Recurse -Filter "*.etl"

# Prepare a staging folder for gzipped ETLs that need to be uploaded
$stagingFolder = Join-Path $TestDataPath "etl-staging"
if (Test-Path $stagingFolder) {
    Remove-Item -Recurse -Force $stagingFolder
}
New-Item -ItemType Directory -Path $stagingFolder

# Initialize an array to track files to be uploaded
$filesToUpload = @()

# Compare the current ETLs with the manifest to detect changes
$etlChanges = @()
$deletedETLs = $manifest.etlFiles | Where-Object { $currentETLs.Name -notcontains $_.name }
$addedETLs = $currentETLs | Where-Object { $manifest.etlFiles.name -notcontains $_.Name }

# Track changes (if any) in the ETLs
foreach ($etl in $currentETLs) {
    $existingETL = $manifest.etlFiles | Where-Object { $_.name -eq $etl.Name }
    
    if ($existingETL) {
        # Check if the hash has changed
        $newHash = (Get-FileHash $etl.FullName -Algorithm SHA256).Hash
        if ($existingETL.hash -ne $newHash) {
            # If the hash has changed, mark the file as updated
            $etlChanges += $etl
            $existingETL.hash = $newHash  # Update the hash in the manifest
        }
    } else {
        # If the ETL is not in the manifest, it’s a new addition
        $addedETLs += $etl
    }
}

# Prepare new manifest data
$manifest.etlFiles = $manifest.etlFiles | Where-Object { $deletedETLs -notcontains $_ }
$manifest.etlFiles += $etlChanges | ForEach-Object {
    @{
        name = $_.Name
        hash = (Get-FileHash $_.FullName -Algorithm SHA256).Hash
        releaseTag = $NewReleaseTag
    }
}
$manifest.etlFiles += $addedETLs | ForEach-Object {
    @{
        name = $_.Name
        hash = (Get-FileHash $_.FullName -Algorithm SHA256).Hash
        releaseTag = $NewReleaseTag
    }
}

# Save the updated manifest
$manifest | ConvertTo-Json -Depth 3 | Set-Content -Path $manifestFilePath

# If there are any changes to the CSVs, commit those changes
$csvChangesDetected = $false
$csvFolder = Join-Path $TestDataPath "tests/Gold"

# Check for CSV file changes (similar logic as ETL changes)
git diff --exit-code -- $csvFolder
if ($?) {
    Write-Host "No changes to CSV files."
} else {
    Write-Host "CSV changes detected, committing changes..."
    git add $csvFolder
    git commit -m "Update CSV files"
    git push origin $pinnedCommit
    $csvChangesDetected = $true
}

# Prepare the new release (if there are any new/changed ETLs to upload)
if ($etlChanges.Count -gt 0 -or $addedETLs.Count -gt 0) {
    Write-Host "Preparing new release for ETLs..."

    # Create tarball of new or updated ETLs
    foreach ($etl in $etlChanges + $addedETLs) {
        $tarFile = Join-Path $stagingFolder "$($etl.Name).tar.gz"
        Write-Host "Creating tarball for ETL: $($etl.Name)..."
        tar -czf $tarFile -C $etlFolder $etl.Name
        $filesToUpload += $tarFile
    }

    # Create new GitHub release for the ETLs
    $releaseUrl = "https://api.github.com/repos/$Owner/$Repo/releases"
    $releaseBody = @{
        tag_name = $NewReleaseTag
        name = "ETL Update $NewReleaseTag"
        body = "Upload of updated ETLs and the manifest file."
        draft = $false
        prerelease = $false
    } | ConvertTo-Json -Depth 3

    $releaseResponse = Invoke-RestMethod -Uri $releaseUrl -Method Post -Body $releaseBody -Headers $Headers
    $releaseId = $releaseResponse.id

    # Upload ETLs as release assets
    foreach ($file in $filesToUpload) {
        $uploadUrl = "https://uploads.github.com/repos/$Owner/$Repo/releases/$releaseId/assets?name=" + [System.Net.WebUtility]::UrlEncode([System.IO.Path]::GetFileName($file))
        Invoke-RestMethod -Uri $uploadUrl -Method Post -Headers $Headers -InFile $file
    }

    # Upload the manifest file to the release
    $manifestUploadUrl = "https://uploads.github.com/repos/$Owner/$Repo/releases/$releaseId/assets?name=etl-manifest.json"
    Invoke-RestMethod -Uri $manifestUploadUrl -Method Post -Headers $Headers -InFile $manifestFilePath

    Write-Host "New ETL release created and uploaded."
}

# Commit the updated lock file with the new pinned commit hash
if ($csvChangesDetected -or $etlChanges.Count -gt 0 -or $addedETLs.Count -gt 0) {
    Write-Host "Committing the updated testdata.lock.json..."
    $lock.pinnedCommitHash = (git rev-parse HEAD)
    $lock.etlStore.releaseTag = $NewReleaseTag
    $lock | ConvertTo-Json -Depth 3 | Set-Content -Path $MainRepoPath\$LockFile
    git add $MainRepoPath\$LockFile
    git commit -m "Update pinned commit and release tag for testdata"
    git push origin main
}

Write-Host "Push process complete."
