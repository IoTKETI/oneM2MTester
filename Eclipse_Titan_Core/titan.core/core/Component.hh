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
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#ifndef COMPONENT_HH
#define COMPONENT_HH

#include "Types.h"
#include "Basetype.hh"
#include "Template.hh"

class Module_Param;

template<typename T>
class OPTIONAL;

// value class for all component types

class COMPONENT : public Base_Type {
  friend class COMPONENT_template;
  friend boolean operator==(component component_value,
    const COMPONENT& other_value);

  static unsigned int n_component_names;
  struct component_name_struct;
  static component_name_struct *component_names;

  component component_value;

public:
  COMPONENT();
  COMPONENT(component other_value);
  COMPONENT(const COMPONENT& other_value);

  COMPONENT& operator=(component other_value);
  COMPONENT& operator=(const COMPONENT& other_value);

  boolean operator==(component other_value) const;
  boolean operator==(const COMPONENT& other_value) const;

  inline boolean operator!=(component other_value) const
    { return !(*this == other_value); }
  inline boolean operator!=(const COMPONENT& other_value) const
    { return !(*this == other_value); }

  operator component() const;

  inline boolean is_bound() const
    { return component_value != UNBOUND_COMPREF; }
  inline boolean is_value() const
    { return component_value != UNBOUND_COMPREF; }
  inline void clean_up() { component_value = UNBOUND_COMPREF; }
  void must_bound(const char*) const;

  void log() const;

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const COMPONENT*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const COMPONENT*>(other_value)); }
  Base_Type* clone() const { return new COMPONENT(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &COMPONENT_descr_; }
  Module_Param* get_param(Module_Param_Name& param_name) const;
#else
  inline boolean is_present() const { return is_bound(); }
#endif

  alt_status done(Index_Redirect*) const;
  alt_status killed(Index_Redirect*) const;

  boolean running(Index_Redirect*) const;
  boolean alive(Index_Redirect*) const;

  void stop() const;
  void kill() const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  static void register_component_name(component component_reference,
    const char *component_name);
  static const char *get_component_name(component component_reference);
  static void clear_component_names();
  static void log_component_reference(component component_reference);
  /** Returns the symbolic value of \p component_reference
   *
   * If it's a special value, it returns its name ("null","mtc" or "system").
   * If it has a name, it returns "name(number)"
   * Else returns "number"
   *
   * @param component_reference
   * @return a string which must be Free()'d
   */
  static char *get_component_string(component component_reference);
  
  inline boolean is_component() { return TRUE; }
};

extern boolean operator==(component component_value,
  const COMPONENT& other_value);

inline boolean operator!=(component component_value,
  const COMPONENT& other_value)
{
  return !(component_value == other_value);
}

extern COMPONENT self;


// template for all component types

class COMPONENT_template : public Base_Template {
private:
  union {
    component single_value;
    struct {
      unsigned int n_values;
      COMPONENT_template *list_value;
    } value_list;
  };

  void copy_template(const COMPONENT_template& other_value);

public:
  COMPONENT_template();
  COMPONENT_template(template_sel other_value);
  COMPONENT_template(component other_value);
  COMPONENT_template(const COMPONENT& other_value);
  COMPONENT_template(const OPTIONAL<COMPONENT>& other_value);
  COMPONENT_template(const COMPONENT_template& other_value);

  ~COMPONENT_template();
  void clean_up();

  COMPONENT_template& operator=(template_sel other_value);
  COMPONENT_template& operator=(component other_value);
  COMPONENT_template& operator=(const COMPONENT& other_value);
  COMPONENT_template& operator=(const OPTIONAL<COMPONENT>& other_value);
  COMPONENT_template& operator=(const COMPONENT_template& other_value);

  boolean match(component other_value, boolean legacy = FALSE) const;
  boolean match(const COMPONENT& other_value, boolean legacy = FALSE) const;
  component valueof() const;

  void set_type(template_sel template_type, unsigned int list_length);
  COMPONENT_template& list_item(unsigned int list_index);

  void log() const;
  void log_match(const COMPONENT& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<COMPONENT*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const COMPONENT*>(other_value)); }
  Base_Template* clone() const { return new COMPONENT_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &COMPONENT_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const COMPONENT*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const COMPONENT*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
  
  inline boolean is_component() { return TRUE; }
};

extern const COMPONENT_template& any_compref;

#endif
