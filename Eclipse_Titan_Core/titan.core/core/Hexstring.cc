/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
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
#include "Hexstring.hh"
#include "../common/memory.h"
#include "Integer.hh"
#include "String_struct.hh"
#include "Param_Types.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Encdec.hh"
#include "RAW.hh"
#include "Addfunc.hh"
#include "Optional.hh"

#include "../common/dbgnew.hh"

#include <string.h>
#include <ctype.h>

// hexstring value class

/** The amount of memory needed for a string containing n hexadecimal digits. */
#define MEMORY_SIZE(n) (sizeof(hexstring_struct) - sizeof(int) + ((n) + 1) / 2)

void HEXSTRING::init_struct(int n_nibbles)
{
  if (n_nibbles < 0) {
    val_ptr = NULL;
    TTCN_error("Initializing an hexstring with a negative length.");
  }
  else if (n_nibbles == 0) {
    /** This will represent the empty strings so they won't need allocated
     * memory, this delays the memory allocation until it is really needed.
     */
    static hexstring_struct empty_string = { 1, 0, "" };
    val_ptr = &empty_string;
    empty_string.ref_count++;
  }
  else {
    val_ptr = (hexstring_struct*) Malloc(MEMORY_SIZE(n_nibbles));
    val_ptr->ref_count = 1;
    val_ptr->n_nibbles = n_nibbles;
  }
}

/** Return the nibble at index i
 *
 * @param nibble_index
 * @return
 */
unsigned char HEXSTRING::get_nibble(int nibble_index) const
{
  unsigned char octet = val_ptr->nibbles_ptr[nibble_index / 2];
  if (nibble_index % 2)
    return octet >> 4;   // odd  nibble -> top
  else
    return octet & 0x0F; // even nibble -> bottom
}

void HEXSTRING::set_nibble(int nibble_index, unsigned char new_value)
{
  unsigned char old_octet = val_ptr->nibbles_ptr[nibble_index / 2];
  if (nibble_index % 2) {
    val_ptr->nibbles_ptr[nibble_index / 2] = (old_octet & 0x0F) | (new_value
      << 4);
  }
  else {
    val_ptr->nibbles_ptr[nibble_index / 2] = (old_octet & 0xF0) | (new_value
      & 0x0F);
  }
}

void HEXSTRING::copy_value()
{
  if (val_ptr == NULL || val_ptr->n_nibbles <= 0) TTCN_error(
    "Internal error: Invalid internal data structure when copying "
      "the memory area of a hexstring value.");
  if (val_ptr->ref_count > 1) {
    hexstring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_nibbles);
    memcpy(val_ptr->nibbles_ptr, old_ptr->nibbles_ptr, (old_ptr->n_nibbles + 1)
      / 2);
  }
}

void HEXSTRING::clear_unused_nibble() const
{
  if (val_ptr->n_nibbles % 2) val_ptr->nibbles_ptr[val_ptr->n_nibbles / 2]
    &= 0x0F;
}

HEXSTRING::HEXSTRING(int n_nibbles)
{
  init_struct(n_nibbles);
}

HEXSTRING::HEXSTRING()
{
  val_ptr = NULL;
}

HEXSTRING::HEXSTRING(int init_n_nibbles, const unsigned char* init_nibbles)
{
  init_struct(init_n_nibbles);
  memcpy(val_ptr->nibbles_ptr, init_nibbles, (init_n_nibbles + 1) / 2);
  clear_unused_nibble();
}

HEXSTRING::HEXSTRING(const HEXSTRING& other_value) :
  Base_Type(other_value)
{
  other_value.must_bound("Initialization from an unbound hexstring value.");
  val_ptr = other_value.val_ptr;
  val_ptr->ref_count++;
}

HEXSTRING::HEXSTRING(const HEXSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Initialization from an unbound hexstring element.");
  init_struct(1);
  val_ptr->nibbles_ptr[0] = other_value.get_nibble();
}

HEXSTRING::~HEXSTRING()
{
  clean_up();
}

void HEXSTRING::clean_up()
{
  if (val_ptr != NULL) {
    if (val_ptr->ref_count > 1)
      val_ptr->ref_count--;
    else if (val_ptr->ref_count == 1)
      Free(val_ptr);
    else
      TTCN_error("Internal error: Invalid reference counter in a hexstring "
        "value.");
    val_ptr = NULL;
  }
}

HEXSTRING& HEXSTRING::operator=(const HEXSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound hexstring value.");
  if (&other_value != this) {
    clean_up();
    val_ptr = other_value.val_ptr;
    val_ptr->ref_count++;
  }
  return *this;
}

HEXSTRING& HEXSTRING::operator=(const HEXSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound hexstring element to a "
    "hexstring.");
  unsigned char nibble_value = other_value.get_nibble();
  clean_up();
  init_struct(1);
  val_ptr->nibbles_ptr[0] = nibble_value;
  return *this;
}

boolean HEXSTRING::operator==(const HEXSTRING& other_value) const
{
  must_bound("Unbound left operand of hexstring comparison.");
  other_value.must_bound("Unbound right operand of hexstring comparison.");
  if (val_ptr->n_nibbles != other_value.val_ptr->n_nibbles) return FALSE;
  if (val_ptr->n_nibbles == 0) return TRUE;
  clear_unused_nibble();
  other_value.clear_unused_nibble();
  return !memcmp(val_ptr->nibbles_ptr, other_value.val_ptr->nibbles_ptr,
    (val_ptr->n_nibbles + 1) / 2);
}

boolean HEXSTRING::operator==(const HEXSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of hexstring comparison.");
  other_value.must_bound("Unbound right operand of hexstring element "
    "comparison.");
  if (val_ptr->n_nibbles != 1) return FALSE;
  return get_nibble(0) == other_value.get_nibble();
}

HEXSTRING HEXSTRING::operator+(const HEXSTRING& other_value) const
{
  must_bound("Unbound left operand of hexstring concatenation.");
  other_value.must_bound("Unbound right operand of hexstring concatenation.");

  int left_n_nibbles = val_ptr->n_nibbles;
  if (left_n_nibbles == 0) return other_value;

  int right_n_nibbles = other_value.val_ptr->n_nibbles;
  if (right_n_nibbles == 0) return *this;

  int n_nibbles = left_n_nibbles + right_n_nibbles;
  // the result
  HEXSTRING ret_val(n_nibbles);

  // the number of bytes used
  int left_n_bytes = (left_n_nibbles + 1) / 2;
  int right_n_bytes = (right_n_nibbles + 1) / 2;

  // pointers to the data areas
  const unsigned char *left_ptr = val_ptr->nibbles_ptr;
  const unsigned char *right_ptr = other_value.val_ptr->nibbles_ptr;
  unsigned char *dest_ptr = ret_val.val_ptr->nibbles_ptr;

  memcpy(dest_ptr, left_ptr, left_n_bytes);

  if (left_n_nibbles % 2) {
    dest_ptr[left_n_bytes - 1] &= 0x0F;
    int n_bytes = (n_nibbles + 1) / 2;
    for (int i = left_n_bytes; i < n_bytes; i++) {
      unsigned char right_byte = right_ptr[i - left_n_bytes];
      dest_ptr[i - 1] |= right_byte << 4;
      dest_ptr[i] = right_byte >> 4;
    }
    if (right_n_nibbles % 2) dest_ptr[n_bytes - 1] |= right_ptr[right_n_bytes
      - 1] << 4;
  }
  else {
    memcpy(dest_ptr + left_n_bytes, right_ptr, right_n_bytes);
    ret_val.clear_unused_nibble();
  }
  return ret_val;
}

