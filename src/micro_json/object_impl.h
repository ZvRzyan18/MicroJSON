#ifndef OBJECT_IMPL_H
#define OBJECT_IMPL_H

#include "micro_json/object.h"
#include "micro_json/object_impl.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/*-----------------Static func-------------------*/

#define IS_POWER_OF_TWO(x)  (!((x) & ((x) - 1)))

/*
 guaranteed alignment of memory
*/
MJS_INLINE void* __aligned_alloc(unsigned int m_size) {
#if defined(MJS_CUSTOM_OPTIMAL_ALIGNMENT)
 void *p1, **p2;
 MJS_Uint64 offset = MJS_OPTIMAL_ALIGNMENT - 1 + sizeof(void*);
 p1 = malloc(m_size + offset);
 p2 = (void**)(((MJS_Uint64)p1 + offset) & ~(MJS_OPTIMAL_ALIGNMENT - 1));
 p2[-1] = p1;
 return p2;
#else
 return malloc(m_size);
#endif
}


MJS_INLINE void* __aligned_realloc(void *ptr, unsigned int m_size) {
#if defined(MJS_CUSTOM_OPTIMAL_ALIGNMENT)
 void *p1, **p2;
 MJS_Uint64 offset = MJS_OPTIMAL_ALIGNMENT - 1 + sizeof(void*);
 p1 = realloc(((void**)ptr)[-1], m_size + offset);
 p2 = (void**)(((MJS_Uint64)p1 + offset) & ~(MJS_OPTIMAL_ALIGNMENT - 1));
 p2[-1] = p1;
 return p2;
#else
 return realloc(ptr, m_size);
#endif
}


MJS_INLINE void __aligned_dealloc(void* ptr) {
#if defined(MJS_CUSTOM_OPTIMAL_ALIGNMENT)
 free(((void**)ptr)[-1]);
#else
 free(ptr);
#endif
}


/*-----------------Static func-------------------*/
/*
 Hash Function
 TODO : optimize this, mininize loop branches
 or use agressive unrolling
*/
MJS_INLINE unsigned int generate_hash_index(const char *str, unsigned int str_size) {
 unsigned int index = 0;
 unsigned int i = 0;
 const unsigned int m = str_size < 4 ? str_size : 4;
 
 while(i < m) index += (unsigned int)str[i++];

 /* power of two makes it a lot faster than other values */
#if IS_POWER_OF_TWO(MJS_MAX_HASH_BUCKETS)
 return (index * 123) & (MJS_MAX_HASH_BUCKETS-1);
#else
 return (index * 123) % MJS_MAX_HASH_BUCKETS;
#endif
}

/*
 alternative to memmem function in string.h
*/

MJS_INLINE unsigned int MJS_FastMemcmp(const char* a, const char *b, unsigned int s) {
 const char *end = a + s;

 while((a+8) < end)  {
  if(*(MJS_Uint64*)(a) != *(MJS_Uint64*)(b)) return 1;
  a += 8; b += 8;
 }
 
 while((a+4) < end)  {
  if(*(int*)(a) != *(int*)(b)) return 1;
  a += 4; b += 4;
 }
 
 while(a < end)  {
  if(*(a++) != *(b++)) return 1;
 }
 return 0;
}


/*
 search a sequence of character inside a string pool
*/
MJS_INLINE unsigned int MJS_Search_FromPool(const char* a, unsigned int str_size_a, const char *b, unsigned int str_size_b) {
 const char *end = a + str_size_a;

 const unsigned int size_b_1 = str_size_b + 1;
 
 while(a < end && ((a + size_b_1) < end)) {
  if(!MJS_FastMemcmp(a, b, size_b_1)) return (unsigned int)(end-a);
  a++;
 }
 return 0xFFFFFFFF;
}


/*
 init pair elements
*/
MJS_INLINE void init_pair(MJSObjectPair *ptr, unsigned int m_size) {
 MJSObjectPair *_begin = ptr;
 MJSObjectPair *_end = ptr + m_size;

 while((_begin+4) < _end) {
  _begin->key_pool_index = 0xFFFFFFFF;
  _begin->value.type = 0xFF;
  _begin++;

  _begin->key_pool_index = 0xFFFFFFFF;
  _begin->value.type = 0xFF;
  _begin++;

  _begin->key_pool_index = 0xFFFFFFFF;
  _begin->value.type = 0xFF;
  _begin++;

  _begin->key_pool_index = 0xFFFFFFFF;
  _begin->value.type = 0xFF;
  _begin++;
 }
 
 while(_begin < _end) {
  _begin->key_pool_index = 0xFFFFFFFF;
  _begin->value.type = 0xFF;
  _begin++;
 }
}


