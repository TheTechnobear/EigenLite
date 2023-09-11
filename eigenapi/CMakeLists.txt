project (eigenapi)

# USE_DUMMY_PICO - if you do not want to use prebuilt binaries (advanced use only), implies dynamic!
# USE_DYNAMIC - use shared libraries for pico decoder

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
    #temporary, so I can see 'real' errors
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
endif()



if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
    #add_definitions(-D ML_USE_SSE)
    add_definitions(-D __MACOSX_CORE__)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
        add_definitions(-D ML_USE_NEON)
    else()
        add_definitions(-D ML_USE_SSE)
    endif ()
endif()

file (GLOB_RECURSE EIGENHARP_HDRS "*/*.h" )

set(EIGENHARP_SRC
    picross/src/pic_backtrace.c
    picross/src/pic_time.c 
    picross/src/pic_usb_generic.cpp 
    picross/src/usb_serial.cpp 
    picross/src/pic_safeq.cpp
    picross/src/pic_error.cpp 
    picross/src/pic_log.cpp 
    picross/src/pic_fastalloc.cpp 
    picross/src/pic_mlock.cpp 
    picross/src/pic_resources.cpp
    lib_alpha2/src/alpha2_passive.cpp 
    lib_alpha2/src/alpha2_active.cpp
    lib_pico/src/pico_passive.cpp 
    lib_pico/src/pico_active.cpp
    src/eigenlite.cpp
    src/ef_harp.cpp
    src/ef_basestation.cpp
    src/ef_alpha.cpp
    src/ef_tau.cpp
    src/ef_pico.cpp
    src/eigenapi.cpp
    src/fwr_posix.cpp
)

set(EMB_FW_SRC_DIR "${CMAKE_SOURCE_DIR}/resources/firmware/cpp")

set(EIGENHARP_SRC ${EIGENHARP_SRC} 
    src/fwr_embedded.cpp
    ${EMB_FW_SRC_DIR}/bs_mm_fw_0103.cpp	
    ${EMB_FW_SRC_DIR}/pico.cpp		
    ${EMB_FW_SRC_DIR}/psu_mm_fw_0102.cpp	
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
set(EIGENHARP_SRC ${EIGENHARP_SRC} 
    picross/src/pic_thread_posix.cpp 
    picross/src/pic_cfrunloop.cpp 
    picross/src/pic_usb_macosx.cpp 
    picross/src/pic_tool_macosx.cpp
    picross/src/pic_power_macosx.mm)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(EIGENHARP_SRC ${EIGENHARP_SRC} 
        picross/src/pic_thread_posix.cpp 
        picross/src/pic_usb_linux.cpp 
        picross/src/pic_tool_linux.cpp)
elseif(WIN32) 
    set(EIGENHARP_SRC ${EIGENHARP_SRC} 
        picross/src/pic_thread_win32.cpp
        picross/src/pic_usb_win32.cpp 
        picross/src/pic_winloop.cpp 
        picross/src/pic_tool_win32.cpp)
endif()


include_directories ( . ./picross  ./src ./lib_alpha2 ./lib_pico ./lib_picodecoder )

add_library( eigenapi STATIC ${EIGENHARP_SRC} ${EIGENHARP_HDRS} )

target_include_directories(eigenapi PUBLIC
        .
        )



if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(eigenapi atomic)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
    target_link_libraries(eigenapi "-framework Cocoa  -framework IOKit -framework CoreAudio")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(eigenapi  "libusb" "pthread")
endif()

# Binary version of picodecoder
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

if(USE_DUMMY_PICO)
    message("using dummy picodecoder")
    add_subdirectory(lib_picodecoder)
else(USE_DUMMY_PICO)
    message("using PICODECODER_DIR : ${PICODECODER_DIR}")
    ADD_LIBRARY(picodecoder_static STATIC IMPORTED)
    SET_TARGET_PROPERTIES(picodecoder_static PROPERTIES IMPORTED_LOCATION "${PICODECODER_DIR}/libpicodecoder_static.a")
endif()

add_subdirectory(tests)



