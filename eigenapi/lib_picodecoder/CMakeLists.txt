project (picodecoder_static)
set(CMAKE_BUILD_TYPE Release)

include_directories(.)
add_library(picodecoder_static STATIC pico_decoder.c)

set_target_properties(picodecoder_static  PROPERTIES PUBLIC_HEADER pico_decoder.h)

INSTALL(TARGETS picodecoder_static 
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION inc
)

###################################################### 

project (picodecoder)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_MACOSX_RPATH 1)

include_directories(.)
add_library(picodecoder SHARED pico_decoder.c)
