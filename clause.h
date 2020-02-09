#ifndef CLAUSE_HEAD
#define CLAUSE_HEAD

#include "literal.h"

typedef struct clause
{
    sint ori_length;
    sint length;
    Literal ls[0];
} Clause;

Clause* new_clause(sint n);
void clause_free(Clause* c);

void clause_remove(Clause* c, Literal l);
void clause_print(Clause *c);
Clause *clause_copy(Clause *c);
#endif
