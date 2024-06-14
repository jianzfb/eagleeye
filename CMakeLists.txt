PROJECT(eagleeye)
cmake_minimum_required(VERSION 3.5)

# --gc-sections -pie
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -std=c++11 -Wl, -fpermissive")
set(CMAKE_CXX_FLAGS"${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fPIC -pie -Wl,--gc-sections -fpermissive")


# Use ccache to speed up rebuilds
# include(ccache.cmake)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) 
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -latomic -pthread -llog -lz")
# set(LIBCXX_LINK_FLAGS "-rtlib=compiler-rt")
# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lz")

# support pipeline plugin
add_definitions(-DEAGLEEYE_ENABLE_LOG)
if(DEFINED ANDROID_ABI)
    # ANDROID
    message(STATUS "CONFIGURE ANDROID")
    add_definitions(-DEAGLEEYE_NEON_OPTIMIZATION)       # 默认对于arm都采用neon加速
    if(DEFINED ANDROID_APP)
        add_definitions(-DEAGLEEYE_ANDROID_APP)
    endif()

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    set(EXECUTABLE_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
    set(LIBRARY_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${ANDROID_ABI})
elseif(DEFINED X86_ABI)
    # WINDOW/LINUX
    # (x86/x86-64)
    message(STATUS "CONFIGURE X86")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${eagleeye_SOURCE_DIR}/bin/${X86_ABI})
    set(EXECUTABLE_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${X86_ABI})
    set(LIBRARY_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${X86_ABI})

    if(BUILD_PYTHON_MODULE)
        include(FindPythonByEnv)
        add_subdirectory(${eagleeye_SOURCE_DIR}/eagleeye/3rd/pybind11)
    endif()

    link_libraries(-lz)
elseif(DEFINED ARM_ABI)
    message(STATUS "CONFIGURE arm")
    add_definitions(-DEAGLEEYE_NEON_OPTIMIZATION)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${eagleeye_SOURCE_DIR}/bin/${ARM_ABI})
    set(EXECUTABLE_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${ARM_ABI})
    set(LIBRARY_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${ARM_ABI})

    if(BUILD_PYTHON_MODULE)
        include(FindPythonByEnv)
        add_subdirectory(${eagleeye_SOURCE_DIR}/eagleeye/3rd/pybind11)
    endif()
    link_libraries(-lz)
else()
    # OSX
    message(STATUS "CONFIGURE OSX")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${eagleeye_SOURCE_DIR}/bin/${OSX_ABI})
    set(EXECUTABLE_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${OSX_ABI})
    set(LIBRARY_OUTPUT_PATH ${eagleeye_SOURCE_DIR}/bin/${OSX_ABI})

    if(BUILD_PYTHON_MODULE)
        include(FindPythonByEnv)
        add_subdirectory(${eagleeye_SOURCE_DIR}/eagleeye/3rd/pybind11)
    endif()
endif()


include_directories(${eagleeye_SOURCE_DIR})
include_directories(${eagleeye_SOURCE_DIR}/eagleeye/3rd/)
include_directories(${eagleeye_SOURCE_DIR}/eagleeye/3rd/eigen)
link_directories(${EXECUTABLE_OUTPUT_PATH})


