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
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef _Common_ustring_HH
#define _Common_ustring_HH

#include <string.h>

class string;

/**
 * Class for handling universal (multi-byte) string values.
 */
class ustring {
public:
  struct universal_char {
    unsigned char group, plane, row, cell;
  };
private:
  /** string value structure */
  struct ustring_struct {
    unsigned int ref_count;
    size_t n_uchars;
    universal_char uchars_ptr[1];
  } *val_ptr;
  
  ustring(size_t n) : val_ptr(0) { init_struct(n); }

  void init_struct(size_t n_uchars);
  void enlarge_memory(size_t incr);
  void copy_value();
  void clean_up();

  /** Three-way lexicographical comparison of \a *this and \a s, much
   *  like \c strcmp. */
  int compare(const ustring& s) const;

public:

  /** The largest possible value of type size_t. That is, size_t(-1). */
  static const size_t max_string_len =
    (-1 - sizeof(ustring_struct)) / sizeof(universal_char) + 1;

  /** Constructs an empty string. */
  ustring() : val_ptr(0) { init_struct(0); }

  /** Constructs a single quadruple from the given values. */
  ustring(unsigned char p_group, unsigned char p_plane, unsigned char p_row,
          unsigned char p_cell);

  /** Constructs from an array of \a n quadruples starting at \a uc_ptr. */
  ustring(size_t n, const universal_char *uc_ptr);

  /** Constructs a universal string from \a s. */
  ustring(const string& s);
  
  /** Constructs a universal string from \a uid which contains \a n chars. */
  ustring(const char** uid, const int n);

  /** Copy constructor */
  ustring(const ustring& s) : val_ptr(s.val_ptr) { val_ptr->ref_count++;}

  /** The destructor. */
  ~ustring() { clean_up(); }

  /** Returns the size of the string. */
  inline size_t size() const { return val_ptr->n_uchars; }

  /** true if the string's size is 0. */
  inline bool empty() const { return val_ptr->n_uchars == 0; }

  /** Returns a pointer to an array of quadruples
   *  representing the string's contents.
   *  The array contains \a size() elements without any terminating symbol. */
  inline const universal_char *u_str() const { return val_ptr->uchars_ptr; }

  /** Erases the string. */
  void clear();

  /** Creates a string from a substring of \a *this.
   * The substring begins at character position \a pos,
   * and terminates at character position \a pos+n or at the end of
   * the string, whichever comes first. */
  ustring substr(size_t pos=0, size_t n=max_string_len) const;

  /** Replaces the \a n long substring of \a *this beginning
   *  at position \a pos with the string \a s. */
  void replace(size_t pos, size_t n, const ustring& s);

  /** Returns the printable representation of value stored in \a this.
   * The non-printable and special characters are escaped in the returned
   * string. */
  string get_stringRepr() const;

  /** Returns the charstring representation of value stored in \a this.
   *  quadruples will be converted into the form \q{group,plane,row,cell}
   */
  string get_stringRepr_for_pattern() const;

  /** Converts the value to hex form used by match and regexp in case of
   *  universal charstring parameters.
   */
  char* convert_to_regexp_form() const;
  
  /** extracts the section matched by a regexp and returns it
    * (the starting and ending indexes refer to the hex representation of the
    * string, which is 8 times longer) */
  ustring extract_matched_section(int start, int end) const;
  
  /** Assignment operator. */
  ustring& operator=(const ustring&);

  /** Returns the <em>n</em>th character.
   * The first character's position is zero.
   * If \a n >= size() then... Fatal error. */
  universal_char& operator[](size_t n);

  /** Returns the <em>n</em>th character.
   * The first character's position is zero.
   * If \a n >= size() then... Fatal error. */
  const universal_char& operator[](size_t n) const;

  /** String concatenation. */
  ustring operator+(const string& s2) const;

  /** String concatenation. */
  ustring operator+(const ustring& s2) const;

  /** Append \a s to \a *this. */
  ustring& operator+=(const string& s);

  /** Append \a s to \a *this. */
  ustring& operator+=(const ustring& s);

  /** String equality. */
  bool operator==(const ustring& s2) const;

  /** String inequality. */
  inline bool operator!=(const ustring& s2) const { return !(*this == s2); }

  /** String comparison. */
  inline bool operator<=(const ustring& s) const { return compare(s) <= 0; }

  /** String comparison. */
  inline bool operator<(const ustring& s) const { return compare(s) < 0; }

  /** String comparison. */
  inline bool operator>=(const ustring& s) const { return compare(s) >= 0; }

  /** String comparison. */
  inline bool operator>(const ustring& s) const { return compare(s) > 0; }

};

extern bool operator==(const ustring::universal_char& uc1,
  const ustring::universal_char& uc2);

extern bool operator<(const ustring::universal_char& uc1,
  const ustring::universal_char& uc2);

/** Converts the unicode string to UTF-8 format */
extern string ustring_to_uft8(const ustring&);

#endif // _Common_ustring_HH
