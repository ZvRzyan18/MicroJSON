#ifndef MC_JSON_PARSER_H
#define MC_JSON_PARSER_H

#include "micro_json/object.h"

#ifdef __cplusplus
extern "C" {
#endif

int MJS_IsWhiteSpace(char c);
int MJS_IsDigit(char c);

//parse string and its special cases, then set them into cache
int MJS_ParseStringToCache(MJSParsedData *data);
//parse number and write it into cache
unsigned char MJS_ParseNumberToCache(MJSParsedData *data);

#ifdef __cplusplus
}
#endif

#endif
