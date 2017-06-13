/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   >
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
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include <string.h>
#include <math.h>
#include <float.h>

#include "../common/memory.h"
#include "Float.hh"
#include "Types.h"
#include "Param_Types.hh"
#include "Optional.hh"
#include "Logger.hh"
#include "Error.hh"
#include "Encdec.hh"
#include "RAW.hh"
#include "BER.hh"
#include "Charstring.hh"
#include "Addfunc.hh"
#include "XmlReader.hh"

#include "../common/dbgnew.hh"


#ifndef INFINITY
#define INFINITY (DBL_MAX*DBL_MAX)
#endif
const FLOAT PLUS_INFINITY(INFINITY);
const FLOAT MINUS_INFINITY(-INFINITY);

#ifdef NAN
const FLOAT NOT_A_NUMBER(NAN);
#else
const FLOAT NOT_A_NUMBER((double)PLUS_INFINITY+(double)MINUS_INFINITY);
// The casts ensure that FLOAT::operator+ is not called
#endif


static inline void log_float(double float_val)
{
  if (   (float_val > -MAX_DECIMAL_FLOAT && float_val <= -MIN_DECIMAL_FLOAT)
      || (float_val >= MIN_DECIMAL_FLOAT && float_val <   MAX_DECIMAL_FLOAT)
      || (float_val == 0.0))
    TTCN_Logger::log_event("%f", float_val);
  else if(float_val==INFINITY)
    TTCN_Logger::log_event_str("infinity");
  else if(float_val==-INFINITY)
    TTCN_Logger::log_event_str("-infinity");
  else if(float_val!=float_val)
    TTCN_Logger::log_event_str("not_a_number");
  else TTCN_Logger::log_event("%e", float_val);
}

// float value class

FLOAT::FLOAT()
{
  bound_flag = FALSE;
}

FLOAT::FLOAT(double other_value)
{
  bound_flag = TRUE;
  float_value = other_value;
}

FLOAT::FLOAT(const FLOAT& other_value)
: Base_Type(other_value)
{
  other_value.must_bound("Copying an unbound float value.");
  bound_flag = TRUE;
  float_value = other_value.float_value;
}

void FLOAT::clean_up()
{
  bound_flag = FALSE;
}

FLOAT& FLOAT::operator=(double other_value)
{
  bound_flag = TRUE;
  float_value = other_value;
  return *this;
}

FLOAT& FLOAT::operator=(const FLOAT& other_value)
{
  other_value.must_bound("Assignment of an unbound float value.");
  bound_flag = TRUE;
  float_value = other_value.float_value;
  return *this;
}

double FLOAT::operator+() const
{
  must_bound("Unbound float operand of unary + operator.");
  return float_value;
}

double FLOAT::operator-() const
{
  must_bound("Unbound float operand of unary - operator (negation).");
  return -float_value;
}

boolean FLOAT::is_special(double flt_val)
{
  return ( (flt_val!=flt_val) || (flt_val==INFINITY) || (flt_val==-INFINITY) );
}

void FLOAT::check_numeric(double flt_val, const char *err_msg_begin)
{
  if (is_special(flt_val)) {
    TTCN_error("%s must be a numeric value instead of %g",
      err_msg_begin, flt_val);
  }
}

double FLOAT::operator+(double other_value) const
{
  must_bound("Unbound left operand of float addition.");
  return float_value + other_value;
}

double FLOAT::operator+(const FLOAT& other_value) const
{
  must_bound("Unbound left operand of float addition.");
  other_value.must_bound("Unbound right operand of float addition.");
  return float_value + other_value.float_value;
}

double FLOAT::operator-(double other_value) const
{
  must_bound("Unbound left operand of float subtraction.");
  return float_value - other_value;
}

double FLOAT::operator-(const FLOAT& other_value) const
{
  must_bound("Unbound left operand of float subtraction.");
  other_value.must_bound("Unbound right operand of float subtraction.");
  return float_value - other_value.float_value;
}

double FLOAT::operator*(double other_value) const
{
  must_bound("Unbound left operand of float multiplication.");
  return float_value * other_value;
}

double FLOAT::operator*(const FLOAT& other_value) const
{
  must_bound("Unbound left operand of float multiplication.");
  other_value.must_bound("Unbound right operand of float multiplication.");
  return float_value * other_value.float_value;
}

double FLOAT::operator/(double other_value) const
{
  must_bound("Unbound left operand of float division.");
  if (other_value == 0.0) TTCN_error("Float division by zero.");
  return float_value / other_value;
}

double FLOAT::operator/(const FLOAT& other_value) const
{
  must_bound("Unbound left operand of float division.");
  other_value.must_bound("Unbound right operand of float division.");
  if (other_value.float_value == 0.0) TTCN_error("Float division by zero.");
  return float_value / other_value.float_value;
}

boolean FLOAT::operator==(double other_value) const
{
  must_bound("Unbound left operand of float comparison.");
  return float_value == other_value;
}

boolean FLOAT::operator==(const FLOAT& other_value) const
{
  must_bound("Unbound left operand of float comparison.");
  other_value.must_bound("Unbound right operand of float comparison.");
  return float_value == other_value.float_value;
}

boolean FLOAT::operator<(double other_value) const
{
  must_bound("Unbound left operand of float comparison.");
  return float_value < other_value;
}

