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
 *   Baranyi, Botond
 *   Cserveni, Akos
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szabo, Bence Janos
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "subtype.hh"
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
#include "Constraint.hh"
#include "../common/JSON_Tokenizer.hh"
#include "ttcn3/Ttcn2Json.hh"
#include "main.hh"

#include <limits.h>

namespace Common {

/**************************
class SubTypeParse
**************************/

SubTypeParse::SubTypeParse(Value *p_single)
: selection(STP_SINGLE)
{
  if (!p_single) FATAL_ERROR("SubTypeParse::SubTypeParse()");
  single = p_single;
}

SubTypeParse::SubTypeParse(Value *p_min, bool p_min_exclusive, Value *p_max, bool p_max_exclusive)
: selection(STP_RANGE)
{
  range.min = p_min;
  range.min_exclusive = p_min_exclusive;
  range.max = p_max;
  range.max_exclusive = p_max_exclusive;
}

SubTypeParse::SubTypeParse(Ttcn::LengthRestriction *p_length)
: selection(STP_LENGTH)
{
  if (!p_length) FATAL_ERROR("SubTypeParse::SubTypeParse()");
  length = p_length;
}

SubTypeParse::SubTypeParse(Ttcn::PatternString *p_pattern)
: selection(STP_PATTERN)
{
  if (!p_pattern) FATAL_ERROR("SubTypeParse::SubTypeParse()");
  pattern = p_pattern;
}

SubTypeParse::~SubTypeParse()
{
  switch (selection) {
  case STP_SINGLE:
    delete single;
    break;
  case STP_RANGE:
    delete range.min;
    delete range.max;
    break;
  case STP_LENGTH:
    delete length;
    break;
  case STP_PATTERN:
    delete pattern;
    break;
  default:
    FATAL_ERROR("SubTypeParse::~SubTypeParse()");
  }
}

Value *SubTypeParse::Single() const
{
  if (selection != STP_SINGLE) FATAL_ERROR("SubTypeParse::Single()");
  return single;
}

Value *SubTypeParse::Min() const
{
  if (selection != STP_RANGE) FATAL_ERROR("SubTypeParse::Min()");
  return range.min;
}

bool SubTypeParse::MinExclusive() const
{
  if (selection != STP_RANGE) FATAL_ERROR("SubTypeParse::MinExclusive()");
  return range.min_exclusive;
}

Value *SubTypeParse::Max() const
{
  if (selection != STP_RANGE) FATAL_ERROR("SubTypeParse::Max()");
  return range.max;
}

bool SubTypeParse::MaxExclusive() const
{
  if (selection != STP_RANGE) FATAL_ERROR("SubTypeParse::MaxExclusive()");
  return range.max_exclusive;
}

Ttcn::LengthRestriction *SubTypeParse::Length() const
{
  if (selection != STP_LENGTH) FATAL_ERROR("SubTypeParse::Length()");
  return length;
}

Ttcn::PatternString *SubTypeParse::Pattern() const
{
  if (selection != STP_PATTERN) FATAL_ERROR("SubTypeParse::Pattern()");
  return pattern;
}

/********************
class SubtypeConstraint
********************/

SubtypeConstraint::SubtypeConstraint(subtype_t st)
{
  subtype  = st;
  length_restriction = NULL;
  switch (subtype) {
  case ST_INTEGER:
    integer_st = NULL;
    break;
  case ST_FLOAT:
    float_st = NULL;
    break;
  case ST_BOOLEAN:
    boolean_st = NULL;
    break;
  case ST_VERDICTTYPE:
    verdict_st = NULL;
    break;
  case ST_BITSTRING:
    bitstring_st = NULL;
    break;
  case ST_HEXSTRING:
    hexstring_st = NULL;
    break;
  case ST_OCTETSTRING:
    octetstring_st = NULL;
    break;
  case ST_CHARSTRING:
    charstring_st = NULL;
    break;
  case ST_UNIVERSAL_CHARSTRING:
    universal_charstring_st = NULL;
    break;
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    value_st = NULL;
    break;
  case ST_RECORDOF:
  case ST_SETOF:
    recof_st = NULL;
    break;
  default:
    FATAL_ERROR("SubtypeConstraint::SubtypeConstraint()");
  }
}

void SubtypeConstraint::copy(const SubtypeConstraint* other)
{
  if ((other==NULL) || (other->subtype!=subtype)) FATAL_ERROR("SubtypeConstraint::copy()");
  switch (subtype) {
  case ST_INTEGER:
    delete integer_st;
    integer_st = other->integer_st ? new IntegerRangeListConstraint(*(other->integer_st)) : NULL;
    break;
  case ST_FLOAT:
    delete float_st;
    float_st = other->float_st ? new RealRangeListConstraint(*(other->float_st)) : NULL;
    break;
  case ST_BOOLEAN:
    delete boolean_st;
    boolean_st = other->boolean_st ? new BooleanListConstraint(*(other->boolean_st)) : NULL;
    break;
  case ST_VERDICTTYPE:
    delete verdict_st;
    verdict_st = other->verdict_st ? new VerdicttypeListConstraint(*(other->verdict_st)) : NULL;
    break;
  case ST_BITSTRING:
    delete bitstring_st;
    bitstring_st = other->bitstring_st ? new BitstringConstraint(*(other->bitstring_st)) : NULL;
    break;
  case ST_HEXSTRING:
    delete hexstring_st;
    hexstring_st = other->hexstring_st ? new HexstringConstraint(*(other->hexstring_st)) : NULL;
    break;
  case ST_OCTETSTRING:
    delete octetstring_st;
    octetstring_st = other->octetstring_st ? new OctetstringConstraint(*(other->octetstring_st)) : NULL;
    break;
  case ST_CHARSTRING:
    delete charstring_st;
    charstring_st = other->charstring_st ? new CharstringSubtypeTreeElement(*(other->charstring_st)) : NULL;
    break;
  case ST_UNIVERSAL_CHARSTRING:
    delete universal_charstring_st;
    universal_charstring_st = other->universal_charstring_st ? new UniversalCharstringSubtypeTreeElement(*(other->universal_charstring_st)) : NULL;
    break;
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    delete value_st;
    value_st = other->value_st ? new ValueListConstraint(*(other->value_st)) : NULL;
    break;
  case ST_RECORDOF:
  case ST_SETOF:
    delete recof_st;
    recof_st = other->recof_st ? new RecofConstraint(*(other->recof_st)) : NULL;
    break;
  default:
    FATAL_ERROR("SubtypeConstraint::copy()");
  }
  delete length_restriction;
  length_restriction = other->length_restriction ? new SizeRangeListConstraint(*(other->length_restriction)) : NULL;
}

// used by get_asn_type_constraint() to store singleton objects and delete them on program exit
struct AsnTypeConstraintSingleton
{
  SubtypeConstraint *printablestring_stc, *numericstring_stc, *bmpstring_stc;
  AsnTypeConstraintSingleton():
    printablestring_stc(NULL), numericstring_stc(NULL), bmpstring_stc(NULL) {}
  ~AsnTypeConstraintSingleton();
};

AsnTypeConstraintSingleton::~AsnTypeConstraintSingleton()
{
  delete printablestring_stc;
  delete numericstring_stc;
  delete bmpstring_stc;
}

SubtypeConstraint* SubtypeConstraint::get_asn_type_constraint(Type* type)
{
  static AsnTypeConstraintSingleton asn_tcs;
  static const char_limit_t zero('0'), nine('9'),
    bigA('A'), bigZ('Z'), smalla('a'), smallz('z');
  static const universal_char_limit_t uni_zero(0);
  static const universal_char_limit_t uni_ffff((1<<16)-1);
  static const CharRangeListConstraint numeric_string_char_range(
    CharRangeListConstraint(zero, nine) +
    CharRangeListConstraint(char_limit_t(' ')));
  static const CharRangeListConstraint printable_string_char_range(
    CharRangeListConstraint(bigA, bigZ) +
    CharRangeListConstraint(smalla, smallz) +
    CharRangeListConstraint(zero , nine) +
    CharRangeListConstraint(char_limit_t(' ')) +
    CharRangeListConstraint(char_limit_t('\'')) +
    CharRangeListConstraint(char_limit_t('(')) +
    CharRangeListConstraint(char_limit_t(')')) +
    CharRangeListConstraint(char_limit_t('+')) +
    CharRangeListConstraint(char_limit_t(',')) +
    CharRangeListConstraint(char_limit_t('-')) +
    CharRangeListConstraint(char_limit_t('.')) +
    CharRangeListConstraint(char_limit_t('/')) +
    CharRangeListConstraint(char_limit_t(':')) +
    CharRangeListConstraint(char_limit_t('=')) +
    CharRangeListConstraint(char_limit_t('?')));
  static const UniversalCharRangeListConstraint bmp_string_char_range(
    UniversalCharRangeListConstraint(uni_zero, uni_ffff));

  switch (type->get_typetype()) {
  case Type::T_TELETEXSTRING:
    // TODO: based on ITU-T Recommendation T.61
    return NULL;
  case Type::T_VIDEOTEXSTRING:
    // TODO: based on ITU-T Recommendation T.100 and T.101
    return NULL;
  case Type::T_NUMERICSTRING:
    if (asn_tcs.numericstring_stc==NULL) {
      asn_tcs.numericstring_stc = new SubtypeConstraint(ST_CHARSTRING);
      asn_tcs.numericstring_stc->charstring_st = new CharstringSubtypeTreeElement(numeric_string_char_range, false);
    }
    return asn_tcs.numericstring_stc;
  case Type::T_PRINTABLESTRING:
    if (asn_tcs.printablestring_stc==NULL) {
      asn_tcs.printablestring_stc = new SubtypeConstraint(ST_CHARSTRING);
      asn_tcs.printablestring_stc->charstring_st = new CharstringSubtypeTreeElement(printable_string_char_range, false);
    }
    return asn_tcs.printablestring_stc;
  case Type::T_BMPSTRING:
    if (asn_tcs.bmpstring_stc==NULL) {
      asn_tcs.bmpstring_stc = new SubtypeConstraint(ST_UNIVERSAL_CHARSTRING);
      asn_tcs.bmpstring_stc->universal_charstring_st = new UniversalCharstringSubtypeTreeElement(bmp_string_char_range, false);
    }
    return asn_tcs.bmpstring_stc;
  default:
    return NULL;
  }
}

SubtypeConstraint* SubtypeConstraint::create_from_asn_value(Type* type, Value* value)
{
  Value* v = value->get_value_refd_last();
  subtype_t st_t = type->get_subtype_type();
  if ( (st_t==ST_ERROR) || (v->get_valuetype()==Value::V_ERROR) ) return NULL;
  SubtypeConstraint* stc = new SubtypeConstraint(st_t);
  switch (v->get_valuetype()) {
  case Value::V_INT:
    if (st_t!=ST_INTEGER) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->integer_st = new IntegerRangeListConstraint(int_limit_t(*(v->get_val_Int())));
    break;
  case Value::V_REAL: {
    if (st_t!=ST_FLOAT) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    ttcn3float r = v->get_val_Real();
    if (r!=r) stc->float_st = new RealRangeListConstraint(true);
    else stc->float_st = new RealRangeListConstraint(real_limit_t(r));
  } break;
  case Value::V_BOOL:
    if (st_t!=ST_BOOLEAN) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->boolean_st = new BooleanListConstraint(v->get_val_bool());
    break;
  case Value::V_OID:
  case Value::V_ROID:
    if (v->has_oid_error()) goto invalid_value;
    if (st_t!=ST_OBJID) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->value_st = new ValueListConstraint(v);
    break;
  case Value::V_BSTR:
    if (st_t!=ST_BITSTRING) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->bitstring_st = new BitstringConstraint(v->get_val_str());
    break;
  case Value::V_HSTR:
    if (st_t!=ST_HEXSTRING) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->hexstring_st = new HexstringConstraint(v->get_val_str());
    break;
  case Value::V_OSTR:
    if (st_t!=ST_OCTETSTRING) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->octetstring_st = new OctetstringConstraint(v->get_val_str());
    break;
  case Value::V_CSTR:
    if (st_t!=ST_CHARSTRING) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->charstring_st = new CharstringSubtypeTreeElement(StringValueConstraint<string>(v->get_val_str()));
    break;
  case Value::V_ISO2022STR:
    if (st_t!=ST_CHARSTRING) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->charstring_st = new CharstringSubtypeTreeElement(StringValueConstraint<string>(v->get_val_iso2022str()));
    break;
  case Value::V_CHARSYMS:
  case Value::V_USTR:
    if (st_t!=ST_UNIVERSAL_CHARSTRING) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->universal_charstring_st = new UniversalCharstringSubtypeTreeElement(StringValueConstraint<ustring>(v->get_val_ustr()));
    break;
  case Value::V_ENUM:
  case Value::V_NULL: // FIXME: should go to ST_NULL
    if (st_t!=ST_ENUM) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->value_st = new ValueListConstraint(v);
    break;
  case Value::V_CHOICE:
  case Value::V_OPENTYPE: // FIXME?
    if (st_t!=ST_UNION) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->value_st = new ValueListConstraint(v);
    break;
  case Value::V_SEQ:
    if (st_t!=ST_RECORD) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->value_st = new ValueListConstraint(v);
    break;
  case Value::V_SET:
    if (st_t!=ST_SET) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->value_st = new ValueListConstraint(v);
    break;
  case Value::V_SEQOF:
    if (st_t!=ST_RECORDOF) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->recof_st = new RecofConstraint(v);
    break;
  case Value::V_SETOF:
    if (st_t!=ST_SETOF) FATAL_ERROR("SubtypeConstraint::create_from_asn_value()");
    stc->recof_st = new RecofConstraint(v);
    break;
  default:
    goto invalid_value;
  }
  return stc;
invalid_value:
  delete stc;
  return NULL;
}

SubtypeConstraint* SubtypeConstraint::create_from_asn_charvalues(Type* type, Value* value)
{
  Value* v = value->get_value_refd_last();
  subtype_t st_t = type->get_subtype_type();
  if ( (st_t==ST_ERROR) || (v->get_valuetype()==Value::V_ERROR) ) return NULL;
  SubtypeConstraint* stc = new SubtypeConstraint(st_t);
  switch (v->get_valuetype()) {
  case Value::V_CSTR:
  case Value::V_ISO2022STR: {
    if (st_t!=ST_CHARSTRING) FATAL_ERROR("SubtypeConstraint::create_from_asn_charvalues()");
    CharRangeListConstraint charvalues;
    string val_str = (v->get_valuetype()==Value::V_CSTR) ? v->get_val_str() : v->get_val_iso2022str();
    for (size_t i=0; i<val_str.size(); i++) {
      if (!char_limit_t::is_valid_value(val_str[i])) {
        value->error("Invalid char in string %s at index %lu",
          value->get_stringRepr().c_str(), (unsigned long)i);
        goto invalid_value;
      }
      charvalues = charvalues + CharRangeListConstraint(val_str[i]);
    }
    stc->charstring_st = new CharstringSubtypeTreeElement(charvalues, true);
  } break;
  case Value::V_CHARSYMS: {
  case Value::V_USTR:
    if (st_t!=ST_UNIVERSAL_CHARSTRING) FATAL_ERROR("SubtypeConstraint::create_from_asn_charvalues()");
    UniversalCharRangeListConstraint ucharvalues;
    ustring val_ustr = v->get_val_ustr();
    for (size_t i=0; i<val_ustr.size(); i++) {
      if (!universal_char_limit_t::is_valid_value(val_ustr[i])) {
        value->error("Invalid universal char in string %s at index %lu",
          value->get_stringRepr().c_str(), (unsigned long)i);
        goto invalid_value;
      }
      ucharvalues = ucharvalues + UniversalCharRangeListConstraint(val_ustr[i]);
    }
    stc->universal_charstring_st = new UniversalCharstringSubtypeTreeElement(ucharvalues, true);
  } break;
  default:
    // error was already reported
    goto invalid_value;
  }
  return stc;
invalid_value:
  delete stc;
  return NULL;
}

int_limit_t SubtypeConstraint::get_int_limit(bool is_upper, Location* loc)
{
  int_limit_t default_limit = is_upper ?
    int_limit_t::maximum :
    ((subtype==ST_INTEGER) ? int_limit_t::minimum : int_limit_t(int_val_t((Int)0)));
  switch (subtype) {
  case ST_INTEGER:
    if (integer_st) {
      if (integer_st->is_empty()==TTRUE) {
        loc->error("Cannot determine the value of %s: the parent subtype is an empty set.",
                   is_upper?"MAX":"MIN");
        return default_limit;
      } else {
        return is_upper ? integer_st->get_maximal() : integer_st->get_minimal();
      }
    }
    return default_limit;
  case ST_BITSTRING:
    if (bitstring_st) {
      size_limit_t sl;
      tribool tb = bitstring_st->get_size_limit(is_upper, sl);
      if (tb==TTRUE) return sl.to_int_limit();
      break;
    }
    return default_limit;
  case ST_HEXSTRING:
    if (hexstring_st) {
      size_limit_t sl;
      tribool tb = hexstring_st->get_size_limit(is_upper, sl);
      if (tb==TTRUE) return sl.to_int_limit();
      break;
    }
    return default_limit;
  case ST_OCTETSTRING:
    if (octetstring_st) {
      size_limit_t sl;
      tribool tb = octetstring_st->get_size_limit(is_upper, sl);
      if (tb==TTRUE) return sl.to_int_limit();
      break;
    }
    return default_limit;
  case ST_CHARSTRING:
    if (charstring_st) {
      size_limit_t sl;
      tribool tb = charstring_st->get_size_limit(is_upper, sl);
      switch (tb) {
      case TFALSE:
        loc->error("Cannot determine the value of %s: the parent subtype does "
          "not define a %simal size value", is_upper?"MAX":"MIN", is_upper?"max":"min");
        break;
      case TUNKNOWN:
        loc->warning("Cannot determine the value of %s from parent subtype %s",
          is_upper?"MAX":"MIN", to_string().c_str());
        break;
      case TTRUE:
        return sl.to_int_limit();
      default:
        FATAL_ERROR("SubtypeConstraint::get_int_limit()");
      }
    }
    return default_limit;
  case ST_UNIVERSAL_CHARSTRING:
    if (universal_charstring_st) {
      size_limit_t sl;
      tribool tb = universal_charstring_st->get_size_limit(is_upper, sl);
      switch (tb) {
      case TFALSE:
        loc->error("Cannot determine the value of %s: the parent subtype does "
          "not define a %simal size value", is_upper?"MAX":"MIN", is_upper?"max":"min");
        break;
      case TUNKNOWN:
        loc->warning("Cannot determine the value of %s from parent subtype %s",
          is_upper?"MAX":"MIN", to_string().c_str());
        break;
      case TTRUE:
        return sl.to_int_limit();
      default:
        FATAL_ERROR("SubtypeConstraint::get_int_limit()");
      }
    }
    return default_limit;
  case ST_RECORDOF:
  case ST_SETOF:
    if (recof_st) {
      size_limit_t sl;
      tribool tb = recof_st->get_size_limit(is_upper, sl);
      if (tb==TTRUE) return sl.to_int_limit();
      break;
    }
    return default_limit;
  default:
    FATAL_ERROR("SubtypeConstraint::get_int_limit()");
  }
  loc->error("Cannot determine the value of %s from parent subtype %s",
    is_upper?"MAX":"MIN", to_string().c_str());
  return default_limit;
}

SubtypeConstraint* SubtypeConstraint::create_from_asn_range(
  Value* vmin, bool min_exclusive, Value* vmax, bool max_exclusive,
  Location* loc, subtype_t st_t, SubtypeConstraint* parent_subtype)
{
  switch (st_t) {
  case SubtypeConstraint::ST_INTEGER: {
    if (((vmin!=NULL) && (vmin->get_valuetype()!=Value::V_INT)) ||
        ((vmax!=NULL) && (vmax->get_valuetype()!=Value::V_INT))) return NULL;

    int_limit_t min_limit;
    if (vmin) {
      min_limit = int_limit_t(*(vmin->get_val_Int()));
    } else { // MIN was used
      if (parent_subtype) {
        min_limit = parent_subtype->get_int_limit(false, loc);
      } else {
        min_limit = int_limit_t::minimum;
      }
    }

    if (min_exclusive) {
      if (min_limit==int_limit_t::minimum) {
        loc->error("invalid lower boundary, -infinity cannot be excluded from an INTEGER value range constraint");
        return NULL;
      } else {
        min_limit = min_limit.next();
      }
    }

    int_limit_t max_limit;
    if (vmax) {
      max_limit = int_limit_t(*(vmax->get_val_Int()));
    } else { // MAX was used
      if (parent_subtype) {
        max_limit = parent_subtype->get_int_limit(true, loc);
      } else {
        max_limit = int_limit_t::maximum;
      }
    }

    if (max_exclusive) {
      if (max_limit==int_limit_t::maximum) {
        loc->error("invalid upper boundary, infinity cannot be excluded from an INTEGER value range constraint");
        return NULL;
      } else {
        max_limit = max_limit.previous();
      }
    }
    if (max_limit<min_limit) {
      loc->error("lower boundary is bigger than upper boundary in INTEGER value range constraint");
      return NULL;
    }
    SubtypeConstraint* stc = new SubtypeConstraint(st_t);
    stc->integer_st = new IntegerRangeListConstraint(min_limit, max_limit);
    return stc;
  } break;
  case ST_FLOAT: {
    if (((vmin!=NULL) && (vmin->get_valuetype()!=Value::V_REAL)) ||
        ((vmax!=NULL) && (vmax->get_valuetype()!=Value::V_REAL))) return NULL;
    if ((vmin!=NULL) && (vmin->get_val_Real()!=vmin->get_val_Real())) {
      loc->error("lower boundary cannot be NOT-A-NUMBER in REAL value range constraint");
      return NULL;
    }
    if ((vmax!=NULL) && (vmax->get_val_Real()!=vmax->get_val_Real())) {
      loc->error("upper boundary cannot be NOT-A-NUMBER in REAL value range constraint");
      return NULL;
    }

    if (parent_subtype && (parent_subtype->subtype!=ST_FLOAT)) FATAL_ERROR("SubtypeConstraint::create_from_asn_range()");
    real_limit_t min_limit;
    if (vmin) {
      min_limit = real_limit_t(vmin->get_val_Real());
    } else { // MIN was used
      if (parent_subtype && parent_subtype->float_st) {
        if (parent_subtype->float_st->is_range_empty()==TTRUE) {
          loc->error("Cannot determine the value of MIN: the parent subtype has no range");
          min_limit = real_limit_t::minimum;
        } else {
          min_limit = parent_subtype->float_st->get_minimal();
        }
      } else {
        min_limit = real_limit_t::minimum;
      }
    }

    if (min_exclusive) {
      min_limit = min_limit.next();
    }

    real_limit_t max_limit;
    if (vmax) {
      max_limit = real_limit_t(vmax->get_val_Real());
    } else { // MAX was used
      if (parent_subtype && parent_subtype->float_st) {
        if (parent_subtype->float_st->is_range_empty()==TTRUE) {
          loc->error("Cannot determine the value of MAX: the parent subtype has no range");
          max_limit = real_limit_t::maximum;
        } else {
          max_limit = parent_subtype->float_st->get_maximal();
        }
      } else {
        max_limit = real_limit_t::maximum;
      }
    }

    if (max_exclusive) {
      max_limit = max_limit.previous();
    }
    if (max_limit<min_limit) {
      loc->error("lower boundary is bigger than upper boundary in REAL value range constraint");
      return NULL;
    }
    SubtypeConstraint* stc = new SubtypeConstraint(st_t);
    stc->float_st = new RealRangeListConstraint(min_limit, max_limit);
    return stc;
  } break;
  case ST_CHARSTRING: {
    if (((vmin!=NULL) && (vmin->get_valuetype()!=Value::V_CSTR)) ||
        ((vmax!=NULL) && (vmax->get_valuetype()!=Value::V_CSTR))) return NULL;
    if (vmin && (vmin->get_val_str().size()!=1)) {
      vmin->error("lower boundary of string value range constraint must be a single element string");
      return NULL;
    }
    if (vmax && (vmax->get_val_str().size()!=1)) {
      vmax->error("upper boundary of string value range constraint must be a single element string");
      return NULL;
    }
    if (vmin && !char_limit_t::is_valid_value(*vmin->get_val_str().c_str())) {
      vmin->error("lower boundary of string value range constraint is an invalid char");
      return NULL;
    }
    if (vmax && !char_limit_t::is_valid_value(*vmax->get_val_str().c_str())) {
      vmax->error("upper boundary of string value range constraint is an invalid char");
      return NULL;
    }

    if (parent_subtype && (parent_subtype->subtype!=ST_CHARSTRING)) FATAL_ERROR("SubtypeConstraint::create_from_asn_range()");

    char_limit_t min_limit;
    if (vmin) {
      min_limit = char_limit_t(*vmin->get_val_str().c_str());
    } else { // MIN was used
      if (parent_subtype && parent_subtype->charstring_st) {
        tribool tb = parent_subtype->charstring_st->get_alphabet_limit(false, min_limit);
        switch (tb) {
        case TFALSE:
          loc->error("Cannot determine the value of MIN: the parent subtype does not define a minimal char value");
          min_limit = char_limit_t::minimum;
          break;
        case TUNKNOWN:
          loc->warning("Cannot determine the value of MIN, using the minimal char value of the type");
          min_limit = char_limit_t::minimum;
          break;
        case TTRUE:
          // min_limit was set to the correct value
          break;
        default:
          FATAL_ERROR("SubtypeConstraint::create_from_asn_range()");
        }
      } else {
        min_limit = char_limit_t::minimum;
      }
    }

    if (min_exclusive) {
      if (min_limit==char_limit_t::maximum) {
        loc->error("exclusive lower boundary is not a legal character");
        return NULL;
      }
      min_limit = min_limit.next();
    }

    char_limit_t max_limit;
    if (vmax) {
      max_limit = char_limit_t(*vmax->get_val_str().c_str());
    } else { // MAX was used
      if (parent_subtype && parent_subtype->charstring_st) {
        tribool tb = parent_subtype->charstring_st->get_alphabet_limit(true, max_limit);
        switch (tb) {
        case TFALSE:
          loc->error("Cannot determine the value of MAX: the parent subtype does not define a maximal char value");
          max_limit = char_limit_t::maximum;
          break;
        case TUNKNOWN:
          loc->warning("Cannot determine the value of MAX, using the maximal char value of the type");
          max_limit = char_limit_t::maximum;
          break;
        case TTRUE:
          // max_limit was set to the correct value
          break;
        default:
          FATAL_ERROR("SubtypeConstraint::create_from_asn_range()");
        }
      } else {
        max_limit = char_limit_t::maximum;
      }
    }

    if (max_exclusive) {
      if (max_limit==char_limit_t::minimum) {
        loc->error("exclusive upper boundary is not a legal character");
        return NULL;
      }
      max_limit = max_limit.previous();
    }
    if (max_limit<min_limit) {
      loc->error("lower boundary is bigger than upper boundary in string value range constraint");
      return NULL;
    }
    SubtypeConstraint* stc = new SubtypeConstraint(st_t);
    stc->charstring_st = new CharstringSubtypeTreeElement(CharRangeListConstraint(min_limit,max_limit), true);
    return stc;
  } break;
  case ST_UNIVERSAL_CHARSTRING: {
    if (((vmin!=NULL) && (vmin->get_valuetype()!=Value::V_USTR)) ||
        ((vmax!=NULL) && (vmax->get_valuetype()!=Value::V_USTR))) return NULL;
    if (vmin && (vmin->get_val_ustr().size()!=1)) {
      vmin->error("lower boundary of string value range constraint must be a single element string");
      return NULL;
    }
    if (vmax && (vmax->get_val_ustr().size()!=1)) {
      vmax->error("upper boundary of string value range constraint must be a single element string");
      return NULL;
    }
    if (vmin && !universal_char_limit_t::is_valid_value(*vmin->get_val_ustr().u_str())) {
      vmin->error("lower boundary of string value range constraint is an invalid universal char");
      return NULL;
    }
    if (vmax && !universal_char_limit_t::is_valid_value(*vmax->get_val_ustr().u_str())) {
      vmax->error("upper boundary of string value range constraint is an invalid universal char");
      return NULL;
    }

    if (parent_subtype && (parent_subtype->subtype!=ST_UNIVERSAL_CHARSTRING)) FATAL_ERROR("SubtypeConstraint::create_from_asn_range()");
    universal_char_limit_t min_limit;
    if (vmin) {
      min_limit = universal_char_limit_t(*vmin->get_val_ustr().u_str());
    } else { // MIN was used
      if (parent_subtype && parent_subtype->universal_charstring_st) {
        tribool tb = parent_subtype->universal_charstring_st->get_alphabet_limit(false, min_limit);
        switch (tb) {
        case TFALSE:
          loc->error("Cannot determine the value of MIN: the parent subtype does not define a minimal char value");
          min_limit = universal_char_limit_t::minimum;
          break;
        case TUNKNOWN:
          loc->warning("Cannot determine the value of MIN, using the minimal char value of the type");
          min_limit = universal_char_limit_t::minimum;
          break;
        case TTRUE:
          // min_limit was set to the correct value
          break;
        default:
          FATAL_ERROR("SubtypeConstraint::create_from_asn_range()");
        }
      } else {
        min_limit = universal_char_limit_t::minimum;
      }
    }

    if (min_exclusive) {
      if (min_limit==universal_char_limit_t::maximum) {
        loc->error("exclusive lower boundary is not a legal character");
        return NULL;
      }
      min_limit = min_limit.next();
    }

    universal_char_limit_t max_limit;
    if (vmax) {
      max_limit = universal_char_limit_t(*vmax->get_val_ustr().u_str());
    } else { // MAX was used
      if (parent_subtype && parent_subtype->universal_charstring_st) {
        tribool tb = parent_subtype->universal_charstring_st->get_alphabet_limit(true, max_limit);
        switch (tb) {
        case TFALSE:
          loc->error("Cannot determine the value of MAX: the parent subtype does not define a maximal char value");
          max_limit = universal_char_limit_t::maximum;
          break;
        case TUNKNOWN:
          loc->warning("Cannot determine the value of MAX, using the maximal char value of the type");
          max_limit = universal_char_limit_t::maximum;
          break;
        case TTRUE:
          // max_limit was set to the correct value
          break;
        default:
          FATAL_ERROR("SubtypeConstraint::create_from_asn_range()");
        }
      } else {
        max_limit = universal_char_limit_t::maximum;
      }
    }

    if (max_exclusive) {
      if (max_limit==universal_char_limit_t::minimum) {
        loc->error("exclusive upper boundary is not a legal character");
        return NULL;
      }
      max_limit = max_limit.previous();
    }
    if (max_limit<min_limit) {
      loc->error("lower boundary is bigger than upper boundary in string value range constraint");
      return NULL;
    }
    SubtypeConstraint* stc = new SubtypeConstraint(st_t);
    stc->universal_charstring_st = new UniversalCharstringSubtypeTreeElement(UniversalCharRangeListConstraint(min_limit,max_limit), true);
    return stc;
  } break;
  default:
    FATAL_ERROR("SubtypeConstraint::create_from_asn_range()");
  }
  return NULL;
}

SubtypeConstraint* SubtypeConstraint::create_from_contained_subtype(SubtypeConstraint* contained_stc, bool char_context, Location* loc)
{
  if (contained_stc==NULL) return NULL;
  SubtypeConstraint* rv_stc = NULL;
  if (char_context) {
    switch (contained_stc->get_subtypetype()) {
    case ST_CHARSTRING:
      if (contained_stc->charstring_st==NULL) {
        rv_stc = new SubtypeConstraint(contained_stc->get_subtypetype()); // full set
      } else {
        if (contained_stc->charstring_st->is_valid_range()) {
          rv_stc = new SubtypeConstraint(contained_stc->get_subtypetype());
          rv_stc->copy(contained_stc);
          rv_stc->charstring_st->set_char_context(true);
        } else {
          loc->error("The type of the contained subtype constraint cannot be used in a permitted alphabet constraint");
        }
      }
      break;
    case ST_UNIVERSAL_CHARSTRING:
      if (contained_stc->universal_charstring_st==NULL) {
        rv_stc = new SubtypeConstraint(contained_stc->get_subtypetype()); // full set
      } else {
        if (contained_stc->universal_charstring_st->is_valid_range()) {
          rv_stc = new SubtypeConstraint(contained_stc->get_subtypetype());
          rv_stc->copy(contained_stc);
          rv_stc->universal_charstring_st->set_char_context(true);
        } else {
          loc->error("The type of the contained subtype constraint cannot be used in a permitted alphabet constraint");
        }
      }
      break;
    default:
      // error was already reported
      break;
    }
  } else {
    rv_stc = new SubtypeConstraint(contained_stc->get_subtypetype());
    rv_stc->copy(contained_stc);
  }
  return rv_stc;
}

SubtypeConstraint* SubtypeConstraint::create_asn_size_constraint(
  SubtypeConstraint* integer_stc, bool char_context, Type* type, Location* loc)
{
  if (integer_stc==NULL) return NULL;
  // convert IntegerRangeListConstraint to SizeRangeListConstraint
  if (integer_stc->subtype!=ST_INTEGER) FATAL_ERROR("SubtypeConstraint::create_asn_size_constraint()");
  SizeRangeListConstraint size_constraint(size_limit_t::minimum, size_limit_t::maximum);
  if (integer_stc->integer_st) {
    static const int_val_t zero((Int)0);
    static const int_limit_t ilt0(zero);
    IntegerRangeListConstraint valid_range(ilt0, int_limit_t::maximum);
    if (integer_stc->integer_st->is_subset(valid_range)==TFALSE) {
      loc->error("Range %s is not a valid range for a size constraint", integer_stc->to_string().c_str());
    } else {
      bool success = convert_int_to_size(*(integer_stc->integer_st), size_constraint);
      if (!success) {
        loc->error("One or more INTEGER values of range %s are too large to be used in a size constraint", integer_stc->to_string().c_str());
      }
    }
  }
  subtype_t st_t = type->get_subtype_type();
  if (st_t==ST_ERROR) return NULL;
  SubtypeConstraint* stc = new SubtypeConstraint(st_t);

  if (!char_context) {
    stc->length_restriction = new SizeRangeListConstraint(size_constraint); // FIXME? : is this Ok if not a top level constraint?
  }

  switch (st_t) {
  case ST_BITSTRING:
      stc->bitstring_st = new BitstringConstraint(size_constraint);
      break;
    case ST_HEXSTRING:
      stc->hexstring_st = new HexstringConstraint(size_constraint);
      break;
    case ST_OCTETSTRING:
      stc->octetstring_st = new OctetstringConstraint(size_constraint);
      break;
    case ST_CHARSTRING:
      if (char_context) {
        if (size_constraint.is_equal(SizeRangeListConstraint(size_limit_t(1)))==TFALSE) {
          loc->error("Only SIZE(1) constraint can be used inside a permitted alphabet constraint");
          delete stc;
          return NULL;
        }
        // SIZE(1) is allowed in char context, it means ALL
      } else {
        stc->charstring_st = new CharstringSubtypeTreeElement(size_constraint);
      }
      break;
    case ST_UNIVERSAL_CHARSTRING:
      if (char_context) {
        if (size_constraint.is_equal(SizeRangeListConstraint(size_limit_t(1)))==TFALSE) {
          loc->error("Only SIZE(1) constraint can be used inside a permitted alphabet constraint");
          delete stc;
          return NULL;
        }
        // SIZE(1) is allowed in char context, it means ALL
      } else {
        stc->universal_charstring_st = new UniversalCharstringSubtypeTreeElement(size_constraint);
      }
      break;
    case ST_RECORDOF:
    case ST_SETOF:
      stc->recof_st = new RecofConstraint(size_constraint);
      break;
  default:
    loc->error("Size constraint is not allowed for type `%s'", type->get_typename().c_str());
    delete stc;
    return NULL;
  }
  return stc;
}

SubtypeConstraint* SubtypeConstraint::create_permitted_alphabet_constraint(
  SubtypeConstraint* stc, bool char_context, Type* type, Location* loc)
{
  if (char_context) {
    loc->error("Permitted alphabet constraint not allowed inside a permitted alphabet constraint");
    return NULL;
  }
  subtype_t st_t = type->get_subtype_type();
  switch (st_t) {
  case ST_CHARSTRING:
  case ST_UNIVERSAL_CHARSTRING: {
    if (stc==NULL) return NULL; // error was reported there
    if (st_t!=stc->get_subtypetype()) FATAL_ERROR("SubtypeConstraint::create_permitted_alphabet_constraint()");
    SubtypeConstraint* rv_stc = new SubtypeConstraint(st_t);
    if (st_t==ST_CHARSTRING) {
      if (stc->charstring_st) {
        rv_stc->charstring_st = new CharstringSubtypeTreeElement(*(stc->charstring_st));
        rv_stc->charstring_st->set_char_context(false);
      }
    } else {
      if (stc->universal_charstring_st) {
        rv_stc->universal_charstring_st = new UniversalCharstringSubtypeTreeElement(*(stc->universal_charstring_st));
        rv_stc->universal_charstring_st->set_char_context(false);
      }
    }
    return rv_stc;
  } break;
  case ST_ERROR:
    // error already reported
    break;
  default:
    loc->error("Permitted alphabet constraint is not allowed for type `%s'", type->get_typename().c_str());
    break;
  }
  return NULL;
}

void SubtypeConstraint::set_to_error()
{
  switch (subtype) {
  case ST_ERROR:
    break;
  case ST_INTEGER:
    delete integer_st;
    break;
  case ST_FLOAT:
    delete float_st;
    break;
  case ST_BOOLEAN:
    delete boolean_st;
    break;
  case ST_VERDICTTYPE:
    delete verdict_st;
    break;
  case ST_BITSTRING:
    delete bitstring_st;
    break;
  case ST_HEXSTRING:
    delete hexstring_st;
    break;
  case ST_OCTETSTRING:
    delete octetstring_st;
    break;
  case ST_CHARSTRING:
    delete charstring_st;
    break;
  case ST_UNIVERSAL_CHARSTRING:
    delete universal_charstring_st;
    break;
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    delete value_st;
    break;
  case ST_RECORDOF:
  case ST_SETOF:
    delete recof_st;
    break;
  default:
    FATAL_ERROR("SubtypeConstraint::set_to_error()");
  }
  subtype = ST_ERROR;
  delete length_restriction;
  length_restriction = NULL;
}

string SubtypeConstraint::to_string() const
{
  switch (subtype) {
  case ST_ERROR:
    return string("<error>");
  case ST_INTEGER:
    return (integer_st==NULL) ? string() : integer_st->to_string();
  case ST_FLOAT:
    return (float_st==NULL) ? string() : float_st->to_string();
  case ST_BOOLEAN:
    return (boolean_st==NULL) ? string() : boolean_st->to_string();
  case ST_VERDICTTYPE:
    return (verdict_st==NULL) ? string() : verdict_st->to_string();
  case ST_BITSTRING:
    return (bitstring_st==NULL) ? string() : bitstring_st->to_string();
  case ST_HEXSTRING:
    return (hexstring_st==NULL) ? string() : hexstring_st->to_string();
  case ST_OCTETSTRING:
    return (octetstring_st==NULL) ? string() : octetstring_st->to_string();
  case ST_CHARSTRING:
    return (charstring_st==NULL) ? string() : charstring_st->to_string();
  case ST_UNIVERSAL_CHARSTRING:
    return (universal_charstring_st==NULL) ? string() : universal_charstring_st->to_string();
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    return (value_st==NULL) ? string() : value_st->to_string();
  case ST_RECORDOF:
  case ST_SETOF:
    return (recof_st==NULL) ? string() : recof_st->to_string();
  default:
    FATAL_ERROR("SubtypeConstraint::to_string()");
  }
}

bool SubtypeConstraint::is_compatible(const SubtypeConstraint *p_st) const
{
  if (p_st==NULL) return true; // the other type has no subtype restriction
  if ( (subtype==ST_ERROR) || (p_st->subtype==ST_ERROR) ) return true;
  if (subtype!=p_st->subtype) FATAL_ERROR("SubtypeConstraint::is_compatible()");
  // if the resulting set.is_empty()==TUNKNOWN then remain silent
  switch (subtype) {
  case ST_INTEGER:
    if ((integer_st==NULL) || (p_st->integer_st==NULL)) return true;
    return ((*integer_st**(p_st->integer_st)).is_empty()!=TTRUE);
  case ST_FLOAT:
    if ((float_st==NULL) || (p_st->float_st==NULL)) return true;
    return ((*float_st**(p_st->float_st)).is_empty()!=TTRUE);
  case ST_BOOLEAN:
    if ((boolean_st==NULL) || (p_st->boolean_st==NULL)) return true;
    return ((*boolean_st**(p_st->boolean_st)).is_empty()!=TTRUE);
  case ST_VERDICTTYPE:
    if ((verdict_st==NULL) || (p_st->verdict_st==NULL)) return true;
    return ((*verdict_st**(p_st->verdict_st)).is_empty()!=TTRUE);
  case ST_BITSTRING:
    if ((bitstring_st==NULL) || (p_st->bitstring_st==NULL)) return true;
    return ((*bitstring_st**(p_st->bitstring_st)).is_empty()!=TTRUE);
  case ST_HEXSTRING:
    if ((hexstring_st==NULL) || (p_st->hexstring_st==NULL)) return true;
    return ((*hexstring_st**(p_st->hexstring_st)).is_empty()!=TTRUE);
  case ST_OCTETSTRING:
    if ((octetstring_st==NULL) || (p_st->octetstring_st==NULL)) return true;
    return ((*octetstring_st**(p_st->octetstring_st)).is_empty()!=TTRUE);
  case ST_CHARSTRING: {
    if ((charstring_st==NULL) || (p_st->charstring_st==NULL)) return true;
    CharstringSubtypeTreeElement* cc = new CharstringSubtypeTreeElement(
      CharstringSubtypeTreeElement::ET_INTERSECTION,
      new CharstringSubtypeTreeElement(*charstring_st),
      new CharstringSubtypeTreeElement(*(p_st->charstring_st))
    );
    bool rv = (cc->is_empty()!=TTRUE);
    delete cc;
    return rv;
  }
  case ST_UNIVERSAL_CHARSTRING: {
    if ((universal_charstring_st==NULL) || (p_st->universal_charstring_st==NULL)) return true;
    UniversalCharstringSubtypeTreeElement* ucc = new UniversalCharstringSubtypeTreeElement(
      UniversalCharstringSubtypeTreeElement::ET_INTERSECTION,
      new UniversalCharstringSubtypeTreeElement(*universal_charstring_st),
      new UniversalCharstringSubtypeTreeElement(*(p_st->universal_charstring_st))
    );
    bool rv = (ucc->is_empty()!=TTRUE);
    delete ucc;
    return rv;
  }
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    if ((value_st==NULL) || (p_st->value_st==NULL)) return true;
    return ((*value_st**(p_st->value_st)).is_empty()!=TTRUE);
  case ST_RECORDOF:
  case ST_SETOF:
    if ((recof_st==NULL) || (p_st->recof_st==NULL)) return true;
    return ((*recof_st**(p_st->recof_st)).is_empty()!=TTRUE);
  default:
    FATAL_ERROR("SubtypeConstraint::is_compatible()");
  }
  return true;
}

bool SubtypeConstraint::is_compatible_with_elem() const
{
  if (subtype==ST_ERROR) return true;
  switch (subtype) {
  case ST_BITSTRING: {
    static BitstringConstraint str_elem(size_limit_t(1));
    if (bitstring_st==NULL) return true;
    return ((*bitstring_st*str_elem).is_empty()!=TTRUE);
  }
  case ST_HEXSTRING: {
    static HexstringConstraint str_elem(size_limit_t(1));
    if (hexstring_st==NULL) return true;
    return ((*hexstring_st*str_elem).is_empty()!=TTRUE);
  }
  case ST_OCTETSTRING: {
    static OctetstringConstraint str_elem(size_limit_t(1));
    if (octetstring_st==NULL) return true;
    return ((*octetstring_st*str_elem).is_empty()!=TTRUE);
  }
  case ST_CHARSTRING: {
    size_limit_t t = size_limit_t(1);
    SizeRangeListConstraint temp = SizeRangeListConstraint(t);
    static CharstringSubtypeTreeElement str_elem(temp);
    if (charstring_st==NULL) return true;
    CharstringSubtypeTreeElement* cc = new CharstringSubtypeTreeElement(
      CharstringSubtypeTreeElement::ET_INTERSECTION,
      new CharstringSubtypeTreeElement(*charstring_st),
      new CharstringSubtypeTreeElement(str_elem)
    );
    bool rv = (cc->is_empty()!=TTRUE);
    delete cc;
    return rv;
  }
  case ST_UNIVERSAL_CHARSTRING: {
    size_limit_t t = size_limit_t(1);
    SizeRangeListConstraint temp = SizeRangeListConstraint(t);
    static UniversalCharstringSubtypeTreeElement str_elem(temp);
    if (universal_charstring_st==NULL) return true;
    UniversalCharstringSubtypeTreeElement* ucc = new UniversalCharstringSubtypeTreeElement(
      UniversalCharstringSubtypeTreeElement::ET_INTERSECTION,
      new UniversalCharstringSubtypeTreeElement(*universal_charstring_st),
      new UniversalCharstringSubtypeTreeElement(str_elem)
    );
    bool rv = (ucc->is_empty()!=TTRUE);
    delete ucc;
    return rv;
  }
  default:
    FATAL_ERROR("SubtypeConstraint::is_compatible_with_elem()");
  }
  return true;
}

bool SubtypeConstraint::is_length_compatible(const SubtypeConstraint *p_st) const
{
  if (p_st==NULL) FATAL_ERROR("SubtypeConstraint::is_length_compatible()");
  if ((subtype==ST_ERROR) || (p_st->subtype==ST_ERROR)) return true;
  if (subtype != ST_RECORDOF && subtype != ST_SETOF &&
      p_st->subtype != ST_RECORDOF && p_st->subtype != ST_SETOF)
    FATAL_ERROR("SubtypeConstraint::is_length_compatible()");
  if (length_restriction==NULL || p_st->length_restriction==NULL) return true;
  return ((*length_restriction * *(p_st->length_restriction)).is_empty()!=TTRUE);
}

bool SubtypeConstraint::is_upper_limit_infinity() const
{
  if (ST_INTEGER == subtype && integer_st) {
    return integer_st->is_upper_limit_infinity();
  }
  if (ST_FLOAT == subtype && float_st) {
    return float_st->is_upper_limit_infinity();
  }
  return false;
}

bool SubtypeConstraint::is_lower_limit_infinity() const
{
  if (ST_INTEGER == subtype && integer_st) {
    return integer_st->is_lower_limit_infinity();
  }
  
  if (ST_FLOAT == subtype && float_st) {
    return float_st->is_lower_limit_infinity();
  }
  return false;
}


void SubtypeConstraint::except(const SubtypeConstraint* other)
{
  if (other==NULL) FATAL_ERROR("SubtypeConstraint::except()");
  if (subtype!=other->subtype) FATAL_ERROR("SubtypeConstraint::except()");
  switch (subtype) {
  case ST_INTEGER:
    if (other->integer_st==NULL) {
      if (integer_st==NULL) {
        integer_st = new IntegerRangeListConstraint();
      } else {
        *integer_st = IntegerRangeListConstraint();
      }
    } else {
      if (integer_st==NULL) {
        integer_st = new IntegerRangeListConstraint(~*(other->integer_st));
      } else {
        *integer_st = *integer_st - *(other->integer_st);
      }
    }
    break;
  case ST_FLOAT:
    if (other->float_st==NULL) {
      if (float_st==NULL) {
        float_st = new RealRangeListConstraint();
      } else {
        *float_st = RealRangeListConstraint();
      }
    } else {
      if (float_st==NULL) {
        float_st = new RealRangeListConstraint(~*(other->float_st));
      } else {
        *float_st = *float_st - *(other->float_st);
      }
    }
    break;
  case ST_BOOLEAN:
    if (other->boolean_st==NULL) {
      if (boolean_st==NULL) {
        boolean_st = new BooleanListConstraint();
      } else {
        *boolean_st = BooleanListConstraint();
      }
    } else {
      if (boolean_st==NULL) {
        boolean_st = new BooleanListConstraint(~*(other->boolean_st));
      } else {
        *boolean_st = *boolean_st - *(other->boolean_st);
      }
    }
    break;
  case ST_VERDICTTYPE:
    if (other->verdict_st==NULL) {
      if (verdict_st==NULL) {
        verdict_st = new VerdicttypeListConstraint();
      } else {
        *verdict_st = VerdicttypeListConstraint();
      }
    } else {
      if (verdict_st==NULL) {
        verdict_st = new VerdicttypeListConstraint(~*(other->verdict_st));
      } else {
        *verdict_st = *verdict_st - *(other->verdict_st);
      }
    }
    break;
  case ST_BITSTRING:
    if (other->bitstring_st==NULL) {
      if (bitstring_st==NULL) {
        bitstring_st = new BitstringConstraint();
      } else {
        *bitstring_st = BitstringConstraint();
      }
    } else {
      if (bitstring_st==NULL) {
        bitstring_st = new BitstringConstraint(~*(other->bitstring_st));
      } else {
        *bitstring_st = *bitstring_st - *(other->bitstring_st);
      }
    }
    break;
  case ST_HEXSTRING:
    if (other->hexstring_st==NULL) {
      if (hexstring_st==NULL) {
        hexstring_st = new HexstringConstraint();
      } else {
        *hexstring_st = HexstringConstraint();
      }
    } else {
      if (hexstring_st==NULL) {
        hexstring_st = new HexstringConstraint(~*(other->hexstring_st));
      } else {
        *hexstring_st = *hexstring_st - *(other->hexstring_st);
      }
    }
    break;
  case ST_OCTETSTRING:
    if (other->octetstring_st==NULL) {
      if (octetstring_st==NULL) {
        octetstring_st = new OctetstringConstraint();
      } else {
        *octetstring_st = OctetstringConstraint();
      }
    } else {
      if (octetstring_st==NULL) {
        octetstring_st = new OctetstringConstraint(~*(other->octetstring_st));
      } else {
        *octetstring_st = *octetstring_st - *(other->octetstring_st);
      }
    }
    break;
  case ST_CHARSTRING:
    if (other->charstring_st==NULL) {
      if (charstring_st==NULL) {
        charstring_st = new CharstringSubtypeTreeElement();
      } else {
        *charstring_st = CharstringSubtypeTreeElement();
      }
    } else {
      if (charstring_st==NULL) {
        CharstringSubtypeTreeElement* call_st = new CharstringSubtypeTreeElement();
        call_st->set_all();
        charstring_st = new CharstringSubtypeTreeElement(CharstringSubtypeTreeElement::ET_EXCEPT,
          call_st, new CharstringSubtypeTreeElement(*(other->charstring_st)));
      } else {
        charstring_st = new CharstringSubtypeTreeElement(CharstringSubtypeTreeElement::ET_EXCEPT,
          charstring_st, new CharstringSubtypeTreeElement(*(other->charstring_st)));
      }
    }
    break;
  case ST_UNIVERSAL_CHARSTRING:
    if (other->universal_charstring_st==NULL) {
      if (universal_charstring_st==NULL) {
        universal_charstring_st = new UniversalCharstringSubtypeTreeElement();
      } else {
        *universal_charstring_st = UniversalCharstringSubtypeTreeElement();
      }
    } else {
      if (universal_charstring_st==NULL) {
        UniversalCharstringSubtypeTreeElement* ucall_st = new UniversalCharstringSubtypeTreeElement();
        ucall_st->set_all();
        universal_charstring_st = new UniversalCharstringSubtypeTreeElement(UniversalCharstringSubtypeTreeElement::ET_EXCEPT,
          ucall_st, new UniversalCharstringSubtypeTreeElement(*(other->universal_charstring_st)));
      } else {
        universal_charstring_st = new UniversalCharstringSubtypeTreeElement(UniversalCharstringSubtypeTreeElement::ET_EXCEPT,
          universal_charstring_st, new UniversalCharstringSubtypeTreeElement(*(other->universal_charstring_st)));
      }
    }
    break;
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    if (other->value_st==NULL) {
      if (value_st==NULL) {
        value_st = new ValueListConstraint();
      } else {
        *value_st = ValueListConstraint();
      }
    } else {
      if (value_st==NULL) {
        value_st = new ValueListConstraint(~*(other->value_st));
      } else {
        *value_st = *value_st - *(other->value_st);
      }
    }
    break;
  case ST_RECORDOF:
  case ST_SETOF:
    if (other->recof_st==NULL) {
      if (recof_st==NULL) {
        recof_st = new RecofConstraint();
      } else {
        *recof_st = RecofConstraint();
      }
    } else {
      if (recof_st==NULL) {
        recof_st = new RecofConstraint(~*(other->recof_st));
      } else {
        *recof_st = *recof_st - *(other->recof_st);
      }
    }
    break;
  default:
    FATAL_ERROR("SubtypeConstraint::except()");
  }
  if (other->length_restriction==NULL) {
    if (length_restriction==NULL) {
      length_restriction = new SizeRangeListConstraint();
    } else {
      *length_restriction = SizeRangeListConstraint();
    }
  } else {
    if (length_restriction==NULL) {
      length_restriction = new SizeRangeListConstraint(~*(other->length_restriction));
    } else {
      *length_restriction = *length_restriction - *(other->length_restriction);
    }
  }
}

void SubtypeConstraint::union_(const SubtypeConstraint* other)
{
  if (other==NULL) FATAL_ERROR("SubtypeConstraint::union_()");
  if (subtype!=other->subtype) FATAL_ERROR("SubtypeConstraint::union_()");
  switch (subtype) {
  case ST_INTEGER:
    if (integer_st==NULL) break;
    if (other->integer_st==NULL) { delete integer_st; integer_st = NULL; break; }
    *integer_st = *integer_st + *(other->integer_st);
    break;
  case ST_FLOAT:
    if (float_st==NULL) break;
    if (other->float_st==NULL) { delete float_st; float_st = NULL; break; }
    *float_st = *float_st + *(other->float_st);
    break;
  case ST_BOOLEAN:
    if (boolean_st==NULL) break;
    if (other->boolean_st==NULL) { delete boolean_st; boolean_st = NULL; break; }
    *boolean_st = *boolean_st + *(other->boolean_st);
    break;
  case ST_VERDICTTYPE:
    if (verdict_st==NULL) break;
    if (other->verdict_st==NULL) { delete verdict_st; verdict_st = NULL; break; }
    *verdict_st = *verdict_st + *(other->verdict_st);
    break;
  case ST_BITSTRING:
    if (bitstring_st==NULL) break;
    if (other->bitstring_st==NULL) { delete bitstring_st; bitstring_st = NULL; break; }
    *bitstring_st = *bitstring_st + *(other->bitstring_st);
    break;
  case ST_HEXSTRING:
    if (hexstring_st==NULL) break;
    if (other->hexstring_st==NULL) { delete hexstring_st; hexstring_st = NULL; break; }
    *hexstring_st = *hexstring_st + *(other->hexstring_st);
    break;
  case ST_OCTETSTRING:
    if (octetstring_st==NULL) break;
    if (other->octetstring_st==NULL) { delete octetstring_st; octetstring_st = NULL; break; }
    *octetstring_st = *octetstring_st + *(other->octetstring_st);
    break;
  case ST_CHARSTRING:
    if (charstring_st==NULL) break;
    if (other->charstring_st==NULL) { delete charstring_st; charstring_st = NULL; break; }
    charstring_st = new CharstringSubtypeTreeElement(CharstringSubtypeTreeElement::ET_UNION,
      charstring_st, new CharstringSubtypeTreeElement(*(other->charstring_st)));
    break;
  case ST_UNIVERSAL_CHARSTRING:
    if (universal_charstring_st==NULL) break;
    if (other->universal_charstring_st==NULL) { delete universal_charstring_st; universal_charstring_st = NULL; break; }
    universal_charstring_st = new UniversalCharstringSubtypeTreeElement(UniversalCharstringSubtypeTreeElement::ET_UNION,
      universal_charstring_st, new UniversalCharstringSubtypeTreeElement(*(other->universal_charstring_st)));
    break;
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    if (value_st==NULL) break;
    if (other->value_st==NULL) { delete value_st; value_st = NULL; break; }
    *value_st = *value_st + *(other->value_st);
    break;
  case ST_RECORDOF:
  case ST_SETOF:
    if (recof_st==NULL) break;
    if (other->recof_st==NULL) { delete recof_st; recof_st = NULL; break; }
    *recof_st = *recof_st + *(other->recof_st);
    break;
  default:
    FATAL_ERROR("SubtypeConstraint::union_()");
  }
  if (length_restriction!=NULL) {
    if (other->length_restriction==NULL) {
      delete length_restriction;
      length_restriction = NULL;
    } else {
      *length_restriction = *length_restriction + *(other->length_restriction);
    }
  }
}

void SubtypeConstraint::intersection(const SubtypeConstraint* other)
{
  if (other==NULL) FATAL_ERROR("SubtypeConstraint::intersection()");
  if (subtype!=other->subtype) FATAL_ERROR("SubtypeConstraint::intersection()");
  switch (subtype) {
  case ST_INTEGER:
    if (other->integer_st!=NULL) {
      if (integer_st==NULL) {
        integer_st = new IntegerRangeListConstraint(*(other->integer_st));
      } else {
        *integer_st = *integer_st * *(other->integer_st);
      }
    }
    break;
  case ST_FLOAT:
    if (other->float_st!=NULL) {
      if (float_st==NULL) {
        float_st = new RealRangeListConstraint(*(other->float_st));
      } else {
        *float_st = *float_st * *(other->float_st);
      }
    }
    break;
  case ST_BOOLEAN:
    if (other->boolean_st!=NULL) {
      if (boolean_st==NULL) {
        boolean_st = new BooleanListConstraint(*(other->boolean_st));
      } else {
        *boolean_st = *boolean_st * *(other->boolean_st);
      }
    }
    break;
  case ST_VERDICTTYPE:
    if (other->verdict_st!=NULL) {
      if (verdict_st==NULL) {
        verdict_st = new VerdicttypeListConstraint(*(other->verdict_st));
      } else {
        *verdict_st = *verdict_st * *(other->verdict_st);
      }
    }
    break;
  case ST_BITSTRING:
    if (other->bitstring_st!=NULL) {
      if (bitstring_st==NULL) {
        bitstring_st = new BitstringConstraint(*(other->bitstring_st));
      } else {
        *bitstring_st = *bitstring_st * *(other->bitstring_st);
      }
    }
    break;
  case ST_HEXSTRING:
    if (other->hexstring_st!=NULL) {
      if (hexstring_st==NULL) {
        hexstring_st = new HexstringConstraint(*(other->hexstring_st));
      } else {
        *hexstring_st = *hexstring_st * *(other->hexstring_st);
      }
    }
    break;
  case ST_OCTETSTRING:
    if (other->octetstring_st!=NULL) {
      if (octetstring_st==NULL) {
        octetstring_st = new OctetstringConstraint(*(other->octetstring_st));
      } else {
        *octetstring_st = *octetstring_st * *(other->octetstring_st);
      }
    }
    break;
  case ST_CHARSTRING:
    if (other->charstring_st!=NULL) {
      if (charstring_st==NULL) {
        charstring_st = new CharstringSubtypeTreeElement(*(other->charstring_st));
      } else {
        charstring_st = new CharstringSubtypeTreeElement(CharstringSubtypeTreeElement::ET_INTERSECTION,
          charstring_st, new CharstringSubtypeTreeElement(*(other->charstring_st)));
      }
    }
    break;
  case ST_UNIVERSAL_CHARSTRING:
    if (other->universal_charstring_st!=NULL) {
      if (universal_charstring_st==NULL) {
        universal_charstring_st = new UniversalCharstringSubtypeTreeElement(*(other->universal_charstring_st));
      } else {
        universal_charstring_st = new UniversalCharstringSubtypeTreeElement(UniversalCharstringSubtypeTreeElement::ET_INTERSECTION,
          universal_charstring_st, new UniversalCharstringSubtypeTreeElement(*(other->universal_charstring_st)));
      }
    }
    break;
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    if (other->value_st!=NULL) {
      if (value_st==NULL) {
        value_st = new ValueListConstraint(*(other->value_st));
      } else {
        *value_st = *value_st * *(other->value_st);
      }
    }
    break;
  case ST_RECORDOF:
  case ST_SETOF:
    if (other->recof_st!=NULL) {
      if (recof_st==NULL) {
        recof_st = new RecofConstraint(*(other->recof_st));
      } else {
        *recof_st = *recof_st * *(other->recof_st);
      }
    }
    break;
  default:
    FATAL_ERROR("SubtypeConstraint::intersection()");
  }
  if (other->length_restriction!=NULL) {
    if (length_restriction==NULL) {
      length_restriction = new SizeRangeListConstraint(*(other->length_restriction));
    } else {
      *length_restriction = *length_restriction * *(other->length_restriction);
    }
  }
}

tribool SubtypeConstraint::is_subset(const SubtypeConstraint* other) const
{
  if (other==NULL) return TTRUE;
  if (other->subtype!=subtype) FATAL_ERROR("SubtypeConstraint::is_subset()");
  switch (subtype) {
  case ST_INTEGER:
    if (other->integer_st==NULL) return TTRUE;
    return integer_st ? integer_st->is_subset(*(other->integer_st)) : TTRUE;
  case ST_FLOAT:
    if (other->float_st==NULL) return TTRUE;
    return float_st ? float_st->is_subset(*(other->float_st)) : TTRUE;
  case ST_BOOLEAN:
    if (other->boolean_st==NULL) return TTRUE;
    return boolean_st ? boolean_st->is_subset(*(other->boolean_st)) : TTRUE;
  case ST_VERDICTTYPE:
    if (other->verdict_st==NULL) return TTRUE;
    return verdict_st ? verdict_st->is_subset(*(other->verdict_st)) : TTRUE;
  case ST_BITSTRING:
    if (other->bitstring_st==NULL) return TTRUE;
    return bitstring_st ? bitstring_st->is_subset(*(other->bitstring_st)) : TTRUE;
  case ST_HEXSTRING:
    if (other->hexstring_st==NULL) return TTRUE;
    return hexstring_st ? hexstring_st->is_subset(*(other->hexstring_st)) : TTRUE;
  case ST_OCTETSTRING:
    if (other->octetstring_st==NULL) return TTRUE;
    return octetstring_st ? octetstring_st->is_subset(*(other->octetstring_st)) : TTRUE;
  case ST_CHARSTRING:
    if (other->charstring_st==NULL) return TTRUE;
    return charstring_st ? charstring_st->is_subset(other->charstring_st) : TTRUE;
  case ST_UNIVERSAL_CHARSTRING:
    if (other->universal_charstring_st==NULL) return TTRUE;
    return universal_charstring_st ? universal_charstring_st->is_subset(other->universal_charstring_st) : TTRUE;
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    if (other->value_st==NULL) return TTRUE;
    return value_st ? value_st->is_subset(*(other->value_st)) : TTRUE;
  case ST_RECORDOF:
  case ST_SETOF:
    if (other->recof_st==NULL) return TTRUE;
    return recof_st ? recof_st->is_subset(*(other->recof_st)) : TTRUE;
  default:
    FATAL_ERROR("SubtypeConstraint::is_subset()");
  }
  return TUNKNOWN;
}

/********************
class SubType
********************/

SubType::SubType(subtype_t st, Type *p_my_owner, SubType* p_parent_subtype,
  vector<SubTypeParse> *p_parsed, Constraints* p_asn_constraints)
: SubtypeConstraint(st), my_owner(p_my_owner), parent_subtype(p_parent_subtype)
, parsed(p_parsed), asn_constraints(p_asn_constraints)
, root(0), extendable(false), extension(0), checked(STC_NO)
, my_parents()
{
  if (p_my_owner==NULL) FATAL_ERROR("SubType::SubType()");
}

SubType::~SubType()
{
  my_parents.clear();
}

void SubType::chk_this_value(Value *value)
{
  if (checked==STC_NO) FATAL_ERROR("SubType::chk_this_value()");
  if ((checked==STC_CHECKING) || (subtype==ST_ERROR)) return;
  Value *val = value->get_value_refd_last();
  bool is_invalid = false;
  switch (val->get_valuetype()) {
  case Value::V_INT:
    if (subtype!=ST_INTEGER) FATAL_ERROR("SubType::chk_this_value()");
    is_invalid = (integer_st!=NULL) && !integer_st->is_element(int_limit_t(*(val->get_val_Int())));
    break;
  case Value::V_REAL:
    if (subtype!=ST_FLOAT) FATAL_ERROR("SubType::chk_this_value()");
    is_invalid = (float_st!=NULL) && !float_st->is_element(val->get_val_Real());
    break;
  case Value::V_BOOL:
    if (subtype!=ST_BOOLEAN) FATAL_ERROR("SubType::chk_this_value()");
    is_invalid = (boolean_st!=NULL) && !boolean_st->is_element(val->get_val_bool());
    break;
  case Value::V_VERDICT: {
    if (subtype!=ST_VERDICTTYPE) FATAL_ERROR("SubType::chk_this_value()");
    VerdicttypeListConstraint::verdicttype_constraint_t vtc;
    switch (val->get_val_verdict()) {
    case Value::Verdict_NONE: vtc = VerdicttypeListConstraint::VC_NONE; break;
    case Value::Verdict_PASS: vtc = VerdicttypeListConstraint::VC_PASS; break;
    case Value::Verdict_INCONC: vtc = VerdicttypeListConstraint::VC_INCONC; break;
    case Value::Verdict_FAIL: vtc = VerdicttypeListConstraint::VC_FAIL; break;
    case Value::Verdict_ERROR: vtc = VerdicttypeListConstraint::VC_ERROR; break;
    default: FATAL_ERROR("SubType::chk_this_value()");
    }
    is_invalid = (verdict_st!=NULL) && !verdict_st->is_element(vtc);
  } break;
  case Value::V_BSTR:
    if (subtype!=ST_BITSTRING) FATAL_ERROR("SubType::chk_this_value()");
    is_invalid = (bitstring_st!=NULL) && !bitstring_st->is_element(val->get_val_str());
    break;
  case Value::V_HSTR:
    if (subtype!=ST_HEXSTRING) FATAL_ERROR("SubType::chk_this_value()");
    is_invalid = (hexstring_st!=NULL) && !hexstring_st->is_element(val->get_val_str());
    break;
  case Value::V_OSTR:
    if (subtype!=ST_OCTETSTRING) FATAL_ERROR("SubType::chk_this_value()");
    is_invalid = (octetstring_st!=NULL) && !octetstring_st->is_element(val->get_val_str());
    break;
  case Value::V_CSTR:
  case Value::V_ISO2022STR:
    if (subtype!=ST_CHARSTRING) FATAL_ERROR("SubType::chk_this_value()");
    is_invalid = (charstring_st!=NULL) && !charstring_st->is_element(val->get_val_str());
    break;
  case Value::V_USTR:
  case Value::V_CHARSYMS:
    if (subtype!=ST_UNIVERSAL_CHARSTRING) FATAL_ERROR("SubType::chk_this_value()");
    is_invalid = (universal_charstring_st!=NULL) && !universal_charstring_st->is_element(val->get_val_ustr());
    break;
  case Value::V_SEQOF:
    if (subtype!=ST_RECORDOF) FATAL_ERROR("SubType::chk_this_value()");
    if (value->is_unfoldable()) return;
    is_invalid = (recof_st!=NULL) && !recof_st->is_element(val);
    break;
  case Value::V_SETOF:
    if (subtype!=ST_SETOF) FATAL_ERROR("SubType::chk_this_value()");
    if (value->is_unfoldable()) return;
    is_invalid = (recof_st!=NULL) && !recof_st->is_element(val);
    break;
  case Value::V_OID:
  case Value::V_ROID:
    if (subtype!=ST_OBJID) FATAL_ERROR("SubType::chk_this_value()");
    if (value->is_unfoldable()) return;
    is_invalid = (value_st!=NULL) && !value_st->is_element(val);
    break;
  case Value::V_ENUM:
  case Value::V_NULL: // FIXME: should go to ST_NULL
    if (subtype!=ST_ENUM) FATAL_ERROR("SubType::chk_this_value()");
    if (value->is_unfoldable()) return;
    is_invalid = (value_st!=NULL) && !value_st->is_element(val);
    break;
  case Value::V_CHOICE:
  case Value::V_OPENTYPE: // FIXME?
    if (subtype!=ST_UNION) FATAL_ERROR("SubType::chk_this_value()");
    if (value->is_unfoldable()) return;
    is_invalid = (value_st!=NULL) && !value_st->is_element(val);
    break;
  case Value::V_SEQ:
    if (subtype!=ST_RECORD) FATAL_ERROR("SubType::chk_this_value()");
    if (value->is_unfoldable()) return;
    is_invalid = (value_st!=NULL) && !value_st->is_element(val);
    break;
  case Value::V_SET:
    if (subtype!=ST_SET) FATAL_ERROR("SubType::chk_this_value()");
    if (value->is_unfoldable()) return;
    is_invalid = (value_st!=NULL) && !value_st->is_element(val);
    break;
  case Value::V_FUNCTION:
    if (subtype!=ST_FUNCTION) FATAL_ERROR("SubType::chk_this_value()");
    if (value->is_unfoldable()) return;
    is_invalid = (value_st!=NULL) && !value_st->is_element(val);
    break;
  case Value::V_ALTSTEP:
    if (subtype!=ST_ALTSTEP) FATAL_ERROR("SubType::chk_this_value()");
    if (value->is_unfoldable()) return;
    is_invalid = (value_st!=NULL) && !value_st->is_element(val);
    break;
  case Value::V_TESTCASE:
    if (subtype!=ST_TESTCASE) FATAL_ERROR("SubType::chk_this_value()");
    if (value->is_unfoldable()) return;
    is_invalid = (value_st!=NULL) && !value_st->is_element(val);
    break;
  case Value::V_ERROR:
    return;
  default:
    return;
  }
  if (is_invalid) {
    value->error("%s is not a valid value for type `%s' which has subtype %s",
      val->get_stringRepr().c_str(),
      my_owner->get_typename().c_str(),
      to_string().c_str());
  }
}

/** \todo revise */
void SubType::chk_this_template_generic(Template *templ)
{
  if (checked==STC_NO) FATAL_ERROR("SubType::chk_this_template_generic()");
  if ((checked==STC_CHECKING) || (subtype==ST_ERROR)) return;
  templ = templ->get_template_refd_last();
  switch (templ->get_templatetype()) {
  case Ttcn::Template::OMIT_VALUE:
  case Ttcn::Template::ANY_OR_OMIT:
  case Ttcn::Template::TEMPLATE_ERROR:
  case Ttcn::Template::ANY_VALUE:
    break;
  case Ttcn::Template::VALUE_LIST:
  case Ttcn::Template::COMPLEMENTED_LIST:
    /* Should be canonical before */
    break;
  case Ttcn::Template::SPECIFIC_VALUE:
    /* SPECIFIC_VALUE must be already checked in Type::chk_this_template() */
    break;
  case Ttcn::Template::TEMPLATE_REFD:
    /* unfoldable reference: cannot be checked at compile time */
    break;
  case Ttcn::Template::TEMPLATE_INVOKE:
    /* should be already checked in Type::chk_this_template() */
    break;
  default:
    chk_this_template(templ);
    break;
  }
  chk_this_template_length_restriction(templ);
}

/** \todo revise */
void SubType::chk_this_template(Template *templ)
{
  switch (templ->get_templatetype()) {
  case Template::TEMPLATE_LIST:
    if ( (length_restriction!=NULL) && TTRUE != length_restriction->is_empty() ) {
      size_t nof_comp_woaon = templ->get_nof_comps_not_anyornone();
      if (!templ->temps_contains_anyornone_symbol() &&
        nof_comp_woaon < length_restriction->get_minimal().get_size()) {
        templ->error("At least %s elements must be present in the list",
          Int2string((Int)(length_restriction->get_minimal().get_size())).c_str());
        return;
      } else if ( !length_restriction->get_maximal().get_infinity() && (nof_comp_woaon > length_restriction->get_maximal().get_size()) ) {
        templ->error("There must not be more than %s elements in the list",
          Int2string((Int)(length_restriction->get_maximal().get_size())).c_str());
        return;
      }
    }
    break;
  /* Simply break.  We don't know how many elements are there.
       SUPERSET_MATCH/SUBSET_MATCH is not possible to be an
       INDEXED_TEMPLATE_LIST.  */
  case Template::INDEXED_TEMPLATE_LIST:
    break;
  case Template::NAMED_TEMPLATE_LIST:
    break;
  case Template::VALUE_RANGE:
    /* Should be canonical before */
    break;
  case Template::ALL_FROM:
  case Template::DECODE_MATCH:
    break;
  case Template::TEMPLATE_CONCAT:
    if (!use_runtime_2) {
      FATAL_ERROR("SubType::chk_this_template()");
    }
    break;
  case Template::SUPERSET_MATCH:
  case Template::SUBSET_MATCH:
    if (subtype!=ST_SETOF){
      templ->error("'subset' template matching mechanism can be used "
        "only with 'set of' types");
      return;
    }
    for (size_t i=0;i<templ->get_nof_comps();i++)
      chk_this_template_generic(templ->get_temp_byIndex(i));
    break;
  case Template::BSTR_PATTERN:
    chk_this_template_pattern("bitstring", templ);
    break;
  case Template::HSTR_PATTERN:
    chk_this_template_pattern("hexstring", templ);
    break;
  case Template::OSTR_PATTERN:
    chk_this_template_pattern("octetstring", templ);
    break;
  case Template::CSTR_PATTERN:
    chk_this_template_pattern("charstring", templ);
    break;
  case Template::USTR_PATTERN:
	  chk_this_template_pattern("universal charstring", templ);
	  break;
  case Template::TEMPLATE_NOTUSED:
    break;
  case Template::TEMPLATE_ERROR:
    break;
  default:
    FATAL_ERROR("SubType::chk_this_template()");
    break;
  }
}

void SubType::chk_this_template_length_restriction(Template *templ)
{
  if (!templ->is_length_restricted()) return;
  // if there is a length restriction on the template then check if
  // the intersection of the two restrictions is not empty
  size_limit_t tmpl_min_len(size_limit_t(0));
  size_limit_t tmpl_max_len(size_limit_t::INFINITE_SIZE);
  Ttcn::LengthRestriction *lr=templ->get_length_restriction();
  lr->chk(Type::EXPECTED_DYNAMIC_VALUE);
  if (!lr->get_is_range()) { //Template's lr is single
    Value *tmp_val=lr->get_single_value();
    if (tmp_val->get_valuetype()!=Value::V_INT) return;
    Int templ_len = tmp_val->get_val_Int()->get_val();
    tmpl_min_len = tmpl_max_len = size_limit_t((size_t)templ_len);
  } else { //Template's lr is range
    Value *tmp_lower=lr->get_lower_value();
    if (tmp_lower->get_valuetype()!=Value::V_INT) return;
    Int templ_lower = tmp_lower->get_val_Int()->get_val();
    tmpl_min_len = size_limit_t((size_t)templ_lower);
    Value *tmp_upper=lr->get_upper_value();
    if (tmp_upper && tmp_upper->get_valuetype()!=Value::V_INT) return;
    if (tmp_upper) tmpl_max_len = size_limit_t((size_t)tmp_upper->get_val_Int()->get_val());
  }

  bool is_err = false;
  switch (subtype) {
  case ST_BITSTRING:
    if (bitstring_st!=NULL) {
      BitstringConstraint bc = *bitstring_st * BitstringConstraint(tmpl_min_len,tmpl_max_len);
      if (bc.is_empty()==TTRUE) is_err = true;
    }
    break;
  case ST_HEXSTRING:
    if (hexstring_st!=NULL) {
      HexstringConstraint hc = *hexstring_st * HexstringConstraint(tmpl_min_len,tmpl_max_len);
      if (hc.is_empty()==TTRUE) is_err = true;
    }
    break;
  case ST_OCTETSTRING:
    if (octetstring_st!=NULL) {
      OctetstringConstraint oc = *octetstring_st * OctetstringConstraint(tmpl_min_len,tmpl_max_len);
      if (oc.is_empty()==TTRUE) is_err = true;
    }
    break;
  case ST_CHARSTRING:
    if (charstring_st!=NULL) {
      CharstringSubtypeTreeElement* cc = new CharstringSubtypeTreeElement(
        CharstringSubtypeTreeElement::ET_INTERSECTION,
        new CharstringSubtypeTreeElement(*charstring_st),
        new CharstringSubtypeTreeElement(SizeRangeListConstraint(tmpl_min_len,tmpl_max_len))
      );
      if (cc->is_empty()==TTRUE) is_err = true;
      delete cc;
    }
    break;
  case ST_UNIVERSAL_CHARSTRING:
    if (universal_charstring_st!=NULL) {
      UniversalCharstringSubtypeTreeElement* ucc = new UniversalCharstringSubtypeTreeElement(
        UniversalCharstringSubtypeTreeElement::ET_INTERSECTION,
        new UniversalCharstringSubtypeTreeElement(*universal_charstring_st),
        new UniversalCharstringSubtypeTreeElement(SizeRangeListConstraint(tmpl_min_len,tmpl_max_len))
      );
      if (ucc->is_empty()==TTRUE) is_err = true;
      delete ucc;
    }
    break;
  case ST_RECORDOF:
  case ST_SETOF:
    if (recof_st!=NULL) {
      RecofConstraint rc = *recof_st * RecofConstraint(tmpl_min_len,tmpl_max_len);
      if (rc.is_empty()==TTRUE) is_err = true;
    }
    break;
  default:
    break;
  }
  if (is_err) {
    templ->error("Template's length restriction %s is outside of the type's subtype constraint %s",
      SizeRangeListConstraint(tmpl_min_len,tmpl_max_len).to_string().c_str(), to_string().c_str());
  }
}

void SubType::chk_this_template_pattern(const char *patt_type, Template *templ)
{
  Template::templatetype_t temptype= templ->get_templatetype();
  if ((temptype==Template::BSTR_PATTERN && subtype!=ST_BITSTRING) ||
    (temptype==Template::HSTR_PATTERN && subtype!=ST_HEXSTRING) ||
    (temptype==Template::OSTR_PATTERN && subtype!=ST_OCTETSTRING) ||
    (temptype==Template::CSTR_PATTERN && subtype!=ST_CHARSTRING) ||
    (temptype==Template::USTR_PATTERN && subtype!=ST_UNIVERSAL_CHARSTRING))
  {
    templ->error("Template is incompatible with subtype");
    return;
  }
  if ( (length_restriction!=NULL) && TTRUE != length_restriction->is_empty() ) {
    Int patt_min_len = static_cast<Int>(templ->get_min_length_of_pattern());
    if (patt_min_len < (Int)(length_restriction->get_minimal().get_size()) &&
      !templ->pattern_contains_anyornone_symbol()) {
      templ->error("At least %s string elements must be present in the %s",
        Int2string((Int)(length_restriction->get_minimal().get_size())).c_str(), patt_type);
    } else if ( !length_restriction->get_maximal().get_infinity() && (patt_min_len > (Int)(length_restriction->get_maximal().get_size())) ) {
      templ->error("There must not be more than %s string elements in the %s",
        Int2string((Int)(length_restriction->get_maximal().get_size())).c_str(), patt_type);
    }
  }
}

void SubType::add_ttcn_value(Value *v)
{
  if (value_st==NULL) value_st = new ValueListConstraint(v);
  else *value_st = *value_st + ValueListConstraint(v);
}

void SubType::add_ttcn_recof(Value *v)
{
  if (recof_st==NULL) recof_st = new RecofConstraint(v);
  else *recof_st = *recof_st + RecofConstraint(v);
}

bool SubType::add_ttcn_type_list_subtype(SubType* p_st)
{
  switch (subtype) {
  case ST_INTEGER:
    if (p_st->integer_st==NULL) return false;
    if (integer_st==NULL) integer_st = new IntegerRangeListConstraint(*(p_st->integer_st));
    else *integer_st = *integer_st + *(p_st->integer_st);
    break;
  case ST_FLOAT:
    if (p_st->float_st==NULL) return false;
    if (float_st==NULL) float_st = new RealRangeListConstraint(*(p_st->float_st));
    else *float_st = *float_st + *(p_st->float_st);
    break;
  case ST_BOOLEAN:
    if (p_st->boolean_st==NULL) return false;
    if (boolean_st==NULL) boolean_st = new BooleanListConstraint(*(p_st->boolean_st));
    else *boolean_st = *boolean_st + *(p_st->boolean_st);
    break;
  case ST_VERDICTTYPE:
    if (p_st->verdict_st==NULL) return false;
    if (verdict_st==NULL) verdict_st = new VerdicttypeListConstraint(*(p_st->verdict_st));
    else *verdict_st = *verdict_st + *(p_st->verdict_st);
    break;
  case ST_BITSTRING:
    if (p_st->bitstring_st==NULL) return false;
    if (bitstring_st==NULL) bitstring_st = new BitstringConstraint(*(p_st->bitstring_st));
    else *bitstring_st = *bitstring_st + *(p_st->bitstring_st);
    break;
  case ST_HEXSTRING:
    if (p_st->hexstring_st==NULL) return false;
    if (hexstring_st==NULL) hexstring_st = new HexstringConstraint(*(p_st->hexstring_st));
    else *hexstring_st = *hexstring_st + *(p_st->hexstring_st);
    break;
  case ST_OCTETSTRING:
    if (p_st->octetstring_st==NULL) return false;
    if (octetstring_st==NULL) octetstring_st = new OctetstringConstraint(*(p_st->octetstring_st));
    else *octetstring_st = *octetstring_st + *(p_st->octetstring_st);
    break;
  case ST_CHARSTRING:
    if (p_st->charstring_st==NULL) return false;
    if (charstring_st==NULL) {
      charstring_st = new CharstringSubtypeTreeElement(*(p_st->charstring_st));
    } else {
      charstring_st = new CharstringSubtypeTreeElement(
        CharstringSubtypeTreeElement::ET_UNION,
        charstring_st,
        new CharstringSubtypeTreeElement(*(p_st->charstring_st)));
    }
    break;
  case ST_UNIVERSAL_CHARSTRING:
    if (p_st->universal_charstring_st==NULL) return false;
    if (universal_charstring_st==NULL) {
      universal_charstring_st = new UniversalCharstringSubtypeTreeElement(*(p_st->universal_charstring_st));
    } else {
      universal_charstring_st = new UniversalCharstringSubtypeTreeElement(
        UniversalCharstringSubtypeTreeElement::ET_UNION,
        universal_charstring_st,
        new UniversalCharstringSubtypeTreeElement(*(p_st->universal_charstring_st)));
    }
    break;
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    if (p_st->value_st==NULL) return false;
    if (value_st==NULL) value_st = new ValueListConstraint(*(p_st->value_st));
    else *value_st = *value_st + *(p_st->value_st);
    break;
  case ST_RECORDOF:
  case ST_SETOF:
    if (p_st->recof_st==NULL) return false;
    if (recof_st==NULL) recof_st = new RecofConstraint(*(p_st->recof_st));
    else *recof_st = *recof_st + *(p_st->recof_st);
    break;
  default:
    FATAL_ERROR("SubType::add_ttcn_type_list_subtype()");
  }
  return true;
}


bool SubType::add_parent_subtype(SubType* st)
{
  if (st==NULL) FATAL_ERROR("SubType::add_parent_subtype()");
  if (my_parents.has_key(st)) return true; // it was already successfully added -> ignore
  ReferenceChain refch(my_owner, "While checking circular type references in subtype definitions");
  refch.add(my_owner->get_fullname()); // current type
  // recursive check for all parents of referenced type
  if (!st->chk_recursion(refch)) return false;
  // if no recursion was detected then add the referenced type as parent
  my_parents.add(st,NULL);
  return true;
}

bool SubType::chk_recursion(ReferenceChain& refch)
{
  if (!refch.add(my_owner->get_fullname())) return false; // try the referenced type
  for (size_t i = 0; i < my_parents.size(); i++) {
    refch.mark_state();
    if (!my_parents.get_nth_key(i)->chk_recursion(refch)) return false;
    refch.prev_state();
  }
  return true;
}

bool SubType::add_ttcn_single(Value *val, size_t restriction_index)
{
  val->set_my_scope(my_owner->get_my_scope());
  val->set_my_governor(my_owner);
  val->set_fullname(my_owner->get_fullname()+".<single_restriction_"+Int2string(restriction_index) + ">");
  my_owner->chk_this_value_ref(val);

  // check if this is type reference, if not then fall through
  if (val->get_valuetype()==Value::V_REFD) {
    Reference* ref = val->get_reference();
    Assignment *ass = ref->get_refd_assignment();
    if (ass==NULL) return false; // defintion was not found, error was reported
    if (ass->get_asstype()==Assignment::A_TYPE) {
      Type* t = ass->get_Type();
      t->chk();
      if (t->get_typetype()==Type::T_ERROR) return false;
      // if there were subreferences then get the referenced field's type
      if (ref->get_subrefs()) {
        t = t->get_field_type(ref->get_subrefs(), Type::EXPECTED_CONSTANT);
        if ( (t==NULL) || (t->get_typetype()==Type::T_ERROR) ) return false;
        t->chk();
        if (t->get_typetype()==Type::T_ERROR) return false;
      }
      if (!t->is_identical(my_owner)) {
        val->error("Reference `%s' must refer to a type which has the same root type as this type",
                   val->get_reference()->get_dispname().c_str());
        return false;
      }
      // check subtype of referenced type
      SubType* t_st = t->get_sub_type();
      if (t_st==NULL) {
        val->error("Type referenced by `%s' does not have a subtype",
                   val->get_reference()->get_dispname().c_str());
        return false;
      }

      // check circular subtype reference
      if (!add_parent_subtype(t_st)) return false;

      if (t_st->get_subtypetype()==ST_ERROR) return false;
      if (t_st->get_subtypetype()!=subtype) FATAL_ERROR("SubType::add_ttcn_single()");
      // add the subtype as union
      bool added = add_ttcn_type_list_subtype(t_st);
      if (!added) {
        val->error("Type referenced by `%s' does not have a subtype",
                   val->get_reference()->get_dispname().c_str());
      }
      return added;
    }
  }

  my_owner->chk_this_value(val, 0, Type::EXPECTED_CONSTANT,
    INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, NO_SUB_CHK);

  Value *v=val->get_value_refd_last();

  switch (v->get_valuetype()) {
  case Value::V_INT:
    if (subtype!=ST_INTEGER) FATAL_ERROR("SubType::add_ttcn_single()");
    if (integer_st==NULL) integer_st = new IntegerRangeListConstraint(int_limit_t(*(v->get_val_Int())));
    else *integer_st = *integer_st + IntegerRangeListConstraint(int_limit_t(*(v->get_val_Int())));
    break;
  case Value::V_REAL: {
    if (subtype!=ST_FLOAT) FATAL_ERROR("SubType::add_ttcn_single()");
    ttcn3float r = v->get_val_Real();
    if (r!=r) {
      if (float_st==NULL) float_st = new RealRangeListConstraint(true);
      else *float_st = *float_st + RealRangeListConstraint(true);
    } else {
      if (float_st==NULL) float_st = new RealRangeListConstraint(real_limit_t(r));
      else *float_st = *float_st + RealRangeListConstraint(real_limit_t(r));
    }
  } break;
  case Value::V_BOOL:
    if (subtype!=ST_BOOLEAN) FATAL_ERROR("SubType::add_ttcn_single()");
    if (boolean_st==NULL) boolean_st = new BooleanListConstraint(v->get_val_bool());
    else *boolean_st = *boolean_st + BooleanListConstraint(v->get_val_bool());
    break;
  case Value::V_VERDICT: {
    if (subtype!=ST_VERDICTTYPE) FATAL_ERROR("SubType::add_ttcn_single()");
    VerdicttypeListConstraint::verdicttype_constraint_t vtc;
    switch (v->get_val_verdict()) {
    case Value::Verdict_NONE: vtc = VerdicttypeListConstraint::VC_NONE; break;
    case Value::Verdict_PASS: vtc = VerdicttypeListConstraint::VC_PASS; break;
    case Value::Verdict_INCONC: vtc = VerdicttypeListConstraint::VC_INCONC; break;
    case Value::Verdict_FAIL: vtc = VerdicttypeListConstraint::VC_FAIL; break;
    case Value::Verdict_ERROR: vtc = VerdicttypeListConstraint::VC_ERROR; break;
    default: FATAL_ERROR("SubType::add_ttcn_single()");
    }
    if (verdict_st==NULL) verdict_st = new VerdicttypeListConstraint(vtc);
    else *verdict_st = *verdict_st + VerdicttypeListConstraint(vtc);
  } break;
  case Value::V_OID:
    if (v->has_oid_error()) return false;
    if (subtype!=ST_OBJID) FATAL_ERROR("SubType::add_ttcn_single()");
    if (value_st==NULL) value_st = new ValueListConstraint(v);
    else *value_st = *value_st + ValueListConstraint(v);
    break;
  case Value::V_BSTR:
    if (subtype!=ST_BITSTRING) FATAL_ERROR("SubType::add_ttcn_single()");
    if (bitstring_st==NULL) bitstring_st = new BitstringConstraint(v->get_val_str());
    else *bitstring_st = *bitstring_st + BitstringConstraint(v->get_val_str());
    break;
  case Value::V_HSTR:
    if (subtype!=ST_HEXSTRING) FATAL_ERROR("SubType::add_ttcn_single()");
    if (hexstring_st==NULL) hexstring_st = new HexstringConstraint(v->get_val_str());
    else *hexstring_st = *hexstring_st + HexstringConstraint(v->get_val_str());
    break;
  case Value::V_OSTR:
    if (subtype!=ST_OCTETSTRING) FATAL_ERROR("SubType::add_ttcn_single()");
    if (octetstring_st==NULL) octetstring_st = new OctetstringConstraint(v->get_val_str());
    else *octetstring_st = *octetstring_st + OctetstringConstraint(v->get_val_str());
    break;
  case Value::V_CSTR: {
    if (subtype!=ST_CHARSTRING) FATAL_ERROR("SubType::add_ttcn_single()");
    CharstringSubtypeTreeElement* cst_elem = new CharstringSubtypeTreeElement(StringValueConstraint<string>(v->get_val_str()));
    if (charstring_st==NULL) charstring_st = cst_elem;
    else charstring_st = new CharstringSubtypeTreeElement(CharstringSubtypeTreeElement::ET_UNION, charstring_st, cst_elem);
  } break;
  case Value::V_USTR: {
    if (subtype!=ST_UNIVERSAL_CHARSTRING) FATAL_ERROR("SubType::add_ttcn_single()");
    UniversalCharstringSubtypeTreeElement* ucst_elem = new UniversalCharstringSubtypeTreeElement(StringValueConstraint<ustring>(v->get_val_ustr()));
    if (universal_charstring_st==NULL) universal_charstring_st = ucst_elem;
    else universal_charstring_st = new UniversalCharstringSubtypeTreeElement(UniversalCharstringSubtypeTreeElement::ET_UNION, universal_charstring_st, ucst_elem);
  } break;
  case Value::V_ENUM:
  case Value::V_NULL: // FIXME: should go to ST_NULL
    if (subtype!=ST_ENUM) FATAL_ERROR("SubType::add_ttcn_single()");
    add_ttcn_value(v);
    break;
  case Value::V_CHOICE:
    if (subtype!=ST_UNION) FATAL_ERROR("SubType::add_ttcn_single()");
    add_ttcn_value(v);
    break;
  case Value::V_SEQ:
    if (subtype!=ST_RECORD) FATAL_ERROR("SubType::add_ttcn_single()");
    add_ttcn_value(v);
    break;
  case Value::V_SET:
    if (subtype!=ST_SET) FATAL_ERROR("SubType::add_ttcn_single()");
    add_ttcn_value(v);
    break;
  case Value::V_FUNCTION:
    if (subtype!=ST_FUNCTION) FATAL_ERROR("SubType::add_ttcn_single()");
    add_ttcn_value(v);
    break;
  case Value::V_ALTSTEP:
    if (subtype!=ST_ALTSTEP) FATAL_ERROR("SubType::add_ttcn_single()");
    add_ttcn_value(v);
    break;
  case Value::V_TESTCASE:
    if (subtype!=ST_TESTCASE) FATAL_ERROR("SubType::add_ttcn_single()");
    add_ttcn_value(v);
    break;
  case Value::V_SEQOF:
    if (subtype!=ST_RECORDOF) FATAL_ERROR("SubType::add_ttcn_single()");
    add_ttcn_recof(v);
    break;
  case Value::V_SETOF:
    if (subtype!=ST_SETOF) FATAL_ERROR("SubType::add_ttcn_single()");
    add_ttcn_recof(v);
    break;
  case Value::V_ERROR:
    return false;
  default:
    return false;
  }
  return true;
}

bool SubType::add_ttcn_range(Value *min, bool min_exclusive,
  Value *max, bool max_exclusive, size_t restriction_index, bool has_other)
{
  switch (subtype) {
  case ST_INTEGER:
  case ST_FLOAT:
  case ST_CHARSTRING:
  case ST_UNIVERSAL_CHARSTRING:
    break;
  default:
    my_owner->error("Range subtyping is not allowed for type `%s'",
      my_owner->get_typename().c_str());
    return false;
  }

  Value *vmin,*vmax;
  if (min==NULL) vmin=NULL;
  else {
    min->set_my_scope(my_owner->get_my_scope());
    min->set_fullname(my_owner->get_fullname()+".<range_restriction_"+Int2string(restriction_index)+"_lower>");
    my_owner->chk_this_value_ref(min);
    my_owner->chk_this_value(min, 0, Type::EXPECTED_CONSTANT,
      INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, NO_SUB_CHK);
    vmin=min->get_value_refd_last();
  }
  if (max==NULL) vmax=NULL;
  else {
    max->set_my_scope(my_owner->get_my_scope());
    max->set_fullname(my_owner->get_fullname()+".<range_restriction_"+Int2string(restriction_index)+"_upper>");
    my_owner->chk_this_value_ref(max);
    my_owner->chk_this_value(max, 0, Type::EXPECTED_CONSTANT,
      INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, NO_SUB_CHK);
    vmax=max->get_value_refd_last();
  }

  if ( (vmin!=NULL) && (vmax!=NULL) && (vmin->get_valuetype()!=vmax->get_valuetype()) ) return false;

  switch (subtype) {
  case ST_INTEGER: {
    if (((vmin!=NULL) && (vmin->get_valuetype()!=Value::V_INT)) ||
        ((vmax!=NULL) && (vmax->get_valuetype()!=Value::V_INT))) return false;
    int_limit_t min_limit = (vmin!=NULL) ? int_limit_t(*(vmin->get_val_Int())) : int_limit_t::minimum;
    if (min_exclusive) {
      if (min_limit==int_limit_t::minimum) {
        my_owner->error("invalid lower boundary, -infinity cannot be excluded from an integer subtype range");
        return false;
      } else {
        if (min_limit==int_limit_t::maximum) {
          my_owner->error("!infinity is not a valid lower boundary");
          return false;
        }
        min_limit = min_limit.next();
      }
    }
    int_limit_t max_limit = (vmax!=NULL) ? int_limit_t(*(vmax->get_val_Int())) : int_limit_t::maximum;
    if (max_exclusive) {
      if (max_limit==int_limit_t::maximum) {
        my_owner->error("invalid upper boundary, infinity cannot be excluded from an integer subtype range");
        return false;
      } else {
        if (max_limit==int_limit_t::minimum) {
          my_owner->error("!-infinity is not a valid upper boundary");
          return false;
        }
        max_limit = max_limit.previous();
      }
    }
    if (max_limit<min_limit) {
      my_owner->error("lower boundary is bigger than upper boundary in integer subtype range");
      return false;
    }
    if (integer_st==NULL) integer_st = new IntegerRangeListConstraint(min_limit, max_limit);
    else *integer_st = *integer_st + IntegerRangeListConstraint(min_limit, max_limit);
  } break;
  case ST_FLOAT: {
    if (((vmin!=NULL) && (vmin->get_valuetype()!=Value::V_REAL)) ||
        ((vmax!=NULL) && (vmax->get_valuetype()!=Value::V_REAL))) return false;
    if ((vmin!=NULL) && (vmin->get_val_Real()!=vmin->get_val_Real())) {
      my_owner->error("lower boundary cannot be not_a_number in float subtype range");
      return false;
    }
    if ((vmax!=NULL) && (vmax->get_val_Real()!=vmax->get_val_Real())) {
      my_owner->error("upper boundary cannot be not_a_number in float subtype range");
      return false;
    }
    real_limit_t min_limit = (vmin!=NULL) ? real_limit_t(vmin->get_val_Real()) : real_limit_t::minimum;
    if (min_exclusive) {
      if (min_limit==real_limit_t::maximum) {
        my_owner->error("!infinity is not a valid lower boundary");
        return false;
      }
      min_limit = min_limit.next();
    }
    real_limit_t max_limit = (vmax!=NULL) ? real_limit_t(vmax->get_val_Real()) : real_limit_t::maximum;
    if (max_exclusive) {
      if (max_limit==real_limit_t::minimum) {
        my_owner->error("!-infinity is not a valid upper boundary");
        return false;
      }
      max_limit = max_limit.previous();
    }
    if (max_limit<min_limit) {
      my_owner->error("lower boundary is bigger than upper boundary in float subtype range");
      return false;
    }
    if (float_st==NULL) float_st = new RealRangeListConstraint(min_limit, max_limit);
    else *float_st = *float_st + RealRangeListConstraint(min_limit, max_limit);
  } break;
  case ST_CHARSTRING: {
    if (((vmin!=NULL) && (vmin->get_valuetype()!=Value::V_CSTR)) ||
        ((vmax!=NULL) && (vmax->get_valuetype()!=Value::V_CSTR))) return false;
    if ((vmin==NULL)&&(vmax==NULL)) {
      my_owner->error("a range subtype of a charstring cannot be (-infinity..infinity)");
      return false;
    }
    if (vmin==NULL) {
      my_owner->error("lower boundary of a charstring subtype range cannot be -infinity");
      return false;
    }
    if (vmax==NULL) {
      my_owner->error("upper boundary of a charstring subtype range cannot be infinity");
      return false;
    }
    if (vmin->get_val_str().size()!=1) {
      min->error("lower boundary of charstring subtype range must be a single element string");
      return false;
    }
    if (vmax->get_val_str().size()!=1) {
      max->error("upper boundary of charstring subtype range must be a single element string");
      return false;
    }
    if (!char_limit_t::is_valid_value(*vmin->get_val_str().c_str())) {
      min->error("lower boundary of charstring subtype range is an invalid char");
      return false;
    }
    if (!char_limit_t::is_valid_value(*vmax->get_val_str().c_str())) {
      max->error("upper boundary of charstring subtype range is an invalid char");
      return false;
    }
    char_limit_t min_limit(*vmin->get_val_str().c_str()), max_limit(*vmax->get_val_str().c_str());
    if (min_exclusive) {
      if (min_limit==char_limit_t::maximum) {
        min->error("exclusive lower boundary is not a legal charstring character");
        return false;
      }
      min_limit = min_limit.next();
    }
    if (max_exclusive) {
      if (max_limit==char_limit_t::minimum) {
        max->error("exclusive upper boundary is not a legal charstring character");
        return false;
      }
      max_limit = max_limit.previous();
    }
    if (max_limit<min_limit) {
      my_owner->error("lower boundary is bigger than upper boundary in charstring subtype range");
      return false;
    }
    if (charstring_st==NULL) charstring_st = new CharstringSubtypeTreeElement(CharRangeListConstraint(min_limit,max_limit), false);
    else {
      if (!has_other) { // union in char context can be done only with range constraints
        charstring_st->set_char_context(true);
        charstring_st = new CharstringSubtypeTreeElement(CharstringSubtypeTreeElement::ET_UNION,
          charstring_st,
          new CharstringSubtypeTreeElement(CharRangeListConstraint(min_limit,max_limit), true));
        charstring_st->set_char_context(false);
      } else {
        // ignore it, error reported elsewhere
        return false;
      }
    }
  } break;
  case ST_UNIVERSAL_CHARSTRING: {
    if (((vmin!=NULL) && (vmin->get_valuetype()!=Value::V_USTR)) ||
        ((vmax!=NULL) && (vmax->get_valuetype()!=Value::V_USTR))) return false;
    if ((vmin==NULL)&&(vmax==NULL)) {
      my_owner->error("a range subtype of a universal charstring cannot be (-infinity..infinity)");
      return false;
    }
    if (vmin==NULL) {
      my_owner->error("lower boundary of a universal charstring subtype range cannot be -infinity");
      return false;
    }
    if (vmax==NULL) {
      my_owner->error("upper boundary of a universal charstring subtype range cannot be infinity");
      return false;
    }
    if (vmin->get_val_ustr().size()!=1) {
      min->error("lower boundary of universal charstring subtype range must be a single element string");
      return false;
    }
    if (vmax->get_val_ustr().size()!=1) {
      max->error("upper boundary of universal charstring subtype range must be a single element string");
      return false;
    }
    if (!universal_char_limit_t::is_valid_value(*vmin->get_val_ustr().u_str())) {
      min->error("lower boundary of universal charstring subtype range is an invalid char");
      return false;
    }
    if (!universal_char_limit_t::is_valid_value(*vmax->get_val_ustr().u_str())) {
      max->error("upper boundary of universal charstring subtype range is an invalid char");
      return false;
    }
    universal_char_limit_t min_limit(*vmin->get_val_ustr().u_str()), max_limit(*vmax->get_val_ustr().u_str());
    if (min_exclusive) {
      if (min_limit==universal_char_limit_t::maximum) {
        min->error("exclusive lower boundary is not a legal universal charstring character");
        return false;
      }
      min_limit = min_limit.next();
    }
    if (max_exclusive) {
      if (max_limit==universal_char_limit_t::minimum) {
        max->error("exclusive upper boundary is not a legal universal charstring character");
        return false;
      }
      max_limit = max_limit.previous();
    }
    if (max_limit<min_limit) {
      my_owner->error("lower boundary is bigger than upper boundary in universal charstring subtype range");
      return false;
    }

    if (universal_charstring_st==NULL) universal_charstring_st = new UniversalCharstringSubtypeTreeElement(UniversalCharRangeListConstraint(min_limit,max_limit), false);
    else {
      if (!has_other) { // union in char context can be done only with range constraints
        universal_charstring_st->set_char_context(true);
        universal_charstring_st = new UniversalCharstringSubtypeTreeElement(UniversalCharstringSubtypeTreeElement::ET_UNION,
          universal_charstring_st,
          new UniversalCharstringSubtypeTreeElement(UniversalCharRangeListConstraint(min_limit,max_limit), true));
        universal_charstring_st->set_char_context(false);
      } else {
        // ignore it, error reported elsewhere
        return false;
      }
    }
  } break;
  default:
    FATAL_ERROR("SubType::add_ttcn_range()");
  }
  return true;
}

bool SubType::set_ttcn_length(const size_limit_t& min, const size_limit_t& max)
{
  switch (subtype) {
  case ST_BITSTRING: {
    if (bitstring_st==NULL) bitstring_st = new BitstringConstraint(min,max);
    else *bitstring_st = *bitstring_st * BitstringConstraint(min,max);
  } break;
  case ST_HEXSTRING: {
    if (hexstring_st==NULL) hexstring_st = new HexstringConstraint(min,max);
    else *hexstring_st = *hexstring_st * HexstringConstraint(min,max);
  } break;
  case ST_OCTETSTRING: {
    if (octetstring_st==NULL) octetstring_st = new OctetstringConstraint(min,max);
    else *octetstring_st = *octetstring_st * OctetstringConstraint(min,max);
  } break;
  case ST_CHARSTRING: {
    CharstringSubtypeTreeElement* cst_elem = new CharstringSubtypeTreeElement(SizeRangeListConstraint(min,max));
    if (charstring_st==NULL) {
      charstring_st = cst_elem;
    } else {
      charstring_st = new CharstringSubtypeTreeElement(CharstringSubtypeTreeElement::ET_INTERSECTION, charstring_st, cst_elem);
    }
  } break;
  case ST_UNIVERSAL_CHARSTRING: {
    UniversalCharstringSubtypeTreeElement* ucst_elem = new UniversalCharstringSubtypeTreeElement(SizeRangeListConstraint(min,max));
    if (universal_charstring_st==NULL) {
      universal_charstring_st = ucst_elem;
    } else {
      universal_charstring_st = new UniversalCharstringSubtypeTreeElement(UniversalCharstringSubtypeTreeElement::ET_INTERSECTION, universal_charstring_st, ucst_elem);
    }
  } break;
  case ST_RECORDOF:
  case ST_SETOF: {
    if (recof_st==NULL) recof_st = new RecofConstraint(min,max);
    else *recof_st = *recof_st * RecofConstraint(min,max);
  } break;
  default:
    my_owner->error("Length subtyping is not allowed for type `%s'",
                    my_owner->get_typename().c_str());
    return false;
  }
  if (length_restriction==NULL) length_restriction = new SizeRangeListConstraint(min,max);
  else *length_restriction = *length_restriction * SizeRangeListConstraint(min,max);
  return true;
}

void SubType::chk_boundary_valid(Value* boundary, Int max_value, const char* boundary_name)
{
  const int_val_t *int_val = boundary->get_val_Int();
  if (*int_val > int_val_t(max_value)) {
    boundary->error("The %s should be less than `%s' instead of `%s'",
      boundary_name,
      int_val_t(max_value).t_str().c_str(),
      int_val->t_str().c_str());
    boundary->set_valuetype(Value::V_ERROR);
  }
}

bool SubType::add_ttcn_length(Ttcn::LengthRestriction *lr, size_t restriction_index)
{
  string s;
  lr->append_stringRepr(s);
  Value *lower=NULL;
  lr->set_my_scope(my_owner->get_my_scope());
  lr->set_fullname(my_owner->get_fullname()+".<length_restriction_"+Int2string(restriction_index)+">");
  lr->chk(Type::EXPECTED_CONSTANT);
  lower = lr->get_is_range() ? lr->get_lower_value() : lr->get_single_value();
  if (!lower->get_my_scope()) FATAL_ERROR("no scope");
  if (lower->get_valuetype() != Value::V_INT) return false;
  if (lr->get_is_range()) {
    Value *upper = lr->get_upper_value();
    if (upper) {//HAS_UPPER
      if (upper->get_valuetype()!=Value::V_INT) return false;
      if (!upper->get_my_scope()) upper->set_my_scope(my_owner->get_my_scope());
      chk_boundary_valid(upper, INT_MAX, "upper boundary");
      if (upper->get_valuetype()!=Value::V_INT) return false;
      return set_ttcn_length(size_limit_t((size_t)lower->get_val_Int()->get_val()),
                             size_limit_t((size_t)upper->get_val_Int()->get_val()));
    } else {//INFINITY:
      chk_boundary_valid(lower, INT_MAX, "lower boundary");
      if (lower->get_valuetype()!=Value::V_INT) return false;
      return set_ttcn_length(size_limit_t((size_t)lower->get_val_Int()->get_val()),
                             size_limit_t(size_limit_t::INFINITE_SIZE));
    }
  }
  else {//SINGLE:
    chk_boundary_valid(lower, INT_MAX, "length restriction value");
    if (lower->get_valuetype()!=Value::V_INT) return false;
    return set_ttcn_length(size_limit_t((size_t)lower->get_val_Int()->get_val()),
                           size_limit_t((size_t)lower->get_val_Int()->get_val()));
  }
}

bool SubType::add_ttcn_pattern(Ttcn::PatternString* pattern, size_t restriction_index)
{
  pattern->set_my_scope(my_owner->get_my_scope());
  pattern->set_fullname(my_owner->get_fullname()+".<pattern_restriction_"+Int2string(restriction_index) + ">");
  switch (subtype) {
  case ST_CHARSTRING: {
    Error_Context cntxt(my_owner, "In character string pattern");
    pattern->chk_refs(Type::EXPECTED_CONSTANT);
	  pattern->join_strings();
  	if (!pattern->has_refs()) { // if chk_refs didn't remove all references then ignore
      pattern->chk_pattern();
      CharstringSubtypeTreeElement* cst_elem = new CharstringSubtypeTreeElement(StringPatternConstraint(pattern));
      if (charstring_st==NULL) {
        charstring_st = cst_elem;
      } else {
        charstring_st = new CharstringSubtypeTreeElement(CharstringSubtypeTreeElement::ET_INTERSECTION, charstring_st, cst_elem);
      }
    }
  } break;
  case ST_UNIVERSAL_CHARSTRING: {
    Error_Context cntxt(my_owner, "In universal string pattern");
    pattern->set_pattern_type(Ttcn::PatternString::USTR_PATTERN);
    pattern->chk_refs(Type::EXPECTED_CONSTANT);
	  pattern->join_strings();
  	if (!pattern->has_refs()) { // if chk_refs didn't remove all references then ignore
      pattern->chk_pattern();
      UniversalCharstringSubtypeTreeElement* ucst_elem = new UniversalCharstringSubtypeTreeElement(StringPatternConstraint(pattern));
      if (universal_charstring_st==NULL) {
        universal_charstring_st = ucst_elem;
      } else {
        universal_charstring_st = new UniversalCharstringSubtypeTreeElement(UniversalCharstringSubtypeTreeElement::ET_INTERSECTION, universal_charstring_st, ucst_elem);
      }
    }
  } break;
  default:
    my_owner->error("Pattern subtyping of type `%s' is not allowed", my_owner->get_typename().c_str());
    return false;
  }
  return true;
}

void SubType::print_full_warning() const
{
  my_owner->warning("The subtype of type `%s' is a full set, "
    "it does not constrain the root type.", my_owner->get_typename().c_str());
}

vector<SubTypeParse> * SubType::get_subtype_parsed() const {
  return parsed;
}

void SubType::chk()
{
  if ((checked!=STC_NO) || (subtype==ST_ERROR)) FATAL_ERROR("SubType::chk()");
  checked = STC_CHECKING;

  // check for circular subtype reference
  if (parent_subtype && !add_parent_subtype(parent_subtype)) {
    set_to_error();
    checked = STC_YES;
    return;
  }

  if (parsed) { // has TTCN-3 subtype constraint
    size_t added_count = 0;
    bool has_single = false, has_range = false,
         has_length = false, has_pattern = false;
    for (size_t i = 0; i < parsed->size(); i++) {
      bool added = false;
      SubTypeParse *parse = (*parsed)[i];
      switch (parse->get_selection()) {
      case SubTypeParse::STP_SINGLE:
        has_single = true;
        added = add_ttcn_single(parse->Single(),i);
        break;
      case SubTypeParse::STP_RANGE:
        has_range = true;
        added = add_ttcn_range(parse->Min(), parse->MinExclusive(),
                               parse->Max(), parse->MaxExclusive(), i,
                               has_single || has_length || has_pattern);
        break;
      case SubTypeParse::STP_LENGTH:
        has_length = true;
        added = add_ttcn_length(parse->Length(),i);
        break;
      case SubTypeParse::STP_PATTERN:
        has_pattern = true;
        added = add_ttcn_pattern(parse->Pattern(),i);
        break;
      default:
        FATAL_ERROR("SubType::chk(): invalid SubTypeParse selection");
      } // switch
      if (added) added_count++;
    }//for
    switch (subtype) {
    case ST_CHARSTRING:
    case ST_UNIVERSAL_CHARSTRING:
      if (has_single && has_range) {
        my_owner->error(
          "Mixing of value list and range subtyping is not allowed for type `%s'",
          my_owner->get_typename().c_str());
        set_to_error();
        checked = STC_YES;
        return;
      }
      break;
    default:
      // in other cases mixing of different restrictions (which are legal for
      // this type) is properly regulated by the TTCN-3 BNF itself
      break;
    }
    if (added_count<parsed->size()) {
      set_to_error();
      checked = STC_YES;
      return;
    }
    if (subtype==ST_ERROR) { checked = STC_YES; return; }

    if (parent_subtype) {
      if (is_subset(parent_subtype->get_root())==TFALSE) {
        my_owner->error("The subtype restriction is not a subset of the restriction on the parent type. "
          "Subtype %s is not subset of subtype %s", to_string().c_str(), parent_subtype->get_root()->to_string().c_str());
        set_to_error();
        checked = STC_YES;
        return;
      }
      intersection(parent_subtype->get_root());
    }
  } else if (asn_constraints) { // has ASN.1 subtype constraint
    SubtypeConstraint* asn_parent_subtype = NULL;
    if (parent_subtype) {
      // the type constraint of the ASN.1 type is already in the parent_subtype,
      // don't add it multiple times
      asn_parent_subtype = parent_subtype->get_root();
    } else {
      asn_parent_subtype = get_asn_type_constraint(my_owner);
    }
    asn_constraints->chk(asn_parent_subtype);
    root = asn_constraints->get_subtype();
    extendable = asn_constraints->is_extendable();
    extension  = asn_constraints->get_extension();
    // the TTCN-3 subtype will be the union of the root and extension parts
    // the ETSI ES 201 873-7 V4.1.2 (2009-07) document says to "ignore any extension markers"
    // but titan now works this way :)
    if (root) copy(root);
    if (extension) union_(extension);
  } else { // no constraints on this type -> this is an alias type, just copy the subtype from the other
    if (parent_subtype) {
      root = parent_subtype->root;
      extendable = parent_subtype->extendable;
      extension = parent_subtype->extension;
      copy(parent_subtype);
    } else {
      SubtypeConstraint* asn_parent_subtype = get_asn_type_constraint(my_owner);
      if (asn_parent_subtype) copy(asn_parent_subtype);
    }
  }

  // check if subtype is valid: it must not be an empty set (is_empty==TTRUE)
  // issue warning if subtype is given but is full set (is_full==TTRUE)
  // ignore cases of TUNKNOWN when compiler can't figure out if the aggregate
  // set is empty or full
  switch (subtype) {
  case ST_INTEGER:
    if (integer_st!=NULL) {
      if (integer_st->is_empty()==TTRUE) goto empty_error;
      if (integer_st->is_full()==TTRUE) {
        print_full_warning();
        delete integer_st;
        integer_st = NULL;
      }
    }
    break;
  case ST_FLOAT:
    if (float_st!=NULL) {
      if (float_st->is_empty()==TTRUE) goto empty_error;
      if (float_st->is_full()==TTRUE) {
        print_full_warning();
        delete float_st;
        float_st = NULL;
      }
    }
    break;
  case ST_BOOLEAN:
    if (boolean_st!=NULL) {
      if (boolean_st->is_empty()==TTRUE) goto empty_error;
      if (boolean_st->is_full()==TTRUE) {
        print_full_warning();
        delete boolean_st;
        boolean_st = NULL;
      }
    }
    break;
  case ST_VERDICTTYPE:
    if (verdict_st!=NULL) {
      if (verdict_st->is_empty()==TTRUE) goto empty_error;
      if (verdict_st->is_full()==TTRUE) {
        print_full_warning();
        delete verdict_st;
        verdict_st = NULL;
      }
    }
    break;
  case ST_BITSTRING:
    if (bitstring_st!=NULL) {
      if (bitstring_st->is_empty()==TTRUE) goto empty_error;
      if (bitstring_st->is_full()==TTRUE) {
        print_full_warning();
        delete bitstring_st;
        bitstring_st = NULL;
      }
    }
    break;
  case ST_HEXSTRING:
    if (hexstring_st!=NULL) {
      if (hexstring_st->is_empty()==TTRUE) goto empty_error;
      if (hexstring_st->is_full()==TTRUE) {
        print_full_warning();
        delete hexstring_st;
        hexstring_st = NULL;
      }
    }
    break;
  case ST_OCTETSTRING:
    if (octetstring_st!=NULL) {
      if (octetstring_st->is_empty()==TTRUE) goto empty_error;
      if (octetstring_st->is_full()==TTRUE) {
        print_full_warning();
        delete octetstring_st;
        octetstring_st = NULL;
      }
    }
    break;
  case ST_CHARSTRING:
    if (charstring_st!=NULL) {
      if (charstring_st->is_empty()==TTRUE) goto empty_error;
      if (charstring_st->is_full()==TTRUE) {
        print_full_warning();
        delete charstring_st;
        charstring_st = NULL;
      }
    }
    break;
  case ST_UNIVERSAL_CHARSTRING:
    if (universal_charstring_st!=NULL) {
      if (universal_charstring_st->is_empty()==TTRUE) goto empty_error;
      if (universal_charstring_st->is_full()==TTRUE) {
        print_full_warning();
        delete universal_charstring_st;
        universal_charstring_st = NULL;
      }
    }
    break;
  case ST_OBJID:
  case ST_RECORD:
  case ST_SET:
  case ST_ENUM:
  case ST_UNION:
  case ST_FUNCTION:
  case ST_ALTSTEP:
  case ST_TESTCASE:
    if (value_st!=NULL) {
      if (value_st->is_empty()==TTRUE) goto empty_error;
      if (value_st->is_full()==TTRUE) {
        print_full_warning();
        delete value_st;
        value_st = NULL;
      }
    }
    break;
  case ST_RECORDOF:
  case ST_SETOF:
    if (recof_st!=NULL) {
      if (recof_st->is_empty()==TTRUE) goto empty_error;
      if (recof_st->is_full()==TTRUE) {
        print_full_warning();
        delete recof_st;
        recof_st = NULL;
      }
    }
    break;
  default:
    FATAL_ERROR("SubType::chk()");
  }
  if ((length_restriction!=NULL) && (length_restriction->is_full()==TTRUE)) {
    delete length_restriction;
    length_restriction = NULL;
  }
  checked = STC_YES;
  return;

empty_error:
  my_owner->error("The subtype is an empty set");
  set_to_error();
  checked = STC_YES;
  return;
}

void SubType::dump(unsigned level) const
{
  string str = to_string();
  if (str.size()>0) DEBUG(level, "restriction(s): %s", str.c_str());
}

Int SubType::get_length_restriction() const
{
  if (checked!=STC_YES) FATAL_ERROR("SubType::get_length_restriction()");
  if (parsed==NULL) return -1; // only own length restriction counts
  if (length_restriction==NULL) return -1;
  if (TTRUE == length_restriction->is_empty()) return -1;
  return ( (length_restriction->get_minimal()==length_restriction->get_maximal()) ?
           (Int)(length_restriction->get_minimal().get_size()) :
           -1 );
}

bool SubType::zero_length_allowed() const
{
  if (checked!=STC_YES) FATAL_ERROR("SubType::zero_length_allowed()");
  if (parsed==NULL) return true; // only own length restriction counts
  if (length_restriction==NULL) return true;
  return length_restriction->is_element(size_limit_t(0));
}

bool SubType::length_allowed(size_t len) const
{
  if (checked != STC_YES) {
    FATAL_ERROR("SubType::length_allowed()");
  }
  if (length_restriction == NULL) {
    return true;
  }
  return length_restriction->is_element(size_limit_t(len));
}

string SubType::to_string() const
{
  if (root) {
    string ret_val(root->to_string());
    if (extendable) ret_val += ", ...";
    if (extension) {
      ret_val += ", ";
      ret_val += extension->to_string();
    }
    return ret_val;
  }
  return SubtypeConstraint::to_string();
}

////////////////////////////////////////////////////////////////////////////////

void SubType::generate_code(output_struct &)
{
  if (checked!=STC_YES) FATAL_ERROR("SubType::generate_code()");
}

void SubType::generate_json_schema(JSON_Tokenizer& json,
                                   bool allow_special_float /* = true */)
{
  bool has_value_list = false;
  size_t nof_ranges = 0;
  for (size_t i = 0; i < parsed->size(); ++i) {
    SubTypeParse *parse = (*parsed)[i];
    switch (parse->get_selection()) {
    case SubTypeParse::STP_SINGLE:
      // single values will be added later, all at once
      has_value_list = true;
      break;
    case SubTypeParse::STP_RANGE:
      ++nof_ranges;
      break;
    case SubTypeParse::STP_LENGTH: {
      Ttcn::LengthRestriction* len_res = parse->Length();
      Value* min_val = len_res->get_is_range() ? len_res->get_lower_value() :
        len_res->get_single_value();
      Value* max_val = len_res->get_is_range() ? len_res->get_upper_value() :
        len_res->get_single_value();
      const char* json_min = NULL;
      const char* json_max = NULL;
      switch (subtype) {
      case ST_RECORDOF:
      case ST_SETOF:
        // use minItems and maxItems for record of/set of
        json_min = "minItems";
        json_max = "maxItems";
        break;
      case ST_BITSTRING:
      case ST_HEXSTRING:
      case ST_OCTETSTRING:
      case ST_CHARSTRING:
      case ST_UNIVERSAL_CHARSTRING:
        // use minLength and maxLength for string types
        json_min = "minLength";
        json_max = "maxLength";
        break;
      default:
        FATAL_ERROR("SubType::generate_json_schema - length %d", subtype);
      }
      json.put_next_token(JSON_TOKEN_NAME, json_min);
      min_val->generate_json_value(json);
      if (max_val != NULL) {
        json.put_next_token(JSON_TOKEN_NAME, json_max);
        max_val->generate_json_value(json);
      }
      break; }
    case SubTypeParse::STP_PATTERN: {
      json.put_next_token(JSON_TOKEN_NAME, "pattern");
      char* json_pattern = parse->Pattern()->convert_to_json();
      json.put_next_token(JSON_TOKEN_STRING, json_pattern);
      Free(json_pattern);
      break; }
    default:
      break;
    }
  }

  bool need_anyOf = (subtype == ST_INTEGER || subtype == ST_FLOAT) &&
    (nof_ranges + (has_value_list ? 1 : 0) > 1);
  if (need_anyOf) {
    // there are multiple value range/value list restrictions,
    // they need to be grouped in an 'anyOf' structure
    json.put_next_token(JSON_TOKEN_NAME, "anyOf");
    json.put_next_token(JSON_TOKEN_ARRAY_START);
    json.put_next_token(JSON_TOKEN_OBJECT_START);
  }
  if (has_value_list) {
    // generate the value list into an enum
    json.put_next_token(JSON_TOKEN_NAME, "enum");
    json.put_next_token(JSON_TOKEN_ARRAY_START);
    generate_json_schema_value_list(json, allow_special_float, false);
    json.put_next_token(JSON_TOKEN_ARRAY_END);
    if (my_owner->has_as_value_union()) {
      // the original value list cannot always be recreated from the generated
      // JSON value list in case of "as value" unions (because there are no field
      // names)
      // the list needs to be regenerated with field names (as if it was a regular
      // union) under a new keyword (valueList)
      json.put_next_token(JSON_TOKEN_NAME, "valueList");
      json.put_next_token(JSON_TOKEN_ARRAY_START);
      generate_json_schema_value_list(json, allow_special_float, true);
      json.put_next_token(JSON_TOKEN_ARRAY_END);
    }
  }
  if (need_anyOf && has_value_list) {
    // end of the value list and beginning of the first value range
    json.put_next_token(JSON_TOKEN_OBJECT_END);
    json.put_next_token(JSON_TOKEN_OBJECT_START);
  }
  if (nof_ranges > 0) {
    switch (subtype) {
    case ST_INTEGER:
    case ST_FLOAT:
      generate_json_schema_number_ranges(json);
      break;
    case ST_CHARSTRING:
    case ST_UNIVERSAL_CHARSTRING: {
      // merge all string range restrictions into one JSON schema pattern
      char* pattern_str = mcopystrn("\"^[", 3);
      pattern_str = generate_json_schema_string_ranges(pattern_str);
      pattern_str = mputstrn(pattern_str, "]*$\"", 4);
      json.put_next_token(JSON_TOKEN_NAME, "pattern");
      json.put_next_token(JSON_TOKEN_STRING, pattern_str);
      Free(pattern_str);
      break; }
    default:
      FATAL_ERROR("SubType::generate_json_schema - range %d", subtype);
    }
  }
  if (need_anyOf) {
    // end of the 'anyOf' structure
    json.put_next_token(JSON_TOKEN_OBJECT_END);
    json.put_next_token(JSON_TOKEN_ARRAY_END);
  }
}

void SubType::generate_json_schema_value_list(JSON_Tokenizer& json,
                                              bool allow_special_float,
                                              bool union_value_list)
{
  for (size_t i = 0; i < parsed->size(); ++i) {
    SubTypeParse *parse = (*parsed)[i];
    if (parse->get_selection() == SubTypeParse::STP_SINGLE) {
      if (parse->Single()->get_valuetype() == Value::V_REFD) {
        Common::Assignment* ass = parse->Single()->get_reference()->get_refd_assignment();
        if (ass->get_asstype() == Common::Assignment::A_TYPE) {
          // it's a reference to another subtype, insert its value list here
          ass->get_Type()->get_sub_type()->generate_json_schema_value_list(json,
            allow_special_float, union_value_list);
        }
      }
      else {
        Ttcn::JsonOmitCombination omit_combo(parse->Single());
        do {
          parse->Single()->generate_json_value(json, allow_special_float,
            union_value_list, &omit_combo);
        } // only generate the first combination for the unions' "valueList" keyword
        while (!union_value_list && omit_combo.next());
      }
    }
  }
}

bool SubType::generate_json_schema_number_ranges(JSON_Tokenizer& json, bool first /* = true */)
{
  for (size_t i = 0; i < parsed->size(); ++i) {
    SubTypeParse *parse = (*parsed)[i];
    if (parse->get_selection() == SubTypeParse::STP_SINGLE) {
      if (parse->Single()->get_valuetype() == Value::V_REFD) {
        Common::Assignment* ass = parse->Single()->get_reference()->get_refd_assignment();
        if (ass->get_asstype() == Common::Assignment::A_TYPE) {
          // it's a reference to another subtype, insert its value ranges here
          first = ass->get_Type()->get_sub_type()->generate_json_schema_number_ranges(json, first);
        }
      }
    }
    else if (parse->get_selection() == SubTypeParse::STP_RANGE) {
      if (!first) {
        // the ranges are in an 'anyOf' structure, they need to be placed in an object
        json.put_next_token(JSON_TOKEN_OBJECT_END);
        json.put_next_token(JSON_TOKEN_OBJECT_START);
      }
      else {
        first = false;
      }
      // add the minimum and/or maximum values as numbers
      if (parse->Min() != NULL) {
        json.put_next_token(JSON_TOKEN_NAME, "minimum");
        parse->Min()->generate_json_value(json);
        json.put_next_token(JSON_TOKEN_NAME, "exclusiveMinimum");
        json.put_next_token(parse->MinExclusive() ? JSON_TOKEN_LITERAL_TRUE : JSON_TOKEN_LITERAL_FALSE);
      }
      if (parse->Max() != NULL) {
        json.put_next_token(JSON_TOKEN_NAME, "maximum");
        parse->Max()->generate_json_value(json);
        json.put_next_token(JSON_TOKEN_NAME, "exclusiveMaximum");
        json.put_next_token(parse->MaxExclusive() ? JSON_TOKEN_LITERAL_TRUE : JSON_TOKEN_LITERAL_FALSE);
      }
    }
  }
  return first;
}

char* SubType::generate_json_schema_string_ranges(char* pattern_str)
{
  for (size_t i = 0; i < parsed->size(); ++i) {
    SubTypeParse *parse = (*parsed)[i];
    if (parse->get_selection() == SubTypeParse::STP_SINGLE) {
      if (parse->Single()->get_valuetype() == Value::V_REFD) {
        Common::Assignment* ass = parse->Single()->get_reference()->get_refd_assignment();
        if (ass->get_asstype() == Common::Assignment::A_TYPE) {
          // it's a reference to another subtype, insert its string ranges here
          pattern_str = ass->get_Type()->get_sub_type()->generate_json_schema_string_ranges(pattern_str);
        }
      }
    }
    else if (parse->get_selection() == SubTypeParse::STP_RANGE) {
      // insert the string range into the pattern string
      string lower_str = (subtype == ST_CHARSTRING) ? parse->Min()->get_val_str() :
        ustring_to_uft8(parse->Min()->get_val_ustr());
      string upper_str = (subtype == ST_CHARSTRING) ? parse->Max()->get_val_str() :
        ustring_to_uft8(parse->Max()->get_val_ustr());
      pattern_str = mputprintf(pattern_str, "%s-%s", lower_str.c_str(), upper_str.c_str());
    }
  }
  return pattern_str;
}

void SubType::generate_json_schema_float(JSON_Tokenizer& json)
{
  bool has_nan = float_st->is_element(make_ttcn3float(REAL_NAN));
  bool has_pos_inf = float_st->is_element(make_ttcn3float(REAL_INFINITY));
  bool has_neg_inf = float_st->is_element(make_ttcn3float(-REAL_INFINITY));
  bool has_special = has_nan || has_pos_inf || has_neg_inf;
  bool has_number = false;
  for (size_t i = 0; i < parsed->size() && !has_number; ++i) {
    // go through the restrictions and check if at least one number is allowed
    SubTypeParse *parse = (*parsed)[i];
    switch (parse->get_selection()) {
    case SubTypeParse::STP_SINGLE: {
      Real r = parse->Single()->get_val_Real();
      if (r == r && r != REAL_INFINITY && r != -REAL_INFINITY) {
        // a single value other than NaN, INF and -INF is a number
        has_number = true;
      }
      break; }
    case SubTypeParse::STP_RANGE: {
      if (parse->Min() != NULL) {
        if (parse->Min()->get_val_Real() != REAL_INFINITY) {
          // a minimum value other than INF means a number is allowed
          has_number = true;
        }
      }
      if (parse->Max() != NULL) {
        // a maximum value other than -INF means a number is allowed
        if (parse->Max()->get_val_Real() != -REAL_INFINITY) {
          has_number = true;
        }
      }
      break; }
    default:
      break;
    }
  }
  if (has_number && has_special) {
    json.put_next_token(JSON_TOKEN_NAME, "anyOf");
    json.put_next_token(JSON_TOKEN_ARRAY_START);
    json.put_next_token(JSON_TOKEN_OBJECT_START);
  }
  if (has_number) {
    json.put_next_token(JSON_TOKEN_NAME, "type");
    json.put_next_token(JSON_TOKEN_STRING, "\"number\"");
    // generate the restrictions' schema elements here
    // (the 2nd parameter makes sure that NaN, INF and -INF are ignored)
    generate_json_schema(json, false);
  }
  if (has_number && has_special) {
    json.put_next_token(JSON_TOKEN_OBJECT_END);
    json.put_next_token(JSON_TOKEN_OBJECT_START);
  }
  if (has_special) {
    json.put_next_token(JSON_TOKEN_NAME, "enum");
    json.put_next_token(JSON_TOKEN_ARRAY_START);
    if (has_nan) {
      json.put_next_token(JSON_TOKEN_STRING, "\"not_a_number\"");
    }
    if (has_pos_inf) {
      json.put_next_token(JSON_TOKEN_STRING, "\"infinity\"");
    }
    if (has_neg_inf) {
      json.put_next_token(JSON_TOKEN_STRING, "\"-infinity\"");
    }
    json.put_next_token(JSON_TOKEN_ARRAY_END);
  }
  if (has_number && has_special) {
    json.put_next_token(JSON_TOKEN_OBJECT_END);
    json.put_next_token(JSON_TOKEN_ARRAY_END);
  }
}

} // namespace Common
