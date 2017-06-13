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
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include <string.h>

#include "Error.hh"
#include "Logger.hh"
#include "Encdec.hh"
#include "BER.hh"
#include "Param_Types.hh"

#include "ASN_CharacterString.hh"

#include "../common/dbgnew.hh"

/*

to do when regenerating:

in .hh file:

add __SUNPRO_CC ifdefs for single_value_struct

in .cc file:

replace '@CHARACTER STRING' with 'CHARACTER STRING'

remove RAW and TEXT enc/dec functions

make the type descriptors of embedded types static

*/

static const ASN_Tag_t CHARACTER_STRING_identification_tag_[] = { { ASN_TAG_CONT, 0u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_identification_ber_ = { 1u, CHARACTER_STRING_identification_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_identification_descr_ = { "CHARACTER STRING.identification", &CHARACTER_STRING_identification_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

static const ASN_Tag_t CHARACTER_STRING_identification_syntaxes_abstract_tag_[] = { { ASN_TAG_CONT, 0u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_identification_syntaxes_abstract_ber_ = { 1u, CHARACTER_STRING_identification_syntaxes_abstract_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_identification_syntaxes_abstract_descr_ = { "CHARACTER STRING.identification.syntaxes.abstract", &CHARACTER_STRING_identification_syntaxes_abstract_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::OBJID };

static const ASN_Tag_t CHARACTER_STRING_identification_syntaxes_transfer_tag_[] = { { ASN_TAG_CONT, 1u }};
const ASN_BERdescriptor_t CHARACTER_STRING_identification_syntaxes_transfer_ber_ = { 1u, CHARACTER_STRING_identification_syntaxes_transfer_tag_ };
const TTCN_Typedescriptor_t CHARACTER_STRING_identification_syntaxes_transfer_descr_ = { "CHARACTER STRING.identification.syntaxes.transfer", &CHARACTER_STRING_identification_syntaxes_transfer_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::OBJID };

static const ASN_Tag_t CHARACTER_STRING_identification_syntaxes_tag_[] = { { ASN_TAG_CONT, 0u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_identification_syntaxes_ber_ = { 1u, CHARACTER_STRING_identification_syntaxes_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_identification_syntaxes_descr_ = { "CHARACTER STRING.identification.syntaxes", &CHARACTER_STRING_identification_syntaxes_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

static const ASN_Tag_t CHARACTER_STRING_identification_syntax_tag_[] = { { ASN_TAG_CONT, 1u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_identification_syntax_ber_ = { 1u, CHARACTER_STRING_identification_syntax_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_identification_syntax_descr_ = { "CHARACTER STRING.identification.syntax", &CHARACTER_STRING_identification_syntax_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::OBJID };

static const ASN_Tag_t CHARACTER_STRING_identification_presentation__context__id_tag_[] = { { ASN_TAG_CONT, 2u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_identification_presentation__context__id_ber_ = { 1u, CHARACTER_STRING_identification_presentation__context__id_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_identification_presentation__context__id_descr_ = { "CHARACTER STRING.identification.presentation-context-id", &CHARACTER_STRING_identification_presentation__context__id_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

static const ASN_Tag_t CHARACTER_STRING_identification_context__negotiation_presentation__context__id_tag_[] = { { ASN_TAG_CONT, 0u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_identification_context__negotiation_presentation__context__id_ber_ = { 1u, CHARACTER_STRING_identification_context__negotiation_presentation__context__id_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_identification_context__negotiation_presentation__context__id_descr_ = { "CHARACTER STRING.identification.context-negotiation.presentation-context-id", &CHARACTER_STRING_identification_context__negotiation_presentation__context__id_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

static const ASN_Tag_t CHARACTER_STRING_identification_context__negotiation_transfer__syntax_tag_[] = { { ASN_TAG_CONT, 1u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_identification_context__negotiation_transfer__syntax_ber_ = { 1u, CHARACTER_STRING_identification_context__negotiation_transfer__syntax_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_identification_context__negotiation_transfer__syntax_descr_ = { "CHARACTER STRING.identification.context-negotiation.transfer-syntax", &CHARACTER_STRING_identification_context__negotiation_transfer__syntax_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::OBJID };

static const ASN_Tag_t CHARACTER_STRING_identification_context__negotiation_tag_[] = { { ASN_TAG_CONT, 3u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_identification_context__negotiation_ber_ = { 1u, CHARACTER_STRING_identification_context__negotiation_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_identification_context__negotiation_descr_ = { "CHARACTER STRING.identification.context-negotiation", &CHARACTER_STRING_identification_context__negotiation_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

static const ASN_Tag_t CHARACTER_STRING_identification_transfer__syntax_tag_[] = { { ASN_TAG_CONT, 4u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_identification_transfer__syntax_ber_ = { 1u, CHARACTER_STRING_identification_transfer__syntax_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_identification_transfer__syntax_descr_ = { "CHARACTER STRING.identification.transfer-syntax", &CHARACTER_STRING_identification_transfer__syntax_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::OBJID };

static const ASN_Tag_t CHARACTER_STRING_identification_fixed_tag_[] = { { ASN_TAG_CONT, 5u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_identification_fixed_ber_ = { 1u, CHARACTER_STRING_identification_fixed_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_identification_fixed_descr_ = { "CHARACTER STRING.identification.fixed", &CHARACTER_STRING_identification_fixed_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

static const ASN_Tag_t CHARACTER_STRING_string__value_tag_[] = { { ASN_TAG_CONT, 2u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_string__value_ber_ = { 1u, CHARACTER_STRING_string__value_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_string__value_descr_ = { "CHARACTER STRING.string-value", &CHARACTER_STRING_string__value_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

static const ASN_Tag_t CHARACTER_STRING_data__value__descriptor_tag_[] = { { ASN_TAG_CONT, 1u }};
static const ASN_BERdescriptor_t CHARACTER_STRING_data__value__descriptor_ber_ = { 1u, CHARACTER_STRING_data__value__descriptor_tag_ };
static const TTCN_Typedescriptor_t CHARACTER_STRING_data__value__descriptor_descr_ = { "CHARACTER STRING.data-value-descriptor", &CHARACTER_STRING_data__value__descriptor_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::GRAPHICSTRING };

void CHARACTER_STRING_identification::clean_up()
{
  switch (union_selection) {
  case ALT_syntaxes:
    delete field_syntaxes;
    break;
  case ALT_syntax:
    delete field_syntax;
    break;
  case ALT_presentation__context__id:
    delete field_presentation__context__id;
    break;
  case ALT_context__negotiation:
    delete field_context__negotiation;
    break;
  case ALT_transfer__syntax:
    delete field_transfer__syntax;
    break;
  case ALT_fixed:
    delete field_fixed;
    break;
  default:
    break;
  }
  union_selection = UNBOUND_VALUE;
}

void CHARACTER_STRING_identification::copy_value(const CHARACTER_STRING_identification& other_value)
{
  switch (other_value.union_selection) {
  case ALT_syntaxes:
    field_syntaxes = new CHARACTER_STRING_identification_syntaxes(*other_value.field_syntaxes);
    break;
  case ALT_syntax:
    field_syntax = new OBJID(*other_value.field_syntax);
    break;
  case ALT_presentation__context__id:
    field_presentation__context__id = new INTEGER(*other_value.field_presentation__context__id);
    break;
  case ALT_context__negotiation:
    field_context__negotiation = new CHARACTER_STRING_identification_context__negotiation(*other_value.field_context__negotiation);
    break;
  case ALT_transfer__syntax:
    field_transfer__syntax = new OBJID(*other_value.field_transfer__syntax);
    break;
  case ALT_fixed:
    field_fixed = new ASN_NULL(*other_value.field_fixed);
    break;
  default:
    TTCN_error("Assignment of an unbound union value of type CHARACTER STRING.identification.");
  }
  union_selection = other_value.union_selection;
}

CHARACTER_STRING_identification::CHARACTER_STRING_identification()
{
  union_selection = UNBOUND_VALUE;
}

CHARACTER_STRING_identification::CHARACTER_STRING_identification(const CHARACTER_STRING_identification& other_value)
: Base_Type(other_value)
{
  copy_value(other_value);
}

CHARACTER_STRING_identification::~CHARACTER_STRING_identification()
{
  clean_up();
}

CHARACTER_STRING_identification& CHARACTER_STRING_identification::operator=(const CHARACTER_STRING_identification& other_value)
{
  if (this != &other_value) {
    clean_up();
    copy_value(other_value);
  }
  return *this;
}

boolean CHARACTER_STRING_identification::operator==(const CHARACTER_STRING_identification& other_value) const
{
  if (union_selection == UNBOUND_VALUE) TTCN_error("The left operand of comparison is an unbound value of union type CHARACTER STRING.identification.");
  if (other_value.union_selection == UNBOUND_VALUE) TTCN_error("The right operand of comparison is an unbound value of union type CHARACTER STRING.identification.");
  if (union_selection != other_value.union_selection) return FALSE;
  switch (union_selection) {
  case ALT_syntaxes:
    return *field_syntaxes == *other_value.field_syntaxes;
  case ALT_syntax:
    return *field_syntax == *other_value.field_syntax;
  case ALT_presentation__context__id:
    return *field_presentation__context__id == *other_value.field_presentation__context__id;
  case ALT_context__negotiation:
    return *field_context__negotiation == *other_value.field_context__negotiation;
  case ALT_transfer__syntax:
    return *field_transfer__syntax == *other_value.field_transfer__syntax;
  case ALT_fixed:
    return *field_fixed == *other_value.field_fixed;
  default:
    return FALSE;
  }
}

CHARACTER_STRING_identification_syntaxes& CHARACTER_STRING_identification::syntaxes()
{
  if (union_selection != ALT_syntaxes) {
    clean_up();
    field_syntaxes = new CHARACTER_STRING_identification_syntaxes;
    union_selection = ALT_syntaxes;
  }
  return *field_syntaxes;
}

const CHARACTER_STRING_identification_syntaxes& CHARACTER_STRING_identification::syntaxes() const
{
  if (union_selection != ALT_syntaxes) TTCN_error("Using non-selected field syntaxes in a value of union type CHARACTER STRING.identification.");
  return *field_syntaxes;
}

OBJID& CHARACTER_STRING_identification::syntax()
{
  if (union_selection != ALT_syntax) {
    clean_up();
    field_syntax = new OBJID;
    union_selection = ALT_syntax;
  }
  return *field_syntax;
}

const OBJID& CHARACTER_STRING_identification::syntax() const
{
  if (union_selection != ALT_syntax) TTCN_error("Using non-selected field syntax in a value of union type CHARACTER STRING.identification.");
  return *field_syntax;
}

INTEGER& CHARACTER_STRING_identification::presentation__context__id()
{
  if (union_selection != ALT_presentation__context__id) {
    clean_up();
    field_presentation__context__id = new INTEGER;
    union_selection = ALT_presentation__context__id;
  }
  return *field_presentation__context__id;
}

const INTEGER& CHARACTER_STRING_identification::presentation__context__id() const
{
  if (union_selection != ALT_presentation__context__id) TTCN_error("Using non-selected field presentation_context_id in a value of union type CHARACTER STRING.identification.");
  return *field_presentation__context__id;
}

CHARACTER_STRING_identification_context__negotiation& CHARACTER_STRING_identification::context__negotiation()
{
  if (union_selection != ALT_context__negotiation) {
    clean_up();
    field_context__negotiation = new CHARACTER_STRING_identification_context__negotiation;
    union_selection = ALT_context__negotiation;
  }
  return *field_context__negotiation;
}

const CHARACTER_STRING_identification_context__negotiation& CHARACTER_STRING_identification::context__negotiation() const
{
  if (union_selection != ALT_context__negotiation) TTCN_error("Using non-selected field context_negotiation in a value of union type CHARACTER STRING.identification.");
  return *field_context__negotiation;
}

OBJID& CHARACTER_STRING_identification::transfer__syntax()
{
  if (union_selection != ALT_transfer__syntax) {
    clean_up();
    field_transfer__syntax = new OBJID;
    union_selection = ALT_transfer__syntax;
  }
  return *field_transfer__syntax;
}

const OBJID& CHARACTER_STRING_identification::transfer__syntax() const
{
  if (union_selection != ALT_transfer__syntax) TTCN_error("Using non-selected field transfer_syntax in a value of union type CHARACTER STRING.identification.");
  return *field_transfer__syntax;
}

ASN_NULL& CHARACTER_STRING_identification::fixed()
{
  if (union_selection != ALT_fixed) {
    clean_up();
    field_fixed = new ASN_NULL;
    union_selection = ALT_fixed;
  }
  return *field_fixed;
}

const ASN_NULL& CHARACTER_STRING_identification::fixed() const
{
  if (union_selection != ALT_fixed) TTCN_error("Using non-selected field fixed in a value of union type CHARACTER STRING.identification.");
  return *field_fixed;
}

boolean CHARACTER_STRING_identification::ischosen(union_selection_type checked_selection) const
{
  if (checked_selection == UNBOUND_VALUE) TTCN_error("Internal error: Performing ischosen() operation on an invalid field of union type CHARACTER STRING.identification.");
  if (union_selection == UNBOUND_VALUE) TTCN_error("Internal error: Performing ischosen() operation on an unbound value of union type CHARACTER STRING.identification.");
  return union_selection == checked_selection;
}

boolean CHARACTER_STRING_identification::is_bound() const
{
  switch (union_selection) {
  case ALT_syntaxes:
    return field_syntaxes->is_bound();
  case ALT_syntax:
    return field_syntax->is_bound();
  case ALT_presentation__context__id:
    return field_presentation__context__id->is_bound();
  case ALT_context__negotiation:
    return field_context__negotiation->is_bound();
  case ALT_transfer__syntax:
    return field_transfer__syntax->is_bound();
  case ALT_fixed:
    return field_fixed->is_bound();
  default:
    return FALSE;
  }
}

boolean CHARACTER_STRING_identification::is_value() const
{
  switch (union_selection) {
  case ALT_syntaxes:
    return field_syntaxes->is_value();
  case ALT_syntax:
    return field_syntax->is_value();
  case ALT_presentation__context__id:
    return field_presentation__context__id->is_value();
  case ALT_context__negotiation:
    return field_context__negotiation->is_value();
  case ALT_transfer__syntax:
    return field_transfer__syntax->is_value();
  case ALT_fixed:
    return field_fixed->is_value();
  default:
    return FALSE;
  }
}

void CHARACTER_STRING_identification::log() const
{
  switch (union_selection) {
  case ALT_syntaxes:
    TTCN_Logger::log_event_str("{ syntaxes := ");
    field_syntaxes->log();
    TTCN_Logger::log_event_str(" }");
    break;
  case ALT_syntax:
    TTCN_Logger::log_event_str("{ syntax := ");
    field_syntax->log();
    TTCN_Logger::log_event_str(" }");
    break;
  case ALT_presentation__context__id:
    TTCN_Logger::log_event_str("{ presentation_context_id := ");
    field_presentation__context__id->log();
    TTCN_Logger::log_event_str(" }");
    break;
  case ALT_context__negotiation:
    TTCN_Logger::log_event_str("{ context_negotiation := ");
    field_context__negotiation->log();
    TTCN_Logger::log_event_str(" }");
    break;
  case ALT_transfer__syntax:
    TTCN_Logger::log_event_str("{ transfer_syntax := ");
    field_transfer__syntax->log();
    TTCN_Logger::log_event_str(" }");
    break;
  case ALT_fixed:
    TTCN_Logger::log_event_str("{ fixed := ");
    field_fixed->log();
    TTCN_Logger::log_event_str(" }");
    break;
  default:
    TTCN_Logger::log_event_str("<unbound>");
    break;
  }
}

void CHARACTER_STRING_identification::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_VALUE, "union value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  if (mp->get_type()==Module_Param::MP_Value_List && mp->get_size()==0) return;
  if (mp->get_type()!=Module_Param::MP_Assignment_List) {
    param.error("union value with field name was expected");
  }
  Module_Param* mp_last = mp->get_elem(mp->get_size()-1);
  if (!strcmp(mp_last->get_id()->get_name(), "syntaxes")) {
    syntaxes().set_param(*mp_last);
    return;
  }
  if (!strcmp(mp_last->get_id()->get_name(), "syntax")) {
    syntax().set_param(*mp_last);
    return;
  }
  if (!strcmp(mp_last->get_id()->get_name(), "presentation_context_id")) {
    presentation__context__id().set_param(*mp_last);
    return;
  }
  if (!strcmp(mp_last->get_id()->get_name(), "context_negotiation")) {
    context__negotiation().set_param(*mp_last);
    return;
  }
  if (!strcmp(mp_last->get_id()->get_name(), "transfer_syntax")) {
    transfer__syntax().set_param(*mp_last);
    return;
  }
  if (!strcmp(mp_last->get_id()->get_name(), "fixed")) {
    fixed().set_param(*mp_last);
    return;
  }
  mp_last->error("Field %s does not exist in type CHARACTER STRING.identification.", mp_last->get_id()->get_name());
}

#ifdef TITAN_RUNTIME_2
Module_Param* CHARACTER_STRING_identification::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  Module_Param* mp_field = NULL;
  switch(get_selection()) {
  case ALT_syntaxes:
    mp_field = field_syntaxes->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("syntaxes")));
    break;
  case ALT_syntax:
    mp_field = field_syntax->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("syntax")));
    break;
  case ALT_presentation__context__id:
    mp_field = field_presentation__context__id->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("presentation_context_id")));
    break;
  case ALT_context__negotiation:
    mp_field = field_context__negotiation->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("context_negotiation")));
    break;
  case ALT_transfer__syntax:
    mp_field = field_transfer__syntax->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("transfer_syntax")));
    break;
  case ALT_fixed:
    mp_field = field_fixed->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("fixed")));
    break;
  default:
    break;
  }
  Module_Param_Assignment_List* mp = new Module_Param_Assignment_List();
  mp->add_elem(mp_field);
  return mp;
}
#endif

void CHARACTER_STRING_identification_template::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_TEMPLATE, "union template");
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
    CHARACTER_STRING_identification_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) break;
    param.type_error("union template", "CHARACTER STRING.identification");
    break;
  case Module_Param::MP_Assignment_List: {
    Module_Param* mp_last = mp->get_elem(mp->get_size()-1);
    if (!strcmp(mp_last->get_id()->get_name(), "syntaxes")) {
      syntaxes().set_param(*mp_last);
      break;
    }
    if (!strcmp(mp_last->get_id()->get_name(), "syntax")) {
      syntax().set_param(*mp_last);
      break;
    }
    if (!strcmp(mp_last->get_id()->get_name(), "presentation_context_id")) {
      presentation__context__id().set_param(*mp_last);
      break;
    }
    if (!strcmp(mp_last->get_id()->get_name(), "context_negotiation")) {
      context__negotiation().set_param(*mp_last);
      break;
    }
    if (!strcmp(mp_last->get_id()->get_name(), "transfer_syntax")) {
      transfer__syntax().set_param(*mp_last);
      break;
    }
    if (!strcmp(mp_last->get_id()->get_name(), "fixed")) {
      fixed().set_param(*mp_last);
      break;
    }
    mp_last->error("Field %s does not exist in type CHARACTER STRING.identification.", mp_last->get_id()->get_name());
  } break;
  default:
    param.type_error("union template", "CHARACTER STRING.identification");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* CHARACTER_STRING_identification_template::get_param(Module_Param_Name& param_name) const
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
  case SPECIFIC_VALUE: {
    Module_Param* mp_field = NULL;
    switch(single_value.union_selection) {
    case CHARACTER_STRING_identification::ALT_syntaxes:
      mp_field = single_value.field_syntaxes->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("syntaxes")));
      break;
    case CHARACTER_STRING_identification::ALT_syntax:
      mp_field = single_value.field_syntax->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("syntax")));
      break;
    case CHARACTER_STRING_identification::ALT_presentation__context__id:
      mp_field = single_value.field_presentation__context__id->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("presentation_context_id")));
      break;
    case CHARACTER_STRING_identification::ALT_context__negotiation:
      mp_field = single_value.field_context__negotiation->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("context_negotiation")));
      break;
    case CHARACTER_STRING_identification::ALT_transfer__syntax:
      mp_field = single_value.field_transfer__syntax->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("transfer_syntax")));
      break;
    case CHARACTER_STRING_identification::ALT_fixed:
      mp_field = single_value.field_fixed->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("fixed")));
      break;
    default:
      break;
    }
    mp = new Module_Param_Assignment_List();
    mp->add_elem(mp_field);
    break; }
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
    TTCN_error("Referencing an uninitialized/unsupported value of type CHARACTER STRING.identification.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void CHARACTER_STRING_identification::encode_text(Text_Buf& text_buf) const
{
  text_buf.push_int(union_selection);
  switch (union_selection) {
  case ALT_syntaxes:
    field_syntaxes->encode_text(text_buf);
    break;
  case ALT_syntax:
    field_syntax->encode_text(text_buf);
    break;
  case ALT_presentation__context__id:
    field_presentation__context__id->encode_text(text_buf);
    break;
  case ALT_context__negotiation:
    field_context__negotiation->encode_text(text_buf);
    break;
  case ALT_transfer__syntax:
    field_transfer__syntax->encode_text(text_buf);
    break;
  case ALT_fixed:
    field_fixed->encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an unbound value of union type CHARACTER STRING.identification.");
  }
}

void CHARACTER_STRING_identification::decode_text(Text_Buf& text_buf)
{
  switch ((union_selection_type)text_buf.pull_int().get_val()) {
  case ALT_syntaxes:
    syntaxes().decode_text(text_buf);
    break;
  case ALT_syntax:
    syntax().decode_text(text_buf);
    break;
  case ALT_presentation__context__id:
    presentation__context__id().decode_text(text_buf);
    break;
  case ALT_context__negotiation:
    context__negotiation().decode_text(text_buf);
    break;
  case ALT_transfer__syntax:
    transfer__syntax().decode_text(text_buf);
    break;
  case ALT_fixed:
    fixed().decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: Unrecognized union selector was received for type CHARACTER STRING.identification.");
  }
}

/* not called
void CHARACTER_STRING_identification::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const
void CHARACTER_STRING_identification::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)
*/

ASN_BER_TLV_t *CHARACTER_STRING_identification::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv;
  TTCN_EncDec_ErrorContext ec_0("Alternative '");
  TTCN_EncDec_ErrorContext ec_1;
  switch (union_selection) {
  case ALT_syntaxes:
    ec_1.set_msg("syntaxes': ");
    new_tlv = field_syntaxes->BER_encode_TLV(CHARACTER_STRING_identification_syntaxes_descr_, p_coding);
    break;
  case ALT_syntax:
    ec_1.set_msg("syntax': ");
    new_tlv = field_syntax->BER_encode_TLV(CHARACTER_STRING_identification_syntax_descr_, p_coding);
    break;
  case ALT_presentation__context__id:
    ec_1.set_msg("presentation_context_id': ");
    new_tlv = field_presentation__context__id->BER_encode_TLV(CHARACTER_STRING_identification_presentation__context__id_descr_, p_coding);
    break;
  case ALT_context__negotiation:
    ec_1.set_msg("context_negotiation': ");
    new_tlv = field_context__negotiation->BER_encode_TLV(CHARACTER_STRING_identification_context__negotiation_descr_, p_coding);
    break;
  case ALT_transfer__syntax:
    ec_1.set_msg("transfer_syntax': ");
    new_tlv = field_transfer__syntax->BER_encode_TLV(CHARACTER_STRING_identification_transfer__syntax_descr_, p_coding);
    break;
  case ALT_fixed:
    ec_1.set_msg("fixed': ");
    new_tlv = field_fixed->BER_encode_TLV(CHARACTER_STRING_identification_fixed_descr_, p_coding);
    break;
  case UNBOUND_VALUE:
    new_tlv = BER_encode_chk_bound(FALSE);
    break;
  default:
    TTCN_EncDec_ErrorContext::error_internal("Unknown selection.");
    new_tlv = NULL;
  }
  return ASN_BER_V2TLV(new_tlv, p_td, p_coding);
}

boolean CHARACTER_STRING_identification::BER_decode_set_selection(const ASN_BER_TLV_t& p_tlv)
{
  clean_up();
  field_syntaxes = new CHARACTER_STRING_identification_syntaxes;
  union_selection = ALT_syntaxes;
  if (field_syntaxes->BER_decode_isMyMsg(CHARACTER_STRING_identification_syntaxes_descr_, p_tlv)) return TRUE;
  delete field_syntaxes;
  field_syntax = new OBJID;
  union_selection = ALT_syntax;
  if (field_syntax->BER_decode_isMyMsg(CHARACTER_STRING_identification_syntax_descr_, p_tlv)) return TRUE;
  delete field_syntax;
  field_presentation__context__id = new INTEGER;
  union_selection = ALT_presentation__context__id;
  if (field_presentation__context__id->BER_decode_isMyMsg(CHARACTER_STRING_identification_presentation__context__id_descr_, p_tlv)) return TRUE;
  delete field_presentation__context__id;
  field_context__negotiation = new CHARACTER_STRING_identification_context__negotiation;
  union_selection = ALT_context__negotiation;
  if (field_context__negotiation->BER_decode_isMyMsg(CHARACTER_STRING_identification_context__negotiation_descr_, p_tlv)) return TRUE;
  delete field_context__negotiation;
  field_transfer__syntax = new OBJID;
  union_selection = ALT_transfer__syntax;
  if (field_transfer__syntax->BER_decode_isMyMsg(CHARACTER_STRING_identification_transfer__syntax_descr_, p_tlv)) return TRUE;
  delete field_transfer__syntax;
  field_fixed = new ASN_NULL;
  union_selection = ALT_fixed;
  if (field_fixed->BER_decode_isMyMsg(CHARACTER_STRING_identification_fixed_descr_, p_tlv)) return TRUE;
  delete field_fixed;
  union_selection = UNBOUND_VALUE;
  return FALSE;
}

boolean CHARACTER_STRING_identification::BER_decode_isMyMsg(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv)
{
  if (p_td.ber->n_tags == 0) {
    CHARACTER_STRING_identification tmp_type;
    return tmp_type.BER_decode_set_selection(p_tlv);
  } else return Base_Type::BER_decode_isMyMsg(p_td, p_tlv);
}

boolean CHARACTER_STRING_identification::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form)
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec_0("While decoding 'CHARACTER STRING.identification' type: ");
  ASN_BER_TLV_t tmp_tlv;
  if (!BER_decode_TLV_CHOICE(*p_td.ber, stripped_tlv, L_form, tmp_tlv) || !BER_decode_CHOICE_selection(BER_decode_set_selection(tmp_tlv), tmp_tlv)) return FALSE;
  TTCN_EncDec_ErrorContext ec_1("Alternative '");
  TTCN_EncDec_ErrorContext ec_2;
  switch (union_selection) {
  case ALT_syntaxes:
    ec_2.set_msg("syntaxes': ");
    field_syntaxes->BER_decode_TLV(CHARACTER_STRING_identification_syntaxes_descr_, tmp_tlv, L_form);
    break;
  case ALT_syntax:
    ec_2.set_msg("syntax': ");
    field_syntax->BER_decode_TLV(CHARACTER_STRING_identification_syntax_descr_, tmp_tlv, L_form);
    break;
  case ALT_presentation__context__id:
    ec_2.set_msg("presentation_context_id': ");
    field_presentation__context__id->BER_decode_TLV(CHARACTER_STRING_identification_presentation__context__id_descr_, tmp_tlv, L_form);
    break;
  case ALT_context__negotiation:
    ec_2.set_msg("context_negotiation': ");
    field_context__negotiation->BER_decode_TLV(CHARACTER_STRING_identification_context__negotiation_descr_, tmp_tlv, L_form);
    break;
  case ALT_transfer__syntax:
    ec_2.set_msg("transfer_syntax': ");
    field_transfer__syntax->BER_decode_TLV(CHARACTER_STRING_identification_transfer__syntax_descr_, tmp_tlv, L_form);
    break;
  case ALT_fixed:
    ec_2.set_msg("fixed': ");
    field_fixed->BER_decode_TLV(CHARACTER_STRING_identification_fixed_descr_, tmp_tlv, L_form);
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

// FIXME maybe: XER_encode and decode is virtually identical to EMBEDDED_PDV

int CHARACTER_STRING_identification::XER_encode(const XERdescriptor_t& p_td,
               TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const
{
  int encoded_length=(int)p_buf.get_len();

  begin_xml(p_td, p_buf, flavor, indent++, FALSE);

  flavor &= XER_MASK;
  switch (union_selection) {
  case ALT_syntaxes:
    field_syntaxes->XER_encode(EMBEDDED_PDV_identification_sxs_xer_, p_buf, flavor, flavor2, indent, 0);
    break;
  case ALT_syntax:
    field_syntax->XER_encode(EMBEDDED_PDV_identification_sx_xer_, p_buf, flavor, flavor2, indent, 0);
    break;
  case ALT_presentation__context__id:
    field_presentation__context__id->XER_encode(EMBEDDED_PDV_identification_pci_xer_, p_buf, flavor, flavor2, indent, 0);
    break;
  case ALT_context__negotiation:
    field_context__negotiation->XER_encode(EMBEDDED_PDV_identification_cn_xer_, p_buf, flavor, flavor2,indent, 0);
    break;
  case ALT_transfer__syntax:
    field_transfer__syntax->XER_encode(EMBEDDED_PDV_identification_ts_xer_, p_buf, flavor, flavor2, indent, 0);
    break;
  case ALT_fixed:
    field_fixed->XER_encode(EMBEDDED_PDV_identification_fix_xer_, p_buf, flavor, flavor2, indent, 0);
    break;
  default:
    TTCN_EncDec_ErrorContext::error_internal("Unknown selection.");
    break;
  }

  end_xml(p_td, p_buf, flavor, --indent, FALSE);

  return (int)p_buf.get_len() - encoded_length;
}

int CHARACTER_STRING_identification::XER_decode(const XERdescriptor_t& p_td,
  XmlReaderWrap& reader, unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*)
{
  boolean exer  = is_exer(flavor);
  // we are supposed to be parked on our element
  for (int success = 1; success == 1; success = reader.Read()) {
    int type = reader.NodeType();
    switch (type) {
    case XML_READER_TYPE_ELEMENT: {
      if (verify_name(reader, p_td, exer)) {
	// it's us
	for (success = reader.Read(); success == 1; success = reader.Read()) {
	  type = reader.NodeType();
	  if (XML_READER_TYPE_ELEMENT == type) break;
	  else if (XML_READER_TYPE_END_ELEMENT == type) goto bail;
	}
	const char *name = (const char*)reader.Name();
	// Avoid chained if-else on strcmp. Use the length as a hash
	// (thankfully, they are all different).
	size_t namelen = strlen(name);
	switch (namelen) {
	case 8: // syntaxes
	  syntaxes().XER_decode(EMBEDDED_PDV_identification_sxs_xer_, reader, flavor, flavor2, 0);
	  break;

	case 6: // syntax
	  syntax().XER_decode(EMBEDDED_PDV_identification_sx_xer_, reader, flavor, flavor2, 0);
	  break;

	case 23: // presentation-context-id
	  presentation__context__id().XER_decode(EMBEDDED_PDV_identification_pci_xer_, reader, flavor, flavor2, 0);
	  break;

	case 19: // context-negotiation
	  context__negotiation().XER_decode(EMBEDDED_PDV_identification_cn_xer_, reader, flavor, flavor2, 0);
	  break;

	case 15: // transfer-syntax
	  transfer__syntax().XER_decode(EMBEDDED_PDV_identification_ts_xer_, reader, flavor, flavor2, 0);
	  break;

	case 5: // fixed
	  fixed().XER_decode(EMBEDDED_PDV_identification_fix_xer_, reader, flavor, flavor2, 0);
	  break;

	default:
	  goto bail;
	} // switch
      }
      else { // it belongs to somebody else
	goto bail;
      }
      break; }
    case XML_READER_TYPE_END_ELEMENT: {
      // advance to the next thing and bail
      reader.Read();
      goto bail; }
    }
  }
  bail:
  return 1;
}

void CHARACTER_STRING_identification_template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    switch (single_value.union_selection) {
    case CHARACTER_STRING_identification::ALT_syntaxes:
      delete single_value.field_syntaxes;
      break;
    case CHARACTER_STRING_identification::ALT_syntax:
      delete single_value.field_syntax;
      break;
    case CHARACTER_STRING_identification::ALT_presentation__context__id:
      delete single_value.field_presentation__context__id;
      break;
    case CHARACTER_STRING_identification::ALT_context__negotiation:
      delete single_value.field_context__negotiation;
      break;
    case CHARACTER_STRING_identification::ALT_transfer__syntax:
      delete single_value.field_transfer__syntax;
      break;
    case CHARACTER_STRING_identification::ALT_fixed:
      delete single_value.field_fixed;
      break;
    default:
      break;
    }
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void CHARACTER_STRING_identification_template::copy_value(const CHARACTER_STRING_identification& other_value)
{
  single_value.union_selection = other_value.get_selection();
  switch (single_value.union_selection) {
  case CHARACTER_STRING_identification::ALT_syntaxes:
    single_value.field_syntaxes = new CHARACTER_STRING_identification_syntaxes_template(other_value.syntaxes());
    break;
  case CHARACTER_STRING_identification::ALT_syntax:
    single_value.field_syntax = new OBJID_template(other_value.syntax());
    break;
  case CHARACTER_STRING_identification::ALT_presentation__context__id:
    single_value.field_presentation__context__id = new INTEGER_template(other_value.presentation__context__id());
    break;
  case CHARACTER_STRING_identification::ALT_context__negotiation:
    single_value.field_context__negotiation = new CHARACTER_STRING_identification_context__negotiation_template(other_value.context__negotiation());
    break;
  case CHARACTER_STRING_identification::ALT_transfer__syntax:
    single_value.field_transfer__syntax = new OBJID_template(other_value.transfer__syntax());
    break;
  case CHARACTER_STRING_identification::ALT_fixed:
    single_value.field_fixed = new ASN_NULL_template(other_value.fixed());
    break;
  default:
    TTCN_error("Initializing a template with an unbound value of type CHARACTER STRING.identification.");
  }
  set_selection(SPECIFIC_VALUE);
}

void CHARACTER_STRING_identification_template::copy_template(const CHARACTER_STRING_identification_template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    single_value.union_selection = other_value.single_value.union_selection;
    switch (single_value.union_selection) {
    case CHARACTER_STRING_identification::ALT_syntaxes:
      single_value.field_syntaxes = new CHARACTER_STRING_identification_syntaxes_template(*other_value.single_value.field_syntaxes);
      break;
    case CHARACTER_STRING_identification::ALT_syntax:
      single_value.field_syntax = new OBJID_template(*other_value.single_value.field_syntax);
      break;
    case CHARACTER_STRING_identification::ALT_presentation__context__id:
      single_value.field_presentation__context__id = new INTEGER_template(*other_value.single_value.field_presentation__context__id);
      break;
    case CHARACTER_STRING_identification::ALT_context__negotiation:
      single_value.field_context__negotiation = new CHARACTER_STRING_identification_context__negotiation_template(*other_value.single_value.field_context__negotiation);
      break;
    case CHARACTER_STRING_identification::ALT_transfer__syntax:
      single_value.field_transfer__syntax = new OBJID_template(*other_value.single_value.field_transfer__syntax);
      break;
    case CHARACTER_STRING_identification::ALT_fixed:
      single_value.field_fixed = new ASN_NULL_template(*other_value.single_value.field_fixed);
      break;
    default:
      TTCN_error("Internal error: Invalid union selector in a specific value when copying a template of type CHARACTER STRING.identification.");
    }
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = new CHARACTER_STRING_identification_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].copy_template(other_value.value_list.list_value[list_count]);
    break;
  default:
    TTCN_error("Copying an uninitialized template of union type CHARACTER STRING.identification.");
  }
  set_selection(other_value);
}

CHARACTER_STRING_identification_template::CHARACTER_STRING_identification_template()
{
}

CHARACTER_STRING_identification_template::CHARACTER_STRING_identification_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

CHARACTER_STRING_identification_template::CHARACTER_STRING_identification_template(const CHARACTER_STRING_identification& other_value)
{
  copy_value(other_value);
}

CHARACTER_STRING_identification_template::CHARACTER_STRING_identification_template(const OPTIONAL<CHARACTER_STRING_identification>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const CHARACTER_STRING_identification&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a template of union type CHARACTER STRING.identification from an unbound optional field.");
  }
}

CHARACTER_STRING_identification_template::CHARACTER_STRING_identification_template(const CHARACTER_STRING_identification_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

CHARACTER_STRING_identification_template::~CHARACTER_STRING_identification_template()
{
  clean_up();
}

CHARACTER_STRING_identification_template& CHARACTER_STRING_identification_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

CHARACTER_STRING_identification_template& CHARACTER_STRING_identification_template::operator=(const CHARACTER_STRING_identification& other_value)
{
  clean_up();
  copy_value(other_value);
  return *this;
}

CHARACTER_STRING_identification_template& CHARACTER_STRING_identification_template::operator=(const OPTIONAL<CHARACTER_STRING_identification>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const CHARACTER_STRING_identification&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a template of union type CHARACTER STRING.identification.");
  }
  return *this;
}

CHARACTER_STRING_identification_template& CHARACTER_STRING_identification_template::operator=(const CHARACTER_STRING_identification_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean CHARACTER_STRING_identification_template::match(const CHARACTER_STRING_identification& other_value,
                                                        boolean /* legacy */) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE:
    {
      CHARACTER_STRING_identification::union_selection_type value_selection = other_value.get_selection();
      if (value_selection == CHARACTER_STRING_identification::UNBOUND_VALUE) return FALSE;
      if (value_selection != single_value.union_selection) return FALSE;
      switch (value_selection) {
      case CHARACTER_STRING_identification::ALT_syntaxes:
	return single_value.field_syntaxes->match(other_value.syntaxes());
      case CHARACTER_STRING_identification::ALT_syntax:
	return single_value.field_syntax->match(other_value.syntax());
      case CHARACTER_STRING_identification::ALT_presentation__context__id:
	return single_value.field_presentation__context__id->match(other_value.presentation__context__id());
      case CHARACTER_STRING_identification::ALT_context__negotiation:
	return single_value.field_context__negotiation->match(other_value.context__negotiation());
      case CHARACTER_STRING_identification::ALT_transfer__syntax:
	return single_value.field_transfer__syntax->match(other_value.transfer__syntax());
      case CHARACTER_STRING_identification::ALT_fixed:
	return single_value.field_fixed->match(other_value.fixed());
      default:
	TTCN_error("Internal error: Invalid selector in a specific value when matching a template of union type CHARACTER STRING.identification.");
      }
    }
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count].match(other_value)) return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error ("Matching an uninitialized template of union type CHARACTER STRING.identification.");
  }
  return FALSE;
}

CHARACTER_STRING_identification CHARACTER_STRING_identification_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of union type CHARACTER STRING.identification.");
  CHARACTER_STRING_identification ret_val;
  switch (single_value.union_selection) {
  case CHARACTER_STRING_identification::ALT_syntaxes:
    ret_val.syntaxes() = single_value.field_syntaxes->valueof();
    break;
  case CHARACTER_STRING_identification::ALT_syntax:
    ret_val.syntax() = single_value.field_syntax->valueof();
    break;
  case CHARACTER_STRING_identification::ALT_presentation__context__id:
    ret_val.presentation__context__id() = single_value.field_presentation__context__id->valueof();
    break;
  case CHARACTER_STRING_identification::ALT_context__negotiation:
    ret_val.context__negotiation() = single_value.field_context__negotiation->valueof();
    break;
  case CHARACTER_STRING_identification::ALT_transfer__syntax:
    ret_val.transfer__syntax() = single_value.field_transfer__syntax->valueof();
    break;
  case CHARACTER_STRING_identification::ALT_fixed:
    ret_val.fixed() = single_value.field_fixed->valueof();
    break;
  default:
    TTCN_error("Internal error: Invalid selector in a specific value when performing valueof operation on a template of union type CHARACTER STRING.identification.");
  }
  return ret_val;
}

CHARACTER_STRING_identification_template& CHARACTER_STRING_identification_template::list_item(unsigned int list_index) const
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST) TTCN_error("Internal error: Accessing a list element of a non-list template of union type CHARACTER STRING.identification.");
  if (list_index >= value_list.n_values) TTCN_error("Internal error: Index overflow in a value list template of union type CHARACTER STRING.identification.");
  return value_list.list_value[list_index];
}
void CHARACTER_STRING_identification_template::set_type(template_sel template_type, unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST) TTCN_error ("Internal error: Setting an invalid list for a template of union type CHARACTER STRING.identification.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new CHARACTER_STRING_identification_template[list_length];
}

CHARACTER_STRING_identification_syntaxes_template& CHARACTER_STRING_identification_template::syntaxes()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != CHARACTER_STRING_identification::ALT_syntaxes) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_syntaxes = new CHARACTER_STRING_identification_syntaxes_template(ANY_VALUE);
    else single_value.field_syntaxes = new CHARACTER_STRING_identification_syntaxes_template;
    single_value.union_selection = CHARACTER_STRING_identification::ALT_syntaxes;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_syntaxes;
}

const CHARACTER_STRING_identification_syntaxes_template& CHARACTER_STRING_identification_template::syntaxes() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field syntaxes in a non-specific template of union type CHARACTER STRING.identification.");
  if (single_value.union_selection != CHARACTER_STRING_identification::ALT_syntaxes) TTCN_error("Accessing non-selected field syntaxes in a template of union type CHARACTER STRING.identification.");
  return *single_value.field_syntaxes;
}

OBJID_template& CHARACTER_STRING_identification_template::syntax()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != CHARACTER_STRING_identification::ALT_syntax) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_syntax = new OBJID_template(ANY_VALUE);
    else single_value.field_syntax = new OBJID_template;
    single_value.union_selection = CHARACTER_STRING_identification::ALT_syntax;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_syntax;
}

