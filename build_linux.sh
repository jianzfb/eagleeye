if [ ! -d "./build" ]; 
then
  mkdir ./build
else
  rm -rf ./build
fi
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release  ..
make
cd ..