if(DEFINED LITE)
message(STATUS "LITE EAGLEEYE VERSION")
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
    eagleeye/common/CJsonObject.cpp
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
    eagleeye/framework/pipeline/TensorSignal.cpp
    eagleeye/common/EagleeyeLog.cpp 
    eagleeye/engine/model_engine.cpp 
    eagleeye/common/EagleeyeStr.cpp 
    eagleeye/common/EagleeyeIO.cpp 
    eagleeye/framework/pipeline/AnyPipeline.cpp 
    eagleeye/common/EagleeyeModule.cpp
    eagleeye/common/EagleeyeIO.cpp
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
    eagleeye/common/EagleeyeRuntime.cpp
    eagleeye/runtime/gpu/opencl_runtime.cpp
    eagleeye/codegen/opencl/encrypt_opencl_kernel.cc    
    eagleeye/port/linux_base/env.cpp
    eagleeye/port/android/env.cpp
    eagleeye/runtime/cpu/cpu_runtime.cpp
    eagleeye/framework/pipeline/YUVSignal.cpp
    eagleeye/common/EagleeyeYUV.cpp
    eagleeye/common/EagleeyeRGBRotate.cpp
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
    eagleeye/processnode/CallbackNode.cpp
    eagleeye/processnode/AutoPipeline.cpp
    eagleeye/common/EagleeyeBGRARotateHWC.cpp
    eagleeye/common/EagleeyeBGRAFlipHWC.cpp
    eagleeye/common/EagleeyeBGRFlipHWC.cpp
    eagleeye/basic/Dim.cpp
    eagleeye/engine/nano/op/factory.cpp
    eagleeye/engine/nano/op/placeholder_op.cpp
    eagleeye/engine/nano/op/pool2d_op
    eagleeye/engine/nano/op/cast_op.cpp
    eagleeye/engine/nano/op/clip_op.cpp
    eagleeye/engine/nano/op/concat_op.cpp
    eagleeye/engine/nano/op/interpolate_op.cpp
    eagleeye/engine/nano/op/pad2d_op.cpp
    eagleeye/engine/nano/op/reduce_op.cpp
    eagleeye/engine/nano/op/repeat_op.cpp
    eagleeye/engine/nano/op/reshape_op.cpp
    eagleeye/engine/nano/op/transpose_op.cpp
    eagleeye/engine/nano/op/preprocess_op.cpp
    eagleeye/engine/nano/op/split_op.cpp
    eagleeye/engine/nano/op/shape_op.cpp
    eagleeye/engine/nano/op/slice_op.cpp
    eagleeye/engine/nano/op/image_resize_op.cpp
    eagleeye/engine/nano/op/where_op.cpp
    eagleeye/engine/nano/op/topk_op.cpp
    eagleeye/engine/nano/op/gather_op.cpp
    eagleeye/engine/nano/op/gather_nd_op.cpp
    eagleeye/engine/nano/op/nms_op.cpp
    eagleeye/engine/nano/op/roi_align_op.cpp
    eagleeye/engine/nano/op/warp_affine_op.cpp
    eagleeye/engine/nano/op/pool2d_op.cpp
    eagleeye/engine/nano/op/argmax_op.cpp
    eagleeye/engine/nano/op/compare_op.cpp
    eagleeye/engine/nano/op/identity_op.cpp
    eagleeye/engine/nano/op/anchor_free_decoder_op.cpp
    eagleeye/engine/nano/op/meshgrid_op.cpp
    eagleeye/engine/nano/op/arange_op.cpp
    eagleeye/engine/nano/op/select_op.cpp
    eagleeye/engine/nano/op/select_by_thres_op.cpp
    eagleeye/engine/nano/op/pose_decoder_op.cpp
    eagleeye/engine/nano/op/pose_affine_op.cpp    
    eagleeye/engine/nano/op/facealign_op.cpp
    eagleeye/engine/nano/op/keepratio_op.cpp
    eagleeye/engine/nano/op/colorcvt_op.cpp
    eagleeye/engine/nano/op/yuv_op.cpp
    eagleeye/engine/nano/op/keepratio_by_scale_op.cpp
    eagleeye/engine/nano/op/inv_keepratio_layout_op.cpp
    eagleeye/engine/nano/op/image_rotate_op.cpp
    eagleeye/engine/nano/op/auto_keepratio_op.cpp    
    eagleeye/engine/nano/op/face_id_op.cpp
    eagleeye/engine/nano/op/kvmemory_w_op.cpp
    eagleeye/engine/nano/op/kvmemory_r_op.cpp
    eagleeye/engine/nano/op/kvmemory_op.cpp
    eagleeye/engine/math/common/pixel_affine.cpp
    eagleeye/engine/math/common/argmax.cc
    eagleeye/engine/thread_pool.cpp
    eagleeye/processnode/ImageResizeNode.cpp
    eagleeye/processnode/ImageTransformNode.cpp
    eagleeye/common/EagleeyeImageUti.cpp
    eagleeye/common/EagleeyeMessageCenter.cpp
    eagleeye/common/EagleeyeRegisterCenter.cpp
    eagleeye/common/EagleeyeCameraCenter.cpp
    eagleeye/processnode/ImageSelect.cpp
    eagleeye/processnode/RTSPReadNode.cpp
    eagleeye/processnode/SnapshotNode.cpp
    eagleeye/render/RenderContext.cpp
    eagleeye/processnode/AndroidCameraNode.cpp
    eagleeye/processnode/PipelineNode.cpp
    eagleeye/processnode/CacheNode.cpp
    eagleeye/processnode/ForNode.cpp
    eagleeye/processnode/SerializeNode.cpp
    eagleeye/processnode/MessageNode.cpp
    eagleeye/processnode/USBCameraNode.cpp
    eagleeye/processnode/V4L2CameraNode.cpp)
