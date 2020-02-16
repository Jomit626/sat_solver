#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

    rec->ls = malloc(sizeof(Literal) * (fu->literal_cnt + 1));
    rec->ls_cnt = 0;

    return rec;
}

void recoder_free(Recoder *rec){
    // TODO:
    while(rec->dcs_top--){
    }
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

void formula_remove_literal(Formula *fu, Recoder *rec, register Literal l)
{
    register Literal nl = LIT_NEG(l);

    Loop_Clauses_in_Formula(c,fu,i)
    {
        Loop_Literals_in_Clause(l1,c,j)
        {
            if (l1 == l)
            {
                // remove this clause
                formula_remove_clause_recoded(fu, rec, i, c);
                break;
            }
            else if (l1 == nl)
            {
                // remove this literal
                clause_remove_literal(fu, rec, c, l1, j);

                if (c->length == 1)
                    recode_unitary_clause(fu, rec, c, i);
                else if(c->length == 0)
                    break;// TODO:
                break;
            }
        }
    }
}

static inline void env_result_add(Formula *fu, Recoder *rec, Literal l)
{
    for (sint i = 0; i < rec->ls_cnt; i++)
    {
        if (LIT_to_VAR(rec->ls[i]) == LIT_to_VAR(l))
            return;
    }

    rec->ls[rec->ls_cnt++] = l;
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
static inline Literal formula_choose_literal(Formula *fu, Info_buff *buff)
{
    if(formula_empty(fu))
        return SUCCESS_LITERAL;
    // choose the first literal
    // and check other things
    Literal l = 0;
    info_buff_reset(buff);
    Loop_Clauses_in_Formula(c, fu, i)
    {
        if (c->length == 0){
            return ERROR_LITERAL;
        } else if(c->length == 3){
            buff->clauses_length3[buff->clauses_length3_cnt++] = c;
        } else if(c->length == 2){
            buff->clauses_length2[buff->clauses_length2_cnt++] = c;
        }
        if (l == 0)
        {
            Loop_Literals_in_Clause(_l,c,j)
            {
                    l = _l;
                    break;
            }
        }
    }
    if(buff->clauses_length3_cnt){
    //if(0){    
        Literal key = formula_select_key_literal(fu,buff);
        if(key != NULL_LITERAL){
            l = key;
        }
    }
    return l;
}
static inline Literal* recoder_form_result(Recoder *rec){
    int n = rec->ls_cnt;
    Literal* res = malloc(sizeof(Literal) * (n + 1));

    memcpy(res,rec->ls,sizeof(Literal) * n);
    res[n] = NULL_LITERAL;
    
    return res;
}

Literal *DPLL(Formula *fu)
{
    fu = formula_copy(fu);
    Recoder *rec = new_recoder(fu);
    Info_buff *buff = new_info_buff(fu);
    recoder_push(fu, rec, NULL_LITERAL, NORMAL_TAG);

    Literal l;
    sint tag = NORMAL_TAG;

    while ((l = formula_choose_literal(fu,buff)))
    {
        if (l == SUCCESS_LITERAL)
        {
            formula_free(fu);
            info_buff_free(buff);
            
            Literal *res = recoder_form_result(rec);
            recoder_free(rec);
            return res;
        }
        else if (l == ERROR_LITERAL)
        {
            while (rec->step_top && !(l = recoder_pop(fu, rec)))
                ;
            if (rec->step_top == 0)
                return NULL;
            tag = NEG_TAG;
        }
        else
        {
            tag = NORMAL_TAG;
        }
        recoder_push(fu, rec, l, tag);
        l = (tag == NEG_TAG) ? LIT_NEG(l) : l;
        env_result_add(fu, rec, l);
        formula_remove_literal(fu, rec, l);

        // unitary clause rule
        while (rec->ucs_top)
        {
            unitary_clause *uc = &rec->ucs_base[--rec->ucs_top];

            if (!fu->cs[uc->id]) // if the clause is still in the formula
                continue;

            Literal l = uc->l;
            env_result_add(fu, rec, l);

            formula_remove_literal(fu, rec, l);
        }
    }
    return NULL;
}
void test(){

}