HEXSTRING HEXSTRING::operator+(const HEXSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of hexstring concatenation.");
  other_value.must_bound("Unbound right operand of hexstring element "
    "concatenation.");
  int n_nibbles = val_ptr->n_nibbles;
  HEXSTRING ret_val(n_nibbles + 1);
  memcpy(ret_val.val_ptr->nibbles_ptr, val_ptr->nibbles_ptr, (n_nibbles + 1)
    / 2);
  ret_val.set_nibble(n_nibbles, other_value.get_nibble());
  return ret_val;
}

#ifdef TITAN_RUNTIME_2
HEXSTRING HEXSTRING::operator+(const OPTIONAL<HEXSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const HEXSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of hexstring concatenation.");
}
#endif

HEXSTRING HEXSTRING::operator~() const
{
  must_bound("Unbound hexstring operand of operator not4b.");

  int n_bytes = (val_ptr->n_nibbles + 1) / 2;
  if (n_bytes == 0) return *this;
  HEXSTRING ret_val(val_ptr->n_nibbles);
  for (int i = 0; i < n_bytes; i++) {
    ret_val.val_ptr->nibbles_ptr[i] = ~val_ptr->nibbles_ptr[i];
  }
  ret_val.clear_unused_nibble();
  return ret_val;
}

HEXSTRING HEXSTRING::operator&(const HEXSTRING& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound hexstring value.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "hexstring value.");
  int n_nibbles = val_ptr->n_nibbles;
  if (n_nibbles != other_value.val_ptr->n_nibbles) TTCN_error("The hexstring "
    "operands of operator and4b must have the same length.");
  if (n_nibbles == 0) return *this;
  HEXSTRING ret_val(n_nibbles);
  int n_bytes = (n_nibbles + 1) / 2;
  for (int i = 0; i < n_bytes; i++) {
    ret_val.val_ptr->nibbles_ptr[i] = val_ptr->nibbles_ptr[i]
      & other_value.val_ptr->nibbles_ptr[i];
  }
  ret_val.clear_unused_nibble();
  return ret_val;
}

HEXSTRING HEXSTRING::operator&(const HEXSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound hexstring value.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "hexstring element.");
  if (val_ptr->n_nibbles != 1) TTCN_error("The hexstring operands of operator "
    "and4b must have the same length.");
  unsigned char result = get_nibble(0) & other_value.get_nibble();
  return HEXSTRING(1, &result);
}

HEXSTRING HEXSTRING::operator|(const HEXSTRING& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound hexstring value.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "hexstring value.");
  int n_nibbles = val_ptr->n_nibbles;
  if (n_nibbles != other_value.val_ptr->n_nibbles) TTCN_error("The hexstring "
    "operands of operator or4b must have the same length.");
  if (n_nibbles == 0) return *this;
  HEXSTRING ret_val(n_nibbles);
  int n_bytes = (n_nibbles + 1) / 2;
  for (int i = 0; i < n_bytes; i++) {
    ret_val.val_ptr->nibbles_ptr[i] = val_ptr->nibbles_ptr[i]
      | other_value.val_ptr->nibbles_ptr[i];
  }
  ret_val.clear_unused_nibble();
  return ret_val;
}

HEXSTRING HEXSTRING::operator|(const HEXSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound hexstring value.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "hexstring element.");
  if (val_ptr->n_nibbles != 1) TTCN_error("The hexstring operands of operator "
    "or4b must have the same length.");
  unsigned char result = get_nibble(0) | other_value.get_nibble();
  return HEXSTRING(1, &result);
}

HEXSTRING HEXSTRING::operator^(const HEXSTRING& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound hexstring value.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "hexstring value.");
  int n_nibbles = val_ptr->n_nibbles;
  if (n_nibbles != other_value.val_ptr->n_nibbles) TTCN_error("The hexstring "
    "operands of operator xor4b must have the same length.");
  if (n_nibbles == 0) return *this;
  HEXSTRING ret_val(n_nibbles);
  int n_bytes = (n_nibbles + 1) / 2;
  for (int i = 0; i < n_bytes; i++) {
    ret_val.val_ptr->nibbles_ptr[i] = val_ptr->nibbles_ptr[i]
      ^ other_value.val_ptr->nibbles_ptr[i];
  }
  ret_val.clear_unused_nibble();
  return ret_val;
}

HEXSTRING HEXSTRING::operator^(const HEXSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound hexstring value.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "hexstring element.");
  if (val_ptr->n_nibbles != 1) TTCN_error("The hexstring operands of operator "
    "xor4b must have the same length.");
  unsigned char result = get_nibble(0) ^ other_value.get_nibble();
  return HEXSTRING(1, &result);
}

HEXSTRING HEXSTRING::operator<<(int shift_count) const
{
  must_bound("Unbound hexstring operand of shift left operator.");

  if (shift_count > 0) {
    int n_nibbles = val_ptr->n_nibbles;
    if (n_nibbles == 0) return *this;
    HEXSTRING ret_val(n_nibbles);
    int n_bytes = (n_nibbles + 1) / 2;
    clear_unused_nibble();
    if (shift_count > n_nibbles) shift_count = n_nibbles;
    int shift_bytes = shift_count / 2;
    if (shift_count % 2) {
      int byte_count = 0;
      for (; byte_count < n_bytes - shift_bytes - 1; byte_count++) {
        ret_val.val_ptr->nibbles_ptr[byte_count]
          = (val_ptr->nibbles_ptr[byte_count + shift_bytes] >> 4)
            | (val_ptr->nibbles_ptr[byte_count + shift_bytes + 1] << 4);
      }
      ret_val.val_ptr->nibbles_ptr[n_bytes - shift_bytes - 1]
        = val_ptr->nibbles_ptr[n_bytes - 1] >> 4;
    }
    else {
      memcpy(ret_val.val_ptr->nibbles_ptr, &val_ptr->nibbles_ptr[shift_count
        / 2], (n_nibbles - shift_count + 1) / 2);
    }
    memset(ret_val.val_ptr->nibbles_ptr + n_bytes - shift_bytes, 0, shift_bytes);
    return ret_val;
  }
  else if (shift_count == 0)
    return *this;
  else
    return *this >> (-shift_count);
}

HEXSTRING HEXSTRING::operator<<(const INTEGER& shift_count) const
{
  shift_count.must_bound("Unbound right operand of hexstring shift left "
    "operator.");
  return *this << (int) shift_count;
}

HEXSTRING HEXSTRING::operator>>(int shift_count) const
{
  must_bound("Unbound operand of hexstring shift right operator.");

  if (shift_count > 0) {
    int n_nibbles = val_ptr->n_nibbles;
    if (n_nibbles == 0) return *this;
    HEXSTRING ret_val(n_nibbles);
    int n_bytes = (n_nibbles + 1) / 2;
    clear_unused_nibble();
    if (shift_count > n_nibbles) shift_count = n_nibbles;
    int shift_bytes = shift_count / 2;
    memset(ret_val.val_ptr->nibbles_ptr, 0, shift_bytes);
    if (shift_count % 2) {
      ret_val.val_ptr->nibbles_ptr[shift_bytes] = val_ptr->nibbles_ptr[0] << 4;
      int byte_count = shift_bytes + 1;
      for (; byte_count < n_bytes; byte_count++) {
        ret_val.val_ptr->nibbles_ptr[byte_count]
          = (val_ptr->nibbles_ptr[byte_count - shift_bytes - 1] >> 4)
            | (val_ptr->nibbles_ptr[byte_count - shift_bytes] << 4);
      }
    }
    else {
      memcpy(&ret_val.val_ptr->nibbles_ptr[shift_bytes], val_ptr->nibbles_ptr,
        (n_nibbles - shift_count + 1) / 2);
    }
    ret_val.clear_unused_nibble();
    return ret_val;
  }
  else if (shift_count == 0)
    return *this;
  else
    return *this << (-shift_count);
}

