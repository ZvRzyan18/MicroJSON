#ifndef MC_JSON_TOKEN_H
#define MC_JSON_TOKEN_H

#include "micro_json/object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* initialize tokenizer */
MJS_COLD int MJS_InitToken(MJSParsedData *parsed_data, const char *str, unsigned int len);

/* load everything to memory */
MJS_COLD MJSTokenResult MJS_StartToken(MJSParsedData *parsed_data);

#ifdef __cplusplus
}
#endif

#endif
