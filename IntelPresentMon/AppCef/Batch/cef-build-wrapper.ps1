<#
.SYNOPSIS
    Build the CEF C++ wrapper with CMake.

.DESCRIPTION
    Configures an out-of-source Visual Studio build and builds both Release and
    Debug. The defaults match the supported PresentMon CMake toolchain.

.PARAMETER RedistPath
    Path to the root of the extracted CEF distribution.

.PARAMETER Clean
    Remove the existing wrapper build directory before configuring.

.PARAMETER Generator
    CMake generator used for the wrapper build.

.PARAMETER Platform
    CMake generator platform used for the wrapper build.

.PARAMETER Toolset
    CMake generator toolset used for the wrapper build.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$RedistPath,

    [Parameter()]
    [switch]$Clean,

    [Parameter()]
    [string]$Generator = 'Visual Studio 17 2022',

    [Parameter()]
    [string]$Platform = 'x64',

    [Parameter()]
    [string]$Toolset = 'v143'
)

$ErrorActionPreference = 'Stop'

$RedistPath = (Resolve-Path $RedistPath).ProviderPath
if (-not (Test-Path -LiteralPath $RedistPath -PathType Container)) {
    throw "Not a valid CEF distribution directory: $RedistPath"
}

$cmake = Get-Command cmake -ErrorAction Stop
$buildDir = Join-Path $RedistPath 'build'
if (Test-Path -LiteralPath $buildDir) {
    if ($Clean) {
        $resolvedBuildDir = (Resolve-Path $buildDir).ProviderPath
        $expectedBuildDir = [System.IO.Path]::GetFullPath((Join-Path $RedistPath 'build'))
        if (-not $resolvedBuildDir.Equals($expectedBuildDir, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to clean unexpected build directory: $resolvedBuildDir"
        }
        Write-Host "Removing existing CEF wrapper build directory: $buildDir"
        Remove-Item -LiteralPath $resolvedBuildDir -Recurse -Force
    } else {
        Write-Host "Found existing build directory: $buildDir"
        Write-Host 'Skipping configuration and build.'
        exit 0
    }
}

# CEF enables /MP without a process limit. On high-core-count systems that can
# exhaust compiler resources and surface as C1001 or D8040. Use a conservative
# default while allowing callers to select a different explicit limit.
$compilerProcessLimit = 4
if ($env:CL_MPCount) {
    $configuredCompilerProcessLimit = 0
    if (-not [int]::TryParse($env:CL_MPCount, [ref]$configuredCompilerProcessLimit) -or
        $configuredCompilerProcessLimit -lt 1 -or
        $configuredCompilerProcessLimit -gt 65536) {
        throw "CL_MPCount must be an integer between 1 and 65536: $env:CL_MPCount"
    }
    $compilerProcessLimit = $configuredCompilerProcessLimit
} else {
    $env:CL_MPCount = [string]$compilerProcessLimit
}

$configureArguments = @(
    '-S', $RedistPath,
    '-B', $buildDir,
    '-G', $Generator
)
if ($Platform) {
    $configureArguments += @('-A', $Platform)
}
if ($Toolset) {
    $configureArguments += @('-T', $Toolset)
}
$configureArguments += '-DUSE_SANDBOX=OFF'

$toolchainDescription = $Generator
if ($Platform) {
    $toolchainDescription += ", platform $Platform"
}
if ($Toolset) {
    $toolchainDescription += ", toolset $Toolset"
}
Write-Host "Configuring the CEF wrapper with $toolchainDescription."
& $cmake.Source @configureArguments
if ($LASTEXITCODE -ne 0) {
    throw "CEF wrapper CMake configuration failed with exit code $LASTEXITCODE."
}

foreach ($configuration in @('Release', 'Debug')) {
    Write-Host "Building the CEF wrapper for $configuration with up to $compilerProcessLimit compiler processes."
    & $cmake.Source --build $buildDir --config $configuration
    if ($LASTEXITCODE -ne 0) {
        throw "CEF wrapper $configuration build failed with exit code $LASTEXITCODE."
    }

    $wrapperLibrary = Join-Path $buildDir "libcef_dll_wrapper\$configuration\libcef_dll_wrapper.lib"
    if (-not (Test-Path -LiteralPath $wrapperLibrary -PathType Leaf)) {
        throw "CEF wrapper output was not produced: $wrapperLibrary"
    }
}

Write-Host "CEF wrapper Debug and Release builds completed in $buildDir"
