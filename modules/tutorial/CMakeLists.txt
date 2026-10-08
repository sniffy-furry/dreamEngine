add_library(tutorial_module SHARED tutorial_module.cpp)
target_include_directories(tutorial_module PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(tutorial_module PRIVATE cxx_std_20)
set_target_properties(tutorial_module PROPERTIES PREFIX "" OUTPUT_NAME "tutorial")
