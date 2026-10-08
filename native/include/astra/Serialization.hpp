#pragma once
#include "Scene.hpp"
#include <fstream>
#include <sstream>
#include <string>

namespace astra {
class SceneSerializer {
public:
    static bool save(const Scene& scene,const std::string& path){
        std::ofstream out(path,std::ios::binary); if(!out)return false;
        out << "{\n  \"format\": 1,\n  \"scene\": \"" << escape(scene.worldName()) << "\",\n  \"entities\": [\n";
        bool first=true; for(const auto& [id,n]:scene.nodes()){
            if(!first) out<<",\n";
            first=false;
            const auto* t=scene.components().get<TransformComponent>(id);
            out << "    {\"id\":"<<id<<",\"name\":\""<<escape(n.name)<<"\",\"parent\":"<<n.parent<<",\"active\":"<<(n.active?"true":"false");
            if(t)out<<",\"position\":["<<t->value.position.x<<","<<t->value.position.y<<","<<t->value.position.z<<"]";
            out<<"}";
        }
        out << "\n  ]\n}\n"; return static_cast<bool>(out);
    }
private:
    static std::string escape(const std::string& s){std::string r;for(char c:s){if(c=='\\'||c=='\"')r+='\\';r+=c;}return r;}
};
}
