#ifndef MC_JSON_OBJECT_H
#define MC_JSON_OBJECT_H

#include "micro_json/types.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------Forward def-------------------*/
/* for pointer types */
typedef struct MJSParsedData MJSParsedData;
typedef struct MJSStringPool MJSStringPool;
typedef struct MJSStringPoolNode MJSStringPoolNode;
typedef struct MJSString MJSString;
typedef struct MJSInt MJSInt;
typedef struct MJSFloat MJSFloat;
typedef struct MJSDouble MJSDouble;
typedef struct MJSBoolean MJSBoolean;
typedef struct MJSArray MJSArray;
typedef struct MJSObject MJSObject;
typedef union  MJSDynamicType MJSDynamicType;
typedef struct MJSObjectPair MJSObjectPair;
typedef struct MJSTokenResult MJSTokenResult;
typedef struct MJSOutputStreamBuffer MJSOutputStreamBuffer;

/*-----------------Struct Components-------------------*/
/*
 JSON types
*/
struct MJSString {
 unsigned char  type;
 unsigned int   str_size;
 unsigned int   pool_index;
 unsigned short chunk_index;
};


struct MJSInt {
 unsigned char type;
 int           value;
};


struct MJSFloat {
 unsigned char type;
 float         value;
};


struct MJSDouble {
 unsigned char type;
 double        value;
};


struct MJSBoolean {
 unsigned char type;
 unsigned char value;
};

/*-----------------String Pooll::-------------------*/

struct MJSStringPool {
 MJSStringPoolNode *root;
 unsigned short node_size;
 unsigned char node_reserve;
};

struct MJSStringPoolNode {
 char *str;
 unsigned int pool_size;
 unsigned short pool_reserve;
};

MJS_COLD int MJSStringPool_Init(MJSStringPool *pool);
MJS_COLD int MJSStringPool_Destroy(MJSStringPool *pool);
MJS_HOT unsigned short MJSStringPool_GetCurrentNode(MJSStringPool *pool);
MJS_HOT int MJSStringPool_ExpandNode(MJSStringPoolNode *node, unsigned int additional_size);
MJS_HOT int MJSStringPool_AddToPool(MJSStringPool *pool, const char *str, unsigned int str_size, unsigned int *out_index, unsigned short *out_chunk_index);

/*-----------------Array Container-------------------*/
struct MJSArray {
 unsigned char  type;
 MJSDynamicType *dynamic_type_ptr;
 unsigned int   size;
 unsigned char  reserve;
};


MJS_COLD int MJSArray_Init(MJSArray *arr);
MJS_COLD int MJSArray_Destroy(MJSArray *arr);
MJS_HOT int MJSArray_Add(MJSArray *arr, MJSDynamicType *value);
MJS_HOT MJSDynamicType* MJSArray_Get(MJSArray *arr, unsigned int index);
MJS_HOT unsigned int MJSArray_Size(MJSArray *arr);

/*-----------------Object Container-------------------*/
struct MJSObject {
 unsigned char  type;
 MJSObjectPair  *obj_pair_ptr;
 unsigned int   obj_pair_size;
 unsigned char  reserve;
};


MJS_COLD int MJSObject_Init(MJSObject *container);
MJS_COLD int MJSObject_Destroy(MJSObject *container);
MJS_HOT int MJSObject_InsertFromPool(MJSObject *container, MJSStringPool *pool, unsigned int pool_index, unsigned int str_size, unsigned short pool_chunk_index, MJSDynamicType *value);
MJS_HOT int MJSObject_Insert(MJSObject *container, MJSStringPool *pool, const char *key, unsigned int str_size, MJSDynamicType *value);
MJS_HOT MJSDynamicType* MJSObject_Get(MJSObject *container, MJSStringPool *pool, const char *key, unsigned int str_size);
MJS_HOT MJSDynamicType* MJSObject_GetFromPool(MJSObject *container, MJSStringPool *pool, unsigned int pool_index, unsigned int str_size, unsigned short pool_chunk_index);


/*-----------------Types and pair-------------------*/
/* this simulates a dynamic type */
union MJSDynamicType {
 unsigned char type;
 MJSString     value_string;
 MJSInt        value_int;
 MJSFloat      value_float;
 MJSDouble     value_double;
 MJSBoolean    value_boolean;
 MJSArray      value_array;
 MJSObject     value_object;
};


struct MJSObjectPair {
 MJSDynamicType value;
 unsigned int   key_pool_index;
 unsigned int   key_pool_size;
 unsigned short chunk_node_index;
 unsigned int   next;
};


/*-----------------Parsed data object-------------------*/
/* main container of json parsed data */
struct MJSParsedData {
 MJSDynamicType container;

 const char *current;
 const char *end;

 unsigned int cl;
};


MJS_COLD int MJSParserData_Init(MJSParsedData *parsed_data);
MJS_COLD int MJSParserData_Destroy(MJSParsedData *parsed_data);

/*-----------------Parsed result-------------------*/
struct MJSTokenResult {
 unsigned int line;
 char code;
};

/*-----------------Output Stream buffer-------------------*/
struct MJSOutputStreamBuffer {
 FILE           *file_ptr;
 char           *buff;
 char           *cache;
 
 unsigned int   buff_size;

 unsigned int   cache_size;
 unsigned int   cache_allocated_size;
 unsigned short buff_reserve;
 unsigned char  mode;
};


MJS_COLD int MJSOutputStreamBuffer_Init(MJSOutputStreamBuffer *buff, unsigned char mode, FILE *fp);
MJS_COLD int MJSOutputStreamBuffer_Destroy(MJSOutputStreamBuffer *buff);
MJS_HOT int MJSOutputStreamBuffer_Write(MJSOutputStreamBuffer *buff, char *arr, unsigned int arr_size);
MJS_HOT int MJSOutputStreamBuffer_Flush(MJSOutputStreamBuffer *buff);
MJS_HOT int MJSOutputStreamBuffer_ExpandCache(MJSOutputStreamBuffer *buff);

#ifdef __cplusplus
}
#endif

#endif