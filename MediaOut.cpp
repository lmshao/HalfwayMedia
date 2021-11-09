//
// Copyright 2020 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "MediaOut.h"

#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

#include "Utils.h"
#include "rtp/RtpHeader.h"

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

MediaFrame::MediaFrame(const Frame &frame, int64_t timestamp) : _timestamp(timestamp), _duration(0)
{
    _frame = frame;
    if (frame.length > 0) {
        uint8_t *payload = frame.payload;
        uint32_t length = frame.length;

        // unpack audio rtp pack
        if (isAudioFrame(frame) && frame.additionalInfo.audio.isRtpPacket) {
            RTPHeader *rtp = reinterpret_cast<RTPHeader *>(payload);
            uint32_t headerLength = rtp->getHeaderLength();
            assert(length >= headerLength);
            payload += headerLength;
            length -= headerLength;
            _frame.additionalInfo.audio.isRtpPacket = false;
            _frame.length = length;
        }

        _frame.payload = (uint8_t *)malloc(length);
        memcpy(_frame.payload, payload, length);
    } else {
        _frame.payload = nullptr;
    }
}

MediaFrame::~MediaFrame()
{
    if (_frame.payload) {
        free(_frame.payload);
        _frame.payload = nullptr;
    }
}

MediaFrameQueue::MediaFrameQueue() : _valid(true), _startTimeOffset(currentTimeMs()) {}

void MediaFrameQueue::pushFrame(const Frame &frame)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (!_valid) return;

    // Cache the last frame
    std::shared_ptr<MediaFrame> lastFrame;
    std::shared_ptr<MediaFrame> mediaFrame(new MediaFrame(frame, currentTimeMs() - _startTimeOffset));
    if (isAudioFrame(frame)) {
        if (!_lastAudioFrame) {
            _lastAudioFrame = mediaFrame;
            return;
        }

        // reset frame timestamp
        _lastAudioFrame->_duration = mediaFrame->_timestamp - _lastAudioFrame->_timestamp;
        if (_lastAudioFrame->_duration <= 0) {
            _lastAudioFrame->_duration = 1;
            mediaFrame->_timestamp = _lastAudioFrame->_timestamp + 1;
        }

        lastFrame = _lastAudioFrame;
        _lastAudioFrame = mediaFrame;
    } else {
        if (!_lastVideoFrame) {
            _lastVideoFrame = mediaFrame;
            return;
        }

        _lastVideoFrame->_duration = mediaFrame->_timestamp - _lastVideoFrame->_timestamp;
        if (_lastVideoFrame->_duration <= 0) {
            _lastVideoFrame->_duration = 1;
            mediaFrame->_timestamp = _lastVideoFrame->_timestamp + 1;
        }

        lastFrame = _lastVideoFrame;
        _lastVideoFrame = mediaFrame;
    }
    logger("_queue.push");
    _queue.push(lastFrame);
    if (_queue.size() == 1) {
        lock.unlock();
        _cond.notify_one();
    }
}

std::shared_ptr<MediaFrame> MediaFrameQueue::popFrame(int timeout)
{
    std::unique_lock<std::mutex> lock(_mutex);
    std::shared_ptr<MediaFrame> mediaFrame;

    if (!_valid) {
        logger("_valid is false");
        return nullptr;
    }

    if (_queue.empty() && timeout > 0) {
        _cond.wait_until(lock, std::chrono::system_clock::now() + std::chrono::milliseconds(timeout));
    }

    if (!_queue.empty()) {
        mediaFrame = _queue.front();
        _queue.pop();
    }

    logger("popFrame %d, left num : %d", mediaFrame ? 1 : 0, _queue.size());
    return mediaFrame;
}

void MediaFrameQueue::cancel()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _valid = false;
    _cond.notify_all();
}

MediaOut::MediaOut(const std::string &url, bool hasAudio, bool hasVideo)
    : _status(Context_EMPTY),
      _url(url),
      _hasAudio(hasAudio),
      _hasVideo(hasVideo),
      _audioFormat(FRAME_FORMAT_UNKNOWN),
      _sampleRate(0),
      _channels(0),
      _videoFormat(FRAME_FORMAT_UNKNOWN),
      _width(0),
      _height(0),
      _videoSourceChanged(true),
      _avFmtCtx(nullptr),
      _audioStream(nullptr),
      _videoStream(nullptr),
      _outFile(nullptr)
{
    logger("url %s, audio %d, video %d", _url.c_str(), _hasAudio, _hasVideo);
    if (!_hasAudio && !_hasVideo) {
        logger("Audio/Video not enabled");
        return;
    }
    _status = Context_INITIALIZING;
    _thread = std::thread(&MediaOut::sendLoop, this);
}

