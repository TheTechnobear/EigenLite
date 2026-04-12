# Task: EL-3 — Synchronous Replay Harness
<!-- status: done -->
<!-- created: 2026-04-11  refs: docs/technical-requirements.md, docs/design-notes.md -->

## Goal
Implement a self-contained replay harness in EigenLite that loads a binary capture file (EL-2 format) and fires all recorded `EigenApi::Callback` events into a caller-supplied callback. Two modes: synchronous (ignores timestamps — for unit tests) and timed (honours timestamps — for integration tests). No dependency on EigenHost or JUCE.

## Dependencies
- EL-1 (GTest infra) — needed to build harness self-tests
- EL-2 (capture format) — harness must parse that exact binary schema

## Context
Both EigenLite (EL-5) and EigenHost (Steps 6–7) need to inject realistic Eigenharp events into a callback without real hardware. The harness is a generic EigenLite utility — it must not import JUCE, not depend on EigenHost, and must work for any consumer of `libeigenapi.a`.

The binary format (EL-2) uses 32-byte fixed-size event records. The harness reads the file header + event array, then dispatches each record to the appropriate `EigenApi::Callback` method.

## Scope
**In:**
- `eigenapi/src/replay_harness.h` — public header (C++11)
- `eigenapi/src/replay_harness.cpp` — implementation
- `eigenapi/CMakeLists.txt` — add `replay_harness.cpp` to `eigenapi` library sources
- `tests/ReplayHarnessTest.cpp` — harness self-tests (added to `EigenLiteTests`)

**Out:** no changes to `eigenapi.h` (public API), no EigenHost code, no JUCE

## Binary Format Reference (from EL-2)

File header: 32 bytes. Magic `0x454C4346`, schema_version uint8, device_type char[8].

Event record: 32 bytes. Fields: `t_ms` (uint32LE), `event_type` (uint8, 0–6), `course`, `key`, `strip_idx`, `active`, `pedal_idx`, `dev_id` (uint16LE), `pressure`, `roll`, `yaw`, `value` (float32LE each), reserved[4].

Event type enum: 0=key, 1=breath, 2=strip, 3=button, 4=pedal, 5=connected, 6=disconnected.

All multi-byte fields are little-endian. Use explicit byte-order reads — do not assume struct layout matches file layout.

## API

```cpp
// eigenapi/src/replay_harness.h
#pragma once
#include <string>
#include <vector>
#include <functional>

// Forward-declare only — callers include eigenapi.h separately
namespace EigenApi { class Callback; }

namespace EigenLite {

class ReplayHarness {
public:
    // Load binary capture file. Returns false if file missing, magic mismatch,
    // unsupported schema_version, or IO error.
    bool loadCapture(const std::string& path);

    // Fire all events synchronously into target (ignores t_ms). target must not be null.
    void replaySynchronous(EigenApi::Callback* target) const;

    // Fire events honouring t_ms delays.
    // sleepFn(ms) is called between consecutive events — caller controls sleep impl.
    // If sleepFn is nullptr, uses std::this_thread::sleep_for.
    void replayTimed(EigenApi::Callback* target,
                     std::function<void(double ms)> sleepFn = nullptr) const;

    // Number of events loaded (0 if loadCapture not called or failed)
    int eventCount() const;

    // Device type string from header ("TAU", "ALPHA", "PICO", etc.)
    std::string deviceType() const;

    // True if last loadCapture call succeeded
    bool isLoaded() const;
};

} // namespace EigenLite
```

## Implementation Notes

### File reading

Read header as 32 raw bytes; extract fields with explicit byte-order logic — do NOT cast the file buffer to a struct (struct padding is compiler-specific).

```cpp
bool ReplayHarness::loadCapture(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    uint8_t header[32];
    if (!f.read(reinterpret_cast<char*>(header), 32)) return false;

    uint32_t magic = readU32LE(header, 0);
    if (magic != 0x454C4346u) return false;

    uint8_t version = header[4];
    if (version != 1) return false;   // unsupported version

    // device_type: bytes 5..12 null-padded
    deviceType_ = std::string(reinterpret_cast<char*>(header + 5), 8);
    deviceType_.erase(deviceType_.find('\0'));  // trim null padding

    // Read events
    while (f) {
        uint8_t rec[32];
        if (!f.read(reinterpret_cast<char*>(rec), 32)) break;
        events_.push_back(parseRecord(rec));
    }
    loaded_ = true;
    return true;
}
```

