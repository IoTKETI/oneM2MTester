/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *
 ******************************************************************************/

#include "f_ext_import_schema.hh"
#include <stdio.h>
#include <string.h>

namespace CompareSchemas {

#define IMPORT_FORMAT_ERROR(cond, comment) \
  if (cond) { \
    TTCN_error("Invalid format for file '%s' (%s). JSON schema could not be imported.", file_name, comment); \
  }

ElemKey get_elem_key(const char* value, size_t value_len, const char* file_name)
{
  if (4 == value_len && 0 == strncmp(value, "$ref", value_len)) {
    return ElemKey::Ref;
  }
  if (4 == value_len && 0 == strncmp(value, "type", value_len)) {
    return ElemKey::Type;
  }
  if (7 == value_len && 0 == strncmp(value, "subType", value_len)) {
    return ElemKey::SubType;
  }
  if (7 == value_len && 0 == strncmp(value, "pattern", value_len)) {
    return ElemKey::Pattern;
  }
  if (12 == value_len && 0 == strncmp(value, "originalName", value_len)) {
    return ElemKey::OriginalName;
  }
  if (11 == value_len && 0 == strncmp(value, "unusedAlias", value_len)) {
    return ElemKey::UnusedAlias;
  }
  if (20 == value_len && 0 == strncmp(value, "additionalProperties", value_len)) {
    return ElemKey::AdditionalProperties;
  }
  if (10 == value_len && 0 == strncmp(value, "omitAsNull", value_len)) {
    return ElemKey::OmitAsNull;
  }
  if (7 == value_len && 0 == strncmp(value, "default", value_len)) {
    return ElemKey::Default;
  }
  if (8 == value_len && 0 == strncmp(value, "minItems", value_len)) {
    return ElemKey::MinItems;
  }
  if (8 == value_len && 0 == strncmp(value, "maxItems", value_len)) {
    return ElemKey::MaxItems;
  }
  if (4 == value_len && 0 == strncmp(value, "enum", value_len)) {
    return ElemKey::Enum;
  }
  if (13 == value_len && 0 == strncmp(value, "numericValues", value_len)) {
    return ElemKey::NumericValues;
  }
  if (5 == value_len && 0 == strncmp(value, "items", value_len)) {
    return ElemKey::Items;
  }
  if (5 == value_len && 0 == strncmp(value, "anyOf", value_len)) {
    return ElemKey::AnyOf;
  }
  if (8 == value_len && 0 == strncmp(value, "required", value_len)) {
    return ElemKey::Required;
  }
  if (10 == value_len && 0 == strncmp(value, "fieldOrder", value_len)) {
    return ElemKey::FieldOrder;
  }
  if (10 == value_len && 0 == strncmp(value, "properties", value_len)) {
    return ElemKey::Properties;
  }
  if (9 == value_len && 0 == strncmp(value, "minLength", value_len)) {
    return ElemKey::MinLength;
  }
  if (9 == value_len && 0 == strncmp(value, "maxLength", value_len)) {
    return ElemKey::MaxLength;
  }
  if (7 == value_len && 0 == strncmp(value, "minimum", value_len)) {
    return ElemKey::Minimum;
  }
  if (7 == value_len && 0 == strncmp(value, "maximum", value_len)) {
    return ElemKey::Maximum;
  }
  if (16 == value_len && 0 == strncmp(value, "exclusiveMinimum", value_len)) {
    return ElemKey::Maximum;
  }
  if (16 == value_len && 0 == strncmp(value, "exclusiveMaximum", value_len)) {
    return ElemKey::Maximum;
  }
  if (5 == value_len && 0 == strncmp(value, "allOf", value_len)) {
    return ElemKey::AllOf;
  }
  if (9 == value_len && 0 == strncmp(value, "valueList", value_len)) {
    return ElemKey::ValueList;
  }
  // it's an extension if none of them matched
  return ElemKey::Extension;
}

// just a forward declaration
AnyValue extract_any_value(JSON_Tokenizer& json, const char* file_name);

ObjectValue extract_object_value(JSON_Tokenizer& json, const char* file_name)
{
  json_token_t token = JSON_TOKEN_NONE;
  char* value = NULL;
  size_t value_len = 0;
  ObjectValue object_value;
  
  int field_index = 0;
  json.get_next_token(&token, &value, &value_len);
  while(JSON_TOKEN_NAME == token) {
    // extract fields until an object end token is found
    CHARSTRING field_name(value_len, value);
    object_value[field_index].key() = field_name;
    object_value[field_index].val() = extract_any_value(json, file_name);
    
    // next field
    ++field_index;
    json.get_next_token(&token, &value, &value_len);
  }

  // object end
  IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token, "missing object value end");
  return object_value;
}

