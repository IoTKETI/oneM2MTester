/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef TEMPLATE_HH
#define TEMPLATE_HH

#include "Types.h"

#ifdef TITAN_RUNTIME_2
#include "Struct_of.hh"
#include "RefdIndex.hh"
struct TTCN_Typedescriptor_t;
struct Erroneous_descriptor_t;
#endif

class Text_Buf;
class Module_Param;
class Module_Param_Name;
class Module_Param_Length_Restriction;
#ifdef TITAN_RUNTIME_2
class OCTETSTRING_template;
class HEXSTRING_template;
class BITSTRING_template;
#endif

enum template_sel {
  UNINITIALIZED_TEMPLATE = -1,
  SPECIFIC_VALUE = 0,
  OMIT_VALUE = 1,
  ANY_VALUE = 2,
  ANY_OR_OMIT = 3,
  VALUE_LIST = 4,
  COMPLEMENTED_LIST = 5,
  VALUE_RANGE = 6,
  STRING_PATTERN = 7,
  SUPERSET_MATCH = 8,
  SUBSET_MATCH = 9,
  DECODE_MATCH = 10
};

enum template_res {
  TR_VALUE,
  TR_OMIT,
  TR_PRESENT
};

#ifdef TITAN_RUNTIME_2
#define VIRTUAL_IF_RUNTIME_2 virtual
#else
#define VIRTUAL_IF_RUNTIME_2
#endif

class Base_Template {
protected:
  template_sel template_selection;
  boolean is_ifpresent;

  Base_Template();
  Base_Template(template_sel other_value);
  // Compiler-generated copy constructor and assignment are acceptable

  static void check_single_selection(template_sel other_value);
  void set_selection(template_sel other_value);
  void set_selection(const Base_Template& other_value);

  void log_generic() const;
  void log_ifpresent() const;

  void encode_text_base(Text_Buf& text_buf) const;
  void decode_text_base(Text_Buf& text_buf);

public:
  inline template_sel get_selection() const { return template_selection; }

  void set_ifpresent();

  VIRTUAL_IF_RUNTIME_2 boolean is_omit() const;
  VIRTUAL_IF_RUNTIME_2 boolean is_any_or_omit() const;
  /** Returns FALSE if \a template_selection is \a UNINITIALIZED_TEMPLATE,
  * TRUE otherwise. */
  VIRTUAL_IF_RUNTIME_2 boolean is_bound() const
    { return UNINITIALIZED_TEMPLATE != template_selection; }
  VIRTUAL_IF_RUNTIME_2 boolean is_value() const
    { return !is_ifpresent && template_selection==SPECIFIC_VALUE; }
  VIRTUAL_IF_RUNTIME_2 void clean_up()
  { template_selection = UNINITIALIZED_TEMPLATE; }
  /** return the name of template restriction \a tr */
  static const char* get_res_name(template_res tr);
  
  VIRTUAL_IF_RUNTIME_2 boolean get_istemplate_kind(const char* type) const;

  /** Initialize this object (or one of its fields/elements) with a 
    * module parameter value. The module parameter may contain references to
    * other module parameters or module parameter expressions, which are processed
    * by this method to calculated the final result.
    * @note Sets the 'ifpresent' flag if either the reference (if any) or the
    * (referenced) module parameter value have their 'ifpresent' flag
    * @note If both the reference (if any) and the (referenced) module parameter
    * have a length restriction, then the reference's length restriction is used.
    * @note A temporary object is used when setting VALUE_LIST or COMPLEMENT_LIST
    * templates, as the list of templates might contain a reference to this object.
    * @param param module parameter value (its ID specifies which object is to be set) */
  VIRTUAL_IF_RUNTIME_2 void set_param(Module_Param& param);
  
#ifdef TITAN_RUNTIME_2
  /** Create a module parameter value equivalent to this object (or one of its
    * fields/elements)
    * @param param_name module parameter ID, specifies which object to convert */
  virtual Module_Param* get_param(Module_Param_Name& param_name) const;
#endif
  
  /** not a component by default (component templates will return true) */
  inline boolean is_component() { return FALSE; }

#ifdef TITAN_RUNTIME_2
  /** set the value of the object pointed to by \a value */
  virtual void valueofv(Base_Type* value) const = 0;
  virtual void set_value(template_sel other_value) = 0;
  virtual void copy_value(const Base_Type* other_value) = 0;

  virtual Base_Template* clone() const = 0;
  virtual const TTCN_Typedescriptor_t* get_descriptor() const = 0;

  virtual void log() const = 0;

  // virtual functions for match and log_match
  virtual boolean matchv(const Base_Type* other_value, boolean legacy) const = 0;
  virtual void log_matchv(const Base_Type* match_value, boolean legacy) const = 0;

