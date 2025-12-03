#include "micro_json/parser.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#if (defined(__ARM_NEON) || defined(__ARM_NEON__)) && defined(MJS_FORCE_VECTORIZE)
#include "micro_json/parser_neon.h"
#endif

/*convert error code to string */
const char* MJS_CodeToString(signed char code) {
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
  case MJS_RESULT_INCOMPLETE_BRACKETS:
   return "MJS_RESULT_INCOMPLETE_BRACKETS";
  break;
  case MJS_RESULT_INCOMPLETE_DOUBLE_QUOTES:
   return "MJS_RESULT_INCOMPLETE_DOUBLE_QUOTES";
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
  case MJS_RESULT_DUPLICATE_KEY:
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
 }
 return "Unknown Error";
}


/*
 TODO : Optimize it for large strings
 minimize loop and switch branches.
*/
int MJS_ParseStringToCache(MJSParsedData *parsed_data) {
 parsed_data->cache_size = 0;
 int result;
 char *cache_str = (char*)parsed_data->cache;
 
 while(parsed_data->current < parsed_data->end) {

#if defined(MJS_NEON)
  if((parsed_data->cache_size+16+5) > parsed_data->cache_allocated_size) {
   result = MJSParserData_ExpandCache(parsed_data);
   if(result) return result;
  }
  Neon_ParseStringToCache(parsed_data);
#else
  if((parsed_data->cache_size+5) > parsed_data->cache_allocated_size) {
   result = MJSParserData_ExpandCache(parsed_data);
   if(result) return result;
  }
#endif

  switch(*parsed_data->current) {
   case '\\':
   
   if((parsed_data->current+1) < parsed_data->end) {
   switch(*(++parsed_data->current)) {
    case '\\':
     cache_str[parsed_data->cache_size++] = '\\';
     parsed_data->current++;
    break;
    case 'n':
     cache_str[parsed_data->cache_size++] = '\n';
     parsed_data->current++;
    break;
    case '\"':
     cache_str[parsed_data->cache_size++] = '\"';
     parsed_data->current++;
    break;
    case 'b':
     cache_str[parsed_data->cache_size++] = '\b';
     parsed_data->current++;
    break;
    case 'r':
     cache_str[parsed_data->cache_size++] = '\r';
     parsed_data->current++;
    break;
    case 't':
     cache_str[parsed_data->cache_size++] = '\t';
     parsed_data->current++;
    break;
    case 'u': /*unicode*/
    
     /* avoid overflow access  */
     if((parsed_data->current+4) >= parsed_data->end)
      return MJS_RESULT_INCOMPLETE_STRING_SYNTAX;
     
     parsed_data->current++;
     result = MJS_ReadUnicodeHexadecimal(parsed_data);
     if(result) return result;
    break;
    default:
     return MJS_RESULT_INVALID_ESCAPE_SEQUENCE;
    break;
   }
   }
   
   break;
   case '\"':
    parsed_data->dq--;
    goto __MJS_ParseStringToCache_finish;
   break;
   case '\n':
    parsed_data->cl++;
   break;
   default:
    cache_str[parsed_data->cache_size++] = *parsed_data->current;
   break;
  }
  
  parsed_data->current++;
 }
 __MJS_ParseStringToCache_finish:
 cache_str[parsed_data->cache_size] = '\0';
 return 0;
}

/*
 supported format
 • 12 (int)
 • -3 (int)
 • 3.45 (float)
 • 7e+2 (float)
 • 8.3e-1 (float)
*/

