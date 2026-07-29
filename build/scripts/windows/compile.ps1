#Requires -Version 5.1
[CmdletBinding()]
param(
  [Parameter(Mandatory)][ValidateSet('c3','c6','s3')][string]$Chip,
  [Parameter(Mandatory)][string]$Version,
  [Parameter(Mandatory)][string]$Timestamp,   # ISO-8601 UTC, e.g. 2026-07-13T11:22:33Z
  [string]$TimestampFs = '',                  # filesystem-safe variant (colons -> dashes)
  [string]$FqbnExtra = '',
  [string]$ConfigJson = ''
)

# 'Continue': native tools (arduino-cli) write progress/warnings to stderr, which
# would raise a terminating error under 'Stop'. Exit codes are checked explicitly;
# correctness-critical cmdlets below use -ErrorAction Stop.
$ErrorActionPreference = 'Continue'

# compile.ps1 - build a versioned ESP32 firmware using Arduino CLI (Windows / PowerShell).
# Mirrors build/scripts/compile.sh.

$CompileStartEpoch = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()

$BuildRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$ConfigPs = Join-Path $BuildRoot 'build_config.ps1'
if (-not (Test-Path $ConfigPs)) { throw "build_config.ps1 not found at $ConfigPs. Run setup_build_environment.ps1 first." }
. $ConfigPs

$ProjectRoot   = Get-Cfg project_root
$BuildsDir     = Get-Cfg builds_dir
$WorkDirBase   = Get-Cfg builds_cache_dir
$ProjectName   = Get-Cfg project_name
$LibsDir       = Get-Cfg libraries_dir
$PythonBin     = Get-Cfg venv_python_bin
$BuildsLatest  = Get-Cfg builds_latest_dir

if ([string]::IsNullOrWhiteSpace($TimestampFs)) {
  $TimestampFs = $Timestamp -replace ':', '-'
}

switch ($Chip) {
  'c3' { $FqbnBoard = 'esp32c3'; $ChipFamily = 'ESP32-C3' }
  'c6' { $FqbnBoard = 'esp32c6'; $ChipFamily = 'ESP32-C6' }
  's3' { $FqbnBoard = 'esp32s3'; $ChipFamily = 'ESP32-S3' }
}

$WorkDir = Join-Path (Split-Path $WorkDirBase -Parent) "cache\$Chip"
New-Item -ItemType Directory -Force -Path $WorkDir -ErrorAction Stop | Out-Null

# Validate + canonicalize config JSON (sorted keys, compact) for meta.json.
# JSON is passed via an env var (PowerShell 5.1 corrupts quote-heavy native args).
$ConfigJsonValidated = '""'
if (-not [string]::IsNullOrWhiteSpace($ConfigJson)) {
  $env:XW_CONFIG_JSON = $ConfigJson
  $py = @'
import json,os
print(json.dumps(json.loads(os.environ["XW_CONFIG_JSON"]), separators=(",",":"), sort_keys=True))
'@
  $tmpPy = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName() + '.py')
  [IO.File]::WriteAllText($tmpPy, $py, (New-Object System.Text.UTF8Encoding $false))
  try {
    $ConfigJsonValidated = (& $PythonBin $tmpPy 2>$null | Out-String).Trim()
    $pyExit = $LASTEXITCODE
  } finally {
    Remove-Item -Force $tmpPy -ErrorAction SilentlyContinue
  }
  if ($pyExit -ne 0) { Write-Host "[X] Invalid -ConfigJson (must be valid JSON): $ConfigJson" -ForegroundColor Red; exit 1 }
}

$FqbnOpts = 'CDCOnBoot=cdc,CPUFreq=160,DebugLevel=none,EraseFlash=all,FlashMode=qio,FlashSize=4M,JTAGAdapter=default,PartitionScheme=no_ota,UploadSpeed=921600'
if (-not [string]::IsNullOrWhiteSpace($FqbnExtra)) { $FqbnOpts += ",$FqbnExtra" }
$Fqbn = "esp32:esp32:${FqbnBoard}:${FqbnOpts}"
$SketchPath = Get-Cfg project_ino_file

$TargetDir = Join-Path $BuildsDir "$TimestampFs-$Version-$Chip-$ProjectName"
$OutputDir = Join-Path $TargetDir 'output'
$BinaryDir = Join-Path $TargetDir 'binary'
New-Item -ItemType Directory -Force -Path $OutputDir, $BinaryDir -ErrorAction Stop | Out-Null

Write-Host "Arduino FQBN: $Fqbn"
Write-Host "Sketch:       $SketchPath"
Write-Host "Using libs:   $LibsDir"
Write-Host "Target dir:   $TargetDir"
Write-Host "Work path:    $WorkDir"

# ---------- Pre-compile: snapshot sources ----------
Copy-Item -Recurse -Force -ErrorAction Stop (Join-Path $ProjectRoot 'src') (Join-Path $TargetDir 'src')
Copy-Item -Recurse -Force -ErrorAction Stop $LibsDir (Join-Path $TargetDir 'libs')

