<#
.SYNOPSIS
    Validate AppCef CEF files against the repository lock.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Stage', 'Installer')]
    [string]$Mode,

    [Parameter()]
    [string]$OutputRoot,

    [Parameter()]
    [ValidateSet('x64', 'arm64', 'ARM64')]
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'cef-lock.psm1') -Force -DisableNameChecking

if ($Mode -eq 'Stage') {
    Assert-CefStageMatchesLock -Platform $Platform
    exit 0
}

Assert-CefInstallerInputsMatchLock
if (-not $OutputRoot) {
    throw 'OutputRoot is required for installer CEF validation.'
}
Assert-CefOutputMatchesLock -OutputRoot $OutputRoot -Platform $Platform
