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
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include "Charstring.hh"
#include "../common/memory.h"
#include "../common/pattern.hh"
#include "Integer.hh"
#include "Octetstring.hh"
#include "String_struct.hh"
#include "Parameters.h"
#include "Param_Types.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Encdec.hh"
#include "RAW.hh"
#include "BER.hh"
#include "TEXT.hh"
#include "XER.hh"
#include "JSON.hh"
#include "Addfunc.hh"
#include "Optional.hh"

#include "../common/dbgnew.hh"

#include <string.h>
#include <ctype.h>
#include <limits.h>

#define ERRMSG_BUFSIZE 500

/** The amount of memory needed for a string containing n characters.
 * -sizeof(int) because that much is already in charstring_struct.
 * +1 for the terminating null character. */
#define MEMORY_SIZE(n) (sizeof(charstring_struct) - sizeof(int) + 1 + (n))

/** Allocate the memory needed to hold n_chars characters.
 *
 * @param \p n_chars the number of characters to hold
 * @pre \p n_chars must be >= 0
 */
void CHARSTRING::init_struct(int n_chars)
{
  if (n_chars < 0) {
    val_ptr = NULL;
    TTCN_error("Initializing a charstring with a negative length.");
  } else if (n_chars == 0) {
    /** This will represent the empty strings so they won't need allocated
      * memory, this delays the memory allocation until it is really needed.
      */
    static charstring_struct empty_string = { 1, 0, "" };
    val_ptr = &empty_string;
    empty_string.ref_count++;
  } else {
    val_ptr = (charstring_struct*)Malloc(MEMORY_SIZE(n_chars));
    val_ptr->ref_count = 1;
    val_ptr->n_chars = n_chars;
    val_ptr->chars_ptr[n_chars] = '\0';
  }
}

/** Implement the copy-on-write.
 *
 * Called from the various CHARSTRING_ELEMENT::operator=(), just before
 * the string is about to be modified. Stops the sharing of the CHARSTRING
 * and creates a new copy for modification.
 */
void CHARSTRING::copy_value()
{
  if (val_ptr == NULL || val_ptr->n_chars <= 0)
    TTCN_error("Internal error: Invalid internal data structure when copying "
      "the memory area of a charstring value.");
  if (val_ptr->ref_count > 1) {
    charstring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_chars);
    memcpy(val_ptr->chars_ptr, old_ptr->chars_ptr, old_ptr->n_chars + 1);
  }
}

/** Create an uninitialized CHARSTRING object.
 *
 * Called from CHARSTRING::operator+() to create the return value with
 * enough storage to hold the concatenation of the arguments.
 *
 * @param n_chars the number of characters to hold
 *
 */
CHARSTRING::CHARSTRING(int n_chars)
{
  init_struct(n_chars);
}

CHARSTRING::CHARSTRING()
{
  val_ptr = NULL;
}

CHARSTRING::CHARSTRING(char other_value)
{
  init_struct(1);
  val_ptr->chars_ptr[0] = other_value;
}

CHARSTRING::CHARSTRING(const char *chars_ptr)
{
  int n_chars;
  if (chars_ptr != NULL) n_chars = strlen(chars_ptr);
  else n_chars = 0;
  init_struct(n_chars);
  memcpy(val_ptr->chars_ptr, chars_ptr, n_chars);
}

CHARSTRING::CHARSTRING(int n_chars, const char *chars_ptr)
{
  init_struct(n_chars);
  memcpy(val_ptr->chars_ptr, chars_ptr, n_chars);
}

CHARSTRING::CHARSTRING(const CHARSTRING& other_value)
: Base_Type(other_value), val_ptr(other_value.val_ptr)
{
  other_value.must_bound("Copying an unbound charstring value.");
  // ref_count can only be incremented after we check val_ptr
  val_ptr->ref_count++;
}

CHARSTRING::CHARSTRING(const CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Initialization of a charstring with an unbound "
                         "charstring element.");
  init_struct(1);
  val_ptr->chars_ptr[0] = other_value.get_char();
}

CHARSTRING::~CHARSTRING()
{
  clean_up();
}

void CHARSTRING::clean_up()
{
  if (val_ptr != NULL) {
    if (val_ptr->ref_count > 1) val_ptr->ref_count--;
    else if (val_ptr->ref_count == 1) Free(val_ptr);
    else TTCN_error("Internal error: Invalid reference counter in a charstring "
      "value.");
    val_ptr = NULL;
  }
}

CHARSTRING& CHARSTRING::operator=(const char* other_value)
{
  if (val_ptr == NULL || val_ptr->chars_ptr != other_value) {
    clean_up();
    int n_chars;
    if (other_value != NULL) n_chars = strlen(other_value);
    else n_chars = 0;
    init_struct(n_chars);
    memcpy(val_ptr->chars_ptr, other_value, n_chars);
  }
  return *this;
}

CHARSTRING& CHARSTRING::operator=(const CHARSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound charstring value.");
  if (&other_value != this) {
    clean_up();
    val_ptr = other_value.val_ptr;
    val_ptr->ref_count++;
  }
  return *this;
}

CHARSTRING& CHARSTRING::operator=(const CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound charstring element to "
                         "a charstring.");
  char char_value = other_value.get_char();
  clean_up();
  init_struct(1);
  val_ptr->chars_ptr[0] = char_value;
  return *this;
}

CHARSTRING& CHARSTRING::operator=(const UNIVERSAL_CHARSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound universal charstring to "
                         "a charstring.");
  if (other_value.charstring) {
    *this = other_value.cstr;
  }
  else {
    clean_up();
    int n_chars = other_value.val_ptr->n_uchars;
    init_struct(n_chars);
    for (int i = 0; i < n_chars; ++i) {
      const universal_char& uc = other_value.val_ptr->uchars_ptr[i];
      if (uc.uc_group != 0 || uc.uc_plane != 0 || uc.uc_row != 0) {
        TTCN_error("Multiple-byte characters cannot be assigned to a charstring, "
          "invalid character char(%u, %u, %u, %u) at index %d.", 
          uc.uc_group, uc.uc_plane, uc.uc_row, uc.uc_cell, i);
      }
      val_ptr->chars_ptr[i] = other_value.val_ptr->uchars_ptr[i].uc_cell;
    }
  }
  return *this;
}

boolean CHARSTRING::operator==(const char* other_value) const
{
  must_bound("Unbound operand of charstring comparison.");
  if (other_value == NULL) other_value = "";
  return !strcmp(val_ptr->chars_ptr, other_value);
}

boolean CHARSTRING::operator==(const CHARSTRING& other_value) const
{
  must_bound("Unbound operand of charstring comparison.");
  other_value.must_bound("Unbound operand of charstring comparison.");
  if (val_ptr->n_chars != other_value.val_ptr->n_chars) return FALSE;
  else return !memcmp(val_ptr->chars_ptr, other_value.val_ptr->chars_ptr,
                      val_ptr->n_chars);
}

boolean CHARSTRING::operator==(const CHARSTRING_ELEMENT& other_value) const
{
  other_value.must_bound("Unbound operand of charstring element "
                         "comparison.");
  must_bound("Unbound operand of charstring comparison.");
  if (val_ptr->n_chars != 1) return FALSE;
  else return val_ptr->chars_ptr[0] == other_value.get_char();
}

boolean CHARSTRING::operator==(const UNIVERSAL_CHARSTRING& other_value) const
{
  must_bound("The left operand of comparison is an unbound charstring value.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring value.");
  if (other_value.charstring)
    return *this == other_value.cstr;
  if (val_ptr->n_chars != other_value.val_ptr->n_uchars) return FALSE;
  for (int i = 0; i < val_ptr->n_chars; i++) {
    if (other_value.val_ptr->uchars_ptr[i].uc_group != 0 ||
      other_value.val_ptr->uchars_ptr[i].uc_plane != 0 ||
      other_value.val_ptr->uchars_ptr[i].uc_row != 0 ||
      other_value.val_ptr->uchars_ptr[i].uc_cell != (cbyte)val_ptr->chars_ptr[i])
      return FALSE;
  }
  return TRUE;
}

boolean CHARSTRING::operator==(const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
  const
{
  must_bound("The left operand of comparison is an unbound charstring value.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring element.");
  if (val_ptr->n_chars != 1) return FALSE;
  const universal_char& uchar = other_value.get_uchar();
  return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
    uchar.uc_cell == (cbyte)val_ptr->chars_ptr[0];
}

CHARSTRING CHARSTRING::operator+(const char* other_value) const
{
  must_bound("Unbound operand of charstring concatenation.");
  int other_len;
  if (other_value == NULL) other_len = 0;
  else other_len = strlen(other_value);
  if (other_len == 0) return *this;
  CHARSTRING ret_val(val_ptr->n_chars + other_len);
  memcpy(ret_val.val_ptr->chars_ptr, val_ptr->chars_ptr, val_ptr->n_chars);
  memcpy(ret_val.val_ptr->chars_ptr + val_ptr->n_chars, other_value, other_len);
  return ret_val;
}

CHARSTRING CHARSTRING::operator+(const CHARSTRING& other_value) const
{
  must_bound("Unbound operand of charstring concatenation.");
  other_value.must_bound("Unbound operand of charstring concatenation.");
  int first_n_chars = val_ptr->n_chars;
  if (first_n_chars == 0) return other_value;
  int second_n_chars = other_value.val_ptr->n_chars;
  if (second_n_chars == 0) return *this;
  CHARSTRING ret_val(first_n_chars + second_n_chars);
  memcpy(ret_val.val_ptr->chars_ptr, val_ptr->chars_ptr, first_n_chars);
  memcpy(ret_val.val_ptr->chars_ptr + first_n_chars,
         other_value.val_ptr->chars_ptr, second_n_chars);
  return ret_val;
}

CHARSTRING CHARSTRING::operator+(const CHARSTRING_ELEMENT& other_value) const
{
  must_bound("Unbound operand of charstring concatenation.");
  other_value.must_bound("Unbound operand of charstring element "
                         "concatenation.");
  CHARSTRING ret_val(val_ptr->n_chars + 1);
  memcpy(ret_val.val_ptr->chars_ptr, val_ptr->chars_ptr, val_ptr->n_chars);
  ret_val.val_ptr->chars_ptr[val_ptr->n_chars] = other_value.get_char();
  return ret_val;
}

#ifdef TITAN_RUNTIME_2
CHARSTRING CHARSTRING::operator+(const OPTIONAL<CHARSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const CHARSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of charstring concatenation.");
}
#endif

UNIVERSAL_CHARSTRING CHARSTRING::operator+
  (const UNIVERSAL_CHARSTRING& other_value) const
{
  must_bound("The left operand of concatenation is an unbound charstring "
    "value.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring value.");
  if (val_ptr->n_chars == 0) return other_value;
  if (other_value.charstring) {
    UNIVERSAL_CHARSTRING ret_val(val_ptr->n_chars + other_value.cstr.val_ptr->n_chars, TRUE);
    memcpy(ret_val.cstr.val_ptr->chars_ptr, val_ptr->chars_ptr, val_ptr->n_chars);
    memcpy(ret_val.cstr.val_ptr->chars_ptr + val_ptr->n_chars, other_value.cstr.val_ptr->chars_ptr, other_value.cstr.val_ptr->n_chars);
    return ret_val;
  } else {
    UNIVERSAL_CHARSTRING ret_val(val_ptr->n_chars +
      other_value.val_ptr->n_uchars);
    for (int i = 0; i < val_ptr->n_chars; i++) {
      ret_val.val_ptr->uchars_ptr[i].uc_group = 0;
      ret_val.val_ptr->uchars_ptr[i].uc_plane = 0;
      ret_val.val_ptr->uchars_ptr[i].uc_row = 0;
      ret_val.val_ptr->uchars_ptr[i].uc_cell = val_ptr->chars_ptr[i];
    }
    memcpy(ret_val.val_ptr->uchars_ptr + val_ptr->n_chars,
      other_value.val_ptr->uchars_ptr,
      other_value.val_ptr->n_uchars * sizeof(universal_char));
    return ret_val;
  }
}

UNIVERSAL_CHARSTRING CHARSTRING::operator+
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of concatenation is an unbound charstring "
    "value.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring element.");
  if (other_value.str_val.charstring) {
    UNIVERSAL_CHARSTRING ret_val(val_ptr->n_chars + 1, TRUE);
    memcpy(ret_val.cstr.val_ptr->chars_ptr, val_ptr->chars_ptr, val_ptr->n_chars);
    ret_val.cstr.val_ptr->chars_ptr[val_ptr->n_chars] = other_value.str_val.cstr.val_ptr->chars_ptr[other_value.uchar_pos];
    return ret_val;
  } else {
    UNIVERSAL_CHARSTRING ret_val(val_ptr->n_chars + 1);
    for (int i = 0; i < val_ptr->n_chars; i++) {
      ret_val.val_ptr->uchars_ptr[i].uc_group = 0;
      ret_val.val_ptr->uchars_ptr[i].uc_plane = 0;
      ret_val.val_ptr->uchars_ptr[i].uc_row = 0;
      ret_val.val_ptr->uchars_ptr[i].uc_cell = val_ptr->chars_ptr[i];
    }
    ret_val.val_ptr->uchars_ptr[val_ptr->n_chars] = other_value.get_uchar();
    return ret_val;
  }
}

