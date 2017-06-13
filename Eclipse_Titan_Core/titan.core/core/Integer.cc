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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Ormandi, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include "Integer.hh"

#include <limits.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#include "Error.hh"
#include "Logger.hh"
#include "Optional.hh"
#include "Types.h"
#include "Param_Types.hh"
#include "Encdec.hh"
#include "RAW.hh"
#include "BER.hh"
#include "TEXT.hh"
#include "Charstring.hh"
#include "Addfunc.hh"
#include "XmlReader.hh"

#include <openssl/bn.h>
#include <openssl/crypto.h>

#include "../common/dbgnew.hh"

#if defined(__GNUC__) && __GNUC__ >= 3
// To provide prediction information for the compiler.
// Borrowed from /usr/src/linux/include/linux/compiler.h.
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif

static const Token_Match integer_value_match("^([\t ]*-?[0-9]+).*$", TRUE);

int_val_t INTEGER::get_val() const
{
  if (likely(native_flag)) return int_val_t(val.native);
  else return int_val_t(BN_dup(val.openssl));
}

void INTEGER::set_val(const int_val_t& other_value)
{
  clean_up();
  bound_flag = TRUE;
  native_flag = other_value.native_flag;
  if (likely(native_flag)) val.native = other_value.val.native;
  else val.openssl = BN_dup(other_value.val.openssl);
}

INTEGER::INTEGER()
{
  bound_flag = FALSE;
  native_flag = TRUE;
}

INTEGER::INTEGER(int other_value)
{
  bound_flag = TRUE;
  native_flag = TRUE;
  val.native = other_value;
}

INTEGER::INTEGER(const INTEGER& other_value)
: Base_Type(other_value)
{
  other_value.must_bound("Copying an unbound integer value.");
  bound_flag = TRUE;
  native_flag = other_value.native_flag;
  if (likely(native_flag)) val.native = other_value.val.native;
  else val.openssl = BN_dup(other_value.val.openssl);
}

/// Return 0 if fail, 1 on success
int INTEGER::from_string(const char *s) {
  BIGNUM *other_value_int = NULL;
  if (BN_dec2bn(&other_value_int, s + (*s == '+')))
  {
    bound_flag = TRUE;
    if (BN_num_bits(other_value_int) > (int)sizeof(int) * 8 - 1) {
      native_flag = FALSE;
      val.openssl = other_value_int;
    } else {
      native_flag = TRUE;
      val.native = string2RInt(s);
      BN_free(other_value_int);
    }
    return 1;
  }
  else return 0;
}

INTEGER::INTEGER(const char *other_value)
{
  if (unlikely(!other_value))
    TTCN_error("Unexpected error when converting `%s' to integer",
      other_value);
  bound_flag = TRUE;
  if (!from_string(other_value)) TTCN_error(
    "Unexpected error when converting `%s' to integer", other_value);
}

// For internal use only.  It's not part of the public interface.
INTEGER::INTEGER(BIGNUM *other_value)
{
  if (unlikely(!other_value))
    TTCN_error("Unexpected error when initializing an integer");
  bound_flag = TRUE;
  native_flag = FALSE;
  val.openssl = other_value;
}

INTEGER::~INTEGER()
{
  if (!bound_flag) return;
  if (unlikely(!native_flag)) BN_free(val.openssl);
}

void INTEGER::clean_up()
{
  if (!bound_flag) return;
  if (unlikely(!native_flag)) BN_free(val.openssl);
  bound_flag = FALSE;
}

INTEGER& INTEGER::operator=(int other_value)
{
  clean_up();
  bound_flag = TRUE;
  native_flag = TRUE;
  val.native = other_value;
  return *this;
}

INTEGER& INTEGER::operator=(const INTEGER& other_value)
{
  if (this == &other_value)
    return *this;
  other_value.must_bound("Assignment of an unbound integer value.");
  clean_up();
  bound_flag = TRUE;
  native_flag = other_value.native_flag;
  if (likely(native_flag)) val.native = other_value.val.native;
  else val.openssl = BN_dup(other_value.val.openssl);
  return *this;
}

// A bit more specific than operator+().
INTEGER& INTEGER::operator++()
{
  must_bound("Unbound integer operand of unary increment operator.");
  if (likely(native_flag)) {
    unsigned int result_u = val.native + 1;
    int result = val.native + 1;
    if (unlikely((static_cast<int>(result_u) != result) || (val.native > 0 && result < 0))) {
      BIGNUM *val_openssl = to_openssl(val.native);
      BIGNUM *one = BN_new();
      BN_set_word(one, 1);
      BN_add(val_openssl, val_openssl, one);
      BN_free(one);
      native_flag = FALSE;
      val.openssl = val_openssl;
    } else {
      val.native++;
    }
  } else {
    BIGNUM *one = BN_new();
    BN_set_word(one, 1);
    BN_add(val.openssl, val.openssl, one);
    BN_free(one);
  }
  return *this;
}

// A bit more specific than operator-().
INTEGER& INTEGER::operator--()
{
  must_bound("Unbound integer operand of unary decrement operator.");
  if (likely(native_flag)) {
    if (unlikely(val.native == INT_MIN)) {
      BIGNUM *val_openssl = to_openssl(val.native);
      BIGNUM *one = BN_new();
      BN_set_word(one, 1);
      BN_sub(val_openssl, val_openssl, one);
      BN_free(one);
      native_flag = FALSE;
      val.openssl = val_openssl;
    } else {
      val.native--;
    }
  } else {
    BIGNUM *one = BN_new();
    BN_set_word(one, 1);
    BN_sub(val.openssl, val.openssl, one);
    BN_free(one);
  }
  return *this;
}

INTEGER INTEGER::operator+() const
{
  must_bound("Unbound integer operand of unary + operator.");
  return *this;
}

INTEGER INTEGER::operator-() const
{
  must_bound("Unbound integer operand of unary - operator (negation).");
  if (likely(native_flag)) {
    if (unlikely(val.native == INT_MIN)) {
      BIGNUM *result = to_openssl(INT_MIN);
      BN_set_negative(result, 0);
      return INTEGER(result);
    } else {
      return INTEGER(-val.native);
    }
  } else {
    BIGNUM *int_max_plus_one = to_openssl(INT_MIN);
    BN_set_negative(int_max_plus_one, 0);
    int cmp = BN_cmp(val.openssl, int_max_plus_one);
    BN_free(int_max_plus_one);
    if (unlikely(cmp == 0)) {
      return INTEGER(INT_MIN);
    } else {
      BIGNUM *result = BN_dup(val.openssl);
      BN_set_negative(result, !BN_is_negative(result));
      return INTEGER(result);
    }
  }
}

INTEGER INTEGER::operator+(int other_value) const
{
  must_bound("Unbound left operand of integer addition.");
  // Don't call out if slow.  Implement this specific case right here.
  return *this + INTEGER(other_value);
}

INTEGER INTEGER::operator+(const INTEGER& other_value) const
{
  must_bound("Unbound left operand of integer addition.");
  other_value.must_bound("Unbound right operand of integer addition.");
  //  *this +  other_value =   *this add other_value
  //  *this + -other_value =   *this sub other_value
  // -*this +  other_value =   other_value sub *this
  // -*this + -other_value = -(*this add other_value)
  // Use only inline functions and BN_* directly.  Call out for operator- in
  // the beginning.
  boolean this_neg = native_flag ? (val.native < 0)
    : BN_is_negative(val.openssl);
  boolean other_value_neg = other_value.native_flag
    ? (other_value.val.native < 0) : BN_is_negative(other_value.val.openssl);
  if (!this_neg && other_value_neg) return operator-(-other_value);
  if (this_neg && !other_value_neg) return other_value.operator-(-(*this));
  if (likely(native_flag)) {
    if (likely(other_value.native_flag)) {
      boolean result_neg = this_neg && other_value_neg;
      unsigned int result_u = val.native + other_value.val.native;
      int result = val.native + other_value.val.native;
      if ((static_cast<int>(result_u) != result) || (!result_neg &&
        result < 0) || (result_neg && result > 0)) {
        // We can safely assume that the sum of two non-negative int values
        // fit in an unsigned int.  limits.h says:
        // # define INT_MAX 2147483647
        // # define UINT_MAX 4294967295
        BIGNUM *this_int = to_openssl(val.native);
        BIGNUM *other_val_int = to_openssl(other_value.val.native);
        BN_add(this_int, this_int, other_val_int);
        BN_free(other_val_int);
        return INTEGER(this_int);
      } else {
        return INTEGER(result);
      }
    } else {
      // int (>= 0) + BIGNUM == BIGNUM.
      BIGNUM *this_int = to_openssl(val.native);
      BN_add(this_int, this_int, other_value.val.openssl);
      return INTEGER(this_int);
    }
  } else {
    // BIGNUM + int (>= 0) == BIGNUM.
    BIGNUM *result = BN_new();
    BIGNUM *other_value_int;
    other_value_int = other_value.native_flag
      ? to_openssl(other_value.val.native) : other_value.val.openssl;
    BN_add(result, val.openssl, other_value_int);
    if (likely(other_value.native_flag)) BN_free(other_value_int);
    return INTEGER(result);
  }
}

INTEGER INTEGER::operator-(int other_value) const
{
  must_bound("Unbound left operand of integer subtraction.");
  return *this - INTEGER(other_value);
}

INTEGER INTEGER::operator-(const INTEGER& other_value) const
{
  must_bound("Unbound left operand of integer subtraction.");
  other_value.must_bound("Unbound right operand of integer subtraction.");
  //  *this -  other_value =   *this sub other_value
  // -*this -  other_value = -(*this add other_value)
  //  *this - -other_value =   *this add other_value
  // -*this - -other_value =  -*this add other_value  = other_value sub *this
  boolean this_neg = native_flag ? (val.native < 0)
    : BN_is_negative(val.openssl);
  boolean other_value_neg = other_value.native_flag
    ? (other_value.val.native < 0) : BN_is_negative(other_value.val.openssl);
  if (!this_neg && other_value_neg) return operator+(-other_value);
  if (this_neg && !other_value_neg) return -other_value.operator+(-(*this));
  if (likely(native_flag)) {
    if (likely(other_value.native_flag)) {
      // Since both operands are non-negative the most negative result of a
      // subtraction can be -INT_MAX and according to limits.h:
      // # define INT_MIN (-INT_MAX - 1)
      return INTEGER(val.native - other_value.val.native);
    } else {
      BIGNUM *this_int = to_openssl(val.native);
      BN_sub(this_int, this_int, other_value.val.openssl);
      // The result can be small enough to fit in int.  Back conversion is a
      // costly operation using strings all the time.
      if (BN_num_bits(this_int) <= (int)sizeof(int) * 8 - 1) {
        char *result_str = BN_bn2dec(this_int);
        RInt result = string2RInt(result_str);
        OPENSSL_free(result_str);
        BN_free(this_int);
        return INTEGER(result);
      } else {
        return INTEGER(this_int);
      }
    }
  } else {
    BIGNUM *result = BN_new();
    BIGNUM *other_value_int = other_value.native_flag ?
      to_openssl(other_value.val.native) : other_value.val.openssl;
    BN_sub(result, val.openssl, other_value_int);
    if (other_value.native_flag) BN_free(other_value_int);
    if (BN_num_bits(result) <= (int)sizeof(int) * 8 - 1) {
      char *result_str = BN_bn2dec(result);
      RInt result_int = string2RInt(result_str);
      OPENSSL_free(result_str);
      BN_free(result);
      return INTEGER(result_int);
    } else {
      return INTEGER(result);
    }
  }
}

