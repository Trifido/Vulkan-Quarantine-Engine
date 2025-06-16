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

# Número de núcleos para paralelizar
$cores = [Environment]::ProcessorCount

# Ruta al directorio donde está este script (setup/)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Proyecto raíz (un nivel arriba de setup/)
$projectRoot = (Resolve-Path (Join-Path $scriptDir "..")).Path

# Build dir en la raíz del proyecto
$buildDir = Join-Path $projectRoot "build"

Write-Host "=== QuarantineEngine Build Script ===" -ForegroundColor Cyan
Write-Host "Project root: $projectRoot"
Write-Host "Build directory: $buildDir"
Write-Host "Configuration: $Configuration"
Write-Host "Parallel cores: $cores"
Write-Host "-------------------------------------"

# 0) Limpieza (opcional) de la carpeta build/
Write-Host "Cleaning existing build directory..." -ForegroundColor Cyan
if (Test-Path $buildDir) {
    try {
        Remove-Item -Recurse -Force $buildDir
        Write-Host "Removed existing build directory." -ForegroundColor Green
    } catch {
        Write-Warning "Failed to remove build directory: $_"
    }
}

# 1) Actualizar submódulos Git
Write-Host "Updating Git submodules..." -ForegroundColor Cyan
Push-Location $projectRoot
git s

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
foreach ($cfg in @("Debug","Release")) {
    Write-Host "Building QuarantineEngine ($cfg)..." -ForegroundColor Cyan
    cmake --build $buildDir `
          --config $cfg `
          --parallel $cores

    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed for configuration $cfg (exit code $LASTEXITCODE)."
        exit $LASTEXITCODE
    }
}

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed (exit code $LASTEXITCODE)."
    exit $LASTEXITCODE
}

Write-Host "Build succeeded!" -ForegroundColor Green
