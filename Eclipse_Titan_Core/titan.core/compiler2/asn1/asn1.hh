/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Forstner, Matyas
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _ASN1_HH
#define _ASN1_HH

/**
 * Call this before using ASN stuff.
 */
extern void asn1_init();
/**
 * Call this after finishing using ASN stuff.
 */
extern void asn1_free();
/**
 * Parse the file given by \a filename.
 */
extern int asn1_parse_file(const char* filename, boolean generate_code);
/**
 * Set to non-zero to print lof of parsing debug messages to stderr.
 */
extern int asn1_yydebug;

/*

#include "../AST.hh"

extern unsigned verb_level=7;
extern int force_overwrite, parse_only, generate_BER;
extern Common::Modules *modules;

*/

#endif // _ASN1_HH