const OBJID_template& CHARACTER_STRING_identification_template::syntax() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field syntax in a non-specific template of union type CHARACTER STRING.identification.");
  if (single_value.union_selection != CHARACTER_STRING_identification::ALT_syntax) TTCN_error("Accessing non-selected field syntax in a template of union type CHARACTER STRING.identification.");
  return *single_value.field_syntax;
}

INTEGER_template& CHARACTER_STRING_identification_template::presentation__context__id()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != CHARACTER_STRING_identification::ALT_presentation__context__id) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_presentation__context__id = new INTEGER_template(ANY_VALUE);
    else single_value.field_presentation__context__id = new INTEGER_template;
    single_value.union_selection = CHARACTER_STRING_identification::ALT_presentation__context__id;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_presentation__context__id;
}

const INTEGER_template& CHARACTER_STRING_identification_template::presentation__context__id() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field presentation_context_id in a non-specific template of union type CHARACTER STRING.identification.");
  if (single_value.union_selection != CHARACTER_STRING_identification::ALT_presentation__context__id) TTCN_error("Accessing non-selected field presentation_context_id in a template of union type CHARACTER STRING.identification.");
  return *single_value.field_presentation__context__id;
}

CHARACTER_STRING_identification_context__negotiation_template& CHARACTER_STRING_identification_template::context__negotiation()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != CHARACTER_STRING_identification::ALT_context__negotiation) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_context__negotiation = new CHARACTER_STRING_identification_context__negotiation_template(ANY_VALUE);
    else single_value.field_context__negotiation = new CHARACTER_STRING_identification_context__negotiation_template;
    single_value.union_selection = CHARACTER_STRING_identification::ALT_context__negotiation;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_context__negotiation;
}

