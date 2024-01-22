//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_SINGLETON_H
#define HALFWAY_MEDIA_SINGLETON_H

#include "noncopyable.h"
#include <memory>
#include <mutex>

template <typename T>
class Singleton : public NonCopyable {
public:
    static std::shared_ptr<T> GetInstance();
    static void DestroyInstance();

protected:
private:
    static std::shared_ptr<T> instance_;
    static std::mutex mutex_;
};

template <typename T>
std::shared_ptr<T> Singleton<T>::instance_ = nullptr;

template <typename T>
std::mutex Singleton<T>::mutex_;

template <typename T>
std::shared_ptr<T> Singleton<T>::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = std::shared_ptr<T>(new T());
        }
    }

    return instance_;
}

template <typename T>
void Singleton<T>::DestroyInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ != nullptr) {
        instance_.reset();
    }
}

#endif // HALFWAY_MEDIA_SINGLETON_H