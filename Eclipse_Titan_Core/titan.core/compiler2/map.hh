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
 *   Koppany, Csaba
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_map_HH
#define _Common_map_HH

#include "error.h"
#include "../common/dbgnew.hh"

/**
 * This container associates a \e key with its elements, and is
 * optimized to access the elements referenced by their keys.
 * The keys -- in contrast with the elements -- are owned by
 * the container. The keys in this container are unique.
 *
 * Accessing an element by its key has logarithmic cost.
 * Adding or removing an element has linear cost (proportional to the number
 * of elements in the map).
 * \pre
 * \arg The type of the key (<em>T_key</em>) must be a type which has
 * equality operator (==)  and less-than operator (<). Other
 * comparison operators are not assumed.
 * \arg \a T_key must have copy constructor and destructor.
 * \arg See also
 * \ref container_concepts "General rules and concepts about containers".
 *
 * There is a possibility to iterate through all
 * elements using integral indices. This index of an element can
 * change when inserting/deleting other elements. Accessing elements
 * by this integral index is not cost-optimal.
 *
 * \ingroup containers
 */
template<class T_key, class T>
class map {

private:

  struct map_struct {
    T_key   key;
    T      *dat;
    map_struct(const T_key& p_key, T *p_dat)
      : key(p_key), dat(p_dat) { }
    map_struct(const map_struct& other)
    : key(other.key), dat(other.dat) {}
  private:
    map_struct& operator=(const map_struct& other);
  };

  size_t num_m; ///< Number of elements (size)
  size_t max_m; ///< Available storage (capacity)
  /// Cache for the last search result.
  /// This will remember the index of the element last searched for;
  /// thus after checking the existence of the item, reaching it
  /// won't be logarithmical but constant
  mutable size_t last_searched_key;
  /// Array of pointers to map data. It is kept sorted (ascending) by keys.
  map_struct **m_ptr;

  static const size_t initial_size = 1, increment_factor = 2;

  /** Copy constructor: DO NOT IMPLEMENT! */
  map(const map&);
  /** Copy assignment: DO NOT IMPLEMENT! */
  map& operator=(const map&);

public:

  static const size_t max_map_length = static_cast<size_t>( -1 );

  /** Creates an empty map. */
  map() : num_m(0), max_m(0), last_searched_key(0), m_ptr(NULL) { }

  /** Deallocates its memory, including the keys.
   * If the container is not empty, FATAL_ERROR occurs,
   * so before destructing, clear() must be invoked explicitly.
   */
  ~map() {
    if (num_m > 0) FATAL_ERROR("map:~map(): map is not empty");
    Free(m_ptr);
  }

  /** Returns the number of elements in the container. */
  size_t size() const { return num_m; }

  /** Returns true if the container has no elements. */
  bool empty() const { return num_m == 0; }

  /** Erases the entire container. */
  void clear() {
    for (size_t r = 0; r < num_m; r++) delete m_ptr[r];
    num_m = 0;
    last_searched_key = 0;
  }

  /** Adds the elem \a T identified by \a key to the container.
   * If an element with the given key already exists, FATAL_ERROR occurs. */
  void add(const T_key& key, T *elem) {
    size_t l = 0;
    if (num_m > 0) {
      size_t r = num_m - 1;
      while (l < r) { // binary search
	size_t m = l + (r - l) / 2;
	if (m_ptr[m]->key < key) l = m + 1;
	else r = m;
      }
      if (m_ptr[l]->key < key) l++;
      else if (m_ptr[l]->key == key)
	FATAL_ERROR("map::add(): key already exists");
      if (num_m >= max_m) {
	// Array is full
	if (num_m > max_m) FATAL_ERROR("map::add(): num_m > max_m");
	if (max_m <= max_map_length / increment_factor)
	  max_m *= increment_factor; // room for doubling
	else if (max_m < max_map_length) max_m = max_map_length;
	else FATAL_ERROR("map::add(): cannot enlarge map");
	m_ptr = static_cast<map_struct**>
	  (Realloc(m_ptr, max_m * sizeof(*m_ptr)));
      }
      memmove(m_ptr + l + 1, m_ptr + l, (num_m - l) * sizeof(*m_ptr));
    } else if (m_ptr == NULL) {
      max_m = initial_size;
      m_ptr = static_cast<map_struct**>(Malloc(initial_size * sizeof(*m_ptr)));
    }
    m_ptr[l] = new map_struct(key, elem);
    num_m++;
    last_searched_key = num_m;
  }

