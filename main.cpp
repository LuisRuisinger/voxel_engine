#include "globalstate.h"

auto main() -> i32 {
    Engine::init();
    Engine::run();
    Engine::shutdown();

    std::exit(EXIT_SUCCESS);
}