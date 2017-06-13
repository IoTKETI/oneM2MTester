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
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "Universal_charstring.hh"

#include "../common/dbgnew.hh"
#include "../common/memory.h"
#include "../common/pattern.hh"
#include "../common/Quadruple.hh"
#include "../common/UnicharPattern.hh"
#include "Integer.hh"
#include "Octetstring.hh"
#include "String_struct.hh"
#include "Param_Types.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Encdec.hh"
#include "Addfunc.hh" // for unichar2int
#include "TEXT.hh"
#include "Optional.hh"
#include <string>
#include <iostream>
#include <stdint.h>

#define ERRMSG_BUFSIZE 500

// global function for universal_char comparison

boolean operator==(const universal_char& left_value,
  const universal_char& right_value)
{
  return left_value.uc_group == right_value.uc_group &&
    left_value.uc_plane == right_value.uc_plane &&
    left_value.uc_row == right_value.uc_row &&
    left_value.uc_cell == right_value.uc_cell;
}

boolean operator<(const universal_char& left_value,
  const universal_char& right_value)
{
  if (left_value.uc_group < right_value.uc_group) return TRUE;
  else if (left_value.uc_group == right_value.uc_group) {
    if (left_value.uc_plane < right_value.uc_plane) return TRUE;
    else if (left_value.uc_plane == right_value.uc_plane) {
      if (left_value.uc_row < right_value.uc_row) return TRUE;
      else if (left_value.uc_row == right_value.uc_row) {
	if (left_value.uc_cell < right_value.uc_cell) return TRUE;
	else return FALSE;
      } else return FALSE;
    } else return FALSE;
  } else return FALSE;
}

/** The amount of memory needed for a string containing n characters. */
#define MEMORY_SIZE(n) (sizeof(universal_charstring_struct) + \
 ((n) - 1) * sizeof(universal_char))

// member functions of class UNIVERSAL_CHARSTRING

/** Allocate space for n characters.
 *
 * @param n_uchars number of characters needed
 *
 * @pre n_uchars >= 0
 *
 * If n_uchars is 0, no memory is allocated and a reference to the
 * "empty string" is used instead.
 *
 * Otherwise, space for n_uchars is allocated (no terminating null).
 *
 * @note If the string is not empty, this will leak memory.
 * Probably clean_up() should have been called before.
 */
void UNIVERSAL_CHARSTRING::init_struct(int n_uchars)
{
  if (n_uchars < 0) {
    val_ptr = NULL;
    TTCN_error("Initializing a universal charstring with a negative length.");
  } else if (n_uchars == 0) {
    /* This will represent the empty strings so they won't need allocated
     * memory, this delays the memory allocation until it is really needed.
     */
    static universal_charstring_struct empty_string =
      { 1, 0, { { '\0', '\0', '\0', '\0' } } };
    val_ptr = &empty_string;
    empty_string.ref_count++;
  } else {
    val_ptr = (universal_charstring_struct*)Malloc(MEMORY_SIZE(n_uchars));
    val_ptr->ref_count = 1;
    val_ptr->n_uchars = n_uchars;
  }
}

void UNIVERSAL_CHARSTRING::copy_value()
{
  if (val_ptr == NULL || val_ptr->n_uchars <= 0)
    TTCN_error("Internal error: Invalid internal data structure when copying "
      "the memory area of a universal charstring value.");
  if (val_ptr->ref_count > 1) {
    universal_charstring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_uchars);
    memcpy(val_ptr->uchars_ptr, old_ptr->uchars_ptr, old_ptr->n_uchars *
       sizeof(universal_char));
  }
}

void UNIVERSAL_CHARSTRING::clean_up()
{
  if (val_ptr != NULL) {
    if (val_ptr->ref_count > 1) val_ptr->ref_count--;
    else if (val_ptr->ref_count == 1) Free(val_ptr);
    else TTCN_error("Internal error: Invalid reference counter in a universal "
      "charstring value.");
    val_ptr = NULL;
  } else
    cstr.clean_up();
}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING(int n_uchars, boolean cstring)
: val_ptr(NULL), cstr(cstring ? n_uchars : 0), charstring(cstring)
{
  if (!charstring)
    init_struct(n_uchars);
}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING()
: val_ptr(NULL), cstr(0), charstring(FALSE)
{}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING(unsigned char uc_group,
  unsigned char uc_plane, unsigned char uc_row, unsigned char uc_cell)
: charstring(FALSE)
{
  init_struct(1);
  val_ptr->uchars_ptr[0].uc_group = uc_group;
  val_ptr->uchars_ptr[0].uc_plane = uc_plane;
  val_ptr->uchars_ptr[0].uc_row = uc_row;
  val_ptr->uchars_ptr[0].uc_cell = uc_cell;
}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING
  (const universal_char& other_value)
: cstr(0), charstring(FALSE)
{
  init_struct(1);
  val_ptr->uchars_ptr[0] = other_value;
}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING(int n_uchars,
  const universal_char *uchars_ptr)
: cstr(0), charstring(FALSE)
{
  init_struct(n_uchars);
  memcpy(val_ptr->uchars_ptr, uchars_ptr, n_uchars * sizeof(universal_char));
}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING(const char *chars_ptr)
: val_ptr(NULL), cstr(chars_ptr), charstring(TRUE)
{
}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING(int n_chars,
  const char *chars_ptr)
: val_ptr(NULL), cstr(n_chars, chars_ptr), charstring(TRUE)
{
}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING(const CHARSTRING& other_value)
: val_ptr(NULL), cstr(other_value), charstring(TRUE)
{
}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING
  (const CHARSTRING_ELEMENT& other_value)
: val_ptr(NULL), cstr(other_value), charstring(TRUE)
{
}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING
  (const UNIVERSAL_CHARSTRING& other_value)
: Base_Type(other_value), charstring(other_value.charstring)
{
  other_value.must_bound("Copying an unbound universal charstring value.");
  if (other_value.charstring) {
    cstr = other_value.cstr;
    val_ptr = NULL;
  } else {
    val_ptr = other_value.val_ptr;
    val_ptr->ref_count++;
    cstr.init_struct(0);
  }
}

UNIVERSAL_CHARSTRING::UNIVERSAL_CHARSTRING
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
: charstring(other_value.get_uchar().is_char())
{
  other_value.must_bound("Initialization of a universal charstring with an "
    "unbound universal charstring element.");
  if (charstring) {
    cstr = CHARSTRING((const char)(other_value.get_uchar().uc_cell));
    val_ptr = NULL;
  } else {
    init_struct(1);
    val_ptr->uchars_ptr[0] = other_value.get_uchar();
  }
}

UNIVERSAL_CHARSTRING::~UNIVERSAL_CHARSTRING()
{
  clean_up();
}


UNIVERSAL_CHARSTRING& UNIVERSAL_CHARSTRING::operator=
  (const universal_char& other_value)
{
  clean_up();
  if (other_value.is_char()) {
    cstr = CHARSTRING(other_value.uc_cell);
    charstring = TRUE;
  } else {
    charstring = FALSE;
    init_struct(1);
    val_ptr->uchars_ptr[0] = other_value;
    cstr.init_struct(0);
  }
  return *this;
}

UNIVERSAL_CHARSTRING& UNIVERSAL_CHARSTRING::operator=
  (const char* other_value)
{
  if (!charstring) {
    clean_up();
    charstring = TRUE;
  }
  cstr = other_value;
  return *this;
}

UNIVERSAL_CHARSTRING& UNIVERSAL_CHARSTRING::operator=
  (const CHARSTRING& other_value)
{
  if (!charstring) {
    clean_up();
    charstring = TRUE;
  }
  cstr = other_value;
  return *this;
}

UNIVERSAL_CHARSTRING& UNIVERSAL_CHARSTRING::operator=
  (const CHARSTRING_ELEMENT& other_value)
{
  if (!charstring) {
    clean_up();
    charstring = TRUE;
  }
  cstr = other_value;
  return *this;
}

UNIVERSAL_CHARSTRING& UNIVERSAL_CHARSTRING::operator=
  (const UNIVERSAL_CHARSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound universal charstring "
    "value.");
  if (&other_value != this) {
    clean_up();
    if (other_value.charstring) {
      cstr = other_value.cstr;
    } else {
      val_ptr = other_value.val_ptr;
      val_ptr->ref_count++;
      cstr.clean_up();
      cstr.init_struct(0);
    }
    charstring = other_value.charstring;
  }
  return *this;
}

UNIVERSAL_CHARSTRING& UNIVERSAL_CHARSTRING::operator=
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound universal charstring "
    "element to a universal charstring.");
  if (other_value.str_val.charstring) {
    char c = other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
    clean_up();
    cstr = CHARSTRING(c);
    charstring = TRUE;
  } else {
    universal_char uchar_value = other_value.get_uchar();
    clean_up();
    init_struct(1);
    val_ptr->uchars_ptr[0] = uchar_value;
    charstring = FALSE;
  }
  return *this;
}

boolean UNIVERSAL_CHARSTRING::operator==
  (const universal_char& other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring value.");
  if (charstring) {
    return cstr.lengthof() == 1 && other_value.uc_group == 0
      && other_value.uc_plane == 0 && other_value.uc_row == 0
      && other_value.uc_cell == (cbyte)cstr.val_ptr->chars_ptr[0];
  }
  if (val_ptr->n_uchars != 1) return FALSE;
  else return val_ptr->uchars_ptr[0] == other_value;
}

boolean UNIVERSAL_CHARSTRING::operator==(const char* other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring value.");
  if (charstring)
    return cstr == other_value;
  else {
    int n_chars;
    if (other_value == NULL) n_chars = 0;
    else n_chars = strlen(other_value);
    if (val_ptr->n_uchars != n_chars) return FALSE;
    for (int i = 0; i < n_chars; i++) {
      if (val_ptr->uchars_ptr[i].uc_group != 0 ||
        val_ptr->uchars_ptr[i].uc_plane != 0 ||
        val_ptr->uchars_ptr[i].uc_row != 0 ||
        val_ptr->uchars_ptr[i].uc_cell != (cbyte)other_value[i]) return FALSE;
    }
  }
  return TRUE;
}

boolean UNIVERSAL_CHARSTRING::operator==
  (const CHARSTRING& other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring value.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "charstring value.");
  if (charstring)
    return cstr == other_value;
  if (val_ptr->n_uchars != other_value.val_ptr->n_chars) return FALSE;
  for (int i = 0; i < val_ptr->n_uchars; i++) {
    if (val_ptr->uchars_ptr[i].uc_group != 0 ||
      val_ptr->uchars_ptr[i].uc_plane != 0 ||
      val_ptr->uchars_ptr[i].uc_row != 0 ||
      val_ptr->uchars_ptr[i].uc_cell !=(cbyte)other_value.val_ptr->chars_ptr[i])
        return FALSE;
  }
  return TRUE;
}

boolean UNIVERSAL_CHARSTRING::operator==
  (const CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring value.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "charstring element.");
  if (charstring)
    return cstr == other_value;
  if (val_ptr->n_uchars != 1) return FALSE;
  else return val_ptr->uchars_ptr[0].uc_group == 0 &&
    val_ptr->uchars_ptr[0].uc_plane == 0 &&
    val_ptr->uchars_ptr[0].uc_row == 0 &&
    val_ptr->uchars_ptr[0].uc_cell == (cbyte)other_value.get_char();
}

boolean UNIVERSAL_CHARSTRING::operator==
  (const UNIVERSAL_CHARSTRING& other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring value.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring value.");
  if (charstring)
    return cstr == other_value;
  else if (other_value.charstring)
    return other_value.cstr == *this;
  if (val_ptr->n_uchars != other_value.val_ptr->n_uchars) return FALSE;
  for (int i = 0; i < val_ptr->n_uchars; i++) {
    if (val_ptr->uchars_ptr[i] != other_value.val_ptr->uchars_ptr[i])
      return FALSE;
  }
  return TRUE;
}

boolean UNIVERSAL_CHARSTRING::operator==
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring value.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring element.");
  if (charstring)
    return cstr == other_value;
  if (val_ptr->n_uchars != 1) return FALSE;
  else return val_ptr->uchars_ptr[0] == other_value.get_uchar();
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator+
  (const universal_char& other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring value.");
  if (charstring) {
    if (other_value.is_char()) {
      UNIVERSAL_CHARSTRING ret_val(cstr.lengthof() + 1, TRUE);
      memcpy(ret_val.cstr.val_ptr->chars_ptr, cstr.val_ptr->chars_ptr,
        cstr.val_ptr->n_chars);
      ret_val.cstr.val_ptr->chars_ptr[cstr.val_ptr->n_chars] =
        other_value.uc_cell;
      return ret_val;
    } else {
      UNIVERSAL_CHARSTRING ret_val(cstr.lengthof() + 1);
      for (int i = 0; i < cstr.val_ptr->n_chars; ++i) {
        universal_char& uc = ret_val.val_ptr->uchars_ptr[i];
        uc.uc_group = uc.uc_plane = uc.uc_row = 0;
        uc.uc_cell = cstr.val_ptr->chars_ptr[i];
      }
      ret_val.val_ptr->uchars_ptr[cstr.val_ptr->n_chars] = other_value;
      return ret_val;
    }
  }
  UNIVERSAL_CHARSTRING ret_val(val_ptr->n_uchars + 1);
  memcpy(ret_val.val_ptr->uchars_ptr, val_ptr->uchars_ptr,
    val_ptr->n_uchars * sizeof(universal_char));
  ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars] = other_value;
  return ret_val;
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator+
  (const char* other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring value.");
  int other_len;
  if (other_value == NULL) other_len = 0;
  else other_len = strlen(other_value);
  if (other_len == 0) return *this;
  if (charstring) {
    UNIVERSAL_CHARSTRING ret_val(cstr.lengthof() + other_len, TRUE);
    memcpy(ret_val.cstr.val_ptr->chars_ptr, cstr.val_ptr->chars_ptr,
      cstr.val_ptr->n_chars);
    memcpy(ret_val.cstr.val_ptr->chars_ptr + cstr.val_ptr->n_chars,
      other_value, other_len);
    return ret_val;
  }
  UNIVERSAL_CHARSTRING ret_val(val_ptr->n_uchars + other_len);
  memcpy(ret_val.val_ptr->uchars_ptr, val_ptr->uchars_ptr,
    val_ptr->n_uchars * sizeof(universal_char));
  for (int i = 0; i < other_len; i++) {
    ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars + i].uc_group = 0;
    ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars + i].uc_plane = 0;
    ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars + i].uc_row = 0;
    ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars + i].uc_cell = other_value[i];
  }
  return ret_val;
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator+
  (const CHARSTRING& other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring value.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "charstring value.");
  if (other_value.val_ptr->n_chars == 0) return *this;
  if (charstring) {
    UNIVERSAL_CHARSTRING ret_val(cstr.lengthof() + other_value.val_ptr->n_chars,
      TRUE);
    memcpy(ret_val.cstr.val_ptr->chars_ptr, cstr.val_ptr->chars_ptr,
      cstr.val_ptr->n_chars);
    memcpy(ret_val.cstr.val_ptr->chars_ptr + cstr.val_ptr->n_chars,
      other_value.val_ptr->chars_ptr, other_value.val_ptr->n_chars);
    return ret_val;
  }
  UNIVERSAL_CHARSTRING ret_val(val_ptr->n_uchars +
    other_value.val_ptr->n_chars);
  memcpy(ret_val.val_ptr->uchars_ptr, val_ptr->uchars_ptr,
    val_ptr->n_uchars * sizeof(universal_char));
  for (int i = 0; i < other_value.val_ptr->n_chars; i++) {
    ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars + i].uc_group = 0;
    ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars + i].uc_plane = 0;
    ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars + i].uc_row = 0;
    ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars + i].uc_cell =
      other_value.val_ptr->chars_ptr[i];
  }
  return ret_val;
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator+
  (const CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring value.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "charstring element.");
  if (charstring) {
    UNIVERSAL_CHARSTRING ret_val(cstr.lengthof() + 1, TRUE);
    memcpy(ret_val.cstr.val_ptr->chars_ptr, cstr.val_ptr->chars_ptr,
      cstr.val_ptr->n_chars);
    ret_val.cstr.val_ptr->chars_ptr[cstr.val_ptr->n_chars] =
      other_value.get_char();
    return ret_val;
  }
  UNIVERSAL_CHARSTRING ret_val(val_ptr->n_uchars + 1);
  memcpy(ret_val.val_ptr->uchars_ptr, val_ptr->uchars_ptr,
    val_ptr->n_uchars * sizeof(universal_char));
  ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars].uc_group = 0;
  ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars].uc_plane = 0;
  ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars].uc_row = 0;
  ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars].uc_cell =
    other_value.get_char();
  return ret_val;
}

#ifdef TITAN_RUNTIME_2
UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator+(
  const OPTIONAL<CHARSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const CHARSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of universal charstring "
    "concatenation.");
}
#endif

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator+
  (const UNIVERSAL_CHARSTRING& other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring value.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring value.");
  if (charstring) {
    if (cstr.val_ptr->n_chars == 0)
      return other_value;
    if (other_value.charstring) {
      if (other_value.cstr.val_ptr->n_chars == 0)
        return *this;
      UNIVERSAL_CHARSTRING ret_val(cstr.val_ptr->n_chars +
        other_value.cstr.val_ptr->n_chars, TRUE);
      memcpy(ret_val.cstr.val_ptr->chars_ptr, cstr.val_ptr->chars_ptr,
        cstr.val_ptr->n_chars);
      memcpy(ret_val.cstr.val_ptr->chars_ptr + cstr.val_ptr->n_chars,
        other_value.cstr.val_ptr->chars_ptr, other_value.cstr.val_ptr->n_chars);
      return ret_val;
    } else {
      if (other_value.val_ptr->n_uchars == 0)
        return *this;
      UNIVERSAL_CHARSTRING ret_val(cstr.val_ptr->n_chars +
        other_value.val_ptr->n_uchars);
      for (int i = 0; i < cstr.val_ptr->n_chars; i++) {
        ret_val.val_ptr->uchars_ptr[i].uc_group = 0;
        ret_val.val_ptr->uchars_ptr[i].uc_plane = 0;
        ret_val.val_ptr->uchars_ptr[i].uc_row = 0;
        ret_val.val_ptr->uchars_ptr[i].uc_cell = cstr.val_ptr->chars_ptr[i];
      }
      memcpy(ret_val.val_ptr->uchars_ptr + cstr.val_ptr->n_chars,
        other_value.val_ptr->uchars_ptr, other_value.val_ptr->n_uchars * sizeof(universal_char));
      return ret_val;
    }
  } else {
    if (other_value.charstring) {
      UNIVERSAL_CHARSTRING ret_val(val_ptr->n_uchars + other_value.cstr.val_ptr->n_chars);
      memcpy(ret_val.val_ptr->uchars_ptr, val_ptr->uchars_ptr, val_ptr->n_uchars * sizeof(universal_char));
      for (int i = val_ptr->n_uchars; i < val_ptr->n_uchars + other_value.cstr.val_ptr->n_chars; i++) {
        ret_val.val_ptr->uchars_ptr[i].uc_group = 0;
        ret_val.val_ptr->uchars_ptr[i].uc_plane = 0;
        ret_val.val_ptr->uchars_ptr[i].uc_row = 0;
        ret_val.val_ptr->uchars_ptr[i].uc_cell = other_value.cstr.val_ptr->chars_ptr[i-val_ptr->n_uchars];
      }
      return ret_val;
    } else {
      if (val_ptr->n_uchars == 0) return other_value;
      if (other_value.val_ptr->n_uchars == 0) return *this;
      UNIVERSAL_CHARSTRING ret_val(val_ptr->n_uchars +
        other_value.val_ptr->n_uchars);
      memcpy(ret_val.val_ptr->uchars_ptr, val_ptr->uchars_ptr,
        val_ptr->n_uchars * sizeof(universal_char));
      memcpy(ret_val.val_ptr->uchars_ptr + val_ptr->n_uchars,
        other_value.val_ptr->uchars_ptr,
        other_value.val_ptr->n_uchars * sizeof(universal_char));
      return ret_val;
    }
  }
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator+
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring value.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring element.");
  boolean other_ischar = other_value.str_val.charstring;
  if (charstring) {
    if (other_ischar) {
      UNIVERSAL_CHARSTRING ret_val(cstr.val_ptr->n_chars + 1, TRUE);
      memcpy(ret_val.cstr.val_ptr->chars_ptr, cstr.val_ptr->chars_ptr,
        cstr.val_ptr->n_chars);
      ret_val.cstr.val_ptr->chars_ptr[cstr.val_ptr->n_chars] =
        other_value.get_uchar().uc_cell;
      return ret_val;
    }
    UNIVERSAL_CHARSTRING ret_val(cstr.val_ptr->n_chars + 1);
    for (int i = 0; i < cstr.val_ptr->n_chars; i++) {
      ret_val.val_ptr->uchars_ptr[i].uc_group = 0;
      ret_val.val_ptr->uchars_ptr[i].uc_plane = 0;
      ret_val.val_ptr->uchars_ptr[i].uc_row = 0;
      ret_val.val_ptr->uchars_ptr[i].uc_cell = cstr.val_ptr->chars_ptr[i];
    }
    ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars] = other_value.get_uchar();
    return ret_val;
  } else {
    UNIVERSAL_CHARSTRING ret_val(val_ptr->n_uchars + 1);
    memcpy(ret_val.val_ptr->uchars_ptr, val_ptr->uchars_ptr,
      val_ptr->n_uchars * sizeof(universal_char));
    if (other_ischar) {
      universal_char& uc = ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars];
      uc.uc_group = uc.uc_plane = uc.uc_row = 0;
      uc.uc_cell = other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
    } else
      ret_val.val_ptr->uchars_ptr[val_ptr->n_uchars] = other_value.get_uchar();
    return ret_val;
  }
}

