#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# scripts/release.sh
#
# release.sh: test the firmware once, tag the commit with the notes and the release card in the tag
# message; CI builds the images and publishes the GitHub Release.
# Usage: scripts/release.sh <version> [--hardware c3] [--notes "line" ...] [--notes-file FILE] [--resume] [--ci-timeout <min>]
#
# This file is the template's, and every firmware project made from the template keeps a copy.
# A project changes exactly two lines: REPO (its GitHub repository) and BOARD_GATES (the gated
# board tests its card session runs). The project name comes from xewe.toml [project] name, which
# must equal REPO's name.
#
# One command, 6 stages. <version> is X.Y.Z or X.Y.Z-suffix (a pre-release), no `v`; the version
# lines (VERSION_LOCATIONS) get X.Y.Z. The notes are 1 to 3 lines: --notes once per line, and/or the
# lines of --notes-file. Everything the run writes is under build/release/<version>/ (gitignored).
#   1 preflight: refuses, and lists every problem, unless: on main; the work tree has no change at
#     all (untracked files included); main is level with origin/main after a fetch; the tag is free
#     here and on origin (--resume excepted); ci.yml calls the tools workflows at the [tools] ref
#     of xewe.toml (a tag, unless XEWE_TOOLS_SOURCE is set); the notes pass `xewe release-notes
#     check`; no tracked file under static/; no active INJECTED_FROM_DOTENV line in Config.h;
#     build/ is ignored; Python >= 3.11 and gh (logged in) are there;
#   2 tests: the version lines set, then, recorded by `xewe release-card record`: ./setup.sh
#     (XEWE_CORE_SOURCE, XEWE_MODULES_SOURCE and XEWE_TOOLS_SOURCE passed through when set), the
#     checkers (ruff, clang-format), `xewe format --check`, `xewe brand-lint .`, `xewe modules
#     validate`, `xewe test --unit-only` (JUnit). With --hardware C: `xewe build --chip C`,
#     `xewe flash --chip C --no-build --require-board` and `xewe test --chip C --require-board --
#     -m board` (JUnit, the BOARD_GATES set), the board having had its first boot (`xewe provision
#     --chip C`; flashing keeps NVS, never erase it). Without it the board tests are compiled, not
#     run, for the xewe.toml chip. No other chip is built here: CI builds every matrix row. The
#     defines of a chip's steps come from XEWE_RELEASE_DEFINES_<CHIP> (environment, space
#     separated KEY=VALUE). The board steps read the dotenv file (XEWE_ENV, else .env): the tools
#     read it, this script never does. A failing step does not stop the others; the card refuses it;
#   3 release card: `xewe release-card render` writes build/release/<version>/release_card.md from
#     those results, or refuses (the draft is in the logs folder);
#   4 commit: only the VERSION_LOCATIONS files are added, `release <version>`; refuses when any
#     other file changed;
#   5 tag: the message is the notes, one blank line, the card; `git tag -a --cleanup=verbatim` (git
#     would drop the card's `#` lines otherwise). Then `git push origin main`, a check that
#     origin/main equals HEAD, and `git push origin <version>`: main first, so that the workflow is
#     registered before the tag triggers it;
#   6 release CI: waits for ci.yml on the tag (--ci-timeout minutes, default 60). Its release job
#     (xewe-release.yml) builds every image per chip, checks the card against its own runs (`xewe
#     release-card verify --ci`) and creates the GitHub Release; this script never creates it. Then
#     it checks that the card check step passed and that the release has every asset: per image
#     row the .bin, meta and manifest, plus firmware_map.csv, the tarball and release_card.md; and
#     prints the release URL.
# A run that stops before stage 4 puts the version lines back. --resume: when the tag exists (here
# or on origin), pushes what is missing (main, then the tag) and runs stage 6; when HEAD is the
# `release <version>` commit and there is no tag, reuses build/release/<version>/ if its card is
# there (else runs stage 2 again), then goes on from stage 3.
# Exit codes: 0 done, 1 a refusal or a failed check.
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

# Every place that states the project's own version: "file|ERE|replacement", {V} = X.Y.Z.
# Each pattern must match at least one line; stage 2 rewrites the matches. ERE is portable (GNU
# and BSD sed -E); no `|` inside.
VERSION_LOCATIONS=(
  'xewe.toml|^version = "[^"]*"$|version = "{V}"'
)

