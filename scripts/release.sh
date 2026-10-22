#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# scripts/release.sh
#
# release.sh: test the firmware in one recorded run, write the release card, then tag; CI builds the
# images and publishes the GitHub Release.
# Usage: scripts/release.sh --prepare <version> [--hardware c3,c6,s3] [--resume] [--dry-run] | <version> [--dry-run] [--ci-timeout <min>] [--skip-ci]
#
# This file is the template's, and every firmware project made from the template keeps a copy.
# A project changes exactly two lines: REPO (its GitHub repository) and BOARD_GATES (the gated
# board tests its card session runs). The project name comes from xewe.toml [project] name, which
# must equal REPO's name.
#
# A release is two runs with one commit in between. Tags are X.Y.Z with no `v`.
# `scripts/release.sh --prepare <version>` (8 stages) makes no git change:
#   1 preflight: refuses a detached HEAD, a merge/rebase/cherry-pick/revert in progress, unmerged
#     paths, any change outside the version lines (VERSION_LOCATIONS) and releases/<version>/, a
#     releases/<version>/testcard.md, a .gitignore that does not ignore releases/*/logs/, and any
#     tracked file under static/ (images are release assets only, written there at release
#     time), and an active INJECTED_FROM_DOTENV line in Config.h (`xewe release` refuses it, so
#     the run stops before the card run); checks that ci.yml calls the tools workflows at the
#     [tools] ref of xewe.toml (a tag, unless XEWE_TOOLS_SOURCE is set) and finds Python >= 3.11;
#   2 versions: every VERSION_LOCATIONS line set to <version>;
#   3 release notes: releases/<version>/notes.txt (the GitHub Release text and the tag message); a
#     missing one gets a skeleton, and while its skeleton marker line is there the run stops with
#     exit 2 (fill it in, run again);
#   4 card run: a venv with the tools (build/release-venv/card), then `xewe release-card init` on
#     releases/<version>/logs/ (gitignored);
#   5 setup (recorded): ./setup.sh (XEWE_CORE_SOURCE, XEWE_MODULES_SOURCE and XEWE_TOOLS_SOURCE
#     passed through when set), then the checkers (ruff, clang-format) into the project's venv;
#   6 checks (recorded): `xewe modules validate` (validate), `xewe test --unit-only` (unit, JUnit);
#   7 chips (recorded, c3, c6, s3): `xewe build --chip C` (build); for a --hardware chip `xewe
#     flash --chip C --no-build --require-board` (flash) and `xewe test --chip C --require-board --
#     -m board` (board, JUnit, the BOARD_GATES set: one session per chip); for any other chip `xewe
#     test --chip C --no-board -- -m board` (board: compiled, not run). The defines of a chip's
#     build, flash and board steps come from XEWE_RELEASE_DEFINES_<CHIP> (environment, space
#     separated KEY=VALUE, e.g. XEWE_RELEASE_DEFINES_S3="XEWE_LED_PIN_DATA=48"); then
#     `xewe format --check` and `xewe brand-lint .` (check);
#   8 card: `xewe release-card render` writes releases/<version>/release_card.md, or refuses (the
#     draft is in the logs folder); prints the commit command.
# A failing step does not stop the others; the card refuses it. --resume (after a failed step)
# keeps the run when commit and tree are unchanged and re-runs only the steps that are not green
# (a chip whose image is rebuilt is flashed and tested again). A --hardware board must have had
# its first boot (`xewe provision --chip C`); flashing keeps NVS, so never erase it before a run.
# The board steps read the dotenv file (XEWE_ENV, else .env): the tools read it, this script never
# does. Then commit what it lists as "prepare <version>" and push.
# `scripts/release.sh <version>` (5 stages), the tag run:
#   1 preflight: on main, clean, level with origin/main after a fetch; tag free here and on origin;
#     notes.txt (no skeleton marker) and release_card.md committed; no testcard.md; every version
#     location says <version>; ci.yml at the [tools] ref; no tracked firmware; gh logged in;
#   2 release card: `xewe release-card verify` (the card as rendered, its tested tree HEAD's clean
#     tree, the local logs present with their sha256; read-only, also in a dry run);
#   3 CI gate on HEAD: the newest ci.yml run for HEAD on main must succeed (waits; --ci-timeout
#     minutes, default 30);
#   4 annotated tag <version> on HEAD with notes.txt as its message, pushed;
#   5 release CI: waits for ci.yml on the tag. Its release job (xewe-release.yml) builds every
#     image per chip, checks the card against its own runs (`xewe release-card verify --ci`) and
#     creates the GitHub Release; this script never creates it. Then it checks that the card check
#     step passed and that the release has every asset: per image row the .bin, meta and manifest,
#     plus firmware_map.csv, the tarball and release_card.md.
# Re-running resumes: when the tag exists (here or on origin), stage 5 runs alone (pushing the tag
# first if origin lacks it), provided the tagged commit's files say <version>. --skip-ci skips both
# CI waits (emergencies only; stage 5 then checks what is there). --dry-run prints every command and
# changes nothing: only read-only git, gh and tool lookups and `release-card verify` run; it reports
# every failed check, never waits, and exits 1 if a check failed.
# Exit codes: 0 done, 1 a refusal or a failed check, 2 notes.txt to fill.
set -euo pipefail

REPO="xewe-labs/xewe-led-os"                         # a project: its repository
BOARD_GATES=(XEWE_TEST_SCHEDULE_PAGE_HTTP=1 XEWE_TEST_ALEXA_HTTP=1)    # a project: its gated board tests, e.g. (XEWE_TEST_X_HTTP=1)

