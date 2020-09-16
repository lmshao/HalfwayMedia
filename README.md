# HalfwayLive

基于FFmpeg (4.0)的流媒体服务器。

系统要求： Linux / MacOS

### 环境配置

#### FFmpeg
```sh
$ ./configure --enable-shared --enable-gpl --enable-version3 --enable-nonfree --enable-postproc --enable-pthreads --enable-libfdk-aac --enable-libmp3lame  --enable-libx264 --enable-libxvid --enable-libvorbis --enable-libx265
or
$ ./configure --enable-shared
$ make
# make install
```