INTEGER INTEGER::operator*(int other_value) const
{
  must_bound("Unbound left operand of integer multiplication.");
  return *this * INTEGER(other_value);
}

INTEGER INTEGER::operator*(const INTEGER& other_value) const
{
  must_bound("Unbound left operand of integer multiplication.");
  other_value.must_bound("Unbound right operand of integer multiplication.");
  if ((native_flag && val.native == 0) || (other_value.native_flag &&
    other_value.val.native == 0)) return INTEGER((int)0);
  if (likely(native_flag)) {
    if (likely(other_value.native_flag)) {
      // TODO: Improve.
      if (likely(abs(val.native) < 32768 && abs(other_value.val.native) < 32768)) {
    	return INTEGER(val.native * other_value.val.native);
      } else {
        BIGNUM *this_int = to_openssl(val.native);
        BIGNUM *other_value_int = to_openssl(other_value.val.native);
        BN_CTX *ctx = BN_CTX_new();
        BN_mul(this_int, this_int, other_value_int, ctx);
        BN_CTX_free(ctx);
        BN_free(other_value_int);
        if (BN_num_bits(this_int) < (int)sizeof(int) * 8) {
          BN_free(this_int);
          return INTEGER(val.native * other_value.val.native);
        } else {
          return INTEGER(this_int);
        }
      }
    } else {
      BIGNUM *this_int = to_openssl(val.native);
      BN_CTX *ctx = BN_CTX_new();
      BN_mul(this_int, this_int, other_value.val.openssl, ctx);
      BN_CTX_free(ctx);
      return INTEGER(this_int);
    }
  } else {
    BIGNUM *result = BN_new();
    BIGNUM *other_value_int = NULL;
    BN_CTX *ctx = BN_CTX_new();
    other_value_int = other_value.native_flag
      ? to_openssl(other_value.val.native) : other_value.val.openssl;
    BN_mul(result, val.openssl, other_value_int, ctx);
    BN_CTX_free(ctx);
    if (likely(other_value.native_flag)) BN_free(other_value_int);
    return INTEGER(result);
  }
}

INTEGER INTEGER::operator/(int other_value) const
{
  must_bound("Unbound left operand of integer division.");
  if (other_value == 0) TTCN_error("Integer division by zero.");
  return *this / INTEGER(other_value);
}

INTEGER INTEGER::operator/(const INTEGER& other_value) const
{
  must_bound("Unbound left operand of integer division.");
  other_value.must_bound("Unbound right operand of integer division.");
  if (other_value == 0) TTCN_error("Integer division by zero.");
  if (native_flag && val.native == 0) return INTEGER((int)0);
  if (likely(native_flag)) {
    if (likely(other_value.native_flag)) {
      return INTEGER(val.native / other_value.val.native);
    } else {
      BIGNUM *this_int = to_openssl(val.native);
      BN_CTX *ctx = BN_CTX_new();
      BN_div(this_int, NULL, this_int, other_value.val.openssl, ctx);
      BN_CTX_free(ctx);
      if (BN_num_bits(this_int) <= (int)sizeof(int) * 8 - 1) {
        char *result_str = BN_bn2dec(this_int);
        RInt result = string2RInt(result_str);
        OPENSSL_free(result_str);
        BN_free(this_int);
        return INTEGER(result);
      } else {
        return INTEGER(this_int);
      }
    }
  } else {
    BIGNUM *result = BN_new();
    BIGNUM *other_value_int = NULL;
    BN_CTX *ctx = BN_CTX_new();
    other_value_int = other_value.native_flag
      ? to_openssl(other_value.val.native) : other_value.val.openssl;
    BN_div(result, NULL, val.openssl, other_value_int, ctx);
    if (likely(other_value.native_flag)) BN_free(other_value_int);
    BN_CTX_free(ctx);
    if (BN_num_bits(result) <= (int)sizeof(int) * 8 - 1) {
      char *result_str = BN_bn2dec(result);
      RInt result_i = string2RInt(result_str);
      OPENSSL_free(result_str);
      BN_free(result);
      return INTEGER(result_i);
    } else {
      return INTEGER(result);
    }
  }
}

boolean INTEGER::operator==(int other_value) const
{
  must_bound("Unbound left operand of integer comparison.");
  if (likely(native_flag)) {
    return val.native == other_value;
  } else {
    BIGNUM *other_value_int = to_openssl(other_value);
    int equal = BN_cmp(val.openssl, other_value_int);
    BN_free(other_value_int);
    return equal == 0;
  }
}

boolean INTEGER::operator==(const INTEGER& other_value) const
{
  must_bound("Unbound left operand of integer comparison.");
  other_value.must_bound("Unbound right operand of integer comparison.");
  if (likely(native_flag)) {
    if (likely(other_value.native_flag)) {
      return val.native == other_value.val.native;
    } else {
      BIGNUM *this_int = to_openssl(val.native);
      int equal = BN_cmp(this_int, other_value.val.openssl);
      BN_free(this_int);
      return equal == 0;
    }
  } else {
    if (likely(other_value.native_flag)) {
      BIGNUM *other_value_int = to_openssl(other_value.val.native);
      int equal = BN_cmp(val.openssl, other_value_int);
      BN_free(other_value_int);
      return equal == 0;
    } else {
      return BN_cmp(val.openssl, other_value.val.openssl) == 0;
    }
  }
}

boolean INTEGER::operator<(int other_value) const
{
  must_bound("Unbound left operand of integer comparison.");
  return *this < INTEGER(other_value);
}

boolean INTEGER::operator<(const INTEGER& other_value) const
{
  must_bound("Unbound left operand of integer comparison.");
  other_value.must_bound("Unbound right operand of integer comparison.");
  if (likely(native_flag)) {
    if (likely(other_value.native_flag)) {
      return val.native < other_value.val.native;
    } else {
      BIGNUM *this_int = to_openssl(val.native);
      int equal = BN_cmp(this_int, other_value.val.openssl);
      BN_free(this_int);
      return equal == -1;
    }
  } else {
    if (likely(other_value.native_flag)) {
      BIGNUM *other_value_int = to_openssl(other_value.val.native);
      int equal = BN_cmp(val.openssl, other_value_int);
      BN_free(other_value_int);
      return equal == -1;
    } else {
      return BN_cmp(val.openssl, other_value.val.openssl) == -1;
    }
  }
}

boolean INTEGER::operator>(int other_value) const
{
  must_bound("Unbound left operand of integer comparison.");
  return *this > INTEGER(other_value);
}

boolean INTEGER::operator>(const INTEGER& other_value) const
{
  // A simple call to operator< and operator== would be much simplier.
  must_bound("Unbound left operand of integer comparison.");
  other_value.must_bound("Unbound right operand of integer comparison.");
  if (likely(native_flag)) {
    if (likely(other_value.native_flag)) {
      return val.native > other_value.val.native;
    } else {
      BIGNUM *this_int = to_openssl(val.native);
      int equal = BN_cmp(this_int, other_value.val.openssl);
      BN_free(this_int);
      return equal == 1;
    }
  } else {
    if (likely(other_value.native_flag)) {
      BIGNUM *other_value_int = to_openssl(other_value.val.native);
      int equal = BN_cmp(val.openssl, other_value_int);
      BN_free(other_value_int);
      return equal == 1;
    } else {
      return BN_cmp(val.openssl, other_value.val.openssl) == 1;
    }
  }
}

INTEGER::operator int() const
{
  must_bound("Using the value of an unbound integer variable.");
  if (unlikely(!native_flag))
    TTCN_error("Invalid conversion of a large integer value");
  return val.native;
}

// To avoid ambiguity we have a separate function to convert our INTEGER
// object to long long.
long long int INTEGER::get_long_long_val() const
{
  must_bound("Using the value of an unbound integer variable.");
  if (likely(native_flag)) return val.native;
  boolean is_negative = BN_is_negative(val.openssl);
  long long int ret_val = 0;
  if (unlikely(BN_is_zero(val.openssl))) return 0;
  // It feels so bad accessing a BIGNUM directly, but faster than string
  // conversion...
  // I know, I had to fix this... Bence
  if ((size_t)BN_num_bytes(val.openssl) <= sizeof(BN_ULONG)) {
    return !is_negative ? BN_get_word(val.openssl) : -BN_get_word(val.openssl);
  }
  
  int num_bytes = BN_num_bytes(val.openssl);
  unsigned char* tmp = (unsigned char*)Malloc(num_bytes * sizeof(unsigned char));
  BN_bn2bin(val.openssl, tmp);
  ret_val = tmp[0] & 0xff;
  for (int i = 1; i < num_bytes; i++) {
    ret_val <<= 8;
    ret_val += tmp[i] & 0xff;
  }
  Free(tmp);
  return !is_negative ? ret_val : -ret_val;
}

void INTEGER::set_long_long_val(long long int other_value)
{
  clean_up();
  bound_flag = TRUE;
  // Seems to be a native.  It's very strange if someone calls this with a
  // small number.  A simple assignment should be used for such values.
  if (unlikely((RInt)other_value == other_value)) {
    native_flag = TRUE;
    val.native = other_value;
    return;
  }
  native_flag = FALSE;
  val.openssl = BN_new();
  // Make it 0.
  BN_zero(val.openssl);
  boolean is_negative = other_value < 0;
  unsigned long long int tmp = !is_negative ? other_value : -other_value;
  for (int i = sizeof(long long int) - 1; i >= 0; i--) {
    BN_add_word(val.openssl, (tmp >> 8 * i) & 0xff);
    if (i) BN_lshift(val.openssl, val.openssl, 8);
  }
  BN_set_negative(val.openssl, is_negative ? 1 : 0);
}

