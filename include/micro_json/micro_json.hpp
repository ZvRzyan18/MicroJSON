#ifndef MC_JSON_HPP
#define MC_JSON_HPP

#include "micro_json/micro_json.h"
/*
 High Level Wrapper of micro json
*/
#ifdef __cplusplus

#include <utility>
#include <string>
#include <cassert>
#include <cstring>
#include <type_traits>

namespace mjs {

/*-----------------Classes-------------------*/

class object;
class dynamic_type;
class string;
class array;
class parsed_data;

template<typename T, typename E>
class expected;

/*-----------------Expected-------------------*/

template<typename T, typename E>
class expected {
private:
 T _val; E _err; bool _has_value;
public:

 inline expected() {
  _has_value = false;
 }

 inline expected(T v) {
  _val = v;
  _has_value = true;
 }
 inline expected(E e) {
  _has_value = false;
  _err = e;
 }
 
 inline expected& operator=(expected e) {
  _has_value = e._has_value;
  _err = e._err;
  _val = e._val;
  return *this;
 }
 
 inline bool has_value() const { 
  return _has_value; 
 }
 inline explicit operator bool() const {
  return _has_value; 
 }
 inline explicit operator bool() {
  return _has_value; 
 }
 inline T value() { 
  return _val; 
 }
 inline E error() {
  return _err;
 }
 inline const T value() const { 
  return _val; 
 }
 inline const E error() const {
  return _err;
 }
};


/*
 special custom type just for string.
*/
struct __str_dynamic_type {
 MJSDynamicType *_type;
 bool borrowed;
 MJSObject *_object;
};


/*-----------------Dynamic Type-------------------*/

class dynamic_type {
 private:
  MJSDynamicType *_type;
  bool borrowed;
  MJSObject *_object; //special reserve type, only for string
 public:
 
 
 inline dynamic_type() {
  _type = nullptr;
  borrowed = false;
  _object = nullptr;
 }
 
  
 inline dynamic_type(dynamic_type& type) {
  this->_type = type._type;
  borrowed = true;
  this->_object = type._object;
 }
  
  
 inline dynamic_type(dynamic_type&& type) {
  this->_type = type._type;
  borrowed = false;
  this->_object = type._object;
  type._type = nullptr;
  type.borrowed = false;
  type._object = nullptr;
 }
 

 explicit inline dynamic_type(MJSDynamicType *type) {
  if(type == nullptr)
   return;
  this->_type = type;
  borrowed = true;
 }
  
  
 inline ~dynamic_type() {
  if(this->_type == nullptr)
   return;
  if(!borrowed) {
  switch(this->_type->type) {
   case MJS_TYPE_ARRAY:
    MJSArray_Destroy(&this->_type->value_array);
   break;
   case MJS_TYPE_OBJECT:
    MJSObject_Destroy(&this->_type->value_object);
   break;
   case MJS_TYPE_STRING:
   case MJS_TYPE_BOOLEAN:
   case MJS_TYPE_NULL:
   case MJS_TYPE_NUMBER_INT:
   case MJS_TYPE_NUMBER_FLOAT:
   case 0xFF:
   break;
  }
  }
 }
 
 
 inline dynamic_type& operator=(dynamic_type type) {
  this->_type = type._type;
  borrowed = true;
  this->_object = type._object;
  return *this;
 }
 
 
 inline unsigned char get_type() {
  if(this->_type == nullptr)
   return 0;
  return this->_type->type;
 }
 
 
 inline unsigned char get_type() const {
  if(this->_type == nullptr)
   return 0;
  return this->_type->type;
 }
 
 
 inline MJSDynamicType* ptr() {
  return this->_type;
 }
 
 
 inline const MJSDynamicType* ptr() const {
  return this->_type;
 }


 inline MJSDynamicType get() {
  assert(!this->_type && "the object is null");
  return *this->_type;
 }
 
 
 inline const MJSDynamicType get() const {
  assert(!this->_type && "the object is null");
  return *this->_type;
 }

};

/*-----------------Object Type-------------------*/

class object {
 private:
  MJSObject *_object;
  bool borrowed;
 public:
 