#ifdef TITAN_RUNTIME_2
UNIVERSAL_CHARSTRING CHARSTRING::operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const UNIVERSAL_CHARSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of universal charstring "
    "concatenation.");
}
#endif

CHARSTRING& CHARSTRING::operator+=(char other_value)
{
  must_bound("Appending a character to an unbound charstring value.");
  if (val_ptr->ref_count > 1) {
    charstring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_chars + 1);
    memcpy(val_ptr->chars_ptr, old_ptr->chars_ptr, old_ptr->n_chars);
    val_ptr->chars_ptr[old_ptr->n_chars] = other_value;
  } else {
    val_ptr = (charstring_struct*)
      Realloc(val_ptr, MEMORY_SIZE(val_ptr->n_chars + 1));
    val_ptr->chars_ptr[val_ptr->n_chars] = other_value;
    val_ptr->n_chars++;
    val_ptr->chars_ptr[val_ptr->n_chars] = '\0';
  }
  return *this;
}

CHARSTRING& CHARSTRING::operator+=(const char *other_value)
{
  must_bound("Appending a string literal to an unbound charstring value.");
  if (other_value != NULL) {
    int other_n_chars = strlen(other_value);
    if (other_n_chars > 0) {
      if (val_ptr->ref_count > 1) {
	charstring_struct *old_ptr = val_ptr;
	old_ptr->ref_count--;
	init_struct(old_ptr->n_chars + other_n_chars);
	memcpy(val_ptr->chars_ptr, old_ptr->chars_ptr, old_ptr->n_chars);
	memcpy(val_ptr->chars_ptr + old_ptr->n_chars, other_value,
	  other_n_chars);
      } else {
	if (other_value >= val_ptr->chars_ptr &&
	    other_value <= val_ptr->chars_ptr + val_ptr->n_chars) {
	  int offset = other_value - val_ptr->chars_ptr;
	  val_ptr = (charstring_struct*)
	    Realloc(val_ptr, MEMORY_SIZE(val_ptr->n_chars + other_n_chars));
	  memcpy(val_ptr->chars_ptr + val_ptr->n_chars,
	    val_ptr->chars_ptr + offset, other_n_chars);
	} else {
	  val_ptr = (charstring_struct*)
	    Realloc(val_ptr, MEMORY_SIZE(val_ptr->n_chars + other_n_chars));
	  memcpy(val_ptr->chars_ptr + val_ptr->n_chars, other_value,
	    other_n_chars);
	}
	val_ptr->n_chars += other_n_chars;
	val_ptr->chars_ptr[val_ptr->n_chars] = '\0';
      }
    }
  }
  return *this;
}

CHARSTRING& CHARSTRING::operator+=(const CHARSTRING& other_value)
{
  must_bound("Appending a charstring value to an unbound charstring value.");
  other_value.must_bound("Appending an unbound charstring value to another "
    "charstring value.");
  int other_n_chars = other_value.val_ptr->n_chars;
  if (other_n_chars > 0) {
    if (val_ptr->n_chars == 0) {
      clean_up();
      val_ptr = other_value.val_ptr;
      val_ptr->ref_count++;
    } else if (val_ptr->ref_count > 1) {
      charstring_struct *old_ptr = val_ptr;
      old_ptr->ref_count--;
      init_struct(old_ptr->n_chars + other_n_chars);
      memcpy(val_ptr->chars_ptr, old_ptr->chars_ptr, old_ptr->n_chars);
      memcpy(val_ptr->chars_ptr + old_ptr->n_chars,
	other_value.val_ptr->chars_ptr, other_n_chars);
    } else {
      val_ptr = (charstring_struct*)
	Realloc(val_ptr, MEMORY_SIZE(val_ptr->n_chars + other_n_chars));
      memcpy(val_ptr->chars_ptr + val_ptr->n_chars,
	other_value.val_ptr->chars_ptr, other_n_chars);
      val_ptr->n_chars += other_n_chars;
      val_ptr->chars_ptr[val_ptr->n_chars] = '\0';
    }
  }
  return *this;
}

CHARSTRING& CHARSTRING::operator+=(const CHARSTRING_ELEMENT& other_value)
{
  must_bound("Appending a charstring element to an unbound charstring value.");
  other_value.must_bound("Appending an unbound charstring element to a "
    "charstring value.");
  if (val_ptr->ref_count > 1) {
    charstring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_chars + 1);
    memcpy(val_ptr->chars_ptr, old_ptr->chars_ptr, old_ptr->n_chars);
    val_ptr->chars_ptr[old_ptr->n_chars] = other_value.get_char();
  } else {
    val_ptr = (charstring_struct*)
      Realloc(val_ptr, MEMORY_SIZE(val_ptr->n_chars + 1));
    val_ptr->chars_ptr[val_ptr->n_chars] = other_value.get_char();
    val_ptr->n_chars++;
    val_ptr->chars_ptr[val_ptr->n_chars] = '\0';
  }
  return *this;
}

CHARSTRING CHARSTRING::operator<<=(int rotate_count) const
{
  must_bound("Unbound charstring operand of rotate left operator.");
  if (val_ptr->n_chars == 0) return *this;
  if (rotate_count >= 0) {
    rotate_count %= val_ptr->n_chars;
    if (rotate_count == 0) return *this;
    CHARSTRING ret_val(val_ptr->n_chars);
    memcpy(ret_val.val_ptr->chars_ptr, val_ptr->chars_ptr + rotate_count,
           val_ptr->n_chars - rotate_count);
    memcpy(ret_val.val_ptr->chars_ptr + val_ptr->n_chars - rotate_count,
           val_ptr->chars_ptr, rotate_count);
    return ret_val;
  } else return *this >>= (-rotate_count);
}

CHARSTRING CHARSTRING::operator<<=(const INTEGER& rotate_count) const
{
  rotate_count.must_bound("Unbound integer operand of rotate left "
                          "operator.");
  return *this <<= (int)rotate_count;
}

CHARSTRING CHARSTRING::operator>>=(int rotate_count) const
{
  must_bound("Unbound charstring operand of rotate right operator.");
  if (val_ptr->n_chars == 0) return *this;
  if (rotate_count >= 0) {
    rotate_count %= val_ptr->n_chars;
    if (rotate_count == 0) return *this;
    CHARSTRING ret_val(val_ptr->n_chars);
    memcpy(ret_val.val_ptr->chars_ptr, val_ptr->chars_ptr + val_ptr->n_chars -
           rotate_count, rotate_count);
    memcpy(ret_val.val_ptr->chars_ptr + rotate_count, val_ptr->chars_ptr,
           val_ptr->n_chars - rotate_count);
    return ret_val;
  } else return *this <<= (-rotate_count);
}

CHARSTRING CHARSTRING::operator>>=(const INTEGER& rotate_count) const
{
  rotate_count.must_bound("Unbound integer operand of rotate right "
                          "operator.");
  return *this >>= (int)rotate_count;
}

CHARSTRING_ELEMENT CHARSTRING::operator[](int index_value)
{
  if (val_ptr == NULL && index_value == 0) {
    init_struct(1);
    return CHARSTRING_ELEMENT(FALSE, *this, 0);
  } else {
    must_bound("Accessing an element of an unbound charstring value.");
    if (index_value < 0) TTCN_error("Accessing a charstring element using a "
      "negative index (%d).", index_value);
    int n_chars = val_ptr->n_chars;
    if (index_value > n_chars) TTCN_error("Index overflow when accessing a "
      "charstring element: The index is %d, but the string has only %d "
      "characters.", index_value, n_chars);
    if (index_value == n_chars) {
      if (val_ptr->ref_count == 1) {
	val_ptr = (charstring_struct*)
	  Realloc(val_ptr, MEMORY_SIZE(n_chars + 1));
	val_ptr->n_chars++;
	val_ptr->chars_ptr[val_ptr->n_chars] = '\0';
      } else {
	charstring_struct *old_ptr = val_ptr;
	old_ptr->ref_count--;
	init_struct(n_chars + 1);
	memcpy(val_ptr->chars_ptr, old_ptr->chars_ptr, n_chars);
      }
      return CHARSTRING_ELEMENT(FALSE, *this, index_value);
    } else return CHARSTRING_ELEMENT(TRUE, *this, index_value);
  }
}

