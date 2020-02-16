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

    Literal *ls;
    sint ls_cnt;
} Recoder;

typedef struct literal_info
{
    sint related_clause_cnt;
    sint cap;
    sint related_clauses[];
} Lit_info;

typedef struct info_buff
{
    sint clauses_length3_cnt;
    sint clauses_length2_cnt;
    Clause** clauses_length3;
    Clause** clauses_length2;
    sint cnt;
    sint *p;
    sint *h;
} Info_buff;


#endif