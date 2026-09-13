param(
    [string]$MainRepoPath = ".\",
    [string]$AuxDataRepoPath = ".\AuxData",
    [string]$DataSubdirName = "Data",
    [string]$LockFile = "aux-data.lock.json"
)

$ErrorActionPreference = "Stop"

function Resolve-InputPath([string]$Path, [string]$BasePath) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $BasePath $Path))
}

function Test-AuxDataReady(
    [string]$ResolvedMainRepoPath,
    [string]$ResolvedAuxDataRepoPath,
    [string]$ResolvedLockFile,
    [string]$ResolvedDataSubdirName
) {
    if (-not (Test-Path -LiteralPath (Join-Path $ResolvedAuxDataRepoPath ".git"))) {
        return $false
    }

    $lockFilePath = Join-Path $ResolvedMainRepoPath $ResolvedLockFile
    if (-not (Test-Path -LiteralPath $lockFilePath)) {
        throw "Auxiliary data lock file not found: $lockFilePath"
    }
    $lock = Get-Content -LiteralPath $lockFilePath | ConvertFrom-Json
    $pinnedCommit = "$($lock.pinnedCommitHash)".Trim()
    if (-not $pinnedCommit) {
        throw "Auxiliary data lock file has no pinnedCommitHash: $lockFilePath"
    }

    $currentCommit = & git -C $ResolvedAuxDataRepoPath rev-parse HEAD 2>$null
    if ($LASTEXITCODE -ne 0 -or "$currentCommit".Trim() -ine $pinnedCommit) {
        return $false
    }

    $manifestRef = "${pinnedCommit}:etl-manifest.json"
    $manifestText = & git -C $ResolvedAuxDataRepoPath show $manifestRef 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $manifestText) {
        return $false
    }
    $manifest = ($manifestText -join [System.Environment]::NewLine) | ConvertFrom-Json
    if (-not $manifest.etlFiles) {
        return $false
    }

    $dataRoot = Join-Path $ResolvedAuxDataRepoPath $ResolvedDataSubdirName
    foreach ($entry in $manifest.etlFiles) {
        $etlPath = Join-Path $dataRoot $entry.name
        if (-not (Test-Path -LiteralPath $etlPath)) {
            return $false
        }
        $actualHash = (Get-FileHash -LiteralPath $etlPath -Algorithm SHA256).Hash
        if ($actualHash -ine $entry.hash) {
            return $false
        }
    }

    return $true
}

$resolvedMainRepoPath = Resolve-InputPath $MainRepoPath $PSScriptRoot
$resolvedAuxDataRepoPath = Resolve-InputPath $AuxDataRepoPath $PSScriptRoot

$hashAlgorithm = [System.Security.Cryptography.SHA256]::Create()
try {
    $pathBytes = [System.Text.Encoding]::UTF8.GetBytes($resolvedAuxDataRepoPath.ToLowerInvariant())
    $mutexHash = -join ($hashAlgorithm.ComputeHash($pathBytes) | ForEach-Object { $_.ToString("x2") })
}
finally {
    $hashAlgorithm.Dispose()
}
$mutex = New-Object System.Threading.Mutex($false, "PresentMonAuxDataEnsure-$mutexHash")
$mutexAcquired = $false

try {
    try {
        $mutexAcquired = $mutex.WaitOne([System.TimeSpan]::FromMinutes(30))
    }
    catch [System.Threading.AbandonedMutexException] {
        $mutexAcquired = $true
    }
    if (-not $mutexAcquired) {
        throw "Timed out waiting for another auxiliary test data restore to finish."
    }

    if (Test-AuxDataReady $resolvedMainRepoPath $resolvedAuxDataRepoPath $LockFile $DataSubdirName) {
        Write-Host "Pinned auxiliary test data is present and valid."
    }
    else {
        Write-Host "Auxiliary test data is missing, stale, or invalid. Restoring the pinned data."
        & (Join-Path $PSScriptRoot "pull-aux.ps1") `
            -MainRepoPath $resolvedMainRepoPath `
            -AuxDataRepoPath $resolvedAuxDataRepoPath `
            -DataSubdirName $DataSubdirName `
            -LockFile $LockFile

        if (-not (Test-AuxDataReady $resolvedMainRepoPath $resolvedAuxDataRepoPath $LockFile $DataSubdirName)) {
            throw "Auxiliary test data restore completed without producing a valid pinned data set."
        }

        Write-Host "Pinned auxiliary test data was restored and validated."
    }
}
finally {
    if ($mutexAcquired) {
        $mutex.ReleaseMutex()
    }
    $mutex.Dispose()
}
