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
if [ "$1"x = "app"x ]; then
cmake -DCMAKE_BUILD_TYPE=Release -DLITE=ON -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 -DANDROID_APP=ON -DWITH_OPENGL=ON -DRKCHIP=$2 ..
else
cmake -DCMAKE_BUILD_TYPE=Release -DLITE=ON -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 -DWITH_OPENGL=ON -DRKCHIP=$1 ..

fi
# arm32编译
# cmake -DCMAKE_BUILD_TYPE=Release -DLITE=ON -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=armeabi-v7a -DANDROID_ARM_NEON=ON -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
# 使用ffmpeg
# cmake -DCMAKE_BUILD_TYPE=Release -DLITE=ON -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DFFMPEG=/Users/zhangjian52/Documents/backup/其他/ffmpeg/arm64-v8a -DX264=/Users/zhangjian52/Documents/backup/其他/X264/arm64-v8a -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
# 使用paddlelite引擎
# cmake -DCMAKE_BUILD_TYPE=Release -DLITE=ON -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DNN_ENGINE=paddle -DPADDLE_PATH=/Users/zhangjian52/Downloads/inference_lite_lib.android.armv8.opencl/cxx -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
make
cd ..

# 3.step 安装
if [ -d "./install" ]; 
then
  rm -rf install
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


mkdir install
cd install
mkdir libs
cd ..
mv include install/
mv bin/* install/libs/
rm -rf bin

# 4.step 第三方库（opencl）
cd install
mkdir 3rd
cp -r ../eagleeye/3rd/opencl 3rd/
cp -r ../eagleeye/3rd/eigen 3rd/
cp -r ../eagleeye/3rd/opencv 3rd/
cp -r ../eagleeye/3rd/libyuv 3rd/
cd ..

# 5.step 脚本工具
cp -r scripts install/

# 6.step 重命名
mv install android-install