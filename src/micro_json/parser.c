#include "micro_json/parser.h"
#include <string.h>
#include <math.h>

//return if its a valid whitespace or not
int MJS_IsWhiteSpace(char c) {
 return (c == '\0') || (c == 0x20) || (c == 0x0A) || (c == 0x0D) || (c == 0x09);
}

int MJS_IsDigit(char c) {
 return c >= '0' && c <= '9';
}



int MJS_ParseStringToCache(MJSParsedData *parsed_data) {
 parsed_data->cache_size = 0;
 //FIX : cache has only 512 size of bytes, it might overflow for large strings
 //handle its reallocation
 while(parsed_data->current < parsed_data->end) {
  switch(*parsed_data->current) {
   case '\\':
   
   
   
   if((parsed_data->current+1) < parsed_data->end) {
   switch(*(++parsed_data->current)) {
    case '\\':
     parsed_data->cache[parsed_data->cache_size++] = '\\';
     parsed_data->current++;
    break;
    case 'n':
     parsed_data->cache[parsed_data->cache_size++] = '\n';
     parsed_data->current++;
    break;
    case '\"':
     parsed_data->cache[parsed_data->cache_size++] = '\"';
     parsed_data->current++;
    break;
    case 'b':
     parsed_data->cache[parsed_data->cache_size++] = '\b';
     parsed_data->current++;
    break;
    case 'r':
     parsed_data->cache[parsed_data->cache_size++] = '\r';
     parsed_data->current++;
    break;
    case 't':
     parsed_data->cache[parsed_data->cache_size++] = '\t';
     parsed_data->current++;
    break;
    default:
     return -1;
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
    parsed_data->cache[parsed_data->cache_size++] = *parsed_data->current;
   break;
  }
  parsed_data->current++;
 }
 __MJS_ParseStringToCache_finish:
 parsed_data->cache[parsed_data->cache_size+1] = '\0';
 return 0;
}



unsigned char MJS_ParseNumberToCache(MJSParsedData *parsed_data) {
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
    if(exponent_part) 
     is_exponent_negative = 1;
    else
     is_negative = 1;
   break;
   case '+':
    if(exponent_part) 
     is_exponent_negative = 1;
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
    if(!MJS_IsDigit(*parsed_data->current))
     return 0;
    else {
     if(has_exponent) {
      exponent_part = exponent_part * 10 + (int)(*parsed_data->current - '0');
     } else if(has_fractional) {
      fractional_part = fractional_part * 10 + (int)(*parsed_data->current - '0');
     } else {
      whole_part = whole_part * 10 + (int)(*parsed_data->current - '0');
     }
    }
   break;
  }
  parsed_data->current++;
 }
 
 if(is_float) {
  float fractional = ((float)fractional_part) * powf(10.0f, (float)-fractional_part_count);
  float whole = (float)whole_part;
  float final_value = 0.0f;
  
  if(has_exponent)
   final_value = (whole + fractional) * powf(10.0f, (float)(is_exponent_negative ? -exponent_part : exponent_part));
  else
   final_value = whole + fractional;
  memcpy(parsed_data->cache, &final_value, sizeof(float));
  parsed_data->cache_size = (unsigned int)sizeof(float);
  return MJS_TYPE_NUMBER_FLOAT;
 } else {
  memcpy(parsed_data->cache, &whole_part, sizeof(int));
  parsed_data->cache_size = (unsigned int)sizeof(int);
  return MJS_TYPE_NUMBER_INT;
 }
 return 0;
}


