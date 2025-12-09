#include "micro_json/object.h"
#include <string.h>
#include <stdlib.h>


/*-----------------Static func-------------------*/

#define IS_POWER_OF_TWO(x)  (!((x) & ((x) - 1)))

/*
 guaranteed alignment of memory
*/
MJS_HOT static void* __aligned_alloc(unsigned int m_size, unsigned char alignment) {
 void *p1, **p2;
 unsigned long long offset = alignment - 1 + sizeof(void*);
 p1 = malloc(m_size + offset);
 p2 = (void**)(((unsigned long long)p1 + offset) & ~(alignment - 1));
 p2[-1] = p1;
 return p2;
}


MJS_HOT static void* __aligned_realloc(void *ptr, unsigned int m_size, unsigned char alignment) {
 if(MJS_Unlikely(!ptr))
  return __aligned_alloc(m_size, alignment);
 void *p1, **p2;
 unsigned long long offset = alignment - 1 + sizeof(void*);
 p1 = realloc(((void**)ptr)[-1], m_size + offset);
 if(MJS_Unlikely(!p1))
  return NULL;
 p2 = (void**)(((unsigned long long)p1 + offset) & ~(alignment - 1));
 p2[-1] = p1;
 return p2;
}


MJS_HOT static void __aligned_dealloc(void* ptr) {
 if(MJS_Unlikely(!ptr)) 
  return;
 free(((void**)ptr)[-1]);
}

/*
 search a sequence of character inside a string pool
*/
MJS_HOT static unsigned int MJS_Search_FromPool(const char* a, unsigned int str_size_a, const char *b, unsigned int str_size_b) {
 const char *begin = a;
 const char *end = a + str_size_a;

 /*
  aggressive unrolled loop
 */
 while((begin+8) < end && ((begin + str_size_b+9) < end)) {
  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;

  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;

  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;

  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;
  
  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;

  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;

  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;

  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;
 }

 while((begin+4) < end && ((begin + str_size_b+5) < end)) {
  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;

  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;

  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;

  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;
 }

 while(begin < end && ((begin + str_size_b+1) < end)) {
  if(memcmp(begin, b, str_size_b+1) == 0) return (unsigned int)(end-begin);
  begin++;
 }
 return 0xFFFFFFFF;
}

/*-----------------MJSArray-------------------*/
/*
 allocate MJSArray object, return 0 if success, return -1 if not.
*/
MJS_COLD int MJSArray_Init(MJSArray *arr) {
 if(MJS_Unlikely(!arr))
  return MJS_RESULT_NULL_POINTER;
 memset(arr, 0x00, sizeof(MJSArray));
 arr->type = MJS_TYPE_ARRAY;
 arr->dynamic_type_ptr = (MJSDynamicType*)__aligned_alloc(sizeof(MJSDynamicType) * MJS_MAX_RESERVE_ELEMENTS, MJS_OPTIMAL_ALIGNMENT);
 if(MJS_Unlikely(!arr->dynamic_type_ptr))
  return MJS_RESULT_ALLOCATION_FAILED;
 arr->reserve = MJS_MAX_RESERVE_ELEMENTS;
 return 0;
}

/*
 destroy MJSArray object, return 0 if success, return -1 if not.
*/
MJS_COLD int MJSArray_Destroy(MJSArray *arr) {
 if(!arr)
  return MJS_RESULT_NULL_POINTER;
 /*
  delete them first, to make sure no memory leaks
 */
 int result;
 unsigned int i;
 for(i = 0; i < arr->size; i++) {
  switch(arr->dynamic_type_ptr[i].type) {
   case MJS_TYPE_ARRAY:
    result = MJSArray_Destroy(&arr->dynamic_type_ptr[i].value_array);
    if(MJS_Unlikely(result)) return result;
   break;
   case MJS_TYPE_OBJECT:
    result = MJSObject_Destroy(&arr->dynamic_type_ptr[i].value_object);
    if(MJS_Unlikely(result)) return result;
   break;
   case MJS_TYPE_STRING:
   case MJS_TYPE_BOOLEAN:
   case MJS_TYPE_NULL:
   case MJS_TYPE_NUMBER_INT:
   case MJS_TYPE_NUMBER_FLOAT:
  /* case 0xFF:*/
   break;
   default:
    return MJS_RESULT_INVALID_TYPE;
   break;
  }
 }
 __aligned_dealloc(arr->dynamic_type_ptr);
 return 0;
}

