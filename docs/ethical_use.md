# Ethical Use Policy

PixelFerrite is intended for:

- defensive research
- controlled lab simulation
- authorized assessment preparation and validation
- educational demonstrations of secure workflow design

## Explicitly Required

- Written authorization before testing any system.
- Isolated lab setup for all simulation workflows.
- Clear scope boundaries for hosts and networks.
- Legal and organizational compliance with local policy.

## Prohibited

- Unauthorized access attempts.
- Malware development or delivery.
- Real exploit weaponization using framework outputs.
- Running outside declared scope or without consent.

## Framework Design Alignment

The runtime is simulation-oriented by default:

- Session flows are modeled, not weaponized.
- Image embedding stores simulation metadata markers.
- Tool passthrough is policy-controlled per workspace.

## Operator Responsibility

Even with simulation-only design, operators are responsible for:

- scope enforcement configuration
- safe data handling
- report confidentiality
- legal compliance in their jurisdiction

## Recommended Process

1. Define scope in writing.
2. Use isolated lab/VLAN/VM environment.
3. Set workspace-specific tool policy.
4. Enable and verify scope controls before execution.
5. Export reports and logs for review/audit.

