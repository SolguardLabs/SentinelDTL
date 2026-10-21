#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "${root}/build"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -I"${root}/include" \
  "${root}/src/analytics.c" "${root}/src/amount.c" \
  "${root}/src/error.c" "${root}/src/types.c" "${root}/src/vault.c" \
  "${root}/tests/native/analytics_test.c" -o "${root}/build/analytics-test"
"${root}/build/analytics-test"
