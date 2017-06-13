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
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include <string.h>

#include "../common/memory.h"
#include "Template.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Textbuf.hh"
#include "Param_Types.hh"

#ifdef TITAN_RUNTIME_2
#include "Integer.hh"

void Base_Template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : get_descriptor()->name);
}
#endif

Base_Template::Base_Template()
: template_selection(UNINITIALIZED_TEMPLATE), is_ifpresent(FALSE)
{}

Base_Template::Base_Template(template_sel other_value)
: template_selection(other_value), is_ifpresent(FALSE)
{}

void Base_Template::check_single_selection(template_sel other_value)
{
  switch (other_value) {
  case ANY_VALUE:
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    break;
  default:
    TTCN_error("Initialization of a template with an invalid selection.");
  }
}

void Base_Template::set_selection(template_sel other_value)
{
  template_selection = other_value;
  is_ifpresent = FALSE;
}

void Base_Template::set_selection(const Base_Template& other_value)
{
  template_selection = other_value.template_selection;
  is_ifpresent = other_value.is_ifpresent;
}

void Base_Template::log_generic() const
{
  switch (template_selection) {
  case UNINITIALIZED_TEMPLATE:
    TTCN_Logger::log_event_uninitialized();
    break;
  case OMIT_VALUE:
    TTCN_Logger::log_event_str("omit");
    break;
  case ANY_VALUE:
    TTCN_Logger::log_char('?');
    break;
  case ANY_OR_OMIT:
    TTCN_Logger::log_char('*');
    break;
  default:
    TTCN_Logger::log_event_str("<unknown template selection>");
    break;
  }
}

void Base_Template::log_ifpresent() const
{
  if (is_ifpresent) TTCN_Logger::log_event_str(" ifpresent");
}

void Base_Template::encode_text_base(Text_Buf& text_buf) const
{
  text_buf.push_int(template_selection);
  text_buf.push_int(is_ifpresent);
}

void Base_Template::decode_text_base(Text_Buf& text_buf)
{
  template_selection = (template_sel)text_buf.pull_int().get_val();
  is_ifpresent = (boolean)text_buf.pull_int().get_val();
}

void Base_Template::set_ifpresent()
{
  is_ifpresent = TRUE;
}

boolean Base_Template::is_omit() const
{
  return template_selection == OMIT_VALUE && !is_ifpresent;
}

boolean Base_Template::is_any_or_omit() const
{
  return template_selection == ANY_OR_OMIT && !is_ifpresent;
}

const char* Base_Template::get_res_name(template_res tr)
{
  switch (tr) {
  case TR_VALUE: return "value";
  case TR_OMIT: return "omit";
  case TR_PRESENT: return "present";
  default: break;
  }
  return "<unknown/invalid>";
}

boolean Base_Template::get_istemplate_kind(const char* type) const {
  if(!strcmp(type, "value")) {
    return is_value();
  } else if (!strcmp(type, "list")) {
    return template_selection == VALUE_LIST;
  } else if (!strcmp(type, "complement")) {
    return template_selection == COMPLEMENTED_LIST;
  } else if (!strcmp(type, "?") || !strcmp(type, "AnyValue")) {
    return template_selection == ANY_VALUE;
  } else if (!strcmp(type, "*") || !strcmp(type, "AnyValueOrNone")) {
    return template_selection == ANY_OR_OMIT;
  } else if (!strcmp(type, "range")) {
    return template_selection == VALUE_RANGE;
  } else if (!strcmp(type, "superset")) {
    return template_selection == SUPERSET_MATCH;
  } else if (!strcmp(type, "subset")) {
    return template_selection == SUBSET_MATCH;
  }else if (!strcmp(type, "omit")) {
    return template_selection == OMIT_VALUE;
  }else if (!strcmp(type, "decmatch")) {
    return template_selection == DECODE_MATCH;
  } else if (!strcmp(type, "ifpresent")) {
    return is_ifpresent;
  } else if (!strcmp(type, "pattern")) {
    return template_selection == STRING_PATTERN;
  } else if (!strcmp(type, "AnyElement") || !strcmp(type, "AnyElementsOrNone") ||
    !strcmp(type, "permutation") || !strcmp(type, "length")) {
    return FALSE;
  }
  TTCN_error("Incorrect second parameter (%s) was passed to istemplatekind.", type);
  return FALSE;
}

void Base_Template::set_param(Module_Param& /*param*/)
{
  TTCN_error("Internal error: Base_Template::set_param()");
}

#ifdef TITAN_RUNTIME_2
Module_Param* Base_Template::get_param(Module_Param_Name& /* param_name */) const
{
  TTCN_error("Internal error: Base_Template::get_param()");
  return NULL;
}
#endif

Restricted_Length_Template::Restricted_Length_Template()
{
  length_restriction_type = NO_LENGTH_RESTRICTION;
}

Restricted_Length_Template::Restricted_Length_Template(template_sel other_value)
: Base_Template(other_value)
{
  length_restriction_type = NO_LENGTH_RESTRICTION;
}

void Restricted_Length_Template::set_selection(template_sel other_value)
{
  template_selection = other_value;
  is_ifpresent = FALSE;
  length_restriction_type = NO_LENGTH_RESTRICTION;
}

void Restricted_Length_Template::set_selection
(const Restricted_Length_Template& other_value)
{
  template_selection = other_value.template_selection;
  is_ifpresent = other_value.is_ifpresent;
  length_restriction_type = other_value.length_restriction_type;
  length_restriction = other_value.length_restriction;
}

boolean Restricted_Length_Template::match_length(int value_length) const
{
  switch (length_restriction_type) {
  case NO_LENGTH_RESTRICTION:
    return TRUE;
  case SINGLE_LENGTH_RESTRICTION:
    return value_length == length_restriction.single_length;
  case RANGE_LENGTH_RESTRICTION:
    return value_length >= length_restriction.range_length.min_length &&
    (!length_restriction.range_length.max_length_set ||
      value_length <= length_restriction.range_length.max_length);
  default:
    TTCN_error("Internal error: Matching with a template that has invalid "
      "length restriction type.");
  }
  return FALSE;
}

int Restricted_Length_Template::check_section_is_single(int min_size,
  boolean has_any_or_none, const char* operation_name,
  const char* type_name_prefix, const char* type_name) const
{
  if (has_any_or_none) // upper limit is infinity
  {
    switch (length_restriction_type)
    {
    case NO_LENGTH_RESTRICTION:
      TTCN_error("Performing %sof() operation on %s %s with no exact %s.",
        operation_name, type_name_prefix, type_name, operation_name);
    case SINGLE_LENGTH_RESTRICTION:
      if (length_restriction.single_length>=min_size)
        return length_restriction.single_length;
      TTCN_error("Performing %sof() operation on an invalid %s. "
        "The minimum %s (%d) contradicts the "
        "length restriction (%d).",
        operation_name, type_name, operation_name,
        min_size, length_restriction.single_length);
    case RANGE_LENGTH_RESTRICTION: {
      boolean has_invalid_restriction;
      if (match_length(min_size))
      {
        if (length_restriction.range_length.max_length_set &&
          (min_size==length_restriction.range_length.max_length))
          return min_size;
        has_invalid_restriction = FALSE;
      }
      else
      {
        has_invalid_restriction =
          min_size>length_restriction.range_length.min_length;
      }
      if (has_invalid_restriction)
      {
        if (length_restriction.range_length.max_length_set)
          TTCN_error("Performing %sof() operation on an invalid %s. "
            "The minimum %s (%d) contradicts the "
            "length restriction (%d..%d).",
            operation_name, type_name, operation_name, min_size,
            length_restriction.range_length.min_length,
            length_restriction.range_length.max_length);
        else
          TTCN_error("Performing %sof() operation on an invalid %s. "
            "The minimum %s (%d) contradicts the "
            "length restriction (%d..infinity).",
            operation_name, type_name, operation_name, min_size,
            length_restriction.range_length.min_length);
      }
      else
        TTCN_error("Performing %sof() operation on %s %s with no exact %s.",
          operation_name, type_name_prefix, type_name, operation_name);
      break; }

    default:
      TTCN_error("Internal error: Template has invalid "
        "length restriction type.");
    }
  }
  else // exact size is in min_size, check for invalid restriction
  {
    switch (length_restriction_type)
    {
    case NO_LENGTH_RESTRICTION:
      return min_size;
    case SINGLE_LENGTH_RESTRICTION:
      if (length_restriction.single_length==min_size) return min_size;
      TTCN_error("Performing %sof() operation on an invalid %s. "
        "The %s (%d) contradicts the length restriction (%d).",
        operation_name, type_name, operation_name, min_size,
        length_restriction.single_length);
    case RANGE_LENGTH_RESTRICTION:
      if (!match_length(min_size))
      {
        if (length_restriction.range_length.max_length_set)
          TTCN_error("Performing %sof() operation on an invalid %s. "
            "The %s (%d) contradicts the length restriction (%d..%d).",
            operation_name, type_name, operation_name, min_size,
            length_restriction.range_length.min_length,
            length_restriction.range_length.max_length);
        else
          TTCN_error("Performing %sof() operation on an invalid %s. "
            "The %s (%d) contradicts the "
            "length restriction (%d..infinity).",
            operation_name, type_name, operation_name, min_size,
            length_restriction.range_length.min_length);
      }
      else
        return min_size;
    default:
      TTCN_error("Internal error: Template has invalid "
        "length restriction type.");
    }
  }
  return 0;
}

void Restricted_Length_Template::log_restricted() const
{
  switch (length_restriction_type) {
  case SINGLE_LENGTH_RESTRICTION:
    TTCN_Logger::log_event(" length (%d)",
      length_restriction.single_length);
    break;
  case NO_LENGTH_RESTRICTION:
    break;
  case RANGE_LENGTH_RESTRICTION:
    TTCN_Logger::log_event(" length (%d .. ",
      length_restriction.range_length.min_length);
    if (length_restriction.range_length.max_length_set)
      TTCN_Logger::log_event("%d)",
        length_restriction.range_length.max_length);
    else TTCN_Logger::log_event_str("infinity)");
    break;
  default:
    TTCN_Logger::log_event_str("<unknown length restriction>");
    break;
  }
}

void Restricted_Length_Template::log_match_length(int value_length) const
{
  if (length_restriction_type != NO_LENGTH_RESTRICTION) {
    if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
      if (!match_length(value_length)){
        TTCN_Logger::print_logmatch_buffer();
        log_restricted();
        TTCN_Logger::log_event(" with %d ", value_length);
      }
    }else{
      log_restricted();
      TTCN_Logger::log_event(" with %d ", value_length);
      if (match_length(value_length)) TTCN_Logger::log_event_str("matched");
      else TTCN_Logger::log_event_str("unmatched");
    }
  }
}

void Restricted_Length_Template::encode_text_restricted(Text_Buf& text_buf)const
{
  encode_text_base(text_buf);
  text_buf.push_int(length_restriction_type);
  switch (length_restriction_type) {
  case SINGLE_LENGTH_RESTRICTION:
    text_buf.push_int(length_restriction.single_length);
    break;
  case NO_LENGTH_RESTRICTION:
    break;
  case RANGE_LENGTH_RESTRICTION:
    text_buf.push_int(length_restriction.range_length.min_length);
    text_buf.push_int(length_restriction.range_length.max_length_set);
    if (length_restriction.range_length.max_length_set)
      text_buf.push_int(length_restriction.range_length.max_length);
    break;
  default:
    TTCN_error("Text encoder: encoding an unknown/unsupported length "
      "restriction type in a template.");
  }
}

void Restricted_Length_Template::decode_text_restricted(Text_Buf& text_buf)
{
  decode_text_base(text_buf);
  length_restriction_type = (length_restriction_type_t)
        text_buf.pull_int().get_val();
  switch (length_restriction_type) {
  case SINGLE_LENGTH_RESTRICTION:
    length_restriction.single_length = text_buf.pull_int().get_val();
    break;
  case NO_LENGTH_RESTRICTION:
    break;
  case RANGE_LENGTH_RESTRICTION:
    length_restriction.range_length.min_length =
      text_buf.pull_int().get_val();
    length_restriction.range_length.max_length_set =
      (boolean)text_buf.pull_int().get_val();
    if (length_restriction.range_length.max_length_set)
      length_restriction.range_length.max_length =
        text_buf.pull_int().get_val();
    break;
  default:
    TTCN_error("Text decoder: an unknown/unsupported length restriction "
      "type was received for a template.");
  }
}

void Restricted_Length_Template::set_length_range(const Module_Param& param)
{
  Module_Param_Length_Restriction* length_range = param.get_length_restriction();
  if (length_range==NULL) {
    length_restriction_type = NO_LENGTH_RESTRICTION;
    return;
  }
  if (length_range->is_single()) {
    length_restriction_type = SINGLE_LENGTH_RESTRICTION;
    length_restriction.single_length = (int)(length_range->get_min());
  } else {
    length_restriction_type = RANGE_LENGTH_RESTRICTION;
    length_restriction.range_length.min_length = (int)(length_range->get_min());
    length_restriction.range_length.max_length_set = length_range->get_has_max();
    if (length_restriction.range_length.max_length_set) {
      length_restriction.range_length.max_length = (int)(length_range->get_max());
    }
  }
}

