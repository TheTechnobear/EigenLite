project(picodecoder)

include_directories(.)
add_library(picodecoder_static STATIC pico_decoder.c)
set_target_properties(picodecoder_static PROPERTIES PUBLIC_HEADER pico_decoder.h)

set(CMAKE_MACOSX_RPATH 1)
add_library(picodecoder SHARED pico_decoder.c)
