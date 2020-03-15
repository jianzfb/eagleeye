PROJECT(test)

cmake_minimum_required(VERSION 3.5)
add_definitions(-std=c++11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${test_SOURCE_DIR}/bin/arm64-v8a)
set(EXECUTABLE_OUTPUT_PATH ${test_SOURCE_DIR}/bin/arm64-v8a)
set(LIBRARY_OUTPUT_PATH ${test_SOURCE_DIR}/bin/arm64-v8a)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) 

add_definitions(-DEAGLEEYE_ENABLE_LOG)
# add_definitions(-DEAGLEEYE_ANDROID_APP)

set(test_SRC 
    ./test_plugin.cpp)

include_directories(/Users/jian/Documents/GitHub/eagleeye/eagleeye/)
include_directories(/Users/jian/Documents/GitHub/eagleeye/)
link_directories(/Users/jian/Documents/GitHub/eagleeye/bin/arm64-v8a)
add_library(test SHARED ${test_SRC})  


set(OpenCV_DIR /Users/jian/Documents/xiaomi/mobile/projects/sdk/native/jni)
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
include_directories(${OpenCV_INCLUDE_DIRS})

TARGET_LINK_LIBRARIES(test ${OpenCV_LIBS})



add_definitions(-DEAGLEEYE_OPENCL_OPTIMIZATION)
include_directories(/Users/jian/Documents/GitHub/eagleeye/eagleeye/opencl/include)
add_library(libOpenCL SHARED IMPORTED)
set_target_properties(
    libOpenCL
    PROPERTIES IMPORTED_LOCATION 
    /Users/jian/Documents/GitHub/eagleeye/eagleeye/opencl/lib/libOpenCL.so
)
TARGET_LINK_LIBRARIES(test libOpenCL)



add_definitions(-DEAGLEEYE_NEON_OPTIMIZATION)


TARGET_LINK_LIBRARIES(test 
                      libeagleeye.so
                      log)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(test OpenMP::OpenMP_CXX)
endif()

message("build test executable demo")
set(test_demo_SRC 
    ./test_demo.cpp)
add_executable(test_demo ${test_demo_SRC}) 
target_link_libraries(test_demo test)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")

# set_target_properties(pipeline PROPERTIES PREFIX "")        # no 'lib' prefix
# set_target_properties(pipeline PROPERTIES SUFFIX ".so")     # must be .so (not .dylib)
# set_target_properties(pipeline PROPERTIES LINK_FLAGS "-all_load")
# TARGET_LINK_LIBRARIES(pipeline 
#                       libeagleeye.a
#                       ${PYTHON_LIBRARIES}
#                       ${Boost_LIBRARIES})
