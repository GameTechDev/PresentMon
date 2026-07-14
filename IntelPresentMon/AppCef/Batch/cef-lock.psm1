Set-StrictMode -Version 3.0

$ErrorActionPreference = 'Stop'

$script:CefTempDirectories = New-Object System.Collections.Generic.List[string]

function Get-AppCefRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot '..')).ProviderPath
}

function Get-RepoRoot {
    return (Resolve-Path (Join-Path (Get-AppCefRoot) '..\..')).ProviderPath
}

function Get-CefLockPath {
    param([string]$Platform = 'x64')
    $name = if ($Platform -ieq 'arm64') { 'cef-lock.arm64.json' } else { 'cef-lock.json' }
    return Join-Path (Get-AppCefRoot) $name
}

function Get-CefStagePath {
    return Join-Path (Get-AppCefRoot) 'Cef'
}

function Test-UriSource {
    param([Parameter(Mandatory = $true)][string]$Source)

    $uri = $null
    if (-not [System.Uri]::TryCreate($Source, [System.UriKind]::Absolute, [ref]$uri)) {
        return $false
    }

    return $uri.Scheme -in @('http', 'https')
}

function Get-CefTempRootCandidates {
    $roots = @()
    if ($env:PRESENTMON_CEF_WORK_ROOT) {
        return @($env:PRESENTMON_CEF_WORK_ROOT)
    }

    if ($env:SystemDrive) {
        $roots += (Join-Path ($env:SystemDrive + '\') 'pcef')
    }
    $roots += [System.IO.Path]::GetTempPath()
    return $roots
}

function New-CefTempDirectory {
    param([Parameter(Mandatory = $true)][string]$Prefix)

    $name = $Prefix + '-' + ([System.IO.Path]::GetRandomFileName() -replace '\.', '')
    $roots = Get-CefTempRootCandidates

    foreach ($root in $roots) {
        $path = $null
        try {
            New-Item -ItemType Directory -Force -Path $root | Out-Null
            $path = Join-Path $root $name
            New-Item -ItemType Directory -Path $path | Out-Null
            $resolvedPath = (Resolve-Path $path).ProviderPath
            $script:CefTempDirectories.Add($resolvedPath)
            return $resolvedPath
        } catch {
            if ($path -and (Test-Path $path)) {
                Remove-Item -LiteralPath $path -Recurse -Force
            }
            if ($env:PRESENTMON_CEF_WORK_ROOT) {
                throw "Failed to create a CEF temp directory under PRESENTMON_CEF_WORK_ROOT: $root"
            }
        }
    }

    throw "Failed to create a CEF temp directory."
}

function Test-CefKeepWorkDirectories {
    return $env:PRESENTMON_CEF_KEEP_WORK -in @('1', 'true', 'True', 'TRUE', 'yes', 'Yes', 'YES')
}

function Get-CefTempDirectories {
    return @($script:CefTempDirectories)
}

function Clear-CefTempDirectories {
    foreach ($path in @($script:CefTempDirectories)) {
        if (-not (Test-Path $path)) {
            continue
        }
        try {
            Remove-Item -LiteralPath $path -Recurse -Force
            Write-Host "Removed CEF work directory: $path"
        } catch {
            Write-Warning "Failed to remove CEF work directory: $path"
        }
    }
    $script:CefTempDirectories.Clear()
}

function Read-CefLock {
    param([string]$Platform = 'x64')
    $path = Get-CefLockPath -Platform $Platform
    if (-not (Test-Path $path -PathType Leaf)) {
        throw "CEF lock file not found: $path"
    }
    return Get-Content $path -Raw | ConvertFrom-Json
}

function Get-FileSha256 {
    param([Parameter(Mandatory = $true)][string]$Path)

    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    try {
        $stream = [System.IO.File]::OpenRead($Path)
        try {
            $hashBytes = $sha256.ComputeHash($stream)
        }
        finally {
            $stream.Dispose()
        }
    }
    finally {
        $sha256.Dispose()
    }

    return ([System.BitConverter]::ToString($hashBytes)).Replace('-', '').ToLowerInvariant()
}

function Get-ObjectPropertyValue {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return $Object[$Name]
        }
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($property) {
        return $property.Value
    }

    return $null
}

function ConvertTo-RepoRelativePath {
    param([Parameter(Mandatory = $true)][string]$Path)
    $repoRoot = Get-RepoRoot
    $fullPath = (Resolve-Path $Path).ProviderPath
    if (-not $fullPath.StartsWith($repoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $null
    }
    return $fullPath.Substring($repoRoot.Length + 1).Replace('\', '/')
}

function Get-FileNameFromUri {
    param([Parameter(Mandatory = $true)][string]$Uri)

    $absoluteUri = [System.Uri]$Uri
    $name = [System.IO.Path]::GetFileName($absoluteUri.LocalPath)
    if (-not $name) {
        $name = 'cef_archive.tar.bz2'
    }
    return $name
}

function Save-CefUriArchive {
    param([Parameter(Mandatory = $true)][string]$Uri)

    $downloadRoot = New-CefTempDirectory -Prefix 'download'
    $archivePath = Join-Path $downloadRoot (Get-FileNameFromUri -Uri $Uri)
    Write-Host "Downloading CEF archive from $Uri"
    Invoke-WebRequest -Uri $Uri -OutFile $archivePath
    return $archivePath
}

function Invoke-CefArchiveExtract {
    param(
        [Parameter(Mandatory = $true)][string]$ArchivePath,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $attemptErrors = New-Object System.Collections.Generic.List[string]
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmake) {
        Push-Location $Destination
        try {
            $global:LASTEXITCODE = 0
            & $cmake.Source -E tar xjf $ArchivePath
            if ($LASTEXITCODE -eq 0) {
                return
            }
            $attemptErrors.Add("cmake -E tar xjf failed with exit code $LASTEXITCODE.")
        } finally {
            Pop-Location
        }
    } else {
        $attemptErrors.Add("cmake was not found in PATH.")
    }

    $tar = Get-Command tar -ErrorAction SilentlyContinue
    if ($tar) {
        $global:LASTEXITCODE = 0
        & $tar.Source -xf $ArchivePath -C $Destination
        if ($LASTEXITCODE -eq 0) {
            return
        }
        $attemptErrors.Add("tar -xf failed with exit code $LASTEXITCODE.")
    } else {
        $attemptErrors.Add("tar was not found in PATH.")
    }

    throw "Failed to extract CEF archive: $ArchivePath`nTried CMake and tar extractors. Ensure CMake is installed or install bzip2 so tar can read .tar.bz2 archives.`n$($attemptErrors -join "`n")"
}

function Resolve-CefSource {
    param([Parameter(Mandatory = $true)][string]$Source)

    if (Test-UriSource -Source $Source) {
        $archivePath = Save-CefUriArchive -Uri $Source
        return [ordered]@{
            type = 'uri'
            input = $Source
            archivePath = $archivePath
        }
    }

    $resolved = (Resolve-Path $Source).ProviderPath
    $sourceInfo = Get-Item -LiteralPath $resolved
    return [ordered]@{
        type = if ($sourceInfo.PSIsContainer) { 'directory' } else { 'archive' }
        input = $resolved
        archivePath = $resolved
    }
}

function Read-CefVersionMetadata {
    param([Parameter(Mandatory = $true)][string]$CefRoot)

    $versionPath = Join-Path $CefRoot 'include\cef_version.h'
    if (-not (Test-Path $versionPath -PathType Leaf)) {
        $versionPath = Join-Path $CefRoot 'Include\include\cef_version.h'
    }
    if (-not (Test-Path $versionPath -PathType Leaf)) {
        throw "CEF version header not found: $versionPath"
    }

    $text = Get-Content $versionPath -Raw
    $metadata = [ordered]@{}
    foreach ($name in @(
        'CEF_VERSION',
        'CEF_VERSION_MAJOR',
        'CEF_VERSION_MINOR',
        'CEF_VERSION_PATCH',
        'CEF_COMMIT_NUMBER',
        'CEF_COMMIT_HASH',
        'CHROME_VERSION_MAJOR',
        'CHROME_VERSION_MINOR',
        'CHROME_VERSION_BUILD',
        'CHROME_VERSION_PATCH'
    )) {
        $match = [regex]::Match($text, "(?m)^#define\s+$name\s+(.+?)\s*$")
        if ($match.Success) {
            $value = $match.Groups[1].Value.Trim().Trim('"')
            $number = 0
            if ([int]::TryParse($value, [ref]$number)) {
                $metadata[$name] = $number
            } else {
                $metadata[$name] = $value
            }
        }
    }

    if (-not $metadata.Contains('CEF_VERSION')) {
        throw "CEF_VERSION was not found in $versionPath"
    }

    return $metadata
}

function Resolve-CefDistributionRoot {
    param([Parameter(Mandatory = $true)][string]$Path)

    $resolved = (Resolve-Path $Path).ProviderPath
    if (Test-Path $resolved -PathType Leaf) {
        $tempRoot = New-CefTempDirectory -Prefix 'extract'
        Write-Host "Extracting CEF archive to $tempRoot"
        Invoke-CefArchiveExtract -ArchivePath $resolved -Destination $tempRoot
        $resolved = $tempRoot
    }

    if (-not (Test-Path $resolved -PathType Container)) {
        throw "CEF source is not a file or directory: $Path"
    }

    $candidates = @($resolved)
    $candidates += @(Get-ChildItem -LiteralPath $resolved -Directory | ForEach-Object { $_.FullName })
    foreach ($candidate in $candidates) {
        if ((Test-Path (Join-Path $candidate 'CMakeLists.txt') -PathType Leaf) -and
            (Test-Path (Join-Path $candidate 'Release') -PathType Container) -and
            (Test-Path (Join-Path $candidate 'Resources') -PathType Container) -and
            (Test-Path (Join-Path $candidate 'include') -PathType Container)) {
            return (Resolve-Path $candidate).ProviderPath
        }
    }

    throw "Could not locate a CEF distribution root under: $Path"
}

function Assert-CefWrapperBuilt {
    param(
        [Parameter(Mandatory = $true)][string]$CefRoot,
        [string]$Platform = 'x64'
    )

    $buildScript = Join-Path $PSScriptRoot 'cef-build-wrapper.ps1'
    Write-Host "Building CEF wrapper ($Platform) from a clean build directory."
    & $buildScript $CefRoot -Platform $Platform -Clean
    if ($LASTEXITCODE -ne 0) {
        throw "CEF wrapper build failed with exit code $LASTEXITCODE."
    }
}

function Copy-DirectoryContents {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination,
        [Parameter()][string[]]$ExcludeFileName = @()
    )

    if (-not (Test-Path $Source -PathType Container)) {
        throw "Source directory not found: $Source"
    }
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Get-ChildItem -LiteralPath $Source -Force |
        Where-Object { $_.PSIsContainer -or ($_.Name -notin $ExcludeFileName) } |
        Copy-Item -Destination $Destination -Recurse -Force
}

function Stage-CefDistribution {
    param(
        [Parameter(Mandatory = $true)][string]$CefRoot,
        [string]$Platform = 'x64'
    )

    Assert-CefWrapperBuilt -CefRoot $CefRoot -Platform $Platform

    $stage = Get-CefStagePath
    $appRoot = Get-AppCefRoot
    $resolvedAppRoot = (Resolve-Path $appRoot).ProviderPath
    $stageParent = (Resolve-Path (Split-Path -Parent $stage)).ProviderPath
    if (-not $stageParent.Equals($resolvedAppRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean unexpected CEF stage path: $stage"
    }

    if (Test-Path $stage) {
        Remove-Item -LiteralPath $stage -Recurse -Force
    }

    New-Item -ItemType Directory -Force -Path (Join-Path $stage 'Bin') | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $stage 'Lib\Debug') | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $stage 'Lib\Release') | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $stage 'Include') | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $stage 'Resources') | Out-Null

    Copy-DirectoryContents -Source (Join-Path $CefRoot 'Release') -Destination (Join-Path $stage 'Bin') -ExcludeFileName @('cef_sandbox.lib', 'libcef.lib')
    Copy-DirectoryContents -Source (Join-Path $CefRoot 'include') -Destination (Join-Path $stage 'Include\include')
    Copy-DirectoryContents -Source (Join-Path $CefRoot 'Resources') -Destination (Join-Path $stage 'Resources')

    Copy-Item -LiteralPath (Join-Path $CefRoot 'Release\libcef.lib') -Destination (Join-Path $stage 'Lib\Debug\libcef.lib') -Force
    Copy-Item -LiteralPath (Join-Path $CefRoot 'Release\libcef.lib') -Destination (Join-Path $stage 'Lib\Release\libcef.lib') -Force
    Copy-DirectoryContents -Source (Join-Path $CefRoot 'build\libcef_dll_wrapper\Debug') -Destination (Join-Path $stage 'Lib\Debug')
    Copy-DirectoryContents -Source (Join-Path $CefRoot 'build\libcef_dll_wrapper\Release') -Destination (Join-Path $stage 'Lib\Release')

    $localesPath = Join-Path $stage 'Resources\locales'
    if (Test-Path $localesPath -PathType Container) {
        Get-ChildItem -LiteralPath $localesPath -File | Where-Object { $_.Name -ine 'en-US.pak' } | Remove-Item -Force
    }

    Write-Host "Staged CEF files at $stage"
}

function Get-CefPayloadEntriesFromStage {
    $stage = Get-CefStagePath
    $entries = @()
    $groups = @(
        @{ Name = 'Bin'; Root = (Join-Path $stage 'Bin'); OutputPrefix = '' },
        @{ Name = 'Resources'; Root = (Join-Path $stage 'Resources'); OutputPrefix = '' }
    )

    foreach ($group in $groups) {
        if (-not (Test-Path $group.Root -PathType Container)) {
            throw "CEF staged $($group.Name) directory is missing: $($group.Root)"
        }

        $root = (Resolve-Path $group.Root).ProviderPath
        $files = Get-ChildItem -LiteralPath $root -Recurse -File | Sort-Object FullName
        foreach ($file in $files) {
            $relative = $file.FullName.Substring($root.Length + 1).Replace('\', '/')
            $stagePath = "$($group.Name)/$relative"
            $outputPath = $relative
            $entries += [ordered]@{
                path = $outputPath
                stagePath = $stagePath
                group = $group.Name
                size = $file.Length
                sha256 = Get-FileSha256 -Path $file.FullName
            }
        }
    }

    return @($entries | Sort-Object { $_.path })
}

function New-CefLockObject {
    param(
        [Parameter(Mandatory = $true)][string]$CefRoot,
        [Parameter(Mandatory = $true)]$Source
    )

    $sourceKind = Get-ObjectPropertyValue -Object $Source -Name 'type'
    if ($sourceKind -eq 'uri') {
        $sourceInput = Get-ObjectPropertyValue -Object $Source -Name 'input'
        $sourceArchivePath = Get-ObjectPropertyValue -Object $Source -Name 'archivePath'
        $sourceInfo = Get-Item -LiteralPath $sourceArchivePath
        $source = [ordered]@{
            type = 'uri'
            uri = $sourceInput
            fileName = $sourceInfo.Name
            size = $sourceInfo.Length
            sha256 = Get-FileSha256 -Path $sourceInfo.FullName
        }
    } else {
        $sourceInput = Get-ObjectPropertyValue -Object $Source -Name 'input'
        $sourceResolved = (Resolve-Path $sourceInput).ProviderPath
        $sourceInfo = Get-Item -LiteralPath $sourceResolved
        $sourceType = if ($sourceInfo.PSIsContainer) {
            $stagePath = (Resolve-Path (Get-CefStagePath)).ProviderPath
            if ($sourceResolved.Equals($stagePath, [System.StringComparison]::OrdinalIgnoreCase)) { 'staged' } else { 'directory' }
        } else {
            'archive'
        }
        $source = [ordered]@{
            type = $sourceType
            fileName = $sourceInfo.Name
        }
        $repoRelativePath = ConvertTo-RepoRelativePath -Path $sourceResolved
        if ($repoRelativePath) {
            $source.path = $repoRelativePath
        }
        if (-not $sourceInfo.PSIsContainer) {
            $source.size = $sourceInfo.Length
            $source.sha256 = Get-FileSha256 -Path $sourceResolved
        }
    }

    return [ordered]@{
        schemaVersion = 1
        generatedBy = 'IntelPresentMon/AppCef/Batch/upgrade-cef.ps1'
        source = $source
        cef = Read-CefVersionMetadata -CefRoot $CefRoot
        payload = Get-CefPayloadEntriesFromStage
    }
}

function Write-CefLock {
    param(
        [Parameter(Mandatory = $true)]$Lock,
        [string]$Platform = 'x64'
    )

    $path = Get-CefLockPath -Platform $Platform
    $json = $Lock | ConvertTo-Json -Depth 8
    [System.IO.File]::WriteAllText($path, $json + "`r`n", [System.Text.UTF8Encoding]::new($false))
    Write-Host "Updated CEF lock: $path"
}

function Compare-CefPayload {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][ValidateSet('Stage', 'Output')][string]$Mode
    )

    $errors = New-Object System.Collections.Generic.List[string]
    $expectedByPath = @{}
    foreach ($entry in $Expected) {
        $relative = if ($Mode -eq 'Stage') { $entry.stagePath } else { $entry.path }
        $expectedByPath[$relative.ToLowerInvariant()] = $entry
        $fullPath = Join-Path $Root ($relative -replace '/', '\')
        if (-not (Test-Path $fullPath -PathType Leaf)) {
            $errors.Add("Missing CEF file: $relative")
            continue
        }
        $item = Get-Item -LiteralPath $fullPath
        if ($item.Length -ne [int64]$entry.size) {
            $errors.Add("Size mismatch for $relative; expected $($entry.size), found $($item.Length)")
        }
        $hash = Get-FileSha256 -Path $fullPath
        if ($hash -ne $entry.sha256) {
            $errors.Add("Hash mismatch for $relative; expected $($entry.sha256), found $hash")
        }
    }

    if (($Mode -eq 'Stage') -and (Test-Path $Root -PathType Container)) {
        $resolvedRoot = (Resolve-Path $Root).ProviderPath
        $payloadRoots = @($Expected | ForEach-Object { ($_.stagePath -split '/')[0] } | Sort-Object -Unique)
        foreach ($payloadRoot in $payloadRoots) {
            $scanRoot = Join-Path $resolvedRoot $payloadRoot
            if (-not (Test-Path $scanRoot -PathType Container)) {
                continue
            }
            $actualFiles = Get-ChildItem -LiteralPath $scanRoot -Recurse -File
            foreach ($file in $actualFiles) {
                $relative = $file.FullName.Substring($resolvedRoot.Length + 1).Replace('\', '/')
                if (-not $expectedByPath.ContainsKey($relative.ToLowerInvariant())) {
                    $errors.Add("Unexpected CEF file: $relative")
                }
            }
        }
    }

    return $errors
}

function Assert-CefStageMatchesLock {
    param([string]$Platform = 'x64')
    $lock = Read-CefLock -Platform $Platform
    $metadata = Read-CefVersionMetadata -CefRoot (Get-CefStagePath)
    foreach ($name in $lock.cef.PSObject.Properties.Name) {
        if ([string]$metadata[$name] -ne [string]$lock.cef.$name) {
            throw "Staged CEF metadata does not match lock for $name. Expected $($lock.cef.$name), found $($metadata[$name])."
        }
    }
    $errors = @(Compare-CefPayload -Expected $lock.payload -Root (Get-CefStagePath) -Mode Stage)
    if ($errors.Count -ne 0) {
        throw "Staged CEF payload does not match $(Get-CefLockPath -Platform $Platform):`n$($errors -join "`n")"
    }
    Write-Host "CEF staged payload matches lock ($Platform)."
}

function Assert-CefOutputMatchesLock {
    param(
        [Parameter(Mandatory = $true)][string]$OutputRoot,
        [string]$Platform = 'x64'
    )

    $lock = Read-CefLock -Platform $Platform
    $resolved = (Resolve-Path $OutputRoot).ProviderPath
    $errors = @(Compare-CefPayload -Expected $lock.payload -Root $resolved -Mode Output)
    if ($errors.Count -ne 0) {
        throw "CEF output payload does not match $(Get-CefLockPath -Platform $Platform):`n$($errors -join "`n")"
    }
    Write-Host "CEF output payload matches lock ($Platform)."
}

function Get-StableWixId {
    param(
        [Parameter(Mandatory = $true)][string]$Prefix,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $safe = [regex]::Replace($Path, '[^A-Za-z0-9_\.]', '_')
    if ($safe.Length -gt 45) {
        $safe = $safe.Substring($safe.Length - 45)
    }
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Path)
    $sha = [System.Security.Cryptography.SHA256]::Create().ComputeHash($bytes)
    $hash = -join ($sha[0..3] | ForEach-Object { $_.ToString('x2') })
    return "${Prefix}_${safe}_${hash}"
}

function Write-CefWixFragment {
    param(
        [Parameter(Mandatory = $true)][string]$OutPath,
        [Parameter(Mandatory = $true)][string]$ComponentGroup,
        [Parameter(Mandatory = $true)]$Entries
    )

    $settings = New-Object System.Xml.XmlWriterSettings
    $settings.Indent = $true
    $settings.Encoding = [System.Text.UTF8Encoding]::new($false)
    $settings.NewLineChars = "`r`n"

    $writer = [System.Xml.XmlWriter]::Create($OutPath, $settings)
    try {
        $writer.WriteStartDocument()
        $writer.WriteStartElement('Wix', 'http://schemas.microsoft.com/wix/2006/wi')

        $writer.WriteStartElement('Fragment')
        $writer.WriteStartElement('DirectoryRef')
        $writer.WriteAttributeString('Id', 'pm_app_folder')
        $dirs = @($Entries | ForEach-Object {
            $dir = Split-Path $_.path -Parent
            if ($dir) { $dir.Replace('\', '/') }
        } | Sort-Object -Unique)
        foreach ($dir in $dirs) {
            if ($dir.Contains('/')) {
                throw "Nested CEF installer directories deeper than one level are not supported yet: $dir"
            }
            $writer.WriteStartElement('Directory')
            $writer.WriteAttributeString('Id', (Get-StableWixId -Prefix 'cef_dir' -Path $dir))
            $writer.WriteAttributeString('Name', $dir)
            $writer.WriteEndElement()
        }
        $writer.WriteEndElement()
        $writer.WriteEndElement()

        $writer.WriteStartElement('Fragment')
        $writer.WriteStartElement('ComponentGroup')
        $writer.WriteAttributeString('Id', $ComponentGroup)

        foreach ($entry in @($Entries | Sort-Object path)) {
            $dir = Split-Path $entry.path -Parent
            $fileName = Split-Path $entry.path -Leaf
            $directoryId = if ($dir) { Get-StableWixId -Prefix 'cef_dir' -Path $dir.Replace('\', '/') } else { 'pm_app_folder' }
            $componentId = Get-StableWixId -Prefix 'cef_cmp' -Path $entry.path
            $fileId = Get-StableWixId -Prefix 'cef_file' -Path $entry.path
            $source = '$(var.PresentMon.TargetDir)\' + ($entry.path -replace '/', '\')

            $writer.WriteStartElement('Component')
            $writer.WriteAttributeString('Id', $componentId)
            $writer.WriteAttributeString('Directory', $directoryId)
            $writer.WriteAttributeString('Guid', '*')
            $writer.WriteStartElement('File')
            $writer.WriteAttributeString('Id', $fileId)
            $writer.WriteAttributeString('Name', $fileName)
            $writer.WriteAttributeString('KeyPath', 'yes')
            $writer.WriteAttributeString('Source', $source)
            $writer.WriteEndElement()
            $writer.WriteEndElement()
        }

        $writer.WriteEndElement()
        $writer.WriteEndElement()
        $writer.WriteEndElement()
        $writer.WriteEndDocument()
    } finally {
        $writer.Close()
    }
}

function Update-CefInstallerFragments {
    $lock = Read-CefLock
    $installerRoot = Join-Path (Get-AppCefRoot) '..\PMInstaller'
    $binEntries = @($lock.payload | Where-Object { $_.group -eq 'Bin' })
    $resourceEntries = @($lock.payload | Where-Object { $_.group -eq 'Resources' })
    Write-CefWixFragment -OutPath (Join-Path $installerRoot 'CefBinaries.wxs') -ComponentGroup 'CefBinaries' -Entries $binEntries
    Write-CefWixFragment -OutPath (Join-Path $installerRoot 'CefResources.wxs') -ComponentGroup 'CefResources' -Entries $resourceEntries
    Write-Host "Updated CEF WiX fragments."
}

function Assert-CefInstallerInputsMatchLock {
    $lock = Read-CefLock
    $temp = Join-Path ([System.IO.Path]::GetTempPath()) ("presentmon-cef-wix-" + [System.Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $temp | Out-Null
    try {
        $installerRoot = Join-Path (Get-AppCefRoot) '..\PMInstaller'
        Write-CefWixFragment -OutPath (Join-Path $temp 'CefBinaries.wxs') -ComponentGroup 'CefBinaries' -Entries @($lock.payload | Where-Object { $_.group -eq 'Bin' })
        Write-CefWixFragment -OutPath (Join-Path $temp 'CefResources.wxs') -ComponentGroup 'CefResources' -Entries @($lock.payload | Where-Object { $_.group -eq 'Resources' })

        foreach ($name in @('CefBinaries.wxs', 'CefResources.wxs')) {
            $expected = Get-Content (Join-Path $temp $name) -Raw
            $actualPath = Join-Path $installerRoot $name
            if (-not (Test-Path $actualPath -PathType Leaf)) {
                throw "Installer CEF fragment is missing: $actualPath"
            }
            $actual = Get-Content $actualPath -Raw
            if ($actual -ne $expected) {
                throw "Installer CEF fragment is stale: $actualPath. Run IntelPresentMon\AppCef\Batch\upgrade-cef.ps1 to refresh it."
            }
        }
    } finally {
        if (Test-Path $temp) {
            Remove-Item -LiteralPath $temp -Recurse -Force
        }
    }
    Write-Host "CEF installer fragments match lock."
}
