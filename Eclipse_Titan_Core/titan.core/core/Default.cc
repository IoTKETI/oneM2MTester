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
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include "Default.hh"

#include "Parameters.h"
#include "Param_Types.hh"
#include "Logger.hh"
#include "Error.hh"
#include "TitanLoggerApi.hh"
#include "Optional.hh"

#include "../common/dbgnew.hh"

Default_Base::Default_Base(const char *par_altstep_name)
{
  default_id = TTCN_Default::activate(this);
  altstep_name = par_altstep_name;
  TTCN_Logger::log_defaultop_activate(par_altstep_name, default_id);
}

Default_Base::~Default_Base()
{
  TTCN_Logger::log_defaultop_deactivate(altstep_name, default_id);
}

void Default_Base::log() const
{
  TTCN_Logger::log_event("default reference: altstep: %s, id: %u",
    altstep_name, default_id);
}

// a little hack to create a pointer constant other than NULL
// by taking the address of a memory object used for nothing
static Default_Base *dummy_ptr = NULL;
static Default_Base * const UNBOUND_DEFAULT = (Default_Base*)&dummy_ptr;

DEFAULT::DEFAULT()
{
  default_ptr = UNBOUND_DEFAULT;
}

DEFAULT::DEFAULT(component other_value)
{
  if (other_value != NULL_COMPREF)
    TTCN_error("Initialization from an invalid default reference.");
  default_ptr = NULL;
}

DEFAULT::DEFAULT(Default_Base *other_value)
{
  default_ptr = other_value;
}

DEFAULT::DEFAULT(const DEFAULT& other_value)
: Base_Type(other_value)
  {
  if (other_value.default_ptr == UNBOUND_DEFAULT)
    TTCN_error("Copying an unbound default reference.");
  default_ptr = other_value.default_ptr;
  }

DEFAULT& DEFAULT::operator=(component other_value)
{
  if (other_value != NULL_COMPREF)
    TTCN_error("Assignment of an invalid default reference.");
  default_ptr = NULL;
  return *this;
}

DEFAULT& DEFAULT::operator=(Default_Base *other_value)
{
  default_ptr = other_value;
  return *this;
}

DEFAULT& DEFAULT::operator=(const DEFAULT& other_value)
{
  if (other_value.default_ptr == UNBOUND_DEFAULT)
    TTCN_error("Assignment of an unbound default reference.");
  default_ptr = other_value.default_ptr;
  return *this;
}

boolean DEFAULT::operator==(component other_value) const
{
  if (default_ptr == UNBOUND_DEFAULT) TTCN_error("The left operand of "
    "comparison is an unbound default reference.");
  if (other_value != NULL_COMPREF)
    TTCN_error("Comparison of an invalid default value.");
  return default_ptr == NULL;
}

boolean DEFAULT::operator==(Default_Base *other_value) const
{
  if (default_ptr == UNBOUND_DEFAULT) TTCN_error("The left operand of "
    "comparison is an unbound default reference.");
  return default_ptr == other_value;
}

boolean DEFAULT::operator==(const DEFAULT& other_value) const
{
  if (default_ptr == UNBOUND_DEFAULT) TTCN_error("The left operand of "
    "comparison is an unbound default reference.");
  if (other_value.default_ptr == UNBOUND_DEFAULT) TTCN_error("The right "
    "operand of comparison is an unbound default reference.");
  return default_ptr == other_value.default_ptr;
}

DEFAULT::operator Default_Base*() const
{
  if (default_ptr == UNBOUND_DEFAULT)
    TTCN_error("Using the value of an unbound default reference.");
  return default_ptr;
}

boolean DEFAULT::is_bound() const
{
  return default_ptr != UNBOUND_DEFAULT; // what about NULL ?
}

boolean DEFAULT::is_value() const
{
  return default_ptr != UNBOUND_DEFAULT; // what about NULL ?
}

void DEFAULT::clean_up()
{
  default_ptr = UNBOUND_DEFAULT;
}

void DEFAULT::log() const
{
  TTCN_Default::log(default_ptr);
}

void DEFAULT::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE, "default reference (null) value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  if (mp->get_type()!=Module_Param::MP_Ttcn_Null) param.type_error("default reference (null) value");
  default_ptr = NULL;
}

#ifdef TITAN_RUNTIME_2
Module_Param* DEFAULT::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  return new Module_Param_Ttcn_Null();
}
#endif

