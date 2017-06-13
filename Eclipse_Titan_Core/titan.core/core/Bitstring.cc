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
 *
 ******************************************************************************/
#include <string.h>

#include "Bitstring.hh"
#include "../common/memory.h"
#include "Integer.hh"
#include "String_struct.hh"
#include "Parameters.h"
#include "Param_Types.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Encdec.hh"
#include "Addfunc.hh"
#include "Optional.hh"

#include "../common/dbgnew.hh"

// bitstring value class

/** The amount of memory needed for a bitstring containing n bits. */
#define MEMORY_SIZE(n) (sizeof(bitstring_struct) - sizeof(int) + ((n) + 7) / 8)

void BITSTRING::init_struct(int n_bits)
{
  if (n_bits < 0) {
    val_ptr = NULL;
    TTCN_error("Initializing a bitstring with a negative length.");
  } else if (n_bits == 0) {
    /** This will represent the empty strings so they won't need allocated
      * memory, this delays the memory allocation until it is really needed.
      */
    static bitstring_struct empty_string = { 1, 0, "" };
    val_ptr = &empty_string;
    empty_string.ref_count++;
  } else {
    val_ptr = (bitstring_struct*)Malloc(MEMORY_SIZE(n_bits));
    val_ptr->ref_count = 1;
    val_ptr->n_bits = n_bits;
  }
}

boolean BITSTRING::get_bit(int bit_index) const
{
  return val_ptr->bits_ptr[bit_index / 8] & (1 << (bit_index % 8));
}

void BITSTRING::set_bit(int bit_index, boolean new_value)
{
  unsigned char mask = 1 << (bit_index % 8);
  if (new_value) val_ptr->bits_ptr[bit_index / 8] |= mask;
  else           val_ptr->bits_ptr[bit_index / 8] &= ~mask;
}

void BITSTRING::copy_value()
{
  if (val_ptr == NULL || val_ptr->n_bits <= 0)
    TTCN_error("Internal error: Invalid internal data structure when copying "
      "the memory area of a bitstring value.");
  if (val_ptr->ref_count > 1) {
    bitstring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_bits);
    memcpy(val_ptr->bits_ptr, old_ptr->bits_ptr, (old_ptr->n_bits + 7) / 8);
  }
}

void BITSTRING::clear_unused_bits() const
{
  int n_bits = val_ptr->n_bits;
  if (n_bits % 8 != 0) val_ptr->bits_ptr[(n_bits - 1) / 8] &=
                         (unsigned char)'\377' >> (7 - (n_bits - 1) % 8);
}

BITSTRING::BITSTRING(int n_bits)
{
  init_struct(n_bits);
}

BITSTRING::BITSTRING()
{
  val_ptr = NULL;
}

BITSTRING::BITSTRING(int n_bits, const unsigned char *bits_ptr)
{
  init_struct(n_bits);
  memcpy(val_ptr->bits_ptr, bits_ptr, (n_bits + 7) / 8);
  clear_unused_bits();
}

BITSTRING::BITSTRING(const BITSTRING& other_value)
: Base_Type(other_value)
{
  other_value.must_bound("Copying an unbound bitstring value.");
  val_ptr = other_value.val_ptr;
  val_ptr->ref_count++;
}

BITSTRING::BITSTRING(const BITSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Copying an unbound bitstring element.");
  init_struct(1);
  val_ptr->bits_ptr[0] = other_value.get_bit() ? 1 : 0;
}

BITSTRING::~BITSTRING()
{
  clean_up();
}

void BITSTRING::clean_up()
{
  if (val_ptr != NULL) {
    if (val_ptr->ref_count > 1) val_ptr->ref_count--;
    else if (val_ptr->ref_count == 1) Free(val_ptr);
    else TTCN_error("Internal error: Invalid reference counter in a bitstring "
      "value.");
    val_ptr = NULL;
  }
}

BITSTRING& BITSTRING::operator=(const BITSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound bitstring value.");
  if (&other_value != this) {
    clean_up();
    val_ptr = other_value.val_ptr;
    val_ptr->ref_count++;
  }
  return *this;
}

BITSTRING& BITSTRING::operator=(const BITSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound bitstring element to a "
    "bitstring.");
  boolean bit_value = other_value.get_bit();
  clean_up();
  init_struct(1);
  val_ptr->bits_ptr[0] = bit_value ? 1 : 0;
  return *this;
}

boolean BITSTRING::operator==(const BITSTRING& other_value) const
{
  must_bound("Unbound left operand of bitstring comparison.");
  other_value.must_bound("Unbound right operand of bitstring comparison.");
  int n_bits = val_ptr->n_bits;
  if (n_bits != other_value.val_ptr->n_bits) return FALSE;
  if (n_bits == 0) return TRUE;
  clear_unused_bits();
  other_value.clear_unused_bits();
  return !memcmp(val_ptr->bits_ptr, other_value.val_ptr->bits_ptr,
                 (n_bits + 7) / 8);
}

boolean BITSTRING::operator==(const BITSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of bitstring comparison.");
  other_value.must_bound("Unbound right operand of bitstring element "
    "comparison.");
  if (val_ptr->n_bits != 1) return FALSE;
  return get_bit(0) == other_value.get_bit();
}

BITSTRING BITSTRING::operator+(const BITSTRING& other_value) const
{
  must_bound("Unbound left operand of bitstring concatenation.");
  other_value.must_bound("Unbound right operand of bitstring concatenation.");

  int left_n_bits = val_ptr->n_bits;
  if (left_n_bits == 0) return other_value;

  int right_n_bits = other_value.val_ptr->n_bits;
  if (right_n_bits == 0) return *this;

  // the length of result
  int n_bits = left_n_bits + right_n_bits;

  // the number of bytes used
  int left_n_bytes = (left_n_bits + 7) / 8;
  int right_n_bytes = (right_n_bits + 7) / 8;

  // the number of bits used in the last incomplete octet of the left operand
  int left_empty_bits = left_n_bits % 8;

  // the result
  BITSTRING ret_val(n_bits);

  // pointers to the data areas
  const unsigned char *left_ptr = val_ptr->bits_ptr;
  const unsigned char *right_ptr = other_value.val_ptr->bits_ptr;
  unsigned char *dest_ptr = ret_val.val_ptr->bits_ptr;

  // copying the left fragment into the result
  memcpy(dest_ptr, left_ptr, left_n_bytes);

  if (left_empty_bits != 0) {
    // non-trivial case: the length of left fragment is not a multiply of 8
    // the bytes used in the result
    int n_bytes = (n_bits + 7) / 8;
    // placing the bytes from the right fragment until the result is filled
    for (int i = left_n_bytes; i < n_bytes; i++) {
      unsigned char right_byte = right_ptr[i - left_n_bytes];
      // finish filling the previous byte
      dest_ptr[i - 1] |= right_byte << left_empty_bits;
      // start filling the actual byte
      dest_ptr[i] = right_byte >> (8 - left_empty_bits);
    }
    if (left_n_bytes + right_n_bytes > n_bytes) {
      // if the result data area is shorter than the two operands together
      // the last bits of right fragment were not placed into the result
      // in the previous for loop
      dest_ptr[n_bytes - 1] |= right_ptr[right_n_bytes - 1] << left_empty_bits;
    }
  } else {
    // trivial case: just append the bytes of the right fragment
    memcpy(dest_ptr + left_n_bytes, right_ptr, right_n_bytes);
  }
  ret_val.clear_unused_bits();
  return ret_val;
}

BITSTRING BITSTRING::operator+(const BITSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of bitstring concatenation.");
  other_value.must_bound("Unbound right operand of bitstring element "
                         "concatenation.");

  int n_bits = val_ptr->n_bits;
  BITSTRING ret_val(n_bits + 1);
  memcpy(ret_val.val_ptr->bits_ptr, val_ptr->bits_ptr, (n_bits + 7) / 8);
  ret_val.set_bit(n_bits, other_value.get_bit());
  return ret_val;
}

