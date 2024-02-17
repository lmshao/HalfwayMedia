//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtsp_sdp.h"
#include "../../common/base64.h"
#include <cstdint>
#include <cstdio>
#include <math.h>
#include <memory>
#include <string>
#include <vector>

bool RtspSdp::Parse(const std::string &sdp)
{
    std::regex regexPattern("(\\w)=(.*)");
    auto regexIterator = std::sregex_iterator(sdp.begin(), sdp.end(), regexPattern);
    auto regexEnd = std::sregex_iterator();

    std::shared_ptr<MediaDescription> mediaTrack;
    for (; regexIterator != regexEnd; ++regexIterator) {
        std::smatch match = *regexIterator;

        std::string key = match[1].str();
        std::string value = match[2].str();
        if (key.empty() || value.empty()) {
            LOGW("invalid line");
            continue;
        }

        switch (key[0]) {
            case 'v':
                version_ = std::stoi(value);
                break;
            case 'o': {
                // o=<username> <sess-id> <sess-version> <nettype> <addrtype> <unicast-address>
                std::regex pattern("([a-zA-Z0-9-]+) ([a-zA-Z0-9-]+) ([a-zA-Z0-9-]+) (IN) "
                                   "(IP4|IP6) ([0-9a-f.:]+)");
                std::smatch sm;
                if (!std::regex_search(value, sm, pattern)) {
                    return false;
                }

                origin_.username = sm[1];
                origin_.sessionId = sm[2];
                origin_.version = std::stoi(sm[3]);
                origin_.netType = sm[4];
                origin_.addrType = sm[5];
                origin_.unicastAddr = sm[6];
                break;
            }
            case 's':
                sessionName_ = value;
                break;
            case 'i':
                sessionTitle_ = value;
                break;
            case 't':
                time_ = value;
                break;
            case 'a':
                if (!mediaTrack) {
                    attributes_.emplace_back(value);
                } else {
                    mediaTrack->attributes.emplace_back(value);
                }
                break;
            case 'm': {
                // m=<media_> <port> <proto> <fmt> ...
                std::regex pattern("^(video|audio|text|application|message) ([0-9]+)(/[0-9]+)? "
                                   "(udp|RTP/AVP|RTP/SAVP|RTP/SAVPF) ([0-9]+)");
                std::smatch sm;
                if (!std::regex_search(value, sm, pattern)) {
                    break;
                }

                if (sm[1].compare("video") == 0) {
                    videoTrack_ = std::make_shared<MediaDescription>();
                    videoTrack_->type = VIDEO;
                    mediaTrack = videoTrack_;

                } else if (sm[1].compare("audio") == 0) {
                    audioTrack_ = std::make_shared<MediaDescription>();
                    audioTrack_->type = AUDIO;
                    mediaTrack = audioTrack_;
                }

                if (mediaTrack) {
                    mediaTrack->port = std::stoi(sm[2]);
                    mediaTrack->transportType = sm[4];
                    mediaTrack->payloadType = std::stoi(sm[5]);
                }
                break;
            }
            case 'c':
                if (mediaTrack) {
                    mediaTrack->connectionInfo = value;
                } else {
                    connectionInfo_ = value;
                }
                break;
            case 'b':
                if (mediaTrack) {
                    mediaTrack->bandwidth = value;
                }
                break;
            default:
                LOGW("unsupported key: %c", key[0]);
                break;
        }
    }

    return true;
}

std::string MediaDescription::GetTrackId()
{
    std::string mark("control:");
    for (auto &a : attributes) {
        if (a.find(mark) != std::string::npos) {
            return a.substr(mark.size());
        }
    }

    return {};
}

std::string MediaDescription::GetAudioConfig()
{
    if (type != AUDIO) {
        return {};
    }

    std::string mark("fmtp:");
    for (auto &a : attributes) {
        if (a.find(mark) != std::string::npos) {
            auto s = a.substr(mark.size());
            auto i = s.find(" ");
            if (i != std::string::npos) {
                return s.substr(i + 1);
            }
        }
    }

    return {};
}

bool MediaDescription::GetAudioConfig(int &pt, std::string &format, int &samplingRate, int &channels)
{
    if (type != AUDIO) {
        return false;
    }

    std::string mark("rtpmap:");
    for (auto &a : attributes) {
        if (a.find(mark) != std::string::npos) {
            auto s = a.substr(mark.size());
            LOGD("rtpmap: %s", s.c_str());
            std::regex pattern("([0-9]+) ([a-zA-Z0-9-]+)/([0-9]+)/([1-9]{1})");
            std::smatch sm;
            if (!std::regex_search(s, sm, pattern)) {
                LOGE("error");
                return false;
            }

            pt = std::stoi(sm[1]);
            format = sm[2];
            samplingRate = std::stoi(sm[3]);
            channels = std::stoi(sm[4]);
            return true;
        }
    }

    return false;
}

