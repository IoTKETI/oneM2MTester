/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kremer, Peter
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include "../common/memory.h"
#include "error.h"

#include "string.hh"
#include "ustring.hh"

#include "Int.hh"

/** Parameters for tuning memory usage of class stringpool */
#define STRINGPOOL_INITIAL_SIZE 8
#define STRINGPOOL_INCREMENT_FACTOR 2

/** The amount of memory needed for a string containing n characters. */
#define MEMORY_SIZE(n) (sizeof(string_struct) - sizeof(size_t) + 1 + (n))

void string::init_struct(size_t n_chars)
{
  if (n_chars == 0) {
    /** This will represent the empty strings so they won't need allocated
     * memory, this delays the memory allocation until it is really needed. */
    static string_struct empty_string = { 1, 0, "" };
    val_ptr = &empty_string;
    empty_string.ref_count++;
  } else {
    val_ptr = static_cast<string_struct*>(Malloc(MEMORY_SIZE(n_chars)));
    val_ptr->ref_count = 1;
    val_ptr->n_chars = n_chars;
    val_ptr->chars_ptr[n_chars] = '\0';
  }
}

void string::copy_value_and_append(const char *s, size_t n)
{
  if (n > max_string_len - val_ptr->n_chars)
    FATAL_ERROR("string::copy_value_and_append(const char*, size_t): " \
      "length overflow");
  if (val_ptr->ref_count == 1) {
    ptrdiff_t offset = s - val_ptr->chars_ptr;
    val_ptr = static_cast<string_struct*>
      (Realloc(val_ptr, MEMORY_SIZE(val_ptr->n_chars + n)));
    // check whether the source buffer s is (part of) our existing buffer
    if (offset >= 0 && static_cast<size_t>(offset) < val_ptr->n_chars)
      memcpy(val_ptr->chars_ptr + val_ptr->n_chars,
	     val_ptr->chars_ptr + offset, n);
    else memcpy(val_ptr->chars_ptr + val_ptr->n_chars, s, n);
    val_ptr->n_chars += n;
    val_ptr->chars_ptr[val_ptr->n_chars] = '\0';
  } else {
    string_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_chars + n);
    memcpy(val_ptr->chars_ptr, old_ptr->chars_ptr, old_ptr->n_chars);
    memcpy(val_ptr->chars_ptr + old_ptr->n_chars, s, n);
  }
}

/** Internal worker for various replace() methods.
 *
 * @param pos   the index of the first character to replace
 * @param n     the number of characters to replace
 * @param s     the replacement
 * @param s_len the length of the replacement
 *
 * @pre \p pos        must point inside the string
 * @pre \p pos + \p n must point inside the string
 * @pre the resulting string must not exceed max_string_len
 */
void string::replace(size_t pos, size_t n, const char *s, size_t s_len)
{
  if (pos > val_ptr->n_chars)
    FATAL_ERROR("string::replace(): start position is outside the string");
  if (pos + n > val_ptr->n_chars)
    FATAL_ERROR("string::replace(): end position is outside the string");
  if (s_len > max_string_len - val_ptr->n_chars + n)
    FATAL_ERROR("string::replace(): length overflow");
  // do nothing if we are replacing with the same string
  if (n == s_len && memcmp(val_ptr->chars_ptr + pos, s, n) == 0) return;
  size_t new_size = val_ptr->n_chars - n + s_len;
  if (new_size == 0) {
    // the result is an empty string
    clean_up(val_ptr);
    init_struct(0);
  } else if (val_ptr->ref_count == 1 && (s < val_ptr->chars_ptr ||
      s >= val_ptr->chars_ptr + val_ptr->n_chars)) {
    // check whether this string is not referenced by others and we are not
    // replacing with ourselves
    if (val_ptr->n_chars < new_size) val_ptr = static_cast<string_struct*>
	(Realloc(val_ptr, MEMORY_SIZE(new_size)));
    memmove(val_ptr->chars_ptr + pos + s_len,
            val_ptr->chars_ptr + pos + n, val_ptr->n_chars - pos - n);
    memcpy(val_ptr->chars_ptr + pos, s, s_len);
    if (val_ptr->n_chars > new_size) val_ptr = static_cast<string_struct*>
	(Realloc(val_ptr, MEMORY_SIZE(new_size)));
    val_ptr->n_chars = new_size;
    val_ptr->chars_ptr[new_size] = '\0';
  } else {
    // the result must be copied into a new memory area
    string_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(new_size);
    memcpy(val_ptr->chars_ptr, old_ptr->chars_ptr, pos);
    memcpy(val_ptr->chars_ptr + pos, s, s_len);
    memcpy(val_ptr->chars_ptr + pos + s_len, old_ptr->chars_ptr + pos + n,
      old_ptr->n_chars - pos - n);
    if (old_ptr->ref_count == 0) Free(old_ptr);
  }
}

