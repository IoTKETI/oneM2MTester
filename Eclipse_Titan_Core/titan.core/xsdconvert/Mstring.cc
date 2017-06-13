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
#include "../common/dbgnew.hh"
// dbgnew must come before Mstring.hh which includes common/memory.hh
#include "Mstring.hh"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h> // only for abort

const Mstring empty_string;

Mstring::Mstring()
: text(memptystr()) {
}

Mstring::Mstring(const Mstring & other)
: text(mcopystr(other.text)) {
}

Mstring::Mstring(const char * s)
: text(mcopystr(s)) {
}

Mstring::Mstring(size_t len, const char *s)
: text((expstring_t) Malloc(len + 1)) {
  memcpy(text, s, len);
  text[len] = 0;
}

Mstring::Mstring(char c)
: text(memptystr()) {
  text = mputc(text, c);
}

Mstring::~Mstring() {
  Free(text);
}

bool Mstring::empty() const {
  return mstrlen(text) == 0;
}

size_t Mstring::size() const {
  return mstrlen(text);
}

void Mstring::clear() {
  Free(text);
  text = memptystr();
}

const char * Mstring::c_str() const {
  return text;
}

void Mstring::eraseChar(size_t pos) {
  Mstring temp;
  for (size_t i = 0; i != size(); ++i)
    if (i != pos) temp += text[i];
  Free(text);
  text = mcopystr(temp.text);
}

void Mstring::insertChar(size_t pos, char c) {
  Mstring temp;
  for (size_t i = 0; i != size(); ++i)
    if (i == pos) {
      temp += c;
      temp += text[i];
    } else temp += text[i];
  Free(text);
  text = mcopystr(temp.text);
}

bool Mstring::isFound(const Mstring & s) const {
  return strstr(text, s.text);
}

bool Mstring::isFound(const char * s) const {
  return strstr(text, s);
}

bool Mstring::isFound(char c) const {
  return strchr(text, c);
}

char * Mstring::foundAt(const char * s) const {
  return strstr(text, s);
}

void Mstring::setCapitalized() {
  text[0] = static_cast<char>( toupper(text[0] ));
}

void Mstring::setUncapitalized() {
  text[0] = static_cast<char>( tolower(text[0] ));
}

Mstring Mstring::getPrefix(const char delimiter) const {
  Mstring result;
  char * pos = strchr(text, delimiter);
  if (pos != NULL) for (int i = 0; text + i != pos; ++i) result += text[i];
  return result;
}

Mstring Mstring::getValueWithoutPrefix(const char delimiter) const {
  char * pos = strrchr(text, delimiter);
  if (pos != NULL) return Mstring(pos + 1);
  else return *this;
}

void Mstring::removeWSfromBegin() {
  size_t i = 0;
  size_t s = mstrlen(text);
  for (; i < s; ++i)
    if (!isspace(static_cast<const unsigned char>( text[i] ))) break;
  memmove(text, text + i, s - i);
  text = mtruncstr(text, s - i);
}

void Mstring::removeWSfromEnd() {
  size_t i = mstrlen(text);
  for (; i > 0; --i)
    if (!isspace(static_cast<const unsigned char>( text[i - 1] ))) break;
  text = mtruncstr(text, i);
}

char & Mstring::operator[](size_t pos) {
  size_t s = mstrlen(text);
  if (pos < s)
    return text[pos];
  else {
    fputs("String index overflow\n", stderr);
    abort();
  }
}

const char & Mstring::operator[](size_t pos) const {
  size_t s = mstrlen(text);
  if (pos < s)
    return text[pos];
  else {
    fputs("String index overflow\n", stderr);
    abort();
  }
}

Mstring & Mstring::operator=(const Mstring & other) {
  if (&other != this) {
    Free(text);
    text = mcopystr(other.text);
  }
  return *this;
}

Mstring & Mstring::operator=(const char * s) {
  Free(text);
  text = mcopystr(s);
  return *this;
}

Mstring & Mstring::operator=(char c) {
  Free(text);
  text = memptystr();
  text = mputc(text, c);
  return *this;
}

