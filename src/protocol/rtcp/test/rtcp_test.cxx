//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "../rtcp.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

// Real-time Transport Control Protocol (Sender Report)
//     [Stream setup by RTSP (frame 26372)]
//     10.. .... = Version: RFC 1889 Version (2)
//     ..0. .... = Padding: False
//     ...0 0000 = Reception report count: 0
//     Packet type: Sender Report (200)
//     Length: 6 (28 bytes)
//     Sender SSRC: 0x60ef152b (1626281259)
//     Timestamp, MSW: 3918177052 (0xe98aa31c)
//     Timestamp, LSW: 1976299136 (0x75cbee80)
//     [MSW and LSW as NTP timestamp: Feb 29, 2024 06:30:52.460142999 UTC]
//     RTP timestamp: 672929340
//     Sender's packet count: 128
//     Sender's octet count: 87738

uint8_t sr[28] = {0x80, 0xc8, 0x00, 0x06, 0x60, 0xef, 0x15, 0x2b, 0xe9, 0x8a, 0xa3, 0x1c, 0x75, 0xcb,
                  0xee, 0x80, 0x28, 0x1c, 0x16, 0x3c, 0x00, 0x00, 0x00, 0x80, 0x00, 0x01, 0x56, 0xba};

// Real-time Transport Control Protocol (Source description)
//     [Stream setup by RTSP (frame 26372)]
//     10.. .... = Version: RFC 1889 Version (2)
//     ..0. .... = Padding: False
//     ...0 0001 = Source count: 1
//     Packet type: Source description (202)
//     Length: 5 (24 bytes)
//     Chunk 1, SSRC/CSRC 0x60EF152B
//         Identifier: 0x60ef152b (1626281259)
//         SDES items
//             Type: CNAME (user and domain) (1)
//             Length: 11
//             Text: shao-lenovo
//             Type: END (0)
//     [RTCP frame length check: OK - 52 bytes]

uint8_t sdes[24] = {0x81, 0xca, 0x00, 0x05, 0x60, 0xef, 0x15, 0x2b, 0x01, 0x0b, 0x73, 0x68,
                    0x61, 0x6f, 0x2d, 0x6c, 0x65, 0x6e, 0x6f, 0x76, 0x6f, 0x00, 0x00, 0x00};

// Real-time Transport Control Protocol (Receiver Report)
//     10.. .... = Version: RFC 1889 Version (2)
//     ..0. .... = Padding: False
//     ...0 0001 = Reception report count: 1
//     Packet type: Receiver Report (201)
//     Length: 7 (32 bytes)
//     Sender SSRC: 0x42298473 (1110017139)
//     Source 1
//         Identifier: 0x60ef152b (1626281259)
//         SSRC contents
//             Fraction lost: 0 / 256
//             Cumulative number of packets lost: -1
//         Extended highest sequence number received: 113775
//             Sequence number cycles count: 1
//             Highest sequence number received: 48239
//         Interarrival jitter: 65
//         Last SR timestamp: 2736732544 (0xa31f3980)
//         Delay since last SR timestamp: 44467 (678 milliseconds)

uint8_t rr[32] = {0x81, 0xc9, 0x00, 0x07, 0x42, 0x29, 0x84, 0x73, 0x60, 0xef, 0x15, 0x2b, 0x00, 0xff, 0xff, 0xff,
                  0x00, 0x01, 0xbc, 0x6f, 0x00, 0x00, 0x00, 0x41, 0xa3, 0x1f, 0x39, 0x80, 0x00, 0x00, 0xad, 0xb3};

// Real-time Transport Control Protocol (Goodbye)
//     10.. .... = Version: RFC 1889 Version (2)
//     ..0. .... = Padding: False
//     ...0 0001 = Source count: 1
//     Packet type: Goodbye (203)
//     Length: 1 (8 bytes)
//     Identifier: 0x42298473 (1110017139)
//     [RTCP frame length check: OK - 40 bytes]

uint8_t bye[8] = {0x81, 0xcb, 0x00, 0x01, 0x42, 0x29, 0x84, 0x73};

int main(int argc, char **argv)
{
    printf("RTCP test\n");
    auto rtcpSR = RtcpSR::Parse(sr, (size_t)sizeof(sr));
    assert(rtcpSR != nullptr);
    assert(rtcpSR->ssrc == 0x60ef152b);
    assert(rtcpSR->senderInfo.nptTsMSW == 3918177052);
    assert(rtcpSR->senderInfo.nptTsLSW == 1976299136);
    assert(rtcpSR->senderInfo.rtpTs == 672929340);
    assert(rtcpSR->senderInfo.packetCount == 128);
    assert(rtcpSR->senderInfo.octetCount == 87738);

    auto genSR = rtcpSR->Generate();
    assert(genSR != nullptr);
    assert(std::string((char *)&sr[0], sizeof(sr)) == std::string((char *)genSR->Data(), genSR->Size()));
    printf("RTCP RtcpSR test pass\n");

    auto rtcpRR = RtcpRR::Parse(rr, (size_t)sizeof(rr));
    assert(rtcpRR != nullptr);
    assert(rtcpRR->ssrc == 0x42298473);
    assert(rtcpRR->reportBlocks.size() == 1);
    assert(rtcpRR->reportBlocks[0].ssrc == 0x60ef152b);
    assert(rtcpRR->reportBlocks[0].lostPacketCount == 0xffffff);
    assert(rtcpRR->reportBlocks[0].highestSequence == 113775);
    assert(rtcpRR->reportBlocks[0].jitter == 65);
    assert(rtcpRR->reportBlocks[0].lastSR == 2736732544);
    assert(rtcpRR->reportBlocks[0].delaySinceLastSR == 44467);

    auto genRR = rtcpRR->Generate();
    assert(genRR != nullptr);
    assert(std::string((char *)&rr[0], sizeof(rr)) == std::string((char *)genRR->Data(), genRR->Size()));
    printf("RTCP RtcpRR test pass\n");

    auto rtcpSDES = RtcpSDES::Parse(sdes, sizeof(sdes));
    assert(rtcpSDES != nullptr);
    assert(rtcpSDES->sourceCount == rtcpSDES->sdesChunks.size());
    assert(rtcpSDES->sdesChunks[0].ssrc == 0x60ef152b);
    assert(rtcpSDES->sdesChunks[0].items[0].second == "shao-lenovo");

    auto genSDES = rtcpSDES->Generate();
    assert(genSDES != nullptr);
    assert(std::string((char *)&sdes[0], sizeof(sdes)) == std::string((char *)genSDES->Data(), genSDES->Size()));
    printf("RTCP RtcpSDES test pass\n");

    auto rtcpBYE = RtcpBYE::Parse(bye, sizeof(bye));
    assert(rtcpBYE != nullptr);
    assert(rtcpBYE->sourceCount == rtcpBYE->ssrcs.size());

    auto genBYE = rtcpBYE->Generate();
    assert(genBYE != nullptr);
    assert(std::string((char *)&bye[0], sizeof(bye)) == std::string((char *)genBYE->Data(), genBYE->Size()));
    printf("RTCP RtcpBYE test pass\n");
}