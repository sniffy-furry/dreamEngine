#pragma once
#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>

namespace astra {
struct Guid {
    std::array<std::uint8_t,16> bytes{};
    bool operator==(const Guid& o) const { return bytes==o.bytes; }
    bool empty() const { for(auto b:bytes) if(b!=0) return false; return true; }
    static Guid random(){
        std::random_device rd; Guid g{}; for(auto &b:g.bytes) b=static_cast<std::uint8_t>(rd());
        bzero(g); return g;
    }
    static void bzero(Guid& g){ g.bytes[6]=(g.bytes[6]&0x0f)|0x40; g.bytes[8]=(g.bytes[8]&0x3f)|0x80; }
    std::string toString() const {
        std::ostringstream s; s<<std::hex<<std::setfill('0');
        for(size_t i=0;i<bytes.size();i++){s<<std::setw(2)<<static_cast<int>(bytes[i]); if(i==3||i==5||i==7||i==9)s<<'-';}
        return s.str();
    }
};
}
namespace std { template<> struct hash<astra::Guid>{ size_t operator()(const astra::Guid& g) const noexcept { size_t h=0xcbf29ce484222325ULL; for(auto b:g.bytes){h^=b;h*=0x100000001b3ULL;} return h; } }; }
