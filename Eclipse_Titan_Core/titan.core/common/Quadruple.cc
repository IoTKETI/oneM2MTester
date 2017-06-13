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
 *   Raduly, Csaba
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "memory.h"
#include "pattern.hh"
#include "Quadruple.hh"

Quad::Quad() : u() {
  u.value = 0;
}

Quad::Quad(unsigned int value) : u() {
  u.value = value;
}

Quad::Quad(unsigned char group, unsigned char plane, unsigned char row,
  unsigned char cell) : u() {
  u.comp.group = group;
  u.comp.plane = plane;
  u.comp.row = row;
  u.comp.cell = cell;
}

Quad::Quad(const Quad& rhs) : u() {
  u.value = rhs.u.value;
}

unsigned int Quad::get_value() const {
  return u.value;
}

void Quad::set(int field, unsigned char c) {
  switch (field) {
  case 0:
    u.comp.group = c;
    break;
  case 1:
    u.comp.plane = c;
    break;
  case 2:
    u.comp.row = c;
    break;
  case 3:
    u.comp.cell = c;
    break;
  }
}

void Quad::set(unsigned char group, unsigned char plane, unsigned char row,
  unsigned char cell) {
  u.comp.group = group;
  u.comp.plane = plane;
  u.comp.row = row;
  u.comp.cell = cell;
}

void Quad::set_hexrepr(const char* hex_repr) {
  u.comp.group = static_cast<unsigned char>(((hex_repr[0] - 'A') << 4) + (hex_repr[1] - 'A'));
  u.comp.plane = static_cast<unsigned char>(((hex_repr[2] - 'A') << 4) + (hex_repr[3] - 'A'));
  u.comp.row =   static_cast<unsigned char>(((hex_repr[4] - 'A') << 4) + (hex_repr[5] - 'A'));
  u.comp.cell =  static_cast<unsigned char>(((hex_repr[6] - 'A') << 4) + (hex_repr[7] - 'A'));
}

const Quad Quad::operator-(const Quad& rhs) const {
  return Quad(u.value - rhs.u.value);
}

const Quad& Quad::operator=(const Quad& rhs) {
  u.value = rhs.u.value;
  return *this;
}

bool Quad::operator==(unsigned int i) const {
  return u.value == i;
}

bool Quad::operator==(const Quad& rhs) const {
  return u.value == rhs.u.value;
}

bool Quad::operator<=(const Quad& rhs) const {
  return u.value <= rhs.u.value;
}

bool Quad::operator>=(const Quad& rhs) const {
  return u.value >= rhs.u.value;
}

bool Quad::operator<(const Quad& rhs) const {
  return u.value < rhs.u.value;
}

bool Quad::operator<(const QuadInterval& rhs) const {
  return u.value < rhs.lower.u.value;
}

unsigned char Quad::operator[](int i) const {
  switch(i) {
  case 0:
    return u.comp.group;
  case 1:
    return u.comp.plane;
  case 2:
    return u.comp.row;
  case 3:
    return u.comp.cell;
  default:
    TTCN_pattern_error("Accessing a nonexistent field of a quadruple: %d.", i);
  }
  return 0; // to get rid of the warning
}

char* Quad::get_hexrepr() const {
  return Quad::get_hexrepr(u.value);
}

char* Quad::get_hexrepr(unsigned int value) {
  char hex[9];
  hex[8] = '\0';
  get_hexrepr(value, hex);
  return mcopystr(hex);
}

void Quad::get_hexrepr(const Quad& q, char* const str) {
  str[0] = static_cast<char>('A' + (q.u.comp.group >> 4)); // high end
  str[1] = static_cast<char>('A' + (q.u.comp.group & 15));
  str[2] = static_cast<char>('A' + (q.u.comp.plane >> 4));
  str[3] = static_cast<char>('A' + (q.u.comp.plane & 15));
  str[4] = static_cast<char>('A' + (q.u.comp.row   >> 4));
  str[5] = static_cast<char>('A' + (q.u.comp.row   & 15));
  str[6] = static_cast<char>('A' + (q.u.comp.cell  >> 4));
  str[7] = static_cast<char>('A' + (q.u.comp.cell  & 15)); // low end
}

char* Quad::char_hexrepr(unsigned char c) {
  char hex[3];
  hex[2] = '\0';
  hex[1] = static_cast<char>((c & 15) + 'A');
  hex[0] = static_cast<char>((c >> 4) + 'A');
  return mcopystr(hex);
}

// QuadInterval ----------------------------------------------------------------

