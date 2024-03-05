//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "rtcp.h"
#include "common/data_buffer.h"
#include "common/log.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

constexpr const uint8_t VERSON = 0x02;
constexpr const size_t SSRC_SIZE = 4;
constexpr const uint64_t NPT_OFFSET_70_YEARS = 2208988800ULL;

#define BS16 __builtin_bswap16
#define BS32 __builtin_bswap32

#define ALIGN32(x) (((x + 3) >> 2) << 2)

#define ASSERT_RETRUN_NULL(expr, exec)                                                                                 \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            exec;                                                                                                      \
            return nullptr;                                                                                            \
        }                                                                                                              \
    } while (0);

std::shared_ptr<RtcpSR> RtcpSR::Parse(const uint8_t *p, size_t length)
{
    ASSERT_RETRUN_NULL((p != nullptr && length >= (sizeof(RtcpHeader) + SSRC_SIZE + sizeof(SenderInfo))),
                       LOGE("invalid input param"));

    auto *header = (RtcpHeader *)p;
    ASSERT_RETRUN_NULL(header->version == VERSON, LOGE("version(%d) is invalid", header->version));
    ASSERT_RETRUN_NULL(header->pt == RTCP_SR, LOGE("pt(%u) != RTCP_SR(%d)", header->pt, RTCP_SR));
    ASSERT_RETRUN_NULL(length >= header->GetLength(),
                       LOGD("actual length(%zu) < rtcp field length(%d)", length, header->GetLength()));

    auto sr = std::make_shared<RtcpSR>();

    sr->padding = header->padding;
    sr->reportCount = header->subtype;
    sr->length = header->GetLength();
    sr->ssrc = header->GetSSRC();

    auto *end = p + length;

    p += sizeof(RtcpHeader) + SSRC_SIZE;
    sr->senderInfo.nptTsMSW = BS32(((SenderInfo *)p)->nptTsMSW);
    sr->senderInfo.nptTsLSW = BS32(((SenderInfo *)p)->nptTsLSW);
    sr->senderInfo.rtpTs = BS32(((SenderInfo *)p)->rtpTs);
    sr->senderInfo.packetCount = BS32(((SenderInfo *)p)->packetCount);
    sr->senderInfo.octetCount = BS32(((SenderInfo *)p)->octetCount);

    // LOGW("TS MSW: %u, TS LSW: %u, RTP ts: %u, packet count: %u, octet count: %u", sr->senderInfo.nptTsMSW,
    //      sr->senderInfo.nptTsLSW, sr->senderInfo.rtpTs, sr->senderInfo.packetCount, sr->senderInfo.octetCount);

    p += sizeof(SenderInfo);
    if (p == end) {
        return sr;
    }

    while (p + sizeof(ReportBlock) <= end) {
        ReportBlock rb;
        rb.ssrc = BS32(((ReportBlock *)p)->ssrc);
        rb.fractionLost = ((ReportBlock *)p)->fractionLost;
        rb.lostPacketCount = BS32(((ReportBlock *)p)->lostPacketCount << 8) & 0xffffff;
        rb.highestSequence = BS32(((ReportBlock *)p)->highestSequence);
        rb.jitter = BS32(((ReportBlock *)p)->jitter);
        rb.lastSR = BS32(((ReportBlock *)p)->lastSR);
        rb.delaySinceLastSR = BS32(((ReportBlock *)p)->delaySinceLastSR);
        sr->reportBlocks.push_back(rb);
        p += sizeof(ReportBlock);
    }

    return sr;
}

