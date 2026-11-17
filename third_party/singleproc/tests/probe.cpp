#include <singleproc/singleproc.hpp>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        return 3;
    }

    const singleproc::Guard guard(argv[1], argv[2]);
    switch (guard.state())
    {
    case singleproc::Guard::State::Primary:
        return 0;
    case singleproc::Guard::State::AlreadyRunning:
        return 1;
    case singleproc::Guard::State::Error:
        return 2;
    }
    return 2;
}