boolean FLOAT::operator<(const FLOAT& other_value) const
{
  must_bound("Unbound left operand of float comparison.");
  other_value.must_bound("Unbound right operand of float comparison.");
  return float_value < other_value.float_value;
}

boolean FLOAT::operator>(double other_value) const
{
  must_bound("Unbound left operand of float comparison.");
  return float_value > other_value;
}

boolean FLOAT::operator>(const FLOAT& other_value) const
{
  must_bound("Unbound left operand of float comparison.");
  other_value.must_bound("Unbound right operand of float comparison.");
  return float_value > other_value.float_value;
}

FLOAT::operator double() const
{
  must_bound("Using the value of an unbound float variable.");
  return float_value;
}

void FLOAT::log() const
{
  if (bound_flag) log_float(float_value);
  else TTCN_Logger::log_event_unbound();
}

void FLOAT::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE, "float value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Float: {
    clean_up();
    bound_flag = TRUE;
    float_value = mp->get_float();
    break; }
  case Module_Param::MP_Expression:
    switch (mp->get_expr_type()) {
    case Module_Param::EXPR_NEGATE: {
      FLOAT operand;
      operand.set_param(*mp->get_operand1());
      *this = - operand;
      break; }
    case Module_Param::EXPR_ADD: {
      FLOAT operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 + operand2;
      break; }
    case Module_Param::EXPR_SUBTRACT: {
      FLOAT operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 - operand2;
      break; }
    case Module_Param::EXPR_MULTIPLY: {
      FLOAT operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 * operand2;
      break; }
    case Module_Param::EXPR_DIVIDE: {
      FLOAT operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      if (operand2 == 0.0) {
        param.error("Floating point division by zero.");
      }
      *this = operand1 / operand2;
      break; }
    default:
      param.expr_type_error("a float");
      break;
    }
    break;
  default:
    param.type_error("float value");
    break;
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* FLOAT::get_param(Module_Param_Name& /* param_name */) const
{
  if (!bound_flag) {
    return new Module_Param_Unbound();
  }
  return new Module_Param_Float(float_value);
}
#endif

void FLOAT::encode_text(Text_Buf& text_buf) const
{
  must_bound("Text encoder: Encoding an unbound float value.");
  text_buf.push_double(float_value);
}

void FLOAT::decode_text(Text_Buf& text_buf)
{
  bound_flag = TRUE;
  float_value = text_buf.pull_double();
}

void FLOAT::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
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

