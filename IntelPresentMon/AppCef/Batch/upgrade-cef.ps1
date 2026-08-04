<#
.SYNOPSIS
    Upgrade the repository-pinned CEF payload and refresh generated inputs.

.DESCRIPTION
    Accepts a new CEF distribution URI for the normal maintainer workflow, or
    a local archive/extracted directory as a fallback. Stages the AppCef
    dependency files, updates AppCef\cef-lock.json, and regenerates the CEF
    WiX fragments used by the installer. This command can create the lock file
    when it does not already exist.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Source,

    [Parameter()]
    [ValidateSet('x64', 'arm64', 'ARM64')]
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'cef-lock.psm1') -Force -DisableNameChecking

$completed = $false
try {
    $resolvedSource = Resolve-CefSource -Source $Source
    $cefRoot = Resolve-CefDistributionRoot -Path (Get-ObjectPropertyValue -Object $resolvedSource -Name 'archivePath')
    Stage-CefDistribution -CefRoot $cefRoot -Platform $Platform
    $lock = New-CefLockObject -CefRoot $cefRoot -Source $resolvedSource
    Write-CefLock -Lock $lock -Platform $Platform
    # The installer WiX fragments are generated from the x64 payload only; ARM64
    # installer packaging is handled separately (multi-arch installer work).
    if ($Platform -ieq 'x64') {
        Update-CefInstallerFragments
    }
    Assert-CefStageMatchesLock -Platform $Platform
    if ($Platform -ieq 'x64') {
        Assert-CefInstallerInputsMatchLock
    }
    $completed = $true
} finally {
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
}
