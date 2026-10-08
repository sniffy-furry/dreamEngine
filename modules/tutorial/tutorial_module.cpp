#include "dream/core/module_api.h"
#include <cstdio>
#include <cstdlib>
#if defined(__ANDROID__)
#include <android/log.h>
#endif

struct TutorialState { double elapsed = 0.0; const DreamEngineHostAPI* host = nullptr; };

static const DreamEngineModuleDescriptor kDescriptor{
    DREAM_ENGINE_MODULE_ABI_VERSION, 1u, "tutorial", "DreamEngine Tutorial"
};

static int initialize(DreamEngineModule* self, const DreamEngineHostAPI* host) {
    auto* s = static_cast<TutorialState*>(self->user_data);
    s->host = host;
    if (host && host->log) host->log(4, "tutorial module initialized");
    return 1;
}
static void update(DreamEngineModule* self, double dt) {
    auto* s = static_cast<TutorialState*>(self->user_data);
    s->elapsed += dt;
    if (s->elapsed >= 1.0) {
        s->elapsed -= 1.0;
        if (s->host && s->host->log) s->host->log(4, "tutorial tick");
    }
}
static void shutdown(DreamEngineModule* self) {
    auto* s = static_cast<TutorialState*>(self->user_data);
    if (s->host && s->host->log) s->host->log(4, "tutorial module shutdown");
    s->host = nullptr;
}

extern "C" DREAM_MODULE_EXPORT const DreamEngineModuleDescriptor* dream_module_get_descriptor() { return &kDescriptor; }
extern "C" DREAM_MODULE_EXPORT DreamEngineModule* dream_module_create(const DreamEngineHostAPI* host) {
    auto* state = new TutorialState{};
    auto* module = new DreamEngineModule{};
    module->user_data = state;
    module->initialize = initialize;
    module->update = update;
    module->shutdown = shutdown;
    if (host) state->host = host;
    return module;
}
extern "C" DREAM_MODULE_EXPORT void dream_module_destroy(DreamEngineModule* module) {
    if (!module) return;
    delete static_cast<TutorialState*>(module->user_data);
    delete module;
}