MediaOut::~MediaOut()
{
    if (_packet) {
        av_packet_free(&_packet);
    }
};

void MediaOut::onFrame(const Frame &frame)
{
    logger("MediaOut onFrame %s", getFormatStr(frame.format));
    if (isAudioFrame(frame)) {
        logger("recv one audio frame");
        if (!_hasAudio) {
            logger("Audio is not enabled");
            return;
        }

        if (!isAudioFormatSupported(frame.format)) {
            logger("Unsupported audio frame format: %s(%d)", getFormatStr(frame.format), frame.format);
            return;
        }

        // fill audio info
        if (_audioFormat == FRAME_FORMAT_UNKNOWN) {
            logger("fill audio info");
            _sampleRate = frame.additionalInfo.audio.sampleRate;
            _channels = frame.additionalInfo.audio.channels;
            _audioFormat = frame.format;
        }

        if (_audioFormat != frame.format) {
            logger("Expected codec(%s), got(%s)", getFormatStr(_audioFormat), getFormatStr(frame.format));
            return;
        }

        // TODO:check status

        if (_channels != frame.additionalInfo.audio.channels || _sampleRate != frame.additionalInfo.audio.sampleRate) {
            logger("Invalid audio frame channels %d, or sample rate: %d", frame.additionalInfo.audio.channels,
                   frame.additionalInfo.audio.sampleRate);
            return;
        }

        logger("pushFrame audio %s", dumpHex(frame.payload, 10));

        _frameQueue.pushFrame(frame);
    } else if (isVideoFrame(frame)) {
        logger("recv one video frame");
        if (!_hasVideo) {
            logger("Video is not enabled");
            return;
        }

        if (!isVideoFormatSupported(frame.format)) {
            logger("Unsupported video frame format: %s(%d)", getFormatStr(frame.format), frame.format);
            return;
        }

        // recv video frame for the first time
        if (_videoFormat == FRAME_FORMAT_UNKNOWN) {
            if (!frame.additionalInfo.video.isKeyFrame) {
                logger("Request video key frame for initialization");
                return;
            }

            logger("Initial video options: format(%s), %dx%d", getFormatStr(frame.format),
                   frame.additionalInfo.video.width, frame.additionalInfo.video.height);

            _videoSourceChanged = false;
            _videoKeyFrame.reset(new MediaFrame(frame));

            _width = frame.additionalInfo.video.width;
            _height = frame.additionalInfo.video.height;
            _videoFormat = frame.format;
        }

        if (_videoFormat != frame.format) {
            logger("Expected codec(%s), got(%s)", getFormatStr(_videoFormat), getFormatStr(frame.format));
            return;
        }

        // video resolution has changed
        if (!_videoSourceChanged &&
            (_width != frame.additionalInfo.video.width || _height != frame.additionalInfo.video.height)) {
            logger("Video resolution changed %dx%d -> %dx%d", _width, _height, frame.additionalInfo.video.width,
                   frame.additionalInfo.video.height);
            _videoSourceChanged = true;
        }

        if (_videoSourceChanged) {
            if (!frame.additionalInfo.video.isKeyFrame) {
                logger("Request video key frame for video changed");
                return;
            }

            logger("Ready after video changed: format(%s), %dx%d", getFormatStr(frame.format),
                   frame.additionalInfo.video.width, frame.additionalInfo.video.height);

            _videoSourceChanged = false;
            _videoKeyFrame.reset(new MediaFrame(frame));

            _width = frame.additionalInfo.video.width;
            _height = frame.additionalInfo.video.height;
            _videoFormat = frame.format;
        }

        logger("pushFrame video %s", dumpHex(frame.payload, 10));

        _frameQueue.pushFrame(frame);
    } else {
        logger("Unsupported frame format: %s(%d)", getFormatStr(frame.format), frame.format);
    }
}

void MediaOut::close()
{
    //    printf("Close %s", _url.c_str());
    _status = Context_CLOSED;
    _frameQueue.cancel();
    _thread.join();
}

