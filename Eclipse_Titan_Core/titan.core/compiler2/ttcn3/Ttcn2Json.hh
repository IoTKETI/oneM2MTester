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

#ifndef TTCN2JSON_HH
#define	TTCN2JSON_HH

#include "../map.hh"

// forward declarations
namespace Common {
  class Modules;
  class Value;
}

class JSON_Tokenizer;

namespace Ttcn {
  
  /** TTCN-3 to JSON schema converter
    * Generates a JSON schema from the type and coding function definitions in
    * TTCN-3 modules */
  class Ttcn2Json {
    
  private:
    
    /** Input modules */
    Common::Modules* modules;
    
    Ttcn2Json(const Ttcn2Json&); // no copying
    Ttcn2Json& operator=(const Ttcn2Json&); // no assignment
    
    /** Inserts the JSON schema skeleton into the parameter JSON tokenizer and
      * passes the tokenizer on to the TTCN-3 modules to insert the schemas for
      * their types and coding functions */
    void create_schema(JSON_Tokenizer& json);
    
    /** Inserts a JSON schema to the end of another schema
      * @param to contains the destination schema
      * @param from contains the inserted (source) schema; its contents will be
      * altered (ruined), do not use after this call */
    void insert_schema(JSON_Tokenizer& to, JSON_Tokenizer& from);
    
  public:
    
    /** Initializes this object with the input modules, calls create_schema() to
      * generate the JSON schema and writes the schema into the given file 
      * @param p_modules input TTCN-3 modules
      * @param p_schema_name JSON schema file name */
    Ttcn2Json(Common::Modules* p_modules, const char* p_schema_name);
    
  };
  
  /** This class helps generate all combinations of JSON encodings for omitted
    * fields
    *
    * JSON code for omitted optional fields of can be generated in 2 ways:
    * - the field is not present in the JSON object or
    * - the field is present and its value is 'null'.
    * Because of this all record/set values containing omitted fields have 2^N
    * possible JSON encodings, where N is the number of omitted fields. */
  class JsonOmitCombination {
  public:
    /** This type contains the JSON encoding type of an omitted optional field */
    enum omit_state_t {
      NOT_OMITTED,     // the field is not omitted
      OMITTED_ABSENT,  // the omitted field is not present in the JSON object
      OMITTED_NULL     // the omitted field is set to 'null' in the JSON object
    };
    
    /** Constructor - generates the first combination*/
    JsonOmitCombination(Common::Value* p_top_level);
    
    /** Destructor */
    ~JsonOmitCombination();
    
    /** Generates the next combination (recursive)
      * 
      * The algorithm is basically adding 1 to a binary number (whose bits are the
      * omitted fields of all the records stored in this object, zero bits are 
      * OMITTED_ABSENT, one bits are OMITTED_NULL, all NOT_OMITTEDs are ignored
      * and the first bit is the least significant bit).
      * 
      * The constructor generates the first combination, where all omitted fields
      * are absent (=all zeros).
      *
      * Usage: keep calling this function (with its default parameter) until the
      * last combination  (where all omitted fields are 'null', = all ones) is reached.
      * 
      * @param current_value index of the value whose omitted fields are currently
      * evaluated (always starts from the first value)
      * 
      * @return true, if the next combination was successfully generated, or
      * false, when called with the last combination */
    bool next(size_t current_value = 0);
    
    /** Returns the state of the specified record/set field in the current
      * combination. 
      * 
      * @param p_rec_value specifies the record value
      * @param p_comp field index inside the record value */
    omit_state_t get_state(Common::Value* p_rec_value, size_t p_comp) const;
    
  private:
    /** Walks through the given value and all of its components (recursively),
      * creates a map entry for each record and set value found. The entry contains
      * which field indexes are omitted (set to OMITTED_ABSENT) and which aren't
      * (set to NOT_OMITTED). */
    void parse_value(Common::Value* p_value);
    
    /** Helper function for the next() function.
      * Goes through the first values in the map and sets their omitted fields'
      * states to OMITTED_ABSENT.
      *
      * @param value_count specifies how many values to reset */
    void reset_previous(size_t value_count);

    /** Map containing the current combination.
      * Key: pointer to a record or set value in the AST (not owned)
      * Value: integer array, each integer contains the JSON omitted state of a
      * field in * the record/set specified by the key, the array's length is equal
      * to the number of fields in the record/set (owned) */
    map<Common::Value*, int> combinations;
  };
}

#endif	/* TTCN2JSON_HH */

