/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Delic, Adam
 *   Raduly, Csaba
 *   Szabados, Kristof
 *
 ******************************************************************************/
#ifndef _Subtypestuff_HH
#define _Subtypestuff_HH

#include "ttcn3/compiler.h"
#include "vector.hh"
#include "map.hh"
#include "Int.hh"
#include "Real.hh"
#include "ustring.hh"
#include "Setting.hh"
#include "../common/ttcn3float.hh"

namespace Ttcn {
    class LengthRestriction;
    class Template;
    class ValueRange;
    class PatternString;
}

namespace Common {

class Identifier;
class Value;
class Type;


enum tribool // http://en.wikipedia.org/wiki/Ternary_logic
{
  TFALSE   = 0, // values are indexes into truth tables
  TUNKNOWN = 1,
  TTRUE    = 2
};

extern tribool operator||(tribool a, tribool b);
extern tribool operator&&(tribool a, tribool b);
extern tribool operator!(tribool tb);
extern tribool TRIBOOL(bool b);
extern string to_string(const tribool& tb);

////////////////////////////////////////////////////////////////////////////////

// integer interval limit type, can be +/- infinity, in case of infinity value has no meaning
class int_limit_t
{
public:
  enum int_limit_type_t {
    MINUS_INFINITY,
    NUMBER,
    PLUS_INFINITY
  };
  static const int_limit_t minimum;
  static const int_limit_t maximum;
private:
  int_limit_type_t type;
  int_val_t value;
public:
  int_limit_t(): type(NUMBER), value() {}
  int_limit_t(int_limit_type_t p_type);
  int_limit_t(const int_val_t& p_value): type(NUMBER), value(p_value) {}
  bool operator<(const int_limit_t& right) const;
  bool operator==(const int_limit_t& right) const;
  bool is_adjacent(const int_limit_t& other) const;
  int_val_t get_value() const;
  int_limit_type_t get_type() const { return type; }
  int_limit_t next() const; // upper neighbour
  int_limit_t previous() const; // lower neighbour
  void check_single_value() const;
  void check_interval_start() const;
  void check_interval_end() const;
  string to_string() const;
};

////////////////////////////////////////////////////////////////////////////////

// limit value for length/size restriction
class size_limit_t
{
public:
  enum size_limit_type_t {
    INFINITE_SIZE
  };
private:
  bool infinity;
  size_t size;
public:
  static const size_limit_t minimum;
  static const size_limit_t maximum;
  size_limit_t() : infinity(false), size() {}
  size_limit_t(size_limit_type_t): infinity(true), size() {}
  size_limit_t(size_t p_size): infinity(false), size(p_size) {}
  bool operator<(const size_limit_t& right) const;
  bool operator==(const size_limit_t& right) const;
  bool is_adjacent(const size_limit_t& other) const;
  size_t get_size() const;
  bool get_infinity() const { return infinity; }
  size_limit_t next() const;
  size_limit_t previous() const;
  void check_single_value() const;
  void check_interval_start() const;
  void check_interval_end() const {}
  string to_string() const;
  int_limit_t to_int_limit() const;
};

////////////////////////////////////////////////////////////////////////////////

// limit value for string range/alphabet restriction
class char_limit_t
{
private:
  short int chr;
  static const short int max_char;
public:
  static bool is_valid_value(short int p_chr);
  static const char_limit_t minimum;
  static const char_limit_t maximum;
  char_limit_t(): chr(0) {}
  char_limit_t(short int p_chr);
  bool operator<(const char_limit_t& right) const { return chr<right.chr; }
  bool operator==(const char_limit_t& right) const { return chr==right.chr; }
  bool is_adjacent(const char_limit_t& other) const { return (chr+1==other.chr); }
  char get_char() const { return static_cast<char>(chr); }
  char_limit_t next() const;
  char_limit_t previous() const;
  void check_single_value() const {}
  void check_interval_start() const {}
  void check_interval_end() const {}
  string to_string() const;
};

////////////////////////////////////////////////////////////////////////////////

class universal_char_limit_t
{
private:
  unsigned int code_point; // UCS-4 values [0..0x7FFFFFFF], for easy calculations
  static const unsigned int max_code_point;
  void check_value() const;
public:
  static bool is_valid_value(const ustring::universal_char& p_uchr);
  static unsigned int uchar2codepoint(const ustring::universal_char& uchr);
  static ustring::universal_char codepoint2uchar(unsigned int cp);
  static const universal_char_limit_t minimum;
  static const universal_char_limit_t maximum;
  universal_char_limit_t(): code_point(0) {}
  universal_char_limit_t(unsigned int p_code_point): code_point(p_code_point) { check_value(); }
  universal_char_limit_t(const ustring::universal_char& p_chr) : code_point(uchar2codepoint(p_chr)) { check_value(); }
  bool operator<(const universal_char_limit_t& right) const { return code_point<right.code_point; }
  bool operator==(const universal_char_limit_t& right) const { return code_point==right.code_point; }
  bool is_adjacent(const universal_char_limit_t& other) const { return (code_point+1==other.code_point); }
  ustring::universal_char get_universal_char() const { return codepoint2uchar(code_point); }
  unsigned int get_code_point() const { return code_point; }
  universal_char_limit_t next() const;
  universal_char_limit_t previous() const;
  void check_single_value() const {}
  void check_interval_start() const {}
  void check_interval_end() const {}
  string to_string() const;
};

////////////////////////////////////////////////////////////////////////////////

class real_limit_t
{
public:
  enum real_limit_type_t {
    LOWER, // the highest value that is less than the value, for open interval's upper limit
    EXACT, // the value itself, for closed interval limits and single values
    UPPER  // the lowest value that is more than the value, for open interval's lower limit
  };
  static const real_limit_t minimum;
  static const real_limit_t maximum;
private:
  real_limit_type_t type;
  ttcn3float value;
  void check_value() const; // check for illegal values: NaN, -inf.lower and inf.upper
public:
  real_limit_t(): type(EXACT), value() { value = 0.0; } // avoid random illegal values
  real_limit_t(const ttcn3float& p_value): type(EXACT), value(p_value) { check_value(); }
  real_limit_t(const ttcn3float& p_value, real_limit_type_t p_type): type(p_type), value(p_value) { check_value(); }

  bool operator<(const real_limit_t& right) const;
  bool operator==(const real_limit_t& right) const;
  bool is_adjacent(const real_limit_t& other) const; // two different values cannot be adjacent in a general floating point value
  ttcn3float get_value() const { return value; }
  real_limit_type_t get_type() const { return type; }
  real_limit_t next() const; // upper neighbour, has same value
  real_limit_t previous() const; // lower neighbour, has same value
  void check_single_value() const;
  void check_interval_start() const;
  void check_interval_end() const;
  string to_string() const;
};

////////////////////////////////////////////////////////////////////////////////

// forward declaration
template <typename LIMITTYPE>
class RangeListConstraint;

bool convert_int_to_size(const RangeListConstraint<int_limit_t>& int_range, RangeListConstraint<size_limit_t>& size_range);

/*
all-in-one constraint class template for xxx_limit_t types
the xxx_limit_t classes must have the same interface for use by this class
canonical form:
- values must be v1 < v2 < v3 < ... < vN (xxx_limit_t::operator<() and xxx_limit_t::operator==() used)
- there cannot be two adjacent intervals that are part of the set (determined by xxx_limit_t::is_adjacent())
- two adjacent values must make an interval (if values[i] is adjacent to values[i+1] then intervals[i] is true)
- empty values[] means empty set
- full set is [minimum-maximum] interval (xxx_limit_t::minimum and xxx_limit_t::maximum are used)
*/
template <typename LIMITTYPE>
class RangeListConstraint
{
private:
  // used in calculating the union and intersection of two sets, _idx are indexes into the values[] vector of the operand sets
  struct sweep_point_t
  {
    int a_idx; // index into the operand's values/intervals vectors or -1
    int b_idx;
    bool union_interval; // is this interval in the set
    bool intersection_interval; // is this interval in the set
    bool intersection_point; // is this point in the set
    sweep_point_t(): a_idx(-1), b_idx(-1), union_interval(false), intersection_interval(false), intersection_point(false) {}
    sweep_point_t(int a, int b): a_idx(a), b_idx(b), union_interval(false), intersection_interval(false), intersection_point(false) {}
  };

  dynamic_array<LIMITTYPE> values;
  dynamic_array<bool> intervals;

public:
  // constructors
  RangeListConstraint(): values(), intervals() {} // empty set
  RangeListConstraint(const LIMITTYPE& l); // single value set
  RangeListConstraint(const LIMITTYPE& l_begin, const LIMITTYPE& l_end); // value range set

  tribool is_empty() const;
  tribool is_full() const;
  tribool is_equal(const RangeListConstraint& other) const;
  bool is_element(const LIMITTYPE& l) const;

  RangeListConstraint set_operation(const RangeListConstraint& other, bool is_union) const; // A union/intersection B -> C
  RangeListConstraint operator+(const RangeListConstraint& other) const { return set_operation(other, true); } // union
  RangeListConstraint operator*(const RangeListConstraint& other) const { return set_operation(other, false); } // intersection
  RangeListConstraint operator~() const; // complement

  tribool is_subset(const RangeListConstraint& other) const { return (*this*~other).is_empty(); }
  RangeListConstraint operator-(const RangeListConstraint& other) const { return ( *this * ~other ); } // except

  // will return the minimal value that is part of the interval,
  // if the interval is empty the returned value is undefined
  LIMITTYPE get_minimal() const;
  LIMITTYPE get_maximal() const;

  bool is_upper_limit_infinity() const;
  bool is_lower_limit_infinity() const;

  string to_string(bool add_brackets=true) const;

