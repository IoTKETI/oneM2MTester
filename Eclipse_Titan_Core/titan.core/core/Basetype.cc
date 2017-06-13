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
 *   Kovacs, Ferenc
 *   Ormandi, Matyas
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include "Basetype.hh"
#include "../common/memory.h"
#include "Logger.hh"
#include "Error.hh"
#include "BER.hh"
#include "RAW.hh"
#include "TEXT.hh"
#include "XER.hh"
#include "JSON.hh"
#include "XmlReader.hh"
#include "Module_list.hh"
#include "Universal_charstring.hh"

#include <openssl/bn.h>

#include <stdarg.h>
#include <string.h>
#include <limits.h>
// Note: RT2-only classes (Record_Type, Record_Of_Type) and Base_Type methods
// are in core2/Basetype2.cc

boolean Base_Type::ispresent() const
{
  TTCN_error("Base_Type::ispresent(): calling ispresent() on a non-optional field.");
  return TRUE;
}

void Base_Type::log() const
{
  TTCN_Logger::log_event_str("<logging of this type is not implemented>");
}

void Base_Type::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
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
    RAW_enc_tree root(TRUE, NULL, &rp, 1, p_td.raw);
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
    if(!p_td.xer) TTCN_EncDec_ErrorContext::error_internal(
      "No XER descriptor available for type '%s'.", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XER_encode(*(p_td.xer),p_buf, XER_coding, 0, 0, 0);
    p_buf.put_c('\n');
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

void Base_Type::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
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
    switch(p_td.raw->top_bit_order) {
    case TOP_BIT_LEFT:
      order=ORDER_LSB;
      break;
    case TOP_BIT_RIGHT:
    default:
      order=ORDER_MSB;
    }
    RAW_decode(p_td, p_buf, p_buf.get_len()*8, order);
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
    for (int success=reader.Read(); success==1; success=reader.Read()) {
      if (reader.NodeType() == XML_READER_TYPE_ELEMENT) break;
    }
    XER_decode(*(p_td.xer), reader, XER_coding, XER_NONE, 0);
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

void Base_Type::BER_chk_descr(const TTCN_Typedescriptor_t& p_td)
{
  if(!p_td.ber)
    TTCN_EncDec_ErrorContext::error_internal
      ("No BER descriptor available for type '%s'.", p_td.name);
}

void Base_Type::BER_encode_chk_coding(unsigned& p_coding)
{
  switch(p_coding) {
  case BER_ENCODE_CER:
  case BER_ENCODE_DER:
    break;
  default:
    TTCN_warning("Unknown BER encoding requested; using DER.");
    p_coding=BER_ENCODE_DER;
    break;
  }
}

void Base_Type::XER_encode_chk_coding(unsigned& p_coding,
  const TTCN_Typedescriptor_t& p_td)
{
  if (!p_td.xer) {
    TTCN_EncDec_ErrorContext::error_internal
      ("No XER descriptor available for type '%s'.", p_td.name);
  }
  switch (p_coding) {
  case XER_BASIC:
  case XER_CANONICAL:
  case XER_EXTENDED:
  case XER_EXTENDED | XER_CANONICAL:
    break;
  default:
    TTCN_warning("Unknown XER encoding requested; using Basic XER.");
    p_coding = XER_BASIC;
    break;
  }
}

static const cbyte empty_tag_end[4] = "/>\n";

int Base_Type::begin_xml(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
  unsigned int& flavor, int indent, boolean empty,
  collector_fn collector, const char *type_atr, unsigned int flavor2) const
{
  const int exer = is_exer(flavor);
  int omit_tag =
    // can never omit the tag at the toplevel, except when the type is union
    (indent != 0 || (flavor2 & THIS_UNION))
    && ( ((flavor & XER_RECOF) // can remove the tag even if not EXER
      && !(exer && (flavor & BXER_EMPTY_ELEM))) // except 26.6, 26.7
      || (exer /*&& */
        && ( (p_td.xer_bits & (UNTAGGED|ANY_ATTRIBUTES|ANY_ELEMENT))
          || (flavor & (EMBED_VALUES|XER_LIST|ANY_ATTRIBUTES|USE_NIL|USE_TYPE_ATTR)))));

  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
    begin_attribute(p_td, p_buf);
  }
  else if (!omit_tag) { // full tag
    const int indenting = !is_canonical(flavor);
    if (indenting) do_indent(p_buf, indent);
    p_buf.put_c('<');
    if (exer) write_ns_prefix(p_td, p_buf);

    boolean namespaces_needed = FALSE;
    if (exer) {
      const namespace_t *ns_info = NULL;
      if (p_td.my_module != NULL && p_td.ns_index != -1) {
        ns_info = p_td.my_module->get_ns((size_t)p_td.ns_index);
      }

      namespaces_needed = exer && //!(p_td.xer_bits & FORM_UNQUALIFIED) &&
        (indent==0 // top-level type
          || (ns_info && *ns_info->px == '\0' // own ns is prefixless
            && (flavor & DEF_NS_SQUASHED))
          );
    }

    size_t num_collected = 0;
    char **collected_ns = NULL;
    boolean def_ns = FALSE;
    if (namespaces_needed) {
      collected_ns = (this->*collector)(p_td, num_collected, def_ns, flavor2);
    }

    p_buf.put_s((size_t)p_td.namelens[exer] - 2, (cbyte*)p_td.names[exer]);

    if (namespaces_needed) {
      for (size_t cur_coll = 0; cur_coll < num_collected; ++cur_coll) {
        p_buf.put_s(strlen(collected_ns[cur_coll]), (cbyte*)collected_ns[cur_coll]);
        Free(collected_ns[cur_coll]); // job done
      }
      Free(collected_ns);
    }

    // If a default namespace is in effect (uri but no prefix) and the type
    // is unqualified, the default namespace must be canceled; otherwise
    // an XML tag without a ns prefix looks like it belongs to the def.namespace
    const boolean empty_ns_hack = exer && !omit_tag && (indent > 0)
      && (p_td.xer_bits & FORM_UNQUALIFIED)
      && (flavor & DEF_NS_PRESENT);

    if (empty_ns_hack) {
      p_buf.put_s(9, (cbyte*)" xmlns=''");
      flavor &= ~DEF_NS_PRESENT;
      flavor |=  DEF_NS_SQUASHED;
    }
    else if (def_ns) {
      flavor &= ~DEF_NS_SQUASHED;
      flavor |=  DEF_NS_PRESENT;
    }

    if (type_atr) {
      p_buf.put_s(mstrlen(const_cast<char*>(type_atr)), (cbyte*)type_atr);
    }
    // now close the tag
    if (empty) {
      p_buf.put_s(2 + indenting, empty_tag_end);
    }
    else {
      p_buf.put_s(
        1 + (indenting
          && !(flavor & SIMPLE_TYPE)
          && !(exer && (p_td.xer_bits & (XER_LIST|USE_TYPE_ATTR)))),
          empty_tag_end+1);
    }
  }
  else { // tag is omitted
    // If the outer XML element optimistically wrote a newline after its start tag,
    // back up over it.
    size_t buf_used = p_buf.get_len();
    if (exer && (flavor & USE_NIL) && (buf_used-- > 0) && // sequence point!
      (p_buf.get_data()[buf_used] == '\n')) {
      p_buf.increase_length((size_t)-1); // back up over the newline
      omit_tag = -1; // to help fix HO85831
    } else if (exer && (p_td.xer_bits & USE_TYPE_ATTR) && type_atr && (flavor2 & FROM_UNION_USETYPE)) {
      p_buf.increase_length((size_t)-1); // back up over the endtag
      p_buf.put_s(mstrlen(const_cast<char*>(type_atr)), (cbyte*)type_atr);
      p_buf.put_c('>');
    }
  }

  Free(const_cast<char*>(type_atr));

  return omit_tag;
}

