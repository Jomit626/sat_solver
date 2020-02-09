#ifndef FORMULA_HEAD
#define FORMULA_HEAD

#include "clause.h"

typedef struct formula
{
    sint type;
    sint literal_cnt;
    sint clause_cnt;
    Clause **cs;
}Formula;

Formula* new_formula(sint type,sint ln,sint cn);
Formula* new_formula_from_file(char* filename);

void formula_free(Formula *fu);
void formula_print(Formula *fu);
sint formula_satisfy(Formula *fu, Literal* ls);
Formula *formula_copy(Formula *fu);



#endif