/*-----------------MJSArray-------------------*/
/*
 allocate MJSArray object, return 0 if success, return -1 if not.
*/
static MJS_HOT int MJSArray_Init_IMPL(MJSArray *arr) { 
 int result = 0;
 arr->type = MJS_TYPE_ARRAY;
 arr->dynamic_type_ptr = (MJSDynamicType*)__aligned_alloc(sizeof(MJSDynamicType) * MJS_MAX_RESERVE_ELEMENTS);
 result = !arr->dynamic_type_ptr * MJS_RESULT_ALLOCATION_FAILED;
 arr->reserve = MJS_MAX_RESERVE_ELEMENTS;
 arr->size = 0;
 return result;
}

/*
 destroy MJSArray object, return 0 if success, return -1 if not.
*/
static MJS_HOT int MJSArray_Destroy_IMPL(MJSArray *arr) {
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
   case MJS_TYPE_NUMBER_DOUBLE:
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
static MJS_HOT int MJSArray_Add_IMPL(MJSArray *arr, MJSDynamicType *value) {
 if(arr->reserve > 0) {
  arr->dynamic_type_ptr[arr->size++] = *value;
  arr->reserve--;
 } else {

  arr->dynamic_type_ptr = (MJSDynamicType*)__aligned_realloc(arr->dynamic_type_ptr, sizeof(MJSDynamicType) * (arr->size + MJS_MAX_RESERVE_ELEMENTS));
  if(MJS_Unlikely(!arr->dynamic_type_ptr))
   return MJS_RESULT_ALLOCATION_FAILED;
   
  arr->reserve = MJS_MAX_RESERVE_ELEMENTS;
  
  /* add value */
  arr->dynamic_type_ptr[arr->size++] = *value;
  arr->reserve--;
 }
 return 0;
}


/*
 get element ptr from MJSArray object, return ptr if success, return NULL if not.
*/
static MJS_HOT MJSDynamicType* MJSArray_Get_IMPL(MJSArray *arr, unsigned int index) {
 return &arr->dynamic_type_ptr[index];
}

/*
 get size from MJSArray object, return size if success, return 0xFFFFFFFF if not.
*/
static MJS_HOT unsigned int MJSArray_Size_IMPL(MJSArray *arr) {
 return arr->size;
}


static MJS_HOT int MJSArray_AddString_IMPL(MJSArray *arr, MJSObject *parent, const char *str) {
 MJSDynamicType str_obj;
 str_obj.type = MJS_TYPE_STRING;
 str_obj.value_string.str_size = (unsigned int)strlen(str);
 str_obj.value_string.pool_index = MJSObject_AddToStringPool(parent, str, str_obj.value_string.str_size);
 
 return MJSArray_Add_IMPL(arr, &str_obj);
}


static MJS_HOT const char* MJSArray_GetString_IMPL(MJSArray *arr, MJSObject *parent, unsigned int index) {
 assert((index < arr->size && index >= 0) && "out of bounds");
 MJSDynamicType *type = MJSArray_Get_IMPL(arr, index);
 assert(type->type == MJS_TYPE_STRING && "not a string");
 return &parent->string_pool[type->value_string.pool_index];
}


static MJS_HOT int MJSArray_AddObject_IMPL(MJSArray *arr, MJSObject *obj) {
 MJSDynamicType obj_type;
 obj_type.value_object = *obj;
 return MJSArray_Add_IMPL(arr, &obj_type);
}


static MJS_HOT MJSObject* MJSArray_GetObject_IMPL(MJSArray *arr, unsigned int index) {
 assert((index < arr->size && index >= 0) && "out of bounds");
 MJSDynamicType *type = MJSArray_Get_IMPL(arr, index);
 assert(type->type == MJS_TYPE_OBJECT && "not an object");
 return &type->value_object;
}


static MJS_HOT int MJSArray_AddInt_IMPL(MJSArray *arr, int _int_type) {
 MJSDynamicType int_obj;
 int_obj.type = MJS_TYPE_NUMBER_INT;
 int_obj.value_int.value = _int_type;
 return MJSArray_Add_IMPL(arr, &int_obj);
}


static MJS_HOT int MJSArray_GetInt_IMPL(MJSArray *arr, unsigned int index) {
 assert((index < arr->size && index >= 0) && "out of bounds");
 MJSDynamicType *type = MJSArray_Get_IMPL(arr, index);
 assert(type->type == MJS_TYPE_NUMBER_INT && "not an integer");
 return type->value_int.value;
}


static MJS_HOT int MJSArray_AddFloat_IMPL(MJSArray *arr, float _float_type) {
 MJSDynamicType float_obj;
 float_obj.type = MJS_TYPE_NUMBER_FLOAT;
 float_obj.value_float.value = _float_type;
 return MJSArray_Add_IMPL(arr, &float_obj);
}


static MJS_HOT float MJSArray_GetFloat_IMPL(MJSArray *arr, unsigned int index) {
 assert((index < arr->size && index >= 0) && "out of bounds");
 MJSDynamicType *type = MJSArray_Get_IMPL(arr, index);
 assert(type->type == MJS_TYPE_NUMBER_FLOAT && "not a float");
 return type->value_float.value;
}


static MJS_HOT int MJSArray_AddBoolean_IMPL(MJSArray *arr, int _bool_type) {
 MJSDynamicType bool_obj;
 bool_obj.type = MJS_TYPE_NUMBER_FLOAT;
 bool_obj.value_boolean.value = _bool_type;
 return MJSArray_Add_IMPL(arr, &bool_obj);
}


static MJS_HOT int MJSArray_GetBoolean_IMPL(MJSArray *arr, unsigned int index) {
 assert((index < arr->size && index >= 0) && "out of bounds");
 MJSDynamicType *type = MJSArray_Get_IMPL(arr, index);
 assert(type->type == MJS_TYPE_NUMBER_INT && "not a boolean");
 return type->value_boolean.value;
}


static MJS_HOT int MJSArray_AddArray_IMPL(MJSArray *arr, MJSArray *arr_input) {
 MJSDynamicType arr_type;
 arr_type.value_array = *arr_input;
 return MJSArray_Add_IMPL(arr, &arr_type);
}


static MJS_HOT MJSArray* MJSArray_GetArray_IMPL(MJSArray *arr, unsigned int index) {
 assert((index < arr->size && index >= 0) && "out of bounds");
 MJSDynamicType *type = MJSArray_Get_IMPL(arr, index);
 assert(type->type == MJS_TYPE_ARRAY && "not an array");
 return &type->value_array;
}

/*-----------------MJSContainer-------------------*/

static MJS_HOT int MJSObject_Init_IMPL(MJSObject *container) {
 int result = 0;
 const unsigned int pre_allocated_pair = (MJS_MAX_HASH_BUCKETS + MJS_MAX_RESERVE_ELEMENTS);
 container->obj_pair_ptr = (MJSObjectPair*)__aligned_alloc(sizeof(MJSObjectPair) * pre_allocated_pair);
 result = !container->obj_pair_ptr * MJS_RESULT_ALLOCATION_FAILED;
 if(MJS_Likely(!result))
 init_pair(container->obj_pair_ptr, pre_allocated_pair);
 /*
 memset(container->obj_pair_ptr, 0xFF, pre_allocated_pair);
 */
 container->type = MJS_TYPE_OBJECT;
 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
 container->string_pool = (char*)__aligned_alloc(MJS_MAX_RESERVE_BYTES);
 result = result ? result : (!container->string_pool * MJS_RESULT_ALLOCATION_FAILED);
 container->string_pool_reserve = MJS_MAX_RESERVE_BYTES;
 container->string_pool_size = 0;
 container->obj_pair_size = 0;
 return result;
}


static MJS_HOT int MJSObject_Destroy_IMPL(MJSObject *container) {
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
   case MJS_TYPE_NUMBER_DOUBLE:
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



static MJS_HOT int MJSObject_InsertFromPool_IMPL(MJSObject *container, unsigned int pool_index, unsigned int str_size, MJSDynamicType *value) {
 if(MJS_Unlikely(!str_size))
  return MJS_RESULT_EMPTY_KEY;
  
 const char *key = &container->string_pool[pool_index];
 const unsigned int str_pool_index = pool_index;
 MJSObjectPair pair;
 
 pair.key_pool_index = str_pool_index;
 pair.key_pool_size = str_size;
 pair.next = 0xFFFFFFFF;
 pair.value = *value;

	const unsigned int hash_index = generate_hash_index(key, str_size);
	if(container->reserve == 0) {
	 unsigned int max_size = container->reserve + container->obj_pair_size + MJS_MAX_HASH_BUCKETS;

  container->obj_pair_ptr = (MJSObjectPair*)__aligned_realloc(container->obj_pair_ptr, (max_size + MJS_MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair));
  if(MJS_Unlikely(!container->obj_pair_ptr))
   return MJS_RESULT_ALLOCATION_FAILED;
  init_pair(&container->obj_pair_ptr[max_size], MJS_MAX_RESERVE_ELEMENTS);
  /*
		memset(&container->obj_pair_ptr[max_size], 0xFF, MJS_MAX_RESERVE_ELEMENTS * sizeof(MJSObjectPair));
		*/
	 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
	}
 
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];

/*
 if(str_size == start_node->key_pool_size) {
  if(!MJS_FastMemcmp(key, &container->string_pool[start_node->key_pool_index], str_size)) {
   return MJS_RESULT_DUPLICATE_KEY;
  }
 }
 */
	if(start_node->key_pool_index == 0xFFFFFFFF) {
		*start_node = pair;
	} else {

	 unsigned int prev_index = hash_index;
  unsigned int next_index = start_node->next;
 	while(next_index != 0xFFFFFFFF) {
 	 start_node = &container->obj_pair_ptr[next_index];
 		prev_index = next_index;
/*
 	 if(str_size == start_node->key_pool_size) {
    if(!MJS_FastMemcmp(key, &container->string_pool[start_node->key_pool_index], str_size)) {
     return MJS_RESULT_DUPLICATE_KEY;
    }
 	 }
*/
		 next_index = start_node->next;
 	}

		next_index = MJS_MAX_HASH_BUCKETS + container->obj_pair_size;
		container->obj_pair_ptr[next_index] = pair;
		container->obj_pair_ptr[prev_index].next = next_index;

		container->reserve--;
		container->obj_pair_size++;
 }
 return 0;
}


static MJS_HOT int MJSObject_Insert_IMPL(MJSObject *container, const char *key, unsigned int str_size, MJSDynamicType *value) { 
 if(MJS_Unlikely(!key[0])) /* empty key */
  return MJS_RESULT_EMPTY_KEY;

 MJSObjectPair pair;
 
 pair.key_pool_size = str_size;
 pair.next = 0xFFFFFFFF;
 pair.value = *value;

	const unsigned int hash_index = generate_hash_index(key, str_size);
	
	if(container->reserve == 0) {
	 unsigned int max_size = container->reserve + container->obj_pair_size + MJS_MAX_HASH_BUCKETS;

  container->obj_pair_ptr = (MJSObjectPair*)__aligned_realloc(container->obj_pair_ptr, (max_size + MJS_MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair));
  if(MJS_Unlikely(!container->obj_pair_ptr))
   return MJS_RESULT_ALLOCATION_FAILED;
  init_pair(&container->obj_pair_ptr[max_size], MJS_MAX_RESERVE_ELEMENTS);
  /*
		memset(&container->obj_pair_ptr[max_size], 0xFF, MJS_MAX_RESERVE_ELEMENTS * sizeof(MJSObjectPair));
		*/
	 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
	}
 
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];
/*
 if(str_size == start_node->key_pool_size) {
  if(!MJS_FastMemcmp(key, &container->string_pool[start_node->key_pool_index], str_size)) {
   return MJS_RESULT_DUPLICATE_KEY;
  }
 }
*/
	if(start_node->key_pool_index == 0xFFFFFFFF) {
		pair.key_pool_index = MJSObject_AddToStringPool(container, key, str_size);
		*start_node = pair;
	} else {
	 unsigned int prev_index = hash_index;
  unsigned int next_index = start_node->next;
 	while(next_index != 0xFFFFFFFF) {
 	 start_node = &container->obj_pair_ptr[next_index];
 		prev_index = next_index;
/*
 	 if(str_size == start_node->key_pool_size) {
    if(!MJS_FastMemcmp(key, &container->string_pool[start_node->key_pool_index], str_size)) {
     return MJS_RESULT_DUPLICATE_KEY; 
    }
 	 }
 */
		 next_index = start_node->next;
 	}
 	
 	pair.key_pool_index = MJSObject_AddToStringPool(container, key, str_size);

		next_index = MJS_MAX_HASH_BUCKETS + container->obj_pair_size;
		
		container->obj_pair_ptr[next_index] = pair;
		container->obj_pair_ptr[prev_index].next = next_index;

		container->reserve--;
		container->obj_pair_size++;
 }
	
 return 0;
}


