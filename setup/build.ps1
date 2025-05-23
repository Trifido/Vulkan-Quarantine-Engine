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

# 0) Actualizar submódulos Git
Write-Host "Updating Git submodules..." -ForegroundColor Cyan
Push-Location $projectRoot
git submodule update --init --recursive
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to update submodules (exit code $LASTEXITCODE)."
    exit $LASTEXITCODE
}
Pop-Location

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
