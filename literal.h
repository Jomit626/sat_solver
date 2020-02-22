#ifndef LITERAL_HEAD
#define LITERAL_HEAD

#include <stdint.h>
#include <limits.h>

typedef uint32_t Literal;
typedef uint32_t Variable;

#define MAX_VARIABLE_CNT ((uint32_t)0x3fffffff)
#define MAX_LITERAL_CNT ((uint32_t)0x3fffffff)

#define VARIABLE_MASK ((uint32_t)0xfffffffe)
#define NEG_MASK    ((uint32_t)0x00000001)

#define NULL_LITERAL ((uint32_t)0)
#define ERROR_LITERAL (NULL_LITERAL|DEL_MASK)

#define Lit_to_Var(_x) ((_x)>>1)
//#define Var_to_Lit(_x,_neg) (((_x)<<1) | ((_neg) ? NEG_MASK : 0x0))
#define Var_to_Lit(_x,_neg) ( ((_x) << 1) | ((_neg) && 1))

#define Lit_Neg(_x) ((_x)^NEG_MASK)

#define Lit_Is_Neg(_x) ((_x)&NEG_MASK)

#define Lit_to_Int(_x) (Lit_Is_Neg(_x) ? (int)(-(_x>>1)) : (int)(_x>>1))
#define Lit_from_Int(_x) (((_x) < 0) ? Lit_Neg((-((Literal)(_x))) << 1) : (((Literal)(_x))<<1) )

#endif