CI_WORKFLOW="ci.yml"
CI_FILE=".github/workflows/ci.yml"
CI_RELEASE_JOB="release / assemble"                  # ci.yml job id / xewe-release.yml job
CI_CARD_STEP="Release card check"                    # its `xewe release-card verify --ci` step
CI_POLL="${RELEASE_CI_POLL:-15}"                     # seconds between CI polls
VENVS="build/release-venv"                           # gitignored (build/)
PY="build/tools/.venv/bin/python"                    # the project's venv (./setup.sh)
CHIPS=(c3 c6 s3)
REQUIRE="setup,validate,unit,build,board"
CHECKERS=("ruff==0.17.0" "clang-format==23.1.3")    # the format pins of the core and modules CI
FIRMWARE="static"                                    # nothing in git (images are release assets)
NOTES_MARKER="XEWE-NOTES-SKELETON"

# Every place that states the project's own version: "file|ERE|replacement", {V} = the version.
# Each pattern must match at least one line; --prepare rewrites the matches. ERE is portable (GNU
# and BSD sed -E); no `|` inside.
VERSION_LOCATIONS=(
  'xewe.toml|^version = "[^"]*"$|version = "{V}"'
)

die() { echo "error: $*" >&2; exit 1; }
warn() { echo "warning: $*" >&2; }
STAGES=5
stage() { echo "== $1/${STAGES} $2"; }

# A failed preflight check: fatal in a real run; a dry run notes it and goes on.
FAILED=0
fail() {
  [[ "${DRY_RUN}" == 1 ]] || die "$*"
  echo "error: $*" >&2
  FAILED=$((FAILED + 1))
}

# Runs a command, or only prints it with --dry-run.
run() {
  if [[ "${DRY_RUN}" == 1 ]]; then
    printf '+'; printf ' %q' "$@"; printf '\n'
  else
    "$@"
  fi
}

# toml_value SECTION KEY: the quoted string KEY = "..." of [SECTION] in xewe.toml.
toml_value() {
  awk -v sec="[$1]" -v key="$2" '
    /^\[/ { in_sec = ($0 == sec) }
    in_sec && $0 ~ "^" key " *= *\"" { sub("^[^\"]*\"", ""); sub("\".*$", ""); print; exit }' xewe.toml
}

# file_at REF FILE: FILE's content at commit REF, or in the working tree when REF is empty.
file_at() {
  if [[ -z "$1" ]]; then cat -- "$2"; else git show "$1:$2"; fi
}

# set_location LOCATION VERSION REF: print the file of LOCATION (at REF, see file_at) with the
# location set to VERSION; exit 1 when the pattern matches no line.
set_location() {
  local file pattern repl rest
  file="${1%%|*}"; rest="${1#*|}"
  pattern="${rest%%|*}"; repl="${rest#*|}"
  repl="${repl//\{V\}/$2}"
  # grep reads all input (no -q): an early exit would SIGPIPE git show and fail the pipeline.
  file_at "$3" "${file}" 2>/dev/null | grep -E -- "${pattern}" >/dev/null || return 1
  file_at "$3" "${file}" | sed -E "s|${pattern}|${repl}|g"
}

# stale_locations VERSION REF: print "file: line" for every location (at REF) that does not say
# VERSION yet, or whose pattern matches nothing.
stale_locations() {
  local loc file
  for loc in "${VERSION_LOCATIONS[@]}"; do
    file="${loc%%|*}"
    if ! set_location "${loc}" "$1" "$2" >/dev/null; then
      echo "${file}: no line matches $(cut -d'|' -f2 <<< "${loc}")"
    elif ! cmp -s <(file_at "$2" "${file}") <(set_location "${loc}" "$1" "$2"); then
      echo "${file}: $(file_at "$2" "${file}" | grep -Em1 -- "$(cut -d'|' -f2 <<< "${loc}")")"
    fi
  done
}

# versioned_file FILE VERSION: FILE at HEAD with every location of FILE set to VERSION.
versioned_file() {
  local loc content
  content="$(git show "HEAD:$1" 2>/dev/null)" || return 1
  for loc in "${VERSION_LOCATIONS[@]}"; do
    [[ "${loc%%|*}" == "$1" ]] || continue
    content="$(printf '%s\n' "${content}" | set_location_stdin "${loc}" "$2")"
  done
  printf '%s\n' "${content}"
}

# set_location_stdin LOCATION VERSION: standard input with the location set to VERSION.
set_location_stdin() {
  local rest pattern repl
  rest="${1#*|}"
  pattern="${rest%%|*}"; repl="${rest#*|}"
  repl="${repl//\{V\}/$2}"
  sed -E "s|${pattern}|${repl}|g"
}

# allowed_change PATH VERSION: true for a path --prepare may find changed: anything under
# releases/VERSION/, and a version file whose only change from HEAD is its version lines.
allowed_change() {
  local loc
  [[ "$1" == "releases/$2/"* ]] && return 0
  for loc in "${VERSION_LOCATIONS[@]}"; do
    if [[ "${loc%%|*}" == "$1" ]]; then
      [[ -f "$1" ]] && cmp -s "$1" <(versioned_file "$1" "$2")
      return
    fi
  done
  return 1
}

# tracked_firmware: files git tracks under FIRMWARE (D26: none).
tracked_firmware() {
  git ls-files -- "${FIRMWARE}"
}

# tree_problems VERSION: one line per reason the work tree cannot be prepared; none when it can.
tree_problems() {
  local v="$1" p path entry code unmerged fw dirty=()
  git symbolic-ref -q HEAD >/dev/null || echo "HEAD is detached: check out the branch to release from"
  for p in MERGE_HEAD CHERRY_PICK_HEAD REVERT_HEAD REBASE_HEAD rebase-merge rebase-apply; do
    path="$(git rev-parse --git-path "${p}")"
    [[ -e "${path}" ]] && echo "a merge, rebase, cherry-pick or revert is in progress (${path} exists): finish or abort it first"
  done
  unmerged="$(git diff --name-only --diff-filter=U)"
  [[ -z "${unmerged}" ]] || echo "unmerged (conflicted) paths: $(tr '\n' ' ' <<< "${unmerged}")"
  # porcelain v1 -z: "XY path", a rename or copy followed by its original path as its own field
  while IFS= read -r -d '' entry; do
    code="${entry:0:2}"; path="${entry:3}"
    allowed_change "${path}" "${v}" || dirty+=("${path}")
    if [[ "${code}" == *[RC]* ]]; then
      IFS= read -r -d '' path || true
      allowed_change "${path}" "${v}" || dirty+=("${path}")
    fi
  done < <(git status --porcelain=v1 -z --untracked-files=all)
  if [[ "${#dirty[@]}" -gt 0 ]]; then
    echo "changes outside the version lines and releases/${v}/ (commit or stash them first): ${dirty[*]}"
  fi
  [[ ! -e "releases/${v}/testcard.md" ]] \
    || echo "releases/${v}/testcard.md exists: the generated release_card.md replaces it; delete it"
  git check-ignore -q "releases/${v}/logs/run.json" \
    || echo ".gitignore does not ignore releases/*/logs/ (the run folder must never be committed)"
  fw="$(tracked_firmware)"
  [[ -z "${fw}" ]] \
    || echo "${FIRMWARE}/ tracks files (images are release assets only, D26; git rm -r --cached them): $(tr '\n' ' ' <<< "${fw}")"
  # an active sentinel takes a value from the dotenv file, and `xewe release` refuses that Config.h
  [[ ! -f Config.h ]] || grep -nE '^[[:space:]]*#[[:space:]]*define[[:space:]]+[[:alnum:]_]+[[:space:]]+INJECTED_FROM_DOTENV' Config.h \
    | sed -E 's/^([0-9]+):[[:space:]]*(.*)$/Config.h:\1: active INJECTED_FROM_DOTENV line, a release image takes no dotenv value (comment it out or write the value): \2/' \
    || true
}

# workflow_refs: print "<ref of the tools workflows in ci.yml> <[tools] ref of xewe.toml>".
workflow_refs() {
  local wf
  wf="$(sed -nE 's|.*xewe-os-tools/[.]github/workflows/[a-z-]+[.]yml@([^[:space:]]+).*|\1|p' "${CI_FILE}" | sort -u | tr '\n' ' ')"
  echo "${wf% } ${TOOLS_REF}"
}

# refs_problem: why ci.yml and xewe.toml do not name one tools tag (nothing when they do).
refs_problem() {
  local refs
  refs="$(workflow_refs)"
  if [[ "${refs% *}" != "${refs#* }" ]]; then
    echo "${CI_FILE} calls the tools workflows at ${refs% *}, xewe.toml [tools] ref is ${TOOLS_REF}"
  elif [[ ! "${TOOLS_REF}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "xewe.toml [tools] ref is ${TOOLS_REF:-unset}, not a tools tag (X.Y.Z): the card and CI must use the same tools"
  fi
}

# find_python: python3.12 (CI's release jobs), else the first python3 >= 3.11 on PATH.
find_python() {
  local cand p
  for cand in python3.12 python3 python3.13 python3.14 python3.11; do
    p="$(command -v "${cand}" 2>/dev/null)" || continue
    if "${p}" -c 'import sys; sys.exit(sys.version_info < (3, 11))' 2>/dev/null; then
      echo "${p}"
      return 0
    fi
  done
  return 0
}

tag_local() { git rev-parse -q --verify "refs/tags/$1" >/dev/null; }
tag_remote() { git ls-remote --exit-code --tags origin "refs/tags/$1" >/dev/null 2>&1; }

# tag_commit VERSION: the commit the tag points at (local tag first, else origin's peeled tag).
tag_commit() {
  if tag_local "$1"; then
    git rev-parse "refs/tags/$1^{commit}"
  else
    git ls-remote --tags origin \
      | awk -v t="refs/tags/$1" '$2 == t { c = $1 } $2 == t "^{}" { p = $1 } END { print (p != "" ? p : c) }'
  fi
}

# ci_run SHA BRANCH: the newest run of CI_WORKFLOW for SHA on BRANCH (main, or the tag name for a
# tag push), as "id status url conclusion" (empty when there is none yet; conclusion is empty
# while the run is not completed).
ci_run() {
  gh run list --commit "$1" --workflow "${CI_WORKFLOW}" --branch "$2" --limit 20 \
    --json databaseId,status,url,conclusion \
    --jq '.[0] | select(. != null) | "\(.databaseId) \(.status) \(.url) \(.conclusion)"'
}

# card_step_result RUN_ID: the conclusion of the release job's card check step in that run.
card_step_result() {
  gh run view "$1" --json jobs \
    --jq ".jobs[] | select(.name == \"${CI_RELEASE_JOB}\") | .steps[] | select(.name == \"${CI_CARD_STEP}\") | .conclusion"
}

# ci_gate SHA BRANCH: wait for CI_WORKFLOW on SHA and BRANCH and fail unless it succeeds, within
# CI_TIMEOUT minutes. CI_RUN_ID is the run's id afterwards (empty when not known).
CI_RUN_ID=""
ci_gate() {
  local sha="$1" br="$2" deadline=$((SECONDS + CI_TIMEOUT * 60)) line id status url conclusion pid
  CI_RUN_ID=""
  if [[ "${SKIP_CI}" == 1 ]]; then
    warn "!!!!!!!! --skip-ci: CI on ${sha:0:12} (${br}) is NOT checked !!!!!!!!"
    return 0
  fi
  if [[ "${DRY_RUN}" == 1 ]]; then
    echo "+ gh run list --commit ${sha} --workflow ${CI_WORKFLOW} --branch ${br} ... (poll every ${CI_POLL}s until a run appears, up to ${CI_TIMEOUT} min)"
    echo "+ gh run watch <run id> --exit-status (if not completed; fail unless success)"
    if [[ "${GH_OK}" == 1 && "${sha}" =~ ^[0-9a-f]{40}$ ]]; then
      line="$(ci_run "${sha}" "${br}" 2>/dev/null)" || line="(gh run list failed)"
      echo "   current CI state of ${sha:0:12} on ${br}: ${line:-no ${CI_WORKFLOW} run yet}"
    fi
    return 0
  fi
  echo "   waiting for a ${CI_WORKFLOW} run on ${sha:0:12} (${br}, timeout ${CI_TIMEOUT} min)"
  while :; do
    line="$(ci_run "${sha}" "${br}")" || die "gh run list failed"
    [[ -n "${line}" ]] && break
    (( SECONDS < deadline )) || die "timed out after ${CI_TIMEOUT} min: no ${CI_WORKFLOW} run for ${sha:0:12} on ${br}"
    sleep "${CI_POLL}"
  done
  read -r id status url conclusion <<< "${line}"
  echo "   run ${url} (${status})"
  if [[ "${status}" != completed ]]; then
    gh run watch "${id}" --exit-status --interval "${CI_POLL}" >/dev/null &
    pid=$!
    while kill -0 "${pid}" 2>/dev/null; do
      if (( SECONDS >= deadline )); then
        kill "${pid}" 2>/dev/null || true
        die "timed out after ${CI_TIMEOUT} min, CI still running: ${url} (re-run to resume)"
      fi
      sleep 2
    done
    wait "${pid}" || die "CI failed: ${url}"
    conclusion=success
  fi
  [[ "${conclusion}" == success ]] || die "CI ${conclusion:-did not succeed}: ${url}"
  CI_RUN_ID="${id}"
  echo "   CI passed: ${url}"
}

# expected_assets: the number of GitHub Release assets xewe-release.yml publishes: three per image
# row of release_matrix.csv (bin, meta, manifest; one row per chip without a matrix), plus
# firmware_map.csv, the tarball and release_card.md.
expected_assets() {
  local py
  py="$(find_python)"
  [[ -n "${py}" ]] || { echo 0; return 0; }
  "${py}" -I - "${#CHIPS[@]}" <<'PY'
import csv, pathlib, sys
rows = int(sys.argv[1])
path = pathlib.Path("release_matrix.csv")
if path.is_file():
    with path.open(encoding="utf-8", newline="") as f:
        rows = sum(1 for r in list(csv.reader(f))[1:] if r and r[0])
print(3 * rows + 3)
PY
}

# release_assets VERSION: check the GitHub Release CI created: the card check passed, every asset
# is there, release_card.md among them; print them.
release_assets() {
  local v="$1" assets count want card line
  want="$(expected_assets)"
  if [[ "${DRY_RUN}" == 1 ]]; then
    echo "+ gh run view <tag run id> --json jobs (job '${CI_RELEASE_JOB}', step '${CI_CARD_STEP}' must be success)"
    echo "+ gh release view ${v} --json assets,url (expect ${want} assets incl. release_card.md)"
    if [[ "${GH_OK}" == 1 ]] && gh release view "${v}" >/dev/null 2>&1; then
      echo "   GitHub Release ${v} exists ($(gh release view "${v}" --json assets --jq '.assets | length') assets)"
    else
      echo "   no GitHub Release ${v} yet"
    fi
    return 0
  fi
  if [[ -n "${CI_RUN_ID}" ]]; then
    card="$(card_step_result "${CI_RUN_ID}")" || die "gh run view ${CI_RUN_ID} failed"
    [[ "${card}" == success ]] \
      || die "the tag's ${CI_WORKFLOW} run passed, but its '${CI_RELEASE_JOB}' step '${CI_CARD_STEP}' is ${card:-not found}"
    echo "   card check of the release job: ${card}"
  fi
  assets="$(gh release view "${v}" --json assets --jq '.assets[].name')" \
    || die "no GitHub Release ${v}: see the release job of the tag's ${CI_WORKFLOW} run"
  count="$(grep -c . <<< "${assets}" || true)"
  echo "   GitHub Release: $(gh release view "${v}" --json url --jq .url) (${count} assets, ${want} expected)"
  while IFS= read -r line; do echo "     ${line}"; done <<< "${assets}"
  grep -qx 'release_card.md' <<< "${assets}" || die "GitHub Release ${v} has no release_card.md asset"
  [[ "${count}" == "${want}" ]] || die "GitHub Release ${v} has ${count} assets, ${want} expected"
}

# tools_spec: what pip installs as the tools: XEWE_TOOLS_SOURCE (a local checkout), else the [tools]
# repo of xewe.toml at its ref.
tools_spec() {
  if [[ -n "${XEWE_TOOLS_SOURCE:-}" ]]; then echo "${XEWE_TOOLS_SOURCE}"; else echo "git+${TOOLS_URL}@${TOOLS_REF}"; fi
}

# card_tools PYTHON: the venv whose tools run `xewe release-card` (not recorded: it makes the card).
# XEWE is the command prefix.
card_tools() {
  local commit
  XEWE=("${VENVS}/card/bin/python" -m xewe)
  run "$1" -m venv --clear "${VENVS}/card"
  run "${VENVS}/card/bin/python" -m pip install -q --disable-pip-version-check --progress-bar off "$(tools_spec)"
  # the tools commit the card header names (the venv is not an editable install)
  if [[ "${DRY_RUN}" == 1 ]]; then
    echo "+ (tools commit) > ${VENVS}/card/xewe-tools-commit"
    return 0
  fi
  if [[ -n "${XEWE_TOOLS_SOURCE:-}" ]]; then
    commit="$(git -C "${XEWE_TOOLS_SOURCE}" rev-parse HEAD 2>/dev/null || echo -)"
  else
    commit="$(git ls-remote "${TOOLS_URL}" "refs/tags/${TOOLS_REF}^{}" "refs/tags/${TOOLS_REF}" "refs/heads/${TOOLS_REF}" \
      | awk '{ c[NR] = $1 } END { print (NR ? c[1] : "-") }')"
  fi
  echo "${commit:--}" > "${VENVS}/card/xewe-tools-commit"
}

# step LABEL RECORD_ARGS...: one recorded step; a failed command is noted and the run goes on (the
# card refuses it), a usage error stops. STEP_RESUME is what it passes as --resume.
FAILED_STEPS=()
step() {
  local label="$1" rc=0
  shift
  if [[ "${DRY_RUN}" == 1 ]]; then
    run "${XEWE[@]}" release-card record --dir "${LOGS}" ${STEP_RESUME[@]+"${STEP_RESUME[@]}"} "$@"
    return 0
  fi
  "${XEWE[@]}" release-card record --dir "${LOGS}" ${STEP_RESUME[@]+"${STEP_RESUME[@]}"} "$@" </dev/null || rc=$?
  case "${rc}" in
    0) ;;
    1|3) FAILED_STEPS+=("${label}"); warn "${label} failed (exit ${rc}; the log is in ${LOGS}); going on" ;;
    *) die "xewe release-card record exited ${rc} on ${label}" ;;
  esac
}

