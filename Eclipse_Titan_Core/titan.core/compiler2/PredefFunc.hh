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
#ifndef _Common_PredefFunc_HH
#define _Common_PredefFunc_HH

#include "Int.hh"
#include "Real.hh"
#include "../common/CharCoding.hh"

class string;
class ustring;

namespace Common {

  extern char hexdigit_to_char(unsigned char hexdigit);
  extern unsigned char char_to_hexdigit(char c);

  extern int_val_t rem(const int_val_t& left, const int_val_t& right);
  extern int_val_t mod(const int_val_t& left, const int_val_t& right);
  extern string* to_uppercase(const string& value);
  extern string* not4b_bit(const string& bstr);
  extern string* not4b_hex(const string& hstr);
  extern string* and4b(const string& left, const string& right);
  extern string* or4b(const string& left, const string& right);
  extern string* xor4b(const string& left, const string& right);
  extern string* shift_left(const string& value, const Int& count);
  extern string* shift_right(const string& value, const Int& count);
  extern string* rotate_left(const string& value, const Int& count);
  extern string* rotate_right(const string& value, const Int& count);
  extern ustring* rotate_left(const ustring& value, const Int& count);
  extern ustring* rotate_right(const ustring& value, const Int& count);

  extern int_val_t* bit2int(const string& bstr);
  extern int_val_t* hex2int(const string& hstr);
  extern Int unichar2int(const ustring& ustr);
  extern string* int2bit(const int_val_t& value, const Int& length);
  extern string* int2hex(const int_val_t& value, const Int& length);
  extern ustring* int2unichar(const Int& value);
  extern string* oct2char(const string& ostr);
  extern string* char2oct(const string& cstr);
  extern string* bit2hex(const string& bstr);
  extern string* hex2oct(const string& hstr);
  extern string* asn_hex2oct(const string& hstr);
  extern string* bit2oct(const string& bstr);
  extern string* asn_bit2oct(const string& bstr);
  extern string* hex2bit(const string& hstr);
  extern Real int2float(const int_val_t& value);
  extern int_val_t* float2int(const Real& value, const Location& loc);
  extern string* float2str(const Real& value);
  extern string* regexp(const string& instr, const string& expression,
                        const Int& groupno, bool nocase);
  extern ustring* regexp(const ustring& instr, const ustring& expression,
                        const Int& groupno, bool nocase);
  extern string* remove_bom(const string& encoded_value);
  extern string* get_stringencoding(const string& encoded_value);
  extern ustring decode_utf8(const string & ostr, CharCoding::CharCodingType expected_coding);
}

#endif // _Common_PredefFunc_HH
