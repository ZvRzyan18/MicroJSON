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


struct MJSDouble {
 unsigned char type;
 double        value;
};


struct MJSBoolean {
 unsigned char type;
 unsigned char value;
};

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

MJS_HOT int MJSArray_AddString(MJSArray *arr, MJSObject *parent, const char *str);
MJS_HOT const char* MJSArray_GetString(MJSArray *arr, MJSObject *parent, unsigned int index);
MJS_HOT int MJSArray_AddObject(MJSArray *arr, MJSObject *obj);
MJS_HOT MJSObject* MJSArray_GetObject(MJSArray *arr, unsigned int index);
MJS_HOT int MJSArray_AddInt(MJSArray *arr, int _int_type);
MJS_HOT int MJSArray_GetInt(MJSArray *arr, unsigned int index);
MJS_HOT int MJSArray_AddFloat(MJSArray *arr, float _float_type);
MJS_HOT float MJSArray_GetFloat(MJSArray *arr, unsigned int index);
MJS_HOT int MJSArray_AddBoolean(MJSArray *arr, int _bool_type);
MJS_HOT int MJSArray_GetBoolean(MJSArray *arr, unsigned int index);
MJS_HOT int MJSArray_AddArray(MJSArray *arr, MJSArray *arr_input);
MJS_HOT MJSArray* MJSArray_GetArray(MJSArray *arr, unsigned int index);


/*-----------------Object Container-------------------*/
struct MJSObject {
 unsigned char  type;
 char           *string_pool;
 MJSObjectPair  *obj_pair_ptr;
 unsigned int   string_pool_size;
/* unsigned int next_empty;*/
 unsigned short string_pool_reserve;
 unsigned int   obj_pair_size;
 unsigned char  reserve;
};


MJS_COLD int MJSObject_Init(MJSObject *container);
MJS_COLD int MJSObject_Destroy(MJSObject *container);
MJS_HOT int MJSObject_InsertFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size, MJSDynamicType *value);
MJS_HOT int MJSObject_Insert(MJSObject *container, const char *key, unsigned int str_size, MJSDynamicType *value);
MJS_HOT MJSDynamicType* MJSObject_Get(MJSObject *container, const char *key, unsigned int str_size);
MJS_HOT MJSDynamicType* MJSObject_GetFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size);
MJS_HOT unsigned int MJSObject_AddToStringPool(MJSObject *container, const char *str, unsigned int str_size);
MJS_HOT const char* MJSObject_GetStringFromPool(MJSObject *container, MJSString *str);

MJS_HOT int MJSObject_InsertString(MJSObject *container, const char *key, const char *str);
MJS_HOT const char* MJSObject_GetString(MJSObject *container, const char *key);
MJS_HOT int MJSObject_InsertObject(MJSObject *container, const char *key, MJSObject *obj);
MJS_HOT MJSObject* MJSObject_GetObject(MJSObject *container, const char *key);
MJS_HOT int MJSObject_InsertInt(MJSObject *container, const char *key, int _int_type);
MJS_HOT int MJSObject_GetInt(MJSObject *container, const char *key);
MJS_HOT int MJSObject_InsertFloat(MJSObject *container, const char *key, float _float_type);
MJS_HOT float MJSObject_GetFloat(MJSObject *container, const char *key);
MJS_HOT int MJSObject_InsertBoolean(MJSObject *container, const char *key, int _bool_type);
MJS_HOT int MJSObject_GetBoolean(MJSObject *container, const char *key);
MJS_HOT int MJSObject_InsertArray(MJSObject *container, const char *key, MJSArray *arr);
MJS_HOT MJSArray* MJSObject_GetArray(MJSObject *container, const char *key);


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
 unsigned int   next;
};


/*-----------------Parsed data object-------------------*/
/* main container of json parsed data */
struct MJSParsedData {
 MJSObject    container;
 unsigned char *cache;

 const char *current;
 const char *begin;
 const char *end;

 unsigned int cache_allocated_size;

 unsigned int cl; /*current line*/
 unsigned int cb; /*curly brackets*/
 unsigned int sb; /*square brackets*/
 unsigned int dq; /*double quote*/
 unsigned int nested;

 unsigned short cache_size;
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
 FILE          *file_ptr;
 char          *buff;
 char         *cache;
 
 unsigned int  buff_size;

 unsigned int cache_size;
 unsigned int cache_allocated_size;
 unsigned short  buff_reserve;
 unsigned char mode;
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