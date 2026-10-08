#include "dream/core/python_scripting.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <sstream>

#if defined(DREAM_ENGINE_WITH_PYTHON)
#include <Python.h>
#endif

namespace dream {

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
static bool run_file(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    const int rc = PyRun_SimpleFileExFlags(f, path.c_str(), 1, nullptr);
    return rc == 0;
}
#endif

bool PythonScriptManager::start(const std::string& python_home) {
    if (running_) return true;
#if defined(DREAM_ENGINE_WITH_PYTHON)
    PyStatus status;
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    config.install_signal_handlers = 0;
    status = PyConfig_SetBytesString(&config, &config.home, python_home.c_str());
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        return false;
    }
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status) || !Py_IsInitialized()) return false;
    running_ = true;
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
    for (const auto& script : scripts_) ok = run_file(script) && ok;
#else
    (void)directory;
#endif
    return ok;
}

bool PythonScriptManager::update(double dt) {
    if (!running_) return false;
#if defined(DREAM_ENGINE_WITH_PYTHON)
    PyObject* main = PyImport_AddModule("__main__");
    if (!main) return false;
    PyObject* fn = PyObject_GetAttrString(main, "on_update");
    if (!fn || !PyCallable_Check(fn)) {
        Py_XDECREF(fn);
        return true;
    }
    PyObject* arg = PyFloat_FromDouble(dt);
    PyObject* result = PyObject_CallOneArg(fn, arg);
    Py_DECREF(arg);
    Py_DECREF(fn);
    if (!result) {
        PyErr_Print();
        return false;
    }
    Py_DECREF(result);
    return true;
#else
    (void)dt;
    return false;
#endif
}

void PythonScriptManager::shutdown() noexcept {
#if defined(DREAM_ENGINE_WITH_PYTHON)
    if (running_ && Py_IsInitialized()) Py_FinalizeEx();
#endif
    running_ = false;
}

} // namespace dream
