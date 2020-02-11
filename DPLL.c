#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#include "DPLL.h"

#define NORMAL_TAG 0
#define NEG_TAG 1


Recoder *new_step_stack(Formula *fu)
{
    Recoder *rec = malloc(sizeof(Recoder));
    rec->dcs_base = malloc(sizeof(del_clause) * (fu->clause_cnt + 1));
    rec->dcs_top = 0;

    rec->dls_base = malloc(sizeof(del_literal) * (fu->literal_cnt * fu->clause_cnt + 1));
    rec->dls_top = 0;

    rec->step_base = malloc(sizeof(step) * (fu->literal_cnt + 1));
    rec->step_top = 0;

    rec->ucs_base = malloc(sizeof(unitary_clause) * (fu->clause_cnt + 1));
    rec->ucs_top = 0;

    rec->ls = malloc(sizeof(Literal) * (fu->literal_cnt + 1));
    rec->ls_cnt = 0;

    return rec;
}

static inline void formula_remove_clause(Formula *fu, Recoder *rec, int id, int pre, int next)
{
    //printf("remove %d\n",id);
    del_clause *dc = &rec->dcs_base[rec->dcs_top++];
    dc->pre = pre;
    dc->id = id;
    dc->next = next;

    fu->removed[id] = 1;
    if(pre>=0)fu->next[pre] = next;
    else fu->first = next;
}

static inline void clause_remove_literal(Formula *fu, Recoder *rec, Clause *c, Literal l, sint literal_id)
{
    del_literal *dl = &rec->dls_base[rec->dls_top++];
    dl->c = c;
    dl->l = &c->ls[literal_id];

    c->ls[literal_id] = l | DEL_MASK;
    c->length--;

    return;
}

static inline void reocde_unitary_clause(Formula *fu, Recoder *rec, Clause *c, sint clause_id)
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

