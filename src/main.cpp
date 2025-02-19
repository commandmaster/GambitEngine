#include "raylib.h"
#include <vector>
#include <iostream>
#include <string>

#include "Renderer.h"
#include "Precomputation.h"
#include "MoveGenerator.h"
#include "Perft.h"
#include "Game.h"


int main()
{
	const int screenWidth = 2000;
	const int screenHeight = 1200;

	SetConfigFlags(FLAG_MSAA_4X_HINT);
	InitWindow(screenWidth, screenHeight, "Chess Engine V4");

	Game game;

	while (game.update())
	{

	}

	return EXIT_SUCCESS;
}