/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "Array.hh"

unsigned int get_timer_array_index(int index_value, unsigned int array_size,
  int index_offset)
{
  if (index_value < index_offset) TTCN_error("Index underflow when accessing "
    "an element of a timer array. The index value should be between %d and %d "
    "instead of %d.", index_offset, index_offset + array_size - 1, index_value);
  unsigned int ret_val = index_value - index_offset;
  if (ret_val >= array_size)
    TTCN_error("Index overflow when accessing an element of a timer array. "
    "The index value should be between %d and %d instead of %d.", index_offset,
    index_offset + array_size - 1, index_value);
  return ret_val;
}

unsigned int get_timer_array_index(const INTEGER& index_value,
  unsigned int array_size, int index_offset)
{
  if (!index_value.is_bound()) TTCN_error("Accessing an element of a timer "
    "array using an unbound index.");
  return get_timer_array_index((int)index_value, array_size, index_offset);
}

unsigned int get_port_array_index(int index_value, unsigned int array_size,
  int index_offset)
{
  if (index_value < index_offset) TTCN_error("Index underflow when accessing "
    "an element of a port array. The index value should be between %d and %d "
    "instead of %d.", index_offset, index_offset + array_size - 1, index_value);
  unsigned int ret_val = index_value - index_offset;
  if (ret_val >= array_size)
    TTCN_error("Index overflow when accessing an element of a port array. "
    "The index value should be between %d and %d instead of %d.", index_offset,
    index_offset + array_size - 1, index_value);
  return ret_val;
}

unsigned int get_port_array_index(const INTEGER& index_value,
  unsigned int array_size, int index_offset)
{
  if (!index_value.is_bound()) TTCN_error("Accessing an element of a port "
    "array using an unbound index.");
  return get_timer_array_index((int)index_value, array_size, index_offset);
}

////////////////////////////////////////////////////////////////////////////////

unsigned int get_array_index(int index_value, unsigned int array_size,
  int index_offset)
{
  if (index_value < index_offset) TTCN_error("Index underflow when accessing "
    "an element of an array. The index value should be between %d and %d "
    "instead of %d.", index_offset, index_offset + array_size - 1, index_value);
  unsigned int ret_val = index_value - index_offset;
  if (ret_val >= array_size)
    TTCN_error("Index overflow when accessing an element of an array. "
    "The index value should be between %d and %d instead of %d.", index_offset,
    index_offset + array_size - 1, index_value);
  return ret_val;
}

unsigned int get_array_index(const INTEGER& index_value,
  unsigned int array_size, int index_offset)
{
  if (!index_value.is_bound()) TTCN_error("Accessing an element of an "
    "array using an unbound index.");
  return get_array_index((int)index_value, array_size, index_offset);
}