ArrayValue extract_array_value(JSON_Tokenizer& json, const char* file_name)
{
  ArrayValue array_value;
  size_t nof_values = 0;
  while(true) {
    // extract values until the array's end is reached
    AnyValue val = extract_any_value(json, file_name);
    if (val.is_bound()) {
      array_value[nof_values] = val;
      ++nof_values;
    }
    else {
      // array end token reached (signalled by the unbound value)
      break;
    }
  }
  return array_value;
}

AnyValue extract_any_value(JSON_Tokenizer& json, const char* file_name)
{
  json_token_t token = JSON_TOKEN_NONE;
  char* value = NULL;
  size_t value_len = 0;
  AnyValue any_value;
  
  json.get_next_token(&token, &value, &value_len);
  switch (token) {
  case JSON_TOKEN_NUMBER:
  case JSON_TOKEN_STRING: {
    CHARSTRING str_val(value_len, value);
    any_value.strVal() = str_val;
    break; }
  case JSON_TOKEN_LITERAL_NULL:
    any_value.strVal() = "null";
    break;
  case JSON_TOKEN_LITERAL_TRUE:
    any_value.boolVal() = TRUE;
    break;
  case JSON_TOKEN_LITERAL_FALSE:
    any_value.boolVal() = FALSE;
    break;
  case JSON_TOKEN_OBJECT_START:
    any_value.objectVal() = extract_object_value(json, file_name);
    break;
  case JSON_TOKEN_ARRAY_START:
    any_value.arrayVal() = extract_array_value(json, file_name);
    break;
  case JSON_TOKEN_ARRAY_END:
    // signal the end of an array by returning an unbound AnyValue
    break;
  default:
    IMPORT_FORMAT_ERROR(TRUE, "missing JSON value");
  }
  return any_value;
}

