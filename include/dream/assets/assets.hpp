#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
namespace dream::assets {
using AssetId=uint64_t;
enum class Type:uint8_t{Unknown,Texture,Mesh,Material,Shader,Audio,Scene,Script};
struct Record{AssetId id=0;Type type=Type::Unknown;std::string path;uint32_t revision=1;bool loaded=false;};
class Manager final{public: AssetId register_asset(const std::string&,Type); const Record* find(AssetId)const noexcept; const Record* find_path(const std::string&)const noexcept; bool mark_loaded(AssetId,bool=true)noexcept; uint64_t content_hash(const std::string&)const noexcept; const std::unordered_map<AssetId,Record>& records()const noexcept{return records_;} private: std::unordered_map<AssetId,Record> records_; std::unordered_map<std::string,AssetId> by_path_;};
}
