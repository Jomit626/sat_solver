#ifndef LITERAL_HEAD
#define LITERAL_HEAD

#include <stdint.h>
#include <limits.h>

typedef uint32_t Literal;
typedef uint32_t Variable;
typedef int32_t sint;
#if 1
#define MAX_VARIABLE_CNT ((uint32_t)0x3fffffff)
#define MAX_LITERAL_CNT ((uint32_t)0x3fffffff)

#define LITERAL_MASK ((uint32_t)0xfffffffe)
#define VARIABLE_MASK ((uint32_t)0xfffffffc)
#define NEG_MASK    ((uint32_t)0x00000002)
#define DEL_MASK    ((uint32_t)0x00000001)

#define NULL_LITERAL ((uint32_t)0)
#define SUCCESS_LITERAL (NEG_MASK|DEL_MASK)
#define ERROR_LITERAL (DEL_MASK)

#define LIT_to_VAR(_x) ((_x)>>2)
#define VAR_to_LIT(_x,_neg) (((_x)<<2) | ((_neg) ? NEG_MASK : 0x0))

#define LIT_NEG(_x) ((_x)^NEG_MASK)
#define LIT_ID(_x) ((_x) >> 1 )

#define IS_NEG(_x) ((_x)&NEG_MASK)
#define IS_DELETED(_x) ((_x)&DEL_MASK)

#define LIT_REMOVE_DEL_BIT(_x) ((_x)&~DEL_MASK)

#define LIT_TO_INT(_x) (IS_NEG(_x) ? (int)(-(_x>>2)) : (int)(_x>>2))
#define LIT_FROM_INT(_x) (((_x) < 0) ? LIT_NEG((-((Literal)(_x))) << 2) : (((Literal)(_x))<<2) )
#define LIT_FROM_ID(_x) ((_x)<<1)
#endif
#endif
