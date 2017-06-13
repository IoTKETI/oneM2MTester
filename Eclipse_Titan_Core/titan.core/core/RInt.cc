/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "Error.hh"
#include "RInt.hh"
#include "../common/memory.h"

// Note: Do not use dbgnew.hh.  It doesn't play well with Qt in mctr_gui.
// This class is mostly the same as compiler2/Int.cc/Int.hh.  They should be
// merged later, but it doesn't seem to be easy.

#include <openssl/crypto.h>
#include <openssl/bn.h>

int_val_t::int_val_t() : native_flag(TRUE)
{
  val.openssl = NULL;
}

int_val_t::int_val_t(const int_val_t& v)
{
  native_flag = v.is_native();
  if (native_flag) val.native = v.get_val();
  else val.openssl = BN_dup(v.get_val_openssl());
}

int_val_t::int_val_t(const char *s)
{
  BIGNUM *n = NULL;
  if (!BN_dec2bn(&n, *s == '+' ? s + 1 : s))
    TTCN_error("Unexpected error when converting `%s' to integer", s);
  if (BN_num_bits(n) > (int)sizeof(int) * 8 - 1) {
    native_flag = FALSE;
    val.openssl = n;
  } else {
    native_flag = TRUE;
    val.native = string2RInt(s);
    BN_free(n);
  }
}

int_val_t::~int_val_t()
{
  if (!native_flag) BN_free(val.openssl);
}

const RInt& int_val_t::get_val() const
{
  if (!native_flag) TTCN_error("Invalid conversion of a large integer value");
  return val.native;
}

BIGNUM *int_val_t::get_val_openssl() const
{
  if (native_flag) TTCN_error("Invalid conversion of a large integer value");
  return val.openssl;
}

// The returned string must be freed with Free by the caller.  Returning
// CHARSTRING would be much better, but there're some linking issues.
char *int_val_t::as_string() const
{
  if (native_flag) {
    char *tmp = mprintf("%d", val.native);
    return tmp;
  } else {
    char *tmp1 = NULL, *tmp2 = NULL;
    if (!(tmp1 = BN_bn2dec(val.openssl))) TTCN_error("int_val_t::c_str()");
    tmp2 = mcopystr(tmp1);
    OPENSSL_free(tmp1);
    return tmp2;
  }
}

int_val_t& int_val_t::operator=(RInt v)
{
  if (!native_flag) BN_free(val.openssl);
  native_flag = TRUE;
  val.native = v;
  return *this;
}

int_val_t& int_val_t::operator=(const int_val_t& right)
{
  if (!native_flag) BN_free(val.openssl);
  native_flag = right.is_native();
  if (native_flag) val.native = right.get_val();
  else val.openssl = BN_dup(right.get_val_openssl());
  return *this;
}

int_val_t& int_val_t::operator<<=(RInt right)
{
  // It makes no sense to support negative operands. GCC returns constant "0"
  // with "warning: left shift count is negative" for these shifts.
  if (right < 0) TTCN_error("The second operand of bitwise shift operators "
    "cannot be negative");
  if (right == 0) return *this;
  if (native_flag) {
    BIGNUM *result = BN_new();
    // Ugly, but can't do better now.
    char *tmp_str = as_string();
    BN_dec2bn(&result, tmp_str);
    Free(tmp_str);
    BN_lshift(result, result, right);
    if (BN_num_bits(result) > (int)sizeof(int) * 8 - 1) {
      val.openssl = result;
      native_flag = FALSE;
    } else {
      val.native <<= right;
      BN_free(result);
    }
  } else {
    BN_lshift(val.openssl, val.openssl, right);
  }
  return *this;
}

int_val_t& int_val_t::operator>>=(RInt right)
{
  if (right < 0) TTCN_error("The second operand of bitwise shift operators "
    "cannot be negative");
  if (right == 0) return *this;
  if (native_flag) {
    val.native >>= right;
  } else {
    BN_rshift(val.openssl, val.openssl, right);
    if (BN_num_bits(val.openssl) <= (int)sizeof(RInt) * 8 - 1) {
      char *result_str = BN_bn2dec(val.openssl);
      RInt result_i = string2RInt(result_str);
      OPENSSL_free(result_str);
      native_flag = TRUE;
      BN_free(val.openssl);
      val.native = result_i;
    }
  }
  return *this;
}

