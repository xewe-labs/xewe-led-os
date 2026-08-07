#Requires -Version 5.1
[CmdletBinding()]
param(
  [Parameter(Mandatory)][string]$Port,
  [string]$Baud = '115200'
)

# 'Continue': serial-monitor tools stream to stderr, which would terminate under 'Stop'.
$ErrorActionPreference = 'Continue'

# listen_serial.ps1 - open a serial monitor to the device (Windows / PowerShell).
# Prefers Arduino CLI monitor; falls back to Python's pyserial miniterm.

$BuildRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$ConfigPs = Join-Path $BuildRoot 'build_config.ps1'
if (-not (Test-Path $ConfigPs)) { throw "build_config.ps1 not found at $ConfigPs. Run setup_build_environment.ps1 first." }
. $ConfigPs

$PythonBin = Get-Cfg venv_python_bin

if (Get-Command arduino-cli -ErrorAction SilentlyContinue) {
  Write-Host "arduino-cli monitor $Port @ $Baud (Ctrl-C to exit)..."
  & arduino-cli monitor -p $Port -c $Baud
  exit $LASTEXITCODE
}

& $PythonBin -c "import importlib.util,sys; sys.exit(0 if importlib.util.find_spec('serial.tools.miniterm') else 1)" 2>$null
if ($LASTEXITCODE -eq 0) {
  Write-Host "Python miniterm $Port @ $Baud (Ctrl-] then q to quit)..."
  & $PythonBin -m serial.tools.miniterm $Port $Baud
  exit $LASTEXITCODE
}

Write-Host "[X] No suitable serial monitor found (arduino-cli or pyserial)." -ForegroundColor Red
exit 1