QuadInterval::QuadInterval(Quad p_lower, Quad p_upper) :
  lower(p_lower), upper(p_upper)
{
}

QuadInterval::QuadInterval(const QuadInterval& rhs)
: lower(rhs.lower), upper(rhs.upper) {
}

const Quad& QuadInterval::get_lower() const {
  return lower;
}

const Quad& QuadInterval::get_upper() const {
  return upper;
}

bool QuadInterval::contains(const Quad& rhs) const {
  return lower <= rhs && upper >= rhs;
}

bool QuadInterval::contains(const QuadInterval& rhs) const {
  return lower <= rhs.lower && upper >= rhs.upper;
}

bool QuadInterval::has_intersection(const QuadInterval& rhs) const {
  return (rhs.lower <= upper && rhs.lower >= lower) ||
  (rhs.upper <= upper && rhs.upper >= lower);
}

void QuadInterval::join(const QuadInterval& rhs) {
  if (rhs.lower <= lower)
    lower = rhs.lower;
  if (rhs.upper >= upper)
    upper = rhs.upper;
}

bool QuadInterval::operator <(const Quad& rhs) const {
  return upper < rhs;
}

bool QuadInterval::operator <(const QuadInterval& rhs) const {
  return !has_intersection(rhs) && upper < rhs.lower;
}

unsigned int QuadInterval::width() const {
  return (upper - lower).get_value();
}

char* QuadInterval::generate_posix() {
  expstring_t res = memptystr();
  expstring_t str = 0;
  int diff[4];
  int i, j, c, k;
  for (i = 0; i < 4; i++)
    diff[i] = upper[i] - lower[i];
  Quad q1, q2;
  for (c = 0; c < 4 && diff[c] == 0; c++) ;
  while (c < 4) {
    if (c < 3)
      for (i = 0; i <= diff[c]; i++) {
        if (i > 0)
          res = mputc(res, '|');
        if (diff[c] > 0) {
          if (i == 0) {
            res = mputc(res, '(');
            q1 = q2 = lower;
            bool pipe = true;
            for (j = 3; j > c; j--) {
              if (j == 3 || (j < 3 && q1[j] < 255)) {
                if (j < 3 && pipe)
                  res = mputc(res, '|');
                for (k = 0; k < j; k++) {
                  res = mputprintf(res, "%s", str = Quad::char_hexrepr(q1[k]));
                  Free(str);
                }
                q2.set(j, 255);
                res = mputprintf(res, "%s",
                  str = generate_hex_interval(q1[j], q2[j]));
                Free(str);
                q1.set(j, 0);
                if (j > 0 && q1[j-1] < 255)
                  q1.set(j - 1, static_cast<unsigned char>(q1[j-1] + 1));
                for (k = j + 1; k < 4; k++) {
                  res = mputprintf(res, "%s",
                    str = generate_hex_interval(0, 255));
                  Free(str);
                }
                pipe = true;
              } else
                pipe = false;
            }
            res = mputc(res, ')');
          } else if (i < diff[c]) {
            for (j = 0; j < c; j++) {
              res = mputstr(res, str = Quad::char_hexrepr(lower[j]));
              Free(str);
            }
            str = generate_hex_interval(static_cast<unsigned char>(lower[c] + 1),
              static_cast<unsigned char>(lower[c] + diff[c] - 1));
            res = mputprintf(res, "%s", str);
            Free(str);
            k = (3 - c) * 2;
            if (k < 6) {
              for (j = 0; j < k; j++)
                res = mputc(res, '.');
            } else
              res = mputprintf(res, ".\\{%d\\}", k);
            i = diff[c] - 1;
          } else {
            res = mputc(res, '(');
            for (k = c; k < 4 && c < 3; k++) {
              q1 = 0;
              q2 = upper;
              for (j = 0; j <= c; j++) {
                q1.set(j, upper[j]);
                res = mputstr(res, str = Quad::char_hexrepr(q1[j]));
                Free(str);
              }
              c++;
              if (c < 3)
                q2.set(c, static_cast<unsigned char>(upper[c] - 1));
              res = mputstr(res, str = generate_hex_interval(q1[c], q2[c]));
              Free(str);
              for (j = c + 1; j < 4; j++) {
                q2.set(j, 255);
                res = mputstr(res, str = generate_hex_interval(q1[j], q2[j]));
                Free(str);
              }
              if (k < 3 && c < 3) {
                res = mputc(res, '|');
              }
            }
            res = mputc(res, ')');
            return res;
          }
        } else if (diff[c] == 0) {
          c++;
          i = diff[c];
        } else {
          TTCN_pattern_error("In set interval: end is lower than start.");
          Free(res);
          return 0;
        }
      }
    else {
      for (j = 0; j < 3; j++) {
        res = mputstr(res, str = Quad::char_hexrepr(lower[j]));
        Free(str);
      }
      res = mputstr(res, str = generate_hex_interval(lower[3], upper[3]));
      Free(str);
      return res;
    }
  }
  return res;
}

