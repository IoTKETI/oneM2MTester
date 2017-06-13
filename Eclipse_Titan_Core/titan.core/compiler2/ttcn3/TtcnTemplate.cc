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
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include "TtcnTemplate.hh"
#include "../Identifier.hh"
#include "Templatestuff.hh"
#include "../Type.hh"
#include "../TypeCompat.hh"
#include "../SigParam.hh"
#include "../CompField.hh"
#include "../Valuestuff.hh"
#include "ArrayDimensions.hh"
#include "PatternString.hh"
#include "../main.hh"
#include "../../common/dbgnew.hh"
#include "Attributes.hh"

namespace Ttcn {

  // =================================
  // ===== Template
  // =================================

  Template::Template(const Template& p) : GovernedSimple(p),
    templatetype(p.templatetype), my_governor(p.my_governor),
    is_ifpresent(p.is_ifpresent), specific_value_checked(false),
    has_permutation(p.has_permutation), base_template(p.base_template)
  {
    switch (templatetype) {
    case TEMPLATE_ERROR:
    case TEMPLATE_NOTUSED:
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
      break;
    case SPECIFIC_VALUE:
      u.specific_value = p.u.specific_value->clone();
      break;
    case TEMPLATE_REFD:
      u.ref.ref = p.u.ref.ref->clone();
      u.ref.refd = 0;
      u.ref.refd_last = 0;
      break;
    case TEMPLATE_INVOKE:
      u.invoke.v = p.u.invoke.v->clone();
      u.invoke.t_list = p.u.invoke.t_list ? p.u.invoke.t_list->clone() : 0;
      u.invoke.ap_list = p.u.invoke.ap_list ? p.u.invoke.ap_list->clone() : 0;
      break;
    case ALL_FROM:
      u.all_from = p.u.all_from->clone();
      break;
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      u.templates = p.u.templates->clone(); // FATAL_ERROR
      break;
    case NAMED_TEMPLATE_LIST:
      u.named_templates = p.u.named_templates->clone();
      break;
    case INDEXED_TEMPLATE_LIST:
      u.indexed_templates = p.u.indexed_templates->clone();
      break;
    case VALUE_RANGE:
      u.value_range = p.u.value_range->clone();
      break;
    case BSTR_PATTERN:
    case HSTR_PATTERN:
    case OSTR_PATTERN:
      u.pattern = new string(*p.u.pattern);
      break;
    case CSTR_PATTERN:
    case USTR_PATTERN:
      u.pstring = p.u.pstring->clone();
      break;
    case DECODE_MATCH:
      u.dec_match.str_enc = p.u.dec_match.str_enc != NULL ?
        p.u.dec_match.str_enc->clone() : NULL;
      u.dec_match.target = p.u.dec_match.target->clone();
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::clone()");
      }
      u.concat.op1 = p.u.concat.op1->clone();
      u.concat.op2 = p.u.concat.op2->clone();
      break;
//    default:
//      FATAL_ERROR("Template::Template()");
    }
    length_restriction =
      p.length_restriction ? p.length_restriction->clone() : 0; // FATAL_ERR
  }

  void Template::clean_up()
  {
    switch(templatetype) {
    case TEMPLATE_ERROR:
    case TEMPLATE_NOTUSED:
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
      break;
    case SPECIFIC_VALUE:
      delete u.specific_value;
      break;
    case TEMPLATE_REFD:
      delete u.ref.ref;
      break;
    case TEMPLATE_INVOKE:
      delete u.invoke.v;
      delete u.invoke.t_list;
      delete u.invoke.ap_list;
      break;
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      delete u.templates;
      break;
    case ALL_FROM:
      delete u.all_from;
      break;
    case NAMED_TEMPLATE_LIST:
      delete u.named_templates;
      break;
    case INDEXED_TEMPLATE_LIST:
      delete u.indexed_templates;
      break;
    case VALUE_RANGE:
      delete u.value_range;
      break;
    case BSTR_PATTERN:
    case HSTR_PATTERN:
    case OSTR_PATTERN:
      delete u.pattern;
      break;
    case CSTR_PATTERN:
    case USTR_PATTERN:
      delete u.pstring;
      break;
    case DECODE_MATCH:
      if (u.dec_match.str_enc != NULL) {
        delete u.dec_match.str_enc;
      }
      delete u.dec_match.target;
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::clean_up()");
      }
      delete u.concat.op1;
      delete u.concat.op2;
      break;
//    default:
//      FATAL_ERROR("Template::clean_up()");
    }
  }

  string Template::create_stringRepr()
  {
    string ret_val;
    switch (templatetype) {
    case TEMPLATE_ERROR:
      ret_val += "<erroneous template>";
      break;
    case TEMPLATE_NOTUSED:
      ret_val += "-";
      break;
    case OMIT_VALUE:
      ret_val += "omit";
      break;
    case ANY_VALUE:
      ret_val += "?";
      break;
    case ANY_OR_OMIT:
      ret_val += "*";
      break;
    case SPECIFIC_VALUE:
      ret_val += u.specific_value->get_stringRepr();
      break;
    case TEMPLATE_REFD: {
      Template *t_last = get_template_refd_last();
      if (t_last->templatetype == TEMPLATE_REFD)
        ret_val += u.ref.ref->get_dispname();
      else ret_val += t_last->get_stringRepr();
      break; }
    case TEMPLATE_INVOKE: {
      ret_val += u.invoke.v->get_stringRepr();
      ret_val += ".invoke(";
      if(u.invoke.ap_list)
        for(size_t i = 0; i < u.invoke.ap_list->get_nof_pars(); i++) {
          if(i>0) ret_val += ", ";
          ret_val += u.invoke.ap_list->get_par(i)->get_fullname();
        }
      ret_val += ")";
      break; }
    case TEMPLATE_LIST:
      if (u.templates->get_nof_ts() > 0) {
        ret_val += "{ ";
        u.templates->append_stringRepr(ret_val);
        ret_val += " }";
      } else ret_val += "{ }";
      break;
    case NAMED_TEMPLATE_LIST:
      ret_val += "{";
      for (size_t i = 0; i < u.named_templates->get_nof_nts(); i++) {
        if (i > 0) ret_val += ", ";
        else ret_val += " ";
        NamedTemplate *nt = u.named_templates->get_nt_byIndex(i);
        ret_val += nt->get_name().get_dispname();
        ret_val += " := ";
        ret_val += nt->get_template()->get_stringRepr();
      }
      ret_val += " }";
      break;
    case INDEXED_TEMPLATE_LIST:
      ret_val += "{";
      for (size_t i = 0; i < u.indexed_templates->get_nof_its(); i++) {
        if (i > 0) ret_val += ", ";
        else ret_val += " [";
        IndexedTemplate *it = u.indexed_templates->get_it_byIndex(i);
        (it->get_index()).append_stringRepr(ret_val);
        ret_val += "] := ";
        ret_val += it->get_template()->get_stringRepr();
      }
      ret_val += "}";
      break;
    case VALUE_LIST:
      ret_val += "(";
      u.templates->append_stringRepr(ret_val);
      ret_val += ")";
      break;
    case COMPLEMENTED_LIST:
      ret_val += "complement(";
      u.templates->append_stringRepr(ret_val);
      ret_val += ")";
      break;
    case VALUE_RANGE:
      u.value_range->append_stringRepr(ret_val);
      break;
    case SUPERSET_MATCH:
      ret_val += "superset(";
      u.templates->append_stringRepr(ret_val);
      ret_val += ")";
      break;
    case SUBSET_MATCH:
      ret_val += "subset(";
      u.templates->append_stringRepr(ret_val);
      ret_val += ")";
      break;
    case PERMUTATION_MATCH:
      ret_val += "permutation(";
      u.templates->append_stringRepr(ret_val);
      ret_val += ")";
      break;
    case BSTR_PATTERN:
      ret_val += "'";
      ret_val += *u.pattern;
      ret_val += "'B";
      break;
    case HSTR_PATTERN:
      ret_val += "'";
      ret_val += *u.pattern;
      ret_val += "'H";
      break;
    case OSTR_PATTERN:
      ret_val += "'";
      ret_val += *u.pattern;
      ret_val += "'O";
      break;
    case CSTR_PATTERN:
    case USTR_PATTERN:
      ret_val += "pattern ";
      if (u.pstring->get_nocase()) {
        ret_val += "@nocase ";
      }
      ret_val += "\"";
      ret_val += u.pstring->get_full_str();
      ret_val += "\"";
      break;
    case DECODE_MATCH:
      ret_val += "decmatch ";
      if (u.dec_match.str_enc != NULL) {
        ret_val += "(";
        ret_val += u.dec_match.str_enc->get_val_str();
        ret_val += ") ";
      }
      ret_val += u.dec_match.target->get_expr_governor(
        Type::EXPECTED_TEMPLATE)->get_stringRepr();
      ret_val += ": ";
      ret_val += u.dec_match.target->get_Template()->create_stringRepr();
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::create_stringRepr()");
      }
      ret_val += u.concat.op1->create_stringRepr();
      ret_val += " & ";
      ret_val += u.concat.op2->create_stringRepr();
      break;
    default:
      ret_val += "<unknown template>";
      break;
    }
    if (length_restriction) length_restriction->append_stringRepr(ret_val);
    if (is_ifpresent) ret_val += " ifpresent";
    return ret_val;
  }

  Template::Template(templatetype_t tt)
    : GovernedSimple(S_TEMPLATE),
      templatetype(tt), my_governor(0), length_restriction(0),
      is_ifpresent(false), specific_value_checked(false),
      has_permutation(false), flattened(true), base_template(0)
  {
    switch (tt) {
    case TEMPLATE_ERROR:
    case TEMPLATE_NOTUSED:
    case OMIT_VALUE:
    //case ANY_VALUE:
    //case ANY_OR_OMIT:
      break;
    default:
      FATAL_ERROR("Template::Template()");
    }
  }

  Template::Template(Value *v)
    : GovernedSimple(S_TEMPLATE),
      templatetype(TEMPLATE_ERROR), my_governor(0), length_restriction(0),
      is_ifpresent(false), specific_value_checked(false),
      has_permutation(false), flattened(true), base_template(0)
  {
    if (!v) FATAL_ERROR("Template::Template()");
    switch (v->get_valuetype()) {
    case Value::V_ANY_VALUE:
    case Value::V_ANY_OR_OMIT:
      templatetype = v->get_valuetype() == Value::V_ANY_VALUE ?
        ANY_VALUE : ANY_OR_OMIT;
      set_length_restriction(v->take_length_restriction());
      delete v;
      break;
    case Value::V_OMIT:
      templatetype = OMIT_VALUE;
      delete v;
      break;
    case Value::V_EXPR:
      if (use_runtime_2 && v->get_optype() == Value::OPTYPE_CONCAT) {
        // convert the operands to templates with recursive calls to this constructor
        // only in RT2
        templatetype = TEMPLATE_CONCAT;
        u.concat.op1 = new Template(v->get_concat_operand(true)->clone());
        u.concat.op1->set_location(*v->get_concat_operand(true));
        u.concat.op2 = new Template(v->get_concat_operand(false)->clone());
        u.concat.op2->set_location(*v->get_concat_operand(false));
        delete v;
        break;
      }
      // other expressions are specific value templates
    default:
      templatetype = SPECIFIC_VALUE;
      u.specific_value = v;
      break;
    }
  }

  Template::Template(Ref_base *p_ref)
    : GovernedSimple(S_TEMPLATE),
      templatetype(TEMPLATE_REFD), my_governor(0), length_restriction(0),
      is_ifpresent(false), specific_value_checked(false),
      has_permutation(false), base_template(0)
  {
    if(!p_ref) FATAL_ERROR("Template::Template()");
    u.ref.ref=p_ref;
    u.ref.refd=0;
    u.ref.refd_last=0;
  }

  Template::Template(templatetype_t tt, Templates *ts)
    : GovernedSimple(S_TEMPLATE),
      templatetype(tt), my_governor(0), length_restriction(0),
      is_ifpresent(false), specific_value_checked(false),
      has_permutation(false), flattened(true), base_template(0)
  {
    switch (tt) {
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      break;
    default:
      FATAL_ERROR("Template::Template()");
    }
    if (!ts) FATAL_ERROR("Template::Template()");
    u.templates = ts;
    if (tt == TEMPLATE_LIST) {
      size_t nof_ts = ts->get_nof_ts();
      for (size_t i = 0; i < nof_ts; i++) {
        if (ts->get_t_byIndex(i)->templatetype == PERMUTATION_MATCH) {
          has_permutation = true;
          break;
        }
      }
    }
  }

  Template::Template(Template *t)
  : GovernedSimple(S_TEMPLATE)
  , templatetype(ALL_FROM), my_governor(0), length_restriction(0)
  , is_ifpresent(false), specific_value_checked(false)
  , has_permutation(false), flattened(true), base_template(0)
  {
    u.all_from = t;
    // t is usually a SPECIFIC_VALUE
    // t->u.specific_value is a V_UNDEF_LOWERID
    // calling set_lowerid_to_ref is too soon (my_scope is not set yet)
  }

  Template::Template(NamedTemplates *nts)
    : GovernedSimple(S_TEMPLATE),
      templatetype(NAMED_TEMPLATE_LIST), my_governor(0), length_restriction(0),
      is_ifpresent(false), specific_value_checked(false),
      has_permutation(false), flattened(true), base_template(0)
  {
    if (!nts) FATAL_ERROR("Template::Template()");
    u.named_templates = nts;
  }

  Template::Template(IndexedTemplates *its)
    : GovernedSimple(S_TEMPLATE),
      templatetype(INDEXED_TEMPLATE_LIST), my_governor(0),
      length_restriction(0), is_ifpresent(false),
      specific_value_checked(false), has_permutation(false), flattened(true),
      base_template(0)
  {
    if (!its) FATAL_ERROR("Template::Template()");
    u.indexed_templates = its;
    size_t nof_its = its->get_nof_its();
    for (size_t i = 0; i < nof_its; i++) {
      if (its->get_it_byIndex(i)->get_template()->templatetype ==
        PERMUTATION_MATCH) {
        has_permutation = true;
        break;
      }
    }
  }

  Template::Template(ValueRange *vr)
    : GovernedSimple(S_TEMPLATE),
      templatetype(VALUE_RANGE), my_governor(0), length_restriction(0),
      is_ifpresent(false), specific_value_checked(false),
      has_permutation(false), flattened(true), base_template(0)
  {
    if (!vr) FATAL_ERROR("Template::Template()");
    u.value_range = vr;
  }

  Template::Template(templatetype_t tt, string *p_patt)
    : GovernedSimple(S_TEMPLATE),
      templatetype(tt), my_governor(0), length_restriction(0),
      is_ifpresent(false), specific_value_checked(false),
      has_permutation(false), flattened(true), base_template(0)
  {
    switch (tt) {
    case BSTR_PATTERN:
    case HSTR_PATTERN:
    case OSTR_PATTERN:
      break;
    default:
      FATAL_ERROR("Template::Template()");
    }
    if (!p_patt) FATAL_ERROR("Template::Template()");
    u.pattern = p_patt;
  }

  Template::Template(PatternString *p_ps)
    : GovernedSimple(S_TEMPLATE),
      templatetype(CSTR_PATTERN), my_governor(0), length_restriction(0),
      is_ifpresent(false), specific_value_checked(false),
      has_permutation(false), flattened(true), base_template(0)
  {
    if (!p_ps) FATAL_ERROR("Template::Template()");
    u.pstring = p_ps;
  }
  
  Template::Template(Value* v, TemplateInstance* ti)
    : GovernedSimple(S_TEMPLATE),
      templatetype(DECODE_MATCH), my_governor(0), length_restriction(0),
      is_ifpresent(false), specific_value_checked(false),
      has_permutation(false), flattened(true), base_template(0)
  {
    if (ti == NULL) {
      FATAL_ERROR("Template::Template()");
    }
    u.dec_match.str_enc = v;
    u.dec_match.target = ti;
  }

  Template::~Template()
  {
    clean_up();
    delete length_restriction;
  }

  Template *Template::clone() const
  {
    return new Template(*this);
  }

  void Template::set_fullname(const string& p_fullname)
  {
    GovernedSimple::set_fullname(p_fullname);
    switch (templatetype) {
    case TEMPLATE_ERROR:
    case TEMPLATE_NOTUSED:
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
    case BSTR_PATTERN:
    case HSTR_PATTERN:
    case OSTR_PATTERN:
      break;
    case SPECIFIC_VALUE:
      u.specific_value->set_fullname(p_fullname);
      break;
    case TEMPLATE_REFD:
      u.ref.ref->set_fullname(p_fullname);
      break;
    case TEMPLATE_INVOKE:
      u.invoke.v->set_fullname(p_fullname);
      if(u.invoke.t_list) u.invoke.t_list->set_fullname(p_fullname);
      if(u.invoke.ap_list) u.invoke.ap_list->set_fullname(p_fullname);
      break;
    case TEMPLATE_LIST:
      u.templates->set_fullname(p_fullname);
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++)
        u.templates->get_t_byIndex(i)->set_fullname(
          p_fullname + "[" + Int2string(i) + "]");
      break;
    case INDEXED_TEMPLATE_LIST:
      u.indexed_templates->set_fullname(p_fullname);
      for (size_t i = 0; i < u.indexed_templates->get_nof_its(); i++)
        u.indexed_templates->get_it_byIndex(i)->set_fullname(
          p_fullname + "[" + Int2string(i) + "]");
      break;
    case NAMED_TEMPLATE_LIST:
      u.named_templates->set_fullname(p_fullname);
      break;
    case ALL_FROM:
      u.all_from->set_fullname(p_fullname);
      break;
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      u.templates->set_fullname(p_fullname);
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++)
        u.templates->get_t_byIndex(i)->set_fullname(
          p_fullname + ".list_item(" + Int2string(i) + ")");
      break;
    case VALUE_RANGE:
      u.value_range->set_fullname(p_fullname);
      break;
    case CSTR_PATTERN:
    case USTR_PATTERN:
      u.pstring->set_fullname(p_fullname);
      break;
    case DECODE_MATCH:
      if (u.dec_match.str_enc != NULL) {
        u.dec_match.str_enc->set_fullname(p_fullname + ".<string_encoding>");
      }
      u.dec_match.target->set_fullname(p_fullname + ".<decoding_target>");
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::set_fullname()");
      }
      u.concat.op1->set_fullname(p_fullname + ".operand1");
      u.concat.op2->set_fullname(p_fullname + ".operand2");
      break;
