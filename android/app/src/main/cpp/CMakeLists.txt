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
)

target_include_directories(dream_engine_core PUBLIC
    "${ENGINE_ROOT}/include"
)

add_library(tutorial_module STATIC
    "${ENGINE_ROOT}/modules/tutorial/tutorial_module.cpp"
)

target_include_directories(tutorial_module PUBLIC
    "${ENGINE_ROOT}/modules/tutorial"
)

target_link_libraries(tutorial_module PUBLIC dream_engine_core)

add_library(dream_engine_android SHARED
    native_bridge.cpp
)

target_include_directories(dream_engine_android PRIVATE
    "${ENGINE_ROOT}/include"
    "${ENGINE_ROOT}/modules/tutorial"
)

target_link_libraries(dream_engine_android PRIVATE
    tutorial_module
    dream_engine_core
    android
    log
)
