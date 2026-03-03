# Module Development

PixelFerrite modules are currently manifest-driven. Runtime behavior is selected by module metadata and category.

## Module Directory Structure

Each module lives under `modules/<category>/<...>/<module_name>/` and includes:

- `module.json` (required)
- `module.cpp` (placeholder/extensibility artifact)

Example:

```text
modules/payloads/windows/telemetry_win/
  module.json
  module.cpp
```

## Required Manifest Fields

- `id`
- `name`
- `category`
- `supported_platforms`
- `supported_arch`
- `author`
- `version`
- `options`
- `safety_level`

## Category Values

- `payload`
- `exploit`
- `auxiliary`
- `encoder`
- `evasion`
- `nop`
- `transport`
- `detection`
- `analysis`
- `lab`

## Options Array

Each option object can define:

- `name` (required)
- `required` (optional bool, default false)
- `default` (optional)
- `description` (optional)

Example:

```json
{
  "options": [
    {
      "name": "RHOSTS",
      "required": true,
      "description": "Comma-separated IPv4/IPv6 targets."
    },
    {
      "name": "LPORT",
      "required": false,
      "default": "4444",
      "description": "Simulated listener port."
    }
  ]
}
```

## Safety Levels

Current manifests typically use:

- `simulation_only`
- `authorized_assessment_only`

These indicate intent and policy level for safe framework behavior.

## Runtime Discovery

`ModuleLoader` scans recursively for `module.json` files under `modules/` and constructs descriptors for:

- discovery listings (`show`, `search`)
- option validation (`check`, `run`)
- category-specific simulation routing

## Authoring Tips

- Keep IDs stable and lowercase path-style (`payloads/windows/telemetry_win`).
- Provide meaningful options with clear descriptions.
- Declare both IPv4 and IPv6 options where relevant.
- Use conservative defaults for simulation reproducibility.

## Validation

For schema and lint workflow, use:

- `data/schemas/module.schema.json`
- `tools/module_lint/` (scaffold placeholder for deeper checks)

