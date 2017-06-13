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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef PARAM_TYPES_H
#define PARAM_TYPES_H

#include <stddef.h>
#include "RInt.hh"
#include "Types.h"
#include "memory.h"
#include "Vector.hh"
#include <cstdio>


// the following structures are used in the lexer/parser (ugly historical stuff)
struct param_objid_t {
  int n_components;
  int *components_ptr;
};
struct param_bitstring_t {
  int n_bits;
  unsigned char *bits_ptr;
};
struct param_hexstring_t {
  int n_nibbles;
  unsigned char *nibbles_ptr;
};
struct param_octetstring_t {
  int n_octets;
  unsigned char *octets_ptr;
};
struct param_charstring_t {
  int n_chars;
  char *chars_ptr;
};
struct param_universal_charstring_t {
  int n_uchars;
  universal_char *uchars_ptr;
};

///////////////////////////////////////////////////////////////////////////////

class Module_Param_Id {
  Module_Param_Id(const Module_Param_Id& p); // copy constructor disabled
  Module_Param_Id& operator=(const Module_Param_Id& p); // assignment disabled
public:
  Module_Param_Id() {}
  virtual ~Module_Param_Id() {}
  virtual boolean is_explicit() const = 0;
  virtual boolean is_index() const { return FALSE; }
  virtual boolean is_custom() const { return FALSE; }
  virtual size_t get_index() const;
  virtual char* get_name() const;
  virtual char* get_current_name() const;
  virtual boolean next_name();
  virtual void reset();
  virtual size_t get_nof_names() const;
  virtual char* get_str() const = 0; // returns an expstring that must be deallocated
};

class Module_Param_Name : public Module_Param_Id {
  /** The first elements are the module name (if any) and the module parameter name,
    * followed by record/set field names and array (or record of/set of) indexes.
    * Since the names of modules, module parameters and fields cannot start with
    * numbers, the indexes are easily distinguishable from these elements. */
  Vector<char*> names;
  /** The position of the current element being checked (in vector 'names') */
  size_t pos;
public:
  Module_Param_Name(const Vector<char*>& p): names(p), pos(0) {}
  ~Module_Param_Name() { for (size_t i = 0; i < names.size(); ++i) Free(names[i]); }
  boolean is_explicit() const { return TRUE; }
  char* get_current_name() const { return names[pos]; }
  boolean next_name() {
    if (pos + 1 >= names.size()) return FALSE;
    ++pos;
    return TRUE;
  }
  void reset() { pos = 0; }
  size_t get_nof_names() const { return names.size(); }
  char* get_str() const;
};

class Module_Param_FieldName : public Module_Param_Id {
  char* name; // owned expstring_t
public:
  Module_Param_FieldName(char* p): name(p) {}
  ~Module_Param_FieldName() { Free(name); }
  char* get_name() const { return name; }
  boolean is_explicit() const { return TRUE; }
  char* get_str() const;
};

class Module_Param_Index : public Module_Param_Id {
  size_t index;
  boolean is_expl;
public:
  Module_Param_Index(size_t p_index, boolean p_is_expl): index(p_index), is_expl(p_is_expl) {}
  size_t get_index() const { return index; }
  boolean is_index() const { return TRUE; }
  boolean is_explicit() const { return is_expl; }
  char* get_str() const;
};

/** Custom module parameter name class, used in Module_Param instances that aren't
  * actual module parameters (length boundaries, array indexes and character codes in
  * quadruples use temporary Module_Param instances to allow the use of expressions
  * and references to module parameters).
  * Errors reported in these cases will contain the custom text set in this class,
  * instead of the regular error message header. */
class Module_Param_CustomName : public Module_Param_Id {
  char* name; // owned expstring_t
public:
  Module_Param_CustomName(char* p): name(p) {}
  ~Module_Param_CustomName() { Free(name); }
  char* get_name() const { return name; }
  boolean is_explicit() const { return TRUE; }
  char* get_str() const;
  boolean is_custom() const { return TRUE; }
};

