#Requires -Version 5.1
[CmdletBinding()]
param(
  [string[]]$Paths,   # optional: specific files/dirs; defaults to <project>/src
  [switch]$Check      # exit 1 if any file would change (CI mode), don't write
)

$ErrorActionPreference = 'Stop'

$ScriptDir   = $PSScriptRoot
$BuildRoot   = (Resolve-Path (Join-Path $ScriptDir '..\..')).Path
$ProjectRoot = (Resolve-Path (Join-Path $ScriptDir '..\..\..')).Path
$AlignScript  = Join-Path $BuildRoot 'tools\align_decls.py'
$HeaderScript = Join-Path $BuildRoot 'tools\header_layout.py'
$StyleFile    = Join-Path $BuildRoot 'tools\.clang-format'
$StyleArg     = "file:$StyleFile"

function Find-ClangFormat {
  $cmd = Get-Command clang-format -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }
  $fallback = 'C:\Program Files\LLVM\bin\clang-format.exe'
  if (Test-Path $fallback) { return $fallback }
  throw "clang-format not found on PATH or at $fallback. Install LLVM (winget install LLVM.LLVM)."
}

function Find-Python {
  $cmd = Get-Command python -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }
  throw "python not found on PATH."
}

$ClangFormat = Find-ClangFormat
$Python      = Find-Python

if (-not $Paths -or $Paths.Count -eq 0) {
  $Paths = @((Join-Path $ProjectRoot 'src'))
}

# Collect .h and .cpp under the given paths (files passed directly are kept as-is).
$targets = New-Object System.Collections.Generic.List[string]
foreach ($p in $Paths) {
  if (Test-Path $p -PathType Leaf) {
    $targets.Add((Resolve-Path $p).Path)
  } elseif (Test-Path $p -PathType Container) {
    Get-ChildItem -Path $p -Recurse -Include *.h, *.cpp -File |
      ForEach-Object { $targets.Add($_.FullName) }
  } else {
    Write-Warning "Path not found: $p"
  }
}

if ($targets.Count -eq 0) { Write-Host "No files to format."; exit 0 }

# Wrap in @() so a single match stays an array — splatting a scalar string
# with @headers would otherwise enumerate it character-by-character.
$headers = @($targets | Where-Object { $_ -like '*.h' })
$sources = @($targets | Where-Object { $_ -like '*.cpp' })

function Get-RelPath([string]$f) {
  ((Resolve-Path -LiteralPath $f).Path.Substring($ProjectRoot.Length).TrimStart('\', '/')) -replace '\\', '/'
}

if ($Check) {
  $changed = @()
  foreach ($f in $targets) {
    # Temp keeps the source's extension so clang-format detects C++; the config
    # is passed explicitly via --style=file:<path>.
    $tmp = Join-Path (Split-Path -Parent $f) (".fmtcheck.$PID." + (Split-Path -Leaf $f))
    Copy-Item -LiteralPath $f -Destination $tmp -Force
    & $ClangFormat -i --style=$StyleArg $tmp
    $rel = Get-RelPath $f
    if ($f -like '*.h') {
      & $Python $AlignScript $tmp | Out-Null
      & $Python $HeaderScript --emit-path $rel $tmp | Out-Null
    } elseif ($f -like '*.cpp') {
      # No sibling .h beside the mangled temp, so the cross-file include move is
      # skipped here; check still catches license/path/ordering drift.
      & $Python $HeaderScript --emit-path $rel $tmp 2>$null | Out-Null
    }
    if ([IO.File]::ReadAllText($f) -ne [IO.File]::ReadAllText($tmp)) { $changed += $f }
    Remove-Item -LiteralPath $tmp -Force
  }
  if ($changed.Count -gt 0) {
    Write-Host "Would reformat:" -ForegroundColor Yellow
    $changed | ForEach-Object { Write-Host "  $_" }
    exit 1
  }
  Write-Host "All files already formatted." -ForegroundColor Green
  exit 0
}

Write-Host "clang-format: $($targets.Count) file(s)..." -ForegroundColor Cyan
& $ClangFormat -i --style=$StyleArg @targets

# .cpp first: a source may relocate third-party <...> includes into its sibling
# header, which the header pass below then re-normalizes.
if ($sources) {
  Write-Host "header_layout: $($sources.Count) source(s)..." -ForegroundColor Cyan
  & $Python $HeaderScript --root $ProjectRoot @sources
}

if ($headers) {
  Write-Host "align_decls: $($headers.Count) header(s)..." -ForegroundColor Cyan
  & $Python $AlignScript @headers
  Write-Host "header_layout: $($headers.Count) header(s)..." -ForegroundColor Cyan
  & $Python $HeaderScript --root $ProjectRoot @headers
}

Write-Host "Done." -ForegroundColor Green
