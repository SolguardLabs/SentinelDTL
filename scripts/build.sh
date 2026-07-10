#!/usr/bin/env bash
set -euo pipefail

cc_bin="${CC:-cc}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out="${SENTINELDTL_OUT:-${root}/build/sentineldtl}"

mkdir -p "${root}/build"

mapfile -t sources < <(find "${root}/src" -name '*.c' | sort)

"${cc_bin}" \
  -std=c11 \
  -Wall \
  -Wextra \
  -Werror \
  -I"${root}/include" \
  "${sources[@]}" \
  -o "${out}"

echo "${out}"
