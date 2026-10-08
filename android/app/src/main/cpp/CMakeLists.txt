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
    "${ENGINE_ROOT}/src/core/python_scripting.cpp")
target_include_directories(dream_engine_core PUBLIC "${ENGINE_ROOT}/include")
add_library(tutorial_module SHARED "${ENGINE_ROOT}/modules/tutorial/tutorial_module.cpp")
target_include_directories(tutorial_module PRIVATE "${ENGINE_ROOT}/include")
set_target_properties(tutorial_module PROPERTIES PREFIX "" OUTPUT_NAME "tutorial")
target_link_libraries(tutorial_module PRIVATE log)
add_library(dream_engine_android SHARED native_bridge.cpp)
target_include_directories(dream_engine_android PRIVATE "${ENGINE_ROOT}/include")
target_link_libraries(dream_engine_android PRIVATE dream_engine_core android log dl)
# The default module is bundled as an asset; after first run the app-private copy is hot-swappable.
set(MODULE_ASSET_DIR "${ENGINE_ROOT}/android/app/src/main/assets/modules")
file(MAKE_DIRECTORY "${MODULE_ASSET_DIR}")
add_custom_command(TARGET tutorial_module POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:tutorial_module>" "${MODULE_ASSET_DIR}/tutorial.so")