#ifdef TITAN_RUNTIME_2
BITSTRING BITSTRING::operator+(const OPTIONAL<BITSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const BITSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of bitstring concatenation.");
}
#endif

BITSTRING BITSTRING::operator~() const
{
  must_bound("Unbound bitstring operand of operator not4b.");
  int n_bytes = (val_ptr->n_bits + 7) / 8;
  if (n_bytes == 0) return *this;
  BITSTRING ret_val(val_ptr->n_bits);
  for (int i = 0; i < n_bytes; i++)
    ret_val.val_ptr->bits_ptr[i] = ~val_ptr->bits_ptr[i];
  ret_val.clear_unused_bits();
  return ret_val;
}

BITSTRING BITSTRING::operator&(const BITSTRING& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound bitstring value.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "bitstring value.");
  int n_bits = val_ptr->n_bits;
  if (n_bits != other_value.val_ptr->n_bits)
    TTCN_error("The bitstring operands of operator and4b must have the "
               "same length.");
  if (n_bits == 0) return *this;
  BITSTRING ret_val(n_bits);
  int n_bytes = (n_bits + 7) / 8;
  for (int i = 0; i < n_bytes; i++)
    ret_val.val_ptr->bits_ptr[i] = val_ptr->bits_ptr[i] &
      other_value.val_ptr->bits_ptr[i];
  ret_val.clear_unused_bits();
  return ret_val;
}

BITSTRING BITSTRING::operator&(const BITSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound bitstring value.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "bitstring element.");
  if (val_ptr->n_bits != 1)
    TTCN_error("The bitstring operands of "
               "operator and4b must have the same length.");
  unsigned char result = get_bit(0) && other_value.get_bit() ? 1 : 0;
  return BITSTRING(1, &result);
}

BITSTRING BITSTRING::operator|(const BITSTRING& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound bitstring value.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "bitstring value.");
  int n_bits = val_ptr->n_bits;
  if (n_bits != other_value.val_ptr->n_bits)
    TTCN_error("The bitstring operands of operator or4b must have the "
               "same length.");
  if (n_bits == 0) return *this;
  BITSTRING ret_val(n_bits);
  int n_bytes = (n_bits + 7) / 8;
  for (int i = 0; i < n_bytes; i++)
    ret_val.val_ptr->bits_ptr[i] = val_ptr->bits_ptr[i] |
      other_value.val_ptr->bits_ptr[i];
  ret_val.clear_unused_bits();
  return ret_val;
}

BITSTRING BITSTRING::operator|(const BITSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound bitstring value.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "bitstring element.");
  if (val_ptr->n_bits != 1)
    TTCN_error("The bitstring operands of "
               "operator or4b must have the same length.");
  unsigned char result = get_bit(0) || other_value.get_bit() ? 1 : 0;
  return BITSTRING(1, &result);
}

BITSTRING BITSTRING::operator^(const BITSTRING& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound bitstring value.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "bitstring value.");
  int n_bits = val_ptr->n_bits;
  if (n_bits != other_value.val_ptr->n_bits)
    TTCN_error("The bitstring operands of operator xor4b must have the "
               "same length.");
  if (n_bits == 0) return *this;
  BITSTRING ret_val(n_bits);
  int n_bytes = (n_bits + 7) / 8;
  for (int i = 0; i < n_bytes; i++)
    ret_val.val_ptr->bits_ptr[i] = val_ptr->bits_ptr[i] ^
      other_value.val_ptr->bits_ptr[i];
  ret_val.clear_unused_bits();
  return ret_val;
}

BITSTRING BITSTRING::operator^(const BITSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound bitstring value.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "bitstring element.");
  if (val_ptr->n_bits != 1)
    TTCN_error("The bitstring operands of "
               "operator xor4b must have the same length.");
  unsigned char result = get_bit(0) != other_value.get_bit() ? 1 : 0;
  return BITSTRING(1, &result);
}

BITSTRING BITSTRING::operator<<(int shift_count) const
{
  must_bound("Unbound bitstring operand of shift left operator.");
  if (shift_count > 0) {
    int n_bits = val_ptr->n_bits;
    if (n_bits == 0) return *this;
    BITSTRING ret_val(n_bits);
    int n_bytes = (n_bits + 7) / 8;
    clear_unused_bits();
    if (shift_count > n_bits) shift_count = n_bits;
    int shift_bytes = shift_count / 8,
      shift_bits = shift_count % 8;
    if (shift_bits != 0) {
      int byte_count = 0;
      for ( ; byte_count < n_bytes - shift_bytes - 1; byte_count++) {
        ret_val.val_ptr->bits_ptr[byte_count] =
          (val_ptr->bits_ptr[byte_count + shift_bytes] >> shift_bits)|
          (val_ptr->bits_ptr[byte_count + shift_bytes + 1] <<
           (8 - shift_bits));
      }
      ret_val.val_ptr->bits_ptr[n_bytes - shift_bytes - 1] =
        val_ptr->bits_ptr[n_bytes - 1] >> shift_bits;
    } else {
      memcpy(ret_val.val_ptr->bits_ptr, val_ptr->bits_ptr + shift_bytes,
             n_bytes - shift_bytes);
    }
    memset(ret_val.val_ptr->bits_ptr + n_bytes - shift_bytes, 0,
           shift_bytes);
    ret_val.clear_unused_bits();
    return ret_val;
  } else if (shift_count == 0) return *this;
  else return *this >> (-shift_count);
}

BITSTRING BITSTRING::operator<<(const INTEGER& shift_count) const
{
  shift_count.must_bound("Unbound right operand of bitstring shift left "
    "operator.");
  return *this << (int)shift_count;
}

BITSTRING BITSTRING::operator>>(int shift_count) const
{
  must_bound("Unbound bitstring operand of shift right operator.");
  if (shift_count > 0) {
    int n_bits = val_ptr->n_bits;
    if (n_bits == 0) return *this;
    BITSTRING ret_val(n_bits);
    int n_bytes = (n_bits + 7) / 8;
    clear_unused_bits();
    if (shift_count > n_bits) shift_count = n_bits;
    int shift_bytes = shift_count / 8, shift_bits = shift_count % 8;
    memset(ret_val.val_ptr->bits_ptr, 0, shift_bytes);
    if (shift_bits != 0) {
      ret_val.val_ptr->bits_ptr[shift_bytes] =
        val_ptr->bits_ptr[0] << shift_bits;
      for (int byte_count = shift_bytes + 1; byte_count < n_bytes; byte_count++)
      {
        ret_val.val_ptr->bits_ptr[byte_count] =
          (val_ptr->bits_ptr[byte_count - shift_bytes - 1] >> (8 - shift_bits))
	  | (val_ptr->bits_ptr[byte_count - shift_bytes] << shift_bits);
      }
    } else {
      memcpy(ret_val.val_ptr->bits_ptr + shift_bytes, val_ptr->bits_ptr,
             n_bytes - shift_bytes);
    }
    ret_val.clear_unused_bits();
    return ret_val;
  } else if (shift_count == 0) return *this;
  else return *this << (-shift_count);
}

BITSTRING BITSTRING::operator>>(const INTEGER& shift_count) const
{
  shift_count.must_bound("Unbound right operand of bitstring shift right "
    "operator.");
  return *this >> (int)shift_count;
}

BITSTRING BITSTRING::operator<<=(int rotate_count) const
{
  must_bound("Unbound bistring operand of rotate left operator.");
  int n_bits = val_ptr->n_bits;
  if (n_bits == 0) return *this;
  if (rotate_count >= 0) {
    rotate_count %= n_bits;
    if (rotate_count == 0) return *this;
    else return (*this << rotate_count) |
           (*this >> (n_bits - rotate_count));
  } else return *this >>= (-rotate_count);
}

BITSTRING BITSTRING::operator<<=(const INTEGER& rotate_count) const
{
  rotate_count.must_bound("Unbound right operand of bitstring rotate left "
    "operator.");
  return *this <<= (int)rotate_count;
}

