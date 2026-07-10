#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

bash "${root}/scripts/build.sh"
node --test "${root}/tests/node"/*.test.js