/*
 add to MJSArray object, return 0 if success, return -1 if not.
*/
MJS_HOT int MJSArray_Add(MJSArray *arr, MJSDynamicType *value) {
 if(MJS_Unlikely(!arr))
  return MJS_RESULT_NULL_POINTER;
 if(MJS_Unlikely(arr->reserve > 0)) {
  arr->dynamic_type_ptr[arr->size] = *value;
  arr->reserve--;
  arr->size++;
 } else {

  arr->dynamic_type_ptr = (MJSDynamicType*)__aligned_realloc(arr->dynamic_type_ptr, sizeof(MJSDynamicType) * (arr->size + MJS_MAX_RESERVE_ELEMENTS), MJS_OPTIMAL_ALIGNMENT);
  if(MJS_Unlikely(!arr->dynamic_type_ptr))
   return MJS_RESULT_ALLOCATION_FAILED;
   
  arr->reserve = MJS_MAX_RESERVE_ELEMENTS;
  
  /* add value */
  arr->dynamic_type_ptr[arr->size] = *value;
  arr->reserve--;
  arr->size++;
 }
 return 0;
}


/*
 get element ptr from MJSArray object, return ptr if success, return NULL if not.
*/
MJS_HOT MJSDynamicType* MJSArray_Get(MJSArray *arr, unsigned int index) {
 if(MJS_Unlikely(!arr))
  return NULL;
 return &arr->dynamic_type_ptr[index];
}

/*
 get size from MJSArray object, return size if success, return 0xFFFFFFFF if not.
*/
MJS_HOT unsigned int MJSArray_Size(MJSArray *arr) {
 if(MJS_Unlikely(!arr))
  return 0xFFFFFFFF;
 return arr->size;
}


MJS_HOT int MJSArray_AddString(MJSArray *arr, MJSObject *parent, const char *str) {
 MJSDynamicType str_obj;
 str_obj.type = MJS_TYPE_STRING;
 str_obj.value_string.str_size = (unsigned int)strlen(str);
 str_obj.value_string.pool_index = MJSObject_AddToStringPool(parent, str, str_obj.value_string.str_size);
 
 return MJSArray_Add(arr, &str_obj);
}


MJS_HOT const char* MJSArray_GetString(MJSArray *arr, MJSObject *parent, unsigned int index) {
 return MJSObject_GetStringFromPool(parent, &MJSArray_Get(arr, index)->value_string);
}


MJS_HOT int MJSArray_AddObject(MJSArray *arr, MJSObject *obj) {
 MJSDynamicType obj_type;
 obj_type.value_object = *obj;
 return MJSArray_Add(arr, &obj_type);
}


MJS_HOT MJSObject* MJSArray_GetObject(MJSArray *arr, unsigned int index) {
 return &MJSArray_Get(arr, index)->value_object;
}


MJS_HOT int MJSArray_AddInt(MJSArray *arr, int _int_type) {
 MJSDynamicType int_obj;
 int_obj.type = MJS_TYPE_NUMBER_INT;
 int_obj.value_int.value = _int_type;
 return MJSArray_Add(arr, &int_obj);
}


MJS_HOT int MJSArray_GetInt(MJSArray *arr, unsigned int index) {
 return MJSArray_Get(arr, index)->value_int.value;
}


MJS_HOT int MJSArray_AddFloat(MJSArray *arr, float _float_type) {
 MJSDynamicType float_obj;
 float_obj.type = MJS_TYPE_NUMBER_FLOAT;
 float_obj.value_float.value = _float_type;
 return MJSArray_Add(arr, &float_obj);
}