void INTEGER::log() const
{
  if (likely(bound_flag)) {
    if (likely(native_flag)) {
      TTCN_Logger::log_event("%d", val.native);
    } else {
      char *tmp = BN_bn2dec(val.openssl);
      TTCN_Logger::log_event("%s", tmp);
      OPENSSL_free(tmp);
    }
  } else {
    TTCN_Logger::log_event_unbound();
  }
}

void INTEGER::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_VALUE, "integer value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Integer: {
    clean_up();
    bound_flag = TRUE;
    const int_val_t* const int_val = mp->get_integer();
    native_flag = int_val->is_native();
    if (likely(native_flag)){
      val.native = int_val->get_val();
    } else {
      val.openssl = BN_dup(int_val->get_val_openssl());
    }
    break; }
  case Module_Param::MP_Expression:
    switch (mp->get_expr_type()) {
    case Module_Param::EXPR_NEGATE: {
      INTEGER operand;
      operand.set_param(*mp->get_operand1());
      *this = - operand;
      break; }
    case Module_Param::EXPR_ADD: {
      INTEGER operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 + operand2;
      break; }
    case Module_Param::EXPR_SUBTRACT: {
      INTEGER operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 - operand2;
      break; }
    case Module_Param::EXPR_MULTIPLY: {
      INTEGER operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 * operand2;
      break; }
    case Module_Param::EXPR_DIVIDE: {
      INTEGER operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      if (operand2 == 0) {
        param.error("Integer division by zero.");
      }
      *this = operand1 / operand2;
      break; }
    default:
      param.expr_type_error("an integer");
      break;
    }
    break;
  default:
    param.type_error("integer value");
    break;
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* INTEGER::get_param(Module_Param_Name& /* param_name */) const
{
  if (!bound_flag) {
    return new Module_Param_Unbound();
  }
  if (native_flag) {
    return new Module_Param_Integer(new int_val_t(val.native));
  }
  return new Module_Param_Integer(new int_val_t(BN_dup(val.openssl)));
}
#endif

void INTEGER::encode_text(Text_Buf& text_buf) const
{
  must_bound("Text encoder: Encoding an unbound integer value.");
  if (likely(native_flag)) {
    text_buf.push_int(val.native);
  } else {
    int_val_t *tmp = new int_val_t(BN_dup(val.openssl));
    text_buf.push_int(*tmp);
    delete tmp;
  }
}

void INTEGER::decode_text(Text_Buf& text_buf)
{
  clean_up();
  bound_flag = TRUE;
  int_val_t tmp(text_buf.pull_int());
  if (likely(tmp.native_flag)) {
    native_flag = TRUE;
    val.native = tmp.get_val();
  } else {
    native_flag = FALSE;
    val.openssl = BN_dup(tmp.get_val_openssl());
  }
}

void INTEGER::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
                     TTCN_EncDec::coding_t p_coding, ...) const
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-encoding type '%s': ", p_td.name);
    unsigned BER_coding=va_arg(pvar, unsigned);
    BER_encode_chk_coding(BER_coding);
    ASN_BER_TLV_t *tlv=BER_encode_TLV(p_td, BER_coding);
    tlv->put_in_buffer(p_buf);
    ASN_BER_TLV_t::destruct(tlv);
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-encoding type '%s': ", p_td.name);
    if(!p_td.raw)
    TTCN_EncDec_ErrorContext::error_internal
      ("No RAW descriptor available for type '%s'.", p_td.name);
    RAW_enc_tr_pos rp;
    rp.level=0;
    rp.pos=NULL;
    RAW_enc_tree root(TRUE,NULL,&rp,1,p_td.raw);
    RAW_encode(p_td, root);
    root.put_to_buf(p_buf);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    TTCN_EncDec_ErrorContext ec("While TEXT-encoding type '%s': ", p_td.name);
    if(!p_td.text)
      TTCN_EncDec_ErrorContext::error_internal
        ("No TEXT descriptor available for type '%s'.", p_td.name);
    TEXT_encode(p_td,p_buf);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-encoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XER_encode(*p_td.xer, p_buf, XER_coding, 0, 0, 0);
    break;}
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-encoding type '%s': ", p_td.name);
    if(!p_td.json)
      TTCN_EncDec_ErrorContext::error_internal
        ("No JSON descriptor available for type '%s'.", p_td.name);
    JSON_Tokenizer tok(va_arg(pvar, int) != 0);
    JSON_encode(p_td, tok);
    p_buf.put_s(tok.get_buffer_length(), (const unsigned char*)tok.get_buffer());
    break;}
  default:
    TTCN_error("Unknown coding method requested to encode type '%s'",
               p_td.name);
  }
  va_end(pvar);
}

void INTEGER::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
                     TTCN_EncDec::coding_t p_coding, ...)
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-decoding type '%s': ", p_td.name);
    unsigned L_form=va_arg(pvar, unsigned);
    ASN_BER_TLV_t tlv;
    BER_decode_str2TLV(p_buf, tlv, L_form);
    BER_decode_TLV(p_td, tlv, L_form);
    if(tlv.isComplete) p_buf.increase_pos(tlv.get_len());
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-decoding type '%s': ", p_td.name);
    if(!p_td.raw)
      TTCN_EncDec_ErrorContext::error_internal
        ("No RAW descriptor available for type '%s'.", p_td.name);
    raw_order_t order;
    switch(p_td.raw->top_bit_order){
    case TOP_BIT_LEFT:
      order=ORDER_LSB;
      break;
    case TOP_BIT_RIGHT:
    default:
      order=ORDER_MSB;
    }
    if(RAW_decode(p_td, p_buf, p_buf.get_len()*8, order)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
        "Can not decode type '%s', because invalid or incomplete"
        " message was received"
        , p_td.name);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    Limit_Token_List limit;
    TTCN_EncDec_ErrorContext ec("While TEXT-decoding type '%s': ", p_td.name);
    if(!p_td.text)
      TTCN_EncDec_ErrorContext::error_internal
        ("No TEXT descriptor available for type '%s'.", p_td.name);
    const unsigned char *b=p_buf.get_data();
    if(b[p_buf.get_len()-1]!='\0'){
      p_buf.set_pos(p_buf.get_len());
      p_buf.put_zero(8,ORDER_LSB);
      p_buf.rewind();
    }
    if(TEXT_decode(p_td,p_buf,limit)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
               "Can not decode type '%s', because invalid or incomplete"
               " message was received"
               , p_td.name);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-decoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XmlReaderWrap reader(p_buf);
    for (int success = reader.Read(); success==1; success=reader.Read()) {
      int type = reader.NodeType();
      if (type==XML_READER_TYPE_ELEMENT)
        break;
    }
    XER_decode(*p_td.xer, reader, XER_coding, XER_NONE, 0);
    size_t bytes = reader.ByteConsumed();
    p_buf.set_pos(bytes);
    break;}
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-decoding type '%s': ", p_td.name);
    if(!p_td.json)
      TTCN_EncDec_ErrorContext::error_internal
        ("No JSON descriptor available for type '%s'.", p_td.name);
    JSON_Tokenizer tok((const char*)p_buf.get_data(), p_buf.get_len());
    if(JSON_decode(p_td, tok, FALSE)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
               "Can not decode type '%s', because invalid or incomplete"
               " message was received"
               , p_td.name);
    p_buf.set_pos(tok.get_buf_pos());
    break;}
  default:
    TTCN_error("Unknown coding method requested to decode type '%s'",
               p_td.name);
  }
  va_end(pvar);
}

ASN_BER_TLV_t *INTEGER::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                        unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv = BER_encode_chk_bound(is_bound());
  if (!new_tlv) {
    if (native_flag) {
      new_tlv = BER_encode_TLV_INTEGER(p_coding, val.native);
    } else {
      // Temporary.
      int_val_t *tmp = new int_val_t(BN_dup(val.openssl));
      new_tlv = BER_encode_TLV_INTEGER(p_coding, *tmp);
      delete tmp;
    }
  }
  new_tlv = ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean INTEGER::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                                const ASN_BER_TLV_t& p_tlv,
                                unsigned L_form)
{
  clean_up();
  bound_flag = FALSE;
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec("While decoding INTEGER type: ");
  int_val_t tmp;
  boolean ret_val = BER_decode_TLV_INTEGER(stripped_tlv, L_form, tmp);
  if (tmp.is_native()) {
    native_flag = TRUE;
    val.native = tmp.get_val();
  } else {
    native_flag = FALSE;
    val.openssl = BN_dup(tmp.get_val_openssl());
  }
  if (ret_val) bound_flag = TRUE;
  return ret_val;
}

int INTEGER::TEXT_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff, Limit_Token_List& limit, boolean no_err, boolean /*first_call*/)
{
  int decoded_length = 0;
  int str_len = 0;
  if (p_td.text->begin_decode) {
    int tl;
    if ((tl = p_td.text->begin_decode->match_begin(buff)) < 0) {
      if (no_err) return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "The specified token '%s' not found for '%s': ",
        (const char*)*(p_td.text->begin_decode), p_td.name);
      return 0;
    }
    decoded_length += tl;
    buff.increase_pos(tl);
  }
  if (buff.get_read_len() <= 1 && no_err) return -TTCN_EncDec::ET_LEN_ERR;
  if (p_td.text->select_token) {
    int tl;
    if ((tl = p_td.text->select_token->match_begin(buff)) < 0) {
      if (no_err) return -1;
      else tl = 0;
    }
    str_len = tl;
  } else if ( p_td.text->val.parameters
    &&        p_td.text->val.parameters->decoding_params.min_length != -1) {
    str_len = p_td.text->val.parameters->decoding_params.min_length;
  } else if (p_td.text->end_decode) {
    int tl;
    if ((tl = p_td.text->end_decode->match_first(buff)) < 0) {
      if (no_err) return -1;
      else tl = 0;
    }
    str_len = tl;
  } else if (limit.has_token()) {
    int tl;
    if ((tl = limit.match(buff)) < 0) {
      if (no_err) return -1;
      else tl = 0;
    }
    str_len = tl;
  } else {
    int tl;
    if ((tl = integer_value_match.match_begin(buff)) < 0) {
      if (no_err) return -1;
      else tl = 0;
    }
    str_len = tl;
  }
  boolean err = FALSE;
  if (str_len > 0) {
    int offs = 0;
    char *atm = (char*)Malloc(str_len + 1); // sizeof(char) == 1 by definition
    const char *b = (const char*)buff.get_read_data();
    memcpy(atm, b, str_len);
    atm[str_len] = 0;
    // 0x2d ('-')
    // 0x20 (' ')
    // 0x30 ('0')
    int neg = *atm == 0x2d ? 1 : 0;
    if (!*(atm + neg)) {
      for (offs = neg; *(atm + offs) == 0x30; offs++) ;  // E.g. 0, -0, 00001234, -00001234.
      if (neg && offs > 1) *(atm + offs - 1) = *atm;  // E.g. -00001234 -> -000-1234.
      offs -= neg;
    } else {
      for(; atm[offs] == 0x20; offs++) ;
    }
    clean_up();
    if (0 == strlen(atm + offs) || 0 == from_string(atm+offs)) {
      err = TRUE;
      native_flag = TRUE;
      val.native = 0;
    }
    Free(atm);
    buff.increase_pos(str_len);
    decoded_length += str_len;
  } else {
    err = TRUE;
  }
  if (err) {
    if (no_err) return -1;
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
      "Can not decode a valid integer for '%s': ", p_td.name);
  }
  if (p_td.text->end_decode) {
    int tl;
    if ((tl = p_td.text->end_decode->match_begin(buff)) < 0) {
      if (no_err) return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "The specified token '%s' not found for '%s': ",
        (const char*)*(p_td.text->end_decode), p_td.name);
      return 0;
    }
    decoded_length += tl;
    buff.increase_pos(tl);
  }
  bound_flag = TRUE;
  return decoded_length;
}

