/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Cserveni, Akos
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#ifndef _Subtype_HH
#define _Subtype_HH

#include "ttcn3/compiler.h"
#include "vector.hh"
#include "map.hh"
#include "Int.hh"
#include "Real.hh"
#include "ustring.hh"
#include "Setting.hh"

#include "subtypestuff.hh"

class JSON_Tokenizer;

namespace Ttcn {
  class LengthRestriction;
  class Template;
  class ValueRange;
  class PatternString;
}

namespace Common {

  class Identifier;
  class Value;
  class Type;
  class Constraints;

/**
 * class for parsing type restrictions
 */
class SubTypeParse {
public:
  enum STPselection { STP_SINGLE, STP_RANGE, STP_LENGTH, STP_PATTERN };
private:
  /// Type selector
  STPselection selection;
  /// Restriction info, owned and freed by the object.
  union {
    Value *single; ///< for STP_SINGLE
    struct {
      Value *min, *max;
      bool min_exclusive, max_exclusive;
    } range; ///< for STP_RANGE
    Ttcn::LengthRestriction *length; ///< for STP_LENGTH
    Ttcn::PatternString* pattern; ///< for STP_PATTERN
  };
  /** Copy constructor disabled */
  SubTypeParse(const SubTypeParse& p);
  /** Assignment disabled */
  SubTypeParse& operator=(const SubTypeParse& p);
public:
  /** Construct a single value restriction */
  SubTypeParse(Value *p_single);
  /** Construct a range restriction
   *
   * @param p_min lower bound
   * @param p_max upper bound
   *
   */
  SubTypeParse(Value *p_min, bool p_min_exclusive, Value *p_max, bool p_max_exclusive);
  /** Construct from a length restriction
   *
   * @param p_length length restriction, SubTypeParse becomes owner
   */
  SubTypeParse(Ttcn::LengthRestriction *p_length);
  /** Construct from a pattern
   *
   * @param p_pattern pattern, SubTypeParse becomes owner
   */
  SubTypeParse(Ttcn::PatternString *p_pattern);
  /// Destructor
  ~SubTypeParse();

  /// Return the restriction type
  STPselection get_selection() const { return selection; }

  /** Return the single value of the restriction.
   *
   * @pre selection is STP_SINGLE, or else FATAL_ERROR
   */
  Value *Single() const;
  /** Return the lower bound of the range.
   *
   * @pre selection is STP_RANGE, or else FATAL_ERROR
   */
  Value *Min() const;
  bool MinExclusive() const;
  /** Return the upper bound of the range.
   *
   * @pre selection is STP_RANGE, or else FATAL_ERROR
   */
  Value *Max() const;
  bool MaxExclusive() const;
  /** Return the length restriction object.
   *
   * @pre selection is STP_LENGTH, or else FATAL_ERROR
   */
  Ttcn::LengthRestriction *Length() const;
  Ttcn::PatternString *Pattern() const;
};

/**
 * Container for all possible subtype constraint classes
 */
class SubtypeConstraint
{
public:
  enum subtype_t {
    ST_ERROR,
    ST_INTEGER,
    ST_FLOAT,
    ST_BOOLEAN,
    ST_OBJID,
    ST_VERDICTTYPE,
    ST_BITSTRING,
    ST_HEXSTRING,
    ST_OCTETSTRING,
    ST_CHARSTRING,
    ST_UNIVERSAL_CHARSTRING,
    ST_RECORD,
    ST_RECORDOF,
    ST_SET,
    ST_SETOF,
    ST_ENUM,
    ST_UNION,
    ST_FUNCTION,
    ST_ALTSTEP,
    ST_TESTCASE
  };
protected:
  subtype_t subtype;
  union {
    IntegerRangeListConstraint* integer_st; // ST_INTEGER
    RealRangeListConstraint* float_st; // ST_FLOAT
    BooleanListConstraint* boolean_st; // ST_BOOLEAN
    VerdicttypeListConstraint* verdict_st; // ST_VERDICTTYPE
    BitstringConstraint* bitstring_st; // ST_BITSTRING
    HexstringConstraint* hexstring_st; // ST_HEXSTRING
    OctetstringConstraint* octetstring_st; // ST_OCTETSTRING
    CharstringSubtypeTreeElement* charstring_st; // ST_CHARSTRING
    UniversalCharstringSubtypeTreeElement* universal_charstring_st; // ST_UNIVERSAL_CHARSTRING
    RecofConstraint* recof_st; // ST_RECORDOF, ST_SETOF
    ValueListConstraint* value_st; // ST_RECORD, ST_SET, ST_UNION, ST_FUNCTION, ST_ALTSTEP, ST_TESTCASE, ST_OBJID, ST_ENUM
                                   // TODO: use more precise subtype classes for ST_OBJID and ST_ENUM
  };
  // the xxx_st values are aggregate subtype restriction sets, the distinct
  // length restrictions are lost in them, the length restriction is also stored separately
  SizeRangeListConstraint* length_restriction;