 inline object() {
  _object = nullptr;
  borrowed = false;
 }
 
 
 inline object(object& obj) {
  _object = obj._object;
  borrowed = true;
 }
 

 inline object(object&& obj) {
  _object = obj._object;
  borrowed = false;
  obj._object = nullptr;
  obj.borrowed = false;
 }

 
 explicit inline object(MJSObject *obj) {
  if(obj == nullptr)
   return;
  _object = obj;
  borrowed = true;
 }
 
 
 inline ~object() {
  if(this->_object == nullptr)
   return;
  if(!this->borrowed && this->_object->type == MJS_TYPE_OBJECT)
   MJSObject_Destroy(this->_object);
 }
 
 
 inline object& operator=(object obj) {
  _object = obj._object;
  borrowed = true;
  return *this;
 }
 
 
 inline expected<object, std::string> init() {
  assert(this->_object  && "object is already allocated");
  assert(this->borrowed  && "cannot init borrowed object");
  unsigned int result;
  result = MJSObject_Init(this->_object);
  if(result)
   return expected<object, std::string>(std::string(MJS_CodeToString(result)));
  return expected<object, std::string>(object(*this));
 }
 
 
 inline void destroy() {
  assert(this->_object && "cannot destroy null object");
  assert(this->borrowed && "cannot destroy borrowed object");
  if(this->_object->type == MJS_TYPE_OBJECT)
   MJSObject_Destroy(this->_object);
 }


 inline unsigned char get_type() {
  if(this->_object == nullptr)
   return 0;
  return this->_object->type;
 }
 
 
 inline unsigned char get_type() const {
 if(this->_object == nullptr)
  return 0;
  return this->_object->type;
 }
 
 
 inline MJSObject* ptr() {
  return this->_object;
 }
 
 
 inline const MJSObject* ptr() const {
  return this->_object;
 }


 inline MJSObject get() {
  assert(this->_object && "the object is null");
  return *this->_object;
 }
 
 
 inline const MJSObject get() const {
  assert(this->_object && "the object is null");
  return *this->_object;
 }
 
 inline expected<dynamic_type, std::string> get(std::string key) {
  MJSObjectPair *pair = MJSObject_GetPairReference(this->_object, key.c_str(), key.length());  
  if(pair == nullptr)
   return expected<dynamic_type, std::string>(std::string("not found."));
  
  switch(pair->value.type) {
   case MJS_TYPE_STRING:
   {
    dynamic_type d(&pair->value);
    __str_dynamic_type *m = reinterpret_cast<__str_dynamic_type*>(&d);
    m->_object = this->_object;
    return expected<dynamic_type, std::string>(d);
   }
   break;
   case MJS_TYPE_ARRAY:
   {
    dynamic_type a(&pair->value);
    __str_dynamic_type *b = reinterpret_cast<__str_dynamic_type*>(&a);
    b->_object = this->_object;
    return expected<dynamic_type, std::string>(a);
   }
   break;
  }
  return expected<dynamic_type, std::string>(dynamic_type(&pair->value));
 }
};

/*-----------------Array Type-------------------*/

class array {
 private:
  MJSArray *_array;
  bool borrowed;
  MJSObject *_object;
 public:
 
 array() {
  this->_array = nullptr;
  this->borrowed = false;
 }
 
 
 inline array(array& arr) {
  _array = arr._array;
  borrowed = true;
 }
 

 inline array(array&& arr) {
  _array = arr._array;
  borrowed = false;
  arr._array = nullptr;
  arr.borrowed = false;
 }

 
 explicit inline array(MJSArray *arr, MJSObject *obj) {
  if(arr == nullptr)
   return;
  _array = arr;
  _object = obj;
  borrowed = true;
 }
 
 
 inline ~array() {
  if(this->_array == nullptr)
   return;
  if(!this->borrowed && this->_array->type == MJS_TYPE_ARRAY)
   MJSArray_Destroy(this->_array);
 }
 
 
 inline array& operator=(array arr) {
  _array = arr._array;
  borrowed = true;
  return *this;
 }
 
 
 inline expected<array, std::string> init() {
  assert(!this->_array  && "object is already allocated");
  assert(this->borrowed  && "cannot init borrowed object");
  unsigned int result;
  result = MJSArray_Init(this->_array);
  if(result)
   return expected<array, std::string>(std::string(MJS_CodeToString(result)));
  return expected<array, std::string>(array(*this));
 }
 
 
 inline void destroy() {
  assert(this->_array && "cannot destroy null object");
  assert(this->borrowed && "cannot destroy borrowed object");
  if(this->_array->type == MJS_TYPE_ARRAY)
   MJSArray_Destroy(this->_array);
 }


