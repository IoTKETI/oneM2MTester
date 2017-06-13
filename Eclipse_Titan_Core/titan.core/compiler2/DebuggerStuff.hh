/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Baranyi, Botond – initial implementation
 *
 ******************************************************************************/

#ifndef DEBUGGERSTUFF_HH
#define DEBUGGERSTUFF_HH

#include <stddef.h>

// forward declarations
namespace Common {
  class Assignment;
  class Module;
  
/** Generates code, that adds a variable to a debugger scope object.
  * @param str code generation buffer
  * @param var_ass the variable's definition
  * @param current_mod scope object's module (NULL means the module is the same,
  * where the variable is defined)
  * @param scope_name the prefix of the debugger scope object (NULL for local
  * variables) */
extern char* generate_code_debugger_add_var(char* str, Common::Assignment* var_ass,
  Common::Module* current_mod = NULL, const char* scope_name = NULL);

/** Generates code, that creates a debugger function object, adds its parameters
  * to be tracked by the debugger, and takes the function's initial snapshot.
  * @param str code generation buffer
  * @param func_ass the function's definition */
extern char* generate_code_debugger_function_init(char* str,
   Common::Assignment* func_ass);

}

#endif /* DEBUGGERSTUFF_HH */

