## Gambit Chess Engine
- A chess engine inspired by the wonderful videos created by Sebatian Lague, the engine is written in C++.
- The current release includes a bare-bones support for the UCI (Universal Chess Interface).
- 
# Features
- Bitboard Move generation
- Pext compiler intrinsics for sliding piece lookups (a replacement for magic bitboards)
- Template metaprogramming to aim for semi-branchless code in the move generator, inspiration taken from the Gigantua move generator. So far, I can generate about 40M moves per second.
- Transposition Table implemented using the Lazy SMP design.
- Move ordering
- Iterative Deepening
- Simple GUI written using raylib
- Many more features to come, including an imgui based gui with vulkan as the backend (checkout the dev branch to see progress on that) <- WIP

# Building 
- Clone the repository
- Build using cmake, you must have a BMI instruction set compatible cpu for the pext instruction
- The Cmake file uses fetch content so there should be no need to install external libraries
- For now use the main branch as the dev branch is currently in a non functioning state

# Art Credit
Piece art credit to: By jurgenwesterhof (adapted from work of Cburnett) - http://commons.wikimedia.org/wiki/Template:SVG_chess_pieces, CC BY-SA 3.0, https://commons.wikimedia.org/w/index.php?curid=35634436
