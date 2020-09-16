#include <stdio.h>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

int main()
{
    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *tag = NULL;
    int ret;
    char *file = "./assets/Sample.mkv";
    if ((ret = avformat_open_input(&fmt_ctx, file, NULL, NULL)))
        return ret;

    av_dump_format(fmt_ctx, 0, file, 0);
    avformat_close_input(&fmt_ctx);
    return 0;
}