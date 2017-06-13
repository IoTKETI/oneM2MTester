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
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "../common/memory.h"
#include "../common/Quadruple.hh"
#include "error.h"

#include "string.hh"
#include "ustring.hh"
#include "PredefFunc.hh"

#include "Int.hh"

/** The amount of memory needed for an ustring containing n characters. */
#define MEMORY_SIZE(n) (sizeof(ustring_struct) + \
  ((n) - 1) * sizeof(universal_char))

void ustring::init_struct(size_t n_uchars)
{
  if (n_uchars == 0) {
    /** This will represent the empty strings so they won't need allocated
     * memory, this delays the memory allocation until it is really needed. */
    static ustring_struct empty_string = { 1, 0, { { '\0', '\0', '\0', '\0' } } };
    val_ptr = &empty_string;
    empty_string.ref_count++;
  } else {
    val_ptr = (ustring_struct*)Malloc(MEMORY_SIZE(n_uchars));
    val_ptr->ref_count = 1;
    val_ptr->n_uchars = n_uchars;
  }
}

void ustring::enlarge_memory(size_t incr)
{
  if (incr > max_string_len - val_ptr->n_uchars)
    FATAL_ERROR("ustring::enlarge_memory(size_t): length overflow");
  size_t new_length = val_ptr->n_uchars + incr;
  if (val_ptr->ref_count == 1) {
    val_ptr = (ustring_struct*)Realloc(val_ptr, MEMORY_SIZE(new_length));
    val_ptr->n_uchars = new_length;
  } else {
    ustring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(new_length);
    memcpy(val_ptr->uchars_ptr, old_ptr->uchars_ptr, old_ptr->n_uchars *
      sizeof(universal_char));
  }
}

void ustring::copy_value()
{
  if (val_ptr->ref_count > 1) {
    ustring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
    init_struct(old_ptr->n_uchars);
    memcpy(val_ptr->uchars_ptr, old_ptr->uchars_ptr,
      old_ptr->n_uchars * sizeof(universal_char));
  }
}

void ustring::clean_up()
{
  if (val_ptr->ref_count > 1) val_ptr->ref_count--;
  else if (val_ptr->ref_count == 1) Free(val_ptr);
  else FATAL_ERROR("ustring::clean_up()");
}

int ustring::compare(const ustring& s) const
{
  if (val_ptr == s.val_ptr) return 0;
  for (size_t i = 0; ; i++) {
    if (i == val_ptr->n_uchars) {
      if (i == s.val_ptr->n_uchars) return 0;
      else return -1;
    } else if (i == s.val_ptr->n_uchars) return 1;
    else if (val_ptr->uchars_ptr[i].group > s.val_ptr->uchars_ptr[i].group)
      return 1;
    else if (val_ptr->uchars_ptr[i].group < s.val_ptr->uchars_ptr[i].group)
      return -1;
    else if (val_ptr->uchars_ptr[i].plane > s.val_ptr->uchars_ptr[i].plane)
      return 1;
    else if (val_ptr->uchars_ptr[i].plane < s.val_ptr->uchars_ptr[i].plane)
      return -1;
    else if (val_ptr->uchars_ptr[i].row > s.val_ptr->uchars_ptr[i].row)
      return 1;
    else if (val_ptr->uchars_ptr[i].row < s.val_ptr->uchars_ptr[i].row)
      return -1;
    else if (val_ptr->uchars_ptr[i].cell > s.val_ptr->uchars_ptr[i].cell)
      return 1;
    else if (val_ptr->uchars_ptr[i].cell < s.val_ptr->uchars_ptr[i].cell)
      return -1;
  }
  return 0; // should never get here
}

ustring::ustring(unsigned char p_group, unsigned char p_plane,
  unsigned char p_row, unsigned char p_cell)
{
  init_struct(1);
  val_ptr->uchars_ptr[0].group = p_group;
  val_ptr->uchars_ptr[0].plane = p_plane;
  val_ptr->uchars_ptr[0].row = p_row;
  val_ptr->uchars_ptr[0].cell = p_cell;
}

