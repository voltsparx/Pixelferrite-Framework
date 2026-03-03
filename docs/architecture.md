# Architecture

## Overview

PixelFerrite is a modular simulation framework with two frontends:

- `pffconsole` for interactive operator workflows
- `pixelgen` for scripted generation of simulation artifacts and simulation image payload files

Both binaries share `pixelferrite_core`.

## High-Level Layers

1. **Application Layer**
   - `apps/pffconsole/main.cpp`
   - `apps/pixelgen/main.cpp`
2. **Core Runtime Layer**
   - `core/src/console.cpp` (command execution + orchestration)
   - `core/src/module_loader.cpp` (manifest discovery)
   - `core/src/session_manager.cpp` (session lifecycle)
   - `core/src/simulation_engine.cpp` (high-fidelity simulation metrics)
   - `core/src/explain_engine.cpp` (explainability output)
   - `core/src/image_engine.cpp` (safe image marker embedding/extraction)
   - `core/src/path_verifier.cpp` (path integrity / writable validation)
3. **Module Catalog Layer**
   - `modules/<category>/<module>/module.json`
   - Manifest-driven behavior mapping
4. **Runtime Data Layer**
   - user-writable logs/reports/sessions/tmp/datasets

## Core Runtime Behavior

### Module Discovery

- `ModuleLoader` recursively scans `modules/**/module.json`.
- Runtime categories include:
  - `payload`, `exploit`, `auxiliary`, `encoder`, `evasion`, `nop`, `transport`, `analysis`, `detection`, `lab`.

### Simulation Orchestration

When `check/run/simulate/exploit` executes:

1. Module is resolved from manifest.
2. Required options are validated.
3. Targets are normalized and scope-filtered.
4. `SimulationEngine` computes:
   - target-level success probability
   - detection risk
   - latency/jitter
   - profile and dual-stack readiness
5. `ExplainEngine` produces:
   - short summary (`explain summary`)
   - detailed reasoning (`explain last`)
6. Outputs are written:
   - reports for non-session categories
   - session JSON for payload/exploit categories

### Session Model

Session fields include:

- `session_id`
- `target_endpoint`
- `ip_version`
- `platform`
- `transport_used`
- `local_bind`
- `simulation_profile`
- `quality_score`
- `detection_risk`

`pixelpreter` is an interactive simulated session shell.

### Image Simulation Embedding

`ImageEngine` supports:

- embed simulation metadata marker to `.jpg/.jpeg/.png/.gif/.svg`
- extract marker back into text
- integrity verification after embedding
- overwrite guards handled by console path logic

## Safety and Policy Controls

- Simulation-only behavioral design for attack-like workflows.
- Scope controls:
  - `scope add/remove/list/clear`
  - `scope enforce on|off`
- Tool policy controls per workspace:
  - `tools allow/deny/list/reset`
  - supported passthrough tools: `nmap`, `netprobe-rs`

## Filesystem and Path Integrity

`PathVerifier` checks are applied for:

- required input files
- output file writability
- runtime directory creation/validation
- modules root validation

This avoids silent failures when paths are wrong or unwritable.

## Runtime Output Layout (User Data)

Per workspace, runtime creates:

- `logs/`
- `reports/`
- `sessions/`
- `scripts/`
- `state/`
- `tmp/`
- `datasets/`

Framework never depends on empty placeholder folders in the repo.