void string::clean_up(string_struct *ptr)
{
  if (ptr->ref_count > 1) ptr->ref_count--;
  else if (ptr->ref_count == 1) Free(ptr);
  else FATAL_ERROR("string::clean_up()");
}

int string::compare(const string& s) const
{
  if (val_ptr == s.val_ptr) return 0;
  else return strcmp(val_ptr->chars_ptr, s.val_ptr->chars_ptr);
}

int string::compare(const char *s) const
{
  if (val_ptr->chars_ptr == s) return 0;
  else return strcmp(val_ptr->chars_ptr, s);
}

string::string(const char *s)
{
  if (s == NULL) FATAL_ERROR("string::string(const char*): called with NULL");
  size_t n_chars = strlen(s);
  init_struct(n_chars);
  memcpy(val_ptr->chars_ptr, s, n_chars);
}

string::string(size_t n, const char *s)
{
  if (s == NULL && n > 0)
    FATAL_ERROR("string::string(size_t, const char*): called with NULL");
  init_struct(n);
  memcpy(val_ptr->chars_ptr, s, n);
}

string::string(const ustring& s)
{
  size_t s_len = s.size();
  init_struct(s_len);
  const ustring::universal_char *src = s.u_str();
  for (size_t i = 0; i < s_len; i++) {
    if (src[i].group != 0 || src[i].plane != 0 || src[i].row != 0)
      FATAL_ERROR("string::string(const ustring&)");
    val_ptr->chars_ptr[i] = src[i].cell;
  }
}

bool string::is_cstr() const
{
  for (size_t i = 0; i < val_ptr->n_chars; i++)
    if ((unsigned char)val_ptr->chars_ptr[i] > 127) return false;
  return true;
}

void string::clear()
{
  if (val_ptr->n_chars > 0) {
    clean_up(val_ptr);
    init_struct(0);
  }
}

string string::substr(size_t pos, size_t n) const
{
  if (pos > val_ptr->n_chars)
    FATAL_ERROR("string::substr(size_t, size_t): position is outside the " \
      "string");
  size_t n_chars = val_ptr->n_chars - pos;
  if (n_chars > n) n_chars = n;
  if (n_chars == val_ptr->n_chars) return *this;
  else return string(n_chars, val_ptr->chars_ptr + pos);
}

void string::resize(size_t n, char c)
{
  size_t old_length = val_ptr->n_chars;
  if (old_length == n) return;
  if (val_ptr->ref_count == 1) {
    if (n > 0) {
      val_ptr = static_cast<string_struct*>(Realloc(val_ptr, MEMORY_SIZE(n)));
      if (n > old_length) {
        memset(val_ptr->chars_ptr + old_length, c, n - old_length);
      }
      val_ptr->chars_ptr[n] = '\0';
      val_ptr->n_chars = n;
    } else {
      clean_up(val_ptr);
      init_struct(0);
    }
  } else {
    val_ptr->ref_count--;
    string_struct *tmp_ptr = val_ptr;
    init_struct(n);
    if (n > old_length) {
      memcpy(val_ptr->chars_ptr, tmp_ptr->chars_ptr, old_length);
      memset(val_ptr->chars_ptr + old_length, c, n - old_length);
    } else {
      memcpy(val_ptr->chars_ptr, tmp_ptr->chars_ptr, n);
    }
  }
}

void string::replace(size_t pos, size_t n, const char *s)
{
  if (s == NULL) FATAL_ERROR("string::replace(size_t, size_t, const char*): " \
    "called with NULL");
  replace(pos, n, s, strlen(s));
}

void string::replace(size_t pos, size_t n, const string& s)
{
  if (pos == 0 && n == val_ptr->n_chars) *this = s;
  else replace(pos, n, s.val_ptr->chars_ptr, s.val_ptr->n_chars);
}

size_t string::find(char c, size_t pos) const
{
  if (pos > val_ptr->n_chars)
    FATAL_ERROR("string::find(char, size_t): position is outside of string");
  for (size_t r = pos; r < val_ptr->n_chars; r++)
    if (c == val_ptr->chars_ptr[r]) return r;
  return val_ptr->n_chars;
}

size_t string::find(const char *s, size_t pos) const
{
  if (s == NULL)
    FATAL_ERROR("string::find(const char *, size_t): called with NULL");
  if (pos >= val_ptr->n_chars)
    FATAL_ERROR("string::find(const char *, size_t): position outside of string");
  const char *ptr = strstr(val_ptr->chars_ptr + pos, s);
  if (ptr != NULL) return ptr - val_ptr->chars_ptr;
  else return val_ptr->n_chars;
}

