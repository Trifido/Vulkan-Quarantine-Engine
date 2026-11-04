#include <Helpers/QETeardownStack.h>

#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

int main()
{
    std::vector<int> order;
    std::exception_ptr failure;

    {
        QE::QETeardownStack teardown;
        teardown.Add("first", [&order]() { order.push_back(1); });
        teardown.Add("throwing", [&order]()
            {
                order.push_back(2);
                throw std::runtime_error("expected teardown failure");
            });
        teardown.Add("last", [&order]() { order.push_back(3); });

        teardown.Close(failure);
        teardown.Close(failure);

        if (!teardown.Empty())
            return 1;
    }

    if (order != std::vector<int>{ 3, 2, 1 })
        return 2;

    if (!failure)
        return 3;

    try
    {
        std::rethrow_exception(failure);
    }
    catch (const std::runtime_error& error)
    {
        if (std::string(error.what()) != "expected teardown failure")
            return 4;
    }
    catch (...)
    {
        return 5;
    }

    bool destructorRan = false;
    {
        QE::QETeardownStack teardown;
        teardown.Add("destructor", [&destructorRan]() { destructorRan = true; });
    }

    return destructorRan ? 0 : 6;
}
