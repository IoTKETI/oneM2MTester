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
 *   Feher, Csaba
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
#include <string.h>

#include "Boolean.hh"
#include "../common/memory.h"
#include "Parameters.h"
#include "Param_Types.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Encdec.hh"
#include "RAW.hh"
#include "BER.hh"
#include "TEXT.hh"
#include "Charstring.hh"
#include "XmlReader.hh"
#include "Optional.hh"

static const Token_Match boolean_true_match("^(true).*$",TRUE);
static const Token_Match boolean_false_match("^(false).*$",TRUE);

BOOLEAN::BOOLEAN()
{
  bound_flag = FALSE;
}

BOOLEAN::BOOLEAN(boolean other_value)
{
  bound_flag = TRUE;
  boolean_value = other_value;
}

BOOLEAN::BOOLEAN(const BOOLEAN& other_value)
: Base_Type(other_value)
{
  other_value.must_bound("Copying an unbound boolean value.");
  bound_flag = TRUE;
  boolean_value = other_value.boolean_value;
}

BOOLEAN& BOOLEAN::operator=(boolean other_value)
{
  bound_flag = TRUE;
  boolean_value = other_value;
  return *this;
}

BOOLEAN& BOOLEAN::operator=(const BOOLEAN& other_value)
{
  other_value.must_bound("Assignment of an unbound boolean value.");
  bound_flag = TRUE;
  boolean_value = other_value.boolean_value;
  return *this;
}

boolean BOOLEAN::operator!() const
{
  must_bound("The operand of not operator is an unbound boolean value.");
  return !boolean_value;
}

boolean BOOLEAN::operator&&(boolean other_value) const
{
  must_bound("The left operand of and operator is an unbound boolean value.");
  return boolean_value && other_value;
}

boolean BOOLEAN::operator&&(const BOOLEAN& other_value) const
{
  must_bound("The left operand of and operator is an unbound boolean value.");
  if (!boolean_value) return FALSE;
  other_value.must_bound("The right operand of and operator is an unbound "
    "boolean value.");
  return other_value.boolean_value;
}

boolean BOOLEAN::operator^(boolean other_value) const
{
  must_bound("The left operand of xor operator is an unbound boolean value.");
  return boolean_value != other_value;
}

boolean BOOLEAN::operator^(const BOOLEAN& other_value) const
{
  must_bound("The left operand of xor operator is an unbound boolean value.");
  other_value.must_bound("The right operand of xor operator is an unbound "
    "boolean value.");
  return boolean_value != other_value.boolean_value;
}

boolean BOOLEAN::operator||(boolean other_value) const
{
  must_bound("The left operand of or operator is an unbound boolean value.");
  return boolean_value || other_value;
}

boolean BOOLEAN::operator||(const BOOLEAN& other_value) const
{
  must_bound("The left operand of or operator is an unbound boolean value.");
  if (boolean_value) return TRUE;
  other_value.must_bound("The right operand of or operator is an unbound "
    "boolean value.");
  return other_value.boolean_value;
}

boolean BOOLEAN::operator==(boolean other_value) const
{
  must_bound("The left operand of comparison is an unbound boolean value.");
  return boolean_value == other_value;
}

boolean BOOLEAN::operator==(const BOOLEAN& other_value) const
{
  must_bound("The left operand of comparison is an unbound boolean value.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "boolean value.");
  return boolean_value == other_value.boolean_value;
}

BOOLEAN::operator boolean() const
{
  must_bound("Using the value of an unbound boolean variable.");
  return boolean_value;
}

void BOOLEAN::clean_up()
{
  bound_flag = FALSE;
}

void BOOLEAN::log() const
{
  if (bound_flag) TTCN_Logger::log_event_str(boolean_value ? "true" : "false");
  else TTCN_Logger::log_event_unbound();
}

void BOOLEAN::encode_text(Text_Buf& text_buf) const
{
  must_bound("Text encoder: Encoding an unbound boolean value.");
  text_buf.push_int(boolean_value ? 1 : 0);
}

void BOOLEAN::decode_text(Text_Buf& text_buf)
{
  int int_value = text_buf.pull_int().get_val();
  switch (int_value) {
  case 0:
    boolean_value = FALSE;
    break;
  case 1:
    boolean_value = TRUE;
    break;
  default:
    TTCN_error("Text decoder: An invalid boolean value (%d) was received.",
      int_value);
  }
  bound_flag = TRUE;
}

void BOOLEAN::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE, "boolean value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  if (mp->get_type()!=Module_Param::MP_Boolean) param.type_error("boolean value");
  bound_flag = TRUE;
  boolean_value = mp->get_boolean();
}

