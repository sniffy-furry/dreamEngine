#include "dream/core/engine.hpp"
#if defined(__ANDROID__)
#include <android/log.h>
#endif
#include <chrono>


namespace {
void host_log(int level, const char* message) {
#if defined(__ANDROID__)
    __android_log_write(level, "DreamEngine", message ? message : "");
#else
    (void)level; (void)message;
#endif
}
double host_time() {
    using clock = std::chrono::steady_clock;
    static const auto start = clock::now();
    return std::chrono::duration<double>(clock::now() - start).count();
}
}

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

bool Engine::load_external_modules(const std::string& directory) {
    DreamEngineHostAPI host{};
    host.abi_version = DREAM_ENGINE_MODULE_ABI_VERSION;
    host.log = host_log;
    host.get_time_seconds = host_time;
    python_scripts_.scan(directory);
    return module_loader_.load_directory(directory, host);
}

void Engine::shutdown() noexcept {
    module_loader_.shutdown();
    if (!initialized_) return;

    for (auto it = modules_.rbegin(); it != modules_.rend(); ++it) {
        (*it)->shutdown();
    }
    initialized_ = false;
}

} // namespace dream
