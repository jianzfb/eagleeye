PROJECT(None)

cmake_minimum_required(VERSION 3.5)
add_definitions(-std=c++11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${None_SOURCE_DIR}/bin/arm64-v8a)
set(EXECUTABLE_OUTPUT_PATH ${None_SOURCE_DIR}/bin/arm64-v8a)
set(LIBRARY_OUTPUT_PATH ${None_SOURCE_DIR}/bin/arm64-v8a)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) 

add_definitions(-DEAGLEEYE_ENABLE_LOG)
add_definitions(-DEAGLEEYE_ANDROID_APP)

set(None_SRC 
    ./None_plugin.cpp)

include_directories(/eagleeye/)
include_directories(/)
link_directories(/bin/arm64-v8a)
add_library(None SHARED ${None_SRC})  




add_definitions(-DEAGLEEYE_OPENCL_OPTIMIZATION)
include_directories(/eagleeye/opencl/include)
add_library(libOpenCL SHARED IMPORTED)
set_target_properties(
    libOpenCL
    PROPERTIES IMPORTED_LOCATION 
    /eagleeye/opencl/lib/libOpenCL.so
)
TARGET_LINK_LIBRARIES(None libOpenCL)



add_definitions(-DEAGLEEYE_NEON_OPTIMIZATION)


TARGET_LINK_LIBRARIES(None 
                      libeagleeye.so
                      log)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(None OpenMP::OpenMP_CXX)
endif()

message("build None executable demo")
set(None_demo_SRC 
    ./None_demo.cpp)
add_executable(None_demo ${None_demo_SRC}) 
target_link_libraries(None_demo None)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")

# set_target_properties(pipeline PROPERTIES PREFIX "")        # no 'lib' prefix
# set_target_properties(pipeline PROPERTIES SUFFIX ".so")     # must be .so (not .dylib)
# set_target_properties(pipeline PROPERTIES LINK_FLAGS "-all_load")
# TARGET_LINK_LIBRARIES(pipeline 
#                       libeagleeye.a
#                       ${PYTHON_LIBRARIES}
#                       ${Boost_LIBRARIES})
