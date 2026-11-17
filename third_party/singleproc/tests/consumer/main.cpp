#include <singleproc/singleproc.hpp>

#include <string>
#include <unistd.h>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 3;
    }

    const std::string key = "consumer-" + std::to_string(static_cast<long long>(::getpid()));
    const singleproc::Guard guard(key, argv[1]);
    if (guard.state() == singleproc::Guard::State::Error)
    {
        return 2;
    }
    return guard.is_primary() ? 0 : 1;
}
