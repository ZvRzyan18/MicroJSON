#ifndef MC_JSON_PARSER_H
#define MC_JSON_PARSER_H

#include "micro_json/object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* check for valid whitespace */
#define MJS_IsWhiteSpace(c) ((c == '\0') || (c == 0x20) || (c == 0x0A) || (c == 0x0D) || (c == 0x09))
/* check it its digit or not */
#define MJS_IsDigit(c) (c >= '0' && c <= '9')
/* rough estimation of unicode value checking */
#define MJS_CheckUnicode(c0, c1) (c0 > 0x7F && c1 != 0)

/* convert code into constant string literals */
MJS_COLD const char* MJS_CodeToString(signed char code);

/* parse string and its special cases, then set them into cache */
MJS_HOT int MJS_ParseStringToCache(MJSParsedData *parsed_data);

/* parse number and write it into cache */
MJS_HOT int MJS_ParseNumberToCache(MJSParsedData *parsed_data);

/* parse unicode hexadecimal */
MJS_HOT int MJS_ReadUnicodeHexadecimal(MJSParsedData *parsed_data);

/* parse unicode value to to char */
MJS_HOT int MJS_UnicodeToChar(unsigned int codepoint, char *out, unsigned int curr);

/* convert from char array to unicode */
MJS_HOT int MJS_UTF8ToUnicode(const char *s, int *code_size);

/* write string to cache */
MJS_HOT int MJS_WriteStringToCache(MJSOutputStreamBuffer *buff, const char *str, unsigned int str_size);


#ifdef __cplusplus
}
#endif

#endif