#ifdef TITAN_RUNTIME_2
UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const UNIVERSAL_CHARSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of universal charstring "
    "concatenation.");
}
#endif

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator<<=
  (int rotate_count) const
{
  must_bound("The left operand of rotate left operator is an unbound "
    "universal charstring value.");

  if (charstring)
    return cstr <<= rotate_count;

  if (val_ptr->n_uchars == 0) return *this;
  if (rotate_count >= 0) {
    rotate_count %= val_ptr->n_uchars;
    if (rotate_count == 0) return *this;
    UNIVERSAL_CHARSTRING ret_val(val_ptr->n_uchars);
    memcpy(ret_val.val_ptr->uchars_ptr, val_ptr->uchars_ptr + rotate_count,
      (val_ptr->n_uchars - rotate_count) * sizeof(universal_char));
    memcpy(ret_val.val_ptr->uchars_ptr + val_ptr->n_uchars - rotate_count,
       val_ptr->uchars_ptr, rotate_count * sizeof(universal_char));
    return ret_val;
  } else return *this >>= (-rotate_count);
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator<<=
  (const INTEGER& rotate_count) const
{
  rotate_count.must_bound("The right operand of rotate left operator is an "
    "unbound integer value.");
  return *this <<= (int)rotate_count;
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator>>=
  (int rotate_count) const
{
  must_bound("The left operand of rotate right operator is an unbound "
    "universal charstring value.");

  if (charstring)
      return cstr >>= rotate_count;

  if (val_ptr->n_uchars == 0) return *this;
  if (rotate_count >= 0) {
    rotate_count %= val_ptr->n_uchars;
    if (rotate_count == 0) return *this;
    UNIVERSAL_CHARSTRING ret_val(val_ptr->n_uchars);
    memcpy(ret_val.val_ptr->uchars_ptr, val_ptr->uchars_ptr +
      val_ptr->n_uchars - rotate_count, rotate_count * sizeof(universal_char));
    memcpy(ret_val.val_ptr->uchars_ptr + rotate_count, val_ptr->uchars_ptr,
      (val_ptr->n_uchars - rotate_count) * sizeof(universal_char));
    return ret_val;
  } else return *this <<= (-rotate_count);

}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::operator>>=
  (const INTEGER& rotate_count) const
{
  rotate_count.must_bound("The right operand of rotate right operator is an "
    "unbound integer value.");
  return *this >>= (int)rotate_count;
}


UNIVERSAL_CHARSTRING_ELEMENT UNIVERSAL_CHARSTRING::operator[]
  (int index_value)
{
  if (!charstring && val_ptr == NULL && index_value == 0) {
    init_struct(1);
    return UNIVERSAL_CHARSTRING_ELEMENT(FALSE, *this, 0);
  } else {
    must_bound("Accessing an element of an unbound universal charstring "
      "value.");
    if (index_value < 0) TTCN_error("Accessing a universal charstring element "
        "using a negative index (%d).", index_value);
    const int n_uchars =
      charstring ? cstr.val_ptr->n_chars : val_ptr->n_uchars;
    if (index_value > n_uchars) TTCN_error("Index overflow when accessing a "
      "universal charstring element: The index is %d, but the string has only "
      "%d characters.", index_value, n_uchars);
    if (index_value == n_uchars) {
      if (charstring)
        (void)cstr[index_value]; // invoked for side-effect only (incr. length)
      else {
        if (val_ptr->ref_count == 1) {
          val_ptr = (universal_charstring_struct*)
          Realloc(val_ptr, MEMORY_SIZE(n_uchars + 1));
          val_ptr->n_uchars++;
        } else {
          universal_charstring_struct *old_ptr = val_ptr;
          old_ptr->ref_count--;
          init_struct(n_uchars + 1);
          memcpy(val_ptr->uchars_ptr, old_ptr->uchars_ptr,
            n_uchars * sizeof(universal_char));
        }
      }
      return UNIVERSAL_CHARSTRING_ELEMENT(FALSE, *this, index_value);
    } else return UNIVERSAL_CHARSTRING_ELEMENT(TRUE, *this, index_value);
  }
}

UNIVERSAL_CHARSTRING_ELEMENT UNIVERSAL_CHARSTRING::operator[]
  (const INTEGER& index_value)
{
  index_value.must_bound("Indexing a universal charstring value with an "
                         "unbound integer value.");
  return (*this)[(int)index_value];
}

const UNIVERSAL_CHARSTRING_ELEMENT UNIVERSAL_CHARSTRING::operator[]
  (int index_value) const
{
  must_bound("Accessing an element of an unbound universal charstring value.");
  const int size = charstring ? cstr.val_ptr->n_chars : val_ptr->n_uchars;
  if (index_value < 0) TTCN_error("Accessing a universal charstring element "
    "using a negative index (%d).", index_value);
  else if (index_value >= size) TTCN_error("Index overflow when "
    "accessing a universal charstring element: The index is %d, but the "
    "string has only %d characters.", index_value, size);
  return UNIVERSAL_CHARSTRING_ELEMENT(TRUE,
    const_cast<UNIVERSAL_CHARSTRING&>(*this), index_value);
}

const UNIVERSAL_CHARSTRING_ELEMENT UNIVERSAL_CHARSTRING::operator[]
  (const INTEGER& index_value) const
{
  index_value.must_bound("Indexing a universal charstring value with an "
    "unbound integer value.");
  return (*this)[(int)index_value];
}


UNIVERSAL_CHARSTRING::operator const universal_char*() const
{
  must_bound("Casting an unbound universal charstring value to const "
    "universal_char*.");
  if (charstring)
    const_cast<UNIVERSAL_CHARSTRING&>(*this).convert_cstr_to_uni();
  return val_ptr->uchars_ptr;
}

int UNIVERSAL_CHARSTRING::lengthof() const
{
  must_bound("Performing lengthof operation on an unbound universal charstring "
    "value.");
  if (charstring)
    return cstr.val_ptr->n_chars;
  return val_ptr->n_uchars;
}

char* UNIVERSAL_CHARSTRING::convert_to_regexp_form() const {
  must_bound("Performing pattern conversion operation on an unbound"
    "universal charstring value.");
  int size = (charstring ? cstr.val_ptr->n_chars : val_ptr->n_uchars) * 8 + 1;
  char* res = static_cast<char*>(Malloc(size));
  char* ptr = res;
  res[size-1] = '\0';
  Quad q;
  if (charstring)
    for (int i = 0; i < cstr.val_ptr->n_chars; i++, ptr += 8) {
      q.set(0, 0, 0, cstr.val_ptr->chars_ptr[i]);
      Quad::get_hexrepr(q, ptr);
    }
  else
    for (int i = 0; i < val_ptr->n_uchars; i++, ptr += 8) {
      q.set(val_ptr->uchars_ptr[i].uc_group, val_ptr->uchars_ptr[i].uc_plane,
        val_ptr->uchars_ptr[i].uc_row, val_ptr->uchars_ptr[i].uc_cell);
      Quad::get_hexrepr(q, ptr);
    }
  return res;
}

static inline boolean is_printable(const universal_char& uchar)
{
  return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
    TTCN_Logger::is_printable(uchar.uc_cell);
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::extract_matched_section(int start, int end) const
{
  // the indexes refer to the string's regexp form, which contains 8 characters
  // for every universal character in the original string
  start /= 8;
  end /= 8;
  if (charstring) {
    return UNIVERSAL_CHARSTRING(end - start, cstr.val_ptr->chars_ptr + start);
  }
  return UNIVERSAL_CHARSTRING(end - start, val_ptr->uchars_ptr + start);
}

CHARSTRING UNIVERSAL_CHARSTRING::get_stringRepr_for_pattern() const {
  must_bound("Performing pattern conversion operation on an unbound"
    "universal charstring value.");
  CHARSTRING ret_val("");
  if (charstring)
    for (int i = 0; i < cstr.val_ptr->n_chars; i++) {
      const char& chr = cstr.val_ptr->chars_ptr[i];
      if (TTCN_Logger::is_printable(chr))
        ret_val += chr;
      else {
        ret_val += "\\q{0,0,0,";
        ret_val += int2str(chr);
        ret_val += "}";
      }
    }
  else
    for (int i = 0; i < val_ptr->n_uchars; i++) {
      const universal_char& uchar = val_ptr->uchars_ptr[i];
      if (is_printable(uchar))
        ret_val += uchar.uc_cell;
      else {
        ret_val += "\\q{";
        ret_val += int2str(uchar.uc_group);
        ret_val += ",";
        ret_val += int2str(uchar.uc_plane);
        ret_val += ",";
        ret_val += int2str(uchar.uc_row);
        ret_val += ",";
        ret_val += int2str(uchar.uc_cell);
        ret_val += "}";
      }
    }
  return ret_val;
}

void UNIVERSAL_CHARSTRING::convert_cstr_to_uni() {
  init_struct(cstr.lengthof());
  for (int i = 0; i < cstr.val_ptr->n_chars; i++) {
    val_ptr->uchars_ptr[i].uc_group = 0;
    val_ptr->uchars_ptr[i].uc_plane = 0;
    val_ptr->uchars_ptr[i].uc_row = 0;
    val_ptr->uchars_ptr[i].uc_cell = cstr.val_ptr->chars_ptr[i];
  }
  charstring = FALSE;
  cstr.clean_up();
  cstr.init_struct(0);
}

void UNIVERSAL_CHARSTRING::dump() const
{
  if (val_ptr != NULL) {
    for (int i = 0; i < val_ptr->n_uchars; i++) {
      const universal_char& uchar = val_ptr->uchars_ptr[i];
      std::wcout << "uchar[" << i << "] = " << "("
        << uchar.uc_group << "," << uchar.uc_plane << "," << uchar.uc_row << "," << uchar.uc_cell << ")"<< std::endl;
    }
  }
}

void UNIVERSAL_CHARSTRING::log() const
{
  if (charstring) {
    cstr.log();
    return;
  }
  if (val_ptr != NULL) {
    expstring_t buffer = 0;
    enum { INIT, PCHAR, UCHAR } state = INIT;
    for (int i = 0; i < val_ptr->n_uchars; i++) {
      const universal_char& uchar = val_ptr->uchars_ptr[i];
      if (is_printable(uchar)) {
	// the actual character is printable
	switch (state) {
	case UCHAR: // concatenation sign if previous part was not printable
	  buffer = mputstr(buffer, " & ");
	  // no break
	case INIT: // opening "
	  buffer = mputc(buffer, '"');
	  // no break
	case PCHAR: // the character itself
	  TTCN_Logger::log_char_escaped(uchar.uc_cell, buffer);
	  break;
	}
	state = PCHAR;
      } else {
	// the actual character is not printable
	switch (state) {
	case PCHAR: // closing " if previous part was printable
	  buffer = mputc(buffer, '"');
	  // no break
	case UCHAR: // concatenation sign
	  buffer = mputstr(buffer, " & ");
	  // no break
	case INIT: // the character itself
	  buffer = mputprintf(buffer, "char(%u, %u, %u, %u)",
	    uchar.uc_group, uchar.uc_plane, uchar.uc_row, uchar.uc_cell);
	  break;
	}
	state = UCHAR;
      }
    }
    // final steps
    switch (state) {
    case INIT: // the string was empty
      buffer = mputstr(buffer, "\"\"");
      break;
    case PCHAR: // last character was printable -> closing "
      buffer = mputc(buffer, '"');
      break;
    default:
      break;
    }
    TTCN_Logger::log_event_str(buffer);
    Free(buffer);
  } else {
    TTCN_Logger::log_event_unbound();
  }
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING::from_UTF8_buffer(TTCN_Buffer& p_buff)
{
  OCTETSTRING os;
  p_buff.get_string(os);
  if ("UTF-8" == get_stringencoding(os)) {
    UNIVERSAL_CHARSTRING ret;
    ret.decode_utf8(p_buff.get_len(), p_buff.get_data());
    return ret;
  } else {
    return UNIVERSAL_CHARSTRING(p_buff.get_len(), (const char*)p_buff.get_data());
  }
}

boolean UNIVERSAL_CHARSTRING::set_param_internal(Module_Param& param, boolean allow_pattern,
                                                 boolean* is_nocase_pattern) {
  boolean is_pattern = FALSE;
  param.basic_check(Module_Param::BC_VALUE|Module_Param::BC_LIST, "universal charstring value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Charstring: {
    switch (param.get_operation_type()) {
    case Module_Param::OT_ASSIGN:
      clean_up();
      // no break
    case Module_Param::OT_CONCAT: {
      TTCN_Buffer buff;
      buff.put_s(mp->get_string_size(), (unsigned char*)mp->get_string_data());
      if (is_bound()) {
        *this = *this + from_UTF8_buffer(buff);
      } else {
        *this = from_UTF8_buffer(buff);
      }
      break; }
    default:
      TTCN_error("Internal error: UNIVERSAL_CHARSTRING::set_param()");
    }
    break; }
  case Module_Param::MP_Universal_Charstring: {
    switch (param.get_operation_type()) {
    case Module_Param::OT_ASSIGN:
      clean_up();
      // no break
    case Module_Param::OT_CONCAT:
      if (is_bound()) {
        *this = *this + UNIVERSAL_CHARSTRING(mp->get_string_size(), (universal_char*)mp->get_string_data());
      } else {
        *this = UNIVERSAL_CHARSTRING(mp->get_string_size(), (universal_char*)mp->get_string_data());
      }
      break;
    default:
      TTCN_error("Internal error: UNIVERSAL_CHARSTRING::set_param()");
    }
    break; }
  case Module_Param::MP_Expression:
    if (mp->get_expr_type() == Module_Param::EXPR_CONCATENATE) {
      UNIVERSAL_CHARSTRING operand1, operand2;
      is_pattern = operand1.set_param_internal(*mp->get_operand1(), allow_pattern,
        is_nocase_pattern);
      operand2.set_param(*mp->get_operand2());
      if (param.get_operation_type() == Module_Param::OT_CONCAT) {
        *this = *this + operand1 + operand2;
      }
      else {
        *this = operand1 + operand2;
      }
    }
    else {
      param.expr_type_error("a universal charstring");
    }
    break;
  case Module_Param::MP_Pattern:
    if (allow_pattern) {
      *this = CHARSTRING(mp->get_pattern());
      is_pattern = TRUE;
      if (is_nocase_pattern != NULL) {
        *is_nocase_pattern = mp->get_nocase();
      }
      break;
    }
    // else fall through
  default:
    param.type_error("universal charstring value");
  }
  return is_pattern;
}

void UNIVERSAL_CHARSTRING::set_param(Module_Param& param) {
  set_param_internal(param, FALSE);
}

#ifdef TITAN_RUNTIME_2
Module_Param* UNIVERSAL_CHARSTRING::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  if (charstring) {
    return cstr.get_param(param_name);
  }
  universal_char* val_cpy = (universal_char*)Malloc(val_ptr->n_uchars * sizeof(universal_char));
  memcpy(val_cpy, val_ptr->uchars_ptr, val_ptr->n_uchars * sizeof(universal_char));
  return new Module_Param_Universal_Charstring(val_ptr->n_uchars, val_cpy);
}
#endif

void UNIVERSAL_CHARSTRING::encode_text(Text_Buf& text_buf) const
{
  must_bound("Text encoder: Encoding an unbound universal charstring value.");
  if (charstring)
    const_cast<UNIVERSAL_CHARSTRING&>(*this).convert_cstr_to_uni();
  text_buf.push_int(val_ptr->n_uchars);
  for (int i = 0; i < val_ptr->n_uchars; i++) {
    unsigned char buf[4];
    buf[0] = val_ptr->uchars_ptr[i].uc_group;
    buf[1] = val_ptr->uchars_ptr[i].uc_plane;
    buf[2] = val_ptr->uchars_ptr[i].uc_row;
    buf[3] = val_ptr->uchars_ptr[i].uc_cell;
    text_buf.push_raw(4, buf);
  }
}

void UNIVERSAL_CHARSTRING::decode_text(Text_Buf& text_buf)
{
  int n_uchars = text_buf.pull_int().get_val();
  if (n_uchars < 0) TTCN_error("Text decoder: Negative length was received "
    "for a universal charstring.");
  clean_up();
  charstring = FALSE;
  init_struct(n_uchars);
  for (int i = 0; i < n_uchars; i++) {
    unsigned char buf[4];
    text_buf.pull_raw(4, buf);
    val_ptr->uchars_ptr[i].uc_group = buf[0];
    val_ptr->uchars_ptr[i].uc_plane = buf[1];
    val_ptr->uchars_ptr[i].uc_row = buf[2];
    val_ptr->uchars_ptr[i].uc_cell = buf[3];
  }
}

void UNIVERSAL_CHARSTRING::encode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const
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
    unsigned XER_coding=va_arg(pvar, unsigned);
    switch (p_td.asnbasetype) {
    case TTCN_Typedescriptor_t::BMPSTRING:
    case TTCN_Typedescriptor_t::UNIVERSALSTRING:
      XER_coding |= XER_ESCAPE_ENTITIES;
      break;
    default: // nothing to do
      break;
    }
    XER_encode(*p_td.xer,p_buf, XER_coding, 0, 0, 0);
    p_buf.put_c('\n');
    break; }
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-encoding type '%s': ", p_td.name);
    if(!p_td.json)
      TTCN_EncDec_ErrorContext::error_internal
        ("No JSON descriptor available for type '%s'.", p_td.name);
    JSON_Tokenizer tok(va_arg(pvar, int) != 0);
    JSON_encode(p_td, tok);
    p_buf.put_s(tok.get_buffer_length(), (const unsigned char*)tok.get_buffer());
    break; }
  default:
    TTCN_error("Unknown coding method requested to encode type '%s'",
               p_td.name);
  }
  va_end(pvar);
}

