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
 *   Beres, Szabolcs
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *
 ******************************************************************************/

#ifndef TITANVECTOR_H_
#define TITANVECTOR_H_

#include "Types.h"
#include <stddef.h>

#ifndef PROF_MERGE
#include "Error.hh"
#else
// there's no point in including Error.hh and all the includes that come with it
// when building the profiler merge tool, just use this simple error function
#include <stdio.h>
#include <stdarg.h>
void TTCN_error(const char *fmt, ...)
{
  va_list parameters;
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  putc('\n', stderr);
  fflush(stderr);
}
#endif

// Not invented here 
template<typename T>
class Vector {
private:
  size_t cap;
  size_t nof_elem;
  T* data;

  void copy(const Vector<T>&);
public:
  explicit Vector(size_t p_capacity = 4);

  explicit Vector(const Vector<T>& other);
  ~Vector();

  Vector<T>& operator=(const Vector<T>& rhs);

  // Capacity
  size_t size() const { return nof_elem; }
  void resize(size_t new_size, T element = T());
  size_t capacity() const { return cap; }
  boolean empty() const { return nof_elem == 0; }
  void reserve(size_t n);
  void shrink_to_fit();
  T* data_ptr() const { return data; }

  // Element access
  T& operator[](size_t idx);
  const T& operator[](size_t idx) const;
  T& at(size_t idx);
  const T& at(size_t idx) const;
  T& front() { return at(0); }
  const T& front() const { return at(0); }
  T& back() { return at(nof_elem - 1); }
  const T& back() const { return at(nof_elem - 1); }
  // This could be used better with iterators
  void erase_at(size_t idx);

  // Modifiers
  void push_back(const T& element);
  void pop_back();
  void clear();
};

template<typename T>
Vector<T>::Vector(size_t p_capacity)
  : cap(p_capacity), nof_elem(0) {

    data = new T[cap];
    if (!data) {
      TTCN_error("Internal error: new returned NULL");
    }
}

template<typename T>
Vector<T>::Vector(const Vector<T>& other) {
  copy(other);
}

template<typename T>
Vector<T>& Vector<T>::operator=(const Vector<T>& rhs) {
  if (this == &rhs) {
    return *this;
  }

  clear();
  delete[] data;

  copy(rhs);

  return *this;
}

template<typename T>
void Vector<T>::copy(const Vector<T>& other) {
  cap = other.cap;
  data = new T[cap];
  if (!data) {
     TTCN_error("Internal error: new returned NULL");
  }

  for (size_t i = 0; i < other.nof_elem; ++i) {
    data[i] = other.data[i];
  }

  nof_elem = other.nof_elem;
}

template<typename T>
Vector<T>::~Vector() {
  clear();
  delete[] data;
}

template<typename T>
void Vector<T>::resize(size_t new_size, T element) {
  if (new_size > nof_elem) {
    reserve(new_size);
    while (nof_elem < new_size) {
      data[nof_elem++] = element;
    }
    return;
  }

  nof_elem = new_size;
}

template<typename T>
void Vector<T>::reserve(size_t new_size) {
  if (new_size <= cap) {
    return;
  }

  cap = new_size;
  T* data_tmp = new T[cap];
  if (!data_tmp) {
      TTCN_error("Internal error: new returned NULL");
  }
  for (size_t i = 0; i < nof_elem; ++i) {
    data_tmp[i] = data[i];
  }

  delete[] data;
  data = data_tmp;
}

// Modifiers
template<typename T>
void Vector<T>::push_back(const T& element) {
  if (nof_elem == cap) {
    size_t new_cap = (cap == 0 ? 4 : (cap * 2));
    reserve(new_cap);
  }
 
  data[nof_elem++] = element;
}

template<typename T>
const T& Vector<T>::at(size_t idx) const {
  if (idx >= nof_elem) {
     TTCN_error("Internal error: Vector over-indexing.");
  }

  return data[idx];
}

template<typename T>
T& Vector<T>::at(size_t idx) {
  if (idx >= nof_elem) {
     TTCN_error("Internal error: Vector over-indexing.");
  }

  return data[idx];
}

template<typename T>
const T& Vector<T>::operator[](size_t idx) const {
  return at(idx);
}

template<typename T>
T& Vector<T>::operator[](size_t idx) {
  return at(idx);
}

template<typename T>
void Vector<T>::erase_at(size_t idx) {
  if (idx >= nof_elem) {
    TTCN_error("Internal error: Vector over-indexing.");
  }

  while (idx < nof_elem - 1) {
    data[idx] = data[idx + 1];
    ++idx;
  }

  --nof_elem;
}

template<typename T>
void Vector<T>::shrink_to_fit() {
  if (nof_elem == cap) {
    return;
  }

  cap = nof_elem;
  T* data_tmp = new T[nof_elem];
  if (!data_tmp) {
    TTCN_error("Internal error: new returned NULL");
  }
  for (size_t i = 0; i < nof_elem; ++i) {
    data_tmp[i] = data[i];
  }
  delete[] data;
  data = data_tmp;
}

template<typename T>
void Vector<T>::clear() {
  nof_elem = 0;
}

#endif /* TITANVECTOR_H_ */
