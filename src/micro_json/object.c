#include "micro_json/object.h"
#include <string.h>
#include <stdlib.h>

static int fast_abs32(int x) {
 int mask = x >> 31;
 return (x ^ mask) - mask;
}


//-----------------MJSArray-------------------//
/*
 allocate MJSArray object, return 0 if success, return -1 if not.
*/
int MJSArray_Init(MJSArray *arr) {
 if(!arr)
  return -1;
 memset(arr, 0x00, sizeof(MJSArray));
 arr->dynamic_type_ptr = (MJSDynamicType*)malloc(sizeof(MJSDynamicType) * MAX_RESERVE_ELEMENTS);
 if(!arr->dynamic_type_ptr)
  return -1;
 arr->reserve = MAX_RESERVE_ELEMENTS;
 return 0;
}

/*
 destroy MJSArray object, return 0 if success, return -1 if not.
*/
int MJSArray_Destroy(MJSArray *arr) {
 if(!arr)
  return -1;
 //delete them first, to make sure no memory leaks
 int i;
 for(i = 0; i < arr->size; i++) {
  switch(arr->dynamic_type_ptr[i].type) {
   case MJS_TYPE_ARRAY:
    if(!MJSArray_Destroy(&arr->dynamic_type_ptr[i].value_array)) return -1;
   break;
   case MJS_TYPE_CONTAINER:
    if(!MJSContainer_Destroy(&arr->dynamic_type_ptr[i].value_container)) return -1;
   break;
  }
 }
 free(arr->dynamic_type_ptr);
 return 0;
}

/*
 add to MJSArray object, return 0 if success, return -1 if not.
*/
int MJSArray_Add(MJSArray *arr, MJSDynamicType value) {
 if(!arr)
  return -1;
 if(arr->reserve > 0) {
  arr->dynamic_type_ptr[arr->size] = value;
  arr->reserve--;
  arr->size++;
 } else {
  MJSDynamicType *new_ptr = (MJSDynamicType*)malloc(sizeof(MJSDynamicType) * (arr->size + MAX_RESERVE_ELEMENTS));
  
  if(!new_ptr)
   return -1;
   
  memcpy(new_ptr, arr->dynamic_type_ptr, sizeof(MJSDynamicType) * arr->size);
  free(arr->dynamic_type_ptr);
  arr->dynamic_type_ptr = new_ptr;
  arr->reserve = MAX_RESERVE_ELEMENTS;
  
  //add value
  arr->dynamic_type_ptr[arr->size] = value;
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


//-----------------MJSContainer-------------------//
static unsigned int generate_hash_index(const char *str, unsigned int str_size);
static unsigned int find_next_empty_slot(MJSContainer *container);

int MJSContainer_Init(MJSContainer *container) {
 if(!container)
  return -1;
 memset(container, 0x00, sizeof(MJSContainer));
 
 container->obj_pair_ptr = (MJSObjectPair*)malloc(sizeof(MJSObjectPair) * MAX_RESERVE_ELEMENTS);
 if(!container->obj_pair_ptr)
  return -1;
 memset(container->obj_pair_ptr, 0xFF, MAX_RESERVE_ELEMENTS * sizeof(MJSObjectPair));
 
 container->reserve = MAX_RESERVE_ELEMENTS;
 container->allocated_size = MAX_RESERVE_ELEMENTS;
 container->string_pool = (char*)malloc(MAX_RESERVE_BYTES);
 if(!container->string_pool)
  return -1;
 container->string_pool_reserve = MAX_RESERVE_BYTES;
 return 0;
}


int MJSContainer_Destroy(MJSContainer *container) {
 if(!container)
  return -1;
 //destroy other allocated memory first.
 int i;
 for(i = 0; i < container->obj_pair_size; i++) {
  switch(container->obj_pair_ptr[i].value.type) {
   case MJS_TYPE_ARRAY:
    if(!MJSArray_Destroy(&container->obj_pair_ptr[i].value.value_array)) return -1;
   break;
   case MJS_TYPE_CONTAINER:
    if(!MJSContainer_Destroy(&container->obj_pair_ptr[i].value.value_container)) return -1;
   break;
  }
 }
 free(container->string_pool);
 free(container->obj_pair_ptr);
 return 0;
}


unsigned int MJSContainer_GetSize(MJSContainer *container) {
 if(!container)
  return 0xFFFFFFFF;
 return container->obj_pair_size;
}

int MJSContainer_InsertFromPool(MJSContainer *container, unsigned int pool_index, unsigned int str_size, MJSDynamicType value) {
 if(!container || (pool_index > container->string_pool_size) || !str_size)
  return -1;
 char *key = &container->string_pool[pool_index];
 unsigned int str_pool_index = pool_index;
 MJSObjectPair pair;
 
 memset(&pair, 0x00, sizeof(MJSObjectPair));
 pair.key_pool_index = str_pool_index;
 pair.key_pool_size = str_size;
 pair.value = value;
 
	const unsigned int hash_index = generate_hash_index(key, str_size);
	
	if(container->reserve == 0) {
	 MJSObjectPair *new_obj = (MJSObjectPair*)malloc((container->allocated_size + MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair));
	 if(!new_obj)
	  return -1;
	 memcpy(new_obj, container->obj_pair_ptr, container->allocated_size * sizeof(MJSObjectPair));
	 memset(&new_obj[container->allocated_size], 0xFF, (unsigned int)fast_abs32((int)container->allocated_size - (int)MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair));
	 
	 container->allocated_size += MAX_RESERVE_ELEMENTS;
	 container->reserve = MAX_RESERVE_ELEMENTS;
	 free(container->obj_pair_ptr);
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
	 	
   if(start_node->key_pool_index == 0xFFFFFFFF) {
			 prev_index = next_index;
		 	next_index = find_next_empty_slot(container);
		 	container->obj_pair_ptr[next_index] = pair;
		  container->obj_pair_ptr[prev_index].next = next_index;
	 	 break;
	 	}
		 next_index = start_node->next;
	 	start_node = &container->obj_pair_ptr[start_node->next];
 	} while(1);

	}
	container->reserve--;
	container->obj_pair_size++;
	
 return 0;
}


int MJSContainer_Insert(MJSContainer *container, const char *key, unsigned int str_size, MJSDynamicType value) {
 if(!container || !key || !str_size)
  return -1;
 
 unsigned int str_pool_index = MJSContainer_AddToStringPool(container, key, str_size);
 MJSObjectPair pair;
 
 memset(&pair, 0x00, sizeof(MJSObjectPair));
 pair.key_pool_index = str_pool_index;
 pair.key_pool_size = str_size;
 pair.value = value;
 
	const unsigned int hash_index = generate_hash_index(key, str_size);
	
	if(container->reserve == 0) {
	 MJSObjectPair *new_obj = (MJSObjectPair*)malloc((container->allocated_size + MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair));
	 if(!new_obj)
	  return -1;
	 memcpy(new_obj, container->obj_pair_ptr, container->allocated_size * sizeof(MJSObjectPair));
	 memset(&new_obj[container->allocated_size], 0xFF, (unsigned int)fast_abs32((int)container->allocated_size - (int)MAX_RESERVE_ELEMENTS) * sizeof(MJSObjectPair));
	 
	 container->allocated_size += MAX_RESERVE_ELEMENTS;
	 container->reserve = MAX_RESERVE_ELEMENTS;
	 free(container->obj_pair_ptr);
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
	 	
   if(start_node->key_pool_index == 0xFFFFFFFF) {
			 prev_index = next_index;
		 	next_index = find_next_empty_slot(container);
		 	container->obj_pair_ptr[next_index] = pair;
		  container->obj_pair_ptr[prev_index].next = next_index;
	 	 break;
	 	}
		 next_index = start_node->next;
	 	start_node = &container->obj_pair_ptr[start_node->next];
 	} while(1);

	}
	container->reserve--;
	container->obj_pair_size++;
	
 return 0;
}


