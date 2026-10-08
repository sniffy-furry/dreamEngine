#pragma once
#include "Animation.hpp"
#include "Assets.hpp"
#include "Audio.hpp"
#include "ECS.hpp"
#include "Editor.hpp"
#include "Input.hpp"
#include "Jobs.hpp"
#include "Networking.hpp"
#include "PackageManager.hpp"
#include "Physics.hpp"
#include "Prefab.hpp"
#include "Profiler.hpp"
#include "RenderGraph.hpp"
#include "Scene.hpp"
#include "Serialization.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace astra {
class Script { public: virtual ~Script()=default; virtual void onInitialize(Scene&){} virtual void onEnable(Scene&){} virtual void onFixedUpdate(Scene&,float){} virtual void onUpdate(Scene&,float){} virtual void onLateUpdate(Scene&,float){} virtual void onDisable(Scene&){} virtual void onDestroy(Scene&){} };
class Engine {
public:
    Engine(); ~Engine();
    Scene& scene(){return *scene_;}
    JobSystem& jobs(){return *jobs_;}
    PhysicsWorld3D& physics(){return physics_;}
    PhysicsWorld2D& physics2D(){return physics2D_;}
    InputSystem& input(){return input_;}
    AudioSystem& audio(){return audio_;}
    Profiler& profiler(){return profiler_;}
    RenderGraph& renderGraph(){return renderGraph_;}
    ECSWorld& ecs(){return ecs_;}
    AssetDatabase& assets(){return assets_;}
    PackageManager& packages(){return packages_;}
    EditorRegistry& editor(){return editor_;}
    void setFixedStep(double seconds){fixedStep_=seconds;}
    void addScript(EntityId id,std::shared_ptr<Script> script){scripts_.push_back({id,std::move(script),true,false});}
    void runFrame(double deltaSeconds);
    bool saveScene(const std::string& path){return SceneSerializer::save(*scene_,path);}
    double timeSeconds()const{return timeSeconds_;}
    std::uint64_t frameCount()const{return frameCount_;}
private:
    struct ScriptInstance{EntityId entity;std::shared_ptr<Script> script;bool enabled;bool initialized;};
    std::unique_ptr<Scene> scene_; std::unique_ptr<JobSystem> jobs_; PhysicsWorld3D physics_; PhysicsWorld2D physics2D_; InputSystem input_; AudioSystem audio_; Profiler profiler_; RenderGraph renderGraph_; ECSWorld ecs_; AssetDatabase assets_; PackageManager packages_; EditorRegistry editor_; std::vector<ScriptInstance> scripts_; double fixedStep_=1.0/60.0, accumulator_=0,timeSeconds_=0; std::uint64_t frameCount_=0;
};
}
