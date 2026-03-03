# PixelFerrite Framework

PixelFerrite is a C++ image-centric security assessment framework with a Metasploit-style operator console (`pffconsole`) and an artifact generator CLI (`pixelgen`).

The current build is simulation-focused: it supports module discovery, option handling, workflow orchestration, report generation, scope controls, session lifecycle simulation (`pixelpreter`), and passthrough execution for approved external network tools.

## Table of Contents

1. [Scope and Safety](#scope-and-safety)
2. [What You Get](#what-you-get)
3. [Repository Layout](#repository-layout)
4. [Prerequisites](#prerequisites)
5. [Build](#build)
6. [Install](#install)
7. [Quick Start](#quick-start)
8. [pffconsole Command Reference](#pffconsole-command-reference)
9. [Session Model and pixelpreter](#session-model-and-pixelpreter)
10. [Module System](#module-system)
11. [pixelgen CLI](#pixelgen-cli)
12. [Data, Logs, Sessions, Reports](#data-logs-sessions-reports)
13. [Config Files](#config-files)
14. [Troubleshooting](#troubleshooting)
15. [Developer Notes](#developer-notes)
16. [Additional Docs](#additional-docs)
17. [License](#license)

## Scope and Safety

- This framework is designed for authorized security assessment, lab use, and research.
- Current runtime behavior is **simulation-oriented** for payload/exploit/session workflows.
- `auxiliary` modules can run local inventory/log/reachability collection routines and write reports.
- External tools are explicitly restricted to an allow/deny policy (`nmap`, `netprobe-rs`) per workspace.

Do not use this project outside explicit authorization boundaries.

## What You Get

- `pffconsole`: interactive REPL with module workflow commands inspired by Metasploit-style operations.
- `pixelgen`: non-interactive CLI for module listing and simulation artifact generation.
- `pixelferrite_core`: shared runtime library used by both apps.
- Manifest-driven module catalog with categories:
  - `payload`, `exploit`, `auxiliary`, `encoder`, `evasion`, `nop`, `transport`, `detection`, `analysis`, `lab`
- Dual-stack target options and scoping primitives:
  - `RHOST`, `RHOST6`, `RHOSTS`, `LHOST`, `LHOST6`, `LPORT`
- Runtime outputs under user-writable OS-specific data roots:
  - logs, reports, session JSON records, generated artifacts

Current module catalog snapshot:

- Total `module.json` manifests: **101**
- By category:
  - `payload: 29`
  - `auxiliary: 9`
  - `exploit: 12`
  - `encoder: 12`
  - `evasion: 11`
  - `nop: 11`
  - `transport: 5`
  - `analysis: 4`
  - `detection: 4`
  - `lab: 4`

## Repository Layout

```text
apps/                Executables (pffconsole, pixelgen)
core/                Shared framework runtime (console, loader, logging, sessions)
modules/             Manifest-driven module catalog (module.json + module.cpp)
config/              YAML configuration stubs
data/                Carriers, datasets, schemas, signatures
scripts/             Simple install scripts
building-scripts/    Structured install/test/update/uninstall scripts
docs/                Supporting markdown docs
```

## Prerequisites

- CMake `>= 3.20`
- C++20-capable compiler
- Ninja (recommended, optional if your generator differs)

Platform notes:

- Windows: PowerShell + CMake + compiler toolchain in `PATH`.
- Linux/macOS: Bash + CMake + compiler toolchain; installers can install dependencies via package manager where available.

Optional tools:

- `nmap` for direct passthrough scans from `pffconsole`.
- `netprobe-rs` for direct passthrough (if installed).

## Build

### Standard CMake Build

```bash
cmake -S . -B build
cmake --build build
```

Binaries are emitted to:

- `build/bin/pffconsole` (or `pffconsole.exe`)
- `build/bin/pixelgen` (or `pixelgen.exe`)

### Ninja Build (recommended)

```bash
cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-ninja
```

## Install

### Structured installers (`building-scripts/`)

Windows system install:

```powershell
powershell -ExecutionPolicy Bypass -File .\building-scripts\install-windows.ps1 -Mode install -Target system
```

Linux system install:

```bash
bash building-scripts/install-linux.sh --install --system
```

macOS system install:

```bash
bash building-scripts/install-macos.sh --install --system
```

Test build only:

- Windows: `powershell -ExecutionPolicy Bypass -File .\building-scripts\install-windows.ps1 -Mode test`
- Linux: `bash building-scripts/install-linux.sh --test`
- macOS: `bash building-scripts/install-macos.sh --test`

Install layout (all platforms):

- framework root contains: `bin/`, `modules/`, `payloads/`, `lib/`, `resources/`, `config/`, `logs/`, `version.txt`
- global exposed command: `pixelferrite`
- internal CLIs still available inside framework `bin/`: `pffconsole`, `pixelgen`

Default roots:

- Windows system: `C:\Program Files\PixelFerrite`
- Linux system: `/usr/local/lib/pixelferrite`
- macOS system: `/usr/local/lib/pixelferrite`

Cross-compile Windows binaries from Linux (MinGW):

```bash
bash building-scripts/build-windows-from-linux.sh --mingw-prefix x86_64-w64-mingw32
```

## Quick Start

### Start console

```bash
build/bin/pffconsole
```

On Windows:

```powershell
.\build-windows\bin\pffconsole.exe
```

### Minimal module workflow example

```text
show payloads
use payloads/windows/file_integrity_snapshot_win
show options
set IMAGE data/carriers/sample.png
set RHOSTS 192.168.1.20,192.168.1.21
set LHOST 192.168.1.10
set LPORT 4444
check
run
sessions -l
sessions -i 1
sysinfo
background
sessions -k 1
```

### Auxiliary collector example

```text
use auxiliary/device_info/local_system_inventory
run
report generate
report export tmp/latest_report.txt
```

## pffconsole Command Reference

`help` in console prints the authoritative command list. Current commands are grouped below.

### Core

- `help`, `?`
- `banner`
- `version`
- `clear`
- `history`
- `exit`, `quit`

### Workspace

- `workspace list`
- `workspace add <name>`
- `workspace select <name>`
- `workspace delete <name>`

### External Tool Policy (per workspace)

- `tools list`
- `tools allow <nmap|netprobe-rs>`
- `tools deny <nmap|netprobe-rs>`
- `tools reset`

Behavior:

- Default workspace allowlist: `nmap`, `netprobe-rs`
- Denied tools are blocked before execution.
- Allowed tools run directly and stream output into console.

Examples:

```text
tools deny nmap
nmap -V              # blocked
tools allow nmap
nmap -V              # runs if installed
```

### Scope Controls

- `scope list`
- `scope add <target>`
- `scope add <prefix*>`
- `scope remove <target|prefix*>`
- `scope clear`
- `scope enforce on`
- `scope enforce off`

When scope enforcement is on, out-of-scope targets are blocked at `check`/`run`.

### Discovery and Visibility

- `search <keyword>`
- `show modules`
- `show payloads|exploits|auxiliary|encoders|evasion|nops|transports|detection|analysis|lab`
- `show platforms`
- `show options`
- `show sessions`

### Module Lifecycle

- `use <module_id>`
- `info`
- `back`
- `set <KEY> <VALUE>`
- `unset <KEY|all>`
- `setg <KEY> <VALUE>`
- `unsetg <KEY|all>`

Notes:

- Option names are normalized to uppercase.
- `set PLATFORM <value>` updates active platform hint.
- `setg` applies global option defaults shared across modules.

### Execution Workflow

- `check`
- `run`
- `exploit`
- `simulate`
- `embed`
- `extract`
- `mutate`
- `analyze`

Behavior summary:

- `check`: validates required options and target scope.
- `run` / `exploit` / `simulate`: creates simulated sessions for non-auxiliary categories.
- `run` on `auxiliary` modules: executes collector logic and writes a report file.

### Sessions

- `sessions` or `sessions -l`
- `sessions -i <id>`
- `sessions -k <id>`
- `sessions -K`

### Logs and Reports

- `log show`
- `log export <file>`
- `report generate`
- `report export <file>`

### Dataset Utilities

- `dataset generate <n>`
- `dataset analyze <dir>`
- `dataset compare <left> <right>`

### Automation and Operations

- `reload_all`
- `jobs`, `jobs -l`, `jobs -k <id>`, `jobs -K`
- `threads <count>`
- `resource <file>`
- `makerc [file]`
- `save [file]`
- `spool start [file]`, `spool status`, `spool stop`
- `route list`, `route add <subnet/cidr> <gateway>`, `route del <subnet/cidr>`, `route flush`
- `connect <host> <port>`
- `db_connect <name|dsn>`, `db_status`

### Compatibility Stubs

- `load`, `unload`, `irb`, `pry`
- `cd`, `color`, `debug`, `edit`
- `hosts`, `services`, `creds`, `loot`, `notes`, `vulns`

## Session Model and pixelpreter

When eligible modules run, `pffconsole` creates simulated session records.

Session record fields:

- `session_id`
- `host_label`
- `target_endpoint`
- `ip_version` (`ipv4` / `ipv6` / `label`)
- `platform`
- `transport_used`
- `local_bind`
- `agent` (`pixelpreter`)

Files:

- `<user-data>/workspace/<workspace>/sessions/session_<id>.json`
- `<user-data>/logs/sessions.log`

Interactive pixelpreter commands (`sessions -i <id>`):

- `help`
- `background`, `exit`, `quit`
- `sysinfo`
- `getuid`
- `ps`
- `ifconfig`
- `pwd`
- `ls`
- `download <remote> <local>`
- `upload <local> <remote>`

## Module System

Each module directory contains:

- `module.json` (manifest, used by runtime discovery)
- `module.cpp` (code placeholder/implementation artifact for extension work)

The runtime discovers modules by scanning `modules/**/module.json`.

### Manifest Schema

See: `data/schemas/module.schema.json`

Required fields:

- `id`
- `name`
- `category`
- `supported_platforms` (array)
- `supported_arch` (array)
- `author`
- `version`
- `options` (array)
- `safety_level`

`options` entries may include:

- `name`
- `required` (boolean)
- `default` (optional default value)
- `description`

### Manifest Example

```json
{
  "id": "auxiliary/network/reachability_probe",
  "name": "Reachability Probe",
  "category": "auxiliary",
  "supported_platforms": ["cross_platform"],
  "supported_arch": ["any"],
  "author": "PixelFerrite Team",
  "version": "0.2.0",
  "options": [
    {
      "name": "RHOSTS",
      "description": "IPv4/IPv6 targets, comma separated.",
      "required": true
    }
  ],
  "safety_level": "authorized_assessment_only"
}
```

## pixelgen CLI

`pixelgen` supports module listing and simulation artifact generation.

### Help

```bash
build/bin/pixelgen --help
```

### List modules by category

```bash
build/bin/pixelgen -l payloads
build/bin/pixelgen -l encoders
build/bin/pixelgen -l all
```

Supported list targets:

- `payloads`
- `exploits`
- `auxiliary`
- `encoders`
- `evasion`
- `nops`
- `transports`
- `detection`
- `analysis`
- `lab`
- `formats`
- `all`

### Generate a simulation artifact

```bash
build/bin/pixelgen \
  -p payloads/windows/telemetry_win \
  -e encoders/aes_encoder \
  -f json \
  -o tmp/out.json \
  --input data/carriers/sample.png \
  --platform cross_platform \
  --iterations 1
```

Supported output formats:

- `json`
- `txt`
- `yaml`

Artifact example:

```json
{
  "tool": "pixelgen",
  "mode": "simulation_only",
  "created_at": "2026-03-03 20:26:21",
  "payload": "payloads/windows/telemetry_win",
  "encoder": "encoders/aes_encoder",
  "platform": "cross_platform",
  "iterations": 1,
  "input_image": "data/carriers/sample.png"
}
```

## Data, Logs, Sessions, Reports

Runtime creates/uses these paths:

Windows:

- `%APPDATA%\PixelFerrite\logs\framework.log`
- `%APPDATA%\PixelFerrite\logs\sessions.log`
- `%APPDATA%\PixelFerrite\workspace\<workspace>\reports\latest_report.txt`
- `%APPDATA%\PixelFerrite\workspace\<workspace>\sessions\session_<id>.json`
- `%APPDATA%\PixelFerrite\workspace\<workspace>\tmp\...`

Linux:

- `~/.local/share/pixelferrite/logs/framework.log`
- `~/.local/share/pixelferrite/logs/sessions.log`
- `~/.local/share/pixelferrite/workspace/<workspace>/reports/latest_report.txt`
- `~/.local/share/pixelferrite/workspace/<workspace>/sessions/session_<id>.json`
- `~/.local/share/pixelferrite/workspace/<workspace>/tmp/...`

macOS:

- `~/Library/Application Support/pixelferrite/logs/framework.log`
- `~/Library/Application Support/pixelferrite/logs/sessions.log`
- `~/Library/Application Support/pixelferrite/workspace/<workspace>/reports/latest_report.txt`
- `~/Library/Application Support/pixelferrite/workspace/<workspace>/sessions/session_<id>.json`
- `~/Library/Application Support/pixelferrite/workspace/<workspace>/tmp/...`

Dataset helper outputs:

- `dataset generate <n>` writes JSON files into `<user-data>/workspace/<workspace>/datasets/generated`

## Config Files

`config/framework.yml`:

- framework name/version
- module/plugin/data roots
- `safety_mode` hint

`config/logging.yml`:

- log level and sink file targets

`config/workspace.yml`:

- default/active workspace metadata
- temp/reports/sessions root hints

User runtime config roots:

- Windows: `%APPDATA%\PixelFerrite\config`
- Linux: `~/.config/pixelferrite`
- macOS: `~/Library/Application Support/pixelferrite/config`

## Troubleshooting

### Build fails with CMake generator errors

- Ensure CMake is installed and available in `PATH`.
- Install Ninja or remove `-G Ninja` to use your default generator.

### `nmap` or `netprobe-rs` command fails in `pffconsole`

- Verify binary is installed and in `PATH`.
- Check workspace policy:
  - `tools list`
  - `tools allow nmap`
  - `tools allow netprobe-rs`

### Module cannot run

- Confirm module is selected: `use <module_id>`
- Check required options: `show options`
- Set missing keys with `set`/`setg`
- If scope is enabled, ensure target is in scope:
  - `scope list`
  - `scope add <target>`

### No reports/sessions appear

- Ensure you used a supported action:
  - `run` on `auxiliary/*` for report generation
  - `run`/`exploit`/`simulate` on non-auxiliary for session creation

## Developer Notes

- Language: C++20
- Build system: CMake
- Runtime module discovery: `core/src/module_loader.cpp`
- Console command engine: `core/src/console.cpp`
- Session state manager: `core/src/session_manager.cpp`

Important implementation detail:

- Module execution logic is currently orchestrated by core runtime behavior keyed by manifest metadata and selected module id.
- `module.cpp` files are present per module for extension structure, but the current runtime path is manifest-driven and simulation-focused.

## Additional Docs

- `docs/architecture.md`
- `docs/console_commands.md`
- `docs/module_dev.md`
- `docs/ethical_use.md`
- `docs/lab_setup_guides.md`
- `building-scripts/README.md`

## License

See `LICENSE`.


