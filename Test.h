//
// Copyright 2021 Liming SHAO <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAYMEDIA_TEST_H
#define HALFWAYMEDIA_TEST_H

int audio_encoding_pcm_s16le_to_aac();

int video_encoding_yuv420_to_h264();

int video_encoding_bgra32_to_h264();

int video_encoding_bgra32_to_mp4();

int video_converison_bgra32_to_yuv420();

int audio_conversion_f32le_to_s16le();

int audio_encoding_pcm_f32le_to_aac();

int audio_encoding_pcm_f32le_to_mp4();

#endif  // HALFWAYMEDIA_TEST_H
