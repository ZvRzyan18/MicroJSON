#include "micro_json/token.h"
#include "micro_json/parser.h"
#include <string.h>

#if (defined(__ARM_NEON) || defined(__ARM_NEON__)) && defined(MJS_FORCE_VECTORIZE)
#include "micro_json/token_neon.h"
#endif



MJS_COLD static MJSTokenResult tokenize_1(MJSParsedData *parsed_data);
MJS_HOT static int read_json_object(MJSParsedData *parsed_data, MJSObject *container, unsigned int depth);
MJS_HOT static int read_json_object_value(MJSParsedData *parsed_data, MJSObject *container, unsigned int pool_index, unsigned int str_size, unsigned int depth);
MJS_HOT static int read_json_object_first_value(MJSParsedData *parsed_data, MJSObject *container, unsigned int pool_index, unsigned int str_size, unsigned int depth);
MJS_HOT static int read_json_array_value(MJSParsedData *parsed_data, MJSObject *container, MJSArray *arr, unsigned int depth);



MJS_COLD int MJS_InitToken(MJSParsedData *parsed_data, const char *str, unsigned int len) { 
 if(!parsed_data || !str || !len)
  return MJS_RESULT_NULL_POINTER;
 parsed_data->current = str;
 parsed_data->begin = str;
 parsed_data->end = str+len;
 return 0;
}



MJS_COLD MJSTokenResult MJS_StartToken(MJSParsedData *parsed_data) {
 MJSTokenResult result;
 result.line = 0xFFFFFFFF;
 result.code = MJS_RESULT_NO_ERROR;

 if(!parsed_data) {
  result.code = MJS_RESULT_NULL_POINTER;
  return result;
 }
 
 return tokenize_1(parsed_data);
}

/*-----------------Static func-------------------*/


/* TODO : implement fast skipping whitespace */
MJS_COLD static MJSTokenResult tokenize_1(MJSParsedData *parsed_data) {
 MJSTokenResult result;
 result.line = 0xFFFFFFFF;
 result.code = MJS_RESULT_NO_ERROR;
 
 /* default loop fallback */

 MJSObject_Init(&parsed_data->container); 
 unsigned int pool_index_key = MJSObject_AddToStringPool(&parsed_data->container, "root", 4);
 if(pool_index_key == 0xFFFFFFFF)
  result.code = MJS_RESULT_ALLOCATION_FAILED;

 if(MJS_Unlikely(result.code)) {
  result.line = parsed_data->cl;
  return result;
 }
 result.code = read_json_object_first_value(parsed_data, &parsed_data->container, pool_index_key, 4, 0);
 if(MJS_Unlikely(result.code)) {
  result.line = parsed_data->cl;
  return result;
 }
 
 if(MJS_Unlikely(parsed_data->cb || parsed_data->sb)) {
  result.code = MJS_RESULT_INCOMPLETE_BRACKETS;
  result.line = parsed_data->cl;
  return result;
 }
 
 if(MJS_Unlikely(parsed_data->dq)) {
  result.code = MJS_RESULT_INCOMPLETE_DOUBLE_QUOTES;
  result.line = parsed_data->cl;
  return result;
 }
 return result;
}