static MJS_HOT MJSDynamicType* MJSObject_Get_IMPL(MJSObject *container, const char *key, unsigned int str_size) {  
 const unsigned int key_len = str_size;
 const unsigned int hash_index = generate_hash_index(key, key_len);

	MJSObjectPair *start_node = NULL;

  unsigned int next_index = hash_index;
  do {
	 	start_node = &container->obj_pair_ptr[next_index];
 	 if(key_len == start_node->key_pool_size) {
    if(!MJS_FastMemcmp(key, &container->string_pool[start_node->key_pool_index], key_len)) {
	 	  return &start_node->value;
	 	 }
 	 }
		 next_index = start_node->next;
 	} while(next_index != 0xFFFFFFFF);
 return NULL;
}


static MJS_HOT MJSDynamicType* MJSObject_GetFromPool_IMPL(MJSObject *container, unsigned int pool_index, unsigned int str_size) {
 const char *key = &container->string_pool[pool_index];
 const unsigned int key_len = str_size;
 const unsigned int hash_index = generate_hash_index(key, key_len);
   
	MJSObjectPair *start_node = NULL;

  unsigned int next_index = hash_index;
 	do {
	 	start_node = &container->obj_pair_ptr[next_index];

 	 if(key_len == start_node->key_pool_size) {
    if(!MJS_FastMemcmp(key, &container->string_pool[start_node->key_pool_index], key_len)) {
	 	  return &start_node->value;
	 	 }
 	 }
		 next_index = start_node->next;
 	} while(next_index != 0xFFFFFFFF);

 return NULL;
}



