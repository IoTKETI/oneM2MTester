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
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "Addfunc.hh"

#include "../common/memory.h"
#include "../common/pattern.hh"
#include "Integer.hh"
#include "Float.hh"
#include "Bitstring.hh"
#include "Hexstring.hh"
#include "Octetstring.hh"
#include "Charstring.hh"
#include "Universal_charstring.hh"
#include "String_struct.hh"
#include "Logger.hh"
#include "Snapshot.hh"
#include "TitanLoggerApi.hh"
#include "../common/UnicharPattern.hh"

#include <openssl/bn.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#define ERRMSG_BUFSIZE 512

// table to reverse the hex digits within an octet
// input: ABCDEFGH, output: DCBAHGFE
static const unsigned char nibble_reverse_table[] =
{
0x00, 0x08, 0x04, 0x0c, 0x02, 0x0a, 0x06, 0x0e,
0x01, 0x09, 0x05, 0x0d, 0x03, 0x0b, 0x07, 0x0f,
0x80, 0x88, 0x84, 0x8c, 0x82, 0x8a, 0x86, 0x8e,
0x81, 0x89, 0x85, 0x8d, 0x83, 0x8b, 0x87, 0x8f,
0x40, 0x48, 0x44, 0x4c, 0x42, 0x4a, 0x46, 0x4e,
0x41, 0x49, 0x45, 0x4d, 0x43, 0x4b, 0x47, 0x4f,
0xc0, 0xc8, 0xc4, 0xcc, 0xc2, 0xca, 0xc6, 0xce,
0xc1, 0xc9, 0xc5, 0xcd, 0xc3, 0xcb, 0xc7, 0xcf,
0x20, 0x28, 0x24, 0x2c, 0x22, 0x2a, 0x26, 0x2e,
0x21, 0x29, 0x25, 0x2d, 0x23, 0x2b, 0x27, 0x2f,
0xa0, 0xa8, 0xa4, 0xac, 0xa2, 0xaa, 0xa6, 0xae,
0xa1, 0xa9, 0xa5, 0xad, 0xa3, 0xab, 0xa7, 0xaf,
0x60, 0x68, 0x64, 0x6c, 0x62, 0x6a, 0x66, 0x6e,
0x61, 0x69, 0x65, 0x6d, 0x63, 0x6b, 0x67, 0x6f,
0xe0, 0xe8, 0xe4, 0xec, 0xe2, 0xea, 0xe6, 0xee,
0xe1, 0xe9, 0xe5, 0xed, 0xe3, 0xeb, 0xe7, 0xef,
0x10, 0x18, 0x14, 0x1c, 0x12, 0x1a, 0x16, 0x1e,
0x11, 0x19, 0x15, 0x1d, 0x13, 0x1b, 0x17, 0x1f,
0x90, 0x98, 0x94, 0x9c, 0x92, 0x9a, 0x96, 0x9e,
0x91, 0x99, 0x95, 0x9d, 0x93, 0x9b, 0x97, 0x9f,
0x50, 0x58, 0x54, 0x5c, 0x52, 0x5a, 0x56, 0x5e,
0x51, 0x59, 0x55, 0x5d, 0x53, 0x5b, 0x57, 0x5f,
0xd0, 0xd8, 0xd4, 0xdc, 0xd2, 0xda, 0xd6, 0xde,
0xd1, 0xd9, 0xd5, 0xdd, 0xd3, 0xdb, 0xd7, 0xdf,
0x30, 0x38, 0x34, 0x3c, 0x32, 0x3a, 0x36, 0x3e,
0x31, 0x39, 0x35, 0x3d, 0x33, 0x3b, 0x37, 0x3f,
0xb0, 0xb8, 0xb4, 0xbc, 0xb2, 0xba, 0xb6, 0xbe,
0xb1, 0xb9, 0xb5, 0xbd, 0xb3, 0xbb, 0xb7, 0xbf,
0x70, 0x78, 0x74, 0x7c, 0x72, 0x7a, 0x76, 0x7e,
0x71, 0x79, 0x75, 0x7d, 0x73, 0x7b, 0x77, 0x7f,
0xf0, 0xf8, 0xf4, 0xfc, 0xf2, 0xfa, 0xf6, 0xfe,
0xf1, 0xf9, 0xf5, 0xfd, 0xf3, 0xfb, 0xf7, 0xff
};

// table to swap the hex digits within an octet
// input: ABCDEFGH, output: EFGHABCD
static const unsigned char nibble_swap_table[] =
{
0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0,
0x01, 0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71,
0x81, 0x91, 0xa1, 0xb1, 0xc1, 0xd1, 0xe1, 0xf1,
0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72,
0x82, 0x92, 0xa2, 0xb2, 0xc2, 0xd2, 0xe2, 0xf2,
0x03, 0x13, 0x23, 0x33, 0x43, 0x53, 0x63, 0x73,
0x83, 0x93, 0xa3, 0xb3, 0xc3, 0xd3, 0xe3, 0xf3,
0x04, 0x14, 0x24, 0x34, 0x44, 0x54, 0x64, 0x74,
0x84, 0x94, 0xa4, 0xb4, 0xc4, 0xd4, 0xe4, 0xf4,
0x05, 0x15, 0x25, 0x35, 0x45, 0x55, 0x65, 0x75,
0x85, 0x95, 0xa5, 0xb5, 0xc5, 0xd5, 0xe5, 0xf5,
0x06, 0x16, 0x26, 0x36, 0x46, 0x56, 0x66, 0x76,
0x86, 0x96, 0xa6, 0xb6, 0xc6, 0xd6, 0xe6, 0xf6,
0x07, 0x17, 0x27, 0x37, 0x47, 0x57, 0x67, 0x77,
0x87, 0x97, 0xa7, 0xb7, 0xc7, 0xd7, 0xe7, 0xf7,
0x08, 0x18, 0x28, 0x38, 0x48, 0x58, 0x68, 0x78,
0x88, 0x98, 0xa8, 0xb8, 0xc8, 0xd8, 0xe8, 0xf8,
0x09, 0x19, 0x29, 0x39, 0x49, 0x59, 0x69, 0x79,
0x89, 0x99, 0xa9, 0xb9, 0xc9, 0xd9, 0xe9, 0xf9,
0x0a, 0x1a, 0x2a, 0x3a, 0x4a, 0x5a, 0x6a, 0x7a,
0x8a, 0x9a, 0xaa, 0xba, 0xca, 0xda, 0xea, 0xfa,
0x0b, 0x1b, 0x2b, 0x3b, 0x4b, 0x5b, 0x6b, 0x7b,
0x8b, 0x9b, 0xab, 0xbb, 0xcb, 0xdb, 0xeb, 0xfb,
0x0c, 0x1c, 0x2c, 0x3c, 0x4c, 0x5c, 0x6c, 0x7c,
0x8c, 0x9c, 0xac, 0xbc, 0xcc, 0xdc, 0xec, 0xfc,
0x0d, 0x1d, 0x2d, 0x3d, 0x4d, 0x5d, 0x6d, 0x7d,
0x8d, 0x9d, 0xad, 0xbd, 0xcd, 0xdd, 0xed, 0xfd,
0x0e, 0x1e, 0x2e, 0x3e, 0x4e, 0x5e, 0x6e, 0x7e,
0x8e, 0x9e, 0xae, 0xbe, 0xce, 0xde, 0xee, 0xfe,
0x0f, 0x1f, 0x2f, 0x3f, 0x4f, 0x5f, 0x6f, 0x7f,
0x8f, 0x9f, 0xaf, 0xbf, 0xcf, 0xdf, 0xef, 0xff
};

// table to reverse the bits within an octet
// input: ABCDEFGH, output: HGFEDCBA
static const unsigned char bit_reverse_table[] =
{
0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static const unsigned char UTF8_BOM[]    = {0xef, 0xbb, 0xbf};
static const unsigned char UTF16BE_BOM[] = {0xfe, 0xff};
static const unsigned char UTF16LE_BOM[] = {0xff, 0xfe};
static const unsigned char UTF32BE_BOM[] = {0x00, 0x00, 0xfe, 0xff};
static const unsigned char UTF32LE_BOM[] = {0xff, 0xfe, 0x00, 0x00};

// Functions for internal purposes

unsigned char char_to_hexdigit(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  else return 0xFF;
}

char hexdigit_to_char(unsigned char hexdigit)
{
  if (hexdigit < 10) return '0' + hexdigit;
  else if (hexdigit < 16) return 'A' + hexdigit - 10;
  else return '\0';
}

static boolean is_whitespace(char c)
{
  switch (c) {
  case ' ':
  case '\t':
  case '\r':
  case '\n':
  case '\v':
  case '\f':
    return TRUE;
  default:
    return FALSE;
  }
}

static CharCoding::CharCodingType is_ascii ( const OCTETSTRING& ostr )
{
  const unsigned char nonASCII = 1 << 7;// MSB is 1 in case of non ASCII character  
  CharCoding::CharCodingType ret = CharCoding::ASCII;
  const unsigned char* strptr = (const unsigned char*)ostr;
  for (int i = 0; i < ostr.lengthof(); ++i) {
    if ( strptr[i] & nonASCII) {
      ret = CharCoding::UNKNOWN;
      break;
    }
  }
  return ret;
}

static CharCoding::CharCodingType is_utf8 ( const OCTETSTRING& ostr )
{
  const unsigned char MSB = 1 << 7; // MSB is 1 in case of non ASCII character  
  const unsigned char MSBmin1 = 1 << 6; // 0100 0000   
  int i = 0;
  const unsigned char* strptr = (const unsigned char*)ostr;
  //  std::cout << "UTF-8 strptr" << strptr << std::endl;  
  while (ostr.lengthof() > i) {
    if ( strptr[i] & MSB) { // non ASCII char
  // std::cout << "UTF-8 strptr[" << i << "]: " << std::hex << (int)strptr[i] << std::endl;
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
  //std::cout << "mask & strptr[" << i << "] " << std::hex << (int)strptr[i]  << std::endl;
        if (!(strptr[i] & MSB) || (strptr[i] & MSBmin1) || i >= ostr.lengthof()) { // if not like this: 10xx xxxx
          return CharCoding::UNKNOWN;
        }
        --noofUTF8;
      }
    }
    ++i;
  }
  return CharCoding::UTF_8;
}

// Additional predefined functions defined in Annex C of ES 101 873-1

// C.1 - int2char

CHARSTRING int2char(int value)
{
  if (value < 0 || value > 127) TTCN_error("The argument of function "
    "int2char() is %d, which is outside the allowed range 0 .. 127.",
    value);
  return CHARSTRING((char)value);
}

CHARSTRING int2char(const INTEGER& value)
{
  value.must_bound("The argument of function int2char() is an unbound "
    "integer value.");
  const int_val_t& ivt = value.get_val();
  if (ivt < 0 || ivt > 127) {
    char *value_str = ivt.as_string();
    TTCN_error("The argument of function int2char() is %s, "
      "which is outside the allowed range 0 .. 127.", value_str);
    Free(value_str);
  }
  return CHARSTRING((char)((int)value));
}

// C.2 - int2unichar

UNIVERSAL_CHARSTRING int2unichar(int value)
{
  if (value < 0 || value > 2147483647) TTCN_error("The argument of function "
    "int2unichar() is %d, which outside the allowed range 0 .. 2147483647.",
    value);
  return UNIVERSAL_CHARSTRING(value >> 24, (value >> 16) & 0xFF,
    (value >> 8) & 0xFF, value & 0xFF);
}

UNIVERSAL_CHARSTRING int2unichar(const INTEGER& value)
{
  value.must_bound("The argument of function int2unichar() is an unbound "
    "integer value.");
  const int_val_t& ivt = value.get_val();
  if (ivt < 0 || ivt > 2147483647) {
    char *value_str = ivt.as_string();
    TTCN_error("The argument of function int2unichar() is %s, "
      "which outside the allowed range 0 .. 2147483647.", value_str);
    Free(value_str);
  }
  return int2unichar((int)value);
}

// C.3 - int2bit

BITSTRING int2bit(const INTEGER& value, int length)
{
  value.must_bound("The first argument (value) of function int2bit() is "
    "an unbound integer value.");
  int_val_t tmp_value(value.get_val());
  if (tmp_value < 0) {
    char *value_str = tmp_value.as_string();
    TTCN_error("The first argument (value) of function "
      "int2bit() is a negative integer value: %s.", value_str);
    Free(value_str);
  }
  if (length < 0) TTCN_error("The second argument (length) of function "
    "int2bit() is a negative integer value: %d.", length);
  BITSTRING ret_val(length);
  unsigned char *bits_ptr = ret_val.val_ptr->bits_ptr;
  // clearing all bits in the result
  memset(bits_ptr, '\0', (length + 7) / 8);
  // we are setting some bits to 1 so we may stop if there are no more 1 bits
  // in the input
  for (int i = length - 1; tmp_value != 0 && i >= 0; i--) {
    if ((tmp_value & 1).get_val()) bits_ptr[i / 8] |= (1 << (i % 8));
    tmp_value >>= 1;
  }
  if (tmp_value != 0) {
    char *value_str = value.get_val().as_string(); // not tmp_value!
    TTCN_error("The first argument of function int2bit(), which is %s, "
      "does not fit in %d bit%s.", value_str, length,
      length > 1 ? "s" :"");
    Free(value_str);
  }
  return ret_val;
}

BITSTRING int2bit(int value, const INTEGER& length)
{
  length.must_bound("The second argument (length) of function int2bit() is "
    "an unbound integer value.");
  return int2bit(INTEGER(value), (int)length);
}

BITSTRING int2bit(int value, int length)
{
  return int2bit(INTEGER(value), length);
}