MJS_HOT static int read_json_object_first_value(MJSParsedData *parsed_data, MJSObject *container, unsigned int pool_index, unsigned int str_size, unsigned int depth) {
 MJSDynamicType dynamic_type;
 unsigned int pool_index_key = 0;
 int result = 0;
 unsigned char number_type = 0;
 
 while(MJS_Likely(parsed_data->current < parsed_data->end)) {
#if defined(MJS_NEON)
  Neon_read_json_object_value(parsed_data);
#endif
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case 't': /* might be true*/
    if(memcmp(parsed_data->current, "true", 4) == 0) {
     parsed_data->current += 4;
     dynamic_type.type = MJS_TYPE_BOOLEAN;
     dynamic_type.value_boolean.value = 1;
     result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
    } else 
     return MJS_RESULT_UNEXPECTED_TOKEN;
   break;
   case 'f': /* might be false */
    if(memcmp(parsed_data->current, "false", 5) == 0) {
     parsed_data->current += 5;
     dynamic_type.type = MJS_TYPE_BOOLEAN;
     dynamic_type.value_boolean.value = 0;
     result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
    } else 
     return MJS_RESULT_UNEXPECTED_TOKEN;
   break;
   case 'n': /* might be null */
    if(memcmp(parsed_data->current, "null", 4) == 0) {
     parsed_data->current += 4;
     dynamic_type.type = MJS_TYPE_NULL;
     result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
    } else
     return MJS_RESULT_UNEXPECTED_TOKEN;
   break;
   case '\"': /* string */
    parsed_data->dq++;
    parsed_data->current++;
    result = MJS_ParseStringToCache(parsed_data);
    if(MJS_Unlikely(result)) return result;
    pool_index_key = MJSObject_AddToStringPool(container, (char*)parsed_data->cache, parsed_data->cache_size);
    if(pool_index_key == 0xFFFFFFFF)
     return MJS_RESULT_ALLOCATION_FAILED;
    dynamic_type.type = MJS_TYPE_STRING;
    dynamic_type.value_string.pool_index = pool_index_key;
    dynamic_type.value_string.str_size = parsed_data->cache_size;
    result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
    parsed_data->current++;
   break;
   case '-': /* might be int or float */
   case '+': /* might be int or float */
    number_type = MJS_ParseNumberToCache(parsed_data);
    if(MJS_Unlikely(!number_type))
     return MJS_RESULT_INVALID_TYPE;
    dynamic_type.type = number_type;
    switch(dynamic_type.type) {
     case MJS_TYPE_NUMBER_INT:
      dynamic_type.value_int.value = *(int*)parsed_data->cache;
     break;
     case MJS_TYPE_NUMBER_FLOAT:
      dynamic_type.value_float.value = *(float*)parsed_data->cache;
     break;
     default:
      return MJS_RESULT_INVALID_TYPE;
     break;
    }
    result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
   break;
   case '[': /* an array */
    parsed_data->sb++;
    parsed_data->current++;
    result = MJSArray_Init(&dynamic_type.value_array);
    if(MJS_Unlikely(result)) return result;
    result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
    result = read_json_array_value(parsed_data, container, &MJSObject_GetPairReferenceFromPool(container, pool_index, str_size)->value.value_array, depth);
    if(MJS_Unlikely(result))
     return result;
   break;
   case '{': /* an object */
    parsed_data->cb++;
    parsed_data->current++;
    result = MJSObject_Init(&dynamic_type.value_object);
    if(MJS_Unlikely(result)) return result;
    result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
     result = read_json_object(parsed_data, &MJSObject_GetPairReferenceFromPool(container, pool_index, str_size)->value.value_object, depth+1);
    if(MJS_Unlikely(result))
     return result;
   break;
   case '}':
    parsed_data->cb--;
   break;
   default:
    if(MJS_IsDigit(*parsed_data->current)) {
     /* might be int or float */
     number_type = MJS_ParseNumberToCache(parsed_data);
     if(MJS_Unlikely(!number_type))
      return MJS_RESULT_INVALID_TYPE;
     dynamic_type.type = number_type;
     switch(dynamic_type.type) {
      case MJS_TYPE_NUMBER_INT:
       dynamic_type.value_int.value = *(int*)parsed_data->cache;
      break;
      case MJS_TYPE_NUMBER_FLOAT:
       dynamic_type.value_float.value = *(float*)parsed_data->cache;
      break;
      default:
       return MJS_RESULT_INVALID_TYPE;
      break;
     }
     result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
    }
    if(MJS_Unlikely(!MJS_IsWhiteSpace(*parsed_data->current))) { /* invalid character */
     return MJS_RESULT_UNEXPECTED_TOKEN;
    }
   break;
  }
  parsed_data->current++;
 }
   
 return 0;
}