///////////////////////////////////////////////////////////////////////////////

class Module_Param_Length_Restriction {
  Module_Param_Length_Restriction(const Module_Param_Length_Restriction& p); // copy constructor disabled
  Module_Param_Length_Restriction& operator=(const Module_Param_Length_Restriction& p); // assignment disabled
  size_t min;
  boolean has_max;
  size_t max;
public:
  Module_Param_Length_Restriction(): min(0), has_max(FALSE), max(0) {}
  void set_single(size_t p_single) { has_max=TRUE; min = max = p_single; }
  void set_min(size_t p_min) { min=p_min; }
  void set_max(size_t p_max) { has_max=TRUE; max=p_max; }
  size_t get_min() const { return min; }
  boolean get_has_max() const { return has_max; }
  size_t get_max() const { return max; }
  boolean is_single() const { return has_max && min==max; }
  void log() const;
};

///////////////////////////////////////////////////////////////////////////////

// forward declaration
class Module_Param_Ptr;

class Module_Param {
  Module_Param(const Module_Param& p); // copy constructor disabled
  Module_Param& operator=(const Module_Param& p); // assignment disabled

public:
  enum type_t { // list of all derived classes that can be instantiated
  MP_NotUsed,
  MP_Omit,
  MP_Integer,
  MP_Float,
  MP_Boolean,
  MP_Verdict,
  MP_Objid,
  MP_Bitstring,
  MP_Hexstring,
  MP_Octetstring,
  MP_Charstring,
  MP_Universal_Charstring,
  MP_Enumerated,
  MP_Ttcn_Null,
  MP_Ttcn_mtc,
  MP_Ttcn_system,
  MP_Asn_Null,
  MP_Any,
  MP_AnyOrNone,
  MP_IntRange,
  MP_FloatRange,
  MP_StringRange,
  MP_Pattern,
  MP_Bitstring_Template,
  MP_Hexstring_Template,
  MP_Octetstring_Template,
  MP_Assignment_List,
  MP_Value_List,
  MP_Indexed_List,
  MP_List_Template,
  MP_ComplementList_Template,
  MP_Superset_Template,
  MP_Subset_Template,
  MP_Permutation_Template,
  MP_Reference,
  MP_Unbound,
  MP_Expression
  };
  enum operation_type_t { OT_ASSIGN, OT_CONCAT };
  enum basic_check_bits_t { // used to parametrize basic_check()
    BC_VALUE =    0x00, // non-list values
    BC_LIST =     0x01, // list values and templates
    BC_TEMPLATE = 0x02  // templates
  };
  enum expression_operand_t { // expression types for MP_Expression
    EXPR_ERROR, // for reporting errors
    EXPR_ADD,
    EXPR_SUBTRACT,
    EXPR_MULTIPLY,
    EXPR_DIVIDE,
    EXPR_CONCATENATE,
    EXPR_NEGATE // only operand1 is used
  };

protected:
  operation_type_t operation_type;
  Module_Param_Id* id; // owned
  Module_Param* parent; // not owned, NULL if no parent
  boolean has_ifpresent; // true if 'ifpresent' was used
  Module_Param_Length_Restriction* length_restriction; // owned, NULL if no length restriction

public:
  // set default values, these shall be changed using member functions or constructors of derived classes
  Module_Param(): operation_type(OT_ASSIGN), id(NULL), parent(NULL), has_ifpresent(FALSE), length_restriction(NULL) {}
  virtual ~Module_Param() { delete id; delete length_restriction; }

