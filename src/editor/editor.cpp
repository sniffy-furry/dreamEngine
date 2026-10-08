#include "dream/editor/editor.hpp"
#include <sstream>
namespace dream::editor {
Core::Core(){auto e=scene_.world.create();scene_.world.add_transform(e);state_.selection.entity=e;state_.dirty=true;}
ecs::Entity Core::create_entity(const std::string&kind){auto e=scene_.world.create();scene_.world.add_transform(e);if(kind=="camera")scene_.world.add_camera(e);else if(kind=="light")scene_.world.add_light(e);else if(kind=="mesh")scene_.world.add_mesh(e);state_.selection.entity=e;state_.dirty=true;return e;}
bool Core::delete_selected(){if(!scene_.world.alive(state_.selection.entity))return false;auto ok=scene_.world.destroy(state_.selection.entity);state_.selection.entity=ecs::kNullEntity;state_.dirty|=ok;return ok;}
bool Core::select(uint32_t index){for(auto e:scene_.world.entities())if(e.index==index){state_.selection.entity=e;return true;}return false;}
bool Core::save_scene(std::string*err){if(state_.scene_path.empty()){if(err)*err="scene path is empty";return false;}if(!scene::save(scene_,state_.scene_path,err))return false;state_.dirty=false;return true;}
bool Core::open_scene(const std::string&p,std::string*err){scene::Scene loaded;if(!scene::load(loaded,p,err))return false;scene_=std::move(loaded);state_.scene_path=p;state_.dirty=false;state_.selection.entity=ecs::kNullEntity;return true;}
std::string Core::hierarchy_text()const{
    std::ostringstream o;
    for(auto e:scene_.world.entities()){
        const char* kind = scene_.world.camera(e) ? "Camera" :
                           scene_.world.light(e) ? "Light" :
                           scene_.world.mesh(e) ? "Mesh" : "GameObject";
        o << e.index << (e==state_.selection.entity ? " * " : " ") << kind << "\n";
    }
    return o.str();
}
std::string Core::inspector_text()const{auto e=state_.selection.entity;if(!scene_.world.alive(e))return "No selection";std::ostringstream o;o<<"Entity "<<e.index<<"\n";if(auto*t=scene_.world.transform(e))o<<"Transform\nPosition: "<<t->px<<", "<<t->py<<", "<<t->pz<<"\nRotation: "<<t->rx<<", "<<t->ry<<", "<<t->rz<<"\nScale: "<<t->sx<<", "<<t->sy<<", "<<t->sz<<"\n";if(scene_.world.camera(e))o<<"Camera\n";if(scene_.world.mesh(e))o<<"MeshRenderer\n";if(scene_.world.light(e))o<<"Light\n";return o.str();}
}