bool MediaDescription::GetVideoConfig(int &pt, std::string &format, int &clockCycle)
{
    if (type != VIDEO) {
        return false;
    }

    std::string mark("rtpmap:");
    for (auto &a : attributes) {
        if (a.find(mark) != std::string::npos) {
            auto s = a.substr(mark.size());
            LOGD("%s", s.c_str());

            std::regex pattern("([0-9]+) ([a-zA-Z0-9-]+)/([0-9]+)");
            std::smatch sm;
            if (!std::regex_search(s, sm, pattern)) {
                LOGE("error");
                return false;
            }

            pt = std::stoi(sm[1]);
            format = sm[2];
            clockCycle = std::stoi(sm[3]);
        }
    }
    return true;
}

std::shared_ptr<DataBuffer> MediaDescription::GetVideoSps()
{
    if (sps_ != nullptr) {
        return sps_;
    }

    ParseVideoSpsPps();
    return sps_;
}

std::shared_ptr<DataBuffer> MediaDescription::GetVideoPps()
{
    if (pps_ != nullptr) {
        return pps_;
    }

    return pps_;
}

std::pair<int, int> MediaDescription::GetVideoSize()
{
    if (sps_ == nullptr) {
        ParseVideoSpsPps();
    }

    if (sps_ == nullptr || sps_->Size() <= 14) {
        LOGE("sps is invalid");
        return {0, 0};
    }

    int32_t width = 0;
    int32_t height = 0;
    uint8_t *buf = sps_->Data() + 4;
    uint32_t len = sps_->Size() - 4;
    uint32_t cursor = 0;

    auto spsBak = std::make_shared<DataBuffer>(*sps_.get());

    ExtractNaluRbsp(buf, &len);

    /// Rec. ITU-T H.264 (08/2021)
    /// 7.3.2.1.1 Sequence parameter set data syntax

    // forbidden_zero_bit
    GetU(1, buf, cursor);
    // nal_ref_idc
    GetU(2, buf, cursor);
    int32_t nalUnitType = GetU(5, buf, cursor);
    if (nalUnitType != 7) {
        return {width, height};
    }

    int32_t profile_idc = GetU(8, buf, cursor);
    // constraint_set0_flag
    GetU(1, buf, cursor);
    // constraint_set1_flag
    GetU(1, buf, cursor);
    // constraint_set2_flag
    GetU(1, buf, cursor);
    // constraint_set3_flag
    GetU(1, buf, cursor);
    // constraint_set4_flag
    GetU(1, buf, cursor);

    // constraint_set5_flag
    GetU(1, buf, cursor);
    // reserved_zero_2bits
    GetU(2, buf, cursor);
    // level_idc
    GetU(8, buf, cursor);
    // seq_parameter_set_id
    GetUe(buf, len, cursor);

    if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
        profile_idc == 83 || profile_idc == 86 || profile_idc == 118 || profile_idc == 128 || profile_idc == 138 ||
        profile_idc == 139 || profile_idc == 134 || profile_idc == 135) {
        int32_t chroma_format_idc = GetUe(buf, len, cursor);
        if (chroma_format_idc == 3) {
            // separate_colour_plane_flag
            GetU(1, buf, cursor);
        }
        // bit_depth_luma_minus8
        GetUe(buf, len, cursor);
        // bit_depth_chroma_minus8
        GetUe(buf, len, cursor);
        // qpprime_y_zero_transform_bypass_flag
        GetU(1, buf, cursor);
        int32_t seq_scaling_matrix_present_flag = GetU(1, buf, cursor);

        int32_t seq_scaling_list_present_flag[12];
        if (seq_scaling_matrix_present_flag) {
            int32_t lastScale = 8;
            int32_t nextScale = 8;
            int32_t sizeOfScalingList;
            for (int32_t i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++) {
                seq_scaling_list_present_flag[i] = GetU(1, buf, cursor);
                if (seq_scaling_list_present_flag[i]) {
                    lastScale = 8;
                    nextScale = 8;
                    sizeOfScalingList = i < 6 ? 16 : 64;
                    for (int32_t j = 0; j < sizeOfScalingList; j++) {
                        if (nextScale != 0) {
                            int32_t deltaScale = GetSe(buf, len, cursor);
                            nextScale = (lastScale + deltaScale) & 0xff;
                        }
                        lastScale = nextScale == 0 ? lastScale : nextScale;
                    }
                }
            }
        }
    }
    // log2_max_frame_num_minus4
    GetUe(buf, len, cursor);
    int32_t pic_order_cnt_type = GetUe(buf, len, cursor);
    if (pic_order_cnt_type == 0) {
        // log2_max_pic_order_cnt_lsb_minus4
        GetUe(buf, len, cursor);
    } else if (pic_order_cnt_type == 1) {
        // delta_pic_order_always_zero_flag
        GetU(1, buf, cursor);
        // offset_for_non_ref_pic
        GetSe(buf, len, cursor);
        // offset_for_top_to_bottom_field
        GetSe(buf, len, cursor);
        int32_t num_ref_frames_in_pic_order_cnt_cycle = GetUe(buf, len, cursor);

        int32_t *offset_for_ref_frame = new int32_t[num_ref_frames_in_pic_order_cnt_cycle];
        for (int32_t i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            offset_for_ref_frame[i] = GetSe(buf, len, cursor);
        }
        delete[] offset_for_ref_frame;
    }
    // max_num_ref_frames
    GetUe(buf, len, cursor);
    // gaps_in_frame_num_value_allowed_flag
    GetU(1, buf, cursor);
    int32_t pic_width_in_mbs_minus1 = GetUe(buf, len, cursor);
    int32_t pic_height_in_map_units_minus1 = GetUe(buf, len, cursor);

    width = (pic_width_in_mbs_minus1 + 1) * 16;
    height = (pic_height_in_map_units_minus1 + 1) * 16;

    int32_t frame_mbs_only_flag = GetU(1, buf, cursor);
    if (!frame_mbs_only_flag) {
        // mb_adaptive_frame_field_flag
        GetU(1, buf, cursor);
    }

    // direct_8x8_inference_flag
    GetU(1, buf, cursor);
    int32_t frame_cropping_flag = GetU(1, buf, cursor);
    if (frame_cropping_flag) {
        int32_t frame_crop_left_offset = GetUe(buf, len, cursor);
        int32_t frame_crop_right_offset = GetUe(buf, len, cursor);
        int32_t frame_crop_top_offset = GetUe(buf, len, cursor);
        int32_t frame_crop_bottom_offset = GetUe(buf, len, cursor);

        int32_t cropUnitX = 2;
        int32_t cropUnitY = 2 * (2 - frame_mbs_only_flag);
        width -= cropUnitX * (frame_crop_left_offset + frame_crop_right_offset);
        height -= cropUnitY * (frame_crop_top_offset + frame_crop_bottom_offset);
    }

    // restore sps
    if (len != spsBak->Size() - 4) {
        sps_ = spsBak;
    }

    return {width, height};
}

