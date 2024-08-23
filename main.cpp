#include "globalstate.h"
#include "util/singleton.h"
#include "util/log.h"
#include "util/assert.h"

auto main() -> i32 {
    Engine::init();
    Engine::run();

    std::exit(EXIT_SUCCESS);
}