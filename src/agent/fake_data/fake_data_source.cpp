//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "fake_data_source.h"

bool FakeDataSource::Init()
{
    return MediaSource::Init();
}

bool FakeDataSource::Start()
{
    return MediaSource::Start();
}

void FakeDataSource::ReceiveDataLoop()
{
    char fakeData[33] = "the fox jumps over the lazy dog\n";
    auto fakeFrame = std::make_shared<Frame>();
    fakeFrame->Append(fakeData);
    fakeFrame->format = FRAME_FORMAT_AUDIO_BASE;
    while (running_) {
        DeliverFrame(fakeFrame);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
