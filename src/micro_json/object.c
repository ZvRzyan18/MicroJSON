#include "micro_json/object.h"
#include <string.h>
#include <stdlib.h>


/*-----------------Static func-------------------*/

/*
 guaranteed alignment of memory
*/
static void* __aligned_alloc(unsigned int m_size, unsigned char alignment) {
	void *p1, **p2;
	unsigned long long offset = alignment - 1 + sizeof(void*);
 p1 = malloc(m_size + offset);
 p2 = (void**)(((unsigned long long)p1 + offset) & ~(alignment - 1));
 p2[-1] = p1;
 return p2;
}

static void* __aligned_realloc(void *ptr, unsigned int m_size, unsigned char alignment) {
	if(!ptr)
	 return __aligned_alloc(m_size, alignment);
	void *p1, **p2;
	unsigned long long offset = alignment - 1 + sizeof(void*);
 p1 = realloc(((void**)ptr)[-1], m_size + offset);
 if(!p1)
  return NULL;
 p2 = (void**)(((unsigned long long)p1 + offset) & ~(alignment - 1));
 p2[-1] = p1;
 return p2;
}

static void __aligned_dealloc(void* ptr) {
	if(!ptr) 
	 return;
	free(((void**)ptr)[-1]);
}


/*-----------------MJSArray-------------------*/
/*
 allocate MJSArray object, return 0 if success, return -1 if not.
*/
int MJSArray_Init(MJSArray *arr) {
 if(!arr)
  return MJS_RESULT_NULL_POINTER;
 memset(arr, 0x00, sizeof(MJSArray));
 arr->type = MJS_TYPE_ARRAY;
 arr->dynamic_type_ptr = (MJSDynamicType*)__aligned_alloc(sizeof(MJSDynamicType) * MJS_MAX_RESERVE_ELEMENTS, MJS_OPTIMAL_ALIGNMENT);
 if(!arr->dynamic_type_ptr)
  return MJS_RESULT_ALLOCATION_FAILED;
 arr->reserve = MJS_MAX_RESERVE_ELEMENTS;
 return 0;
}

