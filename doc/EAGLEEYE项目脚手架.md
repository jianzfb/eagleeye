#EAGLEEYE项目脚手架

####简介
EAGLEEYE项目脚手架用于快速生成模板工程等一系列模板代码。
####安装
进入eagleeye/scripts/目录，运行
```shell
    pip3 install -r requirements.txt
    python3 setup.py install
```
完成脚手架安装。
####生成工程模板
```shell
eagleeye-cli project --project=movingdet    \\ 定义要生成的插件名字
            --version=1.0.0.0       \\ 定义插件版本
            --signature=xxxxx       \\ 定义插件签名（目前未启用）
            --build_type=Release    \\ 定义编译版本
            --opencv=OPENCV_PATH    \\ 定义opencv路径(选择性设置)
            --abi=arm64-v8a         \\ 定义abi
            --eagleeye=EAGLEEYE_PATH \\ 定义eagleeye路径
            --host_platform=MACOS   \\ 设置主机平台（MACOS/LINUX）
```
####生成数据处理节点模板
```shell
eagleeye-cli node --name=NODE_NAME       \\ 数据处理节点名字
                  --inputport=PORT,PORT  \\ 输入端口名字
                  --outputport=PORT,PORT \\ 输出端口名字
```
####发布插件
```shell
eagleeye-cli release --folder=YOUR_FOLDER   \\ 包含.so，等依赖资源
                    --name=PLUGIN_NAME      \\ 定义发布名字
```
####转换数据到无格式
```shell
eagleeye-cli image2raw --folder=IMAGE_FOLDER        \\包含图片的文件夹
                        --mean=127.5,127.5,127,5    \\均值
                        ---var=255.0,255.0,255.0    \\方差
                        ---size=255,255             \\目标大小
```