void UNIVERSAL_CHARSTRING::decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)
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
  case TTCN_EncDec::CT_XER : {
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
    break; }
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
UNIVERSAL_CHARSTRING::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                                     unsigned p_coding) const
{
  if (charstring)
    const_cast<UNIVERSAL_CHARSTRING&>(*this).convert_cstr_to_uni();
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(!new_tlv) {
    TTCN_Buffer buf;
    switch(p_td.asnbasetype) {
    case TTCN_Typedescriptor_t::TELETEXSTRING:
      buf.put_os(TTCN_TeletexString_2_ISO2022(*this));
      break;
    case TTCN_Typedescriptor_t::VIDEOTEXSTRING:
      buf.put_os(TTCN_VideotexString_2_ISO2022(*this));
      break;
    case TTCN_Typedescriptor_t::GRAPHICSTRING:
      buf.put_os(TTCN_GraphicString_2_ISO2022(*this));
      break;
    case TTCN_Typedescriptor_t::GENERALSTRING:
      buf.put_os(TTCN_GeneralString_2_ISO2022(*this));
      break;
    case TTCN_Typedescriptor_t::UNIVERSALSTRING:
      for(int i=0; i<val_ptr->n_uchars; i++) {
        buf.put_c(val_ptr->uchars_ptr[i].uc_group);
        buf.put_c(val_ptr->uchars_ptr[i].uc_plane);
        buf.put_c(val_ptr->uchars_ptr[i].uc_row);
        buf.put_c(val_ptr->uchars_ptr[i].uc_cell);
      }
      break;
    case TTCN_Typedescriptor_t::BMPSTRING:
      for(int i=0; i<val_ptr->n_uchars; i++) {
        buf.put_c(val_ptr->uchars_ptr[i].uc_row);
        buf.put_c(val_ptr->uchars_ptr[i].uc_cell);
      }
      break;
    case TTCN_Typedescriptor_t::UTF8STRING:
      encode_utf8(buf);
      break;
    default:
      TTCN_EncDec_ErrorContext::error_internal
        ("Missing/wrong basetype info for type '%s'.", p_td.name);
    } // switch
    new_tlv=BER_encode_TLV_OCTETSTRING
      (p_coding, buf.get_read_len(), buf.get_read_data());
  }
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}
int UNIVERSAL_CHARSTRING::TEXT_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff, Limit_Token_List& limit, boolean no_err, boolean /*first_call*/)
{
  int decoded_length = 0;
  int str_len = 0;
  clean_up();
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
  //  never return "not enough bits"
  //  if(buff.get_read_len()<=1 && no_err) return -TTCN_EncDec::ET_LEN_ERR;

  if (p_td.text->select_token) {
    int tl;
    if ((tl = p_td.text->select_token->match_begin(buff)) < 0) {
      if (no_err) return -1;
      else tl = 0;
    }
    str_len = tl;
  }
  // The length restriction needs some more work
/*  else if (   p_td.text->val.parameters
    &&        p_td.text->val.parameters->decoding_params.min_length != -1) {
    str_len = p_td.text->val.parameters->decoding_params.min_length;
  }*/
  else if (p_td.text->end_decode) {
    int tl;
    if ((tl = p_td.text->end_decode->match_first(buff)) < 0) {
      if (no_err) return -1;
      else tl = 0;
    }
    str_len = tl;
  }
  else if (limit.has_token()) {
    int tl;
    if ((tl = limit.match(buff)) < 0) tl = buff.get_read_len() - 1;
    str_len = tl;
  }
  else {
    str_len = buff.get_read_len() - 1;
  }

// only utf8 is supported now.
  decode_utf8(str_len,buff.get_read_data());

  decoded_length += str_len;
  buff.increase_pos(str_len);

// Case conversion is an another study
// and it is locale dependent
/*  if (  p_td.text->val.parameters
    &&  p_td.text->val.parameters->decoding_params.convert != 0) {
    if (p_td.text->val.parameters->decoding_params.convert == 1) {
      for (int a = 0; a < str_len; a++) {
        val_ptr->chars_ptr[a] = (char)toupper(val_ptr->chars_ptr[a]);
      }
    }
    else {
      for (int a = 0; a < str_len; a++) {
        val_ptr->chars_ptr[a] = (char)tolower(val_ptr->chars_ptr[a]);
      }
    }
  }*/
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
  return decoded_length;
}

int UNIVERSAL_CHARSTRING::TEXT_encode(const TTCN_Typedescriptor_t& p_td,
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

// The length restriction and case conversion will be added later
//  if(p_td.text->val.parameters==NULL){

    int base_size=buff.get_len(); // strore the current length of the data 
                                   // in the buffer
    
    encode_utf8(buff);
    
    encoded_length+=buff.get_len()-base_size;  // calculate the number of the
                                               // stored octets

/*  } else {
    int chars_before=0;
    int chars_after=0;
    if(val_ptr->n_chars<p_td.text->val.parameters->coding_params.min_length){
      switch(p_td.text->val.parameters->coding_params.just){
        case -1: //left
          chars_after=p_td.text->
              val.parameters->coding_params.min_length-val_ptr->n_chars;
          break;
        case 0:{  // center
          int pad=p_td.text->
              val.parameters->coding_params.min_length-val_ptr->n_chars;
          chars_after=pad/2;
          chars_before=pad-chars_after;
          break;
          }
        case 1:  // right
        default:
          chars_before=p_td.text->
              val.parameters->coding_params.min_length-val_ptr->n_chars;
          break;
      }
    }
    if(chars_before){
      unsigned char* p=NULL;
      size_t len=chars_before;
      buff.get_end(p,len);
      for(int a=0;a<chars_before;a++) p[a]=(unsigned char)' ';
      buff.increase_length(chars_before);
      encoded_length+=chars_before;
    }

    switch(p_td.text->val.parameters->coding_params.convert){
      case -1:{ //lower_case
        unsigned char* p=NULL;
        size_t len=val_ptr->n_chars;
        buff.get_end(p,len);
        for(int a=0;a<val_ptr->n_chars;a++)
            p[a]=(unsigned char)tolower(val_ptr->chars_ptr[a]);
        buff.increase_length(val_ptr->n_chars);
        break;
        }
      case 0:{  // no conversion
        buff.put_cs(*this);
        break;
        }
      case 1:  // upper_case
      default:
        {
        unsigned char* p=NULL;
        size_t len=val_ptr->n_chars;
        buff.get_end(p,len);
        for(int a=0;a<val_ptr->n_chars;a++)
            p[a]=(unsigned char)toupper(val_ptr->chars_ptr[a]);
        buff.increase_length(val_ptr->n_chars);
        break;
        }
    }
    encoded_length+=val_ptr->n_chars;

    if(chars_after){
      unsigned char* p=NULL;
      size_t len=chars_after;
      buff.get_end(p,len);
      for(int a=0;a<chars_after;a++) p[a]=(unsigned char)' ';
      buff.increase_length(chars_after);
      encoded_length+=chars_after;
    }
  }
*/

  if(p_td.text->end_encode){
    buff.put_cs(*p_td.text->end_encode);
    encoded_length+=p_td.text->end_encode->lengthof();
  }
  return encoded_length;
}

void UNIVERSAL_CHARSTRING::encode_utf8(TTCN_Buffer& buf, boolean addBOM /*= false*/) const
{
  // Add BOM
  if (addBOM) {
    buf.put_c(0xEF);
    buf.put_c(0xBB);
    buf.put_c(0xBF);
  }
  if (charstring) {
    buf.put_s(cstr.val_ptr->n_chars, (unsigned char*)cstr.val_ptr->chars_ptr);
    // put_s avoids the check for boundness in put_cs
  }
  else {
    for(int i=0; i<val_ptr->n_uchars; i++) {
      unsigned char g=val_ptr->uchars_ptr[i].uc_group;
      unsigned char p=val_ptr->uchars_ptr[i].uc_plane;
      unsigned char r=val_ptr->uchars_ptr[i].uc_row;
      unsigned char c=val_ptr->uchars_ptr[i].uc_cell;
      if(g==0x00 && p<=0x1F) {
        if(p==0x00) {
          if(r==0x00 && c<=0x7F) {
            // 1 octet
            buf.put_c(c);
          } // r
          // 2 octets
          else if(r<=0x07) {
            buf.put_c(0xC0|r<<2|c>>6);
            buf.put_c(0x80|(c&0x3F));
          } // r
          // 3 octets
          else {
            buf.put_c(0xE0|r>>4);
            buf.put_c(0x80|(r<<2&0x3C)|c>>6);
            buf.put_c(0x80|(c&0x3F));
          } // r
        } // p
        // 4 octets
        else {
          buf.put_c(0xF0|p>>2);
          buf.put_c(0x80|(p<<4&0x30)|r>>4);
          buf.put_c(0x80|(r<<2&0x3C)|c>>6);
          buf.put_c(0x80|(c&0x3F));
        } // p
      } //g
      // 5 octets
      else if(g<=0x03) {
        buf.put_c(0xF8|g);
        buf.put_c(0x80|p>>2);
        buf.put_c(0x80|(p<<4&0x30)|r>>4);
        buf.put_c(0x80|(r<<2&0x3C)|c>>6);
        buf.put_c(0x80|(c&0x3F));
      } // g
      // 6 octets
      else {
        buf.put_c(0xFC|g>>6);
        buf.put_c(0x80|(g&0x3F));
        buf.put_c(0x80|p>>2);
        buf.put_c(0x80|(p<<4&0x30)|r>>4);
        buf.put_c(0x80|(r<<2&0x3C)|c>>6);
        buf.put_c(0x80|(c&0x3F));
      }
    } // for i
  }
}

void UNIVERSAL_CHARSTRING::encode_utf16(TTCN_Buffer& buf,
                                        CharCoding::CharCodingType expected_coding) const
{
  // add BOM
  boolean isbig = TRUE;
  switch (expected_coding) {
    case CharCoding::UTF16:
    case CharCoding::UTF16BE:
      isbig = TRUE;
      break;
    case CharCoding::UTF16LE:
      isbig = FALSE;
      break;
    default:
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
       "Unexpected coding type for UTF-16 encoding");
      break;
  }
  buf.put_c(isbig ? 0xFE : 0xFF);
  buf.put_c(isbig ? 0xFF : 0xFE);
  
  if (charstring) {
    for (int i = 0; i < cstr.val_ptr->n_chars; ++i) {
      buf.put_c(isbig ? 0 : cstr.val_ptr->chars_ptr[i]);
      buf.put_c(isbig ? cstr.val_ptr->chars_ptr[i] : 0);
    }
  }
  else {
    for(int i=0; i<val_ptr->n_uchars; i++) {
      unsigned char g=val_ptr->uchars_ptr[i].uc_group;
      unsigned char p=val_ptr->uchars_ptr[i].uc_plane;
      unsigned char r=val_ptr->uchars_ptr[i].uc_row;
      unsigned char c=val_ptr->uchars_ptr[i].uc_cell;
      if (g || (0x10 < p)) {
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
          "Any UCS code (0x%02X%02X%02X%02X) to be encoded into UTF-16 "
          "shall not be greater than 0x10FFFF", g, p, r, c);
      }
      else if (0x00 == g && 0x00 ==p && 0xD8 <= r && 0xDF >= r) {
        // Values between 0xD800 and 0xDFFF are specifically reserved for use with UTF-16,
        // and don't have any characters assigned to them.
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
        "Any UCS code (0x%02X%02X) between 0xD800 and 0xDFFF is ill-formed", r,c);
      }
      else if (0x00 == g && 0x00 == p) {
        buf.put_c(isbig ? r : c);
        buf.put_c(isbig ? c : r);
      }
      else if (g || p) { // greater than 0xFFFF it needs surrogates
        uint32_t univc = 0, temp = 0;
        univc = g;
        univc <<= 24;
        temp = p;
        temp <<= 16;
        univc |= temp;
        temp = r;
        temp <<= 8;
        univc |= temp;
        univc |= c; // universal char filled in univc 
        uint16_t W1 = 0xD800;
        uint16_t W2 = 0xDC00;
        uint32_t univcmod = univc - 0x10000;
        uint16_t WH = univcmod >> 10;
        uint16_t WL = univcmod & 0x3ff;
        W1 |= WH;
        W2 |= WL;
        unsigned char uc;
        uc = isbig ? W1 >> 8 : W1;
        buf.put_c(uc);
        uc = isbig ? W1 : W1 >> 8;
        buf.put_c(uc);
        uc = isbig ? W2 >> 8 : W2;
        buf.put_c(uc);
        uc = isbig ? W2 : W2 >> 8;
        buf.put_c(uc);
      }
    }
  }
}

void UNIVERSAL_CHARSTRING::encode_utf32(TTCN_Buffer& buf,
                                        CharCoding::CharCodingType expected_coding) const
{
  boolean isbig = TRUE;
  switch (expected_coding) {
    case CharCoding::UTF32:
    case CharCoding::UTF32BE:
      isbig = TRUE;
      break;
    case CharCoding::UTF32LE:
      isbig = FALSE;
      break;
    default:
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
       "Unexpected coding type for UTF-32 encoding");
      break;
  }
  // add BOM
  buf.put_c(isbig ? 0x00 : 0xFF);
  buf.put_c(isbig ? 0x00 : 0xFE);
  buf.put_c(isbig ? 0xFE : 0x00);
  buf.put_c(isbig ? 0xFF : 0x00);
    
  if (charstring) {
    for (int i = 0; i < cstr.val_ptr->n_chars; ++i) {
      buf.put_c(isbig ? 0 : cstr.val_ptr->chars_ptr[i]);
      buf.put_c(0);
      buf.put_c(0);
      buf.put_c(isbig ? cstr.val_ptr->chars_ptr[i] : 0);
    }
  }
  else {
    for(int i = 0; i < val_ptr->n_uchars; ++i) {
      unsigned char g=val_ptr->uchars_ptr[i].uc_group;
      unsigned char p=val_ptr->uchars_ptr[i].uc_plane;
      unsigned char r=val_ptr->uchars_ptr[i].uc_row;
      unsigned char c=val_ptr->uchars_ptr[i].uc_cell;
      uint32_t DW = g << 8 | p;
      DW <<= 8;
      DW |= r;
      DW <<= 8;
      DW |= c;
      if (0x0000D800 <= DW && 0x0000DFFF >= DW) {
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
        "Any UCS code (0x%08X) between 0x0000D800 and 0x0000DFFF is ill-formed", DW);
      }
      else if (0x0010FFFF < DW) {
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
        "Any UCS code (0x%08X) greater than 0x0010FFFF is ill-formed", DW);
      }
      else {
        buf.put_c(isbig ? g : c);
        buf.put_c(isbig ? p : r);
        buf.put_c(isbig ? r : p);
        buf.put_c(isbig ? c : g);
      }
    }
  }
}

#ifdef TITAN_RUNTIME_2
//virtual
int UNIVERSAL_CHARSTRING::encode_raw(TTCN_Buffer& p_buf) const
{
  size_t len_before = p_buf.get_len();
  encode_utf8(p_buf);
  return p_buf.get_len() - len_before;
}

int UNIVERSAL_CHARSTRING::JSON_encode_negtest_raw(JSON_Tokenizer& p_tok) const
{
  TTCN_Buffer tmp_buf;
  encode_utf8(tmp_buf);
  p_tok.put_raw_data((const char*)tmp_buf.get_data(), tmp_buf.get_len());
  return tmp_buf.get_len();
}
#endif


boolean UNIVERSAL_CHARSTRING::BER_decode_TLV
(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv,
 unsigned L_form)
{
  clean_up();
  charstring = FALSE;
  TTCN_EncDec_ErrorContext ec("While decoding universal charstring type: ");
  OCTETSTRING ostr;
  if(!ostr.BER_decode_TLV(p_td, p_tlv, L_form)) return FALSE;
  int os_len=ostr.lengthof();
  int ucs_len;
  const unsigned char* os=ostr;
  switch(p_td.asnbasetype) {
  case TTCN_Typedescriptor_t::TELETEXSTRING:
    *this=TTCN_ISO2022_2_TeletexString(ostr);
    break;
  case TTCN_Typedescriptor_t::VIDEOTEXSTRING:
    *this=TTCN_ISO2022_2_VideotexString(ostr);
    break;
  case TTCN_Typedescriptor_t::GRAPHICSTRING:
    *this=TTCN_ISO2022_2_GraphicString(ostr);
    break;
  case TTCN_Typedescriptor_t::GENERALSTRING:
    *this=TTCN_ISO2022_2_GeneralString(ostr);
    break;
  case TTCN_Typedescriptor_t::UNIVERSALSTRING:
    if(os_len%4)
      TTCN_EncDec_ErrorContext::error
        (TTCN_EncDec::ET_DEC_UCSTR, "Length of UCS-4-coded character"
         " string is not multiple of 4.");
    ucs_len=os_len/4;
    init_struct(ucs_len);
    for(int i=0; i<ucs_len; i++) {
      val_ptr->uchars_ptr[i].uc_group=os[0];
      val_ptr->uchars_ptr[i].uc_plane=os[1];
      val_ptr->uchars_ptr[i].uc_row=os[2];
      val_ptr->uchars_ptr[i].uc_cell=os[3];
      os+=4;
    }
    break;
  case TTCN_Typedescriptor_t::BMPSTRING:
    if(os_len%2)
      TTCN_EncDec_ErrorContext::error
        (TTCN_EncDec::ET_DEC_UCSTR, "Length of UCS-2-coded character"
         " string is not multiple of 2.");
    ucs_len=os_len/2;
    init_struct(ucs_len);
    for(int i=0; i<ucs_len; i++) {
      val_ptr->uchars_ptr[i].uc_group=0;
      val_ptr->uchars_ptr[i].uc_plane=0;
      val_ptr->uchars_ptr[i].uc_row=os[0];
      val_ptr->uchars_ptr[i].uc_cell=os[1];
      os+=2;
    }
    break;
  case TTCN_Typedescriptor_t::UTF8STRING:
    decode_utf8(os_len, os);
    break;
  default:
    TTCN_EncDec_ErrorContext::error_internal
      ("Missing/wrong basetype info for type '%s'.", p_td.name);
  } // switch
  return TRUE;
}

extern void xml_escape(const unsigned int c, TTCN_Buffer& p_buf);

int UNIVERSAL_CHARSTRING::XER_encode(const XERdescriptor_t& p_td,
    TTCN_Buffer& p_buf, unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound UNIVERSAL CHARSTRING value.");
  }
  if (charstring)
    const_cast<UNIVERSAL_CHARSTRING&>(*this).convert_cstr_to_uni();
  int exer  = is_exer(flavor |= SIMPLE_TYPE);
  // SIMPLE_TYPE has no influence on is_exer, we set it for later
  int encoded_length=(int)p_buf.get_len();
  boolean do_empty_element = val_ptr==NULL || val_ptr->n_uchars == 0;

  flavor &= ~XER_RECOF; // universal charstring doesn't care
  if (exer && (p_td.xer_bits & ANY_ELEMENT)) {
    if (!is_canonical(flavor)) {
      // Although ANY_ELEMENT is not canonical, this flag is still used to disable
      // indenting in a record/set with EMBED_VALUES
      do_indent(p_buf, indent);
    }
  }
  else {
    if (do_empty_element && exer && p_td.dfeValue != 0) {
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_REPR,
        "An encoded value with DEFAULT-FOR-EMPTY instruction "
        "applied should not be empty");
    }
    if (begin_xml(p_td, p_buf, flavor, indent, do_empty_element) == -1) {
      --encoded_length;
    }
  } // not any_element