void FLOAT::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
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
FLOAT::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                      unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(!new_tlv) {
    if(float_value==0.0) {
      new_tlv=ASN_BER_TLV_t::construct();
      // nothing to do, Vlen is 0
    }
    /* +Infinity */
    else if(float_value==(double)INFINITY) { // INFINITY may be float => cast
      new_tlv=ASN_BER_TLV_t::construct(1, NULL);
      new_tlv->V.str.Vstr[0]=0x40;
    }
    /* -Infinity */
    else if(float_value==-(double)INFINITY) {
      new_tlv=ASN_BER_TLV_t::construct(1, NULL);
      new_tlv->V.str.Vstr[0]=0x41;
    }
    else if(isnan((double)float_value)) {
      TTCN_EncDec_ErrorContext::error_internal("Value is NaN.");
    }
    else {
      new_tlv=ASN_BER_TLV_t::construct();
      double mantissa, exponent;
      exponent=floor(log10(fabs(float_value)))+1.0-DBL_DIG;
      mantissa=floor(float_value*pow(10.0,-exponent)+0.5);
      if(mantissa)while(!fmod(mantissa,10.0))mantissa/=10.0,exponent+=1.0;
      /** \todo review
          gcc 2.95:
          in mprintf below:
          warning: `.' not followed by `*' or digit in format
       */
      new_tlv->V.str.Vstr=(unsigned char*)
        mprintf("\x03%.f.E%s%.0f", mantissa, exponent==0.0?"+":"", exponent);
      new_tlv->V.str.Vlen=1+strlen((const char*)&new_tlv->V.str.Vstr[1]);
    }
  }
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean FLOAT::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                              const ASN_BER_TLV_t& p_tlv,
                              unsigned L_form)
{
  bound_flag = FALSE;
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec("While decoding REAL type: ");
  stripped_tlv.chk_constructed_flag(FALSE);
  if (!stripped_tlv.isComplete) return FALSE;
  size_t Vlen=stripped_tlv.V.str.Vlen;
  unsigned char *Vstr=stripped_tlv.V.str.Vstr;
  if(Vlen==0) {
    float_value=0.0;
  }
  else if(Vstr[0] & 0x80) {
    /* binary encoding */
    /** \todo Perhaps it were good to implement this. Perhaps not. :) */
    ec.warning("Sorry, decoding of binary encoded REAL values not"
               " supported.");
    float_value=0.0;
  }
  else if(Vstr[0] & 0x40) {
    /* SpecialRealValue */
    if(Vlen>1)
      ec.error(TTCN_EncDec::ET_INVAL_MSG,
               "In case of SpecialRealValue, the length of V-part must be 1"
               " (See X.690 8.5.8).");
    if(Vstr[0] & 0x3E)
      ec.error(TTCN_EncDec::ET_INVAL_MSG,
               "This is a reserved value: 0x%x (See X.690 8.5.8).",
               Vstr[0]);
    if(Vstr[0] & 0x01)
      /* MINUS-INFINITY */
      float_value=-INFINITY;
    else
      /* PLUS-INFINITY */
      float_value=INFINITY;
  }
  else {
    /* decimal encoding */
    if((Vstr[0] & 0x3C) || (Vstr[0] & 0x3F) == 0x00 )
      ec.error(TTCN_EncDec::ET_INVAL_MSG,
               "This is a reserved value: 0x%x (See X.690 8.5.7).",
               Vstr[0]);
    int NR=Vstr[0] & 0x03; // which NumericalRepresentation
    boolean
      leadingzero=FALSE,
      NR_error=FALSE;
    unsigned char
      *Vstr_last=Vstr+Vlen-1,
      *sign=NULL,
      *mant1=NULL,
      *decmark=NULL,
      *mant2=NULL,
      *expmark=NULL,
      *expsign=NULL,
      *expo=NULL,
      *ptr=Vstr+1;
    size_t
      mant1_len=0,
      mant2_len=0,
      expo_len=0;
    long exponum;
    if(Vlen==1) goto dec_error;
    while(*ptr==' ') {
      if(ptr==Vstr_last) goto dec_error;
      ptr++;
    }
    if(*ptr=='+' || *ptr=='-') {
      sign=ptr;
      if(ptr==Vstr_last) goto dec_error;
      ptr++;
    }
    while(*ptr=='0') {
      leadingzero=TRUE;
      if(ptr==Vstr_last) goto str_end;
      ptr++;
    }
    while(*ptr>='0' && *ptr<='9') {
      if(mant1_len==0) mant1=ptr;
      mant1_len++;
      if(ptr==Vstr_last) goto str_end;
      ptr++;
    }
    if(*ptr=='.' || *ptr==',') {
      decmark=ptr;
      if(ptr==Vstr_last) goto str_end;
      ptr++;
    }
    while(*ptr>='0' && *ptr<='9') {
      if(mant2_len==0) mant2=ptr;
      mant2_len++;
      if(ptr==Vstr_last) goto str_end;
      ptr++;
    }
    if(!leadingzero && !mant1 && !mant2) goto dec_error;
    if(*ptr=='e' || *ptr=='E') {
      expmark=ptr;
      if(ptr==Vstr_last) goto dec_error;
      ptr++;
    }
    if(*ptr=='+' || *ptr=='-') {
      expsign=ptr;
      if(ptr==Vstr_last) goto dec_error;
      ptr++;
    }
    while(*ptr=='0') {
      expo=ptr;
      if(ptr==Vstr_last) goto str_end;
      ptr++;
    }
    while(*ptr>='0' && *ptr<='9') {
      if(expo_len==0) expo=ptr;
      expo_len++;
      if(ptr==Vstr_last) goto str_end;
      ptr++;
    }
    if(expo_len==0 && expo!=NULL) expo_len=1; /* only leading zero */
    if(expsign && !expo) goto dec_error;
    ec.error(TTCN_EncDec::ET_INVAL_MSG,
             "Superfluous part at the end of decimal encoding.");
  str_end:
    /* check NR */
    if(NR==1) {
      if(decmark || expmark) NR_error=TRUE;
    }
    else if(NR==2) {
      if(expmark) NR_error=TRUE;
    }
    if(NR_error)
      ec.error(TTCN_EncDec::ET_INVAL_MSG,
               "This decimal encoding does not conform to NR%d form.", NR);
    while(mant2_len>1 && mant2[mant2_len-1]=='0') mant2_len--;
    if(mant2_len==1 && *mant2=='0') mant2_len=0, mant2=NULL;
    float_value=0.0;
    if(mant1) for(size_t i=0; i<mant1_len; i++) {
      float_value*=10.0;
      float_value+=static_cast<double>(mant1[i]-'0');
    } // for i if...
    if(mant2) for(size_t i=0; i<mant2_len; i++) {
      float_value*=10.0;
      float_value+=static_cast<double>(mant2[i]-'0');
    } // for i if...
    exponum=0;
    if(expo) {
      if(ceil(log10(log10(DBL_MAX)))<expo_len) {
        /* overflow */
        if(expsign && *expsign=='-') {
          float_value=0.0;
        }
        else {
          if(sign && *sign=='-') float_value=-INFINITY;
          else float_value=INFINITY;
        }
        goto end;
      } // overflow
      else {
        /* no overflow */
        for(size_t i=0; i<expo_len; i++) {
          exponum*=10;
          exponum+=static_cast<int>(expo[i]-'0');
        } // for i
        if(expsign && *expsign=='-')
          exponum*=-1;
      } // no overflow
    } // if expo
    if(mant2) exponum-=mant2_len;
    float_value*=pow(10.0, static_cast<double>(exponum));
    goto end;
  dec_error:
    ec.error(TTCN_EncDec::ET_INVAL_MSG, "Erroneous decimal encoding.");
    float_value=0.0;
  }
 end:
  bound_flag=TRUE;
  return TRUE;
}

