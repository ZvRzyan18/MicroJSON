#ifndef MC_JSON_WRITER_H
#define MC_JSON_WRITER_H

#include "micro_json/object.h"

#ifdef __cplusplus
extern "C" {
#endif


MJS_COLD int MJSWriter_Serialize(MJSOutputStreamBuffer *buff, MJSStringPool *pool, MJSDynamicType *container);


#ifdef __cplusplus
}
#endif

#endif
