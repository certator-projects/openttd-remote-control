#!/usr/bin/env bash
set -euo pipefail

# cp -r semantics depend on whether the destination already exists:
#   cp -r SRC  DEST     -> if DEST exists, creates DEST/basename(SRC)  (nested copy)
#   cp -r SRC/ DEST/    -> copies contents of SRC into DEST
# Trailing slashes change whether the directory itself or its contents are copied.
# We strip them so callers can pass paths either way, then use explicit replace/copy
# patterns that never rely on those edge cases.

strip_trailing_slash() {
    local path="$1"
    case "$path" in
        /) echo "/" ;;
        */) echo "${path%/}" ;;
        *) echo "$path" ;;
    esac
}

if [[ "$1" == "initialize-cache" ]]; then
    CACHE_DIR=$(strip_trailing_slash "${2}")
    BUILD_DIR=$(strip_trailing_slash "${3}")
    mkdir -p "${BUILD_DIR}"
    if [[ -d "${CACHE_DIR}" ]]; then
        echo "Restoring build directory from cache"
        # Clear contents only — Dockerfile WORKDIR is often BUILD_DIR; removing the
        # directory itself invalidates the shell cwd (getcwd: No such file or directory).
        cd /
        find "${BUILD_DIR}" -mindepth 1 -delete
        cp -a "${CACHE_DIR}/." "${BUILD_DIR}/"
        # Drop stale nested dir left by old cp -r cache updates (e.g. build-debug/build-debug).
        nested="${BUILD_DIR}/$(basename "${BUILD_DIR}")"
        if [[ -d "${nested}" ]]; then
            echo "Removing stale nested cache artifact: ${nested}"
            rm -rf "${nested}"
        fi
        echo "Build directory: ${BUILD_DIR}"
        ls -l "${BUILD_DIR}"
    else
        echo "No cache found, will start from the scratch"
    fi
elif [[ "$1" == "update-cache" ]]; then
    CACHE_DIR=$(strip_trailing_slash "${2}")
    BUILD_DIR=$(strip_trailing_slash "${3}")
    echo "Updating cache"
    rm -rf "${CACHE_DIR}"
    cp -a "${BUILD_DIR}" "${CACHE_DIR}"
    echo "Build directory: ${BUILD_DIR}"
    ls -l "${BUILD_DIR}"
else
    echo "Unknown command: $1"
    exit 1
fi
