PROJECT(eagleeye)

cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_FLAGS"${CMAKE_CXX_FLAGS} -std=c++11 -pie -Wl,--gc-sections")


SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) 
SET(NN_ENGINE "" CACHE STRING "Some user-specified option")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -latomic -pthread -llog -lz")
# set(LIBCXX_LINK_FLAGS "-rtlib=compiler-rt")
# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -latomic -pthread -llog -lz")

add_definitions(-DARM_WITH_OMP)
add_definitions(-DHPPL_STUB_FUNC)
add_definitions(-DLITE_WITH_ARM)
add_definitions(-DLITE_WITH_LIGHT_WEIGHT_FRAMEWORK)
add_definitions(-DLITE_WITH_LINUX)
add_definitions(-DPADDLE_DISABLE_PROFILER)


# support pipeline plugin
add_definitions(-DEAGLEEYE_ENABLE_LOG)
# add_definitions(-DEAGLEEYE_ANDROID_APP)

if(DEFINED ANDROID_ABI)
    message(STATUS "configure android .so")
    add_definitions(-DEAGLEEYE_NEON_OPTIMIZATION)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    set(EXECUTABLE_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    set(LIBRARY_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})

else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${eagleeye_SOURCE_DIR}/bin)
    set(EXECUTABLE_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin)
    set(LIBRARY_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin)
endif()

# support opencl
if(DEFINED OPENCL)
    message(STATUS "OPENCL path "${OPENCL})
    add_definitions(-DEAGLEEYE_OPENCL_OPTIMIZATION)
    include_directories(${OPENCL}/include)
    add_library(libOpenCL SHARED IMPORTED)
    set_target_properties(
        libOpenCL
        PROPERTIES IMPORTED_LOCATION 
        ${OPENCL}/lib/${ANDROID_ABI}/libOpenCL.so
    )
endif()

if(DEFINED PYTHON)
# #   support python interface
# add_definitions(-DSUPPORT_BOOST_PYTHON)

# #
# #	python libs
# #
# find_package(PythonLibs 3.6 REQUIRED)
# include_directories(${PYTHON_INCLUDE_DIRS})

# #
# #	numpy libs
# #
# find_package(NumPy)
# include_directories(${NUMPY_INCLUDE_DIRS})

# #
# # boost libs
# #
# # find_package(Boost COMPONENTS python REQUIRED)
# find_package(Boost COMPONENTS python3.7 REQUIRED)
# include_directories(${Boost_INCLUDE_DIR})

# message(STATUS "PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")
# message(STATUS "PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
# message(STATUS "PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")
# message(STATUS "NUMPY_INCLUDE_DIRS = ${NUMPY_INCLUDE_DIRS}")
# message(STATUS "Boost_LIBRARIES = ${Boost_LIBRARIES}")
endif()

include_directories(${eagleeye_SOURCE_DIR})
include_directories(${eagleeye_SOURCE_DIR}/eagleeye/3rd/)