Utility functions: `readU32LE(buf, offset)`, `readF32LE(buf, offset)`, `readU16LE(buf, offset)` — all explicit byte-order reads.

### Dispatching events

```cpp
void ReplayHarness::replaySynchronous(EigenApi::Callback* target) const {
    for (const auto& ev : events_) {
        dispatch(ev, target);
    }
}

void ReplayHarness::dispatch(const Event& ev, EigenApi::Callback* target) const {
    const char* dev = ev.dev.c_str();
    switch (ev.type) {
        case 0: target->key(dev, ev.course, ev.key, ev.active, ev.pressure, ev.roll, ev.yaw); break;
        case 1: target->breath(dev, ev.value); break;
        case 2: target->strip(dev, ev.strip_idx, ev.value, ev.active); break;
        case 3: target->button(dev, ev.key, ev.active); break;
        case 4: target->pedal(dev, ev.pedal_idx, ev.value); break;
        case 5: target->connected(dev, deviceType_.c_str()); break;
        case 6: target->disconnected(dev); break;
        default: break;  // unknown types: skip (forward compat)
    }
}
```

Confirm `EigenApi::Callback` method signatures from `eigenapi/eigenapi.h` before writing. Match exact signatures.

`dev` string: for single-device captures (dev_id = 0), use `deviceType_` as a stable dev string — sufficient for test purposes. For multi-device captures, look up the device string table (EL-2 optional extension; skip if not present).

### Timed replay

```cpp
void ReplayHarness::replayTimed(EigenApi::Callback* target,
                                std::function<void(double ms)> sleepFn) const {
    uint32_t prev_t = 0;
    for (const auto& ev : events_) {
        double gap = static_cast<double>(ev.t_ms) - static_cast<double>(prev_t);
        if (gap > 0) {
            if (sleepFn) sleepFn(gap);
            else std::this_thread::sleep_for(
                     std::chrono::microseconds(static_cast<int64_t>(gap * 1000)));
        }
        dispatch(ev, target);
        prev_t = ev.t_ms;
    }
}
```

## Harness Self-Tests (`tests/ReplayHarnessTest.cpp`)

These test the harness itself, not the EigenApi contract (that is EL-5).

- `loadCapture` with non-existent path → returns false; `isLoaded()` = false
- `loadCapture` with wrong magic bytes → returns false
- `loadCapture` with schema_version ≠ 1 → returns false
- `loadCapture` with truncated header → returns false
- `loadCapture` with valid header but zero events → returns true; `eventCount()` = 0
- Unknown event_type in record → loaded successfully; that event skipped; expected events still fire
- `replaySynchronous` with known event sequence → fires correct callback methods in order with correct args

For the last test: write a small helper that creates a minimal valid binary capture in memory (or a temp file), loads it, replays, and asserts call counts and field values with a counting callback.

**Test binary generation helper:**
```cpp
std::vector<uint8_t> makeTestCapture(const std::string& deviceType,
                                      const std::vector<TestEvent>& events);
```
Writes header + event records in binary format to a `vector<uint8_t>`. Tests write to a `juce::TemporaryFile` or `tmpnam`-based temp path.

## Adding to CMake

In `tests/CMakeLists.txt`:
```cmake
target_sources(EigenLiteTests PRIVATE ReplayHarnessTest.cpp)
```

In `eigenapi/CMakeLists.txt`, add `src/replay_harness.cpp` to eigenapi sources.

## Validation
```
cmake --build build --target EigenLiteTests
./build/release/bin/EigenLiteTests --gtest_filter=ReplayHarness*
```
All harness self-tests pass. No hardware required.

## Decisions
- 2026-04-12 `connected` takes `DeviceType` enum not string — adjusted from task spec; derive enum from header's device_type field
- 2026-04-12 `key/breath/strip/button/pedal` callbacks take `unsigned long long t` — pass `ev.t_ms` cast to ull
- 2026-04-12 include path: tests use `PROJECT_SOURCE_DIR/eigenapi` base; harness.cpp uses `../eigenapi.h`

## Outcome
Implemented `ReplayHarness` in `eigenapi/src/replay_harness.{h,cpp}`. Added to `eigenapi` library sources. 8 harness self-tests in `tests/ReplayHarnessTest.cpp` — all pass. No hardware required. Nothing deferred.