void DEFAULT::encode_text(Text_Buf&) const
{
  TTCN_error("Default references cannot be sent to other test components.");
}

void DEFAULT::decode_text(Text_Buf&)
{
  TTCN_error("Default references cannot be received from other test "
    "components.");
}

boolean operator==(component default_value, const DEFAULT& other_value)
  {
  if (default_value != NULL_COMPREF) TTCN_error("The left operand of "
    "comparison is an invalid default reference.");
  if (other_value.default_ptr == UNBOUND_DEFAULT) TTCN_error("The right "
    "operand of comparison is an unbound default reference.");
  return other_value.default_ptr == NULL;
  }

boolean operator==(Default_Base *default_value, const DEFAULT& other_value)
  {
  if (other_value.default_ptr == UNBOUND_DEFAULT) TTCN_error("The right "
    "operand of comparison is an unbound default reference.");
  return default_value == other_value.default_ptr;
  }

void DEFAULT_template::clean_up()
{
  if (template_selection == VALUE_LIST ||
    template_selection == COMPLEMENTED_LIST)
    delete [] value_list.list_value;
  template_selection = UNINITIALIZED_TEMPLATE;
}

void DEFAULT_template::copy_template(const DEFAULT_template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    single_value = other_value.single_value;
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value =	new DEFAULT_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
        other_value.value_list.list_value[i]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported default reference "
      "template.");
  }
  set_selection(other_value);
}

DEFAULT_template::DEFAULT_template()
{

}

DEFAULT_template::DEFAULT_template(template_sel other_value)
: Base_Template(other_value)
  {
  check_single_selection(other_value);
  }

DEFAULT_template::DEFAULT_template(component other_value)
: Base_Template(SPECIFIC_VALUE)
  {
  if (other_value != NULL_COMPREF)
    TTCN_error("Creating a template from an invalid default reference.");
  single_value = NULL;
  }

DEFAULT_template::DEFAULT_template(Default_Base *other_value)
: Base_Template(SPECIFIC_VALUE)
  {
  single_value = other_value;
  }

DEFAULT_template::DEFAULT_template(const DEFAULT& other_value)
: Base_Template(SPECIFIC_VALUE)
  {
  if (other_value.default_ptr == UNBOUND_DEFAULT)
    TTCN_error("Creating a template from an unbound default reference.");
  single_value = other_value.default_ptr;
  }

DEFAULT_template::DEFAULT_template(const OPTIONAL<DEFAULT>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (Default_Base*)(const DEFAULT&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  default:
    TTCN_error("Creating a default reference template from an unbound "
      "optional field.");
  }
}

DEFAULT_template::DEFAULT_template(const DEFAULT_template& other_value)
: Base_Template()
  {
  copy_template(other_value);
  }

DEFAULT_template::~DEFAULT_template()
{
  clean_up();
}

DEFAULT_template& DEFAULT_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

DEFAULT_template& DEFAULT_template::operator=(component other_value)
{
  if (other_value != NULL_COMPREF)
    TTCN_error("Assignment of an invalid default reference to a template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = NULL;
  return *this;
}

DEFAULT_template& DEFAULT_template::operator=(Default_Base *other_value)
{
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

DEFAULT_template& DEFAULT_template::operator=(const DEFAULT& other_value)
{
  if (other_value.default_ptr == UNBOUND_DEFAULT)
    TTCN_error("Assignment of an unbound default reference to a template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value.default_ptr;
  return *this;
}

DEFAULT_template& DEFAULT_template::operator=
  (const OPTIONAL<DEFAULT>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (Default_Base*)(const DEFAULT&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  default:
    TTCN_error("Assignment of an unbound optional field to a default "
      "reference template.");
  }
  return *this;
}

DEFAULT_template& DEFAULT_template::operator=
  (const DEFAULT_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean DEFAULT_template::match(component other_value,
                                boolean /* legacy */) const
{
  if (other_value == NULL_COMPREF) return FALSE;
  return match((Default_Base*)NULL);
}

boolean DEFAULT_template::match(Default_Base *other_value,
                                boolean /* legacy */) const
{
  if (other_value == UNBOUND_DEFAULT) return FALSE;
  switch (template_selection) {
  case SPECIFIC_VALUE:
    return single_value == other_value;
  case OMIT_VALUE:
    return FALSE;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int i = 0; i < value_list.n_values; i++)
      if (value_list.list_value[i].match(other_value))
        return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching with an uninitialized/unsupported default "
      "reference template.");
  }
  return FALSE;
}

boolean DEFAULT_template::match(const DEFAULT& other_value,
                                boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  return match(other_value.default_ptr);
}

Default_Base *DEFAULT_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific "
      "default reference template.");
  return single_value;
}