#define UCH(c) {0,0,0,c}
  if (!do_empty_element) {
    if (flavor & XER_ESCAPE_ENTITIES) {
      for (int i = 0; i < val_ptr->n_uchars; ++i) {
        unsigned int ucs4 = unichar2int(val_ptr->uchars_ptr[i]);
        xml_escape( ucs4, p_buf );
      }
    }
    else { // UTF-8 needs only to escape the low 32 and these five: <&>
      TTCN_Buffer other_buf;
      static const universal_char amp[] = { UCH('&'), UCH('a'), UCH('m'),
        UCH('p'), UCH(';') };
      static const universal_char lt [] = { UCH('&'), UCH('l'), UCH('t'),
        UCH(';') };
      static const universal_char gt [] = { UCH('&'), UCH('g'), UCH('t'),
        UCH(';') };
      static const universal_char apos[]= { UCH('&'), UCH('a'), UCH('p'),
        UCH('o'), UCH('s'), UCH(';') };
      static const universal_char quot[]= { UCH('&'), UCH('q'), UCH('u'),
        UCH('o'), UCH('t'), UCH(';') };

      static const universal_char escapes[32][6] = {
        {UCH('<'), UCH('n'), UCH('u'), UCH('l'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('s'), UCH('o'), UCH('h'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('s'), UCH('t'), UCH('x'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('e'), UCH('t'), UCH('x'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('e'), UCH('o'), UCH('t'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('e'), UCH('n'), UCH('q'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('a'), UCH('c'), UCH('k'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('b'), UCH('e'), UCH('l'), UCH('/'), UCH('>')},

        {UCH('<'), UCH('b'), UCH('s'), UCH('/'), UCH('>')},
        {UCH('&'), UCH('#'), UCH('x'), UCH('0'), UCH('9'), UCH(';')}, // TAB
        {UCH('&'), UCH('#'), UCH('x'), UCH('0'), UCH('A'), UCH(';')}, // LF
        {UCH('<'), UCH('v'), UCH('t'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('f'), UCH('f'), UCH('/'), UCH('>')},
        {UCH('&'), UCH('#'), UCH('x'), UCH('0'), UCH('D'), UCH(';')}, // CR
        {UCH('<'), UCH('s'), UCH('o'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('s'), UCH('i'), UCH('/'), UCH('>')},

        {UCH('<'), UCH('d'), UCH('l'), UCH('e'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('d'), UCH('c'), UCH('1'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('d'), UCH('c'), UCH('2'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('d'), UCH('c'), UCH('3'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('d'), UCH('c'), UCH('4'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('n'), UCH('a'), UCH('k'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('s'), UCH('y'), UCH('n'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('e'), UCH('t'), UCH('b'), UCH('/'), UCH('>')},

        {UCH('<'), UCH('c'), UCH('a'), UCH('n'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('e'), UCH('m'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('s'), UCH('u'), UCH('b'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('e'), UCH('s'), UCH('c'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('i'), UCH('s'), UCH('4'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('i'), UCH('s'), UCH('3'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('i'), UCH('s'), UCH('2'), UCH('/'), UCH('>')},
        {UCH('<'), UCH('i'), UCH('s'), UCH('1'), UCH('/'), UCH('>')}
      };

      if (exer && (p_td.xer_bits & ANY_ELEMENT)) { // no escaping
        TTCN_EncDec_ErrorContext ec("While checking anyElement: ");
        encode_utf8(other_buf);
        XmlReaderWrap checker(other_buf);
        // Walk through the XML. If it's not well-formed, XmlReaderWrap
        // will call TTCN_error => Dynamic testcase error.
        while (1 == checker.Read()) {
          if (checker.NodeType() == XML_READER_TYPE_ELEMENT &&
              (p_td.xer_bits & (ANY_FROM | ANY_EXCEPT))) {
            const char* xmlns = (const char*)checker.NamespaceUri();
            // If no xmlns attribute found, and only one namespace is allowed then
            // append the xmlns attribute with the namespace.
            if (xmlns == NULL && (p_td.xer_bits & ANY_FROM) && p_td.nof_ns_uris == 1 &&
                p_td.ns_uris[0] != NULL && strlen(p_td.ns_uris[0]) != 0) {
              // Find the position of the '>' character
              const char * pos = strchr((const char*)(other_buf.get_data()), '>');
              int index = (int)(pos - (const char*)other_buf.get_data());
              
              // Create a new TTCN_Buffer which will contain the anyelement with
              // the xmlns attribute.
              TTCN_Buffer new_buf;
              new_buf.put_s(index, other_buf.get_data());
              new_buf.put_s(8, (const unsigned char*)" xmlns='");
              size_t len = strlen(p_td.ns_uris[0]);
              new_buf.put_s(len, (const unsigned char*)p_td.ns_uris[0]);
              new_buf.put_c('\'');

              other_buf.set_pos(index);
              new_buf.put_s(other_buf.get_len() - index, other_buf.get_read_data());
              other_buf = new_buf;
            } else {
              check_namespace_restrictions(p_td, xmlns);
            }
          }
        }
          
        // other_buf is already UTF-8, just append it
        p_buf.put_buf(other_buf);
      } 
      else if (flavor & ANY_ATTRIBUTES) { // no escaping
        encode_utf8(other_buf);
        p_buf.put_buf(other_buf);
      }
      else {
        for (int i = 0; i < val_ptr->n_uchars; ++i) {
          int char_val = unichar2int(val_ptr->uchars_ptr[i]);
          size_t len = 6; // 3 letters + the surrounding '<', '/' and '>'
          switch (char_val) {
          case '&':
            other_buf.put_s(20, &(amp[0].uc_group));
            break;

          case '<':
            other_buf.put_s(16, &(lt[0].uc_group));
            break;

          case '>':
            other_buf.put_s(16, &(gt[0].uc_group));
            break;

          case '\'': // X.693 20.3.13: Titan uses single quotes for attributes;
            // so if they appear in content they must be escaped.
            // Currently this happens even if the string is not an attribute.
            other_buf.put_s(24, &(apos[0].uc_group));
            break;
            
          case '\"': // HR58225
            other_buf.put_s(24, &(quot[0].uc_group));
            break;

          case  8: case 11: case 12: case 14: case 15: case 25:
            // the name of these control characters has only two letters
            --len;
            // no break
          case  0: case  1: case  2: case  3: case  4: case  5: case  6:
          case  7: case 16: case 17: case 18: case 19: case 20: case 21:
          case 22: case 23: case 24: case 26: case 27: case 28: case 29:
          case 30: case 31:
            other_buf.put_s(len * 4, &(escapes[char_val][0].uc_group));
            break;

          case 9: case 10: case 13:
            if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
              // X.693 20.3.15: TAB,LF,CR also needs to be escaped in ATTRIBUTE
              other_buf.put_s(24, &(escapes[char_val][0].uc_group));
              break;
            } // else fall through
            // no break
          default:
            other_buf.put_s(4, &(val_ptr->uchars_ptr[i].uc_group));
            break;
          }
        } // next
        // other_buf contains UCS-4, need to encode it
        UNIVERSAL_CHARSTRING cs;
        other_buf.get_string(cs);
        (cs).encode_utf8(p_buf);
      } // not any_element
    } // if ESCAPE_ENTITIES

    if (exer && (p_td.xer_bits & ANY_ELEMENT) && !is_canonical(flavor)) {
      p_buf.put_c('\n');
    }
  }

  end_xml(p_td, p_buf, flavor, indent, do_empty_element);

  return (int)p_buf.get_len() - encoded_length;
}

/* Hashing for xmlcstring representation of control characters.
 * This function was generated by gperf 3.0.3
 * (GNU perfect hash function generator) from the list of control character
 * names, one per line, like this:
 *
 * nul
 * soh
 * ...etc...
 * is1
 *
 * See the ASN.1 standard, X.680/2002, clause 11.15.5
 */
inline
static unsigned int
hash (register const char *str, register unsigned int len)
{
  static unsigned char asso_values[] =
    {
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104,   1,
       60,  55,  45, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104,  10,  10,  50,
       40,  15,   0, 104,   6,  15, 104,   1,   5,  10,
        0,  20, 104,  15,   0,   0,  30, 104,   0, 104,
       20, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
      104, 104, 104, 104, 104, 104
    };
  return len + asso_values[(unsigned char)str[len - 1]] +
    asso_values[(unsigned char)str[0]];
}

/* This function was also generated by gperf and hand-edited afterwards.
 * The original wordlist[] was an array of strings containing the names of
 * the control characters, with empty strings as fillers. These have been
 * replaced with the actual numerical values of the control characters,
 * with -1 as the filler.
 */
inline
static char
in_word_set (const char *str, unsigned int len)
{
  static char wordlist[] =
    {
      -1, -1,
      12, // FF
      22, // SYN
      21, // NAK
      -1, -1,
      10, // LF
      0,  // NUL
      1,  // SOH
      -1, -1,
      8,  // BS
      26, // SUB
      6,  // ACK
      -1, -1,
      15, // SI
      7,  // BEL
      31, // IS1
      -1, -1,
      14, // SO
      2,  // STX
      -1, -1, -1,
      25, // EM
      23, // ETB
      -1, -1, -1,
      11, // VT
      5,  // ENQ
      -1, -1, -1, -1,
      3,  // ETX
      -1, -1, -1, -1,
      9,  // TAB
      17, // DC1
      -1, -1, -1,
      4,  // EOT
      -1, -1, -1,
      13, // CR
      24, // CAN
      -1, -1, -1, -1,
      16, // DLE
      -1, -1, -1, -1,
      28, // IS4
      -1, -1, -1, -1,
      27, // ESC
      -1, -1, -1, -1,
      29, // IS3
      -1, -1, -1, -1,
      30, // IS2
      -1, -1, -1, -1, -1, -1, -1, -1, -1,
      20, // DC4
      -1, -1, -1, -1, -1, -1, -1, -1, -1,
      19, // DC3
      -1, -1, -1, -1,
      18  // DC2
    };
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 3
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 103

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char s = wordlist[key];
          return s;
        }
    }
  return -1;
}

universal_char const uspace = {0,0,0,32};

int UNIVERSAL_CHARSTRING::XER_decode(const XERdescriptor_t& p_td,
  XmlReaderWrap& reader, unsigned int flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*)
{
  boolean exer  = is_exer(flavor);
  int success = reader.Ok(), depth = -1;
  boolean omit_tag = exer
    && ((p_td.xer_bits & UNTAGGED)
    || (flavor & (EMBED_VALUES|XER_LIST|USE_TYPE_ATTR|ANY_ATTRIBUTES|USE_NIL)));

  if (exer && (p_td.xer_bits & ANY_ELEMENT)) {
    TTCN_EncDec_ErrorContext ec("While checking anyElement: ");
    for (; success == 1; success = reader.Read()) {
      int type = reader.NodeType();
      if (-1 == depth && XML_READER_TYPE_ELEMENT == type) {
        xmlChar * value = reader.ReadOuterXml();
        size_t num_chars = strlen((const char*)value);
        clean_up();
        charstring = FALSE;
        decode_utf8(num_chars, value); // does init_struct
        xmlFree(value);
        
        if (p_td.xer_bits & (ANY_FROM | ANY_EXCEPT)) {
          const char* xmlns = (const char*)reader.NamespaceUri();
          check_namespace_restrictions(p_td, xmlns);
        }
        
        if (reader.IsEmptyElement()) {
          reader.Read(); // skip past the empty element and we're done
          break;
        }
        depth = reader.Depth(); // signals that we have the element's text
        // Stay in the loop until we reach the corresponding end tag
      }
      else if (reader.Depth() == depth && XML_READER_TYPE_END_ELEMENT == type) {
        reader.Read(); // one last time
        break;
      } // type
    } // next read
    // ANY-ELEMENT and WHITESPACE are mutually exclusive,
    // so this branch skips WHITESPACE processing
  }
  else { // not ANY-ELEMENT
    clean_up(); // start with a clean slate
    charstring = FALSE;
    if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
      // we have it easy (but does decode_utf8 handle &nnn; ?)
      const char * name = verify_name(reader, p_td, exer);
      (void)name;
      const char * value = (const char *)reader.Value();
      int len = strlen(value);
      decode_utf8(len, (cbyte*)value);

      // Let the caller do reader.AdvanceAttribute();
    }
    else { // not an attribute either
      // Get to the start of data.
      if (!omit_tag) for (; success == 1; success = reader.Read()) {
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
              *this = *static_cast<const UNIVERSAL_CHARSTRING*>(p_td.dfeValue);
            }
            else init_struct(0); // it's an empty string
            reader.Read(); // move on
            return 0;
          }
          depth = reader.Depth();
          success = reader.Read();
          break; // the loop
        }
        else if (XML_READER_TYPE_TEXT == type && omit_tag)
          break;
        else // if XML_READER_TYPE_END_ELEMENT, panic?
          continue;
      } // next read

      init_struct(0);
      TTCN_Buffer accumulator;
      if (flavor & PARENT_CLOSED) {} // do nothing
      else for (; success==1; success = reader.Read()) {
        int type = reader.NodeType();
        switch (type) {
        case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
        case XML_READER_TYPE_TEXT:
        case XML_READER_TYPE_CDATA: {
          const char * text = (const char*)reader.Value();
          int len = strlen(text);
          accumulator.put_s(len, (cbyte*)text);
          break; }

        case XML_READER_TYPE_ELEMENT: { // escaped control character
          const char * name = (const char*)reader.LocalName();
          size_t len = strlen(name);
          char ctrl = in_word_set(name, len);
          if (ctrl >= 0) {
            accumulator.put_c(ctrl);
          }
          else {
            TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
              "Invalid escape sequence '<%s/>'", name);
          }
          break; }

        case XML_READER_TYPE_END_ELEMENT: {
          decode_utf8(accumulator.get_len(), accumulator.get_data());
          if (!omit_tag) {
            verify_end(reader, p_td, depth, exer);
            reader.Read(); // Moved to a different XML node!
            if (val_ptr->n_uchars == 0 && exer && p_td.dfeValue != 0) {
              // This should be the <foo></foo> case. Treat it like <foo/>
              *this = *static_cast<const UNIVERSAL_CHARSTRING*>(p_td.dfeValue);
            }
          }
          goto fini; } // double "break" of switch and for loop

        default:
          break;
        } // switch
      } // next read
    } // endif (attribute)
fini:
    if (exer && p_td.whitespace >= WHITESPACE_REPLACE) { // includes _COLLAPSE
      for(int i=0; i<val_ptr->n_uchars; ++i) { // first, _REPLACE
        switch ( unichar2int(val_ptr->uchars_ptr[i]) ) {
        case 9:  // HORIZONTAL TAB
        case 10: // LINE FEED
        case 13: // CARRIAGE RETURN
          val_ptr->uchars_ptr[i] = uspace;
          break;
        default:
          break;
        } // switch
      } // next i

      if (p_td.whitespace >= WHITESPACE_COLLAPSE) {
        universal_char *to;
        const universal_char *from, *end =
          val_ptr->uchars_ptr + val_ptr->n_uchars;
        for (from = to = val_ptr->uchars_ptr; from < end;) {
          *to = *from++;
          // If the copied character (*to) was a space,
          // and the next character to be copied (*from) is also a space
          //     (continuous run of spaces)
          //   or this was the first character (leading spaces to be trimmed),
          // then don't advance the destination (will be overwritten).
          if (*to != uspace
            || (from < end && *from != uspace && to > val_ptr->uchars_ptr))
            ++to;
        } // next
        val_ptr->n_uchars = to - val_ptr->uchars_ptr;
      }
    } // if WHITESPACE
  } // not ANY_ELEMENT
  return 1; // decode successful
}

char* UNIVERSAL_CHARSTRING::to_JSON_string(const TTCN_Buffer& p_buf) const
{
  const unsigned char* ustr = p_buf.get_data();
  const size_t ustr_len = p_buf.get_len();
  
  // Need at least 3 more characters (the double quotes around the string and the terminating zero)
  char* json_str = (char*)Malloc(ustr_len + 3);
  
  json_str[0] = 0;
  json_str = mputc(json_str, '\"');
  
  for (size_t i = 0; i < ustr_len; ++i) {
    // Increase the size of the buffer if it's not big enough to store the
    // characters remaining in the universal charstring
    switch(ustr[i]) {
    case '\\':
      json_str = mputstrn(json_str, "\\\\", 2);
      break;
    case '\n':
      json_str = mputstrn(json_str, "\\n", 2);
      break;
    case '\t':
      json_str = mputstrn(json_str, "\\t", 2);
      break;
    case '\r':
      json_str = mputstrn(json_str, "\\r", 2);
      break;
    case '\f':
      json_str = mputstrn(json_str, "\\f", 2);
      break;
    case '\b':
      json_str = mputstrn(json_str, "\\b", 2);
      break;
    case '\"':
      json_str = mputstrn(json_str, "\\\"", 2);
      break;
    default:
      json_str = mputc(json_str, (char)ustr[i]);
      break;
    }
  }
  
  json_str = mputc(json_str, '\"');
  return json_str;
}

int UNIVERSAL_CHARSTRING::RAW_encode(const TTCN_Typedescriptor_t& p_td,
  RAW_enc_tree& myleaf) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
  }
  if (charstring) {
    return cstr.RAW_encode(p_td, myleaf);
  }
  TTCN_Buffer buff;
  switch (p_td.raw->stringformat) {
  case CharCoding::UNKNOWN: // default is UTF-8
  case CharCoding::UTF_8:
    encode_utf8(buff);
    break;
  case CharCoding::UTF16:
    encode_utf16(buff, CharCoding::UTF16);
    break;
  default:
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INTERNAL,
      "Invalid string serialization type.");
  }
  int buff_len = buff.get_len();
  int bl = buff_len * 8; // bit length
  int align_length = p_td.raw->fieldlength ? p_td.raw->fieldlength - bl : 0;
  if (align_length < 0) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There are insufficient bits to encode '%s': ", p_td.name);
    bl = p_td.raw->fieldlength;
    align_length = 0;
  }
  if (myleaf.must_free) Free(myleaf.body.leaf.data_ptr);
  myleaf.body.leaf.data_ptr = (unsigned char*)Malloc(buff_len);
  memcpy(myleaf.body.leaf.data_ptr, buff.get_data(), buff_len);
  myleaf.must_free = TRUE;
  myleaf.data_ptr_used = TRUE;
  if (p_td.raw->endianness == ORDER_MSB) myleaf.align = -align_length;
  else myleaf.align = align_length;
  return myleaf.length = bl + align_length;
}

int UNIVERSAL_CHARSTRING::RAW_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff, int limit, raw_order_t top_bit_ord, boolean no_err,
  int /*sel_field*/, boolean /*first_call*/)
{
  CHARSTRING buff_str;
  int dec_len = buff_str.RAW_decode(p_td, buff, limit, top_bit_ord, no_err);
  if (buff_str.is_bound()) {
    charstring = TRUE;
    for (int i = 0; i < buff_str.val_ptr->n_chars; ++i) {
      if (buff_str.val_ptr->chars_ptr[i] < 0) {
        charstring = FALSE;
        break;
      }
    }
    switch (p_td.raw->stringformat) {
    case CharCoding::UNKNOWN: // default is UTF-8
    case CharCoding::UTF_8:
      if (charstring) {
        cstr = buff_str;
      }
      else {
        decode_utf8(buff_str.val_ptr->n_chars, (const unsigned char*)buff_str.val_ptr->chars_ptr);
      }
      break;
    case CharCoding::UTF16:
      if (!charstring) {
        decode_utf16(buff_str.val_ptr->n_chars,
          (const unsigned char*)buff_str.val_ptr->chars_ptr, CharCoding::UTF16);
      }
      else {
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
          "Invalid string format. Buffer contains only ASCII characters.");
      }
      break;
    default:
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INTERNAL,
        "Invalid string serialization type.");
    }
  }
  return dec_len;
}

boolean UNIVERSAL_CHARSTRING::from_JSON_string(boolean check_quotes)
{
  int json_len = val_ptr->n_uchars;
  universal_char* json_str = val_ptr->uchars_ptr;
  
  int start = 0;
  int end = json_len;
  if (check_quotes) {
    start = 1;
    end = json_len - 1;
    if (!json_str[0].is_char() || json_str[0].uc_cell != '\"' || 
        !json_str[json_len - 1].is_char() || json_str[json_len - 1].uc_cell != '\"') {
      return FALSE;
    }
  }
  
  // The resulting string (its length is less than or equal to end - start)
  universal_char* ustr = (universal_char*)Malloc((end - start) * sizeof(universal_char));
  memset(ustr, 0, sizeof(universal_char) * (end - start));
  int ustr_len = 0;
  boolean error = FALSE;
  
  for (int i = start; i < end; ++i) {
    if (json_str[i].is_char() && '\\' == json_str[i].uc_cell) {
      if (i == end - 1 || !json_str[i + 1].is_char()) {
        error = TRUE;
        break;
      }
      switch(json_str[i + 1].uc_cell) {
      case 'n':
        ustr[ustr_len++].uc_cell = '\n';
        break;
      case 't':
        ustr[ustr_len++].uc_cell = '\t';
        break;
      case 'r':
        ustr[ustr_len++].uc_cell = '\r';
        break;
      case 'f':
        ustr[ustr_len++].uc_cell = '\f';
        break;
      case 'b':
        ustr[ustr_len++].uc_cell = '\b';
        break;
      case '\\':
        ustr[ustr_len++].uc_cell = '\\';
        break;
      case '\"':
        ustr[ustr_len++].uc_cell = '\"';
        break;
      case '/':
        ustr[ustr_len++].uc_cell = '/';
        break;
      case 'u': {
        if (end - i >= 6 && json_str[i + 2].is_char() && json_str[i + 3].is_char() &&
            json_str[i + 4].is_char() && json_str[i + 5].is_char()) {
          unsigned char row_upper_nibble = char_to_hexdigit(json_str[i + 2].uc_cell);
          unsigned char row_lower_nibble = char_to_hexdigit(json_str[i + 3].uc_cell);
          unsigned char cell_upper_nibble = char_to_hexdigit(json_str[i + 4].uc_cell);
          unsigned char cell_lower_nibble = char_to_hexdigit(json_str[i + 5].uc_cell);
          if (row_upper_nibble <= 0x0F && row_lower_nibble <= 0x0F &&
              cell_upper_nibble <= 0x0F && cell_lower_nibble <= 0x0F) {
            ustr[ustr_len].uc_row = (row_upper_nibble << 4) | row_lower_nibble;
            ustr[ustr_len++].uc_cell = (cell_upper_nibble << 4) | cell_lower_nibble;
            // skip 4 extra characters (the 4 hex digits)
            i += 4;
          } else {
            // error (encountered something other than a hex digit) -> leave the for cycle
            i = end;
            error = TRUE;
          }
        } else {
          // error (not enough characters or the 'hex digits' are not even ascii characters) -> leave the for cycle
          i = end;
          error = TRUE;
        }
        break; 
      }
      default:
        // error (invalid escaped character) -> leave the for cycle
        i = end;
        error = TRUE;
        break;
      }
      // skip an extra character (the \)
      ++i;
    } else {
      ustr[ustr_len++] = json_str[i];
    } 
    
    if (check_quotes && i == json_len - 1) {
      // Special case: the last 2 characters are double escaped quotes ('\\' and '\"')
      error = TRUE;
    }
  }
  
  if (!error) {
    clean_up();
    init_struct(ustr_len);
    memcpy(val_ptr->uchars_ptr, ustr, ustr_len * sizeof(universal_char));
  }
  Free(ustr);
  return !error;
}

int UNIVERSAL_CHARSTRING::JSON_encode(const TTCN_Typedescriptor_t& /*p_td*/, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound universal charstring value.");
    return -1;
  }
  
  char* tmp_str = 0;
  if (charstring) {
    tmp_str = cstr.to_JSON_string();
  } else {
    TTCN_Buffer tmp_buf;
    encode_utf8(tmp_buf);
    tmp_str = to_JSON_string(tmp_buf);
  }
  int enc_len = p_tok.put_next_token(JSON_TOKEN_STRING, tmp_str);
  Free(tmp_str);
  return enc_len;
}

int UNIVERSAL_CHARSTRING::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
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
  if (JSON_TOKEN_ERROR == token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  else if (JSON_TOKEN_STRING == token || use_default) {
    if (cstr.from_JSON_string(value, value_len, !use_default)) {
      charstring = TRUE;
    }
    else {
      charstring = FALSE;
      decode_utf8(value_len, (unsigned char*)value);
      if (!from_JSON_string(!use_default)) {
        JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FORMAT_ERROR, "string", "universal charstring");
        if (p_silent) {
          clean_up();
        }
        return JSON_ERROR_FATAL;
      }
    }
  } else {
    return JSON_ERROR_INVALID_TOKEN;
  }
  return (int)dec_len;
}


static void fill_continuing_octets(int n_continuing,
  unsigned char *continuing_ptr, int n_octets,
  const unsigned char *octets_ptr, int start_pos, int uchar_pos)
{
  for (int i = 0; i < n_continuing; i++) {
    if (start_pos + i < n_octets) {
      unsigned char octet = octets_ptr[start_pos + i];
      if ((octet & 0xC0) != 0x80) {
	TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
	  "Malformed: At character position %d, octet position %d: %02X is "
	  "not a valid continuing octet.", uchar_pos, start_pos + i, octet);
      }
      continuing_ptr[i] = octet & 0x3F;
    } else {
      if (start_pos + i == n_octets) {
        if (i > 0) {
	  // only a part of octets is missing
	  TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
	    "Incomplete: At character position %d, octet position %d: %d out "
	    "of %d continuing octets %s missing from the end of the stream.",
	    uchar_pos, start_pos + i, n_continuing - i, n_continuing,
	    n_continuing - i > 1 ? "are" : "is");
	} else {
	  // all octets are missing
	  TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
	    "Incomplete: At character position %d, octet position %d: %d "
	    "continuing octet%s missing from the end of the stream.", uchar_pos,
	    start_pos, n_continuing, n_continuing > 1 ? "s are" : " is");
	}
      }
      continuing_ptr[i] = 0;
    }
  }
}

void UNIVERSAL_CHARSTRING::decode_utf8(int n_octets,
                                       const unsigned char *octets_ptr,
                                       CharCoding::CharCodingType /*expected_coding*/ /*= UTF8*/,
                                       boolean checkBOM /*= false)*/)
{
  // approximate the number of characters
  int n_uchars = 0;
  for (int i = 0; i < n_octets; i++) {
    // count all octets except the continuing octets (10xxxxxx)
    if ((octets_ptr[i] & 0xC0) != 0x80) n_uchars++;
  }
  // allocate enough memory, start from clean state
  clean_up();
  charstring=FALSE;
  init_struct(n_uchars);
  n_uchars = 0;

  int start = checkBOM ? check_BOM(CharCoding::UTF_8, n_octets, octets_ptr) : 0;
  for (int i = start; i < n_octets; ) {
    // perform the decoding character by character
    if (octets_ptr[i] <= 0x7F) {
      // character encoded on a single octet: 0xxxxxxx (7 useful bits)
      val_ptr->uchars_ptr[n_uchars].uc_group = 0;
      val_ptr->uchars_ptr[n_uchars].uc_plane = 0;
      val_ptr->uchars_ptr[n_uchars].uc_row = 0;
      val_ptr->uchars_ptr[n_uchars].uc_cell = octets_ptr[i];
      i++;
      n_uchars++;
    } else if (octets_ptr[i] <= 0xBF) {
      // continuing octet (10xxxxxx) without leading octet ==> malformed
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
      "Malformed: At character position %d, octet position %d: continuing "
      "octet %02X without leading octet.", n_uchars, i, octets_ptr[i]);
      i++;
    } else if (octets_ptr[i] <= 0xDF) {
      // character encoded on 2 octets: 110xxxxx 10xxxxxx (11 useful bits)
      unsigned char octets[2];
      octets[0] = octets_ptr[i] & 0x1F;
      fill_continuing_octets(1, octets + 1, n_octets, octets_ptr, i + 1, n_uchars);
      val_ptr->uchars_ptr[n_uchars].uc_group = 0;
      val_ptr->uchars_ptr[n_uchars].uc_plane = 0;
      val_ptr->uchars_ptr[n_uchars].uc_row = octets[0] >> 2;
      val_ptr->uchars_ptr[n_uchars].uc_cell = octets[0] << 6 | octets[1];
      if (val_ptr->uchars_ptr[n_uchars].uc_row == 0x00 &&
          val_ptr->uchars_ptr[n_uchars].uc_cell < 0x80)
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
        "Overlong: At character position %d, octet position %d: 2-octet "
        "encoding for quadruple (0, 0, 0, %u).", n_uchars, i,
        val_ptr->uchars_ptr[n_uchars].uc_cell);
      i += 2;
      n_uchars++;
    } else if (octets_ptr[i] <= 0xEF) {
      // character encoded on 3 octets: 1110xxxx 10xxxxxx 10xxxxxx
      // (16 useful bits)
      unsigned char octets[3];
      octets[0] = octets_ptr[i] & 0x0F;
      fill_continuing_octets(2, octets + 1, n_octets, octets_ptr, i + 1,
	n_uchars);
      val_ptr->uchars_ptr[n_uchars].uc_group = 0;
      val_ptr->uchars_ptr[n_uchars].uc_plane = 0;
      val_ptr->uchars_ptr[n_uchars].uc_row = octets[0] << 4 | octets[1] >> 2;
      val_ptr->uchars_ptr[n_uchars].uc_cell = octets[1] << 6 | octets[2];
      if (val_ptr->uchars_ptr[n_uchars].uc_row < 0x08)
	TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
	  "Overlong: At character position %d, octet position %d: 3-octet "
	  "encoding for quadruple (0, 0, %u, %u).", n_uchars, i,
	  val_ptr->uchars_ptr[n_uchars].uc_row,
	  val_ptr->uchars_ptr[n_uchars].uc_cell);
      i += 3;
      n_uchars++;
    } else if (octets_ptr[i] <= 0xF7) {
      // character encoded on 4 octets: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      // (21 useful bits)
      unsigned char octets[4];
      octets[0] = octets_ptr[i] & 0x07;
      fill_continuing_octets(3, octets + 1, n_octets, octets_ptr, i + 1,
	n_uchars);
      val_ptr->uchars_ptr[n_uchars].uc_group = 0;
      val_ptr->uchars_ptr[n_uchars].uc_plane = octets[0] << 2 | octets[1] >> 4;
      val_ptr->uchars_ptr[n_uchars].uc_row = octets[1] << 4 | octets[2] >> 2;
      val_ptr->uchars_ptr[n_uchars].uc_cell = octets[2] << 6 | octets[3];
      if (val_ptr->uchars_ptr[n_uchars].uc_plane == 0x00)
	TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
	  "Overlong: At character position %d, octet position %d: 4-octet "
	  "encoding for quadruple (0, 0, %u, %u).", n_uchars, i,
	  val_ptr->uchars_ptr[n_uchars].uc_row,
	  val_ptr->uchars_ptr[n_uchars].uc_cell);
      i += 4;
      n_uchars++;
    } else if (octets_ptr[i] <= 0xFB) {
      // character encoded on 5 octets: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx
      // 10xxxxxx (26 useful bits)
      unsigned char octets[5];
      octets[0] = octets_ptr[i] & 0x03;
      fill_continuing_octets(4, octets + 1, n_octets, octets_ptr, i + 1,
	n_uchars);
      val_ptr->uchars_ptr[n_uchars].uc_group = octets[0];
      val_ptr->uchars_ptr[n_uchars].uc_plane = octets[1] << 2 | octets[2] >> 4;
      val_ptr->uchars_ptr[n_uchars].uc_row = octets[2] << 4 | octets[3] >> 2;
      val_ptr->uchars_ptr[n_uchars].uc_cell = octets[3] << 6 | octets[4];
      if (val_ptr->uchars_ptr[n_uchars].uc_group == 0x00 &&
	  val_ptr->uchars_ptr[n_uchars].uc_plane < 0x20)
	TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
	  "Overlong: At character position %d, octet position %d: 5-octet "
	  "encoding for quadruple (0, %u, %u, %u).", n_uchars, i,
	  val_ptr->uchars_ptr[n_uchars].uc_plane,
	  val_ptr->uchars_ptr[n_uchars].uc_row,
	  val_ptr->uchars_ptr[n_uchars].uc_cell);
      i += 5;
      n_uchars++;
    } else if (octets_ptr[i] <= 0xFD) {
      // character encoded on 6 octets: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx
      // 10xxxxxx 10xxxxxx (31 useful bits)
      unsigned char octets[6];
      octets[0] = octets_ptr[i] & 0x01;
      fill_continuing_octets(5, octets + 1, n_octets, octets_ptr, i + 1,
	n_uchars);
      val_ptr->uchars_ptr[n_uchars].uc_group = octets[0] << 6 | octets[1];
      val_ptr->uchars_ptr[n_uchars].uc_plane = octets[2] << 2 | octets[3] >> 4;
      val_ptr->uchars_ptr[n_uchars].uc_row = octets[3] << 4 | octets[4] >> 2;
      val_ptr->uchars_ptr[n_uchars].uc_cell = octets[4] << 6 | octets[5];
      if (val_ptr->uchars_ptr[n_uchars].uc_group < 0x04)
	TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
	  "Overlong: At character position %d, octet position %d: 6-octet "
	  "encoding for quadruple (%u, %u, %u, %u).", n_uchars, i,
	  val_ptr->uchars_ptr[n_uchars].uc_group,
	  val_ptr->uchars_ptr[n_uchars].uc_plane,
	  val_ptr->uchars_ptr[n_uchars].uc_row,
	  val_ptr->uchars_ptr[n_uchars].uc_cell);
      i += 6;
      n_uchars++;
    } else {
      // not used code points: FE and FF => malformed
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
	"Malformed: At character position %d, octet position %d: "
	"unused/reserved octet %02X.", n_uchars, i, octets_ptr[i]);
      i++;
    }
  } // for i
  if (val_ptr->n_uchars != n_uchars) {
    // truncate the memory and set the correct size in case of decoding errors
    // (e.g. skipped octets)
    if (n_uchars > 0) {
      val_ptr = (universal_charstring_struct*)Realloc(val_ptr,
	MEMORY_SIZE(n_uchars));
      val_ptr->n_uchars = n_uchars;
    } else {
      clean_up();
      init_struct(0);
    }
  }
}