AVCodecID MediaOut::frameFormat2AVCodecID(int frameFormat)
{
    switch (frameFormat) {
        case FRAME_FORMAT_VP8:
            return AV_CODEC_ID_VP8;
        case FRAME_FORMAT_VP9:
            return AV_CODEC_ID_VP9;
        case FRAME_FORMAT_H264:
            return AV_CODEC_ID_H264;
        case FRAME_FORMAT_H265:
            return AV_CODEC_ID_H265;
        case FRAME_FORMAT_PCMU:
            return AV_CODEC_ID_PCM_MULAW;
        case FRAME_FORMAT_PCMA:
            return AV_CODEC_ID_PCM_ALAW;
        case FRAME_FORMAT_OPUS:
            return AV_CODEC_ID_OPUS;
        case FRAME_FORMAT_AAC:
        case FRAME_FORMAT_AAC_48000_2:
            return AV_CODEC_ID_AAC;
        default:
            return AV_CODEC_ID_NONE;
    }
}
bool MediaOut::connect()
{
    logger("open enter");
    const char *formatName = getFormatName(_url);  // "matroska" for mkv, "mp4" for mp4
    logger("%s", formatName);
    avformat_alloc_output_context2(&_avFmtCtx, nullptr, formatName, _url.c_str());
    if (!_avFmtCtx) {
        logger("Cannot allocate output context, format(%s), url(%s)", "", _url.c_str());
        return false;
    }

    if (!(_avFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&_avFmtCtx->pb, _url.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            logger("Cannot open avio, %s", ff_err2str(ret));
            avformat_free_context(_avFmtCtx);
            _avFmtCtx = nullptr;
            return false;
        }
    }

    logger("open %s", _url.c_str());
    return true;
}

bool MediaOut::addAudioStream(FrameFormat format, uint32_t sampleRate, uint32_t channels)
{
    logger("%s %ld %ld", getFormatStr(format), sampleRate, channels);
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
        case AV_CODEC_ID_AAC:  // AudioSpecificConfig 48000-2
            par->extradata_size = 2;
            par->extradata = (uint8_t *)av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            par->extradata[0] = 0x11;
            par->extradata[1] = 0x90;
            break;
        case AV_CODEC_ID_OPUS:  // OpusHead 48000-2
            par->extradata_size = 19;
            par->extradata = (uint8_t *)av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            par->extradata[0] = 'O';
            par->extradata[1] = 'p';
            par->extradata[2] = 'u';
            par->extradata[3] = 's';
            par->extradata[4] = 'H';
            par->extradata[5] = 'e';
            par->extradata[6] = 'a';
            par->extradata[7] = 'd';
            // Version
            par->extradata[8] = 1;
            // Channel Count
            par->extradata[9] = 2;
            // Pre-skip
            par->extradata[10] = 0x38;
            par->extradata[11] = 0x1;
            // Input Sample Rate (Hz)
            par->extradata[12] = 0x80;
            par->extradata[13] = 0xbb;
            par->extradata[14] = 0;
            par->extradata[15] = 0;
            // Output Gain (Q7.8 in dB)
            par->extradata[16] = 0;
            par->extradata[17] = 0;
            // Mapping Family
            par->extradata[18] = 0;
            break;
        default:
            break;
    }

    logger("dump audio");
    av_dump_format(_avFmtCtx, 0, _url.c_str(), 1);
    return true;
}

bool MediaOut::addVideoStream(FrameFormat format, uint32_t width, uint32_t height)
{
    logger("%s %ld %ld", getFormatStr(format), width, height);
    enum AVCodecID codecId = frameFormat2AVCodecID(format);
    _videoStream = avformat_new_stream(_avFmtCtx, nullptr);
    if (!_videoStream) {
        logger("Cannot add video stream");
        return false;
    }

    AVCodecParameters *par = _videoStream->codecpar;
    par->codec_type = AVMEDIA_TYPE_VIDEO;
    par->codec_id = codecId;
    par->width = width;
    par->height = height;
    if (codecId == AV_CODEC_ID_H264 || codecId == AV_CODEC_ID_H265) {  // extradata
        AVCodecParserContext *parser = av_parser_init(codecId);
        if (!parser) {
            logger("Cannot find video parser");
            return false;
        }

        logger("keyframe %p %d", _videoKeyFrame->_frame.payload, _videoKeyFrame->_frame.length);
        logger("dump _avPacket %s", dumpHex(_videoKeyFrame->_frame.payload, 10));

        int size = parser->parser->split(nullptr, _videoKeyFrame->_frame.payload, _videoKeyFrame->_frame.length);
        if (size > 0) {
            par->extradata_size = size;
            par->extradata = (uint8_t *)av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            memcpy(par->extradata, _videoKeyFrame->_frame.payload, par->extradata_size);
        } else {
            logger("Cannot find video extradata");
        }

        av_parser_close(parser);
    }

    if (codecId == AV_CODEC_ID_H265) {
        par->codec_tag = 0x31637668;  // hvc1
    }

    logger("dump video");
    av_dump_format(_avFmtCtx, 0, _url.c_str(), 1);
    return true;
}

