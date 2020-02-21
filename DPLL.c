#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "dynatic_formula.h"
#include "DPLL.h"

#define NORMAL_TAG 0
#define NEG_TAG 1

Recoder *new_recoder(Formula *fu)
{
    const int max_step = fu->literal_cnt + 1;
    
    Recoder *rec = malloc(sizeof(Recoder));

    rec->dcs_base = malloc(sizeof(del_clause*) * max_step);

    rec->dls_base = malloc(sizeof(del_literal*) * max_step);

    for(int i=0;i<max_step;i++){
        rec->dcs_base[i] = malloc(sizeof(del_clause) * (fu->clause_cnt + 1));
        rec->dls_base[i] = malloc(sizeof(del_literal) * (fu->literal_cnt * fu->clause_cnt));
    }

    rec->dc_stack = rec->dcs_base[0];
    rec->dcs_top = 0;
    rec->dl_stack = rec->dls_base[0];
    rec->dls_top = 0;

    rec->step_base = malloc(sizeof(step) * (fu->literal_cnt + 1));
    rec->step_top = 0;

    rec->ucs_base = malloc(sizeof(unitary_clause) * (fu->clause_cnt + 1));
    rec->ucs_top = 0;

    rec->ls = malloc(sizeof(Literal)* (fu->literal_cnt + 1));
    rec->ls_cnt = 0;
    
    rec->vas = malloc(sizeof(variable_assign) * (fu->literal_cnt + 1));
    for(int i=0;i<fu->literal_cnt+ 1;i++){
        rec->vas[i].l = NULL_LITERAL;
    }
    rec->lit_activity = calloc(sizeof(double),(fu->literal_cnt * 2 + 2));

    rec->conflict_clause = malloc(sizeof(Clause *) * fu->clause_cnt);
    rec->conflict_cnt = 0;
    rec->n = 0;
    return rec;
}

void recoder_free(Recoder *rec){
    // TODO:
    //while(rec->dcs_top--){
    //}
    free(rec->dcs_base);
    free(rec->dls_base);
    free(rec->step_base);
    free(rec->ucs_base);
    free(rec->ls);
    free(rec);
}

static inline void formula_remove_clause_recoded(Formula *fu, Recoder *rec, int id, Clause *c)
{
    del_clause *dc = &rec->dc_stack[rec->dcs_top++];
    dc->id = id;
    dc->c = c;

    formula_remove_clause(fu,id);
}
static inline void clause_remove_literal(Formula *fu, Recoder *rec, Clause *c, Literal l, sint literal_id)
{
    del_literal *dl = &rec->dl_stack[rec->dls_top++];
    dl->c = c;
    dl->l = &c->ls[literal_id];

    c->ls[literal_id] = l | DEL_MASK;
    c->length--;
}

static inline void recode_unitary_clause(Formula *fu, Recoder *rec, Clause *c, sint clause_id)
{
    unitary_clause *uc = &rec->ucs_base[rec->ucs_top++];
    uc->id = clause_id;
    uc->c = c;

    for (sint i = 0; i < c->ori_length; i++)
        if (!IS_DELETED(c->ls[i]))
        {
            uc->l = c->ls[i];
            break;
        }
}

static inline void env_result_add(Recoder *rec, Literal l, Clause* impl_clause, sint id)
{
    variable_assign *va = &rec->vas[LIT_to_VAR(l)];
    if(va->l == NULL_LITERAL){
        rec->ls[rec->ls_cnt++] = l;

        va->l = l;
        va->impl_clause = impl_clause;
        va->impl_clause_id = id;
        va->step = rec->step_top;
    }

}
static inline void recoder_push(Formula *fu, Recoder *rec, Literal l, sint tag)
{
    step *st = &rec->step_base[rec->step_top++];
    st->l = l;
    st->dcs_top = rec->dcs_top;
    st->dls_top = rec->dls_top;
    st->ls_cnt = rec->ls_cnt;
    st->tag = tag;

    rec->ucs_top = 0;

    rec->dcs_top = 0;
    rec->dc_stack = rec->dcs_base[rec->step_top];
    rec->dls_top = 0;
    rec->dl_stack = rec->dls_base[rec->step_top];

    rec->conflict_clause_cnt = 0;
}
static inline void formula_recover_deleted_clause(Formula *fu, Recoder *rec, del_clause *dc)
{
    int id = dc->id;
    Clause *c = dc->c;

    fu->cs[id] = c;
    if(id < fu->first_cluase){
        fu->first_cluase = id;
    }
    if(id >= fu->clause_end){
        fu->clause_end = id + 1;
    }
}
static inline void formula_recover_deleted_literal(Formula *fu, Recoder *rec, del_literal *dl)
{
    *dl->l = LIT_REMOVE_DEL_BIT(*dl->l);
    dl->c->length++;
}
static inline Literal recoder_pop(Formula *fu, Recoder *rec)
{
    step *st = &rec->step_base[--rec->step_top];

    for (sint top = rec->dcs_top - 1; top >= 0 ; top--)
    {
        del_clause *dc = &rec->dc_stack[top];
        formula_recover_deleted_clause(fu, rec, dc);
    }

    for (sint top = rec->dls_top - 1; top >= 0; top--)
    {
        del_literal *dl = &rec->dl_stack[top];
        formula_recover_deleted_literal(fu, rec, dl);
    }

    rec->ucs_top = 0;

    for(sint top = rec->ls_cnt - 1; top >= st->ls_cnt; top--){
        rec->vas[LIT_to_VAR(rec->ls[top])].l = NULL_LITERAL;
    }
    rec->ls_cnt = st->ls_cnt;

    rec->dcs_top = st->dcs_top;
    rec->dls_top = st->dls_top;
    sint step = rec->step_top;
    rec->dc_stack = rec->dcs_base[step];
    rec->dl_stack = rec->dls_base[step];

    return (st->tag == NEG_TAG) ? NULL_LITERAL : st->l;
}

