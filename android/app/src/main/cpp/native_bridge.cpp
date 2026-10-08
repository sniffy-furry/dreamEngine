#include <jni.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <memory>
#include <string>
#include "dream/core/engine.hpp"
#include "dream/core/log.hpp"

namespace { std::unique_ptr<dream::Engine> g_engine; std::string g_modules_dir; }

static std::string to_std(JNIEnv* env, jstring s) {
    if (!s) return {};
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string out = c ? c : "";
    if (c) env->ReleaseStringUTFChars(s, c);
    return out;
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeStart(JNIEnv* env, jclass, jstring moduleDir, jstring pythonHome) {
    if (g_engine) return;
    g_modules_dir = to_std(env, moduleDir);
    const std::string python = to_std(env, pythonHome);

    g_engine = std::make_unique<dream::Engine>(dream::EngineConfig{.application_name="DreamEngine Android", .enable_validation=false});
    // A broken .so or missing Python must NOT take the whole engine down.
    if (!g_engine->load_external_modules(g_modules_dir))
        dream::log_push(5, "some native modules failed to load (see above)");
    if (!g_engine->start_python(python, g_modules_dir))
        dream::log_push(5, "Python scripts not started (see above)");
    if (!g_engine->initialize())
        dream::log_push(6, "engine initialize failed");
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeReload(JNIEnv*, jclass) {
    if (g_engine) g_engine->reload_modules(g_modules_dir);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeDrainLog(JNIEnv* env, jclass) {
    return env->NewStringUTF(dream::log_drain().c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeUpdate(JNIEnv*, jclass, jdouble dt) {
    if (g_engine) g_engine->update(static_cast<double>(dt));
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeStop(JNIEnv*, jclass) {
    if (!g_engine) return;
    gl_teardown();
    g_engine->shutdown();
    g_engine.reset();
}
