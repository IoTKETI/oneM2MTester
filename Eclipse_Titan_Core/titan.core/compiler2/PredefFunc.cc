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
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "PredefFunc.hh"
#include "error.h"
#include "Int.hh"
#include "Real.hh"
#include "Setting.hh"
#include "string.hh"
#include "ustring.hh"
#include "CompilerError.hh"
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>
#include <stdint.h>
#include "../common/memory.h"
#include "../common/pattern.hh"
#include "../common/UnicharPattern.hh"
#include <iostream>

// used by regex
#define ERRMSG_BUFSIZE 512

namespace Common {

  static const char utf32be[] = {'0','0','0','0','F','E','F','F',0};
  static const char utf32le[] = {'F','F','F','E','0','0','0','0',0};
  static const char utf16be[] = {'F','E','F','F',0};
  static const char utf16le[] = {'F','F','F','E',0};
  static const char utf8[]    = {'E','F','B','B','B','F',0};

  static inline unsigned char get_bit_value(char c, unsigned char bit_value)
  {
    switch (c) {
    case '0':
      return 0;
    case '1':
      return bit_value;
    default:
      FATAL_ERROR("Invalid binary digit (%c) in bitstring value", c);
      return 0;
    }
  }

  char toupper (const char c)
  {
    if (('A' <= c && 'F' >= c) ||
        ('0' <= c && '9' >= c)) return c;
    switch (c)
    {
      case 'a' : return 'A';
      case 'b' : return 'B';
      case 'c' : return 'C';
      case 'd' : return 'D';
      case 'e' : return 'E';
      case 'f' : return 'F';
      default:
        FATAL_ERROR("%c cannot be converted to hex character", c);
        break;
    }
  }

  char hexdigit_to_char(unsigned char hexdigit)
  {
    if (hexdigit < 10) return '0' + hexdigit;
    else if (hexdigit < 16) return 'A' + hexdigit - 10;
    else {
      FATAL_ERROR("hexdigit_to_char(): invalid argument: %d", hexdigit);
      return '\0'; // to avoid warning
    }
  }

  unsigned char char_to_hexdigit(char c)
  {
    if (c >= '0' && c <= '9') return c - '0';
    else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    else {
      FATAL_ERROR("char_to_hexdigit(): invalid argument: %c", c);
      return 0; // to avoid warning
    }
  }

  string uchar2str(unsigned char uchar)
  {
    char str[2];
    str[0] = hexdigit_to_char(uchar / 16);
    str[1] = hexdigit_to_char(uchar % 16);
    return string(2, str);
  }

  unsigned char str2uchar(const char& c1, const char& c2)
  {
    unsigned char uc = 0;
    uc = char_to_hexdigit(c1);
    uc <<= 4;
    uc += char_to_hexdigit(c2);
    return uc;
  }

  int_val_t rem(const int_val_t& left, const int_val_t& right)
  {
    return (left - right * (left / right));
  }

  int_val_t mod(const int_val_t& left, const int_val_t& right)
  {
    int_val_t r = right < 0 ? -right : right;
    if (left > 0) {
      return rem(left, r);
    } else {
      int_val_t result = rem(left, r);
      return result == 0 ? result : result + r;
    }
  }

  string* to_uppercase(const string& value)
  {
    string *s = new string(value);
    for (size_t i = 0; i < s->size(); i++) {
      char& c=(*s)[i];
      if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    }
    return s;
  }

  string* not4b_bit(const string& bstr)
  {
    string *s=new string(bstr);
    for(size_t i=0; i<s->size(); i++) {
      char& c=(*s)[i];
      switch(c) {
      case '0': c='1'; break;
      case '1': c='0'; break;
      default:
        FATAL_ERROR("not4b_bit(): Invalid char in bitstring.");
      } // switch c
    } // for i
    return s;
  }

  string* not4b_hex(const string& hstr)
  {
    string *s=new string(hstr);
    for(size_t i=0; i<s->size(); i++) {
      char& c=(*s)[i];
      switch(c) {
      case '0': c='F'; break;
      case '1': c='E'; break;
      case '2': c='D'; break;
      case '3': c='C'; break;
      case '4': c='B'; break;
      case '5': c='A'; break;
      case '6': c='9'; break;
      case '7': c='8'; break;
      case '8': c='7'; break;
      case '9': c='6'; break;
      case 'A': c='5'; break;
      case 'B': c='4'; break;
      case 'C': c='3'; break;
      case 'D': c='2'; break;
      case 'E': c='1'; break;
      case 'F': c='0'; break;
      case 'a': c='5'; break;
      case 'b': c='4'; break;
      case 'c': c='3'; break;
      case 'd': c='2'; break;
      case 'e': c='1'; break;
      case 'f': c='0'; break;
      default:
        FATAL_ERROR("not4b_hex(): Invalid char in hexstring.");
      } // switch c
    } // for i
    return s;
  }

