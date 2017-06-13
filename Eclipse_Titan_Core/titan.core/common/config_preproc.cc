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
 *
 ******************************************************************************/
#include "config_preproc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Solaris 10 on x86 may define ERR in /usr/include/sys/regset.h depending on compiler and flags.
// Workaround enum conflict with undef.
#undef ERR

void string_chain_add(string_chain_t **ec, char *s)
{
  string_chain_t *i = *ec, *new_ec;
  if (i != NULL) {
    for ( ; ; ) {
      if (!strcmp(i->str, s)) {
	Free(s);
	return;
      }
      if (i->next == NULL) break;
      i = i->next;
    }
  }
  new_ec = static_cast<string_chain_t*>( Malloc(sizeof(*new_ec)));
  new_ec->str = s;
  new_ec->next = NULL;
  if (i != NULL) i->next = new_ec;
  else *ec = new_ec;
}

char* string_chain_cut(string_chain_t **ec)
{
  string_chain_t *i = *ec;
  if (i != NULL) {
    char *s = i->str;
    *ec = i->next;
    Free(i);
    return s;
  } else return NULL;
}

/** search the position of a key; returns 1 if found, 0 otherwise. the
 *  position is returned in \a pos. */
static int string_map_pos(const string_map_t *map, const char *key, size_t *pos)
{
  size_t l=0, r=map->n;
  string_keyvalue_t **data=map->data;
  while(l<r) {
    size_t m=(l+r)/2;
    if(strcmp(data[m]->key, key)<0) l=m+1;
    else r=m;
  }
  if(l>=map->n) {
    *pos=map->n;
    return 0;
  }
  else {
    *pos=l;
    if(!strcmp(data[l]->key, key)) return 1;
    return 0;
  }
}

const char* string_map_add(string_map_t *map, char *key,
                           char *value, size_t value_len)
{
  size_t pos;
  if (string_map_pos(map, key, &pos)) {
    /* replacing the old value */
    Free(map->data[pos]->value);
    map->data[pos]->value = value;
    map->data[pos]->value_len = value_len;
    return map->data[pos]->key;
  } else {
    /* creating a new entry */
    map->n++;
    map->data = static_cast<string_keyvalue_t**>(Realloc(map->data, (map->n) * sizeof(*map->data)));
    memmove(map->data + pos + 1, map->data + pos,
            (map->n - pos - 1) * sizeof(*map->data));
    map->data[pos] = static_cast<string_keyvalue_t*>(Malloc(sizeof(**map->data)));
    map->data[pos]->key = key;
    map->data[pos]->value = value;
    map->data[pos]->value_len = value_len;
    return NULL;
  }
}

const char* string_map_get_bykey(const string_map_t *map,
                                 const char *key, size_t *value_len)
{
  size_t pos;
  const char* s;
  if(string_map_pos(map, key, &pos)) {
    *value_len=map->data[pos]->value_len;
    return map->data[pos]->value;
  }
  else if((s=getenv(key))) {
    *value_len=strlen(s);
    return s;
  }
  else {
    *value_len=0;
    return NULL;
  }
}

string_map_t* string_map_new(void)
{
  string_map_t *map = static_cast<string_map_t*>(Malloc(sizeof(string_map_t)));
  map->n=0;
  map->data=NULL;
  return map;
}

/* Used only for debugging purposes
void string_map_dump(const string_map_t *map)
{
  size_t i;
  fprintf(stderr, "--------\n- n: %u -\n--------\n", map->n);
  for(i=0; i<map->n; i++)
    fprintf(stderr, "%u: `%s' -> `%s' (%u)\n", i,
            map->data[i]->key, map->data[i]->value,
            map->data[i]->value_len);
  fprintf(stderr, "--------\n");
}
*/

void string_map_free(string_map_t *map)
{
  size_t i;
  for(i=0; i<map->n; i++) {
    Free(map->data[i]->key);
    Free(map->data[i]->value);
    Free(map->data[i]);
  }
  Free(map->data);
  Free(map);
}

