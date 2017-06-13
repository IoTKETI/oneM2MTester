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
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef TTCN3FLOAT_HH_
#define TTCN3FLOAT_HH_

#include <math.h>

/* TTCN-3 float values that have absolute value smaller than this
   are displayed in exponential notation. */
#define MIN_DECIMAL_FLOAT		1.0E-4
/* TTCN-3 float values that have absolute value larger or equal than this
   are displayed in exponential notation. */
#define MAX_DECIMAL_FLOAT		1.0E+10

#ifndef signbit
// Probably Solaris.
// Thankfully, IEEE Std 1003.1, 2004 Edition says that signbit is a macro,
// hence it's safe to use ifdef.

#ifdef __sparc
// Big endian

inline int signbitfunc(double d)
{
  return *((unsigned char*)&d) & 0x80;
}

#else
// Probably Intel, assume little endian
inline int signbitfunc(double d)
{
  return ((unsigned char*)&d)[sizeof(double)-1] & 0x80;
}

#endif

#define signbit(d) signbitfunc(d)

#endif // def signbit

/** A class which behaves almost, but not quite, entirely unlike
 *  a floating-point value.
 *
 *  It is used as a member of a union (in Value.hh);
 *  it MUST NOT have a constructor.
 */
struct ttcn3float {
  /// Implicit conversion
  operator double() const { return value; }

  /// Assignment from a proper double
  const ttcn3float& operator=(double d) {
    value = d;
    return *this;
  }

  /// Address-of, for scanf
  double* operator&() { return &value; }
  
  double operator+(const ttcn3float& x) const {
    return value + x.value;
  }

  const ttcn3float& operator+=(double d) {
    value += d;
    return *this;
  }

  const ttcn3float& operator-=(double d) {
    value -= d;
    return *this;
  }

  const ttcn3float& operator*=(double d) {
    value *= d;
    return *this;
  }

  const ttcn3float& operator/=(double d) {
    value /= d;
    return *this;
  }

  bool operator<(double d) const {
    if (isnan(value)) {
      return false; // TTCN-3 special: NaN is bigger than anything except NaN
    }
    else if (isnan(d)) {
      return true; // TTCN-3 special: NaN is bigger than anything except NaN
    }
    else if (value==0.0 && d==0.0) { // does not distinguish -0.0
      return signbit(value) && !signbit(d); // value negative, d positive
    }
    else { // finally, the sensible behavior
      return value < d;
    }
  }

  bool operator>(double d) const {
    if (isnan(value)) {
      return !isnan(d); // TTCN-3 special: NaN is bigger than anything except NaN
    }
    else if (isnan(d)) {
      return false; // TTCN-3 special: NaN is bigger than anything except NaN
    }
    else if (value==0.0 && d==0.0) { // does not distinguish -0.0
      return !signbit(value) && signbit(d); // value positive, d negative
    }
    else { // finally, the sensible behavior
      return value > d;
    }
  }

  bool operator==(double d) const {
    if (isnan(value)) {
      return !!isnan(d); // TTCN-3 special: NaN is bigger than anything except NaN
    }
    else if (isnan(d)) {
      return false;
    }
    else if (value==0.0 && d==0.0) { // does not distinguish -0.0
      return signbit(value) == signbit(d);
    }
    else { // finally, the sensible behavior
      return value == d;
    }
  }
public:
  double value;
};

/** Replacement for a user-defined constructor that ttcn3float can't have */
inline ttcn3float make_ttcn3float(double f) {
  ttcn3float retval = { f };
  return retval;
}

#endif /* TTCN3FLOAT_HH_ */