  /** conversion from integer range to size range,
      returns true on success, false if the integers do not fit into the size values,
      when returning false the size_range will be set to the full set */
  friend bool convert_int_to_size(const RangeListConstraint<int_limit_t>& int_range, RangeListConstraint<size_limit_t>& size_range);
};

template <typename LIMITTYPE>
RangeListConstraint<LIMITTYPE>::RangeListConstraint(const LIMITTYPE& l)
: values(), intervals()
{
  l.check_single_value();
  values.add(l);
  intervals.add(false);
}

template <typename LIMITTYPE>
RangeListConstraint<LIMITTYPE>::RangeListConstraint(const LIMITTYPE& l_begin, const LIMITTYPE& l_end)
: values(), intervals()
{
  if (l_end<l_begin) FATAL_ERROR("RangeListConstraint::RangeListConstraint(): invalid range");
  if (l_begin==l_end) {
    l_begin.check_single_value();
    values.add(l_begin);
    intervals.add(false);
  } else {
    l_begin.check_interval_start();
    l_end.check_interval_end();
    values.add(l_begin);
    intervals.add(true);
    values.add(l_end);
    intervals.add(false);
  }
}

template <typename LIMITTYPE>
tribool RangeListConstraint<LIMITTYPE>::is_empty() const
{
  return TRIBOOL(values.size()==0);
}

template <typename LIMITTYPE>
tribool RangeListConstraint<LIMITTYPE>::is_full() const
{
  return TRIBOOL( (values.size()==2) && (values[0]==LIMITTYPE::minimum) && (values[1]==LIMITTYPE::maximum) && (intervals[0]) );
}

template <typename LIMITTYPE>
tribool RangeListConstraint<LIMITTYPE>::is_equal(const RangeListConstraint<LIMITTYPE>& other) const
{
  return TRIBOOL( (values==other.values) && (intervals==other.intervals) );
}

template <typename LIMITTYPE>
bool RangeListConstraint<LIMITTYPE>::is_element(const LIMITTYPE& l) const
{
  if (values.size()==0) return false;
  // binary search in values[]
  size_t lower_idx = 0;
  size_t upper_idx = values.size()-1;
  while (upper_idx>lower_idx+1) {
    size_t middle_idx = lower_idx + (upper_idx-lower_idx) / 2;
    if (values[middle_idx]<l) lower_idx = middle_idx;
    else upper_idx = middle_idx;
  }
  if (lower_idx==upper_idx) {
    if (values[lower_idx]==l) return true; // every value is in the set
    else if (values[lower_idx]<l) return intervals[lower_idx];
    else return ( (lower_idx>0) ? intervals[lower_idx-1] : false );
  } else {
    if (l<values[lower_idx]) return ( (lower_idx>0) ? intervals[lower_idx-1] : false );
    else if (l==values[lower_idx]) return true;
    else if (l<values[upper_idx]) return intervals[upper_idx-1];
    else if (l==values[upper_idx]) return true;
    else return intervals[upper_idx];
  }
}

template <typename LIMITTYPE>
RangeListConstraint<LIMITTYPE> RangeListConstraint<LIMITTYPE>::operator~() const
{
  if (values.size()==0) { // if we have an empty set
    return RangeListConstraint<LIMITTYPE>(LIMITTYPE::minimum, LIMITTYPE::maximum);
  }
  RangeListConstraint<LIMITTYPE> ret_val;
  // invert intervals
  if (!(values[0]==LIMITTYPE::minimum)) {
    if (LIMITTYPE::minimum.is_adjacent(values[0])) {
      ret_val.values.add(LIMITTYPE::minimum);
      ret_val.intervals.add(false);
    } else {
      ret_val.values.add(LIMITTYPE::minimum);
      ret_val.intervals.add(true);
      ret_val.values.add(values[0].previous());
      ret_val.intervals.add(false);
    }
  }
  size_t last = values.size()-1;
  for (size_t i=0; i<last; i++)
  {
    if (!intervals[i]) {
      if (values[i].next()==values[i+1].previous()) {
        // add one value between intervals
        ret_val.values.add(values[i].next());
        ret_val.intervals.add(false);
      } else {
        // add interval between intervals
        ret_val.values.add(values[i].next());
        ret_val.intervals.add(true);
        ret_val.values.add(values[i+1].previous());
        ret_val.intervals.add(false);
      }
    }
  }
  if (!(values[last]==LIMITTYPE::maximum)) {
    if (values[last].is_adjacent(LIMITTYPE::maximum)) {
      ret_val.values.add(LIMITTYPE::maximum);
      ret_val.intervals.add(false);
    } else {
      ret_val.values.add(values[last].next());
      ret_val.intervals.add(true);
      ret_val.values.add(LIMITTYPE::maximum);
      ret_val.intervals.add(false);
    }
  }
  return ret_val;
}

template <typename LIMITTYPE>
RangeListConstraint<LIMITTYPE> RangeListConstraint<LIMITTYPE>::set_operation(const RangeListConstraint<LIMITTYPE>& other, bool is_union) const
{
  // special case: one or both are empty sets
  if (values.size()==0) return is_union ? other : *this;
  if (other.values.size()==0) return is_union ? *this : other;

  // calculate the sweep points
  dynamic_array<sweep_point_t> sweep_points;
  sweep_point_t spi(0,0);
  while ( (spi.a_idx < static_cast<int>(values.size())) || (spi.b_idx < static_cast<int>(other.values.size())) )
  {
    if (spi.a_idx >= static_cast<int>(values.size())) {
      sweep_points.add(sweep_point_t(-1,spi.b_idx));
      spi.b_idx++;
    } else if (spi.b_idx >= static_cast<int>(other.values.size())) {
      sweep_points.add(sweep_point_t(spi.a_idx, -1));
      spi.a_idx++;
    } else { // both are within the vector, get smaller or get both if equal
      if (values[static_cast<size_t>(spi.a_idx)] < other.values[static_cast<size_t>(spi.b_idx)]) {
        sweep_points.add(sweep_point_t(spi.a_idx, -1));
        spi.a_idx++;
      } else if (values[static_cast<size_t>(spi.a_idx)] == other.values[static_cast<size_t>(spi.b_idx)]) {
        sweep_points.add(spi);
        spi.a_idx++;
        spi.b_idx++;
      } else {
        sweep_points.add(sweep_point_t(-1,spi.b_idx));
        spi.b_idx++;
      }
    }
  }

  // sweep (iterate) through both vectors
  bool in_a = false; // we are already in an interval of A
  bool in_b = false;
  for (size_t i=0; i<sweep_points.size(); i++)
  {
    // set bools for A interval
    bool a_interval = in_a;
    bool a_point = false;
    if (sweep_points[i].a_idx!=-1) { // we are at a value in A
      a_point = true;
      if (intervals[static_cast<size_t>(sweep_points[i].a_idx)]) { // this is a starting point of an interval in A
        a_interval = true;
        if (in_a) FATAL_ERROR("RangeListConstraint::set_operation(): invalid double interval");
        in_a = true;
      } else { // this point ends an interval of A
        a_interval = false;
        in_a = false;
      }
    }
    // set bools for B interval
    bool b_interval = in_b;
    bool b_point = false;
    if (sweep_points[i].b_idx!=-1) { // we are at a value in B
      b_point = true;
      if (other.intervals[static_cast<size_t>(sweep_points[i].b_idx)]) { // this is a starting point of an interval in B
        b_interval = true;
        if (in_b) FATAL_ERROR("RangeListConstraint::set_operation(): invalid double interval");
        in_b = true;
      } else { // this point ends an interval of B
        b_interval = false;
        in_b = false;
      }
    }
    // set the booleans of the union and intersection sets
    sweep_points[i].union_interval = a_interval || b_interval;
    sweep_points[i].intersection_point = (a_point || in_a) && (b_point || in_b);
    sweep_points[i].intersection_interval = a_interval && b_interval;
  }

  // canonicalization of ret_val
  if (is_union) {
    // connect adjacent limit points with interval: [i,i+1] becomes interval
    for (size_t i=1; i<sweep_points.size(); i++)
    {
      LIMITTYPE first, second;
      if (sweep_points[i-1].a_idx!=-1) {
        first = values[static_cast<size_t>(sweep_points[i-1].a_idx)];
      } else {
        if (sweep_points[i-1].b_idx!=-1) first = other.values[static_cast<size_t>(sweep_points[i-1].b_idx)];
        else FATAL_ERROR("RangeListConstraint::set_operation()");
      }
      if (sweep_points[i].a_idx!=-1) {
        second = values[static_cast<size_t>(sweep_points[i].a_idx)];
      } else {
        if (sweep_points[i].b_idx!=-1) second = other.values[static_cast<size_t>(sweep_points[i].b_idx)];
        else FATAL_ERROR("RangeListConstraint::set_operation()");
      }
      if (first.is_adjacent(second)) {
        sweep_points[i-1].union_interval = true;
        sweep_points[i-1].intersection_interval = sweep_points[i-1].intersection_point && sweep_points[i].intersection_point;
      }
    }
  }
  // two adjacent intervals shall be united into one
  RangeListConstraint<LIMITTYPE> ret_val;
  for (size_t i=0; i<sweep_points.size(); i++)
  {
    if (is_union) {//FIXME unnecessary to check in every loop
        if ( (i>0) && sweep_points[i-1].union_interval && sweep_points[i].union_interval) {
          // drop this point, it's in a double interval
        } else {
          LIMITTYPE l;
          if (sweep_points[i].a_idx!=-1) {
            l = values[static_cast<size_t>(sweep_points[i].a_idx)];
          } else {
            if (sweep_points[i].b_idx!=-1) l = other.values[static_cast<size_t>(sweep_points[i].b_idx)];
            else FATAL_ERROR("RangeListConstraint::set_operation()");
          }
          ret_val.values.add(l);
          ret_val.intervals.add(sweep_points[i].union_interval);
        }
    } else {
      if (sweep_points[i].intersection_point) {
        if ( (i>0) && sweep_points[i-1].intersection_interval && sweep_points[i].intersection_interval) {
          // drop this point, it's in a double interval
        } else {
          LIMITTYPE l;
          if (sweep_points[i].a_idx!=-1) {
            l = values[static_cast<size_t>(sweep_points[i].a_idx)];
          } else {
            if (sweep_points[i].b_idx!=-1) l = other.values[static_cast<size_t>(sweep_points[i].b_idx)];
            else FATAL_ERROR("RangeListConstraint::set_operation()");
          }
          ret_val.values.add(l);
          ret_val.intervals.add(sweep_points[i].intersection_interval);
        }
      }
    }
  }
  return ret_val;
}

template <typename LIMITTYPE>
LIMITTYPE RangeListConstraint<LIMITTYPE>::get_minimal() const
{
  if (values.size()<1) return LIMITTYPE();
  return values[0];
}

template <typename LIMITTYPE>
LIMITTYPE RangeListConstraint<LIMITTYPE>::get_maximal() const
{
  if (values.size()<1) return LIMITTYPE();
  return values[values.size()-1];
}

template <typename LIMITTYPE>
bool RangeListConstraint<LIMITTYPE>::is_upper_limit_infinity () const
{
  if (0 == values.size()) return false;
  return LIMITTYPE::maximum == values[values.size()-1];
}

template <typename LIMITTYPE>
bool RangeListConstraint<LIMITTYPE>::is_lower_limit_infinity () const
{
  if (0 == values.size()) return false;
  return LIMITTYPE::minimum == values[0];
}

template <typename LIMITTYPE>
string RangeListConstraint<LIMITTYPE>::to_string(bool add_brackets) const
{
  string ret_val;
  if (add_brackets) ret_val += '(';
  if(values.size() > 0) {
    size_t last = values.size()-1;
    for (size_t i=0; i<=last; i++)
    {
      ret_val += values[i].to_string();
      if (intervals[i]) ret_val += "..";
      else if (i<last) ret_val += ',';
    }
  }
  if (add_brackets) ret_val += ')';
  return ret_val;
}

////////////////////////////////////////////////////////////////////////////////

typedef RangeListConstraint<int_limit_t> IntegerRangeListConstraint; // for integer type
typedef RangeListConstraint<size_limit_t> SizeRangeListConstraint; // for length constraints
typedef RangeListConstraint<char_limit_t> CharRangeListConstraint; // for char/bit/hex/octet strings
typedef RangeListConstraint<universal_char_limit_t> UniversalCharRangeListConstraint; // for universal charstring

////////////////////////////////////////////////////////////////////////////////

// RangeListConstraint with added NaN value (NaN is unordered so it cannot be a limit value)
// this is canonical only if two different Real values are never considered to be adjacent
// which means that in theory for two different Real values there are always infinite number of Real values that are between them
class RealRangeListConstraint
{
private:
  bool has_nan;
  RangeListConstraint<real_limit_t> rlc;
public:
  // constructors
  RealRangeListConstraint(bool p_has_nan = false): has_nan(p_has_nan), rlc() {} // empty set or NaN
  RealRangeListConstraint(const real_limit_t& rl): has_nan(false), rlc(rl) {} // single value set (cannot be lower or upper, must be exact value)
  RealRangeListConstraint(const real_limit_t& rl_begin, const real_limit_t& rl_end): has_nan(false), rlc(rl_begin,rl_end) {} // range set