std::shared_ptr<DataBuffer> RtcpSR::Generate()
{
    size_t size = sizeof(RtcpHeader) + SSRC_SIZE + sizeof(SenderInfo) + sizeof(ReportBlock) * reportBlocks.size();
    auto sr = std::make_shared<DataBuffer>(size);
    sr->SetSize(size);

    RtcpHeader *header = (RtcpHeader *)sr->Data();
    header->version = VERSON;
    header->pt = RTCP_SR;
    header->padding = padding;
    header->subtype = reportCount;
    header->SetLength(size);
    header->SetSSRC(ssrc);

    SenderInfo *si = (SenderInfo *)(sr->Data() + sizeof(RtcpHeader) + SSRC_SIZE);
    si->nptTsMSW = BS32(senderInfo.nptTsMSW);
    si->nptTsLSW = BS32(senderInfo.nptTsLSW);
    si->rtpTs = BS32(senderInfo.rtpTs);
    si->packetCount = BS32(senderInfo.packetCount);
    si->octetCount = BS32(senderInfo.octetCount);

    uint8_t *p = sr->Data() + sizeof(RtcpHeader) + SSRC_SIZE + sizeof(SenderInfo);
    for (auto &block : reportBlocks) {
        ReportBlock *rb = (ReportBlock *)p;
        rb->ssrc = BS32(block.ssrc);
        rb->fractionLost = block.fractionLost;
        rb->lostPacketCount = BS32(block.lostPacketCount << 8) & 0xffffff;
        rb->highestSequence = BS32(block.highestSequence);
        rb->jitter = BS32(block.jitter);
        rb->lastSR = BS32(block.lastSR);
        rb->delaySinceLastSR = BS32(block.delaySinceLastSR);
        p += sizeof(ReportBlock);
    }

    return sr;
}

uint64_t RtcpSR::SenderInfo::GetNPTTime() const
{
    uint64_t sec = nptTsMSW - NPT_OFFSET_70_YEARS;
    uint64_t usec = ((uint64_t)nptTsLSW * 1000000) / 0xFFFFFFFFULL;
    return (sec * 1000000) + usec;
}

void RtcpSR::SenderInfo::SetNPTTime(uint64_t utcTime)
{
    nptTsMSW = utcTime / 1000000 + NPT_OFFSET_70_YEARS;
    nptTsLSW = (utcTime % 1000000) * 0xFFFFFFFFULL / 1000000;
}

std::shared_ptr<RtcpRR> RtcpRR::Parse(const uint8_t *p, size_t length)
{
    ASSERT_RETRUN_NULL((p != nullptr && length >= (sizeof(RtcpHeader) + SSRC_SIZE)), LOGE("invalid input param"));

    auto *header = (RtcpHeader *)p;
    ASSERT_RETRUN_NULL(header->version == VERSON, LOGE("version(%d) is invalid", header->version));
    ASSERT_RETRUN_NULL(header->pt == RTCP_RR, LOGE("pt(%u) != RTCP_SR(%d)", header->pt, RTCP_RR));
    ASSERT_RETRUN_NULL(length >= header->GetLength(),
                       LOGD("actual length(%zu) < rtcp field length(%d)", length, header->GetLength()));

    auto rr = std::make_shared<RtcpRR>();

    rr->padding = header->padding;
    rr->reportCount = header->subtype;
    rr->length = header->GetLength();
    rr->ssrc = header->GetSSRC();

    auto *end = p + length;

    p += sizeof(RtcpHeader) + SSRC_SIZE;

    if (p == end) {
        return rr;
    }

    while (p + sizeof(ReportBlock) <= end) {
        ReportBlock rb;
        rb.ssrc = BS32(((ReportBlock *)p)->ssrc);
        rb.fractionLost = ((ReportBlock *)p)->fractionLost;
        rb.lostPacketCount = BS32(((ReportBlock *)p)->lostPacketCount << 8) & 0xffffff;
        rb.highestSequence = BS32(((ReportBlock *)p)->highestSequence);
        rb.jitter = BS32(((ReportBlock *)p)->jitter);
        rb.lastSR = BS32(((ReportBlock *)p)->lastSR);
        rb.delaySinceLastSR = BS32(((ReportBlock *)p)->delaySinceLastSR);
        rr->reportBlocks.push_back(rb);
        p += sizeof(ReportBlock);
    }

    assert(rr->reportCount == rr->reportBlocks.size());
    return rr;
}

