#pragma once
#include "Math.hpp"
#include "Guid.hpp"
#include <string>
#include <cstdint>

namespace astra {
struct TransformComponent { Transform value{}; };
struct CameraComponent { float fovYRadians=1.04719755f; float nearClip=0.03f; float farClip=1000.0f; bool orthographic=false; float orthoSize=10.0f; };
struct MeshRendererComponent { Guid mesh{}; Guid material{}; bool castShadows=true; bool receiveShadows=true; };
struct RigidbodyComponent { Vec3 velocity{}; Vec3 angularVelocity{}; float mass=1.0f; bool kinematic=false; float restitution=0.1f; float friction=0.5f; };
struct BoxColliderComponent { Vec3 halfExtents{0.5f,0.5f,0.5f}; bool isTrigger=false; };
struct TagComponent { std::string tag; };
}
