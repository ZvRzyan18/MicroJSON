#include "micro_json/object.h"
#include "micro_json/object_impl.h"

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


MJS_HOT int MJSArray_AddString(MJSArray *arr, MJSObject *parent, const char *str) {
 return MJSArray_AddString_IMPL(arr, parent, str);
}


MJS_HOT const char* MJSArray_GetString(MJSArray *arr, MJSObject *parent, unsigned int index) {
 return MJSArray_GetString_IMPL(arr, parent, index);
}


MJS_HOT int MJSArray_AddObject(MJSArray *arr, MJSObject *obj) {
 return MJSArray_AddObject_IMPL(arr, obj);
}


MJS_HOT MJSObject* MJSArray_GetObject(MJSArray *arr, unsigned int index) {
 return MJSArray_GetObject_IMPL(arr, index);
}


MJS_HOT int MJSArray_AddInt(MJSArray *arr, int _int_type) {
 return MJSArray_AddInt_IMPL(arr, _int_type);
}


MJS_HOT int MJSArray_GetInt(MJSArray *arr, unsigned int index) {
 return MJSArray_GetInt_IMPL(arr, index);
}


MJS_HOT int MJSArray_AddFloat(MJSArray *arr, float _float_type) {
 return MJSArray_AddFloat_IMPL(arr, _float_type);
}


MJS_HOT float MJSArray_GetFloat(MJSArray *arr, unsigned int index) {
 return MJSArray_GetFloat_IMPL(arr, index);
}


MJS_HOT int MJSArray_AddBoolean(MJSArray *arr, int _bool_type) {
 return MJSArray_AddBoolean_IMPL(arr, _bool_type);
}


MJS_HOT int MJSArray_GetBoolean(MJSArray *arr, unsigned int index) {
 return MJSArray_GetBoolean_IMPL(arr, index);
}


MJS_HOT int MJSArray_AddArray(MJSArray *arr, MJSArray *arr_input) {
 return MJSArray_AddArray_IMPL(arr, arr_input);
}


MJS_HOT MJSArray* MJSArray_GetArray(MJSArray *arr, unsigned int index) {
 return MJSArray_GetArray_IMPL(arr, index);
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



MJS_HOT int MJSObject_InsertFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size, MJSDynamicType *value) {
 if(MJS_Unlikely(!container || (pool_index > container->string_pool_size)))
  return MJS_RESULT_NULL_POINTER;
 return MJSObject_InsertFromPool_IMPL(container, pool_index, str_size, value);
}


MJS_HOT int MJSObject_Insert(MJSObject *container, const char *key, unsigned int str_size, MJSDynamicType *value) {
 if(MJS_Unlikely(!container || !key || !str_size))
  return MJS_RESULT_NULL_POINTER;
 return MJSObject_Insert_IMPL(container, key, str_size, value);
}


MJS_HOT MJSDynamicType* MJSObject_Get(MJSObject *container, const char *key, unsigned int str_size) {
 if(MJS_Unlikely(!container || !key))
  return NULL;
 return MJSObject_Get_IMPL(container, key, str_size);
}


MJS_HOT MJSDynamicType* MJSObject_GetFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size) {
 if(MJS_Unlikely(!container || (pool_index > container->string_pool_size)))
  return NULL;
 return MJSObject_GetFromPool_IMPL(container, pool_index, str_size);
}



MJS_HOT unsigned int MJSObject_AddToStringPool(MJSObject *container, const char *str, unsigned int str_size) {
 if(MJS_Unlikely(!container || !str || !str_size))
  return 0xFFFFFFFF;
 return MJSObject_AddToStringPool_IMPL(container, str, str_size);
}


MJS_HOT const char* MJSObject_GetStringFromPool(MJSObject *container, MJSString *str) {
 if(MJS_Unlikely(!container || !str))
  return NULL;
 return MJSObject_GetStringFromPool_IMPL(container, str);
}




MJS_HOT int MJSObject_InsertString(MJSObject *container, const char *key, const char *str) {
 return MJSObject_InsertString_IMPL(container, key, str);
}


MJS_HOT const char* MJSObject_GetString(MJSObject *container, const char *key) {
 return MJSObject_GetString_IMPL(container, key);
}


MJS_HOT int MJSObject_InsertObject(MJSObject *container, const char *key, MJSObject *obj) {
 return MJSObject_InsertObject_IMPL(container, key, obj);
}


MJS_HOT MJSObject* MJSObject_GetObject(MJSObject *container, const char *key) {
 return MJSObject_GetObject_IMPL(container, key);
}


MJS_HOT int MJSObject_InsertInt(MJSObject *container, const char *key, int _int_type) {
 return MJSObject_InsertInt_IMPL(container, key, _int_type);
}


MJS_HOT int MJSObject_GetInt(MJSObject *container, const char *key) {
 return MJSObject_GetInt_IMPL(container, key);
}


MJS_HOT int MJSObject_InsertFloat(MJSObject *container, const char *key, float _float_type) {
 return MJSObject_InsertFloat_IMPL(container, key, _float_type);
}


MJS_HOT float MJSObject_GetFloat(MJSObject *container, const char *key) {
 return MJSObject_GetFloat_IMPL(container, key);
}


MJS_HOT int MJSObject_InsertBoolean(MJSObject *container, const char *key, int _bool_type) {
 return MJSObject_InsertBoolean_IMPL(container, key, _bool_type);
}


MJS_HOT int MJSObject_GetBoolean(MJSObject *container, const char *key) {
 return MJSObject_GetBoolean_IMPL(container, key);
}


MJS_HOT int MJSObject_InsertArray(MJSObject *container, const char *key, MJSArray *arr) {
 return MJSObject_InsertArray_IMPL(container, key, arr);
}


MJS_HOT MJSArray* MJSObject_GetArray(MJSObject *container, const char *key) {
 return MJSObject_GetArray_IMPL(container, key);
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

