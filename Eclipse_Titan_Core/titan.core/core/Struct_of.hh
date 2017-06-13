/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef STRUCT_OF_HH
#define STRUCT_OF_HH

#include "Types.h"

class Base_Type;
#ifdef TITAN_RUNTIME_2
class Record_Of_Type;
class Set_Of_Template;
#endif
class Restricted_Length_Template;
class Record_Of_Template;

enum answer { FAILURE, SUCCESS, NO_CHANCE };
enum type_of_matching { SUBSET, EXACT, SUPERSET };
extern void **allocate_pointers(int n_elements);
extern void **reallocate_pointers(void **old_pointer, int old_n_elements,
                                  int n_elements);
extern void free_pointers(void **old_pointer);

#ifdef TITAN_RUNTIME_2
typedef boolean (*compare_function_t)(const Record_Of_Type *left_ptr, int left_index,
  const Record_Of_Type *right_ptr, int right_index);
#else
typedef boolean (*compare_function_t)(const Base_Type *left_ptr, int left_index,
  const Base_Type *right_ptr, int right_index);
#endif

typedef boolean (*match_function_t)(const Base_Type *value_ptr, int value_index,
  const Restricted_Length_Template *template_ptr, int template_index, boolean legacy);

typedef void (*log_function_t)(const Base_Type *value_ptr,
  const Restricted_Length_Template *template_ptr, int index_value,
  int index_template, boolean legacy);

#ifdef TITAN_RUNTIME_2
extern boolean compare_set_of(const Record_Of_Type *left_ptr, int left_size,
  const Record_Of_Type *right_ptr, int right_size,
  compare_function_t compare_function);
#else
extern boolean compare_set_of(const Base_Type *left_ptr, int left_size,
  const Base_Type *right_ptr, int right_size,
  compare_function_t compare_function);
#endif

extern boolean match_array(const Base_Type *value_ptr, int value_size,
  const Restricted_Length_Template *template_ptr, int template_size,
  match_function_t match_function, boolean legacy);

extern boolean match_record_of(const Base_Type *value_ptr, int value_size,
  const Record_Of_Template *template_ptr, int template_size,
  match_function_t match_function, boolean legacy);

extern boolean match_set_of(const Base_Type *value_ptr, int value_size,
  const Restricted_Length_Template *template_ptr, int template_size,
  match_function_t match_function, boolean legacy);

extern void log_match_heuristics(const Base_Type *value_ptr, int value_size,
  const Restricted_Length_Template *template_ptr, int template_size,
  match_function_t match_function, log_function_t log_function, boolean legacy);

boolean match_set_of_internal(const Base_Type *value_ptr,
  int value_start, int value_size,
  const Restricted_Length_Template *template_ptr,
  int template_start, int template_size,
  match_function_t match_function,
  type_of_matching match_type,
  int* number_of_uncovered, int* pair_list,
  unsigned int number_of_checked, boolean legacy);

#endif
