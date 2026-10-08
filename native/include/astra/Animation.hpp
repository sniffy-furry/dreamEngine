#pragma once
#include "Math.hpp"
#include <string>
#include <unordered_map>
#include <vector>
namespace astra {
struct AnimationKey { float time=0; Transform value{}; };
struct AnimationClip { std::string name; float length=0; std::vector<AnimationKey> keys; };
struct AnimationState { std::string clip; float time=0; float speed=1; bool looping=true; };
class Animator { std::vector<AnimationClip> clips_; AnimationState state_; public: void addClip(AnimationClip c){clips_.push_back(std::move(c));} void play(std::string n){state_.clip=std::move(n);state_.time=0;} void update(float dt){state_.time+=dt*state_.speed;} const AnimationState& state()const{return state_;} };
}