size_t string::rfind(char c, size_t pos) const
{
  if (pos > val_ptr->n_chars) pos = val_ptr->n_chars;
  for (size_t i = pos; i > 0; i--)
    if (c == val_ptr->chars_ptr[i - 1]) return i - 1;
  return val_ptr->n_chars;
}

size_t string::find_if(size_t first, size_t last, int (*pred)(int)) const
{
  if (first > last) FATAL_ERROR("string::find_if(): first greater than last");
  else if (last > val_ptr->n_chars)
    FATAL_ERROR("string::find_if(): last greater than string length");
  for (size_t r = first; r < last; r++)
    if (pred(val_ptr->chars_ptr[r])) return r;
  return last;
}

bool string::is_whitespace(unsigned char c)
{
  switch (c) {
  case ' ':
  case '\t':
  case '\n':
  case '\r':
  case '\v':
  case '\f':
    return true;
  default:
    return false;
  }
}

bool string::is_printable(unsigned char c)
{
  if (isprint(c)) return true;
  else {
    switch (c) {
    case '\a':
    case '\b':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
      return true;
    default:
      return false;
    }
  }
}

void string::append_stringRepr(char c)
{
  switch (c) {
  case '\a':
    *this += "\\a";
    break;
  case '\b':
    *this += "\\b";
    break;
  case '\t':
    *this += "\\t";
    break;
  case '\n':
    *this += "\\n";
    break;
  case '\v':
    *this += "\\v";
    break;
  case '\f':
    *this += "\\f";
    break;
  case '\r':
    *this += "\\r";
    break;
  case '\\':
    *this += "\\\\";
    break;
  case '"':
    *this += "\\\"";
    break;
  default:
    if (is_printable(c)) *this += c;
    else FATAL_ERROR("string::append_stringRepr()");
    break;
  }
}

string string::get_stringRepr() const
{
  string ret_val;
  enum { INIT, PCHAR, NPCHAR } state = INIT;
  for (size_t i = 0; i < val_ptr->n_chars; i++) {
    char c = val_ptr->chars_ptr[i];
    if (is_printable(c)) {
      // the actual character is printable
      switch (state) {
      case NPCHAR: // concatenation sign if previous part was not printable
	ret_val += " & ";
	// no break
      case INIT: // opening "
	ret_val += '"';
	// no break
      case PCHAR: // the character itself
        ret_val.append_stringRepr(c);
        break;
      }
      state = PCHAR;
    } else {
      // the actual character is not printable
      switch (state) {
      case PCHAR: // closing " if previous part was printable
	ret_val += '"';
	// no break
      case NPCHAR: // concatenation sign
	ret_val += " & ";
	// no break
      case INIT: // the character itself in quadruple notation
	ret_val += "char(0, 0, 0, ";
	ret_val += Common::Int2string((unsigned char)c);
	ret_val += ')';
	break;
      }
      state = NPCHAR;
    }
  }
  // final steps
  switch (state) {
  case INIT: // the string was empty
    ret_val += "\"\"";
    break;
  case PCHAR: // last character was printable -> closing "
    ret_val += '"';
    break;
  default:
    break;
  }
  return ret_val;
}

string& string::operator=(const string& s)
{
  if (&s != this) {
    clean_up(val_ptr);
    val_ptr = s.val_ptr;
    val_ptr->ref_count++;
  }
  return *this;
}

string& string::operator=(const char *s)
{
  if (s == NULL)
    FATAL_ERROR("string::operator=(const char*): called with NULL");
  if (val_ptr->chars_ptr != s) {
      clean_up(val_ptr);
      size_t n_chars = strlen(s);
      init_struct(n_chars);
      memcpy(val_ptr->chars_ptr, s, n_chars);
  }
  return *this;
}

char& string::operator[](size_t n)
{
  if (n >= val_ptr->n_chars)
    FATAL_ERROR("string::operator[](size_t): position is outside the string");
  if (val_ptr->ref_count > 1) {
    string_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_chars);
    memcpy(val_ptr->chars_ptr, old_ptr->chars_ptr, old_ptr->n_chars);
  }
  return val_ptr->chars_ptr[n];
}

char string::operator[](size_t n) const
{
  if (n >= val_ptr->n_chars)
    FATAL_ERROR("string::operator[](size_t) const: position is outside the string");
  return val_ptr->chars_ptr[n];
}