HEXSTRING HEXSTRING::operator>>(const INTEGER& shift_count) const
{
  shift_count.must_bound("Unbound right operand of hexstring shift right "
    "operator.");
  return *this >> (int) shift_count;
}

HEXSTRING HEXSTRING::operator<<=(int rotate_count) const
{
  must_bound("Unbound hexstring operand of rotate left operator.");
  if (val_ptr->n_nibbles == 0) return *this;
  if (rotate_count >= 0) {
    rotate_count %= val_ptr->n_nibbles;
    if (rotate_count == 0) return *this;
    return ((*this) << rotate_count) | ((*this) >> (val_ptr->n_nibbles
      - rotate_count));
  }
  else
    return *this >>= (-rotate_count);
}

HEXSTRING HEXSTRING::operator<<=(const INTEGER& rotate_count) const
{
  rotate_count.must_bound("Unbound right operand of hexstring rotate left "
    "operator.");
  return *this <<= (int) rotate_count;
}

HEXSTRING HEXSTRING::operator>>=(int rotate_count) const
{
  must_bound("Unbound hexstring operand of rotate right operator.");
  if (val_ptr->n_nibbles == 0) return *this;
  if (rotate_count >= 0) {
    rotate_count %= val_ptr->n_nibbles;
    if (rotate_count == 0) return *this;
    return ((*this) >> rotate_count) | ((*this) << (val_ptr->n_nibbles
      - rotate_count));
  }
  else
    return *this <<= (-rotate_count);
}

HEXSTRING HEXSTRING::operator>>=(const INTEGER& rotate_count) const
{
  rotate_count.must_bound("Unbound right operand of hexstring rotate right "
    "operator.");
  return *this >>= (int) rotate_count;
}

HEXSTRING_ELEMENT HEXSTRING::operator[](int index_value)
{
  if (val_ptr == NULL && index_value == 0) {
    init_struct(1);
    clear_unused_nibble();
    return HEXSTRING_ELEMENT(FALSE, *this, 0);
  }
  else {
    must_bound("Accessing an element of an unbound hexstring value.");
    if (index_value < 0) TTCN_error("Accessing an hexstring element using "
      "a negative index (%d).", index_value);
    int n_nibbles = val_ptr->n_nibbles;
    if (index_value > n_nibbles) TTCN_error("Index overflow when accessing a "
      "hexstring element: The index is %d, but the string has only %d "
      "hexadecimal digits.", index_value, n_nibbles);
    if (index_value == n_nibbles) {
      if (val_ptr->ref_count == 1) {
        if (n_nibbles % 2 == 0) val_ptr
          = (hexstring_struct*) Realloc(val_ptr, MEMORY_SIZE(n_nibbles + 1));
        val_ptr->n_nibbles++;
      }
      else {
        hexstring_struct *old_ptr = val_ptr;
        old_ptr->ref_count--;
        init_struct(n_nibbles + 1);
        memcpy(val_ptr->nibbles_ptr, old_ptr->nibbles_ptr, (n_nibbles + 1) / 2);
      }
      return HEXSTRING_ELEMENT(FALSE, *this, index_value);
    }
    else
      return HEXSTRING_ELEMENT(TRUE, *this, index_value);
  }
}

HEXSTRING_ELEMENT HEXSTRING::operator[](const INTEGER& index_value)
{
  index_value.must_bound("Indexing a hexstring value with an unbound integer "
    "value.");
  return (*this)[(int) index_value];
}

const HEXSTRING_ELEMENT HEXSTRING::operator[](int index_value) const
{
  must_bound("Accessing an element of an unbound hexstring value.");
  if (index_value < 0) TTCN_error("Accessing an hexstring element using a "
    "negative index (%d).", index_value);
  if (index_value >= val_ptr->n_nibbles) TTCN_error("Index overflow when "
    "accessing a hexstring element: The index is %d, but the string has only "
    "%d hexadecimal digits.", index_value, val_ptr->n_nibbles);
  return HEXSTRING_ELEMENT(TRUE, const_cast<HEXSTRING&> (*this), index_value);
}

const HEXSTRING_ELEMENT HEXSTRING::operator[](const INTEGER& index_value) const
{
  index_value.must_bound("Indexing a hexstring value with an unbound integer "
    "value.");
  return (*this)[(int) index_value];
}

int HEXSTRING::lengthof() const
{
  must_bound("Getting the length of an unbound hexstring value.");
  return val_ptr->n_nibbles;
}

HEXSTRING::operator const unsigned char*() const
{
  must_bound("Casting an unbound hexstring value to const unsigned char*.");
  return val_ptr->nibbles_ptr;
}

void HEXSTRING::log() const
{
  if (val_ptr != NULL) {
    TTCN_Logger::log_char('\'');
    for (int i = 0; i < val_ptr->n_nibbles; i++)
      TTCN_Logger::log_hex(get_nibble(i));
    TTCN_Logger::log_event_str("'H");
  }
  else {
    TTCN_Logger::log_event_unbound();
  }
}

void HEXSTRING::encode_text(Text_Buf& text_buf) const
{
  must_bound("Text encoder: Encoding an unbound hexstring value");
  int n_nibbles = val_ptr->n_nibbles;
  text_buf.push_int(n_nibbles);
  if (n_nibbles > 0) text_buf.push_raw((n_nibbles + 1) / 2,
    val_ptr->nibbles_ptr);
}

void HEXSTRING::decode_text(Text_Buf& text_buf)
{
  int n_nibbles = text_buf.pull_int().get_val();
  if (n_nibbles < 0) TTCN_error(
    "Text decoder: Invalid length was received for a hexstring.");
  clean_up();
  init_struct(n_nibbles);
  if (n_nibbles > 0) {
    text_buf.pull_raw((n_nibbles + 1) / 2, val_ptr->nibbles_ptr);
    clear_unused_nibble();
  }
}

void HEXSTRING::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE|Module_Param::BC_LIST, "hexstring value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Hexstring:
    switch (param.get_operation_type()) {
    case Module_Param::OT_ASSIGN: {
      clean_up();
      int n_nibbles = mp->get_string_size();
      init_struct(n_nibbles);
      memcpy(val_ptr->nibbles_ptr, mp->get_string_data(), (n_nibbles + 1) / 2);
      clear_unused_nibble();
    } break;
    case Module_Param::OT_CONCAT:
      if (is_bound()) {
        *this = *this + HEXSTRING(mp->get_string_size(), (unsigned char*)mp->get_string_data());
      } else {
        *this = HEXSTRING(mp->get_string_size(), (unsigned char*)mp->get_string_data());
      }
      break;
    default:
      TTCN_error("Internal error: HEXSTRING::set_param()");
    }
    break;
  case Module_Param::MP_Expression:
    if (mp->get_expr_type() == Module_Param::EXPR_CONCATENATE) {
      HEXSTRING operand1, operand2;
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
      param.expr_type_error("a hexstring");
    }
    break;
  default:
    param.type_error("hexstring value");
    break;
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* HEXSTRING::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  int n_bytes = (val_ptr->n_nibbles + 1) / 2;
  unsigned char* val_cpy = (unsigned char *)Malloc(n_bytes);
  memcpy(val_cpy, val_ptr->nibbles_ptr, n_bytes);
  return new Module_Param_Hexstring(val_ptr->n_nibbles, val_cpy);
}
#endif

void HEXSTRING::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
  TTCN_EncDec::coding_t p_coding, ...) const
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch (p_coding) {
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-encoding type '%s': ", p_td.name);
    if (!p_td.raw) TTCN_EncDec_ErrorContext::error_internal(
      "No RAW descriptor available for type '%s'.", p_td.name);
    RAW_enc_tr_pos rp;
    rp.level = 0;
    rp.pos = NULL;
    RAW_enc_tree root(TRUE, NULL, &rp, 1, p_td.raw);
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
    TTCN_error("Unknown coding method requested to encode type '%s'", p_td.name);
  }
  va_end(pvar);
}