#ifdef TITAN_RUNTIME_2
Module_Param* BOOLEAN::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  return new Module_Param_Boolean(boolean_value);
}
#endif

void BOOLEAN::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
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

void BOOLEAN::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
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

ASN_BER_TLV_t*
BOOLEAN::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                        unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(!new_tlv) {
    new_tlv=ASN_BER_TLV_t::construct(1, NULL);
    new_tlv->V.str.Vstr[0]=boolean_value==TRUE?0xFF:0x00;
  }
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean BOOLEAN::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                                const ASN_BER_TLV_t& p_tlv,
                                unsigned L_form)
{
  bound_flag = FALSE;
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec("While decoding BOOLEAN type: ");
  stripped_tlv.chk_constructed_flag(FALSE);
  if (!stripped_tlv.isComplete) return FALSE;
  if(stripped_tlv.V.str.Vlen!=1)
    ec.error(TTCN_EncDec::ET_INVAL_MSG,
             "Length of V-part is %lu (instead of 1).",
             (unsigned long) stripped_tlv.V.str.Vlen);
  if(stripped_tlv.V.str.Vlen>=1) {
    switch(stripped_tlv.V.str.Vstr[0]) {
    case 0x00:
      boolean_value=FALSE;
      break;
    default:
      /* warning? */
    case 0xFF:
      boolean_value=TRUE;
      break;
    } // switch
    bound_flag = TRUE;
    return TRUE;
  } else return FALSE;
}

int BOOLEAN::TEXT_decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff,
  Limit_Token_List&, boolean no_err, boolean /*first_call*/)
{

  int decoded_length = 0;
  int str_len = 0;
  if (p_td.text->begin_decode) {
    int tl;
    if ((tl = p_td.text->begin_decode->match_begin(buff)) < 0) {
      if (no_err) return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "The specified token '%s' not found for '%s': ",
        (const char*) *(p_td.text->begin_decode), p_td.name);
      return 0;
    }
    decoded_length += tl;
    buff.increase_pos(tl);
  }
  if (buff.get_read_len() < 1 && no_err) return -TTCN_EncDec::ET_LEN_ERR;

  boolean found = FALSE;

  if ( p_td.text->val.bool_values
    && p_td.text->val.bool_values->true_decode_token) {
    int tl;
    if ((tl = p_td.text->val.bool_values->true_decode_token->match_begin(buff)) > -1) {
      str_len = tl;
      found = TRUE;
      boolean_value = TRUE;
    }
  }
  else {
    int tl;
    if ((tl = boolean_true_match.match_begin(buff)) >= 0) {
      str_len = tl;
      found = TRUE;
      boolean_value = TRUE;
    }
  }

  if (!found) {
    if ( p_td.text->val.bool_values
      && p_td.text->val.bool_values->false_decode_token) {
      int tl;
      if ((tl = p_td.text->val.bool_values->false_decode_token->match_begin(buff)) > -1) {
        str_len = tl;
        found = TRUE;
        boolean_value = FALSE;
      }
    }
    else {
      int tl;
      if ((tl = boolean_false_match.match_begin(buff)) >= 0) {
        str_len = tl;
        found = TRUE;
        boolean_value = FALSE;
      }
    }
  }

  if (found) {
    decoded_length += str_len;
    buff.increase_pos(str_len);
  }
  else {
    if (no_err) return -1;
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
      "No boolean token found for '%s': ", p_td.name);
    return decoded_length;
  }

  if (p_td.text->end_decode) {
    int tl;
    if ((tl = p_td.text->end_decode->match_begin(buff)) < 0) {
      if (no_err) return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "The specified token '%s' not found for '%s': ",
        (const char*) *(p_td.text->end_decode), p_td.name);
      return 0;
    }
    decoded_length += tl;
    buff.increase_pos(tl);
  }
  bound_flag = TRUE;
  return decoded_length;
}