  tribool is_empty() const { return ( rlc.is_empty() && TRIBOOL(!has_nan) ); }
  tribool is_full() const { return ( rlc.is_full() && TRIBOOL(has_nan) ); }
  tribool is_equal(const RealRangeListConstraint& other) const { return ( rlc.is_equal(other.rlc) && TRIBOOL(has_nan==other.has_nan) ); }
  bool is_element(const ttcn3float& r) const;

  // A union/intersection B -> C
  RealRangeListConstraint set_operation(const RealRangeListConstraint& other, bool is_union) const;

  RealRangeListConstraint operator+(const RealRangeListConstraint& other) const { return set_operation(other, true); } // union
  RealRangeListConstraint operator*(const RealRangeListConstraint& other) const { return set_operation(other, false); } // intersection
  RealRangeListConstraint operator~() const; // complement

  tribool is_subset(const RealRangeListConstraint& other) const { return (*this*~other).is_empty(); }
  RealRangeListConstraint operator-(const RealRangeListConstraint& other) const { return ( *this * ~other ); } // except

  tribool is_range_empty() const { return rlc.is_empty(); }
  real_limit_t get_minimal() const { return rlc.get_minimal(); }
  real_limit_t get_maximal() const { return rlc.get_maximal(); }

  string to_string() const;
  bool is_upper_limit_infinity() const;
  bool is_lower_limit_infinity() const;
};

////////////////////////////////////////////////////////////////////////////////

class BooleanListConstraint
{
private:
  // every value selects a different bit, if the bit is set to 1 then the associated value is element of the set
  enum boolean_constraint_t {
    // 0x00 is empty set
    BC_FALSE = 0x01,
    BC_TRUE  = 0x02,
    BC_ALL   = 0x03 // all values, full set
  };
  unsigned char values;

public:
  // constructors
  BooleanListConstraint(): values(0) {} // empty set
  BooleanListConstraint(bool b): values(b ? BC_TRUE:BC_FALSE) {} // single value set

  tribool is_empty() const { return TRIBOOL(values==0); }
  tribool is_full() const { return TRIBOOL(values==BC_ALL); }
  tribool is_equal(const BooleanListConstraint& other) const { return TRIBOOL(values==other.values); }
  bool is_element(bool b) const { return b ? (values&BC_TRUE) : (values&BC_FALSE); }

  BooleanListConstraint operator+(const BooleanListConstraint& other) const { BooleanListConstraint rv; rv.values = values | other.values; return rv; }
  BooleanListConstraint operator*(const BooleanListConstraint& other) const { BooleanListConstraint rv; rv.values = values & other.values; return rv; }
  BooleanListConstraint operator~() const { BooleanListConstraint rv; rv.values = values ^ BC_ALL; return rv; }

  tribool is_subset(const BooleanListConstraint& other) const { return (*this*~other).is_empty(); }

  BooleanListConstraint operator-(const BooleanListConstraint& other) const { return ( *this * ~other ); }

  string to_string() const;
};

////////////////////////////////////////////////////////////////////////////////

class VerdicttypeListConstraint
{
public:
  enum verdicttype_constraint_t { // every value selects a different bit, if the bit is set to 1 then the associated value is element of the set
    // 0x00 is empty set
    VC_NONE   = 0x01,
    VC_PASS   = 0x02,
    VC_INCONC = 0x04,
    VC_FAIL   = 0x08,
    VC_ERROR  = 0x10,
    VC_ALL    = 0x1F // all values, full set
  };

private:
  unsigned char values;

public:
  // constructors
  VerdicttypeListConstraint(): values(0) {} // empty set
  VerdicttypeListConstraint(verdicttype_constraint_t vtc): values(vtc) {} // single value set

  tribool is_empty() const { return TRIBOOL(values==0); }
  tribool is_full() const { return TRIBOOL(values==VC_ALL); }
  tribool is_equal(const VerdicttypeListConstraint& other) const { return TRIBOOL(values==other.values); }
  bool is_element(verdicttype_constraint_t vtc) const { return vtc&values; }

  VerdicttypeListConstraint operator+(const VerdicttypeListConstraint& other) const { VerdicttypeListConstraint rv; rv.values = values | other.values; return rv; }
  VerdicttypeListConstraint operator*(const VerdicttypeListConstraint& other) const { VerdicttypeListConstraint rv; rv.values = values & other.values; return rv; }
  VerdicttypeListConstraint operator~() const { VerdicttypeListConstraint rv; rv.values = values ^ VC_ALL; return rv; }

  tribool is_subset(const VerdicttypeListConstraint& other) const { return (*this*~other).is_empty(); }

  VerdicttypeListConstraint operator-(const VerdicttypeListConstraint& other) const { return ( *this * ~other ); }

  string to_string() const;
};

////////////////////////////////////////////////////////////////////////////////

// size and value list constraint for bitstring, hexstring and octetstring
// in the compiler octetstring is a special hexstring where 1 octet = 2 hex.chars
// not_values is needed because the operation complement/except
// not_values must always be inside size_constraint set, has_values must be outside of size_constraint set
// canonical form can be obtained by simplifying value list constraints into size constraints
// and by converting not_values information into the other two sets if number of not values is >= [number of all values for L] / 2
// for length(L) there must be exactly N^L number of values that have length=L, where an element can have N different values
// where N = 2^BITCNT, BITCNT is the number of bits needed to store one element, works for BITCNT=1,4,8
// for octetstrings one octet element is 2 chars long, for others one element is one char, real size of string = elem.size()/ELEMSIZE
template<unsigned char BITCNT, unsigned short ELEMSIZE>
class StringSizeAndValueListConstraint
{
private:
  SizeRangeListConstraint size_constraint;
  map<string,void> has_values, not_values;
  void clean_up();
  void copy_content(const StringSizeAndValueListConstraint& other);
  void canonicalize(map<string,void>& values, map<string,void>& other_values, bool if_values);
  void canonicalize();
public:
  // constructors
  StringSizeAndValueListConstraint() {} // empty set
  StringSizeAndValueListConstraint(const string& s); // single value set
  StringSizeAndValueListConstraint(const size_limit_t& sl): size_constraint(sl) {} // single size value set
  StringSizeAndValueListConstraint(const size_limit_t& rl_begin, const size_limit_t& rl_end): size_constraint(rl_begin,rl_end) {} // size range set
  StringSizeAndValueListConstraint(const SizeRangeListConstraint& p_size_constraint): size_constraint(p_size_constraint) {}
  StringSizeAndValueListConstraint(const StringSizeAndValueListConstraint& other) { copy_content(other); }
  StringSizeAndValueListConstraint& operator=(const StringSizeAndValueListConstraint& other) { clean_up(); copy_content(other); return *this; }
  ~StringSizeAndValueListConstraint() { clean_up(); }

  tribool is_empty() const { return ( size_constraint.is_empty() && TRIBOOL(has_values.size()==0) ); }
  tribool is_full() const { return ( size_constraint.is_full() && TRIBOOL(not_values.size()==0) ); }
  tribool is_equal(const StringSizeAndValueListConstraint& other) const;
  bool is_element(const string& s) const;

  StringSizeAndValueListConstraint set_operation(const StringSizeAndValueListConstraint& other, bool is_union) const;

  StringSizeAndValueListConstraint operator+(const StringSizeAndValueListConstraint& other) const { return set_operation(other, true); } // union
  StringSizeAndValueListConstraint operator*(const StringSizeAndValueListConstraint& other) const { return set_operation(other, false); } // intersection
  StringSizeAndValueListConstraint operator~() const; // complement

  tribool is_subset(const StringSizeAndValueListConstraint& other) const { return (*this*~other).is_empty(); }
  StringSizeAndValueListConstraint operator-(const StringSizeAndValueListConstraint& other) const { return ( *this * ~other ); } // except

  tribool get_size_limit(bool is_upper, size_limit_t& limit) const;

