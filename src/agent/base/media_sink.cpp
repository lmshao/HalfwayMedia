//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "media_sink.h"
#include "agent/base/media_frame_pipeline.h"
#include "common/log.h"
#include "event_definition.h"
#include <memory>

void MediaSink::SetMediaInfo(VideoFrameInfo *videoInfo, AudioFrameInfo *audioInfo)
{
    if (videoInfo) {
        if (!videoInfo_) {
            videoInfo_ = std::make_unique<VideoFrameInfo>();
        }
        LOGW("%d x %d", videoInfo->width, videoInfo->height);

        videoInfo_->framerate = videoInfo->framerate;
        videoInfo_->width = videoInfo->width;
        videoInfo_->height = videoInfo->height;
        videoInfo_->isKeyFrame = videoInfo->isKeyFrame;
    }

    if (audioInfo) {
        if (!audioInfo_) {
            audioInfo_ = std::make_unique<AudioFrameInfo>();
        }
        LOGW("audio %d - %d - %d", audioInfo->sampleRate, audioInfo->channels, audioInfo->nbSamples);

        audioInfo_->channels = audioInfo->channels;
        audioInfo_->nbSamples = audioInfo->nbSamples;
        audioInfo_->sampleRate = audioInfo->sampleRate;
    }
}

void MediaSink::NotifySource(AgentEvent event)
{
    FrameSink::NotifySource(&event);
}

bool MediaSink::Notify(AgentEvent event)
{
    return OnNotify(&event);
}

bool MediaSink::OnNotify(void *userdata)
{
    AgentEvent *event = (AgentEvent *)userdata;

    switch (event->type) {
        case EVENT_SINK_SET_PARAMETERS:
            if (event->params) {
                SetMediaInfo(event->params->video, event->params->audio);
            }
            break;
        case EVENT_SINK_INIT:
            return Init();
        case EVENT_SINK_STOP:
            return Stop();
        case EVENT_SINK_ERROR:
            if (event->info) {
                LOGE("OnError: %s", event->info->info.c_str());
            }
            return Stop();
        default:
            LOGW("unsupported event : %d", (int)event->type);
            break;
    }

    return true;
}