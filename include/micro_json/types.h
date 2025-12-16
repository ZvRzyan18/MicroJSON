#ifndef MC_JSON_TYPES_H
#define MC_JSON_TYPES_H

#include "micro_json/config.h"

/*
 compiler hints
*/

#if defined(__GNUC__) || defined(__clang__)
#define MJS_Likely(x)   __builtin_expect(!!(x), 1)
#define MJS_Unlikely(x) __builtin_expect(!!(x), 0)
#define MJS_HOT __attribute__((hot, unused))
#define MJS_COLD __attribute__((cold, unused))

#define MJS_INLINE static __inline__ __attribute__((always_inline, unused, hot))

typedef long long MJS_Int64;
typedef unsigned long long MJS_Uint64;

#define MJS_CountTrailingZeroes __builtin_ctz

#elif defined(_MSC_VER)

#define MJS_Likely(x)   (x)
#define MJS_Unlikely(x) (x)
#define MJS_HOT 
#define MJS_COLD

#define MJS_INLINE static __forceinline

typedef __int64 MJS_Int64;
typedef __uint64 MJS_Uint64;

/* Bruijn algoritm */
MJS_INLINE unsigned short MJS_CountTrailingZeroes(unsigned short x) {
 return (unsigned short)mjs__bruijin_numbers[((x & -x) * 0x077CB531u) >> 27];
}

#else

typedef long int MJS_Int64;
typedef unsigned long int MJS_Uint64;

#define MJS_Likely(x)   (x)
#define MJS_Unlikely(x) (x)
#define MJS_HOT 
#define MJS_COLD

#define MJS_INLINE static


MJS_INLINE unsigned short MJS_CountTrailingZeroes(unsigned short x) {
 return (unsigned short)mjs__bruijin_numbers[((x & -x) * 0x077CB531u) >> 27];
}

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
 MJS_TYPE_NUMBER_DOUBLE = 8,
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
 MJS_RESULT_ALLOCATION_FAILED = -4,
 MJS_RESULT_INVALID_TYPE = -5,
 MJS_RESULT_REACHED_MAX_NESTED_DEPTH = -6,
 MJS_RESULT_EMPTY_KEY = -7,
 MJS_RESULT_DUPLICATE_KEY = -8,
 MJS_RESULT_INCOMPLETE_STRING_SYNTAX = -9,
 MJS_RESULT_INVALID_ESCAPE_SEQUENCE = -10,
 MJS_RESULT_INVALID_HEX_VALUE = -11,
 MJS_RESULT_INVALID_WRITE_MODE = -12,
 MJS_RESULT_UNSUCCESSFUL_IO_WRITE = -13,
 MJS_RESULT_TOO_LARGE_NUMBER = -14,
 MJS_RESULT_TOO_SMALL_NUMBER = -15,
 MJS_RESULT_INVALID_STRING_CHARACTER = -16,
 MJS_RESULT_INVALID_NUMBER_TYPE = -17,
} MJS_RESULT;


/* convert code into constant string literals */
MJS_INLINE const char* MJS_CodeToString(signed char code) {
 switch(code) {
  case MJS_RESULT_NO_ERROR:
   return "MJS_RESULT_NO_ERROR";
  break;
  case MJS_RESULT_NULL_POINTER:
   return "MJS_RESULT_NULL_POINTER";
  break;
  case MJS_RESULT_SYNTAX_ERROR:
   return "MJS_RESULT_SYNTAX_ERROR";
  break;
  case MJS_RESULT_UNEXPECTED_TOKEN:
   return "MJS_RESULT_UNEXPECTED_TOKEN";
  break;
  case MJS_RESULT_ALLOCATION_FAILED:
   return "MJS_RESULT_ALLOCATION_FAILED";
  break;
  case MJS_RESULT_INVALID_TYPE:
   return "MJS_RESULT_INVALID_TYPE";
  break;
  case MJS_RESULT_REACHED_MAX_NESTED_DEPTH:
   return "MJS_RESULT_REACHED_MAX_NESTED_DEPTH";
  break;
  case MJS_RESULT_EMPTY_KEY:
   return "MJS_RESULT_EMPTY_KEY";
  break;
  case MJS_RESULT_DUPLICATE_KEY: /* currently skipped */
   return "MJS_RESULT_DUPLICATE_KEY";
  break;
  case MJS_RESULT_INCOMPLETE_STRING_SYNTAX:
   return "MJS_RESULT_INCOMPLETE_STRING_SYNTAX";
  break;
  case MJS_RESULT_INVALID_ESCAPE_SEQUENCE:
   return "MJS_RESULT_INVALID_ESCAPE_SEQUENCE";
  break;
  case MJS_RESULT_INVALID_HEX_VALUE:
   return "MJS_RESULT_INVALID_HEX_VALUE";
  break;
  case MJS_RESULT_INVALID_WRITE_MODE:
   return "MJS_RESULT_INVALID_WRITE_MODE";
  break;
  case MJS_RESULT_UNSUCCESSFUL_IO_WRITE:
   return "MJE_RESULT_UNSUCCESSFUL_IO_WRITE";
  break;
  case MJS_RESULT_TOO_LARGE_NUMBER:
   return "MJS_RESULT_TOO_LARGE_NUMBER";
  break;
  case MJS_RESULT_TOO_SMALL_NUMBER:
   return "MJS_RESULT_TOO_SMALL_NUMBER";
  break;
  case MJS_RESULT_INVALID_STRING_CHARACTER:
   return "MJS_RESULT_INVALID_STRING_CHARACTER";
  break;
  case MJS_RESULT_INVALID_NUMBER_TYPE:
   return "MJS_RESULT_INVALID_NUMBER_TYPE";
  break;
 }
 return "Unknown Error";
}




#endif

