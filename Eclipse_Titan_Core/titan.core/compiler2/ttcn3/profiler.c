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

#include "profiler.h"
#include "../../common/memory.h"
#include <string.h>
#include <stdio.h>

/** The file list */
static tcov_file_list *the_files = 0;

/** The database */
static profiler_data_t *the_data = 0;

/** The current database index for the next get_profiler_code_line() call */
static size_t current_index = 0;

/** The file parameter for the last get_profiler_code_line() call */
static char* last_file_name = 0;

void init_profiler_data(tcov_file_list* p_file_list)
{
  the_files = p_file_list;
  the_data = (profiler_data_t*)Malloc(sizeof(profiler_data_t));
  the_data->nof_lines = 0;
  the_data->size = 4;
  the_data->lines = (profiler_code_line_t*)Malloc(
    the_data->size * sizeof(profiler_code_line_t));
}

boolean is_file_profiled(const char* p_file_name)
{
  tcov_file_list* file_walker = the_files;
  while (0 != file_walker) {
    if (0 == strcmp(p_file_name, file_walker->file_name)) {
      return TRUE;
    }
    /* check for .ttcnpp instead of .ttcn */
    size_t file_name_len = strlen(file_walker->file_name);
    if ('p' == file_walker->file_name[file_name_len - 1] &&
        'p' == file_walker->file_name[file_name_len - 2] &&
        0 == strncmp(p_file_name, file_walker->file_name, file_name_len - 2)) {
      return TRUE;
    }
    file_walker = file_walker->next;
  }
  return FALSE;
}

void insert_profiler_code_line(const char* p_file_name,
  const char* p_function_name, size_t p_line_no)
{
  /* check if the entry already exists */
  for (size_t i = 0; i < the_data->nof_lines; ++i) {
    if (0 == strcmp(p_file_name,the_data->lines[i].file_name) &&
        p_line_no == the_data->lines[i].line_no) {
      if (0 == the_data->lines[i].function_name && 0 != p_function_name) {
        the_data->lines[i].function_name = mcopystr(p_function_name);
      }
      return;
    }
  }
  if (the_data->nof_lines == the_data->size) {
    /* database size reached, increase the buffer size (exponentially) */
    the_data->size *= 2;
    the_data->lines = (profiler_code_line_t*)Realloc(the_data->lines,
      the_data->size * sizeof(profiler_code_line_t));
  }
  the_data->lines[the_data->nof_lines].file_name = mcopystr(p_file_name);
  if (0 != p_function_name) {
    the_data->lines[the_data->nof_lines].function_name = mcopystr(p_function_name);
  }
  else {
    the_data->lines[the_data->nof_lines].function_name = 0;
  }
  the_data->lines[the_data->nof_lines].line_no = p_line_no;
  ++the_data->nof_lines;
}

boolean get_profiler_code_line(const char *p_file_name,
  char **p_function_name, int *p_line_no)
{
  if (0 == last_file_name || 0 != strcmp(last_file_name, p_file_name)) {
    /* file parameter changed, reset the current index and store the new file name */
    current_index = 0;
    Free(last_file_name);
    last_file_name = mcopystr(p_file_name);
  }
  while (current_index < the_data->nof_lines) {
    /* find the first entry in the specified file and return it */
    if (0 == strcmp(p_file_name, the_data->lines[current_index].file_name)) {
      *p_function_name = the_data->lines[current_index].function_name;
      *p_line_no = the_data->lines[current_index].line_no;
      ++current_index;
      return TRUE;
    }
    ++current_index;
  }
  /* no entry found with the specified file name */
  return FALSE;
}

void free_profiler_data(void)
{
  for (size_t i = 0; i < the_data->nof_lines; ++i) {
    Free(the_data->lines[i].file_name);
    Free(the_data->lines[i].function_name);
  }
  Free(the_data->lines);
  Free(the_data);
  Free(last_file_name);
  
  while (0 != the_files) {
	  tcov_file_list *next_file = the_files->next;
	  Free(the_files->file_name);
	  Free(the_files);
	  the_files = next_file;
	}
}
