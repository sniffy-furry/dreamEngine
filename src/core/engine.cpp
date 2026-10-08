#include "dream/core/engine.hpp"
#include "dream/core/log.hpp"
#include <chrono>


namespace {
void host_log(int level, const char* message) {
    dream::log_push(level, message ? message : "");
}
dream::PropertyRegistry* g_props = nullptr;
dream::UiModel* g_ui = nullptr;
const char* orempty(const char* s) { return s ? s : ""; }

uint32_t c_reg_float(const char* n, float d, float lo, float hi, const char* cat) {
    return g_props->register_prop(orempty(n), dream::PropType::Float, d, lo, hi, orempty(cat));
}
uint32_t c_reg_bool(const char* n, int d, const char* cat) {
    return g_props->register_prop(orempty(n), dream::PropType::Bool, d ? 1.f : 0.f, 0.f, 1.f, orempty(cat));
}
uint32_t c_find(const char* n) { return g_props->find(orempty(n)); }
float c_get(uint32_t id) { return g_props->get(id); }
void c_set(uint32_t id, float v) { g_props->set(id, v); }
const DreamPropertyAPI kPropApi = {c_reg_float, c_reg_bool, c_find, c_get, c_set};

uint32_t c_panel(const char* n) { return g_ui->panel(orempty(n)); }
void c_slider(uint32_t p, uint32_t id, const char* l) { g_ui->add(p, dream::WidgetKind::Slider, id, orempty(l)); }
void c_toggle(uint32_t p, uint32_t id, const char* l) { g_ui->add(p, dream::WidgetKind::Toggle, id, orempty(l)); }
void c_value(uint32_t p, uint32_t id, const char* l) { g_ui->add(p, dream::WidgetKind::Value, id, orempty(l)); }
void c_label(uint32_t p, const char* t) { g_ui->add(p, dream::WidgetKind::Label, dream::kInvalidProp, orempty(t)); }
void c_query(uint32_t p, const char* m, const char* c, const char* t) {
    g_ui->add_query(p, *g_props, orempty(m), orempty(c), orempty(t));
}
const DreamUiAPI kUiApi = {c_panel, c_slider, c_toggle, c_value, c_label, c_query};

double host_time() {
    using clock = std::chrono::steady_clock;
    static const auto start = clock::now();
    return std::chrono::duration<double>(clock::now() - start).count();
}
}

namespace dream {

Engine::Engine(EngineConfig config) : config_(config) {
    g_props = &props_;
    g_ui = &ui_;
    python_scripts_.bind(&props_, &ui_);
}

Engine::~Engine() {
    shutdown();
}

bool Engine::add_module(std::unique_ptr<IEngineModule> module) {
    if (initialized_ || !module) return false;
    modules_.push_back(std::move(module));
    return true;
}

bool Engine::initialize() {
    if (initialized_) return false;

    std::size_t initialized_count = 0;
    for (auto& module : modules_) {
        if (!module->initialize(api_)) {
            while (initialized_count > 0) {
                --initialized_count;
                modules_[initialized_count]->shutdown();
            }
            return false;
        }
        ++initialized_count;
    }

    initialized_ = true;
    return true;
}

void Engine::update(double dt) {
    if (!initialized_) return;
    for (auto& module : modules_) module->update(dt);
    module_loader_.update(dt);
    python_scripts_.update(dt);
}

bool Engine::start_python(const std::string& python_home, const std::string& script_directory) {
    if (!python_scripts_.start(python_home)) return false;
    return python_scripts_.run_directory(script_directory);
}

void Engine::update_python(double dt) {
    python_scripts_.update(dt);
}

bool Engine::load_external_modules(const std::string& directory) {
    external_host_ = {};
    external_host_.abi_version = DREAM_ENGINE_MODULE_ABI_VERSION;
    external_host_.log = host_log;
    external_host_.get_time_seconds = host_time;
    external_host_.props = &kPropApi;
    external_host_.ui = &kUiApi;
    python_scripts_.scan(directory);
    return module_loader_.load_directory(directory, external_host_);
}

bool Engine::reload_modules(const std::string& directory) {
    // Live hot-swap: no app restart needed.
    python_scripts_.unload();
    ui_.clear();   // panels are rebuilt by the reloaded modules; property values are kept
    const bool native_ok = load_external_modules(directory);   // dlcloses old .so, dlopens new
    const bool py_ok = python_scripts_.running() ? python_scripts_.run_directory(directory) : false;
    log_push(4, "reload done: native modules=" + std::to_string(module_loader_.loaded_count()) +
                    ", python scripts=" + std::to_string(python_scripts_.scripts().size()));
    return native_ok && py_ok;
}

void Engine::shutdown() noexcept {
    module_loader_.shutdown();
    python_scripts_.shutdown();
    if (!initialized_) return;

    for (auto it = modules_.rbegin(); it != modules_.rend(); ++it) {
        (*it)->shutdown();
    }
    initialized_ = false;
}

} // namespace dream
