#!/bin/bash

arm_cross_build_root_path=/opt/cross_build/linux-arm64
if [ -d "$arm_cross_build_root_path" ]; then
  echo "Directory exists. Changing to $arm_cross_build_root_path"
else
  echo "Directory does not exist. Creating $arm_cross_build_root_path"
  mkdir -p "$arm_cross_build_root_path"
fi
cd "$arm_cross_build_root_path" || exit

# 1. toolchain download
wget https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz  && \
tar -xvf gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu.tar.xz

# 2. zlib install
wget https://www.zlib.net/zlib-1.3.1.tar.gz && tar -zxvf zlib-1.3.1.tar.gz && export CC=$arm_cross_build_root_path/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc && \
export CXX=$arm_cross_build_root_path/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-g++ && \
cd $arm_cross_build_root_path/zlib-1.3.1 && ./configure && make -j 10 && \
cp libz.a /opt/cross_build/linux-arm64/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/lib64/

# 3. ffmepg install 
if [ ! -d "/root/.3rd/ffmpeg" ];then
  mkdir /root/.3rd/ffmpeg && git clone -b release/7.0 https://git.ffmpeg.org/ffmpeg.git /root/.3rd/ffmpeg
fi
cd /root/.3rd/ffmpeg && cp /root/.3rd/eagleeye/eagleeye/3rd/ffmpeg/libavformat/* /root/.3rd/ffmpeg/libavformat && \
    ./configure --prefix=./linux-arm64-install \
    --cross-prefix=/opt/cross_build/linux-arm64/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu- \
    --arch=aarch64 --target-os=linux --enable-cross-compile \
    --enable-neon --enable-hwaccels --enable-gpl --disable-postproc --disable-debug --enable-small \
    --enable-static --enable-shared --disable-doc --enable-ffmpeg --disable-ffplay --disable-ffprobe \
    --disable-avdevice --disable-doc --enable-symver --pkg-config="pkg-config --static" \
    && make clean && make -j 10 && make install

# 4. rk mpp install
mkdir -p /root/.3rd/rk/
cd /root/.3rd/rk && git clone https://github.com/rockchip-linux/mpp.git
cd /root/.3rd/rk && git clone https://github.com/airockchip/librga.git
cd /root/.3rd/rk/mpp/build/linux/aarch64  && \
    sed -i "s/aarch64-linux-gnu-gcc/${arm_cross_build_root_path//\//\\/}\/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu\/bin\/aarch64-none-linux-gnu-gcc/g" arm.linux.cross.cmake && \
    sed -i "s/aarch64-linux-gnu-g++/${arm_cross_build_root_path//\//\\/}\/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu\/bin\/aarch64-none-linux-gnu-g++/g" arm.linux.cross.cmake && \
    bash make-Makefiles.bash && make -j 10

# 5. minio install TODO: