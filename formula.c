#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "formula.h"

#define NORMAL_TAG 0
#define NEG_TAG 1

Formula *new_formula(int type, int ln, int cn)
{
    Formula *fn = (Formula *)malloc(sizeof(Formula) + sizeof(Clause*) * (cn + 1));
    fn->type = type;
    fn->variable_cnt = ln;
    fn->clause_cnt = cn;

    fn->first_cluase = 1;
    fn->clause_end = cn + 1;
    return fn;
}

void formula_print(Formula *fu)
{
    printf("Formula:%p Type:%d Literal:%d Clause:%d start:%d end:%d\n", fu, fu->type, fu->variable_cnt, fu->clause_cnt,fu->first_cluase,fu->clause_end);
    LoopClausesInFormula(fu, i)
    {
        Clause *c = formula_get_clause(fu,i);
        printf("%d-",i);
        clause_print(c);
    }
}

static char buff[2048];
static char buff2[16];
static int cnt = 0;
static Literal lb[2048];

static Literal get_literal(char *s, int *start)
{
    char *st = s;
    Variable var = 0;
    int neg = 0;
    s = s + *start;

    while (isblank(*s))
        s++;

    if (*s == '-')
        neg = 1, s++;

    while (isdigit(*s))
    {
        var *= 10;
        var += *s - '0';
        s++;
    }
    *start = s - st;
    return Var_to_Lit(var,neg);
}

Formula *new_formula_from_file(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if(!f)
        return NULL;
    char *line = buff;
    int start = 0;
    int type, ln, cn;
    int clause_cnt;
    Formula *fu = NULL;

    while (fgets(line, 2048, f))
    {
        start = 0;
        cnt = 0;
        if (line[0] == 'c')
            continue;
        else if (line[0] == 'p')
        {
            sscanf(line + 1, "%s%d%d", buff2, &ln, &cn);
            clause_cnt = 1;
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
            for (int i = 0; i < cnt; i++)
            {
                fu->cs[clause_cnt]->ls[i] = lb[i];
            }
            clause_cnt++;
        }
    }

    return fu;
}

void formula_dump(Formula* fu,FILE *f){
    fprintf(f,"p cnf %d %d\n",fu->variable_cnt, fu->clause_cnt);
    LoopClausesInFormula(fu,i)
    {
        Clause*c = formula_get_clause(fu,i);
        for(int j=0;j<c->length;j++){
            fprintf(f,"%d ",Lit_to_Int(c->ls[j]));
        }
        fprintf(f,"0\n");
    }
}

Formula *formula_copy(Formula *fu){
    Formula * nfu = new_formula(fu->type,fu->variable_cnt,fu->clause_cnt);
    for (int i = 0; i <=fu->clause_cnt; i++)
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
    for (int i = 0; i < fu->clause_cnt; i++)
    {
        Clause* c = formula_get_clause(fu,i);
        if(c)
            clause_free(c);
    }
    free(fu);
}

int formula_satisfy(Formula *fu, VariableAssignment* va){
    fu = formula_copy(fu);
    int n = fu->clause_cnt;
    LoopClausesInFormula(fu,i){
        Clause *c = formula_get_clause(fu,i);
        LoopLiteralsInClause(c,j){
            Literal l = c->ls[j];
            Variable v = Lit_to_Var(l);
            if(va[v].l == l){
                n--;
                break;
            }
        }
    }
    formula_free(fu);
    return n;
}

inline void formula_remove_clause(Formula *fu, int id){
    fu->cs[id] = NULL;
    if(id == fu->first_cluase && id == fu->clause_end - 1){
        fu->first_cluase = fu->clause_end;
    }else if(id == fu->first_cluase){
        for(int i=id;i<fu->clause_end;i++)
            if(fu->cs[i]){
                fu->first_cluase = i;
                break;
            }
    }else if(id == fu->clause_end - 1){
        for(int i=id;i>=fu->first_cluase;i--)
            if(fu->cs[i]){
                fu->clause_end = i + 1;
                break;
            }
    }
}