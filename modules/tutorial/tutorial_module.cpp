#include "tutorial_module.hpp"
#include <iostream>

namespace dream::tutorial {

struct TutorialTick {
    double dt;
    double elapsed;
};

const char* TutorialModule::name() const noexcept {
    return "tutorial";
}

bool TutorialModule::initialize(EngineAPI& api) {
    api_ = &api;
    std::cout << "[tutorial] initialized\n";
    return true;
}

void TutorialModule::update(double dt) {
    elapsed_ += dt;

    // Deliberately tiny demo: modules communicate through the central EventBus.
    if (api_ && elapsed_ >= 1.0) {
        api_->events().publish(TutorialTick{elapsed_, elapsed_});
        elapsed_ = 0.0;
    }
}

void TutorialModule::shutdown() noexcept {
    std::cout << "[tutorial] shutdown\n";
    api_ = nullptr;
}

std::unique_ptr<IEngineModule> create() {
    return std::make_unique<TutorialModule>();
}

} // namespace dream::tutorial
