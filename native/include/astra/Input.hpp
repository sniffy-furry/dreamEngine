#pragma once
#include <string>
#include <unordered_map>
namespace astra {
enum class InputDeviceType { KeyboardMouse, Gamepad, Touch, XR };
struct InputBinding { InputDeviceType device=InputDeviceType::KeyboardMouse; std::string control; float scale=1.0f; };
class InputSystem { std::unordered_map<std::string,float> values_; std::unordered_map<std::string,InputBinding> bindings_; public: void bind(std::string action,InputBinding b){bindings_[std::move(action)]=std::move(b);} void set(const std::string& action,float value){values_[action]=value;} float value(const std::string& action) const {auto it=values_.find(action);return it==values_.end()?0.0f:it->second;} bool pressed(const std::string& action) const{return value(action)>0.5f;} };
}
