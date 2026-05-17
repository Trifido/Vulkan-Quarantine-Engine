<#
.SYNOPSIS
  Crea un paquete autocontenido de QuarantineEngine para Windows a partir de un build existente.

.DESCRIPTION
  Empaqueta headers/fuentes necesarios para consumo SDK, dependencias de terceros,
  bibliotecas import/lib, DLLs runtime, recursos y metadata en un layout estable
  pensado para futuras descargas desde QuarantineLauncher.
#>

param(
    [string]$Version = "0.1.0-dev",
    [ValidateSet("Debug", "Release", "Both")]
    [string]$Configuration = "Release",
    [string]$Platform = "win-x64",
    [string]$OutputRoot = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = (Resolve-Path (Join-Path $scriptDir "..")).Path
$buildRoot = Join-Path $projectRoot "build"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $projectRoot "artifacts\packages"
}

$packageRoot = Join-Path $OutputRoot "quarantine-engine\$Version\$Platform"

function Ensure-CleanDirectory {
    param([string]$PathToCreate)

    if (Test-Path -LiteralPath $PathToCreate) {
        $resolved = (Resolve-Path $PathToCreate).Path
        if ($resolved -notlike (Join-Path $projectRoot "artifacts\packages\*")) {
            throw "Refusing to delete outside artifacts/packages: $resolved"
        }

        Remove-Item -LiteralPath $resolved -Recurse -Force
    }

    New-Item -ItemType Directory -Path $PathToCreate -Force | Out-Null
}

function Copy-DirectoryContents {
    param(
        [string]$Source,
        [string]$Destination
    )

    if (-not (Test-Path -LiteralPath $Source)) {
        throw "Missing source directory: $Source"
    }

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    Copy-Item -Path (Join-Path $Source "*") -Destination $Destination -Recurse -Force
}

function Copy-FileIfExists {
    param(
        [string]$Source,
        [string]$DestinationDirectory
    )

    if (Test-Path -LiteralPath $Source) {
        New-Item -ItemType Directory -Path $DestinationDirectory -Force | Out-Null
        Copy-Item -LiteralPath $Source -Destination $DestinationDirectory -Force
        return $true
    }

    return $false
}

