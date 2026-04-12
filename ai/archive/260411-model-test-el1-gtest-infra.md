# Task: EL-1 — Add GTest Infrastructure
<!-- status: complete -->
<!-- created: 2026-04-11  refs: docs/technical-requirements.md, docs/design-notes.md -->

## Goal
Add GoogleTest as a CMake dependency to EigenLite and create an `EigenLiteTests` target so that automated unit and contract tests can be built and run as part of the EigenLite project. This is a prerequisite for EL-2 through EL-5.

## Context
EigenLite currently has one test target: `eigenapitest` (`tests/CMakeLists.txt`). This is a manual integration test that requires connected hardware — it is not a GTest binary. A proper test binary is needed so contract tests (EL-5) can be added and run without hardware.

EigenLite must stay C++11 compatible (`CMakeLists.txt` sets `CMAKE_CXX_STANDARD 11`). GTest v1.14 supports C++11.

## Scope
**In:**
- Root `CMakeLists.txt` — add `FetchContent` for GTest, add `BUILD_TESTING` guard for test subtree
- `tests/CMakeLists.txt` — add `EigenLiteTests` target alongside existing `eigenapitest`
- `tests/main_test.cpp` — minimal GTest entry point
- `tests/SmokeTest.cpp` — trivial passing test to verify infra

**Out:** no changes to `eigenapi/`, no changes to `eigenapitest`, no new test logic (that is EL-5)

## Implementation

### 1. Root `CMakeLists.txt`

Add after existing `add_subdirectory(eigenapi)`:
```cmake
include(CTest)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()
```
`BUILD_TESTING` is an upstream CMake standard option (on by default; users can disable). This keeps hardware-free test builds opt-in-able.

### 2. `tests/CMakeLists.txt`

Add below existing `eigenapitest` target (keep it unchanged):
```cmake
if(BUILD_TESTING)
    add_executable(EigenLiteTests
        main_test.cpp
        SmokeTest.cpp
    )
    include_directories("${PROJECT_SOURCE_DIR}/eigenapi")
    target_link_libraries(EigenLiteTests PRIVATE
        eigenapi
        GTest::gtest
    )
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        target_link_libraries(EigenLiteTests PRIVATE
            "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(EigenLiteTests PRIVATE libusb dl pthread)
    endif()
    add_test(NAME EigenLiteTests COMMAND EigenLiteTests)
endif()
```

Note: `EigenLiteTests` links `eigenapi` so test code can include `eigenapi.h` and use EigenLite types. This is needed for EL-5 contract tests.

### 3. `tests/main_test.cpp`
```cpp
#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

### 4. `tests/SmokeTest.cpp`
```cpp
#include <gtest/gtest.h>

TEST(Smoke, TrivialPass) {
    EXPECT_EQ(1, 1);
}
```

## Validation
```
cmake -B build -DBUILD_TESTING=ON
cmake --build build --target EigenLiteTests
./build/release/bin/EigenLiteTests
```
Expected: `[  PASSED  ] 1 test.`

## Decisions
<!-- Updated during work -->

## Outcome
GTest v1.14 added via FetchContent; `EigenLiteTests` target builds and passes (`[  PASSED  ] 1 test.`). `eigenapitest` unchanged. Prerequisite for EL-2 through EL-5 satisfied.