void HEXSTRING::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
  TTCN_EncDec::coding_t p_coding, ...)
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch (p_coding) {
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-decoding type '%s': ", p_td.name);
    if (!p_td.raw) TTCN_EncDec_ErrorContext::error_internal(
      "No RAW descriptor available for type '%s'.", p_td.name);
    raw_order_t order;
    switch (p_td.raw->top_bit_order) {
    case TOP_BIT_LEFT:
      order = ORDER_LSB;
      break;
    case TOP_BIT_RIGHT:
    default:
      order = ORDER_MSB;
    }
    if (RAW_decode(p_td, p_buf, p_buf.get_len() * 8, order) < 0) ec.error(
      TTCN_EncDec::ET_INCOMPL_MSG,
      "Can not decode type '%s', because invalid or incomplete"
        " message was received", p_td.name);
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
    TTCN_error("Unknown coding method requested to decode type '%s'", p_td.name);
  }
  va_end(pvar);
}

int HEXSTRING::RAW_encode(const TTCN_Typedescriptor_t& p_td,
  RAW_enc_tree& myleaf) const
{
  //  unsigned char *bc;
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
  }
  int nbits = val_ptr->n_nibbles * 4;
  int align_length = p_td.raw->fieldlength ? p_td.raw->fieldlength - nbits : 0;
  if ((nbits + align_length) < val_ptr->n_nibbles * 4) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There is no sufficient bits to encode '%s': ", p_td.name);
    nbits = p_td.raw->fieldlength;
    align_length = 0;
  }

  if (myleaf.must_free) Free(myleaf.body.leaf.data_ptr);

  myleaf.must_free = FALSE;
  myleaf.data_ptr_used = TRUE;
  myleaf.body.leaf.data_ptr = val_ptr->nibbles_ptr;

  if (p_td.raw->endianness == ORDER_MSB)
    myleaf.align = -align_length;
  else
    myleaf.align = align_length;
  return myleaf.length = nbits + align_length;
}

int HEXSTRING::RAW_decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff,
  int limit, raw_order_t top_bit_ord, boolean no_err, int /*sel_field*/,
  boolean /*first_call*/)
{
  int prepaddlength = buff.increase_pos_padd(p_td.raw->prepadding);
  limit -= prepaddlength;
  int decode_length = p_td.raw->fieldlength == 0
    ? (limit / 4) * 4 : p_td.raw->fieldlength;
  if ( p_td.raw->fieldlength > limit
    || p_td.raw->fieldlength > (int) buff.unread_len_bit()) {
    if (no_err) return -TTCN_EncDec::ET_LEN_ERR;
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There is not enough bits in the buffer to decode type %s.", p_td.name);
    decode_length = ((limit > (int) buff.unread_len_bit()
      ? (int)buff.unread_len_bit() : limit) / 4) * 4;
  }
  RAW_coding_par cp;
  boolean orders = FALSE;
  if (p_td.raw->bitorderinoctet == ORDER_MSB) orders = TRUE;
  if (p_td.raw->bitorderinfield == ORDER_MSB) orders = !orders;
  cp.bitorder = orders ? ORDER_MSB : ORDER_LSB;
  orders = FALSE;
  if (p_td.raw->byteorder == ORDER_MSB) orders = TRUE;
  if (p_td.raw->bitorderinfield == ORDER_MSB) orders = !orders;
  cp.byteorder = orders ? ORDER_MSB : ORDER_LSB;
  cp.fieldorder = p_td.raw->fieldorder;
  cp.hexorder = p_td.raw->hexorder;
  clean_up();
  init_struct(decode_length / 4);
  buff.get_b((size_t) decode_length, val_ptr->nibbles_ptr, cp, top_bit_ord);

  if (p_td.raw->length_restrition != -1) {
    val_ptr->n_nibbles = p_td.raw->length_restrition;
    if (p_td.raw->endianness == ORDER_MSB) {
      if ((decode_length - val_ptr->n_nibbles * 4) % 8) {
        int bound = (decode_length - val_ptr->n_nibbles * 4) % 8;
        int maxindex = (decode_length - 1) / 8;
        for (int a = 0, b = (decode_length - val_ptr->n_nibbles * 4 - 1) / 8; a
          < (val_ptr->n_nibbles * 4 + 7) / 8; a++, b++) {
          val_ptr->nibbles_ptr[a] = val_ptr->nibbles_ptr[b] >> bound;
          if (b < maxindex)
            val_ptr->nibbles_ptr[a] = val_ptr->nibbles_ptr[b + 1] << (8 - bound);
        }
      }
      else memmove(val_ptr->nibbles_ptr,
        val_ptr->nibbles_ptr + (decode_length - val_ptr->n_nibbles * 4) / 8,
        val_ptr->n_nibbles * 8 * sizeof(unsigned char));
    }
  }

  /*  for(int a=0; a<decode_length/8; a++)
   val_ptr->nibbles_ptr[a]= ((val_ptr->nibbles_ptr[a]<<4)&0xf0) |
   ((val_ptr->nibbles_ptr[a]>>4)&0x0f);*/
  decode_length += buff.increase_pos_padd(p_td.raw->padding);
  clear_unused_nibble();
  return decode_length + prepaddlength;
}

// From Charstring.cc
extern char  base64_decoder_table[256];
extern const char cb64[];

int HEXSTRING::XER_encode(const XERdescriptor_t& p_td,
  TTCN_Buffer& p_buf, unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound hexstring value.");
  }
  int exer  = is_exer(flavor |= SIMPLE_TYPE);
  // SIMPLE_TYPE has no influence on is_exer, we set it for later
  int encoded_length=(int)p_buf.get_len();
  int empty_element = val_ptr==NULL || val_ptr->n_nibbles == 0;

  flavor &= ~XER_RECOF; // octetstring doesn't care
  begin_xml(p_td, p_buf, flavor, indent, empty_element);

  if (exer && (p_td.xer_bits & BASE_64)) {
    // bit more work
    size_t clear_len = (val_ptr->n_nibbles + 1) / 2; // lengthof is in nibbles
    const unsigned char * in = val_ptr->nibbles_ptr;

    /* Encode (up to) 6 nibbles of cleartext into 4 bytes of base64.
     * This is different from Octetstring.cc because hexstring's
     * big-endian data storage. */
    for (size_t i = 0; i < clear_len; i += 3) {
      unsigned char first  = in[i],
        second = ((i+1 < clear_len) ? in[i+1] :0),
        third  = ((i+2 < clear_len) ? in[i+2] :0);

      p_buf.put_c(cb64[(first & 0x0F) << 2 | first >> 6]);
      p_buf.put_c(cb64[(first & 0x30)      | (second & 0x0F)]);
      p_buf.put_c(i+1 >= clear_len ? '='
        : cb64[(second & 0xF0) >> 2 | (third & 0x0C) >> 2]);
      p_buf.put_c( i+2 >= clear_len ? '='
        : cb64[(third  & 0x03) << 4 | (third & 0xF0) >> 4]);
    } // next i
  }
  else {
    CHARSTRING val = hex2str(*this);
    p_buf.put_string(val);
  }

  end_xml(p_td, p_buf, flavor, indent, empty_element);

  return (int)p_buf.get_len() - encoded_length;

}

unsigned int xlate_hs(xmlChar in[4], int phase, unsigned char*dest) {
  static unsigned char nbytes[4] = { 3,1,1,2 };
  unsigned char out[4];

  out[0] = in[0] >> 2 | (in[0] & 3) << 6 | (in[1] & 0x30);
  out[1] = (in[1] & 0x0F) | (in[2] & 0x3C) << 2;
  out[2] = (in[3] & 0x0F) << 4 | (in[3] & 0x30) >> 4 | (in[2] & 3) << 2;
  memcpy(dest, out, nbytes[phase]);
  return nbytes[phase];
}


