<#
.SYNOPSIS
    Measures crosshair.exe: file size, startup time, idle CPU, RAM and GPU.

.DESCRIPTION
    Each run copies the executable and the repository's default config.toml to a
    temporary folder, starts the process, and records:
      - startup: milliseconds from process creation until its window is visible
        (includes process creation from PowerShell, so read it as an upper bound),
        and the CPU time the process used to get there;
      - RAM after a short settle time: private working set (what Task Manager
        shows as "Memory"), total working set and private commit;
      - idle CPU: CPU time used during -IdleSeconds of doing nothing;
      - GPU: summed "GPU Engine" utilization for the process, if Windows exposes it.
    It prints the median of -Runs runs. Close other heavy programs first and use
    the same machine state when comparing two builds.

.EXAMPLE
    .\scripts\measure.ps1 -Exe build\Release\crosshair.exe
#>
param(
    [string]$Exe = "build\Release\crosshair.exe",
    [int]$Runs = 5,
    [int]$IdleSeconds = 10
)

$ErrorActionPreference = 'Stop'

$exePath = (Resolve-Path $Exe).Path
$config = Join-Path $PSScriptRoot '..\config.toml'
$work = Join-Path $env:TEMP 'crosshair-measure'
[void][IO.Directory]::CreateDirectory($work)
[IO.File]::Copy($exePath, "$work\crosshair.exe", $true)
[IO.File]::Copy($config, "$work\config.toml", $true)

function Get-Median($values) {
    $sorted = @($values | Sort-Object)
    $sorted[[int][math]::Floor(($sorted.Count - 1) / 2)]
}

function Measure-Run {
    $psi = [Diagnostics.ProcessStartInfo]::new("$work\crosshair.exe")
    $psi.WorkingDirectory = $work

    $clock = [Diagnostics.Stopwatch]::StartNew()
    $proc = [Diagnostics.Process]::Start($psi)
    try {
        while ($true) {
            $proc.Refresh()
            if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { break }
            if ($proc.HasExited -or $clock.ElapsedMilliseconds -gt 5000) {
                throw "crosshair.exe did not show a window"
            }
            Start-Sleep -Milliseconds 5
        }
        $startupMs = $clock.Elapsed.TotalMilliseconds
        $proc.Refresh()
        $startupCpuMs = $proc.TotalProcessorTime.TotalMilliseconds

        Start-Sleep -Seconds 3 # let the process settle

        $perf = Get-CimInstance Win32_PerfFormattedData_PerfProc_Process -Filter "IDProcess=$($proc.Id)"
        $proc.Refresh()
        $workingSet = $proc.WorkingSet64
        $commit = $proc.PrivateMemorySize64

        $cpuBefore = $proc.TotalProcessorTime.TotalMilliseconds
        Start-Sleep -Seconds $IdleSeconds
        $proc.Refresh()
        $idleCpuMs = $proc.TotalProcessorTime.TotalMilliseconds - $cpuBefore

        $gpu = $null
        try {
            $samples = Get-Counter -Counter "\GPU Engine(pid_$($proc.Id)_*)\Utilization Percentage" `
                -SampleInterval 1 -MaxSamples 3 -ErrorAction Stop
            $gpu = ($samples.CounterSamples | Measure-Object CookedValue -Sum).Sum / 3
        } catch { }

        [pscustomobject]@{
            StartupMs    = $startupMs
            StartupCpuMs = $startupCpuMs
            PrivateWsKB  = $perf.WorkingSetPrivate / 1KB
            WorkingSetKB = $workingSet / 1KB
            CommitKB     = $commit / 1KB
            IdleCpuMs    = $idleCpuMs
            GpuPercent   = $gpu
        }
    } finally {
        if (-not $proc.HasExited) { $proc.Kill() }
        $proc.WaitForExit()
    }
}

$results = 1..$Runs | ForEach-Object { Measure-Run }
[IO.Directory]::Delete($work, $true)

$gpuValues = @($results.GpuPercent | Where-Object { $null -ne $_ })

[pscustomobject]@{
    Exe                   = $Exe
    'Size (bytes)'        = (Get-Item $exePath).Length
    Runs                  = $Runs
    'Startup (ms)'        = [math]::Round((Get-Median $results.StartupMs))
    'Startup CPU (ms)'    = [math]::Round((Get-Median $results.StartupCpuMs))
    'Private WS (KB)'     = [math]::Round((Get-Median $results.PrivateWsKB))
    'Working set (KB)'    = [math]::Round((Get-Median $results.WorkingSetKB))
    'Commit (KB)'         = [math]::Round((Get-Median $results.CommitKB))
    "Idle CPU in ${IdleSeconds}s (ms)" = [math]::Round((Get-Median $results.IdleCpuMs))
    'GPU (%)'             = if ($gpuValues.Count) { [math]::Round((Get-Median $gpuValues), 2) } else { 'n/a' }
} | Format-List
