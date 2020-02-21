#ifndef DPLL_INCLUDED
#define DPLL_INCLUDED

#include "formula.h"

Literal* DPLL(Formula *fu);

typedef struct del_clause{
    int id;
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
} step;

typedef struct unitary_clause {
    Clause *c;
    sint id;
    Literal l;
} unitary_clause;

typedef struct variable_assign {
    Literal l;
    sint impl_clause_id;
    sint step;
    Clause* impl_clause;
} variable_assign;

typedef struct recoder {
    del_clause **dcs_base;
    del_clause *dc_stack;
    sint dcs_top;

    del_literal **dls_base;
    del_literal *dl_stack;
    sint dls_top;

    step *step_base;
    sint step_top;

    unitary_clause *ucs_base;
    sint ucs_top;

    Literal* ls;
    sint ls_cnt;
    sint new_vas;
    variable_assign *vas;

    double *lit_activity;

    Clause **conflict_clause;
    sint conflict_clause_cnt;

    sint conflict_cnt;
    sint n;
} Recoder;

typedef struct info_buff
{
    sint clauses_length3_cnt;
    sint clauses_length2_cnt;
    Clause** clauses_length3;
    Clause** clauses_length2;

    sint cnt;
    sint *p;
    sint *h;

    Literal* conflict_induced_clause;
    sint * visited;
} Info_buff;


#endif