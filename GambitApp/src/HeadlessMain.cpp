#include "UCI.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // Optional: Check for arguments (e.g. bench, perft)
    // For now, just start UCI loop
    UCI::loop();
    return 0;
}
