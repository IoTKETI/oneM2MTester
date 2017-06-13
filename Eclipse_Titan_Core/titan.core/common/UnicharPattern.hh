/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baranyi, Botond – initial implementation
 *
 ******************************************************************************/

#ifndef UNICHARPATTERN_HH
#define UNICHARPATTERN_HH

#include "Quadruple.hh"

/** Helper class for handling case-insensitive universal charstring patterns
  * (this includes all patterns used in universal charstring templates and
  * universal charstring subtypes, and the universal charstring version of
  * the predefined function 'regexp', as long as they have the '@nocase' modifier)
  * 
  * Only one (global) instance of this class is created, which is used to convert
  * the uppercase letters in patterns and the strings matched by the patterns
  * to lowercase.
  *
  * The instance is initialized with a table at its construction, which contains
  * the case mappings of Unicode characters (read from the file CaseFolding.txt,
  * from the official Unicode site).
  *
  * This class does simple case foldings (from the folding types described in
  * CaseFolding.txt), so only the mappings with statuses 'C' and 'S' are used. */
class UnicharPattern {
  
  /** structure containing one character's mapping (linked list) */
  struct mapping_t {
    /** character mapped from (uppercase letter) */
    Quad from;
    /** character mapped to (lowercase letter) */
    Quad to;
    /** pointer to the next element in the list */
    mapping_t* next;
  };

  /** pointer to the head of the linked list of mappings */
  mapping_t* mappings_head;
  
  /** deletes the mappings list */
  void clean_up();
  
  /** finds and returns the mapping list element with the 'from' character 
    * equivalent to the parameter */
  mapping_t* find_mapping(const Quad& q) const;
  
public:

  /** constructor - reads the case mappings from a text file and stores them
    * in the linked list */
  UnicharPattern();
  
  /** destructor - deletes the list */
  ~UnicharPattern() { clean_up(); }
  
  /** converts the specified character to lowercase (if it's an uppercase letter),
    * and returns the result */
  Quad convert_quad_to_lowercase(const Quad& q) const;

  /** goes through the null-terminated regex string parameter and changes each 
    * uppercase letter to its lowercase equivalent
    * @param str a universal charstring in regex format (meaning that every universal
    * character is coded as 8 characters from 'A' to 'P', each representing a
    * hexadecimal digit in the universal character's code) */
  void convert_regex_str_to_lowercase(char* str) const;
};

/** The one instance of the universal charstring pattern helper class. */
extern UnicharPattern unichar_pattern;

#endif /* UNICHARPATTERN_HH */

