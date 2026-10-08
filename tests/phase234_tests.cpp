#include "dream/ecs/ecs.hpp"
#include "dream/scene/scene.hpp"
#include "dream/editor/editor.hpp"
#include "dream/assets/assets.hpp"
#include "dream/project/project.hpp"
#include <cstdio>
#include <iostream>
#include <string>

namespace {
int failures = 0;
void check(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        std::cerr << "[FAIL] " << file << ':' << line << ": " << expression << '\n';
        ++failures;
    }
}
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

int main() {
    dream::ecs::World w;
    auto e = w.create();
    w.add_transform(e).px = 3;
    CHECK(w.alive(e));
    CHECK(w.destroy(e));
    CHECK(!w.alive(e));
    auto e2 = w.create();
    CHECK(e2.generation != e.generation);

    dream::scene::Scene s;
    s.name = "Test";
    auto x = s.world.create();
    s.world.add_transform(x).py = 4;
    auto text = dream::scene::serialize(s);

    dream::scene::Scene loaded;
    const bool deserialized = dream::scene::deserialize(loaded, text);
    CHECK(deserialized);
    CHECK(loaded.world.entities().size() == 1);
    if (loaded.world.entities().size() == 1) {
        auto t = loaded.world.transform(loaded.world.entities()[0]);
        CHECK(t != nullptr);
        if (t) CHECK(t->py == 4);
    }

    dream::editor::Core ed;
    auto n = ed.create_entity("camera");
    const bool selected = ed.select(n.index);
    CHECK(selected);
    if (selected) CHECK(ed.inspector_text().find("Camera") != std::string::npos);

    dream::assets::Manager am;
    auto id = am.register_asset("models/test.mesh", dream::assets::Type::Mesh);
    CHECK(am.find(id) != nullptr);
    CHECK(am.find_path("models/test.mesh") != nullptr);
    CHECK(am.mark_loaded(id));

    dream::project::Project p;
    p.name = "Game";
    std::string err;
    const bool project_saved = dream::project::save(p, "phase234.project", &err);
    CHECK(project_saved);
    dream::project::Project q;
    if (project_saved) {
        CHECK(dream::project::load(q, "phase234.project", &err));
        CHECK(q.name == "Game");
    }
    std::remove("phase234.project");

    if (failures != 0) {
        std::cerr << "DreamEngine phase 2-4 tests: " << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "DreamEngine phase 2-4 tests: PASS\n";
    return 0;
}
