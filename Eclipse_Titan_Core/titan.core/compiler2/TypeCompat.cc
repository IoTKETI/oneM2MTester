/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "TypeCompat.hh"

#include "AST.hh"
#include "Type.hh"
#include "CompField.hh"
#include "ttcn3/ArrayDimensions.hh"
#include "ttcn3/TtcnTemplate.hh"
#include "main.hh"

namespace Common {

TypeCompatInfo::TypeCompatInfo(Module *p_my_module, Type *p_type1,
                               Type *p_type2, bool p_add_ref_str,
                               bool p_strict, bool p_is_temp) :
  m_my_module(p_my_module), m_type1(p_type1), m_type2(p_type2),
  m_strict(p_strict), m_is_temp(p_is_temp), m_needs_conversion(false),
  m_erroneous(false), str1_elem(false), str2_elem(false)
{
  if (p_type1 && p_add_ref_str) m_ref_str1 = p_type1->get_typename();
  if (p_type2 && p_add_ref_str) m_ref_str2 = p_type2->get_typename();
}

void TypeCompatInfo::set_is_erroneous(Type *p_type1, Type *p_type2,
                                      const string& p_error_str)
{
  m_erroneous = true;
  m_type1 = p_type1;
  m_type2 = p_type2;
  m_error_str = p_error_str;
}

string TypeCompatInfo::get_error_str_str() const
{
  // The resulting string should look like: "`f1.f2.f3...fn' [of type `tn']
  // and `g1.g2.g3...gm' [of type `tm']".  In run-time a simple error will
  // occure with a similarly simple error message.
  string ret_val = "Type mismatch: `" + m_ref_str1;
  if (m_ref_str1 != m_type1->get_typename())
    ret_val += ("' of type `" + m_type1->get_typename());
  ret_val += ("' and `" + m_ref_str2);
  if (m_ref_str2 != m_type2->get_typename())
    ret_val += ("' of type `" + m_type2->get_typename());
  ret_val += "' are not compatible";
  if (m_error_str.size() > 0) ret_val += (": " + m_error_str);
  return ret_val;
}

void TypeCompatInfo::add_type_conversion(Type *p_from_type, Type *p_to_type)
{
  if (!m_my_module) FATAL_ERROR("TypeCompatInfo::add_type_conversion()");
  if (p_from_type == p_to_type) return;
  TypeConv *conv = new TypeConv(p_from_type, p_to_type, m_is_temp);
  m_my_module->add_type_conv(conv);
}

void TypeCompatInfo::append_ref_str(int p_ref_no, const string& p_ref_str)
{
  string& ref_str = p_ref_no == 0 ? m_ref_str1 : m_ref_str2;
  ref_str += p_ref_str;
}

string TypeConv::get_conv_func(Type *p_from, Type *p_to, Module *p_mod)
{
  const char *from_name = p_from->get_genname_own().c_str();
  const char *to_name = p_to->get_genname_own().c_str();
  Module *from_mod = p_from->get_my_scope()->get_scope_mod();
  Module *to_mod = p_to->get_my_scope()->get_scope_mod();
  string ret_val = "conv_" + (from_mod != p_mod
    ? (from_mod->get_modid().get_name() + "_") : string()) + from_name + "_" +
    (to_mod != p_mod ? (to_mod->get_modid().get_name() + "_") : string()) +
    to_name;
  return ret_val;
}

bool TypeConv::needs_conv_refd(GovernedSimple *p_val_or_temp)
{
  if (!use_runtime_2) FATAL_ERROR("TypeConv::needs_conv_refd()");
  Type *original_type = NULL;
  Type *current_type = NULL;
  switch (p_val_or_temp->get_st()) {
  case Setting::S_TEMPLATE: {
    Template *p_temp = static_cast<Template *>(p_val_or_temp);
    if (p_temp->get_templatetype() != Template::TEMPLATE_REFD) return false;
    current_type = p_temp->get_my_governor()->get_type_refd_last();
    Common::Reference *ref = p_temp->get_reference();
    if (!ref) FATAL_ERROR("TypeConv::needs_conv_refd()");
    original_type = ref->get_refd_assignment()->get_Type()
      ->get_field_type(ref->get_subrefs(), Type::EXPECTED_TEMPLATE)
      ->get_type_refd_last();
    break; }
  case Setting::S_V: {
    Value *p_val = static_cast<Value *>(p_val_or_temp);
    if (p_val->get_valuetype() != Value::V_REFD) return false;
    current_type = p_val->get_my_governor()->get_type_refd_last();
    Common::Reference *ref = p_val->get_reference();
    if (!ref) FATAL_ERROR("TypeConv::needs_conv_refd()");
    original_type = ref->get_refd_assignment()->get_Type()
      ->get_field_type(ref->get_subrefs(), Type::EXPECTED_DYNAMIC_VALUE)
      ->get_type_refd_last();
    break; }
  default:
    return false;
  }
  // We should have the scope at this point.  Templates like "?" shouldn't
  // reach this point.
  return p_val_or_temp->get_my_scope()->get_scope_mod()
    ->needs_type_conv(original_type, current_type);
}

// Always call needs_conv_refd() before this.  It is assumed that the value
// is a reference and the conversion is necessary.
char *TypeConv::gen_conv_code_refd(char *p_str, const char *p_name,
                                   GovernedSimple *p_val_or_temp)
{
  if (!use_runtime_2) FATAL_ERROR("TypeConv::gen_conv_code_refd()");
  Type *original_type = NULL;
  Type *current_type = NULL;
  string original_type_genname("<unknown>");
  string current_type_genname("<unknown>");
  Common::Scope *my_scope = NULL;
  switch (p_val_or_temp->get_st()) {
  case Setting::S_TEMPLATE: {
    Template *p_temp = static_cast<Template *>(p_val_or_temp);
    my_scope = p_temp->get_my_scope();
    if (p_temp->get_templatetype() != Template::TEMPLATE_REFD)
      FATAL_ERROR("TypeConv::gen_conv_code_refd()");
    Common::Reference *ref = p_temp->get_reference();
    current_type = p_temp->get_my_governor()->get_type_refd_last();
    original_type = ref->get_refd_assignment()->get_Type()
      ->get_field_type(ref->get_subrefs(), Type::EXPECTED_TEMPLATE)
      ->get_type_refd_last();
    original_type_genname = original_type->get_genname_template(my_scope);
    current_type_genname = current_type->get_genname_template(my_scope);
    break; }
  case Setting::S_V: {
    Value *p_val = static_cast<Value *>(p_val_or_temp);
    my_scope = p_val->get_my_scope();
    // We can't really handle other values and templates.
    if (p_val->get_valuetype() != Value::V_REFD)
      FATAL_ERROR("TypeConv::gen_conv_code_refd()");
    Common::Reference *ref = p_val->get_reference();
    current_type = p_val->get_my_governor()->get_type_refd_last();
    original_type = ref->get_refd_assignment()->get_Type()
      ->get_field_type(ref->get_subrefs(), Type::EXPECTED_DYNAMIC_VALUE)
      ->get_type_refd_last();
    original_type_genname = original_type->get_genname_value(my_scope);
    current_type_genname = current_type->get_genname_value(my_scope);
    break; }
  default:
    FATAL_ERROR("TypeConv::gen_conv_code_refd()");
  }
  const string& tmp_id1 = p_val_or_temp->get_temporary_id();
  const char *tmp_id_str1 = tmp_id1.c_str();  // For "p_val/p_temp".
  const string& tmp_id2 = p_val_or_temp->get_temporary_id();
  const char *tmp_id_str2 = tmp_id2.c_str();  // For converted "p_val/p_temp".
  expression_struct expr;
  Code::init_expr(&expr);
  expr.expr = mputprintf(expr.expr,
    "%s %s;\n%s = ", original_type_genname.c_str(), tmp_id_str1,
    tmp_id_str1);
  // Always save the value into a separate temporary.  Who knows?  It can
  // avoid some problems with referencing complex expressions.  The third
  // argument passed to the conversion function is unused unless the type
  // we're converting to is a T_CHOICE_{A,T}/T_ANYTYPE.  Calling
  // generate_code_init() directly may cause infinite recursion.
  if (p_val_or_temp->get_st() == Setting::S_V)
    static_cast<Value *>(p_val_or_temp)->get_reference()->generate_code(&expr);
  else static_cast<Template *>(p_val_or_temp)->get_reference()->generate_code(&expr);
  expr.expr = mputprintf(expr.expr,
    ";\n%s %s;\n"
    "if (!%s(%s, %s)) TTCN_error(\"Values or templates of types `%s' and "
    "`%s' are not compatible at run-time\")",  // ";\n" will be added later.
    current_type_genname.c_str(), tmp_id_str2, get_conv_func(original_type,
    current_type, my_scope->get_scope_mod()).c_str(), tmp_id_str2,
    tmp_id_str1, original_type->get_typename().c_str(),
    current_type->get_typename().c_str());
  // Merge by hand.  Code::merge_free_expr() puts an additional ";\n".
  // "p_str" is just a local pointer here, it needs an mputprintf() at the
  // end.
  p_str = Code::merge_free_expr(p_str, &expr);
  return mputprintf(p_str, "%s = %s;\n", p_name, tmp_id_str2);
}

void TypeConv::gen_conv_func(char **p_prototypes, char **p_bodies,
                             Module *p_mod)
{
  string from_name = m_is_temp ? m_from->get_genname_template(p_mod)
    : m_from->get_genname_value(p_mod);
  string to_name = m_is_temp ? m_to->get_genname_template(p_mod)
    : m_to->get_genname_value(p_mod);
  *p_prototypes = mputprintf(*p_prototypes,
    "%sboolean %s(%s& p_to_v, const %s& p_from_v);\n",
    split_to_slices ? "" : "static ", get_conv_func(m_from, m_to, p_mod).c_str(), to_name.c_str(),
    from_name.c_str());
  *p_bodies = mputprintf(*p_bodies,
    "%sboolean %s(%s& p_to_v, const %s& p_from_v)\n{\n",
    split_to_slices ? "" : "static ", get_conv_func(m_from, m_to, p_mod).c_str(), to_name.c_str(),
    from_name.c_str());
  switch (m_to->get_typetype()) {
  case Type::T_SEQ_A:
  case Type::T_SEQ_T:
    switch (m_from->get_typetype()) {
    case Type::T_SEQ_A:
    case Type::T_SEQ_T:
      gen_conv_func_record_set(p_bodies, p_mod);
      break;
    case Type::T_SEQOF:
      gen_conv_func_record_set_record_of_set_of(p_bodies, p_mod);
      break;
    case Type::T_ARRAY:
      gen_conv_func_array_record(p_bodies, p_mod);
      break;
    default:
      FATAL_ERROR("TypeConv::gen_conv_func()");
      return;
    } break;
  case Type::T_SEQOF:
    switch (m_from->get_typetype()) {
    case Type::T_SEQ_A:
    case Type::T_SEQ_T:
    case Type::T_SEQOF:
      gen_conv_func_record_set_record_of_set_of(p_bodies, p_mod);
      break;
    case Type::T_ARRAY:
      gen_conv_func_array_record_of(p_bodies, p_mod);
      break;
    default:
      FATAL_ERROR("TypeConv::gen_conv_func()");
      return;
    } break;
  case Type::T_ARRAY:
    switch (m_from->get_typetype()) {
    case Type::T_SEQ_A:
    case Type::T_SEQ_T:
      gen_conv_func_array_record(p_bodies, p_mod);
      break;
    case Type::T_SEQOF:
    case Type::T_ARRAY:
      gen_conv_func_array_record_of(p_bodies, p_mod);
      break;
    default:
      FATAL_ERROR("TypeConv::gen_conv_func()");
      return;
    } break;
  case Type::T_SET_A:
  case Type::T_SET_T:
    switch (m_from->get_typetype()) {
    case Type::T_SET_A:
    case Type::T_SET_T:
      gen_conv_func_record_set(p_bodies, p_mod);
      break;
    case Type::T_SETOF:
      gen_conv_func_record_set_record_of_set_of(p_bodies, p_mod);
      break;
    default:
      FATAL_ERROR("TypeConv::gen_conv_func()");
      return;
    } break;
  case Type::T_SETOF:
    switch (m_from->get_typetype()) {
    case Type::T_SET_A:
    case Type::T_SET_T:
    case Type::T_SETOF:
      gen_conv_func_record_set_record_of_set_of(p_bodies, p_mod);
      break;
    default:
      FATAL_ERROR("TypeConv::gen_conv_func()");
      return;
    } break;
  case Type::T_CHOICE_A:
  case Type::T_CHOICE_T:
    switch (m_from->get_typetype()) {
    case Type::T_CHOICE_A:
    case Type::T_CHOICE_T:
      gen_conv_func_choice_anytype(p_bodies, p_mod);
      break;
    default:
      FATAL_ERROR("TypeConv::gen_conv_func()");
      return;
    } break;
  case Type::T_ANYTYPE:
    switch (m_from->get_typetype()) {
    case Type::T_ANYTYPE:
      gen_conv_func_choice_anytype(p_bodies, p_mod);
      break;
    default:
      FATAL_ERROR("TypeConv::gen_conv_func()");
      return;
    } break;
  default:
    FATAL_ERROR("TypeConv::gen_conv_func()");
    return;
  }
  *p_bodies = mputprintf(*p_bodies, "return TRUE;\n}\n\n");
}

// Conversions between records-records and sets-sets.
void TypeConv::gen_conv_func_record_set(char **p_bodies, Module *p_mod)
{
  Type::typetype_t m_from_tt = m_from->get_typetype();
  Type::typetype_t m_to_tt = m_to->get_typetype();
  switch (m_from_tt) {
  case Type::T_SEQ_A:
  case Type::T_SEQ_T:
    if (m_to_tt != Type::T_SEQ_A && m_to_tt != Type::T_SEQ_T)
      FATAL_ERROR("TypeConv::gen_conv_func_record_set()");
    break;
  case Type::T_SET_A:
  case Type::T_SET_T:
    if (m_to_tt != Type::T_SET_A && m_to_tt != Type::T_SET_T)
      FATAL_ERROR("TypeConv::gen_conv_func_record_set()");
    break;
  default:
    FATAL_ERROR("TypeConv::gen_conv_func_record_set()");
    return;
  }
  for (size_t i = 0; i < m_to->get_nof_comps(); i++) {
    CompField *cf_to = m_to->get_comp_byIndex(i);
    CompField *cf_from = m_from->get_comp_byIndex(i);
    const Identifier& cf_id_to = cf_to->get_name();
    const Identifier& cf_id_from = cf_from->get_name();
    Type *cf_type_to = cf_to->get_type()->get_type_refd_last();
    Type *cf_type_from = cf_from->get_type()->get_type_refd_last();
    string cf_type_to_name = m_is_temp ? cf_type_to
      ->get_genname_template(p_mod) : cf_type_to->get_genname_value(p_mod);
    if (!p_mod->needs_type_conv(cf_type_from, cf_type_to)) {
      *p_bodies = mputprintf(*p_bodies,
        "if (p_from_v.%s().is_bound()) p_to_v.%s() = p_from_v.%s();\n",
        cf_id_from.get_name().c_str(), cf_id_to.get_name().c_str(),
        cf_id_from.get_name().c_str());
    } else {
      const string& tmp_id = p_mod->get_temporary_id();
      const char *tmp_id_str = tmp_id.c_str();
      *p_bodies = mputprintf(*p_bodies,
        "%s %s;\n"
        "if (!%s(%s, p_from_v.%s())) return FALSE;\n"
        "if (%s.is_bound()) p_to_v.%s() = %s;\n",
        cf_type_to_name.c_str(), tmp_id_str, get_conv_func(cf_type_from,
        cf_type_to, p_mod).c_str(), tmp_id_str, cf_id_from.get_name()
        .c_str(), tmp_id_str, cf_id_to.get_name().c_str(), tmp_id_str);
    }
  }
}

// Converting arrays to record types vice versa.
void TypeConv::gen_conv_func_array_record(char **p_bodies, Module *p_mod)
{
  Type::typetype_t m_from_tt = m_from->get_typetype();
  Type::typetype_t m_to_tt = m_to->get_typetype();
  switch (m_from_tt) {
  case Type::T_SEQ_A:
  case Type::T_SEQ_T:
    if (m_to_tt != Type::T_ARRAY)
      FATAL_ERROR("TypeConv::gen_conv_func_array_record()");
    break;
  case Type::T_ARRAY:
    if (m_to_tt != Type::T_SEQ_A && m_to_tt != Type::T_SEQ_T)
      FATAL_ERROR("TypeConv::gen_conv_func_array_record()");
    break;
  default:
    FATAL_ERROR("TypeConv::gen_conv_func_array_record()");
    return;
  }
  // From array to a structure type.
  if (m_from_tt == Type::T_ARRAY) {
    Type *of_type_from = m_from->get_ofType()->get_type_refd_last();
    Int of_type_from_offset = m_from->get_dimension()->get_offset();
    for (size_t i = 0; i < m_to->get_nof_comps(); i++) {
      CompField *cf_to = m_to->get_comp_byIndex(i);
      const Identifier& cf_id_to = cf_to->get_name();
      Type *cf_type_to = cf_to->get_type()->get_type_refd_last();
      string cf_type_to_name = m_is_temp ? cf_type_to
        ->get_genname_template(p_mod) : cf_type_to
        ->get_genname_value(p_mod);
      if (!p_mod->needs_type_conv(of_type_from, cf_type_to)) {
        *p_bodies = mputprintf(*p_bodies,
          "if (p_from_v[%lu].is_bound()) p_to_v.%s() = p_from_v[%lu];\n",
          (long unsigned)(i + of_type_from_offset),
          cf_id_to.get_name().c_str(),
          (long unsigned)(i + of_type_from_offset));
      } else {
        const string& tmp_id = p_mod->get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        *p_bodies = mputprintf(*p_bodies,
          "%s %s;\n"
          "if (!%s(%s, p_from_v[%lu])) return FALSE;\n"
          "if (%s.is_bound()) p_to_v.%s() = %s;\n",
          cf_type_to_name.c_str(), tmp_id_str, get_conv_func(of_type_from,
          cf_type_to, p_mod).c_str(), tmp_id_str,
          (long unsigned)(i + of_type_from_offset),
          tmp_id_str, cf_id_to.get_name().c_str(), tmp_id_str);
      }
    }
  // From a structure to an array.  An example 6.3.2.2 shows that
  // OMIT_VALUES should be skipped at assignments.
  } else {
    Type *of_type_to = m_to->get_ofType()->get_type_refd_last();
    string of_type_to_name = m_is_temp ? of_type_to
      ->get_genname_template(p_mod) : of_type_to->get_genname_value(p_mod);
    *p_bodies = mputprintf(*p_bodies,
      "unsigned int index = %lu;\n",
      (long unsigned)m_to->get_dimension()->get_offset());
    for (size_t i = 0; i < m_from->get_nof_comps(); i++) {
      CompField *cf_from = m_from->get_comp_byIndex(i);
      const Identifier& cf_id_from = cf_from->get_name();
      Type *cf_type_from = cf_from->get_type()->get_type_refd_last();
      if (!p_mod->needs_type_conv(cf_type_from, of_type_to)) {
        *p_bodies = mputprintf(*p_bodies,
          "if (p_from_v.%s().is_bound()) {\n",
          cf_id_from.get_name().c_str());
        if (cf_from->get_is_optional()) {
          *p_bodies = mputprintf(*p_bodies,
            "if (!(p_from_v.%s()%s)) p_to_v[index++] "
            "= p_from_v.%s();\n",
            cf_id_from.get_name().c_str(), (m_is_temp ? ".is_omit()" :
            " == OMIT_VALUE"), cf_id_from.get_name().c_str());
        } else {
          *p_bodies = mputprintf(*p_bodies,
                                 "p_to_v[index++] = p_from_v.%s();\n",
                                 cf_id_from.get_name().c_str());
        }
        // For unbound elements.
        *p_bodies = mputstr(*p_bodies, "} else index++;\n");
      } else {
        const string& tmp_id = p_mod->get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        *p_bodies = mputprintf(*p_bodies,
          "%s %s;\n"
          "if (!%s(%s, p_from_v.%s())) return FALSE;\n"
          "if (%s.is_bound()) p_to_v[index++] = %s;\n",
          of_type_to_name.c_str(), tmp_id_str, get_conv_func(cf_type_from,
          of_type_to, p_mod).c_str(), tmp_id_str, cf_id_from.get_name()
          .c_str(), tmp_id_str, tmp_id_str);
      }
    }
  }
}

// Conversions between arrays and between record ofs and arrays.
void TypeConv::gen_conv_func_array_record_of(char **p_bodies, Module *p_mod)
{
  Type::typetype_t m_from_tt = m_from->get_typetype();
  Type::typetype_t m_to_tt = m_to->get_typetype();
  switch (m_from_tt) {
  case Type::T_SEQOF:
    if (m_to_tt == Type::T_SEQOF)
      FATAL_ERROR("TypeConv::gen_conv_func_array_record_of()");
    break;
  case Type::T_ARRAY:
    break;
  default:
    FATAL_ERROR("TypeConv::gen_conv_func_array_record_of()");
    return;
  }
  Type *of_type_from = m_from->get_ofType()->get_type_refd_last();
  Type *of_type_to = m_to->get_ofType()->get_type_refd_last();
  string of_type_to_name = m_is_temp ? of_type_to
    ->get_genname_template(p_mod) : of_type_to->get_genname_value(p_mod);
  Int of_type_from_offset = m_from_tt == Type::T_ARRAY ? m_from
    ->get_dimension()->get_offset() : 0;
  Int of_type_to_offset = m_to_tt == Type::T_ARRAY ? m_to->get_dimension()
    ->get_offset() : 0;
  // If we have two arrays the dimensions must match at this point.  For
  // record of types we need to check the current size.
  if (m_from_tt == Type::T_SEQOF)
    *p_bodies = mputprintf(*p_bodies,
      "if (!p_from_v.is_bound() || p_from_v.size_of() != %lu) return FALSE;\n",
      (long unsigned)m_to->get_dimension()->get_size());
  else if (m_to_tt == Type::T_SEQOF)
    *p_bodies = mputprintf(*p_bodies, "p_to_v.set_size(%lu);\n",
      (long unsigned)m_from->get_dimension()->get_size());
  size_t dim = m_from_tt == Type::T_ARRAY ? m_from->get_dimension()->get_size() :
    m_to->get_dimension()->get_size();
  for (size_t i = 0; i < dim; i++) {
    if (!p_mod->needs_type_conv(of_type_from, of_type_to)) {
      *p_bodies = mputprintf(*p_bodies,
        "if (p_from_v[%lu].is_bound()) p_to_v[%lu] = p_from_v[%lu];\n",
        (long unsigned)(i + of_type_from_offset),
        (long unsigned)(i + of_type_to_offset),
        (long unsigned)(i + of_type_from_offset));
    } else {
      const string& tmp_id = p_mod->get_temporary_id();
      const char *tmp_id_str = tmp_id.c_str();
      *p_bodies = mputprintf(*p_bodies,
        "%s %s;\n"
        "if (!%s(%s, p_from_v[%lu])) return FALSE;\n"
        "if (%s.is_bound()) p_to_v[%lu] = %s;\n"
        "}\n",
      of_type_to_name.c_str(), tmp_id_str, get_conv_func(of_type_from,
      of_type_to, p_mod).c_str(), tmp_id_str,
      (long unsigned)(i + of_type_from_offset), tmp_id_str,
      (long unsigned)(i + of_type_from_offset), tmp_id_str);
    }
  }
}

// Conversions from records or sets to record ofs or set ofs.  Vice versa.
void TypeConv::gen_conv_func_record_set_record_of_set_of(char **p_bodies, Module *p_mod)
{
  Type::typetype_t m_from_tt = m_from->get_typetype();
  Type::typetype_t m_to_tt = m_to->get_typetype();
  switch (m_from_tt) {
  case Type::T_SEQ_A:
  case Type::T_SEQ_T:
    if (m_to_tt == Type::T_SEQ_A || m_to_tt == Type::T_SEQ_T)
      // From record to record: this function was not supposed to be called
      FATAL_ERROR("TypeConv::gen_conv_func_record_set_record_of_set_of()");
    break;
  case Type::T_SEQOF:
    if (m_to_tt == Type::T_SET_A || m_to_tt == Type::T_SET_T
        || m_to_tt == Type::T_SETOF)
      FATAL_ERROR("TypeConv::gen_conv_func_record_set_record_of_set_of()");
    break;
  case Type::T_SET_A:
  case Type::T_SET_T:
    if (m_to_tt == Type::T_SET_A || m_to_tt == Type::T_SET_T)
      // From set to set: this function was not supposed to be called
      FATAL_ERROR("TypeConv::gen_conv_func_record_set_record_of_set_of()");
    break;
  case Type::T_SETOF:
    if (m_to_tt == Type::T_SEQ_A || m_to_tt == Type::T_SEQ_T
        || m_to_tt == Type::T_SEQOF)
      FATAL_ERROR("TypeConv::gen_conv_func_record_set_record_of_set_of()");
    break;
  default:
    FATAL_ERROR("TypeConv::gen_conv_func_record_set_record_of_set_of()");
    return;
  }
  if (m_from_tt == Type::T_SEQOF || m_from_tt == Type::T_SETOF) {
    // From a list type to a structure.
    if (m_to_tt == Type::T_SEQ_A || m_to_tt == Type::T_SEQ_T
        || m_to_tt == Type::T_SET_A || m_to_tt == Type::T_SET_T) {
    *p_bodies = mputprintf(*p_bodies,
      "if (!p_from_v.is_bound() || p_from_v.size_of() != %lu) return FALSE;\n",
      (long unsigned)m_to->get_nof_comps());
    Type *of_type_from = m_from->get_ofType()->get_type_refd_last();
    for (size_t i = 0; i < m_to->get_nof_comps(); i++) {
      CompField *cf_to = m_to->get_comp_byIndex(i);
      const Identifier& cf_id_to = cf_to->get_name();
      Type *cf_type_to = cf_to->get_type()->get_type_refd_last();
      string cf_type_to_name = m_is_temp ? cf_type_to
        ->get_genname_template(p_mod) : cf_type_to->get_genname_value(p_mod);
      if (!p_mod->needs_type_conv(of_type_from, cf_type_to)) {
        *p_bodies = mputprintf(*p_bodies,
        "if (p_from_v[%lu].is_bound()) p_to_v.%s() = p_from_v[%lu];\n",
        (long unsigned)i, cf_id_to.get_name().c_str(), (long unsigned)i);
      } else {
        const string& tmp_id = p_mod->get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        *p_bodies = mputprintf(*p_bodies,
          "%s %s;\n"
          "if (!%s(%s, p_from_v[%lu])) return FALSE;\n"
          "if (%s.is_bound()) p_to_v.%s() = %s;\n",
          cf_type_to_name.c_str(), tmp_id_str, get_conv_func(of_type_from,
          cf_type_to, p_mod).c_str(), tmp_id_str, (long unsigned)i,
          tmp_id_str, cf_id_to.get_name().c_str(), tmp_id_str);
      }
      // Unbound elements needs to be assigned as OMIT_VALUE.
      if (cf_to->get_is_optional())
        *p_bodies = mputprintf(*p_bodies,
                               "else p_to_v.%s() = OMIT_VALUE;\n",
                               cf_id_to.get_name().c_str());
    }
    // From a list type to a list type.
    } else {
      *p_bodies = mputstr(*p_bodies,
        "p_to_v.set_size(p_from_v.size_of());\n"
        "for (int i = 0; i < p_from_v.size_of(); i++) {\n");
      Type *of_type_from = m_from->get_ofType()->get_type_refd_last();
      Type *of_type_to = m_to->get_ofType()->get_type_refd_last();
      string of_type_to_name = m_is_temp ? of_type_to
        ->get_genname_template(p_mod) : of_type_to->get_genname_value(p_mod);
      if (!p_mod->needs_type_conv(of_type_from, of_type_to)) {
        *p_bodies = mputstr(*p_bodies,
          "if (p_from_v[i].is_bound()) p_to_v[i] = p_from_v[i];\n}\n");
      } else {
        const string& tmp_id = p_mod->get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        *p_bodies = mputprintf(*p_bodies,
          "%s %s;\n"
          "if (!%s(%s, p_from_v[i])) return FALSE;\n"
          "if (%s.is_bound()) p_to_v[i] = %s;\n"
          "}\n",
          of_type_to_name.c_str(), tmp_id_str, get_conv_func(of_type_from,
          of_type_to, p_mod).c_str(), tmp_id_str, tmp_id_str, tmp_id_str);
      }
    }
  } else {
    // From a structure to a structure.  Hey...
    if (m_to_tt == Type::T_SEQ_A || m_to_tt == Type::T_SEQ_T
        || m_to_tt == Type::T_SET_A || m_to_tt == Type::T_SET_T) {
      FATAL_ERROR("TypeConv::gen_conv_func_record_set_record_of_set_of()");
    // From a structure to a list type.
    } else {
      *p_bodies = mputprintf(*p_bodies,
        "p_to_v.set_size(%lu);\n",
        (long unsigned)m_from->get_nof_comps());
      Type *of_type_to = m_to->get_ofType()->get_type_refd_last();
      string of_type_to_name = m_is_temp ? of_type_to
        ->get_genname_template(p_mod) : of_type_to->get_genname_value(p_mod);
      for (size_t i = 0; i < m_from->get_nof_comps(); i++) {
        CompField *cf_from = m_from->get_comp_byIndex(i);
        const Identifier& cf_id_from = cf_from->get_name();
        Type *cf_type_from = cf_from->get_type()->get_type_refd_last();
        if (!p_mod->needs_type_conv(cf_type_from, of_type_to)) {
          *p_bodies = mputprintf(*p_bodies,
            "if (p_from_v.%s().is_bound()", cf_id_from.get_name().c_str());
          if (cf_from->get_is_optional()) {
            // Additional check to avoid omitted fields.
            *p_bodies = mputprintf(*p_bodies,
              " && !(p_from_v.%s()%s)",
              cf_id_from.get_name().c_str(), (m_is_temp ? ".is_omit()" :
              " == OMIT_VALUE"));
          }
          // Final assignment.
          *p_bodies = mputprintf(*p_bodies,
            ") p_to_v[%lu] = p_from_v.%s();\n", (long unsigned)i,
            cf_id_from.get_name().c_str());
        } else {
          const string& tmp_id = p_mod->get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          *p_bodies = mputprintf(*p_bodies,
            "%s %s;\n"
            "if (!%s(%s, p_from_v.%s())) return FALSE;\n"
            "if (%s.is_bound()) p_to_v[%lu] = %s;\n",
            of_type_to_name.c_str(), tmp_id_str, get_conv_func(cf_type_from,
            of_type_to, p_mod).c_str(), tmp_id_str,
            cf_id_from.get_name().c_str(), tmp_id_str, (long unsigned)i,
            tmp_id_str);
        }
      }
    }
  }
}

void TypeConv::gen_conv_func_choice_anytype(char **p_bodies, Module *p_mod)
{
  Type::typetype_t from_tt = m_from->get_typetype();
  Type::typetype_t to_tt = m_to->get_typetype();
  switch (from_tt) {
  case Type::T_CHOICE_A:
  case Type::T_CHOICE_T:
    if (to_tt != Type::T_CHOICE_A && to_tt != Type::T_CHOICE_T)
      FATAL_ERROR("TypeConv::gen_conv_func_choice_anytype()");
    break;
  case Type::T_ANYTYPE:
    if (to_tt != Type::T_ANYTYPE)
      FATAL_ERROR("TypeConv::gen_conv_func_choice_anytype()");
    break;
  default:
    FATAL_ERROR("TypeConv::gen_conv_func_choice_anytype()");
  }
  // Anytype field accessors always have an "AT_" prefix.
  const char *anytype_prefix = from_tt == Type::T_ANYTYPE ? "AT_" : "";
  *p_bodies = mputprintf(*p_bodies,
    "if (!p_from_v.is_bound()) return FALSE;\n"
    "switch (p_from_v.get_selection()) {\n");
  for (size_t i = 0; i < m_from->get_nof_comps(); i++) {
    CompField *cf_from = m_from->get_comp_byIndex(i);
    Type *cf_type_from = cf_from->get_type()->get_type_refd_last();
    const Identifier& cf_id_from = cf_from->get_name();
    for (size_t j = 0; j < m_to->get_nof_comps(); j++) {
      CompField *cf_to = m_to->get_comp_byIndex(j);
      Type *cf_type_to = cf_to->get_type()->get_type_refd_last();
      const Identifier& cf_id_to = cf_to->get_name();
      // We have at most one field with the same name, break the inner loop
      // if found.  If the two different types have the same field name, but
      // they don't have a conversion function (e.g. an integer-boolean
      // pair) don't generate the assignment.  It's only an union specific
      // problem, since they're compatible if they have at least one
      // "compatible" field.
      if (cf_id_from.get_name() == cf_id_to.get_name()) {
        string from_name = m_is_temp ? m_from->get_genname_template(p_mod)
          : m_from->get_genname_value(p_mod);
        string from_name_sel = m_from->get_genname_value(p_mod);
        if (p_mod->needs_type_conv(cf_type_from, cf_type_to)) {
          const string& tmp_id = p_mod->get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          *p_bodies = mputprintf(*p_bodies,
            "case %s::ALT_%s: {\n"
            "%s %s;\n"
            "if (!%s(%s, p_from_v.%s%s())) return FALSE;\n"
            "if (%s.is_bound()) p_to_v = %s;\n"
            "break; }\n",
            from_name_sel.c_str(), cf_id_from.get_name().c_str(),
            from_name.c_str(), tmp_id_str,
            get_conv_func(cf_type_from, cf_type_to, p_mod).c_str(),
            tmp_id_str, anytype_prefix, cf_id_from.get_name().c_str(),
            tmp_id_str, tmp_id_str);
        } else if (cf_type_from->is_compatible(cf_type_to, NULL, NULL)) {  // E.g. basic types.
          // The same module + field name is required for anytype field
          // types.  Only for structured types.
          bool both_structured = cf_type_from->is_structured_type()
            && cf_type_to->is_structured_type();
          if (from_tt == Type::T_ANYTYPE && both_structured
              && cf_type_from->get_my_scope()->get_scope_mod() !=
              cf_type_to->get_my_scope()->get_scope_mod())
            break;
          *p_bodies = mputprintf(*p_bodies,
            "case %s::ALT_%s:\n"
            "if (p_from_v.%s%s().is_bound()) "
            "p_to_v.%s%s() = p_from_v.%s%s();\n"
            "break;\n",
            from_name_sel.c_str(), cf_id_from.get_name().c_str(),
            anytype_prefix, cf_id_from.get_name().c_str(), anytype_prefix,
            cf_id_to.get_name().c_str(), anytype_prefix,
            cf_id_from.get_name().c_str());
        }
        break;
      }
    }
  }
  *p_bodies = mputprintf(*p_bodies,
                         "default:\n"
                         "return FALSE;\n"
                         "}\n");
}

TypeChain::TypeChain() : m_first_double(-1) { }

TypeChain::~TypeChain()
{
  while (!m_marked_states.empty()) delete m_marked_states.pop();
  m_marked_states.clear();  // Does nothing.
  m_chain.clear();  // (Type *)s should stay.
}

void TypeChain::add(Type *p_type)
{
  if (m_first_double == -1) {
    for (size_t i = 0; i < m_chain.size() && m_first_double == -1; i++)
      if (m_chain[i] == p_type)
        m_first_double = m_chain.size();
  }
  m_chain.add(p_type);
}

void TypeChain::mark_state()
{
  // FIXME: Dynamically allocated integers used only for bookkeeping.
  m_marked_states.push(new int(m_chain.size()));
}

void TypeChain::previous_state()
{
  if (m_marked_states.empty()) FATAL_ERROR("TypeChain::previous_state()");
  int state = *m_marked_states.top();
  for (size_t i = m_chain.size() - 1; (int)i >= state; i--)
    m_chain.replace(i, 1);
  delete m_marked_states.pop();
  if ((int)m_chain.size() <= m_first_double) m_first_double = -1;
}

} /* namespace Common */
