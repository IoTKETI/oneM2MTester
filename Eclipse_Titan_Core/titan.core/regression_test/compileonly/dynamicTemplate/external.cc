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
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "DynamicTemplate.hh"

#ifndef OLD_NAMES
namespace DynamicTemplate {
#endif

FLOAT_template ef1()
{
  return FLOAT_template(ANY_VALUE);
}

T3_template ef2()
{
  return T3_template(ANY_VALUE);
}

void ef3(const INTEGER_template& /*par1*/, T2_template& /*par2*/, T3_template& /*par3*/)
{
}

#ifndef OLD_NAMES
} /* end of namespace */
#endif
