/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
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
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include "Verdicttype.hh"

#include "Param_Types.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Textbuf.hh"
#include "Optional.hh"

#include "../common/dbgnew.hh"

#define UNBOUND_VERDICT ((verdicttype)(ERROR + 1))
#define IS_VALID(verdict_value) (verdict_value >= NONE && verdict_value <= ERROR)

const char * const verdict_name[5] = { "none", "pass", "inconc", "fail", "error" };

VERDICTTYPE::VERDICTTYPE()
{
  verdict_value = UNBOUND_VERDICT;
}

VERDICTTYPE::VERDICTTYPE(verdicttype other_value)
{
  if (!IS_VALID(other_value)) TTCN_error("Initializing a verdict variable "
    "with an invalid value (%d).", other_value);
  verdict_value = other_value;
}

VERDICTTYPE::VERDICTTYPE(const VERDICTTYPE& other_value)
: Base_Type(other_value)
{
  if (!other_value.is_bound()) TTCN_error("Copying an unbound verdict value.");
  verdict_value = other_value.verdict_value;
}

VERDICTTYPE& VERDICTTYPE::operator=(verdicttype other_value)
{
  if (!IS_VALID(other_value))
    TTCN_error("Assignment of an invalid verdict value (%d).", other_value);
  verdict_value = other_value;
  return *this;
}

VERDICTTYPE& VERDICTTYPE::operator=(const VERDICTTYPE& other_value)
{
  if (!other_value.is_bound())
    TTCN_error("Assignment of an unbound verdict value.");
  verdict_value = other_value.verdict_value;
  return *this;
}

boolean VERDICTTYPE::operator==(verdicttype other_value) const
{
  if (!is_bound()) TTCN_error("The left operand of comparison is an unbound "
    "verdict value.");
  if (!IS_VALID(other_value)) TTCN_error("The right operand of comparison is "
    "an invalid verdict value (%d).", other_value);
  return verdict_value == other_value;
}

boolean VERDICTTYPE::operator==(const VERDICTTYPE& other_value) const
{
  if (!is_bound()) TTCN_error("The left operand of comparison is an unbound "
    "verdict value.");
  if (!other_value.is_bound()) TTCN_error("The right operand of comparison is "
    "an unbound verdict value.");
  return verdict_value == other_value.verdict_value;
}

VERDICTTYPE::operator verdicttype() const
{
  if (!is_bound())
    TTCN_error("Using the value of an unbound verdict variable.");
  return verdict_value;
}

void VERDICTTYPE::clean_up()
{
  verdict_value = UNBOUND_VERDICT;
}

void VERDICTTYPE::log() const
{
  if (IS_VALID(verdict_value))
    TTCN_Logger::log_event_str(verdict_name[verdict_value]);
  else if (verdict_value == UNBOUND_VERDICT)
    TTCN_Logger::log_event_unbound();
  else TTCN_Logger::log_event("<invalid verdict value: %d>", verdict_value);
}

void VERDICTTYPE::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE, "verdict value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  if (mp->get_type()!=Module_Param::MP_Verdict) param.type_error("verdict value");
  const verdicttype verdict = mp->get_verdict();
  if (!IS_VALID(verdict)) param.error("Internal error: invalid verdict value (%d).", verdict);
  verdict_value = verdict;
}

#ifdef TITAN_RUNTIME_2
Module_Param* VERDICTTYPE::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  return new Module_Param_Verdict(verdict_value);
}
#endif

void VERDICTTYPE::encode_text(Text_Buf& text_buf) const
{
  if (!is_bound())
    TTCN_error("Text encoder: Encoding an unbound verdict value.");
  text_buf.push_int(verdict_value);
}

void VERDICTTYPE::decode_text(Text_Buf& text_buf)
{
  int received_value = text_buf.pull_int().get_val();
  if (!IS_VALID(received_value)) TTCN_error("Text decoder: Invalid verdict "
    "value (%d) was received.", received_value);
  verdict_value = (verdicttype)received_value;
}

void VERDICTTYPE::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
                     TTCN_EncDec::coding_t p_coding, ...) const
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
#if 0
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
    if(!p_td.raw)
    TTCN_EncDec_ErrorContext::error_internal
      ("No RAW descriptor available for type '%s'.", p_td.name);
    RAW_enc_tr_pos rp;
    rp.level=0;
    rp.pos=NULL;
    RAW_enc_tree root(TRUE,NULL,&rp,1,p_td.raw);
    RAW_encode(p_td, root);
    root.put_to_buf(p_buf);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    TTCN_EncDec_ErrorContext ec("While TEXT-encoding type '%s': ", p_td.name);
    if(!p_td.text)
      TTCN_EncDec_ErrorContext::error_internal
        ("No TEXT descriptor available for type '%s'.", p_td.name);
    TEXT_encode(p_td,p_buf);
    break;}
