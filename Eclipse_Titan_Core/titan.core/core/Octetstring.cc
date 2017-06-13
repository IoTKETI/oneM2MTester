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
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include "Octetstring.hh"
#include "../common/memory.h"
#include "Integer.hh"
#include "String_struct.hh"
#include "Param_Types.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Encdec.hh"
#include "RAW.hh"
#include "BER.hh"
#include "TEXT.hh"
#include "Addfunc.hh"
#include "Optional.hh"

#include "../common/dbgnew.hh"
#include <string.h>
#include <ctype.h>

// octetstring value class

/** The amount of memory needed for a string containing n octets. */
#define MEMORY_SIZE(n) (sizeof(octetstring_struct) - sizeof(int) + (n))

static const Token_Match
  octetstring_value_match("^(([0-9A-Fa-f]{2})+).*$", TRUE);

void OCTETSTRING::init_struct(int n_octets)
{
  if (n_octets < 0) {
    val_ptr = NULL;
    TTCN_error("Initializing an octetstring with a negative length.");
  } else if (n_octets == 0) {
    /* This will represent the empty strings so they won't need allocated
     * memory, this delays the memory allocation until it is really needed.
     */
    static octetstring_struct empty_string = { 1, 0, "" };
    val_ptr = &empty_string;
    empty_string.ref_count++;
  } else {
    val_ptr = (octetstring_struct*)Malloc(MEMORY_SIZE(n_octets));
    val_ptr->ref_count = 1;
    val_ptr->n_octets = n_octets;
  }
}

void OCTETSTRING::copy_value()
{
  if (val_ptr == NULL || val_ptr->n_octets <= 0)
    TTCN_error("Internal error: Invalid internal data structure when copying "
      "the memory area of an octetstring value.");
  if (val_ptr->ref_count > 1) {
    octetstring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_octets);
    memcpy(val_ptr->octets_ptr, old_ptr->octets_ptr, old_ptr->n_octets);
  }
}

// Called by operator+, operator~, operator&, operator|
// to initialize the return value to the appropriate capacity
// without copying bytes (which will be altered).
OCTETSTRING::OCTETSTRING(int n_octets)
{
  init_struct(n_octets);
}

OCTETSTRING::OCTETSTRING()
{
  val_ptr = NULL;
}

OCTETSTRING::OCTETSTRING(int n_octets, const unsigned char* octets_ptr)
{
  init_struct(n_octets);
  memcpy(val_ptr->octets_ptr, octets_ptr, n_octets);
}

OCTETSTRING::OCTETSTRING(const OCTETSTRING& other_value)
: Base_Type(other_value)
{
  other_value.must_bound("Copying an unbound octetstring value.");
  val_ptr = other_value.val_ptr;
  val_ptr->ref_count++;
}

OCTETSTRING::OCTETSTRING(const OCTETSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Copying an unbound octetstring element.");
  init_struct(1);
  val_ptr->octets_ptr[0] = other_value.get_octet();
}

OCTETSTRING::~OCTETSTRING()
{
  clean_up();
}

OCTETSTRING& OCTETSTRING::operator=(const OCTETSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound octetstring value.");
  if (&other_value != this) {
    clean_up();
    val_ptr = other_value.val_ptr;
    val_ptr->ref_count++;
  }
  return *this;
}

OCTETSTRING& OCTETSTRING::operator=(const OCTETSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound octetstring element to an "
    "octetstring.");
  unsigned char octet_value = other_value.get_octet();
  clean_up();
  init_struct(1);
  val_ptr->octets_ptr[0] = octet_value;
  return *this;
}

boolean OCTETSTRING::operator==(const OCTETSTRING& other_value) const
{
  must_bound("Unbound left operand of octetstring comparison.");
  other_value.must_bound("Unbound right operand of octetstring comparison.");
  if (val_ptr->n_octets != other_value.val_ptr->n_octets) return FALSE;
  return !memcmp(val_ptr->octets_ptr, other_value.val_ptr->octets_ptr,
                 val_ptr->n_octets);
}

boolean OCTETSTRING::operator==(const OCTETSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of octetstring comparison.");
  other_value.must_bound("Unbound right operand of octetstring element "
                         "comparison.");
  if (val_ptr->n_octets != 1) return FALSE;
  return val_ptr->octets_ptr[0] == other_value.get_octet();
}

OCTETSTRING OCTETSTRING::operator+(const OCTETSTRING& other_value) const
{
  must_bound("Unbound left operand of octetstring concatenation.");
  other_value.must_bound("Unbound right operand of octetstring concatenation.");
  int left_n_octets = val_ptr->n_octets;
  if (left_n_octets == 0) return other_value;
  int right_n_octets = other_value.val_ptr->n_octets;
  if (right_n_octets == 0) return *this;
  OCTETSTRING ret_val(left_n_octets + right_n_octets);
  memcpy(ret_val.val_ptr->octets_ptr, val_ptr->octets_ptr, left_n_octets);
  memcpy(ret_val.val_ptr->octets_ptr + left_n_octets,
         other_value.val_ptr->octets_ptr, right_n_octets);
  return ret_val;
}

OCTETSTRING OCTETSTRING::operator+(const OCTETSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of octetstring concatenation.");
  other_value.must_bound("Unbound right operand of octetstring element "
                         "concatenation.");
  OCTETSTRING ret_val(val_ptr->n_octets + 1);
  memcpy(ret_val.val_ptr->octets_ptr, val_ptr->octets_ptr, val_ptr->n_octets);
  ret_val.val_ptr->octets_ptr[val_ptr->n_octets] = other_value.get_octet();
  return ret_val;
}

#ifdef TITAN_RUNTIME_2
OCTETSTRING OCTETSTRING::operator+(const OPTIONAL<OCTETSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const OCTETSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of octetstring concatenation.");
}
#endif

OCTETSTRING& OCTETSTRING::operator+=(const OCTETSTRING& other_value)
{
  must_bound("Appending an octetstring value to an unbound octetstring value.");
  other_value.must_bound("Appending an unbound octetstring value to another "
    "octetstring value.");
  int other_n_octets = other_value.val_ptr->n_octets;
  if (other_n_octets > 0) {
    if (val_ptr->n_octets == 0) {
      clean_up();
      val_ptr = other_value.val_ptr;
      val_ptr->ref_count++;
    } else if (val_ptr->ref_count > 1) {
      octetstring_struct *old_ptr = val_ptr;
      old_ptr->ref_count--;
      init_struct(old_ptr->n_octets + other_n_octets);
      memcpy(val_ptr->octets_ptr, old_ptr->octets_ptr, old_ptr->n_octets);
      memcpy(val_ptr->octets_ptr + old_ptr->n_octets,
	other_value.val_ptr->octets_ptr, other_n_octets);
    } else {
      val_ptr = (octetstring_struct*)
	Realloc(val_ptr, MEMORY_SIZE(val_ptr->n_octets + other_n_octets));
      memcpy(val_ptr->octets_ptr + val_ptr->n_octets,
	other_value.val_ptr->octets_ptr, other_n_octets);
      val_ptr->n_octets += other_n_octets;
    }
  }
  return *this;
}

OCTETSTRING& OCTETSTRING::operator+=(const OCTETSTRING_ELEMENT& other_value)
{
  must_bound("Appending an octetstring element to an unbound octetstring "
    "value.");
  other_value.must_bound("Appending an unbound octetstring element to an "
    "octetstring value.");
  if (val_ptr->ref_count > 1) {
    octetstring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_octets + 1);
    memcpy(val_ptr->octets_ptr, old_ptr->octets_ptr, old_ptr->n_octets);
    val_ptr->octets_ptr[old_ptr->n_octets] = other_value.get_octet();
  } else {
    val_ptr = (octetstring_struct*)
      Realloc(val_ptr, MEMORY_SIZE(val_ptr->n_octets + 1));
    val_ptr->octets_ptr[val_ptr->n_octets] = other_value.get_octet();
    val_ptr->n_octets++;
  }
  return *this;
}

OCTETSTRING OCTETSTRING::operator~() const
{
  must_bound("Unbound octetstring operand of operator not4b.");
  OCTETSTRING ret_val(val_ptr->n_octets);
  for (int i = 0; i < val_ptr->n_octets; i++)
    ret_val.val_ptr->octets_ptr[i] = ~val_ptr->octets_ptr[i];
  return ret_val;
}

OCTETSTRING OCTETSTRING::operator&(const OCTETSTRING& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound octetstring value.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "octetstring value.");
  if (val_ptr->n_octets != other_value.val_ptr->n_octets)
    TTCN_error("The octetstring operands of operator and4b must have the "
               "same length.");
  OCTETSTRING ret_val(val_ptr->n_octets);
  for (int i = 0; i < val_ptr->n_octets; i++)
    ret_val.val_ptr->octets_ptr[i] =
      val_ptr->octets_ptr[i] & other_value.val_ptr->octets_ptr[i];
  return ret_val;
}

