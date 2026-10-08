#pragma once
#include <cstdint>
#include <string_view>

namespace dream {

using ModuleId = std::uint32_t;
using EventType = std::uint32_t;

struct EngineConfig {
    std::string_view application_name = "DreamEngine";
    bool enable_validation = false;
};

} // namespace dream
