cmake_minimum_required(VERSION 3.4.1)

file(GLOB LIB_PNG_SRC "./eagleeye/3rd/pnglib/*.c")
message(STATUS ${LIB_PNG_SRC})

set(CMAKE_C_FLAGS "-lz ${CMAKE_C_FLAGS}")
add_library(pnglib SHARED ${LIB_PNG_SRC})

# set(CMAKE_PREFIX_PATH ../bin/arm64-v8a)
# find_library(pnglib_LIBRARY NAMES libpnglib.a PATHS ${CMAKE_PREFIX_PATH})
if(APPLE)
set(pnglib_LIBRARY libpnglib.dylib)
else()
set(pnglib_LIBRARY libpnglib.so)
endif()

set(pnglib_FOUND TRUE)
set(pnglib_INCLUDE_DIRS ${pnglib_INCLUDE_DIR})
set(pnglib_LIBRARIES ${pnglib_LIBRARY})
mark_as_advanced(pnglib_LIBRARY pnglib_INCLUDE_DIR)