MJS_HOT float MJSArray_GetFloat(MJSArray *arr, unsigned int index) {
 return MJSArray_Get(arr, index)->value_float.value;
}


MJS_HOT int MJSArray_AddBoolean(MJSArray *arr, int _bool_type) {
 MJSDynamicType bool_obj;
 bool_obj.type = MJS_TYPE_NUMBER_FLOAT;
 bool_obj.value_boolean.value = _bool_type;
 return MJSArray_Add(arr, &bool_obj);
}


MJS_HOT int MJSArray_GetBoolean(MJSArray *arr, unsigned int index) {
 return MJSArray_Get(arr, index)->value_boolean.value;
}


MJS_HOT int MJSArray_AddArray(MJSArray *arr, MJSArray *arr_input) {
 MJSDynamicType arr_type;
 arr_type.value_array = *arr_input;
 return MJSArray_Add(arr, &arr_type);
}


MJS_HOT MJSArray* MJSArray_GetArray(MJSArray *arr, unsigned int index) {
 return &MJSArray_Get(arr, index)->value_array;
}

/*-----------------MJSContainer-------------------*/
MJS_HOT static unsigned int generate_hash_index(const char *str, unsigned int str_size);


MJS_COLD int MJSObject_Init(MJSObject *container) {
 if(MJS_Unlikely(!container))
  return MJS_RESULT_NULL_POINTER;
 memset(container, 0x00, sizeof(MJSObject));
 const unsigned int pre_allocated_pair = sizeof(MJSObjectPair) * (MJS_MAX_HASH_BUCKETS + MJS_MAX_RESERVE_ELEMENTS);
 container->obj_pair_ptr = (MJSObjectPair*)__aligned_alloc(pre_allocated_pair, MJS_OPTIMAL_ALIGNMENT);
 if(MJS_Unlikely(!container->obj_pair_ptr))
  return MJS_RESULT_ALLOCATION_FAILED;
 memset(container->obj_pair_ptr, 0xFF, pre_allocated_pair);
 container->type = MJS_TYPE_OBJECT;
 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
 container->next_empty = MJS_MAX_HASH_BUCKETS;
 container->string_pool = (char*)__aligned_alloc(sizeof(char) * MJS_MAX_RESERVE_BYTES, MJS_OPTIMAL_ALIGNMENT);
 if(MJS_Unlikely(!container->string_pool))
  return MJS_RESULT_ALLOCATION_FAILED;
 container->string_pool_reserve = MJS_MAX_RESERVE_BYTES;
 return 0;
}


MJS_COLD int MJSObject_Destroy(MJSObject *container) {
 if(MJS_Unlikely(!container))
  return MJS_RESULT_NULL_POINTER;
 /* destroy other allocated memory first. */
 int result;
 unsigned int i;
 unsigned int estimated_size = container->obj_pair_size + container->reserve + MJS_MAX_HASH_BUCKETS;
 for(i = 0; i < estimated_size; i++) {
  switch(container->obj_pair_ptr[i].value.type) {
   case MJS_TYPE_ARRAY:
    result = MJSArray_Destroy(&container->obj_pair_ptr[i].value.value_array);
    if(MJS_Unlikely(result)) return result;
   break;
   case MJS_TYPE_OBJECT:
    result = MJSObject_Destroy(&container->obj_pair_ptr[i].value.value_object);
    if(MJS_Unlikely(result)) return result;
   break;
   case MJS_TYPE_STRING:
   case MJS_TYPE_BOOLEAN:
   case MJS_TYPE_NULL:
   case MJS_TYPE_NUMBER_INT:
   case MJS_TYPE_NUMBER_FLOAT:
   case 0xFF:
   break;
   default:
    return MJS_RESULT_INVALID_TYPE;
   break;
  }
 }
 __aligned_dealloc(container->string_pool);
 __aligned_dealloc(container->obj_pair_ptr);
 return 0;
}