  /** Returns the index of k within *m_ptr[] or num_m if key was not found */
  size_t find_key(const T_key& k) const {
    if (last_searched_key < num_m && m_ptr[last_searched_key]->key == k)
      return last_searched_key;
    else if (num_m == 0) return 0;
    size_t l = 0, r = num_m - 1;
    while (l < r) {
      size_t m = l + (r - l) / 2;
      if (m_ptr[m]->key < k) l = m + 1;
      else r = m;
    }
    if (m_ptr[l]->key == k) last_searched_key = l;
    else last_searched_key = num_m;
    return last_searched_key;
  }

  /** Erases the element identified by \a key. If no such element,
   * then silently ignores the request. */
  void erase(const T_key& key) {
    size_t n = find_key(key);
    if (n < num_m) {
      delete m_ptr[n];
      num_m--;
      memmove(m_ptr + n, m_ptr + n + 1, (num_m - n) * sizeof(*m_ptr));
      last_searched_key = num_m;
    }
  }

  /** Returns true if an element with the given \a key
   * already exists. */
  bool has_key(const T_key& key) const {
    return find_key(key) < num_m;
  }

  /** Returns the copy of the key contained in the map.
   *
   * The copy of the key may be longer-lived than the argument.
   * They are otherwise identical (operator== would return true).
   *
   * @param \a key
   * @return copy of \a key contained in the map
   * @pre key must exist in the map
   */
  const T_key& get_key(const T_key& key) const {
    size_t n = find_key(key);
    if (n >= num_m) FATAL_ERROR("map::get_key() const: key not found");
    return m_ptr[n]->key;
  }

  /** Returns the element identified by \a key.
   * If no such element, then a FATAL_ERROR occurs. */
  T *operator[](const T_key& key) const {
    size_t n = find_key(key);
    if (n >= num_m) FATAL_ERROR("map::operator[]() const: key not found");
    return m_ptr[n]->dat;
  }

  /** Returns the element identified by \a key.
   * If no such element, then a FATAL_ERROR occurs.
   * \note This member can not be used to add new elements to
   * the collection.
   */
  T*& operator[](const T_key& key) {
    size_t n = find_key(key);
    if (n >= num_m) FATAL_ERROR("map::operator[]() const: key not found");
    return m_ptr[n]->dat;
  }

  /** Returns the <em>n</em>th element. This is used to iterate through
   * the ordered list of elements. Elements are ordered by their keys.
   * If \a n >= size(), then a FATAL_ERROR occurs.
   * The key of the element is accessible via get_nth_key(). */
  T *get_nth_elem(size_t n) const {
    if (n >= num_m) FATAL_ERROR("map::get_nth_elem() const: index overflow");
    /* do not break the previous line, suncc doesn't like it... */
    return m_ptr[n]->dat;
  }

  /** Returns the <em>n</em>th element. This is used to iterate through
   * the elements. If \a n >= size(), then a FATAL_ERROR occurs.
   * The key of the element is accessible via get_nth_key(). */
  T*& get_nth_elem(size_t n) {
    if (n >= num_m) FATAL_ERROR("map::get_nth_elem(): index overflow");
    return m_ptr[n]->dat;
  }

  /** Returns the key of the <em>n</em>th element.
   * This is used to iterate through the keys of elements.
   * If \a n >= size(), then a FATAL_ERROR occurs.
   * \note There is only \c const version of this member. If you
   * want to modify the key of an element, you have to erase it from
   * the container, then add with the new key.
   */
  const T_key& get_nth_key(size_t n) const {
    if (n >= num_m) FATAL_ERROR("map::get_nth_key() const: index overflow");
    return m_ptr[n]->key;
  }
};

#endif // _Common_map_HH
