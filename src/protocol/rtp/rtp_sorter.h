//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_PROTOCOL_RTP_SORTER_H
#define HALFWAY_MEDIA_PROTOCOL_RTP_SORTER_H

#include "../../common/data_buffer.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>

struct CompareRtpSequenceNumber {
    bool operator()(const uint16_t &x, const uint16_t &y) const
    {
        uint16_t diff = y - x;
        return diff <= UINT16_MAX / 2 && diff != 0;
    }
};

class RtpSorter {
public:
    void Input(std::shared_ptr<DataBuffer> packet);
    void SetCallback(std::function<void(std::shared_ptr<DataBuffer>)> cb) { callback_ = cb; }

private:
    void TryPopCache();

private:
    uint16_t lastSeq_;
    bool init_ = false;
    std::mutex mutex_;
    std::function<void(std::shared_ptr<DataBuffer>)> callback_;
    std::map<uint16_t, std::shared_ptr<DataBuffer>, CompareRtpSequenceNumber> cache_;
};

#endif // HALFWAY_MEDIA_PROTOCOL_RTP_SORTER_H