/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Beres, Szabolcs
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_config_preproc_H
#define _Common_config_preproc_H

#include <string>

#include "memory.h"

  extern void config_preproc_error(const char *error_str, ...)
       __attribute__ ((__format__ (__printf__, 1, 2)));

#ifdef __cplusplus
extern "C" {
#endif

  extern void path_error(const char *fmt, ...)
       __attribute__ ((__format__ (__printf__, 1, 2)));

#ifdef __cplusplus
} /* extern "C" */
#endif

  extern std::string get_cfg_preproc_current_file();
  extern int config_preproc_yylineno;

  /** this struct is used to maintain a list of config files */
  typedef struct string_chain_t {
    char *str;
    struct string_chain_t *next;
  } string_chain_t;

  /** adds a new string to the end of chain (reference), if it is not
   *  contained by the chain */
  void string_chain_add(string_chain_t **ec, char *s);
  /** cuts the head of the chain (reference!) and returns that
   *  string */
  char* string_chain_cut(string_chain_t **ec);

  /** struct to store string key-value pairs. the value can contain
   *  null-characters so we store also the length. */
  typedef struct string_keyvalue_t {
    char *key;
    char *value;
    size_t value_len;
  } string_keyvalue_t;

  /** an array. keep it sorted */
  typedef struct string_map_t {
    size_t n;
    string_keyvalue_t **data;
  } string_map_t;

  /** adds a new key-value pair. if the key exists, it will be
   *  overwritten, and the return value is the (old) key. */
  const char* string_map_add(string_map_t *map, char *key,
                             char *value, size_t value_len);

  /** returns NULL if no such key. the length of value is returned in
   *  \a value_len */
  const char* string_map_get_bykey(const string_map_t *map, const char *key,
                                   size_t *value_len);

  /** constructor */
  string_map_t* string_map_new(void);
  /** destructor */
  void string_map_free(string_map_t *map);

  /** Parses out and returns the macro identifier from macro reference \a str.
   * The input shall be in format "${<macro_id>,something}", NULL pointer is
   * returned otherwise. Whitespaces are allowed anywhere within the braces.
   * The returned string shall be deallocated by the caller using Free(). */
  char *get_macro_id_from_ref(const char *str);

  /** Entry point for preprocessing config files. 
   *
   *  @param [in] filename the main config file
   *  @param [out] filenames main config plus all the included files
   *  @param [out] defines the macro definitions
   *  @return 1 if there were errors, 0 otherwise.
   */
  int preproc_parse_file(const char *filename, string_chain_t **filenames,
                         string_map_t **defines);

  int string_is_int(const char *str, size_t len);
  int string_is_float(const char *str, size_t len);
  int string_is_id(const char *str, size_t len);
  int string_is_bstr(const char *str, size_t len);
  int string_is_hstr(const char *str, size_t len);
  int string_is_ostr(const char *str, size_t len);
  int string_is_hostname(const char *str, size_t len);

#endif /* _Common_config_preproc_H */