ADD_SUBDIRECTORY(eagleeye/3rd/glm/)
if(DEFINED LITE)
message(STATUS "LITE version")
set(EAGLEEYE_SRC 
    eagleeye/processnode/SubPipeline.cpp
    eagleeye/processnode/ParallelNode.cpp
    eagleeye/processnode/IfElseNode.cpp
    eagleeye/processnode/SkipNode.cpp
    eagleeye/processnode/VideoReadNode.cpp
    eagleeye/processnode/VideoWriteNode.cpp
    eagleeye/processnode/AsynNode.cpp
    eagleeye/processnode/NNNode.cpp
    eagleeye/processnode/SerialReadNode.cpp
    eagleeye/processnode/SerialWriteNode.cpp
    eagleeye/processnode/ImageWriteNode.cpp
    eagleeye/processnode/ImageReadNode.cpp
    eagleeye/processnode/ReadLineFromFileNode.cpp
    eagleeye/processnode/SerialStringNode.cpp
    eagleeye/processnode/AutoNode.cpp
    eagleeye/processnode/QueueNode.cpp
    eagleeye/processnode/DebugNode.cpp
    eagleeye/processnode/LogicalNode.cpp
    eagleeye/processnode/State2BooleanNode.cpp
    eagleeye/processnode/CopyNode.cpp
    eagleeye/processnode/DelayFetch.cpp
    eagleeye/common/EagleeyeFile.cpp 
    eagleeye/common/EagleeyeTime.cpp 
    eagleeye/common/EagleeyeProcessor.cpp
    eagleeye/common/EagleeyeSerial.cpp
    eagleeye/common/EagleeyeLog.cpp 
    eagleeye/common/EagleeyeIni.cpp
    eagleeye/framework/EagleeyeTimeStamp.cpp 
    eagleeye/framework/pipeline/AnyNode.cpp 
    eagleeye/framework/pipeline/AnySignal.cpp 
    eagleeye/framework/pipeline/AnyUnit.cpp 
    eagleeye/framework/pipeline/BooleanSignal.cpp
    eagleeye/framework/pipeline/StringSignal.cpp
    eagleeye/framework/pipeline/StateSignal.cpp
    eagleeye/framework/pipeline/AnyPipeline.cpp 
    eagleeye/engine/model_engine.cpp 
    eagleeye/common/EagleeyeStr.cpp 
    eagleeye/common/EagleeyeIO.cpp 
    eagleeye/common/EagleeyeModule.cpp
    eagleeye/common/EagleeyePy.cpp
    eagleeye/common/EagleeyeIO.cpp
    eagleeye/common/EagleeyeThreadPool.cpp
    eagleeye/common/EagleeyeOpenCL.cpp
    eagleeye/basic/MatrixMath.cpp
    eagleeye/basic/blob.cpp
    eagleeye/basic/Tensor.cpp
    eagleeye/port/linux_base/env.cpp
    eagleeye/port/android/env.cpp
    eagleeye/runtime/cpu/cpu_runtime.cpp
    eagleeye/runtime/gpu/opencl_runtime.cpp
    eagleeye/codegen/opencl/encrypt_opencl_kernel.cc)
