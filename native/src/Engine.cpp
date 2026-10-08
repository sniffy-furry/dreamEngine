#include "astra/Engine.hpp"
#include <algorithm>
namespace astra {
Engine::Engine():scene_(std::make_unique<Scene>("Main")),jobs_(std::make_unique<JobSystem>()){}
Engine::~Engine(){for(auto& s:scripts_)if(s.initialized&&s.enabled)s.script->onDisable(*scene_);for(auto& s:scripts_)if(s.initialized)s.script->onDestroy(*scene_);}
void Engine::runFrame(double deltaSeconds){
    deltaSeconds=std::clamp(deltaSeconds,0.0,0.25); profiler_.begin("Frame"); accumulator_+=deltaSeconds; timeSeconds_+=deltaSeconds; ++frameCount_;
    for(auto& s:scripts_)if(!s.initialized){s.script->onInitialize(*scene_);if(s.enabled)s.script->onEnable(*scene_);s.initialized=true;}
    while(accumulator_>=fixedStep_){ physics_.fixedStep(static_cast<float>(fixedStep_)); physics2D_.step(static_cast<float>(fixedStep_)); for(auto& s:scripts_)if(s.initialized&&s.enabled)s.script->onFixedUpdate(*scene_,static_cast<float>(fixedStep_)); accumulator_-=fixedStep_; }
    for(auto& s:scripts_)if(s.initialized&&s.enabled)s.script->onUpdate(*scene_,static_cast<float>(deltaSeconds));
    for(auto& s:scripts_)if(s.initialized&&s.enabled)s.script->onLateUpdate(*scene_,static_cast<float>(deltaSeconds));
    renderGraph_.execute(); profiler_.end("Frame");
}
}