BITSTRING BITSTRING::operator>>=(int rotate_count) const
{
  must_bound("Unbound bistring operand of rotate right operator.");
  int n_bits = val_ptr->n_bits;
  if (n_bits == 0) return *this;
  if (rotate_count >= 0) {
    rotate_count %= n_bits;
    if (rotate_count == 0) return *this;
    else return (*this >> rotate_count) |
           (*this << (n_bits - rotate_count));
  } else return *this <<= (-rotate_count);
}

BITSTRING BITSTRING::operator>>=(const INTEGER& rotate_count) const
{
  rotate_count.must_bound("Unbound right operand of bitstring rotate right "
    "operator.");
  return *this >>= (int)rotate_count;
}

BITSTRING_ELEMENT BITSTRING::operator[](int index_value)
{
  if (val_ptr == NULL && index_value == 0) {
    init_struct(1);
    clear_unused_bits();
    return BITSTRING_ELEMENT(FALSE, *this, 0);
  } else {
    must_bound("Accessing an element of an unbound bitstring value.");
    if (index_value < 0) TTCN_error("Accessing an bitstring element using "
      "a negative index (%d).", index_value);
    int n_bits = val_ptr->n_bits;
    if (index_value > n_bits) TTCN_error("Index overflow when accessing a "
      "bitstring element: The index is %d, but the string has only %d bits.",
      index_value, n_bits);
    if (index_value == n_bits) {
      if (val_ptr->ref_count == 1) {
	if (n_bits % 8 == 0) val_ptr = (bitstring_struct*)
	  Realloc(val_ptr, MEMORY_SIZE(n_bits + 1));
	val_ptr->n_bits++;
      } else {
	bitstring_struct *old_ptr = val_ptr;
	old_ptr->ref_count--;
	init_struct(n_bits + 1);
	memcpy(val_ptr->bits_ptr, old_ptr->bits_ptr, (n_bits + 7) / 8);
      }
      clear_unused_bits();
      return BITSTRING_ELEMENT(FALSE, *this, index_value);
    } else return BITSTRING_ELEMENT(TRUE, *this, index_value);
  }
}

BITSTRING_ELEMENT BITSTRING::operator[](const INTEGER& index_value)
{
  index_value.must_bound("Indexing a bitstring value with an unbound integer "
    "value.");
  return (*this)[(int)index_value];
}

const BITSTRING_ELEMENT BITSTRING::operator[](int index_value) const
{
  must_bound("Accessing an element of an unbound bitstring value.");
  if (index_value < 0) TTCN_error("Accessing an bitstring element using a "
    "negative index (%d).", index_value);
  if (index_value >= val_ptr->n_bits) TTCN_error("Index overflow when "
    "accessing a bitstring element: The index is %d, but the string has only "
    "%d bits.", index_value, val_ptr->n_bits);
  return BITSTRING_ELEMENT(TRUE, const_cast<BITSTRING&>(*this), index_value);
}

const BITSTRING_ELEMENT BITSTRING::operator[](const INTEGER& index_value) const
{
  index_value.must_bound("Indexing a bitstring value with an unbound integer "
    "value.");
  return (*this)[(int)index_value];
}

int BITSTRING::lengthof() const
{
  must_bound("Getting the length of an unbound bitstring value.");
  return val_ptr->n_bits;
}

BITSTRING::operator const unsigned char*() const
{
  must_bound("Casting an unbound bitstring value to const unsigned char*.");
  return val_ptr->bits_ptr;
}

void BITSTRING::log() const
{
  if (val_ptr != NULL) {
    TTCN_Logger::log_char('\'');
    for (int bit_count = 0; bit_count < val_ptr->n_bits; bit_count++)
      TTCN_Logger::log_char(get_bit(bit_count) ? '1' : '0');
    TTCN_Logger::log_event_str("'B");
  } else TTCN_Logger::log_event_unbound();
}

void BITSTRING::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE|Module_Param::BC_LIST, "bitstring value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Bitstring:
    switch (param.get_operation_type()) {
    case Module_Param::OT_ASSIGN:
      clean_up();
      init_struct(mp->get_string_size());
      memcpy(val_ptr->bits_ptr, mp->get_string_data(), (val_ptr->n_bits + 7) / 8);
      clear_unused_bits();
      break;
    case Module_Param::OT_CONCAT:
      if (is_bound()) {
        *this = *this + BITSTRING(mp->get_string_size(), (unsigned char*)mp->get_string_data());
      } else {
        *this = BITSTRING(mp->get_string_size(), (unsigned char*)mp->get_string_data());
      }
      break;
    default:
      TTCN_error("Internal error: BITSTRING::set_param()");
    }
    break;
  case Module_Param::MP_Expression:
    if (mp->get_expr_type() == Module_Param::EXPR_CONCATENATE) {
      BITSTRING operand1, operand2;
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
      param.expr_type_error("a bitstring");
    }
    break;
  default:
    param.type_error("bitstring value");
    break;
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* BITSTRING::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  int n_bytes = (val_ptr->n_bits + 7) / 8;
  unsigned char* val_cpy = (unsigned char *)Malloc(n_bytes);
  memcpy(val_cpy, val_ptr->bits_ptr, n_bytes);
  return new Module_Param_Bitstring(val_ptr->n_bits, val_cpy);
}
#endif

void BITSTRING::encode_text(Text_Buf& text_buf) const
{
  must_bound("Text encoder: Encoding an unbound bitstring value.");
  int n_bits = val_ptr->n_bits;
  text_buf.push_int(n_bits);
  if (n_bits > 0) text_buf.push_raw((n_bits + 7) / 8, val_ptr->bits_ptr);
}

void BITSTRING::decode_text(Text_Buf& text_buf)
{
  int n_bits = text_buf.pull_int().get_val();
  if (n_bits < 0)
    TTCN_error("Text decoder: Invalid length was received for a bitstring.");
  clean_up();
  init_struct(n_bits);
  if (n_bits > 0) {
    text_buf.pull_raw((n_bits + 7) / 8, val_ptr->bits_ptr);
    clear_unused_bits();
  }
}

void BITSTRING::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
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

void BITSTRING::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
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

void BITSTRING::BER_encode_putbits(unsigned char *target,
                                   unsigned int bitnum_start,
                                   unsigned int bit_count) const
{
  unsigned int nof_bits, nof_octets, i, j;
  unsigned char c;

  nof_bits=val_ptr->n_bits;
  if(bitnum_start>nof_bits
     || bitnum_start+bit_count>nof_bits)
    TTCN_EncDec_ErrorContext::error_internal
      ("In BITSTRING::BER_encode_putbits(): Index overflow.");
  nof_octets=(bit_count+7)/8;
  if(!nof_octets) {
    target[0]=0x00;
    return;
  }
  target[0] = static_cast<unsigned char>(nof_octets*8-bit_count);
  for(i=0; i<nof_octets-1; i++) {
    c=0;
    for(j=0; j<8; j++) {
      c<<=1;
      if(get_bit(bitnum_start+8*i+j)) c|=0x01;
    }
    target[1+i]=c;
  } // for
  c=0;
  for(j=0; j<8; j++) {
    c<<=1;
    if(8*i+j<bit_count)
      if(get_bit(bitnum_start+8*i+j)) c|=0x01;
  }
  target[1+i]=c;
}