BITSTRING int2bit(const INTEGER& value, const INTEGER& length)
{
  value.must_bound("The first argument (value) of function int2bit() is "
    "an unbound integer value.");
  length.must_bound("The second argument (length) of function int2bit() is "
    "an unbound integer value.");
  return int2bit(value, (int)length);
}

// C.4 - int2hex

HEXSTRING int2hex(const INTEGER& value, int length)
{
  value.must_bound("The first argument (value) of function int2hex() is "
    "an unbound integer value.");
  int_val_t tmp_value(value.get_val());
  if (value < 0) {
    char *value_str = tmp_value.as_string();
    TTCN_error("The first argument (value) of function int2hex() is a "
      "negative integer value: %s.", value_str);
    Free(value_str);
  }
  if (length < 0) TTCN_error("The second argument (length) of function "
    "int2hex() is a negative integer value: %d.", length);
  HEXSTRING ret_val(length);
  unsigned char *nibbles_ptr = ret_val.val_ptr->nibbles_ptr;
  // clearing the unused bits in the last octet if necessary
  if (length % 2) nibbles_ptr[length / 2] = '\0';
  for (int i = length - 1; i >= 0; i--) {
    if (i % 2) nibbles_ptr[i / 2] = (tmp_value & 0xF).get_val() << 4;
    else nibbles_ptr[i / 2] |= (tmp_value & 0xF).get_val();
    tmp_value >>= 4;
  }
  if (tmp_value != 0) {
    char *value_str = value.get_val().as_string(); // not tmp_value!
    TTCN_error("The first argument of function int2hex(), which is %s, "
      "does not fit in %d hexadecimal digit%s.", value_str, length,
      length > 1 ? "s" :"");
    Free(value_str);  // ???
  }
  return ret_val;
}

HEXSTRING int2hex(int value, const INTEGER& length)
{
  length.must_bound("The second argument (length) of function int2hex() is "
    "an unbound integer value.");
  return int2hex(INTEGER(value), (int)length);
}

HEXSTRING int2hex(int value, int length)
{
  return int2hex(INTEGER(value), length);
}

HEXSTRING int2hex(const INTEGER& value, const INTEGER& length)
{
  value.must_bound("The first argument (value) of function int2hex() is "
    "an unbound integer value.");
  length.must_bound("The second argument (length) of function int2hex() is "
    "an unbound integer value.");
  return int2hex(value, (int)length);
}

// C.5 - int2oct

OCTETSTRING int2oct(int value, int length)
{
  if (value < 0) TTCN_error("The first argument (value) of function "
    "int2oct() is a negative integer value: %d.", value);
  if (length < 0) TTCN_error("The second argument (length) of function "
    "int2oct() is a negative integer value: %d.", length);
  OCTETSTRING ret_val(length);
  unsigned char *octets_ptr = ret_val.val_ptr->octets_ptr;
  unsigned int tmp_value = value;
  for (int i = length - 1; i >= 0; i--) {
    octets_ptr[i] = tmp_value & 0xFF;
    tmp_value >>= 8;
  }
  if (tmp_value != 0) {
    TTCN_error("The first argument of function int2oct(), which is %d, "
      "does not fit in %d octet%s.", value, length,
      length > 1 ? "s" :"");
  }
  return ret_val;
}

OCTETSTRING int2oct(int value, const INTEGER& length)
{
  length.must_bound("The second argument (length) of function int2oct() is "
    "an unbound integer value.");
  return int2oct(value, (int)length);
}

OCTETSTRING int2oct(const INTEGER& value, int length)
{
  value.must_bound("The first argument (value) of function int2oct() is "
    "an unbound integer value.");
  const int_val_t& value_int = value.get_val();
  char *tmp_str = value_int.as_string();
  CHARSTRING value_str(tmp_str);
  Free(tmp_str);
  if (value_int.is_native()) {
    return int2oct((int)value, length);
  } else {
    if (value_int < 0) TTCN_error("The first argument (value) of function "
      "int2oct() is a negative integer value: %s.",
      (const char *)value_str);
    if (length < 0) TTCN_error("The second argument (length) of function "
      "int2oct() is a negative integer value: %d.", length);
    OCTETSTRING ret_val(length);
    unsigned char *octets_ptr = ret_val.val_ptr->octets_ptr;
    BIGNUM *value_tmp = BN_dup(value_int.get_val_openssl());
    int bytes = BN_num_bytes(value_tmp);
    if (bytes > length) {
      BN_free(value_tmp);
      TTCN_error("The first argument of function int2oct(), which is %s, "
        "does not fit in %d octet%s.", (const char *)value_str, length,
        length > 1 ? "s" : "");
    } else {
      unsigned char* tmp = (unsigned char*)Malloc(bytes * sizeof(unsigned char));
      BN_bn2bin(value_tmp, tmp);
      for (int i = length - 1; i >= 0; i--) {
        if (bytes-length+i >= 0) {
          octets_ptr[i] = tmp[bytes-length+i] & 0xff;
        }
        else { // we used up all of the bignum; zero the beginning and quit
          memset(octets_ptr, 0, i+1);
          break;
        }
      }
      BN_free(value_tmp);
      Free(tmp);
      return ret_val;
    }
  }
}

OCTETSTRING int2oct(const INTEGER& value, const INTEGER& length)
{
  value.must_bound("The first argument (value) of function int2oct() is an "
    "unbound integer value.");
  length.must_bound("The second argument (length) of function int2oct() is "
    "an unbound integer value.");
  const int_val_t& value_int = value.get_val();
  if (value_int.is_native()) return int2oct(value_int.get_val(), (int)length);
  return int2oct(value, (int)length);
}

// C.6 - int2str

CHARSTRING int2str(int value)
{
  char str_buf[64];
  int str_len = snprintf(str_buf, sizeof(str_buf), "%d", value);
  if (str_len < 0 || str_len >= (int)sizeof(str_buf)) {
    TTCN_error("Internal error: system call snprintf() returned "
      "unexpected status code %d when converting value %d in function "
      "int2str().", str_len, value);
  }
  return CHARSTRING(str_len, str_buf);
}

CHARSTRING int2str(const INTEGER& value)
{
  value.must_bound("The argument of function int2str() is an unbound "
    "integer value.");
  char *value_tmp = value.get_val().as_string();
  CHARSTRING value_str(value_tmp);
  Free(value_tmp);
  return value_str;
}

// C.7 - int2float

double int2float(int value)
{
  return (double)value;
}

double int2float(const INTEGER& value)
{
  value.must_bound("The argument of function int2float() is an unbound "
    "integer value.");
  return value.get_val().to_real();
}

// C.8 - float2int

INTEGER float2int(double value)
{
  if (value >= (double)INT_MIN && value <= (double)INT_MAX) return (int)value;
  // DBL_MAX has 316 digits including the trailing 0-s on x86_64.
  char buf[512] = "";
  snprintf(buf, 511, "%f", value);
  char *dot = strchr(buf, '.');
  if (!dot) TTCN_error("Conversion of float value `%f' to integer failed", value);
  else memset(dot, 0, sizeof(buf) - (dot - buf));
  return INTEGER(buf);
}

INTEGER float2int(const FLOAT& value)
{
  value.must_bound("The argument of function float2int() is an unbound float "
    "value.");
  return float2int((double)value);
}

// C.9 - char2int

int char2int(char value)
{
  unsigned char uchar_value = value;
  if (uchar_value > 127) TTCN_error("The argument of function "
    "char2int() contains a character with character code %u, which is "
    "outside the allowed range 0 .. 127.", uchar_value);
  return uchar_value;
}

int char2int(const char *value)
{
  if (value == NULL) value = "";
  int value_length = strlen(value);
  if (value_length != 1) TTCN_error("The length of the argument in function "
    "char2int() must be exactly 1 instead of %d.", value_length);
  return char2int(value[0]);
}

int char2int(const CHARSTRING& value)
{
  value.must_bound("The argument of function char2int() is an unbound "
    "charstring value.");
  int value_length = value.lengthof();
  if (value_length != 1) TTCN_error("The length of the argument in function "
    "char2int() must be exactly 1 instead of %d.", value_length);
  return char2int(((const char*)value)[0]);
}

int char2int(const CHARSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function char2int() is an unbound "
    "charstring element.");
  return char2int(value.get_char());
}

// C.10 - char2oct

OCTETSTRING char2oct(const char *value)
{
  if (value == NULL) return OCTETSTRING(0, NULL);
  else return OCTETSTRING(strlen(value), (const unsigned char*)value);
}

OCTETSTRING char2oct(const CHARSTRING& value)
{
  value.must_bound("The argument of function char2oct() is an unbound "
    "charstring value.");
  return OCTETSTRING(value.lengthof(),
    (const unsigned char*)(const char*)value);
}

OCTETSTRING char2oct(const CHARSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function char2oct() is an unbound "
    "charstring element.");
  unsigned char octet = value.get_char();
  return OCTETSTRING(1, &octet);
}

// C.11 - unichar2int

int unichar2int(const universal_char& value)
{
  if (value.uc_group > 127) TTCN_error("The argument of function "
    "unichar2int() is the invalid quadruple char(%u, %u, %u, %u), the "
    "first number of which is outside the allowed range 0 .. 127.",
    value.uc_group, value.uc_plane, value.uc_row, value.uc_cell);
  int result = (value.uc_group << 24) | (value.uc_plane << 16) |
    (value.uc_row << 8) | value.uc_cell;
  return result;
}

int unichar2int(const UNIVERSAL_CHARSTRING& value)
{
  value.must_bound("The argument of function unichar2int() is an unbound "
    "universal charstring value.");
  int value_length = value.lengthof();
  if (value_length != 1) TTCN_error("The length of the argument in function "
    "unichar2int() must be exactly 1 instead of %d.", value_length);
  return unichar2int(((const universal_char*)value)[0]);
}

int unichar2int(const UNIVERSAL_CHARSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function unichar2int() is an unbound "
    "universal charstring element.");
  return unichar2int(value.get_uchar());
}

// C.12 - bit2int

INTEGER bit2int(const BITSTRING& value)
{
  value.must_bound("The argument of function bit2int() is an unbound "
    "bitstring value.");
  int n_bits = value.lengthof();
  const unsigned char *bit_ptr = (const unsigned char *)value;
  // skip the leading zero bits
  int start_index;
  for (start_index = 0; start_index < n_bits; start_index++)
    if (bit_ptr[start_index / 8] & (1 << (start_index % 8))) break;
  // do the conversion
  int_val_t ret_val((RInt)0);
  for (int i = start_index; i < n_bits; i++) {
    ret_val <<= 1;
    if (bit_ptr[i / 8] & (1 << (i % 8))) ret_val += 1;
  }
  if (ret_val.is_native()) return INTEGER(ret_val.get_val());
  else return INTEGER(BN_dup(ret_val.get_val_openssl()));
}

INTEGER bit2int(const BITSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function bit2int() is an unbound "
    "bitstring element.");
  return INTEGER(value.get_bit() ? 1 : 0);
}

// C.13 - bit2hex

HEXSTRING bit2hex(const BITSTRING& value)
{
  value.must_bound("The argument of function bit2hex() is an unbound "
    "bitstring value.");
  int n_bits = value.lengthof();
  int n_nibbles = (n_bits + 3) / 4;
  int padding_bits = 4 * n_nibbles - n_bits;
  const unsigned char *bits_ptr = (const unsigned char *)value;
  HEXSTRING ret_val(n_nibbles);
  unsigned char *nibbles_ptr = ret_val.val_ptr->nibbles_ptr;
  memset(nibbles_ptr, '\0', (n_nibbles + 1) / 2);
  for (int i = 0; i < n_bits; i++) {
    if (bits_ptr[i / 8] & (1 << (i % 8))) {
      nibbles_ptr[(i + padding_bits) / 8] |= 0x80 >>
        ((i + padding_bits + 4) % 8);
    }
  }
  return ret_val;
}

HEXSTRING bit2hex(const BITSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function bit2hex() is an unbound "
    "bitstring element.");
  unsigned char nibble = value.get_bit() ? 0x01 : 0x00;
  return HEXSTRING(1, &nibble);
}

// C.14 - bit2oct

OCTETSTRING bit2oct(const BITSTRING& value)
{
  value.must_bound("The argument of function bit2oct() is an unbound "
    "bitstring value.");
  int n_bits = value.lengthof();
  int n_octets = (n_bits + 7) / 8;
  int padding_bits = 8 * n_octets - n_bits;
  const unsigned char *bits_ptr = (const unsigned char *)value;
  OCTETSTRING ret_val(n_octets);
  unsigned char *octets_ptr = ret_val.val_ptr->octets_ptr;
  memset(octets_ptr, '\0', n_octets);
  for (int i = 0; i < n_bits; i++) {
    if (bits_ptr[i / 8] & (1 << (i % 8))) {
      octets_ptr[(i + padding_bits) / 8] |= 0x80 >>
        ((i + padding_bits) % 8);
    }
  }
  return ret_val;
}

OCTETSTRING bit2oct(const BITSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function bit2oct() is an unbound "
    "bitstring element.");
  unsigned char octet = value.get_bit() ? 0x01 : 0x00;
  return OCTETSTRING(1, &octet);
}

// C.15 - bit2str

CHARSTRING bit2str(const BITSTRING& value)
{
  value.must_bound("The argument of function bit2str() is an unbound "
    "bitstring value.");
  int n_bits = value.lengthof();
  const unsigned char *bits_ptr = (const unsigned char*)value;
  CHARSTRING ret_val(n_bits);
  char *chars_ptr = ret_val.val_ptr->chars_ptr;
  for (int i = 0; i < n_bits; i++) {
    if (bits_ptr[i / 8] & (1 << (i % 8))) chars_ptr[i] = '1';
    else chars_ptr[i] = '0';
  }
  return ret_val;
}