# ---------- Compile ----------
$compileArgs = @('compile', '--fqbn', $Fqbn, '--build-path', $WorkDir, '--warnings', 'default')
foreach ($libdir in (Get-ChildItem -Directory $LibsDir -ErrorAction SilentlyContinue)) {
  $compileArgs += @('--library', $libdir.FullName)
}
$compileArgs += $SketchPath

& arduino-cli @compileArgs
if ($LASTEXITCODE -ne 0) { Write-Host "[X] Compile failed" -ForegroundColor Red; exit 1 }

# ---------- Process artifacts ----------
$mergedBin = Get-ChildItem -LiteralPath $WorkDir -Filter "$ProjectName.ino.merged.bin" -File -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $mergedBin) { Write-Host "[X] Merged bin not found in $WorkDir" -ForegroundColor Red; exit 1 }

$mergedBinFilename = "$Version-$Chip-$ProjectName.bin"
Copy-Item -Force -ErrorAction Stop $mergedBin.FullName (Join-Path $BinaryDir $mergedBinFilename)

# UTF-8 without BOM (PS 5.1 Set-Content -Encoding UTF8 emits a BOM that can break JSON consumers)
$Utf8NoBom = New-Object System.Text.UTF8Encoding $false
function Write-TextNoBom {
  param([string]$Path, [string]$Content)
  [System.IO.File]::WriteAllText($Path, $Content, $Utf8NoBom)
}

$manifestJson = @"
{
  "name": "$ProjectName",
  "version": "$Version",
  "new_install_improv_wait_time": 0,
  "builds": [
    {
      "chipFamily": "$ChipFamily",
      "parts": [
        { "path": "$mergedBinFilename", "offset": 0 }
      ]
    }
  ]
}
"@
Write-TextNoBom -Path (Join-Path $BinaryDir 'manifest.json') -Content $manifestJson

# ---------- Update builds/latest junction ----------
if (Test-Path $BuildsLatest) {
  # rmdir removes only the junction link, never the target contents
  cmd /c rmdir "$BuildsLatest" 2>$null | Out-Null
}
New-Item -ItemType Junction -Path $BuildsLatest -Target $TargetDir -ErrorAction Stop | Out-Null

$CompileTimeSec = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds() - $CompileStartEpoch

function ConvertTo-JsonString {
  param([string]$s)
  if ($null -eq $s) { return '' }
  $s = $s -replace '\\', '\\'
  $s = $s -replace '"', '\"'
  $s = $s -replace "`n", '\n'
  $s = $s -replace "`r", '\r'
  $s = $s -replace "`t", '\t'
  return $s
}

$targetDirRel = "$ProjectName" + $TargetDir.Substring($TargetDir.IndexOf($ProjectName) + $ProjectName.Length)

$metaJson = @"
{
  "type": "$(ConvertTo-JsonString $Chip)",
  "chip_family": "$(ConvertTo-JsonString $ChipFamily)",
  "project_name": "$(ConvertTo-JsonString $ProjectName)",
  "version": "$(ConvertTo-JsonString $Version)",
  "timestamp_param": "$(ConvertTo-JsonString $Timestamp)",
  "config": $ConfigJsonValidated,
  "fqbn": "$(ConvertTo-JsonString $Fqbn)",
  "fqbn_extra": "$(ConvertTo-JsonString $FqbnExtra)",
  "compile_time_sec": $CompileTimeSec,
  "artifacts": {
    "binary_filename": "$(ConvertTo-JsonString $mergedBinFilename)",
    "path_rel_binary": "$(ConvertTo-JsonString "$targetDirRel/binary/$mergedBinFilename")",
    "path_rel_manifest_json": "$(ConvertTo-JsonString "$targetDirRel/binary/manifest.json")",
    "path_rel_meta_json": "$(ConvertTo-JsonString "$targetDirRel/binary/meta.json")",
    "path_abs_binary": "$(ConvertTo-JsonString (Join-Path $BinaryDir $mergedBinFilename))",
    "path_abs_manifest_json": "$(ConvertTo-JsonString (Join-Path $BinaryDir 'manifest.json'))",
    "path_abs_meta_json": "$(ConvertTo-JsonString (Join-Path $BinaryDir 'meta.json'))"
  }
}
"@
Write-TextNoBom -Path (Join-Path $BinaryDir 'meta.json') -Content $metaJson

Write-Host ''
Write-Host "Build complete." -ForegroundColor Green
Write-Host ("Total compile time: {0}m {1}s" -f [int]($CompileTimeSec / 60), ($CompileTimeSec % 60))
Write-Host " -> Final dir : $TargetDir"
Write-Host " -> Firmware  : $(Join-Path $BinaryDir $mergedBinFilename)"
Write-Host " -> Version   : $Version"
exit 0
