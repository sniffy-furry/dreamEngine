add_library(tutorial_module STATIC
    tutorial_module.cpp
)
target_include_directories(tutorial_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tutorial_module PUBLIC dream_engine_core)
