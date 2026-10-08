#pragma once
#include "Registry.hpp"
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace astra {
class Scene {
public:
    struct Node { EntityId id; std::string name; EntityId parent=InvalidEntity; std::vector<EntityId> children; bool active=true; };
    explicit Scene(std::string name="Untitled") : name_(std::move(name)) {}
    EntityId createEntity(std::string name="GameObject", EntityId parent=InvalidEntity){
        EntityId id=++nextId_; nodes_.emplace(id,Node{id,std::move(name),parent,{ },true}); registry_.emplace<TransformComponent>(id);
        if(parent!=InvalidEntity){nodes_[parent].children.push_back(id);} roots_.push_back(parent==InvalidEntity?id:InvalidEntity); roots_.erase(std::remove(roots_.begin(),roots_.end(),InvalidEntity),roots_.end()); return id;
    }
    bool destroyEntity(EntityId id){auto it=nodes_.find(id); if(it==nodes_.end())return false; auto children=it->second.children; for(auto c:children)destroyEntity(c); if(it->second.parent!=InvalidEntity){auto& v=nodes_[it->second.parent].children; v.erase(std::remove(v.begin(),v.end(),id),v.end());} else roots_.erase(std::remove(roots_.begin(),roots_.end(),id),roots_.end()); registry_.destroy(id); nodes_.erase(id); return true;}
    Node* node(EntityId id){auto it=nodes_.find(id);return it==nodes_.end()?nullptr:&it->second;}
    const Node* node(EntityId id) const {auto it=nodes_.find(id);return it==nodes_.end()?nullptr:&it->second;}
    ComponentRegistry& components(){return registry_;} const ComponentRegistry& components() const{return registry_;}
    const std::vector<EntityId>& roots() const{return roots_;}
    const std::unordered_map<EntityId,Node>& nodes() const{return nodes_;}
    std::string worldName() const{return name_;}
    void traverse(EntityId root,const std::function<void(EntityId)>& fn) const {auto n=node(root);if(!n)return;fn(root);for(auto c:n->children)traverse(c,fn);}
    Mat4 worldMatrix(EntityId id) const {
        const auto* n=node(id); if(!n) return Mat4::identity();
        const auto* t=registry_.get<TransformComponent>(id); const Mat4 local=t?t->value.localMatrix():Mat4::identity();
        return n->parent==InvalidEntity ? local : worldMatrix(n->parent)*local;
    }
    Vec3 worldPosition(EntityId id) const { const auto m=worldMatrix(id); return {m.m[12],m.m[13],m.m[14]}; }
private:
    std::string name_; EntityId nextId_=0; std::unordered_map<EntityId,Node> nodes_; std::vector<EntityId> roots_; ComponentRegistry registry_;
};
}
