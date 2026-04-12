# API Reference

Single header: `#include <eigenapi.h>`. All symbols are in the `EigenApi` namespace.

---

## `Eigenharp` — Main Class

The primary entry point. Uses PIMPL; all internals are hidden.

```cpp
EigenApi::Eigenharp myD;          // uses embedded firmware reader
EigenApi::Eigenharp myD(&fwr);    // supply custom firmware reader
```

### Lifecycle

```cpp
bool start();    // start USB discovery thread, initialise platform
bool process();  // call in a tight loop; polls devices, dispatches events
bool stop();     // stop discovery, destroy all devices, turn off LEDs
```

`start()` returns false only on severe platform failure. `process()` returns true when it did meaningful work (device found or polled). Call it from a dedicated thread or your audio process callback.

```cpp
// minimal usage pattern
myD.start();
while (running) {
    myD.process();
}
myD.stop();
```

### Callbacks

```cpp
void addCallback(Callback* api);
void removeCallback(Callback* api);
void clearCallbacks();
```

Caller owns the `Callback*` pointer. Remains valid until explicitly removed or the `Eigenharp` is destroyed.

Multiple callbacks are supported; all receive every event.

### LED Control

```cpp
enum LedColour { LED_OFF, LED_GREEN, LED_RED, LED_ORANGE };
void setLED(const char* dev, unsigned course, unsigned key, LedColour colour);
```

`dev` = USB device identifier from `connected()` callback. Pass `nullptr` or empty string to broadcast to all connected devices.

Key addressing matches key event addressing (see key layout in `docs/reference/devices.md`).

LED states are cleared on `stop()`.

### Configuration

```cpp
void setPollTime(unsigned pollTimeUs);  // default 100µs minimum between polls
void setDeviceFilter(unsigned allBasePico, unsigned devEnum);
```

**setDeviceFilter:**
- `allBasePico`: 0 = all devices, 1 = basestations only (Alpha/Tau), 2 = Pico only
- `devEnum`: 0 = all in category, 1-N = specific device by USB enumeration order

### Info

```cpp
const char* versionString();   // returns "1.0.0"
```

---

## `Callback` — Event Interface

Subclass and override the methods you need. Default implementations are no-ops.

```cpp
class MyCallback : public EigenApi::Callback {
    void connected(const char* dev, DeviceType dt) override { ... }
    void key(const char* dev, unsigned long long t,
             unsigned course, unsigned key, bool active,
             float p, float r, float y) override { ... }
    // ...
};
```

### Device Events

```cpp
// Called when USB device list changes (before connection attempt)
virtual void beginDeviceInfo();
virtual void deviceInfo(bool isPico, unsigned devEnum, const char* dev);
virtual void endDeviceInfo();

// Called when a device is fully initialised and ready
virtual void connected(const char* dev, DeviceType dt);
// dt = PICO | TAU | ALPHA

// Called when a device is removed or destroyed
virtual void disconnected(const char* dev);

// Called when firmware/USB error causes device to die
virtual void dead(const char* dev, unsigned reason);
```

### Sensor Events

All sensor values are normalised floats. Timestamps are microseconds.

> **Hardware note:** the stated ranges are nominal. Individual devices may produce values slightly outside these bounds due to factory calibration differences (e.g. breath sensors on some units have been observed slightly below −1.0 at rest). Do not hard-clamp on these ranges; prefer tolerance when validating or displaying values.

```cpp
// Key press/release
virtual void key(const char* dev, unsigned long long t,
                 unsigned course, unsigned key, bool active,
                 float p, float r, float y);
// active = true on press, false on release
// p = pressure [0, 1]  (nominal)
// r = roll     [-1, 1] (nominal)
// y = yaw      [-1, 1] (nominal)
// On release: p=r=y=0

// Mode/percussion button press
virtual void button(const char* dev, unsigned long long t,
                    unsigned key, bool active);

// Breath controller
virtual void breath(const char* dev, unsigned long long t, float val);
// val = [-1, 1] (bipolar, nominal); hysteresis-filtered

// Touch strip
virtual void strip(const char* dev, unsigned long long t,
                   unsigned strip, float val, bool active);
// strip = 1 or 2 (Alpha has 2; Tau/Pico have 1)
// val = [0, 1] (nominal), 1.0 = top of strip; hysteresis-filtered
// active = true while finger is on strip

// Pedal (Alpha/Tau only)
virtual void pedal(const char* dev, unsigned long long t,
                   unsigned pedal, float val);
// pedal = 1..4; val = [0, 1] (nominal); hysteresis-filtered
```

---

## `IFW_Reader` — Firmware Reader Interface

Abstracts where firmware bytes come from. Two implementations provided:

### `FWR_Embedded`

Uses firmware arrays compiled into the library. Default when no reader is supplied.

```cpp
EigenApi::Eigenharp myD;              // implicitly creates FWR_Embedded
// or:
EigenApi::FWR_Embedded fwr;
EigenApi::Eigenharp myD(&fwr);
```

### `FWR_Posix`

Reads `.ihx` files from a directory on disk.

```cpp
EigenApi::FWR_Posix fwr("/path/to/ihx/files/");
if (!fwr.confirmResources()) { /* files not found */ }
EigenApi::Eigenharp myD(&fwr);
```

Required files in the directory:
- `pico.ihx`
- `bs_mm_fw_0103.ihx`
- `psu_mm_fw_0102.ihx`

### Custom `IFW_Reader`

Implement the interface to load firmware from any source (e.g., bundled resources):

```cpp
class IFW_Reader {
    virtual bool open(const char* filename, int oFlags, void** fd) = 0;
    virtual long read(void* fd, void* data, long byteCount) = 0;
    virtual void close(void* fd) = 0;
    virtual bool confirmResources() = 0;
};
```

---

## `Logger` — Optional Logging Hook

```cpp
EigenApi::Logger::setLogFunc([](const char* msg) {
    printf("%s\n", msg);
});
```

If not set, log messages are silently discarded. Set before calling `start()`.

---

## Firmware File Constants

Defined in `eigenapi.h`:

```cpp
#define PICO_FIRMWARE        "pico.ihx"
#define BASESTATION_FIRMWARE "bs_mm_fw_0103.ihx"
#define PSU_FIRMWARE         "psu_mm_fw_0102.ihx"
```

---

## Full Usage Example

```cpp
#include <eigenapi.h>
#include <thread>

class MyCallback : public EigenApi::Callback {
public:
    void connected(const char* dev, DeviceType dt) override {
        printf("connected: %s type=%d\n", dev, dt);
    }
    void key(const char* dev, unsigned long long t,
             unsigned course, unsigned key, bool a,
             float p, float r, float y) override {
        if (a) printf("key %u:%u press  p=%.3f r=%.3f y=%.3f\n", course, key, p, r, y);
    }
    void breath(const char* dev, unsigned long long t, float val) override {
        printf("breath %.3f\n", val);
    }
};

int main() {
    EigenApi::Logger::setLogFunc([](const char* m){ printf("[EL] %s\n", m); });

    EigenApi::Eigenharp eh;
    MyCallback cb;
    eh.addCallback(&cb);
    eh.setPollTime(100);

    if (!eh.start()) return 1;

    bool running = true;
    std::thread t([&]{ while (running) eh.process(); });
    // ... wait for signal ...
    running = false;
    t.join();
    eh.stop();
}
```
