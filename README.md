# Halfway Media Tool

基于FFmpeg (6.x)的流媒体处理工具，可以接收远程媒体流或者本地媒体文件，音视频编解码，发布流媒体。开发中。

系统要求： Linux / MacOS

## 依赖更新

Clone本仓库后，执行下面命令更新依赖仓库。

```sh
git submodule update --init --recursive
```

## 环境配置

### libfdk_aac

```sh
wget http://sourceforge.net/projects/opencore-amr/files/fdk-aac/fdk-aac-2.0.3.tar.gz/download -O fdk-aac-2.0.3.tar.gz
tar zxvf fdk-aac-2.0.3.tar.gz
cd fdk-aac-2.0.3
./configure --enable-shared --disable-static
make -j8 && make install
```

### libx264

```sh
wget https://code.videolan.org/videolan/x264/-/archive/master/x264-master.tar.bz2
tar jxvf x264-master.tar.bz2
cd x264-master
./configure --enable-shared
make -j && make install
```

Ubuntu

```sh
sudo apt install libx264-dev
```

### libx265

```sh
wget http://ftp.videolan.org/pub/videolan/x265/x265_3.2.tar.gz
tar zxvf x265_3.2.tar.gz
cd x265_3.2/build/linux
./make-Makefiles.bash
make -j && make install
```

Ubuntu

```sh
sudo apt install libx265-dev x265
```

### FFmpeg

Download FFmpeg version 6.x from [https://github.com/FFmpeg/FFmpeg](https://github.com/FFmpeg/FFmpeg)

```sh
./configure --enable-shared --disable-static --enable-gpl --enable-libx264  --enable-libx265 --enable-nonfree --enable-libfdk-aac
make -j && make install
```

## 功能期望

- 本地文件编解码
- 接收媒体流保存文件
- 读取本地文件发布流媒体
- 支持多种流媒体协议：RTSP/RTMP/