# step_count KIND CHIP: how many steps of KIND for CHIP run.json holds (0 without a run).
step_count() {
  [[ -f "${LOGS}/run.json" ]] || { echo 0; return 0; }
  "${VENVS}/card/bin/python" -I - "${LOGS}/run.json" "$1" "$2" <<'PY'
import json, sys
steps = json.load(open(sys.argv[1], encoding="utf-8")).get("steps", [])
print(sum(1 for s in steps if s.get("kind") == sys.argv[2] and s.get("chip") == sys.argv[3]))
PY
}

# chip_defines CHIP: `--define KEY=VALUE` words from XEWE_RELEASE_DEFINES_<CHIP> (environment;
# space separated), one per line.
chip_defines() {
  local var d
  var="XEWE_RELEASE_DEFINES_$(tr '[:lower:]' '[:upper:]' <<< "$1")"
  for d in ${!var-}; do printf '%s\n%s\n' --define "${d}"; done
}

notes_skeleton() {
  cat <<EOF
${NOTES_MARKER}: fill in this file, then delete this line (--prepare stops while it is here)
${NAME} $1

What the firmware is and what this release brings, in two or three sentences.

CHANGED SINCE <previous version>

- ...

IMAGES

- One merged image per chip (c3, c6, s3), written at 0x0, with its meta.json and ESP Web Tools
  manifest.json: GitHub Release assets of this release.

TESTING

What was tested, on which tree and with which result: release_card.md, generated by
scripts/release.sh --prepare $1 and attached to this release.
EOF
}