MJS_HOT int MJSObject_InsertFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size, MJSDynamicType *value) {
 if(MJS_Unlikely(!container || (pool_index > container->string_pool_size)))
  return MJS_RESULT_NULL_POINTER;

 if(MJS_Unlikely(!str_size))
  return MJS_RESULT_EMPTY_KEY;
  
 char *key = &container->string_pool[pool_index];
 unsigned int str_pool_index = pool_index;
 MJSObjectPair pair;
 
 pair.key_pool_index = str_pool_index;
 pair.key_pool_size = str_size;
 pair.next = 0xFFFFFFFF;
 pair.value = *value;

	const unsigned int hash_index = generate_hash_index(key, str_size);
	if(MJS_Unlikely(container->reserve == 0)) {
	 unsigned int max_size = container->reserve + container->obj_pair_size + MJS_MAX_HASH_BUCKETS;

  container->obj_pair_ptr = (MJSObjectPair*)__aligned_realloc(container->obj_pair_ptr, (max_size + MJS_MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair), MJS_OPTIMAL_ALIGNMENT);
  if(MJS_Unlikely(!container->obj_pair_ptr))
   return MJS_RESULT_ALLOCATION_FAILED;
		memset(&container->obj_pair_ptr[max_size], 0xFF, MJS_MAX_RESERVE_ELEMENTS * sizeof(MJSObjectPair));
	 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
	}
 
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];

	if(MJS_Unlikely(start_node->key_pool_index == 0xFFFFFFFF)) {
		*start_node = pair;
	} else {

	 unsigned int prev_index = hash_index;
  unsigned int next_index = hash_index;
 	do {
 	 start_node = &container->obj_pair_ptr[next_index];
 		prev_index = next_index;

 	 if(str_size == start_node->key_pool_size) {
    if(MJS_Unlikely(memcmp(key, &container->string_pool[start_node->key_pool_index], str_size) == 0)) {
      return MJS_RESULT_DUPLICATE_KEY; /* duplicate keys*/
    }
 	 }
 	 
		 next_index = start_node->next;
 	} while(next_index != 0xFFFFFFFF);

		next_index = container->next_empty++;
		container->obj_pair_ptr[next_index] = pair;
		container->obj_pair_ptr[prev_index].next = next_index;

		container->reserve--;
		container->obj_pair_size++;
 }
 return 0;
}


MJS_HOT int MJSObject_Insert(MJSObject *container, const char *key, unsigned int str_size, MJSDynamicType *value) {
 if(MJS_Unlikely(!container || !key || !str_size))
  return MJS_RESULT_NULL_POINTER;
 
 if(MJS_Unlikely(!key[0])) /* empty key */
  return MJS_RESULT_EMPTY_KEY;

 MJSObjectPair pair;
 
 pair.key_pool_index = 0;
 pair.key_pool_size = str_size;
 pair.next = 0xFFFFFFFF;
 pair.value = *value;

	const unsigned int hash_index = generate_hash_index(key, str_size);
	
	if(MJS_Unlikely(container->reserve == 0)) {
	 unsigned int max_size = container->reserve + container->obj_pair_size + MJS_MAX_HASH_BUCKETS;

  container->obj_pair_ptr = (MJSObjectPair*)__aligned_realloc(container->obj_pair_ptr, (max_size + MJS_MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair), MJS_OPTIMAL_ALIGNMENT);
  if(MJS_Unlikely(!container->obj_pair_ptr))
   return MJS_RESULT_ALLOCATION_FAILED;
		memset(&container->obj_pair_ptr[max_size], 0xFF, MJS_MAX_RESERVE_ELEMENTS * sizeof(MJSObjectPair));
	 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
	}
 
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];

	if(MJS_Unlikely(start_node->key_pool_index == 0xFFFFFFFF)) {
		pair.key_pool_index = MJSObject_AddToStringPool(container, key, str_size);
		*start_node = pair;
	} else {
	 unsigned int prev_index = hash_index;
  unsigned int next_index = hash_index;
 	do {
 	 start_node = &container->obj_pair_ptr[next_index];
 		prev_index = next_index;

 	 if(str_size == start_node->key_pool_size) {
    if(MJS_Unlikely(memcmp(key, &container->string_pool[start_node->key_pool_index], str_size) == 0)) {
      return MJS_RESULT_DUPLICATE_KEY; /* duplicate keys*/
    }
 	 }
 	 
		 next_index = start_node->next;
 	} while(next_index != 0xFFFFFFFF);
 	
 	pair.key_pool_index = MJSObject_AddToStringPool(container, key, str_size);

		next_index = container->next_empty++;
		
		container->obj_pair_ptr[next_index] = pair;
		container->obj_pair_ptr[prev_index].next = next_index;

		container->reserve--;
		container->obj_pair_size++;
 }
	
 return 0;
}