  virtual type_t get_type() const = 0;
  void set_parent(Module_Param* p_parent) { parent = p_parent; }
  void set_id(Module_Param_Id* p_id);
  Module_Param_Id* get_id() const; // returns the Id or error, never returns NULL (because every module parameter 
                                   // should have either an explicit or an implicit id when this is called)
  void set_ifpresent() { has_ifpresent = TRUE; }
  boolean get_ifpresent() const { return has_ifpresent; }
  void set_length_restriction(Module_Param_Length_Restriction* p_length_restriction);
  Module_Param_Length_Restriction* get_length_restriction() const { return length_restriction; }
  operation_type_t get_operation_type() const { return operation_type; }
  void set_operation_type(operation_type_t p_optype) { operation_type = p_optype; }
  const char* get_operation_type_str() const;
  const char* get_operation_type_sign_str() const;
  virtual const char* get_type_str() const = 0;
  void log(boolean log_id = TRUE) const;
  virtual void log_value() const = 0;
  char* get_param_context() const;

  // error reporter functions
  void error(const char* err, ...) const
    __attribute__ ((__format__ (__printf__, 2, 3), __noreturn__));

  void type_error(const char* expected, const char* type_name = NULL) const
    __attribute__ ((__noreturn__));

  inline void expr_type_error(const char* type_name) const
    __attribute__ ((__noreturn__)) {
    error("%s is not allowed in %s expression.",
      get_expr_type_str(), type_name);
  }

  // check and error report function for operation type, ifpresent and length restriction
  void basic_check(int check_type, const char* what) const;

  // add element to compound type, error if not compound type
  virtual void add_elem(Module_Param* value);
  virtual void add_list_with_implicit_ids(Vector<Module_Param*>* mp_list);

  // try to access data of a given type, error if it's another type
  virtual boolean get_boolean() const;
  virtual size_t get_size() const;
  virtual Module_Param* get_elem(size_t index) const;
  virtual int get_string_size() const;
  virtual void* get_string_data() const;
  virtual int_val_t* get_lower_int() const;
  virtual int_val_t* get_upper_int() const;
  virtual boolean get_is_min_exclusive() const;
  virtual boolean get_is_max_exclusive() const;
  virtual double get_lower_float() const;
  virtual double get_upper_float() const;
  virtual boolean has_lower_float() const;
  virtual boolean has_upper_float() const;
  virtual universal_char get_lower_uchar() const;
  virtual universal_char get_upper_uchar() const;
  virtual int_val_t* get_integer() const;
  virtual double get_float() const;
  virtual char* get_pattern() const;
  virtual boolean get_nocase() const;
  virtual verdicttype get_verdict() const;
  virtual char* get_enumerated() const;
#ifdef TITAN_RUNTIME_2
  virtual Module_Param_Ptr get_referenced_param() const;
#endif
  virtual expression_operand_t get_expr_type() const;
  virtual const char* get_expr_type_str() const;
  virtual Module_Param* get_operand1() const;
  virtual Module_Param* get_operand2() const;
};

/** Smart pointer class for Module_Param instances
  * Uses a reference counter so the Module_Param object is never copied.
  * Deletes the object (if it's temporary), when the reference counter reaches zero. */
class Module_Param_Ptr {
  struct module_param_ptr_struct {
    Module_Param* mp_ptr;
    boolean temporary;
    int ref_count;
  } *ptr;
  void clean_up();
public:
  Module_Param_Ptr(Module_Param* p);
  Module_Param_Ptr(const Module_Param_Ptr& r);
  ~Module_Param_Ptr() { clean_up(); }
  Module_Param_Ptr& operator=(const Module_Param_Ptr& r);
  void set_temporary() { ptr->temporary = TRUE; }
  Module_Param& operator*() { return *ptr->mp_ptr; }
  Module_Param* operator->() { return ptr->mp_ptr; }
};

#ifdef TITAN_RUNTIME_2
/** Module parameter reference (and enumerated value)
  * Stores a reference to another module parameter, that can be retrieved with the
  * method get_referenced_param().
  * @note Enumerated values are stored as references (with only 1 name segment),
  * since the parser cannot distinguish them. */
class Module_Param_Reference : public Module_Param {
  Module_Param_Name* mp_ref;
public:
  type_t get_type() const { return MP_Reference; }
  Module_Param_Reference(Module_Param_Name* p);
  ~Module_Param_Reference() { delete mp_ref; }
  Module_Param_Ptr get_referenced_param() const;
  char* get_enumerated() const;
  const char* get_type_str() const { return "module parameter reference"; }
  void log_value() const;
};

