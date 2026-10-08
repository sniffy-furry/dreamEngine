#pragma once
#include "event_bus.hpp"
#include "module.hpp"
#include "module_loader.hpp"
#include "python_scripting.hpp"
#include "registry.hpp"
#include "types.hpp"
#include <memory>
#include <string>
#include <vector>

namespace dream {

class EngineAPI final {
public:
    Registry& services() noexcept { return registry_; }
    EventBus& events() noexcept { return events_; }

private:
    Registry registry_;
    EventBus events_;
    friend class Engine;
};

class Engine final {
public:
    explicit Engine(EngineConfig config = {});
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool add_module(std::unique_ptr<IEngineModule> module);
    bool initialize();
    void update(double dt);
    bool load_external_modules(const std::string& directory);
    void shutdown() noexcept;
    std::size_t external_module_count() const noexcept { return module_loader_.loaded_count(); }
    const PythonScriptManager& python_scripts() const noexcept { return python_scripts_; }

    EngineAPI& api() noexcept { return api_; }
    const EngineConfig& config() const noexcept { return config_; }

private:
    EngineConfig config_;
    EngineAPI api_;
    std::vector<std::unique_ptr<IEngineModule>> modules_;
    bool initialized_ = false;
    ModuleLoader module_loader_;
    PythonScriptManager python_scripts_;
};

} // namespace dream
