#ifndef LITERAL_HEAD
#define LITERAL_HEAD

#include <stdint.h>
#include <limits.h>

typedef uint32_t Literal;
typedef int32_t sint;

#define MAX_LITERAL_CNT ((uint32_t)0x3fffffff)

#define ID_MASK     ((uint32_t)0xfffffffc)
#define NEG_MASK    ((uint32_t)0x00000002)
#define DEL_MASK    ((uint32_t)0x00000001)

#define NULL_LITERAL ((uint32_t)0)

#define ID(_x) ((_x)>>1)
#define VAR_ID(_x) ((_x)>>2)

#define NEG_ID(_x) ((_x)^(NEG_MASK>>1))

#define IS_NEG(_x) ((_x)&NEG_MASK)
#define IS_DELETED(_x) ((_x)&DEL_MASK)

#define NEG(_x) ((_x)^NEG_MASK)
#define REMOVE_DEL(_x) ((_x)&~DEL_MASK)

#define SUCCESS_LITERAL (NEG_MASK|DEL_MASK)
#define ERROR_LITERAL (DEL_MASK)

#define TO_INT(_x) (IS_NEG(_x) ? (-(_x>>2)) : (_x>>2))
#define FROM_INT(_x) (((_x) < 0) ? NEG((-(_x)) << 2) : ((_x)<<2) )
#define FROM_ID(_x) ((_x)<<1)

#endif
