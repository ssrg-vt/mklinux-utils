#!/bin/bash
set -eu -o pipefail

fail() {
    echo "ERROR: ${1}. Aborting" >&2
    exit 1
} 

absolute_path() {
    [ -d "$1" ] || fail "not a directory: $1"
    pushd . > /dev/null
    cd "$1"
    echo "$(pwd)/"
    popd > /dev/null
}

absolute_file_path() {
    local dir
    dir="$(dirname "$1")"
    echo "$(absolute_path "$dir")/$(basename "$1")"
}

is_dir() {
    [ -n "${!1}" ] || fail "variable $1 is empty"
    [ -d "${!1}" ] || fail "${!1}: directory not found (pwd is $(pwd))"
}

is_file() {
    [ -n "${!1}" ] || fail "variable $1 is empty"
    [ -f "${!1}" ] || fail "${!1}: file not found (pwd is $(pwd))"
}
