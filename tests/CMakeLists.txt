set(BUILDAPI_TEST 1)
if(BUILDAPI_TEST)


set(PICODECODER_DIR "${CMAKE_SOURCE_DIR}/resources/picodecoder")
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
    set(PICODECODER_DIR "${PICODECODER_DIR}/macOs/${CMAKE_HOST_SYSTEM_PROCESSOR}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PICODECODER_DIR "${PICODECODER_DIR}/linux/${CMAKE_HOST_SYSTEM_PROCESSOR}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PICODECODER_DIR "${PICODECODER_DIR}/windows/${CMAKE_HOST_SYSTEM_PROCESSOR}")
else()
    set(PICODECODER_DIR "${PICODECODER_DIR}/unknown/${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

message("using PICODECODER_DIR : ${PICODECODER_DIR}")
ADD_LIBRARY(picodecoder_static STATIC IMPORTED)
SET_TARGET_PROPERTIES(picodecoder_static PROPERTIES IMPORTED_LOCATION "${PICODECODER_DIR}/libpicodecoder_static.a")


set(EIGENAPITEST_SRC "eigenapitest.cpp")
include_directories ("${PROJECT_SOURCE_DIR}/eigenapi")
add_executable(eigenapitest ${EIGENAPITEST_SRC})

target_link_libraries (eigenapitest eigenapi)

if(USE_DYNAMIC)
	target_link_libraries (eigenapitest picodecoder)
else(USE_DYNAMIC)
	target_link_libraries(eigenapitest picodecoder_static)
endif(USE_DYNAMIC)


if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
	target_link_libraries(eigenapitest  "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(eigenapitest  "libusb" "dl" "pthread")
endif()

endif(BUILDAPI_TEST)