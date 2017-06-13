/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Godar, Marton
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef XSTRING_HH_
#define XSTRING_HH_

#include "../common/memory.h"

class Mstring {
  expstring_t text;

public:
  /**
   * Construct Mstring object
   */

  /**
   * Content is initialized to an empty (non-NULL) Mstring via memptystr()
   */
  Mstring();

  /**
   * Content is initialized to a copy of the Mstring object str.
   */
  Mstring(const Mstring & str);

  /**
   * Content is initialized to a copy of the string
   * formed by the null-terminated character sequence (C string) pointed by s.
   * The length of the character sequence is determined by the first occurrence of a null character.
   * If s is NULL, content is initialized to an empty Mstring
   */
  explicit Mstring(const char * s);

  /**
   * Content is initialized to a single character c.
   */
  explicit Mstring(char c);

  /** Construct from \a len characters at \a s (may not be null-terminated) */
  Mstring(size_t len, const char *s);

  /**
   * Destruct Mstring object
   */
  ~Mstring();

  /**
   * Test if Mstring is empty
   * true if the Mstring size is 0
   * false otherwise
   */
  bool empty() const;

  /**
   * Return length of Mstring
   */
  size_t size() const;

  /**
   * The Mstring content is set to an empty Mstring,
   * erasing any previous content and thus leaving its size at 0 characters.
   */
  void clear();

  /**
   * Generates a null-terminated sequence of characters (c-string)
   * with the same content as the Mstring object and
   * returns it as a pointer to an array of characters.
   */
  const char * c_str() const;

  /**
   * The function deletes one character at position pos from the Mstring content
   * Size is reduced by one
   */
  void eraseChar(size_t pos);

  /**
   * The function inserts one character before position into the Mstring content
   * Size is increments by one
   */
  void insertChar(size_t pos, char c);

  /**
   * Look for s Mstring content
   * true if s is found
   * false otherwise
   */
  bool isFound(const Mstring & s) const;

  /**
   * Look for s c-string content
   * true if s is found
   * false otherwise
   */
  bool isFound(const char * s) const;

  /**
   * Look for c character content
   * true if s is found
   * false otherwise
   */
  bool isFound(char c) const;

  /**
   * Look for c-string content
   * and returns a pointer to the first
   * character where the matching found,
   * returns null otherwise
   */
  char * foundAt(const char * c) const;

  /**
   * The first character of the Mstring is set to uppercase
   */
  void setCapitalized();

  /**
   * The first character of the Mstring is set to lowercase
   */
  void setUncapitalized();

  /**
   * Creates a new Mstring object
   * Looks for the first occurence of the give character (delimiter)
   * If delimiter is found: * the prefix is found in the string *
   * 		the content of the new object will be the part before the found char
   * If delimiter is not found: * the prefix is not found in the string *
   * 		the new object will be an empty Mstring
   */
  Mstring getPrefix(const char delimiter) const;

  /**
   *	Creates a new Mstring object
   * Looks for the last occurence of the give character (delimiter)
   * If delimiter is found: * the prefix is found in the string *
   * 		the content of the new object will be the part after the found char
   * If delimiter is not found: * the prefix is not found in the string *
   * 		the new object will be the copy of this Mstring
   */
  Mstring getValueWithoutPrefix(const char delimiter) const;

  /**
   * Remove all whitespace characters (' ', '\n', '\t', '\r')
   * from the begining or the end of the Mstring content
   */
  void removeWSfromBegin();
  void removeWSfromEnd();

  /**
   * Get character in string
   * Returns a reference the character at position pos in the string.
   */
  char & operator[](size_t pos);
  const char & operator[](size_t pos) const;

  /**
   * Mstring assignment
   * Sets a copy of the argument as the new content for the string object.
   * The previous content is dropped.
   */
  Mstring & operator=(const Mstring & str);
  Mstring & operator=(const char * s);
  Mstring & operator=(char c);

  const Mstring * operator*() const {
    return this;
  }

  /**
   * Append to Mstring
   * Appends a copy of the argument to the Mstring content.
   * The new Mstring content is the content existing in the string object before the call
   * followed by the content of the argument.
   */
  Mstring & operator+=(const Mstring & str);
  Mstring & operator+=(const char * s);
  Mstring & operator+=(char c);

  /**
   * String comparison operators
   * These overloaded global operator functions perform the appropriate comparison operation between lhs and rhs.
   *
   */
  friend bool operator==(const Mstring & lhs, const Mstring & rhs);
  friend bool operator==(const char * lhs, const Mstring & rhs);
  friend bool operator==(const Mstring & lhs, const char * rhs);

  friend bool operator!=(const Mstring & lhs, const Mstring & rhs);
  friend bool operator!=(const char * lhs, const Mstring & rhs);
  friend bool operator!=(const Mstring & lhs, const char * rhs);

  friend bool operator<(const Mstring & lhs, const Mstring & rhs);
  friend bool operator<(const char * lhs, const Mstring & rhs);
  friend bool operator<(const Mstring & lhs, const char * rhs);

  friend bool operator>(const Mstring & lhs, const Mstring & rhs);
  friend bool operator>(const char * lhs, const Mstring & rhs);
  friend bool operator>(const Mstring & lhs, const char * rhs);

  friend bool operator<=(const Mstring & lhs, const Mstring & rhs);
  friend bool operator<=(const char * lhs, const Mstring & rhs);
  friend bool operator<=(const Mstring & lhs, const char * rhs);

  friend bool operator>=(const Mstring & lhs, const Mstring & rhs);
  friend bool operator>=(const char * lhs, const Mstring & rhs);
  friend bool operator>=(const Mstring & lhs, const char * rhs);
};

/*
 * Add strings
 * Returns an Mstring object whose contents are the combination of the content of lhs followed by those of rhs.
 */
const Mstring operator+(const Mstring & lhs, const Mstring & rhs);
const Mstring operator+(const char * lhs, const Mstring & rhs);
const Mstring operator+(char lhs, const Mstring & rhs);
const Mstring operator+(const Mstring & lhs, const char * rhs);
const Mstring operator+(const Mstring & lhs, char rhs);

bool operator==(const Mstring & lhs, const Mstring & rhs);
bool operator==(const char * lhs, const Mstring & rhs);
bool operator==(const Mstring & lhs, const char * rhs);

bool operator!=(const Mstring & lhs, const Mstring & rhs);
bool operator!=(const char * lhs, const Mstring & rhs);
bool operator!=(const Mstring & lhs, const char * rhs);

bool operator<(const Mstring & lhs, const Mstring & rhs);
bool operator<(const char * lhs, const Mstring & rhs);
bool operator<(const Mstring & lhs, const char * rhs);

bool operator>(const Mstring & lhs, const Mstring & rhs);
bool operator>(const char * lhs, const Mstring & rhs);
bool operator>(const Mstring & lhs, const char * rhs);

bool operator<=(const Mstring & lhs, const Mstring & rhs);
bool operator<=(const char * lhs, const Mstring & rhs);
bool operator<=(const Mstring & lhs, const char * rhs);

bool operator>=(const Mstring & lhs, const Mstring & rhs);
bool operator>=(const char * lhs, const Mstring & rhs);
bool operator>=(const Mstring & lhs, const char * rhs);

extern const Mstring empty_string;

#endif /* XSTRING_HH_ */
