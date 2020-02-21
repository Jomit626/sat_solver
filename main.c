#include <stdio.h>

#include "DPLL.h"
#include "binary_puzzle.h"

int main(int argc, char** argv){
    const char* map_filename;
    if(argc > 1)
        map_filename = argv[1];
    else
        map_filename = "tests/bp14.map";
    
    Puzzle *puz =  new_puzzle_from_file(map_filename);
    puts("Puzzle:");
    puzzle_print_map(puz);
    FILE *f = fopen("dd","w");
    formula_dump(puz->fu,f);
    Literal *res = DPLL(puz->fu);
    for(int i=0;i<=puz->size * puz->size;i++){
        puz->map[i] = IS_NEG(res[i]) ? 0 : 1;
    }
    puts("Solved Puzzle:");
    puzzle_print_map(puz);

    return 0;
}