CHARSTRING bit2str(const BITSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function bit2str() is an unbound "
    "bitstring element.");
  return CHARSTRING(value.get_bit() ? '1' : '0');
}

// C.16 - hex2int

INTEGER hex2int(const HEXSTRING& value)
{
  value.must_bound("The argument of function hex2int() is an unbound "
    "hexstring value.");
  int n_nibbles = value.lengthof();
  const unsigned char *nibble_ptr = (const unsigned char *)value;
  // skip the leading zero hex digits
  int start_index;
  for (start_index = 0; start_index < n_nibbles; start_index++) {
    unsigned char mask = start_index % 2 ? 0xF0 : 0x0F;
    if (nibble_ptr[start_index / 2] & mask) break;
  }
  // do the conversion
  int_val_t ret_val((RInt)0);
  for (int i = start_index; i < n_nibbles; i++) {
    ret_val <<= 4;
    if (i % 2) ret_val += nibble_ptr[i / 2] >> 4;
    else ret_val += nibble_ptr[i / 2] & 0x0F;
  }
  if (ret_val.is_native()) return INTEGER(ret_val.get_val());
  else return INTEGER(BN_dup(ret_val.get_val_openssl()));
}

INTEGER hex2int(const HEXSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function hex2int() is an unbound "
    "hexstring element.");
  return INTEGER(value.get_nibble());
}

// C.17 - hex2bit

BITSTRING hex2bit(const HEXSTRING& value)
{
  value.must_bound("The argument of function hex2bit() is an unbound "
    "hexstring value.");
  int n_nibbles = value.lengthof();
  const unsigned char *nibbles_ptr = (const unsigned char *)value;
  BITSTRING ret_val(4 * n_nibbles);
  unsigned char *bits_ptr = ret_val.val_ptr->bits_ptr;
  int n_octets = (n_nibbles + 1) / 2;
  for (int i = 0; i < n_octets; i++) {
    bits_ptr[i] = nibble_reverse_table[nibbles_ptr[i]];
  }
  ret_val.clear_unused_bits();
  return ret_val;
}

BITSTRING hex2bit(const HEXSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function hex2bit() is an unbound "
    "hexstring element.");
  unsigned char bits = nibble_reverse_table[value.get_nibble()];
  return BITSTRING(4, &bits);
}

// C.18 - hex2oct

OCTETSTRING hex2oct(const HEXSTRING& value)
{
  value.must_bound("The argument of function hex2oct() is an unbound "
    "hexstring value.");
  int n_nibbles = value.lengthof();
  int n_octets = (n_nibbles + 1) / 2;
  int padding_nibbles = n_nibbles % 2;
  const unsigned char *nibbles_ptr = (const unsigned char *)value;
  OCTETSTRING ret_val(n_octets);
  unsigned char *octets_ptr = ret_val.val_ptr->octets_ptr;
  if (padding_nibbles > 0) octets_ptr[0] = 0;
  for (int i = 0; i < n_nibbles; i++) {
    unsigned char hexdigit;
    if (i % 2) hexdigit = nibbles_ptr[i / 2] >> 4;
    else hexdigit = nibbles_ptr[i / 2] & 0x0F;
    if ((i + padding_nibbles) % 2)
      octets_ptr[(i + padding_nibbles) / 2] |= hexdigit;
    else octets_ptr[(i + padding_nibbles) / 2] = hexdigit << 4;
  }
  return ret_val;
}

OCTETSTRING hex2oct(const HEXSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function hex2oct() is an unbound "
    "hexstring element.");
  unsigned char octet = value.get_nibble();
  return OCTETSTRING(1, &octet);
}

// C.19 - hex2str

CHARSTRING hex2str(const HEXSTRING& value)
{
  value.must_bound("The argument of function hex2str() is an unbound "
    "hexstring value.");
  int n_nibbles = value.lengthof();
  const unsigned char *nibbles_ptr = (const unsigned char *)value;
  CHARSTRING ret_val(n_nibbles);
  char *chars_ptr = ret_val.val_ptr->chars_ptr;
  for (int i = 0; i < n_nibbles; i++) {
    unsigned char hexdigit;
    if (i % 2) hexdigit = nibbles_ptr[i / 2] >> 4;
    else hexdigit = nibbles_ptr[i / 2] & 0x0F;
    chars_ptr[i] = hexdigit_to_char(hexdigit);
  }
  return ret_val;
}

CHARSTRING hex2str(const HEXSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function hex2str() is an unbound "
    "hexstring element.");
  return CHARSTRING(hexdigit_to_char(value.get_nibble()));
}

// C.20 - oct2int

INTEGER oct2int(const OCTETSTRING& value)
{
  value.must_bound("The argument of function oct2int() is an unbound "
    "octetstring value.");
  int n_octets = value.lengthof();
  const unsigned char *octet_ptr = (const unsigned char *)value;
  int start_index;
  for (start_index = 0; start_index < n_octets; start_index++)
    if (octet_ptr[start_index]) break;
  int_val_t ret_val((RInt)0);
  for (int i = start_index; i < n_octets; i++) {
    ret_val <<= 8;
    ret_val += octet_ptr[i];
  }
  if (ret_val.is_native()) return INTEGER(ret_val.get_val());
  else return INTEGER(BN_dup(ret_val.get_val_openssl()));
}

INTEGER oct2int(const OCTETSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function oct2int() is an unbound "
    "octetstring element.");
  return INTEGER(value.get_octet());
}

// C.21 - oct2bit

BITSTRING oct2bit(const OCTETSTRING& value)
{
  value.must_bound("The argument of function oct2bit() is an unbound "
    "octetstring value.");
  int n_octets = value.lengthof();
  const unsigned char *octets_ptr = (const unsigned char *)value;
  BITSTRING ret_val(8 * n_octets);
  unsigned char *bits_ptr = ret_val.val_ptr->bits_ptr;
  for (int i = 0; i < n_octets; i++) {
    bits_ptr[i] = bit_reverse_table[octets_ptr[i]];
  }
  return ret_val;
}

BITSTRING oct2bit(const OCTETSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function oct2bit() is an unbound "
    "octetstring element.");
  unsigned char bits = bit_reverse_table[value.get_octet()];
  return BITSTRING(8, &bits);
}

// C.22 - oct2hex

HEXSTRING oct2hex(const OCTETSTRING& value)
{
  value.must_bound("The argument of function oct2hex() is an unbound "
    "octetstring value.");
  int n_octets = value.lengthof();
  const unsigned char *octets_ptr = (const unsigned char *)value;
  HEXSTRING ret_val(2 * n_octets);
  unsigned char *nibbles_ptr = ret_val.val_ptr->nibbles_ptr;
  for (int i = 0; i < n_octets; i++) {
    nibbles_ptr[i] = nibble_swap_table[octets_ptr[i]];
  }
  return ret_val;
}

HEXSTRING oct2hex(const OCTETSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function oct2hex() is an unbound "
    "octetstring element.");
  unsigned char nibbles = nibble_swap_table[value.get_octet()];
  return HEXSTRING(2, &nibbles);
}

// C.23 - oct2str

CHARSTRING oct2str(const OCTETSTRING& value)
{
  value.must_bound("The argument of function oct2str() is an unbound "
    "octetstring value.");
  int n_octets = value.lengthof();
  const unsigned char *octets_ptr = (const unsigned char *)value;
  CHARSTRING ret_val(2 * n_octets);
  char *chars_ptr = ret_val.val_ptr->chars_ptr;
  for (int i = 0; i < n_octets; i++) {
    chars_ptr[2 * i] = hexdigit_to_char(octets_ptr[i] >> 4);
    chars_ptr[2 * i + 1] = hexdigit_to_char(octets_ptr[i] & 0x0F);
  }
  return ret_val;
}

CHARSTRING oct2str(const OCTETSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function oct2str() is an unbound "
    "octetstring element.");
  unsigned char octet = value.get_octet();
  char result[2];
  result[0] = hexdigit_to_char(octet >> 4);
  result[1] = hexdigit_to_char(octet & 0x0F);
  return CHARSTRING(2, result);
}

// C.24 - oct2char

CHARSTRING oct2char(const OCTETSTRING& value)
{
  value.must_bound("The argument of function oct2char() is an unbound "
    "octetstring value.");
  int value_length = value.lengthof();
  const unsigned char *octets_ptr = (const unsigned char*)value;
  for (int i = 0; i < value_length; i++) {
    unsigned char octet = octets_ptr[i];
    if (octet > 127) TTCN_error("The argument of function oct2char() "
      "contains octet %02X at index %d, which is outside the allowed "
      "range 00 .. 7F.", octet, i);
  }
  return CHARSTRING(value_length, (const char*)octets_ptr);
}

CHARSTRING oct2char(const OCTETSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function oct2char() is an unbound "
    "octetstring element.");
  unsigned char octet = value.get_octet();
  if (octet > 127) TTCN_error("The argument of function oct2char() "
    "contains the octet %02X, which is outside the allowed range 00 .. 7F.",
    octet);
  return CHARSTRING((char)octet);
}

UNIVERSAL_CHARSTRING oct2unichar(const OCTETSTRING& invalue)
{
// default encoding is UTF-8
  UNIVERSAL_CHARSTRING ucstr;
  TTCN_EncDec::error_behavior_t err_behavior = TTCN_EncDec::get_error_behavior(TTCN_EncDec::ET_DEC_UCSTR);
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_DEC_UCSTR, TTCN_EncDec::EB_ERROR);
  ucstr.decode_utf8(invalue.lengthof(), (const unsigned char *)invalue, CharCoding::UTF_8, TRUE);
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_DEC_UCSTR, err_behavior);
  return ucstr;
}

UNIVERSAL_CHARSTRING oct2unichar(const OCTETSTRING& invalue,
                                 const CHARSTRING& string_encoding)
{
  UNIVERSAL_CHARSTRING ucstr;
  TTCN_EncDec::error_behavior_t err_behavior = TTCN_EncDec::get_error_behavior(TTCN_EncDec::ET_DEC_UCSTR);
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_DEC_UCSTR, TTCN_EncDec::EB_ERROR);
  if ("UTF-8" == string_encoding) {
    ucstr.decode_utf8(invalue.lengthof(), (const unsigned char *)invalue, CharCoding::UTF_8, TRUE);
  }
  else if ("UTF-16" == string_encoding) {
    ucstr.decode_utf16(invalue.lengthof(), (const unsigned char *)invalue, CharCoding::UTF16);
  }
  else if ("UTF-16BE" == string_encoding) {
    ucstr.decode_utf16(invalue.lengthof(), (const unsigned char *)invalue, CharCoding::UTF16BE);
  }
  else if ("UTF-16LE" == string_encoding) {
    ucstr.decode_utf16(invalue.lengthof(), (const unsigned char *)invalue, CharCoding::UTF16LE);
  }
  else if ("UTF-32" == string_encoding) {
    ucstr.decode_utf32(invalue.lengthof(), (const unsigned char *)invalue, CharCoding::UTF32);
  }
  else if ("UTF-32BE" == string_encoding) {
    ucstr.decode_utf32(invalue.lengthof(), (const unsigned char *)invalue, CharCoding::UTF32BE);
  }
  else if ("UTF-32LE" == string_encoding) {
    ucstr.decode_utf32(invalue.lengthof(), (const unsigned char *)invalue, CharCoding::UTF32LE);
  }
  else {    TTCN_error("oct2unichar: Invalid parameter: %s", (const char*)string_encoding);
  }
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_DEC_UCSTR, err_behavior);
  return ucstr;
}

// C.25 - str2int

INTEGER str2int(const char *value)
{
  return str2int(CHARSTRING(value));
}

INTEGER str2int(const CHARSTRING& value)
{
  value.must_bound("The argument of function str2int() is an unbound "
    "charstring value.");
  int value_len = value.lengthof();
  if (value_len == 0) TTCN_error("The argument of function str2int() is an "
    "empty string, which does not represent a valid integer value.");
  const char *value_str = value;
  enum { S_INITIAL, S_FIRST, S_ZERO, S_MORE, S_END, S_ERR } state = S_INITIAL;
  // state: expected characters
  // S_INITIAL: +, -, first digit, leading whitespace
  // S_FIRST: first digit
  // S_ZERO, S_MORE: more digit(s), trailing whitespace
  // S_END: trailing whitespace
  // S_ERR: error was found, stop
  boolean leading_ws = FALSE, leading_zero = FALSE;
  for (int i = 0; i < value_len; i++) {
    char c = value_str[i];
    switch (state) {
    case S_INITIAL:
      if (c == '+' || c == '-') state = S_FIRST;
      else if (c == '0') state = S_ZERO;
      else if (c >= '1' && c <= '9') state = S_MORE;
      else if (is_whitespace(c)) leading_ws = TRUE;
      else state = S_ERR;
      break;
    case S_FIRST:
      if (c == '0') state = S_ZERO;
      else if (c >= '1' && c <= '9') state = S_MORE;
      else state = S_ERR;
      break;
    case S_ZERO:
      if (c >= '0' && c <= '9') {
        leading_zero = TRUE;
        state = S_MORE;
      } else if (is_whitespace(c)) state = S_END;
      else state = S_ERR;
      break;
    case S_MORE:
      if (c >= '0' && c <= '9') {}
      else if (is_whitespace(c)) state = S_END;
      else state = S_ERR;
      break;
    case S_END:
      if (!is_whitespace(c)) state = S_ERR;
      break;
    default:
      break;
    }
    if (state == S_ERR) {
      TTCN_error_begin("The argument of function str2int(), which is ");
      value.log();
      TTCN_Logger::log_event_str(", does not represent a valid integer "
        "value. Invalid character `");
      TTCN_Logger::log_char_escaped(c);
      TTCN_Logger::log_event("' was found at index %d.", i);
      TTCN_error_end();
    }
  }
  if (state != S_ZERO && state != S_MORE && state != S_END) {
    TTCN_error_begin("The argument of function str2int(), which is ");
    value.log();
    TTCN_Logger::log_event_str(", does not represent a valid integer "
      "value. Premature end of the string.");
    TTCN_error_end();
  }
  if (leading_ws) {
    TTCN_warning_begin("Leading whitespace was detected in the argument "
      "of function str2int(): ");
    value.log();
    TTCN_Logger::log_char('.');
    TTCN_warning_end();
  }
  if (leading_zero) {
    TTCN_warning_begin("Leading zero digit was detected in the argument "
      "of function str2int(): ");
    value.log();
    TTCN_Logger::log_char('.');
    TTCN_warning_end();
  }
  if (state == S_END) {
    TTCN_warning_begin("Trailing whitespace was detected in the argument "
      "of function str2int(): ");
    value.log();
    TTCN_Logger::log_char('.');
    TTCN_warning_end();
  }
  return INTEGER(value_str);
}

