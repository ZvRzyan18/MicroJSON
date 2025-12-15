#include "micro_json/token.h"
#include "micro_json/parser.h"
#include "micro_json/object_impl.h"
#include <string.h>

#if (defined(__ARM_NEON) || defined(__ARM_NEON__)) && defined(MJS_FORCE_VECTORIZE)
#include "micro_json/token_neon.h"
#endif

MJS_HOT static int read_json_object_first_value(MJSParsedData *parsed_data, MJSObject *container, unsigned int pool_index, unsigned int str_size, unsigned int depth);
MJS_HOT static int read_json_object(MJSParsedData *parsed_data, MJSObject *container, unsigned int depth);
MJS_HOT static int read_json_object_value(MJSParsedData *parsed_data, MJSObject *container, unsigned int pool_index, unsigned int str_size, unsigned int depth);
MJS_HOT static int read_json_array_value(MJSParsedData *parsed_data, MJSObject *container, MJSArray *arr, unsigned int depth);

MJS_INLINE int fast_memcmp_4(const char *a, const char *b) {
 return (*((unsigned int*)a)) != (*((unsigned int*)b));
}

MJS_INLINE int fast_memcmp_5(const char *a, const char *b) {
 return (*(unsigned int*)a) != (*(unsigned int*)b) | a[4] != b[4];
}


/*-----------------Token func-------------------*/


MJS_HOT MJSTokenResult MJS_TokenParse(MJSParsedData *parsed_data, const char *str, unsigned int len) { 

 MJSTokenResult result;
 result.line = 0xFFFFFFFF;
 result.code = MJS_RESULT_NO_ERROR;

 if(MJS_Unlikely(!parsed_data || !str || !len)) {
  result.code = MJS_RESULT_NULL_POINTER;
  return result;
 }
 
 parsed_data->current = str;
 parsed_data->end = str+len;

 result.code = MJSObject_Init(&parsed_data->container); 
 unsigned int pool_index_key = MJSObject_AddToStringPool_IMPL(&parsed_data->container, "root", 4);
 result.code = ((!result.code && pool_index_key == 0xFFFFFFFF) ? MJS_RESULT_ALLOCATION_FAILED : result.code);

 result.code = result.code ? result.code :  read_json_object_first_value(parsed_data, &parsed_data->container, pool_index_key, 4, 0);
 result.line = parsed_data->cl;
 
 return result;
}


/*-----------------Static func-------------------*/


MJS_HOT static int read_json_object_first_value(MJSParsedData *parsed_data, MJSObject *container, unsigned int pool_index, unsigned int str_size, unsigned int depth) {
 MJSDynamicType dynamic_type;
 unsigned int pool_index_key = 0;
 unsigned int pool_str_size = 0;
 int result = 0;
 /*unsigned char number_type = 0;*/
 
 while(parsed_data->current < parsed_data->end) {
#if defined(MJS_NEON)
  Neon_read_json_object_value(parsed_data);
#endif
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case 't': /* might be true*/
    result = fast_memcmp_4(parsed_data->current, "true") * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 3;
    dynamic_type.type = MJS_TYPE_BOOLEAN;
    dynamic_type.value_boolean.value = 1;
    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
   break;
   case 'f': /* might be false */
    result = fast_memcmp_5(parsed_data->current, "false") * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 4;
    dynamic_type.type = MJS_TYPE_BOOLEAN;
    dynamic_type.value_boolean.value = 0;
    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
   break;
   case 'n': /* might be null */
    result = fast_memcmp_4(parsed_data->current, "null") * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 3;
    dynamic_type.type = MJS_TYPE_NULL;
    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
   break;
   case '\"': /* string */
    parsed_data->current++;
    /*
    result = MJS_ParseStringToCache(parsed_data);
    pool_index_key = MJSObject_AddToStringPool_IMPL(container, (char*)parsed_data->cache, parsed_data->cache_size);
    result = ((!result && pool_index_key != 0xFFFFFFFF) ? result : MJS_RESULT_ALLOCATION_FAILED);
    */
    result = MJS_ParseStringToPool(parsed_data, container, &pool_index_key, &pool_str_size);
    dynamic_type.type = MJS_TYPE_STRING;
    dynamic_type.value_string.pool_index = pool_index_key;
    dynamic_type.value_string.str_size = pool_str_size;
    if(MJS_Likely(!result))
    result = MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
   break;
   case '-': /* might be int or float */
   case '+': /* might be int or float */
    result = MJS_ParseNumber(parsed_data, &dynamic_type);

    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
   break;
   case '[': /* an array */
    parsed_data->current++;
    result = MJSArray_Init_IMPL(&dynamic_type.value_array);
    if(MJS_Likely(!result))
    result = read_json_array_value(parsed_data, container, &dynamic_type.value_array, depth);
    if(MJS_Likely(!result))
    result = MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);

   break;
   case '{': /* an object */
    parsed_data->current++;
    result = MJSObject_Init_IMPL(&dynamic_type.value_object);
    if(MJS_Likely(!result))
    result = read_json_object(parsed_data, &dynamic_type.value_object, depth+1);
    if(MJS_Likely(!result))
    result = MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
   break;
   default:
    if(MJS_IsDigit(*parsed_data->current)) {
     /* might be int or float */
     result = MJS_ParseNumber(parsed_data, &dynamic_type);

     result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
    }
    result = (!result && !MJS_IsWhiteSpace(*parsed_data->current) ? MJS_RESULT_UNEXPECTED_TOKEN : result);
   break;
  }
  if(MJS_Unlikely(result))
   return result;
  parsed_data->current++;
 }
   
 return result;
}


