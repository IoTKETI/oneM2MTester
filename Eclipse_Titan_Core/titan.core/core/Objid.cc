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
#include "Objid.hh"

#include "../common/dbgnew.hh"
#include <errno.h>
#include <limits.h>
#include "../common/static_check.h"
#include "Integer.hh"
#include "Optional.hh"

static const size_t MIN_COMPONENTS = 2;

struct OBJID::objid_struct {
  unsigned int ref_count;
  int n_components; ///< number of elements in \a components_ptr (min. 2)
  int overflow_idx; ///< index of the first overflow, or -1
  objid_element components_ptr[MIN_COMPONENTS];
};

#define OBJID_FMT "%u"
//#define OBJID_FMT "%lu"

void OBJID::init_struct(int n_components)
{
  if (n_components < 0) {
    val_ptr = NULL;
    TTCN_error("Initializing an objid value with a negative number of "
      "components.");
  }
  // TODO check n_components >= 2
  val_ptr = (objid_struct*)Malloc(sizeof(objid_struct)
    + (n_components - MIN_COMPONENTS) * sizeof(objid_element));
  val_ptr->ref_count = 1;
  val_ptr->n_components = n_components;
  val_ptr->overflow_idx = -1;
}

void OBJID::copy_value()
{
  if (val_ptr != NULL && val_ptr->ref_count > 1) {
    objid_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_components); // val_ptr reallocated
    memcpy(val_ptr->components_ptr, old_ptr->components_ptr,
           old_ptr->n_components * sizeof(objid_element));
    val_ptr->overflow_idx = old_ptr->overflow_idx;
  }
}

void OBJID::clean_up()
{
  if (val_ptr != NULL) {
    if (val_ptr->ref_count > 1) val_ptr->ref_count--;
    else if (val_ptr->ref_count == 1) Free(val_ptr);
    else TTCN_error("Internal error: Invalid reference counter in an objid "
      "value.");
    val_ptr = NULL;
  }
}

OBJID::OBJID()
{
  val_ptr = NULL; // unbound
}

OBJID::OBJID(int init_n_components, ...)
{
  init_struct(init_n_components);
  va_list ap;
  va_start(ap, init_n_components);
  for (int i = 0; i < init_n_components; i++) {
    val_ptr->components_ptr[i] = va_arg(ap, objid_element);
  }
  va_end(ap);
}


OBJID::OBJID(int init_n_components, const objid_element *init_components)
{
  init_struct(init_n_components);
  memcpy(val_ptr->components_ptr, init_components, init_n_components *
         sizeof(objid_element));
}

OBJID::OBJID(const OBJID& other_value)
: Base_Type(other_value)
{
  if (other_value.val_ptr == NULL)
    TTCN_error("Copying an unbound objid value.");
  val_ptr = other_value.val_ptr;
  val_ptr->ref_count++;
}

OBJID::~OBJID()
{
  clean_up();
}

OBJID& OBJID::operator=(const OBJID& other_value)
{
  if (other_value.val_ptr == NULL)
    TTCN_error("Assignment of an unbound objid value.");
  if (&other_value != this) {
    clean_up();
    val_ptr = other_value.val_ptr;
    val_ptr->ref_count++;
  }
  return *this;
}

boolean OBJID::operator==(const OBJID& other_value) const
{
  if (val_ptr == NULL) TTCN_error("The left operand of comparison is an "
    "unbound objid value.");
  if (other_value.val_ptr == NULL) TTCN_error("The right operand of comparison "
    "is an unbound objid value.");
  if (val_ptr->n_components != other_value.val_ptr->n_components) return FALSE;
  if (val_ptr->overflow_idx != other_value.val_ptr->overflow_idx) return FALSE;
  return !memcmp(val_ptr->components_ptr,
                 other_value.val_ptr->components_ptr,
                 val_ptr->n_components * sizeof(objid_element));
}

OBJID::objid_element& OBJID::operator[](int index_value)
{
  if (val_ptr == NULL) {
    if (index_value != 0)
      TTCN_error("Accessing a component of an unbound objid value.");
    init_struct(1);
    return val_ptr->components_ptr[0];
  } else {
    if (index_value < 0) TTCN_error("Accessing an objid component using "
                                   "a negative index (%d).", index_value);
    int n_components = val_ptr->n_components;
    if (index_value > n_components) TTCN_error("Index overflow when accessing "
      "an objid component: the index is %d, but the value has only %d "
      "components.", index_value, n_components);
    else if (index_value == n_components) {
      if (val_ptr->ref_count == 1) {
        val_ptr = (objid_struct*)
          Realloc(val_ptr, sizeof(objid_struct)
            + n_components * sizeof(objid_element));
        val_ptr->n_components++;
      } else {
        objid_struct *old_ptr = val_ptr;
        old_ptr->ref_count--;
        init_struct(n_components + 1);
        memcpy(val_ptr->components_ptr, old_ptr->components_ptr,
               n_components * sizeof(objid_element));
      }
    }
    return val_ptr->components_ptr[index_value];
  }
}

