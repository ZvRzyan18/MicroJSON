#include "micro_json/token.h"
#include "micro_json/parser.h"
#include "micro_json/object_impl.h"
#include <string.h>

#if (defined(__ARM_NEON) || defined(__ARM_NEON__)) && defined(MJS_FORCE_VECTORIZE)
#include "micro_json/token_neon.h"
#endif

#define _HAS_VALUE          0b1
#define _EXPECTED_FOR_VALUE 0b10
#define _IS_EMPTY           0b100
#define _EXPECTED_FOR_NAME  0b1000
/*
#define _CONDITIONAL_ERROR_ACCUMULATE(err, cond, code) err |= (-((!(err)) & (cond))) & (code)
#define _UNCONDITIONAL_ERROR_ACCUMULATE(err, code)     err = (-((!(err)))) & (code)
#define _CONDITIONAL_ERROR(err, cond, code)            err = (-(cond)) & (code)
*/

MJS_HOT static int read_json_object_first_value(MJSParsedData *parsed_data, MJSStringPool *pool, unsigned int depth);
MJS_HOT static int read_json_object(MJSParsedData *parsed_data, MJSStringPool *pool, MJSObject *container, unsigned int depth);
MJS_HOT static int read_json_object_value(MJSParsedData *parsed_data, MJSStringPool *pool, MJSObject *container, unsigned int pool_index, unsigned int str_size, unsigned short chunk_index, unsigned int depth);
MJS_HOT static int read_json_array_value(MJSParsedData *parsed_data, MJSStringPool *pool, MJSArray *arr, unsigned int depth);

#define _TRUE  "rue"
#define _FALSE "alse"
#define _NULL  "ull"

MJS_INLINE int fast_memcmp_3(const char *a, const char *b) {
 return a[0] != b[0] | a[1] != b[1] | a[2] != b[2];
}

MJS_INLINE int fast_memcmp_4(const char *a, const char *b) {
 return a[0] != b[0] | a[1] != b[1] | a[2] != b[2] | a[3] != b[3];
}

/*-----------------Token func-------------------*/


MJS_HOT MJSTokenResult MJS_TokenParse(MJSParsedData *parsed_data, MJSStringPool *pool, const char *str, unsigned int len) { 

 MJSTokenResult result;
 result.line = 0xFFFFFFFF;
 result.code = MJS_RESULT_NO_ERROR;

 if(MJS_Unlikely(!parsed_data || !str || !len)) {
  result.code = MJS_RESULT_NULL_POINTER;
  return result;
 }
 
 parsed_data->current = str;
 parsed_data->end = str+len;

 result.code = result.code ? result.code : read_json_object_first_value(parsed_data, pool, 0);
 result.line = parsed_data->cl;
 
 return result;
}


/*-----------------Static func-------------------*/


MJS_HOT static int read_json_object_first_value(MJSParsedData *parsed_data, MJSStringPool *pool, unsigned int depth) {
 int result = 0;
 
 while(parsed_data->current < parsed_data->end && !result) {
#if defined(MJS_NEON)
  Neon_read_json_object_value(parsed_data);
#endif
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case 't': /* might be true*/
    result = fast_memcmp_3(++parsed_data->current, _TRUE) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 2;
    parsed_data->container.type = MJS_TYPE_BOOLEAN;
    parsed_data->container.value_boolean.value = 1;
   break;
   case 'f': /* might be false */
    result = fast_memcmp_4(++parsed_data->current, _FALSE) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 3;
    parsed_data->container.type = MJS_TYPE_BOOLEAN;
    parsed_data->container.value_boolean.value = 0;
   break;
   case 'n': /* might be null */
    result = fast_memcmp_3(++parsed_data->current, _NULL) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 2;
    parsed_data->container.type = MJS_TYPE_NULL;
   break;
   case '\"': /* string */
    parsed_data->current++;
    parsed_data->container.type = MJS_TYPE_STRING;
    parsed_data->container.value_string.chunk_index = MJSStringPool_GetCurrentNode_IMPL(pool);
    result = MJS_ParseStringToPool(parsed_data, &pool->root[parsed_data->container.value_string.chunk_index], &parsed_data->container.value_string.pool_index, &parsed_data->container.value_string.str_size);
   break;
   case '-': /* might be int or float */
   case '+': /* might be int or float */
    result = MJS_ParseNumber(parsed_data, &parsed_data->container);
   break;
   case '[': /* an array */
    parsed_data->current++;
    result = MJSArray_Init_IMPL(&parsed_data->container.value_array);
    if(MJS_Likely(!result))
    result = read_json_array_value(parsed_data, pool, &parsed_data->container.value_array, depth);
   break;
   case '{': /* an object */
    parsed_data->current++;
    result = MJSObject_Init_IMPL(&parsed_data->container.value_object);
    if(MJS_Likely(!result))
    result = read_json_object(parsed_data, pool, &parsed_data->container.value_object, depth+1);
   break;
   default:
    if(MJS_IsDigit(*parsed_data->current)) {
     /* might be int or float */
     result = MJS_ParseNumber(parsed_data, &parsed_data->container);
    } else
     result = (!result && !MJS_IsWhiteSpace(*parsed_data->current) ? MJS_RESULT_UNEXPECTED_TOKEN : result);
   break;
  }
  parsed_data->current++;
 }
   
 return result;
}