Module_Param_Length_Restriction* Restricted_Length_Template::get_length_range() const
{
  if (length_restriction_type == NO_LENGTH_RESTRICTION) {
    return NULL;
  }
  Module_Param_Length_Restriction* mp_res = new Module_Param_Length_Restriction();
  if (length_restriction_type == SINGLE_LENGTH_RESTRICTION) {
    mp_res->set_single(length_restriction.single_length);
  }
  else {
    mp_res->set_min(length_restriction.range_length.min_length);
    if (length_restriction.range_length.max_length_set) {
      mp_res->set_max(length_restriction.range_length.max_length);
    }
  }
  return mp_res;
}

void Restricted_Length_Template::set_single_length(int single_length)
{
  length_restriction_type = SINGLE_LENGTH_RESTRICTION;
  length_restriction.single_length = single_length;
}

void Restricted_Length_Template::set_min_length(int min_length)
{
  if (min_length < 0) TTCN_error("The lower limit for the length is negative"
    " (%d) in a template with length restriction.", min_length);
  length_restriction_type = RANGE_LENGTH_RESTRICTION;
  length_restriction.range_length.min_length = min_length;
  length_restriction.range_length.max_length_set = FALSE;
}

void Restricted_Length_Template::set_max_length(int max_length)
{
  if (length_restriction_type != RANGE_LENGTH_RESTRICTION)
    TTCN_error("Internal error: Setting a maximum length for a template "
      "the length restriction of which is not a range.");
  if (max_length < 0) TTCN_error("The upper limit for the length is negative"
    " (%d) in a template with length restriction.", max_length);
  if (length_restriction.range_length.min_length > max_length)
    TTCN_error("The upper limit for the length (%d) is smaller than the "
      "lower limit (%d) in a template with length restriction.",
      max_length, length_restriction.range_length.min_length);
  length_restriction.range_length.max_length = max_length;
  length_restriction.range_length.max_length_set = TRUE;
}

boolean Restricted_Length_Template::is_omit() const
{
  return template_selection == OMIT_VALUE && !is_ifpresent &&
    length_restriction_type == NO_LENGTH_RESTRICTION;
}

boolean Restricted_Length_Template::is_any_or_omit() const
{
  return template_selection == ANY_OR_OMIT && !is_ifpresent &&
    length_restriction_type == NO_LENGTH_RESTRICTION;
}

#ifdef TITAN_RUNTIME_2
template_sel operator+(template_sel left_template_sel, template_sel right_template_sel)
{
  if (left_template_sel == ANY_VALUE && right_template_sel == ANY_VALUE) {
    return ANY_VALUE;
  }
  TTCN_error("Operand of template concatenation is an uninitialized or "
    "unsupported template.");
}
#endif

////////////////////////////////////////////////////////////////////////////////

struct Record_Of_Template::Pair_of_elements{
  unsigned int start_index, end_index; //beginning and ending index
};

Record_Of_Template::Record_Of_Template()
{
#ifdef TITAN_RUNTIME_2
  err_descr = NULL;
#endif
  number_of_permutations = 0;
  permutation_intervals = NULL;
}

Record_Of_Template::Record_Of_Template(template_sel other_value)
: Restricted_Length_Template(other_value)
{
#ifdef TITAN_RUNTIME_2
  err_descr = NULL;
#endif
  number_of_permutations = 0;
  permutation_intervals = NULL;
}

void Record_Of_Template::clean_up_intervals()
{
  number_of_permutations = 0;
  Free(permutation_intervals);
  permutation_intervals = NULL;
}

void Record_Of_Template::set_selection(template_sel other_value)
{
  Restricted_Length_Template::set_selection(other_value);
  clean_up_intervals();
}

void Record_Of_Template::set_selection(const Record_Of_Template& other_value)
{
  Restricted_Length_Template::set_selection(other_value);
  clean_up_intervals();
  if(other_value.template_selection == SPECIFIC_VALUE)
  {
    number_of_permutations = other_value.number_of_permutations;
    permutation_intervals = (Pair_of_elements*) Malloc(number_of_permutations * sizeof(Pair_of_elements));
    memcpy(permutation_intervals,other_value.permutation_intervals,number_of_permutations*sizeof(Pair_of_elements));
  }
}

#ifdef TITAN_RUNTIME_2

boolean Record_Of_Template::match_function_specific(
  const Base_Type *value_ptr, int value_index,
  const Restricted_Length_Template *template_ptr, int template_index,
  boolean legacy)
{
  const Record_Of_Template* rec_tmpl_ptr = static_cast<const Record_Of_Template*>(template_ptr);
  if (value_index >= 0) {
    const Record_Of_Type* recof_ptr = static_cast<const Record_Of_Type*>(value_ptr);
    return rec_tmpl_ptr->single_value.value_elements[template_index]->matchv(recof_ptr->get_at(value_index), legacy);
  } else {
    return rec_tmpl_ptr->single_value.value_elements[template_index]->is_any_or_omit();
  }
}

void Record_Of_Template::valueofv(Base_Type* value) const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of type %s.", get_descriptor()->name);
  Record_Of_Type* recof_value = static_cast<Record_Of_Type*>(value);
  recof_value->set_size(single_value.n_elements);
  for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
    single_value.value_elements[elem_count]->valueofv(recof_value->get_at(elem_count));
  recof_value->set_err_descr(err_descr);
}

void Record_Of_Template::set_value(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  err_descr = NULL;
}

void Record_Of_Template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      delete single_value.value_elements[elem_count];
    free_pointers((void**)single_value.value_elements);
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      delete value_list.list_value[list_count];
    free_pointers((void**)value_list.list_value);
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void Record_Of_Template::copy_value(const Base_Type* other_value)
{
  if (!other_value->is_bound())
    TTCN_error("Initialization of a record of template with an unbound value.");
  const Record_Of_Type* other_recof = static_cast<const Record_Of_Type*>(other_value);
  single_value.n_elements = other_recof->size_of();
  single_value.value_elements = (Base_Template**)allocate_pointers(single_value.n_elements);
  for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
    single_value.value_elements[elem_count] = create_elem();
    if (other_recof->get_at(elem_count)->is_bound()) {
      single_value.value_elements[elem_count]->copy_value(other_recof->get_at(elem_count));
    }
  }
  set_selection(SPECIFIC_VALUE);
  err_descr = other_recof->get_err_descr();
}

void Record_Of_Template::copy_template(const Record_Of_Template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    single_value.n_elements = other_value.single_value.n_elements;
    single_value.value_elements = (Base_Template**)allocate_pointers(single_value.n_elements);
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
      if (other_value.single_value.value_elements[elem_count]->is_bound()) {
        single_value.value_elements[elem_count] = other_value.single_value.value_elements[elem_count]->clone();
      } else {
        single_value.value_elements[elem_count] = create_elem();
      }
    }
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = (Record_Of_Template**)allocate_pointers(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (other_value.value_list.list_value[list_count]->is_bound()) {
        value_list.list_value[list_count] = static_cast<Record_Of_Template*>(other_value.value_list.list_value[list_count]->clone());
      } else {
        value_list.list_value[list_count] = static_cast<Record_Of_Template*>(create_elem());
      }
    }
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported record of template.");
    break;
  }
  set_selection(other_value);
  err_descr = other_value.err_descr;
}

void Record_Of_Template::copy_optional(const Base_Type* other_value)
{
  if (other_value->is_present()) {
    const Record_Of_Type* other_recof = static_cast<const Record_Of_Type*>(other_value->get_opt_value());
    copy_value(other_recof);
  } else if (other_value->is_bound()) {
    set_selection(OMIT_VALUE);
    err_descr = NULL;
  } else {
    TTCN_error("Initialization of a record of template with an unbound optional field.");
  }
}

Base_Template* Record_Of_Template::clone() const
{
  Record_Of_Template* recof = create();
  recof->copy_template(*this);
  return recof;
}

Record_Of_Template::~Record_Of_Template()
{
    clean_up_intervals();
    clean_up();
}
#else
Record_Of_Template::~Record_Of_Template()
{
  clean_up_intervals();
}
#endif

void Record_Of_Template::encode_text_permutation(Text_Buf& text_buf) const
{
  encode_text_restricted(text_buf);
  text_buf.push_int(number_of_permutations);

  for(unsigned int i = 0; i < number_of_permutations; i++)
  {
    text_buf.push_int(permutation_intervals[i].start_index);
    text_buf.push_int(permutation_intervals[i].end_index);
  }
}

void Record_Of_Template::decode_text_permutation(Text_Buf& text_buf)
{
  decode_text_restricted(text_buf);

  number_of_permutations = text_buf.pull_int().get_val();
  permutation_intervals = (Pair_of_elements *)Malloc
    (number_of_permutations * sizeof(Pair_of_elements));

  for (unsigned int i = 0; i < number_of_permutations; i++)
  {
    permutation_intervals[i].start_index =
      text_buf.pull_int().get_val();
    permutation_intervals[i].end_index =
      text_buf.pull_int().get_val();
  }
}

void Record_Of_Template::add_permutation(unsigned int start_index, unsigned int end_index)
{
  if(start_index > end_index)
    TTCN_error("wrong permutation interval settings start (%d)"
      "can not be greater than end (%d)",start_index, end_index);

  if(number_of_permutations > 0 &&
    permutation_intervals[number_of_permutations - 1].end_index >= start_index)
    TTCN_error("the %dth permutation overlaps the previous one", number_of_permutations);

  permutation_intervals = (Pair_of_elements*)Realloc(permutation_intervals, (number_of_permutations + 1) * sizeof(Pair_of_elements));
  permutation_intervals[number_of_permutations].start_index = start_index;
  permutation_intervals[number_of_permutations].end_index = end_index;
  number_of_permutations++;
}

unsigned int Record_Of_Template::get_number_of_permutations(void) const
{
  return number_of_permutations;
}

unsigned int Record_Of_Template::get_permutation_start(unsigned int index_value) const
{
  if(index_value >= number_of_permutations)
    TTCN_error("Index overflow (%d)", index_value);

  return permutation_intervals[index_value].start_index;
}

unsigned int Record_Of_Template::get_permutation_end(unsigned int index_value) const
{
  if(index_value >= number_of_permutations)
    TTCN_error("Index overflow (%d)", index_value);

  return permutation_intervals[index_value].end_index;
}

unsigned int Record_Of_Template::get_permutation_size(unsigned int index_value) const
{
  if(index_value >= number_of_permutations)
    TTCN_error("Index overflow (%d)", index_value);

  return permutation_intervals[index_value].end_index - permutation_intervals[index_value].start_index + 1;
}

boolean Record_Of_Template::permutation_starts_at(unsigned int index_value) const
{
  for(unsigned int i = 0; i < number_of_permutations; i++)
  {
    if(permutation_intervals[i].start_index == index_value)
      return TRUE;
  }

  return FALSE;
}

boolean Record_Of_Template::permutation_ends_at(unsigned int index_value) const
{
  for(unsigned int i = 0; i < number_of_permutations; i++)
  {
    if(permutation_intervals[i].end_index == index_value)
      return TRUE;
  }

  return FALSE;
}

#ifdef TITAN_RUNTIME_2

void Record_Of_Template::substr_(int index, int returncount, Record_Of_Type* rec_of) const
{
  if (!is_value()) TTCN_error("The first argument of function substr() is a template of type %s with non-specific value.", get_descriptor()->name);
  rec_of->set_val(NULL_VALUE);
  Base_Type* this_value = rec_of->clone();
  valueofv(this_value);
  static_cast<Record_Of_Type*>(this_value)->substr_(index, returncount, rec_of);
  delete this_value;
}

void Record_Of_Template::replace_(int index, int len,
  const Record_Of_Template* repl, Record_Of_Type* rec_of) const
{
  if (!is_value()) TTCN_error("The first argument of function replace() is a template of type %s with non-specific value.", get_descriptor()->name);
  if (!repl->is_value()) TTCN_error("The fourth argument of function replace() is a template of type %s with non-specific value.", repl->get_descriptor()->name);
  rec_of->set_val(NULL_VALUE);
  Base_Type* this_value = rec_of->clone();
  valueofv(this_value);
  Base_Type* repl_value = rec_of->clone();
  repl->valueofv(repl_value);
  // call the replace() function of the value class instance
  static_cast<Record_Of_Type*>(this_value)->replace_(index, len, static_cast<Record_Of_Type*>(repl_value), rec_of);
  delete this_value;
  delete repl_value;
}

void Record_Of_Template::replace_(int index, int len,
  const Record_Of_Type* repl, Record_Of_Type* rec_of) const
{
  if (!is_value()) TTCN_error("The first argument of function replace() is a template of type %s with non-specific value.", get_descriptor()->name);
  rec_of->set_val(NULL_VALUE);
  Base_Type* this_value = rec_of->clone();
  valueofv(this_value);
  // call the replace() function of the value class instance
  static_cast<Record_Of_Type*>(this_value)->replace_(index, len, repl, rec_of);
  delete this_value;
}