OBJID::objid_element OBJID::operator[](int index_value) const
{
  if (val_ptr == NULL)
    TTCN_error("Accessing a component of an unbound objid value.");
  if (index_value < 0)
    TTCN_error("Accessing an objid component using a negative index (%d).",
      index_value);
  if (index_value >= val_ptr->n_components) TTCN_error("Index overflow when "
    "accessing an objid component: the index is %d, but the value has only %d "
    "components.", index_value, val_ptr->n_components);
  return val_ptr->components_ptr[index_value];
}

int OBJID::size_of() const
{
  if (val_ptr == NULL)
    TTCN_error("Getting the size of an unbound objid value.");
  return val_ptr->n_components;
}

OBJID::operator const objid_element*() const
{
  if (val_ptr == NULL)
    TTCN_error("Casting an unbound objid value to const int*.");
  return val_ptr->components_ptr;
}

OBJID::objid_element OBJID::from_INTEGER(const INTEGER& p_int)
{
  int_val_t i_val = p_int.get_val();
  if (i_val.is_negative()) {
    TTCN_error("An OBJECT IDENTIFIER component cannot be negative");
  } 
  if (!i_val.is_native()) {
    TTCN_error("The value of an OBJECT IDENTIFIER component cannot exceed %u",
      INT_MAX);
  }
  return (OBJID::objid_element)i_val.get_val();
}

void OBJID::log() const
{
  if (val_ptr != NULL) {
    TTCN_Logger::log_event_str("objid { ");
    for (int i = 0; i < val_ptr->n_components; i++) {
      if (i == val_ptr->overflow_idx) {
        TTCN_Logger::log_event_str("overflow:");
      }

      TTCN_Logger::log_event(OBJID_FMT " ", val_ptr->components_ptr[i]);
    }
    TTCN_Logger::log_char('}');
  } else TTCN_Logger::log_event_unbound();
}

void OBJID::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE, "objid value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  if (mp->get_type()!=Module_Param::MP_Objid) param.type_error("objid value");
  if (sizeof(objid_element)!=sizeof(int)) TTCN_error("Internal error: OBJID::set_param()");
  clean_up();
  init_struct(mp->get_string_size());
  memcpy(val_ptr->components_ptr, mp->get_string_data(), val_ptr->n_components * sizeof(objid_element));
}

#ifdef TITAN_RUNTIME_2
Module_Param* OBJID::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  int* val_cpy = (int *)Malloc(val_ptr->n_components * sizeof(int));
  memcpy(val_cpy, val_ptr->components_ptr, val_ptr->n_components * sizeof(int));
  return new Module_Param_Objid(val_ptr->n_components, val_cpy);
}
#endif

void OBJID::encode_text(Text_Buf& text_buf) const
{
  if (val_ptr == NULL)
    TTCN_error("Text encoder: Encoding an unbound objid value.");
  text_buf.push_int(val_ptr->n_components);
  for (int i = 0; i < val_ptr->n_components; i++)
    text_buf.push_int(val_ptr->components_ptr[i]);
}

void OBJID::decode_text(Text_Buf& text_buf)
{
  int n_components = text_buf.pull_int().get_val();
  if (n_components < 0) TTCN_error("Text decoder: Negative number of "
    "components was received for an objid value.");
  clean_up();
  init_struct(n_components);
  for (int i = 0; i < n_components; i++)
    val_ptr->components_ptr[i] = text_buf.pull_int().get_val();
}

void OBJID::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
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
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-encoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No RAW descriptor available for type '%s'.", p_td.name);
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
  default:
    TTCN_error("Unknown coding method requested to encode type '%s'",
               p_td.name);
  }
  va_end(pvar);
}

void OBJID::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
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
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-decoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No RAW descriptor available for type '%s'.", p_td.name);
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
  default:
    TTCN_error("Unknown coding method requested to decode type '%s'",
               p_td.name);
  }
  va_end(pvar);
}

