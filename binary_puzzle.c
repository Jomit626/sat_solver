#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "binary_puzzle.h"

typedef struct{
    Formula *fu;
    Clause **cs;
    int cap;
    int n;
} formula_constructor;

static formula_constructor* new_fuc(){
    formula_constructor* fuc = malloc(sizeof(formula_constructor));
    fuc->fu = new_formula(1,0,1024);
    fuc->cap = 1024;
    fuc->n = 0;
    fuc->cs = fuc->fu->cs;
    return fuc;
}

static inline Formula* fuc_get_fu(formula_constructor *fuc,int lit_cnt){
    Formula *fu = fuc->fu;
    fu->literal_cnt = lit_cnt;
    fu->clause_end = fuc->n;
    fu->clause_cnt = fuc->n;
    return fu;
}

static inline void formula_add_clause(formula_constructor* fuc, Clause *c){
    fuc->cs[fuc->n++] = c;
    if(fuc->n >= fuc->cap){
        fuc->cap += 1024;
        fuc->cs = realloc(fuc->cs,sizeof(Clause*) * fuc->cap);
        fuc->fu->cs = fuc->cs;
    }
}
// Tseitin Transform
// z === a V b
// (-c V a V b) and (z V -a) and (z V -b)
static inline void add_tseitin_tranform_or2(formula_constructor *fuc,Literal z, Literal a, Literal b){
    Clause *c1 = new_clause(3);formula_add_clause(fuc,c1);
    Clause *c2 = new_clause(2);formula_add_clause(fuc,c2);
    Clause *c3 = new_clause(2);formula_add_clause(fuc,c3);
    c1->ls[0] = LIT_NEG(z); c2->ls[0] =         z ; c3->ls[0] =         z ;
    c1->ls[1] =         a ; c2->ls[1] = LIT_NEG(a); c3->ls[1] = LIT_NEG(b);
    c1->ls[2] =         b ;
}

// c === a ^ b
// (c V -a V -b) and (-z V a) and (-z V b)
static inline void add_tseitin_tranform_and2(formula_constructor *fuc,Literal z, Literal a, Literal b){
    Clause *c1 = new_clause(3);formula_add_clause(fuc,c1);
    Clause *c2 = new_clause(2);formula_add_clause(fuc,c2);
    Clause *c3 = new_clause(2);formula_add_clause(fuc,c3);
    c1->ls[0] =         z ; c2->ls[0] = LIT_NEG(z); c3->ls[0] = LIT_NEG(z);
    c1->ls[1] = LIT_NEG(a); c2->ls[1] =         a ; c3->ls[1] =         b ;
    c1->ls[2] = LIT_NEG(b);
}

// map of size n
//    0 1 2 ... n-1
//0   1 2 3 ... n
//1   n+1 .
//2   .
//.   
//.   
//.   
//n-1 
#define Lit_x_ij(_i,_j,_n) (((_n)*(_i) + (_j) + 1) << 2)

#define Tseitin_Variable_cnt (3*n)
#define Lit_a(_i) ((literal_id_start + (_i)) << 2)
#define Lit_b(_i) ((literal_id_start + n + (_i)) << 2)
#define Lit_c(_i) ((literal_id_start + 2*n + (_i)) << 2)

// no equ colunm
//Tseitin vairable  a_i === xij_1 ^ xij_2
//                  b_i === -xij_1 ^ -xij_2
//                  c_i === a_i V b_i
//                  V-c_i === no equ colunm
int col_no_equ(formula_constructor *fuc,int j1, int j2,int n, int literal_id_start){
    for(int i=0;i<n;i++){
        add_tseitin_tranform_and2(fuc,Lit_a(i),Lit_x_ij(i,j1,n),Lit_x_ij(i,j2,n));
        add_tseitin_tranform_and2(fuc,Lit_b(i),LIT_NEG(Lit_x_ij(i,j1,n)),LIT_NEG(Lit_x_ij(i,j2,n)));
        add_tseitin_tranform_or2(fuc,Lit_c(i),Lit_a(i),Lit_b(i));
    }
    Clause *c = new_clause(n);formula_add_clause(fuc,c);
    for(int i=0;i<n;i++)
        c->ls[i] = LIT_NEG(Lit_c(i));
    return literal_id_start + Tseitin_Variable_cnt;
}

// no equ row
//Tseitin vairable  a_i === xi_1j ^ xi_2j
//                  b_i === -xi_1j ^ -xi_2j
//                  c_i === a_i V b_i
//                  V-c_i === no equ row
int row_no_equ(formula_constructor *fuc,int i1, int i2,int n, int literal_id_start){
    for(int j=0;j<n;j++){
        add_tseitin_tranform_and2(fuc,Lit_a(j),Lit_x_ij(i1,j,n),Lit_x_ij(i2,j,n));
        add_tseitin_tranform_and2(fuc,Lit_b(j),LIT_NEG(Lit_x_ij(i1,j,n)),LIT_NEG(Lit_x_ij(i2,j,n)));
        add_tseitin_tranform_or2(fuc,Lit_c(j),Lit_a(j),Lit_b(j));
    }
    Clause *c = new_clause(n);formula_add_clause(fuc,c);
    for(int j=0;j<n;j++)
        c->ls[j] = LIT_NEG(Lit_c(j));
    return literal_id_start + Tseitin_Variable_cnt;
}

