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


void flatten(CHARSTRING& par) {
  CHARSTRING retval(0, ""); // empty string
  const int max = par.lengthof();
  for (int i = 0; i < max; ++i) {
    char c = par[i].get_char();
    //if ( c != '\t'
    //  && c != '\n') {
    if ( isprint(c) ) {
      retval += c;
    }
  }
  par = retval;
}

void flatten(UNIVERSAL_CHARSTRING& par) {
  TTCN_Buffer buf;
  const int max = par.lengthof();
  for (int i = 0; i < max; ++i) {
    universal_char uc = par[i].get_uchar();
    //if ( c != '\t'
    //  && c != '\n') {
    if ( !uc.uc_group && !uc.uc_plane && !uc.uc_row && isprint(uc.uc_cell)) {
      buf.put_s(4, (const unsigned char*)&uc);
    }
  }
  buf.get_string(par);
}


namespace Txerasntypes {
  void flatten(CHARSTRING& par)
  { ::flatten(par); }
  void flatten(UNIVERSAL_CHARSTRING& par)
  { ::flatten(par); }
}

namespace Txerboolean {
  void flatten(CHARSTRING& par)
  { ::flatten(par); }
  void flatten(UNIVERSAL_CHARSTRING& par)
  { ::flatten(par); }
}

namespace Txerint {
  void flatten(CHARSTRING& par)
  { ::flatten(par); }
  void flatten(UNIVERSAL_CHARSTRING& par)
  { ::flatten(par); }
}

namespace Txerstring {
  void flatten(CHARSTRING& par)
  { ::flatten(par); }
  void flatten(UNIVERSAL_CHARSTRING& par)
  { ::flatten(par); }
}

namespace Txerfloat {
  void flatten(CHARSTRING& par)
  { ::flatten(par); }
  void flatten(UNIVERSAL_CHARSTRING& par)
  { ::flatten(par); }
}

namespace Txerenum {
  void flatten(CHARSTRING& par)
  { ::flatten(par); }
  void flatten(UNIVERSAL_CHARSTRING& par)
  { ::flatten(par); }
}

namespace Txernested {
  void flatten(CHARSTRING& par)
  { ::flatten(par); }
  void flatten(UNIVERSAL_CHARSTRING& par)
  { ::flatten(par); }
}

namespace Txersets {
  void flatten(CHARSTRING& par)
  { ::flatten(par); }
  void flatten(UNIVERSAL_CHARSTRING& par)
  { ::flatten(par); }
}

namespace Txerobjclass {
  void flatten(CHARSTRING& par)
  { ::flatten(par); }
  void flatten(UNIVERSAL_CHARSTRING& par)
  { ::flatten(par); }
}

