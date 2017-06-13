/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Bene, Tamas
 *   Lovassy, Arpad
 *   Szabo, Janos Zoltan – initial implementation
 *   Vilmos Varga - author
 *
 ******************************************************************************/
//
// Description:           Implementation file for UserInterface
//----------------------------------------------------------------------------
#include "UserInterface.h"

using namespace mctr;

//----------------------------------------------------------------------------

UserInterface::~UserInterface()
{

}

//----------------------------------------------------------------------------

void UserInterface::initialize()
{

}

void UserInterface::executeBatchFile(const char* /* filename */)
{
  error(/* severity */ 0, "This user interface does not support batch files.");
}

//----------------------------------------------------------------------------
// Local Variables:
// mode: C++
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