CHARSTRING_ELEMENT CHARSTRING::operator[](const INTEGER& index_value)
{
  index_value.must_bound("Indexing a charstring value with an unbound integer "
    "value.");
  return (*this)[(int)index_value];
}

const CHARSTRING_ELEMENT CHARSTRING::operator[](int index_value) const
{
  must_bound("Accessing an element of an unbound charstring value.");
  if (index_value < 0) TTCN_error("Accessing a charstring element using a "
    "negative index (%d).", index_value);
  if (index_value >= val_ptr->n_chars) TTCN_error("Index overflow when "
    "accessing a charstring element: The index is %d, but the string has only "
    "%d characters.", index_value, val_ptr->n_chars);
  return CHARSTRING_ELEMENT(TRUE, const_cast<CHARSTRING&>(*this), index_value);
}

const CHARSTRING_ELEMENT CHARSTRING::operator[](const INTEGER& index_value) const
{
  index_value.must_bound("Indexing a charstring value with an unbound integer "
    "value.");
  return (*this)[(int)index_value];
}

CHARSTRING::operator const char*() const
{
  must_bound("Casting an unbound charstring value to const char*.");
  return val_ptr->chars_ptr;
}

int CHARSTRING::lengthof() const
{
  must_bound("Performing lengthof operation on an unbound charstring value.");
  return val_ptr->n_chars;
}