void Base_Type::end_xml  (const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
  unsigned int flavor, int indent, boolean empty, unsigned int flavor2) const
{
  int exer = is_exer(flavor);
  boolean omit_tag =
    // can never omit the tag at the toplevel, except when the type is union
    (indent != 0 || (flavor2 & THIS_UNION))
    && ( ((flavor & XER_RECOF) // can remove the tag even if not EXER
      && !(exer && (flavor & BXER_EMPTY_ELEM))) // except 26.6, 26.7
      || (exer /*&& */
        && ( (p_td.xer_bits & (UNTAGGED|ANY_ATTRIBUTES|ANY_ELEMENT))
          || (flavor & (EMBED_VALUES|XER_LIST|ANY_ATTRIBUTES|USE_NIL|USE_TYPE_ATTR)))));

  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
    p_buf.put_c('\'');
  }
  else if (!empty && !omit_tag) {
    // now close the tag
    int indenting = !is_canonical(flavor);
    if (indenting && !(flavor & SIMPLE_TYPE)) do_indent(p_buf, indent);
    p_buf.put_s(2, (cbyte*)"</");
    if (exer) write_ns_prefix(p_td, p_buf);
    p_buf.put_s((size_t)p_td.namelens[exer]-1+indenting, (cbyte*)p_td.names[exer]);
  }

}


ASN_BER_TLV_t* Base_Type::BER_encode_chk_bound(boolean p_isbound)
{
  if(!p_isbound) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
    ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(0, NULL);
    new_tlv->Tlen=0;
    new_tlv->Tstr=NULL;
    new_tlv->Llen=0;
    new_tlv->Lstr=NULL;
    return new_tlv;
  }
  else return NULL;
}

// Called only from the two places in BER_encode_TLV_OCTETSTRING, below.
void Base_Type::BER_encode_putoctets_OCTETSTRING
(unsigned char *target,
 unsigned int octetnum_start, unsigned int octet_count,
 int p_nof_octets, const unsigned char *p_octets_ptr)
{
  if(   octetnum_start             > static_cast<unsigned int>(p_nof_octets)
     || octetnum_start+octet_count > static_cast<unsigned int>(p_nof_octets))
    TTCN_EncDec_ErrorContext::error_internal
      ("In Base_Type::BER_encode_putoctets_OCTETSTRING(): Index overflow.");
  memcpy(target, &p_octets_ptr[octetnum_start], octet_count);
}