/** Unbound module parameter
  * This cannot be created by the parser, only by get_referenced_param(), when
  * the referenced module parameter is unbound. */
class Module_Param_Unbound : public Module_Param {
  type_t get_type() const { return MP_Unbound; }
  const char* get_type_str() const { return "<unbound>"; }
  void log_value() const;
};
#endif

/** Module parameter expression
  * Contains an unprocessed module parameter expression with one or two operands.
  * Expression types:
  * with 2 operands: +, -, *, /, &
  * with 1 operand: - (unary + is handled by the parser). */
class Module_Param_Expression : public Module_Param {
private:
  expression_operand_t expr_type;
  Module_Param* operand1;
  Module_Param* operand2;
public:
  Module_Param_Expression(expression_operand_t p_type, Module_Param* p_op1,
    Module_Param* p_op2);
  Module_Param_Expression(Module_Param* p_op);
  ~Module_Param_Expression();
  expression_operand_t get_expr_type() const { return expr_type; }
  const char* get_expr_type_str() const;
  Module_Param* get_operand1() const { return operand1; }
  Module_Param* get_operand2() const { return operand2; }
  type_t get_type() const { return MP_Expression; }
  const char* get_type_str() const { return "expression"; }
  void log_value() const;
};

class Module_Param_NotUsed : public Module_Param {
public:
  type_t get_type() const { return MP_NotUsed; }
  const char* get_type_str() const { return "-"; }
  void log_value() const;
};

class Module_Param_Omit : public Module_Param {
public:
  type_t get_type() const { return MP_Omit; }
  const char* get_type_str() const { return "omit"; }
  void log_value() const;
};

class Module_Param_Integer : public Module_Param {
  int_val_t* integer_value;
public:
  type_t get_type() const { return MP_Integer; }
  Module_Param_Integer(int_val_t* p);
  ~Module_Param_Integer() { delete integer_value; }
  int_val_t* get_integer() const { return integer_value; }
  const char* get_type_str() const { return "integer"; }
  void log_value() const;
};

class Module_Param_Float : public Module_Param {
  double float_value;
public:
  type_t get_type() const { return MP_Float; }
  Module_Param_Float(double p): float_value(p) {}
  double get_float() const { return float_value; }
  const char* get_type_str() const { return "float"; }
  void log_value() const;
};

class Module_Param_Boolean : public Module_Param {
  boolean boolean_value;
public:
  type_t get_type() const { return MP_Boolean; }
  Module_Param_Boolean(boolean p): boolean_value(p) {}
  boolean get_boolean() const { return boolean_value; }
  const char* get_type_str() const { return "boolean"; }
  void log_value() const;
};

class Module_Param_Verdict : public Module_Param {
  verdicttype verdict_value;
public:
  type_t get_type() const { return MP_Verdict; }
  Module_Param_Verdict(verdicttype p): verdict_value(p) {}
  verdicttype get_verdict() const { return verdict_value; }
  const char* get_type_str() const { return "verdict"; }
  void log_value() const;
};

template <typename CHARTYPE>
class Module_Param_String : public Module_Param {
protected:
  int n_chars;
  CHARTYPE* chars_ptr;
public:
  Module_Param_String(int p_n, CHARTYPE* p_c): n_chars(p_n), chars_ptr(p_c) {}
  ~Module_Param_String() { Free(chars_ptr); }
  int get_string_size() const { return n_chars; }
  void* get_string_data() const { return (void*)chars_ptr; }
};

class Module_Param_Objid : public Module_Param_String<int> { // special string of integers :)
public:
  type_t get_type() const { return MP_Objid; }
  Module_Param_Objid(int p_n, int* p_c): Module_Param_String<int>(p_n, p_c) {}
  const char* get_type_str() const { return "object identifier"; }
  void log_value() const;
};

