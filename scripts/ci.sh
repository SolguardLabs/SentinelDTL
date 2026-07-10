#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

bash "${root}/scripts/build.sh"
node "${root}/scripts/check-js.mjs"
node --test "${root}/tests/node"/*.test.js
"${root}/build/sentineldtl" --list >/dev/null
"${root}/build/sentineldtl" baseline >/dev/null
"${root}/build/sentineldtl" pause-resume >/dev/null
"${root}/build/sentineldtl" limits >/dev/null
"${root}/build/sentineldtl" protection >/dev/null
"${root}/build/sentineldtl" batch >/dev/null
"${root}/build/sentineldtl" treasury >/dev/null
"${root}/build/sentineldtl" snapshot >/dev/null