//    default:
//      FATAL_ERROR("Template::set_fullname()");
    }
    if (length_restriction)
      length_restriction->set_fullname(p_fullname + ".<length_restriction>");
  }

  void Template::set_my_scope(Scope *p_scope)
  {
    GovernedSimple::set_my_scope(p_scope);
    switch (templatetype) {
    case TEMPLATE_ERROR:
    case TEMPLATE_NOTUSED:
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
    case BSTR_PATTERN:
    case HSTR_PATTERN:
    case OSTR_PATTERN:
      break;
    case SPECIFIC_VALUE:
      u.specific_value->set_my_scope(p_scope);
      break;
    case TEMPLATE_REFD:
      u.ref.ref->set_my_scope(p_scope);
      break;
    case TEMPLATE_INVOKE:
      u.invoke.v->set_my_scope(p_scope);
      if(u.invoke.t_list) u.invoke.t_list->set_my_scope(p_scope);
      if(u.invoke.ap_list) u.invoke.ap_list->set_my_scope(p_scope);
      break;
    case ALL_FROM:
      u.all_from->set_my_scope(p_scope);
      break;
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      u.templates->set_my_scope(p_scope);
      break;
    case NAMED_TEMPLATE_LIST:
      u.named_templates->set_my_scope(p_scope);
      break;
    case INDEXED_TEMPLATE_LIST:
      u.indexed_templates->set_my_scope(p_scope);
      break;
    case VALUE_RANGE:
      u.value_range->set_my_scope(p_scope);
      break;
    case CSTR_PATTERN:
    case USTR_PATTERN:
      u.pstring->set_my_scope(p_scope);
      break;
    case DECODE_MATCH:
      if (u.dec_match.str_enc != NULL) {
        u.dec_match.str_enc->set_my_scope(p_scope);
      }
      u.dec_match.target->set_my_scope(p_scope);
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::set_my_scope()");
      }
      u.concat.op1->set_my_scope(p_scope);
      u.concat.op2->set_my_scope(p_scope);
      break;
//    default:
//      FATAL_ERROR("Template::set_my_scope()");
    }
    if (length_restriction) length_restriction->set_my_scope(p_scope);
  }

  void Template::set_genname_recursive(const string& p_genname)
  {
    set_genname(p_genname);
    switch (templatetype) {
    case TEMPLATE_LIST: {
      if (!my_governor) return; // error recovery
      Type *type = my_governor->get_type_refd_last();
      Int offset;
      if (type->get_typetype() == Type::T_ARRAY)
        offset = type->get_dimension()->get_offset();
      else offset = 0;
      size_t nof_ts = u.templates->get_nof_ts();
      for (size_t i = 0; i < nof_ts; i++) {
        string embedded_genname(p_genname);
        embedded_genname += '[';
        embedded_genname += Int2string(offset + i);
        embedded_genname += ']';
        u.templates->get_t_byIndex(i)->set_genname_recursive(embedded_genname);
      }
      break; }
    case NAMED_TEMPLATE_LIST: {
      if (!my_governor) return; // error recovery
      Type *type = my_governor->get_type_refd_last();
      size_t nof_nts = u.named_templates->get_nof_nts();
      for (size_t i = 0; i < nof_nts; i++) {
        NamedTemplate *nt = u.named_templates->get_nt_byIndex(i);
        string embedded_genname(p_genname);
        embedded_genname += '.';
        if (type->get_typetype() == Type::T_ANYTYPE)
          embedded_genname += "AT_";
        embedded_genname += nt->get_name().get_name();
        embedded_genname += "()";
        nt->get_template()->set_genname_recursive(embedded_genname);
      }
      break; }
    default:
      break;
    }
  }

  void Template::set_genname_prefix(const char *p_genname_prefix)
  {
    GovernedSimple::set_genname_prefix(p_genname_prefix);
    switch (templatetype) {
    case TEMPLATE_LIST:
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++)
        u.templates->get_t_byIndex(i)->set_genname_prefix(p_genname_prefix);
      break;
    case NAMED_TEMPLATE_LIST:
      for (size_t i = 0; i < u.named_templates->get_nof_nts(); i++)
        u.named_templates->get_nt_byIndex(i)->get_template()
          ->set_genname_prefix(p_genname_prefix);
      break;
    case INDEXED_TEMPLATE_LIST:
      for (size_t i = 0; i < u.indexed_templates->get_nof_its(); i++)
        u.indexed_templates->get_it_byIndex(i)->get_template()
          ->set_genname_prefix(p_genname_prefix);
      break;
    default:
      break;
    }
  }

  void Template::set_code_section(code_section_t p_code_section)
  {
    GovernedSimple::set_code_section(p_code_section);
    switch (templatetype) {
    case SPECIFIC_VALUE:
      u.specific_value->set_code_section(p_code_section);
      break;
    case TEMPLATE_REFD:
      u.ref.ref->set_code_section(p_code_section);
      break;
    case TEMPLATE_INVOKE:
      u.invoke.v->set_code_section(p_code_section);
      if(u.invoke.t_list) u.invoke.t_list->set_code_section(p_code_section);
      if(u.invoke.ap_list)
        for(size_t i = 0; i < u.invoke.ap_list->get_nof_pars(); i++)
            u.invoke.ap_list->get_par(i)->set_code_section(p_code_section);
      break;
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++)
        u.templates->get_t_byIndex(i)->set_code_section(p_code_section);
      break;
    case NAMED_TEMPLATE_LIST:
      for (size_t i = 0; i < u.named_templates->get_nof_nts(); i++)
        u.named_templates->get_nt_byIndex(i)->get_template()
          ->set_code_section(p_code_section);
      break;
    case INDEXED_TEMPLATE_LIST:
      for (size_t i = 0; i <u.indexed_templates->get_nof_its(); i++)
        u.indexed_templates->get_it_byIndex(i)
          ->set_code_section(p_code_section);
      break;
    case VALUE_RANGE:
      u.value_range->set_code_section(p_code_section);
      break;
    case CSTR_PATTERN:
    case USTR_PATTERN:
      u.pstring->set_code_section(p_code_section);
      break;
    case DECODE_MATCH:
      if (u.dec_match.str_enc != NULL) {
        u.dec_match.str_enc->set_code_section(p_code_section);
      }
      u.dec_match.target->set_code_section(p_code_section);
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::set_code_section()");
      }
      u.concat.op1->set_code_section(p_code_section);
      u.concat.op2->set_code_section(p_code_section);
      break;
    default:
      break;
    }
    if (length_restriction)
      length_restriction->set_code_section(p_code_section);
  }

  void Template::set_templatetype(templatetype_t p_templatetype)
  {
    if (p_templatetype == templatetype) return;
    if (p_templatetype == TEMPLATE_ERROR) {
      clean_up();
      templatetype = TEMPLATE_ERROR;
      return;
    }
    switch (templatetype) {
    case SPECIFIC_VALUE: // current type
      switch(p_templatetype) {
      case TEMPLATE_REFD: {
        Value *v = u.specific_value;
        u.ref.ref = v->steal_ttcn_ref_base();
        u.ref.refd = 0;
        u.ref.refd_last = 0;
        delete v;
        break; }
      case TEMPLATE_INVOKE: {
        Value *v = u.specific_value;
        v->steal_invoke_data(u.invoke.v, u.invoke.t_list, u.invoke.ap_list);
        delete v;
        break; }
      default:
        FATAL_ERROR("Template::set_templatetype()");
      }
      break;
    case TEMPLATE_LIST: // current type
      if (p_templatetype == NAMED_TEMPLATE_LIST) {
        // TEMPLATE_LIST -> NAMED_TEMPLATE_LIST:
        // value list -> assignment notation
        // applicable to record types, signatures and empty set types
        if (!my_governor) FATAL_ERROR("Template::set_templatetype()");
        Type *t = my_governor->get_type_refd_last();
        size_t nof_comps = t->get_nof_comps(); // "expected" nr of components
        Templates *sts = u.templates;
        size_t nof_temps = sts->get_nof_ts(); // "actual" nr in the list
        Type::typetype_t tt = t->get_typetype();
        switch (tt) {
        case Type::T_SEQ_T:
        case Type::T_SEQ_A:
        case Type::T_SIGNATURE:
          break;
        case Type::T_SET_A:
        case Type::T_SET_T:
          if (nof_temps == 0 && nof_comps == 0) break;
        default:
          FATAL_ERROR("Template::set_templatetype()");
        }
        // If it's a record or set template, allow fewer elements than
        // what is required, because implicit omit may take care of it.
        // If it's a signature template, be precise.
        bool allow_fewer = false;
        switch (my_governor->get_typetype()) {
        case Type::T_SEQ_T: case Type::T_SET_T:
        case Type::T_SEQ_A: case Type::T_SET_A:
          allow_fewer = true;
          break;
        case Type::T_SIGNATURE: // be precise
          break;
        default: // not possible, fatal error ?
          break;
        }
        if ( nof_temps > nof_comps
          || (!allow_fewer && (nof_temps < nof_comps))) {
          error("Too %s elements in value list notation for type `%s': "
            "%lu was expected instead of %lu",
            nof_temps > nof_comps ? "many" : "few",
              t->get_typename().c_str(),
              (unsigned long) nof_comps, (unsigned long) nof_temps);
        }
        size_t upper_limit; // min(nof_temps, nof_comps)
        bool all_notused;
        if (nof_temps <= nof_comps) {
          upper_limit = nof_temps;
          all_notused = true;
        } else {
          upper_limit = nof_comps;
          all_notused = false;
        }
        u.named_templates = new NamedTemplates;
        for (size_t i = 0; i < upper_limit; i++) {
          Template*& temp = sts->get_t_byIndex(i);
          if (temp->templatetype == TEMPLATE_NOTUSED) continue;
          all_notused = false;
          NamedTemplate *nt = new
            NamedTemplate(t->get_comp_id_byIndex(i).clone(), temp);
          nt->set_location(*temp);
          u.named_templates->add_nt(nt);
          temp = 0;
        }
        u.named_templates->set_my_scope(get_my_scope());
        u.named_templates->set_fullname(get_fullname());
        delete sts;
        if (all_notused && nof_temps > 0 && tt != Type::T_SIGNATURE)
          warning("All elements of value list notation for type `%s' are "
            "not used symbols (`-')", t->get_typename().c_str());
      } else FATAL_ERROR("Template::set_templatetype()");
      break;
    case CSTR_PATTERN: // current type
      if (p_templatetype == USTR_PATTERN)
        templatetype = USTR_PATTERN;
      else
        FATAL_ERROR("Template::set_templatetype()");
      break;
    case TEMPLATE_CONCAT:
      switch (p_templatetype) {
      case SPECIFIC_VALUE:
      case ANY_VALUE:
      case BSTR_PATTERN:
      case HSTR_PATTERN:
      case OSTR_PATTERN:
        // OK
        delete u.concat.op1;
        delete u.concat.op2;
        break;
      default:
        FATAL_ERROR("Template::set_templatetype()");
      }
      break;
    default:
      FATAL_ERROR("Template::set_templatetype()");
    }
    templatetype = p_templatetype;
  }

  const char *Template::get_templatetype_str() const
  {
    switch(templatetype) {
    case TEMPLATE_ERROR:
      return "erroneous template";
    case TEMPLATE_NOTUSED:
      return "not used symbol";
    case OMIT_VALUE:
      return "omit value";
    case ANY_VALUE:
      return "any value";
    case ANY_OR_OMIT:
      return "any or omit";
    case SPECIFIC_VALUE:
      return "specific value";
    case TEMPLATE_REFD:
      return "referenced template";
    case TEMPLATE_INVOKE:
      return "template returning invoke";
    case ALL_FROM:
      return "template with 'all from'";
    case TEMPLATE_LIST:
      return "value list notation";
    case NAMED_TEMPLATE_LIST:
      return "assignment notation";
    case INDEXED_TEMPLATE_LIST:
      return "assignment notation with array indices";
    case VALUE_RANGE:
      return "value range match";
    case VALUE_LIST:
      return "value list match";
    case COMPLEMENTED_LIST:
      return "complemented list match";
    case SUPERSET_MATCH:
      return "superset match";
    case SUBSET_MATCH:
      return "subset match";
    case PERMUTATION_MATCH:
      return "permutation match";
    case BSTR_PATTERN:
      return "bitstring pattern";
    case HSTR_PATTERN:
      return "hexstring pattern";
    case OSTR_PATTERN:
      return "octetstring pattern";
    case CSTR_PATTERN:
      return "character string pattern";
    case USTR_PATTERN:
      return "universal string pattern";
    case DECODE_MATCH:
      return "decoded content match";
    case TEMPLATE_CONCAT:
      return "template concatenation";
    default:
      return "unknown template";
    }
  }

  bool Template::is_undef_lowerid()
  {
    return templatetype == SPECIFIC_VALUE &&
      u.specific_value->is_undef_lowerid();
  }

  void Template::set_lowerid_to_ref()
  {
    switch (templatetype) {
    case SPECIFIC_VALUE:
      u.specific_value->set_lowerid_to_ref();
      if (u.specific_value->get_valuetype() == Value::V_REFD) {
        Common::Assignment *t_ass =
          u.specific_value->get_reference()->get_refd_assignment(false);
        if (t_ass) {
          switch (t_ass->get_asstype()) {
          case Common::Assignment::A_MODULEPAR_TEMP:
          case Common::Assignment::A_TEMPLATE:
          case Common::Assignment::A_VAR_TEMPLATE:
          case Common::Assignment::A_PAR_TEMPL_IN:
          case Common::Assignment::A_PAR_TEMPL_OUT:
          case Common::Assignment::A_PAR_TEMPL_INOUT:
          case Common::Assignment::A_FUNCTION_RTEMP:
          case Common::Assignment::A_EXT_FUNCTION_RTEMP:
            set_templatetype(TEMPLATE_REFD);
          default:
            break;
          }
        } else set_templatetype(TEMPLATE_ERROR);
      }
      break;
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++)
        u.templates->get_t_byIndex(i)->set_lowerid_to_ref();
      break;
    case VALUE_RANGE:
      u.value_range->set_lowerid_to_ref();
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::set_lowerid_to_ref()");
      }
      u.concat.op1->set_lowerid_to_ref();
      u.concat.op2->set_lowerid_to_ref();
      break;
    default:
      break;
    }
  }

  Type::typetype_t Template::get_expr_returntype(Type::expected_value_t exp_val)
  {
    switch (templatetype) {
    case TEMPLATE_ERROR:
      return Type::T_ERROR;
    case SPECIFIC_VALUE:
      return u.specific_value->get_expr_returntype(exp_val);
    case TEMPLATE_REFD: {
      Type *t = get_expr_governor(exp_val);
      if (t) return t->get_type_refd_last()->get_typetype_ttcn3();
      else return Type::T_ERROR; }
    case TEMPLATE_INVOKE: {
      Type *t = get_expr_governor(exp_val);
      if(t) return t->get_type_refd_last()->get_typetype_ttcn3();
      else return Type::T_ERROR; }
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++) {
        Type::typetype_t tt = u.templates->get_t_byIndex(i)
          ->get_expr_returntype(exp_val);
        if (tt != Type::T_UNDEF) return tt;
      }
      return Type::T_UNDEF;
    case VALUE_RANGE:
      return u.value_range->get_expr_returntype(exp_val);
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
      return Type::T_SETOF;
    case BSTR_PATTERN:
      return Type::T_BSTR;
    case HSTR_PATTERN:
      return Type::T_HSTR;
    case OSTR_PATTERN:
      return Type::T_OSTR;
    case CSTR_PATTERN:
      return Type::T_CSTR;
    case USTR_PATTERN:
      return Type::T_USTR;
    case TEMPLATE_CONCAT: {
      if (!use_runtime_2) {
        FATAL_ERROR("Template::get_expr_returntype()");
      }
      Type::typetype_t tt1 = u.concat.op1->get_expr_returntype(exp_val);
      Type::typetype_t tt2 = u.concat.op2->get_expr_returntype(exp_val);
      if (tt1 == Type::T_UNDEF) {
        if (tt2 == Type::T_UNDEF) {
          return Type::T_UNDEF;
        }
        return tt2;
      }
      else {
        if (tt2 == Type::T_UNDEF) {
          return tt1;
        }
        if ((tt1 == Type::T_CSTR && tt2 == Type::T_USTR) ||
            (tt1 == Type::T_USTR && tt2 == Type::T_CSTR)) {
          return Type::T_USTR;
        }
        if (tt1 != tt2) {
          return Type::T_ERROR;
        }
        return tt1;
      }
    }
    default:
      return Type::T_UNDEF;
    }
  }

  Type *Template::get_expr_governor(Type::expected_value_t exp_val)
  {
    if (my_governor) return my_governor;
    switch (templatetype) {
    case SPECIFIC_VALUE:
      return u.specific_value->get_expr_governor(exp_val);
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++) {
        Type *t = u.templates->get_t_byIndex(i)->get_expr_governor(exp_val);
        if (t) return t;
      }
      return 0;
    case VALUE_RANGE:
      return u.value_range->get_expr_governor(exp_val);
    case TEMPLATE_REFD: {
      Type *t = u.ref.ref->get_refd_assignment()->get_Type()
        ->get_field_type(u.ref.ref->get_subrefs(), exp_val);
      if (!t) set_templatetype(TEMPLATE_ERROR);
      return t;
      }
    case TEMPLATE_INVOKE: {
      Type *t = u.invoke.v->get_expr_governor(Type::EXPECTED_DYNAMIC_VALUE);
      if(!t) {
        if(u.invoke.v->get_valuetype() != Value::V_ERROR)
          u.invoke.v->error("A value of type function expected");
        goto error;
      }
      t = t->get_type_refd_last();
      switch(t->get_typetype()) {
      case Type::T_FUNCTION:
        if(exp_val==Type::EXPECTED_DYNAMIC_VALUE && t->get_returns_template()){
          error("Reference to a value was expected instead of a "
            "template of type `%s'"
            , t->get_function_return_type()->get_typename().c_str());
          goto error;
        }
        return t->get_function_return_type();
      case Type::T_ALTSTEP:
        goto error;
      default:
        u.invoke.v->error("A value of type function expected instead of `%s'",
          t->get_typename().c_str());
        goto error;
      }
      break; }
    case TEMPLATE_CONCAT:
      {
        Type* t1_gov = u.concat.op1->get_expr_governor(exp_val);
        Type* t2_gov = u.concat.op2->get_expr_governor(exp_val);
        if (t1_gov != NULL) {
          if (t2_gov != NULL) { // both have governors
            // return the type that is compatible with both (if there is no type mismatch)
            if (t1_gov->is_compatible(t2_gov, NULL, NULL)) {
              return t1_gov;
            }
            else {
              return t2_gov;
            }
          }
          else {
            return t1_gov;
          }
        }
        else { // t1 has no governor
          if (t2_gov != NULL) {
            return t2_gov;
          }
          else {
            return NULL; // neither has governor
          }
        }
      }
    default:
      return Type::get_pooltype(get_expr_returntype(exp_val));
    }
  error:
    set_templatetype(TEMPLATE_ERROR);
    return 0;
  }

  void Template::set_my_governor(Type *p_gov)
  {
    if (!p_gov)
      FATAL_ERROR("Template::set_my_governor(): NULL parameter");
    my_governor=p_gov;
  }

  Type *Template::get_my_governor() const
  {
    return my_governor;
  }

  void Template::set_length_restriction(LengthRestriction *p_lr)
  {
    if (p_lr == NULL) return;
    if (length_restriction) FATAL_ERROR("Template::set_length_restriction()");
    length_restriction = p_lr;
  }

  Template::completeness_t
    Template::get_completeness_condition_seof(bool incomplete_allowed)
  {
    if (!incomplete_allowed) return C_MUST_COMPLETE;
    else if (!base_template) return C_MAY_INCOMPLETE;
    else {
      Template *t = base_template->get_template_refd_last();
      switch (t->templatetype) {
      // partial overwriting is allowed
      case TEMPLATE_ERROR: // to suppress more errors
      case TEMPLATE_NOTUSED:  // modifying a modified template
      case ANY_VALUE: // in case of ?
      case ANY_OR_OMIT: // in case of *
      case TEMPLATE_REFD: // e.g. the actual value of a formal parameter
      case TEMPLATE_INVOKE:
      case NAMED_TEMPLATE_LIST: // t is erroneous
      case INDEXED_TEMPLATE_LIST:
        return C_MAY_INCOMPLETE;
      case TEMPLATE_LIST:
        switch (my_governor->get_type_refd_last()->get_typetype()) {
        case Type::T_SEQOF:
        case Type::T_SETOF:
          // only the first elements can be incomplete
          return C_PARTIAL;
        default:
          // we are in error recovery
          return C_MAY_INCOMPLETE;
        }
        break; // should not get here
        default:
          // partial overwriting is not allowed for literal specific values,
          // matching ranges/lists/sets and patterns
          return C_MUST_COMPLETE;
      }
    }
  }

  Template::completeness_t Template::get_completeness_condition_choice
    (bool incomplete_allowed, const Identifier& p_fieldname)
  {
    if (!incomplete_allowed) return C_MUST_COMPLETE;
    else if (!base_template) return C_MAY_INCOMPLETE;
    else {
      Template *t = base_template->get_template_refd_last();
      switch (t->templatetype) {
      // partial overwriting is allowed
      case TEMPLATE_ERROR: // to suppress more errors
      case TEMPLATE_NOTUSED:  // t is erroneous
      case ANY_VALUE: // in case of ?
      case ANY_OR_OMIT: // in case of *
      case TEMPLATE_REFD: // e.g. the actual value of a formal parameter
      case TEMPLATE_INVOKE:
      case TEMPLATE_LIST: // t is erroneous
        return C_MAY_INCOMPLETE;
      case NAMED_TEMPLATE_LIST: // some fields may be missing
        if (t->u.named_templates->has_nt_withName(p_fieldname))
          return C_MAY_INCOMPLETE;
        else return C_MUST_COMPLETE;
      default:
        // partial overwriting is not allowed for literal specific values,
        // matching ranges/lists/sets and patterns
        return C_MUST_COMPLETE;
      }
    }
  }

  void Template::add_named_temp(NamedTemplate* nt) {
    if (templatetype != NAMED_TEMPLATE_LIST)
      FATAL_ERROR("Template::add_named_temp()");
    u.named_templates->add_nt(nt);
  }

  Value *Template::get_specific_value() const
  {
    if (templatetype != SPECIFIC_VALUE)
      FATAL_ERROR("Template::get_specific_value()");
    return u.specific_value;
  }

  Ref_base *Template::get_reference() const
  {
    if (templatetype != TEMPLATE_REFD)
      FATAL_ERROR("Template::get_reference()");
    return u.ref.ref;
  }

  ValueRange *Template::get_value_range() const
  {
    if (templatetype != VALUE_RANGE)
      FATAL_ERROR("Template::get_value_range()");
    return u.value_range;
  }

  PatternString* Template::get_cstr_pattern() const
  {
    if (templatetype != CSTR_PATTERN)
      FATAL_ERROR("Template::get_cstr_pattern()");
    return u.pstring;
  }

  PatternString* Template::get_ustr_pattern() const
  {
    if (templatetype != USTR_PATTERN)
      FATAL_ERROR("Template::get_ustr_pattern()");
    return u.pstring;
  }

  size_t Template::get_nof_comps() const
  {
    switch (templatetype) {
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      return u.templates->get_nof_ts();
    case NAMED_TEMPLATE_LIST:
      return u.named_templates->get_nof_nts();
    case INDEXED_TEMPLATE_LIST:
      return u.indexed_templates->get_nof_its();
    default:
      FATAL_ERROR("Template::get_of_comps()");
      return 0;
    }
  }

  Template *Template::get_temp_byIndex(size_t n) const
  {
    switch (templatetype) {
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      return u.templates->get_t_byIndex(n);
    default:
      FATAL_ERROR("Template::get_temp_byIndex()");
      return 0;
    }
  }

  IndexedTemplate *Template::get_indexedtemp_byIndex(size_t n) const
  {
    if (templatetype != INDEXED_TEMPLATE_LIST)
      FATAL_ERROR("Template::get_indexedtemp_byIndex()");
    return u.indexed_templates->get_it_byIndex(n);
  }

  NamedTemplate *Template::get_namedtemp_byIndex(size_t n) const
  {
    if (templatetype != NAMED_TEMPLATE_LIST)
      FATAL_ERROR("Template::get_namedtemp_byIndex()");
    return u.named_templates->get_nt_byIndex(n);
  }
  
  NamedTemplate* Template::get_namedtemp_byName(const Identifier& name) const
  {
    if (templatetype != NAMED_TEMPLATE_LIST) {
      FATAL_ERROR("Template::get_namedtemp_byName()");
    }
    return u.named_templates->has_nt_withName(name) ?
      u.named_templates->get_nt_byName(name) : NULL;
  }

  Template *Template::get_all_from() const
  {
    if (templatetype != ALL_FROM)
      FATAL_ERROR("Template::get_all_from()");
    return u.all_from;
  }

  // Not applicable to INDEXED_TEMPLATE_LIST nodes.  The actual number of
  // elements is not known.
  size_t Template::get_nof_listitems() const
  {
    if (templatetype != TEMPLATE_LIST)
      FATAL_ERROR("Template::get_nof_listitems()");
    if (has_permutation) {
      size_t nof_ts = u.templates->get_nof_ts(), ret_val = 0;
      for (size_t i = 0; i < nof_ts; i++) {
        Template *t = u.templates->get_t_byIndex(i);
        if (t->templatetype == PERMUTATION_MATCH)
          ret_val += t->u.templates->get_nof_ts();
        else ret_val++;
      }
      return ret_val;
    } else return u.templates->get_nof_ts();
  }

  Template *Template::get_listitem_byIndex(size_t n) const
  {
    if (templatetype != TEMPLATE_LIST)
      FATAL_ERROR("Template::get_listitam_byIndex()");
    if (has_permutation) {
      size_t nof_ts = u.templates->get_nof_ts(), index = 0;
      for (size_t i = 0; i < nof_ts; i++) {
        Template *t = u.templates->get_t_byIndex(i);
        if (t->templatetype == PERMUTATION_MATCH) {
          size_t nof_perm_ts = t->u.templates->get_nof_ts();
          if (n < index + nof_perm_ts)
            return t->u.templates->get_t_byIndex(n - index);
          else index += nof_perm_ts;
        } else {
          if (n == index) return t;
          else index++;
        }
      }
      FATAL_ERROR("Template::get_listitem_byIndex(): index overflow");
      return 0;
    } else return u.templates->get_t_byIndex(n);
  }

  /** \todo revise and merge with get_template_refd() */
  Template* Template::get_template_refd_last(ReferenceChain *refch)
  {
    // return this for non-referenced templates
    if (templatetype != TEMPLATE_REFD &&
        templatetype != TEMPLATE_CONCAT) return this;
    if (templatetype == TEMPLATE_REFD) {
      // use the cached template if present
      if (u.ref.refd_last) return u.ref.refd_last;
      else {
        Common::Assignment *t_ass = u.ref.ref->get_refd_assignment();
        // escape from invalid recursion loops
        if (templatetype != TEMPLATE_REFD) return this;
        if (!t_ass) FATAL_ERROR("Template::get_template_refd_last()");
        if (t_ass->get_asstype() != Common::Assignment::A_TEMPLATE) {
          // return this if the reference does not point to a template
          u.ref.refd_last = this;
          return u.ref.refd_last;
        }
      }
      // otherwise evaluate the reference
    }
    bool destroy_refch;
    if (refch) {
      refch->mark_state();
      destroy_refch = false;
    } else {
      refch = new ReferenceChain(this, templatetype == TEMPLATE_REFD ?
        "While searching for referenced template" :
        "While evaluating template concatenation");
      destroy_refch = true;
    }
    Template *ret_val;
    if (refch->add(get_fullname())) {
      if (templatetype == TEMPLATE_REFD) {
        Template *t_refd = get_template_refd(refch);
        // get_template_refd() may set u.ref.refd_last if there are unfoldable
        // sub-references in u.ref.ref
        if (!u.ref.refd_last) {
          u.ref.refd_last = t_refd->get_template_refd_last(refch);
        }
        ret_val = u.ref.refd_last;
      }
      else {
        // evaluate the template concatenation, if the operands are known at
        // compile-time
        set_lowerid_to_ref();
        Type::typetype_t tt = get_expr_returntype(Type::EXPECTED_TEMPLATE);
        // evaluate the operands first
        Template* t1 = u.concat.op1->get_template_refd_last(refch);
        Template* t2 = u.concat.op2->get_template_refd_last(refch);
        switch (tt) {
        case Type::T_BSTR:
        case Type::T_HSTR:
        case Type::T_OSTR:
        case Type::T_CSTR:
        case Type::T_USTR: {
          // only string concatenations can be evaluated at compile-time
          // case 1: concatenating two values results in a value
          if (u.concat.op1->templatetype == SPECIFIC_VALUE &&
              u.concat.op2->templatetype == SPECIFIC_VALUE) {
            // delegate the handling of the concatenation to the Value class:
            // steal the specific values from the operands and use them in the
            // creation of the new value
            Value* v = new Value(Value::OPTYPE_CONCAT,
              u.concat.op1->u.specific_value, u.concat.op2->u.specific_value);
            // set their specific value pointers to null, so they are not
            // deleted when the template operands are deleted by set_templatetype
            u.concat.op1->u.specific_value = NULL;
            u.concat.op2->u.specific_value = NULL;
            v->set_location(*this);
            v->set_my_scope(get_my_scope());
            v->set_fullname(get_fullname());
            if (!destroy_refch) {
              // the value has the same fullname as the template, so adding it
              // to the reference chain would cause a 'circular reference' error,
              // which is why the template's name is removed from the chain here
              refch->prev_state();
              refch->mark_state();
            }
            // calling get_value_refd_last evaluates the value
            v->get_value_refd_last(destroy_refch ? NULL : refch);
            set_templatetype(SPECIFIC_VALUE);
            u.specific_value = v;
          }
          else if (t1->templatetype == SPECIFIC_VALUE &&
                   t2->templatetype == SPECIFIC_VALUE) {
            // same as case 1, but one (or both) of the operands is a reference
            // to a specific value template
            if (!t1->u.specific_value->is_unfoldable(refch) &&
                !t2->u.specific_value->is_unfoldable(refch)) {
              // in this case the referenced values cannot be stolen (since they
              // are not part of this template), so evaluate the concatenation
              // if both values are known
              Value* v;
              if (tt == Type::T_USTR) {
                v = new Value(Value::V_USTR, new ustring(
                  t1->u.specific_value->get_value_refd_last()->get_val_ustr() +
                  t2->u.specific_value->get_value_refd_last()->get_val_ustr()));
              }
              else {
                Value::valuetype_t vt;
                switch (tt) {
                case Type::T_BSTR:
                  vt = Value::V_BSTR;
                  break;
                case Type::T_HSTR:
                  vt = Value::V_HSTR;
                  break;
                case Type::T_OSTR:
                  vt = Value::V_OSTR;
                  break;
                case Type::T_CSTR:
                  vt = Value::V_CSTR;
                  break;
                default:
                  FATAL_ERROR("Template::get_template_refd_last");
                }
                v = new Value(vt, new string(
                  t1->u.specific_value->get_value_refd_last()->get_val_str() +
                  t2->u.specific_value->get_value_refd_last()->get_val_str()));
              }
              v->set_location(*this);
              v->set_my_scope(get_my_scope());
              v->set_fullname(get_fullname());
              set_templatetype(SPECIFIC_VALUE);
              u.specific_value = v;
            }
            else {
              // the values are not known at compile-time, so just leave the
              // template as it is
              break;
            }
          }
          // the rest of the cases are only possible for binary strings
          else if (tt == Type::T_BSTR || tt == Type::T_HSTR || tt == Type::T_OSTR) {
            // case 2: ? & ? = ?
            if (t1->templatetype == ANY_VALUE && t2->templatetype == ANY_VALUE &&
                t1->length_restriction == NULL && t2->length_restriction == NULL) {
              set_templatetype(ANY_VALUE);
            }
            // case 3: operands are values, patterns, '?' or '?'/'*' with fixed
            // length restriction => result is a binary string pattern
            string* patt_ptr = new string;
            bool evaluated = t1->concat_to_bin_pattern(*patt_ptr, tt) &&
              t2->concat_to_bin_pattern(*patt_ptr, tt);
            if (evaluated) {
              set_templatetype(tt == Type::T_BSTR ? BSTR_PATTERN :
                (tt == Type::T_HSTR ? HSTR_PATTERN : OSTR_PATTERN));
              u.pattern = patt_ptr;
            }
            else {
              // erroneous or cannot be evaluated => leave it as it is
              delete patt_ptr;
            }
          }
          break; }
        default:
          break;
        }
        ret_val = this;    
      }
    } else {
      // a circular reference was found
      set_templatetype(TEMPLATE_ERROR);
      ret_val = this;
    }
    if (destroy_refch) delete refch;
    else refch->prev_state();
    return ret_val;
  }

  Template* Template::get_refd_sub_template(Ttcn::FieldOrArrayRefs *subrefs,
                                            bool usedInIsbound,
                                            ReferenceChain *refch,
                                            bool silent)
  {
    if (!subrefs) return this;
    Template *t=this;
    for (size_t i=0; i<subrefs->get_nof_refs(); i++) {
      if(!t) break;
      t=t->get_template_refd_last(refch);
      t->set_lowerid_to_ref();
      switch(t->templatetype) {
      case TEMPLATE_ERROR:
        return t;
      case TEMPLATE_REFD:
      case INDEXED_TEMPLATE_LIST:
        // unfoldable stuff
        return this;
      case SPECIFIC_VALUE:
        /*(void)t->u.specific_value->get_refd_sub_value(
          subrefs, i, usedInIsbound, refch, silent); // only to report errors*/
        break;
      default:
        break;
      } // switch
      Ttcn::FieldOrArrayRef *ref=subrefs->get_ref(i);
      if(ref->get_type() == Ttcn::FieldOrArrayRef::FIELD_REF) {
        if (t->get_my_governor()->get_type_refd_last()->get_typetype() == 
            Type::T_OPENTYPE) {
          // allow the alternatives of open types as both lower and upper identifiers
          ref->set_field_name_to_lowercase();
        }
        t=t->get_refd_field_template(*ref->get_id(), *ref, usedInIsbound,
            refch, silent);
      }
      else t=t->get_refd_array_template(ref->get_val(), usedInIsbound, refch,
        silent);
    }
    return t;
  }
  
  bool Template::concat_to_bin_pattern(string& patt_str, Type::typetype_t exp_tt) const
  {
    switch (templatetype) {
    case SPECIFIC_VALUE:
      if (!u.specific_value->is_unfoldable()) {
        patt_str += u.specific_value->get_val_str();
        return true;
      }
      break;
    case BSTR_PATTERN:
      if (exp_tt == Type::T_BSTR) {
        patt_str += *u.pattern;
        return true;
      }
      break;
    case HSTR_PATTERN:
      if (exp_tt == Type::T_HSTR) {
        patt_str += *u.pattern;
        return true;
      }
      break;
    case OSTR_PATTERN:
      if (exp_tt == Type::T_OSTR) {
        patt_str += *u.pattern;
        return true;
      }
      break;
    case ANY_VALUE:
    case ANY_OR_OMIT:
      if (length_restriction != NULL) {
        int len;
        if (length_restriction->get_is_range()) {
          Value* lower = length_restriction->get_lower_value();
          Value* upper = length_restriction->get_upper_value();
          if (lower->is_unfoldable() || upper == NULL || upper->is_unfoldable() ||
              lower->get_val_Int()->get_val() != upper->get_val_Int()->get_val()) {
            break;
          }
          len = lower->get_val_Int()->get_val();
        }
        else {
          Value* single = length_restriction->get_single_value();
          if (single->is_unfoldable()) {
            break;
          }
          len = single->get_val_Int()->get_val();
        }
        for (int i = 0; i < len; ++i) {
          patt_str += '?';
        }
        return true;
      }
      else if (templatetype == ANY_VALUE) {
        if (patt_str.empty() || patt_str[patt_str.size() - 1] != '*') {
          patt_str += '*';
        }
        return true;
      }
      break;
    default:
      break;
    }
    return false;
  }
  
  Value* Template::get_string_encoding() const
  {
    if (templatetype != DECODE_MATCH) {
      FATAL_ERROR("Template::get_string_encoding()");
    }
    return u.dec_match.str_enc;
  }
  
  TemplateInstance* Template::get_decode_target() const
  {
    if (templatetype != DECODE_MATCH) {
      FATAL_ERROR("Template::get_decode_target()");
    }
    return u.dec_match.target;
  }
  
  Template* Template::get_concat_operand(bool first) const
  {
    if (!use_runtime_2 || templatetype != TEMPLATE_CONCAT) {
      FATAL_ERROR("Template::get_concat_operand");
    }
    return first ? u.concat.op1 : u.concat.op2;
  }

  Template* Template::get_template_refd(ReferenceChain *refch)
  {
    unsigned int const prev_err_count = get_error_count();
    if (templatetype != TEMPLATE_REFD)
      FATAL_ERROR("Template::get_template_refd()");
    // use the cached pointer if it is already set
    if (u.ref.refd) return u.ref.refd;
    Common::Assignment *ass = u.ref.ref->get_refd_assignment();
    if (!ass) FATAL_ERROR("Template::get_template_refd()");
    if(ass->get_asstype() == Common::Assignment::A_TEMPLATE) {
      FieldOrArrayRefs *subrefs = u.ref.ref->get_subrefs();
      Template *asst = ass->get_Template();
      Template *t   = asst->get_refd_sub_template(
        subrefs, u.ref.ref->getUsedInIsbound(), refch);
      if (t) {
        u.ref.refd = t;
        // Why do we not set u.ref.refd_last ?
      }
      else if (subrefs && subrefs->has_unfoldable_index()) {
        // some array indices could not be evaluated
        u.ref.refd = this;
        u.ref.refd_last = this;
      } else if (u.ref.ref->getUsedInIsbound()) {
        u.ref.refd = this;
        u.ref.refd_last = this;
      } else {
        // an error was found while resolving sub-references
        if (get_error_count() == prev_err_count) {
          // it was not reported, report it now
          error("Using a template which refers to a non-template is not supported");
          asst->note("Workaround: change the right hand side refer to a template");
          if (ass->is_local()) {
            ass->note("Workaround: change the template definition "
              "to a var template");
          }
        }
        set_templatetype(TEMPLATE_ERROR);
        return this;
      }
    } else {
      // the reference is unfoldable
      u.ref.refd = this;
    }
    return u.ref.refd;
  }

  Template* Template::get_refd_field_template(const Identifier& field_id,
    const Location& loc, bool usedInIsbound, ReferenceChain *refch, bool silent)
  {
    switch (templatetype) {
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
      // the above template types are valid matching mechanisms,
      // but they cannot be sub-referenced
      if (!silent) {
        loc.error("Reference to field `%s' of %s `%s'",
          field_id.get_dispname().c_str(), get_templatetype_str(),
          get_fullname().c_str());
      }
      break;
    default:
      break;
    }
    if(!my_governor) FATAL_ERROR("Template::get_refd_field_template()");
    Type *t = my_governor->get_type_refd_last();
    const char *typetype_str="set";
    switch(t->get_typetype()) {
    case Type::T_ERROR:
      // remain silent
      return 0;
    case Type::T_CHOICE_A:
    case Type::T_CHOICE_T:
    case Type::T_OPENTYPE:
    case Type::T_ANYTYPE:
      if (!t->has_comp_withName(field_id)) {
        if (!silent) {
          loc.error("Reference to non-existent union field `%s' in type `%s'",
            field_id.get_dispname().c_str(), t->get_typename().c_str());
        }
        return 0;
      } else if (templatetype != NAMED_TEMPLATE_LIST) {
        // this is an invalid matching mechanism, the error is already reported
//error("invalid matching mechanism (not template list) but %d", templatetype);
        return 0;
      } else if (u.named_templates->get_nof_nts() != 1) {
        // this is an invalid union template (more than one active field)
        // the error is already reported
//error("invalid union template ");
        return 0;
      } else {
        NamedTemplate *nt = u.named_templates->get_nt_byIndex(0);
        if (nt->get_name() != field_id) {
          if (!usedInIsbound && !silent) {
            loc.error("Reference to inactive field `%s' in a template of"
                    " union type `%s'. The active field is `%s'.",
                    field_id.get_dispname().c_str(),
                    t->get_typename().c_str(),
                    nt->get_name().get_dispname().c_str());
          }
          return 0;
        } else {
          // everything is OK
          return nt->get_template();
        }
      }
    case Type::T_SEQ_A:
    case Type::T_SEQ_T:
      typetype_str="record";
      // no break
    case Type::T_SET_A:
    case Type::T_SET_T:
      if (!t->has_comp_withName(field_id)) {
        if (!silent) {
          loc.error("Reference to non-existent %s field `%s' in type `%s'",
            typetype_str, field_id.get_dispname().c_str(),
            t->get_typename().c_str());
        }
        return 0;
      } else if (templatetype != NAMED_TEMPLATE_LIST) {
        // this is an invalid matching mechanism
        // the error should be already reported
        return 0;
      } else if (u.named_templates->has_nt_withName(field_id)) {
        // the field is found, everything is OK
        return u.named_templates->get_nt_byName(field_id)->get_template();
      } else if (base_template) {
        // take the field from the base template (recursively)
        return base_template->get_template_refd_last(refch)
          ->get_refd_field_template(field_id, loc, usedInIsbound, refch, silent);
      } else {
        if (!usedInIsbound && !silent) {
          // this should not happen unless there is an error
          // (e.g. missing field)
          loc.error("Reference to an unbound field `%s'",
            field_id.get_dispname().c_str());
        }
        return 0;
      }
    default:
      if (!silent) {
        loc.error("Invalid field reference `%s': type `%s' "
          "does not have fields", field_id.get_dispname().c_str(),
          t->get_typename().c_str());
      }
      return 0;
    }
  }

  Template* Template::get_refd_array_template(Value *array_index,
                                              bool usedInIsbound,
                                              ReferenceChain *refch,
                                              bool silent)
  {
    switch (templatetype) {
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
      // the above template types are valid matching mechanisms,
      // but they cannot be sub-referenced
      if (!silent) {
        array_index->error("Reference with index to an element of %s `%s'",
          get_templatetype_str(), get_fullname().c_str());
      }
      break;
    default:
      break;
    }
    Value *v_index = array_index->get_value_refd_last(refch);
    Int index = 0;
    bool index_available = false;
    if (!v_index->is_unfoldable()) {
      if (v_index->get_valuetype() == Value::V_INT) {
        index = v_index->get_val_Int()->get_val();
        index_available = true;
      } else if (!silent) {
        array_index->error("An integer value was expected as index");
      }
    }
    if (!my_governor) FATAL_ERROR("Template::get_refd_array_template()");
    Type *t = my_governor->get_type_refd_last();
    const char *typetype_str="set";
    switch(t->get_typetype()) {
    case Type::T_ERROR:
      // remain silent
      return 0;
    case Type::T_SEQOF:
      typetype_str="record";
      // no break
    case Type::T_SETOF:
      if (index_available) {
        if(index < 0) {
          if (!silent) {
            array_index->error("A non-negative integer value was expected "
              "instead of %s for indexing a template of `%s of' type `%s'",
              Int2string(index).c_str(), typetype_str,
              t->get_typename().c_str());
          }
          return 0;
        } else if (templatetype != TEMPLATE_LIST) {
          // remain silent the error has been already reported
          return 0;
        } else {
          size_t nof_elements = get_nof_listitems();
          if (index >= static_cast<Int>(nof_elements)) {
            if (!silent) {
              array_index->error("Index overflow in a template of `%s of' type "
                "`%s': the index is %s, but the template has only %lu elements",
                typetype_str, t->get_typename().c_str(),
                Int2string(index).c_str(), (unsigned long) nof_elements);
            }
            return 0;
          }
        }
      } else {
        // the index is not available or the error has been reported above
        return 0;
      }
      break;
    case Type::T_ARRAY:
      if (index_available) {
        ArrayDimension *dim = t->get_dimension();
        dim->chk_index(v_index, Type::EXPECTED_DYNAMIC_VALUE);
        if (templatetype == TEMPLATE_LIST && !dim->get_has_error()) {
          // perform the index transformation
          index -= dim->get_offset();
          // check for index underflow/overflow or too few elements in template
          if (index < 0 || index >= static_cast<Int>(get_nof_listitems()))
            return 0;
        } else {
          // remain silent, the error has been already reported
          return 0;
        }
      } else {
        // the index is not available or the error has been reported above
        return 0;
      }
      break;
    default:
      if (!silent) {
        array_index->error("Invalid array element reference: type `%s' cannot "
                           "be indexed", t->get_typename().c_str());
      }
      return 0;
    }
    Template *ret_val = get_listitem_byIndex((size_t)index);
    if (ret_val->templatetype == TEMPLATE_NOTUSED) {
      if (base_template) {
        // take the referred element from the base template
        return base_template->get_template_refd_last(refch)
          ->get_refd_array_template(v_index, usedInIsbound, refch, silent);
      } else {
        if(!silent && ret_val->get_templatetype() == TEMPLATE_NOTUSED)
                error("Not used symbol is not allowed in this context");
        return 0;
      }
    } else return ret_val;
  }

  bool Template::temps_contains_anyornone_symbol() const
  {
    switch (templatetype) {
    case TEMPLATE_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      break;
    default:
      FATAL_ERROR("Template::temps_contains_anyornone_symbol()");
    }
    size_t nof_comps = u.templates->get_nof_ts();
    for (size_t i = 0; i < nof_comps; i++) {
      Template *t = u.templates->get_t_byIndex(i);
      switch (t->templatetype) {
      case ANY_OR_OMIT:
      case ALL_FROM:
        // 'all from' clauses not known at compile time are also considered
        // as 'AnyOrNone'
        return true;
      case PERMUTATION_MATCH:
        // walk recursively
        if (t->temps_contains_anyornone_symbol()) return true;
        break;
      default:
        break;
      }
    }
    return false;
  }

  size_t Template::get_nof_comps_not_anyornone() const
  {
    switch (templatetype) {
    case TEMPLATE_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      break;
    default:
      FATAL_ERROR("Template::get_nof_comps_not_anyornone()");
    }
    size_t ret_val = 0;
    size_t nof_comps = u.templates->get_nof_ts();
    for (size_t i = 0; i < nof_comps; i++) {
      Template *t = u.templates->get_t_byIndex(i);
      switch (t->templatetype) {
      case ANY_OR_OMIT:
      case ALL_FROM:
        // do not count it
        break;
      case PERMUTATION_MATCH:
        // walk recursively
        ret_val += t->get_nof_comps_not_anyornone();
        break;
     default:
        // other types are counted as 1
        ret_val++;
        break;
      }
    }
    return ret_val;
  }

  bool Template::pattern_contains_anyornone_symbol() const
  {
    switch (templatetype) {
    case BSTR_PATTERN:
    case HSTR_PATTERN:
    case OSTR_PATTERN:
      return u.pattern->find('*') < u.pattern->size();
    case CSTR_PATTERN:
    case USTR_PATTERN:
      return true;
    default:
      FATAL_ERROR("Template::pattern_contains_anyornone_symbol()");
      return false;
    }
  }

  size_t Template::get_min_length_of_pattern() const
  {
    size_t ret_val = 0;
    switch (templatetype) {
    case BSTR_PATTERN:
    case HSTR_PATTERN: {
      size_t pattern_len = u.pattern->size();
      const char *pattern_ptr = u.pattern->c_str();
      for (size_t i = 0; i < pattern_len; i++)
        if (pattern_ptr[i] != '*') ret_val++;
      break; }
    case OSTR_PATTERN: {
      size_t pattern_len = u.pattern->size();
      const char *pattern_ptr = u.pattern->c_str();
      for (size_t i = 0; i < pattern_len; i++) {
        switch (pattern_ptr[i]) {
        case '*':
          // do not count
          break;
        case '?':
          // count as 1
          ret_val++;
          break;
        default:
          // count as 1 and skip over the next hex digit
          ret_val++;
          i++;
        }
      }
      break; }
    case CSTR_PATTERN:
    case USTR_PATTERN:
      break;
    default:
      FATAL_ERROR("Template::get_min_length_of_pattern()");
    }
    return ret_val;
  }

  bool Template::is_Value() const
  {
    if (length_restriction || is_ifpresent) return false;
    switch (templatetype) {
    case TEMPLATE_ERROR:
    case TEMPLATE_NOTUSED:
    case OMIT_VALUE:
      return true;
    case SPECIFIC_VALUE:
      if(u.specific_value->get_valuetype() == Value::V_INVOKE) {
        Type *t = u.specific_value
          ->get_invoked_type(Type::EXPECTED_DYNAMIC_VALUE);
        if(t && t->get_type_refd_last()->get_typetype() == Type::T_FUNCTION &&
          t->get_type_refd_last()->get_returns_template()) return false;
      }
      return true;
    case TEMPLATE_LIST: {
      Templates *ts = u.templates;
      for (size_t i = 0; i < ts->get_nof_ts(); i++)
        if (!ts->get_t_byIndex(i)->is_Value()) return false;
      return true;
    }
    case NAMED_TEMPLATE_LIST: {
      NamedTemplates *ts = u.named_templates;
      for (size_t i = 0;i < ts->get_nof_nts(); i++)
        if (!ts->get_nt_byIndex(i)->get_template()->is_Value()) return false;
      return true;
    }
    case INDEXED_TEMPLATE_LIST: {
      IndexedTemplates *ts = u.indexed_templates;
      for (size_t i = 0; i < ts->get_nof_its(); i++)
        if (!ts->get_it_byIndex(i)->get_template()->is_Value()) return false;
      return true;
    }
    case TEMPLATE_REFD: {
      Common::Assignment *ass = u.ref.ref->get_refd_assignment();
      switch (ass->get_asstype()) {
      case Common::Assignment::A_EXT_CONST:
      case Common::Assignment::A_PAR_VAL:
      case Common::Assignment::A_PAR_VAL_IN:
      case Common::Assignment::A_PAR_VAL_OUT:
      case Common::Assignment::A_PAR_VAL_INOUT:
      case Common::Assignment::A_VAR:
        return true;
      default:
        return false;
      }
    }
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::is_Value()");
      }
      return u.concat.op1->is_Value() && u.concat.op2->is_Value();
    default:
      return false;
    }
  }

  Value *Template::get_Value()
  {
    Value *ret_val;
    switch(templatetype) {
    case TEMPLATE_ERROR:
      ret_val = new Value(Value::V_ERROR);
      break;
    case TEMPLATE_NOTUSED:
      ret_val = new Value(Value::V_NOTUSED);
      break;
    case OMIT_VALUE:
      ret_val = new Value(Value::V_OMIT);
      break;
    case SPECIFIC_VALUE:
      ret_val = u.specific_value;
      u.specific_value = 0;
      set_templatetype(TEMPLATE_ERROR);
      return ret_val;
    case TEMPLATE_LIST: {
      Values *vs = new Values;
      size_t nof_ts = u.templates->get_nof_ts();
      Type* gov = 0;
      for (size_t i = 0; i < nof_ts; i++) {
        Value* v = u.templates->get_t_byIndex(i)->get_Value();
        if (!gov) gov = v->get_my_governor();
        vs->add_v(v);
      }
      ret_val = new Value(Value::V_SEQOF, vs);
      if (gov) gov = gov->get_parent_type();
      if (gov) ret_val->set_my_governor(gov);
      break; }
    case NAMED_TEMPLATE_LIST: {
      NamedValues *nvs = new NamedValues;
      size_t nof_nts = u.named_templates->get_nof_nts();
      Type* gov = 0;
      for (size_t i = 0; i < nof_nts; i++) {
        NamedTemplate *nt = u.named_templates->get_nt_byIndex(i);
        Value* v = nt->get_template()->get_Value();
        if (!gov) gov = v->get_my_governor();
        NamedValue *nv = new NamedValue(nt->get_name().clone(), v);
        nv->set_location(*nt);
        nvs->add_nv(nv);
      }
      ret_val = new Value(Value::V_SEQ, nvs);
      if (gov) gov = gov->get_parent_type();
      if (gov) ret_val->set_my_governor(gov);
      break; }
    case INDEXED_TEMPLATE_LIST: {
      Values *ivs = new Values(true);
      size_t nof_its = u.indexed_templates->get_nof_its();
      Type* gov = 0;
      for (size_t i = 0; i < nof_its; i++) {
        IndexedTemplate *it = u.indexed_templates->get_it_byIndex(i);
        Value* v = it->get_template()->get_Value();
        if (!gov) gov = v->get_my_governor();
        IndexedValue *iv = new IndexedValue(it->get_index().clone(), v);
        iv->set_location(*it);
        ivs->add_iv(iv);
      }
      ret_val = new Value(Value::V_SEQOF, ivs);
      if (gov) gov = gov->get_parent_type();
      if (gov) ret_val->set_my_governor(gov);
      break; }
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::get_Value()");
      }
      ret_val = new Value(Value::OPTYPE_CONCAT, u.concat.op1->get_Value(),
        u.concat.op2->get_Value());
      break;
    default:
      FATAL_ERROR("Template::get_Value()");
      ret_val = 0;
      break;
    }
    ret_val->set_location(*this);
    ret_val->set_my_scope(get_my_scope());
    ret_val->set_fullname(get_fullname());
    return ret_val;
  }

  bool Template::is_Ref() const
  {
    if (length_restriction || is_ifpresent || templatetype != SPECIFIC_VALUE)
      return false;
    Value *v = u.specific_value;
    switch (v->get_valuetype()) {
    case Value::V_UNDEF_LOWERID:
      return true;
    case Value::V_REFD:
      if (dynamic_cast<Ref_base*>(v->get_reference())) return true;
      else return false;
    default:
      return false;
    }
  }

  Ref_base *Template::get_Ref()
  {
    if (templatetype != SPECIFIC_VALUE)
      FATAL_ERROR("Template::get_Ref()");
    return u.specific_value->steal_ttcn_ref_base();
  }

  void Template::chk_recursions(ReferenceChain& refch)
  {
    if (recurs_checked) return;
    Template *t = this;
    for ( ; ; ) {
      if (t->templatetype == SPECIFIC_VALUE) break;
      else if (!refch.add(t->get_fullname())) goto end;
      else if (t->templatetype != TEMPLATE_REFD) break;
      t->u.ref.ref->get_refd_assignment(true); // make sure the parameter list is checked
      ActualParList *parlist = t->u.ref.ref->get_parlist();
      if (parlist) parlist->chk_recursions(refch);
      Template *t_refd = t->get_template_refd(&refch);
      if (t_refd == t) break;
      else t = t_refd;
    }
    t->set_lowerid_to_ref();
    switch (t->templatetype) {
    case SPECIFIC_VALUE:
          t->u.specific_value->chk_recursions(refch);
      break;
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      for (size_t i = 0; i < t->u.templates->get_nof_ts(); i++) {
        refch.mark_state();
        t->u.templates->get_t_byIndex(i)->chk_recursions(refch);
        refch.prev_state();
      }
      break;
    case NAMED_TEMPLATE_LIST:
      for (size_t i = 0; i < t->u.named_templates->get_nof_nts(); i++) {
        refch.mark_state();
        t->u.named_templates->get_nt_byIndex(i)
          ->get_template()->chk_recursions(refch);
        refch.prev_state();
      }
      break;
    case INDEXED_TEMPLATE_LIST:
      for (size_t i = 0; i < t->u.indexed_templates->get_nof_its(); i++) {
        refch.mark_state();
        t->u.indexed_templates->get_it_byIndex(i)
          ->get_template()->chk_recursions(refch);
        refch.prev_state();
      }
      break;
    case CSTR_PATTERN:
    case USTR_PATTERN:
      t->u.pstring->chk_recursions(refch);
      break;
    case DECODE_MATCH:
      t->u.dec_match.target->chk_recursions(refch);
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::chk_recursions()");
      }
      refch.mark_state();
      t->u.concat.op1->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      t->u.concat.op2->chk_recursions(refch);
      refch.prev_state();
      break;
    default:
      break;
    }
