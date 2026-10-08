#include "dream/core/engine.hpp"
#include "../modules/tutorial/tutorial_module.hpp"
#include <iostream>

struct FrameEvent {
    double dt;
};

int main() {
    dream::Engine engine({
        .application_name = "DreamEngine Demo",
        .enable_validation = true
    });

    engine.add_module(dream::tutorial::create());

    auto subscription = engine.api().events().subscribe<dream::tutorial::TutorialTick>(
        [](const dream::tutorial::TutorialTick& e) {
            std::cout << "[event] tutorial tick: " << e.dt << "\n";
        });

    if (!engine.initialize()) {
        std::cerr << "Engine initialization failed\n";
        return 1;
    }

    for (int i = 0; i < 300; ++i) {
        engine.update(1.0 / 60.0);
    }

    engine.api().events().unsubscribe(subscription);
    engine.shutdown();
    return 0;
}