char* QuadInterval::generate_hex_interval(unsigned char source,
  unsigned char dest) {
  expstring_t res = memptystr();
  int s_lo, s_hi, d_lo, d_hi, lo, hi;
  s_lo = (source & 15) + 'A';
  s_hi = (source >> 4) + 'A';
  d_lo = (dest & 15) + 'A';
  d_hi = (dest >> 4) + 'A';
  lo = d_lo - s_lo;
  hi = d_hi - s_hi;
  if (hi > 0)
    res = mputc(res, '(');

  if (hi == 0) {
    if (lo < 0) { // This is possibly reported during parsing.
      TTCN_pattern_error("Illegal interval in set: start > end.");
    } else if (lo > 0) {
      res = mputc(res, static_cast<char>(s_hi));
      if (s_lo == 'A' && d_lo == 'P')
        res = mputc(res, '.');
      else
        res = mputprintf(res, "[%c-%c]", s_lo, d_lo);
    } else {
      res = mputc(res, static_cast<char>(s_hi));
      res = mputc(res, static_cast<char>(s_lo));
    }
    return res;
  }

  if (hi > 0) {
    bool alt = false;
    if (s_lo != 'A') {
      res = mputprintf(res, "%c[%c-P]", s_hi, s_lo);
      s_hi++;
      alt = true;
    }
    if (d_lo != 'P') {
      if (alt)
        res = mputc(res, '|');
      else
        alt = true;
      res = mputprintf(res, "%c[A-%c]", d_hi, d_lo);
      d_hi--;
    }
    if (d_hi > s_hi) {
      if (alt)
        res = mputc(res, '|');
      if (s_hi == 'A' && d_hi == 'P')
        res = mputc(res, '.');
      else
        res = mputprintf(res, "[%c-%c]", s_hi, d_hi);
      res = mputc(res, '.');
    }
  }

  if (hi > 0)
    res = mputc(res, ')');
  return res;
}

// -------- QuadSet ------------------------------------------------------------

QuadSet::QuadSet() :
  set(0), negate(false)
{
}

bool QuadSet::add(Quad* p_quad) {
  bool contains = false;
  quadset_node_t* it = set;
  quadset_node_t* after = 0, *it_old = 0;
  while (it) {
    switch (it->etype) {
    case QSET_QUAD:
      contains = *(it->u.p_quad) == *p_quad;
      if (!contains && *p_quad < *(it->u.p_quad))
        after = it_old;
      break;
    case QSET_INTERVAL:
      contains = it->u.p_interval->contains(*p_quad);
      if (!contains && *p_quad < *(it->u.p_quad))
        after = it_old;
      break;
    }
    it_old = it;
    it = it->next;
  }
  if (!contains) {
    quadset_node_t* newnode = new quadset_node_t;
    newnode->etype = QSET_QUAD;
    newnode->u.p_quad = p_quad;
    if (after == 0) { // largest element in the set so far
      newnode->next = 0;
      if (it_old != 0)
        it_old->next = newnode;
      else
        set = newnode;
    } else {
      newnode->next = after->next;
      after->next = newnode;
    }
    return true;
  } else {
    delete p_quad;
    return false;
  }
}

void QuadSet::add(QuadInterval* interval) {
  bool contains = false;
  quadset_node_t* it = set;
  quadset_node_t* after = 0, *it_old = 0;
  while (it) {
    switch (it->etype) {
    case QSET_QUAD:
      if (interval->contains(*(it->u.p_quad))) {  // delete the quad
        delete it->u.p_quad;
        if (it == set)
          set = it->next;
        quadset_node_t* p = it;
        it = it->next;
        delete p;
        continue;
      } else if (*interval < *(it->u.p_quad)) {
        after = it_old;
      }
      break;
    case QSET_INTERVAL:
      contains = it->u.p_interval->contains(*interval);
      if (!contains) {
        if (it->u.p_interval->has_intersection(*interval)) {  // merge
          it->u.p_interval->join(*interval);
          delete interval;
          join_if_possible(it);
          return;
        } else if (*interval < *(it->u.p_interval))
          after = it_old;
      }
      break;
    }
    it_old = it;
    it = it->next;
  }
  if (!contains) {
    quadset_node_t* newnode = new quadset_node_t;
    newnode->etype = QSET_INTERVAL;
    newnode->u.p_interval = interval;
    if (after == 0) { // largest element in the set so far
      newnode->next = 0;
      if (it_old != 0)
        it_old->next = newnode;
      else
        set = newnode;
    } else {
      newnode->next = after->next;
      after->next = newnode;
    }
  } else {
    delete interval;
  }
}

