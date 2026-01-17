#!/usr/bin/env bash
set -euo pipefail

# Push binary, manifest, optional build_notes.txt, and builds/.version_state to 'binaries' branch.
# Preserves earlier artifacts by starting from the current binaries tree.
# Never touches your worktree or current branch.

PROJECT_ROOT=""
TARGET_DIR=""
VERSION_STR=""
REMOTE="origin"
BRANCH="binaries"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --project-root) PROJECT_ROOT="${2:-}"; shift 2 ;;
    --target-dir)   TARGET_DIR="${2:-}"; shift 2 ;;
    --version)      VERSION_STR="${2:-}"; shift 2 ;;
    -h|--help)
      echo "usage: $0 --project-root <abs> --target-dir <abs> --version <x.y.zzz>"
      exit 0 ;;
    *) echo "unknown arg: $1"; exit 2 ;;
  esac
done

[[ -d "${PROJECT_ROOT}" ]] || { echo "project-root invalid"; exit 2; }
[[ -d "${TARGET_DIR}"   ]] || { echo "target-dir invalid"; exit 2; }
[[ -n "${VERSION_STR}"  ]] || { echo "version missing"; exit 2; }

# Locate artifacts.
BIN_FILE="$(find "${TARGET_DIR}/binary" -maxdepth 1 -type f -name "*.bin" | head -n1 || true)"
MANIFEST_FILE="$(find "${TARGET_DIR}" -maxdepth 2 -type f -name "manifest.json" | head -n1 || true)"
NOTES_FILE="${TARGET_DIR}/build_notes.txt"
VERSION_STATE_FILE="${PROJECT_ROOT}/build/builds/.version_state"

[[ -n "${BIN_FILE}" && -f "${MANIFEST_FILE}" ]] || { echo "artifacts missing"; exit 0; }
[[ -f "${VERSION_STATE_FILE}" ]] || { echo ".version_state missing"; exit 2; }

# Ensure files are under repo; capture relative paths.
case "${BIN_FILE}" in
  "${PROJECT_ROOT}/"*) REL_BIN="${BIN_FILE#"${PROJECT_ROOT}/"}" ;;
  *) echo "binary not under repo"; exit 2 ;;
esac
case "${MANIFEST_FILE}" in
  "${PROJECT_ROOT}/"*) REL_MAN="${MANIFEST_FILE#"${PROJECT_ROOT}/"}" ;;
  *) echo "manifest not under repo"; exit 2 ;;
esac
REL_NOTES=""
if [[ -f "${NOTES_FILE}" && -s "${NOTES_FILE}" ]]; then
  case "${NOTES_FILE}" in
    "${PROJECT_ROOT}/"*) REL_NOTES="${NOTES_FILE#"${PROJECT_ROOT}/"}" ;;
    *) echo "notes not under repo"; exit 2 ;;
  esac
fi
case "${VERSION_STATE_FILE}" in
  "${PROJECT_ROOT}/"*) REL_VER_STATE="${VERSION_STATE_FILE#"${PROJECT_ROOT}/"}" ;;
  *) echo ".version_state not under repo"; exit 2 ;;
esac

# Pre-store blobs.
BIN_BLOB="$(git -C "${PROJECT_ROOT}" hash-object -w -- "${BIN_FILE}")"
MAN_BLOB="$(git -C "${PROJECT_ROOT}" hash-object -w -- "${MANIFEST_FILE}")"
VER_STATE_BLOB="$(git -C "${PROJECT_ROOT}" hash-object -w -- "${VERSION_STATE_FILE}")"
if [[ -n "${REL_NOTES}" ]]; then
  NOTES_BLOB="$(git -C "${PROJECT_ROOT}" hash-object -w -- "${NOTES_FILE}")"
fi

# Temp index; never touch user index or worktree.
INDEX_FILE="$(mktemp)"
cleanup() { rm -f "${INDEX_FILE}"; }
trap cleanup EXIT
export GIT_INDEX_FILE="${INDEX_FILE}"

get_remote_tip() {
  git -C "${PROJECT_ROOT}" fetch "${REMOTE}" "${BRANCH}" >/dev/null 2>&1 || true
  if git -C "${PROJECT_ROOT}" ls-remote --exit-code --heads "${REMOTE}" "${BRANCH}" >/dev/null 2>&1; then
    git -C "${PROJECT_ROOT}" rev-parse "refs/remotes/${REMOTE}/${BRANCH}^{commit}"
    return 0
  fi
  return 1
}

commit_from_tree() {
  local parent_commit="$1"
  if [[ -n "${parent_commit}" ]]; then
    local base_tree
    base_tree="$(git -C "${PROJECT_ROOT}" rev-parse "${parent_commit}^{tree}")"
    git -C "${PROJECT_ROOT}" read-tree "${base_tree}"         # start from existing binaries tree
  else
    git -C "${PROJECT_ROOT}" read-tree --empty                # first commit only
  fi

  git -C "${PROJECT_ROOT}" update-index --add --cacheinfo 100644 "${BIN_BLOB}" "${REL_BIN}"
  git -C "${PROJECT_ROOT}" update-index --add --cacheinfo 100644 "${MAN_BLOB}" "${REL_MAN}"
  git -C "${PROJECT_ROOT}" update-index --add --cacheinfo 100644 "${VER_STATE_BLOB}" "${REL_VER_STATE}"
  if [[ -n "${REL_NOTES}" ]]; then
    git -C "${PROJECT_ROOT}" update-index --add --cacheinfo 100644 "${NOTES_BLOB}" "${REL_NOTES}"
  fi

  local new_tree msg new_commit
  new_tree="$(git -C "${PROJECT_ROOT}" write-tree)"
  msg="binaries: ${VERSION_STR}"
  if [[ -n "${parent_commit}" ]]; then
    new_commit="$(git -C "${PROJECT_ROOT}" commit-tree "${new_tree}" -p "${parent_commit}" -m "${msg}")"
  else
    new_commit="$(git -C "${PROJECT_ROOT}" commit-tree "${new_tree}" -m "${msg}")"
  fi
  printf "%s" "${new_commit}"
}

# Push with retries on race.
for _ in {1..6}; do
  if REMOTE_TIP="$(get_remote_tip)"; then
    NEW_COMMIT="$(commit_from_tree "${REMOTE_TIP}")"
  else
    NEW_COMMIT="$(commit_from_tree "")"  # root commit
  fi

  if git -C "${PROJECT_ROOT}" push "${REMOTE}" "${NEW_COMMIT}:refs/heads/${BRANCH}"; then
    echo "pushed ${VERSION_STR} to ${BRANCH} @ ${NEW_COMMIT}"
    exit 0
  fi
  sleep 1
done

echo "push failed: non-fast-forward after retries"
exit 1
