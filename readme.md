#EAGLEEYE图像应用开发框架

####简介
EAGLEEYE图像应用开发框架针对于快速将图像算法推向落地而设计。依靠统一通用的模块接口定义和数据流管线架构设计，极度简化团队协同开发。

####EAGLEEYE核心库编译
编译移动端EAGLEEYE库（使用高通AI推理库SNPE）
```c++
mkdir ./build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=True -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DNN_ENGINE=snpe -DSNPE_PATH=YOUR SNPE FOLDER/snpe-1.35.0.698/ -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..

make
```

注意：需要提前安装Android NDK。

####快速入手
* 创建算法管线插件模板
    调用脚本
    <pre><code>
    python main.py --project=your plugin --version=1.0.0.0  --signature=xxxxxxxxxx
    </code></pre>

    生成开发模板（--project=test），实现c=a+b功能
    <pre><code>
    test_plugin
        - test_plugin.h
        - test_plugin.cpp
        - test_demo.cpp
        - CMakeLists.txt
        - build.sh
    </code></pre>
    调用build.sh，将会构建插件和DEMO工程。

* 编写算法管线
    实现c=a+b功能，在test_plugin.cpp文件中
    <pre><code>
    namespace eagleeye{
    // 注册算法管线
    EAGLEEYE_PIPELINE_REGISTER(test, 1.0.0.0, xxxxx);
    
    // 初始换算法管线
    EAGLEEYE_BEGIN_PIPELINE_INITIALIZE(test)
    // 建立数据源节点
    DataSourceNode<ImageSignal<float>>* data_source = 
                new DataSourceNode<ImageSignal<float>>();
    data_source->setSourceType(EAGLEEYE_SIGNAL_IMAGE);
    data_source->setSourceTarget(EAGLEEYE_CAPTURE_PREVIEW_IMAGE);
    
    // 第一步：建立节点 a,b,c
    IncrementOrAddNode< ImageSignal< float >,ImageSignal< float > >* a = 
        new IncrementOrAddNode< ImageSignal< float >,ImageSignal< float > >(1,1);
    
    IncrementOrAddNode< ImageSignal< float >,ImageSignal< float > >* b = 
        new IncrementOrAddNode< ImageSignal< float >,ImageSignal< float > >(1,1);
    
    IncrementOrAddNode< ImageSignal< float >,ImageSignal< float > >* c = 
        new IncrementOrAddNode< ImageSignal< float >,ImageSignal< float > >(2,1);
    
    // 第二步：将节点加入管线
    test->add(data_source,"data_source", SOURCE_NODE);
    test->add(a,"a");
    test->add(b,"b");
    test->add(c,"c", SINK_NODE);
    
    // 第三步：建立节点间的关系（a->c,b->c）
    // 实现 c=a+b
    test->bind("data_source",0,"a",0);
    test->bind("data_source",0,"b",0);
    test->bind("a",0,"c",0);
    test->bind("b",0,"c",1);
    
    EAGLEEYE_END_PIPELINE_INITIALIZE
    </code></pre>
* 创建自定义数据处理节点
    调用脚本
    ```c++
    python node --name=NODE --inputport=A,B --outputport=C 
    ```
    

####项目开发实例
见doc/。