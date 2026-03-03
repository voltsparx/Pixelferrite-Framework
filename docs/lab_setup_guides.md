# Lab Setup Guides

This guide describes a safe baseline for running PixelFerrite simulations.

## Lab Principles

- Use non-production assets only.
- Isolate traffic from the public internet.
- Snapshot VMs before test cycles.
- Treat all generated artifacts as test-only data.

## Reference Topology

- Host machine (runs `pffconsole` and `pixelgen`)
- One or more target VMs
- Optional monitoring VM
- Private virtual switch / host-only network

Example addressing:

- IPv4: `192.168.56.0/24`
- IPv6: `fd00:56::/64`

## Minimum Build

1. Create isolated network in hypervisor.
2. Deploy 2+ VMs with snapshots enabled.
3. Configure static test IPs (IPv4 + IPv6 where needed).
4. Verify connectivity between host and VMs.

## PixelFerrite Lab Baseline

In console:

```text
workspace add lab01
workspace select lab01
scope add 192.168.56.*
scope add fd00:56::*
scope enforce on
tools list
```

## Dual-Stack Readiness Test

```text
use auxiliary/network/dualstack_connectivity_audit
set RHOST 192.168.56.101
set RHOST6 fd00:56::101
check
run
report generate
```

## Session Simulation Test

```text
use payloads/windows/telemetry_win
set RHOSTS 192.168.56.101,fd00:56::101
set LHOST 192.168.56.1
set LHOST6 fd00:56::1
set LPORT 4444
set INTENSITY 85
check
run
sessions -l
explain last
```

## Image Simulation Embedding Test

```text
use payloads/cross_platform/json_telemetry
set IMAGE data/carriers/sample.png
set OUTFILE var/workspaces/lab01/sample_embedded.png
set SIMCODE lab-sim-telemetry-v1
embed
set IMAGE var/workspaces/lab01/sample_embedded.png
extract
```

## Reset Checklist (Per Test Cycle)

1. Export reports/logs.
2. Close sessions (`sessions -K`).
3. Clear generated temp data in workspace if needed.
4. Revert VM snapshots.