  string* and4b(const string& left, const string& right)
  {
    string *s=new string(left);
    for(size_t i=0; i<s->size(); i++) {
      char& c=(*s)[i];
      c=hexdigit_to_char(char_to_hexdigit(c) & char_to_hexdigit(right[i]));
    } // for i
    return s;
  }

  string* or4b(const string& left, const string& right)
  {
    string *s=new string(left);
    for(size_t i=0; i<s->size(); i++) {
      char& c=(*s)[i];
      c=hexdigit_to_char(char_to_hexdigit(c) | char_to_hexdigit(right[i]));
    } // for i
    return s;
  }

  string* xor4b(const string& left, const string& right)
  {
    string *s=new string(left);
    for(size_t i=0; i<s->size(); i++) {
      char& c=(*s)[i];
      c=hexdigit_to_char(char_to_hexdigit(c) ^ char_to_hexdigit(right[i]));
    } // for i
    return s;
  }

  string* shift_left(const string& value, const Int& count)
  {
    if (count > 0) {
      string *s = new string;
      if (count < static_cast<Int>(value.size())) *s = value.substr(count);
      s->resize(value.size(), '0');
      return s;
    } else if (count < 0) return shift_right(value, -count);
    else return new string(value);
  }

  string* shift_right(const string& value, const Int& count)
  {
    if (count > 0) {
      string *s = new string;
      if (count < static_cast<Int>(value.size())) {
	s->resize(count, '0');
	*s += value.substr(0, value.size()-count);
      } else s->resize(value.size(), '0');
      return s;
    } else if (count < 0) return shift_left(value, -count);
    else return new string(value);
  }

  string* rotate_left(const string& value, const Int& p_count)
  {
    size_t size = value.size();
    if (size == 0) return new string(value);
    else if (p_count < 0) return rotate_right(value, -p_count);
    size_t count = p_count % size;
    if (count == 0) return new string(value);
    else return new string(value.substr(count) + value.substr(0, count));
  }

  string* rotate_right(const string& value, const Int& p_count)
  {
    size_t size = value.size();
    if (size == 0) return new string(value);
    else if (p_count < 0) return rotate_left(value, -p_count);
    size_t count = p_count % size;
    if (count == 0) return new string(value);
    else return new string(value.substr(size - count) +
      value.substr(0, size - count));
  }


  ustring* rotate_left(const ustring& value, const Int& p_count)
  {
    size_t size = value.size();
    if (size == 0) return new ustring(value);
    else if (p_count < 0) return rotate_right(value, -p_count);
    size_t count = p_count % size;
    if (count == 0) return new ustring(value);
    else return new ustring(value.substr(count) + value.substr(0, count));
  }

  ustring* rotate_right(const ustring& value, const Int& p_count)
  {
    size_t size = value.size();
    if (size == 0) return new ustring(value);
    else if (p_count < 0) return rotate_left(value, -p_count);
    size_t count = p_count % size;
    if (count == 0) return new ustring(value);
    else return new ustring(value.substr(size - count) +
      value.substr(0, size - count));
  }

  int_val_t* bit2int(const string& bstr)
  {
    size_t nof_bits = bstr.size();
    // skip the leading zeros
    size_t start_index = 0;
    while (start_index < nof_bits && bstr[start_index] == '0') start_index++;
    int_val_t *ret_val = new int_val_t((Int)0);
    for (size_t i = start_index; i < nof_bits; i++) {
      *ret_val <<= 1;
      if (bstr[i] == '1') *ret_val += 1;
    }
    return ret_val;
  }

  int_val_t* hex2int(const string& hstr)
  {
    size_t nof_digits = hstr.size();
    size_t start_index = 0;
    // Skip the leading zeros.
    while (start_index < nof_digits && hstr[start_index] == '0')
      start_index++;
    int_val_t *ret_val = new int_val_t((Int)0);
    for (size_t i = start_index; i < nof_digits; i++) {
      *ret_val <<= 4;
      *ret_val += char_to_hexdigit(hstr[i]);
    }
    return ret_val;
  }

  Int unichar2int(const ustring& ustr)
  {
    if (ustr.size() != 1) FATAL_ERROR("unichar2int(): invalid argument");
    const ustring::universal_char& uchar = ustr.u_str()[0];
    Int ret_val = (uchar.group << 24) | (uchar.plane << 16) | (uchar.row << 8) |
      uchar.cell;
    return ret_val;
  }

  string *int2bit(const int_val_t& value, const Int& length)
  {
    if (length < 0) FATAL_ERROR("int2bit(): negative length");
    size_t string_length = static_cast<size_t>(length);
    if (static_cast<Int>(string_length) != length ||
	string_length > string::max_string_len)
      FATAL_ERROR("int2bit(): length is too large");
    if (value < 0) FATAL_ERROR("int2bit(): negative value");
    string *bstr = new string;
    bstr->resize(string_length);
    int_val_t tmp_value = value;
    for (size_t i = 1; i <= string_length; i++) {
      (*bstr)[string_length - i] = (tmp_value & 1).get_val() ? '1' : '0';
      tmp_value >>= 1;
    }
    if (tmp_value != 0)
      FATAL_ERROR("int2bit(): %s does not fit in %lu bits", \
	value.t_str().c_str(), (unsigned long)string_length);
    return bstr;
  }

