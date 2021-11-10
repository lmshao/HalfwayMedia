//
// Created by lmshao on 2021/11/8.
//

#ifndef HALFWAYMEDIA_ADTSHEADER_H
#define HALFWAYMEDIA_ADTSHEADER_H

#include <string.h>
int sampling_frequency_table[13] = {96000, 88200, 64000, 48000, 44100, 32000, 24000,
                                    22050, 16000, 12000, 11025, 8000,  7350};
// 7 bytes
struct ADTS_Header {
    // fixed header
    // 1 byte
    uint8_t sync_word_l : 8;  // 0xFF
    // 2 byte
    // sync_word_h + id + layer + protection_absent
    uint8_t protection_absent : 1;  // 1 no CRC, 0 has CRC
    uint8_t layer : 2;              // 00
    uint8_t id : 1;                 // 0: MPEG-4, 1: MPEG-2
    uint8_t sync_word_h : 4;        // 0xF

    // 3rd byte
    // profile + sampling_frequency_index + private_bit + channel_configuration_l
    uint8_t channel_configuration_l : 1;
    uint8_t private_bit : 1;
    uint8_t sampling_frequency_index : 4;
    uint8_t profile : 2;  // 0:main, 1: LC, 2: SSR, 3: reserved

    // 4th byte
    uint8_t aac_frame_length_l : 2;
    uint8_t copyright_identification_start : 1;
    uint8_t copyright_identification_bit : 1;
    uint8_t home : 1;
    uint8_t original_copy : 1;
    uint8_t channel_configuration_h : 2;

    // 5th byte
    uint8_t aac_frame_length_m : 8;
    // 6th byte
    uint8_t adts_buffer_fullness_l : 5;
    uint8_t aac_frame_length_h : 3;
    // 7th byte
    uint8_t number_of_raw_data_blocks_in_frame : 2;
    uint8_t adts_buffer_fullness_h : 6;  // adts_buffer_fullness 0x7ff VBR

    ADTS_Header()
    {
        memset(this, 0, sizeof(ADTS_Header));
        setSyncWord();
        protection_absent = 1;
        profile = 1;
    }

    ADTS_Header(int samplingFreq, int channel, int length)
    {
        memset(this, 0, sizeof(ADTS_Header));
        setSyncWord();
        setSamplingFrequency(samplingFreq);
        setChannel(channel);
        setLength(length);
        protection_absent = 1;
        profile = 1;
    }

    ADTS_Header &setSyncWord()
    {
        sync_word_l = 0xff;
        sync_word_h = 0xf;
        return *this;
    }

    ADTS_Header &setSamplingFrequency(int sf)
    {
        sampling_frequency_index = 0xf;
        for (int i = 0; i < 13; ++i) {
            if (sampling_frequency_table[i] == sf) {
                sampling_frequency_index = i;
                break;
            }
        }
        return *this;
    }

    ADTS_Header &setChannel(int ch)
    {
        if (ch > 0 && ch < 7)
            channel_configuration_h = ch;
        else if (ch == 8)
            channel_configuration_h = 7 >> 24;
        return *this;
    }

    // length = header length + aac es stream length
    ADTS_Header &setLength(int length)
    {
        aac_frame_length_l = (length >> 11) & 0x03;
        aac_frame_length_m = (length >> 3) & 0xff;
        aac_frame_length_h = (length & 0x07);
        return *this;
    }

    int getLength()
    {
        int l = (aac_frame_length_l << 11) | (aac_frame_length_m << 3) | (aac_frame_length_h);
        return l;
    }

    int getChannel()
    {
        if (channel_configuration_h == 8) return 8;
        return channel_configuration_h;
    }
};

#endif  // HALFWAYMEDIA_ADTSHEADER_H
