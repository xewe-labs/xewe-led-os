#Requires -Version 5.1
[CmdletBinding()]
param(
  [Parameter(Mandatory)][ValidateSet('c3','c6','s3')][Alias('c')][string]$Chip,
  [Alias('p')][string]$Port = '',
  [string]$ConfigJson = '',
  [Alias('n')][string]$BuildNotes = $null,
  [string]$FqbnExtra = ''
)

# 'Continue': native tools (python, arduino-cli via sub-scripts) write to stderr,
# which would terminate under 'Stop'. Exit codes are checked explicitly.
$ErrorActionPreference = 'Continue'

# build.ps1 - orchestrates compile -> optional upload -> optional serial monitor (Windows / PowerShell).
# Mirrors build/scripts/build.sh.
#
# Examples:
#   .\build.ps1 -Chip c3
#   .\build.ps1 -Chip s3 -Port COM5
#   .\build.ps1 -Chip c6 -ConfigJson '{"WIFI_SSID": "\"MyNet\"", "DEBUG_LEVEL": "2"}'
#   .\build.ps1 -Chip s3 -BuildNotes "Fixed Wi-Fi stability"
#   .\build.ps1 -Chip s3 -BuildNotes ""   # skips prompt, skips writing notes file

$ScriptDir = $PSScriptRoot
$BuildRoot = (Resolve-Path (Join-Path $ScriptDir '..\..')).Path
$ConfigPs = Join-Path $BuildRoot 'build_config.ps1'
if (-not (Test-Path $ConfigPs)) { throw "build_config.ps1 not found at $ConfigPs. Run setup_build_environment.ps1 first." }
. $ConfigPs

$ProjectName  = Get-Cfg project_name
$StateFile    = Get-Cfg build_state_file
$ConfigFile   = Get-Cfg project_config_h_file
$PythonBin    = Get-Cfg venv_python_bin
$BuildsLatest = Get-Cfg builds_latest_dir

$SerialBaud = '115200'

$TsIso = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ssZ')
$TsFs  = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH-mm-ssZ')

$BuildNotesProvided = $PSBoundParameters.ContainsKey('BuildNotes')

# Runs a Python snippet by writing it to a temp file (PowerShell 5.1 mangles double
# quotes when code is passed via `python -c`). Values are handed to Python through
# environment variables so quote-heavy data (JSON) survives intact.
# Returns captured output; sets $script:LastPyExit to the process exit code.
function Invoke-PyFile {
  param([Parameter(Mandatory)][string]$Code)
  $tmpPy = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName() + '.py')
  [IO.File]::WriteAllText($tmpPy, $Code, (New-Object System.Text.UTF8Encoding $false))
  try {
    $out = & $PythonBin $tmpPy 2>&1
    $script:LastPyExit = $LASTEXITCODE
    return ($out | Out-String)
  } finally {
    Remove-Item -Force $tmpPy -ErrorAction SilentlyContinue
  }
}

# ---------- Inject JSON configs into Config.h ----------
if (-not [string]::IsNullOrWhiteSpace($ConfigJson)) {
  Write-Host "Injecting JSON configs into $ConfigFile ..."
  $env:XW_CONFIG_JSON = $ConfigJson
  $env:XW_CONFIG_FILE = $ConfigFile
  $pyConfig = @'
import json,os,re
try:
    d=json.loads(os.environ["XW_CONFIG_JSON"]); f=os.environ["XW_CONFIG_FILE"]
    with open(f,"r") as file: c=file.read()
    for k,v in d.items():
        c, n = re.subn(r"(?m)^(#define\s+)"+re.escape(k)+r"(\s+).*$", r"\g<1>"+k+r"\g<2>"+str(v), c)
        if n == 0:
            raise SystemExit(f'Error: Key "{k}" not found in {f}')
    with open(f,"w") as file: file.write(c)
except Exception as e:
    raise SystemExit(str(e))
'@
  $out = Invoke-PyFile -Code $pyConfig
  if ($script:LastPyExit -ne 0) { Write-Host "[X] Config injection failed: $out" -ForegroundColor Red; exit 1 }
}