char *get_macro_id_from_ref(const char *str)
{
  char *ret_val = NULL;
  if (str != NULL && str[0] == '$' && str[1] == '{') {
    size_t i = 2;
    /* skip over the whitespaces after the brace */
    while (str[i] == ' ' || str[i] == '\t') i++;
    if ((str[i] >= 'A' && str[i] <= 'Z') ||
        (str[i] >= 'a' && str[i] <= 'z')) {
      /* the first character of the id shall be a letter */
      do {
	ret_val = mputc(ret_val, str[i]);
	i++;
      } while ((str[i] >= 'A' && str[i] <= 'Z') ||
	       (str[i] >= 'a' && str[i] <= 'z') ||
	       (str[i] >= '0' && str[i] <= '9') ||
	        str[i] == '_');
      if (str[i] != ' ' && str[i] != '\t' && str[i] != ',' && str[i] != '}') {
	/* the next character after the id is not a whitespace or , or } */
	Free(ret_val);
	ret_val = NULL;
      }
    }
  }
  return ret_val;
}

int string_is_int(const char *str, size_t len)
{
  enum { INITIAL, FIRST, ZERO, MORE, ERR } state = INITIAL;
  /* state: expected characters
   * INITIAL: +, -, first digit
   * FIRST: first digit
   * MORE, ZERO: more digit(s)
   * ERR: error was found, stop
   */
  size_t i;
  if(str==NULL || str[0]=='\0') return 0;
  for (i = 0; str[i] != '\0'; i++) {
    char c = str[i];
    switch (state) {
    case INITIAL:
      if (c == '+' || c == '-') state = FIRST;
      else if (c == '0') state = ZERO;
      else if (c >= '1' && c <= '9') state = MORE;
      else state = ERR;
      break;
    case FIRST:
      if (c == '0') state = ZERO;
      else if (c >= '1' && c <= '9') state = MORE;
      else state = ERR;
      break;
    case ZERO:
      if (c >= '0' && c <= '9') {
        state = MORE;
      } else state = ERR;
      break;
    case MORE:
      if (c >= '0' && c <= '9') /* do nothing */;
      else state = ERR;
    default:
      break;
    }
    if (state == ERR) return 0;
  }
  if (state != ZERO && state != MORE) return 0;
  if(i!=len) return 0;
  return 1;
}

int string_is_float(const char *str, size_t len)
{
  enum { INITIAL, FIRST_M, ZERO_M, MORE_M, FIRST_F, MORE_F, INITIAL_E,
         FIRST_E, ZERO_E, MORE_E, ERR } state = INITIAL;
  /* state: expected characters
   * INITIAL: +, -, first digit of integer part in mantissa
   * FIRST_M: first digit of integer part in mantissa
   * ZERO_M, MORE_M: more digits of mantissa, decimal dot, E
   * FIRST_F: first digit of fraction
   * MORE_F: more digits of fraction, E
   * INITIAL_E: +, -, first digit of exponent
   * FIRST_E: first digit of exponent
   * ZERO_E, MORE_E: more digits of exponent
   * ERR: error was found, stop
   */
  size_t i;
  if(str==NULL || str[0]=='\0') return 0;
  for (i = 0; str[i] != '\0'; i++) {
    char c = str[i];
    switch (state) {
    case INITIAL:
      if (c == '+' || c == '-') state = FIRST_M;
      else if (c == '0') state = ZERO_M;
      else if (c >= '1' && c <= '9') state = MORE_M;
      else state = ERR;
      break;
    case FIRST_M:
      if (c == '0') state = ZERO_M;
      else if (c >= '1' && c <= '9') state = MORE_M;
      else state = ERR;
      break;
    case ZERO_M:
      if (c == '.') state = FIRST_F;
      else if (c == 'E' || c == 'e') state = INITIAL_E;
      else if (c >= '0' && c <= '9') {
        state = MORE_M;
      } else state = ERR;
      break;
    case MORE_M:
      if (c == '.') state = FIRST_F;
      else if (c == 'E' || c == 'e') state = INITIAL_E;
      else if (c >= '0' && c <= '9') /* do nothing */;
      else state = ERR;
      break;
    case FIRST_F:
      if (c >= '0' && c <= '9') state = MORE_F;
      else state = ERR;
      break;
    case MORE_F:
      if (c == 'E' || c == 'e') state = INITIAL_E;
      else if (c >= '0' && c <= '9') /* do nothing */;
      else state = ERR;
      break;
    case INITIAL_E:
      if (c == '+' || c == '-') state = FIRST_E;
      else if (c == '0') state = ZERO_E;
      else if (c >= '1' && c <= '9') state = MORE_E;
      else state = ERR;
      break;
    case FIRST_E:
      if (c == '0') state = ZERO_E;
      else if (c >= '1' && c <= '9') state = MORE_E;
      else state = ERR;
      break;
    case ZERO_E:
      if (c >= '0' && c <= '9') {
        state = MORE_E;
      } else state = ERR;
      break;
    case MORE_E:
      if (c >= '0' && c <= '9') /* do nothing */;
      else state = ERR;
    default:
      break;
    }
    if (state == ERR) return 0;
  }
  if (state != MORE_F && state != ZERO_E && state != MORE_E
      && state != ZERO_M && state != MORE_M)
    return 0;
  if(i!=len) return 0;
  return 1;
}

