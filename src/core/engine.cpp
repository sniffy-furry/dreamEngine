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
    module_loader_.update(dt);
    python_scripts_.update(dt);
}

bool Engine::start_python(const std::string& python_home, const std::string& script_directory) {
    if (!python_scripts_.start(python_home)) return false;
    return python_scripts_.run_directory(script_directory);
}

void Engine::update_python(double dt) {
    python_scripts_.update(dt);
}

bool Engine::load_external_modules(const std::string& directory) {
    external_host_ = {};
    external_host_.abi_version = DREAM_ENGINE_MODULE_ABI_VERSION;
    external_host_.log = host_log;
    external_host_.get_time_seconds = host_time;
    python_scripts_.scan(directory);
    return module_loader_.load_directory(directory, external_host_);
}

void Engine::shutdown() noexcept {
    module_loader_.shutdown();
    python_scripts_.shutdown();
    if (!initialized_) return;

    for (auto it = modules_.rbegin(); it != modules_.rend(); ++it) {
        (*it)->shutdown();
    }
    initialized_ = false;
}

} // namespace dream
