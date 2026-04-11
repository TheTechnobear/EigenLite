# Architectural Patterns

Patterns consistently applied in this codebase.

---

## PIMPL (Pointer to Implementation)

**Where:** `Eigenharp` class in `eigenapi.h` / `eigenapi.cpp`.

`Eigenharp` holds `void* impl` which is cast to `EigenLite*` internally. The public header has no `#include` of any internal type. All public methods delegate directly to the implementation.

**Why:** Keeps the public header clean and stable. Users of the library are fully isolated from internal headers, driver dependencies, and STL containers in the implementation.

---

## Strategy (Firmware Reader)

**Where:** `IFW_Reader` interface with `FWR_Embedded` and `FWR_Posix` implementations.

The firmware loading code in `EF_Harp::loadFirmware` / `processIHXLine` is written against the `IFW_Reader` interface. The concrete reader is injected into `EigenLite` at construction time.

**Why:** Allows the same IHX parsing code to work with embedded firmware (no filesystem) or file-based firmware. Applications on embedded targets can use the default embedded reader; development workflows can use posix reader for quick iteration without recompile.

---

## Observer / Callback Broadcast

**Where:** `Callback` interface + `std::vector<Callback*> callbacks_` in `EigenLite`.

All `fire*` methods in `EigenLite` iterate the callback vector and call every registered observer. `EF_Harp` and its subclasses call `efd_.fire*()`, not callbacks directly.

**Why:** Decouples event producers (devices) from consumers (application). Supports multiple consumers (e.g., a MIDI processor and a UI monitor both registered at once).

---

## Template Method (Device Lifecycle)

**Where:** `EF_Harp` base class with `create/destroy/start/stop/poll` virtual methods.

`EF_Harp` provides shared firmware loading, USB device creation, and hysteresis filtering. Subclasses (`EF_Pico`, `EF_BaseStation`) override to add device-specific behaviour and call the base class method.

**Why:** Avoids duplicating USB device lifecycle code. The base class owns the `pic::usbdevice_t*` and common sensor filtering.

---

## Delegate (Driver Event Handling)

**Where:** `pico::active_t::delegate_t` and `alpha2::active_t::delegate_t`.

Each device creates an inner class or nested class that implements the driver's delegate interface. The delegate translates driver-level callbacks (raw integers, driver key numbering) into normalised EigenLite events.

- `EF_Pico::Delegate` — inner class of EF_Pico
- `EF_BaseDelegate` — shared base for EF_Alpha and EF_Tau
- `EF_Alpha`, `EF_Tau` — extend `EF_BaseDelegate`

**Why:** Isolates driver-specific translation logic. EF_Alpha and EF_Tau share the same value normalisation helpers in EF_BaseDelegate but have different key layout logic.

---

## State Machine (Pico Strip)

**Where:** `EF_Pico::Delegate::kbd_strip` in `ef_pico.cpp`.

The Pico strip hardware sends very noisy raw data. A 4-state machine handles debouncing and activation:

```
IDLE (0) → STARTING (1) → ACTIVE (2) → ENDING (3) → IDLE (0)
```

Sample decimation (fires every 20th sample in states 1-3; every 100th sample in state 0) reduces CPU load and noise. Transitions based on threshold crossings and large deltas (>200 raw units).

**Why:** The strip hardware requires active filtering to produce stable float values. The EigenD pico module does similar filtering.

---

## Bitmap Diff (Alpha/Tau Key Release)

**Where:** `EF_Alpha::kbd_keydown` and `EF_Tau::kbd_keydown` in `ef_alpha.cpp` / `ef_tau.cpp`.

The alpha2 driver fires `kbd_keydown` with a full 9-word bitmap (144 bits) of currently pressed keys. It does not directly report key-up events. EigenLite maintains `curmap_[9]` and computes the diff against the incoming bitmap to detect released keys.

`skpmap_[9]` suppresses spurious key-up firings when a key was already up in the previous bitmap.

**Why:** The driver's model is "here is what's down now" rather than "this key just changed". EigenLite converts this to the event model the API presents.

---

## Spinlock for Cross-Thread Safety

**Where:** `usbDevCheckSpinLock` (`std::atomic_flag`) in `EigenLite`.

The discovery thread and `poll()` both access the USB device lists. Rather than a mutex, a `test_and_set` spinlock is used. If `poll()` holds it, the discovery thread backs off for 1ms and retries. If discovery holds it, `poll()` skips device-change handling that cycle.

**Why:** USB enumeration is infrequent (every 10s). A mutex would be heavier than needed. The patterns of access (short, non-blocking sections) suit a spinlock. The `volatile bool usbDevChange_` flag is set by discovery and checked by poll.
