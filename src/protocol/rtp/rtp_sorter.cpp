//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtp_sorter.h"
#include "../../common/log.h"
#include "rtp_packet.h"
#include <cstdint>
#include <mutex>

const size_t SORTER_CACHE_SIZE_MAX = 100;

void RtpSorter::Input(std::shared_ptr<DataBuffer> packet)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (packet == nullptr || packet->Size() <= 12) { // RTP fixed header size
        return;
    }

    RtpHeader *rtp = (RtpHeader *)packet->Data();
    uint16_t seq = rtp->GetSeqNumber();
    static CompareRtpSequenceNumber comparator;

    if (!init_) {
        init_ = true;
        lastSeq_ = seq;
        if (callback_) {
            callback_(packet);
        }
        return;
    }

    // ordered rtp packet
    if (lastSeq_ + 1 == seq || (lastSeq_ == UINT16_MAX && seq == 0)) {
        lastSeq_ = seq;
        if (callback_) {
            callback_(packet);
        }

        TryPopCache();
    } else if (comparator(seq, lastSeq_)) {
        LOGW("discard the late packet %d before last %d", seq, lastSeq_);
    } else {
        cache_.emplace(seq, packet);

        if (cache_.size() > SORTER_CACHE_SIZE_MAX) {
            lastSeq_ = cache_.begin()->first;
            if (callback_) {
                callback_(cache_.begin()->second);
            }

            cache_.erase(cache_.begin());
        }

        TryPopCache();
    }
}

void RtpSorter::TryPopCache()
{
    if (cache_.empty()) {
        return;
    }

    std::vector<uint16_t> keysToDelete;
    for (auto &it : cache_) {
        if (lastSeq_ + 1 == it.first) {
            lastSeq_ = it.first;

            if (callback_) {
                callback_(it.second);
            }

            keysToDelete.push_back(it.first);
        } else {
            break;
        }
    }

    for (uint16_t key : keysToDelete) {
        cache_.erase(key);
    }
}