class Module_Param_Bitstring : public Module_Param_String<unsigned char> {
public:
  type_t get_type() const { return MP_Bitstring; }
  Module_Param_Bitstring(int p_n, unsigned char* p_c): Module_Param_String<unsigned char>(p_n, p_c) {}
  const char* get_type_str() const { return "bitstring"; }
  void log_value() const;
};

class Module_Param_Hexstring : public Module_Param_String<unsigned char> {
public:
  type_t get_type() const { return MP_Hexstring; }
  Module_Param_Hexstring(int p_n, unsigned char* p_c): Module_Param_String<unsigned char>(p_n, p_c) {}
  const char* get_type_str() const { return "hexstring"; }
  void log_value() const;
};

class Module_Param_Octetstring : public Module_Param_String<unsigned char> {
public:
  type_t get_type() const { return MP_Octetstring; }
  Module_Param_Octetstring(int p_n, unsigned char* p_c): Module_Param_String<unsigned char>(p_n, p_c) {}
  const char* get_type_str() const { return "octetstring"; }
  void log_value() const;
};

class Module_Param_Charstring : public Module_Param_String<char> {
public:
  type_t get_type() const { return MP_Charstring; }
  Module_Param_Charstring(int p_n, char* p_c): Module_Param_String<char>(p_n, p_c) {}
  const char* get_type_str() const { return "charstring"; }
  void log_value() const;
};

class Module_Param_Universal_Charstring : public Module_Param_String<universal_char> {
public:
  type_t get_type() const { return MP_Universal_Charstring; }
  Module_Param_Universal_Charstring(int p_n, universal_char* p_c)
  : Module_Param_String<universal_char>(p_n, p_c) {}
  const char* get_type_str() const { return "universal charstring"; }
  void log_value() const;
};

class Module_Param_Enumerated : public Module_Param {
  char* enum_value;
public:
  type_t get_type() const { return MP_Enumerated; }
  Module_Param_Enumerated(char* p_e): enum_value(p_e) {}
  ~Module_Param_Enumerated() { Free(enum_value); }
  char* get_enumerated() const { return enum_value; }
  const char* get_type_str() const { return "enumerated"; }
  void log_value() const;
};

class Module_Param_Ttcn_Null : public Module_Param {
public:
  type_t get_type() const { return MP_Ttcn_Null; }
  const char* get_type_str() const { return "null"; }
  void log_value() const;
};

class Module_Param_Ttcn_mtc : public Module_Param {
public:
  type_t get_type() const { return MP_Ttcn_mtc; }
  const char* get_type_str() const { return "mtc"; }
  void log_value() const;
};

class Module_Param_Ttcn_system : public Module_Param {
public:
  type_t get_type() const { return MP_Ttcn_system; }
  const char* get_type_str() const { return "system"; }
  void log_value() const;
};

class Module_Param_Asn_Null : public Module_Param {
public:
  type_t get_type() const { return MP_Asn_Null; }
  const char* get_type_str() const { return "NULL"; }
  void log_value() const;
};

class Module_Param_Any : public Module_Param {
public:
  type_t get_type() const { return MP_Any; }
  const char* get_type_str() const { return "?"; }
  void log_value() const;
};

class Module_Param_AnyOrNone : public Module_Param {
public:
  type_t get_type() const { return MP_AnyOrNone; }
  const char* get_type_str() const { return "*"; }
  void log_value() const;
};

class Module_Param_IntRange : public Module_Param {
  int_val_t* lower_bound; // NULL == -infinity
  int_val_t* upper_bound; // NULL == infinity
  boolean min_exclusive, max_exclusive;
public:
  type_t get_type() const { return MP_IntRange; }
  Module_Param_IntRange(int_val_t* p_l, int_val_t* p_u, boolean min_is_exclusive, boolean max_is_exclusive):
    lower_bound(p_l), upper_bound(p_u), min_exclusive(min_is_exclusive), max_exclusive(max_is_exclusive) {}
  ~Module_Param_IntRange() { Free(lower_bound); Free(upper_bound); }
  int_val_t* get_lower_int() const { return lower_bound; }
  int_val_t* get_upper_int() const { return upper_bound; }
  const char* get_type_str() const { return "integer range"; }
  boolean get_is_min_exclusive() const { return min_exclusive; }
  boolean get_is_max_exclusive() const { return max_exclusive; }
  void log_value() const;
  static void log_bound(int_val_t* bound, boolean is_lower);
};