const CHARACTER_STRING_identification_context__negotiation_template& CHARACTER_STRING_identification_template::context__negotiation() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field context_negotiation in a non-specific template of union type CHARACTER STRING.identification.");
  if (single_value.union_selection != CHARACTER_STRING_identification::ALT_context__negotiation) TTCN_error("Accessing non-selected field context_negotiation in a template of union type CHARACTER STRING.identification.");
  return *single_value.field_context__negotiation;
}

OBJID_template& CHARACTER_STRING_identification_template::transfer__syntax()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != CHARACTER_STRING_identification::ALT_transfer__syntax) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_transfer__syntax = new OBJID_template(ANY_VALUE);
    else single_value.field_transfer__syntax = new OBJID_template;
    single_value.union_selection = CHARACTER_STRING_identification::ALT_transfer__syntax;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_transfer__syntax;
}

const OBJID_template& CHARACTER_STRING_identification_template::transfer__syntax() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field transfer_syntax in a non-specific template of union type CHARACTER STRING.identification.");
  if (single_value.union_selection != CHARACTER_STRING_identification::ALT_transfer__syntax) TTCN_error("Accessing non-selected field transfer_syntax in a template of union type CHARACTER STRING.identification.");
  return *single_value.field_transfer__syntax;
}

ASN_NULL_template& CHARACTER_STRING_identification_template::fixed()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != CHARACTER_STRING_identification::ALT_fixed) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_fixed = new ASN_NULL_template(ANY_VALUE);
    else single_value.field_fixed = new ASN_NULL_template;
    single_value.union_selection = CHARACTER_STRING_identification::ALT_fixed;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_fixed;
}

