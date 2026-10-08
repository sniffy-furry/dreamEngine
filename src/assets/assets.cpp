#include "dream/assets/assets.hpp"
namespace dream::assets {
uint64_t Manager::content_hash(const std::string&s)const noexcept{uint64_t h=1469598103934665603ull;for(unsigned char c:s){h^=c;h*=1099511628211ull;}return h;}
AssetId Manager::register_asset(const std::string&p,Type t){auto i=by_path_.find(p);if(i!=by_path_.end()){auto&r=records_[i->second];if(r.type!=t){r.type=t;++r.revision;}return i->second;}AssetId id=content_hash(p);while(records_.count(id))++id;records_.emplace(id,Record{id,t,p,1,false});by_path_[p]=id;return id;}
const Record* Manager::find(AssetId id)const noexcept{auto i=records_.find(id);return i==records_.end()?nullptr:&i->second;}const Record* Manager::find_path(const std::string&p)const noexcept{auto i=by_path_.find(p);return i==by_path_.end()?nullptr:find(i->second);}bool Manager::mark_loaded(AssetId id,bool v)noexcept{auto i=records_.find(id);if(i==records_.end())return false;i->second.loaded=v;return true;}
}