die() { echo "error: $*" >&2; exit 1; }
warn() { echo "warning: $*" >&2; }
STAGES=6
stage() { echo "== $1/${STAGES} $2"; }

# A failed preflight check: noted, and all of them are listed before the run stops.
PROBLEMS=()
fail() { PROBLEMS+=("$*"); }
check_problems() {
  [[ "${#PROBLEMS[@]}" -eq 0 ]] && return 0
  local p
  echo "error: the release cannot start:" >&2
  for p in "${PROBLEMS[@]}"; do echo "  - ${p}" >&2; done
  exit 1
}

# Prints a command and runs it.
run() {
  printf '+'; printf ' %q' "$@"; printf '\n'
  "$@"
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

# version_files: every file named in VERSION_LOCATIONS, once.
version_files() {
  local loc
  for loc in "${VERSION_LOCATIONS[@]}"; do echo "${loc%%|*}"; done | awk '!seen[$0]++'
}

# tracked_firmware: files git tracks under FIRMWARE (D26: none).
tracked_firmware() {
  git ls-files -- "${FIRMWARE}"
}

# tree_problems: one line per reason the work tree cannot be released from; none when it can.
tree_problems() {
  local p path unmerged fw dirty
  git symbolic-ref -q HEAD >/dev/null || echo "HEAD is detached: check out main"
  for p in MERGE_HEAD CHERRY_PICK_HEAD REVERT_HEAD REBASE_HEAD rebase-merge rebase-apply; do
    path="$(git rev-parse --git-path "${p}")"
    [[ -e "${path}" ]] && echo "a merge, rebase, cherry-pick or revert is in progress (${path} exists): finish or abort it first"
  done
  unmerged="$(git diff --name-only --diff-filter=U)"
  [[ -z "${unmerged}" ]] || echo "unmerged (conflicted) paths: $(tr '\n' ' ' <<< "${unmerged}")"
  dirty="$(git status --porcelain=v1 --untracked-files=all | cut -c4- | head -n 10 | tr '\n' ' ')"
  [[ -z "${dirty}" ]] || echo "the work tree is not clean (commit, stash or remove): ${dirty% }"
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

# ci_run SHA REF: the newest run of CI_WORKFLOW for SHA on REF (the tag name for a tag push), as
# "id status url conclusion" (empty when there is none yet; conclusion is empty while the run is
# not completed).
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

# ci_gate SHA REF: wait for CI_WORKFLOW on SHA and REF and fail unless it succeeds, within
# CI_TIMEOUT minutes. CI_RUN_ID is the run's id afterwards.
CI_RUN_ID=""
ci_gate() {
  local sha="$1" ref="$2" deadline=$((SECONDS + CI_TIMEOUT * 60)) line id status url conclusion pid
  CI_RUN_ID=""
  echo "   waiting for a ${CI_WORKFLOW} run on ${sha:0:12} (${ref}, timeout ${CI_TIMEOUT} min)"
  while :; do
    line="$(ci_run "${sha}" "${ref}")" || die "gh run list failed"
    [[ -n "${line}" ]] && break
    (( SECONDS < deadline )) || die "timed out after ${CI_TIMEOUT} min: no ${CI_WORKFLOW} run for ${sha:0:12} on ${ref}"
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
        die "timed out after ${CI_TIMEOUT} min, CI still running: ${url} (re-run with --resume)"
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
# is there, release_card.md among them; print them and the release URL.
release_assets() {
  local v="$1" assets count want card line
  want="$(expected_assets)"
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
  if [[ -n "${XEWE_TOOLS_SOURCE:-}" ]]; then
    commit="$(git -C "${XEWE_TOOLS_SOURCE}" rev-parse HEAD 2>/dev/null || echo -)"
  else
    commit="$(git ls-remote "${TOOLS_URL}" "refs/tags/${TOOLS_REF}^{}" "refs/tags/${TOOLS_REF}" "refs/heads/${TOOLS_REF}" \
      | awk '{ c[NR] = $1 } END { print (NR ? c[1] : "-") }')"
  fi
  echo "${commit:--}" > "${VENVS}/card/xewe-tools-commit"
}

# step LABEL RECORD_ARGS...: one recorded step; a failed command is noted and the run goes on (the
# card refuses it), a usage error stops.
FAILED_STEPS=()
step() {
  local label="$1" rc=0
  shift
  "${XEWE[@]}" release-card record --dir "${LOGS}" "$@" </dev/null || rc=$?
  case "${rc}" in
    0) ;;
    1|3) FAILED_STEPS+=("${label}"); warn "${label} failed (exit ${rc}; the log is in ${LOGS}); going on" ;;
    *) die "xewe release-card record exited ${rc} on ${label}" ;;
  esac
}