const ASN_NULL_template& CHARACTER_STRING_identification_template::fixed() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field fixed in a non-specific template of union type CHARACTER STRING.identification.");
  if (single_value.union_selection != CHARACTER_STRING_identification::ALT_fixed) TTCN_error("Accessing non-selected field fixed in a template of union type CHARACTER STRING.identification.");
  return *single_value.field_fixed;
}

boolean CHARACTER_STRING_identification_template::ischosen(CHARACTER_STRING_identification::union_selection_type checked_selection) const
{
  if (checked_selection == CHARACTER_STRING_identification::UNBOUND_VALUE) TTCN_error("Internal error: Performing ischosen() operation on an invalid field of union type CHARACTER STRING.identification.");
  switch (template_selection) {
  case SPECIFIC_VALUE:
    if (single_value.union_selection == CHARACTER_STRING_identification::UNBOUND_VALUE) TTCN_error("Internal error: Invalid selector in a specific value when performing ischosen() operation on a template of union type CHARACTER STRING.identification.");
    return single_value.union_selection == checked_selection;
  case VALUE_LIST:
    {
      if (value_list.n_values < 1)
	TTCN_error("Internal error: Performing ischosen() operation on a template of union type CHARACTER STRING.identification containing an empty list.");
      boolean ret_val = value_list.list_value[0].ischosen(checked_selection);
      boolean all_same = TRUE;
      for (unsigned int list_count = 1; list_count < value_list.n_values; list_count++) {
	if (value_list.list_value[list_count].ischosen(checked_selection) != ret_val) {
	  all_same = FALSE;
	  break;
	}
      }
      if (all_same) return ret_val;
    }
    // FIXME really no break?
  case ANY_VALUE:
  case ANY_OR_OMIT:
  case OMIT_VALUE:
  case COMPLEMENTED_LIST:
    TTCN_error("Performing ischosen() operation on a template of union type CHARACTER STRING.identification, which does not determine unambiguously the chosen field of the matching values.");
  default:
    TTCN_error("Performing ischosen() operation on an uninitialized template of union type CHARACTER STRING.identification");
  }
  return FALSE;
}

void CHARACTER_STRING_identification_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    switch (single_value.union_selection) {
    case CHARACTER_STRING_identification::ALT_syntaxes:
      TTCN_Logger::log_event_str("{ syntaxes := ");
      single_value.field_syntaxes->log();
      TTCN_Logger::log_event_str(" }");
      break;
    case CHARACTER_STRING_identification::ALT_syntax:
      TTCN_Logger::log_event_str("{ syntax := ");
      single_value.field_syntax->log();
      TTCN_Logger::log_event_str(" }");
      break;
    case CHARACTER_STRING_identification::ALT_presentation__context__id:
      TTCN_Logger::log_event_str("{ presentation_context_id := ");
      single_value.field_presentation__context__id->log();
      TTCN_Logger::log_event_str(" }");
      break;
    case CHARACTER_STRING_identification::ALT_context__negotiation:
      TTCN_Logger::log_event_str("{ context_negotiation := ");
      single_value.field_context__negotiation->log();
      TTCN_Logger::log_event_str(" }");
      break;
    case CHARACTER_STRING_identification::ALT_transfer__syntax:
      TTCN_Logger::log_event_str("{ transfer_syntax := ");
      single_value.field_transfer__syntax->log();
      TTCN_Logger::log_event_str(" }");
      break;
    case CHARACTER_STRING_identification::ALT_fixed:
      TTCN_Logger::log_event_str("{ fixed := ");
      single_value.field_fixed->log();
      TTCN_Logger::log_event_str(" }");
      break;
    default:
      TTCN_Logger::log_event_str("<invalid selector>");
      break;
    }
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count].log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void CHARACTER_STRING_identification_template::log_match(const CHARACTER_STRING_identification& match_value,
                                                         boolean /* legacy */) const
{
  if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
    if(match(match_value)){
      TTCN_Logger::print_logmatch_buffer();
      TTCN_Logger::log_event_str(" matched ");
    }
    return;
  }
  if (template_selection == SPECIFIC_VALUE && single_value.union_selection == match_value.get_selection()) {
    switch (single_value.union_selection) {
    case CHARACTER_STRING_identification::ALT_syntaxes:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".syntaxes");
        single_value.field_syntaxes->log_match(match_value.syntaxes());
      }else{
        TTCN_Logger::log_event_str("{ syntaxes := ");
        single_value.field_syntaxes->log_match(match_value.syntaxes());
        TTCN_Logger::log_event_str(" }");
      }
      break;
    case CHARACTER_STRING_identification::ALT_syntax:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".syntax");
        single_value.field_syntax->log_match(match_value.syntax());
      }else{
        TTCN_Logger::log_event_str("{ syntax := ");
        single_value.field_syntax->log_match(match_value.syntax());
        TTCN_Logger::log_event_str(" }");
      }
      break;
    case CHARACTER_STRING_identification::ALT_presentation__context__id:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".presentation_context_id");
        single_value.field_presentation__context__id->log_match(match_value.presentation__context__id());
      }else{
        TTCN_Logger::log_event_str("{ presentation_context_id := ");
        single_value.field_presentation__context__id->log_match(match_value.presentation__context__id());
        TTCN_Logger::log_event_str(" }");
      }
      break;
    case CHARACTER_STRING_identification::ALT_context__negotiation:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".context_negotiation");
        single_value.field_context__negotiation->log_match(match_value.context__negotiation());
      }else{
        TTCN_Logger::log_event_str("{ context_negotiation := ");
        single_value.field_context__negotiation->log_match(match_value.context__negotiation());
        TTCN_Logger::log_event_str(" }");
      }
      break;
    case CHARACTER_STRING_identification::ALT_transfer__syntax:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".transfer_syntax");
        single_value.field_transfer__syntax->log_match(match_value.transfer__syntax());
      }else{
        TTCN_Logger::log_event_str("{ transfer_syntax := ");
        single_value.field_transfer__syntax->log_match(match_value.transfer__syntax());
        TTCN_Logger::log_event_str(" }");
      }
      break;
    case CHARACTER_STRING_identification::ALT_fixed:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".fixed");
        single_value.field_fixed->log_match(match_value.fixed());
      }else{
      TTCN_Logger::log_event_str("{ fixed := ");
      single_value.field_fixed->log_match(match_value.fixed());
      TTCN_Logger::log_event_str(" }");
      }
      break;
    default:
      TTCN_Logger::print_logmatch_buffer();
      TTCN_Logger::log_event_str("<invalid selector>");
      break;
    }
  } else {
    TTCN_Logger::print_logmatch_buffer();
    match_value.log();
    TTCN_Logger::log_event_str(" with ");
    log();
    if (match(match_value)) TTCN_Logger::log_event_str(" matched");
    else TTCN_Logger::log_event_str(" unmatched");
  }
}

void CHARACTER_STRING_identification_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    text_buf.push_int(single_value.union_selection);
    switch (single_value.union_selection) {
    case CHARACTER_STRING_identification::ALT_syntaxes:
      single_value.field_syntaxes->encode_text(text_buf);
      break;
    case CHARACTER_STRING_identification::ALT_syntax:
      single_value.field_syntax->encode_text(text_buf);
      break;
    case CHARACTER_STRING_identification::ALT_presentation__context__id:
      single_value.field_presentation__context__id->encode_text(text_buf);
      break;
    case CHARACTER_STRING_identification::ALT_context__negotiation:
      single_value.field_context__negotiation->encode_text(text_buf);
      break;
    case CHARACTER_STRING_identification::ALT_transfer__syntax:
      single_value.field_transfer__syntax->encode_text(text_buf);
      break;
    case CHARACTER_STRING_identification::ALT_fixed:
      single_value.field_fixed->encode_text(text_buf);
      break;
    default:
      TTCN_error("Internal error: Invalid selector in a specific value when encoding a template of union type CHARACTER STRING.identification.");
    }
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized template of type CHARACTER STRING.identification.");
  }
}

void CHARACTER_STRING_identification_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    {
      single_value.union_selection = CHARACTER_STRING_identification::UNBOUND_VALUE;
      CHARACTER_STRING_identification::union_selection_type new_selection = (CHARACTER_STRING_identification::union_selection_type)text_buf.pull_int().get_val();
      switch (new_selection) {
      case CHARACTER_STRING_identification::ALT_syntaxes:
	single_value.field_syntaxes = new CHARACTER_STRING_identification_syntaxes_template;
	single_value.field_syntaxes->decode_text(text_buf);
	break;
      case CHARACTER_STRING_identification::ALT_syntax:
	single_value.field_syntax = new OBJID_template;
	single_value.field_syntax->decode_text(text_buf);
	break;
      case CHARACTER_STRING_identification::ALT_presentation__context__id:
	single_value.field_presentation__context__id = new INTEGER_template;
	single_value.field_presentation__context__id->decode_text(text_buf);
	break;
      case CHARACTER_STRING_identification::ALT_context__negotiation:
	single_value.field_context__negotiation = new CHARACTER_STRING_identification_context__negotiation_template;
	single_value.field_context__negotiation->decode_text(text_buf);
	break;
      case CHARACTER_STRING_identification::ALT_transfer__syntax:
	single_value.field_transfer__syntax = new OBJID_template;
	single_value.field_transfer__syntax->decode_text(text_buf);
	break;
      case CHARACTER_STRING_identification::ALT_fixed:
	single_value.field_fixed = new ASN_NULL_template;
	single_value.field_fixed->decode_text(text_buf);
	break;
      default:
	TTCN_error("Text decoder: Unrecognized union selector was received for a template of type CHARACTER STRING.identification.");
      }
      single_value.union_selection = new_selection;
    }
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new CHARACTER_STRING_identification_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: Unrecognized selector was received in a template of type CHARACTER STRING.identification.");
  }
}

boolean CHARACTER_STRING_identification_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean CHARACTER_STRING_identification_template::match_omit(boolean legacy /* = FALSE */) const
{
  if (is_ifpresent) return TRUE;
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    if (legacy) {
      for (unsigned int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i].match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    } // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
void CHARACTER_STRING_identification_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "CHARACTER STRING.identification");
}
#endif

