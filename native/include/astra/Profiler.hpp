#pragma once
#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
namespace astra {
class Profiler { using Clock=std::chrono::steady_clock; struct Sample{std::uint64_t calls=0; double ms=0; Clock::time_point start{};}; std::unordered_map<std::string,Sample> samples_; public: void begin(const std::string& n){samples_[n].start=Clock::now();} void end(const std::string& n){auto& s=samples_[n];s.ms+=std::chrono::duration<double,std::milli>(Clock::now()-s.start).count();s.calls++;} double milliseconds(const std::string& n)const{auto it=samples_.find(n);return it==samples_.end()?0:it->second.ms;} std::uint64_t calls(const std::string& n)const{auto it=samples_.find(n);return it==samples_.end()?0:it->second.calls;} };
}
