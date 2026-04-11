# Device Reference

Detailed specifications for each Eigenharp model as exposed by EigenLite.

---

## Pico

**Driver:** `pico::active_t` (`eigenapi/lib_pico/pico_active.h`)  
**USB Vendor:** `BCTPICO_USBVENDOR`  
**Pre-load product ID:** `0x0001`  
**Operational product ID:** `BCTPICO_USBPRODUCT`  
**Firmware file:** `pico.ihx`

### Key Layout

| Course | Key Range | Count | Event Type |
|--------|-----------|-------|------------|
| 0      | 0–17      | 18    | `key()`    |
| 1      | 0–3       | 4     | `button()` |

Course 1 keys are the 4 "mode" buttons on the Pico body. They fire `button()` not `key()`.

### Sensors

| Sensor  | Count | Notes |
|---------|-------|-------|
| Strip   | 1     | Touch strip; state-machine debounced |
| Breath  | 1     | 1000-sample warm-up calibration |

No pedal support. No percussion course.

### LED Addressing

```cpp
setLED(dev, 0, key, colour);  // main keys 0–17
setLED(dev, 1, key, colour);  // mode buttons 0–3
```

### Value Normalisation (Pico)

| Sensor   | Raw range         | Float range | Formula |
|----------|-------------------|-------------|---------|
| Pressure | 0–3192            | [0, 1]      | `v / 3192.0` clamped |
| Roll     | 0–1024            | [-1, 1]     | `v / 1024.0` clamped |
| Yaw      | 0–1024            | [-1, 1]     | `v / 1024.0` clamped |
| Strip    | 110–3050          | [0, 1]      | `1 - (v - 110) / 2940.0` clamped; 1.0=top |
| Breath   | centred at 2047   | [-1, 1]     | `((v - 2047) / 2047.0) * 1.4` clamped |

### Strip State Machine

The Pico strip fires `kbd_strip` rapidly with raw ADC values. A state machine in `EF_Pico::Delegate` debounces this:

```
State 0 (IDLE):     wait for value > threshold (65); poll every 100 samples
State 1 (STARTING): confirm still above threshold; poll every 100 samples
State 2 (ACTIVE):   fire strip events; poll every 20 samples
                    → if value < threshold OR delta > 200: goto state 3
State 3 (ENDING):   poll every 80 samples
                    → if still below threshold: fire val=0, goto state 0
                    → if above threshold: goto state 2 (resume)
```

Threshold = 65 (raw). Min = 110, Max = 3050.

### Breath Warm-up

For the first 1000 breath samples, EigenLite accumulates without firing events. After 1000 samples the current value is stored as `breathZero_`. Subsequent events fire `fv - breathZero_` to remove mechanical offset.

---

## Tau

**Driver:** `alpha2::active_t` (`eigenapi/lib_alpha2/alpha2_active.h`)  
**Basestation USB Vendor:** `BCTKBD_USBVENDOR`  
**Pre-load product IDs:** `0x0002` (basestation), `0x0003` (PSU variant)  
**Operational product IDs:** `BCTKBD_USBPRODUCT` (BS), `0x0105` (PSU)  
**Firmware files:** `bs_mm_fw_0103.ihx` / `psu_mm_fw_0102.ihx`  
**Detection:** USB control request `0xc6`, response byte `2`

### Key Layout

Internal (driver) key numbering vs EigenLite course:key addressing:

| Driver keys | EigenLite      | Event     |
|-------------|----------------|-----------|
| 0–71        | course=0, 0–71 | `key()`   |
| 72–83       | course=1, 0–11 | `key()`   |
| 89–96       | button 0–7     | `button()`|

Driver keys 84 (DESENSE), 85 (BREATH1), 86 (BREATH2), 87 (STRIP1), 89 (ACCEL) are sensor channels, not physical keys.

### Sensors

| Sensor      | Driver ID          | Notes |
|-------------|-------------------|-------|
| Breath      | TAU_KBD_BREATH1 (85) | |
| Strip 1     | TAU_KBD_STRIP1 (87)  | |
| Pedal 1–4   | `pedal_down()` delegate method | |