ASN_BER_TLV_t*
BITSTRING::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                          unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(!new_tlv) {
    unsigned char *V_ptr;
    size_t V_len;
    unsigned int nof_bits=val_ptr->n_bits;
    unsigned int nof_octets=(nof_bits+7)/8;
    unsigned int nof_fragments=0;
    if(p_coding==BER_ENCODE_CER) {
      nof_fragments=(nof_octets+998)/999;
      if(!nof_fragments) nof_fragments=1;
    }
    else /*if(coding==BER_ENCODE_DER)*/ {
      nof_fragments=1;
    }

    boolean is_constructed=nof_fragments>1;
    if(!is_constructed) {
      V_len=nof_octets+1;
      V_ptr=(unsigned char*)Malloc(V_len);
      BER_encode_putbits(V_ptr, 0, nof_bits);
      new_tlv=ASN_BER_TLV_t::construct(V_len, V_ptr);
    }
    else { // is constructed
      ASN_BER_TLV_t *tmp_tlv=NULL;
      new_tlv=ASN_BER_TLV_t::construct(NULL);
      unsigned int rest_octets=nof_octets-(nof_fragments-1)*999;
      /*
      V_len=(nof_fragments-1)*1004;
      V_len+=rest_octets<128?2:4;
      V_len+=rest_octets+1;
      V_ptr=(unsigned char*)Malloc(V_len);
      */
      V_len=999;
      unsigned int nof_bits_curr=8*999;
      for(unsigned int i=0; i<nof_fragments; i++) {
        if(i==nof_fragments-1) {
          V_len=rest_octets;
          nof_bits_curr=nof_bits-i*8*999;
        }
        V_ptr=(unsigned char*)Malloc(V_len+1); // because of unused bits-octet
        /*
        V_ptr[0]=0x03;
        V_ptr[1]=0x82;
        V_ptr[2]=0x03;
        V_ptr[3]=0xE8;
        */
        BER_encode_putbits(V_ptr, i*8*999, nof_bits_curr);
        /*
        V_ptr+=1004;
        */
        tmp_tlv=ASN_BER_TLV_t::construct(V_len+1, V_ptr);
        tmp_tlv=ASN_BER_V2TLV(tmp_tlv, BITSTRING_descr_, p_coding);
        new_tlv->add_TLV(tmp_tlv);
      }
      /*
      V_ptr[0]=0x03;
      if(rest_octets<128) {
        V_ptr[1]=(rest_octets+1) & '\x7F';
        V_ptr+=2;
      }
      else {
        V_ptr[1]=0x82;
        V_ptr[2]=((rest_octets+1)/256) & 0xFF;
        V_ptr[3]=(rest_octets+1) & 0xFF;
        V_ptr+=4;
      }
      BER_encode_putbits(V_ptr, i*8*999, nof_bits-i*8*999);
      */
    }
  }
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

void BITSTRING::BER_decode_getbits(const unsigned char *source,
                                   size_t s_len, unsigned int& bitnum_start)
{
  unsigned int i, j;
  unsigned char c;
  if(s_len<1) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_INVAL_MSG, "Length of V-part of bitstring"
       " cannot be 0.");
    return;
  }
  unsigned int nof_octets=s_len-1;
  unsigned int nof_restbits=8-source[0];
  if(nof_octets==0) {
    if(nof_restbits!=8)
      TTCN_EncDec_ErrorContext::error
        (TTCN_EncDec::ET_INVAL_MSG,
         "If the bitstring is empty,"
         " the initial octet shall be 0, not %u [see X.690 clause 8.6.2.3].",
         source[0]);
    return;
  }
  if(source[0]>7) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_INVAL_MSG, "The number of unused bits in bitstring"
       " cannot be %u (should be less than 8) [see X.690 clause 8.6.2.2].",
       source[0]);
    nof_restbits=1;
  }
  // And what about overflow? :)
  i = (nof_octets - 1) * 8 + nof_restbits;
  if (i > 0) {
    if (val_ptr->ref_count > 1) {
      bitstring_struct *old_ptr = val_ptr;
      old_ptr->ref_count--;
      init_struct(bitnum_start + i);
      memcpy(val_ptr->bits_ptr, old_ptr->bits_ptr, (old_ptr->n_bits + 7) / 8);
    } else {
      if ((bitnum_start + i + 7) / 8 > ((unsigned int)val_ptr->n_bits + 7) / 8)
	val_ptr = (bitstring_struct*)Realloc(val_ptr,
	  MEMORY_SIZE(bitnum_start + i));
      val_ptr->n_bits = bitnum_start + i;
    }
  }
  for(i=0; i<nof_octets-1; i++) {
    c=source[1+i];
    for(j=0; j<8; j++) {
      set_bit(bitnum_start+8*i+j, c & 0x80?TRUE:FALSE);
      c<<=1;
    }
  }
  c=source[1+i];
  for(j=0; j<nof_restbits; j++) {
    set_bit(bitnum_start+8*i+j, c & 0x80?TRUE:FALSE);
    c<<=1;
  }
  bitnum_start+=(nof_octets-1)*8+nof_restbits;
}

void BITSTRING::BER_decode_TLV_(const ASN_BER_TLV_t& p_tlv, unsigned L_form,
                                unsigned int& bitnum_start)
{
  if(!p_tlv.isConstructed) {
    if (p_tlv.isComplete || p_tlv.V.str.Vlen > 0)
      BER_decode_getbits(p_tlv.V.str.Vstr, p_tlv.V.str.Vlen, bitnum_start);
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
           "Incomplete TLV in a constructed BITSTRING TLV.");
        return;
      }
      if(!p_tlv.isLenDefinite && tlv2.tagnumber==0
         && tlv2.tagclass==ASN_TAG_UNIV)
        doit=FALSE; // End-of-contents
      if(doit) {
        ASN_BER_TLV_t stripped_tlv;
        BER_decode_strip_tags(BITSTRING_ber_, tlv2, L_form, stripped_tlv);
	BER_decode_TLV_(tlv2, L_form, bitnum_start);
	V_pos+=tlv2.get_len();
	if(V_pos>=p_tlv.V.str.Vlen) doit=FALSE;
      }
    } // while(doit)
  } // else / is constructed
}

boolean BITSTRING::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                                  const ASN_BER_TLV_t& p_tlv,
                                  unsigned L_form)
{
  clean_up();
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec("While decoding BITSTRING type: ");
  init_struct(0);
  unsigned int bitnum_start = 0;
  BER_decode_TLV_(stripped_tlv, L_form, bitnum_start);
  return TRUE;
}

int BITSTRING::RAW_encode(const TTCN_Typedescriptor_t& p_td, RAW_enc_tree& myleaf) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
  }
  int bl = val_ptr->n_bits;
  int align_length = p_td.raw->fieldlength ? p_td.raw->fieldlength - bl : 0;
  if ((bl + align_length) < val_ptr->n_bits) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There is no sufficient bits to encode '%s': ", p_td.name);
    bl = p_td.raw->fieldlength;
    align_length = 0;
  }
//  myleaf.ext_bit=EXT_BIT_NO;
  if (myleaf.must_free) Free(myleaf.body.leaf.data_ptr);
  myleaf.must_free = FALSE;
  myleaf.data_ptr_used = TRUE;
  myleaf.body.leaf.data_ptr = val_ptr->bits_ptr;
  boolean orders = FALSE;
  if (p_td.raw->byteorder       == ORDER_MSB) orders = TRUE;
  if (p_td.raw->bitorderinfield == ORDER_LSB) orders = !orders;
  myleaf.coding_par.byteorder = orders ? ORDER_MSB : ORDER_LSB;
  orders = FALSE;
  if (p_td.raw->bitorderinoctet == ORDER_MSB) orders = TRUE;
  if (p_td.raw->bitorderinfield == ORDER_LSB) orders = !orders;
  myleaf.coding_par.bitorder = orders ? ORDER_MSB : ORDER_LSB;

  if (p_td.raw->endianness == ORDER_MSB) myleaf.align =  align_length;
  else                                   myleaf.align = -align_length;

  return myleaf.length = bl + align_length;
}

