#include "agent/base/media_frame_pipeline.h"
#include "agent/base/media_source.h"
#include "agent/fake_data/fake_data_source.h"
#include "agent/media_file/media_file_source.h"
#include "agent/media_file/raw_file_sink.h"
#include <iostream>
#include <string>

#include <unistd.h>

int main(int argc, char **argv)
{
    printf("Hello Halfway, Built at %s on %s.\n", __TIME__, __DATE__);

    auto raw_file_sink = RawFileSink::Create("outFile.h264");
    std::cout << raw_file_sink.get() << std::endl;
    raw_file_sink->Init();

    auto fakeSrc = MediaFileSource::Create("../assets/Sample.mp4");
    fakeSrc->AddVideoSink(raw_file_sink);
    fakeSrc->Init();
    fakeSrc->Start();

    sleep(20);
    fakeSrc->Stop();
    printf("end.\n");
    return 0;
}