Base_Template* Record_Of_Template::get_at(int index_value)
{
  if (index_value < 0)
    TTCN_error("Accessing an element of a template for type %s using a "
      "negative index: %d.", get_descriptor()->name, index_value);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    if(index_value < single_value.n_elements) break;
    // no break
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
  case UNINITIALIZED_TEMPLATE:
    set_size(index_value + 1);
    break;
  default:
    TTCN_error("Accessing an element of a non-specific template for type %s.",
      get_descriptor()->name);
    break;
  }
  return single_value.value_elements[index_value];
}

Base_Template* Record_Of_Template::get_at(const INTEGER& index_value)
{
  if (!index_value.is_bound())
    TTCN_error("Using an unbound integer value for indexing a template of "
               "type %s.", get_descriptor()->name);
  return get_at((int)index_value);
}

const Base_Template* Record_Of_Template::get_at(int index_value) const
{
  if (index_value < 0)
    TTCN_error("Accessing an element of a template for type %s using "
               "a negative index: %d.", get_descriptor()->name, index_value);
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing an element of a non-specific template for type %s.",
               get_descriptor()->name);
  if (index_value >= single_value.n_elements)
    TTCN_error("Index overflow in a template of type %s: The index is %d, but "
               "the template has only %d elements.", get_descriptor()->name, index_value,
               single_value.n_elements);
  return single_value.value_elements[index_value];
}

const Base_Template* Record_Of_Template::get_at(
  const INTEGER& index_value) const
{
  if (!index_value.is_bound())
    TTCN_error("Using an unbound integer value for indexing a template of "
               "type %s.", get_descriptor()->name);
  return get_at((int)index_value);
}

int Record_Of_Template::get_length_for_concat(boolean& is_any_value) const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    return single_value.n_elements;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    switch (length_restriction_type) {
    case NO_LENGTH_RESTRICTION:
      if (template_selection == ANY_VALUE) {
        // ? => { * }
        is_any_value = TRUE;
        return 1;
      }
      TTCN_error("Operand of record of template concatenation is an "
        "AnyValueOrNone (*) matching mechanism with no length restriction");
    case RANGE_LENGTH_RESTRICTION:
      if (!length_restriction.range_length.max_length ||
          length_restriction.range_length.max_length != length_restriction.range_length.min_length) {
        TTCN_error("Operand of record of template concatenation is an %s "
          "matching mechanism with non-fixed length restriction",
          template_selection == ANY_VALUE ? "AnyValue (?)" : "AnyValueOrNone (*)");
      }
      // else fall through (range length restriction is allowed if the minimum
      // and maximum value are the same)
    case SINGLE_LENGTH_RESTRICTION:
      // ? length(N) or * length(N) => { ?, ?, ... ? } N times
      return length_restriction_type == SINGLE_LENGTH_RESTRICTION ?
        length_restriction.single_length : length_restriction.range_length.min_length;
    }
  default:
    TTCN_error("Operand of record of template concatenation is an "
      "uninitialized or unsupported template.");
  }
}

int Record_Of_Template::get_length_for_concat(const Record_Of_Type& operand)
{
  if (!operand.is_bound()) {
    TTCN_error("Operand of record of template concatenation is an "
      "unbound value.");
  }
  return operand.val_ptr->n_elements;
}

int Record_Of_Template::get_length_for_concat(template_sel operand)
{
  if (operand == ANY_VALUE) {
    // ? => { * }
    return 1;
  }
  TTCN_error("Operand of record of template concatenation is an "
    "uninitialized or unsupported template.");
}

void Record_Of_Template::concat(int& pos, const Record_Of_Template& operand)
{
  // all errors should have already been caught by the operand's
  // get_length_for_concat() call;
  // the result template (this) should already be set to SPECIFIC_VALUE and
  // single_value.value_elements should already be allocated
  switch (operand.template_selection) {
  case SPECIFIC_VALUE:
    for (int i = 0; i < operand.single_value.n_elements; ++i) {
      single_value.value_elements[pos + i] =
        operand.single_value.value_elements[i]->clone();
    }
    pos += operand.single_value.n_elements;
    break;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    switch (operand.length_restriction_type) {
    case NO_LENGTH_RESTRICTION:
      // ? => { * }
      single_value.value_elements[pos] = create_elem();
      single_value.value_elements[pos]->set_value(ANY_OR_OMIT);
      ++pos;
      break;
    case RANGE_LENGTH_RESTRICTION:
    case SINGLE_LENGTH_RESTRICTION: {
      // ? length(N) or * length(N) => { ?, ?, ... ? } N times
      int N = operand.length_restriction_type == SINGLE_LENGTH_RESTRICTION ?
        operand.length_restriction.single_length :
        operand.length_restriction.range_length.min_length;
      for (int i = 0; i < N; ++i) {
        single_value.value_elements[pos + i] = create_elem();
        single_value.value_elements[pos + i]->set_value(ANY_VALUE);
      }
      pos += N;
      break; }
    }
  default:
    break;
  }
}

void Record_Of_Template::concat(int& pos, const Record_Of_Type& operand)
{
  // all errors should have already been caught by the
  // get_length_for_concat() call;
  // the result template (this) should already be set to SPECIFIC_VALUE and
  // single_value.value_elements should already be allocated
  for (int i = 0; i < operand.val_ptr->n_elements; ++i) {
    single_value.value_elements[pos + i] = create_elem();
    single_value.value_elements[pos + i]->copy_value(operand.get_at(i));
  }
  pos += operand.val_ptr->n_elements;
}

void Record_Of_Template::concat(int& pos)
{
  // this concatenates a template_sel to the result template;
  // there is no need for a template_sel parameter, since the only template
  // selection that can be concatenated is ANY_VALUE;
  // the template selection has already been checked in the
  // get_length_for_concat() call;
  // the result template (this) should already be set to SPECIFIC_VALUE and
  // single_value.value_elements should already be allocated
  single_value.value_elements[pos] = create_elem();
  single_value.value_elements[pos]->set_value(ANY_OR_OMIT);
  ++pos;
}

void Record_Of_Template::set_size(int new_size)
{
  if (new_size < 0)
    TTCN_error("Internal error: Setting a negative size for a template of "
               "type %s.", get_descriptor()->name);
  template_sel old_selection = template_selection;
  if (old_selection != SPECIFIC_VALUE) {
    clean_up();
    set_selection(SPECIFIC_VALUE);
    single_value.n_elements = 0;
    single_value.value_elements = NULL;
  }
  if (new_size > single_value.n_elements) {
    single_value.value_elements = (Base_Template**)reallocate_pointers(
      (void**)single_value.value_elements, single_value.n_elements, new_size);
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) {
      for (int elem_count = single_value.n_elements; elem_count < new_size; elem_count++) {
        single_value.value_elements[elem_count] = create_elem();
        single_value.value_elements[elem_count]->set_value(ANY_VALUE);
      }
    } else {
      for (int elem_count = single_value.n_elements; elem_count < new_size; elem_count++)
        single_value.value_elements[elem_count] = create_elem();
    }
    single_value.n_elements = new_size;
  } else if (new_size < single_value.n_elements) {
    for (int elem_count = new_size; elem_count < single_value.n_elements; elem_count++)
      delete single_value.value_elements[elem_count];
    single_value.value_elements = (Base_Template**)reallocate_pointers(
      (void**)single_value.value_elements, single_value.n_elements, new_size);
    single_value.n_elements = new_size;
  }
}

int Record_Of_Template::size_of(boolean is_size) const
{
  const char* op_name = is_size ? "size" : "length";
  int min_size;
  boolean has_any_or_none;
  if (is_ifpresent)
    TTCN_error("Performing %sof() operation on a template of type %s "
               "which has an ifpresent attribute.", op_name, get_descriptor()->name);
  switch (template_selection)
  {
  case SPECIFIC_VALUE: {
    min_size = 0;
    has_any_or_none = FALSE;
    int elem_count = single_value.n_elements;
    if (!is_size) {
      while (elem_count>0 && !(single_value.value_elements[elem_count-1])->is_bound())
        elem_count--;
    }
    for (int i=0; i<elem_count; i++)
    {
      switch (((Base_Template*)single_value.value_elements[i])->get_selection())
      {
      case OMIT_VALUE:
        TTCN_error("Performing %sof() operation on a template of type %s "
                   "containing omit element.", op_name, get_descriptor()->name);
      case ANY_OR_OMIT:
        has_any_or_none = TRUE;
        break;
      default:
        min_size++;
        break;
      }
    }
  } break;
  case OMIT_VALUE:
    TTCN_error("Performing %sof() operation on a template of type %s"
               " containing omit value.", op_name, get_descriptor()->name);
  case ANY_VALUE:
  case ANY_OR_OMIT:
    min_size = 0;
    has_any_or_none = TRUE;
    break;
  case VALUE_LIST:
  {
    if (value_list.n_values<1)
      TTCN_error("Performing %sof() operation on a template of type %s "
                 "containing an empty list.", op_name, get_descriptor()->name);
    int item_size = value_list.list_value[0]->size_of(is_size);
    for (int i = 1; i < value_list.n_values; i++) {
      if (value_list.list_value[i]->size_of(is_size)!=item_size)
        TTCN_error("Performing %sof() operation on a template of type %s "
          "containing a value list with different sizes.", op_name, get_descriptor()->name);
    }
    min_size = item_size;
    has_any_or_none = FALSE;
    break;
  }
  case COMPLEMENTED_LIST:
    TTCN_error("Performing %sof() operation on a template of type %s "
               "containing complemented list.", op_name, get_descriptor()->name);
  default:
    TTCN_error("Performing %sof() operation on an uninitialized/unsupported "
               "template of type %s.", op_name, get_descriptor()->name);
  }
  return check_section_is_single(min_size, has_any_or_none, op_name,
           "a template of type", get_descriptor()->name);
}

int Record_Of_Template::n_elem() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    return single_value.n_elements;

  case COMPLEMENTED_LIST:
    TTCN_error("Performing n_elem() operation on a template of type %s "
               "containing complemented list.", get_descriptor()->name);

  case UNINITIALIZED_TEMPLATE:
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
  case VALUE_LIST:
  case VALUE_RANGE:
  case STRING_PATTERN:
  case SUPERSET_MATCH:
  case SUBSET_MATCH:
  case DECODE_MATCH:
    break;
  }
  TTCN_error("Performing n_elem() operation on an uninitialized/unsupported "
             "template of type %s.", get_descriptor()->name);
}

boolean Record_Of_Template::matchv(const Base_Type* other_value,
                                   boolean legacy) const
{
  const Record_Of_Type* other_recof = static_cast<const Record_Of_Type*>(other_value);
  if (!other_value->is_bound()) return FALSE;
  int value_length = other_recof->size_of();
  if (!match_length(value_length)) return FALSE;
  switch (template_selection) {
  case SPECIFIC_VALUE:
    return match_record_of(other_recof, value_length, this,
      single_value.n_elements, match_function_specific, legacy);
  case OMIT_VALUE:
    return FALSE;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count]->matchv(other_value, legacy))
        return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching with an uninitialized/unsupported template of type %s.",
               get_descriptor()->name);
  }
  return FALSE;
}

boolean Record_Of_Template::is_value() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent) return FALSE;
  for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
    if (!single_value.value_elements[elem_count]->is_value()) return FALSE;
  return TRUE;
}

void Record_Of_Template::set_type(template_sel template_type, int list_length)
{
  clean_up();
  switch (template_type) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = list_length;
    value_list.list_value = (Record_Of_Template**)allocate_pointers(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count] = create();
    break;
  default:
    TTCN_error("Internal error: Setting an invalid type for a template of "
               "type %s.", get_descriptor()->name);
  }
  set_selection(template_type);
}

Record_Of_Template* Record_Of_Template::get_list_item(int list_index)
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST)
    TTCN_error("Internal error: Accessing a list element of a non-list "
               "template of type %s.", get_descriptor()->name);
  if (list_index < 0)
    TTCN_error("Internal error: Accessing a value list template "
               "of type %s using a negative index (%d).",
               get_descriptor()->name, list_index);
  if (list_index >= value_list.n_values)
    TTCN_error("Internal error: Index overflow in a value list template "
               "of type %s.", get_descriptor()->name);
  return value_list.list_value[list_index];
}

void Record_Of_Template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    if (single_value.n_elements > 0) {
      TTCN_Logger::log_event_str("{ ");
      for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
        if (elem_count > 0) TTCN_Logger::log_event_str(", ");
        if (permutation_starts_at(elem_count)) TTCN_Logger::log_event_str("permutation(");
        single_value.value_elements[elem_count]->log();
        if (permutation_ends_at(elem_count)) TTCN_Logger::log_char(')');
      }
      TTCN_Logger::log_event_str(" }");
    } else TTCN_Logger::log_event_str("{ }");
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count]->log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_restricted();
  log_ifpresent();
  if (err_descr) err_descr->log();
}