  void set_to_error(); // clears the subtype information and sets value to ST_ERROR

  /// Copy constructor disabled
  SubtypeConstraint(const SubtypeConstraint&);
  /// Assignment disabled
  SubtypeConstraint& operator=(const SubtypeConstraint&);
public:
  SubtypeConstraint(subtype_t st);
  virtual ~SubtypeConstraint() { set_to_error(); }

  /** copy the content of other to this (deletes content of this) */
  void copy(const SubtypeConstraint* other);

  /** set operations */
  void intersection(const SubtypeConstraint* other);
  void union_(const SubtypeConstraint* other);
  void except(const SubtypeConstraint* other);

  tribool is_subset(const SubtypeConstraint* other) const;

  /** special ASN.1 types (NumericString, etc.) have default subtype constraints,
      return default constraint or NULL */
  static SubtypeConstraint* get_asn_type_constraint(Type* type);
  static SubtypeConstraint* create_from_asn_value(Type* type, Value* value);
  static SubtypeConstraint* create_from_asn_charvalues(Type* type, Value* value);
  static SubtypeConstraint* create_from_contained_subtype(
    SubtypeConstraint* contained_stc, bool char_context, Location* loc);
  int_limit_t get_int_limit(bool is_upper, Location* loc); ///< helper func. of create_from_asn_range()
  static SubtypeConstraint* create_from_asn_range(
    Value* vmin, bool min_exclusive,
    Value* vmax, bool max_exclusive,
    Location* loc, subtype_t st_t, SubtypeConstraint* parent_subtype);
  static SubtypeConstraint* create_asn_size_constraint(
    SubtypeConstraint* integer_stc, bool char_context, Type* type, Location* loc);
  static SubtypeConstraint* create_permitted_alphabet_constraint(
    SubtypeConstraint* stc, bool char_context, Type* type, Location* loc);

  subtype_t get_subtypetype() const { return subtype; }
  virtual string to_string() const;
  /** Two subtypes are compatible if their intersection is not an empty set */
  bool is_compatible(const SubtypeConstraint *p_stp) const;
  /** Check if this string subtype is compatible with a string element */
  bool is_compatible_with_elem() const;
  // used to check compatibility of structured types
  bool is_length_compatible(const SubtypeConstraint *p_st) const;
  bool is_upper_limit_infinity() const;
  bool is_lower_limit_infinity() const;
};

/**
 * class for semantic analysis of subtypes
 */
class SubType : public SubtypeConstraint {

  Type *my_owner; ///< Pointer to the type, object not owned
  SubType* parent_subtype; ///< pointer to the inherited subtype, not owned
  vector<SubTypeParse> *parsed; ///< TTCN-3 parsed subtype data, not owned
  Constraints* asn_constraints; ///< constraints of ASN.1 type or NULL, not owned

  SubtypeConstraint* root; ///< root part of the ASN.1 subtype, owned by asn_constraints
  bool extendable; ///< ASN.1 extendable type
  SubtypeConstraint* extension; ///< ASN.1 extension addition, owned by asn_constraints

  enum checked_t {
    STC_NO,
    STC_CHECKING,
    STC_YES
  } checked;

  map<SubType*,void> my_parents; // used to check for circular references

  /// Copy constructor disabled
  SubType(const SubType&);
  /// Assignment disabled
  SubType& operator=(const SubType&);
public:

  SubType(subtype_t st, Type *p_my_owner, SubType* p_parent_subtype,
    vector<SubTypeParse> *p_parsed, Constraints* p_asn_constraints);
  ~SubType();

