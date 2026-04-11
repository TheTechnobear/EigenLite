# Technical Requirements

Required behaviour and constraints. These define what EigenLite must do correctly.

---

## API Contract

### Single-Header Public Interface
- All user-visible symbols must be accessible via `#include <eigenapi.h>` alone.
- The public header must not expose any internal types (EigenLite, EF_Harp, etc.).
- Only basic C++ types in public API signatures.

### Callback Ownership
- Callback pointers passed to `addCallback` are owned by the caller.
- They must remain valid until `removeCallback`/`clearCallbacks` is called or the `Eigenharp` object is destroyed.
- Adding the same callback twice must be a no-op (not duplicated).

### Callback Dispatch
- All registered callbacks receive every event.
- Callbacks fire on the application thread (the thread calling `process()`).
- Order of callbacks is registration order.

### Firmware Reader Ownership
- A firmware reader passed to `Eigenharp(IFW_Reader*)` is owned by the caller.
- The default `FWR_Embedded` reader is created and destroyed by `EigenLite` internally when no reader is supplied.

---

## Device Lifecycle Requirements

### Auto-Discovery
- EigenLite must automatically detect newly connected Eigenharps without application interaction.
- Detection must not block `process()` — runs in a separate thread.
- Discovery thread polls every 10 seconds.

### Firmware Loading
- If a device is found in bootloader mode (pre-load product ID), firmware must be uploaded before use.
- After firmware load, the device must be found again within 10 seconds (up to 10×1s retries) before declaring failure.
- Firmware loading must work with both embedded and file-based readers.

### Device Identification
- Each connected device must be uniquely identified by its USB device path string.
- This string is the `dev` parameter in all callbacks and `setLED`.
- The string must be stable for the lifetime of the connection.

### connect/disconnect Symmetry
- Every `connected()` callback must be paired with exactly one `disconnected()` callback.
- `disconnected()` fires when a device is cleanly destroyed.
- `dead()` fires when a device dies unexpectedly (USB error); `disconnected()` follows.

### LED Cleanup
- On `stop()` or device `destroy()`, all LEDs on a connected device must be turned off.

---

## Sensor Data Requirements

### Key Events
- `key()` must fire on every key press and release.
- On press: `active=true`, normalised p/r/y values.
- On release: `active=false`, p=r=y=0.
- course/key addressing must be consistent across `key()` and `setLED()`.

### Button Events
- Pico mode keys (4 buttons, course-1 hardware) must map to `button()` with key=0..3.
- Tau mode keys (8 buttons) must map to `button()` with key=0..7.
- Alpha has no mode keys; only percussion keys (course 1, via `key()`).

### Breath Events
- Breath must be hysteresis-filtered (threshold 0.01) to suppress jitter.
- Pico breath must apply a warm-up period (1000 samples) to establish zero before firing events.

### Strip Events
- Strip must be hysteresis-filtered (threshold 0.01) when active.
- `active=false` events must fire with the last known value (not current raw value).
- Strip 1 exists on all devices. Strip 2 exists on Alpha only.
- Alpha/Tau strips report via `kbd_key` with special sensor IDs; Pico uses a dedicated callback with state machine debouncing.

### Pedal Events
- Pedal must be hysteresis-filtered (threshold 0.01).
- Pedals 1–4 supported (hardware-dependent how many are present).

---

## Normalisation Requirements

All sensor values must be normalised to float before firing callbacks.

| Sensor   | Range     | Notes                              |
|----------|-----------|------------------------------------|
| Pressure | [0, 1]    | Clamped, never negative            |
| Roll     | [-1, 1]   | Bipolar; direction differs Pico vs Alpha/Tau |
| Yaw      | [-1, 1]   | Bipolar; direction differs Pico vs Alpha/Tau |
| Strip    | [0, 1]    | 1.0 = top of strip                 |
| Breath   | [-1, 1]   | Bipolar; Pico uses gain 1.4×       |
| Pedal    | [0, 1]    | Unipolar                           |

Values must be clamped to their stated ranges.

---

## Filter/Device Selection Requirements

### Device Filter
- `setDeviceFilter(0, 0)` must connect all devices.
- `setDeviceFilter(1, 0)` must connect basestations (Alpha/Tau) only.
- `setDeviceFilter(2, 0)` must connect Picos only.
- `setDeviceFilter(x, N)` must connect only the Nth device of the filtered type.

---

## LED Requirements

- LED colour values: OFF=0, GREEN=1, RED=2, ORANGE=3.
- Out-of-range course or key must be silently ignored.
- `setLED(nullptr, ...)` must broadcast to all connected devices.

| Device | Course 0         | Course 1         | Course 2  |
|--------|-----------------|-----------------|-----------|
| Pico   | keys 0–17       | keys 0–3        | —         |
| Tau    | keys 0–71       | keys 0–11       | keys 0–7  |
| Alpha  | keys 0–119      | keys 0–11       | —         |

---

## Platform Requirements

- Must build and run on: macOS (arm64, x86_64), Linux (x86_64, arm7, arm6).
- Windows support is present but limited; `DISABLE_LIBUSB` and `DISABLE_EIGENHARP` flags exist.
- Must support Bela (arm7/Xenomai) as a Linux target.
- Must compile with C++11.

---

## Static Linking Requirement

- The default build must be fully self-contained: no filesystem access required at runtime.
- Firmware must be statically linked via the embedded C++ arrays.
- picodecoder (closed-source) must be available as a static import library per platform/arch.
