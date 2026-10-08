#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace dream {

using PropId = uint32_t;
constexpr PropId kInvalidProp = 0xFFFFFFFFu;

enum class PropType : uint8_t { Float = 0, Bool = 1 };

struct PropInfo {
    std::string name;      // "physics.gravity"  (module = text before the first '.')
    std::string module;
    std::string category;
    std::vector<std::string> tags;
    PropType type = PropType::Float;
    float min = 0.f, max = 1.f;
};

// Hot path = get()/set() by handle: one bounds check + one array access. No strings, no hashing.
// Strings are only used at registration / lookup time. Not thread-safe (call from the engine thread).
class PropertyRegistry {
public:
    PropId register_prop(const std::string& name, PropType type, float def, float min, float max,
                         const std::string& category, std::vector<std::string> tags = {});
    PropId find(const std::string& name) const;

    float get(PropId id) const noexcept { return id < values_.size() ? values_[id] : 0.f; }
    void set(PropId id, float v) noexcept;

    // Dirty tracking: every consumer remembers the revision it last saw and compares.
    uint64_t revision() const noexcept { return revision_; }
    uint64_t changed_at(PropId id) const noexcept { return id < changed_.size() ? changed_[id] : 0; }

    const PropInfo* info(PropId id) const noexcept { return id < infos_.size() ? &infos_[id] : nullptr; }
    std::size_t size() const noexcept { return infos_.size(); }

    // Empty string = wildcard. Run once when a panel is built; keep the resulting handle list.
    std::vector<PropId> query(const std::string& module, const std::string& category,
                              const std::string& tag) const;

private:
    std::vector<PropInfo> infos_;
    std::vector<float> values_;
    std::vector<uint64_t> changed_;
    std::unordered_map<std::string, PropId> by_name_;
    uint64_t revision_ = 1;
};

} // namespace dream
