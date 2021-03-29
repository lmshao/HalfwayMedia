# Halfway Media Tool

基于FFmpeg (4.0)的流媒体处理工具，可以接收远程媒体流或者本地媒体文件，音视频编解码，发布流媒体。开发中。

系统要求： Linux / MacOS

## 环境配置

### libfdk_aac

```sh
$ wget http://sourceforge.net/projects/opencore-amr/files/fdk-aac/fdk-aac-2.0.1.tar.gz/download
$ ./configure --enable-shared --disable-static
$ make install
```

### FFmpeg
```sh
# Download FFmpeg version 4.0 from https://github.com/FFmpeg/FFmpeg
$ ./configure --enable-shared --disable-static --enable-gpl --enable-libx264  --enable-nonfree --enable-libfdk-aac
$ make install
or
sudo apt install libavcodec-dev libavdevice-dev libavfilter-dev libavformat-dev libavutil-dev libavresample-dev libswresample-dev libswscale-dev
```

## 功能期望
- 本地文件编解码
- 接收媒体流保存文件
- 读取本地文件发布流媒体
- 支持多种流媒体协议：RTSP/RTMP/
