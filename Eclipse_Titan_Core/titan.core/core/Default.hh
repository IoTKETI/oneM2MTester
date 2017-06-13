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
#ifndef DEFAULT_HH
#define DEFAULT_HH

#include "Types.h"
#include "Basetype.hh"
#include "Template.hh"

class Text_Buf;
class Module_Param;

template<typename T>
class OPTIONAL;

class Default_Base {
  friend class TTCN_Default;
  friend class DEFAULT;
  friend class DEFAULT_template;

  unsigned int default_id;
  const char *altstep_name;
  Default_Base *default_prev, *default_next;
  // Even though Default_base doesn't own these pointers,
  // it's best to disallow copying.
  Default_Base(const Default_Base&);
  Default_Base& operator=(const Default_Base&);
public:
  Default_Base(const char *par_altstep_name);
  virtual ~Default_Base();

  virtual alt_status call_altstep() = 0;

  void log() const;
};

class DEFAULT : public Base_Type {
  friend class TTCN_Default;
  friend class DEFAULT_template;
  friend boolean operator==(component default_value,
    const DEFAULT& other_value);
  friend boolean operator==(Default_Base *default_value,
    const DEFAULT& other_value);

  Default_Base *default_ptr;
public:
  DEFAULT();
  DEFAULT(component other_value);
  DEFAULT(Default_Base *other_value);
  DEFAULT(const DEFAULT& other_value);

  DEFAULT& operator=(component other_value);
  DEFAULT& operator=(Default_Base *other_value);
  DEFAULT& operator=(const DEFAULT& other_value);

  boolean operator==(component other_value) const;
  boolean operator==(Default_Base *other_value) const;
  boolean operator==(const DEFAULT& other_value) const;

  boolean operator!=(component other_value) const
    { return !(*this == other_value); }
  boolean operator!=(Default_Base *other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const DEFAULT& other_value) const
    { return !(*this == other_value); }

  operator Default_Base*() const;

  boolean is_bound() const;
  boolean is_value() const;
  void clean_up();
  void log() const;

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const DEFAULT*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const DEFAULT*>(other_value)); }
  Base_Type* clone() const { return new DEFAULT(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &DEFAULT_descr_; }
  Module_Param* get_param(Module_Param_Name& param_name) const;
#else
  inline boolean is_present() const { return is_bound(); }
#endif

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);
};

extern boolean operator==(component default_value, const DEFAULT& other_value);
extern boolean operator==(Default_Base *default_value,
  const DEFAULT& other_value);

inline boolean operator!=(component default_value, const DEFAULT& other_value)
        { return !(default_value == other_value); }
inline boolean operator!=(Default_Base *default_value,
  const DEFAULT& other_value)
  { return !(default_value == other_value); }


class DEFAULT_template : public Base_Template {
  union {
    Default_Base *single_value;
    struct {
      unsigned int n_values;
      DEFAULT_template *list_value;
    } value_list;
  };

  void copy_template(const DEFAULT_template& other_value);

public:
  DEFAULT_template();
  DEFAULT_template(template_sel other_value);
  DEFAULT_template(component other_value);
  DEFAULT_template(Default_Base *other_value);
  DEFAULT_template(const DEFAULT& other_value);
  DEFAULT_template(const OPTIONAL<DEFAULT>& other_value);
  DEFAULT_template(const DEFAULT_template& other_value);

  ~DEFAULT_template();
  void clean_up();

  DEFAULT_template& operator=(template_sel other_value);
  DEFAULT_template& operator=(component other_value);
  DEFAULT_template& operator=(Default_Base *other_value);
  DEFAULT_template& operator=(const DEFAULT& other_value);
  DEFAULT_template& operator=(const OPTIONAL<DEFAULT>& other_value);
  DEFAULT_template& operator=(const DEFAULT_template& other_value);

  boolean match(component other_value, boolean legacy = FALSE) const;
  boolean match(Default_Base *other_value, boolean legacy = FALSE) const;
  boolean match(const DEFAULT& other_value, boolean legacy = FALSE) const;
  Default_Base *valueof() const;

  void set_type(template_sel template_type, unsigned int list_length);
  DEFAULT_template& list_item(unsigned int list_index);

  void log() const;
  void log_match(const DEFAULT& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<DEFAULT*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const DEFAULT*>(other_value)); }
  Base_Template* clone() const { return new DEFAULT_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &DEFAULT_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const DEFAULT*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const DEFAULT*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
};

class TTCN_Default {
  static unsigned int default_count, backup_count;
  static Default_Base *list_head, *list_tail, *backup_head, *backup_tail;
  static boolean control_defaults_saved;
public:
  static unsigned int activate(Default_Base *new_default);
  static void deactivate(Default_Base *removable_default);
  static void deactivate(const DEFAULT& removable_default);
  static void deactivate_all();

  static alt_status try_altsteps();

  static void log(Default_Base *default_ptr);

  static void save_control_defaults();
  static void restore_control_defaults();
  static void reset_counter();
};

#endif
