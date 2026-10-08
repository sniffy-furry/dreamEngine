#pragma once
#include "Scene.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace astra {
struct PrefabOverride { EntityId entity=InvalidEntity; std::string path; std::string serializedValue; };
struct PrefabAsset { Guid guid{}; Guid basePrefab{}; std::string sourceScene; std::string rootName; std::vector<Guid> nestedPrefabs; std::vector<PrefabOverride> overrides; };
class PrefabSystem {
public:
    PrefabAsset makePrefab(const Scene& scene,EntityId root,const std::string& source){
        PrefabAsset p{Guid::random(),Guid{},source,scene.node(root)?scene.node(root)->name:"Prefab",{}, {}}; return p;
    }
    EntityId instantiate(Scene& scene,const PrefabAsset& prefab,EntityId parent=InvalidEntity) const {return scene.createEntity(prefab.rootName,parent);}
};
}
