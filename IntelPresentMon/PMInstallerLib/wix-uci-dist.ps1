[CmdletBinding()]
param(
    [string]$UciSdkDir,
    [string]$OutPath = 'UciDist.wxs',
    [string]$DistPath,
    [string]$WixRoot,
    [string]$HeatExe,
    [switch]$CMakePackaging,
    [string]$UciEnabled = ''
)

function Get-DeterministicGuid {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $md5 = [System.Security.Cryptography.MD5]::Create()
    try {
        $bytes = $md5.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($Name.ToLowerInvariant()))
    }
    finally {
        $md5.Dispose()
    }

    $bytes[6] = ($bytes[6] -band 0x0F) -bor 0x30
    $bytes[8] = ($bytes[8] -band 0x3F) -bor 0x80
    return (New-Object System.Guid -ArgumentList (, $bytes)).ToString().ToUpperInvariant()
}

function Get-DeterministicWixId {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [Parameter(Mandatory = $true)]
        [string]$Prefix
    )

    $md5 = [System.Security.Cryptography.MD5]::Create()
    try {
        $bytes = $md5.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($Name.ToLowerInvariant()))
    }
    finally {
        $md5.Dispose()
    }

    $hashHex = [System.BitConverter]::ToString($bytes).Replace('-', '').ToLowerInvariant()
    # Merge modules decorate Component/File ids with ".{ModulePackageGuid}" (~37 chars). MSI columns allow 72.
    return $Prefix + $hashHex.Substring(0, 8)
}

function Write-EmptyUciDistFragment {
    param(
        [Parameter(Mandatory = $true)]
        [string]$OutPath
    )

    $settings = New-Object System.Xml.XmlWriterSettings
    $settings.Encoding = New-Object System.Text.UTF8Encoding($false)
    $settings.Indent = $true
    $settings.IndentChars = '    '
    $settings.NewLineChars = "`r`n"
    $settings.NewLineHandling = [System.Xml.NewLineHandling]::Replace

    $writer = [System.Xml.XmlWriter]::Create($OutPath, $settings)
    try {
        $writer.WriteStartDocument()
        $writer.WriteStartElement('Wix', 'http://schemas.microsoft.com/wix/2006/wi')
        $writer.WriteStartElement('Fragment')
        $writer.WriteStartElement('ComponentGroup')
        $writer.WriteAttributeString('Id', 'uci_dist_files')
        $writer.WriteEndElement()
        $writer.WriteEndElement()
        $writer.WriteEndElement()
        $writer.WriteEndDocument()
    }
    finally {
        $writer.Dispose()
    }
}

function Resolve-UciEnabledFlag {
    param(
        [string]$RawValue,
        [bool]$DefaultWhenUnset
    )

    if ([string]::IsNullOrWhiteSpace($RawValue)) {
        return $DefaultWhenUnset
    }

    switch ($RawValue.Trim().ToLowerInvariant()) {
        '1' { return $true }
        'true' { return $true }
        'on' { return $true }
        'yes' { return $true }
        '0' { return $false }
        'false' { return $false }
        'off' { return $false }
        'no' { return $false }
        default {
            throw "Invalid -UciEnabled value '$RawValue'. Use true or false."
        }
    }
}

function Resolve-HeatExecutable {
    param(
        [string]$HeatExePath,
        [string]$WixRootPath
    )

    if (-not [string]::IsNullOrWhiteSpace($HeatExePath)) {
        if (-not (Test-Path -LiteralPath $HeatExePath -PathType Leaf)) {
            throw "heat.exe not found at '$HeatExePath'."
        }
        return $HeatExePath
    }

    $resolvedWixRoot = $WixRootPath
    if ([string]::IsNullOrWhiteSpace($resolvedWixRoot)) {
        $resolvedWixRoot = $env:WIX
    }
    if ([string]::IsNullOrWhiteSpace($resolvedWixRoot)) {
        throw 'WiX root is not set. Pass -WixRoot, -HeatExe, or set the WIX environment variable.'
    }

    $candidate = Join-Path $resolvedWixRoot 'bin\heat.exe'
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "heat.exe not found at '$candidate'. Verify the WiX installation path."
    }
    return $candidate
}

