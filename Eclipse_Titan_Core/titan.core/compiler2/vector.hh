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
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_vector_HH
#define _Common_vector_HH

#ifdef __SUNPRO_CC
/**
 * Inclusion of the STL vector header file prevents the Sun Forte 6.2 C++
 * compiler from a mysterious internal error.
 */
#include <vector>
#include <stdio.h>
#endif

#include "error.h"
#include "../common/memory.h"
#include <string.h> // for memmove

/**
 * Container optimized to store elements sequentially,
 * and access them randomly, referenced by indices.
 *
 * Accessing an element has constant time cost.
 * Adding an element at the end has amortized constant time const.
 * Other operations (adding at the beginning, replacing/deleting elements)
 * have linear time cost.
 *
 * If there aren't elements in the buffer, then no space is allocated.
 * If there are, the size of the allocated buffer is the smallest power of 2
 *   that is not smaller than the number of elements (num_e).
 *
 * The container stores pointers to objects of type T; it doesn't own them.
 * It is the responsibility of the caller to create and destroy the objects
 * and supply the pointers to the container.
 *
 * \ingroup containers
 */
template<class T>
class vector {

private:

  size_t num_e;
  T **e_ptr;

  static const size_t initial_size = 1, increment_factor = 2;

  /** Copy constructor: DO NOT IMPLEMENT! */
  vector(const vector&);
  /** Copy assignment: DO NOT IMPLEMENT! */
  vector& operator=(const vector&);

public:

  static const size_t max_vector_length = static_cast<size_t>( -1 );

  /** Creates an empty vector. */
  vector() : num_e(0), e_ptr(NULL) { }

  /** Deallocates its memory. If the container is not empty,
   *  FATAL_ERROR occurs, so before destructing, clear() must be
   *  invoked explicitly.
   */
  ~vector() {
    if (num_e > 0) FATAL_ERROR("vector::~vector(): vector is not empty");
    Free(e_ptr);
  }

  /** Returns the number of elements in the container. */
  size_t size() const { return num_e; }

  /** Returns true if the container has no elements. */
  bool empty() const { return num_e == 0; }

  /** Erases the entire container. */
  void clear() {
    num_e = 0;
    Free(e_ptr);
    e_ptr = NULL;
  }

  /** Appends the \a elem to the end of vector. */
  void add(T *elem) {
    if (e_ptr == NULL) {
      e_ptr = static_cast<T**>(Malloc(initial_size * sizeof(*e_ptr)));
    } else {
      size_t max_e = initial_size;
      while (max_e < num_e) max_e *= increment_factor;
      if (max_e <= num_e) {
	if (max_e >= max_vector_length / increment_factor)
	    FATAL_ERROR("vector::add(): vector index overflow");
	e_ptr = static_cast<T**>
		(Realloc(e_ptr, max_e * increment_factor * sizeof(*e_ptr)));
      }
    }
    e_ptr[num_e++] = elem;
  }

  /** Inserts the \a elem to the beginning of vector. */
  void add_front(T *elem) {
    if (e_ptr == NULL) {
      e_ptr = static_cast<T**>(Malloc(initial_size * sizeof(*e_ptr)));
    } else {
      size_t max_e = initial_size;
      while (max_e < num_e) max_e *= increment_factor;
      if (max_e <= num_e) {
	if (max_e >= max_vector_length / increment_factor)
	    FATAL_ERROR("vector::add_front(): vector index overflow");
	e_ptr = static_cast<T**>
		(Realloc(e_ptr, max_e * increment_factor * sizeof(*e_ptr)));
      }
    }
    memmove(e_ptr + 1, e_ptr, num_e * sizeof(*e_ptr));
    num_e++;
    e_ptr[0] = elem;
  }

  /** Returns the <em>n</em>th element. The index of the first element is
   * zero. If no such index, then FATAL_ERROR occurs. */
  T* operator[](size_t n) const {
    if (n >= num_e)
      FATAL_ERROR("vector::operator[](size_t) const: index overflow");
    return e_ptr[n];
  }

  /** Returns the <em>n</em>th element. The index of the first element is
   * zero. If no such index, then FATAL_ERROR occurs. */
  T*& operator[](size_t n) {
    if (n >= num_e)
      FATAL_ERROR("vector::operator[](size_t): index overflow");
    return e_ptr[n];
  }

  /** Replaces \a n elements beginning from position \a pos
   * with elements in \a v. If \a pos+n > size() then FATAL_ERROR occurs.
   * If \a v == NULL then deletes the elements.
   */
  void replace(size_t pos, size_t n, const vector *v = NULL) {
    if (pos > num_e) FATAL_ERROR("vector::replace(): position points over " \
      "the last element");
    else if (n > num_e - pos) FATAL_ERROR("vector::replace(): not enough " \
      "elements after the start position");
    else if (v == this) FATAL_ERROR("vector::replace(): trying to replace " \
      "the original vector");
    size_t v_len = v != NULL ? v->num_e : 0;
    if (v_len > max_vector_length - num_e + n)
      FATAL_ERROR("vector::replace(): resulting vector size exceeds maximal " \
	"length");
    size_t new_len = num_e - n + v_len;
    if (new_len > num_e) {
      size_t max_e = initial_size;
      if (e_ptr == NULL) {
	while (max_e < new_len) max_e *= increment_factor;
	e_ptr = static_cast<T**>(Malloc(max_e * sizeof(*e_ptr)));
      } else {
	while (max_e < num_e) max_e *= increment_factor;
	if (new_len > max_e) {
	  while (max_e < new_len) max_e *= increment_factor;
	  e_ptr = static_cast<T**>(Realloc(e_ptr, max_e * sizeof(*e_ptr)));
	}
      }
    }
    if (pos + n < num_e && v_len != n) memmove(e_ptr + pos + v_len,
	e_ptr + pos + n, (num_e - pos - n) * sizeof(*e_ptr));
    if (v_len > 0) memcpy(e_ptr + pos, v->e_ptr, v_len * sizeof(*e_ptr));
    if (new_len < num_e) {
      if (new_len == 0) {
	Free(e_ptr);
	e_ptr = NULL;
      } else {
	size_t max_e = initial_size;
	while (max_e < num_e) max_e *= increment_factor;
	size_t max_e2 = initial_size;
	while (max_e2 < new_len) max_e2 *= increment_factor;
	if (max_e2 < max_e)
	  e_ptr = static_cast<T**>(Realloc(e_ptr, max_e2 * sizeof(*e_ptr)));
      }
    }
    num_e = new_len;
  }

