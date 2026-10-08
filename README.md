# DreamEngine v0.5 — hot-swappable modules

The Android app keeps the core in the APK and loads native modules from its private `files/modules` directory.
On first launch the default `tutorial.so` is copied from APK assets. The app has an **Import modules from Downloads** button using Android's document picker. Put `.so` and `.py` files in a folder such as `Download/DreamEngine/modules`, select that folder, then restart the app to activate native module replacements.

## Native module ABI

Modules export only these C ABI symbols:
- `dream_module_get_descriptor`
- `dream_module_create`
- `dream_module_destroy`

ABI version is currently `1`. No STL, C++ classes, exceptions, or ownership cross the module boundary.

## Python

`.py` files are accepted and indexed by the engine in the same hot-swap directory. The `PythonScriptManager` is deliberately isolated from the native module ABI so an embedded CPython runtime can be enabled later without changing the module ABI. This build does **not** claim to execute Python yet; it discovers scripts and keeps their paths ready for the Python runtime layer.