void CHARSTRING::log() const
{
  if (val_ptr != NULL) {
    expstring_t buffer = 0;
    enum { INIT, PCHAR, NPCHAR } state = INIT;
    for (int i = 0; i < val_ptr->n_chars; i++) {
      char c = val_ptr->chars_ptr[i];
      if (TTCN_Logger::is_printable(c)) {
	// the actual character is printable
	switch (state) {
	case NPCHAR: // concatenation sign if previous part was not printable
	  buffer = mputstr(buffer, " & ");
	  // no break
	case INIT: // opening "
	  buffer = mputc(buffer, '"');
	  // no break
	case PCHAR: // the character itself
	  TTCN_Logger::log_char_escaped(c, buffer);
	  break;
	}
	state = PCHAR;
      } else {
	// the actual character is not printable
	switch (state) {
	case PCHAR: // closing " if previous part was printable
	  buffer = mputc(buffer, '"');
	  // no break
	case NPCHAR: // concatenation sign
	  buffer = mputstr(buffer, " & ");
	  // no break
	case INIT: // the character itself
	  buffer = mputprintf(buffer, "char(0, 0, 0, %u)", (unsigned char)c);
	  break;
	}
	state = NPCHAR;
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

boolean CHARSTRING::set_param_internal(Module_Param& param, boolean allow_pattern,
                                       boolean* is_nocase_pattern) {
  boolean is_pattern = FALSE;
  param.basic_check(Module_Param::BC_VALUE|Module_Param::BC_LIST, "charstring value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Universal_Charstring:
  case Module_Param::MP_Charstring:
    switch (param.get_operation_type()) {
    case Module_Param::OT_ASSIGN:
      clean_up();
      // no break
    case Module_Param::OT_CONCAT: {
      // The universal charstring will decode the string value if it is UTF-8 encoded
      UNIVERSAL_CHARSTRING ucs;
      ucs.set_param(*mp);
      if (ucs.charstring) {
        // No special characters were found
        if (is_bound()) {
          *this = *this + ucs.cstr;
        } else {
          *this = ucs.cstr;
        }
      } else {
        // Special characters found -> check if the UTF-8 decoding resulted in any multi-byte characters
        for (int i = 0; i < ucs.val_ptr->n_uchars; ++i) {
          if (0 != ucs.val_ptr->uchars_ptr[i].uc_group ||
              0 != ucs.val_ptr->uchars_ptr[i].uc_plane ||
              0 != ucs.val_ptr->uchars_ptr[i].uc_row) {
            param.error("Type mismatch: a charstring value without multi-octet characters was expected.");
          }
        }
        CHARSTRING new_cs(ucs.val_ptr->n_uchars);
        for (int i = 0; i < ucs.val_ptr->n_uchars; ++i) {
          new_cs.val_ptr->chars_ptr[i] = ucs.val_ptr->uchars_ptr[i].uc_cell;
        }
        if (is_bound()) {
          *this = *this + new_cs;
        } else {
          *this = new_cs;
        }
      }
      break; }
    default:
      TTCN_error("Internal error: CHARSTRING::set_param()");
    }
    break;
  case Module_Param::MP_Expression:
    if (mp->get_expr_type() == Module_Param::EXPR_CONCATENATE) {
      // only allow string patterns for the first operand
      CHARSTRING operand1, operand2;
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
      param.expr_type_error("a charstring");
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
    param.type_error("charstring value");
    break;
  }
  return is_pattern;
}

void CHARSTRING::set_param(Module_Param& param) {
  set_param_internal(param, FALSE);
}

#ifdef TITAN_RUNTIME_2
Module_Param* CHARSTRING::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  return new Module_Param_Charstring(val_ptr->n_chars, mcopystr(val_ptr->chars_ptr));
}
#endif

void CHARSTRING::encode_text(Text_Buf& text_buf) const
{
  must_bound("Text encoder: Encoding an unbound charstring value.");
  int n_chars = val_ptr->n_chars;
  text_buf.push_int(n_chars);
  if (n_chars > 0) text_buf.push_raw(n_chars, val_ptr->chars_ptr);
}

void CHARSTRING::decode_text(Text_Buf& text_buf)
{
  int n_chars = text_buf.pull_int().get_val();
  if (n_chars < 0)
    TTCN_error("Text decoder: invalid length of a charstring.");
  clean_up();
  init_struct(n_chars);
  if (n_chars > 0) text_buf.pull_raw(n_chars, val_ptr->chars_ptr);
}

void CHARSTRING::encode(const TTCN_Typedescriptor_t& p_td,
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

void CHARSTRING::decode(const TTCN_Typedescriptor_t& p_td,
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
CHARSTRING::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                           unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(!new_tlv) {
    new_tlv=BER_encode_TLV_OCTETSTRING
      (p_coding, val_ptr->n_chars,
       (const unsigned char*)val_ptr->chars_ptr);
  }
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean CHARSTRING::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                                   const ASN_BER_TLV_t& p_tlv,
                                   unsigned L_form)
{
  clean_up();
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec("While decoding CHARSTRING type: ");
  /* Upper estimation for the length. */
  size_t stripped_tlv_len = stripped_tlv.get_len();
  if (stripped_tlv_len < 2) return FALSE;
  int max_len = stripped_tlv_len - 2;
  init_struct(max_len);
  unsigned int octetnum_start = 0;
  BER_decode_TLV_OCTETSTRING(stripped_tlv, L_form, octetnum_start,
                             val_ptr->n_chars,
                             (unsigned char*)val_ptr->chars_ptr);
  if (val_ptr->n_chars < max_len) {
    if (val_ptr->n_chars == 0) {
      clean_up();
      init_struct(0);
    } else {
      val_ptr = (charstring_struct*)
	Realloc(val_ptr, MEMORY_SIZE(val_ptr->n_chars));
      val_ptr->chars_ptr[val_ptr->n_chars] = '\0';
    }
  }
  return TRUE;
}

int CHARSTRING::TEXT_decode(const TTCN_Typedescriptor_t& p_td,
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
  else if (   p_td.text->val.parameters
    &&        p_td.text->val.parameters->decoding_params.min_length != -1) {
    str_len = p_td.text->val.parameters->decoding_params.min_length;
  }
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

  init_struct(str_len);
  memcpy(val_ptr->chars_ptr, buff.get_read_data(), str_len);
  decoded_length += str_len;
  buff.increase_pos(str_len);

  if (  p_td.text->val.parameters
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
  return decoded_length;
}

int CHARSTRING::TEXT_encode(const TTCN_Typedescriptor_t& p_td,
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

  if(p_td.text->val.parameters==NULL){
    buff.put_cs(*this);
    encoded_length+=val_ptr->n_chars;
  } else {
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

  if(p_td.text->end_encode){
    buff.put_cs(*p_td.text->end_encode);
    encoded_length+=p_td.text->end_encode->lengthof();
  }
  return encoded_length;
}

#ifdef TITAN_RUNTIME_2
int CHARSTRING::encode_raw(TTCN_Buffer& p_buf) const
{
  p_buf.put_string(*this);
  return val_ptr ? val_ptr->n_chars : 0;
}

int CHARSTRING::JSON_encode_negtest_raw(JSON_Tokenizer& p_tok) const
{
  if (val_ptr != NULL) {
    p_tok.put_raw_data(val_ptr->chars_ptr, val_ptr->n_chars);
    return val_ptr->n_chars;
  }
  return 0;
}
#endif


void xml_escape(const unsigned int masked_c, TTCN_Buffer& p_buf)
{
  size_t len = 6;
  // length of the majority of the names of control characters, +3 for </>
  static const char *escapes[32] = {
    "<nul/>","<soh/>","<stx/>","<etx/>","<eot/>","<enq/>","<ack/>","<bel/>",
    "<bs/>" ,"<tab/>","<lf/>" ,"<vt/>" ,"<ff/>" ,"<cr/>" ,"<so/>" ,"<si/>" ,
    "<dle/>","<dc1/>","<dc2/>","<dc3/>","<dc4/>","<nak/>","<syn/>","<etb/>",
    "<can/>","<em/>" ,"<sub/>","<esc/>","<is4/>","<is3/>","<is2/>","<is1/>",
  };
  unsigned int c = (masked_c & 0x7FFFFFFF); // unmasked

  switch (c) {
  // XML's "own" characters, escaped according to X.680/2002, 11.15.4 b)
  case '<':
    p_buf.put_s(4, (cbyte*)"&lt;");
    break;

  case '>':
    p_buf.put_s(4, (cbyte*)"&gt;");
    break;

  case '&':
    p_buf.put_s(5, (cbyte*)"&amp;");
    break;

    // Control characters, escaped according to X.680/2002, 11.15.5
  case  8: case 11: case 12: case 14: case 15: case 25:
    // the name of these control characters has only two letters
    --len;
    // no break
  case  0: case  1: case  2: case  3: case  4: case  5: case  6: case  7:
  case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
  case 24:          case 26: case 27: case 28: case 29: case 30: case 31:
    // 9=TAB 10=LF 13=CR are absent from this block altogether
    p_buf.put_s(len, (cbyte*)escapes[c]);
    break;

  case '\"': // HR58225
    p_buf.put_s(6, (cbyte*)"&quot;");
    break;

  case '\'': // X.693 20.3.13: Titan uses single quotes for attributes;
    // so if they appear in content they must be escaped.
    // Currently this happens even if the string is not an attribute.
    p_buf.put_s(6, (cbyte*)"&apos;");
    break;

  case 9: case 10: case 13:
    c = masked_c; // put the mask back on (makes it >127)
    // no break
  default:
    if (c > 127) { // XML numeric entity, as in X.680/2002 11.15.8
      c &= 0x7FFFFFFF; // take it off again
      // Ensure that an even number of hex digits is produced
      int width = (1 + (c > 0xFF) + (c > 0xFFFF) + (c > 0xFFFFFF)) << 1;
      char escapade[16];
      len = snprintf(escapade, 16, "&#x%0*X;", width, c);
      p_buf.put_s(len, (cbyte*)(escapade+0));
    }
    else { // a plain old, unmolested character
      p_buf.put_c(c);
    }
    break;
  }
}

// Base64 encoder table
extern const char cb64[]=
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int CHARSTRING::XER_encode(const XERdescriptor_t& p_td,
                 TTCN_Buffer& p_buf, unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound character string value.");
  }
  int exer  = is_exer(flavor |= SIMPLE_TYPE);
  // SIMPLE_TYPE has no influence on is_exer, we set it for later
  int encoded_length=(int)p_buf.get_len();
  boolean do_empty_element = val_ptr==NULL || val_ptr->n_chars == 0;

  flavor &= ~XER_RECOF; // charstring doesn't care

  if (do_empty_element && exer && p_td.dfeValue != 0) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_REPR,
      "An encoded value with DEFAULT-FOR-EMPTY instruction applied should not be empty");
  }
  if (begin_xml(p_td, p_buf, flavor, indent, do_empty_element) == -1) {
    --encoded_length; // it was shortened by one
  }

  if (!do_empty_element) {
    const char * current   = val_ptr->chars_ptr;
    const char * const end = val_ptr->chars_ptr + val_ptr->n_chars;
    const char * to_escape;
    unsigned int mask;
    if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
      to_escape = "<&>'\"\x09\x0A\x0D";
      mask = 0x80000000; // guaranteed bigger than any Unicode character
    }
    else {
      to_escape = "<&>'\"";
      mask = 0;
    }

    // If Base64 is needed, use a temporary buffer.
    TTCN_Buffer tmpbuf, &rbuf = (exer && (p_td.xer_bits & BASE_64)) ? tmpbuf : p_buf;

    // This here is an optimization. Only &lt;&amp;&gt; need to be escaped.
    // Contiguous runs of "ordinary" characters are put in the buffer
    // with a single call.
    // TODO: is it really faster ? strpbrk is probably O(nm)
    while ( const char * trouble = strpbrk(current, to_escape) ) {
      rbuf.put_s((size_t)(trouble - current), (cbyte*)current);
      xml_escape(*trouble | mask, rbuf); // escape the troublesome character
      current = trouble+1;
    }

    // put the remainder in the buffer
    rbuf.put_s( (size_t)(end - current), (cbyte*)current);

    if (exer && (p_td.xer_bits & BASE_64)) {
      size_t clear_len = tmpbuf.get_len(); // the length before padding
      // Pad the temporary buffer so i+1 and i+2 don't go outside the buffer
      unsigned char zero[2] = {0,0};
      tmpbuf.put_s(2, zero);
      // Get the buffer after the possible realloc caused by the padding.
      cbyte * in = tmpbuf.get_data();

      // Encode 3 bytes of cleartext into 4 bytes of Base64
      for (size_t i = 0; i < clear_len; i += 3) {
        p_buf.put_c( cb64[ in[i] >> 2 ] );
        p_buf.put_c( cb64[ ((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4) ]);
        p_buf.put_c( i+1 < clear_len
          ? cb64[ ((in[i+1] & 0x0f) << 2) | ((in[i+2] & 0xc0) >> 6) ]
                  : '=' );
        p_buf.put_c( i+2 < clear_len ? cb64[ in[i+2] & 0x3f ] : '=' );
      } // next i
    } // if base64
  }

  end_xml(p_td, p_buf, flavor, indent, do_empty_element);
  return (int)p_buf.get_len() - encoded_length;
}

/**
* Translates a Base64 value to either its 6-bit reconstruction value
* or a negative number indicating some other meaning.
* Public domain from http://iharder.net/base64 */
char  base64_decoder_table[256] =
{
  -9,-9,-9,-9,-9,-9,-9,-9,-9,                 // Decimal  0 -  8
  -5,-5,                                      // Whitespace: Tab and Linefeed
  -9,-9,                                      // Decimal 11 - 12
  -5,                                         // Whitespace: Carriage Return
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 14 - 26
  -9,-9,-9,-9,-9,                             // Decimal 27 - 31
  -5,                                         // Whitespace: Space
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,              // Decimal 33 - 42
  62,                                         // Plus sign at decimal 43
  -9,-9,-9,                                   // Decimal 44 - 46
  63,                                         // Slash at decimal 47
  52,53,54,55,56,57,58,59,60,61,              // Numbers zero through nine
  -9,-9,-9,                                   // Decimal 58 - 60
  -1,                                         // Equals sign at decimal 61
  -9,-9,-9,                                   // Decimal 62 - 64
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,            // Letters 'A' through 'N'
  14,15,16,17,18,19,20,21,22,23,24,25,        // Letters 'O' through 'Z'
  -9,-9,-9,-9,-9,-9,                          // Decimal 91 - 96
  26,27,28,29,30,31,32,33,34,35,36,37,38,     // Letters 'a' through 'm'
  39,40,41,42,43,44,45,46,47,48,49,50,51,     // Letters 'n' through 'z'
  -9,-9,-9,-9,-9                              // Decimal 123 - 127
  ,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,       // Decimal 128 - 139
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 140 - 152
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 153 - 165
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 166 - 178
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 179 - 191
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 192 - 204
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 205 - 217
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 218 - 230
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 231 - 243
  -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9         // Decimal 244 - 255
};

unsigned int xlate(cbyte*in, int phase, unsigned char*dest) {
  static unsigned char nbytes[4] = { 3,1,1,2 };
  unsigned char out[4];
  out[0] = in[0] << 2 | in[1] >> 4;
  out[1] = in[1] << 4 | in[2] >> 2;
  out[2] = in[2] << 6 | in[3] >> 0;
  memcpy(dest, out, nbytes[phase]);
  return nbytes[phase];
}

int CHARSTRING::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
                           unsigned int flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*) {
  int exer = is_exer(flavor);

  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
    const char * name = verify_name(reader, p_td, exer);
    (void)name;
    const char * value = (const char *)reader.Value();
    // FIXME copy & paste
    *this = value;

    // Let the caller do reader.AdvanceAttribute();
  }
  else {
    boolean omit_tag = exer
      && (p_td.xer_bits & UNTAGGED || flavor & (EMBED_VALUES|XER_LIST|USE_TYPE_ATTR|USE_NIL));
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
        if (reader.IsEmptyElement()) { // has no text, needs special processing
          if (exer && p_td.dfeValue != 0) {
            *this = *static_cast<const CHARSTRING*>(p_td.dfeValue);
          }
          else init_struct(0);
          reader.Read();
          break; // exit the loop early
        } // if empty element
        // otherwise, not an empty element, stay in the loop
        depth = reader.Depth();
      }
      else if ((depth != -1 || omit_tag)
        && (XML_READER_TYPE_TEXT == type || XML_READER_TYPE_CDATA == type))
        // Process #text node if we already processed the element node, or
        // there is no element node because UNTAGGED is in effect.
      {
        const xmlChar * value = reader.Value();
        size_t num_chars = strlen((const char*)value);

        if (exer && (p_td.xer_bits & BASE_64)) {
          clean_up();
          init_struct(num_chars * 3 / 4);
          xmlChar in[4];

          int phase = 0;
          unsigned char * dest = (unsigned char *)val_ptr->chars_ptr;

          for (size_t o=0; o<num_chars; ++o) {
            xmlChar c = value[o];
            if(c == '=') { // Padding starts; decode last chunk and exit.
              dest += xlate(in,phase, dest);
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
          val_ptr->n_chars = (char*)dest - val_ptr->chars_ptr;
        }
        else {
          clean_up();
          init_struct(num_chars);
          memcpy(val_ptr->chars_ptr, value, num_chars);
        } // if base64

        if(!omit_tag) {
          // find the end element
          for (success = reader.Read(); success == 1; success = reader.Read()) {
            type = reader.NodeType();
            if (XML_READER_TYPE_END_ELEMENT == type) {
              verify_end(reader, p_td, depth, exer);
              reader.Read(); // one last time
              break;
            }
          }
        }
        break;
      }
      else if (XML_READER_TYPE_END_ELEMENT == type) {
        if (!omit_tag) {
          verify_end(reader, p_td, depth, exer);
          if (!val_ptr) {
            // We are at our end tag and no content
            if (exer && p_td.dfeValue != 0) {
              *this = *static_cast<const CHARSTRING*>(p_td.dfeValue);
            }
            else init_struct(0); // empty string
          }
          reader.Read();
        }
        break;
      } // if type
    } // next read
  } // if attribute

  if (exer && p_td.whitespace >= WHITESPACE_REPLACE) { // includes _COLLAPSE
    for (int i=0; i<val_ptr->n_chars; ++i) { // first, _REPLACE
      switch (val_ptr->chars_ptr[i]) {
      case 9:  // HORIZONTAL TAB
      case 10: // LINE FEED
      case 13: // CARRIAGE RETURN
        val_ptr->chars_ptr[i] = ' ';
        break;
      default:
        break;
      }
    } // next i

    if (p_td.whitespace >= WHITESPACE_COLLAPSE) {
      char *to;
      const char *from, *end = val_ptr->chars_ptr + val_ptr->n_chars;
      for (from = to = val_ptr->chars_ptr; from < end;) {
        *to = *(from++);
        // If the copied character (*to) was a space,
        // and the next character to be copied (*from) is also a space
        //     (continuous run of spaces)
        //   or this was the first character (leading spaces to be trimmed),
        // then don't advance the destination (will be overwritten).
        if (*to != ' '
          || (from < end && *from != ' ' && to > val_ptr->chars_ptr)) ++to;
      }
      *to = 0;
      val_ptr->n_chars = to - val_ptr->chars_ptr; // truncate
      // TODO maybe realloc after truncation
    } // collapse
  } // if WHITESPACE
  
  return 1;
}

int CHARSTRING::RAW_encode(const TTCN_Typedescriptor_t& p_td,
  RAW_enc_tree& myleaf) const
{
  int bl = val_ptr->n_chars * 8; // bit length
  int align_length = p_td.raw->fieldlength ? p_td.raw->fieldlength - bl : 0;
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
  }
  if ((bl + align_length) < val_ptr->n_chars * 8) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There is no sufficient bits to encode '%s': ", p_td.name);
    bl = p_td.raw->fieldlength;
    align_length = 0;
  }
  if (myleaf.must_free) Free(myleaf.body.leaf.data_ptr);
  myleaf.must_free = FALSE;
  myleaf.data_ptr_used = TRUE;
  myleaf.body.leaf.data_ptr = (unsigned char*) val_ptr->chars_ptr;
  if (p_td.raw->endianness == ORDER_MSB) myleaf.align = -align_length;
  else myleaf.align = align_length;
  return myleaf.length = bl + align_length;
}

