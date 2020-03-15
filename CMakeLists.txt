PROJECT(eagleeye)

cmake_minimum_required(VERSION 3.5)
# add_definitions(-std=c++11)
set(CMAKE_CXX_FLAGS"${CMAKE_CXX_FLAGS} -std=c++11")


SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) 
SET(NN_ENGINE "" CACHE STRING "Some user-specified option")

# support pipeline plugin
add_definitions(-DEAGLEEYE_ENABLE_LOG)
# add_definitions(-DEAGLEEYE_ANDROID_APP)

if(DEFINED ANDROID_ABI)
    message(STATUS "configure android .so")
    add_definitions(-DEAGLEEYE_NEON_OPTIMIZATION)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    set(EXECUTABLE_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    set(LIBRARY_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    include_directories(${eagleeye_SOURCE_DIR}/eagleeye/opencl/include)
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${eagleeye_SOURCE_DIR}/bin)
    set(EXECUTABLE_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin)
    set(LIBRARY_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin)
    include_directories(${eagleeye_SOURCE_DIR}/eagleeye/opencl/include)
endif()

# support mace
if(${NN_ENGINE} MATCHES "mace")
    message(STATUS "support mace NN engine")
    add_definitions(-DEAGLEEYE_MACE_SUPPORT)
    add_definitions(-DMACE_ENABLE_OPENCL)
    # copy model to 
    include_directories(${MACE_PATH}/public/)
    file(COPY ${eagleeye_SOURCE_DIR}/eagleeye/models/mace/${ANDROID_ABI}/srpn_103_183.a DESTINATION ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    file(COPY ${eagleeye_SOURCE_DIR}/eagleeye/models/mace/${ANDROID_ABI}/portrait_segmentation.a DESTINATION ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    file(COPY ${MACE_PATH}/model/${ANDROID_ABI}/libmace.so DESTINATION ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})

    # load detect lib 
    add_library(srpn_103_183 STATIC IMPORTED)
    set_target_properties(
        srpn_103_183
        PROPERTIES IMPORTED_LOCATION 
        ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI}/srpn_103_183.a
    )
    add_library(portrait_segmentation STATIC IMPORTED)
    set_target_properties(
        portrait_segmentation
        PROPERTIES IMPORTED_LOCATION 
        ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI}/portrait_segmentation.a
    )
    
    # load mace lib
    add_library(libmace SHARED IMPORTED)
    set_target_properties(
        libmace
        PROPERTIES IMPORTED_LOCATION 
        ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI}/libmace.so
    )
elseif(${NN_ENGINE} MATCHES "snpe")
    message(STATUS "support snpe NN engine")
    add_definitions(-DEAGLEEYE_SNPE_SUPPORT)

    # load snpe lib
    set(SNPE_LIB_DIR ${SNPE_PATH}/lib/aarch64-android-clang6.0)
    set(SNPE_INCLUDE_DIR ${SNPE_PATH}/include/zdl)
    include_directories(${SNPE_INCLUDE_DIR})

    add_library(libSNPE SHARED IMPORTED)
    set_target_properties(
        libSNPE
        PROPERTIES IMPORTED_LOCATION 
        ${SNPE_LIB_DIR}/libSNPE.so
    )
else()
    message(STATUS "support eagleeye NN engine")    
endif()

