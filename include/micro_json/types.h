#ifndef MC_JSON_TYPES_H
#define MC_JSON_TYPES_H

/*
 compiler hints
*/
#if defined(__GNUC__) || defined(__clang__)
#define MJS_Likely(x)   __builtin_expect(!!(x), 1)
#define MJS_Unlikely(x) __builtin_expect(!!(x), 0)
#define MJS_HOT __attribute__((hot))
#define MJS_COLD __attribute__((cold))
#else
#define MJS_Likely(x)   (x)
#define MJS_Unlikely(x) (x)
#define MJS_HOT 
#define MJS_COLD
#endif

/*
 types
*/
typedef enum {
 MJS_TYPE_STRING = 1,
 MJS_TYPE_BOOLEAN = 2,
 MJS_TYPE_NULL = 3,
 MJS_TYPE_OBJECT = 4,
 MJS_TYPE_ARRAY = 5,
 MJS_TYPE_NUMBER_INT = 6,
 MJS_TYPE_NUMBER_FLOAT = 7,
} MJS_TYPE;

/*
 write mode
*/
typedef enum {
 MJS_WRITE_TO_MEMORY_BUFFER = 1,
 MJS_WRITE_TO_FILE = 2,
} MJS_WRITE_MODE;
/*
 error defs
*/
typedef enum {
 MJS_RESULT_NO_ERROR = 0,
 MJS_RESULT_NULL_POINTER = -1,
 MJS_RESULT_SYNTAX_ERROR = -2,
 MJS_RESULT_UNEXPECTED_TOKEN = -3,
 MJS_RESULT_INCOMPLETE_BRACKETS = -4,
 MJS_RESULT_INCOMPLETE_DOUBLE_QUOTES = -5,
 MJS_RESULT_ALLOCATION_FAILED = -6,
 MJS_RESULT_INVALID_TYPE = -7,
 MJS_RESULT_REACHED_MAX_NESTED_DEPTH = -8,
 MJS_RESULT_EMPTY_KEY = -9,
 MJS_RESULT_DUPLICATE_KEY = -10,
 MJS_RESULT_INCOMPLETE_STRING_SYNTAX = -11,
 MJS_RESULT_INVALID_ESCAPE_SEQUENCE = -12,
 MJS_RESULT_INVALID_HEX_VALUE = -13,
 MJS_RESULT_INVALID_WRITE_MODE = -14,
 MJE_RESULT_UNSUCCESSFUL_IO_WRITE = -15,
 MJE_RESULT_ROOT_NOT_FOUND = -16,
} MJS_RESULT;

/*
 limits
*/
#define MJS_MAX_CACHE_BYTES       32
#define MJS_MAX_RESERVE_BYTES     16
#define MJS_MAX_RESERVE_ELEMENTS  8
#define MJS_MAX_NESTED_VALUE      10
#define MJS_MAX_HASH_BUCKETS      8
#define MJS_OPTIMAL_ALIGNMENT     16

#define MJS_FORCE_VECTORIZE 

#endif

