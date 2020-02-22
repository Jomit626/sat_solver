#include <stdlib.h>
#include <stdio.h>

#include "clause.h"

Clause* new_clause(int n){
    Clause* c = malloc(sizeof(Clause) + n * sizeof(Literal));
    c->true_lit_cnt = 0;
    c->unbounded_lit_cnt = 0;
    c->deleted = 0;
    c->length = n;
    return c;
}

void clause_free(Clause* c){
    free(c);
}

void clause_print(Clause *c){
    printf("Clause:%p %d %d\n-Litreals:",c,c->true_lit_cnt,c->unbounded_lit_cnt);
    for(int i=0;i<c->length;i++){
        Literal l = c->ls[i];
        printf("%d ",Lit_to_Int(l));
    }
    putchar('\n');
}

Clause *clause_copy(Clause *c){
    Clause *nc = new_clause(c->length);
    for(int i=0;i<c->length;i++){
        nc->ls[i] = c->ls[i];
    }
    return nc;
}

ClauseArray* new_clause_array(int cap){
    ClauseArray *ca = malloc(sizeof(ClauseArray) + sizeof(Clause*) * cap);
    ca->first_idx = 0;
    ca->end_idx = 0;

    ca->capacity = cap;
    ca->length = 0;

    ca->slots_cap = cap;
    ca->slots_cnt = 0;
    ca->slots = malloc(sizeof(int) * cap);

    return ca;
}

void cluase_array_free(ClauseArray *ca){
    free(ca->slots);
    free(ca);
}

inline int is_clause_satisfied(Clause* c){
    return c->true_lit_cnt > 0;
}


static inline void cluase_add(ClauseArray *ca, Clause* c, int idx){
    ca->clauses[idx] = c;

    if(idx < ca->first_idx)
        ca->first_idx = idx;
    if(idx >= ca->end_idx)
        ca->end_idx = idx + 1;
}

inline void cluase_array_remove(ClauseArray *ca, int idx){
    ca->length--;
    ca->clauses[idx] = 0;
    ca->slots[ca->slots_cnt++] = idx;
    if(ca->slots_cnt >= ca->slots_cap){
        ca->slots_cap += 128;
        ca->slots = realloc(ca->slots,sizeof(int) * ca->slots_cap);
    }
}

void cluase_array_push(ClauseArray *ca, Clause* c){
    ca->length++;
    if(ca->slots_cnt){
        cluase_add(ca,c,ca->slots[--ca->slots_cnt]);
    } else {
        cluase_add(ca,c,ca->end_idx);
    }
}

void cluase_array_clean(ClauseArray *ca){
    ca->slots_cnt = 0;
    int i=0;
    for(int j=ca->first_idx;j<ca->end_idx;j++){
        if(ca->clauses[j]){
            ca->clauses[i++] = ca->clauses[j];
        }
    }
    ca->first_idx = 0;
    ca->end_idx = i;
}