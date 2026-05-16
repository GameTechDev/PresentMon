<#
.SYNOPSIS
    Restore the repository-pinned CEF payload without changing the lock file.

.DESCRIPTION
    Downloads the locked CEF distribution URI when called without arguments,
    or accepts a CEF distribution archive or extracted CEF distribution
    directory as a fallback. Stages the AppCef dependency files and verifies
    the staged runtime payload against AppCef\cef-lock.json.
#>

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$SourcePath
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'cef-lock.psm1') -Force -DisableNameChecking

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

$source = Resolve-CefSource -Source $SourcePath
$sourceArchivePath = Get-ObjectPropertyValue -Object $source -Name 'archivePath'
$lockedSha256 = Get-ObjectPropertyValue -Object $lock.source -Name 'sha256'
if ((Test-Path $sourceArchivePath -PathType Leaf) -and $lockedSha256) {
    $actualHash = Get-FileSha256 -Path $sourceArchivePath
    if ($actualHash -ne $lockedSha256) {
        throw "CEF archive hash does not match lock. Expected $lockedSha256, found $actualHash."
    }
}

$cefRoot = Resolve-CefDistributionRoot -Path $sourceArchivePath
Stage-CefDistribution -CefRoot $cefRoot
Assert-CefStageMatchesLock