/* Here's how the bits get transferred to and from Base64:

Titan stores the hex digits in "little endian order", the first (index 0)
goes into the lower nibble, the second goes into the high nibble
of the first byte. So, For the hexstring value 'DECAFBAD'H,
Titan stores the following bytes: ED AC BF DA

Because of this, the bit shifting is different. The first three bytes

3x8 bits: eeeedddd aaaacccc bbbbffff

4x6 bits: ddddee eecccc aaaaff ffbbbb

*/

int HEXSTRING::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
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
          *this = *static_cast<const HEXSTRING*> (p_td.dfeValue);
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
        *this = *static_cast<const HEXSTRING*>(p_td.dfeValue);
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
      init_struct(len * 3 / 2); // 4 bytes decoded into 3 octets (6 nibbles)
      unsigned char * dest = val_ptr->nibbles_ptr;

      for (size_t o=0; o<len; ++o) {
        xmlChar c = value[o];
        if(c == '=') { // padding starts
          dest += xlate_hs(in, phase, dest);
          break;
        }

        int val = base64_decoder_table[c];
        if(val >= 0)    {
          in[phase] = val;
          phase = (phase + 1) % 4;
          if(phase == 0) {
            dest += xlate_hs(in,phase, dest);
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
      val_ptr->n_nibbles = (dest - val_ptr->nibbles_ptr) * 2;

    }
    else { // not base64
      init_struct(len);

      for (size_t i = 0; i < len; ++i) {
        unsigned char nibble = char_to_hexdigit(value[i]);
        if (nibble > 0x0F) {
          if (exer && (flavor & EXIT_ON_ERROR)) {
            clean_up();
            return -1;
          } else {
            TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
              "The hexstring value may contain hexadecimal digits only. "
              "Character \"%c\" was found.", value[i]);
            nibble=0;
          }
        }
        //val_ptr->nibbles_ptr[i] = nibble;
        set_nibble(i, nibble);
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
        *this = *static_cast<const HEXSTRING*>(p_td.dfeValue);
      }
      reader.Read(); // one last time
      break;
    }
  }
finished:
  return 1; // decode successful
}

int HEXSTRING::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound hexstring value.");
    return -1;
  }
  
  char* tmp_str = (char*)Malloc(val_ptr->n_nibbles + 3);
  tmp_str[0] = '\"';
  tmp_str[val_ptr->n_nibbles + 1] = '\"';
  for(int i = 0; i < val_ptr->n_nibbles; ++i) {
    if (i % 2) {
      tmp_str[i + 1] = hexdigit_to_char(val_ptr->nibbles_ptr[i / 2] >> 4);
    } else {
      tmp_str[i + 1] = hexdigit_to_char(val_ptr->nibbles_ptr[i / 2] & 0x0F);
    }
  }
  tmp_str[val_ptr->n_nibbles + 2] = 0;
  int enc_len = p_tok.put_next_token(JSON_TOKEN_STRING, tmp_str);
  Free(tmp_str);
  return enc_len;
}

int HEXSTRING::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
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
      // White spaces are ignored, so the resulting hexstring might be shorter
      // than the extracted JSON string
      int nibbles = value_len;
      for (size_t i = 0; i < value_len; ++i) {
        if (value[i] == ' ') {
          --nibbles;
        }
        else if (!isxdigit(value[i])) {
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
      }
      if (!error) {
        init_struct(nibbles);
        int nibble_index = 0;
        for (size_t i = 0; i < value_len; ++i) {
          if (!isxdigit(value[i])) {
            continue;
          }
          set_nibble(nibble_index, char_to_hexdigit(value[i]));
          ++nibble_index;
        }
      }
    } else {
      error = TRUE;
    }
  } else {
    return JSON_ERROR_INVALID_TOKEN;
  }
  
  if (error) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FORMAT_ERROR, "string", "hexstring");
    return JSON_ERROR_FATAL;    
  }
  return (int)dec_len;
}


//---------------------- hexstring element class ----------------------

HEXSTRING_ELEMENT::HEXSTRING_ELEMENT(boolean par_bound_flag,
  HEXSTRING& par_str_val, int par_nibble_pos) :
  bound_flag(par_bound_flag), str_val(par_str_val), nibble_pos(par_nibble_pos)
{
}

HEXSTRING_ELEMENT& HEXSTRING_ELEMENT::operator=(
  const HEXSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound hexstring element.");
  bound_flag = TRUE;
  str_val.copy_value();
  str_val.set_nibble(nibble_pos, other_value.str_val.get_nibble(
    other_value.nibble_pos));
  return *this;
}

HEXSTRING_ELEMENT& HEXSTRING_ELEMENT::operator=(const HEXSTRING& other_value)
{
  other_value.must_bound("Assignment of unbound hexstring value.");
  if (other_value.lengthof() != 1) TTCN_error(
    "Assignment of a hexstring value "
      "with length other than 1 to a hexstring element.");
  bound_flag = TRUE;
  str_val.copy_value();
  str_val.set_nibble(nibble_pos, other_value.get_nibble(0));
  return *this;
}

boolean HEXSTRING_ELEMENT::operator==(const HEXSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of hexstring element comparison.");
  other_value.must_bound("Unbound right operand of hexstring comparison.");
  return str_val.get_nibble(nibble_pos) == other_value.str_val.get_nibble(
    other_value.nibble_pos);
}

boolean HEXSTRING_ELEMENT::operator==(const HEXSTRING& other_value) const
{
  must_bound("Unbound left operand of hexstring element comparison.");
  other_value.must_bound("Unbound right operand of hexstring element "
    "comparison.");
  if (other_value.val_ptr->n_nibbles != 1) return FALSE;
  return str_val.get_nibble(nibble_pos) == other_value.get_nibble(0);
}

HEXSTRING HEXSTRING_ELEMENT::operator+(const HEXSTRING& other_value) const
{
  must_bound("Unbound left operand of hexstring element concatenation.");
  other_value.must_bound("Unbound right operand of hexstring concatenation.");
  int n_nibbles = other_value.val_ptr->n_nibbles;
  HEXSTRING ret_val(n_nibbles + 1);
  const unsigned char *src_ptr = other_value.val_ptr->nibbles_ptr;
  unsigned char *dest_ptr = ret_val.val_ptr->nibbles_ptr;
  dest_ptr[0] = str_val.get_nibble(nibble_pos);
  // bytes in the result minus 1
  int n_complete_bytes = n_nibbles / 2;
  for (int i = 0; i < n_complete_bytes; i++) {
    unsigned char right_octet = src_ptr[i];
    dest_ptr[i] |= right_octet << 4;
    dest_ptr[i + 1] = right_octet >> 4;
  }
  if (n_nibbles % 2) dest_ptr[n_complete_bytes] |= src_ptr[n_complete_bytes]
    << 4;
  return ret_val;
}

HEXSTRING HEXSTRING_ELEMENT::operator+(const HEXSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound left operand of hexstring element concatenation.");
  other_value.must_bound("Unbound right operand of hexstring element "
    "concatenation.");
  unsigned char result = str_val.get_nibble(nibble_pos)
    | (other_value.str_val.get_nibble(other_value.nibble_pos) << 4);
  return HEXSTRING(2, &result);
}

#ifdef TITAN_RUNTIME_2
HEXSTRING HEXSTRING_ELEMENT::operator+(
  const OPTIONAL<HEXSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const HEXSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of hexstring concatenation.");
}
#endif

HEXSTRING HEXSTRING_ELEMENT::operator~() const
{
  must_bound("Unbound hexstring element operand of operator not4b.");
  unsigned char result = ~str_val.get_nibble(nibble_pos) & 0x0F;
  return HEXSTRING(1, &result);
}

