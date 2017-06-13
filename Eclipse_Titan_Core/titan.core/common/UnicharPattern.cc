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

#include "UnicharPattern.hh"
#include "pattern.hh"
#include "memory.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

//////////////////////////////////////////////
//////////// the global instance /////////////
//////////////////////////////////////////////
UnicharPattern unichar_pattern;

//////////////////////////////////////////////
////////////// helper functions //////////////
//////////////////////////////////////////////

/** removes spaces from the beginning and end of the input string and returns
  * the result */
static char* remove_spaces(char* str)
{
  if (str == NULL) {
    return NULL;
  }
  size_t len = strlen(str);
  size_t start = 0;
  while (isspace(str[start])) {
    ++start;
  }
  size_t end = len - 1;
  while (isspace(str[end])) {
    str[end] = '\0';
    --end;
  }
  return str + start;
}

/** Exception class
  *
  * Thrown when one of the characters processed by hexchar_to_char or
  * hexstr_to_char is not a hexadecimal digit */
class NotHexException {};

/** converts a character containing a hexadecimal digit to its numeric value */
static unsigned char hexchar_to_char(const char c)
{
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  else if (c >= 'A' && c <= 'F') {
    return c + 10 - 'A';
  }
  else if (c >= 'a' && c <= 'f') {
    return c + 10 - 'a';
  }
  throw NotHexException();
}

/** converts a string of two hexadecimal digits to the character the digits
  * represent */
static unsigned char hexstr_to_char(const char* hex_str)
{
  return (hexchar_to_char(hex_str[0]) << 4) | hexchar_to_char(hex_str[1]);
}

//////////////////////////////////////////////
// member functions of class UnicharPattern //
//////////////////////////////////////////////
UnicharPattern::UnicharPattern() : mappings_head(NULL)
{
  // if anything goes wrong while parsing the case mappings file, just delete the
  // partial results, display a warning, and treat all patterns as case-sensitive
  FILE* fp = NULL;
  const char* ttcn3_dir = getenv("TTCN3_DIR");
  char* mappings_file = NULL;
  if (ttcn3_dir != NULL) {
    size_t ttcn3_dir_len = strlen(ttcn3_dir);
    bool ends_with_slash = ttcn3_dir_len > 0 && ttcn3_dir[ttcn3_dir_len - 1] == '/';
    mappings_file = mprintf("%s%setc/CaseFolding.txt", ttcn3_dir,
      ends_with_slash ? "" : "/");
    fp = fopen(mappings_file, "r");
    if (fp == NULL) {
      // during the TITAN build the case mappings file has not been moved to the
      // installation directory yet; try its original location
      fp = fopen("../etc/CaseFolding.txt", "r");
    }
  }
  if (fp == NULL) {
    if (ttcn3_dir == NULL) {
      TTCN_pattern_warning("Environment variable TTCN3_DIR not present. "
        "Case-insensitive universal charstring patterns are disabled.\n");
    }
    else {
      TTCN_pattern_warning("Cannot open file '%s' for reading. "
        "Case-insensitive universal charstring patterns are disabled.\n",
        mappings_file);
    }
    Free(mappings_file);
    return;
  }
  
  Free(mappings_file);
  
  // this always points to the last element of the list
  mapping_t* mappings_tail = NULL;
  
  // read one line at a time
  char line[1024];
  while (fgets(line, sizeof(line), fp) != NULL) {
    // skip empty lines
    if (strcmp(line,"\n") == 0 || strcmp(line,"\r\n") == 0) {
      continue;
    }
    // ignore everything after the '#' (this is the 'comment' indicator)
    char* line_end = strchr(line, '#');
    if (line_end != NULL) {
      *line_end = '\0';
    }
    
    // each column ends with a ';', use that as the separator for strtok
    char* from_str = remove_spaces(strtok(line, ";"));
    size_t from_str_len = from_str != NULL ? strlen(from_str) : 0;
    if (from_str_len == 0) {
      // nothing but comments and spaces in this line
      continue;
    }
    // all character codes are 4 or 5 digits long
    if (from_str_len < 4 || from_str_len > 5) {
      TTCN_pattern_warning("Invalid format of case folding file (code column). "
        "Case-insensitive universal charstring patterns are disabled.\n");
      fclose(fp);
      clean_up();
      return;
    }
    char* status = remove_spaces(strtok(NULL, ";"));
    // the status is one character long
    if (status == NULL || strlen(status) != 1) {
      TTCN_pattern_warning("Invalid format of case folding file (status column). "
        "Case-insensitive universal charstring patterns are disabled.\n");
      fclose(fp);
      clean_up();
      return;
    }
    else if (status[0] != 'C' && status[0] != 'S') {
      // only use the lines with statuses 'C' and 'S', ignore the rest
      continue;
    }
    char* to_str = remove_spaces(strtok(NULL, ";"));
    size_t to_str_len = to_str != NULL ? strlen(to_str) : 0;
    if (to_str_len < 4 || to_str_len > 5) {
      TTCN_pattern_warning("Invalid format of case folding file (mapping column). "
        "Case-insensitive universal charstring patterns are disabled.\n");
      fclose(fp);
      clean_up();
      return;
    }
    
    // create the new element
    if (mappings_tail == NULL) {
      mappings_head = new mapping_t;
      mappings_tail = mappings_head;
    }
    else {
      mappings_tail->next = new mapping_t;
      mappings_tail = mappings_tail->next;
    }
    mappings_tail->next = NULL;
    
    // try to convert the extracted tokens to their character codes
    try {
      mappings_tail->from.set(0, from_str_len == 5 ? from_str[0] : 0,
        hexstr_to_char(from_str + from_str_len - 4),
        hexstr_to_char(from_str + from_str_len - 2));
      mappings_tail->to.set(0, to_str_len == 5 ? to_str[0] : 0,
        hexstr_to_char(to_str + to_str_len - 4),
        hexstr_to_char(to_str + to_str_len - 2));
    }
    catch (NotHexException) {
      // one of the tokens contained a non-hex character
      TTCN_pattern_warning("Invalid format of case folding file (character code). "
        "Case-insensitive universal charstring patterns are disabled.\n");
      fclose(fp);
      clean_up();
      return;
    }
  }
  fclose(fp);
}

void UnicharPattern::clean_up()
{
  while (mappings_head != NULL) {
    mapping_t* temp = mappings_head;
    mappings_head = mappings_head->next;
    delete temp;
  }
}

UnicharPattern::mapping_t* UnicharPattern::find_mapping(const Quad& q) const
{
  mapping_t* ptr = mappings_head;
  while (ptr != NULL) {
    if (ptr->from == q) {
      return ptr;
    }
    ptr = ptr->next;
  }
  return NULL;
}

Quad UnicharPattern::convert_quad_to_lowercase(const Quad& q) const
{
  mapping_t* mapping = find_mapping(q);
  if (mapping != NULL) {
    return mapping->to;
  }
  return q;
}

void UnicharPattern::convert_regex_str_to_lowercase(char* str) const
{
  if (mappings_head != NULL) {
    size_t len = strlen(str) / 8;
    for (size_t i = 0; i < len; ++i) {
      // the class 'Quad' contains the logic to convert to and from regex strings
      Quad q;
      q.set_hexrepr(str + 8 * i);
      mapping_t* mapping = find_mapping(q);
      if (mapping != NULL) {
        // this call actually saves the specified Quad's regex string to the
        // specified location in the string
        Quad::get_hexrepr(mapping->to, str + 8 * i);
      }
    }
  }
}
