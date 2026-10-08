#include "astra/Engine.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    using namespace astra;
    { Vec3 a{1,0,0},b{0,1,0}; auto c=Vec3::cross(a,b); assert(c.z==1); }
    { Scene s("Test"); auto root=s.createEntity("Root"); auto child=s.createEntity("Child",root); assert(s.node(child)->parent==root); assert(s.roots().size()==1); assert(s.destroyEntity(root)); assert(s.node(child)==nullptr); }
    { RenderGraph g; auto a=g.createResource("A"),b=g.createResource("B"); int seq=0; g.addPass({"P1",{}, {a}, [&]{assert(seq==0);seq=1;}}); g.addPass({"P2",{a},{b}, [&]{assert(seq==1);seq=2;}}); g.addPass({"P3",{b},{}, [&]{assert(seq==2);seq=3;}}); g.execute(); assert(seq==3); }
    { PhysicsWorld3D p; p.addBody({42,{0,2,0},{0,0,0},{0.5f,0.5f,0.5f},1,false}); for(int i=0;i<120;i++)p.fixedStep(1.0f/60.0f); assert(p.bodies()[0].position.y>=0.49f); auto h=p.raycast({{0,0.5f,-5},{0,0,1}},20); assert(h.hit); }
    { JobSystem jobs(2); auto a=jobs.submit([]{return 21;}); auto b=jobs.submit([]{return 21;}); assert(a.get()+b.get()==42); }
    std::cout<<"All native tests passed.\n"; return 0;
}
