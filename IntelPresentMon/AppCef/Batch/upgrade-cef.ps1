<#
.SYNOPSIS
    Upgrade the repository-pinned CEF payload and refresh generated inputs.

.DESCRIPTION
    Accepts a new CEF distribution URI for the normal maintainer workflow, or
    a local archive as a fallback. Stages the AppCef dependency files, updates
    AppCef\cef-lock.json, and regenerates the CEF WiX fragments used by the
    installer. This command can create the lock file when it does not already
    exist.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Source
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'cef-lock.psm1') -Force -DisableNameChecking

$completed = $false
$stageMutex = Enter-CefStageLock -StageKind Legacy
try {
    $resolvedSource = Resolve-CefSource -Source $Source
    if ((Get-ObjectPropertyValue -Object $resolvedSource -Name 'type') -eq 'directory') {
        throw 'CEF lock upgrades require a URI or archive so the lock records an archive SHA-256. Extracted directories are supported only by the Legacy pull workflow.'
    }
    $cefRoot = Resolve-CefDistributionRoot -Path (Get-ObjectPropertyValue -Object $resolvedSource -Name 'archivePath')
    Stage-CefDistribution -CefRoot $cefRoot -StageKind Legacy
    $lock = New-CefLockObject -CefRoot $cefRoot -Source $resolvedSource
    Write-CefLock -Lock $lock
    Update-CefInstallerFragments
    Assert-CefStageMatchesLock -StageKind Legacy
    Assert-CefInstallerInputsMatchLock
    $completed = $true
} finally {
    try {
        if ($completed) {
            if (Test-CefKeepWorkDirectories) {
                Write-Host "Keeping CEF work directories because PRESENTMON_CEF_KEEP_WORK is set."
            } else {
                Clear-CefTempDirectories
            }
        } elseif ((Get-CefTempDirectories).Count -ne 0) {
            Write-Host "Leaving CEF work directories after failed upgrade:"
            Get-CefTempDirectories | ForEach-Object { Write-Host "  $_" }
        }
    } finally {
        Exit-CefStageLock -Mutex $stageMutex
    }
}
