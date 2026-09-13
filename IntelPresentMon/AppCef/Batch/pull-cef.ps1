<#
.SYNOPSIS
    Restore the repository-pinned CEF payload without changing the lock file.

.DESCRIPTION
    Downloads the locked CEF distribution URI when called without arguments,
    or accepts a CEF distribution archive or extracted CEF distribution
    directory as a fallback. Stages the AppCef dependency files and verifies
    the staged runtime payload against AppCef\cef-lock.json.

.PARAMETER StageKind
    Legacy restores IntelPresentMon\AppCef\Cef for MSBuild. CMake restores
    build\ThirdParty\cef and requires the source archive to match the lock.

.PARAMETER Generator
    CMake generator used to build the CEF wrapper.

.PARAMETER Platform
    Optional CMake generator platform used to build the CEF wrapper.

.PARAMETER Toolset
    Optional CMake generator toolset used to build the CEF wrapper.
#>

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$SourcePath,

    [Parameter()]
    [ValidateSet('Legacy', 'CMake')]
    [string]$StageKind = 'Legacy',

    [Parameter()]
    [string]$Generator = 'Visual Studio 17 2022',

    [Parameter()]
    [string]$Platform = 'x64',

    [Parameter()]
    [string]$Toolset = 'v143'
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'cef-lock.psm1') -Force -DisableNameChecking

$stagePath = Get-CefStagePath -StageKind $StageKind
Write-Host "Restoring the $StageKind CEF stage at $stagePath"

$lock = Read-CefLock
if (-not $SourcePath) {
    $sourceUri = Get-ObjectPropertyValue -Object $lock.source -Name 'uri'
    $sourcePath = Get-ObjectPropertyValue -Object $lock.source -Name 'path'
    $sourceType = Get-ObjectPropertyValue -Object $lock.source -Name 'type'
    if ($sourceUri) {
        $SourcePath = $sourceUri
    } elseif ($sourcePath -and ($sourceType -in @('archive', 'directory'))) {
        $candidate = Join-Path (Get-RepoRoot) ($sourcePath -replace '/', '\')
        if (Test-Path $candidate) {
            $SourcePath = $candidate
        }
    }
}

if (-not $SourcePath) {
    throw 'The CEF lock does not define a source URI. Provide path\to\cef_archive.tar.bz2 as a fallback.'
}

$completed = $false
try {
    $lockedUri = Get-ObjectPropertyValue -Object $lock.source -Name 'uri'
    $lockedSha256 = Get-ObjectPropertyValue -Object $lock.source -Name 'sha256'
    if (($StageKind -eq 'CMake') -and (-not $lockedSha256)) {
        throw 'The CEF lock must contain a source SHA-256 before restoring the CMake stage.'
    }
    if (($StageKind -eq 'CMake') -and (Test-UriSource -Source $SourcePath)) {
        if (-not $lockedUri) {
            throw 'The CEF lock does not define a source URI for the CMake stage.'
        }
        $requestedUri = ([System.Uri]$SourcePath).AbsoluteUri
        $expectedUri = ([System.Uri]$lockedUri).AbsoluteUri
        if (-not $requestedUri.Equals($expectedUri, [System.StringComparison]::Ordinal)) {
            throw "The CMake CEF stage accepts only the URI recorded in the lock: $lockedUri"
        }
    }

    $source = Resolve-CefSource -Source $SourcePath
    $sourceArchivePath = Get-ObjectPropertyValue -Object $source -Name 'archivePath'
    $sourceKind = Get-ObjectPropertyValue -Object $source -Name 'type'

    if (($StageKind -eq 'CMake') -and ($sourceKind -eq 'directory')) {
        throw 'The CMake CEF stage requires the locked URI or a local copy of the locked archive. Extracted directories are supported only for the Legacy stage.'
    }

    if (Test-Path $sourceArchivePath -PathType Leaf) {
        $actualHash = Get-FileSha256 -Path $sourceArchivePath
        if ($lockedSha256 -and ($actualHash -ne $lockedSha256)) {
            throw "CEF archive hash does not match lock. Expected $lockedSha256, found $actualHash."
        }
    }

    $cefRoot = Resolve-CefDistributionRoot -Path $sourceArchivePath
    Install-CefStage -CefRoot $cefRoot -StageKind $StageKind -Generator $Generator -Platform $Platform -Toolset $Toolset
    $completed = $true
} finally {
    if ($completed) {
        if (Test-CefKeepWorkDirectories) {
            Write-Host "Keeping CEF work directories because PRESENTMON_CEF_KEEP_WORK is set."
        } else {
            Clear-CefTempDirectories
        }
    } elseif ((Get-CefTempDirectories).Count -ne 0) {
        Write-Host "Leaving CEF work directories after failed pull:"
        Get-CefTempDirectories | ForEach-Object { Write-Host "  $_" }
    }
}