function Test-HarvestedRuntimeSources {
    param(
        [Parameter(Mandatory = $true)]
        [string]$StageRoot,

        [Parameter(Mandatory = $true)]
        [xml]$Document,

        [Parameter(Mandatory = $true)]
        [System.Xml.XmlNamespaceManager]$NamespaceManager
    )

    $stageFullPath = (Resolve-Path -LiteralPath $StageRoot).Path
    if (-not $stageFullPath.EndsWith('\')) {
        $stageFullPath += '\'
    }

    $fileNodes = $Document.SelectNodes('//w:File', $NamespaceManager)
    foreach ($fileNode in $fileNodes) {
        $source = $fileNode.GetAttribute('Source')
        if ([string]::IsNullOrWhiteSpace($source)) {
            throw 'Encountered a harvested File node without a Source attribute.'
        }

        if ($source -match '\.\.') {
            throw "Harvested source contains parent traversal: $source"
        }

        $normalizedSource = $source -replace '/', '\'
        if ($normalizedSource -match '^[a-zA-Z]:\\') {
            $resolvedSource = $normalizedSource
        }
        elseif ($normalizedSource -like '$(var.UciDistDir)*') {
            $relative = $normalizedSource.Substring('$(var.UciDistDir)'.Length).TrimStart('\')
            $resolvedSource = Join-Path $stageFullPath $relative
        }
        else {
            $resolvedSource = Join-Path $stageFullPath $normalizedSource.TrimStart('\')
        }

        $resolvedSource = [System.IO.Path]::GetFullPath($resolvedSource)
        if (-not $resolvedSource.StartsWith($stageFullPath, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Harvested source escapes the runtime stage: $source"
        }

        if (-not (Test-Path -LiteralPath $resolvedSource -PathType Leaf)) {
            throw "Harvested source file is missing from the runtime stage: $source"
        }

        $item = Get-Item -LiteralPath $resolvedSource -Force
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Harvested source is a reparse point: $resolvedSource"
        }

    }
}

$outDir = Split-Path -Parent $OutPath
if (-not [string]::IsNullOrWhiteSpace($outDir) -and -not (Test-Path $outDir -PathType Container)) {
    New-Item -Path $outDir -ItemType Directory -Force | Out-Null
}

$uciEnabledForCMake = $false
if ($CMakePackaging) {
    $uciEnabledForCMake = Resolve-UciEnabledFlag -RawValue $UciEnabled -DefaultWhenUnset $false
    if (-not $uciEnabledForCMake) {
        Write-EmptyUciDistFragment -OutPath $OutPath
        exit 0
    }

    if ([string]::IsNullOrWhiteSpace($DistPath)) {
        throw 'CMake UCI packaging requires -DistPath when UCI is enabled.'
    }
    if (-not (Test-Path -LiteralPath $DistPath -PathType Container)) {
        throw "UCI runtime stage directory was not found: $DistPath"
    }

    $dllPath = Join-Path $DistPath 'unified-collector-interface.dll'
    $collectorsPath = Join-Path $DistPath 'collectors'
    if (-not (Test-Path -LiteralPath $dllPath -PathType Leaf)) {
        throw "Enabled UCI runtime stage is missing unified-collector-interface.dll at $dllPath"
    }
    if (-not (Test-Path -LiteralPath $collectorsPath -PathType Container)) {
        throw "Enabled UCI runtime stage is missing collectors directory at $collectorsPath"
    }
    $collectorFile = Get-ChildItem -LiteralPath $collectorsPath -Recurse -File -Force | Select-Object -First 1
    if ($null -eq $collectorFile) {
        throw "Enabled UCI runtime stage collectors directory contains no files: $collectorsPath"
    }

    $distPathForHeat = (Resolve-Path -LiteralPath $DistPath).Path
}
else {
    Push-Location $PSScriptRoot

    if ([string]::IsNullOrWhiteSpace($UciSdkDir)) {
        $UciSdkDir = $env:PMON_UCI_SDK_DIR
    }
    if ([string]::IsNullOrWhiteSpace($UciSdkDir)) {
        $UciSdkDir = Join-Path $PSScriptRoot '..\ControlLib\uci\external'
    }

    $distPathForHeat = $UciSdkDir

    if (-not (Test-Path $distPathForHeat -PathType Container)) {
        Pop-Location
        Write-Warning "UCI dist directory '$distPathForHeat' was not found. Skipping UCI payload files."
        Write-EmptyUciDistFragment -OutPath $OutPath
        exit 0
    }

    if ($null -eq (Get-ChildItem -Path $distPathForHeat -Recurse -File -Force | Select-Object -First 1)) {
        Pop-Location
        Write-Warning "UCI dist directory '$distPathForHeat' contains no files. Skipping UCI payload files."
        Write-EmptyUciDistFragment -OutPath $OutPath
        exit 0
    }
}

$heatExePath = Resolve-HeatExecutable -HeatExePath $HeatExe -WixRootPath $WixRoot

& $heatExePath dir $distPathForHeat `
    -srd -sreg -scom `
    -dr service_folder `
    -cg uci_dist_files `
    -var var.UciDistDir `
    -suid -g1 -ag `
    -out $OutPath

$code = $LASTEXITCODE
if ($code -ne 0) {
    if (-not $CMakePackaging) {
        Pop-Location
    }
    Write-Error "heat.exe failed for '$distPathForHeat' (exit code $code)"
    exit $code
}

[xml]$doc = Get-Content $OutPath
$ns = New-Object System.Xml.XmlNamespaceManager($doc.NameTable)
$ns.AddNamespace('w', 'http://schemas.microsoft.com/wix/2006/wi')

if ($CMakePackaging) {
    Test-HarvestedRuntimeSources -StageRoot $distPathForHeat -Document $doc -NamespaceManager $ns
}

$componentNodes = $doc.SelectNodes('//w:Component', $ns)
foreach ($componentNode in $componentNodes) {
    $componentNode.SetAttribute('Win64', 'yes')
    $fileNode = $componentNode.SelectSingleNode('w:File', $ns)
    if ($null -eq $fileNode) {
        if (-not $CMakePackaging) {
            Pop-Location
        }
        Write-Error 'Encountered a harvested component without a File node.'
        exit 1
    }

    $source = $fileNode.GetAttribute('Source')
    $guid = Get-DeterministicGuid -Name $source
    $componentNode.SetAttribute('Guid', "{$guid}")
    $fileId = Get-DeterministicWixId -Name "file:$source" -Prefix 'f'
    $componentId = Get-DeterministicWixId -Name "cmp:$source" -Prefix 'c'
    $fileNode.SetAttribute('Id', $fileId)
    $componentNode.SetAttribute('Id', $componentId)
}

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$settings.Indent = $true
$settings.IndentChars = '    '
$settings.NewLineChars = "`r`n"
$settings.NewLineHandling = [System.Xml.NewLineHandling]::Replace

$writer = [System.Xml.XmlWriter]::Create((Resolve-Path $OutPath), $settings)
try {
    $doc.Save($writer)
}
finally {
    $writer.Dispose()
    if (-not $CMakePackaging) {
        Pop-Location
    }
}
