# Console Commands (`pffconsole`)

This page documents the active command surface from `help`.

## Core

- `help`, `?`
- `banner`
- `version`
- `clear`
- `history`
- `explain [last|summary]`
- `verify paths`
- `verify config`
- `exit`, `quit`

## Workspace

- `workspace list`
- `workspace add <name>`
- `workspace select <name>`
- `workspace delete <name>`

Each workspace has isolated runtime output under user data path.

## Tool Policy

- `tools list`
- `tools allow <tool>`
- `tools deny <tool>`
- `tools reset`

Supported passthrough tools:

- `nmap`
- `netprobe-rs`

## Scope

- `scope list`
- `scope add <target>`
- `scope remove <target>`
- `scope clear`
- `scope enforce <on|off>`

When scope enforcement is `on`, out-of-scope targets are blocked during checks/runs.

## Discovery

- `search <keyword>`
- `show modules`
- `show payloads|exploits|auxiliary|encoders|evasion|nops|transports|detection|analysis|lab`
- `show platforms`
- `show options`
- `show sessions`
- `show config`

## Module Lifecycle

- `use <module_id>`
- `info`
- `back`
- `set <key> <value>`
- `unset <key|all>`
- `setg <key> <value>`
- `unsetg <key|all>`

Common network options:

- `RHOST`, `RHOST6`, `RHOSTS`
- `LHOST`, `LHOST6`, `LPORT`
- `INTENSITY` (30-100 simulation intensity)

Image workflow options:

- `IMAGE`
- `OUTFILE`
- `SIMCODE`
- `EXTRACT_OUT`

## Execution Workflow

- `check`
- `run`
- `exploit`
- `simulate`
- `embed`
- `extract`
- `mutate`
- `analyze`

Behavior summary:

- `check` computes readiness + simulation metrics.
- `run/simulate/exploit` execute simulation pipeline.
- non-session categories write simulation reports.
- payload/exploit categories open simulated sessions.
- `embed/extract` operate on simulation markers in supported image formats.

## Sessions

- `sessions`
- `sessions -l`
- `sessions -i <id>`
- `sessions -k <id>`
- `sessions -K`

Interactive `pixelpreter` commands:

- `help`
- `background` / `exit` / `quit`
- `sysinfo`
- `getuid`
- `ps`
- `ifconfig`
- `pwd`
- `ls`
- `download <remote> <local>`
- `upload <local> <remote>`

## Logs and Reports

- `log show`
- `log export <file>`
- `report generate`
- `report export <file>`

## Dataset

- `dataset generate <n>`
- `dataset analyze <dir>`
- `dataset compare <left> <right>`

## Automation and Operations

- `reload_all`
- `jobs`, `jobs -l`, `jobs -k <id>`, `jobs -K`
- `threads <count>`
- `resource <file>`
- `makerc [file]`
- `save [file]`
- `spool <start [file]|stop|status>`
- `route <list|add|del|flush>`
- `connect <host> <port>`
- `db_connect <name>`
- `db_status`

## Compatibility Stubs

Stub commands currently return simulation placeholder behavior:

- `load`, `unload`, `irb`, `pry`, `cd`, `color`, `debug`, `edit`
- `hosts`, `services`, `creds`, `loot`, `notes`, `vulns`

## Practical Example

```text
use payloads/windows/telemetry_win
set RHOSTS 192.168.56.101,fd00::101
set LHOST 192.168.56.1
set LPORT 4444
set INTENSITY 90
check
run
show sessions
explain last
```