int INTEGER::TEXT_encode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff) const
{
  int encoded_length = 0;
  if (p_td.text->begin_encode) {
    buff.put_cs(*p_td.text->begin_encode);
    encoded_length += p_td.text->begin_encode->lengthof();
  }
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND,"Encoding an unbound value.");
    if (p_td.text->end_encode) {
      buff.put_cs(*p_td.text->end_encode);
      encoded_length+=p_td.text->end_encode->lengthof();
    }
    return encoded_length;
  }
  char *tmp_str;
  // Temporary.
  if (native_flag) tmp_str = mprintf("%d", val.native);
  else tmp_str = BN_bn2dec(val.openssl);
  CHARSTRING ch(tmp_str);
  if (native_flag) Free(tmp_str);
  else OPENSSL_free(tmp_str);
  if (p_td.text->val.parameters == NULL) {
    buff.put_cs(ch);
    encoded_length += ch.lengthof();
  } else {
    TTCN_TEXTdescriptor_values params = p_td.text->val.parameters
      ->coding_params;
    if (params.min_length < 0) {
      buff.put_cs(ch);
      encoded_length += ch.lengthof();
    } else {
      unsigned char *p = NULL;
      int a = 0;
      size_t len = params.min_length + 1;
      buff.get_end(p, len);
      if (params.leading_zero) {
        if (native_flag) {
          a = snprintf((char*)p, len, "%0*d", params.min_length, val.native);
        } else {
          int chlen = ch.lengthof(), pad = 0;
          int neg = native_flag ? (val.native < 0) : BN_is_negative(val.openssl);
          if (params.min_length > chlen)
            pad = params.min_length - chlen + neg;
          // `sprintf' style.
          if (neg) *p = 0x2d;
          memset(p + neg, 0x30, pad);
          for (int i = 0; i < chlen - neg; i++)
            p[i + pad] = ch[i + neg].get_char();
          a += pad + chlen - neg;
        }
      } else {
        a = snprintf((char*)p, len, "%*s", p_td.text->val.parameters->
          coding_params.min_length, (const char*)ch);
      }
      buff.increase_length(a);
      encoded_length += a;
    }
  }
  if (p_td.text->end_encode) {
    buff.put_cs(*p_td.text->end_encode);
    encoded_length += p_td.text->end_encode->lengthof();
  }
  return encoded_length;
}

unsigned char INTX_MASKS[] = { 0 /*dummy*/, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };

int INTEGER::RAW_encode(const TTCN_Typedescriptor_t& p_td, RAW_enc_tree& myleaf) const
{
  if (!native_flag) return RAW_encode_openssl(p_td, myleaf);
  unsigned char *bc;
  int length; // total length, in bytes
  int val_bits = 0, len_bits = 0; // only for IntX
  int value = val.native;
  boolean neg_sgbit = (value < 0) && (p_td.raw->comp == SG_SG_BIT);
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
    value = 0;
    neg_sgbit = FALSE;
  }
  if (value != 0 && value == -value) {
    // value == -INT_MAX-1 a.k.a. INT_MIN a.k.a. 0x8000....
    INTEGER big_value(to_openssl(val.native)); // too big for native
    return big_value.RAW_encode_openssl(p_td, myleaf);
  }
  if ((value < 0) && (p_td.raw->comp == SG_NO)) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_SIGN_ERR,
      "Unsigned encoding of a negative number: %s", p_td.name);
    value = -value;
  }
  if (neg_sgbit) value = -value;
  //myleaf.ext_bit=EXT_BIT_NO;
  if (myleaf.must_free) Free(myleaf.body.leaf.data_ptr);
  if (p_td.raw->fieldlength == RAW_INTX) { // IntX (variable length)
    val_bits = (p_td.raw->comp != SG_NO); // bits needed to store the value
    int v2 = value;
    if (v2 < 0 && p_td.raw->comp == SG_2COMPL) {
      v2 = ~v2;
    }
    do {
      v2 >>= 1;
      ++val_bits;
    }
    while (v2 != 0);
    len_bits = 1 + val_bits / 8; // bits needed to store the length
    if (val_bits % 8 + len_bits % 8 > 8) {
      // the remainder of the value bits and the length bits do not fit into
      // an octet => an extra octet is needed and the length must be increased
      ++len_bits;
    }
    length = (len_bits + val_bits + 7) / 8;
    if (len_bits % 8 == 0 && val_bits % 8 != 0) {
      // special case: the value can be stored on 8k - 1 octets plus the partial octet
      // - len_bits = 8k is not enough, since there's no partial octet in that case
      // and the length would then be followed by 8k octets (and it only indicates
      // 8k - 1 further octets)
      // - len_bits = 8k + 1 is too much, since there are only 8k - 1 octets
      // following the partial octet (and 8k are indicated)
      // solution: len_bits = 8k + 1 and insert an extra empty octet
      ++len_bits;
      ++length;
    }
  }
  else { // not IntX, use the field length
    length = (p_td.raw->fieldlength + 7) / 8;
    if (min_bits(value) > p_td.raw->fieldlength) {
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
        "There are insufficient bits to encode '%s' : ", p_td.name);
      value = 0; // substitute with zero
    }
  }
  if (length > RAW_INT_ENC_LENGTH) { // does not fit in the small buffer
    myleaf.body.leaf.data_ptr = bc = (unsigned char*)Malloc(length * sizeof(*bc));
    myleaf.must_free = TRUE;
    myleaf.data_ptr_used = TRUE;
  }
  else bc = myleaf.body.leaf.data_array;
  if (p_td.raw->fieldlength == RAW_INTX) {
    int i = 0;
    // treat the empty space between the value and the length as if it was part
    // of the value, too
    val_bits = length * 8 - len_bits;
    // first, encode the value
    do {
      bc[i] = value & INTX_MASKS[val_bits > 8 ? 8 : val_bits];
      ++i;
      value >>= 8;
      val_bits -= 8;
    }
    while (val_bits > 0);
    if (neg_sgbit) {
      // the sign bit is the first bit after the length
      unsigned char mask = 0x80 >> len_bits % 8;
      bc[i - 1] |= mask;
    }
    // second, encode the length (ignore the last zero)
    --len_bits;
    if (val_bits != 0) {
      // the remainder of the length is in the same octet as the remainder of the
      // value => step back onto it
      --i;
    }
    else {
      // the remainder of the length is in a separate octet
      bc[i] = 0;
    }
    // insert the length's partial octet
    unsigned char mask = 0x80;
    for (int j = 0; j < len_bits % 8; ++j) {
      bc[i] |= mask;
      mask >>= 1;
    }
    if (len_bits % 8 > 0 || val_bits != 0) {
      // there was a partial octet => step onto the first full octet
      ++i;
    }
    // insert the length's full octets
    while (len_bits >= 8) {
      // octets containing only ones in the length
      bc[i] = 0xFF;
      ++i;
      len_bits -= 8;
    }
    myleaf.length = length * 8;
  }
  else {
    for (int a = 0; a < length; a++) {
      bc[a] = value & 0xFF;
      value >>= 8;
    }
    if (neg_sgbit) {
      unsigned char mask = 0x01 << (p_td.raw->fieldlength - 1) % 8;
      bc[length - 1] |= mask;
    }
    myleaf.length = p_td.raw->fieldlength;
  }
  return myleaf.length;
}

