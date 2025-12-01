#include "micro_json/object.h"
#include <string.h>
#include <stdlib.h>


/*-----------------Static func-------------------*/
/*
static int fast_abs32(int x) {
 int mask = x >> 31;
 return (x ^ mask) - mask;
}*/

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

static void __aligned_dealloc(void* ptr) {
	free(((void**)ptr)[-1]);
}


/*-----------------MJSArray-------------------*/
/*
 allocate MJSArray object, return 0 if success, return -1 if not.
*/
int MJSArray_Init(MJSArray *arr) {
 if(!arr)
  return -1;
 memset(arr, 0x00, sizeof(MJSArray));
 arr->dynamic_type_ptr = (MJSDynamicType*)__aligned_alloc(sizeof(MJSDynamicType) * MJS_MAX_RESERVE_ELEMENTS, MJS_OPTIMAL_ALIGNMENT);
 if(!arr->dynamic_type_ptr)
  return -1;
 arr->reserve = MJS_MAX_RESERVE_ELEMENTS;
 return 0;
}

/*
 destroy MJSArray object, return 0 if success, return -1 if not.
*/
int MJSArray_Destroy(MJSArray *arr) {
 if(!arr)
  return -1;
 /*
  delete them first, to make sure no memory leaks
 */
 unsigned int i;
 for(i = 0; i < arr->size; i++) {
  switch(arr->dynamic_type_ptr[i].type) {
   case MJS_TYPE_ARRAY:
    if(MJSArray_Destroy(&arr->dynamic_type_ptr[i].value_array)) return -1;
   break;
   case MJS_TYPE_OBJECT:
    if(MJSObject_Destroy(&arr->dynamic_type_ptr[i].value_object)) return -1;
   break;
   case MJS_TYPE_STRING:
   case MJD_TYPE_BOOLEAN:
   case MJS_TYPE_NULL:
   case MJS_TYPE_NUMBER_INT:
   case MJS_TYPE_NUMBER_FLOAT:
   case 0xFF:
   case 0:
   break;
   default:
    return -1;
   break;
  }
 }
 if(arr->dynamic_type_ptr)
 __aligned_dealloc(arr->dynamic_type_ptr);
 return 0;
}

/*
 add to MJSArray object, return 0 if success, return -1 if not.
*/
int MJSArray_Add(MJSArray *arr, MJSDynamicType *value) {
 if(!arr)
  return -1;
 if(arr->reserve > 0) {
  arr->dynamic_type_ptr[arr->size] = *value;
  arr->reserve--;
  arr->size++;
 } else {
  MJSDynamicType *new_ptr = (MJSDynamicType*)__aligned_alloc(sizeof(MJSDynamicType) * (arr->size + MJS_MAX_RESERVE_ELEMENTS), MJS_OPTIMAL_ALIGNMENT);
  
  if(!new_ptr)
   return -1;
   
  memcpy(new_ptr, arr->dynamic_type_ptr, sizeof(MJSDynamicType) * arr->size);
  __aligned_dealloc(arr->dynamic_type_ptr);
  arr->dynamic_type_ptr = new_ptr;
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
  return -1;
 memset(container, 0x00, sizeof(MJSObject)); 
 container->obj_pair_ptr = (MJSObjectPair*)__aligned_alloc(sizeof(MJSObjectPair) * MJS_MAX_RESERVE_ELEMENTS, MJS_OPTIMAL_ALIGNMENT);
 if(!container->obj_pair_ptr)
  return -1;
 memset(container->obj_pair_ptr, 0xFF, MJS_MAX_RESERVE_ELEMENTS * sizeof(MJSObjectPair));
 
 container->reserve = MJS_MAX_HASH_BUCKETS + MJS_MAX_RESERVE_ELEMENTS;
 container->allocated_size = MJS_MAX_HASH_BUCKETS + MJS_MAX_RESERVE_ELEMENTS;
 container->next_empty = MJS_MAX_HASH_BUCKETS;
 container->string_pool = (char*)__aligned_alloc(sizeof(char) * MJS_MAX_RESERVE_BYTES, MJS_OPTIMAL_ALIGNMENT);
 if(!container->string_pool)
  return -1;
 container->string_pool_reserve = MJS_MAX_RESERVE_BYTES;
 return 0;
}


int MJSObject_Destroy(MJSObject *container) {
 if(!container)
  return -1;
 /* destroy other allocated memory first. */
 unsigned int i;
 for(i = 0; i < container->allocated_size; i++) {
  switch(container->obj_pair_ptr[i].value.type) {
   case MJS_TYPE_ARRAY:
    if(MJSArray_Destroy(&container->obj_pair_ptr[i].value.value_array)) return -1;
   break;
   case MJS_TYPE_OBJECT:
    if(MJSObject_Destroy(&container->obj_pair_ptr[i].value.value_object)) return -1;
   break;
   case MJS_TYPE_STRING:
   case MJD_TYPE_BOOLEAN:
   case MJS_TYPE_NULL:
   case MJS_TYPE_NUMBER_INT:
   case MJS_TYPE_NUMBER_FLOAT:
   case 0xFF:
   case 0:
   break;
   default:
    return -1;
   break;
  }
 }
 if(container->string_pool)
  __aligned_dealloc(container->string_pool);
 if(container->obj_pair_ptr)
  __aligned_dealloc(container->obj_pair_ptr);
 return 0;
}


unsigned int MJSObject_GetSize(MJSObject *container) {
 if(!container)
  return 0xFFFFFFFF;
 return container->obj_pair_size;
}



int MJSObject_InsertFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size, MJSDynamicType *value) {
 if(!container || (pool_index > container->string_pool_size) || !str_size)
  return -1;
 char *key = &container->string_pool[pool_index];
 unsigned int str_pool_index = pool_index;
 MJSObjectPair pair;
 
 memset(&pair, 0xFF, sizeof(MJSObjectPair));
 pair.key_pool_index = str_pool_index;
 pair.key_pool_size = str_size;
 pair.value = *value;

	const unsigned int hash_index = generate_hash_index(key, str_size);
	
	if(container->reserve == 0) {
	 MJSObjectPair *new_obj = (MJSObjectPair*)__aligned_alloc((container->allocated_size + MJS_MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair), MJS_OPTIMAL_ALIGNMENT);
	 if(!new_obj)
	  return -1;
	 memcpy(new_obj, container->obj_pair_ptr, container->allocated_size * sizeof(MJSObjectPair));
	 memset(&new_obj[container->allocated_size], 0xFF, sizeof(MJSObjectPair) * MJS_MAX_RESERVE_ELEMENTS);
	 
	 container->allocated_size += MJS_MAX_RESERVE_ELEMENTS;
	 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
	 __aligned_dealloc(container->obj_pair_ptr);
	 container->obj_pair_ptr = new_obj;
	}
	
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];
	
	if(start_node->key_pool_index == 0xFFFFFFFF) {
		*start_node = pair;
	} else {
	 unsigned int prev_index = hash_index;
  unsigned int next_index = hash_index;
 	do {
 		prev_index = next_index;
	 	
   if(start_node->next == 0xFFFFFFFF) {
			 prev_index = next_index;
		 	next_index = container->next_empty++;
		 	if(next_index == 0xFFFFFFFF)
		 	 return -1;
		 	container->obj_pair_ptr[next_index] = pair;
		  container->obj_pair_ptr[prev_index].next = next_index;
	 	 break;
	 	}
	 	
		 next_index = start_node->next;
	  start_node = &container->obj_pair_ptr[next_index];
 	} while(1);
	}
	container->reserve--;
	container->obj_pair_size++;
	
 return 0;
}


