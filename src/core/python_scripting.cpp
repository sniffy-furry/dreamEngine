#include "dream/core/python_scripting.hpp"
#include "dream/core/log.hpp"
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>

#if defined(DREAM_ENGINE_WITH_PYTHON)
#include <Python.h>
#endif

namespace dream {

struct PythonScriptManager::Loaded {
    std::string path;
    std::string module_name;
#if defined(DREAM_ENGINE_WITH_PYTHON)
    PyObject* module = nullptr;
    PyObject* on_update = nullptr;
    PyObject* on_stop = nullptr;
#endif
    bool failed = false;
};

PythonScriptManager::PythonScriptManager() = default;
PythonScriptManager::~PythonScriptManager() { shutdown(); }

bool PythonScriptManager::scan(const std::string& directory) {
    scripts_.clear();
    DIR* d = opendir(directory.c_str());
    if (!d) return false;
    while (auto* e = readdir(d)) {
        const char* n = e->d_name;
        const std::size_t l = std::strlen(n);
        if (l > 3 && std::strcmp(n + l - 3, ".py") == 0)
            scripts_.push_back(directory + "/" + n);
    }
    closedir(d);
    std::sort(scripts_.begin(), scripts_.end());
    return true;
}

#if defined(DREAM_ENGINE_WITH_PYTHON)
namespace {

// Python print()/tracebacks -> in-app log + logcat.
PyObject* dream_log_write(PyObject*, PyObject* args) {
    const char* text = nullptr;
    if (!PyArg_ParseTuple(args, "s", &text)) return nullptr;
    if (text && *text) {
        std::string t(text);
        while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back();
        if (!t.empty()) log_push(4, t);
    }
    Py_RETURN_NONE;
}
PyMethodDef kLogMethods[] = {
    {"write", dream_log_write, METH_VARARGS, "write text to the engine log"},
    {nullptr, nullptr, 0, nullptr}};
PyModuleDef kLogModule = {PyModuleDef_HEAD_INIT, "_dream_log", nullptr, -1, kLogMethods,
                          nullptr, nullptr, nullptr, nullptr};
PyObject* init_dream_log() { return PyModule_Create(&kLogModule); }

void log_py_error(const std::string& where) {
    log_push(6, "Python error in " + where);
    PyErr_Print();  // goes through sys.stderr -> engine log
}

std::string read_file(const std::string& path, bool& ok) {
    std::ifstream in(path, std::ios::binary);
    ok = static_cast<bool>(in);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string module_name_for(const std::string& path) {
    auto slash = path.find_last_of('/');
    std::string base = slash == std::string::npos ? path : path.substr(slash + 1);
    if (base.size() > 3) base.resize(base.size() - 3);
    for (auto& c : base) if (!(isalnum(static_cast<unsigned char>(c)) || c == '_')) c = '_';
    return "dream_mod_" + base;
}

}  // namespace
#endif

bool PythonScriptManager::start(const std::string& python_home) {
    if (running_) return true;
#if defined(DREAM_ENGINE_WITH_PYTHON)
    PyImport_AppendInittab("_dream_log", init_dream_log);
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    config.install_signal_handlers = 0;
    PyStatus status = PyConfig_SetBytesString(&config, &config.home, python_home.c_str());
    if (PyStatus_Exception(status)) {
        log_push(6, std::string("Python config failed: ") + (status.err_msg ? status.err_msg : "?"));
        PyConfig_Clear(&config);
        return false;
    }
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status) || !Py_IsInitialized()) {
        log_push(6, std::string("Python init failed (home=") + python_home + "): " +
                        (status.err_msg ? status.err_msg : "?"));
        return false;
    }
    PyRun_SimpleString(
        "import sys, _dream_log\n"
        "class _W:\n"
        "    def write(self, s):\n"
        "        _dream_log.write(s); return len(s)\n"
        "    def flush(self): pass\n"
        "sys.stdout = sys.stderr = _W()\n");
    running_ = true;
    log_push(4, "Python started");
    return true;
#else
    (void)python_home;
    return false;
#endif
}

