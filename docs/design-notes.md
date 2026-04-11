# Design Notes

Current architecture and boundaries. Updated to reflect code as of version 1.0.0.

---

## Layers

```
┌────────────────────────────────────┐
│  Application                       │
│  #include <eigenapi.h>             │
├────────────────────────────────────┤
│  Public API Layer                  │
│  Eigenharp (PIMPL facade)          │
│  Callback (event interface)        │
│  IFW_Reader / FWR_Embedded / Posix │
├────────────────────────────────────┤
│  EigenLite (eigenlite_impl.h)      │
│  Orchestration / discovery / filter│
├────────────────────────────────────┤
│  Device Abstraction (ef_harp.h)    │
│  EF_Harp (base)                    │
│  EF_Pico                           │
│  EF_BaseStation + EF_Alpha/EF_Tau  │
├────────────────────────────────────┤
│  Drivers (from EigenD — don't mod) │
│  pico::active_t   (lib_pico/)      │
│  alpha2::active_t (lib_alpha2/)    │
│  picross/  (platform abstraction)  │
├────────────────────────────────────┤
│  picodecoder (closed source)       │
│  prebuilt .a / .so per platform    │
└────────────────────────────────────┘
```

---

## Public API Layer

**`Eigenharp`** (`eigenapi.h`, `eigenapi.cpp`) — thin PIMPL facade. Holds `void* impl` pointing to an `EigenLite` instance. The public header has zero dependency on internal types. Users see only `Eigenharp`, `Callback`, `IFW_Reader`, and `Logger`.

**`Callback`** — pure virtual interface with default no-op implementations. Multiple callbacks can be registered; all fire for every event.

**`IFW_Reader`** — strategy interface for firmware source. Two concrete implementations: `FWR_Embedded` (default; uses arrays compiled in) and `FWR_Posix` (reads from filesystem path).

---

## EigenLite — Core Orchestrator

`EigenLite` (`eigenlite_impl.h`, `eigenlite.cpp`) manages:

### USB Discovery Thread
A background `std::thread` polls for USB device changes every 10 seconds. It maintains lists of available pico and basestation USB devices by enumerating USB vendor/product IDs. Sets `usbDevChange_ = true` when the list changes.

A `std::atomic_flag` spinlock (`usbDevCheckSpinLock`) prevents the discovery thread and `poll()` from running simultaneous USB checks. If the poll loop is busy, the discovery thread retries in 1ms.

### Poll Loop
`poll()` is called by the application in a loop. It:
1. Checks `usbDevChange_` under the spinlock and dispatches `deviceInfo` callbacks
2. Connects newly discovered devices (respecting device filter)
3. Cleans up `deadDevices_`
4. Calls `poll(t)` on each live device

`pollTime_` (default 100µs) is the minimum interval between polling devices.

### Device Lifecycle
When a new USB device is seen:
- Create `EF_Pico` or `EF_BaseStation` via `new`
- Call `create(usbdev)` — loads firmware if needed, opens driver
- Call `start()` — starts driver loop, fires `connected` callback
- Add to `devices_` vector

When a device dies:
- `fireDeadEvent` is called from device delegate
- Device added to `deadDevices_` set
- Next `poll()` iteration calls `destroy()` and fires `disconnected`

### Device Filter
`filterAllBasePico_` (0/1/2) and `filterDeviceEnum_` (0=all, 1-N=specific) control which discovered devices are connected.

---

## Device Abstraction Layer

### EF_Harp (base)
Manages:
- `pic::usbdevice_t* pDevice_` — USB device handle
- Firmware loading from IHX format (see `docs/reference/firmware.md`)
- Hysteresis filtering for breath/strip/pedal events (threshold 0.01)
- Strip fires last known value on deactivation

Subclasses implement: `create`, `destroy`, `start`, `stop`, `poll`, `setLED`, `restartKeyboard`.

### EF_Pico
Uses `pico::active_t`. The Pico driver does not distinguish devices from their USB path (by name), so `EF_Pico` passes the USB device name to the driver directly.

Notable details:
- Calibration loaded from device on each start
- Strip implemented as a 4-state machine with sample decimation (20x) to filter noise (see `docs/reference/devices.md`)
- Breath has a 1000-sample warm-up to establish zero calibration
- Mode keys (keys 18-21 in driver) are remapped to `button()` events with `key=0..3`
- LED indexing: course 0 = keys 0-17, course 1 = keys 0-3; internal: `course * 18 + key`

### EF_BaseStation
Uses `alpha2::active_t`. Detects Alpha vs Tau at `create()` via USB control request `0xc6`:
- Response `1` → Alpha, creates `EF_Alpha` delegate
- Response `2` → Tau, creates `EF_Tau` delegate
- Other → assumes Alpha

Maintains `curmap_[9]` and `skpmap_[9]` bitmaps (9 words × 16 bits = 144 bits, covering up to 140 keys + 6 sensors).

### EF_Alpha / EF_Tau
Both extend `EF_BaseDelegate` and implement `alpha2::active_t::delegate_t`.

**Alpha:**
- `kbd_key` fires for key presses, strips, and breath
- `kbd_keydown` fires for key releases (bitmap diff approach)
- Keys 0-119: main keys (course 0); keys 120-131: percussion (course 1)
- 2 strips (KBD_STRIP1, KBD_STRIP2)

**Tau:**
- Same bitmap approach
- Keys 0-71: main (course 0); 72-83: percussion (course 1); 89-96: mode → `button()` (course 2 mapped to button)
- 1 strip (TAU_KBD_STRIP1)

Key-off tracking: `kbd_keydown` receives full bitmap; compares against `curmap_` to detect released keys and fires events with `active=false, p=r=y=0`.

---

## Sensor Value Normalisation

Raw integer values from the hardware are normalised to floats in the device delegates. The normalisation constants differ between Pico and Alpha/Tau (see `docs/reference/devices.md` for tables).

Hysteresis filtering happens one layer up in `EF_Harp` for breath, strip, and pedal.

---

## Firmware Loading

Two-phase process: detect bootloader product ID → upload IHX → wait for device to re-enumerate with operational product ID. Implemented in `EF_Harp::loadFirmware` and device-specific `checkFirmware`. See `docs/reference/firmware.md`.

---

## Threading Model

Two threads:
1. **Application thread** — calls `process()` in a loop
2. **Discovery thread** — polls USB every 10s (or 1ms on spinlock contention)

The `atomic_flag` spinlock guards the shared device lists (`availablePicos_`, `availableBaseStations_`, `usbDevChange_`). No other synchronisation primitives are used. Callbacks fire on the application thread.

---

## Build Output

Single static library: `libeigenapi.a` (or `eigenapi.lib` on Windows).

Copied to `dist/eigenapi-{platform}-{arch}.a` after each build.

Users link against this library and include `eigenapi/eigenapi.h`.

---

## EigenD Sync Boundary

The directories below are copied from EigenD. Changes should be made in EigenD first:
- `eigenapi/lib_alpha2/`
- `eigenapi/lib_pico/`
- `eigenapi/picross/`

The remaining `eigenapi/src/` files are EigenLite-specific and are not in EigenD.