MJS_HOT static int read_json_object(MJSParsedData *parsed_data, MJSStringPool *pool, MJSObject *container, unsigned int depth) {
 int result = 0;

 unsigned int pool_index_key = 0;
 unsigned int pool_str_size = 0;
 unsigned short pool_chunk_index = 0;
 
 signed char flags = _EXPECTED_FOR_NAME | _IS_EMPTY;


 /* without this, it might cause stack overflow */
 if(MJS_Unlikely(depth >= MJS_MAX_NESTED_VALUE))
  return MJS_RESULT_REACHED_MAX_NESTED_DEPTH;

 while(parsed_data->current < parsed_data->end && !result) {

#if defined(MJS_NEON)
  Neon_read_json_object(parsed_data);
#endif
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case '\"': /* expected */
   
    result = !(flags & _EXPECTED_FOR_NAME) * MJS_RESULT_SYNTAX_ERROR;
    parsed_data->current++;
    pool_chunk_index = MJSStringPool_GetCurrentNode_IMPL(pool);
    result = result ? result : ((pool_chunk_index == 0xFFFF) * MJS_RESULT_ALLOCATION_FAILED);
    if(MJS_Likely(!result))
    result = MJS_ParseStringToPool(parsed_data, &pool->root[pool_chunk_index], &pool_index_key, &pool_str_size);
    flags = _EXPECTED_FOR_VALUE;
    
   break;
   case ':':
    
    result = !(flags & _EXPECTED_FOR_VALUE) * MJS_RESULT_SYNTAX_ERROR;
    parsed_data->current++;
    result = result ? result : read_json_object_value(parsed_data, pool, container, pool_index_key, pool_str_size, pool_chunk_index, depth);
    flags = _HAS_VALUE;
    
   break;
   case ',':
   
    result = !(flags & _HAS_VALUE) * MJS_RESULT_SYNTAX_ERROR;
    flags = _EXPECTED_FOR_NAME;
    
   break;
   case '}':
    return !((flags & _HAS_VALUE) || (flags & _IS_EMPTY)) * MJS_RESULT_UNEXPECTED_TOKEN;
   break;
   default:
    result = !MJS_IsWhiteSpace(*parsed_data->current) * MJS_RESULT_UNEXPECTED_TOKEN;
   break;
  }
  parsed_data->current++;
 }
  
 return 0;
}


