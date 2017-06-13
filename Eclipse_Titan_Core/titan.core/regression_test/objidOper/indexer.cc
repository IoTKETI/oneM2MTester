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
#include <Integer.hh>
#include <Objid.hh>

namespace TobjidOper {

INTEGER indexer(OBJID const& o, INTEGER const& idx)
{
  INTEGER r;
  r.set_long_long_val(o[idx]);
  // Use long long because OBJID::objid_element (currently unsigned 32bit)
  // does not fit into the native representation (_signed_ 32bit) of INTEGER
  return r;
}


}