OCTETSTRING OCTETSTRING::operator&(const OCTETSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound octetstring value.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "octetstring element.");
  if (val_ptr->n_octets != 1)
    TTCN_error("The octetstring operands of "
               "operator and4b must have the same length.");
  unsigned char result = val_ptr->octets_ptr[0] & other_value.get_octet();
  return OCTETSTRING(1, &result);
}

OCTETSTRING OCTETSTRING::operator|(const OCTETSTRING& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound octetstring value.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "octetstring value.");
  if (val_ptr->n_octets != other_value.val_ptr->n_octets)
    TTCN_error("The octetstring operands of operator or4b must have the "
               "same length.");
  OCTETSTRING ret_val(val_ptr->n_octets);
  for (int i = 0; i < val_ptr->n_octets; i++)
    ret_val.val_ptr->octets_ptr[i] =
      val_ptr->octets_ptr[i] | other_value.val_ptr->octets_ptr[i];
  return ret_val;
}

OCTETSTRING OCTETSTRING::operator|(const OCTETSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound octetstring value.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "octetstring element.");
  if (val_ptr->n_octets != 1)
    TTCN_error("The octetstring operands of "
               "operator or4b must have the same length.");
  unsigned char result = val_ptr->octets_ptr[0] | other_value.get_octet();
  return OCTETSTRING(1, &result);
}

OCTETSTRING OCTETSTRING::operator^(const OCTETSTRING& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound octetstring value.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "octetstring value.");
  if (val_ptr->n_octets != other_value.val_ptr->n_octets)
    TTCN_error("The octetstring operands of operator xor4b must have the "
               "same length.");
  OCTETSTRING ret_val(val_ptr->n_octets);
  for (int i = 0; i < val_ptr->n_octets; i++)
    ret_val.val_ptr->octets_ptr[i] =
      val_ptr->octets_ptr[i] ^ other_value.val_ptr->octets_ptr[i];
  return ret_val;
}

OCTETSTRING OCTETSTRING::operator^(const OCTETSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound octetstring value.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "octetstring element.");
  if (val_ptr->n_octets != 1)
    TTCN_error("The octetstring operands of "
               "operator xor4b must have the same length.");
  unsigned char result = val_ptr->octets_ptr[0] ^ other_value.get_octet();
  return OCTETSTRING(1, &result);
}

OCTETSTRING OCTETSTRING::operator<<(int shift_count) const
{
  must_bound("Unbound octetstring operand of shift left operator.");
  if (shift_count > 0) {
    if (val_ptr->n_octets == 0) return *this;
    OCTETSTRING ret_val(val_ptr->n_octets);
    if (shift_count > val_ptr->n_octets) shift_count = val_ptr->n_octets;
    memcpy(ret_val.val_ptr->octets_ptr, val_ptr->octets_ptr + shift_count,
           val_ptr->n_octets - shift_count);
    memset(ret_val.val_ptr->octets_ptr + val_ptr->n_octets - shift_count,
           0, shift_count);
    return ret_val;
  } else if (shift_count == 0) return *this;
  else return *this >> (-shift_count);
}

OCTETSTRING OCTETSTRING::operator<<(const INTEGER& shift_count) const
{
  shift_count.must_bound("Unbound right operand of octetstring shift left "
    "operator.");
  return *this << (int)shift_count;
}

OCTETSTRING OCTETSTRING::operator>>(int shift_count) const
{
  must_bound("Unbound octetstring operand of shift right operator.");
  if (shift_count > 0) {
    if (val_ptr->n_octets == 0) return *this;
    OCTETSTRING ret_val(val_ptr->n_octets);
    if (shift_count > val_ptr->n_octets) shift_count = val_ptr->n_octets;
    memset(ret_val.val_ptr->octets_ptr, 0, shift_count);
    memcpy(ret_val.val_ptr->octets_ptr + shift_count, val_ptr->octets_ptr,
           val_ptr->n_octets - shift_count);
    return ret_val;
  } else if (shift_count == 0) return *this;
  else return *this << (-shift_count);
}

OCTETSTRING OCTETSTRING::operator>>(const INTEGER& shift_count) const
{
  shift_count.must_bound("Unbound right operand of octetstring shift right "
    "operator.");
  return *this >> (int)shift_count;
}

OCTETSTRING OCTETSTRING::operator<<=(int rotate_count) const
{
  must_bound("Unbound octetstring operand of rotate left operator.");
  if (val_ptr->n_octets == 0) return *this;
  if (rotate_count >= 0) {
    rotate_count %= val_ptr->n_octets;
    if (rotate_count == 0) return *this;
    OCTETSTRING ret_val(val_ptr->n_octets);
    memcpy(ret_val.val_ptr->octets_ptr, val_ptr->octets_ptr +
           rotate_count, val_ptr->n_octets - rotate_count);
    memcpy(ret_val.val_ptr->octets_ptr + val_ptr->n_octets - rotate_count,
           val_ptr->octets_ptr, rotate_count);
    return ret_val;
  } else return *this >>= (-rotate_count);
}

OCTETSTRING OCTETSTRING::operator<<=(const INTEGER& rotate_count) const
{
  rotate_count.must_bound("Unbound right operand of octetstring rotate left "
    "operator.");
  return *this <<= (int)rotate_count;
}

OCTETSTRING OCTETSTRING::operator>>=(int rotate_count) const
{
  must_bound("Unbound octetstring operand of rotate right operator.");
  if (val_ptr->n_octets == 0) return *this;
  if (rotate_count >= 0) {
    rotate_count %= val_ptr->n_octets;
    if (rotate_count == 0) return *this;
    OCTETSTRING ret_val(val_ptr->n_octets);
    memcpy(ret_val.val_ptr->octets_ptr, val_ptr->octets_ptr +
           val_ptr->n_octets - rotate_count, rotate_count);
    memcpy(ret_val.val_ptr->octets_ptr + rotate_count,
           val_ptr->octets_ptr, val_ptr->n_octets - rotate_count);
    return ret_val;
  } else return *this <<= (-rotate_count);
}

OCTETSTRING OCTETSTRING::operator>>=(const INTEGER& rotate_count) const
{
  rotate_count.must_bound("Unbound right operand of octetstring rotate right "
    "operator.");
  return *this >>= (int)rotate_count;
}

OCTETSTRING_ELEMENT OCTETSTRING::operator[](int index_value)
{
  if (val_ptr == NULL && index_value == 0) {
    init_struct(1);
    return OCTETSTRING_ELEMENT(FALSE, *this, 0);
  } else {
    must_bound("Accessing an element of an unbound octetstring value.");
    if (index_value < 0) TTCN_error("Accessing an octetstring element using "
      "a negative index (%d).", index_value);
    int n_octets = val_ptr->n_octets;
    if (index_value > n_octets) TTCN_error("Index overflow when accessing a "
      "octetstring element: The index is %d, but the string has only %d "
      "octets.", index_value, n_octets);
    if (index_value == n_octets) {
      if (val_ptr->ref_count == 1) {
	val_ptr = (octetstring_struct*)
	  Realloc(val_ptr, MEMORY_SIZE(n_octets + 1));
	val_ptr->n_octets++;
      } else {
	octetstring_struct *old_ptr = val_ptr;
	old_ptr->ref_count--;
	init_struct(n_octets + 1);
	memcpy(val_ptr->octets_ptr, old_ptr->octets_ptr, n_octets);
      }
      return OCTETSTRING_ELEMENT(FALSE, *this, index_value);
    } else return OCTETSTRING_ELEMENT(TRUE, *this, index_value);
  }
}

OCTETSTRING_ELEMENT OCTETSTRING::operator[](const INTEGER& index_value)
{
  index_value.must_bound("Indexing a octetstring value with an unbound integer "
    "value.");
  return (*this)[(int)index_value];
}

const OCTETSTRING_ELEMENT OCTETSTRING::operator[](int index_value) const
{
  must_bound("Accessing an element of an unbound octetstring value.");
  if (index_value < 0) TTCN_error("Accessing an octetstring element using a "
    "negative index (%d).", index_value);
  if (index_value >= val_ptr->n_octets) TTCN_error("Index overflow when "
    "accessing a octetstring element: The index is %d, but the string has "
    "only %d octets.", index_value, val_ptr->n_octets);
  return OCTETSTRING_ELEMENT(TRUE, const_cast<OCTETSTRING&>(*this),
    index_value);
}

const OCTETSTRING_ELEMENT OCTETSTRING::operator[](const INTEGER& index_value)
  const
{
  index_value.must_bound("Indexing a octetstring value with an unbound integer "
    "value.");
  return (*this)[(int)index_value];
}

void OCTETSTRING::clean_up()
{
  if (val_ptr != NULL) {
    if (val_ptr->ref_count > 1) val_ptr->ref_count--;
    else if (val_ptr->ref_count == 1) Free(val_ptr);
    else TTCN_error("Internal error: Invalid reference counter in an "
      "octetstring value.");
    val_ptr = NULL;
  }
}

