#include <jni.h>
#include <memory>
#include <string>
#include "dream/core/engine.hpp"

namespace { std::unique_ptr<dream::Engine> g_engine; }

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeStart(JNIEnv* env, jclass, jstring moduleDir, jstring pythonHome) {
    if (g_engine) return;
    const char* module_chars = env->GetStringUTFChars(moduleDir, nullptr);
    const char* python_chars = env->GetStringUTFChars(pythonHome, nullptr);
    std::string modules = module_chars ? module_chars : "";
    std::string python = python_chars ? python_chars : "";
    if (module_chars) env->ReleaseStringUTFChars(moduleDir, module_chars);
    if (python_chars) env->ReleaseStringUTFChars(pythonHome, python_chars);

    g_engine = std::make_unique<dream::Engine>(dream::EngineConfig{.application_name="DreamEngine Android", .enable_validation=false});
    if (!g_engine->load_external_modules(modules)) {
        g_engine.reset();
        return;
    }
    if (!g_engine->start_python(python, modules)) {
        g_engine.reset();
        return;
    }
    g_engine->initialize();
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeUpdate(JNIEnv*, jclass, jdouble dt) {
    if (g_engine) g_engine->update(static_cast<double>(dt));
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeStop(JNIEnv*, jclass) {
    if (!g_engine) return;
    g_engine->shutdown();
    g_engine.reset();
}
