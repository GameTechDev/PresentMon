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
    [string]$Source
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'cef-lock.psm1') -Force -DisableNameChecking

$resolvedSource = Resolve-CefSource -Source $Source
$cefRoot = Resolve-CefDistributionRoot -Path (Get-ObjectPropertyValue -Object $resolvedSource -Name 'archivePath')
Stage-CefDistribution -CefRoot $cefRoot
$lock = New-CefLockObject -CefRoot $cefRoot -Source $resolvedSource
Write-CefLock -Lock $lock
Update-CefInstallerFragments
Assert-CefStageMatchesLock
Assert-CefInstallerInputsMatchLock