CHARACTER_STRING_identification_syntaxes::CHARACTER_STRING_identification_syntaxes()
{
}

CHARACTER_STRING_identification_syntaxes::CHARACTER_STRING_identification_syntaxes(const OBJID& par_abstract,
										   const OBJID& par_transfer)
  : field_abstract(par_abstract),
    field_transfer(par_transfer)
{
}

boolean CHARACTER_STRING_identification_syntaxes::operator==(const CHARACTER_STRING_identification_syntaxes& other_value) const
{
  return field_abstract==other_value.field_abstract
    && field_transfer==other_value.field_transfer;
}

int CHARACTER_STRING_identification_syntaxes::size_of() const
{
  int ret_val = 2;
  return ret_val;
}

void CHARACTER_STRING_identification_syntaxes::log() const
{
  TTCN_Logger::log_event_str("{ abstract := ");
  field_abstract.log();
  TTCN_Logger::log_event_str(", transfer := ");
  field_transfer.log();
  TTCN_Logger::log_event_str(" }");
}

boolean CHARACTER_STRING_identification_syntaxes::is_bound() const
{
  if(field_abstract.is_bound()) return TRUE;
  if(field_transfer.is_bound()) return TRUE;
  return FALSE;
}

boolean CHARACTER_STRING_identification_syntaxes::is_value() const
{
  if(!field_abstract.is_value()) return FALSE;
  if(!field_transfer.is_value()) return FALSE;
  return TRUE;
}

void CHARACTER_STRING_identification_syntaxes::clean_up()
{
  field_abstract.clean_up();
  field_transfer.clean_up();
}

void CHARACTER_STRING_identification_syntaxes::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_VALUE, "record value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) return;
    if (2!=mp->get_size()) {
      param.error("record value of type CHARACTER STRING.identification.syntaxes has 2 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) abstract().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) transfer().set_param(*mp->get_elem(1));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "abstract")) {
        abstract().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "transfer")) {
        transfer().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type CHARACTER STRING.identification.syntaxes: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record value", "CHARACTER STRING.identification.syntaxes");
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* CHARACTER_STRING_identification_syntaxes::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  Module_Param* mp_field_abstract = field_abstract.get_param(param_name);
  mp_field_abstract->set_id(new Module_Param_FieldName(mcopystr("abstract")));
  Module_Param* mp_field_transfer = field_transfer.get_param(param_name);
  mp_field_transfer->set_id(new Module_Param_FieldName(mcopystr("transfer")));
  Module_Param_Assignment_List* mp = new Module_Param_Assignment_List();
  mp->add_elem(mp_field_abstract);
  mp->add_elem(mp_field_transfer);
  return mp;
}
#endif

void CHARACTER_STRING_identification_syntaxes::encode_text(Text_Buf& text_buf) const
{
  field_abstract.encode_text(text_buf);
  field_transfer.encode_text(text_buf);
}

void CHARACTER_STRING_identification_syntaxes::decode_text(Text_Buf& text_buf)
{
  field_abstract.decode_text(text_buf);
  field_transfer.decode_text(text_buf);
}

/* not called
void CHARACTER_STRING_identification_syntaxes::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const
void CHARACTER_STRING_identification_syntaxes::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)
*/

ASN_BER_TLV_t* CHARACTER_STRING_identification_syntaxes::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(NULL);
  TTCN_EncDec_ErrorContext ec_0("Component '");
  TTCN_EncDec_ErrorContext ec_1;
  ec_1.set_msg("abstract': ");
  new_tlv->add_TLV(field_abstract.BER_encode_TLV(CHARACTER_STRING_identification_syntaxes_abstract_descr_, p_coding));
  ec_1.set_msg("transfer': ");
  new_tlv->add_TLV(field_transfer.BER_encode_TLV(CHARACTER_STRING_identification_syntaxes_transfer_descr_, p_coding));
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean CHARACTER_STRING_identification_syntaxes::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form)
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec_0("While decoding 'CHARACTER STRING.identification.syntaxes' type: ");
  stripped_tlv.chk_constructed_flag(TRUE);
  size_t V_pos=0;
  ASN_BER_TLV_t tmp_tlv;
  boolean tlv_present=FALSE;
  {
    TTCN_EncDec_ErrorContext ec_1("Component '");
    TTCN_EncDec_ErrorContext ec_2;
    ec_2.set_msg("abstract': ");
    if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
    if(!tlv_present) return FALSE;
    field_abstract.BER_decode_TLV(CHARACTER_STRING_identification_syntaxes_abstract_descr_, tmp_tlv, L_form);
    tlv_present=FALSE;
    ec_2.set_msg("transfer': ");
    if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
    if(!tlv_present) return FALSE;
    field_transfer.BER_decode_TLV(CHARACTER_STRING_identification_syntaxes_transfer_descr_, tmp_tlv, L_form);
    tlv_present=FALSE;
  }
  BER_decode_constdTLV_end(stripped_tlv, V_pos, L_form, tmp_tlv, tlv_present);
  return TRUE;
}

int CHARACTER_STRING_identification_syntaxes::XER_encode(const XERdescriptor_t& p_td,
               TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const
{
  int encoded_length=(int)p_buf.get_len();

  begin_xml(p_td, p_buf, flavor, indent++, FALSE);

  field_abstract.XER_encode(CHARACTER_STRING_identification_sxs_abs_xer_, p_buf, flavor, flavor2, indent, 0);
  field_transfer.XER_encode(CHARACTER_STRING_identification_sxs_xfr_xer_, p_buf, flavor, flavor2, indent, 0);

  end_xml(p_td, p_buf, flavor, --indent, FALSE);

  return p_buf.get_len() - encoded_length;
}

int CHARACTER_STRING_identification_syntaxes::XER_decode(
  const XERdescriptor_t& /*p_td*/, XmlReaderWrap& reader, unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*)
{ // we stand on <syntaxes>, move ahead first
  for (int success = reader.Read(); success == 1; success = reader.Read())
  {
    int type = reader.NodeType();
    if (XML_READER_TYPE_ELEMENT == type)
      // no verify_name for a CHOICE
      break;
  }
  // FIXME this assumes the right element
  field_abstract.XER_decode(EMBEDDED_PDV_identification_sxs_abs_xer_, reader, flavor, flavor2, 0);
  field_transfer.XER_decode(EMBEDDED_PDV_identification_sxs_xfr_xer_, reader, flavor, flavor2, 0);
  for (int success = 1; success == 1; success = reader.Read())
  {
    int type = reader.NodeType();
    if (XML_READER_TYPE_END_ELEMENT == type)
      break;
  }
  return 0; // TODO maybe return proper value
}


struct CHARACTER_STRING_identification_syntaxes_template::single_value_struct {
  OBJID_template field_abstract;
  OBJID_template field_transfer;
};

void CHARACTER_STRING_identification_syntaxes_template::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_TEMPLATE, "record template");
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
    CHARACTER_STRING_identification_syntaxes_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) break;
    if (2!=mp->get_size()) {
      param.error("record template of type CHARACTER STRING.identification.syntaxes has 2 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) abstract().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) transfer().set_param(*mp->get_elem(1));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "abstract")) {
        abstract().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "transfer")) {
        transfer().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type CHARACTER STRING.identification.syntaxes: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record template", "CHARACTER STRING.identification.syntaxes");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* CHARACTER_STRING_identification_syntaxes_template::get_param(Module_Param_Name& param_name) const
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
  case SPECIFIC_VALUE: {
    Module_Param* mp_field_abstract = single_value->field_abstract.get_param(param_name);
    mp_field_abstract->set_id(new Module_Param_FieldName(mcopystr("abstract")));
    Module_Param* mp_field_transfer = single_value->field_transfer.get_param(param_name);
    mp_field_transfer->set_id(new Module_Param_FieldName(mcopystr("transfer")));
    mp = new Module_Param_Assignment_List();
    mp->add_elem(mp_field_abstract);
    mp->add_elem(mp_field_transfer);
    break; }
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
    TTCN_error("Referencing an uninitialized/unsupported template of type CHARACTER STRING.identification.syntaxes.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void CHARACTER_STRING_identification_syntaxes_template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    delete single_value;
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void CHARACTER_STRING_identification_syntaxes_template::set_specific()
{
  if (template_selection != SPECIFIC_VALUE) {
    template_sel old_selection = template_selection;
    clean_up();
    single_value = new single_value_struct;
    set_selection(SPECIFIC_VALUE);
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) {
      single_value->field_abstract = ANY_VALUE;
      single_value->field_transfer = ANY_VALUE;
    }
  }
}

void CHARACTER_STRING_identification_syntaxes_template::copy_value(const CHARACTER_STRING_identification_syntaxes& other_value)
{
  single_value = new single_value_struct;
  single_value->field_abstract = other_value.abstract();
  single_value->field_transfer = other_value.transfer();
  set_selection(SPECIFIC_VALUE);
}

void CHARACTER_STRING_identification_syntaxes_template::copy_template(const CHARACTER_STRING_identification_syntaxes_template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct(*other_value.single_value);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = new CHARACTER_STRING_identification_syntaxes_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].copy_template(other_value.value_list.list_value[list_count]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported template of type CHARACTER STRING.identification.syntaxes.");
  }
  set_selection(other_value);
}

CHARACTER_STRING_identification_syntaxes_template::CHARACTER_STRING_identification_syntaxes_template()
{
}

CHARACTER_STRING_identification_syntaxes_template::CHARACTER_STRING_identification_syntaxes_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

CHARACTER_STRING_identification_syntaxes_template::CHARACTER_STRING_identification_syntaxes_template(const CHARACTER_STRING_identification_syntaxes& other_value)
{
  copy_value(other_value);
}

CHARACTER_STRING_identification_syntaxes_template::CHARACTER_STRING_identification_syntaxes_template(const OPTIONAL<CHARACTER_STRING_identification_syntaxes>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const CHARACTER_STRING_identification_syntaxes&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a template of type CHARACTER STRING.identification.syntaxes from an unbound optional field.");
  }
}

CHARACTER_STRING_identification_syntaxes_template::CHARACTER_STRING_identification_syntaxes_template(const CHARACTER_STRING_identification_syntaxes_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

CHARACTER_STRING_identification_syntaxes_template::~CHARACTER_STRING_identification_syntaxes_template()
{
  clean_up();
}

CHARACTER_STRING_identification_syntaxes_template& CHARACTER_STRING_identification_syntaxes_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

CHARACTER_STRING_identification_syntaxes_template& CHARACTER_STRING_identification_syntaxes_template::operator=(const CHARACTER_STRING_identification_syntaxes& other_value)
{
  clean_up();
  copy_value(other_value);
  return *this;
}

CHARACTER_STRING_identification_syntaxes_template& CHARACTER_STRING_identification_syntaxes_template::operator=(const OPTIONAL<CHARACTER_STRING_identification_syntaxes>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const CHARACTER_STRING_identification_syntaxes&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a template of type CHARACTER STRING.identification.syntaxes.");
  }
  return *this;
}

CHARACTER_STRING_identification_syntaxes_template& CHARACTER_STRING_identification_syntaxes_template::operator=(const CHARACTER_STRING_identification_syntaxes_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean CHARACTER_STRING_identification_syntaxes_template::match(const CHARACTER_STRING_identification_syntaxes& other_value,
                                                                 boolean /* legacy */) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE:
    if (!other_value.abstract().is_bound()) return FALSE;
    if (!single_value->field_abstract.match(other_value.abstract())) return FALSE;
    if (!other_value.transfer().is_bound()) return FALSE;
    if (!single_value->field_transfer.match(other_value.transfer())) return FALSE;
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count].match(other_value)) return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching an uninitialized/unsupported template of type CHARACTER STRING.identification.syntaxes.");
  }
  return FALSE;
}

CHARACTER_STRING_identification_syntaxes CHARACTER_STRING_identification_syntaxes_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of type CHARACTER STRING.identification.syntaxes.");
  CHARACTER_STRING_identification_syntaxes ret_val;
  ret_val.abstract() = single_value->field_abstract.valueof();
  ret_val.transfer() = single_value->field_transfer.valueof();
  return ret_val;
}

void CHARACTER_STRING_identification_syntaxes_template::set_type(template_sel template_type, unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list for a template of type CHARACTER STRING.identification.syntaxes.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new CHARACTER_STRING_identification_syntaxes_template[list_length];
}

CHARACTER_STRING_identification_syntaxes_template& CHARACTER_STRING_identification_syntaxes_template::list_item(unsigned int list_index) const
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list template of type CHARACTER STRING.identification.syntaxes.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a value list template of type CHARACTER STRING.identification.syntaxes.");
  return value_list.list_value[list_index];
}

OBJID_template& CHARACTER_STRING_identification_syntaxes_template::abstract()
{
  set_specific();
  return single_value->field_abstract;
}

const OBJID_template& CHARACTER_STRING_identification_syntaxes_template::abstract() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field abstract of a non-specific template of type CHARACTER STRING.identification.syntaxes.");
  return single_value->field_abstract;
}

OBJID_template& CHARACTER_STRING_identification_syntaxes_template::transfer()
{
  set_specific();
  return single_value->field_transfer;
}

const OBJID_template& CHARACTER_STRING_identification_syntaxes_template::transfer() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field transfer of a non-specific template of type CHARACTER STRING.identification.syntaxes.");
  return single_value->field_transfer;
}

