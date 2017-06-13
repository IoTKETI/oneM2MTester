/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Kremer, Peter
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef ASN1_PREPARSER_H
#define ASN1_PREPARSER_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int is_asn1_module(const char *file_name, FILE *fp, char **module_name);

#ifdef __cplusplus
}
#endif

#endif /* ASN1_PREPARSER_H */
