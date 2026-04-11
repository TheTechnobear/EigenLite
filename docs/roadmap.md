# Roadmap

Future plans and ideas. Sourced from `docs/archive/devnotes.md` and code analysis.

---

## Near Term

### Discover Thread Cleanup
Move `discoverProcessRun` from global `volatile bool` to `std::atomic<bool>` member of `EigenLite`. Add condition variable so `destroy()` wakes the thread immediately rather than waiting up to 10s. — see `ai/work/260411-discover-thread-cleanup.md`

### EigenD Source Sync Tooling
Automate comparison between EigenLite's driver directories and EigenD source, and track intentional divergences. — see `ai/work/260411-eigend-sync-agents.md`

### Pico Key-Press LEDs
Implement software LED lighting when a Pico key is pressed, matching the automatic orange-on-press behaviour that Alpha/Tau basestations provide in firmware. Already done in EigenD's pico module.

---

## Medium Term

### Headphone Support
Expose Alpha/Tau headphone control via the public API:
- Requires verifying audio clock initialisation in `alpha2::active_t`
- Needs API design (enable, gain, limit)
- Blocked by: unknown initialisation state; needs testing

### Relative Strip Values
Expose relative strip position (delta from touch origin) in addition to the current absolute value. EigenD module level already computes this.

---

## Design Considerations (Not Committed)

### App-Initiated Connection Model
Currently EigenLite auto-connects all discovered devices. An alternative:
- Inform application of available devices (already done via `deviceInfo` callbacks)
- Application explicitly requests connection to a specific device
- Removes need for `setDeviceFilter` as a workaround

Complication: firmware loading happens during discovery; the model would need to decouple detection from firmware loading from full connection.

### Microphone Support
Expose Alpha microphone input via `kbd_mic` callback path. Blocked by: no test hardware. Low priority.
