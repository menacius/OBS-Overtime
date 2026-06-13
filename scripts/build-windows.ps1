<#
.SYNOPSIS
  Configure and build OBS Overtime on Windows.

.EXAMPLE
  pwsh ./scripts/build-windows.ps1 -Clean

.EXAMPLE
  pwsh ./scripts/build-windows.ps1 -ObsSdkDir "C:\Users\menac\Desktop\obs-build-dependencies" -Qt6Path "C:\Qt\6.6.3\msvc2019_64" -Clean

.EXAMPLE
  pwsh ./scripts/build-windows.ps1 -Clean -InstallToObs
#>

[CmdletBinding()]
param(
  [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
  [string]$Configuration = "RelWithDebInfo",

  [string]$BuildDir = "build_x64",

  [string]$Generator = "Visual Studio 17 2022",

  [string]$ObsSdkDir = $env:OBS_SDK_DIR,

  [string]$ObsStudioDir = $env:OBS_STUDIO_DIR,

  [string]$Qt6Path = $env:Qt6_DIR,

  [switch]$Clean,

  [switch]$InstallToObs,

  [string]$ObsInstallDir = "${env:ProgramFiles}\obs-studio"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

function Write-Step {
  param([string]$Message)
  Write-Host ""
  Write-Host "== $Message ==" -ForegroundColor Cyan
}

function Find-ObsSdk {
  param(
    [string]$ExplicitObsSdkDir,
    [string]$ExplicitObsStudioDir,
    [string]$RepoRoot
  )

  $candidates = @()

  if ($ExplicitObsSdkDir) { $candidates += $ExplicitObsSdkDir }
  if ($ExplicitObsStudioDir) { $candidates += $ExplicitObsStudioDir }

  $candidates += @(
    (Join-Path $RepoRoot ".deps"),
    (Join-Path $RepoRoot "obs-build-dependencies"),
    "$env:USERPROFILE\Desktop\obs-build-dependencies",
    "$env:USERPROFILE\Downloads\obs-build-dependencies",
    "$env:ProgramFiles\obs-studio",
    "$env:ProgramW6432\obs-studio"
  )

  foreach ($base in @((Join-Path $RepoRoot ".deps"), (Join-Path $RepoRoot "obs-build-dependencies"), "$env:USERPROFILE\Desktop\obs-build-dependencies", "$env:USERPROFILE\Downloads\obs-build-dependencies")) {
    if (Test-Path $base) {
      Get-ChildItem -Path $base -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like "plugin-deps-*" -or $_.Name -like "obs-studio*" -or $_.Name -like "windows-deps*" } |
        ForEach-Object { $candidates += $_.FullName }
    }
  }

  foreach ($candidate in $candidates) {
    if (-not $candidate -or -not (Test-Path $candidate)) { continue }

    $obsHeader = Get-ChildItem -Path $candidate -Recurse -Filter "obs-module.h" -ErrorAction SilentlyContinue | Select-Object -First 1
    $obsLib = Get-ChildItem -Path $candidate -Recurse -Include "obs.lib", "libobs.lib" -ErrorAction SilentlyContinue | Select-Object -First 1
    $frontendLib = Get-ChildItem -Path $candidate -Recurse -Filter "obs-frontend-api.lib" -ErrorAction SilentlyContinue | Select-Object -First 1

    if ($obsHeader -and $obsLib -and $frontendLib) {
      return (Resolve-Path $candidate).Path
    }
  }

  return $null
}

function Find-QtPrefix {
  param(
    [string]$ExplicitQt6Path,
    [string]$ObsSdk,
    [string]$RepoRoot
  )

  $candidates = @()

  if ($ExplicitQt6Path) { $candidates += $ExplicitQt6Path }

  if ($env:CMAKE_PREFIX_PATH) {
    $candidates += ($env:CMAKE_PREFIX_PATH -split ";")
  }

  if ($ObsSdk) {
    $candidates += @($ObsSdk, (Join-Path $ObsSdk "qt6"))
  }

  $candidates += @(
    (Join-Path $RepoRoot ".deps\qt6"),
    "$env:USERPROFILE\Desktop\obs-build-dependencies",
    "$env:USERPROFILE\Downloads\obs-build-dependencies",
    "C:\Qt\6.7.3\msvc2019_64",
    "C:\Qt\6.6.3\msvc2019_64",
    "C:\Qt\6.6.2\msvc2019_64"
  )

  foreach ($candidate in $candidates) {
    if (-not $candidate -or -not (Test-Path $candidate)) { continue }

    # Case 1: candidate is the Qt prefix.
    $config1 = Join-Path $candidate "lib\cmake\Qt6\Qt6Config.cmake"
    if (Test-Path $config1) {
      return (Resolve-Path $candidate).Path
    }

    # Case 2: candidate is already the lib directory.
    $config2 = Join-Path $candidate "cmake\Qt6\Qt6Config.cmake"
    if (Test-Path $config2) {
      return (Resolve-Path (Join-Path $candidate "..")).Path
    }

    # Case 3: recursive fallback, useful for obs-build-dependencies folders.
    $qtConfig = Get-ChildItem -Path $candidate -Recurse -Filter "Qt6Config.cmake" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($qtConfig) {
      # Qt6Config.cmake is expected under: <prefix>\lib\cmake\Qt6\Qt6Config.cmake
      $qt6Dir = Split-Path -Parent $qtConfig.FullName
      $cmakeDir = Split-Path -Parent $qt6Dir
      $libDir = Split-Path -Parent $cmakeDir
      $prefixDir = Split-Path -Parent $libDir
      return (Resolve-Path $prefixDir).Path
    }
  }

  return $null
}

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $RepoRoot

Write-Step "OBS Overtime Windows build"
Write-Host "Repo          : $RepoRoot"
Write-Host "Configuration : $Configuration"
Write-Host "Build dir     : $BuildDir"

if (-not [System.Environment]::Is64BitOperatingSystem) {
  throw "64-bit Windows is required."
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
  throw "cmake was not found in PATH."
}

if ($Clean -and (Test-Path $BuildDir)) {
  Write-Step "Cleaning build directory"
  Remove-Item -Recurse -Force $BuildDir
}

Write-Step "Resolving dependencies"

$ResolvedObsSdk = Find-ObsSdk -ExplicitObsSdkDir $ObsSdkDir -ExplicitObsStudioDir $ObsStudioDir -RepoRoot $RepoRoot
if (-not $ResolvedObsSdk) {
  throw @"
Could not find OBS development files.

Pass one of these:
  -ObsSdkDir C:\path\to\obs-sdk-or-plugin-deps
  -ObsStudioDir C:\path\to\obs-studio-build-or-install

The directory must contain:
  include\obs-module.h
  obs.lib or libobs.lib
  obs-frontend-api.lib
"@
}

$ResolvedQt = Find-QtPrefix -ExplicitQt6Path $Qt6Path -ObsSdk $ResolvedObsSdk -RepoRoot $RepoRoot
if (-not $ResolvedQt) {
  throw @"
Could not find Qt6.

Pass:
  -Qt6Path C:\Qt\6.x.x\msvc2019_64

The directory must contain:
  lib\cmake\Qt6\Qt6Config.cmake
"@
}

$Qt6ConfigDir = Join-Path $ResolvedQt "lib\cmake\Qt6"
$PrefixPath = @($ResolvedObsSdk, $ResolvedQt) -join ";"

Write-Host "OBS SDK       : $ResolvedObsSdk"
Write-Host "Qt6 prefix    : $ResolvedQt"
Write-Host "Qt6_DIR       : $Qt6ConfigDir"

Write-Step "Configuring"

$CMakeConfigureArgs = @(
  "-S", ".",
  "-B", $BuildDir,
  "-G", $Generator,
  "-A", "x64",
  "-DENABLE_FRONTEND_API=ON",
  "-DENABLE_QT=ON",
  "-DOBS_SDK_DIR=$ResolvedObsSdk",
  "-DQt6_DIR=$Qt6ConfigDir",
  "-DCMAKE_PREFIX_PATH=$PrefixPath"
)

& cmake @CMakeConfigureArgs
if ($LASTEXITCODE -ne 0) {
  throw "CMake configure failed with exit code $LASTEXITCODE."
}

Write-Step "Building"

& cmake --build $BuildDir --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) {
  throw "Build failed with exit code $LASTEXITCODE."
}