TypeSchema extract_type_schema(JSON_Tokenizer& json, const char* file_name)
{
  json_token_t token = JSON_TOKEN_NONE;
  char* value = NULL;
  size_t value_len = 0;

  // type schema object start
  json.get_next_token(&token, NULL, NULL);
	IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_START != token, "missing type object start");

  // type schema elements
  TypeSchema type_schema;
  int elem_index = 0;
  json.get_next_token(&token, &value, &value_len);
  while(JSON_TOKEN_NAME == token) {
    type_schema[elem_index].key() = get_elem_key(value, value_len, file_name);
    switch(type_schema[elem_index].key()) {
    case ElemKey::Ref:
    case ElemKey::Type:
    case ElemKey::SubType:
    case ElemKey::Pattern:
    case ElemKey::OriginalName:
    case ElemKey::UnusedAlias:
    case ElemKey::MinItems:
    case ElemKey::MaxItems:
    case ElemKey::AdditionalProperties:
    case ElemKey::OmitAsNull:
    case ElemKey::Default:
    case ElemKey::MinLength:
    case ElemKey::MaxLength:
    case ElemKey::Minimum:
    case ElemKey::Maximum: {
      // string or boolean value
      json.get_next_token(&token, &value, &value_len);
      switch (token) {
        case JSON_TOKEN_STRING:
        case JSON_TOKEN_NUMBER: {
          CHARSTRING str_val(value_len, value);
          type_schema[elem_index].val().strVal() = str_val;
          break; }
        case JSON_TOKEN_LITERAL_FALSE:
          type_schema[elem_index].val().boolVal() = FALSE;
          break;
        case JSON_TOKEN_LITERAL_TRUE:
          type_schema[elem_index].val().boolVal() = TRUE;
          break;
        default:
          IMPORT_FORMAT_ERROR(JSON_TOKEN_LITERAL_FALSE != token, "string, number or boolean value expected");
      }
      break; }

    case ElemKey::NumericValues:
    case ElemKey::Required:
    case ElemKey::FieldOrder: {
      // string array value
      json.get_next_token(&token, NULL, NULL);
	    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token, "missing string array start");
      json.get_next_token(&token, &value, &value_len);
      int string_index = 0;
      while (JSON_TOKEN_STRING == token || JSON_TOKEN_NUMBER == token) {
        CHARSTRING str_val(value_len, value);
        type_schema[elem_index].val().strArrayVal()[string_index] = str_val;

        // next string
        ++string_index;
        json.get_next_token(&token, &value, &value_len);
      }

      // string array end
	    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_END != token, "missing string array end");
      break; }

    case ElemKey::Items: {
      // type schema value
      type_schema[elem_index].val().typeVal() = extract_type_schema(json, file_name);
      break; }

    case ElemKey::AnyOf:
    case ElemKey::AllOf: {
      // type schema array value
      json.get_next_token(&token, NULL, NULL);
	    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token, "missing type array start");
      int type_index = 0;
      size_t buf_pos = json.get_buf_pos();
      json.get_next_token(&token, NULL, NULL);
      while (JSON_TOKEN_OBJECT_START == token) {
        // revert this extraction, the type schema extractor will read the "{" again
        json.set_buf_pos(buf_pos);
        type_schema[elem_index].val().typeArrayVal()[type_index] = extract_type_schema(json, file_name);
        
        // next type schema
        ++type_index;
        buf_pos = json.get_buf_pos();
        json.get_next_token(&token, NULL, NULL);
      }
      
      // type schema array end
	    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_END != token, "missing type array end");
      break; }

      case ElemKey::Properties: {
        // field set value
        json.get_next_token(&token, NULL, NULL);
	      IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_START != token, "missing field set start");
        int field_index = 0;
        json.get_next_token(&token, &value, &value_len);
        while(JSON_TOKEN_NAME == token) {
          // store field name and schema
          CHARSTRING field_name(value_len, value);
          type_schema[elem_index].val().fieldSetVal()[field_index].fieldName() = field_name;
          type_schema[elem_index].val().fieldSetVal()[field_index].schema() = extract_type_schema(json, file_name);
          
          // next field
          ++field_index;
          json.get_next_token(&token, &value, &value_len);
        }

        // field set value end
	      IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token, "missing field set end");
        break; }
      case ElemKey::Extension: {
        // extension value
        // store the field name (already extracted)
        CHARSTRING str_key(value_len, value);
        type_schema[elem_index].val().extVal().key() = str_key;
        // store the string value
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_STRING != token, "string value expected");
        CHARSTRING str_val(value_len, value);
        type_schema[elem_index].val().extVal().val() = str_key;
        break; }

      case ElemKey::Enum:
      case ElemKey::ValueList: {
        // array value
        json.get_next_token(&token, NULL, NULL);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token, "missing array value start");
        type_schema[elem_index].val().arrayVal() = extract_array_value(json, file_name);
        break; }
      default:
        break;
    }

    // next schema element
    ++elem_index;
    json.get_next_token(&token, &value, &value_len);
  }

  // type schema object end
	IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token, "missing type object end");
  return type_schema;
}