HEXSTRING HEXSTRING_ELEMENT::operator&(const HEXSTRING& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound hexstring element.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "hexstring value.");
  if (other_value.val_ptr->n_nibbles != 1) TTCN_error("The hexstring operands "
    "of operator and4b must have the same length.");
  unsigned char result = str_val.get_nibble(nibble_pos)
    & other_value.get_nibble(0);
  return HEXSTRING(1, &result);
}

HEXSTRING HEXSTRING_ELEMENT::operator&(const HEXSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator and4b is an unbound hexstring element.");
  other_value.must_bound("Right operand of operator and4b is an unbound "
    "hexstring element.");
  unsigned char result = str_val.get_nibble(nibble_pos)
    & other_value.str_val.get_nibble(other_value.nibble_pos);
  return HEXSTRING(1, &result);
}

HEXSTRING HEXSTRING_ELEMENT::operator|(const HEXSTRING& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound hexstring element.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "hexstring value.");
  if (other_value.val_ptr->n_nibbles != 1) TTCN_error("The hexstring operands "
    "of operator or4b must have the same length.");
  unsigned char result = str_val.get_nibble(nibble_pos)
    | other_value.get_nibble(0);
  return HEXSTRING(1, &result);
}

HEXSTRING HEXSTRING_ELEMENT::operator|(const HEXSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator or4b is an unbound hexstring element.");
  other_value.must_bound("Right operand of operator or4b is an unbound "
    "hexstring element.");
  unsigned char result = str_val.get_nibble(nibble_pos)
    | other_value.str_val.get_nibble(other_value.nibble_pos);
  return HEXSTRING(1, &result);
}

HEXSTRING HEXSTRING_ELEMENT::operator^(const HEXSTRING& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound hexstring element.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "hexstring value.");
  if (other_value.val_ptr->n_nibbles != 1) TTCN_error("The hexstring operands "
    "of operator xor4b must have the same length.");
  unsigned char result = str_val.get_nibble(nibble_pos)
    ^ other_value.get_nibble(0);
  return HEXSTRING(1, &result);
}

HEXSTRING HEXSTRING_ELEMENT::operator^(const HEXSTRING_ELEMENT& other_value) const
{
  must_bound("Left operand of operator xor4b is an unbound hexstring element.");
  other_value.must_bound("Right operand of operator xor4b is an unbound "
    "hexstring element.");
  unsigned char result = str_val.get_nibble(nibble_pos)
    ^ other_value.str_val.get_nibble(other_value.nibble_pos);
  return HEXSTRING(1, &result);
}

unsigned char HEXSTRING_ELEMENT::get_nibble() const
{
  return str_val.get_nibble(nibble_pos);
}

void HEXSTRING_ELEMENT::log() const
{
  if (bound_flag) {
    TTCN_Logger::log_char('\'');
    TTCN_Logger::log_hex(str_val.get_nibble(nibble_pos));
    TTCN_Logger::log_event_str("'H");
  }
  else {
    TTCN_Logger::log_event_unbound();
  }
}

//---------------------- hexstring template class ----------------------

void HEXSTRING_template::clean_up()
{
  switch (template_selection) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete[] value_list.list_value;
    break;
  case STRING_PATTERN:
    if (pattern_value->ref_count > 1)
      pattern_value->ref_count--;
    else if (pattern_value->ref_count == 1)
      Free(pattern_value);
    else
      TTCN_error("Internal error: Invalid reference counter in a hexstring "
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

void HEXSTRING_template::copy_template(const HEXSTRING_template& other_value)
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
    value_list.list_value = new HEXSTRING_template[value_list.n_values];
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
    TTCN_error("Copying an uninitialized/unsupported hexstring template.");
  }
  set_selection(other_value);
}

/*
 This is the same algorithm that match_array uses
 to match 'record of' types.
 The only differences are: how two elements are matched and
 how an asterisk or ? is identified in the template
 */
boolean HEXSTRING_template::match_pattern(
  const hexstring_pattern_struct *string_pattern,
  const HEXSTRING::hexstring_struct *string_value)
{
  // the empty pattern matches the empty hexstring only
  if (string_pattern->n_elements == 0) return string_value->n_nibbles == 0;

  int value_index = 0;
  unsigned int template_index = 0;
  int last_asterisk = -1;
  int last_value_to_asterisk = -1;
  //the following variables are just to speed up the function
  unsigned char pattern_element;
  unsigned char octet;
  unsigned char hex_digit;

  for (;;) {
    pattern_element = string_pattern->elements_ptr[template_index];
    if (pattern_element < 16) {
      octet = string_value->nibbles_ptr[value_index / 2];
      if (value_index % 2)
        hex_digit = octet >> 4;
      else
        hex_digit = octet & 0x0F;
      if (hex_digit == pattern_element) {
        value_index++;
        template_index++;
      }
      else {
        if (last_asterisk == -1) return FALSE;
        template_index = last_asterisk + 1;
        value_index = ++last_value_to_asterisk;
      }
    }
    else if (pattern_element == 16) {//?
      value_index++;
      template_index++;
    }
    else if (pattern_element == 17) {//*
      last_asterisk = template_index++;
      last_value_to_asterisk = value_index;
    }
    else
      TTCN_error("Internal error: invalid element in a hexstring "
        "pattern.");

    if (value_index == string_value->n_nibbles && template_index
      == string_pattern->n_elements) {
      return TRUE;
    }
    else if (template_index == string_pattern->n_elements) {
      if (string_pattern->elements_ptr[template_index - 1] == 17) {
        return TRUE;
      }
      else if (last_asterisk == -1) {
        return FALSE;
      }
      else {
        template_index = last_asterisk + 1;
        value_index = ++last_value_to_asterisk;
      }
    }
    else if (value_index == string_value->n_nibbles) {
      while (template_index < string_pattern->n_elements
        && string_pattern->elements_ptr[template_index] == 17)
        template_index++;

      return template_index == string_pattern->n_elements;
    }
  }
}

HEXSTRING_template::HEXSTRING_template()
{
}

HEXSTRING_template::HEXSTRING_template(template_sel other_value) :
  Restricted_Length_Template(other_value)
{
  check_single_selection(other_value);
}

HEXSTRING_template::HEXSTRING_template(const HEXSTRING& other_value) :
  Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

HEXSTRING_template::HEXSTRING_template(const HEXSTRING_ELEMENT& other_value) :
  Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

HEXSTRING_template::HEXSTRING_template(const OPTIONAL<HEXSTRING>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const HEXSTRING&) other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a hexstring template from an unbound optional field.");
  }
}

HEXSTRING_template::HEXSTRING_template(unsigned int n_elements,
  const unsigned char *pattern_elements) :
  Restricted_Length_Template(STRING_PATTERN)
{
  pattern_value
    = (hexstring_pattern_struct*) Malloc(sizeof(hexstring_pattern_struct) + n_elements - 1);
  pattern_value->ref_count = 1;
  pattern_value->n_elements = n_elements;
  memcpy(pattern_value->elements_ptr, pattern_elements, n_elements);
}

HEXSTRING_template::HEXSTRING_template(const HEXSTRING_template& other_value) :
  Restricted_Length_Template()
{
  copy_template(other_value);
}

HEXSTRING_template::~HEXSTRING_template()
{
  clean_up();
}

HEXSTRING_template& HEXSTRING_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

HEXSTRING_template& HEXSTRING_template::operator=(const HEXSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound hexstring value to a "
    "template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

HEXSTRING_template& HEXSTRING_template::operator=(
  const HEXSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound hexstring element to a "
    "template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

HEXSTRING_template& HEXSTRING_template::operator=(
  const OPTIONAL<HEXSTRING>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const HEXSTRING&) other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a hexstring "
      "template.");
  }
  return *this;
}