# ---------- Version helpers ----------
function Read-State {
  $state = @{ MAJOR = 0; MINOR = 0; PATCH = 0; BUILD_ID = 0 }
  if (Test-Path $StateFile) {
    foreach ($line in Get-Content -LiteralPath $StateFile) {
      if ($line -match '^\s*([A-Z_]+)\s*=\s*(.*)\s*$') { $state[$Matches[1]] = $Matches[2] }
    }
  }
  return $state
}

function Get-Version {
  $s = Read-State
  return "$([int]$s.MAJOR).$([int]$s.MINOR).$([int]$s.PATCH)"
}

function Bump-Patch {
  $s = Read-State
  $major = [int]$s.MAJOR
  $minor = [int]$s.MINOR
  $patch = [int]$s.PATCH + 1
  $buildId = [int]$s.BUILD_ID + 1
  $ts = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ssZ')
  @"
MAJOR=$major
MINOR=$minor
PATCH=$patch
BUILD_ID=$buildId
LAST_BUILD_TS=$ts
"@ | Set-Content -LiteralPath $StateFile -Encoding ASCII -ErrorAction Stop
}

$VersionNext = Get-Version

# ---------- Inject build params (Project, Version, Timestamp) into Config.h ----------
Write-Host "Injecting build params into $ConfigFile ..."
$env:XW_PROJECT_NAME = $ProjectName
$env:XW_BUILD_VERSION = $VersionNext
$env:XW_BUILD_TS = $TsIso
$env:XW_CONFIG_FILE = $ConfigFile
$pyParams = @'
import os,re
try:
    f=os.environ["XW_CONFIG_FILE"]
    d={"PROJECT_NAME": '"'+os.environ["XW_PROJECT_NAME"]+'"',
       "BUILD_VERSION": '"'+os.environ["XW_BUILD_VERSION"]+'"',
       "BUILD_TIMESTAMP": '"'+os.environ["XW_BUILD_TS"]+'"'}
    with open(f,"r") as file: c=file.read()
    if c and not c.endswith("\n"): c += "\n"
    for k,v in d.items():
        pattern = r"(?m)^(#define\s+)"+re.escape(k)+r"(\s+).*$"
        if re.search(pattern, c):
            c = re.sub(pattern, r"\g<1>"+k+r"\g<2>"+v, c)
        else:
            c += f"#define {k} {v}\n"
    with open(f,"w") as file: file.write(c)
except Exception as e:
    raise SystemExit(str(e))
'@
$out = Invoke-PyFile -Code $pyParams
if ($script:LastPyExit -ne 0) { Write-Host "[X] Build param injection failed: $out" -ForegroundColor Red; exit 1 }

# ---------- Build notes ----------
if (-not $BuildNotesProvided) {
  $BuildNotes = Read-Host 'Build notes (Enter to skip)'
}

# ---------- Compile ----------
$compilePs = Join-Path $ScriptDir 'compile.ps1'
$compileParams = @{
  Chip        = $Chip
  Version     = $VersionNext
  Timestamp   = $TsIso
  TimestampFs = $TsFs
}
if (-not [string]::IsNullOrWhiteSpace($FqbnExtra))  { $compileParams.FqbnExtra  = $FqbnExtra }
if (-not [string]::IsNullOrWhiteSpace($ConfigJson)) { $compileParams.ConfigJson = $ConfigJson }
& $compilePs @compileParams
if ($LASTEXITCODE -ne 0) { Write-Host "[X] Compile step failed" -ForegroundColor Red; exit 1 }

if (-not [string]::IsNullOrWhiteSpace($BuildNotes)) {
  $BuildNotes | Set-Content -LiteralPath (Join-Path $BuildsLatest 'build_notes.txt') -Encoding UTF8
}

# ---------- Upload ----------
if (-not [string]::IsNullOrWhiteSpace($Port)) {
  Write-Host "Upload -> $Port"
  & (Join-Path $ScriptDir 'upload.ps1') -Chip $Chip -Port $Port
  if ($LASTEXITCODE -ne 0) { Write-Host "[X] Upload step failed" -ForegroundColor Red; exit 1 }

  Bump-Patch
  Write-Host "Version bumped -> $(Get-Version)" -ForegroundColor Green
  Write-Host "Serial monitor..."
  & (Join-Path $ScriptDir 'listen_serial.ps1') -Port $Port -Baud $SerialBaud
} else {
  Write-Host "No port provided. Upload skipped."
}

Write-Host ''
Write-Host "Done." -ForegroundColor Green