int MJSObject_Insert(MJSObject *container, const char *key, unsigned int str_size, MJSDynamicType *value) {
 if(!container || !key || !str_size)
  return -1;
 
 unsigned int str_pool_index = MJSObject_AddToStringPool(container, key, str_size);
 MJSObjectPair pair;
 
 memset(&pair, 0x00, sizeof(MJSObjectPair));
 pair.key_pool_index = str_pool_index;
 pair.key_pool_size = str_size;
 pair.value = *value;
 
	const unsigned int hash_index = generate_hash_index(key, str_size);
	
	if(container->reserve == 0) {
	 MJSObjectPair *new_obj = (MJSObjectPair*)__aligned_alloc((container->allocated_size + MJS_MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair), MJS_OPTIMAL_ALIGNMENT);
	 if(!new_obj)
	  return -1;
	 memcpy(new_obj, container->obj_pair_ptr, container->allocated_size * sizeof(MJSObjectPair));
	 memset(&new_obj[container->allocated_size], 0xFF, sizeof(MJSObjectPair) * MJS_MAX_RESERVE_ELEMENTS);

	 container->allocated_size += MJS_MAX_RESERVE_ELEMENTS;
	 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
	 __aligned_dealloc(container->obj_pair_ptr);
	 container->obj_pair_ptr = new_obj;
	}
			
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];
	
	if(start_node->key_pool_index == 0xFFFFFFFF) {
		*start_node = pair;
	} else {
	 unsigned int prev_index = hash_index;
  unsigned int next_index = hash_index;
 	do {
 		prev_index = next_index;
	 	
   if(start_node->next == 0xFFFFFFFF) {
			 prev_index = next_index;
		 	next_index = container->next_empty++;
		 	if(next_index == 0xFFFFFFFF)
		 	 return -1;
		 	container->obj_pair_ptr[next_index] = pair;
		  container->obj_pair_ptr[prev_index].next = next_index;
	 	 break;
	 	}
	 	
		 next_index = start_node->next;
	  start_node = &container->obj_pair_ptr[next_index];
 	} while(1);

	}
	container->reserve--;
	container->obj_pair_size++;
	
 return 0;
}


