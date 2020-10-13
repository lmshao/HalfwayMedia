# HalfwayLive

基于FFmpeg (4.0)的流媒体服务器。

系统要求： Linux / MacOS

## 环境配置

### FFmpeg
```sh
$ ./configure --enable-shared --enable-gpl --enable-version3 --enable-nonfree --enable-postproc --enable-pthreads --enable-libfdk-aac --enable-libmp3lame  --enable-libx264 --enable-libxvid --enable-libvorbis --enable-libx265
or
$ ./configure --enable-shared
$ make
# make install
```

## 功能期望
|优先级|输入|输出|完成度|
|:---:|:---:|:---:|:---:|
1|文件|文件|×
2|文件|RTMP|×
3|RTMP|文件|×
4|文件|RTP|×
...|...|...|...

```
MediaFileIn  --+                                                                      +-- MediaFileOut
               +-- MediaIn --  FrameSource -- Queue -- FrameDestination -- MediaOut --+
LiveStreamIn --+                                                                      +-- LiveStreamOut
```
