if [ -d "./build" ]; 
then
  rm -rf build
fi
# 0.step 当前目录
CRTDIR=$(pwd)

# 1.step 更新依赖
git submodule init
git submodule update

# 2.step 准备环境
arm_cross_build_root_path=/opt/cross_build/linux-arm64
if [ -d "$arm_cross_build_root_path" ]; then
  echo "use /opt/cross_build/linux-arm64 cross compile env"
else
  source env/prepare_arm_cross_build_env_10.2.sh
fi

# 3.step 编译
mkdir build
cd build

tool_chain_path="/opt/cross_build/linux-arm64/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu"

# arm64编译
cmake -DCMAKE_BUILD_TYPE=Release \
  -DARM_ABI=arm64-v8a  \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64  \
  -DTOOLCHAIN_PATH=$tool_chain_path \
  -DCMAKE_C_COMPILER=$tool_chain_path/bin/aarch64-none-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=$tool_chain_path/bin/aarch64-none-linux-gnu-g++ \
  -DCMAKE_FIND_ROOT_PATH="$tool_chain_path/aarch64-linux-gnu;/opt/cross_build/linux-arm64/zlib-1.3.1" \
  -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
  -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
  -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
  -DZLIB_ROOT=/opt/cross_build/linux-arm64/zlib-1.3.1 \
  -DZLIB_INCLUDE_DIR=/opt/cross_build/linux-arm64/zlib-1.3.1 \
  -DZLIB_LIBRARY=/opt/cross_build/linux-arm64/zlib-1.3.1/libz.so \
  -DFFMPEG=/root/.3rd/ffmpeg \
  -DRKCHIP=/root/.3rd/rk  \
  -DMINIO:BOOL=OFF \
  ..

make -j 6
cd ..

install_dir="linux-arm64-v8a-install"
# 4.step 安装
if [ -d $install_dir ]; 
then
  rm -rf $install_dir
fi
mkdir include

host_os=`uname  -a`
if [[ $host_os == Darwin* ]];then
  find ./eagleeye \( -path "./eagleeye/3rd" -o -path "./eagleeye/codegen" -o -path "./eagleeye/test" \) -prune -o -name "*.hpp" -type f -exec rsync -R {} include/ \;
  find ./eagleeye \( -path "./eagleeye/3rd" -o -path "./eagleeye/codegen" -o -path "./eagleeye/test" \) -prune -o -name "*.h" -type f -exec rsync -R {} include/ \;
else
  find ./eagleeye \( -path "./eagleeye/3rd" -o -path "./eagleeye/codegen" -o -path "./eagleeye/test" \) -prune -o -name "*.hpp" -type f -exec cp --parent -r {} include/ \;
  find ./eagleeye \( -path "./eagleeye/3rd" -o -path "./eagleeye/codegen" -o -path "./eagleeye/test" \) -prune -o -name "*.h" -type f -exec cp --parent -r {} include/ \;
fi


mkdir $install_dir
cd $install_dir
mkdir libs
cd ..
mv include $install_dir/
mv bin/* $install_dir/libs/
rm -rf bin

# 5.step 第三方库（opencl）
cd $install_dir
mkdir 3rd
cp -r ../eagleeye/3rd/opencl 3rd/
cp -r ../eagleeye/3rd/eigen 3rd/
cp -r ../eagleeye/3rd/opencv 3rd/
cd ..

# 6.step 脚本工具
cp -r scripts $install_dir

ldconfig
