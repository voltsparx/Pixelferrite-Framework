# PixelFerrite-Framework

PixelFerrite is a C++20, image-centric **security simulation framework** with:

- `pffconsole`: metasploit-style interactive console for module workflows
- `pixelgen`: non-interactive generator for simulation artifacts and simulation image payload files

The current runtime is intentionally **simulation-only**. It models payload/exploit/evasion/auxiliary workflows for authorized research and validation without generating real weaponized payloads.

## Documentation Index

- Architecture: [`docs/architecture.md`](docs/architecture.md)
- Console command reference: [`docs/console_commands.md`](docs/console_commands.md)
- Pixelgen usage: [`docs/pixelgen.md`](docs/pixelgen.md)
- Image embedding/extraction: [`docs/image_embedding.md`](docs/image_embedding.md)
- Module development: [`docs/module_dev.md`](docs/module_dev.md)
- Lab setup: [`docs/lab_setup_guides.md`](docs/lab_setup_guides.md)
- Ethical use policy: [`docs/ethical_use.md`](docs/ethical_use.md)
- Installer docs: [`building-scripts/README.md`](building-scripts/README.md)

## Safety Model

- Simulation-first behavior for payload/exploit paths.
- Scope enforcement and workspace policy controls in console.
- External tool passthrough is restricted by workspace policy (`nmap`, `netprobe-rs`).
- Image embedding stores **simulation metadata markers**, not executable shellcode.

## Core Capabilities

- Manifest-driven module discovery from `modules/**/module.json`
- Runtime config loader for `config/framework.yml`, `config/logging.yml`, `config/workspace.yml`
- Module categories: `payload`, `exploit`, `auxiliary`, `encoder`, `evasion`, `nop`, `transport`, `analysis`, `detection`, `lab`
- Dual-stack simulation support (`RHOST`, `RHOST6`, `RHOSTS`, `LHOST`, `LHOST6`, `LPORT`)
- Simulation quality metrics: profile, confidence, stability, detection risk, dual-stack readiness
- Explainability engine with `explain summary` / `explain last`
- Workspace-isolated runtime output (logs/reports/sessions/tmp/datasets)
- Safe image simulation embedding/extraction with integrity checks for:
  - `.jpg`, `.jpeg`, `.png`, `.gif`, `.svg`

## Configuration

Runtime configuration is loaded at startup from:

1. `PF_CONFIG_DIR` (if set)
2. `./config`
3. `PF_HOME/config` (if set)
4. install-relative `config/` paths

Quick inspection from console:

```text
show config
verify config
```

## Repository Layout

```text
apps/                Executables (pffconsole, pixelgen)
core/                Shared runtime engines and command system
modules/             Manifest-driven module catalog
config/              Framework config files
data/                Sample carriers/datasets/schemas
docs/                Full documentation set
building-scripts/    Cross-platform installers and helpers
tools/               Helper tool placeholders (lint/export)
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

Binaries:

- `build/bin/pffconsole`
- `build/bin/pixelgen`

Windows (example):

```powershell
cmake -S . -B build-windows -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-windows --config Release
```

## Install

Use platform installers in `building-scripts/`:

- Windows: `building-scripts/install-windows.ps1`
- Linux: `building-scripts/install-linux.sh`
- macOS: `building-scripts/install-macos.sh`

Detailed install model and paths:

- [`building-scripts/README.md`](building-scripts/README.md)

## Quick Start

### Start Console

```bash
build/bin/pffconsole
```

### Basic Console Flow

```text
show payloads
use payloads/windows/telemetry_win
set RHOSTS 192.168.56.101,fd00::101
set LHOST 192.168.56.1
set LPORT 4444
set INTENSITY 85
check
run
sessions -l
explain summary
```

### Generate Artifact + Simulation Image (`pixelgen`)

Current directory auto-output:

```bash
pixelgen -p payloads/windows/telemetry_win -t https \
  --carrier data/carriers/sample.png \
  --rhost 192.168.56.101 --lhost 192.168.56.1 --lport 4444 \
  -o tmp/artifact.json
```

Custom path and name:

```bash
pixelgen -p payloads/linux/telemetry_linux -t tcp \
  --carrier data/carriers/sample.jpg \
  --dir ./out --name vm_payload.jpg \
  -o ./out/artifact.json
```

## Runtime Data Paths

Windows:

- `%APPDATA%\PixelFerrite\logs`
- `%APPDATA%\PixelFerrite\workspace\<workspace>\{<reports_root>,<sessions_root>,<temp_root>,datasets}`

Linux:

- `~/.local/share/pixelferrite/logs`
- `~/.local/share/pixelferrite/workspace/<workspace>/{<reports_root>,<sessions_root>,<temp_root>,datasets}`

macOS:

- `~/Library/Application Support/pixelferrite/logs`
- `~/Library/Application Support/pixelferrite/workspace/<workspace>/{<reports_root>,<sessions_root>,<temp_root>,datasets}`

## Current Tooling Notes

- `tools/module_lint` and `tools/report_export` are scaffold placeholders.
- Console command set is documented in [`docs/console_commands.md`](docs/console_commands.md).
- Pixelgen CLI details are documented in [`docs/pixelgen.md`](docs/pixelgen.md).

## License

See [`LICENSE`](LICENSE).
