#!/usr/bin/env bash
set -euo pipefail
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to your Android NDK}"
cmake -S . -B build/android-arm64 \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DCMAKE_BUILD_TYPE=Release \
  -DASTRA_BUILD_TESTS=OFF \
  -DASTRA_BUILD_SAMPLE=OFF
cmake --build build/android-arm64