HEXSTRING_template& HEXSTRING_template::operator=(
  const HEXSTRING_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

#ifdef TITAN_RUNTIME_2
void HEXSTRING_template::concat(Vector<unsigned char>& v) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    switch (length_restriction_type) {
    case NO_LENGTH_RESTRICTION:
      if (template_selection == ANY_VALUE) {
        // ? => '*'
        if (v.size() == 0 || v[v.size() - 1] != 17) {
          // '**' == '*', so just ignore the second '*'
          v.push_back(17);
        }
      }
      else {
        TTCN_error("Operand of hexstring template concatenation is an "
          "AnyValueOrNone (*) matching mechanism with no length restriction");
      }
      break;
    case RANGE_LENGTH_RESTRICTION:
      if (!length_restriction.range_length.max_length ||
          length_restriction.range_length.max_length != length_restriction.range_length.min_length) {
        TTCN_error("Operand of hexstring template concatenation is an %s "
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
        v.push_back(16);
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
    TTCN_error("Operand of hexstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
}

void HEXSTRING_template::concat(Vector<unsigned char>& v, const HEXSTRING& val)
{
  if (!val.is_bound()) {
    TTCN_error("Operand of hexstring template concatenation is an "
      "unbound value.");
  }
  for (int i = 0; i < val.val_ptr->n_nibbles; ++i) {
    v.push_back(val.get_nibble(i));
  }
}

void HEXSTRING_template::concat(Vector<unsigned char>& v, template_sel sel)
{
  if (sel == ANY_VALUE) {
    // ? => '*'
    if (v.size() == 0 || v[v.size() - 1] != 17) {
      // '**' == '*', so just ignore the second '*'
      v.push_back(17);
    }
  }
  else {
    TTCN_error("Operand of hexstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
}


HEXSTRING_template HEXSTRING_template::operator+(
  const HEXSTRING_template& other_value) const
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
    return HEXSTRING_template(ANY_VALUE);
  }
  // otherwise the result is an hexstring pattern
  Vector<unsigned char> v;
  concat(v);
  other_value.concat(v);
  return HEXSTRING_template(v.size(), v.data_ptr());
}

HEXSTRING_template HEXSTRING_template::operator+(
  const HEXSTRING& other_value) const
{
  if (template_selection == SPECIFIC_VALUE) {
    // result is a specific value template
    return single_value + other_value;
  }
  // otherwise the result is an hexstring pattern
  Vector<unsigned char> v;
  concat(v);
  concat(v, other_value);
  return HEXSTRING_template(v.size(), v.data_ptr());
}

HEXSTRING_template HEXSTRING_template::operator+(
  const HEXSTRING_ELEMENT& other_value) const
{
  return *this + HEXSTRING(other_value);
}

HEXSTRING_template HEXSTRING_template::operator+(
  const OPTIONAL<HEXSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const HEXSTRING&)other_value;
  }
  TTCN_error("Operand of hexstring template concatenation is an "
    "unbound or omitted record/set field.");
}

HEXSTRING_template HEXSTRING_template::operator+(
  template_sel other_template_sel) const
{
  if (template_selection == ANY_VALUE && other_template_sel == ANY_VALUE &&
      length_restriction_type == NO_LENGTH_RESTRICTION) {
    // special case: ? & ? => ?
    return HEXSTRING_template(ANY_VALUE);
  }
  // the result is always an hexstring pattern
  Vector<unsigned char> v;
  concat(v);
  concat(v, other_template_sel);
  return HEXSTRING_template(v.size(), v.data_ptr());
}

HEXSTRING_template operator+(const HEXSTRING& left_value,
  const HEXSTRING_template& right_template)
{
  if (right_template.template_selection == SPECIFIC_VALUE) {
    // result is a specific value template
    return left_value + right_template.single_value;
  }
  // otherwise the result is an hexstring pattern
  Vector<unsigned char> v;
  HEXSTRING_template::concat(v, left_value);
  right_template.concat(v);
  return HEXSTRING_template(v.size(), v.data_ptr());
}

HEXSTRING_template operator+(const HEXSTRING_ELEMENT& left_value,
  const HEXSTRING_template& right_template)
{
  return HEXSTRING(left_value) + right_template;
}

HEXSTRING_template operator+(const OPTIONAL<HEXSTRING>& left_value,
  const HEXSTRING_template& right_template)
{
  if (left_value.is_present()) {
    return (const HEXSTRING&)left_value + right_template;
  }
  TTCN_error("Operand of hexstring template concatenation is an "
    "unbound or omitted record/set field.");
}

HEXSTRING_template operator+(template_sel left_template_sel,
  const HEXSTRING_template& right_template)
{
  if (left_template_sel == ANY_VALUE &&
      right_template.template_selection == ANY_VALUE &&
      right_template.length_restriction_type ==
      Restricted_Length_Template::NO_LENGTH_RESTRICTION) {
    // special case: ? & ? => ?
    return HEXSTRING_template(ANY_VALUE);
  }
  // the result is always an hexstring pattern
  Vector<unsigned char> v;
  HEXSTRING_template::concat(v, left_template_sel);
  right_template.concat(v);
  return HEXSTRING_template(v.size(), v.data_ptr());
}

HEXSTRING_template operator+(const HEXSTRING& left_value,
  template_sel right_template_sel)
{
  // the result is always an hexstring pattern
  Vector<unsigned char> v;
  HEXSTRING_template::concat(v, left_value);
  HEXSTRING_template::concat(v, right_template_sel);
  return HEXSTRING_template(v.size(), v.data_ptr());
}

HEXSTRING_template operator+(const HEXSTRING_ELEMENT& left_value,
  template_sel right_template_sel)
{
  return HEXSTRING(left_value) + right_template_sel;
}

HEXSTRING_template operator+(const OPTIONAL<HEXSTRING>& left_value,
  template_sel right_template_sel)
{
  if (left_value.is_present()) {
    return (const HEXSTRING&)left_value + right_template_sel;
  }
  TTCN_error("Operand of hexstring template concatenation is an "
    "unbound or omitted record/set field.");
}

HEXSTRING_template operator+(template_sel left_template_sel,
  const HEXSTRING& right_value)
{
  // the result is always an hexstring pattern
  Vector<unsigned char> v;
  HEXSTRING_template::concat(v, left_template_sel);
  HEXSTRING_template::concat(v, right_value);
  return HEXSTRING_template(v.size(), v.data_ptr());
}

HEXSTRING_template operator+(template_sel left_template_sel,
  const HEXSTRING_ELEMENT& right_value)
{
  return left_template_sel + HEXSTRING(right_value);
}

HEXSTRING_template operator+(template_sel left_template_sel,
  const OPTIONAL<HEXSTRING>& right_value)
{
  if (right_value.is_present()) {
    return left_template_sel + (const HEXSTRING&)right_value;
  }
  TTCN_error("Operand of hexstring template concatenation is an "
    "unbound or omitted record/set field.");
}
#endif // TITAN_RUNTIME_2

HEXSTRING_ELEMENT HEXSTRING_template::operator[](int index_value)
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Accessing a hexstring element of a non-specific hexstring "
               "template.");
  return single_value[index_value];
}

HEXSTRING_ELEMENT HEXSTRING_template::operator[](const INTEGER& index_value)
{
  index_value.must_bound("Indexing a hexstring template with an unbound "
                         "integer value.");
  return (*this)[(int)index_value];
}

const HEXSTRING_ELEMENT HEXSTRING_template::operator[](int index_value) const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Accessing a hexstring element of a non-specific hexstring "
               "template.");
  return single_value[index_value];
}

const HEXSTRING_ELEMENT HEXSTRING_template::operator[](const INTEGER& index_value) const
{
  index_value.must_bound("Indexing a hexstring template with an unbound "
                         "integer value.");
  return (*this)[(int)index_value];
}

