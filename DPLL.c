#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "DPLL.h"

static void p(VariableAssignment *vas, int n){
    for(int i=1;i<=n;i++){
        printf("%d ",Lit_to_Int(vas[i].l));
    }
    putchar('\n');
}

void attach_clause(Recoder* rec, Clause *c){
    c->satisifid = 0;
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

static inline void detach_clause(Recoder *rec, Clause* c){
    c->deleted = 1;
}

static inline void set_satisify(Recoder *rec, Clause *c, Literal l){
    c->satisifid = 1;
    c->satisifid_step = rec->step - 1;
    c->satisifid_cause = l;
    rec->n ++ ;
}

static inline void unset_satisify(Recoder *rec, Clause *c){
    c->satisifid = 0;
    rec->n -- ;
}

static inline void remove_relat(Recoder *rec,ClauseArray *ca, Clause *c,int idx){
    rec->removed_relat++;
    int step = c->satisifid_step;

    if(step == rec->step - 1)
        return;

    RemovedClauseRelation* rcr = rec->removed_clause_relation[step];
    int rcr_top = rec->removed_clause_relation_tops[step]++;
    RemovedClauseRelation *r = &rcr[rcr_top];

    r->ca = ca;
    r->c = c;

    cluase_array_remove(ca,idx);
}

void recoder_clean(Recoder* rec){
    int n = rec->variable_cnt;
    ClauseArray** ca1 = rec->pos_relat_clause;
    ClauseArray** ca2 = rec->neg_relat_clause;
    for(int i=1;i<=n;i++)
        cluase_array_clean(ca1[i]);
    for(int i=1;i<=n;i++)
        cluase_array_clean(ca2[i]);
}

Literal choose_unbound_lit(Recoder *rec, Clause*c){
    for(int i=0;i<c->length;i++){
        Literal l = c->ls[i];
        Variable v = Lit_to_Var(l);
        if(rec->vas[v].l == NULL_LITERAL)
            return l;
    }
    return NULL_LITERAL;
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

    Clause *c;
    LoopClauseArray(neg_ca,c,idx){
        if(!is_clause_satisfied(c)){
            c->unbounded_lit_cnt--;
            if(c->unbounded_lit_cnt == 0)
                conf = 1;
            else if(c->unbounded_lit_cnt == 1){
                Literal l = choose_unbound_lit(rec,c);
                if(l == NULL_LITERAL){
                    clause_print(c);
                    assert(l != NULL_LITERAL);
                }
                record_untiary_clause(rec,c,l);
            }
        } else {
            remove_relat(rec,neg_ca,c,idx);
        }
    }
    
    LoopClauseArray(pos_ca,c,idx){
        if(!is_clause_satisfied(c)){
            set_satisify(rec,c,l);
        } else {
            remove_relat(rec,pos_ca,c,idx);
        }
    }

    return conf;
}

void unassign(Formula *fu, Recoder *rec, Literal l){
    Variable v = Lit_to_Var(l);
    int neg = Lit_Is_Neg(l);
    ClauseArray *neg_ca = neg ? rec->pos_relat_clause[v] : rec->neg_relat_clause[v];
    ClauseArray *pos_ca = neg ? rec->neg_relat_clause[v] : rec->pos_relat_clause[v];

    rec->vas[v].l = NULL_LITERAL;

    Clause *c;
    LoopClauseArray(neg_ca,c,idx){
        if(!is_clause_satisfied(c))
        //        p(rec->vas,rec->variable_cnt);
        //}
        //assert(!is_clause_satisfied(c));
            c->unbounded_lit_cnt++;
    }

    LoopClauseArray(pos_ca,c,idx){
        if(c->satisifid_cause == l)
            unset_satisify(rec,c);
    }
}

static inline int satisify(Formula *fu,Recoder *rec){
    return rec->n == fu->clause_cnt;
}

Recoder *new_recoder(Formula* fu){
    int variable_cnt = fu->variable_cnt;
    int clause_cnt = fu->clause_cnt;
    int n = variable_cnt * 2 + 2;

    Recoder *rec = calloc(sizeof(Recoder) + sizeof(VariableAssignment) * (n + 1),1);

    rec->variable_cnt = variable_cnt;

    rec->lit_activity = calloc(sizeof(double),n);
    rec->conflict_clauses = calloc(sizeof(Clause*), clause_cnt);

    rec->step_base = calloc(sizeof(Step),(variable_cnt + 2));

    rec->ls = malloc(sizeof(Literal) * (variable_cnt + 1));
    rec->ls_cnt = 0;

    rec->pos_relat_clause = malloc(sizeof(ClauseArray*) * (variable_cnt + 1));
    rec->neg_relat_clause = malloc(sizeof(ClauseArray*) * (variable_cnt + 1));
    for(Variable i=1;i<=variable_cnt;i++){
        rec->pos_relat_clause[i] = new_clause_array(clause_cnt);
        rec->neg_relat_clause[i] = new_clause_array(clause_cnt);
    }

    rec->removed_clause_relation = malloc(sizeof(RemovedClauseRelation*) * (variable_cnt + 1));
    rec->removed_clause_relation_tops = calloc(sizeof(int),variable_cnt + 1);
    for(int i=0;i<=variable_cnt;i++){
        rec->removed_clause_relation[i] = malloc(sizeof(RemovedClauseRelation) * (clause_cnt + 1));
    }

    for(int i=fu->first_cluase;i<fu->clause_end;i++){
        Clause *c = fu->cs[i];
        if(c){
            attach_clause(rec,c);
        }
    }

    rec->untiary_clauses = malloc(sizeof(UntiaryClause) * clause_cnt);
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
    int step = --rec->step;
    Step *st = &rec->step_base[step];
    RemovedClauseRelation* rcr = rec->removed_clause_relation[step];
    int rcr_top = rec->removed_clause_relation_tops[step];

    for(int i=rcr_top -1;i>=0;i--){
        RemovedClauseRelation *r = &rcr[i];
        cluase_array_push(r->ca,r->c);
    }
    rec->removed_relat -= rcr_top;
    rec->removed_clause_relation_tops[step] = 0;

    for(int i=rec->ls_cnt - 1;i >= st->ls_cnt;i--){
        unassign(fu,rec,rec->ls[i]);
    }
    rec->ls_cnt = st->ls_cnt;
    
    return st->neg ? NULL_LITERAL  : st->l;
}

VariableAssignment * form_result(Formula *fu,Recoder *rec){
    int n = fu->variable_cnt * fu->variable_cnt + 2;
    n = sizeof(VariableAssignment) * n;
    VariableAssignment* vas = malloc(n);
    memcpy(vas,rec->vas,n);
    return vas;
}

int propagate(Formula *fu, Recoder *rec, Literal l){
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

    recorde(fu,rec,1,1);

    int neg = 0;    
    Literal l = choose_lit(fu,rec);
    while(1){
        recorde(fu,rec,l,neg);

        l = neg ?  Lit_Neg(l) : l;

        int conf = propagate(fu,rec,l);
        int success = satisify(fu,rec);

        if(success){
            printf("%d\n",rec->clean_cnt);
            return form_result(fu,rec);
        }

        if(conf){
            while(rec->step && !(l = recover(fu,rec)))
                ;
            if(rec->step == 0){
                printf("%d\n",rec->clean_cnt);
                return NULL;
            }
            neg = 1;
        } else {
            neg = 0;
            l = choose_lit(fu,rec);
        }

        if(rec->removed_relat > 1000){
            rec->removed_relat = 0;
            rec->clean_cnt++;
            recoder_clean(rec);
        }
    }
}