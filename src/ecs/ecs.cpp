#include "dream/ecs/ecs.hpp"
namespace dream::ecs {
Entity World::create(){ EntityIndex i; if(!free_.empty()){i=free_.back();free_.pop_back();}else{i=(EntityIndex)slots_.size();slots_.push_back({});} auto &s=slots_[i];s.alive=true; Entity e{i,s.generation};entities_.push_back(e);return e; }
bool World::alive(Entity e) const noexcept{return e.index<slots_.size()&&slots_[e.index].alive&&slots_[e.index].generation==e.generation;}
bool World::destroy(Entity e){if(!alive(e))return false;slots_[e.index].alive=false;++slots_[e.index].generation;transforms_.erase(e.index);cameras_.erase(e.index);meshes_.erase(e.index);lights_.erase(e.index);for(size_t i=0;i<entities_.size();++i)if(entities_[i]==e){entities_[i]=entities_.back();entities_.pop_back();break;}free_.push_back(e.index);return true;}
Transform& World::add_transform(Entity e){return transforms_[e.index];} Camera& World::add_camera(Entity e){return cameras_[e.index];} MeshRenderer& World::add_mesh(Entity e){return meshes_[e.index];} Light& World::add_light(Entity e){return lights_[e.index];}
Transform* World::transform(Entity e)noexcept{return alive(e)?([&]()->Transform*{auto i=transforms_.find(e.index);return i==transforms_.end()?nullptr:&i->second;})():nullptr;}
Camera* World::camera(Entity e)noexcept{return alive(e)?([&]()->Camera*{auto i=cameras_.find(e.index);return i==cameras_.end()?nullptr:&i->second;})():nullptr;}
MeshRenderer* World::mesh(Entity e)noexcept{return alive(e)?([&]()->MeshRenderer*{auto i=meshes_.find(e.index);return i==meshes_.end()?nullptr:&i->second;})():nullptr;}
Light* World::light(Entity e)noexcept{return alive(e)?([&]()->Light*{auto i=lights_.find(e.index);return i==lights_.end()?nullptr:&i->second;})():nullptr;}
void World::clear(){slots_.clear();entities_.clear();free_.clear();transforms_.clear();cameras_.clear();meshes_.clear();lights_.clear();}
}

namespace dream::ecs { const Transform* World::transform(Entity e) const noexcept{return alive(e)?([&]()->const Transform*{auto i=transforms_.find(e.index);return i==transforms_.end()?nullptr:&i->second;})():nullptr;} const Camera* World::camera(Entity e) const noexcept{return alive(e)?([&]()->const Camera*{auto i=cameras_.find(e.index);return i==cameras_.end()?nullptr:&i->second;})():nullptr;} const MeshRenderer* World::mesh(Entity e) const noexcept{return alive(e)?([&]()->const MeshRenderer*{auto i=meshes_.find(e.index);return i==meshes_.end()?nullptr:&i->second;})():nullptr;} const Light* World::light(Entity e) const noexcept{return alive(e)?([&]()->const Light*{auto i=lights_.find(e.index);return i==lights_.end()?nullptr:&i->second;})():nullptr;} }
