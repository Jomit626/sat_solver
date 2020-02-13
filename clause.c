#include <stdlib.h>
#include <stdio.h>

#include "clause.h"

Clause* new_clause(sint n){
    Clause* c = malloc(sizeof(Clause) + n * sizeof(Literal));
    c->ori_length = n;
    c->length = n;
    return c;
}

void clause_free(Clause* c){
    free(c);
}

// remove a literal in a clause
void clause_remove(Clause* c, Literal l){
    l = LIT_REMOVE_DEL_BIT(l);

    for(sint i=0;i<c->ori_length;i++){
        if(c->ls[i] == l){
            c->ls[i] |= DEL_MASK;
             c->length--;
            break;
        }
    }
}

void clause_print(Clause *c){
    printf("Clause:%p\n-Litreals:",c);
    for(sint i=0;i<c->ori_length;i++){
        Literal l = c->ls[i];
        printf("%d%c ",LIT_TO_INT(l),IS_DELETED(l)? 'X' : ' ');
    }
    putchar('\n');
}

Clause *clause_copy(Clause *c){
    Clause *nc = new_clause(c->ori_length);
    for(sint i=0;i<c->ori_length;i++){
        nc->ls[i] = c->ls[i];
    }
    return nc;
}
