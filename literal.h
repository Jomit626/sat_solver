#ifndef LITERAL_HEAD
#define LITERAL_HEAD

#include <stdint.h>
#include <limits.h>

#ifdef SAT_USE_64_BIT

typedef uint64_t Literal;
typedef uint64_t sint;

#define MAX_LITERAL_CNT ((uint64_t)0x3fffffffffffffff)

#define ID_MASK     ((uint64_t)0x3fffffffffffffff)
#define NEG_MASK    ((uint64_t)0x4000000000000000)
#define DEL_MASK    ((uint64_t)0x8000000000000000)

#define NULL_LITERAL ((uint64_t)0)

#else
#define SAT_USE_32_BIT

typedef uint32_t Literal;
typedef uint32_t sint;

#define MAX_LITERAL_CNT ((uint32_t)0x3fffffff)

#define ID_MASK     ((uint32_t)0x3fffffff)
#define NEG_MASK    ((uint32_t)0x40000000)
#define DEL_MASK    ((uint32_t)0x80000000)

#define NULL_LITERAL ((uint32_t)0)

#endif

#define ID(_x) (_x&ID_MASK)
#define IS_NEG(_x) (_x&NEG_MASK)
#define IS_DELETED(_x) (_x&DEL_MASK)

#define NEG(_x) (_x^NEG_MASK)
#define REMOVE_DEL(_x) (_x&~DEL_MASK)

#define SUCCESS_LITERAL (NEG_MASK|DEL_MASK)
#define ERROR_LITERAL (NULL_LITERAL|DEL_MASK)

#endif
