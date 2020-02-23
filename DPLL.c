#include <stdlib.h>
#include <assert.h>

#include "DPLL.h"

void attach_clause(Recoder* rec, Clause *c){
    c->true_lit_cnt = 0;
    c->unbounded_lit_cnt = c->length;
    c->deleted = 0;
    for(int i=0;i<c->length;i++){
        Literal l = c->ls[i];
        Variable v = Lit_to_Var(l);
        if(Lit_Is_Neg(l)){
            cluase_array_push(rec->neg_relat_clause[v],c);
        }else {
            cluase_array_push(rec->pos_relat_clause[v],c);
        }
    }
}

inline void detach_clause(Recoder *rec, Clause* c){
    c->deleted = 1;
}

void recoder_clean(Recoder* rec){
    int n = rec->variable_cnt + 1;
    ClauseArray** ca1 = rec->pos_relat_clause;
    ClauseArray** ca2 = rec->neg_relat_clause;
    for(int i=0;i<n;i++)
        cluase_array_clean(ca1[i]);
    for(int i=0;i<n;i++)
        cluase_array_clean(ca2[i]);
}

Literal choose_unbound_lit(Recoder *rec, Clause*c){
    for(int i=0;i<c->length;i++){
        Literal l = c->ls[i];
        Variable v = Lit_to_Var(l);
        if(rec->vas[v].l == NULL_LITERAL)
            return l;
    }
}

static inline void record_untiary_clause(Recoder *rec, Clause *c, Literal l){
    UntiaryClause *uc = &rec->untiary_clauses[rec->untiary_clauses_cnt++];
    uc->c = c;
    uc->l = l;
}

int assign(Formula *fu, Recoder *rec, Literal l){
    Variable v = Lit_to_Var(l);
    int neg = Lit_Is_Neg(l);
    int conf = 0;
    ClauseArray *neg_ca = neg ? rec->pos_relat_clause[v] : rec->neg_relat_clause[v];
    ClauseArray *pos_ca = neg ? rec->neg_relat_clause[v] : rec->pos_relat_clause[v];

    rec->vas[v].l = l;
    rec->ls[rec->ls_cnt ++] = l;

    for(int i=neg_ca->first_idx;i<neg_ca->end_idx;i++){
        Clause *c = neg_ca->clauses[i];
        if(!c) continue;
        c->unbounded_lit_cnt--;

        if(c->unbounded_lit_cnt == 0 && c->true_lit_cnt == 0)
            conf = 1;
        if(c->unbounded_lit_cnt == 1 && c->true_lit_cnt == 0){
            Literal l = choose_unbound_lit(rec,c);
            record_untiary_clause(rec,c,l);
        }
    }

    for(int i=pos_ca->first_idx;i<pos_ca->end_idx;i++){
        Clause *c = pos_ca->clauses[i];
        if(!c) continue;
        if(c->true_lit_cnt == 0)
            rec->n++;
        c->true_lit_cnt ++;
    }
    return conf;
}

void unassign(Formula *fu, Recoder *rec, Literal l){
    Variable v = Lit_to_Var(l);
    int neg = Lit_Is_Neg(l);
    ClauseArray *neg_ca = neg ? rec->pos_relat_clause[v] : rec->neg_relat_clause[v];
    ClauseArray *pos_ca = neg ? rec->neg_relat_clause[v] : rec->pos_relat_clause[v];

    rec->vas[v].l = NULL_LITERAL;

    for(int i=neg_ca->first_idx;i<neg_ca->end_idx;i++){
        Clause *c = neg_ca->clauses[i];
        if(!c) continue;

        c->unbounded_lit_cnt++;
    }

    for(int i=pos_ca->first_idx;i<pos_ca->end_idx;i++){
        Clause *c = pos_ca->clauses[i];
        if(!c) continue;
        if(c->true_lit_cnt == 1)
            rec->n--;
        c->true_lit_cnt --;
    }
}

static inline int satisify(Formula *fu,Recoder *rec){
    return rec->n == fu->clause_cnt;
}

