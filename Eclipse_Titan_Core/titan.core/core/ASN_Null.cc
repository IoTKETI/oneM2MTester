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
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include <stdarg.h>

#include "ASN_Null.hh"
#include "Parameters.h"
#include "Param_Types.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Encdec.hh"
#include "BER.hh"

#include "../common/dbgnew.hh"

ASN_NULL::ASN_NULL()
{
  bound_flag = FALSE;
}

ASN_NULL::ASN_NULL(asn_null_type)
{
  bound_flag = TRUE;
}

ASN_NULL::ASN_NULL(const ASN_NULL& other_value)
: Base_Type(other_value)
{
  if (!other_value.bound_flag)
    TTCN_error("Copying an unbound ASN.1 NULL value.");
  bound_flag = TRUE;
}

ASN_NULL& ASN_NULL::operator=(asn_null_type)
{
  bound_flag = TRUE;
  return *this;
}

ASN_NULL& ASN_NULL::operator=(const ASN_NULL& other_value)
{
  if (!other_value.bound_flag)
    TTCN_error("Assignment of an unbound ASN.1 NULL value.");
  bound_flag = TRUE;
  return *this;
}

boolean ASN_NULL::operator==(asn_null_type) const
{
  if (!bound_flag) TTCN_error("The left operand of comparison is an unbound "
    "ASN.1 NULL value.");
  return TRUE;
}

boolean ASN_NULL::operator==(const ASN_NULL& other_value) const
{
  if (!bound_flag) TTCN_error("The left operand of comparison is an unbound "
    "ASN.1 NULL value.");
  if (!other_value.bound_flag) TTCN_error("The right operand of comparison "
    "is an unbound ASN.1 NULL value.");
  return TRUE;
}

void ASN_NULL::log() const
{
  if (bound_flag) TTCN_Logger::log_event_str("NULL");
  else TTCN_Logger::log_event_unbound();
}

void ASN_NULL::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE, "NULL value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  if (mp->get_type()!=Module_Param::MP_Asn_Null) param.type_error("NULL value");
  bound_flag = TRUE;
}

#ifdef TITAN_RUNTIME_2
Module_Param* ASN_NULL::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  return new Module_Param_Asn_Null();
}
#endif

void ASN_NULL::encode_text(Text_Buf&) const
{
  if (!bound_flag)
    TTCN_error("Text encoder: Encoding an unbound ASN.1 NULL value.");
}

void ASN_NULL::decode_text(Text_Buf&)
{
  bound_flag = TRUE;
}

void ASN_NULL::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
                      TTCN_EncDec::coding_t p_coding, ...) const
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
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-encoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XER_encode(*p_td.xer, p_buf, XER_coding, 0, 0, 0);
    break;}
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-encoding type '%s': ", p_td.name);
    if(!p_td.json)
      TTCN_EncDec_ErrorContext::error_internal
        ("No JSON descriptor available for type '%s'.", p_td.name);
    JSON_Tokenizer tok(va_arg(pvar, int) != 0);
    JSON_encode(p_td, tok);
    p_buf.put_s(tok.get_buffer_length(), (const unsigned char*)tok.get_buffer());
    break;}
  case TTCN_EncDec::CT_RAW:
  default:
    TTCN_error("Unknown coding method requested to encode type '%s'",
               p_td.name);
  }
  va_end(pvar);
}

void ASN_NULL::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
                      TTCN_EncDec::coding_t p_coding, ...)
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
    if(!p_td.json)
      TTCN_EncDec_ErrorContext::error_internal
        ("No JSON descriptor available for type '%s'.", p_td.name);
    JSON_Tokenizer tok((const char*)p_buf.get_data(), p_buf.get_len());
    if(JSON_decode(p_td, tok, FALSE)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
               "Can not decode type '%s', because invalid or incomplete"
               " message was received"
               , p_td.name);
    p_buf.set_pos(tok.get_buf_pos());
    break;}
  case TTCN_EncDec::CT_RAW:
  default:
    TTCN_error("Unknown coding method requested to decode type '%s'",
               p_td.name);
  }
  va_end(pvar);
}

