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
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include <string.h>

#include "../common/memory.h"

#include "Component.hh"
#include "Logger.hh"
#include "Parameters.h"
#include "Param_Types.hh"
#include "Runtime.hh"
#include "Optional.hh"

#include "../common/dbgnew.hh"

COMPONENT::COMPONENT()
{
  component_value = UNBOUND_COMPREF;
}

COMPONENT::COMPONENT(component other_value)
{
  component_value = other_value;
}

COMPONENT::COMPONENT(const COMPONENT& other_value)
: Base_Type(other_value)
  {
  if (other_value.component_value == UNBOUND_COMPREF)
    TTCN_error("Copying an unbound component reference.");
  component_value = other_value.component_value;
  }

COMPONENT& COMPONENT::operator=(component other_value)
{
  component_value = other_value;
  return *this;
}

COMPONENT& COMPONENT::operator=(const COMPONENT& other_value)
{
  if (other_value.component_value == UNBOUND_COMPREF)
    TTCN_error("Assignment of an unbound component reference.");
  component_value = other_value.component_value;
  return *this;
}

boolean COMPONENT::operator==(component other_value) const
{
  if (component_value == UNBOUND_COMPREF) TTCN_error("The left operand of "
    "comparison is an unbound component reference.");
  return component_value == other_value;
}

boolean COMPONENT::operator==(const COMPONENT& other_value) const
{
  if (component_value == UNBOUND_COMPREF) TTCN_error("The left operand of "
    "comparison is an unbound component reference.");
  if (other_value.component_value == UNBOUND_COMPREF) TTCN_error("The right "
    "operand of comparison is an unbound component reference.");
  return component_value == other_value.component_value;
}

COMPONENT::operator component() const
{
  if (component_value == UNBOUND_COMPREF) TTCN_error("Using the value of an "
    "unbound component reference.");
  return component_value;
}

void COMPONENT::log() const
{
  if (component_value != UNBOUND_COMPREF)
    log_component_reference(component_value);
  else TTCN_Logger::log_event_unbound();
}

alt_status COMPONENT::done(Index_Redirect*) const
{
  if (component_value == UNBOUND_COMPREF) TTCN_error("Performing done "
    "operation on an unbound component reference.");
  return TTCN_Runtime::component_done(component_value);
}

alt_status COMPONENT::killed(Index_Redirect*) const
{
  if (component_value == UNBOUND_COMPREF) TTCN_error("Performing killed "
    "operation on an unbound component reference.");
  return TTCN_Runtime::component_killed(component_value);
}

boolean COMPONENT::running(Index_Redirect*) const
{
  if (component_value == UNBOUND_COMPREF) TTCN_error("Performing running "
    "operation on an unbound component reference.");
  return TTCN_Runtime::component_running(component_value);
}

boolean COMPONENT::alive(Index_Redirect*) const
{
  if (component_value == UNBOUND_COMPREF) TTCN_error("Performing alive "
    "operation on an unbound component reference.");
  return TTCN_Runtime::component_alive(component_value);
}

void COMPONENT::stop() const
{
  if (component_value == UNBOUND_COMPREF) TTCN_error("Performing stop "
    "operation on an unbound component reference.");
  TTCN_Runtime::stop_component(component_value);
}

void COMPONENT::kill() const
{
  if (component_value == UNBOUND_COMPREF) TTCN_error("Performing kill "
    "operation on an unbound component reference.");
  TTCN_Runtime::kill_component(component_value);
}

void COMPONENT::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE, "component reference (integer or null) value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  if (Ttcn_String_Parsing::happening() || Debugger_Value_Parsing::happening()) {
    // accept all component values in case it's a string2ttcn operation or
    // an overwrite operation through the debugger
    switch (mp->get_type()) {
    case Module_Param::MP_Integer:
      component_value = (component)mp->get_integer()->get_val();
      break;
    case Module_Param::MP_Ttcn_Null:
      component_value = NULL_COMPREF;
      break;
    case Module_Param::MP_Ttcn_mtc:
      component_value = MTC_COMPREF;
      break;
    case Module_Param::MP_Ttcn_system:
      component_value = SYSTEM_COMPREF;
      break;
    default:
      param.type_error("component reference (integer or null) value");
    }
  }
  else {
    // only accept the null value if it's a module parameter
    if (Module_Param::MP_Ttcn_Null != mp->get_type()) {
      param.error("Only the 'null' value is allowed for module parameters of type 'component'.");
    }
    component_value = NULL_COMPREF;
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* COMPONENT::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  return new Module_Param_Ttcn_Null();
}
#endif

