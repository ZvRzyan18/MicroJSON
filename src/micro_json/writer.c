#include "micro_json/writer.h"
#include "micro_json/parser.h"
#include <string.h>

/*-----------------Static func decl-------------------*/

MJS_HOT static int write_object(MJSOutputStreamBuffer *buff, MJSObject *obj, unsigned int depth);
MJS_HOT static int write_value(MJSOutputStreamBuffer *buff, MJSObject *obj, MJSDynamicType *value, unsigned int depth);
MJS_HOT static int write_array(MJSOutputStreamBuffer *buff, MJSObject *obj, MJSArray *arr, unsigned int depth);
MJS_HOT static int indent(MJSOutputStreamBuffer *buff, unsigned int count);

/*-----------------Writer func-------------------*/

MJS_COLD int MJSWriter_Serialize(MJSOutputStreamBuffer *buff, MJSObject *obj) {
 if(MJS_Unlikely(!buff || !obj))
  return MJS_RESULT_NULL_POINTER;
 int result;
 
 MJSObjectPair *ref = MJSObject_GetPairReference(obj, "root", 4);
 if(MJS_Unlikely(!ref))
  return MJE_RESULT_ROOT_NOT_FOUND;
  
 result = write_value(buff, obj, &ref->value, 0);
 if(MJS_Unlikely(result))
  return result;
  
 result = MJSOutputStreamBuffer_Flush(buff);
 if(MJS_Unlikely(result))
  return result;

 return 0;
}

/*-----------------Static func-------------------*/

MJS_HOT static int write_object(MJSOutputStreamBuffer *buff, MJSObject *obj, unsigned int depth) {
 
 if(MJS_Unlikely(depth > MJS_MAX_NESTED_VALUE))
  return MJS_RESULT_REACHED_MAX_NESTED_DEPTH;

 
 MJSObjectPair *pairs = obj->obj_pair_ptr;
 int result;
 unsigned int i, iter;
 unsigned int estimated_size = obj->obj_pair_size + obj->reserve + MJS_MAX_HASH_BUCKETS;
 unsigned int total_size = 0;

 buff->cache[0] = '\n';
 result = MJSOutputStreamBuffer_Write(buff, buff->cache, 1);
 if(MJS_Unlikely(result))
  return result;

 result = indent(buff, depth);
 if(MJS_Unlikely(result))
  return result;
 
 result = MJSOutputStreamBuffer_Write(buff, "{\n", 2);
 if(MJS_Unlikely(result))
  return result;

 result = indent(buff, depth);
 if(MJS_Unlikely(result))
  return result;


 /* check the size first */
 for(i = 0; i < estimated_size; i++) {
  if(pairs[i].key_pool_index != 0xFFFFFFFF) {
   total_size++;
  }
 }
 
 iter = 0;
 for(i = 0; i < estimated_size; i++) {
  if(pairs[i].key_pool_index != 0xFFFFFFFF) {

   MJSObjectPair m_pair = pairs[i];
   result = MJS_WriteStringToCache(buff, &obj->string_pool[m_pair.key_pool_index], m_pair.key_pool_size);
   if(MJS_Unlikely(result))
    return result;
   result = MJSOutputStreamBuffer_Write(buff, buff->cache, buff->cache_size);
   if(MJS_Unlikely(result))
    return result;
   
   result = MJSOutputStreamBuffer_Write(buff, " : ", 3);
   if(MJS_Unlikely(result))
    return result;

   result = write_value(buff, obj, &m_pair.value, depth);
  if(MJS_Unlikely(result))
    return result;

  if((iter+1) < total_size) {
   buff->cache[0] = ',';
   buff->cache[1] = '\n';
   result = MJSOutputStreamBuffer_Write(buff, buff->cache, 2);
   if(MJS_Unlikely(result))
    return result;

   result = indent(buff, depth);
   if(MJS_Unlikely(result))
    return result;

  }
  iter++;
  }
  
 }
 result = MJSOutputStreamBuffer_Write(buff, "\n", 1);
 if(MJS_Unlikely(result))
  return result;

 result = indent(buff, depth);
 if(MJS_Unlikely(result))
  return result;

 
 buff->cache[0] = '}';
 result = MJSOutputStreamBuffer_Write(buff, buff->cache, 1);
 if(MJS_Unlikely(result))
  return result;


 return 0;
}