  static const char hdigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

  string *int2hex(const int_val_t& value, const Int& length)
  {
    if (length < 0)
      FATAL_ERROR("int2hex(): negative length");
    size_t string_length = static_cast<size_t>(length);
    if (static_cast<Int>(string_length) != length ||
	string_length > string::max_string_len)
      FATAL_ERROR("int2hex(): length is too large");
    if (value < 0) FATAL_ERROR("int2hex(): negative value");
    string *hstr = new string;
    hstr->resize(string_length);
    int_val_t tmp_value = value;
    for (size_t i = 1; i <= string_length; i++) {
      (*hstr)[string_length - i] = hdigits[(tmp_value & 0x0f).get_val()];
      tmp_value >>= 4;
    }
    if (tmp_value != 0) {
      FATAL_ERROR("int2hex(): %s does not fit in %lu hexadecimal digits",
        value.t_str().c_str(), (unsigned long)string_length);
    }
    return hstr;
  }

  ustring *int2unichar(const Int& value)
  {
    if (value < 0 || value > 2147483647)
      FATAL_ERROR("int2unichar(): invalid argument");
    unsigned char group = (value >> 24) & 0xFF,
      plane = (value >> 16) & 0xFF,
      row = (value >> 8) & 0xFF,
      cell = value & 0xFF;
    return new ustring(group, plane, row, cell);
  }

  string *oct2char(const string& ostr)
  {
    string *cstr = new string;
    size_t ostr_size = ostr.size();
    if (ostr_size % 2)
      FATAL_ERROR("oct2char(): argument has odd length: %lu",
        (unsigned long) ostr_size);
    size_t cstr_size = ostr_size / 2;
    cstr->resize(cstr_size);
    const char *ostr_ptr = ostr.c_str();
    for (size_t i = 0; i < cstr_size; i++) {
      unsigned char c = 16 * char_to_hexdigit(ostr_ptr[2 * i]) +
      char_to_hexdigit(ostr_ptr[2 * i + 1]);
      if (c > 127) FATAL_ERROR("oct2char(): resulting charstring contains " \
                               "non-ascii character: %d", c);
      (*cstr)[i] = c;
    }
    return cstr;
  }

  string *char2oct(const string& cstr)
  {
    string *ostr = new string;
    size_t cstr_size = cstr.size();
    ostr->resize(cstr_size * 2, '0');
    const char *cstr_ptr = cstr.c_str();
    for (size_t i = 0; i < cstr_size; i++) {
      unsigned char c = cstr_ptr[i];
      (*ostr)[2 * i] = hexdigit_to_char(c / 16);
      (*ostr)[2 * i + 1] = hexdigit_to_char(c % 16);
    }
    return ostr;
  }

  string *bit2hex(const string& bstr)
  {
    size_t size=bstr.size();
    size_t hsize=(size+3)/4;
    string *hstr = new string;
    string *bstr4=NULL;
    if(size%4) {
      bstr4=new string;
      bstr4->resize(hsize*4,'0');
      bstr4->replace(4-(size%4),size,bstr);
    }
    hstr->resize(hsize,'0');
    string b4(4,"0000");
    for(size_t i=0;i<hsize;i++) {
      unsigned int u;
      if(size%4)b4=bstr4->substr(i*4,4);
      else b4=bstr.substr(i*4,4);
      if(b4[0]=='1')u=8;else u=0;
      if(b4[1]=='1')u+=4;
      if(b4[2]=='1')u+=2;
      if(b4[3]=='1')u++;
      (*hstr)[i]=hdigits[u];
    }
    if(bstr4!=NULL)delete bstr4;
    return hstr;
  }

  string *hex2oct(const string& hstr)
  {
    if(hstr.size()%2==0)return new string(hstr);
    else {
      string *ostr=new string("0");
      (*ostr)+=hstr;
      return ostr;
    }
  }

  string *asn_hex2oct(const string& hstr)
  {
    string *ostr = new string(hstr);
    size_t size = ostr->size();
    if (size % 2) ostr->resize(size + 1, '0');
    return ostr;
  }

  string *bit2oct(const string& bstr)
  {
    string *s1,*s2;
    s1=bit2hex(bstr);
    s2=hex2oct(*s1);
    delete s1;
    return s2;
  }

  string *asn_bit2oct(const string& bstr)
  {
    size_t size = bstr.size();
    string *ostr = new string;
    ostr->resize(((size+7)/8)*2);
    for(size_t i=0, j=0; i<size; ) {
      unsigned char digit1=0, digit2=0;
      digit1 += get_bit_value(bstr[i++], 8);
      if (i < size) {
        digit1 += get_bit_value(bstr[i++], 4);
        if (i < size) {
          digit1 += get_bit_value(bstr[i++], 2);
          if (i < size) {
            digit1 += get_bit_value(bstr[i++], 1);
            if (i < size) {
              digit2 += get_bit_value(bstr[i++], 8);
              if (i < size) {
                digit2 += get_bit_value(bstr[i++], 4);
                if (i < size) {
                  digit2 += get_bit_value(bstr[i++], 2);
                  if (i < size) digit2 += get_bit_value(bstr[i++], 1);
                }
              }
            }
          }
        }
      }
      (*ostr)[j++] = hexdigit_to_char(digit1);
      (*ostr)[j++] = hexdigit_to_char(digit2);
    }
    return ostr;
  }

