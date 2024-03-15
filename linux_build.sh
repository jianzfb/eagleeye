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
if [[ $1 == BUILD_PYTHON_MODULE ]];then
cmake -DCMAKE_BUILD_TYPE=Release -DX86_ABI=X86-64 -DLITE=ON -DBUILD_PYTHON_MODULE:BOOL=ON ..
else
cmake -DCMAKE_BUILD_TYPE=Release -DX86_ABI=X86-64 -DLITE=ON ..
fi
make
cd ..

# 3.step 安装
if [ -d "./linux-install" ]; 
then
  rm -rf linux-install
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

mkdir linux-install
cd linux-install
mkdir libs
cd ..
mv include linux-install/
mv bin/* linux-install/libs/
rm -rf bin

# 4.step 第三方库
cd linux-install
# 第三方代码库
mkdir 3rd
cp -r ../eagleeye/3rd/eigen 3rd/
cp -r ../eagleeye/3rd/pybind11 3rd/

# 第三方依赖.so
cp -r  ../eagleeye/3rd/libyuv/lib/linux/X86-64/* libs/X86-64/
cd ..

# 5.step 脚本工具
cp -r scripts linux-install/