class Module_Param_FloatRange : public Module_Param {
  double lower_bound;
  boolean has_lower;
  double upper_bound;
  boolean has_upper;
  boolean min_exclusive, max_exclusive;
public:
  type_t get_type() const { return MP_FloatRange; }
  Module_Param_FloatRange(double p_lb, boolean p_hl, double p_ub, boolean p_hu, boolean min_is_exclusive, boolean max_is_exclusive): lower_bound(p_lb), has_lower(p_hl), upper_bound(p_ub), has_upper(p_hu), min_exclusive(min_is_exclusive), max_exclusive(max_is_exclusive) {}
  double get_lower_float() const { return lower_bound; }
  double get_upper_float() const { return upper_bound; }
  boolean has_lower_float() const { return has_lower; }
  boolean has_upper_float() const { return has_upper; }
  const char* get_type_str() const { return "float range"; }
  boolean get_is_min_exclusive() const { return min_exclusive; }
  boolean get_is_max_exclusive() const { return max_exclusive; }
  void log_value() const;
};

class Module_Param_StringRange : public Module_Param {
  universal_char lower_bound;
  universal_char upper_bound;
  boolean min_exclusive, max_exclusive;
public:
  type_t get_type() const { return MP_StringRange; }
  Module_Param_StringRange(const universal_char& p_lb, const universal_char& p_ub, boolean min_is_exclusive, boolean max_is_exclusive): lower_bound(p_lb), upper_bound(p_ub), min_exclusive(min_is_exclusive), max_exclusive(max_is_exclusive) {}
  universal_char get_lower_uchar() const { return lower_bound; }
  universal_char get_upper_uchar() const { return upper_bound; }
  const char* get_type_str() const { return "char range"; }
  boolean get_is_min_exclusive() const { return min_exclusive; }
  boolean get_is_max_exclusive() const { return max_exclusive; }
  void log_value() const;
};

class Module_Param_Pattern : public Module_Param {
  char* pattern;
  boolean nocase;
public:
  type_t get_type() const { return MP_Pattern; }
  Module_Param_Pattern(char* p_p, boolean p_nc): pattern(p_p), nocase(p_nc) {}
  ~Module_Param_Pattern() { Free(pattern); }
  char* get_pattern() const { return pattern; }
  boolean get_nocase() const { return nocase; }
  const char* get_type_str() const { return "pattern"; }
  void log_value() const;
};

class Module_Param_Bitstring_Template : public Module_Param_String<unsigned char> { // template parameter type is taken from String_struct.hh (B/H/O-string template structures)
public:
  type_t get_type() const { return MP_Bitstring_Template; }
  Module_Param_Bitstring_Template(int p_n, unsigned char* p_c): Module_Param_String<unsigned char>(p_n, p_c) {}
  const char* get_type_str() const { return "bitstring template"; }
  void log_value() const;
};

class Module_Param_Hexstring_Template : public Module_Param_String<unsigned char> {
public:
  type_t get_type() const { return MP_Hexstring_Template; }
  Module_Param_Hexstring_Template(int p_n, unsigned char* p_c): Module_Param_String<unsigned char>(p_n, p_c) {}
  const char* get_type_str() const { return "hexstring template"; }
  void log_value() const;
};

class Module_Param_Octetstring_Template : public Module_Param_String<unsigned short> {
public:
  type_t get_type() const { return MP_Octetstring_Template; }
  Module_Param_Octetstring_Template(int p_n, unsigned short* p_c): Module_Param_String<unsigned short>(p_n, p_c) {}
  const char* get_type_str() const { return "octetstring template"; }
  void log_value() const;
};

