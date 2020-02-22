#ifndef CLAUSE_HEAD
#define CLAUSE_HEAD

#include "literal.h"

typedef int ClauseRef;

typedef struct clause
{
    int deleted;
    int true_lit_cnt;
    int unbounded_lit_cnt;
    int length;
    Literal ls[0];
} Clause;

Clause* new_clause(int n);
void clause_free(Clause* c);
Clause *clause_copy(Clause *c);

int is_clause_satisfied(Clause* c);

void clause_print(Clause *c);
#define Loop_Literals_in_Clause(_c,_i) \
    for (int _i = 0; _i < _c->length; _i++)

typedef struct {
    int capacity,length;
    int first_idx,end_idx;
    int slots_cap,slots_cnt;
    int *slots;
    Clause* clauses[0];
} ClauseArray;

ClauseArray* new_clause_array(int cap);
void cluase_array_free(ClauseArray *ca);
void cluase_array_remove(ClauseArray *ca, int idx);
void cluase_array_push(ClauseArray *ca, Clause* c);
void cluase_array_clean(ClauseArray *lrc);
/*     for(int i=ca->first_idx;i<ca->end_idx;i++){
        if(ca->clauses[i]){
            printf("%d ", ca->clauses[i]);
        }
    } */
#endif
