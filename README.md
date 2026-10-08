# DreamEngine Core — v0.1

This is the first architectural foundation, not a finished graphics engine.

## Design

The core is intentionally small:
- Engine lifecycle
- Module interface
- Central service registry
- Typed EventBus
- Static module support suitable for Android
- Tutorial module showing communication through the central API

Modules do not depend directly on one another. They receive `EngineAPI`, then use shared services/events.

## Why this structure

For Android we want the hot path to stay native C++ and avoid filesystem IPC or JSON between modules. The central API is an in-memory interface.

Future modules can include:
- Vulkan renderer
- render graph
- ECS
- physics
- audio
- input
- UI
- asset/streaming system
- Python scripting
- SNN/AI
- networking
- profiler

## Graphics target

The architecture does not promise "Forza-level" graphics by itself. Achieving that class of visuals requires a serious renderer, physically based materials, HDR, modern shadows, temporal reconstruction, streaming, LODs, animation, post-processing, GPU profiling, and high-quality assets.

The renderer should therefore be added as a separate module rather than polluting the core.


## Android

The repository now contains an Android application under `android/`.

- Native engine core is compiled with the Android NDK.
- JNI bridge exposes the native engine to the Android Activity.
- ARM64 (`arm64-v8a`) and x86_64 are enabled.
- GitHub Actions builds debug and release APK artifacts.
- The renderer is intentionally not implemented yet; the next graphics layer should be a Vulkan module.

The Android build keeps the game/engine hot path in C++ rather than moving engine logic into Kotlin.
