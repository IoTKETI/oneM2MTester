/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Cserveni, Akos
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kremer, Peter
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef _Common_string_HH
#define _Common_string_HH

#include <string.h>

class stringpool;
class ustring;

/**
 * Class for handling general string operations.
 */
class string {
  friend class stringpool;
  friend string operator+(const char *s1, const string& s2);

  /** string value structure */
  struct string_struct {
    unsigned int ref_count;
    size_t n_chars;
    char chars_ptr[sizeof(size_t)];
  } *val_ptr;

  string(size_t n) : val_ptr(0) { init_struct(n); }
  string(string_struct *ptr) : val_ptr(ptr) { val_ptr->ref_count++; }

  void init_struct(size_t n_chars);
  void copy_value_and_append(const char *s, size_t n);
  void replace(size_t pos, size_t n, const char *s, size_t s_len);
  static void clean_up(string_struct *ptr);

  /** Three-way lexicographical comparison of \a *this and \a s,
   * much like \c strcmp. */
  int compare(const string& s) const;

  /** Three-way lexicographical comparison of \a *this and \a s,
   * much like \c strcmp. */
  int compare(const char *s) const;

public:
  /** The largest possible value of type size_t. That is, size_t(-1). */
  static const size_t max_string_len = static_cast<size_t>(-1) -
    (sizeof(string_struct) - sizeof(size_t) + 1);

  /** Constructs an empty string. */
  string() : val_ptr(0) { init_struct(0); }

  /** Constructs a string containing single character \a c. */
  explicit string(char c) : val_ptr(0)
  { init_struct(1); val_ptr->chars_ptr[0] = c; }

  /** Constructs a string from \a s. */
  explicit string(const char *s);

  /** Constructs a string from \a s taking the first \a n characters. */
  string(size_t n, const char *s);

  /** Copy constructor */
  string(const string& s) : val_ptr(s.val_ptr) { val_ptr->ref_count++; }

  /** Constructs a string from ustring \a s. All characters of \a s must fit
   *  in one byte. */
  explicit string(const ustring& s);

  /** The destructor. */
  ~string() { clean_up(val_ptr); }

  /** Returns the size of the string. */
  size_t size() const { return val_ptr->n_chars; }

  /** true if the string's size is 0. */
  bool empty() const { return val_ptr->n_chars == 0; }

  /** true if the string contains a valid value of TTCN-3 type charstring
   * i.e. character code of every character is within the range 0..127 */
  bool is_cstr() const;

  /** Returns a pointer to a null-terminated array of characters
   *  representing the string's contents. */
  const char *c_str() const { return val_ptr->chars_ptr; }

  /** Erases the string; identical to \a *this="". */
  void clear();

  /** Creates a string from a substring of \a *this.
   * The substring begins at character position \a pos,
   * and terminates at character position \a pos+n or at the end of
   * the string, whichever comes first. */
  string substr(size_t pos=0, size_t n=max_string_len) const;

  /** Appends characters, or erases characters from the end,
   *  as necessary to make the string's length exactly \a n characters. */
  void resize(size_t n, char c = '\0');

  /** Replaces the \a n long substring of \a *this beginning
   *  at position \a pos with the string \a s. */
  void replace(size_t pos, size_t n, const string& s);

  /** Replaces the \a n long substring of \a *this beginning
   *  at position \a pos with the string \a s.
   *  @pre \a s must not be NULL */
  void replace(size_t pos, size_t n, const char *s);

  /** Searches for the character \a c, beginning at character
   * position \a pos. If not found, returns size(). */
  size_t find(char c, size_t pos=0) const;

  /** Searches for a null-terminated character array
   * as a substring of \a *this, beginning at character \a pos
   * of \a *this. If not found, returns size(). */
  size_t find(const char* s, size_t pos=0) const;

  /** Searches backward for the character \a c,
   * beginning at character position \a min(pos, size()). */
  size_t rfind(char c, size_t pos=max_string_len) const;

  /** Searches backward for a null-terminated character array
   * as a substring of \a *this, beginning at character
   * min(pos, size())
   size_t rfind(const char* s, size_t pos=max_string_len) const; */

