#Requires -Version 5.1
[CmdletBinding()]
param(
  [switch]$Yes  # answer "yes" to all prompts (non-interactive)
)

# 'Continue' (not 'Stop'): under 'Stop', native tools that merely write to stderr
# (arduino-cli, git, pip, winget) raise a terminating NativeCommandError. We check
# $LASTEXITCODE explicitly for the calls that matter instead.
$ErrorActionPreference = 'Continue'

# setup_build_environment.ps1 - one-time environment setup for this repo (Windows / PowerShell).
# - Checks winget (used to install missing tools)
# - Checks Arduino CLI; offers to install via winget if missing
# - Checks ESP32 core; offers to install if missing
# - Checks Python; offers to install via winget if missing
# - Creates .venv under build/
# - Checks esptool; offers to install into .venv if missing
# - Clones Arduino libraries from required_libraries.txt into build/libraries
# - Creates build_config.ps1 for reuse by build/upload scripts

$ScriptDir   = $PSScriptRoot
$BuildRoot   = (Resolve-Path (Join-Path $ScriptDir '..\..')).Path
$ProjectRoot = (Resolve-Path (Join-Path $ScriptDir '..\..\..')).Path
$ProjectName = Split-Path $ProjectRoot -Leaf

$VenvDir          = Join-Path $BuildRoot '.venv'
$LibrariesDir     = Join-Path $BuildRoot 'libraries'
$RequirementsFile = Join-Path $LibrariesDir 'required_libraries.txt'
$StateFile        = Join-Path $BuildRoot 'version_state'
$ReleaseMatrix    = Join-Path $BuildRoot 'release_matrix.csv'
$BuildConfigFile  = Join-Path $BuildRoot 'build_config.ps1'

$VenvPython = Join-Path $VenvDir 'Scripts\python.exe'
$VenvPip    = Join-Path $VenvDir 'Scripts\pip.exe'
$InoFile    = Join-Path $ProjectRoot "$ProjectName.ino"
$ConfigH    = Join-Path $ProjectRoot 'Config.h'

$Esp32CoreFqbn       = 'esp32:esp32'
$Esp32BoardManagerUrl = 'https://espressif.github.io/arduino-esp32/package_esp32_index.json'

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
function Write-Info { param([string]$Msg) Write-Host $Msg -ForegroundColor Cyan }
function Write-Ok   { param([string]$Msg) Write-Host "[OK] $Msg" -ForegroundColor Green }
function Write-Warn { param([string]$Msg) Write-Host "[!] $Msg" -ForegroundColor Yellow }
function Write-Err  { param([string]$Msg) Write-Host "[X] $Msg" -ForegroundColor Red }

function Confirm-Action {
  param([string]$Prompt = 'Continue?')
  if ($Yes) { return $true }
  $ans = Read-Host "$Prompt [y/N]"
  return ($ans -match '^(y|Y|yes|YES)$')
}