# prepare VERSION: the 8 stages of --prepare (see the header).
prepare() {
  local v="$1" dir="releases/$1" problems x py chip hw loc file rc before fw_bin
  local changed=() defines=() env_src=() chip_resume=()
  LOGS="${dir}/logs"
  STAGES=8

  stage 1 preflight
  problems="$(tree_problems "${v}")"
  if [[ -n "${problems}" ]]; then
    if [[ "${DRY_RUN}" == 1 ]]; then
      while IFS= read -r x; do fail "${x}"; done <<< "${problems}"
    else
      die "the work tree cannot be prepared:"$'\n'"${problems}"
    fi
  fi
  x="$(refs_problem)"
  if [[ -n "${x}" ]]; then
    if [[ -n "${XEWE_TOOLS_SOURCE:-}" && "${x}" == *"not a tools tag"* ]]; then
      warn "${x}; XEWE_TOOLS_SOURCE is used here"
    else
      fail "${x}"
    fi
  fi
  x="$(workflow_refs)"
  echo "   tools: $(tools_spec) (CI: the tools workflows at ${x% *})"
  for x in CORE MODULES TOOLS; do
    loc="XEWE_${x}_SOURCE"
    [[ -z "${!loc:-}" ]] || { echo "   ${loc}=${!loc} (passed to ./setup.sh)"; env_src+=("${loc}=${!loc}"); }
  done
  py="$(find_python)"
  [[ -n "${py}" ]] || die "Python >= 3.11 is required (python3.12 is what CI's release jobs use)"
  echo "   python: ${py}"
  # the dotenv the board steps use (the tools read it; this script only checks that it exists)
  if [[ -n "${XEWE_ENV:-}" ]]; then
    echo "   dotenv: ${XEWE_ENV}"
  elif [[ -f .env ]]; then
    echo "   dotenv: .env"
  else
    echo "   dotenv: none"
  fi
  echo "   hardware: ${HARDWARE:-none} (other chips: compiled, not run)"
  for chip in "${CHIPS[@]}"; do
    x="$(chip_defines "${chip}" | { grep -v '^--define$' || true; } | tr '\n' ' ')"
    [[ -z "${x}" ]] || echo "   ${chip} defines: ${x% }"
  done
  [[ -z "${HARDWARE}" ]] \
    || echo "   each --hardware board must have had its first boot (xewe provision --chip C); never erase it for the run"
  [[ "${FAILED}" -eq 0 ]] && echo "   ok: $(git symbolic-ref --short HEAD 2>/dev/null || echo '(detached)') at $(git rev-parse --short=12 HEAD)"

  stage 2 versions
  for loc in "${VERSION_LOCATIONS[@]}"; do
    file="${loc%%|*}"
    set_location "${loc}" "${v}" "" >/dev/null || die "${file}: no line matches $(cut -d'|' -f2 <<< "${loc}")"
    if ! cmp -s "${file}" <(set_location "${loc}" "${v}" ""); then
      if [[ "${DRY_RUN}" == 1 ]]; then
        [[ " ${changed[*]-} " == *" ${file} "* ]] || changed+=("${file}")
      else
        set_location "${loc}" "${v}" "" > "${file}.tmp"
        cat "${file}.tmp" > "${file}" # cat keeps the file's mode
        rm -f "${file}.tmp"
      fi
    fi
    # a version file that differs from HEAD goes into the prepare commit
    if ! git diff --quiet HEAD -- "${file}"; then
      [[ " ${changed[*]-} " == *" ${file} "* ]] || changed+=("${file}")
    fi
  done
  if [[ "${#changed[@]}" -gt 0 ]]; then
    echo "   version ${v}: ${changed[*]}$([[ "${DRY_RUN}" == 1 ]] && echo ' (would change)')"
  else
    echo "   version ${v}: every location already says it"
  fi

  stage 3 "release notes"
  if [[ ! -s "${dir}/notes.txt" ]]; then
    echo "   create ${dir}/notes.txt (skeleton)"
    if [[ "${DRY_RUN}" == 1 ]]; then
      echo "   a real run stops here (exit 2) until ${dir}/notes.txt is filled"
    else
      mkdir -p "${dir}"
      notes_skeleton "${v}" > "${dir}/notes.txt"
      echo "error: fill ${dir}/notes.txt (the GitHub Release text and the tag message, embedded in the card), delete its ${NOTES_MARKER} line, then run scripts/release.sh --prepare ${v} again" >&2
      exit 2
    fi
  elif grep -q -- "${NOTES_MARKER}" "${dir}/notes.txt"; then
    if [[ "${DRY_RUN}" == 1 ]]; then
      echo "   ${dir}/notes.txt is still the skeleton: a real run stops here (exit 2)"
    else
      echo "error: ${dir}/notes.txt is still the skeleton (its ${NOTES_MARKER} line): fill it, delete that line, run again" >&2
      exit 2
    fi
  else
    echo "   ${dir}/notes.txt: filled ($(wc -l < "${dir}/notes.txt" | tr -d ' ') lines)"
  fi

  stage 4 "card run"
  card_tools "${py}"
  run "${XEWE[@]}" release-card init --dir "${LOGS}" --repo "${REPO}" --version "${v}" --tag "${v}" \
    ${RESUME_ARGS[@]+"${RESUME_ARGS[@]}"}

  stage 5 setup
  STEP_RESUME=(${RESUME_ARGS[@]+"${RESUME_ARGS[@]}"})
  # ./setup.sh installs the tools of [tools] (or XEWE_TOOLS_SOURCE), the shared toolchain, the core,
  # the modules and the libraries of xewe.toml (or the local sources), no module menu (stdin closed)
  step setup --kind setup --name setup -- env -u XEWE_PROJECT XEWE_NO_BOARD=1 ${env_src[@]+"${env_src[@]}"} ./setup.sh
  step checkers --kind setup --name checkers -- "${PY}" -m pip install -q --disable-pip-version-check \
    --progress-bar off "${CHECKERS[@]}"

  stage 6 checks
  step validate --kind validate --name modules -- env XEWE_NO_BOARD=1 "${PY}" -m xewe modules validate
  step unit --kind unit --name unit --junit "${VENVS}/junit-unit.xml" -- env XEWE_NO_BOARD=1 "${PY}" -m xewe \
    test --unit-only -- "--junitxml=${ROOT}/${VENVS}/junit-unit.xml" -o junit_family=xunit1 -o junit_logging=no -ra

  stage 7 chips
  for chip in "${CHIPS[@]}"; do
    hw=0
    [[ ",${HARDWARE}," == *",${chip},"* ]] && hw=1
    defines=()
    while IFS= read -r x; do defines+=("${x}"); done < <(chip_defines "${chip}")
    STEP_RESUME=(${RESUME_ARGS[@]+"${RESUME_ARGS[@]}"})
    before=0
    [[ "${DRY_RUN}" == 1 ]] || before="$(step_count build "${chip}")"
    step "build ${chip}" --kind build --chip "${chip}" --name image -- env XEWE_NO_BOARD=1 "${PY}" -m xewe \
      build --chip "${chip}" ${defines[@]+"${defines[@]}"}
    # a rebuilt image (resume) is a new firmware sha: flash and test it again
    chip_resume=(${RESUME_ARGS[@]+"${RESUME_ARGS[@]}"})
    if [[ "${DRY_RUN}" != 1 && "$(step_count build "${chip}")" != "${before}" ]]; then
      chip_resume=()
    fi
    STEP_RESUME=(${chip_resume[@]+"${chip_resume[@]}"})
    if [[ "${hw}" == 1 ]]; then
      fw_bin="build/builds/${chip}/out/<image>.bin"
      if [[ "${DRY_RUN}" != 1 ]]; then
        fw_bin="$(find "build/builds/${chip}/out" -maxdepth 1 -name "*-${chip}-*.bin" ! -name '*bootloader*' \
          ! -name '*partitions*' 2>/dev/null | head -n 1)"
        [[ -n "${fw_bin}" ]] || fw_bin="build/builds/${chip}/out/missing.bin"
      fi
      step "flash ${chip}" --kind flash --chip "${chip}" --firmware "${fw_bin}" -- env -u XEWE_NO_BOARD "${PY}" -m xewe \
        flash --chip "${chip}" --no-build --require-board ${defines[@]+"${defines[@]}"}
      step "board ${chip}" --kind board --chip "${chip}" --junit "${VENVS}/junit-board-${chip}.xml" \
        -- env -u XEWE_NO_BOARD ${BOARD_GATES[@]+"${BOARD_GATES[@]}"} "${PY}" -m xewe test --chip "${chip}" \
        --require-board ${defines[@]+"${defines[@]}"} -- -m board -o junit_family=xunit1 -o junit_logging=no -ra \
        "--junitxml=${ROOT}/${VENVS}/junit-board-${chip}.xml"
    else
      step "board ${chip}" --kind board --chip "${chip}" --junit "${VENVS}/junit-board-${chip}.xml" \
        -- env XEWE_NO_BOARD=1 ${BOARD_GATES[@]+"${BOARD_GATES[@]}"} "${PY}" -m xewe test --chip "${chip}" \
        --no-board ${defines[@]+"${defines[@]}"} -- -m board -o junit_family=xunit1 -o junit_logging=no -ra \
        "--junitxml=${ROOT}/${VENVS}/junit-board-${chip}.xml"
    fi
  done
  STEP_RESUME=(${RESUME_ARGS[@]+"${RESUME_ARGS[@]}"})
  step format --kind check --name format -- env "XEWE_CLANG_FORMAT=${ROOT}/build/tools/.venv/bin/clang-format" \
    "${PY}" -m xewe format --check
  step brand-lint --kind check --name brand-lint -- "${PY}" -m xewe brand-lint .
  if [[ "${#FAILED_STEPS[@]}" -gt 0 ]]; then
    warn "failed steps: ${FAILED_STEPS[*]} (the card will refuse them)"
  fi

  stage 8 card
  rc=0
  run "${XEWE[@]}" release-card render --dir "${LOGS}" --notes "${dir}/notes.txt" --out "${dir}/release_card.md" \
    --require "${REQUIRE}" --chips "$(IFS=,; echo "${CHIPS[*]}")" ${HARDWARE:+--hardware "${HARDWARE}"} \
    --title "${NAME}" || rc=$?
  case "${rc}" in
    0) ;;
    1) die "no card: the reasons are above (draft: ${LOGS}/release_card.draft.md). Fix the cause, then: scripts/release.sh --prepare ${v}${HARDWARE:+ --hardware ${HARDWARE}} --resume" ;;
    *) die "xewe release-card render exited ${rc}" ;;
  esac
  if [[ "${DRY_RUN}" == 1 ]]; then
    echo "   dry run: nothing was prepared; a real run ends with:"
  else
    echo "   prepared in ${SECONDS} s; no git change was made. Review ${dir}/release_card.md, then commit and push:"
  fi
  echo "  git add ${changed[*]+${changed[*]} }${dir}/notes.txt ${dir}/release_card.md && git commit -m \"prepare ${v}\" && git push origin $(git symbolic-ref --short HEAD 2>/dev/null || echo '<branch>')"
  echo "then, once on main with ${CI_WORKFLOW} green: scripts/release.sh ${v} --dry-run; scripts/release.sh ${v}"
  if [[ "${DRY_RUN}" == 1 && "${FAILED}" -gt 0 ]]; then
    echo "== dry run: ${FAILED} check(s) failed; a real run stops at the first"
    exit 1
  fi
}

