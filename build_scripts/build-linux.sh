#!/usr/bin/env bash
set -euo pipefail
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux
ctest --test-dir build/linux --output-on-failure
