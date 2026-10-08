#include "dream/core/module_loader.hpp"
#include <dlfcn.h>
#include <dirent.h>
#if defined(__ANDROID__)
#include <android/log.h>
#endif
#include "dream/core/log.hpp"
#include <cstring>
#include <algorithm>

namespace dream {

namespace {
constexpr const char* kTag = "DreamEngine";
void log_line(int level, const char* message) {
    (void)kTag;
    log_push(level, message ? message : "");
}
}

struct ModuleLoader::Loaded {
    void* handle = nullptr;
    DreamEngineModule* module = nullptr;
    DreamModuleDestroyFn destroy = nullptr;
    std::string path;
    const DreamRenderModule* render = nullptr;
    bool gl_inited = false;
};

ModuleLoader::ModuleLoader() = default;
ModuleLoader::~ModuleLoader() { shutdown(); }

bool ModuleLoader::load_directory(const std::string& directory, const DreamEngineHostAPI& host) {
    shutdown();
    host_ = &host;
    DIR* dir = opendir(directory.c_str());
    if (!dir) return false;

    std::vector<std::string> paths;
    while (auto* entry = readdir(dir)) {
        const char* n = entry->d_name;
        const std::size_t len = std::strlen(n);
        if (len > 3 && std::strcmp(n + len - 3, ".so") == 0) {
            paths.emplace_back(directory + "/" + n);
        }
    }
    closedir(dir);
    std::sort(paths.begin(), paths.end());

    bool all_ok = true;
    for (const auto& path : paths) {
        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            all_ok = false;
            const char* err = dlerror();
            log_line(6, (std::string("dlopen failed: ") + path + ": " + (err ? err : "?")).c_str());
            continue;
        }
        auto get_desc = reinterpret_cast<DreamModuleGetDescriptorFn>(dlsym(handle, "dream_module_get_descriptor"));
        auto create = reinterpret_cast<DreamModuleCreateFn>(dlsym(handle, "dream_module_create"));
        auto destroy = reinterpret_cast<DreamModuleDestroyFn>(dlsym(handle, "dream_module_destroy"));
        if (!get_desc || !create || !destroy) {
            log_line(6, (path + ": missing dream_module_* exports").c_str());
            dlclose(handle);
            all_ok = false;
            continue;
        }
        const auto* desc = get_desc();
        if (!desc || desc->abi_version != DREAM_ENGINE_MODULE_ABI_VERSION || !desc->id || !desc->name) {
            log_line(6, (path + ": bad descriptor / ABI mismatch").c_str());
            dlclose(handle);
            all_ok = false;
            continue;
        }
        DreamEngineModule* module = create(&host);
        if (!module || !module->initialize || !module->update || !module->shutdown || !module->initialize(module, &host)) {
            log_line(6, (path + ": module initialize failed").c_str());
            if (module && destroy) destroy(module);
            dlclose(handle);
            all_ok = false;
            continue;
        }
        auto loaded = std::make_unique<Loaded>();
        loaded->handle = handle;
        loaded->module = module;
        loaded->destroy = destroy;
        loaded->path = path;
        if (auto get_render = reinterpret_cast<DreamModuleGetRenderFn>(dlsym(handle, "dream_module_get_render")))
            loaded->render = get_render();
        if (gl_ready_ && loaded->render && loaded->render->gl_init) {   // hot-loaded while a surface exists
            loaded->render->gl_init(const_cast<DreamRenderModule*>(loaded->render), &host, gl_w_, gl_h_);
            loaded->gl_inited = true;
        }
        loaded_.push_back(std::move(loaded));
        log_line(4, ("loaded native module: " + path).c_str());
    }
    return all_ok;
}

void ModuleLoader::shutdown() noexcept {
    for (auto it = loaded_.rbegin(); it != loaded_.rend(); ++it) {
        auto& m = *it;
        if (gl_ready_ && m->gl_inited && m->render && m->render->gl_shutdown)
            m->render->gl_shutdown(const_cast<DreamRenderModule*>(m->render));
        if (m->module) m->module->shutdown(m->module);
        if (m->destroy && m->module) m->destroy(m->module);
        if (m->handle) dlclose(m->handle);
    }
    loaded_.clear();
}

void ModuleLoader::update(double dt) noexcept {
    for (auto& m : loaded_) {
        if (m->module && m->module->update) m->module->update(m->module, dt);
    }
}

void ModuleLoader::gl_init_all(int w, int h) noexcept {
    gl_ready_ = true; gl_w_ = w; gl_h_ = h;
    for (auto& m : loaded_)
        if (m->render && m->render->gl_init && host_ && !m->gl_inited) {
            m->render->gl_init(const_cast<DreamRenderModule*>(m->render), host_, w, h);
            m->gl_inited = true;
        }
}
void ModuleLoader::gl_resize_all(int w, int h) noexcept {
    gl_w_ = w; gl_h_ = h;
    for (auto& m : loaded_)
        if (m->gl_inited && m->render && m->render->gl_resize)
            m->render->gl_resize(const_cast<DreamRenderModule*>(m->render), w, h);
}
void ModuleLoader::gl_draw_all(double dt) noexcept {
    for (auto& m : loaded_)
        if (m->gl_inited && m->render && m->render->gl_draw)
            m->render->gl_draw(const_cast<DreamRenderModule*>(m->render), dt);
}
void ModuleLoader::gl_shutdown_all() noexcept {
    for (auto& m : loaded_)
        if (m->gl_inited && m->render && m->render->gl_shutdown) {
            m->render->gl_shutdown(const_cast<DreamRenderModule*>(m->render));
            m->gl_inited = false;
        }
    gl_ready_ = false;
}

std::size_t ModuleLoader::loaded_count() const noexcept { return loaded_.size(); }

} // namespace dream