usage() { sed -n 's/^# Usage: //p' "${BASH_SOURCE[0]}"; }

main() {
  VERSION=""
  DRY_RUN=0
  SKIP_CI=0
  PREPARE=0
  RESUME_ARGS=()
  STEP_RESUME=()
  XEWE=()
  HARDWARE=""
  CI_TIMEOUT=30
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --dry-run) DRY_RUN=1 ;;
      --skip-ci) SKIP_CI=1 ;;
      --prepare) PREPARE=1 ;;
      --resume) RESUME_ARGS=(--resume) ;;
      --hardware)
        [[ "${2:-}" =~ ^(c3|c6|s3)(,(c3|c6|s3))*$ ]] || die "--hardware needs a list of c3, c6, s3 ($(usage))"
        HARDWARE="$2"
        shift
        ;;
      --ci-timeout)
        [[ "${2:-}" =~ ^[0-9]+$ ]] || die "--ci-timeout needs whole minutes ($(usage))"
        CI_TIMEOUT="$2"
        shift
        ;;
      -h|--help) usage; exit 0 ;;
      -*) die "unknown option $1 ($(usage))" ;;
      *)
        [[ -z "${VERSION}" ]] || die "one version only ($(usage))"
        VERSION="$1"
        ;;
    esac
    shift
  done
  [[ -n "${VERSION}" ]] || die "no version given ($(usage))"
  [[ "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] \
    || die "version must be X.Y.Z with no 'v' (the ecosystem's tags are un-prefixed): ${VERSION}"
  if [[ "${PREPARE}" == 1 ]]; then
    [[ "${SKIP_CI}" == 0 ]] || die "--skip-ci does not apply to --prepare"
  else
    [[ "${#RESUME_ARGS[@]}" -eq 0 ]] || die "--resume applies to --prepare only (the tag run resumes by itself)"
    [[ -z "${HARDWARE}" ]] || die "--hardware applies to --prepare only"
  fi

  # Repo root: the top of the git work tree this is run in.
  command -v git >/dev/null 2>&1 || die "git is required"
  ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || die "not inside a git work tree"
  cd "${ROOT}"
  [[ -f xewe.toml && -f setup.sh ]] || die "${ROOT} is not a firmware project (xewe.toml, setup.sh)"
  NAME="$(toml_value project name)"
  [[ "${NAME}" == "${REPO#*/}" ]] \
    || die "xewe.toml [project] name is \"${NAME}\", REPO is ${REPO}: set REPO at the top of this script"
  TOOLS_URL="$(toml_value tools repo)"
  TOOLS_URL="${TOOLS_URL:-https://github.com/xewe-labs/xewe-os-tools}"
  TOOLS_REF="$(toml_value tools ref)"
  [[ "${DRY_RUN}" == 1 ]] && echo "== dry run: nothing is changed; commands are printed with a leading +"
  export XEWE_NO_BOARD=1 # only a --hardware chip's flash and board steps may look for a board

  if [[ "${PREPARE}" == 1 ]]; then
    prepare "${VERSION}"
    return
  fi
  [[ "${SKIP_CI}" == 1 ]] && warn "!!!!!!!! --skip-ci: the CI gates are skipped (emergencies only) !!!!!!!!"

  stage 1 preflight
  GH_OK=0
  if ! command -v gh >/dev/null 2>&1; then
    fail "gh is not installed (https://cli.github.com)"
  elif ! gh auth status >/dev/null 2>&1; then
    fail "gh is not logged in (gh auth login)"
  else
    GH_OK=1
  fi

  run git fetch --quiet origin

  # resume: the tag exists (here or on origin); only the wait for its CI and the checks are left.
  local resume=0 tagged="" stale x
  if tag_local "${VERSION}" || tag_remote "${VERSION}"; then
    [[ "${GH_OK}" == 1 ]] || die "tag ${VERSION} exists; gh is needed to follow its CI run"
    resume=1
    tagged="$(tag_commit "${VERSION}")"
    git cat-file -e "${tagged}^{commit}" 2>/dev/null \
      || die "tag ${VERSION} points at ${tagged:0:12}, which is not in this clone (git fetch --tags origin)"
    stale="$(stale_locations "${VERSION}" "${tagged}")"
    [[ -z "${stale}" ]] \
      || die "tag ${VERSION} points at ${tagged:0:12}, whose files do not say ${VERSION}:"$'\n'"${stale}"
    git cat-file -e "${tagged}:releases/${VERSION}/release_card.md" 2>/dev/null \
      || die "tag ${VERSION} points at ${tagged:0:12}, which has no releases/${VERSION}/release_card.md"
  fi

  local notes="releases/${VERSION}/notes.txt" card="releases/${VERSION}/release_card.md" head
  head="$(git rev-parse HEAD)"
  # A fresh release needs a clean main at origin's head; a resume only needs the tagged commit.
  if [[ "${resume}" == 0 ]]; then
    [[ -s "${notes}" ]] || fail "${notes} is missing (the release notes; scripts/release.sh --prepare ${VERSION})"
    [[ -s "${card}" ]] || fail "${card} is missing (the release card; scripts/release.sh --prepare ${VERSION})"
    git ls-files --error-unmatch -- "${notes}" "${card}" >/dev/null 2>&1 \
      || fail "${notes} and ${card} are not both committed (the prepare commit)"
    if [[ -s "${notes}" ]] && grep -q -- "${NOTES_MARKER}" "${notes}"; then
      fail "${notes} is still the skeleton (its ${NOTES_MARKER} line)"
    fi
    [[ ! -e "releases/${VERSION}/testcard.md" ]] \
      || fail "releases/${VERSION}/testcard.md exists: the release card replaces it; delete it"
    [[ -z "$(git status --porcelain)" ]] || fail "the working tree is not clean (git status)"
    local branch remote
    branch="$(git symbolic-ref --quiet --short HEAD || echo "(detached)")"
    [[ "${branch}" == main ]] || fail "on branch ${branch}, not main"
    remote="$(git ls-remote origin refs/heads/main | cut -f1)"
    if [[ -z "${remote}" ]]; then
      fail "origin has no main branch"
    elif [[ "${head}" != "${remote}" ]]; then
      fail "main (${head:0:12}) is not level with origin/main (${remote:0:12}): pull or push first"
    fi
    stale="$(stale_locations "${VERSION}" "")"
    [[ -z "${stale}" ]] \
      || fail "these version locations do not say ${VERSION} (scripts/release.sh --prepare ${VERSION}):"$'\n'"${stale}"
    x="$(refs_problem)"
    [[ -z "${x}" ]] || fail "${x}"
    x="$(tracked_firmware)"
    [[ -z "${x}" ]] || fail "${FIRMWARE}/ tracks files (D26): $(tr '\n' ' ' <<< "${x}")"
  fi
  [[ "${FAILED}" -eq 0 ]] && echo "   ok: ${VERSION} on ${head:0:12}"

  if [[ "${resume}" == 1 ]]; then
    echo "== resuming at stage 5: tag ${VERSION} exists (${tagged:0:12})"
    tag_remote "${VERSION}" || run git push origin "refs/tags/${VERSION}"
  else
    stage 2 "release card"
    # verify is read-only: it runs in a dry run too, with the card tools of --prepare
    if [[ -x "${VENVS}/card/bin/python" ]]; then
      XEWE=("${VENVS}/card/bin/python" -m xewe)
    elif [[ "${DRY_RUN}" != 1 ]]; then
      local py
      py="$(find_python)"
      [[ -n "${py}" ]] || die "Python >= 3.11 is required"
      card_tools "${py}"
    fi
    echo "+ ${VENVS}/card/bin/python -m xewe release-card verify --card ${card}"
    if [[ "${#XEWE[@]}" -gt 0 ]]; then
      "${XEWE[@]}" release-card verify --card "${card}" \
        || fail "release-card verify failed on ${card} (reasons above)"
    else
      echo "   (not run: no card tools in ${VENVS}/card; --prepare ${VERSION} creates them, a real run too)"
    fi

    stage 3 "CI on HEAD"
    ci_gate "${head}" main

    stage 4 tag
    # the tag message is the GitHub Release text: xewe-release.yml reads it
    run git tag -a "${VERSION}" -F "${notes}" "${head}"
    run git push origin "refs/tags/${VERSION}"
    tagged="${head}"
  fi

  stage 5 "release CI on the tag"
  ci_gate "${tagged}" "${VERSION}"
  release_assets "${VERSION}"
  echo "Projects made from ${REPO}: tag ${VERSION}; images at https://github.com/${REPO}/releases/download/${VERSION}/<asset>."
  if [[ "${DRY_RUN}" == 1 && "${FAILED}" -gt 0 ]]; then
    echo "== dry run: ${FAILED} check(s) failed; a real run stops at the first"
    exit 1
  fi
}

# Sourcing the file (tests) defines the functions without running a release.
if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