ASN_BER_TLV_t*
OBJID::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                      unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(!new_tlv) {
    size_t V_len=0;
    switch(p_td.asnbasetype) {
    case TTCN_Typedescriptor_t::OBJID:
      if(val_ptr->n_components<2)
        TTCN_EncDec_ErrorContext::error_internal
          ("OBJID must have at least 2 components.");
      V_len=(min_needed_bits(val_ptr->components_ptr[0]*40
                             +val_ptr->components_ptr[1])+6)/7;
      for(int i=2; i<val_ptr->n_components; i++)
        V_len+=(min_needed_bits(val_ptr->components_ptr[i])+6)/7;
      break;
    case TTCN_Typedescriptor_t::ROID:
      for(int i=0; i<val_ptr->n_components; i++)
        V_len+=(min_needed_bits(val_ptr->components_ptr[i])+6)/7;
      break;
    default:
      TTCN_EncDec_ErrorContext::error_internal
        ("Missing/wrong basetype info for type '%s'.", p_td.name);
    } // switch
    new_tlv=ASN_BER_TLV_t::construct(V_len, NULL);
    unsigned char *Vptr=new_tlv->V.str.Vstr;
    for(int i=0; i<val_ptr->n_components; i++) {
      unsigned long ul;
      if(i==0 && p_td.asnbasetype==TTCN_Typedescriptor_t::OBJID) {
        ul=val_ptr->components_ptr[0]*40+val_ptr->components_ptr[1];
        i++;
      }
      else ul=val_ptr->components_ptr[i];
      size_t noo=(min_needed_bits(ul)+6)/7;
      for(size_t j=noo; j>0; j--) {
        Vptr[j-1]=(ul & 0x7F) | 0x80;
        ul>>=7;
      }
      Vptr[noo-1]&=0x7F;
      Vptr+=noo;
    } // for i
  }
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}


boolean OBJID::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                              const ASN_BER_TLV_t& p_tlv,
                              unsigned L_form)
{
  clean_up();
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec("While decoding OBJID type: ");
  stripped_tlv.chk_constructed_flag(FALSE);
  if (!stripped_tlv.isComplete) return FALSE;
  if (!stripped_tlv.V_tlvs_selected && stripped_tlv.V.str.Vlen==0) {
    ec.error(TTCN_EncDec::ET_INVAL_MSG, "Length of V-part is 0.");
    return FALSE;
  }
  switch(p_td.asnbasetype) {
  case TTCN_Typedescriptor_t::OBJID:
  case TTCN_Typedescriptor_t::ROID:
    break;
  default:
    TTCN_EncDec_ErrorContext::error_internal
      ("Missing/wrong basetype info for type '%s'.", p_td.name);
  } // switch
  unsigned char *Vptr=stripped_tlv.V.str.Vstr;
  boolean eoc=FALSE; // end-of-component
  int i=0;
  unsigned long long ull=0;
  STATIC_ASSERT(sizeof(ull) > sizeof(objid_element));

  boolean err_repr=FALSE;
  while (Vptr < stripped_tlv.V.str.Vstr + stripped_tlv.V.str.Vlen) {
    ull |= *Vptr & 0x7F;
    if ((*Vptr & 0x80) && err_repr==FALSE) { // not-eoc
      if (ull & unsigned_llong_7msb) {
        ec.error(TTCN_EncDec::ET_REPR,
                 "Value of the #%d component is too big.", i+1);
        err_repr=TRUE;
      }
      ull<<=7;
      eoc=FALSE;
    }
    else { // eoc
      if (i==0 && p_td.asnbasetype==TTCN_Typedescriptor_t::OBJID) {
        // first two component of objid
        switch(ull/40ul) {
        case 0:
          (*this)[0]=0; break;
        case 1:
          (*this)[0]=1; break;
        default:
          (*this)[0]=2; break;
        }
        (*this)[1]=(int)(ull-40*(*this)[0]);
        i=1;
      }
      else { // other components (>2)
        // objid_element is UINT/ULONG; the result of the cast is Uxxx_MAX.
        // It's computed at compile time.
        if(ull > ((objid_element)-1)) {
          if(err_repr==FALSE)
            ec.error(TTCN_EncDec::ET_REPR,
                     "Value of the #%d component is too big.", i+1);
          (*this)[i]=(objid_element)-1;
          // remember the first overflow
          if (val_ptr->overflow_idx < 0) val_ptr->overflow_idx = i;
        } // if ul too big
        else
          (*this)[i]=(objid_element)ull;
      }
      err_repr=FALSE;
      ull=0;
      eoc=TRUE;
      i++;
    } // eoc
    Vptr++;
  } // while Vptr...
  if(eoc==FALSE)
    ec.error(TTCN_EncDec::ET_INVAL_MSG,
             "The last component (#%d) is unterminated.", i+1);
  return TRUE;
}


