#ifndef FORMULA_HEAD
#define FORMULA_HEAD

#include "clause.h"

typedef struct formula
{
    sint type;
    sint literal_cnt;
    sint clause_cnt;
    Clause **cs;
    sint first_cluase;
    sint clause_end;
}Formula;

Formula* new_formula(sint type,sint ln,sint cn);
Formula* new_formula_from_file(const char* filename);

void formula_free(Formula *fu);
void formula_print(Formula *fu);
sint formula_satisfy(Formula *fu, Literal* ls);
Formula *formula_copy(Formula *fu);

#define Loop_Literals_in_Clause(_l, _c, _i)                  \
    Literal _l;                                  \
    for (sint _i = 0; _i < _c->ori_length; ++_i) \
        if (((_l = _c->ls[_i]),!IS_DELETED(_l)))

#define Loop_Clauses_in_Formula(_c, _fu, _i)      \
    Clause *_c;                                   \
    for (int _i = fu->first_cluase; _i<fu->clause_end; _i++) \
    if((_c = fu->cs[_i]))


#endif
