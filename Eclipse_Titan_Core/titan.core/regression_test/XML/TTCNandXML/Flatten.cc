/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include <TTCN3.hh>
#include <ctype.h>

#ifndef OLD_NAMES
namespace Flattener {
#endif

UNIVERSAL_CHARSTRING flatten(UNIVERSAL_CHARSTRING const& par) {
  TTCN_Buffer buf;
  const int max = par.lengthof();
  for (int i = 0; i < max; ++i) {
    universal_char uc = par[i].get_uchar();
    //if ( !uc.uc_group && !uc.uc_plane && !uc.uc_row && isprint(uc.uc_cell))

    // Drop characters which are in 0000-00FF and unprintable
    if ( uc.uc_group || uc.uc_plane || uc.uc_row || isprint(uc.uc_cell))
    {
      buf.put_s(4, (const unsigned char*)&uc);
    }
  }
  buf.put_s(4, (const unsigned char*)"\0\0\0\n");

  UNIVERSAL_CHARSTRING retval;
  buf.get_string(retval);
  return retval;
}


#ifndef OLD_NAMES
} // namespace
#endif
