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
 *   Gecse, Roland
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include "Real.hh"
#include "string.hh"
#include "error.h"
#include "Setting.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <cmath>

namespace Common {

  bool isSpecialFloatValue(const Real& r)
  {
    return ((r!=r) || (r==REAL_INFINITY) || (r==-REAL_INFINITY));
  }

  string Real2string(const Real& r)
  {
    if (r == 0.0) return ((1.0/r)==-REAL_INFINITY) ? string("-0.0e0") : string("0.0e0");
    else if (r == REAL_INFINITY) return string("INF");
    else if (r == -REAL_INFINITY) return string("-INF");
    else if (r != r) return string("NaN");

    Real   rabs     = fabs(r);
    int    sign     = (r / rabs < 0) ? -1 : 1;
    double exponent = floor(log10(rabs));
    double fraction = rabs * pow(10.0, -exponent);
    double integral = floor(fraction);
    char   tmp[64];

    sprintf(tmp, "%s%.15g%se%d",
            (sign == -1) ? "-" : "",
            fraction,
            (fraction == integral) ? ".0" : "",
            (int)exponent);

    return string(tmp);
  }

  string Real2code(const Real& r)
  {
    if (r == 0.0) return ((1.0/r)==-REAL_INFINITY) ? string("-0.0") : string("0.0");
    else if (r == REAL_INFINITY) return string("PLUS_INFINITY");
    else if (r == -REAL_INFINITY) return string("MINUS_INFINITY");
    else if (r != r) return string("NOT_A_NUMBER");
    else {
      Real rabs = fabs(r);
      Real exponent = floor(log10(rabs));
      Real mantissa = rabs * pow(10.0, -exponent);

      char *tmp = mprintf("%s%.15g", r < 0.0 ? "-" : "", mantissa);
      if (floor(mantissa) == mantissa) tmp = mputstr(tmp, ".0");
      if (exponent != 0.0) tmp = mputprintf(tmp, "e%d", (int)exponent);
      string ret_val(tmp);
      Free(tmp);
      return ret_val;
    }
  }

  Real string2Real(const char *s, const Location& loc)
  {
    if (s[0] == '\0' || !strcmp(s, "0.0e0")) return 0.0;
    else if (!strcmp(s, "-INF")) return -REAL_INFINITY;
    else if (!strcmp(s, "INF")) return REAL_INFINITY;
    else if (!strcmp(s, "NaN")) return REAL_NAN;
    errno = 0;
    Real r = strtod(s, NULL);
    switch (errno) {
    case ERANGE:
      loc.error("Overflow when converting `%s' to a floating point value: %s",
        s, strerror(errno));
      break;
    case 0:
      break;
    default:
      FATAL_ERROR("string2Real(): Unexpected error when converting `%s' to real: %s",
        s, strerror(errno));
    }
    return r;
  }

} // namespace Common
