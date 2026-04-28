<#
.SYNOPSIS
  Configura y compila QuarantineEngine usando CMake + Visual Studio 2022,
  generando la carpeta build/ en la raiz del repositorio.

.PARAMETER Configuration
  La configuracion a compilar: Debug, Release o Both. Por defecto "Both".

.EXAMPLE
  # Desde la carpeta setup:
  .\build.ps1             # compila Debug y Release
  .\build.ps1 -Configuration Debug   # solo Debug
  .\build.ps1 -Configuration Release # solo Release
#>

param(
    [ValidateSet("Debug", "Release", "Both")]
    [string]$Configuration = "Both"
)

$ErrorActionPreference = "Stop"

# Numero de nucleos para paralelizar.
$cores = [Environment]::ProcessorCount

# Ruta al directorio donde esta este script (setup/).
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Proyecto raiz (un nivel arriba de setup/).
$projectRoot = (Resolve-Path (Join-Path $scriptDir "..")).Path

# Build dir en la raiz del proyecto.
$buildDir = Join-Path $projectRoot "build"
$launcherProject = Join-Path $projectRoot "src\QuarantineLauncher\QuarantineLauncher.csproj"

function Require-Command {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [string]$Hint
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $command) {
        if ($Hint) {
            throw "No se encontro '$Name'. $Hint"
        }

        throw "No se encontro '$Name'."
    }

    return $command
}

function Find-Glslc {
    if ($env:VULKAN_SDK) {
        $candidate = Join-Path $env:VULKAN_SDK "Bin\glslc.exe"
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    $base = "C:\VulkanSDK"
    if (Test-Path $base) {
        $dirs = Get-ChildItem -Path $base -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending
        foreach ($dir in $dirs) {
            $candidate = Join-Path $dir.FullName "Bin\glslc.exe"
            if (Test-Path $candidate) {
                return $candidate
            }
        }
    }

    return $null
}

function Compile-Shaders {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RootPath,
        [Parameter(Mandatory = $true)]
        [string]$GlslcPath
    )

    $possibleShaderDirs = @(
        (Join-Path $RootPath "Vulkan-Quarantine-Engine\resources\shaders"),
        (Join-Path $RootPath "resources\shaders")
    )

    $shadersDir = $possibleShaderDirs | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $shadersDir) {
        $found = Get-ChildItem -Path $RootPath -Directory -Recurse -Filter "shaders" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            $shadersDir = $found.FullName
        }
    }

    if (-not $shadersDir) {
        Write-Warning "No se encontro carpeta 'shaders' bajo el proyecto. Saltando compilacion de shaders."
        return
    }

    Write-Host "Compiling shaders in: $shadersDir" -ForegroundColor Cyan

    $supported = @(".vert", ".vs", ".frag", ".fs", ".comp", ".task", ".mesh")
    $patterns = $supported | ForEach-Object { "*$_" }
    $shaderFiles = Get-ChildItem -Path $shadersDir -Recurse -File -Include $patterns -ErrorAction SilentlyContinue

    if (-not $shaderFiles -or $shaderFiles.Count -eq 0) {
        Write-Host "No se encontraron shaders en $shadersDir" -ForegroundColor Yellow
        return
    }

    foreach ($file in $shaderFiles) {
        $typeName = $file.Extension.TrimStart(".").ToLower()
        if ($typeName -eq "vs") { $typeName = "vert" }
        if ($typeName -eq "fs") { $typeName = "frag" }

        $outName = $file.BaseName + "_" + $typeName + ".spv"
        $outPath = Join-Path $file.DirectoryName $outName

        Write-Host "Compiling $($file.FullName) -> $outPath"
        $args = @($file.FullName, "-o", $outPath)
        if ($file.Extension -in @(".mesh", ".task")) {
            $args += @("--target-env=vulkan1.3", "--target-spv=spv1.6")
        }

        & $GlslcPath @args

        if ($LASTEXITCODE -ne 0) {
            throw "glslc fallo para $($file.FullName) (exit code $LASTEXITCODE)."
        }
    }
}

Write-Host "=== QuarantineEngine Build Script ===" -ForegroundColor Cyan
Write-Host "Project root: $projectRoot"
Write-Host "Build directory: $buildDir"
Write-Host "Configuration: $Configuration"
Write-Host "Parallel cores: $cores"
Write-Host "-------------------------------------"

Write-Host "Checking required tools..." -ForegroundColor Cyan
Require-Command -Name "git" -Hint "Instala Git y asegurate de que este disponible en PATH." | Out-Null
Require-Command -Name "cmake" -Hint "Instala CMake y asegurate de que este disponible en PATH." | Out-Null
Require-Command -Name "dotnet" -Hint "Instala el SDK de .NET y asegurate de que este disponible en PATH." | Out-Null

Write-Host "Cleaning existing build directory..." -ForegroundColor Cyan
if (Test-Path $buildDir) {
    $resolvedBuildDir = (Resolve-Path $buildDir).Path
    if ($resolvedBuildDir -ne $buildDir) {
        Write-Host "Resolved build directory: $resolvedBuildDir"
    }

    if ($resolvedBuildDir -notlike (Join-Path $projectRoot "build")) {
        throw "La ruta de build resuelta no es segura para eliminar: $resolvedBuildDir"
    }

    Remove-Item -LiteralPath $resolvedBuildDir -Recurse -Force
    Write-Host "Removed existing build directory." -ForegroundColor Green
}

Write-Host "Updating Git submodules..." -ForegroundColor Cyan
Push-Location $projectRoot
try {
    git submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) {
        throw "git submodule update fallo con exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

$glslcPath = Find-Glslc
if ($null -eq $glslcPath) {
    Write-Warning "No se encontro glslc.exe. La compilacion de shaders sera saltada. Asegura que VULKAN_SDK este configurado o instala Vulkan SDK."
}
else {
    Write-Host "Using glslc: $glslcPath" -ForegroundColor Green
    Compile-Shaders -RootPath $projectRoot -GlslcPath $glslcPath
}

Write-Host "Running CMake configure..." -ForegroundColor Cyan
cmake -S $projectRoot -B $buildDir `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DCMAKE_CONFIGURATION_TYPES="Debug;Release" `
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed (exit code $LASTEXITCODE)."
}

$configs = @()
switch ($Configuration) {
    "Debug" { $configs = @("Debug") }
    "Release" { $configs = @("Release") }
    default { $configs = @("Debug", "Release") }
}

foreach ($cfg in $configs) {
    Write-Host "Building QuarantineEngine ($cfg)..." -ForegroundColor Cyan
    cmake --build $buildDir --config $cfg --parallel $cores

    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for configuration $cfg (exit code $LASTEXITCODE)."
    }
}

Write-Host "Building QuarantineLauncher..." -ForegroundColor Cyan
dotnet build $launcherProject

if ($LASTEXITCODE -ne 0) {
    throw "Launcher build failed (exit code $LASTEXITCODE)."
}

Write-Host "Build succeeded!" -ForegroundColor Green