void COMPONENT::encode_text(Text_Buf& text_buf) const
{
  if (component_value == UNBOUND_COMPREF) TTCN_error("Text encoder: Encoding "
    "an unbound component reference.");
  text_buf.push_int((int)component_value);
  switch (component_value) {
  case NULL_COMPREF:
  case MTC_COMPREF:
  case SYSTEM_COMPREF:
    break;
  default:
    text_buf.push_string(get_component_name(component_value));
    break;
  }
}

void COMPONENT::decode_text(Text_Buf& text_buf)
{
  component_value = (component)text_buf.pull_int().get_val();
  switch (component_value) {
  case NULL_COMPREF:
  case MTC_COMPREF:
  case SYSTEM_COMPREF:
    break;
  default:
    char *component_name = text_buf.pull_string();
    register_component_name(component_value, component_name);
    delete [] component_name;
    break;
  }
}

boolean operator==(component component_value, const COMPONENT& other_value)
{
  if (other_value.component_value == UNBOUND_COMPREF) TTCN_error("The right "
    "operand of comparison is an unbound component reference.");
  return component_value == other_value.component_value;
}

unsigned int COMPONENT::n_component_names = 0;
struct COMPONENT::component_name_struct {
  component component_reference;
  char *component_name;
} *COMPONENT::component_names = NULL;

void COMPONENT::register_component_name(component component_reference,
  const char *component_name)
{
  if (self.component_value == component_reference) {
    // the own name of the component will not be registered,
    // but check whether we got the right string
    const char *local_name = TTCN_Runtime::get_component_name();
    if (component_name == NULL || component_name[0] == '\0') {
      if (local_name != NULL) {
        TTCN_error("Internal error: Trying to register the component "
          "reference of this PTC without any name, but this "
          "component has name %s.", local_name);
      }
    } else {
      if (local_name == NULL) {
        TTCN_error("Internal error: Trying to register the component "
          "reference of this PTC with name %s, but this component "
          "does not have name.", component_name);
      } else if (strcmp(component_name, local_name)) {
        TTCN_error("Internal error: Trying to register the component "
          "reference of this PTC with name %s, but this component "
          "has name %s.", component_name, local_name);
      }
    }
    return;
  }
  unsigned int min = 0;
  if (n_component_names > 0) {
    // perform a binary search to find the place for the component reference
    unsigned int max = n_component_names - 1;
    while (min < max) {
      unsigned int mid = min + (max - min) / 2;
      if (component_names[mid].component_reference < component_reference)
        min = mid + 1;
      else if (component_names[mid].component_reference ==
        component_reference) {
        min = mid;
        break;
      } else max = mid;
    }
    if (component_names[min].component_reference == component_reference) {
      // the component reference is already registered
      const char *stored_name = component_names[min].component_name;
      if (component_name == NULL || component_name[0] == '\0') {
        if (stored_name != NULL) {
          TTCN_error("Internal error: Trying to register component "
            "reference %d without any name, but this component "
            "reference is already registered with name %s.",
            component_reference, stored_name);
        }
      } else {
        if (stored_name == NULL) {
          TTCN_error("Internal error: Trying to register component "
            "reference %d with name %s, but this component "
            "reference is already registered without name.",
            component_reference, component_name);
        } else if (strcmp(component_name, stored_name)) {
          TTCN_error("Internal error: Trying to register component "
            "reference %d with name %s, but this component "
            "reference is already registered with a different "
            "name (%s).", component_reference, component_name,
            stored_name);
        }
      }
      return;
    } else {
      if (component_names[min].component_reference < component_reference)
        min++;
      // the component reference will be inserted before the index "min"
      component_names =
        (component_name_struct*)Realloc(component_names,
          (n_component_names + 1) * sizeof(*component_names));
      memmove(component_names + min + 1, component_names + min,
        (n_component_names - min) * sizeof(*component_names));
    }
  } else {
    // this is the first component reference to be registered
    component_names =
      (component_name_struct*)Malloc(sizeof(*component_names));
  }
  component_names[min].component_reference = component_reference;
  if (component_name == NULL || component_name[0] == '\0')
    component_names[min].component_name = NULL;
  else component_names[min].component_name = mcopystr(component_name);
  n_component_names++;
}

