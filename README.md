# Gambit Engine

A high-performance Chess Engine and Graphical User Interface built with C++ and the Walnut framework.

![Gambit Engine Logo](GambitApp/assets/appLogo.png)

## Overview

Gambit Engine is a modern chess engine designed for both performance and ease of use. It features a custom-built move generator, an advanced search algorithm, and a polished graphical interface.

## Key Features

- **High-Performance Move Generation**: Efficiently generates all legal chess moves, including special moves like castling, en passant, and promotions.
- **Advanced Search Algorithm**:
    - **Negamax with Alpha-Beta Pruning**: Efficiently prunes the search tree to find the best moves.
    - **Iterative Deepening**: Allows the engine to return the best move found so far if time runs out.
    - **Quiescence Search**: Mitigates the "horizon effect" by searching tactical sequences to a quiet state.
    - **Transposition Tables**: Uses Zobrist hashing to store and reuse search results for seen positions.
- **Optimized Evaluation**:
    - **SIMD Acceleration**: Leverages modern CPU instructions for faster board evaluation.
    - **Piece-Square Tables**: Heuristics to encourage strategic piece placement.
- **Polished GUI**:
    - **Walnut & ImGui**: A sleek, responsive user interface powered by Vulkan and Dear ImGui.
    - **Real-time Evaluation**: Visualize the engine's thought process and evaluation scores.
- **UCI Compatibility**: Support for the Universal Chess Interface (UCI) protocol, allowing it to be used with other chess GUIs like Arena or Fritz.
- **Opening Book Support**: Supports standard Polyglot (.bin) opening books for varied and professional play.

## Getting Started

### Prerequisites

- **C++20 Compiler**: A modern compiler supporting C++20 (e.g., MSVC 2022, GCC 11+, or Clang 13+).
- **Vulkan SDK**: Required for the graphical interface.
- **CMake**: For build configuration.

### Building

1. Clone the repository:
   ```bash
   git clone --recursive https://github.com/Bennett-W/GambitEngine.git
   cd GambitEngine
   ```

2. Generate project files:
   ```bash
   cmake -B build
   ```

3. Build the project:
   ```bash
   cmake --build build --config Release
   ```

## Usage

### Graphical Mode
Run the `GambitApp` executable generated in the build directory to start the GUI.

### Headless (UCI) Mode
Run the `gambit_uci` executable for a standard UCI-compatible command-line interface.

## 3rd Party Libraries

- [Walnut](https://github.com/TheCherno/Walnut) - Application framework
- [Dear ImGui](https://github.com/ocornut/imgui) - Bloat-free Graphical User interface
- [GLFW](https://github.com/glfw/glfw) - Windowing and input
- [GLM](https://github.com/g-truc/glm) - OpenGL Mathematics
- [stb_image](https://github.com/nothings/stb) - Image loading

## License

This project is licensed under the MIT License - see the [LICENSE.txt](LICENSE.txt) file for details.
