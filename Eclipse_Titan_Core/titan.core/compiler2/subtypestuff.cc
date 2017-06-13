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
 *
 ******************************************************************************/
#include "subtypestuff.hh"
#include "../common/dbgnew.hh"
#include "Identifier.hh"
#include "Value.hh"
#include "Setting.hh"
#include "Type.hh"
#include "CompilerError.hh"
#include "Valuestuff.hh"
#include "ttcn3/TtcnTemplate.hh"
#include "ttcn3/Templatestuff.hh"
#include "ttcn3/PatternString.hh"
#include "PredefFunc.hh"

#include <limits.h>

namespace Common {


tribool operator||(tribool a, tribool b)
{
  static tribool truth_table[3][3] = { {TFALSE, TUNKNOWN, TTRUE}, {TUNKNOWN, TUNKNOWN, TTRUE}, {TTRUE, TTRUE, TTRUE} };
  return truth_table[a][b];
}

tribool operator&&(tribool a, tribool b)
{
  static tribool truth_table[3][3] = { {TFALSE, TFALSE, TFALSE}, {TFALSE, TUNKNOWN, TUNKNOWN}, {TFALSE, TUNKNOWN, TTRUE} };
  return truth_table[a][b];
}

tribool operator!(tribool tb)
{
  static tribool truth_table[3] = { TTRUE, TUNKNOWN, TFALSE };
  return truth_table[tb];
}

tribool TRIBOOL(bool b) { return ( b ? TTRUE : TFALSE ); }

string to_string(const tribool& tb)
{
  switch (tb) {
  case TFALSE:
    return string("false");
  case TTRUE:
    return string("true");
  case TUNKNOWN:
    return string("unknown");
  default:
    FATAL_ERROR("print(tribool)");
  }
  return string();
}

////////////////////////////////////////////////////////////////////////////////

const int_limit_t int_limit_t::minimum(int_limit_t::MINUS_INFINITY);

const int_limit_t int_limit_t::maximum(int_limit_t::PLUS_INFINITY);

int_limit_t::int_limit_t(int_limit_type_t p_type):
  type(p_type)
{
  switch (p_type) {
  case MINUS_INFINITY:
  case PLUS_INFINITY:
    break;
  default:
    FATAL_ERROR("int_limit_t::int_limit_t(int_limit_type_t)");
  }
}

bool int_limit_t::operator<(const int_limit_t& right) const
{
  switch (type) {
  case MINUS_INFINITY:
    return (right.type!=MINUS_INFINITY);
  case NUMBER:
    return ( (right.type==PLUS_INFINITY) || ( (right.type==NUMBER) && (value<right.value) ) );
  case PLUS_INFINITY:
    return false;
  default:
    FATAL_ERROR("int_limit_t::operator<()");
  }
}

bool int_limit_t::operator==(const int_limit_t& right) const
{
  if (type==NUMBER) return ( (right.type==NUMBER) && (value==right.value) );
  else return (type==right.type);
}

bool int_limit_t::is_adjacent(const int_limit_t& other) const
{
  return ( (type==NUMBER) && (other.type==NUMBER) && ((value+1)==other.value) );
}

int_val_t int_limit_t::get_value() const
{
  if (type!=NUMBER) FATAL_ERROR("int_limit_t::get_value()");
  return value;
}

int_limit_t int_limit_t::next() const
{
  return ( (type==NUMBER) ? int_limit_t(value+1) : *this );
}

int_limit_t int_limit_t::previous() const
{
  return ( (type==NUMBER) ? int_limit_t(value-1) : *this );
}

void int_limit_t::check_single_value() const
{
  if (type!=NUMBER) FATAL_ERROR("int_limit_t::check_single_value()");
}

void int_limit_t::check_interval_start() const
{
  if (type==PLUS_INFINITY) FATAL_ERROR("int_limit_t::check_interval_start()");
}

void int_limit_t::check_interval_end() const
{
  if (type==MINUS_INFINITY) FATAL_ERROR("int_limit_t::check_interval_end()");
}

string int_limit_t::to_string() const
{
  switch (type) {
  case int_limit_t::MINUS_INFINITY:
    return string("-infinity");
  case int_limit_t::NUMBER:
    return value.t_str();
  case int_limit_t::PLUS_INFINITY:
    return string("infinity");
  default:
    FATAL_ERROR("int_limit_t::print()");
  }
  return string();
}

////////////////////////////////////////////////////////////////////////////////

const size_limit_t size_limit_t::minimum(0);

const size_limit_t size_limit_t::maximum(INFINITE_SIZE);

bool size_limit_t::operator<(const size_limit_t& right) const
{
  return ( !infinity && ( right.infinity || (size<right.size) ) );
}

bool size_limit_t::operator==(const size_limit_t& right) const
{
  return ( (infinity==right.infinity) && (infinity || (size==right.size)) );
}

bool size_limit_t::is_adjacent(const size_limit_t& other) const
{
  return ( !infinity && !other.infinity && (size+1==other.size) );
}

size_t size_limit_t::get_size() const
{
  if (infinity) FATAL_ERROR("size_limit_t::get_size()");
  return size;
}

size_limit_t size_limit_t::next() const
{
  return ( infinity ? *this : size_limit_t(size+1) );
}

size_limit_t size_limit_t::previous() const
{
  if (size==0) FATAL_ERROR("size_limit_t::previous()");
  return ( infinity ? *this : size_limit_t(size-1) );
}

void size_limit_t::check_single_value() const
{
  if (infinity) FATAL_ERROR("size_limit_t::check_single_value()");
}

void size_limit_t::check_interval_start() const
{
  if (infinity) FATAL_ERROR("size_limit_t::check_interval_start()");
}

string size_limit_t::to_string() const
{
  if (infinity) return string("infinity");
  return Int2string(static_cast<Int>(size));
}

int_limit_t size_limit_t::to_int_limit() const
{
  if (infinity) return int_limit_t(int_limit_t::PLUS_INFINITY);
  return int_limit_t(int_val_t(static_cast<Int>(size))); // FIXME: size_t -> Int
}

////////////////////////////////////////////////////////////////////////////////

const short int char_limit_t::max_char = 127;

const char_limit_t char_limit_t::minimum(0);

const char_limit_t char_limit_t::maximum(max_char);

bool char_limit_t::is_valid_value(short int p_chr)
{
  return ( (p_chr>=0) && (p_chr<=max_char) );
}

char_limit_t::char_limit_t(short int p_chr): chr(p_chr)
{
  if ( (chr<0) || (chr>max_char) ) FATAL_ERROR("char_limit_t::char_limit_t()");
}

char_limit_t char_limit_t::next() const
{
  if (chr>=max_char) FATAL_ERROR("char_limit_t::next()");
  return char_limit_t(chr+1);
}

char_limit_t char_limit_t::previous() const
{
  if (chr<=0) FATAL_ERROR("char_limit_t::previous()");
  return char_limit_t(chr-1);
}

string char_limit_t::to_string() const
{
  return string(static_cast<char>(chr)).get_stringRepr();
}

////////////////////////////////////////////////////////////////////////////////

void universal_char_limit_t::check_value() const
{
  if (code_point>max_code_point) FATAL_ERROR("universal_char_limit_t::check_value()");
}

unsigned int universal_char_limit_t::uchar2codepoint(const ustring::universal_char& uchr)
{
  return ( ((static_cast<unsigned int>(uchr.group))<<24) + ((static_cast<unsigned int>(uchr.plane))<<16) + ((static_cast<unsigned int>(uchr.row))<<8) + (static_cast<unsigned int>(uchr.cell)) );
}

ustring::universal_char universal_char_limit_t::codepoint2uchar(unsigned int cp)
{
  ustring::universal_char uchr;
  uchr.cell  = static_cast<unsigned char>(cp & 0xFF);
  uchr.row   = static_cast<unsigned char>((cp>>8) & 0xFF);
  uchr.plane = static_cast<unsigned char>((cp>>16) & 0xFF);
  uchr.group = static_cast<unsigned char>((cp>>24) & 0xFF);
  return uchr;
}

const unsigned int universal_char_limit_t::max_code_point = 0x7FFFFFFF;

const universal_char_limit_t universal_char_limit_t::minimum(0);

const universal_char_limit_t universal_char_limit_t::maximum(max_code_point);

bool universal_char_limit_t::is_valid_value(const ustring::universal_char& p_uchr)
{
  return (uchar2codepoint(p_uchr)<=max_code_point);
}

universal_char_limit_t universal_char_limit_t::next() const
{
  if (code_point>=max_code_point) FATAL_ERROR("universal_char_limit_t::next()");
  return universal_char_limit_t(code_point+1);
}

universal_char_limit_t universal_char_limit_t::previous() const
{
  if (code_point<=0) FATAL_ERROR("universal_char_limit_t::previous()");
  return universal_char_limit_t(code_point-1);
}

string universal_char_limit_t::to_string() const
{
  ustring::universal_char uc = codepoint2uchar(code_point);
  return ustring(1,&uc).get_stringRepr();
}

////////////////////////////////////////////////////////////////////////////////

const real_limit_t real_limit_t::minimum(make_ttcn3float(-REAL_INFINITY));

const real_limit_t real_limit_t::maximum(make_ttcn3float(REAL_INFINITY));

void real_limit_t::check_value() const
{
  if (value!=value) FATAL_ERROR("real_limit_t::check_value(): cannot be NaN");
  if ( (value==-REAL_INFINITY) && (type==LOWER) ) FATAL_ERROR("real_limit_t::check_value(): cannot be -infinity.lower");
  if ( (value==REAL_INFINITY) && (type==UPPER) ) FATAL_ERROR("real_limit_t::check_value(): cannot be infinity.upper");
}

bool real_limit_t::operator<(const real_limit_t& right) const
{
  ttcn3float v1,v2;
  v1 = value;
  v2 = right.value;
  return ( (v1<v2) || ((v1==v2)&&(type<right.type)) );
}

bool real_limit_t::operator==(const real_limit_t& right) const
{
  ttcn3float v1,v2;
  v1 = value;
  v2 = right.value;
  return ( (v1==v2) && (type==right.type) );
}

bool real_limit_t::is_adjacent(const real_limit_t& other) const
{
  ttcn3float v1,v2;
  v1 = value;
  v2 = other.value;
  return ( (v1==v2) && (((type==LOWER)&&(other.type==EXACT)) || ((type==EXACT)&&(other.type==UPPER))) );
}

real_limit_t real_limit_t::next() const
{
  switch (type) {
  case LOWER:
    return real_limit_t(value);
  case EXACT:
  case UPPER:
    return real_limit_t(value, UPPER);
  default:
    FATAL_ERROR("real_limit_t::next()");
  }
}

real_limit_t real_limit_t::previous() const
{
  switch (type) {
  case LOWER:
  case EXACT:
    return real_limit_t(value, LOWER);
  case UPPER:
    return real_limit_t(value);
  default:
    FATAL_ERROR("real_limit_t::previous()");
  }
}

void real_limit_t::check_single_value() const
{
  if (type!=EXACT) FATAL_ERROR("real_limit_t::check_single_value()");
}

void real_limit_t::check_interval_start() const
{
  if (type==LOWER) FATAL_ERROR("real_limit_t::check_interval_start()");
}

void real_limit_t::check_interval_end() const
{
  if (type==UPPER) FATAL_ERROR("real_limit_t::check_interval_end()");
}

string real_limit_t::to_string() const
{
  string ret_val;
  if (type!=EXACT) ret_val += '!';
  ret_val += Real2string(value);
  return ret_val;
}

////////////////////////////////////////////////////////////////////////////////

bool convert_int_to_size(const RangeListConstraint<int_limit_t>& int_range, RangeListConstraint<size_limit_t>& size_range)
{
  size_range = RangeListConstraint<size_limit_t>();
  size_range.intervals = int_range.intervals;
  size_range.values = dynamic_array<size_limit_t>(int_range.values.size());
  for (size_t i=0; i<int_range.values.size(); i++) {
    const int_limit_t& il = int_range.values[i];
    size_limit_t sl;
    switch (il.get_type()) {
    case int_limit_t::MINUS_INFINITY:
      size_range = RangeListConstraint<size_limit_t>(size_limit_t::minimum, size_limit_t::maximum);
      return false;
    case int_limit_t::NUMBER: {
      int_val_t number = il.get_value();
      if ((number<0) || !number.is_native_fit()) {
        size_range = RangeListConstraint<size_limit_t>(size_limit_t::minimum, size_limit_t::maximum);
        return false;
      }
      sl = size_limit_t(static_cast<size_t>(number.get_val()));
    } break;
    case int_limit_t::PLUS_INFINITY:
      sl = size_limit_t::maximum;
      break;
    default:
      FATAL_ERROR("RangeListConstraint::convert_int_to_size()");
    }
    size_range.values.add(sl);
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool RealRangeListConstraint::is_element(const ttcn3float& r) const
{
  if (r!=r) // this is a NaN value
    return has_nan;
  else
    return rlc.is_element(real_limit_t(r));
}

RealRangeListConstraint RealRangeListConstraint::set_operation(const RealRangeListConstraint& other, bool is_union) const
{
  RealRangeListConstraint ret_val;
  ret_val.rlc = rlc.set_operation(other.rlc, is_union);
  ret_val.has_nan = is_union ? (has_nan || other.has_nan) : (has_nan && other.has_nan);
  return ret_val;
}

RealRangeListConstraint RealRangeListConstraint::operator~() const
{
  RealRangeListConstraint ret_val;
  ret_val.rlc = ~rlc;
  ret_val.has_nan = !has_nan;
  return ret_val;
}

string RealRangeListConstraint::to_string() const
{
  string ret_val;
  ret_val += '(';
  ret_val += rlc.to_string(false);
  if (has_nan) {
    if (rlc.is_empty()!=TTRUE) ret_val += ',';
    ret_val += "NaN";
  }
  ret_val += ')';
  return ret_val;
}

bool RealRangeListConstraint::is_upper_limit_infinity () const
{
  return rlc.is_upper_limit_infinity();
}

bool RealRangeListConstraint::is_lower_limit_infinity () const
{
  return rlc.is_lower_limit_infinity();
}

////////////////////////////////////////////////////////////////////////////////

string BooleanListConstraint::to_string() const
{
  string ret_val;
  ret_val += '(';
  if (values&BC_FALSE) ret_val += "false";
  if (values==BC_ALL) ret_val += ',';
  if (values&BC_TRUE) ret_val += "true";
  ret_val += ')';
  return ret_val;
}

////////////////////////////////////////////////////////////////////////////////

string VerdicttypeListConstraint::to_string() const
{
  static const size_t verdict_count = 5;
  static const char* verdict_names[verdict_count] = { "none", "pass", "inconc", "fail", "error" };
  string ret_val;
  ret_val += '(';
  bool has_value = false;
  for (size_t i=VC_NONE,idx=0; (i<VC_ALL)&&(idx<verdict_count); i<<=1,idx++)
  {
    if (values&i) {
      if (has_value) ret_val += ',';
      ret_val += verdict_names[idx];
      has_value = true;
    }
  }
  ret_val += ')';
  return ret_val;
}

////////////////////////////////////////////////////////////////////////////////

tribool StringPatternConstraint::match(const string& str) const
{
  string patt = pattern->get_full_str();
  if (patt.size()==0) return TRIBOOL(str.size()==0);
  string *result = regexp(str, string('(')+patt+string(')'), 0, pattern->get_nocase());
  bool rv = (result->size()!=0);
  delete result;
  return TRIBOOL(rv);
}

string StringPatternConstraint::to_string() const
{
  string ret_val;
  ret_val += "pattern ";
  if (pattern->get_nocase()) {
    ret_val += "@nocase ";
  }
  ret_val += "(";
  ret_val += pattern->get_full_str();
  ret_val += ')';
  return ret_val;
}

////////////////////////////////////////////////////////////////////////////////
// constraint classes for structured types

void ValueList::clean_up()
{
  values.clear();
}

void ValueList::copy_content(const ValueList& other)
{
  for (size_t j=0; j<other.values.size(); j++) values.add(other.values[j]);
}

tribool ValueList::is_equal(const ValueList& other) const
{
  if (values.size()!=other.values.size()) return TFALSE;
  dynamic_array<bool> found(other.values.size()); // helper to skip unnecessary comparisons
  for (size_t j=0; j<other.values.size(); j++) found[j] = false;
  for (size_t i=0; i<values.size(); i++) {
    bool found_i = false;
    for (size_t j=0; j<other.values.size(); j++) {
      if (found[j]) continue; // skip already found equal elements
      if ( (values[i]==other.values[j]) || (*(values[i])==*(other.values[j])) ) {
        found[j] = true;
        found_i = true;
        break;
      }
    }
    if (!found_i) return TFALSE;
  }
  return TTRUE;
}

bool ValueList::is_element(Value* v) const
{
  for (size_t i=0; i<values.size(); i++) {
    if ( (values[i]==v) || (*(values[i])==*v) ) return true;
  }
  return false;
}

ValueList ValueList::set_operation(const ValueList& other, bool is_union) const
{
  ValueList ret_val;
  if (is_union) {
    for (size_t i=0; i<values.size(); i++) ret_val.values.add(values[i]);
    for (size_t i=0; i<other.values.size(); i++) {
      if (!is_element(other.values[i])) ret_val.values.add(other.values[i]);
    }
  } else {
    for (size_t i=0; i<values.size(); i++) {
      if (other.is_element(values[i])) ret_val.values.add(values[i]);
    }
  }
  return ret_val;
}

ValueList ValueList::operator-(const ValueList& other) const
{
  ValueList ret_val;
  for (size_t i=0; i<values.size(); i++) {
    if (!other.is_element(values[i])) ret_val.values.add(values[i]);
  }
  return ret_val;
}

void ValueList::remove(const SizeRangeListConstraint& size_constraint, bool if_element)
{
  for (size_t i=0; i<values.size(); i++) {
    if (size_constraint.is_element(size_limit_t(values[i]->get_nof_comps()))==if_element) {
      values.replace(i,1);
      i--;
    }
  }
}

string ValueList::to_string() const
{
  string ret_val;
  ret_val += '(';
  for (size_t i=0; i<values.size(); i++) {
    if (i>0) ret_val += ',';
    ret_val += values[i]->create_stringRepr();
  }
  ret_val += ')';
  return ret_val;
}

////////////////////////////////////////////////////////////////////////////////

tribool ValueListConstraint::is_empty() const
{
  return complemented ? values.is_full() : values.is_empty();
}


tribool ValueListConstraint::is_full() const
{
  return complemented ? values.is_empty() : values.is_full();
}


tribool ValueListConstraint::is_equal(const ValueListConstraint& other) const
{
  return (complemented==other.complemented) ? values.is_equal(other.values) : TUNKNOWN;
}


bool ValueListConstraint::is_element(Value* v) const
{
  return complemented ^ values.is_element(v);
}

ValueListConstraint ValueListConstraint::operator+(const ValueListConstraint& other) const
{
  ValueListConstraint ret_val;
  if (complemented) {
    if (other.complemented) {
      ret_val.complemented = true;
      ret_val.values = values * other.values;
    } else {
      ret_val.complemented = true;
      ret_val.values = values - other.values;
    }
  } else {
    if (other.complemented) {
      ret_val.complemented = true;
      ret_val.values = other.values - values;
    } else {
      ret_val.complemented = false;
      ret_val.values = values + other.values;
    }
  }
  return ret_val;
}

ValueListConstraint ValueListConstraint::operator*(const ValueListConstraint& other) const
{
  ValueListConstraint ret_val;
  if (complemented) {
    if (other.complemented) {
      ret_val.complemented = true;
      ret_val.values = values + other.values;
    } else {
      ret_val.complemented = false;
      ret_val.values = other.values - values;
    }
  } else {
    if (other.complemented) {
      ret_val.complemented = false;
      ret_val.values = values - other.values;
    } else {
      ret_val.complemented = false;
      ret_val.values = values * other.values;
    }
  }
  return ret_val;
}

ValueListConstraint ValueListConstraint::operator~() const
{
  ValueListConstraint ret_val;
  ret_val.complemented = !complemented;
  ret_val.values = values;
  return ret_val;
}

string ValueListConstraint::to_string() const
{
  if (complemented) {
    string ret_val;
    ret_val += "(ALL except ";
    ret_val += values.to_string();
    ret_val += ')';
    return ret_val;
  }
  return values.to_string();
}

////////////////////////////////////////////////////////////////////////////////

tribool RecofConstraint::is_empty() const
{
  if ( (size_constraint.is_empty()==TTRUE) && (has_values.is_empty()==TTRUE) ) return TTRUE;
  if (has_values.is_empty()==TFALSE) return TFALSE;
  if (not_values.is_empty()==TTRUE) return TFALSE;
  return TUNKNOWN; // the set of not_values may possibly cancel the size constraint set
}

tribool RecofConstraint::is_full() const
{
  if ( (size_constraint.is_full()==TTRUE) && (not_values.is_empty()==TTRUE) ) return TTRUE;
  if (not_values.is_empty()==TFALSE) return TFALSE;
  return TUNKNOWN;
}

tribool RecofConstraint::is_equal(const RecofConstraint& other) const
{
  if ( (size_constraint.is_equal(other.size_constraint)==TTRUE) &&
       (has_values.is_equal(other.has_values)==TTRUE) && (not_values.is_equal(other.not_values)==TTRUE) )
    return TTRUE;
  return TUNKNOWN; // unknown because there's no canonical form
}

bool RecofConstraint::is_element(Value* v) const
{
  if (size_constraint.is_element(size_limit_t(v->get_nof_comps()))) return !not_values.is_element(v);
  return has_values.is_element(v);
}

// representation of two sets: [Si+Vi-Ni] where Si=size_constraint, Vi=has_values, Ni=not_values
// UNION:        [S1+V1-N1] + [S2+V2-N2] = ... = [(S1+S2)+(V1+V2)-((~S1*N2)+(N1*~S2)+(N1*N2))]
// INTERSECTION: [S1+V1-N1] * [S2+V2-N2] = ... = [(S1*S2)+((S1*V2-N1)+(S2*V1-N2)+(V1*V2))-(N1+N2)]
RecofConstraint RecofConstraint::set_operation(const RecofConstraint& other, bool is_union) const
{
  RecofConstraint ret_val;
  ret_val.size_constraint = size_constraint.set_operation(other.size_constraint, is_union);
  if (is_union) {
    // V1+V2
    ret_val.has_values = has_values + other.has_values;
    // ~S1*N2
    ValueList vlc1 = other.not_values;
    vlc1.remove(size_constraint, true);
    // N1*~S2
    ValueList vlc2 = not_values;
    vlc2.remove(other.size_constraint, true);
    // ((~S1*N2)+(N1*~S2)+(N1*N2))
    ret_val.not_values = vlc1 + vlc2 + (not_values * other.not_values);
  } else { // intersection
    // S2*V1-N2
    ValueList vlc1 = has_values;
    vlc1.remove(other.size_constraint, false);
    vlc1 = vlc1 - other.not_values;
    // S1*V2-N1
    ValueList vlc2 = other.has_values;
    vlc2.remove(size_constraint, false);
    vlc2 = vlc2 - not_values;
    // (S1*V2-N1)+(S2*V1-N2)+(V1*V2)
    ret_val.has_values = (has_values * other.has_values) + vlc1 + vlc2;
    // union of not_values
    ret_val.not_values = not_values + other.not_values;
  }
  // drop the intersection, holes and points cancel each other
  ValueList vlc = ret_val.has_values * ret_val.not_values;
  ret_val.has_values = ret_val.has_values - vlc;
  ret_val.not_values = ret_val.not_values - vlc;
  // drop ret_val.has_values elements that are elements of the ret_val.size_constraint set
  ret_val.has_values.remove(ret_val.size_constraint, true);
  // drop ret_val.not_values elements that are not elements of the ret_val.size_constraint set
  ret_val.not_values.remove(ret_val.size_constraint, false);
  return ret_val;
}

RecofConstraint RecofConstraint::operator~() const
{
  RecofConstraint ret_val;
  ret_val.size_constraint = ~size_constraint;
  ret_val.has_values = not_values;
  ret_val.not_values = has_values;
  return ret_val;
}

tribool RecofConstraint::get_size_limit(bool is_upper, size_limit_t& limit) const
{
  if (size_constraint.is_empty()==TTRUE) return TFALSE;
  limit = is_upper ? size_constraint.get_maximal() : size_constraint.get_minimal();
  return TTRUE;
}

string RecofConstraint::to_string() const
{
  string ret_val;
  if (has_values.is_empty()!=TTRUE) ret_val += has_values.to_string();
  if (size_constraint.is_empty()!=TTRUE) {
    if (has_values.is_empty()!=TTRUE) ret_val += " union ";
    ret_val += "length";
    ret_val += size_constraint.to_string();
  }
  // except not_values
  if (not_values.is_empty()!=TTRUE) {
    ret_val += " except ";
    ret_val += not_values.to_string();
  }
  return ret_val;
}


} // namespace Common
