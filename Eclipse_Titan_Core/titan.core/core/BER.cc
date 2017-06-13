/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "../common/memory.h"
#include "Basetype.hh"
#include "BER.hh"
#include "Error.hh"
#include "Logger.hh"

/** An \c ASN_Tagnumber_t with 7 bits set in the most significant byte */
static const ASN_Tagnumber_t ASN_Tagnumber_t_7msb
=static_cast<ASN_Tagnumber_t>(0x7F)<<(sizeof(ASN_Tagnumber_t)*8-8);

/** A \c size_t value with 8 bits set (0xFF) in the most significant byte */
static const size_t size_t_8msb
=static_cast<size_t>(0xFF)<<(sizeof(size_t)*8-8);

const unsigned long long unsigned_llong_7msb
=static_cast<unsigned long long>(0x7F)<<(sizeof(unsigned long long)*8-8);

const unsigned long unsigned_long_8msb
=static_cast<unsigned long>(0xFF)<<(sizeof(unsigned long)*8-8);

char* ASN_Tag_t::print() const
{
  const char *prefix;
  switch (tagclass) {
  case ASN_TAG_UNDEF:
    prefix = "<UNDEF> ";
    break;
  case ASN_TAG_UNIV:
    prefix = "UNIVERSAL ";
    break;
  case ASN_TAG_APPL:
    prefix = "APPLICATION ";
    break;
  case ASN_TAG_CONT:
    prefix = "";
    break;
  case ASN_TAG_PRIV:
    prefix = "PRIVATE ";
    break;
  default:
    prefix = "<ERROR> ";
    break;
  } // switch
  return mprintf("[%s%u]", prefix, tagnumber);
}

char* ASN_BERdescriptor_t::print_tags() const
{
  if (n_tags == 0) return mcopystr("<no tags>");
  else {
    char *s = NULL;
    for (size_t i = n_tags; i > 0; i--) {
      char *tagstr = tags[i - 1].print();
      s = mputstr(s, tagstr);
      Free(tagstr);
      if (i != 1) s = mputc(s, ' ');
    } // for i
    return s;
  }
}

static const ASN_Tag_t BOOLEAN_tag_[]={{ASN_TAG_UNIV, 1u}};
const ASN_BERdescriptor_t BOOLEAN_ber_={1u, BOOLEAN_tag_};

static const ASN_Tag_t INTEGER_tag_[]={{ASN_TAG_UNIV, 2u}};
const ASN_BERdescriptor_t INTEGER_ber_={1u, INTEGER_tag_};

static const ASN_Tag_t BITSTRING_tag_[]={{ASN_TAG_UNIV, 3u}};
const ASN_BERdescriptor_t BITSTRING_ber_={1u, BITSTRING_tag_};

static const ASN_Tag_t OCTETSTRING_tag_[]={{ASN_TAG_UNIV, 4u}};
const ASN_BERdescriptor_t OCTETSTRING_ber_={1u, OCTETSTRING_tag_};

static const ASN_Tag_t ASN_NULL_tag_[]={{ASN_TAG_UNIV, 5u}};
const ASN_BERdescriptor_t ASN_NULL_ber_={1u, ASN_NULL_tag_};

static const ASN_Tag_t OBJID_tag_[]={{ASN_TAG_UNIV, 6u}};
const ASN_BERdescriptor_t OBJID_ber_={1u, OBJID_tag_};

static const ASN_Tag_t ObjectDescriptor_tag_[]={{ASN_TAG_UNIV, 7u}};
const ASN_BERdescriptor_t ObjectDescriptor_ber_={1u, ObjectDescriptor_tag_};

static const ASN_Tag_t EXTERNAL_tag_[]={{ASN_TAG_UNIV, 8u}};
const ASN_BERdescriptor_t EXTERNAL_ber_={1u, EXTERNAL_tag_};

static const ASN_Tag_t REAL_tag_[]={{ASN_TAG_UNIV, 9u}};
const ASN_BERdescriptor_t FLOAT_ber_={1u, REAL_tag_};

static const ASN_Tag_t ENUMERATED_tag_[]={{ASN_TAG_UNIV, 10u}};
const ASN_BERdescriptor_t ENUMERATED_ber_={1u, ENUMERATED_tag_};