int BOOLEAN::TEXT_encode(const TTCN_Typedescriptor_t& p_td,
                 TTCN_Buffer& buff) const{
  int encoded_length=0;
  if(p_td.text->begin_encode){
    buff.put_cs(*p_td.text->begin_encode);
    encoded_length+=p_td.text->begin_encode->lengthof();
  }
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
    if(p_td.text->end_encode){
      buff.put_cs(*p_td.text->end_encode);
      encoded_length+=p_td.text->end_encode->lengthof();
    }
    return encoded_length;
  }

  if(p_td.text->val.bool_values==NULL){
    if(boolean_value){
      buff.put_s(4,(const unsigned char*)"true");
      encoded_length+=4;
    }
    else {
      buff.put_s(5,(const unsigned char*)"false");
      encoded_length+=5;
    }
  } else {
    if(boolean_value){
      if(p_td.text->val.bool_values->true_encode_token){
        buff.put_cs(*p_td.text->val.bool_values->true_encode_token);
        encoded_length+=p_td.text->
            val.bool_values->true_encode_token->lengthof();
      } else {
        buff.put_s(4,(const unsigned char*)"true");
        encoded_length+=4;
      }
    }
    else {
      if(p_td.text->val.bool_values->false_encode_token){
        buff.put_cs(*p_td.text->val.bool_values->false_encode_token);
        encoded_length+=p_td.text->
            val.bool_values->false_encode_token->lengthof();
      } else {
        buff.put_s(5,(const unsigned char*)"false");
        encoded_length+=5;
      }
    }
  }

  if(p_td.text->end_encode){
    buff.put_cs(*p_td.text->end_encode);
    encoded_length+=p_td.text->end_encode->lengthof();
  }
  return encoded_length;
}

int BOOLEAN::RAW_encode(const TTCN_Typedescriptor_t& p_td, RAW_enc_tree& myleaf) const
{
  unsigned char *bc;
  int loc_length = p_td.raw->fieldlength ? p_td.raw->fieldlength : 1;
  int length = (loc_length + 7) / 8;
  unsigned char tmp;
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
    tmp = '\0';
  }
  else tmp = boolean_value ? 0xFF : 0x00;
//  myleaf.ext_bit=EXT_BIT_NO;
  if (myleaf.must_free) Free(myleaf.body.leaf.data_ptr);
  if (length > RAW_INT_ENC_LENGTH) {
    myleaf.body.leaf.data_ptr = bc = (unsigned char*)Malloc(length*sizeof(*bc));
    myleaf.must_free = TRUE;
    myleaf.data_ptr_used = TRUE;
  }
  else bc = myleaf.body.leaf.data_array;

  memset(bc, tmp, length * sizeof(*bc));
  if (boolean_value && loc_length % 8 != 0) {
    // remove the extra ones from the last octet
    bc[length - 1] &= BitMaskTable[loc_length % 8];
  }
  return myleaf.length = loc_length;
}