int OBJID::XER_encode(const XERdescriptor_t& p_td,
		  TTCN_Buffer& p_buf, unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound object identifier value.");
  }
  int encoded_length=(int)p_buf.get_len();

  flavor |= SIMPLE_TYPE;
  flavor &= ~XER_RECOF; // object identifier doesn't care
  begin_xml(p_td, p_buf, flavor, indent, FALSE);

  static char str_buf[64];
  for (int i = 0; i < val_ptr->n_components; ++i ) {
    // output dot before the second and subsequent components
    if (i > 0) p_buf.put_c('.');
    // output current component
    int str_len = snprintf(str_buf, sizeof(str_buf), OBJID_FMT,
      val_ptr->components_ptr[i]);
    if (str_len < 0 || str_len >= (int)sizeof(str_buf)) {
      TTCN_error("Internal error: system call snprintf() returned "
        "unexpected status code %d when converting value " OBJID_FMT,
        str_len, val_ptr->components_ptr[i]);
    }
    else p_buf.put_s(str_len, (const unsigned char*)str_buf);
  }

  end_xml(p_td, p_buf, flavor, indent, FALSE);

  return (int)p_buf.get_len() - encoded_length;
}

void OBJID::from_string(char* p_str)
{
  // Count dots to find number of components. (1 dot = 2 components, etc.)
  unsigned comps = 1;
  const char *p;
  for (p = p_str; *p != 0; ++p) {
    if (*p == '.') ++comps;
  }
  // p now points at the end of the string. If it was empty, then there were
  // no components; compensate the fact that we started at 1.
  init_struct((p != p_str) ? comps : 0);

  char *beg, *end = 0;
  comps = 0;
  for (beg = p_str; beg < p; ++beg) {
    errno = 0;
    long ret = strtol(beg, &end, 10);
    if (errno) break;

    // TODO check value for too big ?
    (*this)[comps++] = ret;
    beg = end; // move to the dot; will move past it when incremented
  }
}

int OBJID::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
		  unsigned int flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*)
{
  boolean exer  = is_exer(flavor);
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
  if (success == 1) {
    char * val = (char *)reader.ReadString(); // We own this (writable) string
    if (0 == val) {
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG, "Bogus object identifier");
      return 0;
    }
    
    from_string(val);

    xmlFree(val);
  }
  for (success = reader.Read(); success == 1; success = reader.Read()) {
    int type = reader.NodeType();
    if (XML_READER_TYPE_END_ELEMENT == type) {
      verify_end(reader, p_td, depth, exer);
      reader.Read();
      break;
    }
  }
  return 1; // decode successful
}

int OBJID::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound object identifier value.");
    return -1;
  }
  
  char* objid_str = mcopystrn("\"", 1);
  for (int i = 0; i < val_ptr->n_components; ++i) {
    objid_str = mputprintf(objid_str, "%s" OBJID_FMT, (i > 0 ? "." : ""), val_ptr->components_ptr[i]);
  }
  objid_str = mputstrn(objid_str, "\"", 1);
  int enc_len = p_tok.put_next_token(JSON_TOKEN_STRING, objid_str);
  Free(objid_str);
  return enc_len;
}

int OBJID::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
{
  json_token_t token = JSON_TOKEN_NONE;
  char* value = 0;
  size_t value_len = 0;
  boolean error = FALSE;
  size_t dec_len = 0;
  boolean use_default = p_td.json->default_value && 0 == p_tok.get_buffer_length();
  if (use_default) {
    // No JSON data in the buffer -> use default value
    value = const_cast<char*>(p_td.json->default_value);
    value_len = strlen(value);
  } else {
    dec_len = p_tok.get_next_token(&token, &value, &value_len);
  }
  if (JSON_TOKEN_ERROR == token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  else if (JSON_TOKEN_STRING == token || use_default) {
    if (use_default || (value_len > 2 && value[0] == '\"' && value[value_len - 1] == '\"')) {
      if (!use_default) {
        // The default value doesn't have quotes around it
        value_len -= 2;
        ++value;
      }
      // need a null-terminated string
      char* value2 = mcopystrn(value, value_len);
      from_string(value2);
      Free(value2);
    }
  }
  else {
    return JSON_ERROR_INVALID_TOKEN;
  }
  
  if (error) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FORMAT_ERROR, "string", "object identifier");
    if (p_silent) {
      clean_up();
    }
    return JSON_ERROR_FATAL;    
  }
  return (int)dec_len;
}

