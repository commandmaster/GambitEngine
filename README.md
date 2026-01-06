# Gambit Engine

<p align="center">
  <img src="GambitApp/assets/appLogo.png" alt="Gambit Engine Logo" width="200"/>
</p>

<p align="center">
  Chess engine and GUI written in C++20
</p>

<p align="center">
  <a href="#features">Features</a> |
  <a href="#getting-started">Getting Started</a> |
  <a href="#usage">Usage</a> |
  <a href="#architecture">Architecture</a> |
  <a href="#license">License</a>
</p>

---

## Features

### Search
- Negamax with alpha-beta pruning
- Iterative deepening
- Quiescence search
- Transposition table with Zobrist hashing
- Move ordering (MVV-LVA, hash moves)

### Evaluation
- SIMD evaluation using SSE intrinsics
- Piece-square tables
- Tapered evaluation (middlegame to endgame)

### Move Generation
- Bitboard representation
- Legal move generation with pin/check detection
- Castling, en passant, promotion

### GUI
- Walnut/ImGui interface (Vulkan)
- Drag and drop pieces
- Move highlighting
- Move animation
- Game history navigation
- AI time limit configuration

### Other
- UCI protocol support
- Polyglot opening book support
- Async search (non-blocking UI)

## Getting Started

### Prerequisites

- Windows 10/11
- Visual Studio 2022 with C++20
- Vulkan SDK

### Building

1. Clone the repository:
   ```bash
   git clone --recursive https://github.com/commandmaster/GambitEngine.git
   cd GambitEngine
   ```

2. Generate Visual Studio solution:
   ```bash
   vendor\premake\premake5.exe vs2022
   ```

3. Open `GambitEngine.sln` in Visual Studio and build.

### Dependencies (included as submodules)

- [Walnut](https://github.com/TheCherno/Walnut)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [GLFW](https://github.com/glfw/glfw)
- [GLM](https://github.com/g-truc/glm)
- [stb_image](https://github.com/nothings/stb)

## Usage

### GUI

Run `GambitApp.exe` from the build output:

```
bin\Release-windows-x86_64\GambitApp\GambitApp.exe
```

- Click or drag pieces to move
- Use playback controls to navigate history
- Configure AI in the Board Settings panel

### UCI

The engine supports UCI for use with external GUIs (Arena, Cute Chess, etc.):

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
│       ├── Board.h/cpp           # Board representation
│       ├── MoveGenerator.h/cpp   # Move generation
│       ├── Search.h              # Search (negamax, quiescence)
│       ├── Evaluation.h          # Evaluation
│       ├── TranspositionTable.h  # TT
│       ├── Opening.h/cpp         # Opening book
│       ├── UCI.h/cpp             # UCI protocol
│       ├── Game.h                # Game logic
│       └── Renderer.h/cpp        # Rendering
├── Walnut/                       # GUI framework
└── vendor/                       # Dependencies
```

## License

MIT License - see [LICENSE.txt](LICENSE.txt)