EncDecData extract_enc_dec_data(JSON_Tokenizer& json, const char* file_name)
{
  // initialize (error behavior and printing data might not appear in the schema)
  json_token_t token = JSON_TOKEN_NONE;
  char* value = NULL;
  size_t value_len = 0;
  EncDecData data;
  data.eb() = OMIT_VALUE;
  data.printing() = OMIT_VALUE;

  // enc/dec data start
  json.get_next_token(&token, NULL, NULL);
  IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_START != token, "missing enc/dec data start");
  json.get_next_token(&token, &value, &value_len);
  while(JSON_TOKEN_NAME == token) {
    if (9 == value_len && 0 == strncmp(value, "prototype", value_len)) {
      // prototype (string array)
      json.get_next_token(&token, NULL, NULL);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token, "missing prototype array start");
      json.get_next_token(&token, &value, &value_len);
      int pt_index = 0;
      while(JSON_TOKEN_STRING == token) {
        CHARSTRING pt_str(value_len, value);
        data.prototype()[pt_index] = pt_str;

        // next prototype element
        ++pt_index;
        json.get_next_token(&token, &value, &value_len);
      }

      // prototype end
      IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_END != token, "missing prototype array end");
    }
    else if (13 == value_len && 0 == strncmp(value, "errorBehavior", value_len)) {
      // error behavior (object)
      json.get_next_token(&token, NULL, NULL);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_START != token, "missing error behavior start");
      json.get_next_token(&token, &value, &value_len);
      int eb_index = 0;
      while(JSON_TOKEN_NAME == token) {
        // store the key-value pair
        CHARSTRING error_type(value_len, value);
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_STRING != token, "expected error behavior string");
        CHARSTRING error_behavior(value_len, value);
        data.eb()()[eb_index].errorType() = error_type;
        data.eb()()[eb_index].errorBehavior() = error_behavior;

        // next pair
        ++eb_index;
        json.get_next_token(&token, &value, &value_len);
      }

      // error behavior end
      IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token, "missing error behavior end");
    }
    else if (8 == value_len && 0 == strncmp(value, "printing", value_len)) {
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_STRING != token, "expected printing string");
      CHARSTRING printing_str(value_len, value);
      data.printing()() = printing_str;
    }
    else {
      // invalid key
      IMPORT_FORMAT_ERROR(true, "invalid enc/dec data key");
    }

    // next key-value pair
    json.get_next_token(&token, &value, &value_len);
  }
  
  // enc/dec data end
  IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token, "missing enc/dec data end");
  return data;
}

RefSchema extract_ref_schema(JSON_Tokenizer& json, const char* file_name)
{
  // initialize (set optional fields to "omit")
  json_token_t token = JSON_TOKEN_NONE;
  char* value = NULL;
  size_t value_len = 0;
  RefSchema ref_schema;
  ref_schema.enc() = OMIT_VALUE;
  ref_schema.dec() = OMIT_VALUE;

  // read reference data
  json.get_next_token(&token, &value, &value_len);
  while(JSON_TOKEN_NAME == token) {
    if (4 == value_len && 0 == strncmp(value, "$ref", value_len)) {
      // reference
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_STRING != token, "missing reference string");
      CHARSTRING ref_str(value_len, value);
      ref_schema.ref() = ref_str;
    }
    else if (8 == value_len && 0 == strncmp(value, "encoding", value_len)) {
      // encoding function
      ref_schema.enc()() = extract_enc_dec_data(json, file_name);
    }
    else if (8 == value_len && 0 == strncmp(value, "decoding", value_len)) {
      // encoding function
      ref_schema.dec()() = extract_enc_dec_data(json, file_name);
    }
    else {
      // invalid key
      IMPORT_FORMAT_ERROR(true, "invalid reference/function data key");
    }

    // next key-value pair
    json.get_next_token(&token, &value, &value_len);
  }

  // reference & function info end
  IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token, "missing reference/function data end");
  return ref_schema;
}