int BOOLEAN::RAW_decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff,
  int limit, raw_order_t top_bit_ord, boolean no_err, int /*sel_field*/,
  boolean /*first_call*/)
{
  bound_flag = FALSE;
  int prepaddlength = buff.increase_pos_padd(p_td.raw->prepadding);
  limit -= prepaddlength;
  int decode_length = p_td.raw->fieldlength > 0 ? p_td.raw->fieldlength : 1;
  if (decode_length > limit) {
    if (no_err) return -TTCN_EncDec::ET_LEN_ERR;
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There is not enough bits in the buffer to decode type %s (needed: %d, "
        "found: %d).", p_td.name, decode_length, limit);
    decode_length = limit;
  }
  int nof_unread_bits = buff.unread_len_bit();
  if (decode_length > nof_unread_bits) {
    if (no_err) return -TTCN_EncDec::ET_INCOMPL_MSG;
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INCOMPL_MSG,
      "There is not enough bits in the buffer to decode type %s (needed: %d, "
        "found: %d).", p_td.name, decode_length, nof_unread_bits);
    decode_length = nof_unread_bits;
  }
  if (decode_length < 0) return -1;
  else if (decode_length == 0) boolean_value = FALSE;
  else {
    RAW_coding_par cp;
    boolean orders = FALSE;
    if (p_td.raw->bitorderinoctet == ORDER_MSB) orders = TRUE;
    if (p_td.raw->bitorderinfield == ORDER_MSB) orders = !orders;
    cp.bitorder = orders ? ORDER_MSB : ORDER_LSB;
    orders = FALSE;
    if (p_td.raw->byteorder       == ORDER_MSB) orders = TRUE;
    if (p_td.raw->bitorderinfield == ORDER_MSB) orders = !orders;
    cp.byteorder = orders ? ORDER_MSB : ORDER_LSB;
    cp.fieldorder = p_td.raw->fieldorder;
    cp.hexorder = ORDER_LSB;
    int length = (decode_length + 7) / 8;
    unsigned char *data = (unsigned char*)Malloc(length*sizeof(unsigned char));
    buff.get_b((size_t)decode_length, data, cp, top_bit_ord);
    if(decode_length % 8){
      data[length - 1] &= BitMaskTable[decode_length % 8];
    }
    unsigned char ch = '\0';
    for (int a = 0; a < length; a++) ch |= data[a];
    Free(data);
    boolean_value = ch != '\0';
  }
  bound_flag = TRUE;
  decode_length += buff.increase_pos_padd(p_td.raw->padding);
  return decode_length + prepaddlength;
}

int BOOLEAN::XER_encode(const XERdescriptor_t& p_td,
  TTCN_Buffer& p_buf, unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound boolean value.");
  }
  int encoded_length=(int)p_buf.get_len();

  boolean exer  = is_exer(flavor);

  flavor |= (SIMPLE_TYPE | BXER_EMPTY_ELEM);
  if (begin_xml(p_td, p_buf, flavor, indent, FALSE) == -1) --encoded_length;

  if (exer) {
    if (p_td.xer_bits & XER_TEXT) {
      p_buf.put_c(boolean_value ? '1' : '0');
    }
    else {
      if (boolean_value) p_buf.put_s(4, (cbyte*)"true");
      else               p_buf.put_s(5, (cbyte*)"false");
    }
  }
  else {
    if (boolean_value) p_buf.put_s(7, (cbyte*)"<true/>");
    else               p_buf.put_s(8, (cbyte*)"<false/>");
  }

  end_xml(p_td, p_buf, flavor, indent, FALSE);

  return (int)p_buf.get_len() - encoded_length;
}