# chip_defines CHIP: `--define KEY=VALUE` words from XEWE_RELEASE_DEFINES_<CHIP> (environment;
# space separated), one per line.
chip_defines() {
  local var d
  var="XEWE_RELEASE_DEFINES_$(tr '[:lower:]' '[:upper:]' <<< "$1")"
  for d in ${!var-}; do printf '%s\n%s\n' --define "${d}"; done
}

# write_notes: the notes lines into NOTES_FILE.
write_notes() {
  mkdir -p "${OUT}"
  if [[ "${#NOTES[@]}" -gt 0 ]]; then printf '%s\n' "${NOTES[@]}" > "${NOTES_FILE}"; else : > "${NOTES_FILE}"; fi
}

# notes_problems: one line per problem of the notes (none when `xewe release-notes check` accepts them).
notes_problems() {
  local out line
  if [[ "${#NOTES[@]}" -eq 0 ]]; then
    echo "no release notes: give 1 to 3 lines with --notes \"line\" (repeated) or --notes-file FILE"
    return 0
  fi
  out="$("${XEWE[@]}" release-notes check "${NOTES_FILE}" 2>&1)" || {
    while IFS= read -r line; do [[ -z "${line}" ]] || echo "release notes: ${line}"; done <<< "${out}"
  }
}

# restore_versions: put the version files back to HEAD when a run stops before the commit.
VERSIONS_WRITTEN=0
VFILES=()
restore_versions() {
  if [[ "${VERSIONS_WRITTEN}" == 1 ]]; then
    VERSIONS_WRITTEN=0
    git checkout -q -- "${VFILES[@]}" 2>/dev/null || true
    echo "   version lines put back" >&2
  fi
}

