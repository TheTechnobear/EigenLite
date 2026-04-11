# Known Issues and Technical Debt

Active limitations and deferred work. Sourced from code analysis and `docs/archive/devnotes.md`.

---

## Unimplemented Hardware Features

### Headphone Support (Alpha/Tau)
- The `alpha2::active_t` API has full headphone control: `headphone_enable`, `headphone_gain`, `headphone_limit`.
- EigenLite never calls these. Not exposed in public API.
- Deferred because EigenLite was originally designed for non-audio-rate usage.
- Risk: unknown whether audio clock initialisation is correct; untested.

### Microphone Support (Alpha)
- `alpha2::active_t` has mic API: `mic_enable`, `mic_gain`, `mic_type`, `kbd_mic` delegate callback.
- Not exposed. No test hardware (Alpha microphone) available.
- `EF_BaseDelegate::kbd_mic` is a no-op stub.

---

## Simplified Hysteresis

EigenD has configurable windows for axis rounding, stepping, and gain per sensor. EigenLite uses fixed constants:
- Breath: ±0.01
- Strip: ±0.01
- Pedal: ±0.01

These values were chosen based on practical observation, not a strict port of EigenD logic. May need tuning for specific use cases.

---

## Strip: Absolute Values Only

EigenLite exposes only the absolute strip position. EigenD (at module level) also exposes relative (delta from touch origin). Computing relative is left to the application.

---

## Pico Key-Press LEDs

On Tau/Alpha, the basestation firmware automatically lights a key orange when pressed (non-configurable at the firmware level). The Pico does not do this. EigenLite does not compensate.

EigenD's pico module implements this in software. EigenLite could do the same but has not.

---

## Auto-Connect Model

EigenLite automatically connects to any discovered Eigenharp (subject to filter). There is no "inform app of available devices, let app decide to connect" API.

This means:
- The filter (`setDeviceFilter`) is the only control mechanism.
- Multi-device scenarios are handled by enumeration order, not application choice.

A future inversion (app-initiated connection) would require careful handling of the basestation's firmware loading and reconnection logic.

---

## EigenD Source Drift

The three directories copied from EigenD (`lib_alpha2/`, `lib_pico/`, `picross/`) can drift from the EigenD 3.0 branch over time. There is no automated tooling to compare or sync them.

Known historical policy: changes to shared code should be made in EigenD first, then copied here. No patches have been applied that would block this.

---

## Compiler Warnings in Driver Code

The EigenD-derived driver code produces compiler warnings (deprecated declarations, etc.). These are suppressed with `-Wno-deprecated-declarations` on macOS rather than fixed, to minimise divergence from EigenD source.

---

## discoverProcessRun Global

`discoverProcessRun` in `eigenlite.cpp` is a `volatile bool` global, not a member of `EigenLite`. This means creating multiple `EigenLite` instances simultaneously would be unsafe (last writer wins on create/destroy).

Not a current concern — typical usage is one instance. The global was introduced in the July 2019 "new reconnecting api" commit (28b4d20) as the simplest way to control the background discovery thread when reconnection was first added. It has not been revisited since. — see `ai/work/260411-discover-thread-cleanup.md`
