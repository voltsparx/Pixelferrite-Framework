# Architecture

PixelFerrite is split into:
- `apps/` executable entry points
- `core/` reusable framework runtime library
- `modules/` manifest-driven capability extensions
- `data/`, `config/`, `var/`, `tmp/` operational state

Current framework behavior includes:
- metasploit-style module categories including `auxiliary` collectors
- dual-stack target handling (`RHOST`, `RHOST6`, `RHOSTS`, `LHOST`, `LHOST6`, `LPORT`)
- scope enforcement via console `scope` command
