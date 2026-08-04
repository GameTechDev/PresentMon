<#
.SYNOPSIS
  Out-of-source build of the CEF C++ wrapper.

.DESCRIPTION
  1. Takes a positional parameter: the path to your CEF redist root (the dir with CMakeLists.txt).
  2. If `<redist>/build` already exists, exits immediately (unless -Clean).
  3. Finds VS via vswhere and locates vcvarsall.bat.
  4. Creates and cds into `<redist>/build`.
  5. Invokes CMake directly, targeting the requested architecture and pinning the
     v143 toolset so the wrapper's C++ ABI/CRT matches the PresentMon UI app.
  6. Invokes msbuild for Release and then Debug (each in its own vcvarsall/msbuild invocation).

.PARAMETER RedistPath
  (Positional) Path to the root of your CEF redist directory.

.PARAMETER Platform
  Target architecture: x64 (default) or arm64. Selects the CMake -A value, the
  MSBuild platform, and the vcvarsall host_target argument.
#>

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$RedistPath,

    [Parameter()]
    [ValidateSet('x64','arm64','ARM64','x86','Win32')]
    [string]$Platform = 'x64',

    [Parameter()]
    [switch]$Clean
)

# Resolve & verify
try { $RedistPath = (Resolve-Path $RedistPath).ProviderPath } catch {
    Write-Error "Cannot resolve path: $RedistPath"; exit 1
}
if (-not (Test-Path $RedistPath -PathType Container)) {
    Write-Error "Not a valid directory: $RedistPath"; exit 1
}

# Skip if already built, unless the caller requested a clean wrapper build.
$buildDir = Join-Path $RedistPath "build"
if (Test-Path $buildDir) {
    if ($Clean) {
        $resolvedBuildDir = (Resolve-Path $buildDir).ProviderPath
        $resolvedRedistPath = (Resolve-Path $RedistPath).ProviderPath
        if (-not $resolvedBuildDir.StartsWith($resolvedRedistPath, [System.StringComparison]::OrdinalIgnoreCase)) {
            Write-Error "Refusing to clean unexpected build directory: $resolvedBuildDir"
            exit 1
        }
        Write-Host "Removing existing CEF wrapper build directory: $buildDir"
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    } else {
        Write-Host "Found existing build directory: $buildDir"
        Write-Host "Skipping configuration and build."
        exit 0
    }
}

# Pick the CMake Visual Studio generator that matches the installed VS major version.
$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { Write-Error "vswhere.exe not found"; exit 1 }
$vsVer = & $vswhere -latest -products * -prerelease -requires Microsoft.Component.MSBuild -property installationVersion
if (-not $vsVer) { Write-Error "No Visual Studio with MSBuild found"; exit 1 }
$vsMajor = ($vsVer -split '\.')[0]
$genMap = @{ '18' = 'Visual Studio 18 2026'; '17' = 'Visual Studio 17 2022'; '16' = 'Visual Studio 16 2019' }
$Generator = $genMap[$vsMajor]
if (-not $Generator) { Write-Error "Unsupported Visual Studio major version: $vsMajor"; exit 1 }

# Normalize the requested platform to the CMake -A architecture value.
switch -Regex ($Platform) {
    '^(arm64|ARM64)$'   { $CmakeArch = 'ARM64' }
    '^(x86|Win32)$'     { $CmakeArch = 'Win32' }
    default             { $CmakeArch = 'x64' }
}

Write-Host "CEF wrapper build: generator='$Generator' arch='$CmakeArch' toolset='v143'`n"

# Make build dir & enter
Write-Host "Creating build directory at $buildDir"
New-Item -ItemType Directory -Path $buildDir | Out-Null
Push-Location $buildDir

# 1) Configure with CMake. Pin the toolset to v143 so the static wrapper lib is
#    ABI/CRT-compatible with the v143 PresentMon UI app.
Write-Host "Running CMake configure..."
cmake -G "$Generator" -A $CmakeArch -T v143 -DUSE_SANDBOX=OFF "$RedistPath"
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed (exit code $LASTEXITCODE)"; Pop-Location; exit $LASTEXITCODE
}

# 2/3) Build the wrapper via CMake so the driver (msbuild/.sln vs .slnx) is chosen
#      by CMake and never hardcoded. --config selects the MSVC configuration.
foreach ($config in 'Release', 'Debug') {
    Write-Host "`nBuilding $config..."
    cmake --build $buildDir --config $config --target libcef_dll_wrapper -- /m
    if ($LASTEXITCODE -ne 0) {
        Write-Error "$config build failed (exit code $LASTEXITCODE)"; Pop-Location; exit $LASTEXITCODE
    }
}

Pop-Location
Write-Host "`nAll done: CEF wrapper built in $buildDir"
