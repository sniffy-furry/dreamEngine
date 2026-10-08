cmake_minimum_required(VERSION 3.22)
project(DreamEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DREAM_ENGINE_BUILD_TUTORIAL "Build tutorial module" OFF)

add_library(dream_engine_core STATIC
    src/core/engine.cpp
    src/core/event_bus.cpp
    src/core/module.cpp
    src/core/module_loader.cpp
    src/core/registry.cpp
    src/core/python_scripting.cpp
    src/core/log.cpp
    src/core/properties.cpp
)
target_include_directories(dream_engine_core PUBLIC include)
target_compile_features(dream_engine_core PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(dream_engine_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(dream_engine_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(DREAM_ENGINE_BUILD_TUTORIAL)
    add_subdirectory(modules/tutorial)
endif()

add_executable(engine_demo examples/main.cpp)
target_link_libraries(engine_demo PRIVATE dream_engine_core)