#endif
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
  default:
    TTCN_error("Unknown coding method requested to encode type '%s'",
               p_td.name);
  }
  va_end(pvar);
}

void VERDICTTYPE::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
                     TTCN_EncDec::coding_t p_coding, ...)
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
#if 0
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-decoding type '%s': ", p_td.name);
    if(!p_td.raw)
      TTCN_EncDec_ErrorContext::error_internal
        ("No RAW descriptor available for type '%s'.", p_td.name);
    raw_order_t order;
    switch(p_td.raw->top_bit_order){
    case TOP_BIT_LEFT:
      order=ORDER_LSB;
      break;
    case TOP_BIT_RIGHT:
    default:
      order=ORDER_MSB;
    }
    if(RAW_decode(p_td, p_buf, p_buf.get_len()*8, order)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
        "Can not decode type '%s', because invalid or incomplete"
        " message was received"
        , p_td.name);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    Limit_Token_List limit;
    TTCN_EncDec_ErrorContext ec("While TEXT-decoding type '%s': ", p_td.name);
    if(!p_td.text)
      TTCN_EncDec_ErrorContext::error_internal
        ("No TEXT descriptor available for type '%s'.", p_td.name);
    const unsigned char *b=p_buf.get_data();
    if(b[p_buf.get_len()-1]!='\0'){
      p_buf.set_pos(p_buf.get_len());
      p_buf.put_zero(8,ORDER_LSB);
      p_buf.rewind();
    }
    if(TEXT_decode(p_td,p_buf,limit)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
               "Can not decode type '%s', because invalid or incomplete"
               " message was received"
               , p_td.name);
    break;}
#endif
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-decoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XmlReaderWrap reader(p_buf);
    for (int success = reader.Read(); success==1; success=reader.Read()) {
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
  default:
    TTCN_error("Unknown coding method requested to decode type '%s'",
               p_td.name);
  }
  va_end(pvar);
}


int VERDICTTYPE::XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
  unsigned int p_flavor, unsigned int /*p_flavor2*/, int p_indent, embed_values_enc_struct_t*) const
{
  int encoded_length=(int)p_buf.get_len();
  //const boolean e_xer = is_exer(p_flavor);
  p_flavor |= (SIMPLE_TYPE | BXER_EMPTY_ELEM);
  if (begin_xml(p_td, p_buf, p_flavor, p_indent, FALSE) == -1) --encoded_length;
  //if (!e_xer) p_buf.put_c('<');
  {
    const char * enumval = verdict_name[verdict_value];
    p_buf.put_s(strlen(enumval), (const unsigned char*)enumval);
  }
  //if (!e_xer) p_buf.put_s(2, (const unsigned char*)"/>");
  end_xml(p_td, p_buf, p_flavor, p_indent, FALSE);
  return (int)p_buf.get_len() - encoded_length;
}

verdicttype VERDICTTYPE::str_to_verdict(const char *v, boolean silent)
{
  for (int i = NONE; i <= ERROR; ++i) {
    if (0 == strcmp(v, verdict_name[i])) {
      return (verdicttype)i;
    }
  }
  
  if (!silent) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
      "Invalid value for verdicttype: '%s'", v);
  }
  return UNBOUND_VERDICT;
}

