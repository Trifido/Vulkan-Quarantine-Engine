<#
.SYNOPSIS
  Configura y compila QuarantineEngine usando CMake + Visual Studio 2022,
  generando la carpeta build/ en la raíz del repositorio.

.PARAMETER Configuration
  La configuración a compilar: Debug o Release. Por defecto "Debug".

.EXAMPLE
  # Desde la carpeta setup:
  .\build.ps1
  .\build.ps1 -Configuration Release
#>

param(
    [ValidateSet("Debug","Release")]
    [string]$Configuration = "Debug"
)

# Núcleos para paralelizar
$cores = [environment]::ProcessorCount

# Ruta al directorio del script (setup/)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Proyecto raíz (un nivel arriba de setup/)
$projectRoot = Join-Path $scriptDir ".." | Resolve-Path -Path

# Build dir en la raíz
$buildDir = Join-Path $projectRoot "build"

Write-Host "=== QuarantineEngine Build Script ===" -ForegroundColor Cyan
Write-Host "Project root: $projectRoot"
Write-Host "Build directory: $buildDir"
Write-Host "Configuration: $Configuration"
Write-Host "Parallel cores: $cores"
Write-Host "-------------------------------------"

# 1) Crear carpeta build/ si no existe
if (!(Test-Path $buildDir)) {
    Write-Host "Creating build directory at root..."
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# 2) CMake configure
Write-Host "Running CMake configure..."
cmake -S $projectRoot -B $buildDir `
      -G "Visual Studio 17 2022" `
      -A x64 `
      -DCMAKE_CONFIGURATION_TYPES="Debug;Release" `
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed (exit code $LASTEXITCODE)."
    exit $LASTEXITCODE
}

# 3) CMake build
Write-Host "Building QuarantineEngine ($Configuration)..."
cmake --build $buildDir `
      --config $Configuration `
      --parallel $cores

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed (exit code $LASTEXITCODE)."
    exit $LASTEXITCODE
}

Write-Host "Build succeeded!" -ForegroundColor Green