boolean HEXSTRING_template::match(const HEXSTRING& other_value,
                                  boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  if (!match_length(other_value.val_ptr->n_nibbles)) return FALSE;
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
      if (value_list.list_value[i].match(other_value)) return template_selection
        == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  case STRING_PATTERN:
    return match_pattern(pattern_value, other_value.val_ptr);
  case DECODE_MATCH: {
    TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);
    TTCN_EncDec::clear_error();
    OCTETSTRING os(hex2oct(other_value));
    TTCN_Buffer buff(os);
    boolean ret_val = dec_match->instance->match(buff);
    TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL,TTCN_EncDec::EB_DEFAULT);
    TTCN_EncDec::clear_error();
    return ret_val; }
  default:
    TTCN_error("Matching an uninitialized/unsupported hexstring template.");
  }
  return FALSE;
}

const HEXSTRING& HEXSTRING_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent) TTCN_error(
    "Performing a valueof or send operation on a non-specific "
      "hexstring template.");
  return single_value;
}

int HEXSTRING_template::lengthof() const
{
  int min_length;
  boolean has_any_or_none;
  if (is_ifpresent) TTCN_error(
    "Performing lengthof() operation on a hexstring template "
      "which has an ifpresent attribute.");
  switch (template_selection) {
  case SPECIFIC_VALUE:
    min_length = single_value.lengthof();
    has_any_or_none = FALSE;
    break;
  case OMIT_VALUE:
    TTCN_error("Performing lengthof() operation on a hexstring template "
      "containing omit value.");
  case ANY_VALUE:
  case ANY_OR_OMIT:
    min_length = 0;
    has_any_or_none = TRUE; // max. length is infinity
    break;
  case VALUE_LIST: {
    // error if any element does not have length or the lengths differ
    if (value_list.n_values < 1) TTCN_error("Internal error: "
      "Performing lengthof() operation on a hexstring template "
      "containing an empty list.");
    int item_length = value_list.list_value[0].lengthof();
    for (unsigned int i = 1; i < value_list.n_values; i++) {
      if (value_list.list_value[i].lengthof() != item_length) TTCN_error(
        "Performing lengthof() operation on a hexstring template "
          "containing a value list with different lengths.");
    }
    min_length = item_length;
    has_any_or_none = FALSE;
    break;
  }
  case COMPLEMENTED_LIST:
    TTCN_error("Performing lengthof() operation on a hexstring template "
      "containing complemented list.");
  case STRING_PATTERN:
    min_length = 0;
    has_any_or_none = FALSE; // TRUE if * chars in the pattern
    for (unsigned int i = 0; i < pattern_value->n_elements; i++) {
      if (pattern_value->elements_ptr[i] < 17)
        min_length++; // case of 0-F, ?
      else
        has_any_or_none = TRUE; // case of * character
    }
    break;
  default:
    TTCN_error("Performing lengthof() operation on an "
      "uninitialized/unsupported hexstring template.");
  }
  return check_section_is_single(min_length, has_any_or_none, "length", "a",
    "hexstring template");
}

void HEXSTRING_template::set_type(template_sel template_type,
  unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST &&
      template_type != DECODE_MATCH) TTCN_error(
    "Setting an invalid list type for a hexstring template.");
  clean_up();
  set_selection(template_type);
  if (template_type != DECODE_MATCH) {
    value_list.n_values = list_length;
    value_list.list_value = new HEXSTRING_template[list_length];
  }
}

HEXSTRING_template& HEXSTRING_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST && template_selection
    != COMPLEMENTED_LIST) TTCN_error(
    "Accessing a list element of a non-list hexstring template.");
  if (list_index >= value_list.n_values) TTCN_error(
    "Index overflow in a hexstring value list template.");
  return value_list.list_value[list_index];
}

void HEXSTRING_template::set_decmatch(Dec_Match_Interface* new_instance)
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Setting the decoded content matching mechanism of a non-decmatch "
      "hexstring template.");
  }
  dec_match = new decmatch_struct;
  dec_match->ref_count = 1;
  dec_match->instance = new_instance;
}

void* HEXSTRING_template::get_decmatch_dec_res() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the decoding result of a non-decmatch hexstring "
      "template.");
  }
  return dec_match->instance->get_dec_res();
}

const TTCN_Typedescriptor_t* HEXSTRING_template::get_decmatch_type_descr() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the decoded type's descriptor in a non-decmatch "
      "hexstring template.");
  }
  return dec_match->instance->get_type_descr();
}

void HEXSTRING_template::log() const
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
      if (pattern < 16)
        TTCN_Logger::log_hex(pattern);
      else if (pattern == 16)
        TTCN_Logger::log_char('?');
      else if (pattern == 17)
        TTCN_Logger::log_char('*');
      else
        TTCN_Logger::log_event_str("<unknown>");
    }
    TTCN_Logger::log_event_str("'H");
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

void HEXSTRING_template::log_match(const HEXSTRING& match_value,
                                   boolean /* legacy */) const
{
  if (TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()
    && TTCN_Logger::get_logmatch_buffer_len() != 0) {
    TTCN_Logger::print_logmatch_buffer();
    TTCN_Logger::log_event_str(" := ");
  }
  match_value.log();
  TTCN_Logger::log_event_str(" with ");
  log();
  if (match(match_value))
    TTCN_Logger::log_event_str(" matched");
  else
    TTCN_Logger::log_event_str(" unmatched");
}

void HEXSTRING_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE|Module_Param::BC_LIST, "hexstring template");
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
    HEXSTRING_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Hexstring:
    *this = HEXSTRING(mp->get_string_size(), (unsigned char*)mp->get_string_data());
    break;
  case Module_Param::MP_Hexstring_Template:
    *this = HEXSTRING_template(mp->get_string_size(), (unsigned char*)mp->get_string_data());
    break;
  case Module_Param::MP_Expression:
    if (mp->get_expr_type() == Module_Param::EXPR_CONCATENATE) {
      HEXSTRING operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 + operand2;
    }
    else {
      param.expr_type_error("a bitstring");
    }
    break;
  default:
    param.type_error("hexstring template");
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
Module_Param* HEXSTRING_template::get_param(Module_Param_Name& param_name) const
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
    mp = new Module_Param_Hexstring_Template(pattern_value->n_elements, val_cpy);
    break; }
  case DECODE_MATCH:
    TTCN_error("Referencing a decoded content matching template is not supported.");
    break;
  default:
    TTCN_error("Referencing an uninitialized/unsupported hexstring template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  mp->set_length_restriction(get_length_range());
  return mp;
}
#endif

void HEXSTRING_template::encode_text(Text_Buf& text_buf) const
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
      "hexstring template.");
  }
}

void HEXSTRING_template::decode_text(Text_Buf& text_buf)
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
    value_list.list_value = new HEXSTRING_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  case STRING_PATTERN: {
    unsigned int n_elements = text_buf.pull_int().get_val();
    pattern_value
      = (hexstring_pattern_struct*) Malloc(sizeof(hexstring_pattern_struct) + n_elements - 1);
    pattern_value->ref_count = 1;
    pattern_value->n_elements = n_elements;
    text_buf.pull_raw(n_elements, pattern_value->elements_ptr);
    break;
  }
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was "
      "received for a hexstring template.");
  }
}

boolean HEXSTRING_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean HEXSTRING_template::match_omit(boolean legacy /* = FALSE */) const
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
      for (unsigned int i = 0; i < value_list.n_values; i++)
        if (value_list.list_value[i].match_omit()) return template_selection
          == VALUE_LIST;
      return template_selection == COMPLEMENTED_LIST;
    }
    // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
void HEXSTRING_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "hexstring");
}
#endif
