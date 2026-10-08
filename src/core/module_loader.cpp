#include "dream/core/module_loader.hpp"
#include <dlfcn.h>
#include <dirent.h>
#if defined(__ANDROID__)
#include <android/log.h>
#endif
#include <cstring>
#include <algorithm>

namespace dream {

namespace {
constexpr const char* kTag = "DreamEngine";
void log_line(int level, const char* message) {
#if defined(__ANDROID__)
    __android_log_write(level, kTag, message ? message : "");
#else
    (void)level; (void)message;
#endif
}
}

struct ModuleLoader::Loaded {
    void* handle = nullptr;
    DreamEngineModule* module = nullptr;
    DreamModuleDestroyFn destroy = nullptr;
    std::string path;
};

ModuleLoader::ModuleLoader() = default;
ModuleLoader::~ModuleLoader() { shutdown(); }

bool ModuleLoader::load_directory(const std::string& directory, const DreamEngineHostAPI& host) {
    shutdown();
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
            log_line(6, dlerror());
            continue;
        }
        auto get_desc = reinterpret_cast<DreamModuleGetDescriptorFn>(dlsym(handle, "dream_module_get_descriptor"));
        auto create = reinterpret_cast<DreamModuleCreateFn>(dlsym(handle, "dream_module_create"));
        auto destroy = reinterpret_cast<DreamModuleDestroyFn>(dlsym(handle, "dream_module_destroy"));
        if (!get_desc || !create || !destroy) {
            dlclose(handle);
            all_ok = false;
            continue;
        }
        const auto* desc = get_desc();
        if (!desc || desc->abi_version != DREAM_ENGINE_MODULE_ABI_VERSION || !desc->id || !desc->name) {
            dlclose(handle);
            all_ok = false;
            continue;
        }
        DreamEngineModule* module = create(&host);
        if (!module || !module->initialize || !module->update || !module->shutdown || !module->initialize(module, &host)) {
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
        loaded_.push_back(std::move(loaded));
    }
    return all_ok;
}

void ModuleLoader::shutdown() noexcept {
    for (auto it = loaded_.rbegin(); it != loaded_.rend(); ++it) {
        auto& m = *it;
        if (m->module) m->module->shutdown(m->module);
        if (m->destroy && m->module) m->destroy(m->module);
        if (m->handle) dlclose(m->handle);
    }
    loaded_.clear();
}

std::size_t ModuleLoader::loaded_count() const noexcept { return loaded_.size(); }

} // namespace dream