void f__ext__import__schema(const CHARSTRING& file, JsonSchema& schema)
{
  // need a null-terminated string for fopen
  char* file_name = mcopystrn((const char*)file, file.lengthof());
  FILE* fp = fopen(file_name, "r");
  try {
    if (NULL == fp) {
      TTCN_error("Could not open file '%s' for reading. JSON schema could not be imported.", file_name);
    }
    
    // get the file size
	  fseek(fp, 0, SEEK_END);
	  int file_size = ftell(fp);
	  rewind(fp);

    // read the entire file into a character buffer
	  char* buffer = (char*)Malloc(file_size);
	  fread(buffer, 1, file_size, fp);
    fclose(fp);

	  // initialize a JSON tokenizer with the buffer
	  JSON_Tokenizer json(buffer, file_size);
	  Free(buffer);

    // extract tokens and store the schema in the JsonSchema parameter
    // throw a DTE if the file format is invalid
    json_token_t token = JSON_TOKEN_NONE;
    char* value = NULL;
    size_t value_len = 0;

    // top level object
    json.get_next_token(&token, NULL, NULL);
	  IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_START != token, "missing top level object start");
	  
	  // schema header
	  json.get_next_token(&token, &value, &value_len);
	  IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 7 ||
      0 != strncmp(value, "$schema", value_len), "missing $schema key");
    json.get_next_token(&token, &value, &value_len);
    IMPORT_FORMAT_ERROR(JSON_TOKEN_STRING != token || value_len != 41 ||
      0 != strncmp(value, "\"http://json-schema.org/draft-04/schema#\"", value_len),
      "missing $schema value");

    // definitions
    json.get_next_token(&token, &value, &value_len);
	  IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 11 ||
      0 != strncmp(value, "definitions", value_len), "missing definitions key");

    // module list
    json.get_next_token(&token, NULL, NULL);
	  IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_START != token, "missing module list start");
    json.get_next_token(&token, &value, &value_len);
    int type_index = 0;
    while (JSON_TOKEN_NAME == token) {
      // extract module name, it will be inserted later
      CHARSTRING module_name(value_len, value);

      // type list
      json.get_next_token(&token, NULL, NULL);
	    IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_START != token, "missing type list start");
      json.get_next_token(&token, &value, &value_len);
      while (JSON_TOKEN_NAME == token) {
        // extract type name
        CHARSTRING type_name(value_len, value);
        
        // extract type schema
        TypeSchema type_schema = extract_type_schema(json, file_name);
        
        // store definition data
        schema.defs()[type_index].moduleName() = module_name;
        schema.defs()[type_index].typeName() = type_name;
        schema.defs()[type_index].schema() = type_schema;

        // next type
        ++type_index;
        json.get_next_token(&token, &value, &value_len);
      }

      // end of type list
	    IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token, "missing type list end");

      // next module
      json.get_next_token(&token, &value, &value_len);
    }

    // end of module list
	  IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token, "missing module list end");

    if (0 == type_index) {
      // no definitions, don't leave the field unbound
      schema.defs().set_size(0);
    }

    // top level anyOf
    json.get_next_token(&token, &value, &value_len);
	  IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 5 ||
      0 != strncmp(value, "anyOf", value_len), "missing anyOf key");

    // reference & function info array
    json.get_next_token(&token, NULL, NULL);
    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token, "missing reference/function list start");
    json.get_next_token(&token, NULL, NULL);
    int ref_index = 0;
    while (JSON_TOKEN_OBJECT_START == token) {
      
      // extract reference schema
      schema.refs()[ref_index] = extract_ref_schema(json, file_name);
      
      // next reference
      ++ref_index;
      json.get_next_token(&token, NULL, NULL);
    }

    // end of reference & function info array
    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_END != token, "missing reference/function list end");

    if (0 == ref_index) {
      // no references, don't leave the field unbound
      schema.refs().set_size(0);
    }

    // end of top level object
    json.get_next_token(&token, NULL, NULL);
	  IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token, "missing top level object end");

    // end of the schema
    json.get_next_token(&token, NULL, NULL);
	  IMPORT_FORMAT_ERROR(JSON_TOKEN_NONE != token, "expected end of file");
  }
  catch (...) {
    Free(file_name);
    throw;
  }
  Free(file_name);
}

}

