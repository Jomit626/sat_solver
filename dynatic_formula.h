#ifndef DYNATIC_FORMULA_INCLUDED
#define DYNATIC_FORMULA_INCLUDED

#include "DPLL.h"

typedef struct Dformula {
    sint type;
    sint literal_cnt;
    sint clause_cnt;
    sint capacity;

    sint* slots;
    sint slot_top;
    Clause *cs[0];
} Dformula;

Dformula* new_dformula(sint type,sint ln, sint cap);
Dformula* dformula_add_clause(Dformula *dfu,Clause *c);
int dfumula_check(Dformula *dfu,variable_assign *vas,Clause **conflict_clause);
void dformula_print(Dformula *dfu);
void dformula_remove_clause(Dformula *dfu,int id);
#endif