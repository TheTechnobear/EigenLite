# Build Reference

---

## Prerequisites

- CMake ≥ 3.28
- C++11 compatible compiler
- Platform-specific USB library:
  - macOS: IOKit + Cocoa (system frameworks, no install needed)
  - Linux: libusb

## CMake Options

| Option                    | Default | Description |
|---------------------------|---------|-------------|
| `USE_DUMMY_PICO`          | OFF     | Use stub picodecoder instead of prebuilt binary. Implies dynamic mode. Pico won't produce data. |
| `EIGENLITE_BUILD_TESTS`   | ON (top-level), OFF (submodule) | Build test executables |
| `CREATE_DIST`             | true    | Copy built library to `dist/` |
| `DISABLE_LIBUSB`          | unset   | Skip libusb subproject (Windows) |
| `DISABLE_EIGENHARP`       | unset   | Skip Eigenharp entirely (Windows) |
| `DISABLE_RPATH_OVERRIDE`  | unset   | Skip RPATH setup |
| `CPU_OPTIMIZATION_FLAGS`  | unset   | Override auto-detected CPU flags |
| `SALT`, `PEPPER`          | unset   | Bela board variants (both imply `BELA`) |

## Platform Builds

### macOS

```bash
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# cross-compile for specific arch:
cmake -B build-arm64 -DCMAKE_OSX_ARCHITECTURES=arm64
cmake -B build-x86   -DCMAKE_OSX_ARCHITECTURES=x86_64
```

Verify architecture: `file dist/eigenapi-macOs-arm64.a`

### Linux x86_64

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Linux arm7 (Bela)

```bash
cmake -B build -DBELA=on -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

For SALT/PEPPER variants: `-DSALT=on` or `-DPEPPER=on`.

### ARM6

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
# ARM6 is auto-detected from CMAKE_SYSTEM_PROCESSOR matching ^armv6l
cmake --build build
```

## Build Output

- Static library: `build/release/lib/libeigenapi.a` (Linux/macOS)
- Copied to: `dist/eigenapi-{platform}-{arch}.a`
- Test executables: `build/release/bin/`

## Using as a Submodule

EigenLite is designed to be consumed as a CMake submodule. Include it from the parent project:

```cmake
add_subdirectory(external/EigenLite eigenapi)
target_link_libraries(myapp PRIVATE eigenapi)
target_include_directories(myapp PRIVATE external/EigenLite/eigenapi)
```

When used as a submodule, `EIGENLITE_BUILD_TESTS` defaults to OFF.

## CMake Presets

`CMakePresets.json` is present (details depend on local config — check file for defined presets).
