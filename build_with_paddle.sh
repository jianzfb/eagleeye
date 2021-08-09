if [ ! -d "./build" ]; 
then
  mkdir ./build
else
  rm -rf ./build
fi
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=/Users/zhangjian52/Documents/workspace/eagleeye/eagleeye/3rd/opencl -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DFFMPEG=/Users/zhangjian52/Documents/backup/其他/ffmpeg/arm64-v8a -DX264=/Users/zhangjian52/Documents/backup/其他/X264/arm64-v8a -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
# cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=/Users/zhangjian52/Documents/workspace/eagleeye/eagleeye/3rd/opencl -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
# cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=/Users/zhangjian52/Documents/workspace/eagleeye/eagleeye/3rd/opencl -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=armeabi-v7a -DANDROID_ARM_NEON=ON -DANDROID_NATIVE_API_LEVEL=android-23 ..

# cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=/Users/zhangjian52/Documents/workspace/eagleeye/eagleeye/3rd/opencl -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DNN_ENGINE=paddle -DPADDLE_PATH=/Users/zhangjian52/Downloads/inference_lite_lib.android.armv8.opencl/cxx -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
make
cd ..