static MJS_HOT unsigned int MJSObject_AddToStringPool_IMPL(MJSObject *container, const char *str, unsigned int str_size) { 
 unsigned int out_index = 0xFFFFFFFF;
 /* this allows the duplication of strings end up in the same location */
 
 /*
 out_index = MJS_Search_FromPool(container->string_pool, container->string_pool_size, str, str_size);
 if(out_index != 0xFFFFFFFF)
  return out_index;
 */
 
 /*
 void *ptr = memmem(container->string_pool, container->string_pool_size, str, str_size);
 if(ptr != NULL) 
  return (unsigned int) ((char*)ptr - container->string_pool);
 */
 if(container->string_pool_reserve > (str_size+1)) {
  memcpy(&container->string_pool[container->string_pool_size], str, str_size);
  container->string_pool[container->string_pool_size+str_size] = '\0';
  
  out_index = container->string_pool_size;
  container->string_pool_size += str_size+1;
  container->string_pool_reserve -= str_size+1;
  return out_index;
 } else {
  /*
   allocate a new reserve bytes
  */
  container->string_pool = (char*)__aligned_realloc(container->string_pool, (container->string_pool_size + (str_size+1) + MJS_MAX_RESERVE_ELEMENTS));
  if(MJS_Unlikely(!container->string_pool))
   return 0xFFFFFFFF;
  
  memcpy(&container->string_pool[container->string_pool_size], str, str_size);
  container->string_pool[container->string_pool_size+str_size] = '\0';

  out_index = container->string_pool_size;
  container->string_pool_size += str_size+1;
  container->string_pool_reserve = MJS_MAX_RESERVE_ELEMENTS;
  return out_index;
 }
}


