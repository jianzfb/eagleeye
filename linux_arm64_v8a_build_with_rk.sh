if [ -d "./build" ]; 
then
  rm -rf build
fi
# 0.step 当前目录
CRTDIR=$(pwd)

# 1.step 更新依赖
git submodule init
git submodule update

# 2.step 编译
mkdir build
cd build

# arm64编译
if [[ $1 == BUILD_PYTHON_MODULE ]];then
cmake -DCMAKE_BUILD_TYPE=Release -DARM_ABI=arm64-v8a -DBUILD_PYTHON_MODULE:BOOL=ON ..
else
cmake -DCMAKE_BUILD_TYPE=Release -DARM_ABI=arm64-v8a -DRKCHIP=/root/.3rd/rk -DMINIO:BOOL=ON -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake ..
fi
make -j 6
cd ..

install_dir="linux-arm64-v8a-install"
# 3.step 安装
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

# 4.step 第三方库（opencl）
cd $install_dir
mkdir 3rd
cp -r ../eagleeye/3rd/opencl 3rd/
cp -r ../eagleeye/3rd/eigen 3rd/
cp -r ../eagleeye/3rd/opencv 3rd/
cd ..

# 5.step 脚本工具
cp -r scripts $install_dir

ldconfig
