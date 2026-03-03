# Pixelgen CLI

`pixelgen` is the non-interactive generator for:

- simulation metadata artifacts (`json`, `txt`, `yaml`)
- optional simulation image output from a carrier file

It does not generate real weaponized payloads.

## Usage

```text
pixelgen -l <target>
pixelgen -p <payload> [options]
```

## List Targets

- `payloads`
- `exploits`
- `auxiliary`
- `encoders`
- `evasion`
- `nops`
- `transports`
- `detection`
- `analysis`
- `lab`
- `formats`
- `all`

Example:

```bash
pixelgen -l payloads
```

## Generation Options

- `-p, --payload <id>`
- `-e, --encoder <id>`
- `-t, --transport <type>`
  - `tcp|http|https|dns|quic|tls|file`
- `-f, --format <format>`
  - `json|txt|yaml`
- `-o, --out <file>`
- `--input <image>` (alias: `--carrier`)
- `--image-out <file>`
- `--name <filename>`
- `--dir <directory>`
- `--lhost <addr>`
- `--lport <port>`
- `--rhost <addr>`
- `--platform <name>`
- `--iterations <n>`

## Output Rules

### Artifact Output

- Always writes artifact when `-p` is set.
- Default path (if `-o` missing):
  - user-data workspace tmp (`pixelgen_artifact.json`)

### Simulation Image Output

Triggered only when `--carrier` / `--input` is provided.

Output resolution order:

1. `--image-out <full path>`
2. `--dir` + `--name`
3. default: current working directory + auto-generated file name

Auto filename pattern:

- `pixelgen_<payload_leaf>_<transport>_<timestamp>.<carrier_ext>`

## Examples

Artifact only:

```bash
pixelgen -p payloads/windows/telemetry_win -t https -o tmp/artifact.json
```

Carrier to current directory:

```bash
pixelgen -p payloads/windows/telemetry_win -t https \
  --carrier data/carriers/sample.png \
  --rhost 192.168.56.101 --lhost 192.168.56.1 --lport 4444 \
  -o tmp/artifact.json
```

Carrier to custom location/name:

```bash
pixelgen -p payloads/linux/telemetry_linux -t tcp \
  --carrier data/carriers/sample.jpg \
  --dir ./out --name vm_payload.jpg \
  -o ./out/artifact.json
```

## Validation and Error Handling

`pixelgen` validates:

- modules root path exists
- output artifact path writable
- carrier image exists (if provided)
- output image path writable (if carrier provided)
- transport profile is supported

## Related Docs

- Image embedding details: [`image_embedding.md`](image_embedding.md)
- Console workflow: [`console_commands.md`](console_commands.md)