  virtual void encode_text(Text_Buf& text_buf) const = 0;
  virtual void decode_text(Text_Buf& text_buf) = 0;
  virtual boolean is_present(boolean legacy = FALSE) const = 0;
  virtual boolean match_omit(boolean legacy = FALSE) const = 0;

  virtual void check_restriction(template_res t_res, const char* t_name=NULL,
    boolean legacy = FALSE) const;

  virtual ~Base_Template() { }
#endif
};

class Restricted_Length_Template : public Base_Template
#ifdef TITAN_RUNTIME_2
  , public RefdIndexInterface
#endif
{
  
#ifdef TITAN_RUNTIME_2
  friend OCTETSTRING_template operator+(template_sel left_template_sel,
    const OCTETSTRING_template& right_template);
  friend HEXSTRING_template operator+(template_sel left_template_sel,
    const HEXSTRING_template& right_template);
  friend BITSTRING_template operator+(template_sel left_template_sel,
    const BITSTRING_template& right_template);
#endif
  
protected:
  enum length_restriction_type_t {
    NO_LENGTH_RESTRICTION = 0,
      SINGLE_LENGTH_RESTRICTION = 1,
      RANGE_LENGTH_RESTRICTION =2
  } length_restriction_type;
  union {
    int single_length;
    struct {
      int min_length, max_length;
      boolean max_length_set;
    } range_length;
  } length_restriction;

  Restricted_Length_Template();
  Restricted_Length_Template(template_sel other_value);
  // Compiler-generated copy constructor and assignment are acceptable

  void set_selection(template_sel new_selection);
  void set_selection(const Restricted_Length_Template& other_value);

public:
  boolean match_length(int value_length) const;

protected:
  int check_section_is_single(int min_size, boolean has_any_or_none,
    const char* operation_name,
    const char* type_name_prefix,
    const char* type_name) const;

  void log_restricted() const;
  void log_match_length(int value_length) const;

  void encode_text_restricted(Text_Buf& text_buf) const;
  void decode_text_restricted(Text_Buf& text_buf);

  void set_length_range(const Module_Param& param);
  Module_Param_Length_Restriction* get_length_range() const;

public:

  void set_single_length(int single_length);
  void set_min_length(int min_length);
  void set_max_length(int max_length);

  boolean is_omit() const;
  boolean is_any_or_omit() const;
};

#ifdef TITAN_RUNTIME_2
template_sel operator+(template_sel left_template_sel,
  template_sel right_template_sel);
#endif

#ifndef TITAN_RUNTIME_2

class Record_Of_Template : public Restricted_Length_Template {
protected:
  struct Pair_of_elements;
  Pair_of_elements *permutation_intervals;
  unsigned int number_of_permutations;

  Record_Of_Template();
  Record_Of_Template(template_sel new_selection);
  Record_Of_Template(const Record_Of_Template& other_value);
  ~Record_Of_Template();
  
  void clean_up_intervals();

  void set_selection(template_sel new_selection);
  void set_selection(const Record_Of_Template& other_value);

  void encode_text_permutation(Text_Buf& text_buf) const;
  void decode_text_permutation(Text_Buf& text_buf);
private:
  Record_Of_Template& operator=(const Record_Of_Template& other_value);

public:
  void add_permutation(unsigned int start_index, unsigned int end_index);
  
  /** Removes all permutations set on this template, used when template variables
    *  are given new values. */
  void remove_all_permutations() { clean_up_intervals(); }
  
  unsigned int get_number_of_permutations() const;
  unsigned int get_permutation_start(unsigned int index_value) const;
  unsigned int get_permutation_end(unsigned int index_value) const;
  unsigned int get_permutation_size(unsigned int index_value) const;
  boolean permutation_starts_at(unsigned int index_value) const;
  boolean permutation_ends_at(unsigned int index_value) const;
  boolean get_istemplate_kind(const char *type) const;
};

#else

class INTEGER;

class Set_Of_Template : public Restricted_Length_Template {
protected:
  union {
    struct {
      int n_elements;
      Base_Template** value_elements; // instances of a class derived from Base_Template
    } single_value;
    struct {
      int n_values;
      Set_Of_Template** list_value; // instances of a class derived from Set_Of_Template
    } value_list;
  };
  Erroneous_descriptor_t* err_descr;

  Set_Of_Template();
  Set_Of_Template(template_sel new_selection);
  /** Copy constructor not implemented */
  Set_Of_Template(const Set_Of_Template& other_value);
  /** Assignment disabled */
  Set_Of_Template& operator=(const Set_Of_Template& other_value);
  ~Set_Of_Template();

  void copy_template(const Set_Of_Template& other_value);
  void copy_optional(const Base_Type* other_value);

public:
  Base_Template* clone() const;
  void copy_value(const Base_Type* other_value);
  void clean_up();

