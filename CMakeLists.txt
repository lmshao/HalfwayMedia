cmake_minimum_required(VERSION 3.10)
project(HalfwayMedia)

set(CMAKE_CXX_STANDARD 17)
#include_directories(SYSTEM "/usr/local/include/")
#link_directories(SYSTEM "/usr/local/lib/")
link_libraries(avcodec avdevice avfilter avutil avformat swscale swresample pthread)
add_executable(HalfwayLive main.cpp
        MediaIn.cpp MediaIn.h
        MediaFileIn.cpp MediaFileIn.h
        MediaOut.cpp MediaOut.h
        MediaFileOut.cpp MediaFileOut.h
        LiveStreamOut.cpp LiveStreamOut.h
        Utils.cpp Utils.h
        MediaFramePipeline.cpp MediaFramePipeline.h
        VideoTranscoder.cpp VideoTranscoder.h
        AudioTranscoder.cpp AudioTranscoder.h
        VideoDecoder.cpp VideoDecoder.h
        AudioDecoder.cpp AudioDecoder.h
        AudioEncoder.cpp AudioEncoder.h
        VideoEncoder.cpp VideoEncoder.h
        LiveStreamIn.cpp LiveStreamIn.h
        JitterBuffer.cpp JitterBuffer.h
        RawFileOut.cpp RawFileOut.h)