INTEGER str2int(const CHARSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function str2int() is an unbound "
    "charstring element.");
  char c = value.get_char();
  if (c < '0' || c > '9') {
    TTCN_error_begin("The argument of function str2int(), which is a "
      "charstring element containing character `");
    TTCN_Logger::log_char_escaped(c);
    TTCN_Logger::log_event_str("', does not represent a valid integer "
      "value.");
    TTCN_error_end();
  }
  return INTEGER(c - '0');
}

// C.26 - str2oct

OCTETSTRING str2oct(const char *value)
{
  if (value == NULL) return OCTETSTRING(0, NULL);
  else return str2oct(CHARSTRING(value));
}

OCTETSTRING str2oct(const CHARSTRING& value)
{
  value.must_bound("The argument of function str2oct() is an unbound "
    "charstring value.");
  int value_len = value.lengthof();
  if (value_len % 2) TTCN_error("The argument of function str2oct() must "
    "have even number of characters containing hexadecimal digits, but "
    "the length of the string is odd: %d.", value_len);
  OCTETSTRING ret_val(value_len / 2);
  unsigned char *octets_ptr = ret_val.val_ptr->octets_ptr;
  const char *chars_ptr = value;
  for (int i = 0; i < value_len; i++) {
    char c = chars_ptr[i];
    unsigned char hex_digit = char_to_hexdigit(c);
    if (hex_digit > 0x0F) {
      TTCN_error_begin("The argument of function str2oct() shall "
        "contain hexadecimal digits only, but character `");
      TTCN_Logger::log_char_escaped(c);
      TTCN_Logger::log_event("' was found at index %d.", i);
      TTCN_error_end();
    }
    if (i % 2) octets_ptr[i / 2] |= hex_digit;
    else octets_ptr[i / 2] = hex_digit << 4;
  }
  return ret_val;
}

// C.27 - str2float

double str2float(const char *value)
{
  return str2float(CHARSTRING(value));
}

/*
 * leading zeros are allowed;
 * leading "+" sign before positive values is allowed;
 * "-0.0" is allowed;
 * no numbers after the dot in the decimal notation are allowed.
 */
double str2float(const CHARSTRING& value)
{
  value.must_bound("The argument of function str2float() is an unbound "
    "charstring value.");
  int value_len = value.lengthof();
  if (value_len == 0) TTCN_error("The argument of function str2float() is "
    "an empty string, which does not represent a valid float value.");
  const char *value_str = value;
  enum { S_INITIAL, S_FIRST_M, S_ZERO_M, S_MORE_M, S_FIRST_F, S_MORE_F,
    S_INITIAL_E, S_FIRST_E, S_ZERO_E, S_MORE_E, S_END, S_ERR }
  state = S_INITIAL;
  // state: expected characters
  // S_INITIAL: +, -, first digit of integer part in mantissa,
  //            leading whitespace
  // S_FIRST_M: first digit of integer part in mantissa
  // S_ZERO_M, S_MORE_M: more digits of mantissa, decimal dot, E
  // S_FIRST_F: first digit of fraction
  // S_MORE_F: more digits of fraction, E, trailing whitespace
  // S_INITIAL_E: +, -, first digit of exponent
  // S_FIRST_E: first digit of exponent
  // S_ZERO_E, S_MORE_E: more digits of exponent, trailing whitespace
  // S_END: trailing whitespace
  // S_ERR: error was found, stop
  boolean leading_ws = FALSE, leading_zero = FALSE;
  for (int i = 0; i < value_len; i++) {
    char c = value_str[i];
    switch (state) {
    case S_INITIAL:
      if (c == '+' || c == '-') state = S_FIRST_M;
      else if (c == '0') state = S_ZERO_M;
      else if (c >= '1' && c <= '9') state = S_MORE_M;
      else if (is_whitespace(c)) leading_ws = TRUE;
      else state = S_ERR;
      break;
    case S_FIRST_M: // first mantissa digit
      if (c == '0') state = S_ZERO_M;
      else if (c >= '1' && c <= '9') state = S_MORE_M;
      else state = S_ERR;
      break;
    case S_ZERO_M: // leading mantissa zero
      if (c == '.') state = S_FIRST_F;
      else if (c == 'E' || c == 'e') state = S_INITIAL_E;
      else if (c >= '0' && c <= '9') {
        leading_zero = TRUE;
        state = S_MORE_M;
      } else state = S_ERR;
      break;
    case S_MORE_M:
      if (c == '.') state = S_FIRST_F;
      else if (c == 'E' || c == 'e') state = S_INITIAL_E;
      else if (c >= '0' && c <= '9') {}
      else state = S_ERR;
      break;
    case S_FIRST_F:
      if (c >= '0' && c <= '9') state = S_MORE_F;
      else state = S_ERR;
      break;
    case S_MORE_F:
      if (c == 'E' || c == 'e') state = S_INITIAL_E;
      else if (c >= '0' && c <= '9') {}
      else if (is_whitespace(c)) state = S_END;
      else state = S_ERR;
      break;
    case S_INITIAL_E:
      if (c == '+' || c == '-') state = S_FIRST_E;
      else if (c == '0') state = S_ZERO_E;
      else if (c >= '1' && c <= '9') state = S_MORE_E;
      else state = S_ERR;
      break;
    case S_FIRST_E:
      if (c == '0') state = S_ZERO_E;
      else if (c >= '1' && c <= '9') state = S_MORE_E;
      else state = S_ERR;
      break;
    case S_ZERO_E:
      if (c >= '0' && c <= '9') {
        leading_zero = TRUE;
        state = S_MORE_E;
      } else if (is_whitespace(c)) state = S_END;
      else state = S_ERR;
      break;
    case S_MORE_E:
      if (c >= '0' && c <= '9') {}
      else if (is_whitespace(c)) state = S_END;
      else state = S_ERR;
      break;
    case S_END:
      if (!is_whitespace(c)) state = S_ERR;
      break;
    default:
      break;
    }
    if (state == S_ERR) {
      TTCN_error_begin("The argument of function str2float(), which is ");
      value.log();
      TTCN_Logger::log_event_str(", does not represent a valid float "
        "value. Invalid character `");
      TTCN_Logger::log_char_escaped(c);
      TTCN_Logger::log_event("' was found at index %d.", i);
      TTCN_error_end();
    }
  }
  switch (state) {
  case S_MORE_F:
  case S_ZERO_E:
  case S_MORE_E:
    // OK, originally
    break;
  case S_ZERO_M:
  case S_MORE_M:
    // OK now (decimal dot missing after mantissa)
    break;
  case S_FIRST_F:
    // OK now (fraction part missing)
    break;
  default:
    TTCN_error_begin("The argument of function str2float(), which is ");
    value.log();
    TTCN_Logger::log_event_str(", does not represent a valid float value. "
      "Premature end of the string.");
    TTCN_error_end();
    break;
  }
  if (leading_ws) {
    TTCN_warning_begin("Leading whitespace was detected in the argument "
      "of function str2float(): ");
    value.log();
    TTCN_Logger::log_char('.');
    TTCN_warning_end();
  }
  if (leading_zero) {
    TTCN_warning_begin("Leading zero digit was detected in the argument "
      "of function str2float(): ");
    value.log();
    TTCN_Logger::log_char('.');
    TTCN_warning_end();
  }
  if (state == S_END) {
    TTCN_warning_begin("Trailing whitespace was detected in the argument "
      "of function str2float(): ");
    value.log();
    TTCN_Logger::log_char('.');
    TTCN_warning_end();
  }
  return atof(value_str);
}

// C.33 - regexp

CHARSTRING regexp(const CHARSTRING& instr, const CHARSTRING& expression,
  int groupno, boolean nocase)
{
  instr.must_bound("The first argument (instr) of function regexp() is an "
    "unbound charstring value.");
  expression.must_bound("The second argument (expression) of function "
    "regexp() is an unbound charstring value.");
  if (groupno < 0) TTCN_error("The third argument (groupno) of function "
    "regexp() is a negative integer value: %d.", groupno);
  int instr_len = instr.lengthof();
  const char *instr_str = instr;
  for (int i = 0; i < instr_len; i++) {
    if (instr_str[i] == '\0') {
      TTCN_warning_begin("The first argument (instr) of function regexp(), "
        "which is ");
      instr.log();
      TTCN_Logger::log_event(", contains a character with zero character code "
        "at index %d. The rest of the string will be ignored during matching.",
        i);
      TTCN_warning_end();
      break;
    }
  }
  int expression_len = expression.lengthof();
  const char *expression_str = expression;
  for (int i = 0; i < expression_len; i++) {
    if (expression_str[i] == '\0') {
      TTCN_warning_begin("The second argument (expression) of function "
        "regexp(), which is ");
      expression.log();
      TTCN_Logger::log_event(", contains a character with zero character code "
        "at index %d. The rest of the string will be ignored during matching.",
        i);
      TTCN_warning_end();
      break;
    }
  }
  char *posix_str = TTCN_pattern_to_regexp(expression_str);
  if (posix_str == NULL) {
    TTCN_error_begin("The second argument (expression) of function "
      "regexp(), which is ");
    expression.log();
    TTCN_Logger::log_event(", is not a valid TTCN-3 character pattern.");
    TTCN_error_end();
  }
  if (TTCN_Logger::log_this_event(TTCN_Logger::DEBUG_UNQUALIFIED)) {
    TTCN_Logger::begin_event(TTCN_Logger::DEBUG_UNQUALIFIED);
    TTCN_Logger::log_event_str("regexp(): POSIX ERE equivalent of ");
    CHARSTRING_template(STRING_PATTERN, expression, nocase).log();
    TTCN_Logger::log_event_str(" is: ");
    CHARSTRING(posix_str).log();
    TTCN_Logger::end_event();
  }
  regex_t posix_regexp;
  int ret_val = regcomp(&posix_regexp, posix_str, REG_EXTENDED |
    (nocase ? REG_ICASE : 0));
  Free(posix_str);
  if (ret_val != 0) {
    char msg[ERRMSG_BUFSIZE];
    regerror(ret_val, &posix_regexp, msg, sizeof(msg));
    regfree(&posix_regexp);
    TTCN_error_begin("Internal error: Compilation of POSIX regular expression "
      "failed in function regexp() when trying to match with character "
      "pattern ");
    expression.log();
    TTCN_Logger::log_event(". Error message: %s.", msg);
    TTCN_error_end();
  }
  int re_nsub = posix_regexp.re_nsub;
  if (re_nsub <= 0) {
    regfree(&posix_regexp);
    TTCN_error_begin("The character pattern in the second argument "
      "(expression) of function regexp() does not contain any groups: ");
    expression.log();
    TTCN_Logger::log_char('.');
    TTCN_error_end();
  }
  if (groupno >= re_nsub) {
    regfree(&posix_regexp);
    TTCN_error("The third argument (groupno) of function regexp() is too "
      "large: The requested group index is %d, but the pattern contains only "
      "%d group%s.", groupno, re_nsub, re_nsub > 1 ? "s" : "");
  }
  size_t nmatch = groupno + 1;
  regmatch_t *pmatch = (regmatch_t*)Malloc((nmatch + 1) * sizeof(*pmatch));
  ret_val = regexec(&posix_regexp, instr, nmatch + 1, pmatch, 0);
  if (ret_val == 0) {
    int begin_index = pmatch[nmatch].rm_so, end_index = pmatch[nmatch].rm_eo;
    Free(pmatch);
    regfree(&posix_regexp);
    if (end_index > instr_len) TTCN_error("Internal error: The end index "
      "of the substring (%d) to be returned in function regexp() is greater "
      "than the length of the input string (%d).", end_index, instr_len);
    if (begin_index > end_index) TTCN_error("Internal error: The start index "
      "of the substring (%d) to be returned in function regexp() is greater "
      "than the end index (%d).", begin_index, end_index);
    return CHARSTRING(end_index - begin_index, instr_str + begin_index);
  } else {
    Free(pmatch);
    if (ret_val != REG_NOMATCH) {
      char msg[ERRMSG_BUFSIZE];
      regerror(ret_val, &posix_regexp, msg, ERRMSG_BUFSIZE);
      regfree(&posix_regexp);
      TTCN_error("Internal error: POSIX regular expression matching returned "
        "unexpected status code in function regexp(): %s.", msg);
    } else regfree(&posix_regexp);
    return CHARSTRING(0, NULL);
  }
}

