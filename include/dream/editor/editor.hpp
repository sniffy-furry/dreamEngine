#pragma once
#include "dream/scene/scene.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace dream::editor {
struct Selection { ecs::Entity entity=ecs::kNullEntity; };
struct State { bool playing=false; bool dirty=false; Selection selection{}; std::string project_path; std::string scene_path; };
class Core final { public: Core(); scene::Scene& scene() noexcept{return scene_;} const scene::Scene& scene()const noexcept{return scene_;} State& state()noexcept{return state_;} const State& state()const noexcept{return state_;}
 ecs::Entity create_entity(const std::string& kind); bool delete_selected(); bool select(uint32_t index); bool save_scene(std::string*err=nullptr); bool open_scene(const std::string&,std::string*err=nullptr); std::string hierarchy_text()const; std::string inspector_text()const; void set_playing(bool v)noexcept{state_.playing=v;}
private: scene::Scene scene_; State state_; };
}