int BITSTRING::RAW_decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff,
  int limit, raw_order_t top_bit_ord, boolean no_err, int /*sel_field*/,
  boolean /*first_call*/)
{
  int prepaddlength = buff.increase_pos_padd(p_td.raw->prepadding);
  limit -= prepaddlength;
  int decode_length = p_td.raw->fieldlength == 0
    ? limit : p_td.raw->fieldlength;
  if ( p_td.raw->fieldlength > limit
    || p_td.raw->fieldlength > (int) buff.unread_len_bit()) {
    if (no_err) return -TTCN_EncDec::ET_LEN_ERR;
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There is not enough bits in the buffer to decode type %s.", p_td.name);
    decode_length = limit > (int) buff.unread_len_bit()
      ? buff.unread_len_bit() : limit;
  }
  clean_up();
  init_struct(decode_length);
  RAW_coding_par cp;
  boolean orders = FALSE;
  if (p_td.raw->bitorderinoctet == ORDER_MSB) orders = TRUE;
  if (p_td.raw->bitorderinfield == ORDER_LSB) orders = !orders;
  cp.bitorder = orders ? ORDER_MSB : ORDER_LSB;
  orders = FALSE;
  if (p_td.raw->byteorder == ORDER_MSB) orders = TRUE;
  if (p_td.raw->bitorderinfield == ORDER_LSB) orders = !orders;
  cp.byteorder = orders ? ORDER_MSB : ORDER_LSB;
  cp.fieldorder = p_td.raw->fieldorder;
  cp.hexorder = ORDER_LSB;
  buff.get_b((size_t) decode_length, val_ptr->bits_ptr, cp, top_bit_ord);
  if (p_td.raw->length_restrition != -1) {
    val_ptr->n_bits = p_td.raw->length_restrition;
    if (p_td.raw->endianness == ORDER_LSB) {
      if ((decode_length - val_ptr->n_bits) % 8) {
        int bound = (decode_length - val_ptr->n_bits) % 8;
        int maxindex = (decode_length - 1) / 8;
        for (int a = 0, b = (decode_length - val_ptr->n_bits - 1) / 8;
          a < (val_ptr->n_bits + 7) / 8; a++, b++) {
          val_ptr->bits_ptr[a] = val_ptr->bits_ptr[b] >> bound;
          if (b < maxindex) {
            val_ptr->bits_ptr[a] = val_ptr->bits_ptr[b + 1] << (8 - bound);
          }
        }
      }
      else memmove(val_ptr->bits_ptr,
        val_ptr->bits_ptr + (decode_length - val_ptr->n_bits) / 8,
        val_ptr->n_bits / 8 * sizeof(unsigned char));
    }
  }
  decode_length += buff.increase_pos_padd(p_td.raw->padding);
  clear_unused_bits();
  return decode_length + prepaddlength;
}

int BITSTRING::XER_encode(const XERdescriptor_t& p_td,
  TTCN_Buffer& p_buf, unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound bitstring value.");
  }

  int encoded_length=(int)p_buf.get_len();
  int empty_element = val_ptr==NULL || val_ptr->n_bits == 0;
  flavor |= SIMPLE_TYPE;
  flavor &= ~XER_RECOF; // bitstring doesn't care

  begin_xml(p_td, p_buf, flavor, indent, empty_element);

  if (!empty_element) {
    for (int bit_count = 0; bit_count < val_ptr->n_bits; bit_count++)
      p_buf.put_c(get_bit(bit_count) ? '1' : '0');
  }

  end_xml(p_td, p_buf, flavor, indent, empty_element);
  return (int)p_buf.get_len() - encoded_length;
}

int BITSTRING::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
                          unsigned int flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*)
{
  boolean exer  = is_exer(flavor);
  int success = reader.Ok(), depth = -1, type;
  boolean own_tag = !is_exerlist(flavor) && !(exer && (p_td.xer_bits & UNTAGGED));

  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
    const char * name = verify_name(reader, p_td, exer);
    (void)name;
  }
  else if (own_tag) {
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
        if (reader.IsEmptyElement()) {
          init_struct(0);
          reader.Read();
          return 1;
        }
      }
      else if (XML_READER_TYPE_TEXT == type && depth != -1) break;
      else if (XML_READER_TYPE_END_ELEMENT == type) {
        // End tag without intervening #text == empty content
        verify_end(reader, p_td, depth, exer);
        init_struct(0);
        reader.Read();
        return 1;
      }
    }
  }

  type = reader.NodeType();
  if (success == 1 && (XML_READER_TYPE_TEXT == type || XML_READER_TYPE_ATTRIBUTE == type)) {
    const char* value = (const char *)reader.Value();
    size_t num_bits = strlen(value);
    init_struct(num_bits);
    for (size_t i = 0; i < num_bits; ++i) {
      if (value[i] < '0' || value[i] > '1') {
        if (exer && (flavor & EXIT_ON_ERROR)) {
          clean_up();
          return -1;
        } else {
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
            "The bitstring value may only contain ones and zeros.");
        }
      }
      set_bit(i, value[i] - '0');
    }
  }
  
  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
    // Let the caller do reader.AdvanceAttribute();
  }
  else if (own_tag) {
    for (success = reader.Read(); success == 1; success = reader.Read()) {
      type = reader.NodeType();
      if (XML_READER_TYPE_END_ELEMENT == type) {
        verify_end(reader, p_td, depth, exer);
        reader.Read(); // one last time
        break;
      }
    }
  }
  return 1;
}

int BITSTRING::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound bitstring value.");
    return -1;
  }
  
  char* tmp_str = (char*)Malloc(val_ptr->n_bits + 3);
  tmp_str[0] = '\"';
  tmp_str[val_ptr->n_bits + 1] = '\"';
  for (int i = 0; i < val_ptr->n_bits; ++i) {
    tmp_str[i + 1] = get_bit(i) ? '1' : '0';
  }
  tmp_str[val_ptr->n_bits + 2] = 0;
  int enc_len = p_tok.put_next_token(JSON_TOKEN_STRING, tmp_str);
  Free(tmp_str);
  return enc_len;
}

int BITSTRING::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
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
      // White spaces are ignored, so the resulting bitstring might be shorter
      // than the extracted JSON string
      int bits = value_len;
      for (size_t i = 0; i < value_len; ++i) {
        if (value[i] == ' ') {
          --bits;
        }
        else if (value[i] != '0' && value[i] != '1') {
          if (value[i] == '\\' && i + 1 < value_len &&
              (value[i + 1] == 'n' || value[i + 1] == 'r' || value[i + 1] == 't')) {
            // Escaped white space character
            ++i;
            bits -= 2;
          }
          else {
            error = TRUE;
            break;
          }
        }
      }
      if (!error) {
        init_struct(bits);
        int bit_index = 0;
        for (size_t i = 0; i < value_len; ++i) {
          if (value[i] == '0' || value[i] == '1') {
            set_bit(bit_index, value[i] - '0');
            ++bit_index;
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
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FORMAT_ERROR, "string", "bitstring");
    return JSON_ERROR_FATAL;    
  }
  return (int)dec_len;
}


// bitstring element class

BITSTRING_ELEMENT::BITSTRING_ELEMENT(boolean par_bound_flag,
  BITSTRING& par_str_val, int par_bit_pos)
  : bound_flag(par_bound_flag), str_val(par_str_val), bit_pos(par_bit_pos)
{
}

BITSTRING_ELEMENT& BITSTRING_ELEMENT::operator=(const BITSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound bitstring value.");
  if(other_value.val_ptr->n_bits != 1)
    TTCN_error("Assignment of a bitstring "
               "value with length other than 1 to a bitstring element.");
  bound_flag = TRUE;
  str_val.copy_value();
  str_val.set_bit(bit_pos, other_value.get_bit(0));
  return *this;
}

BITSTRING_ELEMENT& BITSTRING_ELEMENT::operator=
(const BITSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound bitstring element.");
  bound_flag = TRUE;
  str_val.copy_value();
  str_val.set_bit(bit_pos, other_value.str_val.get_bit(other_value.bit_pos));
  return *this;
}

boolean BITSTRING_ELEMENT::operator==(const BITSTRING& other_value) const
{
  must_bound("Unbound left operand of bitstring element comparison.");
  other_value.must_bound("Unbound right operand of bitstring comparison.");
  if(other_value.val_ptr->n_bits != 1) return FALSE;
  return str_val.get_bit(bit_pos) == other_value.get_bit(0);
}

boolean BITSTRING_ELEMENT::operator==
(const BITSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of bitstring element comparison.");
  other_value.must_bound("Unbound right operand of bitstring element "
    "comparison.");
  return str_val.get_bit(bit_pos) ==
    other_value.str_val.get_bit(other_value.bit_pos);
}

