[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $ServiceExecutable,

    [Parameter(Mandatory = $true)]
    [string] $SampleClientExecutable,

    [Parameter(Mandatory = $true)]
    [string] $MiddlewareDll,

    [Parameter(Mandatory = $true)]
    [string] $LogDirectory,

    [ValidateRange(1, 120)]
    [int] $StartupTimeoutSeconds = 15,

    [ValidateRange(1, 300)]
    [int] $ClientTimeoutSeconds = 30,

    [ValidateRange(1, 120)]
    [int] $ShutdownTimeoutSeconds = 10
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

public static class PmonSmokeNativeMethods
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool WaitNamedPipe(string name, int timeoutMilliseconds);
}

public sealed class PmonSmokeJob : IDisposable
{
    private const uint KillOnJobClose = 0x00002000;
    private const int ExtendedLimitInformation = 9;
    private readonly SafeFileHandle handle;

    public PmonSmokeJob()
    {
        handle = CreateJobObject(IntPtr.Zero, null);
        if (handle.IsInvalid) {
            throw new Win32Exception(Marshal.GetLastWin32Error(), "CreateJobObject failed");
        }

        var information = new JobObjectExtendedLimitInformation();
        information.BasicLimitInformation.LimitFlags = KillOnJobClose;
        if (!SetInformationJobObject(
            handle,
            ExtendedLimitInformation,
            ref information,
            (uint) Marshal.SizeOf(typeof(JobObjectExtendedLimitInformation)))) {
            var error = new Win32Exception(
                Marshal.GetLastWin32Error(),
                "SetInformationJobObject failed");
            handle.Dispose();
            throw error;
        }
    }

