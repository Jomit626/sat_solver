#ifndef BINARY_PUZZLE_INCLUDED
#define BINARY_PUZZLE_INCLUDED

#include "formula.h"

typedef struct puzzle{
    int size;
    Formula* fu;
    int map[0];
} Puzzle;
void puzzle_print_map(Puzzle* puz);
Puzzle * new_puzzle_from_file(const char* filename);

#endif