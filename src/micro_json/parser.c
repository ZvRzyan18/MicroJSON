#include "micro_json/parser.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#if (defined(__ARM_NEON) || defined(__ARM_NEON__)) && defined(MJS_FORCE_VECTORIZE)
#include "micro_json/parser_neon.h"
#endif

static const double __exp10_table[40] = {
 0.00000000000000000001,
 0.0000000000000000001,
 0.000000000000000001,
 0.00000000000000001,
 0.0000000000000001,
 0.000000000000001,
 0.00000000000001,
 0.0000000000001,
 0.000000000001,
 0.00000000001,
 0.0000000001,
 0.000000001,
 0.00000001,
 0.0000001,
 0.000001,
 0.00001,
 0.0001,
 0.001,
 0.01,
 0.1,
 1.0,
 10.0,
 100.0,
 1000.0,
 10000.0,
 100000.0,
 1000000.0,
 10000000.0,
 100000000.0,
 1000000000.0,
 10000000000.0,
 100000000000.0,
 1000000000000.0,
 10000000000000.0,
 100000000000000.0,
 1000000000000000.0,
 10000000000000000.0,
 100000000000000000.0,
 1000000000000000000.0,
 10000000000000000000.0,
};

static const double *__exp10 = __exp10_table + 20;

/*convert error code to string */
MJS_COLD const char* MJS_CodeToString(signed char code) {
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
  case MJS_RESULT_INVALID_WRITE_MODE:
   return "MJS_RESULT_INVALID_WRITE_MODE";
  break;
  case MJS_RESULT_UNSUCCESSFUL_IO_WRITE:
   return "MJE_RESULT_UNSUCCESSFUL_IO_WRITE";
  break;
  case MJS_RESULT_ROOT_NOT_FOUND:
   return "MJE_RESULT_ROOT_NOT_FOUND";
  break;
  case MJS_RESULT_TOO_LARGE_NUMBER:
   return "MJS_RESULT_TOO_LARGE_NUMBER";
  break;
  case MJS_RESULT_TOO_SMALL_NUMBER:
   return "MJS_RESULT_TOO_SMALL_NUMBER";
  break;
 }
 return "Unknown Error";
}