int CHARSTRING::RAW_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff, int limit, raw_order_t top_bit_ord, boolean no_err,
  int /*sel_field*/, boolean /*first_call*/)
{
  int prepaddlength = buff.increase_pos_padd(p_td.raw->prepadding);
  limit -= prepaddlength;
  int decode_length = p_td.raw->fieldlength == 0
    ? (limit / 8) * 8 : p_td.raw->fieldlength;
  if ( decode_length > limit
    || decode_length > (int) buff.unread_len_bit()) {
    if (no_err) return -TTCN_EncDec::ET_LEN_ERR;
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
      "There is not enough bits in the buffer to decode type %s.", p_td.name);
    decode_length = ((limit > (int) buff.unread_len_bit() ? (int)buff.unread_len_bit() : limit) / 8) * 8;
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
  cp.hexorder = ORDER_LSB;
  clean_up();
  init_struct(decode_length / 8);
  buff.get_b((size_t) decode_length, (unsigned char*) val_ptr->chars_ptr, cp,
    top_bit_ord);

  if (p_td.raw->length_restrition != -1) {
    val_ptr->n_chars = p_td.raw->length_restrition;
    if (p_td.raw->endianness == ORDER_MSB) memmove(val_ptr->chars_ptr,
      val_ptr->chars_ptr + (decode_length / 8 - val_ptr->n_chars),
      val_ptr->n_chars); // sizeof(char) == 1 by definition
  }
  decode_length += buff.increase_pos_padd(p_td.raw->padding);
  return decode_length + prepaddlength;
}

char* CHARSTRING::to_JSON_string() const
{
  // Need at least 3 more characters (the double quotes around the string and the terminating zero)
  char* json_str = (char*)Malloc(val_ptr->n_chars + 3);
  
  json_str[0] = 0;
  json_str = mputc(json_str, '\"');
  
  for (int i = 0; i < val_ptr->n_chars; ++i) {
    // Increase the size of the buffer if it's not big enough to store the
    // characters remaining in the charstring plus 1 (for safety, in case this
    // character needs to be double-escaped).
    switch(val_ptr->chars_ptr[i]) {
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
      json_str = mputc(json_str, val_ptr->chars_ptr[i]);
      break;
    }
  }
  
  json_str = mputc(json_str, '\"');
  return json_str;
}

boolean CHARSTRING::from_JSON_string(const char* p_value, size_t p_value_len, boolean check_quotes)
{
  size_t start = 0;
  size_t end = p_value_len;
  if (check_quotes) {
    start = 1;
    end = p_value_len - 1;
    if (p_value[0] != '\"' || p_value[p_value_len - 1] != '\"') {
      return FALSE;
    }
  }
  
  // The resulting string (its length is less than or equal to end - start)
  char* str = (char*)Malloc(end - start);
  size_t len = 0;
  boolean error = FALSE;
  
  for (size_t i = start; i < end; ++i) {
    if (0 > p_value[i]) {
      error = TRUE;
      break;
    }
    if ('\\' == p_value[i]) {
      if (i == end - 1) {
        error = TRUE;
        break;
      }
      switch(p_value[i + 1]) {
      case 'n':
        str[len++] = '\n';
        break;
      case 't':
        str[len++] = '\t';
        break;
      case 'r':
        str[len++] = '\r';
        break;
      case 'f':
        str[len++] = '\f';
        break;
      case 'b':
        str[len++] = '\b';
        break;
      case '\\':
        str[len++] = '\\';
        break;
      case '\"':
        str[len++] = '\"';
        break;
      case '/':
        str[len++] = '/';
        break;
      case 'u': {
        if (end - i >= 6 && '0' == p_value[i + 2] && '0' == p_value[i + 3]) {
          unsigned char upper_nibble = char_to_hexdigit(p_value[i + 4]);
          unsigned char lower_nibble = char_to_hexdigit(p_value[i + 5]);
          if (0x07 >= upper_nibble && 0x0F >= lower_nibble) {
            str[len++] = (upper_nibble << 4) | lower_nibble;
            // skip 4 extra characters (the 4 hex digits)
            i += 4;
          } else {
            // error (found something other than hex digits) -> leave the for cycle
            i = end;
            error = TRUE;
          }
        } else {
          // error (not enough characters left or the first 2 hex digits are non-null) -> leave the for cycle
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
      str[len++] = p_value[i];
    } 
    
    if (check_quotes && i == p_value_len - 1) {
      // Special case: the last 2 characters are double escaped quotes ('\\' and '\"')
      error = TRUE;
    }
  }
  
  if (!error) {
    clean_up();
    init_struct(len);
    memcpy(val_ptr->chars_ptr, str, len);
    val_ptr->chars_ptr[len] = 0;
  }
  Free(str);
  return !error;
}

int CHARSTRING::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound charstring value.");
    return -1;
  }
  
  char* tmp_str = to_JSON_string();  
  int enc_len = p_tok.put_next_token(JSON_TOKEN_STRING, tmp_str);
  Free(tmp_str);
  return enc_len;
}

int CHARSTRING::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
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
    if (!from_JSON_string(value, value_len, !use_default)) {
      JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FORMAT_ERROR, "string", "charstring");
      clean_up();
      return JSON_ERROR_FATAL;
    }
  } else {
    return JSON_ERROR_INVALID_TOKEN;
  }
  return (int)dec_len;
}


CHARSTRING_ELEMENT::CHARSTRING_ELEMENT(boolean par_bound_flag,
  CHARSTRING& par_str_val, int par_char_pos)
  : bound_flag(par_bound_flag), str_val(par_str_val), char_pos(par_char_pos)
{

}

CHARSTRING_ELEMENT& CHARSTRING_ELEMENT::operator=(const char* other_value)
{
  if (other_value == NULL ||
      other_value[0] == '\0' || other_value[1] != '\0')
    TTCN_error("Assignment of a charstring value with length other "
               "than 1 to a charstring element.");
  bound_flag = TRUE;
  str_val.copy_value();
  str_val.val_ptr->chars_ptr[char_pos] = other_value[0];
  return *this;
}

CHARSTRING_ELEMENT& CHARSTRING_ELEMENT::operator=
(const CHARSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound charstring value to a "
                         "charstring element.");
  if(other_value.val_ptr->n_chars != 1)
    TTCN_error("Assignment of a charstring value with length other than "
               "1 to a charstring element.");
  bound_flag = TRUE;
  str_val.copy_value();
  str_val.val_ptr->chars_ptr[char_pos] = other_value.val_ptr->chars_ptr[0];
  return *this;
}

CHARSTRING_ELEMENT& CHARSTRING_ELEMENT::operator=
(const CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound charstring element.");
  if (&other_value != this) {
    bound_flag = TRUE;
    str_val.copy_value();
    str_val.val_ptr->chars_ptr[char_pos] =
      other_value.str_val.val_ptr->chars_ptr[other_value.char_pos];
  }
  return *this;
}

boolean CHARSTRING_ELEMENT::operator==(const char *other_value) const
{
  must_bound("Comparison of an unbound charstring element.");
  if (other_value == NULL || other_value[0] == '\0' ||
      other_value[1] != '\0') return FALSE;
  return str_val.val_ptr->chars_ptr[char_pos] == other_value[0];
}

boolean CHARSTRING_ELEMENT::operator==(const CHARSTRING& other_value) const
{
  must_bound("Comparison of an unbound charstring element.");
  other_value.must_bound("Comparison of an unbound charstring value.");
  if (other_value.val_ptr->n_chars != 1) return FALSE;
  return str_val.val_ptr->chars_ptr[char_pos] ==
    other_value.val_ptr->chars_ptr[0];
}

boolean CHARSTRING_ELEMENT::operator==(const CHARSTRING_ELEMENT& other_value) const
{
  must_bound("Comparison of an unbound charstring element.");
  other_value.must_bound("Comparison of an unbound charstring element.");
  return str_val.val_ptr->chars_ptr[char_pos] ==
    other_value.str_val.val_ptr->chars_ptr[other_value.char_pos];
}

boolean CHARSTRING_ELEMENT::operator==(const UNIVERSAL_CHARSTRING& other_value)
  const
{
  must_bound("The left operand of comparison is an unbound charstring "
    "element.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring value.");
  if (other_value.charstring) {
    if (other_value.cstr.val_ptr->n_chars != 1) return FALSE;
    return     str_val.val_ptr->chars_ptr[char_pos] ==
      other_value.cstr.val_ptr->chars_ptr[0];
  }
  else {
    if (other_value.val_ptr->n_uchars != 1) return FALSE;
    return other_value.val_ptr->uchars_ptr[0].uc_group == 0 &&
      other_value.val_ptr->uchars_ptr[0].uc_plane == 0 &&
      other_value.val_ptr->uchars_ptr[0].uc_row == 0 &&
      other_value.val_ptr->uchars_ptr[0].uc_cell ==
        (cbyte)str_val.val_ptr->chars_ptr[char_pos];
  }
}

boolean CHARSTRING_ELEMENT::operator==
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of comparison is an unbound charstring "
    "element.");
  other_value.must_bound("The right operand of comparison is an unbound "
    "universal charstring element.");
  const universal_char& uchar = other_value.get_uchar();
  return uchar.uc_group == 0 && uchar.uc_plane == 0 && uchar.uc_row == 0 &&
    uchar.uc_cell == (cbyte)str_val.val_ptr->chars_ptr[char_pos];
}

CHARSTRING CHARSTRING_ELEMENT::operator+(const char *other_value) const
{
  must_bound("Unbound operand of charstring element concatenation.");
  int other_len;
  if (other_value == NULL) other_len = 0;
  else other_len = strlen(other_value);
  CHARSTRING ret_val(other_len + 1);
  ret_val.val_ptr->chars_ptr[0] = str_val.val_ptr->chars_ptr[char_pos];
  memcpy(ret_val.val_ptr->chars_ptr + 1, other_value, other_len);
  return ret_val;
}

CHARSTRING CHARSTRING_ELEMENT::operator+(const CHARSTRING& other_value) const
{
  must_bound("Unbound operand of charstring element concatenation.");
  other_value.must_bound("Unbound operand of charstring concatenation.");
  int n_chars = other_value.val_ptr->n_chars;
  CHARSTRING ret_val(n_chars + 1);
  ret_val.val_ptr->chars_ptr[0] = str_val.val_ptr->chars_ptr[char_pos];
  memcpy(ret_val.val_ptr->chars_ptr + 1, other_value.val_ptr->chars_ptr,
         n_chars);
  return ret_val;
}