int INTEGER::RAW_encode_openssl(const TTCN_Typedescriptor_t& p_td,
  RAW_enc_tree& myleaf) const
{
  unsigned char *bc = NULL;
  int length; // total length, in bytes
  int val_bits = 0, len_bits = 0; // only for IntX
  BIGNUM *D = BN_new();
  BN_copy(D, val.openssl);
  boolean neg_sgbit = (BN_is_negative(D)) && (p_td.raw->comp == SG_SG_BIT);
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
    BN_clear(D);
    neg_sgbit = FALSE;
  }
  if ((BN_is_negative(D)) && (p_td.raw->comp == SG_NO)) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_SIGN_ERR,
      "Unsigned encoding of a negative number: %s", p_td.name);
    BN_set_negative(D, 0);
    neg_sgbit = FALSE;
  }
  // `if (neg_sgbit) tmp->neg = tmp->neg == 0;' is not needed, because the
  // sign is stored separately from the number.  Default encoding of negative
  // values in 2's complement form.
  if (myleaf.must_free) Free(myleaf.body.leaf.data_ptr);
  if (p_td.raw->fieldlength == RAW_INTX) {
    val_bits = BN_num_bits(D) + (p_td.raw->comp != SG_NO); // bits needed to store the value
    len_bits = 1 + val_bits / 8; // bits needed to store the length
    if (val_bits % 8 + len_bits % 8 > 8) {
      // the remainder of the value bits and the length bits do not fit into
      // an octet => an extra octet is needed and the length must be increased
      ++len_bits;
    }
    length = (len_bits + val_bits + 7) / 8;
    if (len_bits % 8 == 0 && val_bits % 8 != 0) {
      // special case: the value can be stored on 8k - 1 octets plus the partial octet
      // - len_bits = 8k is not enough, since there's no partial octet in that case
      // and the length would then be followed by 8k octets (and it only indicates
      // 8k - 1 further octets)
      // - len_bits = 8k + 1 is too much, since there are only 8k - 1 octets
      // following the partial octet (and 8k are indicated)
      // solution: len_bits = 8k + 1 and insert an extra empty octet
      ++len_bits;
      ++length;
    }
  }
  else {
    length = (p_td.raw->fieldlength + 7) / 8;
    if (min_bits(D) > p_td.raw->fieldlength) {
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
        "There are insufficient bits to encode '%s': ", p_td.name);
      // `tmp = -((-tmp) & BitMaskTable[min_bits(tmp)]);' doesn't make any sense
      // at all for negative values.  Just simply clear the value.
      BN_clear(D);
      neg_sgbit = FALSE;
    }
  }
  if (length > RAW_INT_ENC_LENGTH) {
    myleaf.body.leaf.data_ptr = bc =
      (unsigned char *)Malloc(length * sizeof(*bc));
    myleaf.must_free = TRUE;
    myleaf.data_ptr_used = TRUE;
  } else {
    bc = myleaf.body.leaf.data_array;
  }
  boolean twos_compl = (BN_is_negative(D)) && !neg_sgbit;
  // Conversion to 2's complement.
  if (twos_compl) {
    BN_set_negative(D, 0);
    int num_bytes = BN_num_bytes(D);
    unsigned char* tmp = (unsigned char*)Malloc(num_bytes * sizeof(unsigned char));
    BN_bn2bin(D, tmp);
    for (int a = 0; a < num_bytes; a++) tmp[a] = ~tmp[a];
    BN_bin2bn(tmp, num_bytes, D);
    BN_add_word(D, 1);
    Free(tmp);
  }
  if (p_td.raw->fieldlength == RAW_INTX) {
    int i = 0;
    // treat the empty space between the value and the length as if it was part
    // of the value, too
    val_bits = length * 8 - len_bits;
    // first, encode the value
    unsigned num_bytes = BN_num_bytes(D);
    unsigned char* tmp = (unsigned char*)Malloc(num_bytes * sizeof(unsigned char));
    BN_bn2bin(D, tmp);
    do {
      bc[i] = (num_bytes-i > 0 ? tmp[num_bytes - (i + 1)] : (twos_compl ? 0xFF : 0)) & INTX_MASKS[val_bits > 8 ? 8 : val_bits];
      ++i;
      val_bits -= 8;
    }
    while (val_bits > 0);
    Free(tmp);
    BN_free(D);
    if (neg_sgbit) {
      // the sign bit is the first bit after the length
      unsigned char mask = 0x80 >> len_bits % 8;
      bc[i - 1] |= mask;
    }
    // second, encode the length (ignore the last zero)
    --len_bits;
    if (val_bits != 0) {
      // the remainder of the length is in the same octet as the remainder of the
      // value => step back onto it
      --i;
    }
    else {
      // the remainder of the length is in a separate octet
      bc[i] = 0;
    }
    // insert the length's partial octet
    unsigned char mask = 0x80;
    for (int j = 0; j < len_bits % 8; ++j) {
      bc[i] |= mask;
      mask >>= 1;
    }
    if (len_bits % 8 > 0 || val_bits != 0) {
      // there was a partial octet => step onto the first full octet
      ++i;
    }
    // insert the length's full octets
    while (len_bits >= 8) {
      // octets containing only ones in the length
      bc[i] = 0xFF;
      ++i;
      len_bits -= 8;
    }
    myleaf.length = length * 8;
  }
  else {
    int num_bytes = BN_num_bytes(D);
    unsigned char* tmp = (unsigned char*)Malloc(num_bytes * sizeof(unsigned char));
    BN_bn2bin(D, tmp);
    for (int a = 0; a < length; a++) {
      if (twos_compl && num_bytes - 1 < a) bc[a] = 0xff;
      else bc[a] = (num_bytes - a > 0 ? tmp[num_bytes - (a + 1)] : 0) & 0xff;
    }
    if (neg_sgbit) {
      unsigned char mask = 0x01 << (p_td.raw->fieldlength - 1) % 8;
      bc[length - 1] |= mask;
    }
    Free(tmp);
    BN_free(D);
    myleaf.length = p_td.raw->fieldlength;
  }
  return myleaf.length;
}

int INTEGER::RAW_decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff,
  int limit, raw_order_t top_bit_ord, boolean no_err, int /*sel_field*/,
  boolean /*first_call*/)
{
  bound_flag = FALSE;
  int prepaddlength = buff.increase_pos_padd(p_td.raw->prepadding);
  limit -= prepaddlength;
  RAW_coding_par cp;
  boolean orders = FALSE;
  if (p_td.raw->bitorderinoctet == ORDER_MSB) orders = TRUE;
  if (p_td.raw->bitorderinfield == ORDER_MSB) orders = !orders;
  cp.bitorder = orders ? ORDER_MSB : ORDER_LSB;
  orders = FALSE;
  if (p_td.raw->byteorder == ORDER_MSB) orders = TRUE;
  if (p_td.raw->bitorderinfield == ORDER_MSB) orders = !orders;
  cp.byteorder = orders ? ORDER_MSB : ORDER_LSB;
  cp.fieldorder = p_td.raw->fieldorder;
  cp.hexorder = ORDER_LSB;
  int decode_length = 0;
  int len_bits = 0; // only for IntX (amount of bits used to store the length)
  unsigned char len_data = 0; // only for IntX (an octet used to store the length)
  int partial_octet_bits = 0; // only for IntX (amount of value bits in the partial octet)
  if (p_td.raw->fieldlength == RAW_INTX) {
    // extract the length
    do {
      // check if at least 8 bits are available in the buffer
      if (8 > limit) {
        if (!no_err) {
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
            "There are not enough bits in the buffer to decode the length of IntX "
            "type %s (needed: %d, found: %d).", p_td.name, len_bits + 8,
            len_bits + limit);
        }
        return -TTCN_EncDec::ET_LEN_ERR; 
      }
      else {
        limit -= 8;
      }
      int nof_unread_bits = buff.unread_len_bit();
      if (nof_unread_bits < 8) {
        if (!no_err) {
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INCOMPL_MSG,
            "There are not enough bits in the buffer to decode the length of IntX "
            "type %s (needed: %d, found: %d).", p_td.name, len_bits + 8,
            len_bits + nof_unread_bits);
        }
        return -TTCN_EncDec::ET_INCOMPL_MSG;
      }
      
      // extract the next length octet (or partial length octet)
      buff.get_b(8, &len_data, cp, top_bit_ord);
      unsigned char mask = 0x80;
      do {
        ++len_bits;
        if (len_data & mask) {
          mask >>= 1;
        }
        else {
          // the first zero signals the end of the length
          // the rest of the bits in the octet are part of the value
          partial_octet_bits = (8 - len_bits % 8) % 8;
          
          // decode_length only stores the amount of bits in full octets needed
          // by the value, the bits in the partial octet are stored by len_data
          decode_length = 8 * (len_bits - 1);
          break;
        }
      }
      while (len_bits % 8 != 0);
    }
    while (decode_length == 0 && partial_octet_bits == 0);
  }
  else {
    // not IntX, use the static field length
    decode_length = p_td.raw->fieldlength;
  }
  if (decode_length > limit) {
    if (!no_err) {
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_LEN_ERR,
        "There are not enough bits in the buffer to decode%s type %s (needed: %d, "
        "found: %d).", p_td.raw->fieldlength == RAW_INTX ? " the value of IntX" : "",
        p_td.name, decode_length, limit);
    }
    if (no_err || p_td.raw->fieldlength == RAW_INTX) {
      return -TTCN_EncDec::ET_LEN_ERR;
    }
    decode_length = limit;
  }
  int nof_unread_bits = buff.unread_len_bit();
  if (decode_length > nof_unread_bits) {
    if (!no_err) {
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INCOMPL_MSG,
        "There are not enough bits in the buffer to decode%s type %s (needed: %d, "
        "found: %d).", p_td.raw->fieldlength == RAW_INTX ? " the value of IntX" : "",
        p_td.name, decode_length, nof_unread_bits);
    }
    if (no_err || p_td.raw->fieldlength == RAW_INTX) {
      return -TTCN_EncDec::ET_INCOMPL_MSG;
    }
    decode_length = nof_unread_bits;
  }
  clean_up();
  if (decode_length < 0) return -1;
  else if (decode_length == 0 && partial_octet_bits == 0) {
    native_flag = TRUE;
    val.native = 0;
  }
  else {
    int tmp = 0;
    int twos_compl = 0;
    unsigned char *data = (unsigned char *) Malloc(
      (decode_length + partial_octet_bits + 7) / 8);
    buff.get_b((size_t) decode_length, data, cp, top_bit_ord);
    if (partial_octet_bits != 0) {
      // in case there are value bits in the last length octet (only for IntX),
      // these need to be appended to the extracted data
      data[decode_length / 8] = len_data;
      decode_length += partial_octet_bits;
    }
    int end_pos = decode_length;
    int idx = (end_pos - 1) / 8;
    boolean negativ_num = FALSE;
    switch (p_td.raw->comp) {
    case SG_2COMPL:
      if (data[idx] >> ((end_pos - 1) % 8) & 0x01) {
        tmp = -1;
        twos_compl = 1;
      }
      break;
    case SG_NO:
      break;
    case SG_SG_BIT:
      negativ_num = (data[idx] >> ((end_pos - 1) % 8)) & 0x01;
      end_pos--;
      break;
    default:
      break;
    }
    if (end_pos < 9) {
      tmp <<= end_pos;
      tmp |= data[0] & BitMaskTable[end_pos];
    }
    else {
      idx = (end_pos - 1) / 8;
      tmp <<= (end_pos - 1) % 8 + 1;
      tmp |= data[idx--] & BitMaskTable[(end_pos - 1) % 8 + 1];
      if (decode_length > (RInt) sizeof(RInt) * 8 - 1) {
        BIGNUM *D = BN_new();
        BN_set_word(D, tmp);
        int pad = tmp == 0 ? 1 : 0;
        for (; idx >= 0; idx--) {
          if (pad && data[idx] != 0) {
            BN_set_word(D, data[idx] & 0xff);
            pad = 0;
            continue;
          }
          if (pad) continue;
          BN_lshift(D, D, 8);
          BN_add_word(D, data[idx] & 0xff);
        }
        if (twos_compl) {
          BIGNUM *D_tmp = BN_new();
          BN_set_bit(D_tmp, BN_num_bits(D));
          BN_sub(D, D, D_tmp);
          BN_free(D_tmp);
        }
        else if (negativ_num) {
          BN_set_negative(D, 1);
        }
        // Back to native.  "BN_num_bits(D) + BN_is_negative(D) >
        // (RInt)sizeof(RInt) * 8 - !BN_is_negative(D)" was an over-complicated
        // condition.
        if (BN_num_bits(D) > (RInt) sizeof(RInt) * 8 - 1) {
          native_flag = FALSE;
          val.openssl = D;
        }
        else {
          native_flag = TRUE;
          val.native = BN_is_negative(D) ? -BN_get_word(D) : BN_get_word(D);
          BN_free(D);
        }
        Free(data);
        goto end;
      }
      else {
        for (; idx >= 0; idx--) {
          tmp <<= 8;
          tmp |= data[idx] & 0xff;
        }
      }
    }
    Free(data);
    native_flag = TRUE;
    val.native = negativ_num ? (RInt) -tmp : (RInt) tmp;
  }
  end: decode_length += buff.increase_pos_padd(p_td.raw->padding);
  bound_flag = TRUE;
  return decode_length + prepaddlength + len_bits;
}