static const ASN_Tag_t EMBEDDED_PDV_tag_[]={{ASN_TAG_UNIV, 11u}};
const ASN_BERdescriptor_t EMBEDDED_PDV_ber_={1u, EMBEDDED_PDV_tag_};

static const ASN_Tag_t UTF8String_tag_[]={{ASN_TAG_UNIV, 12u}};
const ASN_BERdescriptor_t UTF8String_ber_={1u, UTF8String_tag_};

static const ASN_Tag_t ASN_ROID_tag_[]={{ASN_TAG_UNIV, 13u}};
const ASN_BERdescriptor_t ASN_ROID_ber_={1u, ASN_ROID_tag_};

static const ASN_Tag_t SEQUENCE_tag_[]={{ASN_TAG_UNIV, 16u}};
const ASN_BERdescriptor_t SEQUENCE_ber_={1u, SEQUENCE_tag_};

static const ASN_Tag_t SET_tag_[]={{ASN_TAG_UNIV, 17u}};
const ASN_BERdescriptor_t SET_ber_={1u, SET_tag_};

const ASN_BERdescriptor_t CHOICE_ber_={0u, NULL};

const ASN_BERdescriptor_t ASN_ANY_ber_={0u, NULL};

static const ASN_Tag_t NumericString_tag_[]={{ASN_TAG_UNIV, 18u}};
const ASN_BERdescriptor_t NumericString_ber_={1u, NumericString_tag_};

static const ASN_Tag_t PrintableString_tag_[]={{ASN_TAG_UNIV, 19u}};
const ASN_BERdescriptor_t PrintableString_ber_={1u, PrintableString_tag_};

static const ASN_Tag_t TeletexString_tag_[]={{ASN_TAG_UNIV, 20u}};
const ASN_BERdescriptor_t TeletexString_ber_={1u, TeletexString_tag_};

static const ASN_Tag_t VideotexString_tag_[]={{ASN_TAG_UNIV, 21u}};
const ASN_BERdescriptor_t VideotexString_ber_={1u, VideotexString_tag_};

static const ASN_Tag_t IA5String_tag_[]={{ASN_TAG_UNIV, 22u}};
const ASN_BERdescriptor_t IA5String_ber_={1u, IA5String_tag_};

static const ASN_Tag_t ASN_UTCTime_tag_[]={{ASN_TAG_UNIV, 23u}};
const ASN_BERdescriptor_t ASN_UTCTime_ber_={1u, ASN_UTCTime_tag_};

static const ASN_Tag_t ASN_GeneralizedTime_tag_[]={{ASN_TAG_UNIV, 24u}};
const ASN_BERdescriptor_t ASN_GeneralizedTime_ber_={1u, ASN_GeneralizedTime_tag_};

static const ASN_Tag_t GraphicString_tag_[]={{ASN_TAG_UNIV, 25u}};
const ASN_BERdescriptor_t GraphicString_ber_={1u, GraphicString_tag_};

static const ASN_Tag_t VisibleString_tag_[]={{ASN_TAG_UNIV, 26u}};
const ASN_BERdescriptor_t VisibleString_ber_={1u, VisibleString_tag_};

const ASN_Tag_t GeneralString_tag_[]={{ASN_TAG_UNIV, 27u}};
const ASN_BERdescriptor_t GeneralString_ber_={1u, GeneralString_tag_};

static const ASN_Tag_t UniversalString_tag_[]={{ASN_TAG_UNIV, 28u}};
const ASN_BERdescriptor_t UniversalString_ber_={1u, UniversalString_tag_};

static const ASN_Tag_t CHARACTER_STRING_tag_[]={{ASN_TAG_UNIV, 29u}};
const ASN_BERdescriptor_t CHARACTER_STRING_ber_={1u, CHARACTER_STRING_tag_};

static const ASN_Tag_t BMPString_tag_[]={{ASN_TAG_UNIV, 30u}};
const ASN_BERdescriptor_t BMPString_ber_={1u, BMPString_tag_};