ustring::ustring(size_t n, const universal_char *uc_ptr)
{
  // Check for UTF8 encoding and decode it
  // incase the editor encoded the TTCN-3 file with UTF-8
  string octet_str;
  bool isUTF8 = true;
  for (size_t i = 0; i < n; ++i) {
    if (uc_ptr[i].group != 0 || uc_ptr[i].plane != 0 || uc_ptr[i].row != 0) {
      // Not UTF8
      isUTF8 = false;
      break;
    }
    octet_str += Common::hexdigit_to_char(uc_ptr[i].cell / 16);
    octet_str += Common::hexdigit_to_char(uc_ptr[i].cell % 16);
  }
  if (isUTF8) {
    string* ret = Common::get_stringencoding(octet_str);
    if ("UTF-8" != *ret) {
      isUTF8 = false;
    }
    delete ret;
  }
  if (isUTF8) {
    ustring s = Common::decode_utf8(octet_str, CharCoding::UTF_8);
    val_ptr = s.val_ptr;
    val_ptr->ref_count++;
  } else {
    init_struct(n);
    memcpy(val_ptr->uchars_ptr, uc_ptr, n * sizeof(universal_char));
  }
}

ustring::ustring(const string& s)
{
  // Check for UTF8 encoding and decode it
  // incase the editor encoded the TTCN-3 file with UTF-8
  string octet_str;
  bool isUTF8 = true;
  size_t len = s.size();
  for (size_t i = 0; i < len; ++i) {
    octet_str += Common::hexdigit_to_char(static_cast<unsigned char>(s[i]) / 16);
    octet_str += Common::hexdigit_to_char(static_cast<unsigned char>(s[i]) % 16);
  }
  if (isUTF8) {
    string* ret = Common::get_stringencoding(octet_str);
    if ("UTF-8" != *ret) {
      isUTF8 = false;
    }
    delete ret;
  }
  if (isUTF8) {
    ustring str = Common::decode_utf8(octet_str, CharCoding::UTF_8);
    val_ptr = str.val_ptr;
    val_ptr->ref_count++;
  } else {
    init_struct(s.size());
    const char *src = s.c_str();
    for (size_t i = 0; i < val_ptr->n_uchars; i++) {
      val_ptr->uchars_ptr[i].group = 0;
      val_ptr->uchars_ptr[i].plane = 0;
      val_ptr->uchars_ptr[i].row = 0;
      val_ptr->uchars_ptr[i].cell = src[i];
    }
  }
}

ustring::ustring(const char** uid, const int n) {
  //Init the size for characters
  init_struct(n);
  for (size_t i = 0; i < val_ptr->n_uchars; ++i) {
    const char * uidchar = uid[i];
    size_t offset = 1; //Always starts with u or U
    offset = uidchar[1] == '+' ? offset + 1 : offset; //Optional '+'
    string chunk = string(uidchar + offset);
    //Convert hex to int and get value
    Common::int_val_t * val = Common::hex2int(chunk);
    Common::Int int_val = val->get_val();

    //Fill in the quadruple
    val_ptr->uchars_ptr[i].group = (int_val >> 24) & 0xFF;
    val_ptr->uchars_ptr[i].plane = (int_val >> 16) & 0xFF;
    val_ptr->uchars_ptr[i].row   = (int_val >> 8) & 0xFF;
    val_ptr->uchars_ptr[i].cell  = int_val & 0xFF;
    
    //Free pointer
    Free(val);
  }
}

void ustring::clear()
{
  if (val_ptr->n_uchars > 0) {
    clean_up();
    init_struct(0);
  }
}