else()
message(STATUS "FULL EAGLEEYE VERSION")
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
    eagleeye/processnode/CallbackNode.cpp
    eagleeye/processnode/AutoPipeline.cpp
    eagleeye/common/EagleeyeFile.cpp 
    eagleeye/common/EagleeyeTime.cpp 
    eagleeye/common/EagleeyeProcessor.cpp
    eagleeye/common/EagleeyeSerial.cpp
    eagleeye/common/EagleeyeIni.cpp
    eagleeye/common/cJSON.cpp
    eagleeye/common/CJsonObject.cpp
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
    eagleeye/framework/pipeline/TensorSignal.cpp
    eagleeye/common/EagleeyeLog.cpp 
    eagleeye/engine/model_engine.cpp 
    eagleeye/common/EagleeyeStr.cpp 
    eagleeye/common/EagleeyeIO.cpp 
    eagleeye/framework/pipeline/AnyPipeline.cpp 
    eagleeye/common/EagleeyeModule.cpp
    eagleeye/common/EagleeyeIO.cpp
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
    eagleeye/runtime/gpu/opencl_runtime.cpp
    eagleeye/common/EagleeyeRuntime.cpp
    eagleeye/codegen/opencl/encrypt_opencl_kernel.cc    
    eagleeye/port/linux_base/env.cpp
    eagleeye/port/android/env.cpp
    eagleeye/runtime/cpu/cpu_runtime.cpp
    eagleeye/engine/nano/op/placeholder_op.cpp
    eagleeye/framework/pipeline/YUVSignal.cpp
    eagleeye/common/EagleeyeYUV.cpp
    eagleeye/common/EagleeyeRGBRotate.cpp
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
    eagleeye/common/EagleeyeBGRARotateHWC.cpp
    eagleeye/common/EagleeyeBGRAFlipHWC.cpp
    eagleeye/common/EagleeyeBGRFlipHWC.cpp
    eagleeye/basic/Dim.cpp
    eagleeye/engine/nano/op/factory.cpp
    eagleeye/engine/nano/op/cast_op.cpp
    eagleeye/engine/nano/op/clip_op.cpp
    eagleeye/engine/nano/op/concat_op.cpp
    eagleeye/engine/nano/op/interpolate_op.cpp
    eagleeye/engine/nano/op/pad2d_op.cpp
    eagleeye/engine/nano/op/reduce_op.cpp
    eagleeye/engine/nano/op/repeat_op.cpp
    eagleeye/engine/nano/op/reshape_op.cpp
    eagleeye/engine/nano/op/transpose_op.cpp
    eagleeye/engine/nano/op/preprocess_op.cpp
    eagleeye/engine/nano/op/split_op.cpp
    eagleeye/engine/nano/op/shape_op.cpp
    eagleeye/engine/nano/op/slice_op.cpp
    eagleeye/engine/nano/op/image_resize_op.cpp
    eagleeye/engine/nano/op/where_op.cpp
    eagleeye/engine/nano/op/topk_op.cpp
    eagleeye/engine/nano/op/gather_op.cpp
    eagleeye/engine/nano/op/gather_nd_op.cpp
    eagleeye/engine/nano/op/nms_op.cpp
    eagleeye/engine/nano/op/roi_align_op.cpp
    eagleeye/engine/nano/op/warp_affine_op.cpp
    eagleeye/engine/nano/op/pool2d_op.cpp
    eagleeye/engine/math/common/pixel_affine.cpp
    eagleeye/engine/math/common/argmax.cc
    eagleeye/engine/nano/op/argmax_op.cpp  
    eagleeye/engine/nano/op/compare_op.cpp
    eagleeye/engine/nano/op/identity_op.cpp
    eagleeye/engine/nano/op/anchor_free_decoder_op.cpp
    eagleeye/engine/nano/op/meshgrid_op.cpp
    eagleeye/engine/nano/op/arange_op.cpp
    eagleeye/engine/nano/op/select_op.cpp
    eagleeye/engine/nano/op/select_by_thres_op.cpp    
    eagleeye/engine/nano/op/pose_decoder_op.cpp
    eagleeye/engine/nano/op/pose_affine_op.cpp
    eagleeye/engine/nano/op/facealign_op.cpp
    eagleeye/engine/nano/op/keepratio_op.cpp
    eagleeye/engine/nano/op/colorcvt_op.cpp
    eagleeye/engine/nano/op/yuv_op.cpp
    eagleeye/engine/nano/op/keepratio_by_scale_op.cpp
    eagleeye/engine/nano/op/inv_keepratio_layout_op.cpp
    eagleeye/engine/nano/op/image_rotate_op.cpp
    eagleeye/engine/nano/op/auto_keepratio_op.cpp
    eagleeye/engine/nano/op/face_id_op.cpp
    eagleeye/engine/nano/op/kvmemory_w_op.cpp
    eagleeye/engine/nano/op/kvmemory_r_op.cpp
    eagleeye/engine/nano/op/kvmemory_op.cpp
    eagleeye/engine/thread_pool.cpp
    eagleeye/processnode/ImageResizeNode.cpp
    eagleeye/processnode/ImageTransformNode.cpp
    eagleeye/processnode/RTSPReadNode.cpp
    eagleeye/processnode/SnapshotNode.cpp
    eagleeye/common/EagleeyeImageUti.cpp
    eagleeye/common/EagleeyeMessageCenter.cpp
    eagleeye/common/EagleeyeRegisterCenter.cpp
    eagleeye/common/EagleeyeCameraCenter.cpp
    eagleeye/processnode/ImageSelect.cpp
    eagleeye/processnode/AndroidCameraNode.cpp
    eagleeye/processnode/PipelineNode.cpp    
    eagleeye/processnode/CacheNode.cpp
    eagleeye/processnode/ForNode.cpp
    eagleeye/processnode/PipelineNode.cpp
    eagleeye/processnode/SerializeNode.cpp
    eagleeye/processnode/MessageNode.cpp
    eagleeye/processnode/USBCameraNode.cpp
    eagleeye/processnode/V4L2CameraNode.cpp)
