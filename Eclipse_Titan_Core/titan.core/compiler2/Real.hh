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
 *   Delic, Adam
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_Real_HH
#define _Common_Real_HH

#include "string.hh"

// older versions of gcc do not have numeric_limits<double> or it is wrong
// and they do not have constants defined by C99 standard
#ifdef INFINITY
static const double REAL_INFINITY = INFINITY;
#else
#include <float.h>
static const double REAL_INFINITY = (DBL_MAX*DBL_MAX);
#endif

#ifdef NAN
static const double REAL_NAN = NAN;
#else
static const double REAL_NAN = (REAL_INFINITY-REAL_INFINITY);
#endif

namespace Common {

  class Location;

  /**
   * \defgroup Real Real type and related functions
   *
   * Real type is used to represent those real/float values which will
   * be used in TITAN run-time environment, in the compiler.
   *
   * @{
   */

  /** Real typedef */

  typedef double Real;

  /** +/- infinity and not_a_number are non-numeric float values in ttcn-3,
      these special values cannot be used in some places */
  bool isSpecialFloatValue(const Real& r);

  /**
   * Converts the Common::Real value to string.
   *
   * The returned string looks like this:
   *
   * <pre>
     (
      (
        ([\-][1-9]\.(([0-9]+[1-9])|0))
       |
        ([\-]0\.([0-9]+[1-9]))
      )
      "e"
      (([\-][1-9][0-9]*)|0)
     )
    |
     (0\.0e0)
    |
     "INF"
    |
     "-INF"
   * </pre>
   */
  string Real2string(const Real& r);

  /** Returns the C++ equivalent of value r */
  string Real2code(const Real& r);

  /**
   * Converts the string value to Common::Real.
   */
  Real string2Real(const char *s, const Location& loc);
  inline Real string2Real(const string& s, const Location& loc)
    { return string2Real(s.c_str(), loc); }
  /** @} end of Real group */

} // namespace Common

#endif // _Common_Real_HH
