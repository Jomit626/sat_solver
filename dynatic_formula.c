#include <stdlib.h>
#include <stdio.h>

#include "dynatic_formula.h"

Dformula* new_dformula(sint type,sint ln, sint cap){
    Dformula *dfu = malloc(sizeof(Dformula) + sizeof(Clause*) * cap);

    dfu->type = type;
    dfu->literal_cnt = ln;
    dfu->capacity = cap;
    dfu->clause_cnt = 0;
    dfu->slots = malloc(sizeof(sint) * cap);
    dfu->slot_top = 0;
    return dfu;
}

inline void dformula_remove_clause(Dformula *dfu,int id){
    free(dfu->cs[id]);
    dfu->cs[id] = NULL;
    dfu->slots[dfu->slot_top++] = id;
}

inline Dformula* dformula_add_clause(Dformula *dfu,Clause *c){
    if(dfu->slot_top){
        int id = dfu->slots[--dfu->slot_top];
        dfu->cs[id] = c;
        return dfu;
    }
    dfu->cs[dfu->clause_cnt++] = c;
    if(dfu->clause_cnt >= dfu->capacity){
        dfu->capacity += 2048;
        dfu = realloc(dfu,sizeof(Dformula) + sizeof(Clause*) * dfu->capacity);
        dfu->slots = realloc(dfu->slots,sizeof(sint) * dfu->capacity);
    }
    return dfu;
}

#define NSTAISIFY 0
#define STAISIFY 1
#define UNKOWN 2

int dfumula_check(Dformula *dfu,variable_assign *vas,Clause **conflict_clause){
    for(sint i=0;i<dfu->clause_cnt;i++){
        Clause* c = dfu->cs[i];
        if(!c)
            continue;
        int satisify = NSTAISIFY;
        for(sint j=0;j<c->ori_length;j++){
            Literal l = LIT_REMOVE_DEL_BIT(c->ls[j]);
            Literal nl = LIT_NEG(l);
            Variable v = LIT_to_VAR(l);

            variable_assign *va = &vas[v];
            if(va->l == l){
                satisify = STAISIFY;
                break;
            }
            else if(va->l == nl){
                continue;
            } else if(va->l == NULL_LITERAL){
                satisify = UNKOWN;
                if(c->length >= 4) 
                    dformula_remove_clause(dfu,i);
                break;
            }
        }
        if(satisify == NSTAISIFY){
            int min_step = dfu->literal_cnt + 1;
            for(sint j=0;j<c->ori_length;j++){
                Literal l = LIT_REMOVE_DEL_BIT(c->ls[j]);
                Variable v = LIT_to_VAR(l);

                variable_assign *va = &vas[v];
                min_step = min_step > va->step ? va->step : min_step;
            }
            *conflict_clause = (Clause*)c;
            return 1;
        }
    }
    return 0;
}

void dfumula_simplfy(Dformula *dfu,variable_assign *vas){
    // TODO:
}


void dformula_print(Dformula *dfu)
{
    printf("Formula:%p Type:%d Literal:%d Clause:%d cap:%d\n", dfu, dfu->type, dfu->literal_cnt, dfu->clause_cnt,dfu->capacity);
    for(int i=0;i<dfu->clause_cnt;i++){
        printf("%d-",i);
        clause_print(dfu->cs[i]);
    }
}