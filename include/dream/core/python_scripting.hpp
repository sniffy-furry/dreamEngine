#pragma once
#include <string>
#include <vector>

namespace dream {

class PythonScriptManager final {
public:
    PythonScriptManager() = default;
    ~PythonScriptManager();
    PythonScriptManager(const PythonScriptManager&) = delete;
    PythonScriptManager& operator=(const PythonScriptManager&) = delete;

    bool start(const std::string& python_home);
    bool run_directory(const std::string& directory);
    bool update(double dt);
    void shutdown() noexcept;

    bool scan(const std::string& directory);
    const std::vector<std::string>& scripts() const noexcept { return scripts_; }
    bool running() const noexcept { return running_; }

private:
    std::vector<std::string> scripts_;
    bool running_ = false;
};

} // namespace dream