int OCTETSTRING::lengthof() const
{
  must_bound("Getting the length of an unbound octetstring value.");
  return val_ptr->n_octets;
}

OCTETSTRING::operator const unsigned char*() const
{
  must_bound("Casting an unbound octetstring  value to const unsigned char*.");
  return val_ptr->octets_ptr;
}

void OCTETSTRING::log() const
{
  if (val_ptr != NULL) {
    boolean only_printable = TRUE;
    TTCN_Logger::log_char('\'');
    for (int i = 0; i < val_ptr->n_octets; i++) {
      unsigned char octet = val_ptr->octets_ptr[i];
      TTCN_Logger::log_octet(octet);
      if (only_printable && !TTCN_Logger::is_printable(octet))
	only_printable = FALSE;
    }
    TTCN_Logger::log_event_str("'O");
    if (only_printable && val_ptr->n_octets > 0) {
      TTCN_Logger::log_event_str(" (\"");
      for (int i = 0; i < val_ptr->n_octets; i++)
	TTCN_Logger::log_char_escaped(val_ptr->octets_ptr[i]);
      TTCN_Logger::log_event_str("\")");
    }
  } else TTCN_Logger::log_event_unbound();
}

void OCTETSTRING::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE|Module_Param::BC_LIST, "octetstring value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Octetstring:
    switch (param.get_operation_type()) {
    case Module_Param::OT_ASSIGN:
      clean_up();
      init_struct(mp->get_string_size());
      memcpy(val_ptr->octets_ptr, mp->get_string_data(), val_ptr->n_octets);
      break;
    case Module_Param::OT_CONCAT:
      if (is_bound()) {
        *this += OCTETSTRING(mp->get_string_size(), (unsigned char*)mp->get_string_data());
      } else {
        *this = OCTETSTRING(mp->get_string_size(), (unsigned char*)mp->get_string_data());
      }
      break;
    default:
      TTCN_error("Internal error: OCTETSTRING::set_param()");
    }
    break;
  case Module_Param::MP_Expression:
    if (mp->get_expr_type() == Module_Param::EXPR_CONCATENATE) {
      OCTETSTRING operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      if (param.get_operation_type() == Module_Param::OT_CONCAT) {
        *this = *this + operand1 + operand2;
      }
      else {
        *this = operand1 + operand2;
      }
    }
    else {
      param.expr_type_error("a octetstring");
    }
    break;
  default:
    param.type_error("octetstring value");
    break;
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* OCTETSTRING::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  unsigned char* val_cpy = (unsigned char *)Malloc(val_ptr->n_octets);
  memcpy(val_cpy, val_ptr->octets_ptr, val_ptr->n_octets);
  return new Module_Param_Octetstring(val_ptr->n_octets, val_cpy);
}
#endif

void OCTETSTRING::encode_text(Text_Buf& text_buf) const
{
  must_bound("Text encoder: Encoding an unbound octetstring value.");
  text_buf.push_int(val_ptr->n_octets);
  if(val_ptr->n_octets > 0)
    text_buf.push_raw(val_ptr->n_octets, val_ptr->octets_ptr);
}

void OCTETSTRING::decode_text(Text_Buf& text_buf)
{
  int n_octets = text_buf.pull_int().get_val();
  if (n_octets < 0)
    TTCN_error("Text decoder: Invalid length was received for an octetstring.");
  clean_up();
  init_struct(n_octets);
  if (n_octets > 0) text_buf.pull_raw(n_octets, val_ptr->octets_ptr);
}

void OCTETSTRING::encode(const TTCN_Typedescriptor_t& p_td,
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
    TTCN_EncDec_ErrorContext ec("While TEXT-encoding type '%s': ", p_td.name);
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

void OCTETSTRING::decode(const TTCN_Typedescriptor_t& p_td,
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
OCTETSTRING::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                            unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(!new_tlv) {
    new_tlv=BER_encode_TLV_OCTETSTRING
      (p_coding, val_ptr->n_octets, val_ptr->octets_ptr);
  }
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

#ifdef TITAN_RUNTIME_2
ASN_BER_TLV_t* OCTETSTRING::BER_encode_negtest_raw() const
{
  unsigned char *p_Vstr = (unsigned char *)Malloc(val_ptr->n_octets);
  memcpy(p_Vstr, val_ptr->octets_ptr, val_ptr->n_octets);
  ASN_BER_TLV_t* new_tlv = ASN_BER_TLV_t::construct(val_ptr->n_octets, p_Vstr);
  return new_tlv;
}

int OCTETSTRING::encode_raw(TTCN_Buffer& p_buf) const
{
  p_buf.put_string(*this);
  return val_ptr ? val_ptr->n_octets : 0;
}

int OCTETSTRING::RAW_encode_negtest_raw(RAW_enc_tree& p_myleaf) const
{
  if (p_myleaf.must_free)
    Free(p_myleaf.body.leaf.data_ptr);
  p_myleaf.must_free = FALSE;
  p_myleaf.data_ptr_used = TRUE;
  p_myleaf.body.leaf.data_ptr = val_ptr->octets_ptr;
  return p_myleaf.length = val_ptr->n_octets * 8;
}

int OCTETSTRING::JSON_encode_negtest_raw(JSON_Tokenizer& p_tok) const
{
  if (val_ptr != NULL) {
    p_tok.put_raw_data((const char*)val_ptr->octets_ptr, val_ptr->n_octets);
    return val_ptr->n_octets;
  }
  return 0;
}
#endif

boolean OCTETSTRING::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                                    const ASN_BER_TLV_t& p_tlv,
                                    unsigned L_form)
{
  clean_up();
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec("While decoding OCTETSTRING type: ");
  /* Upper estimation for the length. */
  size_t stripped_tlv_len = stripped_tlv.get_len();
  if (stripped_tlv_len < 2) return FALSE;
  int max_len = stripped_tlv_len - 2;
  init_struct(max_len);
  unsigned int octetnum_start = 0;
  BER_decode_TLV_OCTETSTRING(stripped_tlv, L_form, octetnum_start,
                             val_ptr->n_octets, val_ptr->octets_ptr);
  if (val_ptr->n_octets < max_len) {
    if (val_ptr->n_octets == 0) {
      clean_up();
      init_struct(0);
    } else {
      val_ptr = (octetstring_struct*)
	Realloc(val_ptr, MEMORY_SIZE(val_ptr->n_octets));
    }
  }
  return TRUE;
}

int OCTETSTRING::TEXT_decode(const TTCN_Typedescriptor_t& p_td,
                 TTCN_Buffer& buff,  Limit_Token_List& limit,
                 boolean no_err, boolean /*first_call*/)
{
  int decoded_length=0;
  int str_len=0;
  clean_up();
  if(p_td.text->begin_decode){
    int tl;
    if((tl=p_td.text->begin_decode->match_begin(buff))<0){
      if(no_err)return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "The specified token '%s' not found for '%s': ",
        (const char*)*(p_td.text->begin_decode), p_td.name);
      return 0;
    }
    decoded_length+=tl;
    buff.increase_pos(tl);
  }
//  never returns "too short"
//  if(buff.get_read_len()<=1 && no_err) return -TTCN_EncDec::ET_LEN_ERR;

  if(p_td.text->select_token){
    int tl;
    if((tl=p_td.text->select_token->match_begin(buff))<0) {
      if(no_err) return -1;
      else tl=0;
    }
    str_len=tl;
  } else
    if(     p_td.text->val.parameters
      &&    p_td.text->val.parameters->decoding_params.min_length!=-1){
    str_len=p_td.text->val.parameters->decoding_params.min_length*2;
  } else if(p_td.text->end_decode){
    int tl;
    if((tl=p_td.text->end_decode->match_first(buff))<0) {
      if(no_err) return -1;
      else tl=0;
    }
    str_len=tl;
  } else if (limit.has_token()){
    int tl;
    if((tl=limit.match(buff))<0)
      tl=buff.get_read_len()-1;;
    str_len=tl;
  } else {
    int tl;
    if((tl=octetstring_value_match.match_begin(buff))<0) {
      if(no_err) {return -1; }
      else tl=0;
    }
    str_len=tl;
  }
  str_len=(str_len/2)*2;
//printf("HALI chr:%d ",str_len);
  int n_octets = str_len / 2;
  init_struct(n_octets);
  unsigned char *octets_ptr = val_ptr->octets_ptr;
  const char *value=(const char*)buff.get_read_data();
  for (int i = 0; i < n_octets; i++) {
    unsigned char upper_nibble = char_to_hexdigit(value[2 * i]);
    unsigned char lower_nibble = char_to_hexdigit(value[2 * i + 1]);
    if (upper_nibble > 0x0F){
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
        "The octetstring value may contain hexadecimal digits only. "
        "Character \"%c\" was found.", value[2 * i]);
      upper_nibble=0;
    }
    if (lower_nibble > 0x0F){
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
        "The octetstring value str2oct() may contain hexadecimal digits only. "
        "Character \"%c\" was found.", value[2 * i + 1]);
      lower_nibble=0;
    }
    octets_ptr[i] = (upper_nibble << 4) | lower_nibble;
  }