std::shared_ptr<DataBuffer> RtcpRR::Generate()
{
    size_t size = sizeof(RtcpHeader) + SSRC_SIZE + sizeof(ReportBlock) * reportBlocks.size();
    auto sr = std::make_shared<DataBuffer>(size);
    sr->SetSize(size);

    RtcpHeader *header = (RtcpHeader *)sr->Data();
    header->version = VERSON;
    header->pt = RTCP_RR;
    header->padding = padding;
    header->subtype = reportBlocks.size();
    header->SetLength(size);
    header->SetSSRC(ssrc);

    uint8_t *p = sr->Data() + sizeof(RtcpHeader) + SSRC_SIZE;
    for (auto &block : reportBlocks) {
        ReportBlock *rb = (ReportBlock *)p;
        rb->ssrc = BS32(block.ssrc);
        rb->fractionLost = block.fractionLost;
        rb->lostPacketCount = BS32(block.lostPacketCount << 8) & 0xffffff;
        rb->highestSequence = BS32(block.highestSequence);
        rb->jitter = BS32(block.jitter);
        rb->lastSR = BS32(block.lastSR);
        rb->delaySinceLastSR = BS32(block.delaySinceLastSR);
        p += sizeof(ReportBlock);
    }

    return sr;
}

std::shared_ptr<RtcpSDES> RtcpSDES::Parse(const uint8_t *p, size_t length)
{
    ASSERT_RETRUN_NULL((p != nullptr && length >= (sizeof(RtcpHeader))), LOGE("invalid input param"));

    auto *header = (RtcpHeader *)p;
    ASSERT_RETRUN_NULL(header->version == VERSON, LOGE("version(%d) is invalid", header->version));
    ASSERT_RETRUN_NULL(header->pt == RTCP_SDES, LOGE("pt(%u) != RTCP_SDES(%d)", header->pt, RTCP_SDES));
    ASSERT_RETRUN_NULL(length >= header->GetLength(),
                       LOGE("actual length(%zu) < rtcp field length(%d)", length, header->GetLength()));

    auto sdes = std::make_shared<RtcpSDES>();
    sdes->padding = header->padding;
    sdes->sourceCount = header->subtype;
    sdes->length = header->GetLength();

    auto end = p + length;
    p += sizeof(RtcpHeader);
    while (p < end) {
        SdesChunk chunk;
        auto chunkStart = p;
        chunk.ssrc = BS32(*(uint32_t *)p);
        p += sizeof(uint32_t);

        while (*p != RTCP_SDES_END && p < end) {
            SdesType type = (SdesType)*p++;
            size_t length = *p++;
            chunk.items.push_back({type, std::string((char *)p, length)});
            p += length;
        }
        sdes->sdesChunks.push_back(chunk);
        if ((p - chunkStart) % 4 != 0) {
            p = chunkStart + ALIGN32(p - chunkStart);
        }
    }

    assert(sdes->sourceCount == sdes->sdesChunks.size());
    return sdes;
}

std::shared_ptr<DataBuffer> RtcpSDES::Generate()
{

    size_t estimatedSize = sizeof(RtcpHeader);
    for (auto &chunk : sdesChunks) {
        size_t chunkSize = SSRC_SIZE;
        for (auto &item : chunk.items) {
            if (item.first <= RTCP_SDES_END || item.first > RTCP_SDES_PRIVATE) {
                continue;
            }
            chunkSize += 2;
            chunkSize += item.second.size();
        }
        estimatedSize += ALIGN32(chunkSize);
    }

    auto data = std::make_shared<DataBuffer>(estimatedSize);

    RtcpHeader *header = (RtcpHeader *)data->Data();
    header->pt = RTCP_SDES;
    header->subtype = sdesChunks.size();
    header->version = VERSON;
    header->padding = 0;
    data->SetSize(sizeof(RtcpHeader));

    for (auto &chunk : sdesChunks) {
        size_t chunkPoint = data->Size();
        uint32_t ssrc = BS32(chunk.ssrc);
        data->Append((char *)&ssrc, sizeof(uint32_t));
        for (auto &item : chunk.items) {
            if (item.first <= RTCP_SDES_END || item.first > RTCP_SDES_PRIVATE) {
                continue;
            }

            data->Append((uint8_t)item.first);
            char length = item.second.size();
            data->Append(length);
            data->Append(item.second);
        }

        size_t chunkLength = data->Size() - chunkPoint;
        if ((chunkLength) % 4 != 0) {
            size_t count = ALIGN32(chunkLength) - chunkLength;
            for (size_t i = 0; i < count; i++) {
                data->Append((uint8_t)0);
            }
        }
    }

    header->SetLength(data->Size());
    return data;
}