endif()

# code related with arm
if(DEFINED ANDROID_ABI OR DEFINED ARM_ABI)
    set(EAGLEEYE_SRC 
        ${EAGLEEYE_SRC} 
        eagleeye/engine/math/arm/elementwise.cc
        eagleeye/engine/math/arm/interpolate.cc
        eagleeye/engine/math/arm/pad2d.cc
        eagleeye/engine/math/arm/reduce_max.cc
        eagleeye/engine/math/arm/reduce_mean.cc
        eagleeye/engine/math/arm/reduce_min.cc
        eagleeye/engine/math/arm/reduce_prod.cc
        eagleeye/engine/math/arm/reduce_sum.cc
        eagleeye/engine/math/arm/preprocess.cc
        eagleeye/engine/math/arm/type_trans.cc
        eagleeye/engine/math/arm/pooling.cc
        eagleeye/engine/math/arm/clip.cc)
else()
    # OSX_ABI/X86_ABI
    set(EAGLEEYE_SRC 
        ${EAGLEEYE_SRC} 
        eagleeye/engine/math/x86/preprocess.cc
        eagleeye/engine/math/x86/interpolate.cc)
endif()

# opencl
if(WITH_OPENCL)
    set(EAGLEEYE_SRC 
        ${EAGLEEYE_SRC} 
        eagleeye/engine/nano/util/opencl_util.cpp
    )

    if(DEFINED ANDROID_ABI)
        add_definitions(-DEAGLEEYE_OPENCL_OPTIMIZATION)
        include_directories(${eagleeye_SOURCE_DIR}/eagleeye/3rd/opencl/include)
        add_library(libOpenCL SHARED IMPORTED)
        set_target_properties(
            libOpenCL
            PROPERTIES IMPORTED_LOCATION 
            ${eagleeye_SOURCE_DIR}/eagleeye/3rd/opencl/lib/${ANDROID_ABI}/libOpenCL.so
        )
    else()
        find_package (OpenCL)
        if(OpenCL_FOUND)
            add_definitions(-DEAGLEEYE_OPENCL_OPTIMIZATION)
            include_directories(${OpenCL_INCLUDE_DIR})
        endif()
    endif()
endif()

