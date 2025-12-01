#ifndef MC_JSON_PARSER_H
#define MC_JSON_PARSER_H

#include "micro_json/object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* convert code into constant string literals */
const char* MJS_CodeToString(signed char code);

/* check for valid whitespace */
int MJS_IsWhiteSpace(char c);

/* check it its digit or not */
int MJS_IsDigit(char c);

/* parse string and its special cases, then set them into cache */
int MJS_ParseStringToCache(MJSParsedData *parsed_data);

/* parse number and write it into cache */
int MJS_ParseNumberToCache(MJSParsedData *parsed_data);

/* parse unicode hexadecimal */
int MJS_ReadUnicodeHexadecimal(MJSParsedData *parsed_data);

/* parse unicode value to to char */
int MJS_UnicodeToChar(unsigned int codepoint, char *out, unsigned int curr);

#ifdef __cplusplus
}
#endif

#endif
