param(
    [Parameter(Mandatory = $true)]
    [string]$WorkingDirectory
)

$ErrorActionPreference = "Stop"

Push-Location $WorkingDirectory
try {
    cmd /c "npm ci && npm run build"
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
