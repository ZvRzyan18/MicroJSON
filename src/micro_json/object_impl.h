#ifndef OBJECT_IMPL_H
#define OBJECT_IMPL_H

#include "micro_json/object.h"
#include "micro_json/object_impl.h"
#include <string.h>
#include <stdlib.h>

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


/*-----------------String Pool-------------------*/

static MJS_COLD int MJSStringPool_Init_IMPL(MJSStringPool *pool) {
 pool->root = (MJSStringPoolNode*)__aligned_alloc(sizeof(MJSStringPoolNode) * MJS_MAX_POOL_CHUNK_NODE);
 pool->node_size = 1;
 pool->node_reserve = MJS_MAX_POOL_CHUNK_NODE;
 if(MJS_Unlikely(!pool->root))
  return MJS_RESULT_ALLOCATION_FAILED;
 pool->root[0].str = (char*)__aligned_alloc(MJS_MAX_POOL_ALLOCATION_BYTES);
 pool->root[0].pool_size = 0;
 pool->root[0].pool_reserve = MJS_MAX_POOL_ALLOCATION_BYTES;
 return 0;
}


static MJS_COLD int MJSStringPool_Destroy_IMPL(MJSStringPool *pool) {
 unsigned int i;
 for(i = 0; i < pool->node_size; i++) {
  __aligned_dealloc(pool->root[i].str);
 }
 __aligned_dealloc(pool->root);
 return 0;
}


static MJS_HOT unsigned short MJSStringPool_GetCurrentNode_IMPL(MJSStringPool *pool) {
 unsigned short i;
 MJSStringPoolNode *curr = NULL;
 i = pool->node_size - 1;
 curr = &pool->root[i];
 
 if(curr->pool_size < MJS_MAX_POOL_MEMORY_THRESHOLD)
  return i;
 
 if(MJS_Unlikely(!pool->node_reserve)) {
  pool->root = (MJSStringPoolNode*)__aligned_realloc(pool->root, sizeof(MJSStringPoolNode) * (pool->node_size + MJS_MAX_POOL_CHUNK_NODE));
  if(MJS_Unlikely(!pool->root))
   return 0xFFFF;
  pool->node_reserve = MJS_MAX_POOL_CHUNK_NODE;
 }
 curr = &pool->root[pool->node_size];
 i = pool->node_size;
 curr->str = (char*)__aligned_alloc(MJS_MAX_POOL_ALLOCATION_BYTES);
 if(MJS_Unlikely(!curr->str))
  return 0xFFFF;
 curr->pool_size = 0;
 curr->pool_reserve = MJS_MAX_POOL_ALLOCATION_BYTES;
 
 pool->node_size++;
 pool->node_reserve--;
 return i;
}


static MJS_HOT int MJSStringPool_ExpandNode_IMPL(MJSStringPoolNode *node, unsigned int additional_size) {
 node->str = (char*)__aligned_realloc(node->str, node->pool_size + node->pool_reserve + additional_size);
 node->pool_reserve += additional_size;
 return !node->str * MJS_RESULT_ALLOCATION_FAILED;
}