Info_buff *new_info_buff(Formula *fu)
{
    Info_buff *buff = malloc(sizeof(Info_buff));

    buff->clauses_length2_cnt = 0;
    buff->clauses_length2 = malloc(sizeof(Clause*) * fu->clause_cnt);
    buff->clauses_length3_cnt = 0;
    buff->clauses_length3 = malloc(sizeof(Clause*) * fu->clause_cnt);

    buff->cnt = sizeof(sint) * ((fu->literal_cnt + 1) * 2 + 1);
    buff->p = malloc(buff->cnt);
    buff->h = malloc(buff->cnt);

    buff->conflict_induced_clause = malloc(sizeof(Literal) * (fu->literal_cnt + 1));

    buff->visited = malloc(sizeof(sint) * (fu->clause_cnt + 1));
    return buff;
}

static inline void info_buff_free(Info_buff *buff){
    free(buff->clauses_length2);
    free(buff->clauses_length3);
    free(buff->p);
    free(buff->h);

    free(buff);
}

static inline void info_buff_reset(Info_buff *buff){
    buff->clauses_length2_cnt = 0;
    buff->clauses_length3_cnt = 0;
}
Literal formula_select_key_literal(Formula *fu, Info_buff *buff)
{
    sint *p = buff->p;
    sint *h = buff->h;
    memset(p,0x0,buff->cnt);
    memset(h,0x0,buff->cnt);

    Clause **clause3 = buff->clauses_length3;
    Clause **clause2 = buff->clauses_length2;

    for(sint i=0;i<buff->clauses_length3_cnt;i++)
    {
        Clause *c = clause3[i];
        Loop_Literals_in_Clause(l, c, j)
            p[LIT_ID(l)] += 1;
    }
    for(sint i=0;i<buff->clauses_length2_cnt;i++)
    {
        Clause *c = clause2[i];
        Loop_Literals_in_Clause(l, c, j)
            p[LIT_ID(l)] += 2;
    }

    Literal ls[3];
    Literal key = NULL_LITERAL;
    sint max_h = 0;
    for(sint i=0;i<buff->clauses_length3_cnt;i++)
    {
        sint ln = 0;
        Clause *c = clause3[i];
        Loop_Literals_in_Clause(l, c, j)
            ls[ln++] = l;
        Literal l0 = ls[0],l1 = ls[1],l2 = ls[2];
        Literal nl0 = LIT_NEG(l0), nl1 = LIT_NEG(l1), nl2 = LIT_NEG(l2);
        h[LIT_ID(l0)] += p[LIT_ID(nl1)] * p[LIT_ID(nl2)];
        h[LIT_ID(l1)] += p[LIT_ID(nl0)] * p[LIT_ID(nl2)];
        h[LIT_ID(l2)] += p[LIT_ID(nl0)] * p[LIT_ID(nl1)];
        if(h[LIT_ID(l0)] > max_h){
            key = l0;
            max_h = h[LIT_ID(l0)];
        }
        if(h[LIT_ID(l1)] > max_h){
            key = l1;
            max_h = h[LIT_ID(l1)];
        }
        if(h[LIT_ID(l2)] > max_h){
            key = l2;
            max_h = h[LIT_ID(l2)];
        }
    }
    return key;
}
static inline int formula_empty(Formula *fu){
    return fu->first_cluase == fu->clause_end;
}
static inline Literal formula_choose_first_literal(Formula *fu, Recoder *rec){
    Literal l = 0;
    Loop_Clauses_in_Formula(c, fu, i)
    {
        Loop_Literals_in_Clause(_l,c,j)
        {
                l = _l;
                break;
        }
    }

    return l;
}

