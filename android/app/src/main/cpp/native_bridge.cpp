#include <jni.h>
#include <memory>
#include <string>
#include "dream/core/engine.hpp"
namespace { std::unique_ptr<dream::Engine> g_engine; }
extern "C" JNIEXPORT void JNICALL Java_com_dreamingbully_dreamengine_MainActivity_nativeStart(JNIEnv* env, jclass, jstring moduleDir) {
    if (g_engine) return;
    const char* chars = env->GetStringUTFChars(moduleDir, nullptr);
    std::string dir = chars ? chars : "";
    if (chars) env->ReleaseStringUTFChars(moduleDir, chars);
    g_engine = std::make_unique<dream::Engine>(dream::EngineConfig{.application_name="DreamEngine Android", .enable_validation=false});
    g_engine->load_external_modules(dir);
    g_engine->initialize();
}
extern "C" JNIEXPORT void JNICALL Java_com_dreamingbully_dreamengine_MainActivity_nativeStop(JNIEnv*, jclass) {
    if (!g_engine) return;
    g_engine->shutdown();
    g_engine.reset();
}
