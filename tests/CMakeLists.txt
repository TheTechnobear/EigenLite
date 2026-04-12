set(PICODECODER_DIR "${CMAKE_SOURCE_DIR}/resources/picodecoder")
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
    set(PICODECODER_DIR "${PICODECODER_DIR}/macOs/${CMAKE_OSX_ARCHITECTURES}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PICODECODER_DIR "${PICODECODER_DIR}/linux/${CMAKE_HOST_SYSTEM_PROCESSOR}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PICODECODER_DIR "${PICODECODER_DIR}/windows/${CMAKE_HOST_SYSTEM_PROCESSOR}")
else()
    set(PICODECODER_DIR "${PICODECODER_DIR}/unknown/${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

message("using PICODECODER_DIR : ${PICODECODER_DIR}")

set(EIGENAPITEST_SRC "eigenapitest.cpp")
include_directories ("${PROJECT_SOURCE_DIR}/eigenapi")
add_executable(eigenapitest ${EIGENAPITEST_SRC})

target_link_libraries (eigenapitest eigenapi)

if(USE_DYNAMIC)
	target_link_libraries (eigenapitest picodecoder)
else()
	target_link_libraries(eigenapitest picodecoder_static)
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(eigenapitest  "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(eigenapitest  "libusb" "dl" "pthread")
endif()

if(BUILD_TESTING)
    add_executable(EigenLiteTests
        main_test.cpp
        SmokeTest.cpp
        ReplayHarnessTest.cpp
        ApiContractTest.cpp
    )
    target_compile_definitions(EigenLiteTests PRIVATE
        FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures")
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