//printf("%s\n\r",val_ptr->chars_ptr);
  decoded_length+=str_len;
  buff.increase_pos(str_len);

  if(p_td.text->end_decode){
    int tl;
    if((tl=p_td.text->end_decode->match_begin(buff))<0){
      if(no_err)return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "The specified token '%s' not found for '%s': ",
        (const char*)*(p_td.text->end_decode), p_td.name);
      return 0;
    }
    decoded_length+=tl;
    buff.increase_pos(tl);
  }
  return decoded_length;
}

// From Charstring.cc
extern char  base64_decoder_table[256];
extern unsigned int xlate(cbyte* in, int phase, unsigned char* dest);
extern const char cb64[];

int OCTETSTRING::XER_encode(const XERdescriptor_t& p_td,
    TTCN_Buffer& p_buf, unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound octetstring value.");
  }
  int exer  = is_exer(flavor |= SIMPLE_TYPE);
  // SIMPLE_TYPE has no influence on is_exer, we set it for later
  int encoded_length=(int)p_buf.get_len();
  int empty_element = val_ptr==NULL || val_ptr->n_octets == 0;

  flavor &= ~XER_RECOF; // octetstring doesn't care
  begin_xml(p_td, p_buf, flavor, indent, empty_element);

  if (exer && (p_td.xer_bits & BASE_64)) {
    // bit more work
    size_t clear_len = lengthof();
    const unsigned char * in = operator const unsigned char*();

    /* Encode 3 bytes of cleartext into 4 bytes of Base64.
     * Note that the algorithm is slightly different from Charstring.cc:
     * we can't pad the source because it's const (this),
     * so we need to check the indexes i+1 and i+2 before dereferencing */
    for (size_t i = 0; i < clear_len; i += 3) {
      p_buf.put_c( cb64[ in[i] >> 2 ] );
      p_buf.put_c( cb64[ ((in[i] & 0x03) << 4) | (i+1 < clear_len
        ? ((in[i+1] & 0xf0) >> 4)
        :-0) ] );
      p_buf.put_c( i+1 < clear_len
        ? cb64[ ((in[i+1] & 0x0f) << 2) | (i+2 < clear_len ? ((in[i+2] & 0xc0) >> 6) :-0) ]
        :'=');
      p_buf.put_c( i+2 < clear_len ? cb64[ in[i+2] & 0x3f ] : '=' );
    } // next i
  }
  else {
    CHARSTRING val = oct2str(*this);
    p_buf.put_string(val);
  }

  end_xml(p_td, p_buf, flavor, indent, empty_element);

  return (int)p_buf.get_len() - encoded_length;
}

int OCTETSTRING::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
    unsigned int flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*)
{
  boolean exer  = is_exer(flavor);
  int success = reader.Ok(), depth = -1, type;
  boolean own_tag = !is_exerlist(flavor) && !(exer && (p_td.xer_bits & UNTAGGED));

  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
    const char * name = verify_name(reader, p_td, exer);
    (void)name;
  }
  else
   if (own_tag) for (; success == 1; success = reader.Read()) {
    type = reader.NodeType();
    if (XML_READER_TYPE_ELEMENT == type) {
      // If our parent is optional and there is an unexpected tag then return and
      // we stay unbound.
      if ((flavor & XER_OPTIONAL) && !check_name((const char*)reader.LocalName(), p_td, exer)) {
        return -1;
      }
      verify_name(reader, p_td, exer);
      depth = reader.Depth();
      if (reader.IsEmptyElement()) {
        if (exer && p_td.dfeValue != 0) {
          *this = *static_cast<const OCTETSTRING*> (p_td.dfeValue);
        }
        else init_struct(0);
        reader.Read();
        goto finished;
      }
    }
    else if (XML_READER_TYPE_TEXT == type && depth != -1) break;
    else if (XML_READER_TYPE_END_ELEMENT ==  type) {
      // End tag without intervening #text == empty content
      verify_end(reader, p_td, depth, exer);
      if (exer && p_td.dfeValue != 0) {
        *this = *static_cast<const OCTETSTRING*>(p_td.dfeValue);
      }
      else init_struct(0);
      reader.Read();
      goto finished;
    }
  }

  type = reader.NodeType();
  if (success == 1 && (XML_READER_TYPE_TEXT == type || XML_READER_TYPE_ATTRIBUTE == type)) {
    const char * value = (const char *)reader.Value();
    size_t len = value ? strlen(value) : 0;

    if (exer && (p_td.xer_bits & BASE_64)) {
      xmlChar in[4];

      int phase = 0;
      init_struct(len * 3 / 4);
      unsigned char * dest = val_ptr->octets_ptr;

      for (size_t o=0; o<len; ++o) {
        xmlChar c = value[o];
        if(c == '=') { // padding starts
          dest += xlate(in, phase, dest);
          break;
        }

        int val = base64_decoder_table[c];
        if(val >= 0)    {
          in[phase] = val;
          phase = (phase + 1) % 4;
          if(phase == 0) {
            dest += xlate(in,phase, dest);
            in[0]=in[1]=in[2]=in[3]=0;
          }
        }
        else if (exer && (flavor & EXIT_ON_ERROR)) {
          clean_up();
          return -1;
        } else {
          TTCN_EncDec_ErrorContext::warning(
            /* if this was an error... TTCN_EncDec::ET_INVAL_MSG,*/
            "Invalid character for Base64 '%02X'", c);
        }
      } // while
      // adjust
      val_ptr->n_octets = dest - val_ptr->octets_ptr;

    }
    else { // not base64
      if (len & 1) { // that's odd...
        if (exer && (flavor & EXIT_ON_ERROR)) {
          clean_up();
          return -1;
        } else {
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
            "Odd number of characters in octetstring");
          len &= ~1; // make it even
        }
      }
      size_t n_octets = len / 2;
      init_struct(n_octets);

      len = 0; // will act as 2*i
      for (size_t i = 0; i < n_octets; ++i, ++++len) {
        unsigned char upper_nibble = char_to_hexdigit(value[len]);
        unsigned char lower_nibble = char_to_hexdigit(value[len+1]);
        if (upper_nibble > 0x0F) {
          if (exer && (flavor & EXIT_ON_ERROR)) {
            clean_up();
            return -1;
          } else {
            TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
              "The octetstring value may contain hexadecimal digits only. "
              "Character \"%c\" was found.", value[len]);
            upper_nibble=0;
          }
        }
        if (lower_nibble > 0x0F) {
          if (exer && (flavor & EXIT_ON_ERROR)) {
            clean_up();
            return -1;
          } else {
            TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
              "The octetstring value may contain hexadecimal digits only. "
              "Character \"%c\" was found.", value[len+1]);
            lower_nibble=0;
          }
        }
        val_ptr->octets_ptr[i] = (upper_nibble << 4) | lower_nibble;
      } // next
    } // if base64
  } // if success

  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
    // Let the caller do reader.AdvanceAttribute();
  }
  else
  if (own_tag) for (success = reader.Read(); success == 1; success = reader.Read()) {
    type = reader.NodeType();
    if (XML_READER_TYPE_END_ELEMENT == type) {
      verify_end(reader, p_td, depth, exer);
      if (val_ptr == 0 && p_td.dfeValue != 0) {
        // The end tag must have followed the start tag
        *this = *static_cast<const OCTETSTRING*>(p_td.dfeValue);
      }
      reader.Read(); // one last time
      break;
    }
  }
finished:
  return 1; // decode successful
}

int OCTETSTRING::TEXT_encode(const TTCN_Typedescriptor_t& p_td,
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

  int chars_before=0;

  if(p_td.text->val.parameters){
    if(val_ptr->n_octets<p_td.text->val.parameters->coding_params.min_length){
      chars_before=(p_td.text->
              val.parameters->coding_params.min_length-val_ptr->n_octets)*2;
    }
  }

  if(chars_before){
      unsigned char* p=NULL;
      size_t len=chars_before;
      buff.get_end(p,len);
      for(int a=0;a<chars_before;a++) p[a]=(unsigned char)'0';
      buff.increase_length(chars_before);
      encoded_length+=chars_before;
  }

  if(val_ptr->n_octets){
      unsigned char* p=NULL;
      size_t len=val_ptr->n_octets*2;
      buff.get_end(p,len);
      len=val_ptr->n_octets;
      for(size_t i=0;i<len;i++){
        p[2 * i] = hexdigit_to_char(val_ptr->octets_ptr[i] >> 4);
        p[2 * i + 1] = hexdigit_to_char(val_ptr->octets_ptr[i] & 0x0F);
      }
      buff.increase_length(len*2);
      encoded_length+=len*2;
  }


  if(p_td.text->end_encode){
    buff.put_cs(*p_td.text->end_encode);
    encoded_length+=p_td.text->end_encode->lengthof();
  }
  return encoded_length;
}