MJS_HOT MJSDynamicType* MJSObject_Get(MJSObject *container, const char *key, unsigned int str_size) {
 if(MJS_Unlikely(!container || !key))
  return NULL;
  
 const unsigned int key_len = str_size;
 const unsigned int hash_index = generate_hash_index(key, key_len);

	MJSObjectPair *start_node = NULL;

  unsigned int next_index = hash_index;
  do {
	 	start_node = &container->obj_pair_ptr[next_index];
 	 if(key_len == start_node->key_pool_size) {
    if(MJS_Unlikely(memcmp(key, &container->string_pool[start_node->key_pool_index], key_len) == 0)) {
	 	  return &start_node->value;
	 	 }
 	 }
		 next_index = start_node->next;
 	} while(next_index != 0xFFFFFFFF);
 return NULL;
}


MJS_HOT MJSDynamicType* MJSObject_GetFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size) {
 if(MJS_Unlikely(!container || (pool_index > container->string_pool_size)))
  return NULL;

 char *key = &container->string_pool[pool_index];
 const unsigned int key_len = str_size;
 const unsigned int hash_index = generate_hash_index(key, key_len);
   
	MJSObjectPair *start_node = NULL;

  unsigned int next_index = hash_index;
 	do {
	 	start_node = &container->obj_pair_ptr[next_index];

 	 if(key_len == start_node->key_pool_size) {
    if(MJS_Unlikely(memcmp(key, &container->string_pool[start_node->key_pool_index], key_len) == 0)) {
	 	  return &start_node->value;
	 	 }
 	 }
		 next_index = start_node->next;
 	} while(next_index != 0xFFFFFFFF);

 return NULL;
}



MJS_HOT unsigned int MJSObject_AddToStringPool(MJSObject *container, const char *str, unsigned int str_size) {
 if(MJS_Unlikely(!container || !str || !str_size))
  return 0xFFFFFFFF;
 
 unsigned int out_index = 0xFFFFFFFF;
 /* this allows the duplication of strings end up in the same location */
 out_index = MJS_Search_FromPool(container->string_pool, container->string_pool_size, str, str_size);
 if(out_index != 0xFFFFFFFF)
  return out_index;
 
 if(container->string_pool_reserve > (str_size+1)) {
  /* strncpy(&container->string_pool[container->string_pool_size], str, sizeof(char) * (str_size+1)); */
  memcpy(&container->string_pool[container->string_pool_size], str, sizeof(char) * str_size);
  container->string_pool[container->string_pool_size+str_size] = '\0';
  
  out_index = container->string_pool_size;
  container->string_pool_size += str_size+1;
  container->string_pool_reserve -= str_size+1;
  return out_index;
 } else {
  /*
   allocate a new reserve bytes
  */
  container->string_pool = (char*)__aligned_realloc(container->string_pool, sizeof(char) * (container->string_pool_size + (str_size+1) + MJS_MAX_RESERVE_ELEMENTS), MJS_OPTIMAL_ALIGNMENT);
  if(MJS_Unlikely(!container->string_pool))
   return 0xFFFFFFFF;
  /*
   concat the input string into the pool.
  */
  /* strncpy(&container->string_pool[container->string_pool_size], str, sizeof(char) * (str_size+1)); */
  memcpy(&container->string_pool[container->string_pool_size], str, sizeof(char) * str_size);
  container->string_pool[container->string_pool_size+str_size] = '\0';

  out_index = container->string_pool_size;
  container->string_pool_size += str_size+1;
  container->string_pool_reserve = MJS_MAX_RESERVE_ELEMENTS;
  return out_index;
 }
 return out_index;
}


