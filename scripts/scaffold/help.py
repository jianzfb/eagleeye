import os

def printhelp():
    print("\n生成工程模板\n")
    print('''
    eagleeye-cli project --project=movingdet     \\\\定义要生成的插件名字
            --version=1.0.0.0           \\\\ 定义插件版本
            --signature=xxxxx           \\\\ 定义插件签名（目前未启用）
            --build_type=Release        \\\\ 定义编译版本
            --abi=arm64-v8a             \\\\ 定义abi
            --eagleeye=EAGLEEYE_PATH    \\\\ 定义eagleeye安装路径
            ''')

    print("生成数据处理节点模板\n")
    print('''
    eagleeye-cli node --project=NODE_NAME       \\\\ 数据处理节点名字
                  --inputport=PORT,PORT         \\\\ 输入端口名字
                  --outputport=PORT,PORT        \\\\ 输出端口名字
                  --eagleeye=EAGLEEYE_PATH      \\\\ 定义eagleeye安装路径
                  --abi=arm64-v8a               \\\\ 定义abi
                  --build_type=Release          \\\\ 定义编译版本
    ''')
    print("发布插件\n")
    print('''
    eagleeye-cli release --folder=YOUR_FOLDER   \\\\ 包含.so，等依赖资源
                    --project=PLUGIN_NAME       \\\\ 定义发布名字
    ''')