#pragma once
#include "Scene.hpp"
#include <functional>
#include <string>
#include <unordered_map>
namespace astra {
class EditorRegistry { std::unordered_map<std::string,std::function<void()>> menus_; public: void menu(std::string name,std::function<void()> fn){menus_[std::move(name)]=std::move(fn);} void execute(const std::string& n){auto it=menus_.find(n);if(it!=menus_.end())it->second();} };
struct GizmoContext { bool snap=false; float snapDistance=1.0f; };
}