MJS_HOT const char* MJSObject_GetStringFromPool(MJSObject *container, MJSString *str) {
 if(MJS_Unlikely(!container || !str))
  return NULL;
 return &container->string_pool[str->pool_index];
}




MJS_HOT int MJSObject_InsertString(MJSObject *container, const char *key, const char *str) {
 MJSDynamicType str_obj;
 str_obj.type = MJS_TYPE_STRING;
 str_obj.value_string.str_size = (unsigned int)strlen(str);
 str_obj.value_string.pool_index = MJSObject_AddToStringPool(container, str, str_obj.value_string.str_size);
 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool(container, key, key_size);
 
 return MJSObject_InsertFromPool(container, key_index, key_size, &str_obj);
}


MJS_HOT const char* MJSObject_GetString(MJSObject *container, const char *key) {
 return MJSObject_GetStringFromPool(container, &MJSObject_Get(container, key, strlen(key))->value_string);
}


MJS_HOT int MJSObject_InsertObject(MJSObject *container, const char *key, MJSObject *obj) {
 MJSDynamicType obj_type;
 obj_type.value_object = *obj;

 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool(container, key, key_size);
 
 return MJSObject_InsertFromPool(container, key_index, key_size, &obj_type);
}


MJS_HOT MJSObject* MJSObject_GetObject(MJSObject *container, const char *key) {
 return &MJSObject_Get(container, key, strlen(key))->value_object;
}


MJS_HOT int MJSObject_InsertInt(MJSObject *container, const char *key, int _int_type) {
 MJSDynamicType int_type;
 int_type.type = MJS_TYPE_NUMBER_INT;
 int_type.value_int.value = _int_type;
 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool(container, key, key_size);
 
 return MJSObject_InsertFromPool(container, key_index, key_size, &int_type);
}


MJS_HOT int MJSObject_GetInt(MJSObject *container, const char *key) {
 return MJSObject_Get(container, key, strlen(key))->value_int.value;
}


MJS_HOT int MJSObject_InsertFloat(MJSObject *container, const char *key, float _float_type) {
 MJSDynamicType float_type;
 float_type.type = MJS_TYPE_NUMBER_FLOAT;
 float_type.value_float.value = _float_type;
 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool(container, key, key_size);
 
 return MJSObject_InsertFromPool(container, key_index, key_size, &float_type);
}


MJS_HOT float MJSObject_GetFloat(MJSObject *container, const char *key) {
 return MJSObject_Get(container, key, strlen(key))->value_float.value;
}


MJS_HOT int MJSObject_InsertBoolean(MJSObject *container, const char *key, int _bool_type) {
 MJSDynamicType bool_type;
 bool_type.type = MJS_TYPE_BOOLEAN;
 bool_type.value_boolean.value = _bool_type;
 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool(container, key, key_size);
 
 return MJSObject_InsertFromPool(container, key_index, key_size, &bool_type);
}


MJS_HOT int MJSObject_GetBoolean(MJSObject *container, const char *key) {
 return MJSObject_Get(container, key, strlen(key))->value_boolean.value;
}


MJS_HOT int MJSObject_InsertArray(MJSObject *container, const char *key, MJSArray *arr) {
 MJSDynamicType arr_type;
 arr_type.type = MJS_TYPE_ARRAY;
 arr_type.value_array = *arr;
 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool(container, key, key_size);
 
 return MJSObject_InsertFromPool(container, key_index, key_size, &arr_type);
}


MJS_HOT MJSArray* MJSObject_GetArray(MJSObject *container, const char *key) {
 return &MJSObject_Get(container, key, strlen(key))->value_array;
}


