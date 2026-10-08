# DreamEngine Editor Rework — Android Phase 4

This revision replaces the previous prototype-style Android UI with a real editor shell.

## Editor layout

- top menu bar
- scene/play toolbar
- move/rotate/scale tool selection
- Hierarchy panel
- central Scene View backed by the engine render surface
- Inspector panel
- bottom Project / Console / Profiler tabs
- GameObject / Camera / Light creation
- selection and deletion
- scene save
- module import + hot reload
- move tool drag editing of the selected Transform

## Architecture

The Android layer owns presentation and touch input. Editor state remains in the native `dream::editor::Core` and ECS. The central render surface is a `TextureView` so the editor chrome can be drawn above the live engine render surface without using the old split-screen demo layout.

This is the foundation for the next editor phases: real component inspectors, hierarchy parenting, gizmo rendering, undo/redo, asset browser, docking/resizable panels, and richer scene rendering.

## Verification

The host Release regression suite was rebuilt after the editor changes:

`DreamEngine phase 2-4 tests: PASS`

Android APK compilation still depends on the GitHub Actions environment because the host environment does not contain the Android SDK/Gradle toolchain or the prepared CPython Android package.