int INTEGER::XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
  unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound integer value.");
  }
  int encoded_length = (int) p_buf.get_len();

  flavor |= SIMPLE_TYPE;
  flavor &= ~XER_RECOF; // integer doesn't care
  if (begin_xml(p_td, p_buf, flavor, indent, FALSE) == -1) --encoded_length;

  char *tmp_str;
  if (native_flag)
    tmp_str = mprintf("%d", val.native);
  else
    tmp_str = BN_bn2dec(val.openssl);
  CHARSTRING value(tmp_str);
  if (native_flag)
    Free(tmp_str);
  else
    OPENSSL_free(tmp_str);
  p_buf.put_string(value);

  end_xml(p_td, p_buf, flavor, indent, FALSE);

  return (int) p_buf.get_len() - encoded_length;
}

int INTEGER::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
  unsigned int flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*)
{
  const boolean exer = is_exer(flavor);
  const char * value = 0;

  boolean own_tag = !(exer && (p_td.xer_bits & UNTAGGED)) && !is_exerlist(flavor);

  if (!own_tag) goto tagless;
  if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
    verify_name(reader, p_td, exer);
tagless:
    value = (const char *)reader.Value();

    for (; *value && isspace(*value); ++value) {}
    from_string(value);
    if (get_nof_digits() != (int)strlen(value) - (value[0] == '-') ? 1 : 0) {
      clean_up();
    }
    // Let the caller do reader.AdvanceAttribute();
  }
  else {
    int depth = -1, success = reader.Ok(), type;
    for (; success == 1; success = reader.Read()) {
      type = reader.NodeType();
      if (XML_READER_TYPE_ELEMENT == type) {
        // If our parent is optional and there is an unexpected tag then return and
        // we stay unbound.
        if ((flavor & XER_OPTIONAL) && !check_name((const char*)reader.LocalName(), p_td, exer)) {
          return -1;
        }
        verify_name(reader, p_td, exer);
        if (reader.IsEmptyElement()) {
          if (exer && p_td.dfeValue != 0) {
            *this = *static_cast<const INTEGER*> (p_td.dfeValue);
          }
          reader.Read();
          break;
        }
        depth = reader.Depth();
      }
      else if (XML_READER_TYPE_TEXT == type && depth != -1) {
        value = (const char*) reader.Value();
        for (; *value && isspace(*value); ++value) {}
        from_string(value);
        if (get_nof_digits() != (int)strlen(value) - (value[0] == '-') ? 1 : 0) {
          clean_up();
        }
      }
      else if (XML_READER_TYPE_END_ELEMENT == type) {
        verify_end(reader, p_td, depth, exer);
        if (!bound_flag && exer && p_td.dfeValue != 0) {
          *this = *static_cast<const INTEGER*> (p_td.dfeValue);
        }
        reader.Read();
        break;
      }
    } // next read
  } // if not attribute

  return 1;
}

int INTEGER::get_nof_digits()
{
  int digits = 0;
  if (native_flag) {
    RInt x = val.native;
    if (x == 0) return 1;
    if (x < 0) x = -x;
    while (x > 0) {
      ++digits;
      x /= 10;
    }
  } else {
    BIGNUM *x = BN_new();
    BN_copy(x, val.openssl);
    if (BN_is_zero(x)) return 1;
    BN_set_negative(x, 1);
    while (!BN_is_zero(x)) {
      ++digits;
      BN_div_word(x, 10);
    }
  }
  return digits;
}

int INTEGER::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound integer value.");
    return -1;
  }
  char* tmp_str = 0;
  if (native_flag) {
    tmp_str = mprintf("%d", val.native);
  } else {
    tmp_str = BN_bn2dec(val.openssl);
  }
  
  int enc_len = p_tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
  
  if (native_flag) {
    Free(tmp_str);
  } else {
    OPENSSL_free(tmp_str);
  }
  
  return enc_len;
}

int INTEGER::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
{
  json_token_t token = JSON_TOKEN_NONE;
  char* value = 0;
  size_t value_len = 0;
  size_t dec_len = 0;
  boolean use_default = p_td.json->default_value && 0 == p_tok.get_buffer_length();
  if (use_default) {
    // No JSON data in the buffer -> use default value
    value = const_cast<char*>(p_td.json->default_value);
    value_len = strlen(value);
  } else {
    dec_len = p_tok.get_next_token(&token, &value, &value_len);
  }
  if (JSON_TOKEN_ERROR == token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  else if (JSON_TOKEN_NUMBER == token || use_default) {
    char* number = mcopystrn(value, value_len);
    if (from_string(number) && (int)value_len == get_nof_digits() + ('-' == value[0] ? 1 : 0)) {
      bound_flag = TRUE;
    } else {
      JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FORMAT_ERROR, "number", "integer");
      bound_flag = FALSE;
      dec_len = JSON_ERROR_FATAL;
    }
    Free(number);
  } else {
    bound_flag = FALSE;
    return JSON_ERROR_INVALID_TOKEN;
  }
  return (int)dec_len;
}


// Global functions.

INTEGER operator+(int int_value, const INTEGER& other_value)
{
  other_value.must_bound("Unbound right operand of integer addition.");
  return INTEGER(int_value) + other_value;
}

INTEGER operator-(int int_value, const INTEGER& other_value)
{
  other_value.must_bound("Unbound right operand of integer subtraction.");
  return INTEGER(int_value) - other_value;
}

INTEGER operator*(int int_value, const INTEGER& other_value)
{
  other_value.must_bound("Unbound right operand of integer multiplication.");
  return INTEGER(int_value) * other_value;
}

INTEGER operator/(int int_value, const INTEGER& other_value)
{
  other_value.must_bound("Unbound right operand of integer division.");
  if (other_value.get_val() == 0)
    TTCN_error("Integer division by zero.");
  return INTEGER(int_value) / other_value;
}

INTEGER rem(int left_value, int right_value)
{
  if (right_value == 0)
    TTCN_error("The right operand of rem operator is zero.");
  return INTEGER(left_value - right_value * (left_value / right_value));
}

INTEGER rem(const INTEGER& left_value, const INTEGER& right_value)
{
  left_value.must_bound("Unbound left operand of rem operator.");
  right_value.must_bound("Unbound right operand of rem operator.");
  return left_value - right_value * (left_value / right_value);
}

INTEGER rem(const INTEGER& left_value, int right_value)
{
  left_value.must_bound("Unbound left operand of rem operator.");
  return rem(left_value, INTEGER(right_value));
}

INTEGER rem(int left_value, const INTEGER& right_value)
{
  right_value.must_bound("Unbound right operand of rem operator.");
  return rem(INTEGER(left_value), right_value);
}

INTEGER mod(int left_value, int right_value)
{
  if (right_value < 0) right_value = -right_value;
  else if (right_value == 0)
    TTCN_error("The right operand of mod operator is zero.");
  if (left_value > 0) return rem(left_value, right_value);
  else {
    int result = rem(left_value, right_value);
    if (result == 0) return 0;
    else return INTEGER(right_value + result);
  }
}

INTEGER mod(const INTEGER& left_value, const INTEGER& right_value)
{
  left_value.must_bound("Unbound left operand of mod operator.");
  right_value.must_bound("Unbound right operand of mod operator.");
  INTEGER right_value_abs(right_value);
  if (right_value < 0) right_value_abs = -right_value_abs;
  else if (right_value == 0)
    TTCN_error("The right operand of mod operator is zero.");
  if (left_value > 0) {
    return rem(left_value, right_value_abs);
  } else {
    INTEGER result = rem(left_value, right_value_abs);
    if (result == 0) return INTEGER((int)0);
    else return INTEGER(right_value_abs + result);
  }
}

INTEGER mod(const INTEGER& left_value, int right_value)
{
  left_value.must_bound("Unbound left operand of mod operator.");
  return mod(left_value, INTEGER(right_value));
}

INTEGER mod(int left_value, const INTEGER& right_value)
{
  right_value.must_bound("Unbound right operand of mod operator.");
  return mod(INTEGER(left_value), right_value);
}

boolean operator==(int int_value, const INTEGER& other_value)
{
  other_value.must_bound("Unbound right operand of integer comparison.");
  return INTEGER(int_value) == other_value;
}

boolean operator<(int int_value, const INTEGER& other_value)
{
  other_value.must_bound("Unbound right operand of integer comparison.");
  return INTEGER(int_value) < other_value;
}

boolean operator>(int int_value, const INTEGER& other_value)
{
  other_value.must_bound("Unbound right operand of integer comparison.");
  return INTEGER(int_value) > other_value;
}

// Integer template class.

void INTEGER_template::clean_up()
{
  switch (template_selection) {
    case SPECIFIC_VALUE:
      if (unlikely(!int_val.native_flag)) BN_free(int_val.val.openssl);
      break;
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
      delete [] value_list.list_value;
      break;
    case VALUE_RANGE:
      if (value_range.min_is_present && unlikely(!value_range.min_value.native_flag))
        BN_free(value_range.min_value.val.openssl);
      if (value_range.max_is_present && unlikely(!value_range.max_value.native_flag))
        BN_free(value_range.max_value.val.openssl);
      break;
    default:
      break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void INTEGER_template::copy_template(const INTEGER_template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    int_val.native_flag = other_value.int_val.native_flag;
    if (likely(int_val.native_flag))
      int_val.val.native = other_value.int_val.val.native;
    else int_val.val.openssl = BN_dup(other_value.int_val.val.openssl);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = new INTEGER_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].copy_template(
        other_value.value_list.list_value[i]);
    break;
  case VALUE_RANGE:
    value_range.min_is_present = other_value.value_range.min_is_present;
    value_range.min_is_exclusive = other_value.value_range.min_is_exclusive;
    if (value_range.min_is_present) {
      value_range.min_value.native_flag = other_value.value_range.min_value.native_flag;
      if (likely(value_range.min_value.native_flag))
        value_range.min_value.val.native =
          other_value.value_range.min_value.val.native;
      else
        value_range.min_value.val.openssl =
          BN_dup(other_value.value_range.min_value.val.openssl);
    }
    value_range.max_is_present = other_value.value_range.max_is_present;
    value_range.max_is_exclusive = other_value.value_range.max_is_exclusive;
    if (value_range.max_is_present) {
      value_range.max_value.native_flag = other_value.value_range.max_value.native_flag;
      if (likely(value_range.max_value.native_flag))
        value_range.max_value.val.native =
          other_value.value_range.max_value.val.native;
      else
        value_range.max_value.val.openssl =
          BN_dup(other_value.value_range.max_value.val.openssl);
    }
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported integer template.");
  }
  set_selection(other_value);
}