ASN_BER_TLV_t* ASN_BER_TLV_t::construct(ASN_BER_TLV_t *p_tlv)
{
  ASN_BER_TLV_t *new_tlv = (ASN_BER_TLV_t*)Malloc(sizeof(*new_tlv));
  new_tlv->isConstructed = TRUE;
  new_tlv->V_tlvs_selected = TRUE;
  new_tlv->isLenDefinite = FALSE;
  new_tlv->isLenShort = FALSE;
  new_tlv->isTagComplete = FALSE;
  new_tlv->isComplete = FALSE;
  new_tlv->tagclass = ASN_TAG_UNIV;
  new_tlv->tagnumber = 0;
  new_tlv->Tlen = 0;
  new_tlv->Llen = 0;
  new_tlv->Tstr = NULL;
  new_tlv->Lstr = NULL;
  if (p_tlv != NULL) {
    new_tlv->V.tlvs.n_tlvs = 1;
    new_tlv->V.tlvs.tlvs = (ASN_BER_TLV_t**)
      Malloc(sizeof(*new_tlv->V.tlvs.tlvs));
    new_tlv->V.tlvs.tlvs[0] = p_tlv;
  } else {
    new_tlv->V.tlvs.n_tlvs = 0;
    new_tlv->V.tlvs.tlvs = NULL;
  }
  return new_tlv;
}

ASN_BER_TLV_t* ASN_BER_TLV_t::construct(size_t p_Vlen, unsigned char *p_Vstr)
{
  ASN_BER_TLV_t *new_tlv = (ASN_BER_TLV_t*)Malloc(sizeof(*new_tlv));
  new_tlv->isConstructed = FALSE;
  new_tlv->V_tlvs_selected = FALSE;
  new_tlv->isLenDefinite = FALSE;
  new_tlv->isLenShort = FALSE;
  new_tlv->isTagComplete = FALSE;
  new_tlv->isComplete = FALSE;
  new_tlv->tagclass = ASN_TAG_UNIV;
  new_tlv->tagnumber = 0;
  new_tlv->Tlen = 0;
  new_tlv->Llen = 0;
  new_tlv->Tstr = NULL;
  new_tlv->Lstr = NULL;
  new_tlv->V.str.Vlen = p_Vlen;
  if (p_Vstr != NULL) new_tlv->V.str.Vstr = p_Vstr;
  else new_tlv->V.str.Vstr = (unsigned char*)Malloc(p_Vlen);
  return new_tlv;
}

ASN_BER_TLV_t* ASN_BER_TLV_t::construct()
{
  ASN_BER_TLV_t *new_tlv = (ASN_BER_TLV_t*)Malloc(sizeof(*new_tlv));
  new_tlv->isConstructed = FALSE;
  new_tlv->V_tlvs_selected = FALSE;
  new_tlv->isLenDefinite = FALSE;
  new_tlv->isLenShort = FALSE;
  new_tlv->isTagComplete = FALSE;
  new_tlv->isComplete = FALSE;
  new_tlv->tagclass = ASN_TAG_UNIV;
  new_tlv->tagnumber = 0;
  new_tlv->Tlen = 0;
  new_tlv->Llen = 0;
  new_tlv->Tstr = NULL;
  new_tlv->Lstr = NULL;
  new_tlv->V.str.Vlen = 0;
  new_tlv->V.str.Vstr = NULL;
  return new_tlv;
}

void ASN_BER_TLV_t::destruct(ASN_BER_TLV_t *p_tlv, boolean no_str)
{
  if (p_tlv == NULL) return;
  if(!no_str) {
    Free(p_tlv->Tstr);
    Free(p_tlv->Lstr);
  }
  if(p_tlv->V_tlvs_selected) {
    for(size_t i=0; i<p_tlv->V.tlvs.n_tlvs; i++)
      ASN_BER_TLV_t::destruct(p_tlv->V.tlvs.tlvs[i], no_str);
    Free(p_tlv->V.tlvs.tlvs);
  }
  else {
    if(!no_str)
      Free(p_tlv->V.str.Vstr);
  }
  Free(p_tlv);
}

void ASN_BER_TLV_t::chk_constructed_flag(boolean flag_expected) const
{
  if (Tlen > 0 && isConstructed != flag_expected)
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_INVAL_MSG,
       "Invalid 'constructed' flag (must be %sset).",
       flag_expected?"":"un");

}

