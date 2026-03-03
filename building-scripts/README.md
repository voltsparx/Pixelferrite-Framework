# Building Scripts

This directory contains platform-oriented helpers:

- `install-linux.sh`
- `install-macos.sh`
- `install-windows.ps1`
- `build-windows-from-linux.sh`

## Install Model

The installers now follow a self-contained framework root:

- only `pixelferrite` is globally exposed
- binaries/assets/modules stay inside a dedicated framework folder
- runtime writable data always goes to per-user locations

Installed layout:

- `bin/` (`pixelferrite`, `pffconsole`, `pixelgen`)
- `modules/`
- `payloads/`
- `lib/`
- `resources/`
- `config/default.conf`
- `version.txt`

Note: optional directories (`payloads`, `lib`, `resources`, `logs`) are only created when populated by installer inputs.

## Platform Paths

Linux:

- framework root: `/usr/local/lib/pixelferrite` (system) or `~/.local/lib/pixelferrite` (user)
- global command: `/usr/local/bin/pixelferrite` symlink to framework `bin/pixelferrite`
- user data: `~/.config/pixelferrite` and `~/.local/share/pixelferrite`

macOS:

- framework root: `/usr/local/lib/pixelferrite` (system) or `~/.local/lib/pixelferrite` (user)
- global command: `/usr/local/bin/pixelferrite` symlink to framework `bin/pixelferrite`
- user data: `~/Library/Application Support/pixelferrite`

Windows:

- framework root: `C:\Program Files\PixelFerrite` (system) or `%LOCALAPPDATA%\PixelFerrite` (user)
- global command: `pixelferrite.exe` from framework `bin\`
- user data: `%APPDATA%\PixelFerrite`

## Quick Usage

Linux:

```bash
bash building-scripts/install-linux.sh --install --system
```

macOS:

```bash
bash building-scripts/install-macos.sh --install --system
```

Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\building-scripts\install-windows.ps1 -Mode install -Target system
```

Linux -> Windows cross-build:

```bash
bash building-scripts/build-windows-from-linux.sh
```