std::shared_ptr<RtcpBYE> RtcpBYE::Parse(const uint8_t *p, size_t length)
{
    ASSERT_RETRUN_NULL((p != nullptr && length >= (sizeof(RtcpHeader))), LOGE("invalid input param"));

    auto *header = (RtcpHeader *)p;
    ASSERT_RETRUN_NULL(header->version == VERSON, LOGE("version(%d) is invalid", header->version));
    ASSERT_RETRUN_NULL(header->pt == RTCP_BYE, LOGE("pt(%u) != RTCP_BYE(%d)", header->pt, RTCP_BYE));
    ASSERT_RETRUN_NULL(length >= header->GetLength(),
                       LOGE("actual length(%zu) < rtcp field length(%d)", length, header->GetLength()));

    auto bye = std::make_shared<RtcpBYE>();
    bye->padding = header->padding;
    bye->sourceCount = header->subtype;
    bye->length = header->GetLength();

    auto end = p + length;
    p += sizeof(RtcpHeader);

    ASSERT_RETRUN_NULL(p + bye->sourceCount * SSRC_SIZE <= end, LOGE("the size of all SSRCs > lenght"));
    for (size_t i = 0; i < bye->sourceCount; i++) {
        uint32_t ssrc = BS32(*(uint32_t *)p);
        bye->ssrcs.push_back(ssrc);
        p += SSRC_SIZE;
    }

    if (p < end) {
        uint16_t length = BS16(*(uint16_t *)p);
        p += 2;
        if (p + length <= end) {
            bye->reason = std::string((char *)p, length);
        }
        p += length;
    }

    return bye;
}

std::shared_ptr<DataBuffer> RtcpBYE::Generate()
{

    size_t totalSize = sizeof(RtcpHeader);
    totalSize += ssrcs.size() * SSRC_SIZE;
    if (!reason.empty()) {
        if ((reason.size() + 2) % 4 != 0) {
            padding = 1;
        } else {
            padding = 0;
        }
        totalSize += ALIGN32(reason.size() + 2);
    }

    auto data = std::make_shared<DataBuffer>(totalSize);
    data->SetSize(totalSize);

    RtcpHeader *header = (RtcpHeader *)data->Data();
    header->pt = RTCP_BYE;
    header->subtype = ssrcs.size();
    header->version = VERSON;
    header->padding = padding;

    header->SetLength(totalSize);

    auto p = data->Data() + sizeof(RtcpHeader);
    for (auto &ssrc : ssrcs) {
        *(uint32_t *)p = BS32(ssrc);
        p += SSRC_SIZE;
    }

    if (!reason.empty()) {
        *(uint16_t *)p = BS16((uint16_t)reason.size());
        p += 2;
        memcpy(p, reason.data(), reason.size());
    }

    return data;
}

std::shared_ptr<RtcpAPP> RtcpAPP::Parse(const uint8_t *p, size_t length)
{
    LOGE("unimplemented");
    return nullptr;
}

std::shared_ptr<DataBuffer> RtcpAPP::Generate()
{
    LOGE("unimplemented");
    return nullptr;
}

std::shared_ptr<RtcpRTPFB> RtcpRTPFB::Parse(const uint8_t *p, size_t length)
{
    LOGE("unimplemented");
    return nullptr;
}

std::shared_ptr<DataBuffer> RtcpRTPFB::Generate()
{
    LOGE("unimplemented");
    return nullptr;
}