int_val_t& int_val_t::operator+=(RInt right)
{
  // Unfortunately we have to check the sign of the "right" operand and
  // perform addition or subtraction accordingly.
  if (right == 0) return *this;
  boolean neg = right < 0;
  if (native_flag) {
    BIGNUM *result = BN_new();
    BN_set_word(result, (BN_ULONG)val.native);
    if (neg) BN_sub_word(result, (BN_ULONG)right);
    else BN_add_word(result, (BN_ULONG)right);
    if (BN_num_bits(result) > (int)sizeof(int) * 8 - 1) {
      val.openssl = result;
      native_flag = FALSE;
    } else {
      val.native += right;
      BN_free(result);
    }
  } else {
    if (neg) BN_sub_word(val.openssl, (BN_ULONG)right);
    else BN_add_word(val.openssl, (BN_ULONG)right);
    if (BN_num_bits(val.openssl) <= (int)sizeof(int) * 8 - 1) {
      BN_ULONG tmp = BN_get_word(val.openssl);
      if (BN_is_negative(val.openssl)) tmp *= -1;
      BN_free(val.openssl);
      val.native = tmp;
      native_flag = TRUE;
    }
  }
  return *this;
}

boolean int_val_t::operator==(const int_val_t& right) const
{
  if (native_flag) {
    if (right.is_native()) {
      return val.native == right.val.native;
    } else {
      BIGNUM *this_big = to_openssl(val.native);
      int eq = BN_cmp(this_big, right.get_val_openssl());
      BN_free(this_big);
      return eq == 0;
    }
  } else {
    if (right.is_native()) {
      BIGNUM *right_big = to_openssl(right.val.native);
      int eq = BN_cmp(val.openssl, right_big);
      BN_free(right_big);
      return eq == 0;
    } else {
      return BN_cmp(val.openssl, right.val.openssl) == 0;
    }
  }
}

boolean int_val_t::operator<(const int_val_t& v) const
{
  if (native_flag) {
    if (v.is_native()) {
      return val.native < v.val.native;
    } else {
      BIGNUM *this_big = to_openssl(val.native);
      int this_equ = BN_cmp(this_big, v.get_val_openssl());
      BN_free(this_big);
      return this_equ == -1;
    }
  } else {
    if (v.is_native()) {
      BIGNUM *v_big = to_openssl(v.val.native);
      int v_equ = BN_cmp(val.openssl, v_big);
      BN_free(v_big);
      return v_equ == -1;
    } else {
      return BN_cmp(val.openssl, v.val.openssl) == -1;
    }
  }
}

int_val_t int_val_t::operator&(RInt right) const
{
  // TODO Right can be int_val_t.  BN_is_bit_set/BN_set_bit should be used.
  BN_ULONG right_bn_ulong = (BN_ULONG)right;
  if (right != (RInt)right_bn_ulong) TTCN_error("Bitmask is too big");
  if (native_flag) {
    return int_val_t(val.native & right);
  } else {
    BIGNUM *tmp = BN_dup(val.openssl);
    BN_mask_bits(tmp, sizeof(BN_ULONG) * 8);
    BN_ULONG word = BN_get_word(tmp);
    BN_free(tmp);
    return int_val_t(word & right_bn_ulong);
  }
}

// Cannot be inline since bignum_st is just a forward declaration in the
// header.  The compiler must know bignum_st at this point.
boolean int_val_t::is_negative() const
{
  return (native_flag && val.native < 0) ||
          (!native_flag && BN_is_negative(val.openssl));
}

double int_val_t::to_real() const
{
  if (native_flag) {
    return (double)val.native;
  } else {
    char *result_str = BN_bn2dec(val.openssl);
    double result = 0;
    if (sscanf(result_str, "%lf", &result) != 1)
       TTCN_error("Conversion of integer value `%s' to float failed",
                  result_str);
    OPENSSL_free(result_str);
    return result;
  }
}

BIGNUM *to_openssl(int other_value)
{
  BIGNUM *result = NULL;
  char *str = mprintf("%d", other_value);
  BN_dec2bn(&result, str);
  Free(str);
  return result;
}

RInt string2RInt(const char *s)
{
  errno = 0;
  RInt i = (RInt)strtol(s, (char **)NULL, 10);
  switch (errno) {
  case ERANGE:
    TTCN_error("Overflow when converting `%s' to integer value: %s", s,
      strerror(errno));
    break;
  case 0:
    break;
  default:  // EINVAL and others.
    TTCN_error("Unexpected error when converting `%s' to integer: %s", s,
      strerror(errno));
    break;
  }
  return i;
}