int OCTETSTRING::RAW_encode(const TTCN_Typedescriptor_t& p_td,
  RAW_enc_tree& myleaf) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
  }
  unsigned char *bc;
  int bl = val_ptr->n_octets * 8;
  int align_length = p_td.raw->fieldlength ? p_td.raw->fieldlength - bl : 0;
  int blength = val_ptr->n_octets;
  if ((bl + align_length) < val_ptr->n_octets * 8) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There are insufficient bits to encode '%s': ", p_td.name);
    blength = p_td.raw->fieldlength / 8;
    bl = p_td.raw->fieldlength;
    align_length = 0;
  }
  if (myleaf.must_free) Free(myleaf.body.leaf.data_ptr);
  myleaf.must_free = FALSE;
  myleaf.data_ptr_used = TRUE;
  if (p_td.raw->extension_bit != EXT_BIT_NO
    && myleaf.coding_par.bitorder == ORDER_MSB) {
    if (blength > RAW_INT_ENC_LENGTH) {
      myleaf.body.leaf.data_ptr = bc = (unsigned char*)Malloc(blength*sizeof(*bc));
      myleaf.must_free = TRUE;
      myleaf.data_ptr_used = TRUE;
    }
    else {
      bc = myleaf.body.leaf.data_array;
      myleaf.data_ptr_used = FALSE;
    }
    for (int a = 0; a < blength; a++) bc[a] = val_ptr->octets_ptr[a] << 1;
  }
  else myleaf.body.leaf.data_ptr = val_ptr->octets_ptr;
  if (p_td.raw->endianness == ORDER_MSB) myleaf.align = -align_length;
  else myleaf.align = align_length;
  return myleaf.length = bl + align_length;
}

int OCTETSTRING::RAW_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff, int limit, raw_order_t top_bit_ord, boolean no_err,
  int /*sel_field*/, boolean /*first_call*/)
{
  int prepaddlength = buff.increase_pos_padd(p_td.raw->prepadding);
  limit -= prepaddlength;
  int decode_length = p_td.raw->fieldlength == 0
    ? (limit / 8) * 8 : p_td.raw->fieldlength;
  if (decode_length > limit || decode_length > (int) buff.unread_len_bit()) {
    if (no_err) return -TTCN_EncDec::ET_LEN_ERR;
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There is not enough bits in the buffer to decode type %s.", p_td.name);
    decode_length = ((limit > (int) buff.unread_len_bit()
      ? (int)buff.unread_len_bit() : limit) / 8) * 8;
  }
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
  if (p_td.raw->extension_bit != EXT_BIT_NO) {
    const unsigned char* data = buff.get_read_data();
    int count = 1;
    int rot = top_bit_ord == ORDER_LSB ? 0 : 7;
    if (p_td.raw->extension_bit == EXT_BIT_YES) {
      while (((data[count - 1] >> rot) & 0x01) == 0 && count * 8 < decode_length)
        count++;
    }
    else {
      while (((data[count - 1] >> rot) & 0x01) == 1 && count * 8 < decode_length)
        count++;
    }
    decode_length = count * 8;
  }
  clean_up();
  init_struct(decode_length / 8);
  buff.get_b((size_t) decode_length, val_ptr->octets_ptr, cp, top_bit_ord);

  if (p_td.raw->length_restrition != -1) {
    val_ptr->n_octets = p_td.raw->length_restrition;
    if (p_td.raw->endianness == ORDER_MSB) memmove(val_ptr->octets_ptr,
      val_ptr->octets_ptr + (decode_length / 8 - val_ptr->n_octets),
      val_ptr->n_octets * sizeof(unsigned char));
  }
  if (p_td.raw->extension_bit != EXT_BIT_NO && cp.bitorder == ORDER_MSB) {
    for (int a = 0; a < decode_length / 8; a++)
      val_ptr->octets_ptr[a] = val_ptr->octets_ptr[a] >> 1 | val_ptr->octets_ptr[a] << 7;
  }
  decode_length += buff.increase_pos_padd(p_td.raw->padding);
  return decode_length + prepaddlength;
}

void OCTETSTRING::dump () const
{
  if (val_ptr != NULL) {
    printf("octetstring(%d) :\n", val_ptr->n_octets);
    for (int i = 0; i < val_ptr->n_octets; i++) {
      printf("%02X ", val_ptr->octets_ptr[i]);
    }
    printf("\n");
  }
}

int OCTETSTRING::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound octetstring value.");
    return -1;
  }
  
  char* tmp_str = (char*)Malloc(val_ptr->n_octets * 2 + 3);
  tmp_str[0] = '\"';
  tmp_str[val_ptr->n_octets * 2 + 1] = '\"';
  for(int i = 0; i < val_ptr->n_octets; ++i) {
    tmp_str[2 * i + 1] = hexdigit_to_char(val_ptr->octets_ptr[i] >> 4);
    tmp_str[2 * i + 2] = hexdigit_to_char(val_ptr->octets_ptr[i] & 0x0F);
  }
  tmp_str[val_ptr->n_octets * 2 + 2] = 0;
  int enc_len = p_tok.put_next_token(JSON_TOKEN_STRING, tmp_str);
  Free(tmp_str);
  return enc_len;
}

int OCTETSTRING::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
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
    if (use_default || (value_len >= 2 && value[0] == '\"' && value[value_len - 1] == '\"')) {
      if (!use_default) {
        // The default value doesn't have quotes around it
        value_len -= 2;
        ++value;
      }
      // White spaces are ignored, so the resulting octetstring might be shorter
      // than the extracted JSON string
      int nibbles = value_len;
      for (size_t i = 0; i < value_len; ++i) {
        if (value[i] == ' ') {
          --nibbles;
        }
        else if (!isxdigit(value[i]) || i + 1 == value_len ||
                 !isxdigit(value[i + 1])) {
          if (value[i] == '\\' && i + 1 < value_len &&
              (value[i + 1] == 'n' || value[i + 1] == 'r' || value[i + 1] == 't')) {
            // Escaped white space character
            ++i;
            nibbles -= 2;
          }
          else {
            error = TRUE;
            break;
          }
        }
        else {
          // It's a valid octet (jump through its second nibble)
          ++i;
        }
      }
      if (!error) {
        init_struct(nibbles / 2);
        if (value_len != 0) {
          int octet_index = 0;
          for (size_t i = 0; i < value_len - 1; ++i) {
            if (!isxdigit(value[i]) || !isxdigit(value[i + 1])) {
              continue;
            }
            unsigned char upper_nibble = char_to_hexdigit(value[i]);
            unsigned char lower_nibble = char_to_hexdigit(value[i + 1]);
            val_ptr->octets_ptr[octet_index] = (upper_nibble << 4) | lower_nibble;
            ++octet_index;
            ++i;
          }
        }
      }
    } else {
      error = TRUE;
    }
  } else {
    return JSON_ERROR_INVALID_TOKEN;
  }
  
  if (error) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FORMAT_ERROR, "string", "octetstring");
    return JSON_ERROR_FATAL;    
  }
  return (int)dec_len;
}

// octetstring element class

OCTETSTRING_ELEMENT::OCTETSTRING_ELEMENT(boolean par_bound_flag,
  OCTETSTRING& par_str_val, int par_octet_pos)
  : bound_flag(par_bound_flag), str_val(par_str_val), octet_pos(par_octet_pos)
{
}

OCTETSTRING_ELEMENT& OCTETSTRING_ELEMENT::operator=
(const OCTETSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound octetstring value.");
  if(other_value.val_ptr->n_octets != 1)
    TTCN_error("Assignment of an octetstring "
               "with length other than 1 to an octetstring element.");
  bound_flag = TRUE;
  str_val.copy_value();
  str_val.val_ptr->octets_ptr[octet_pos] = other_value.val_ptr->octets_ptr[0];
  return *this;
}

OCTETSTRING_ELEMENT& OCTETSTRING_ELEMENT::operator=
(const OCTETSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound octetstring element.");
  if (&other_value != this) {
    bound_flag = TRUE;
    str_val.copy_value();
    str_val.val_ptr->octets_ptr[octet_pos] =
      other_value.str_val.val_ptr->octets_ptr[other_value.octet_pos];
  }
  return *this;
}

boolean OCTETSTRING_ELEMENT::operator==(const OCTETSTRING& other_value) const
{
  must_bound("Unbound left operand of octetstring element comparison.");
  other_value.must_bound("Unbound right operand of octetstring comparison.");
  if(other_value.val_ptr->n_octets != 1) return FALSE;
  return str_val.val_ptr->octets_ptr[octet_pos] ==
    other_value.val_ptr->octets_ptr[0];
}

boolean OCTETSTRING_ELEMENT::operator==
(const OCTETSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of octetstring element comparison.");
  other_value.must_bound("Unbound right operand of octetstring element "
    "comparison.");
  return str_val.val_ptr->octets_ptr[octet_pos] ==
    other_value.str_val.val_ptr->octets_ptr[other_value.octet_pos];
}