  string to_string() const;
};

template<unsigned char BITCNT, unsigned short ELEMSIZE>
tribool StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::get_size_limit(bool is_upper, size_limit_t& limit) const
{
  if (size_constraint.is_empty()==TTRUE) return TFALSE;
  limit = is_upper ? size_constraint.get_maximal() : size_constraint.get_minimal();
  return TTRUE;
}

template<unsigned char BITCNT, unsigned short ELEMSIZE>
void StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::clean_up()
{
  size_constraint = SizeRangeListConstraint();
  has_values.clear();
  not_values.clear();
}

template<unsigned char BITCNT, unsigned short ELEMSIZE>
void StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::copy_content(const StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>& other)
{
  size_constraint = other.size_constraint;
  for (size_t i=0; i<other.has_values.size(); i++) has_values.add(other.has_values.get_nth_key(i),NULL);
  for (size_t i=0; i<other.not_values.size(); i++) not_values.add(other.not_values.get_nth_key(i),NULL);
}

template<unsigned char BITCNT, unsigned short ELEMSIZE>
StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::StringSizeAndValueListConstraint(const string& s)
{
  if (s.size() % ELEMSIZE != 0) FATAL_ERROR("StringSizeAndValueListConstraint::StringSizeAndValueListConstraint()");
  if (BITCNT==1) {
    for (size_t i=0; i<s.size(); i++)
      if ( (s[i]!='0') && (s[i]!='1') ) FATAL_ERROR("StringSizeAndValueListConstraint::StringSizeAndValueListConstraint()");
  } else if ( (BITCNT==4) || (BITCNT==8) ) {
    for (size_t i=0; i<s.size(); i++)
      if ( !((s[i]>='0')&&(s[i]<='9')) && !((s[i]>='A')&&(s[i]<='F')) )
        FATAL_ERROR("StringSizeAndValueListConstraint::StringSizeAndValueListConstraint()");
  } else {
    FATAL_ERROR("StringSizeAndValueListConstraint::StringSizeAndValueListConstraint()");
  }
  has_values.add(s,NULL);
}

template<unsigned char BITCNT, unsigned short ELEMSIZE>
void StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::canonicalize(map<string,void>& values, map<string,void>& other_values, bool if_values)
{
  map<size_t,size_t> values_lengths; // length -> number of values
  for (size_t i=0; i<values.size(); i++) {
    size_t value_size = values.get_nth_key(i).size()/ELEMSIZE;
    if (values_lengths.has_key(value_size)) (*values_lengths[value_size])++;
    else values_lengths.add(value_size,new size_t(1));
  }

  for (size_t i=0; i<values_lengths.size(); i++) {
    // calculate the number of all possible values
    size_t size = values_lengths.get_nth_key(i); // length of string
    size_t count = *(values_lengths.get_nth_elem(i)); // number of strings with this length
    size_t all_values_count = ~(static_cast<size_t>(0)); // fake infinity
    if (BITCNT*size<sizeof(size_t)*8) all_values_count = ( (static_cast<size_t>(1)) << (BITCNT*size) );
    if (count==all_values_count) {
      // delete all values which have this size
      for (size_t hv_idx=0; hv_idx<values.size(); hv_idx++) {
        if ((values.get_nth_key(hv_idx).size()/ELEMSIZE)==size) {
          values.erase(values.get_nth_key(hv_idx));
          hv_idx--;
        }
      }
      // add/remove a single value size constraint with this size
      if (if_values) size_constraint = size_constraint + SizeRangeListConstraint(size_limit_t(size));
      else size_constraint = size_constraint - SizeRangeListConstraint(size_limit_t(size));
    } else
    if ( (!if_values && (count>=all_values_count/2)) || (if_values && (count>all_values_count/2)) ) {
      // add to other_values the complement of these values on the set with this size
      for (size_t act_value=0; act_value<all_values_count; act_value++) {
        string str; // for each value of act_value there is corresponding string value str
        for (size_t elem_idx=0; elem_idx<size; elem_idx++) { // for every element
          size_t ei = ( ( act_value >> (elem_idx*BITCNT) ) & ( (1<<BITCNT) - 1 ) );
          if (BITCNT==1) {
            str += '0' + static_cast<char>(ei);
          } else if (BITCNT==4) {
            str += (ei<10) ? ('0' + static_cast<char>(ei)) : ('A' + (static_cast<char>(ei-10)));
          } else if (BITCNT==8) {
            char c = ei & 0x0F;
            str += (c<10) ? ('0' + c) : ('A' + (c-10));
            c = (ei >> (BITCNT/ELEMSIZE)) & 0x0F;
            str += (c<10) ? ('0' + c) : ('A' + (c-10));
          } else {
            FATAL_ERROR("StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::canonicalize()");
          }
        }
        // if str is not element of values then add to other_values
        if (!values.has_key(str)) other_values.add(str,NULL);
      }
      // delete all values which have this size
      for (size_t hv_idx=0; hv_idx<values.size(); hv_idx++) {
        if ((values.get_nth_key(hv_idx).size()/ELEMSIZE)==size) {
          values.erase(values.get_nth_key(hv_idx));
          hv_idx--;
        }
      }
      // add/remove a single value size constraint with this size
      if (if_values) size_constraint = size_constraint + SizeRangeListConstraint(size_limit_t(size));
      else size_constraint = size_constraint - SizeRangeListConstraint(size_limit_t(size));
    }
  }
  // clean up
  for (size_t i=0; i<values_lengths.size(); i++) delete (values_lengths.get_nth_elem(i));
  values_lengths.clear();
}

template<unsigned char BITCNT, unsigned short ELEMSIZE>
void StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::canonicalize()
{
  canonicalize(has_values, not_values, true);
  canonicalize(not_values, has_values, false);
}

template<unsigned char BITCNT, unsigned short ELEMSIZE>
tribool StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::is_equal(const StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>& other) const
{
  if (size_constraint.is_equal(other.size_constraint)==TFALSE) return TFALSE;
  if (has_values.size()!=other.has_values.size()) return TFALSE;
  if (not_values.size()!=other.not_values.size()) return TFALSE;
  for (size_t i=0; i<has_values.size(); i++) if (has_values.get_nth_key(i)!=other.has_values.get_nth_key(i)) return TFALSE;
  for (size_t i=0; i<not_values.size(); i++) if (not_values.get_nth_key(i)!=other.not_values.get_nth_key(i)) return TFALSE;
  return TTRUE;
}

template<unsigned char BITCNT, unsigned short ELEMSIZE>
bool StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::is_element(const string& s) const
{
  return ( ( size_constraint.is_element(s.size()/ELEMSIZE) && !not_values.has_key(s) ) || has_values.has_key(s) );
}

// representation of two sets: [Si+Vi-Ni] where Si=size_constraint, Vi=has_values, Ni=not_values
// UNION:        [S1+V1-N1] + [S2+V2-N2] = ... = [(S1+S2)+(V1+V2)-((~S1*N2)+(N1*~S2)+(N1*N2))]
// INTERSECTION: [S1+V1-N1] * [S2+V2-N2] = ... = [(S1*S2)+((S1*V2-N1)+(S2*V1-N2)+(V1*V2))-(N1+N2)]
template<unsigned char BITCNT, unsigned short ELEMSIZE>
StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>
  StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::
  set_operation(const StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>& other, bool is_union) const
{
  StringSizeAndValueListConstraint<BITCNT,ELEMSIZE> ret_val;
  ret_val.size_constraint = size_constraint.set_operation(other.size_constraint, is_union);
  if (is_union) {
    // V1+V2 (union of has_values)
    for (size_t i=0; i<has_values.size(); i++) ret_val.has_values.add(has_values.get_nth_key(i),NULL);
    for (size_t i=0; i<other.has_values.size(); i++) {
      const string& str = other.has_values.get_nth_key(i);
      if (!ret_val.has_values.has_key(str)) ret_val.has_values.add(str,NULL);
    }
    // N1*N2 (intersection of not_values)
    for (size_t i=0; i<not_values.size(); i++) {
      const string& str = not_values.get_nth_key(i);
      if (other.not_values.has_key(str)) ret_val.not_values.add(str,NULL);
    }
    // ~S1*N2
    for (size_t i=0; i<other.not_values.size(); i++) {
      const string& str = other.not_values.get_nth_key(i);
      if (!size_constraint.is_element(size_limit_t(str.size()/ELEMSIZE)) &&
          !ret_val.not_values.has_key(str)) ret_val.not_values.add(str,NULL);
    }
    // N1*~S2
    for (size_t i=0; i<not_values.size(); i++) {
      const string& str = not_values.get_nth_key(i);
      if (!other.size_constraint.is_element(size_limit_t(str.size()/ELEMSIZE)) &&
          !ret_val.not_values.has_key(str)) ret_val.not_values.add(str,NULL);
    }
  } else { // intersection
    // V1*V2 (intersection of has_values)
    for (size_t i=0; i<has_values.size(); i++) {
      const string& str = has_values.get_nth_key(i);
      if (other.has_values.has_key(str)) ret_val.has_values.add(str,NULL);
    }
    // S2*V1-N2
    for (size_t i=0; i<has_values.size(); i++) {
      const string& str = has_values.get_nth_key(i);
      if (other.size_constraint.is_element(size_limit_t(str.size()/ELEMSIZE)) &&
          !other.not_values.has_key(str) &&
          !ret_val.has_values.has_key(str)) ret_val.has_values.add(str,NULL);
    }
    // S1*V2-N1
    for (size_t i=0; i<other.has_values.size(); i++) {
      const string& str = other.has_values.get_nth_key(i);
      if (size_constraint.is_element(size_limit_t(str.size()/ELEMSIZE)) &&
          !not_values.has_key(str) &&
          !ret_val.has_values.has_key(str)) ret_val.has_values.add(str,NULL);
    }
    // N1+N2 (union of not_values)
    for (size_t i=0; i<not_values.size(); i++) ret_val.not_values.add(not_values.get_nth_key(i),NULL);
    for (size_t i=0; i<other.not_values.size(); i++) {
      const string& str = other.not_values.get_nth_key(i);
      if (!ret_val.not_values.has_key(str)) ret_val.not_values.add(str,NULL);
    }
  }
  {
    // drop ret_val.has_values that are elements of ret_val.not_values too, drop from ret_val.not_values too
    for (size_t i=0; i<ret_val.not_values.size(); i++) {
      string str = ret_val.not_values.get_nth_key(i);
      if (ret_val.has_values.has_key(str)) {
        ret_val.has_values.erase(str);
        ret_val.not_values.erase(str);
        i--;
      }
    }
    // drop ret_val.has_values elements that are elements of the ret_val.size_constraint set
    for (size_t i=0; i<ret_val.has_values.size(); i++) {
      string str = ret_val.has_values.get_nth_key(i);
      if (ret_val.size_constraint.is_element(size_limit_t(str.size()/ELEMSIZE))) {
        ret_val.has_values.erase(str);
        i--;
      }
    }
    // drop ret_val.not_values elements that are not elements of the ret_val.size_constraint set
    for (size_t i=0; i<ret_val.not_values.size(); i++) {
      string str = ret_val.not_values.get_nth_key(i);
      if (!ret_val.size_constraint.is_element(size_limit_t(str.size()/ELEMSIZE))) {
        ret_val.not_values.erase(str);
        i--;
      }
    }
  }
  ret_val.canonicalize();
  return ret_val;
}

template<unsigned char BITCNT, unsigned short ELEMSIZE>
StringSizeAndValueListConstraint<BITCNT,ELEMSIZE> StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::operator~() const
{
  StringSizeAndValueListConstraint<BITCNT,ELEMSIZE> ret_val;
  ret_val.size_constraint = ~size_constraint;
  for (size_t i=0; i<has_values.size(); i++) ret_val.not_values.add(has_values.get_nth_key(i),NULL);
  for (size_t i=0; i<not_values.size(); i++) ret_val.has_values.add(not_values.get_nth_key(i),NULL);
  ret_val.canonicalize();
  return ret_val;
}

template<unsigned char BITCNT, unsigned short ELEMSIZE>
string StringSizeAndValueListConstraint<BITCNT,ELEMSIZE>::to_string() const
{
  string ret_val;
  if (has_values.size()>0) {
    ret_val += '(';
    for (size_t i=0; i<has_values.size(); i++) {
      if (i>0) ret_val += ',';
      ret_val += '\'';
      ret_val += has_values.get_nth_key(i);
      ret_val += '\'';
      if (BITCNT==1) ret_val += 'B';
      else if (BITCNT==4) ret_val += 'H';
      else if (BITCNT==8) ret_val += 'O';
    }
    ret_val += ')';
  }
  // size constraint
  if (size_constraint.is_empty()!=TTRUE) {
    if (has_values.size()>0) ret_val += " union ";
    ret_val += "length";
    ret_val += size_constraint.to_string();
  }
  // except not_values
  if (not_values.size()>0) {
    ret_val += " except (";
    for (size_t i=0; i<not_values.size(); i++) {
      if (i>0) ret_val += ',';
      ret_val += '\'';
      ret_val += not_values.get_nth_key(i);
      ret_val += '\'';
      if (BITCNT==1) ret_val += 'B';
      else if (BITCNT==4) ret_val += 'H';
      else if (BITCNT==8) ret_val += 'O';
    }
    ret_val += ')';
  }
  return ret_val;
}

typedef StringSizeAndValueListConstraint<1,1> BitstringConstraint;
typedef StringSizeAndValueListConstraint<4,1> HexstringConstraint;
typedef StringSizeAndValueListConstraint<8,2> OctetstringConstraint; // one char is half octet

////////////////////////////////////////////////////////////////////////////////

class StringPatternConstraint
{
private:
  Ttcn::PatternString* pattern; // not owned
public:
  // constructors
  StringPatternConstraint() : pattern(0) {} // empty set
  StringPatternConstraint(Ttcn::PatternString* p): pattern(p)  {}

  tribool is_empty() const { return TUNKNOWN; }
  tribool is_full() const { return TUNKNOWN; }
  tribool is_equal(const StringPatternConstraint&) const { return TUNKNOWN; }