 inline unsigned char get_type() {
  if(this->_array == nullptr)
   return 0;
  return this->_array->type;
 }
 
 
 inline unsigned char get_type() const {
 if(this->_array == nullptr)
  return 0;
  return this->_array->type;
 }
 
 
 inline MJSArray* ptr() {
  return this->_array;
 }
 
 
 inline const MJSArray* ptr() const {
  return this->_array;
 }


 inline MJSArray get() {
  assert(this->_array && "the object is null");
  return *this->_array;
 }
 
 
 inline const MJSArray get() const {
  assert(this->_array && "the object is null");
  return *this->_array;
 }
 
 
 inline unsigned int size() {
  return MJSArray_Size(this->_array);
 }
 
 
 inline const unsigned int size() const {
  return MJSArray_Size(this->_array);
 }
 
 
 inline expected<dynamic_type, std::string> get(unsigned int index) {
  MJSDynamicType *type = MJSArray_Get(this->_array, index);  
  if(type == nullptr)
   expected<dynamic_type, std::string>(std::string("not found."));
  
  switch(type->type) {
   case MJS_TYPE_STRING:
   {
    dynamic_type d(type);
    __str_dynamic_type *m = reinterpret_cast<__str_dynamic_type*>(&d);
    m->_object = this->_object;
    return expected<dynamic_type, std::string>(d);
   }
   break;
   case MJS_TYPE_ARRAY:
   {
    dynamic_type a(type);
    __str_dynamic_type *b = reinterpret_cast<__str_dynamic_type*>(&a);
    b->_object = this->_object;
    return expected<dynamic_type, std::string>(a);
   }
   break;
  }
  return expected<dynamic_type, std::string>(dynamic_type(type));
 }
};

/*-----------------String type-------------------*/

class string {
 private:
  MJSString *_string;
  bool borrowed;
  MJSObject *_object; //its needed, since the char array is stored in a pool
 public:
 
 inline string() {
  borrowed = false;
  this->_string = nullptr;
  this->_object = nullptr;
 }
 
 
 inline string(MJSString *str, MJSObject *obj) {
  this->_string = str;
  this->_object = obj;
 }
  
  
 inline string(string& str) {
  this->_string = str._string;
  this->_object = str._object;
 }
 
 
 inline string(string&& str) {
  this->_string = str._string;
  this->_object = str._object;
  str._string = nullptr;
  str._object = nullptr;
 }
 
 
 inline ~string() {
 }
 
 
 inline string& operator=(string str) {
  this->_string = str._string;
  this->_object = str._object;
  return *this;
 }
 
 
 inline const std::string to_std_string() {
  return std::string(MJSObject_GetStringFromPool(this->_object, this->_string));
 }
 

 inline unsigned char get_type() {
  if(this->_object == nullptr)
   return 0;
  return this->_string->type;
 }
 
 
 inline unsigned char get_type() const {
 if(this->_object == nullptr)
  return 0;
  return this->_string->type;
 }
 
 
 inline MJSString* ptr() {
  return this->_string;
 }
 
 
 inline const MJSString* ptr() const {
  return this->_string;
 }