  string *hex2bit(const string& hstr)
  {
    size_t size=hstr.size();
    string *bstr = new string;
    bstr->resize(4*size);
    for(size_t i=0; i<size; i++) {
      switch(hstr[i]) {
      case '0':
        bstr->replace(4*i, 4, "0000");
        break;
      case '1':
        bstr->replace(4*i, 4, "0001");
        break;
      case '2':
        bstr->replace(4*i, 4, "0010");
        break;
      case '3':
        bstr->replace(4*i, 4, "0011");
        break;
      case '4':
        bstr->replace(4*i, 4, "0100");
        break;
      case '5':
        bstr->replace(4*i, 4, "0101");
        break;
      case '6':
        bstr->replace(4*i, 4, "0110");
        break;
      case '7':
        bstr->replace(4*i, 4, "0111");
        break;
      case '8':
        bstr->replace(4*i, 4, "1000");
        break;
      case '9':
        bstr->replace(4*i, 4, "1001");
        break;
      case 'A':
      case 'a':
        bstr->replace(4*i, 4, "1010");
        break;
      case 'B':
      case 'b':
        bstr->replace(4*i, 4, "1011");
        break;
      case 'C':
      case 'c':
        bstr->replace(4*i, 4, "1100");
        break;
      case 'D':
      case 'd':
        bstr->replace(4*i, 4, "1101");
        break;
      case 'E':
      case 'e':
        bstr->replace(4*i, 4, "1110");
        break;
      case 'F':
      case 'f':
        bstr->replace(4*i, 4, "1111");
        break;
      default:
        FATAL_ERROR("Common::hex2bit(): invalid hexadecimal "
                    "digit in hexstring value");
      }
    }
    return bstr;
  }

  int_val_t* float2int(const Real& value, const Location& loc)
  {
    // We shouldn't mimic generality with `Int'.
    if (value >= (Real)LLONG_MIN && value <= (Real)LLONG_MAX)
      return new int_val_t((Int)value);
    char buf[512] = "";
    snprintf(buf, 511, "%f", value);
    char *dot = strchr(buf, '.');
    if (!dot) FATAL_ERROR("Conversion of float value `%f' to integer failed", value);
    else memset(dot, 0, sizeof(buf) - (dot - buf));
    return new int_val_t(buf, loc);
  }

/* TTCN-3 float values that have absolute value smaller than this are
   displayed in exponential notation. Same as in core/Float.hh */
#ifndef MIN_DECIMAL_FLOAT
#define MIN_DECIMAL_FLOAT		1.0E-4
#endif
/* TTCN-3 float values that have absolute value larger or equal than
   this are displayed in exponential notation. Same as in
   core/Float.hh */
#ifndef MAX_DECIMAL_FLOAT
#define MAX_DECIMAL_FLOAT		1.0E+10
#endif

  string *float2str(const Real& value)
  {
    char str_buf[64];
    if ( (value > -MAX_DECIMAL_FLOAT && value <= -MIN_DECIMAL_FLOAT)
      || (value >= MIN_DECIMAL_FLOAT && value <   MAX_DECIMAL_FLOAT)
      || (value == 0.0))
      snprintf(str_buf,64,"%f",value);
    else snprintf(str_buf,64,"%e",value);
    return new string(str_buf);
  }

