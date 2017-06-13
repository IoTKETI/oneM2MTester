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

#ifndef PROFILER_H
#define	PROFILER_H

#include "../main.hh"

#ifdef	__cplusplus
extern "C" {
#endif

/** Structure containing one code line or function for the profiler */
typedef struct profiler_code_line_t {
  /** Name of the file containing this line */
  char* file_name;
  /** Name of the function that starts at this line (or null if no function starts here) */
  char* function_name;
  /** Code line number */
  size_t line_no;
} profiler_code_line_t;

/** Profiler database
  *
  * Contains entries for the code lines in the processed TTCN-3 files.
  * Used for initializing the TTCN3_Profiler (in runtime) with all the code 
  * lines and functions in the processed files, so it can determine which the
  * unused lines and functions. */
typedef struct profiler_data_t {
  profiler_code_line_t *lines;
  size_t nof_lines;
  size_t size;
} profiler_data_t;

/** Initializes the database (must be called once, at the start of compilation) */
void init_profiler_data(tcov_file_list* p_file_list);

/** Returns 1 if the specified file is in the profiler's file list,
  * otherwise returns 0.*/
boolean is_file_profiled(const char* p_file_name);

/** Inserts one code line or function to the database 
  * @param p_file_name [in] name of the file containing the line/function
  * @param p_function_name [in] name of the function (for functions) or null (for lines)
  * @param p_line_no [in] line number (for lines) or the function's starting line 
  * (for functions) */
void insert_profiler_code_line(const char* p_file_name,
  const char* p_function_name, size_t p_line_no);

/** Retrieves the next code line or function from the specified file. Calling this
  * function with the same file parameter will keep returning the next code line
  * or function until the end of the database is reached. If the file parameter is
  * changed, the function will start again from the beginning of the database.
  * @param p_file_name [in] name of the file to search for
  * @param p_function_name [out] contains the function name if a function was found
  * @param p_line_no [out] contains the line number of function start line
  * @return 1 if a line or function was found, otherwise 0 */
boolean get_profiler_code_line(const char *p_file_name,
  char **p_function_name, int *p_line_no);

/** Frees the database (must be called once, at the end of compilation) */
void free_profiler_data(void);

#ifdef	__cplusplus
}
#endif

#endif	/* PROFILER_H */

