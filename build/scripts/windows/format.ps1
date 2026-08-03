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
$MethodScript = Join-Path $BuildRoot 'tools\method_order.py'
$InlineScript = Join-Path $BuildRoot 'tools\inline_move.py'
$ParamScript  = Join-Path $BuildRoot 'tools\param_split.py'
$CtorScript   = Join-Path $BuildRoot 'tools\ctor_brace.py'
$OpenBreakScript = Join-Path $BuildRoot 'tools\open_break.py'
$DangleScript = Join-Path $BuildRoot 'tools\dangling_close.py'
$ModeCfgScript = Join-Path $BuildRoot 'tools\modeconfig_layout.py'
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
    # Split multi-param definition signatures one-per-line and put ctor body
    # braces on their own line so a canonical file compares equal (clang-format
    # under ColumnLimit:0 does neither).
    & $Python $ParamScript --fix $tmp 2>$null | Out-Null
    & $Python $CtorScript --fix $tmp 2>$null | Out-Null
    # Break the first element off a multi-line opener so a canonical file
    # compares equal (clang-format under ColumnLimit:0 glues it to the opener).
    & $Python $OpenBreakScript --fix $tmp 2>$null | Out-Null
    # Dangle multi-line closers so a canonical file compares equal (clang-format
    # under ColumnLimit:0 glues them to the last arg line). On .h only inline
    # function bodies are affected.
    & $Python $DangleScript --fix $tmp 2>$null | Out-Null
    # Reproduce the ModeConfig shallow relayout on the temp so a correctly
    # formatted source compares equal (clang-format alone deep-indents tables).
    & $Python $ModeCfgScript --fix $tmp 2>$null | Out-Null
    if ([IO.File]::ReadAllText($f) -ne [IO.File]::ReadAllText($tmp)) { $changed += $f }
    Remove-Item -LiteralPath $tmp -Force
  }
  # Structural checks run on the real files (they read across the .h/.cpp pair,
  # which the per-file mangled temp copies above cannot represent). inline_move
  # --check catches un-moved inline .h bodies (clang-format leaves them and
  # align_decls skips the braced line, so the temp diff cannot see them).
  $lintFail = $false
  if ($headers) {
    & $Python $InlineScript --check @headers
    if ($LASTEXITCODE -ne 0) { $lintFail = $true }
  }
  if ($sources) {
    & $Python $MethodScript --check @sources
    if ($LASTEXITCODE -ne 0) { $lintFail = $true }
  }
  if ($changed.Count -gt 0 -or $lintFail) {
    if ($changed.Count -gt 0) {
      Write-Host "Would reformat:" -ForegroundColor Yellow
      $changed | ForEach-Object { Write-Host "  $_" }
    }
    exit 1
  }
  Write-Host "All files already formatted." -ForegroundColor Green
  exit 0
}

# --- 1. Structural passes (run BEFORE clang-format) ---------------------------
# Move qualifying inline member-function bodies out of each .h into its sibling
# .cpp. This must precede method_order: an inline body has no depth-0 ';', so the
# member is invisible to method_order's header parser until it becomes a plain
# declaration here.
if ($headers) {
  Write-Host "inline_move: $($headers.Count) header(s)..." -ForegroundColor Cyan
  & $Python $InlineScript --fix @headers
}

# Reorder each .cpp's out-of-line definitions to match its sibling .h (now that
# the moved members participate in the .h declaration order).
if ($sources) {
  Write-Host "method_order: $($sources.Count) source(s)..." -ForegroundColor Cyan
  & $Python $MethodScript --fix @sources
}

# --- 2. Normalize whitespace/braces everywhere --------------------------------
Write-Host "clang-format: $($targets.Count) file(s)..." -ForegroundColor Cyan
& $ClangFormat -i --style=$StyleArg @targets

# --- 3. Cosmetic passes (depend on clang-format's output) ---------------------
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

# Split every multi-param function DEFINITION one parameter per line (clang-format
# under ColumnLimit:0 never wraps) and collapse the name-column padding, then put
# each constructor's body '{' on its own line (clang-format attaches it to the
# last member initializer). Both run on headers and sources.
Write-Host "param_split: $($targets.Count) file(s)..." -ForegroundColor Cyan
& $Python $ParamScript --fix @targets
Write-Host "ctor_brace: $($targets.Count) file(s)..." -ForegroundColor Cyan
& $Python $CtorScript --fix @targets

# Break the first element off every multi-line opener (clang-format under
# ColumnLimit:0 + AlignAfterOpenBracket:DontAlign glues it to the opener line and
# block-indents the rest). Restricted to bracket groups inside function bodies;
# declaration signature param lists are owned by align_decls/param_split. Runs
# before dangling_close, which then dangles the matching closer.
Write-Host "open_break: $($targets.Count) file(s)..." -ForegroundColor Cyan
& $Python $OpenBreakScript --fix @targets

# Put multi-line bracket-group closers on their own line (clang-format under
# ColumnLimit:0 glues them onto the last argument line). Runs on headers too, but
# there only inside inline function bodies (declaration signatures stay glued —
# align_decls owns those). Before modeconfig_layout, which owns the ModeConfig
# table's own close line.
Write-Host "dangling_close: $($targets.Count) file(s)..." -ForegroundColor Cyan
& $Python $DangleScript --fix @targets

# Last: rewrite each ModeConfig table into the shallow layout clang-format
# would otherwise deep-indent.
if ($sources) {
  Write-Host "modeconfig_layout: $($sources.Count) source(s)..." -ForegroundColor Cyan
  & $Python $ModeCfgScript --fix @sources
}

Write-Host "Done." -ForegroundColor Green