void Record_Of_Template::log_matchv(const Base_Type* match_value, boolean legacy) const
{
  if (TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()) {
    if (matchv(match_value, legacy)) {
      TTCN_Logger::print_logmatch_buffer();
      TTCN_Logger::log_event_str(" matched");
    } else {
      const Record_Of_Type* recof_value = static_cast<const Record_Of_Type*>(match_value);
      if (template_selection == SPECIFIC_VALUE &&
          single_value.n_elements > 0 && get_number_of_permutations() == 0 &&
          single_value.n_elements == recof_value->size_of()) {
        size_t previous_size = TTCN_Logger::get_logmatch_buffer_len();
        for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
          if(!single_value.value_elements[elem_count]->matchv(recof_value->get_at(elem_count), legacy)){
            TTCN_Logger::log_logmatch_info("[%d]", elem_count);
            single_value.value_elements[elem_count]->log_matchv(recof_value->get_at(elem_count), legacy);
            TTCN_Logger::set_logmatch_buffer_len(previous_size);
          }
        }
        log_match_length(single_value.n_elements);
      } else {
        TTCN_Logger::print_logmatch_buffer();
        match_value->log();
        TTCN_Logger::log_event_str(" with ");
        log();
        TTCN_Logger::log_event_str(" unmatched");
      }
    }
  } else {
    const Record_Of_Type* recof_value = static_cast<const Record_Of_Type*>(match_value);
    if (template_selection == SPECIFIC_VALUE &&
        single_value.n_elements > 0 && get_number_of_permutations() == 0 &&
        single_value.n_elements == recof_value->size_of()) {
      TTCN_Logger::log_event_str("{ ");
      for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
        if (elem_count > 0) TTCN_Logger::log_event_str(", ");
        single_value.value_elements[elem_count]->log_matchv(recof_value->get_at(elem_count), legacy);
      }
      TTCN_Logger::log_event_str(" }");
      log_match_length(single_value.n_elements);
    } else {
      match_value->log();
      TTCN_Logger::log_event_str(" with ");
      log();
      if (matchv(match_value, legacy)) TTCN_Logger::log_event_str(" matched");
      else TTCN_Logger::log_event_str(" unmatched");
    }
  }
}

void Record_Of_Template::encode_text(Text_Buf& text_buf) const
{
  encode_text_permutation(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    text_buf.push_int(single_value.n_elements);
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      single_value.value_elements[elem_count]->encode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count]->encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported template "
               "of type %s.", get_descriptor()->name);
  }
}

void Record_Of_Template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_permutation(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value.n_elements = text_buf.pull_int().get_val();
    if (single_value.n_elements < 0)
      TTCN_error("Text decoder: Negative size was received for a template of "
                 "type %s.", get_descriptor()->name);
    single_value.value_elements = (Base_Template**)allocate_pointers(single_value.n_elements);
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
      single_value.value_elements[elem_count] = create_elem();
      single_value.value_elements[elem_count]->decode_text(text_buf);
    }
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = (Record_Of_Template**)allocate_pointers(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++) {
      value_list.list_value[list_count] = create();
      value_list.list_value[list_count]->decode_text(text_buf);
    }
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received "
               "for a template of type %s.", get_descriptor()->name);
  }
}

boolean Record_Of_Template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean Record_Of_Template::match_omit(boolean legacy /* = FALSE */) const
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
      for (int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i]->match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    }
    // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

void Record_Of_Template::set_param(Module_Param& param)
{
  if (dynamic_cast<Module_Param_Name*>(param.get_id()) != NULL &&
      param.get_id()->next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the elements, not to the whole record of
    char* param_field = param.get_id()->get_current_name();
    if (param_field[0] < '0' || param_field[0] > '9') {
      param.error("Unexpected record field name in module parameter, expected a valid"
        " index for record of template type `%s'", get_descriptor()->name);
    }
    int param_index = -1;
    sscanf(param_field, "%d", &param_index);
    get_at(param_index)->set_param(param);
    return;
  }
  
  param.basic_check(Module_Param::BC_TEMPLATE|Module_Param::BC_LIST, "record of template");
  
  Module_Param_Ptr mp = &param;
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
  
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    set_value(OMIT_VALUE);
    break;
  case Module_Param::MP_Any:
    set_value(ANY_VALUE);
    break;
  case Module_Param::MP_AnyOrNone:
    set_value(ANY_OR_OMIT);
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    Record_Of_Template** list_items = (Record_Of_Template**)
      allocate_pointers(mp->get_size());
    for (size_t i = 0; i < mp->get_size(); i++) {
      list_items[i] = create();
      list_items[i]->set_param(*mp->get_elem(i));
    }
    clean_up();
    template_selection = mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST;
    value_list.n_values = mp->get_size();
    value_list.list_value = list_items;
    break; }
  case Module_Param::MP_Value_List: {
    set_size(mp->get_size()); // at least this size if there are no permutation elements, if there are then get_at() will automatically resize
    int curr_idx = 0; // current index into this
    for (size_t i=0; i<mp->get_size(); ++i) {
      Module_Param* const curr = mp->get_elem(i);
      switch (curr->get_type()) {
      case Module_Param::MP_NotUsed:
        // skip this element
        curr_idx++;
        break;
      case Module_Param::MP_Permutation_Template: {
        // insert all elements from the permutation
        int perm_start_idx = curr_idx;
        for (size_t perm_i=0; perm_i<curr->get_size(); perm_i++) {
          get_at(curr_idx)->set_param(*(curr->get_elem(perm_i)));
          curr_idx++;
        }
        int perm_end_idx = curr_idx - 1;
        add_permutation(perm_start_idx, perm_end_idx);
      } break;
      default:
        get_at(curr_idx)->set_param(*curr);
        curr_idx++;
      }
    }
  } break;
  case Module_Param::MP_Indexed_List:
    if (template_selection!=SPECIFIC_VALUE) set_size(0);
    for (size_t i=0; i<mp->get_size(); ++i) {
      Module_Param* const current = mp->get_elem(i);
      get_at((int)current->get_id()->get_index())->set_param(*current);
    }
    break;
  default:
    param.type_error("record of template", get_descriptor()->name);
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
  if (param.get_length_restriction() != NULL) {
    set_length_range(param);
  }
  else {
    set_length_range(*mp);
  }
}

Module_Param* Record_Of_Template::get_param(Module_Param_Name& param_name) const
{
  if (param_name.next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the elements, not to the whole record of
    char* param_field = param_name.get_current_name();
    if (param_field[0] < '0' || param_field[0] > '9') {
      TTCN_error("Unexpected record field name in module parameter reference, "
        "expected a valid index for record of template type `%s'",
        get_descriptor()->name);
    }
    int param_index = -1;
    sscanf(param_field, "%d", &param_index);
    return get_at(param_index)->get_param(param_name);
  }
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
  case SPECIFIC_VALUE: {
    Vector<Module_Param*> values;
    for (int i = 0; i < single_value.n_elements; ++i) {
      values.push_back(single_value.value_elements[i]->get_param(param_name));
    }
    mp = new Module_Param_Value_List();
    mp->add_list_with_implicit_ids(&values);
    values.clear();
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (int i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i]->get_param(param_name));
    }
    break; }
  default:
    TTCN_error("Referencing an uninitialized/unsupported template of type %s.", get_descriptor()->name);
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  mp->set_length_restriction(get_length_range());
  return mp;
}

boolean Record_Of_Template::get_istemplate_kind(const char* type) const {
  if (!strcmp(type, "AnyElement")) {
    if (template_selection != SPECIFIC_VALUE) {
      return FALSE;
    }
    for (int i = 0; i < single_value.n_elements; i++) {
      if (single_value.value_elements[i]->get_selection() == ANY_VALUE) {
        return TRUE;
      }
    }
    return FALSE;
  } else if (!strcmp(type, "AnyElementsOrNone")) {
    if (template_selection != SPECIFIC_VALUE) {
      return FALSE;
    }
    for (int i = 0; i < single_value.n_elements; i++) {
      if (single_value.value_elements[i]->get_selection() == ANY_OR_OMIT) {
        return TRUE;
      }
    }
    return FALSE;
  } else if (!strcmp(type, "permutation")) {
      return number_of_permutations;
  } else if (!strcmp(type, "length")) {
      return length_restriction_type != NO_LENGTH_RESTRICTION;
  } else {
     return Base_Template::get_istemplate_kind(type);
  }
}


void Record_Of_Template::check_restriction(template_res t_res, const char* t_name,
                                           boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return;
  switch ((t_name && (t_res==TR_VALUE)) ? TR_OMIT : t_res) {
  case TR_OMIT:
    if (template_selection==OMIT_VALUE) return;
    // no break
  case TR_VALUE:
    if (template_selection!=SPECIFIC_VALUE || is_ifpresent) break;
    for (int i=0; i<single_value.n_elements; i++)
      single_value.value_elements[i]->check_restriction(t_res, t_name ? t_name : get_descriptor()->name);
    return;
  case TR_PRESENT:
    if (!match_omit(legacy)) return;
    break;
  default:
    return;
  }
  TTCN_error("Restriction `%s' on template of type %s violated.",
             get_res_name(t_res), t_name ? t_name : get_descriptor()->name);
}

////////////////////////////////////////////////////////////////////////////////

void Set_Of_Template::substr_(int index, int returncount, Record_Of_Type* rec_of) const
{
  if (!is_value()) TTCN_error("The first argument of function substr() is a template of type %s with non-specific value.", get_descriptor()->name);
  rec_of->set_val(NULL_VALUE);
  Base_Type* this_value = rec_of->clone();
  valueofv(this_value);
  static_cast<Record_Of_Type*>(this_value)->substr_(index, returncount, rec_of);
  delete this_value;
}

void Set_Of_Template::replace_(int index, int len,
  const Set_Of_Template* repl, Record_Of_Type* rec_of) const
{
  if (!is_value()) TTCN_error("The first argument of function replace() is a template of type %s with non-specific value.", get_descriptor()->name);
  if (!repl->is_value()) TTCN_error("The fourth argument of function replace() is a template of type %s with non-specific value.", repl->get_descriptor()->name);
  rec_of->set_val(NULL_VALUE);
  Base_Type* this_value = rec_of->clone();
  valueofv(this_value);
  Base_Type* repl_value = rec_of->clone();
  repl->valueofv(repl_value);
  // call the replace() function of the value class instance
  static_cast<Record_Of_Type*>(this_value)->replace_(index, len, static_cast<Record_Of_Type*>(repl_value), rec_of);
  delete this_value;
  delete repl_value;
}

void Set_Of_Template::replace_(int index, int len,
  const Record_Of_Type* repl, Record_Of_Type* rec_of) const
{
  if (!is_value()) TTCN_error("The first argument of function replace() is a template of type %s with non-specific value.", get_descriptor()->name);
  rec_of->set_val(NULL_VALUE);
  Base_Type* this_value = rec_of->clone();
  valueofv(this_value);
  // call the replace() function of the value class instance
  static_cast<Record_Of_Type*>(this_value)->replace_(index, len, repl, rec_of);
  delete this_value;
}

Set_Of_Template::Set_Of_Template(): err_descr(NULL)
{
}

Set_Of_Template::Set_Of_Template(template_sel other_value)
    : Restricted_Length_Template(other_value), err_descr(NULL)
{
}

Set_Of_Template::~Set_Of_Template()
{
  clean_up();
}

void Set_Of_Template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
  case SUPERSET_MATCH:
  case SUBSET_MATCH:
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      delete single_value.value_elements[elem_count];
    free_pointers((void**)single_value.value_elements);
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (int elem_count = 0; elem_count < value_list.n_values; elem_count++)
      delete value_list.list_value[elem_count];
    free_pointers((void**)value_list.list_value);
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void Set_Of_Template::copy_value(const Base_Type* other_value)
{
  if (!other_value->is_bound())
    TTCN_error("Initialization of a set of template with an unbound value.");
  const Record_Of_Type* other_setof = static_cast<const Record_Of_Type*>(other_value);
  single_value.n_elements = other_setof->size_of();
  single_value.value_elements = (Base_Template**)allocate_pointers(single_value.n_elements);
  for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
    single_value.value_elements[elem_count] = create_elem();
    single_value.value_elements[elem_count]->copy_value(other_setof->get_at(elem_count));
  }
  set_selection(SPECIFIC_VALUE);
  err_descr = other_setof->get_err_descr();
}

void Set_Of_Template::copy_optional(const Base_Type* other_value)
{
  if (other_value->is_present()) {
    const Record_Of_Type* other_setof = static_cast<const Record_Of_Type*>(other_value->get_opt_value());
    copy_value(other_setof);
  } else if (other_value->is_bound()) {
    set_selection(OMIT_VALUE);
    err_descr = NULL;
  } else {
    TTCN_error("Initialization of a set of template with an unbound optional field.");
  }
}

void Set_Of_Template::copy_template(const Set_Of_Template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
  case SUPERSET_MATCH:
  case SUBSET_MATCH:
    single_value.n_elements = other_value.single_value.n_elements;
    single_value.value_elements = (Base_Template**)allocate_pointers(single_value.n_elements);
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      single_value.value_elements[elem_count] = other_value.single_value.value_elements[elem_count]->clone();
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = (Set_Of_Template**)allocate_pointers(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count] = static_cast<Set_Of_Template*>(other_value.value_list.list_value[list_count]->clone());
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported set of template");
    break;
  }
  set_selection(other_value);
  err_descr = other_value.err_descr;
}