CHARSTRING regexp(const CHARSTRING& instr, const CHARSTRING& expression,
  const INTEGER& groupno, boolean nocase)
{
  groupno.must_bound("The third argument (groupno) of function regexp() is an "
    "unbound integer value.");
  return regexp(instr, expression, (int)groupno, nocase);
}

UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING& instr,
  const UNIVERSAL_CHARSTRING* expression_val,
  const UNIVERSAL_CHARSTRING_template* expression_tmpl,
  int groupno, boolean nocase)
{
  if ( (expression_val && expression_tmpl) ||
    (!expression_val && !expression_tmpl) )
    TTCN_error("Internal error: regexp(): invalid parameters");
  instr.must_bound("The first argument (instr) of function regexp() is an "
    "unbound charstring value.");
  if (expression_val)
    expression_val->must_bound("The second argument (expression) of function "
      "regexp() is an unbound universal charstring value.");
  else {
    if (!expression_tmpl->is_bound())
      TTCN_error("The second argument (expression) of function "
        "regexp() is an unbound universal charstring template.");
  }
  if (groupno < 0) TTCN_error("The third argument (groupno) of function "
    "regexp() is a negative integer value: %d.", groupno);

  int* user_groups = 0;
  CHARSTRING expression_str;
  if (expression_val)
    expression_str = expression_val->get_stringRepr_for_pattern();
  else
    expression_str = expression_tmpl->get_single_value();
  char *posix_str = TTCN_pattern_to_regexp_uni((const char*)expression_str,
    nocase, &user_groups);
  if (user_groups == 0) {
    Free(user_groups);
    Free(posix_str);
    TTCN_error("Cannot find any groups in the second argument of regexp().");
  }
  if (posix_str == NULL) {
    TTCN_error_begin("The second argument (expression) of function "
      "regexp(), which is ");
    if (expression_val) expression_val->log(); else expression_tmpl->log();
    TTCN_Logger::log_event(", is not a valid TTCN-3 character pattern.");
    TTCN_error_end();
  }
  if (TTCN_Logger::log_this_event(TTCN_Logger::DEBUG_UNQUALIFIED)) {
    TTCN_Logger::begin_event(TTCN_Logger::DEBUG_UNQUALIFIED);
    TTCN_Logger::log_event_str("regexp(): POSIX ERE equivalent of ");
    CHARSTRING_template(STRING_PATTERN, expression_str, nocase).log();
    TTCN_Logger::log_event_str(" is: ");
    CHARSTRING(posix_str).log();
    TTCN_Logger::end_event();
  }
  
  regex_t posix_regexp;
  int ret_val = regcomp(&posix_regexp, posix_str, REG_EXTENDED);
  Free(posix_str);
  if (ret_val != 0) {
    char msg[ERRMSG_BUFSIZE];
    regerror(ret_val, &posix_regexp, msg, sizeof(msg));
    regfree(&posix_regexp);
    TTCN_error_begin("Internal error: Compilation of POSIX regular expression "
      "failed in function regexp() when trying to match with character "
      "pattern ");
    if (expression_val) expression_val->log(); else expression_tmpl->log();
    TTCN_Logger::log_event(". Error message: %s.", msg);
    TTCN_error_end();
  }

  int re_nsub = user_groups[0];

  if (posix_regexp.re_nsub <= 0) {
    regfree(&posix_regexp);
    TTCN_error_begin("The character pattern in the second argument "
      "(expression) of function regexp() does not contain any groups: ");
    if (expression_val) expression_val->log(); else expression_tmpl->log();
    TTCN_Logger::log_char('.');
    TTCN_error_end();
  }
  if (groupno >= re_nsub) {
    regfree(&posix_regexp);
    TTCN_error("The third argument (groupno) of function regexp() is too "
      "large: The requested group index is %d, but the pattern contains only "
      "%d group%s.", groupno, re_nsub, re_nsub > 1 ? "s" : "");
  }
  if (groupno > user_groups[0]) {
    printf("  user_groups: %d\n", user_groups[0]);
    TTCN_error("Error during parsing the pattern.");
  }
  size_t nmatch = (size_t)user_groups[groupno+1] + 1;
  regmatch_t *pmatch = (regmatch_t*)Malloc((nmatch + 1) * sizeof(*pmatch));

  Free(user_groups);

  char* instr_conv = instr.convert_to_regexp_form();
  int instr_len = instr.lengthof() * 8;
  
  if (nocase) {
    unichar_pattern.convert_regex_str_to_lowercase(instr_conv);
  }

  ret_val = regexec(&posix_regexp, instr_conv, nmatch + 1, pmatch, 0);
  Free(instr_conv);
  if (ret_val == 0) {
    int begin_index = pmatch[nmatch].rm_so, end_index = pmatch[nmatch].rm_eo;
    Free(pmatch);
    regfree(&posix_regexp);
    if (end_index > instr_len) TTCN_error("Internal error: The end index "
      "of the substring (%d) to be returned in function regexp() is greater "
      "than the length of the input string (%d).", end_index, instr_len);
    if (begin_index > end_index) TTCN_error("Internal error: The start index "
      "of the substring (%d) to be returned in function regexp() is greater "
      "than the end index (%d).", begin_index, end_index);
    return instr.extract_matched_section(begin_index, end_index);
  } else {
    Free(pmatch);
    if (ret_val != REG_NOMATCH) {
      char msg[ERRMSG_BUFSIZE];
      regerror(ret_val, &posix_regexp, msg, ERRMSG_BUFSIZE);
      regfree(&posix_regexp);
      TTCN_error("Internal error: POSIX regular expression matching returned "
        "unexpected status code in function regexp(): %s.", msg);
    } else regfree(&posix_regexp);
    return UNIVERSAL_CHARSTRING(0, (const char*)NULL);
  }
}

UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING& instr,
  const UNIVERSAL_CHARSTRING& expression, int groupno, boolean nocase)
{
  return regexp(instr, &expression, NULL, groupno, nocase);
}

UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING& instr,
  const UNIVERSAL_CHARSTRING& expression, const INTEGER& groupno, boolean nocase)
{
  groupno.must_bound("The third argument (groupno) of function regexp() is an "
    "unbound integer value.");
  return regexp(instr, expression, (int)groupno, nocase);
}

// regexp() on templates

CHARSTRING regexp(const CHARSTRING_template& instr,
  const CHARSTRING_template& expression, int groupno, boolean nocase)
{
  if (!instr.is_value())
    TTCN_error("The first argument of function regexp() is a "
      "template with non-specific value.");
  if (expression.is_value())
    return regexp(instr.valueof(), expression.valueof(), groupno, nocase);
  // pattern matching to specific value
  if (expression.get_selection()==STRING_PATTERN)
    return regexp(instr.valueof(), expression.get_single_value(), groupno, nocase);
  TTCN_error("The second argument of function regexp() should be "
    "specific value or pattern matching template.");
}

CHARSTRING regexp(const CHARSTRING_template& instr,
  const CHARSTRING_template& expression, const INTEGER& groupno, boolean nocase)
{
  groupno.must_bound("The third argument (groupno) of function regexp() is an "
    "unbound integer value.");
  return regexp(instr, expression, (int)groupno, nocase);
}

UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING_template& instr,
  const UNIVERSAL_CHARSTRING_template& expression, int groupno, boolean nocase)
{
  if (!instr.is_value())
    TTCN_error("The first argument of function regexp() is a "
      "template with non-specific value.");
  if (expression.is_value())
    return regexp(instr.valueof(), expression.valueof(), groupno, nocase);
  // pattern matching to specific value
  if (expression.get_selection()==STRING_PATTERN)
    return regexp(instr.valueof(), NULL, &expression, groupno, nocase);
  TTCN_error("The second argument of function regexp() should be "
    "specific value or pattern matching template.");
}

UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING_template& instr,
  const UNIVERSAL_CHARSTRING_template& expression, const INTEGER& groupno,
  boolean nocase)
{
  groupno.must_bound("The third argument (groupno) of function regexp() is an "
    "unbound integer value.");
  return regexp(instr, expression, (int)groupno, nocase);
}

// C.34 - substr

void check_substr_arguments(int value_length, int idx,
  int returncount, const char *string_type, const char *element_name)
{
  if (idx < 0) TTCN_error("The second argument (index) of function "
    "substr() is a negative integer value: %d.", idx);
  if (idx > value_length) TTCN_error("The second argument (index) of "
    "function substr(), which is %d, is greater than the length of the "
    "%s value: %d.", idx, string_type, value_length);
  if (returncount < 0) TTCN_error("The third argument (returncount) of "
    "function substr() is a negative integer value: %d.", returncount);
  if (idx + returncount > value_length) TTCN_error("The first argument of "
    "function substr(), the length of which is %d, does not have enough "
    "%ss starting at index %d: %d %s%s needed, but there %s only %d.",
    value_length, element_name, idx, returncount, element_name,
    returncount > 1 ? "s are" : " is",
      value_length - idx > 1 ? "are" : "is", value_length - idx);
}

static void check_substr_arguments(int idx, int returncount,
  const char *string_type, const char *element_name)
{
  if (idx < 0) TTCN_error("The second argument (index) of function "
    "substr() is a negative integer value: %d.", idx);
  if (idx > 1) TTCN_error("The second argument (index) of function "
    "substr(), which is %d, is greater than 1 (i.e. the length of the "
    "%s element).", idx, string_type);
  if (returncount < 0) TTCN_error("The third argument (returncount) of "
    "function substr() is a negative integer value: %d.", returncount);
  if (idx + returncount > 1) TTCN_error("The first argument of function "
    "substr(), which is a%s %s element, does not have enough %ss starting "
    "at index %d: %d %s%s needed, but there is only %d.",
    string_type[0] == 'o' ? "n" : "", string_type, element_name, idx,
      returncount, element_name, returncount > 1 ? "s are" : " is",
        1 - idx);
}

BITSTRING substr(const BITSTRING& value, int idx, int returncount)
{
  value.must_bound("The first argument (value) of function substr() "
    "is an unbound bitstring value.");
  check_substr_arguments(value.lengthof(), idx, returncount, "bitstring",
    "bit");
  if (idx % 8) {
    BITSTRING ret_val(returncount);
    for (int i = 0; i < returncount; i++) {
      ret_val.set_bit(i, value.get_bit(idx + i));
    }
    ret_val.clear_unused_bits();
    return ret_val;
  } else {
    return BITSTRING(returncount, (const unsigned char*)value + idx / 8);
  }
}

BITSTRING substr(const BITSTRING& value, int idx, const INTEGER& returncount)
{
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, idx, (int)returncount);
}

BITSTRING substr(const BITSTRING& value, const INTEGER& idx, int returncount)
{
  idx.must_bound("The second argument (index) of function substr() "
    "is an unbound integer value.");
  return substr(value, (int)idx, returncount);
}

BITSTRING substr(const BITSTRING& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  idx.must_bound("The second argument (index) of function substr() is an "
    "unbound integer value.");
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, (int)idx, (int)returncount);
}

BITSTRING substr(const BITSTRING_ELEMENT& value, int idx, int returncount)
{
  value.must_bound("The first argument (value) of function substr() "
    "is an unbound bitstring element.");
  check_substr_arguments(idx, returncount, "bitstring", "bit");
  if (returncount == 0) return BITSTRING(0, NULL);
  else {
    unsigned char bit = value.get_bit() ? 0x01 : 0x00;
    return BITSTRING(1, &bit);
  }
}

BITSTRING substr(const BITSTRING_ELEMENT& value, int idx,
  const INTEGER& returncount)
{
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, idx, (int)returncount);
}

BITSTRING substr(const BITSTRING_ELEMENT& value, const INTEGER& idx,
  int returncount)
{
  idx.must_bound("The second argument (index) of function substr() "
    "is an unbound integer value.");
  return substr(value, (int)idx, returncount);
}

BITSTRING substr(const BITSTRING_ELEMENT& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  idx.must_bound("The second argument (index) of function substr() is an "
    "unbound integer value.");
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, (int)idx, (int)returncount);
}

HEXSTRING substr(const HEXSTRING& value, int idx, int returncount)
{
  value.must_bound("The first argument (value) of function substr() "
    "is an unbound hexstring value.");
  check_substr_arguments(value.lengthof(), idx, returncount, "hexstring",
    "hexadecimal digit");
  const unsigned char *src_ptr = (const unsigned char*)value;
  if (idx % 2) {
    HEXSTRING ret_val(returncount);
    unsigned char *dst_ptr = ret_val.val_ptr->nibbles_ptr;
    for (int i = 0; i < returncount; i++) {
      if (i % 2) dst_ptr[i / 2] |= (src_ptr[(i + idx) / 2] & 0x0F) << 4;
      else dst_ptr[i / 2] = src_ptr[(i + idx) / 2] >> 4;
    }
    return ret_val;
  } else return HEXSTRING(returncount, src_ptr + idx / 2);
}

HEXSTRING substr(const HEXSTRING& value, int idx, const INTEGER& returncount)
{
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, idx, (int)returncount);
}

HEXSTRING substr(const HEXSTRING& value, const INTEGER& idx, int returncount)
{
  idx.must_bound("The second argument (index) of function substr() "
    "is an unbound integer value.");
  return substr(value, (int)idx, returncount);
}

HEXSTRING substr(const HEXSTRING& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  idx.must_bound("The second argument (index) of function substr() is an "
    "unbound integer value.");
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, (int)idx, (int)returncount);
}

HEXSTRING substr(const HEXSTRING_ELEMENT& value, int idx, int returncount)
{
  value.must_bound("The first argument (value) of function substr() "
    "is an unbound hexstring element.");
  check_substr_arguments(idx, returncount, "hexstring",
    "hexadecimal digit");
  if (returncount == 0) return HEXSTRING(0, NULL);
  else {
    unsigned char nibble = value.get_nibble();
    return HEXSTRING(1, &nibble);
  }
}