int FLOAT::RAW_encode(const TTCN_Typedescriptor_t& p_td, RAW_enc_tree& myleaf) const
{
  unsigned char *bc;
  unsigned char *dv;
  int length = p_td.raw->fieldlength / 8;
  double tmp = float_value;
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
    tmp = 0.0;
  }
  if (isnan(tmp)) {
    TTCN_EncDec_ErrorContext::error_internal("Value is NaN.");
  }
  if (myleaf.must_free) Free(myleaf.body.leaf.data_ptr);
  if (length > RAW_INT_ENC_LENGTH) {
    myleaf.body.leaf.data_ptr = bc = (unsigned char*)Malloc(length*sizeof(*bc));
    myleaf.must_free = TRUE;
    myleaf.data_ptr_used = TRUE;
  }
  else {
    bc = myleaf.body.leaf.data_array;
  }
  if (length == 8) {
    dv = (unsigned char *) &tmp;
#if defined __sparc__ || defined __sparc
    memcpy(bc,dv,8);
#else
    for (int i = 0, k = 7; i < 8; i++, k--) bc[i] = dv[k];
#endif
  }
  else if (length == 4) {
    if (tmp == 0.0) memset(bc, 0, 4);
    else if (tmp == -0.0) {
      memset(bc, 0, 4);
      bc[0] |= 0x80;
    }
    else {
#if defined __sparc__ || defined __sparc
      int index=0;
      int adj=1;
#else
      int index = 7;
      int adj = -1;
#endif
      dv = (unsigned char *) &tmp;
      bc[0] = dv[index] & 0x80;
      int exponent = dv[index] & 0x7F;
      exponent <<= 4;
      index += adj;
      exponent += (dv[index] & 0xF0) >> 4;
      exponent -= 1023;

      if (exponent > 127) {
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
          "The float value '%f' is out of the range of "
          "the single precision: %s", (double)float_value, p_td.name);
        tmp = 0.0;
        exponent = 0;
      }
      else if (exponent < -127) {
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_FLOAT_TR,
          "The float value '%f' is too small to represent it "
          "in single precision: %s", (double)float_value, p_td.name);
        tmp = 0.0;
        exponent = 0;
      }
      else exponent += 127;
      bc[0] |= (exponent >> 1) & 0x7F;
      bc[1] = ((exponent << 7) & 0x80) | ((dv[index] & 0x0F) << 3)
        | ((dv[index + adj] & 0xE0) >> 5);
      index += adj;
      bc[2] = ((dv[index] & 0x1F) << 3) | ((dv[index + adj] & 0xE0) >> 5);
      index += adj;
      bc[3] = ((dv[index] & 0x1F) << 3) | ((dv[index + adj] & 0xE0) >> 5);
    }
  }
  else
    TTCN_EncDec_ErrorContext::error_internal("Invalid FLOAT length %d", length);

  return myleaf.length = p_td.raw->fieldlength;
}

int FLOAT::RAW_decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff,
  int limit, raw_order_t top_bit_ord, boolean no_err, int /*sel_field*/,
  boolean /*first_call*/)
{
  int prepaddlength = buff.increase_pos_padd(p_td.raw->prepadding);
  limit -= prepaddlength;
  int decode_length = p_td.raw->fieldlength;
  if ( p_td.raw->fieldlength > limit
    || p_td.raw->fieldlength > (int) buff.unread_len_bit()) {
    if (no_err) return -1;
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There is not enough bits in the buffer to decode type %s.", p_td.name);
    decode_length = limit > (int) buff.unread_len_bit()
      ? buff.unread_len_bit() : limit;
    bound_flag = TRUE;
    float_value = 0.0;
    decode_length += buff.increase_pos_padd(p_td.raw->padding);
    return decode_length + prepaddlength;
  }
  double tmp = 0.0;
  unsigned char data[16];
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
  buff.get_b((size_t) decode_length, data, cp, top_bit_ord);
  if (decode_length == 64) {
    unsigned char *dv = (unsigned char *) &tmp;
#if defined __sparc__ || defined __sparc
    memcpy(dv,data,8);
#else
    for (int i = 0, k = 7; i < 8; i++, k--) dv[i] = data[k];
#endif
    if (isnan(tmp)) {
      if (no_err) return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
        "Not a Number received for type %s.", p_td.name);
      tmp = 0.0;
    }
  }
  else if (decode_length == 32) {
    int sign = (data[0] & 0x80) >> 7;
    int exponent = ((data[0] & 0x7F) << 1) | ((data[1] & 0x80) >> 7);
    int fraction = ((data[1] & 0x7F) << 1) | ((data[2] & 0x80) >> 7);
    fraction <<= 8;
    fraction += ((data[2] & 0x7F) << 1) | ((data[3] & 0x80) >> 7);
    fraction <<= 7;
    fraction += data[3] & 0x7F;
    if (exponent == 0 && fraction == 0) tmp = sign ? -0.0 : 0.0;
    else if (exponent == 0xFF && fraction != 0) {
      if (no_err) return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
        "Not a Number received for type %s.", p_td.name);
      tmp = 0.0;
    }
    else if (exponent == 0 && fraction != 0) {
      double sign_v = sign ? -1.0 : 1.0;
      tmp = sign_v * (static_cast<double> (fraction) / 8388608.0)
        * pow(2.0, -126.0);
    }
    else {
      double sign_v = sign ? -1.0 : 1.0;
      exponent -= 127;
      tmp = sign_v * (1.0 + static_cast<double> (fraction) / 8388608.0)
        * pow(2.0, static_cast<double> (exponent));
    }

  }
  decode_length += buff.increase_pos_padd(p_td.raw->padding);
  bound_flag = TRUE;
  float_value = tmp;
  return decode_length + prepaddlength;
}