OCTETSTRING OCTETSTRING_ELEMENT::operator+(const OCTETSTRING& other_value) const
{
  must_bound("Unbound left operand of octetstring element concatenation.");
  other_value.must_bound("Unbound right operand of octetstring concatenation.");
  OCTETSTRING ret_val(other_value.val_ptr->n_octets + 1);
  ret_val.val_ptr->octets_ptr[0] = str_val.val_ptr->octets_ptr[octet_pos];
  memcpy(ret_val.val_ptr->octets_ptr + 1,
         other_value.val_ptr->octets_ptr, other_value.val_ptr->n_octets);
  return ret_val;
}

OCTETSTRING OCTETSTRING_ELEMENT::operator+
(const OCTETSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of octetstring element concatenation.");
  other_value.must_bound("Unbound right operand of octetstring element "
                         "concatenation.");
  unsigned char result[2];
  result[0] = str_val.val_ptr->octets_ptr[octet_pos];
  result[1] = other_value.str_val.val_ptr->octets_ptr[other_value.octet_pos];
  return OCTETSTRING(2, result);
}

#ifdef TITAN_RUNTIME_2
OCTETSTRING OCTETSTRING_ELEMENT::operator+(
  const OPTIONAL<OCTETSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const OCTETSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of octetstring concatenation.");
}
#endif

OCTETSTRING OCTETSTRING_ELEMENT::operator~() const
{
  must_bound("Unbound octetstring element operand of operator not4b.");
  unsigned char result = ~str_val.val_ptr->octets_ptr[octet_pos];
  return OCTETSTRING(1, &result);
}

OCTETSTRING OCTETSTRING_ELEMENT::operator&(const OCTETSTRING& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound octetstring "
    "element.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "octetstring value.");
  if (other_value.val_ptr->n_octets != 1)
    TTCN_error("The octetstring "
               "operands of operator and4b must have the same length.");
  unsigned char result = str_val.val_ptr->octets_ptr[octet_pos] &
    other_value.val_ptr->octets_ptr[0];
  return OCTETSTRING(1, &result);
}

OCTETSTRING OCTETSTRING_ELEMENT::operator&
(const OCTETSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound octetstring "
    "element.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "octetstring element.");
  unsigned char result = str_val.val_ptr->octets_ptr[octet_pos] &
    other_value.str_val.val_ptr->octets_ptr[other_value.octet_pos];
  return OCTETSTRING(1, &result);
}

OCTETSTRING OCTETSTRING_ELEMENT::operator|(const OCTETSTRING& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound octetstring "
    "element.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "octetstring value.");
  if (other_value.val_ptr->n_octets != 1)
    TTCN_error("The octetstring "
               "operands of operator or4b must have the same length.");
  unsigned char result = str_val.val_ptr->octets_ptr[octet_pos] |
    other_value.val_ptr->octets_ptr[0];
  return OCTETSTRING(1, &result);
}

OCTETSTRING OCTETSTRING_ELEMENT::operator|
(const OCTETSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound octetstring "
    "element.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "octetstring element.");
  unsigned char result = str_val.val_ptr->octets_ptr[octet_pos] |
    other_value.str_val.val_ptr->octets_ptr[other_value.octet_pos];
  return OCTETSTRING(1, &result);
}

OCTETSTRING OCTETSTRING_ELEMENT::operator^(const OCTETSTRING& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound octetstring "
    "element.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "octetstring value.");
  if (other_value.val_ptr->n_octets != 1)
    TTCN_error("The octetstring "
               "operands of operator xor4b must have the same length.");
  unsigned char result = str_val.val_ptr->octets_ptr[octet_pos] ^
    other_value.val_ptr->octets_ptr[0];
  return OCTETSTRING(1, &result);
}

OCTETSTRING OCTETSTRING_ELEMENT::operator^
(const OCTETSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound octetstring "
    "element.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "octetstring element.");
  unsigned char result = str_val.val_ptr->octets_ptr[octet_pos] ^
    other_value.str_val.val_ptr->octets_ptr[other_value.octet_pos];
  return OCTETSTRING(1, &result);
}

unsigned char OCTETSTRING_ELEMENT::get_octet() const
{
  return str_val.val_ptr->octets_ptr[octet_pos];
}

void OCTETSTRING_ELEMENT::log() const
{
  if (bound_flag) {
    unsigned char octet = str_val.val_ptr->octets_ptr[octet_pos];
    TTCN_Logger::log_char('\'');
    TTCN_Logger::log_octet(octet);
    TTCN_Logger::log_event_str("'O");
    if (TTCN_Logger::is_printable(octet)) {
      TTCN_Logger::log_event_str(" (\"");
      TTCN_Logger::log_char_escaped(octet);
      TTCN_Logger::log_event_str("\")");
    }
  } else TTCN_Logger::log_event_unbound();
}

// octetstring template class

void OCTETSTRING_template::clean_up()
{
  switch (template_selection) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  case STRING_PATTERN:
    if (pattern_value->ref_count > 1) pattern_value->ref_count--;
    else if (pattern_value->ref_count == 1) Free(pattern_value);
    else TTCN_error("Internal error: Invalid reference counter in an "
      "octetstring pattern.");
    break;
  case DECODE_MATCH:
    if (dec_match->ref_count > 1) {
      dec_match->ref_count--;
    }
    else if (dec_match->ref_count == 1) {
      delete dec_match->instance;
      delete dec_match;
    }
    else {
      TTCN_error("Internal error: Invalid reference counter in a "
        "decoded content match.");
    }
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void OCTETSTRING_template::copy_template
  (const OCTETSTRING_template& other_value)
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
    value_list.list_value = new OCTETSTRING_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
        other_value.value_list.list_value[i]);
    break;
  case STRING_PATTERN:
    pattern_value = other_value.pattern_value;
    pattern_value->ref_count++;
    break;
  case DECODE_MATCH:
    dec_match = other_value.dec_match;
    dec_match->ref_count++;
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported octetstring template.");
  }
  set_selection(other_value);
}

/*
  This is the same algorithm that match_array uses
    to match 'record of' types.
  The only differences are: how two elements are matched and
    how an asterisk or ? is identified in the template
*/
boolean OCTETSTRING_template::match_pattern(
  const octetstring_pattern_struct *string_pattern,
  const OCTETSTRING::octetstring_struct *string_value)
{
  // the empty pattern matches the empty octetstring only
  if (string_pattern->n_elements == 0) return string_value->n_octets == 0;

  int value_index = 0;
  unsigned int template_index = 0;
  int last_asterisk = -1;
  int last_value_to_asterisk = -1;
  //this variable is used to speed up the function
  unsigned short pattern_element;

  for(;;)
  {
    pattern_element = string_pattern->elements_ptr[template_index];
    if( pattern_element < 256){
      if(string_value->octets_ptr[value_index] == pattern_element)
      {
        value_index++;
        template_index++;
      }else{
        if(last_asterisk == -1) return FALSE;
        template_index = last_asterisk +1;
        value_index = ++last_value_to_asterisk;
      }
    } else if(pattern_element == 256)
    {//? found
      value_index++;
      template_index++;
    }else if(pattern_element == 257)
    {//* found
      last_asterisk = template_index++;
      last_value_to_asterisk = value_index;
    } else TTCN_error("Internal error: invalid element in an octetstring "
        "pattern.");

    if(value_index == string_value->n_octets
      && template_index == string_pattern->n_elements)
    {
      return TRUE;
    }else if (template_index == string_pattern->n_elements)
    {
      if(string_pattern->elements_ptr[template_index - 1] == 257)
      {
        return TRUE;
      }else if(last_asterisk == -1)
      {
        return FALSE;
      }else{
        template_index = last_asterisk+1;
        value_index = ++last_value_to_asterisk;
      }
    }else if(value_index == string_value->n_octets)
    {
      while(template_index < string_pattern->n_elements
        && string_pattern->elements_ptr[template_index] == 257)
      template_index++;

      return template_index == string_pattern->n_elements;
    }
  }
}

OCTETSTRING_template::OCTETSTRING_template()
{
}

OCTETSTRING_template::OCTETSTRING_template(template_sel other_value)
  : Restricted_Length_Template(other_value)
{
  check_single_selection(other_value);
}

OCTETSTRING_template::OCTETSTRING_template(const OCTETSTRING& other_value)
  : Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

OCTETSTRING_template::OCTETSTRING_template
  (const OCTETSTRING_ELEMENT& other_value)
  : Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

OCTETSTRING_template::OCTETSTRING_template
(const OPTIONAL<OCTETSTRING>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const OCTETSTRING&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating an octetstring template from an unbound optional "
      "field.");
  }
}

OCTETSTRING_template::OCTETSTRING_template(unsigned int n_elements,
  const unsigned short *pattern_elements)
  : Restricted_Length_Template(STRING_PATTERN)
{
  pattern_value = (octetstring_pattern_struct*)
    Malloc(sizeof(octetstring_pattern_struct) +
      (n_elements - 1) * sizeof(unsigned short));
  pattern_value->ref_count = 1;
  pattern_value->n_elements = n_elements;
  memcpy(pattern_value->elements_ptr, pattern_elements,
    n_elements * sizeof(unsigned short));
}

