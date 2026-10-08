#include "dream/core/engine.hpp"
#include <iostream>
int main() {
    dream::Engine engine({.application_name="DreamEngine Demo", .enable_validation=true});
    if (!engine.initialize()) { std::cerr << "Engine initialization failed\n"; return 1; }
    for (int i=0;i<60;++i) engine.update(1.0/60.0);
    engine.shutdown();
    std::cout << "DreamEngine core OK\n";
    return 0;
}