  tribool match(const string& str) const;
  tribool match(const ustring&) const { return TUNKNOWN; } // TODO

  StringPatternConstraint set_operation(const StringPatternConstraint&, bool) const { FATAL_ERROR("StringPatternConstraint::set_operation(): not implemented"); }

  StringPatternConstraint operator+(const StringPatternConstraint& other) const { return set_operation(other, true); } // union
  StringPatternConstraint operator*(const StringPatternConstraint& other) const { return set_operation(other, false); } // intersection
  StringPatternConstraint operator~() const { FATAL_ERROR("StringPatternConstraint::operator~(): not implemented"); }

  tribool is_subset(const StringPatternConstraint&) const { return TUNKNOWN; }
  StringPatternConstraint operator-(const StringPatternConstraint& other) const { return ( *this * ~other ); } // except

  string to_string() const;
};

////////////////////////////////////////////////////////////////////////////////

template <class STRINGTYPE>
class StringValueConstraint
{
private:
  map<STRINGTYPE,void> values;
  void clean_up();
  void copy_content(const StringValueConstraint& other);
public:
  // constructors
  StringValueConstraint() {} // empty set
  StringValueConstraint(const STRINGTYPE& s) { values.add(s,NULL); } // single value set

  StringValueConstraint(const StringValueConstraint& other) { copy_content(other); }
  StringValueConstraint& operator=(const StringValueConstraint& other) { clean_up(); copy_content(other); return *this; }
  ~StringValueConstraint() { clean_up(); }

  tribool is_empty() const { return TRIBOOL(values.size()==0); }
  tribool is_full() const { return TFALSE; }
  tribool is_equal(const StringValueConstraint& other) const;
  bool is_element(const STRINGTYPE& s) const { return values.has_key(s); }

  StringValueConstraint set_operation(const StringValueConstraint& other, bool is_union) const;

  StringValueConstraint operator+(const StringValueConstraint& other) const { return set_operation(other, true); } // union
  StringValueConstraint operator*(const StringValueConstraint& other) const { return set_operation(other, false); } // intersection

  tribool is_subset(const StringValueConstraint& other) const { return (*this-other).is_empty(); }
  StringValueConstraint operator-(const StringValueConstraint& other) const; // except

  // remove strings that are or are not elements of the set defined by the XXX_constraint object,
  // using the XXX_constraint.is_element() member function
  void remove(const SizeRangeListConstraint& size_constraint, bool if_element);
  template <class CHARLIMITTYPE> void remove(const RangeListConstraint<CHARLIMITTYPE>& alphabet_constraint, bool if_element);
  void remove(const StringPatternConstraint& pattern_constraint, bool if_element);

  string to_string() const;
};

template <class STRINGTYPE>
void StringValueConstraint<STRINGTYPE>::clean_up()
{
  values.clear();
}

template <class STRINGTYPE>
void StringValueConstraint<STRINGTYPE>::copy_content(const StringValueConstraint<STRINGTYPE>& other)
{
  for (size_t i=0; i<other.values.size(); i++) values.add(other.values.get_nth_key(i),NULL);
}

template <class STRINGTYPE>
tribool StringValueConstraint<STRINGTYPE>::is_equal(const StringValueConstraint<STRINGTYPE>& other) const
{
  if (values.size()!=other.values.size()) return TFALSE;
  for (size_t i=0; i<values.size(); i++) {
    if (values.get_nth_key(i)!=other.values.get_nth_key(i)) return TFALSE;
  }
  return TTRUE;
}

template <class STRINGTYPE>
StringValueConstraint<STRINGTYPE> StringValueConstraint<STRINGTYPE>::set_operation
  (const StringValueConstraint<STRINGTYPE>& other, bool is_union) const
{
  StringValueConstraint<STRINGTYPE> ret_val;
  if (is_union) {
    for (size_t i=0; i<values.size(); i++) ret_val.values.add(values.get_nth_key(i), NULL);
    for (size_t i=0; i<other.values.size(); i++) {
      const STRINGTYPE& str = other.values.get_nth_key(i);
      if (!ret_val.values.has_key(str)) ret_val.values.add(str, NULL);
    }
  } else {
    for (size_t i=0; i<values.size(); i++) {
      const STRINGTYPE& str = values.get_nth_key(i);
      if (other.values.has_key(str)) ret_val.values.add(str, NULL);
    }
  }
  return ret_val;
}

template <class STRINGTYPE>
StringValueConstraint<STRINGTYPE> StringValueConstraint<STRINGTYPE>::operator-(const StringValueConstraint<STRINGTYPE>& other) const
{
  StringValueConstraint<STRINGTYPE> ret_val;
  for (size_t i=0; i<values.size(); i++) {
    const STRINGTYPE& str = values.get_nth_key(i);
    if (!other.values.has_key(str)) ret_val.values.add(str, NULL);
  }
  return ret_val;
}

template <class STRINGTYPE>
void StringValueConstraint<STRINGTYPE>::remove(const SizeRangeListConstraint& size_constraint, bool if_element)
{
  for (size_t i=0; i<values.size(); i++) {
    STRINGTYPE str = values.get_nth_key(i);
    if (size_constraint.is_element(size_limit_t(str.size()))==if_element) {
      values.erase(str);
      i--;
    }
  }
}

template <class STRINGTYPE>
template <class CHARLIMITTYPE>
void StringValueConstraint<STRINGTYPE>::remove(const RangeListConstraint<CHARLIMITTYPE>& alphabet_constraint, bool if_element)
{
  for (size_t i=0; i<values.size(); i++) {
    STRINGTYPE str = values.get_nth_key(i);
    bool all_chars_are_elements = true;
    for (size_t chr_idx=0; chr_idx<str.size(); chr_idx++)
    {
      if (!alphabet_constraint.is_element(CHARLIMITTYPE(str[chr_idx]))) {
        all_chars_are_elements = false;
        break;
      }
    }
    if (all_chars_are_elements==if_element) {
      values.erase(str);
      i--;
    }
  }
}

template <class STRINGTYPE>
void StringValueConstraint<STRINGTYPE>::remove(const StringPatternConstraint& pattern_constraint, bool if_element)
{
  for (size_t i=0; i<values.size(); i++) {
    STRINGTYPE str = values.get_nth_key(i);
    switch (pattern_constraint.match(str)) {
    case TFALSE:
      if (!if_element) { values.erase(str); i--; }
      break;
    case TUNKNOWN:
      break;
    case TTRUE:
      if (if_element) { values.erase(str); i--; }
      break;
    default:
      FATAL_ERROR("StringValueConstraint::remove(StringPatternConstraint)");
    }
  }
}

template <class STRINGTYPE>
string StringValueConstraint<STRINGTYPE>::to_string() const
{
  string ret_val;
  ret_val += '(';
  for (size_t i=0; i<values.size(); i++) {
    if (i>0) ret_val += ',';
    STRINGTYPE str = values.get_nth_key(i);
    ret_val += str.get_stringRepr();
  }
  ret_val += ')';
  return ret_val;
}

////////////////////////////////////////////////////////////////////////////////

// contains a tree of subtype constraints
template <class STRINGTYPE, class CHARLIMITTYPE>
class StringSubtypeTreeElement
{
public:
  enum elementtype_t {
    ET_NONE,          // empty set
    ET_ALL,           // all values of the root type, no subtype constraint was given, other data members have no meaning
    ET_CONSTRAINT,    // constraint value
    ET_INTERSECTION,  // A intersection B
    ET_UNION,         // A union B
    ET_EXCEPT         // A except B
  };
  enum constrainttype_t {
    CT_SIZE,
    CT_ALPHABET,
    CT_VALUES,
    CT_PATTERN
  };
private:
  elementtype_t elementtype;
  union {
    struct { // operation (ET_INTERSECTION, ET_UNION, ET_EXCEPT)
      StringSubtypeTreeElement* a; // owned
      StringSubtypeTreeElement* b; // owned
    } op;
    struct { // constraint
      constrainttype_t constrainttype;
      union { // constraint objects are owned
        SizeRangeListConstraint* s;             // size/length constraint type
        struct {
          RangeListConstraint<CHARLIMITTYPE>* c;  // range/alphabet constraint type
          bool char_context; // this constraint can be either in char or string context
                             // char context is constraining a single char,
                             // string context is constraining a string
                             // this bool value affects evaluation
                             // in char context only range,all,none and operation
                             // constructors can be called, operations must always evaluate
                             // to range, all or none
        } a;
        StringValueConstraint<STRINGTYPE>* v;   // value set constraint
        StringPatternConstraint* p; // pattern constraint
      };
    } cs;
  } u;
  void clean_up();
  void copy_content(const StringSubtypeTreeElement& other); // *this must be empty
  void set_to_operand(bool operand_a);
  void simplify(); // convert to ET_NONE or ET_ALL if possible
  void evaluate(); // tries to evaluate a tree to a single constraint, works recursively for the whole tree
public:
  StringSubtypeTreeElement(): elementtype(ET_NONE) {}
  StringSubtypeTreeElement(elementtype_t p_elementtype, StringSubtypeTreeElement* p_a, StringSubtypeTreeElement* p_b);
  StringSubtypeTreeElement(const SizeRangeListConstraint& p_s);
  StringSubtypeTreeElement(const RangeListConstraint<CHARLIMITTYPE>& p_a, bool p_char_context);
  StringSubtypeTreeElement(const StringValueConstraint<STRINGTYPE>& p_v);
  StringSubtypeTreeElement(const StringPatternConstraint& p_p);

  StringSubtypeTreeElement(const StringSubtypeTreeElement& p) { copy_content(p); }
  StringSubtypeTreeElement& operator=(const StringSubtypeTreeElement& other) { clean_up(); copy_content(other); return *this; }
  ~StringSubtypeTreeElement() { clean_up(); }

  tribool is_empty() const;
  tribool is_full() const;
  tribool is_equal(const StringSubtypeTreeElement* other) const;
  bool is_element(const STRINGTYPE& s) const;
  tribool is_subset(const StringSubtypeTreeElement* other) const;

  bool is_single_constraint() const { return ( (elementtype==ET_CONSTRAINT) || (elementtype==ET_NONE) || (elementtype==ET_ALL) ); }
  void set_none() { clean_up(); elementtype = ET_NONE; }
  void set_all() { clean_up(); elementtype = ET_ALL; }

  /** return value:
        TFALSE: limit does not exist (empty set or values, ...)
        TUNKNOWN: limit cannot be determined
        TTRUE: limit was set to the proper value  */
  tribool get_alphabet_limit(bool is_upper, CHARLIMITTYPE& limit) const;
  tribool get_size_limit(bool is_upper, size_limit_t& limit) const;

  bool is_valid_range() const;
  void set_char_context(bool p_char_context);

