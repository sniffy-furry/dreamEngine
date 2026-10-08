#pragma once
#include "types.hpp"

namespace dream {

class EngineAPI;

class IEngineModule {
public:
    virtual ~IEngineModule() = default;
    virtual const char* name() const noexcept = 0;
    virtual bool initialize(EngineAPI&) = 0;
    virtual void update(double dt) = 0;
    virtual void shutdown() noexcept = 0;
};

} // namespace dream