MJS_HOT static int write_value(MJSOutputStreamBuffer *buff, MJSObject *obj, MJSDynamicType *value, unsigned int depth) {
 int result;
 switch(value->type) {
  case MJS_TYPE_STRING:

   result = MJS_WriteStringToCache(buff, MJSObject_GetStringFromPool(obj, &value->value_string), value->value_string.str_size);
   if(MJS_Unlikely(result))
    return result;
   result = MJSOutputStreamBuffer_Write(buff, buff->cache, buff->cache_size);
   if(MJS_Unlikely(result))
    return result;

  break;
  case MJS_TYPE_BOOLEAN:
  
   if(value->value_boolean.value) {
    result = MJSOutputStreamBuffer_Write(buff, "true", 4);
    if(MJS_Unlikely(result))
     return result;
   } else {
    result = MJSOutputStreamBuffer_Write(buff, "false", 5);
    if(MJS_Unlikely(result))
     return result;
   }
  break;
  case MJS_TYPE_NULL:

   result = MJSOutputStreamBuffer_Write(buff, "null", 4);
   if(MJS_Unlikely(result))
    return result;
      
  break;
  case MJS_TYPE_OBJECT:

   result = write_object(buff, &value->value_object, depth+1);
   if(MJS_Unlikely(result))
    return result;
 
  break;
  case MJS_TYPE_ARRAY:

   result = write_array(buff, obj, &value->value_array, depth+1);
   if(MJS_Unlikely(result))
    return result;

  break;
  case MJS_TYPE_NUMBER_INT:

   sprintf(buff->cache, "%i", value->value_int.value);
   result = MJSOutputStreamBuffer_Write(buff, buff->cache, strlen(buff->cache));
   if(MJS_Unlikely(result))
    return result;

  break;
  case MJS_TYPE_NUMBER_FLOAT:

   sprintf(buff->cache, "%f", value->value_float.value);
   result = MJSOutputStreamBuffer_Write(buff, buff->cache, strlen(buff->cache));
   if(MJS_Unlikely(result))
    return result;

  break;
  case 0xFF:
  /* empty space */
  break;
  default:
   return MJS_RESULT_INVALID_TYPE;
  break;
 }
 return 0;
}



MJS_HOT static int write_array(MJSOutputStreamBuffer *buff, MJSObject *obj, MJSArray *arr, unsigned int depth) {
 int result;
 unsigned int i;
 unsigned int arr_size = MJSArray_Size(arr);

 result = MJSOutputStreamBuffer_Write(buff, "[", 1);
 if(MJS_Unlikely(result))
  return result;
  
 for(i = 0; i < arr_size; i++) {
  MJSDynamicType *curr_obj = MJSArray_Get(arr, i);
  result = write_value(buff, obj, curr_obj, depth);
  if(MJS_Unlikely(result))
   return result;
  
  if((i+1) < arr_size) {
   result = MJSOutputStreamBuffer_Write(buff, ", ", 2);
   if(MJS_Unlikely(result))
    return result;
  }
 
  if(curr_obj->type == MJS_TYPE_OBJECT) {
   result = MJSOutputStreamBuffer_Write(buff, "\n", 1);
   if(MJS_Unlikely(result))
    return result;


  result = indent(buff, depth);
  if(MJS_Unlikely(result))
   return result;

  }
 }

 result = MJSOutputStreamBuffer_Write(buff, "]", 1);
 if(MJS_Unlikely(result))
  return result;
 return 0;
}



MJS_HOT static int indent(MJSOutputStreamBuffer *buff, unsigned int count) {
 memset(buff->cache, ' ', count);
 int result = MJSOutputStreamBuffer_Write(buff, buff->cache, count);
 if(MJS_Unlikely(result))
  return result;
 return 0;
}