  string to_string() const;
};

template <class STRINGTYPE, class CHARLIMITTYPE>
StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::StringSubtypeTreeElement(elementtype_t p_elementtype, StringSubtypeTreeElement* p_a, StringSubtypeTreeElement* p_b)
: elementtype(p_elementtype)
{
  switch (elementtype) {
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    break;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::StringSubtypeTreeElement()");
  }
  u.op.a = p_a;
  u.op.b = p_b;
  evaluate();
}


template <class STRINGTYPE, class CHARLIMITTYPE>
tribool StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::
  get_alphabet_limit(bool is_upper, CHARLIMITTYPE& limit) const
{
  switch (elementtype) {
  case ET_NONE:
    return TFALSE;
  case ET_ALL:
    limit = is_upper ? CHARLIMITTYPE::maximum : CHARLIMITTYPE::minimum;
    return TTRUE;
  case ET_CONSTRAINT:
    switch (u.cs.constrainttype) {
    case CT_SIZE:
      limit = is_upper ? CHARLIMITTYPE::maximum : CHARLIMITTYPE::minimum;
      return TTRUE;
    case CT_ALPHABET:
      if (u.cs.a.c->is_empty()==TTRUE) return TFALSE;
      limit = is_upper ? u.cs.a.c->get_maximal() : u.cs.a.c->get_minimal();
      return TTRUE;
    case CT_VALUES:
      return TFALSE;
    case CT_PATTERN:
      return TUNKNOWN;
    default:
      FATAL_ERROR("StringSubtypeTreeElement::get_alphabet_limit()");
    }
  case ET_INTERSECTION: {
    CHARLIMITTYPE la, lb;
    tribool tb = u.op.a->get_alphabet_limit(is_upper, la) && u.op.b->get_alphabet_limit(is_upper, lb);
    if (tb==TTRUE) {
      limit = ((lb<la) ^ !is_upper) ? lb : la;
    }
    return tb;
  }
  case ET_UNION:
  case ET_EXCEPT:
    return TUNKNOWN;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::get_alphabet_limit()");
  }
  return TUNKNOWN;
}

template <class STRINGTYPE, class CHARLIMITTYPE>
tribool StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::
  get_size_limit(bool is_upper, size_limit_t& limit) const
{
  switch (elementtype) {
  case ET_NONE:
    return TFALSE;
  case ET_ALL:
    limit = is_upper ? size_limit_t::maximum : size_limit_t::minimum;
    return TTRUE;
  case ET_CONSTRAINT:
    switch (u.cs.constrainttype) {
    case CT_SIZE:
      if (u.cs.s->is_empty()==TTRUE) return TFALSE;
      limit = is_upper ? u.cs.s->get_maximal() : u.cs.s->get_minimal();
      return TTRUE;
    case CT_ALPHABET:
      limit = is_upper ? size_limit_t::maximum : size_limit_t::minimum;
      return TTRUE;
    case CT_VALUES:
      return TFALSE;
    case CT_PATTERN:
      return TUNKNOWN;
    default:
      FATAL_ERROR("StringSubtypeTreeElement::get_size_limit()");
    }
  case ET_INTERSECTION: {
    size_limit_t la, lb;
    tribool tb = u.op.a->get_size_limit(is_upper, la) && u.op.b->get_size_limit(is_upper, lb);
    if (tb==TTRUE) {
      limit = ((lb<la) ^ !is_upper) ? lb : la;
    }
    return tb;
  }
  case ET_UNION:
  case ET_EXCEPT:
    return TUNKNOWN;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::get_size_limit()");
  }
  return TUNKNOWN;
}

template <class STRINGTYPE, class CHARLIMITTYPE>
bool StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::is_valid_range() const
{
  switch (elementtype) {
  case ET_NONE:
  case ET_ALL:
    return true;
  case ET_CONSTRAINT:
    switch (u.cs.constrainttype) {
    case CT_SIZE:
      // must be SIZE(1)
      return (u.cs.s->is_equal(SizeRangeListConstraint(size_limit_t(1)))==TTRUE);
    case CT_ALPHABET:
      return true;
    case CT_VALUES:
    case CT_PATTERN:
      return false;
    default:
      FATAL_ERROR("StringSubtypeTreeElement::is_valid_range()");
    }
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    return ( u.op.a->is_valid_range() && u.op.b->is_valid_range() );
  default:
    FATAL_ERROR("StringSubtypeTreeElement::is_valid_range()");
  }
  return false;
}

template <class STRINGTYPE, class CHARLIMITTYPE>
void StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::set_char_context(bool p_char_context)
{
  switch (elementtype) {
  case ET_NONE:
  case ET_ALL:
    break;
  case ET_CONSTRAINT:
    u.cs.a.char_context = p_char_context;
    switch (u.cs.constrainttype) {
    case CT_SIZE:
      if (p_char_context) set_all(); // false -> true
      else FATAL_ERROR("StringSubtypeTreeElement::set_char_context()");
    case CT_ALPHABET:
      break;
    default:
      FATAL_ERROR("StringSubtypeTreeElement::set_char_context()");
    }
    break;
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    u.op.a->set_char_context(p_char_context);
    u.op.b->set_char_context(p_char_context);
    break;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::set_char_context()");
  }
}

template <class STRINGTYPE, class CHARLIMITTYPE>
StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::StringSubtypeTreeElement
  (const SizeRangeListConstraint& p_s):
  elementtype(ET_CONSTRAINT)
{
  u.cs.constrainttype = CT_SIZE;
  u.cs.s = new SizeRangeListConstraint(p_s);
  simplify();
}

template <class STRINGTYPE, class CHARLIMITTYPE>
StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::StringSubtypeTreeElement
  (const RangeListConstraint<CHARLIMITTYPE>& p_a, bool p_char_context):
  elementtype(ET_CONSTRAINT)
{
  u.cs.constrainttype = CT_ALPHABET;
  u.cs.a.c = new RangeListConstraint<CHARLIMITTYPE>(p_a);
  u.cs.a.char_context = p_char_context;
  simplify();
}

template <class STRINGTYPE, class CHARLIMITTYPE>
StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::StringSubtypeTreeElement
  (const StringValueConstraint<STRINGTYPE>& p_v):
  elementtype(ET_CONSTRAINT)
{
  u.cs.constrainttype = CT_VALUES;
  u.cs.v = new StringValueConstraint<STRINGTYPE>(p_v);
  simplify();
}

template <class STRINGTYPE, class CHARLIMITTYPE>
StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::StringSubtypeTreeElement
  (const StringPatternConstraint& p_p):
  elementtype(ET_CONSTRAINT)
{
  u.cs.constrainttype = CT_PATTERN;
  u.cs.p = new StringPatternConstraint(p_p);
  simplify();
}

template <class STRINGTYPE, class CHARLIMITTYPE>
void StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::clean_up()
{
  switch (elementtype) {
  case ET_NONE:
  case ET_ALL:
    break;
  case ET_CONSTRAINT:
    switch (u.cs.constrainttype) {
    case CT_SIZE: delete u.cs.s; break;
    case CT_ALPHABET: delete u.cs.a.c; break;
    case CT_VALUES: delete u.cs.v; break;
    case CT_PATTERN: delete u.cs.p; break;
    default: FATAL_ERROR("StringSubtypeTreeElement::clean_up()");
    }
    break;
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    delete u.op.a;
    delete u.op.b;
    break;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::clean_up()");
  }
}

template <class STRINGTYPE, class CHARLIMITTYPE>
void StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::copy_content(const StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>& other)
{
  elementtype = other.elementtype;
  switch (elementtype) {
  case ET_NONE:
  case ET_ALL:
    break;
  case ET_CONSTRAINT:
    u.cs.constrainttype = other.u.cs.constrainttype;
    switch (u.cs.constrainttype) {
    case CT_SIZE: u.cs.s = new SizeRangeListConstraint(*(other.u.cs.s)); break;
    case CT_ALPHABET:
      u.cs.a.c = new RangeListConstraint<CHARLIMITTYPE>(*(other.u.cs.a.c));
      u.cs.a.char_context = other.u.cs.a.char_context;
      break;
    case CT_VALUES: u.cs.v = new StringValueConstraint<STRINGTYPE>(*(other.u.cs.v)); break;
    case CT_PATTERN: u.cs.p = new StringPatternConstraint(*(other.u.cs.p)); break;
    default: FATAL_ERROR("StringSubtypeTreeElement::copy_content()");
    }
    break;
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    u.op.a = new StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>(*(other.u.op.a));
    u.op.b = new StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>(*(other.u.op.b));
    break;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::copy_content()");
  }
}

template <class STRINGTYPE, class CHARLIMITTYPE>
void StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::set_to_operand(bool operand_a)
{
  switch (elementtype) {
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    break;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::copy_operand()");
  }
  StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>* op;
  if (operand_a) {
    delete u.op.b;
    op = u.op.a;
  } else {
    delete u.op.a;
    op = u.op.b;
  }
  // steal the content of op into myself
  elementtype = op->elementtype;
  switch (elementtype) {
  case ET_NONE:
  case ET_ALL:
    break;
  case ET_CONSTRAINT:
    u.cs.constrainttype = op->u.cs.constrainttype;
    switch (u.cs.constrainttype) {
    case CT_SIZE: u.cs.s = op->u.cs.s; break;
    case CT_ALPHABET:
      u.cs.a.c = op->u.cs.a.c;
      u.cs.a.char_context = op->u.cs.a.char_context;
      break;
    case CT_VALUES: u.cs.v = op->u.cs.v; break;
    case CT_PATTERN: u.cs.p = op->u.cs.p; break;
    default: FATAL_ERROR("StringSubtypeTreeElement::copy_operand()");
    }
    break;
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    u.op.a = op->u.op.a;
    u.op.b = op->u.op.b;
    break;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::copy_operand()");
  }
  // delete the empty op
  op->elementtype = ET_NONE;
  delete op;
}

template <class STRINGTYPE, class CHARLIMITTYPE>
void StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::simplify()
{
  switch (elementtype) {
  case ET_NONE:
  case ET_ALL:
    break;
  case ET_CONSTRAINT:
    switch (u.cs.constrainttype) {
    case CT_SIZE:
      if (u.cs.s->is_empty()==TTRUE) { set_none(); return; }
      if (u.cs.s->is_full()==TTRUE) { set_all(); return; }
      break;
    case CT_ALPHABET:
      if (u.cs.a.c->is_empty()==TTRUE) { set_none(); return; }
      if (u.cs.a.c->is_full()==TTRUE) { set_all(); return; }
      break;
    case CT_VALUES:
      if (u.cs.v->is_empty()==TTRUE) { set_none(); return; }
      if (u.cs.v->is_full()==TTRUE) { set_all(); return; }
      break;
    case CT_PATTERN:
      if (u.cs.p->is_empty()==TTRUE) { set_none(); return; }
      if (u.cs.p->is_full()==TTRUE) { set_all(); return; }
      break;
    default: FATAL_ERROR("StringSubtypeTreeElement::simplify()");
    }
    break;
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    break;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::simplify()");
  }
}

template <class STRINGTYPE, class CHARLIMITTYPE>
tribool StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::is_empty() const
{
  switch (elementtype) {
  case ET_NONE:
    return TTRUE;
  case ET_ALL:
    return TFALSE;
  case ET_CONSTRAINT:
    switch (u.cs.constrainttype) {
    case CT_SIZE: return u.cs.s->is_empty();
    case CT_ALPHABET: return ( u.cs.a.char_context ? u.cs.a.c->is_empty() : TFALSE );
    case CT_VALUES: return u.cs.v->is_empty();
    case CT_PATTERN: return u.cs.p->is_empty();
    default: FATAL_ERROR("StringSubtypeTreeElement::is_empty()");
    }
  case ET_INTERSECTION:
    return ( u.op.a->is_empty() || u.op.b->is_empty() );
  case ET_UNION:
    return ( u.op.a->is_empty() && u.op.b->is_empty() );
  case ET_EXCEPT: {
    tribool a_empty = u.op.a->is_empty();
    return ( (a_empty!=TFALSE) ? a_empty :
             ( (u.op.b->is_empty()==TTRUE) ? TFALSE : TUNKNOWN ) );
  }
  default:
    FATAL_ERROR("StringSubtypeTreeElement::is_empty()");
  }
  return TUNKNOWN;
}

template <class STRINGTYPE, class CHARLIMITTYPE>
tribool StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::is_full() const
{
  switch (elementtype) {
  case ET_NONE:
    return TFALSE;
  case ET_ALL:
    return TTRUE;
  case ET_CONSTRAINT:
    switch (u.cs.constrainttype) {
    case CT_SIZE: return u.cs.s->is_full();
    case CT_ALPHABET: return u.cs.a.c->is_full();
    case CT_VALUES: return u.cs.v->is_full();
    case CT_PATTERN: return u.cs.p->is_full();
    default: FATAL_ERROR("StringSubtypeTreeElement::is_full()");
    }
  case ET_INTERSECTION:
    return ( u.op.a->is_full() && u.op.b->is_full() );
  case ET_UNION:
    return ( u.op.a->is_full() || u.op.b->is_full() );
  case ET_EXCEPT:
    return ( u.op.a->is_full() && u.op.b->is_empty() );
  default:
    FATAL_ERROR("StringSubtypeTreeElement::is_full()");
  }
  return TUNKNOWN;
}

template <class STRINGTYPE, class CHARLIMITTYPE>
tribool StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::is_equal(const StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>* other) const
{
  if (elementtype!=other->elementtype) return TUNKNOWN;
  switch (elementtype) {
  case ET_NONE:
  case ET_ALL:
    return TTRUE;
  case ET_CONSTRAINT:
    if (u.cs.constrainttype!=other->u.cs.constrainttype) return TUNKNOWN;
    switch (u.cs.constrainttype) {
    case CT_SIZE: return u.cs.s->is_equal(*(other->u.cs.s));
    case CT_ALPHABET: return u.cs.a->is_equal(*(other->u.cs.a));
    case CT_VALUES: return u.cs.v->is_equal(*(other->u.cs.v));
    case CT_PATTERN: return u.cs.p->is_equal(*(other->u.cs.p));
    default: FATAL_ERROR("StringSubtypeTreeElement::is_equal()");
    }
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    return TUNKNOWN;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::is_equal()");
  }
  return TUNKNOWN;
}

template <class STRINGTYPE, class CHARLIMITTYPE>
bool StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::is_element(const STRINGTYPE& s) const
{
  switch (elementtype) {
  case ET_NONE:
    return false;
  case ET_ALL:
    return true;
  case ET_CONSTRAINT:
    switch (u.cs.constrainttype) {
    case CT_SIZE: return u.cs.s->is_element(size_limit_t(s.size()));
    case CT_ALPHABET: {
      for (size_t i=0; i<s.size(); i++) {
        CHARLIMITTYPE cl(s[i]);
        if (!u.cs.a.c->is_element(cl)) return false;
      }
      return true;
    }
    case CT_VALUES: return u.cs.v->is_element(s);
    case CT_PATTERN: {
      switch (u.cs.p->match(s)) {
      case TFALSE: return false;
      case TUNKNOWN: return true; // don't know if it matches
      case TTRUE: return true;
      default: FATAL_ERROR("StringSubtypeTreeElement::is_element()");
      }
    }
    default: FATAL_ERROR("StringSubtypeTreeElement::is_element()");
    }
  case ET_INTERSECTION:
    return ( u.op.a->is_element(s) && u.op.b->is_element(s) );
  case ET_UNION:
    return ( u.op.a->is_element(s) || u.op.b->is_element(s) );
  case ET_EXCEPT:
    return ( u.op.a->is_element(s) && !u.op.b->is_element(s) );
  default:
    FATAL_ERROR("StringSubtypeTreeElement::is_element()");
  }
  return true; // don't know if it matches
}

// if the constraints are ortogonal (e.g. size and alphabet) or just different then return TUNKNOWN
// in case of ortogonal constraints we should return TFALSE (if other is not full set)
// but it seems that the standard wants to ignore such trivial cases, example:
//   length(1..4) is_subset ('a'..'z') shall not report an error
template <class STRINGTYPE, class CHARLIMITTYPE>
tribool StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::is_subset(const StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>* other) const
{
  switch (elementtype) {
  case ET_NONE:
    return TTRUE;
  case ET_ALL:
    if (other->elementtype==ET_ALL) return TTRUE;
    else return TUNKNOWN;
  case ET_CONSTRAINT:
    if (elementtype!=other->elementtype) return TUNKNOWN;
    if (u.cs.constrainttype!=other->u.cs.constrainttype) return TUNKNOWN;
    switch (u.cs.constrainttype) {
    case CT_SIZE: return u.cs.s->is_subset(*(other->u.cs.s));
    case CT_ALPHABET: return u.cs.a.c->is_subset(*(other->u.cs.a.c));
    case CT_VALUES: return u.cs.v->is_subset(*(other->u.cs.v));
    case CT_PATTERN: return u.cs.p->is_subset(*(other->u.cs.p));
    default: FATAL_ERROR("StringSubtypeTreeElement::is_subset()");
    }
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    return TUNKNOWN;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::is_subset()");
  }
  return TUNKNOWN;
}

template <class STRINGTYPE, class CHARLIMITTYPE>
void StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::evaluate()
{
  switch (elementtype) {
  case ET_NONE:
  case ET_ALL:
  case ET_CONSTRAINT:
    // these are the simplest forms, others are reduced to these in ideal case
    return;
  case ET_INTERSECTION:
  case ET_UNION:
  case ET_EXCEPT:
    break;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::evaluate()");
  }

  // recursive evaluation
  u.op.a->evaluate();
  u.op.b->evaluate();

  // special case where the construct looks like this:
  // 1. ( A + B ) + C
  // 2. A + ( B + C )
  // 3. (A+B) + (C+D)
  // can be union or intersection, try to exchange constraint to have same type of constraints as operands,
  // constraint types: tree, ....
  // if succeeded then evaluate the new construct
  // TODO...

  // special simple cases when one side is ET_ALL or ET_NONE but the other can be a tree
  if ( (u.op.a->elementtype==ET_NONE) || (u.op.b->elementtype==ET_NONE) ) {
    if (elementtype==ET_INTERSECTION) { set_none(); return; }
    if (elementtype==ET_UNION) {
      // the result is the other set
      set_to_operand(u.op.b->elementtype==ET_NONE);
      return;
    }
  }
  if ( (u.op.b->elementtype==ET_NONE) && (elementtype==ET_EXCEPT) ) {
    set_to_operand(true);
    return;
  }
  if ( (u.op.a->elementtype==ET_ALL) || (u.op.b->elementtype==ET_ALL) ) {
    if (elementtype==ET_INTERSECTION) {
      // the result is the other set
      set_to_operand(u.op.b->elementtype==ET_ALL);
      return;
    }
    if (elementtype==ET_UNION) { set_all(); return; }
  }
  if ( (u.op.b->elementtype==ET_ALL) && (elementtype==ET_EXCEPT) ) { set_none(); return; }

  // both operands must be single constraints (ALL,NONE,CONSTRAINT),
  // after this point trees will not be further simplified
  if (!u.op.a->is_single_constraint() || !u.op.b->is_single_constraint()) return;

  // special case: ALL - some constraint type that can be complemented
  if ( (u.op.a->elementtype==ET_ALL) && (elementtype==ET_EXCEPT) && (u.op.b->elementtype==ET_CONSTRAINT) ) {
    switch (u.op.b->u.cs.constrainttype) {
    case CT_SIZE: {
      SizeRangeListConstraint* rvs = new SizeRangeListConstraint(~*(u.op.b->u.cs.s));
      clean_up();
      elementtype = ET_CONSTRAINT;
      u.cs.constrainttype = CT_SIZE;
      u.cs.s = rvs;
      simplify();
    } return;
    case CT_ALPHABET: {
      if (u.cs.a.char_context) {
        RangeListConstraint<CHARLIMITTYPE>* rva = new RangeListConstraint<CHARLIMITTYPE>(~*(u.op.b->u.cs.a.c));
        clean_up();
        elementtype = ET_CONSTRAINT;
        u.cs.constrainttype = CT_ALPHABET;
        u.cs.a.c = rva;
        u.cs.a.char_context = true;
        simplify();
      }
    } return;
    default:
      break;
    }
  }

  // special case: when one operand is CT_VALUES then is_element() can be called for the values
  // and drop values or drop the other operand set or both depending on the operation
  StringValueConstraint<STRINGTYPE>* svc = NULL;
  bool first_is_svc = false;
  if ( (u.op.a->elementtype==ET_CONSTRAINT) && (u.op.a->u.cs.constrainttype==CT_VALUES) ) {
    svc = u.op.a->u.cs.v;
    first_is_svc = true;
  } else if ( (u.op.b->elementtype==ET_CONSTRAINT) && (u.op.b->u.cs.constrainttype==CT_VALUES) ) {
    svc = u.op.b->u.cs.v;
    first_is_svc = false; // it's the second
  }
  if (svc!=NULL) { // if one or both operands are CT_VALUES
    switch (elementtype) {
    case ET_INTERSECTION: {
      switch (first_is_svc ? u.op.b->u.cs.constrainttype : u.op.a->u.cs.constrainttype) {
      case CT_SIZE:
        svc->remove(*(first_is_svc ? u.op.b->u.cs.s : u.op.a->u.cs.s), false);
        break;
      case CT_ALPHABET:
        svc->remove(*(first_is_svc ? u.op.b->u.cs.a.c : u.op.a->u.cs.a.c), false);
        break;
      case CT_PATTERN:
        svc->remove(*(first_is_svc ? u.op.b->u.cs.p : u.op.a->u.cs.p), false);
        break;
      default:
        break;
      }
      // drop the other operand, keep the values as constraint of this object
      StringValueConstraint<STRINGTYPE>* keep_svc = new StringValueConstraint<STRINGTYPE>(*svc);
      clean_up();
      elementtype = ET_CONSTRAINT;
      u.cs.constrainttype = CT_VALUES;
      u.cs.v = keep_svc;
      simplify();
      return;
    }
    case ET_UNION: {
      switch (first_is_svc ? u.op.b->u.cs.constrainttype : u.op.a->u.cs.constrainttype) {
      case CT_SIZE:
        svc->remove(*(first_is_svc ? u.op.b->u.cs.s : u.op.a->u.cs.s), true);
        break;
      case CT_ALPHABET:
        svc->remove(*(first_is_svc ? u.op.b->u.cs.a.c : u.op.a->u.cs.a.c), true);
        break;
      case CT_PATTERN:
        svc->remove(*(first_is_svc ? u.op.b->u.cs.p : u.op.a->u.cs.p), true);
        break;
      default:
        break;
      }
      // if all values were dropped then drop the empty values operand
      if (svc->is_empty()==TTRUE) {
        set_to_operand(!first_is_svc);
        return;
      }
    } break;
    case ET_EXCEPT: {
      if (first_is_svc) { // the second operand is u.op.b->u.cs.X where X can be length/alphabet/pattern constraint
        switch (u.op.b->u.cs.constrainttype) {
        case CT_SIZE:
          svc->remove(*(u.op.b->u.cs.s), true);
          break;
        case CT_ALPHABET:
          svc->remove(*(u.op.b->u.cs.a.c), true);
          break;
        case CT_PATTERN:
          svc->remove(*(u.op.b->u.cs.p), true);
          break;
        default:
          break;
        }
        // drop the other operand, keep the values as constraint of this object
        StringValueConstraint<STRINGTYPE>* keep_svc = new StringValueConstraint<STRINGTYPE>(*svc);
        clean_up();
        elementtype = ET_CONSTRAINT;
        u.cs.constrainttype = CT_VALUES;
        u.cs.v = keep_svc;
        simplify();
        return;
      }
    } break;
    default:
      FATAL_ERROR("StringSubtypeTreeElement::evaluate()");
    }
  }

  // operands of same types can be evaluated to one constraint using their
  // set arithmetic member functions
  // alphabet constraint in string context:
  //   FROM(A) UNION FROM(B) =/= FROM(A UNION B)
  //   ALL - FROM(A) =/= FROM(ALL - A)
  //   FROM(A) INTERSECTION FROM(B) == FROM (A INTERSECTION B)
  if (u.op.a->elementtype==u.op.b->elementtype) {
    switch (u.op.a->elementtype) {
    case ET_ALL:
      if (elementtype==ET_EXCEPT) set_none();
      else set_all();
      return;
    case ET_NONE:
      set_none();
      return;
    case ET_CONSTRAINT:
      if (u.op.a->u.cs.constrainttype==u.op.b->u.cs.constrainttype) {
        switch (u.op.a->u.cs.constrainttype) {
        case CT_SIZE: {
          SizeRangeListConstraint* rvs = new SizeRangeListConstraint;
          switch (elementtype) {
          case ET_INTERSECTION:
            *rvs = *(u.op.a->u.cs.s) * *(u.op.b->u.cs.s);
            break;
          case ET_UNION:
            *rvs = *(u.op.a->u.cs.s) + *(u.op.b->u.cs.s);
            break;
          case ET_EXCEPT:
            *rvs = *(u.op.a->u.cs.s) - *(u.op.b->u.cs.s);
            break;
          default:
            FATAL_ERROR("StringSubtypeTreeElement::evaluate()");
          }
          clean_up();
          elementtype = ET_CONSTRAINT;
          u.cs.constrainttype = CT_SIZE;
          u.cs.s = rvs;
        } break;
        case CT_ALPHABET: {
          bool char_ctx = u.op.a->u.cs.a.char_context;
          if (u.op.b->u.cs.a.char_context!=char_ctx) FATAL_ERROR("StringSubtypeTreeElement::evaluate()");
          if (char_ctx || (elementtype==ET_INTERSECTION)) {
            RangeListConstraint<CHARLIMITTYPE>* rva = new RangeListConstraint<CHARLIMITTYPE>;
            switch (elementtype) {
            case ET_INTERSECTION:
              *rva = *(u.op.a->u.cs.a.c) * *(u.op.b->u.cs.a.c);
              break;
            case ET_UNION:
              *rva = *(u.op.a->u.cs.a.c) + *(u.op.b->u.cs.a.c);
              break;
            case ET_EXCEPT:
              *rva = *(u.op.a->u.cs.a.c) - *(u.op.b->u.cs.a.c);
              break;
            default:
              FATAL_ERROR("StringSubtypeTreeElement::evaluate()");
            }
            clean_up();
            elementtype = ET_CONSTRAINT;
            u.cs.constrainttype = CT_ALPHABET;
            u.cs.a.c = rva;
            u.cs.a.char_context = char_ctx;
          }
        } break;
        case CT_VALUES: {
          StringValueConstraint<STRINGTYPE>* rvv = new StringValueConstraint<STRINGTYPE>;
          switch (elementtype) {
          case ET_INTERSECTION:
            *rvv = *(u.op.a->u.cs.v) * *(u.op.b->u.cs.v);
            break;
          case ET_UNION:
            *rvv = *(u.op.a->u.cs.v) + *(u.op.b->u.cs.v);
            break;
          case ET_EXCEPT:
            *rvv = *(u.op.a->u.cs.v) - *(u.op.b->u.cs.v);
            break;
          default:
            FATAL_ERROR("StringSubtypeTreeElement::evaluate()");
          }
          clean_up();
          elementtype = ET_CONSTRAINT;
          u.cs.constrainttype = CT_VALUES;
          u.cs.v = rvv;
        } break;
        case CT_PATTERN:
          return; // OH SHI-
        default:
          FATAL_ERROR("StringSubtypeTreeElement::evaluate()");
        }
      }
      break;
    default:
      FATAL_ERROR("StringSubtypeTreeElement::evaluate()");
    }
  }
  simplify();
}

template <class STRINGTYPE, class CHARLIMITTYPE>
string StringSubtypeTreeElement<STRINGTYPE,CHARLIMITTYPE>::to_string() const
{
  string ret_val;
  bool print_operation = false;
  string op_str;
  switch (elementtype) {
  case ET_NONE:
    break;
  case ET_ALL:
    ret_val += "ALL";
    break;
  case ET_CONSTRAINT:
    switch (u.cs.constrainttype) {
    case CT_SIZE:
      ret_val += "length";
      ret_val += u.cs.s->to_string();
      break;
    case CT_ALPHABET:
      ret_val += u.cs.a.char_context ? "range" : "from";
      ret_val += u.cs.a.c->to_string();
      break;
    case CT_VALUES:
      ret_val += u.cs.v->to_string();
      break;
    case CT_PATTERN:
      ret_val += u.cs.p->to_string();
      break;
    default:
      FATAL_ERROR("StringSubtypeTreeElement::to_string()");
    }
    break;
  case ET_INTERSECTION:
    op_str = "intersection";
    print_operation = true;
    break;
  case ET_UNION:
    op_str = "union";
    print_operation = true;
    break;
  case ET_EXCEPT:
    op_str = "except";
    print_operation = true;
    break;
  default:
    FATAL_ERROR("StringSubtypeTreeElement::to_string()");
  }
  if (print_operation) {
    ret_val += '(';
    ret_val += u.op.a->to_string();
    ret_val += ' ';
    ret_val += op_str;
    ret_val += ' ';
    ret_val += u.op.b->to_string();
    ret_val += ')';
  }
  return ret_val;
}

typedef StringSubtypeTreeElement<string,char_limit_t> CharstringSubtypeTreeElement;
typedef StringSubtypeTreeElement<ustring,universal_char_limit_t> UniversalCharstringSubtypeTreeElement;

////////////////////////////////////////////////////////////////////////////////
// constraint classes for structured types

// used by ValueListConstraint and RecofConstraint classes
// only operator==() is used, since most value types do not have operator<()
// when used in RecofConstraint it will use Value::get_nof_comps()
class ValueList
{
private:
  vector<Value> values; // values are not owned
  void clean_up();
  void copy_content(const ValueList& other);
public:
  ValueList() : values() {} // empty set
  ValueList(Value* v) : values()  { values.add(v); }

