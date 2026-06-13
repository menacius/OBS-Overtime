<#
.SYNOPSIS
  Configure and build OBS Overtime on Windows.

.DESCRIPTION
  This script intentionally prefers the newest OBS/Qt dependency directories.
  Old plugin-deps packages such as plugin-deps-2022-08-02-qt6-x64 can fail to
  parse Qt headers with current Visual Studio 2022 toolchains. Pass -Qt6Path or
  -ObsSdkDir to override auto-detection.
#>

[CmdletBinding()]
param(
  [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
  [string]$Configuration = "RelWithDebInfo",

  [string]$BuildDir = "build_x64",

  [string]$Generator = "Visual Studio 17 2022",

  [string]$ObsSdkDir = "",

  [string]$ObsStudioDir = "",

  [string]$Qt6Path = "",

  [switch]$Clean,

  [switch]$InstallToObs,

  [string]$ObsInstallDir = "${env:ProgramFiles}\obs-studio"
)

$ErrorActionPreference = "Stop"

function Write-Step {
  param([string]$Message)
  Write-Host ""
  Write-Host "== $Message ==" -ForegroundColor Cyan
}

function Get-ExistingDirectories {
  param([string[]]$Paths)

  $result = @()
  foreach ($path in $Paths) {
    if ($path -and (Test-Path $path -PathType Container)) {
      $result += (Resolve-Path $path).Path
    }
  }
  return $result | Select-Object -Unique
}

function Get-DateScoreFromName {
  param([string]$Name)

  if ($Name -match '(20\d{2})[-_](\d{2})[-_](\d{2})') {
    return [int]("$($Matches[1])$($Matches[2])$($Matches[3])")
  }
  if ($Name -match '(20\d{6})') {
    return [int]$Matches[1]
  }
  return 0
}

function Normalize-QtPrefix {
  param([string]$Path)

  if (-not $Path -or -not (Test-Path $Path)) { return $null }
  $resolved = (Resolve-Path $Path).Path

  # Qt config dir: <prefix>\lib\cmake\Qt6
  if (Test-Path (Join-Path $resolved "Qt6Config.cmake")) {
    $qt6Dir = $resolved
    $cmakeDir = Split-Path -Parent $qt6Dir
    $libDir = Split-Path -Parent $cmakeDir
    $prefixDir = Split-Path -Parent $libDir
    if (Test-Path (Join-Path $prefixDir "lib\cmake\Qt6\Qt6Config.cmake")) {
      return (Resolve-Path $prefixDir).Path
    }
  }

  # Qt prefix: <prefix>\lib\cmake\Qt6\Qt6Config.cmake
  if (Test-Path (Join-Path $resolved "lib\cmake\Qt6\Qt6Config.cmake")) {
    return $resolved
  }

  # Already <prefix>\lib
  if (Test-Path (Join-Path $resolved "cmake\Qt6\Qt6Config.cmake")) {
    return (Resolve-Path (Join-Path $resolved "..")).Path
  }

  return $null
}

function Find-QtPrefix {
  param(
    [string]$ExplicitQt6Path,
    [string]$ObsSdk,
    [string]$RepoRoot
  )

  $manual = @()
  if ($ExplicitQt6Path) { $manual += $ExplicitQt6Path }
  if ($env:Qt6_DIR) { $manual += $env:Qt6_DIR }
  if ($env:QTDIR) { $manual += $env:QTDIR }

  foreach ($candidate in $manual) {
    $prefix = Normalize-QtPrefix $candidate
    if ($prefix) { return $prefix }
  }

  $bases = @(
    (Join-Path $RepoRoot ".deps"),
    (Join-Path $RepoRoot "obs-build-dependencies"),
    "$env:USERPROFILE\Desktop\obs-build-dependencies",
    "$env:USERPROFILE\Downloads\obs-build-dependencies"
  )

  if ($ObsSdk) {
    $bases += $ObsSdk
    $bases += (Split-Path -Parent $ObsSdk)
  }

  if ($env:CMAKE_PREFIX_PATH) {
    $bases += ($env:CMAKE_PREFIX_PATH -split ";")
  }

  $dirs = @()
  foreach ($base in (Get-ExistingDirectories $bases)) {
    $prefix = Normalize-QtPrefix $base
    if ($prefix) { $dirs += $prefix }

    Get-ChildItem -Path $base -Directory -ErrorAction SilentlyContinue |
      Where-Object { $_.Name -match 'qt6' -or $_.Name -match 'Qt' -or $_.Name -match 'plugin-deps' } |
      ForEach-Object {
        $prefix = Normalize-QtPrefix $_.FullName
        if ($prefix) { $dirs += $prefix }
      }
  }

  $dirs = $dirs | Select-Object -Unique
  if (-not $dirs -or $dirs.Count -eq 0) { return $null }

  $ranked = $dirs | ForEach-Object {
    [PSCustomObject]@{
      Path = $_
      Score = Get-DateScoreFromName (Split-Path -Leaf $_)
      LastWriteTime = (Get-Item $_).LastWriteTimeUtc
    }
  } | Sort-Object -Property Score, LastWriteTime -Descending

  return $ranked[0].Path
}

function Test-ObsSdkDir {
  param([string]$Path)

  if (-not $Path -or -not (Test-Path $Path -PathType Container)) { return $false }

  $obsHeader = Get-ChildItem -Path $Path -Recurse -Filter "obs-module.h" -ErrorAction SilentlyContinue | Select-Object -First 1
  $obsLib = Get-ChildItem -Path $Path -Recurse -Include "obs.lib", "libobs.lib" -ErrorAction SilentlyContinue | Select-Object -First 1
  $frontendLib = Get-ChildItem -Path $Path -Recurse -Filter "obs-frontend-api.lib" -ErrorAction SilentlyContinue | Select-Object -First 1

  return [bool]($obsHeader -and $obsLib -and $frontendLib)
}

function Find-ObsSdk {
  param(
    [string]$ExplicitObsSdkDir,
    [string]$ExplicitObsStudioDir,
    [string]$RepoRoot
  )

  $manual = @()
  if ($ExplicitObsSdkDir) { $manual += $ExplicitObsSdkDir }
  if ($ExplicitObsStudioDir) { $manual += $ExplicitObsStudioDir }
  if ($env:OBS_SDK_DIR) { $manual += $env:OBS_SDK_DIR }
  if ($env:OBS_STUDIO_DIR) { $manual += $env:OBS_STUDIO_DIR }

  foreach ($candidate in $manual) {
    if (Test-ObsSdkDir $candidate) { return (Resolve-Path $candidate).Path }
  }

  $bases = @(
    (Join-Path $RepoRoot ".deps"),
    (Join-Path $RepoRoot "obs-build-dependencies"),
    "$env:USERPROFILE\Desktop\obs-build-dependencies",
    "$env:USERPROFILE\Downloads\obs-build-dependencies",
    "$env:ProgramFiles\obs-studio",
    "$env:ProgramW6432\obs-studio"
  )

  $dirs = @()
  foreach ($base in (Get-ExistingDirectories $bases)) {
    Get-ChildItem -Path $base -Directory -ErrorAction SilentlyContinue |
      Where-Object { $_.Name -match 'plugin-deps' -or $_.Name -match 'obs-studio' -or $_.Name -match 'windows-deps' } |
      ForEach-Object {
        if (Test-ObsSdkDir $_.FullName) { $dirs += $_.FullName }
      }

    if (Test-ObsSdkDir $base) { $dirs += $base }
  }

  $dirs = $dirs | Select-Object -Unique
  if (-not $dirs -or $dirs.Count -eq 0) { return $null }

  $ranked = $dirs | ForEach-Object {
    [PSCustomObject]@{
      Path = (Resolve-Path $_).Path
      Score = Get-DateScoreFromName (Split-Path -Leaf $_)
      LastWriteTime = (Get-Item $_).LastWriteTimeUtc
    }
  } | Sort-Object -Property Score, LastWriteTime -Descending

  return $ranked[0].Path
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $RepoRoot

Write-Step "OBS Overtime Windows build"
Write-Host "Repo          : $RepoRoot"
Write-Host "Configuration : $Configuration"
Write-Host "Build dir     : $BuildDir"

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
  throw "Could not find OBS development files. Pass -ObsSdkDir C:\path\to\plugin-deps-or-obs-build."
}

$ResolvedQt = Find-QtPrefix -ExplicitQt6Path $Qt6Path -ObsSdk $ResolvedObsSdk -RepoRoot $RepoRoot
if (-not $ResolvedQt) {
  throw "Could not find Qt6. Pass -Qt6Path C:\path\to\plugin-deps-YYYY-MM-DD-qt6-x64."
}

$Qt6ConfigDir = Join-Path $ResolvedQt "lib\cmake\Qt6"
$PrefixPath = @($ResolvedObsSdk, $ResolvedQt) -join ";"

Write-Host "OBS SDK       : $ResolvedObsSdk"
Write-Host "Qt6 prefix    : $ResolvedQt"
Write-Host "Qt6_DIR       : $Qt6ConfigDir"

if ($ResolvedQt -match '2022-08-02') {
  Write-Warning "You are building against old Qt deps: $ResolvedQt"
  Write-Warning "If qobject.h / qtimer.h fails with QtPrivate parser errors, install or select newer OBS Qt6 deps, e.g. plugin-deps-2025-xx-xx-qt6-x64, using -Qt6Path."
}

Write-Step "Configuring"

$CMakeConfigureArgs = @(
  "-S", ".",
  "-B", $BuildDir,
  "-G", $Generator,
  "-A", "x64",
  "-DOBS_SDK_DIR=$ResolvedObsSdk",
  "-DQt6_DIR=$Qt6ConfigDir",
  "-DCMAKE_PREFIX_PATH=$PrefixPath"
)

& cmake @CMakeConfigureArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed with exit code $LASTEXITCODE." }

Write-Step "Building"
& cmake --build $BuildDir --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE." }

$PackageDir = Join-Path $RepoRoot "$BuildDir\obs-overtime"
Write-Step "Build complete"
Write-Host "Package dir: $PackageDir" -ForegroundColor Green

if ($InstallToObs) {
  if (-not (Test-Path $ObsInstallDir)) { throw "OBS install dir not found: $ObsInstallDir" }

  $BuiltDll = Get-ChildItem -Path $PackageDir -Recurse -Filter "obs-overtime.dll" -ErrorAction SilentlyContinue | Select-Object -First 1
  if (-not $BuiltDll) { throw "Could not find built obs-overtime.dll under $PackageDir" }

  $TargetBinDir = Join-Path $ObsInstallDir "obs-plugins\64bit"
  New-Item -ItemType Directory -Force -Path $TargetBinDir | Out-Null
  Copy-Item -Force $BuiltDll.FullName (Join-Path $TargetBinDir "obs-overtime.dll")

  $StagedData = Join-Path $PackageDir "data"
  if (Test-Path $StagedData) {
    $ObsDataDir = Join-Path $ObsInstallDir "data\obs-plugins\obs-overtime"
    New-Item -ItemType Directory -Force -Path $ObsDataDir | Out-Null
    Copy-Item -Recurse -Force "$StagedData\*" $ObsDataDir
  }

  Write-Host "Installed to OBS." -ForegroundColor Green
}
