#include "raw_file_sink.h"
#include "common/log.h"

RawFileSink::~RawFileSink()
{
    if (fileWriter_) {
        fileWriter_->Close();
    }
}

void RawFileSink::OnFrame(const std::shared_ptr<Frame> &frame)
{
    LOGD("recv frame size: %zu", frame->Size());
    fflush(stdout);
    if (fileWriter_) {
        fileWriter_->Write(frame->Data(), frame->Size());
        // fileWriter_->Flush();
    }
}

bool RawFileSink::Init()
{
    fileWriter_ = FileWriter::Open(fileName_);
    return fileWriter_ != nullptr;
}