const char* XER_POS_INF_STR = "INF";
const char* XER_NEG_INF_STR = "-INF";
const char* XER_NAN_STR = "NaN";

int FLOAT::XER_encode(const XERdescriptor_t& p_td,
  TTCN_Buffer& p_buf, unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error(
      TTCN_EncDec::ET_UNBOUND, "Encoding an unbound float value.");
  }
  int exer  = is_exer(flavor |= SIMPLE_TYPE);
  // SIMPLE_TYPE has no influence on is_exer, we set it for later
  int encoded_length=(int)p_buf.get_len();
  flavor &= ~XER_RECOF; // float doesn't care

  begin_xml(p_td, p_buf, flavor, indent, FALSE);

  if (exer && (p_td.xer_bits & XER_DECIMAL)) {
    char buf[312];
    int n = 0;
    if (isnan((double)float_value)) {
      n = snprintf(buf, sizeof(buf), "%s", XER_NAN_STR);
    } else if ((double)float_value == (double)INFINITY) {
      n = snprintf(buf, sizeof(buf), "%s", XER_POS_INF_STR);
    } else if ((double)float_value == -(double)INFINITY) {
      n = snprintf(buf, sizeof(buf), "%s", XER_NEG_INF_STR);
    } else {
      n = snprintf(buf, sizeof(buf), "%f", (double)float_value);
      if (p_td.fractionDigits != -1) {
        char *p = strchr(buf, '.');
        if (p != NULL) {
          int offset = p_td.fractionDigits == 0 ? 0 : p_td.fractionDigits + 1;
          p[offset] = 0;
          n = strlen(buf);
        }
      }
    }
    p_buf.put_s((size_t)n, (const unsigned char*)buf);
  }
  else {
    CHARSTRING value;
    if (isnan((double)float_value)) {
      value = XER_NAN_STR;
    } else if ((double)float_value == (double)INFINITY) {
      value = XER_POS_INF_STR;
    } else if ((double)float_value == -(double)INFINITY) {
      value = XER_NEG_INF_STR;
    } else {
      value = float2str(float_value);
    }
    p_buf.put_string(value);
  }

  end_xml(p_td, p_buf, flavor, indent, FALSE);

  return (int)p_buf.get_len() - encoded_length;
}

boolean FLOAT::is_float(const char* p_str)
{
  boolean first_digit = FALSE; // first digit reached
  boolean decimal_point = FALSE; // decimal point (.) reached
  boolean exponent_mark = FALSE; // exponential mark (e or E) reached
  boolean exponent_sign = FALSE; // sign of the exponential (- or +) reached
  
  if ('-' == *p_str || '+' == *p_str) {
    ++p_str;
  }
  
  while (0 != *p_str) {
    switch(*p_str) {
    case '.':
      if (decimal_point || exponent_mark || !first_digit) {
        return FALSE;
      }
      decimal_point = TRUE;
      first_digit = FALSE;
      break;
    case 'e':
    case 'E':
      if (exponent_mark || !first_digit) {
        return FALSE;
      }
      exponent_mark = TRUE;
      first_digit = FALSE;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      first_digit = TRUE;
      break;
    case '-':
    case '+':
      if (exponent_sign || !exponent_mark || first_digit) {
        return FALSE;
      }
      exponent_sign = TRUE;
      break;
    default:
      return FALSE;
    }
    
    ++p_str; 
  }
  return first_digit;
}