else()
message(STATUS "FULL version")
set(EAGLEEYE_SRC 
    eagleeye/processnode/OpticalFlowNode.cpp
    eagleeye/processnode/ConsistenceNode.cpp
    eagleeye/processnode/AdaptorNode.cpp
    eagleeye/processnode/TemplateMatchingNode.cpp
    eagleeye/processnode/SubPipeline.cpp
    eagleeye/processnode/ParallelNode.cpp
    eagleeye/processnode/IfElseNode.cpp
    eagleeye/processnode/SkipNode.cpp
    eagleeye/processnode/VideoReadNode.cpp
    eagleeye/processnode/VideoWriteNode.cpp
    eagleeye/processnode/AsynNode.cpp
    eagleeye/processnode/NNNode.cpp
    eagleeye/processnode/SerialReadNode.cpp
    eagleeye/processnode/SerialWriteNode.cpp
    eagleeye/processnode/ImageWriteNode.cpp
    eagleeye/processnode/ImageReadNode.cpp
    eagleeye/processnode/HistogramEqualizationNode.cpp
    eagleeye/processnode/ReadLineFromFileNode.cpp
    eagleeye/processnode/SerialStringNode.cpp
    eagleeye/processnode/AutoNode.cpp
    eagleeye/processnode/QueueNode.cpp
    eagleeye/processnode/DebugNode.cpp
    eagleeye/processnode/LogicalNode.cpp
    eagleeye/processnode/State2BooleanNode.cpp
    eagleeye/processnode/CopyNode.cpp
    eagleeye/processnode/DelayFetch.cpp
    eagleeye/common/EagleeyeFile.cpp 
    eagleeye/common/EagleeyeTime.cpp 
    eagleeye/common/EagleeyeProcessor.cpp
    eagleeye/common/EagleeyeSerial.cpp
    eagleeye/common/EagleeyeIni.cpp
    eagleeye/common/cJSON.cpp
    eagleeye/common/cJsonObject.cpp
    eagleeye/common/EagleeyeFactory.cpp
    eagleeye/common/EagleeyeNodeManager.cpp
    eagleeye/framework/EagleeyeTimeStamp.cpp 
    eagleeye/framework/pipeline/AnyNode.cpp 
    eagleeye/framework/pipeline/AnySignal.cpp 
    eagleeye/framework/pipeline/AnyUnit.cpp 
    eagleeye/framework/pipeline/BooleanSignal.cpp
    eagleeye/framework/pipeline/StringSignal.cpp
    eagleeye/framework/pipeline/StateSignal.cpp
    eagleeye/framework/pipeline/LandmarkSignal.cpp
    eagleeye/common/EagleeyeLog.cpp 
    eagleeye/engine/model_engine.cpp 
    eagleeye/common/EagleeyeStr.cpp 
    eagleeye/common/EagleeyeIO.cpp 
    eagleeye/framework/pipeline/AnyPipeline.cpp 
    eagleeye/common/EagleeyeModule.cpp
    eagleeye/common/EagleeyePy.cpp
    eagleeye/common/EagleeyeIO.cpp
    eagleeye/common/EagleeyeThreadPool.cpp
    eagleeye/basic/MatrixMath.cpp
    eagleeye/basic/blob.cpp
    eagleeye/basic/Tensor.cpp
    eagleeye/algorithm/qr_solver.cpp
    eagleeye/algorithm/llsp.cpp
    eagleeye/algorithm/nms.cpp
    eagleeye/algorithm/homographymodel.cpp
    eagleeye/algorithm/segment.cpp
    eagleeye/algorithm/filter.cpp
    eagleeye/common/EagleeyeOpenCL.cpp
    eagleeye/port/linux_base/env.cpp
    eagleeye/port/android/env.cpp
    eagleeye/runtime/cpu/cpu_runtime.cpp
    eagleeye/runtime/gpu/opencl_runtime.cpp
    eagleeye/engine/nano/util/opencl_util.cpp
    eagleeye/engine/nano/op/placeholder.cpp
    eagleeye/render/ImageShow.cpp
    eagleeye/codegen/opencl/encrypt_opencl_kernel.cc
    eagleeye/render/GLUtils.cpp
    eagleeye/render/RenderContext.cpp
    eagleeye/render/RenderNode.cpp
    eagleeye/render/ImageContainer.cpp
    eagleeye/render/RenderBase.cpp
    eagleeye/render/ShapeNode.cpp
    eagleeye/render/HighlightShow.cpp
    eagleeye/render/ImageBlend.cpp
    eagleeye/render/RenderMapToCPU.cpp
    eagleeye/ui/ScrollView.cpp
    eagleeye/framework/pipeline/YUVSignal.cpp
    eagleeye/render/YUVShow.cpp
    eagleeye/common/EagleeyeYUV.cpp
    eagleeye/common/EagleeyeShader.cpp
    eagleeye/common/EagleeyeGraph.cpp
    eagleeye/framework/pipeline/YUVSignal.cpp
    eagleeye/framework/pipeline/EmptySignal.cpp
    eagleeye/framework/pipeline/GroupSignal.cpp
    eagleeye/processnode/YUVConvertNode.cpp
    eagleeye/processnode/YUVResizeNode.cpp
    eagleeye/processnode/OrNode.cpp
    eagleeye/processnode/IsUpdateNode.cpp
    eagleeye/processnode/ConstNode.cpp
    eagleeye/processnode/DottingNode.cpp
    eagleeye/processnode/MosaicNode.cpp
    eagleeye/framework/pipeline/AnyRegister.cpp
    eagleeye/processnode/ThreadNode.cpp
    eagleeye/processnode/GroupNode.cpp
    eagleeye/processnode/IdentityNode.cpp
    eagleeye/processnode/ClassicalFilter.cpp
    eagleeye/processnode/SwitchNode.cpp
    eagleeye/processnode/AnyPlaceholder.cpp
    eagleeye/processnode/CropNode.cpp
    eagleeye/basic/BGRARotateHWC.cpp
    eagleeye/basic/BGRRotateHWC.cpp
    eagleeye/basic/BGRAFlipHWC.cpp
    eagleeye/basic/BGRFlipHWC.cpp
    eagleeye/render/AutoRenderRegion.cpp
    eagleeye/basic/Dim.cpp)