BITSTRING BITSTRING_ELEMENT::operator+(const BITSTRING& other_value) const
{
  must_bound("Unbound left operand of bitstring element concatenation.");
  other_value.must_bound("Unbound right operand of bitstring concatenation.");
  int n_bits = other_value.val_ptr->n_bits;
  BITSTRING ret_val(n_bits + 1);
  ret_val.val_ptr->bits_ptr[0] = str_val.get_bit(bit_pos) ? 1 : 0;
  int n_bytes = (n_bits + 7) / 8;
  for (int byte_count = 0; byte_count < n_bytes; byte_count++) {
    ret_val.val_ptr->bits_ptr[byte_count] |=
      other_value.val_ptr->bits_ptr[byte_count] << 1;
    if (n_bits > byte_count * 8 + 7)
      ret_val.val_ptr->bits_ptr[byte_count + 1] =
	other_value.val_ptr->bits_ptr[byte_count] >> 7;
  }
  ret_val.clear_unused_bits();
  return ret_val;
}

BITSTRING BITSTRING_ELEMENT::operator+(const BITSTRING_ELEMENT& other_value)
  const
{
  must_bound("Unbound left operand of bitstring element concatenation.");
  other_value.must_bound("Unbound right operand of bitstring element "
                         "concatenation.");
  unsigned char result = str_val.get_bit(bit_pos) ? 1 : 0;
  if (other_value.str_val.get_bit(other_value.bit_pos)) result |= 2;
  return BITSTRING(2, &result);
}

#ifdef TITAN_RUNTIME_2
BITSTRING BITSTRING_ELEMENT::operator+(
  const OPTIONAL<BITSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const BITSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of bitstring concatenation.");
}
#endif

BITSTRING BITSTRING_ELEMENT::operator~() const
{
  must_bound("Unbound bitstring element operand of operator not4b.");
  unsigned char result = str_val.get_bit(bit_pos) ? 0 : 1;
  return BITSTRING(1, &result);
}

BITSTRING BITSTRING_ELEMENT::operator&(const BITSTRING& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound bitstring element.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "bitstring value.");
  if (other_value.val_ptr->n_bits != 1)
    TTCN_error("The bitstring operands "
               "of operator and4b must have the same length.");
  unsigned char result = str_val.get_bit(bit_pos) && other_value.get_bit(0) ?
    1 : 0;
  return BITSTRING(1, &result);
}

BITSTRING BITSTRING_ELEMENT::operator&
(const BITSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound bitstring element.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "bitstring element.");
  unsigned char result = str_val.get_bit(bit_pos) &&
    other_value.str_val.get_bit(other_value.bit_pos) ? 1 : 0;
  return BITSTRING(1, &result);
}

BITSTRING BITSTRING_ELEMENT::operator|(const BITSTRING& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound bitstring element.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "bitstring value.");
  if (other_value.val_ptr->n_bits != 1)
    TTCN_error("The bitstring operands "
               "of operator or4b must have the same length.");
  unsigned char result = str_val.get_bit(bit_pos) || other_value.get_bit(0) ?
    1 : 0;
  return BITSTRING(1, &result);
}

BITSTRING BITSTRING_ELEMENT::operator|
(const BITSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound bitstring element.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "bitstring element.");
  unsigned char result = str_val.get_bit(bit_pos) ||
    other_value.str_val.get_bit(other_value.bit_pos) ? 1 : 0;
  return BITSTRING(1, &result);
}

BITSTRING BITSTRING_ELEMENT::operator^(const BITSTRING& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound bitstring element.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "bitstring value.");
  if (other_value.val_ptr->n_bits != 1)
    TTCN_error("The bitstring operands "
               "of operator xor4b must have the same length.");
  unsigned char result = str_val.get_bit(bit_pos) != other_value.get_bit(0) ?
    1 : 0;
  return BITSTRING(1, &result);
}

BITSTRING BITSTRING_ELEMENT::operator^
(const BITSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound bitstring element.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "bitstring element.");
  unsigned char result = str_val.get_bit(bit_pos) !=
    other_value.str_val.get_bit(other_value.bit_pos) ? 1 : 0;
  return BITSTRING(1, &result);
}

boolean BITSTRING_ELEMENT::get_bit() const
{
  return str_val.get_bit(bit_pos);
}

void BITSTRING_ELEMENT::log() const
{
  if (bound_flag) TTCN_Logger::log_event("'%c'B", str_val.get_bit(bit_pos) ?
    '1' : '0');
  else TTCN_Logger::log_event_unbound();
}

// bitstring template class

void BITSTRING_template::clean_up()
{
  switch (template_selection) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  case STRING_PATTERN:
    if (pattern_value->ref_count > 1) pattern_value->ref_count--;
    else if (pattern_value->ref_count == 1) Free(pattern_value);
    else TTCN_error("Internal error: Invalid reference counter in a bitstring "
      "pattern.");
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

void BITSTRING_template::copy_template(const BITSTRING_template& other_value)
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
    value_list.list_value = new BITSTRING_template[value_list.n_values];
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
    TTCN_error("Copying an uninitialized/unsupported bitstring template.");
  }
  set_selection(other_value);
}

/*
  This is the same algorithm that match_array uses
    to match 'record of' types.
  The only differences are: how two elements are matched and
    how an asterisk or ? is identified in the template
*/
boolean BITSTRING_template::match_pattern(
  const bitstring_pattern_struct *string_pattern,
  const BITSTRING::bitstring_struct *string_value)
{
  if(string_pattern->n_elements == 0) return string_value->n_bits == 0;

  int value_index = 0;
  unsigned int template_index = 0;
  int last_asterisk = -1;
  int last_value_to_asterisk = -1;

  for(;;)
  {
    switch(string_pattern->elements_ptr[template_index]) {
      case 0:
        if (!(string_value->bits_ptr[value_index / 8] &
          (1 << (value_index % 8))))
        {
          value_index++;
          template_index++;
        }else{
          if(last_asterisk == -1) return FALSE;
          template_index = last_asterisk +1;
          value_index = ++last_value_to_asterisk;
        }
        break;
      case 1:
        if (string_value->bits_ptr[value_index / 8] & (1 << (value_index % 8)))
        {
          value_index++;
          template_index++;
        }else {
          if(last_asterisk == -1) return FALSE;
          template_index = last_asterisk +1;
          value_index = ++last_value_to_asterisk;
        }
        break;
      case 2:
        //we found a ? element, it matches anything
        value_index++;
        template_index++;
        break;
      case 3:
        //we found an asterisk
        last_asterisk = template_index++;
        last_value_to_asterisk = value_index;
        break;
      default:
        TTCN_error("Internal error: invalid element in bitstring pattern.");
    }

    if(value_index == string_value->n_bits
      && template_index == string_pattern->n_elements)
    {
      return TRUE;
    }else if(template_index == string_pattern->n_elements)
    {
      if(string_pattern->elements_ptr[template_index-1] == 3)
      {
        return TRUE;
      } else if (last_asterisk == -1){
        return FALSE;
      } else{
        template_index = last_asterisk+1;
        value_index = ++last_value_to_asterisk;
      }
    } else if(value_index == string_value->n_bits)
    {
      while(template_index < string_pattern->n_elements &&
            string_pattern->elements_ptr[template_index] == 3)
        template_index++;

      return template_index == string_pattern->n_elements;
    }
  }
}

BITSTRING_template::BITSTRING_template()
{
}

BITSTRING_template::BITSTRING_template(template_sel other_value)
  : Restricted_Length_Template(other_value)
{
  check_single_selection(other_value);
}

BITSTRING_template::BITSTRING_template(const BITSTRING& other_value)
  : Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

BITSTRING_template::BITSTRING_template(const BITSTRING_ELEMENT& other_value)
  : Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

BITSTRING_template::~BITSTRING_template()
{
  clean_up();
}

BITSTRING_template::BITSTRING_template(const OPTIONAL<BITSTRING>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const BITSTRING&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a bitstring template from an unbound optional field.");
  }
}

