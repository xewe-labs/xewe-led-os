#Requires -Version 5.1
[CmdletBinding()]
param(
  [string[]]$Paths,   # optional: specific files/dirs; defaults to <project>/src
  [switch]$Check      # exit 1 if any file would change (CI mode), don't write
)

# format.ps1 — thin forwarder to the Python orchestrator
# (build/tools/code_formatter/format.py).

$ErrorActionPreference = 'Stop'

$ScriptDir  = $PSScriptRoot
$ToolsDir   = (Resolve-Path (Join-Path $ScriptDir '..\..\tools\code_formatter')).Path
$FormatPy   = Join-Path $ToolsDir 'format.py'

$cmd = Get-Command python -ErrorAction SilentlyContinue
if (-not $cmd) { throw "python not found on PATH." }
$Python = $cmd.Source

$fwd = @($FormatPy)
if ($Check) { $fwd += '--check' }
if ($Paths) { $fwd += $Paths }

& $Python @fwd
exit $LASTEXITCODE