$PackageDir = Join-Path $RepoRoot "$BuildDir"
Write-Step "Build complete"
Write-Host "Build dir: $PackageDir" -ForegroundColor Green

if ($InstallToObs) {
  if (-not (Test-Path $ObsInstallDir)) {
    throw "OBS install dir not found: $ObsInstallDir"
  }

  $BuiltDll = Get-ChildItem -Path $BuildDir -Recurse -Filter "obs-overtime.dll" | Select-Object -First 1
  if (-not $BuiltDll) {
    throw "Could not find built obs-overtime.dll under $BuildDir"
  }

  $TargetBinDir = Join-Path $ObsInstallDir "obs-plugins\64bit"
  New-Item -ItemType Directory -Force -Path $TargetBinDir | Out-Null
  Copy-Item -Force $BuiltDll.FullName (Join-Path $TargetBinDir "obs-overtime.dll")

  $StagedData = Join-Path $RepoRoot "data"
  if (Test-Path $StagedData) {
    $TargetDataDir = Join-Path $ObsInstallDir "data\obs-plugins\obs-overtime"
    New-Item -ItemType Directory -Force -Path $TargetDataDir | Out-Null
    Copy-Item -Recurse -Force "$StagedData\*" $TargetDataDir
  }

  Write-Host "Installed to OBS: $ObsInstallDir" -ForegroundColor Green
}