MJS_HOT static int read_json_object(MJSParsedData *parsed_data, MJSObject *container, unsigned int depth) {
 int result = 0;
 int has_name = 0;
 int has_value = 0;
 unsigned int pool_index_key = 0;
 
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
     parsed_data->dq++;
     parsed_data->current++;
     result = MJS_ParseStringToCache(parsed_data);
      if(MJS_Unlikely(result)) return result;
     pool_index_key = MJSObject_AddToStringPool(container, (char*)parsed_data->cache, parsed_data->cache_size);
     if(MJS_Unlikely(pool_index_key == 0xFFFFFFFF))
      return MJS_RESULT_ALLOCATION_FAILED;
     has_name = 1;
    } else
     return MJS_RESULT_SYNTAX_ERROR;
   break;
   case ':':
    if(has_name && !has_value) { 
     parsed_data->current++;
     result = read_json_object_value(parsed_data, container, pool_index_key, parsed_data->cache_size, depth);
     if(MJS_Unlikely(result)) return result;
     has_value = 1; 
     parsed_data->current--; 
    } else
     return MJS_RESULT_SYNTAX_ERROR;
   break;
   case ',':
    if(MJS_Unlikely(!has_name || !has_value))  \
     return MJS_RESULT_SYNTAX_ERROR; \
    has_name = 0;
    has_value = 0;
   break;
   case '}':
    parsed_data->cb--;
    parsed_data->current++;
    return 0;
   break;
   default:
   if(MJS_Unlikely(!MJS_IsWhiteSpace(*parsed_data->current))) {
    return MJS_RESULT_UNEXPECTED_TOKEN;
   }
   break;
  }
  parsed_data->current++;
 }
 
 if(MJS_Unlikely(!has_name || !has_value)) 
  return MJS_RESULT_SYNTAX_ERROR;
  
 return 0;
}






