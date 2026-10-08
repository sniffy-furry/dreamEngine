#include <jni.h>
#include <memory>
#include "dream/core/engine.hpp"
#include "tutorial_module.hpp"

namespace {
std::unique_ptr<dream::Engine> g_engine;

void start() {
    if (g_engine) return;

    g_engine = std::make_unique<dream::Engine>(dream::EngineConfig{
        .application_name = "DreamEngine Android",
        .enable_validation = false
    });

    g_engine->add_module(dream::tutorial::create());
    g_engine->initialize();
}

void stop() {
    if (!g_engine) return;
    g_engine->shutdown();
    g_engine.reset();
}
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeStart(
        JNIEnv*, jclass) {
    start();
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeStop(
        JNIEnv*, jclass) {
    stop();
}