/*-----------------Static func-------------------*/
/*
 Hash Function
 TODO : optimize this, mininize loop branches
 or use agressive unrolling
*/
MJS_HOT static unsigned int generate_hash_index(const char *str, unsigned int str_size) {
 unsigned int index = 0;
 unsigned int i = 0;
 const unsigned int m = str_size < 8 ? str_size : 8;
 
 while(i < m) index += (unsigned int)str[i++];

 /* power of two makes it a lot faster than other values */
#if IS_POWER_OF_TWO(MJS_MAX_HASH_BUCKETS)
 return (index * 123) & (MJS_MAX_HASH_BUCKETS-1);
#else
 return (index * 123) % MJS_MAX_HASH_BUCKETS;
#endif
}


/*-----------------MJSParser-------------------*/
/*
 allocate MJSParserData, return 0 if sucess, return -1 if not.
*/
MJS_COLD int MJSParserData_Init(MJSParsedData *parsed_data) {
 if(MJS_Unlikely(!parsed_data))
  return MJS_RESULT_NULL_POINTER;
 
 memset(parsed_data, 0, sizeof(MJSParsedData));
 parsed_data->cache = (unsigned char*)__aligned_alloc(sizeof(unsigned char) * MJS_MAX_CACHE_BYTES, MJS_OPTIMAL_ALIGNMENT);
 if(MJS_Unlikely(!parsed_data->cache))
  return MJS_RESULT_ALLOCATION_FAILED;
 parsed_data->cache_allocated_size = MJS_MAX_CACHE_BYTES;

 return 0;
}

/*
 destroy MJSParserData, return 0 if success, return -1 if not.
*/
MJS_COLD int MJSParserData_Destroy(MJSParsedData *parsed_data) {
 if(MJS_Unlikely(!parsed_data))
  return MJS_RESULT_NULL_POINTER;
 int result;
 result = MJSObject_Destroy(&parsed_data->container);
 if(MJS_Unlikely(result)) 
  return result;
 __aligned_dealloc(parsed_data->cache);
 return 0;
}


MJS_HOT int MJSParserData_ExpandCache(MJSParsedData *parsed_data) {
 if(MJS_Unlikely(!parsed_data))
  return MJS_RESULT_NULL_POINTER;
  
 parsed_data->cache = (unsigned char*)__aligned_realloc(parsed_data->cache, sizeof(unsigned char) * (parsed_data->cache_allocated_size + MJS_MAX_RESERVE_BYTES), MJS_OPTIMAL_ALIGNMENT);
 if(MJS_Unlikely(!parsed_data->cache))
  return MJS_RESULT_ALLOCATION_FAILED;
 parsed_data->cache_allocated_size += MJS_MAX_RESERVE_BYTES;
 return 0;
}


/*-----------------MJSOutputStreamBuffer_Init-------------------*/
MJS_COLD int MJSOutputStreamBuffer_Init(MJSOutputStreamBuffer *buff, unsigned char mode, FILE* fp) {
 if(MJS_Unlikely(!buff))
  return MJS_RESULT_NULL_POINTER;
 memset(buff, 0, sizeof(MJSOutputStreamBuffer));
 
 buff->mode = mode;
 
 switch(mode) {
  case MJS_WRITE_TO_MEMORY_BUFFER:
   buff->buff_reserve = MJS_MAX_RESERVE_BYTES;
   buff->buff = (char*)__aligned_alloc(sizeof(char) * MJS_MAX_RESERVE_BYTES, MJS_OPTIMAL_ALIGNMENT);
   if(MJS_Unlikely(!buff->buff))
    return MJS_RESULT_ALLOCATION_FAILED;
  break;
  case MJS_WRITE_TO_FILE:
   buff->file_ptr = fp;
  break;
  default:
   return MJS_RESULT_INVALID_WRITE_MODE;
  break;
 }

 buff->cache_allocated_size = MJS_MAX_RESERVE_BYTES;
 buff->cache = (char*)__aligned_alloc(sizeof(char) * MJS_MAX_RESERVE_BYTES, MJS_OPTIMAL_ALIGNMENT);
 if(MJS_Unlikely(!buff->cache))
  return MJS_RESULT_ALLOCATION_FAILED;
 return 0;
}