int BOOLEAN::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
  unsigned int flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*)
{
  const boolean exer = is_exer(flavor);
  int XMLValueList = !exer && is_record_of(flavor);
  const boolean notag = (exer && (p_td.xer_bits & (UNTAGGED))) ||
    is_exerlist(flavor) || XMLValueList;
  int depth = -1, success, type;
  const char *value = 0;

  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
    verify_name(reader, p_td, exer);
    value = (const char *)reader.Value();
  }
  else {
    for (success = reader.Ok(); success == 1; success = reader.Read()) {
      type = reader.NodeType();
      if (!notag && depth == -1) {
        if (XML_READER_TYPE_ELEMENT == type) {
          // If our parent is optional and there is an unexpected tag then return and
          // we stay unbound.
          if ((flavor & XER_OPTIONAL) && !check_name((const char*)reader.LocalName(), p_td, exer)) {
            return -1;
          }
          verify_name(reader, p_td, exer);
          depth = reader.Depth();

          if (exer && (p_td.dfeValue != 0) && reader.IsEmptyElement()) {
            *this = *static_cast<const BOOLEAN*>(p_td.dfeValue);
            (void)reader.Read();
            goto fini;
          }
          continue;
        } // if type
      }
      else { // found the enclosing tag already
        if (!exer && XML_READER_TYPE_ELEMENT == type) {
          // this must be EmptyElement Boolean
          if (!reader.IsEmptyElement()) TTCN_EncDec_ErrorContext::error(
            TTCN_EncDec::ET_INVAL_MSG, "Boolean must be empty element");
          value = (const char*)reader.LocalName();
        }
        else if (XML_READER_TYPE_TEXT == type) {
          // TextBoolean
          value = (const char*)reader.Value();
        }

        // Must not modify the buffer when attempting to find the selected alternative for USE-UNION
        if (!exer || !(flavor & EXIT_ON_ERROR)) reader.Read();
        break;
      } // if depth
    } // next read
  } // if not attribute

  if (value != 0 && *value != 0) {
    // extract the data
    if (value[1]=='\0' && (*value & 0x3E) == '0')
    {
      bound_flag = TRUE;
      boolean_value = *value == '1';
    }
    else if (!strcmp(value, "true")) {
      boolean_value = TRUE;
      bound_flag    = TRUE;
    }
    else if (!strcmp(value, "false")) {
      boolean_value = FALSE;
      bound_flag    = TRUE;
    }

  }

  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) { // I am an attribute
    // Let the caller do reader.AdvanceAttribute();
  }
  else if (!notag) {
    for (success = reader.Ok(); success == 1; success = reader.Read()) {
      type = reader.NodeType();
      if (XML_READER_TYPE_END_ELEMENT == type) {
        verify_end(reader, p_td, depth, exer);
        reader.Read(); // one last time
        break;
      }
    } // next
  }
fini:
  return 1;
}

int BOOLEAN::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound boolean value.");
    return -1;
  }
  return p_tok.put_next_token((boolean_value) ? JSON_TOKEN_LITERAL_TRUE : JSON_TOKEN_LITERAL_FALSE, NULL);
}

int BOOLEAN::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
{
  json_token_t token = JSON_TOKEN_NONE;
  size_t dec_len = 0;
  if (p_td.json->default_value && 0 == p_tok.get_buffer_length()) {
    // No JSON data in the buffer -> use default value
    if (strcmp(p_td.json->default_value, "true") == 0) {
      token = JSON_TOKEN_LITERAL_TRUE;
    } 
    else {
      token = JSON_TOKEN_LITERAL_FALSE;
    }
  } else {
    dec_len = p_tok.get_next_token(&token, NULL, NULL);
  }
  if (JSON_TOKEN_ERROR == token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  else if (JSON_TOKEN_LITERAL_TRUE == token) {
    bound_flag = TRUE;
    boolean_value = TRUE;
  }
  else if (JSON_TOKEN_LITERAL_FALSE == token) {
    bound_flag = TRUE;
    boolean_value = FALSE;
  } 
  else {
    bound_flag = FALSE;
    return JSON_ERROR_INVALID_TOKEN;
  }
  return (int)dec_len;
}


boolean operator&&(boolean bool_value, const BOOLEAN& other_value)
{
  if (!bool_value) return FALSE;
  other_value.must_bound("The right operand of and operator is an unbound "
    "boolean value.");
  return other_value.boolean_value;
}

boolean operator^(boolean bool_value, const BOOLEAN& other_value)
{
  other_value.must_bound("The right operand of xor operator is an unbound "
    "boolean value.");
  return bool_value != other_value.boolean_value;
}

boolean operator||(boolean bool_value, const BOOLEAN& other_value)
{
  if (bool_value) return TRUE;
  other_value.must_bound("The right operand of or operator is an unbound "
    "boolean value.");
  return other_value.boolean_value;
}

boolean operator==(boolean bool_value, const BOOLEAN& other_value)
{
  other_value.must_bound("The right operand of comparison is an unbound "
    "boolean value.");
  return bool_value == other_value.boolean_value;
}

void BOOLEAN_template::clean_up()
{
  if (template_selection == VALUE_LIST ||
      template_selection == COMPLEMENTED_LIST)
    delete [] value_list.list_value;
  template_selection = UNINITIALIZED_TEMPLATE;
}

void BOOLEAN_template::copy_template(const BOOLEAN_template& other_value)
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
    value_list.list_value = new BOOLEAN_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
        other_value.value_list.list_value[i]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported boolean template.");
  }
  set_selection(other_value);
}