void m_choose_n(formula_constructor *fuc,int *nums,int size,int m,int n,int last){
    if(size == n){
        for(int i=0;i<m;i++){
            Clause *c1 = new_clause(n);formula_add_clause(fuc,c1);
            Clause *c2 = new_clause(n);formula_add_clause(fuc,c2);
            Clause *c3 = new_clause(n);formula_add_clause(fuc,c3);
            Clause *c4 = new_clause(n);formula_add_clause(fuc,c4);
            for(int j=0;j<n;j++){
                int k = nums[j];
                c1->ls[j] = Lit_x_ij(i,k,m);
                c2->ls[j] = LIT_NEG(Lit_x_ij(i,k,m));
                c3->ls[j] = Lit_x_ij(k,i,m);
                c4->ls[j] = LIT_NEG(Lit_x_ij(k,i,m));
            }
        }
        return;
    }
    for(int i=last+1;i<m;i++){
        nums[size] = i;
        m_choose_n(fuc,nums,size+1,m,n,i);
    }
}
Formula* restrict_formula_of_binary_puzzle(int n, int* map){
    formula_constructor *fuc = new_fuc();

    for(int i=1;i<=n*n;i++){
        if(map[i] == 1){
            Clause*c = new_clause(1);formula_add_clause(fuc,c);
            c->ls[0] = LIT_FROM_INT(i);
        } else if(map[i] == 0){
            Clause*c = new_clause(1);formula_add_clause(fuc,c);
            c->ls[0] = LIT_FROM_INT(-i);
        }
    }

    for(int i=0;i<n-2;i++){
        for(int j=0;j<n;j++){
            Clause *c1 = new_clause(3);Clause *c2 = new_clause(3);
            Clause *c3 = new_clause(3);Clause *c4 = new_clause(3);
            formula_add_clause(fuc,c1);formula_add_clause(fuc,c2);
            formula_add_clause(fuc,c3);formula_add_clause(fuc,c4);
            c1->ls[0] = Lit_x_ij(i,j,n);c1->ls[1] = Lit_x_ij(i+1,j,n);c1->ls[2] = Lit_x_ij(i+2,j,n);
            c2->ls[0] = LIT_NEG(Lit_x_ij(i,j,n));c2->ls[1] = LIT_NEG(Lit_x_ij(i+1,j,n));c2->ls[2] = LIT_NEG(Lit_x_ij(i+2,j,n));
            c3->ls[0] = Lit_x_ij(j,i,n);c3->ls[1] = Lit_x_ij(j,i+1,n);c3->ls[2] = Lit_x_ij(j,i+2,n);
            c4->ls[0] = LIT_NEG(Lit_x_ij(j,i,n));c4->ls[1] = LIT_NEG(Lit_x_ij(j,i+1,n));c4->ls[2] = LIT_NEG(Lit_x_ij(j,i+2,n));
        }
    }

    int *nums = malloc(sizeof(int) * n);
    m_choose_n(fuc,nums,0,n,n/2+1,-1);
    free(nums);

    int literal_id_start = LIT_to_VAR(Lit_x_ij(n-1,n-1,n)) + 1;
    for(int i=0;i<n - 1;i++){
        for(int j=i + 1;j<n;j++){
            literal_id_start = row_no_equ(fuc,i,j,n,literal_id_start);
            literal_id_start = col_no_equ(fuc,i,j,n,literal_id_start);
        }
    }
    return fuc_get_fu(fuc,literal_id_start);
}

Puzzle * new_puzzle_from_file(const char* filename){
    FILE* f = fopen(filename,"r");
    if(!f)
        return NULL;

    int size;
    fscanf(f,"%d",&size);
    assert( (size & 1) == 0);

    int c;
    int n = 1;
    Puzzle *puz = malloc(sizeof(Puzzle) + sizeof(int) *(size * size + 1));
    while((c = fgetc(f)) && n <= (size * size)){
        if(isblank(c))
            continue;
        else if(c == '1')
            puz->map[n++] = 1;
        else if(c == '0')
            puz->map[n++] = 0;
        else if(c == '.')
            puz->map[n++] = 2;
    }
    while (n <= (size * size))
        puz->map[n++] = 2;
    
    puz->size = size;
    puz->fu = restrict_formula_of_binary_puzzle(size, puz->map);

    formula_sort(puz->fu);
    fclose(f);
    return puz;
}

void puzzle_print_map(Puzzle* puz){
    int *map = puz->map;
    int n = puz->size;
    for(int i=1;i<=n*n;i++){
        if(map[i] == 0)
            putchar('0');
        else if(map[i] == 1)
            putchar('1');
        else if(map[i] == 2)
            putchar('.');
        if(i%n == 0)
            putchar('\n');
    }
}

#ifdef DEBUG
int main(){
    FILE *f = fopen("tmp/cnf","w");
    Puzzle *puz =  new_puzzle_from_file("tests/bp10.map");
    formula_dump(puz->fu,f);

}
#endif