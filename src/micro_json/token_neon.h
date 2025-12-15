#ifndef TOKEN_NEON_H
#define TOKEN_NEON_H
#include "micro_json/vectorize_arm_neon.h"


/*
 NOTE : this might be slow for memory bounded
 devices.
*/

MJS_INLINE void Neon_read_json_object(MJSParsedData *parsed_data) {
 const int8x16_t new_line_16 = vdupq_n_s8('\n');
 const int8x16_t curly_brackets_16 = vdupq_n_s8('}');
 const int8x16_t double_quote_16 = vdupq_n_s8('\"');
 const int8x16_t colon_16 = vdupq_n_s8(':');
 const int8x16_t comma_16 = vdupq_n_s8(',');
 const int8x16_t all_ones_16 = vdupq_n_s8(0xFF);

 uint8x16_t is_equal_16;
 int8x16_t current;
 
 int increment = 16;
  
 /* process 16 character per iteration */
 while((parsed_data->current+16) < parsed_data->end && !Neon_AnyOf_s16(is_equal_16)) {
  current = vld1q_s8((const signed char*)parsed_data->current);
  uint8x16_t any_equals = vorrq_u8(vorrq_u8(vorrq_u8(vceqq_s8(current, curly_brackets_16), vceqq_s8(current, double_quote_16)), vceqq_s8(current, colon_16)), vceqq_s8(current, comma_16));
  uint8x16_t not_whitespace = veorq_u8(Neon_IsWhitespace_s16(current), all_ones_16);
  is_equal_16 = vorrq_u8(not_whitespace, any_equals);
  
  parsed_data->cl += Neon_CountNonZero(vceqq_s8(current, new_line_16));

  increment = Neon_FirstNonZeroIndex(is_equal_16);
  parsed_data->current += increment;
 }
 parsed_data->current = (increment != 16) ? parsed_data->current-1 : parsed_data->current;
}


MJS_INLINE void Neon_read_json_object_value(MJSParsedData *parsed_data) {
 const int8x16_t new_line_16 = vdupq_n_s8('\n');
 const int8x16_t t_16 = vdupq_n_s8('t');
 const int8x16_t f_16 = vdupq_n_s8('f');
 const int8x16_t n_16 = vdupq_n_s8('n');
 const int8x16_t double_quote_16 = vdupq_n_s8('\"');
 const int8x16_t plus_16 = vdupq_n_s8('+');
 const int8x16_t minus_16 = vdupq_n_s8('-');
 const int8x16_t open_curly_brackets_16 = vdupq_n_s8('{');
 const int8x16_t close_curly_brackets_16 = vdupq_n_s8('}');
 const int8x16_t open_square_brackets_16 = vdupq_n_s8('[');
 const int8x16_t comma_16 = vdupq_n_s8(',');
 const int8x16_t all_ones_16 = vdupq_n_s8(0xFF);

 uint8x16_t is_equal_16;
 int8x16_t current;
 
 int increment = 16;


 /* process 16 character per iteration */
 while((parsed_data->current+16) < parsed_data->end && !Neon_AnyOf_s16(is_equal_16)) {
  current = vld1q_s8((const signed char*)parsed_data->current);
  uint8x16_t any_equals = vorrq_u8(vorrq_u8(vorrq_u8(vceqq_s8(current, t_16), vceqq_s8(current, f_16)), vceqq_s8(current, n_16)), vceqq_s8(current, double_quote_16));
  any_equals = vorrq_u8(vorrq_u8(vorrq_u8(vorrq_u8(vorrq_u8(vceqq_s8(current, plus_16), vceqq_s8(current, minus_16)), vceqq_s8(current, open_curly_brackets_16)), vceqq_s8(current, close_curly_brackets_16)), vceqq_s8(current, comma_16)), vceqq_s8(current, open_square_brackets_16));
  uint8x16_t not_whitespace = veorq_u8(Neon_IsWhitespace_s16(current), all_ones_16);
  is_equal_16 = vorrq_u8(vorrq_u8(Neon_IsDigit_s16(current), any_equals), not_whitespace);

  parsed_data->cl += Neon_CountNonZero(vceqq_s8(current, new_line_16));

  increment = Neon_FirstNonZeroIndex(is_equal_16);
  parsed_data->current += increment;
 }
 parsed_data->current = (increment != 16) ? parsed_data->current-1 : parsed_data->current;
}


MJS_INLINE void Neon_read_json_array_value(MJSParsedData *parsed_data) {
 const int8x16_t new_line_16 = vdupq_n_s8('\n');
 const int8x16_t t_16 = vdupq_n_s8('t');
 const int8x16_t f_16 = vdupq_n_s8('f');
 const int8x16_t n_16 = vdupq_n_s8('n');
 const int8x16_t double_quote_16 = vdupq_n_s8('\"');
 const int8x16_t plus_16 = vdupq_n_s8('+');
 const int8x16_t minus_16 = vdupq_n_s8('-');
 const int8x16_t open_curly_brackets_16 = vdupq_n_s8('{');
 const int8x16_t close_curly_brackets_16 = vdupq_n_s8('}');
 const int8x16_t open_square_brackets_16 = vdupq_n_s8('[');
 const int8x16_t close_square_brackets_16 = vdupq_n_s8('[');
 const int8x16_t comma_16 = vdupq_n_s8(',');
 const int8x16_t all_ones_16 = vdupq_n_s8(0xFF);

 uint8x16_t is_equal_16;
 int8x16_t current;
 
 int increment = 16;

 /* process 16 character per iteration */
 while((parsed_data->current+16) < parsed_data->end && !Neon_AnyOf_s16(is_equal_16)) {
  current = vld1q_s8((const signed char*)parsed_data->current);
  uint8x16_t any_equals = vorrq_u8(vorrq_u8(vorrq_u8(vceqq_s8(current, t_16), vceqq_s8(current, f_16)), vceqq_s8(current, n_16)), vceqq_s8(current, double_quote_16));
  any_equals = vorrq_u8(vorrq_u8(vorrq_u8(vorrq_u8(vorrq_u8(vorrq_u8(vceqq_s8(current, plus_16), vceqq_s8(current, minus_16)), vceqq_s8(current, open_curly_brackets_16)), vceqq_s8(current, close_curly_brackets_16)), vceqq_s8(current, comma_16)), vceqq_s8(current, open_square_brackets_16)), vceqq_s8(current, close_square_brackets_16));
  
  uint8x16_t not_whitespace = veorq_u8(Neon_IsWhitespace_s16(current), all_ones_16);
  is_equal_16 = vorrq_u8(vorrq_u8(Neon_IsDigit_s16(current), any_equals), not_whitespace);

  parsed_data->cl += Neon_CountNonZero(vceqq_s8(current, new_line_16));

  increment = Neon_FirstNonZeroIndex(is_equal_16);
  parsed_data->current += increment;

 }
 parsed_data->current = (increment != 16) ? parsed_data->current-1 : parsed_data->current;
}


#endif
