<#
.SYNOPSIS
  Empaqueta QuarantineLauncher como distribucion portable para Windows.

.DESCRIPTION
  Publica el launcher con dotnet publish, conserva la configuracion inicial
  y genera un layout estable listo para distribuir como zip o como base de
  un instalador.
#>

param(
    [string]$Version = "0.1.0-dev",
    [string]$Runtime = "win-x64",
    [string]$Configuration = "Release",
    [string]$OutputRoot = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = (Resolve-Path (Join-Path $scriptDir "..")).Path
$launcherProject = Join-Path $projectRoot "src\QuarantineLauncher\QuarantineLauncher.csproj"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $projectRoot "artifacts\launcher"
}

$packageRoot = Join-Path $OutputRoot "$Version\$Runtime"
$publishDir = Join-Path $packageRoot "app"
$archivePath = Join-Path $packageRoot "quarantine-launcher-$Version-$Runtime.zip"
$manifestPath = Join-Path $packageRoot "launcher-manifest.json"

function Ensure-CleanDirectory {
    param([string]$PathToCreate)

    if (Test-Path -LiteralPath $PathToCreate) {
        Remove-Item -LiteralPath $PathToCreate -Recurse -Force
    }

    New-Item -ItemType Directory -Path $PathToCreate -Force | Out-Null
}

Write-Host "Packaging QuarantineLauncher $Version for $Runtime" -ForegroundColor Cyan
Write-Host "Project root: $projectRoot"
Write-Host "Package root: $packageRoot"

Ensure-CleanDirectory -PathToCreate $packageRoot

dotnet publish $launcherProject `
    -c $Configuration `
    -r $Runtime `
    --self-contained true `
    -p:PublishSingleFile=false `
    -p:PublishReadyToRun=false `
    -o $publishDir

if ($LASTEXITCODE -ne 0) {
    throw "dotnet publish failed with exit code $LASTEXITCODE."
}

$manifest = [ordered]@{
    id = "quarantine-launcher"
    version = $Version
    runtime = $Runtime
    generatedAtUtc = [DateTime]::UtcNow.ToString("o")
    entryPoint = "Quarantine Engine.exe"
    feedIndex = "https://trifido.github.io/Vulkan-Quarantine-Engine/feed/quarantine-engine/index.json"
}

$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $manifestPath -Encoding utf8

if (Test-Path -LiteralPath $archivePath) {
    Remove-Item -LiteralPath $archivePath -Force
}

Compress-Archive -Path (Join-Path $publishDir '*') -DestinationPath $archivePath -CompressionLevel Optimal

Write-Host "Launcher package created successfully at $packageRoot" -ForegroundColor Green
