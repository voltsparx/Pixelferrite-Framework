param(
    [int]$Count = 10,
    [string]$OutputDir = "data/datasets/generated"
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

for ($i = 1; $i -le $Count; $i++) {
    $path = Join-Path $OutputDir ("sample_{0:D4}.json" -f $i)
    $obj = @{
        id = ("sample_{0:D4}" -f $i)
        label = "clean"
        notes = "synthetic scaffold entry"
    } | ConvertTo-Json -Depth 3

    Set-Content -Path $path -Encoding utf8 -Value $obj
}

Write-Host "Generated $Count dataset items in $OutputDir"
