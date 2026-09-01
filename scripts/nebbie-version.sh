#!/usr/bin/env bash
# Resolve the Nebbie editor package version for build scripts.
nebbie_resolve_version() {
    local build_dir="${1:-}"
    local root_dir="${2:-}"

    if [[ -n "${NEBBIE_VERSION:-}" ]]; then
        printf '%s' "${NEBBIE_VERSION}"
        return 0
    fi

    if [[ -n "${build_dir}" && -f "${build_dir}/generated/version.hpp" ]]; then
        sed -n 's/^#define NEBBIE_VERSION "\(.*\)"/\1/p' "${build_dir}/generated/version.hpp" | head -1
        return 0
    fi

    if [[ -z "${root_dir}" ]]; then
        root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
    fi

    if [[ -f "${root_dir}/CMakeLists.txt" ]]; then
        sed -n 's/^project(nebbie-editor VERSION \([^ )]*\).*/\1/p' "${root_dir}/CMakeLists.txt" | head -1
        return 0
    fi

    printf '%s' "0.0.0"
}
