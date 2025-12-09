#include "micro_json/vectorize_arm_neon.h"


/*
 NOTE : this might be slow for memory bounded
 devices.
*/


/*
 minimized branches vector,
 optimized for large strings.
*/
__FORCE_INLINE__ void Neon_ParseStringToCache(MJSParsedData *parsed_data) {

 const int8x16_t back_slash_16 = vdupq_n_s8('\\');
 const int8x16_t new_line_16 = vdupq_n_s8('\n');
 const int8x16_t double_quote_16 = vdupq_n_s8('\"');

 int8x16_t is_equal_16;
 int8x16_t current_16;
 int64x2_t counter;
 
 int8x16_t tmp1, tmp2, tmp3;
 
 /* 16-bit alignment */
 int ptr_aligned = (int)(((int64_t)parsed_data->current) & 15);
 
 /* process 16 characters in one iteration */
 while((parsed_data->current+16) < parsed_data->end && ptr_aligned) {
  current_16 = vld1q_s8((const signed char*)parsed_data->current);

  tmp1 = vceqq_s8(current_16, double_quote_16);
  tmp2 = vceqq_s8(current_16, back_slash_16);
  tmp3 = vceqq_s8(current_16, new_line_16);
  
  is_equal_16 = vorrq_s8(tmp2, tmp1);
  
  parsed_data->cl += Neon_CountNonZero(tmp3);

  if(Neon_AnyOf_s16(is_equal_16)) {
   /* subdivide the 128-bit vector into two 64-bit vectors */  
   counter[0] = parsed_data->cache_size;
   counter[1] = (int64_t)parsed_data->current;
   counter = Neon_UpdateCounterIfAny(counter, vget_low_s8(is_equal_16), 8);
   counter = Neon_UpdateCounterIfAny(counter, vget_high_s8(is_equal_16), 8);
   vst1q_s8((signed char*)&parsed_data->cache[parsed_data->cache_size], current_16);

   parsed_data->cache_size = counter[0];
   parsed_data->current = (char*)counter[1];

   return;
  }
  
  vst1q_s8((signed char*)&parsed_data->cache[parsed_data->cache_size], current_16);
  parsed_data->cache_size += 16;
  parsed_data->current += 16;


 }

}

