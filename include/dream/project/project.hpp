#pragma once
#include <string>
namespace dream::project {
struct Project{std::string name="DreamProject";std::string start_scene="scenes/main.scene";int format=1;};
bool save(const Project&,const std::string&,std::string*err=nullptr); bool load(Project&,const std::string&,std::string*err=nullptr);
}
