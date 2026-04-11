# Product Overview

## What is EigenLite

EigenLite is a lightweight C++ library that provides a programmatic API for the Eigenharp family of instruments — Pico, Tau, and Alpha. It allows applications to receive sensor data and control LEDs on these instruments without taking on the dependencies or complexity of the full EigenD software stack.

## Problem it Solves

The Eigenharp instruments require firmware loading over USB and low-level driver interaction to function. The original EigenD project supports this but is large, Python-dependent, and built with Scons. EigenLite extracts just the device interaction layer, wraps it in modern C++ (C++11), and builds with CMake — making it practical to embed in audio applications, plugins, and embedded systems.

## Target Users

Developers building applications that receive Eigenharp input. Not an end-user product. Known downstream users:
- [MEC](https://github.com/TheTechnobear/MEC) (TheTechnobear)
- [ECMapper](https://github.com/KaiDrange/ECMapper) (Kai Drange)

## Relationship to EigenD

EigenLite is derived from EigenD (https://github.com/thetechnobear/EigenD), which is the primary maintainer's fork of the original Eigenlabs EigenD. The low-level driver directories are copied from EigenD and should be kept in sync:

- `eigenapi/lib_alpha2/`
- `eigenapi/lib_pico/`
- `eigenapi/picross/`

The EigenLite-specific code lives in `eigenapi/src/` and presents the public abstraction.

The goal is minimal divergence from EigenD's driver code. EigenD improvements should be made there first, then copied here.

## Abstraction Level

Pre-1.x: thin wrapper around the raw drivers (lib_pico, lib_alpha2). Applications had to handle device differences.

1.x and beyond: raised to approximately the "Module" level that EigenD presents via `pico_manager` (plg_pkbd) and `alpha_manager` (plg_keyboard). At this level:
- Device differences are isolated internally
- Sensor values are floats (unipolar [0,1] or bipolar [-1,1]), not raw integers
- Key layout is abstracted into course/key addressing

## Key Design Goals

1. **Simple API** — one header, one class, basic C++ types only
2. **Light wrapper** — minimal logic, expose what the hardware offers
3. **No filesystem dependency** — firmware can be statically linked
4. **Static library** — suitable for embedding in audio applications
5. **Platform portability** — macOS, Linux (x86, arm/Bela), limited Windows

## Current Limitations

- No headphone support (Alpha/Tau only — hardware exists, not implemented)
- No microphone support (Alpha only — no test hardware available)
- Simplified hysteresis vs EigenD configurable windows
- Strip values: absolute only (relative is left to the application)
- Pico key-press LEDs not auto-lit (unlike Alpha/Tau basestation behaviour)