bool PythonScriptManager::run_directory(const std::string& directory) {
    if (!running_ || !scan(directory)) return false;
    bool ok = true;
#if defined(DREAM_ENGINE_WITH_PYTHON)
    for (const auto& path : scripts_) {
        auto item = std::make_unique<Loaded>();
        item->path = path;
        item->module_name = module_name_for(path);

        bool read_ok = false;
        const std::string src = read_file(path, read_ok);
        if (!read_ok) { log_push(6, "cannot read " + path); ok = false; continue; }

        PyObject* code = Py_CompileString(src.c_str(), path.c_str(), Py_file_input);
        if (!code) { log_py_error(path); ok = false; continue; }

        // Each script gets its own module: scripts no longer overwrite each other's on_update.
        PyObject* mod = PyImport_ExecCodeModuleEx(item->module_name.c_str(), code, path.c_str());
        Py_DECREF(code);
        if (!mod) { log_py_error(path); ok = false; continue; }

        item->module = mod;
        PyObject* start = PyObject_GetAttrString(mod, "on_start");
        if (start && PyCallable_Check(start)) {
            PyObject* r = PyObject_CallNoArgs(start);
            if (!r) { log_py_error(path + " on_start"); item->failed = true; ok = false; }
            Py_XDECREF(r);
        }
        Py_XDECREF(start);
        PyErr_Clear();

        item->on_update = PyObject_GetAttrString(mod, "on_update");
        if (item->on_update && !PyCallable_Check(item->on_update)) { Py_CLEAR(item->on_update); }
        item->on_stop = PyObject_GetAttrString(mod, "on_stop");
        if (item->on_stop && !PyCallable_Check(item->on_stop)) { Py_CLEAR(item->on_stop); }
        PyErr_Clear();

        log_push(4, "loaded python script: " + path);
        loaded_.push_back(std::move(item));
    }
#else
    (void)directory;
#endif
    return ok;
}

bool PythonScriptManager::update(double dt) {
    if (!running_) return false;
#if defined(DREAM_ENGINE_WITH_PYTHON)
    bool ok = true;
    for (auto& s : loaded_) {
        if (s->failed || !s->on_update) continue;
        PyObject* arg = PyFloat_FromDouble(dt);
        PyObject* result = PyObject_CallOneArg(s->on_update, arg);
        Py_DECREF(arg);
        if (!result) {
            log_py_error(s->path + " on_update (script disabled)");
            s->failed = true;  // don't spam the log every frame
            ok = false;
            continue;
        }
        Py_DECREF(result);
    }
    return ok;
#else
    (void)dt;
    return false;
#endif
}

void PythonScriptManager::unload() noexcept {
#if defined(DREAM_ENGINE_WITH_PYTHON)
    if (running_ && Py_IsInitialized()) {
        PyObject* modules = PyImport_GetModuleDict();  // borrowed
        for (auto it = loaded_.rbegin(); it != loaded_.rend(); ++it) {
            auto& s = *it;
            if (s->on_stop && !s->failed) {
                PyObject* r = PyObject_CallNoArgs(s->on_stop);
                if (!r) log_py_error(s->path + " on_stop"); else Py_DECREF(r);
            }
            Py_XDECREF(s->on_update);
            Py_XDECREF(s->on_stop);
            Py_XDECREF(s->module);
            if (modules) PyDict_DelItemString(modules, s->module_name.c_str());
            PyErr_Clear();
        }
    }
#endif
    loaded_.clear();
}

void PythonScriptManager::shutdown() noexcept {
    unload();
#if defined(DREAM_ENGINE_WITH_PYTHON)
    if (running_ && Py_IsInitialized()) Py_FinalizeEx();
#endif
    running_ = false;
}

} // namespace dream