 inline MJSString get() {
  assert(this->_string && "the object is null");
  return *this->_string;
 }
 
 
 inline const MJSString get() const {
  assert(this->_object && "the object is null");
  return *this->_string;
 }
};


/*-----------------Specialized Templates-------------------*/
//only used for custom casting
template<class A, class B>
inline A explicit_cast(B t) {}

template<>
inline object explicit_cast<object, dynamic_type>(dynamic_type t) {
 assert(t.get_type() != MJS_TYPE_NULL && "null has no type, you should use dynamic_type::get_type() before casting it.");
 assert(t.get_type() == MJS_TYPE_OBJECT && "not an object");
 return object(&t.ptr()->value_object);
}



template<>
inline int explicit_cast<int, dynamic_type>(dynamic_type t) {
 assert(t.get_type() != MJS_TYPE_NULL && "null has no type, you should use dynamic_type::get_type() before casting it.");
 assert(t.get_type() == MJS_TYPE_NUMBER_INT && "not an integer");
 return t.ptr()->value_int.value;
}


template<>
inline float explicit_cast<float, dynamic_type>(dynamic_type t) {
 assert(t.get_type() != MJS_TYPE_NULL && "null has no type, you should use dynamic_type::get_type() before casting it.");
 assert(t.get_type() == MJS_TYPE_NUMBER_FLOAT && "not a float");
 return t.ptr()->value_float.value;
}


template<>
inline bool explicit_cast<bool, dynamic_type>(dynamic_type t) {
 assert(t.get_type() != MJS_TYPE_NULL && "null has no type, you should use dynamic_type::get_type() before casting it.");
 assert(t.get_type() == MJS_TYPE_BOOLEAN && "not a boolean");
 return t.ptr()->value_boolean.value;
}


template<>
inline string explicit_cast<string, dynamic_type>(dynamic_type t) {
 assert(t.get_type() != MJS_TYPE_NULL && "null has no type, you should use dynamic_type::get_type() before casting it.");
 assert(t.get_type() == MJS_TYPE_STRING && "not a string");
 __str_dynamic_type *m = reinterpret_cast<__str_dynamic_type*>(&t);
 return string(&t.ptr()->value_string, m->_object);
}

template<>
inline array explicit_cast<array, dynamic_type>(dynamic_type t) {
 assert(t.get_type() != MJS_TYPE_NULL && "null has no type, you should use dynamic_type::get_type() before casting it.");
 assert(t.get_type() == MJS_TYPE_ARRAY && "not an array");
 __str_dynamic_type *m = reinterpret_cast<__str_dynamic_type*>(&t);
 return array(&t.ptr()->value_array, m->_object);
}

/*-----------------Parsed Data-------------------*/

class parsed_data {
 private:
  MJSParsedData _data;
  bool allocated;
 public:
 
 inline parsed_data() {
  this->_data = {};
  allocated = false;
 }
 
 
 inline parsed_data(parsed_data& d) = delete;
 
 inline parsed_data(parsed_data&& d) {
  this->_data = d._data;
  d._data = {};
 }
 
 
 inline ~parsed_data() {
  if(allocated)
   MJSParserData_Destroy(&this->_data);
 }
 
 
 inline expected<int, std::string> init(std::string json) {
  int code;
  code = MJSParserData_Init(&this->_data);
  if(code)
   return expected<int, std::string>(std::string(MJS_CodeToString(code)));
  code = MJS_InitToken(&this->_data, json.c_str(), json.length());
  if(code)
   return expected<int, std::string>(std::string(MJS_CodeToString(code)));
  allocated = true;
  return expected<int, std::string>(0);
 }
 
 
 inline expected<int, std::string> destroy() {
  if(allocated) {
   int code;
   code = MJSParserData_Destroy(&this->_data);
   if(!code)
    return expected<int, std::string>(std::string(MJS_CodeToString(code)));
   allocated = false;
  }
  return expected<int, std::string>(0);
 }
 
 
 inline expected<int, std::string> start_tokenize() {
  assert(allocated && "the object should be allocated");
  MJSTokenResult result = MJS_StartToken(&this->_data);
  if(result.code) 
   return expected<int, std::string>(std::string("Error : Line [").append(std::to_string(result.line)).append("] -> ").append(MJS_CodeToString(result.code)));
  return expected<int, std::string>(0);
 }
 
 
 inline object get_object() {
  assert(allocated && "the object should be allocated");
  return object(&this->_data.container);
 }
 
};



} //mjs namespace

#endif //ifdef __cplusplus

#endif
