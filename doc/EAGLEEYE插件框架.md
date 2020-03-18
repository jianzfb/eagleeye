#EAGLEEYE插件框架
---
####简介
基于AI算法加持的应用，多人协同和小步迭代式是最常见的开发模式。统一的模块接口定义和模块动态更新是快速推动算法落地的必要前提。EAGLEEYE所提供的数据流管道框架和插件框架为大规模算法落地提供了基础平台。

####插件管理机制
![](./resource/plugin_framework.png)
在EAGLEEYE中依靠dlopen/dlsym实现动态加载插件过程。开发者开发的算法插件统一放置于预定义的插件目录，比如

<pre><code>
/data/data/your app/plugin/
</code></pre>

在EagleeyeModule.h中定义了所有插件管理的接口函数，包括
* 初始化插件框架
    
    <pre>
    <code>
    bool eagleeye_init_module(std::vector < std::string > & pipeline_names, const char* plugin_folder);
    </code></pre>
    
    参数：
    * plugin_folder指定存放插件的目录
    * pipeline_names获得返回的符合要求的插件

    说明：
    调用此函数获得所有符合要求的插件列表

* 插件初始化

    <pre>
    <code>
    bool eagleeye_pipeline_initialize(const char* pipeline_name);
    </code></pre>

    参数：
    * pipeline_name 设置需要初始化的插件名字

    说明：
    初始化插件资源。当需要调用算法插件前，进行调用。

* 释放插件资源

    <pre>
    <code>
    bool eagleeye_pipeline_release(const char* pipeline_name);
    </code></pre>

    参数：
    * pipeline_name设置需要释放的插件名字
    
    说明：
    释放插件资源。当退出当前算法插件时，进行调用。

* 释放插件框架

    <pre>
    <code>
    bool eagleeye_release_module()；
    </code></pre>

    说明：
    释放插件框架。当全局退出时，进行调用。

####算法插件创建
下面的例子是示例工程中的运动检测算法插件（来自eagleeye/model-sample/movingdet_plugin.cpp）。

编写算法插件，只需依靠在EagleeyeModule.h中定义的宏便可轻松完成：

* 定义插件注册函数
    <pre>
    <code>
    EAGLEEYE_PIPELINE_REGISTER(pipeline, version, key);
    </code></pre>
    
    参数：
    * pipeline 定义插件名字
    * version 定义插件版本
    * key 定义申请的插件签名（目前暂未启用）

    在movingdet_plugin.cpp中的调用方式如下

    <pre><code>
    EAGLEEYE_PIPELINE_REGISTER(movingdet, 1.0, xxxxxxx);
    </code></pre>

* 定义插件初始化函数
    <pre><code>
    EAGLEEYE_BEGIN_PIPELINE_INITIALIZE(movingdet)
    // 实现算法处理管线搭建（详见数据流编程框架介绍）
    // 第一步：定义数据源，用以接收外部传入的数据
    DataSourceNode<ImageSignal<Array<unsigned char, 3>>>* data_source = 
                new DataSourceNode<ImageSignal<Array<unsigned char, 3>>>();
    data_source->setSourceType(EAGLEEYE_SIGNAL_IMAGE);
    data_source->setSourceTarget(EAGLEEYE_CAPTURE_PREVIEW_IMAGE);
    
    // 第二步：定义数据变换节点，resize到指定大小
    ImageTransformNode* image_transform_node = new ImageTransformNode(false);
    image_transform_node->setMinSize(96);
    
    // 第三步：光流计算节点
    OpticalFlowNode* optical_flow = new OpticalFlowNode();
    optical_flow->setBRIEFSamplingRad(7);
    optical_flow->setRandomSearchRad(5);
    optical_flow->setMedianFSize(3);
    
    // 第四步：运动检测节点
    MovingDetNode* mdn = new MovingDetNode();
    
    // 第五步：将处理节点加入算法管线（movingdet）
    movingdet->add(data_source, "source");
    movingdet->add(image_transform_node, "image_transform_node");
    movingdet->add(optical_flow, "optical_flow");
    movingdet->add(mdn, "mdn");
    
    // 第六步：将处理节点建立关联，构建管线
    movingdet->bind("source",0, "image_transform_node", 0);
    movingdet->bind("image_transform_node", 0, "mdn", 0);
    movingdet->bind("image_transform_node", 0, "optical_flow", 0);
    movingdet->bind("optical_flow",0,"mdn", 1);
    EAGLEEYE_END_PIPELINE_INITIALIZE
    </code></pre>

####通用算法插件集成/调用
插件集成分为三个步骤
* 插件搜索及初始化
    <pre><code>
    // 第一步：在插件搜索路径中，检索所有插件
    std::vector<std::string> pipeline_names;
    eagleeye_init_module(pipeline_names, "/data/local/tmp/eagleeye/plugin/");
    // 第二步：初始化指定插件
    pipeline_name = pipeline_names[0];
    eagleeye_pipeline_initialize(pipeline_name.c_str());
    // 第三步：设置插件参数（底层依靠）
    // 第一个参数为插件名称
    // 第二个参数为计算网络图中的计算节点名字
    // 第三个参数为计算节点中的监控变量（见数据流编程框架细节）
    // 第四个参数为监控变量设置新数值=
    int k = 6;
    eagleeye_pipeline_set_param(pipeline_name.c_str(), "mdn", "K",&k);

    </code></pre>