void UNIVERSAL_CHARSTRING::decode_utf16(int n_octets,
                                        const unsigned char *octets_ptr,
                                        CharCoding::CharCodingType expected_coding)
{
  if (n_octets % 2 || 0 > n_octets) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
    "Wrong UTF-16 string. The number of bytes (%d) in octetstring shall be non negative and divisible by 2",
    n_octets);
  }
  int start = check_BOM(expected_coding, n_octets, octets_ptr);
  int n_uchars = n_octets/2;
  init_struct(n_uchars);
  n_uchars = 0;
  boolean isbig = TRUE;
  switch (expected_coding) {
    case CharCoding::UTF16:
    case CharCoding::UTF16BE:
      isbig = TRUE;
      break;
    case CharCoding::UTF16LE:
      isbig = FALSE;
      break;
    default:
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
       "Unexpected coding type for UTF-16 encoding");
      break;
  }
  for (int i = start; i < n_octets; i += 2 ) {
    int first  = isbig ? i     : i + 1;
    int second = isbig ? i + 1 : i;
    int third  = isbig ? i + 2 : i + 3;
    int fourth = isbig ? i + 3 : i + 2;

    uint16_t W1 = octets_ptr[first] << 8 | octets_ptr[second];
    uint16_t W2 = (i + 3 < n_octets) ? octets_ptr[third] << 8 | octets_ptr[fourth] : 0;

    if (0xD800 > W1 || 0xDFFF < W1) {
     //if W1 < 0xD800 or W1 > 0xDFFF, the character value is the value of W1
      val_ptr->uchars_ptr[n_uchars].uc_group = 0;
      val_ptr->uchars_ptr[n_uchars].uc_plane = 0;
      val_ptr->uchars_ptr[n_uchars].uc_row = octets_ptr[first];
      val_ptr->uchars_ptr[n_uchars].uc_cell = octets_ptr[second];
      ++n_uchars;
    }
    else if (0xD800 > W1 || 0xDBFF < W1) {
      //Determine if W1 is between 0xD800 and 0xDBFF. If not, the sequence
      //is in error and no valid character can be obtained using W1.
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
      "The word (0x%04X) shall be between 0xD800 and 0xDBFF", W1);
    }
    else if (0 == W2 || (0xDC00 > W2 || 0xDFFF < W2)) {
      //If there is no W2 (that is, the sequence ends with W1), or if W2
      //is not between 0xDC00 and 0xDFFF, the sequence is in error.
      if (W2)
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
        "Wrong UTF-16 string. The word (0x%04X) shall be between 0xDC00 and 0xDFFF", W2);
      else
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
        "Wrong UTF-16 string. The decoding algorythm does not expect 0x00 or EOL");
    }
    else {
      //Construct a 20-bit unsigned integer, taking the 10 low-order bits of W1 as its 10 high-
      //order bits and the 10 low-order bits of W2 as its 10 low-order bits.
      const uint16_t mask10bitlow = 0x3FF;
      uint32_t DW = (W1 & mask10bitlow) << 10;
      DW |= (W2 & mask10bitlow);
      DW += 0x10000;
      val_ptr->uchars_ptr[n_uchars].uc_group = 0;
      val_ptr->uchars_ptr[n_uchars].uc_plane = DW >> 16;
      val_ptr->uchars_ptr[n_uchars].uc_row = DW >> 8;
      val_ptr->uchars_ptr[n_uchars].uc_cell = DW;
      ++n_uchars;
      i += 2; // jump over w2 in octetstring
    }
  }
  if (val_ptr->n_uchars != n_uchars) {
    // truncate the memory and set the correct size in case of decoding errors
    // (e.g. skipped octets)
    if (n_uchars > 0) {
      val_ptr = (universal_charstring_struct*)Realloc(val_ptr, MEMORY_SIZE(n_uchars));
      val_ptr->n_uchars = n_uchars;
    }
    else {
      clean_up();
      init_struct(0);
    }
  }
}

void UNIVERSAL_CHARSTRING::decode_utf32(int n_octets, const unsigned char *octets_ptr,
                                        CharCoding::CharCodingType expected_coding)
{
  if (n_octets % 4 || 0 > n_octets) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
    "Wrong UTF-32 string. The number of bytes (%d) in octetstring shall be non negative and divisible by 4",
    n_octets);
  }
  int start = check_BOM(expected_coding, n_octets, octets_ptr);
  int n_uchars = n_octets/4;
  init_struct(n_uchars);
  n_uchars = 0;
  boolean isbig = TRUE;
  switch (expected_coding) {
    case CharCoding::UTF32:
    case CharCoding::UTF32BE:
      isbig = TRUE;
      break;
    case CharCoding::UTF32LE:
      isbig = FALSE;
      break;
    default:
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
       "Unexpected coding type for UTF-32 encoding");
      break;
  }
    for (int i = start; i < n_octets; i += 4 ) {
      int first  = isbig ? i     : i + 3;
      int second = isbig ? i + 1 : i + 2;
      int third  = isbig ? i + 2 : i + 1;
      int fourth = isbig ? i + 3 : i;
      uint32_t DW = octets_ptr[first] << 8 | octets_ptr[second];
      DW <<= 8;
      DW |= octets_ptr[third];
      DW <<= 8;
      DW |= octets_ptr[fourth];
      if (0x0000D800 <= DW && 0x0000DFFF >= DW) {
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
        "Any UTF-32 code (0x%08X) between 0x0000D800 and 0x0000DFFF is ill-formed", DW);
      }
      else if (0x0010FFFF < DW) {
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_DEC_UCSTR,
        "Any UTF-32 code (0x%08X) greater than 0x0010FFFF is ill-formed", DW);
      }
      else {
        val_ptr->uchars_ptr[n_uchars].uc_group = octets_ptr[first];
        val_ptr->uchars_ptr[n_uchars].uc_plane = octets_ptr[second];
        val_ptr->uchars_ptr[n_uchars].uc_row = octets_ptr[third];
        val_ptr->uchars_ptr[n_uchars].uc_cell = octets_ptr[fourth];
        ++n_uchars;
      }
    }
    if (val_ptr->n_uchars != n_uchars) {
      // truncate the memory and set the correct size in case of decoding errors
      // (e.g. skipped octets)
      if (n_uchars > 0) {
        val_ptr = (universal_charstring_struct*)Realloc(val_ptr, MEMORY_SIZE(n_uchars));
        val_ptr->n_uchars = n_uchars;
      }
      else {
        clean_up();
        init_struct(0);
      }
    }
}
int UNIVERSAL_CHARSTRING::check_BOM(CharCoding::CharCodingType expected_coding,
                                    unsigned int length,
                                    const unsigned char* ostr)
{
  std::string coding_str;
  //BOM indicates that the byte order is determined by a byte order mark, 
  //if present at the beginning the length of BOM is returned.
  switch (expected_coding) {
    case CharCoding::UTF32BE:
    case CharCoding::UTF32:
      if (4 <= length && 0x00 == ostr[0] && 0x00 == ostr[1] &&
          0xFE == ostr[2] && 0xFF == ostr[3]) {
        return 4;
      }
      coding_str = "UTF-32BE";
    break;
    case CharCoding::UTF32LE:
      if (4 <= length && 0xFF == ostr[0] && 0xFE == ostr[1] &&
          0x00 == ostr[2] && 0x00 == ostr[3]) {
        return 4;
      }
      coding_str = "UTF-32LE";
      break;
    case CharCoding::UTF16BE:
    case CharCoding::UTF16:
      if (2 <= length && 0xFE == ostr[0] && 0xFF == ostr[1]) {
        return 2;
      }
      coding_str = "UTF-16BE";
      break;
    case CharCoding::UTF16LE:
      if (2 <= length && 0xFF == ostr[0] && 0xFE == ostr[1]) {
        return 2;
      }
      coding_str = "UTF-16LE";
      break;
    case CharCoding::UTF_8:
      if (3 <= length && 0xEF == ostr[0] && 0xBB == ostr[1] && 0xBF == ostr[2]) {
        return 3;
      }
      coding_str = "UTF-8";
      break;
    default:
      TTCN_error("Internal error: invalid expected coding (%d)", expected_coding);
      break;
  }
  TTCN_EncDec_ErrorContext::warning("No %s Byte Order Mark(BOM) detected. It may result decoding errors",
  coding_str.c_str());
  return 0;
}