Recoder *new_recoder(Formula* fu){
    int variable_cnt = fu->variable_cnt;
    int clause_cnt = fu->clause_cnt;
    int n = variable_cnt * 2 + 2;

    Recoder *rec = calloc(sizeof(Recoder),1);

    rec->variable_cnt = variable_cnt;

    rec->lit_activity = calloc(sizeof(double),n);
    rec->conflict_clauses = calloc(sizeof(Clause*), clause_cnt);

    rec->step_base = calloc(sizeof(Step),(variable_cnt + 2));

    rec->vas = calloc(sizeof(VariableAssignment),n + 1);
    rec->ls = malloc(sizeof(Literal) * (variable_cnt + 1));
    rec->ls_cnt = 0;
    rec->pos_relat_clause = malloc(sizeof(ClauseArray*) * (variable_cnt + 1));
    rec->neg_relat_clause = malloc(sizeof(ClauseArray*) * (variable_cnt + 1));
    for(Variable i=1;i<=variable_cnt;i++){
        rec->pos_relat_clause[i] = new_clause_array(clause_cnt);
        rec->neg_relat_clause[i] = new_clause_array(clause_cnt);
    }

    for(int i=fu->first_cluase;i<fu->clause_end;i++){
        Clause *c = fu->cs[i];
        if(c){
            attach_clause(rec,c);
        }
    }

    rec->untiary_clauses = malloc(sizeof(UntiaryClause) * clause_cnt);
    rec->untiary_clauses_cnt = 0;
    return rec;
}

Literal choose_lit(Formula *fu, Recoder *rec){
    for(int i=1;i<=fu->variable_cnt;i++){
        if(rec->vas[i].l == NULL_LITERAL){
            Literal l = Var_to_Lit(i,0);
            return l;
        }
    }
    return NULL_LITERAL;
}

static inline void recorde(Formula *fu,Recoder *rec, Literal l, int neg){
    Step *st = &rec->step_base[rec->step++];
    st->l = l;
    st->neg = neg;
    st->ls_cnt = rec->ls_cnt;

    rec->untiary_clauses_cnt = 0;
}

static inline Literal recover(Formula *fu,Recoder *rec){
    Step *st = &rec->step_base[--rec->step];

    for(int i=rec->ls_cnt - 1;i >= st->ls_cnt;i--){
        unassign(fu,rec,rec->ls[i]);
    }
    rec->ls_cnt = st->ls_cnt;
    
    return st->neg ? NULL_LITERAL  : st->l;
}

Literal * form_result(Formula *fu,Recoder *rec){
    Literal* res = malloc(sizeof(Literal) * (fu->variable_cnt + 1));

    for(int i=0;i<=fu->variable_cnt;i++){
        res[i] = rec->vas[i].l;
    }

    return res;
}

int protagnit(Formula *fu, Recoder *rec, Literal l){
    int conf = assign(fu,rec,l);
    while (!conf && rec->untiary_clauses_cnt)
    {
        UntiaryClause *uc = &rec->untiary_clauses[--rec->untiary_clauses_cnt];
        Literal l = uc->l;
        Variable v = Lit_to_Var(l);
        if(rec->vas[v].l == NULL_LITERAL){
            conf = assign(fu,rec,l);
        }
    }
    return conf;
}

VariableAssignment *DPLL(Formula *fu){
    Recoder* rec = new_recoder(fu);

    recorde(fu,rec,0,1);

    int neg = 0;    
    Literal l = choose_lit(fu,rec);
    while(1){
        recorde(fu,rec,l,neg);

        l = neg ?  Lit_Neg(l) : l;

        int conf = protagnit(fu,rec,l);
        int success = satisify(fu,rec);

        if(success){
            return rec->vas;
        }

        if(conf){
            while(rec->step && !(l = recover(fu,rec)))
                ;
            if(rec->step == 0)
                return NULL;
            neg = 1;
        } else {
            neg = 0;
            l = choose_lit(fu,rec);
        }
    }
}