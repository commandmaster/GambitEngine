cmake_minimum_required(VERSION 3.10)
project(GambitEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
if(MSVC)
    add_compile_options(/O2 /arch:AVX2)
else()
    add_compile_options(-O3 -march=native -ftemplate-depth=2048)
endif()

# Include directories
include_directories(GambitApp/src)

# Pybind11 for Python bindings
include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11
  GIT_TAG        v2.11.1
)
FetchContent_MakeAvailable(pybind11)

# Source files
set(SOURCES
    GambitApp/src/Board.cpp
    GambitApp/src/MoveGenerator.cpp
    GambitApp/src/Opening.cpp
    GambitApp/src/Perft.cpp
    GambitApp/src/Precomputation.cpp
    GambitApp/src/Timer.cpp
    GambitApp/src/UCI.cpp
    GambitApp/src/HeadlessMain.cpp
)

# Add executable
add_executable(gambit_uci ${SOURCES})

# Link libraries (pthread for Linux/Unix)
if(UNIX)
    target_link_libraries(gambit_uci pthread)
endif()

# Python Module
pybind11_add_module(gambit_engine
    GambitApp/src/Bindings.cpp
    GambitApp/src/Board.cpp
    GambitApp/src/MoveGenerator.cpp
    GambitApp/src/Opening.cpp
    GambitApp/src/Perft.cpp
    GambitApp/src/Precomputation.cpp
    GambitApp/src/Timer.cpp
    # Excluding UCI.cpp and HeadlessMain.cpp as they are for the standalone app
)

if(UNIX)
    target_link_libraries(gambit_engine PRIVATE pthread)
endif()