void DEFAULT_template::set_type(template_sel template_type,
    unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list type for a default reference "
      "template.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new DEFAULT_template[list_length];
}

DEFAULT_template& DEFAULT_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
    template_selection != COMPLEMENTED_LIST) TTCN_error("Accessing a list "
      "element of a non-list default reference template.");
  if (list_index >= value_list.n_values) TTCN_error("Index overflow in a "
    "default reference value list template.");
  return value_list.list_value[list_index];
}

void DEFAULT_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    TTCN_Default::log(single_value);
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int i = 0; i < value_list.n_values; i++) {
      if (i > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[i].log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void DEFAULT_template::log_match(const DEFAULT& match_value,
                                 boolean /* legacy */) const
{
  if (TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()
  &&  TTCN_Logger::get_logmatch_buffer_len() != 0) {
    TTCN_Logger::print_logmatch_buffer();
    TTCN_Logger::log_event_str(" := ");
  }
  match_value.log();
  TTCN_Logger::log_event_str(" with ");
  log();
  if(match(match_value))TTCN_Logger::log_event_str(" matched");
  else TTCN_Logger::log_event_str(" unmatched");
}

void DEFAULT_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE, "default reference (null) template");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    *this = OMIT_VALUE;
    break;
  case Module_Param::MP_Any:
    *this = ANY_VALUE;
    break;
  case Module_Param::MP_AnyOrNone:
    *this = ANY_OR_OMIT;
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    DEFAULT_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Ttcn_Null:
    *this = DEFAULT(NULL_COMPREF);
    break;
  default:
    param.type_error("default reference (null) template");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* DEFAULT_template::get_param(Module_Param_Name& param_name) const
{
  Module_Param* mp = NULL;
  switch (template_selection) {
  case UNINITIALIZED_TEMPLATE:
    mp = new Module_Param_Unbound();
    break;
  case OMIT_VALUE:
    mp = new Module_Param_Omit();
    break;
  case ANY_VALUE:
    mp = new Module_Param_Any();
    break;
  case ANY_OR_OMIT:
    mp = new Module_Param_AnyOrNone();
    break;
  case SPECIFIC_VALUE:
    mp = new Module_Param_Ttcn_Null();
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (size_t i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i].get_param(param_name));
    }
    break; }
  default:
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void DEFAULT_template::encode_text(Text_Buf&) const
{
  TTCN_error("Default reference templates cannot be sent to other test "
    "components.");
}

void DEFAULT_template::decode_text(Text_Buf&)
{
  TTCN_error("Default reference templates cannot be received from other test "
    "components.");
}