ASN_BER_TLV_t* Base_Type::BER_encode_TLV_OCTETSTRING
(unsigned p_coding,
 int p_nof_octets, const unsigned char *p_octets_ptr)
{
  unsigned char *V_ptr;
  size_t V_len;
  unsigned int nof_fragments=0;

  if(p_coding==BER_ENCODE_CER) {
    nof_fragments=(p_nof_octets+999)/1000;
    if(!nof_fragments) nof_fragments=1;
  }
  else /*if(coding==BER_ENCODE_DER)*/ {
    nof_fragments=1;
  }

  ASN_BER_TLV_t *new_tlv=NULL;
  boolean is_constructed=nof_fragments>1;
  if(!is_constructed) {
    V_len=p_nof_octets;
    V_ptr=(unsigned char*)Malloc(V_len);
    BER_encode_putoctets_OCTETSTRING(V_ptr, 0, p_nof_octets,
                                     p_nof_octets, p_octets_ptr);
    new_tlv=ASN_BER_TLV_t::construct(V_len, V_ptr);
  }
  else { // is constructed
    ASN_BER_TLV_t *tmp_tlv=NULL;
    new_tlv=ASN_BER_TLV_t::construct(NULL);
    unsigned int rest_octets=p_nof_octets-(nof_fragments-1)*1000;
    V_len=1000;
    for(unsigned int i=0; i<nof_fragments; i++) {
      if(i==nof_fragments-1) V_len=rest_octets;
      V_ptr=(unsigned char*)Malloc(V_len);
      BER_encode_putoctets_OCTETSTRING(V_ptr, i*1000, V_len,
                                       p_nof_octets, p_octets_ptr);
      tmp_tlv=ASN_BER_TLV_t::construct(V_len, V_ptr);
      tmp_tlv=ASN_BER_V2TLV(tmp_tlv, OCTETSTRING_descr_, p_coding);
      new_tlv->add_TLV(tmp_tlv);
    }
  }
  return new_tlv;
}

/*
// for debugging purposes
static std::string bignum_as_bin(const BIGNUM* const BN) {
  int bytes = BN_num_bytes(BN);
  unsigned char* bn_as_bin = (unsigned char*) Malloc(bytes);
  BN_bn2bin(BN, bn_as_bin);

  std::string result;
  for (int i = 0; i < bytes; ++i) {
    result.append(char_as_bin(bn_as_bin[i]));
    result.append(" ");
  }
  Free(bn_as_bin);
  return result;
}

static std::string int_as_bin(int myInt) {
  std::string result;
  for (int i = sizeof(myInt) - 1; i >= 0; --i) {
    char current = (myInt >> (i * 8)) & 0xFF;
    result.append(char_as_bin(current));
  }
  return result;
}

static std::string char_as_bin(unsigned char ch) {
  std::string result;
  for (int i = 0; i < 8; ++i) {
    result.append((ch & 0x80) == 0x80 ? "1" : "0");
    ch <<= 1;
  }
  return result;
}

static std::string chars_as_bin(const unsigned char* const chars, int len) {
  std::string result;
  for (int i = 0; i < len; ++i) {
    result.append(char_as_bin(chars[i]));
    if (i != len - 1) {
      result.append(" ");
    }
  }
  return result;
}
*/

ASN_BER_TLV_t *Base_Type::BER_encode_TLV_INTEGER(unsigned,
  const int_val_t& p_int_val)
{
  if(p_int_val.is_native()){
    RInt p_int_val_int=p_int_val.get_val();
    // Determine the number of octets to be used in the encoding.
    unsigned long ulong_val=p_int_val_int >= 0
      ?static_cast<unsigned long>(p_int_val_int):
      ~static_cast<unsigned long>(p_int_val_int);
    size_t V_len=1;
    ulong_val>>=7;
    while(ulong_val!=0){
      V_len++;
      ulong_val>>=8;
    }
    ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(V_len,NULL);
    // Already in 2's complement encoding.
    ulong_val=static_cast<unsigned long>(p_int_val_int);
    for(size_t i=V_len;i>0;i--){
      new_tlv->V.str.Vstr[i-1]=ulong_val&0xFF;
      ulong_val>>=8;
    }
    return new_tlv;
  }

  // bignum
  
  const BIGNUM* const D = p_int_val.get_val_openssl();
  if (BN_is_zero(D)) {
    ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(1,NULL);
    new_tlv->V.str.Vstr[0] = 0;
    return new_tlv;
  }

  size_t num_bytes = BN_num_bytes(D);
  unsigned char* bn_as_bin = (unsigned char*) Malloc(num_bytes);
  BN_bn2bin(D, bn_as_bin);

  boolean pad = FALSE;
  if (BN_is_negative(D)) {
    for(size_t i = 0; i < num_bytes; ++i){
      bn_as_bin[i] = ~bn_as_bin[i];
    }

    // add one
    boolean stop = FALSE;
    for (int i = num_bytes - 1; i >= 0 && !stop; --i) {
      for (int j = 0; j < 8 && !stop; ++j) {
        unsigned char mask = (0x1 << j);
        if (!(bn_as_bin[i] & mask)) {
          bn_as_bin[i] |= mask;
          stop = TRUE;
        } else {
          bn_as_bin[i] ^= mask;
        }
      }
    }
    pad = !(bn_as_bin[0] & 0x80);
  } else {
    pad = bn_as_bin[0] & 0x80;
  }

  ASN_BER_TLV_t* new_tlv = ASN_BER_TLV_t::construct(num_bytes + pad, NULL);
  if (pad) {
    new_tlv->V.str.Vstr[0] = BN_is_negative(D) ? 0xFF : 0x0;
  } 
  
  memcpy(new_tlv->V.str.Vstr + pad, bn_as_bin, num_bytes);

  Free(bn_as_bin);
  return new_tlv;
}