int FLOAT::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
  unsigned int flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*)
{
  bound_flag = FALSE;
  boolean exer  = is_exer(flavor);
  int success = reader.Ok(), depth = -1;
  if (success <= 0) return 0;
  boolean own_tag = !(exer && (p_td.xer_bits & UNTAGGED)) && !is_exerlist(flavor);

  if (!own_tag) goto tagless;
  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
    verify_name(reader, p_td, exer);
tagless:
    const char * value = (const char *)reader.Value();
    if (value) {
      if (is_float(value)) {
        if (exer && (p_td.xer_bits & XER_DECIMAL) && p_td.fractionDigits != -1) {
          const char *p = strchr(value, '.');
          if (p != NULL) {
            unsigned int fraction_digits_pos = (int)(p - value) + 1 + p_td.fractionDigits;
            if (fraction_digits_pos < strlen(value)) {
              TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_FLOAT_TR,
                "The float value (%s) contains too many fractionDigits. Expected %i or less.",
                value,
                p_td.fractionDigits);
            }
          }
        }
        bound_flag = TRUE;
        sscanf(value, "%lf", &float_value);
      } else if (strcmp(XER_NAN_STR, value) == 0 ) {
        bound_flag = TRUE;
#ifdef NAN
        float_value = NAN;
#else
        float_value = INFINITY + (-INFINITY);
#endif
      } else if (strcmp(XER_POS_INF_STR, value) == 0) {
        bound_flag = TRUE;
        float_value = (double)INFINITY;
      } else if (strcmp(XER_NEG_INF_STR, value) == 0) {
        bound_flag = TRUE;
        float_value = -(double)INFINITY;
      }
    }
    // Let the caller do reader.AdvanceAttribute();
  }
  else {
    for (; success == 1; success = reader.Read()) {
      int type = reader.NodeType();
      if (XML_READER_TYPE_ELEMENT == type) {
        // If our parent is optional and there is an unexpected tag then return and
        // we stay unbound.
        if ((flavor & XER_OPTIONAL) && !check_name((const char*)reader.LocalName(), p_td, exer)) {
          return -1;
        }
        verify_name(reader, p_td, exer);
        if (reader.IsEmptyElement()) {
          if (exer && p_td.dfeValue != 0) {
            *this = *static_cast<const FLOAT*>(p_td.dfeValue);
          }
          reader.Read();
          break;
        }
        depth = reader.Depth();
      }
      else if (XML_READER_TYPE_TEXT == type && depth != -1) {
        const char * value = (const char*)reader.Value();
        if (value) {
          if (is_float(value)) {
            if (exer && (p_td.xer_bits & XER_DECIMAL) && p_td.fractionDigits != -1) {
              const char *p = strchr(value, '.');
              if (p != NULL) {
                unsigned int fraction_digits_pos = (int)(p - value) + 1 + p_td.fractionDigits;
                if (fraction_digits_pos < strlen(value)) {
                  TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_FLOAT_TR,
                    "The float value (%s) contains too many fractionDigits. Expected %i or less.",
                    value,
                    p_td.fractionDigits);
                }
              }
            }
            bound_flag = TRUE;
            sscanf(value, "%lf", &float_value);
          } else if (strcmp("NaN", value) == 0 ) {
            bound_flag = TRUE;
#ifdef NAN
            float_value = NAN;
#else
            float_value = INFINITY + (-INFINITY);
#endif
          } else if (strcmp("INF", value) == 0) {
            bound_flag = TRUE;
            float_value = (double)INFINITY;
          } else if (strcmp("-INF", value) == 0) {
            bound_flag = TRUE;
            float_value = -(double)INFINITY;
          }
        }
      }
      else if (XML_READER_TYPE_END_ELEMENT == type) {
        verify_end(reader, p_td, depth, exer);
        if (!bound_flag && exer && p_td.dfeValue != 0) {
          *this = *static_cast<const FLOAT*>(p_td.dfeValue);
        }
        reader.Read();
        break;
      }
    } // next read
  } // if not attribute
  return 1; // decode successful
}

const char* POS_INF_STR = "\"infinity\"";
const char* NEG_INF_STR = "\"-infinity\"";
const char* NAN_STR = "\"not_a_number\"";

int FLOAT::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound float value.");
    return -1;
  }
  
  double value = (double)float_value;
  if ((double)INFINITY == value) {
    return p_tok.put_next_token(JSON_TOKEN_STRING, POS_INF_STR);
  }
  if (-(double)INFINITY == value) {
    return p_tok.put_next_token(JSON_TOKEN_STRING, NEG_INF_STR);
  }
  if (isnan(value)) {
    return p_tok.put_next_token(JSON_TOKEN_STRING, NAN_STR);
  }
  
  // true if decimal representation possible (use %f format)
  boolean decimal_repr = (value == 0.0)
    || (value > -MAX_DECIMAL_FLOAT && value <= -MIN_DECIMAL_FLOAT)
    || (value >= MIN_DECIMAL_FLOAT && value < MAX_DECIMAL_FLOAT);
  
  char* tmp_str = mprintf(decimal_repr ? "%f" : "%e", value);
  int enc_len = p_tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
  Free(tmp_str);
  return enc_len;
}

int FLOAT::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
{
  bound_flag = FALSE;
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
  if (JSON_TOKEN_ERROR == token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  else if (JSON_TOKEN_STRING == token || use_default) {
    if (0 == strncmp(value, POS_INF_STR + (use_default ? 1 : 0), value_len)) {
      bound_flag = TRUE;
      float_value = INFINITY;
    }
    else if (0 == strncmp(value, NEG_INF_STR + (use_default ? 1 : 0), value_len)) {
      bound_flag = TRUE;
      float_value = -INFINITY;
    }
    else if (0 == strncmp(value, NAN_STR + (use_default ? 1 : 0), value_len)) {
      bound_flag = TRUE;
#ifdef NAN
      float_value = NAN;
#else
      float_value = INFINITY + (-INFINITY);
#endif
    }
    else if (!use_default) {
      char* spec_val = mprintf("float (%s, %s or %s)", POS_INF_STR, NEG_INF_STR, NAN_STR);
      JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FORMAT_ERROR, "string", spec_val);
      Free(spec_val);
      bound_flag = FALSE;
      return JSON_ERROR_FATAL;
    }
  }
  else if (JSON_TOKEN_NUMBER == token) {
    char* value2 = mcopystrn(value, value_len);
    sscanf(value2, "%lf", &float_value);
    bound_flag = TRUE;
    Free(value2);
  } else {
    return JSON_ERROR_INVALID_TOKEN;
  }
  if (!bound_flag && use_default) {
    // Already checked the default value for the string possibilities, now
    // check for a valid number
    char* value2 = mcopystrn(value, value_len);
    sscanf(value2, "%lf", &float_value);
    bound_flag = TRUE;
    Free(value2);
  }
  return (int)dec_len;
}


// global functions

double operator+(double double_value, const FLOAT& other_value)
{
  other_value.must_bound("Unbound right operand of float addition.");
  return double_value + other_value.float_value;
}

double operator-(double double_value, const FLOAT& other_value)
{
  other_value.must_bound("Unbound right operand of float subtraction.");
  return double_value - other_value.float_value;
}

