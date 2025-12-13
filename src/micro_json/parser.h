#ifndef MC_JSON_PARSER_H
#define MC_JSON_PARSER_H

#include "micro_json/object.h"

#if (defined(__ARM_NEON) || defined(__ARM_NEON__)) && defined(MJS_FORCE_VECTORIZE)
#include "micro_json/parser_neon.h"
#endif

/* convert lowecase char to uppercase */
#define MJS_ToUpperChar(c) (c & ~( (c >= 'a' && c <= 'z') * 0x20 ))

/* check for valid whitespace */
#define MJS_IsWhiteSpace(c) ((c == 0x20) || (c == 0x0A) || (c == 0x0D) || (c == 0x09))
/* check it its digit or not */
#define MJS_IsDigit(c) (c >= '0' && c <= '9')
/* rough estimation of unicode value checking */
#define MJS_CheckUnicode(c0, c1) (c0 > 0x7F && c1 != 0)

extern const double *mjs__exp10;

extern const char mjs__hex_table[255];

extern const unsigned char mjs__bruijin_numbers[32];

/*
 convert unicode hex unsigned int into char array
*/
MJS_INLINE int MJS_UnicodeToChar(unsigned int unicode, char *out, unsigned int curr) {
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
 return 0;
}

/* parse unicode hexadecimal */
MJS_INLINE int MJS_ReadUnicodeHexadecimal(MJSParsedData *parsed_data) {

 unsigned int value;
 
 const char c0 = *(parsed_data->current++);
  if(MJS_Unlikely((c0 < '0' && c0 > '9') || (c0 < 'a' && c0 > 'f') || (c0 < 'A' && c0 > 'F')))
   return MJS_RESULT_INVALID_HEX_VALUE;
 const char c1 = *(parsed_data->current++);
  if(MJS_Unlikely((c1 < '0' && c1 > '9') || (c1 < 'a' && c1 > 'f') || (c1 < 'A' && c1 > 'F')))
   return MJS_RESULT_INVALID_HEX_VALUE;
 const char c2 = *(parsed_data->current++);
  if(MJS_Unlikely((c2 < '0' && c2 > '9') || (c2 < 'a' && c2 > 'f') || (c2 < 'A' && c2 > 'F')))
   return MJS_RESULT_INVALID_HEX_VALUE;
 const char c3 = *(parsed_data->current);
  if(MJS_Unlikely((c3 < '0' && c3 > '9') || (c3 < 'a' && c3 > 'f') || (c3 < 'A' && c3 > 'F')))
   return MJS_RESULT_INVALID_HEX_VALUE;

 
 value = (mjs__hex_table[(int)c0] << 12) 
       | (mjs__hex_table[(int)c1] << 8)
       | (mjs__hex_table[(int)c2] << 4)
       | (mjs__hex_table[(int)c3]);
  
  parsed_data->cache_size += MJS_UnicodeToChar(value, (char*)parsed_data->cache, parsed_data->cache_size);
 return 0;
}



/*
 vice versa convertion from ascii
*/
MJS_INLINE int MJS_UTF8ToUnicode(const char *s, int *code_size) {
 if(s[0] < 0x80) {
  *code_size = 1;
  return s[0];
 } else if((s[0] >> 5) == 0x6) {
  *code_size = 2;
  return  ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
 } else if((s[0] >> 4) == 0xE) {
  *code_size = 3;
  return ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
 } else if((s[0] >> 3) == 0x1E) {
  *code_size = 4;
  return ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
 }
 return 0;
}


MJS_INLINE MJS_Int64 MJS_TruncuateFractional(MJS_Int64 fractional, char *count) {
 double tmp;
 char m_count = *count;
 char diff;
 if(m_count > 17) {
  diff = m_count - 17;
  tmp = (double)fractional;
  tmp *= mjs__exp10[-(m_count - diff)];
  *count = diff;
  return (MJS_Int64)tmp;
 }
 return fractional;
}


/* parse number and write it into cache */
MJS_HOT int MJS_ParseNumberToCache(MJSParsedData *parsed_data);

/* parse string to cache */
MJS_HOT int MJS_ParseStringToCache(MJSParsedData *parsed_data);

/* write string to cache */
MJS_HOT int MJS_WriteStringToCache(MJSOutputStreamBuffer *buff, const char *str, unsigned int str_size);


#endif