MJSObjectPair* MJSContainer_GetPairReference(MJSContainer *container, const char *key) {
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


unsigned int MJSContainer_AddToStringPool(MJSContainer *container, const char *str, unsigned int str_size) {
 if(!container || !str || !str_size)
  return 0xFFFFFFFF;
  
 unsigned int out_index = 0xFFFFFFFF;
 
 if(container->string_pool_reserve > (str_size+1)) {
  memcpy(&container->string_pool[container->string_pool_size], str, str_size);
  out_index = container->string_pool_size;
  container->string_pool_size += str_size;
  container->string_pool_reserve -= str_size;
  //null terminatior
  container->string_pool[container->string_pool_size++] = '\0';
  return out_index;
 } else {
  char *new_str = (char*)malloc(container->string_pool_size + (str_size+1) + MAX_RESERVE_ELEMENTS);
  if(!new_str)
   return 0xFFFFFFFF;
  memcpy(new_str, container->string_pool, container->string_pool_size);
  free(container->string_pool);
  container->string_pool = new_str;
  memcpy(&container->string_pool[container->string_pool_size], str, str_size);
  out_index = container->string_pool_size;
  container->string_pool_size += str_size;
  container->string_pool_reserve = MAX_RESERVE_ELEMENTS;
  //null terminatior
  container->string_pool[container->string_pool_size++] = '\0';
  return out_index;
 }
 return out_index;
}


//-----------------Static func-------------------//
//Hash Function
//TODO : optimize this, mininize loop branches
static unsigned int generate_hash_index(const char *str, unsigned int str_size) {
 unsigned int index = 0;
 int i;
 for(i = 0; i < str_size; i++)
  index += (unsigned int)str[i];
 return index % MAX_HASH_BUCKETS;
}

//Linear Probing
//TODO : optimize this, mininize loop branches
static unsigned int find_next_empty_slot(MJSContainer *container) {
 unsigned int index = MAX_HASH_BUCKETS-1;
 while(index < container->allocated_size) {
  if(container->obj_pair_ptr[index].key_pool_index == 0xFFFFFFFF)
   return index;
  index++;
 }
 return 0xFFFFFFFF;
}



//-----------------MJSParser-------------------//
/*
 allocate MJSParserData, return 0 if sucess, return -1 if not.
*/
int MJSParserData_Create(MJSParsedData *parsed_data) {
 if(!parsed_data)
  return -1;
 
 memset(parsed_data, 0, sizeof(MJSParsedData));
 parsed_data->cache = (unsigned char*)malloc(MAX_CACHE_BYTES);
 parsed_data->cache_allocated_size = MAX_CACHE_BYTES;
 
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
 MJSContainer_Destroy(&parsed_data->container);
 free(parsed_data->cache);
 return 0;
}