MJS_HOT static int read_json_object_value(MJSParsedData *parsed_data, MJSStringPool *pool, MJSObject *container, unsigned int pool_index, unsigned int str_size, unsigned short chunk_index, unsigned int depth) {
 MJSDynamicType dynamic_type;
 int result = 0;
 
 while(parsed_data->current < parsed_data->end && !result) {
#if defined(MJS_NEON)
  Neon_read_json_object_value(parsed_data);
#endif
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case 't': /* might be true*/
    result = fast_memcmp_3(++parsed_data->current, _TRUE) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 2;
    dynamic_type.type = MJS_TYPE_BOOLEAN;
    dynamic_type.value_boolean.value = 1;
    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool, pool_index, str_size, chunk_index, &dynamic_type);
    return result;
   break;
   case 'f': /* might be false */
    result = fast_memcmp_4(++parsed_data->current, _FALSE) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 3;
    dynamic_type.type = MJS_TYPE_BOOLEAN;
    dynamic_type.value_boolean.value = 0;
    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool, pool_index, str_size, chunk_index, &dynamic_type);
    return result;
   break;
   case 'n': /* might be null */
    result = fast_memcmp_3(++parsed_data->current, _NULL) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 2;
    dynamic_type.type = MJS_TYPE_NULL;
    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool, pool_index, str_size, chunk_index, &dynamic_type);
    return result;
   break;
   case '\"': /* string */
    parsed_data->current++;
    dynamic_type.type = MJS_TYPE_STRING;
    dynamic_type.value_string.chunk_index = MJSStringPool_GetCurrentNode(pool);
    result = result ? result : ((dynamic_type.value_string.chunk_index == 0xFFFF) * MJS_RESULT_ALLOCATION_FAILED);
    if(MJS_Likely(!result))
    result = MJS_ParseStringToPool(parsed_data, &pool->root[dynamic_type.value_string.chunk_index], &dynamic_type.value_string.pool_index, &dynamic_type.value_string.str_size);
    result = result ? result :  MJSObject_InsertFromPool_IMPL(container, pool, pool_index, str_size, chunk_index, &dynamic_type);
    return result;
   break;
   case '-': /* might be int or float */
   case '+': /* might be int or float */
    result = MJS_ParseNumber(parsed_data, &dynamic_type);
    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool, pool_index, str_size, chunk_index, &dynamic_type);
    return result;
   break;
   case '[': /* an array */
    parsed_data->current++;
    result = MJSArray_Init_IMPL(&dynamic_type.value_array);
    if(MJS_Likely(!result)) 
    result = read_json_array_value(parsed_data, pool, &dynamic_type.value_array, depth);
    if(MJS_Likely(!result))
    result = MJSObject_InsertFromPool_IMPL(container, pool, pool_index, str_size, chunk_index, &dynamic_type);
    return result;
   break;
   case '{': /* an object */
    parsed_data->current++;
    result = MJSObject_Init_IMPL(&dynamic_type.value_object);
    if(MJS_Likely(!result))
    result = read_json_object(parsed_data, pool, &dynamic_type.value_object, depth+1);
    if(MJS_Likely(!result))
    result = MJSObject_InsertFromPool_IMPL(container, pool, pool_index, str_size, chunk_index, &dynamic_type);
    return result;
   break;
   default:
    if(MJS_IsDigit(*parsed_data->current)) {
     /* might be int or float */
     result = MJS_ParseNumber(parsed_data, &dynamic_type);
     result = result ? result :  MJSObject_InsertFromPool_IMPL(container, pool, pool_index, str_size, chunk_index, &dynamic_type);
     return result;
    } else
     result = !MJS_IsWhiteSpace(*parsed_data->current) * MJS_RESULT_UNEXPECTED_TOKEN;
   break;
  }
  parsed_data->current++;
 }
   
 return MJS_RESULT_SYNTAX_ERROR;/* no value */
}


