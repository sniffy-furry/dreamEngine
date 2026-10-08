#include "dream/core/engine.hpp"

namespace dream {

Engine::Engine(EngineConfig config) : config_(config) {}

Engine::~Engine() {
    shutdown();
}

bool Engine::add_module(std::unique_ptr<IEngineModule> module) {
    if (initialized_ || !module) return false;
    modules_.push_back(std::move(module));
    return true;
}

bool Engine::initialize() {
    if (initialized_) return false;

    std::size_t initialized_count = 0;
    for (auto& module : modules_) {
        if (!module->initialize(api_)) {
            while (initialized_count > 0) {
                --initialized_count;
                modules_[initialized_count]->shutdown();
            }
            return false;
        }
        ++initialized_count;
    }

    initialized_ = true;
    return true;
}

void Engine::update(double dt) {
    if (!initialized_) return;
    for (auto& module : modules_) module->update(dt);
}

void Engine::shutdown() noexcept {
    if (!initialized_) return;

    for (auto it = modules_.rbegin(); it != modules_.rend(); ++it) {
        (*it)->shutdown();
    }
    initialized_ = false;
}

} // namespace dream
