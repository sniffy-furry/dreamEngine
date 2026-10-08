# AstraForge feature status

Legend: **Implemented** = functional in this repository; **API** = public architecture/extension point exists; **Planned** = requires a platform/backend or substantially larger subsystem.

| Area | Status | Notes |
|---|---|---|
| Scene/entity/component/transform | Implemented | Native scene graph + C# API; local/world transform support. |
| Quaternion + Euler | Implemented | Left-handed Y-up Z-forward convention documented. |
| Prefabs/variants/nesting/overrides | API | Data model present; full diff/merge/prefab editor still planned. |
| Script lifecycle | Implemented | Native base + managed Behaviour lifecycle. |
| Coroutines/async/await | API | C# async is native; coroutine container is exposed. |
| Job system/native containers | Implemented | Native worker pool; managed job handle/native-array API. |
| ECS archetypes/chunks/worlds | Implemented | Compact archetype/chunk runtime skeleton. |
| C# scripting/AOT | API | Managed C# 14 API + native ABI. NativeAOT integration is build-stage work. |
| Input | API | Action maps and device abstractions. |
| Networking | API | Transport/RPC/authority data model; dedicated UDP/relay/lobby backends planned. |
| Render graph | Implemented | Dependency ordered render passes/resources. |
| Render pipelines / Vulkan / D3D / GL / WebGPU | API | Backend contracts and enums; GPU implementations planned. |
| PBR/HDR/GI/shadows/post | API | Data structures/pipeline slots; production GPU implementation planned. |
| Physics 3D | Implemented | Basic rigidbody/AABB gravity + raycast. |
| Physics 2D | API | Separate world/update path. |
| NavMesh / cloth / vehicles / joints | Planned | Requires dedicated production subsystems. |
| Audio | API | 3D emitter/listener model and backend interface. |
| Animation | API | Clips/state/animator data model. |
| UI | Implemented/API | Canvas/text/button model; retained-mode styling/data binding planned. |
| XR/AR/VR | API | Provider abstraction. |
| Assets/importers/addressables/bundles | API | GUID/sidecar/registry exists; high-volume import + CDN/content patching planned. |
| Serialization | Implemented/API | Native JSON scene writer + managed JSON/YAML writer. Binary persistence remains planned. |
| Package manager/assembly definitions | Implemented | Manifest/lock + asmdef examples. |
| Editor panels/debuggers | API | Extensibility types and editor shell; full GUI is planned. |
| Profiling/memory/frame debugger | Implemented/API | CPU profiler present; full GPU/memory debugger planned. |
| Build/deployment | API | Cross-platform CMake/build scripts for Linux/Windows/Android. |