MJS_COLD int MJSOutputStreamBuffer_Destroy(MJSOutputStreamBuffer *buff) {
 if(MJS_Unlikely(!buff))
  return MJS_RESULT_NULL_POINTER;

 switch(buff->mode) {
  case MJS_WRITE_TO_MEMORY_BUFFER:
   __aligned_dealloc(buff->buff);
  break;
  case MJS_WRITE_TO_FILE:
   if(MJS_Unlikely(!buff->file_ptr))
    return MJS_RESULT_NULL_POINTER;
  break;
  default:
   return MJS_RESULT_INVALID_WRITE_MODE;
  break;
 }
 __aligned_dealloc(buff->cache);
 return 0;
}


MJS_HOT int MJSOutputStreamBuffer_Write(MJSOutputStreamBuffer *buff, char *arr, unsigned int arr_size) {
 if(MJS_Unlikely(!buff))
  return MJS_RESULT_NULL_POINTER;
 unsigned int elements;
 
 switch(buff->mode) {
  case MJS_WRITE_TO_MEMORY_BUFFER:
  
   /* maintain overall proper null terminated value via strncpy */
   if((arr_size+1) > buff->buff_reserve) {
    buff->buff = (char*)__aligned_realloc(buff->buff, sizeof(char) * (buff->buff_size + buff->buff_reserve + MJS_MAX_RESERVE_BYTES), MJS_OPTIMAL_ALIGNMENT);
    if(MJS_Unlikely(!buff->buff))
     return MJS_RESULT_ALLOCATION_FAILED;
    buff->buff_reserve += MJS_MAX_RESERVE_BYTES;
    /*strncpy(&buff->buff[buff->buff_size], arr, arr_size+1);*/
    memcpy(&buff->buff[buff->buff_size], arr, arr_size);
    buff->buff[buff->buff_size+arr_size] = '\0';

    buff->buff_size += arr_size;
    buff->buff_reserve -= arr_size;
   } else {
    /* strncpy(&buff->buff[buff->buff_size], arr, arr_size+1); */
 
    memcpy(&buff->buff[buff->buff_size], arr, arr_size);
    buff->buff[buff->buff_size+arr_size] = '\0';

    buff->buff_size += arr_size;
    buff->buff_reserve -= arr_size;
   }
  break;
  case MJS_WRITE_TO_FILE:
   if(MJS_Unlikely(!buff->file_ptr))
    return MJS_RESULT_NULL_POINTER;
   elements = (unsigned int)fwrite(arr, sizeof(char), arr_size, buff->file_ptr);
   if(MJS_Unlikely(elements != arr_size))
    return MJE_RESULT_UNSUCCESSFUL_IO_WRITE;
  break;
  default:
   return MJS_RESULT_INVALID_WRITE_MODE;
  break;
 }
 return 0;
}


MJS_HOT int MJSOutputStreamBuffer_Flush(MJSOutputStreamBuffer *buff) {
 if(MJS_Unlikely(!buff))
  return MJS_RESULT_NULL_POINTER;
 switch(buff->mode) {
  case MJS_WRITE_TO_MEMORY_BUFFER:
  break;
  case MJS_WRITE_TO_FILE:
   if(MJS_Unlikely(!buff->file_ptr))
    return MJS_RESULT_NULL_POINTER;
   fflush(buff->file_ptr);
  break;
  default:
   return MJS_RESULT_INVALID_WRITE_MODE;
  break;
 }
 return 0;
}


MJS_HOT int MJSOutputStreamBuffer_ExpandCache(MJSOutputStreamBuffer *buff) {
 if(MJS_Unlikely(!buff))
  return MJS_RESULT_NULL_POINTER;
  
 buff->cache = (char*)__aligned_realloc(buff->cache, sizeof(char) * (buff->cache_allocated_size + MJS_MAX_RESERVE_BYTES), MJS_OPTIMAL_ALIGNMENT);
 if(MJS_Unlikely(!buff->cache))
  return MJS_RESULT_ALLOCATION_FAILED;
 buff->cache_allocated_size += MJS_MAX_RESERVE_BYTES;
 return 0;
}