HEXSTRING substr(const HEXSTRING_ELEMENT& value, int idx,
  const INTEGER& returncount)
{
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, idx, (int)returncount);
}

HEXSTRING substr(const HEXSTRING_ELEMENT& value, const INTEGER& idx,
  int returncount)
{
  idx.must_bound("The second argument (index) of function substr() "
    "is an unbound integer value.");
  return substr(value, (int)idx, returncount);
}

HEXSTRING substr(const HEXSTRING_ELEMENT& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  idx.must_bound("The second argument (index) of function substr() is an "
    "unbound integer value.");
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, (int)idx, (int)returncount);
}

OCTETSTRING substr(const OCTETSTRING& value, int idx, int returncount)
{
  value.must_bound("The first argument (value) of function substr() "
    "is an unbound octetstring value.");
  check_substr_arguments(value.lengthof(), idx, returncount, "octetstring",
    "octet");
  return OCTETSTRING(returncount, (const unsigned char*)value + idx);
}

OCTETSTRING substr(const OCTETSTRING& value, int idx,
  const INTEGER& returncount)
{
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, idx, (int)returncount);
}

OCTETSTRING substr(const OCTETSTRING& value, const INTEGER& idx,
  int returncount)
{
  idx.must_bound("The second argument (index) of function substr() "
    "is an unbound integer value.");
  return substr(value, (int)idx, returncount);
}

OCTETSTRING substr(const OCTETSTRING& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  idx.must_bound("The second argument (index) of function substr() is an "
    "unbound integer value.");
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, (int)idx, (int)returncount);
}

OCTETSTRING substr(const OCTETSTRING_ELEMENT& value, int idx, int returncount)
{
  value.must_bound("The first argument (value) of function substr() "
    "is an unbound octetstring element.");
  check_substr_arguments(idx, returncount, "octetstring", "octet");
  if (returncount == 0) return OCTETSTRING(0, NULL);
  else {
    unsigned char octet = value.get_octet();
    return OCTETSTRING(1, &octet);
  }
}

OCTETSTRING substr(const OCTETSTRING_ELEMENT& value, int idx,
  const INTEGER& returncount)
{
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, idx, (int)returncount);
}

OCTETSTRING substr(const OCTETSTRING_ELEMENT& value, const INTEGER& idx,
  int returncount)
{
  idx.must_bound("The second argument (index) of function substr() "
    "is an unbound integer value.");
  return substr(value, (int)idx, returncount);
}

OCTETSTRING substr(const OCTETSTRING_ELEMENT& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  idx.must_bound("The second argument (index) of function substr() is an "
    "unbound integer value.");
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, (int)idx, (int)returncount);
}

CHARSTRING substr(const CHARSTRING& value, int idx, int returncount)
{
  value.must_bound("The first argument (value) of function substr() "
    "is an unbound charstring value.");
  check_substr_arguments(value.lengthof(), idx, returncount, "charstring",
    "character");
  return CHARSTRING(returncount, (const char*)value + idx);
}

CHARSTRING substr(const CHARSTRING& value, int idx,
  const INTEGER& returncount)
{
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, idx, (int)returncount);
}

CHARSTRING substr(const CHARSTRING& value, const INTEGER& idx,
  int returncount)
{
  idx.must_bound("The second argument (index) of function substr() "
    "is an unbound integer value.");
  return substr(value, (int)idx, returncount);
}

CHARSTRING substr(const CHARSTRING& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  idx.must_bound("The second argument (index) of function substr() is an "
    "unbound integer value.");
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, (int)idx, (int)returncount);
}

CHARSTRING substr(const CHARSTRING_ELEMENT& value, int idx, int returncount)
{
  value.must_bound("The first argument (value) of function substr() "
    "is an unbound charstring element.");
  check_substr_arguments(idx, returncount, "charstring", "character");
  if (returncount == 0) return CHARSTRING(0, NULL);
  else return CHARSTRING(value.get_char());
}

CHARSTRING substr(const CHARSTRING_ELEMENT& value, int idx,
  const INTEGER& returncount)
{
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, idx, (int)returncount);
}

CHARSTRING substr(const CHARSTRING_ELEMENT& value, const INTEGER& idx,
  int returncount)
{
  idx.must_bound("The second argument (index) of function substr() "
    "is an unbound integer value.");
  return substr(value, (int)idx, returncount);
}

CHARSTRING substr(const CHARSTRING_ELEMENT& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  idx.must_bound("The second argument (index) of function substr() is an "
    "unbound integer value.");
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, (int)idx, (int)returncount);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING& value, int idx,
  int returncount)
{
  value.must_bound("The first argument (value) of function substr() "
    "is an unbound universal charstring value.");
  check_substr_arguments(value.lengthof(), idx, returncount,
    "universal charstring", "character");
  return UNIVERSAL_CHARSTRING(returncount,
    (const universal_char*)value + idx);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING& value, int idx,
  const INTEGER& returncount)
{
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, idx, (int)returncount);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING& value,
  const INTEGER& idx, int returncount)
{
  idx.must_bound("The second argument (index) of function substr() "
    "is an unbound integer value.");
  return substr(value, (int)idx, returncount);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING& value,
  const INTEGER& idx, const INTEGER& returncount)
{
  idx.must_bound("The second argument (index) of function substr() is an "
    "unbound integer value.");
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, (int)idx, (int)returncount);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_ELEMENT& value,
  int idx, int returncount)
{
  value.must_bound("The first argument (value) of function substr() "
    "is an unbound universal charstring element.");
  check_substr_arguments(idx, returncount, "universal charstring",
    "character");
  if (returncount == 0)
    return UNIVERSAL_CHARSTRING(0, (const universal_char*)NULL);
  else return UNIVERSAL_CHARSTRING(value.get_uchar());
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_ELEMENT& value,
  int idx, const INTEGER& returncount)
{
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, idx, (int)returncount);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_ELEMENT& value,
  const INTEGER& idx, int returncount)
{
  idx.must_bound("The second argument (index) of function substr() "
    "is an unbound integer value.");
  return substr(value, (int)idx, returncount);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_ELEMENT& value,
  const INTEGER& idx, const INTEGER& returncount)
{
  idx.must_bound("The second argument (index) of function substr() is an "
    "unbound integer value.");
  returncount.must_bound("The third argument (returncount) of function "
    "substr() is an unbound integer value.");
  return substr(value, (int)idx, (int)returncount);
}

