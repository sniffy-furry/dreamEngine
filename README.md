# AstraForge Engine 0.1

A cross-platform, scene-oriented game-engine foundation targeting **Android (ARM64), Linux, and Windows** with a native runtime core and a C# 14/.NET 10 managed scripting SDK.

> This is an intentionally honest v0.1 foundation, not a claim of feature-for-feature parity with a mature commercial engine. The architecture exposes the requested systems and implements a substantial runtime subset; platform/render/editor integrations are isolated behind interfaces so they can be completed without rewriting the scene/runtime model.

## Implemented in this drop

- Scene + entity hierarchy, parent/child transforms, local-space data, quaternion rotation and left-handed Y-up/Z-forward conventions.
- Component model and lifecycle hooks (initialize/enable/fixed/update/late/disable/destroy).
- Prefab/override data model and serialization-ready structures.
- Asset GUID records, sidecar `.meta` concept, asset database registry.
- Package manifests and assembly definitions.
- Managed C# API designed for .NET 10 / C# 14, plus P/Invoke bridge into the native runtime.
- Native job system with dependency-friendly futures; C# job handles and native arrays.
- Archetype/chunk ECS skeleton with worlds and systems.
- Input action map and device abstraction.
- Render graph with explicit resources/read/write dependencies and pass ordering.
- PBR/material/light descriptors, graphics-backend abstraction.
- 3D AABB rigidbody simulation + raycast and separate 2D physics step abstraction.
- Audio, animation, UI, XR, networking, native plugin, profiling and build pipeline interfaces/data models.
- Native sample, native unit tests, C# sample.

## Architecture

`managed/Astra.Managed` is the user-facing engine SDK. `native/Astra.Native` owns high-performance runtime services and an ABI-stable C API used by the managed layer. A platform backend can be added for Win32/X11/Wayland/Android without changing gameplay code.

The coordinate convention is **left-handed, Y-up, +Z forward, metres**. C# uses `System.Numerics` for storage/math while the engine API documents this convention.

## Build

### Native core

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/Astra.Sample
```

### Managed SDK

Install the .NET 10 SDK, then:

```bash
dotnet build managed/Astra.Managed/Astra.Managed.csproj -c Release
dotnet run --project managed/SampleGame/SampleGame.csproj -c Release
```

### Android

Use Android Studio/SDK + NDK and cross-compile the native library for `arm64-v8a`. The managed project is deliberately free of external packages; an Android host/application can load the same `Astra.Native` ABI and wire the surface/input/audio backends.

## Feature boundaries

The following are **interfaces/extension points in v0.1**, not finished production subsystems: GPU implementations for every listed API, full editor GUI, complete FBX/video/spreadsheet importers, high-end PBR/lighting/GI/shadow pipeline, full 2D/3D joints/vehicles/cloth, NavMesh/crowd baking, FMOD/Wwise adapters, OpenXR/ARKit/ARCore concrete providers, relay/lobby/matchmaking, asset bundles/addressables, actual visual-scripting graph runtime, LLVM Burst-style compiler, and platform-specific console/TV/automotive backends.

These are intentionally isolated so they can be added as packages/backends rather than contaminating the core API.

## Repository map

- `native/` native runtime, math, ECS, scene graph, physics, render graph, interop, tests.
- `managed/` C# engine SDK and sample game.
- `docs/` architecture notes and platform strategy.
- `tools/editor/` editor roadmap and command-shell foundation.
- `build_scripts/` build/CI entry points.
