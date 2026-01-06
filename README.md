# Gambit Engine

<p align="center">
  <img src="GambitApp/assets/appLogo.png" alt="Gambit Engine Logo" width="200"/>
</p>

<p align="center">
  <strong>A high-performance chess engine with a modern graphical interface</strong>
</p>

<p align="center">
  <a href="#features">Features</a> |
  <a href="#getting-started">Getting Started</a> |
  <a href="#usage">Usage</a> |
  <a href="#architecture">Architecture</a> |
  <a href="#license">License</a>
</p>

---

## Overview

Gambit Engine is a modern chess engine written in C++20, featuring both a polished graphical user interface and a UCI-compatible command-line interface. Built for performance and extensibility, it combines classical chess programming techniques with modern optimizations.

## Features

### Search Algorithm
- **Negamax with Alpha-Beta Pruning** - Efficient minimax search with alpha-beta cutoffs
- **Iterative Deepening** - Progressive depth search with time management
- **Quiescence Search** - Extended search on tactical positions to avoid horizon effects
- **Transposition Tables** - Zobrist hashing for position caching and move ordering
- **Move Ordering** - MVV-LVA and hash move prioritization for better pruning

### Evaluation
- **SIMD-Accelerated Evaluation** - Leverages SSE/AVX instructions for fast board evaluation
- **Piece-Square Tables** - Position-aware piece value bonuses
- **Tapered Evaluation** - Smooth transition between middlegame and endgame piece values
- **Material Counting** - Efficient bitboard-based material calculation

### Move Generation
- **Bitboard Representation** - 64-bit board representation for fast move generation
- **Legal Move Generation** - Direct legal move generation with pin and check detection
- **Special Moves** - Full support for castling, en passant, and pawn promotion

### User Interface
- **Walnut/ImGui GUI** - Modern, responsive interface powered by Vulkan and Dear ImGui
- **Drag & Drop Pieces** - Intuitive piece movement with visual feedback
- **Move Highlighting** - Legal move visualization for selected pieces
- **Move Animation** - Smooth piece movement animations
- **Game History** - Navigate through game history with forward/back controls
- **AI Configuration** - Adjustable AI time limits and color selection

### Integration
- **UCI Protocol** - Full Universal Chess Interface support for external GUI compatibility
- **Opening Book Support** - Polyglot (.bin) opening book integration
- **Async Search** - Non-blocking AI search keeps the UI responsive

## Getting Started

### Prerequisites

- **Windows 10/11** - Currently Windows-only
- **Visual Studio 2022** - With C++20 support
- **Vulkan SDK** - Required for the graphical interface

### Building

1. **Clone the repository:**
   ```bash
   git clone --recursive https://github.com/commandmaster/GambitEngine.git
   cd GambitEngine
   ```

2. **Generate Visual Studio solution:**
   ```bash
   vendor\premake\premake5.exe vs2022
   ```

3. **Build the project:**
   
   Open `GambitEngine.sln` in Visual Studio and build (Release recommended), or:
   ```bash
   msbuild GambitEngine.sln /p:Configuration=Release
   ```

### Dependencies (included as submodules)

- [Walnut](https://github.com/TheCherno/Walnut) - Application framework
- [Dear ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI
- [GLFW](https://github.com/glfw/glfw) - Window management and input
- [GLM](https://github.com/g-truc/glm) - Mathematics library
- [stb_image](https://github.com/nothings/stb) - Image loading

## Usage

### GUI Mode

Run the `GambitApp` executable to launch the graphical interface:

```bash
bin\Release-windows-x86_64\GambitApp\GambitApp.exe
```

**Controls:**
- Click or drag pieces to move them
- Use the playback controls to navigate game history
- Configure AI players in the Board Settings panel

### UCI Protocol

The engine supports the Universal Chess Interface (UCI) protocol, making it compatible with external chess GUIs like Arena or Cute Chess.

**Example UCI commands:**
```
uci
isready
position startpos moves e2e4 e7e5
go movetime 1000
quit
```

## Architecture

```
GambitEngine/
├── GambitApp/
│   └── src/
│       ├── Board.h/cpp        # Board representation and move execution
│       ├── MoveGenerator.h/cpp # Legal move generation
│       ├── Search.h           # Search algorithm (negamax, quiescence)
│       ├── Evaluation.h       # Position evaluation with SIMD
│       ├── TranspositionTable.h # Zobrist hashing and TT
│       ├── Opening.h/cpp      # Polyglot opening book
│       ├── UCI.h/cpp          # UCI protocol implementation
│       ├── Game.h             # GUI game logic
│       └── Renderer.h/cpp     # Board rendering
├── Walnut/                    # GUI framework
└── vendor/                    # Third-party dependencies
```

## Performance

- Bitboard-based move generation for efficient legal move enumeration
- SIMD-optimized evaluation using SSE intrinsics
- Transposition table with Zobrist hashing for position caching
- Asynchronous search thread for responsive UI

## License

This project is licensed under the MIT License - see the [LICENSE.txt](LICENSE.txt) file for details.

---

<p align="center">
  Made with C++ and a passion for chess
</p>
