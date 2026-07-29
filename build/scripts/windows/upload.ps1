#Requires -Version 5.1
[CmdletBinding()]
param(
  [Parameter(Mandatory)][ValidateSet('c3','c6','s3')][string]$Chip,
  [Parameter(Mandatory)][string]$Port,
  [string]$BuildDir = '',
  [string]$Baud = '921600'
)

# 'Continue': esptool writes progress to stderr, which would terminate under 'Stop'.
# Exit code is checked explicitly below.
$ErrorActionPreference = 'Continue'

# upload.ps1 - flash a single merged image (at 0x0) via esptool (Windows / PowerShell).
# Mirrors build/scripts/upload.sh.

$BuildRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$ConfigPs = Join-Path $BuildRoot 'build_config.ps1'
if (-not (Test-Path $ConfigPs)) { throw "build_config.ps1 not found at $ConfigPs. Run setup_build_environment.ps1 first." }
. $ConfigPs

if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Get-Cfg builds_latest_dir }
$PythonBin = Get-Cfg venv_python_bin

switch ($Chip) {
  'c3' { $EspId = 'esp32c3' }
  'c6' { $EspId = 'esp32c6' }
  's3' { $EspId = 'esp32s3' }
}

$binDir = Join-Path $BuildDir 'binary'
$binFiles = @(Get-ChildItem -LiteralPath $binDir -Filter '*.bin' -File -ErrorAction SilentlyContinue)

if ($binFiles.Count -eq 0) {
  Write-Host "[X] No .bin file found in $binDir" -ForegroundColor Red
  exit 1
} elseif ($binFiles.Count -gt 1) {
  Write-Host "[!] Multiple .bin files found. Using the first one: $($binFiles[0].FullName)" -ForegroundColor Yellow
}

$firmwareBin = $binFiles[0].FullName

Write-Host "Build   : $BuildDir"
Write-Host "Image   : $firmwareBin"
Write-Host "Port    : $Port"
Write-Host "Baud    : $Baud"
Write-Host "Chip    : $EspId"
Write-Host "Esptool : $PythonBin -m esptool"
Write-Host "Flashing merged image at 0x00000000 ..."

& $PythonBin -m esptool --chip $EspId --port $Port --baud $Baud write-flash 0x0 $firmwareBin
if ($LASTEXITCODE -ne 0) { Write-Host "[X] Upload failed" -ForegroundColor Red; exit 1 }

Write-Host "Upload complete." -ForegroundColor Green
exit 0
