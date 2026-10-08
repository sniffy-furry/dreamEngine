#pragma once
#include "Math.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>
namespace astra {
struct Ray { Vec3 origin{}, direction{0,0,1}; };
struct Aabb { Vec3 min{},max{}; };
struct RaycastHit { bool hit=false; float distance=0; Vec3 point{},normal{}; std::uint64_t entity=0; };
class PhysicsWorld3D {
public:
    struct Body { std::uint64_t entity=0; Vec3 position{},velocity{}; Vec3 halfExtents{0.5f,0.5f,0.5f}; float mass=1; bool kinematic=false; };
    std::uint64_t addBody(Body b){bodies_.push_back(b);return b.entity;}
    void fixedStep(float dt){for(auto& b:bodies_)if(!b.kinematic){b.velocity.y-=9.81f*dt;b.position+=b.velocity*dt;if(b.position.y-b.halfExtents.y<0){b.position.y=b.halfExtents.y;if(b.velocity.y<0)b.velocity.y*=-0.2f;}}}
    RaycastHit raycast(Ray ray,float maxDistance=1000) const {ray.direction=ray.direction.normalized();RaycastHit best{};best.distance=maxDistance;for(const auto& b:bodies_){Aabb a{b.position-b.halfExtents,b.position+b.halfExtents};float tmin=0,tmax=maxDistance;for(int axis=0;axis<3;axis++){float o=axis==0?ray.origin.x:axis==1?ray.origin.y:ray.origin.z;float d=axis==0?ray.direction.x:axis==1?ray.direction.y:ray.direction.z;float mn=axis==0?a.min.x:axis==1?a.min.y:a.min.z;float mx=axis==0?a.max.x:axis==1?a.max.y:a.max.z;if(std::fabs(d)<1e-6f){if(o<mn||o>mx){tmin=tmax+1;break;}}else{float t1=(mn-o)/d,t2=(mx-o)/d;if(t1>t2)std::swap(t1,t2);tmin=std::max(tmin,t1);tmax=std::min(tmax,t2);if(tmin>tmax)break;}}if(tmin<=tmax&&tmin<best.distance){best={true,tmin,ray.origin+ray.direction*tmin,{0,1,0},b.entity};}}return best;}
    const std::vector<Body>& bodies() const{return bodies_;}
private:std::vector<Body>bodies_;
};
class PhysicsWorld2D { public: struct Body{std::uint64_t entity=0; float x=0,y=0,vx=0,vy=0;}; void step(float dt){for(auto& b:bodies_){b.vy-=9.81f*dt;b.x+=b.vx*dt;b.y+=b.vy*dt;if(b.y<0){b.y=0;if(b.vy<0)b.vy*=-0.2f;}}} void add(Body b){bodies_.push_back(b);} private: std::vector<Body>bodies_;};
}
