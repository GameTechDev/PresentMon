<#
.SYNOPSIS
    Validate CEF files against the repository lock.

.PARAMETER Mode
    Stage validates a restored CEF stage. Output validates a build output
    directory that received the locked CEF runtime payload. Installer also
    validates the generated installer fragments.

.PARAMETER OutputRoot
    Directory holding the copied CEF runtime payload. Required for Output and
    Installer validation.

.PARAMETER StageKind
    Selects the fixed Legacy or CMake stage. Legacy is the default.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Stage', 'Output', 'Installer')]
    [string]$Mode,

    [Parameter()]
    [string]$OutputRoot,

    [Parameter()]
    [ValidateSet('Legacy', 'CMake')]
    [string]$StageKind = 'Legacy'
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'cef-lock.psm1') -Force -DisableNameChecking

if ($Mode -eq 'Stage') {
    Assert-CefStageMatchesLock -StageKind $StageKind
    exit 0
}

if ($Mode -eq 'Output') {
    if (-not $OutputRoot) {
        throw 'OutputRoot is required for output CEF validation.'
    }
    Assert-CefOutputMatchesLock -OutputRoot $OutputRoot
    exit 0
}

Assert-CefInstallerInputsMatchLock
if (-not $OutputRoot) {
    throw 'OutputRoot is required for installer CEF validation.'
}
Assert-CefOutputMatchesLock -OutputRoot $OutputRoot