# support opencl
if(DEFINED OPENCL)
    add_definitions(-DEAGLEEYE_OPENCL_OPTIMIZATION)
    file(READ ${eagleeye_SOURCE_DIR}/eagleeye/opencl/src/square.cl A)
    string(REPLACE "\n" "" OPENCL_SQUARE_CL "${A}")
    
    file(READ ${eagleeye_SOURCE_DIR}/eagleeye/opencl/src/algorithm.cl B)
    string(REPLACE "\n" "" OPENCL_ALGORITHM_CL "${B}")
    
    configure_file(${eagleeye_SOURCE_DIR}/eagleeye/common/EagleeyeCL_CODE.h ${eagleeye_SOURCE_DIR}/EagleeyeCL_CODE.h)    

    add_library(libOpenCL SHARED IMPORTED)
    set_target_properties(
        libOpenCL
        PROPERTIES IMPORTED_LOCATION 
        ${eagleeye_SOURCE_DIR}/eagleeye/opencl/lib/libOpenCL.so
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

if(DEFINED FFMPEG)
add_definitions(-DEAGLEEYE_FFMPEG)
include_directories(../ffmpeg/arm64-v8a/include/)
include_directories(../x264/arm64-v8a/include/)
endif()

set(EAGLEEYE_SRC 
    eagleeye/processnode/OpticalFlowNode.cpp
    eagleeye/processnode/ConsistenceNode.cpp
    eagleeye/processnode/movingdetnode.cpp
    eagleeye/processnode/GOTURNTrackingNode.cpp
    eagleeye/processnode/SegNode.cpp
    eagleeye/processnode/AdaptorNode.cpp
    eagleeye/processnode/TemplateMatchingNode.cpp
    eagleeye/processnode/SaliencyDetNode.cpp
    eagleeye/processnode/ImageTransformNode.cpp
    eagleeye/processnode/SSDReIDPersonDetNode.cpp
    eagleeye/processnode/SubPipeline.cpp
    eagleeye/processnode/ParallelNode.cpp
    eagleeye/processnode/IfElseNode.cpp
    eagleeye/processnode/SkipNode.cpp
    eagleeye/processnode/VideoReadNode.cpp
    eagleeye/processnode/VideoWriteNode.cpp
    eagleeye/processnode/AsynNode.cpp
    eagleeye/processnode/NNNode.cpp
    eagleeye/processnode/InstancePersonSegMRCNNNode.cpp
    eagleeye/processnode/SerialReadNode.cpp
    eagleeye/processnode/SerialWriteNode.cpp
    eagleeye/processnode/ImageWriteNode.cpp
    eagleeye/processnode/ImageReadNode.cpp
    eagleeye/processnode/HistogramEqualizationNode.cpp
    eagleeye/common/EagleeyeImageModule.cpp
    eagleeye/common/EagleeyeVisTool.cpp
    eagleeye/engine/mace_run.cpp
    eagleeye/common/EagleeyeFile.cpp 
    eagleeye/common/EagleeyeTime.cpp 
    eagleeye/common/EagleeyeProcessor.cpp
    eagleeye/common/EagleeyeSerial.cpp
    eagleeye/framework/EagleeyeTimeStamp.cpp 
    eagleeye/framework/pipeline/AnyNode.cpp 
    eagleeye/framework/pipeline/AnySignal.cpp 
    eagleeye/framework/pipeline/AnyUnit.cpp 
    eagleeye/framework/pipeline/BooleanSignal.cpp
    eagleeye/common/EagleeyeLog.cpp 
    eagleeye/engine/snpe_run.cpp 
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
    eagleeye/basic/MatrixMathCL.cpp
    eagleeye/algorithm/qr_solver.cpp
    eagleeye/algorithm/llsp.cpp
    eagleeye/algorithm/nms.cpp
    eagleeye/algorithm/homographymodel.cpp
    eagleeye/algorithm/segment.cpp
    eagleeye/algorithm/filter.cpp
    eagleeye/common/EagleeyeOpenCL.cpp
    eagleeye/tensorop/imageop.cpp
    eagleeye/tensorop/tensorop.cpp
    eagleeye/port/linux_base/env.cpp
    eagleeye/port/android/env.cpp
    eagleeye/runtime/cpu/cpu_runtime.cpp
    eagleeye/runtime/gpu/opencl_runtime.cpp
    eagleeye/engine/nano/util/quantization.cpp
    eagleeye/engine/nano/op/FixedCNNOp.cpp
    eagleeye/engine/nano/op/FixedResizedOp.cpp
    eagleeye/engine/nano/op/FixedReluOp.cpp
    eagleeye/engine/nano/op/FixedSoftmaxOp.cpp
    eagleeye/engine/nano/util/conv_pool_2d_util.cpp)

add_library(eagleeye SHARED ${EAGLEEYE_SRC})

if(DEFINED PYTHON)
    target_link_libraries(eagleeye 
                        ${PYTHON_LIBRARIES} 
                        ${Boost_LIBRARIES})
endif()

if(DEFINED OPENMP)
    add_definitions(-DEAGLEEYE_OPENMP)
    find_package(OpenMP REQUIRED)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(eagleeye OpenMP::OpenMP_CXX)
    endif()
endif()

if(DEFINED FFMPEG)
    link_directories(../ffmpeg/arm64-v8a/lib/)
    link_directories(../x264/arm64-v8a/lib/)
    target_link_libraries(eagleeye 
                            libavdevice.so
                            libavutil.so
                            libswresample.so
                            libswscale.so
                            libavcodec.so
                            libavformat.so
                            libavfilter.so
                            libpostproc.so
                            libx264.so)
endif()

find_package(pnglib)
if(pnglib_FOUND)
    add_definitions(-DEAGLEEYE_PNG)
    include_directories(${pnglib_INCLUDE_DIRS})
    target_link_libraries(eagleeye ${pnglib_LIBRARIES})
else()
    message(FATAL_ERROR "pnglib")
endif()

if(${NN_ENGINE} MATCHES "snpe")
    target_link_libraries(eagleeye libSNPE)
endif()

if(DEFINED OPENCL)
target_link_libraries(eagleeye libOpenCL)
endif()

# set(OpenCV_DIR ../sdk/native/jni)
# # set(OpenCV_DIR "/Users/jian/Downloads/opencv-master")
# find_package(OpenCV REQUIRED)
# message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
# include_directories(${OpenCV_INCLUDE_DIRS})

if(DEFINED ANDROID_ABI)
target_link_libraries(eagleeye log)
endif()
