#ifndef DPLL_INCLUDED
#define DPLL_INCLUDED

#include "formula.h"

VariableAssignment * DPLL(Formula *fu);

typedef struct step {
    Literal l;
    int neg;
    int ls_cnt;
    int satisifid_clauses_top;
} Step;

typedef struct {
    ClauseArray *ca;
    Clause *c;
} RemovedClauseRelation;

typedef struct {
    Clause *c;
    Literal l;
} UntiaryClause;

typedef struct recoder {
    //Statistics
    int conflict_cnt;
    int variable_cnt;
    int clean_cnt;
    double *lit_activity;

    int n;

    Clause **conflict_clauses;
    int conflict_clause_cnt;

    RemovedClauseRelation **removed_clause_relation;
    int *removed_clause_relation_tops;
    int removed_relat;

    int step;
    Step *step_base;
    Literal *ls;
    int ls_cnt;
    ClauseArray** pos_relat_clause;
    ClauseArray** neg_relat_clause;

    UntiaryClause *untiary_clauses;
    int untiary_clauses_cnt;

    VariableAssignment vas[0];
} Recoder;

#endif