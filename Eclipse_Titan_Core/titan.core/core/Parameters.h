/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Beres, Szabolcs
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "Types.h"

/** @brief Reads and processes the configuration file

Calls preproc_parse_file(), then opens and parses the filenames returned.

@param file_name
@return 1 if successful, 0 on error
*/
extern boolean process_config_file(const char *file_name);

/** @brief Parse configuration in a buffer

@param config_string configuration buffer (not necessarily 0-terminated)
@param string_len length of buffer
@return 1 if successful, 0 on error
*/
extern boolean process_config_string(const char *config_string, int string_len);

extern void config_process_error_f(const char *error_str, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

extern void config_process_error(const char *error_str);

struct execute_list_item {
  char *module_name;
  char *testcase_name;
};

extern int execute_list_len;
extern execute_list_item *execute_list;

#endif
