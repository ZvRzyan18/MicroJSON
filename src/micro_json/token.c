#include "micro_json/token.h"
#include "micro_json/parser.h"
#include <string.h>

static MJSTokenResult tokenize_1(MJSParsedData *parsed_data);
static int read_json_object(MJSParsedData *parsed_data, MJSContainer *container, int depth);
static int read_json_value(MJSParsedData *parsed_data, MJSContainer *container, unsigned int pool_index, unsigned int str_size, int depth);



MJSTokenResult MJS_StartToken(MJSParsedData *parsed_data, const char *str, unsigned int len) { 
 parsed_data->current = str;
 parsed_data->begin = str;
 parsed_data->end = str+len;
 return tokenize_1(parsed_data);
}

//-----------------Static func-------------------//

static MJSTokenResult tokenize_1(MJSParsedData *parsed_data) {
 MJSTokenResult result;
 result.line = 0xFFFFFFFF;
 result.code = MJS_RESULT_NO_ERROR;

 while(parsed_data->current < parsed_data->end) {
 
  switch(*parsed_data->current) {
   case '{':
    parsed_data->cb++;
    
    parsed_data->current++; //skip '{'
    result.code = read_json_object(parsed_data, &parsed_data->container, 0);
    if(result.code) {
     result.line = parsed_data->cl;
     return result;
    }
   break;
   case '\n':
    parsed_data->cl++;
   break;
   default:
    if(!MJS_IsWhiteSpace(*parsed_data->current)) {
     result.code = MJS_RESULT_UNEXPECTED_TOKEN;
     result.line = parsed_data->cl;
     return result;
    }
   break;
  }
  parsed_data->current++;
 }
 
 
 
 if(parsed_data->cb || parsed_data->sb) {
  result.code = MJS_RESULT_INCOMPLETE_BRACKETS;
  result.line = parsed_data->cl;
  return result;
 }
 
 if(parsed_data->dq) {
  result.code = MJS_RESULT_INCOMPLETE_DOUBLE_QUOTES;
  result.line = parsed_data->cl;
  return result;
 }
 return result;
}


static int read_json_object(MJSParsedData *parsed_data, MJSContainer *container, int depth) {
 MJSContainer_Init(container); 
 int result = 0;
 int has_name = 0;
 int has_value = 0;
 unsigned int pool_index_key = 0;
 
 while(parsed_data->current < parsed_data->end) {
  switch(*parsed_data->current) {
   case '\"': //expected
    parsed_data->dq++;
    parsed_data->current++;
    if(MJS_ParseStringToCache(parsed_data))
     return MJS_RESULT_SYNTAX_ERROR;
    pool_index_key = MJSContainer_AddToStringPool(container, (char*)parsed_data->cache, parsed_data->cache_size);
    if(pool_index_key == 0xFFFFFFFF)
     return MJS_RESULT_ALLOCATION_FAILED;
    has_name = 1;
   break;
   case ':':
    parsed_data->current++;
    result = read_json_value(parsed_data, container, pool_index_key, parsed_data->cache_size, depth);
    if(result)
     return result;
    has_value = 1;
   break;
   case ',':
    if(!has_name || !has_value) 
     return MJS_RESULT_SYNTAX_ERROR;
    has_name = 0;
    has_value = 0;
   break;
   default:
   if(!MJS_IsWhiteSpace(*parsed_data->current))
    return MJS_RESULT_UNEXPECTED_TOKEN;
  }
  parsed_data->current++;
 }
 
 if(!has_name || !has_value) 
  return MJS_RESULT_SYNTAX_ERROR;
  
 return 0;
}



static int read_json_value(MJSParsedData *parsed_data, MJSContainer *container, unsigned int pool_index, unsigned int str_size, int depth) {
 MJSDynamicType dynamic_type;
 unsigned int pool_index_key = 0;
 int has_value = 0;
 while(parsed_data->current < parsed_data->end) {
  
  switch(*parsed_data->current) {
   case 'n': //might be null
    if(strncmp(parsed_data->current, "null", 4) == 0) {
     parsed_data->current += 3;
     dynamic_type.type = MJS_TYPE_NULL;
     if(MJSContainer_InsertFromPool(container, pool_index, str_size, dynamic_type))
      return MJS_RESULT_ALLOCATION_FAILED;
     return 0;
    } else 
     return MJS_RESULT_UNEXPECTED_TOKEN;
    has_value = 1;
   break;
   case '\"': //string
    
    parsed_data->dq++;
    parsed_data->current++;
    if(MJS_ParseStringToCache(parsed_data))
     return MJS_RESULT_SYNTAX_ERROR;
     
    pool_index_key = MJSContainer_AddToStringPool(container, (char*)parsed_data->cache, parsed_data->cache_size);
    if(pool_index_key == 0xFFFFFFFF)
     return MJS_RESULT_ALLOCATION_FAILED;

    dynamic_type.type = MJS_TYPE_STRING;
    dynamic_type.value_string.pool_index = pool_index_key;
    dynamic_type.value_string.str_size = parsed_data->cache_size;
    
    if(MJSContainer_InsertFromPool(container, pool_index, str_size, dynamic_type))
     return MJS_RESULT_ALLOCATION_FAILED;
    has_value = 1;
   break;
   case '-': //might be int or float
   break;
   case '+': //might be int or float
   break;
   case '[': //an array
    parsed_data->sb++;
   break;
   case '{': //an object
   break;
   case '}':
    parsed_data->cb--;
    return 0;
   break;
   case ',':
    if(!has_value)
     return MJS_RESULT_SYNTAX_ERROR;
    parsed_data->current--;
    return 0;
   break;
   default:
    if(MJS_IsDigit(*parsed_data->current)) {
     //might be int or float
    }
    if(!MJS_IsWhiteSpace(*parsed_data->current)) {//invalid character
     return MJS_RESULT_UNEXPECTED_TOKEN;
    }
   break;
  }
  parsed_data->current++;
 }
 
 if(!has_value)
  return MJS_RESULT_SYNTAX_ERROR;
  
 return 0;
}