INTEGER_template::INTEGER_template()
{
}

INTEGER_template::INTEGER_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

INTEGER_template::INTEGER_template(int other_value)
  : Base_Template(SPECIFIC_VALUE)
{
  int_val.native_flag = TRUE;
  int_val.val.native = other_value;
}

INTEGER_template::INTEGER_template(const INTEGER& other_value)
  : Base_Template(SPECIFIC_VALUE)
{
  other_value.must_bound("Creating a template from an unbound integer "
    "value.");
  int_val_t other_value_int = other_value.get_val();
  int_val.native_flag = other_value_int.native_flag;
  if (likely(int_val.native_flag))
    int_val.val.native = other_value_int.val.native;
  else int_val.val.openssl = BN_dup(other_value_int.val.openssl);
}

INTEGER_template::INTEGER_template(const OPTIONAL<INTEGER>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT: {
    set_selection(SPECIFIC_VALUE);
    int_val_t other_value_int = ((const INTEGER &)other_value).get_val();
    int_val.native_flag = other_value_int.native_flag;
    if (likely(int_val.native_flag))
      int_val.val.native = other_value_int.val.native;
    else int_val.val.openssl = BN_dup(other_value_int.val.openssl);
    break; }
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating an integer template from an unbound optional field.");
  }
}

INTEGER_template::INTEGER_template(const INTEGER_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

INTEGER_template::~INTEGER_template()
{
  clean_up();
}

INTEGER_template& INTEGER_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

INTEGER_template& INTEGER_template::operator=(int other_value)
{
  clean_up();
  set_selection(SPECIFIC_VALUE);
  int_val.native_flag = TRUE;
  int_val.val.native = other_value;
  return *this;
}

INTEGER_template& INTEGER_template::operator=(const INTEGER& other_value)
{
  other_value.must_bound("Assignment of an unbound integer value to a "
    "template.");
  clean_up();
  set_selection(SPECIFIC_VALUE);
  int_val_t other_value_int = other_value.get_val();
  int_val.native_flag = other_value_int.native_flag;
  if (likely(int_val.native_flag))
    int_val.val.native = other_value_int.val.native;
  else int_val.val.openssl = BN_dup(other_value_int.val.openssl);
  return *this;
}

INTEGER_template& INTEGER_template::operator=
  (const OPTIONAL<INTEGER>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT: {
    set_selection(SPECIFIC_VALUE);
    int_val_t other_value_int = ((const INTEGER &)other_value).get_val();
    int_val.native_flag = other_value_int.native_flag;
    if (likely(int_val.native_flag))
      int_val.val.native = other_value_int.val.native;
    else int_val.val.openssl = BN_dup(other_value_int.val.openssl);
    break; }
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to an integer "
      "template.");
  }
  return *this;
}

INTEGER_template& INTEGER_template::operator=
  (const INTEGER_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean INTEGER_template::match(int other_value, boolean /* legacy */) const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    if (likely(int_val.native_flag)) return int_val.val.native == other_value;
    return int_val_t(BN_dup(int_val.val.openssl)) == other_value;
  case OMIT_VALUE:
    return FALSE;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for(unsigned int i = 0; i < value_list.n_values; i++)
      if (value_list.list_value[i].match(other_value))
        return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  case VALUE_RANGE: {
    boolean lower_boundary = !value_range.min_is_present;
    boolean upper_boundary = !value_range.max_is_present;
    // Lower boundary is set.
    if (!lower_boundary) {
      if (!value_range.min_is_exclusive) {
      lower_boundary = (likely(value_range.min_value.native_flag) ?
        int_val_t(value_range.min_value.val.native) :
          int_val_t(BN_dup(value_range.min_value.val.openssl))) <= other_value;
      } else {
        lower_boundary = (likely(value_range.min_value.native_flag) ?
        int_val_t(value_range.min_value.val.native) :
          int_val_t(BN_dup(value_range.min_value.val.openssl))) < other_value;
      }
    }
    // Upper boundary is set.
    if (!upper_boundary) {
      if (!value_range.max_is_exclusive) {
      upper_boundary = (likely(value_range.max_value.native_flag) ?
        int_val_t(value_range.max_value.val.native) :
          int_val_t(BN_dup(value_range.max_value.val.openssl))) >= other_value;
      } else {
        upper_boundary = (likely(value_range.max_value.native_flag) ?
        int_val_t(value_range.max_value.val.native) :
          int_val_t(BN_dup(value_range.max_value.val.openssl))) > other_value;
      }
    }
    return lower_boundary && upper_boundary; }
  default:
    TTCN_error("Matching with an uninitialized/unsupported integer "
      "template.");
  }
  return FALSE;
}

boolean INTEGER_template::match(const INTEGER& other_value,
                                boolean /* legacy */) const
{
  if (!other_value.is_bound()) return FALSE;
  switch (template_selection) {
    case SPECIFIC_VALUE: {
      int_val_t int_val_int = likely(int_val.native_flag) ?
        int_val_t(int_val.val.native) : int_val_t(BN_dup(int_val.val.openssl));
      return int_val_int == other_value.get_val(); }
    case OMIT_VALUE:
      return FALSE;
    case ANY_VALUE:
    case ANY_OR_OMIT:
      return TRUE;
    case VALUE_LIST:
    case COMPLEMENTED_LIST: // Merged cases.
      for (unsigned int i = 0; i < value_list.n_values; i++)
        if (value_list.list_value[i].match(other_value))
          return template_selection == VALUE_LIST;
      return template_selection == COMPLEMENTED_LIST;
    case VALUE_RANGE: {
      boolean lower_boundary = !value_range.min_is_present;
      boolean upper_boundary = !value_range.max_is_present;
      // Lower boundary is set.
      if (!lower_boundary) {
        if (!value_range.min_is_exclusive) {
          lower_boundary = (likely(value_range.min_value.native_flag) ?
            int_val_t(value_range.min_value.val.native) :
              int_val_t(BN_dup(value_range.min_value.val.openssl))) <= other_value.get_val();
        } else {
          lower_boundary = (likely(value_range.min_value.native_flag) ?
            int_val_t(value_range.min_value.val.native) :
              int_val_t(BN_dup(value_range.min_value.val.openssl))) < other_value.get_val();
        }
      }
      // Upper boundary is set.
      if (!upper_boundary) {
        if (value_range.max_is_exclusive) {
        upper_boundary = (likely(value_range.max_value.native_flag) ?
          int_val_t(value_range.max_value.val.native) :
            int_val_t(BN_dup(value_range.max_value.val.openssl))) > other_value.get_val();
        } else {
          upper_boundary = (likely(value_range.max_value.native_flag) ?
            int_val_t(value_range.max_value.val.native) :
            int_val_t(BN_dup(value_range.max_value.val.openssl))) >= other_value.get_val();
      }
      }
      return lower_boundary && upper_boundary; }
    default:
      TTCN_error("Matching with an uninitialized/unsupported integer "
        "template.");
  }
  return FALSE;
}

INTEGER INTEGER_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific "
      "integer template.");
  if (likely(int_val.native_flag)) return INTEGER(int_val.val.native);
  else return INTEGER(BN_dup(int_val.val.openssl));
}

void INTEGER_template::set_type(template_sel template_type,
  unsigned int list_length)
{
  clean_up();
  switch (template_type) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    set_selection(template_type);
    value_list.n_values = list_length;
    value_list.list_value = new INTEGER_template[list_length];
    break;
  case VALUE_RANGE:
    set_selection(VALUE_RANGE);
    value_range.min_is_present = FALSE;
    value_range.max_is_present = FALSE;
    value_range.min_is_exclusive = FALSE;
    value_range.max_is_exclusive = FALSE;
    break;
  default:
    TTCN_error("Setting an invalid type for an integer template.");
  }
}

INTEGER_template& INTEGER_template::list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
      template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list integer template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in an integer value list template.");
  return value_list.list_value[list_index];
}

void INTEGER_template::set_min(int min_value)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Integer template is not range when setting lower limit.");
  if (value_range.max_is_present) {
    int_val_t max_value_int = likely(value_range.max_value.native_flag) ?
      int_val_t(value_range.max_value.val.native) :
        int_val_t(BN_dup(value_range.max_value.val.openssl));
    if (max_value_int < min_value)
      TTCN_error("The lower limit of the range is greater than the upper "
        "limit in an integer template.");
  }
  value_range.min_is_present = TRUE;
  value_range.min_is_exclusive = FALSE;
  value_range.min_value.native_flag = TRUE;
  value_range.min_value.val.native = min_value;
}

void INTEGER_template::set_min(const INTEGER& min_value)
{
  // Redundant, but performace matters.  :)
  min_value.must_bound("Using an unbound value when setting the lower bound "
    "in an integer range template.");
  if (template_selection != VALUE_RANGE)
    TTCN_error("Integer template is not range when setting lower limit.");
  int_val_t min_value_int = min_value.get_val();
  if (value_range.max_is_present) {
    int_val_t max_value_int = likely(value_range.max_value.native_flag) ?
      int_val_t(value_range.max_value.val.native) :
        int_val_t(BN_dup(value_range.max_value.val.openssl));
    if (max_value_int < min_value_int)
      TTCN_error("The lower limit of the range is greater than the upper "
        "limit in an integer template.");
  }
  value_range.min_is_present = TRUE;
  value_range.min_is_exclusive = FALSE;
  value_range.min_value.native_flag = min_value_int.native_flag;
  if (likely(value_range.min_value.native_flag))
    value_range.min_value.val.native = min_value_int.val.native;
  else value_range.min_value.val.openssl = BN_dup(min_value_int.val.openssl);
}

void INTEGER_template::set_max(int max_value)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Integer template is not range when setting upper limit.");
  if (value_range.min_is_present) {
    int_val_t min_value_int = likely(value_range.min_value.native_flag) ?
      int_val_t(value_range.min_value.val.native) :
        int_val_t(BN_dup(value_range.min_value.val.openssl));
    if (min_value_int > max_value)
      TTCN_error("The upper limit of the range is smaller than the lower "
        "limit in an integer template.");
  }
  value_range.max_is_present = TRUE;
  value_range.max_is_exclusive = FALSE;
  value_range.max_value.native_flag = TRUE;
  value_range.max_value.val.native = max_value;
}

