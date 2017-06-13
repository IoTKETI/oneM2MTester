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

#include "Ttcn2Json.hh"

#include "compiler.h"

#include "../AST.hh"
#include "../../common/JSON_Tokenizer.hh"
#include "../Value.hh"
#include "../Valuestuff.hh"

// forward declarations
namespace Common {
  class Type;
}

namespace Ttcn {

Ttcn2Json::Ttcn2Json(Common::Modules* p_modules, const char* p_schema_name)
: modules(p_modules)
{
  boolean is_temporary;
  FILE* file = open_output_file(p_schema_name, &is_temporary);
  
  JSON_Tokenizer json(true);
  
  create_schema(json);
  
  fprintf(file, "%s\n", json.get_buffer());
  
  close_output_file(p_schema_name, file, is_temporary, 0);
}

void Ttcn2Json::create_schema(JSON_Tokenizer& json)
{
  // top-level object start
  json.put_next_token(JSON_TOKEN_OBJECT_START, NULL);
  
  // insert the schema header
  json.put_next_token(JSON_TOKEN_NAME, "$schema");
  json.put_next_token(JSON_TOKEN_STRING, "\"http://json-schema.org/draft-04/schema#\"");
  
  // start of type definitions
  json.put_next_token(JSON_TOKEN_NAME, "definitions");
  json.put_next_token(JSON_TOKEN_OBJECT_START, NULL);
  
  // insert module names and schemas for types; gather references to types and
  // JSON encoding/decoding function information
  map<Common::Type*, JSON_Tokenizer> json_refs;
  modules->generate_json_schema(json, json_refs);
  
  // end of type definitions
  json.put_next_token(JSON_TOKEN_OBJECT_END, NULL);

  if (!json_refs.empty()) {
    // top-level "anyOf" structure containing references to the types the schema validates
    // don't insert an empty "anyOf" if there are no references
    json.put_next_token(JSON_TOKEN_NAME, "anyOf");
    json.put_next_token(JSON_TOKEN_ARRAY_START, NULL);

    // close schema segments and add them to the main schema
    for (size_t i = 0; i < json_refs.size(); ++i) {
      JSON_Tokenizer* segment = json_refs.get_nth_elem(i);
      segment->put_next_token(JSON_TOKEN_OBJECT_END, NULL);
      insert_schema(json, *segment);
      delete segment;
    }
    json_refs.clear();

    // end of the "anyOf" structure
    json.put_next_token(JSON_TOKEN_ARRAY_END, NULL);
  }
  
  // top-level object end
  json.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
}

void Ttcn2Json::insert_schema(JSON_Tokenizer& to, JSON_Tokenizer& from)
{
  json_token_t token = JSON_TOKEN_NONE;
  char* value_str = NULL;
  size_t value_len = 0;
  char temp = 0;
  
  do {
    from.get_next_token(&token, &value_str, &value_len);
    
    if (token == JSON_TOKEN_ERROR) {
      FATAL_ERROR("Ttcn2Json::insert_schema");
    }
    
    if (value_str != NULL) {
      // put_next_token expects a null terminated string, save the next character
      // and set it to null
      temp = value_str[value_len];
      value_str[value_len] = 0;
    }
    
    to.put_next_token(token, value_str);
    
    if (value_str != NULL) {
      // put the original character back to its place
      value_str[value_len] = temp;
    }
  } while (token != JSON_TOKEN_NONE);
}

JsonOmitCombination::JsonOmitCombination(Common::Value* p_top_level)
{
  parse_value(p_top_level->get_value_refd_last());
}

JsonOmitCombination::~JsonOmitCombination()
{
  for (size_t i = 0; i < combinations.size(); ++i) {
    delete combinations.get_nth_elem(i);
  }
  combinations.clear();
}

bool JsonOmitCombination::next(size_t current_value /* = 0 */)
{
  if (current_value == combinations.size()) {
    return false;
  }
  Common::Value* val = combinations.get_nth_key(current_value);
  int* omitted_fields = combinations.get_nth_elem(current_value);
  for (size_t i = 0; i < val->get_nof_comps(); ++i) {
    if (omitted_fields[i] == OMITTED_ABSENT) {
      // ABSENT (zero bit) found, set it to NULL (one bit) and reset all previous
      // NULLs (ones) to ABSENTs (zeros), first in this value ...
      omitted_fields[i] = OMITTED_NULL;
      for (size_t j = 0; j < i; ++j) {
        if (omitted_fields[j] == OMITTED_NULL) {
          omitted_fields[j] = OMITTED_ABSENT;
        }
      }
      // ... and then in all previous record values
      reset_previous(current_value);
      return true;
    }
  }
  // no new combinations in this value, check the next one
  return next(current_value + 1);
}

JsonOmitCombination::omit_state_t JsonOmitCombination::get_state(
  Common::Value* p_rec_value, size_t p_comp) const
{
  return (JsonOmitCombination::omit_state_t)combinations[p_rec_value][p_comp];
}

void JsonOmitCombination::parse_value(Common::Value* p_value)
{
  switch (p_value->get_valuetype()) {
  case Common::Value::V_SEQ:
  case Common::Value::V_SET: {
    size_t len = p_value->get_nof_comps();
    int* omitted_fields = new int[len]; // stores one combination
    for (size_t i = 0; i < len; ++i) {
      Common::Value* val = p_value->get_se_comp_byIndex(i)->get_value();
      if (val->get_valuetype() == Common::Value::V_OMIT) {
        // all omitted fields are absent in the first combination
        omitted_fields[i] = OMITTED_ABSENT;
      }
      else {
        omitted_fields[i] = NOT_OMITTED;
      }
      parse_value(val->get_value_refd_last());
    }
    combinations.add(p_value, omitted_fields);
    break; }
  case Common::Value::V_SEQOF:
  case Common::Value::V_SETOF:
    for (size_t i = 0; i < p_value->get_nof_comps(); ++i) {
      parse_value(p_value->get_comp_byIndex(i)->get_value_refd_last());
    }
    break;
  case Common::Value::V_CHOICE:
    parse_value(p_value->get_alt_value()->get_value_refd_last());
  default:
    break;
  }
}

void JsonOmitCombination::reset_previous(size_t value_count)
{
  for (size_t i = 0; i < value_count; ++i) {
    Common::Value* val = combinations.get_nth_key(i);
    int* omitted_fields = combinations.get_nth_elem(i);
    for (size_t j = 0; j < val->get_nof_comps(); ++j) {
      if (omitted_fields[j] == OMITTED_NULL) {
        omitted_fields[j] = OMITTED_ABSENT;
      }
    }
  }
}

} // namespace

