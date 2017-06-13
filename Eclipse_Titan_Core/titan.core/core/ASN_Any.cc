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
 *   Forstner, Matyas
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include "ASN_Any.hh"
#include "String_struct.hh"
#include <string.h>
#include "../common/memory.h"

void ASN_ANY::encode(const TTCN_Typedescriptor_t& p_td,
                     TTCN_Buffer& p_buf,
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

void ASN_ANY::decode(const TTCN_Typedescriptor_t& p_td,
                     TTCN_Buffer& p_buf,
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
ASN_ANY::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                        unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(new_tlv) return new_tlv;
  ASN_BER_TLV_t *tmp_tlv=ASN_BER_TLV_t::construct(0, NULL);
  {
    TTCN_EncDec_ErrorContext ec("While checking ANY value: ");
    if(!ASN_BER_str2TLV(val_ptr->n_octets, val_ptr->octets_ptr,
                        *tmp_tlv, BER_ACCEPT_ALL)
       || tmp_tlv->get_len()!=static_cast<size_t>(val_ptr->n_octets))
      TTCN_EncDec_ErrorContext::error
        (TTCN_EncDec::ET_INCOMPL_ANY,
         "The content of an ASN ANY value must be a valid, complete TLV.");
  }
  new_tlv=ASN_BER_TLV_t::construct(0, NULL);
  *new_tlv=*tmp_tlv;
  new_tlv->Tstr=(unsigned char*)Malloc(new_tlv->Tlen);
  new_tlv->Lstr=(unsigned char*)Malloc(new_tlv->Llen);
  new_tlv->V.str.Vstr=(unsigned char*)Malloc(new_tlv->V.str.Vlen);
  memcpy(new_tlv->Tstr, tmp_tlv->Tstr, new_tlv->Tlen);
  memcpy(new_tlv->Lstr, tmp_tlv->Lstr, new_tlv->Llen);
  memcpy(new_tlv->V.str.Vstr, tmp_tlv->V.str.Vstr, new_tlv->V.str.Vlen);
  Free(tmp_tlv);
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean ASN_ANY::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                                const ASN_BER_TLV_t& p_tlv,
                                unsigned L_form)
{
  clean_up();
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec("While decoding ASN ANY type: ");
  if(stripped_tlv.V_tlvs_selected)
    TTCN_EncDec_ErrorContext::error_internal
      ("In ASN_ANY::BER_decode_TLV().");
  size_t pos=0;
  if(p_td.ber->n_tags>0) {
    stripped_tlv.Tlen=0;
    stripped_tlv.Llen=0;
  }
  init_struct(stripped_tlv.get_len());
  memcpy(val_ptr->octets_ptr+pos, stripped_tlv.Tstr, stripped_tlv.Tlen);
  pos+=stripped_tlv.Tlen;
  memcpy(val_ptr->octets_ptr+pos, stripped_tlv.Lstr, stripped_tlv.Llen);
  pos+=stripped_tlv.Llen;
  memcpy(val_ptr->octets_ptr+pos,
         stripped_tlv.V.str.Vstr, stripped_tlv.V.str.Vlen);
  return TRUE;
}
