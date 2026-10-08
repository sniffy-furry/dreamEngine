#pragma once
#include "dream/core/module.hpp"
#include <memory>

namespace dream::tutorial {

struct TutorialTick {
    double dt;
    double elapsed;
};

class TutorialModule final : public IEngineModule {
public:
    const char* name() const noexcept override;
    bool initialize(EngineAPI& api) override;
    void update(double dt) override;
    void shutdown() noexcept override;

private:
    EngineAPI* api_ = nullptr;
    double elapsed_ = 0.0;
};

std::unique_ptr<IEngineModule> create();

} // namespace dream::tutorial
