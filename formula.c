#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "formula.h"

#define NORMAL_TAG 0
#define NEG_TAG 1

Formula *new_formula(sint type, sint ln, sint cn)
{
    Formula *fn = (Formula *)malloc(sizeof(Formula));
    fn->type = type;
    fn->literal_cnt = ln;
    fn->clause_cnt = cn;

    fn->cs = (Clause **)malloc(sizeof(Clause *) * cn);
    return fn;
}

void formula_print(Formula *fu)
{
    printf("Formula:%p Type:%d Literal:%d Clause:%d\n", fu, fu->type, fu->literal_cnt, fu->clause_cnt);
    for (sint i = 0; i < fu->clause_cnt; i++)
    {
        if(fu->cs[i]){
            printf("%d-",i);
            clause_print(fu->cs[i]);
        }
    }
}

static char buff[2048];
static char buff2[16];
static sint cnt = 0;
static Literal lb[2048];

static Literal get_literal(char *s, sint *start)
{
    char *st = s;
    s = s + *start;
    sint neg = 0, num = 0;

    while (isblank(*s))
        s++;

    if (*s == '-')
        neg = 1, s++;

    while (isdigit(*s))
    {
        num *= 10;
        num += *s - '0';
        s++;
    }

    *start = s - st;
    return neg ? NEG(num) : num;
}

Formula *new_formula_from_file(char *filename)
{
    FILE *f = fopen(filename, "r");
    char *line = buff;
    sint start = 0;
    sint type, ln, cn;
    sint clause_cnt;
    Formula *fu = NULL;

    while (fgets(line, 2048, f))
    {
        start = 0;
        cnt = 0;
        //printf("%d get:%s\n",clause_cnt,line);
        //printf("1:%c\n",line[0]);
        if (line[0] == 'c')
            continue;
        else if (line[0] == 'p')
        {
            sscanf(line + 1, "%s%d%d", buff2, &ln, &cn);
            clause_cnt = 0;
            if (strcmp(buff2, "cnf") == 0)
            {
                type = 1; // TODO:
            }
            else
            {
                type = 0;
            }
            fu = new_formula(type, ln, cn);
        }
        else if (fu == NULL)
        {
            // TODO: error
        }
        else
        {
            Literal l = get_literal(line, &start);
            for (; l != 0; l = get_literal(line, &start))
            {
                lb[cnt++] = l;
            }
            fu->cs[clause_cnt] = new_clause(cnt);
            for (sint i = 0; i < cnt; i++)
            {
                fu->cs[clause_cnt]->ls[i] = lb[i];
            }
            clause_cnt++;
        }
    }

    return fu;
}

Formula *formula_copy(Formula *fu){
    Formula * nfu = new_formula(fu->type,fu->literal_cnt,fu->clause_cnt);
    for (sint i = 0; i < fu->clause_cnt; i++)
    {
        Clause* c = fu->cs[i];
        if(c){
            nfu->cs[i] = clause_copy(c);
        } else {
            nfu->cs[i] = NULL;
        }
    }
    return nfu;
}

void formula_free(Formula *fu){
    for (sint i = 0; i < fu->clause_cnt; i++)
    {
        Clause* c = fu->cs[i];
        if(c)
            clause_free(c);
    }
    free(fu->cs);
    free(fu);
}

sint formula_satisfy(Formula *fu, Literal* ls){
    sint n = fu->clause_cnt;
    for(sint i=0;ls[i] != NULL_LITERAL && n;i++){
        Literal l = REMOVE_DEL(ls[i]);
        for(sint j=0;j<fu->clause_cnt && n;j++){
            Clause *c = fu->cs[j];
            if(!c)
                continue;
            for(sint k=0;k<c->ori_length;k++){
                if(REMOVE_DEL(c->ls[k]) == l){
                    fu->cs[j] = NULL;
                    free(c);
                    n--;
                    break;
                }
            }
        }
    }
    return n;
}