function Require-Files {
    param(
        [string]$Label,
        [string[]]$Paths
    )

    $missing = @($Paths | Where-Object { -not (Test-Path -LiteralPath $_) })
    if ($missing.Count -gt 0) {
        throw "$Label is incomplete. Missing:`n$($missing -join "`n")"
    }
}

function Get-Configurations {
    switch ($Configuration) {
        "Debug" { return @("Debug") }
        "Release" { return @("Release") }
        default { return @("Debug", "Release") }
    }
}

function New-Manifest {
    param(
        [string]$DestinationPath,
        [string[]]$Configurations
    )

    $gitCommit = ""
    try {
        $gitCommit = (git -C $projectRoot rev-parse --short HEAD 2>$null).Trim()
    }
    catch {
        $gitCommit = ""
    }

    $manifest = [ordered]@{
        id = "quarantine-engine"
        version = $Version
        platform = $Platform
        layoutVersion = 1
        generatedAtUtc = [DateTime]::UtcNow.ToString("o")
        sourceCommit = $gitCommit
        configurations = $Configurations
        paths = [ordered]@{
            binaries = "bin"
            libraries = "lib"
            engineSource = "src"
            thirdPartyHeaders = "extern"
            fetchedHeaders = "_deps"
            resources = "resources"
            templates = "templates"
        }
    }

    $json = $manifest | ConvertTo-Json -Depth 5
    Set-Content -LiteralPath $DestinationPath -Value $json -Encoding utf8
}

$configs = Get-Configurations

Write-Host "Packaging QuarantineEngine $Version for $Platform" -ForegroundColor Cyan
Write-Host "Project root: $projectRoot"
Write-Host "Build root: $buildRoot"
Write-Host "Package root: $packageRoot"
Write-Host "Configurations: $($configs -join ', ')"

Ensure-CleanDirectory -PathToCreate $packageRoot

Copy-DirectoryContents -Source (Join-Path $projectRoot "src\QuarantineEngine") -Destination (Join-Path $packageRoot "src\QuarantineEngine")
Copy-DirectoryContents -Source (Join-Path $projectRoot "extern\assimp\include") -Destination (Join-Path $packageRoot "extern\assimp\include")
Copy-DirectoryContents -Source (Join-Path $projectRoot "extern\imgui") -Destination (Join-Path $packageRoot "extern\imgui")
Copy-DirectoryContents -Source (Join-Path $projectRoot "extern\jolt") -Destination (Join-Path $packageRoot "extern\jolt")
Copy-DirectoryContents -Source (Join-Path $projectRoot "extern\meshoptimizer\src") -Destination (Join-Path $packageRoot "extern\meshoptimizer\src")
Copy-DirectoryContents -Source (Join-Path $projectRoot "extern\SPIRV-Reflect") -Destination (Join-Path $packageRoot "extern\SPIRV-Reflect")
Copy-DirectoryContents -Source (Join-Path $buildRoot "_deps\glfw-src\include") -Destination (Join-Path $packageRoot "_deps\glfw-src\include")
Copy-DirectoryContents -Source (Join-Path $buildRoot "_deps\glm-src") -Destination (Join-Path $packageRoot "_deps\glm-src")
Copy-DirectoryContents -Source (Join-Path $buildRoot "_deps\stb-src") -Destination (Join-Path $packageRoot "_deps\stb-src")
Copy-DirectoryContents -Source (Join-Path $buildRoot "_deps\yaml-cpp-src\include") -Destination (Join-Path $packageRoot "_deps\yaml-cpp-src\include")
Copy-DirectoryContents -Source (Join-Path $buildRoot "_deps\ktx-src\include") -Destination (Join-Path $packageRoot "_deps\ktx-src\include")
Copy-DirectoryContents -Source (Join-Path $projectRoot "resources\shaders") -Destination (Join-Path $packageRoot "resources\shaders")
Copy-DirectoryContents -Source (Join-Path $projectRoot "resources\textures") -Destination (Join-Path $packageRoot "resources\textures")
Copy-DirectoryContents -Source (Join-Path $projectRoot "src\QuarantineEngine\Data\SceneTemplates") -Destination (Join-Path $packageRoot "templates\scene")

foreach ($cfg in $configs) {
    $libTargetDir = Join-Path $packageRoot "lib\$cfg"
    $binTargetDir = Join-Path $packageRoot "bin\$cfg"
    New-Item -ItemType Directory -Path $libTargetDir -Force | Out-Null
    New-Item -ItemType Directory -Path $binTargetDir -Force | Out-Null

    $debugSuffix = if ($cfg -eq "Debug") { "d" } else { "" }
    $assimpLibName = if ($cfg -eq "Debug") { "assimp-vc143-mtd.lib" } else { "assimp-vc143-mt.lib" }

    $requiredLibs = @(
        (Join-Path $buildRoot "$cfg\QuarantineEngine$debugSuffix.lib"),
        (Join-Path $buildRoot "$cfg\SPIRV-Reflect$debugSuffix.lib"),
        (Join-Path $buildRoot "$cfg\imgui$debugSuffix.lib"),
        (Join-Path $buildRoot "_deps\glfw-build\src\$cfg\glfw3$debugSuffix.lib"),
        (Join-Path $buildRoot "_deps\yaml-cpp-build\$cfg\yaml-cpp$debugSuffix.lib"),
        (Join-Path $buildRoot "_deps\ktx-build\$cfg\ktx.lib"),
        (Join-Path $buildRoot "extern\jolt\Build\$cfg\Jolt.lib"),
        (Join-Path $buildRoot "extern\meshoptimizer\$cfg\meshoptimizer$debugSuffix.lib"),
        (Join-Path $buildRoot "extern\assimp\lib\$cfg\$assimpLibName")
    )

    Require-Files -Label "Required libraries for $cfg" -Paths $requiredLibs

    foreach ($path in $requiredLibs) {
        Copy-Item -LiteralPath $path -Destination $libTargetDir -Force
    }

    $runtimePatterns = @("*.dll", "*.pdb")
    foreach ($pattern in $runtimePatterns) {
        Get-ChildItem -Path (Join-Path $buildRoot $cfg) -Filter $pattern -File -ErrorAction SilentlyContinue |
            ForEach-Object {
                Copy-Item -LiteralPath $_.FullName -Destination $binTargetDir -Force
            }
    }
}

$metadataDir = Join-Path $packageRoot "metadata"
New-Item -ItemType Directory -Path $metadataDir -Force | Out-Null
New-Manifest -DestinationPath (Join-Path $metadataDir "manifest.json") -Configurations $configs

Write-Host "Package created successfully at $packageRoot" -ForegroundColor Green