int CHARACTER_STRING_identification_syntaxes_template::size_of() const
{
  switch (template_selection)
    {
    case SPECIFIC_VALUE:
      {
	int ret_val = 2;
	return ret_val;
      }
    case VALUE_LIST:
      {
	if (value_list.n_values<1)
	  TTCN_error("Internal error: Performing sizeof() operation on a template of type CHARACTER STRING.identification.syntaxes containing an empty list.");
	int item_size = value_list.list_value[0].size_of();
	for (unsigned int i = 1; i < value_list.n_values; i++)
	  {
	    if (value_list.list_value[i].size_of()!=item_size)
	      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING.identification.syntaxes containing a value list with different sizes.");
	  }
	return item_size;
      }
    case OMIT_VALUE:
      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING.identification.syntaxes containing omit value.");
    case ANY_VALUE:
    case ANY_OR_OMIT:
      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING.identification.syntaxes containing */? value.");
    case COMPLEMENTED_LIST:
      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING.identification.syntaxes containing complemented list.");
    default:
      TTCN_error("Performing sizeof() operation on an uninitialized/unsupported template of type CHARACTER STRING.identification.syntaxes.");
    }
  return 0;
}

void CHARACTER_STRING_identification_syntaxes_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    TTCN_Logger::log_event_str("{ abstract := ");
    single_value->field_abstract.log();
    TTCN_Logger::log_event_str(", transfer := ");
    single_value->field_transfer.log();
    TTCN_Logger::log_event_str(" }");
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count].log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void CHARACTER_STRING_identification_syntaxes_template::log_match(const CHARACTER_STRING_identification_syntaxes& match_value,
                                                                  boolean /* legacy */) const
{
  if (template_selection == SPECIFIC_VALUE) {
    TTCN_Logger::log_event_str("{ abstract := ");
    single_value->field_abstract.log_match(match_value.abstract());
    TTCN_Logger::log_event_str(", transfer := ");
    single_value->field_transfer.log_match(match_value.transfer());
    TTCN_Logger::log_event_str(" }");
  } else {
    match_value.log();
    TTCN_Logger::log_event_str(" with ");
    log();
    if (match(match_value)) TTCN_Logger::log_event_str(" matched");
    else TTCN_Logger::log_event_str(" unmatched");
  }
}

void CHARACTER_STRING_identification_syntaxes_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value->field_abstract.encode_text(text_buf);
    single_value->field_transfer.encode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported template of type CHARACTER STRING.identification.syntaxes.");
  }
}

void CHARACTER_STRING_identification_syntaxes_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct;
    single_value->field_abstract.decode_text(text_buf);
    single_value->field_transfer.decode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new CHARACTER_STRING_identification_syntaxes_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received in a template of type CHARACTER STRING.identification.syntaxes.");
  }
}

boolean CHARACTER_STRING_identification_syntaxes_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean CHARACTER_STRING_identification_syntaxes_template::match_omit(boolean legacy /* = FALSE */) const
{
  if (is_ifpresent) return TRUE;
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    if (legacy) {
      for (unsigned int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i].match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    } // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
void CHARACTER_STRING_identification_syntaxes_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "CHARACTER STRING.identification.syntaxes");
}
#endif

CHARACTER_STRING_identification_context__negotiation::CHARACTER_STRING_identification_context__negotiation()
{
}

CHARACTER_STRING_identification_context__negotiation::CHARACTER_STRING_identification_context__negotiation(const INTEGER& par_presentation__context__id,
													   const OBJID& par_transfer__syntax)
  : field_presentation__context__id(par_presentation__context__id),
    field_transfer__syntax(par_transfer__syntax)
{
}

boolean CHARACTER_STRING_identification_context__negotiation::operator==(const CHARACTER_STRING_identification_context__negotiation& other_value) const
{
  return field_presentation__context__id==other_value.field_presentation__context__id
    && field_transfer__syntax==other_value.field_transfer__syntax;
}

int CHARACTER_STRING_identification_context__negotiation::size_of() const
{
  int ret_val = 2;
  return ret_val;
}

void CHARACTER_STRING_identification_context__negotiation::log() const
{
  TTCN_Logger::log_event_str("{ presentation_context_id := ");
  field_presentation__context__id.log();
  TTCN_Logger::log_event_str(", transfer_syntax := ");
  field_transfer__syntax.log();
  TTCN_Logger::log_event_str(" }");
}

boolean CHARACTER_STRING_identification_context__negotiation::is_bound() const
{
  if(field_presentation__context__id.is_bound()) return TRUE;
  if(field_transfer__syntax.is_bound()) return TRUE;
  return FALSE;
}

boolean CHARACTER_STRING_identification_context__negotiation::is_value() const
{
  if(!field_presentation__context__id.is_value()) return FALSE;
  if(!field_transfer__syntax.is_value()) return FALSE;
  return TRUE;
}

void CHARACTER_STRING_identification_context__negotiation::clean_up()
{
  field_presentation__context__id.clean_up();
  field_transfer__syntax.clean_up();
}

void CHARACTER_STRING_identification_context__negotiation::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_VALUE, "record value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) return;
    if (2!=mp->get_size()) {
      param.error("record value of type CHARACTER STRING.identification.context-negotiation has 2 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) presentation__context__id().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) transfer__syntax().set_param(*mp->get_elem(1));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "presentation_context_id")) {
        presentation__context__id().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "transfer_syntax")) {
        transfer__syntax().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type CHARACTER STRING.identification.context-negotiation: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record value", "CHARACTER STRING.identification.context-negotiation");
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* CHARACTER_STRING_identification_context__negotiation::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  Module_Param* mp_field_presentation_context_id = field_presentation__context__id.get_param(param_name);
  mp_field_presentation_context_id->set_id(new Module_Param_FieldName(mcopystr("presentation_context_id")));
  Module_Param* mp_field_transfer_syntax = field_transfer__syntax.get_param(param_name);
  mp_field_transfer_syntax->set_id(new Module_Param_FieldName(mcopystr("transfer_syntax")));
  Module_Param_Assignment_List* mp = new Module_Param_Assignment_List();
  mp->add_elem(mp_field_presentation_context_id);
  mp->add_elem(mp_field_transfer_syntax);
  return mp;
}
#endif

void CHARACTER_STRING_identification_context__negotiation::encode_text(Text_Buf& text_buf) const
{
  field_presentation__context__id.encode_text(text_buf);
  field_transfer__syntax.encode_text(text_buf);
}

void CHARACTER_STRING_identification_context__negotiation::decode_text(Text_Buf& text_buf)
{
  field_presentation__context__id.decode_text(text_buf);
  field_transfer__syntax.decode_text(text_buf);
}

/* not called
void CHARACTER_STRING_identification_context__negotiation::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const
void CHARACTER_STRING_identification_context__negotiation::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)
*/

ASN_BER_TLV_t* CHARACTER_STRING_identification_context__negotiation::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(NULL);
  TTCN_EncDec_ErrorContext ec_0("Component '");
  TTCN_EncDec_ErrorContext ec_1;
  ec_1.set_msg("presentation_context_id': ");
  new_tlv->add_TLV(field_presentation__context__id.BER_encode_TLV(CHARACTER_STRING_identification_context__negotiation_presentation__context__id_descr_, p_coding));
  ec_1.set_msg("transfer_syntax': ");
  new_tlv->add_TLV(field_transfer__syntax.BER_encode_TLV(CHARACTER_STRING_identification_context__negotiation_transfer__syntax_descr_, p_coding));
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean CHARACTER_STRING_identification_context__negotiation::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form)
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec_0("While decoding 'CHARACTER STRING.identification.context-negotiation' type: ");
  stripped_tlv.chk_constructed_flag(TRUE);
  size_t V_pos=0;
  ASN_BER_TLV_t tmp_tlv;
  boolean tlv_present=FALSE;
  {
    TTCN_EncDec_ErrorContext ec_1("Component '");
    TTCN_EncDec_ErrorContext ec_2;
    ec_2.set_msg("presentation_context_id': ");
    if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
    if(!tlv_present) return FALSE;
    field_presentation__context__id.BER_decode_TLV(CHARACTER_STRING_identification_context__negotiation_presentation__context__id_descr_, tmp_tlv, L_form);
    tlv_present=FALSE;
    ec_2.set_msg("transfer_syntax': ");
    if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
    if(!tlv_present) return FALSE;
    field_transfer__syntax.BER_decode_TLV(CHARACTER_STRING_identification_context__negotiation_transfer__syntax_descr_, tmp_tlv, L_form);
    tlv_present=FALSE;
  }
  BER_decode_constdTLV_end(stripped_tlv, V_pos, L_form, tmp_tlv, tlv_present);
  return TRUE;
}

int CHARACTER_STRING_identification_context__negotiation::XER_encode(const XERdescriptor_t& p_td,
               TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const
{
  int encoded_length=(int)p_buf.get_len();

  begin_xml(p_td, p_buf, flavor, indent++, FALSE);

  field_presentation__context__id.XER_encode(CHARACTER_STRING_identification_pci_xer_, p_buf, flavor, flavor2, indent, 0);
  field_transfer__syntax         .XER_encode(CHARACTER_STRING_identification_ts_xer_ , p_buf, flavor, flavor2, indent, 0);

  end_xml(p_td, p_buf, flavor, --indent, FALSE);

  return p_buf.get_len() - encoded_length;
}

int CHARACTER_STRING_identification_context__negotiation::XER_decode(
  const XERdescriptor_t& p_td, XmlReaderWrap& reader, unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*)
{
  boolean exer  = is_exer(flavor);
  int type = reader.NodeType(), depth = -1;
  const char* name = (const char*)reader.Name();
  int success = reader.Ok();
  if (type==XML_READER_TYPE_ELEMENT && check_name(name, p_td, exer)) {
    // we are in the right place
    depth = reader.Depth();
    success = reader.Read();
  }
  field_presentation__context__id.XER_decode(EMBEDDED_PDV_identification_cn_pci_xer_, reader, flavor, flavor2, 0);
  field_transfer__syntax         .XER_decode(EMBEDDED_PDV_identification_cn_tsx_xer_, reader, flavor, flavor2, 0);
  for (; success == 1; success = reader.Read()) {
    type = reader.NodeType();
    if (XML_READER_TYPE_END_ELEMENT == type) {
      verify_end(reader, p_td, depth, exer);
      reader.Read();
      break;
    }
  }
  return 0; // TODO sensible return value
}

struct CHARACTER_STRING_identification_context__negotiation_template::single_value_struct {
  INTEGER_template field_presentation__context__id;
  OBJID_template field_transfer__syntax;
};

void CHARACTER_STRING_identification_context__negotiation_template::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_TEMPLATE, "record template");
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
    CHARACTER_STRING_identification_context__negotiation_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) break;
    if (2!=mp->get_size()) {
      param.error("record template of type CHARACTER STRING.identification.context-negotiation has 2 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) presentation__context__id().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) transfer__syntax().set_param(*mp->get_elem(1));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "presentation_context_id")) {
        presentation__context__id().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "transfer_syntax")) {
        transfer__syntax().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type CHARACTER STRING.identification.context-negotiation: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record template", "CHARACTER STRING.identification.context-negotiation");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* CHARACTER_STRING_identification_context__negotiation_template::get_param(Module_Param_Name& param_name) const
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
  case SPECIFIC_VALUE: {
    Module_Param* mp_field_presentation_context_id = single_value->field_presentation__context__id.get_param(param_name);
    mp_field_presentation_context_id->set_id(new Module_Param_FieldName(mcopystr("presentation_context_id")));
    Module_Param* mp_field_transfer_syntax = single_value->field_transfer__syntax.get_param(param_name);
    mp_field_transfer_syntax->set_id(new Module_Param_FieldName(mcopystr("transfer_syntax")));
    mp = new Module_Param_Assignment_List();
    mp->add_elem(mp_field_presentation_context_id);
    mp->add_elem(mp_field_transfer_syntax);
    break; }
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
    TTCN_error("Referencing an uninitialized/unsupported template of type CHARACTER STRING.identification.context-negotiation.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void CHARACTER_STRING_identification_context__negotiation_template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    delete single_value;
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void CHARACTER_STRING_identification_context__negotiation_template::set_specific()
{
  if (template_selection != SPECIFIC_VALUE) {
    template_sel old_selection = template_selection;
    clean_up();
    single_value = new single_value_struct;
    set_selection(SPECIFIC_VALUE);
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) {
      single_value->field_presentation__context__id = ANY_VALUE;
      single_value->field_transfer__syntax = ANY_VALUE;
    }
  }
}

void CHARACTER_STRING_identification_context__negotiation_template::copy_value(const CHARACTER_STRING_identification_context__negotiation& other_value)
{
  single_value = new single_value_struct;
  single_value->field_presentation__context__id = other_value.presentation__context__id();
  single_value->field_transfer__syntax = other_value.transfer__syntax();
  set_selection(SPECIFIC_VALUE);
}

void CHARACTER_STRING_identification_context__negotiation_template::copy_template(const CHARACTER_STRING_identification_context__negotiation_template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct(*other_value.single_value);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = new CHARACTER_STRING_identification_context__negotiation_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].copy_template(other_value.value_list.list_value[list_count]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported template of type CHARACTER STRING.identification.context-negotiation.");
  }
  set_selection(other_value);
}

CHARACTER_STRING_identification_context__negotiation_template::CHARACTER_STRING_identification_context__negotiation_template()
{
}

CHARACTER_STRING_identification_context__negotiation_template::CHARACTER_STRING_identification_context__negotiation_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

CHARACTER_STRING_identification_context__negotiation_template::CHARACTER_STRING_identification_context__negotiation_template(const CHARACTER_STRING_identification_context__negotiation& other_value)
{
  copy_value(other_value);
}

CHARACTER_STRING_identification_context__negotiation_template::CHARACTER_STRING_identification_context__negotiation_template(const OPTIONAL<CHARACTER_STRING_identification_context__negotiation>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const CHARACTER_STRING_identification_context__negotiation&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a template of type CHARACTER STRING.identification.context-negotiation from an unbound optional field.");
  }
}

