PROJECT(eagleeye)

cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_FLAGS"${CMAKE_CXX_FLAGS} -std=c++11 -pie -Wl,--gc-sections")


SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) 
SET(NN_ENGINE "" CACHE STRING "Some user-specified option")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -latomic -pthread -ldl -llog -lz")


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

# support mace
if(${NN_ENGINE} MATCHES "mace")
    # message(STATUS "support mace NN engine")
    # add_definitions(-DEAGLEEYE_MACE_SUPPORT)
    # add_definitions(-DMACE_ENABLE_OPENCL)
    # # copy model to 
    # include_directories(${MACE_PATH}/public/)
    # file(COPY ${eagleeye_SOURCE_DIR}/eagleeye/models/mace/${ANDROID_ABI}/srpn_103_183.a DESTINATION ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    # file(COPY ${eagleeye_SOURCE_DIR}/eagleeye/models/mace/${ANDROID_ABI}/portrait_segmentation.a DESTINATION ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    # file(COPY ${MACE_PATH}/model/${ANDROID_ABI}/libmace.so DESTINATION ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})

    # # load detect lib 
    # add_library(srpn_103_183 STATIC IMPORTED)
    # set_target_properties(
    #     srpn_103_183
    #     PROPERTIES IMPORTED_LOCATION 
    #     ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI}/srpn_103_183.a
    # )
    # add_library(portrait_segmentation STATIC IMPORTED)
    # set_target_properties(
    #     portrait_segmentation
    #     PROPERTIES IMPORTED_LOCATION 
    #     ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI}/portrait_segmentation.a
    # )
    
    # # load mace lib
    # add_library(libmace SHARED IMPORTED)
    # set_target_properties(
    #     libmace
    #     PROPERTIES IMPORTED_LOCATION 
    #     ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI}/libmace.so
    # )
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
elseif(${NN_ENGINE} MATCHES "paddle")
    message(STATUS "support paddle engine")
    add_definitions(-DEAGLEEYE_PADDLE_SUPPORT)

    include_directories(${PADDLE_PATH}/include)
    add_library(libpaddle_api_light_bundled SHARED IMPORTED)
    set_target_properties(
    libpaddle_api_light_bundled
    PROPERTIES IMPORTED_LOCATION 
    ${PADDLE_PATH}/lib/libpaddle_light_api_shared.so
)
else()
    message(STATUS "support eagleeye NN engine")    
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
    eagleeye/processnode/DrawNode.cpp
    eagleeye/processnode/AutoNode.cpp
    eagleeye/processnode/QueueNode.cpp
    eagleeye/processnode/DebugNode.cpp
    eagleeye/processnode/LogicalNode.cpp
    eagleeye/processnode/State2BooleanNode.cpp
    eagleeye/processnode/CopyNode.cpp
    eagleeye/processnode/DelayFetch.cpp
    eagleeye/common/EagleeyeImageModule.cpp
    eagleeye/common/EagleeyeVisTool.cpp
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
    eagleeye/engine/snpe_run.cpp 
    eagleeye/engine/paddle_run.cpp
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
    eagleeye/processnode/DrawNode.cpp
    eagleeye/processnode/AutoNode.cpp
    eagleeye/processnode/QueueNode.cpp
    eagleeye/processnode/DebugNode.cpp
    eagleeye/processnode/LogicalNode.cpp
    eagleeye/processnode/State2BooleanNode.cpp
    eagleeye/processnode/CopyNode.cpp
    eagleeye/processnode/DelayFetch.cpp
    eagleeye/common/EagleeyeImageModule.cpp
    eagleeye/common/EagleeyeVisTool.cpp
    eagleeye/engine/nn_run.cpp
    eagleeye/common/EagleeyeFile.cpp 
    eagleeye/common/EagleeyeTime.cpp 
    eagleeye/common/EagleeyeProcessor.cpp
    eagleeye/common/EagleeyeSerial.cpp
    eagleeye/common/EagleeyeIni.cpp
    eagleeye/common/cJSON.cpp
    eagleeye/common/cJsonObject.cpp
    eagleeye/common/EagleeyeFactory.cpp
    eagleeye/framework/EagleeyeTimeStamp.cpp 
    eagleeye/framework/pipeline/AnyNode.cpp 
    eagleeye/framework/pipeline/AnySignal.cpp 
    eagleeye/framework/pipeline/AnyUnit.cpp 
    eagleeye/framework/pipeline/BooleanSignal.cpp
    eagleeye/framework/pipeline/StringSignal.cpp
    eagleeye/framework/pipeline/StateSignal.cpp
    eagleeye/common/EagleeyeLog.cpp 
    eagleeye/engine/snpe_run.cpp 
    eagleeye/engine/paddle_run.cpp
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
    eagleeye/engine/nano/dataflow/schedule.cpp
    eagleeye/engine/nano/dataflow/heft.cpp
    eagleeye/codegen/opencl/encrypt_opencl_kernel.cc)
endif()

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
                            ${X264}/lib/libx264.so)
endif()

find_package(pnglib)
if(DEFINED PNG AND pnglib_FOUND)
    add_definitions(-DEAGLEEYE_PNG)
    include_directories(${pnglib_INCLUDE_DIRS})
    target_link_libraries(eagleeye ${LIBRARY_OUTPUT_PATH}/${pnglib_LIBRARIES})
endif()

if(${NN_ENGINE} MATCHES "snpe")
    target_link_libraries(eagleeye libSNPE)
endif()

if(${NN_ENGINE} MATCHES "paddle")
    TARGET_LINK_LIBRARIES(eagleeye libpaddle_api_light_bundled)
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