CHARSTRING CHARSTRING_ELEMENT::operator+(const CHARSTRING_ELEMENT&
                                         other_value) const
{
  must_bound("Unbound operand of charstring element concatenation.");
  other_value.must_bound("Unbound operand of charstring element "
                         "concatenation.");
  char result[2];
  result[0] = str_val.val_ptr->chars_ptr[char_pos];
  result[1] = other_value.str_val.val_ptr->chars_ptr[other_value.char_pos];
  return CHARSTRING(2, result);
}

#ifdef TITAN_RUNTIME_2
CHARSTRING CHARSTRING_ELEMENT::operator+(
  const OPTIONAL<CHARSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const CHARSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of charstring concatenation.");
}
#endif

UNIVERSAL_CHARSTRING CHARSTRING_ELEMENT::operator+
  (const UNIVERSAL_CHARSTRING& other_value) const
{
  must_bound("The left operand of concatenation is an unbound charstring "
    "element.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring value.");
  if (other_value.charstring) {
    UNIVERSAL_CHARSTRING ret_val(other_value.cstr.val_ptr->n_chars + 1, TRUE);
    ret_val.cstr.val_ptr->chars_ptr[0] = str_val.val_ptr->chars_ptr[char_pos];
    memcpy(ret_val.cstr.val_ptr->chars_ptr + 1, other_value.cstr.val_ptr->chars_ptr, other_value.cstr.val_ptr->n_chars);
    return ret_val;
  } else {
    UNIVERSAL_CHARSTRING ret_val(other_value.val_ptr->n_uchars + 1);
    ret_val.val_ptr->uchars_ptr[0].uc_group = 0;
    ret_val.val_ptr->uchars_ptr[0].uc_plane = 0;
    ret_val.val_ptr->uchars_ptr[0].uc_row = 0;
    ret_val.val_ptr->uchars_ptr[0].uc_cell = str_val.val_ptr->chars_ptr[char_pos];
    memcpy(ret_val.val_ptr->uchars_ptr + 1, other_value.val_ptr->uchars_ptr,
      other_value.val_ptr->n_uchars * sizeof(universal_char));
    return ret_val;
  }
}

UNIVERSAL_CHARSTRING CHARSTRING_ELEMENT::operator+
  (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const
{
  must_bound("The left operand of concatenation is an unbound charstring "
    "element.");
  other_value.must_bound("The right operand of concatenation is an unbound "
    "universal charstring element.");
  universal_char result[2];
  result[0].uc_group = 0;
  result[0].uc_plane = 0;
  result[0].uc_row = 0;
  result[0].uc_cell = str_val.val_ptr->chars_ptr[char_pos];
  result[1] = other_value.get_uchar();
  return UNIVERSAL_CHARSTRING(2, result);
}

#ifdef TITAN_RUNTIME_2
UNIVERSAL_CHARSTRING CHARSTRING_ELEMENT::operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const
{
  if (other_value.is_present()) {
    return *this + (const UNIVERSAL_CHARSTRING&)other_value;
  }
  TTCN_error("Unbound or omitted right operand of universal charstring "
    "concatenation.");
}
#endif

char CHARSTRING_ELEMENT::get_char() const
{
  return str_val.val_ptr->chars_ptr[char_pos];
}

void CHARSTRING_ELEMENT::log() const
{
  if (bound_flag) {
    char c = str_val.val_ptr->chars_ptr[char_pos];
    if (TTCN_Logger::is_printable(c)) {
      TTCN_Logger::log_char('"');
      TTCN_Logger::log_char_escaped(c);
      TTCN_Logger::log_char('"');
    } else TTCN_Logger::log_event("char(0, 0, 0, %u)", (unsigned char)c);
  } else TTCN_Logger::log_event_unbound();
}

// global functions

boolean operator==(const char* string_value, const CHARSTRING& other_value)
{
  other_value.must_bound("Unbound operand of charstring comparison.");
  if (string_value == NULL) string_value = "";
  return !strcmp(string_value, other_value.val_ptr->chars_ptr);
}

boolean operator==(const char* string_value,
                   const CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Unbound operand of charstring element "
                         "comparison.");
  if (string_value == NULL || string_value[0] == '\0' ||
      string_value[1] != '\0') return FALSE;
  return string_value[0] == other_value.get_char();
}

CHARSTRING operator+(const char* string_value, const CHARSTRING& other_value)
{
  other_value.must_bound("Unbound operand of charstring concatenation.");
  int string_len;
  if (string_value == NULL) string_len = 0;
  else string_len = strlen(string_value);
  if (string_len == 0) return other_value;
  CHARSTRING ret_val(string_len + other_value.val_ptr->n_chars);
  memcpy(ret_val.val_ptr->chars_ptr, string_value, string_len);
  memcpy(ret_val.val_ptr->chars_ptr + string_len,
         other_value.val_ptr->chars_ptr, other_value.val_ptr->n_chars);
  return ret_val;
}

CHARSTRING operator+(const char* string_value,
                     const CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Unbound operand of charstring element "
                         "concatenation.");
  int string_len;
  if (string_value == NULL) string_len = 0;
  else string_len = strlen(string_value);
  if (string_len == 0) return CHARSTRING(other_value);
  CHARSTRING ret_val(string_len + 1);
  memcpy(ret_val.val_ptr->chars_ptr, string_value, string_len);
  ret_val.val_ptr->chars_ptr[string_len] = other_value.get_char();
  return ret_val;
}

#ifdef TITAN_RUNTIME_2
CHARSTRING operator+(const OPTIONAL<CHARSTRING>& left_value,
  const CHARSTRING& right_value)
{
  if (left_value.is_present()) {
    return (const CHARSTRING&)left_value + right_value;
  }
  TTCN_error("Unbound or omitted left operand of charstring "
    "concatenation.");
}

CHARSTRING operator+(const OPTIONAL<CHARSTRING>& left_value,
  const CHARSTRING_ELEMENT& right_value)
{
  if (left_value.is_present()) {
    return (const CHARSTRING&)left_value + right_value;
  }
  TTCN_error("Unbound or omitted left operand of charstring "
    "concatenation.");
}

UNIVERSAL_CHARSTRING operator+(const OPTIONAL<CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING& right_value)
{
  if (left_value.is_present()) {
    return (const CHARSTRING&)left_value + right_value;
  }
  TTCN_error("Unbound or omitted left operand of universal charstring "
    "concatenation.");
}

UNIVERSAL_CHARSTRING operator+(const OPTIONAL<CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& right_value)
{
  if (left_value.is_present()) {
    return (const CHARSTRING&)left_value + right_value;
  }
  TTCN_error("Unbound or omitted left operand of universal charstring "
    "concatenation.");
}

UNIVERSAL_CHARSTRING operator+(const OPTIONAL<CHARSTRING>& left_value,
  const OPTIONAL<UNIVERSAL_CHARSTRING>& right_value)
{
  if (!left_value.is_present()) {
    TTCN_error("Unbound or omitted left operand of universal charstring "
    "concatenation.");
  }
  if (!right_value.is_present()) {
    TTCN_error("Unbound or omitted right operand of universal charstring "
    "concatenation.");
  }
  return (const CHARSTRING&)left_value + (const UNIVERSAL_CHARSTRING&)right_value;
}
#endif // TITAN_RUNTIME_2

CHARSTRING operator<<=(const char *string_value, const INTEGER& rotate_count)
{
  return CHARSTRING(string_value) <<= rotate_count;
}

CHARSTRING operator>>=(const char *string_value, const INTEGER& rotate_count)
{
  return CHARSTRING(string_value) >>= rotate_count;
}

// charstring template class

void CHARSTRING_template::clean_up()
{
  switch(template_selection) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  case STRING_PATTERN:
    if (pattern_value.regexp_init) regfree(&pattern_value.posix_regexp);
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

void CHARSTRING_template::copy_template(const CHARSTRING_template& other_value)
{
  switch (other_value.template_selection) {
  case STRING_PATTERN:
    pattern_value.regexp_init = FALSE;
    pattern_value.nocase = other_value.pattern_value.nocase;
    /* no break */
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
    value_list.list_value = new CHARSTRING_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
	other_value.value_list.list_value[i]);
    break;
  case VALUE_RANGE:
    if (!other_value.value_range.min_is_set) TTCN_error("The lower bound is "
      "not set when copying a charstring value range template.");
    if (!other_value.value_range.max_is_set) TTCN_error("The upper bound is "
      "not set when copying a charstring value range template.");
    value_range = other_value.value_range;
    break;
  case DECODE_MATCH:
    dec_match = other_value.dec_match;
    dec_match->ref_count++;
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported charstring template.");
  }
  set_selection(other_value);
}

CHARSTRING_template::CHARSTRING_template()
{
}

CHARSTRING_template::CHARSTRING_template(template_sel other_value)
  : Restricted_Length_Template(other_value)
{
  check_single_selection(other_value);
}

CHARSTRING_template::CHARSTRING_template(const CHARSTRING& other_value)
  : Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

CHARSTRING_template::CHARSTRING_template(const CHARSTRING_ELEMENT& other_value)
  : Restricted_Length_Template(SPECIFIC_VALUE), single_value(other_value)
{
}

CHARSTRING_template::CHARSTRING_template(const OPTIONAL<CHARSTRING>& other_value)
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
    TTCN_error("Creating a charstring template from an unbound optional "
      "field.");
  }
}

CHARSTRING_template::CHARSTRING_template(template_sel p_sel,
                                         const CHARSTRING& p_str,
                                         boolean p_nocase /* = FALSE */)
  : Restricted_Length_Template(STRING_PATTERN), single_value(p_str)
{
  if(p_sel!=STRING_PATTERN)
    TTCN_error("Internal error: Initializing a charstring pattern template "
               "with invalid selection.");
  pattern_value.regexp_init=FALSE;
  pattern_value.nocase = p_nocase;
}

CHARSTRING_template::CHARSTRING_template(const CHARSTRING_template& other_value)
: Restricted_Length_Template()
{
  copy_template(other_value);
}

CHARSTRING_template::~CHARSTRING_template()
{
  clean_up();
}

CHARSTRING_template& CHARSTRING_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

