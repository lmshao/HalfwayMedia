//
// Created by liming on 2023/2/14.
//

#ifndef WAV_HEADER_H
#define WAV_HEADER_H

// https://exiftool.org/TagNames/RIFF.html
#include <stdint.h>
#include <string>

struct RiffSubChunk {
    char id[4];
    uint32_t size;
    uint8_t data[0];
    std::string GetId()
    {
        return std::string(id, 4);
    }
};

struct RiffSubChunkWavFmt : RiffSubChunk {
    uint16_t audio_format = 1; // 1 PCM, 2 MS ADPCM, 3 IEEE float, 6 G711 ALAW, 7 G711 ULAW
    uint16_t num_channels = 2; // 1 2
    uint32_t sample_rate{};    // 44100/48000
    uint32_t byte_rate{};      // channels*sample_rate*bits_per_sample/8
    uint16_t block_align{};    // channels*bits_per_sample/8
    uint16_t bits_per_sample{};

    RiffSubChunkWavFmt() : RiffSubChunk({{'f', 'm', 't', ' '}, 16}) {}
    RiffSubChunkWavFmt(uint16_t channels, uint32_t sample_rate, uint16_t sample_depth)
        : RiffSubChunk({{'f', 'm', 't', ' '}, 16}), num_channels(channels), sample_rate(sample_rate),
          bits_per_sample(sample_depth)
    {
        block_align = num_channels * bits_per_sample / 8;
        byte_rate = block_align * sample_rate;
    }

    void Finish()
    {
        block_align = num_channels * bits_per_sample / 8;
        byte_rate = block_align * sample_rate;
    }

    void Dump() const
    {
        if (audio_format != 1) {
            printf("Not PCM format\n");
            return;
        }
        printf("PCM:\t\t%d\n"
               "channel(s):\t%d\n"
               "sampling rate:\t%d\n"
               "bit depth:\t%d\n"
               "byte rate:\t%d kb/s\n",
               audio_format == 1 ? 1 : 0, num_channels, sample_rate, bits_per_sample, byte_rate * 8 / 1000);
    }
};

struct RiffSubChunkData : RiffSubChunk {
    RiffSubChunkData() : RiffSubChunk({{'d', 'a', 't', 'a'}, 0}) {}
};

struct RiffHeader : RiffSubChunk {
    char format[4]{};
    uint8_t sub_chunk[0];

    explicit RiffHeader(const char f[4]) : RiffSubChunk({{'R', 'I', 'F', 'F'}}), format{f[0], f[1], f[2], f[3]} {}

    RiffSubChunkWavFmt *GetWavFmtChunk()
    {
        uint8_t *sc = sub_chunk;
        while (sc < sub_chunk + size) {
            if (((RiffSubChunk *)sc)->GetId() == "fmt ") {
                return (RiffSubChunkWavFmt *)sc;
            } else {
                sc += (((RiffSubChunk *)sc)->size + 8);
            }
        }

        return nullptr;
    }

    RiffSubChunkData *GetWavDataChunk()
    {
        RiffSubChunkWavFmt *fmt = GetWavFmtChunk();
        if (!fmt) {
            return nullptr;
        }

        uint8_t *sc = (uint8_t *)fmt + 8 + fmt->size;
        while (sc < sub_chunk + size) {
            if (((RiffSubChunk *)sc)->GetId() == "data") {
                return (RiffSubChunkData *)sc;
            } else {
                sc += (((RiffSubChunk *)sc)->size + 8);
            }
        }

        return nullptr;
    }
};

// The simplest WAV structure
// Usage:
//     DefaultWavHeader wavHeader;
//     wavHeader.size = pcmSize + sizeof(DefaultWavHeader) - 8;
//     wavHeader.fmt = RiffSubChunkWavFmt(2, 44100, 16);
//     wavHeader.data.size = pcmSize;
//     write(fd, (char*)&wavHeader, sizeof(DefaultWavHeader));
//     write(fd, pcm, pcmSize);
struct DefaultWavHeader : RiffHeader {
    DefaultWavHeader() : RiffHeader("WAVE") {}
    RiffSubChunkWavFmt fmt;
    RiffSubChunkData data;
};

#endif // WAV_HEADER_H
