//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_FAKE_DATA_SOURCE_H
#define HALFWAY_MEDIA_FAKE_DATA_SOURCE_H

#include "../base/media_source.h"
#include <memory>
#include <thread>

enum class FakeDataType {
    CHARACTERS = 0,
};

class FakeDataSource : public MediaSource {
public:
    ~FakeDataSource() override = default;

    static std::shared_ptr<FakeDataSource> Create(FakeDataType type = FakeDataType::CHARACTERS)
    {
        return std::shared_ptr<FakeDataSource>(new FakeDataSource(type));
    }

    // impl MediaSource
    bool Init() override;
    bool Start() override;

protected:
    // impl MediaSource
    void ReceiveDataLoop() override;

private:
    explicit FakeDataSource(FakeDataType type = FakeDataType::CHARACTERS) : type_(type) {}

private:
    FakeDataType type_;
};

#endif // HALFWAY_MEDIA_FAKE_DATA_SOURCE_H
