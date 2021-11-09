//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#include "LiveStreamIn.h"

extern "C" {
#include <libavutil/intreadwrite.h>
}

inline int findNALU(uint8_t *buf, int size, int *nal_start, int *nal_end, int *sc_len)
{
    int i = 0;
    *nal_start = 0;
    *nal_end = 0;
    *sc_len = 0;

    while (true) {
        if (size < i + 3) return -1; /* Did not find NAL start */

        /* ( next_bits( 24 ) == {0, 0, 1} ) */
        if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1) {
            i += 3;
            *sc_len = 3;
            break;
        }

        /* ( next_bits( 32 ) == {0, 0, 0, 1} ) */
        if (size > i + 3 && buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 0 && buf[i + 3] == 1) {
            i += 4;
            *sc_len = 4;
            break;
        }

        ++i;
    }

    // assert(buf[i - 1] == 1);
    *nal_start = i;

    /*( next_bits( 24 ) != {0, 0, 1} )*/
    while ((size > i + 2) && (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 1)) ++i;

    if (size <= i + 2)
        *nal_end = size;
    else if (buf[i - 1] == 0)
        *nal_end = i - 1;
    else
        *nal_end = i;

    return (*nal_end - *nal_start);
}

static int filterNALs(uint8_t *data, int size, const std::vector<int> &removeTypes, const std::vector<int> &passTypes)
{
    int remove_nals_size = 0;

    int naluFoundLength = 0;
    uint8_t *bufferStart = data;
    int bufferLength = size;
    int naluStartOffset = 0;
    int naluEndOffset = 0;
    int scLen = 0;
    int naluType;

    if (!removeTypes.empty() && !passTypes.empty()) return -1;

    while (bufferLength > 0) {
        naluFoundLength = findNALU(bufferStart, bufferLength, &naluStartOffset, &naluEndOffset, &scLen);
        if (naluFoundLength < 0) {
            /* Error, should never happen */
            break;
        }

        naluType = bufferStart[naluStartOffset] & 0x1F;
        // drop removeTypes-type nalu
        if ((removeTypes.size() > 0 && find(removeTypes.begin(), removeTypes.end(), naluType) != removeTypes.end()) ||
            (passTypes.size() > 0 && find(passTypes.begin(), passTypes.end(), naluType) == passTypes.end())) {
            std::unique_ptr<uint8_t> bufferDelegate(bufferStart);
            memmove(bufferDelegate.get(), bufferDelegate.get() + naluStartOffset + naluFoundLength,
                    bufferLength - naluStartOffset - naluFoundLength);
            bufferDelegate.release();
            bufferLength -= naluStartOffset + naluFoundLength;

            remove_nals_size += naluStartOffset + naluFoundLength;
            continue;
        }

        bufferStart += (naluStartOffset + naluFoundLength);
        bufferLength -= (naluStartOffset + naluFoundLength);
    }
    return size - remove_nals_size;
}

//-----------------------------------

LiveStreamIn::LiveStreamIn(const std::string &url) : MediaIn(url)
{
    logger("%s", __FUNCTION__);
    if (isRtsp()) {
        // for RTP over UDP
        uint32_t buffSize = DEFAULT_UDP_BUF_SIZE;
        char buf[256];
        snprintf(buf, sizeof(buf), "%u", buffSize);
        av_dict_set(&_options, "buffer_size", buf, 0);
        av_dict_set(&_options, "rtsp_transport", "udp", 0);
        logger("rtsp, transport: udp(%u)", buffSize);

        // for RTP over UDP
        //        av_dict_set(&_options, "rtsp_transport", "tcp", 0);
    }

    if (isFileInput()) {
        _isFileInput = true;
    }

    _enableVideo = "auto";
    _enableAudio = "auto";
    _runing = true;
}

