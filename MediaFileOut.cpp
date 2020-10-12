//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaFileOut.h"
#include <unistd.h>
#include "Utils.h"

//extern "C" {
//#include <libavformat/avformat.h>
//}

MediaFileOut::MediaFileOut(const std::string &url, bool hasAudio, bool hasVideo)
  : MediaOut(url, hasAudio, hasVideo)
{
}

MediaFileOut::~MediaFileOut()
{
    logger("~");
};

void MediaFileOut::sendLoop()
{
    const uint32_t waitMs = 20, totalWaitMs = 10*1000;
    uint32_t timeout = 0;
    while ((_hasAudio && _audioFormat == FRAME_FORMAT_UNKNOWN) ||
           (_hasAudio && _videoFormat == FRAME_FORMAT_UNKNOWN)) {
        if (_status == Context_CLOSED) {
            goto exit;
        }

        if (timeout >= totalWaitMs){
            logger("fatal: No a/v frames, timeout %ld ms", timeout);
            goto exit;
        }

        timeout += waitMs;
        usleep(waitMs * 1000);
        logger("Wait for av options avaible, timeout %ld ms", timeout);
    }

    if (!open()){
        logger("init Cannot open connection");
        goto exit;
    }

    if (_hasAudio && !addAudioStream(_audioFormat, _sampleRate, _channels)) {
        logger("Cannot add audio stream");
        goto exit;
    }

    if (_hasVideo && !addVideoStream(_videoFormat, _width, _height)) {
        logger("");
        goto exit;
    }

    if (!writeHeader()) {
        logger("Cannot write header");
        goto exit;
    }

    av_dump_format(_avFmtCtx, 0, _avFmtCtx->url, 1);

    _status = Context_READY;

    while (_status == Context_READY) {
        std::shared_ptr<MediaFrame> mediaFrame = _frameQueue.popFrame(2000);
        if (!mediaFrame) {
            if (_status == Context_READY) {
                logger("No input frames available");
            }
            break;
        }

        bool ret = writeFrame(isVideoFrame(mediaFrame->_frame) ? _videoStream : _audioStream, mediaFrame);
        if (!ret) {
            logger("Cannot write frame");
            break;
        }
    }

    av_write_trailer(_avFmtCtx);

exit:
    _status = Context_CLOSED;
    logger("Thread exited!");
}

bool MediaFileOut::isAudioFormatSupported(FrameFormat format)
{
    switch (format) {
        case FRAME_FORMAT_PCMU:
        case FRAME_FORMAT_PCMA:
        case FRAME_FORMAT_AAC:
        case FRAME_FORMAT_AAC_48000_2:
        case FRAME_FORMAT_OPUS:
            return true;
        default:
            return false;
    }
}

bool MediaFileOut::isVideoFormatSupported(FrameFormat format)
{
    switch (format) {
        case FRAME_FORMAT_VP8:
        case FRAME_FORMAT_VP9:
        case FRAME_FORMAT_H264:
        case FRAME_FORMAT_H265:
            return true;
        default:
            return false;
    }
}

const char *MediaFileOut::getFormatName(std::string &url)
{
    return nullptr;
}

uint32_t MediaFileOut::getKeyFrameInterval(void)
{
    return 0;
}

uint32_t MediaFileOut::getReconnectCount(void)
{
    return 0;
}

bool MediaFileOut::open() {
    const char *formatName = "mp4"; // "matroska" for mkv, "mp4" for mp4
    avformat_alloc_output_context2(&_avFmtCtx, nullptr, formatName, _filename.c_str());
    if (!_avFmtCtx){
        logger("Cannot allocate output context, format(%s), url(%s)", "", _filename.c_str());
        return false;
    }

    if (!(_avFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&_avFmtCtx->pb, _avFmtCtx->url, AVIO_FLAG_WRITE);
        if (ret < 0){
            logger("Cannot open avio, %s", ff_err2str(ret));

            avformat_free_context(_avFmtCtx);
            _avFmtCtx = nullptr;
            return false;
        }
    }

    return true;
}