MJS_HOT static int read_json_object_value(MJSParsedData *parsed_data, MJSObject *container, unsigned int pool_index, unsigned int str_size, unsigned int depth) {
 MJSDynamicType dynamic_type;
 unsigned int pool_index_key = 0;
 int has_value = 0;
 int result = 0;
 unsigned char number_type = 0;
 
 while(MJS_Likely(parsed_data->current < parsed_data->end)) {
#if defined(MJS_NEON)
  Neon_read_json_object_value(parsed_data);
#endif
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case 't': /* might be true*/
    if(memcmp(parsed_data->current, "true", 4) == 0) {
     parsed_data->current += 4;
     dynamic_type.type = MJS_TYPE_BOOLEAN;
     dynamic_type.value_boolean.value = 1;
     result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
     return 0;
    } else 
     return MJS_RESULT_UNEXPECTED_TOKEN;
    has_value = 1;
    return 0;
   break;
   case 'f': /* might be false */
    if(memcmp(parsed_data->current, "false", 5) == 0) {
     parsed_data->current += 5;
     dynamic_type.type = MJS_TYPE_BOOLEAN;
     dynamic_type.value_boolean.value = 0;
     result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
     return 0;
    } else 
     return MJS_RESULT_UNEXPECTED_TOKEN;
    has_value = 1;
    return 0;
   break;
   case 'n': /* might be null */
    if(memcmp(parsed_data->current, "null", 4) == 0) {
     parsed_data->current += 4;
     dynamic_type.type = MJS_TYPE_NULL;
     result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
     return 0;
    } else
     return MJS_RESULT_UNEXPECTED_TOKEN;
    has_value = 1;
    return 0;
   break;
   case '\"': /* string */
    parsed_data->dq++;
    parsed_data->current++;
    result = MJS_ParseStringToCache(parsed_data);
    if(MJS_Unlikely(result)) return result;
    pool_index_key = MJSObject_AddToStringPool(container, (char*)parsed_data->cache, parsed_data->cache_size);
    if(MJS_Unlikely(pool_index_key == 0xFFFFFFFF))
     return MJS_RESULT_ALLOCATION_FAILED;
    dynamic_type.type = MJS_TYPE_STRING;
    dynamic_type.value_string.pool_index = pool_index_key;
    dynamic_type.value_string.str_size = parsed_data->cache_size;
    result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
    has_value = 1;
    parsed_data->current++;
    return 0;
   break;
   case '-': /* might be int or float */
   case '+': /* might be int or float */
    number_type = MJS_ParseNumberToCache(parsed_data);
    if(MJS_Unlikely(!number_type))
     return MJS_RESULT_INVALID_TYPE;
    dynamic_type.type = number_type;
    switch(dynamic_type.type) {
     case MJS_TYPE_NUMBER_INT:
      dynamic_type.value_int.value = *(int*)parsed_data->cache;
     break;
     case MJS_TYPE_NUMBER_FLOAT:
      dynamic_type.value_float.value = *(float*)parsed_data->cache;
     break;
     default:
      return MJS_RESULT_INVALID_TYPE;
     break;
    }
    result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
    has_value = 1;
    return 0;
   break;
   case '[': /* an array */
    parsed_data->sb++;
    parsed_data->current++;
    result = MJSArray_Init(&dynamic_type.value_array);
    if(MJS_Unlikely(result)) return result;
    result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
    result = read_json_array_value(parsed_data, container, &MJSObject_GetPairReferenceFromPool(container, pool_index, str_size)->value.value_array, depth);
    if(MJS_Unlikely(result))
     return result;
    has_value = 1;
    return 0;
   break;
   case '{': /* an object */
    parsed_data->cb++;
    parsed_data->current++;
    result = MJSObject_Init(&dynamic_type.value_object);
    if(MJS_Unlikely(result)) return result;
    result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
     result = read_json_object(parsed_data, &MJSObject_GetPairReferenceFromPool(container, pool_index, str_size)->value.value_object, depth+1);
    if(MJS_Unlikely(result))
     return result;
    has_value = 1;
    return 0;
   break;
   case '}':
    parsed_data->cb--;
    return 0;
   break;
   case ',':
    if(MJS_Unlikely(!has_value))
     return MJS_RESULT_SYNTAX_ERROR;
    parsed_data->current--;
    return 0;
   break;
   default:
    if(MJS_IsDigit(*parsed_data->current)) {
     /* might be int or float */
     number_type = MJS_ParseNumberToCache(parsed_data);
     if(MJS_Unlikely(!number_type))
      return MJS_RESULT_INVALID_TYPE;
     dynamic_type.type = number_type;
     switch(dynamic_type.type) {
      case MJS_TYPE_NUMBER_INT:
       dynamic_type.value_int.value = *(int*)parsed_data->cache;
      break;
      case MJS_TYPE_NUMBER_FLOAT:
       dynamic_type.value_float.value = *(float*)parsed_data->cache;
      break;
      default:
       return MJS_RESULT_INVALID_TYPE;
      break;
     }
     result = MJSObject_InsertFromPool(container, pool_index, str_size, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
     has_value = 1;
     return 0;
    } else if(MJS_Unlikely(!MJS_IsWhiteSpace(*parsed_data->current))) { /* invalid character */
     return MJS_RESULT_UNEXPECTED_TOKEN;
    }
   break;
  }
  parsed_data->current++;
 }
   
 return 0;
}









