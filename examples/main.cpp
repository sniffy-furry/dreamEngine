#include "dream/core/engine.hpp"
#include <iostream>

int main() {
    dream::Engine engine({.application_name="DreamEngine Demo", .enable_validation=true});
    if (!engine.load_external_modules("modules/tutorial")) {
        std::cerr << "External module loading failed\n";
        return 1;
    }
    if (engine.external_module_count() != 1) {
        std::cerr << "Expected one external module\n";
        return 1;
    }
    if (!engine.initialize()) {
        std::cerr << "Engine initialization failed\n";
        return 1;
    }
    for (int i = 0; i < 120; ++i) engine.update(1.0 / 60.0);
    engine.shutdown();
    std::cout << "DreamEngine core + hot module OK\n";
    return 0;
}