class Module_Param_Compound : public Module_Param {
  Vector<Module_Param*> values; // Module_Param instances are owned
public:
  Module_Param_Compound() {}
  ~Module_Param_Compound();
  void log_value_vec(const char* begin_str, const char* end_str) const;
  void add_elem(Module_Param* value);
  // add a list of params, adds to each one an implicit index based on 
  // the position in the values vector
  void add_list_with_implicit_ids(Vector<Module_Param*>* mp_list);
  size_t get_size() const;
  Module_Param* get_elem(size_t index) const;
};

class Module_Param_Assignment_List : public Module_Param_Compound {
public:
  type_t get_type() const { return MP_Assignment_List; }
  const char* get_type_str() const { return "list with assignment notation"; }
  void log_value() const { log_value_vec("{","}"); }
};

class Module_Param_Value_List : public Module_Param_Compound {
public:
  type_t get_type() const { return MP_Value_List; }
  const char* get_type_str() const { return "value list"; }
  void log_value() const { log_value_vec("{","}"); }
};

class Module_Param_Indexed_List : public Module_Param_Compound {
public:
  type_t get_type() const { return MP_Indexed_List; }
  const char* get_type_str() const { return "indexed value list"; }
  void log_value() const { log_value_vec("{","}"); }
};

class Module_Param_List_Template : public Module_Param_Compound {
public:
  type_t get_type() const { return MP_List_Template; }
  const char* get_type_str() const { return "list template"; }
  void log_value() const { log_value_vec("(",")"); }
};

class Module_Param_ComplementList_Template : public Module_Param_Compound {
public:
  type_t get_type() const { return MP_ComplementList_Template; }
  const char* get_type_str() const { return "complemented list template"; }
  void log_value() const { log_value_vec("complement(",")"); }
};

class Module_Param_Superset_Template : public Module_Param_Compound {
public:
  type_t get_type() const { return MP_Superset_Template; }
  const char* get_type_str() const { return "superset template"; }
  void log_value() const { log_value_vec("superset(",")"); }
};

class Module_Param_Subset_Template : public Module_Param_Compound {
public:
  type_t get_type() const { return MP_Subset_Template; }
  const char* get_type_str() const { return "subset template"; }
  void log_value() const { log_value_vec("subset(",")"); }
};

class Module_Param_Permutation_Template : public Module_Param_Compound {
public:
  type_t get_type() const { return MP_Permutation_Template; }
  const char* get_type_str() const { return "permutation template"; }
  void log_value() const { log_value_vec("permutation(",")"); }
};

class Ttcn_String_Parsing {
private: // only instantiation can set it to true and destruction set it back to false
  static boolean string_parsing;
public:
  Ttcn_String_Parsing() { string_parsing = TRUE; }
  ~Ttcn_String_Parsing() { string_parsing = FALSE; }
  static boolean happening() { return string_parsing; }
};

class Debugger_Value_Parsing {
private: // only instantiation can set it to true and destruction set it back to false
  static boolean is_happening;
public:
  Debugger_Value_Parsing() { is_happening = TRUE; }
  ~Debugger_Value_Parsing() { is_happening = FALSE; }
  static boolean happening() { return is_happening; }
};

/** Use the configuration file parser to convert a string into a TTCN-3 value.
  * @param mp_str the converted string (used as if it were a module parameter in a
  * config file)
  * @param is_component true, if the expected TTCN-3 value is a component
  * @return the TTCN-3 value in module parameter form (will be set using set_param)
  * @tricky Component names (given with the "start" command) can contain any characters.
  * This conflicts with several other rules, so certain rules in the parser will only
  * be applied to components. */
extern Module_Param* process_config_string2ttcn(const char* mp_str, boolean is_component);

extern Module_Param* process_config_debugger_value(const char* mp_str);

#endif