MJS_HOT static int read_json_array_value(MJSParsedData *parsed_data, MJSObject *container, MJSArray *arr, unsigned int depth) {
 MJSDynamicType dynamic_type;
 unsigned int pool_index_key = 0;
 int has_value = 0;
 int result = 0;
 unsigned char number_type = 0;
 
 while(MJS_Likely(parsed_data->current < parsed_data->end)) {
  switch(*parsed_data->current) {
   case '\n':
    parsed_data->cl++;
   break;
   case 'n':

   if(memcmp(parsed_data->current, "null", 4) == 0) {
     parsed_data->current += 3;
     dynamic_type.type = MJS_TYPE_NULL;
     result = MJSArray_Add(arr, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
    } else 
     return MJS_RESULT_UNEXPECTED_TOKEN;
    has_value = 1;

   break;
   case 't': /* might be true*/

    if(memcmp(parsed_data->current, "true", 4) == 0) {
     parsed_data->current += 3;
     dynamic_type.type = MJS_TYPE_BOOLEAN;
     dynamic_type.value_boolean.value = 1;
     result = MJSArray_Add(arr, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
    } else 
     return MJS_RESULT_UNEXPECTED_TOKEN;
    has_value = 1;

   break;
   case 'f': /* might be false */

    if(memcmp(parsed_data->current, "false", 5) == 0) {
     parsed_data->current += 4;
     dynamic_type.type = MJS_TYPE_BOOLEAN;
     dynamic_type.value_boolean.value = 0;
     result = MJSArray_Add(arr, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
    } else 
     return MJS_RESULT_UNEXPECTED_TOKEN;
    has_value = 1;

   break;
   case '\"':

    parsed_data->dq++;
    parsed_data->current++;
    result = MJS_ParseStringToCache(parsed_data);
    if(MJS_Unlikely(result)) return result;
     
    pool_index_key = MJSObject_AddToStringPool(container, (char*)parsed_data->cache, parsed_data->cache_size);
    if(MJS_Unlikely(pool_index_key == 0xFFFFFFFF))
     return MJS_RESULT_ALLOCATION_FAILED;

    dynamic_type.type = MJS_TYPE_STRING;
    dynamic_type.value_string.pool_index = pool_index_key;
    dynamic_type.value_string.str_size = parsed_data->cache_size;
    
    result = MJSArray_Add(arr, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
    has_value = 1;
 
   break;
   case '-':
   case '+':

    number_type = MJS_ParseNumberToCache(parsed_data);
    if(MJS_Unlikely(!number_type))
     return MJS_RESULT_INVALID_TYPE;
     
    dynamic_type.type = number_type;
    switch(dynamic_type.type) {
     case MJS_TYPE_NUMBER_INT:
      dynamic_type.value_int.value = *(int*)parsed_data->cache;
     break;
     case MJS_TYPE_NUMBER_FLOAT:
      dynamic_type.value_float.value = *(float*)parsed_data->cache;
     break;
     default:
      return MJS_RESULT_INVALID_TYPE;
     break;
    }
    result = MJSArray_Add(arr, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
    has_value = 1;
    parsed_data->current--;
   break;
   case '{':

    parsed_data->cb++;
    
    parsed_data->current++;
    result = MJSObject_Init(&dynamic_type.value_object);
    if(MJS_Unlikely(result)) return result;
    
    result = MJSArray_Add(arr, &dynamic_type);
    if(MJS_Unlikely(result)) return result;
    result = read_json_object(parsed_data, &MJSArray_Get(arr, MJSArray_Size(arr)-1)->value_object, depth+1);
    if(MJS_Unlikely(result))
     return result;
    has_value = 1;

   break;
   case ']':
     parsed_data->current++;
     parsed_data->sb--;
     return 0;
   break;
   case ',':
    if(MJS_Likely(has_value))
     has_value = 0;
    else
     return MJS_RESULT_SYNTAX_ERROR;
   break;
   default:
    if(MJS_IsDigit(*parsed_data->current)) {

     number_type = MJS_ParseNumberToCache(parsed_data);
     if(MJS_Unlikely(!number_type))
      return MJS_RESULT_INVALID_TYPE;
      
     dynamic_type.type = number_type;
     switch(dynamic_type.type) {
      case MJS_TYPE_NUMBER_INT:
       dynamic_type.value_int.value = *(int*)parsed_data->cache;
      break;
      case MJS_TYPE_NUMBER_FLOAT:
       dynamic_type.value_float.value = *(float*)parsed_data->cache;
      break;
      default:
       return MJS_RESULT_INVALID_TYPE;
      break;
     }
     result = MJSArray_Add(arr, &dynamic_type);
     if(MJS_Unlikely(result)) return result;
     has_value = 1;
     parsed_data->current--;
     break;
    } else if(MJS_Unlikely(!MJS_IsWhiteSpace(*parsed_data->current))) { /* invalid character */
     return MJS_RESULT_UNEXPECTED_TOKEN;
    }
    
   break;
  }
  parsed_data->current++;
 }
 return 0;
}


