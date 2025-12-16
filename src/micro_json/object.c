#include "micro_json/object.h"
#include "micro_json/object_impl.h"

/*-----------------String Pooll::-------------------*/

MJS_COLD int MJSStringPool_Init(MJSStringPool *pool) {
 if(MJS_Unlikely(!pool))
  return MJS_RESULT_NULL_POINTER;
 return MJSStringPool_Init_IMPL(pool);
}


MJS_COLD int MJSStringPool_Destroy(MJSStringPool *pool) {
 if(MJS_Unlikely(!pool))
  return MJS_RESULT_NULL_POINTER;
 return MJSStringPool_Destroy_IMPL(pool);
}


MJS_HOT unsigned short MJSStringPool_GetCurrentNode(MJSStringPool *pool) {
 if(MJS_Unlikely(!pool))
  return 0xFFFF;
 return MJSStringPool_GetCurrentNode_IMPL(pool);
}

MJS_HOT int MJSStringPool_ExpandNode(MJSStringPoolNode *node, unsigned int additional_size) {
 if(MJS_Unlikely(!node))
  return MJS_RESULT_NULL_POINTER;
 return MJSStringPool_ExpandNode_IMPL(node, additional_size);
}


MJS_HOT int MJSStringPool_AddToPool(MJSStringPool *pool, const char *str, unsigned int str_size, unsigned int *out_index, unsigned short *out_chunk_index) {
 if(MJS_Unlikely(!pool || !str || !out_index || !out_chunk_index))
  return MJS_RESULT_NULL_POINTER;
 return MJSStringPool_AddToPool_IMPL(pool, str, str_size, out_index, out_chunk_index);
}


/*-----------------MJSArray-------------------*/
/*
 allocate MJSArray object, return 0 if success, return -1 if not.
*/
MJS_COLD int MJSArray_Init(MJSArray *arr) {
 if(MJS_Unlikely(!arr))
  return MJS_RESULT_NULL_POINTER;
 return MJSArray_Init_IMPL(arr);
}

/*
 destroy MJSArray object, return 0 if success, return -1 if not.
*/
MJS_COLD int MJSArray_Destroy(MJSArray *arr) {
 if(!arr)
  return MJS_RESULT_NULL_POINTER;
 return MJSArray_Destroy_IMPL(arr);
}

/*
 add to MJSArray object, return 0 if success, return -1 if not.
*/
MJS_HOT int MJSArray_Add(MJSArray *arr, MJSDynamicType *value) {
 if(MJS_Unlikely(!arr))
  return MJS_RESULT_NULL_POINTER;
 return MJSArray_Add_IMPL(arr, value);
}


/*
 get element ptr from MJSArray object, return ptr if success, return NULL if not.
*/
MJS_HOT MJSDynamicType* MJSArray_Get(MJSArray *arr, unsigned int index) {
 if(MJS_Unlikely(!arr))
  return NULL;
 return MJSArray_Get_IMPL(arr, index);
}

/*
 get size from MJSArray object, return size if success, return 0xFFFFFFFF if not.
*/
MJS_HOT unsigned int MJSArray_Size(MJSArray *arr) {
 if(MJS_Unlikely(!arr))
  return 0xFFFFFFFF;
 return MJSArray_Size_IMPL(arr);
}

/*-----------------MJSContainer-------------------*/


MJS_COLD int MJSObject_Init(MJSObject *container) {
 if(MJS_Unlikely(!container))
  return MJS_RESULT_NULL_POINTER;
 return MJSObject_Init_IMPL(container);
}


MJS_COLD int MJSObject_Destroy(MJSObject *container) {
 if(MJS_Unlikely(!container))
  return MJS_RESULT_NULL_POINTER;
 return MJSObject_Destroy_IMPL(container);
}



MJS_HOT int MJSObject_InsertFromPool(MJSObject *container, MJSStringPool *pool, unsigned int pool_index, unsigned int str_size, unsigned short pool_chunk_index, MJSDynamicType *value) {
 if(MJS_Unlikely(!container || !pool))
  return MJS_RESULT_NULL_POINTER;
 return MJSObject_InsertFromPool_IMPL(container, pool, pool_index, str_size, pool_chunk_index, value);
}


MJS_HOT int MJSObject_Insert(MJSObject *container, MJSStringPool *pool, const char *key, unsigned int str_size, MJSDynamicType *value) {
 if(MJS_Unlikely(!container || !key || !str_size || !pool))
  return MJS_RESULT_NULL_POINTER;
 return MJSObject_Insert_IMPL(container, pool, key, str_size, value);
}


MJS_HOT MJSDynamicType* MJSObject_Get(MJSObject *container, MJSStringPool *pool, const char *key, unsigned int str_size) {
 if(MJS_Unlikely(!container || !key || !pool))
  return NULL;
 return MJSObject_Get_IMPL(container, pool, key, str_size);
}


MJS_HOT MJSDynamicType* MJSObject_GetFromPool(MJSObject *container, MJSStringPool *pool, unsigned int pool_index, unsigned int str_size, unsigned short pool_chunk_index) {
 if(MJS_Unlikely(!container || !pool))
  return NULL;
 return MJSObject_GetFromPool_IMPL(container, pool, pool_index, str_size, pool_chunk_index);
}



/*-----------------MJSParser-------------------*/
/*
 allocate MJSParserData, return 0 if sucess, return -1 if not.
*/
MJS_COLD int MJSParserData_Init(MJSParsedData *parsed_data) {
 if(MJS_Unlikely(!parsed_data))
  return MJS_RESULT_NULL_POINTER;
 return MJSParserData_Init_IMPL(parsed_data);
}

/*
 destroy MJSParserData, return 0 if success, return -1 if not.
*/
MJS_COLD int MJSParserData_Destroy(MJSParsedData *parsed_data) {
 if(MJS_Unlikely(!parsed_data))
  return MJS_RESULT_NULL_POINTER;
 return MJSParserData_Destroy_IMPL(parsed_data);
}


/*-----------------MJSOutputStreamBuffer_Init-------------------*/
MJS_COLD int MJSOutputStreamBuffer_Init(MJSOutputStreamBuffer *buff, unsigned char mode, FILE* fp) {
 if(MJS_Unlikely(!buff))
  return MJS_RESULT_NULL_POINTER;
 return MJSOutputStreamBuffer_Init_IMPL(buff, mode, fp);
}


MJS_COLD int MJSOutputStreamBuffer_Destroy(MJSOutputStreamBuffer *buff) {
 if(MJS_Unlikely(!buff))
  return MJS_RESULT_NULL_POINTER;
 return MJSOutputStreamBuffer_Destroy_IMPL(buff);
}


MJS_HOT int MJSOutputStreamBuffer_Write(MJSOutputStreamBuffer *buff, char *arr, unsigned int arr_size) {
 if(MJS_Unlikely(!buff))
  return MJS_RESULT_NULL_POINTER;
 return MJSOutputStreamBuffer_Write_IMPL(buff, arr, arr_size);
}


MJS_HOT int MJSOutputStreamBuffer_Flush(MJSOutputStreamBuffer *buff) {
 if(MJS_Unlikely(!buff))
  return MJS_RESULT_NULL_POINTER;
 return MJSOutputStreamBuffer_Flush_IMPL(buff);
}


MJS_HOT int MJSOutputStreamBuffer_ExpandCache(MJSOutputStreamBuffer *buff) {
 if(MJS_Unlikely(!buff))
  return MJS_RESULT_NULL_POINTER;
 return MJSOutputStreamBuffer_ExpandCache_IMPL(buff);
}