* 插件运行
    包括设置计算管线的输入、计算管线运行、计算管线输出
    * 设置管线输入

        <pre><code>
        void* a_data = ...;
        int a_data_size[] = {rows, cols, 3};
        eagleeye_pipeline_set_input(pipeline_name.c_str(), "data_source", a_data, a_data_size, 3);
        </code><pre>

    * 管线运行

        <pre><code>
        eagleeye_pipeline_run(pipeline_name.c_str());
        </code></pre>
    * 获得管线输出

        <pre><code>
        void* out_data;
        int out_data_size[3];
        int out_data_dims=0;
        int out_data_type=0;
        eagleeye_pipeline_get_output(pipeline_name.c_str(), "mdn/0",out_data, out_data_size, out_data_dims, out_data_type);
        </code></pre>
        
        其中，out_data获得的数据指针；out_data_size获得的数据大小包括高、宽、通道数；out_data_type数据类型。
        数据类型定义如下
        char            - 0
        unsigned char   - 1
        short           - 2
        unsigned short  - 3
        int             - 4
        unsigned int    - 5
        float           - 6
        double          - 7
        Array< unsigned char, 3 >    - 8
        Array< unsigned char, 4 >    - 9
* 插件注销及模块注销
    <pre><code>
    // 第一步：释放插件
    eagleeye_pipeline_release(pipeline_name.c_str());
    // 第二步：释放模块
    eagleeye_release_module();
    </code></pre>

####快速生成插件代码模板
使用scripts/main.py快速创建插件模板

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

运行脚本后将在当前目录下生成以指定的插件名命名的文件夹（这里--project=movingdet），例如
<pre><code>
movingdet_plugin
    - movingdet_plugin.h
    - movingdet_plugin.cpp
    - movingdet_demo.cpp
    - CMakeLists.txt
    - build.sh
</code></pre>

开发者可以直接在自动生成的模板下进行插件开发。调用build.sh后，将会构建插件和对应DEMO工程。


####特定算法插件直接集成/调用
在一些特定应用中，不需要支持动态加载插件的能力。此时可以直接使用插件特有接口，实现算法调用。在这种集成方式下，编译出的插件.so文件需要静态链接进工程中。

以上面自动生成的插件模板（movingdet）为例，插件特有接口定义在movingdet_plugin.h中。

<pre><code>
extern "C"{
    /**
     * @brief initialize movingdet pipeline
     * 
     * @return true 
     * @return false 
     */
    bool eagleeye_movingdet_initialize();

    /**
     * @brief release movingdet pipeline
     * 
     * @return true 
     * @return false 
     */
    bool eagleeye_movingdet_release();

    /**
     * @brief run movingdet pipeline
     * 
     * @return true 
     * @return false 
     */
    bool eagleeye_movingdet_run();

    /**
     * @brief get movingdet pipeline version
     * 
     * @param pipeline_version
     * @return true 
     * @return false 
     */
    bool eagleeye_movingdet_version(char* pipeline_version);

    /**
     * @brief reset movingdet pipeline state
     * 
     * @return true 
     * @return false 
     */
    bool eagleeye_movingdet_reset();

    /**
     * @brief set any node param in movingdet pipeline
     * 
     * @param node_name node name in pipeline
     * @param param_name node param in pipeline
     * @param value param value
     * @return true  success to set
     * @return false fail to set
     */
    bool eagleeye_movingdet_set_param(const char* node_name, const char* param_name, const void* value);

    /**
     * @brief get any node param in movingdet pipeline
     * 
     * @param node_name node name in pipeline
     * @param param_name node param in pipeline
     * @param value param value
     * @return true success to get
     * @return false fail to get
     */
    bool eagleeye_movingdet_get_param(const char* node_name, const char* param_name, void* value);

    /**
     * @brief set input data from movingdet input node
     * 
     * @param node_name node name in pipeline
     * @param data data pointer
     * @param data_size dimension (H,W,C)
     * @param data_dims dimension number
     * @return true 
     * @return false 
     */
    bool eagleeye_movingdet_set_input(const char* node_name, void* data, const int* data_size, const int data_dims);

    /**
     * @brief get output data from movingdet output node
     * 
     * @param node_name node name in pipeline
     * @param data data pointer
     * @param data_size dimension (H,W,C)/(B,H,W,C)
     * @param data_dims dimension number
     * @param data_type data type
     * @return true 
     * @return false 
     */
    bool eagleeye_movingdet_get_output(const char* node_name, void*& data, int* data_size, int& data_dims,int& data_type);
}
</code></pre>

在集成时，依然分为三个步骤

* 插件初始化
    <pre><code>
    eagleeye_movingdet_initialize();
    </code></pre>
* 插件运行
    <pre><code>
    // 第一步：设置输入
    void* data = image.data;
    int data_size[] = {image.rows, image.cols, 3}; 
    eagleeye_movingdet_set_input("data_source", data, data_size, 3);
    // 第二步：驱动管线运行
    eagleeye_movingdet_run();
    </code></pre>
* 插件释放
    <pre><code>
    eagleeye_movingdet_release();
    </code></pre>