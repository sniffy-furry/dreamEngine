cmake_minimum_required(VERSION 3.20)
project(AstraForgeEngine VERSION 0.1.0 LANGUAGES CXX)

option(ASTRA_BUILD_TESTS "Build native tests" ON)
option(ASTRA_BUILD_SAMPLE "Build sample runtime" ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(Astra.Native SHARED
    native/src/Engine.cpp
    native/src/Interop.cpp
)
target_include_directories(Astra.Native PUBLIC native/include)
target_compile_definitions(Astra.Native PRIVATE ASTRA_BUILDING_NATIVE=1)
if (MSVC)
  target_compile_options(Astra.Native PRIVATE /W4 /permissive-)
else()
  target_compile_options(Astra.Native PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if (WIN32)
  target_link_libraries(Astra.Native PRIVATE ws2_32)
else()
  target_link_libraries(Astra.Native PRIVATE pthread dl)
endif()

if (ASTRA_BUILD_SAMPLE)
  add_executable(Astra.Sample native/src/main.cpp)
  target_link_libraries(Astra.Sample PRIVATE Astra.Native)
  target_include_directories(Astra.Sample PRIVATE native/include)
endif()

if (ASTRA_BUILD_TESTS)
  enable_testing()
  add_executable(Astra.NativeTests native/tests/NativeTests.cpp)
  target_link_libraries(Astra.NativeTests PRIVATE Astra.Native)
  target_include_directories(Astra.NativeTests PRIVATE native/include)
  add_test(NAME Astra.NativeTests COMMAND Astra.NativeTests)
endif()
