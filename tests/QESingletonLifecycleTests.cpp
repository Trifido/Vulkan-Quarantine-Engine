#include <QESingleton.h>

#include <array>
#include <atomic>
#include <thread>

class LifecycleTestService final : public QESingleton<LifecycleTestService>
{
private:
    friend class QESingleton<LifecycleTestService>;

    LifecycleTestService()
        : generation(++constructionCount)
    {
    }

public:
    ~LifecycleTestService() override
    {
        ++destructionCount;
    }

    int generation = 0;

    inline static std::atomic<int> constructionCount{ 0 };
    inline static std::atomic<int> destructionCount{ 0 };
};

int main()
{
    constexpr size_t threadCount = 8;
    std::array<LifecycleTestService*, threadCount> instances{};
    std::array<std::thread, threadCount> threads;

    for (size_t i = 0; i < threadCount; ++i)
    {
        threads[i] = std::thread([&instances, i]()
            {
                instances[i] = LifecycleTestService::getInstance();
            });
    }

    for (auto& thread : threads)
        thread.join();

    if (instances[0] == nullptr)
        return 1;

    for (const auto* instance : instances)
    {
        if (instance != instances[0])
            return 2;
    }

    if (instances[0]->generation != 1 || LifecycleTestService::constructionCount.load() != 1)
        return 3;

    LifecycleTestService::ResetInstance();
    if (LifecycleTestService::destructionCount.load() != 1)
        return 4;

    auto* recreated = LifecycleTestService::getInstance();
    if (recreated == nullptr || recreated->generation != 2 || LifecycleTestService::constructionCount.load() != 2)
        return 5;

    LifecycleTestService::ResetInstance();
    if (LifecycleTestService::destructionCount.load() != 2)
        return 6;

    // Reset is deliberately idempotent.
    LifecycleTestService::ResetInstance();
    if (LifecycleTestService::destructionCount.load() != 2)
        return 7;

    return 0;
}