# tests VERSION: stage 2 (see the header).
tests() {
  local v="$1" chip hw x loc file fw_bin
  local defines=() env_src=()
  stage 2 tests
  for loc in "${VERSION_LOCATIONS[@]}"; do
    file="${loc%%|*}"
    set_location "${loc}" "${BASE}" "" >/dev/null || die "${file}: no line matches $(cut -d'|' -f2 <<< "${loc}")"
    if ! cmp -s "${file}" <(set_location "${loc}" "${BASE}" ""); then
      [[ "${MODE}" == fresh ]] && VERSIONS_WRITTEN=1
      set_location "${loc}" "${BASE}" "" > "${file}.tmp"
      cat "${file}.tmp" > "${file}" # cat keeps the file's mode
      rm -f "${file}.tmp"
    fi
  done
  echo "   version lines say ${BASE}"
  for x in CORE MODULES TOOLS; do
    loc="XEWE_${x}_SOURCE"
    [[ -z "${!loc:-}" ]] || { echo "   ${loc}=${!loc} (passed to ./setup.sh)"; env_src+=("${loc}=${!loc}"); }
  done
  run "${XEWE[@]}" release-card init --dir "${LOGS}" --repo "${REPO}" --version "${v}" --tag "${v}"

  # ./setup.sh installs the tools of [tools] (or XEWE_TOOLS_SOURCE), the shared toolchain, the core,
  # the modules and the libraries of xewe.toml (or the local sources), no module menu (stdin closed)
  step setup --kind setup --name setup -- env -u XEWE_PROJECT XEWE_NO_BOARD=1 ${env_src[@]+"${env_src[@]}"} ./setup.sh
  step checkers --kind setup --name checkers -- "${PY}" -m pip install -q --disable-pip-version-check \
    --progress-bar off "${CHECKERS[@]}"
  step format --kind check --name format -- env "XEWE_CLANG_FORMAT=${ROOT}/build/tools/.venv/bin/clang-format" \
    "${PY}" -m xewe format --check
  step brand-lint --kind check --name brand-lint -- "${PY}" -m xewe brand-lint .
  step validate --kind validate --name modules -- env XEWE_NO_BOARD=1 "${PY}" -m xewe modules validate
  step unit --kind unit --name unit --junit "${OUT}/junit-unit.xml" -- env XEWE_NO_BOARD=1 "${PY}" -m xewe \
    test --unit-only -- "--junitxml=${ROOT}/${OUT}/junit-unit.xml" -o junit_family=xunit1 -o junit_logging=no -ra

  chip="${RUN_CHIP}"
  hw=0
  [[ -z "${HARDWARE}" ]] || hw=1
  while IFS= read -r x; do defines+=("${x}"); done < <(chip_defines "${chip}")
  step "build ${chip}" --kind build --chip "${chip}" --name image -- env XEWE_NO_BOARD=1 "${PY}" -m xewe \
    build --chip "${chip}" ${defines[@]+"${defines[@]}"}
  if [[ "${hw}" == 1 ]]; then
    fw_bin="$(find "build/builds/${chip}/out" -maxdepth 1 -name "*-${chip}-*.bin" ! -name '*bootloader*' \
      ! -name '*partitions*' 2>/dev/null | head -n 1)"
    [[ -n "${fw_bin}" ]] || fw_bin="build/builds/${chip}/out/missing.bin"
    step "flash ${chip}" --kind flash --chip "${chip}" --firmware "${fw_bin}" -- env -u XEWE_NO_BOARD "${PY}" -m xewe \
      flash --chip "${chip}" --no-build --require-board ${defines[@]+"${defines[@]}"}
    step "board ${chip}" --kind board --chip "${chip}" --junit "${OUT}/junit-board-${chip}.xml" \
      -- env -u XEWE_NO_BOARD ${BOARD_GATES[@]+"${BOARD_GATES[@]}"} "${PY}" -m xewe test --chip "${chip}" \
      --require-board ${defines[@]+"${defines[@]}"} -- -m board -o junit_family=xunit1 -o junit_logging=no -ra \
      "--junitxml=${ROOT}/${OUT}/junit-board-${chip}.xml"
  else
    step "board ${chip}" --kind board --chip "${chip}" --junit "${OUT}/junit-board-${chip}.xml" \
      -- env XEWE_NO_BOARD=1 ${BOARD_GATES[@]+"${BOARD_GATES[@]}"} "${PY}" -m xewe test --chip "${chip}" \
      --no-board ${defines[@]+"${defines[@]}"} -- -m board -o junit_family=xunit1 -o junit_logging=no -ra \
      "--junitxml=${ROOT}/${OUT}/junit-board-${chip}.xml"
  fi
  if [[ "${#FAILED_STEPS[@]}" -gt 0 ]]; then
    warn "failed steps: ${FAILED_STEPS[*]} (the card will refuse them)"
  fi
}

# card_stage VERSION: stage 3 (see the header).
card_stage() {
  local rc=0
  stage 3 "release card"
  run "${XEWE[@]}" release-card render --dir "${LOGS}" --notes "${NOTES_FILE}" --out "${CARD}" \
    --require "${REQUIRE}" --chips "${RUN_CHIP}" ${HARDWARE:+--hardware "${HARDWARE}"} \
    --title "${NAME}" || rc=$?
  case "${rc}" in
    0) ;;
    1) die "no card: the reasons are above (draft: ${LOGS}/release_card.draft.md). Fix the cause, then run again" ;;
    *) die "xewe release-card render exited ${rc}" ;;
  esac
  [[ "$(head -n 1 "${CARD}")" == "# "* ]] || die "${CARD} does not start with a '# ' title line"
}

# commit_stage VERSION: stage 4 (see the header).
commit_stage() {
  local entry path stray=()
  stage 4 commit
  if [[ "${MODE}" == commit ]]; then
    echo "   HEAD is already the release ${1} commit"
    return 0
  fi
  while IFS= read -r -d '' entry; do
    path="${entry:3}"
    version_files | grep -xF -- "${path}" >/dev/null || stray+=("${path}")
  done < <(git status --porcelain=v1 -z --untracked-files=all)
  [[ "${#stray[@]}" -eq 0 ]] || die "files other than the version lines changed (${stray[*]}): nothing was committed"
  run git add -- "${VFILES[@]}"
  run git commit -q --allow-empty -m "release ${1}"
  VERSIONS_WRITTEN=0
}

