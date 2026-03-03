# Image Embedding and Extraction (Simulation)

PixelFerrite supports safe simulation metadata embedding and extraction for:

- `.jpg`
- `.jpeg`
- `.png`
- `.gif`
- `.svg`

This is **metadata embedding for simulation**, not executable payload creation.

## Engines and Commands

Core engine:

- `ImageEngine` in `core/src/image_engine.cpp`

Console commands:

- `embed`
- `extract`

Console options:

- `IMAGE`
- `OUTFILE`
- `SIMCODE`
- `EXTRACT_OUT`

Pixelgen options:

- `--carrier` / `--input`
- `--image-out`
- `--name`
- `--dir`

## How Embedding Works

### Binary formats (`jpg/jpeg/png/gif`)

- Framework appends a bounded simulation marker block to the output file.
- Original file prefix is preserved byte-for-byte.
- Format signature is revalidated after write.

### SVG

- Framework inserts simulation marker as an XML comment near `</svg>`.
- SVG structure is checked after write.

## Integrity Validation

Post-write checks include:

- format re-detection
- prefix-preservation check (binary formats)
- structural marker validation (SVG)
- input/output size capture in embed report

## Safety Guards

Console runtime blocks:

- `OUTFILE == IMAGE`
- `EXTRACT_OUT == IMAGE`

These guards prevent accidental image overwrite corruption.

## Console Example

```text
use payloads/cross_platform/json_telemetry
set IMAGE data/carriers/sample.png
set OUTFILE out/sample_embedded.png
set SIMCODE sim-lab-v1
embed

set IMAGE out/sample_embedded.png
set EXTRACT_OUT out/extracted_simcode.txt
extract
```

## Pixelgen Example

```bash
pixelgen -p payloads/windows/telemetry_win -t https \
  --carrier data/carriers/sample.png \
  --dir ./out --name vm_test_payload.png \
  -o ./out/artifact.json
```

## Failure Modes

Common safe failures:

- unsupported/invalid carrier format
- unreadable input path
- unwritable output path
- corrupted or missing marker during extract

All failures return descriptive messages instead of silent writes.