OCTETSTRING_template::OCTETSTRING_template
  (const OCTETSTRING_template& other_value)
: Restricted_Length_Template()
{
  copy_template(other_value);
}

OCTETSTRING_template::~OCTETSTRING_template()
{
  clean_up();
}

OCTETSTRING_template& OCTETSTRING_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

OCTETSTRING_template& OCTETSTRING_template::operator=
(const OCTETSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound octetstring value to a "
                         "template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

OCTETSTRING_template& OCTETSTRING_template::operator=
  (const OCTETSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound octetstring element to a "
                         "template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

OCTETSTRING_template& OCTETSTRING_template::operator=
  (const OPTIONAL<OCTETSTRING>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const OCTETSTRING&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to an octetstring "
      "template.");
  }
  return *this;
}

OCTETSTRING_template& OCTETSTRING_template::operator=
(const OCTETSTRING_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

#ifdef TITAN_RUNTIME_2
void OCTETSTRING_template::concat(Vector<unsigned short>& v) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    switch (length_restriction_type) {
    case NO_LENGTH_RESTRICTION:
      if (template_selection == ANY_VALUE) {
        // ? => '*'
        if (v.size() == 0 || v[v.size() - 1] != 257) {
          // '**' == '*', so just ignore the second '*'
          v.push_back(257);
        }
      }
      else {
        TTCN_error("Operand of octetstring template concatenation is an "
          "AnyValueOrNone (*) matching mechanism with no length restriction");
      }
      break;
    case RANGE_LENGTH_RESTRICTION:
      if (!length_restriction.range_length.max_length ||
          length_restriction.range_length.max_length != length_restriction.range_length.min_length) {
        TTCN_error("Operand of octetstring template concatenation is an %s "
          "matching mechanism with non-fixed length restriction",
          template_selection == ANY_VALUE ? "AnyValue (?)" : "AnyValueOrNone (*)");
      }
      // else fall through (range length restriction is allowed if the minimum
      // and maximum value are the same)
    case SINGLE_LENGTH_RESTRICTION: {
      // ? length(N) or * length(N) => '??...?' N times
      int len = length_restriction_type == SINGLE_LENGTH_RESTRICTION ?
        length_restriction.single_length : length_restriction.range_length.min_length;
      for (int i = 0; i < len; ++i) {
        v.push_back(256);
      }
      break; }
    }
    break;
  case SPECIFIC_VALUE:
    concat(v, single_value);
    break;
  case STRING_PATTERN:
    for (unsigned int i = 0; i < pattern_value->n_elements; ++i) {
      v.push_back(pattern_value->elements_ptr[i]);
    }
    break;
  default:
    TTCN_error("Operand of octetstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
}

void OCTETSTRING_template::concat(Vector<unsigned short>& v, const OCTETSTRING& val)
{
  if (!val.is_bound()) {
    TTCN_error("Operand of octetstring template concatenation is an "
      "unbound value.");
  }
  for (int i = 0; i < val.val_ptr->n_octets; ++i) {
    v.push_back(val.val_ptr->octets_ptr[i]);
  }
}

void OCTETSTRING_template::concat(Vector<unsigned short>& v, template_sel sel)
{
  if (sel == ANY_VALUE) {
    // ? => '*'
    if (v.size() == 0 || v[v.size() - 1] != 257) {
      // '**' == '*', so just ignore the second '*'
      v.push_back(257);
    }
  }
  else {
    TTCN_error("Operand of octetstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
}


OCTETSTRING_template OCTETSTRING_template::operator+(
  const OCTETSTRING_template& other_value) const
{
  if (template_selection == SPECIFIC_VALUE &&
      other_value.template_selection == SPECIFIC_VALUE) {
    // result is a specific value template
    return single_value + other_value.single_value;
  }
  if (template_selection == ANY_VALUE &&
      other_value.template_selection == ANY_VALUE &&
      length_restriction_type == NO_LENGTH_RESTRICTION &&
      other_value.length_restriction_type == NO_LENGTH_RESTRICTION) {
    // special case: ? & ? => ?
    return OCTETSTRING_template(ANY_VALUE);
  }
  // otherwise the result is an octetstring pattern
  Vector<unsigned short> v;
  concat(v);
  other_value.concat(v);
  return OCTETSTRING_template(v.size(), v.data_ptr());
}

OCTETSTRING_template OCTETSTRING_template::operator+(
  const OCTETSTRING& other_value) const
{
  if (template_selection == SPECIFIC_VALUE) {
    // result is a specific value template
    return single_value + other_value;
  }
  // otherwise the result is an octetstring pattern
  Vector<unsigned short> v;
  concat(v);
  concat(v, other_value);
  return OCTETSTRING_template(v.size(), v.data_ptr());
}

OCTETSTRING_template OCTETSTRING_template::operator+(
  const OCTETSTRING_ELEMENT& other_value) const
{
  return *this + OCTETSTRING(other_value);
}

OCTETSTRING_template OCTETSTRING_template::operator+(
  const OPTIONAL<OCTETSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const OCTETSTRING&)other_value;
  }
  TTCN_error("Operand of octetstring template concatenation is an "
    "unbound or omitted record/set field.");
}

OCTETSTRING_template OCTETSTRING_template::operator+(
  template_sel other_template_sel) const
{
  if (template_selection == ANY_VALUE && other_template_sel == ANY_VALUE &&
      length_restriction_type == NO_LENGTH_RESTRICTION) {
    // special case: ? & ? => ?
    return OCTETSTRING_template(ANY_VALUE);
  }
  // the result is always an octetstring pattern
  Vector<unsigned short> v;
  concat(v);
  concat(v, other_template_sel);
  return OCTETSTRING_template(v.size(), v.data_ptr());
}

OCTETSTRING_template operator+(const OCTETSTRING& left_value,
  const OCTETSTRING_template& right_template)
{
  if (right_template.template_selection == SPECIFIC_VALUE) {
    // result is a specific value template
    return left_value + right_template.single_value;
  }
  // otherwise the result is an octetstring pattern
  Vector<unsigned short> v;
  OCTETSTRING_template::concat(v, left_value);
  right_template.concat(v);
  return OCTETSTRING_template(v.size(), v.data_ptr());
}

OCTETSTRING_template operator+(const OCTETSTRING_ELEMENT& left_value,
  const OCTETSTRING_template& right_template)
{
  return OCTETSTRING(left_value) + right_template;
}

OCTETSTRING_template operator+(const OPTIONAL<OCTETSTRING>& left_value,
  const OCTETSTRING_template& right_template)
{
  if (left_value.is_present()) {
    return (const OCTETSTRING&)left_value + right_template;
  }
  TTCN_error("Operand of octetstring template concatenation is an "
    "unbound or omitted record/set field.");
}

OCTETSTRING_template operator+(template_sel left_template_sel,
  const OCTETSTRING_template& right_template)
{
  if (left_template_sel == ANY_VALUE &&
      right_template.template_selection == ANY_VALUE &&
      right_template.length_restriction_type ==
      Restricted_Length_Template::NO_LENGTH_RESTRICTION) {
    // special case: ? & ? => ?
    return OCTETSTRING_template(ANY_VALUE);
  }
  // the result is always an octetstring pattern
  Vector<unsigned short> v;
  OCTETSTRING_template::concat(v, left_template_sel);
  right_template.concat(v);
  return OCTETSTRING_template(v.size(), v.data_ptr());
}

OCTETSTRING_template operator+(const OCTETSTRING& left_value,
  template_sel right_template_sel)
{
  // the result is always an octetstring pattern
  Vector<unsigned short> v;
  OCTETSTRING_template::concat(v, left_value);
  OCTETSTRING_template::concat(v, right_template_sel);
  return OCTETSTRING_template(v.size(), v.data_ptr());
}

OCTETSTRING_template operator+(const OCTETSTRING_ELEMENT& left_value,
  template_sel right_template_sel)
{
  return OCTETSTRING(left_value) + right_template_sel;
}

OCTETSTRING_template operator+(const OPTIONAL<OCTETSTRING>& left_value,
  template_sel right_template_sel)
{
  if (left_value.is_present()) {
    return (const OCTETSTRING&)left_value + right_template_sel;
  }
  TTCN_error("Operand of octetstring template concatenation is an "
    "unbound or omitted record/set field.");
}

OCTETSTRING_template operator+(template_sel left_template_sel,
  const OCTETSTRING& right_value)
{
  // the result is always an octetstring pattern
  Vector<unsigned short> v;
  OCTETSTRING_template::concat(v, left_template_sel);
  OCTETSTRING_template::concat(v, right_value);
  return OCTETSTRING_template(v.size(), v.data_ptr());
}

OCTETSTRING_template operator+(template_sel left_template_sel,
  const OCTETSTRING_ELEMENT& right_value)
{
  return left_template_sel + OCTETSTRING(right_value);
}

OCTETSTRING_template operator+(template_sel left_template_sel,
  const OPTIONAL<OCTETSTRING>& right_value)
{
  if (right_value.is_present()) {
    return left_template_sel + (const OCTETSTRING&)right_value;
  }
  TTCN_error("Operand of octetstring template concatenation is an "
    "unbound or omitted record/set field.");
}
#endif // TITAN_RUNTIME_2

OCTETSTRING_ELEMENT OCTETSTRING_template::operator[](int index_value)
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Accessing an octetstring element of a non-specific "
               "octetstring template.");
  return single_value[index_value];
}