LiveStreamIn::~LiveStreamIn()
{
    logger("Closing %s", _url.c_str());
    _runing = false;
    if (_timeoutHandler) _timeoutHandler->stop();
    //    _thread.join();

    if (_videoJitterBuffer) {
        _videoJitterBuffer->stop();
        _videoJitterBuffer.reset();
    }

    if (_audioJitterBuffer) {
        _audioJitterBuffer->stop();
        _audioJitterBuffer.reset();
    }

    _needCheckVBS = true;
    _needApplyVBSF = false;
    if (_vbsf) {
        av_bsf_free(&_vbsf);
        _vbsf = nullptr;
    }

    _enableVideoExtradata = false;
    if (_spsPpsBuffer) {
        _spsPpsBuffer.reset();
        _spsPpsBufferLength = 0;
    }

    if (_avFmtCtx) {
        avformat_close_input(&_avFmtCtx);
        avformat_free_context(_avFmtCtx);
        _avFmtCtx = nullptr;
    }

    av_dict_free(&_options);
    if (_timeoutHandler) {
        delete _timeoutHandler;
        _timeoutHandler = nullptr;
    }

    logger("Closed");
}

bool LiveStreamIn::open()
{
    return false;
}

void LiveStreamIn::onDeliverFrame(JitterBuffer *jitterBuffer, AVPacket *pkt)
{
    if (_videoJitterBuffer.get() == jitterBuffer) {
        deliverVideoFrame(pkt);
    } else if (_audioJitterBuffer.get() == jitterBuffer) {
        deliverAudioFrame(pkt);
    } else {
        logger("Invalid JitterBuffer onDeliver event!");
    }
}

void LiveStreamIn::onSyncTimeChanged(JitterBuffer *jitterBuffer, int64_t syncTimestamp)
{
    if (_audioJitterBuffer.get() == jitterBuffer) {
        logger("onSyncTimeChanged audio, timestamp %ld ", syncTimestamp);

        // rtsp audio/video time base is different, it will lost sync after roll back
        if (!isRtsp()) {
            boost::posix_time::ptime mst = boost::posix_time::microsec_clock::local_time();

            _audioJitterBuffer->setSyncTime(syncTimestamp, mst);
            if (_videoJitterBuffer) _videoJitterBuffer->setSyncTime(syncTimestamp, mst);
        }
    } else if (_videoJitterBuffer.get() == jitterBuffer) {
        logger("onSyncTimeChanged video, timestamp %ld ", syncTimestamp);
    } else {
        logger("Invalid JitterBuffer onSyncTimeChanged event!");
    }
}

void LiveStreamIn::deliverNullVideoFrame()
{
    uint8_t dumyData = 0;
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = _videoFormat;
    frame.payload = &dumyData;
    deliverFrame(frame);

    logger("deliver null video frame");
}

void LiveStreamIn::deliverVideoFrame(AVPacket *pkt)
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = _videoFormat;
    frame.payload = reinterpret_cast<uint8_t *>(pkt->data);
    frame.length = pkt->size;
    frame.timeStamp = av_rescale_q(pkt->dts, _msTimeBase, _videoTimeBase);
    frame.additionalInfo.video.width = _videoWidth;
    frame.additionalInfo.video.height = _videoHeight;
    frame.additionalInfo.video.isKeyFrame = (pkt->flags & AV_PKT_FLAG_KEY);
    deliverFrame(frame);
    logger("deliver video frame, timestamp %ld(%ld), size %4d, %s",
           av_rescale_q(frame.timeStamp, _videoTimeBase, _msTimeBase), pkt->dts, frame.length,
           (pkt->flags & AV_PKT_FLAG_KEY) ? "key" : "non-key");
}

void LiveStreamIn::deliverAudioFrame(AVPacket *pkt)
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = _audioFormat;
    frame.payload = reinterpret_cast<uint8_t *>(pkt->data);
    frame.length = pkt->size;
    frame.timeStamp = av_rescale_q(pkt->dts, _msTimeBase, _audioTimeBase);
    frame.additionalInfo.audio.isRtpPacket = 0;
    frame.additionalInfo.audio.sampleRate = _audioSampleRate;
    frame.additionalInfo.audio.channels = _audioChannels;
    frame.additionalInfo.audio.nbSamples = frame.length / frame.additionalInfo.audio.channels / 2;
    deliverFrame(frame);

    logger("deliver audio frame, timestamp %ld(%ld), size %4d",
           av_rescale_q(frame.timeStamp, _audioTimeBase, _msTimeBase), pkt->dts, frame.length);
}

