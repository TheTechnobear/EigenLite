# Task: EL-5 — EigenLite API Contract Tests
<!-- status: active -->
<!-- created: 2026-04-11  refs: docs/technical-requirements.md, docs/design-notes.md -->

## Goal
Write GTest tests that verify EigenLite's `EigenApi::Callback` contract against committed capture files. These tests assert: given a known capture, the replay harness fires the expected sequence of callbacks with the correct values. Future changes to EigenLite that alter callback behaviour will fail these tests.

## Dependencies
- EL-1 (GTest infra)
- EL-3 (replay harness — both as library and `ReplayHarness` class)
- EL-4 (committed capture files in `tests/fixtures/`)

## Context
These are EigenLite-side contract tests. They validate normalisation, event ordering, and API behaviour. They do NOT test EigenHost routing logic (that is EigenHost Steps 6–7).

The tests use the replay harness to fire captured events into a recording callback, then assert on the results. No real hardware needed to run these tests.

## Test File: `tests/ApiContractTest.cpp`

### Fixture

```cpp
class RecordingCallback : public EigenApi::Callback {
public:
    struct Event { std::string type, dev; int course, key, strip, pedal; bool active; float p, r, y, value; };
    std::vector<Event> events_;
    void key(const char* dev, int course, int key, bool active, float p, float r, float y) override {
        events_.push_back({"key", dev, course, key, 0, 0, active, p, r, y, 0.f});
    }
    // ... all other callback types
    int countType(const std::string& t) const;
    const Event* firstOfType(const std::string& t) const;
};

class ApiContractTest : public ::testing::Test {
protected:
    EigenLite::ReplayHarness harness_;
    RecordingCallback cb_;
    // Capture files are binary .elcf format (EL-2 schema)
    std::string fixturePath(const std::string& name) {
        return std::string(FIXTURES_DIR) + "/" + name;  // CMake defines FIXTURES_DIR
    }
};
```

Add to `tests/CMakeLists.txt`:
```cmake
target_compile_definitions(EigenLiteTests PRIVATE
    FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures")
```

Capture files use `.elcf` extension (EigenLite Capture File, binary). Load with `harness_.loadCapture(fixturePath("TAU_20260411_01.elcf"))`. Do not attempt to parse the binary directly in tests — use `ReplayHarness` exclusively.

### Contracts to Test

#### Normalisation (ref `docs/technical-requirements.md#normalisation-requirements`)

For each event type in each available capture:
- `key` pressure: all values in [0, 1]; active=true events have p > 0; active=false events have p = 0
- `key` roll: all values in [-1, 1]
- `key` yaw: all values in [-1, 1]
- `breath` value: in [-1, 1]
- `strip` value: in [0, 1]
- `pedal` value: in [0, 1]

These bounds are from the spec. If the code produces values outside the specified range, that is a finding.

#### Event pairing (ref `docs/technical-requirements.md#device-lifecycle-requirements`)
- Every `key` active=true must have a corresponding `key` active=false for the same (course, key) — no stuck-key events in capture (validates capture quality too)
- Strip active=false event fires with last known value (not 0): find a strip release event and assert value ≠ 0 when the strip was not at position 0

#### Course/key addressing consistency
- `key` events reference courses and key indices that are within the documented range for the device type:
  - TAU course 0: keys 0–71; course 1: keys 0–11; course 2 mapped to `button()` (not `key()`)
  - PICO course 0: keys 0–17; course 1 (mode keys) mapped to `button()` (not `key()`)
  - ALPHA course 0: keys 0–119; course 1: keys 0–11

#### Button mapping (TAU)
- TAU mode keys (hardware course 2, keys 89–96) must appear as `button()` events with key=0..7, NOT as `key()` events
- Assert: no `key` event has course=2 for TAU captures
- Assert: `button` events are present with key values in [0,7]

#### Button mapping (PICO)
- Pico mode keys (driver keys 18–21) must appear as `button()` events with key=0..3
- (Requires PICO capture from EL-4)

#### Hysteresis
Not directly testable from captures (hysteresis is applied before events reach the capture). Document as a known gap.

#### connected/disconnected symmetry
- Every `connected` event has exactly one matching `disconnected` event for the same `dev`

### Coverage Gap Analysis
After writing tests against TAU captures:
- List which contracts can only be verified with ALPHA or PICO captures
- Mark these as "pending hardware" — add tests when captures become available

## Adding `ApiContractTest.cpp` to CMake

In `tests/CMakeLists.txt`, add `ApiContractTest.cpp` to `EigenLiteTests`:
```cmake
add_executable(EigenLiteTests
    main_test.cpp
    SmokeTest.cpp
    ReplayHarnessTest.cpp   # from EL-3
    ApiContractTest.cpp
)
```

## Authoring Rules
- Derive expected bounds from `docs/technical-requirements.md#normalisation-requirements`, NOT from observing what the code currently produces
- If a capture value is outside spec bounds, that is a finding — record it before writing the test
- Comment every non-obvious assertion with: what contract, what spec reference, any findings resolved

## Decisions
<!-- Updated during work -->

## Outcome
<!-- Filled on completion — list passing tests and any findings raised -->
