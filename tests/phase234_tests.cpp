#include "dream/ecs/ecs.hpp"
#include "dream/scene/scene.hpp"
#include "dream/editor/editor.hpp"
#include "dream/assets/assets.hpp"
#include "dream/project/project.hpp"
#include <cassert>
#include <fstream>
int main(){
 dream::ecs::World w; auto e=w.create(); w.add_transform(e).px=3; assert(w.alive(e)); assert(w.destroy(e)); assert(!w.alive(e)); auto e2=w.create(); assert(e2.generation!=e.generation);
 dream::scene::Scene s; s.name="Test"; auto x=s.world.create(); s.world.add_transform(x).py=4; auto text=dream::scene::serialize(s); dream::scene::Scene loaded; assert(dream::scene::deserialize(loaded,text)); assert(loaded.world.entities().size()==1); auto t=loaded.world.transform(loaded.world.entities()[0]); assert(t&&t->py==4);
 dream::editor::Core ed; auto n=ed.create_entity("camera"); assert(ed.select(n.index)); assert(ed.inspector_text().find("Camera")!=std::string::npos);
 dream::assets::Manager am; auto id=am.register_asset("models/test.mesh",dream::assets::Type::Mesh); assert(am.find(id)); assert(am.find_path("models/test.mesh")); assert(am.mark_loaded(id));
 dream::project::Project p; p.name="Game"; std::string err; assert(dream::project::save(p,"phase234.project",&err)); dream::project::Project q; assert(dream::project::load(q,"phase234.project",&err)); assert(q.name=="Game"); std::remove("phase234.project"); return 0;
}
