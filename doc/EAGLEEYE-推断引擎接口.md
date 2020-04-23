#EAGLEEYE-推断引擎接口
----
####简介
深度学习网络模型推断引擎负责完成神经网络的推断任务，目前EAGLEEYE框架集成了三个常用的引擎接口，（1）SNPE推断引擎；（2）MACE推断引擎；（3）TensorFlow推断引擎。

####推断引擎封装逻辑

在EAGLEEYE框架中，所有的引擎通过ModelRun类进行隔离。在编译期间通过设置不同的宏定义进行选择编译。比如
|宏定义|引擎类型|
|---|---|
|EAGLEEYE_SNPE_SUPPORT|SNPE 引擎|
|EAGLEEYE_TF_SUPPORT|TensorFlow 引擎|
从开发者角度来看，面对的是统一的类ModelRun，下面看一下ModelRun类的定义
```c++
class ModelRun: public ModelEngine{
public:
    // 构建模型
    ModelRun(std::string model_name, 
             std::string device,
             std::vector<std::string> input_names=std::vector<std::string>(),
             std::vector<std::vector<int64_t>> input_shapes=std::vector<std::vector<int64_t>>(),
             std::vector<std::string> output_names=std::vector<std::string>(),
             std::vector<std::vector<int64_t>> output_shapes=std::vector<std::vector<int64_t>>(),
             int omp_num_threads=-1, 
             int cpu_affinity_policy=-1, 
             std::string writable_path="/data/local/tmp/");
    // 模型析构         
    virtual ~ModelRun();
    
    // 模型运行
    virtual bool run(std::map<std::string, unsigned char*> inputs, 
                     std::map<std::string, unsigned char*>& outputs);
    
    // 初始化模型
    virtual bool initialize();
    
    // 设置模型可写目录
    virtual void setWritablePath(std::string writable_path);
    
    // 获得输入节点内存指针
    virtual void* getInputPtr(std::string input_name);

private:
    ...
};
```
* 构造函数
    * model_name 指定模型名字。
    自动在指定的可写目录下自动寻找模型文件。对于SNPE引擎，则自动寻找model_name.dlc；对于TensorFlow引擎，则自动寻找model_name.pb。
    * device 指定运行设备
    "GPU"、"DSP"、"NPU"、"CPU"
    * input_names 指定输入节点名字
     例如，
        ``` c++ 
        std::vector<std::string>{"input_1","input_2"}
        ```
    * input_shapes 指定输入节点形状
    例如，
        ```c++
        std::vector<std::vector<int>>{std::vector<int>{1,256,256,3},std::vector<int>{1,256,256,3}}
        ```
    * output_names 指定输出节点名字
    例如，
        ```c++
        std::vector<std::string>{"output_1","output_2"}
        ```
    * output_shapes 指定输出节点形状
    例如， 
        ```c++
        std::vector<std::vector<int>>{std::vector<int>{1,256,256,3},std::vector<int>{1,256,256,3}}
        ```
    * omp_num_threads 指定线程数
    * cpu_affinity_policy 指定线程亲和度
    * writable_path 指定可写目录
    模型运行涉及的相关资源都放置在此文件下，包括模型文件、DSP\NPU相关库(SNPE引擎使用)
* setWritablePath
    指定可写目录
* initialize
    在指定的可写目录下，找寻指定的模型文件。对于SNPE引擎，则自动寻找model_name.dlc；对于TensorFlow引擎，则自动寻找model_name.pb。然后对推断框架初始化。
* getInputPtr
    根据输入节点名字获取推断框架的内存指针名字，以防止外部冗余的内存申请。

在proxy_run.h文件中，可以看到系统是如何根据宏定义进行选择性编译的，代码片段如下
```c++
    #if EAGLEEYE_TF_SUPPORT
        #include "eagleeye/engine/tf_run.h"
    #elif EAGLEEYE_SNPE_SUPPORT
        #include "eagleeye/engine/snpe_run.h"
    #else
        ...
```

> 注意，当底层使用SNPE推断引擎时，模型文件中对于模型输出节点名字的定义，与我们生成pb（TensorFlow输出的模型文件）文件时对应节点的定义不同。其分为计算节点的名字和输出张量的名字。举例来说，如果输出节点是卷积算子的输出，那么节点的名字则是.../Conv_1/Conv2D，而输出张量的名字为.../Conv_1/BiasAdd。对于其他算子一般情况下这两个名字相同（使用tf.identity(...)进行的重命名无效）。建议采用SNPE提供的工具snpe-view-dlc对dlc模型文件进行可视化查看。

####引擎应用示例
以下代码是基于SNPE推断引擎的人像分割模型集成示例（对于其他引擎代码完全相同），
```c++
    // 1.step 模型构建 (指定在DSP硬件执行)
    // 1.1.step 设置模型名字（在初始化时，将自动寻找model_name.dlc模型文件）
    std::string model_name = "portrait_model";
    // 1.2.step 设置输入图像尺寸
    int model_h = 384;
    int model_w = 384;
    // 1.3.step 设置输出张量尺寸
    int output_h = 384;
    int output_w = 384;
    ModelRun* seg_model = new ModelRun(model_name,
                                        "DSP",
                                        std::vector<std::string>{"input_node"},
                                        std::vector<std::vector<int64_t>>{
                                                std::vector<int64_t>{1, model_h, model_w, 3}},
                                        std::vector<std::string>{"output_node"},
                                        std::vector<std::vector<int64_t>>{
                                                std::vector<int64_t>{1, output_h, output_w, 2}});

    // 2.step 设置输出节点名字映射关系
    // 对于SNPE推断框架，考虑到计算效率问题，对原始模型文件中定义的相关算子
    // 和结构需要重新精简和构建，比如，去除tf.identity(...)，将BatchNorm
    // 合并入上一级卷积计算中，等。故一般通过tf.identity重定义的算子名字，
    // 在转换后的SNPE模型文件中是不存在的。由于要兼容多个底层推断引擎（有些引擎会保留重定义的名字），
    // 故在ModelRun的构造函数中，设置的输出节点名字为重定义的算子名字（这里是"output_node"），
    // 但还需要定义此名字与真实的模型中算子名字和输出张量名字的对应关系。

    // 2.1.step 建立与算子名字的对应关系
    std::map<std::string, std::string> output_name_map;
    output_name_map["output_node"] = "output_logits/Conv_1/Conv2D";
    seg_model->setOutputNameMap(output_name_map);

    // 2.2.step 建立与输出张量名字的对应关系
    std::map<std::string, std::string> output_name_map2;
    output_name_map2[output_node] = "output_logits/Conv_1/BiasAdd";
    seg_model->setOutputNameMap2(output_name_map2);

    // 3.step 设置可写目录
    // 将SNPE对应的模型文件放置于此目录下，由于这里要求模型运行在DSP上，
    // 故还需将对应DSP库放置于此目录。
    seg_model->setWritablePath(".../.../...")

    // 4.step 运行模型
    // 4.1.step 设置输入节点内存块（将输入数据填充其中）
    std::map<std::string, unsigned char *> inputs;
    void* in_ptr = (void*)malloc(sizeof(float)*model_h*model_w*3);
    // 填充内存
    ...
    inputs["input_node"] = in_ptr;
    std::map<std::string, unsigned char *> outputs;

    // 4.2.step 运行模型
    bool status = this->seg_model->run(inputs, outputs);

    // 4.3.step 重输出节点获取对应结果
    void* out_ptr = outputs["output_node"];

```