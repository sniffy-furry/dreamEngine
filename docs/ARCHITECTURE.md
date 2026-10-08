# AstraForge Architecture

## Runtime layers

1. **Platform** — window/surface, clocks, file system, threading, input, sockets, audio device, GPU context.
2. **Native core** — scene graph, transforms, component storage, jobs, ECS, physics, render graph, asset registry, profiling.
3. **Managed runtime** — C# behaviours, serialization, packages, editor extensions, async/coroutines, visual scripting API.
4. **Presentation** — render pipelines, UI, animation, audio, XR.
5. **Editor** — scene/game views, hierarchy, inspector, project browser, console, profiler/debuggers, importers.

## Update order

`Input -> Fixed simulation -> Script FixedUpdate -> Script Update -> Animation -> Script LateUpdate -> RenderGraph -> Present`.

## Serialization contract

Scenes/prefabs should be deterministic, text-friendly and VCS-friendly. Asset files receive sidecar metadata containing the GUID, importer version/settings, and dependency list. Binary caches are disposable derived data.

## Package/assembly contract

Packages contain a manifest plus lock data. Assembly definitions declare compile boundaries, references, unsafe usage and target platforms. Editor-only code must not leak into player assemblies.

## Platform strategy

- **Windows:** Win32 surface + Vulkan/D3D12/D3D11 adapters; optional OpenGL compatibility.
- **Linux:** X11/Wayland surface + Vulkan/OpenGL adapters.
- **Android:** NativeActivity/GameActivity host + Vulkan/GLES; ARM64 primary target.
- Gameplay code remains platform-neutral through the managed abstractions.