Base_Template* Set_Of_Template::clone() const
{
  Set_Of_Template* setof = create();
  setof->copy_template(*this);
  return setof;
}

Base_Template* Set_Of_Template::get_at(int index_value)
{
  if (index_value < 0)
    TTCN_error("Accessing an element of a template for type %s using a "
      "negative index: %d.", get_descriptor()->name, index_value);
  if (template_selection != SPECIFIC_VALUE ||
      index_value >= single_value.n_elements) set_size(index_value + 1);
  return single_value.value_elements[index_value];
}

Base_Template* Set_Of_Template::get_at(const INTEGER& index_value)
{
  if (!index_value.is_bound())
    TTCN_error("Using an unbound integer value for indexing a template of "
               "type %s.", get_descriptor()->name);
  return get_at((int)index_value);
}

const Base_Template* Set_Of_Template::get_at(int index_value) const
{
  if (index_value < 0)
    TTCN_error("Accessing an element of a template for type %s using "
               "a negative index: %d.", get_descriptor()->name, index_value);
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing an element of a non-specific template for type %s.",
               get_descriptor()->name);
  if (index_value >= single_value.n_elements)
    TTCN_error("Index overflow in a template of type %s: The index is %d, but "
               "the template has only %d elements.", get_descriptor()->name,
               index_value, single_value.n_elements);
  return single_value.value_elements[index_value];
}

const Base_Template* Set_Of_Template::get_at(const INTEGER& index_value) const
{
  if (!index_value.is_bound())
    TTCN_error("Using an unbound integer value for indexing a template of "
               "type %s.", get_descriptor()->name);
  return get_at((int)index_value);
}

int Set_Of_Template::get_length_for_concat(boolean& is_any_value) const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    return single_value.n_elements;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    switch (length_restriction_type) {
    case NO_LENGTH_RESTRICTION:
      if (template_selection == ANY_VALUE) {
        // ? => { * }
        is_any_value = TRUE;
        return 1;
      }
      TTCN_error("Operand of record of template concatenation is an "
        "AnyValueOrNone (*) matching mechanism with no length restriction");
    case RANGE_LENGTH_RESTRICTION:
      if (!length_restriction.range_length.max_length ||
          length_restriction.range_length.max_length != length_restriction.range_length.min_length) {
        TTCN_error("Operand of record of template concatenation is an %s "
          "matching mechanism with non-fixed length restriction",
          template_selection == ANY_VALUE ? "AnyValue (?)" : "AnyValueOrNone (*)");
      }
      // else fall through (range length restriction is allowed if the minimum
      // and maximum value are the same)
    case SINGLE_LENGTH_RESTRICTION:
      // ? length(N) or * length(N) => { ?, ?, ... ? } N times
      return length_restriction_type == SINGLE_LENGTH_RESTRICTION ?
        length_restriction.single_length : length_restriction.range_length.min_length;
    }
  default:
    TTCN_error("Operand of record of template concatenation is an "
      "uninitialized or unsupported template.");
  }
}

int Set_Of_Template::get_length_for_concat(const Record_Of_Type& operand)
{
  if (!operand.is_bound()) {
    TTCN_error("Operand of record of template concatenation is an "
      "unbound value.");
  }
  return operand.val_ptr->n_elements;
}

int Set_Of_Template::get_length_for_concat(template_sel operand)
{
  if (operand == ANY_VALUE) {
    // ? => { * }
    return 1;
  }
  TTCN_error("Operand of record of template concatenation is an "
    "uninitialized or unsupported template.");
}

void Set_Of_Template::concat(int& pos, const Set_Of_Template& operand)
{
  // all errors should have already been caught by the operand's
  // get_length_for_concat() call;
  // the result template (this) should already be set to SPECIFIC_VALUE and
  // single_value.value_elements should already be allocated
  switch (operand.template_selection) {
  case SPECIFIC_VALUE:
    for (int i = 0; i < operand.single_value.n_elements; ++i) {
      single_value.value_elements[pos + i] =
        operand.single_value.value_elements[i]->clone();
    }
    pos += operand.single_value.n_elements;
    break;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    switch (operand.length_restriction_type) {
    case NO_LENGTH_RESTRICTION:
      // ? => { * }
      single_value.value_elements[pos] = create_elem();
      single_value.value_elements[pos]->set_value(ANY_OR_OMIT);
      ++pos;
      break;
    case RANGE_LENGTH_RESTRICTION:
    case SINGLE_LENGTH_RESTRICTION: {
      // ? length(N) or * length(N) => { ?, ?, ... ? } N times
      int N = operand.length_restriction_type == SINGLE_LENGTH_RESTRICTION ?
        operand.length_restriction.single_length :
        operand.length_restriction.range_length.min_length;
      for (int i = 0; i < N; ++i) {
        single_value.value_elements[pos + i] = create_elem();
        single_value.value_elements[pos + i]->set_value(ANY_VALUE);
      }
      pos += N;
      break; }
    }
  default:
    break;
  }
}

void Set_Of_Template::concat(int& pos, const Record_Of_Type& operand)
{
  // all errors should have already been caught by the
  // get_length_for_concat() call;
  // the result template (this) should already be set to SPECIFIC_VALUE and
  // single_value.value_elements should already be allocated
  for (int i = 0; i < operand.val_ptr->n_elements; ++i) {
    single_value.value_elements[pos + i] = create_elem();
    single_value.value_elements[pos + i]->copy_value(operand.get_at(i));
  }
  pos += operand.val_ptr->n_elements;
}

void Set_Of_Template::concat(int& pos)
{
  // this concatenates a template_sel to the result template;
  // there is no need for a template_sel parameter, since the only template
  // selection that can be concatenated is ANY_VALUE;
  // the template selection has already been checked in the
  // get_length_for_concat() call;
  // the result template (this) should already be set to SPECIFIC_VALUE and
  // single_value.value_elements should already be allocated
  single_value.value_elements[pos] = create_elem();
  single_value.value_elements[pos]->set_value(ANY_OR_OMIT);
  ++pos;
}

void Set_Of_Template::set_size(int new_size)
{
  if (new_size < 0)
    TTCN_error("Internal error: Setting a negative size for a template of "
               "type %s.", get_descriptor()->name);
  template_sel old_selection = template_selection;
  if (old_selection != SPECIFIC_VALUE) {
    clean_up();
    set_selection(SPECIFIC_VALUE);
    single_value.n_elements = 0;
    single_value.value_elements = NULL;
  }
  if (new_size > single_value.n_elements) {
    single_value.value_elements = (Base_Template**)reallocate_pointers((void**)
      single_value.value_elements, single_value.n_elements, new_size);
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) {
      for (int elem_count = single_value.n_elements;
           elem_count < new_size; elem_count++) {
        single_value.value_elements[elem_count] = create_elem();
        single_value.value_elements[elem_count]->set_value(ANY_VALUE);
      }
    } else {
      for (int elem_count = single_value.n_elements;
           elem_count < new_size; elem_count++)
        single_value.value_elements[elem_count] = create_elem();
    }
    single_value.n_elements = new_size;
  } else if (new_size < single_value.n_elements) {
    for (int elem_count = new_size;
         elem_count < single_value.n_elements; elem_count++)
      delete single_value.value_elements[elem_count];
    single_value.value_elements = (Base_Template**)reallocate_pointers((void**)
      single_value.value_elements, single_value.n_elements, new_size);
    single_value.n_elements = new_size;
  }
}

int Set_Of_Template::size_of(boolean is_size) const
{
  const char* op_name = is_size ? "size" : "length";
  int min_size = -1;
  boolean has_any_or_none = FALSE;
  if (is_ifpresent)
    TTCN_error("Performing %sof() operation on a template of type %s "
               "which has an ifpresent attribute.", op_name, get_descriptor()->name);
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
  case SUPERSET_MATCH:
  case SUBSET_MATCH: {
    min_size = 0;
    has_any_or_none = FALSE;
    int elem_count = single_value.n_elements;
    if (!is_size) {
      while (elem_count>0 && !single_value.value_elements[elem_count-1]->is_bound())
        elem_count--;
    }
    for (int i=0; i<elem_count; i++)
    {
      switch (single_value.value_elements[i]->get_selection())
      {
      case OMIT_VALUE:
        TTCN_error("Performing %sof() operation on a template of type %s "
                   "containing omit element.", op_name, get_descriptor()->name);
      case ANY_OR_OMIT:
        has_any_or_none = TRUE;
        break;
      default:
        min_size++;
        break;
      }
    }
    if (template_selection==SUPERSET_MATCH)
    {
      has_any_or_none = TRUE;
    }
    else if (template_selection==SUBSET_MATCH)
    {
      int max_size = min_size;
      min_size = 0;
      if (!has_any_or_none) // [0,max_size]
      {
        switch (length_restriction_type)
        {
        case NO_LENGTH_RESTRICTION:
          if (max_size==0) return 0;
          TTCN_error("Performing %sof() operation on a template of type "
                     "%s with no exact size.", op_name, get_descriptor()->name);
        case SINGLE_LENGTH_RESTRICTION:
          if (length_restriction.single_length<=max_size)
            return length_restriction.single_length;
          TTCN_error("Performing %sof() operation on an invalid template of "
            "type %s. The maximum size (%d) contradicts the length restriction "
            "(%d).", op_name, get_descriptor()->name, max_size,
            length_restriction.single_length);
        case RANGE_LENGTH_RESTRICTION:
          if (max_size==length_restriction.range_length.min_length)
            return max_size;
          else if (max_size>length_restriction.range_length.min_length)
            TTCN_error("Performing %sof() operation on a template of type %s "
                       "with no exact size.", op_name, get_descriptor()->name);
          else
            TTCN_error("Performing %sof() operation on an invalid template of "
              "type %s. Maximum size (%d) contradicts the length restriction "
              "(%d..%d).", op_name, get_descriptor()->name, max_size,
              length_restriction.range_length.min_length,
              length_restriction.range_length.max_length);
        default:
          TTCN_error("Internal error: Template has invalid length restriction type.");
        }
      }
    }
  } break;
  case OMIT_VALUE:
    TTCN_error("Performing %sof() operation on a template of type %s"
               " containing omit value.", op_name, get_descriptor()->name);
  case ANY_VALUE:
  case ANY_OR_OMIT:
    min_size = 0;
    has_any_or_none = TRUE;
    break;
  case VALUE_LIST:
  {
    if (value_list.n_values<1)
      TTCN_error("Performing %sof() operation on a template of type %s "
                 "containing an empty list.", op_name, get_descriptor()->name);
    int item_size = value_list.list_value[0]->size_of(is_size);
    for (int i = 1; i < value_list.n_values; i++) {
      if (value_list.list_value[i]->size_of(is_size)!=item_size)
        TTCN_error("Performing %sof() operation on a template of type %s "
          "containing a value list with different sizes.", op_name, get_descriptor()->name);
    }
    min_size = item_size;
    has_any_or_none = FALSE;
    break;
  }
  case COMPLEMENTED_LIST:
    TTCN_error("Performing %sof() operation on a template of type %s "
               "containing complemented list.", op_name, get_descriptor()->name);
  case UNINITIALIZED_TEMPLATE:
  case VALUE_RANGE:
  case STRING_PATTERN:
  case DECODE_MATCH:
    TTCN_error("Performing %sof() operation on an uninitialized/unsupported "
               "template of type %s.", op_name, get_descriptor()->name);
  }
  return check_section_is_single(min_size, has_any_or_none, op_name,
           "a template of type", get_descriptor()->name);
}

int Set_Of_Template::n_elem() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
  case SUPERSET_MATCH:
  case SUBSET_MATCH:
    return single_value.n_elements;

  case COMPLEMENTED_LIST:
    TTCN_error("Performing n_elem() operation on a template of type %s "
               "containing complemented list.", get_descriptor()->name);

  case UNINITIALIZED_TEMPLATE:
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
  case VALUE_LIST:
  case VALUE_RANGE:
  case STRING_PATTERN:
  case DECODE_MATCH:
    break;
  }
  TTCN_error("Performing n_elem() operation on an uninitialized/unsupported "
             "template of type %s.", get_descriptor()->name);
}

boolean Set_Of_Template::match_function_specific(
  const Base_Type *value_ptr, int value_index,
  const Restricted_Length_Template *template_ptr, int template_index,
  boolean legacy)
{
  const Set_Of_Template* set_tmpl_ptr = static_cast<const Set_Of_Template*>(template_ptr);
  if (value_index >= 0) {
    const Record_Of_Type* recof_ptr = static_cast<const Record_Of_Type*>(value_ptr);
    return set_tmpl_ptr->single_value.value_elements[template_index]->matchv(recof_ptr->get_at(value_index), legacy);
  } else {
    return set_tmpl_ptr->single_value.value_elements[template_index]->is_any_or_omit();
  }
}