ASN_BER_TLV_t *Base_Type::BER_encode_TLV_INTEGER(unsigned p_coding,
  const int& p_int_val)
{
  int_val_t p_int_val_val(p_int_val);
  return BER_encode_TLV_INTEGER(p_coding, p_int_val_val);
}

void Base_Type::BER_encode_chk_enum_valid(const TTCN_Typedescriptor_t& p_td,
                                          boolean p_isvalid, int p_value)
{
  if(!p_isvalid)
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_ENC_ENUM,
       "Encoding unknown value '%d' for enumerated type '%s'.",
       p_value, p_td.name);

}

void Base_Type::BER_decode_str2TLV(TTCN_Buffer& p_buf, ASN_BER_TLV_t& p_tlv,
                                   unsigned L_form)
{
  if(!ASN_BER_str2TLV(p_buf.get_read_len(), p_buf.get_read_data(),
                      p_tlv, L_form))
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_INCOMPL_MSG, "TLV is not complete.");
}

boolean Base_Type::BER_decode_constdTLV_next(const ASN_BER_TLV_t& p_tlv,
                                             size_t& V_pos,
                                             unsigned L_form,
                                             ASN_BER_TLV_t& p_target_tlv)
{
  if(p_tlv.V.str.Vlen<=V_pos) {
    if(!p_tlv.isLenDefinite)
      TTCN_EncDec_ErrorContext::error
        (TTCN_EncDec::ET_INCOMPL_MSG,
         "Missing end-of-contents octet in the indefinite length"
         " constructed TLV.");
    return FALSE;
  }
  if(!ASN_BER_str2TLV(p_tlv.V.str.Vlen-V_pos,
                      p_tlv.V.str.Vstr+V_pos, p_target_tlv, L_form)) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_INCOMPL_MSG,
       "Incomplete TLV in the constructed TLV.");
  }
  if(!p_tlv.isLenDefinite && p_target_tlv.tagnumber==0
     && p_target_tlv.tagclass==ASN_TAG_UNIV)
    return FALSE;
  V_pos+=p_target_tlv.get_len();
  return TRUE;
}

void Base_Type::BER_decode_constdTLV_end(const ASN_BER_TLV_t& p_tlv,
                                         size_t& V_pos,
                                         unsigned L_form,
                                         ASN_BER_TLV_t& p_target_tlv,
                                         boolean tlv_present)
{
  if(tlv_present
     || BER_decode_constdTLV_next(p_tlv, V_pos, L_form, p_target_tlv)) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_SUPERFL,
       "Superfluous TLV(s) at the end of constructed TLV.");
  }
}

static void BER_decode_chk_tag(const ASN_Tag_t& tag,
                               const ASN_BER_TLV_t& tlv)
{
  if (tlv.isTagComplete &&
      (tag.tagclass != tlv.tagclass || tag.tagnumber != tlv.tagnumber)) {
    ASN_Tag_t rcvdtag;
    rcvdtag.tagclass=tlv.tagclass;
    rcvdtag.tagnumber=tlv.tagnumber;
    char *rcvdstr=rcvdtag.print();
    try {
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,
	"Tag mismatch: Received: %s.", rcvdstr);
    } catch (...) {
      Free(rcvdstr);
      throw;
    }
    Free(rcvdstr);
  }
}

void Base_Type::BER_decode_strip_tags(const ASN_BERdescriptor_t& p_ber,
                                      const ASN_BER_TLV_t& p_tlv,
                                      unsigned L_form,
                                      ASN_BER_TLV_t& stripped_tlv)
{
  size_t i=p_ber.n_tags;
  if(i==0) {
    stripped_tlv=p_tlv;
    return;
  }
  char *expectedstr=p_ber.print_tags();
  TTCN_EncDec_ErrorContext ec("While checking tags (expecting %s): ",
                              expectedstr);
  Free(expectedstr);
  if(i==1) {
    BER_decode_chk_tag(p_ber.tags[0], p_tlv);
    stripped_tlv=p_tlv;
    return;
  }
  ASN_BER_TLV_t curr_tlv=p_tlv;
  boolean doit=TRUE;
  i--;
  while(doit) {
    TTCN_EncDec_ErrorContext ec2("At pos #%lu: ",
      (unsigned long) (p_ber.n_tags - i));
    BER_decode_chk_tag(p_ber.tags[i], curr_tlv);
    if(i!=0) { // not the innermost tag
      if(!curr_tlv.isConstructed) {
        ec2.error(TTCN_EncDec::ET_TAG,
          "The other (innermost %lu) tag(s) are missing.", (unsigned long) i);
        doit=FALSE;
        stripped_tlv=curr_tlv;
      }
      else { // O.K., is constructed
        size_t V_pos=0;
        BER_decode_constdTLV_next(curr_tlv, V_pos, L_form, stripped_tlv);
        // if superfluous...?
        ASN_BER_TLV_t endchecker_tlv;
        BER_decode_constdTLV_end(curr_tlv, V_pos, L_form, endchecker_tlv,
                                 FALSE);
        curr_tlv=stripped_tlv;
        i--;
      } // is constructed
    } // not the innermost
    else { // innermost tag
      doit=FALSE;
    }
  } // while doit
}

