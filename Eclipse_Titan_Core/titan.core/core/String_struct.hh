/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef STRING_STRUCT_HH
#define STRING_STRUCT_HH

#include "Types.h"
#include "Bitstring.hh"
#include "Hexstring.hh"
#include "Octetstring.hh"
#include "Charstring.hh"
#include "Universal_charstring.hh"
#include "Encdec.hh"
#include "CharCoding.hh"

/** Type of the reference counters in all structures. */
typedef unsigned int ref_count_t;

/** Packed storage of bits, filled from LSB. */
struct BITSTRING::bitstring_struct {
  ref_count_t ref_count;
  int n_bits;
  unsigned char bits_ptr[sizeof(int)];
};

/** Each element occupies one byte. Meaning of values:
 * 0 -> 0, 1 -> 1, 2 -> ?, 3 -> * */
struct BITSTRING_template::bitstring_pattern_struct {
  ref_count_t ref_count;
  unsigned int n_elements;
  unsigned char elements_ptr[1];
};

/** Packed storage of hex digits, filled from LSB. */
struct HEXSTRING::hexstring_struct {
  ref_count_t ref_count;
  int n_nibbles;
  unsigned char nibbles_ptr[sizeof(int)];
};

/** Each element occupies one byte. Meaning of values:
 * 0 .. 15 -> 0 .. F, 16 -> ?, 17 -> * */
struct HEXSTRING_template::hexstring_pattern_struct {
  ref_count_t ref_count;
  unsigned int n_elements;
  unsigned char elements_ptr[1];
};

struct OCTETSTRING::octetstring_struct {
  ref_count_t ref_count;
  int n_octets;
  unsigned char octets_ptr[sizeof(int)];
};

/** Each element is represented as an unsigned short. Meaning of values:
 * 0 .. 255 -> 00 .. FF, 256 -> ?, 257 -> * */
struct OCTETSTRING_template::octetstring_pattern_struct {
  ref_count_t ref_count;
  unsigned int n_elements;
  unsigned short elements_ptr[1];
};

struct CHARSTRING::charstring_struct {
  ref_count_t ref_count;
  int n_chars;
  char chars_ptr[sizeof(int)];
};

struct UNIVERSAL_CHARSTRING::universal_charstring_struct {
  ref_count_t ref_count;
  int n_uchars;
  universal_char uchars_ptr[1];
};

/* The layout of this structure must match that of charstring_struct */
struct TTCN_Buffer::buffer_struct {
  ref_count_t ref_count;
  int unused_length_field; /**< placeholder only */
  unsigned char data_ptr[sizeof(int)];
};

/** Structure for storing a decoded content matching mechanism 
  * (for bitstrings, hexstrings, octetstrings and charstrings) */
struct decmatch_struct {
  ref_count_t ref_count;
  Dec_Match_Interface* instance;
};

/** Structure for storing a decoded content matching mechanism for universal
  * charstrings (also contains the character coding method) */
struct unichar_decmatch_struct {
  ref_count_t ref_count;
  Dec_Match_Interface* instance;
  CharCoding::CharCodingType coding;
};

#endif