const char *COMPONENT::get_component_name(component component_reference)
{
  if (self.component_value == component_reference) {
    // the own name of the PTC is not registered
    return TTCN_Runtime::get_component_name();
  } else if (n_component_names > 0) {
    unsigned int min = 0, max = n_component_names - 1;
    while (min < max) {
      unsigned int mid = min + (max - min) / 2;
      if (component_names[mid].component_reference < component_reference)
        min = mid + 1;
      else if (component_names[mid].component_reference ==
        component_reference)
        return component_names[mid].component_name;
      else max = mid;
    }
    if (component_names[min].component_reference != component_reference)
      TTCN_error("Internal error: Trying to get the name of PTC with "
        "component reference %d, but the name of the component "
        "is not registered.", component_reference);
    return component_names[min].component_name;
  } else {
    TTCN_error("Internal error: Trying to get the name of PTC with "
      "component reference %d, but there are no component names "
      "registered.", component_reference);
    return NULL;
  }
}

void COMPONENT::clear_component_names()
{
  for (unsigned int i = 0; i < n_component_names; i++)
    Free(component_names[i].component_name);
  Free(component_names);
  n_component_names = 0;
  component_names = NULL;
}

void COMPONENT::log_component_reference(component component_reference)
{
  switch (component_reference) {
  case NULL_COMPREF:
    TTCN_Logger::log_event_str("null");
    break;
  case MTC_COMPREF:
    TTCN_Logger::log_event_str("mtc");
    break;
  case SYSTEM_COMPREF:
    TTCN_Logger::log_event_str("system");
    break;
  default:
    const char *component_name = get_component_name(component_reference);
    if (component_name != NULL) TTCN_Logger::log_event("%s(%d)",
      component_name, component_reference);
    else TTCN_Logger::log_event("%d", component_reference);
    break;
  }
}

// get_component_string is suspiciously similar to log_component_reference.
// It would be trivially easy to implement log_... with get_...
// but it would involve a runtime penalty, because get_component_string
// returns a newly allocated string which must be freed, whereas log_...
// uses fixed strings. So the current implementation pays for speed with size.
// Perhaps log_component_reference will go away one day.
char *COMPONENT::get_component_string(component component_reference)
{
  switch (component_reference) {
  case NULL_COMPREF:
    return mcopystr("null");
  case MTC_COMPREF:
    return mcopystr("mtc");
  case SYSTEM_COMPREF:
    return mcopystr("system");
  case CONTROL_COMPREF:
    return mcopystr("control");
  default:
    const char *component_name = get_component_name(component_reference);
    if (component_name != NULL) return mprintf("%s(%d)",
      component_name, component_reference);
    else return mprintf("%d", component_reference);
  }
}

COMPONENT self;

void COMPONENT_template::clean_up()
{
  if (template_selection == VALUE_LIST
    ||template_selection == COMPLEMENTED_LIST)
    delete [] value_list.list_value;
  template_selection = UNINITIALIZED_TEMPLATE;
}

void COMPONENT_template::copy_template(const COMPONENT_template& other_value)
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
    value_list.list_value =  new COMPONENT_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
        other_value.value_list.list_value[i]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported component reference "
      "template.");
  }
  set_selection(other_value);
}

COMPONENT_template::COMPONENT_template()
{

}

COMPONENT_template::COMPONENT_template(template_sel other_value)
: Base_Template(other_value)
  {
  check_single_selection(other_value);
  }

COMPONENT_template::COMPONENT_template(component other_value)
: Base_Template(SPECIFIC_VALUE)
  {
  single_value = other_value;
  }

COMPONENT_template::COMPONENT_template(const COMPONENT& other_value)
: Base_Template(SPECIFIC_VALUE)
  {
  if (other_value.component_value == UNBOUND_COMPREF)
    TTCN_error("Creating a template from an unbound component reference.");
  single_value = other_value.component_value;
  }

COMPONENT_template::COMPONENT_template(const OPTIONAL<COMPONENT>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (component)(const COMPONENT&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  default:
    TTCN_error("Creating a component reference template from an unbound "
      "optional field.");
  }
}

