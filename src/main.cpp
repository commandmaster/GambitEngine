#include "raylib.h"
#include <vector>
#include <iostream>
#include <string>

#include "Renderer.h"
#include "Precomputation.h"
#include "MoveGenerator.h"
#include "Perft.h"
#include "Game.h"
#include "UCI.h"


int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--uci") {
        UCI::loop();
        return 0;
    }

    const int screenWidth = 1280;
    const int screenHeight = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Chess Engine V4");
    
    Game game(screenWidth, screenHeight);
    while (game.update()) {}
    
    return EXIT_SUCCESS;
}