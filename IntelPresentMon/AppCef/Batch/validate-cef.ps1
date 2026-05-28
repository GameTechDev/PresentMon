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
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'cef-lock.psm1') -Force -DisableNameChecking

if ($Mode -eq 'Stage') {
    Assert-CefStageMatchesLock
    exit 0
}

Assert-CefInstallerInputsMatchLock
if (-not $OutputRoot) {
    throw 'OutputRoot is required for installer CEF validation.'
}
Assert-CefOutputMatchesLock -OutputRoot $OutputRoot
