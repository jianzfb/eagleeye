cmake_minimum_required(VERSION 3.4.1)

file(GLOB LIB_PNG_SRC "/Users/jian/Documents/GitHub/eagleeye/eagleeye/3rd/pnglib/*.c")
message(STATUS ${LIB_PNG_SRC})

set(CMAKE_C_FLAGS "-lz -Wunused-command-line-argument ${CMAKE_C_FLAGS}")
add_library(pnglib SHARED ${LIB_PNG_SRC})

set(CMAKE_PREFIX_PATH /Users/jian/Documents/GitHub/eagleeye/bin/arm64-v8a)
# find_library(pnglib_LIBRARY NAMES libpnglib.a PATHS ${CMAKE_PREFIX_PATH})
set(pnglib_LIBRARY /Users/jian/Documents/GitHub/eagleeye/bin/arm64-v8a/libpnglib.so)
set(pnglib_FOUND TRUE)
set(pnglib_INCLUDE_DIRS ${pnglib_INCLUDE_DIR})
set(pnglib_LIBRARIES ${pnglib_LIBRARY})
mark_as_advanced(pnglib_LIBRARY pnglib_INCLUDE_DIR)