void ASN_BER_TLV_t::add_TLV(ASN_BER_TLV_t *p_tlv)
{
  if(!isConstructed || !V_tlvs_selected)
    TTCN_EncDec_ErrorContext::error_internal
      ("ASN_BER_TLV_t::add_TLV() invoked for a non-constructed TLV.");
  V.tlvs.n_tlvs++;
  V.tlvs.tlvs=(ASN_BER_TLV_t**)
    Realloc(V.tlvs.tlvs, V.tlvs.n_tlvs*sizeof(*V.tlvs.tlvs));
  V.tlvs.tlvs[V.tlvs.n_tlvs-1]=p_tlv;
}

void ASN_BER_TLV_t::add_UNIV0_TLV()
{
  ASN_BER_TLV_t *new_tlv=(ASN_BER_TLV_t*)Malloc(sizeof(*new_tlv));
  new_tlv->isConstructed=FALSE;
  new_tlv->V_tlvs_selected=FALSE;
  new_tlv->isLenDefinite=TRUE;
  new_tlv->isLenShort=TRUE;
  new_tlv->tagclass=ASN_TAG_UNIV;
  new_tlv->tagnumber=0;
  new_tlv->Tlen=1;
  new_tlv->Tstr=(unsigned char*)Malloc(1);
  new_tlv->Tstr[0]=0x00;
  new_tlv->Llen=1;
  new_tlv->Lstr=(unsigned char*)Malloc(1);
  new_tlv->Lstr[0]=0x00;
  new_tlv->V.str.Vlen=0;
  new_tlv->V.str.Vstr=NULL;
  add_TLV(new_tlv);
}

size_t ASN_BER_TLV_t::get_len() const
{
  size_t len=Tlen+Llen;
  if(!V_tlvs_selected)
    len+=V.str.Vlen;
  else
    for(size_t i=0; i<V.tlvs.n_tlvs; i++)
      len+=V.tlvs.tlvs[i]->get_len();
  return len;
}

void ASN_BER_TLV_t::add_TL(ASN_Tagclass_t p_tagclass,
                           ASN_Tagnumber_t p_tagnumber,
                           unsigned coding)
{
  TTCN_EncDec_ErrorContext ec("ASN_BER_TLV_t::add_TL(): ");
  tagclass=p_tagclass;
  tagnumber=p_tagnumber;
  /* L-part */
  if(coding==BER_ENCODE_CER && isConstructed) {
    isLenDefinite=FALSE;
    add_UNIV0_TLV();
  }
  else {
    isLenDefinite=TRUE;
  }
  size_t Vlen=0;
  if(isLenDefinite) {
    Tlen=Llen=0;
    Vlen=get_len();
    if(Vlen>127) {
      isLenShort=FALSE;
      Llen=1+(min_needed_bits(Vlen)+7)/8;
    }
    else {
      isLenShort=TRUE;
      Llen=1;
    }
  }
  else Llen=1;
  Lstr=(unsigned char*)Malloc(Llen);
  if(isLenDefinite) {
    if(isLenShort) Lstr[0]=(unsigned char)Vlen;
    else { // long form
      size_t i=Llen-1; // number of needed octets
      Lstr[0]=(i & 0x7F) | 0x80;
      size_t tmp=Vlen;
      while(i) {
        Lstr[i]=tmp & 0xFF;
        i--;
        tmp>>=8;
      }
    }
  } // isLenDefinite
  else { // indefinite form
    Lstr[0]=0x80;
  }
  /* T-part */
  if(tagnumber>30) Tlen=1+(min_needed_bits(tagnumber)+6)/7;
  else Tlen=1;
  Tstr=(unsigned char*)Malloc(Tlen);

  switch(tagclass) {
  case ASN_TAG_UNIV: Tstr[0]=0x00; break;
  case ASN_TAG_APPL: Tstr[0]=0x40; break;
  case ASN_TAG_CONT: Tstr[0]=0x80; break;
  case ASN_TAG_PRIV: Tstr[0]=0xC0; break;
  case ASN_TAG_UNDEF:
  default:
    ec.error_internal("Unhandled case or undefined tagclass.");
    break;
  }
  if(isConstructed) Tstr[0]|=0x20;
  if(tagnumber<=30) Tstr[0]|=(unsigned char)tagnumber;
  else {
    Tstr[0]|=0x1F;
    size_t tmp=tagnumber;
    size_t i=Tlen-1; // number of needed octets
    while(i) {
      Tstr[i]=(tmp & 0x7F) | 0x80;
      i--;
      tmp>>=7;
    }
    Tstr[Tlen-1]&=0x7F;
  }
  isComplete = TRUE;
  isTagComplete = TRUE;
}