bool MediaFileOut::addAudioStream(FrameFormat format, uint32_t sampleRate, uint32_t channels) {
     AVCodecID codecId = frameFormat2AVCodecID(format);
    _audioStream = avformat_new_stream(_avFmtCtx, nullptr);
    if (!_audioStream) {
        logger("Cannot add audio stream");
        return false;
    }

    AVCodecParameters *par = _audioStream->codecpar;
    par->codec_type = AVMEDIA_TYPE_AUDIO;
    par->codec_id = codecId;
    par->sample_rate = sampleRate;
    par->channels = channels;
    par->channel_layout = av_get_default_channel_layout(par->channels);
    switch (par->codec_id) {
        case AV_CODEC_ID_AAC: // AudioSpecificConfig 48000-2
            par->extradata_size = 2;
            par->extradata = (uint8_t*) av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            par->extradata[0] = 0x11;
            par->extradata[1] = 0x90;
            break;
        case AV_CODEC_ID_OPUS:  //OpusHead 48000-2
            par->extradata_size = 19;
            par->extradata      = (uint8_t *)av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            par->extradata[0]   = 'O';
            par->extradata[1]   = 'p';
            par->extradata[2]   = 'u';
            par->extradata[3]   = 's';
            par->extradata[4]   = 'H';
            par->extradata[5]   = 'e';
            par->extradata[6]   = 'a';
            par->extradata[7]   = 'd';
            //Version
            par->extradata[8]   = 1;
            //Channel Count
            par->extradata[9]   = 2;
            //Pre-skip
            par->extradata[10]  = 0x38;
            par->extradata[11]  = 0x1;
            //Input Sample Rate (Hz)
            par->extradata[12]  = 0x80;
            par->extradata[13]  = 0xbb;
            par->extradata[14]  = 0;
            par->extradata[15]  = 0;
            //Output Gain (Q7.8 in dB)
            par->extradata[16]  = 0;
            par->extradata[17]  = 0;
            //Mapping Family
            par->extradata[18]  = 0;
            break;
        default:
            break;
    }

    return true;
}

bool MediaFileOut::addVideoStream(FrameFormat format, uint32_t width, uint32_t height) {
    enum AVCodecID codecId = frameFormat2AVCodecID(format);
    _videoStream = avformat_new_stream(_avFmtCtx, NULL);
    if (!_videoStream) {
        logger("Cannot add video stream");
        return false;
    }

    AVCodecParameters *par = _videoStream->codecpar;
    par->codec_type = AVMEDIA_TYPE_VIDEO;
    par->codec_id   = codecId;
    par->width = width;
    par->height = height;
    if (codecId == AV_CODEC_ID_H264 || codecId == AV_CODEC_ID_H265) { //extradata
        AVCodecParserContext *parser = av_parser_init(codecId);
        if (!parser) {
            logger("Cannot find video parser");
            return false;
        }

        int size = parser->parser->split(nullptr, _videoKeyFrame->_frame.payload, _videoKeyFrame->_frame.length);
        if (size > 0) {
            par->extradata_size = size;
            par->extradata      = (uint8_t *)av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            memcpy(par->extradata, _videoKeyFrame->_frame.payload, par->extradata_size);
        } else {
            logger("Cannot find video extradata");
        }

        av_parser_close(parser);
    }

    if (codecId == AV_CODEC_ID_H265) {
        par->codec_tag = 0x31637668; //hvc1
    }

    return true;
}

bool MediaFileOut::writeHeader() {
    int ret;
    AVDictionary *options = nullptr;

    ret = avformat_write_header(_avFmtCtx, options ? &options: nullptr);
    if (ret < 0){
        logger("Cannot write header, %s", ff_err2str(ret));
        return false;
    }
    return false;
}

bool MediaFileOut::writeFrame(AVStream *stream, std::shared_ptr<MediaFrame> mediaFrame) {
    int ret;
    AVPacket pkt;
    if (stream == nullptr || mediaFrame == nullptr) {
        return false;
    }

    av_init_packet(&pkt);
    pkt.data = mediaFrame->_frame.payload;
    pkt.size = mediaFrame->_frame.length;
    pkt.dts = (int64_t)(mediaFrame->_timestamp / (av_q2d(stream->time_base) * 1000));
    pkt.pts = pkt.dts;
    pkt.duration = (int64_t)(mediaFrame->_duration /(av_q2d(stream->time_base) * 1000));
    pkt.stream_index = stream->index;

    if (isVideoFrame(mediaFrame->_frame)){
        if (mediaFrame->_frame.additionalInfo.video.isKeyFrame) {
            pkt.flags |= AV_PKT_FLAG_KEY;
            // TODO:
        }
    }

    logger("Send frame");

    ret = av_interleaved_write_frame(_avFmtCtx, &pkt);
    if (ret < 0) {
        logger("Cannot write frame, %s", ff_err2str(ret));
        return false;
    }
    return true;
}