void INTEGER_template::set_min_exclusive(boolean min_exclusive)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Integer template is not range when setting lower limit exclusiveness.");
  value_range.min_is_exclusive = min_exclusive;
}

void INTEGER_template::set_max_exclusive(boolean max_exclusive)
{
  if (template_selection != VALUE_RANGE)
    TTCN_error("Integer template is not range when setting upper limit exclusiveness.");
  value_range.max_is_exclusive = max_exclusive;
}

void INTEGER_template::set_max(const INTEGER& max_value)
{
  max_value.must_bound("Using an unbound value when setting the upper bound "
    "in an integer range template.");
  if (template_selection != VALUE_RANGE)
    TTCN_error("Integer template is not range when setting upper limit.");
  int_val_t max_value_int = max_value.get_val();
  if (value_range.min_is_present) {
    int_val_t min_value_int = likely(value_range.min_value.native_flag) ?
      int_val_t(value_range.min_value.val.native) :
        int_val_t(BN_dup(value_range.min_value.val.openssl));
    if (min_value_int > max_value_int)
      TTCN_error("The upper limit of the range is smaller than the lower "
        "limit in an integer template.");
  }
  value_range.max_is_present = TRUE;
  value_range.max_is_exclusive = FALSE;
  value_range.max_value.native_flag = max_value_int.native_flag;
  if (likely(value_range.max_value.native_flag))
    value_range.max_value.val.native = max_value_int.val.native;
  else value_range.max_value.val.openssl = BN_dup(max_value_int.val.openssl);
}

void INTEGER_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE: {
    int_val_t int_val_int = likely(int_val.native_flag) ?
      int_val_t(int_val.val.native) : int_val_t(BN_dup(int_val.val.openssl));
    char *tmp_str = int_val_int.as_string();
    TTCN_Logger::log_event("%s", tmp_str);
    Free(tmp_str);
    break; }
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int i = 0; i < value_list.n_values; i++) {
      if (i > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[i].log();
    }
    TTCN_Logger::log_char(')');
    break;
  case VALUE_RANGE:
    TTCN_Logger::log_char('(');
    if (value_range.min_is_exclusive) TTCN_Logger::log_char('!');
    if (value_range.min_is_present) {
      int_val_t min_value_int = likely(value_range.min_value.native_flag) ?
        int_val_t(value_range.min_value.val.native) :
          int_val_t(BN_dup(value_range.min_value.val.openssl));
      char *min_str = min_value_int.as_string();
      TTCN_Logger::log_event("%s", min_str);
      Free(min_str);
    } else {
      TTCN_Logger::log_event_str("-infinity");
    }
    TTCN_Logger::log_event_str(" .. ");
    if (value_range.max_is_exclusive) TTCN_Logger::log_char('!');
    if (value_range.max_is_present) {
      int_val_t max_value_int = likely(value_range.max_value.native_flag) ?
        int_val_t(value_range.max_value.val.native) :
          int_val_t(BN_dup(value_range.max_value.val.openssl));
      char *max_str = max_value_int.as_string();
      TTCN_Logger::log_event("%s", max_str);
      Free(max_str);
    } else {
      TTCN_Logger::log_event_str("infinity");
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void INTEGER_template::log_match(const INTEGER& match_value,
                                 boolean /* legacy */) const
{
  if (TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()
  &&  TTCN_Logger::get_logmatch_buffer_len() != 0) {
    TTCN_Logger::print_logmatch_buffer();
    TTCN_Logger::log_event_str(" := ");
  }
  match_value.log();
  TTCN_Logger::log_event_str(" with ");
  log();
  if (match(match_value)) TTCN_Logger::log_event_str(" matched");
  else TTCN_Logger::log_event_str(" unmatched");
}

void INTEGER_template::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_TEMPLATE, "integer template");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    *this = OMIT_VALUE;
    break;
  case Module_Param::MP_Any:
    *this = ANY_VALUE;
    break;
  case Module_Param::MP_AnyOrNone:
    *this = ANY_OR_OMIT;
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    INTEGER_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Integer: {
    INTEGER tmp;
    tmp.set_val(*mp->get_integer());
    *this = tmp;
  } break;
  case Module_Param::MP_IntRange: {
    set_type(VALUE_RANGE);
    if (mp->get_lower_int()!=NULL) {
      INTEGER tmp;
      tmp.set_val(*mp->get_lower_int());
      set_min(tmp);
    }
    set_min_exclusive(mp->get_is_min_exclusive());
    if (mp->get_upper_int()!=NULL) {
      INTEGER tmp;
      tmp.set_val(*mp->get_upper_int());
      set_max(tmp);
    }
    set_max_exclusive(mp->get_is_max_exclusive());
  } break;
  case Module_Param::MP_Expression:
    switch (mp->get_expr_type()) {
    case Module_Param::EXPR_NEGATE: {
      INTEGER operand;
      operand.set_param(*mp->get_operand1());
      *this = - operand;
      break; }
    case Module_Param::EXPR_ADD: {
      INTEGER operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 + operand2;
      break; }
    case Module_Param::EXPR_SUBTRACT: {
      INTEGER operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 - operand2;
      break; }
    case Module_Param::EXPR_MULTIPLY: {
      INTEGER operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      *this = operand1 * operand2;
      break; }
    case Module_Param::EXPR_DIVIDE: {
      INTEGER operand1, operand2;
      operand1.set_param(*mp->get_operand1());
      operand2.set_param(*mp->get_operand2());
      if (operand2 == 0) {
        param.error("Integer division by zero.");
      }
      *this = operand1 / operand2;
      break; }
    default:
      param.expr_type_error("an integer");
      break;
    }
    break;    
  default:
    param.type_error("integer template");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* INTEGER_template::get_param(Module_Param_Name& param_name) const
{
  Module_Param* mp = NULL;
  switch (template_selection) {
  case UNINITIALIZED_TEMPLATE:
    mp = new Module_Param_Unbound();
    break;
  case OMIT_VALUE:
    mp = new Module_Param_Omit();
    break;
  case ANY_VALUE:
    mp = new Module_Param_Any();
    break;
  case ANY_OR_OMIT:
    mp = new Module_Param_AnyOrNone();
    break;
  case SPECIFIC_VALUE:
    if (likely(int_val.native_flag)) {
      mp = new Module_Param_Integer(new int_val_t(int_val.val.native));
    }
    else {
      mp = new Module_Param_Integer(new int_val_t(BN_dup(int_val.val.openssl)));
    }
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (size_t i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i].get_param(param_name));
    }
    break; }
  case VALUE_RANGE: {
    int_val_t* lower_bound = NULL;
    int_val_t* upper_bound = NULL;
    if (value_range.min_is_present) {
      if (value_range.min_value.native_flag) {
        lower_bound = new int_val_t(value_range.min_value.val.native);
      }
      else {
        lower_bound = new int_val_t(BN_dup(value_range.min_value.val.openssl));
      }
    }
    if (value_range.max_is_present) {
      if (value_range.max_value.native_flag) {
        upper_bound = new int_val_t(value_range.max_value.val.native);
      }
      else {
        upper_bound = new int_val_t(BN_dup(value_range.max_value.val.openssl));
      }
    }
    mp = new Module_Param_IntRange(lower_bound, upper_bound, value_range.min_is_exclusive, value_range.max_is_exclusive);
    break; }
  default:
    TTCN_error("Referencing an uninitialized/unsupported integer template.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void INTEGER_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE:
    text_buf.push_int(likely(int_val.native_flag) ? int_val_t(int_val.val.native)
      : int_val_t(BN_dup(int_val.val.openssl)));
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].encode_text(text_buf);
    break;
  case VALUE_RANGE:
    text_buf.push_int(value_range.min_is_present ? 1 : 0);
    if (value_range.min_is_present)
      text_buf.push_int(likely(value_range.min_value.native_flag) ?
        int_val_t(value_range.min_value.val.native) :
          int_val_t(BN_dup(value_range.min_value.val.openssl)));
    text_buf.push_int(value_range.max_is_present ? 1 : 0);
    if (value_range.max_is_present)
      text_buf.push_int(likely(value_range.max_value.native_flag) ?
        int_val_t(value_range.max_value.val.native) :
          int_val_t(BN_dup(value_range.max_value.val.openssl)));
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported integer "
      "template.");
  }
}

void INTEGER_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case SPECIFIC_VALUE: {
    int_val_t int_val_int = text_buf.pull_int();
    int_val.native_flag = int_val_int.native_flag;
    if (likely(int_val.native_flag)) int_val.val.native = int_val_int.val.native;
    else int_val.val.openssl = BN_dup(int_val_int.val.openssl);
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new INTEGER_template[value_list.n_values];
    for (unsigned int i = 0; i < value_list.n_values; i++)
      value_list.list_value[i].decode_text(text_buf);
    break;
  case VALUE_RANGE:
    value_range.min_is_present = text_buf.pull_int() != 0;
    if (value_range.min_is_present) {
      int_val_t min_value_int = text_buf.pull_int();
      value_range.min_value.native_flag = min_value_int.native_flag;
      if (likely(value_range.min_value.native_flag))
        value_range.min_value.val.native = min_value_int.val.native;
      else value_range.min_value.val.openssl = BN_dup(min_value_int.val.openssl);
    }
    value_range.max_is_present = text_buf.pull_int() != 0;
    if (value_range.max_is_present) {
      int_val_t max_value_int = text_buf.pull_int();
      value_range.max_value.native_flag = max_value_int.native_flag;
      if (likely(value_range.max_value.native_flag))
        value_range.max_value.val.native = max_value_int.val.native;
      else value_range.max_value.val.openssl = BN_dup(max_value_int.val.openssl);
    }
    value_range.min_is_exclusive = FALSE;
    value_range.max_is_exclusive = FALSE;
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received "
      "for an integer template.");
  }
}

boolean INTEGER_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean INTEGER_template::match_omit(boolean legacy /* = FALSE */) const
{
  if (is_ifpresent) return TRUE;
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    if (legacy) {
      // legacy behavior: 'omit' can appear in the value/complement list
      for (unsigned int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i].match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    }
    // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
void INTEGER_template::check_restriction(template_res t_res, const char* t_name,
                                         boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return;
  switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {
  case TR_VALUE:
    if (!is_ifpresent && template_selection==SPECIFIC_VALUE) return;
    break;
  case TR_OMIT:
    if (!is_ifpresent && (template_selection==OMIT_VALUE ||
        template_selection==SPECIFIC_VALUE)) return;
    break;
  case TR_PRESENT:
    if (!match_omit(legacy)) return;
    break;
  default:
    return;
  }
  TTCN_error("Restriction `%s' on template of type %s violated.",
             get_res_name(t_res), t_name ? t_name : "integer");
}
#endif
