# Module Development

Each module lives in its own directory and includes:
- `module.json`
- `module.cpp`

Required manifest fields:
- `id`
- `name`
- `category`
- `supported_platforms`
- `supported_arch`
- `author`
- `version`
- `options`
- `safety_level`

Category values in current framework:
- `payload`, `exploit`, `auxiliary`, `encoder`, `evasion`, `nop`, `transport`, `detection`, `analysis`, `lab`

`options` is an array of option descriptors, for example:

```json
[
  {
    "name": "RHOSTS",
    "required": true,
    "description": "IPv4/IPv6 targets, comma separated."
  }
]
```
