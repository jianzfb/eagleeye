if [ ! -d "./build" ]; 
then
  mkdir ./build
else
  rm -rf ./build
fi
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLITE=True -DOPENMP=True -DOPENCL=/Users/zhangjian52/Documents/workspace/eagleeye/eagleeye/3rd/opencl -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DNN_ENGINE=paddle -DPADDLE_PATH=/Users/zhangjian52/Downloads/inference_lite_lib.android.armv8.opencl/cxx -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
make
cd ..