CHARSTRING_template& CHARSTRING_template::operator=
  (const CHARSTRING& other_value)
{
  other_value.must_bound("Assignment of an unbound charstring value to a "
                         "template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

CHARSTRING_template& CHARSTRING_template::operator=
  (const CHARSTRING_ELEMENT& other_value)
{
  other_value.must_bound("Assignment of an unbound charstring element to a "
                         "template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value = other_value;
  return *this;
}

CHARSTRING_template& CHARSTRING_template::operator=
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
    TTCN_error("Assignment of an unbound optional field to a charstring "
      "template.");
  }
  return *this;
}

CHARSTRING_template& CHARSTRING_template::operator=
  (const CHARSTRING_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

#ifdef TITAN_RUNTIME_2
CHARSTRING_template CHARSTRING_template::operator+(
  const CHARSTRING_template& other_value) const
{
  if (template_selection == SPECIFIC_VALUE &&
      other_value.template_selection == SPECIFIC_VALUE) {
    return single_value + other_value.single_value;
  }
  TTCN_error("Operand of charstring template concatenation is an "
    "uninitialized or unsupported template.");
}

CHARSTRING_template CHARSTRING_template::operator+(const CHARSTRING& other_value) const
{
  if (template_selection == SPECIFIC_VALUE) {
    return single_value + other_value;
  }
  TTCN_error("Operand of charstring template concatenation is an "
    "uninitialized or unsupported template.");
}

CHARSTRING_template CHARSTRING_template::operator+(
  const CHARSTRING_ELEMENT& other_value) const
{
  if (template_selection == SPECIFIC_VALUE) {
    return single_value + other_value;
  }
  TTCN_error("Operand of charstring template concatenation is an "
    "uninitialized or unsupported template.");
}

CHARSTRING_template CHARSTRING_template::operator+(
  const OPTIONAL<CHARSTRING>& other_value) const
{
  if (template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  if (!other_value.is_present()) {
    TTCN_error("Operand of charstring template concatenation is an "
      "unbound or omitted record/set field.");
  }
  return single_value + (const CHARSTRING&)other_value;
}

UNIVERSAL_CHARSTRING_template CHARSTRING_template::operator+(
  const UNIVERSAL_CHARSTRING& other_value) const
{
  if (template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return single_value + other_value;
}

UNIVERSAL_CHARSTRING_template CHARSTRING_template::operator+(
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const
{
  if (template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return single_value + other_value;
}

UNIVERSAL_CHARSTRING_template CHARSTRING_template::operator+(
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

CHARSTRING_template operator+(const CHARSTRING& left_value,
  const CHARSTRING_template& right_template)
{
  if (right_template.template_selection == SPECIFIC_VALUE) {
    return left_value + right_template.single_value;
  }
  TTCN_error("Operand of charstring template concatenation is an "
    "uninitialized or unsupported template.");
}

CHARSTRING_template operator+(const CHARSTRING_ELEMENT& left_value,
  const CHARSTRING_template& right_template)
{
  if (right_template.template_selection == SPECIFIC_VALUE) {
    return left_value + right_template.single_value;
  }
  TTCN_error("Operand of charstring template concatenation is an "
    "uninitialized or unsupported template.");
}

CHARSTRING_template operator+(const OPTIONAL<CHARSTRING>& left_value,
  const CHARSTRING_template& right_template)
{
  if (!left_value.is_present()) {
    TTCN_error("Operand of charstring template concatenation is an "
      "unbound or omitted record/set field.");
  }
  if (right_template.template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return (const CHARSTRING&)left_value + right_template.single_value;
}

UNIVERSAL_CHARSTRING_template operator+(const UNIVERSAL_CHARSTRING& left_value,
  const CHARSTRING_template& right_template)
{
  if (right_template.template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return left_value + right_template.single_value;
}

UNIVERSAL_CHARSTRING_template operator+(
  const UNIVERSAL_CHARSTRING_ELEMENT& left_value,
  const CHARSTRING_template& right_template)
{
  if (right_template.template_selection != SPECIFIC_VALUE) {
    TTCN_error("Operand of charstring template concatenation is an "
      "uninitialized or unsupported template.");
  }
  return left_value + right_template.single_value;
}

UNIVERSAL_CHARSTRING_template operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const CHARSTRING_template& right_template)
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
#endif // TITAN_RUNTIME_2

CHARSTRING_ELEMENT CHARSTRING_template::operator[](int index_value)
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Accessing a charstring element of a non-specific charstring "
               "template.");
  return single_value[index_value];
}

CHARSTRING_ELEMENT CHARSTRING_template::operator[](const INTEGER& index_value)
{
  index_value.must_bound("Indexing a charstring template with an unbound "
                         "integer value.");
  return (*this)[(int)index_value];
}

const CHARSTRING_ELEMENT CHARSTRING_template::operator[](int index_value) const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Accessing a charstring element of a non-specific charstring "
               "template.");
  return single_value[index_value];
}

const CHARSTRING_ELEMENT CHARSTRING_template::operator[](const INTEGER& index_value) const
{
  index_value.must_bound("Indexing a charstring template with an unbound "
                         "integer value.");
  return (*this)[(int)index_value];
}

boolean CHARSTRING_template::match(const CHARSTRING& other_value,
                                   boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  int value_length = other_value.lengthof();
  if (!match_length(value_length)) return FALSE;
  switch (template_selection) {
  case SPECIFIC_VALUE:
    return single_value == other_value;
  case STRING_PATTERN: {
    if (!pattern_value.regexp_init) {
      char *posix_str=TTCN_pattern_to_regexp(single_value);
      if(posix_str==NULL) {
        TTCN_error("Cannot convert pattern \"%s\" to POSIX-equivalent.",
                   (const char*)single_value);
      }
      /*
      //TTCN_Logger::begin_event(TTCN_DEBUG);
      TTCN_Logger::log_event_str("POSIX ERE equivalent of pattern ");
      single_value.log();
      TTCN_Logger::log_event(" is: \"%s\"", posix_str);
      //TTCN_Logger::end_event();
      */
      int flags = REG_EXTENDED|REG_NOSUB;
      if (pattern_value.nocase) {
        flags |= REG_ICASE;
      }
      int ret_val=regcomp(&pattern_value.posix_regexp, posix_str, flags);
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
    int ret_val=regexec(&pattern_value.posix_regexp, other_value, 0, NULL, 0);
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
      "matching with a charstring value range template.");
    if (!value_range.max_is_set) TTCN_error("The upper bound is not set when "
      "matching with a charstring value range template.");
    if (value_range.min_value > value_range.max_value)
      TTCN_error("The lower bound (\"%c\") is greater than the upper bound "
	"(\"%c\") when matching with a charstring value range template.",
	value_range.min_value, value_range.max_value);
    const char *chars_ptr = other_value;
    int min_value_offset = 0;
    int max_value_offset = 0;
    if (value_range.min_is_exclusive) min_value_offset = 1;
    if (value_range.max_is_exclusive) max_value_offset = 1;
    for (int i = 0; i < value_length; i++) {
      if (chars_ptr[i] < (value_range.min_value + min_value_offset) ||
        chars_ptr[i] > (value_range.max_value - max_value_offset)) return FALSE;
    }
    return TRUE;
    break; }
  case DECODE_MATCH: {
    TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);
    TTCN_EncDec::clear_error();
    TTCN_Buffer buff(other_value);
    boolean ret_val = dec_match->instance->match(buff);
    TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL,TTCN_EncDec::EB_DEFAULT);
    TTCN_EncDec::clear_error();
    return ret_val; }
  default:
    TTCN_error("Matching with an uninitialized/unsupported charstring "
               "template.");
  }
  return FALSE;
}

const CHARSTRING& CHARSTRING_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific "
               "charstring template.");
  return single_value;
}

int CHARSTRING_template::lengthof() const
{
  int min_length;
  boolean has_any_or_none;
  if (is_ifpresent)
    TTCN_error("Performing lengthof() operation on a charstring template "
               "which has an ifpresent attribute.");
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
    min_length = single_value.lengthof();
    has_any_or_none = FALSE;
    break;
  case OMIT_VALUE:
    TTCN_error("Performing lengthof() operation on a charstring template "
               "containing omit value.");
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
                 "Performing lengthof() operation on a charstring template "
                 "containing an empty list.");
    int item_length = value_list.list_value[0].lengthof();
    for (unsigned int i = 1; i < value_list.n_values; i++) {
      if (value_list.list_value[i].lengthof()!=item_length)
        TTCN_error("Performing lengthof() operation on a charstring template "
                   "containing a value list with different lengths.");
    }
    min_length = item_length;
    has_any_or_none = FALSE;
    break;
  }
  case COMPLEMENTED_LIST:
    TTCN_error("Performing lengthof() operation on a charstring template "
               "containing complemented list.");
  case STRING_PATTERN:
    TTCN_error("Performing lengthof() operation on a charstring template "
               "containing a pattern is not allowed.");
  default:
    TTCN_error("Performing lengthof() operation on an "
               "uninitialized/unsupported charstring template.");
  }
  return check_section_is_single(min_length, has_any_or_none,
                                 "length", "a", "charstring template");
}

void CHARSTRING_template::set_type(template_sel template_type,
   unsigned int list_length)
{
  clean_up();
  switch (template_type) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    set_selection(template_type);
    value_list.n_values = list_length;
    value_list.list_value = new CHARSTRING_template[list_length];
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
    TTCN_error("Setting an invalid type for a charstring template.");
  }
}

CHARSTRING_template& CHARSTRING_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
      template_selection != COMPLEMENTED_LIST)
    TTCN_error("Internal error: Accessing a list element of a non-list "
               "charstring template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Internal error: Index overflow in a charstring value list "
               "template.");
  return value_list.list_value[list_index];
}

void CHARSTRING_template::set_min(const CHARSTRING& min_value)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Setting the lower bound for a non-range charstring template.");
  min_value.must_bound("Setting an unbound value as lower bound in a "
    "charstring value range template.");
  int length = min_value.lengthof();
  if (length != 1) TTCN_error("The length of the lower bound in a "
    "charstring value range template must be 1 instead of %d.", length);
  value_range.min_is_set = TRUE;
  value_range.min_is_exclusive = FALSE;
  value_range.min_value = *(const char*)min_value;
  if (value_range.max_is_set && value_range.min_value > value_range.max_value)
    TTCN_error("The lower bound (\"%c\") in a charstring value range template "
      "is greater than the upper bound (\"%c\").", value_range.min_value,
      value_range.max_value);
}

void CHARSTRING_template::set_max(const CHARSTRING& max_value)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Setting the upper bound for a non-range charstring template.");
  max_value.must_bound("Setting an unbound value as upper bound in a "
    "charstring value range template.");
  int length = max_value.lengthof();
  if (length != 1) TTCN_error("The length of the upper bound in a "
    "charstring value range template must be 1 instead of %d.", length);
  value_range.max_is_set = TRUE;
  value_range.max_is_exclusive = FALSE;
  value_range.max_value = *(const char*)max_value;
  if (value_range.min_is_set && value_range.min_value > value_range.max_value)
    TTCN_error("The upper bound (\"%c\") in a charstring value range template "
      "is smaller than the lower bound (\"%c\").", value_range.max_value,
      value_range.min_value);
}