CharCoding::CharCodingType UNIVERSAL_CHARSTRING::get_character_coding
  (const char* coding_str, const char* context_str)
{
  CharCoding::CharCodingType new_coding = CharCoding::UTF_8;
  if (coding_str != NULL && strcmp(coding_str, "UTF-8") != 0) {
    if (strcmp(coding_str, "UTF-16") == 0) {
      new_coding = CharCoding::UTF16;
    }
    else if (strcmp(coding_str, "UTF-16LE") == 0) {
      new_coding = CharCoding::UTF16LE;
    }
    else if (strcmp(coding_str, "UTF-16BE") == 0) {
      new_coding = CharCoding::UTF16BE;
    }
    else if (strcmp(coding_str, "UTF-32") == 0) {
      new_coding = CharCoding::UTF32;
    }
    else if (strcmp(coding_str, "UTF-32LE") == 0) {
      new_coding = CharCoding::UTF32LE;
    }
    else if (strcmp(coding_str, "UTF-32BE") == 0) {
      new_coding = CharCoding::UTF32BE;
    }
    else {
      TTCN_error("Invalid string serialization for %s.", context_str);
    }
  }
  return new_coding;
}

// member functions of class UNIVERSAL_CHARSTRING_ELEMENTS

UNIVERSAL_CHARSTRING_ELEMENT::UNIVERSAL_CHARSTRING_ELEMENT
  (boolean par_bound_flag, UNIVERSAL_CHARSTRING& par_str_val,
  int par_uchar_pos) :
  bound_flag(par_bound_flag), str_val(par_str_val), uchar_pos(par_uchar_pos)
{

}

UNIVERSAL_CHARSTRING_ELEMENT& UNIVERSAL_CHARSTRING_ELEMENT::operator=
  (const universal_char& other_value)
{
  bound_flag = TRUE;
  if (str_val.charstring) {
    if (other_value.is_char()) {
      str_val.cstr.val_ptr->chars_ptr[uchar_pos] = other_value.uc_cell;
      return *this;
    } else
      str_val.convert_cstr_to_uni();
  } else
    str_val.copy_value();
  str_val.val_ptr->uchars_ptr[uchar_pos] = other_value;
  return *this;
}

UNIVERSAL_CHARSTRING_ELEMENT& UNIVERSAL_CHARSTRING_ELEMENT::operator=
  (const char* other_value)
{
  if (other_value == NULL || other_value[0] == '\0' || other_value[1] != '\0')
    TTCN_error("Assignment of a charstring value with length other than 1 to "
      "a universal charstring element.");
  bound_flag = TRUE;
  if (str_val.charstring)
    str_val.cstr.val_ptr->chars_ptr[uchar_pos] = other_value[0];
  else {
    str_val.copy_value();
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_group = 0;
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_plane = 0;
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_row = 0;
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_cell = other_value[0];
  }
  return *this;
}

UNIVERSAL_CHARSTRING_ELEMENT& UNIVERSAL_CHARSTRING_ELEMENT::operator=
  (const CHARSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound charstring value to a "
    "universal charstring element.");
  if(other_value.val_ptr->n_chars != 1)
    TTCN_error("Assignment of a charstring value with length other than 1 to "
      "a universal charstring element.");
  bound_flag = TRUE;
  if (str_val.charstring)
    str_val.cstr.val_ptr->chars_ptr[uchar_pos] =
      other_value.val_ptr->chars_ptr[0];
  else {
    str_val.copy_value();
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_group = 0;
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_plane = 0;
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_row = 0;
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_cell =
      other_value.val_ptr->chars_ptr[0];
  }
  return *this;
}

UNIVERSAL_CHARSTRING_ELEMENT& UNIVERSAL_CHARSTRING_ELEMENT::operator=
  (const CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound charstring element to a "
    "universal charstring element.");
  bound_flag = TRUE;
  if (str_val.charstring)
    str_val.cstr.val_ptr->chars_ptr[uchar_pos] = other_value.get_char();
  else {
    str_val.copy_value();
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_group = 0;
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_plane = 0;
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_row = 0;
    str_val.val_ptr->uchars_ptr[uchar_pos].uc_cell = other_value.get_char();
  }
  return *this;
}

UNIVERSAL_CHARSTRING_ELEMENT& UNIVERSAL_CHARSTRING_ELEMENT::operator=
  (const UNIVERSAL_CHARSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound universal charstring value "
    "to a universal charstring element.");
  int other_value_size = other_value.charstring ? other_value.cstr.val_ptr->n_chars :
    other_value.val_ptr->n_uchars;
  if (other_value_size != 1)
    TTCN_error("Assignment of a universal charstring value with length other "
      "than 1 to a universal charstring element.");
  bound_flag = TRUE;
  *this = other_value[0];
  return *this;
}

UNIVERSAL_CHARSTRING_ELEMENT& UNIVERSAL_CHARSTRING_ELEMENT::operator=
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound universal charstring "
    "element.");
  if (&other_value != this) {
    bound_flag = TRUE;
    if (str_val.charstring) {
      if (other_value.str_val.charstring)
        str_val.cstr.val_ptr->chars_ptr[uchar_pos] =
          other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
      else {
        str_val.convert_cstr_to_uni();
        str_val.val_ptr->uchars_ptr[uchar_pos] =
          other_value.str_val.val_ptr->uchars_ptr[other_value.uchar_pos];
      }
    } else {
      if (other_value.str_val.charstring) {
        str_val.val_ptr->uchars_ptr[uchar_pos].uc_group = 0;
        str_val.val_ptr->uchars_ptr[uchar_pos].uc_plane = 0;
        str_val.val_ptr->uchars_ptr[uchar_pos].uc_row = 0;
        str_val.val_ptr->uchars_ptr[uchar_pos].uc_cell =
          other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
      } else {
        str_val.copy_value();
        str_val.val_ptr->uchars_ptr[uchar_pos] =
          other_value.str_val.val_ptr->uchars_ptr[other_value.uchar_pos];
      }
    }
  }
  return *this;
}

boolean UNIVERSAL_CHARSTRING_ELEMENT::operator==
  (const universal_char& other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring element.");
  if (str_val.charstring && other_value.is_char())
    return str_val.cstr.val_ptr->chars_ptr[uchar_pos] == other_value.uc_cell;
  if (str_val.charstring && !other_value.is_char())
    return FALSE;
  if (!str_val.charstring && other_value.is_char()) {
    const universal_char& uchar = str_val.val_ptr->uchars_ptr[uchar_pos];
    return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
      uchar.uc_cell == other_value.uc_cell;
  }
  return str_val.val_ptr->uchars_ptr[uchar_pos] == other_value;
}

boolean UNIVERSAL_CHARSTRING_ELEMENT::operator==
  (const char* other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring element.");
  if (other_value == NULL || other_value[0] == '\0' ||
      other_value[1] != '\0') return FALSE;
  if (str_val.charstring)
    return str_val.cstr.val_ptr->chars_ptr[uchar_pos] == other_value[0];
  const universal_char& uchar = str_val.val_ptr->uchars_ptr[uchar_pos];
  return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
    uchar.uc_cell == (cbyte)other_value[0];
}

boolean UNIVERSAL_CHARSTRING_ELEMENT::operator==
  (const CHARSTRING& other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring element.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "charstring value.");
  if (other_value.val_ptr->n_chars != 1) return FALSE;
  if (str_val.charstring)
    return str_val.cstr.val_ptr->chars_ptr[uchar_pos] ==
      other_value.val_ptr->chars_ptr[0];
  const universal_char& uchar = str_val.val_ptr->uchars_ptr[uchar_pos];
  return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
    uchar.uc_cell == (cbyte)other_value.val_ptr->chars_ptr[0];
}

boolean UNIVERSAL_CHARSTRING_ELEMENT::operator==
  (const CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring element.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "charstring element.");
  if (str_val.charstring)
    return str_val.cstr.val_ptr->chars_ptr[uchar_pos] == other_value.get_char();
  const universal_char& uchar = str_val.val_ptr->uchars_ptr[uchar_pos];
  return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
    uchar.uc_cell == (cbyte)other_value.get_char();
}

boolean UNIVERSAL_CHARSTRING_ELEMENT::operator==
  (const UNIVERSAL_CHARSTRING& other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring element.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring value.");
  if (other_value.charstring) {
    if (other_value.cstr.val_ptr->n_chars != 1)
      return FALSE;
    if (str_val.charstring)
      return str_val.cstr.val_ptr->chars_ptr[uchar_pos] ==
        other_value.cstr.val_ptr->chars_ptr[0];
    else {
      const universal_char& uchar = str_val.val_ptr->uchars_ptr[uchar_pos];
      return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0
        && uchar.uc_cell == (cbyte)other_value.cstr.val_ptr->chars_ptr[0];
    }
  } else {
    if (other_value.val_ptr->n_uchars != 1)
      return FALSE;
    if (str_val.charstring) {
      const universal_char& uchar = other_value.val_ptr->uchars_ptr[0];
      return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
        uchar.uc_cell == (cbyte)str_val.cstr.val_ptr->chars_ptr[uchar_pos];
    } else
      return str_val.val_ptr->uchars_ptr[uchar_pos] ==
        other_value.val_ptr->uchars_ptr[0];
  }
}

boolean UNIVERSAL_CHARSTRING_ELEMENT::operator==
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of comparison is an unbound universal "
    "charstring element.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring element.");
  if (str_val.charstring) {
    if (other_value.str_val.charstring)
      return str_val.cstr.val_ptr->chars_ptr[uchar_pos] ==
        other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
    else {
      const universal_char& uchar = other_value.get_uchar();
      return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
        uchar.uc_cell == str_val.cstr.val_ptr->chars_ptr[uchar_pos];
    }
  } else {
    if (other_value.str_val.charstring) {
      const universal_char& uchar = str_val.val_ptr->uchars_ptr[uchar_pos];
      return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
        uchar.uc_cell ==
        other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
    } else
      return str_val.val_ptr->uchars_ptr[uchar_pos] ==
        other_value.str_val.val_ptr->uchars_ptr[other_value.uchar_pos];
  }
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING_ELEMENT::operator+
  (const universal_char& other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring element.");
  if (str_val.charstring && other_value.is_char()) {
    UNIVERSAL_CHARSTRING ret_val(2, TRUE);
    ret_val.cstr.val_ptr->chars_ptr[0] =
      str_val.cstr.val_ptr->chars_ptr[uchar_pos];
    ret_val.cstr.val_ptr->chars_ptr[1] = other_value.uc_cell;
    return ret_val;
  } else if (str_val.charstring ^ other_value.is_char()) {
    universal_char result[2];
    if (str_val.charstring) {
      result[0].uc_group = result[0].uc_plane = result[0].uc_row = 0;
      result[0].uc_cell = str_val.cstr.val_ptr->chars_ptr[uchar_pos];
    } else
      result[0] = str_val.val_ptr->uchars_ptr[uchar_pos];
    result[1] = other_value;
    return UNIVERSAL_CHARSTRING(2, result);
  }
  universal_char result[2];
  result[0] = str_val.val_ptr->uchars_ptr[uchar_pos];
  result[1] = other_value;
  return UNIVERSAL_CHARSTRING(2, result);
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING_ELEMENT::operator+
  (const char* other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring element.");
  int other_len;
  if (other_value == NULL) other_len = 0;
  else other_len = strlen(other_value);

  UNIVERSAL_CHARSTRING ret_val(other_len + 1, str_val.charstring);
  if (str_val.charstring) {
    ret_val.cstr.val_ptr->chars_ptr[0] =
      str_val.cstr.val_ptr->chars_ptr[uchar_pos];
    memcpy(ret_val.cstr.val_ptr->chars_ptr + 1, other_value, other_len);
  } else {
    ret_val.val_ptr->uchars_ptr[0] = str_val.val_ptr->uchars_ptr[uchar_pos];
    for (int i = 0; i < other_len; i++) {
      ret_val.val_ptr->uchars_ptr[i + 1].uc_group = 0;
      ret_val.val_ptr->uchars_ptr[i + 1].uc_plane = 0;
      ret_val.val_ptr->uchars_ptr[i + 1].uc_row = 0;
      ret_val.val_ptr->uchars_ptr[i + 1].uc_cell = other_value[i];
    }
  }
  return ret_val;
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING_ELEMENT::operator+
  (const CHARSTRING& other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring element.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "charstring value.");
  UNIVERSAL_CHARSTRING ret_val(other_value.val_ptr->n_chars + 1,
    str_val.charstring);
  if (str_val.charstring) {
    ret_val.cstr.val_ptr->chars_ptr[0] =
      str_val.cstr.val_ptr->chars_ptr[uchar_pos];
    memcpy(ret_val.cstr.val_ptr->chars_ptr + 1, other_value.val_ptr->chars_ptr,
      other_value.val_ptr->n_chars);
  } else {
    ret_val.val_ptr->uchars_ptr[0] = str_val.val_ptr->uchars_ptr[uchar_pos];
    for (int i = 0; i < other_value.val_ptr->n_chars; i++) {
      ret_val.val_ptr->uchars_ptr[i + 1].uc_group = 0;
      ret_val.val_ptr->uchars_ptr[i + 1].uc_plane = 0;
      ret_val.val_ptr->uchars_ptr[i + 1].uc_row = 0;
      ret_val.val_ptr->uchars_ptr[i + 1].uc_cell =
        other_value.val_ptr->chars_ptr[i];
    }
  }
  return ret_val;
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING_ELEMENT::operator+
  (const CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring element.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "charstring element.");
  if (str_val.charstring) {
    UNIVERSAL_CHARSTRING ret_val(2, TRUE);
    ret_val.cstr.val_ptr->chars_ptr[0] =
      str_val.cstr.val_ptr->chars_ptr[uchar_pos];
    ret_val.cstr.val_ptr->chars_ptr[1] = other_value.get_char();
    return ret_val;
  } else {
    universal_char result[2];
    result[0] = str_val.val_ptr->uchars_ptr[uchar_pos];
    result[1].uc_group = 0;
    result[1].uc_plane = 0;
    result[1].uc_row = 0;
    result[1].uc_cell = other_value.get_char();
    return UNIVERSAL_CHARSTRING(2, result);
  }
}

#ifdef TITAN_RUNTIME_2
UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING_ELEMENT::operator+(
  const OPTIONAL<CHARSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const CHARSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of universal charstring "
    "concatenation.");
}
#endif

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING_ELEMENT::operator+
  (const UNIVERSAL_CHARSTRING& other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring element.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring value.");
  if (str_val.charstring) {
    if (other_value.charstring) {
      UNIVERSAL_CHARSTRING ret_val(other_value.cstr.val_ptr->n_chars + 1, TRUE);
      ret_val.cstr.val_ptr->chars_ptr[0] =
        str_val.cstr.val_ptr->chars_ptr[uchar_pos];
      memcpy(ret_val.cstr.val_ptr->chars_ptr + 1,
        other_value.cstr.val_ptr->chars_ptr, other_value.cstr.val_ptr->n_chars);
      return ret_val;
    } else {
      UNIVERSAL_CHARSTRING ret_val(other_value.val_ptr->n_uchars + 1);
      universal_char& uchar = ret_val.val_ptr->uchars_ptr[0];
      uchar.uc_group = uchar.uc_plane = uchar.uc_row = 0;
      uchar.uc_cell = str_val.cstr.val_ptr->chars_ptr[uchar_pos];
      memcpy(ret_val.val_ptr->uchars_ptr + 1, other_value.val_ptr->uchars_ptr,
        other_value.val_ptr->n_uchars * sizeof(universal_char));
      return ret_val;
    }
  } else {
    if (other_value.charstring) {
      UNIVERSAL_CHARSTRING ret_val(other_value.cstr.val_ptr->n_chars + 1);
      ret_val.val_ptr->uchars_ptr[0] = str_val.val_ptr->uchars_ptr[uchar_pos];
      for (int i = 1; i <= other_value.cstr.val_ptr->n_chars; ++i) {
        ret_val.val_ptr->uchars_ptr[i].uc_group = 0;
        ret_val.val_ptr->uchars_ptr[i].uc_plane = 0;
        ret_val.val_ptr->uchars_ptr[i].uc_row = 0;
        ret_val.val_ptr->uchars_ptr[i].uc_cell =
          other_value.cstr.val_ptr->chars_ptr[i-1];
      }
      return ret_val;
    } else {
      UNIVERSAL_CHARSTRING ret_val(other_value.val_ptr->n_uchars + 1);
      ret_val.val_ptr->uchars_ptr[0] = str_val.val_ptr->uchars_ptr[uchar_pos];
      memcpy(ret_val.val_ptr->uchars_ptr + 1, other_value.val_ptr->uchars_ptr,
        other_value.val_ptr->n_uchars * sizeof(universal_char));
      return ret_val;
    }
  }
}

UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING_ELEMENT::operator+
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of concatenation is an unbound universal "
    "charstring element.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring element.");
  if (str_val.charstring) {
    if (other_value.str_val.charstring) {
      UNIVERSAL_CHARSTRING ret_val(2, TRUE);
      ret_val.cstr.val_ptr->chars_ptr[0] =
        str_val.cstr.val_ptr->chars_ptr[uchar_pos];
      ret_val.cstr.val_ptr->chars_ptr[1] =
        other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
      return ret_val;
    } else {
      UNIVERSAL_CHARSTRING ret_val(2);
      universal_char& uchar = ret_val.val_ptr->uchars_ptr[0];
      uchar.uc_group = uchar.uc_plane = uchar.uc_row = 0;
      uchar.uc_cell = str_val.cstr.val_ptr->chars_ptr[uchar_pos];
      ret_val.val_ptr->uchars_ptr[1] =
        other_value.str_val.val_ptr->uchars_ptr[other_value.uchar_pos];
      return ret_val;
    }
  } else {
    if (other_value.str_val.charstring) {
      UNIVERSAL_CHARSTRING ret_val(2);
      ret_val.val_ptr->uchars_ptr[0] = str_val.val_ptr->uchars_ptr[uchar_pos];
      ret_val.val_ptr->uchars_ptr[1].uc_group = 0;
      ret_val.val_ptr->uchars_ptr[1].uc_plane = 0;
      ret_val.val_ptr->uchars_ptr[1].uc_row = 0;
      ret_val.val_ptr->uchars_ptr[1].uc_cell =
        other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
      return ret_val;
    } else {
      universal_char result[2];
      result[0] = str_val.val_ptr->uchars_ptr[uchar_pos];
      result[1] = other_value.str_val.val_ptr->uchars_ptr[other_value.uchar_pos];
      return UNIVERSAL_CHARSTRING(2, result);
    }
  }
}

#ifdef TITAN_RUNTIME_2
UNIVERSAL_CHARSTRING UNIVERSAL_CHARSTRING_ELEMENT::operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const UNIVERSAL_CHARSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of universal charstring "
    "concatenation.");
}
#endif

const universal_char& UNIVERSAL_CHARSTRING_ELEMENT::get_uchar() const
{
  if (str_val.charstring)
    const_cast<UNIVERSAL_CHARSTRING_ELEMENT&>
      (*this).str_val.convert_cstr_to_uni();
  return str_val.val_ptr->uchars_ptr[uchar_pos];
}

void UNIVERSAL_CHARSTRING_ELEMENT::log() const
{
  if (bound_flag) {
    if (str_val.charstring) {
      str_val.cstr[uchar_pos].log();
      return;
    }
    const universal_char& uchar = str_val.val_ptr->uchars_ptr[uchar_pos];
    if (is_printable(uchar)) {
      // the character is printable
      TTCN_Logger::log_char('"');
      TTCN_Logger::log_char_escaped(uchar.uc_cell);
      TTCN_Logger::log_char('"');
    } else {
      // the character is not printable
      TTCN_Logger::log_event("char(%u, %u, %u, %u)",
	uchar.uc_group, uchar.uc_plane, uchar.uc_row, uchar.uc_cell);
    }
  } else TTCN_Logger::log_event_unbound();
}

// global functions

boolean operator==(const universal_char& uchar_value,
  const UNIVERSAL_CHARSTRING& other_value)
{
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring value.");
  if (other_value.charstring) {
    if (other_value.cstr.val_ptr->n_chars != 1) return FALSE;
    else return uchar_value.is_char() &&
      uchar_value.uc_cell == other_value.cstr.val_ptr->chars_ptr[0];
  }
  if (other_value.val_ptr->n_uchars != 1) return FALSE;
  else return uchar_value == other_value.val_ptr->uchars_ptr[0];
}

boolean operator==(const universal_char& uchar_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring element.");
  return uchar_value == other_value.get_uchar();
}