# OPENGL
if(DEFINED ANDROID_ABI AND DEFINED WITH_OPENGL)
    # 仅针对ANDROID平台，引入opengl环境
    message(STATUS "CONFIGURE OPENGL")
    set(EAGLEEYE_SRC 
        ${EAGLEEYE_SRC} 
        eagleeye/ui/ScrollView.cpp
        eagleeye/render/YUVShow.cpp
        eagleeye/render/GLUtils.cpp
        eagleeye/render/RenderContext.cpp
        eagleeye/render/RenderNode.cpp
        eagleeye/render/ImageContainer.cpp
        eagleeye/render/RenderBase.cpp
        eagleeye/render/ShapeNode.cpp
        eagleeye/render/HighlightShow.cpp
        eagleeye/render/ImageBlend.cpp
        eagleeye/render/RenderMapToCPU.cpp
        eagleeye/render/ImageShow.cpp
        eagleeye/render/AutoRenderRegion.cpp
        eagleeye/render/RenderMapToCPU.cpp
        eagleeye/common/EagleeyeShader.cpp
        eagleeye/render/RenderRegionNode.cpp
    )
    add_definitions(-DEAGLEEYE_OPENGL)
    ADD_SUBDIRECTORY(eagleeye/3rd/glm/)
    find_path(GLES3_INCLUDE_DIR GLES3/gl3.h HINTS ${ANDROID_NDK})              #找到opengl头文件路径
    find_library(GLES3_LIBRARY libGLESv3.so HINTS ${GLES3_INCLUDE_DIR}/../lib) #找到opengl库文件路径
    include_directories(${GLES3_INCLUDE_DIR})    
endif()


if(BUILD_PYTHON_MODULE)
    set(EAGLEEYE_SRC 
        ${EAGLEEYE_SRC} 
        eagleeye/common/EagleeyePy.cpp)
    pybind11_add_module(eagleeye ${EAGLEEYE_SRC})
else()
    add_library(eagleeye SHARED ${EAGLEEYE_SRC})
endif()