double operator*(double double_value, const FLOAT& other_value)
{
  other_value.must_bound("Unbound right operand of float multiplication.");
  return double_value * other_value.float_value;
}

double operator/(double double_value, const FLOAT& other_value)
{
  other_value.must_bound("Unbound right operand of float division.");
  if (other_value.float_value == 0.0) TTCN_error("Float division by zero.");
  return double_value / other_value.float_value;
}

boolean operator==(double double_value, const FLOAT& other_value)
{
  other_value.must_bound("Unbound right operand of float comparison.");
  return double_value == other_value.float_value;
}

boolean operator<(double double_value, const FLOAT& other_value)
{
  other_value.must_bound("Unbound right operand of float comparison.");
  return double_value < other_value.float_value;
}

boolean operator>(double double_value, const FLOAT& other_value)
{
  other_value.must_bound("Unbound right operand of float comparison.");
  return double_value > other_value.float_value;
}

// float template class

void FLOAT_template::clean_up()
{
  if (template_selection == VALUE_LIST ||
      template_selection == COMPLEMENTED_LIST)
    delete [] value_list.list_value;
  template_selection = UNINITIALIZED_TEMPLATE;
}

void FLOAT_template::copy_template(const FLOAT_template& other_value)
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
    value_list.list_value = new FLOAT_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
        other_value.value_list.list_value[i]);
    break;
  case VALUE_RANGE:
    value_range = other_value.value_range;
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported float template.");
  }
  set_selection(other_value);
}

FLOAT_template::FLOAT_template()
{

}

FLOAT_template::FLOAT_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

FLOAT_template::FLOAT_template(double other_value)
  : Base_Template(SPECIFIC_VALUE)
{
  single_value = other_value;
}

FLOAT_template::FLOAT_template(const FLOAT& other_value)
  : Base_Template(SPECIFIC_VALUE)
{
  other_value.must_bound("Creating a template from an unbound float value.");
  single_value = other_value.float_value;
}

FLOAT_template::FLOAT_template(const OPTIONAL<FLOAT>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (double)(const FLOAT&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a float template from an unbound optional field.");
  }
}

FLOAT_template::FLOAT_template(const FLOAT_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

FLOAT_template::~FLOAT_template()
{
  clean_up();
}

FLOAT_template& FLOAT_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

FLOAT_template& FLOAT_template::operator=(double other_value)
{
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

FLOAT_template& FLOAT_template::operator=(const FLOAT& other_value)
{
  other_value.must_bound("Assignment of an unbound float value "
                         "to a template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value.float_value;
  return *this;
}

FLOAT_template& FLOAT_template::operator=(const OPTIONAL<FLOAT>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (double)(const FLOAT&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a float template.");
  }
  return *this;
}

FLOAT_template& FLOAT_template::operator=(const FLOAT_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean FLOAT_template::match(double other_value, boolean /* legacy */) const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    return single_value == other_value || // check if they're both NaN
      (single_value != single_value && other_value != other_value);
  case OMIT_VALUE:
    return FALSE;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int i = 0; i < value_list.n_values; i++)
      if(value_list.list_value[i].match(other_value))
        return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  case VALUE_RANGE:
    
    return (
      // Min boundary is -infinity and (not min exclusive            or  the value is larger than -infinity)
      (!value_range.min_is_present && (!value_range.min_is_exclusive || other_value != MINUS_INFINITY)) ||
      // Min boundary is a number and not min exclusive           and it is less or equal than the value   or
      (value_range.min_is_present && !value_range.min_is_exclusive && value_range.min_value <= other_value) ||
      // Min boundary is a number and min exclusive              and it is less than the value
      (value_range.min_is_present && value_range.min_is_exclusive && value_range.min_value < other_value))
      &&
      // Max boundary is infinity  and  (not min exclusive            or the value is smaller than infinity)
      ((!value_range.max_is_present && (!value_range.max_is_exclusive || other_value != PLUS_INFINITY)) ||
      // Max boundary is a number and not max exclusive           and it is more or equal than the value   or
       (value_range.max_is_present && !value_range.max_is_exclusive && value_range.max_value >= other_value) ||
      // Max boundary is a number and max exclusive               and it is more than the value
       (value_range.max_is_present && value_range.max_is_exclusive && value_range.max_value > other_value));
  default:
    TTCN_error("Matching with an uninitialized/unsupported float template.");
  }
  return FALSE;
}

boolean FLOAT_template::match(const FLOAT& other_value, boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  return match(other_value.float_value);
}


void FLOAT_template::set_type(template_sel template_type,
  unsigned int list_length)
{
  clean_up();
  switch (template_type) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    set_selection(template_type);
    value_list.n_values = list_length;
    value_list.list_value = new FLOAT_template[list_length];
    break;
  case VALUE_RANGE:
    set_selection(VALUE_RANGE);
    value_range.min_is_present = FALSE;
    value_range.max_is_present = FALSE;
    value_range.min_is_exclusive = FALSE;
    value_range.max_is_exclusive = FALSE;
    break;
  default:
    TTCN_error("Setting an invalid type for a float template.");
  }
}

FLOAT_template& FLOAT_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
      template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list float template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a float value list template.");
  return value_list.list_value[list_index];
}