/* static inline Literal formula_choose_activity_literal(Formula *fu, Recoder *rec){
    Literal l = NULL_LITERAL;
    double h = -1.0;
    for(sint i=1;i<=fu->literal_cnt;i++){
        Literal l0 = LIT_FROM_ID (i);
        double act = rec->lit_activity[i];
    }
    return l;
} */

static inline Literal formula_choose_literal(Formula *fu, Recoder *rec, Info_buff *buff)
{
    Literal l = 0;
    info_buff_reset(buff);
    Loop_Clauses_in_Formula(c, fu, i)
    {
        if (c->length == 0){
            //rec->conflict_clause = c;
            return ERROR_LITERAL;
        } //else if(c->length == 3){
        //    buff->clauses_length3[buff->clauses_length3_cnt++] = c;
        //} else if(c->length == 2){
        //    buff->clauses_length2[buff->clauses_length2_cnt++] = c;
        //}
        if (l == 0)
        {
            Loop_Literals_in_Clause(_l,c,j)
            {
                    l = _l;
                    break;
            }
        }
    }

    //if(buff->clauses_length3_cnt){
    //if(0){    
    //    Literal key = formula_select_key_literal(fu,buff);
    //    if(key != NULL_LITERAL){
    //        l = key;
    //    }
    //}
    return l;
}
static inline Literal* recoder_form_result(Formula *fu,Recoder *rec){
    Literal* res = malloc(sizeof(Literal) * (fu->literal_cnt + 1));

    for(int i=0;i<=fu->literal_cnt;i++){
        res[i] = rec->vas[i].l;
    }

    return res;
}

/* static int conflict_anal0(variable_assign *vas, sint* visited,sint *added, Literal* cic, sint n,const  Clause *c){
    for(sint i=0;i<c->ori_length;i++){
        Literal l = c->ls[i];
        Variable v = LIT_to_VAR(l);

        variable_assign *va = &vas[v];
        sint id = va->impl_clause_id;
        if(va->impl_clause == NULL && !added[v]){
            added[v] = 1;
            cic[n++] = LIT_NEG(LIT_REMOVE_DEL_BIT(va->l));
        } else if(!visited[id]) {
            visited[id] = 1;
            n = conflict_anal0(vas,visited,added, cic,n,va->impl_clause);
        }
    }
    return n;
} */

/* Clause *conflict_anal(Formula* fu, Recoder *rec, Info_buff* buff){
    Literal *cic = buff->conflict_induced_clause;
    sint *visited = buff->visited;
    sint *added = buff->p;
    memset(added,0x0,sizeof(sint) * fu->literal_cnt + 1);
    memset(visited,0x0,sizeof(sint) * fu->clause_cnt);
    visited[0] = 1;
    sint n = conflict_anal0(rec->vas,visited,added,cic,0,rec->conflict_clause);

    Clause *c = new_clause(n);
    memcpy(&c->ls,cic,sizeof(Literal)*n);
    return c;
} */

static void unitary_clause_anal(Recoder *rec,Clause *c, int cluase_id){
    Loop_Literals_in_Clause(l,c,k)
        break;
    Variable v = LIT_to_VAR(l);

    env_result_add(rec,l,c,cluase_id);
}

static inline void record_conflict_clause(Recoder *rec, Clause* c){
    rec->conflict_clause[rec->conflict_clause_cnt++] = c;
}

