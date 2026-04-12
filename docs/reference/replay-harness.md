# ReplayHarness

`EigenLite::ReplayHarness` loads `.elcf` binary captures and fires their events into any `EigenApi::Callback`. Used by contract tests and integration tests that need deterministic, hardware-free playback.

Header: `eigenapi/src/replay_harness.h`

---

## Quick Start

```cpp
#include "replay_harness.h"
#include "eigenapi.h"

struct MyCallback : EigenApi::Callback {
    void key(const char*, unsigned long long t,
             unsigned course, unsigned key, bool active,
             float p, float r, float y) override { /* ... */ }
    // ... other overrides
};

EigenLite::ReplayHarness h;
if (!h.loadCapture("session.elcf")) { /* bad file */ }

MyCallback cb;
h.replaySynchronous(&cb);   // fire all events, no timing
```

---

## API

### `bool loadCapture(const std::string& path)`

Loads an `.elcf` file. Returns `false` if the file is missing, has wrong magic, or has an unsupported schema version. On success, `isLoaded()` returns true.

### `void replaySynchronous(EigenApi::Callback* target)`

Fires all events to `target` in order, ignoring both `t_ms` and `t_us`. Use for unit tests that only care about which events fired.

### `void replayTimed(EigenApi::Callback* target, std::function<void(double ms)> sleepFn = nullptr)`

Fires events honouring `t_ms` (wall-clock) gaps between events.

- `sleepFn(ms)` is called with the inter-event gap in milliseconds. Pass `nullptr` to use `std::this_thread::sleep_for`.
- Useful for integration tests that model host-side timing behaviour.
- See **Timing Model** below for the `t_ms` vs `t_us` trade-off.

### Accessors

| Method | Returns |
|---|---|
| `isLoaded()` | `true` after a successful `loadCapture` |
| `eventCount()` | Number of event records loaded |
| `deviceType()` | Device type string from file header: `"TAU"`, `"ALPHA"`, or `"PICO"` |

---

## Timing Model

Each event record carries two independent timestamps:

### `t_ms` — wall-clock elapsed milliseconds

Recorded by the capture tool's `std::chrono::steady_clock`. Reflects the timing the *application* experienced on the capture machine: includes poll-loop scheduling, OS jitter, and USB latency.

- `replayTimed` uses `t_ms` to sleep between events.
- Appropriate for integration tests that model host-side behaviour.
- Can be inflated by host load during capture without affecting `t_us`.

### `t_us` — device hardware timestamp, microseconds

Passed through verbatim from `EigenApi::Callback`'s `t` parameter, which originates in the USB driver. Reflects the timing the *device* saw: hardware-accurate USB packet timestamps, independent of the host OS scheduler.

- Not currently used by `replayTimed`.
- A future `replayTimedUs` would use `t_us` deltas for hardware-accurate inter-event gaps — useful for jitter-sensitive tests (e.g. EigenHost timing). Implement when a concrete need arises.

### Divergence between the two clocks

- They may have different epochs (`t_us` epoch is typically device boot or connection time).
- High host load during capture inflates `t_ms` gaps while `t_us` remains unchanged.
- Neither clock is guaranteed monotonic across connect/disconnect cycles.

---

## Shared format constants

All ELCF magic values, schema version, record size, field offsets, and the `EventType` enum are defined in `eigenapi/src/elcf_format.h`. Both `ReplayHarness` and the capture/convert tools include this header as the single source of truth.

See `docs/reference/capture-format.md` for the full binary format specification.
