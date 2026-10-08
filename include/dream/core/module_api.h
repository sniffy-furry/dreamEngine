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

// Properties: register once (strings), then use the uint32 handle every frame (no strings, no hashing).
typedef struct DreamPropertyAPI {
    uint32_t (*register_float)(const char* name, float def, float min, float max, const char* category);
    uint32_t (*register_bool)(const char* name, int def, const char* category);
    uint32_t (*find)(const char* name);          // 0xFFFFFFFF if missing
    float (*get)(uint32_t id);
    void (*set)(uint32_t id, float value);
} DreamPropertyAPI;

// UI model: describe panels; the engine/UI module decides how to draw them.
typedef struct DreamUiAPI {
    uint32_t (*panel)(const char* name);          // get-or-create
    void (*slider)(uint32_t panel, uint32_t prop, const char* label_or_null);
    void (*toggle)(uint32_t panel, uint32_t prop, const char* label_or_null);
    void (*value)(uint32_t panel, uint32_t prop, const char* label_or_null);   // read-only readout
    void (*label)(uint32_t panel, const char* text);
    void (*query)(uint32_t panel, const char* module, const char* category, const char* tag); // NULL = any
} DreamUiAPI;

// New fields are only ever APPENDED, so modules built against older headers keep working.
typedef struct DreamEngineHostAPI {
    uint32_t abi_version;
    void* user_data;
    DreamLogFn log;
    DreamGetTimeFn get_time_seconds;
    const DreamPropertyAPI* props;
    const DreamUiAPI* ui;
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