static MJS_HOT int MJSStringPool_AddToPool_IMPL(MJSStringPool *pool, const char *str, unsigned int str_size, unsigned int *out_index, unsigned short *out_chunk_index) {
 int result = 0;
 *out_chunk_index = MJSStringPool_GetCurrentNode_IMPL(pool);
 MJSStringPoolNode *node = &pool->root[*out_chunk_index];
 
 *out_index = node->pool_size;
 if(MJS_Unlikely(node->pool_reserve <= str_size))
  result = MJSStringPool_ExpandNode_IMPL(node, str_size - node->pool_reserve + 1);

 memcpy(&node->str[node->pool_size], str, str_size);
 node->pool_size += str_size;
 node->str[node->pool_size++] = '\0';
 return result;
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
static MJS_COLD int MJSArray_Destroy_IMPL(MJSArray *arr) {
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
MJS_HOT static int MJSArray_Add_IMPL(MJSArray *arr, MJSDynamicType *value) {
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
MJS_INLINE MJSDynamicType* MJSArray_Get_IMPL(MJSArray *arr, unsigned int index) {
 return &arr->dynamic_type_ptr[index];
}

/*
 get size from MJSArray object, return size if success, return 0xFFFFFFFF if not.
*/
MJS_INLINE unsigned int MJSArray_Size_IMPL(MJSArray *arr) {
 return arr->size;
}


/*-----------------MJSContainer-------------------*/

static MJS_HOT int MJSObject_Init_IMPL(MJSObject *container) {
 int result = 0;
 const unsigned int pre_allocated_pair = sizeof(MJSObjectPair) * (MJS_MAX_HASH_BUCKETS + MJS_MAX_RESERVE_ELEMENTS);
 container->obj_pair_ptr = (MJSObjectPair*)__aligned_alloc(pre_allocated_pair);
 result = !container->obj_pair_ptr * MJS_RESULT_ALLOCATION_FAILED;
 if(MJS_Likely(!result))
 memset(container->obj_pair_ptr, 0xFF, pre_allocated_pair);
 container->type = MJS_TYPE_OBJECT;
 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
 container->obj_pair_size = 0;
 return result;
}


static MJS_COLD int MJSObject_Destroy_IMPL(MJSObject *container) {
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
 __aligned_dealloc(container->obj_pair_ptr);
 return 0;
}



MJS_HOT static int MJSObject_InsertFromPool_IMPL(MJSObject *container, MJSStringPool *pool, unsigned int pool_index, unsigned int str_size, unsigned short pool_chunk_index, MJSDynamicType *value) {
 if(MJS_Unlikely(!str_size))
  return MJS_RESULT_EMPTY_KEY;
  
 const char *key = &pool->root[pool_chunk_index].str[pool_index];
 const unsigned int str_pool_index = pool_index;
 MJSObjectPair pair;
 
 pair.key_pool_index = str_pool_index;
 pair.key_pool_size = str_size;
 pair.chunk_node_index = pool_chunk_index;
 pair.next = 0xFFFFFFFF;
 pair.value = *value;

	const unsigned int hash_index = generate_hash_index(key, str_size);
	if(container->reserve == 0) {
	 unsigned int max_size = container->reserve + container->obj_pair_size + MJS_MAX_HASH_BUCKETS;

  container->obj_pair_ptr = (MJSObjectPair*)__aligned_realloc(container->obj_pair_ptr, (max_size + MJS_MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair));
  if(MJS_Unlikely(!container->obj_pair_ptr))
   return MJS_RESULT_ALLOCATION_FAILED;

		memset(&container->obj_pair_ptr[max_size], 0xFF, MJS_MAX_RESERVE_ELEMENTS * sizeof(MJSObjectPair));
	 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
	}
 
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];

 if(str_size == start_node->key_pool_size) {
  if(!memcmp(key, &pool->root[start_node->chunk_node_index].str[start_node->key_pool_index], str_size)) {
   return MJS_RESULT_DUPLICATE_KEY;
  }
 }
 
	if(start_node->key_pool_index == 0xFFFFFFFF) {
		*start_node = pair;
	} else {

	 unsigned int prev_index = hash_index;
  unsigned int next_index = start_node->next;
 	while(next_index != 0xFFFFFFFF) {
 	 start_node = &container->obj_pair_ptr[next_index];
 		prev_index = next_index;

 	 if(str_size == start_node->key_pool_size) {
    if(!memcmp(key, &pool->root[start_node->chunk_node_index].str[start_node->key_pool_index], str_size)) {
     return MJS_RESULT_DUPLICATE_KEY;
    }
 	 }

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


MJS_INLINE int MJSObject_Insert_IMPL(MJSObject *container, MJSStringPool *pool, const char *key, unsigned int str_size, MJSDynamicType *value) { 
 if(MJS_Unlikely(!key[0])) /* empty key */
  return MJS_RESULT_EMPTY_KEY;
 int result = 0;
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
		memset(&container->obj_pair_ptr[max_size], 0xFF, MJS_MAX_RESERVE_ELEMENTS * sizeof(MJSObjectPair));
	 container->reserve = MJS_MAX_RESERVE_ELEMENTS;
	}
 
	MJSObjectPair *start_node = &container->obj_pair_ptr[hash_index];

 if(str_size == start_node->key_pool_size) {
  if(!memcmp(key, &pool->root[start_node->chunk_node_index].str[start_node->key_pool_index], str_size)) {
   return MJS_RESULT_DUPLICATE_KEY;
  }
 }

	if(start_node->key_pool_index == 0xFFFFFFFF) { 
		result = MJSStringPool_AddToPool_IMPL(pool, key, str_size, &pair.key_pool_index, &pair.chunk_node_index);
		*start_node = pair;
	} else {
	 unsigned int prev_index = hash_index;
  unsigned int next_index = start_node->next;
 	while(next_index != 0xFFFFFFFF) {
 	 start_node = &container->obj_pair_ptr[next_index];
 		prev_index = next_index;

 	 if(str_size == start_node->key_pool_size) {
    if(!memcmp(key, &pool->root[start_node->chunk_node_index].str[start_node->key_pool_index], str_size)) {
     return MJS_RESULT_DUPLICATE_KEY; 
    }
 	 }

		 next_index = start_node->next;
 	}
 	
		result = MJSStringPool_AddToPool_IMPL(pool, key, str_size, &pair.key_pool_index, &pair.chunk_node_index);

		next_index = MJS_MAX_HASH_BUCKETS + container->obj_pair_size;
		
		container->obj_pair_ptr[next_index] = pair;
		container->obj_pair_ptr[prev_index].next = next_index;

		container->reserve--;
		container->obj_pair_size++;
 }
	
 return 0;
}


MJS_INLINE MJSDynamicType* MJSObject_Get_IMPL(MJSObject *container, MJSStringPool *pool, const char *key, unsigned int str_size) {  
 const unsigned int key_len = str_size;
 const unsigned int hash_index = generate_hash_index(key, key_len);

	MJSObjectPair *start_node = NULL;

  unsigned int next_index = hash_index;
  do {
	 	start_node = &container->obj_pair_ptr[next_index];
 	 if(key_len == start_node->key_pool_size) {
    if(!memcmp(key, &pool->root[start_node->chunk_node_index].str[start_node->key_pool_index], key_len)) {
	 	  return &start_node->value;
	 	 }
 	 }
		 next_index = start_node->next;
 	} while(next_index != 0xFFFFFFFF);
 return NULL;
}


MJS_INLINE MJSDynamicType* MJSObject_GetFromPool_IMPL(MJSObject *container, MJSStringPool *pool, unsigned int pool_index, unsigned int str_size, unsigned short pool_chunk_index) {
 const char *key = &pool->root[pool_chunk_index].str[pool_index];
 const unsigned int key_len = str_size;
 const unsigned int hash_index = generate_hash_index(key, key_len);
 
	MJSObjectPair *start_node = NULL;

  unsigned int next_index = hash_index;
 	do {
	 	start_node = &container->obj_pair_ptr[next_index];

 	 if(key_len == start_node->key_pool_size) {
    if(!memcmp(key, &pool->root[start_node->chunk_node_index].str[start_node->key_pool_index], key_len)) {
	 	  return &start_node->value;
	 	 }
 	 }
		 next_index = start_node->next;
 	} while(next_index != 0xFFFFFFFF);

 return NULL;
}


/*-----------------MJSParser-------------------*/
/*
 allocate MJSParserData, return 0 if sucess, return -1 if not.
*/
static MJS_HOT int MJSParserData_Init_IMPL(MJSParsedData *parsed_data) {
 memset(parsed_data, 0, sizeof(MJSParsedData));
 return 0;
}

/*
 destroy MJSParserData, return 0 if success, return -1 if not.
*/
static MJS_HOT int MJSParserData_Destroy_IMPL(MJSParsedData *parsed_data) {
 int result = 0;
 switch(parsed_data->container.type) {
  case MJS_TYPE_ARRAY:
   result = MJSArray_Destroy(&parsed_data->container.value_array);
   if(MJS_Unlikely(result)) return result;
  break;
  case MJS_TYPE_OBJECT:
   result = MJSObject_Destroy(&parsed_data->container.value_object);
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
 return result;
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
   if((arr_size+1) > buff->buff_reserve) {
    buff->buff = (char*)__aligned_realloc(buff->buff, sizeof(char) * (buff->buff_size + buff->buff_reserve + MJS_MAX_RESERVE_BYTES));
    if(MJS_Unlikely(!buff->buff))
     return MJS_RESULT_ALLOCATION_FAILED;
    buff->buff_reserve += MJS_MAX_RESERVE_BYTES;
    memcpy(&buff->buff[buff->buff_size], arr, arr_size);
    buff->buff[buff->buff_size+arr_size] = '\0';

    buff->buff_size += arr_size;
    buff->buff_reserve -= arr_size;
   } else { 
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