  ValueList(const ValueList& other) : values()  { copy_content(other); }
  ValueList& operator=(const ValueList& other) { clean_up(); copy_content(other); return *this; }
  ~ValueList() { clean_up(); }

  tribool is_empty() const { return TRIBOOL(values.size()==0); }
  tribool is_full() const { return TUNKNOWN; } // it's unknown how many possible values we have
  tribool is_equal(const ValueList& other) const;
  bool is_element(Value* v) const;

  // remove all elements whose size is (not) element of the given length set
  // works only if the type of values is correct (the get_nof_comps() doesn't end in FATAL_ERROR)
  void remove(const SizeRangeListConstraint& size_constraint, bool if_element);

  ValueList set_operation(const ValueList& other, bool is_union) const;

  ValueList operator+(const ValueList& other) const { return set_operation(other, true); } // union
  ValueList operator*(const ValueList& other) const { return set_operation(other, false); } // intersection
  ValueList operator-(const ValueList& other) const; // except

  tribool is_subset(const ValueList& other) const { return (*this-other).is_empty(); }

  string to_string() const;
};

// can be a ValueList or (ALL-ValueList), used for subtypes of structured types, enumerated and objid
class ValueListConstraint
{
private:
  bool complemented;
  ValueList values;
public:
  ValueListConstraint(): complemented(false), values() {} // empty set
  ValueListConstraint(Value* v): complemented(false), values(v) {} // single element set
  ValueListConstraint(bool p_complemented): complemented(p_complemented), values() {} // empty of full set

