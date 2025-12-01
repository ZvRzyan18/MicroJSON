#include "micro_json/vectorize_arm_neon.h"

/*
 forward declaration of function to 
 minimize the error
*/


__FORCE_INLINE__ void Neon_tokenize_1(MJSParsedData *parsed_data) {
 const int8x8_t new_line_8 = vdup_n_s8('\n');
 const int8x8_t curly_brackets_8 = vdup_n_s8('{');

 const int8x16_t new_line_16 = vdupq_n_s8('\n');
 const int8x16_t curly_brackets_16 = vdupq_n_s8('{');

 /* process 32 character per iteration */
 while((parsed_data->current+32) < parsed_data->end) {
  int8x16_t current = vld1q_s8((const signed char*)parsed_data->current);
  int8x16_t any_equals = vorrq_s8(vceqq_s8(current, new_line_16), vceqq_s8(current, curly_brackets_16));
  if(!Neon_AnyOf_s16(Neon_IsWhitespace_s16(current)) || Neon_AnyOf_s16(any_equals))
   return;

  current = vld1q_s8((const signed char*)parsed_data->current+16);
  any_equals = vorrq_s8(vceqq_s8(current, new_line_16), vceqq_s8(current, curly_brackets_16));
  if(!Neon_AnyOf_s16(Neon_IsWhitespace_s16(current)) || Neon_AnyOf_s16(any_equals))
   return;

  parsed_data->current += 32;
 }

 /* process 16 character per iteration */
 while((parsed_data->current+16) < parsed_data->end) {
  const int8x16_t current = vld1q_s8((const signed char*)parsed_data->current);
  int8x16_t any_equals = vorrq_s8(vceqq_s8(current, new_line_16), vceqq_s8(current, curly_brackets_16));
  if(!Neon_AnyOf_s16(Neon_IsWhitespace_s16(current)) || Neon_AnyOf_s16(any_equals))
   return;
  parsed_data->current += 16;
 }

  /* process 8 character per iteration */
 while((parsed_data->current+8) < parsed_data->end) {
  const int8x8_t current = vld1_s8((const signed char*)parsed_data->current);
  int8x8_t any_equals = vorr_s8(vceq_s8(current, new_line_8), vceq_s8(current, curly_brackets_8));
  if(!Neon_AnyOf_s8(Neon_IsWhitespace_s8(current)) || Neon_AnyOf_s8(any_equals))
   return;
  parsed_data->current += 8;
 }
}



__FORCE_INLINE__ void Neon_read_json_object(MJSParsedData *parsed_data) {
 const int8x8_t new_line_8 = vdup_n_s8('\n');
 const int8x8_t curly_brackets_8 = vdup_n_s8('}');
 const int8x8_t double_quote_8 = vdup_n_s8('\"');
 const int8x8_t colon_8 = vdup_n_s8(':');
 const int8x8_t comma_8 = vdup_n_s8(',');

 const int8x16_t new_line_16 = vdupq_n_s8('\n');
 const int8x16_t curly_brackets_16 = vdupq_n_s8('}');
 const int8x16_t double_quote_16 = vdupq_n_s8('\"');
 const int8x16_t colon_16 = vdupq_n_s8(':');
 const int8x16_t comma_16 = vdupq_n_s8(',');


 /* process 32 character per iteration */
 while((parsed_data->current+32) < parsed_data->end) {
  int8x16_t current = vld1q_s8((const signed char*)parsed_data->current);
  int8x16_t any_equals = vorrq_s8(vorrq_s8(vorrq_s8(vorrq_s8(vceqq_s8(current, new_line_16), vceqq_s8(current, curly_brackets_16)), vceqq_s8(current, double_quote_16)), vceqq_s8(current, colon_16)), vceqq_s8(current, comma_16));
  if(!Neon_AnyOf_s16(Neon_IsWhitespace_s16(current)) || Neon_AnyOf_s16(any_equals))
   return;
  current = vld1q_s8((const signed char*)parsed_data->current+16);
  any_equals = vorrq_s8(vorrq_s8(vorrq_s8(vorrq_s8(vceqq_s8(current, new_line_16), vceqq_s8(current, curly_brackets_16)), vceqq_s8(current, double_quote_16)), vceqq_s8(current, colon_16)), vceqq_s8(current, comma_16));
  if(!Neon_AnyOf_s16(Neon_IsWhitespace_s16(current)) || Neon_AnyOf_s16(any_equals))
   return;
  parsed_data->current += 32;
 }

 /* process 16 character per iteration */
 while((parsed_data->current+16) < parsed_data->end) {
  const int8x16_t current = vld1q_s8((const signed char*)parsed_data->current);
  int8x16_t any_equals = vorrq_s8(vorrq_s8(vorrq_s8(vorrq_s8(vceqq_s8(current, new_line_16), vceqq_s8(current, curly_brackets_16)), vceqq_s8(current, double_quote_16)), vceqq_s8(current, colon_16)), vceqq_s8(current, comma_16));
  if(!Neon_AnyOf_s16(Neon_IsWhitespace_s16(current)) || Neon_AnyOf_s16(any_equals))
   return;
  parsed_data->current += 16;
 }
 
 /* process 8 character per iteration */
 while((parsed_data->current+8) < parsed_data->end) {
  const int8x8_t current = vld1_s8((const signed char*)parsed_data->current);
  int8x8_t any_equals = vorr_s8(vorr_s8(vorr_s8(vorr_s8(vceq_s8(current, new_line_8), vceq_s8(current, curly_brackets_8)), vceq_s8(current, double_quote_8)), vceq_s8(current, colon_8)), vceq_s8(current, comma_8));
  if(!Neon_AnyOf_s8(Neon_IsWhitespace_s8(current)) || Neon_AnyOf_s8(any_equals))
   return;
  parsed_data->current += 8;
 }
}

