[CmdletBinding()]
param(
    [ValidateRange(1, 1000)]
    [int]$Cycles = 100,

    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [string]$ProjectPath = "",

    [ValidateRange(8, 10000)]
    [int]$FramesPerCycle = 8
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$editorExecutable = Join-Path $projectRoot "build\$Configuration\QuarantineEditor.exe"

if ([string]::IsNullOrWhiteSpace($ProjectPath))
{
    $ProjectPath = Join-Path $projectRoot "QEProjects\QEExample"
}

$editorExecutable = [System.IO.Path]::GetFullPath($editorExecutable)
$ProjectPath = [System.IO.Path]::GetFullPath($ProjectPath)
$quotedProjectPath = '"' + $ProjectPath.Replace('"', '\"') + '"'

if (-not (Test-Path -LiteralPath $editorExecutable -PathType Leaf))
{
    throw "QuarantineEditor executable not found: $editorExecutable"
}

if (-not (Test-Path -LiteralPath $ProjectPath -PathType Container))
{
    throw "Editor project not found: $ProjectPath"
}

$localAppData = [Environment]::GetFolderPath([Environment+SpecialFolder]::LocalApplicationData)
$logRoot = Join-Path $localAppData "QuarantineEngine\Logs\QuarantineEditor"
$failures = [System.Collections.Generic.List[string]]::new()

Write-Host "Running $Cycles editor stability cycles ($Configuration, $FramesPerCycle frames each)."
Write-Host "Editor: $editorExecutable"
Write-Host "Project: $ProjectPath"

$previousSmokeFrames = [Environment]::GetEnvironmentVariable("QE_STABILITY_SMOKE_FRAMES", "Process")

try
{
    [Environment]::SetEnvironmentVariable(
        "QE_STABILITY_SMOKE_FRAMES",
        $FramesPerCycle.ToString([Globalization.CultureInfo]::InvariantCulture),
        "Process")

    for ($cycle = 1; $cycle -le $Cycles; ++$cycle)
    {
        $startedAt = Get-Date
        $process = Start-Process `
            -FilePath $editorExecutable `
            -ArgumentList @($quotedProjectPath) `
            -WorkingDirectory (Split-Path -Parent $editorExecutable) `
            -WindowStyle Hidden `
            -Wait `
            -PassThru

        if ($process.ExitCode -ne 0)
        {
            $failures.Add("Cycle ${cycle}: editor exited with code $($process.ExitCode).")
            continue
        }

        $logFile = Get-ChildItem -LiteralPath $logRoot -Filter "*.log" -File |
            Where-Object { $_.LastWriteTime -ge $startedAt.AddSeconds(-1) } |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1

        if ($null -eq $logFile)
        {
            $failures.Add("Cycle ${cycle}: no log file was produced.")
            continue
        }

        $logContent = Get-Content -LiteralPath $logFile.FullName -Raw
        $failurePatterns = @(
            "\[Error\]",
            "SYNC-HAZARD",
            "Live VkBuffer",
            "VUID-vkDestroyDevice-device-05137"
        )

        foreach ($pattern in $failurePatterns)
        {
            if ($logContent -match $pattern)
            {
                $failures.Add("Cycle ${cycle}: log contains '$pattern' ($($logFile.Name)).")
            }
        }

        $resizeCount = ([regex]::Matches($logContent, "Swapchain recreated at")).Count
        if ($resizeCount -lt 2)
        {
            $failures.Add("Cycle ${cycle}: expected at least 2 swapchain recreations, found $resizeCount.")
        }

        if ($logContent -notmatch "Automated editor smoke test completed")
        {
            $failures.Add("Cycle ${cycle}: smoke-test completion marker is missing.")
        }

        if (($cycle % 10) -eq 0 -or $cycle -eq $Cycles)
        {
            Write-Host "Completed $cycle/$Cycles cycles."
        }
    }
}
finally
{
    [Environment]::SetEnvironmentVariable(
        "QE_STABILITY_SMOKE_FRAMES",
        $previousSmokeFrames,
        "Process")
}

if ($failures.Count -gt 0)
{
    $details = $failures -join [Environment]::NewLine
    throw "Editor stability validation failed:`n$details"
}

Write-Host "Editor stability validation passed: $Cycles/$Cycles clean cycles."