static MJS_HOT const char* MJSObject_GetStringFromPool_IMPL(MJSObject *container, MJSString *str) {
 return &container->string_pool[str->pool_index];
}




static MJS_HOT int MJSObject_InsertString_IMPL(MJSObject *container, const char *key, const char *str) {
 MJSDynamicType str_obj;
 str_obj.type = MJS_TYPE_STRING;
 str_obj.value_string.str_size = (unsigned int)strlen(str);
 str_obj.value_string.pool_index = MJSObject_AddToStringPool_IMPL(container, str, str_obj.value_string.str_size);
 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool_IMPL(container, key, key_size);
 
 return MJSObject_InsertFromPool_IMPL(container, key_index, key_size, &str_obj);
}


static MJS_HOT const char* MJSObject_GetString_IMPL(MJSObject *container, const char *key) {
 MJSDynamicType *type = MJSObject_Get_IMPL(container, key, strlen(key));
 assert(type != NULL && "not found");
 assert(type->type == MJS_TYPE_STRING && "not a string");
 return MJSObject_GetStringFromPool_IMPL(container, &type->value_string);
}


static MJS_HOT int MJSObject_InsertObject_IMPL(MJSObject *container, const char *key, MJSObject *obj) {
 MJSDynamicType obj_type;
 obj_type.value_object = *obj;

 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool_IMPL(container, key, key_size);
 
 return MJSObject_InsertFromPool_IMPL(container, key_index, key_size, &obj_type);
}


