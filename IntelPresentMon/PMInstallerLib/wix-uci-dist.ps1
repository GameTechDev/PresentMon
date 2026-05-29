[CmdletBinding()]
param()

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

    $writer = [System.Xml.XmlWriter]::Create((Join-Path $PSScriptRoot $OutPath), $settings)
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

Push-Location $PSScriptRoot

$uciSdkDir = $env:PMON_UCI_SDK_DIR
if ([string]::IsNullOrWhiteSpace($uciSdkDir)) {
    $uciSdkDir = '..\ControlLib\uci\external'
}

$distPath = Join-Path $uciSdkDir 'bin'
$outPath = 'UciDist.wxs'

if (-not (Test-Path $distPath -PathType Container)) {
    Pop-Location
    Write-Warning "UCI dist directory '$distPath' was not found. Skipping UCI payload files."
    Write-EmptyUciDistFragment -OutPath $outPath
    exit 0
}

if ($null -eq (Get-ChildItem -Path $distPath -Recurse -File -Force | Select-Object -First 1)) {
    Pop-Location
    Write-Warning "UCI dist directory '$distPath' contains no files. Skipping UCI payload files."
    Write-EmptyUciDistFragment -OutPath $outPath
    exit 0
}

if (-not $env:WIX) {
    Pop-Location
    Write-Error 'Environment variable WIX is not set. Please install WIX or set WIX to the WiX installation directory.'
    exit 1
}

$heatExe = Join-Path $env:WIX 'bin\heat.exe'
if (-not (Test-Path $heatExe)) {
    Pop-Location
    Write-Error "heat.exe not found at '$heatExe'. Verify your WIX environment variable points to the correct location."
    exit 1
}

& $heatExe dir $distPath `
    -srd -sreg -scom `
    -dr service_folder `
    -cg uci_dist_files `
    -var var.UciDistDir `
    -suid -g1 -ag `
    -out $outPath

$code = $LASTEXITCODE
if ($code -ne 0) {
    Pop-Location
    Write-Error "heat.exe failed for '$distPath' (exit code $code)"
    exit $code
}

[xml]$doc = Get-Content $outPath
$ns = New-Object System.Xml.XmlNamespaceManager($doc.NameTable)
$ns.AddNamespace('w', 'http://schemas.microsoft.com/wix/2006/wi')

$componentNodes = $doc.SelectNodes('//w:Component', $ns)
foreach ($componentNode in $componentNodes) {
    $componentNode.SetAttribute('Win64', 'yes')
    $fileNode = $componentNode.SelectSingleNode('w:File', $ns)
    if ($null -eq $fileNode) {
        Pop-Location
        Write-Error 'Encountered a harvested component without a File node.'
        exit 1
    }

    $source = $fileNode.GetAttribute('Source')
    $guid = Get-DeterministicGuid -Name $source
    $componentNode.SetAttribute('Guid', "{$guid}")
}

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$settings.Indent = $true
$settings.IndentChars = '    '
$settings.NewLineChars = "`r`n"
$settings.NewLineHandling = [System.Xml.NewLineHandling]::Replace

$writer = [System.Xml.XmlWriter]::Create((Resolve-Path $outPath), $settings)
try {
    $doc.Save($writer)
}
finally {
    $writer.Dispose()
    Pop-Location
}