UNIVERSAL_CHARSTRING operator+(const universal_char& uchar_value,
  const UNIVERSAL_CHARSTRING& other_value)
{
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring value.");
  if (other_value.charstring) {
    if (uchar_value.is_char()) {
      UNIVERSAL_CHARSTRING ret_val(other_value.cstr.val_ptr->n_chars + 1, TRUE);
      ret_val.cstr.val_ptr->chars_ptr[0] = uchar_value.uc_cell;
      memcpy(ret_val.cstr.val_ptr->chars_ptr + 1,
        other_value.cstr.val_ptr->chars_ptr, other_value.cstr.val_ptr->n_chars);
      return ret_val;
    } else {
      UNIVERSAL_CHARSTRING ret_val(other_value.val_ptr->n_uchars + 1);
      ret_val.val_ptr->uchars_ptr[0] = uchar_value;
      for (int i = 0; i < other_value.cstr.val_ptr->n_chars; i++) {
        ret_val.val_ptr->uchars_ptr[i+1].uc_group = 0;
        ret_val.val_ptr->uchars_ptr[i+1].uc_plane = 0;
        ret_val.val_ptr->uchars_ptr[i+1].uc_row = 0;
        ret_val.val_ptr->uchars_ptr[i+1].uc_cell =
          other_value.cstr.val_ptr->chars_ptr[i];
      }
      return ret_val;
    }
  } else {
    UNIVERSAL_CHARSTRING ret_val(other_value.val_ptr->n_uchars + 1);
    ret_val.val_ptr->uchars_ptr[0] = uchar_value;
    memcpy(ret_val.val_ptr->uchars_ptr + 1, other_value.val_ptr->uchars_ptr,
      other_value.val_ptr->n_uchars * sizeof(universal_char));
    return ret_val;
  }
}

UNIVERSAL_CHARSTRING operator+(const universal_char& uchar_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring element.");
  if (other_value.str_val.charstring) {
    if (uchar_value.is_char()) {
      char result[2];
      result[0] = uchar_value.uc_cell;
      result[1] =
        other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
      return UNIVERSAL_CHARSTRING(2, result);
    } else {
      universal_char result[2];
      result[0] = uchar_value;
      result[1].uc_group = 0;
      result[1].uc_plane = 0;
      result[1].uc_row = 0;
      result[1].uc_cell =
        other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
      return UNIVERSAL_CHARSTRING(2, result);
    }
  } else {
    universal_char result[2];
    result[0] = uchar_value;
    result[1] = other_value.get_uchar();
    return UNIVERSAL_CHARSTRING(2, result);
  }
}

#ifdef TITAN_RUNTIME_2
UNIVERSAL_CHARSTRING operator+(const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING& right_value)
{
  if (left_value.is_present()) {
    return (const UNIVERSAL_CHARSTRING&)left_value + right_value;
  }
  TTCN_error("Unbound or omitted left operand of universal charstring "
    "concatenation.");
}

UNIVERSAL_CHARSTRING operator+(const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& right_value)
{
  if (left_value.is_present()) {
    return (const UNIVERSAL_CHARSTRING&)left_value + right_value;
  }
  TTCN_error("Unbound or omitted left operand of universal charstring "
    "concatenation.");
}

UNIVERSAL_CHARSTRING operator+(const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const CHARSTRING& right_value)
{
  if (left_value.is_present()) {
    return (const UNIVERSAL_CHARSTRING&)left_value + right_value;
  }
  TTCN_error("Unbound or omitted left operand of universal charstring "
    "concatenation.");
}

UNIVERSAL_CHARSTRING operator+(const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const CHARSTRING_ELEMENT& right_value)
{
  if (left_value.is_present()) {
    return (const UNIVERSAL_CHARSTRING&)left_value + right_value;
  }
  TTCN_error("Unbound or omitted left operand of universal charstring "
    "concatenation.");
}

UNIVERSAL_CHARSTRING operator+(const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const OPTIONAL<CHARSTRING>& right_value)
{
  if (!left_value.is_present()) {
    TTCN_error("Unbound or omitted left operand of universal charstring "
    "concatenation.");
  }
  if (!right_value.is_present()) {
    TTCN_error("Unbound or omitted right operand of universal charstring "
    "concatenation.");
  }
  return (const UNIVERSAL_CHARSTRING&)left_value + (const CHARSTRING&)right_value;
}
#endif // TITAN_RUNTIME_2

boolean operator==(const char *string_value,
  const UNIVERSAL_CHARSTRING& other_value)
{
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring value.");
  int string_len;
  if (string_value == NULL) string_len = 0;
  else string_len = strlen(string_value);
  if (other_value.charstring) {
    return other_value.cstr == string_value;
  } else {
    if (string_len != other_value.val_ptr->n_uchars) return FALSE;
    for (int i = 0; i < string_len; i++) {
      if (other_value.val_ptr->uchars_ptr[i].uc_group != 0 ||
        other_value.val_ptr->uchars_ptr[i].uc_plane != 0 ||
        other_value.val_ptr->uchars_ptr[i].uc_row != 0 ||
        other_value.val_ptr->uchars_ptr[i].uc_cell != string_value[i])
        return FALSE;
    }
  }
  return TRUE;
}

boolean operator==(const char *string_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring element.");
  if (string_value == NULL || string_value[0] == '\0' ||
    string_value[1] != '\0') return FALSE;
  if (other_value.str_val.charstring)
    return other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos]
      == string_value[0];
  const universal_char& uchar = other_value.get_uchar();
  return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
    uchar.uc_cell == string_value[0];
}

UNIVERSAL_CHARSTRING operator+(const char *string_value,
  const UNIVERSAL_CHARSTRING& other_value)
{
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring value.");
  int string_len;
  if (string_value == NULL) string_len = 0;
  else string_len = strlen(string_value);
  if (other_value.charstring) {
    UNIVERSAL_CHARSTRING ret_val(string_len + other_value.cstr.val_ptr->n_chars,
      TRUE);
    memcpy(ret_val.cstr.val_ptr->chars_ptr, string_value, string_len);
    memcpy(ret_val.cstr.val_ptr->chars_ptr + string_len,
      other_value.cstr.val_ptr->chars_ptr, other_value.cstr.val_ptr->n_chars);
    return ret_val;
  }
  UNIVERSAL_CHARSTRING ret_val(string_len + other_value.val_ptr->n_uchars);
  for (int i = 0; i < string_len; i++) {
    ret_val.val_ptr->uchars_ptr[i].uc_group = 0;
    ret_val.val_ptr->uchars_ptr[i].uc_plane = 0;
    ret_val.val_ptr->uchars_ptr[i].uc_row = 0;
    ret_val.val_ptr->uchars_ptr[i].uc_cell = string_value[i];
  }
  memcpy(ret_val.val_ptr->uchars_ptr + string_len,
    other_value.val_ptr->uchars_ptr,
    other_value.val_ptr->n_uchars * sizeof(universal_char));
  return ret_val;
}

UNIVERSAL_CHARSTRING operator+(const char *string_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring element.");
  int string_len;
  if (string_value == NULL) string_len = 0;
  else string_len = strlen(string_value);
  if (other_value.str_val.charstring) {
    UNIVERSAL_CHARSTRING ret_val(string_len + 1, TRUE);
    memcpy(ret_val.cstr.val_ptr->chars_ptr, string_value, string_len);
    ret_val.cstr.val_ptr->chars_ptr[string_len] =
      other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
    return ret_val;
  }
  UNIVERSAL_CHARSTRING ret_val(string_len + 1);
  for (int i = 0; i < string_len; i++) {
    ret_val.val_ptr->uchars_ptr[i].uc_group = 0;
    ret_val.val_ptr->uchars_ptr[i].uc_plane = 0;
    ret_val.val_ptr->uchars_ptr[i].uc_row = 0;
    ret_val.val_ptr->uchars_ptr[i].uc_cell = string_value[i];
  }
  ret_val.val_ptr->uchars_ptr[string_len] = other_value.get_uchar();
  return ret_val;
}

// member functions of class UNIVERSAL_CHARSTRING_template

void UNIVERSAL_CHARSTRING_template::clean_up()
{
  if (template_selection == VALUE_LIST ||
    template_selection == COMPLEMENTED_LIST) delete [] value_list.list_value;
  else if (template_selection == STRING_PATTERN) {
    if (pattern_value.regexp_init) regfree(&pattern_value.posix_regexp);
    delete pattern_string;
  }
  else if (template_selection == DECODE_MATCH) {
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
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void UNIVERSAL_CHARSTRING_template::copy_template
  (const CHARSTRING_template& other_value)
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
    value_list.list_value =
      new UNIVERSAL_CHARSTRING_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
	other_value.value_list.list_value[i]);
    break;
  case VALUE_RANGE:
    if (!other_value.value_range.min_is_set) TTCN_error("The lower bound is "
      "not set when copying a charstring value range template to a universal "
      "charstring template.");
    if (!other_value.value_range.max_is_set) TTCN_error("The upper bound is "
      "not set when copying a charstring value range template to a universal "
      "charstring template.");
    value_range.min_is_set = TRUE;
    value_range.max_is_set = TRUE;
    value_range.min_is_exclusive = other_value.value_range.min_is_exclusive;
    value_range.max_is_exclusive = other_value.value_range.max_is_exclusive;;
    value_range.min_value.uc_group = 0;
    value_range.min_value.uc_plane = 0;
    value_range.min_value.uc_row = 0;
    value_range.min_value.uc_cell = other_value.value_range.min_value;
    value_range.max_value.uc_group = 0;
    value_range.max_value.uc_plane = 0;
    value_range.max_value.uc_row = 0;
    value_range.max_value.uc_cell = other_value.value_range.max_value;
    break;
  case STRING_PATTERN:
    pattern_string = new CHARSTRING(other_value.single_value);
    pattern_value.regexp_init=FALSE;
    pattern_value.nocase = other_value.pattern_value.nocase;
    break;
  case DECODE_MATCH:
    dec_match = other_value.dec_match;
    dec_match->ref_count++;
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported charstring template to a "
      "universal charstring template.");
  }
  set_selection(other_value);
}

void UNIVERSAL_CHARSTRING_template::copy_template
  (const UNIVERSAL_CHARSTRING_template& other_value)
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
    value_list.list_value =
      new UNIVERSAL_CHARSTRING_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
	other_value.value_list.list_value[i]);
    break;
  case VALUE_RANGE:
    if (!other_value.value_range.min_is_set) TTCN_error("The lower bound is "
      "not set when copying a universal charstring value range template.");
    if (!other_value.value_range.max_is_set) TTCN_error("The upper bound is "
      "not set when copying a universal charstring value range template.");
    value_range = other_value.value_range;
    break;
  case STRING_PATTERN:
    pattern_string = new CHARSTRING(*(other_value.pattern_string));
    pattern_value.regexp_init=FALSE;
    pattern_value.nocase = other_value.pattern_value.nocase;
    break;
  case DECODE_MATCH:
    dec_match = other_value.dec_match;
    dec_match->ref_count++;
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported universal charstring "
      "template.");
  }
  set_selection(other_value);
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template()
{
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template
  (template_sel other_value) : Restricted_Length_Template(other_value)

{
  check_single_selection(other_value);
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template
  (const CHARSTRING& other_value) :
  Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template
  (const UNIVERSAL_CHARSTRING& other_value) :
  Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template
  (const CHARSTRING_ELEMENT& other_value) :
  Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) :
  Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template
  (const OPTIONAL<CHARSTRING>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const CHARSTRING&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a universal charstring template from an unbound "
      "optional field.");
  }
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template
  (const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const UNIVERSAL_CHARSTRING&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a universal charstring template from an unbound "
      "optional field.");
  }
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template
  (const CHARSTRING_template& other_value)
: Restricted_Length_Template()
{
  copy_template(other_value);
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template
  (const UNIVERSAL_CHARSTRING_template& other_value)
: Restricted_Length_Template()
{
  copy_template(other_value);
}

UNIVERSAL_CHARSTRING_template::UNIVERSAL_CHARSTRING_template
  (template_sel p_sel, const CHARSTRING& p_str, boolean p_nocase /* = FALSE */)
: Restricted_Length_Template(STRING_PATTERN)
{
  if(p_sel!=STRING_PATTERN)
    TTCN_error("Internal error: Initializing a universal charstring"
      "pattern template with invalid selection.");
  pattern_string = new CHARSTRING(p_str);
  pattern_value.regexp_init=FALSE;
  pattern_value.nocase = p_nocase;
}

UNIVERSAL_CHARSTRING_template::~UNIVERSAL_CHARSTRING_template()
{
  clean_up();
}

UNIVERSAL_CHARSTRING_template& UNIVERSAL_CHARSTRING_template::operator=
  (template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

UNIVERSAL_CHARSTRING_template& UNIVERSAL_CHARSTRING_template::operator=
  (const CHARSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound charstring value to a "
    "universal charstring template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

UNIVERSAL_CHARSTRING_template& UNIVERSAL_CHARSTRING_template::operator=
  (const UNIVERSAL_CHARSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound universal charstring value "
    "to a template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

UNIVERSAL_CHARSTRING_template& UNIVERSAL_CHARSTRING_template::operator=
  (const CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound charstring element to a "
    "universal charstring template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

UNIVERSAL_CHARSTRING_template& UNIVERSAL_CHARSTRING_template::operator=
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound universal charstring "
    "element to a template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

UNIVERSAL_CHARSTRING_template& UNIVERSAL_CHARSTRING_template::operator=
  (const OPTIONAL<CHARSTRING>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const CHARSTRING&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a universal "
      "charstring template.");
  }
  return *this;
}

UNIVERSAL_CHARSTRING_template& UNIVERSAL_CHARSTRING_template::operator=
  (const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    set_selection(SPECIFIC_VALUE);
    single_value = (const UNIVERSAL_CHARSTRING&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a universal "
      "charstring template.");
  }
  return *this;
}

UNIVERSAL_CHARSTRING_template& UNIVERSAL_CHARSTRING_template::operator=
  (const CHARSTRING_template& other_value)
{
  clean_up();
  copy_template(other_value);
  return *this;
}

UNIVERSAL_CHARSTRING_template& UNIVERSAL_CHARSTRING_template::operator=
  (const UNIVERSAL_CHARSTRING_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

#ifdef TITAN_RUNTIME_2
UNIVERSAL_CHARSTRING_template UNIVERSAL_CHARSTRING_template::operator+(
  const UNIVERSAL_CHARSTRING_template& other_value) const
{
  if (template_selection == SPECIFIC_VALUE &&
      other_value.template_selection == SPECIFIC_VALUE) {
    return single_value + other_value.single_value;
  }
  TTCN_error("Operand of universal charstring template concatenation is an "
    "uninitialized or unsupported template.");
}

UNIVERSAL_CHARSTRING_template UNIVERSAL_CHARSTRING_template::operator+(
  const UNIVERSAL_CHARSTRING& other_value) const
{
  if (template_selection == SPECIFIC_VALUE) {
    return single_value + other_value;
  }
  TTCN_error("Operand of universal charstring template concatenation is an "
    "uninitialized or unsupported template.");
}

UNIVERSAL_CHARSTRING_template UNIVERSAL_CHARSTRING_template::operator+(
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const
{
  if (template_selection == SPECIFIC_VALUE) {
    return single_value + other_value;
  }
  TTCN_error("Operand of universal charstring template concatenation is an "
    "uninitialized or unsupported template.");
}

UNIVERSAL_CHARSTRING_template UNIVERSAL_CHARSTRING_template::operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const
{
  if (template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  if (!other_value.is_present()) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "unbound or omitted record/set field.");
  }
  return single_value + (const UNIVERSAL_CHARSTRING&)other_value;
}

UNIVERSAL_CHARSTRING_template UNIVERSAL_CHARSTRING_template::operator+(
  const CHARSTRING& other_value) const
{
  if (template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return single_value + other_value;
}

UNIVERSAL_CHARSTRING_template UNIVERSAL_CHARSTRING_template::operator+(
  const CHARSTRING_ELEMENT& other_value) const
{
  if (template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return single_value + other_value;
}

UNIVERSAL_CHARSTRING_template UNIVERSAL_CHARSTRING_template::operator+(
  const OPTIONAL<CHARSTRING>& other_value) const
{
  if (template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  if (!other_value.is_present()) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "unbound or omitted record/set field.");
  }
  return single_value + (const CHARSTRING&)other_value;
}

UNIVERSAL_CHARSTRING_template UNIVERSAL_CHARSTRING_template::operator+(
    const CHARSTRING_template& other_value) const
{
  if (template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  if (other_value.template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return single_value + other_value.single_value;
}

UNIVERSAL_CHARSTRING_template operator+(const UNIVERSAL_CHARSTRING& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template)
{
  if (right_template.template_selection == SPECIFIC_VALUE) {
    return left_value + right_template.single_value;
  }
  TTCN_error("Operand of universal charstring template concatenation is an "
    "uninitialized or unsupported template.");
}

UNIVERSAL_CHARSTRING_template operator+(
  const UNIVERSAL_CHARSTRING_ELEMENT& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template)
{
  if (right_template.template_selection == SPECIFIC_VALUE) {
    return left_value + right_template.single_value;
  }
  TTCN_error("Operand of universal charstring template concatenation is an "
    "uninitialized or unsupported template.");
}

UNIVERSAL_CHARSTRING_template operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template)
{
  if (!left_value.is_present()) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "unbound or omitted record/set field.");
  }
  if (right_template.template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return (const UNIVERSAL_CHARSTRING&)left_value + right_template.single_value;
}

UNIVERSAL_CHARSTRING_template operator+(const CHARSTRING& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template)
{
  if (right_template.template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return left_value + right_template.single_value;
}

UNIVERSAL_CHARSTRING_template operator+(const CHARSTRING_ELEMENT& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template)
{
  if (right_template.template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return left_value + right_template.single_value;
}

UNIVERSAL_CHARSTRING_template operator+(const OPTIONAL<CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template)
{
  if (!left_value.is_present()) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "unbound or omitted record/set field.");
  }
  if (right_template.template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return (const CHARSTRING&)left_value + right_template.single_value;
}

UNIVERSAL_CHARSTRING_template operator+(const CHARSTRING_template& left_template,
  const UNIVERSAL_CHARSTRING_template& right_template)
{
  if (left_template.template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  if (right_template.template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of universal charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return left_template.single_value + right_template.single_value;
}
#endif // TITAN_RUNTIME_2

UNIVERSAL_CHARSTRING_ELEMENT UNIVERSAL_CHARSTRING_template::operator[](int index_value)
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Accessing a universal charstring element of a non-specific "
               "universal charstring template.");
  return single_value[index_value];
}

UNIVERSAL_CHARSTRING_ELEMENT UNIVERSAL_CHARSTRING_template::operator[](const INTEGER& index_value)
{
  index_value.must_bound("Indexing a universal charstring template with an "
                         "unbound integer value.");
  return (*this)[(int)index_value];
}

const UNIVERSAL_CHARSTRING_ELEMENT UNIVERSAL_CHARSTRING_template::operator[](int index_value) const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Accessing a universal charstring element of a non-specific "
               "universal charstring template.");
  return single_value[index_value];
}

const UNIVERSAL_CHARSTRING_ELEMENT UNIVERSAL_CHARSTRING_template::operator[](const INTEGER& index_value) const
{
  index_value.must_bound("Indexing a universal charstring template with an "
                         "unbound integer value.");
  return (*this)[(int)index_value];
}

boolean UNIVERSAL_CHARSTRING_template::match
  (const UNIVERSAL_CHARSTRING& other_value, boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  int value_length = other_value.lengthof();
  if (!match_length(value_length)) return FALSE;
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
  case VALUE_RANGE: {
    if (!value_range.min_is_set) TTCN_error("The lower bound is not set when "
      "matching with a universal charstring value range template.");
    if (!value_range.max_is_set) TTCN_error("The upper bound is not set when "
      "matching with a universal charstring value range template.");
    if (value_range.max_value < value_range.min_value)
      TTCN_error("The lower bound is greater than the upper bound "
	"when matching with a universal charstring value range template.");
    const universal_char *uchars_ptr = other_value;
    for (int i = 0; i < value_length; i++) {
      if (uchars_ptr[i] < value_range.min_value ||
        value_range.max_value < uchars_ptr[i]) {
        return FALSE;
      } else if ((value_range.min_is_exclusive && uchars_ptr[i] == value_range.min_value) ||
                 (value_range.max_is_exclusive && uchars_ptr[i] == value_range.max_value)) {
        return FALSE;
      }
    }
    return TRUE; }
  case STRING_PATTERN: {
    if (!pattern_value.regexp_init) {
      char *posix_str =
        TTCN_pattern_to_regexp_uni((const char*)(*pattern_string),
          pattern_value.nocase);
      if(posix_str==NULL) {
        TTCN_error("Cannot convert pattern \"%s\" to POSIX-equivalent.",
          (const char*)(*pattern_string));
      }
      int ret_val=regcomp(&pattern_value.posix_regexp, posix_str,
        REG_EXTENDED|REG_NOSUB);
      Free(posix_str);
      if(ret_val!=0) {
        /* regexp error */
        char msg[ERRMSG_BUFSIZE];
        regerror(ret_val, &pattern_value.posix_regexp, msg, ERRMSG_BUFSIZE);
        regfree(&pattern_value.posix_regexp);
        TTCN_error("Pattern matching error: %s", msg);
      }
      pattern_value.regexp_init=TRUE;
    }
    char* other_value_converted = other_value.convert_to_regexp_form();
    if (pattern_value.nocase) {
      unichar_pattern.convert_regex_str_to_lowercase(other_value_converted);
    }
    int ret_val=regexec(&pattern_value.posix_regexp, other_value_converted, 0,
      NULL, 0);
    Free(other_value_converted);
    switch (ret_val) {
    case 0:
      return TRUE;
    case REG_NOMATCH:
      return FALSE;
    default:
      /* regexp error */
      char msg[ERRMSG_BUFSIZE];
      regerror(ret_val, &pattern_value.posix_regexp, msg, ERRMSG_BUFSIZE);
      TTCN_error("Pattern matching error: %s", msg);
    }
    break;}
  case DECODE_MATCH: {
    TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);
    TTCN_EncDec::clear_error();
    TTCN_Buffer buff;
    switch (dec_match->coding) {
    case CharCoding::UTF_8:
      other_value.encode_utf8(buff, FALSE);
      break;
    case CharCoding::UTF16:
    case CharCoding::UTF16LE:
    case CharCoding::UTF16BE:
      other_value.encode_utf16(buff, dec_match->coding);
      break;
    case CharCoding::UTF32:
    case CharCoding::UTF32LE:
    case CharCoding::UTF32BE:
      other_value.encode_utf32(buff, dec_match->coding);
      break;
    default:
      TTCN_error("Internal error: Invalid string serialization for "
        "universal charstring template with decoded content matching.");
    }
    boolean ret_val = dec_match->instance->match(buff);
    TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL,TTCN_EncDec::EB_DEFAULT);
    TTCN_EncDec::clear_error();
    return ret_val; }
  default:
    TTCN_error("Matching with an uninitialized/unsupported universal "
      "charstring template.");
  }
  return FALSE;
}