/*
 since number has only very few characters,
  it does not need a vectorized function.
*/
int MJS_ParseNumberToCache(MJSParsedData *parsed_data) {
 parsed_data->cache_size = 0;
 int is_float = 0;
 int is_negative = 0;
 int has_exponent = 0;
 int has_fractional = 0;
 int whole_part = 0;
 int fractional_part = 0;
 int fractional_part_count = 0;
 int exponent_part = 0;
 int is_exponent_negative = 0;
 
 while(parsed_data->current < parsed_data->end) {
  switch(*parsed_data->current) {
   case '-':
    if(has_exponent) 
     is_exponent_negative = 1;
    else
     is_negative = 1;
   break;
   case '+':
    if(has_exponent) 
     is_exponent_negative = 0;
    else
     is_negative = 0;
   break;
   case '.':
    if(has_exponent)
     return 0;
    is_float = 1;
    has_fractional = 1;
   break;
   case 'e':
    is_float = 1;
    has_exponent = 1;
   break;
   default:
   
    if(!MJS_IsDigit(*parsed_data->current)) {
     switch(*parsed_data->current) {
      case ',':
      case '}':
      case ']':
       goto __MJS_ParseNumberToCache_Output;
      break;
      default:
       goto __MJS_ParseNumberToCache_Output;
      break;
     }
    } else {
     if(has_exponent) {
      exponent_part = exponent_part * 10 + (int)(*parsed_data->current - '0');
     } else if(has_fractional) {
      fractional_part = fractional_part * 10 + (int)(*parsed_data->current - '0');
      fractional_part_count++;
     } else {
      whole_part = whole_part * 10 + (int)(*parsed_data->current - '0');
     }
    }
   break;
  }
  parsed_data->current++;
 }
 
 __MJS_ParseNumberToCache_Output:
 
 if(is_float) {
  float fractional = ((float)fractional_part) * (float)pow(10.0, (double)-fractional_part_count);
  float whole = (float)whole_part;
  float final_value = 0.0f;
  
  if(has_exponent && exponent_part > 0)
   final_value = (whole + fractional) * (float)pow(10.0, (double)(is_exponent_negative ? -exponent_part : exponent_part));
  else
   final_value = whole + fractional;
   
  final_value = is_negative ? -final_value : final_value;
  memcpy(parsed_data->cache, &final_value, sizeof(float));
  parsed_data->cache_size = (unsigned int)sizeof(float);
  return MJS_TYPE_NUMBER_FLOAT;
 } else {
  whole_part = is_negative ? -whole_part : whole_part;
  memcpy(parsed_data->cache, &whole_part, sizeof(int));
  parsed_data->cache_size = (unsigned int)sizeof(int);
  return MJS_TYPE_NUMBER_INT;
 }
 return 0;
}


int MJS_ReadUnicodeHexadecimal(MJSParsedData *parsed_data) {
 if((parsed_data->current+4) < parsed_data->end) {
  unsigned int i, value;
  value = 0;
  /* 
   json only requires 4 character hex.
   no more or less.
  */
  for (i = 0; i < 4; i++) {
   const char c = *(parsed_data->current++);
   value <<= 4;
   if(c >= '0' && c <= '9') value |= (c - '0');
   else if (c >= 'A' && c <= 'F') value |= (c - 'A' + 0xA);
   else if (c >= 'a' && c <= 'f') value |= (c - 'a' + 0xA);
   else return MJS_RESULT_INVALID_HEX_VALUE;
  }
  parsed_data->current--;
  parsed_data->cache_size += MJS_UnicodeToChar(value, (char*)parsed_data->cache, parsed_data->cache_size);
 }
 return 0;
}

/*
 convert unicode hex unsigned int into char array
*/
int MJS_UnicodeToChar(unsigned int unicode, char *out, unsigned int curr) {
 if(unicode <= 0x7F) {
  out[curr++] = unicode;
  return 1;
 } else if(unicode <= 0x7FF) {
  out[curr++] = 0xC0 | (unicode >> 6);
  out[curr++] = 0x80 | (unicode & 0x3F);
  return 2;
 } else if(unicode <= 0xFFFF) {
  out[curr++] = 0xE0 | (unicode >> 12);
  out[curr++] = 0x80 | ((unicode >> 6) & 0x3F);
  out[curr++] = 0x80 | (unicode & 0x3F);
  return 3;
 } else {
  out[curr++] = 0xF0 | (unicode >> 18);
  out[curr++] = 0x80 | ((unicode >> 12) & 0x3F);
  out[curr++] = 0x80 | ((unicode >> 6) & 0x3F);
  out[curr++] = 0x80 | (unicode & 0x3F);
  return 4;
 }
 return 0; /* unreachable */
}


