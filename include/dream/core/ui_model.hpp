#pragma once
#include "properties.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace dream {

// The UI *model*: panels are just lists of widgets that point at property handles.
// Nothing here draws. A renderer (Android views today, a GPU UI module later) reads this.
enum class WidgetKind : uint8_t { Slider = 0, Toggle = 1, Label = 2, Value = 3 };

struct Widget {
    WidgetKind kind = WidgetKind::Label;
    PropId prop = kInvalidProp;
    std::string text;
};

struct Panel {
    std::string name;
    std::vector<Widget> widgets;
};

class UiModel {
public:
    uint32_t panel(const std::string& name);  // get-or-create
    void add(uint32_t panel, WidgetKind kind, PropId prop, const std::string& text);
    // Adds a widget for every property matching the filter (bool -> Toggle, float -> Slider).
    std::size_t add_query(uint32_t panel, const PropertyRegistry& props, const std::string& module,
                          const std::string& category, const std::string& tag);
    void clear();  // called on hot reload; scripts/modules rebuild their panels
    const std::vector<Panel>& panels() const noexcept { return panels_; }
    uint32_t layout_revision() const noexcept { return revision_; }

    // Compact text for a renderer (parsed only when layout_revision changes):
    //   P\tname | S\tid\tlabel\tmin\tmax | T\tid\tlabel | V\tid\tlabel | L\ttext
    std::string describe(const PropertyRegistry& props) const;

private:
    std::vector<Panel> panels_;
    uint32_t revision_ = 1;
};

} // namespace dream
