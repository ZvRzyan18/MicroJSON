#include "micro_json/vectorize_arm_neon.h"


/*
 minimized branches vector,
 optimized for large strings.
*/
__FORCE_INLINE__ void Neon_ParseStringToCache(MJSParsedData *parsed_data) {

 const int8x16_t back_slash_16 = vdupq_n_s8('\\');
 const int8x16_t new_line_16 = vdupq_n_s8('\n');
 const int8x16_t double_quote_16 = vdupq_n_s8('\"');

 const int8x8_t back_slash_8 = vdup_n_s8('\\');
 const int8x8_t new_line_8 = vdup_n_s8('\n');
 const int8x8_t double_quote_8 = vdup_n_s8('\"');

 /*
  highly agressive loop unrolling, optimized
  by vector intructions.
 */
 /* process 64 characters in one iteration */
 while((parsed_data->current+64+5) < parsed_data->end) {

  /* expand cache size so it wont overflow */
  if((parsed_data->cache_size+64) > parsed_data->cache_allocated_size) {
   if(MJSParserData_ExpandCache(parsed_data))
    return;
  }
  
  int8x16_t current = vld1q_s8((const signed char*)parsed_data->current);
  int8x16_t is_equal = vorrq_s8(vorrq_s8(vceqq_s8(current, back_slash_16), vceqq_s8(current, new_line_16)), vceqq_s8(current, double_quote_16));

  if(Neon_AnyOf_s16(is_equal)) break;

  vst1q_s8((signed char*)parsed_data->cache, current);
  parsed_data->cache_size += 16;
  parsed_data->current += 16;

  current = vld1q_s8((const signed char*)parsed_data->current);
  is_equal = vorrq_s8(vorrq_s8(vceqq_s8(current, back_slash_16), vceqq_s8(current, new_line_16)), vceqq_s8(current, double_quote_16));

  if(Neon_AnyOf_s16(is_equal)) break;

  vst1q_s8((signed char*)parsed_data->cache, current);
  parsed_data->cache_size += 16;
  parsed_data->current += 16;
  
  current = vld1q_s8((const signed char*)parsed_data->current);
  is_equal = vorrq_s8(vorrq_s8(vceqq_s8(current, back_slash_16), vceqq_s8(current, new_line_16)), vceqq_s8(current, double_quote_16));

  if(Neon_AnyOf_s16(is_equal)) break;

  vst1q_s8((signed char*)parsed_data->cache, current);
  parsed_data->cache_size += 16;
  parsed_data->current += 16;

  current = vld1q_s8((const signed char*)parsed_data->current);
  is_equal = vorrq_s8(vorrq_s8(vceqq_s8(current, back_slash_16), vceqq_s8(current, new_line_16)), vceqq_s8(current, double_quote_16));

  if(Neon_AnyOf_s16(is_equal)) break;

  vst1q_s8((signed char*)parsed_data->cache, current);
  parsed_data->cache_size += 16;
  parsed_data->current += 16;
 }
 
 /* process 32 characters in one iteration */
 while((parsed_data->current+32) < parsed_data->end) {
  /* expand cache size so it wont overflow */
  if((parsed_data->cache_size+32+5) > parsed_data->cache_allocated_size) {
   if(MJSParserData_ExpandCache(parsed_data))
    return;
  }
  
  int8x16_t current = vld1q_s8((const signed char*)parsed_data->current);
  int8x16_t is_equal = vorrq_s8(vorrq_s8(vceqq_s8(current, back_slash_16), vceqq_s8(current, new_line_16)), vceqq_s8(current, double_quote_16));

  if(Neon_AnyOf_s16(is_equal)) break;

  vst1q_s8((signed char*)parsed_data->cache, current);
  parsed_data->cache_size += 16;
  parsed_data->current += 16;

  current = vld1q_s8((const signed char*)parsed_data->current);
  is_equal = vorrq_s8(vorrq_s8(vceqq_s8(current, back_slash_16), vceqq_s8(current, new_line_16)), vceqq_s8(current, double_quote_16));

  if(Neon_AnyOf_s16(is_equal)) break;

  vst1q_s8((signed char*)parsed_data->cache, current);
  parsed_data->cache_size += 16;
  parsed_data->current += 16;
 }
 
 
 /* process 16 characters in one iteration */
 while((parsed_data->current+16) < parsed_data->end) {
  const int8x16_t current = vld1q_s8((const signed char*)parsed_data->current);
  int8x16_t is_equal = vorrq_s8(vorrq_s8(vceqq_s8(current, back_slash_16), vceqq_s8(current, new_line_16)), vceqq_s8(current, double_quote_16));
  /*
   guarded with if, since it could hurt the performance
   and require to process characters one by one.
  */
  if(Neon_AnyOf_s16(is_equal)) break;

  /* expand cache size so it wont overflow */
  if((parsed_data->cache_size+16+5) > parsed_data->cache_allocated_size) {
   if(MJSParserData_ExpandCache(parsed_data))
    return;
  }
  
  vst1q_s8((signed char*)parsed_data->cache, current);
  parsed_data->cache_size += 16;
  parsed_data->current += 16;
 }


 /* process 8 characters in one iteration */
 while((parsed_data->current+8) < parsed_data->end) {
  const int8x8_t current = vld1_s8((const signed char*)parsed_data->current);
  int8x8_t is_equal = vorr_s8(vorr_s8(vceq_s8(current, back_slash_8), vceq_s8(current, new_line_8)), vceq_s8(current, double_quote_8));

  if(Neon_AnyOf_s8(is_equal)) break;
  vst1_s8((signed char*)parsed_data->cache, current);

  /* expand cache size so it wont overflow */
  if((parsed_data->cache_size+8+5) > parsed_data->cache_allocated_size) {
   if(MJSParserData_ExpandCache(parsed_data))
    return;
  }
  
  parsed_data->cache_size += 8;
  parsed_data->current += 8;
 }
}