static MJS_HOT MJSObject* MJSObject_GetObject_IMPL(MJSObject *container, const char *key) {
 MJSDynamicType *type = MJSObject_Get_IMPL(container, key, strlen(key));
 assert(type != NULL && "not found");
 assert(type->type == MJS_TYPE_OBJECT && "not an object");
 return &type->value_object;
}


static MJS_HOT int MJSObject_InsertInt_IMPL(MJSObject *container, const char *key, int _int_type) {
 MJSDynamicType int_type;
 int_type.type = MJS_TYPE_NUMBER_INT;
 int_type.value_int.value = _int_type;
 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool_IMPL(container, key, key_size);
 
 return MJSObject_InsertFromPool_IMPL(container, key_index, key_size, &int_type);
}


static MJS_HOT int MJSObject_GetInt_IMPL(MJSObject *container, const char *key) {
 MJSDynamicType *type = MJSObject_Get_IMPL(container, key, strlen(key));
 assert(type != NULL && "not found");
 assert(type->type == MJS_TYPE_NUMBER_INT && "not an integer");
 return type->value_int.value;
}


static MJS_HOT int MJSObject_InsertFloat_IMPL(MJSObject *container, const char *key, float _float_type) {
 MJSDynamicType float_type;
 float_type.type = MJS_TYPE_NUMBER_FLOAT;
 float_type.value_float.value = _float_type;
 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool_IMPL(container, key, key_size);
 
 return MJSObject_InsertFromPool_IMPL(container, key_index, key_size, &float_type);
}


static MJS_HOT float MJSObject_GetFloat_IMPL(MJSObject *container, const char *key) {
 MJSDynamicType *type = MJSObject_Get_IMPL(container, key, strlen(key));
 assert(type != NULL && "not found");
 assert(type->type == MJS_TYPE_NUMBER_FLOAT && "not a float");
 return type->value_float.value;
}


static MJS_HOT int MJSObject_InsertBoolean_IMPL(MJSObject *container, const char *key, int _bool_type) {
 MJSDynamicType bool_type;
 bool_type.type = MJS_TYPE_BOOLEAN;
 bool_type.value_boolean.value = _bool_type;
 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool_IMPL(container, key, key_size);
 
 return MJSObject_InsertFromPool_IMPL(container, key_index, key_size, &bool_type);
}


static MJS_HOT int MJSObject_GetBoolean_IMPL(MJSObject *container, const char *key) {
 MJSDynamicType *type = MJSObject_Get_IMPL(container, key, strlen(key));
 assert(type != NULL && "not found");
 assert(type->type == MJS_TYPE_BOOLEAN && "not a boolean");
 return type->value_boolean.value;
}


static MJS_HOT int MJSObject_InsertArray_IMPL(MJSObject *container, const char *key, MJSArray *arr) {
 MJSDynamicType arr_type;
 arr_type.type = MJS_TYPE_ARRAY;
 arr_type.value_array = *arr;
 const unsigned int key_size = (unsigned int)strlen(key);
 const unsigned int key_index = MJSObject_AddToStringPool_IMPL(container, key, key_size);
 
 return MJSObject_InsertFromPool_IMPL(container, key_index, key_size, &arr_type);
}