MJSObjectPair* MJSObject_GetPairReference(MJSObject *container, const char *key) {
 if(!container || !key)
  return NULL;
  
 const unsigned int key_len = (unsigned int)strlen(key);
 const unsigned int hash_index = generate_hash_index(key, key_len);
 
 if(!container->obj_pair_size)
  return NULL;
  
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];
	
	if(start_node->key_pool_index == 0xFFFFFFFF)
	 return NULL;
	else {
  unsigned int next_index = hash_index;
 	do {
 	
 	 if(key_len == start_node->key_pool_size) {
    if(memcmp(key, &container->string_pool[start_node->key_pool_index], key_len) == 0) {
	 	  return start_node;
	 	 }
 	 }
		 next_index = start_node->next;
	 	start_node = &container->obj_pair_ptr[start_node->next];
 	} while(1);
	}
 return NULL;
}


MJSObjectPair* MJSObject_GetPairReferenceFromPool(MJSObject *container, unsigned int pool_index, unsigned int str_size) {
 if(!container || (pool_index > container->string_pool_size))
  return NULL;

 char *key = &container->string_pool[pool_index];
 const unsigned int key_len = str_size;
 const unsigned int hash_index = generate_hash_index(key, key_len);
 
 if(!container->obj_pair_size)
  return NULL;
  
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];
	
	if(start_node->key_pool_index == 0xFFFFFFFF)
	 return NULL;
 else {
  unsigned int next_index = hash_index;
 	do {
 	
 	 if(key_len == start_node->key_pool_size) {
    if(memcmp(key, &container->string_pool[start_node->key_pool_index], key_len) == 0) {
	 	  return start_node;
	 	 }
 	 }
		 next_index = start_node->next;
	 	start_node = &container->obj_pair_ptr[start_node->next];
 	} while(1);
	}
 return NULL;
}



unsigned int MJSObject_AddToStringPool(MJSObject *container, const char *str, unsigned int str_size) {
 if(!container || !str || !str_size)
  return 0xFFFFFFFF;
  
 unsigned int out_index = 0xFFFFFFFF;
 
 if(container->string_pool_reserve > (str_size+1)) {
  memcpy(&container->string_pool[container->string_pool_size], str, sizeof(char) * str_size);
  out_index = container->string_pool_size;
  container->string_pool_size += str_size;
  container->string_pool_reserve -= str_size;
  /* null terminatior */
  container->string_pool[container->string_pool_size++] = '\0';
  return out_index;
 } else {
  char *new_str = (char*)__aligned_alloc(sizeof(char) * (container->string_pool_size + (str_size+1) + MJS_MAX_RESERVE_ELEMENTS), MJS_OPTIMAL_ALIGNMENT);
  if(!new_str)
   return 0xFFFFFFFF;
  memcpy(new_str, container->string_pool, sizeof(char) * container->string_pool_size);
  __aligned_dealloc(container->string_pool);
  container->string_pool = new_str;
  memcpy(&container->string_pool[container->string_pool_size], str, sizeof(char) * str_size);
  out_index = container->string_pool_size;
  container->string_pool_size += str_size;
  container->string_pool_reserve = MJS_MAX_RESERVE_ELEMENTS;
  /* null terminatior */
  container->string_pool[container->string_pool_size++] = '\0';
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
 
 /* agressive loop unrolling*/
 while((i+8) < str_size) {
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
 }

 while((i+4) < str_size) {
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
 }
 
 while((i+2) < str_size) {
  index += (unsigned int)str[i++];
  index += (unsigned int)str[i++];
 }
 
 while(i < str_size) 
  index += (unsigned int)str[i++];
 return index % MJS_MAX_HASH_BUCKETS;
}


/*-----------------MJSParser-------------------*/
/*
 allocate MJSParserData, return 0 if sucess, return -1 if not.
*/
int MJSParserData_Create(MJSParsedData *parsed_data) {
 if(!parsed_data)
  return -1;
 
 memset(parsed_data, 0, sizeof(MJSParsedData));
 parsed_data->cache = (unsigned char*)__aligned_alloc(sizeof(unsigned char) * MJS_MAX_CACHE_BYTES, MJS_OPTIMAL_ALIGNMENT);
 parsed_data->cache_allocated_size = MJS_MAX_CACHE_BYTES;
 
 if(!parsed_data->cache)
  return -1;
 return 0;
}

/*
 destroy MJSParserData, return 0 if success, return -1 if not.
*/
int MJSParserData_Destroy(MJSParsedData *parsed_data) {
 if(!parsed_data)
  return -1;
 if(MJSObject_Destroy(&parsed_data->container))
  return -1;
 if(parsed_data->cache)
 __aligned_dealloc(parsed_data->cache);
 return 0;
}


int MJSParserData_ExpandCache(MJSParsedData *parsed_data) {
 if(!parsed_data)
  return -1;
 unsigned char *new_cache = (unsigned char*)__aligned_alloc(sizeof(unsigned char) * (parsed_data->cache_allocated_size + MJS_MAX_RESERVE_BYTES), MJS_OPTIMAL_ALIGNMENT);
 
 if(!new_cache)
  return -1;
  
 memcpy(new_cache, parsed_data->cache, sizeof(unsigned char) * parsed_data->cache_size);
 __aligned_dealloc(parsed_data->cache);
 parsed_data->cache = new_cache;
 parsed_data->cache_allocated_size += MJS_MAX_RESERVE_BYTES;
 return 0;
}

