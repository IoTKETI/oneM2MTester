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
 *   Gecse, Roland
 *   Koppany, Csaba
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "../common/dbgnew.hh"
#include "Int.hh"
#include "string.hh"
#include "error.h"
#include "Setting.hh"

#include <openssl/crypto.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// We cannot check without using a "./configure" script or such if we have
// llabs() or not.  Define our own function instead.
inline long long ll_abs(long long x) { return ((x >= 0) ? (x) : (-x)); }

namespace Common {

string Int2string(const Int& i)
{
  char *s = NULL;
  s = mprintf("%lld", i);
  string str(s);
  Free(s);
  return str;
}

Int string2Int(const char *s, const Location& loc)
{
  errno = 0;
  Int i = strtoll(s, NULL, 10);
  switch (errno) {
  case ERANGE: {
    if (loc.get_filename() != NULL) {
      loc.error("Overflow when converting `%s' to integer value: %s", s,
        strerror(errno));
    } else {
      FATAL_ERROR("Overflow when converting `%s' to integer value: %s", s,
        strerror(errno));
    }
    break; }
  case 0:
    break;
  default:
    FATAL_ERROR("Unexpected error when converting `%s' to integer: %s", s,
      strerror(errno));
  }
  return i;
}

int_val_t::int_val_t() : native_flag(true)
{
  val.openssl = NULL;
}

int_val_t::int_val_t(const int_val_t& v)
{
  native_flag = v.is_native();
  if (native_flag) val.native = v.get_val();
  else val.openssl = BN_dup(v.get_val_openssl());
}

int_val_t::int_val_t(const char *s, const Location& loc)
{
  BIGNUM *n = NULL;
  if (!BN_dec2bn(&n, *s == '+' ? s + 1 : s))
    loc.error("Unexpected error when converting `%s' to integer", s);
  if (BN_num_bits(n) > (int)sizeof(long long) * 8 - 1) {
    native_flag = false;
    val.openssl = n;
  } else {
    native_flag = true;
    val.native = string2Int(s, loc);
    BN_free(n);
  }
}

int_val_t::~int_val_t()
{
  if (!native_flag) BN_free(val.openssl);
}

string int_val_t::t_str() const
{
  char *tmp = NULL;
  if (native_flag) {
    tmp = mprintf("%lld", val.native);
    string s(tmp);
    Free(tmp);
    return s;
  } else {
    if (!(tmp = BN_bn2dec(val.openssl)))
      FATAL_ERROR("int_val_t::t_str()");
    string s(tmp);
    OPENSSL_free(tmp);
    return s;
  }
}

BIGNUM *int_val_t::to_openssl() const
{
  BIGNUM *big = NULL;
  if (native_flag) {
    string str = Int2string(val.native);
    if (!BN_dec2bn(&big, str.c_str())) FATAL_ERROR("int_val_t::to_openssl()");
  } else {
    big = BN_dup(val.openssl);
    if (!big) FATAL_ERROR("int_val_t::to_openssl()");
  }
  return big;
}

bool int_val_t::operator==(Int right) const
{
  if (!native_flag) return false;
  return val.native == right;
}

bool int_val_t::operator==(const int_val_t& v) const
{
  if (native_flag != v.is_native()) return false;
  if (native_flag) return val.native == v.get_val();
  return !BN_cmp(val.openssl, v.get_val_openssl());
}

bool int_val_t::operator<(const int_val_t& v) const
{
  if (native_flag) {
    if (v.is_native()) {
      return val.native < v.get_val();
    } else {
      BIGNUM *this_big = to_openssl();
      if (!this_big) FATAL_ERROR("int_val_t::operator<(int_val_t& v)");
      int this_equ = BN_cmp(this_big, v.get_val_openssl());
      BN_free(this_big);
      return this_equ == -1;
    }
  } else {
    if (v.is_native()) {
      BIGNUM *v_big = v.to_openssl();
      if (!v_big) FATAL_ERROR("int_val_t::operator<(int_val_t& v)");
      int v_equ = BN_cmp(val.openssl, v_big);
      BN_free(v_big);
      return v_equ == -1;
    } else {
      return BN_cmp(val.openssl, v.val.openssl) == -1;
    }
  }
}

int_val_t int_val_t::operator-() const
{
  if (native_flag) {
    if (val.native == LLONG_MIN) {
      BIGNUM *result = int_val_t(LLONG_MIN).to_openssl();
      BN_set_negative(result, 0);
      return int_val_t(result);
    } else {
      return int_val_t(-val.native);
    }
  } else {
    BIGNUM *llong_max_plus_one = int_val_t(LLONG_MIN).to_openssl();
    BN_set_negative(llong_max_plus_one, 0);
    int cmp = BN_cmp(val.openssl, llong_max_plus_one);
    BN_free(llong_max_plus_one);
    if (cmp == 0) {
      return int_val_t(LLONG_MIN);
    } else {
      BIGNUM *result = BN_dup(val.openssl);
      BN_set_negative(result, !BN_is_negative(result));
      return int_val_t(result);
    }
  }
}

int_val_t int_val_t::operator+(const int_val_t& right) const
{
  //  a +  b =   a add b
  //  a + -b =   a sub b
  // -a +  b =   b sub a
  // -a + -b = -(a add b)
  // Use only inline functions and BN_* directly.  Call out for operator- in
  // the beginning.
  bool a_neg = is_negative();
  bool b_neg = right.is_negative();
  if (!a_neg && b_neg) return operator-(-right);
  if (a_neg && !b_neg) return right.operator-(-(*this));
  if (native_flag) {
    if (right.is_native()) {
      unsigned long long result = val.native + right.get_val();
      long long result_ = val.native + right.get_val();
      bool r_neg = a_neg && b_neg;
      if (static_cast<long long>(result) != result_ ||
          (!r_neg && result_ < 0) || (r_neg && result_ > 0)) {
        // We can safely assume that the sum of two non-negative long long
        // values fit in an unsigned long long.  limits.h says:
        // # ifndef ULLONG_MAX
        // # define ULLONG_MAX (LLONG_MAX * 2ULL + 1)
        // # endif
        // This is the most complicated case.  We cannot be sure that
        // sizeof(BN_ULONG) == sizeof(long long).  First convert the long long
        // to string and feed BN_dec2bn.
        BIGNUM *left_ = to_openssl();
        BIGNUM *right_ = right.to_openssl();
        BN_add(left_, left_, right_);
        BN_free(right_);
        return int_val_t(left_);
      } else {
        return int_val_t(result_);
      }
    } else {
      // long long (>= 0) + BIGNUM == BIGNUM.
      BIGNUM *result = BN_new();
      BIGNUM *left_ = to_openssl();
      BN_add(result, left_, right.get_val_openssl());
      return int_val_t(result);
    }
  } else {
    // BIGNUM + long long (>= 0) == BIGNUM.
    BIGNUM *result = BN_new();
    BIGNUM *right_;
    right_ = right.is_native() ? right.to_openssl() : right.get_val_openssl();
    BN_add(result, val.openssl, right_);
    if (right.is_native())
      BN_free(right_);
    return int_val_t(result);
  }
}

int_val_t int_val_t::operator-(const int_val_t& right) const
{
  //  a -  b =   a sub b
  // -a -  b = -(a add b)
  //  a - -b =   a add b
  // -a - -b =  -a add b  = b sub a
  bool a_neg = is_negative();
  bool b_neg = right.is_negative();
  if (!a_neg && b_neg) return operator+(-right);
  if (a_neg && !b_neg) return -right.operator+(-(*this));
  if (native_flag) {
    if (right.is_native()) {
      // Since both operands are non-negative the most negative result of a
      // subtraction can be -LLONG_MAX and according to limits.h:
      // # ifndef LLONG_MIN
      // # define LLONG_MIN (-LLONG_MAX-1)
      // # endif
      return int_val_t(val.native - right.get_val());
    } else {
      BIGNUM *left_bn = to_openssl();
      BN_sub(left_bn, left_bn, right.get_val_openssl());
      // The result can be small enough to fit in long long.  The same is true
      // for division.  Back conversion is a really costly operation using
      // strings all the time.  TODO Improve it.
      if (BN_num_bits(left_bn) <= (int)sizeof(long long) * 8 - 1) {
        char *result_str = BN_bn2dec(left_bn);
        Int result_ll = string2Int(result_str, Location());
        OPENSSL_free(result_str);
        BN_free(left_bn);
        return int_val_t(result_ll);
      } else {
        return int_val_t(left_bn);
      }
    }
  } else {
    BIGNUM *result = BN_new();
    BIGNUM *right_bn;
    right_bn = right.is_native() ? right.to_openssl() :
      right.get_val_openssl();
    BN_sub(result, val.openssl, right_bn);
    if (right.is_native()) BN_free(right_bn);
    if (BN_num_bits(result) <= (int)sizeof(long long) * 8 - 1) {
      char *result_str = BN_bn2dec(result);
      Int result_ll = string2Int(result_str, Location());
      OPENSSL_free(result_str);
      return int_val_t(result_ll);
    } else {
      return int_val_t(result);
    }
  }
}

int_val_t int_val_t::operator*(const int_val_t& right) const
{
  if ((native_flag && val.native == 0LL) ||
    (right.native_flag && right.val.native == 0LL))
    return int_val_t(0LL);
  if (native_flag) {
    if (right.native_flag) {
      // 2^15 is used as a simple heuristic.
      // TODO: Improve.
      if (ll_abs(val.native) < 32768LL && ll_abs(right.val.native) < 32768LL) {
        return int_val_t(val.native * right.val.native);
      } else {
        BIGNUM *left_bn = to_openssl();
        BIGNUM *right_bn = right.to_openssl();
        BN_CTX *ctx = BN_CTX_new();
        BN_mul(left_bn, left_bn, right_bn, ctx);
        BN_CTX_free(ctx);
        BN_free(right_bn);
        if (BN_num_bits(left_bn) < (int)sizeof(long long) * 8) {
          BN_free(left_bn);
          return int_val_t(val.native * right.val.native);
        } else {
          return int_val_t(left_bn);
        }
      }
    } else {
      BIGNUM *this_bn = to_openssl();
      BN_CTX *ctx = BN_CTX_new();
      BN_mul(this_bn, this_bn, right.get_val_openssl(), ctx);
      BN_CTX_free(ctx);
      return int_val_t(this_bn);
    }
  } else {
    BIGNUM *result = BN_new();
    BIGNUM *right_bn;
    BN_CTX *ctx = BN_CTX_new();
    right_bn = right.native_flag ? right.to_openssl()
      : right.get_val_openssl();
    BN_mul(result, val.openssl, right_bn, ctx);
    BN_CTX_free(ctx);
    if (right.native_flag) BN_free(right_bn);
    return int_val_t(result);
  }
}

int_val_t int_val_t::operator/(const int_val_t& right) const
{
  if (native_flag && val.native == 0LL)
    return int_val_t(0LL);
  if (right.is_native() && right.get_val() == 0LL)
    FATAL_ERROR("Division by zero after semantic check");
  if (native_flag) {
    if (right.native_flag) {
      return int_val_t(val.native / right.get_val());
    } else {
      BIGNUM *left_bn = to_openssl();
      BN_CTX *ctx = BN_CTX_new();
      BN_div(left_bn, NULL, left_bn, right.get_val_openssl(), ctx);
      BN_CTX_free(ctx);
      if (BN_num_bits(left_bn) <= (int)sizeof(long long) * 8 - 1) {
        char *result_str = BN_bn2dec(left_bn);
        Int result_ll = string2Int(result_str, Location());
        OPENSSL_free(result_str);
        BN_free(left_bn);
        return int_val_t(result_ll);
      } else {
        return int_val_t(left_bn);
      }
    }
  } else {
    BIGNUM *result = BN_new();
    BIGNUM *right_bn;
    BN_CTX *ctx = BN_CTX_new();
    right_bn = right.is_native() ? right.to_openssl() :
      right.get_val_openssl();
    BN_div(result, NULL, val.openssl, right_bn, ctx);
    BN_CTX_free(ctx);
    if (BN_num_bits(result) <= (int)sizeof(long long) * 8 - 1) {
      char *result_str = BN_bn2dec(result);
      Int result_ll = string2Int(result_str, Location());
      OPENSSL_free(result_str);
      return int_val_t(result_ll);
    } else {
      if (right.is_native())
        BN_free(right_bn);
      return int_val_t(result);
    }
  }
}

int_val_t int_val_t::operator&(Int right) const
{
  // TODO Right can be int_val_t.  Now it works only if right fits in
  // BN_ULONG.  If it's not true right must be converted to BIGNUM and the
  // bits should be set with BN_is_bit_set/BN_set_bit.
  BN_ULONG right_bn_ulong = (BN_ULONG)right;
  if (right != (long long)right_bn_ulong)
    FATAL_ERROR("Bitmask is too big");
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

int_val_t int_val_t::operator>>(Int right) const
{
  if (native_flag) {
    // Shifting right (or left) with a number greater or equal to the bits of
    // the type of the left operand has an undefined behaviour.
    // http://bytes.com/groups/c/495137-right-shift-weird-result-why
    Int shifted_value = right >= static_cast<Int>(sizeof(Int) * 8) ? 0 :
      val.native >> right;
    return int_val_t(shifted_value);
  } else {
    BIGNUM *result = BN_new();
    BN_rshift(result, val.openssl, right);
    if (BN_num_bits(result) < (int)sizeof(long long) * 8 - 1) {
      char *result_str = BN_bn2dec(result);
      Int result_ll = string2Int(result_str, Location());
      OPENSSL_free(result_str);
      BN_free(result);
      return int_val_t(result_ll);
    } else {
      return int_val_t(result);
    }
  }
}

const Int& int_val_t::get_val() const
{
  if (!native_flag) FATAL_ERROR("Invalid conversion of a large integer value");
  return val.native;
}

BIGNUM *int_val_t::get_val_openssl() const
{
  if (native_flag) FATAL_ERROR("Invalid conversion of a large integer value");
  return val.openssl;
}

Real int_val_t::to_real() const
{
  if (native_flag) {
    return (double)val.native;
  } else {
    char *result_str = BN_bn2dec(val.openssl);
    Real result = 0.0;
    // Use fixed-point notation.  The mantissa is usually at most 52-bits.
    // Bigger integer values will be rounded.
    if (sscanf(result_str, "%lf", &result) != 1)
      FATAL_ERROR("Conversion of integer value `%s' to float failed",
                  result_str);  // No deallocation, it'll crash anyway...
    OPENSSL_free(result_str);
    return result;
  }
}

int_val_t& int_val_t::operator=(const int_val_t& right)
{
  if (!native_flag) BN_free(val.openssl);
  native_flag = right.native_flag;
  if (native_flag) val.native = right.get_val();
  else val.openssl = BN_dup(right.get_val_openssl());
  return *this;
}

int_val_t& int_val_t::operator<<=(Int right)
{
  // It makes no sense to support negative operands. GCC returns constant "0"
  // with "warning: left shift count is negative" for these shifts.
  // BN_set_word is not enough since sizeof(BN_ULONG) != sizeof(long long).
  // In TTCN-3 <<= right means >>= -right.
  if (right < 0) FATAL_ERROR("The second operand of bitwise shift operators "
    "cannot be negative");
  if (right == 0) return *this;
  if (native_flag) {
    BIGNUM *result = BN_new();
    BN_dec2bn(&result, Int2string(val.native).c_str());
    BN_lshift(result, result, right);
    if (BN_num_bits(result) > (int)sizeof(long long) * 8 - 1) {
      val.openssl = result;
      native_flag = false;
    } else {
      val.native <<= right;
      BN_free(result);
    }
  } else {
    BN_lshift(val.openssl, val.openssl, right);
  }
  return *this;
}

int_val_t& int_val_t::operator>>=(Int right)
{
  if (right < 0) FATAL_ERROR("The second operand of bitwise shift operators "
    "cannot be negative");
  if (right == 0) return *this;
  if (native_flag) {
    val.native >>= right;
  } else {
    BN_rshift(val.openssl, val.openssl, right);
    if (BN_num_bits(val.openssl) <= (int)sizeof(long long) * 8 - 1) {
      char *result_str = BN_bn2dec(val.openssl);
      Int result_ll = string2Int(result_str, Location());
      OPENSSL_free(result_str);
      native_flag = true;
      BN_free(val.openssl);
      val.native = result_ll;
    }
  }
  return *this;
}

int_val_t& int_val_t::operator+=(Int right)
{
  // Unfortunately we have to check the sign of the "right" operand and
  // perform addition or subtraction accordingly.
  if (right == 0) return *this;
  bool neg = right < 0;
  if (native_flag) {
    BIGNUM *result = BN_new();
    BN_set_word(result, (BN_ULONG)val.native);
    if (neg) BN_sub_word(result, (BN_ULONG)right);
    else BN_add_word(result, (BN_ULONG)right);
    if (BN_num_bits(result) > (int)sizeof(long long) * 8 - 1) {
      val.openssl = result;
      native_flag = false;
    } else {
      val.native += right;
      BN_free(result);
    }
  } else {
    if (neg) BN_sub_word(val.openssl, (BN_ULONG)right);
    else BN_add_word(val.openssl, (BN_ULONG)right);
    if (BN_num_bits(val.openssl) <= (int)sizeof(long long) * 8 - 1) {
      // TODO BN_ULONG != long long.
      BN_ULONG tmp = BN_get_word(val.openssl);
      if (BN_is_negative(val.openssl)) tmp *= -1;
      BN_free(val.openssl);
      val.native = tmp;
      native_flag = true;
    }
  }
  return *this;
}

}  // Common