  int size_of() const { return size_of(TRUE); }
  int lengthof() const { return size_of(FALSE); }
  int n_elem() const;
  void set_size(int new_size);
  void set_type(template_sel template_type, int list_length);
  boolean is_value() const;
  void valueofv(Base_Type* value) const;
  void set_value(template_sel other_value);
protected:
  Base_Template* get_at(int index_value);
  Base_Template* get_at(const INTEGER& index_value);
  const Base_Template* get_at(int index_value) const;
  const Base_Template* get_at(const INTEGER& index_value) const;
  
  int get_length_for_concat(boolean& is_any_value) const;
  static int get_length_for_concat(const Record_Of_Type& operand);
  static int get_length_for_concat(template_sel operand);
  
  void concat(int& pos, const Set_Of_Template& operand);
  void concat(int& pos, const Record_Of_Type& operand);
  void concat(int& pos);

  int size_of(boolean is_size) const;
  Set_Of_Template* get_list_item(int list_index);
  Base_Template* get_set_item(int set_index);

  void substr_(int index, int returncount, Record_Of_Type* rec_of) const;
  void replace_(int index, int len, const Set_Of_Template* repl, Record_Of_Type* rec_of) const;
  void replace_(int index, int len, const Record_Of_Type* repl, Record_Of_Type* rec_of) const;

public:
  void log() const;

  boolean matchv(const Base_Type* other_value, boolean legacy) const;
  void log_matchv(const Base_Type* match_value, boolean legacy) const;
  /** create an instance of this */
  virtual Set_Of_Template* create() const = 0;
  /** create an instance of the element class */
  virtual Base_Template* create_elem() const = 0;

  // used for both set of and record of types
  static boolean match_function_specific(
    const Base_Type *value_ptr, int value_index,
    const Restricted_Length_Template *template_ptr, int template_index,
    boolean legacy);
  // 2 static functions only for set of types
  static boolean match_function_set(
    const Base_Type *value_ptr, int value_index,
    const Restricted_Length_Template *template_ptr, int template_index,
    boolean legacy);
  static void log_function(const Base_Type *value_ptr,
    const Restricted_Length_Template *template_ptr,
    int index_value, int index_template, boolean legacy);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);
  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;

  void set_param(Module_Param& param);
  Module_Param* get_param(Module_Param_Name& param_name) const;
  
  boolean get_istemplate_kind(const char *type) const;
  
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
  void set_err_descr(Erroneous_descriptor_t* p_err_descr) { err_descr=p_err_descr; }
};

class Record_Of_Template : public Restricted_Length_Template {
protected:
  union {
    struct {
      int n_elements;
      Base_Template **value_elements; // instances of a class derived from Base_Template
    } single_value;
    struct {
      int n_values;
      Record_Of_Template** list_value; // instances of a class derived from Record_Of_Template
    } value_list;
  };
  Erroneous_descriptor_t* err_descr;

  struct Pair_of_elements;
  Pair_of_elements *permutation_intervals;
  unsigned int number_of_permutations;

  Record_Of_Template();
  Record_Of_Template(template_sel new_selection);
  /** Copy constructor not implemented */
  Record_Of_Template(const Record_Of_Template& other_value);
  /** Assignment disabled */
  Record_Of_Template& operator=(const Record_Of_Template& other_value);
  ~Record_Of_Template();
  
  void clean_up_intervals();

  void set_selection(template_sel new_selection);
  void set_selection(const Record_Of_Template& other_value);

  void encode_text_permutation(Text_Buf& text_buf) const;
  void decode_text_permutation(Text_Buf& text_buf);

  void copy_template(const Record_Of_Template& other_value);
  void copy_optional(const Base_Type* other_value);

public:
  Base_Template* clone() const;
  void copy_value(const Base_Type* other_value);
  void clean_up();

  void add_permutation(unsigned int start_index, unsigned int end_index);
  
  /** Removes all permutations set on this template, used when template variables
    *  are given new values. */
  void remove_all_permutations() { clean_up_intervals(); }

  unsigned int get_number_of_permutations() const;
  unsigned int get_permutation_start(unsigned int index_value) const;
  unsigned int get_permutation_end(unsigned int index_value) const;
  unsigned int get_permutation_size(unsigned int index_value) const;
  boolean permutation_starts_at(unsigned int index_value) const;
  boolean permutation_ends_at(unsigned int index_value) const;

  void set_size(int new_size);
  void set_type(template_sel template_type, int list_length);
  boolean is_value() const;
  void valueofv(Base_Type* value) const;
  void set_value(template_sel other_value);
protected:
  Base_Template* get_at(int index_value);
  Base_Template* get_at(const INTEGER& index_value);
  const Base_Template* get_at(int index_value) const;
  const Base_Template* get_at(const INTEGER& index_value) const;
  