  string* regexp(const string& instr, const string& expression,
                 const Int& groupno, bool nocase)
  {
    string *retval=0;

    if(groupno<0) {
      FATAL_ERROR("regexp(): groupno must be a non-negative integer");
      return retval;
    }
    // do not report the warnings again
    // they were already reported while checking the operands
    unsigned orig_verb_level = verb_level;
    verb_level &= ~(1|2);
    char *posix_str=TTCN_pattern_to_regexp(expression.c_str());
    verb_level = orig_verb_level;
    if(posix_str==NULL) {
      FATAL_ERROR("regexp(): Cannot convert pattern `%s' to POSIX-equivalent.",
                  expression.c_str());
      return retval;
    }

    regex_t posix_regexp;
    int ret_val=regcomp(&posix_regexp, posix_str, REG_EXTENDED |
      (nocase ? REG_ICASE : 0));
    Free(posix_str);
    if(ret_val!=0) {
      /* regexp error */
      char msg[ERRMSG_BUFSIZE];
      regerror(ret_val, &posix_regexp, msg, sizeof(msg));
      FATAL_ERROR("regexp(): regcomp() failed: %s", msg);
      return retval;
    }

    size_t nmatch=groupno+1;
    if(nmatch>posix_regexp.re_nsub) {
      FATAL_ERROR("regexp(): requested groupno is %lu, but this expression "
                  "contains only %lu group(s).", (unsigned long) (nmatch - 1),
		  (unsigned long) posix_regexp.re_nsub);
      return retval;
    }
    regmatch_t* pmatch=(regmatch_t*)Malloc((nmatch+1)*sizeof(regmatch_t));
    ret_val=regexec(&posix_regexp, instr.c_str(), nmatch+1, pmatch, 0);
    if(ret_val==0) {
      if(pmatch[nmatch].rm_so != -1 && pmatch[nmatch].rm_eo != -1)
        retval = new string(instr.substr(pmatch[nmatch].rm_so,
          pmatch[nmatch].rm_eo - pmatch[nmatch].rm_so));
      else retval=new string();
    }
    Free(pmatch);
    if(ret_val!=0) {
      if(ret_val==REG_NOMATCH) {
        regfree(&posix_regexp);
        retval=new string();
      }
      else {
        /* regexp error */
        char msg[ERRMSG_BUFSIZE];
        regerror(ret_val, &posix_regexp, msg, sizeof(msg));
        FATAL_ERROR("regexp(): regexec() failed: %s", msg);
      }
    }
    else regfree(&posix_regexp);

    return retval;
  }