string string::operator+(const string& s) const
{
  if (s.val_ptr->n_chars > max_string_len - val_ptr->n_chars)
    FATAL_ERROR("string::operator+(const string&): length overflow");
  if (val_ptr->n_chars == 0) return s;
  else if (s.val_ptr->n_chars == 0) return *this;
  else {
    string ret_val(val_ptr->n_chars + s.val_ptr->n_chars);
    memcpy(ret_val.val_ptr->chars_ptr, val_ptr->chars_ptr, val_ptr->n_chars);
    memcpy(ret_val.val_ptr->chars_ptr + val_ptr->n_chars,
           s.val_ptr->chars_ptr, s.val_ptr->n_chars);
    return ret_val;
  }
}

string string::operator+(const char *s) const
{
  if (s == NULL)
    FATAL_ERROR("string::operator+(const char*): called with NULL)");
  size_t s_len = strlen(s);
  if (s_len > max_string_len - val_ptr->n_chars)
    FATAL_ERROR("string::operator+(const char*): length overflow");
  if (s_len == 0) return *this;
  else {
    string ret_val(val_ptr->n_chars + s_len);
    memcpy(ret_val.val_ptr->chars_ptr, val_ptr->chars_ptr, val_ptr->n_chars);
    memcpy(ret_val.val_ptr->chars_ptr + val_ptr->n_chars, s, s_len);
    return ret_val;
  }
}

string& string::operator+=(const string& s)
{
  if (s.val_ptr->n_chars > 0) {
    if (val_ptr->n_chars > 0) {
      copy_value_and_append(s.val_ptr->chars_ptr, s.val_ptr->n_chars);
    } else {
      clean_up(val_ptr);
      val_ptr = s.val_ptr;
      val_ptr->ref_count++;
    }
  }
  return *this;
}

string& string::operator+=(const char *s)
{
  if (s == NULL)
    FATAL_ERROR("string::operator+=(const char*): called with NULL");
  size_t s_len = strlen(s);
  if (s_len > 0) copy_value_and_append(s, s_len);
  return *this;
}

bool string::operator==(const string& s) const
{
  if (val_ptr == s.val_ptr) return true;
  else if (val_ptr->n_chars != s.val_ptr->n_chars) return false;
  else return memcmp(val_ptr->chars_ptr, s.val_ptr->chars_ptr,
    val_ptr->n_chars) == 0;
}

bool string::operator==(const char *s) const
{
  if (s == NULL)
    FATAL_ERROR("string::operator==(const char*): called with NULL");
  if (s == val_ptr->chars_ptr) return true;
  else return strcmp(val_ptr->chars_ptr, s) == 0;
}

string operator+(const char *s1, const string& s2)
{
  if (s1 == NULL)
    FATAL_ERROR("operator+(const char *, const string&): called with NULL");
  size_t s1_len = strlen(s1);
  if (s1_len > string::max_string_len - s2.val_ptr->n_chars)
    FATAL_ERROR("operator+(const char *,const string&): length overflow");
  if (s1_len > 0) {
    string s(s1_len + s2.val_ptr->n_chars);
    memcpy(s.val_ptr->chars_ptr, s1, s1_len);
    memcpy(s.val_ptr->chars_ptr + s1_len, s2.val_ptr->chars_ptr,
            s2.val_ptr->n_chars);
    return s;
  } else return s2;
}

void stringpool::clear()
{
  for (size_t i = 0; i < list_len; i++) string::clean_up(string_list[i]);
  Free(string_list);
  string_list = NULL;
  list_size = 0;
  list_len = 0;
}

const char *stringpool::add(const string& s)
{
  if (list_len > list_size) FATAL_ERROR("stringpool::add()");
  else if (list_len == list_size) {
    if (list_size == 0) {
      string_list = static_cast<string::string_struct**>
	(Malloc(STRINGPOOL_INITIAL_SIZE * sizeof(*string_list)));
      list_size = STRINGPOOL_INITIAL_SIZE;
    } else {
      list_size *= STRINGPOOL_INCREMENT_FACTOR;
      string_list = static_cast<string::string_struct**>
	(Realloc(string_list, list_size * sizeof(*string_list)));
    }
    if (list_len >= list_size) FATAL_ERROR("stringpool::add()");
  }
  string::string_struct *val_ptr = s.val_ptr;
  string_list[list_len++] = val_ptr;
  val_ptr->ref_count++;
  return val_ptr->chars_ptr;
}

const char *stringpool::get_str(size_t n) const
{
  if (n >= list_len) FATAL_ERROR("stringpool::get_str()");
  return string_list[n]->chars_ptr;
}

string stringpool::get_string(size_t n) const
{
  if (n >= list_len) FATAL_ERROR("stringpool::get_string()");
  return string(string_list[n]);
}