Mstring & Mstring::operator+=(const Mstring & other) {
  text = mputstr(text, other.text);
  return *this;
}

Mstring & Mstring::operator+=(const char * s) {
  text = mputstr(text, s);
  return *this;
}

Mstring & Mstring::operator+=(char c) {
  text = mputc(text, c);
  return *this;
}

const Mstring operator+(const Mstring & lhs, const Mstring & rhs) {
  return Mstring(lhs) += rhs;
}

const Mstring operator+(const char * lhs, const Mstring & rhs) {
  return Mstring(lhs) += rhs;
}

const Mstring operator+(char lhs, const Mstring & rhs) {
  return Mstring(lhs) += rhs;
}

const Mstring operator+(const Mstring & lhs, const char * rhs) {
  return Mstring(lhs) += rhs;
}

const Mstring operator+(const Mstring & lhs, char rhs) {
  return Mstring(lhs) += rhs;
}

bool operator==(const Mstring & lhs, const Mstring & rhs) {
  if (strcmp(lhs.text, rhs.text) == 0) // they are equal
    return true;
  else
    return false;
}

bool operator==(const char * lhs, const Mstring & rhs) {
  if (strcmp(lhs, rhs.text) == 0) // they are equal
    return true;
  else
    return false;
}

bool operator==(const Mstring & lhs, const char * rhs) {
  if (strcmp(lhs.text, rhs) == 0) // they are equal
    return true;
  else
    return false;
}

bool operator!=(const Mstring & lhs, const Mstring & rhs) {
  if (strcmp(lhs.text, rhs.text) != 0) // they are NOT equal
    return true;
  else
    return false;
}

bool operator!=(const char * lhs, const Mstring & rhs) {
  if (strcmp(lhs, rhs.text) != 0) // they are NOT equal
    return true;
  else
    return false;
}

bool operator!=(const Mstring & lhs, const char * rhs) {
  if (strcmp(lhs.text, rhs) != 0) // they are NOT equal
    return true;
  else
    return false;
}

bool operator<(const Mstring & lhs, const Mstring & rhs) {
  if (strcmp(lhs.text, rhs.text) < 0)
    return true;
  else
    return false;
}

bool operator<(const char * lhs, const Mstring & rhs) {
  if (strcmp(lhs, rhs.text) < 0)
    return true;
  else
    return false;
}

bool operator<(const Mstring & lhs, const char * rhs) {
  if (strcmp(lhs.text, rhs) < 0)
    return true;
  else
    return false;
}

bool operator>(const Mstring & lhs, const Mstring & rhs) {
  if (strcmp(lhs.text, rhs.text) > 0)
    return true;
  else
    return false;
}

bool operator>(const char * lhs, const Mstring & rhs) {
  if (strcmp(lhs, rhs.text) > 0)
    return true;
  else
    return false;
}

bool operator>(const Mstring & lhs, const char * rhs) {
  if (strcmp(lhs.text, rhs) > 0)
    return true;
  else
    return false;
}

bool operator<=(const Mstring & lhs, const Mstring & rhs) {
  if (strcmp(lhs.text, rhs.text) <= 0)
    return true;
  else
    return false;
}

bool operator<=(const char * lhs, const Mstring & rhs) {
  if (strcmp(lhs, rhs.text) <= 0)
    return true;
  else
    return false;
}

bool operator<=(const Mstring & lhs, const char * rhs) {
  if (strcmp(lhs.text, rhs) <= 0)
    return true;
  else
    return false;
}

bool operator>=(const Mstring & lhs, const Mstring & rhs) {
  if (strcmp(lhs.text, rhs.text) >= 0)
    return true;
  else
    return false;
}

bool operator>=(const char * lhs, const Mstring & rhs) {
  if (strcmp(lhs, rhs.text) >= 0)
    return true;
  else
    return false;
}

bool operator>=(const Mstring & lhs, const char * rhs) {
  if (strcmp(lhs.text, rhs) >= 0)
    return true;
  else
    return false;
}