boolean Set_Of_Template::match_function_set(
  const Base_Type *value_ptr, int value_index,
  const Restricted_Length_Template *template_ptr, int template_index,
  boolean legacy)
{
  const Set_Of_Template* set_tmpl_ptr = static_cast<const Set_Of_Template*>(template_ptr);
  if (value_index >= 0) {
    const Record_Of_Type* recof_ptr = static_cast<const Record_Of_Type*>(value_ptr);
    return set_tmpl_ptr->single_value.value_elements[template_index]->matchv(recof_ptr->get_at(value_index), legacy);
  } else {
    return set_tmpl_ptr->single_value.value_elements[template_index]->is_any_or_omit();
  }
}

void Set_Of_Template::log_function(
  const Base_Type *value_ptr, const Restricted_Length_Template *template_ptr,
  int index_value, int index_template, boolean legacy)
{
  const Set_Of_Template* set_tmpl_ptr = static_cast<const Set_Of_Template*>(template_ptr);
  const Record_Of_Type* recof_ptr = static_cast<const Record_Of_Type*>(value_ptr);
  if (value_ptr != NULL && template_ptr != NULL)
    set_tmpl_ptr->single_value.value_elements[index_template]->log_matchv(recof_ptr->get_at(index_value), legacy);
  else if (value_ptr != NULL)
    recof_ptr->get_at(index_value)->log();
  else if (template_ptr != NULL)
    set_tmpl_ptr->single_value.value_elements[index_template]->log();
}

boolean Set_Of_Template::matchv(const Base_Type* other_value,
                                boolean legacy) const
{
  const Record_Of_Type* other_recof = static_cast<const Record_Of_Type*>(other_value);
  if (!other_recof->is_bound())
    TTCN_error("Matching an unbound value of type %s with a template.",
               other_recof->get_descriptor()->name);
  int value_length = other_recof->size_of();
  if (!match_length(value_length)) return FALSE;
  switch (template_selection) {
  case SPECIFIC_VALUE:
    return match_set_of(other_recof, value_length, this,
                        single_value.n_elements, match_function_specific, legacy);
  case OMIT_VALUE:
    return FALSE;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count]->matchv(other_recof, legacy))
        return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  case SUPERSET_MATCH:
  case SUBSET_MATCH:
    return match_set_of(other_recof, value_length, this,
                        single_value.n_elements, match_function_set, legacy);
  default:
    TTCN_error("Matching with an uninitialized/unsupported template of type %s.",
               get_descriptor()->name);
  }
  return FALSE;
}

boolean Set_Of_Template::is_value() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent) return FALSE;
  for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
    if (!single_value.value_elements[elem_count]->is_value()) return FALSE;
  return TRUE;
}

void Set_Of_Template::valueofv(Base_Type* value) const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of type %s.", get_descriptor()->name);
  Record_Of_Type* setof_value = static_cast<Record_Of_Type*>(value);
  setof_value->set_size(single_value.n_elements);
  for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
    single_value.value_elements[elem_count]->valueofv(setof_value->get_at(elem_count));
  setof_value->set_err_descr(err_descr);
}

void Set_Of_Template::set_value(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  err_descr = NULL;
}

void Set_Of_Template::set_type(template_sel template_type, int list_length)
{
  clean_up();
  switch (template_type) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = list_length;
    value_list.list_value = (Set_Of_Template**)allocate_pointers(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count] = create();
    break;
  case SUPERSET_MATCH:
  case SUBSET_MATCH:
    single_value.n_elements = list_length;
    single_value.value_elements = (Base_Template**)allocate_pointers(single_value.n_elements);
    for (int set_count = 0; set_count < single_value.n_elements; set_count++)
      single_value.value_elements[set_count] = create_elem();
    break;
  default:
    TTCN_error("Internal error: Setting an invalid type for a template of "
               "type %s.", get_descriptor()->name);
  }
  set_selection(template_type);
}

Set_Of_Template* Set_Of_Template::get_list_item(int list_index)
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST)
    TTCN_error("Internal error: Accessing a list element of a non-list "
               "template of type %s.", get_descriptor()->name);
  if (list_index < 0)
    TTCN_error("Internal error: Accessing a value list template "
               "of type %s using a negative index (%d).",
               get_descriptor()->name, list_index);
  if (list_index >= value_list.n_values)
    TTCN_error("Internal error: Index overflow in a value list template "
               "of type %s.", get_descriptor()->name);
  return value_list.list_value[list_index];
}

Base_Template* Set_Of_Template::get_set_item(int set_index)
{
  if (template_selection != SUPERSET_MATCH && template_selection != SUBSET_MATCH)
    TTCN_error("Internal error: Accessing a set element of a non-set "
	           "template of type %s.", get_descriptor()->name);
  if (set_index >= single_value.n_elements || set_index < 0)
    TTCN_error("Internal error: Index overflow in a set template of "
	           "type %s.", get_descriptor()->name);
  return single_value.value_elements[set_index];
}

void Set_Of_Template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    if (single_value.n_elements > 0) {
      TTCN_Logger::log_event_str("{ ");
      for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
        if (elem_count > 0) TTCN_Logger::log_event_str(", ");
        single_value.value_elements[elem_count]->log();
      }
      TTCN_Logger::log_event_str(" }");
    } else TTCN_Logger::log_event_str("{ }");
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count]->log();
    }
    TTCN_Logger::log_char(')');
    break;
  case SUPERSET_MATCH:
  case SUBSET_MATCH:
    TTCN_Logger::log_event("%s(",
      template_selection == SUPERSET_MATCH ? "superset" : "subset");
    for (int set_count = 0; set_count < single_value.n_elements; set_count++) {
      if (set_count > 0) TTCN_Logger::log_event_str(", ");
      single_value.value_elements[set_count]->log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_restricted();
  log_ifpresent();
  if (err_descr) err_descr->log();
}

void Set_Of_Template::log_matchv(const Base_Type* match_value, boolean legacy) const
{
  if (TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()) {
    if (matchv(match_value, legacy)) {
      TTCN_Logger::print_logmatch_buffer();
      TTCN_Logger::log_event_str(" matched");
    } else {
      size_t previous_size = TTCN_Logger::get_logmatch_buffer_len();
      if (template_selection == SPECIFIC_VALUE) {
        const Record_Of_Type* setof_value = static_cast<const Record_Of_Type*>(match_value);
        log_match_heuristics(setof_value, setof_value->size_of(), this,
          single_value.n_elements, match_function_specific, log_function, legacy);
      } else {
        if (previous_size != 0) {
          TTCN_Logger::print_logmatch_buffer();
          TTCN_Logger::set_logmatch_buffer_len(previous_size);
          TTCN_Logger::log_event_str(":=");
        }
        match_value->log();
        TTCN_Logger::log_event_str(" with ");
        log();
        TTCN_Logger::log_event_str(" unmatched");
      }
    }
  } else {
    match_value->log();
    TTCN_Logger::log_event_str(" with ");
    log();
    if (matchv(match_value, legacy)) TTCN_Logger::log_event_str(" matched");
    else {
      TTCN_Logger::log_event_str(" unmatched");
      if (template_selection == SPECIFIC_VALUE) {
        const Record_Of_Type* setof_value = static_cast<const Record_Of_Type*>(match_value);
        log_match_heuristics(setof_value, setof_value->size_of(), this,
          single_value.n_elements, match_function_specific, log_function, legacy);
      }
    }
  }
}

void Set_Of_Template::encode_text(Text_Buf& text_buf) const
{
  encode_text_restricted(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
  case SUPERSET_MATCH:
  case SUBSET_MATCH:
    text_buf.push_int(single_value.n_elements);
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      single_value.value_elements[elem_count]->encode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count]->encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported template "
               "of type %s.", get_descriptor()->name);
  }
}

void Set_Of_Template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_restricted(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
  case SUPERSET_MATCH:
  case SUBSET_MATCH:
    single_value.n_elements = text_buf.pull_int().get_val();
    if (single_value.n_elements < 0)
      TTCN_error("Text decoder: Negative size was received for a template of "
                 "type %s.", get_descriptor()->name);
    single_value.value_elements = (Base_Template**)allocate_pointers(single_value.n_elements);
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
      single_value.value_elements[elem_count] = create_elem();
      single_value.value_elements[elem_count]->decode_text(text_buf);
    }
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = (Set_Of_Template**)allocate_pointers(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++) {
      value_list.list_value[list_count] = create();
      value_list.list_value[list_count]->decode_text(text_buf);
    }
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received "
               "for a template of type %s.", get_descriptor()->name);
  }
}

boolean Set_Of_Template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean Set_Of_Template::match_omit(boolean legacy /* = FALSE */) const
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
      for (int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i]->match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    }
    // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

void Set_Of_Template::set_param(Module_Param& param)
{
  if (dynamic_cast<Module_Param_Name*>(param.get_id()) != NULL &&
      param.get_id()->next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the elements, not to the whole record of
    char* param_field = param.get_id()->get_current_name();
    if (param_field[0] < '0' || param_field[0] > '9') {
      param.error("Unexpected record field name in module parameter, expected a valid"
        " index for set of template type `%s'", get_descriptor()->name);
    }
    int param_index = -1;
    sscanf(param_field, "%d", &param_index);
    get_at(param_index)->set_param(param);
    return;
  }
  
  param.basic_check(Module_Param::BC_TEMPLATE|Module_Param::BC_LIST, "set of template");
  
  Module_Param_Ptr mp = &param;
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
  
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    set_value(OMIT_VALUE);
    break;
  case Module_Param::MP_Any:
    set_value(ANY_VALUE);
    break;
  case Module_Param::MP_AnyOrNone:
    set_value(ANY_OR_OMIT);
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    Set_Of_Template** list_items = (Set_Of_Template**)
      allocate_pointers(mp->get_size());
    for (size_t i = 0; i < mp->get_size(); i++) {
      list_items[i] = create();
      list_items[i]->set_param(*mp->get_elem(i));
    }
    clean_up();
    template_selection = mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST;
    value_list.n_values = mp->get_size();
    value_list.list_value = list_items;
    break; }
  case Module_Param::MP_Value_List:
    set_size(mp->get_size());
    for (size_t i=0; i<mp->get_size(); ++i) {
      Module_Param* const curr = mp->get_elem(i);
      if (curr->get_type()!=Module_Param::MP_NotUsed) {
        get_at(i)->set_param(*curr);
      }
    }
    break;
  case Module_Param::MP_Indexed_List:
    if (template_selection!=SPECIFIC_VALUE) set_size(0);
    for (size_t i=0; i<mp->get_size(); ++i) {
      Module_Param* const current = mp->get_elem(i);
      get_at((int)current->get_id()->get_index())->set_param(*current);
    }
    break;
  case Module_Param::MP_Superset_Template:
  case Module_Param::MP_Subset_Template:
    set_type(mp->get_type()==Module_Param::MP_Superset_Template ? SUPERSET_MATCH : SUBSET_MATCH, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      get_set_item((int)i)->set_param(*mp->get_elem(i));
    }
    break;
  default:
    param.type_error("set of template", get_descriptor()->name);
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
  if (param.get_length_restriction() != NULL) {
    set_length_range(param);
  }
  else {
    set_length_range(*mp);
  }
}

Module_Param* Set_Of_Template::get_param(Module_Param_Name& param_name) const
{
  if (param_name.next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the elements, not to the whole record of
    char* param_field = param_name.get_current_name();
    if (param_field[0] < '0' || param_field[0] > '9') {
      TTCN_error("Unexpected record field name in module parameter reference, "
        "expected a valid index for set of template type `%s'",
        get_descriptor()->name);
    }
    int param_index = -1;
    sscanf(param_field, "%d", &param_index);
    return get_at(param_index)->get_param(param_name);
  }
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
  case SPECIFIC_VALUE: {
    Vector<Module_Param*> values;
    for (int i = 0; i < single_value.n_elements; ++i) {
      values.push_back(single_value.value_elements[i]->get_param(param_name));
    }
    mp = new Module_Param_Value_List();
    mp->add_list_with_implicit_ids(&values);
    values.clear();
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (int i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i]->get_param(param_name));
    }
    break; }
  default:
    TTCN_error("Referencing an uninitialized/unsupported template of type %s.", get_descriptor()->name);
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  mp->set_length_restriction(get_length_range());
  return mp;
}

boolean Set_Of_Template::get_istemplate_kind(const char* type) const {
  if (!strcmp(type, "AnyElement")) {
    if (template_selection != SPECIFIC_VALUE) {
      return FALSE;
    }
    for (int i = 0; i < single_value.n_elements; i++) {
      if (single_value.value_elements[i]->get_selection() == ANY_VALUE) {
        return TRUE;
      }
    }
    return FALSE;
  } else if (!strcmp(type, "AnyElementsOrNone")) {
    if (template_selection != SPECIFIC_VALUE) {
      return FALSE;
    }
    for (int i = 0; i < single_value.n_elements; i++) {
      if (single_value.value_elements[i]->get_selection() == ANY_OR_OMIT) {
        return TRUE;
      }
    }
    return FALSE;
  } else if (!strcmp(type, "permutation")) {
      return FALSE; // Set of can´t have permutation
  } else if (!strcmp(type, "length")) {
      return length_restriction_type != NO_LENGTH_RESTRICTION;
  } else {
     return Base_Template::get_istemplate_kind(type);
  }
}