OCTETSTRING_ELEMENT OCTETSTRING_template::operator[](const INTEGER& index_value)
{
  index_value.must_bound("Indexing a octetstring value with an unbound "
                         "integer value.");
  return (*this)[(int)index_value];
}

const OCTETSTRING_ELEMENT OCTETSTRING_template::operator[](int index_value) const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Accessing an octetstring element of a non-specific "
               "octetstring template.");
  return single_value[index_value];
}

const OCTETSTRING_ELEMENT OCTETSTRING_template::operator[](const INTEGER& index_value) const
{
  index_value.must_bound("Indexing a octetstring template with an unbound "
                         "integer value.");
  return (*this)[(int)index_value];
}

boolean OCTETSTRING_template::match(const OCTETSTRING& other_value,
                                    boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  if (!match_length(other_value.val_ptr->n_octets)) return FALSE;
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
    for(unsigned int i = 0; i < value_list.n_values; i++)
      if (value_list.list_value[i].match(other_value))
        return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  case STRING_PATTERN:
    return match_pattern(pattern_value, other_value.val_ptr);
  case DECODE_MATCH: {
    TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);
    TTCN_EncDec::clear_error();
    TTCN_Buffer buff(other_value);
    boolean ret_val = dec_match->instance->match(buff);
    TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL,TTCN_EncDec::EB_DEFAULT);
    TTCN_EncDec::clear_error();
    return ret_val; }
  default:
    TTCN_error("Matching an uninitialized/unsupported octetstring template.");
  }
  return FALSE;
}

const OCTETSTRING& OCTETSTRING_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific "
      "octetstring template.");
  return single_value;
}

int OCTETSTRING_template::lengthof() const
{
  int min_length;
  boolean has_any_or_none;
  if (is_ifpresent)
    TTCN_error("Performing lengthof() operation on a octetstring template "
               "which has an ifpresent attribute.");
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
    min_length = single_value.lengthof();
    has_any_or_none = FALSE;
    break;
  case OMIT_VALUE:
    TTCN_error("Performing lengthof() operation on an octetstring template "
               "containing omit value.");
  case ANY_VALUE:
  case ANY_OR_OMIT:
    min_length = 0;
    has_any_or_none = TRUE; // max. length is infinity
    break;
  case VALUE_LIST:
  {
    // error if any element does not have length or the lengths differ
    if (value_list.n_values<1)
      TTCN_error("Internal error: "
                 "Performing lengthof() operation on an octetstring template "
                 "containing an empty list.");
    int item_length = value_list.list_value[0].lengthof();
    for (unsigned int i = 1; i < value_list.n_values; i++) {
      if (value_list.list_value[i].lengthof()!=item_length)
        TTCN_error("Performing lengthof() operation on an octetstring template "
                   "containing a value list with different lengths.");
    }
    min_length = item_length;
    has_any_or_none = FALSE;
    break;
  }
  case COMPLEMENTED_LIST:
    TTCN_error("Performing lengthof() operation on an octetstring template "
               "containing complemented list.");
  case STRING_PATTERN:
    min_length = 0;
    has_any_or_none = FALSE; // TRUE if * chars in the pattern
    for (unsigned int i = 0; i < pattern_value->n_elements; i++)
    {
      if (pattern_value->elements_ptr[i] < 257) min_length++;
      else has_any_or_none = TRUE;   // case of * character
    }
    break;
  default:
    TTCN_error("Performing lengthof() operation on an "
               "uninitialized/unsupported octetstring template.");
  }
  return check_section_is_single(min_length, has_any_or_none,
                                 "length", "an", "octetstring template");
}

void OCTETSTRING_template::set_type(template_sel template_type,
  unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST &&
      template_type != DECODE_MATCH)
    TTCN_error("Setting an invalid type for an octetstring template.");
  clean_up();
  set_selection(template_type);
  if (template_type != DECODE_MATCH) {
    value_list.n_values = list_length;
    value_list.list_value = new OCTETSTRING_template[list_length];
  }
}

OCTETSTRING_template& OCTETSTRING_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
      template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list octetstring template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in an octetstring value list template.");
  return value_list.list_value[list_index];
}

void OCTETSTRING_template::set_decmatch(Dec_Match_Interface* new_instance)
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Setting the decoded content matching mechanism of a non-decmatch "
      "octetstring template.");
  }
  dec_match = new decmatch_struct;
  dec_match->ref_count = 1;
  dec_match->instance = new_instance;
}

void* OCTETSTRING_template::get_decmatch_dec_res() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the decoding result of a non-decmatch octetstring "
      "template.");
  }
  return dec_match->instance->get_dec_res();
}

const TTCN_Typedescriptor_t* OCTETSTRING_template::get_decmatch_type_descr() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the decoded type's descriptor in a non-decmatch "
      "octetstring template.");
  }
  return dec_match->instance->get_type_descr();
}

void OCTETSTRING_template::log() const
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
  case STRING_PATTERN:
    TTCN_Logger::log_char('\'');
    for (unsigned int i = 0; i < pattern_value->n_elements; i++) {
      unsigned short pattern = pattern_value->elements_ptr[i];
      if (pattern < 256) TTCN_Logger::log_octet(pattern);
      else if (pattern == 256) TTCN_Logger::log_char('?');
      else if (pattern == 257) TTCN_Logger::log_char('*');
      else TTCN_Logger::log_event_str("<unknown>");
    }
    TTCN_Logger::log_event_str("'O");
    break;
  case DECODE_MATCH:
    TTCN_Logger::log_event_str("decmatch ");
    dec_match->instance->log();
    break;
  default:
    log_generic();
    break;
  }
  log_restricted();
  log_ifpresent();
}

void OCTETSTRING_template::log_match(const OCTETSTRING& match_value,
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

void OCTETSTRING_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE|Module_Param::BC_LIST, "octetstring template");
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
    OCTETSTRING_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Octetstring:
    *this = OCTETSTRING(mp->get_string_size(), (unsigned char*)mp->get_string_data());
    break;
  case Module_Param::MP_Octetstring_Template:
    *this = OCTETSTRING_template(mp->get_string_size(), (unsigned short*)mp->get_string_data());
    break;
  case Module_Param::MP_Expression:
    if (mp->get_expr_type() == Module_Param::EXPR_CONCATENATE) {
      OCTETSTRING operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 + operand2;
    }
    else {
      param.expr_type_error("a bitstring");
    }
    break;
  default:
    param.type_error("octetstring template");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
  if (param.get_length_restriction() != NULL) {
    set_length_range(param);
  }
  else {
    set_length_range(*mp);
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* OCTETSTRING_template::get_param(Module_Param_Name& param_name) const
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
  case STRING_PATTERN: {
    unsigned short* val_cpy = (unsigned short*)Malloc(pattern_value->n_elements *
      sizeof(unsigned short));
    memcpy(val_cpy, pattern_value->elements_ptr, pattern_value->n_elements *
      sizeof(unsigned short));
    mp = new Module_Param_Octetstring_Template(pattern_value->n_elements, val_cpy);
    break; }
  case DECODE_MATCH:
    TTCN_error("Referencing a decoded content matching template is not supported.");
    break;
  default:
    TTCN_error("Referencing an uninitialized/unsupported octetstring template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  mp->set_length_restriction(get_length_range());
  return mp;
}
#endif

void OCTETSTRING_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_restricted(text_buf);
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
  case STRING_PATTERN:
    text_buf.push_int(pattern_value->n_elements);
    for (unsigned int i = 0; i < pattern_value->n_elements; i++)
      text_buf.push_int(pattern_value->elements_ptr[i]);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported "
               "octetstring template.");
  }
}

void OCTETSTRING_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_restricted(text_buf);
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
    value_list.list_value = new OCTETSTRING_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
   case STRING_PATTERN: {
     unsigned int n_elements = text_buf.pull_int().get_val();
     pattern_value = (octetstring_pattern_struct*)
       Malloc(sizeof(octetstring_pattern_struct) + (n_elements - 1) *
         sizeof(unsigned short));
     pattern_value->ref_count = 1;
     pattern_value->n_elements = n_elements;
     for (unsigned int i = 0; i < n_elements; i++)
       pattern_value->elements_ptr[i] = text_buf.pull_int().get_val();
     break;}
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was "
               "received for an octetstring template.");
  }
}

boolean OCTETSTRING_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean OCTETSTRING_template::match_omit(boolean legacy /* = FALSE */) const
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
void OCTETSTRING_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "octetstring");
}
#endif