BITSTRING_template::BITSTRING_template(unsigned int n_elements,
  const unsigned char *pattern_elements)
  : Restricted_Length_Template(STRING_PATTERN)
{
  pattern_value = (bitstring_pattern_struct*)
    Malloc(sizeof(bitstring_pattern_struct) + n_elements - 1);
  pattern_value->ref_count = 1;
  pattern_value->n_elements = n_elements;
  memcpy(pattern_value->elements_ptr, pattern_elements, n_elements);
}

BITSTRING_template::BITSTRING_template(const BITSTRING_template& other_value)
: Restricted_Length_Template()
{
  copy_template(other_value);
}

BITSTRING_template& BITSTRING_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

BITSTRING_template& BITSTRING_template::operator=(const BITSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound bitstring value to a "
                         "template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

BITSTRING_template& BITSTRING_template::operator=
  (const BITSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound bitstring element to a "
                         "template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

BITSTRING_template& BITSTRING_template::operator=
  (const OPTIONAL<BITSTRING>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const BITSTRING&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a bitstring "
      "template.");
  }
  return *this;
}

BITSTRING_template& BITSTRING_template::operator=
(const BITSTRING_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

#ifdef TITAN_RUNTIME_2
void BITSTRING_template::concat(Vector<unsigned char>& v) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    switch (length_restriction_type) {
    case NO_LENGTH_RESTRICTION:
      if (template_selection == ANY_VALUE) {
        // ? => '*'
        if (v.size() == 0 || v[v.size() - 1] != 3) {
          // '**' == '*', so just ignore the second '*'
          v.push_back(3);
        }
      }
      else {
        TTCN_error("Operand of bitstring template concatenation is an "
          "AnyValueOrNone (*) matching mechanism with no length restriction");
      }
      break;
    case RANGE_LENGTH_RESTRICTION:
      if (!length_restriction.range_length.max_length ||
          length_restriction.range_length.max_length != length_restriction.range_length.min_length) {
        TTCN_error("Operand of bitstring template concatenation is an %s "
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
        v.push_back(2);
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
    TTCN_error("Operand of bitstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
}

void BITSTRING_template::concat(Vector<unsigned char>& v, const BITSTRING& val)
{
  if (!val.is_bound()) {
    TTCN_error("Operand of bitstring template concatenation is an "
      "unbound value.");
  }
  for (int i = 0; i < val.val_ptr->n_bits; ++i) {
    v.push_back(val.get_bit(i));
  }
}

void BITSTRING_template::concat(Vector<unsigned char>& v, template_sel sel)
{
  if (sel == ANY_VALUE) {
    // ? => '*'
    if (v.size() == 0 || v[v.size() - 1] != 3) {
      // '**' == '*', so just ignore the second '*'
      v.push_back(3);
    }
  }
  else {
    TTCN_error("Operand of bitstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
}


BITSTRING_template BITSTRING_template::operator+(
  const BITSTRING_template& other_value) const
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
    return BITSTRING_template(ANY_VALUE);
  }
  // otherwise the result is an bitstring pattern
  Vector<unsigned char> v;
  concat(v);
  other_value.concat(v);
  return BITSTRING_template(v.size(), v.data_ptr());
}

BITSTRING_template BITSTRING_template::operator+(
  const BITSTRING& other_value) const
{
  if (template_selection == SPECIFIC_VALUE) {
    // result is a specific value template
    return single_value + other_value;
  }
  // otherwise the result is an bitstring pattern
  Vector<unsigned char> v;
  concat(v);
  concat(v, other_value);
  return BITSTRING_template(v.size(), v.data_ptr());
}

BITSTRING_template BITSTRING_template::operator+(
  const BITSTRING_ELEMENT& other_value) const
{
  return *this + BITSTRING(other_value);
}

BITSTRING_template BITSTRING_template::operator+(
  const OPTIONAL<BITSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const BITSTRING&)other_value;
  }
  TTCN_error("Operand of bitstring template concatenation is an "
    "unbound or omitted record/set field.");
}

BITSTRING_template BITSTRING_template::operator+(
  template_sel other_template_sel) const
{
  if (template_selection == ANY_VALUE && other_template_sel == ANY_VALUE &&
      length_restriction_type == NO_LENGTH_RESTRICTION) {
    // special case: ? & ? => ?
    return BITSTRING_template(ANY_VALUE);
  }
  // the result is always an bitstring pattern
  Vector<unsigned char> v;
  concat(v);
  concat(v, other_template_sel);
  return BITSTRING_template(v.size(), v.data_ptr());
}

BITSTRING_template operator+(const BITSTRING& left_value,
  const BITSTRING_template& right_template)
{
  if (right_template.template_selection == SPECIFIC_VALUE) {
    // result is a specific value template
    return left_value + right_template.single_value;
  }
  // otherwise the result is an bitstring pattern
  Vector<unsigned char> v;
  BITSTRING_template::concat(v, left_value);
  right_template.concat(v);
  return BITSTRING_template(v.size(), v.data_ptr());
}

BITSTRING_template operator+(const BITSTRING_ELEMENT& left_value,
  const BITSTRING_template& right_template)
{
  return BITSTRING(left_value) + right_template;
}

BITSTRING_template operator+(const OPTIONAL<BITSTRING>& left_value,
  const BITSTRING_template& right_template)
{
  if (left_value.is_present()) {
    return (const BITSTRING&)left_value + right_template;
  }
  TTCN_error("Operand of bitstring template concatenation is an "
    "unbound or omitted record/set field.");
}

BITSTRING_template operator+(template_sel left_template_sel,
  const BITSTRING_template& right_template)
{
  if (left_template_sel == ANY_VALUE &&
      right_template.template_selection == ANY_VALUE &&
      right_template.length_restriction_type ==
      Restricted_Length_Template::NO_LENGTH_RESTRICTION) {
    // special case: ? & ? => ?
    return BITSTRING_template(ANY_VALUE);
  }
  // the result is always an bitstring pattern
  Vector<unsigned char> v;
  BITSTRING_template::concat(v, left_template_sel);
  right_template.concat(v);
  return BITSTRING_template(v.size(), v.data_ptr());
}

BITSTRING_template operator+(const BITSTRING& left_value,
  template_sel right_template_sel)
{
  // the result is always an bitstring pattern
  Vector<unsigned char> v;
  BITSTRING_template::concat(v, left_value);
  BITSTRING_template::concat(v, right_template_sel);
  return BITSTRING_template(v.size(), v.data_ptr());
}

BITSTRING_template operator+(const BITSTRING_ELEMENT& left_value,
  template_sel right_template_sel)
{
  return BITSTRING(left_value) + right_template_sel;
}

BITSTRING_template operator+(const OPTIONAL<BITSTRING>& left_value,
  template_sel right_template_sel)
{
  if (left_value.is_present()) {
    return (const BITSTRING&)left_value + right_template_sel;
  }
  TTCN_error("Operand of bitstring template concatenation is an "
    "unbound or omitted record/set field.");
}

BITSTRING_template operator+(template_sel left_template_sel,
  const BITSTRING& right_value)
{
  // the result is always an bitstring pattern
  Vector<unsigned char> v;
  BITSTRING_template::concat(v, left_template_sel);
  BITSTRING_template::concat(v, right_value);
  return BITSTRING_template(v.size(), v.data_ptr());
}

BITSTRING_template operator+(template_sel left_template_sel,
  const BITSTRING_ELEMENT& right_value)
{
  return left_template_sel + BITSTRING(right_value);
}

BITSTRING_template operator+(template_sel left_template_sel,
  const OPTIONAL<BITSTRING>& right_value)
{
  if (right_value.is_present()) {
    return left_template_sel + (const BITSTRING&)right_value;
  }
  TTCN_error("Operand of bitstring template concatenation is an "
    "unbound or omitted record/set field.");
}
#endif // TITAN_RUNTIME_2

BITSTRING_ELEMENT BITSTRING_template::operator[](int index_value)
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Accessing a bitstring element of a non-specific bitstring "
               "template.");
  return single_value[index_value];
}

