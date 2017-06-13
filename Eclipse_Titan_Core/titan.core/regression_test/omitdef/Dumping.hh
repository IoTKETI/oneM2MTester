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
 *
 ******************************************************************************/
#ifndef DUMPING_HH
#define DUMPING_HH

#include <string>

std::string dump( const TTCN_Buffer& buf );

template < typename T >
std::string dump( const T& t, const TTCN_Typedescriptor_t ty )
{
  TTCN_Buffer ttcnbuf;
  t.encode( ty, ttcnbuf, TTCN_EncDec::CT_BER, BER_ENCODE_DER );
  return dump( ttcnbuf );
}

#endif
