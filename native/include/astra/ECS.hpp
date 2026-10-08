#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
namespace astra {
using ArchetypeId=std::uint32_t; using WorldId=std::uint32_t;
struct Chunk { std::vector<std::uint64_t> entities; std::size_t capacity=256; };
struct Archetype { ArchetypeId id=0; std::vector<std::string> componentTypes; std::vector<Chunk> chunks; };
class ECSWorld { std::unordered_map<ArchetypeId,Archetype> archetypes_; ArchetypeId next_=0; public: ArchetypeId createArchetype(std::vector<std::string> types){auto id=++next_;archetypes_[id]={id,std::move(types),{}};archetypes_[id].chunks.push_back({{},256});return id;} void addEntity(ArchetypeId a,std::uint64_t e){auto& ar=archetypes_.at(a);auto& c=ar.chunks.back();if(c.entities.size()>=c.capacity)ar.chunks.push_back({{},256});ar.chunks.back().entities.push_back(e);} template<class F>void query(F&& f){for(auto& [_,a]:archetypes_)for(auto& c:a.chunks)for(auto e:c.entities)f(e);} };
}
