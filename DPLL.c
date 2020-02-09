#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "DPLL.h"

#define NORMAL_TAG 0
#define NEG_TAG 1

Step_stack *new_step_stack(Formula *fu)
{
    Step_stack *ss = malloc(sizeof(Step_stack));
    ss->dcs_base = malloc(sizeof(del_clause) * (fu->clause_cnt + 1));
    ss->dcs_top = 0;

    ss->dls_base = malloc(sizeof(del_literal) * (fu->literal_cnt * fu->clause_cnt + 1));
    ss->dls_top = 0;

    ss->step_base = malloc(sizeof(Step) * (fu->literal_cnt + 1));
    ss->step_top = 0;

    ss->ucs_base = malloc(sizeof(unitary_clause) * (fu->clause_cnt + 1));
    ss->ucs_top = 0;

    ss->ls = malloc(sizeof(Literal) * (fu->literal_cnt + 1));
    ss->ls_cnt = 0;

    return ss;
}

static inline void formula_remove_clause(Formula *fu, Step_stack *ss, sint clause_id, Clause *c)
{
    del_clause *dc = &ss->dcs_base[ss->dcs_top++];
    dc->id = clause_id;
    dc->c = c;

    fu->cs[clause_id] = NULL;

    return;
}
static inline void clause_remove_literal(Formula *fu, Step_stack *ss, Clause *c, Literal l, sint literal_id)
{
    del_literal *dl = &ss->dls_base[ss->dls_top++];
    dl->c = c;
    dl->l = &c->ls[literal_id];

    c->ls[literal_id] = l | DEL_MASK;
    c->length--;

    return;
}

static inline void env_add_unitary_clause(Formula *fu, Step_stack *ss, Clause *c, sint clause_id)
{
    unitary_clause *uc = &ss->ucs_base[ss->ucs_top++];
    uc->id = clause_id;
    uc->c = c;

    for (sint i = 0; i < c->ori_length; i++)
        if (!IS_DELETED(c->ls[i]))
        {
            uc->l = c->ls[i];
            break;
        }
}

static inline void formula_remove_literal(Formula *fu, Step_stack *ss, register Literal l)
{
    register Literal nl = NEG(l);

    for (sint i = 0; i < fu->clause_cnt; i++)
    {
        Clause *c = fu->cs[i];
        if (!c)
            continue;
        for (sint j = 0; j < c->ori_length; j++)
        {
            Literal l1 = c->ls[j];
            if (IS_DELETED(l1))
                continue;
            // TODO: batter cmp?
            if (l1 == l)
            {
                // remove this clause
                formula_remove_clause(fu, ss, i, c);
                break;
            }
            else if (l1 == nl)
            {
                // remove this literal
                clause_remove_literal(fu, ss, c, l1, j);

                if (c->length == 1)
                    env_add_unitary_clause(fu, ss, c, i);

                break;
            }
        }
    }
}

static inline void env_result_add(Formula *fu, Step_stack *ss, Literal l)
{
    for(sint i=0;i<ss->ls_cnt;i++){
        if(ID(ss->ls[i]) == ID(l))
            return;
    }
    ss->ls[ss->ls_cnt++] = l;
}
static inline void env_step_push(Formula *fu, Step_stack *ss, Literal l, sint tag)
{
    Step *st = &ss->step_base[ss->step_top++];
    st->l = l;
    st->dcs_top = ss->dcs_top;
    st->dls_top = ss->dls_top;
    st->ls_cnt = ss->ls_cnt;

    ss->ucs_top = 0;

    st->tag = tag;
}
static inline void formula_recover_deleted_clause(Formula *fu, Step_stack *ss, del_clause *dc)
{
    fu->cs[dc->id] = dc->c;
}
static inline void formula_recover_deleted_literal(Formula *fu, Step_stack *ss, del_literal *dl)
{
    *dl->l = REMOVE_DEL(*dl->l);
    dl->c->length++;
}
static inline Literal env_step_pop(Formula *fu, Step_stack *ss)
{
    Step *st = &ss->step_base[--ss->step_top];

    for (sint top = st->dcs_top; top < ss->dcs_top; top++)
    {
        del_clause *dc = &ss->dcs_base[top];
        formula_recover_deleted_clause(fu, ss, dc);
    }
    ss->dcs_top = st->dcs_top;

    for (sint top = st->dls_top; top < ss->dls_top; top++)
    {
        del_literal *dl = &ss->dls_base[top];
        formula_recover_deleted_literal(fu, ss, dl);
    }
    ss->dls_top = st->dls_top;

    ss->ucs_top = 0;

    ss->ls_cnt = st->ls_cnt;

    return (st->tag == NEG_TAG) ? NULL_LITERAL : st->l;
}
static inline Literal formula_choose_literal(Formula *fu)
{
    // choose the first literal
    // and check other things
    Literal l = 0;
    sint clause_cnt = 0;
    for (sint i = 0; i < fu->clause_cnt; i++)
    {
        Clause *c = fu->cs[i];
        if (!c)
            continue;
        if (c->length == 0)
        {
            return ERROR_LITERAL;
        }
        if (l == 0)
        {
            for (sint j = 0; j < c->ori_length; j++)
            {
                if (!IS_DELETED(c->ls[j]))
                {
                    l = c->ls[j];
                }
            }
        }
        clause_cnt++;
    }
    return clause_cnt > 0 ? l : SUCCESS_LITERAL;
}

Literal *DPLL(Formula *fu)
{
    fu = formula_copy(fu);
    Step_stack *ss = new_step_stack(fu);
    env_step_push(fu, ss, NULL_LITERAL, NORMAL_TAG);

    Literal l;
    sint tag = NORMAL_TAG;
    while ((l = formula_choose_literal(fu)))
    {
        if (l == SUCCESS_LITERAL)
        {
            formula_free(fu);// TODO:
            ss->ls[ss->ls_cnt] = NULL_LITERAL;
            return ss->ls;
        }
        else if (l == ERROR_LITERAL)
        {
            while (ss->step_top && !(l = env_step_pop(fu, ss)))
                ;
            if (ss->step_top == 0)
                return NULL;
            tag = NEG_TAG;
        }
        else
        {
            tag = NORMAL_TAG;
        }
        env_step_push(fu, ss, l, tag);
        l = (tag == NEG_TAG) ? NEG(l) : l;
        env_result_add(fu, ss, l);
        formula_remove_literal(fu, ss, l);

        // unitary clause rule
        while(ss->ucs_top){
            unitary_clause *uc = &ss->ucs_base[--ss->ucs_top];

            if(!fu->cs[uc->id]) // if the clause is still in the formula
                continue;
        
            Literal l = uc->l;
            env_result_add(fu,ss,l);
        
            formula_remove_literal(fu,ss,l);
        }
    }
    return NULL;
}
