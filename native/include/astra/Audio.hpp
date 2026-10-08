#pragma once
#include "Math.hpp"
#include <cstdint>
#include <string>
namespace astra {
struct AudioClip { std::uint64_t id=0; std::string path; float length=0; };
struct AudioEmitter { Vec3 position{}; float volume=1; float pitch=1; bool looping=false; };
class AudioSystem { std::uint64_t next_=0; public: std::uint64_t createClip(std::string path,float seconds){(void)path;(void)seconds;return ++next_;} void updateListener(Vec3 pos,Vec3 forward,Vec3 up){(void)pos;(void)forward;(void)up;} };
}
