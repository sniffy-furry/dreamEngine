#include "astra/Engine.hpp"
#include <iostream>
using namespace astra;
class Spinner : public Script { EntityId id_; float angle_=0; public: explicit Spinner(EntityId id):id_(id){} void onUpdate(Scene& s,float dt) override {angle_+=dt; if(auto* t=s.components().get<TransformComponent>(id_)) t->value.rotation=Quat::angleAxis(angle_,{0,1,0});} };
int main(){
    Engine engine; auto root=engine.scene().createEntity("AstraRoot"); auto child=engine.scene().createEntity("Child",root);
    engine.scene().components().get<TransformComponent>(child)->value.position={0,2,0};
    engine.physics().addBody({child,{0,3,0},{0,0,0},{0.5f,0.5f,0.5f},1,false});
    engine.addScript(root,std::make_shared<Spinner>(root));
    auto color=engine.renderGraph().createResource("HDRColor"); auto depth=engine.renderGraph().createResource("Depth");
    engine.renderGraph().addPass({"DepthPrepass",{}, {depth}, []{}});
    engine.renderGraph().addPass({"Opaque",{depth},{color}, []{}});
    engine.renderGraph().addPass({"ToneMap",{color},{}, []{}});
    for(int i=0;i<120;i++)engine.runFrame(1.0/60.0);
    engine.saveScene("Astra.Sample.scene.json");
    std::cout<<"AstraForge sample OK; frames="<<engine.frameCount()<<" childY="<<engine.physics().bodies().front().position.y<<"\n";
    return 0;
}
