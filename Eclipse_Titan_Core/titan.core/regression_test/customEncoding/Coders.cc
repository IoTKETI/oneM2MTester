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
 *
 ******************************************************************************/

#include "Coders.hh"

using Custom1::c__separator;

namespace Custom2 {

BITSTRING f__enc__rec(const Custom3::Rec& x)
{
  return int2bit(x.num(), 8);
}

INTEGER f__dec__rec(BITSTRING& b, Custom3::Rec& x)
{
  x.num() = bit2int(b);
  x.str() = "c++";
  b = BITSTRING(0, NULL);
  return 0;
}

BITSTRING f__enc__uni(const Custom1::Uni& x)
{
  if (x.get_selection() == Custom1::Uni::ALT_i) {
    return c__separator + int2bit(x.i(), 8) + c__separator;
  }
  else {
    return c__separator + c__separator;
  }
}

INTEGER f__dec__uni(BITSTRING& b, Custom1::Uni& x)
{
  int b_len = b.lengthof();
  int sep_len = c__separator.lengthof();
  if (b_len >= 2 * sep_len &&
      substr(b, 0, sep_len) == c__separator &&
      substr(b, b_len - sep_len, sep_len) == c__separator) {
    if (b_len > 2 * sep_len) {
      x.i() = bit2int(substr(b, sep_len, b_len - 2 * sep_len));
    }
    b = BITSTRING(0, NULL);
    return 0;
  }
  else {
    return 1;
  }
}

BITSTRING f__enc__bs(const BITSTRING& x)
{
  return x;
}

INTEGER f__dec__bs(BITSTRING& b, BITSTRING& x)
{
  x = b;
  b = BITSTRING(0, NULL);
  return 0;
}

} // namespace Custom2

namespace Custom1 {

BITSTRING f__enc__recof(const RecOf& x)
{
  BITSTRING res = x[0];
  for (int i = 1; i < x.size_of(); ++i) {
    res = res + c__separator + x[i];
  }
  return res;
}

int find_bitstring(const BITSTRING& src, int start, const BITSTRING& fnd)
{
  int len = fnd.lengthof();
  for (int i = start; i <= src.lengthof() - len; ++i) {
    if (substr(src, i, len) == fnd) {
      return i;
    }
  }
  return -1;
}

INTEGER f__dec__recof(BITSTRING& b, RecOf& x)
{
  int start = 0;
  int end = find_bitstring(b, start, c__separator);
  int index = 0;
  while(end != -1) {
    x[index] = substr(b, start, end - start);
    ++index;
    start = end + c__separator.lengthof();
    end = find_bitstring(b, start, c__separator);
  }
  x[index] = substr(b, start, b.lengthof() - start);
  b = BITSTRING(0, NULL);
  return 0;
}

BITSTRING f__enc__seqof(const Types::SeqOf& x)
{
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_DEFAULT);
  TTCN_Buffer buf;
  x.encode(Types::SeqOf_descr_, buf, TTCN_EncDec::CT_JSON, false);
  OCTETSTRING tmp;
  buf.get_string(tmp);
  return oct2bit(tmp);
}

INTEGER f__dec__seqof(BITSTRING& x, Types::SeqOf& y)
{
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);
  TTCN_Buffer buf(bit2oct(x));
  y.decode(Types::SeqOf_descr_, buf, TTCN_EncDec::CT_JSON);
  switch (TTCN_EncDec::get_last_error_type()) {
  case TTCN_EncDec::ET_NONE: {
    buf.cut();
    OCTETSTRING tmp;
    buf.get_string(tmp);
    x = oct2bit(tmp);
    return 0; }
  case TTCN_EncDec::ET_INCOMPL_MSG:
  case TTCN_EncDec::ET_LEN_ERR:
    return 2;
  default:
    return 1;
  }
}

INTEGER f__dec__choice(BITSTRING& x, Types::Choice& y)
{
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);
  TTCN_Buffer buf(bit2oct(x));
  y.decode(Types::Choice_descr_, buf, TTCN_EncDec::CT_JSON);
  switch (TTCN_EncDec::get_last_error_type()) {
  case TTCN_EncDec::ET_NONE: {
    buf.cut();
    OCTETSTRING tmp;
    buf.get_string(tmp);
    x = oct2bit(tmp);
    return 0; }
  case TTCN_EncDec::ET_INCOMPL_MSG:
  case TTCN_EncDec::ET_LEN_ERR:
    return 2;
  default:
    return 1;
  }
}

} // namespace Custom1

namespace Custom5 {

BITSTRING f__enc__set(const Custom3::Set& x)
{
  return int2bit(x.num(), 8);
}

INTEGER f__dec__set(BITSTRING& b, Custom3::Set& x)
{
  x.num() = bit2int(b);
  x.str() = "c++";
  b = BITSTRING(0, NULL);
  return 0;
}

BITSTRING f__enc__setof(const Types::SetOf& x)
{
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_DEFAULT);
  TTCN_Buffer buf;
  x.encode(Types::SetOf_descr_, buf, TTCN_EncDec::CT_JSON, false);
  OCTETSTRING tmp;
  buf.get_string(tmp);
  return oct2bit(tmp);
}

INTEGER f__dec__setof(BITSTRING& x, Types::SetOf& y)
{
  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);
  TTCN_Buffer buf(bit2oct(x));
  y.decode(Types::SetOf_descr_, buf, TTCN_EncDec::CT_JSON);
  switch (TTCN_EncDec::get_last_error_type()) {
  case TTCN_EncDec::ET_NONE: {
    buf.cut();
    OCTETSTRING tmp;
    buf.get_string(tmp);
    x = oct2bit(tmp);
    return 0; }
  case TTCN_EncDec::ET_INCOMPL_MSG:
  case TTCN_EncDec::ET_LEN_ERR:
    return 2;
  default:
    return 1;
  }
}

} // namespace Custom5
