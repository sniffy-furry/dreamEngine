#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace dream::ecs {
using EntityIndex = uint32_t;
using Generation = uint32_t;
struct Entity { EntityIndex index=0; Generation generation=0; friend bool operator==(Entity a,Entity b){return a.index==b.index&&a.generation==b.generation;} };
constexpr Entity kNullEntity{0xFFFFFFFFu,0};

struct Transform { float px=0,py=0,pz=0; float rx=0,ry=0,rz=0; float sx=1,sy=1,sz=1; };
struct Camera { float fov=70, near_plane=.1f, far_plane=1000; };
struct MeshRenderer { uint64_t mesh=0, material=0; };
struct Light { float r=1,g=1,b=1; float intensity=1; float range=10; };

class World final {
public:
 Entity create(); bool destroy(Entity); bool alive(Entity) const noexcept;
 Transform& add_transform(Entity); Camera& add_camera(Entity); MeshRenderer& add_mesh(Entity); Light& add_light(Entity);
 Transform* transform(Entity) noexcept; const Transform* transform(Entity) const noexcept; Camera* camera(Entity) noexcept; const Camera* camera(Entity) const noexcept; MeshRenderer* mesh(Entity) noexcept; const MeshRenderer* mesh(Entity) const noexcept; Light* light(Entity) noexcept; const Light* light(Entity) const noexcept;
 const std::vector<Entity>& entities() const noexcept { return entities_; }
 void clear();
private:
 struct Slot { Generation generation=1; bool alive=false; };
 std::vector<Slot> slots_; std::vector<Entity> entities_; std::vector<EntityIndex> free_;
 std::unordered_map<EntityIndex,Transform> transforms_; std::unordered_map<EntityIndex,Camera> cameras_;
 std::unordered_map<EntityIndex,MeshRenderer> meshes_; std::unordered_map<EntityIndex,Light> lights_;
};
}