void OBJID_template::clean_up()
{
  if (template_selection == VALUE_LIST ||
    template_selection == COMPLEMENTED_LIST) delete [] value_list.list_value;
  template_selection = UNINITIALIZED_TEMPLATE;
}

void OBJID_template::copy_template(const OBJID_template& other_value)
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
    value_list.list_value = new OBJID_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
	other_value.value_list.list_value[i]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported objid template.");
  }
  set_selection(other_value);
}

OBJID_template::OBJID_template()
{

}

OBJID_template::OBJID_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

OBJID_template::OBJID_template(const OBJID& other_value)
  : Base_Template(SPECIFIC_VALUE), single_value(other_value)
{

}

OBJID_template::OBJID_template(const OPTIONAL<OBJID>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const OBJID&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating an objid template from an unbound optional field.");
  }
}

OBJID_template::OBJID_template(const OBJID_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

OBJID_template::~OBJID_template()
{
  clean_up();
}

OBJID_template& OBJID_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

OBJID_template& OBJID_template::operator=(const OBJID& other_value)
{
  if (!other_value.is_bound())
    TTCN_error("Assignment of an unbound objid value to a template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

OBJID_template& OBJID_template::operator=(const OPTIONAL<OBJID>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const OBJID&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to an objid template.");
  }
  return *this;
}

OBJID_template& OBJID_template::operator=(const OBJID_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean OBJID_template::match(const OBJID& other_value, boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
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
    TTCN_error("Matching with an uninitialized/unsupported objid template.");
  }
  return FALSE;
}

const OBJID& OBJID_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof "
               "or send operation on a non-specific objid template.");
  return single_value;
}

int OBJID_template::size_of() const
{
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
    return single_value.size_of();
  case OMIT_VALUE:
    TTCN_error("Performing sizeof() operation on an objid template "
               "containing omit value.");
  case ANY_VALUE:
  case ANY_OR_OMIT:
    TTCN_error("Performing sizeof() operation on a */? objid template.");
  case VALUE_LIST:
  {
    if (value_list.n_values<1)
      TTCN_error("Internal error: "
                 "Performing sizeof() operation on an objid template "
                 "containing an empty list.");
    int item_size = value_list.list_value[0].size_of();
    for (unsigned int i = 1; i < value_list.n_values; i++) {
      if (value_list.list_value[i].size_of()!=item_size)
        TTCN_error("Performing sizeof() operation on an objid template "
                   "containing a value list with different sizes.");
    }
    return item_size;
  }
  case COMPLEMENTED_LIST:
    TTCN_error("Performing sizeof() operation on an objid template "
               "containing complemented list.");
  default:
    TTCN_error("Performing sizeof() operation on an "
               "uninitialized/unsupported objid template.");
  }
  return 0;
}

void OBJID_template::set_type(template_sel template_type,
  unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
      TTCN_error("Setting an invalid list type for an objid template.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new OBJID_template[list_length];
}

OBJID_template& OBJID_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
    template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list objid template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in an objid value list template.");
  return value_list.list_value[list_index];
}

void OBJID_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value.log();
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for(unsigned int i = 0; i < value_list.n_values; i++) {
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

void OBJID_template::log_match(const OBJID& match_value,
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

void OBJID_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE, "objid template");
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
    OBJID_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Objid:
    if (sizeof(OBJID::objid_element)!=sizeof(int)) TTCN_error("Internal error: OBJID_template::set_param()"); 
    *this = OBJID(mp->get_string_size(), (OBJID::objid_element*)mp->get_string_data());
    break;
  //case Module_Param::MP_Objid_Template:
  // TODO
  //break;
  default:
    param.type_error("objid template");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* OBJID_template::get_param(Module_Param_Name& param_name) const
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
    mp = single_value.get_param(param_name);
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
    TTCN_error("Referencing an uninitialized/unsupported objid template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void OBJID_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE:
    single_value.encode_text(text_buf);
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an undefined/unsupported objid "
      "template.");
  }
}

void OBJID_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE:
    single_value.decode_text(text_buf);
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new OBJID_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was "
               "received for an objid template.");
  }
}

boolean OBJID_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean OBJID_template::match_omit(boolean legacy /* = FALSE */) const
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
void OBJID_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "objid");
}
#endif