bool MediaDescription::ParseVideoSpsPps()
{
    if (type != VIDEO) {
        return false;
    }

    std::string mark("fmtp:");
    for (auto &a : attributes) {
        if (a.find(mark) != std::string::npos) {
            std::regex pattern("sprop-parameter-sets=([a-zA-Z0-9+=]+),([a-zA-Z0-9+=]+)");
            std::smatch sm;
            if (!std::regex_search(a, sm, pattern)) {
                LOGE("error");
                return false;
            }

            std::string spsBase64 = sm[1];
            std::string ppsBase64 = sm[2];

            std::vector<uint8_t> spsVec, ppsVec;
            if (!Base64::Decode(spsBase64, spsVec) || !Base64::Decode(ppsBase64, ppsVec)) {
                return false;
            }

            char startCode[4] = {0x00, 0x00, 0x00, 0x01};

            sps_ = DataBuffer::Create(4 + spsVec.size());
            sps_->Assign(startCode, 4);
            sps_->Append(spsVec.data(), spsVec.size());

            pps_ = DataBuffer::Create(4 + ppsVec.size());
            pps_->Assign(startCode, 4);
            pps_->Append(ppsVec.data(), ppsVec.size());
            return true;
        }
    }

    return false;
}

int32_t MediaDescription::GetUe(const uint8_t *buf, uint32_t nLen, uint32_t &pos)
{
    uint32_t nZeroNum = 0;
    while (pos < nLen * 8) {
        if (buf[pos / 8] & (0x80 >> (pos % 8))) {
            break;
        }
        nZeroNum++;
        pos++;
    }
    pos++;

    uint64_t dwRet = 0;
    for (uint32_t i = 0; i < nZeroNum; i++) {
        dwRet <<= 1;
        if (buf[pos / 8] & (0x80 >> (pos % 8))) {
            dwRet += 1;
        }
        pos++;
    }

    return (int32_t)((1 << nZeroNum) - 1 + dwRet);
}

int32_t MediaDescription::GetSe(uint8_t *buf, uint32_t nLen, uint32_t &pos)
{
    int32_t UeVal = GetUe(buf, nLen, pos);
    double k = UeVal;
    int32_t nValue = ceil(k / 2);
    if (UeVal % 2 == 0) {
        nValue = -nValue;
    }

    return nValue;
}

int32_t MediaDescription::GetU(uint8_t bitCount, const uint8_t *buf, uint32_t &pos)
{
    int32_t value = 0;
    for (uint32_t i = 0; i < bitCount; i++) {
        value <<= 1;
        if (buf[pos / 8] & (0x80 >> (pos % 8))) {
            value += 1;
        }
        pos++;
    }

    return value;
}

void MediaDescription::ExtractNaluRbsp(uint8_t *buf, uint32_t *bufSize)
{
    uint8_t *t = buf;
    uint32_t tSize = *bufSize;
    for (uint32_t i = 0; i < (tSize - 2); i++) {
        if (!t[i] && !t[i + 1] && t[i + 2] == 3) {
            for (uint32_t j = i + 2; j < tSize - 1; j++) {
                t[j] = t[j + 1];
            }
            (*bufSize)--;
        }
    }
}