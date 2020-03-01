if [ ! -d "./build" ]; 
then
  mkdir ./build
else
  rm -rf ./build
fi
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DOPENCL=True -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DNN_ENGINE=mace -DMACE_PATH=mace/ -DANDROID_STL=c++_shared -DANDROID_NATIVE_API_LEVEL=android-23 ..
make
cd ..
