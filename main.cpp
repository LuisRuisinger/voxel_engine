#include "globalstate.h"

auto main() -> i32 {
    Engine::init();
    Engine::run();

    std::exit(EXIT_SUCCESS);
}