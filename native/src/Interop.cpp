#include "astra/Engine.hpp"
#include <memory>
#if defined(_WIN32)
#define ASTRA_EXPORT extern "C" __declspec(dllexport)
#else
#define ASTRA_EXPORT extern "C" __attribute__((visibility("default")))
#endif
struct AstraHandle { std::unique_ptr<astra::Engine> engine; };
ASTRA_EXPORT AstraHandle* astra_create(){return new AstraHandle{std::make_unique<astra::Engine>()};}
ASTRA_EXPORT void astra_destroy(AstraHandle* h){delete h;}
ASTRA_EXPORT std::uint64_t astra_create_entity(AstraHandle* h,const char* name){return h?h->engine->scene().createEntity(name?name:"GameObject"):0;}
ASTRA_EXPORT void astra_set_position(AstraHandle* h,std::uint64_t e,float x,float y,float z){if(!h)return;auto* t=h->engine->scene().components().get<astra::TransformComponent>(e);if(t)t->value.position={x,y,z};}
ASTRA_EXPORT void astra_step(AstraHandle* h,double dt){if(h)h->engine->runFrame(dt);}
ASTRA_EXPORT std::uint64_t astra_frame_count(AstraHandle* h){return h?h->engine->frameCount():0;}
ASTRA_EXPORT int astra_save_scene(AstraHandle* h,const char* path){return h&&path&&h->engine->saveScene(path)?1:0;}