  ustring* regexp(const ustring& instr, const ustring& expression,
    const Int& groupno, bool nocase)
  {
    ustring *retval=0;

    if(groupno<0) {
      FATAL_ERROR("regexp(): groupno must be a non-negative integer");
      return retval;
    }
    // do not report the warnings again
    // they were already reported while checking the operands
    unsigned orig_verb_level = verb_level;
    verb_level &= ~(1|2);
    int* user_groups;
    char *posix_str = TTCN_pattern_to_regexp_uni(
      expression.get_stringRepr_for_pattern().c_str(), nocase, &user_groups);
    if (user_groups == 0)
      FATAL_ERROR("regexp(): Cannot find any groups in the second argument.");
    verb_level = orig_verb_level;
    if(posix_str==NULL) {
      FATAL_ERROR("regexp(): Cannot convert pattern `%s' to POSIX-equivalent.",
        expression.get_stringRepr().c_str());
      return retval;
    }

    regex_t posix_regexp;
    int ret_val=regcomp(&posix_regexp, posix_str, REG_EXTENDED);
    Free(posix_str);
    if(ret_val!=0) {
      /* regexp error */
      char msg[ERRMSG_BUFSIZE];
      regerror(ret_val, &posix_regexp, msg, sizeof(msg));
      FATAL_ERROR("regexp(): regcomp() failed: %s", msg);
      return retval;
    }

    size_t nmatch=user_groups[groupno+1]+1;
    if(nmatch>posix_regexp.re_nsub) {
      FATAL_ERROR("regexp(): requested groupno is %lu, but this expression "
        "contains only %lu group(s).", (unsigned long) (groupno),
        (unsigned long) user_groups[0]);
      return retval;
    }

    Free(user_groups);

    regmatch_t* pmatch = (regmatch_t*)Malloc((nmatch+1)*sizeof(regmatch_t));
    char* tmp = instr.convert_to_regexp_form();
    
    if (nocase) {
      unichar_pattern.convert_regex_str_to_lowercase(tmp);
    }
    
    string instr_conv(tmp);
    Free(tmp);
    ret_val = regexec(&posix_regexp, instr_conv.c_str(), nmatch+1, pmatch, 0);
    if(ret_val == 0) {
      if(pmatch[nmatch].rm_so != -1 && pmatch[nmatch].rm_eo != -1) {
        retval = new ustring(instr.extract_matched_section(pmatch[nmatch].rm_so,
          pmatch[nmatch].rm_eo));
      } else { retval = new ustring(); }
    }
    Free(pmatch);
    if(ret_val!=0) {
      if(ret_val==REG_NOMATCH) {
        regfree(&posix_regexp);
        retval=new ustring();
      }
      else {
        /* regexp error */
        char msg[ERRMSG_BUFSIZE];
        regerror(ret_val, &posix_regexp, msg, sizeof(msg));
        FATAL_ERROR("regexp(): regexec() failed: %s", msg);
      }
    }
    else regfree(&posix_regexp);

    return retval;
  }

string* remove_bom(const string& encoded_value)
{
  size_t length = encoded_value.size();
  if (0 == length) return new string();
  if (length % 2) {
    ERROR("remove_bom(): Wrong string. The number of nibbles (%d) in string "
                "shall be divisible by 2", static_cast<int>(length));
    return new string(encoded_value);
  }

  int length_of_BOM = 0;
  string str_uppercase(encoded_value);
  size_t enough = length > sizeof(utf32be)-1 ? sizeof(utf32be)-1 : length;
  for (size_t i = 0; i < enough; ++i) {
    str_uppercase[i] = (char)toupper(encoded_value[i]);
  }

  if      (str_uppercase.find(utf32be, 0) < length) length_of_BOM = sizeof(utf32be)-1;
  else if (str_uppercase.find(utf32le, 0) < length) length_of_BOM = sizeof(utf32le)-1;
  else if (str_uppercase.find(utf16be, 0) < length) length_of_BOM = sizeof(utf16be)-1;
  else if (str_uppercase.find(utf16le, 0) < length) length_of_BOM = sizeof(utf16le)-1;
  else if (str_uppercase.find(utf8,    0) < length) length_of_BOM = sizeof(utf8)-1;
  else return new string(encoded_value); // no BOM found

  return new string(encoded_value.substr(length_of_BOM, length));
}

static CharCoding::CharCodingType is_ascii (size_t length, const unsigned char* strptr)
{
  const unsigned char nonASCII = 1 << 7;// MSB is 1 in case of non ASCII character  
  CharCoding::CharCodingType ret = CharCoding::ASCII;
  for (size_t i = 0; i < length; ++i) {
    if ( strptr[i] & nonASCII) {
      ret = CharCoding::UNKNOWN;
      break;
    }
  }
  return ret;
}

static CharCoding::CharCodingType is_utf8(size_t length, const unsigned char* strptr)
{
  const unsigned char MSB = 1 << 7; // MSB is 1 in case of non ASCII character  
  const unsigned char MSBmin1 = 1 << 6; // 0100 0000   
  size_t i = 0;
  while (length > i) {
    if ( strptr[i] & MSB) { // non ASCII char
    unsigned char maskUTF8 = 1 << 6; // 111x xxxx shows how many additional bytes are there
      if (!(strptr[i] & maskUTF8)) return CharCoding::UNKNOWN; // accepted 11xxx xxxx but received 10xx xxxx
      unsigned int noofUTF8 = 0; // 11xx xxxxx -> 2 bytes, 111x xxxxx -> 3 bytes , 1111 xxxxx -> 4 bytes in UTF-8
      while (strptr[i] & maskUTF8) {
        ++noofUTF8;
        maskUTF8 >>= 1; // shift right the mask
      }
      // the second and third (and so on) UTF-8 byte looks like 10xx xxxx      
      while (0 < noofUTF8 ) {
        ++i;
        if (!(strptr[i] & MSB) || (strptr[i] & MSBmin1) || i >= length) { // if not like this: 10xx xxxx
          return CharCoding::UNKNOWN;
        }
        --noofUTF8;
      }
    }
    ++i;
  }
  return CharCoding::UTF_8;
}

string* get_stringencoding(const string& encoded_value)
{
  size_t length = encoded_value.size();
  if (0 == length) return new string("<unknown>");
  if (length % 2) {
    ERROR("get_stringencoding(): Wrong string. The number of nibbles (%d) in string "
                "shall be divisible by 2", static_cast<int>(length));
    return new string("<unknown>");
  }

  string str_uppercase(encoded_value);
  size_t enough = length > sizeof(utf32be)-1 ? sizeof(utf32be)-1 : length;
  for (size_t i = 0; i < enough; ++i) {
    str_uppercase[i] = (char)toupper(encoded_value[i]);
  }

  if      (str_uppercase.find(utf32be, 0) < length) return new string("UTF-32BE");
  else if (str_uppercase.find(utf32le, 0) < length) return new string("UTF-32LE");
  else if (str_uppercase.find(utf16be, 0) < length) return new string("UTF-16BE");
  else if (str_uppercase.find(utf16le, 0) < length) return new string("UTF-16LE");
  else if (str_uppercase.find(utf8,    0) < length) return new string("UTF-8");

  unsigned char *uc_str = new unsigned char[length/2];
  string ret;
  for (size_t i = 0; i < length / 2; ++i) {
    uc_str[i] = str2uchar(encoded_value[2 * i], encoded_value[2 * i + 1]);
  }
  if (is_ascii (length / 2, uc_str) == CharCoding::ASCII) ret = "ASCII";
  else if (CharCoding::UTF_8 == is_utf8 (length / 2, uc_str)) ret = "UTF-8";
  else ret = "<unknown>";

  delete [] uc_str;
  return new string(ret);
}

static size_t check_BOM(CharCoding::CharCodingType expected_coding, size_t n_uc, unsigned char* uc_str)
{
  if (0 == n_uc) return 0;

  switch (expected_coding) {
    case CharCoding::UTF32:
    case CharCoding::UTF32BE:
    case CharCoding::UTF32LE:
      if (4 > n_uc) {
        ERROR("decode_utf32(): The string is shorter than the expected BOM");
        return 0;
      }
      break;
    case CharCoding::UTF16:
    case CharCoding::UTF16BE:
    case CharCoding::UTF16LE:
      if (2 > n_uc) {
        ERROR("decode_utf16(): The string is shorter than the expected BOM");
        return 0;
      }
      break;
    default: break;
  }

  //BOM indicates that the byte order is determined by a byte order mark, 
  //if present at the beginning the length of BOM is returned.
  bool badBOM = false;
  string errmsg;
  string caller;
  switch (expected_coding) {
    case CharCoding::UTF32BE:
    case CharCoding::UTF32:
      if (0x00 == uc_str[0] && 0x00 == uc_str[1] && 0xFE == uc_str[2] && 0xFF == uc_str[3]) 
        return 4;
      badBOM = true;
      caller = "decode_utf32()";
      errmsg = "UTF-32BE";
      break;
    case CharCoding::UTF32LE:
      if (0xFF == uc_str[0] && 0xFE == uc_str[1] && 0x00 == uc_str[2] && 0x00 == uc_str[3])
        return 4;
      badBOM = true;
      caller = "decode_utf32()";
      errmsg = "UTF-32LE";
      break;
    case CharCoding::UTF16BE:
    case CharCoding::UTF16:
      if (0xFE == uc_str[0] && 0xFF == uc_str[1])
        return 2;
      badBOM = true;
      caller = "decode_utf16()";
      errmsg = "UTF-16BE";
      break;
    case CharCoding::UTF16LE:
      if (0xFF == uc_str[0] && 0xFE == uc_str[1])
        return 2;
      badBOM = true;
      caller = "decode_utf16()";
      errmsg = "UTF-16LE";
      break;
    case CharCoding::UTF_8:
      if (0xEF == uc_str[0] && 0xBB == uc_str[1] && 0xBF == uc_str[2])
        return 3;
      return 0;
    default:
      if (CharCoding::UTF32 == expected_coding || CharCoding::UTF16 == expected_coding) {
        const char* str = CharCoding::UTF32 == expected_coding ? "UTF-32" : "UTF-16";
        ERROR("Wrong %s string. No BOM detected, however the given coding type (%s) "
               "expects it to define the endianness", str, str);
      }
      else {
        ERROR("Wrong string. No BOM detected");
      }
    }
  if (badBOM) ERROR("%s: Wrong %s string. The expected coding could not be verified",
                    caller.c_str(), errmsg.c_str());
  return 0;
}

static void fill_continuing_octets(int n_continuing, unsigned char *continuing_ptr,
                            size_t n_uc, const unsigned char* uc_str, int start_pos, 
                            int uchar_pos)
{
  for (int i = 0; i < n_continuing; i++) {
    if (start_pos + i < static_cast<int>(n_uc)) {
      unsigned char octet = uc_str[start_pos + i];
      if ((octet & 0xC0) != 0x80) {
        ERROR("decode_utf8(): Malformed: At character position %u, octet position %u: %02X is "
              "not a valid continuing octet.", uchar_pos, start_pos + i, octet);
        return;
      }
      continuing_ptr[i] = octet & 0x3F;
    } 
    else {
      if (start_pos + i == static_cast<int>(n_uc)) {
        if (i > 0) {
    // only a part of octets is missing
          ERROR("decode_utf8(): Incomplete: At character position %d, octet position %d: %d out "
                "of %d continuing octets %s missing from the end of the stream.",
                uchar_pos, start_pos + i, n_continuing - i, n_continuing,
                n_continuing - i > 1 ? "are" : "is");
          return;
        }
        else {
          // all octets are missing
          ERROR("decode_utf8(): Incomplete: At character position %d, octet position %d: %d "
                "continuing octet%s missing from the end of the stream.", uchar_pos,
                start_pos, n_continuing, n_continuing > 1 ? "s are" : " is");
          return;
        }
      }
      continuing_ptr[i] = 0;
    }
  }
}

ustring decode_utf8(const string & ostr, CharCoding::CharCodingType /*expected_coding*/)
{
  size_t length = ostr.size();
  if (0 == length) return ustring();
  if (length % 2) {
    ERROR("decode_utf8(): Wrong UTF-8 string. The number of nibbles (%d) in octetstring "
          "shall be divisible by 2", static_cast<int>(length));
    return ustring();
  }

  unsigned char *uc_str = new unsigned char[length/2];
  for (size_t i = 0; i < length / 2; ++i) {
    uc_str[i] = str2uchar(ostr[2 * i], ostr[2 * i + 1]);
  }
  ustring ucstr;
  size_t start = check_BOM(CharCoding::UTF_8, length /2, uc_str);

  for (size_t i = start; i < length / 2;) {
    // perform the decoding character by character
    if (uc_str[i] <= 0x7F) {
      // character encoded on a single octet: 0xxxxxxx (7 useful bits)
      unsigned char g = 0;
      unsigned char p = 0;
      unsigned char r = 0;
      unsigned char c = uc_str[i];
      ucstr += ustring(g, p, r, c);
      ++i;
    }
    else if (uc_str[i] <= 0xBF) {
      // continuing octet (10xxxxxx) without leading octet ==> malformed
      ERROR("decode_utf8(): Malformed: At character position %d, octet position %d: continuing "
             "octet %02X without leading octet.", static_cast<int>(ucstr.size()),
             static_cast<int>(i), uc_str[i]);
      goto dec_error;
    }
    else if (uc_str[i] <= 0xDF) {
      // character encoded on 2 octets: 110xxxxx 10xxxxxx (11 useful bits)
      unsigned char octets[2];
      octets[0] = uc_str[i] & 0x1F;
      fill_continuing_octets(1, octets + 1, length / 2, uc_str, i + 1, ucstr.size());
      unsigned char g = 0;
      unsigned char p = 0;
      unsigned char r = octets[0] >> 2;
      unsigned char c = octets[0] << 6 | octets[1];
      if (r == 0x00 && c < 0x80) {
        ERROR("decode_utf8(): Overlong: At character position %d, octet position %d: 2-octet "
              "encoding for quadruple (0, 0, 0, %u).", static_cast<int>(ucstr.size()),
              static_cast<int>(i), c);
        goto dec_error;
      }
      ucstr += ustring(g, p, r, c);
      i += 2;
    } 
    else if (uc_str[i] <= 0xEF) {
      // character encoded on 3 octets: 1110xxxx 10xxxxxx 10xxxxxx
      // (16 useful bits)
      unsigned char octets[3];
      octets[0] = uc_str[i] & 0x0F;
      fill_continuing_octets(2, octets + 1, length / 2, uc_str, i + 1,ucstr.size());
      unsigned char g = 0;
      unsigned char p = 0;
      unsigned char r = octets[0] << 4 | octets[1] >> 2;
      unsigned char c = octets[1] << 6 | octets[2];
      if (r < 0x08) {
        ERROR("decode_utf8(): Overlong: At character position %d, octet position %d: 3-octet "
              "encoding for quadruple (0, 0, %u, %u).", static_cast<int>(ucstr.size()),
              static_cast<int>(i), r, c);
        goto dec_error;
      }
      ucstr += ustring(g, p, r, c);
      i += 3;
    } 
    else if (uc_str[i] <= 0xF7) {
      // character encoded on 4 octets: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      // (21 useful bits)
      unsigned char octets[4];
      octets[0] = uc_str[i] & 0x07;
      fill_continuing_octets(3, octets + 1, length / 2, uc_str, i + 1, ucstr.size());
      unsigned char g = 0;
      unsigned char p = octets[0] << 2 | octets[1] >> 4;
      unsigned char r = octets[1] << 4 | octets[2] >> 2;
      unsigned char c = octets[2] << 6 | octets[3];
      if (p == 0x00) {
        ERROR("decode_utf8(): Overlong: At character position %d, octet position %d: 4-octet "
              "encoding for quadruple (0, 0, %u, %u).", static_cast<int>(ucstr.size()),
              static_cast<int>(i), r, c);
        goto dec_error;
      }
      ucstr += ustring(g, p, r, c);
      i += 4;
    }
    else if (uc_str[i] <= 0xFB) {
      // character encoded on 5 octets: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx
      // 10xxxxxx (26 useful bits)
      unsigned char octets[5];
      octets[0] = uc_str[i] & 0x03;
      fill_continuing_octets(4, octets + 1, length / 2, uc_str, i + 1, ucstr.size());
      unsigned char g = octets[0];
      unsigned char p = octets[1] << 2 | octets[2] >> 4;
      unsigned char r = octets[2] << 4 | octets[3] >> 2;
      unsigned char c = octets[3] << 6 | octets[4];
      if (g == 0x00 && p < 0x20) {
        ERROR("decode_utf8(): Overlong: At character position %d, octet position %d: 5-octet "
              "encoding for quadruple (0, %u, %u, %u).", static_cast<int>(ucstr.size()),
              static_cast<int>(i), p, r, c);
        goto dec_error;
      }
      ucstr += ustring(g, p, r, c);
      i += 5;
    }
    else if (uc_str[i] <= 0xFD) {
      // character encoded on 6 octets: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx
      // 10xxxxxx 10xxxxxx (31 useful bits)
      unsigned char octets[6];
      octets[0] = uc_str[i] & 0x01;
      fill_continuing_octets(5, octets + 1, length / 2, uc_str, i + 1,ucstr.size());
      unsigned char g = octets[0] << 6 | octets[1];
      unsigned char p = octets[2] << 2 | octets[3] >> 4;
      unsigned char r = octets[3] << 4 | octets[4] >> 2;
      unsigned char c = octets[4] << 6 | octets[5];
      if (g < 0x04) {
        ERROR("decode_utf8(): Overlong: At character position %d, octet position %d: 6-octet "
              "encoding for quadruple (%u, %u, %u, %u).", static_cast<int>(ucstr.size()),
                    static_cast<int>(i), g, p, r, c);
        goto dec_error;
      }
      ucstr += ustring(g, p, r, c);
      i += 6;
    }
    else {
      // not used code points: FE and FF => malformed
      ERROR("decode_utf8(): Malformed: At character position %d, octet position %d: "
            "unused/reserved octet %02X.", static_cast<int>(ucstr.size()),
            static_cast<int>(i), uc_str[i]);
      goto dec_error;
    }
  }

  dec_error:
  delete[] uc_str;
  return ucstr;
}

}
