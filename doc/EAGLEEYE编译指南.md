#EAGLEEYE编译指南
---

####概览
EAGLEEYE提供插件化的编程框架，主要涉及核心库和插件库。核心库提供插件管理框架以及数据流编程框架。插件库基于数据流编程框架串起实现具体功能的所有节点，并完成向插件管理中心注册和初始化。

####EAGLEEYE核心库编译
EAGLEEYE核心库的工程配置在eagleeye/CmakeLists.txt中定义。主要包括如下几项配置：
* 日志开关
    开启日志
    add_definitions(-DEAGLEEYE_ENABLE_LOG)
    开启Android App日志
    add_definitions(-DEAGLEEYE_ANDROID_APP)
    注：在未定义EAGLEEYE_ANDROID_APP时，后台将采用标注输出流进行日志输出

* 神经网络推断框架SNPE
    构建时外部指定 -DNN_ENGINE=snpe -DSNPE_PATH=/your folder/snpe-1.35.0.698/

* OPENCL加速开启\关闭
    构建时外部指定 -DOPENCL=OPENCL_PATH。将会添加EAGLEEYE_OPENCL_OPTIMIZATION宏定义。所有存在OPENCL加速的模块，将启动OPENCL计算。

* NEON加速开启\关闭
    构建时外部指定 -DANDROID_ABI=arm64-v8a。对于构建ARM端应用，将自动添加EAGLEEYE_NEON_OPTIMIZATION宏定义。所有存在NEON加速的模块，将启动NEON计算。

* 视频编解码支持
    构建时外部指定 -DVIDEO_SUPPORT=True。将第三方MPEG编码解码库引入，并开启视频编解码模块。

编译64位ARM核心库，使用如下命令：
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=OPENCL_PATH -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DNN_ENGINE=snpe -DSNPE_PATH=/your path/snpe-1.35.0.698/ -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
make
编译后生成libeagleeye.so 核心库。

####插件库工程创建与编译
使用在eagleeye/scripts下的脚本，可以快速创建算法插件工程，比如创建实现运动检测的算法插件，调用:

<pre><code>
eagleeye-cli project --project=movingdet \\ 定义要生成的插件名字
        --version=1.0.0.0 \\            定义插件版本
        --signature=xxxxx \\            定义插件签名（目前未启用）
        --build_type=Release \\         定义编译版本
        --opencv=OPENCV_PATH \\         定义opencv路径(选择性设置)
        --abi=arm64-v8a \\              定义abi
        --eagleeye=EAGLEEYE_PATH \\     定义eagleeye路径
        --opencl=OPENCL_PATH \\         定义OPENCL路径（选择性设置）
        --neon=1                        定义NEON加速(1/0)
</code></pre>

运行脚本后将在当前目录下生成以指定的插件名命名的文件夹（这里--project=movingdet）：
<pre><code>
movingdet_plugin
    - movingdet_plugin.h
    - movingdet_plugin.cpp
    - movingdet_demo.cpp
    - CMakeLists.txt
    - build.sh
</code></pre>

开发者可以直接在自动生成的模板下进行插件开发。调用build.sh后，将会构建和编译插件和对应DEMO工程。