int VERDICTTYPE::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& p_reader,
  unsigned int p_flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*)
{
  const boolean e_xer = is_exer(p_flavor);
  if (e_xer && ((p_td.xer_bits & XER_ATTRIBUTE) || is_exerlist(p_flavor))) {
    if ((p_td.xer_bits & XER_ATTRIBUTE)) verify_name(p_reader, p_td, e_xer);
    const char * value = (const char *)p_reader.Value();
    if (value) {
        verdict_value = str_to_verdict(value, (p_flavor & EXIT_ON_ERROR) ? TRUE : FALSE);
    }
  }
  else {
    int rd_ok = 1, type;
    const boolean name_tag = !((!e_xer && is_record_of(p_flavor)) || (e_xer && ((p_td.xer_bits & UNTAGGED) ||(is_record_of(p_flavor) && is_exerlist(p_flavor)))));
    if (name_tag)
      for (; rd_ok == 1; rd_ok = p_reader.Read()) {
        type = p_reader.NodeType();
        if (XML_READER_TYPE_ELEMENT == type) {
          // If our parent is optional and there is an unexpected tag then return and
          // we stay unbound.
          if ((p_flavor & XER_OPTIONAL) && !check_name((const char*)p_reader.LocalName(), p_td, e_xer)) {
            return -1;
          }
          verify_name(p_reader, p_td, e_xer);
          rd_ok = p_reader.Read();
          break;
        }
      }
    for (; rd_ok == 1; rd_ok = p_reader.Read()) {
      type = p_reader.NodeType();
      if (!e_xer && XML_READER_TYPE_ELEMENT == type) break;
      if (XML_READER_TYPE_TEXT == type) break;
    }
    const char *local_name = /*e_xer ?*/ (const char *)p_reader.Value() /*: (const char *)p_reader.Name()*/;
    if (!local_name) ; else    {
      for (; '\t'==*local_name || '\n'==*local_name; ++local_name) ;
      verdict_value = str_to_verdict(local_name, (p_flavor & EXIT_ON_ERROR) ? TRUE : FALSE);
    }
    if (name_tag)
      for (rd_ok = p_reader.Read(); rd_ok == 1; rd_ok = p_reader.Read()) {
        type = p_reader.NodeType();
        if (XML_READER_TYPE_END_ELEMENT == type) {
          p_reader.Read();
          break;
        }
      }
    else p_reader.Read();
  }
  int decoded_length = 0;
  return decoded_length;
}

int VERDICTTYPE::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound verdicttype value.");
    return -1;
  }
  
  char* tmp_str = mprintf("\"%s\"", verdict_name[verdict_value]);
  int enc_len = p_tok.put_next_token(JSON_TOKEN_STRING, tmp_str);
  Free(tmp_str);
  return enc_len;
}

int VERDICTTYPE::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
{
  json_token_t token = JSON_TOKEN_NONE;
  char* value = 0;
  size_t value_len = 0;
  size_t dec_len = 0;
  boolean use_default = p_td.json->default_value && 0 == p_tok.get_buffer_length();
  if (use_default) {
    // No JSON data in the buffer -> use default value
    value = const_cast<char*>(p_td.json->default_value);
    value_len = strlen(value);
  } else {
    dec_len = p_tok.get_next_token(&token, &value, &value_len);
  }
  boolean error = TRUE;
  if (JSON_TOKEN_ERROR == token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
    dec_len = JSON_ERROR_FATAL;
  }
  else if (JSON_TOKEN_STRING == token || use_default) {
    if (use_default || (value[0] == '\"' && value[value_len - 1] == '\"')) {
      if (!use_default) {
        // The default value doesn't have quotes around it
        value_len -= 2;
        ++value;
      }
      for (int i = NONE; i <= ERROR; ++i) {
        if (0 == strncmp(value, verdict_name[i], value_len)) {
          verdict_value = (verdicttype)i;
          error = FALSE;
          break;
        }
      }
    }
  } else {
    verdict_value = UNBOUND_VERDICT;
    return JSON_ERROR_INVALID_TOKEN;
  }
  if (error) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FORMAT_ERROR, "string", "verdicttype");
    verdict_value = UNBOUND_VERDICT;
    return JSON_ERROR_FATAL;
  }
  return (int)dec_len;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean operator==(verdicttype par_value, const VERDICTTYPE& other_value)
{
  if (!IS_VALID(par_value)) TTCN_error("The left operand of comparison is "
    "an invalid verdict value (%d).", par_value);
  if (!other_value.is_bound()) TTCN_error("The right operand of comparison "
    "is an unbound verdict value.");
  return par_value == other_value.verdict_value;
}

void VERDICTTYPE_template::clean_up()
{
  if (template_selection == VALUE_LIST ||
      template_selection == COMPLEMENTED_LIST)
    delete [] value_list.list_value;
  template_selection = UNINITIALIZED_TEMPLATE;
}

void VERDICTTYPE_template::copy_value(const VERDICTTYPE& other_value)
{
  if (!other_value.is_bound())
    TTCN_error("Creating a template from an unbound verdict value.");
  single_value = other_value.verdict_value;
  set_selection(SPECIFIC_VALUE);
}

void VERDICTTYPE_template::copy_template
  (const VERDICTTYPE_template& other_value)
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
    value_list.list_value = new VERDICTTYPE_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
	other_value.value_list.list_value[i]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported verdict template.");
  }
  set_selection(other_value);
}