void ASN_BER_TLV_t::put_in_buffer(TTCN_Buffer& p_buf)
{
  p_buf.put_s(Tlen, Tstr);
  p_buf.put_s(Llen, Lstr);

  if(!V_tlvs_selected)
    p_buf.put_s(V.str.Vlen, V.str.Vstr);
  else
    for(size_t i=0; i<V.tlvs.n_tlvs; i++)
      V.tlvs.tlvs[i]->put_in_buffer(p_buf);
}

unsigned char ASN_BER_TLV_t::get_pos(size_t p_pos) const
{
  size_t pos=p_pos;
  boolean success=FALSE;
  unsigned char c=_get_pos(pos, success);
  if(!success)
    TTCN_EncDec_ErrorContext::error_internal
      ("Index overflow in ASN_BER_TLV_t::get_pos()");
  return c;
}

unsigned char ASN_BER_TLV_t::_get_pos(size_t& pos, boolean& success) const
{
  if(pos<Tlen) {success=TRUE; return Tstr[pos];}
  pos-=Tlen;
  if(pos<Llen) {success=TRUE; return Lstr[pos];}
  pos-=Llen;

  if(!V_tlvs_selected) {
    if(pos<V.str.Vlen) {success=TRUE; return V.str.Vstr[pos];}
    pos-=V.str.Vlen;
  }
  else { // V_tlvs_selected
    for(size_t i=0; i<V.tlvs.n_tlvs; i++) {
      unsigned char c=V.tlvs.tlvs[i]->_get_pos(pos, success);
      if(success) return c;
    }
  }
  success=FALSE; return 0;
}

int ASN_BER_TLV_t::compare(const ASN_BER_TLV_t *other) const
{
  size_t pos=0, pos1, pos2;
  boolean success1;
  boolean success2;
  unsigned char c1, c2;
  do {
    pos1=pos2=pos;
    c1=_get_pos(pos1, success1);
    c2=other->_get_pos(pos2, success2);
    if(!success1 && !success2) return 0;
    if(c1<c2) return -1;
    if(c1>c2) return 1;
    pos++;
  } while(1);
}

/** This functions is used by qsort() in
  * ASN_BER_TLV_t::sort_tlvs(). */
int ASN_BER_compare_TLVs(const void *p1, const void *p2)
{
  const ASN_BER_TLV_t *left=*((ASN_BER_TLV_t const* const*)p1);
  const ASN_BER_TLV_t *right=*((ASN_BER_TLV_t const* const*)p2);
  return left->compare(right);
}

void ASN_BER_TLV_t::sort_tlvs()
{
  if(!V_tlvs_selected)
    TTCN_EncDec_ErrorContext::error_internal
      ("ASN_BER_TLV_t::sort_tlvs() called but !V_tlvs_selected");
  qsort(V.tlvs.tlvs, V.tlvs.n_tlvs, sizeof(ASN_BER_TLV_t*),
        ASN_BER_compare_TLVs);
}

int ASN_BER_TLV_t::compare_tags(const ASN_BER_TLV_t *other) const
{
  if(tagclass<other->tagclass) return -1;
  if(tagclass>other->tagclass) return 1;
  if(tagnumber<other->tagnumber) return -1;
  if(tagnumber>other->tagnumber) return 1;
  return 0;
}

/** This functions is used by qsort() in
  * ASN_BER_TLV_t::sort_tlvs_tag(). */
int ASN_BER_compare_TLVs_tag(const void *p1, const void *p2)
{
  const ASN_BER_TLV_t *left=*((ASN_BER_TLV_t const* const*)p1);
  const ASN_BER_TLV_t *right=*((ASN_BER_TLV_t const* const*)p2);
  return left->compare_tags(right);
}

