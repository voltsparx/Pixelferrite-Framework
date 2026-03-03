param(
    [ValidateSet("system", "user")]
    [string]$Target = "user"
)

$ErrorActionPreference = "Stop"
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$installer = Join-Path $scriptRoot "..\building-scripts\install-windows.ps1"

powershell -ExecutionPolicy Bypass -File $installer -Mode install -Target $Target


