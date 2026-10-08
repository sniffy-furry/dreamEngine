#pragma once
#include "dream/ecs/ecs.hpp"
#include <string>
namespace dream::scene {
struct Scene { std::string name="Untitled"; ecs::World world; };
bool save(const Scene&, const std::string& path, std::string* error=nullptr);
bool load(Scene&, const std::string& path, std::string* error=nullptr);
std::string serialize(const Scene&);
bool deserialize(Scene&, const std::string&, std::string* error=nullptr);
}
