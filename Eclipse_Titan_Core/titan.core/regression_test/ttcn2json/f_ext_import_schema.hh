/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *
 ******************************************************************************/

#ifndef f_ext_import_schema_HH
#define f_ext_import_schema_HH

#include "CompareSchemas.hh"

namespace CompareSchemas {

/** Imports a JSON schema from a given file to the specified container.
  * Throws a Dynamic Testcase Error on failure.
  *
  * @param file source file name
  * @param schema JSON schema container (defined in CompareSchemas.ttcn)
  */
extern void f__ext__import__schema(const CHARSTRING& file, JsonSchema& schema);

}

#endif

