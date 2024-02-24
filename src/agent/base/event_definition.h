//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_AGENT_EVENT_DEFINITION_H
#define HALFWAY_MEDIA_AGENT_EVENT_DEFINITION_H

#include "../../common/frame.h"
#include <string>

enum EventType {
    EVENT_SINK_START,
    EVENT_SINK_STOP,
    EVENT_SINK_INIT,
    EVENT_SINK_SET_PARAMETERS,
    EVENT_SINK_ERROR,
    EVENT_SOURCE_MESSAGE,
    EVENT_SOURCE_ERROR,
};

struct MediaParameters {
    VideoFrameInfo *video;
    AudioFrameInfo *audio;
};

struct StringInfo {
    std::string info;
};

struct AgentEvent {
    EventType type;
    union {
        StringInfo *info;
        MediaParameters *params;
    };
};

#endif // HALFWAY_MEDIA_AGENT_EVENT_DEFINITION_H