# Firmware Reference

How EigenLite manages Eigenharp firmware.

---

## Background

Eigenharps use a two-stage USB initialisation:

1. **Power-on / bootloader phase** — device enumerates with a "pre-load" USB product ID and runs only a minimal bootloader.
2. **Operational phase** — after firmware upload, the device re-enumerates with a different product ID. This is the device the user interacts with.

EigenLite handles this transparently: when a pre-load product ID is detected, firmware is uploaded before the device is opened for normal use.

---

## Firmware Files

Three firmware files cover all devices:

| File                 | Constant              | Device                    |
|----------------------|-----------------------|---------------------------|
| `pico.ihx`           | `PICO_FIRMWARE`       | Eigenharp Pico            |
| `bs_mm_fw_0103.ihx`  | `BASESTATION_FIRMWARE`| Basestation (Alpha/Tau)   |
| `psu_mm_fw_0102.ihx` | `PSU_FIRMWARE`        | Basestation PSU variant   |

The Alpha and Tau share a basestation, so they use the same firmware files. The firmware itself does not distinguish them; the instrument type is detected via a USB control query after loading.

---

## IHX Format

Intel HEX (IHX) is a text format for binary data. Each line:

```
:BBAAAATT[DD...]CC
```

- `:` — start code
- `BB` — byte count (2 hex chars)
- `AAAA` — address (4 hex chars)
- `TT` — record type: `00` = data, `01` = EOF
- `DD...` — data bytes (BB pairs)
- `CC` — checksum: `(~(sum of all bytes)) + 1`

EigenLite parses IHX in `EF_Harp::processIHXLine`. Records are transmitted to the device via USB control transfers.

---

## Firmware Loading Process

Implemented in `EF_Harp::loadFirmware` and called from device-specific `checkFirmware` methods.

### Step 1: Check if firmware needed

```
EF_Pico::checkFirmware:
  enumerate USB for (BCTPICO_USBVENDOR, PICO_PRE_LOAD=0x0001)
  if found → load firmware

EF_BaseStation::checkFirmware:
  enumerate for (BCTKBD_USBVENDOR, BASESTATION_PRE_LOAD=0x0002)
  if not found: try (BCTKBD_USBVENDOR, PSU_PRE_LOAD=0x0003)
  if found → load appropriate firmware
```

If the device is already at the operational product ID, firmware loading is skipped.

### Step 2: Upload firmware

```
open USB device (pre-load product ID)
call pic::usbdevice_t::start_pipes()
send reset command to CPU (CPUCS register = 0x01)
loop: read IHX lines, poke data to device via control_out(USB_TYPE_VENDOR, FIRMWARE_LOAD, address, ...)
send run command (CPUCS = 0x00)
detach and close USB device
```

USB control transfer parameters:
- Request type: `0x40` (USB_TYPE_VENDOR)
- Request: `0xa0` (FIRMWARE_LOAD)
- Value: firmware address
- CPUCS address: `0xe600`

### Step 3: Wait for re-enumeration

After firmware upload, the device disconnects and re-enumerates at the operational product ID. EigenLite polls for up to 10 seconds (10 × 1s sleeps) waiting for the new product ID to appear.

---

## Embedded Firmware

For static/embedded deployments, the IHX files are converted to C++ byte arrays:

```
eigenapi/resources/firmware/cpp/
    pico.cpp           → ___ihx_pico_ihx[]
    bs_mm_fw_0103.cpp  → ___ihx_bs_mm_fw_0103_ihx[]
    psu_mm_fw_0102.cpp → ___ihx_psu_mm_fw_0102_ihx[]
```

These are compiled into `libeigenapi.a` by default. `FWR_Embedded` reads from these arrays by matching the filename string to the appropriate array pointer and length.

The IHX files in `eigenapi/resources/firmware/ihx/` are the authoritative source. The C++ arrays are generated from them.

The Eigenharp firmware is not being developed further, so static linking has no practical downside.

---

## picodecoder

The Pico sensor decoder is a separate closed-source library (`libpicodecoder`). It decodes the USB bulk transfer data from the Pico into per-key sensor readings.

### Prebuilt libraries

```
resources/picodecoder/
    macOs/arm64/    libpicodecoder_static.a, libpicodecoder.dylib
    macOs/x86_64/   libpicodecoder_static.a, libpicodecoder.dylib
    linux/arm7_32/  libpicodecoder.so
    linux/x86_64/   libpicodecoder.so
    unsupported/    old i386/win32 versions
```

By default, CMake links the static version for the current platform.

### Stub library

`eigenapi/lib_picodecoder/pico_decoder.c` is an open-source stub that compiles but has no-op implementations. Enable with `USE_DUMMY_PICO=ON` in CMake. This allows building and testing without the prebuilt binary, but the Pico will not produce sensor data.

### Interface

Defined in `eigenapi/resources/pico_decoder.h` (and per-platform copies in `resources/picodecoder/`):

```c
void pico_decoder_create(pico_decoder_t *decoder, int tp);
void pico_decoder_raw(...);    // fire raw key callback
void pico_decoder_cooked(...); // fire normalised key callback
void pico_decoder_cal(...);    // set calibration
```

Decoder types: `PICO_DECODER_PICO=0`, `PICO_DECODER_MICRO=1`.
