#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/scripts/release.sh
#
# release.sh: tag xewe-led-os <version> from a prepared, pushed main; CI builds and publishes it.
# Usage: scripts/release.sh <version> [--dry-run] [--ci-timeout <min>] [--skip-ci] | --prepare <version> [--dry-run]
# A release is two steps. First `scripts/release.sh --prepare <version>`: it sets every version
# location (VERSION_LOCATIONS) to <version> and creates releases/<version>/ with notes.txt (the
# GitHub Release text) and testcard.md (what was tested, attached to the release) as skeletons when
# they are missing. It makes no git change; fill the two files, commit everything as
# "prepare <version>" and push. Then `scripts/release.sh <version>`:
#   1 preflight: version X.Y.Z without `v`; on main, clean, level with origin/main after a fetch;
#     tag free locally and on origin; releases/<version>/ files committed; every version location
#     says <version> (it lists the ones that do not); ci.yml calls the tools workflows at the
#     [tools] ref of xewe.toml; gh installed and logged in;
#   2 CI gate on HEAD: the newest run of workflow ci.yml for HEAD on main must succeed (waits for
#     it to appear and finish; --ci-timeout minutes, default 30);
#   3 local checks: `xewe test --no-board` and `xewe build` for c3, `xewe build --dry-run` for c6
#     and s3 (needs ./setup.sh first);
#   4 annotated tag <version> on HEAD with notes.txt as its message, push it;
#   5 wait for ci.yml on the tag (the same gate): its release job builds c3, c6 and s3, creates the
#     GitHub Release from the tag message with the .bin files and firmware-<version>.tar.gz, and
#     commits static/firmware/releases/<version>/ to main (a bot commit after the tag); then attach
#     testcard.md, and check and print the release assets and that commit on origin/main.
# Re-running resumes: when the tag exists (here or on origin), stage 5 runs alone (pushing the tag
# first if origin lacks it), provided the tagged commit's files say <version>. --skip-ci skips both
# CI gates (emergencies only; stage 5 then checks what is there without waiting).
# --dry-run prints every command and changes nothing (read-only gh and git queries still run); it
# reports every failed preflight check, not only the first, shows the CI state, never waits, and
# exits 1 if a check failed.
set -euo pipefail

REPO="xewe-labs/xewe-led-os"
CI_WORKFLOW="ci.yml"
CI_POLL="${RELEASE_CI_POLL:-15}" # seconds between CI polls
REL="static/firmware/releases"

# Every place that states the template's own version: "file|ERE|replacement", {V} = the version.
# Each pattern must match at least one line; --prepare rewrites the matches and the preflight fails
# when that rewrite would change anything. ERE is portable (GNU and BSD sed -E); no `|` inside.
VERSION_LOCATIONS=(
  'xewe.toml|^version = "[^"]*"$|version = "{V}"'
)

die() { echo "error: $*" >&2; exit 1; }
warn() { echo "warning: $*" >&2; }
stage() { echo "== $1/5 $2"; }

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

