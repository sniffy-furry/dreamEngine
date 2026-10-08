# DreamEngine v0.6 — hot-swappable C++ modules + embedded CPython

This version keeps the DreamEngine core inside the APK while native C++ modules and Python scripts are replaceable without rebuilding the APK.

## Hot-swap flow

1. Build a replacement `.so` or edit a `.py` script.
2. Put it in `Download/DreamEngine/modules/`.
3. Open the app and choose **Import modules from Downloads**.
4. The app copies `.so` and `.py` files into its private `files/modules` directory.
5. Restart the app. The core validates and loads the native modules. Python scripts are executed by embedded CPython.

The bundled `tutorial.so` is copied only when a file with the same name does not already exist, so an imported replacement is not overwritten on the next launch.

## Native module ABI

Modules export only these C ABI symbols:
- `dream_module_get_descriptor`
- `dream_module_create`
- `dream_module_destroy`

ABI version is `1`. STL containers, C++ classes, exceptions, and ownership do not cross the module boundary.

## CPython

The Android build embeds official CPython 3.14 Android packages. CPython is initialized in embedded mode, with its standard library and architecture-specific extension modules extracted from APK assets into the app-private `files/python/<abi>` directory. This follows CPython's Android embedding model.

Pure Python gameplay scripts live in `files/modules/*.py` and can be replaced without rebuilding any `.so`. A script may define `on_update(dt)`; DreamEngine calls it from the Android game tick.

The GitHub workflow runs `tools/prepare_cpython_android.py` before Gradle. That script downloads the official CPython Android packages for `aarch64` and `x86_64`, stages their shared libraries for packaging, and stages the standard library as assets.

## Verification

The host build remains Python-free and must compile with CPython disabled. Android builds require the CPython preparation step.