void FLOAT_template::set_min(double min_value)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Float template is not range when setting lower limit.");
  if (value_range.max_is_present && value_range.max_value < min_value)
    TTCN_error("The lower limit of the range is greater than the "
               "upper limit in a float template.");
  value_range.min_is_present = TRUE;
  value_range.min_is_exclusive = FALSE;
  value_range.min_value = min_value;
}

void FLOAT_template::set_min(const FLOAT& min_value)
{
  min_value.must_bound("Using an unbound value when setting the lower bound "
                       "in a float range template.");
  set_min(min_value.float_value);
}

void FLOAT_template::set_max(double max_value)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Float template is not range when setting upper limit.");
  if (value_range.min_is_present && value_range.min_value > max_value)
    TTCN_error("The upper limit of the range is smaller than the "
               "lower limit in a float template.");
  value_range.max_is_present = TRUE;
  value_range.max_is_exclusive = FALSE;
  value_range.max_value = max_value;
}

void FLOAT_template::set_max(const FLOAT& max_value)
{
  max_value.must_bound("Using an unbound value when setting the upper bound "
                       "in a float range template.");
  set_max(max_value.float_value);
}

void FLOAT_template::set_min_exclusive(boolean min_exclusive)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Float template is not range when setting lower limit exclusiveness.");
  value_range.min_is_exclusive = min_exclusive;
}

void FLOAT_template::set_max_exclusive(boolean max_exclusive)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Float template is not range when setting upper limit exclusiveness.");
  value_range.max_is_exclusive = max_exclusive;
}

double FLOAT_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof "
               "or send operation on a non-specific float template.");
  return single_value;
}

void FLOAT_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    log_float(single_value);
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
  case VALUE_RANGE:
    TTCN_Logger::log_char('(');
    if (value_range.min_is_exclusive) TTCN_Logger::log_char('!');
    if (value_range.min_is_present) log_float(value_range.min_value);
    else TTCN_Logger::log_event_str("-infinity");
    TTCN_Logger::log_event_str(" .. ");
    if (value_range.max_is_exclusive) TTCN_Logger::log_char('!');
    if (value_range.max_is_present) log_float(value_range.max_value);
    else TTCN_Logger::log_event_str("infinity");
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void FLOAT_template::log_match(const FLOAT& match_value,
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

void FLOAT_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE, "float template");
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
    FLOAT_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Float:
    *this = mp->get_float();
    break;
  case Module_Param::MP_FloatRange:
    set_type(VALUE_RANGE);
    if (mp->has_lower_float()) set_min(mp->get_lower_float());
    if (mp->has_upper_float()) set_max(mp->get_upper_float());
    set_min_exclusive(mp->get_is_min_exclusive());
    set_max_exclusive(mp->get_is_max_exclusive());
    break;
  case Module_Param::MP_Expression:
    switch (mp->get_expr_type()) {
    case Module_Param::EXPR_NEGATE: {
      FLOAT operand;
      operand.set_param(*mp->get_operand1());
      *this = - operand;
      break; }
    case Module_Param::EXPR_ADD: {
      FLOAT operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 + operand2;
      break; }
    case Module_Param::EXPR_SUBTRACT: {
      FLOAT operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 - operand2;
      break; }
    case Module_Param::EXPR_MULTIPLY: {
      FLOAT operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 * operand2;
      break; }
    case Module_Param::EXPR_DIVIDE: {
      FLOAT operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      if (operand2 == 0.0) {
        param.error("Floating point division by zero.");
      }
      *this = operand1 / operand2;
      break; }
    default:
      param.expr_type_error("a float");
      break;
    }
    break;
  default:
    param.type_error("float template");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* FLOAT_template::get_param(Module_Param_Name& param_name) const
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
    mp = new Module_Param_Float(single_value);
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
  case VALUE_RANGE:
    mp = new Module_Param_FloatRange(
      value_range.min_value, value_range.min_is_present,
      value_range.max_value, value_range.max_is_present, value_range.min_is_exclusive, value_range.max_is_exclusive);
    break;
  default:
    TTCN_error("Referencing an uninitialized/unsupported float template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void FLOAT_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE:
    text_buf.push_double(single_value);
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].encode_text(text_buf);
    break;
  case VALUE_RANGE:
    text_buf.push_int(value_range.min_is_present ? 1 : 0);
    if (value_range.min_is_present)
      text_buf.push_double(value_range.min_value);
    text_buf.push_int(value_range.max_is_present ? 1 : 0);
    if (value_range.max_is_present)
      text_buf.push_double(value_range.max_value);
    break;
  default:
    TTCN_error("Text encoder: Encoding an undefined/unsupported "
               "float template.");
  }
}

void FLOAT_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE:
    single_value = text_buf.pull_double();
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new FLOAT_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  case VALUE_RANGE:
    value_range.min_is_present = text_buf.pull_int() != 0;
    if (value_range.min_is_present)
      value_range.min_value = text_buf.pull_double();
    value_range.max_is_present = text_buf.pull_int() != 0;
    if (value_range.max_is_present)
      value_range.max_value = text_buf.pull_double();
    value_range.min_is_exclusive = FALSE;
    value_range.max_is_exclusive = FALSE;
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was "
               "received for a float template.");
  }
}

boolean FLOAT_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean FLOAT_template::match_omit(boolean legacy /* = FALSE */) const
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
void FLOAT_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "float");
}
#endif