static int Propagat(Formula *fu, Recoder *rec){
    register variable_assign *vas = rec->vas;
    int new_var_idx = fu->clause_cnt + 1;
    int conflict_idx = 0, new_var;

    while(1){
        new_var = 0;
        Loop_Clauses_in_Formula(c,fu,i){
            if(i > new_var_idx && !new_var)
                break;
            
            Loop_Literals_in_Clause(l,c,j){
                Variable v = LIT_to_VAR(l);
                Literal val = vas[v].l;
                //printf("%d",c->length);
                if(val == NULL_LITERAL){
                    continue;
                }else if(val == l){ // satisify
                    formula_remove_clause_recoded(fu,rec,i,c);
                    break;
                } else {
                    clause_remove_literal(fu,rec,c,l,j);
                    if(c->length == 0){
                        //formula_remove_clause_recoded(fu,rec,i,c);
                        record_conflict_clause(rec,c);

                        conflict_idx = i;
                        goto __conflict_detected;
                    } else if(c->length == 1){
                        // unitary clause
                        //clause_remove_literal(fu,rec,c,l,j);// TODO:
                        //formula_remove_clause_recoded(fu,rec,i,c);
                        unitary_clause_anal(rec,c,i);

                        new_var_idx = i;
                        new_var = 1;
                        break;
                    } else {
                        //clause_remove_literal(fu,rec,c,l,j);
                    }
                }
            }
        }

        if(!new_var)
            break;
    }
    return 0;

    __conflict_detected:;
    if(new_var){// then check clauses before new_var_idx
        Loop_Clauses_in_Formula(c,fu,i){
            if(i >= new_var_idx)
                break;
            if(c->length > 2)
                continue;
            Loop_Literals_in_Clause(l,c,j){
                Variable v = LIT_to_VAR(l);
                Literal val = vas[v].l;
                if(val == LIT_NEG(l)){ 
                    if(c->length == 1){
                        record_conflict_clause(rec,c);
                    } else {// c->length == 2;
                        for(sint k=j+1;k<c->ori_length;k++){
                            Literal l = c->ls[k];
                            if(IS_DELETED(l))
                                continue;
                            Variable v = LIT_to_VAR(l);
                            Literal val = vas[v].l;
                            if(val == LIT_NEG(l)){
                                record_conflict_clause(rec,c);
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }
    }

    // then check clauses after conflict_idx
    //int i = conflict_idx + 1;
    //if(i <= new_var_idx)
    //    return 1;
    for(int i = conflict_idx + 1;i<fu->clause_end;i++){
        Clause*c = fu->cs[i];
        if(!c)
            continue;

        if(c->length > 2)
            continue;
        
        Loop_Literals_in_Clause(l,c,j){
            Variable v = LIT_to_VAR(l);
            Literal val = vas[v].l;
            if(val == LIT_NEG(l)){ 
                if(c->length == 1){
                    record_conflict_clause(rec,c);
                } else {// c->length == 2;
                    for(sint k=j+1;k<c->ori_length;k++){
                        Literal l = c->ls[k];
                        if(IS_DELETED(l))
                            continue;
                        Variable v = LIT_to_VAR(l);
                        Literal val = vas[v].l;
                        if(val == LIT_NEG(l)){
                            record_conflict_clause(rec,c);
                        }
                        break;
                    }
                }
            }
            break;
        }
    }
    return 1;
}

Literal *DPLL(Formula *fu)
{
    fu = formula_copy(fu);
    Recoder *rec = new_recoder(fu);
    //Info_buff *buff = new_info_buff(fu);
    //Dformula *dfu = new_dformula(fu->type,fu->literal_cnt,1024);

    recoder_push(fu, rec, NULL_LITERAL, NORMAL_TAG);

    Literal l;
    sint tag = NORMAL_TAG;

    while (1)
    {
        int conflict = Propagat(fu,rec);
        
        int success = formula_empty(fu);
        if(success){
                Literal *res = recoder_form_result(fu,rec);
                
                //TODO: free
                return res;
        }

        if(conflict){
            //puts("conf");
            while(rec->step_top) {
                l = recoder_pop(fu, rec);
                if(l != NULL_LITERAL)
                    break;
            }
            if (rec->step_top == 0)
                return NULL;
            tag = NEG_TAG;
        } else {
            l = formula_choose_first_literal(fu,rec);
            tag = NORMAL_TAG;
        }

        recoder_push(fu, rec, l, tag);
        l = (tag == NEG_TAG) ? LIT_NEG(l) : l;
        //printf("\nchoose:%d\n",LIT_TO_INT(l));
        env_result_add(rec, l, NULL, 0);
    }
    return NULL;
}

void print_result(Formula* fu,Literal *res) {
    if(!res){
        printf("FIVE!\n");
        return;
    }
    for(int i=1;i<=fu->literal_cnt;i++){
        Literal l = res[i];
        printf("%d ", LIT_TO_INT(l));
    }
    putchar('\n');
}

void test(){
    const char* cnf_filename = "tests/spec_test";
    Formula *fu = new_formula_from_file(cnf_filename);
    Recoder *rec = new_recoder(fu);

    formula_print(fu);
    recoder_push(fu, rec, NULL_LITERAL, NORMAL_TAG);

    env_result_add(rec, LIT_FROM_INT(1), NULL, 0);
    env_result_add(rec, LIT_FROM_INT(4), NULL, 0);
    int i = Propagat(fu,rec);
    if(i) puts("conf");
    formula_print(fu);

}