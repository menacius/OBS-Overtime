<#
.SYNOPSIS
  Converts Qt keyword macros to QT_NO_KEYWORDS-safe names.

.DESCRIPTION
  Run once from the repository root after replacing CMakeLists.txt.
  It updates source/header files under src/:
    signals: -> Q_SIGNALS:
    public slots: -> public Q_SLOTS:
    protected slots: -> protected Q_SLOTS:
    private slots: -> private Q_SLOTS:
    slots: -> Q_SLOTS:
    emit foo -> Q_EMIT foo
#>

[CmdletBinding()]
param(
  [string]$SourceDir = "src"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $SourceDir)) {
  throw "Source directory not found: $SourceDir"
}

$files = Get-ChildItem -Path $SourceDir -Recurse -Include *.cpp,*.hpp,*.h,*.cc,*.cxx

foreach ($file in $files) {
  $text = Get-Content -Raw -Path $file.FullName
  $original = $text

  $text = $text -replace '(?m)^([ \t]*)signals\s*:', '$1Q_SIGNALS:'
  $text = $text -replace '(?m)^([ \t]*)public\s+slots\s*:', '$1public Q_SLOTS:'
  $text = $text -replace '(?m)^([ \t]*)protected\s+slots\s*:', '$1protected Q_SLOTS:'
  $text = $text -replace '(?m)^([ \t]*)private\s+slots\s*:', '$1private Q_SLOTS:'
  $text = $text -replace '(?m)^([ \t]*)slots\s*:', '$1Q_SLOTS:'
  $text = $text -replace '\bemit\s+', 'Q_EMIT '

  if ($text -ne $original) {
    Set-Content -Path $file.FullName -Value $text -NoNewline
    Write-Host "Updated $($file.FullName)"
  }
}

Write-Host "Done. Now run: pwsh ./scripts/build-windows.ps1 -Clean" -ForegroundColor Green