FIND_PACKAGE(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    if(BUILD_PYTHON_MODULE)
        target_link_libraries(eagleeye PRIVATE OpenMP::OpenMP_CXX)
    else()
        target_link_libraries(eagleeye OpenMP::OpenMP_CXX)
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

if(CUDA)
    add_definitions(-DEAGLEEYE_CUDA)
    message(STATUS "Define CUDA")
    include_directories("/usr/local/cuda-12.1/targets/x86_64-linux/include")
    find_library(CUDART cudart PATHS /usr/local/cuda-12.1/lib64/)
    target_link_libraries(eagleeye ${CUDART})
    find_library(EXEC_NPPICC nppicc PATHS /usr/local/cuda-12.1/lib64/)
    target_link_libraries(eagleeye ${EXEC_NPPICC})
endif()

if(MINIO)
    add_definitions(-DEAGLEEYE_MINIO)
    message(STATUS "Define MINIO")
    find_package(miniocpp REQUIRED)
    target_link_libraries(eagleeye miniocpp::miniocpp)
endif()

if(DEFINED FFMPEG)
    message(STATUS "Define FFMPEG")
    add_definitions(-DEAGLEEYE_FFMPEG)
    include_directories(${FFMPEG})

    if(BUILD_PYTHON_MODULE)
        target_link_libraries(eagleeye PRIVATE
                                ${FFMPEG}/install/lib/libavcodec.so
                                ${FFMPEG}/install/lib/libavfilter.so
                                ${FFMPEG}/install/lib/libavformat.so
                                ${FFMPEG}/install/lib/libavutil.so
                                ${FFMPEG}/install/lib/libswresample.so
                                ${FFMPEG}/install/lib/libswscale.so)
    else()
        target_link_libraries(eagleeye 
                                ${FFMPEG}/install/lib/libavcodec.so
                                ${FFMPEG}/install/lib/libavfilter.so
                                ${FFMPEG}/install/lib/libavformat.so
                                ${FFMPEG}/install/lib/libavutil.so
                                ${FFMPEG}/install/lib/libswresample.so
                                ${FFMPEG}/install/lib/libswscale.so)
    endif()
endif()


if(DEFINED RKCHIP)
    message(STATUS "Define RK")
    add_definitions(-DEAGLEEYE_RKCHIP)
    include_directories(${RKCHIP}/mpp/)
    include_directories(${RKCHIP}/mpp/inc/)
    include_directories(${RKCHIP}/mpp/utils/)
    include_directories(${RKCHIP}/mpp/osal/inc/)
    include_directories(${RKCHIP}/librga/include)

    if(DEFINED ANDROID_ABI)
        target_link_libraries(eagleeye 
                                ${RKCHIP}/mpp/build/android/mpp/librockchip_mpp.so
                                ${RKCHIP}/mpp/build/android/utils/libutils.a
                                ${RKCHIP}/mpp/build/android/osal/libosal.a
                                ${RKCHIP}/librga/libs/AndroidNdk/arm64-v8a/librga.so)
    elseif(DEFINED ARM_ABI)
        target_link_libraries(eagleeye 
                        ${RKCHIP}/mpp/build/linux/aarch64/mpp/librockchip_mpp.so
                        ${RKCHIP}/mpp/build/linux/aarch64/utils/libutils.a
                        ${RKCHIP}/mpp/build/linux/aarch64/osal/libosal.a
                        ${RKCHIP}/librga/libs/Linux/gcc-aarch64/librga.so)
    endif()
endif()


if(WITH_OPENCL)
    if(DEFINED ANDROID_ABI)
        target_link_libraries(eagleeye libOpenCL)
    else()
        if(BUILD_PYTHON_MODULE)
            target_link_libraries(eagleeye PRIVATE ${OpenCL_LIBRARY})
        else()
            target_link_libraries(eagleeye ${OpenCL_LIBRARY})
        endif()
    endif()
endif()


if(DEFINED ANDROID_ABI AND DEFINED WITH_OPENGL)
    target_link_libraries(eagleeye ${GLES3_LIBRARY})
    target_link_libraries(eagleeye EGL)
endif()

set(HAD_LIBYUV TRUE)
if(HAD_LIBYUV)
    set(LIBYUV_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/eagleeye/3rd/libyuv)
    if(DEFINED ANDROID_ABI)
        set(LIBYUV_BINARY_DIR ${LIBYUV_SOURCE_DIR}/${ANDROID_ABI})
    elseif(DEFINED X86_ABI)
        set(LIBYUV_BINARY_DIR ${LIBYUV_SOURCE_DIR}/${X86_ABI})
    elseif(DEFINED ARM_ABI)
        set(LIBYUV_BINARY_DIR ${LIBYUV_SOURCE_DIR}/${ARM_ABI})
    else()
        set(LIBYUV_BINARY_DIR ${LIBYUV_SOURCE_DIR}/${OSX_ABI})
    endif()
    message(STATUS "libyuv output dir: ${LIBYUV_BINARY_DIR}")
    add_subdirectory(${LIBYUV_SOURCE_DIR} ${LIBYUV_BINARY_DIR})
    target_include_directories(eagleeye PRIVATE ${LIBYUV_SOURCE_DIR}/include)
    if(BUILD_PYTHON_MODULE)
        target_link_libraries(eagleeye PRIVATE yuv_shared)
    else()
        target_link_libraries(eagleeye yuv_shared)
    endif()
endif()

set(HAS_LIBPNG TRUE)
if(HAS_LIBPNG)
    set(LIBPNG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/eagleeye/3rd/libpng)
    if(DEFINED ANDROID_ABI)
        set(LIBPNG_BIN_DIR ${LIBPNG_SOURCE_DIR}/${ANDROID_ABI})
    elseif(DEFINED X86_ABI)
        set(LIBPNG_BIN_DIR ${LIBPNG_SOURCE_DIR}/${X86_ABI})
    elseif(DEFINED ARM_ABI)
        set(LIBPNG_BIN_DIR ${LIBPNG_SOURCE_DIR}/${ARM_ABI})
    else()
        set(LIBPNG_BIN_DIR ${LIBPNG_SOURCE_DIR}/${OSX_ABI})
    endif()
    message(STATUS "libpng output dir: ${LIBPNG_BIN_DIR}")
    add_subdirectory(${LIBPNG_SOURCE_DIR} ${LIBPNG_BIN_DIR})
    target_include_directories(eagleeye PRIVATE ${LIBPNG_SOURCE_DIR})
    target_include_directories(eagleeye PRIVATE ${LIBPNG_BIN_DIR})
    if(BUILD_PYTHON_MODULE)
        target_link_libraries(eagleeye PRIVATE png_shared)
    else()
        target_link_libraries(eagleeye png_shared)
    endif()
endif()

if(DEFINED ANDROID_ABI)
    # android, dont support build_python_module
    target_link_libraries(eagleeye log mediandk android camera2ndk)
endif()

