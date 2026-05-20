[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$repoRoot = $PSScriptRoot
$originalLocation = Get-Location

function Invoke-BootstrapStep {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [scriptblock]$Action
    )

    Write-Host ""
    Write-Host "==> $Name"
    $global:LASTEXITCODE = 0
    & $Action

    if ($LASTEXITCODE -ne $null -and $LASTEXITCODE -ne 0) {
        throw "$Name failed with exit code $LASTEXITCODE."
    }
}

try {
    Invoke-BootstrapStep "Pull CEF" {
        & (Join-Path $repoRoot "IntelPresentMon\AppCef\Batch\pull-cef.ps1")
    }

    Invoke-BootstrapStep "Pull auxiliary test data" {
        & (Join-Path $repoRoot "Tests\pull-aux.ps1")
    }

    Invoke-BootstrapStep "Build frontend" {
        Push-Location (Join-Path $repoRoot "IntelPresentMon\AppCef")
        try {
            & (Join-Path $repoRoot "IntelPresentMon\AppCef\Batch\build-web.bat")
        } finally {
            Pop-Location
        }
    }
} finally {
    Set-Location $originalLocation
}
