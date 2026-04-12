# EigenLite Capture Format (ELCF)

Binary format for recording EigenApi callback events as reusable test fixtures.

**Extension:** `.elcf`  
**Byte order:** little-endian throughout  
**Schema version:** 3

---

## File Layout

```
[File Header — 36 bytes]
[Event Record — 36 bytes] × N
```

---

## File Header (36 bytes)

| Offset | Size | Field | Notes |
|--------|------|-------|-------|
| 0 | 4 | `magic` | `uint32LE = 0x454C4346` (`'ELCF'`) |
| 4 | 1 | `schema_version` | `uint8 = 3` |
| 5 | 8 | `device_type` | `char[8]`, null-padded (e.g. `"TAU\0\0\0\0\0"`) |
| 13 | 8 | `captured_at` | `int64LE` — Unix timestamp, seconds |
| 21 | 15 | `reserved` | `uint8[15] = 0` |

---

## Event Record (36 bytes)

| Offset | Size | Field | Notes |
|--------|------|-------|-------|
| 0 | 4 | `t_ms` | `uint32LE` — ms since first event |
| 4 | 1 | `event_type` | See table below |
| 5 | 1 | `course` | Course index; 0 for non-key events |
| 6 | 1 | `key` | Key index; 0 for non-key events |
| 7 | 1 | `strip_idx` | Strip number; 0 if not a strip event |
| 8 | 1 | `active` | 0 or 1 |
| 9 | 1 | `pedal_idx` | Pedal number; 0 if not a pedal event |
| 10 | 2 | `dev_id` | `uint16LE` — index into device string table; 0 for single-device sessions |
| 12 | 4 | `pressure` | `float32LE` |
| 16 | 4 | `roll` | `float32LE` |
| 20 | 4 | `yaw` | `float32LE` |
| 24 | 4 | `value` | `float32LE` — breath, strip, or pedal value; 0.0 for other types |
| 28 | 8 | `t_us` | `uint64LE` — device timestamp in microseconds |

`t_ms` and `t_us` are both stored intentionally:

- `t_ms`: local elapsed milliseconds, used for replay pacing
- `t_us`: high-resolution device timestamp, used to distinguish events that land in the same `t_ms` bucket

For `connected` and `disconnected`, `t_us` is `0`.

### event_type values

| Value | Meaning |
|-------|---------|
| 0 | key |
| 1 | breath |
| 2 | strip |
| 3 | button |
| 4 | pedal |
| 5 | connected |
| 6 | disconnected |

---

## Device String Table

Not currently used by the capture tool. No table is written in schema v2.

`dev_id` remains reserved for future multi-device capture support; current single-device captures set `dev_id = 0` and use `device_type` in the header.

---

## Storage Characteristics

At 100 events/second: 3.6 KB/sec. A 60-second session ≈ 216 KB.

---

## Naming Convention

`<DEVICE>_<YYYYMMDD>_<N>.elcf` — e.g. `TAU_20260411_01.elcf`

Stored in `tests/fixtures/`.

---

## Verification

Check magic bytes at offset 0: `45 4C 43 46` (ASCII `ELCF`).