### LED Addressing

```cpp
setLED(dev, 0, key, colour);  // main keys 0–71
setLED(dev, 1, key, colour);  // percussion 0–11
setLED(dev, 2, key, colour);  // mode keys 0–7
```

Internal LED numbering: main=0–71, perc=72–83 (+72), mode=84–91 (+84).

### Value Normalisation (Tau / Alpha shared base)

| Sensor   | Raw range | Float range | Formula |
|----------|-----------|-------------|---------|
| Pressure | 0–4096    | [0, 1]      | `v / 4096.0` clamped |
| Roll     | 0–4096    | [-1, 1]     | `(2047 - v) / 1024.0` clamped |
| Yaw      | 0–4096    | [-1, 1]     | `(2047 - v) / 1024.0` clamped |
| Strip    | 0–4096    | [0, 1]      | `1 - v / 4096.0` clamped; 1.0=top |
| Breath   | centred 2047 | [-1, 1]  | `(v - 2047) / 2047.0` clamped |
| Pedal    | 0–4096    | [0, 1]      | `v / 4096.0` clamped |

Note: Roll/Yaw direction is inverted relative to Pico (EigenD normalises to the same direction).

### Key-Off Detection

The alpha2 driver reports `kbd_keydown(t, bitmap)` with a full 9×16-bit bitmap of currently held keys. EigenLite diffs this against `curmap_[9]` to detect released keys and fires `key(... active=false, p=r=y=0)`.

`skpmap_[9]` is ANDed with the incoming bitmap each frame to handle edge cases where a key appears to release but was never pressed in EigenLite's view.

---

## Alpha

**Driver:** `alpha2::active_t` (same as Tau)  
**Detection:** USB control request `0xc6`, response byte `1` (or any value other than `2`)  
**Firmware:** Same as Tau (shared basestation)

### Key Layout

| Driver keys | EigenLite       | Event     |
|-------------|-----------------|-----------|
| 0–119       | course=0, 0–119 | `key()`   |
| 120–131     | course=1, 0–11  | `key()`   |

Driver constants: `KBD_KEYS = 132`, `KBD_BREATH1 = 133`, `KBD_STRIP1 = 135`, `KBD_STRIP2 = 136`.

### Sensors

| Sensor      | Driver ID     | Notes |
|-------------|--------------|-------|
| Breath      | KBD_BREATH1 (133) | |
| Strip 1     | KBD_STRIP1 (135)  | |
| Strip 2     | KBD_STRIP2 (136)  | Alpha only |
| Pedal 1–4   | `pedal_down()` delegate | |

### LED Addressing

```cpp
setLED(dev, 0, key, colour);  // main keys 0–119
setLED(dev, 1, key, colour);  // percussion 0–11
```

Internal LED numbering: main=0–119, perc=120–131 (`course * 120 + key`).

### Value Normalisation

Same as Tau (see table above).

---

## Comparison Summary

| Feature          | Pico  | Tau   | Alpha |
|-----------------|-------|-------|-------|
| Main keys        | 18    | 72    | 120   |
| Percussion keys  | —     | 12    | 12    |
| Mode buttons     | 4     | 8     | —     |
| Strips           | 1     | 1     | 2     |
| Breath           | yes   | yes   | yes   |
| Pedals           | —     | 4     | 4     |
| Headphones       | —     | yes*  | yes*  |
| Microphone       | —     | —     | yes*  |

\* Hardware exists but not implemented in EigenLite.

---

## USB Product IDs

| Device               | Vendor ID | Pre-load PID | Operational PID |
|----------------------|-----------|--------------|-----------------|
| Pico                 | BCTPICO_USBVENDOR | 0x0001 | BCTPICO_USBPRODUCT |
| BaseStation (BS)     | BCTKBD_USBVENDOR  | 0x0002 | BCTKBD_USBPRODUCT  |
| BaseStation (PSU)    | BCTKBD_USBVENDOR  | 0x0003 | 0x0105             |

The PSU variant is a different power supply board for the basestation; same firmware handling path, different product IDs and firmware file.
