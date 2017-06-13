/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include <TTCN3.hh>
#include <ctype.h>

namespace xmlTest__Shell {

// Count the lines which starts with a number
INTEGER f__countDiffs(const CHARSTRING& diffoutput) {
  INTEGER result = 0;
  const char* c_diffoutput = (const char*)diffoutput;
  if (isdigit(*c_diffoutput)) {
    result = result + 1;
  }
  const char* pos = strchr(c_diffoutput, '\n');
  while (pos != NULL) {
    if ((pos + 1) != NULL && isdigit(*(pos + 1))) {
      result = result + 1;
    }
    pos = strchr(pos + 1, '\n');
  }
  return result;
}

}
