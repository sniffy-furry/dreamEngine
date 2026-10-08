#pragma once
#include <memory>
#include <string>
#include <vector>

namespace dream {

class PropertyRegistry;
class UiModel;

class PythonScriptManager final {
public:
    PythonScriptManager();
    ~PythonScriptManager();
    PythonScriptManager(const PythonScriptManager&) = delete;
    PythonScriptManager& operator=(const PythonScriptManager&) = delete;

    void bind(PropertyRegistry* props, UiModel* ui) noexcept;
    bool start(const std::string& python_home);
    // Loads every *.py in `directory`, each in its own module namespace.
    bool run_directory(const std::string& directory);
    // Calls on_stop() on loaded scripts and drops them (interpreter stays alive).
    void unload() noexcept;
    bool update(double dt);
    void shutdown() noexcept;

    bool scan(const std::string& directory);
    const std::vector<std::string>& scripts() const noexcept { return scripts_; }
    bool running() const noexcept { return running_; }

private:
    struct Loaded;
    std::vector<std::string> scripts_;
    std::vector<std::unique_ptr<Loaded>> loaded_;
    bool running_ = false;
};

} // namespace dream
