#ifndef FORMULA_HEAD
#define FORMULA_HEAD
#include <stdio.h>

#include "clause.h"

typedef struct formula
{
    int type;
    int variable_cnt;
    int clause_cnt;
    ClauseRef first_cluase;
    ClauseRef clause_end;
    Clause *cs[0];
}Formula;

typedef struct assignment {
    Literal l;
    ClauseRef cr;
    int step; 
} VariableAssignment;

Formula* new_formula(int type,int ln,int cn);
Formula* new_formula_from_file(const char* filename);
void formula_free(Formula *fu);
Formula *formula_copy(Formula *fu);
void formula_dump(Formula* fu,FILE *f);

void formula_print(Formula *fu);

int formula_satisfy(Formula *fu, VariableAssignment* va);

#define LoopClausesInFormula(_fu,_i) \
    for(int _i=fu->first_cluase;i<fu->clause_end;i++)

#define formula_get_clause(_fu,_cr) (_fu->cs[(_cr)])

#endif