// substr() on templates
BITSTRING substr(const BITSTRING_template& value, int idx, int returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

BITSTRING substr(const BITSTRING_template& value, int idx, const INTEGER& returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

BITSTRING substr(const BITSTRING_template& value, const INTEGER& idx, int returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

BITSTRING substr(const BITSTRING_template& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

HEXSTRING substr(const HEXSTRING_template& value, int idx, int returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

HEXSTRING substr(const HEXSTRING_template& value, int idx, const INTEGER& returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

HEXSTRING substr(const HEXSTRING_template& value, const INTEGER& idx, int returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

HEXSTRING substr(const HEXSTRING_template& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

OCTETSTRING substr(const OCTETSTRING_template& value, int idx, int returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

OCTETSTRING substr(const OCTETSTRING_template& value, int idx,
  const INTEGER& returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

OCTETSTRING substr(const OCTETSTRING_template& value, const INTEGER& idx,
  int returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

OCTETSTRING substr(const OCTETSTRING_template& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

CHARSTRING substr(const CHARSTRING_template& value, int idx, int returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

CHARSTRING substr(const CHARSTRING_template& value, int idx,
  const INTEGER& returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

CHARSTRING substr(const CHARSTRING_template& value, const INTEGER& idx,
  int returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

CHARSTRING substr(const CHARSTRING_template& value, const INTEGER& idx,
  const INTEGER& returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_template& value, int idx,
  int returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_template& value, int idx,
  const INTEGER& returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_template& value,
  const INTEGER& idx, int returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_template& value,
  const INTEGER& idx, const INTEGER& returncount)
{
  if (!value.is_value()) TTCN_error("The first argument of function substr() is a template with non-specific value.");
  return substr(value.valueof(), idx, returncount);
}

// C.35 - replace

void check_replace_arguments(int value_length, int idx, int len,
  const char *string_type, const char *element_name)
{
  if (idx < 0) TTCN_error("The second argument (index) of function "
    "replace() is a negative integer value: %d.", idx);
  if (idx > value_length) TTCN_error("The second argument (index) of "
    "function replace(), which is %d, is greater than the length of the "
    "%s value: %d.", idx, string_type, value_length);
  if (len < 0) TTCN_error("The third argument (len) of function replace() "
    "is a negative integer value: %d.", len);
  if (len > value_length) TTCN_error("The third argument (len) of function "
    "replace(), which is %d, is greater than the length of the "
    "%s value: %d.", len, string_type, value_length);
  if (idx + len > value_length) TTCN_error("The first argument of "
    "function replace(), the length of which is %d, does not have enough "
    "%ss starting at index %d: %d %s%s needed, but there %s only %d.",
    value_length, element_name, idx, len, element_name,
    len > 1 ? "s are" : " is", value_length - idx > 1 ? "are" : "is",
      value_length - idx);
}

BITSTRING replace(const BITSTRING& value, int idx, int len,
  const BITSTRING& repl)
{
  value.must_bound("The first argument (value) of function replace() "
    "is an unbound bitstring value.");
  repl.must_bound("The fourth argument (repl) of function replace() is an "
    "unbound bitstring value.");
  check_replace_arguments(value.lengthof(), idx, len, "bitstring", "bit");
  int value_len = value.lengthof();
  int repl_len = repl.lengthof();
  BITSTRING ret_val(value_len + repl_len - len);
  for (int i = 0; i < idx; i++) ret_val.set_bit(i, value.get_bit(i));
  for (int i = 0; i < repl_len; i++)
    ret_val.set_bit(i + idx, repl.get_bit(i));
  for (int i = 0; i < value_len - idx - len; i++)
    ret_val.set_bit(i + idx + repl_len, value.get_bit(idx + len + i));
  return ret_val;
}

BITSTRING replace(const BITSTRING& value, int idx, const INTEGER& len,
  const BITSTRING& repl)
{
  len.must_bound("The third argument (len) of function replace() is an "
    "unbound integer value.");
  return replace(value, idx, (int)len, repl);
}

BITSTRING replace(const BITSTRING& value, const INTEGER& idx, int len,
  const BITSTRING& repl)
{
  idx.must_bound("The second argument (index) of function replace() is an "
    "unbound integer value.");
  return replace(value, (int)idx, len, repl);
}

BITSTRING replace(const BITSTRING& value, const INTEGER& idx,
  const INTEGER& len, const BITSTRING& repl)
{
  idx.must_bound("The second argument (index) of function replace() is an "
    "unbound integer value.");
  len.must_bound("The third argument (len) of function replace() is an "
    "unbound integer value.");
  return replace(value, (int)idx, (int)len, repl);
}

HEXSTRING replace(const HEXSTRING& value, int idx, int len,
  const HEXSTRING& repl)
{
  value.must_bound("The first argument (value) of function replace() "
    "is an unbound hexstring value.");
  repl.must_bound("The fourth argument (repl) of function replace() is an "
    "unbound hexstring value.");
  check_replace_arguments(value.lengthof(), idx, len, "hexstring",
    "hexadecimal digit");
  int value_len = value.lengthof();
  int repl_len = repl.lengthof();
  HEXSTRING ret_val(value_len + repl_len - len);
  for (int i = 0; i < idx; i++)
    ret_val.set_nibble(i, value.get_nibble(i));
  for (int i = 0; i < repl_len; i++)
    ret_val.set_nibble(idx + i, repl.get_nibble(i));
  for (int i = 0; i < value_len - idx - len; i++)
    ret_val.set_nibble(idx + i + repl_len,
      value.get_nibble(idx + i + len));
  return ret_val;
}

HEXSTRING replace(const HEXSTRING& value, int idx, const INTEGER& len,
  const HEXSTRING& repl)
{
  len.must_bound("The third argument (len) of function replace() is an "
    "unbound integer value.");
  return replace(value, idx, (int)len, repl);
}

HEXSTRING replace(const HEXSTRING& value, const INTEGER& idx, int len,
  const HEXSTRING& repl)
{
  idx.must_bound("The second argument (index) of function replace() is an "
    "unbound integer value.");
  return replace(value, (int)idx, len, repl);
}

HEXSTRING replace(const HEXSTRING& value, const INTEGER& idx,
  const INTEGER& len, const HEXSTRING& repl)
{
  idx.must_bound("The second argument (index) of function replace() is an "
    "unbound integer value.");
  len.must_bound("The third argument (len) of function replace() is an "
    "unbound integer value.");
  return replace(value, (int)idx, (int)len, repl);
}

OCTETSTRING replace(const OCTETSTRING& value, int idx, int len,
  const OCTETSTRING& repl)
{
  value.must_bound("The first argument (value) of function replace() "
    "is an unbound octetstring value.");
  repl.must_bound("The fourth argument (repl) of function replace() is an "
    "unbound octetstring value.");
  check_replace_arguments(value.lengthof(), idx, len, "octetstring",
    "octet");
  int value_len = value.lengthof();
  int repl_len = repl.lengthof();
  OCTETSTRING ret_val(value_len + repl_len - len);
  memcpy(ret_val.val_ptr->octets_ptr, value.val_ptr->octets_ptr, idx);
  memcpy(ret_val.val_ptr->octets_ptr + idx, repl.val_ptr->octets_ptr,
    repl_len);
  memcpy(ret_val.val_ptr->octets_ptr + idx + repl_len,
    value.val_ptr->octets_ptr + idx + len, value_len - idx - len);
  return ret_val;
}

OCTETSTRING replace(const OCTETSTRING& value, int idx, const INTEGER& len,
  const OCTETSTRING& repl)
{
  len.must_bound("The third argument (len) of function replace() is an "
    "unbound integer value.");
  return replace(value, idx, (int)len, repl);
}

OCTETSTRING replace(const OCTETSTRING& value, const INTEGER& idx, int len,
  const OCTETSTRING& repl)
{
  idx.must_bound("The second argument (index) of function replace() is an "
    "unbound integer value.");
  return replace(value, (int)idx, len, repl);
}

OCTETSTRING replace(const OCTETSTRING& value, const INTEGER& idx,
  const INTEGER& len, const OCTETSTRING& repl)
{
  idx.must_bound("The second argument (index) of function replace() is an "
    "unbound integer value.");
  len.must_bound("The third argument (len) of function replace() is an "
    "unbound integer value.");
  return replace(value, (int)idx, (int)len, repl);
}

CHARSTRING replace(const CHARSTRING& value, int idx, int len,
  const CHARSTRING& repl)
{
  value.must_bound("The first argument (value) of function replace() "
    "is an unbound charstring value.");
  repl.must_bound("The fourth argument (repl) of function replace() is an "
    "unbound charstring value.");
  check_replace_arguments(value.lengthof(), idx, len, "charstring",
    "character");
  int value_len = value.lengthof();
  int repl_len = repl.lengthof();
  CHARSTRING ret_val(value_len + repl_len - len);
  /* According to http://gcc.gnu.org/ml/fortran/2007-05/msg00341.html it's
       worth using memcpy() instead of strncat().  There's no need to scan the
       strings for '\0'.  */
  memcpy(ret_val.val_ptr->chars_ptr, value.val_ptr->chars_ptr, idx);
  memcpy(ret_val.val_ptr->chars_ptr + idx, repl.val_ptr->chars_ptr,
    repl_len);
  memcpy(ret_val.val_ptr->chars_ptr + idx + repl_len,
    value.val_ptr->chars_ptr + idx + len, value_len - idx - len);
  return ret_val;
}

CHARSTRING replace(const CHARSTRING& value, int idx, const INTEGER& len,
  const CHARSTRING& repl)
{
  len.must_bound("The third argument (len) of function replace() is an "
    "unbound integer value.");
  return replace(value, idx, (int)len, repl);
}

CHARSTRING replace(const CHARSTRING& value, const INTEGER& idx, int len,
  const CHARSTRING& repl)
{
  idx.must_bound("The second argument (index) of function replace() is an "
    "unbound integer value.");
  return replace(value, (int)idx, len, repl);
}

CHARSTRING replace(const CHARSTRING& value, const INTEGER& idx,
  const INTEGER& len, const CHARSTRING& repl)
{
  idx.must_bound("The second argument (index) of function replace() is an "
    "unbound integer value.");
  len.must_bound("The third argument (len) of function replace() is an "
    "unbound integer value.");
  return replace(value, (int)idx, (int)len, repl);
}

UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING& value, int idx,
  int len, const UNIVERSAL_CHARSTRING& repl)
{
  value.must_bound("The first argument (value) of function replace() "
    "is an unbound universal charstring value.");
  repl.must_bound("The fourth argument (repl) of function replace() is an "
    "unbound universal charstring value.");
  check_replace_arguments(value.lengthof(), idx, len,
    "universal charstring", "character");
  int value_len = value.lengthof();
  int repl_len = repl.lengthof();
  UNIVERSAL_CHARSTRING ret_val(value_len + repl_len - len);
  memcpy(ret_val.val_ptr->uchars_ptr, value.val_ptr->uchars_ptr,
    idx * sizeof(universal_char));
  memcpy(ret_val.val_ptr->uchars_ptr + idx, repl.val_ptr->uchars_ptr,
    repl_len * sizeof(universal_char));
  memcpy(ret_val.val_ptr->uchars_ptr + idx + repl_len,
    value.val_ptr->uchars_ptr + idx + len,
    (value_len - idx - len) * sizeof(universal_char));
  return ret_val;
}

UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING& value, int idx,
  const INTEGER& len, const UNIVERSAL_CHARSTRING& repl)
{
  len.must_bound("The third argument (len) of function replace() is an "
    "unbound integer value.");
  return replace(value, idx, (int)len, repl);
}

UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING& value,
  const INTEGER& idx, int len, const UNIVERSAL_CHARSTRING& repl)
{
  idx.must_bound("The second argument (index) of function replace() is an "
    "unbound integer value.");
  return replace(value, (int)idx, len, repl);
}

UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING& value,
  const INTEGER& idx, const INTEGER& len,
  const UNIVERSAL_CHARSTRING& repl)
{
  idx.must_bound("The second argument (index) of function replace() is an "
    "unbound integer value.");
  len.must_bound("The third argument (len) of function replace() is an "
    "unbound integer value.");
  return replace(value, (int)idx, (int)len, repl);
}

// replace on templates

BITSTRING replace(const BITSTRING_template& value, int idx, int len,
  const BITSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

BITSTRING replace(const BITSTRING_template& value, int idx, const INTEGER& len,
  const BITSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

BITSTRING replace(const BITSTRING_template& value, const INTEGER& idx, int len,
  const BITSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

BITSTRING replace(const BITSTRING_template& value, const INTEGER& idx,
  const INTEGER& len, const BITSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

HEXSTRING replace(const HEXSTRING_template& value, int idx, int len,
  const HEXSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

HEXSTRING replace(const HEXSTRING_template& value, int idx, const INTEGER& len,
  const HEXSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

HEXSTRING replace(const HEXSTRING_template& value, const INTEGER& idx, int len,
  const HEXSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

HEXSTRING replace(const HEXSTRING_template& value, const INTEGER& idx,
  const INTEGER& len, const HEXSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

OCTETSTRING replace(const OCTETSTRING_template& value, int idx, int len,
  const OCTETSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

OCTETSTRING replace(const OCTETSTRING_template& value, int idx, const INTEGER& len,
  const OCTETSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

OCTETSTRING replace(const OCTETSTRING_template& value, const INTEGER& idx, int len,
  const OCTETSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

OCTETSTRING replace(const OCTETSTRING_template& value, const INTEGER& idx,
  const INTEGER& len, const OCTETSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

CHARSTRING replace(const CHARSTRING_template& value, int idx, int len,
  const CHARSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

CHARSTRING replace(const CHARSTRING_template& value, int idx, const INTEGER& len,
  const CHARSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

CHARSTRING replace(const CHARSTRING_template& value, const INTEGER& idx, int len,
  const CHARSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

CHARSTRING replace(const CHARSTRING_template& value, const INTEGER& idx,
  const INTEGER& len, const CHARSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING_template& value, int idx, int len,
  const UNIVERSAL_CHARSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING_template& value, int idx, const INTEGER& len,
  const UNIVERSAL_CHARSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING_template& value, const INTEGER& idx, int len,
  const UNIVERSAL_CHARSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING_template& value, const INTEGER& idx,
  const INTEGER& len, const UNIVERSAL_CHARSTRING_template& repl)
{
  if (!value.is_value()) TTCN_error("The first argument of function replace() is a template with non-specific value.");
  if (!repl.is_value()) TTCN_error("The fourth argument of function replace() is a template with non-specific value.");
  return replace(value.valueof(), idx, len, repl.valueof());
}

// C.36 - rnd

static boolean rnd_seed_set = FALSE;

static void set_rnd_seed(double float_seed)
{
  FLOAT::check_numeric(float_seed, "The seed value of function rnd()");
  long integer_seed = 0;
  const unsigned char *src_ptr = (const unsigned char*)&float_seed;
  unsigned char *dst_ptr = (unsigned char*)&integer_seed;
  for (size_t i = 0; i < sizeof(float_seed); i++) {
    dst_ptr[i % sizeof(integer_seed)] ^= bit_reverse_table[src_ptr[i]];
    dst_ptr[(sizeof(float_seed) - 1 - i) % sizeof(integer_seed)] ^=
      src_ptr[i];
  }

  srand48(integer_seed);
  TTCN_Logger::log_random(TitanLoggerApi::RandomAction::seed, float_seed, integer_seed);
  rnd_seed_set = TRUE;
}

static double rnd_generate()
{
  double ret_val;
  ret_val = drand48();
  TTCN_Logger::log_random(TitanLoggerApi::RandomAction::read__out, ret_val, 0);
  return ret_val;
}

double rnd()
{
  if (!rnd_seed_set) set_rnd_seed(TTCN_Snapshot::time_now());
  return rnd_generate();
}

double rnd(double seed)
{
  set_rnd_seed(seed);
  return rnd_generate();
}

double rnd(const FLOAT& seed)
{
  seed.must_bound("Initializing the random number generator with "
    "an unbound float value as seed.");
  set_rnd_seed((double)seed);
  return rnd_generate();
}


// Additional predefined functions defined in Annex B of ES 101 873-7

// B.1 decomp - not implemented yet


// Non-standard functions

// str2bit

BITSTRING str2bit(const char *value)
{
  if (value == NULL) return BITSTRING(0, NULL);
  else return str2bit(CHARSTRING(value));
}

BITSTRING str2bit(const CHARSTRING& value)
{
  value.must_bound("The argument of function str2bit() is an unbound "
    "charstring value.");
  int value_length = value.lengthof();
  const char *chars_ptr = value;
  BITSTRING ret_val(value_length);
  for (int i = 0; i < value_length; i++) {
    char c = chars_ptr[i];
    switch (c) {
    case '0':
      ret_val.set_bit(i, FALSE);
      break;
    case '1':
      ret_val.set_bit(i, TRUE);
      break;
    default:
      TTCN_error_begin("The argument of function str2bit() shall "
        "contain characters `0' and `1' only, but character `");
      TTCN_Logger::log_char_escaped(c);
      TTCN_Logger::log_event("' was found at index %d.", i);
      TTCN_error_end();
    }
  }
  ret_val.clear_unused_bits();
  return ret_val;
}

BITSTRING str2bit(const CHARSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function str2bit() is an unbound "
    "charstring element.");
  char c = value.get_char();
  if (c != '0' && c != '1') {
    TTCN_error_begin("The argument of function str2bit() shall contain "
      "characters `0' and `1' only, but the given charstring element "
      "contains the character `");
    TTCN_Logger::log_char_escaped(c);
    TTCN_Logger::log_event_str("'.");
    TTCN_error_end();
  }
  unsigned char bit = c == '1' ? 0x01 : 0x00;
  return BITSTRING(1, &bit);
}

// str2hex

HEXSTRING str2hex(const char *value)
{
  if (value == NULL) return HEXSTRING(0, NULL);
  else return str2hex(CHARSTRING(value));
}

HEXSTRING str2hex(const CHARSTRING& value)
{
  value.must_bound("The argument of function str2hex() is an unbound "
    "charstring value.");
  int value_length = value.lengthof();
  const char *chars_ptr = value;
  HEXSTRING ret_val(value_length);
  unsigned char *nibbles_ptr = ret_val.val_ptr->nibbles_ptr;
  for (int i = 0; i < value_length; i++) {
    char c = chars_ptr[i];
    unsigned char hex_digit = char_to_hexdigit(c);
    if (hex_digit > 0x0F) {
      TTCN_error_begin("The argument of function str2hex() shall "
        "contain hexadecimal digits only, but character `");
      TTCN_Logger::log_char_escaped(c);
      TTCN_Logger::log_event("' was found at index %d.", i);
      TTCN_error_end();
    }
    if (i % 2) nibbles_ptr[i / 2] |= hex_digit << 4;
    else nibbles_ptr[i / 2] = hex_digit;
  }
  return ret_val;
}

HEXSTRING str2hex(const CHARSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function str2hex() is an unbound "
    "charstring element.");
  char c = value.get_char();
  unsigned char hex_digit = char_to_hexdigit(c);
  if (hex_digit > 0x0F) {
    TTCN_error_begin("The argument of function str2hex() shall contain "
      "only hexadecimal digits, but the given charstring element "
      "contains the character `");
    TTCN_Logger::log_char_escaped(c);
    TTCN_Logger::log_event_str("'.");
    TTCN_error_end();
  }
  return HEXSTRING(1, &hex_digit);
}

// float2str

CHARSTRING float2str(double value)
{
  boolean f = value == 0.0
    || (value > -MAX_DECIMAL_FLOAT && value <= -MIN_DECIMAL_FLOAT)
    || (value >= MIN_DECIMAL_FLOAT && value < MAX_DECIMAL_FLOAT);
  // true if decimal representation possible (use %f format)
  char str_buf[64];
  int str_len = snprintf(str_buf, sizeof(str_buf), f ? "%f" : "%e", value);
  if (str_len < 0 || str_len >= (int)sizeof(str_buf)) {
    TTCN_error("Internal error: system call snprintf() returned "
      "unexpected status code %d when converting value %g in function "
      "float2str().", str_len, value);
  }
  return CHARSTRING(str_len, str_buf);
}

CHARSTRING float2str(const FLOAT& value)
{
  value.must_bound("The argument of function float2str() is an unbound "
    "float value.");
  return float2str((double)value);
}

// unichar2char

CHARSTRING unichar2char(const UNIVERSAL_CHARSTRING& value)
{
  value.must_bound("The argument of function unichar2char() is an unbound "
    "universal charstring value.");
  int value_length = value.lengthof();
  const universal_char *uchars_ptr = value;
  CHARSTRING ret_val(value_length);
  char *chars_ptr = ret_val.val_ptr->chars_ptr;
  for (int i = 0; i < value_length; i++) {
    const universal_char& uchar = uchars_ptr[i];
    if (uchar.uc_group != 0 || uchar.uc_plane != 0 || uchar.uc_row != 0 ||
      uchar.uc_cell > 127) {
      TTCN_error("The characters in the argument of function "
        "unichar2char() shall be within the range char(0, 0, 0, 0) .. "
        "char(0, 0, 0, 127), but quadruple char(%u, %u, %u, %u) was "
        "found at index %d.", uchar.uc_group, uchar.uc_plane,
        uchar.uc_row, uchar.uc_cell, i);
    }
    chars_ptr[i] = uchar.uc_cell;
  }
  return ret_val;
}

CHARSTRING unichar2char(const UNIVERSAL_CHARSTRING_ELEMENT& value)
{
  value.must_bound("The argument of function unichar2char() is an unbound "
    "universal charstring element.");
  const universal_char& uchar = value.get_uchar();
  if (uchar.uc_group != 0 || uchar.uc_plane != 0 || uchar.uc_row != 0 ||
    uchar.uc_cell > 127) {
    TTCN_error("The characters in the argument of function unichar2char() "
      "shall be within the range char(0, 0, 0, 0) .. char(0, 0, 0, 127), "
      "but the given universal charstring element contains the quadruple "
      "char(%u, %u, %u, %u).", uchar.uc_group, uchar.uc_plane,
      uchar.uc_row, uchar.uc_cell);
  }
  return CHARSTRING((char)uchar.uc_cell);
}

OCTETSTRING unichar2oct(const UNIVERSAL_CHARSTRING& invalue, const CHARSTRING& string_encoding)
{
  invalue.must_bound("The argument of function unichar2oct() is an unbound "
    "universal charstring value.");
  TTCN_EncDec::error_behavior_t err_behavior = TTCN_EncDec::get_error_behavior(TTCN_EncDec::ET_DEC_UCSTR);
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_DEC_UCSTR, TTCN_EncDec::EB_ERROR);
  TTCN_Buffer buf;
  if ("UTF-8" == string_encoding) {
    invalue.encode_utf8(buf, FALSE);
  }
  else if ("UTF-8 BOM" == string_encoding) {
    invalue.encode_utf8(buf, TRUE);
  }
  else if ("UTF-16" == string_encoding) {
    invalue.encode_utf16(buf, CharCoding::UTF16);
  }
  else if ("UTF-16BE" == string_encoding) {
    invalue.encode_utf16(buf, CharCoding::UTF16BE);
  }
  else if ("UTF-16LE" == string_encoding) {
    invalue.encode_utf16(buf, CharCoding::UTF16LE);
  }
  else if ("UTF-32" == string_encoding) {
    invalue.encode_utf32(buf, CharCoding::UTF32);
  }
  else if ("UTF-32BE" == string_encoding) {
    invalue.encode_utf32(buf, CharCoding::UTF32BE);
  }  else if ("UTF-32LE" == string_encoding) {
  invalue.encode_utf32(buf, CharCoding::UTF32LE);
  }
  else {
    TTCN_error("unichar2oct: Invalid parameter: %s", (const char*)string_encoding);
  }
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_DEC_UCSTR, err_behavior);
  return OCTETSTRING (buf.get_len(), buf.get_data());
}

OCTETSTRING unichar2oct(const UNIVERSAL_CHARSTRING& invalue)
{ 
// no encoding parameter is default UTF-8
  invalue.must_bound("The argument of function unichar2oct() is an unbound "
    "universal charstring value.");
  TTCN_EncDec::error_behavior_t err_behavior = TTCN_EncDec::get_error_behavior(TTCN_EncDec::ET_DEC_UCSTR);
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_DEC_UCSTR, TTCN_EncDec::EB_ERROR);
  TTCN_Buffer buf;
  invalue.encode_utf8(buf, FALSE);
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_DEC_UCSTR, err_behavior);
  return OCTETSTRING (buf.get_len(), buf.get_data());
}

CHARSTRING get_stringencoding(const OCTETSTRING& encoded_value)
{
  if (!encoded_value.lengthof()) return CHARSTRING("<unknown>");
  unsigned int i, j, length = encoded_value.lengthof();
  const unsigned char* strptr = (const unsigned char*)encoded_value;
  for (i = 0, j = 0; UTF8_BOM[i++] == strptr[j++] && i < sizeof (UTF8_BOM););
  if (i == sizeof (UTF8_BOM) && sizeof(UTF8_BOM) <= length) return "UTF-8";
  //UTF-32 shall be tested before UTF-16 !!!
  for (i = 0, j = 0; UTF32BE_BOM[i++] == strptr[j++] && i < sizeof (UTF32BE_BOM););
  if (i == sizeof (UTF32BE_BOM) && sizeof (UTF32BE_BOM) <= length ) return "UTF-32BE";
  for (i = 0, j = 0; UTF32LE_BOM[i++] == strptr[j++] && i < sizeof (UTF32LE_BOM););
  if (i == sizeof (UTF32LE_BOM) && sizeof (UTF32LE_BOM) <= length) return "UTF-32LE";
  //UTF-32 shall be tested before UTF-16 !!!
  for (i = 0, j = 0; UTF16BE_BOM[i++] == strptr[j++] && i < sizeof (UTF16BE_BOM););
  if (i == sizeof (UTF16BE_BOM) && sizeof (UTF16BE_BOM) <= length) return "UTF-16BE";
  for (i = 0, j = 0; UTF16LE_BOM[i++] == strptr[j++] && i < sizeof (UTF16LE_BOM););
  if (i == sizeof (UTF16LE_BOM) && sizeof (UTF16LE_BOM) <= length) return "UTF-16LE";
  if (is_ascii (encoded_value) == CharCoding::ASCII) {
    return "ASCII";
  }  
  else if (CharCoding::UTF_8 == is_utf8 (encoded_value)) {
    return "UTF-8";
  }  else {
  return "<unknown>";
  }
}

// for internal purposes:
// to obtain the name of the port when port array references are used
// in connect/map/disconnect/unmap operations

CHARSTRING get_port_name(const char *port_name, int array_index)
{
  char *result_str = mprintf("%s[%d]", port_name, array_index);
  CHARSTRING ret_val(mstrlen(result_str), result_str);
  Free(result_str);
  return ret_val;
}

CHARSTRING get_port_name(const char *port_name, const INTEGER& array_index)
{
  array_index.must_bound("Using an unbound integer value for indexing an "
    "array of ports.");
  return get_port_name(port_name, (int)array_index);
}

CHARSTRING get_port_name(const CHARSTRING& port_name, int array_index)
{
  port_name.must_bound("Internal error: Using an unbound charstring value "
    "to obtain the name of a port.");
  return get_port_name((const char*)port_name, array_index);
}

CHARSTRING get_port_name(const CHARSTRING& port_name,
  const INTEGER& array_index)
{
  port_name.must_bound("Internal error: Using an unbound charstring value "
    "to obtain the name of a port.");
  array_index.must_bound("Using an unbound integer value for indexing an "
    "array of ports.");
  return get_port_name((const char*)port_name, (int)array_index);
}

OCTETSTRING remove_bom(const OCTETSTRING& encoded_value)
{
  const unsigned char* str = (const unsigned char*)encoded_value;
  int length_of_BOM = 0;
  if (0x00 == str[0] && 0x00 == str[1] && 0xFE == str[2] && 0xFF == str[3]) { // UTF-32BE
    length_of_BOM = 4;
  }
  else if (0xFF == str[0] && 0xFE == str[1] && 0x00 == str[2] && 0x00 == str[3]) { // UTF-32LE
    length_of_BOM = 4;
  }
  else if (0xFE == str[0] && 0xFF == str[1]) { // UTF-16BE
    length_of_BOM = 2;
  }
  else if (0xFF == str[0] && 0xFE == str[1]) { // UTF-16LE
    length_of_BOM = 2;
  }
  else if (0xEF == str[0] && 0xBB == str[1] && 0xBF == str[2]) { // UTF-8
    length_of_BOM = 3;
  }
  else {
    return OCTETSTRING (encoded_value); // no BOM found
  }
  return OCTETSTRING (encoded_value.lengthof() - length_of_BOM, (str + length_of_BOM));
}

CHARSTRING encode_base64(const OCTETSTRING& msg)
{
  const char *code_table = {
    "ABCDEFGHIJKLMNOP"
    "QRSTUVWXYZabcdef"
    "ghijklmnopqrstuv"
    "wxyz0123456789+/"
  };
  const char pad = '=';
  const unsigned char *p_msg = (const unsigned char *)msg;
  int octets_left = msg.lengthof();
  char *output = (char*)Malloc(((octets_left*22)>>4) + 7);
  char *p_output = output;
  while(octets_left >= 3) {
    *p_output++ = code_table[p_msg[0] >> 2];
    *p_output++ = code_table[((p_msg[0] << 4) | (p_msg[1] >> 4)) & 0x3f];
    *p_output++ = code_table[((p_msg[1] << 2) | (p_msg[2] >> 6)) & 0x3f];
    *p_output++ = code_table[p_msg[2] & 0x3f];
    p_msg += 3;
    octets_left -= 3;
  }
  switch(octets_left) {
  case 1:
    *p_output++ = code_table[p_msg[0] >> 2];
    *p_output++ = code_table[(p_msg[0] << 4) & 0x3f];
    *p_output++ = pad;
    *p_output++ = pad;
    break;
  case 2:
    *p_output++ = code_table[p_msg[0] >> 2];
    *p_output++ = code_table[((p_msg[0] << 4) | (p_msg[1] >> 4)) & 0x3f];
    *p_output++ = code_table[(p_msg[1] << 2) & 0x3f];
    *p_output++ = pad;
    break;
  default:
    break;
  }
  *p_output = '\0';
  CHARSTRING ret_val(output);
  Free(output);
  return ret_val;
}

CHARSTRING encode_base64(const OCTETSTRING& msg, boolean use_linebreaks)
{
  const char *code_table = {
    "ABCDEFGHIJKLMNOP"
    "QRSTUVWXYZabcdef"
    "ghijklmnopqrstuv"
    "wxyz0123456789+/"
  };
  const char pad = '=';
  const unsigned char *p_msg = (const unsigned char *)msg;
  int octets_left = msg.lengthof();
  char *output = (char*)Malloc(((octets_left*22)>>4) + 7);
  char *p_output = output;
  int n_4chars = 0;
  while(octets_left >= 3) {
    *p_output++ = code_table[p_msg[0] >> 2];
    *p_output++ = code_table[((p_msg[0] << 4) | (p_msg[1] >> 4)) & 0x3f];
    *p_output++ = code_table[((p_msg[1] << 2) | (p_msg[2] >> 6)) & 0x3f];
    *p_output++ = code_table[p_msg[2] & 0x3f];
    n_4chars++;
    if (use_linebreaks && n_4chars>=19 && octets_left != 3) {
      *p_output++ = '\r';
      *p_output++ = '\n';
      n_4chars = 0;
    }
    p_msg += 3;
    octets_left -= 3;
  }
  switch(octets_left) {
  case 1:
    *p_output++ = code_table[p_msg[0] >> 2];
    *p_output++ = code_table[(p_msg[0] << 4) & 0x3f];
    *p_output++ = pad;
    *p_output++ = pad;
    break;
  case 2:
    *p_output++ = code_table[p_msg[0] >> 2];
    *p_output++ = code_table[((p_msg[0] << 4) | (p_msg[1] >> 4)) & 0x3f];
    *p_output++ = code_table[(p_msg[1] << 2) & 0x3f];
    *p_output++ = pad;
    break;
  default:
    break;
  }
  *p_output = '\0';
  CHARSTRING ret_val(output);
  Free(output);
  return ret_val;
}


OCTETSTRING decode_base64(const CHARSTRING& b64)
{
  const unsigned char decode_table[] = {
    80, 80, 80, 80, 80, 80, 80, 80,   80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80,   80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80,   80, 80, 80, 62, 80, 80, 80, 63,
    52, 53, 54, 55, 56, 57, 58, 59,   60, 61, 80, 80, 80, 70, 80, 80,
    80,  0,  1,  2,  3,  4,  5,  6,    7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22,   23, 24, 25, 80, 80, 80, 80, 80,
    80, 26, 27, 28, 29, 30, 31, 32,   33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48,   49, 50, 51, 80, 80, 80, 80, 80
  };
  const unsigned char *p_b64 = (const unsigned char *) ((const char *)b64);
  int chars_left = b64.lengthof();
  unsigned char *output = (unsigned char*)Malloc(((chars_left >> 2) + 1) * 3);
  unsigned char *p_output = output;
  unsigned int bits = 0;
  size_t n_bits = 0;
  while(chars_left--) {
    unsigned char dec;
    if (*p_b64 > 0 && (dec = decode_table[*p_b64]) < 64) {
      bits <<= 6;
      bits |= dec;
      n_bits += 6;
      if (n_bits >= 8) {
        *p_output++ = (bits >> (n_bits - 8)) & 0xff;
        n_bits-= 8;
      }
    } 
    else if (*p_b64 == '=') {
      break;
    }
    else {
      if (*p_b64 == '\r' && *(p_b64 + 1) == '\n') {
        ++p_b64; // skip \n too
      }
      else {
        Free(output);
        TTCN_error("Error: Invalid character in Base64 encoded data: 0x%02X", *p_b64);
      }
    }
    ++p_b64;
  }
  OCTETSTRING ret_val(p_output - output, output);
  Free(output);
  return ret_val;
}

