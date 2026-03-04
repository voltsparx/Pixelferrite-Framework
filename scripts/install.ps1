param(
    [ValidateSet("system", "user")]
    [string]$Target = "user",
    [switch]$Yes
)

$ErrorActionPreference = "Stop"
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$installer = Join-Path $scriptRoot "..\building-scripts\install-windows.ps1"

$args = @("-Mode", "install", "-Target", $Target)
if ($Yes) {
    $args += "-Yes"
}
powershell -ExecutionPolicy Bypass -File $installer @args


