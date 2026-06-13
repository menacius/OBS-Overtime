<#
.SYNOPSIS
    Configures and builds the Projector Time Overlay OBS plugin on Windows.

.DESCRIPTION
    Wraps the CMake configure + build steps using the Visual Studio
    generator (x64). It locates the OBS Studio and Qt6 dependencies from,
    in order of precedence:
      1. The -ObsPath / -Qt6Path parameters.
      2. The OBS_STUDIO_DIR / Qt6_DIR (or CMAKE_PREFIX_PATH) environment
         variables.
      3. A local '.deps' directory next to the repository (matching the
         layout produced by the OBS obs-deps release archives).

.PARAMETER Configuration
    CMake build configuration. Defaults to RelWithDebInfo.

.PARAMETER ObsPath
    Path to an OBS Studio build/install tree that provides libobs and
    obs-frontend-api CMake config packages.

.PARAMETER Qt6Path
    Path to the Qt6 installation prefix (the folder containing
    'lib/cmake/Qt6').

.PARAMETER BuildDir
    Output build directory. Defaults to 'build_x64'.

.EXAMPLE
    ./scripts/build-windows.ps1 -ObsPath C:\obs-studio -Qt6Path C:\Qt\6.6.2\msvc2019_64

.EXAMPLE
    # Using a local .deps directory created from obs-deps archives.
    ./scripts/build-windows.ps1
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Configuration = 'RelWithDebInfo',

    [string]$ObsPath = $env:OBS_STUDIO_DIR,

    [string]$Qt6Path = $env:Qt6_DIR,

    [string]$BuildDir = 'build_x64',

    [string]$Generator = 'Visual Studio 17 2022'
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

# Repository root (parent of this scripts/ directory).
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

Write-Host "== Projector Time Overlay :: Windows build ==" -ForegroundColor Cyan
Write-Host "Repository : $RepoRoot"
Write-Host "Config     : $Configuration"

# --- Resolve dependency paths ------------------------------------------------

$DepsDir = Join-Path $RepoRoot '.deps'

function Resolve-DepPath {
    param(
        [string]$Explicit,
        [string[]]$Candidates,
        [string]$Name
    )
    if ($Explicit -and (Test-Path $Explicit)) {
        return (Resolve-Path $Explicit).Path
    }
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }
    Write-Warning "Could not auto-detect $Name. Pass it explicitly or set the matching environment variable."
    return $null
}

$ResolvedObs = Resolve-DepPath -Explicit $ObsPath -Name 'OBS Studio (libobs)' -Candidates @(
    (Join-Path $DepsDir 'obs-studio'),
    (Join-Path $DepsDir 'obs-studio/build_x64')
)

$ResolvedQt = Resolve-DepPath -Explicit $Qt6Path -Name 'Qt6' -Candidates @(
    (Join-Path $DepsDir 'qt6'),
    $env:CMAKE_PREFIX_PATH
)

# --- Assemble CMake arguments -----------------------------------------------

$prefixPaths = @()
if ($ResolvedObs) { $prefixPaths += $ResolvedObs }
if ($ResolvedQt) { $prefixPaths += $ResolvedQt }

$cmakeArgs = @(
    '-S', '.',
    '-B', $BuildDir,
    '-G', $Generator,
    '-A', 'x64',
    '-DENABLE_FRONTEND_API=ON',
    '-DENABLE_QT=ON'
)

if ($prefixPaths.Count -gt 0) {
    $joined = ($prefixPaths -join ';')
    $cmakeArgs += "-DCMAKE_PREFIX_PATH=$joined"
    Write-Host "CMAKE_PREFIX_PATH : $joined"
}

# --- Configure ---------------------------------------------------------------

Write-Host "`n-- Configuring --" -ForegroundColor Yellow
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE."
}

# --- Build -------------------------------------------------------------------

Write-Host "`n-- Building --" -ForegroundColor Yellow
& cmake --build $BuildDir --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

$outDir = Join-Path $RepoRoot "$BuildDir/$Configuration"
Write-Host "`n== Build complete ==" -ForegroundColor Green
Write-Host "Plugin binary (.dll) is in: $outDir"
Write-Host "Copy the .dll into '<OBS>/obs-plugins/64bit' and the 'data' folder into '<OBS>/data/obs-plugins/projector-time-overlay'."
