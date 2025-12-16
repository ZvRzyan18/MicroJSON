#include "micro_json/parser.h"
#include "micro_json/object_impl.h"
#include <string.h>
#include <stdlib.h>


static const double mjs__exp10_table[40] = {
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

const double *mjs__exp10 = mjs__exp10_table + 20;

const unsigned char mjs__bruijin_numbers[32] = {
 0, 1, 28, 2, 29, 
 14, 24, 3, 30,  
 22, 20, 15, 25, 
 17, 4, 8, 31, 
 27, 13, 23, 21, 
 19, 16, 7, 26, 
 12, 18, 6, 11, 
 5, 10, 9
};
 
const char mjs__hex_table[255] = {
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 1, 2, 3, 4, 5, 6, 7,
 8, 9, 0, 0, 0, 0, 0, 0,
 0, 10, 11, 12, 13, 14, 15, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 10, 11, 12, 13, 14, 15, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0,
};
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

MJS_HOT int MJS_ParseNumber(MJSParsedData *parsed_data, MJSDynamicType *type) {

#define MJS_IS_FLOAT             0b00000001
#define MJS_IS_NEGATIVE          0b00000010
#define MJS_HAS_EXPONENT         0b00000100
#define MJS_HAS_FRACTIONAL       0b00001000
#define MJS_IS_EXPONENT_NEGATIVE 0b00010000

 MJS_Int64 whole_part = 0;
 MJS_Int64 fractional_part = 0;
 int whole_count = 0;
 char bool_state = 0;
 char fractional_part_count = 0;
 char exponent_part = 0;

 while(parsed_data->current < parsed_data->end) {
  switch(*parsed_data->current) {
   case '-':
    if(bool_state & MJS_HAS_EXPONENT) /* has_exponent */
     bool_state |= MJS_IS_EXPONENT_NEGATIVE; /* is_exponent_negative */
    else
     bool_state |= MJS_IS_NEGATIVE; /* is_negative */
   break;
   case '+':
    if(bool_state & MJS_HAS_EXPONENT) 
     bool_state &= ~MJS_IS_EXPONENT_NEGATIVE; /* is_exponent_negative */
    else
     bool_state &= ~MJS_IS_NEGATIVE; /* is_negative */
   break;
   case '.':
    if(bool_state & MJS_HAS_EXPONENT) /* has exponent */
     return 0; /* invalid number type */
    bool_state |= MJS_IS_FLOAT; /* is float */
    bool_state |= MJS_HAS_FRACTIONAL; /* has_fractional */
   break;
   case 'e':
    bool_state |= MJS_IS_FLOAT; /* is float */
    bool_state |= MJS_HAS_EXPONENT; /* has_exponent */
   break;
   default:
   
    if(!MJS_IsDigit(*parsed_data->current)) {
     parsed_data->current--;
     goto __MJS_ParseNumberToCache_Output;
    } else {
     if(bool_state & MJS_HAS_EXPONENT) { /* has_exponent */
      exponent_part = ((exponent_part << 1) + (exponent_part << 3)) + (int)(*parsed_data->current - '0');
     } else if(bool_state & MJS_HAS_FRACTIONAL) {
      fractional_part = ((fractional_part << 1) + (fractional_part << 3)) + (int)(*parsed_data->current - '0');
      fractional_part_count++;
     } else {
      whole_part = ((whole_part << 1) + (whole_part << 3)) + (int)(*parsed_data->current - '0');
      whole_count++;
     }
    }
   break;
  }
  parsed_data->current++;
 }

 __MJS_ParseNumberToCache_Output:
 
 if(bool_state & MJS_IS_FLOAT) {
  
  if(MJS_Unlikely(!(bool_state & MJS_IS_EXPONENT_NEGATIVE) && exponent_part > 15))
   return MJS_RESULT_TOO_LARGE_NUMBER;

  if(MJS_Unlikely((bool_state & MJS_IS_EXPONENT_NEGATIVE) && exponent_part > 15))
   return MJS_RESULT_TOO_SMALL_NUMBER;

  fractional_part = MJS_TruncuateFractional(fractional_part, &fractional_part_count);
  double fractional = ((double)fractional_part) * mjs__exp10[-fractional_part_count];

  double whole = (double)whole_part;
  double final_value = 0.0;
  if((bool_state & MJS_HAS_EXPONENT) && exponent_part > 0)
   final_value = (fractional + whole) * mjs__exp10[(bool_state & MJS_IS_EXPONENT_NEGATIVE) ? -exponent_part : exponent_part];
  else
   final_value = whole + fractional;
 
  final_value = (bool_state & MJS_IS_NEGATIVE) ? -final_value : final_value;
 
  if(((whole_count + fractional_part_count) <= 7) && (exponent_part <= 7)) {
   /**((float*)parsed_data->cache) = (float)final_value;*/
   type->type = MJS_TYPE_NUMBER_FLOAT;
   type->value_float.value = (float)final_value;
   return 0;
  } else {
   /**((double*)parsed_data->cache) = final_value;*/
   type->type = MJS_TYPE_NUMBER_DOUBLE;
   type->value_double.value = final_value;
   return 0;
  }
 } else {
  if(MJS_Unlikely(whole_count >= 10)) {
   /*max value : 2,147,483,647*/
   if(whole_count == 10)
    if(whole_part <= 2147483647) /*2 ^ 31 - 1*/
    whole_part = (bool_state & MJS_IS_NEGATIVE) ? -whole_part : whole_part;
   else 
     return MJS_RESULT_TOO_LARGE_NUMBER;
   else
    return MJS_RESULT_TOO_LARGE_NUMBER;
  } else {
   whole_part = (bool_state & MJS_IS_NEGATIVE) ? -whole_part : whole_part;
  }
  /**((int*)parsed_data->cache) = whole_part;*/
  type->type = MJS_TYPE_NUMBER_INT;
  type->value_int.value = whole_part;
  return 0;
 }
 return 0;
}

/*
 minimize the overhead of copy
*/
MJS_HOT int MJS_ParseStringToPool(MJSParsedData *parsed_data, MJSStringPoolNode *node, unsigned int *_index, unsigned int *_size) {
 int result;
 *_index = node->pool_size;
 unsigned int m_index = node->pool_size;
 unsigned int diff = 0;
 while(parsed_data->current < parsed_data->end) {


#if defined(MJS_NEON)
  if(MJS_Unlikely(node->pool_reserve < 5+16)) {
   result = MJSStringPool_ExpandNode(node, MJS_MAX_RESERVE_BYTES);
   if(MJS_Unlikely(result)) return result;
  }
  Neon_ParseStringToPool(parsed_data, node);
#else
  if(MJS_Unlikely(node->pool_reserve < 5)) {
   result = MJSStringPool_ExpandNode(node, MJS_MAX_RESERVE_BYTES);
   if(MJS_Unlikely(result)) return result;
  }
#endif

  switch(*parsed_data->current) {
   case '\\':
   
   if(MJS_Likely((parsed_data->current+1) < parsed_data->end)) {
   switch(*(++parsed_data->current)) {
    case '\\':
     node->str[node->pool_size++] = '\\';
    break;
    case 'n':
     node->str[node->pool_size++] = '\n';
    break;
    case '\"':
     node->str[node->pool_size++] = '\"';
    break;
    case 'b':
     node->str[node->pool_size++] = '\b';
    break;
    case 'r':
     node->str[node->pool_size++] = '\r';
    break;
    case 't':
     node->str[node->pool_size++] = '\t';
    break;
    case 'u': /*unicode*/
    
     /* avoid overflow access  */
     if(MJS_Unlikely((parsed_data->current+4) >= parsed_data->end))
      return MJS_RESULT_INCOMPLETE_STRING_SYNTAX;
     
     parsed_data->current++;
     result = MJS_ReadUnicodeHexadecimal(parsed_data, node);
     if(MJS_Unlikely(result)) return result;
    break;
    default:
     return MJS_RESULT_INVALID_ESCAPE_SEQUENCE;
    break;
   }
   }
   
   break;
   case '\"':
    goto __MJS_ParseStringToCache_finish;
   break;
   case '\n':
    return MJS_RESULT_INVALID_STRING_CHARACTER;
   break;
   default:
    node->str[node->pool_size++] = *parsed_data->current;
   break;
  }
  
  diff = node->pool_size - m_index;
  node->pool_reserve -= diff;
  m_index += diff;
  parsed_data->current++;
 }
 __MJS_ParseStringToCache_finish:
 *_size = (node->pool_size - (*_index));
 node->str[node->pool_size++] = '\0';
 node->pool_reserve--;
 return 0;
}


