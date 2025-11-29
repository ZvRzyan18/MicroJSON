#ifndef MC_JSON_TOKEN_H
#define MC_JSON_TOKEN_H

#include "micro_json/object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
 unsigned int line;
 unsigned char code;
} MJSTokenResult;

MJSTokenResult MJS_StartToken(MJSParsedData *parsed_data, const char *str, unsigned int len);


#ifdef __cplusplus
}
#endif

#endif
