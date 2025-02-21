cmake_minimum_required(VERSION 3.12)

project("ChessEngine_V4" LANGUAGES CXX)


add_executable(ChessEngine_V4 "src/main.cpp" "src/Renderer.cpp" "src/Renderer.h" "src/Board.cpp" "src/Board.h" "src/MoveGenerator.cpp" "src/MoveGenerator.h" "src/Timer.cpp" "src/Timer.h" "src/Precomputation.cpp" "src/Precomputation.h" "src/Perft.h" "src/Perft.cpp" "src/Game.cpp" "src/Game.h" "src/UCI.h" "src/UCI.cpp" "src/Search.h" "src/Search.cpp" "src/Opening.cpp" "src/Opening.h")


include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
)

FetchContent_MakeAvailable(raylib)

target_link_libraries(ChessEngine_V4 raylib)

set_target_properties(ChessEngine_V4 PROPERTIES
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" 
)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/assets" # Destination directory (relative to build dir)
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets" # Dependencies for rebuilds
    COMMENT "Copying assets" # Optional comment
    VERBATIM # Important for paths with spaces
)


# Use C++20
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ChessEngine_V4 PROPERTY CXX_STANDARD 20)
endif()
