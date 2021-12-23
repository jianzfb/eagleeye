#EAGLEEYE之Blob类
####简介
Blob类实现了多设备存储自由转换，其构造函数如下，
```c++
    Blob(size_t size, 
         Aligned aligned=Aligned(64), 
         EagleeyeRuntime runtime=EagleeyeRuntime(EAGLEEYE_CPU), 
         void* data=NULL, 
         bool copy=false, 
         std::string group="default");
```
参数列表
|参数名字|含义|
|---|---|
|size|内存大小|
|aligned|对齐比特数|
|runtime|设备|
|data|初始化数据指针|
|copy|拷贝或共享|
|group|-|

####申请内存
申请在内存中的空间，
```c++
Blob blob(1024, Aligned(64), EagleeyeRuntime(EAGLEEYE_CPU));
```
获得对应显存空间，
```c++
void* gpu_mem = blob.gpu(); // cl_mem
```
在某些性能优先场景下，为了掩盖从内存到显存的数据传输时间，我们可以采用非阻塞调用模式，如下
```c++
// 启动GPU到CPU数据传输（非阻塞模式） 
blob.transfer(EagleeyeRuntime(EAGLEEYE_GPU), true);
// 做一些其他事情,掩盖blob的设备间的数据传输时间
...
// 等待数据传输完成并返回
void* gpu_mem = blob.gpu();
```
将blob主存储位置调度到GPU设备上，
```c++
blob.schedule(EagleeyeRuntime(EAGLEEYE_GPU), true);
```

####申请显存
申请在显存中的空间
```c++
Blob blob(1024, Aligned(64), EagleeyeRuntime(EAGLEEYE_GPU));
```
获得对应的内存空间，
```c++
void* cpu_mem = blob.cpu();
```
在某些性能优先场景下，为了掩盖从内存到显存的数据传输时间，我们可以采用非阻塞调用模式，如下
```c++
// 启动CPU到GPU数据传输（非阻塞模式） 
blob.transfer(EagleeyeRuntime(EAGLEEYE_CPU), true);
// 做一些其他事情,掩盖blob的设备间的数据传输时间
...
// 等待数据传输完成并返回
void* cpu_mem = blob.cpu();
```
将blob主存储位置调度到CPU设备上，
```c++
blob.schedule(EagleeyeRuntime(EAGLEEYE_CPU), true);
```