# workflow_refs: print "<ref of the tools workflows in ci.yml> <[tools] ref of xewe.toml>".
workflow_refs() {
  local wf tools
  wf="$(sed -nE 's|.*xewe-os-tools/[.]github/workflows/[a-z-]+[.]yml@([^[:space:]]+).*|\1|p' .github/workflows/ci.yml | sort -u | tr '\n' ' ')"
  tools="$(sed -n '/^\[tools\]/,/^\[/s/^ref = "\([^"]*\)".*/\1/p' xewe.toml)"
  echo "${wf% } ${tools}"
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

# ci_gate SHA BRANCH: wait for CI_WORKFLOW on SHA and BRANCH and fail unless it succeeds, within
# CI_TIMEOUT minutes.
ci_gate() {
  local sha="$1" br="$2" deadline=$((SECONDS + CI_TIMEOUT * 60)) line id status url conclusion pid
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
  echo "   CI passed: ${url}"
}

# verify VERSION: attach the test card, then print the GitHub Release's assets and the bot commit
# that put static/firmware/releases/VERSION/ on origin/main; fail when either is missing.
verify() {
  local v="$1" card="releases/$1/testcard.md" assets bins commit files line
  if [[ "${DRY_RUN}" == 1 ]]; then
    echo "+ gh release view ${v} --json assets (fail unless it has .bin assets)"
    printf '+'; printf ' %q' gh release upload "${v}" "${card}" --clobber; printf '\n'
    echo "+ git fetch --quiet origin main; git ls-tree -r --name-only origin/main -- ${REL}/${v}/"
    if [[ "${GH_OK}" == 1 ]] && gh release view "${v}" >/dev/null 2>&1; then
      echo "   GitHub Release ${v} exists"
    else
      echo "   no GitHub Release ${v} yet"
    fi
    return 0
  fi
  assets="$(gh release view "${v}" --json assets --jq '.assets[].name')" \
    || die "no GitHub Release ${v}: see the release job of the tag's ${CI_WORKFLOW} run"
  bins="$(grep -c '[.]bin$' <<< "${assets}" || true)"
  [[ "${bins}" -gt 0 ]] || die "GitHub Release ${v} has no .bin assets"
  gh release upload "${v}" "${card}" --clobber
  git fetch --quiet origin main
  files="$(git ls-tree -r --name-only origin/main -- "${REL}/${v}/")"
  [[ -n "${files}" ]] || die "origin/main has no ${REL}/${v}/: see the release job's \"Commit to the releases branch\" step"
  commit="$(git log -1 --format='%h %s (%an)' origin/main -- "${REL}/${v}/")"
  echo "   GitHub Release: $(gh release view "${v}" --json url --jq .url)"
  while IFS= read -r line; do echo "     ${line}"; done <<< "${assets}"
  echo "     $(basename "${card}")"
  echo "   origin/main: ${commit}"
  while IFS= read -r line; do echo "     ${line}"; done <<< "${files}"
}

# prepare VERSION: set every version location and create the releases/VERSION/ skeletons.
prepare() {
  local v="$1" loc file changed=() dir="releases/$1"
  for loc in "${VERSION_LOCATIONS[@]}"; do
    file="${loc%%|*}"
    set_location "${loc}" "${v}" "" >/dev/null || die "${file}: no line matches $(cut -d'|' -f2 <<< "${loc}")"
    if ! cmp -s "${file}" <(set_location "${loc}" "${v}" ""); then
      if [[ "${DRY_RUN}" != 1 ]]; then
        set_location "${loc}" "${v}" "" > "${file}.tmp"
        cat "${file}.tmp" > "${file}" # cat keeps the file's mode
        rm -f "${file}.tmp"
      fi
      [[ " ${changed[*]-} " == *" ${file} "* ]] || changed+=("${file}")
    fi
  done
  if [[ "${#changed[@]}" -gt 0 ]]; then
    echo "version ${v}: ${changed[*]}"
  else
    echo "version ${v}: every location already says it"
  fi
  if [[ ! -s "${dir}/notes.txt" ]]; then
    echo "create ${dir}/notes.txt (skeleton: fill it in)"
    [[ "${DRY_RUN}" == 1 ]] || { mkdir -p "${dir}"; cat > "${dir}/notes.txt" <<EOF
xewe-led-os ${v}

What the firmware is, in two or three sentences.

CHANGED SINCE <previous version>

- ...

BINARIES

- c3, c6, s3: <version>-<chip>-xewe-led-os.bin (merged image, flash at 0x0), also in
  static/firmware/releases/${v}/ on main.

TESTING

What was tested on which commits, and what was not: releases/${v}/testcard.md (attached to this
release).
EOF
    }
  fi
  if [[ ! -s "${dir}/testcard.md" ]]; then
    echo "create ${dir}/testcard.md (skeleton: fill it in)"
    [[ "${DRY_RUN}" == 1 ]] || { mkdir -p "${dir}"; cat > "${dir}/testcard.md" <<EOF
# xewe-led-os ${v} test card

| Check | Where | Result | Not covered |
|---|---|---|---|
| host tests (\`xewe test --no-board\`) | | | |
| builds c3, c6, s3 | | | |
| CI | | | |
| board tests | | | |

## Environment

## Commit
EOF
    }
  fi
  echo "no git change was made. Review, fill releases/${v}/, then commit and push:"
  echo "  git add ${changed[*]+${changed[*]} }${dir} && git commit -m \"prepare ${v}\" && git push origin main"
  echo "then: scripts/release.sh ${v} --dry-run; scripts/release.sh ${v}"
}

usage() { sed -n 's/^# Usage: //p' "${BASH_SOURCE[0]}"; }

main() {
  VERSION=""
  DRY_RUN=0
  SKIP_CI=0
  PREPARE=0
  CI_TIMEOUT=30
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --dry-run) DRY_RUN=1 ;;
      --skip-ci) SKIP_CI=1 ;;
      --prepare) PREPARE=1 ;;
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

  # Repo root: the top of the git work tree this is run in.
  command -v git >/dev/null 2>&1 || die "git is required"
  ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || die "not inside a git work tree"
  cd "${ROOT}"
  grep -qx 'name = "xewe-led-os"' xewe.toml 2>/dev/null || die "${ROOT} is not the xewe-led-os repo"
  [[ "${DRY_RUN}" == 1 ]] && echo "== dry run: nothing is changed; commands are printed with a leading +"

  if [[ "${PREPARE}" == 1 ]]; then
    [[ "${SKIP_CI}" == 0 ]] || die "--skip-ci does not apply to --prepare"
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
  local resume=0 tagged="" stale
  if tag_local "${VERSION}" || tag_remote "${VERSION}"; then
    [[ "${GH_OK}" == 1 ]] || die "tag ${VERSION} exists; gh is needed to follow its CI run"
    resume=1
    tagged="$(tag_commit "${VERSION}")"
    git cat-file -e "${tagged}^{commit}" 2>/dev/null \
      || die "tag ${VERSION} points at ${tagged:0:12}, which is not in this clone (git fetch --tags origin)"
    stale="$(stale_locations "${VERSION}" "${tagged}")"
    [[ -z "${stale}" ]] \
      || die "tag ${VERSION} points at ${tagged:0:12}, whose files do not say ${VERSION}:"$'\n'"${stale}"
  fi

  local notes="releases/${VERSION}/notes.txt" card="releases/${VERSION}/testcard.md"
  [[ -s "${notes}" ]] || fail "${notes} is missing (the release notes; scripts/release.sh --prepare ${VERSION})"
  [[ -s "${card}" ]] || fail "${card} is missing (the test card; scripts/release.sh --prepare ${VERSION})"
  git ls-files --error-unmatch -- "${notes}" "${card}" >/dev/null 2>&1 \
    || fail "releases/${VERSION}/ is not committed"
  local head refs
  head="$(git rev-parse HEAD)"
  # A fresh release needs a clean main at origin's head; a resume only needs the tagged commit.
  if [[ "${resume}" == 0 ]]; then
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
    refs="$(workflow_refs)"
    [[ "${refs% *}" == "${refs#* }" ]] \
      || fail "ci.yml calls the tools workflows at ${refs% *}, xewe.toml [tools] ref is ${refs#* }"
  fi
  if [[ "${FAILED}" -gt 0 ]]; then
    echo "== dry run: ${FAILED} preflight check(s) failed; a real run stops at the first"
  else
    echo "   ok: ${VERSION} on ${head:0:12}"
  fi

  if [[ "${resume}" == 1 ]]; then
    echo "== resuming at stage 5: tag ${VERSION} exists (${tagged:0:12})"
    tag_remote "${VERSION}" || run git push origin "refs/tags/${VERSION}"
  else
    stage 2 "CI on HEAD"
    ci_gate "${head}" main

    stage 3 "local checks"
    local py=build/tools/.venv/bin/python
    [[ -x "${py}" || "${DRY_RUN}" == 1 ]] || die "${py} is missing: run ./setup.sh first"
    run env XEWE_NO_BOARD=1 "${py}" -m xewe test --no-board
    run env XEWE_NO_BOARD=1 "${py}" -m xewe build --chip c3
    run env XEWE_NO_BOARD=1 "${py}" -m xewe build --chip c6 --dry-run
    run env XEWE_NO_BOARD=1 "${py}" -m xewe build --chip s3 --dry-run

    stage 4 "tag"
    # the tag message is the GitHub Release text: xewe-release.yml reads it
    run git tag -a "${VERSION}" -F "${notes}" "${head}"
    run git push origin "refs/tags/${VERSION}"
    tagged="${head}"
  fi

  stage 5 "CI release on the tag"
  ci_gate "${tagged}" "${VERSION}"
  verify "${VERSION}"
  echo "Pull the release commit: git pull --ff-only origin main (git-lfs fetches the .bin files)."
  echo "Projects made from the template: ${REPO} tag ${VERSION}."
  if [[ "${DRY_RUN}" == 1 && "${FAILED}" -gt 0 ]]; then
    exit 1
  fi
}

# Sourcing the file (tests) defines the functions without running a release.
if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
