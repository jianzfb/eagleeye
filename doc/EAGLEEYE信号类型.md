#EAGLEEYE信号类型
----
###简介
EAGLEEYE中的信号起到传递数据处理节点间信息的作用。在建立数据处理节点的连接关系时，均基于信号构建。例如
```c++
    AnyNode* a = new 节点();
    AnyNode* b = new 节点();
    AnySignal* a_sig = a->getOutputPort(0);
    b->setInputPort(a_sig, 0);
```
其中，使用如下代码
```c++
    AnySignal* a_sig = a->getOutputPort(0);
```
获得数据处理节点a的第0号输出端口信号；使用如下代码
```c++
    b->setInputPort(a_sig, 0);
```
将a_sig信号放置于数据处理节点b的第0号输入端口。
进而在管线运行时，在数据处理节点b的重载函数
```c++
    void executeNodeInfo(){
        ImageSignal<Array<unsigned char, 3>>* a_sig = (ImageSignal<Array<unsigned char, 3>>*)b->getInputPort(0);
        Matrix<Array<unsigned char,3>> data = a_sig->getData();
        
        Matrix<Array<unsigned char,3>> result;
        ......

        ImageSignal<Array<unsigned char, 3>>* b_sig = (ImageSignal<Array<unsigned char, 3>>*)b->getOutputPort(0);
        b_sig->setData(result);
    }
```
中通过调用
```c++
    Matrix<Array<unsigned char,3>> data = a_sig->getData();
```
获取在信号a_sig中承载的数据，并在其中完成数据处理节点b的所有工作。最后调用
```c++
    ImageSignal<Array<unsigned char, 3>>* b_sig = (ImageSignal<Array<unsigned char, 3>>*)b->getOutputPort(0);
```
获取输出处理节点b的第0输出端口信号，并调用
```c++
    b_sig->setData(result);
```
给信号具体赋值。
###信号
* 图像信号
    是在EAGLEEYE中最常用的信号类型，所有矩阵类型数据均可采用此类型信号作为节点间的传输桥梁。
    ```c++
        template<class T>
        class ImageSignal:public BaseImageSignal{
            ...
        };
    ```
    ImageSignal是模板类，需要指定具体的数据类型进行构建。如
    ```c++
        // 构建三通道图像信号，其承载的数据是Matrix<Array<unsigned char, 3>>。
        ImageSignal<Array<unsigned char,3>>* a = new ImageSignal<Array<unsigned char,3>>();
        // 构建单通道图像信号，其承载的数据是Matrix<int>。
        ImageSignal<int>* b = new ImageSignal<int>();
    ```
    
    对于图像信号常用的5种操作，
    * 设置数据
        ```c++
        ImageSignal<Array<unsigned char,3>>* a = new ImageSignal<Array<unsigned char,3>>();
        Matrix<Array<unsigned char,3>> data;
        a->setData(data);
        ```
    * 获取数据
        ```c++
        ImageSignal<Array<unsigned char,3>>* a = new ImageSignal<Array<unsigned char,3>>();
        Matrix<Array<unsigned char,3>> data = a->getData();
        ```
    * 创建同类型信号
        ```c++
        ImageSignal<Array<unsigned char,3>>* a = new ImageSignal<Array<unsigned char,3>>();
        AnySignal* a_cp = a->make();
        ```
    * 复制信号（包括其承载的数据）
        ```c++
        ImageSignal<Array<unsigned char,3>>* a = new ImageSignal<Array<unsigned char,3>>();
        AnySignal* a_cp = a->make();
        a_cp->copy(a);
        ```
    * 转换成队列版本
        将承载数据模式转换成队列模式，也就是说当调用
        ```c++
        a->setData(...);
        ```
        时，将输入的数据置于内部维护队列中。当调用
        ```c++
        ... = a->getData();
        ```
        时，是阻塞调用，如果内部维护的队列无数据可用，则阻塞等待。
        转换到队列模式需要调用
        ```c++
        a->transformCategoryToQ();
        ```
* 张量信号
    专门针对神经网络计算使用的信号。
* 字符串信号
    承载字符串(std::string)的信号。
    ```c++
        class StringSignal:public AnySignal{

        };
    ```
    构建方式
    ```c++
        StringSignal* ss = new StringSignal();
    ```
    常用的6种操作
    * 设置数据
        ```c++
        std::string value = "hello the world";
        ss->setData(value);
        ```
    * 获取信号
        ```c++
        std::string value = ss->getData();
        ```
    * 创建同类型信号
        ```c++
        AnySignal* ss_cp = ss->make();
        ```
    * 复制信息
        ```c++
        ss_cp->copy(ss);
        ```
    * 转换成队列模式
        ```c++
        ss->transformCategoryToQ();
        ```
    * 设置初始状态
        ```c++
        ss->setInit("");
        ```
* 状态信号
    承载状态信息(int)的信号。
    ```c++
    class StateSignal:public AnySignal{
        public:
            StateSignal(int ini_state=0);
            ...
        private:
            int m_state;
    }
    ```
    构建方式
    ```c++
    StateSignal* ss = new StateSignal(0);
    ```
    常用的6种操作
    * 设置数据
        ```c++
        int value = 10;
        ss->setData(value);
        ```
    * 获取信号
        ```c++
        int value = ss->getData();
        ```
    * 创建同类型信号
        ```c++
        AnySignal* ss_cp = ss->make();
        ```
    * 复制信息
        ```c++
        ss_cp->copy(ss);
        ```
    * 转换成队列模式
        ```c++
        ss->transformCategoryToQ();
        ```
    * 设置初始状态
        ```c++
        ss->setInit(1);
        ```
* 控制信号
    承载布尔信息的信号。
    ```c++
    class BooleanSignal:public AnySignal{
        public:
            BooleanSignal(bool ini_boolean=false);
            ...
        private:
            bool m_boolean;	
    };
    ```
    构建方式
    ```c++
    BooleanSignal* bs = new BooleanSignal(true);
    ```
    常用的5种操作
    * 设置数据
        ```c++
        bool value = false;
        bs->setData(value);
        ```
    * 获取信号
        ```c++
        bool value = bs->getData();
        ```
    * 创建同类型信号
        ```c++
        AnySignal* bs_cp = bs->make();
        ```
    * 复制信息
        ```c++
        bs_cp->copy(ss);
        ```
    * 转换成队列模式
        ```c++
        bs->transformCategoryToQ();
        ```
    * 设置初始状态
        ```c++
        ss->setInit(false);
        ```