void Set_Of_Template::check_restriction(template_res t_res, const char* t_name,
                                        boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return;
  switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {
  case TR_OMIT:
    if (template_selection==OMIT_VALUE) return;
    // no break
  case TR_VALUE:
    if (template_selection!=SPECIFIC_VALUE || is_ifpresent) break;
    for (int i=0; i<single_value.n_elements; i++)
      single_value.value_elements[i]->check_restriction(t_res, t_name ? t_name : get_descriptor()->name);
    return;
  case TR_PRESENT:
    if (!match_omit(legacy)) return;
    break;
  default:
    return;
  }
  TTCN_error("Restriction `%s' on template of type %s violated.",
             get_res_name(t_res), t_name ? t_name : get_descriptor()->name);
}

////////////////////////////////////////////////////////////////////////////////

Record_Template::Record_Template(): Base_Template(), err_descr(NULL)
{
}

Record_Template::Record_Template(template_sel other_value):
  Base_Template(other_value), err_descr(NULL)
{
  check_single_selection(other_value);
}

Record_Template::~Record_Template()
{
  clean_up();
}

void Record_Template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      delete single_value.value_elements[elem_count];
    free_pointers((void**)single_value.value_elements);
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (int elem_count = 0; elem_count < value_list.n_values; elem_count++)
      delete value_list.list_value[elem_count];
    free_pointers((void**)value_list.list_value);
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void Record_Template::copy_value(const Base_Type* other_value)
{
  if (!other_value->is_bound())
    TTCN_error("Initialization of a record/set template with an unbound value.");
  set_specific();
  const Record_Type* other_rec = static_cast<const Record_Type*>(other_value);
  const int* optional_indexes = other_rec->get_optional_indexes();
  int next_optional_idx = 0;
  for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
    boolean is_optional = optional_indexes && (optional_indexes[next_optional_idx]==elem_count);
    if (is_optional) next_optional_idx++;
    const Base_Type* elem_value = other_rec->get_at(elem_count);
    if (!elem_value->is_bound()) continue;
    if (is_optional) {
      if (elem_value->is_present()) {
        single_value.value_elements[elem_count]->copy_value(other_rec->get_at(elem_count)->get_opt_value());
      } else {
        single_value.value_elements[elem_count]->set_value(OMIT_VALUE);
      }
    } else {
      single_value.value_elements[elem_count]->copy_value(other_rec->get_at(elem_count));
    }
  }
  err_descr = other_rec->get_err_descr();
}

void Record_Template::copy_template(const Record_Template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    set_specific(); // FIXME: create_elem(int elem_idx) should be used, but that's more code to generate...
    // single_value.n_elements = other_value.single_value.n_elements;
    // single_value.value_elements = (Base_Template**)allocate_pointers(single_value.n_elements);
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
      if (other_value.single_value.value_elements[elem_count]->is_bound()) {
        delete single_value.value_elements[elem_count]; // FIXME: unbound elements should not be created when the element is bound
        single_value.value_elements[elem_count] = other_value.single_value.value_elements[elem_count]->clone();
      }
    }
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = (Record_Template**)allocate_pointers(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (other_value.value_list.list_value[list_count]->is_bound()) {
        value_list.list_value[list_count] = static_cast<Record_Template*>(other_value.value_list.list_value[list_count]->clone());
      } else {
        value_list.list_value[list_count] = create();
      }
    }
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported record/set template.");
    break;
  }
  set_selection(other_value);
  err_descr = other_value.err_descr;
}

void Record_Template::copy_optional(const Base_Type* other_value)
{
  if (other_value->is_present()) {
    const Record_Type* other_rec = static_cast<const Record_Type*>(other_value->get_opt_value());
    copy_value(other_rec);
  } else if (other_value->is_bound()) {
    set_selection(OMIT_VALUE);
    err_descr = NULL;
  } else {
    TTCN_error("Initialization of a record/set template with an unbound optional field.");
  }
}

void Record_Template::set_type(template_sel template_type, int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list for a template of type %s.", get_descriptor()->name);
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = (Record_Template**)allocate_pointers(value_list.n_values);
  for (int list_count = 0; list_count < value_list.n_values; list_count++)
    value_list.list_value[list_count] = create();
}

Record_Template* Record_Template::get_list_item(int list_index) const
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list template of type %s.", get_descriptor()->name);
  if (list_index < 0)
    TTCN_error("Internal error: Accessing a value list template "
               "of type %s using a negative index (%d).",
               get_descriptor()->name, list_index);
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a value list template of type %s.", get_descriptor()->name);
  return value_list.list_value[list_index];
}

int Record_Template::size_of() const
{
  if (is_ifpresent)
    TTCN_error("Performing sizeof() operation on a template of type %s "
               "which has an ifpresent attribute.", get_descriptor()->name);
  switch (template_selection)
  {
  case SPECIFIC_VALUE: {
    int my_size = 0;
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      if (single_value.value_elements[elem_count]->is_present()) my_size++;
    return my_size;
  }
  case VALUE_LIST: {
    if (value_list.n_values<1)
      TTCN_error("Internal error: Performing sizeof() operation on a template of type %s containing an empty list.", get_descriptor()->name);
    int item_size = value_list.list_value[0]->size_of();
    for (int i = 1; i < value_list.n_values; i++)
      if (value_list.list_value[i]->size_of()!=item_size)
        TTCN_error("Performing sizeof() operation on a template of type %s containing a value list with different sizes.", get_descriptor()->name);
    return item_size;
  }
  case OMIT_VALUE:
    TTCN_error("Performing sizeof() operation on a template of type %s containing omit value.", get_descriptor()->name);
  case ANY_VALUE:
  case ANY_OR_OMIT:
    TTCN_error("Performing sizeof() operation on a template of type %s containing */? value.", get_descriptor()->name);
  case COMPLEMENTED_LIST:
    TTCN_error("Performing sizeof() operation on a template of type %s containing complemented list.", get_descriptor()->name);
  default:
    TTCN_error("Performing sizeof() operation on an uninitialized/unsupported template of type %s.", get_descriptor()->name);
  }
  return 0;
}

boolean Record_Template::is_value() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent) return FALSE;
  for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
      const Base_Template* tmpl_elem = single_value.value_elements[elem_count];
      if (tmpl_elem->get_selection()!=OMIT_VALUE && !tmpl_elem->is_value()) return FALSE;
  }
  return TRUE;
}

void Record_Template::valueofv(Base_Type* value) const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of type %s.", get_descriptor()->name);
  Record_Type* rec_value = static_cast<Record_Type*>(value);
  const int* optional_indexes = rec_value->get_optional_indexes();
  int next_optional_idx = 0;
  for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
    boolean is_optional = optional_indexes && (optional_indexes[next_optional_idx]==elem_count);
    if (is_optional) {
      if (single_value.value_elements[elem_count]->get_selection()==OMIT_VALUE) {
        rec_value->get_at(elem_count)->set_to_omit();
      } else {
        rec_value->get_at(elem_count)->set_to_present(); // create instance of optional value object
        single_value.value_elements[elem_count]->valueofv(rec_value->get_at(elem_count)->get_opt_value());
      }
    } else {
      single_value.value_elements[elem_count]->valueofv(rec_value->get_at(elem_count));
    }
    if (is_optional) next_optional_idx++;
  }
  rec_value->set_err_descr(err_descr);
}

void Record_Template::set_value(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  err_descr = NULL;
}

Base_Template* Record_Template::get_at(int index_value)
{
  set_specific();
  if (index_value < 0 || index_value >= single_value.n_elements)
    TTCN_error("Internal error: accessing an element of a template of type %s using an "
      "invalid index: %d.", get_descriptor()->name, index_value);
  return single_value.value_elements[index_value];
}

const Base_Template* Record_Template::get_at(int index_value) const
{
  if (index_value < 0 || index_value >= single_value.n_elements)
    TTCN_error("Internal error: accessing an element of a template of type %s using an "
      "invalid index: %d.", get_descriptor()->name, index_value);
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field %s of a non-specific template of type %s.",
               fld_name(index_value), get_descriptor()->name);
  return single_value.value_elements[index_value];
}

Base_Template* Record_Template::clone() const
{
  Record_Template* rec = create();
  rec->copy_template(*this);
  return rec;
}

void Record_Template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    if (single_value.n_elements > 0) {
      TTCN_Logger::log_event_str("{ ");
      for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
        if (elem_count > 0) TTCN_Logger::log_event_str(", ");
        TTCN_Logger::log_event_str(fld_name(elem_count));
        TTCN_Logger::log_event_str(" := ");
        single_value.value_elements[elem_count]->log();
      }
      TTCN_Logger::log_event_str(" }");
    } else TTCN_Logger::log_event_str("{ }");
  break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count]->log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
  if (err_descr) err_descr->log();
}

boolean Record_Template::matchv(const Base_Type* other_value,
                                boolean legacy) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE: {
    const Record_Type* other_rec = static_cast<const Record_Type*>(other_value);
    const int* optional_indexes = other_rec->get_optional_indexes();
    int next_optional_idx = 0;
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
      boolean is_optional = optional_indexes && (optional_indexes[next_optional_idx]==elem_count);
      const Base_Template* elem_tmpl = single_value.value_elements[elem_count];
      const Base_Type* elem_value = other_rec->get_at(elem_count);
      if (!elem_value->is_bound()) return FALSE;
      boolean elem_match = is_optional ?
        ( elem_value->ispresent() ? elem_tmpl->matchv(elem_value->get_opt_value(), legacy) : elem_tmpl->match_omit(legacy) ) :
        elem_tmpl->matchv(other_rec->get_at(elem_count), legacy);
      if (!elem_match) return FALSE;
      if (is_optional) next_optional_idx++;
    }
  } return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count]->matchv(other_value, legacy)) return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching an uninitialized/unsupported template of type %s.", get_descriptor()->name);
  }
  return FALSE;
}

void Record_Template::log_matchv(const Base_Type* match_value, boolean legacy) const
{
  if (TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()) {
    if (matchv(match_value, legacy)) {
      TTCN_Logger::print_logmatch_buffer();
      TTCN_Logger::log_event_str(" matched");
    } else {
      if (template_selection == SPECIFIC_VALUE) {
        size_t previous_size = TTCN_Logger::get_logmatch_buffer_len();
        const Record_Type* match_rec = static_cast<const Record_Type*>(match_value);
        const int* optional_indexes = match_rec->get_optional_indexes();
        int next_optional_idx = 0;
        for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
          boolean is_optional = optional_indexes && (optional_indexes[next_optional_idx]==elem_count);
          const Base_Template* elem_tmpl = single_value.value_elements[elem_count];
          const Base_Type* elem_value = match_rec->get_at(elem_count);
          if (is_optional) {
            if (elem_value->ispresent()) {
              if (!elem_tmpl->matchv(elem_value->get_opt_value(), legacy)) {
                TTCN_Logger::log_logmatch_info(".%s", fld_name(elem_count));
                elem_tmpl->log_matchv(elem_value->get_opt_value(), legacy);
                TTCN_Logger::set_logmatch_buffer_len(previous_size);
              }
            } else {
              if (!elem_tmpl->match_omit(legacy)) {
                TTCN_Logger::log_logmatch_info(".%s := omit with ", fld_name(elem_count));
                TTCN_Logger::print_logmatch_buffer();
                elem_tmpl->log();
                TTCN_Logger::log_event_str(" unmatched");
                TTCN_Logger::set_logmatch_buffer_len(previous_size);
              }
            }
          } else {//mandatory
            if (!elem_tmpl->matchv(elem_value, legacy)) {
              TTCN_Logger::log_logmatch_info(".%s", fld_name(elem_count));
              elem_tmpl->log_matchv(elem_value, legacy);
              TTCN_Logger::set_logmatch_buffer_len(previous_size);
            }
          }//if
          if (is_optional) next_optional_idx++;
        }//for
      } else {
        TTCN_Logger::print_logmatch_buffer();
        match_value->log();
        TTCN_Logger::log_event_str(" with ");
        log();
        TTCN_Logger::log_event_str(" unmatched");
      }
    }
  } else {
    if (template_selection == SPECIFIC_VALUE) {
      const Record_Type* match_rec = static_cast<const Record_Type*>(match_value);
      const int* optional_indexes = match_rec->get_optional_indexes();
      int next_optional_idx = 0;
      TTCN_Logger::log_event_str("{ ");
      for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++) {
        boolean is_optional = optional_indexes && (optional_indexes[next_optional_idx]==elem_count);
        const Base_Template* elem_tmpl = single_value.value_elements[elem_count];
        const Base_Type* elem_value = match_rec->get_at(elem_count);
        if (elem_count) TTCN_Logger::log_event_str(", ");
        TTCN_Logger::log_event_str(fld_name(elem_count));
        TTCN_Logger::log_event_str(" := ");
        if (is_optional) {
          if (elem_value->ispresent()) elem_tmpl->log_matchv(elem_value->get_opt_value(), legacy);
          else {
            TTCN_Logger::log_event_str("omit with ");
            elem_tmpl->log();
            if (elem_tmpl->match_omit(legacy)) TTCN_Logger::log_event_str(" matched");
            else TTCN_Logger::log_event_str(" unmatched");
          }
        } else {
          elem_tmpl->log_matchv(elem_value, legacy);
        }
        if (is_optional) next_optional_idx++;
      }
      TTCN_Logger::log_event_str(" }");
    } else {
      match_value->log();
      TTCN_Logger::log_event_str(" with ");
      log();
      if (matchv(match_value, legacy)) TTCN_Logger::log_event_str(" matched");
      else TTCN_Logger::log_event_str(" unmatched");
    }
  }
}