ASN_BER_TLV_t*
ASN_NULL::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                         unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(!new_tlv) {
    new_tlv=ASN_BER_TLV_t::construct(0, NULL);
  }
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean ASN_NULL::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                                 const ASN_BER_TLV_t& p_tlv,
                                 unsigned L_form)
{
  bound_flag = FALSE;
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec("While decoding NULL type: ");
  stripped_tlv.chk_constructed_flag(FALSE);
  if(!stripped_tlv.V_tlvs_selected && stripped_tlv.V.str.Vlen!=0)
    ec.error(TTCN_EncDec::ET_INVAL_MSG, "Length of V-part is not 0.");
  bound_flag=TRUE;
  return TRUE;
}

int ASN_NULL::XER_encode(const XERdescriptor_t& p_td,
    TTCN_Buffer& p_buf, unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  boolean exer  = is_exer(flavor);
  TTCN_EncDec_ErrorContext ec("While XER encoding NULL type: ");
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error
    (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound ASN.1 NULL value.");
  }

  int indenting = !is_canonical(flavor) && !is_record_of(flavor);
  int encoded_length=(int)p_buf.get_len();

  if (indenting) do_indent(p_buf, indent);
  p_buf.put_c('<');

  // empty element tag
  if (exer) write_ns_prefix(p_td, p_buf);
  p_buf.put_s((size_t)p_td.namelens[exer]-2, (const unsigned char*)p_td.names[exer]);

  p_buf.put_s(2 + indenting , (const unsigned char*)"/>\n");
  return (int)p_buf.get_len() - encoded_length;
}

int ASN_NULL::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
    unsigned int flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*)
{
  boolean exer  = is_exer(flavor);
  TTCN_EncDec_ErrorContext ec("While XER decoding NULL type: ");
  int success = reader.Ok(), depth = -1;
  for (; success == 1; success = reader.Read()) {
    int type = reader.NodeType();
    if (XML_READER_TYPE_ELEMENT == type) {
      // If our parent is optional and there is an unexpected tag then return and
      // we stay unbound.
      if ((flavor & XER_OPTIONAL) && !check_name((const char*)reader.LocalName(), p_td, exer)) {
        return -1;
      }
      verify_name(reader, p_td, exer);
      depth = reader.Depth();
      break;
    }
  }
  bound_flag = TRUE;
  int gol = reader.IsEmptyElement();
  if (!gol) { // shouldn't happen
    for (success = reader.Read(); success == 1; success = reader.Read()) {
      int type = reader.NodeType();
      if (XML_READER_TYPE_END_ELEMENT == type) {
	verify_end(reader, p_td, depth, exer);
	// FIXME reader.Read() ??
	break;
      }
    } // next
  } // if gol

  reader.Read();
  return 1; // decode successful
}

int ASN_NULL::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound ASN.1 NULL value.");
    return -1;
  }
  
  return p_tok.put_next_token(JSON_TOKEN_LITERAL_NULL);
}

int ASN_NULL::JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok, boolean p_silent)
{
  json_token_t token = JSON_TOKEN_NONE;
  size_t dec_len = p_tok.get_next_token(&token, NULL, NULL);
  if (JSON_TOKEN_ERROR == token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  else if (JSON_TOKEN_LITERAL_NULL != token) {
    return JSON_ERROR_INVALID_TOKEN;
  }
  bound_flag = TRUE;
  return (int)dec_len;
}

boolean operator==(asn_null_type, const ASN_NULL& other_value)
{
  if (!other_value.is_bound()) TTCN_error("The right operand of comparison "
    "is an unbound ASN.1 NULL value.");
  return TRUE;
}

void ASN_NULL_template::clean_up()
{
  if (template_selection == VALUE_LIST ||
    template_selection == COMPLEMENTED_LIST) delete [] value_list.list_value;
  template_selection = UNINITIALIZED_TEMPLATE;
}

void ASN_NULL_template::copy_template(const ASN_NULL_template& other_value)
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
    value_list.list_value = new ASN_NULL_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
	other_value.value_list.list_value[i]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported template of ASN.1 "
      "NULL type.");
  }
  set_selection(other_value);
}

ASN_NULL_template::ASN_NULL_template()
{

}

