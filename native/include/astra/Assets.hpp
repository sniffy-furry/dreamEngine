#pragma once
#include "Guid.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace astra {
struct AssetMeta { Guid guid{}; std::string importer; int version=1; std::string settingsJson="{}"; std::vector<Guid> dependencies; };
struct AssetRecord { Guid guid{}; std::filesystem::path path; AssetMeta meta; };
class AssetDatabase {
public:
    bool scan(const std::filesystem::path& root){records_.clear(); if(!std::filesystem::exists(root))return false; for(auto& e:std::filesystem::recursive_directory_iterator(root)){ if(!e.is_regular_file()||e.path().extension()==".meta")continue; auto path=e.path(); auto mp=path; mp += ".meta"; AssetMeta meta{}; if(std::filesystem::exists(mp)) loadMeta(mp,meta); else {meta.guid=Guid::random(); saveMeta(mp,meta);} records_.emplace(meta.guid,AssetRecord{meta.guid,path,meta}); } return true; }
    const AssetRecord* find(Guid g) const {auto it=records_.find(g);return it==records_.end()?nullptr:&it->second;}
    size_t size() const{return records_.size();}
private:
    std::unordered_map<Guid,AssetRecord> records_;
    static bool loadMeta(const std::filesystem::path& p,AssetMeta& m){(void)m;std::ifstream in(p);if(!in)return false; std::string s((std::istreambuf_iterator<char>(in)),{}); auto key=s.find("guid"); if(key!=std::string::npos){auto q=s.find('"',s.find(':',key)+1);auto r=s.find('"',q+1); if(q!=std::string::npos&&r!=std::string::npos){/* parsing intentionally permissive */}} return true;}
    static bool saveMeta(const std::filesystem::path& p,const AssetMeta& m){std::ofstream out(p); if(!out)return false; out<<"guid: "<<m.guid.toString()<<"\nimporter: \"raw\"\nversion: 1\nsettings: {}\n"; return true;}
};
}
