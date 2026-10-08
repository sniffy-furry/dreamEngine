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

    // Render hooks (engine thread, GL context current).
    void gl_init_all(int width, int height) noexcept;
    void gl_resize_all(int width, int height) noexcept;
    void gl_draw_all(double dt) noexcept;
    void gl_shutdown_all() noexcept;

private:
    struct Loaded;
    std::vector<std::unique_ptr<Loaded>> loaded_;
    const DreamEngineHostAPI* host_ = nullptr;
    bool gl_ready_ = false;
    int gl_w_ = 0, gl_h_ = 0;
};

} // namespace dream