CHARACTER_STRING_identification_context__negotiation_template::CHARACTER_STRING_identification_context__negotiation_template(const CHARACTER_STRING_identification_context__negotiation_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

CHARACTER_STRING_identification_context__negotiation_template::~CHARACTER_STRING_identification_context__negotiation_template()
{
  clean_up();
}

CHARACTER_STRING_identification_context__negotiation_template& CHARACTER_STRING_identification_context__negotiation_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

CHARACTER_STRING_identification_context__negotiation_template& CHARACTER_STRING_identification_context__negotiation_template::operator=(const CHARACTER_STRING_identification_context__negotiation& other_value)
{
  clean_up();
  copy_value(other_value);
  return *this;
}

CHARACTER_STRING_identification_context__negotiation_template& CHARACTER_STRING_identification_context__negotiation_template::operator=(const OPTIONAL<CHARACTER_STRING_identification_context__negotiation>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const CHARACTER_STRING_identification_context__negotiation&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a template of type CHARACTER STRING.identification.context-negotiation.");
  }
  return *this;
}

CHARACTER_STRING_identification_context__negotiation_template& CHARACTER_STRING_identification_context__negotiation_template::operator=(const CHARACTER_STRING_identification_context__negotiation_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean CHARACTER_STRING_identification_context__negotiation_template::match(const CHARACTER_STRING_identification_context__negotiation& other_value,
                                                                             boolean /* legacy */) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE:
    if (!other_value.presentation__context__id().is_bound()) return FALSE;
    if (!single_value->field_presentation__context__id.match(other_value.presentation__context__id())) return FALSE;
    if (!other_value.transfer__syntax().is_bound()) return FALSE;
    if (!single_value->field_transfer__syntax.match(other_value.transfer__syntax())) return FALSE;
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count].match(other_value)) return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching an uninitialized/unsupported template of type CHARACTER STRING.identification.context-negotiation.");
  }
  return FALSE;
}

CHARACTER_STRING_identification_context__negotiation CHARACTER_STRING_identification_context__negotiation_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of type CHARACTER STRING.identification.context-negotiation.");
  CHARACTER_STRING_identification_context__negotiation ret_val;
  ret_val.presentation__context__id() = single_value->field_presentation__context__id.valueof();
  ret_val.transfer__syntax() = single_value->field_transfer__syntax.valueof();
  return ret_val;
}

void CHARACTER_STRING_identification_context__negotiation_template::set_type(template_sel template_type, unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list for a template of type CHARACTER STRING.identification.context-negotiation.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new CHARACTER_STRING_identification_context__negotiation_template[list_length];
}

CHARACTER_STRING_identification_context__negotiation_template& CHARACTER_STRING_identification_context__negotiation_template::list_item(unsigned int list_index) const
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list template of type CHARACTER STRING.identification.context-negotiation.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a value list template of type CHARACTER STRING.identification.context-negotiation.");
  return value_list.list_value[list_index];
}

INTEGER_template& CHARACTER_STRING_identification_context__negotiation_template::presentation__context__id()
{
  set_specific();
  return single_value->field_presentation__context__id;
}

const INTEGER_template& CHARACTER_STRING_identification_context__negotiation_template::presentation__context__id() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field presentation_context_id of a non-specific template of type CHARACTER STRING.identification.context-negotiation.");
  return single_value->field_presentation__context__id;
}

OBJID_template& CHARACTER_STRING_identification_context__negotiation_template::transfer__syntax()
{
  set_specific();
  return single_value->field_transfer__syntax;
}

const OBJID_template& CHARACTER_STRING_identification_context__negotiation_template::transfer__syntax() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field transfer_syntax of a non-specific template of type CHARACTER STRING.identification.context-negotiation.");
  return single_value->field_transfer__syntax;
}

int CHARACTER_STRING_identification_context__negotiation_template::size_of() const
{
  switch (template_selection)
    {
    case SPECIFIC_VALUE:
      {
	int ret_val = 2;
	return ret_val;
      }
    case VALUE_LIST:
      {
	if (value_list.n_values<1)
	  TTCN_error("Internal error: Performing sizeof() operation on a template of type CHARACTER STRING.identification.context-negotiation containing an empty list.");
	int item_size = value_list.list_value[0].size_of();
	for (unsigned int i = 1; i < value_list.n_values; i++)
	  {
	    if (value_list.list_value[i].size_of()!=item_size)
	      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING.identification.context-negotiation containing a value list with different sizes.");
	  }
	return item_size;
      }
    case OMIT_VALUE:
      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING.identification.context-negotiation containing omit value.");
    case ANY_VALUE:
    case ANY_OR_OMIT:
      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING.identification.context-negotiation containing */? value.");
    case COMPLEMENTED_LIST:
      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING.identification.context-negotiation containing complemented list.");
    default:
      TTCN_error("Performing sizeof() operation on an uninitialized/unsupported template of type CHARACTER STRING.identification.context-negotiation.");
    }
  return 0;
}

void CHARACTER_STRING_identification_context__negotiation_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    TTCN_Logger::log_event_str("{ presentation_context_id := ");
    single_value->field_presentation__context__id.log();
    TTCN_Logger::log_event_str(", transfer_syntax := ");
    single_value->field_transfer__syntax.log();
    TTCN_Logger::log_event_str(" }");
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count].log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void CHARACTER_STRING_identification_context__negotiation_template::log_match(const CHARACTER_STRING_identification_context__negotiation& match_value,
                                                                              boolean /* legacy */) const
{
  if (template_selection == SPECIFIC_VALUE) {
    TTCN_Logger::log_event_str("{ presentation_context_id := ");
    single_value->field_presentation__context__id.log_match(match_value.presentation__context__id());
    TTCN_Logger::log_event_str(", transfer_syntax := ");
    single_value->field_transfer__syntax.log_match(match_value.transfer__syntax());
    TTCN_Logger::log_event_str(" }");
  } else {
    match_value.log();
    TTCN_Logger::log_event_str(" with ");
    log();
    if (match(match_value)) TTCN_Logger::log_event_str(" matched");
    else TTCN_Logger::log_event_str(" unmatched");
  }
}

void CHARACTER_STRING_identification_context__negotiation_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value->field_presentation__context__id.encode_text(text_buf);
    single_value->field_transfer__syntax.encode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported template of type CHARACTER STRING.identification.context-negotiation.");
  }
}

void CHARACTER_STRING_identification_context__negotiation_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct;
    single_value->field_presentation__context__id.decode_text(text_buf);
    single_value->field_transfer__syntax.decode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new CHARACTER_STRING_identification_context__negotiation_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received in a template of type CHARACTER STRING.identification.context-negotiation.");
  }
}

boolean CHARACTER_STRING_identification_context__negotiation_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean CHARACTER_STRING_identification_context__negotiation_template::match_omit(boolean legacy /* = FALSE */) const
{
  if (is_ifpresent) return TRUE;
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    if (legacy) {
      for (unsigned int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i].match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    } // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
void CHARACTER_STRING_identification_context__negotiation_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "CHARACTER STRING.identification.context-negotiation");
}
#endif

CHARACTER_STRING::CHARACTER_STRING()
{
}

CHARACTER_STRING::CHARACTER_STRING(const CHARACTER_STRING_identification& par_identification,
				   const OPTIONAL<UNIVERSAL_CHARSTRING>& par_data__value__descriptor,
				   const OCTETSTRING& par_string__value)
  : field_identification(par_identification),
    field_data__value__descriptor(par_data__value__descriptor),
    field_string__value(par_string__value)
{
}

boolean CHARACTER_STRING::operator==(const CHARACTER_STRING& other_value) const
{
  return field_identification==other_value.field_identification
    && field_data__value__descriptor==other_value.field_data__value__descriptor
    && field_string__value==other_value.field_string__value;
}

int CHARACTER_STRING::size_of() const
{
  int ret_val = 2;
  if (field_data__value__descriptor.ispresent()) ret_val++;
  return ret_val;
}

boolean CHARACTER_STRING::is_bound() const
{
  if(field_identification.is_bound()) return TRUE;
  if(OPTIONAL_OMIT == field_data__value__descriptor.get_selection() || field_data__value__descriptor.is_bound()) return TRUE;
  if(field_string__value.is_bound()) return TRUE;
  return FALSE;
}

boolean CHARACTER_STRING::is_value() const
{
  if(!field_identification.is_value()) return FALSE;
  if(OPTIONAL_OMIT != field_data__value__descriptor.get_selection() && !field_data__value__descriptor.is_value()) return FALSE;
  if(!field_string__value.is_value()) return FALSE;
  return TRUE;
}

void CHARACTER_STRING::clean_up()
{
  field_identification.clean_up();
  field_data__value__descriptor.clean_up();
  field_string__value.clean_up();
}

void CHARACTER_STRING::log() const
{
  TTCN_Logger::log_event_str("{ identification := ");
  field_identification.log();
  TTCN_Logger::log_event_str(", data_value_descriptor := ");
  field_data__value__descriptor.log();
  TTCN_Logger::log_event_str(", string_value := ");
  field_string__value.log();
  TTCN_Logger::log_event_str(" }");
}

void CHARACTER_STRING::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_VALUE, "record value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) return;
    if (3!=mp->get_size()) {
      param.error("record value of type CHARACTER STRING has 3 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) identification().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) data__value__descriptor().set_param(*mp->get_elem(1));
    if (mp->get_elem(2)->get_type()!=Module_Param::MP_NotUsed) string__value().set_param(*mp->get_elem(2));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "identification")) {
        identification().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "data_value_descriptor")) {
        data__value__descriptor().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "string_value")) {
        string__value().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type CHARACTER STRING: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record value", "CHARACTER STRING");
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* CHARACTER_STRING::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  Module_Param* mp_field_identification = field_identification.get_param(param_name);
  mp_field_identification->set_id(new Module_Param_FieldName(mcopystr("identification")));
  Module_Param* mp_field_data_value_descriptor = field_data__value__descriptor.get_param(param_name);
  mp_field_data_value_descriptor->set_id(new Module_Param_FieldName(mcopystr("data_value_descriptor")));
  Module_Param* mp_field_string_value = field_string__value.get_param(param_name);
  mp_field_string_value->set_id(new Module_Param_FieldName(mcopystr("string_value")));
  Module_Param_Assignment_List* mp = new Module_Param_Assignment_List();
  mp->add_elem(mp_field_identification);
  mp->add_elem(mp_field_data_value_descriptor);
  mp->add_elem(mp_field_string_value);
  return mp;
}
#endif

void CHARACTER_STRING::encode_text(Text_Buf& text_buf) const
{
  field_identification.encode_text(text_buf);
  field_data__value__descriptor.encode_text(text_buf);
  field_string__value.encode_text(text_buf);
}

void CHARACTER_STRING::decode_text(Text_Buf& text_buf)
{
  field_identification.decode_text(text_buf);
  field_data__value__descriptor.decode_text(text_buf);
  field_string__value.decode_text(text_buf);
}

void CHARACTER_STRING::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-encoding type '%s': ", p_td.name);
    unsigned BER_coding=va_arg(pvar, unsigned);
    BER_encode_chk_coding(BER_coding);
    ASN_BER_TLV_t *tlv=BER_encode_TLV(p_td, BER_coding);
    tlv->put_in_buffer(p_buf);
    ASN_BER_TLV_t::destruct(tlv);
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-encoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No RAW descriptor available for type '%s'.", p_td.name);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    TTCN_EncDec_ErrorContext ec("While TEXT-encoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No TEXT descriptor available for type '%s'.", p_td.name);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-encoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XER_encode(*p_td.xer,p_buf, XER_coding, 0, 0, 0);
    p_buf.put_c('\n');
    break;}
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-encoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No JSON descriptor available for type '%s'.", p_td.name);
    break;}
  default:
    TTCN_error("Unknown coding method requested to encode type '%s'", p_td.name);
  }
  va_end(pvar);
}

void CHARACTER_STRING::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-decoding type '%s': ", p_td.name);
    unsigned L_form=va_arg(pvar, unsigned);
    ASN_BER_TLV_t tlv;
    BER_decode_str2TLV(p_buf, tlv, L_form);
    BER_decode_TLV(p_td, tlv, L_form);
    if(tlv.isComplete) p_buf.increase_pos(tlv.get_len());
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-decoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No RAW descriptor available for type '%s'.", p_td.name);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    TTCN_EncDec_ErrorContext ec("While TEXT-decoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No TEXT descriptor available for type '%s'.", p_td.name);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-decoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XmlReaderWrap reader(p_buf);
    int success = reader.Read();
    for (; success==1; success=reader.Read()) {
      int type = reader.NodeType();
      if (type==XML_READER_TYPE_ELEMENT)
	break;
    }
    XER_decode(*p_td.xer, reader, XER_coding, XER_NONE, 0);
    size_t bytes = reader.ByteConsumed();
    p_buf.set_pos(bytes);
    break;}
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-decoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No JSON descriptor available for type '%s'.", p_td.name);
    break;}
  default:
    TTCN_error("Unknown coding method requested to decode type '%s'", p_td.name);
  }
  va_end(pvar);
}

ASN_BER_TLV_t* CHARACTER_STRING::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(NULL);
  TTCN_EncDec_ErrorContext ec_0("Component '");
  TTCN_EncDec_ErrorContext ec_1;
  ec_1.set_msg("identification': ");
  new_tlv->add_TLV(field_identification.BER_encode_TLV(CHARACTER_STRING_identification_descr_, p_coding));
  ec_1.set_msg("data_value_descriptor': ");
  new_tlv->add_TLV(field_data__value__descriptor.BER_encode_TLV(CHARACTER_STRING_data__value__descriptor_descr_, p_coding));
  ec_1.set_msg("string_value': ");
  new_tlv->add_TLV(field_string__value.BER_encode_TLV(CHARACTER_STRING_string__value_descr_, p_coding));
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean CHARACTER_STRING::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form)
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec_0("While decoding 'CHARACTER STRING' type: ");
  stripped_tlv.chk_constructed_flag(TRUE);
  size_t V_pos=0;
  ASN_BER_TLV_t tmp_tlv;
  boolean tlv_present=FALSE;
  {
    TTCN_EncDec_ErrorContext ec_1("Component '");
    TTCN_EncDec_ErrorContext ec_2;
    ec_2.set_msg("identification': ");
    if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
    if(!tlv_present) return FALSE;
    field_identification.BER_decode_TLV(CHARACTER_STRING_identification_descr_, tmp_tlv, L_form);
    tlv_present=FALSE;
    ec_2.set_msg("data_value_descriptor': ");
    if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
    if(!tlv_present) field_data__value__descriptor=OMIT_VALUE;
    else {
      field_data__value__descriptor.BER_decode_TLV(CHARACTER_STRING_data__value__descriptor_descr_, tmp_tlv, L_form);
      if(field_data__value__descriptor.ispresent()) tlv_present=FALSE;
    }
    ec_2.set_msg("string_value': ");
    if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
    if(!tlv_present) return FALSE;
    field_string__value.BER_decode_TLV(CHARACTER_STRING_string__value_descr_, tmp_tlv, L_form);
    tlv_present=FALSE;
  }
  BER_decode_constdTLV_end(stripped_tlv, V_pos, L_form, tmp_tlv, tlv_present);
  return TRUE;
}

