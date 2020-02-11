#ifndef FORMULA_HEAD
#define FORMULA_HEAD

#include "clause.h"

typedef struct formula
{
    sint type;
    sint literal_cnt;
    sint clause_cnt;
    Clause **cs;
    int first;
    int *next;
    int *removed;
}Formula;

Formula* new_formula(sint type,sint ln,sint cn);
Formula* new_formula_from_file(const char* filename);

void formula_free(Formula *fu);
void formula_print(Formula *fu);
sint formula_satisfy(Formula *fu, Literal* ls);
Formula *formula_copy(Formula *fu);

#define Loop_Literals_in_Clause(_l, _c, _i)                  \
    Literal _l = _c->ls[0];                                  \
    for (sint _i = 0; _i < _c->ori_length; _l = _c->ls[++_i]) \
        if (!IS_DELETED(_l))

#define Loop_Clauses_in_Formula(_c, _fu, _i, _pre, _next)      \
    Clause *_c=fu->cs[fu->first];                                   \
    for (int _i = fu->first,_pre = -1,_next=fu->_next[i]; _i >= 0; _pre = _i,_i = _next,_next=fu->next[i],_c=fu->cs[_i]) \


#endif