boolean DEFAULT_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean DEFAULT_template::match_omit(boolean legacy /* = FALSE */) const
{
  if (is_ifpresent) return TRUE;
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    if (legacy) {
      // legacy behavior: 'omit' can appear in the value/complement list
      for (unsigned int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i].match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    }
    // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
void DEFAULT_template::check_restriction(template_res t_res, const char* t_name,
                                         boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return;
  switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {
  case TR_VALUE:
    if (!is_ifpresent && template_selection==SPECIFIC_VALUE) return;
    break;
  case TR_OMIT:
    if (!is_ifpresent && (template_selection==OMIT_VALUE ||
      template_selection==SPECIFIC_VALUE)) return;
    break;
  case TR_PRESENT:
    if (!match_omit(legacy)) return;
    break;
  default:
    return;
  }
  TTCN_error("Restriction `%s' on template of type %s violated.",
             get_res_name(t_res), t_name ? t_name : "default reference");

}
#endif

unsigned int TTCN_Default::default_count = 0, TTCN_Default::backup_count = 0;
Default_Base *TTCN_Default::list_head = NULL, *TTCN_Default::list_tail = NULL,
    *TTCN_Default::backup_head = NULL, *TTCN_Default::backup_tail = NULL;
boolean TTCN_Default::control_defaults_saved = FALSE;

unsigned int TTCN_Default::activate(Default_Base *new_default)
{
  new_default->default_prev = list_tail;
  new_default->default_next = NULL;
  if (list_tail != NULL) list_tail->default_next = new_default;
  else list_head = new_default;
  list_tail = new_default;
  return ++default_count;
}

void TTCN_Default::deactivate(Default_Base *removable_default)
{
  for (Default_Base *default_iter = list_head; default_iter != NULL;
    default_iter = default_iter->default_next) {
    if (default_iter == removable_default) {
      if (removable_default->default_prev != NULL)
        removable_default->default_prev->default_next =
          removable_default->default_next;
      else list_head = removable_default->default_next;
      if (removable_default->default_next != NULL)
        removable_default->default_next->default_prev =
          removable_default->default_prev;
      else list_tail = removable_default->default_prev;
      delete removable_default;
      return;
    }
  }
  TTCN_warning("Performing a deactivate operation on an inactive "
    "default reference.");
}

void TTCN_Default::deactivate(const DEFAULT &removable_default)
{
  if (removable_default.default_ptr == UNBOUND_DEFAULT)
    TTCN_error("Performing a deactivate operation on an unbound default "
      "reference.");
  if (removable_default.default_ptr == NULL)
    TTCN_Logger::log_defaultop_deactivate(NULL, 0);
  else deactivate(removable_default.default_ptr);
}

void TTCN_Default::deactivate_all()
{
  while (list_head != NULL) deactivate(list_head);
}

alt_status TTCN_Default::try_altsteps()
{
  alt_status ret_val = ALT_NO;
  for (Default_Base *default_iter = list_tail; default_iter != NULL; ) {
    Default_Base *prev_iter = default_iter->default_prev;
    unsigned int default_id = default_iter->default_id;
    const char *altstep_name = default_iter->altstep_name;
    switch (default_iter->call_altstep()) {
    case ALT_YES:
      TTCN_Logger::log_defaultop_exit(altstep_name, default_id,
        TitanLoggerApi::DefaultEnd::finish);
      return ALT_YES;
    case ALT_REPEAT:
      TTCN_Logger::log_defaultop_exit(altstep_name, default_id,
        TitanLoggerApi::DefaultEnd::repeat__);
      return ALT_REPEAT;
    case ALT_BREAK:
      TTCN_Logger::log_defaultop_exit(altstep_name, default_id,
        TitanLoggerApi::DefaultEnd::break__);
      return ALT_BREAK;
    case ALT_MAYBE:
      ret_val = ALT_MAYBE;
      break;
    default:
      break;
    }
    default_iter = prev_iter;
  }
  return ret_val;
}

void TTCN_Default::log(Default_Base *default_ptr)
{
  if (default_ptr == UNBOUND_DEFAULT) TTCN_Logger::log_event_unbound();
  else if (default_ptr == NULL) TTCN_Logger::log_event_str("null");
  else {
    for (Default_Base *default_iter = list_head; default_iter != NULL;
      default_iter = default_iter->default_next) {
      if (default_iter == default_ptr) {
        default_ptr->log();
        return;
      }
    }
    TTCN_Logger::log_event_str("default reference: already deactivated");
  }
}

void TTCN_Default::save_control_defaults()
{
  if (control_defaults_saved)
    TTCN_error("Internal error: Control part defaults are already saved.");
  // put the list of control part defaults into the backup
  backup_head = list_head;
  list_head = NULL;
  backup_tail = list_tail;
  list_tail = NULL;
  backup_count = default_count;
  default_count = 0;
  control_defaults_saved = TRUE;
}

void TTCN_Default::restore_control_defaults()
{
  if (!control_defaults_saved)
    TTCN_error("Internal error: Control part defaults are not saved.");
  if (list_head != NULL)
    TTCN_error("Internal error: There are defaults timers. "
      "Control part defaults cannot be restored.");
  // restore the list of control part defaults from the backup
  list_head = backup_head;
  backup_head = NULL;
  list_tail = backup_tail;
  backup_tail = NULL;
  default_count = backup_count;
  backup_count = 0;
  control_defaults_saved = FALSE;
}

void TTCN_Default::reset_counter()
{
  if (control_defaults_saved) TTCN_error("Internal error: Default counter "
    "cannot be reset when the control part defaults are saved.");
  if (list_head != NULL) TTCN_error("Internal error: Default counter "
    "cannot be reset when there are active defaults.");
  default_count = 0;
}