MJS_HOT static int read_json_object(MJSParsedData *parsed_data, MJSObject *container, unsigned int depth) {
 int result = 0;
 int has_name = 0;
 int has_value = 0;
 unsigned int pool_index_key = 0;
 unsigned int pool_str_size = 0;
 /* without this, it might cause stack overflow */
 if(MJS_Unlikely(depth >= MJS_MAX_NESTED_VALUE))
  return MJS_RESULT_REACHED_MAX_NESTED_DEPTH;

 while(parsed_data->current < parsed_data->end) {

#if defined(MJS_NEON)
  Neon_read_json_object(parsed_data);
#endif
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case '\"': /* expected */
    if(!has_name && !has_value) {
     parsed_data->current++;
     result = MJS_ParseStringToPool(parsed_data, container, &pool_index_key, &pool_str_size);
     /*
     result = MJS_ParseStringToCache(parsed_data);
     if(MJS_Likely(!result))
     pool_index_key = MJSObject_AddToStringPool_IMPL(container, (char*)parsed_data->cache, parsed_data->cache_size);
     result = !result && pool_index_key == 0xFFFFFFFF ?  MJS_RESULT_ALLOCATION_FAILED : result;
     */
     has_name = 1;
    } else
     result = MJS_RESULT_SYNTAX_ERROR;
   break;
   case ':':
    if(has_name && !has_value) { 
     parsed_data->current++;
     result = read_json_object_value(parsed_data, container, pool_index_key, pool_str_size, depth);
     has_value = 1; 
    } else
     result = MJS_RESULT_SYNTAX_ERROR;
   break;
   case ',':
    result = (!has_name || !has_value) * MJS_RESULT_SYNTAX_ERROR;
    has_name = 0;
    has_value = 0;
   break;
   case '}':
    return 0;
   break;
   default:
    result = !MJS_IsWhiteSpace(*parsed_data->current) * MJS_RESULT_UNEXPECTED_TOKEN;
   break;
  }
  if(MJS_Unlikely(result))
   return result;
  parsed_data->current++;
 }
  
 return 0;
}


MJS_HOT static int read_json_object_value(MJSParsedData *parsed_data, MJSObject *container, unsigned int pool_index, unsigned int str_size, unsigned int depth) {
 MJSDynamicType dynamic_type;
 unsigned int pool_index_key = 0;
 unsigned int pool_str_size = 0;
 int result = 0;
 /*unsigned char number_type = 0;*/
 
 while(MJS_Likely(parsed_data->current < parsed_data->end)) {
#if defined(MJS_NEON)
  Neon_read_json_object_value(parsed_data);
#endif
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case 't': /* might be true*/
    result = fast_memcmp_4(parsed_data->current, "true") * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 3;
    dynamic_type.type = MJS_TYPE_BOOLEAN;
    dynamic_type.value_boolean.value = 1;
    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
    return result;
   break;
   case 'f': /* might be false */
    result = fast_memcmp_5(parsed_data->current, "false") * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 4;
    dynamic_type.type = MJS_TYPE_BOOLEAN;
    dynamic_type.value_boolean.value = 0;
    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
    return result;
   break;
   case 'n': /* might be null */
    result = fast_memcmp_4(parsed_data->current, "null") * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 3;
    dynamic_type.type = MJS_TYPE_NULL;
    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
    return result;
   break;
   case '\"': /* string */
    parsed_data->current++;
    /*
    result = MJS_ParseStringToCache(parsed_data);
    if(MJS_Likely(!result))
    pool_index_key = MJSObject_AddToStringPool_IMPL(container, (char*)parsed_data->cache, parsed_data->cache_size);
    result = (!result && pool_index_key == 0xFFFFFFFF) ? MJS_RESULT_ALLOCATION_FAILED : result;
    */
    result = MJS_ParseStringToPool(parsed_data, container, &pool_index_key, &pool_str_size);

    dynamic_type.type = MJS_TYPE_STRING;
    dynamic_type.value_string.pool_index = pool_index_key;
    dynamic_type.value_string.str_size = pool_str_size;
    result = result ? result :  MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
    return result;
   break;
   case '-': /* might be int or float */
   case '+': /* might be int or float */
    result = MJS_ParseNumber(parsed_data, &dynamic_type);

    result = result ? result : MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
    return result;
   break;
   case '[': /* an array */
    parsed_data->current++;
    result = MJSArray_Init_IMPL(&dynamic_type.value_array);
    if(MJS_Likely(!result)) 
    result = read_json_array_value(parsed_data, container, &dynamic_type.value_array, depth);
    if(MJS_Likely(!result))
    result = MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
    return result;
   break;
   case '{': /* an object */
    parsed_data->current++;
    result = MJSObject_Init_IMPL(&dynamic_type.value_object);
    if(MJS_Likely(!result))
    result = read_json_object(parsed_data, &dynamic_type.value_object, depth+1);
    if(MJS_Likely(!result))
    result = MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
    return result;
   break;
   default:
    if(MJS_IsDigit(*parsed_data->current)) {
     /* might be int or float */
     result = MJS_ParseNumber(parsed_data, &dynamic_type);
 
     result = result ? result :  MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, &dynamic_type);
     return result;
    } else
     result = !MJS_IsWhiteSpace(*parsed_data->current) * MJS_RESULT_UNEXPECTED_TOKEN;
   break;
  }
  if(MJS_Unlikely(result)) 
   return result;
  parsed_data->current++;
 }
   
 return 0;
}


