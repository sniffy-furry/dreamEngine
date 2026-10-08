# DreamEngine Revamp — Phases 2–4

## Phase 2 — Rendering foundation
- Render Hardware Interface (`dream/graphics/rhi.hpp`)
- OpenGL ES 3 device seam for Android
- Existing render-module ABI remains compatible

## Phase 3 — World / project foundation
- Generation-safe ECS
- Transform, Camera, MeshRenderer and Light components
- Scene serialization/loading (`*.scene`)
- Asset registry with stable IDs/revisions
- Project descriptor (`project.dream`)

## Phase 4 — Android editor foundation
- Platform-independent Editor Core
- Hierarchy selection/deletion/creation
- Transform/component inspector
- Android editor controls for Entity/Camera/Light/Delete/Save Scene
- Android stores editor scenes under app-private `files/project/scenes`
- Editor and runtime share the same ECS/scene representation

## Verification
Host C++ regression suite: PASS.
Android Gradle build: CI-only in this environment; not claimed as locally executed.
