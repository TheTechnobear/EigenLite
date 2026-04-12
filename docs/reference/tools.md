# EigenLite Tools

Two command-line tools ship alongside the library: `eigenlite-capture` (requires hardware) and `eigenlite-convert` (no hardware dependency).

---

## eigenlite-capture

Records a live Eigenharp session to an `.elcf` binary file.

**Requires hardware.** Only built when `DISABLE_EIGENHARP` is not set.

### Usage

```
eigenlite-capture [--device TAU|ALPHA|PICO] [--duration-s N] --out <file.elcf>
```

| Flag | Description |
|---|---|
| `--out <file.elcf>` | Output path (required) |
| `--duration-s N` | Stop after N seconds; omit to run until Ctrl-C |
| `--device` | Ignored — device type is detected automatically from `connected()` |

### Behaviour

- Connects to the first discovered Eigenharp via `EigenApi::Eigenharp`.
- Captures all events: `key`, `button`, `breath`, `strip`, `pedal`, `connected`, `disconnected`.
- Each event record includes two timestamps:
  - `t_ms` — wall-clock elapsed ms since first event (steady_clock), used for timed replay
  - `t_us` — device hardware timestamp in µs, passed through verbatim from the driver
- Writes a 36-byte ELCF header followed by 36-byte records on exit.
- Poll time is set to 100 µs. Setting it lower (e.g. 10 µs) can starve the USB bus and cause disconnections.

### Live TUI

While capturing, a non-scrolling ANSI display updates at ~30 fps showing:
- Device type, elapsed / remaining time
- Breath bar and current value
- Strip bar, value, and active state
- Last key pressed (course, key, p/r/y)
- Keys held count, buttons held count
- Total event count

Press Ctrl-C (or wait for `--duration-s`) to stop; the file is written on exit.

### Output format

See `docs/reference/capture-format.md` for the full binary layout.

---

## eigenlite-convert

Converts an `.elcf` binary file to JSON or CSV. No hardware required.

### Usage

```
eigenlite-convert --input <file.elcf> [--format json|csv] [--out <file>]
```

| Flag | Description |
|---|---|
| `--input <file.elcf>` | Input capture file (required) |
| `--format json\|csv` | Output format (default: csv) |
| `--out <file>` | Output path; omit to write to stdout |

### Output columns (CSV)

```
t_ms, t_us, event_type, device_type, course, key, strip_idx, active,
pedal_idx, dev_id, pressure, roll, yaw, value
```

`event_type` is a string: `key`, `breath`, `strip`, `button`, `pedal`, `connected`, `disconnected`.

### Output structure (JSON)

```json
{
  "device_type": "TAU",
  "event_count": 12392,
  "events": [
    {"t_ms":0,"type":"connected","course":0,"key":0,...},
    ...
  ]
}
```

### Notes

- Both `t_ms` and `t_us` are included in every record, even for event types that don't use them (zeros for `connected`/`disconnected`).
- `convert` validates the ELCF magic and schema version; unrecognised files are rejected with a clear error.