static inline void formula_remove_literal(Formula *fu, Recoder *rec, register Literal l)
{
    register Literal nl = NEG(l);

    Loop_Clauses_in_Formula(c,fu,i,pre,next)
    {
        Loop_Literals_in_Clause(l1,c,j)
        {
            if (l1 == l)
            {
                // remove this clause
                formula_remove_clause(fu, rec, i, pre,next);
                i = pre;
                break;
            }
            else if (l1 == nl)
            {
                // remove this literal
                clause_remove_literal(fu, rec, c, l1, j);

                if (c->length == 1)
                    reocde_unitary_clause(fu, rec, c, i);
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
        if (VAR_ID(rec->ls[i]) == VAR_ID(l))
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

    rec->ucs_top = 0;

    st->tag = tag;

}
static inline void formula_recover_deleted_clause(Formula *fu, Recoder *rec, del_clause *dc)
{
    int pre = dc->pre,id = dc->id, next = dc->next;
    //printf("add %d\n",id);
    fu->removed[id] = 0;
    
    if(pre>=0)fu->next[pre] = id;
    else fu->first = id;
}
static inline void formula_recover_deleted_literal(Formula *fu, Recoder *rec, del_literal *dl)
{
    *dl->l = REMOVE_DEL(*dl->l);
    dl->c->length++;
}
static inline Literal recoder_pop(Formula *fu, Recoder *rec)
{
    step *st = &rec->step_base[--rec->step_top];

    for (sint top = rec->dcs_top - 1; top >= 0 && top >= st->dcs_top; top--)
    {
        del_clause *dc = &rec->dcs_base[top];
        formula_recover_deleted_clause(fu, rec, dc);
    }
    rec->dcs_top = st->dcs_top;

    for (sint top = st->dls_top; top < rec->dls_top; top++)
    {
        del_literal *dl = &rec->dls_base[top];
        formula_recover_deleted_literal(fu, rec, dl);
    }
    rec->dls_top = st->dls_top;

    rec->ucs_top = 0;

    rec->ls_cnt = st->ls_cnt;

    return (st->tag == NEG_TAG) ? NULL_LITERAL : st->l;
}

Info_buff *new_info_buff(Formula *fu)
{
    Info_buff *buff = malloc(sizeof(Info_buff));

    buff->clauses_length2_cnt = 0;
    buff->clauses_length2 = malloc(sizeof(Clause*) * fu->clause_cnt);
    buff->clauses_length3_cnt = 0;
    buff->clauses_length3 = malloc(sizeof(Clause*) * fu->clause_cnt);

    buff->cnt = sizeof(sint) * fu->literal_cnt * 2;
    buff->p1 = malloc(buff->cnt);
    buff->p2 = malloc(buff->cnt);
    buff->h = malloc(buff->cnt);

    return buff;
}
static inline void info_buff_reset(Info_buff *buff){
    buff->clauses_length2_cnt = 0;
    buff->clauses_length3_cnt = 0;
}
static inline Literal formula_select_key_literal(Formula *fu, Info_buff *buff)
{
    sint *p = buff->p1;
    sint *h = buff->h;
    memset(p,0x0,buff->cnt);
    memset(h,0x0,buff->cnt);

    Clause **clause3 = buff->clauses_length3;
    Clause **clause2 = buff->clauses_length2;

    for(sint i=0;i<buff->clauses_length3_cnt;i++)
    {
        Clause *c = clause3[i];
        Loop_Literals_in_Clause(l, c, j)
            p[ID(l)] += 1;
    }
    for(sint i=0;i<buff->clauses_length2_cnt;i++)
    {
        Clause *c = clause2[i];
        Loop_Literals_in_Clause(l, c, j)
            p[ID(l)] += 2;
    }

    Literal ls[3];
    Literal key = NULL_LITERAL;
    sint max_h = 0;
    for(sint i=0;i<buff->clauses_length3_cnt;i++)
    {
        sint ln = 0;
        Clause *c = clause3[i];
        Loop_Literals_in_Clause(l, c, j)
        {
            ls[ln++] = ID(l);
        }
        register Literal l0 = ls[0],l1 = ls[1],l2 = ls[2];
        h[l0] += p[NEG_ID(l1)] * p[NEG_ID(l2)];
        h[l1] += p[NEG_ID(l0)] * p[NEG_ID(l2)];
        h[l2] += p[NEG_ID(l0)] * p[NEG_ID(l1)];
        if(h[l0] > max_h){
            key = l0;
            max_h = h[l0];
        }
        if(h[l1] > max_h){
            key = l1;
            max_h = h[l1];
        }
        if(h[l2] > max_h){
            key = l2;
            max_h = h[l2];
        }
    }
    return FROM_ID(key);
}

static inline Literal formula_choose_literal(Formula *fu, Info_buff *buff)
{
    // choose the first literal
    // and check other things
    Literal l = 0;
    sint clause_cnt = 0;
    info_buff_reset(buff);
    //printf("Loop:");
    Loop_Clauses_in_Formula(c, fu, i,pre,next)
    {
        //printf("%d ",i);
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
        clause_cnt++;
    }
    //putchar('\n');
    if(clause_cnt == 0)
        return SUCCESS_LITERAL;
    if(0){
        Literal key = formula_select_key_literal(fu,buff);
        if(key != NULL_LITERAL){
            //printf("Key %d\n",TO_INT(key));
            l = key;
        }
    }
    //printf("choose func:%d\n",TO_INT(l));
    return l;
}

Literal *DPLL(Formula *fu)
{
    fu = formula_copy(fu);
    Recoder *rec = new_step_stack(fu);
    Info_buff *buff = new_info_buff(fu);
    recoder_push(fu, rec, NULL_LITERAL, NORMAL_TAG);

    Literal l;
    sint tag = NORMAL_TAG;

    while ((l = formula_choose_literal(fu,buff)))
    {
        if (l == SUCCESS_LITERAL)
        {
            formula_free(fu); // TODO:
            rec->ls[rec->ls_cnt] = NULL_LITERAL;
            return rec->ls;
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
        l = (tag == NEG_TAG) ? NEG(l) : l;
        env_result_add(fu, rec, l);
        formula_remove_literal(fu, rec, l);

        // unitary clause rule
        while (rec->ucs_top)
        {
            unitary_clause *uc = &rec->ucs_base[--rec->ucs_top];

            if (!fu->removed[uc->id]) // if the clause is still in the formula
                continue;

            Literal l = uc->l;
            env_result_add(fu, rec, l);

            formula_remove_literal(fu, rec, l);
        }
    }
    return NULL;
}
void test(){
    Formula *fu = new_formula_from_file("tests/func_test");
    Recoder *rec = new_step_stack(fu);
    recoder_push(fu, rec, 0, 0);
    formula_print(fu);
    formula_remove_clause(fu,rec,0,-1,1);
    formula_remove_clause(fu,rec,1,-1,-1);
    puts("..................");
    formula_print(fu);
    recoder_pop(fu,rec);
    puts("..................");
    formula_print(fu);
}