void CHARSTRING_template::set_min_exclusive(boolean min_exclusive)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Setting the lower bound for a non-range charstring template.");
  value_range.min_is_exclusive = min_exclusive;
}

void CHARSTRING_template::set_max_exclusive(boolean max_exclusive)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Setting the upper bound for a non-range charstring template.");
  value_range.max_is_exclusive = max_exclusive;
}

void CHARSTRING_template::set_decmatch(Dec_Match_Interface* new_instance)
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Setting the decoded content matching mechanism of a non-decmatch "
      "charstring template.");
  }
  dec_match = new unichar_decmatch_struct;
  dec_match->ref_count = 1;
  dec_match->instance = new_instance;
  // the character coding is only used if this template is copied to a
  // universal charstring template
  dec_match->coding = CharCoding::UTF_8;
}

void* CHARSTRING_template::get_decmatch_dec_res() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the decoding result of a non-decmatch charstring "
      "template.");
  }
  return dec_match->instance->get_dec_res();
}

const TTCN_Typedescriptor_t* CHARSTRING_template::get_decmatch_type_descr() const
{
  if (template_selection != DECODE_MATCH) {
    TTCN_error("Retrieving the decoded type's descriptor in a non-decmatch "
      "charstring template.");
  }
  return dec_match->instance->get_type_descr();
}

void CHARSTRING_template::log_pattern(int n_chars, const char *chars_ptr,
                                      boolean nocase)
{
  TTCN_Logger::log_event_str("pattern ");
  if (nocase) {
    TTCN_Logger::log_event_str("@nocase ");
  }
  TTCN_Logger::log_event_str("\"");
  enum { INITIAL, BACKSLASH, BACKSLASH_Q, QUADRUPLE, HASHMARK, REPETITIONS }
    state = INITIAL;
  for (int i = 0; i < n_chars; i++) {
    unsigned char c = chars_ptr[i];
    // print the character
    if (isprint(c)) {
      switch (c) {
      case '"':
	TTCN_Logger::log_event_str("\\\"");
	break;
      case '{':
	if (state == BACKSLASH || state == BACKSLASH_Q)
	  TTCN_Logger::log_char('{');
	else TTCN_Logger::log_event_str("\\{");
	break;
      case '}':
	if (state == BACKSLASH || state == QUADRUPLE)
	  TTCN_Logger::log_char('}');
	else TTCN_Logger::log_event_str("\\}");
	break;
      case ' ':
	if (state != INITIAL && state != BACKSLASH) break;
	// no break
      default:
	TTCN_Logger::log_char(c);
	break;
      }
    } else {
      switch (c) {
      case '\t':
	if (state == INITIAL || state == BACKSLASH)
	  TTCN_Logger::log_event_str("\\t");
	break;
      case '\r':
	if (state == INITIAL || state == BACKSLASH)
	  TTCN_Logger::log_event_str("\\r");
	break;
      case '\n':
      case '\v':
      case '\f':
	if (state != INITIAL && state != BACKSLASH) break;
	// no break
      default:
	TTCN_Logger::log_event("\\q{0,0,0,%u}", c);
	break;
      }
    }
    // update the state
    switch (state) {
    case INITIAL:
      switch (c) {
      case '\\':
	state = BACKSLASH;
	break;
      case '#':
	state = HASHMARK;
	break;
      default:
        break;
      }
      break;
    case BACKSLASH:
      if (c == 'q') state = BACKSLASH_Q;
      else state = INITIAL;
      break;
    case BACKSLASH_Q:
      switch (c) {
      case '{':
	state = QUADRUPLE;
	break;
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      case '\v':
      case '\f':
	break;
      default:
	state = INITIAL;
	break;
      }
      break;
    case HASHMARK:
      switch (c) {
      case '(':
	state = REPETITIONS;
	break;
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      case '\v':
      case '\f':
	break;
      default:
	state = INITIAL;
	break;
      }
      break;
    case QUADRUPLE:
    case REPETITIONS:
      switch (c) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      case '\v':
      case '\f':
      case ',':
	break;
      default:
	if (!isdigit(c)) state = INITIAL;
	break;
      }
      break;
    }
  }
  TTCN_Logger::log_char('"');
}

void CHARSTRING_template::log() const
{
  switch (template_selection) {
  case STRING_PATTERN:
    log_pattern(single_value.lengthof(), (const char*)single_value,
      pattern_value.nocase);
    break;
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
  case VALUE_RANGE:
    TTCN_Logger::log_char('(');
    if (value_range.min_is_exclusive) TTCN_Logger::log_char('!');
    if (value_range.min_is_set) {
      if (TTCN_Logger::is_printable(value_range.min_value)) {
	TTCN_Logger::log_char('"');
	TTCN_Logger::log_char_escaped(value_range.min_value);
	TTCN_Logger::log_char('"');
      } else TTCN_Logger::log_event("char(0, 0, 0, %u)",
	(unsigned char)value_range.min_value);
    } else TTCN_Logger::log_event_str("<unknown lower bound>");
    TTCN_Logger::log_event_str(" .. ");
    if (value_range.max_is_exclusive) TTCN_Logger::log_char('!');
    if (value_range.max_is_set) {
      if (TTCN_Logger::is_printable(value_range.max_value)) {
	TTCN_Logger::log_char('"');
	TTCN_Logger::log_char_escaped(value_range.max_value);
	TTCN_Logger::log_char('"');
      } else TTCN_Logger::log_event("char(0, 0, 0, %u)",
	(unsigned char)value_range.max_value);
    } else TTCN_Logger::log_event_str("<unknown upper bound>");
    TTCN_Logger::log_char(')');
    break;
  case DECODE_MATCH:
    TTCN_Logger::log_event_str("decmatch ");
    dec_match->instance->log();
    break;
  default:
    log_generic();
  }
  log_restricted();
  log_ifpresent();
}

void CHARSTRING_template::log_match(const CHARSTRING& match_value,
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

void CHARSTRING_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE|Module_Param::BC_LIST, "charstring template");
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
    CHARSTRING_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Charstring:
    *this = CHARSTRING(mp->get_string_size(), (char*)mp->get_string_data());
    break;
  case Module_Param::MP_StringRange: {
    universal_char lower_uchar = mp->get_lower_uchar();
    universal_char upper_uchar = mp->get_upper_uchar();
    if (!lower_uchar.is_char()) param.error("Lower bound of char range cannot be a multiple-byte character");
    if (!upper_uchar.is_char()) param.error("Upper bound of char range cannot be a multiple-byte character");
    clean_up();
    set_selection(VALUE_RANGE);
    value_range.min_is_set = TRUE;
    value_range.max_is_set = TRUE;
    value_range.min_value = static_cast<char>(lower_uchar.uc_cell);
    value_range.max_value = static_cast<char>(upper_uchar.uc_cell);
    set_min_exclusive(mp->get_is_min_exclusive());
    set_max_exclusive(mp->get_is_max_exclusive());
  } break;
  case Module_Param::MP_Pattern:
    clean_up();
    single_value = CHARSTRING(mp->get_pattern());
    pattern_value.regexp_init = FALSE;
    pattern_value.nocase = mp->get_nocase();
    set_selection(STRING_PATTERN);
    break;
  case Module_Param::MP_Expression:
    if (mp->get_expr_type() == Module_Param::EXPR_CONCATENATE) {
      // only allow string patterns for the first operand
      CHARSTRING operand1, operand2, result;
      boolean nocase;
      boolean is_pattern = operand1.set_param_internal(*mp->get_operand1(),
        TRUE, &nocase);
      operand2.set_param(*mp->get_operand2());
      result = operand1 + operand2;
      if (is_pattern) {
        clean_up();
        single_value = result;
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
    param.type_error("charstring template");
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
Module_Param* CHARSTRING_template::get_param(Module_Param_Name& param_name) const
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
  case VALUE_RANGE: {
    universal_char lower_bound = { 0, 0, 0, (unsigned char)value_range.min_value };
    universal_char upper_bound = { 0, 0, 0, (unsigned char)value_range.max_value };
    mp = new Module_Param_StringRange(lower_bound, upper_bound, value_range.min_is_exclusive, value_range.max_is_exclusive);
    break; }
  case STRING_PATTERN:
    mp = new Module_Param_Pattern(mcopystr(single_value), pattern_value.nocase);
    break;
  case DECODE_MATCH:
    TTCN_error("Referencing a decoded content matching template is not supported.");
    break;
  default:
    TTCN_error("Referencing an uninitialized/unsupported charstring template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  mp->set_length_restriction(get_length_range());
  return mp;
}
#endif

void CHARSTRING_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_restricted(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case STRING_PATTERN:
    text_buf.push_int(pattern_value.nocase);
    // no break;
  case SPECIFIC_VALUE:
    single_value.encode_text(text_buf);
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].encode_text(text_buf);
    break;
  case VALUE_RANGE:
    if (!value_range.min_is_set) TTCN_error("Text encoder: The lower bound is "
      "not set in a charstring value range template.");
    if (!value_range.max_is_set) TTCN_error("Text encoder: The upper bound is "
      "not set in a charstring value range template.");
    text_buf.push_raw(1, &value_range.min_value);
    text_buf.push_raw(1, &value_range.max_value);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported "
               "charstring template.");
  }
}

void CHARSTRING_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_restricted(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case STRING_PATTERN:
    pattern_value.regexp_init=FALSE;
    pattern_value.nocase = text_buf.pull_int().get_val();
    /* no break */
  case SPECIFIC_VALUE:
    single_value.decode_text(text_buf);
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new CHARSTRING_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  case VALUE_RANGE:
    text_buf.pull_raw(1, &value_range.min_value);
    text_buf.pull_raw(1, &value_range.max_value);
    if (value_range.min_value > value_range.max_value)
      TTCN_error("Text decoder: The received lower bound is greater than the "
      "upper bound in a charstring value range template.");
    value_range.min_is_set = TRUE;
    value_range.max_is_set = TRUE;
    value_range.min_is_exclusive = FALSE;
    value_range.max_is_exclusive = FALSE;
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was "
               "received for a charstring template.");
  }
}

boolean CHARSTRING_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean CHARSTRING_template::match_omit(boolean legacy /* = FALSE */) const
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
void CHARSTRING_template::check_restriction(template_res t_res, const char* t_name,
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
             get_res_name(t_res), t_name ? t_name : "charstring");
}
#endif

const CHARSTRING& CHARSTRING_template::get_single_value() const {
  switch (template_selection) {
    case STRING_PATTERN:
    case SPECIFIC_VALUE:
      break;
    default:
      TTCN_error("This template does not have single value.");
  }
  return single_value;
}