# push_main: push main and check that origin/main is HEAD (main first: the workflow is registered
# before the tag triggers it).
push_main() {
  run git push origin main
  [[ "$(git rev-parse origin/main)" == "$(git rev-parse HEAD)" ]] \
    || die "origin/main ($(git rev-parse --short=12 origin/main)) is not HEAD ($(git rev-parse --short=12 HEAD)) after the push"
}

# tag_stage VERSION: stage 5 (see the header).
tag_stage() {
  stage 5 tag
  { cat "${NOTES_FILE}"; echo; cat "${CARD}"; } > "${MSG}"
  run git tag -a --cleanup=verbatim -F "${MSG}" "${1}"
  push_main
  run git push origin "refs/tags/${1}"
}

usage() { sed -n 's/^# Usage: //p' "${BASH_SOURCE[0]}"; }

main() {
  VERSION=""
  RESUME=0
  HARDWARE=""
  CI_TIMEOUT=60
  NOTES=()
  XEWE=()
  local line file
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --resume) RESUME=1 ;;
      --hardware)
        [[ "${2:-}" =~ ^(c3|c6|s3)$ ]] || die "--hardware needs one chip: c3, c6 or s3 ($(usage))"
        HARDWARE="$2"
        shift
        ;;
      --notes)
        [[ $# -ge 2 ]] || die "--notes needs a line ($(usage))"
        NOTES+=("$2")
        shift
        ;;
      --notes-file)
        file="${2:-}"
        [[ -f "${file}" ]] || die "--notes-file needs a file: ${file}"
        while IFS= read -r line || [[ -n "${line}" ]]; do NOTES+=("${line}"); done < "${file}"
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
  [[ "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z][0-9A-Za-z.-]*)?$ ]] \
    || die "version must be X.Y.Z or X.Y.Z-suffix with no 'v' (the ecosystem's tags are un-prefixed): ${VERSION}"
  BASE="${VERSION%%-*}"
  while IFS= read -r line; do VFILES+=("${line}"); done < <(version_files)

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
  export XEWE_NO_BOARD=1 # only a --hardware chip's flash and board steps may look for a board
  OUT="build/release/${VERSION}"
  LOGS="${OUT}/logs"
  NOTES_FILE="${OUT}/notes.txt"
  CARD="${OUT}/release_card.md"
  MSG="${OUT}/tag_message.txt"
  trap restore_versions EXIT
  trap 'exit 130' INT TERM

  stage 1 preflight
  local x py head tagged="" lines
  MODE=fresh
  head="$(git rev-parse HEAD)"
  if ! command -v gh >/dev/null 2>&1; then
    fail "gh is not installed (https://cli.github.com)"
  elif ! gh auth status >/dev/null 2>&1; then
    fail "gh is not logged in (gh auth login)"
  fi
  run git fetch --quiet origin || die "git fetch origin failed"
  if tag_local "${VERSION}" || tag_remote "${VERSION}"; then
    if [[ "${RESUME}" == 1 ]]; then MODE=tag; else fail "tag ${VERSION} exists (here or on origin); --resume continues it"; fi
  elif [[ "${RESUME}" == 1 ]]; then
    if [[ "$(git log -1 --format=%s)" == "release ${VERSION}" ]]; then
      MODE=commit
    else
      fail "--resume: there is no tag ${VERSION} and HEAD is not the 'release ${VERSION}' commit: run without --resume"
    fi
  fi

  if [[ "${MODE}" == tag ]]; then
    tagged="$(tag_commit "${VERSION}")"
    if ! git cat-file -e "${tagged}^{commit}" 2>/dev/null; then
      fail "tag ${VERSION} points at ${tagged:0:12}, which is not in this clone (git fetch --tags origin)"
    else
      lines="$(stale_locations "${BASE}" "${tagged}")"
      [[ -z "${lines}" ]] || fail "tag ${VERSION} points at ${tagged:0:12}, whose files do not say ${BASE}:"$'\n'"${lines}"
    fi
    check_problems
    echo "   ok: resuming, tag ${VERSION} exists (${tagged:0:12})"
  else
    [[ "$(git symbolic-ref --quiet --short HEAD || echo '(detached)')" == main ]] \
      || fail "on branch $(git symbolic-ref --quiet --short HEAD || echo '(detached)'), not main"
    while IFS= read -r x; do [[ -z "${x}" ]] || fail "${x}"; done < <(tree_problems)
    if [[ "${MODE}" == commit ]]; then
      git merge-base --is-ancestor refs/remotes/origin/main HEAD 2>/dev/null \
        || fail "origin/main is not an ancestor of HEAD: pull first"
      lines="$(stale_locations "${BASE}" HEAD)"
      [[ -z "${lines}" ]] || fail "the release commit's files do not say ${BASE}:"$'\n'"${lines}"
    else
      x="$(git rev-parse -q --verify refs/remotes/origin/main || true)"
      if [[ -z "${x}" ]]; then
        fail "origin has no main branch"
      elif [[ "${head}" != "${x}" ]]; then
        fail "main (${head:0:12}) is not level with origin/main (${x:0:12}): pull or push first"
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
    git check-ignore -q "${LOGS}/run.json" || fail ".gitignore does not ignore build/ (the run folder must never be committed)"
    RUN_CHIP="${HARDWARE:-$(toml_value project chip)}"
    [[ "${RUN_CHIP}" =~ ^(c3|c6|s3)$ ]] || fail "xewe.toml [project] chip is \"${RUN_CHIP}\", not c3, c6 or s3"
    py="$(find_python)"
    if [[ -z "${py}" ]]; then
      fail "Python >= 3.11 is required (python3.12 is what CI's release jobs use)"
    else
      echo "   python: ${py}"
      write_notes
      card_tools "${py}"
      while IFS= read -r x; do [[ -z "${x}" ]] || fail "${x}"; done < <(notes_problems)
    fi
    check_problems
    echo "   tools: $(tools_spec) (CI: the tools workflows at $(workflow_refs | sed 's/ .*//'))"
    if [[ -n "${XEWE_ENV:-}" ]]; then echo "   dotenv: ${XEWE_ENV}"; elif [[ -f .env ]]; then echo "   dotenv: .env"; else echo "   dotenv: none"; fi
    echo "   hardware: ${HARDWARE:-none} (the ${RUN_CHIP} board tests are $([[ -n "${HARDWARE}" ]] && echo run || echo 'compiled, not run'); CI builds every chip)"
    [[ -z "${HARDWARE}" ]] || echo "   the ${HARDWARE} board must have had its first boot (xewe provision --chip ${HARDWARE}); never erase it for the run"
    x="$(chip_defines "${RUN_CHIP}" | { grep -v '^--define$' || true; } | tr '\n' ' ')"
    [[ -z "${x}" ]] || echo "   ${RUN_CHIP} defines: ${x% }"
    echo "   ok: ${VERSION} on ${head:0:12}"

    if [[ "${MODE}" == commit && -s "${CARD}" && -f "${LOGS}/run.json" ]]; then
      echo "== reusing the results under ${OUT}/ (stage 2 skipped)"
    else
      tests "${VERSION}"
    fi
    card_stage
    commit_stage "${VERSION}"
    tag_stage "${VERSION}"
    tagged="$(git rev-parse HEAD)"
  fi

  if [[ "${MODE}" == tag ]]; then
    echo "== resuming at stage 6: tag ${VERSION} exists (${tagged:0:12})"
    if ! tag_remote "${VERSION}"; then
      if ! git merge-base --is-ancestor "${tagged}" refs/remotes/origin/main 2>/dev/null; then
        if [[ "$(git symbolic-ref --quiet --short HEAD || true)" != main ]] || ! git merge-base --is-ancestor "${tagged}" HEAD; then
          die "origin/main lacks the tagged commit ${tagged:0:12} and this clone's main does not hold it: push main first"
        fi
        push_main
      fi
      run git push origin "refs/tags/${VERSION}"
    fi
  fi

  stage 6 "release CI on the tag"
  ci_gate "${tagged}" "${VERSION}"
  release_assets "${VERSION}"
  echo "Projects made from ${REPO}: tag ${VERSION}; images at https://github.com/${REPO}/releases/download/${VERSION}/<asset>."
  echo "done in ${SECONDS} s"
}

# Sourcing the file (tests) defines the functions without running a release.
if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