  /**
   * Copies a part of the vector to a new vector. The part is
   * specified by the starting position (<em>pos</em>) and the number
   * of elements (<em>n</em>) to copy. If <em>n</em> is greater than
   * the number of elements beginning from the given <em>pos</em>,
   * then the returned vector contains less elements.
   *
   * \note The pointers are copied, the objects they refer to will not
   * be duplicated.
   */
  vector* subvector(size_t pos = 0, size_t n = max_vector_length) const
  {
    if (pos > num_e) FATAL_ERROR("vector::subvector(): position points " \
      "over last vector element");
    if (n > num_e - pos) n = num_e - pos;
    vector *tmp_vector = new vector;
    if (n > 0) {
      size_t max_e = initial_size;
      while (max_e < n) max_e *= increment_factor;
      tmp_vector->e_ptr = static_cast<T**>(Malloc(max_e * sizeof(*e_ptr)));
      memcpy(tmp_vector->e_ptr, e_ptr + pos, n * sizeof(*e_ptr));
      tmp_vector->num_e = n;
    }
    return tmp_vector;
  }

}; // class vector

/**
 * Container to store simple types (can have constructor, but it should be fast)
 * that are simple and small, stores the objects and not the pointers (copy 
 * constructor must be implemented). The capacity is increased to be always the
 * power of two, the container capacity is never decreased. An initial
 * capacity value can be supplied to the constructor, this can be used to avoid
 * any further memory allocations during the life of the container.
 */
template<class T>
class dynamic_array {
private:
  size_t count;
  size_t capacity; // 0,1,2,4,8,...
  T* data;

  void clean_up();
  void copy_content(const dynamic_array& other);
public:

  dynamic_array() : count(0), capacity(0), data(NULL) {}
  // to avoid reallocations and copying
  dynamic_array(size_t init_capacity) : count(0), capacity(init_capacity), data(NULL) 
    { if (capacity>0) data = new T[capacity]; }
  dynamic_array(const dynamic_array& other) : count(0), capacity(0), data(NULL) { copy_content(other); }
  dynamic_array& operator=(const dynamic_array& other) { clean_up(); copy_content(other); return *this; }
  ~dynamic_array() { clean_up(); }

  bool operator==(const dynamic_array& other) const;
  bool operator!=(const dynamic_array& other) const { return (!(*this==other)); }

  /** Returns the number of elements in the container. */
  size_t size() const { return count; }

  /** Erases the entire container. */
  void clear() { clean_up(); }

  /** Appends the \a elem to the end of vector. */
  void add(const T& elem);

  /** Removes an element that is at index \a idx */
  void remove(size_t idx);

  /** Returns the <em>n</em>th element. The index of the first element is
   * zero. If no such index, then FATAL_ERROR occurs. */
  const T& operator[](size_t n) const {
    if (n>=count) FATAL_ERROR("dynamic_array::operator[] const: index overflow");
    return data[n];
  }

  /** Returns the <em>n</em>th element. The index of the first element is
   * zero. If no such index, then FATAL_ERROR occurs. */
  T& operator[](size_t n) {
    if (n>=count) FATAL_ERROR("dynamic_array::operator[]: index overflow");
    return data[n];
  }
}; // class dynamic_array

template<class T>
void dynamic_array<T>::clean_up()
{
  delete[] data;
  data = NULL;
  capacity = 0;
  count = 0;
}

template<class T>
void dynamic_array<T>::copy_content(const dynamic_array<T>& other)
{
  capacity = other.capacity;
  count = other.count;
  if (capacity>0) {
    data = new T[capacity];
    for (size_t i=0; i<count; i++) data[i] = other.data[i];
  }
}

template<class T>
bool dynamic_array<T>::operator==(const dynamic_array<T>& other) const
{
  if (count!=other.count) return false;
  for (size_t i=0; i<count; i++) if (!(data[i]==other.data[i])) return false;
  return true;
}

template<class T>
void dynamic_array<T>::add(const T& elem)
{
  if (data==NULL) {
    capacity = 1;
    count = 1;
    data = new T[1];
    data[0] = elem;
  } else {
    if (count<capacity) {
      data[count] = elem;
      count++;
    } else {
      // no more room, need to allocate new memory
      if (capacity==0) FATAL_ERROR("dynamic_array::add()");
      capacity *= 2;
      T* new_data = new T[capacity];
      for (size_t i=0; i<count; i++) new_data[i] = data[i];
      delete[] data;
      data = new_data;
      data[count] = elem;
      count++;
    }
  }
}

template<class T>
void dynamic_array<T>::remove(size_t idx)
{
  if (idx>=count) FATAL_ERROR("dynamic_array::remove(): index overflow");
  for (size_t i=idx+1; i<count; i++) data[i-1] = data[i];
  count--;
}

#endif // _Common_vector_HH
