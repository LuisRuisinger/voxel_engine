#include "globalstate.h"

auto main() -> i32 {
    auto engine = Engine {};
    engine.init();
    engine.run();
    engine.shutdown();

    std::exit(EXIT_SUCCESS);
}