COMPONENT_template::COMPONENT_template(const COMPONENT_template& other_value)
: Base_Template()
  {
  copy_template(other_value);
  }

COMPONENT_template::~COMPONENT_template()
{
  clean_up();
}

COMPONENT_template& COMPONENT_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

COMPONENT_template& COMPONENT_template::operator=(component other_value)
{
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

COMPONENT_template& COMPONENT_template::operator=(const COMPONENT& other_value)
{
  if (other_value.component_value == UNBOUND_COMPREF)
    TTCN_error("Assignment of an unbound component reference to a "
      "template.");
  return *this = other_value.component_value;
}

COMPONENT_template& COMPONENT_template::operator=
  (const OPTIONAL<COMPONENT>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (component)(const COMPONENT&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  default:
    TTCN_error("Assignment of an unbound optional field to a component "
      "reference template.");
  }
  return *this;
}

COMPONENT_template& COMPONENT_template::operator=
  (const COMPONENT_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean COMPONENT_template::match(component other_value,
                                  boolean /* legacy */) const
{
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
    TTCN_error("Matching an uninitialized/unsupported component reference "
      "template.");
  }
  return FALSE;
}

boolean COMPONENT_template::match(const COMPONENT& other_value,
                                  boolean /* legacy */) const
{
  if (other_value.component_value == UNBOUND_COMPREF)
    TTCN_error("Matching an unbound component reference with a template.");
  return match(other_value.component_value);
}

component COMPONENT_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific "
      "component reference template.");
  return single_value;
}

void COMPONENT_template::set_type(template_sel template_type,
    unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list type for a component reference "
      "template.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new COMPONENT_template[list_length];
}

COMPONENT_template& COMPONENT_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
    template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list component "
      "reference template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a component reference value list "
      "template.");
  return value_list.list_value[list_index];
}

void COMPONENT_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    switch (single_value) {
    case NULL_COMPREF:
      TTCN_Logger::log_event_str("null");
      break;
    case MTC_COMPREF:
      TTCN_Logger::log_event_str("mtc");
      break;
    case SYSTEM_COMPREF:
      TTCN_Logger::log_event_str("system");
      break;
    default:
      TTCN_Logger::log_event("%d", single_value);
      break;
    }
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

void COMPONENT_template::log_match(const COMPONENT& match_value,
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
  if (match(match_value)) TTCN_Logger::log_event_str(" matched");
  else TTCN_Logger::log_event_str(" unmatched");
}

void COMPONENT_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE, "component reference (integer or null) template");
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
    COMPONENT_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Integer:
    *this = (component)mp->get_integer()->get_val();
    break;
  case Module_Param::MP_Ttcn_Null:
    *this = NULL_COMPREF;
    break;
  case Module_Param::MP_Ttcn_mtc:
    *this = MTC_COMPREF;
    break;
  case Module_Param::MP_Ttcn_system:
    *this = SYSTEM_COMPREF;
    break;
  default:
    param.type_error("component reference (integer or null) template");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* COMPONENT_template::get_param(Module_Param_Name& param_name) const
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
    switch (single_value) {
    case NULL_COMPREF:
      mp = new Module_Param_Ttcn_Null();
      break;
    case MTC_COMPREF:
      mp = new Module_Param_Ttcn_mtc();
      break;
    case SYSTEM_COMPREF:
      mp = new Module_Param_Ttcn_system();
      break;
    default:
      mp = new Module_Param_Integer(new int_val_t(single_value));
      break;
    }    
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
    TTCN_error("Referencing an uninitialized/unsupported component reference template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void COMPONENT_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE:
    text_buf.push_int(single_value);
    break;
  case COMPLEMENTED_LIST:
  case VALUE_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported "
      "component reference template.");
  }
}

void COMPONENT_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE:
    single_value = (component)text_buf.pull_int().get_val();
    break;
  case COMPLEMENTED_LIST:
  case VALUE_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new COMPONENT_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was "
      "received for a component reference template.");
  }
}

boolean COMPONENT_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean COMPONENT_template::match_omit(boolean legacy /* = FALSE */) const
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
void COMPONENT_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "component reference");
}
#endif

const COMPONENT_template any_compref_value(ANY_VALUE);
const COMPONENT_template& any_compref = any_compref_value;
