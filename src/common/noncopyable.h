//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_NONCOPYABLE_H
#define HALFWAY_MEDIA_NONCOPYABLE_H

class NonCopyable {
protected:
    NonCopyable() = default;
    virtual ~NonCopyable() = default;

public:
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
    NonCopyable(NonCopyable &&) = delete;
    NonCopyable &operator=(NonCopyable &&) = delete;
};

#endif // HALFWAY_MEDIA_NONCOPYABLE_H