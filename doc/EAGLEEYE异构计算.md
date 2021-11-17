#EAGLEEYE异构计算
####简介
所谓异构计算，是指CPU+ GPU或者CPU+ 其它设备（如FPGA等）协同计算。一般我们的程序，是在CPU上计算。但是，当大量的数据需要计算时，CPU显得力不从心。那么，是否可以找寻其它的方法来解决计算速度呢？那就是异构计算。例如可利用CPU（Central Processing Unit）、GPU（Graphic Processing Unit）、甚至APU(Accelerated Processing Units， CPU与GPU的融合)。等计算设备的计算能力从而来提高系统的速度。
![]()

为了充分利用异构资源，需要关注两个方面，（1）如何构建任务图谱；（2）如何进行任务调度到计算设备；（3）如何拆分掩藏设备间数据传输时间；

####构建任务有向无环图
基于任务间的依赖关系，创建任务图。下面我们看一下，创建任务类，

```c++
#include "eagleeye/engine/nano/dataflow/base.h"
class ExampleOp:public BaseOp<Tensor<float>, 2, 1>{    
public:
    ExampleOp()
        :BaseOp<Tensor<float>,2,1>(){};
    virtual ~ExampleOp(){}

    virtual void init(void* data){
        // 初始化输出数据形状
        this->m_output_shape[0] = std::vector<int64_t>{1,5,5,1};
    }

    virtual void runOnCpu(std::vector<Tensor<float>> output, std::vector<Tensor<float>> input){
        // 虚函数，编写运行在CPU上的任务代码
        // 获取CPU下的数据指针
        void* input_cpu_ptr = input[0].cpu();
        void* output_cpu_ptr = output[0].cpu();
    }

    virtual void runOnGpu(std::vector<Tensor<float>> output, std::vector<Tensor<float>> input){
        // 虚函数，编写运行在GPU上的任务代码
        // 获取GPU下的数据buffer
        void* input_gpu_buff = input[0].gpu();
        void* output_gpu_buff = output[0].gpu();
    }
};
```
其中，基类BaseOp是模板类，模板的三个模板参数，第一个是设置任务的数据类型，第二个是设置输入数据个数，第三个是设置输出数据个数。

创建任务图，
```c++
// 第一步，创建任务图，并设置计算单元和线程数
Graph g(std::vector<EagleeyeRuntime>{EagleeyeRuntime(EAGLEEYE_CPU), EagleeyeRuntime(EAGLEEYE_GPU)}, 2);
// 第二步，创建任务，并指定入口任务、中间任务、和出口任务类型
auto& a1 = g.addNode("a1",ExampleEntry(2),ENTRY,EagleeyeRuntime(EAGLEEYE_CPU));
auto& a2 = g.addNode("a2",ExampleEntry(10),ENTRY,EagleeyeRuntime(EAGLEEYE_CPU));

auto& b = g.addNode("b",ExampleOp(),DEFAULT,EagleeyeRuntime(EAGLEEYE_CPU));
auto& c = g.addNode("c",ExampleOp(),DEFAULT,EagleeyeRuntime(EAGLEEYE_CPU));
auto& d = g.addNode("d",ExampleOp(),EXIT,EagleeyeRuntime(EAGLEEYE_CPU));

// 第三步，连接任务建的关系
// 将a1任务的第0个数据，绑定到任务b的输入数据中
g.connect(a1,0,b);
// 将a2任务的第0个数据，绑定到任务b的输入输入中
g.connect(a2,0,b);
g.connect(b,0,c);
g.connect(a2,0,c);
g.connect(b,0,d);
g.connect(c,0,d);

```

####构建任务调度算法
在构建任务图时，构造函数的第三个参数就是设置任务调度算法，
```c++
Graph(std::vector<EagleeyeRuntime> runtimes, 
        std::size_t num_worker = 4,
        ScheduleType schedule_type=NO_SCHEDULE){

}
```
目前支持，两种任务调度算法，
* 人工指定
* HEFT算法

####构建数据异步传输
在异构设备下，设备间的数据传输代价往往不可忽视，尤其对于一些实时应用算法。掩盖数据传输代价的关键就是，如何将数据传输在实际需要（依赖它的任务执行时）前启动？

在Graph中，实现了相关功能，
```c++
  void fire_(std::size_t id, Node & n) {
    n.count_ = 0;
    // 1.step schedule 
    EagleeyeRuntime runtime = this->schedule_->getRuntime(&n);

    // 2.step run node op
    n.fire(runtime);

    // 3.step finish node
    finish(&n);

    // 4.step active succeed nodes
    int i = id;
    for (Edge* next : n.next_) {
      unsigned n = next->next().count_.fetch_add(1);
      if (next->next().prev_.size() == (n + 1)){
        // 4.1.step transfer data asyn
        Node* next_node = &next->next();
        next_node->transfer(this->schedule_->getRuntime(next_node));

        // 4.2.step push to queue, prepare to execute
        queue_[i % numWorker()].push(next_node);
        ++i;
      }
    }
  }

```
上面的fire_函数是Graph的成员函数，用于触发任务的执行，并在执行后，根据任务依赖关系获得下继任务节点的运行设备并启动数据的异步传输，
```c++
    next_node->transfer(this->schedule_->getRuntime(next_node));
```
当然执行数据传输的前提是，任务所依赖的数据支持此功能。目前eagleeye框架下的Matrix和Tensor类均支持此操作。

####简单示例
```c++
    // 第一步，创建任务图，并设置计算单元和线程数
    Graph g(std::vector<EagleeyeRuntime>{EagleeyeRuntime(EAGLEEYE_CPU), EagleeyeRuntime(EAGLEEYE_GPU)}, 2);
    // 第二步，创建任务，并指定入口任务、中间任务、和出口任务类型
    auto& a1 = g.addNode("a1",ExampleEntry(2),ENTRY,EagleeyeRuntime(EAGLEEYE_CPU));
    auto& a2 = g.addNode("a2",ExampleEntry(10),ENTRY,EagleeyeRuntime(EAGLEEYE_CPU));

    auto& b = g.addNode("b",ExampleOp(),DEFAULT,EagleeyeRuntime(EAGLEEYE_CPU));
    auto& c = g.addNode("c",ExampleOp(),DEFAULT,EagleeyeRuntime(EAGLEEYE_CPU));
    auto& d = g.addNode("d",ExampleOp(),EXIT,EagleeyeRuntime(EAGLEEYE_CPU));

    // 第三步，连接任务建的关系
    // 将a1任务的第0个数据，绑定到任务b的输入数据中
    g.connect(a1,0,b);
    // 将a2任务的第0个数据，绑定到任务b的输入输入中
    g.connect(a2,0,b);
    g.connect(b,0,c);
    g.connect(a2,0,c);
    g.connect(b,0,d);
    g.connect(c,0,d);

    // 初始化任务（包括初始化任务参数，分配内存空间等）
    g.init(NULL);
    // 分析任务关系
    g.analyze();
    // 运行任务
    g.run();

    // 获得任务d的输出数据
    Node* n = g.find("d");
    Tensor<float> dd = *(Tensor<float>*)(n->data(0));
```
####NANO神经网络推断引擎
正在开发中