/*
 destroy MJSArray object, return 0 if success, return -1 if not.
*/
int MJSArray_Destroy(MJSArray *arr) {
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
    if(result) return result;
   break;
   case MJS_TYPE_OBJECT:
    result = MJSObject_Destroy(&arr->dynamic_type_ptr[i].value_object);
    if(result) return result;
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
int MJSArray_Add(MJSArray *arr, MJSDynamicType *value) {
 if(!arr)
  return MJS_RESULT_NULL_POINTER;
 if(arr->reserve > 0) {
  arr->dynamic_type_ptr[arr->size] = *value;
  arr->reserve--;
  arr->size++;
 } else {

  arr->dynamic_type_ptr = (MJSDynamicType*)__aligned_realloc(arr->dynamic_type_ptr, sizeof(MJSDynamicType) * (arr->size + MJS_MAX_RESERVE_ELEMENTS), MJS_OPTIMAL_ALIGNMENT);
  if(!arr->dynamic_type_ptr)
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
MJSDynamicType* MJSArray_Get(MJSArray *arr, unsigned int index) {
 if(!arr)
  return NULL;
 return &arr->dynamic_type_ptr[index];
}

/*
 get size from MJSArray object, return size if success, return 0xFFFFFFFF if not.
*/
unsigned int MJSArray_Size(MJSArray *arr) {
 if(!arr)
  return 0xFFFFFFFF;
 return arr->size;
}


/*-----------------MJSContainer-------------------*/
static unsigned int generate_hash_index(const char *str, unsigned int str_size);


int MJSObject_Init(MJSObject *container) {
 if(!container)
  return MJS_RESULT_NULL_POINTER;
 memset(container, 0x00, sizeof(MJSObject));
 const unsigned int pre_allocated_pair = sizeof(MJSObjectPair) * (MJS_MAX_HASH_BUCKETS + MJS_MAX_RESERVE_ELEMENTS);
 container->obj_pair_ptr = (MJSObjectPair*)__aligned_alloc(pre_allocated_pair, MJS_OPTIMAL_ALIGNMENT);
 if(!container->obj_pair_ptr)
  return MJS_RESULT_ALLOCATION_FAILED;
 memset(container->obj_pair_ptr, 0xFF, pre_allocated_pair);
 container->type = MJS_TYPE_OBJECT;
 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
 container->next_empty = MJS_MAX_HASH_BUCKETS;
 container->string_pool = (char*)__aligned_alloc(sizeof(char) * MJS_MAX_RESERVE_BYTES, MJS_OPTIMAL_ALIGNMENT);
 if(!container->string_pool)
  return MJS_RESULT_ALLOCATION_FAILED;
 container->string_pool_reserve = MJS_MAX_RESERVE_BYTES;
 return 0;
}


int MJSObject_Destroy(MJSObject *container) {
 if(!container)
  return MJS_RESULT_NULL_POINTER;
 /* destroy other allocated memory first. */
 int result;
 unsigned int i;
 unsigned int max_size = container->obj_pair_size + container->reserve + MJS_MAX_HASH_BUCKETS;
 for(i = 0; i < max_size; i++) {
  switch(container->obj_pair_ptr[i].value.type) {
   case MJS_TYPE_ARRAY:
    result = MJSArray_Destroy(&container->obj_pair_ptr[i].value.value_array);
    if(result) return result;
   break;
   case MJS_TYPE_OBJECT:
    result = MJSObject_Destroy(&container->obj_pair_ptr[i].value.value_object);
    if(result) return result;
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



int MJSObject_InsertFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size, MJSDynamicType *value) {
 if(!container || (pool_index > container->string_pool_size))
  return MJS_RESULT_NULL_POINTER;

 if(!str_size)
  return MJS_RESULT_EMPTY_KEY;
  
 char *key = &container->string_pool[pool_index];
 unsigned int str_pool_index = pool_index;
 MJSObjectPair pair;
 
 pair.key_pool_index = str_pool_index;
 pair.key_pool_size = str_size;
 pair.next = 0xFFFFFFFF;
 pair.value = *value;

	const unsigned int hash_index = generate_hash_index(key, str_size);
	if(container->reserve == 0) {
	 unsigned int max_size = container->reserve + container->obj_pair_size + MJS_MAX_HASH_BUCKETS;

  container->obj_pair_ptr = (MJSObjectPair*)__aligned_realloc(container->obj_pair_ptr, (max_size + MJS_MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair), MJS_OPTIMAL_ALIGNMENT);
  if(!container->obj_pair_ptr)
   return MJS_RESULT_ALLOCATION_FAILED;
		memset(&container->obj_pair_ptr[max_size], 0xFF, MJS_MAX_RESERVE_ELEMENTS * sizeof(MJSObjectPair));
	 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
	}
 
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];

	if(start_node->key_pool_index == 0xFFFFFFFF) {
		*start_node = pair;
	} else {

	 unsigned int prev_index = hash_index;
  unsigned int next_index = hash_index;
 	do {
 	 start_node = &container->obj_pair_ptr[next_index];
 		prev_index = next_index;

 	 if(str_size == start_node->key_pool_size) {
    if(memcmp(key, &container->string_pool[start_node->key_pool_index], str_size) == 0) {
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


int MJSObject_Insert(MJSObject *container, const char *key, unsigned int str_size, MJSDynamicType *value) {
 if(!container || !key || !str_size)
  return MJS_RESULT_NULL_POINTER;
 
 if(!key[0]) /* empty key */
  return MJS_RESULT_EMPTY_KEY;
 
 unsigned int str_pool_index = MJSObject_AddToStringPool(container, key, str_size);
 MJSObjectPair pair;
 
 pair.key_pool_index = str_pool_index;
 pair.key_pool_size = str_size;
 pair.next = 0xFFFFFFFF;
 pair.value = *value;

	const unsigned int hash_index = generate_hash_index(key, str_size);
	
	if(container->reserve == 0) {
	 unsigned int max_size = container->reserve + container->obj_pair_size + MJS_MAX_HASH_BUCKETS;

  container->obj_pair_ptr = (MJSObjectPair*)__aligned_realloc(container->obj_pair_ptr, (max_size + MJS_MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair), MJS_OPTIMAL_ALIGNMENT);
  if(!container->obj_pair_ptr)
   return MJS_RESULT_ALLOCATION_FAILED;
		memset(&container->obj_pair_ptr[max_size], 0xFF, MJS_MAX_RESERVE_ELEMENTS * sizeof(MJSObjectPair));
	 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
	}
 
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];

	if(start_node->key_pool_index == 0xFFFFFFFF) {
		*start_node = pair;
	} else {
	 unsigned int prev_index = hash_index;
  unsigned int next_index = hash_index;
 	do {
 	 start_node = &container->obj_pair_ptr[next_index];
 		prev_index = next_index;

 	 if(str_size == start_node->key_pool_size) {
    if(memcmp(key, &container->string_pool[start_node->key_pool_index], str_size) == 0) {
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


MJSObjectPair* MJSObject_GetPairReference(MJSObject *container, const char *key, unsigned int str_size) {
 if(!container || !key)
  return NULL;
  
 const unsigned int key_len = str_size;
 const unsigned int hash_index = generate_hash_index(key, key_len);

	MJSObjectPair *start_node = NULL;

  unsigned int next_index = hash_index;
  do {
	 	start_node = &container->obj_pair_ptr[next_index];
 	 if(key_len == start_node->key_pool_size) {
    if(strncmp(key, &container->string_pool[start_node->key_pool_index], key_len) == 0) {
	 	  return start_node;
	 	 }
 	 }
		 next_index = start_node->next;
 	} while(next_index != 0xFFFFFFFF);
 return NULL;
}


MJSObjectPair* MJSObject_GetPairReferenceFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size) {
 if(!container || (pool_index > container->string_pool_size))
  return NULL;

 char *key = &container->string_pool[pool_index];
 const unsigned int key_len = str_size;
 const unsigned int hash_index = generate_hash_index(key, key_len);
   
	MJSObjectPair *start_node = NULL;

  unsigned int next_index = hash_index;
 	do {
	 	start_node = &container->obj_pair_ptr[next_index];

 	 if(key_len == start_node->key_pool_size) {
    if(strncmp(key, &container->string_pool[start_node->key_pool_index], key_len) == 0) {
	 	  return start_node;
	 	 }
 	 }
		 next_index = start_node->next;
 	} while(next_index != 0xFFFFFFFF);

 return NULL;
}



unsigned int MJSObject_AddToStringPool(MJSObject *container, const char *str, unsigned int str_size) {
 if(!container || !str || !str_size)
  return 0xFFFFFFFF;
 
 unsigned int out_index = 0xFFFFFFFF;
 
 if(container->string_pool_reserve > (str_size+1)) {
  strncpy(&container->string_pool[container->string_pool_size], str, sizeof(char) * str_size);
  out_index = container->string_pool_size;
  container->string_pool_size += str_size+1;
  container->string_pool_reserve -= str_size+1;
  return out_index;
 } else {
  /*
   allocate a new reserve bytes
  */
  container->string_pool = (char*)__aligned_realloc(container->string_pool, sizeof(char) * (container->string_pool_size + (str_size+1) + MJS_MAX_RESERVE_ELEMENTS), MJS_OPTIMAL_ALIGNMENT);
  if(!container->string_pool)
   return 0xFFFFFFFF;
  /*
   concat the input string into the pool.
  */
  strncpy(&container->string_pool[container->string_pool_size], str, sizeof(char) * str_size);
  out_index = container->string_pool_size;
  container->string_pool_size += str_size+1;
  container->string_pool_reserve = MJS_MAX_RESERVE_ELEMENTS;
  return out_index;
 }
 return out_index;
}


const char* MJSObject_GetStringFromPool(MJSObject *container, MJSString *str) {
 if(!container || !str)
  return NULL;
 return &container->string_pool[str->pool_index];
}

/*-----------------Static func-------------------*/
/*
 Hash Function
 TODO : optimize this, mininize loop branches
 or use agressive unrolling
*/
static unsigned int generate_hash_index(const char *str, unsigned int str_size) {
 unsigned int index = 0;
 unsigned int i = 0;
 const unsigned int m = str_size < 8 ? str_size : 8;
 
 while(i < m) index += (unsigned int)str[i++];
 
 return (index << 1) % MJS_MAX_HASH_BUCKETS;
}


/*-----------------MJSParser-------------------*/
/*
 allocate MJSParserData, return 0 if sucess, return -1 if not.
*/
int MJSParserData_Init(MJSParsedData *parsed_data) {
 if(!parsed_data)
  return MJS_RESULT_NULL_POINTER;
 
 memset(parsed_data, 0, sizeof(MJSParsedData));
 parsed_data->cache = (unsigned char*)__aligned_alloc(sizeof(unsigned char) * MJS_MAX_CACHE_BYTES, MJS_OPTIMAL_ALIGNMENT);
 if(!parsed_data->cache)
  return MJS_RESULT_ALLOCATION_FAILED;
 parsed_data->cache_allocated_size = MJS_MAX_CACHE_BYTES;

 return 0;
}

/*
 destroy MJSParserData, return 0 if success, return -1 if not.
*/
int MJSParserData_Destroy(MJSParsedData *parsed_data) {
 if(!parsed_data)
  return MJS_RESULT_NULL_POINTER;
 int result;
 result = MJSObject_Destroy(&parsed_data->container);
 if(result) 
  return result;
 __aligned_dealloc(parsed_data->cache);
 return 0;
}


int MJSParserData_ExpandCache(MJSParsedData *parsed_data) {
 if(!parsed_data)
  return MJS_RESULT_NULL_POINTER;
  
 parsed_data->cache = (unsigned char*)__aligned_realloc(parsed_data->cache, sizeof(unsigned char) * (parsed_data->cache_allocated_size + MJS_MAX_RESERVE_BYTES), MJS_OPTIMAL_ALIGNMENT);
 if(!parsed_data->cache)
  return MJS_RESULT_ALLOCATION_FAILED;
 parsed_data->cache_allocated_size += MJS_MAX_RESERVE_BYTES;
 return 0;
}