void Record_Template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      single_value.value_elements[elem_count]->encode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count]->encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported template"
      " of type %s.", get_descriptor()->name);
  }
}

void Record_Template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    template_selection = UNINITIALIZED_TEMPLATE; //set_specific will set it
    set_specific();
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      single_value.value_elements[elem_count]->decode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = (Record_Template**)allocate_pointers(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++) {
      value_list.list_value[list_count] = create();
      value_list.list_value[list_count]->decode_text(text_buf);
    }
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received in a template of type %s.", get_descriptor()->name);
  }
}

boolean Record_Template::is_present(boolean legacy /*= FALSE*/) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean Record_Template::match_omit(boolean legacy /*= FALSE*/) const
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
      for (int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i]->match_omit()) return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    }
    // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

void Record_Template::set_param(Module_Param& param)
{
  if (dynamic_cast<Module_Param_Name*>(param.get_id()) != NULL &&
      param.get_id()->next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the fields, not to the whole record
    char* param_field = param.get_id()->get_current_name();
    if (param_field[0] >= '0' && param_field[0] <= '9') {
      param.error("Unexpected array index in module parameter, expected a valid field"
        " name for record/set template type `%s'", get_descriptor()->name);
    }
    set_specific();
    for (int field_idx = 0; field_idx < single_value.n_elements; field_idx++) {
      if (strcmp(fld_name(field_idx), param_field) == 0) {
        single_value.value_elements[field_idx]->set_param(param);
        return;
      }
    }
    param.error("Field `%s' not found in record/set template type `%s'",
      param_field, get_descriptor()->name);
  }
  
  param.basic_check(Module_Param::BC_TEMPLATE, "record/set template");
  
  Module_Param_Ptr mp = &param;
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
  
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    set_value(OMIT_VALUE);
    break;
  case Module_Param::MP_Any:
    set_value(ANY_VALUE);
    break;
  case Module_Param::MP_AnyOrNone:
    set_value(ANY_OR_OMIT);
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    Record_Template** list_items = (Record_Template**)
      allocate_pointers(mp->get_size());
    for (size_t i = 0; i < mp->get_size(); i++) {
      list_items[i] = create();
      list_items[i]->set_param(*mp->get_elem(i));
    }
    clean_up();
    template_selection = mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST;
    value_list.n_values = mp->get_size();
    value_list.list_value = list_items;
    break; }
  case Module_Param::MP_Value_List:
    set_specific();
    if (single_value.n_elements<(int)mp->get_size()) {
      param.error("Record/set template of type %s has %d fields but list value has %d fields", get_descriptor()->name, single_value.n_elements, (int)mp->get_size());
    }
    for (size_t i=0; i<mp->get_size(); i++) {
      Module_Param* mp_field = mp->get_elem(i);
      if (mp_field->get_type()!=Module_Param::MP_NotUsed) {
        get_at((int)i)->set_param(*mp_field);
      }
    }
    break;
  case Module_Param::MP_Assignment_List:
    set_specific();
    for (size_t i=0; i<mp->get_size(); ++i) {
      Module_Param* const current = mp->get_elem(i);
      boolean found = FALSE;
      for (int j=0; j<single_value.n_elements; ++j) {
        if (!strcmp(fld_name(j), current->get_id()->get_name())) {
          if (current->get_type()!=Module_Param::MP_NotUsed) {
            get_at(j)->set_param(*current);
          }
          found = TRUE;
          break;
        }
      }
      if (!found) {
        current->error("Non existent field name in type %s: %s.", get_descriptor()->name, current->get_id()->get_name());
      }
    }
    break;
  default:
    param.type_error("record/set template", get_descriptor()->name);
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

Module_Param* Record_Template::get_param(Module_Param_Name& param_name) const
{
  if (param_name.next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the fields, not to the whole record
    char* param_field = param_name.get_current_name();
    if (param_field[0] >= '0' && param_field[0] <= '9') {
      TTCN_error("Unexpected array index in module parameter reference, "
        "expected a valid field name for record/set template type `%s'",
        get_descriptor()->name);
    }
    for (int field_idx = 0; field_idx < single_value.n_elements; field_idx++) {
      if (strcmp(fld_name(field_idx), param_field) == 0) {
        return get_at(field_idx)->get_param(param_name);
      }
    }
    TTCN_error("Field `%s' not found in record/set type `%s'",
      param_field, get_descriptor()->name);
  }
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
  case SPECIFIC_VALUE: {
    mp = new Module_Param_Assignment_List();
    for (int i = 0; i < single_value.n_elements; ++i) {
      Module_Param* mp_field = get_at(i)->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr(fld_name(i))));
      mp->add_elem(mp_field);
    }
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (int i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i]->get_param(param_name));
    }
    break; }
  default:
    TTCN_error("Referencing an uninitialized/unsupported template of type %s.", get_descriptor()->name);
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}

void Record_Template::check_restriction(template_res t_res, const char* t_name,
                                        boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return;
  switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {
  case TR_OMIT:
    if (template_selection==OMIT_VALUE) return;
    // no break
  case TR_VALUE:
    if (template_selection!=SPECIFIC_VALUE || is_ifpresent) break;
    for (int i=0; i<single_value.n_elements; i++)
      single_value.value_elements[i]->check_restriction(t_res, t_name ? t_name : get_descriptor()->name);
    return;
  case TR_PRESENT:
    if (!match_omit(legacy)) return;
    break;
  default:
    return;
  }
  TTCN_error("Restriction `%s' on template of type %s violated.",
             get_res_name(t_res), t_name ? t_name : get_descriptor()->name);
}

////////////////////////////////////////////////////////////////////////////////

Empty_Record_Template::Empty_Record_Template(): Base_Template()
{
}

Empty_Record_Template::Empty_Record_Template(template_sel other_value):
  Base_Template(other_value)
{
  check_single_selection(other_value);
}

Empty_Record_Template::~Empty_Record_Template()
{
  clean_up();
}

void Empty_Record_Template::clean_up()
{
  switch (template_selection) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (int elem_count = 0; elem_count < value_list.n_values; elem_count++)
      delete value_list.list_value[elem_count];
    free_pointers((void**)value_list.list_value);
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void Empty_Record_Template::copy_value(const Base_Type* other_value)
{
  if (!other_value->is_bound())
    TTCN_error("Initialization of a record/set template with an unbound value.");
  set_selection(SPECIFIC_VALUE);
}

void Empty_Record_Template::copy_template(const Empty_Record_Template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = (Empty_Record_Template**)allocate_pointers(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count] = static_cast<Empty_Record_Template*>(other_value.value_list.list_value[list_count]->clone());
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported record/set template.");
    break;
  }
  set_selection(other_value);
}

void Empty_Record_Template::copy_optional(const Base_Type* other_value)
{
  if (other_value->is_present()) {
    const Empty_Record_Type* other_rec = static_cast<const Empty_Record_Type*>(other_value->get_opt_value());
    copy_value(other_rec);
  } else if (other_value->is_bound()) {
    set_selection(OMIT_VALUE);
  } else {
    TTCN_error("Initialization of a record/set template with an unbound optional field.");
  }
}

void Empty_Record_Template::set_type(template_sel template_type, int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list for a template of type %s.", get_descriptor()->name);
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = (Empty_Record_Template**)allocate_pointers(value_list.n_values);
  for (int list_count = 0; list_count < value_list.n_values; list_count++)
    value_list.list_value[list_count] = create();
}

Empty_Record_Template* Empty_Record_Template::get_list_item(int list_index) const
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list template of type %s.", get_descriptor()->name);
  if (list_index < 0)
    TTCN_error("Internal error: Accessing a value list template "
               "of type %s using a negative index (%d).",
               get_descriptor()->name, list_index);
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a value list template of type %s.", get_descriptor()->name);
  return value_list.list_value[list_index];
}

int Empty_Record_Template::size_of() const
{
  if (is_ifpresent)
    TTCN_error("Performing sizeof() operation on a template of type %s "
               "which has an ifpresent attribute.", get_descriptor()->name);
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
    return 0;
  case VALUE_LIST: {
    if (value_list.n_values<1)
      TTCN_error("Internal error: Performing sizeof() operation on a template of type %s containing an empty list.", get_descriptor()->name);
    int item_size = value_list.list_value[0]->size_of();
    for (int i = 1; i < value_list.n_values; i++)
      if (value_list.list_value[i]->size_of()!=item_size)
        TTCN_error("Performing sizeof() operation on a template of type %s containing a value list with different sizes.", get_descriptor()->name);
    return item_size;
  }
  case OMIT_VALUE:
    TTCN_error("Performing sizeof() operation on a template of type %s containing omit value.", get_descriptor()->name);
  case ANY_VALUE:
  case ANY_OR_OMIT:
    TTCN_error("Performing sizeof() operation on a template of type %s containing */? value.", get_descriptor()->name);
  case COMPLEMENTED_LIST:
    TTCN_error("Performing sizeof() operation on a template of type %s containing complemented list.", get_descriptor()->name);
  default:
    TTCN_error("Performing sizeof() operation on an uninitialized/unsupported template of type %s.", get_descriptor()->name);
  }
  return 0;
}

boolean Empty_Record_Template::is_value() const
{
  return (template_selection == SPECIFIC_VALUE && !is_ifpresent);
}

void Empty_Record_Template::valueofv(Base_Type* value) const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of type %s.", get_descriptor()->name);
  Empty_Record_Type* rec_value = static_cast<Empty_Record_Type*>(value);
  rec_value->set_null();
}

void Empty_Record_Template::set_value(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
}

Base_Template* Empty_Record_Template::clone() const
{
  Empty_Record_Template* rec = create();
  rec->copy_template(*this);
  return rec;
}

void Empty_Record_Template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    TTCN_Logger::log_event_str("{ }");
  break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count]->log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

boolean Empty_Record_Template::matchv(const Base_Type* other_value,
                                      boolean legacy) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count]->matchv(other_value, legacy)) return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching an uninitialized/unsupported template of type %s.", get_descriptor()->name);
  }
  return FALSE;
}

void Empty_Record_Template::log_matchv(const Base_Type* match_value, boolean legacy) const
{
  match_value->log();
  TTCN_Logger::log_event_str(" with ");
  log();
  if (matchv(match_value, legacy)) TTCN_Logger::log_event_str(" matched");
  else TTCN_Logger::log_event_str(" unmatched");
}

void Empty_Record_Template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count]->encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported template of type %s.", get_descriptor()->name);
  }
}

void Empty_Record_Template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = (Empty_Record_Template**)allocate_pointers(value_list.n_values);
    for (int list_count = 0; list_count < value_list.n_values; list_count++) {
      value_list.list_value[list_count] = create();
      value_list.list_value[list_count]->decode_text(text_buf);
    }
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received in a template of type %s.", get_descriptor()->name);
  }
}

boolean Empty_Record_Template::is_present(boolean legacy /*= FALSE*/) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean Empty_Record_Template::match_omit(boolean legacy /* = FALSE */) const
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
      for (int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i]->match_omit()) return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    }
    // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

void Empty_Record_Template::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_TEMPLATE, "empty record/set template");
  Module_Param_Ptr mp = &param;
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    set_value(OMIT_VALUE);
    break;
  case Module_Param::MP_Any:
    set_value(ANY_VALUE);
    break;
  case Module_Param::MP_AnyOrNone:
    set_value(ANY_OR_OMIT);
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    Empty_Record_Template** list_items = (Empty_Record_Template**)
      allocate_pointers(mp->get_size());
    for (size_t i = 0; i < mp->get_size(); i++) {
      list_items[i] = create();
      list_items[i]->set_param(*mp->get_elem(i));
    }
    clean_up();
    template_selection = mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST;
    value_list.n_values = mp->get_size();
    value_list.list_value = list_items;
    break; }
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) {
      set_selection(SPECIFIC_VALUE);
    }
    else param.type_error("empty record/set template", get_descriptor()->name);
    break;
  default:
    param.type_error("empty record/set template", get_descriptor()->name);
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

Module_Param* Empty_Record_Template::get_param(Module_Param_Name& param_name) const
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
    mp = new Module_Param_Value_List();
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (int i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i]->get_param(param_name));
    }
    break; }
  default:
    TTCN_error("Referencing an uninitialized/unsupported template of type %s.", get_descriptor()->name);
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}

#endif
