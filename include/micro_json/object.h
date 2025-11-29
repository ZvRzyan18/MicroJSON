#ifndef MC_JSON_OBJECT_H
#define MC_JSON_OBJECT_H

#include "micro_json/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MJSParsedData MJSParsedData;
typedef struct MJSString MJSString;
typedef struct MJSInt MJSInt;
typedef struct MJSFloat MJSFloat;
typedef struct MJSBoolean MJSBoolean;
typedef struct MJSArray MJSArray;
typedef struct MJSContainer MJSContainer;
typedef union  MJSDynamicType MJSDynamicType;
typedef struct MJSObjectPair MJSObjectPair;



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


struct MJSArray {
 unsigned char  type;
 unsigned char  reserve;
 unsigned int   size;
 MJSDynamicType *dynamic_type_ptr;
};


int MJSArray_Init(MJSArray *arr);
int MJSArray_Destroy(MJSArray *arr);
int MJSArray_Add(MJSArray *arr, MJSDynamicType value);
MJSDynamicType* MJSArray_Get(MJSArray *arr, unsigned int index);
unsigned int MJSArray_Size(MJSArray *arr);

struct MJSContainer {
 char          *string_pool;
 unsigned int  string_pool_size;
 unsigned int  string_pool_reserve;
 
 MJSObjectPair *obj_pair_ptr;
 unsigned int  allocated_size;
 unsigned int  obj_pair_size;
 unsigned char reserve;
};


int MJSContainer_Init(MJSContainer *container);
int MJSContainer_Destroy(MJSContainer *container);
unsigned int MJSContainer_GetSize(MJSContainer *container);
int MJSContainer_InsertFromPool(MJSContainer *container, unsigned int pool_index, unsigned int str_size, MJSDynamicType value);
int MJSContainer_Insert(MJSContainer *container, const char *key, unsigned int str_size, MJSDynamicType value);
MJSObjectPair* MJSContainer_GetPairReference(MJSContainer *container, const char *key);
unsigned int MJSContainer_AddToStringPool(MJSContainer *container, const char *str, unsigned int str_size);


//this simulates a dynamic type
union MJSDynamicType {
 unsigned char type;
 MJSString     value_string;
 MJSInt        value_int;
 MJSFloat      value_float;
 MJSBoolean    value_boolean;
 MJSArray      value_array;
 MJSContainer  value_container;
};


struct MJSObjectPair {
 unsigned int   key_pool_index;
 unsigned int   key_pool_size;
 MJSDynamicType value;
 unsigned int   next;
};



//main container of json data
struct MJSParsedData {
 unsigned char *cache;
 unsigned int cache_size;
 unsigned int cache_allocated_size;

 const char *current;
 const char *begin;
 const char *end;
 unsigned int cl; //current line
 unsigned int cb; //curly brackets
 unsigned int sb; //square brackets
 unsigned int dq; //double quote
 unsigned int nested;
 MJSContainer container;
};


int MJSParserData_Create(MJSParsedData *parsed_data);
int MJSParserData_Destroy(MJSParsedData *parsed_data);




#ifdef __cplusplus
}
#endif

#endif