MJS_HOT static int read_json_array_value(MJSParsedData *parsed_data, MJSObject *container, MJSArray *arr, unsigned int depth) {
 MJSDynamicType dynamic_type;
 unsigned int pool_index_key = 0;
 unsigned int pool_str_size = 0;
 int has_value = 0;
 int result = 0;
 /*unsigned char number_type = 0;*/
 
 while(MJS_Likely(parsed_data->current < parsed_data->end)) {
#if defined(MJS_NEON)
  Neon_read_json_array_value(parsed_data);
#endif
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case 'n':

    result = fast_memcmp_4(parsed_data->current, "null") * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 3;
    dynamic_type.type = MJS_TYPE_NULL;
    result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
    has_value = 1;

   break;
   case 't': /* might be true*/

    result = fast_memcmp_4(parsed_data->current, "true") * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 3;
    dynamic_type.type = MJS_TYPE_BOOLEAN;
    dynamic_type.value_boolean.value = 1;
    result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
    has_value = 1;

   break;
   case 'f': /* might be false */

    result = fast_memcmp_5(parsed_data->current, "false") * MJS_RESULT_UNEXPECTED_TOKEN;
    parsed_data->current += 4;
    dynamic_type.type = MJS_TYPE_BOOLEAN;
    dynamic_type.value_boolean.value = 0;
    result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
    has_value = 1;

   break;
   case '\"':

    parsed_data->current++;
    /*
    result = MJS_ParseStringToCache(parsed_data);
    pool_index_key = MJSObject_AddToStringPool_IMPL(container, (char*)parsed_data->cache, parsed_data->cache_size);
    result = result ? result : ((pool_index_key == 0xFFFFFFFF) * MJS_RESULT_ALLOCATION_FAILED);
    */
    result = MJS_ParseStringToPool(parsed_data, container, &pool_index_key, &pool_str_size);

    dynamic_type.type = MJS_TYPE_STRING;
    dynamic_type.value_string.pool_index = pool_index_key;
    dynamic_type.value_string.str_size = pool_str_size;
    
    result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
    has_value = 1;
    
   break;
   case '-':
   case '+':

    result = MJS_ParseNumber(parsed_data, &dynamic_type);

    result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
    has_value = 1;
 
   break;
   case '{':
    
    parsed_data->current++;
    result = MJSObject_Init_IMPL(&dynamic_type.value_object);
    if(MJS_Likely(!result))
    
    result = read_json_object(parsed_data, &dynamic_type.value_object, depth+1);
    if(MJS_Likely(!result))
    result = MJSArray_Add_IMPL(arr, &dynamic_type);
    has_value = 1;

   break;
   case '[':
   
    parsed_data->current++;
    result = MJSArray_Init_IMPL(&dynamic_type.value_array);
    if(MJS_Likely(!result))
    result = read_json_array_value(parsed_data, container, &dynamic_type.value_array, depth);
    if(MJS_Likely(!result))
    result = MJSArray_Add_IMPL(arr, &dynamic_type);
    has_value = 1;
    
   break;
   case ']':
    return 0;
   break;
   case ',':
    has_value = 0;
    result = has_value * MJS_RESULT_SYNTAX_ERROR;
   break;
   default:
    if(MJS_IsDigit(*parsed_data->current)) {

     result = MJS_ParseNumber(parsed_data, &dynamic_type);

     result = result ? result : MJSArray_Add_IMPL(arr, &dynamic_type);
     has_value = 1;
     break;
    } else
     result = !MJS_IsWhiteSpace(*parsed_data->current) * MJS_RESULT_UNEXPECTED_TOKEN;
    
   break;
  }
  if(MJS_Unlikely(result))
   return result;
  parsed_data->current++;
 }
 return result;
}















