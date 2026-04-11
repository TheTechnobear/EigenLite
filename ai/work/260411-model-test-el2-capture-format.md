# Task: EL-2 — Capture Format + CLI Capture Tool
<!-- status: active -->
<!-- created: 2026-04-11  refs: docs/technical-requirements.md, docs/design-notes.md -->

## Goal
Define a compact binary capture format for EigenApi callback events, and build a standalone CLI tool that connects to real Eigenharp hardware and records a session to a capture file. Committed capture files become reusable test fixtures for both EigenLite (EL-5) and EigenHost (Step 7) — no hardware required to run tests.

## Dependencies
- EL-1 complete (not strictly required to build the tool, but both should land together before EL-3/EL-5).

## Why binary, not JSON

The Eigenharp poll loop is designed to run on an audio-priority thread. At typical play rates (multiple pressed keys + breath + strip), the hardware produces 50–100 events/second. JSON serialisation on that thread would introduce IO overhead and skew timing data. The binary format writes fixed-size event records to an in-memory buffer; a background thread flushes to disk. This mirrors the existing EigenLite threading model.

## Scope
**In:**
- `docs/reference/capture-format.md` — binary schema specification
- `tools/capture/capture_main.cpp` — CLI capture tool
- `tests/fixtures/` (new dir) — where committed capture files live
- Minimal CMake addition to build the capture tool

**Out:** replay harness (EL-3), GTest tests (EL-5), any EigenHost-specific code

---

## Binary Format Schema

**Design goals:** fixed-size event records (fast write, easy iteration), no dynamic allocation at capture time, platform-neutral byte order (little-endian explicit), schema version for forward compat.

### File Header (32 bytes, fixed)

```
offset  size  field
0       4     magic: uint32LE = 0x454C4346  ('ELCF')
4       1     schema_version: uint8 = 1
5       8     device_type: char[8], null-padded  (e.g. "TAU\0\0\0\0\0")
13      8     captured_at: int64LE  (Unix timestamp, seconds)
21      11    reserved: uint8[11] = 0
```

### Event Record (32 bytes, fixed, repeated)

```
offset  size  field
0       4     t_ms: uint32LE  (milliseconds since first event)
4       1     event_type: uint8
              0 = key
              1 = breath
              2 = strip
              3 = button
              4 = pedal
              5 = connected
              6 = disconnected
5       1     course: uint8
6       1     key: uint8
7       1     strip_idx: uint8    (strip number; 0 if not a strip event)
8       1     active: uint8       (0 or 1)
9       1     pedal_idx: uint8    (pedal number; 0 if not a pedal event)
10      2     dev_id: uint16LE    (index into device string table; 0 for single-device sessions)
12      4     pressure: float32LE
16      4     roll: float32LE
20      4     yaw: float32LE
24      4     value: float32LE    (breath, strip, or pedal value; 0.0 for other types)
28      4     reserved: uint8[4] = 0
```

Total event size: 32 bytes. At 100 events/second: 3.2 KB/sec. A 60-second session ≈ 192 KB in memory.

### Device String Table (optional — after all events)

For multi-device sessions: a null-terminated string per dev_id index, appended after the event array. For single-device sessions, omit — all dev_id fields are 0 and the dev string is in `device_type` in the header.

Single-device sessions are the common case; the tool can record multi-device sessions if the implementer chooses, but is not required to.

---

## CLI Capture Tool

### Location: `tools/capture/capture_main.cpp`

Standalone C++ program. Connects to EigenApi, captures events to an in-memory buffer on the poll thread, writes binary file from a background thread on exit.

No JUCE dependency — C++11 only: `<chrono>`, `<thread>`, `<atomic>`, `<fstream>`.

### Threading model

```
[poll thread]  →  lock-free ring buffer  →  [writer thread]  →  file
```

Ring buffer size: 65536 records (2 MB). At 100 events/sec this holds ~10 minutes of data. If the buffer fills (pathological case), oldest events are overwritten (or writer stalls — implementer's choice; document which).

A simpler alternative for a capture tool (not real-time audio): `std::vector<EventRecord>` captured on the poll thread, written to file on exit. This is acceptable if poll runs fast enough. Prefer this if ring buffer complexity is not justified.

**Minimum requirement:** the poll thread must never block on file IO. Writing must happen on a separate thread or at shutdown.

### Interface

```
eigenlite-capture [--device TAU|ALPHA|PICO] [--duration-s 30] --out tests/fixtures/TAU_20260411_01.elcf
```

`.elcf` extension = EigenLite Capture File.

### Implementation sketch

```cpp
struct EventRecord {
    uint32_t t_ms;
    uint8_t  event_type, course, key, strip_idx, active, pedal_idx;
    uint16_t dev_id;
    float    pressure, roll, yaw, value;
    uint8_t  reserved[4];
};
static_assert(sizeof(EventRecord) == 32, "EventRecord must be 32 bytes");

class CaptureCallback : public EigenApi::Callback {
    std::vector<EventRecord> events_;  // pre-allocated (reserve 100000)
    std::chrono::steady_clock::time_point start_;

    void key(const char*, int course, int key, bool active,
             float p, float r, float y) override {
        events_.push_back({elapsedMs(), 0, (uint8_t)course, (uint8_t)key,
                           0, (uint8_t)active, 0, 0, p, r, y, 0.f, {}});
    }
    // ... all other callback types

    void writeBinary(const std::string& path);
    uint32_t elapsedMs() { /* chrono */ }
};
```

Write the header + event array in one sequential pass at exit. No dynamic allocation during capture.

### CMake addition

```cmake
if(NOT DISABLE_EIGENHARP)
    add_executable(eigenlite-capture tools/capture/capture_main.cpp)
    target_include_directories(eigenlite-capture PRIVATE eigenapi)
    target_link_libraries(eigenlite-capture PRIVATE eigenapi)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        target_link_libraries(eigenlite-capture PRIVATE
            "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(eigenlite-capture PRIVATE libusb dl pthread)
    endif()
endif()
```

Add picodecoder link (static or dynamic, matching `eigenapitest` pattern).

## Validation
1. Build `eigenlite-capture`
2. Connect Eigenharp hardware
3. Run: `./eigenlite-capture --duration-s 15 --out tests/fixtures/TAU_20260411_01.elcf`
4. Play: keys, breath, strip, buttons
5. Verify output: correct magic bytes (`45 4C 43 46`); correct `device_type` field; event count > 0
6. Write a small reader (or do it in EL-3) to verify event_type/course/key values look sensible
7. Commit capture file to repo

## Naming Convention
`<DEVICE>_<YYYYMMDD>_<N>.elcf` — e.g. `TAU_20260411_01.elcf`. Stored in `tests/fixtures/`.

## Decisions
<!-- Updated during work -->

## Outcome
<!-- Filled on completion — include path to first committed capture file -->
