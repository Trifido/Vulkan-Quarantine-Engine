<#
.SYNOPSIS
  Configura y compila QuarantineEngine usando CMake + Visual Studio 2022,
  generando la carpeta build/ en la raíz del repositorio.

.PARAMETER Configuration
  La configuración a compilar: Debug, Release o Both. Por defecto "Both".

.EXAMPLE
  # Desde la carpeta setup:
  .\build.ps1             # compila Debug y Release
  .\build.ps1 -Configuration Debug   # solo Debug
  .\build.ps1 -Configuration Release # solo Release
#>

param(
    [ValidateSet("Debug","Release","Both")]
    [string]$Configuration = "Both"
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
git submodule update --init --recursive
Pop-Location

# --- Localizar glslc.exe
function Find-Glslc {
    # Primero: usar VULKAN_SDK si existe
    if ($env:VULKAN_SDK) {
        $candidate = Join-Path $env:VULKAN_SDK "Bin\glslc.exe"
        if (Test-Path $candidate) { return $candidate }
    }

    # Segundo: buscar bajo C:\VulkanSDK\ la versión más reciente
    $base = "C:\VulkanSDK"
    if (Test-Path $base) {
        try {
            $dirs = Get-ChildItem -Path $base -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending
            foreach ($d in $dirs) {
                $candidate = Join-Path $d.FullName "Bin\glslc.exe"
                if (Test-Path $candidate) { return $candidate }
            }
        } catch {
            # ignore
        }
    }

    return $null
}

$glslcPath = Find-Glslc
if ($null -eq $glslcPath) {
    Write-Warning "No se encontró glslc.exe. La compilación de shaders será saltada. Asegura VULKAN_SDK esté configurado o instala Vulkan SDK."
} else {
    Write-Host "Using glslc: $glslcPath" -ForegroundColor Green
}

# --- Compilar shaders a SPIR-V (si glslc disponible)
if ($glslcPath) {
    # Intentar localizar la carpeta shaders en varias ubicaciones posibles:
    $possible1 = Join-Path $projectRoot "Vulkan-Quarantine-Engine\resources\shaders"
    $possible2 = Join-Path $projectRoot "resources\shaders"

    if (Test-Path $possible1) {
        $shadersDir = $possible1
    } elseif (Test-Path $possible2) {
        $shadersDir = $possible2
    } else {
        # buscar primer directorio llamado "shaders"
        $found = Get-ChildItem -Path $projectRoot -Directory -Recurse -Filter "shaders" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) { $shadersDir = $found.FullName } else { $shadersDir = $null }
    }

    if (-not $shadersDir) {
        Write-Warning "No se encontró carpeta 'shaders' bajo el proyecto. Saltando compilación de shaders."
    } else {
        Write-Host "Compiling shaders in: $shadersDir" -ForegroundColor Cyan

        # Extensiones soportadas (case-insensitive)
        $supported = @(".vert",".vs",".frag",".fs",".comp",".task",".mesh")

        # Buscar cualquier archivo con esas extensiones recursivamente
        $patterns = $supported | ForEach-Object { "*$($_)" }
        $shaderFiles = Get-ChildItem -Path $shadersDir -Recurse -File -Include $patterns -ErrorAction SilentlyContinue

        if (-not $shaderFiles -or $shaderFiles.Count -eq 0) {
            Write-Host "No shader files encontrados en $shadersDir" -ForegroundColor Yellow
        } else {
            foreach ($file in $shaderFiles) {
                $ext = $file.Extension.ToLower()
                if (-not ($supported -contains $ext)) {
                    Write-Host "Tipo no soportado (por seguridad): $($file.FullName). Saltando."
                    continue
                }

                # Sufijo de salida: _<tipo>.spv (ej: mesh.task -> mesh_task.spv)
                $typeName = $ext.TrimStart(".")     # "vert","frag","comp","task","mesh","vs","fs"
                # Mapear vs/fs a vert/frag para consistencia en sufijo
                if ($typeName -eq "vs") { $typeName = "vert" }
                if ($typeName -eq "fs") { $typeName = "frag" }

                $outName = $file.BaseName + "_" + $typeName + ".spv"
                $outPath = Join-Path $file.DirectoryName $outName

                Write-Host "Compiling $($file.FullName) -> $outPath"

                # Construir argumentos para glslc
                $args = @()
                $args += $file.FullName
                $args += "-o"
                $args += $outPath
                if ($VulkanTarget) {
                    $args += "--target-env=$VulkanTarget"
                }

                # Ejecutar glslc
                & "$glslcPath" @args
                $rc = $LASTEXITCODE
                if ($rc -ne 0) {
                    Write-Warning "glslc falló para $($file.FullName) (exit code $rc)."
                } else {
                    Write-Host "OK: $outName" -ForegroundColor Green
                }
            }
        }
    }
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

# 3) CMake build según configuración
# Definimos la lista de configs a compilar
$configs = @()
switch ($Configuration) {
    'Debug'   { $configs = @('Debug'); break }
    'Release' { $configs = @('Release'); break }
    default   { $configs = @('Debug','Release'); break }
}

foreach ($cfg in $configs) {
    Write-Host "Building QuarantineEngine ($cfg)..." -ForegroundColor Cyan
    cmake --build $buildDir `
          --config $cfg `
          --parallel $cores

    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed for configuration $cfg (exit code $LASTEXITCODE)."
        exit $LASTEXITCODE
    }
}

Write-Host "Build succeeded!" -ForegroundColor Green
