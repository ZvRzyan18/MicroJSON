#ifndef MC_JSON_TOKEN_H
#define MC_JSON_TOKEN_H

#include "micro_json/object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* initialize tokenizer */
int MJS_InitToken(MJSParsedData *parsed_data, const char *str, unsigned int len);

/* load everything to memory */
MJSTokenResult MJS_StartToken(MJSParsedData *parsed_data);

#ifdef __cplusplus
}
#endif

#endif