  SubtypeConstraint* get_root() { return root ? root : this; }
  bool is_extendable() { return extendable; }
  SubtypeConstraint* get_extension() { return extension; }

  string to_string() const;
  
  vector<SubTypeParse> * get_subtype_parsed() const;

  /** Set restrictions.
   *
   * @param r list of restrictions
   * @pre set_restrictions() has not been called before. */
  void set_restrictions(vector<SubTypeParse> *r);
  const Type *get_owner() const { return my_owner; }

  void chk();

  void chk_this_value(Value *value);
  void chk_this_template_generic(Ttcn::Template *templ);

  /// No-op.
  void generate_code(output_struct &);
  
  /** Returns true if there are JSON schema elements to be generated for this subtype */
  boolean has_json_schema() const { return parsed != NULL; }
  
  /** Generates the JSON schema segment for the type restrictions.
    * 
    * The float special values NaN, INF and -INF are not included in the code 
    * generated for float value list restrictions if the 2nd parameter is false. */
  void generate_json_schema(JSON_Tokenizer& json, bool allow_special_float = true);
  
  /** Generates the JSON values inside the subtype's value list restriction.
    * Recursive (it also inserts the values of referenced subtypes into the list).
    * 
    * The float special values NaN, INF and -INF are not included in the code 
    * generated for float value lists if the 2nd parameter is false. */
  void generate_json_schema_value_list(JSON_Tokenizer& json, bool allow_special_float,
    bool union_value_list);
  
  /** Generates the JSON schema elements for integer and float range restrictions.
    * If there are multiple restrictions, then they are placed in an 'anyOf' structure,
    * each one in a JSON object. The function also inserts the separators between these
    * objects (the 2nd parameter indicates whether the first range has been inserted).
    *
    * Recursive (it also inserts the value ranges of referenced subtypes).
    * @return true, if the first value range has not been inserted yet */
  bool generate_json_schema_number_ranges(JSON_Tokenizer& json, bool first = true);
  
  /** Generates the segments of the JSON schema string pattern (regex) used for
    * representing the range restrictions of charstrings and universal charstrings.
    * A value range (inside a regex set expression) is generated for each TTCN-3
    * range restriction.
    *
    * Recursive (it also inserts the string ranges of referenced subtypes). */
  char* generate_json_schema_string_ranges(char* pattern_str);
  
  /** Generates the JSON schema segment of the float type this subtype belongs to
    * (the schema segment for the whole type is generated, not only the type's
    * restrictions).
    * This replaces the schema segment generated by Type::generate_json_schema().*/
  void generate_json_schema_float(JSON_Tokenizer& json);

  void dump(unsigned level) const;

  /// return single length restriction or -1
  Int get_length_restriction() const;
  /// true unless a length restriction prohibits zero elements
  bool zero_length_allowed() const;
  /// true if the length restriction allows the specified length
  bool length_allowed(size_t len) const;

  bool add_parent_subtype(SubType* st);
private:
  void print_full_warning() const;
  bool chk_recursion(ReferenceChain& refch);
  // adding a single value/type constraint, union (works only in case of TTCN-3 BNF)
  bool add_ttcn_type_list_subtype(SubType* p_st);
  void add_ttcn_value(Value *v);
  void add_ttcn_recof(Value *v);
  bool add_ttcn_single(Value *val, size_t restriction_index);
  // adding a value range constraint, makes a union (works for TTCN-3 only)
  bool add_ttcn_range(Value *min, bool min_exclusive, Value *max, bool max_exclusive, size_t restriction_index, bool has_other);
  // adding a length constraint, makes an intersection
  bool set_ttcn_length(const size_limit_t& min, const size_limit_t& max);
  void chk_boundary_valid(Value* boundary, Int max_value, const char* boundary_name);
  bool add_ttcn_length(Ttcn::LengthRestriction *lr, size_t restriction_index);
  // adding a pattern constraint, add with intersection
  bool add_ttcn_pattern(Ttcn::PatternString* pattern, size_t restriction_index);

  void chk_this_template(Ttcn::Template *templ);
  void chk_this_template_pattern(const char *patt_type,Ttcn::Template *templ);
  void chk_this_template_length_restriction(Ttcn::Template *templ);
};// class SubType

}// namespace Common

#endif // #ifndef _Subtype_HH
