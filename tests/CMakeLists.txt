set(EIGENAPITEST_SRC "eigenapitest.cpp")
include_directories ("${PROJECT_SOURCE_DIR}/eigenapi")
add_executable(eigenapitest ${EIGENAPITEST_SRC})
target_link_libraries (eigenapitest eigenapi picodecoder)



if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
	target_link_libraries(eigenapitest  "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(eigenapitest  "libusb" "dl" "pthread")
endif()
