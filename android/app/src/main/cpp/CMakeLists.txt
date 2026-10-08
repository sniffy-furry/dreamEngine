cmake_minimum_required(VERSION 3.22)
project(dream_engine_android LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
add_library(dream_engine_core STATIC
    "${ENGINE_ROOT}/src/core/engine.cpp"
    "${ENGINE_ROOT}/src/core/event_bus.cpp"
    "${ENGINE_ROOT}/src/core/module.cpp"
    "${ENGINE_ROOT}/src/core/module_loader.cpp"
    "${ENGINE_ROOT}/src/core/registry.cpp"
    "${ENGINE_ROOT}/src/core/python_scripting.cpp"
    "${ENGINE_ROOT}/src/core/log.cpp"
    "${ENGINE_ROOT}/src/core/properties.cpp"
    "${ENGINE_ROOT}/src/ecs/ecs.cpp"
    "${ENGINE_ROOT}/src/scene/scene.cpp"
    "${ENGINE_ROOT}/src/editor/editor.cpp"
    "${ENGINE_ROOT}/src/graphics/gles_device.cpp"
    "${ENGINE_ROOT}/src/assets/assets.cpp"
    "${ENGINE_ROOT}/src/project/project.cpp")
target_include_directories(dream_engine_core PUBLIC "${ENGINE_ROOT}/include")
add_library(tutorial_module SHARED "${ENGINE_ROOT}/modules/tutorial/tutorial_module.cpp")
target_include_directories(tutorial_module PRIVATE "${ENGINE_ROOT}/include")
set_target_properties(tutorial_module PROPERTIES PREFIX "" OUTPUT_NAME "tutorial")
target_link_libraries(tutorial_module PRIVATE log)
add_library(render_cube_module SHARED "${ENGINE_ROOT}/modules/render_cube/render_cube.cpp")
target_include_directories(render_cube_module PRIVATE "${ENGINE_ROOT}/include")
set_target_properties(render_cube_module PROPERTIES PREFIX "" OUTPUT_NAME "render_cube")
target_link_libraries(render_cube_module PRIVATE GLESv3 log)
add_library(dream_engine_android SHARED native_bridge.cpp)
target_include_directories(dream_engine_android PRIVATE "${ENGINE_ROOT}/include")
target_link_libraries(dream_engine_android PRIVATE dream_engine_core android log dl EGL GLESv3)

# CPython 3.14 is prepared by the GitHub workflow / tools script.
set(DREAM_PYTHON_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../build/cpython/${ANDROID_ABI}")
set(DREAM_PYTHON_INCLUDE "${DREAM_PYTHON_ROOT}/include/python3.14")
set(DREAM_PYTHON_LIB "${DREAM_PYTHON_ROOT}/lib/libpython3.14.so")
if(EXISTS "${DREAM_PYTHON_LIB}" AND EXISTS "${DREAM_PYTHON_INCLUDE}/Python.h")
    add_library(dream_python SHARED IMPORTED)
    set_target_properties(dream_python PROPERTIES IMPORTED_LOCATION "${DREAM_PYTHON_LIB}" INTERFACE_INCLUDE_DIRECTORIES "${DREAM_PYTHON_INCLUDE}")
    target_compile_definitions(dream_engine_core PRIVATE DREAM_ENGINE_WITH_PYTHON=1)
    target_include_directories(dream_engine_core PRIVATE "${DREAM_PYTHON_INCLUDE}")
    target_link_libraries(dream_engine_android PRIVATE dream_python)
else()
    message(FATAL_ERROR "CPython Android package not prepared: ${DREAM_PYTHON_ROOT}")
endif()
# The default module is bundled as an asset; after first run the app-private copy is hot-swappable.
set(MODULE_ASSET_DIR "${ENGINE_ROOT}/android/app/src/main/assets/modules")
file(MAKE_DIRECTORY "${MODULE_ASSET_DIR}")
add_custom_command(TARGET tutorial_module POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:tutorial_module>" "${MODULE_ASSET_DIR}/tutorial.so")
add_custom_command(TARGET render_cube_module POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:render_cube_module>" "${MODULE_ASSET_DIR}/render_cube.so")
