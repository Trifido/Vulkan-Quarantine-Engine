#pragma once

#include <exception>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace QE
{
class QETeardownStack final
{
public:
    using Action = std::function<void()>;

    QETeardownStack() = default;
    ~QETeardownStack() noexcept
    {
        std::exception_ptr ignoredFailure;
        Close(ignoredFailure);
    }

    QETeardownStack(const QETeardownStack&) = delete;
    QETeardownStack& operator=(const QETeardownStack&) = delete;
    QETeardownStack(QETeardownStack&&) = delete;
    QETeardownStack& operator=(QETeardownStack&&) = delete;

    void Add(std::string name, Action action)
    {
        entries.push_back(Entry{ std::move(name), std::move(action) });
    }

    void Close(std::exception_ptr& firstFailure) noexcept
    {
        while (!entries.empty())
        {
            Entry entry = std::move(entries.back());
            entries.pop_back();

            try
            {
                if (entry.action)
                    entry.action();
            }
            catch (...)
            {
                if (!firstFailure)
                    firstFailure = std::current_exception();
            }
        }
    }

    [[nodiscard]] bool Empty() const noexcept
    {
        return entries.empty();
    }

private:
    struct Entry
    {
        std::string name;
        Action action;
    };

    std::vector<Entry> entries;
};
} // namespace QE