  int get_length_for_concat(boolean& is_any_value) const;
  static int get_length_for_concat(const Record_Of_Type& operand);
  static int get_length_for_concat(template_sel operand);
  
  void concat(int& pos, const Record_Of_Template& operand);
  void concat(int& pos, const Record_Of_Type& operand);
  void concat(int& pos);

  int size_of(boolean is_size) const;
  Record_Of_Template* get_list_item(int list_index);

  void substr_(int index, int returncount, Record_Of_Type* rec_of) const;
  void replace_(int index, int len, const Record_Of_Template* repl, Record_Of_Type* rec_of) const;
  void replace_(int index, int len, const Record_Of_Type* repl, Record_Of_Type* rec_of) const;

public:
  int size_of() const { return size_of(TRUE); }
  int lengthof() const { return size_of(FALSE); }
  int n_elem() const;
  void log() const;

  boolean matchv(const Base_Type* other_value, boolean legacy) const;
  void log_matchv(const Base_Type* match_value, boolean legacy) const;
  /** create an instance of this */
  virtual Record_Of_Template* create() const = 0;
  /** create an instance of the element class */
  virtual Base_Template* create_elem() const = 0;

  // used for both set of and record of types
  static boolean match_function_specific(
    const Base_Type *value_ptr, int value_index,
    const Restricted_Length_Template *template_ptr, int template_index, boolean legacy);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);
  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;

  void set_param(Module_Param& param);
  Module_Param* get_param(Module_Param_Name& param_name) const;
  
  boolean get_istemplate_kind(const char *type) const;

  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
  void set_err_descr(Erroneous_descriptor_t* p_err_descr) { err_descr=p_err_descr; }
};

class Record_Template : public Base_Template
{
protected:
  union {
    struct {
      int n_elements;
      Base_Template** value_elements;
    } single_value;
    struct {
      int n_values;
      Record_Template** list_value;
    } value_list;
  };
  Erroneous_descriptor_t* err_descr;

  /** create value elements by calling their constructor */
  virtual void set_specific() = 0;

  void copy_optional(const Base_Type* other_value);
  void copy_template(const Record_Template& other_value);

  Record_Template();
  Record_Template(template_sel other_value);
  /** Copy constructor not implemented */
  Record_Template(const Record_Template& other_value);
  /** Assignment disabled */
  Record_Template& operator=(const Record_Template& other_value);
  ~Record_Template();

public:
  void copy_value(const Base_Type* other_value);
  void clean_up();

  void set_type(template_sel template_type, int list_length);
  Record_Template* get_list_item(int list_index) const;

  int size_of() const;

  boolean is_value() const;
  void valueofv(Base_Type* value) const;
  void set_value(template_sel other_value);

protected:
  Base_Template* get_at(int index_value);
  const Base_Template* get_at(int index_value) const;

  /** create an instance by calling the default constructor */
  virtual Record_Template* create() const = 0;

  virtual const char* fld_name(int /*field_index*/) const { return ""; }

public:
  Base_Template* clone() const;

  void log() const;

  boolean matchv(const Base_Type* other_value, boolean legacy) const;
  void log_matchv(const Base_Type* match_value, boolean legacy) const;

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);
  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;

  void set_param(Module_Param& param);
  Module_Param* get_param(Module_Param_Name& param_name) const;
  
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
  void set_err_descr(Erroneous_descriptor_t* p_err_descr) { err_descr=p_err_descr; }
};

class Empty_Record_Template : public Base_Template
{
protected:
  struct {
    int n_values;
    Empty_Record_Template** list_value;
  } value_list;

  void copy_optional(const Base_Type* other_value);
  void copy_template(const Empty_Record_Template& other_value);

  Empty_Record_Template();
  Empty_Record_Template(template_sel other_value);
  /** Copy constructor not implemented */
  Empty_Record_Template(const Empty_Record_Template& other_value);
  /** Assignment disabled */
  Empty_Record_Template& operator=(const Empty_Record_Template& other_value);
  ~Empty_Record_Template();

public:
  void copy_value(const Base_Type* other_value);
  void clean_up();

  void set_type(template_sel template_type, int list_length);
  Empty_Record_Template* get_list_item(int list_index) const;

  int size_of() const;

  boolean is_value() const;
  void valueofv(Base_Type* value) const;
  void set_value(template_sel other_value);

protected:
  /** create an instance by calling the default constructor */
  virtual Empty_Record_Template* create() const = 0;

public:
  Base_Template* clone() const;

  void log() const;

  boolean matchv(const Base_Type* other_value, boolean legacy) const;
  void log_matchv(const Base_Type* match_value, boolean legacy) const;

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);
  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;

  void set_param(Module_Param& param);
  Module_Param* get_param(Module_Param_Name& param_name) const;
};

#undef VIRTUAL_IF_RUNTIME_2
#endif

#endif
