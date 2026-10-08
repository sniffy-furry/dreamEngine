#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { DREAM_ENGINE_MODULE_ABI_VERSION = 1u };

typedef struct DreamEngineHostAPI DreamEngineHostAPI;
typedef struct DreamEngineModule DreamEngineModule;

typedef void (*DreamLogFn)(int level, const char* message);
typedef double (*DreamGetTimeFn)(void);

typedef struct DreamEngineHostAPI {
    uint32_t abi_version;
    void* user_data;
    DreamLogFn log;
    DreamGetTimeFn get_time_seconds;
} DreamEngineHostAPI;

typedef struct DreamEngineModuleDescriptor {
    uint32_t abi_version;
    uint32_t module_version;
    const char* id;
    const char* name;
} DreamEngineModuleDescriptor;

typedef struct DreamEngineModule {
    void* user_data;
    int (*initialize)(DreamEngineModule* self, const DreamEngineHostAPI* host);
    void (*update)(DreamEngineModule* self, double dt);
    void (*shutdown)(DreamEngineModule* self);
} DreamEngineModule;

typedef const DreamEngineModuleDescriptor* (*DreamModuleGetDescriptorFn)(void);
typedef DreamEngineModule* (*DreamModuleCreateFn)(const DreamEngineHostAPI* host);
typedef void (*DreamModuleDestroyFn)(DreamEngineModule* module);

#ifdef __cplusplus
}
#endif

#if defined(__GNUC__) || defined(__clang__)
#define DREAM_MODULE_EXPORT __attribute__((visibility("default")))
#else
#define DREAM_MODULE_EXPORT
#endif

#ifdef __cplusplus
namespace dream {
inline constexpr uint32_t kModuleAbiVersion = DREAM_ENGINE_MODULE_ABI_VERSION;
}
#endif