function Test-Cmd {
  param([string]$Name)
  return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

# Refresh the current session PATH from Machine + User scopes so tools installed
# by winget during this run become callable without restarting the shell.
function Update-SessionPath {
  $machine = [Environment]::GetEnvironmentVariable('Path', 'Machine')
  $user    = [Environment]::GetEnvironmentVariable('Path', 'User')
  $env:Path = (@($machine, $user) | Where-Object { $_ }) -join ';'
}

function Install-WithWinget {
  param(
    [Parameter(Mandatory)][string]$Id,
    [Parameter(Mandatory)][string]$DisplayName
  )
  if (-not (Test-Cmd winget)) {
    Write-Err "winget not found; cannot auto-install $DisplayName."
    Write-Err "Install '$DisplayName' manually, or install App Installer (winget) from the Microsoft Store, then re-run."
    exit 1
  }
  Write-Info "Installing $DisplayName via winget ($Id)..."
  winget install --id $Id --exact --accept-source-agreements --accept-package-agreements --silent
  if ($LASTEXITCODE -ne 0) {
    Write-Err "winget failed to install $DisplayName (exit $LASTEXITCODE)."
    exit 1
  }
  Update-SessionPath
}

# ---------------------------------------------------------------------------
# Steps
# ---------------------------------------------------------------------------
function Ensure-Git {
  if (Test-Cmd git) { Write-Ok "git found: $(git --version)"; return }
  Write-Warn 'git not found (needed to clone libraries).'
  if (Confirm-Action 'Install Git via winget now?') {
    Install-WithWinget -Id 'Git.Git' -DisplayName 'Git'
    if (-not (Test-Cmd git)) { Write-Err 'git still not found after install.'; exit 1 }
    Write-Ok 'git installed.'
  } else {
    Write-Err 'git is required to clone libraries.'; exit 1
  }
}

function Ensure-ArduinoCli {
  if (Test-Cmd arduino-cli) { Write-Ok "arduino-cli found: $(arduino-cli version)"; return }
  Write-Warn 'arduino-cli not found.'
  if (Confirm-Action 'Install Arduino CLI via winget now?') {
    Install-WithWinget -Id 'ArduinoSA.CLI' -DisplayName 'Arduino CLI'
    if (-not (Test-Cmd arduino-cli)) { Write-Err 'arduino-cli still not found after install.'; exit 1 }
    Write-Ok 'arduino-cli installed.'
  } else {
    Write-Err 'Arduino CLI is required for compile.ps1.'; exit 1
  }
}

function Ensure-Esp32Core {
  $installed = (arduino-cli core list 2>$null | Select-String -Pattern "^$([regex]::Escape($Esp32CoreFqbn))\s")
  if ($installed) { Write-Ok "ESP32 core ($Esp32CoreFqbn) is already installed."; return }

  Write-Warn 'ESP32 core not found.'
  if (Confirm-Action "Install ESP32 core ($Esp32CoreFqbn) now?") {
    Write-Info 'Initializing Arduino config and adding Espressif URL...'
    arduino-cli config init 2>$null | Out-Null
    arduino-cli config add board_manager.additional_urls $Esp32BoardManagerUrl 2>$null | Out-Null

    Write-Info 'Updating core index...'
    arduino-cli core update-index

    Write-Info "Installing $Esp32CoreFqbn ..."
    arduino-cli core install $Esp32CoreFqbn
    if ($LASTEXITCODE -ne 0) { Write-Err 'Failed to install ESP32 core.'; exit 1 }
    Write-Ok 'ESP32 core installed.'
  } else {
    Write-Err 'The ESP32 core is required to build the project.'; exit 1
  }
}

function Ensure-Libraries {
  if (-not (Test-Path $RequirementsFile)) {
    Write-Warn "Requirements file not found at: $RequirementsFile"
    Write-Warn 'Skipping library installation.'
    return
  }

  Write-Info "Checking Arduino libraries in $LibrariesDir ..."
  New-Item -ItemType Directory -Force -Path $LibrariesDir -ErrorAction Stop | Out-Null

  foreach ($raw in Get-Content -LiteralPath $RequirementsFile) {
    $line = $raw.Trim()
    if ([string]::IsNullOrWhiteSpace($line) -or $line.StartsWith('#')) { continue }

    $parts   = $line -split '\s+'
    $repoUrl = $parts[0]
    $gitArgs = @($parts[1..($parts.Length - 1)]) | Where-Object { $_ }

    $repoName   = [IO.Path]::GetFileNameWithoutExtension($repoUrl)
    $targetPath = Join-Path $LibrariesDir $repoName

    if (Test-Path $targetPath) {
      Write-Host "   - $repoName already exists."
      continue
    }

    Write-Host "   downloading $repoName ..."
    $cloneArgs = @('clone', '--quiet', '--depth', '1') + $gitArgs + @($repoUrl, $targetPath)
    & git @cloneArgs
    if ($LASTEXITCODE -ne 0) { Write-Err "Failed to clone $repoUrl"; exit 1 }

    Remove-Item -Recurse -Force (Join-Path $targetPath '.git') -ErrorAction SilentlyContinue
    Write-Host "      (removed .git from $repoName)"
  }

  Write-Ok 'Libraries are ready.'
}

function Ensure-Python {
  if (Test-Cmd python) {
    $ver = (python --version) 2>&1
    Write-Ok "Python found: $ver"
    return
  }
  Write-Warn 'Python not found.'
  if (Confirm-Action 'Install Python 3 via winget now?') {
    Install-WithWinget -Id 'Python.Python.3.12' -DisplayName 'Python 3.12'
    if (-not (Test-Cmd python)) { Write-Err 'python still not found after install.'; exit 1 }
    Write-Ok "Python installed: $((python --version) 2>&1)"
  } else {
    Write-Err 'Python is required (for venv + esptool).'; exit 1
  }
}

function Ensure-Venv {
  if ((Test-Path $VenvDir) -and (Test-Path $VenvPython)) {
    Write-Ok ".venv already exists: $VenvDir"
    return
  }
  Write-Info "Creating venv at $VenvDir"
  python -m venv $VenvDir
  if (-not (Test-Path $VenvPython)) { Write-Err 'venv creation failed.'; exit 1 }
  & $VenvPython -m pip install --upgrade pip | Out-Null
  Write-Ok 'venv ready.'
}

function Ensure-Esptool {
  & $VenvPython -c 'import esptool' 2>$null
  if ($LASTEXITCODE -eq 0) { Write-Ok 'esptool is present in .venv'; return }

  Write-Warn 'esptool not found in .venv (required for upload).'
  if (Confirm-Action 'Install esptool into .venv now?') {
    & $VenvPython -m pip install --upgrade esptool
    & $VenvPython -c 'import esptool' 2>$null
    if ($LASTEXITCODE -ne 0) { Write-Err 'esptool install failed.'; exit 1 }
    Write-Ok 'esptool installed in .venv'
  } else {
    Write-Err 'esptool is required for upload.ps1.'; exit 1
  }
}

function Init-StateFile {
  if (Test-Path $StateFile) { Write-Ok "State file already exists: $StateFile"; return }
  @'
MAJOR=0
MINOR=0
PATCH=0
BUILD_ID=0
LAST_BUILD_TS=0
'@ | Set-Content -LiteralPath $StateFile -Encoding ASCII
  Write-Ok "Created state file: $StateFile"
}

function Init-ReleaseMatrix {
  if (Test-Path $ReleaseMatrix) { Write-Ok "Release matrix already exists: $ReleaseMatrix"; return }
  "CHIP,LED_PIN_CLOCK,LED_PIN_DATA,_BUILD_NOTES" | Set-Content -LiteralPath $ReleaseMatrix -Encoding ASCII
  Write-Ok "Created release matrix: $ReleaseMatrix"
}

function Ensure-ProjectIno {
  if (Test-Path $InoFile) { Write-Ok "Sketch file found: $InoFile"; return }
  Write-Warn "No .ino found. Creating: $InoFile"
  @'
#include "Config.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Hello World");
}

void loop() {
}
'@ | Set-Content -LiteralPath $InoFile -Encoding ASCII
  Write-Ok "Sketch file ready: $InoFile"
}

