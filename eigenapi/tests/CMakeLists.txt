set(A2DUMP_SRC "a2dump.cpp")
include_directories ("${PROJECT_SOURCE_DIR}/eigenapi")
add_executable(a2dump ${A2DUMP_SRC})
target_link_libraries (a2dump eigenapi )

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
	target_link_libraries(a2dump  "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(a2dump  "libusb" "dl" "pthread")
endif()


######################################

set(PICODUMP_SRC "picodump.cpp")
include_directories ("${PROJECT_SOURCE_DIR}/eigenapi")
add_executable(picodump ${PICODUMP_SRC})
target_link_libraries (picodump eigenapi)

if(USE_DYNAMIC)
    target_link_libraries (picodump picodecoder)
else(USE_DYNAMIC)
	target_link_libraries(picodump picodecoder_static)
endif(USE_DYNAMIC)



if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
	target_link_libraries(picodump  "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(picodump  "libusb" "dl" "pthread")
endif()