int string_is_id(const char *str, size_t len)
{
  size_t i;
  int has_hyphen, has_underscore;
  char first_char;
  if (len == 0) return 0;
  first_char = str[0];
  if ((first_char < 'a' || first_char > 'z') &&
      (first_char < 'A' || first_char > 'Z')) return 0;
  has_hyphen = 0;
  has_underscore = 0;
  for (i = 1; i < len; i++) {
    char c = str[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9')) {
      /* do nothing */
    } else if (c == '_') {
      if (has_hyphen) return 0;
      else has_underscore = 1;
    } else if (c == '-') {
      if (has_underscore || str[i - 1] == '-' || i == len - 1 ||
          first_char < 'a' || first_char > 'z') return 0;
      else has_hyphen = 1;
    } else return 0;
  }
  return 1;
}

int string_is_bstr(const char *str, size_t len)
{
  size_t i;
  for (i = 0; i < len; i++) {
    char c = str[i];
    if (c != '0' && c != '1') return 0;
  }
  return 1;
}

int string_is_hstr(const char *str, size_t len)
{
  size_t i;
  for (i = 0; i < len; i++) {
    char c = str[i];
    if ((c < '0' || c >'9') && (c < 'A' || c > 'F') && (c < 'a' || c > 'f'))
      return 0;
  }
  return 1;
}

int string_is_ostr(const char *str, size_t len)
{
  if (len % 2) return 0;
  else return string_is_hstr(str, len);
}

int string_is_hostname(const char *str, size_t len)
{
  enum { INITIAL, ALPHANUM, DOT, DASH, COLON, PERCENT } state = INITIAL;
  size_t i;
  for (i = 0; i < len; i++) {
    char c = str[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
	(c >= '0' && c <= '9')) {
      state = ALPHANUM;
    } else if (c == '.') {
      if (state == ALPHANUM) state = DOT;
      else return 0;
    } else if (c == ':') {
      if (state == INITIAL || state == ALPHANUM || state == COLON) state = COLON;
      else return 0;
    } else if (c == '%') {
      if (state == ALPHANUM) state = PERCENT;
      else return 0;
    } else if (c == '-' || c == '_') {
      if (state == INITIAL || state == DOT || state == COLON || state == PERCENT) return 0;
      else state = DASH;
    } else {
      return 0;
    }
  }
  if (state == ALPHANUM || state == DOT) return 1;
  else return 0;
}
