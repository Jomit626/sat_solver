#ifndef DPLL_INCLUDED
#define DPLL_INCLUDED

#include "formula.h"

Literal* DPLL(Formula *fu);

typedef struct del_clause{
    sint id;
    Clause *c;
} del_clause;

typedef struct del_literal{
    Literal *l;
    Clause *c;
} del_literal;

typedef struct step {
    Literal l;
    sint dcs_top;
    sint dls_top;
    sint ls_cnt;
    sint tag;
} Step;

typedef struct unitary_clause {
    Clause *c;
    sint id;
    Literal l;
} unitary_clause;

typedef struct step_stack {
    del_clause *dcs_base;
    sint dcs_top;

    del_literal *dls_base;
    sint dls_top;

    Step *step_base;
    sint step_top;

    unitary_clause *ucs_base;
    sint ucs_top;

    Literal *ls;
    sint ls_cnt;
} Step_stack;

#endif