const UNIVERSAL_CHARSTRING& UNIVERSAL_CHARSTRING_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific "
      "universal charstring template.");
  return single_value;
}

int UNIVERSAL_CHARSTRING_template::lengthof() const
{
  int min_length;
  boolean has_any_or_none;
  if (is_ifpresent)
    TTCN_error("Performing lengthof() operation on a universal charstring "
               "template which has an ifpresent attribute.");
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
    min_length = single_value.lengthof();
    has_any_or_none = FALSE;
    break;
  case OMIT_VALUE:
    TTCN_error("Performing lengthof() operation on a universal charstring "
               "template containing omit value.");
  case ANY_VALUE:
  case ANY_OR_OMIT:
  case VALUE_RANGE:
    min_length = 0;
    has_any_or_none = TRUE; // max. length is infinity
    break;
  case VALUE_LIST:
  {
    // error if any element does not have length or the lengths differ
    if (value_list.n_values<1)
      TTCN_error("Internal error: "
                 "Performing lengthof() operation on a universal charstring "
                 "template containing an empty list.");
    int item_length = value_list.list_value[0].lengthof();
    for (unsigned int i = 1; i < value_list.n_values; i++) {
      if (value_list.list_value[i].lengthof()!=item_length)
        TTCN_error("Performing lengthof() operation on a universal charstring "
                   "template containing a value list with different lengths.");
    }
    min_length = item_length;
    has_any_or_none = FALSE;
    break;
  }
  case COMPLEMENTED_LIST:
    TTCN_error("Performing lengthof() operation on a universal charstring "
               "template containing complemented list.");
  case STRING_PATTERN:
    TTCN_error("Performing lengthof() operation on a universal charstring "
               "template containing a pattern is not allowed.");
  default:
    TTCN_error("Performing lengthof() operation on an "
               "uninitialized/unsupported universal charstring template.");
  }
  return check_section_is_single(min_length, has_any_or_none,
                                "length", "a", "universal charstring template");
}

void UNIVERSAL_CHARSTRING_template::set_type(template_sel template_type,
  unsigned int list_length)
{
  clean_up();
  switch (template_type) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    set_selection(template_type);
    value_list.n_values = list_length;
    value_list.list_value = new UNIVERSAL_CHARSTRING_template[list_length];
    break;
  case VALUE_RANGE:
    set_selection(VALUE_RANGE);
    value_range.min_is_set = FALSE;
    value_range.max_is_set = FALSE;
    value_range.min_is_exclusive = FALSE;
    value_range.max_is_exclusive = FALSE;
    break;
  case DECODE_MATCH:
    set_selection(DECODE_MATCH);
    break;
  default:
    TTCN_error("Setting an invalid type for a universal charstring template.");
  }
}

UNIVERSAL_CHARSTRING_template& UNIVERSAL_CHARSTRING_template::list_item
  (unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
      template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list universal charstring "
      "template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a universal charstring value list template.");
  return value_list.list_value[list_index];
}

void UNIVERSAL_CHARSTRING_template::set_min
  (const UNIVERSAL_CHARSTRING& min_value)
{
  if (template_selection != VALUE_RANGE) TTCN_error("Setting the lower bound "
    "for a non-range universal charstring template.");
  min_value.must_bound("Setting an unbound value as lower bound in a "
    "universal charstring value range template.");
  int length = min_value.lengthof();
  if (length != 1) TTCN_error("The length of the lower bound in a universal "
    "charstring value range template must be 1 instead of %d.", length);
  value_range.min_is_set = TRUE;
  value_range.min_is_exclusive = FALSE;
  value_range.min_value = *(const universal_char*)min_value;
  if (value_range.max_is_set && value_range.max_value < value_range.min_value)
    TTCN_error("The lower bound in a universal charstring value range template "
      "is greater than the upper bound.");
}

void UNIVERSAL_CHARSTRING_template::set_max
  (const UNIVERSAL_CHARSTRING& max_value)
{
  if (template_selection != VALUE_RANGE) TTCN_error("Setting the upper bound "
    "for a non-range universal charstring template.");
  max_value.must_bound("Setting an unbound value as upper bound in a "
    "universal charstring value range template.");
  int length = max_value.lengthof();
  if (length != 1) TTCN_error("The length of the upper bound in a universal "
    "charstring value range template must be 1 instead of %d.", length);
  value_range.max_is_set = TRUE;
  value_range.max_is_exclusive = FALSE;
  value_range.max_value = *(const universal_char*)max_value;
  if (value_range.min_is_set && value_range.max_value < value_range.min_value)
    TTCN_error("The upper bound in a universal charstring value range template "
      "is smaller than the lower bound.");
}

void UNIVERSAL_CHARSTRING_template::set_min_exclusive(boolean min_exclusive)
{
  if (template_selection != VALUE_RANGE) TTCN_error("Setting the lower bound "
    " exclusiveness for a non-range universal charstring template.");
  value_range.min_is_exclusive = min_exclusive;
}

void UNIVERSAL_CHARSTRING_template::set_max_exclusive(boolean max_exclusive)
{
  if (template_selection != VALUE_RANGE) TTCN_error("Setting the upper bound "
    " exclusiveness for a non-range universal charstring template.");
  value_range.max_is_exclusive = max_exclusive;
}

void UNIVERSAL_CHARSTRING_template::set_decmatch(Dec_Match_Interface* new_instance,
                                                 const char* coding_str /* = NULL */)
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Setting the decoded content matching mechanism of a non-decmatch "
      "universal charstring template.");
  }
  CharCoding::CharCodingType new_coding = UNIVERSAL_CHARSTRING::get_character_coding(
    coding_str, "decoded content match");
  dec_match = new unichar_decmatch_struct;
  dec_match->ref_count = 1;
  dec_match->instance = new_instance;
  dec_match->coding = new_coding;
}

void* UNIVERSAL_CHARSTRING_template::get_decmatch_dec_res() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the decoding result of a non-decmatch universal "
      "charstring template.");
  }
  return dec_match->instance->get_dec_res();
}

CharCoding::CharCodingType UNIVERSAL_CHARSTRING_template::get_decmatch_str_enc() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the encoding format of a non-decmatch universal "
      "charstring template.");
  }
  return dec_match->coding;
}

const TTCN_Typedescriptor_t* UNIVERSAL_CHARSTRING_template::get_decmatch_type_descr() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the decoded type's descriptor in a non-decmatch "
      "universal charstring template.");
  }
  return dec_match->instance->get_type_descr();
}

void UNIVERSAL_CHARSTRING_template::log() const
{
  switch (template_selection) {
  case STRING_PATTERN:
    CHARSTRING_template::log_pattern(pattern_string->lengthof(),
      (const char*)*pattern_string, pattern_value.nocase);
    break;
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
  case VALUE_RANGE:
    TTCN_Logger::log_char('(');
    if (value_range.min_is_exclusive) TTCN_Logger::log_char('!');
    if (value_range.min_is_set) {
      if (is_printable(value_range.min_value)) {
	TTCN_Logger::log_char('"');
	TTCN_Logger::log_char_escaped(value_range.min_value.uc_cell);
	TTCN_Logger::log_char('"');
      } else {
	TTCN_Logger::log_event("char(%u, %u, %u, %u)",
	  value_range.min_value.uc_group, value_range.min_value.uc_plane,
	  value_range.min_value.uc_row, value_range.min_value.uc_cell);
      }
    } else TTCN_Logger::log_event_str("<unknown lower bound>");
    TTCN_Logger::log_event_str(" .. ");
    if (value_range.max_is_exclusive) TTCN_Logger::log_char('!');
    if (value_range.max_is_set) {
      if (is_printable(value_range.max_value)) {
	TTCN_Logger::log_char('"');
	TTCN_Logger::log_char_escaped(value_range.max_value.uc_cell);
	TTCN_Logger::log_char('"');
      } else {
	TTCN_Logger::log_event("char(%u, %u, %u, %u)",
	  value_range.max_value.uc_group, value_range.max_value.uc_plane,
	  value_range.max_value.uc_row, value_range.max_value.uc_cell);
      }
    } else TTCN_Logger::log_event_str("<unknown upper bound>");
    TTCN_Logger::log_char(')');
    break;
  case DECODE_MATCH:
    TTCN_Logger::log_event_str("decmatch(");
    switch (dec_match->coding) {
    case CharCoding::UTF_8:
      TTCN_Logger::log_event_str("UTF-8");
      break;
    case CharCoding::UTF16:
      TTCN_Logger::log_event_str("UTF-16");
      break;
    case CharCoding::UTF16LE:
      TTCN_Logger::log_event_str("UTF-16LE");
      break;
    case CharCoding::UTF16BE:
      TTCN_Logger::log_event_str("UTF-16BE");
      break;
    case CharCoding::UTF32:
      TTCN_Logger::log_event_str("UTF-32");
      break;
    case CharCoding::UTF32LE:
      TTCN_Logger::log_event_str("UTF-32LE");
      break;
    case CharCoding::UTF32BE:
      TTCN_Logger::log_event_str("UTF-32BE");
      break;
    default:
      TTCN_Logger::log_event_str("<unknown coding>");
      break;
    }
    TTCN_Logger::log_event_str(") ");
    dec_match->instance->log();
    break;
  default:
    log_generic();
    break;
  }
  log_restricted();
  log_ifpresent();
}

void UNIVERSAL_CHARSTRING_template::log_match
  (const UNIVERSAL_CHARSTRING& match_value, boolean /* legacy */) const
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

void UNIVERSAL_CHARSTRING_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE|Module_Param::BC_LIST, "universal charstring template");
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
    UNIVERSAL_CHARSTRING_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Charstring: {
    TTCN_Buffer buff;
    buff.put_s(mp->get_string_size(), (unsigned char*)mp->get_string_data());
    *this = UNIVERSAL_CHARSTRING::from_UTF8_buffer(buff);
    break; }
  case Module_Param::MP_Universal_Charstring:
    *this = UNIVERSAL_CHARSTRING(mp->get_string_size(), (universal_char*)mp->get_string_data());
    break;
  case Module_Param::MP_StringRange: {
    universal_char lower_uchar = mp->get_lower_uchar();
    universal_char upper_uchar = mp->get_upper_uchar();
    clean_up();
    set_selection(VALUE_RANGE);
    value_range.min_is_set = TRUE;
    value_range.max_is_set = TRUE;
    value_range.min_value = lower_uchar;
    value_range.max_value = upper_uchar;
    set_min_exclusive(mp->get_is_min_exclusive());
    set_max_exclusive(mp->get_is_max_exclusive());
  } break;
  case Module_Param::MP_Pattern:
    clean_up();
    pattern_string = new CHARSTRING(mp->get_pattern());
    pattern_value.regexp_init = FALSE;
    pattern_value.nocase = mp->get_nocase();
    set_selection(STRING_PATTERN);
    break;
  case Module_Param::MP_Expression:
    if (mp->get_expr_type() == Module_Param::EXPR_CONCATENATE) {
      UNIVERSAL_CHARSTRING operand1, operand2, result;
      boolean nocase;
      boolean is_pattern = operand1.set_param_internal(*mp->get_operand1(),
        TRUE, &nocase);
      operand2.set_param(*mp->get_operand2());
      result = operand1 + operand2;
      if (is_pattern) {
        clean_up();
        if (result.charstring) {
          pattern_string = new CHARSTRING(result.cstr);
        }
        else {
          pattern_string = new CHARSTRING(result.get_stringRepr_for_pattern());
        }
        pattern_value.regexp_init = FALSE;
        pattern_value.nocase = nocase;
        set_selection(STRING_PATTERN);
      }
      else {
        *this = result;
      }
    }
    else {
      param.expr_type_error("a charstring");
    }
    break;
  default:
    param.type_error("universal charstring template");
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
Module_Param* UNIVERSAL_CHARSTRING_template::get_param(Module_Param_Name& param_name) const
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
  case VALUE_RANGE:
    mp = new Module_Param_StringRange(value_range.min_value, value_range.max_value, value_range.min_is_exclusive, value_range.max_is_exclusive);
    break;
  case STRING_PATTERN:
    mp = new Module_Param_Pattern(mcopystr(*pattern_string), pattern_value.nocase);
    break;
  case DECODE_MATCH:
    TTCN_error("Referencing a decoded content matching template is not supported.");
    break;
  default:
    TTCN_error("Referencing an uninitialized/unsupported universal charstring template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  mp->set_length_restriction(get_length_range());
  return mp;
}
#endif

void UNIVERSAL_CHARSTRING_template::encode_text(Text_Buf& text_buf) const
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
  case VALUE_RANGE: {
    if (!value_range.min_is_set) TTCN_error("Text encoder: The lower bound is "
      "not set in a universal charstring value range template.");
    if (!value_range.max_is_set) TTCN_error("Text encoder: The upper bound is "
      "not set in a universal charstring value range template.");
    unsigned char buf[8];
    buf[0] = value_range.min_value.uc_group;
    buf[1] = value_range.min_value.uc_plane;
    buf[2] = value_range.min_value.uc_row;
    buf[3] = value_range.min_value.uc_cell;
    buf[4] = value_range.max_value.uc_group;
    buf[5] = value_range.max_value.uc_plane;
    buf[6] = value_range.max_value.uc_row;
    buf[7] = value_range.max_value.uc_cell;
    text_buf.push_raw(8, buf);
    break; }
  case STRING_PATTERN:
    text_buf.push_int(pattern_value.nocase);
    pattern_string->encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported universal "
      "charstring template.");
  }
}

void UNIVERSAL_CHARSTRING_template::decode_text(Text_Buf& text_buf)
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
    value_list.list_value =
      new UNIVERSAL_CHARSTRING_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  case VALUE_RANGE: {
    unsigned char buf[8];
    text_buf.pull_raw(8, buf);
    value_range.min_value.uc_group = buf[0];
    value_range.min_value.uc_plane = buf[1];
    value_range.min_value.uc_row = buf[2];
    value_range.min_value.uc_cell = buf[3];
    value_range.max_value.uc_group = buf[4];
    value_range.max_value.uc_plane = buf[5];
    value_range.max_value.uc_row = buf[6];
    value_range.max_value.uc_cell = buf[7];
    if (value_range.max_value < value_range.min_value)
      TTCN_error("Text decoder: The received lower bound is greater than the "
      "upper bound in a universal charstring value range template.");
    value_range.min_is_set = TRUE;
    value_range.max_is_set = TRUE;
    value_range.min_is_exclusive = FALSE;
    value_range.max_is_exclusive = FALSE;
    break; }
  case STRING_PATTERN:
    pattern_value.regexp_init = FALSE;
    pattern_value.nocase = text_buf.pull_int().get_val();
    pattern_string = new CHARSTRING;
    pattern_string->decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was "
      "received for a universal charstring template.");
  }
}

boolean UNIVERSAL_CHARSTRING_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean UNIVERSAL_CHARSTRING_template::match_omit(boolean legacy /* = FALSE */) const
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
void UNIVERSAL_CHARSTRING_template::check_restriction(template_res t_res,
  const char* t_name, boolean legacy) const
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
             get_res_name(t_res), t_name ? t_name : "universal charstring");
}
#endif

const CHARSTRING& UNIVERSAL_CHARSTRING_template::get_single_value() const {
  if (!pattern_string)
    TTCN_error("Pattern string does not exist in universal charstring template");
  return *pattern_string;
}

/**
 * ISO 2022 conversion functions -- these can be overwritten by users
 * by defining their own versions. Because these --of course :)-- are
 * not really conform to ISO 2022. :o)
 */

/** TTCN_UCSTR_2_ISO2022: A dummy common implementation. */
OCTETSTRING TTCN_UCSTR_2_ISO2022(const UNIVERSAL_CHARSTRING& p_s)
{
  const universal_char* ucstr=static_cast<const universal_char*>(p_s);
  int len=p_s.lengthof();
  unsigned char* osstr=(unsigned char*)Malloc(len);
  for(int i=0; i<len; i++) osstr[i]=ucstr[i].uc_cell;
  OCTETSTRING os(len, osstr);
  Free(osstr);
  return os;
}

/** TTCN_ISO2022_2_UCSTR: A dummy common implementation. */
UNIVERSAL_CHARSTRING TTCN_ISO2022_2_UCSTR(const OCTETSTRING& p_os)
{
  const unsigned char* osstr=static_cast<const unsigned char*>(p_os);
  int len=p_os.lengthof();
  universal_char* ucstr=(universal_char*)Malloc(len*sizeof(*ucstr));
  for(int i=0; i<len; i++) {
    ucstr[i].uc_group=0;
    ucstr[i].uc_plane=0;
    ucstr[i].uc_row=0;
    ucstr[i].uc_cell=osstr[i];
  }
  UNIVERSAL_CHARSTRING us(len, ucstr);
  Free(ucstr);
  return us;
}

OCTETSTRING TTCN_TeletexString_2_ISO2022(const TeletexString& p_s)
{
  return TTCN_UCSTR_2_ISO2022(p_s);
}

TeletexString TTCN_ISO2022_2_TeletexString(const OCTETSTRING& p_os)
{
  return TTCN_ISO2022_2_UCSTR(p_os);
}

OCTETSTRING TTCN_VideotexString_2_ISO2022(const VideotexString& p_s)
{
  return TTCN_UCSTR_2_ISO2022(p_s);
}

VideotexString TTCN_ISO2022_2_VideotexString(const OCTETSTRING& p_os)
{
  return TTCN_ISO2022_2_UCSTR(p_os);
}

OCTETSTRING TTCN_GraphicString_2_ISO2022(const GraphicString& p_s)
{
  return TTCN_UCSTR_2_ISO2022(p_s);
}

GraphicString TTCN_ISO2022_2_GraphicString(const OCTETSTRING& p_os)
{
  return TTCN_ISO2022_2_UCSTR(p_os);
}

OCTETSTRING TTCN_GeneralString_2_ISO2022(const GeneralString& p_s)
{
  return TTCN_UCSTR_2_ISO2022(p_s);
}

GeneralString TTCN_ISO2022_2_GeneralString(const OCTETSTRING& p_os)
{
  return TTCN_ISO2022_2_UCSTR(p_os);
}
