/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef TYPECOMPAT_HH_
#define TYPECOMPAT_HH_

#include "string.hh"
#include "stack.hh"
#include "vector.hh"

struct expression_struct_t;

namespace Ttcn {
  class TemplateInstance;
}

namespace Common {
  class Module;
  class Type;
  class GovernedSimple;
  class Reference;

/** Compatibility information for error reporting during semantic checks and
 *  some helpers for code generation.  Used during semantic checks only.  */
class TypeCompatInfo {
private:
  Module *m_my_module;
  Type *m_type1;            /**< Type of the LHS. Not owned. */
  Type *m_type2;            /**< Type of the RHS. Not owned. */
  bool m_strict;            /**< Type compatibility means equivalence. */
  bool m_is_temp;           /**< Compatibility between templates. */
  bool m_needs_conversion;  /**< Need for conversion code generation. */
  bool m_erroneous;         /**< Set if type conversion is not possible. */
  string m_error_str;       /**< Additional information. */
  string m_ref_str1;        /**< Reference chain for the LHS.  */
  string m_ref_str2;        /**< Reference chain for the RHS.  */
  string subtype_error_str; /**< Subtype error string */
  bool str1_elem;           /**< LHS is a reference to a string element */
  bool str2_elem;           /**< RHS is a reference to a string element */
  /// Copy constructor disabled
  TypeCompatInfo(const TypeCompatInfo&);
  /// Assignment disabled
  TypeCompatInfo& operator=(const TypeCompatInfo&);
public:
  TypeCompatInfo(Module *p_my_module, Type *p_type1 = NULL,
                 Type *p_type2 = NULL, bool p_add_ref_str = true,
                 bool p_strict = true, bool p_is_temp = false);
  inline bool needs_conversion() const { return m_needs_conversion; }
  inline void set_needs_conversion(bool p_needs_conversion)
    { m_needs_conversion = p_needs_conversion; }
  void add_type_conversion(Type *p_from_type, Type *p_to_type);
  void append_ref_str(int p_ref_no, const string& p_ref);
  inline const string& get_ref_str(int p_ref_no)
    { return p_ref_no == 0 ? m_ref_str1 : m_ref_str2; }
  inline Type *get_type(int p_type_no) const
    { return p_type_no == 0 ? m_type1 : m_type2; }
  inline bool is_strict() const { return m_strict; }
  inline bool is_erroneous() const { return m_erroneous; }
  inline Module *get_my_module() const { return m_my_module; }
  void set_is_erroneous(Type *p_type1, Type *p_type2,
                        const string& p_error_str);
  inline const string& get_error_str() { return m_error_str; }
  /** Assemble the error message.  */
  string get_error_str_str() const;
  void set_subtype_error(const string error_str) { subtype_error_str = error_str; }
  bool is_subtype_error() const { return !subtype_error_str.empty(); }
  string get_subtype_error() { return subtype_error_str; }
  void set_str1_elem(bool p_str1_elem) { str1_elem = p_str1_elem; }
  void set_str2_elem(bool p_str2_elem) { str2_elem = p_str2_elem; }
  bool get_str1_elem() const { return str1_elem; }
  bool get_str2_elem() const { return str2_elem; }
};

class TypeConv {
private:
  Type *m_from;
  Type *m_to;
  bool m_is_temp;
public:
  TypeConv(Type *p_from, Type *p_to, bool p_is_temp) : m_from(p_from),
    m_to(p_to), m_is_temp(p_is_temp) { }
  inline Type *get_from_type() const { return m_from; }
  inline Type *get_to_type() const { return m_to; }
  inline bool is_temp() const { return m_is_temp; }
  /** Assemble the name of the generated conversion function.  Take care of
   *  types declared in other modules.  */
  static string get_conv_func(Type *p_from, Type *p_to, Module *p_mod);
  static bool needs_conv_refd(GovernedSimple *p_val_or_temp);
  /** Generate initial conversion code for a given value or template
   *  reference.  */
  static char *gen_conv_code_refd(char *str, const char *name,
                                  GovernedSimple *p_val_or_temp);
  void gen_conv_func(char **p_prototypes, char **p_bodies, Module *p_mod);
  void gen_conv_func_record_set(char **p_bodies, Module *p_mod);
  void gen_conv_func_array_record(char **p_bodies, Module *p_mod);
  void gen_conv_func_array_record_of(char **p_bodies, Module *p_mod);
  void gen_conv_func_record_set_record_of_set_of(char **p_bodies,
                                                 Module *p_mod);
  void gen_conv_func_choice_anytype(char **p_bodies, Module *p_mod);
};

/** Class to detect recursion in the type hierarchy.  It is used by
 *  is_compatible*() functions to check if the two types in question
 *  reference each other in a way that would cause problems (e.g. infinite
 *  recursion) during the semantic analysis.  */
class TypeChain {
private:
  vector<Type> m_chain;
  stack<int> m_marked_states;
  int m_first_double;
public:
  TypeChain();
  ~TypeChain();
  inline bool has_recursion() const { return m_first_double != -1; }
  /** Check if this is the first level of the type hierarchy.  The "root"
   *  types are always added at first and removed only by the
   *  destructor.  */
  inline bool empty() const { return m_chain.empty(); }
  void add(Type *p_type);
  void mark_state();
  void previous_state();
};

} /* namespace Common */
#endif /* TYPECOMPAT_HH_ */
