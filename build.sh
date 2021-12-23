if [ -d "./build" ]; 
then
  rm -rf build
fi
# 1.step 编译
mkdir build
cd build
# cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=/Users/zhangjian52/Documents/workspace/eagleeye/eagleeye/3rd/opencl -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DFFMPEG=/Users/zhangjian52/Documents/backup/其他/ffmpeg/arm64-v8a -DX264=/Users/zhangjian52/Documents/backup/其他/X264/arm64-v8a -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=/Users/jian/Downloads/workspace/mltalker/eagleeye/eagleeye/3rd/opencl -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
# cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=/Users/zhangjian52/Documents/workspace/eagleeye/eagleeye/3rd/opencl -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=armeabi-v7a -DANDROID_ARM_NEON=ON -DANDROID_NATIVE_API_LEVEL=android-23 ..
# cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=/Users/zhangjian52/Documents/workspace/eagleeye/eagleeye/3rd/opencl -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DNN_ENGINE=paddle -DPADDLE_PATH=/Users/zhangjian52/Downloads/inference_lite_lib.android.armv8.opencl/cxx -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
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

# 3.step 第三方库（opencl）
cd install
mkdir 3rd
cp -r ../eagleeye/3rd/opencl 3rd/
cp -r ../eagleeye/3rd/Eigen 3rd/
cp -r ../eagleeye/3rd/opencv 3rd/
cd ..

# 4.step 脚本工具
cp -r scripts install/