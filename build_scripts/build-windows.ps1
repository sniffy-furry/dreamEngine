$ErrorActionPreference = 'Stop'
cmake -S . -B build/windows -G "Visual Studio 18 2026" -A x64
cmake --build build/windows --config Release
ctest --test-dir build/windows --build-config Release --output-on-failure