MJS_HOT static int read_json_array_value(MJSParsedData *parsed_data, MJSStringPool *pool, MJSArray *arr, unsigned int depth) {
 MJSDynamicType dynamic_type;
 int result = 0;
 signed char flags = _EXPECTED_FOR_VALUE | _IS_EMPTY; 
 
 while(parsed_data->current < parsed_data->end && !result) {
#if defined(MJS_NEON)
  Neon_read_json_array_value(parsed_data);
#endif
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case 'n':
   
    result = !(flags & _EXPECTED_FOR_VALUE) * MJS_RESULT_UNEXPECTED_TOKEN;
    result = result ? result : fast_memcmp_3(++parsed_data->current, _NULL) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 2;
    dynamic_type.type = MJS_TYPE_NULL;
    result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
    flags = _HAS_VALUE;

   break;
   case 't': /* might be true*/
   
    result = !(flags & _EXPECTED_FOR_VALUE) * MJS_RESULT_UNEXPECTED_TOKEN;
    result = result ? result : fast_memcmp_3(++parsed_data->current, _TRUE) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 2;
    dynamic_type.type = MJS_TYPE_BOOLEAN;
    dynamic_type.value_boolean.value = 1;
    result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
    flags = _HAS_VALUE;

   break;
   case 'f': /* might be false */
   
    result = !(flags & _EXPECTED_FOR_VALUE) * MJS_RESULT_UNEXPECTED_TOKEN;
    result = result ? : fast_memcmp_4(++parsed_data->current, _FALSE) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 3;
    dynamic_type.type = MJS_TYPE_BOOLEAN;
    dynamic_type.value_boolean.value = 0;
    result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
    flags = _HAS_VALUE;

   break;
   case '\"':
    
    result = !(flags & _EXPECTED_FOR_VALUE) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current++;
    dynamic_type.type = MJS_TYPE_STRING;
    dynamic_type.value_string.chunk_index = MJSStringPool_GetCurrentNode(pool);
    result = result ? result : ((dynamic_type.value_string.chunk_index == 0xFFFF) * MJS_RESULT_ALLOCATION_FAILED);
    if(MJS_Likely(!result))
    result = MJS_ParseStringToPool(parsed_data, &pool->root[dynamic_type.value_string.chunk_index], &dynamic_type.value_string.pool_index, &dynamic_type.value_string.str_size);
    result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
    flags = _HAS_VALUE;
    
   break;
   case '-':
   case '+':
   
    result = !(flags & _EXPECTED_FOR_VALUE) * MJS_RESULT_UNEXPECTED_TOKEN;
    result = result ? result : MJS_ParseNumber(parsed_data, &dynamic_type);
    result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
    flags = _HAS_VALUE;
 
   break;
   case '{':
   
    result = !(flags & _EXPECTED_FOR_VALUE) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current++;
    result = result ? result : MJSObject_Init_IMPL(&dynamic_type.value_object);
    if(MJS_Likely(!result))
    result = read_json_object(parsed_data, pool, &dynamic_type.value_object, depth+1);
    if(MJS_Likely(!result))
    result = MJSArray_Add_IMPL(arr, &dynamic_type);
    flags = _HAS_VALUE;

   break;
   case '[':
    
    result = !(flags & _EXPECTED_FOR_VALUE) * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current++;    
    result = result ? result : MJSArray_Init_IMPL(&dynamic_type.value_array);
    if(MJS_Likely(!result))
    result = read_json_array_value(parsed_data, pool, &dynamic_type.value_array, depth);
    if(MJS_Likely(!result))
    result = MJSArray_Add_IMPL(arr, &dynamic_type);
    flags = _HAS_VALUE;
    
   break;
   case ']':
    /* excess comma */
    return ((flags & _EXPECTED_FOR_VALUE) && !(flags & _IS_EMPTY)) * MJS_RESULT_UNEXPECTED_TOKEN;
   break;
   case ',':
   
    result = (!(flags & _HAS_VALUE)) * MJS_RESULT_UNEXPECTED_TOKEN;
    flags = _EXPECTED_FOR_VALUE;
   break;
   default:
    if(MJS_IsDigit(*parsed_data->current)) {
     result = !(flags & _EXPECTED_FOR_VALUE) * MJS_RESULT_UNEXPECTED_TOKEN;
     result = result ? result : MJS_ParseNumber(parsed_data, &dynamic_type);
     result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
     flags = _HAS_VALUE;
     break;
    } else
     result = !MJS_IsWhiteSpace(*parsed_data->current) * MJS_RESULT_UNEXPECTED_TOKEN;
   break;
  }
  parsed_data->current++;
 }
 /* unclosed array */
 return MJS_RESULT_UNEXPECTED_TOKEN;
}