ustring ustring::substr(size_t pos, size_t n) const
{
  if (pos > val_ptr->n_uchars)
    FATAL_ERROR("ustring::substr(size_t, size_t): position is outside of string");
  if (pos == 0 && n >= val_ptr->n_uchars) return *this;
  if (n > val_ptr->n_uchars - pos) n = val_ptr->n_uchars - pos;
  return ustring(n, val_ptr->uchars_ptr + pos);
}

void ustring::replace(size_t pos, size_t n, const ustring& s)
{
  if (pos > val_ptr->n_uchars)
    FATAL_ERROR("ustring::replace(): start position is outside the string");
  if (pos + n > val_ptr->n_uchars)
    FATAL_ERROR("ustring::replace(): end position is outside the string");
  size_t s_len = s.size();
  /* The replacement string is greater than the maximum string length.  The
     replaced characters are taken into account.  */
  if (s_len > max_string_len - val_ptr->n_uchars + n)
	FATAL_ERROR("ustring::replace(): length overflow");
  size_t new_size = val_ptr->n_uchars - n + s_len;
  if (new_size == 0) {
    clean_up();
    init_struct(0);
  } else {
    ustring_struct *old_ptr = val_ptr;
    old_ptr->ref_count--;
	init_struct(new_size);
	memcpy(val_ptr->uchars_ptr, old_ptr->uchars_ptr,
		   pos * sizeof(universal_char));
    memcpy(val_ptr->uchars_ptr + pos, s.u_str(),
    	   s_len * sizeof(universal_char));
    memcpy(val_ptr->uchars_ptr + pos + s_len, old_ptr->uchars_ptr + pos + n,
	       (old_ptr->n_uchars - pos - n) * sizeof(universal_char));
	if (old_ptr->ref_count == 0) Free(old_ptr);
  }
}

