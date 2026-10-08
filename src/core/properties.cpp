#include "dream/core/properties.hpp"
#include "dream/core/ui_model.hpp"
#include <algorithm>

namespace dream {

PropId PropertyRegistry::register_prop(const std::string& name, PropType type, float def, float min,
                                       float max, const std::string& category,
                                       std::vector<std::string> tags) {
    if (auto it = by_name_.find(name); it != by_name_.end()) return it->second;  // keeps current value
    const PropId id = static_cast<PropId>(infos_.size());
    PropInfo info;
    info.name = name;
    const auto dot = name.find('.');
    info.module = dot == std::string::npos ? std::string() : name.substr(0, dot);
    info.category = category;
    info.tags = std::move(tags);
    info.type = type;
    info.min = min;
    info.max = max;
    infos_.push_back(std::move(info));
    values_.push_back(def);
    changed_.push_back(++revision_);
    by_name_.emplace(name, id);
    return id;
}

PropId PropertyRegistry::find(const std::string& name) const {
    auto it = by_name_.find(name);
    return it == by_name_.end() ? kInvalidProp : it->second;
}

void PropertyRegistry::set(PropId id, float v) noexcept {
    if (id >= values_.size()) return;
    const PropInfo& i = infos_[id];
    if (i.type == PropType::Bool) v = v != 0.f ? 1.f : 0.f;
    else if (i.max > i.min) v = std::min(std::max(v, i.min), i.max);
    if (values_[id] == v) return;
    values_[id] = v;
    changed_[id] = ++revision_;
}

std::vector<PropId> PropertyRegistry::query(const std::string& module, const std::string& category,
                                            const std::string& tag) const {
    std::vector<PropId> out;
    for (PropId id = 0; id < infos_.size(); ++id) {
        const PropInfo& i = infos_[id];
        if (!module.empty() && i.module != module) continue;
        if (!category.empty() && i.category != category) continue;
        if (!tag.empty() && std::find(i.tags.begin(), i.tags.end(), tag) == i.tags.end()) continue;
        out.push_back(id);
    }
    return out;
}

uint32_t UiModel::panel(const std::string& name) {
    for (uint32_t i = 0; i < panels_.size(); ++i)
        if (panels_[i].name == name) return i;
    panels_.push_back(Panel{name, {}});
    ++revision_;
    return static_cast<uint32_t>(panels_.size() - 1);
}

void UiModel::add(uint32_t panel, WidgetKind kind, PropId prop, const std::string& text) {
    if (panel >= panels_.size()) return;
    panels_[panel].widgets.push_back(Widget{kind, prop, text});
    ++revision_;
}

std::size_t UiModel::add_query(uint32_t panel, const PropertyRegistry& props, const std::string& module,
                               const std::string& category, const std::string& tag) {
    const auto ids = props.query(module, category, tag);
    for (PropId id : ids) {
        const PropInfo* i = props.info(id);
        add(panel, i->type == PropType::Bool ? WidgetKind::Toggle : WidgetKind::Slider, id, i->name);
    }
    return ids.size();
}

void UiModel::clear() {
    panels_.clear();
    ++revision_;
}

std::string UiModel::describe(const PropertyRegistry& props) const {
    std::string out;
    for (const Panel& p : panels_) {
        out += "P\t" + p.name + "\n";
        for (const Widget& w : p.widgets) {
            const PropInfo* i = props.info(w.prop);
            std::string label = !w.text.empty() ? w.text : (i ? i->name : std::string());
            switch (w.kind) {
                case WidgetKind::Slider:
                    if (i) out += "S\t" + std::to_string(w.prop) + "\t" + label + "\t" +
                                  std::to_string(i->min) + "\t" + std::to_string(i->max) + "\n";
                    break;
                case WidgetKind::Toggle:
                    if (i) out += "T\t" + std::to_string(w.prop) + "\t" + label + "\n";
                    break;
                case WidgetKind::Value:
                    if (i) out += "V\t" + std::to_string(w.prop) + "\t" + label + "\n";
                    break;
                case WidgetKind::Label:
                    out += "L\t" + w.text + "\n";
                    break;
            }
        }
    }
    return out;
}

} // namespace dream