/*
 TODO : Optimize it for large strings
 minimize loop and switch branches.
*/
MJS_HOT int MJS_ParseStringToCache(MJSParsedData *parsed_data) {
 parsed_data->cache_size = 0;
 int result;
 char *cache_str = (char*)parsed_data->cache;
 
 while(MJS_Likely(parsed_data->current < parsed_data->end)) {

#if defined(MJS_NEON)
  if(MJS_Unlikely((parsed_data->cache_size+16+5) > parsed_data->cache_allocated_size)) {
   result = MJSParserData_ExpandCache(parsed_data);
   if(MJS_Unlikely(result)) return result;
  }
  Neon_ParseStringToCache(parsed_data);
#else
  if(MJS_Unlikely((parsed_data->cache_size+5) > parsed_data->cache_allocated_size)) {
   result = MJSParserData_ExpandCache(parsed_data);
   if(MJS_Unlikely(result)) return result;
  }
#endif

  switch(*parsed_data->current) {
   case '\\':
   
   if(MJS_Likely((parsed_data->current+1) < parsed_data->end)) {
   switch(*(++parsed_data->current)) {
    case '\\':
     cache_str[parsed_data->cache_size++] = '\\';
     /*parsed_data->current++;*/
    break;
    case 'n':
     cache_str[parsed_data->cache_size++] = '\n';
     /*parsed_data->current++;*/
    break;
    case '\"':
     cache_str[parsed_data->cache_size++] = '\"';
     /*parsed_data->current++;*/
    break;
    case 'b':
     cache_str[parsed_data->cache_size++] = '\b';
     /*parsed_data->current++;*/
    break;
    case 'r':
     cache_str[parsed_data->cache_size++] = '\r';
     /*parsed_data->current++;*/
    break;
    case 't':
     cache_str[parsed_data->cache_size++] = '\t';
     /*parsed_data->current++;*/
    break;
    case 'u': /*unicode*/
    
     /* avoid overflow access  */
     if(MJS_Unlikely((parsed_data->current+4) >= parsed_data->end))
      return MJS_RESULT_INCOMPLETE_STRING_SYNTAX;
     
     parsed_data->current++;
     result = MJS_ReadUnicodeHexadecimal(parsed_data);
     if(MJS_Unlikely(result)) return result;
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

MJS_HOT int MJS_ParseNumberToCache(MJSParsedData *parsed_data) {
 parsed_data->cache_size = 0;
 int is_float = 0;
 int is_negative = 0;
 int has_exponent = 0;
 int has_fractional = 0;
 int whole_count = 0;
 MJS_Int64 whole_part = 0;
 MJS_Int64 fractional_part = 0;
 char fractional_part_count = 0;
 char exponent_part = 0;
 int is_exponent_negative = 0;

 while(MJS_Likely(parsed_data->current < parsed_data->end)) {
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
     goto __MJS_ParseNumberToCache_Output;
/*
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
*/
    } else {
     if(has_exponent) {
      exponent_part = exponent_part * 10 + (int)(*parsed_data->current - '0');
     } else if(has_fractional) {
      fractional_part = fractional_part * 10 + (int)(*parsed_data->current - '0');
      fractional_part_count++;
     } else {
      whole_part = whole_part * 10 + (int)(*parsed_data->current - '0');
      whole_count++;
     }
    }
   break;
  }
  parsed_data->current++;
 }

 __MJS_ParseNumberToCache_Output:
 
 if(is_float) {
  
  /* too large for float, its double */
  if(((whole_count + fractional_part_count) > 7) || (exponent_part > 7)) {
   if(MJS_Unlikely(!is_exponent_negative && exponent_part > 15))
    return MJS_RESULT_TOO_LARGE_NUMBER;

   if(MJS_Unlikely(is_exponent_negative && exponent_part > 15))
    return MJS_RESULT_TOO_SMALL_NUMBER;

   fractional_part = MJS_TruncuateFractional(fractional_part, &fractional_part_count);
   double fractional = ((double)fractional_part) * __exp10[-fractional_part_count];

   double whole = (double)whole_part;
   float final_value = 0.0;
   if(has_exponent && exponent_part > 0)
    final_value = (fractional + whole) * __exp10[is_exponent_negative ? -exponent_part : exponent_part];
   else
    final_value = whole + fractional;
 
   final_value = is_negative ? -final_value : final_value;
  
   /* might violate strict aliasing */
   *((double*)parsed_data->cache) = final_value;
  
   /* memcpy(parsed_data->cache, &final_value, sizeof(double)); */
   parsed_data->cache_size = (unsigned int)sizeof(double);

   return MJS_TYPE_NUMBER_DOUBLE;
  }
  
  /* small decimals, only store to float */
  float fractional = ((float)fractional_part) * (float)__exp10[-fractional_part_count];

  float whole = (float)whole_part;
  float final_value = 0.0f;
  if(has_exponent && exponent_part > 0)
   final_value = (float)(fractional + whole) * (float)__exp10[is_exponent_negative ? -exponent_part : exponent_part];
  else
   final_value = whole + fractional;

  final_value = is_negative ? -final_value : final_value;
  
  /* might violate strict aliasing */
  *((float*)parsed_data->cache) = final_value;
  
  /* memcpy(parsed_data->cache, &final_value, sizeof(float)); */
  parsed_data->cache_size = (unsigned int)sizeof(float);
  return MJS_TYPE_NUMBER_FLOAT;
 } else {
  if(MJS_Unlikely(whole_count >= 10)) {
   /*2,147,483,647*/
   if(whole_count == 10)
    if(whole_part == 2147483647) /*2 ^ 31 - 1*/
    whole_part = is_negative ? -whole_part : whole_part;
   else 
     return MJS_RESULT_TOO_LARGE_NUMBER;
   else
    return MJS_RESULT_TOO_LARGE_NUMBER;
  } else {
   whole_part = is_negative ? -whole_part : whole_part;
  }
  /* might violate strict aliasing */
  *((int*)parsed_data->cache) = whole_part;

  /* memcpy(parsed_data->cache, &whole_part, sizeof(int)); */
  parsed_data->cache_size = (unsigned int)sizeof(int);
  return MJS_TYPE_NUMBER_INT;
 }
 return 0;
}


MJS_HOT int MJS_ReadUnicodeHexadecimal(MJSParsedData *parsed_data) {
 if(MJS_Unlikely((parsed_data->current+4) < parsed_data->end)) {
  unsigned int i, value;
  value = 0;
  /* 
   json only requires 4 character hex.
   no more or less.
  */
  /* hoping that compiler might unroll this */
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
MJS_HOT int MJS_UnicodeToChar(unsigned int unicode, char *out, unsigned int curr) {
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

/*
 vice versa convertion from ascii
*/
MJS_HOT int MJS_UTF8ToUnicode(const char *s, int *code_size) {
 int cp = 0;
 if(s[0] < 0x80) {
  cp = s[0];
  *code_size = 1;
 } else if((s[0] >> 5) == 0x6) {
  cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
  *code_size = 2;
 } else if((s[0] >> 4) == 0xE) {
  cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
  *code_size = 3;
 } else if((s[0] >> 3) == 0x1E) {
  cp = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
  *code_size = 4;
 }
 return cp;
}


/*
 string writer
*/
MJS_HOT int MJS_WriteStringToCache(MJSOutputStreamBuffer *buff, const char *str, unsigned int str_size) {
 const char *begin_ptr = str;
 const char *end_ptr = str+str_size;
 buff->cache_size = 0;
 int advance = 0;
 int unicode = 0;
 
 buff->cache[buff->cache_size++] = '\"';

 while(begin_ptr < end_ptr) {
  
  /* make sure no overflow */
  if(MJS_Unlikely((buff->cache_size+5) > buff->cache_allocated_size)) {
   MJSOutputStreamBuffer_ExpandCache(buff);
  }
  
  /*
   convert some ascii back to the json escape sequence equivalent
  */
  switch(*begin_ptr) {
   case '\\':
    buff->cache[buff->cache_size++] = '\\';
    buff->cache[buff->cache_size++] = '\\';
    begin_ptr++;
   break;
   case '\n':
    buff->cache[buff->cache_size++] = '\\';
    buff->cache[buff->cache_size++] = 'n';
    begin_ptr++;
   break;
   case '\"':
    buff->cache[buff->cache_size++] = '\\';
    buff->cache[buff->cache_size++] = '\"';
    begin_ptr++;
   break;
   case '\b':
    buff->cache[buff->cache_size++] = '\\';
    buff->cache[buff->cache_size++] = 'b';
    begin_ptr++;
   break;
   case '\r':
    buff->cache[buff->cache_size++] = '\\';
    buff->cache[buff->cache_size++] = 'r';
    begin_ptr++;
   break;
   case '\t':
    buff->cache[buff->cache_size++] = '\\';
    buff->cache[buff->cache_size++] = 't';
    begin_ptr++;
   break;
   default:
    if(MJS_Unlikely(MJS_CheckUnicode(begin_ptr[0], begin_ptr[1]))) {
     unicode = MJS_UTF8ToUnicode(begin_ptr, &advance);
     sprintf(&buff->cache[buff->cache_size], "\\u%04x", unicode);
     buff->cache_size += advance+3;
     begin_ptr += advance;
    }
   break;
  }
 
  buff->cache[buff->cache_size] = *begin_ptr;
  buff->cache_size++;
  begin_ptr++;
 }

 buff->cache[buff->cache_size++] = '\"';

 return 0;
}


MJS_HOT MJS_Int64 MJS_TruncuateFractional(MJS_Int64 fractional, char *count) {
 char m_count = *count;
 char diff;
 double tmp;
 if(m_count > 17) {
  diff = m_count - 17;
  tmp = (double)fractional;
  tmp *= __exp10[-(m_count - diff)];
  *count = diff;
  return (MJS_Int64)tmp;
 }
 return fractional;
}

