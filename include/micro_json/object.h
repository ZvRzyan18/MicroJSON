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
typedef struct MJSString MJSString;
typedef struct MJSInt MJSInt;
typedef struct MJSFloat MJSFloat;
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
 unsigned char type;
 unsigned int  str_size;
 unsigned int  pool_index;
};


struct MJSInt {
 unsigned char type;
 int           value;
};


struct MJSFloat {
 unsigned char type;
 float         value;
};


struct MJSBoolean {
 unsigned char type;
 unsigned char value;
};

/*-----------------Array Container-------------------*/
struct MJSArray {
 unsigned char  type;
 unsigned char  reserve;
 unsigned int   size;
 MJSDynamicType *dynamic_type_ptr;
};


MJS_COLD int MJSArray_Init(MJSArray *arr);
MJS_COLD int MJSArray_Destroy(MJSArray *arr);
MJS_HOT int MJSArray_Add(MJSArray *arr, MJSDynamicType *value);
MJS_HOT MJSDynamicType* MJSArray_Get(MJSArray *arr, unsigned int index);
MJS_HOT unsigned int MJSArray_Size(MJSArray *arr);

/*-----------------Object Container-------------------*/
struct MJSObject {
 unsigned char type;
 char          *string_pool;
 unsigned int  string_pool_size;
 unsigned int  string_pool_reserve;
 
 MJSObjectPair *obj_pair_ptr;
 unsigned int  next_empty;
 unsigned int  obj_pair_size;
 unsigned char reserve;
};


MJS_COLD int MJSObject_Init(MJSObject *container);
MJS_COLD int MJSObject_Destroy(MJSObject *container);
MJS_HOT int MJSObject_InsertFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size, MJSDynamicType *value);
MJS_HOT int MJSObject_Insert(MJSObject *container, const char *key, unsigned int str_size, MJSDynamicType *value);
MJS_HOT MJSObjectPair* MJSObject_GetPairReference(MJSObject *container, const char *key, unsigned int str_size);
MJS_HOT MJSObjectPair* MJSObject_GetPairReferenceFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size);
MJS_HOT unsigned int MJSObject_AddToStringPool(MJSObject *container, const char *str, unsigned int str_size);
MJS_HOT const char* MJSObject_GetStringFromPool(MJSObject *container, MJSString *str);

/*-----------------Types and pair-------------------*/
/* this simulates a dynamic type */
union MJSDynamicType {
 unsigned char type;
 MJSString     value_string;
 MJSInt        value_int;
 MJSFloat      value_float;
 MJSBoolean    value_boolean;
 MJSArray      value_array;
 MJSObject     value_object;
};


struct MJSObjectPair {
 unsigned int   key_pool_index;
 unsigned int   key_pool_size;
 MJSDynamicType value;
 unsigned int   next;
};


/*-----------------Parsed data object-------------------*/
/* main container of json parsed data */
struct MJSParsedData {
 unsigned char *cache;
 unsigned int cache_size;
 unsigned int cache_allocated_size;

 const char *current;
 const char *begin;
 const char *end;
 unsigned int cl; /*current line*/
 unsigned int cb; /*curly brackets*/
 unsigned int sb; /*square brackets*/
 unsigned int dq; /*double quote*/
 unsigned int nested;
 MJSObject    container;
};


MJS_COLD int MJSParserData_Init(MJSParsedData *parsed_data);
MJS_COLD int MJSParserData_Destroy(MJSParsedData *parsed_data);
MJS_HOT int MJSParserData_ExpandCache(MJSParsedData *parsed_data);

/*-----------------Parsed result-------------------*/
struct MJSTokenResult {
 unsigned int line;
 char code;
};

/*-----------------Output Stream buffer-------------------*/
struct MJSOutputStreamBuffer {
 unsigned char mode;
 FILE          *file_ptr;
 char          *buff;
 unsigned int  buff_size;
 unsigned int  buff_reserve;
 
 char         *cache;
 unsigned int cache_size;
 unsigned int cache_allocated_size;
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