void Base_Type::BER_decode_getoctets_OCTETSTRING
(const unsigned char *source, size_t s_len,
 unsigned int& octetnum_start,
 int& p_nof_octets, unsigned char *p_octets_ptr)
{
  p_nof_octets=octetnum_start+s_len;
  memcpy(&p_octets_ptr[octetnum_start], source, s_len);
  octetnum_start+=s_len;
}

void Base_Type::BER_decode_TLV_OCTETSTRING
(const ASN_BER_TLV_t& p_tlv, unsigned L_form,
 unsigned int& octetnum_start,
 int& p_nof_octets, unsigned char *p_octets_ptr)
{
  if(!p_tlv.isConstructed) {
    BER_decode_getoctets_OCTETSTRING
      (p_tlv.V.str.Vstr, p_tlv.V.str.Vlen, octetnum_start,
       p_nof_octets, p_octets_ptr);
  }
  else { // is constructed
    ASN_BER_TLV_t tlv2;
    size_t V_pos=0;
    boolean doit=TRUE;
    while(doit) {
      if(!ASN_BER_str2TLV(p_tlv.V.str.Vlen-V_pos, p_tlv.V.str.Vstr+V_pos,
                          tlv2, L_form)) {
        TTCN_EncDec_ErrorContext::error
          (TTCN_EncDec::ET_INCOMPL_MSG,
           "Incomplete TLV in a constructed OCTETSTRING TLV.");
        return;
      }
      if(!p_tlv.isLenDefinite && tlv2.tagnumber==0
         && tlv2.tagclass==ASN_TAG_UNIV)
        doit=FALSE; // End-of-contents
      if(doit) {
        ASN_BER_TLV_t stripped_tlv;
        BER_decode_strip_tags(OCTETSTRING_ber_, tlv2, L_form, stripped_tlv);
	BER_decode_TLV_OCTETSTRING(tlv2, L_form, octetnum_start,
                                   p_nof_octets, p_octets_ptr);
	V_pos+=tlv2.get_len();
	if(V_pos>=p_tlv.V.str.Vlen) doit=FALSE;
      }
    } // while(doit)
  } // else / is constructed
}



boolean Base_Type::BER_decode_TLV_INTEGER(const ASN_BER_TLV_t& p_tlv,
  unsigned, int_val_t& p_int_val)
{
  p_tlv.chk_constructed_flag(FALSE);
  if (!p_tlv.isComplete) return FALSE;
  if (!p_tlv.V_tlvs_selected && p_tlv.V.str.Vlen == 0) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
      "Length of V-part is 0.");
    return FALSE;
  }

  const size_t Vlen = p_tlv.V.str.Vlen;

  if (Vlen > sizeof(RInt)) { // Bignum

    const boolean negative = p_tlv.V.str.Vstr[0] & 0x80;
    BIGNUM *D = BN_new();

    if (negative) {
      unsigned char* const Vstr = (unsigned char*) Malloc(Vlen);
      memcpy(Vstr, p_tlv.V.str.Vstr, Vlen);
      // -1 
      boolean stop = FALSE;
      for (int i = Vlen - 1; i >= 0 && !stop; --i) {
        for(int j = 0; j < 8 && !stop; ++j) {
          unsigned char mask = (0x1 << j);
          if (Vstr[i] & mask) {
            Vstr[i] ^= mask;
            stop = TRUE;
          } else {
            Vstr[i] |= mask;
          }
        }
      }

      for (size_t i = 0; i < Vlen; ++i) {
        Vstr[i] = ~Vstr[i];
      }

      BN_bin2bn(Vstr, Vlen, D);
      Free(Vstr);
    } else { // positive case
      BN_bin2bn(p_tlv.V.str.Vstr, Vlen, D);
    }

    BN_set_negative(D, negative);
    p_int_val = int_val_t(D);
    return TRUE;
  } // bignum 
  
  // Native int Vlen <= sizeof(RInt)
  const unsigned char* const Vstr = p_tlv.V.str.Vstr;
  RInt int_val = 0; 
  if (Vstr[0] & 0x80) { // negative 
      // the first bytes should be 1-s
      // e.g. -1 encoded on 32 bits: 11111111111111111111111111111111
      // e.g. -1 encoded on 8  bits:                         11111111 
      //                             [(sizeof(RInt)-Vlen)*8 ][Vlen*8]
      //                             [         sizeof(RInt)*8       ]

    for (size_t i = 0; i < sizeof(RInt) - Vlen; ++i) {
      int_val |= 0xFF;
      int_val <<= 8;
    }
  }

  int_val |= p_tlv.V.str.Vstr[0];
  for (size_t i = 1; i < Vlen; ++i) {
    int_val <<= 8;
    int_val |= p_tlv.V.str.Vstr[i];
  }

  p_int_val = int_val_t(int_val);
  return TRUE;
}

boolean Base_Type::BER_decode_TLV_INTEGER(const ASN_BER_TLV_t& p_tlv,
  unsigned L_form, int& p_int_val)
{
  int_val_t p_int_val_val(p_int_val);
  boolean ret_val = BER_decode_TLV_INTEGER(p_tlv, L_form, p_int_val_val);
  if (p_int_val_val.is_native()) {
    p_int_val = p_int_val_val.get_val();
  } else {
    TTCN_warning("Large integer value was decoded and it can't be returned "
    "as a native `int'");
  }
  return ret_val;
}