void LiveStreamIn::start()
{
    _timeoutHandler = new TimeoutHandler();
    _thread = std::thread(&LiveStreamIn::receiveLoop, this);
}

bool LiveStreamIn::reconnect()
{
    int res;
    logger("Read input data failed, trying to reopen input from url %s", _url.c_str());

    if (_audioJitterBuffer) {
        _audioJitterBuffer->drain();
        _audioJitterBuffer->stop();
    }

    if (_videoJitterBuffer) {
        _videoJitterBuffer->drain();
        _videoJitterBuffer->stop();
    }

    _needCheckVBS = true;
    _needApplyVBSF = false;
    if (_vbsf) {
        av_bsf_free(&_vbsf);
        _vbsf = nullptr;
    }

    _enableVideoExtradata = false;
    if (_spsPpsBuffer) {
        _spsPpsBuffer.reset();
        _spsPpsBufferLength = 0;
    }

    if (_avFmtCtx) {
        avformat_close_input(&_avFmtCtx);
        avformat_free_context(_avFmtCtx);
        _avFmtCtx = nullptr;
    }

    //    _timeoutHandler->reset(10000);
    _avFmtCtx = avformat_alloc_context();
    //    _avFmtCtx->interrupt_callback = {&TimeoutHandler::checkInterrupt, _timeoutHandler};

    logger("Opening input");
    //    _timeoutHandler->reset(60000);
    res = avformat_open_input(&_avFmtCtx, _url.c_str(), nullptr, &_options);
    if (res != 0) {
        logger("Error opening input %s", ff_err2str(res));
        return false;
    }

    logger("Finding stream info");
    //    _timeoutHandler->reset(10000);
    _avFmtCtx->fps_probe_size = 0;
    _avFmtCtx->max_ts_probe = 0;
    res = avformat_find_stream_info(_avFmtCtx, nullptr);
    if (res < 0) {
        logger("Error find stream info %s", ff_err2str(res));
        return false;
    }

    logger("Dump format");
    av_dump_format(_avFmtCtx, 0, _url.c_str(), 0);

    if (_videoStreamIndex != -1) {
        int streamNo = av_find_best_stream(_avFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (streamNo < 0) {
            logger("No Video stream found");
            return false;
        }

        if (_videoStreamIndex != streamNo) {
            logger("Video stream index changed, %d -> %d", _videoStreamIndex, streamNo);
            _videoStreamIndex = streamNo;
        }
    }

    if (_audioStreamIndex != -1) {
        int streamNo = av_find_best_stream(_avFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (streamNo < 0) {
            logger("No Audio stream found");
            return false;
        }

        if (_audioStreamIndex != streamNo) {
            logger("Audio stream index changed, %d -> %d", _audioStreamIndex, streamNo);
            _audioStreamIndex = streamNo;
        }
    }

    if (_isFileInput) _timstampOffset = _lastTimstamp + 1;

    av_read_play(_avFmtCtx);

    if (_videoJitterBuffer) _videoJitterBuffer->start();
    if (_audioJitterBuffer) _audioJitterBuffer->start();

    if (!strcmp(_avFmtCtx->iformat->name, "flv") && _videoStreamIndex != -1 &&
        _avFmtCtx->streams[_videoStreamIndex]->codecpar->codec_id == AV_CODEC_ID_H264) {
        _enableVideoExtradata = true;
        logger("Enable video extradata");
    }

    return true;
}

bool LiveStreamIn::connect()
{
    int res;
    _avFmtCtx = avformat_alloc_context();
    //    _avFmtCtx->interrupt_callback = { &TimeoutHandler::checkInterrupt, _timeoutHandler };

    logger("Opening input");
    _timeoutHandler->reset(30000);
    res = avformat_open_input(&_avFmtCtx, _url.c_str(), nullptr, _options != nullptr ? &_options : nullptr);
    if (res != 0) {
        logger("Failed to open input %s, %s", _url.c_str(), ff_err2str(res));
        return false;
    }

    logger("Finding stream info");
    _timeoutHandler->reset(10000);
    _avFmtCtx->fps_probe_size = 0;
    _avFmtCtx->max_ts_probe = 0;
    res = avformat_find_stream_info(_avFmtCtx, nullptr);
    if (res < 0) {
        logger("Failed to find stream info %s", ff_err2str(res));
        return false;
    }

    logger("Dump format");
    av_dump_format(_avFmtCtx, 0, _url.c_str(), 0);

    AVStream *vStream, *aStream;
    if (_enableVideo == "yes" || _enableVideo == "auto") {
        logger("find video index");
        int streamNo = av_find_best_stream(_avFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (streamNo >= 0) {
            vStream = _avFmtCtx->streams[streamNo];
            logger("Has video, video stream number(%d), codec(%s), %s, %dx%d", streamNo,
                   avcodec_get_name(vStream->codecpar->codec_id),
                   avcodec_profile_name(vStream->codecpar->codec_id, vStream->codecpar->profile),
                   vStream->codecpar->width, vStream->codecpar->height);

            switch (vStream->codecpar->codec_id) {
                case AV_CODEC_ID_H264:
                    _videoFormat = FRAME_FORMAT_H264;
                    switch (vStream->codecpar->profile) {
                        case FF_PROFILE_H264_CONSTRAINED_BASELINE:
                            logger("CB");
                            break;
                        case FF_PROFILE_H264_BASELINE:
                            logger("Baseline");
                            break;
                        case FF_PROFILE_H264_MAIN:
                            logger("Main");
                            break;
                        case FF_PROFILE_H264_EXTENDED:
                            logger("EXTENDED");
                            break;
                        case FF_PROFILE_H264_HIGH:
                            logger("H");
                            break;
                        default:
                            logger("Unsupported H264 profile: %s",
                                   avcodec_profile_name(vStream->codecpar->codec_id, vStream->codecpar->profile));
                            break;
                    }
                    break;
                case AV_CODEC_ID_H265:
                    _videoFormat = FRAME_FORMAT_H265;
                    break;
                default:
                    logger("Video codec %s is not supported", avcodec_get_name(vStream->codecpar->codec_id));
                    break;
            }

            if (_videoFormat != FRAME_FORMAT_UNKNOWN) {
                _videoWidth = vStream->codecpar->width;
                _videoHeight = vStream->codecpar->height;

                if (!isRtsp()) _videoJitterBuffer.reset(new JitterBuffer("video", JitterBuffer::SYNC_MODE_SLAVE, this));

                _videoTimeBase.num = 1;
                _videoTimeBase.den = 90000;

                _videoStreamIndex = streamNo;
                logger("_videoStreamIndex = %d", _videoStreamIndex);
            } else if (_enableVideo == "yes") {
                logger("video codec is not supported");
                return false;
            }
        } else {
            logger("No Video stream found");

            if (_enableVideo == "yes") {
                logger("no video stream found");
                return false;
            }
        }
    }

    if (_enableAudio == "yes" || _enableAudio == "auto") {
        logger("find audio index");
        int streamNo = av_find_best_stream(_avFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (streamNo >= 0) {
            aStream = _avFmtCtx->streams[streamNo];
            logger("Has audio, audio stream number(%d), codec(%s), %d-%d", streamNo,
                   avcodec_get_name(aStream->codecpar->codec_id), aStream->codecpar->sample_rate,
                   aStream->codecpar->channels);

            switch (aStream->codecpar->codec_id) {
                case AV_CODEC_ID_AAC:
                    _audioFormat = FRAME_FORMAT_AAC;
                    break;
                case AV_CODEC_ID_OPUS:
                    _audioFormat = FRAME_FORMAT_OPUS;
                    break;
                case AV_CODEC_ID_MP3:
                    _audioFormat = FRAME_FORMAT_MP3;
                    break;
                default:
                    logger("Audio codec %s is not supported ", avcodec_get_name(aStream->codecpar->codec_id));
                    break;
            }

            if (_audioFormat != FRAME_FORMAT_UNKNOWN) {
                if (!isRtsp())
                    _audioJitterBuffer.reset(new JitterBuffer("audio", JitterBuffer::SYNC_MODE_MASTER, this));

                _audioTimeBase.num = 1;
                _audioTimeBase.den = aStream->codecpar->sample_rate;
                _audioSampleRate = aStream->codecpar->sample_rate;
                _audioChannels = aStream->codecpar->channels;
                _audioStreamIndex = streamNo;
                logger("_videoStreamIndex = %d", _audioStreamIndex);
            } else if (_enableAudio == "yes") {
                logger("audio codec is not supported");
                return false;
            }
        } else {
            logger("No Audio stream found");

            if (_enableAudio == "yes") {
                logger("no audio stream found");
                return false;
            }
        }
    }

    _msTimeBase.num = 1;
    _msTimeBase.den = 1000;

    av_read_play(_avFmtCtx);

    if (_videoJitterBuffer) {
        _videoJitterBuffer->start();
    }

    if (_audioJitterBuffer) {
        _audioJitterBuffer->start();
    }

    if (!strcmp(_avFmtCtx->iformat->name, "flv") && _videoStreamIndex != -1 &&
        _avFmtCtx->streams[_videoStreamIndex]->codecpar->codec_id == AV_CODEC_ID_H264) {
        _enableVideoExtradata = true;
        logger("Enable video extradata");
    }

    return true;
}

void LiveStreamIn::receiveLoop()
{
    int ret = connect();
    if (!ret) {
        logger("Connect failed, %s", _url.c_str());
        return;
    }
    logger("connect ok");
    logger("Start playing %s", _url.c_str());

    if (_videoStreamIndex != -1) {
        int i = 0;
        while (_runing && !_keyFrameRequest) {
            if (i++ >= 100) {
                logger("No incoming key frame request");
                break;
            }

            deliverNullVideoFrame();
            logger("Wait for key frame request, retry %d", i);
            usleep(10000);
        }
    }

    while (_runing) {
        logger("-");
        if (_isFileInput) {
            if (_videoJitterBuffer && _videoJitterBuffer->sizeInMs() > 500) {
                usleep(1000);
                continue;
            }

            if (_audioJitterBuffer && _audioJitterBuffer->sizeInMs() > 500) {
                usleep(1000);
                continue;
            }
        }

        av_packet_unref(_avPacket);
        ret = av_read_frame(_avFmtCtx, _avPacket);
        if (ret < 0) {
            logger("Error read frame, %s", ff_err2str(ret));
            if (isFileInput()) {
                break;
            }

            // Try to re-open the input - silently.
            ret = reconnect();
            if (!ret) {
                logger("Reconnect failed");
                break;
            }
            continue;
        }

        logger("stream_index = %d", _avPacket->stream_index);
        if (_avPacket->stream_index == _videoStreamIndex) {
            AVStream *vStream = _avFmtCtx->streams[_videoStreamIndex];
            _avPacket->dts = av_rescale_q(_avPacket->dts, vStream->time_base, _msTimeBase) + _timestampOffset;
            _avPacket->pts = av_rescale_q(_avPacket->dts, vStream->time_base, _msTimeBase) + _timestampOffset;

            logger("Receive video frame packet, dts %ld, size %d", _avPacket->dts, _avPacket->size);
            if (filterVBS(vStream, _avPacket)) {
                filterPS(vStream, _avPacket);

                if (_videoJitterBuffer)
                    _videoJitterBuffer->insert(_avPacket);
                else
                    deliverVideoFrame(_avPacket);
            }
        } else if (_avPacket->stream_index == _audioStreamIndex) {
            AVStream *aStream = _avFmtCtx->streams[_audioStreamIndex];
            _avPacket->dts = av_rescale_q(_avPacket->dts, aStream->time_base, _msTimeBase) + _timestampOffset;
            _avPacket->pts = av_rescale_q(_avPacket->dts, aStream->time_base, _msTimeBase) + _timestampOffset;

            logger("Receive audio frame packet, dts %ld, duration %ld, size %d", _avPacket->dts, _avPacket->duration,
                   _avPacket->size);

            if (_audioJitterBuffer)
                _audioJitterBuffer->insert(_avPacket);
            else
                deliverAudioFrame(_avPacket);
        }
        _lastTimestamp = _avPacket->dts;
        av_packet_unref(_avPacket);
    }

    logger("Thread exited!");
}

// init video bitstream filter 'mp4toannexb' for codec H264/HEVC
void LiveStreamIn::checkVideoBitstream(AVStream *st, const AVPacket *pkt)
{
    int ret;
    const char *filter_name = nullptr;
    const AVBitStreamFilter *bsf;

    if (!_needCheckVBS) return;

    _needApplyVBSF = false;
    switch (st->codecpar->codec_id) {
        case AV_CODEC_ID_H264:
            if (pkt->size < 5 || AV_RB32(pkt->data) == 0x0000001 || AV_RB24(pkt->data) == 0x000001) break;
            filter_name = "h264_mp4toannexb";
            break;
        case AV_CODEC_ID_HEVC:
            if (pkt->size < 5 || AV_RB32(pkt->data) == 0x0000001 || AV_RB24(pkt->data) == 0x000001) break;
            filter_name = "hevc_mp4toannexb";
            break;
        default:
            break;
    }

    if (filter_name) {
        bsf = av_bsf_get_by_name(filter_name);
        if (!bsf) {
            logger("Fail to get bsf, %s", filter_name);
            goto exit;
        }

        ret = av_bsf_alloc(bsf, &_vbsf);
        if (ret) {
            logger("Fail to alloc bsf context");
            goto exit;
        }

        ret = avcodec_parameters_copy(_vbsf->par_in, st->codecpar);
        if (ret < 0) {
            logger("Fail to copy bsf parameters, %s", ff_err2str(ret));
            av_bsf_free(&_vbsf);
            _vbsf = nullptr;
            goto exit;
        }

        ret = av_bsf_init(_vbsf);
        if (ret < 0) {
            logger("Fail to init bsf, %s", ff_err2str(ret));
            av_bsf_free(&_vbsf);
            _vbsf = nullptr;
            goto exit;
        }

        _needApplyVBSF = true;
    }

exit:
    _needCheckVBS = false;
    logger("%s video bitstream filter", _needApplyVBSF ? "Apply" : "Not apply");
}

bool LiveStreamIn::filterVBS(AVStream *st, AVPacket *pkt)
{
    int ret;

    checkVideoBitstream(st, pkt);
    if (!_needApplyVBSF) return true;

    if (!_vbsf) {
        logger("Invalid vbs filter");
        return false;
    }

    AVPacket filterPkt, filteredPkt;

    memset(&filterPkt, 0, sizeof(filterPkt));
    memset(&filteredPkt, 0, sizeof(filteredPkt));

    if ((ret = av_packet_ref(&filterPkt, pkt)) < 0) {
        logger("Fail to ref input pkt, %s", ff_err2str(ret));
        return false;
    }

    if ((ret = av_bsf_send_packet(_vbsf, &filterPkt)) < 0) {
        logger("Fail to send packet, %s", ff_err2str(ret));
        av_packet_unref(&filterPkt);
        return false;
    }

    if ((ret = av_bsf_receive_packet(_vbsf, &filteredPkt)) < 0) {
        logger("Fail to receive packet, %s", ff_err2str(ret));
        av_packet_unref(&filterPkt);
        return false;
    }

    av_packet_unref(&filterPkt);
    av_packet_unref(pkt);
    av_packet_move_ref(pkt, &filteredPkt);

    return true;
}

// AVCC -> Annex-B
bool LiveStreamIn::parseAVCC(AVPacket *pkt)
{
    uint8_t *data;
    int size;

    data = av_packet_get_side_data(_avPacket, AV_PKT_DATA_NEW_EXTRADATA, &size);
    if (data == nullptr) return true;

    //    AVCC structure
    //    bits
    //    8   version ( always 0x01 )
    //    8   avc profile ( sps[0][1] )
    //    8   avc compatibility ( sps[0][2] )
    //    8   avc level ( sps[0][3] )
    //    3   reserved ( all bits on )
    //
    //    5   number of SPS NALUs (usually 1)
    //    repeated once per SPS:
    //    16     SPS size
    //    variable   SPS NALU data
    //
    //    8   number of PPS NALUs (usually 1)
    //    repeated once per PPS
    //    16    PPS size
    //    variable PPS NALU data

    if (data[0] == 1) {  // AVCDecoderConfigurationRecord
        int nalsSize = 0;

        if (_spsPpsBuffer) {
            _spsPpsBuffer.reset();
            _spsPpsBufferLength = 0;
        }

        int nalsBufLength = 128;
        uint8_t *nalsBuf = (uint8_t *)malloc(nalsBufLength);

        int i, cnt, nalSize;
        const uint8_t *p = data;

        if (size < 7) {
            logger("avcC %d too short", size);
            free(nalsBuf);
            return false;
        }

        // Decode sps from avcC
        cnt = *(p + 5) & 0x1f;  // Number of sps
        p += 6;
        for (i = 0; i < cnt; i++) {
            nalSize = AV_RB16(p) + 2;
            if (nalSize > size - (p - data)) {
                logger("avcC %d invalid sps size", nalSize);
                free(nalsBuf);
                return false;
            }

            p += 2;
            nalSize -= 2;

            if (nalSize + 4 >= nalsBufLength - nalsSize) {
                logger("enlarge avcC nals buf %d", nalSize + 4);

                nalsBufLength += nalSize + 4;
                nalsBuf = (uint8_t *)realloc(nalsBuf, nalsBufLength);
            }

            nalsBuf[nalsSize] = 0;
            nalsBuf[nalsSize + 1] = 0;
            nalsBuf[nalsSize + 2] = 0;
            nalsBuf[nalsSize + 3] = 1;
            memcpy(nalsBuf + nalsSize + 4, p, nalSize);
            nalsSize += nalSize + 4;

            p += nalSize;
        }

        // Decode pps from avcC
        cnt = *(p++);  // Number of pps
        for (i = 0; i < cnt; i++) {
            nalSize = AV_RB16(p) + 2;
            if (nalSize > size - (p - data)) {
                logger("avcC %d invalid pps size", nalSize);

                free(nalsBuf);
                return false;
            }

            p += 2;
            nalSize -= 2;

            if (nalSize + 4 >= nalsBufLength - nalsSize) {
                logger("enlarge avcC nals buf %d", nalSize + 4);

                nalsBufLength += nalSize + 4;
                nalsBuf = (uint8_t *)realloc(nalsBuf, nalsBufLength);
            }

            nalsBuf[nalsSize] = 0;
            nalsBuf[nalsSize + 1] = 0;
            nalsBuf[nalsSize + 2] = 0;
            nalsBuf[nalsSize + 3] = 1;
            memcpy(nalsBuf + nalsSize + 4, p, nalSize);
            nalsSize += nalSize + 4;

            p += nalSize;
        }

        if (nalsSize > 0) {
            logger("New video extradata");

            _spsPpsBufferLength = nalsSize;
            _spsPpsBuffer.reset(new uint8_t[_spsPpsBufferLength]);
            memcpy(_spsPpsBuffer.get(), nalsBuf, _spsPpsBufferLength);
        }

        free(nalsBuf);
    } else {
        logger("Unsupported video extradata\n");
    }

    return true;
}

bool LiveStreamIn::filterPS(AVStream *st, AVPacket *pkt)
{
    if (!_enableVideoExtradata) return true;

    parseAVCC(pkt);
    if (_spsPpsBuffer && _spsPpsBufferLength > 0) {
        int size;

        std::vector<int> sps_pps, pass_types;
        sps_pps.push_back(7);
        sps_pps.push_back(8);

        size = filterNALs(pkt->data, pkt->size, sps_pps, pass_types);
        if (size != pkt->size) {
            logger("Rewrite sps/pps\n");

            av_shrink_packet(pkt, size);
            av_grow_packet(pkt, _spsPpsBufferLength);
            // insert sps/pps before video data
            memmove(pkt->data + _spsPpsBufferLength, pkt->data, pkt->size - _spsPpsBufferLength);
            memcpy(pkt->data, _spsPpsBuffer.get(), _spsPpsBufferLength);
        }
    }

    return true;
}