end:
    recurs_checked = true;
  }

  void Template::chk_specific_value(bool allow_omit)
  {
    Template *t = get_template_refd_last();
    if (!allow_omit && t->templatetype==OMIT_VALUE) {
      t->error("A specific value was expected instead of omit");
    }
    chk_specific_value_generic();
  }

  void Template::chk_specific_value_generic()
  {
    if (specific_value_checked) return;
    Template *t = get_template_refd_last();
    if (t->specific_value_checked) return;
    switch (t->templatetype) {
    case TEMPLATE_ERROR:
    case TEMPLATE_NOTUSED:
    case TEMPLATE_REFD: // unfoldable reference
      break;
    case SPECIFIC_VALUE:
      if(u.specific_value->get_valuetype() == Value::V_INVOKE) {
        Type *t_type = u.specific_value
          ->get_invoked_type(Type::EXPECTED_DYNAMIC_VALUE);
        if(t_type && t_type->get_type_refd_last()->get_returns_template()) {
          set_templatetype(TEMPLATE_INVOKE);
          chk_invoke();
        }
      }
      break;
    case TEMPLATE_INVOKE:
      chk_invoke();
      break;
    case TEMPLATE_LIST:
      for (size_t i = 0; i < t->u.templates->get_nof_ts(); i++)
        t->u.templates->get_t_byIndex(i)->chk_specific_value_generic();
      break;
    case NAMED_TEMPLATE_LIST:
      for (size_t i = 0; i < t->u.named_templates->get_nof_nts(); i++)
        t->u.named_templates->get_nt_byIndex(i)
          ->get_template()->chk_specific_value_generic();
      break;
    case INDEXED_TEMPLATE_LIST:
      for (size_t i = 0; i < t->u.indexed_templates->get_nof_its(); i++)
        t->u.indexed_templates->get_it_byIndex(i)
          ->get_template()->chk_specific_value_generic();
      break;
    case OMIT_VALUE:
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::chk_specific_value_generic()");
      }
      t->u.concat.op1->chk_specific_value_generic();
      t->u.concat.op2->chk_specific_value_generic();
      break;
    default:
      t->error("A specific value was expected instead of %s",
        t->get_templatetype_str());
      break;
    }
    t->specific_value_checked = true;
    specific_value_checked = true;
  }

  void Template::chk_invoke()
  {
    if(templatetype != TEMPLATE_INVOKE) FATAL_ERROR("Template::chk_invoke()");
    if(!u.invoke.t_list) return; //already checked
    Error_Context cntxt(this, "In `apply()' operation");
    Type *t = u.invoke.v->get_expr_governor_last();
    if (t) {
      if (t->get_typetype() != Type::T_FUNCTION) {
        u.invoke.v->error("A value of type function was expected in the "
          "argument instead of `%s'", t->get_typename().c_str());
        set_templatetype(TEMPLATE_ERROR);
        return;
      }
    } else {
      if (u.invoke.v->get_valuetype() != Value::V_ERROR)
        u.invoke.v->error("A value of type function was expected in the "
          "argument");
      set_templatetype(TEMPLATE_ERROR);
      return;
    }
    my_scope->chk_runs_on_clause(t, *this, "call");
    Ttcn::FormalParList *fp_list = t->get_fat_parameters();
    Ttcn::ActualParList *parlist = new Ttcn::ActualParList;
    bool is_erroneous = fp_list->fold_named_and_chk(u.invoke.t_list,parlist);
    delete u.invoke.t_list;
    u.invoke.t_list = 0;
    if(is_erroneous) {
      delete parlist;
      u.invoke.ap_list = 0;
    } else {
      parlist->set_fullname(get_fullname());
      parlist->set_my_scope(get_my_scope());
      u.invoke.ap_list = parlist;
    }
  }

  Templates *Template::harbinger(Template *t, bool from_permutation, bool killer)
  {
    Templates *new_templates = new Templates;
    switch (t->u.all_from->templatetype) {
    case SPECIFIC_VALUE: {
      Value *innerv = t->u.all_from->get_specific_value();
      //if (v->get_valuetype() == Value::V_UNDEF_LOWERID)
      innerv->set_lowerid_to_ref();
      // should be a ref now
      bool can_flatten = true;
      Common::Reference  *ref = innerv->get_reference();
      if (dynamic_cast<Ttcn::Ref_pard*>(ref)) {
        // Cannot flatten at compile time if the template has parameters.
        can_flatten = false;
      }

      // check for subreferences in the 'all from' target
      FieldOrArrayRefs* subrefs = ref->get_subrefs();
      if (NULL != subrefs) {
        for (size_t i = 0; i < subrefs->get_nof_refs(); ++i) {
          FieldOrArrayRef* subref = subrefs->get_ref(i);
          if (FieldOrArrayRef::ARRAY_REF == subref->get_type()) {
            // set any array indexes from undefined lowerID to reference
            subref->get_val()->set_lowerid_to_ref();
          }
        }
      }

      Common::Assignment *ass = ref->get_refd_assignment();
      if (ass == NULL) { // perhaps erroneous
        break;
      }

      Common::Assignment::asstype_t asst = ass->get_asstype();
      switch (asst) {
      case Common::Assignment::A_TEMPLATE: {
        Template *tpl = ass->get_Template();
        // tpl is the template whose name appears after the "all from"
        Common::Type *type = ass->get_Type()->get_type_refd_last();
        if (NULL != subrefs) {
          // walk through the subreferences to determine the type and value of the 'all from' target
          // Note: the templates referenced by the array indexes and field names
          // have not been checked yet
          for (size_t i = 0; i < subrefs->get_nof_refs(); ++i) {
            FieldOrArrayRef* subref = subrefs->get_ref(i);
            if (FieldOrArrayRef::ARRAY_REF == subref->get_type()) {
              // check if the type can be indexed
              Common::Type::typetype_t tt = type->get_typetype();
              if (Common::Type::T_SEQOF != tt && Common::Type::T_SETOF != tt &&
                  Common::Type::T_ARRAY != tt) {
                subref->error("Cannot apply an array index to type '%s'",
                  type->get_typename().c_str());
                i = subrefs->get_nof_refs(); // quit from the cycle, too
                tpl = NULL;
                break;
              }
              if (can_flatten && !subref->get_val()->is_unfoldable()) {
                switch(tpl->get_templatetype()) {
                case TEMPLATE_LIST:
                case VALUE_LIST: {
                  Int index = subref->get_val()->get_val_Int()->get_val();
                  // check for index overflow
                  if (index >= static_cast<Int>(tpl->get_nof_comps())) {
                    subref->error("Index overflow in a template %s type `%s':"
                      " the index is %s, but the template has only %lu elements",
                      Common::Type::T_ARRAY == tt ? "array of" : 
                      (Common::Type::T_SEQOF == tt ? "of 'record of'" : "of 'set of'"),
                      type->get_typename().c_str(), Int2string(index).c_str(),
                      (unsigned long)tpl->get_nof_comps());
                    i = subrefs->get_nof_refs(); // quit from the cycle, too
                    tpl = NULL;
                    break;
                  }
                  tpl = tpl->get_temp_byIndex((size_t)index);
                  // check if the element is initialized
                  if (TEMPLATE_NOTUSED == tpl->get_templatetype()) {
                    subref->error("An uninitialized list element can not be used as target of 'all from'");
                    i = subrefs->get_nof_refs(); // quit from the cycle, too
                    tpl = NULL;
                    break;
                  }
                  break; }
                case INDEXED_TEMPLATE_LIST:
                  can_flatten = false; // currently not supported
                  break;
                default:
                  subref->error("Expected a specific value of type '%s' instead of %s",
                    type->get_typename().c_str(), tpl->get_templatetype_str());
                  i = subrefs->get_nof_refs(); // quit from the cycle, too
                  tpl = NULL;
                  break;
                }
              }
              else {
                // one of the array indexes is a reference => cannot flatten
                can_flatten = false;
              }
              type = type->get_ofType()->get_type_refd_last();
            }
            else { // FIELD_REF
              // check if the type can have fields
              Common::Type::typetype_t tt = type->get_typetype();
              if (Common::Type::T_SEQ_T != tt && Common::Type::T_SEQ_A != tt &&
                  Common::Type::T_SET_T != tt && Common::Type::T_SET_A != tt &&
                  Common::Type::T_CHOICE_T != tt && Common::Type::T_CHOICE_A != tt) {
                subref->error("Cannot apply a field name to type '%s'",
                  type->get_typename().c_str());
                tpl = NULL;
                break;
              }
              // check if the field name is valid
              if (!type->has_comp_withName(*subref->get_id())) {
                subref->error("Type '%s' does not have a field with name '%s'",
                  type->get_typename().c_str(), subref->get_id()->get_dispname().c_str());
                tpl = NULL;
                break;
              }
              if (can_flatten) {
                switch(tpl->get_templatetype()) {
                case NAMED_TEMPLATE_LIST: {
                  // check if there is any data in the template for this field
                  // (no data means it's uninitialized)
                  if (!tpl->u.named_templates->has_nt_withName(*subref->get_id())) {
                    subref->error("An uninitialized field can not be used as target of 'all from'");
                    i = subrefs->get_nof_refs(); // quit from the cycle, too
                    tpl = NULL;
                    break;
                  }
                  tpl = tpl->u.named_templates->get_nt_byName(*subref->get_id())->get_template();
                  // check if the field is initialized and present (not omitted)
                  if (OMIT_VALUE == tpl->get_templatetype() || TEMPLATE_NOTUSED == tpl->get_templatetype()) {
                    subref->error("An %s field can not be used as target of 'all from'",
                      OMIT_VALUE == tpl->get_templatetype() ? "omitted" : "uninitialized");
                    i = subrefs->get_nof_refs(); // quit from the cycle, too
                    tpl = NULL;
                    break;
                  }
                  break; }
                default:
                  subref->error("Expected a specific value of type '%s' instead of %s",
                    type->get_typename().c_str(), tpl->get_templatetype_str());
                  i = subrefs->get_nof_refs(); // quit from the cycle, too
                  tpl = NULL;
                  break;
                }
              }
              type = type->get_comp_byName(*subref->get_id())->get_type()->get_type_refd_last();
            }
          }
        }
        if (NULL != tpl) { // tpl is set to null if an error occurs
          if (can_flatten) {
            Template::templatetype_t tpltt = tpl->get_templatetype();
            switch (tpltt) {
            case INDEXED_TEMPLATE_LIST: // currently not supported
            case TEMPLATE_REFD: {
              delete new_templates;
              new_templates = 0;
              killer = false;
            break; }

            case TEMPLATE_LIST:
          // ALL_FROM ?
            case VALUE_LIST: {
              size_t nvl = tpl->get_nof_comps();
              for (size_t ti = 0; ti < nvl; ++ti) {
              Template *orig = tpl->get_temp_byIndex(ti);
                switch (orig->templatetype) {
                case SPECIFIC_VALUE: {
                  Value *val = orig->get_specific_value();
                  if (val->get_valuetype() == Value::V_REFD) {
                    if (dynamic_cast<Ttcn::Ref_pard*>(val->get_reference())) {
                      // Cannot flatten at compile time if one of the values or templates has parameters.
                      can_flatten = false;
                    }
                  }
                  break; }
                case ANY_OR_OMIT:
                  if (from_permutation) {
                    break; // AnyElementOrNone allowed in "all from" now
                  }
                  // no break
                case PERMUTATION_MATCH:
                  t->error("'all from' can not refer to permutation or AnyElementsOrNone");
                  t->set_templatetype(TEMPLATE_ERROR);
                default:
                  break;
                }
              }
              if (can_flatten) {
                for (size_t ti = 0; ti < nvl; ++ti) {
                Template *orig = tpl->get_temp_byIndex(ti);
                  Template *copy = orig->clone();
                  copy->set_my_scope(orig->get_my_scope());
                  new_templates->add_t(copy);
                }
              }
              else {
                // Cannot flatten at compile time
                delete new_templates;
                new_templates = 0;
                killer = false;
              }
              break; }

            case NAMED_TEMPLATE_LIST: {
              size_t nvl = tpl->get_nof_comps();
              for (size_t ti = 0; ti < nvl; ++ti) {
                NamedTemplate *orig = tpl->get_namedtemp_byIndex(ti);
                switch (orig->get_template()->get_templatetype()) {
                  case ANY_OR_OMIT:
                    break;
                  case PERMUTATION_MATCH:
                    t->error("'all from' can not refer to permutation or AnyElementsOrNone");
                    t->set_templatetype(TEMPLATE_ERROR);
                  default:
                    break;
                }
              }
              delete new_templates;
              new_templates = 0;
              killer = false;
              break; }

            case ANY_VALUE:
            case ANY_OR_OMIT:
              tpl->error("Matching mechanism can not be used as target of 'all from'");
              break;
            default:
              tpl->error("A template of type '%s' can not be used as target of 'all from'",
                type->get_typename().c_str());
              break;
            }
          }
          else { // cannot flatten
            switch (type->get_typetype()) {
            case Common::Type::T_SEQOF: case Common::Type::T_SETOF:
            case Common::Type::T_ARRAY:
              delete new_templates;
              new_templates = 0;
              killer = false;
              break;
            default:
              type->error("A template of type `%s' can not be used as target of 'all from'",
                type->get_typename().c_str());
              break;
            } // switch(typetype)
          }
        }

        if (killer) delete t;
        break; }


      case Common::Assignment::A_CONST: { // all from a constant definition
        Common::Value *val = ass->get_Value();
        Common::Type *type = ass->get_Type()->get_type_refd_last();
        if (NULL != subrefs) {
          // walk through the subreferences to determine the type and value of the 'all from' target
          for (size_t i = 0; i < subrefs->get_nof_refs(); ++i) {
            FieldOrArrayRef* subref = subrefs->get_ref(i);
            if (FieldOrArrayRef::ARRAY_REF == subref->get_type()) {
              // check if the type can be indexed
              Common::Type::typetype_t tt = type->get_typetype();
              if (Common::Type::T_SEQOF != tt && Common::Type::T_SETOF != tt &&
                  Common::Type::T_ARRAY != tt) {
                subref->error("Cannot apply an array index to type '%s'",
                  type->get_typename().c_str());
                val = NULL;
                break;
              }
              if (can_flatten && !subref->get_val()->is_unfoldable()) {
                Int index = subref->get_val()->get_val_Int()->get_val();
                // check for index overflow
                if (index >= static_cast<Int>(val->get_nof_comps())) {
                  subref->error("Index overflow in a value %s type `%s':"
                    " the index is %s, but the template has only %lu elements",
                    Common::Type::T_ARRAY == tt ? "array of" : 
                    (Common::Type::T_SEQOF == tt ? "of 'record of'" : "of 'set of'"),
                    type->get_typename().c_str(), Int2string(index).c_str(),
                    (unsigned long)val->get_nof_comps());
                  val = NULL;
                  break;
                }
                val = val->get_comp_byIndex((size_t)index);
                // check if the element is initialized
                if (Common::Value::V_NOTUSED == val->get_valuetype()) {
                  subref->error("An unbound list element can not be used as target of 'all from'");
                  val = NULL;
                  break;
                }
              }
              else {
                // one of the array indexes is a reference => cannot flatten
                can_flatten = false;
              }
              type = type->get_ofType()->get_type_refd_last();
            }
            else { // FIELD_REF
              // check if the type can have fields
              Common::Type::typetype_t tt = type->get_typetype();
              if (Common::Type::T_SEQ_T != tt && Common::Type::T_SEQ_A != tt &&
                  Common::Type::T_SET_T != tt && Common::Type::T_SET_A != tt &&
                  Common::Type::T_CHOICE_T != tt && Common::Type::T_CHOICE_A != tt) {
                subref->error("Cannot apply a field name to type '%s'",
                  type->get_typename().c_str());
                val = NULL;
                break;
              }
              // check if the field name is valid
              if (!type->has_comp_withName(*subref->get_id())) {
                subref->error("Type '%s' does not have a field with name '%s'",
                  type->get_typename().c_str(), subref->get_id()->get_dispname().c_str());
                val = NULL;
                break;
              }
              type = type->get_comp_byName(*subref->get_id())->get_type()->get_type_refd_last();
              if (can_flatten) {
                // check if the value has any data for this field (no data = unbound)
                if (!val->has_comp_withName(*subref->get_id())) {
                  subref->error("An unbound field can not be used as target of 'all from'");
                  val = NULL;
                  break;
                }
                val = val->get_comp_value_byName(*subref->get_id());
                // check if the field is bound and present (not omitted)
                if (Common::Value::V_OMIT == val->get_valuetype() ||
                    Common::Value::V_NOTUSED == val->get_valuetype()) {
                  subref->error("An %s field can not be used as target of 'all from'",
                    Common::Value::V_OMIT == val->get_valuetype() ? "omitted" : "unbound");
                  val = NULL;
                  break;
                }
              }
            }
          }
        }
        if (NULL != val) { // val is set to null if an error occurs
          switch (type->get_typetype()) {
          case Common::Type::T_SEQOF: case Common::Type::T_SETOF:
          case Common::Type::T_ARRAY: {
            if (can_flatten) {
              const size_t ncomp = val->get_nof_comps();
              for (size_t i = 0; i < ncomp; ++i) {
                Value *v = val->get_comp_byIndex(i);
                Template *newt = new Template(v->clone());
                new_templates->add_t(newt);
              }
            }
            else {
              delete new_templates;
              new_templates = 0;
              killer = false;
            }
            break; }

          default:
            type->error("A constant of type `%s' can not be used as target of 'all from'",
              type->get_typename().c_str());
            break;
          } // switch(typetype)
        }
        if (killer) delete t;
        break; }

      case Common::Assignment::A_MODULEPAR_TEMP:
      case Common::Assignment::A_VAR_TEMPLATE: 
      case Common::Assignment::A_FUNCTION_RTEMP: 
      case Common::Assignment::A_EXT_FUNCTION_RTEMP:
      case Common::Assignment::A_PAR_TEMPL_IN:
      case Common::Assignment::A_PAR_TEMPL_INOUT:
      case Common::Assignment::A_PAR_TEMPL_OUT:
//TODO: flatten if the actual par is const template
      case Common::Assignment::A_MODULEPAR: // all from a module parameter
      case Common::Assignment::A_VAR: // all from a variable
      case Common::Assignment::A_PAR_VAL_IN:
      case Common::Assignment::A_PAR_VAL_INOUT:
      case Common::Assignment::A_PAR_VAL_OUT:
      case Common::Assignment::A_FUNCTION_RVAL:
      case Common::Assignment::A_EXT_FUNCTION_RVAL: {
        Common::Type *type = ass->get_Type()->get_type_refd_last();
        if (NULL != subrefs) {
          // walk through the subreferences to determine the type of the 'all from' target
          for (size_t i = 0; i < subrefs->get_nof_refs(); ++i) {
            FieldOrArrayRef* subref = subrefs->get_ref(i);
            if (FieldOrArrayRef::ARRAY_REF == subref->get_type()) {
              // check if the type can be indexed
              Common::Type::typetype_t tt = type->get_typetype();
              if (Common::Type::T_SEQOF != tt && Common::Type::T_SETOF != tt &&
                  Common::Type::T_ARRAY != tt) {
                subref->error("Cannot apply an array index to type '%s'",
                  type->get_typename().c_str());
                type = NULL;
                break;
              }
              type = type->get_ofType()->get_type_refd_last();
            }
            else { // FIELD_REF
              // check if the type can have fields
              Common::Type::typetype_t tt = type->get_typetype();
              if (Common::Type::T_SEQ_T != tt && Common::Type::T_SEQ_A != tt &&
                  Common::Type::T_SET_T != tt && Common::Type::T_SET_A != tt &&
                  Common::Type::T_CHOICE_T != tt && Common::Type::T_CHOICE_A != tt) {
                subref->error("Cannot apply a field name to type '%s'",
                  type->get_typename().c_str());
                type = NULL;
                break;
              }
              // check if the field name is valid
              if (!type->has_comp_withName(*subref->get_id())) {
                subref->error("Type '%s' does not have a field with name '%s'",
                  type->get_typename().c_str(), subref->get_id()->get_dispname().c_str());
                type = NULL;
                break;
              }
              type = type->get_comp_byName(*subref->get_id())->get_type()->get_type_refd_last();
            }
          }
        }
        if (NULL != type) {
          switch (type->get_typetype()) {
          case Common::Type::T_SEQOF: case Common::Type::T_SETOF:
          case Common::Type::T_ARRAY:
            delete new_templates; // cannot flatten at compile time
            new_templates = 0;
            break;
          default: {
            // not an array type => error
            const char* ass_name = ass->get_assname();
            string descr;
            switch(asst) {
            case Common::Assignment::A_MODULEPAR_TEMP:
            case Common::Assignment::A_VAR_TEMPLATE:
            case Common::Assignment::A_MODULEPAR:
            case Common::Assignment::A_VAR:
            case Common::Assignment::A_PAR_TEMPL_IN:
            case Common::Assignment::A_PAR_VAL_IN:
              descr = "A ";
              descr += ass_name;
              break;
            case Common::Assignment::A_PAR_TEMPL_INOUT:
            case Common::Assignment::A_PAR_TEMPL_OUT:
            case Common::Assignment::A_PAR_VAL_INOUT:
            case Common::Assignment::A_PAR_VAL_OUT:
              descr = "An ";
              descr += ass_name;
              break;
            // the assignment name string for functions is no good here
            case Common::Assignment::A_FUNCTION_RTEMP:
              descr = "A function returning a template";
              break;
            case Common::Assignment::A_FUNCTION_RVAL:
              descr = "A function returning a value";
              break;
            case Common::Assignment::A_EXT_FUNCTION_RTEMP:
              descr = "An external function returning a template";
              break;
            case Common::Assignment::A_EXT_FUNCTION_RVAL:
              descr = "An external function returning a value";
              break;
            default:
              break;
            }
            type->error("%s of type `%s' can not be used as target of 'all from'",
              descr.c_str(), type->get_typename().c_str());
            break; }
          }
        } // switch(typetype)
        break; }

      default:
        FATAL_ERROR("harbinger asst %d", asst);
        break;
      }
      break; }

    case ALL_FROM:
      FATAL_ERROR("unexpected all from inside all from");
      break;
    default:
      FATAL_ERROR("tt %d", t->u.all_from->templatetype);
    }

    return new_templates;
  }

  bool Template::flatten(bool from_permutation)
  {
    switch (templatetype) {
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH: {
      size_t num_t = u.templates->get_nof_ts(); // one of these is the "all from"
      Templates *new_templates = new Templates;
      for (size_t i = 0; i < num_t; ++i) {
        Template *& t = u.templates->get_t_byIndex(i);
        // the element in the (,,,)
        switch (t->templatetype) {

        case ALL_FROM: { // subset(1, all from ..., 99)
          // some number of elements--^^^^^^^^^^^^
          if (t->checked) FATAL_ERROR("too late");
          Templates *af = harbinger(t, from_permutation, true);
          if (af) {
            for (size_t a = 0, num = af->get_nof_ts(); a < num; ++a) {
              Template *& t2 = af->get_t_byIndex(a);
              new_templates->add_t(t2);
              t2 = 0; // take it away from its current owner
            }
            delete af;
          }
          else {
            new_templates->add_t(t); // transfer it unchanged
            flattened = false;
          }
          break; } // case ALL_FROM

        case PERMUTATION_MATCH: {
          // permut inside a value list: { 1, permut(), 2 }
          flattened &= t->flatten(true);
          new_templates->add_t(t);
          break; }
        case TEMPLATE_LIST:
        case VALUE_LIST:
       // case COMPLEMENTED_LIST:
       // case SUPERSET_MATCH:
       // case SUBSET_MATCH: 
       {
          flattened &=t->flatten(false);
          new_templates->add_t(t);
          break; }
        default:
          new_templates->add_t(t);
        }

        t = 0; // take it away from the original
      }
      delete u.templates;
      u.templates = new_templates;
//printf("!!! flattened from %lu to %lu at line %d\n",
//  (unsigned long)num_t, (unsigned long)new_templates->get_nof_ts(),
//  get_first_line());

      break; }

    case TEMPLATE_ERROR: case TEMPLATE_NOTUSED:
    case OMIT_VALUE:     case ANY_VALUE: case ANY_OR_OMIT:
    case SPECIFIC_VALUE:
    case TEMPLATE_REFD:  case TEMPLATE_INVOKE:
    case NAMED_TEMPLATE_LIST: case INDEXED_TEMPLATE_LIST:
    case VALUE_RANGE:
    case ALL_FROM:
    case BSTR_PATTERN: case HSTR_PATTERN: case OSTR_PATTERN:
    case CSTR_PATTERN: case USTR_PATTERN: case DECODE_MATCH:
    case TEMPLATE_CONCAT:
      break; // NOP
    }

    set_fullname(get_fullname()); // recompute fullname (esp. for components)
    set_my_scope(get_my_scope());

    return flattened;
  }

  const char* Template::get_restriction_name(template_restriction_t tr)
  {
    switch (tr) {
    case TR_NONE:
      break;
    case TR_OMIT:
      return "omit";
    case TR_VALUE:
      return "value";
    case TR_PRESENT:
      return "present";
    default:
      FATAL_ERROR("Template::get_restriction_name()");
    }
    return "";
  }

  template_restriction_t Template::get_sub_restriction(
    template_restriction_t tr, Ref_base* ref)
  {
    if (!ref) FATAL_ERROR("Template::get_sub_restriction()");
    if (!ref->get_subrefs()) return tr;
    bool is_optional = true; // the referenced field is on an optional path
    Common::Assignment* ass = ref->get_refd_assignment();
    if (ass) {
      Type* t = ass->get_Type();
      if (t) {
        ReferenceChain refch(ref, "While checking template restriction");
        t = t->get_field_type(ref->get_subrefs(), Type::EXPECTED_TEMPLATE,
                              &refch, true);
        if (t) is_optional = false;
      }
    }
    switch (tr) {
    case TR_NONE:
      return TR_NONE;
    case TR_OMIT:
      return TR_OMIT;
    case TR_VALUE:
      return is_optional ? TR_OMIT : TR_VALUE;
    case TR_PRESENT:
      return is_optional ? TR_NONE : TR_PRESENT;
    default:
      FATAL_ERROR("Template::get_sub_restriction()");
    }
    return tr;
  }

  bool Template::is_less_restrictive(template_restriction_t needed_tr,
    template_restriction_t refd_tr)
  {
    switch (needed_tr) {
    case TR_NONE:
      return false;
    case TR_VALUE:
      return refd_tr!=TR_VALUE;
    case TR_OMIT:
      return refd_tr!=TR_VALUE && refd_tr!=TR_OMIT;
    case TR_PRESENT:
      return refd_tr!=TR_VALUE && refd_tr!=TR_PRESENT;
    default:
      FATAL_ERROR("Template::is_less_restrictive()");
    }
    return true;
  }

  char* Template::generate_restriction_check_code(char* str, const char* name,
    template_restriction_t tr)
  {
    const char* tr_name;
    switch (tr) {
    case TR_NONE:
      return str;
    case TR_OMIT:
      tr_name = "TR_OMIT";
      break;
    case TR_VALUE:
      tr_name = "TR_VALUE";
      break;
    case TR_PRESENT:
      tr_name = "TR_PRESENT";
      break;
    default:
      FATAL_ERROR("Template::generate_restriction_check_code()");
    }
    return mputprintf(str, "%s.check_restriction(%s%s);\n", name, tr_name,
      (omit_in_value_list ? ", NULL, TRUE" : ""));
  }

  // embedded templates -> check needed only for case of TR_OMIT
  bool Template::chk_restriction_named_list(const char* definition_name,
    map<string, void>& checked_map, size_t needed_checked_cnt,
    const Location* usage_loc)
  {
    bool needs_runtime_check = false;
    if (checked_map.size()>=needed_checked_cnt) return needs_runtime_check;
    switch (templatetype) {
    case NAMED_TEMPLATE_LIST:
      for (size_t i = 0;i < u.named_templates->get_nof_nts(); i++) {
        NamedTemplate *nt = u.named_templates->get_nt_byIndex(i);
        Template* tmpl = nt->get_template();
        const string& name = nt->get_name().get_name();
        if (!checked_map.has_key(name)) {
          bool nrc = tmpl->chk_restriction(definition_name, TR_OMIT, usage_loc);
          needs_runtime_check = needs_runtime_check || nrc;
          checked_map.add(name, 0);
        }
      }
      if (base_template) {
        bool nrc = base_template->chk_restriction_named_list(definition_name,
                     checked_map, needed_checked_cnt, usage_loc);
        needs_runtime_check = needs_runtime_check || nrc;
      }
      break;
    case ANY_VALUE:
    case ANY_OR_OMIT:
      error("Restriction on %s does not allow usage of %s",
            definition_name, get_templatetype_str());
      break;
    default:
      // others will be converted to specific value
      break;
    }
    return needs_runtime_check;
  }

  bool Template::chk_restriction_refd(const char* definition_name,
    template_restriction_t template_restriction, const Location* usage_loc)
  {
    bool needs_runtime_check = false;
    Common::Assignment* ass = u.ref.ref->get_refd_assignment();
    if (!ass) FATAL_ERROR("Template::chk_restriction_refd()");
    // get the restriction on the referenced template
    template_restriction_t refd_tr = TR_NONE;
    bool is_var_template = false;
    switch (ass->get_asstype()) {
    case Common::Assignment::A_TEMPLATE: {
      Template* t_last = get_template_refd_last();
      if (t_last!=this) {
        bool nrc = t_last->chk_restriction(definition_name,
          template_restriction, usage_loc);
        needs_runtime_check = needs_runtime_check || nrc;
      }
      break; }
    case Common::Assignment::A_MODULEPAR_TEMP:
      is_var_template = true;
      refd_tr = TR_NONE; // can't place restriction on this thing
      break;
    case Common::Assignment::A_VAR_TEMPLATE: {
      Def_Var_Template* dvt = dynamic_cast<Def_Var_Template*>(ass);
      if (!dvt) FATAL_ERROR("Template::chk_restriction_refd()");
      is_var_template = true;
      refd_tr = dvt->get_template_restriction();
      break; }
    case Common::Assignment::A_EXT_FUNCTION_RTEMP:
      // we do not trust external functions because there is no generated
      // restriction check on their return value
      if (template_restriction!=TR_NONE) needs_runtime_check = true;
      // no break
    case Common::Assignment::A_FUNCTION_RTEMP: {
      Def_Function_Base* dfb = dynamic_cast<Def_Function_Base*>(ass);
      if (!dfb) FATAL_ERROR("Template::chk_restriction_refd()");
      is_var_template = true;
      refd_tr = dfb->get_template_restriction();
      break; }
    case Common::Assignment::A_PAR_TEMPL_IN:
    case Common::Assignment::A_PAR_TEMPL_OUT:
    case Common::Assignment::A_PAR_TEMPL_INOUT: {
      FormalPar* fp = dynamic_cast<FormalPar*>(ass);
      if (!fp) FATAL_ERROR("Template::chk_restriction_refd()");
      is_var_template = true;
      refd_tr = fp->get_template_restriction();
      break; }
    default:
      break;
    }
    if (is_var_template) {
      // check the restriction
      refd_tr = get_sub_restriction(refd_tr, u.ref.ref);
      // if restriction not satisfied issue warning
      if (is_less_restrictive(template_restriction, refd_tr)) {
        needs_runtime_check = true;
        warning("Inadequate restriction on the referenced %s `%s', this may "
          "cause a dynamic test case error at runtime", ass->get_assname(),
          u.ref.ref->get_dispname().c_str());
        ass->note("Referenced %s is here", ass->get_assname());
      }
    }
    return needs_runtime_check;
  }

  bool Template::chk_restriction(const char* definition_name,
                                 template_restriction_t template_restriction,
                                 const Location* usage_loc)
  {
    bool needs_runtime_check = false;
    bool erroneous = false;
    switch (template_restriction) {
    case TR_NONE:
      break;
    case TR_OMIT:
    case TR_VALUE:
      if (length_restriction) {
        usage_loc->error("Restriction on %s does not allow usage of length "
          "restriction", definition_name);
        erroneous = true;
      }
      if (is_ifpresent) {
        usage_loc->error("Restriction on %s does not allow usage of `ifpresent'",
          definition_name);
        erroneous = true;
      }
      switch(templatetype) {
      case TEMPLATE_ERROR:
        break;
      case TEMPLATE_NOTUSED:
        if (base_template) {
          bool nrc = base_template->chk_restriction(definition_name,
            template_restriction, usage_loc);
          needs_runtime_check = needs_runtime_check || nrc;
        }
        else needs_runtime_check = true;
        break;
      case SPECIFIC_VALUE:
      case TEMPLATE_INVOKE:
        break;
      case TEMPLATE_REFD: {
        bool nrc = chk_restriction_refd(definition_name, template_restriction,
          usage_loc);
        needs_runtime_check = needs_runtime_check || nrc;
      } break;
      case TEMPLATE_LIST:
        for (size_t i = 0; i < u.templates->get_nof_ts(); i++) {
          bool nrc = u.templates->get_t_byIndex(i)->
                       chk_restriction(definition_name, TR_OMIT, usage_loc);
          needs_runtime_check = needs_runtime_check || nrc;
        }
        break;
      case NAMED_TEMPLATE_LIST: {
        map<string, void> checked_map;
        size_t needed_checked_cnt = 0;
        if (base_template && my_governor) {
          switch (my_governor->get_typetype()) {
          case Type::T_SEQ_A:
          case Type::T_SEQ_T:
          case Type::T_SET_A:
          case Type::T_SET_T:
          case Type::T_SIGNATURE:
            needed_checked_cnt = my_governor->get_nof_comps();
            break;
          default:
            break;
          }
        }
        for (size_t i = 0;i < u.named_templates->get_nof_nts(); i++) {
          bool nrc = u.named_templates->get_nt_byIndex(i)->get_template()->
                       chk_restriction(definition_name, TR_OMIT, usage_loc);
          needs_runtime_check = needs_runtime_check || nrc;
          if (needed_checked_cnt)
            checked_map.add(
              u.named_templates->get_nt_byIndex(i)->get_name().get_name(), 0);
        }
        if (needed_checked_cnt) {
          bool nrc = base_template->chk_restriction_named_list(definition_name,
                       checked_map, needed_checked_cnt, usage_loc);
          needs_runtime_check = needs_runtime_check || nrc;
          checked_map.clear();
        }
      } break;
      case INDEXED_TEMPLATE_LIST:
        for (size_t i = 0; i < u.indexed_templates->get_nof_its(); i++) {
          bool nrc = u.indexed_templates->get_it_byIndex(i)->get_template()->
                       chk_restriction(definition_name, TR_OMIT, usage_loc);
          needs_runtime_check = needs_runtime_check || nrc;
        }
        needs_runtime_check = true; // only basic check, needs runtime check
        break;
      case TEMPLATE_CONCAT:
        if (!use_runtime_2) {
          FATAL_ERROR("Template::chk_restriction()");
        }
        u.concat.op1->chk_restriction(definition_name, template_restriction,
          usage_loc);
        u.concat.op2->chk_restriction(definition_name, template_restriction,
          usage_loc);
        break;
      case OMIT_VALUE:
        if (template_restriction==TR_OMIT) break;
        // Else restriction is TR_VALUE, but template type is OMIT:
        // fall through to error.
      default:
        usage_loc->error("Restriction on %s does not allow usage of %s",
          definition_name, get_templatetype_str());
        erroneous = true;
        break;
      }
      break;
    case TR_PRESENT:
      if (is_ifpresent) {
        usage_loc->error("Restriction on %s does not allow usage of `ifpresent'",
          definition_name);
        erroneous = true;
      }
      switch(templatetype) {
      case TEMPLATE_REFD: {
        bool nrc = chk_restriction_refd(definition_name, template_restriction,
          usage_loc);
        needs_runtime_check = needs_runtime_check || nrc;
      } break;
      case VALUE_LIST:
        for (size_t i = 0; i < u.templates->get_nof_ts(); i++) {
          bool nrc = u.templates->get_t_byIndex(i)->
            chk_restriction(definition_name, template_restriction, usage_loc);
          needs_runtime_check = needs_runtime_check || nrc;
        }
        break;
      case COMPLEMENTED_LIST:
        // some basic check, always needs runtime check
        needs_runtime_check = true;
        if (omit_in_value_list) {
          bool has_any_or_omit = false;
          for (size_t i = 0; i < u.templates->get_nof_ts(); i++) {   
            templatetype_t item_templatetype =   
            u.templates->get_t_byIndex(i)->templatetype;   
            if (item_templatetype==OMIT_VALUE || item_templatetype==ANY_OR_OMIT) {   
              has_any_or_omit = true;   
              break;   
            }   
          }   
          if (!has_any_or_omit) {
            usage_loc->error("Restriction on %s does not allow usage of %s without "
              "omit or AnyValueOrNone in the list", definition_name,
              get_templatetype_str());
            erroneous = true;
          }
        }
        break;
      case OMIT_VALUE:
      case ANY_OR_OMIT:
        usage_loc->error("Restriction on %s does not allow usage of %s",
          definition_name, get_templatetype_str());
        erroneous = true;
        break;
      case TEMPLATE_CONCAT:
        if (!use_runtime_2) {
          FATAL_ERROR("Template::chk_restriction()");
        }
        u.concat.op1->chk_restriction(definition_name, template_restriction,
          usage_loc);
        u.concat.op2->chk_restriction(definition_name, template_restriction,
          usage_loc);
        break;
      default:
        break; // all others are ok
      }
      break;
    default:
      FATAL_ERROR("Template::chk_restriction()");
    }
    if (erroneous && usage_loc != this) {
      // display the template's location, too
      note("Referenced template is here");
    }
    return needs_runtime_check;
  }

  void Template::generate_code_expr(expression_struct *expr,
    template_restriction_t template_restriction)
  {
    // Only templates without extra matching attributes can be directly
    // represented in a C++ expression.
    if (!length_restriction && !is_ifpresent
        && template_restriction == TR_NONE) {
      // The single expression must be tried first because this rule might
      // cover some referenced templates.
      if (has_single_expr()) {
        expr->expr = mputstr(expr->expr, get_single_expr(true).c_str());
        return;
      }
      switch (templatetype) {
      case SPECIFIC_VALUE:
        // A simple specific value: use explicit cast.
        expr->expr = mputprintf(expr->expr, "%s(",
          my_governor->get_genname_template(my_scope).c_str());
        u.specific_value->generate_code_expr(expr);
        expr->expr = mputc(expr->expr, ')');
        return;
      case TEMPLATE_REFD:
        // A simple unfoldable referenced template.
        if (!get_needs_conversion()) {
          u.ref.ref->generate_code(expr);
        } else {
          Type *my_gov = get_expr_governor(Type::EXPECTED_TEMPLATE)
            ->get_type_refd_last();
          Type *refd_gov = u.ref.ref->get_refd_assignment()->get_Type()
            ->get_field_type(u.ref.ref->get_subrefs(),
            Type::EXPECTED_TEMPLATE)->get_type_refd_last();
          if (!my_gov || !refd_gov || my_gov == refd_gov)
            FATAL_ERROR("Template::generate_code_expr()");
          expression_struct expr_tmp;
          Code::init_expr(&expr_tmp);
          const string& tmp_id1 = get_temporary_id();
          const char *tmp_id_str1 = tmp_id1.c_str();
          const string& tmp_id2 = get_temporary_id();
          const char *tmp_id_str2 = tmp_id2.c_str();
          expr->preamble = mputprintf(expr->preamble,
            "%s %s;\n", refd_gov->get_genname_template(my_scope).c_str(),
            tmp_id_str1);
          expr_tmp.expr = mputprintf(expr_tmp.expr, "%s = ", tmp_id_str1);
          u.ref.ref->generate_code(&expr_tmp);
          expr->preamble = Code::merge_free_expr(expr->preamble, &expr_tmp);
          expr->preamble = mputprintf(expr->preamble,
            "%s %s;\n"
            "if (!%s(%s, %s)) TTCN_error(\"Values or templates of types `%s' "
            "and `%s' are not compatible at run-time\");\n",
            my_gov->get_genname_template(my_scope).c_str(), tmp_id_str2,
            TypeConv::get_conv_func(refd_gov, my_gov, get_my_scope()
            ->get_scope_mod()).c_str(), tmp_id_str2, tmp_id_str1, my_gov
            ->get_typename().c_str(), refd_gov->get_typename().c_str());
          expr->expr = mputprintf(expr->expr, "%s", tmp_id_str2);
        }
        return;
      case TEMPLATE_INVOKE:
        generate_code_expr_invoke(expr);
        return;
      default:
        break;
      }
    }
    // if none of the above methods are applicable use the most generic and
    // least efficient solution
    // create a temporary object, initialize it and use it in the expression
    const string& tmp_id = get_temporary_id();
    const char *tmp_id_str = tmp_id.c_str();

    expr->preamble = mputprintf(expr->preamble, "%s %s;\n",
      my_governor->get_genname_template(my_scope).c_str(), tmp_id_str);
    set_genname_recursive(tmp_id);
    expr->preamble = generate_code_init(expr->preamble, tmp_id_str);
    if (template_restriction != TR_NONE)
      expr->preamble = Template::generate_restriction_check_code(expr->preamble,
                         tmp_id_str, template_restriction);
    expr->expr = mputstr(expr->expr, tmp_id_str);
  }

  char *Template::generate_code_init(char *str, const char *name)
  {
    if (get_code_generated()) return str;
    set_code_generated();
    switch (templatetype) {
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
    case BSTR_PATTERN:
    case HSTR_PATTERN:
    case OSTR_PATTERN:
      str = mputprintf(str, "%s = %s;\n", name, get_single_expr(false).c_str());
      break;  
    case CSTR_PATTERN:
    case USTR_PATTERN: {
      string preamble;
      string expr = generate_code_str_pattern(false, preamble);
      str = mputprintf(str, "%s%s = %s;\n", preamble.c_str(), name, expr.c_str());
      break; }
    case SPECIFIC_VALUE:
      if (get_code_section() == CS_POST_INIT)
        str = u.specific_value->rearrange_init_code(str, my_scope->get_scope_mod_gen());
      str = u.specific_value->generate_code_init(str, name);
      break;
    case TEMPLATE_REFD:
      str = generate_code_init_refd(str, name);
      break;
    case TEMPLATE_INVOKE:
      if (get_code_section() == CS_POST_INIT)
        str = rearrange_init_code_invoke(str, my_scope->get_scope_mod_gen());
      str = generate_code_init_invoke(str, name);
      break;
    case TEMPLATE_LIST:
    case INDEXED_TEMPLATE_LIST:
      str = generate_code_init_seof(str, name);
      break;
    case NAMED_TEMPLATE_LIST:
      str = generate_code_init_se(str, name);
      break;
    case ALL_FROM:
      str = generate_code_init_all_from(str, name);
      break;
    case VALUE_LIST:
      str = generate_code_init_list(str, name, false);
      break;
    case COMPLEMENTED_LIST:
      str = generate_code_init_list(str, name, true);
      break;
    case VALUE_RANGE:
      if (get_code_section() == CS_POST_INIT)
        str = u.value_range->rearrange_init_code(str, my_scope->get_scope_mod_gen());
      str = u.value_range->generate_code_init(str, name);
      break;
    case SUPERSET_MATCH:
      str = generate_code_init_set(str, name, true);
      break;
    case SUBSET_MATCH:
      str = generate_code_init_set(str, name, false);
      break;
    case PERMUTATION_MATCH:
      warning("Don't know how to init PERMUT");
      str = mputprintf(str, "/* FIXME: PERMUT goes here, name=%s*/\n", name);
      break;
    case DECODE_MATCH:
      str = generate_code_init_dec_match(str, name);
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::generate_code_init()");
      }
      str = generate_code_init_concat(str, name);
      break;
    case TEMPLATE_NOTUSED:
      break;
    case TEMPLATE_ERROR:
      // "default"
      FATAL_ERROR("Template::generate_code_init()");
    }
    if (length_restriction) {
      if (get_code_section() == CS_POST_INIT)
        str = length_restriction->rearrange_init_code(str, my_scope->get_scope_mod_gen());
      str = length_restriction->generate_code_init(str, name);
    }
    if (is_ifpresent) str = mputprintf(str, "%s.set_ifpresent();\n", name);
    return str;
  }

  char *Template::rearrange_init_code(char *str, Common::Module* usage_mod)
  {
    switch (templatetype) {
    case SPECIFIC_VALUE:
      str = u.specific_value->rearrange_init_code(str, usage_mod);
      break;
    case TEMPLATE_REFD:
      str = rearrange_init_code_refd(str, usage_mod);
      break;
    case TEMPLATE_INVOKE:
      str = rearrange_init_code_invoke(str, usage_mod);
      break;
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++)
        str = u.templates->get_t_byIndex(i)->rearrange_init_code(str, usage_mod);
      break;
    case NAMED_TEMPLATE_LIST:
      for (size_t i = 0; i < u.named_templates->get_nof_nts(); i++)
        str = u.named_templates->get_nt_byIndex(i)->get_template()
        ->rearrange_init_code(str, usage_mod);
      break;
    case INDEXED_TEMPLATE_LIST:
      for (size_t i = 0; i < u.indexed_templates->get_nof_its(); i++)
        str = u.indexed_templates->get_it_byIndex(i)->get_template()
          ->rearrange_init_code(str, usage_mod);
      break;
    case VALUE_RANGE:
      str = u.value_range->rearrange_init_code(str, usage_mod);
      break;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::rearrange_init_code()");
      }
      str = u.concat.op1->rearrange_init_code(str, usage_mod);
      str = u.concat.op2->rearrange_init_code(str, usage_mod);
      break;
    default:
      break;
    }
    if (length_restriction) str = length_restriction->rearrange_init_code(str, usage_mod);
    return str;
  }

  bool Template::use_single_expr_for_init()
  {
    Template *t_last = get_template_refd_last();
    // return false in case of unfoldable references
    if (t_last->templatetype == TEMPLATE_REFD) return false;
    // return false if t_last is in a different module
    if (t_last->my_scope->get_scope_mod_gen() != my_scope->get_scope_mod_gen())
      return false;
    // return false if t_last cannot be represented by a single expression
    if (!t_last->has_single_expr()) return false;
    // return true if t_last is a generic wildcard, string pattern, etc.
    if (t_last->templatetype != SPECIFIC_VALUE) return true;
    // examine the specific value
    Value *v_last = t_last->u.specific_value->get_value_refd_last();
    switch (v_last->get_valuetype()) {
    case Value::V_EXPR:
      // do not calculate expressions again
      return false;
    case Value::V_REFD: {
      // v_last is an unfoldable value reference
      // the scope of the definition that v_last refers to
      Scope *v_scope =
        v_last->get_reference()->get_refd_assignment()->get_my_scope();
      for (Scope *t_scope = my_scope; t_scope;
        t_scope = t_scope->get_parent_scope()) {
        // return true if the referred definition is in the same scope
        // as this or in one of the parent scopes of this
        if (t_scope == v_scope) return true;
      }
      // otherwise return false
      return false; }
    default:
      // return true only if v_last is defined in the same module as this
      return v_last->get_my_scope()->get_scope_mod_gen() ==
          my_scope->get_scope_mod_gen();
    }
  }

  char *Template::generate_code_init_refd(char *str, const char *name)
  {
    if (use_single_expr_for_init() && has_single_expr()) {
      str = mputprintf(str, "%s = %s;\n", name,
                       get_single_expr(false).c_str());
    } else {
      expression_struct expr;
      Code::init_expr(&expr);
      bool use_ref_for_codegen = true;
      if (get_code_section() == CS_POST_INIT) {
        // the referencing template is a part of a non-parameterized template
        Common::Assignment *ass = u.ref.ref->get_refd_assignment();
        if (ass->get_asstype() == Common::Assignment::A_TEMPLATE) {
          // the reference points to (a field of) a template
          if (ass->get_FormalParList()) {
            // the referred template is parameterized
            // generate the initialization sequence first for all dependent
            // non-parameterized templates
            str = rearrange_init_code_refd(str, my_scope->get_scope_mod_gen());
          } else if (ass->get_my_scope()->get_scope_mod_gen() ==
                     my_scope->get_scope_mod_gen()) {
            // the referred template is non-parameterized
            // use a different algorithm for code generation
            str = generate_rearrange_init_code_refd(str, &expr);
            use_ref_for_codegen = false;
          }
        }
      }
      if (use_ref_for_codegen) u.ref.ref->generate_code_const_ref(&expr);
      if (expr.preamble || expr.postamble) {
        // the expressions within reference need temporary objects
        str = mputstr(str, "{\n");
        str = mputstr(str, expr.preamble);
        if (use_runtime_2 && get_needs_conversion()) {
          const string& tmp_id = get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          Type *my_gov = my_governor->get_type_refd_last();
          Type *refd_gov = u.ref.ref->get_refd_assignment()->get_Type()
            ->get_type_refd_last();
          if (!my_gov || !refd_gov)
            FATAL_ERROR("Template::generate_code_init_refd()");
          str = mputprintf(str,
            "%s %s;\n"
            "if (!%s(%s, %s)) TTCN_error(\"Values or templates of types `%s' "
            "and `%s' are not compatible at run-time\");\n",
            my_gov->get_genname_template(my_scope).c_str(), tmp_id_str,
            TypeConv::get_conv_func(refd_gov, my_gov, get_my_scope()
            ->get_scope_mod()).c_str(), tmp_id_str, expr.expr, my_gov
            ->get_typename().c_str(), refd_gov->get_typename().c_str());
          str = mputprintf(str, "%s = %s;\n", name, tmp_id_str);
        } else {
          str = mputprintf(str, "%s = %s;\n", name, expr.expr);
        }
        str = mputstr(str, expr.postamble);
        str = mputstr(str, "}\n");
      } else {
        // the reference does not need temporary objects
        if (use_runtime_2 && get_needs_conversion()) {
          const string& tmp_id = get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          Type *my_gov = my_governor->get_type_refd_last();
          Type *refd_gov = u.ref.ref->get_refd_assignment()->get_Type()
            ->get_type_refd_last();
          if (!my_gov || !refd_gov)
            FATAL_ERROR("Template::generate_code_init_refd()");
          str = mputprintf(str,
            "%s %s;\n"
            "if (!%s(%s, %s)) TTCN_error(\"Values or templates of types `%s' "
            "and `%s' are not compatible at run-time\");\n",
            my_gov->get_genname_template(my_scope).c_str(), tmp_id_str,
            TypeConv::get_conv_func(refd_gov, my_gov, get_my_scope()
            ->get_scope_mod()).c_str(), tmp_id_str, expr.expr, my_gov
            ->get_typename().c_str(), refd_gov->get_typename().c_str());
          str = mputprintf(str, "%s = %s;\n", name, tmp_id_str);
        } else {
          str = mputprintf(str, "%s = %s;\n", name, expr.expr);
        }
      }
      Code::free_expr(&expr);
    }
    return str;
  }

  char *Template::generate_code_init_invoke(char *str, const char *name)
  {
    expression_struct expr;
    Code::init_expr(&expr);
    expr.expr = mputprintf(expr.expr, "%s = ", name);
    generate_code_expr_invoke(&expr);
    return Code::merge_free_expr(str, &expr);
  }

  char *Template::generate_rearrange_init_code_refd(char *str,
    expression_struct *expr)
  {
    /** Initially we can assume that:
     *  - this is a referenced template and a part of a non-parameterized
     *    template
     *  - u.ref.ref points to (a field of) a non-parameterized template within
     *    the same module as \a this.
     *  - this ensures that the do-while loop will run at least twice (i.e. the
     *    first continue statement will be reached in the first iteration) */
    stack<FieldOrArrayRef> refstack;
    Template *t = this;
    // first try to find the smallest dependent template
    for ( ; ; ) {
      if (t->templatetype == TEMPLATE_REFD) {
        Common::Assignment *ass = t->u.ref.ref->get_refd_assignment();
        /** Don't follow the reference if:
         *  - the referenced definition is not a template
         *  - the referenced template is parameterized or
         *  - the referenced template is in different module */
        if (ass->get_asstype() == Common::Assignment::A_TEMPLATE &&
          ass->get_FormalParList() == 0 &&
          ass->get_my_scope()->get_scope_mod_gen() ==
            my_scope->get_scope_mod_gen()) {
          // accumulate the sub-references of the referred reference
          FieldOrArrayRefs *subrefs = t->u.ref.ref->get_subrefs();
          if (subrefs) {
            for (size_t i = subrefs->get_nof_refs(); i > 0; i--)
              refstack.push(subrefs->get_ref(i - 1));
          }
          // jump to the referred top-level template
          t = ass->get_Template();
          // start the iteration from the beginning
          continue;
          // stop otherwise: the reference cannot be followed
        } else break;
      }
      // stop if there are no sub-references
      if (refstack.empty()) break;
      // take the topmost sub-reference
      FieldOrArrayRef *subref = refstack.top();
      if (subref->get_type() == FieldOrArrayRef::FIELD_REF) {
        if (t->templatetype != NAMED_TEMPLATE_LIST) break;
        // the field reference can be followed
        t = t->u.named_templates->get_nt_byName(*subref->get_id())
          ->get_template();
      } else {
        // trying to follow an array reference
        if (t->templatetype != TEMPLATE_LIST) break;
        Value *array_index = subref->get_val()->get_value_refd_last();
        if (array_index->get_valuetype() != Value::V_INT) break;
        // the index is available at compilation time
        Int index = array_index->get_val_Int()->get_val();
        // index transformation in case of arrays
        if (t->my_governor->get_typetype() == Type::T_ARRAY)
          index -= t->my_governor->get_dimension()->get_offset();
        t = t->get_listitem_byIndex((size_t)index);
      }
      // the topmost sub-reference was processed
      // it can be erased from the stack
      refstack.pop();
    }
    // the smallest dependent template is now in t
    // generate the initializer sequence for t
    str = t->generate_code_init(str, t->get_lhs_name().c_str());
    // the equivalent C++ code of the referenced template is composed of the
    // genname of t and the remained sub-references in refstack
    expr->expr = mputstr(expr->expr, t->get_genname_own(my_scope).c_str());
    while (!refstack.empty()) {
      FieldOrArrayRef *subref = refstack.pop();
      if (subref->get_type() == FieldOrArrayRef::FIELD_REF) {
        expr->expr = mputprintf(expr->expr, ".%s()",
          subref->get_id()->get_name().c_str());
      } else {
        expr->expr = mputc(expr->expr, '[');
        subref->get_val()->generate_code_expr(expr);
        expr->expr = mputc(expr->expr, ']');
      }
    }
    return str;
  }

  bool Template::compile_time() const
  {
    switch (templatetype) {
    case ALL_FROM:
      return false;
    case TEMPLATE_ERROR: /**< erroneous template */
    case TEMPLATE_NOTUSED: /**< not used symbol (-) */
    case OMIT_VALUE: /**< omit */
    case ANY_VALUE: /**< any value (?) */
    case ANY_OR_OMIT: /**< any or omit (*) */
    case SPECIFIC_VALUE: /**< specific value */
    case TEMPLATE_REFD: /**< reference to another template */
    case VALUE_RANGE: /**< value range match */
    case BSTR_PATTERN: /**< bitstring pattern */
    case HSTR_PATTERN: /**< hexstring pattern */
    case OSTR_PATTERN: /**< octetstring pattern */
    case CSTR_PATTERN: /**< character string pattern */
    case USTR_PATTERN:  /**< universal charstring pattern */
    case TEMPLATE_INVOKE:
    case DECODE_MATCH:
      // Simple templates can be computed at compile time
      return true;

      // "Complex" templates need to look at all elements
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++)
        if (u.templates->get_t_byIndex(i)->compile_time()) continue;
        else return false;
      return true;
    case NAMED_TEMPLATE_LIST:
      for (size_t i = 0; i < u.named_templates->get_nof_nts(); i++)
        if (!u.named_templates->get_nt_byIndex(i)->get_template()->compile_time())
          return false;
      return true;
    case INDEXED_TEMPLATE_LIST:
      for (size_t i = 0; i <u.indexed_templates->get_nof_its(); i++)
        if (!u.indexed_templates->get_it_byIndex(i)->get_template()->compile_time())
          return false;
      return true;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::compile_time()");
      }
      // record of/set of concatenation is not evaluated at compile-time
      return false;
    }

    return true; // not reached
  }

  char *Template::generate_code_init_seof(char *str, const char *name)
  {
    switch (templatetype) {
    case TEMPLATE_LIST: {
      size_t nof_ts = u.templates->get_nof_ts();
      if (nof_ts == 0) {
        str = mputprintf(str, "%s = NULL_VALUE;\n", name);
        break;
      }

      // nof_ts > 0
      Type *t_last = my_governor->get_type_refd_last();
      Int index_offset;
      if (t_last->get_typetype() == Type::T_ARRAY) {
        // take the start index from the array dimension
        index_offset = t_last->get_dimension()->get_offset();
      } else {
        // indexing always starts from zero
        index_offset = 0;
      }

      const string& oftype_name =
        t_last->get_ofType()->get_genname_template(my_scope);
      const char *oftype_name_str = oftype_name.c_str();

      ReferenceChain refch (this, "While searching template");
      
      if (compile_time()) goto compile_time;
      {
        // run-time, variable sized init

        // This is a record-of var template, like this:
        // var template rec_of_int vt_r := {
        //   1, 2, 3, permutation(10, all from x, 20), 4, 5 }
        //   ^^^^^^^--- these ------------------------^^^^^
        //   are known at compile time, but the length of the "all from"
        //   is only known at run time.
        // Collect the indices where there is an "all from".
        dynamic_array<size_t> variables;
        size_t fixed_part = 0;
        if (has_permutation) {
          for (size_t i = 0; i < nof_ts; i++) {
            Template *t = u.templates->get_t_byIndex(i);
            if (t->templatetype == PERMUTATION_MATCH) {
              size_t num_p = t->u.templates->get_nof_ts();
              // t->u.templates is 2: "hello" and all from ...
              for (size_t j = 0; j < num_p; ++j) {
                Template *subt = t->u.templates->get_t_byIndex(j);
                if (subt->templatetype == ALL_FROM) {
                  variables.add(j);
                }
                else fixed_part++;
              }
            }
            else fixed_part++;
          }

          char* str_preamble = 0;
          char* str_set_size = mputprintf(0, "%s.set_size(%lu", name,
            (unsigned long)fixed_part);

          // variable part
          for (size_t i = 0; i < nof_ts; i++) {
            Template *t = u.templates->get_t_byIndex(i);
            for (size_t k = 0, v = variables.size(); k < v; ++k) {
              if (t->templatetype != PERMUTATION_MATCH) continue; // ?? really nothing to do ??
              Template *subt = t->u.templates->get_t_byIndex(variables[k]);
              if (subt->templatetype == ALL_FROM) {
                Value *refv = subt->u.all_from->u.specific_value;
                // don't call get_Value(), it rips out the value from the template

                if (refv->get_valuetype()!=Value::V_REFD) FATAL_ERROR("%s", __FUNCTION__);
                Common::Reference  *ref = refv->get_reference();
                FieldOrArrayRefs   *subrefs = ref->get_subrefs();
                Common::Assignment *ass = ref->get_refd_assignment();

                str_set_size = mputstrn(str_set_size, " + ", 3);

                Ref_pard* ref_pard = dynamic_cast<Ref_pard*>(ref);
                if (ref_pard) {
                  // in case of parametrised references:
                  //  - temporary parameters need to be declared (stored in str_preamble)
                  //  - the same temporary needs to be used at each call (generate_code_cached call)
                  expression_struct expr;
                  Code::init_expr(&expr);

                  ref_pard->generate_code_cached(&expr);
                  str_set_size = mputprintf(str_set_size, "%s", expr.expr);
                  if (expr.preamble)
                    str_preamble = mputstr(str_preamble, expr.preamble);

                  Code::free_expr(&expr);
                }
                else {
                  str_set_size = mputstr (str_set_size, ass->get_id().get_name().c_str());
                  if (subrefs) {
                    expression_struct expr;
                    Code::init_expr(&expr);

                    subrefs->generate_code(&expr, ass);
                    str_set_size = mputprintf(str_set_size, "%s", expr.expr);

                    Code::free_expr(&expr);
                  }
                }

                switch(ass->get_asstype()) {
                case Common::Assignment::A_CONST:
                case Common::Assignment::A_EXT_CONST:
                case Common::Assignment::A_MODULEPAR:
                case Common::Assignment::A_VAR:
                case Common::Assignment::A_PAR_VAL_IN:
                case Common::Assignment::A_PAR_VAL_OUT:
                case Common::Assignment::A_PAR_VAL_INOUT:
                case Common::Assignment::A_FUNCTION_RVAL:
                case Common::Assignment::A_EXT_FUNCTION_RVAL:
                  if (ass->get_Type()->field_is_optional(subrefs)) {
                    str_set_size = mputstrn(str_set_size, "()", 2);
                  }
                  break;
                default:
                  break;
                }

                str_set_size = mputstr(str_set_size, ".n_elem()");
              }
            }
          }

          str = mputstr(str, str_preamble);
          str = mputstr(str, str_set_size);

          Free(str_preamble);
          Free(str_set_size);

          str = mputstrn(str, ");\n", 3); // finally done set_size
          
          size_t index = 0;
          string skipper, hopper;
          for (size_t i = 0; i < nof_ts; i++) {
            Template *t = u.templates->get_t_byIndex(i);
            switch (t->templatetype) {
            case ALL_FROM: {
              break; }
            case PERMUTATION_MATCH: {
              size_t nof_perm_ts = t->u.templates->get_nof_ts();
              for (size_t j = 0; j < nof_perm_ts; j++) {
                Int ix(index_offset + index + j);
                Template *permut_elem = t->u.templates->get_t_byIndex(j);
                if (permut_elem->templatetype == ALL_FROM) {
                  expression_struct expr;
                  Code::init_expr(&expr);
                  switch (permut_elem->u.all_from->templatetype) {
                  case SPECIFIC_VALUE: {
                    Value *spec = permut_elem->u.all_from->u.specific_value;
                    switch (spec->get_valuetype()) {
                    case Common::Value::V_REFD: {
                      Common::Reference  *ref = spec->get_reference();
                      
                      Ref_pard* ref_pard = dynamic_cast<Ref_pard*>(ref);
                      if (ref_pard)
                        ref_pard->generate_code_cached(&expr);
                      else
                        ref->generate_code(&expr);

                      Common::Assignment* ass = ref->get_refd_assignment();
                      switch(ass->get_asstype()) {
                      case Common::Assignment::A_CONST:
                      case Common::Assignment::A_EXT_CONST:
                      case Common::Assignment::A_MODULEPAR:
                      case Common::Assignment::A_VAR:
                      case Common::Assignment::A_PAR_VAL_IN:
                      case Common::Assignment::A_PAR_VAL_OUT:
                      case Common::Assignment::A_PAR_VAL_INOUT:
                      case Common::Assignment::A_FUNCTION_RVAL:
                      case Common::Assignment::A_EXT_FUNCTION_RVAL:
                        if (ass->get_Type()->field_is_optional(ref->get_subrefs())) {
                          expr.expr = mputstrn(expr.expr, "()", 2);
                        }
                        break;
                      default:
                        break;
                      }
                      
                      break; }
                    default:
                      FATAL_ERROR("vtype %d", spec->get_valuetype());
                      break;
                    }
                    break; }


                  default:
                    FATAL_ERROR("ttype %d", permut_elem->u.all_from->templatetype);
                    break;
                  }
                  str = mputprintf(str,
                    "for (int i_i = 0, i_lim = %s.n_elem(); i_i < i_lim; ++i_i) {\n",
                    expr.expr);

                  str = permut_elem->generate_code_init_seof_element(str, name,
                    (Int2string(ix) + skipper + " + i_i").c_str(),
                    oftype_name_str);

                  str = mputstrn(str, "}\n", 2);
                  skipper += "-1+";
                  skipper += expr.expr;
                  skipper += ".n_elem() /* 3005 */ ";
                  Code::free_expr(&expr);
                }
                else {
                  str = permut_elem->generate_code_init_seof_element(str, name,
                    (Int2string(ix) + skipper).c_str(), oftype_name_str);
                }
              }
              // do not consider index_offset in case of permutation indicators
              str = mputprintf(str, "%s.add_permutation(%lu%s, %lu%s);\n", name,
                (unsigned long)index,                     hopper.c_str(),
                (unsigned long)(index + nof_perm_ts - 1), skipper.c_str());
              hopper = skipper;
              t->set_code_generated();
              index += nof_perm_ts;
              break; }

            default:
              str = t->generate_code_init_seof_element(str, name,
                (Int2string(index_offset + index) + skipper).c_str(), oftype_name_str);
              // no break
            case TEMPLATE_NOTUSED:
              index++;
              break;
            }
          }

          break;

        }


        if (!has_permutation && has_allfrom()) {
          for (size_t i = 0; i < nof_ts; i++) {
            Template *t = u.templates->get_t_byIndex(i);
            if (t->templatetype == ALL_FROM) {
              variables.add(i);
            }
            else {
              fixed_part++;
            }
          }
          char* str_preamble = 0;
          char* str_set_size = mputprintf(0, "%s.set_size(%lu", name,
            (unsigned long)fixed_part);

          // variable part
          for (size_t i = 0, v = variables.size(); i < v; ++i) {
            Template *t = u.templates->get_t_byIndex(variables[i]);
            if (t->templatetype == ALL_FROM) {
              Value *refv = t->u.all_from->u.specific_value;
              // don't call get_Value(), it rips out the value from the template
              if (refv->get_valuetype()!=Value::V_REFD) FATAL_ERROR("%s", __FUNCTION__);
              Common::Reference  *ref = refv->get_reference();
              FieldOrArrayRefs   *subrefs = ref->get_subrefs();
              Common::Assignment *ass = ref->get_refd_assignment();
              str_set_size = mputstrn(str_set_size, " + ", 3);
              Ref_pard* ref_pard = dynamic_cast<Ref_pard*>(ref);
              if (ref_pard) {
                 // in case of parametrised references:
                 //  - temporary parameters need to be declared (stored in str_preamble)
                 //  - the same temporary needs to be used at each call (generate_code_cached call)
                expression_struct expr;
                Code::init_expr(&expr);

                ref_pard->generate_code_cached(&expr);
                str_set_size = mputprintf(str_set_size, "%s", expr.expr);
                if (expr.preamble)
                  str_preamble = mputstr(str_preamble, expr.preamble);
                Code::free_expr(&expr);
              }
              else {
                str_set_size = mputstr (str_set_size, ass->get_id().get_name().c_str());
                if (subrefs) {
                  expression_struct expr;
                  Code::init_expr(&expr);
                  subrefs->generate_code(&expr, ass);
                  str_set_size = mputprintf(str_set_size, "%s", expr.expr);
                  Code::free_expr(&expr);
                }
              }

              switch(ass->get_asstype()) {
              case Common::Assignment::A_CONST:
              case Common::Assignment::A_EXT_CONST:
              case Common::Assignment::A_MODULEPAR:
              case Common::Assignment::A_VAR:
              case Common::Assignment::A_PAR_VAL_IN:
              case Common::Assignment::A_PAR_VAL_OUT:
              case Common::Assignment::A_PAR_VAL_INOUT:
              case Common::Assignment::A_FUNCTION_RVAL:
              case Common::Assignment::A_EXT_FUNCTION_RVAL:
                if (ass->get_Type()->field_is_optional(subrefs)) {
                  str_set_size = mputstrn(str_set_size, "()", 2);
                }
                break;
              default:
                break;
              }

              str_set_size = mputstr(str_set_size, ".n_elem()");
            }
          }
          str = mputstr(str, str_preamble);
          str = mputstr(str, str_set_size);
          Free(str_preamble);
          Free(str_set_size);
          str = mputstrn(str, ");\n", 3); // finally done set_size

          size_t index = 0;
          string skipper;
          for (size_t i = 0; i < nof_ts; i++) {
            Template *t = u.templates->get_t_byIndex(i);
            Int ix(index_offset + i);
            switch (t->templatetype) {
            case ALL_FROM: {
              expression_struct expr;
              Code::init_expr(&expr);
              switch (t->u.all_from->templatetype) {
                case SPECIFIC_VALUE: {
                  Value *spec = t->u.all_from->u.specific_value;
                  switch (spec->get_valuetype()) {
                  case Common::Value::V_REFD: {
                    Common::Reference  *ref = spec->get_reference();
                    Ref_pard* ref_pard = dynamic_cast<Ref_pard*>(ref);
                    if (ref_pard)
                      ref_pard->generate_code_cached(&expr);
                    else
                      ref->generate_code(&expr);

                    Common::Assignment* ass = ref->get_refd_assignment();
                    switch(ass->get_asstype()) {
                    case Common::Assignment::A_CONST:
                    case Common::Assignment::A_EXT_CONST:
                    case Common::Assignment::A_MODULEPAR:
                    case Common::Assignment::A_VAR:
                    case Common::Assignment::A_PAR_VAL_IN:
                    case Common::Assignment::A_PAR_VAL_OUT:
                    case Common::Assignment::A_PAR_VAL_INOUT:
                    case Common::Assignment::A_FUNCTION_RVAL:
                    case Common::Assignment::A_EXT_FUNCTION_RVAL:
                      if (ass->get_Type()->field_is_optional(ref->get_subrefs())) {
                        expr.expr = mputstrn(expr.expr, "()", 2);
                      }
                      break;
                    default:
                      break;
                    }

                    break; }
                  default:
                    FATAL_ERROR("vtype %d", spec->get_valuetype());
                    break;
                  }
                  break; }
                default: {
                  FATAL_ERROR("ttype %d", t->u.all_from->templatetype);
                  break; }
              }
              str = mputprintf(str,
                "for (int i_i = 0, i_lim = %s.n_elem(); i_i < i_lim; ++i_i) {\n",
                expr.expr);
              str = t->generate_code_init_seof_element(str, name,
                (Int2string(ix) + skipper + " + i_i").c_str(),
                oftype_name_str);
              str = mputstrn(str, "}\n", 2);
              skipper += "-1+";
              skipper += expr.expr;
              skipper += ".n_elem() ";
              Code::free_expr(&expr);
              t->set_code_generated();
              ++index;
              break; }
            default: {
               str = t->generate_code_init_seof_element(str, name,
                 (Int2string(index_offset + index) + skipper).c_str(), oftype_name_str);
               // no break
            case TEMPLATE_NOTUSED:
               ++index;
               break; }
            }
          }
          break;
        }

        // else carry on
      }
compile_time:
      // setting the size first
      if (!has_allfrom())
        str = mputprintf(str, "%s.set_size(%lu);\n", name, (unsigned long) get_nof_listitems());
      // determining the index offset based on the governor

      size_t index = 0;
      for (size_t i = 0; i < nof_ts; i++) {
        Template *t = u.templates->get_t_byIndex(i);
        switch (t->templatetype) {
        case PERMUTATION_MATCH: {
          size_t nof_perm_ts = t->u.templates->get_nof_ts();
          for (size_t j = 0; j < nof_perm_ts; j++) {
            Int ix(index_offset + index + j);
            str = t->u.templates->get_t_byIndex(j)
                  ->generate_code_init_seof_element(str, name,
                    Int2string(ix).c_str(), oftype_name_str);
          }
          // do not consider index_offset in case of permutation indicators
          str = mputprintf(str, "%s.add_permutation(%lu, %lu);\n", name,
            (unsigned long)index, (unsigned long) (index + nof_perm_ts - 1));
          t->set_code_generated();
          index += nof_perm_ts;
          break; }

        default:
          str = t->generate_code_init_seof_element(str, name,
            Int2string(index_offset + index).c_str(), oftype_name_str);
          // no break
        case ALL_FROM:
        case TEMPLATE_NOTUSED:
          index++;
        }
      }
      break; }
    case INDEXED_TEMPLATE_LIST: {
      size_t nof_its = u.indexed_templates->get_nof_its();
      if (nof_its > 0) {
        Type *t_last = my_governor->get_type_refd_last();
        const string& oftype_name =
          t_last->get_ofType()->get_genname_template(my_scope);
        const char *oftype_name_str = oftype_name.c_str();
        // There's no need to generate a set_size call here.  To do that, we
        // should know the size of the base template, which is not available
        // from here.
        for (size_t i = 0; i < nof_its; i++) {
          IndexedTemplate *it = u.indexed_templates->get_it_byIndex(i);
          const string& tmp_id_1 = get_temporary_id();
          str = mputstr(str, "{\n");
          Value *index = (it->get_index()).get_val();
          if (index->get_valuetype() != Value::V_INT) {
            const string& tmp_id_2 = get_temporary_id();
            str = mputprintf(str, "int %s;\n", tmp_id_2.c_str());
            str = index->generate_code_init(str, tmp_id_2.c_str());
            str = mputprintf(str, "%s& %s = %s[%s];\n", oftype_name_str,
                    tmp_id_1.c_str(), name, tmp_id_2.c_str());
          } else {
              str = mputprintf(str, "%s& %s = %s[%s];\n", oftype_name_str,
                      tmp_id_1.c_str(), name,
                  Int2string(index->get_val_Int()->get_val()).c_str());
          }
          str = it->get_template()->generate_code_init(str, tmp_id_1.c_str());
          str = mputstr(str, "}\n");
        }
      } else {
        // It seems to be impossible in this case.
        str = mputprintf(str, "%s = NULL_VALUE;\n", name);
      }
      break; }
    default:
      FATAL_ERROR("Template::generate_code_init_seof()");
      return NULL;
    }
    return str;
  }

  char *Template::generate_code_init_seof_element(char *str, const char *name,
    const char* index, const char *element_type_genname)
  {
    if (needs_temp_ref()) {
      const string& tmp_id = get_temporary_id();
      str = mputprintf(str, "{\n"
        "%s& %s = %s[%s];\n",
        element_type_genname, tmp_id.c_str(), name, index);
      str = generate_code_init(str, tmp_id.c_str());
      str = mputstr(str, "}\n");
    } else {
      char *embedded_name = mprintf("%s[%s]", name, index);
      str = generate_code_init(str, embedded_name);
      Free(embedded_name);
    }
    return str;
  }

  char *Template::generate_code_init_all_from(char *str, const char *name)
  {
    // we are ALL_FROM, hence u.all_from
    switch (u.all_from->templatetype) {
    case SPECIFIC_VALUE: {
      Value *spec = u.all_from->u.specific_value;
      switch (spec->get_valuetype()) {
      case Common::Value::V_REFD: {
        Common::Reference  *ref = spec->get_reference();
        expression_struct expr;
        Code::init_expr(&expr);
        Ref_pard* ref_pard = dynamic_cast<Ref_pard*>(ref);
        if (ref_pard)
          ref_pard->generate_code_cached(&expr);
        else
          ref->generate_code(&expr);

        Common::Assignment* ass = ref->get_refd_assignment();
        switch(ass->get_asstype()) {
        case Common::Assignment::A_CONST:
        case Common::Assignment::A_EXT_CONST:
        case Common::Assignment::A_MODULEPAR:
        case Common::Assignment::A_VAR:
        case Common::Assignment::A_PAR_VAL_IN:
        case Common::Assignment::A_PAR_VAL_OUT:
        case Common::Assignment::A_PAR_VAL_INOUT:
        case Common::Assignment::A_FUNCTION_RVAL:
        case Common::Assignment::A_EXT_FUNCTION_RVAL:
          if (ass->get_Type()->field_is_optional(ref->get_subrefs())) {
            expr.expr = mputstrn(expr.expr, "()", 2);
          }
          break;
        default:
          break;
        }
        
        str = mputprintf(str, "%s = %s[i_i];\n", name, expr.expr);
        // The caller will have to provide the for cycle with this variable
        Code::free_expr(&expr);
        break; }
      default:
        break;
      }
      break; }
    default:
      break;
    }
    return str;
  }

  char *Template::generate_code_init_se(char *str, const char *name)
  { // named template list
    Type *type = my_governor->get_type_refd_last();
    if (type->get_nof_comps() > 0) {
      size_t nof_nts = u.named_templates->get_nof_nts();
      for (size_t i = 0; i < nof_nts; i++) {
        NamedTemplate *nt = u.named_templates->get_nt_byIndex(i);
        const Identifier& fieldname = nt->get_name();
        char *fieldname_str = mprintf("%s%s",
              type->get_typetype()==Type::T_ANYTYPE ? "AT_" : "", fieldname.get_name().c_str());
        Template *t = nt->get_template();
        if (t->needs_temp_ref()) {
          Type *field_type;
          if (type->get_typetype() == Type::T_SIGNATURE) {
            field_type = type->get_signature_parameters()
              ->get_param_byName(fieldname)->get_type();
          } else {
            field_type = type->get_comp_byName(fieldname)->get_type();
          }
          const string& tmp_id = get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          str = mputprintf(str, "{\n"
            "%s& %s = %s.%s();\n",
            field_type->get_genname_template(my_scope).c_str(), tmp_id_str,
            name, fieldname_str);
          str = t->generate_code_init(str, tmp_id_str);
          str = mputstr(str, "}\n");
        } else {
          char *embedded_name = mprintf("%s.%s()", name, fieldname_str);
          str = t->generate_code_init(str, embedded_name);
          Free(embedded_name);
        }
        Free(const_cast<char*>(fieldname_str));
      }
    } else {
      str = mputprintf(str, "%s = NULL_VALUE;\n", name);
    }
    return str;
  }

  char *Template::generate_code_init_list(char *str, const char *name,
     bool is_complemented) // VALUE_LIST or COMPLEMENTED_LIST
  {
    size_t nof_ts = u.templates->get_nof_ts();
    const string& type_name = my_governor->get_genname_template(my_scope);
    const char *type_name_str = type_name.c_str();

    dynamic_array<size_t> variables;
    size_t fixed_part = 0;
    for (size_t i = 0; i < nof_ts; ++i) {
      Template *t = u.templates->get_t_byIndex(i);
      if (t->templatetype == ALL_FROM) {
        variables.add(i);
      }
      else ++fixed_part;
    }

    if (variables.size() > 0) {
      char* str_preamble = 0;
      char* str_set_type = mprintf("%s.set_type(%s, %lu", name,
        (is_complemented ? "COMPLEMENTED_LIST" : "VALUE_LIST"),
        (unsigned long)fixed_part);
      // The code to compute the number of elements at run time (the variable part).
      // This is the sum of sizes of "all from"s.
      for (size_t v = 0, vs = variables.size(); v < vs; ++v) {
        Template *vt = u.templates->get_t_byIndex(variables[v]);
        if (vt->templatetype != ALL_FROM) FATAL_ERROR("must be ALL_FROM");
        if (vt->u.all_from->templatetype != SPECIFIC_VALUE) FATAL_ERROR("not value");
        Value *val = vt->u.all_from->u.specific_value;
        if (val->get_valuetype() != Value::V_REFD) FATAL_ERROR("ref expected from val");
        Common::Reference  *ref = val->get_reference();
        FieldOrArrayRefs   *subrefs = ref->get_subrefs();
        Common::Assignment *ass = ref->get_refd_assignment();
        if (!ass) FATAL_ERROR("Could not grab ass!");

        str_set_type = mputstrn(str_set_type, " + ", 3);
        
        Ref_pard* ref_pard = dynamic_cast<Ref_pard*>(ref);
        if (ref_pard) {
          // in case of parametrised references:
          //  - temporary parameters need to be declared (stored in str_preamble)
          //  - the same temporary needs to be used at each call (generate_code_cached call)
          expression_struct expr;
          Code::init_expr(&expr);

          ref_pard->generate_code_cached(&expr);
          str_set_type = mputprintf(str_set_type, "%s", expr.expr);
          if (expr.preamble)
            str_preamble = mputstr(str_preamble, expr.preamble);

          Code::free_expr(&expr);
        }
        else {
          expression_struct expr;
          Code::init_expr(&expr);

          ref->generate_code(&expr);
          str_set_type = mputprintf(str_set_type, "%s", expr.expr);

          Code::free_expr(&expr);
        }
        
        switch(ass->get_asstype()) {
        case Common::Assignment::A_CONST:
        case Common::Assignment::A_EXT_CONST:
        case Common::Assignment::A_MODULEPAR:
        case Common::Assignment::A_VAR:
        case Common::Assignment::A_PAR_VAL_IN:
        case Common::Assignment::A_PAR_VAL_OUT:
        case Common::Assignment::A_PAR_VAL_INOUT:
        case Common::Assignment::A_FUNCTION_RVAL:
        case Common::Assignment::A_EXT_FUNCTION_RVAL:
          if (ass->get_Type()->field_is_optional(subrefs)) {
            str_set_type = mputstrn(str_set_type, "()", 2);
          }
          break;
        default:
          break;
        }
        
        str_set_type = mputstr(str_set_type, ".n_elem()");
      }
      
      str = mputstr(str, str_preamble);
      str = mputstr(str, str_set_type);
      
      Free(str_preamble);
      Free(str_set_type);
      
      str = mputstrn(str, ");\n", 3);
      
      string shifty; // contains the expression used to calculate
      // the size increase due to the runtime expansion of "all from".
      // In nof_ts, each "all from" appears as 1, but actually contributes
      // more. So the increase (by which all elements after the "all from"
      // are shifted) is:Â    target_of_all_from.n_elem()-1
      // This needs to be accumulated for each "all from".
      for (size_t vi = 0; vi < nof_ts; ++vi) {
        Template *t = u.templates->get_t_byIndex(vi);
        switch (t->templatetype) {
        case ALL_FROM: {
          expression_struct expr;
          Code::init_expr(&expr);
          // variable one
          switch (t->u.all_from->templatetype) {
          case SPECIFIC_VALUE: {
            Value *sv = t->u.all_from->u.specific_value;
            switch (sv->get_valuetype()) {
            case Value::V_REFD: {
              Common::Reference *ref = sv->get_reference();             
              Ref_pard* ref_pard = dynamic_cast<Ref_pard*>(ref);
              if (ref_pard)
                ref_pard->generate_code_cached(&expr);
              else
                ref->generate_code(&expr);

              Common::Assignment* ass = ref->get_refd_assignment();
              switch(ass->get_asstype()) {
              case Common::Assignment::A_CONST:
              case Common::Assignment::A_EXT_CONST:
              case Common::Assignment::A_MODULEPAR:
              case Common::Assignment::A_VAR:
              case Common::Assignment::A_PAR_VAL_IN:
              case Common::Assignment::A_PAR_VAL_OUT:
              case Common::Assignment::A_PAR_VAL_INOUT:
              case Common::Assignment::A_FUNCTION_RVAL:
              case Common::Assignment::A_EXT_FUNCTION_RVAL:
                if (ass->get_Type()->field_is_optional(ref->get_subrefs())) {
                  expr.expr = mputstrn(expr.expr, "()", 2);
                }
                break;
              default:
                break;
              }

              break; }

            default:
              FATAL_ERROR("VT NOT HANDLED");
            } // switch valuetype
            break; }

          default:
            FATAL_ERROR("ttype not handled");
            break;
          }

          // All local variables should contain a single underscore,
          // to avoid potential clashes with field names.
          str = mputprintf(str, "for (int i_i= 0, i_lim = %s.n_elem(); i_i < i_lim; ++i_i) {\n",
            expr.expr);

          if (t->needs_temp_ref()) {
            // Not possible, because t->templatetype is ALL_FROM
            FATAL_ERROR("temp ref not handled");
          }
          char *embedded_name = mprintf("%s.list_item(%lu + i_i%s)", name,
            (unsigned long) vi, shifty.c_str());
          str = t->generate_code_init(str, embedded_name);
          Free(embedded_name);

          str = mputstrn(str, "}\n", 2);

          shifty += "-1+";
          shifty += expr.expr;
          shifty += ".n_elem() /* 3303 */ ";

          Code::free_expr(&expr);
          break; }

        default: // "fixed one"
          if (t->needs_temp_ref()) { // FIXME: this is copypasta from below / but may need to be changed
            const string& tmp_id = get_temporary_id();
            const char *tmp_id_str = tmp_id.c_str();
            str = mputprintf(str, "{\n"
              "%s& %s = %s.list_item(%lu);\n",
              type_name_str, tmp_id_str, name, (unsigned long) vi);
            str = t->generate_code_init(str, tmp_id_str);
            str = mputstr(str, "}\n");
          } else {
            char *embedded_name = mprintf("%s.list_item(%lu%s)", name,
              (unsigned long) vi, shifty.c_str());
            str = t->generate_code_init(str, embedded_name);
            Free(embedded_name);
          }
          break;
        } // switch t->templatetype
      }
    }
    else {
      str = mputprintf(str, "%s.set_type(%s, %lu);\n", name,
        is_complemented ? "COMPLEMENTED_LIST" : "VALUE_LIST",
          (unsigned long) nof_ts);
      for (size_t i = 0; i < nof_ts; i++) {
        Template *t = u.templates->get_t_byIndex(i);
        if (t->needs_temp_ref()) {
          const string& tmp_id = get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          str = mputprintf(str, "{\n"
            "%s& %s = %s.list_item(%lu);\n",
            type_name_str, tmp_id_str, name, (unsigned long) i);
          str = t->generate_code_init(str, tmp_id_str);
          str = mputstr(str, "}\n");
        } else {
          char *embedded_name = mprintf("%s.list_item(%lu)", name,
            (unsigned long) i);
          str = t->generate_code_init(str, embedded_name);
          Free(embedded_name);
        }
      }
    }
    return str;
  }

  char *Template::generate_code_init_set(char *str, const char *name,
    bool is_superset) // SUPERSET_MATCH and SUBSET_MATCH
  {
    size_t nof_ts = u.templates->get_nof_ts();
    const string& oftype_name =
      my_governor->get_ofType()->get_genname_template(my_scope);
    const char *oftype_name_str = oftype_name.c_str();

    dynamic_array<size_t> variables;
    size_t fixed_part = 0;
    for (size_t i = 0; i < nof_ts; ++i) {
      Template *t = u.templates->get_t_byIndex(i);
      if (t->templatetype == ALL_FROM) {
        variables.add(i);
      }
      else ++fixed_part;
    }

    //warning("There are %lu set elements", nof_ts);
    if (variables.size() > 0) {
      char* str_preamble = 0;
      char* str_set_type = mputprintf(0, "%s.set_type(%s, %lu", name,
        is_superset ? "SUPERSET_MATCH" : "SUBSET_MATCH", (unsigned long) fixed_part);

      for (size_t v = 0, vs = variables.size(); v < vs; ++v) {
        Template *vt = u.templates->get_t_byIndex(variables[v]);
        if (vt->templatetype != ALL_FROM) FATAL_ERROR("must be ALL_FROM");
        if (vt->u.all_from->templatetype != SPECIFIC_VALUE) FATAL_ERROR("not value");
        Value *val = vt->u.all_from->u.specific_value;
        if (val->get_valuetype() != Value::V_REFD) FATAL_ERROR("ref expected from val");
        Common::Reference  *ref = val->get_reference();
        FieldOrArrayRefs   *subrefs = ref->get_subrefs();
        Common::Assignment *ass = ref->get_refd_assignment();
        if (!ass) FATAL_ERROR("Could not grab ass!");

        str_set_type = mputstrn(str_set_type, " + ", 3);
        
        Ref_pard* ref_pard = dynamic_cast<Ref_pard*>(ref);
        if (ref_pard) {
          // in case of parametrised references:
          //  - temporary parameters need to be declared (stored in str_preamble)
          //  - the same temporary needs to be used at each call (generate_code_cached call)
          expression_struct expr;
          Code::init_expr(&expr);

          ref_pard->generate_code_cached(&expr);
          str_set_type = mputprintf(str_set_type, "%s", expr.expr);
          if (expr.preamble)
            str_preamble = mputstr(str_preamble, expr.preamble);

          Code::free_expr(&expr);
        }
        else {
          str_set_type = mputstr (str_set_type, ass->get_id().get_name().c_str());
          if (subrefs) {
            expression_struct expr;
            Code::init_expr(&expr);

            subrefs->generate_code(&expr, ass);
            str_set_type = mputprintf(str_set_type, "%s", expr.expr);

            Code::free_expr(&expr);
          }
        }

        switch(ass->get_asstype()) {
        case Common::Assignment::A_CONST:
        case Common::Assignment::A_EXT_CONST:
        case Common::Assignment::A_MODULEPAR:
        case Common::Assignment::A_VAR:
        case Common::Assignment::A_PAR_VAL_IN:
        case Common::Assignment::A_PAR_VAL_OUT:
        case Common::Assignment::A_PAR_VAL_INOUT:
        case Common::Assignment::A_FUNCTION_RVAL:
        case Common::Assignment::A_EXT_FUNCTION_RVAL:
          if (ass->get_Type()->field_is_optional(subrefs)) {
            str_set_type = mputstrn(str_set_type, "()", 2);
          }
          break;
        default:
          break;
        }
        
        str_set_type = mputstr(str_set_type, ".n_elem()");
      }
      
      str = mputstr(str, str_preamble);
      str = mputstr(str, str_set_type);
      
      Free(str_preamble);
      Free(str_set_type);
      
      str = mputstrn(str, ");\n", 3);

      string shifty;
      for (size_t i = 0; i < nof_ts; i++) {
        Template *t = u.templates->get_t_byIndex(i);
        switch (t->templatetype) {
        case ALL_FROM: {
          expression_struct expr; // FIXME copypasta from init_list above !
          Code::init_expr(&expr);
          // variable one
          switch (t->u.all_from->templatetype) {
          case SPECIFIC_VALUE: {
            Value *sv = t->u.all_from->u.specific_value;
            switch (sv->get_valuetype()) {
            case Value::V_REFD: {
              Common::Reference *ref = sv->get_reference();
              Ref_pard* ref_pard = dynamic_cast<Ref_pard*>(ref);
              if (ref_pard)
                ref_pard->generate_code_cached(&expr);
              else
                ref->generate_code(&expr);

              Common::Assignment* ass = ref->get_refd_assignment();
              switch(ass->get_asstype()) {
              case Common::Assignment::A_CONST:
              case Common::Assignment::A_EXT_CONST:
              case Common::Assignment::A_MODULEPAR:
              case Common::Assignment::A_VAR:
              case Common::Assignment::A_PAR_VAL_IN:
              case Common::Assignment::A_PAR_VAL_OUT:
              case Common::Assignment::A_PAR_VAL_INOUT:
              case Common::Assignment::A_FUNCTION_RVAL:
              case Common::Assignment::A_EXT_FUNCTION_RVAL:
                if (ass->get_Type()->field_is_optional(ref->get_subrefs())) {
                  expr.expr = mputstrn(expr.expr, "()", 2);
                }
                break;
              default:
                break;
              }

              break; }

            default:
              FATAL_ERROR("VT NOT HANDLED");
            } // switch valuetype
            break; }

          default:
            FATAL_ERROR("ttype not handled");
            break;
          }

          str = mputprintf(str,
            "for (int i_i = 0, i_lim = %s.n_elem(); i_i < i_lim; ++i_i) {\n",
            expr.expr);

          // Name for the LHS
          char *embedded_name = mprintf("%s.set_item(%lu%s + i_i)", name,
            (unsigned long) i, shifty.c_str());
          str = t->generate_code_init_all_from(str, embedded_name);
          Free(embedded_name);

          str = mputstrn(str, "}\n", 2);

          shifty += "-1+";
          shifty += expr.expr;
          shifty += ".n_elem() /* 3442 */";
          Code::free_expr(&expr);
          break; }

        default:
          if (t->needs_temp_ref()) {
            const string& tmp_id = get_temporary_id();
            const char *tmp_id_str = tmp_id.c_str();
            str = mputprintf(str, "{\n"
              "%s& %s = %s.set_item(%lu%s);\n",
              oftype_name_str, tmp_id_str, name, (unsigned long) i, shifty.c_str());
            str = t->generate_code_init(str, tmp_id_str);
            str = mputstr(str, "}\n");
          } else {
            char *embedded_name = mprintf("%s.set_item(%lu%s)", name,
              (unsigned long) i, shifty.c_str());
            str = t->generate_code_init(str, embedded_name);
            Free(embedded_name);
          }
          break;
        } // switch t->templatetype
      }

    }
    else {
      str = mputprintf(str, "%s.set_type(%s, %lu);\n", name,
        is_superset ? "SUPERSET_MATCH" : "SUBSET_MATCH", (unsigned long) nof_ts);
      for (size_t i = 0; i < nof_ts; i++) {
        Template *t = u.templates->get_t_byIndex(i);
        if (t->needs_temp_ref()) {
          const string& tmp_id = get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          str = mputprintf(str, "{\n"
            "%s& %s = %s.set_item(%lu);\n",
            oftype_name_str, tmp_id_str, name, (unsigned long) i);
          str = t->generate_code_init(str, tmp_id_str);
          str = mputstr(str, "}\n");
        } else {
          char *embedded_name = mprintf("%s.set_item(%lu)", name,
            (unsigned long) i);
          str = t->generate_code_init(str, embedded_name);
          Free(embedded_name);
        }
      }
    }
    return str;
  }
  
  char* Template::generate_code_init_dec_match(char* str, const char* name)
  {
    // generate a new class for this decmatch template
    string class_tmp_id = my_scope->get_scope_mod_gen()->get_temporary_id();
    Type* target_type = u.dec_match.target->get_expr_governor(
      Type::EXPECTED_TEMPLATE)->get_type_refd_last();
    // use the name of the type at the end of the reference chain for logging
    string type_name;
    const char* type_name_ptr = target_type->get_typename_builtin(
      target_type->get_typetype_ttcn3());
    if (type_name_ptr == NULL) {
      type_name = target_type->get_type_refd_last()->get_dispname();
    }
    else {
      type_name = type_name_ptr;
    }
    str = mputprintf(str,
      "class Dec_Match_%s : public Dec_Match_Interface {\n"
      // store the decoding target as a member, since 2 functions use it
      "%s target;\n"
      // store the decoding result from the last successful match() call
      "%s* dec_val;\n"
      "public:\n"
      "Dec_Match_%s(%s p_target): target(p_target), dec_val(NULL) { }\n"
      "~Dec_Match_%s() { if (dec_val != NULL) delete dec_val; }\n"
      // called when matching, the buffer parameter contains the string to be matched
      "virtual boolean match(TTCN_Buffer& buff)\n"
      "{\n"
      "if (dec_val != NULL) delete dec_val;\n"
      "dec_val = new %s;\n"
      "boolean ret_val;\n", class_tmp_id.c_str(),
      target_type->get_genname_template(my_scope).c_str(),
      target_type->get_genname_value(my_scope).c_str(), class_tmp_id.c_str(),
      target_type->get_genname_template(my_scope).c_str(), class_tmp_id.c_str(),
      target_type->get_genname_value(my_scope).c_str());
    bool dec_by_func = target_type->is_coding_by_function(false);
    if (dec_by_func) {
      str = mputprintf(str,
        // convert the TTCN_Buffer into a bitstring
        "OCTETSTRING os;\n"
        "buff.get_string(os);\n"
        "BITSTRING bs(oct2bit(os));\n"
        // decode the value (with the user function)
        "if (%s(bs, *dec_val) != 0) {\n"
        "TTCN_warning(\"Decoded content matching failed, because the data could "
        "not be decoded by the provided function.\");\n"
        "ret_val = FALSE;\n"
        "}\n"
        // make sure the bitstring is empty after decoding, display a warning otherwise
        "else if (bs.lengthof() != 0) {\n",
        target_type->get_coding_function(false)->get_genname_from_scope(my_scope).c_str());
    }
    else {
      str = mputprintf(str,
        // decode the value (with a built-in decoder)
        "dec_val->decode(%s_descr_, buff, TTCN_EncDec::CT_%s);\n"
        // make sure no errors occurred (these already displayed warnings during
        // decoding)
        "if (TTCN_EncDec::get_last_error_type() != TTCN_EncDec::ET_NONE) "
        "ret_val = FALSE;\n"
        // make sure the buffer is empty after decoding, display a warning otherwise
        "else if (buff.get_read_len() != 0) {\n",
        target_type->get_genname_typedescriptor(my_scope).c_str(),
        target_type->get_coding(false).c_str());
    }
    str = mputprintf(str,
      "TTCN_warning(\"Decoded content matching failed, because the buffer was not "
      "empty after decoding. Remaining %s: %%d.\", %s);\n"
      "ret_val = FALSE;\n"
      "}\n"
      // finally, match the decoded value against the target template
      "else ret_val = target.match(*dec_val%s);\n"
      // delete the decoded value if matching was not successful
      "if (!ret_val) {\n"
      "delete dec_val;\n"
      "dec_val = NULL;\n"
      "}\n"
      "return ret_val;\n"
      "}\n"
      "virtual void log() const\n"
      "{\n"
      // the decoding target is always logged as an in-line template
      "TTCN_Logger::log_event_str(\"%s: \");\n"
      "target.log();\n"
      "}\n"
      // retrieves the decoding result from the last successful matching
      // (used for optimizing decoded value and parameter redirects)
      "void* get_dec_res() const { return dec_val; }\n"
      // returns a pointer to the type descriptor used in the decoding
      // (used for the runtime type check for decoded value and parameter redirects)
      "const TTCN_Typedescriptor_t* get_type_descr() const { return &%s_descr_; }\n"
      "};\n"
      "%s.set_type(DECODE_MATCH);\n"
      "{\n", dec_by_func ? "bits" : "octets",
      dec_by_func ? "bs.lengthof()" : "(int)buff.get_read_len()",
      omit_in_value_list ? ", TRUE" : "", type_name.c_str(),
      target_type->get_genname_typedescriptor(my_scope).c_str(), name);
    
    // generate the decoding target into a temporary
    string target_tmp_id = my_scope->get_scope_mod_gen()->get_temporary_id();
    if (u.dec_match.target->get_DerivedRef() != NULL) {
      // it's a derived reference: initialize the decoding target with the
      // base template first
      expression_struct ref_expr;
      Code::init_expr(&ref_expr);
      u.dec_match.target->get_DerivedRef()->generate_code(&ref_expr);
      if (ref_expr.preamble != NULL) {
        str = mputstr(str, ref_expr.preamble);
      }
      str = mputprintf(str, "%s %s(%s);\n",
        target_type->get_genname_template(my_scope).c_str(),
        target_tmp_id.c_str(), ref_expr.expr);
      if (ref_expr.postamble != NULL) {
        str = mputstr(str, ref_expr.postamble);
      }
      Code::free_expr(&ref_expr);
    }
    else {
      str = mputprintf(str, "%s %s;\n",
        target_type->get_genname_template(my_scope).c_str(),
        target_tmp_id.c_str());
    }
    str = u.dec_match.target->get_Template()->generate_code_init(str,
      target_tmp_id.c_str());
    
    // the string encoding format might be an expression, generate its preamble here
    expression_struct coding_expr;
    Code::init_expr(&coding_expr);
    if (u.dec_match.str_enc != NULL) {
      u.dec_match.str_enc->generate_code_expr(&coding_expr);
      if (coding_expr.preamble != NULL) {
        str = mputstr(str, coding_expr.preamble);
      }
    }
    // initialize the decmatch template with an instance of the new class
    // (pass the temporary template to the new instance's constructor) and
    // the encoding format if it's an universal charstring
    str = mputprintf(str,
      "%s.set_decmatch(new Dec_Match_%s(%s)%s%s);\n",
      name, class_tmp_id.c_str(), target_tmp_id.c_str(),
      (coding_expr.expr != NULL) ? ", " : "",
      (coding_expr.expr != NULL) ? coding_expr.expr : "");
    if (coding_expr.postamble != NULL) {
      str = mputstr(str, coding_expr.postamble);
    }
    Code::free_expr(&coding_expr);
    str = mputstr(str, "}\n");
    return str;
  }
  
  char* Template::generate_code_init_concat(char* str, const char* name)
  {
    string left_expr;
    string right_expr;
    if (u.concat.op1->has_single_expr()) {
      left_expr = u.concat.op1->get_single_expr(false);
    }
    else {
      left_expr = my_scope->get_scope_mod_gen()->get_temporary_id();
      str = mputprintf(str, "%s %s;\n",
        my_governor->get_genname_template(my_scope).c_str(), left_expr.c_str());
      str = u.concat.op1->generate_code_init(str, left_expr.c_str());
    }
    if (u.concat.op2->has_single_expr()) {
      right_expr = u.concat.op2->get_single_expr(false);
    }
    else {
      right_expr = my_scope->get_scope_mod_gen()->get_temporary_id();
      str = mputprintf(str, "%s %s;\n",
        my_governor->get_genname_template(my_scope).c_str(), right_expr.c_str());
      str = u.concat.op2->generate_code_init(str, right_expr.c_str());
    }
      
    str = mputprintf(str, "%s = %s + %s;\n", name, left_expr.c_str(),
      right_expr.c_str());
    return str;
  }

  void Template::generate_code_expr_invoke(expression_struct *expr)
  {
    Value *last_v = u.invoke.v->get_value_refd_last();
    if (last_v->get_valuetype() == Value::V_FUNCTION) {
      Common::Assignment *function = last_v->get_refd_fat();
      expr->expr = mputprintf(expr->expr, "%s(",
        function->get_genname_from_scope(my_scope).c_str());
      u.invoke.ap_list->generate_code_alias(expr,
        function->get_FormalParList(), function->get_RunsOnType(), false);
    } else {
      u.invoke.v->generate_code_expr_mandatory(expr);
      expr->expr = mputstr(expr->expr, ".invoke(");
      Type* gov_last = u.invoke.v->get_expr_governor_last();
      u.invoke.ap_list->generate_code_alias(expr, 0,
        gov_last->get_fat_runs_on_type(), gov_last->get_fat_runs_on_self());
    }
    expr->expr = mputc(expr->expr, ')');
  }

  char *Template::rearrange_init_code_refd(char *str, Common::Module* usage_mod)
  {
    if (templatetype != TEMPLATE_REFD)
      FATAL_ERROR("Template::rearrange_init_code_refd()");
    ActualParList *actual_parlist = u.ref.ref->get_parlist();
    // generate code for the templates that are used in the actual parameter
    // list of the reference
    Common::Assignment *ass = u.ref.ref->get_refd_assignment();
    if (actual_parlist) str = actual_parlist->rearrange_init_code(str, usage_mod);
    // do nothing if the reference does not point to a template definition
    if (ass->get_asstype() != Common::Assignment::A_TEMPLATE) return str;
    Template *t = ass->get_Template();
    FormalParList *formal_parlist = ass->get_FormalParList();
    if (formal_parlist) {
      // the reference points to a parameterized template
      // we must perform the rearrangement for all non-parameterized templates
      // that are referred by the parameterized template regardless of the
      // sub-references of u.ref.ref
      str = t->rearrange_init_code(str, usage_mod);
      // the parameterized template's default values must also be generated
      // (this only generates their value assignments, their declarations will
      // be generated when the template's definition is reached)
      if (ass->get_my_scope()->get_scope_mod_gen() == usage_mod) {
        str = formal_parlist->generate_code_defval(str);
      }
    } else {
      // the reference points to a non-parameterized template
      FieldOrArrayRefs *subrefs = u.ref.ref->get_subrefs();
      if (subrefs) {
        // we should follow the sub-references as much as we can
        // and perform the rearrangement for the referred field only
        for (size_t i = 0; i < subrefs->get_nof_refs(); i++) {
          FieldOrArrayRef *ref = subrefs->get_ref(i);
          if (ref->get_type() == FieldOrArrayRef::FIELD_REF) {
            // stop if the body does not have fields
            if (t->templatetype != NAMED_TEMPLATE_LIST) break;
            // get the sub-template of the referred field
            t = t->u.named_templates->get_nt_byName(*ref->get_id())
              ->get_template();
          } else {
            // stop if the body is not a list
            if (t->templatetype != TEMPLATE_LIST) break;
            Value *array_index = ref->get_val()->get_value_refd_last();
            // stop if the index cannot be evaluated at compile time
            if (array_index->get_valuetype() != Value::V_INT) break;
            Int index = array_index->get_val_Int()->get_val();
            // index transformation in case of arrays
            if (t->my_governor->get_typetype() == Type::T_ARRAY)
              index -= t->my_governor->get_dimension()->get_offset();
            // get the template with the given index
            t = t->get_listitem_byIndex((size_t)index);
          }
        }
      }
      // otherwise if the reference points to a top-level template
      // we should initialize its entire body
      if (ass->get_my_scope()->get_scope_mod_gen() == usage_mod) {
        str = t->generate_code_init(str, t->get_lhs_name().c_str());
      }
    }
    return str;
  }

  char *Template::rearrange_init_code_invoke(char *str, Common::Module* usage_mod)
  {
    str = u.invoke.v->rearrange_init_code(str, usage_mod);
    str = u.invoke.ap_list->rearrange_init_code(str, usage_mod);
    return str;
  }

  bool Template::needs_temp_ref()
  {
    if (length_restriction || is_ifpresent) return true;
    switch (templatetype) {
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
    case SPECIFIC_VALUE:
    case TEMPLATE_REFD:
    case TEMPLATE_INVOKE:
    case BSTR_PATTERN:
    case HSTR_PATTERN:
    case OSTR_PATTERN:
    case CSTR_PATTERN:
    case USTR_PATTERN:
    case TEMPLATE_NOTUSED:
      return false;
    case TEMPLATE_LIST:
      // temporary reference is needed if the template has at least one
      // element (excluding not used symbols)
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++) {
        if (u.templates->get_t_byIndex(i)->templatetype != TEMPLATE_NOTUSED)
          return true;
      }
      return false;
    case INDEXED_TEMPLATE_LIST:
      for (size_t i = 0; i < u.indexed_templates->get_nof_its(); i++) {
        if (u.indexed_templates->get_it_byIndex(i)->get_template()
          ->templatetype != TEMPLATE_NOTUSED)
          return true;
      }
      return false;
    case NAMED_TEMPLATE_LIST:
      return u.named_templates->get_nof_nts() > 1;
    case ALL_FROM:
      return false;
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case VALUE_RANGE:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case DECODE_MATCH:
      return true;
    case TEMPLATE_ERROR:
      FATAL_ERROR("Template::needs_temp_ref()");
    case PERMUTATION_MATCH:
      // FIXME
      return false;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::needs_temp_ref()");
      }
      return u.concat.op1->needs_temp_ref() || u.concat.op2->needs_temp_ref();
    }
    return false;
  }

  bool Template::has_single_expr()
  {
    if (length_restriction || is_ifpresent || get_needs_conversion())
      return false;
    switch (templatetype) {
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
    case BSTR_PATTERN:
    case HSTR_PATTERN:
    case OSTR_PATTERN:
    case TEMPLATE_NOTUSED:
      return true;
    case CSTR_PATTERN: // Some preamble needed when \N{ref} used
    case USTR_PATTERN:
      return false;
    case SPECIFIC_VALUE:
      return u.specific_value->has_single_expr();
    case TEMPLATE_REFD: {
      Template *t_last = get_template_refd_last();
      if (t_last != this && t_last->has_single_expr()) {
        for (Scope *t_scope = my_scope; t_scope;
             t_scope = t_scope->get_parent_scope()) {
          // return true if t_last is visible from my scope
          if (t_scope == t_last->my_scope) return true;
        }
      }
      // otherwise consider the reference itself
      return u.ref.ref->has_single_expr(); }
    case TEMPLATE_INVOKE:
      if (!u.invoke.v->has_single_expr()) return false;
      for (size_t i = 0; i < u.invoke.ap_list->get_nof_pars(); i++)
        if (!u.invoke.ap_list->get_par(i)->has_single_expr()) return false;
      return true;
    case TEMPLATE_LIST:
      return u.templates->get_nof_ts() == 0;
    case NAMED_TEMPLATE_LIST: {
      if (!my_governor) FATAL_ERROR("Template::has_single_expr()");
      Type *type = my_governor->get_type_refd_last();
      return type->get_nof_comps() == 0; }
    case INDEXED_TEMPLATE_LIST:
      return u.indexed_templates->get_nof_its() == 0;
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case VALUE_RANGE:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
    case DECODE_MATCH:
      return false;
    case ALL_FROM:
      return false;
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::has_single_expr()");
      }
      return u.concat.op1->has_single_expr() && u.concat.op2->has_single_expr();
    default:
      FATAL_ERROR("Template::has_single_expr()");
      return false;
    }
  }

  
  string Template::generate_code_str_pattern(bool cast_needed, string& preamble) {
    if (cast_needed && (length_restriction || is_ifpresent))
      FATAL_ERROR("Template::generate_code_str_pattern()");
    string ret_val;
    switch(templatetype) {
      case CSTR_PATTERN:
      case USTR_PATTERN:
        ret_val = u.pstring
        ->create_charstring_literals(get_my_scope()->get_scope_mod_gen(), preamble);
        break;
      default:
        FATAL_ERROR("Template::generate_code_str_pattern()");
    }
    if (cast_needed) ret_val = my_governor->get_genname_template(my_scope) +
      "(" + ret_val + ")";
    return ret_val;
  }
  
  string Template::get_single_expr(bool cast_needed)
  {
    if (cast_needed && (length_restriction || is_ifpresent))
      FATAL_ERROR("Template::get_single_expr()");
    string ret_val;
    switch (templatetype) {
    case OMIT_VALUE:
      ret_val = "OMIT_VALUE";
      break;
    case ANY_VALUE:
      ret_val = "ANY_VALUE";
      break;
    case ANY_OR_OMIT:
      ret_val = "ANY_OR_OMIT";
      break;
    case SPECIFIC_VALUE:
      ret_val = u.specific_value->get_single_expr();
      break;
    case TEMPLATE_REFD: {
      // convert the reference to a single expression
      expression_struct expr;
      Code::init_expr(&expr);
      u.ref.ref->generate_code(&expr);
      if (expr.preamble || expr.postamble)
        FATAL_ERROR("Template::get_single_expr()");
      ret_val = expr.expr;
      Code::free_expr(&expr);
      return ret_val;
      }
    case TEMPLATE_INVOKE: {
      expression_struct expr;
      Code::init_expr(&expr);
      generate_code_expr_invoke(&expr);
      if (expr.preamble || expr.postamble)
        FATAL_ERROR("Template::get_single_expr()");
      ret_val = expr.expr;
      Code::free_expr(&expr);
      return ret_val; }
    case TEMPLATE_LIST:
      if (u.templates->get_nof_ts() != 0)
        FATAL_ERROR("Template::get_single_expr()");
      ret_val = "NULL_VALUE";
      break;
    case NAMED_TEMPLATE_LIST:
      if (u.named_templates->get_nof_nts() != 0)
        FATAL_ERROR("Template::get_single_expr()");
      ret_val = "NULL_VALUE";
      break;
    case INDEXED_TEMPLATE_LIST:
      if (u.indexed_templates->get_nof_its() != 0)
        FATAL_ERROR("Template::get_single_expr()");
      ret_val = "NULL_VALUE";
      break;
    case BSTR_PATTERN:
      return get_my_scope()->get_scope_mod_gen()
        ->add_bitstring_pattern(*u.pattern);
    case HSTR_PATTERN:
      return get_my_scope()->get_scope_mod_gen()
        ->add_hexstring_pattern(*u.pattern);
    case OSTR_PATTERN:
      return get_my_scope()->get_scope_mod_gen()
        ->add_octetstring_pattern(*u.pattern);
    case TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("Template::get_single_expr()");
      }
      ret_val = u.concat.op1->get_single_expr(false) + " + " +
        u.concat.op2->get_single_expr(false);
      break;
    default:
      FATAL_ERROR("Template::get_single_expr()");
    }
    if (cast_needed) ret_val = my_governor->get_genname_template(my_scope) +
      "(" + ret_val + ")";
    return ret_val;
  }

  void Template::dump(unsigned level) const
  {
    DEBUG(level, "%s", get_templatetype_str());
    switch (templatetype) {
    case TEMPLATE_ERROR:
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
      break;
    case SPECIFIC_VALUE:
      u.specific_value->dump(level+1);
      break;
    case TEMPLATE_REFD:
      u.ref.ref->dump(level+1);
      break;
    case TEMPLATE_INVOKE:
      u.invoke.v->dump(level+1);
      if (u.invoke.ap_list) u.invoke.ap_list->dump(level + 1);
      else if (u.invoke.t_list) u.invoke.t_list->dump(level + 1);
      break;
    case TEMPLATE_LIST:
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
    case SUPERSET_MATCH:
    case SUBSET_MATCH:
    case PERMUTATION_MATCH:
      for (size_t i = 0; i < u.templates->get_nof_ts(); i++)
        u.templates->get_t_byIndex(i)->dump(level+1);
      break;
    case NAMED_TEMPLATE_LIST:
      for (size_t i = 0; i < u.named_templates->get_nof_nts(); i++)
        u.named_templates->get_nt_byIndex(i)->dump(level+1);
      break;
    case INDEXED_TEMPLATE_LIST:
      for (size_t i = 0; i < u.indexed_templates->get_nof_its(); i++)
        u.indexed_templates->get_it_byIndex(i)->dump(level+1);
      break;
    case VALUE_RANGE:
      u.value_range->dump(level);
      break;
    case BSTR_PATTERN:
    case HSTR_PATTERN:
    case OSTR_PATTERN:
      DEBUG(level+1, "%s", u.pattern->c_str());
      break;
    case CSTR_PATTERN:
    case USTR_PATTERN:
      u.pstring->dump(level+1);
      break;
    case ALL_FROM:
      u.all_from->dump(level+1);
      break;
    case DECODE_MATCH:
      if (u.dec_match.str_enc != NULL) {
        DEBUG(level, "string encoding:");
        u.dec_match.str_enc->dump(level + 1);
      }
      DEBUG(level, "decoding target:");
      u.dec_match.target->dump(level + 1);
      break;
    case TEMPLATE_CONCAT:
      DEBUG(level, "operand #1:");
      u.concat.op1->dump(level + 1);
      DEBUG(level, "operand #2:");
      u.concat.op2->dump(level + 1);
      break;
    default:
      break;
    }
    if (length_restriction) length_restriction->dump(level + 1);
  }

  bool Template::has_allfrom() const
  { // the code generation of all from is not fully implemented. This helps avoid of using it.
    if (templatetype != TEMPLATE_LIST) FATAL_ERROR("has_allfrom(): Templatetype shall be TEMPLATE_LIST");
    size_t nof_ts = u.templates->get_nof_ts();
    for (size_t i = 0; i < nof_ts; i++) {
      if (u.templates->get_t_byIndex(i)->templatetype == ALL_FROM) {
        return true;
      }
    }
    return false;
  }

  // =================================
  // ===== TemplateInstance
  // =================================

  TemplateInstance::TemplateInstance(const TemplateInstance& p)
    : Node(p), Location(p)
  {
    type = p.type ? p.type->clone() : 0;
    derived_reference = p.derived_reference ? p.derived_reference->clone() : 0;
    template_body = p.template_body->clone();
    last_gen_expr = p.last_gen_expr ? mcopystr(p.last_gen_expr) : NULL;
  }

  TemplateInstance::TemplateInstance(Type *p_type, Ref_base *p_ref,
    Template *p_body) : Node(), Location(), type(p_type),
    derived_reference(p_ref), template_body(p_body), last_gen_expr(NULL)
  {
    if (!p_body) FATAL_ERROR("TemplateInstance::TemplateInstance()");
    if (type) type->set_ownertype(Type::OT_TEMPLATE_INST, this);
  }

   TemplateInstance::~TemplateInstance()
   {
     delete type;
     delete derived_reference;
     delete template_body;
     Free(last_gen_expr);
   }

   void TemplateInstance::release()
   {
     type = 0;
     derived_reference = 0;
     template_body = 0;
   }

  TemplateInstance *TemplateInstance::clone() const
  {
    return new TemplateInstance(*this);
  }

  void TemplateInstance::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (type) type->set_fullname(p_fullname + ".<type>");
    if (derived_reference)
      derived_reference->set_fullname(p_fullname + ".<derived_reference>");
    template_body->set_fullname(p_fullname);
  }

  void TemplateInstance::set_my_scope(Scope *p_scope)
  {
    if (type) type->set_my_scope(p_scope);
    if (derived_reference) derived_reference->set_my_scope(p_scope);
    template_body->set_my_scope(p_scope);
  }

  Type::typetype_t TemplateInstance::get_expr_returntype
    (Type::expected_value_t exp_val)
  {
    Type *t = get_expr_governor(exp_val);
    if (t) return t->get_type_refd_last()->get_typetype_ttcn3();
    else return template_body->get_expr_returntype(exp_val);
  }

  Type *TemplateInstance::get_expr_governor(Type::expected_value_t exp_val)
  {
    if (type) return type;
    if (derived_reference) {
      Type *ret_val = chk_DerivedRef(0);
      if (ret_val) return ret_val;
    }
    return template_body->get_expr_governor(exp_val);
  }

  void TemplateInstance::chk(Type *governor)
  {
    if (!governor) FATAL_ERROR("TemplateInstance::chk()");
    governor = chk_Type(governor);
    governor = chk_DerivedRef(governor);
    template_body->set_my_governor(governor);
    governor->chk_this_template_ref(template_body);
    governor->chk_this_template_generic(template_body,
      (derived_reference != 0 ? INCOMPLETE_ALLOWED : INCOMPLETE_NOT_ALLOWED),
      OMIT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK, NOT_IMPLICIT_OMIT, 0);
  }

  Type *TemplateInstance::chk_Type(Type *governor)
  {
    if (!type) return governor;
    {
      Error_Context cntxt(type, "In explicit type specification");
      type->chk();
    }
    TypeCompatInfo info(template_body->get_template_refd_last()
                        ->get_my_scope()->get_scope_mod(), governor, type,
                        true, false);
    TypeChain l_chain;
    TypeChain r_chain;
    if (!governor) return type;
    else if (governor->is_compatible(type, &info, NULL, &l_chain, &r_chain, true)) {
      return governor;
    } else {
      if (info.is_subtype_error()) {
        type->error("%s", info.get_subtype_error().c_str());
      } else
      if (!info.is_erroneous()) {
        type->error("Incompatible explicit type specification: `%s' was "
                    "expected instead of `%s'",
                    governor->get_typename().c_str(),
                    type->get_typename().c_str());
      } else {
        type->error("%s", info.get_error_str_str().c_str());
      }
      return type;
    }
  }

  Type *TemplateInstance::chk_DerivedRef(Type *governor)
  {
    if (!derived_reference) return governor;
    Error_Context cntxt(derived_reference, "In derived reference");
    Common::Assignment *ass = derived_reference->get_refd_assignment();
    // the base template reference must not have sub-references
    if (derived_reference->get_subrefs())
      FATAL_ERROR("TemplateInstance::chk_DerivedRef()");
    bool set_bt = false;
    if (!ass) goto error;
    switch (ass->get_asstype()) {
    case Common::Assignment::A_TEMPLATE:
      set_bt = true;
      // no break
    case Common::Assignment::A_MODULEPAR_TEMP:
    case Common::Assignment::A_VAR_TEMPLATE:
    case Common::Assignment::A_PAR_TEMPL_IN:
    case Common::Assignment::A_PAR_TEMPL_OUT:
    case Common::Assignment::A_PAR_TEMPL_INOUT:
    case Common::Assignment::A_FUNCTION_RTEMP:
    case Common::Assignment::A_EXT_FUNCTION_RTEMP: {
      if (!governor) governor = type;
      Type *base_template_type = ass->get_Type();
      if (governor) {
        TypeCompatInfo info(template_body->get_template_refd_last()
                            ->get_my_scope()->get_scope_mod(), governor, type,
                            true, false);
        TypeChain l_chain;
        TypeChain r_chain;
        if (!governor->is_compatible(base_template_type, &info, this, &l_chain,
                                     &r_chain)) {
          if (info.is_subtype_error()) {
            derived_reference->error("%s", info.get_subtype_error().c_str());
          } else
          if (!info.is_erroneous()) {
            derived_reference->error("Base template `%s' has incompatible "
                                     "type: `%s' was expected instead of `%s'",
                                     ass->get_fullname().c_str(),
                                     governor->get_typename().c_str(),
                                     base_template_type->get_typename().c_str());
          } else {
            derived_reference->error("%s", info.get_error_str_str().c_str());
          }
          // if explicit type specification is omitted
          // check the template body against the type of the base template
          if (!type) governor = base_template_type;
          set_bt = false;
        } else {
          if (info.needs_conversion())
            template_body->set_needs_conversion();
        }
      } else governor = base_template_type;
      break; }
    default:
      derived_reference->error("Reference to a template was expected instead "
                               "of %s", ass->get_description().c_str());
      goto error;
    }
    if (set_bt) template_body->set_base_template(ass->get_Template());
    return governor;
  error:
    // drop the erroneous derived_reference to avoid further errors
    delete derived_reference;
    derived_reference = 0;
    return governor;
  }

  void TemplateInstance::chk_recursions(ReferenceChain& refch)
  {
    template_body->chk_recursions(refch);
  }

  bool TemplateInstance::is_string_type(Type::expected_value_t exp_val)
  {
    switch (get_expr_returntype(exp_val)) {
    case Type::T_CSTR:
    case Type::T_USTR:
    case Type::T_BSTR:
    case Type::T_HSTR:
    case Type::T_OSTR:
      return true;
    default:
      return false;
    }
  }

  bool TemplateInstance::chk_restriction(const char* definition_name,
    template_restriction_t template_restriction, const Location* usage_loc)
  {
    bool needs_runtime_check = false;
    if (derived_reference) // if modified
    {
      Common::Assignment *ass = derived_reference->get_refd_assignment();
      switch (ass->get_asstype()) {
      case Common::Assignment::A_TEMPLATE:
        // already added to template_body as base template by chk_DerivedRef()
        needs_runtime_check = template_body->chk_restriction(
          definition_name, template_restriction, usage_loc);
        break;
      case Common::Assignment::A_MODULEPAR_TEMP:
      case Common::Assignment::A_VAR_TEMPLATE:
      case Common::Assignment::A_EXT_FUNCTION_RTEMP:
      case Common::Assignment::A_FUNCTION_RTEMP:
      case Common::Assignment::A_PAR_TEMPL_IN:
      case Common::Assignment::A_PAR_TEMPL_OUT:
      case Common::Assignment::A_PAR_TEMPL_INOUT: {
        // create a temporary Template to be added as the base template and
        // check, remove and delete after checked
        if (template_body->get_base_template())
          FATAL_ERROR("TemplateInstance::chk_restriction()");
        template_body->set_base_template(new Template(derived_reference));
        needs_runtime_check = template_body->chk_restriction(
          definition_name, template_restriction, usage_loc);
        delete template_body->get_base_template();
        template_body->set_base_template(0);
      } break;
      default:
        break;
      }
    } else { // simple
      needs_runtime_check = template_body->chk_restriction(definition_name,
                              template_restriction, usage_loc);
    }
    return needs_runtime_check;
  }

  bool TemplateInstance::has_single_expr()
  {
    if (derived_reference) return false;
    else if (type) return false;
    else return template_body->has_single_expr();
  }

  void TemplateInstance::set_code_section(
    GovernedSimple::code_section_t p_code_section)
  {
    if (derived_reference) derived_reference->set_code_section(p_code_section);
    template_body->set_code_section(p_code_section);
  }

  bool TemplateInstance::needs_temp_ref()
  {
    if (template_body->get_templatetype() != Template::SPECIFIC_VALUE)
      return false;
    Value *val = template_body->get_specific_value();
    if (val->get_valuetype() == Value::V_REFD) {
      FieldOrArrayRefs *refs = val->get_reference()->get_subrefs();
      if (!refs) return false;
      // We need at least a 2-long reference chain.  E.g. "a[0].b".  3.0.4
      // can't handle a normal reference following an indexed reference.  The
      // indexed reference must be on the first place.  Code like: "a.b[0].c"
      // compiles fine.
      if (refs->get_nof_refs() < 2
          || refs->get_ref(0)->get_type() != FieldOrArrayRef::ARRAY_REF)
        return false;
    } else {
      return false;
    }
    return true;
  }

  void TemplateInstance::generate_code(expression_struct *expr,
    template_restriction_t template_restriction, bool has_decoded_redirect)
  {
    size_t start_pos = mstrlen(expr->expr);
    if (derived_reference) {
      // preserve the target expression
      char *expr_backup = expr->expr;
      // reset the space for the target expression
      expr->expr = NULL;
      derived_reference->generate_code(expr);
      // now the C++ equivalent of the base template reference is in expr->expr
      const string& tmp_id = template_body->get_temporary_id();
      const char *tmp_id_str = tmp_id.c_str();
      // create a temporary variable and copy the contents of base template
      // into it
      expr->preamble = mputprintf(expr->preamble, "%s %s(%s);\n",
        template_body->get_my_governor()->get_genname_template(
        template_body->get_my_scope()).c_str(), tmp_id_str, expr->expr);
      // perform the modifications on the temporary variable
      expr->preamble = template_body->generate_code_init(expr->preamble,
        tmp_id_str);
      // runtime template restriction check
      if (template_restriction!=TR_NONE)
        expr->preamble = Template::generate_restriction_check_code(
          expr->preamble, tmp_id_str, template_restriction);
      // the base template reference is no longer needed
      Free(expr->expr);
      // restore the target expression append the name of the temporary
      // variable to it
      expr->expr = mputstr(expr_backup, tmp_id_str);
    } else {
      char *expr_backup;
      if (has_decoded_redirect) {
        // preserve the target expression
        expr_backup = expr->expr;
        // reset the space for the target expression
        expr->expr = NULL;
      }
      template_body->generate_code_expr(expr, template_restriction);
      if (has_decoded_redirect) {
        // create a temporary variable and move the template's initialization code
        // after it
        const string& tmp_id = template_body->get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        expr->preamble = mputprintf(expr->preamble, "%s %s = %s;\n",
          template_body->get_my_governor()->get_genname_template(
          template_body->get_my_scope()).c_str(), tmp_id_str, expr->expr);
        Free(expr->expr);
        // restore the target expression and append the name of the temporary
        // variable to it
        expr->expr = mputstr(expr_backup, tmp_id_str);
      }
    }
    size_t end_pos = mstrlen(expr->expr);
    Free(last_gen_expr);
    last_gen_expr = mcopystrn(expr->expr + start_pos, end_pos - start_pos);
  }

  char *TemplateInstance::rearrange_init_code(char *str, Common::Module* usage_mod)
  {
    if (derived_reference) {
      ActualParList *actual_parlist = derived_reference->get_parlist();
      Common::Assignment *ass = derived_reference->get_refd_assignment();
      if (!ass) FATAL_ERROR("TemplateInstance::rearrange_init_code()");
      if (actual_parlist) str = actual_parlist->rearrange_init_code(str, usage_mod);
      if (ass->get_asstype() == Common::Assignment::A_TEMPLATE) {
        Template *t = ass->get_Template();
        FormalParList *formal_parlist = ass->get_FormalParList();
        if (formal_parlist) {
          // the referred template is parameterized
          // the embedded referenced templates shall be visited
          str = t->rearrange_init_code(str, usage_mod);
          // the constants used for default values have to be initialized now
          if (ass->get_my_scope()->get_scope_mod_gen() == usage_mod) {
            str = formal_parlist->generate_code_defval(str);
          }
        } else {
          // the referred template is not parameterized
          // its entire body has to be initialized now
          if (ass->get_my_scope()->get_scope_mod_gen() == usage_mod) {
            str = t->generate_code_init(str, t->get_lhs_name().c_str());
          }
        }
      }
    }
    str = template_body->rearrange_init_code(str, usage_mod);
    return str;
  }

  void TemplateInstance::append_stringRepr(string& str) const
  {
    if (type) {
      str += type->get_typename();
      str += " : ";
    }
    if (derived_reference) {
      str += "modifies ";
      str += derived_reference->get_dispname();
      str += " := ";
    }
    str += template_body->get_stringRepr();
  }

  void TemplateInstance::dump(unsigned level) const
  {
    if (type) {
      DEBUG(level, "type:");
      type->dump(level + 1);
    }
    if (derived_reference) {
      DEBUG(level, "modifies:");
      derived_reference->dump(level + 1);
    }
    template_body->dump(level);
  }

  Value* TemplateInstance::get_specific_value() const
  {
    if (type) return NULL;
    if (derived_reference) return NULL;
    if (template_body->is_length_restricted() || template_body->get_ifpresent())
      return NULL;
    if (template_body->get_templatetype()!=Template::SPECIFIC_VALUE) return NULL;
    return template_body->get_specific_value();
  }

  Def_Template* TemplateInstance::get_Referenced_Base_Template()
  { // it may return 0
    if (!get_Template()) return NULL;
    Ttcn::Template::templatetype_t tpt =  get_Template()->get_templatetype();
    if (Ttcn::Template::TEMPLATE_REFD != tpt) return NULL;
    Ttcn::Ref_base* refbase = get_Template()->get_reference();
    Ttcn::Reference * ref = dynamic_cast<Ttcn::Reference*>(refbase);
    if (!ref) return NULL;
    Common::Assignment* ass = ref->get_refd_assignment();
    Ttcn::Definition* def = dynamic_cast<Ttcn::Definition*>(ass);
    if (!def) return NULL;
    Ttcn::Def_Template* deftemp = dynamic_cast<Ttcn::Def_Template*>(def);
    if (!deftemp) return NULL;

    return deftemp;
  }

}