bool MediaOut::writeHeader()
{
    int ret;
    AVDictionary *options = nullptr;

    ret = getHeaderOpt(_url, &options);
    if (!ret) {
        logger("Cannot get header options");
        return false;
    }

    ret = avformat_write_header(_avFmtCtx, options ? &options : nullptr);
    if (ret < 0) {
        //        logger("Cannot write %s header, %s", _avFmtCtx->url, ff_err2str(ret));
        return false;
    }
    return true;
}

bool MediaOut::writeFrame(AVStream *stream, const std::shared_ptr<MediaFrame> &mediaFrame)
{
    int ret;
    if (!_packet) {
        _packet = av_packet_alloc();
    }

    if (mediaFrame == nullptr) {
        return false;
    }

    if (stream == nullptr) {
        return false;
    }

    av_packet_unref(_packet);
    _packet->data = mediaFrame->_frame.payload;
    _packet->size = mediaFrame->_frame.length;
    _packet->dts = (int64_t)(mediaFrame->_timestamp / (av_q2d(stream->time_base) * 1000));
    _packet->pts = _packet->dts;
    _packet->duration = (int64_t)(mediaFrame->_duration / (av_q2d(stream->time_base) * 1000));
    _packet->stream_index = stream->index;

    if (isVideoFrame(mediaFrame->_frame)) {
        if (mediaFrame->_frame.additionalInfo.video.isKeyFrame) {
            _packet->flags |= AV_PKT_FLAG_KEY;
            // TODO:
        }
    }

    logger("Send frame");

    ret = av_interleaved_write_frame(_avFmtCtx, _packet);
    if (ret < 0) {
        logger("Cannot write frame, %s", ff_err2str(ret));
        return false;
    }
    return true;
}

void MediaOut::sendLoop()
{
    const uint32_t waitMs = 20, totalWaitMs = 10 * 1000;
    uint32_t timeout = 0;
    int ret2;
    int count = 0;

    while ((_hasAudio && _audioFormat == FRAME_FORMAT_UNKNOWN) || (_hasVideo && _videoFormat == FRAME_FORMAT_UNKNOWN)) {
        if (_status == Context_CLOSED) {
            goto exit;
        }

        if (timeout >= totalWaitMs) {
            logger("fatal: No a/v frames, timeout %ld ms", timeout);
            goto exit;
        }

        timeout += waitMs;
        usleep(waitMs * 1000);
        logger("Wait for av options avaible, timeout %ld ms", timeout);
    }

    if (!connect()) {
        logger("init Cannot open connection");
        goto exit;
    }

    if (_hasAudio && !addAudioStream(_audioFormat, _sampleRate, _channels)) {
        logger("Cannot add audio stream");
        goto exit;
    }

    if (_hasVideo && !addVideoStream(_videoFormat, _width, _height)) {
        logger("Cannot add video stream");
        goto exit;
    }

    if (!writeHeader()) {
        logger("Cannot write header");
        goto exit;
    }

    logger("Dump out file");
    av_dump_format(_avFmtCtx, 0, _avFmtCtx->url, 1);

    _status = Context_READY;
    while (_status == Context_READY) {
        logger("-----------------------------------");
        std::shared_ptr<MediaFrame> mediaFrame = _frameQueue.popFrame(2000);
        if (!mediaFrame) {
            if (_status == Context_READY) {
                logger("No input frames available");
            }
            logger("---------");
            break;
        }

        logger("writeFrame [%d]", count++);
        DUMP_HEX(mediaFrame->_frame.payload, 10);
        bool ret = writeFrame(isVideoFrame(mediaFrame->_frame) ? _videoStream : _audioStream, mediaFrame);
        if (!ret) {
            logger("Cannot write frame");
            break;
        }
    }

    ret2 = av_write_trailer(_avFmtCtx);
    logger("av_write_trailer %s", ff_err2str(ret2));

exit:
    logger("---------------------------------------------------------------");
    _status = Context_CLOSED;
    logger("Thread exited!");
}
void MediaOut::waitThread()
{
    _thread.join();
}