BITSTRING_ELEMENT BITSTRING_template::operator[](const INTEGER& index_value)
{
  index_value.must_bound("Indexing a bitstring template with an unbound "
                         "integer value.");
  return (*this)[(int)index_value];
}

const BITSTRING_ELEMENT BITSTRING_template::operator[](int index_value) const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Accessing a bitstring element of a non-specific bitstring "
               "template.");
  return single_value[index_value];
}

const BITSTRING_ELEMENT BITSTRING_template::operator[](const INTEGER& index_value) const
{
  index_value.must_bound("Indexing a bitstring template with an unbound "
                         "integer value.");
  return (*this)[(int)index_value];
}

boolean BITSTRING_template::match(const BITSTRING& other_value,
                                  boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  if (!match_length(other_value.val_ptr->n_bits)) return FALSE;
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
  case STRING_PATTERN:
    return match_pattern(pattern_value, other_value.val_ptr);
  case DECODE_MATCH: {
    TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);
    TTCN_EncDec::clear_error();
    OCTETSTRING os(bit2oct(other_value));
    TTCN_Buffer buff(os);
    boolean ret_val = dec_match->instance->match(buff);
    TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL,TTCN_EncDec::EB_DEFAULT);
    TTCN_EncDec::clear_error();
    return ret_val; }
  default:
    TTCN_error("Matching an uninitialized/unsupported bitstring template.");
  }
  return FALSE;
}

const BITSTRING& BITSTRING_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific "
      "bitstring template.");
  return single_value;
}

int BITSTRING_template::lengthof() const
{
  int min_length;
  boolean has_any_or_none;
  if (is_ifpresent)
    TTCN_error("Performing lengthof() operation on a bitstring template "
               "which has an ifpresent attribute.");
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
    min_length = single_value.lengthof();
    has_any_or_none = FALSE;
    break;
  case OMIT_VALUE:
    TTCN_error("Performing lengthof() operation on a bitstring template "
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
                 "Performing lengthof() operation on a bitstring template "
                 "containing an empty list.");
    int item_length = value_list.list_value[0].lengthof();
    for (unsigned int i = 1; i < value_list.n_values; i++) {
      if (value_list.list_value[i].lengthof()!=item_length)
        TTCN_error("Performing lengthof() operation on a bitstring template "
                   "containing a value list with different lengths.");
    }
    min_length = item_length;
    has_any_or_none = FALSE;
    break;
  }
  case COMPLEMENTED_LIST:
    TTCN_error("Performing lengthof() operation on a bitstring template "
               "containing complemented list.");
  case STRING_PATTERN:
    min_length = 0;
    has_any_or_none = FALSE; // TRUE if * chars in the pattern
    for (unsigned int i = 0; i < pattern_value->n_elements; i++)
    {
      if (pattern_value->elements_ptr[i] < 3) min_length++; // case of 1, 0, ?
      else has_any_or_none = TRUE;   // case of * character
    }
    break;
  default:
    TTCN_error("Performing lengthof() operation on an "
               "uninitialized/unsupported bitstring template.");
  }
  return check_section_is_single(min_length, has_any_or_none,
                                 "length", "a", "bitstring template");
}

void BITSTRING_template::set_type(template_sel template_type,
  unsigned int list_length /* = 0 */)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST &&
      template_type != DECODE_MATCH)
    TTCN_error("Setting an invalid list type for a bitstring template.");
  clean_up();
  set_selection(template_type);
  if (template_type != DECODE_MATCH) {
    value_list.n_values = list_length;
    value_list.list_value = new BITSTRING_template[list_length];
  }
}

BITSTRING_template& BITSTRING_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
      template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list bitstring template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a bitstring value list template.");
  return value_list.list_value[list_index];
}

void BITSTRING_template::set_decmatch(Dec_Match_Interface* new_instance)
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Setting the decoded content matching mechanism of a non-decmatch "
      "bitstring template.");
  }
  dec_match = new decmatch_struct;
  dec_match->ref_count = 1;
  dec_match->instance = new_instance;
}

void* BITSTRING_template::get_decmatch_dec_res() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the decoding result of a non-decmatch bitstring "
      "template.");
  }
  return dec_match->instance->get_dec_res();
}

const TTCN_Typedescriptor_t* BITSTRING_template::get_decmatch_type_descr() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the decoded type's descriptor in a non-decmatch "
      "bitstring template.");
  }
  return dec_match->instance->get_type_descr();
}

static const char patterns[] = { '0', '1', '?', '*' };

void BITSTRING_template::log() const
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
    for (unsigned int i = 0; i < value_list.n_values; i++) {
      if (i > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[i].log();
    }
    TTCN_Logger::log_char(')');
    break;
  case STRING_PATTERN:
    TTCN_Logger::log_char('\'');
    for (unsigned int i = 0; i < pattern_value->n_elements; i++) {
      unsigned char pattern = pattern_value->elements_ptr[i];
      if (pattern < 4) TTCN_Logger::log_char(patterns[pattern]);
      else TTCN_Logger::log_event_str("<unknown>");
    }
    TTCN_Logger::log_event_str("'B");
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

void BITSTRING_template::log_match(const BITSTRING& match_value,
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

void BITSTRING_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE|Module_Param::BC_LIST, "bitstring template");
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
    BITSTRING_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Bitstring:
    *this = BITSTRING(mp->get_string_size(), (unsigned char*)mp->get_string_data());
    break;
  case Module_Param::MP_Bitstring_Template:
    *this = BITSTRING_template(mp->get_string_size(), (unsigned char*)mp->get_string_data());
    break;
  case Module_Param::MP_Expression:
    if (mp->get_expr_type() == Module_Param::EXPR_CONCATENATE) {
      BITSTRING operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 + operand2;
    }
    else {
      param.expr_type_error("a bitstring");
    }
    break;
  default:
    param.type_error("bitstring template");
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
Module_Param* BITSTRING_template::get_param(Module_Param_Name& param_name) const
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
    unsigned char* val_cpy = (unsigned char*)Malloc(pattern_value->n_elements);
    memcpy(val_cpy, pattern_value->elements_ptr, pattern_value->n_elements);
    mp = new Module_Param_Bitstring_Template(pattern_value->n_elements, val_cpy);
    break; }
  case DECODE_MATCH:
    TTCN_error("Referencing a decoded content matching template is not supported.");
    break;
  default:
    TTCN_error("Referencing an uninitialized/unsupported bitstring template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  mp->set_length_restriction(get_length_range());
  return mp;
}
#endif

void BITSTRING_template::encode_text(Text_Buf& text_buf) const
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
    text_buf.push_raw(pattern_value->n_elements, pattern_value->elements_ptr);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported "
               "bitstring template.");
  }
}

void BITSTRING_template::decode_text(Text_Buf& text_buf)
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
    value_list.list_value = new BITSTRING_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  case STRING_PATTERN: {
    unsigned int n_elements = text_buf.pull_int().get_val();
    pattern_value = (bitstring_pattern_struct*)
      Malloc(sizeof(bitstring_pattern_struct) + n_elements - 1);
    pattern_value->ref_count = 1;
    pattern_value->n_elements = n_elements;
    text_buf.pull_raw(n_elements, pattern_value->elements_ptr);
    break;}
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was "
               "received for a bitstring template.");
  }
}

boolean BITSTRING_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean BITSTRING_template::match_omit(boolean legacy /* = FALSE */) const
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
void BITSTRING_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "bitstring");
}
#else
int BITSTRING::RAW_encode_negtest_raw(RAW_enc_tree& p_myleaf) const
{
  if (p_myleaf.must_free)
    Free(p_myleaf.body.leaf.data_ptr);
  p_myleaf.must_free = FALSE;
  p_myleaf.data_ptr_used = TRUE;
  p_myleaf.body.leaf.data_ptr = val_ptr->bits_ptr;
  return p_myleaf.length = val_ptr->n_bits;
}
#endif
