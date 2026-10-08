#pragma once
#include "Components.hpp"
#include <any>
#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace astra {
using EntityId=std::uint64_t;
constexpr EntityId InvalidEntity=0;

class ComponentRegistry {
    struct StorageBase { virtual ~StorageBase()=default; virtual void erase(EntityId)=0; };
    template<class T> struct Storage:StorageBase { std::unordered_map<EntityId,T> data; void erase(EntityId e) override{data.erase(e);} };
    std::unordered_map<std::type_index,std::unique_ptr<StorageBase>> storages_;
    template<class T> Storage<T>& storage(){
        auto key=std::type_index(typeid(T)); auto it=storages_.find(key); if(it==storages_.end()) it=storages_.emplace(key,std::make_unique<Storage<T>>()).first; return *static_cast<Storage<T>*>(it->second.get());
    }
public:
    template<class T,class... Args> T& emplace(EntityId e,Args&&... args){auto& s=storage<T>(); return s.data.insert_or_assign(e,T{std::forward<Args>(args)...}).first->second;}
    template<class T> T* get(EntityId e){auto& s=storage<T>(); auto it=s.data.find(e); return it==s.data.end()?nullptr:&it->second;}
    template<class T> const T* get(EntityId e) const {auto key=std::type_index(typeid(T)); auto it=storages_.find(key); if(it==storages_.end()) return nullptr; auto* s=static_cast<const Storage<T>*>(it->second.get()); auto jt=s->data.find(e); return jt==s->data.end()?nullptr:&jt->second;}
    template<class T> bool has(EntityId e){return get<T>(e)!=nullptr;}
    template<class T> void remove(EntityId e){storage<T>().erase(e);}
    void destroy(EntityId e){for(auto& [_,s]:storages_)s->erase(e);}
};
}
