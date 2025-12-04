# MicroJSON
Simple, Extremely Fast, Low level and lightweight json parser/generator
written in c89

# Sample
 [sample_array.cpp](https://github.com/ZvRzyan18/MicroJSON/blob/main/sample_array.cpp)
```cpp
#include <stdio.h>
#include <string.h>

#include "micro_json/micro_json.h"

int main(int argc, char *argv[]) {

 //C++ syntax (only used here for clear example)
 const char* json_string = R"(
 
  {
   "my_array" : ["string1", 45, 3.14, null, false, true, 88e-1]
  }
  
 )";

 MJSParsedData json_parser;
 MJSTokenResult json_result;
 
 //init parser data
 if(MJSParserData_Init(&json_parser)) {
  perror("init data failed");
  return -1;
 }
  
 //init tokenizer
 if(MJS_InitToken(&json_parser, json_string, strlen(json_string))) {
  perror("init token failed");
  return -1;
 }
 
 //extract the main object container
 MJSObject *main_object = &json_parser.container;


 //start the parsing process
  json_result = MJS_StartToken(&json_parser);
  if(json_result.code) {
   fprintf(stderr, "Error : Line [%i] -> %s", json_result.line, MJS_CodeToString(json_result.code));
   return -1;
  } else {
 
  //extract the array reference pointer.
   const char *ref_key = "my_array";
   MJSObjectPair *pair = MJSObject_GetPairReference(main_object, ref_key, strlen(ref_key));
   if(!pair) {
    printf("%s, not found\n", ref_key);
    return -1;
   }
   MJSArray *arr = &pair->value.value_array;
   
   //print all results
   for(int i = 0; i < MJSArray_Size(arr); i++) {
   
    //dynamic type
    MJSDynamicType *dynamic_type = MJSArray_Get(arr, i);
    
    //json uses a dynamic typing, so it needs to check
    //specific types first before using it
    switch(dynamic_type->type) {
    case MJS_TYPE_STRING:
     printf("string : %s\n", MJSObject_GetStringFromPool(main_object, &dynamic_type->value_string));
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
  
 }
  
 //all objects allocated by parser will be destroyed.
 if(MJSParserData_Destroy(&json_parser)) {
  perror("destroy failed");
  return -1;
 }
 
	return 0;
}
```
# Features
* Simple
* Embeddable (No need build systems)
* Self Contained (No external library aside from libc)

# Optimization Techniques
* SIMD Instructions (ARM NEON)
* Aggressive Loop Unrolling
* Memory Aligned allocator
* Cache friendly Array Based Hash
* Tag Unions for multiple types