/*

// Actually, this is not used, but when we ever need a
// type-independent integer decoding, then we can move this to
// Basetype.hh. It is so beautiful and wonderful, I did not wanted
// this to get lost. :)

template<typename int_type>
void Base_Type::BER_decode_TLV_INTEGER
(const ASN_BER_TLV_t& p_tlv, unsigned L_form, int_type& p_intval)
{
  boolean overflow_flag=FALSE;
  p_tlv.chk_constructed_flag(FALSE);
  if(!p_tlv.V_tlvs_selected && p_tlv.V.str.Vlen==0) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_INVAL_MSG, "Length of V-part is 0.");
    p_intval=static_cast<int_type>(0);
    return;
  }
  int_type tmp_int=static_cast<int_type>(p_tlv.V.str.Vstr[0] & 0x7F);
  for(size_t i=1; i<p_tlv.V.str.Vlen; i++) {
    p_intval=tmp_int;
    tmp_int=static_cast<int_type>(256*tmp_int);
    if(tmp_int<p_intval) overflow_flag=TRUE;
    tmp_int=static_cast<int_type>(tmp_int+p_tlv.V.str.Vstr[i]);
  }
  if(p_tlv.V.str.Vstr[0] & 0x80) { // negative
    int_type highbit=static_cast<int_type>(1);
    for(size_t i=0; i<p_tlv.V.str.Vlen*8-1; i++) {
      int_type backup=highbit;
      highbit=static_cast<int_type>(2*highbit);
      if(highbit/2!=backup) overflow_flag=TRUE;
    }
    p_intval=static_cast<int_type>(tmp_int-highbit);
  }
  else { // positive
    p_intval=tmp_int;
  }
  if(overflow_flag)
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_REPR,
       "Value is too big (%lu octets).", p_tlv.V.str.Vlen);
}

*/

boolean Base_Type::BER_decode_TLV_CHOICE(const ASN_BERdescriptor_t& p_ber,
                                         const ASN_BER_TLV_t& p_tlv,
                                         unsigned L_form,
                                         ASN_BER_TLV_t& p_target_tlv)
{
  if(p_ber.n_tags>0) {
    size_t V_pos=0;
    p_tlv.chk_constructed_flag(TRUE);
    if(!BER_decode_constdTLV_next(p_tlv, V_pos, L_form, p_target_tlv))
      return FALSE;
  }
  else p_target_tlv=p_tlv;
  return TRUE;
}

boolean Base_Type::BER_decode_CHOICE_selection(boolean select_result,
                                               const ASN_BER_TLV_t& p_tlv)
{
  if(select_result) return TRUE;
  ASN_Tag_t rcvd_tag;
  rcvd_tag.tagclass=p_tlv.tagclass;
  rcvd_tag.tagnumber=p_tlv.tagnumber;
  char *rcvd_str=rcvd_tag.print();
  try {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,
      "Invalid tag or unknown alternative: %s.", rcvd_str);
  } catch (...) {
    Free(rcvd_str);
    throw;
  }
  Free(rcvd_str);
  return FALSE;
}

void Base_Type::BER_decode_chk_enum_valid(const TTCN_Typedescriptor_t& p_td,
                                          boolean p_isvalid, int p_value)
{
  /** \todo If extensible ENUMERATED is supported (for example, in the
    * typedescriptor there is something...), then give different error
    * message depending on that flag. */
  if(!p_isvalid)
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_DEC_ENUM,
       "Unknown value '%d' received for enumerated type '%s'.",
       p_value, p_td.name);
}

ASN_BER_TLV_t*
Base_Type::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                          unsigned) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *tlv=BER_encode_chk_bound(TRUE);
  if(tlv) return tlv;
  TTCN_EncDec_ErrorContext::error_internal
    ("BER_encode_V() not implemented for type '%s'.", p_td.name);
  return NULL;
}

boolean Base_Type::BER_decode_isMyMsg(const TTCN_Typedescriptor_t& p_td,
                                      const ASN_BER_TLV_t& p_tlv)
{
  if(p_td.ber->n_tags==0 || !p_tlv.isTagComplete) return TRUE;
  const ASN_Tag_t& tag=p_td.ber->tags[p_td.ber->n_tags-1];
  return (tag.tagclass==p_tlv.tagclass && tag.tagnumber==p_tlv.tagnumber);
}

boolean Base_Type::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                                  const ASN_BER_TLV_t& p_tlv,
                                  unsigned L_form)
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec;
  ec.error_internal
    ("BER_decode_V() not implemented for type '%s'.", p_td.name);
  return FALSE;
}

int Base_Type::RAW_encode(const TTCN_Typedescriptor_t& p_td,
                           RAW_enc_tree&) const
{
  TTCN_error("RAW encoding requested for type '%s'"
             " which has no RAW encoding method.", p_td.name);
  return 0;
}

int Base_Type::TEXT_encode(const TTCN_Typedescriptor_t& p_td,
                           TTCN_Buffer&) const
{
  TTCN_error("TEXT encoding requested for type '%s'"
             " which has no TEXT encoding method.", p_td.name);
  return 0;
}

int Base_Type::TEXT_decode(const TTCN_Typedescriptor_t& p_td,
                 TTCN_Buffer&,  Limit_Token_List&, boolean, boolean)
{
  TTCN_error("TEXT decoding requested for type '%s'"
             " which has no TEXT decoding method.", p_td.name);
  return 0;
}