VERDICTTYPE_template::VERDICTTYPE_template()
{
}

VERDICTTYPE_template::VERDICTTYPE_template(template_sel other_value)
    : Base_Template(other_value)
{
    check_single_selection(other_value);
}

VERDICTTYPE_template::VERDICTTYPE_template(verdicttype other_value)
    : Base_Template(SPECIFIC_VALUE)
{
  if (!IS_VALID(other_value)) TTCN_error("Creating a template from an "
    "invalid verdict value (%d).", other_value);
  single_value = other_value;
}

VERDICTTYPE_template::VERDICTTYPE_template(const VERDICTTYPE& other_value)
{
  copy_value(other_value);
}

VERDICTTYPE_template::VERDICTTYPE_template
  (const OPTIONAL<VERDICTTYPE>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const VERDICTTYPE&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a verdict template from an unbound optional field.");
  }
}

VERDICTTYPE_template::VERDICTTYPE_template
  (const VERDICTTYPE_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

VERDICTTYPE_template::~VERDICTTYPE_template()
{
  clean_up();
}

VERDICTTYPE_template& VERDICTTYPE_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

VERDICTTYPE_template& VERDICTTYPE_template::operator=(verdicttype other_value)
{
  if (!IS_VALID(other_value)) TTCN_error("Assignment of an invalid verdict "
    "value (%d) to a template.", other_value);
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

VERDICTTYPE_template& VERDICTTYPE_template::operator=
  (const VERDICTTYPE& other_value)
{
  clean_up();
  copy_value(other_value);
  return *this;
}

VERDICTTYPE_template& VERDICTTYPE_template::operator=
  (const OPTIONAL<VERDICTTYPE>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const VERDICTTYPE&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a verdict "
      "template.");
  }
  return *this;
}

VERDICTTYPE_template& VERDICTTYPE_template::operator=
  (const VERDICTTYPE_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean VERDICTTYPE_template::match(verdicttype other_value,
                                    boolean /* legacy */) const
{
  if (!IS_VALID(other_value)) TTCN_error("Matching a verdict template with "
    "an invalid value (%d).", other_value);
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
    TTCN_error("Matching with an uninitialized/unsupported verdict template.");
  }
  return FALSE;
}

boolean VERDICTTYPE_template::match(const VERDICTTYPE& other_value,
                                    boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  return match(other_value.verdict_value);
}

verdicttype VERDICTTYPE_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof "
               "or send operation on a non-specific verdict template.");
  return single_value;
}

void VERDICTTYPE_template::set_type(template_sel template_type,
    unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Internal error: Setting an invalid list type for a verdict "
      "template.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new VERDICTTYPE_template[list_length];
}

VERDICTTYPE_template& VERDICTTYPE_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
      template_selection != COMPLEMENTED_LIST)
    TTCN_error("Internal error: Accessing a list element of a non-list "
      "verdict template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Internal error: Index overflow in a verdict value list "
      "template.");
  return value_list.list_value[list_index];
}

void VERDICTTYPE_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    if (IS_VALID(single_value))
      TTCN_Logger::log_event("%s", verdict_name[single_value]);
    else TTCN_Logger::log_event("<unknown verdict value: %d>", single_value);
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

void VERDICTTYPE_template::log_match(const VERDICTTYPE& match_value,
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

void VERDICTTYPE_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE, "verdict template");
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
    VERDICTTYPE_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Verdict:
    *this = mp->get_verdict();
    break;
  default:
    param.type_error("verdict template");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* VERDICTTYPE_template::get_param(Module_Param_Name& param_name) const
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
    mp = new Module_Param_Verdict(single_value);
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
    TTCN_error("Referencing an uninitialized/unsupported verdict template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void VERDICTTYPE_template::encode_text(Text_Buf& text_buf) const
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
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an undefined/unsupported verdict "
      "template.");
  }
}

void VERDICTTYPE_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE: {
    int received_value = text_buf.pull_int().get_val();
    if (!IS_VALID(received_value)) TTCN_error("Text decoder: Invalid "
      "verdict value (%d) was received for a template.", received_value);
    single_value = (verdicttype)received_value;
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new VERDICTTYPE_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received "
      "for a verdict template.");
  }
}

boolean VERDICTTYPE_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean VERDICTTYPE_template::match_omit(boolean legacy /* = FALSE */) const
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
void VERDICTTYPE_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "verdict");
}
#endif