function Ensure-ProjectConfigH {
  if (Test-Path $ConfigH) { Write-Ok "Config header found: $ConfigH"; return }
  Write-Warn "Config.h not found. Creating: $ConfigH"
  @'
#ifndef CONFIG_H
#define CONFIG_H

// Project configuration goes here.
// Example:
// #define WIFI_SSID "your-ssid"
// #define WIFI_PASSWORD "your-password"

#endif // CONFIG_H
'@ | Set-Content -LiteralPath $ConfigH -Encoding ASCII
  Write-Ok "Config header ready: $ConfigH"
}

function Ensure-Gitignore {
  $gitignore = Join-Path $BuildRoot '.gitignore'
  $required = @('.venv/', 'builds/', 'build_config', 'build_config.ps1', 'version_state')
  if (-not (Test-Path $gitignore)) { New-Item -ItemType File -Force -Path $gitignore | Out-Null }
  $existing = Get-Content -LiteralPath $gitignore -ErrorAction SilentlyContinue
  foreach ($line in $required) {
    if ($existing -notcontains $line) { Add-Content -LiteralPath $gitignore -Value $line }
  }
  Write-Ok "gitignore ready: $gitignore"
}

function Write-BuildConfig {
  $arduinoCliPath = (Get-Command arduino-cli -ErrorAction SilentlyContinue).Source
  $gitPath        = (Get-Command git -ErrorAction SilentlyContinue).Source

  $data = @"
# Auto-generated by setup_build_environment.ps1
# Dot-source this file from compile/upload scripts:
#   . "`$BuildRoot\build_config.ps1"

`$BuildConfig = @{
  setup_success           = `$true

  project_name            = '$ProjectName'
  project_root            = '$ProjectRoot'
  project_ino_file        = '$InoFile'
  project_config_h_file   = '$ConfigH'

  build_root              = '$BuildRoot'
  builds_dir              = '$(Join-Path $BuildRoot 'builds')'
  builds_cache_dir        = '$(Join-Path $BuildRoot 'builds\cache')'
  builds_latest_dir       = '$(Join-Path $BuildRoot 'builds\latest')'
  build_state_file        = '$StateFile'

  venv_dir                = '$VenvDir'
  venv_python_bin         = '$VenvPython'
  venv_pip                = '$VenvPip'

  libraries_dir           = '$LibrariesDir'
  release_matrix_file     = '$ReleaseMatrix'

  arduino_cli             = '$arduinoCliPath'
  git_bin                 = '$gitPath'

  esp32_core_fqbn         = '$Esp32CoreFqbn'
  esp32_board_manager_url = '$Esp32BoardManagerUrl'
  build_config_file       = '$BuildConfigFile'
}
"@

  $func = @'

function Get-Cfg {
  param([Parameter(Mandatory)][string]$Key)
  if ($BuildConfig.ContainsKey($Key)) {
    return $BuildConfig[$Key]
  }
  throw "missing key: $Key"
}
'@

  ($data + "`n" + $func) | Set-Content -LiteralPath $BuildConfigFile -Encoding UTF8
  Write-Ok "Wrote build config: $BuildConfigFile"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
Ensure-Git
Ensure-ArduinoCli
Ensure-Esp32Core
Ensure-Libraries
Ensure-Python
Ensure-Venv
Ensure-Esptool
Init-StateFile
Init-ReleaseMatrix
Ensure-ProjectIno
Ensure-ProjectConfigH
Ensure-Gitignore
Write-BuildConfig

Write-Host ''
Write-Ok 'Setup complete.'
Write-Host "   - arduino-cli: $((Get-Command arduino-cli).Source)"
Write-Host "   - python:      $((python --version) 2>&1)"
Write-Host "   - venv:        $VenvDir"
Write-Host "   - libraries:   $LibrariesDir"
Write-Host "   - config:      $BuildConfigFile"
Write-Host ''
Write-Host "Next: .\build.ps1 -Chip c3 [-Port COM5]"
