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
 *   Gecse, Roland
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_stack_HH
#define _Common_stack_HH

#include "error.h"
#include "../common/memory.h"

/**
 * LIFO container optimized to store elements sequentially,
 * and access only the element on the top of the stack.
 * Accessing the top of the stack has amortized constant time cost.
 *
 * The container stores pointers to objects of type T; it doesn't own them.
 * It is the responsibility of the caller to create and destroy the objects
 * and supply the pointers to the container.
 *
 * \ingroup containers
 */
template<class T>
class stack {

private:

  size_t num_s; ///< used size
  size_t max_s; ///< allocated size
  T **s_ptr; ///< array of pointers to objects of type T

  static const size_t initial_size = 1, increment_factor = 2;

  /** Copy constructor: DO NOT IMPLEMENT! */
  stack(const stack&);
  /** Copy assignment: DO NOT IMPLEMENT! */
  stack& operator=(const stack&);

public:

  static const size_t max_stack_length = static_cast<size_t>(-1 );

  /** Creates an empty stack. */
  stack() : num_s(0), max_s(0), s_ptr(NULL) { }

  /** Deallocates its memory. If the container is not empty,
   * FATAL_ERROR occurs, so before destructing, clear()
   * must be invoked explicitly.
   */
  ~stack() {
    if(num_s > 0) FATAL_ERROR("stack::~stack(): stack is not empty");
    Free(s_ptr);
  }

  /** Returns the number of elements in the container. */
  size_t size() const { return num_s; }

  /** Returns true if the container has no elements. */
  bool empty() const { return num_s == 0; }

  /** Forgets all elements in the container.
   * No memory is freed. */
  void clear() { num_s = 0; }

  /** Push the elem onto the stack. */
  void push(T *elem) {
    if (max_s <= num_s) {
      if (max_s == 0) {
	max_s = initial_size;
	s_ptr = static_cast<T**>(Malloc(initial_size * sizeof(*s_ptr)));
      } else {
	if(max_s >= max_stack_length / increment_factor)
	  FATAL_ERROR("stack::push(): stack index overflow");
	max_s *= increment_factor;
	s_ptr = static_cast<T**>(Realloc(s_ptr, max_s * sizeof(*s_ptr)));
      }
    }
    s_ptr[num_s++] = elem;
  }

  /** Returns the top of the stack or FATAL_ERROR if empty. */
  T* top() const {
    if(num_s == 0) FATAL_ERROR("stack::top() const: stack is empty");
    return s_ptr[num_s - 1];
  }

  /** Returns the top of the stack, and removes it from the
   * container. If the stack is empty then FATAL_ERROR occurs. */
  T* pop() {
    if(num_s == 0) FATAL_ERROR("stack::pop(): stack is empty");
    return s_ptr[--num_s];
  }

};

#endif // _Common_stack_HH
