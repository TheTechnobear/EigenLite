# Task: EL-4 — Initial Hardware Captures
<!-- status: done -->
<!-- created: 2026-04-11  refs: docs/technical-requirements.md -->

## Goal
Record and commit real Eigenharp hardware capture sessions using the CLI tool from EL-2. These become permanent test fixtures for EL-5 (API contract tests) and EigenHost Step 7 (golden file tests). **Real hardware required.** All three device types (TAU, PICO, ALPHA) are available.

## Dependencies
- EL-2 complete — capture tool built and binary format finalised
- Hardware: TAU, PICO, ALPHA all available; EigenD or equivalent running

## Strategy

Start with TAU to validate the capture tool and binary format end-to-end before recording all three devices. Do not record all sessions at once — verify EL-3 can replay the first TAU capture cleanly before committing the full set.

## Sessions to Record

Capture files live in `tests/fixtures/`. Naming: `<DEVICE>_<YYYYMMDD>_<N>.elcf`.

### TAU — Session 1 (required first)
Purpose: end-to-end validation of capture → replay pipeline.

Target events:
- Course 0: ≥ 10 keys from varied positions across the grid
- Course 1 (percussion): ≥ 3 keys
- Course 2 (mode buttons): all 8 buttons, press + release
- Strip: touch → slide → release (active=false event must be present)
- Breath: rest → blow → release
- At least one pedal if connected

Duration: 20–30 seconds. File: `TAU_<date>_01.elcf`

After recording: verify with EL-3 harness before proceeding to other devices.

### TAU — Session 2 (required, boundary cases)
Purpose: edge cases that EigenHost Step 7 golden tests will assert.

Target events:
- Quick key press (very short hold, < 50ms active duration)
- ≥ 2 simultaneous key presses
- Strip active=false event where position ≠ 0 (confirms last-known-value behaviour)
- Breath near minimum and maximum values

File: `TAU_<date>_02.elcf`

### PICO — Session 1
Purpose: verify PICO-specific normalisation and button mapping.

Target events:
- Course 0: ≥ 8 keys
- Course 1 mode keys (appear as `button()` events, key=0..3): all 4 buttons
- Strip
- Breath

File: `PICO_<date>_01.elcf`

### ALPHA — Session 1
Purpose: Alpha-specific paths — two strips, percussion (course 1), no mode buttons.

Target events:
- Course 0: ≥ 10 keys
- Course 1 (percussion): ≥ 3 keys
- Both strips (strip 0 and strip 1)
- Breath

File: `ALPHA_<date>_01.elcf`

## Post-Capture Checklist

After each recording: 
1. Check binary header: magic bytes `45 4C 43 46`, correct device_type, event_count > 0
2. Verify with EL-3 replay harness: load + `replaySynchronous` into a printing callback; scan output for active=false events, value ranges
3. Spot-check normalised values: pressure [0,1], roll [-1,1], yaw [-1,1], strip [0,1], breath [-1,1]
4. Confirm active=false events are present for all keys that were pressed (no stuck notes)
5. Run EL-5 contract tests against the new capture before committing
6. Record coverage summary in task Outcome below

## Note on Timing Quality

The capture tool runs on the poll thread (audio thread equivalent). As long as the binary write is deferred to a background thread or done at shutdown (see EL-2), timing data will be representative. Do not run a parallel heavy process during capture — minimise system load to keep timing realistic.

## Decisions
- 2026-04-12 format upgraded to schema v3 (36-byte records, added t_us device timestamp) during session — all tools updated in sync
- 2026-04-12 capture tool bug: setPollTime(10) starved USB; fixed to 100µs + sleep-on-idle in poll loop
- 2026-04-12 PICO breath range slightly exceeds [-1,1] (min -1.195) — hardware calibration, not clamped intentionally; document as characteristic not bug
- 2026-04-12 ALPHA deferred: requires different computer; pedal deferred: not available; neither blocks EL-5

## Outcome
Committed fixtures: `TAU_20260412_01.elcf` (12392 events), `TAU_20260412_02.elcf` (5487 events), `PICO_20260412_01.elcf` (3598 events). All pass magic/version/alignment checks; active=false events present; value ranges within expected bounds (PICO breath slightly outside [-1,1] — hardware characteristic). ALPHA and pedal deferred to a later session on appropriate hardware.