void QuadSet::join(QuadSet* rhs) {
  quadset_node_t* it = rhs->set;
  while (it) {
    switch (it->etype) {
    case QSET_QUAD:
      add(new Quad(*(it->u.p_quad)));
      break;
    case QSET_INTERVAL:
      add(new QuadInterval(*(it->u.p_interval)));
      break;
    }
    it = it->next;
  }
}

void QuadSet::set_negate(bool neg) {
  negate = neg;
}

bool QuadSet::has_quad(const Quad& q) const {
  quadset_node_t* it = set;
  while (it) {
    switch(it->etype) {
    case QSET_QUAD:
      if (q == *(it->u.p_quad))
        return true;
      break;
    case QSET_INTERVAL:
      if (it->u.p_interval->contains(q))
        return true;
    }
    it = it->next;
  }
  return false;
}

bool QuadSet::is_empty() const {
  return 0 == set;
}

char* QuadSet::generate_posix() {
  expstring_t str = 0;
  if (negate)
    do_negate();
  char* res = memptystr();
  res = mputc(res, '(');
  quadset_node_t* it = set;
  while (it) {
    if (it != set)
      res = mputc(res, '|');
    switch (it->etype) {
    case QSET_QUAD:
      str = it->u.p_quad->get_hexrepr();
      res = mputprintf(res, "%s", str);
      Free(str);
      break;
    case QSET_INTERVAL:
      str = it->u.p_interval->generate_posix();
      res = mputprintf(res, "%s", str);
      Free(str);
      break;
    }
    it = it->next;
  }
  res = mputc(res, ')');
  return res;
}

QuadSet::~QuadSet() {
  clean(set);
}

void QuadSet::clean(quadset_node_t* start) {
  quadset_node_t* it = start;
  while (it) {
    switch (it->etype) {
    case QSET_QUAD:
      delete it->u.p_quad;
      break;
    case QSET_INTERVAL:
      delete it->u.p_interval;
      break;
    }
    quadset_node_t* p = it;
    it = it->next;
    delete p;
  }
}

void QuadSet::do_negate() {
  QuadSet* qs = new QuadSet();
  quadset_node_t* it = set;
  Quad q1, q2;
  while (it) {
    switch (it->etype) {
    case QSET_QUAD:
      q2 = it->u.p_quad->get_value() - 1;
      qs->add_negate_interval(q1, q2);
      q1 = q2.get_value() + 1;
      break;
    case QSET_INTERVAL:
      q2 = it->u.p_interval->get_lower().get_value() - 1;
      qs->add_negate_interval(q1, q2);
      q1 = it->u.p_interval->get_upper().get_value() + 1;
      break;
    }
    it = it->next;
  }
  if (!(q1 == 0)) {
    q2.set(255, 255, 255, 255);
    qs->add_negate_interval(q1, q2);
  }/* else
    delete q1;*/
  clean(set);
  set = qs->set;
  qs->set = 0;
  delete qs;
}

void QuadSet::add_negate_interval(const Quad& q1, const Quad& q2) {
  if (q2 >= q1) {
    unsigned int w = q2.get_value() - q1.get_value();
    if (w > 0)
      add(new QuadInterval(q1, q2));
    else {
      add(new Quad(q2));
    }
  }
}

void QuadSet::join_if_possible(quadset_node_t* start) {
  quadset_node_t* it = start->next;
  while (it) {
    switch (it->etype) {
    case QSET_QUAD:
      if (start->u.p_interval->contains(*(it->u.p_quad))) {
        delete it->u.p_quad;
        quadset_node_t* p = it;
        it = it->next;
        delete p;
        continue;
      }
      break;
    case QSET_INTERVAL:
      if (start->u.p_interval->has_intersection(*(it->u.p_interval))) {
        start->u.p_interval->join(*(it->u.p_interval));
        delete it->u.p_interval;
        quadset_node_t* p = it;
        it = it->next;
        delete p;
        continue;
      }
      break;
    }
    it = it->next;
  }
}
