/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Bibo, Zoltan
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef Int_HH_
#define Int_HH_

#include "Real.hh"
#include "string.hh"

#include <openssl/bn.h>

#include <limits.h>

// Better together.
#if !defined(LLONG_MAX) || !defined(LLONG_MIN)
#define LLONG_MAX 9223372036854775807LL
#define LLONG_MIN (-LLONG_MAX-1)
#else
#if __GNUC__ < 3
#undef LLONG_MIN
#define LLONG_MIN ((long long)(-LLONG_MAX-1))
#endif
#endif

namespace Common {

class Location;

typedef long long Int;
typedef int RInt;

// Converts the Common::Int value to string.
string Int2string(const Int& i);

// Converts the string value to Common::Int.  Throws Error_Int if conversion
// is not possible (e.g. value is out of range, or s is not a valid integer
// representation).
Int string2Int(const char *s, const Location& loc);
inline Int string2Int(const string& s, const Location& loc)
  { return string2Int(s.c_str(), loc); }

class int_val_t
{
private:
  bool native_flag;
  union {
    Int native;
    BIGNUM *openssl;
  } val;
  // Hide all BIGNUM related stuff here.
  BIGNUM *to_openssl() const;
  BIGNUM *get_val_openssl() const;
  explicit int_val_t(BIGNUM *v) : native_flag(false), val() { val.openssl = v; }

public:
  int_val_t();
  int_val_t(const int_val_t& v);
  int_val_t(const char *s, const Location& loc);
  explicit int_val_t(Int v) : native_flag(true), val() { val.native = v; }
  ~int_val_t();
  string t_str() const;
  int_val_t operator-() const;
  int_val_t operator>>(Int right) const;
  int_val_t operator+(const int_val_t& right) const;
  int_val_t operator-(const int_val_t& right) const;
  inline int_val_t operator+(Int right) const { return operator+(int_val_t(right)); }
  inline int_val_t operator-(Int right) const { return operator-(int_val_t(right)); }
  int_val_t operator*(const int_val_t& right) const;
  int_val_t operator/(const int_val_t& right) const;
  int_val_t operator&(Int right) const;
  bool operator<(const int_val_t& right) const;
  inline bool operator<(Int right) const { return *this < int_val_t(right); }
  inline bool operator>(Int right) const { return *this > int_val_t(right); }
  inline bool operator>(const int_val_t& right) const { return *this != right && !(*this < right); }
  inline bool operator<=(Int right) const { return *this == right || *this < right; }
  inline bool operator>=(Int right) const { return *this == right || *this > right; }
  inline bool operator<=(const int_val_t& right) const { return *this == right || *this < right; }
  inline bool operator>=(const int_val_t& right) const { return *this == right || *this > right; }
  inline bool operator!=(Int right) const { return !(*this == right); }
  inline bool operator!=(const int_val_t& right) const { return !(*this == right); }
  bool operator==(const int_val_t& right) const;
  bool operator==(Int right) const;
  int_val_t& operator=(const int_val_t& right);
  int_val_t& operator+=(Int right);
  int_val_t& operator<<=(Int right);
  int_val_t& operator>>=(Int right);
  const Int& get_val() const;
  Real to_real() const;
  inline bool is_native_fit() const
    // It seems to give correct results (-2147483648..2147483647).
    { return native_flag && val.native == static_cast<RInt>(val.native); }
  inline bool is_native() const { return native_flag; }
  inline bool is_openssl() const { return !native_flag; }
  inline bool is_negative() const
    { return native_flag ? val.native < 0 : BN_is_negative(val.openssl); }
};

}  // Common

#endif  // Int_HH_