int Base_Type::RAW_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer&, int /* limit */, raw_order_t /* top_bit_ord */,
  boolean /* no_error */, int /* sel_field */, boolean /* first_call */ )
{
  TTCN_error("RAW decoding requested for type '%s'"
             " which has no RAW decoding method.",p_td.name);
  return 0;
}

int Base_Type::XER_encode(const XERdescriptor_t& p_td,
                          TTCN_Buffer&, unsigned int, unsigned int, int, embed_values_enc_struct_t*) const
{
  TTCN_error("XER encoding requested for type '%-.*s' which has no"
             " XER encoding method.", p_td.namelens[0]-2, p_td.names[0]);
  return 0;
}

int Base_Type::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap&,
                          unsigned int, unsigned int, embed_values_dec_struct_t*) {
  TTCN_error("XER decoding requested for type '%-.*s' which has no"
             " XER decoding method.", p_td.namelens[0]-2, p_td.names[0]);
  return 0;
}

int Base_Type::JSON_encode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer&) const
{
  TTCN_error("JSON encoding requested for type '%s' which has no"
             " JSON encoding method.", p_td.name);
  return 0;
}

int Base_Type::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer&, boolean) 
{
  TTCN_error("JSON decoding requested for type '%s' which has no"
             " JSON decoding method.", p_td.name);
  return 0;
}

boolean Base_Type::can_start(const char *name, const char *uri,
  XERdescriptor_t const& xd, unsigned int flavor, unsigned int /*flavor2*/)
{
  boolean e_xer = is_exer(flavor);
  // Check the name. If EXER, check the namespace too.
  return check_name(name, xd, e_xer) && (!e_xer || check_namespace(uri, xd));
}

char ** Base_Type::collect_ns(const XERdescriptor_t& p_td, size_t& num, bool& def_ns, unsigned int /* flavor */) const
{
  def_ns = FALSE;
  char *tmp = NULL;
  if (p_td.my_module != 0 && p_td.ns_index != -1
    && !(p_td.xer_bits & FORM_UNQUALIFIED)) {
    const namespace_t *my_ns = p_td.my_module->get_ns((size_t)p_td.ns_index);
    if (!*my_ns->px) def_ns = TRUE;
    tmp = mprintf(" xmlns%s%s='%s'",
      ((*my_ns->px) ? ":" : ""), my_ns->px,
      my_ns->ns
    );
  }
  // The above may throw, but then nothing was allocated.
  if (tmp != NULL) {
    num = 1;
    char **retval = (char**)Malloc(sizeof(char*));
    *retval = tmp;
    return retval;
  }
  else {
    num = 0;
    return NULL;
  }
}

void Base_Type::merge_ns(char **&collected_ns, size_t& num_collected,
  char **new_namespaces, size_t num_new)
{

  for (size_t cur_ns = 0; cur_ns < num_new; ++cur_ns) {
    for (size_t cur_coll = 0; cur_coll < num_collected; ++cur_coll) {
      if (!strcmp(new_namespaces[cur_ns], collected_ns[cur_coll])) {
        // same, drop it
        Free(new_namespaces[cur_ns]);
        new_namespaces[cur_ns] = NULL;
        break;
      }
    }

    if (new_namespaces[cur_ns]) { // still there
      collected_ns = (char**)Realloc(collected_ns, sizeof(char*) * ++num_collected);
      collected_ns[num_collected-1] = new_namespaces[cur_ns];
    }
  }
  Free(new_namespaces);
  new_namespaces = 0;
}

/////////////////////////////////////////////////////////////

TTCN_Type_list::~TTCN_Type_list()
{
  Free(types);
}

void TTCN_Type_list::push(const Base_Type *p_type)
{
  types=(const Base_Type**)Realloc(types, ++n_types*sizeof(*types));
  types[n_types-1]=p_type;
}

const Base_Type* TTCN_Type_list::pop()
{
  if(!n_types)
    TTCN_EncDec_ErrorContext::error_internal
      ("TTCN_Type_list::pop(): List is empty.");
  const Base_Type *t;
  t=types[--n_types];
  types=(const Base_Type**)Realloc(types, n_types*sizeof(*types));
  return t;
}

const Base_Type* TTCN_Type_list::get_nth(size_t pos) const
{
  if(pos==0) return types[0];
  if(pos>n_types)
    TTCN_EncDec_ErrorContext::error_internal
      ("TTCN_Type_list::get_nth(%lu): Out of range.", (unsigned long) pos);
  return types[n_types-pos];
}

