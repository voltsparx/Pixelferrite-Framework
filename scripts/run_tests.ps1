$ErrorActionPreference = "Stop"

if (-not (Test-Path "build")) {
    cmake -S . -B build
}

cmake --build build
ctest --test-dir build --output-on-failure