BOOLEAN_template::BOOLEAN_template()
{

}

BOOLEAN_template::BOOLEAN_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

BOOLEAN_template::BOOLEAN_template(boolean other_value)
  : Base_Template(SPECIFIC_VALUE)
{
  single_value = other_value;
}

BOOLEAN_template::BOOLEAN_template(const BOOLEAN& other_value)
  : Base_Template(SPECIFIC_VALUE)
{
  other_value.must_bound("Creating a template from an unbound integer value.");
  single_value = other_value.boolean_value;
}

BOOLEAN_template::BOOLEAN_template(const OPTIONAL<BOOLEAN>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (boolean)(const BOOLEAN&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a boolean template from an unbound optional field.");
  }
}

BOOLEAN_template::BOOLEAN_template(const BOOLEAN_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

BOOLEAN_template::~BOOLEAN_template()
{
  clean_up();
}

BOOLEAN_template& BOOLEAN_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

BOOLEAN_template& BOOLEAN_template::operator=(boolean other_value)
{
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

BOOLEAN_template& BOOLEAN_template::operator=(const BOOLEAN& other_value)
{
  other_value.must_bound("Assignment of an unbound boolean value to a "
    "template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value.boolean_value;
  return *this;
}

BOOLEAN_template& BOOLEAN_template::operator=
  (const OPTIONAL<BOOLEAN>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (boolean)(const BOOLEAN&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a boolean "
      "template.");
  }
  return *this;
}

BOOLEAN_template& BOOLEAN_template::operator=
  (const BOOLEAN_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean BOOLEAN_template::match(boolean other_value,
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
    TTCN_error("Matching with an uninitialized/unsupported boolean template.");
  }
  return FALSE;
}

boolean BOOLEAN_template::match(const BOOLEAN& other_value,
                                boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  return match(other_value.boolean_value);
}

boolean BOOLEAN_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing valueof or "
               "send operation on a non-specific boolean template.");
  return single_value;
}

void BOOLEAN_template::set_type(template_sel template_type,
  unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list type for a boolean template.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new BOOLEAN_template[list_length];
}

BOOLEAN_template& BOOLEAN_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
      template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list boolean template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a boolean value list template.");
  return value_list.list_value[list_index];
}

void BOOLEAN_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    TTCN_Logger::log_event_str(single_value ? "true" : "false");
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

void BOOLEAN_template::log_match(const BOOLEAN& match_value,
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

void BOOLEAN_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE, "boolean template");
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
    BOOLEAN_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Boolean:
    *this = mp->get_boolean();
    break;
  default:
    param.type_error("boolean template");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* BOOLEAN_template::get_param(Module_Param_Name& param_name) const
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
    mp = new Module_Param_Boolean(single_value);
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
    TTCN_error("Referencing an uninitialized/unsupported boolean template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void BOOLEAN_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE:
    text_buf.push_int(single_value ? 1 : 0);
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported boolean "
      "template.");
  }
}

void BOOLEAN_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE: {
    int int_value = text_buf.pull_int().get_val();
    switch (int_value) {
    case 0:
      single_value = FALSE;
      break;
    case 1:
      single_value = TRUE;
      break;
    default:
      TTCN_error("Text decoder: An invalid boolean value (%d) was received for "
	"a template.", int_value);
    }
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new BOOLEAN_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was "
               "received for a boolean template.");
  }
}

boolean BOOLEAN_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean BOOLEAN_template::match_omit(boolean legacy /* = FALSE */) const
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
void BOOLEAN_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "boolean");
}
#endif