string ustring::get_stringRepr() const
{
  string ret_val;
  enum { INIT, PCHAR, UCHAR } state = INIT;
  for (size_t i = 0; i < val_ptr->n_uchars; i++) {
    const universal_char& uchar = val_ptr->uchars_ptr[i];
    if (uchar.group == 0 && uchar.plane == 0 && uchar.row == 0 &&
	string::is_printable(uchar.cell)) {
      // the actual character is printable
      switch (state) {
      case UCHAR: // concatenation sign if previous part was not printable
	ret_val += " & ";
	// no break
      case INIT: // opening "
	ret_val += '"';
	// no break
      case PCHAR: // the character itself
	ret_val.append_stringRepr(uchar.cell);
	break;
      }
      state = PCHAR;
    } else {
      // the actual character is not printable
      switch (state) {
      case PCHAR: // closing " if previous part was printable
	ret_val += '"';
	// no break
      case UCHAR: // concatenation sign
	ret_val += " & ";
	// no break
      case INIT: // the character itself in quadruple notation
	ret_val += "char(";
	ret_val += Common::Int2string(uchar.group);
	ret_val += ", ";
	ret_val += Common::Int2string(uchar.plane);
	ret_val += ", ";
	ret_val += Common::Int2string(uchar.row);
	ret_val += ", ";
	ret_val += Common::Int2string(uchar.cell);
	ret_val += ')';
	break;
      }
      state = UCHAR;
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

string ustring::get_stringRepr_for_pattern() const {
  string ret_val; // empty string
  for (size_t i = 0; i < val_ptr->n_uchars; i++) {
    const universal_char& uchar = val_ptr->uchars_ptr[i];
    if (uchar.group == 0 && uchar.plane == 0 && uchar.row == 0 &&
      string::is_printable(uchar.cell)) {
      ret_val.append_stringRepr(uchar.cell);
    } else {
      ret_val += "\\q{";
      ret_val += Common::Int2string(uchar.group);
      ret_val += ",";
      ret_val += Common::Int2string(uchar.plane);
      ret_val += ",";
      ret_val += Common::Int2string(uchar.row);
      ret_val += ",";
      ret_val += Common::Int2string(uchar.cell);
      ret_val += "}";
    }
  }
  return ret_val;
}

char* ustring::convert_to_regexp_form() const {
  char* res = (char*)Malloc(val_ptr->n_uchars * 8 + 1);
  char* ptr = res;
  res[val_ptr->n_uchars * 8] = '\0';
  Quad q;
  for (size_t i = 0; i < val_ptr->n_uchars; i++, ptr += 8) {
    const universal_char& uchar = val_ptr->uchars_ptr[i];
    q.set(uchar.group, uchar.plane, uchar.row, uchar.cell);
    Quad::get_hexrepr(q, ptr);
  }
  return res;
}

ustring ustring::extract_matched_section(int start, int end) const
{
  // the indexes refer to the string's regexp form, which contains 8 characters
  // for every universal character in the original string
  start /= 8;
  end /= 8;
  ustring ret_val(end - start);
  memcpy(ret_val.val_ptr->uchars_ptr, val_ptr->uchars_ptr + start, (end - start) *
    sizeof(universal_char));
  return ret_val;
}

ustring& ustring::operator=(const ustring& s)
{
  if(&s != this) {
    clean_up();
    val_ptr = s.val_ptr;
    val_ptr->ref_count++;
  }
  return *this;
}

ustring::universal_char& ustring::operator[](size_t n)
{
  if (n >= val_ptr->n_uchars)
    FATAL_ERROR("ustring::operator[](size_t): position is outside the string");
  copy_value();
  return val_ptr->uchars_ptr[n];
}

const ustring::universal_char& ustring::operator[](size_t n) const
{
  if (n >= val_ptr->n_uchars)
    FATAL_ERROR("ustring::operator[](size_t) const: position is outside the string");
  return val_ptr->uchars_ptr[n];
}

ustring ustring::operator+(const string& s2) const
{
  size_t s2_size = s2.size();
  if (s2_size > max_string_len - val_ptr->n_uchars)
    FATAL_ERROR("ustring::operator+(const string&): length overflow");
  if (s2_size > 0) {
    ustring s(val_ptr->n_uchars + s2_size);
    memcpy(s.val_ptr->uchars_ptr, val_ptr->uchars_ptr, val_ptr->n_uchars *
      sizeof(universal_char));
    const char *src = s2.c_str();
    for (size_t i = 0; i < s2_size; i++) {
      s.val_ptr->uchars_ptr[val_ptr->n_uchars + i].group = 0;
      s.val_ptr->uchars_ptr[val_ptr->n_uchars + i].plane = 0;
      s.val_ptr->uchars_ptr[val_ptr->n_uchars + i].row = 0;
      s.val_ptr->uchars_ptr[val_ptr->n_uchars + i].cell = src[i];
    }
    return s;
  } else return *this;
}

ustring ustring::operator+(const ustring& s2) const
{
  if (s2.val_ptr->n_uchars > max_string_len - val_ptr->n_uchars)
    FATAL_ERROR("ustring::operator+(const ustring&): length overflow");
  if (val_ptr->n_uchars == 0) return s2;
  else if (s2.val_ptr->n_uchars == 0) return *this;
  else {
    ustring s(val_ptr->n_uchars + s2.val_ptr->n_uchars);
    memcpy(s.val_ptr->uchars_ptr, val_ptr->uchars_ptr, val_ptr->n_uchars *
      sizeof(universal_char));
    memcpy(s.val_ptr->uchars_ptr + val_ptr->n_uchars,
      s2.val_ptr->uchars_ptr, s2.val_ptr->n_uchars * sizeof(universal_char)); 
    return s;
  }
}

ustring& ustring::operator+=(const string& s)
{
  size_t s_size = s.size();
  if (s_size > 0) {
    size_t old_size = val_ptr->n_uchars;
    enlarge_memory(s_size);
    const char *src = s.c_str();
    for (size_t i = 0; i < s_size; i++) {
      val_ptr->uchars_ptr[old_size + i].group = 0;
      val_ptr->uchars_ptr[old_size + i].plane = 0;
      val_ptr->uchars_ptr[old_size + i].row = 0;
      val_ptr->uchars_ptr[old_size + i].cell = src[i];
    }
  }
  return *this;
}

ustring& ustring::operator+=(const ustring& s)
{
  if (s.val_ptr->n_uchars > 0) {
    if (val_ptr->n_uchars > 0) {
      size_t old_size = val_ptr->n_uchars, s_size = s.val_ptr->n_uchars;
      enlarge_memory(s_size);
      memcpy(val_ptr->uchars_ptr + old_size, s.val_ptr->uchars_ptr,
	s_size * sizeof(universal_char));
    } else {
      clean_up();
      val_ptr = s.val_ptr;
      val_ptr->ref_count++;
    }
  }
  return *this;
}

bool ustring::operator==(const ustring& s2) const
{
  if (val_ptr == s2.val_ptr) return true;
  else if (val_ptr->n_uchars != s2.val_ptr->n_uchars) return false;
  else return !memcmp(val_ptr->uchars_ptr, s2.val_ptr->uchars_ptr,
    val_ptr->n_uchars * sizeof(universal_char));
}

bool operator==(const ustring::universal_char& uc1,
  const ustring::universal_char& uc2)
{
  return uc1.group == uc2.group && uc1.plane == uc2.plane &&
    uc1.row == uc2.row && uc1.cell == uc2.cell;
}

bool operator<(const ustring::universal_char& uc1,
  const ustring::universal_char& uc2)
{
  if (uc1.group < uc2.group) return true;
  else if (uc1.group > uc2.group) return false;
  else if (uc1.plane < uc2.plane) return true;
  else if (uc1.plane > uc2.plane) return false;
  else if (uc1.row < uc2.row) return true;
  else if (uc1.row > uc2.row) return false;
  else return uc1.cell < uc2.cell;
}

string ustring_to_uft8(const ustring& ustr)
{
  string ret_val;
  for(size_t i = 0; i < ustr.size(); i++) {
    unsigned char g = ustr[i].group;
    unsigned char p = ustr[i].plane;
    unsigned char r = ustr[i].row;
    unsigned char c = ustr[i].cell;
    if(g == 0x00 && p <= 0x1F) {
      if(p == 0x00) {
        if(r == 0x00 && c <= 0x7F) {
          // 1 octet
          ret_val += c;
        } // r
        // 2 octets
        else if(r <= 0x07) {
          ret_val += (0xC0 | r << 2 | c >> 6);
          ret_val += (0x80 | (c & 0x3F));
        } // r
        // 3 octets
        else {
          ret_val += (0xE0 | r >> 4);
          ret_val += (0x80 | (r << 2 & 0x3C) | c >> 6);
          ret_val += (0x80 | (c & 0x3F));
        } // r
      } // p
      // 4 octets
      else {
        ret_val += (0xF0 | p >> 2);
        ret_val += (0x80 | (p << 4 & 0x30) | r >> 4);
        ret_val += (0x80 | (r << 2 & 0x3C) | c >> 6);
        ret_val += (0x80 | (c & 0x3F));
      } // p
    } //g
    // 5 octets
    else if(g <= 0x03) {
      ret_val += (0xF8 | g);
      ret_val += (0x80 | p >> 2);
      ret_val += (0x80 | (p << 4 & 0x30) | r >> 4);
      ret_val += (0x80 | (r << 2 & 0x3C) | c >> 6);
      ret_val += (0x80 | (c & 0x3F));
    } // g
    // 6 octets
    else {
      ret_val += (0xFC | g >> 6);
      ret_val += (0x80 | (g & 0x3F));
      ret_val += (0x80 | p >> 2);
      ret_val += (0x80 | (p << 4 & 0x30) | r >> 4);
      ret_val += (0x80 | (r << 2 & 0x3C) | c >> 6);
      ret_val += (0x80 | (c & 0x3F));
    }
  } // for i
  return ret_val;
}