endif()

add_library(eagleeye SHARED ${EAGLEEYE_SRC})

if(DEFINED PYTHON)
    target_link_libraries(eagleeye 
                        ${PYTHON_LIBRARIES} 
                        ${Boost_LIBRARIES})
endif()


FIND_PACKAGE( OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(eagleeye OpenMP::OpenMP_CXX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()


if(DEFINED FFMPEG AND DEFINED X264)
    message("use FFMPEG")
    add_definitions(-DEAGLEEYE_FFMPEG)
    include_directories(${FFMPEG}/include/)
    include_directories(${X264}/include/)

    target_link_libraries(eagleeye 
                            ${FFMPEG}/lib/libavdevice.so
                            ${FFMPEG}/lib/libavutil.so
                            ${FFMPEG}/lib/libswresample.so
                            ${FFMPEG}/lib/libswscale.so
                            ${FFMPEG}/lib/libavcodec.so
                            ${FFMPEG}/lib/libavformat.so
                            ${FFMPEG}/lib/libavfilter.so
                            ${FFMPEG}/lib/libpostproc.so
                            ${X264}/lib/libx264.163.so)
endif()

find_package(pnglib)
if(DEFINED PNG AND pnglib_FOUND)
    add_definitions(-DEAGLEEYE_PNG)
    include_directories(${pnglib_INCLUDE_DIRS})
    target_link_libraries(eagleeye ${LIBRARY_OUTPUT_PATH}/${pnglib_LIBRARIES})
endif()

if(DEFINED OPENCL)
target_link_libraries(eagleeye libOpenCL)
target_link_libraries(eagleeye EGL)
endif()

find_path(GLES3_INCLUDE_DIR GLES3/gl3.h
    HINTS ${ANDROID_NDK})              #找到opengl头文件路径
find_library(GLES3_LIBRARY libGLESv3.so
    HINTS ${GLES3_INCLUDE_DIR}/../lib) #找到opengl库文件路径
include_directories(${GLES3_INCLUDE_DIR})
target_link_libraries(eagleeye ${GLES3_LIBRARY})

add_library(libyuv SHARED IMPORTED)
set_target_properties(libyuv PROPERTIES IMPORTED_LOCATION
                        ${eagleeye_SOURCE_DIR}/eagleeye/3rd/libyuv/lib/${ANDROID_ABI}/libyuv.so)
include_directories(${eagleeye_SOURCE_DIR}/eagleeye/3rd/libyuv/include)
target_link_libraries(eagleeye libyuv)

# add_library(libunwind STATIC IMPORTED)
# set_target_properties(
#         libunwind
#         PROPERTIES IMPORTED_LOCATION
#         ${ANDROID_NDK}/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/lib/arm-linux-androideabi/libunwind.a
# )


# set(OpenCV_DIR ../sdk/native/jni)
# # set(OpenCV_DIR "/Users/jian/Downloads/opencv-master")
# find_package(OpenCV REQUIRED)
# message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
# include_directories(${OpenCV_INCLUDE_DIRS})
if(DEFINED ANDROID_ABI)
target_link_libraries(eagleeye log)
endif()