  tribool is_empty() const;
  tribool is_full() const;
  tribool is_equal(const ValueListConstraint& other) const;
  bool is_element(Value* v) const;

  ValueListConstraint operator+(const ValueListConstraint& other) const; // union
  ValueListConstraint operator*(const ValueListConstraint& other) const; // intersection
  ValueListConstraint operator~() const; // complement

  inline tribool is_subset(const ValueListConstraint& other) const
    { return (*this*~other).is_empty(); }
  inline ValueListConstraint operator-(const ValueListConstraint& other) const
    { return ( *this * ~other ); } // except

  string to_string() const;
};

// length and value list constraints for record/set of types
class RecofConstraint
{
private:
  SizeRangeListConstraint size_constraint;
  ValueList has_values, not_values;
public:
  // constructors
  RecofConstraint(): size_constraint(), has_values(), not_values() {} // empty set
  RecofConstraint(Value* v): size_constraint(), has_values(v), not_values() {} // single value set
  RecofConstraint(const size_limit_t& sl): size_constraint(sl), has_values(), not_values() {} // single size value set
  RecofConstraint(const size_limit_t& rl_begin, const size_limit_t& rl_end)
  : size_constraint(rl_begin,rl_end), has_values(), not_values() {} // size range set
  RecofConstraint(const SizeRangeListConstraint& p_size_constraint)
  : size_constraint(p_size_constraint), has_values(), not_values() {}

  tribool is_empty() const;
  tribool is_full() const;
  tribool is_equal(const RecofConstraint& other) const;
  bool is_element(Value* v) const;

  RecofConstraint set_operation(const RecofConstraint& other, bool is_union) const;

  inline RecofConstraint operator+(const RecofConstraint& other) const { return set_operation(other, true); } // union
  inline RecofConstraint operator*(const RecofConstraint& other) const { return set_operation(other, false); } // intersection
  RecofConstraint operator~() const; // complement

  inline tribool is_subset(const RecofConstraint& other) const { return (*this*~other).is_empty(); }
  inline RecofConstraint operator-(const RecofConstraint& other) const { return ( *this * ~other ); } // except

  tribool get_size_limit(bool is_upper, size_limit_t& limit) const;

  string to_string() const;
};


}// namespace Common

#endif // #ifndef _Subtypestuff_HH