ASN_NULL_template::ASN_NULL_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

ASN_NULL_template::ASN_NULL_template(asn_null_type)
  : Base_Template(SPECIFIC_VALUE)
{

}

ASN_NULL_template::ASN_NULL_template(const ASN_NULL& other_value)
  : Base_Template(SPECIFIC_VALUE)
{
  if (!other_value.is_bound())
    TTCN_error("Creating a template from an unbound ASN.1 NULL value.");
}

ASN_NULL_template::ASN_NULL_template(const OPTIONAL<ASN_NULL>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a template of ASN.1 NULL type from an unbound "
      "optional field.");
  }
}

ASN_NULL_template::ASN_NULL_template(const ASN_NULL_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

ASN_NULL_template::~ASN_NULL_template()
{
  clean_up();
}

ASN_NULL_template& ASN_NULL_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

ASN_NULL_template& ASN_NULL_template::operator=(asn_null_type)
{
  clean_up();
  set_selection(SPECIFIC_VALUE);
  return *this;
}

ASN_NULL_template& ASN_NULL_template::operator=(const ASN_NULL& other_value)
{
  if (!other_value.is_bound()) TTCN_error("Assignment of an unbound ASN.1 "
    "NULL value to a template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  return *this;
}

ASN_NULL_template& ASN_NULL_template::operator=
  (const OPTIONAL<ASN_NULL>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a template of "
      "ASN.1 NULL type.");
  }
  return *this;
}

ASN_NULL_template& ASN_NULL_template::operator=
  (const ASN_NULL_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean ASN_NULL_template::match(asn_null_type other_value,
                                 boolean /* legacy */) const
{
  switch (template_selection) {
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE:
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
    TTCN_error("Matching with an uninitialized/unsupported template of "
      "ASN.1 NULL type.");
  }
  return FALSE;
}

boolean ASN_NULL_template::match(const ASN_NULL& other_value,
                                 boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  return match(ASN_NULL_VALUE);
}

asn_null_type ASN_NULL_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof "
    "or send operation on a non-specific template of ASN.1 NULL type.");
  return ASN_NULL_VALUE;
}

void ASN_NULL_template::set_type(template_sel template_type,
   unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list type for a template of ASN.1 NULL "
      "type.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new ASN_NULL_template[list_length];
}

ASN_NULL_template& ASN_NULL_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
    template_selection != COMPLEMENTED_LIST) TTCN_error("Accessing a list "
      "element of a non-list template for ASN.1 NULL type.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a value list template of ASN.1 NULL type.");
  return value_list.list_value[list_index];
}

void ASN_NULL_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    TTCN_Logger::log_event_str("NULL");
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
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
  }
  log_ifpresent();
}

void ASN_NULL_template::log_match(const ASN_NULL& match_value,
                                  boolean /* legacy */) const
{
  if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
    TTCN_Logger::print_logmatch_buffer();
    TTCN_Logger::log_event_str(" := ");
  }
  match_value.log();
  TTCN_Logger::log_event_str(" with ");
  log();
  if (match(match_value)) TTCN_Logger::log_event_str(" matched");
  else TTCN_Logger::log_event_str(" unmatched");
}

void ASN_NULL_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE, "NULL template");
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
    ASN_NULL_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Asn_Null:
    *this = ASN_NULL_VALUE;
    break;
  default:
    param.type_error("NULL template");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* ASN_NULL_template::get_param(Module_Param_Name& param_name) const
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
    mp = new Module_Param_Asn_Null();
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
    TTCN_error("Referencing an uninitialized/unsupported ASN.1 NULL template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void ASN_NULL_template::encode_text(Text_Buf& text_buf) const
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
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an undefined/unsupported template "
      "of ASN.1 NULL type.");
  }
}

void ASN_NULL_template::decode_text(Text_Buf& text_buf)
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
    value_list.list_value = new ASN_NULL_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received "
      "in a template for ASN.1 NULL type.");
  }
}

boolean ASN_NULL_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean ASN_NULL_template::match_omit(boolean legacy /* = FALSE */) const
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
void ASN_NULL_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "ASN.1 NULL");
}
#endif
