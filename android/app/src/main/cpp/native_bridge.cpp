#include <jni.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <memory>
#include <string>
#include "dream/core/engine.hpp"
#include "dream/core/log.hpp"
#include "dream/editor/editor.hpp"

namespace {
std::unique_ptr<dream::Engine> g_engine;
std::string g_modules_dir;
std::unique_ptr<dream::editor::Core> g_editor;

// Minimal EGL context bound to the engine thread (the Android main thread).
struct Gl {
    ANativeWindow* window = nullptr;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    int w = 0, h = 0;
} g_gl;

void gl_teardown() {
    if (g_engine) g_engine->render_surface_lost();   // modules free their GL objects (context still current)
    if (g_gl.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_gl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_gl.surface != EGL_NO_SURFACE) eglDestroySurface(g_gl.display, g_gl.surface);
        if (g_gl.context != EGL_NO_CONTEXT) eglDestroyContext(g_gl.display, g_gl.context);
        eglTerminate(g_gl.display);
    }
    if (g_gl.window) ANativeWindow_release(g_gl.window);
    g_gl = Gl{};
}

bool gl_setup(ANativeWindow* window) {
    g_gl.window = window;
    g_gl.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_gl.display == EGL_NO_DISPLAY || !eglInitialize(g_gl.display, nullptr, nullptr)) return false;
    const EGLint cfg_attrs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                                EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
                                EGL_DEPTH_SIZE, 24, EGL_NONE};
    EGLConfig config; EGLint n = 0;
    if (!eglChooseConfig(g_gl.display, cfg_attrs, &config, 1, &n) || n < 1) return false;
    const EGLint ctx_attrs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    g_gl.context = eglCreateContext(g_gl.display, config, EGL_NO_CONTEXT, ctx_attrs);
    g_gl.surface = eglCreateWindowSurface(g_gl.display, config, window, nullptr);
    if (g_gl.context == EGL_NO_CONTEXT || g_gl.surface == EGL_NO_SURFACE) return false;
    if (!eglMakeCurrent(g_gl.display, g_gl.surface, g_gl.surface, g_gl.context)) return false;
    eglSwapInterval(g_gl.display, 1);
    return true;
}
}

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

    g_editor = std::make_unique<dream::editor::Core>();
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
Java_com_dreamingbully_dreamengine_MainActivity_nativeSurfaceChanged(JNIEnv* env, jclass, jobject surface, jint w, jint h) {
    if (!g_engine) return;
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) return;
    if (g_gl.window == window) {            // same surface, new size
        ANativeWindow_release(window);
        g_gl.w = w; g_gl.h = h;
        g_engine->render_surface_resized(w, h);
        return;
    }
    gl_teardown();
    if (!gl_setup(window)) {
        dream::log_push(6, "EGL/GLES3 setup failed");
        gl_teardown();
        return;
    }
    g_gl.w = w; g_gl.h = h;
    dream::log_push(4, "GL surface ready " + std::to_string(w) + "x" + std::to_string(h));
    g_engine->render_surface_ready(w, h);
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeSurfaceDestroyed(JNIEnv*, jclass) {
    gl_teardown();
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeRender(JNIEnv*, jclass, jdouble dt) {
    if (!g_engine || g_gl.surface == EGL_NO_SURFACE) return;
    g_engine->render_frame(static_cast<double>(dt));
    eglSwapBuffers(g_gl.display, g_gl.surface);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeUiRevision(JNIEnv*, jclass) {
    return g_engine ? static_cast<jint>(g_engine->ui().layout_revision()) : 0;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeUiLayout(JNIEnv* env, jclass) {
    return env->NewStringUTF(g_engine ? g_engine->ui().describe(g_engine->props()).c_str() : "");
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeGetProp(JNIEnv*, jclass, jint id) {
    return g_engine ? g_engine->props().get(static_cast<dream::PropId>(id)) : 0.f;
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeSetProp(JNIEnv*, jclass, jint id, jfloat v) {
    if (g_engine) g_engine->props().set(static_cast<dream::PropId>(id), v);
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeUpdate(JNIEnv*, jclass, jdouble dt) {
    if (g_engine) g_engine->update(static_cast<double>(dt));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeEditorHierarchy(JNIEnv* env, jclass) {
    return env->NewStringUTF(g_editor ? g_editor->hierarchy_text().c_str() : "");
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeEditorInspector(JNIEnv* env, jclass) {
    return env->NewStringUTF(g_editor ? g_editor->inspector_text().c_str() : "No editor");
}
extern "C" JNIEXPORT jint JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeEditorCreate(JNIEnv* env, jclass, jstring kind) {
    if (!g_editor) return -1; const char* k=kind?env->GetStringUTFChars(kind,nullptr):"entity";
    auto e=g_editor->create_entity(k?k:"entity"); if(kind) env->ReleaseStringUTFChars(kind,k); return (jint)e.index;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeEditorSelect(JNIEnv*, jclass, jint index) { return g_editor && g_editor->select((uint32_t)index); }
extern "C" JNIEXPORT jboolean JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeEditorDelete(JNIEnv*, jclass) { return g_editor && g_editor->delete_selected(); }
extern "C" JNIEXPORT jboolean JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeEditorSave(JNIEnv* env, jclass, jstring path) {
    if (!g_editor || !path) return false; const char* p=env->GetStringUTFChars(path,nullptr); g_editor->state().scene_path=p?p:""; if(p)env->ReleaseStringUTFChars(path,p); std::string err; if(!g_editor->save_scene(&err)){dream::log_push(6,err);return false;} return true;
}

extern "C" JNIEXPORT void JNICALL
Java_com_dreamingbully_dreamengine_MainActivity_nativeStop(JNIEnv*, jclass) {
    if (!g_engine) return;
    gl_teardown();
    g_engine->shutdown();
    g_engine.reset();
    g_editor.reset();
}