const TTCN_Typedescriptor_t BOOLEAN_descr_={"BOOLEAN", &BOOLEAN_ber_,
  &BOOLEAN_raw_, &BOOLEAN_text_, &BOOLEAN_xer_, &BOOLEAN_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t INTEGER_descr_={"INTEGER", &INTEGER_ber_,
  &INTEGER_raw_, &INTEGER_text_, &INTEGER_xer_, &INTEGER_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t FLOAT_descr_={"REAL", &FLOAT_ber_, &FLOAT_raw_,
  NULL, &FLOAT_xer_, &FLOAT_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t VERDICTTYPE_descr_={"verdicttype", NULL, NULL,
  NULL, &VERDICTTYPE_xer_, &VERDICTTYPE_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t OBJID_descr_={"OBJECT IDENTIFIER", &OBJID_ber_,
  NULL, NULL, &OBJID_xer_, &OBJID_json_, NULL, TTCN_Typedescriptor_t::OBJID};

const TTCN_Typedescriptor_t BITSTRING_descr_={"BIT STRING", &BITSTRING_ber_,
  &BITSTRING_raw_, NULL, &BITSTRING_xer_, &BITSTRING_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t HEXSTRING_descr_={"hexstring", NULL,
  &HEXSTRING_raw_, NULL, &HEXSTRING_xer_, &HEXSTRING_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t OCTETSTRING_descr_={"OCTET STRING",
  &OCTETSTRING_ber_, &OCTETSTRING_raw_, &OCTETSTRING_text_, &OCTETSTRING_xer_, &OCTETSTRING_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t CHARSTRING_descr_={"charstring", NULL,
  &CHARSTRING_raw_, &CHARSTRING_text_, &CHARSTRING_xer_, &CHARSTRING_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t UNIVERSAL_CHARSTRING_descr_={"universal charstring",
  NULL, &UNIVERSAL_CHARSTRING_raw_, &UNIVERSAL_CHARSTRING_text_, &UNIVERSAL_CHARSTRING_xer_, &UNIVERSAL_CHARSTRING_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t COMPONENT_descr_={"component", NULL, NULL, NULL,
  NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t DEFAULT_descr_={"default", NULL, NULL, NULL,
  NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t ASN_NULL_descr_={"NULL", &ASN_NULL_ber_, NULL,
  NULL, &ASN_NULL_xer_, &ASN_NULL_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t ASN_ANY_descr_={"ANY", &ASN_ANY_ber_, NULL,
  NULL, NULL, &ASN_ANY_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t EXTERNAL_descr_={"EXTERNAL", &EXTERNAL_ber_, NULL,
  NULL, &EXTERNAL_xer_, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t EMBEDDED_PDV_descr_={"EMBEDDED PDV",
  &EMBEDDED_PDV_ber_, NULL, NULL, &EMBEDDED_PDV_xer_, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t CHARACTER_STRING_descr_={"CHARACTER STRING",
  &CHARACTER_STRING_ber_, NULL, NULL, &CHARACTER_STRING_xer_, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t ObjectDescriptor_descr_={"ObjectDescriptor",
  &ObjectDescriptor_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::GRAPHICSTRING};

const TTCN_Typedescriptor_t UTF8String_descr_={"UTF8String", &UTF8String_ber_,
  NULL, NULL, &UTF8String_xer_, &UTF8String_json_, NULL, TTCN_Typedescriptor_t::UTF8STRING};

const TTCN_Typedescriptor_t ASN_ROID_descr_={"RELATIVE-OID", &ASN_ROID_ber_,
  NULL, NULL, &ASN_ROID_xer_, &ASN_ROID_json_, NULL, TTCN_Typedescriptor_t::ROID};

const TTCN_Typedescriptor_t NumericString_descr_={"NumericString",
  &NumericString_ber_, NULL, NULL, &NumericString_xer_, &NumericString_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t PrintableString_descr_={"PrintableString",
  &PrintableString_ber_, NULL, NULL, &PrintableString_xer_, &PrintableString_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t TeletexString_descr_={"TeletexString",
  &TeletexString_ber_, NULL, NULL, &TeletexString_xer_, &TeletexString_json_, NULL, TTCN_Typedescriptor_t::TELETEXSTRING};
const TTCN_Typedescriptor_t& T61String_descr_=TeletexString_descr_;

const TTCN_Typedescriptor_t VideotexString_descr_={"VideotexString",
  &VideotexString_ber_, NULL, NULL, &VideotexString_xer_, &VideotexString_json_, NULL, TTCN_Typedescriptor_t::VIDEOTEXSTRING};

const TTCN_Typedescriptor_t IA5String_descr_={"IA5String", &IA5String_ber_,
  NULL, NULL, &IA5String_xer_, &IA5String_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t ASN_GeneralizedTime_descr_={"GeneralizedTime",
  &ASN_GeneralizedTime_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t ASN_UTCTime_descr_={"UTCTime", &ASN_UTCTime_ber_,
  NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE};

const TTCN_Typedescriptor_t GraphicString_descr_={"GraphicString",
  &GraphicString_ber_, NULL, NULL, &GraphicString_xer_, &GraphicString_json_, NULL, TTCN_Typedescriptor_t::GRAPHICSTRING};

const TTCN_Typedescriptor_t VisibleString_descr_={"VisibleString",
  &VisibleString_ber_, NULL, NULL, &VisibleString_xer_, &VisibleString_json_, NULL, TTCN_Typedescriptor_t::DONTCARE};
const TTCN_Typedescriptor_t& ISO646String_descr_=VisibleString_descr_;

const TTCN_Typedescriptor_t GeneralString_descr_={"GeneralString",
  &GeneralString_ber_, NULL, NULL, &GeneralString_xer_, &GeneralString_json_, NULL, TTCN_Typedescriptor_t::GENERALSTRING};

const TTCN_Typedescriptor_t UniversalString_descr_={"UniversalString",
  &UniversalString_ber_, NULL, NULL, &UniversalString_xer_, &UniversalString_json_, NULL, TTCN_Typedescriptor_t::UNIVERSALSTRING};

const TTCN_Typedescriptor_t BMPString_descr_={"BMPString", &BMPString_ber_,
  NULL, NULL, &BMPString_xer_, &BMPString_json_, NULL, TTCN_Typedescriptor_t::BMPSTRING};


