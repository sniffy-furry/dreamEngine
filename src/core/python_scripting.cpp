#include "dream/core/python_scripting.hpp"
#include "dream/core/log.hpp"
#include "dream/core/properties.hpp"
#include "dream/core/ui_model.hpp"
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

namespace {
PropertyRegistry* g_props = nullptr;
UiModel* g_ui = nullptr;
}

void PythonScriptManager::bind(PropertyRegistry* props, UiModel* ui) noexcept {
    g_props = props;
    g_ui = ui;
}

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

// ---- _dream: thin handle-based bindings (the friendly `dream.props` / `dream.ui` API is built on top) ----
PyObject* d_props_register(PyObject*, PyObject* args) {
    const char *name, *cat, *tags;
    int kind;
    double def, lo, hi;
    if (!PyArg_ParseTuple(args, "sidddss", &name, &kind, &def, &lo, &hi, &cat, &tags)) return nullptr;
    std::vector<std::string> tl;
    std::string t(tags);
    for (std::size_t pos = 0; pos <= t.size();) {
        auto c = t.find(',', pos);
        if (c == std::string::npos) c = t.size();
        if (c > pos) tl.push_back(t.substr(pos, c - pos));
        pos = c + 1;
    }
    const PropId id = g_props->register_prop(name, kind == 1 ? PropType::Bool : PropType::Float,
                                             static_cast<float>(def), static_cast<float>(lo),
                                             static_cast<float>(hi), cat, std::move(tl));
    return PyLong_FromUnsignedLong(id);
}
PyObject* d_props_find(PyObject*, PyObject* args) {
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) return nullptr;
    const PropId id = g_props->find(name);
    return PyLong_FromLong(id == kInvalidProp ? -1 : static_cast<long>(id));
}
PyObject* d_props_get(PyObject*, PyObject* args) {
    long id;
    if (!PyArg_ParseTuple(args, "l", &id)) return nullptr;
    return PyFloat_FromDouble(id < 0 ? 0.0 : g_props->get(static_cast<PropId>(id)));
}
PyObject* d_props_set(PyObject*, PyObject* args) {
    long id;
    double v;
    if (!PyArg_ParseTuple(args, "ld", &id, &v)) return nullptr;
    if (id >= 0) g_props->set(static_cast<PropId>(id), static_cast<float>(v));
    Py_RETURN_NONE;
}
PyObject* d_ui_panel(PyObject*, PyObject* args) {
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) return nullptr;
    return PyLong_FromUnsignedLong(g_ui->panel(name));
}
PyObject* d_ui_add(PyObject*, PyObject* args) {
    unsigned long panel;
    int kind;
    long prop;
    const char* text;
    if (!PyArg_ParseTuple(args, "kils", &panel, &kind, &prop, &text)) return nullptr;
    g_ui->add(static_cast<uint32_t>(panel), static_cast<WidgetKind>(kind),
              prop < 0 ? kInvalidProp : static_cast<PropId>(prop), text);
    Py_RETURN_NONE;
}
PyObject* d_ui_query(PyObject*, PyObject* args) {
    unsigned long panel;
    const char *m, *c, *t;
    if (!PyArg_ParseTuple(args, "ksss", &panel, &m, &c, &t)) return nullptr;
    return PyLong_FromSize_t(g_ui->add_query(static_cast<uint32_t>(panel), *g_props, m, c, t));
}
PyMethodDef kDreamMethods[] = {
    {"props_register", d_props_register, METH_VARARGS, ""},
    {"props_find", d_props_find, METH_VARARGS, ""},
    {"props_get", d_props_get, METH_VARARGS, ""},
    {"props_set", d_props_set, METH_VARARGS, ""},
    {"ui_panel", d_ui_panel, METH_VARARGS, ""},
    {"ui_add", d_ui_add, METH_VARARGS, ""},
    {"ui_query", d_ui_query, METH_VARARGS, ""},
    {nullptr, nullptr, 0, nullptr}};
PyModuleDef kDreamModule = {PyModuleDef_HEAD_INIT, "_dream", nullptr, -1, kDreamMethods,
                            nullptr, nullptr, nullptr, nullptr};
PyObject* init_dream() { return PyModule_Create(&kDreamModule); }

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
    PyImport_AppendInittab("_dream", init_dream);
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
    // `import dream` -> dream.props / dream.ui (friendly wrappers over the handle-based _dream module)
    PyRun_SimpleString(
        "def _boot():\n"
        "    import sys, types, _dream as d\n"
        "    def pid(p): return p if isinstance(p, int) else d.props_find(p)\n"
        "    class Props:\n"
        "        def register(self, name, type=float, default=0.0, min=0.0, max=1.0, category='', tags=()):\n"
        "            return d.props_register(name, 1 if type is bool else 0, float(default), float(min), float(max), category, ','.join(tags))\n"
        "        def find(self, name): return d.props_find(name)\n"
        "        def get(self, p): return d.props_get(pid(p))\n"
        "        def set(self, p, v): d.props_set(pid(p), float(v))\n"
        "    class Panel:\n"
        "        def __init__(self, i): self.id = i\n"
        "        def slider(self, p, label=''): d.ui_add(self.id, 0, pid(p), label); return self\n"
        "        def toggle(self, p, label=''): d.ui_add(self.id, 1, pid(p), label); return self\n"
        "        def label(self, text): d.ui_add(self.id, 2, -1, text); return self\n"
        "        def value(self, p, label=''): d.ui_add(self.id, 3, pid(p), label); return self\n"
        "        def query(self, module='', category='', tag=''): d.ui_query(self.id, module, category, tag); return self\n"
        "    class Ui:\n"
        "        def panel(self, name): return Panel(d.ui_panel(name))\n"
        "    m = types.ModuleType('dream')\n"
        "    m.props = Props(); m.ui = Ui()\n"
        "    sys.modules['dream'] = m\n"
        "_boot(); del _boot\n");
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
