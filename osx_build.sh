if [ -d "./build" ]; 
then
  rm -rf build
fi
# 1.step 编译
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DOSX_ABI=X86-64 ..
make
cd ..

# 2.step 安装
if [ -d "./install" ]; 
then
  rm -rf install
fi
mkdir include
find ./eagleeye \( -path "./eagleeye/3rd" -o -path "./eagleeye/codegen" -o -path "./eagleeye/test" \) -prune -o -name "*.hpp" -type f -exec rsync -R {} include/ \;
find ./eagleeye \( -path "./eagleeye/3rd" -o -path "./eagleeye/codegen" -o -path "./eagleeye/test" \) -prune -o -name "*.h" -type f -exec rsync -R {} include/ \;

mkdir install
cd install
mkdir libs
cd ..
mv include install/
mv bin/* install/libs/
rm -rf bin

# 3.step 第三方库
cd install
# 第三方代码库
mkdir 3rd
cp -r ../eagleeye/3rd/Eigen 3rd/

# 第三方依赖.so
cp -r  ../eagleeye/3rd/libyuv/lib/X86-64/* libs/X86-64/
cd ..

# 4.step 脚本工具
cp -r scripts install/