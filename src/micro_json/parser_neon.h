#ifndef PARSER_NEON_H
#define PARSER_NEON_H

#include "micro_json/vectorize_arm_neon.h"

/*
 NOTE : this might be slow for memory bounded
 devices.
*/


/*
 minimized branches vector,
 optimized for large strings.
*/
MJS_INLINE void Neon_ParseStringToCache(MJSParsedData *parsed_data) {
 const int8x16_t back_slash_16 = vdupq_n_s8('\\');
 const int8x16_t new_line_16 = vdupq_n_s8('\n');
 const int8x16_t double_quote_16 = vdupq_n_s8('\"');

 uint8x16_t is_equal_16;
 int8x16_t current_16;
 
 int increment = 16;
 
 uint8x16_t tmp1, tmp2, tmp3;
 
 /* process 16 characters in one iteration */
 while((parsed_data->current+16) < parsed_data->end && !Neon_AnyOf_s16(is_equal_16)) {
  current_16 = vld1q_s8((const signed char*)parsed_data->current);
  
  tmp1 = vceqq_s8(current_16, double_quote_16);
  tmp2 = vceqq_s8(current_16, back_slash_16);
  tmp3 = vceqq_s8(current_16, new_line_16);
  
  is_equal_16 = vorrq_u8(vorrq_u8(tmp2, tmp1), tmp3);
  

  increment = Neon_FirstNonZeroIndex(is_equal_16);
  
  vst1q_s8((signed char*)&parsed_data->cache[parsed_data->cache_size], current_16);

  parsed_data->cache_size += increment;
  parsed_data->current += increment;

 }
 if(increment != 16) {
  parsed_data->current--;
  parsed_data->cache_size--;
 }
}

#endif

