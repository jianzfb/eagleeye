import os

def printhelp():
    print("\n生成工程模板\n")
    print('''
    eagleeye-cli project --project=movingdet     \\\\定义要生成的插件名字
            --version=1.0.0.0       \\\\ 定义插件版本
            --signature=xxxxx       \\\\ 定义插件签名（目前未启用）
            --build_type=Release    \\\\ 定义编译版本
            --opencv=OPENCV_PATH    \\\\ 定义opencv路径(选择性设置)
            --abi=arm64-v8a         \\\\ 定义abi
            --eagleeye=EAGLEEYE_PATH \\\\ 定义eagleeye路径
            --opencl=OPENCL_PATH    \\\\ 定义OPENCL路径（选择性设置）
            --neon=1                \\\\定义NEON加速(1/0)
            ''')

    print("生成数据处理节点模板\n")
    print('''
    eagleeye-cli node --name=NODE_NAME       \\\\ 数据处理节点名字
                  --inputport=PORT,PORT  \\\\ 输入端口名字
                  --outputport=PORT,PORT \\\\ 输出端口名字

    ''')
    print("发布插件\n")
    print('''
    eagleeye-cli release --folder=YOUR_FOLDER   \\\\ 包含.so，等依赖资源
                    --name=PLUGIN_NAME      \\\\ 定义发布名字
    ''')
    print('转换数据到无格式\n')
    print('''
    eagleeye-cli image2raw --folder=IMAGE_FOLDER        \\\\包含图片的文件夹
                        --mean=127.5,127.5,127,5    \\\\均值
                        ---var=255.0,255.0,255.0    \\\\方差
                        ---size=255,255             \\\\目标大小
    ''')