$ErrorActionPreference = "Stop"

Write-Host "Configuring PixelFerrite..."
cmake -S . -B build
Write-Host "Done. Build with: cmake --build build"
