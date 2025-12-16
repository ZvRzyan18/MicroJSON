# MicroJSON
Simple, Extremely Fast, Low level and lightweight json parser/generator
written in c89 (NOTE : this library is not stable yet)

# Features
* Simple
* Embeddable (No need build systems)
* Self Contained (No external library aside from libc, <stdlib.h>, <string.h>, <math.h>)

# Limitations
* Not thread safe
  
# Optimizations 
* String Pool
* SIMD Instructions (ARM NEON)
* Aggressive Loop Unrolling
* Memory aligned allocator
* Cache friendly array Based Hash
* Tag unions for multiple types

# Example
 [sample_array.cpp](https://github.com/ZvRzyan18/MicroJSON/blob/main/sample_array.cpp)
```cpp
#include <stdio.h>
#include <string.h>

#include "micro_json/micro_json.h"

int main(int argc, char *argv[]) {

 const char* json_string = R"(
 
  {
   "my_array" : ["string1", 45, 3.14, null, false, true, 88e-1]
  }
  
 )";

 MJSParsedData json_parser;
 MJSTokenResult json_result;
 MJSStringPool pool;
 int result;
 //init parser data
 result = MJSParserData_Init(&json_parser);
 if(result) {
  fprintf(stderr, "init data failed %s\n", MJS_CodeToString(result));
  return -1;
 }
  
 result = MJSStringPool_Init(&pool);
 if(result) {
  fprintf(stderr, "init pool failed %s\n", MJS_CodeToString(result));
  return -1;
 }
 
 json_result = MJS_TokenParse(&json_parser, &pool, json_string, strlen(json_string));
 if(json_result.code) {
  fprintf(stderr, "Error : Line [%i] -> %s", json_result.line, MJS_CodeToString(json_result.code));
  return -1;
 }
 
 //extract the main object container
 MJSDynamicType *main_object = &json_parser.container; 
 
 //the top hierarchy type in json above is object, so it must ne checked
 if(main_object->type != MJS_TYPE_OBJECT)
  fprintf(stderr, " not an object \n");
  
  //extract the array reference pointer, it will return NULL if not found
  const char *ref_key = "my_array";
  MJSDynamicType *value = MJSObject_Get(&main_object->value_object, &pool, ref_key, strlen(ref_key));
   if(!value) {
    printf("%s, not found\n", ref_key);
    return -1;
   }
  //make sure all types must be checked first, so it wont crash
  if(value->type != MJS_TYPE_ARRAY)
   fprintf(stderr, " reference %s 's value not an array \n", ref_key);

   MJSArray *arr = &value->value_array;
   
   //print all results
   for(int i = 0; i < MJSArray_Size(arr); i++) {
   
    //dynamic type (array[i] element)
    MJSDynamicType *dynamic_type = MJSArray_Get(arr, i);
    
    //json uses a dynamic typing, so it needs to check
    //specific types first before using it
    switch(dynamic_type->type) {
    case MJS_TYPE_STRING:
     // read string from pool
     printf("string : %s\n", &pool.root[dynamic_type->value_string.chunk_index].str[dynamic_type->value_string.pool_index]);
    break;
    case MJS_TYPE_NULL:
     printf("no_type : null\n");
    break;
    case MJS_TYPE_NUMBER_INT:
     printf("int : %i\n", dynamic_type->value_int.value);
    break;
    case MJS_TYPE_NUMBER_FLOAT:
     printf("float : %f\n", dynamic_type->value_float.value);
    break;
    case MJS_TYPE_BOOLEAN:
     printf("boolean : %s\n", dynamic_type->value_boolean.value ? "true" : "false");
    break;
   }
  }
  
  
 //all objects allocated by parser will be destroyed.
 result = MJSParserData_Destroy(&json_parser);
 if(result) {
  fprintf(stderr, "destroyed error %s\n", MJS_CodeToString(result));
  return -1;
 }
 
 result = MJSStringPool_Destroy(&pool);
 if(result) {
  fprintf(stderr, "destroyed error %s\n", MJS_CodeToString(result));
  return -1;
 }
 
	return 0;
}
```