void ASN_BER_TLV_t::sort_tlvs_tag()
{
  if(!V_tlvs_selected)
    TTCN_EncDec_ErrorContext::error_internal
      ("ASN_BER_TLV_t::sort_tlvs_tag() called but !V_tlvs_selected");
  qsort(V.tlvs.tlvs, V.tlvs.n_tlvs, sizeof(ASN_BER_TLV_t*),
        ASN_BER_compare_TLVs_tag);
}

boolean ASN_BER_str2TLV(size_t p_len_s,
                        const unsigned char* p_str,
                        ASN_BER_TLV_t& tlv,
                        unsigned L_form)
{
  size_t curr_pos=0;
  unsigned char c, c2;
  boolean doit;
  TTCN_EncDec_ErrorContext ec("While splitting TLV: ");

  tlv.isConstructed=FALSE;
  tlv.V_tlvs_selected=FALSE;
  tlv.isLenDefinite=TRUE;
  tlv.isLenShort=TRUE;
  tlv.isTagComplete=FALSE;
  tlv.isComplete=FALSE;
  tlv.tagclass=ASN_TAG_UNIV;
  tlv.tagnumber=0;
  tlv.Tlen=0;
  tlv.Llen=0;
  tlv.V.str.Vlen=0;
  tlv.Tstr=NULL;
  tlv.Lstr=NULL;
  tlv.V.str.Vstr=NULL;

  if(p_len_s==0)
    goto incomplete;
  tlv.Tstr=const_cast<unsigned char*>(p_str);
  c=p_str[0];
  switch((c & 0xC0) >> 6) {
  case 0: tlv.tagclass=ASN_TAG_UNIV; break;
  case 1: tlv.tagclass=ASN_TAG_APPL; break;
  case 2: tlv.tagclass=ASN_TAG_CONT; break;
  case 3: tlv.tagclass=ASN_TAG_PRIV; break;
  }
  if(c & 0x20) tlv.isConstructed=TRUE;
  else tlv.isConstructed=FALSE;
  c2=c & 0x1F;
  if(c2 != 0x1F) // low tag number
    tlv.tagnumber=c2;
  else { // high tag number
    tlv.tagnumber=0; doit=TRUE;
    boolean err_repr=FALSE;
    while(doit) {
      curr_pos++;
      if(curr_pos>=p_len_s)
        goto incomplete;
      c=p_str[curr_pos];
      if(!err_repr) {
        if(tlv.tagnumber & ASN_Tagnumber_t_7msb) {
          err_repr=TRUE;
          ec.error(TTCN_EncDec::ET_REPR, "Tag number is too big.");
          tlv.tagnumber=~(ASN_Tagnumber_t)0;
        } // if 7msb on
        else {
          tlv.tagnumber<<=7;
          tlv.tagnumber|=c & 0x7F;
        }
      } // !err_repr
      if(!(c & 0x80)) doit=FALSE;
    }
  } // high tag number
  tlv.isTagComplete=TRUE;
  curr_pos++;
  if(curr_pos>=p_len_s)
    goto incomplete;

  tlv.Lstr=const_cast<unsigned char*>(p_str+curr_pos);
  tlv.Tlen=tlv.Lstr-tlv.Tstr;
  tlv.isLenDefinite=TRUE;
  tlv.isLenShort=FALSE;
  c=p_str[curr_pos];
  if(!(c & 0x80)) { // short form
    tlv.Llen=1;
    tlv.V.str.Vlen=c;
    tlv.isLenShort=TRUE;
    if(!(L_form & BER_ACCEPT_SHORT)) {
      ec.error(TTCN_EncDec::ET_LEN_FORM,
               "Short length form is not acceptable.");
    } // if wrong L_form
  }
  else {
    if(c==0x80) { // indefinite len
      tlv.Llen=1;
      tlv.isLenDefinite=FALSE;
      if(!(L_form & BER_ACCEPT_INDEFINITE)) {
        ec.error(TTCN_EncDec::ET_LEN_FORM,
                 "Indefinite length form is not acceptable.");
      } // if wrong L_form
    }
    else if(c==0xFF) { // reserved len
      ec.error(TTCN_EncDec::ET_INVAL_MSG,
               "Error in L: In the long form, the value 0xFF shall"
               " not be used (see X.690 clause 8.1.3.5.c)).");
      // using as normal long form
    } // if reserved len
    else { // long form
      if(!(L_form & BER_ACCEPT_LONG)) {
        ec.error(TTCN_EncDec::ET_LEN_FORM,
                 "Long length form is not acceptable.");
      } // if wrong L_form
      tlv.Llen=(c & 0x7F)+1;
      if(tlv.Tlen+tlv.Llen>p_len_s) {
        tlv.Llen=p_len_s-tlv.Tlen;
        goto incomplete;
      }
      tlv.V.str.Vlen=0;
      boolean err_repr=FALSE;
      for(size_t i=tlv.Llen-1; i; i--) {
        if(!err_repr) {
          if(tlv.V.str.Vlen & size_t_8msb) {
            err_repr=TRUE;
            ec.error(TTCN_EncDec::ET_REPR,
                     "In long form L: Length of V is too big.");
            tlv.V.str.Vlen=~(size_t)0;
          } // if 8msb on
          else
            tlv.V.str.Vlen<<=8;
        } // if !err_repr
        curr_pos++;
        if(!err_repr) {
          c=p_str[curr_pos];
          tlv.V.str.Vlen|=c;
        } // if !err_repr
      } // for i
    } // if long form
  }
  curr_pos++;
  tlv.V.str.Vstr=const_cast<unsigned char*>(p_str+curr_pos);
  if(tlv.isLenDefinite) {
    if(tlv.V.str.Vlen>p_len_s-tlv.Tlen-tlv.Llen) {
      goto incomplete;
    }
  } // definite len
  else { // indefinite len for V
    if(!tlv.isConstructed) {
      ec.error(TTCN_EncDec::ET_INVAL_MSG,
               "A sender can use the indefinite form only if the"
               " encoding is constructed (see X.690 clause 8.1.3.2.a).");
    } // if !isConstructed

    TTCN_EncDec_ErrorContext tmp_ec;
    ASN_BER_TLV_t tmp_tlv;
    size_t tmp_len;
    size_t i=1;
    doit=TRUE;
    while(doit) {
      tmp_ec.set_msg("While checking constructed V part #%lu: ",
        (unsigned long) i);
      if(!ASN_BER_str2TLV(p_len_s-curr_pos, &p_str[curr_pos],
                          tmp_tlv, BER_ACCEPT_ALL))
        goto incomplete;
      tmp_len=tmp_tlv.get_len();
      curr_pos+=tmp_len;
      tlv.V.str.Vlen+=tmp_len;
      if(tmp_tlv.tagclass==ASN_TAG_UNIV && tmp_tlv.tagnumber==0)
        doit=FALSE; // End-of-contents
      i++;
    } // while doit
    // tlv.V.str.Vlen=&p_str[curr_pos]-tlv.V.str.Vstr;
  } // if indefinite len for V
  tlv.isComplete=TRUE;
  return TRUE;
 incomplete:
  if(tlv.Tlen==0) tlv.Tlen=p_len_s;
  if(tlv.V.str.Vstr!=NULL && tlv.V.str.Vstr>tlv.Lstr+tlv.Llen)
    tlv.Llen=tlv.V.str.Vstr-tlv.Lstr;
  if(tlv.Tlen+tlv.Llen+tlv.V.str.Vlen>p_len_s)
    tlv.V.str.Vlen=p_len_s-tlv.Tlen-tlv.Llen;
  return FALSE;
}

ASN_BER_TLV_t* ASN_BER_V2TLV(ASN_BER_TLV_t* p_tlv,
                             const TTCN_Typedescriptor_t& p_td,
                             unsigned coding)
{
  if(p_td.ber->n_tags==0) return p_tlv;
  ASN_BER_TLV_t *tlv2;
  if(!(p_tlv->tagclass==ASN_TAG_UNIV && p_tlv->tagnumber==0))
    tlv2=ASN_BER_TLV_t::construct(p_tlv);
  else tlv2=p_tlv;
  const ASN_BERdescriptor_t *ber=p_td.ber;
  for(size_t i=0; i<ber->n_tags; i++) {
    const ASN_Tag_t *tag=ber->tags+i;
    tlv2->add_TL(tag->tagclass, tag->tagnumber, coding);
    if(i!=ber->n_tags-1)
      tlv2=ASN_BER_TLV_t::construct(tlv2);
  } // for i
  return tlv2;
}