static MJS_HOT MJSArray* MJSObject_GetArray_IMPL(MJSObject *container, const char *key) {
 MJSDynamicType *type = MJSObject_Get_IMPL(container, key, strlen(key));
 assert(type != NULL && "not found");
 assert(type->type == MJS_TYPE_ARRAY && "not an array");
 return &type->value_array;
}



/*-----------------MJSParser-------------------*/
/*
 allocate MJSParserData, return 0 if sucess, return -1 if not.
*/
static MJS_HOT int MJSParserData_Init_IMPL(MJSParsedData *parsed_data) {
 memset(parsed_data, 0, sizeof(MJSParsedData));
 parsed_data->cache = (unsigned char*)__aligned_alloc(MJS_MAX_CACHE_BYTES);
 if(MJS_Unlikely(!parsed_data->cache))
  return MJS_RESULT_ALLOCATION_FAILED;
 parsed_data->cache_allocated_size = MJS_MAX_CACHE_BYTES;

 return 0;
}

/*
 destroy MJSParserData, return 0 if success, return -1 if not.
*/
static MJS_HOT int MJSParserData_Destroy_IMPL(MJSParsedData *parsed_data) {
 int result;
 result = MJSObject_Destroy(&parsed_data->container);
 if(MJS_Unlikely(result)) 
  return result;
 __aligned_dealloc(parsed_data->cache);
 return 0;
}


static MJS_HOT int MJSParserData_ExpandCache_IMPL(MJSParsedData *parsed_data) {
 parsed_data->cache = (unsigned char*)__aligned_realloc(parsed_data->cache, (parsed_data->cache_allocated_size + MJS_MAX_RESERVE_BYTES));
 if(MJS_Unlikely(!parsed_data->cache))
  return MJS_RESULT_ALLOCATION_FAILED;
 parsed_data->cache_allocated_size += MJS_MAX_RESERVE_BYTES;
 return 0;
}


/*-----------------MJSOutputStreamBuffer_Init-------------------*/
static MJS_HOT int MJSOutputStreamBuffer_Init_IMPL(MJSOutputStreamBuffer *buff, unsigned char mode, FILE* fp) {
 memset(buff, 0, sizeof(MJSOutputStreamBuffer));
 
 buff->mode = mode;
 
 switch(mode) {
  case MJS_WRITE_TO_MEMORY_BUFFER:
   buff->buff_reserve = MJS_MAX_RESERVE_BYTES;
   buff->buff = (char*)__aligned_alloc(MJS_MAX_RESERVE_BYTES);
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
 buff->cache = (char*)__aligned_alloc(MJS_MAX_RESERVE_BYTES);
 if(MJS_Unlikely(!buff->cache))
  return MJS_RESULT_ALLOCATION_FAILED;
 return 0;
}


static MJS_HOT int MJSOutputStreamBuffer_Destroy_IMPL(MJSOutputStreamBuffer *buff) {
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


static MJS_HOT int MJSOutputStreamBuffer_Write_IMPL(MJSOutputStreamBuffer *buff, char *arr, unsigned int arr_size) {
 unsigned int elements;
 
 switch(buff->mode) {
  case MJS_WRITE_TO_MEMORY_BUFFER:
  
   /* maintain overall proper null terminated value via strncpy */
   if((arr_size+1) > buff->buff_reserve) {
    buff->buff = (char*)__aligned_realloc(buff->buff, sizeof(char) * (buff->buff_size + buff->buff_reserve + MJS_MAX_RESERVE_BYTES));
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
    return MJS_RESULT_UNSUCCESSFUL_IO_WRITE;
  break;
  default:
   return MJS_RESULT_INVALID_WRITE_MODE;
  break;
 }
 return 0;
}


static MJS_HOT int MJSOutputStreamBuffer_Flush_IMPL(MJSOutputStreamBuffer *buff) {
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


static MJS_HOT int MJSOutputStreamBuffer_ExpandCache_IMPL(MJSOutputStreamBuffer *buff) {  
 buff->cache = (char*)__aligned_realloc(buff->cache, (buff->cache_allocated_size + MJS_MAX_RESERVE_BYTES));
 if(MJS_Unlikely(!buff->cache))
  return MJS_RESULT_ALLOCATION_FAILED;
 buff->cache_allocated_size += MJS_MAX_RESERVE_BYTES;
 return 0;
}


#endif