    public void Attach(Process process)
    {
        if (!AssignProcessToJobObject(handle, process.Handle)) {
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "AssignProcessToJobObject failed");
        }
    }

    public void Dispose()
    {
        handle.Dispose();
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct IoCounters
    {
        public ulong ReadOperationCount;
        public ulong WriteOperationCount;
        public ulong OtherOperationCount;
        public ulong ReadTransferCount;
        public ulong WriteTransferCount;
        public ulong OtherTransferCount;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct JobObjectBasicLimitInformation
    {
        public long PerProcessUserTimeLimit;
        public long PerJobUserTimeLimit;
        public uint LimitFlags;
        public UIntPtr MinimumWorkingSetSize;
        public UIntPtr MaximumWorkingSetSize;
        public uint ActiveProcessLimit;
        public UIntPtr Affinity;
        public uint PriorityClass;
        public uint SchedulingClass;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct JobObjectExtendedLimitInformation
    {
        public JobObjectBasicLimitInformation BasicLimitInformation;
        public IoCounters IoInfo;
        public UIntPtr ProcessMemoryLimit;
        public UIntPtr JobMemoryLimit;
        public UIntPtr PeakProcessMemoryUsed;
        public UIntPtr PeakJobMemoryUsed;
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern SafeFileHandle CreateJobObject(
        IntPtr jobAttributes,
        string name);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetInformationJobObject(
        SafeFileHandle job,
        int informationClass,
        ref JobObjectExtendedLimitInformation information,
        uint informationLength);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool AssignProcessToJobObject(
        SafeFileHandle job,
        IntPtr process);
}
'@

function Resolve-RequiredFile {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,

        [Parameter(Mandatory = $true)]
        [string] $Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function ConvertTo-WindowsCommandLineArgument {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyString()]
        [string] $Value
    )

    if ($Value.Length -gt 0 -and $Value -notmatch '[\s"]') {
        return $Value
    }

    $quoted = [System.Text.StringBuilder]::new()
    [void] $quoted.Append('"')
    $backslashCount = 0
    foreach ($character in $Value.ToCharArray()) {
        if ($character -eq '\') {
            ++$backslashCount
            continue
        }

        if ($character -eq '"') {
            [void] $quoted.Append(('\' * (($backslashCount * 2) + 1)))
            [void] $quoted.Append('"')
        }
        else {
            [void] $quoted.Append(('\' * $backslashCount))
            [void] $quoted.Append($character)
        }
        $backslashCount = 0
    }
    [void] $quoted.Append(('\' * ($backslashCount * 2)))
    [void] $quoted.Append('"')
    return $quoted.ToString()
}

function Start-RedirectedProcess {
    param(
        [Parameter(Mandatory = $true)]
        [string] $FilePath,

        [Parameter(Mandatory = $true)]
        [string[]] $ArgumentList,

        [Parameter(Mandatory = $true)]
        [string] $WorkingDirectory,

        [Parameter(Mandatory = $true)]
        [PmonSmokeJob] $Job
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.Arguments = (($ArgumentList | ForEach-Object {
        ConvertTo-WindowsCommandLineArgument $_
    }) -join ' ')
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    if (-not $process.Start()) {
        throw "Failed to start process: $FilePath"
    }
    try {
        $Job.Attach($process)
    }
    catch {
        $attachFailure = $_
        try {
            $process.Kill()
            $process.WaitForExit()
        }
        catch {
            # Preserve the job-assignment failure.
        }
        $process.Dispose()
        throw $attachFailure
    }
    return $process
}

function Stop-OwnedProcess {
    param(
        [System.Diagnostics.Process] $Process
    )

    if ($null -eq $Process) {
        return
    }

    try {
        if (-not $Process.HasExited) {
            $Process.Kill()
        }
    }
    catch [System.InvalidOperationException] {
        # The process exited between the state check and Kill().
    }
    catch [System.ComponentModel.Win32Exception] {
        $hasExited = $false
        try {
            $hasExited = $Process.HasExited
        }
        catch [System.InvalidOperationException] {
            $hasExited = $true
        }
        if (-not $hasExited) {
            throw
        }
    }

    try {
        $Process.WaitForExit()
    }
    catch [System.InvalidOperationException] {
        # No wait is needed if the process already released its handle.
    }
}

function Read-ServiceResponse {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process] $Process,

        [Parameter(Mandatory = $true)]
        [string] $ExpectedPayload,

        [Parameter(Mandatory = $true)]
        [int] $TimeoutSeconds,

        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]] $CapturedLines,

        [Parameter(Mandatory = $true)]
        [ref] $PendingRead
    )

    $expectedLine = "%%{$ExpectedPayload}%%"
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        $remainingMilliseconds = [int] [Math]::Max(
            1,
            ($deadline - [DateTime]::UtcNow).TotalMilliseconds
        )
        $PendingRead.Value = $Process.StandardOutput.ReadLineAsync()
        if (-not $PendingRead.Value.Wait($remainingMilliseconds)) {
            throw "Timed out waiting for the PresentMonService $ExpectedPayload response."
        }

        $line = $PendingRead.Value.Result
        $PendingRead.Value = $null
        if ($null -eq $line) {
            throw "PresentMonService closed stdout before the $ExpectedPayload response."
        }
        $CapturedLines.Add($line)
        if ($line -eq $expectedLine) {
            return
        }
    }

    throw "Timed out waiting for the PresentMonService $ExpectedPayload response."
}

function Wait-ForServicePipe {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process] $Process,

        [Parameter(Mandatory = $true)]
        [string] $PipeName,

        [Parameter(Mandatory = $true)]
        [int] $TimeoutSeconds
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while (-not [PmonSmokeNativeMethods]::WaitNamedPipe($PipeName, 0)) {
        if ($Process.HasExited) {
            throw "PresentMonService exited before creating its control pipe."
        }
        if ([DateTime]::UtcNow -ge $deadline) {
            throw "Timed out waiting for the PresentMonService control pipe."
        }
        Start-Sleep -Milliseconds 50
    }
}

function Read-CompletedTask {
    param(
        $Task
    )

    if ($null -eq $Task) {
        return ''
    }
    $Task.Wait()
    return [string] $Task.Result
}

$servicePath = Resolve-RequiredFile $ServiceExecutable 'PresentMonService executable'
$clientPath = Resolve-RequiredFile $SampleClientExecutable 'SampleClient executable'
$middlewarePath = Resolve-RequiredFile $MiddlewareDll 'PresentMonAPI2 middleware DLL'

New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
$resolvedLogDirectory = (Resolve-Path -LiteralPath $LogDirectory).Path
$runId = [Guid]::NewGuid().ToString('N')
$runDirectory = Join-Path $resolvedLogDirectory $runId
New-Item -ItemType Directory -Path $runDirectory | Out-Null

$controlPipe = "\\.\pipe\pmon-cmake-p3-$runId"
$sharedMemoryPrefix = "Local\pmon-cmake-p3-$runId"
$etwSessionName = "pmon-cmake-p3-$runId"

$serviceStdoutPath = Join-Path $runDirectory 'service.stdout.log'
$serviceStderrPath = Join-Path $runDirectory 'service.stderr.log'
$clientStdoutPath = Join-Path $runDirectory 'client.stdout.log'
$clientStderrPath = Join-Path $runDirectory 'client.stderr.log'

$serviceArguments = @(
    '--control-pipe', $controlPipe,
    '--shm-name-prefix', $sharedMemoryPrefix,
    '--etw-session-name', $etwSessionName,
    '--enable-test-control',
    '--enable-stdio-log',
    '--disable-colorized-stdio-log',
    '--disable-ipc-log',
    '--log-level', 'info'
)
$clientArguments = @(
    '--mode', 'Introspection',
    '--control-pipe', $controlPipe,
    '--middleware-dll-path', $middlewarePath,
    '--log-folder', $runDirectory,
    '--log-name-pid',
    '--log-level', 'info'
)

$processJob = [PmonSmokeJob]::new()
$serviceProcess = $null
$clientProcess = $null
$serviceStderrTask = $null
$clientStdoutTask = $null
$clientStderrTask = $null
$pendingServiceRead = $null
$serviceStdoutLines = [System.Collections.Generic.List[string]]::new()
$serviceReady = $false
$quitAttempted = $false
$clientExitCode = $null
$serviceExitCode = $null
$clientStdout = ''
$clientStderr = ''
$serviceStderr = ''
$failureMessage = $null

try {
    $serviceProcess = Start-RedirectedProcess `
        -FilePath $servicePath `
        -ArgumentList $serviceArguments `
        -WorkingDirectory $runDirectory `
        -Job $processJob
    $serviceStderrTask = $serviceProcess.StandardError.ReadToEndAsync()

    $serviceProcess.StandardInput.WriteLine('%ping')
    $serviceProcess.StandardInput.Flush()
    Read-ServiceResponse `
        -Process $serviceProcess `
        -ExpectedPayload 'ping-ok' `
        -TimeoutSeconds $StartupTimeoutSeconds `
        -CapturedLines $serviceStdoutLines `
        -PendingRead ([ref] $pendingServiceRead)
    Wait-ForServicePipe `
        -Process $serviceProcess `
        -PipeName "$controlPipe-in" `
        -TimeoutSeconds $StartupTimeoutSeconds
    $serviceReady = $true

    $clientProcess = Start-RedirectedProcess `
        -FilePath $clientPath `
        -ArgumentList $clientArguments `
        -WorkingDirectory $runDirectory `
        -Job $processJob
    $clientStdoutTask = $clientProcess.StandardOutput.ReadToEndAsync()
    $clientStderrTask = $clientProcess.StandardError.ReadToEndAsync()

    if (-not $clientProcess.WaitForExit($ClientTimeoutSeconds * 1000)) {
        throw 'Timed out waiting for SampleClient introspection.'
    }
    $clientProcess.WaitForExit()
    $clientExitCode = $clientProcess.ExitCode
    $clientStdout = Read-CompletedTask $clientStdoutTask
    $clientStderr = Read-CompletedTask $clientStderrTask

    if ($serviceProcess.HasExited) {
        throw 'PresentMonService exited before SampleClient completed.'
    }

    $quitAttempted = $true
    $serviceProcess.StandardInput.WriteLine('%quit')
    $serviceProcess.StandardInput.Flush()
    Read-ServiceResponse `
        -Process $serviceProcess `
        -ExpectedPayload 'quit-ok' `
        -TimeoutSeconds $ShutdownTimeoutSeconds `
        -CapturedLines $serviceStdoutLines `
        -PendingRead ([ref] $pendingServiceRead)
    if (-not $serviceProcess.WaitForExit($ShutdownTimeoutSeconds * 1000)) {
        throw 'Timed out waiting for PresentMonService to exit after quit.'
    }
    $serviceProcess.WaitForExit()
    $serviceExitCode = $serviceProcess.ExitCode
}
catch {
    $failureMessage = $_.Exception.Message
}
finally {
    try {
        Stop-OwnedProcess $clientProcess
    }
    catch {
        if (-not $failureMessage) {
            $failureMessage = "Failed to stop SampleClient: $($_.Exception.Message)"
        }
    }

    try {
        if ($null -ne $serviceProcess -and -not $serviceProcess.HasExited -and
            $serviceReady -and -not $quitAttempted) {
            $quitAttempted = $true
            $serviceProcess.StandardInput.WriteLine('%quit')
            $serviceProcess.StandardInput.Flush()
            [void] $serviceProcess.WaitForExit($ShutdownTimeoutSeconds * 1000)
        }
    }
    catch {
        if (-not $failureMessage) {
            $failureMessage = "Failed to request PresentMonService shutdown: $($_.Exception.Message)"
        }
    }

    try {
        Stop-OwnedProcess $serviceProcess
    }
    catch {
        if (-not $failureMessage) {
            $failureMessage = "Failed to stop PresentMonService: $($_.Exception.Message)"
        }
    }

    try {
        $processJob.Dispose()
    }
    catch {
        if (-not $failureMessage) {
            $failureMessage = "Failed to close the smoke-test process job: $($_.Exception.Message)"
        }
    }

    if ($null -ne $pendingServiceRead) {
        try {
            $pendingServiceRead.Wait()
            if ($null -ne $pendingServiceRead.Result) {
                $serviceStdoutLines.Add([string] $pendingServiceRead.Result)
            }
        }
        catch {
            # Preserve the original failure and continue collecting diagnostics.
        }
        $pendingServiceRead = $null
    }

    if ($null -ne $serviceProcess) {
        try {
            $remainingServiceStdout = $serviceProcess.StandardOutput.ReadToEnd()
            if ($remainingServiceStdout) {
                $serviceStdoutLines.Add($remainingServiceStdout.TrimEnd("`r", "`n"))
            }
        }
        catch {
            # Preserve the original failure and continue collecting diagnostics.
        }
    }

    try {
        $serviceStderr = Read-CompletedTask $serviceStderrTask
    }
    catch {
        if (-not $failureMessage) {
            $failureMessage = "Failed to read PresentMonService diagnostics: $($_.Exception.Message)"
        }
    }
    try {
        if (-not $clientStdout) {
            $clientStdout = Read-CompletedTask $clientStdoutTask
        }
        if (-not $clientStderr) {
            $clientStderr = Read-CompletedTask $clientStderrTask
        }
    }
    catch {
        if (-not $failureMessage) {
            $failureMessage = "Failed to read SampleClient diagnostics: $($_.Exception.Message)"
        }
    }

    $serviceStdout = $serviceStdoutLines -join [Environment]::NewLine
    Set-Content -LiteralPath $serviceStdoutPath -Value $serviceStdout -Encoding UTF8
    Set-Content -LiteralPath $serviceStderrPath -Value $serviceStderr -Encoding UTF8
    Set-Content -LiteralPath $clientStdoutPath -Value $clientStdout -Encoding UTF8
    Set-Content -LiteralPath $clientStderrPath -Value $clientStderr -Encoding UTF8
}

if (-not $failureMessage -and $clientExitCode -ne 0) {
    $failureMessage = "SampleClient exited with code $clientExitCode."
}
if (-not $failureMessage -and $serviceExitCode -ne 0) {
    $failureMessage = "PresentMonService exited with code $serviceExitCode."
}
if (-not $failureMessage -and
    $clientStdout -notmatch '(?m)^Metric Id: [0-9]+, Metric Symbol: PM_METRIC_') {
    $failureMessage = 'SampleClient returned no metric introspection data.'
}

if ($failureMessage) {
    Write-Host "Phase 3 service/SDK smoke test failed: $failureMessage"
    Write-Host "Service stdout: $serviceStdoutPath"
    Write-Host "Service stderr: $serviceStderrPath"
    Write-Host "Client stdout: $clientStdoutPath"
    Write-Host "Client stderr: $clientStderrPath"
    throw $failureMessage
}

Write-Host 'Phase 3 service/SDK smoke test passed.'
Write-Host "Logs: $runDirectory"
