#pragma once

#ifndef QESingleton_H
#define QESingleton_H

#include <memory>
#include <mutex>

template <typename T>
class QESingleton
{
public:
    // Singleton instances cannot be copied or assigned.
    QESingleton(const QESingleton&) = delete;
    QESingleton& operator=(const QESingleton&) = delete;

    static T* getInstance()
    {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance)
        {
            instance.reset(new T());
        }

        return instance.get();
    }

    static void ResetInstance()
    {
        std::unique_ptr<T> instanceToDestroy;
        {
            std::lock_guard<std::mutex> lock(instanceMutex);
            instanceToDestroy = std::move(instance);
        }

        // Destroy outside the mutex in case T accesses another singleton from
        // its destructor.
        instanceToDestroy.reset();
    }

protected:
    QESingleton() = default;
    virtual ~QESingleton() = default; // Destructor virtual

private:
    inline static std::mutex instanceMutex{};
    inline static std::unique_ptr<T> instance{};
};

namespace QE
{
    using ::QESingleton;
} // namespace QE
// QE namespace aliases
#endif // !QESingleton_H