int CHARACTER_STRING::XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
  unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const
{
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }
  int encoded_length=(int)p_buf.get_len();

  flavor &= ~XER_RECOF; // ASN.1 character string doesn't care
  begin_xml(p_td, p_buf, flavor, indent++, FALSE);

  field_identification.XER_encode(CHARACTER_STRING_identification_xer_, p_buf, flavor, flavor2, indent, 0);
  // data-value-descriptor is OPTIONAL and can never be present.
  // Its encoding is empty.
  field_string__value .XER_encode(CHARACTER_STRING_data_value_xer_    , p_buf, flavor, flavor2, indent, 0);

  end_xml(p_td, p_buf, flavor, --indent, FALSE);

  return (int)p_buf.get_len() - encoded_length;
}

int CHARACTER_STRING::XER_decode(const XERdescriptor_t& p_td,
  XmlReaderWrap& reader, unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*)
{
  boolean exer  = is_exer(flavor);
  int success = reader.Ok(), depth = -1, type;
  for (; success == 1; success = reader.Read()) {
    type = reader.NodeType();
    if (XML_READER_TYPE_ELEMENT == type) {
      // If our parent is optional and there is an unexpected tag then return and
      // we stay unbound.
      if ((flavor & XER_OPTIONAL) && !check_name((const char*)reader.LocalName(), p_td, exer)) {
        return -1;
      }
      verify_name(reader, p_td, exer);
      depth = reader.Depth();
      reader.Read();
      break;
    }
  }
  field_identification         .XER_decode(CHARACTER_STRING_identification_xer_       , reader, flavor, flavor2, 0);
  field_data__value__descriptor.XER_decode(CHARACTER_STRING_data_value_descriptor_xer_, reader, flavor, flavor2, 0);
  if (field_data__value__descriptor.is_value()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
      "data-value-descriptor not allowed for EMBEDDED PDV");
  }
  field_string__value          .XER_decode(CHARACTER_STRING_data_value_xer_           , reader, flavor, flavor2, 0);
  for (success = reader.Read(); success == 1; success = reader.Read()) {
    type = reader.NodeType();
    if (XML_READER_TYPE_END_ELEMENT == type) {
      verify_end(reader, p_td, depth, exer);
      reader.Read();
      break;
    }
  }
  return 1;

}


struct CHARACTER_STRING_template::single_value_struct {
  CHARACTER_STRING_identification_template field_identification;
  UNIVERSAL_CHARSTRING_template field_data__value__descriptor;
  OCTETSTRING_template field_string__value;
};

void CHARACTER_STRING_template::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_TEMPLATE, "record template");
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
    CHARACTER_STRING_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) break;
    if (3!=mp->get_size()) {
      param.error("record template of type CHARACTER STRING has 3 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) identification().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) data__value__descriptor().set_param(*mp->get_elem(1));
    if (mp->get_elem(2)->get_type()!=Module_Param::MP_NotUsed) string__value().set_param(*mp->get_elem(2));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "identification")) {
        identification().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "data_value_descriptor")) {
        data__value__descriptor().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "string_value")) {
        string__value().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type CHARACTER STRING: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record template", "CHARACTER STRING");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* CHARACTER_STRING_template::get_param(Module_Param_Name& param_name) const
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
  case SPECIFIC_VALUE: {
    Module_Param* mp_field_identification = single_value->field_identification.get_param(param_name);
    mp_field_identification->set_id(new Module_Param_FieldName(mcopystr("identification")));
    Module_Param* mp_field_data_value_descriptor = single_value->field_data__value__descriptor.get_param(param_name);
    mp_field_data_value_descriptor->set_id(new Module_Param_FieldName(mcopystr("data_value_descriptor")));
    Module_Param* mp_field_string_value = single_value->field_string__value.get_param(param_name);
    mp_field_string_value->set_id(new Module_Param_FieldName(mcopystr("string_value")));
    mp = new Module_Param_Assignment_List();
    mp->add_elem(mp_field_identification);
    mp->add_elem(mp_field_data_value_descriptor);
    mp->add_elem(mp_field_string_value);
    break; }
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
    TTCN_error("Referencing an uninitialized/unsupported template of type CHARACTER STRING.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void CHARACTER_STRING_template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    delete single_value;
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void CHARACTER_STRING_template::set_specific()
{
  if (template_selection != SPECIFIC_VALUE) {
    template_sel old_selection = template_selection;
    clean_up();
    single_value = new single_value_struct;
    set_selection(SPECIFIC_VALUE);
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) {
      single_value->field_identification = ANY_VALUE;
      single_value->field_data__value__descriptor = ANY_OR_OMIT;
      single_value->field_string__value = ANY_VALUE;
    }
  }
}

void CHARACTER_STRING_template::copy_value(const CHARACTER_STRING& other_value)
{
  single_value = new single_value_struct;
  single_value->field_identification = other_value.identification();
  if (other_value.data__value__descriptor().ispresent()) single_value->field_data__value__descriptor = (const UNIVERSAL_CHARSTRING&)(other_value.data__value__descriptor());
  else single_value->field_data__value__descriptor = OMIT_VALUE;
  single_value->field_string__value = other_value.string__value();
  set_selection(SPECIFIC_VALUE);
}

void CHARACTER_STRING_template::copy_template(const CHARACTER_STRING_template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct(*other_value.single_value);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = new CHARACTER_STRING_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].copy_template(other_value.value_list.list_value[list_count]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported template of type CHARACTER STRING.");
  }
  set_selection(other_value);
}

CHARACTER_STRING_template::CHARACTER_STRING_template()
{
}

CHARACTER_STRING_template::CHARACTER_STRING_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

CHARACTER_STRING_template::CHARACTER_STRING_template(const CHARACTER_STRING& other_value)
{
  copy_value(other_value);
}

CHARACTER_STRING_template::CHARACTER_STRING_template(const OPTIONAL<CHARACTER_STRING>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const CHARACTER_STRING&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a template of type CHARACTER STRING from an unbound optional field.");
  }
}

CHARACTER_STRING_template::CHARACTER_STRING_template(const CHARACTER_STRING_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

CHARACTER_STRING_template::~CHARACTER_STRING_template()
{
  clean_up();
}

CHARACTER_STRING_template& CHARACTER_STRING_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

CHARACTER_STRING_template& CHARACTER_STRING_template::operator=(const CHARACTER_STRING& other_value)
{
  clean_up();
  copy_value(other_value);
  return *this;
}

CHARACTER_STRING_template& CHARACTER_STRING_template::operator=(const OPTIONAL<CHARACTER_STRING>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const CHARACTER_STRING&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a template of type CHARACTER STRING.");
  }
  return *this;
}

CHARACTER_STRING_template& CHARACTER_STRING_template::operator=(const CHARACTER_STRING_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean CHARACTER_STRING_template::match(const CHARACTER_STRING& other_value,
                                         boolean /* legacy */) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE:
    if (!other_value.identification().is_bound()) return FALSE;
    if (!single_value->field_identification.match(other_value.identification())) return FALSE;
    if (!other_value.data__value__descriptor().is_bound()) return FALSE;
    if (other_value.data__value__descriptor().ispresent() ? !single_value->field_data__value__descriptor.match((const UNIVERSAL_CHARSTRING&)other_value.data__value__descriptor()) : !single_value->field_data__value__descriptor.match_omit()) return FALSE;
    if (!other_value.string__value().is_bound()) return FALSE;
    if (!single_value->field_string__value.match(other_value.string__value())) return FALSE;
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count].match(other_value)) return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching an uninitialized/unsupported template of type CHARACTER STRING.");
  }
  return FALSE;
}

CHARACTER_STRING CHARACTER_STRING_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of type CHARACTER STRING.");
  CHARACTER_STRING ret_val;
  ret_val.identification() = single_value->field_identification.valueof();
  if (single_value->field_data__value__descriptor.is_omit()) ret_val.data__value__descriptor() = OMIT_VALUE;
  else ret_val.data__value__descriptor() = single_value->field_data__value__descriptor.valueof();
  ret_val.string__value() = single_value->field_string__value.valueof();
  return ret_val;
}

void CHARACTER_STRING_template::set_type(template_sel template_type, unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list for a template of type CHARACTER STRING.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new CHARACTER_STRING_template[list_length];
}

CHARACTER_STRING_template& CHARACTER_STRING_template::list_item(unsigned int list_index) const
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list template of type CHARACTER STRING.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a value list template of type CHARACTER STRING.");
  return value_list.list_value[list_index];
}

CHARACTER_STRING_identification_template& CHARACTER_STRING_template::identification()
{
  set_specific();
  return single_value->field_identification;
}

const CHARACTER_STRING_identification_template& CHARACTER_STRING_template::identification() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field identification of a non-specific template of type CHARACTER STRING.");
  return single_value->field_identification;
}

UNIVERSAL_CHARSTRING_template& CHARACTER_STRING_template::data__value__descriptor()
{
  set_specific();
  return single_value->field_data__value__descriptor;
}

const UNIVERSAL_CHARSTRING_template& CHARACTER_STRING_template::data__value__descriptor() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field data_value_descriptor of a non-specific template of type CHARACTER STRING.");
  return single_value->field_data__value__descriptor;
}

OCTETSTRING_template& CHARACTER_STRING_template::string__value()
{
  set_specific();
  return single_value->field_string__value;
}

const OCTETSTRING_template& CHARACTER_STRING_template::string__value() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field string_value of a non-specific template of type CHARACTER STRING.");
  return single_value->field_string__value;
}

int CHARACTER_STRING_template::size_of() const
{
  switch (template_selection)
    {
    case SPECIFIC_VALUE:
      {
	int ret_val = 2;
	if (single_value->field_data__value__descriptor.is_present()) ret_val++;
	return ret_val;
      }
    case VALUE_LIST:
      {
	if (value_list.n_values<1)
	  TTCN_error("Internal error: Performing sizeof() operation on a template of type CHARACTER STRING containing an empty list.");
	int item_size = value_list.list_value[0].size_of();
	for (unsigned int i = 1; i < value_list.n_values; i++)
	  {
	    if (value_list.list_value[i].size_of()!=item_size)
	      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING containing a value list with different sizes.");
	  }
	return item_size;
      }
    case OMIT_VALUE:
      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING containing omit value.");
    case ANY_VALUE:
    case ANY_OR_OMIT:
      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING containing */? value.");
    case COMPLEMENTED_LIST:
      TTCN_error("Performing sizeof() operation on a template of type CHARACTER STRING containing complemented list.");
    default:
      TTCN_error("Performing sizeof() operation on an uninitialized/unsupported template of type CHARACTER STRING.");
    }
  return 0;
}

void CHARACTER_STRING_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    TTCN_Logger::log_event_str("{ identification := ");
    single_value->field_identification.log();
    TTCN_Logger::log_event_str(", data_value_descriptor := ");
    single_value->field_data__value__descriptor.log();
    TTCN_Logger::log_event_str(", string_value := ");
    single_value->field_string__value.log();
    TTCN_Logger::log_event_str(" }");
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count].log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void CHARACTER_STRING_template::log_match(const CHARACTER_STRING& match_value,
                                          boolean /* legacy */) const
{
  if (template_selection == SPECIFIC_VALUE) {
    TTCN_Logger::log_event_str("{ identification := ");
    single_value->field_identification.log_match(match_value.identification());
    TTCN_Logger::log_event_str(", data_value_descriptor := ");
    if (match_value.data__value__descriptor().ispresent()) single_value->field_data__value__descriptor.log_match(match_value.data__value__descriptor());
    else {
      single_value->field_data__value__descriptor.log();
      if (single_value->field_data__value__descriptor.match_omit()) TTCN_Logger::log_event_str(" matched");
      else TTCN_Logger::log_event_str(" unmatched");
    }
    TTCN_Logger::log_event_str(", string_value := ");
    single_value->field_string__value.log_match(match_value.string__value());
    TTCN_Logger::log_event_str(" }");
  } else {
    match_value.log();
    TTCN_Logger::log_event_str(" with ");
    log();
    if (match(match_value)) TTCN_Logger::log_event_str(" matched");
    else TTCN_Logger::log_event_str(" unmatched");
  }
}

void CHARACTER_STRING_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value->field_identification.encode_text(text_buf);
    single_value->field_data__value__descriptor.encode_text(text_buf);
    single_value->field_string__value.encode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported template of type CHARACTER STRING.");
  }
}

void CHARACTER_STRING_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct;
    single_value->field_identification.decode_text(text_buf);
    single_value->field_data__value__descriptor.decode_text(text_buf);
    single_value->field_string__value.decode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new CHARACTER_STRING_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received in a template of type CHARACTER STRING.");
  }
}

boolean CHARACTER_STRING_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean CHARACTER_STRING_template::match_omit(boolean legacy /* = FALSE */) const
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
void CHARACTER_STRING_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "CHARACTER STRING");
}
#endif
