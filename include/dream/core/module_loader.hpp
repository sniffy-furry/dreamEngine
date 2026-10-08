#pragma once
#include "module_api.h"
#include <memory>
#include <string>
#include <vector>

namespace dream {

class ModuleLoader final {
public:
    ModuleLoader();
    ~ModuleLoader();
    ModuleLoader(const ModuleLoader&) = delete;
    ModuleLoader& operator=(const ModuleLoader&) = delete;

    bool load_directory(const std::string& directory, const DreamEngineHostAPI& host);
    void shutdown() noexcept;
    std::size_t loaded_count() const noexcept;
    void update(double dt) noexcept;

private:
    struct Loaded;
    std::vector<std::unique_ptr<Loaded>> loaded_;
};

} // namespace dream
