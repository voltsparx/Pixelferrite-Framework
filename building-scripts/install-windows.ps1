param(
    [ValidateSet("install", "test")]
    [string]$Mode = "install",
    [ValidateSet("system", "user")]
    [string]$Target = "system",
    [switch]$Yes,
    [switch]$UpdateExisting,
    [switch]$UninstallExisting,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

function Write-Info([string]$Message) {
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Write-Warn([string]$Message) {
    Write-Host "Warning: $Message" -ForegroundColor Yellow
}

function Fail([string]$Message) {
    Write-Host "Error: $Message" -ForegroundColor Red
    exit 1
}

function Show-Usage {
@"
Usage:
  powershell -ExecutionPolicy Bypass -File .\building-scripts\install-windows.ps1 [options]

Options:
  -Mode <install|test>      Install mode or test mode
  -Target <system|user>     Install target root
  -Yes                      Auto-confirm dependency installation prompts
  -UpdateExisting           Update an existing pixelferrite installation
  -UninstallExisting        Uninstall an existing pixelferrite installation
  -Help                     Show this help
"@ | Write-Host
}

function Invoke-Tool {
    param(
        [Parameter(Mandatory = $true)][string]$Tool,
        [Parameter(Mandatory = $false)][string[]]$Args = @(),
        [Parameter(Mandatory = $true)][string]$FailureMessage
    )

    & $Tool @Args
    if ($LASTEXITCODE -ne 0) {
        Fail "$FailureMessage (exit code $LASTEXITCODE)."
    }
}

function Ensure-Directory([string]$Path) {
    New-Item -ItemType Directory -Force -Path $Path | Out-Null
}

function Remove-PathIfExists([string]$Path) {
    if (Test-Path $Path) {
        Remove-Item -Path $Path -Recurse -Force
    }
}

function Normalize-Path([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $null
    }
    try {
        return [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
    } catch {
        return $Path.TrimEnd('\')
    }
}

function Resolve-BuildBinary([string]$BuildDir, [string]$Name) {
    $candidates = @(
        (Join-Path $BuildDir "bin\$Name.exe"),
        (Join-Path $BuildDir "bin\Release\$Name.exe"),
        (Join-Path $BuildDir "Release\$Name.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

function Test-DirHasContent([string]$Path) {
    if (-not (Test-Path $Path -PathType Container)) {
        return $false
    }
    $first = Get-ChildItem -Force -Recurse -ErrorAction SilentlyContinue $Path | Select-Object -First 1
    return $null -ne $first
}

function Add-ToPath([string]$PathToAdd, [string]$Scope) {
    $normalized = Normalize-Path $PathToAdd
    if (-not $normalized) {
        return
    }

    $current = [Environment]::GetEnvironmentVariable("Path", $Scope)
    if (-not $current) { $current = "" }

    $entries = $current -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    foreach ($entry in $entries) {
        if ((Normalize-Path $entry) -ieq $normalized) {
            return
        }
    }

    $newPath = if ($current) { "$current;$PathToAdd" } else { $PathToAdd }
    [Environment]::SetEnvironmentVariable("Path", $newPath, $Scope)
}

function Remove-FromPath([string]$PathToRemove, [string]$Scope) {
    $normalized = Normalize-Path $PathToRemove
    if (-not $normalized) {
        return
    }

    $current = [Environment]::GetEnvironmentVariable("Path", $Scope)
    if (-not $current) {
        return
    }

    $entries = $current -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    $filtered = @()
    foreach ($entry in $entries) {
        if ((Normalize-Path $entry) -ine $normalized) {
            $filtered += $entry
        }
    }

    [Environment]::SetEnvironmentVariable("Path", ($filtered -join ';'), $Scope)
}

function Refresh-CurrentPath {
    $machinePath = [Environment]::GetEnvironmentVariable("Path", "Machine")
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")

    if (-not $machinePath) { $machinePath = "" }
    if (-not $userPath) { $userPath = "" }

    $combined = @($machinePath, $userPath) -join ';'
    $entries = $combined -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    $env:Path = ($entries | Select-Object -Unique) -join ';'
}

function Get-AvailableCompiler {
    foreach ($candidate in @("g++", "clang++", "cl")) {
        if (Get-Command $candidate -ErrorAction SilentlyContinue) {
            return $candidate
        }
    }
    return $null
}

function Confirm-Install([string]$Prompt) {
    if ($Yes) {
        return $true
    }

    if (-not [Environment]::UserInteractive) {
        Fail "$Prompt (non-interactive shell detected; rerun with -Yes to auto-confirm)"
    }

    $reply = Read-Host "$Prompt [y/N]"
    return $reply -match '^(?i:y|yes)$'
}

function Ensure-BuildDependencies {
    $missing = @()

    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        $missing += [PSCustomObject]@{
            Name = "cmake"
            WingetId = "Kitware.CMake"
        }
    }

    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
        $missing += [PSCustomObject]@{
            Name = "ninja"
            WingetId = "Ninja-build.Ninja"
        }
    }

    if (-not (Get-AvailableCompiler)) {
        $missing += [PSCustomObject]@{
            Name = "compiler (clang++)"
            WingetId = "LLVM.LLVM"
        }
    }

    if ($missing.Count -eq 0) {
        Write-Info "Build dependencies already installed."
        return
    }

    $missingNames = ($missing | ForEach-Object { $_.Name }) -join ", "
    Write-Warn "Missing build dependencies: $missingNames"

    $winget = Get-Command winget -ErrorAction SilentlyContinue
    if (-not $winget) {
        Fail "winget is not available. Install the missing dependencies manually: $missingNames"
    }

    if (-not (Confirm-Install "Install missing dependencies using winget?")) {
        Fail "Cannot continue without required build dependencies."
    }

    foreach ($dep in $missing) {
        Write-Info "Installing $($dep.Name) via winget ($($dep.WingetId))"
        & winget install --id $dep.WingetId -e --silent --accept-package-agreements --accept-source-agreements
        if ($LASTEXITCODE -ne 0) {
            Fail "Failed to install $($dep.Name) via winget (exit code $LASTEXITCODE)."
        }
    }

    Refresh-CurrentPath

    $stillMissing = @()
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { $stillMissing += "cmake" }
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) { $stillMissing += "ninja" }
    if (-not (Get-AvailableCompiler)) { $stillMissing += "compiler (g++/clang++/cl)" }

    if ($stillMissing.Count -gt 0) {
        $list = $stillMissing -join ", "
        Fail "Dependencies still missing after installation: $list"
    }
}

if ($Help) {
    Show-Usage
    exit 0
}

if ($UpdateExisting -and $UninstallExisting) {
    Fail "Use only one of -UpdateExisting or -UninstallExisting."
}

if ($Mode -eq "test" -and ($UpdateExisting -or $UninstallExisting)) {
    Fail "-UpdateExisting and -UninstallExisting are only supported in install mode."
}

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).
    IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$buildDir = Join-Path $projectRoot "build-windows"

$systemRoot = "C:\Program Files\PixelFerrite"
$systemBin = Join-Path $systemRoot "bin"
$userRoot = Join-Path $env:LOCALAPPDATA "PixelFerrite"
$userBin = Join-Path $userRoot "bin"

$existingTarget = $null
if (Test-Path (Join-Path $systemBin "pixelferrite.exe")) {
    $existingTarget = "system"
} elseif (Test-Path (Join-Path $userBin "pixelferrite.exe")) {
    $existingTarget = "user"
}

if ($UpdateExisting) {
    if (-not $existingTarget) {
        Fail "No existing pixelferrite installation detected."
    }
    $Target = $existingTarget
}

if ($UninstallExisting) {
    if (-not $existingTarget) {
        Fail "No existing pixelferrite installation detected."
    }

    $root = if ($existingTarget -eq "system") { $systemRoot } else { $userRoot }
    if ($existingTarget -eq "system" -and -not $isAdmin) {
        Fail "System uninstall requires Administrator privileges."
    }

    Write-Info "Removing $root"
    Remove-PathIfExists $root
    if ($existingTarget -eq "system") {
        Remove-FromPath -PathToRemove $systemBin -Scope "Machine"
    } else {
        Remove-FromPath -PathToRemove $userBin -Scope "User"
    }
    Write-Host "Uninstalled pixelferrite." -ForegroundColor Green
    exit 0
}

Ensure-BuildDependencies

if ($Mode -eq "install" -and $Target -eq "system" -and -not $isAdmin) {
    Fail "System install requires Administrator privileges."
}

Write-Info "Configuring build directory $buildDir"
$compiler = Get-AvailableCompiler
$cmakeConfigureArgs = @("-S", $projectRoot, "-B", $buildDir, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
if ($compiler -eq "g++") {
    $cmakeConfigureArgs += "-DCMAKE_CXX_COMPILER=g++"
} elseif ($compiler -eq "clang++") {
    $cmakeConfigureArgs += "-DCMAKE_CXX_COMPILER=clang++"
}
Invoke-Tool -Tool "cmake" -Args $cmakeConfigureArgs -FailureMessage "CMake configure failed"

Write-Info "Building"
Invoke-Tool -Tool "cmake" -Args @("--build", $buildDir, "--config", "Release") -FailureMessage "Build failed"

$pffconsoleExe = Resolve-BuildBinary -BuildDir $buildDir -Name "pffconsole"
$pixelgenExe = Resolve-BuildBinary -BuildDir $buildDir -Name "pixelgen"

if (-not $pffconsoleExe -or -not $pixelgenExe) {
    Fail "Build output not found under $buildDir."
}

if ($Mode -eq "test") {
    Write-Host "Test build ready:" -ForegroundColor Green
    Write-Host "  $pffconsoleExe"
    Write-Host "  $pixelgenExe"
    exit 0
}

$targetRoot = if ($Target -eq "system") { $systemRoot } else { $userRoot }
$targetBin = if ($Target -eq "system") { $systemBin } else { $userBin }

$tempRoot = Join-Path $env:TEMP ("pixelferrite_install_" + [guid]::NewGuid().ToString("N"))
$stageRoot = Join-Path $tempRoot "PixelFerrite"

try {
    Ensure-Directory $stageRoot
    Ensure-Directory (Join-Path $stageRoot "bin")

    Copy-Item -Force $pffconsoleExe (Join-Path $stageRoot "bin\pffconsole.exe")
    Copy-Item -Force $pixelgenExe (Join-Path $stageRoot "bin\pixelgen.exe")
    Copy-Item -Force $pffconsoleExe (Join-Path $stageRoot "bin\pixelferrite.exe")

    $modulesSrc = Join-Path $projectRoot "modules"
    if (Test-DirHasContent $modulesSrc) {
        Copy-Item -Recurse -Force $modulesSrc (Join-Path $stageRoot "modules")
    }

    $payloadSrc = Join-Path $projectRoot "modules\payloads"
    if (Test-DirHasContent $payloadSrc) {
        Copy-Item -Recurse -Force $payloadSrc (Join-Path $stageRoot "payloads")
    }

    $buildLib = Join-Path $buildDir "lib"
    if (Test-DirHasContent $buildLib) {
        Ensure-Directory (Join-Path $stageRoot "lib")
        Copy-Item -Recurse -Force (Join-Path $buildLib "*") (Join-Path $stageRoot "lib")
    }

    $docsSrc = Join-Path $projectRoot "docs"
    $dataSrc = Join-Path $projectRoot "data"
    $labsSrc = Join-Path $projectRoot "labs"
    $readmeSrc = Join-Path $projectRoot "README.md"
    $hasResources = (Test-DirHasContent $docsSrc) -or (Test-DirHasContent $dataSrc) -or (Test-DirHasContent $labsSrc) -or (Test-Path $readmeSrc)

    if ($hasResources) {
        Ensure-Directory (Join-Path $stageRoot "resources")
    }

    if (Test-DirHasContent $docsSrc) {
        Ensure-Directory (Join-Path $stageRoot "resources\docs")
        Copy-Item -Recurse -Force (Join-Path $projectRoot "docs\*") (Join-Path $stageRoot "resources\docs")
    }
    if (Test-DirHasContent $dataSrc) {
        Ensure-Directory (Join-Path $stageRoot "resources\data")
        Copy-Item -Recurse -Force (Join-Path $projectRoot "data\*") (Join-Path $stageRoot "resources\data")
    }
    if (Test-DirHasContent $labsSrc) {
        Ensure-Directory (Join-Path $stageRoot "resources\labs")
        Copy-Item -Recurse -Force (Join-Path $projectRoot "labs\*") (Join-Path $stageRoot "resources\labs")
    }
    if (Test-Path $readmeSrc) {
        Copy-Item -Force $readmeSrc (Join-Path $stageRoot "resources\README.md")
    }

    $configDir = Join-Path $stageRoot "config"
    Ensure-Directory $configDir
    if (Test-Path (Join-Path $projectRoot "config")) {
        Copy-Item -Recurse -Force (Join-Path $projectRoot "config\*") $configDir
    }
    @"
# PixelFerrite default configuration
safety_mode=strict
module_root=modules
"@ | Set-Content -Path (Join-Path $configDir "default.conf") -Encoding Ascii

    "PixelFerrite 0.1.0" | Set-Content -Path (Join-Path $stageRoot "version.txt") -Encoding Ascii
} catch {
    Fail "Failed to stage install content. Details: $($_.Exception.Message)"
}

Write-Info "Installing framework root $targetRoot"
Remove-PathIfExists $targetRoot
Ensure-Directory $targetRoot
Copy-Item -Recurse -Force (Join-Path $stageRoot "*") $targetRoot

Write-Info "Adding $targetBin to PATH"
if ($Target -eq "system") {
    Add-ToPath -PathToAdd $targetBin -Scope "Machine"
} else {
    Add-ToPath -PathToAdd $targetBin -Scope "User"
}

$userDataRoot = Join-Path $env:APPDATA "PixelFerrite"
$userConfig = Join-Path $userDataRoot "config"
$userLogs = Join-Path $userDataRoot "logs"
$workspaceRoot = Join-Path $userDataRoot "workspace\default"

Ensure-Directory $userConfig
Ensure-Directory $userLogs
Ensure-Directory (Join-Path $workspaceRoot "logs")
Ensure-Directory (Join-Path $workspaceRoot "reports")
Ensure-Directory (Join-Path $workspaceRoot "sessions")
Ensure-Directory (Join-Path $workspaceRoot "scripts")
Ensure-Directory (Join-Path $workspaceRoot "state")
Ensure-Directory (Join-Path $workspaceRoot "tmp")
Ensure-Directory (Join-Path $workspaceRoot "datasets")

$userDefaultConf = Join-Path $userConfig "default.conf"
$installDefaultConf = Join-Path $targetRoot "config\default.conf"
if ((Test-Path $installDefaultConf) -and -not (Test-Path $userDefaultConf)) {
    Copy-Item -Force $installDefaultConf $userDefaultConf
}

try {
    Remove-PathIfExists $tempRoot
} catch {
    Write-Warn "Could not clean temporary staging directory: $tempRoot"
}

Write-Host "Installed framework root: $targetRoot" -ForegroundColor Green
Write-Host "Main launcher: $(Join-Path $targetBin 'pixelferrite.exe')" -ForegroundColor Green
Write-Host "User data root: $userDataRoot" -ForegroundColor Green