  /**
   * Returns the first position \a i in the range [first, last)
   * such that <tt>pred((*this)[i]) != 0</tt>.
   * Returns \a last if no such position exists.
   *
   * I want to use this function like this:
   * \code size_t pos=str.find_if(0, str.size(), islower); \endcode
   */
  size_t find_if(size_t first, size_t last,
                 int (*pred)(int)) const;

  /** Returns whether \a c is a whitespace character */
  static bool is_whitespace(unsigned char c);
  /** Returns whether \a c is a printable character */
  static bool is_printable(unsigned char c);
  /** Appends the (possibly escaped) printable representation of character
   * \a c to \a this. Applicable only if \a c is a printable character. */
  void append_stringRepr(char c);
  /** Returns the printable representation of charstring value stored in
   * \a this. The non-printable and special characters are escaped in the
   * returned string. */
  string get_stringRepr() const;

  /** Assignment operator. */
  string& operator=(const string&);

  /** Assign a null-terminated character array to a string. */
  string& operator=(const char *s);

  /** Returns the <em>n</em>th character.
   * The first character's position is zero.
   * If \a n >= size() then... Fatal error. */
  char& operator[](size_t n);

  /** Returns the <em>n</em>th character.
   * The first character's position is zero.
   * If \a n >= size() then... Fatal error. */
  char operator[](size_t n) const;

  /** String concatenation. */
  string operator+(const string& s) const;

  /** String concatenation. */
  string operator+(const char *s) const;

  /** Append \a s to \a *this. */
  string& operator+=(const string& s);

  /** Append \a s to \a *this. */
  string& operator+=(const char *s);

  /** Append \a c to \a *this. */
  string& operator+=(char c) { copy_value_and_append(&c, 1); return *this; }

  /** String equality. Equivalent to this->compare(string(s)) == 0. */
  bool operator==(const char *s) const;

  /** String equality. Equivalent to this->compare(s2) == 0. */
  bool operator==(const string& s) const;

  /** String inequality. Equivalent to this->compare(s2) != 0. */
  inline bool operator!=(const string& s) const { return !(*this == s); }

  /** String inequality. Equivalent to this->compare(string(s2)) != 0. */
  inline bool operator!=(const char *s) const { return !(*this == s); }

  /** String comparison. */
  inline bool operator<=(const string& s) const { return compare(s) <= 0; }

  /** String comparison. */
  inline bool operator<=(const char *s) const { return compare(s) <= 0; }

  /** String comparison. */
  inline bool operator<(const string& s) const { return compare(s) < 0; }

  /** String comparison. */
  inline bool operator<(const char *s) const { return compare(s) < 0; }

  /** String comparison. */
  inline bool operator>=(const string& s) const { return compare(s) >= 0; }

  /** String comparison. */
  inline bool operator>=(const char *s) const { return compare(s) >= 0; }

  /** String comparison. */
  inline bool operator>(const string& s) const { return compare(s) > 0; }

  /** String comparison. */
  inline bool operator>(const char *s) const { return compare(s) > 0; }
};

/** String concatenation. */
extern string operator+(const char *s1, const string& s2);

/** String equality. */
inline bool operator==(const char *s1, const string& s2) { return s2 == s1; }

/** String inequality. */
inline bool operator!=(const char *s1, const string& s2) { return !(s2 == s1); }

/**
 * Class for preserving the memory allocated for temporary strings within a
 * statement block.
 */
class stringpool {
  string::string_struct **string_list;
  size_t list_size; /**< The number of strings allocated */
  size_t list_len;  /**< The number of strings in use. At most \a list_size */
  /** Copy constructor not implemented */
  stringpool(const stringpool& p);
  /** Assignment not implemented */
  stringpool& operator=(const stringpool& p);

public:
  /** Drops all strings from the pool. */
  void clear();
  /** Default constructor: creates an empty pool */
  stringpool() : string_list(NULL), list_size(0), list_len(0) { }
  /** Destructor: drops the contents of the entire pool */
  ~stringpool() { clear(); }
  /** Adds the current value of string \a s to the pool and returns its
   * pointer. The pointer is usable until \a this stringpool is destructed. */
  const char *add(const string& s);
  /** Returns the number of strings in the pool */
  size_t size() const { return list_len; }
  /** Returns the pointer to the \a n-th string of the pool */
  const char *get_str(size_t n) const;
  /** Returns a newly constructed string containing the \a n-th element */
  string get_string(size_t n) const;
};

#endif // _Common_string_HH
