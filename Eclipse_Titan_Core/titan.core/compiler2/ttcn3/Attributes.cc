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
 *   Delic, Adam
 *   Feher, Csaba
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "Attributes.hh"
#include "../map.hh"
#include "../CompilerError.hh"
#include "../Type.hh"
#include "../main.hh"
#include "TtcnTemplate.hh"
#include "Statement.hh"

namespace Ttcn {

  // ==== Qualifier ====

  Qualifier* Qualifier::clone() const
  {
    return new Qualifier(*this);
  }

  const Identifier* Qualifier::get_identifier(size_t p_index) const
  {
    FieldOrArrayRef* ref = get_ref(p_index);
    return (ref->get_type()==FieldOrArrayRef::FIELD_REF) ?
           ref->get_id() :
           &underscore_zero ;
  }

  Qualifier* Qualifier::get_qualifier_without_first_id() const
  {
    Qualifier* temp = this->clone();
    temp->remove_refs(1);
    return temp;
  }

  string Qualifier::get_stringRepr() const
  {
    string str_repr;
    append_stringRepr(str_repr);
    if (str_repr[0]=='.') str_repr.replace(0, 1, ""); // remove leading dot if present
    return str_repr;
  }

  void Qualifier::dump(unsigned level) const
  {
    const string& rep = get_stringRepr();
    DEBUG(level, "(%s)", rep.c_str());
  }

  // ==== Qualifiers ====

  Qualifiers::~Qualifiers()
  {
    for(size_t i = 0; i < qualifiers.size(); i++)
    {
      delete qualifiers[i];
    }
    qualifiers.clear();
  }

  Qualifiers::Qualifiers(const Qualifiers& p)
    : Node(p)
  {
    for(size_t i = 0; i < p.get_nof_qualifiers(); i++)
    {
      qualifiers.add(p.get_qualifier(i)->clone());
    }
  }

  void Qualifiers::add_qualifier(Qualifier* p_qualifier)
  {
    if(p_qualifier->get_nof_identifiers() > 0)
      qualifiers.add(p_qualifier);
    else
      delete p_qualifier;
  }

  void Qualifiers::delete_qualifier(size_t p_index)
  {
    delete qualifiers[p_index];
    qualifiers.replace(p_index,1,NULL);
  }

  const Qualifier* Qualifiers::get_qualifier(size_t p_index) const
  {
    return qualifiers[p_index];
  }

  Qualifiers* Qualifiers::clone() const
  {
    return new Qualifiers(*this);
  }

  void Qualifiers::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for(size_t i = 0; i < qualifiers.size(); i++)
    {
      qualifiers[i]->set_fullname(p_fullname + ".<qualifier "
        + Int2string(i) + ">");
    }
  }

  bool Qualifiers::has_qualifier(Qualifier* p_qualifier) const
  {
    bool result = false;
    size_t index;
    bool same;
    for(size_t i = 0; i < qualifiers.size() && !result; i++)
    {
      if(qualifiers[i]->get_nof_identifiers()
          ==p_qualifier->get_nof_identifiers())
      {
        index = 0;
        same = true;
        while(index < p_qualifier->get_nof_identifiers() && same)
        {
          same = (*qualifiers[i]->get_identifier(index)
                == *p_qualifier->get_identifier(index));
          index++;
        }

        result |= same;
      }
    }
    return result;
  }

  void Qualifiers::dump(unsigned level) const
  {
    DEBUG(level, "has %lu qualifiers",
        static_cast<unsigned long>( get_nof_qualifiers() ));
    for(size_t i = 0; i < qualifiers.size(); i++)
      qualifiers[i]->dump(level);
  }

  // ==== ErroneousDescriptor ====

  ErroneousDescriptor::~ErroneousDescriptor()
  {
    for (size_t i=0; i<descr_m.size(); i++) delete descr_m.get_nth_elem(i);
    descr_m.clear();
    for (size_t i=0; i<values_m.size(); i++) delete values_m.get_nth_elem(i);
    values_m.clear();
  }

  // ==== ErroneousAttributeSpec ====

  ErroneousAttributeSpec::ErroneousAttributeSpec(bool p_is_raw, indicator_t p_indicator, TemplateInstance* p_tmpl_inst, bool p_has_all_keyword):
    is_raw(p_is_raw), has_all_keyword(p_has_all_keyword), indicator(p_indicator),
    tmpl_inst(p_tmpl_inst), type(0), value(0)
  {
    if (!tmpl_inst) FATAL_ERROR("ErroneousAttributeSpec::ErroneousAttributeSpec()");
  }

  ErroneousAttributeSpec::ErroneousAttributeSpec(const ErroneousAttributeSpec& p)
    : Node(p), Location(p), is_raw(p.is_raw), has_all_keyword(p.has_all_keyword),
    indicator(p.indicator), type(0), value(0)
  {
    tmpl_inst = p.tmpl_inst ? tmpl_inst->clone() : 0;
  }

  ErroneousAttributeSpec::~ErroneousAttributeSpec()
  {
    delete tmpl_inst;
    delete value;
  }

  ErroneousAttributeSpec* ErroneousAttributeSpec::clone() const
  {
    return new ErroneousAttributeSpec(*this);
  }

  void ErroneousAttributeSpec::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    tmpl_inst->set_fullname(p_fullname+".<template_instance>");
  }

  void ErroneousAttributeSpec::set_my_scope(Scope *p_scope)
  {
    tmpl_inst->set_my_scope(p_scope);
  }

  void ErroneousAttributeSpec::dump(unsigned level) const
  {
    DEBUG(level, "raw: %s", is_raw ? "yes" : "no");
    DEBUG(level, "indicator:");
    switch (indicator) {
    case I_BEFORE:
      DEBUG(level+1, "before");
      break;
    case I_VALUE:
      DEBUG(level+1, "value");
      break;
    case I_AFTER:
      DEBUG(level+1, "after");
      break;
    case I_INVALID:
      DEBUG(level+1, "<invalid>");
      break;
    default:
      FATAL_ERROR("ErroneousAttributeSpec::dump()");
    }
    DEBUG(level, "template instance:");
    tmpl_inst->dump(level+1);
  }

  bool ErroneousAttributeSpec::get_is_omit() const
  {
    return (tmpl_inst->get_Template()->get_templatetype()==Template::OMIT_VALUE);
  }

  void ErroneousAttributeSpec::chk(bool in_update_stmt)
  {
    if (get_is_omit()) { // special case, no type needed
      if ((indicator==I_BEFORE)||(indicator==I_AFTER)) {
        if (!has_all_keyword) {
          tmpl_inst->error(
            "Keyword `all' is expected after `omit' when omitting all fields %s the specified field",
            get_indicator_str(indicator));
        }
      } else {
        if (has_all_keyword) {
          tmpl_inst->error(
            "Unexpected `all' keyword after `omit' when omitting one field");
        }
      }
      type = 0;
      return;
    }
    if (has_all_keyword) {
      tmpl_inst->error(
        "Unexpected `all' keyword after the in-line template");
    }

    // determine the type of the tmpl_inst
    type = tmpl_inst->get_expr_governor(Type::EXPECTED_TEMPLATE);
    if (!type) {
      tmpl_inst->get_Template()->set_lowerid_to_ref();
      type = tmpl_inst->get_expr_governor(Type::EXPECTED_TEMPLATE);
    }
    if (!type) {
      tmpl_inst->error("Cannot determine the type of the in-line template");
      return;
    }
    type->chk();
    Type* type_last = type->get_type_refd_last();
    if (!type_last || type_last->get_typetype()==Type::T_ERROR) {
      type = 0;
      return;
    }
    if (is_raw) {
      switch (type_last->get_typetype_ttcn3()) {
      case Type::T_BSTR:
      case Type::T_OSTR:
      case Type::T_CSTR:
      case Type::T_USTR:
        break;
      default:
        tmpl_inst->error("An in-line template of type `%s' cannot be used as a `raw' erroneous value",
                         type_last->get_typename().c_str());
      }
    }

    if (tmpl_inst->get_DerivedRef()) {
      tmpl_inst->error("Reference to a constant value was expected instead of an in-line modified template");
      type = 0;
      return;
    }

    Template* templ = tmpl_inst->get_Template();

    if (templ->is_Value()) {
      value = templ->get_Value();
      value->set_my_governor(type);
      type->chk_this_value_ref(value);
      // dynamic values are allowed for attribute specs in '@update' statements,
      // but not for the initial attribute specs of constants and templates
      type->chk_this_value(value, 0, in_update_stmt ?
        Type::EXPECTED_DYNAMIC_VALUE : Type::EXPECTED_CONSTANT,
        INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
      //{ FIXME: make this work
      //  ReferenceChain refch(type, "While checking embedded recursions");
      //  value->chk_recursions(refch);
      //}
      value->set_code_section(GovernedSimple::CS_PRE_INIT);
    } else {
      tmpl_inst->error("A specific value without matching symbols was expected");
      type = 0;
      return;
    }
  }

  const char* ErroneousAttributeSpec::get_indicator_str(indicator_t i)
  {
    switch (i) {
    case I_BEFORE: return "before";
    case I_VALUE: return "value";
    case I_AFTER: return "after";
    default: FATAL_ERROR("ErroneousAttributeSpec::get_indicator_str()");
    }
    return "";
  }

  char* ErroneousAttributeSpec::generate_code_str(char *str, char *& def, string genname)
  {
    if (get_is_omit()) return str;
    if (!type) FATAL_ERROR("ErroneousAttributeSpec::generate_code_str()");
    if (!value) FATAL_ERROR("ErroneousAttributeSpec::generate_code_str()");
    if (first_genname.empty()) { // this is the first use
      str = mputprintf(str, "%s%s %s;\n", split_to_slices ? "" : "static ",
        type->get_genname_value(value->get_my_scope()).c_str(), genname.c_str());
      first_genname = genname;
      if (split_to_slices) {
        def = mputprintf(def, "extern %s %s;\n",
          type->get_genname_value(value->get_my_scope()).c_str(), genname.c_str());
      }
    } else {
      str = mputprintf(str, "%s& %s = %s;\n",
        type->get_genname_value(value->get_my_scope()).c_str(),
        genname.c_str(), first_genname.c_str());
    }
    return str;
  }

  char* ErroneousAttributeSpec::generate_code_init_str(char *str, string genname)
  {
    if (get_is_omit()) return str;
    if (!type) FATAL_ERROR("ErroneousAttributeSpec::generate_code_init_str()");
    if (!value) FATAL_ERROR("ErroneousAttributeSpec::generate_code_init_str()");
    str = value->generate_code_init(str, genname.c_str());
    return str;
  }

  string ErroneousAttributeSpec::get_typedescriptor_str()
  {
    if (get_is_omit() || is_raw) return string("NULL");
    if (!type) FATAL_ERROR("ErroneousAttributeSpec::get_typedescriptor_str()");
    if (!value) FATAL_ERROR("ErroneousAttributeSpec::generate_code_ti_str()");
    return ( "&" + type->get_genname_typedescriptor(value->get_my_scope()) + "_descr_" );
  }

  void ErroneousAttributeSpec::chk_recursions(ReferenceChain& refch)
  {
    if (value) {
      refch.mark_state();
      value->chk_recursions(refch);
      refch.prev_state();
    }
  }

  // ==== ErroneousValues ====

  char* ErroneousValues::generate_code_embedded_str(char *str, char *& def, string genname)
  {
    if (before) str = generate_code_embedded_str(str, def, genname+"_before", before);
    if (value) str = generate_code_embedded_str(str, def, genname+"_value", value);
    if (after) str = generate_code_embedded_str(str, def, genname+"_after", after);
    return str;
  }

  char* ErroneousValues::generate_code_embedded_str(char *str, char *& def, string genname, ErroneousAttributeSpec* attr_spec)
  {
    str = attr_spec->generate_code_str(str, def, genname+"_errval");
    str = mputprintf(str, "%sErroneous_value_t %s = { %s, %s, %s };\n",
      split_to_slices ? "" : "static ", genname.c_str(),
      attr_spec->get_is_raw() ? "true" : "false",
      attr_spec->get_is_omit() ? "NULL" : ("&"+genname+"_errval").c_str(),
      attr_spec->get_typedescriptor_str().c_str());
    if (split_to_slices) {
      def = mputprintf(def, "extern Erroneous_value_t %s;\n", genname.c_str());
    }
    return str;
  }

  char* ErroneousValues::generate_code_init_str(char *str, string genname)
  {
    if (before) str = before->generate_code_init_str(str, genname+"_before_errval");
    if (value) str = value->generate_code_init_str(str, genname+"_value_errval");
    if (after) str = after->generate_code_init_str(str, genname+"_after_errval");
    return str;
  }

  char* ErroneousValues::generate_code_struct_str(char *str, string genname, int field_index)
  {
    str = mputprintf(str, "{ %d, %s, %s, %s, %s }", field_index,
      ("\""+field_name+"\"").c_str(),
      before ? ("&"+genname+"_before").c_str() : "NULL",
      value ? ("&"+genname+"_value").c_str() : "NULL",
      after ? ("&"+genname+"_after").c_str() : "NULL");
    return str;
  }

  void ErroneousValues::chk_recursions(ReferenceChain& refch)
  {
    if (before) before->chk_recursions(refch);
    if (value) value->chk_recursions(refch);
    if (after) after->chk_recursions(refch);
  }

  // ==== ErroneousDescriptor ====

  char* ErroneousDescriptor::generate_code_embedded_str(char *str, char *& def, string genname)
  {
    // values
    for (size_t i=0; i<values_m.size(); i++) {
      str = values_m.get_nth_elem(i)->generate_code_embedded_str(str, def, genname+"_v"+Int2string(static_cast<int>( values_m.get_nth_key(i) )));
    }
    // embedded descriptors
    for (size_t i=0; i<descr_m.size(); i++) {
      str = descr_m.get_nth_elem(i)->generate_code_embedded_str(str, def, genname+"_d"+Int2string(static_cast<int>( descr_m.get_nth_key(i) )));
    }
    // values vector
    if (values_m.size()>0) {
      str = mputprintf(str, "%sErroneous_values_t %s_valsvec[%d] = { ",
        split_to_slices ? "" : "static ", genname.c_str(), static_cast<int>( values_m.size() ));
      for (size_t i=0; i<values_m.size(); i++) {
        if (i>0) str = mputstr(str, ", ");
        int key_i = static_cast<int>( values_m.get_nth_key(i) );
        str = values_m.get_nth_elem(i)->generate_code_struct_str(str, genname+"_v"+Int2string(key_i), key_i);
      }
      str = mputstr(str, " };\n");
      if (split_to_slices) {
        def = mputprintf(def, "extern Erroneous_values_t %s_valsvec[%d];\n", genname.c_str(), static_cast<int>( values_m.size() ));
      }
    }
    // embedded descriptor vector
    if (descr_m.size()>0) {
      str = mputprintf(str, "%sErroneous_descriptor_t %s_embvec[%d] = { ",
        split_to_slices ? "" : "static ", genname.c_str(), static_cast<int>( descr_m.size() ));
      for (size_t i=0; i<descr_m.size(); i++) {
        if (i>0) str = mputstr(str, ", ");
        int key_i = static_cast<int>( descr_m.get_nth_key(i) );
        str = descr_m.get_nth_elem(i)->generate_code_struct_str(str, def, genname+"_d"+Int2string(key_i), key_i);
      }
      str = mputstr(str, " };\n");
      if (split_to_slices) {
        def = mputprintf(def, "extern Erroneous_descriptor_t %s_embvec[%d];\n", genname.c_str(), static_cast<int>( descr_m.size() ));
      }
    }
    return str;
  }

  char* ErroneousDescriptor::generate_code_init_str(char *str, string genname)
  {
    for (size_t i=0; i<values_m.size(); i++) {
      str = values_m.get_nth_elem(i)->generate_code_init_str(str, genname+"_v"+Int2string(static_cast<int>( values_m.get_nth_key(i) )));
    }
    for (size_t i=0; i<descr_m.size(); i++) {
      str = descr_m.get_nth_elem(i)->generate_code_init_str(str, genname+"_d"+Int2string(static_cast<int>( descr_m.get_nth_key(i) )));
    }
    return str;
  }

  char* ErroneousDescriptor::generate_code_struct_str(char *str, char *& /*def*/, string genname, int field_index)
  {
    string genname_values_vec = genname + "_valsvec";
    string genname_embedded_vec = genname + "_embvec";
    str = mputprintf(str, "{ %d, %d, %s, %d, %s, %d, %s, %d, %s }",
      field_index,
      omit_before, (omit_before==-1)?"NULL":("\""+omit_before_name+"\"").c_str(),
      omit_after,  (omit_after==-1)?"NULL":("\""+omit_after_name+"\"").c_str(),
      static_cast<int>( values_m.size() ), (values_m.size()>0) ? genname_values_vec.c_str(): "NULL",
      static_cast<int>( descr_m.size() ), (descr_m.size()>0) ? genname_embedded_vec.c_str(): "NULL");
    return str;
  }

  void ErroneousDescriptor::chk_recursions(ReferenceChain& refch)
  {
    for (size_t i=0; i<values_m.size(); i++) {
      values_m.get_nth_elem(i)->chk_recursions(refch);
    }
      
    for (size_t i=0; i<descr_m.size(); i++) {
      descr_m.get_nth_elem(i)->chk_recursions(refch);
    }
  }

  char* ErroneousDescriptor::generate_code_str(char *str, char *& def, string genname)
  {
    genname += "_err_descr";
    str = generate_code_embedded_str(str, def, genname);
    str = mputprintf(str, "%sErroneous_descriptor_t %s = ",
      split_to_slices ? "" : "static ", genname.c_str());
    str = generate_code_struct_str(str, def, genname, -1);
    str = mputstr(str, ";\n");
    if (split_to_slices) {
      def = mputprintf(def, "extern Erroneous_descriptor_t %s;\n", genname.c_str());
    }
    return str;
  }
  
  ErroneousDescriptors::~ErroneousDescriptors()
  {
    descr_map.clear();
  }
  
  void ErroneousDescriptors::add(Statement* p_update_statement, ErroneousDescriptor* p_descr)
  {
    descr_map.add(p_update_statement, p_descr);
  }
  
  bool ErroneousDescriptors::has_descr(Statement* p_update_statement)
  {
    return descr_map.has_key(p_update_statement);
  }
  
  size_t ErroneousDescriptors::get_descr_index(Statement* p_update_statement)
  {
    return descr_map.find_key(p_update_statement);
  }
  
  char* ErroneousDescriptors::generate_code_init_str(Statement* p_update_statement, char *str, string genname)
  {
    size_t i = descr_map.find_key(p_update_statement);
    return descr_map.get_nth_elem(i)->generate_code_init_str(str,
      genname + string("_") + Int2string(static_cast<int>(i)) + string("_err_descr"));
  }
  
  char* ErroneousDescriptors::generate_code_str(Statement* p_update_statement, char *str, char *& def, string genname)
  {
    size_t i = descr_map.find_key(p_update_statement);
    return descr_map.get_nth_elem(i)->generate_code_str(str, def,
      genname + string("_") + Int2string(static_cast<int>(i)));
  }
  
  void ErroneousDescriptors::chk_recursions(ReferenceChain& refch)
  {
    for (size_t i = 0; i < descr_map.size(); ++i) {
      descr_map.get_nth_elem(i)->chk_recursions(refch);
    }
  }
  
  boolean ErroneousDescriptors::can_have_err_attribs(Type* t)
  {
    if (t == NULL) {
      FATAL_ERROR("ErroneousDescriptors::can_have_err_attribs");
    }
    t = t->get_type_refd_last();
    switch (t->get_typetype_ttcn3()) {
    case Type::T_SEQ_T:
    case Type::T_SET_T:
      if (t->get_nof_comps() == 0) {
        // empty records/sets can't have erroneous attributes
        return FALSE;
      }
      // else fall through
    case Type::T_SEQOF:
    case Type::T_SETOF:
    case Type::T_CHOICE_T:
      return use_runtime_2;
    default:
      return FALSE;
    }
  }

  // ==== ErroneousAttributes ====

  ErroneousAttributes::ErroneousAttributes(Type* p_type):
    type(p_type), err_descr_tree(NULL)
  {
    if (!type) FATAL_ERROR("ErroneousAttributes::ErroneousAttributes()");
  }

  ErroneousAttributes::~ErroneousAttributes()
  {
    for (size_t i=0; i<spec_vec.size(); i++) {
      delete spec_vec[i];
    }
    spec_vec.clear();
    if (err_descr_tree) delete err_descr_tree;
  }

  ErroneousAttributes::ErroneousAttributes(const ErroneousAttributes& p)
    : Node(p), type(p.type), err_descr_tree(NULL)
  {
  }

  ErroneousAttributes* ErroneousAttributes::clone() const
  {
    return new ErroneousAttributes(*this);
  }

  void ErroneousAttributes::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for (size_t i=0; i<spec_vec.size(); i++) {
      spec_vec[i]->set_fullname(p_fullname+".<erroneous_attr_spec_"+Int2string(i)+">");
    }
  }

  void ErroneousAttributes::dump(unsigned level) const
  {
    DEBUG(level, "erroneous attributes:");
    for (size_t i=0; i<spec_vec.size(); i++) {
      spec_vec[i]->dump(level+1);
    }
  }

  void ErroneousAttributes::add_spec(ErroneousAttributeSpec* err_attr_spec)
  {
    if (!err_attr_spec) FATAL_ERROR("ErroneousAttributes::add_spec()");
    spec_vec.add(err_attr_spec);
  }

  void ErroneousAttributes::add_pair(const Qualifier* qualifier, ErroneousAttributeSpec* err_attr_spec)
  {
    if (!qualifier || !err_attr_spec) FATAL_ERROR("ErroneousAttributes::add()");
    field_err_t f;
    f.qualifier = qualifier;
    f.err_attr = err_attr_spec;
    field_array.add(f);
  }

  void ErroneousAttributes::chk()
  {
    // check that encodings of erroneous type and templateinstance type match
    for (size_t i=0; i<spec_vec.size(); i++) {
      ErroneousAttributeSpec* act_attr = spec_vec[i];
      Type* ti_type = act_attr->get_type();
      if ((act_attr->get_indicator()!=ErroneousAttributeSpec::I_INVALID) && ti_type) {
        if (act_attr->get_is_raw()) {
          switch (ti_type->get_typetype_ttcn3()) {
          case Type::T_BSTR:
            if (!type->has_encoding(Type::CT_RAW)) {
              act_attr->error("A `raw' %s value was used for erroneous type `%s' which has no RAW encoding.",
                              ti_type->get_typename().c_str(), type->get_typename().c_str());
            }
            break;
          case Type::T_CSTR:
            if (!type->has_encoding(Type::CT_TEXT) && !type->has_encoding(Type::CT_XER) &&
                !type->has_encoding(Type::CT_JSON)) {
              act_attr->error("A `raw' %s value was used for erroneous type `%s' which has no TEXT, XER or JSON encodings.",
                              ti_type->get_typename().c_str(), type->get_typename().c_str());
            }
            break;
          case Type::T_USTR:
            if (!type->has_encoding(Type::CT_XER) && !type->has_encoding(Type::CT_JSON)) {
              act_attr->error("A `raw' %s value was used for erroneous type `%s' which has no XER or JSON encodings.",
                              ti_type->get_typename().c_str(), type->get_typename().c_str());
            }
            break;
          default:
            break;
          }
        } else {
          // the two types must have at least one common encoding
          if (!((type->has_encoding(Type::CT_BER)&&ti_type->has_encoding(Type::CT_BER)) ||
                (type->has_encoding(Type::CT_RAW)&&ti_type->has_encoding(Type::CT_RAW)) ||
                (type->has_encoding(Type::CT_TEXT)&&ti_type->has_encoding(Type::CT_TEXT)) ||
                (type->has_encoding(Type::CT_XER)&&ti_type->has_encoding(Type::CT_XER)) ||
                (type->has_encoding(Type::CT_JSON)&&ti_type->has_encoding(Type::CT_JSON)))) {
            act_attr->error("Type `%s' and type `%s' have no common encoding",
              ti_type->get_typename().c_str(), type->get_typename().c_str());
          }
        }
      }
    }

    // for every erroneous field calculate the corresponding index and type arrays
    // for example: x[5].z -> [3,5,2] and [MyRec,MyRecOf,MyUnion]
    // MyRec.x field has index 3, etc.
    for (size_t i=0; i<field_array.size(); i++) {
      bool b = type->get_subrefs_as_array(field_array[i].qualifier, field_array[i].subrefs_array, field_array[i].type_array);
      if (!b) FATAL_ERROR("ErroneousAttributes::chk()");
    }
    // check the qualifiers and build the tree
    err_descr_tree = build_descr_tree(field_array);
  }

  ErroneousDescriptor* ErroneousAttributes::build_descr_tree(dynamic_array<field_err_t>& fld_array)
  {
    ErroneousDescriptor* err_descr = new ErroneousDescriptor();
    const Qualifier * omit_before_qual = NULL, * omit_after_qual = NULL;
    map< size_t, dynamic_array<field_err_t> > embedded_fld_array_m; // used for recursive calls
    for (size_t i=0; i<fld_array.size(); i++) {
      field_err_t& act_field_err = fld_array[i];
      if (act_field_err.subrefs_array.size()<1) FATAL_ERROR("ErroneousAttributes::build_descr_tree()");
      size_t fld_idx = act_field_err.subrefs_array[0];
      if (omit_before_qual && (err_descr->omit_before!=-1) && (err_descr->omit_before>static_cast<int>(fld_idx))) {
        act_field_err.qualifier->error(
          "Field `%s' cannot be referenced because all fields before field `%s' have been omitted",
          act_field_err.qualifier->get_stringRepr().c_str(), omit_before_qual->get_stringRepr().c_str());
        continue;
      }
      if (omit_after_qual && (err_descr->omit_after!=-1) && (err_descr->omit_after<static_cast<int>(fld_idx))) {
        act_field_err.qualifier->error(
          "Field `%s' cannot be referenced because all fields after field `%s' have been omitted",
          act_field_err.qualifier->get_stringRepr().c_str(), omit_after_qual->get_stringRepr().c_str());
        continue;
      }
      ErroneousAttributeSpec::indicator_t act_indicator = act_field_err.err_attr->get_indicator();
      bool is_omit = act_field_err.err_attr->get_is_omit();
      if (act_field_err.subrefs_array.size()==1) { // erroneous value
        if (act_field_err.type_array.size()!=1) FATAL_ERROR("ErroneousAttributes::build_descr_tree()");
        if ((act_field_err.type_array[0]->get_typetype()==Type::T_SET_A) &&
            is_omit && (act_indicator!=ErroneousAttributeSpec::I_VALUE)) {
          act_field_err.qualifier->error(
            "Cannot omit all fields %s `%s' which is a field of an ASN.1 SET type",
            ErroneousAttributeSpec::get_indicator_str(act_indicator), act_field_err.qualifier->get_stringRepr().c_str());
          act_field_err.qualifier->note(
            "The order of fields in ASN.1 SET types changes depending on tagging (see X.690 9.3). "
            "Fields can be omitted individually, independently of the field order which depends on tagging");
            continue;
        }
        switch (act_field_err.type_array[0]->get_typetype_ttcn3()) {
        case Type::T_CHOICE_T:
          if (act_indicator!=ErroneousAttributeSpec::I_VALUE) {
            act_field_err.qualifier->error(
              "Indicator `%s' cannot be used with reference `%s' which points to a field of a union type",
              ErroneousAttributeSpec::get_indicator_str(act_indicator), act_field_err.qualifier->get_stringRepr().c_str());
            continue;
          }
          break;
        case Type::T_SEQ_T:
        case Type::T_SET_T:
          if (is_omit && (act_indicator==ErroneousAttributeSpec::I_AFTER) &&
              (fld_idx==act_field_err.type_array[0]->get_nof_comps()-1)) {
            act_field_err.qualifier->error(
              "There is nothing to omit after the last field (%s) of a record/set type",
              act_field_err.qualifier->get_stringRepr().c_str());
            continue;
          }
          // no break
        case Type::T_SEQOF:
        case Type::T_SETOF:
          if (is_omit && (act_indicator==ErroneousAttributeSpec::I_BEFORE) &&
              (fld_idx==0)) {
            act_field_err.qualifier->error(
              "There is nothing to omit before the first field (%s)",
              act_field_err.qualifier->get_stringRepr().c_str());
            continue;
          }
          break;
        default:
          break;
        }
        // check for duplicate value+indicator
        if (err_descr->values_m.has_key(fld_idx)) {
          ErroneousValues* evs = err_descr->values_m[fld_idx];
          if ( (evs->before && (act_indicator==ErroneousAttributeSpec::I_BEFORE)) ||
               (evs->value && (act_indicator==ErroneousAttributeSpec::I_VALUE)) ||
               (evs->after && (act_indicator==ErroneousAttributeSpec::I_AFTER)) ) {
            act_field_err.qualifier->error(
              "Duplicate reference to field `%s' with indicator `%s'",
              act_field_err.qualifier->get_stringRepr().c_str(), ErroneousAttributeSpec::get_indicator_str(act_indicator));
            continue;
          }
        }
        // when overwriting a value check if embedded values were used
        if ((act_indicator==ErroneousAttributeSpec::I_VALUE) && embedded_fld_array_m.has_key(fld_idx)) {
          act_field_err.qualifier->error(
            "Reference to field `%s' with indicator `value' would invalidate previously specified erroneous data",
            act_field_err.qualifier->get_stringRepr().c_str());
          continue;
        }
        // if before/after omit then check that no references to omitted regions and no duplication of omit before/after rule
        if ((act_indicator==ErroneousAttributeSpec::I_BEFORE) && is_omit) {
          if (omit_before_qual && (err_descr->omit_before!=-1)) {
            act_field_err.qualifier->error(
              "Duplicate rule for omitting all fields before the specified field. Used on field `%s' but previously already used on field `%s'",
              act_field_err.qualifier->get_stringRepr().c_str(), omit_before_qual->get_stringRepr().c_str());
            continue;
          }
          bool is_invalid = false;
          for (size_t j=0; j<err_descr->values_m.size(); j++) {
            if (err_descr->values_m.get_nth_key(j)<fld_idx) {
              is_invalid = true;
              break;
            }
          }
          if (!is_invalid) {
            for (size_t j=0; j<embedded_fld_array_m.size(); j++) {
              if (embedded_fld_array_m.get_nth_key(j)<fld_idx) {
                is_invalid = true;
                break;
              }
            }
          }
          if (is_invalid) {
            act_field_err.qualifier->error(
              "Omitting fields before field `%s' would invalidate previously specified erroneous data",
              act_field_err.qualifier->get_stringRepr().c_str());
            continue;
          }
          // save valid omit before data
          omit_before_qual = act_field_err.qualifier;
          err_descr->omit_before = fld_idx;
          err_descr->omit_before_name = omit_before_qual->get_stringRepr();
          continue;
        }
        if ((act_indicator==ErroneousAttributeSpec::I_AFTER) && is_omit) {
          if (omit_after_qual && (err_descr->omit_after!=-1)) {
            act_field_err.qualifier->error(
              "Duplicate rule for omitting all fields after the specified field. Used on field `%s' but previously already used on field `%s'",
              act_field_err.qualifier->get_stringRepr().c_str(), omit_after_qual->get_stringRepr().c_str());
            continue;
          }
          bool is_invalid = false;
          for (size_t j=0; j<err_descr->values_m.size(); j++) {
            if (err_descr->values_m.get_nth_key(j)>fld_idx) {
              is_invalid = true;
              break;
            }
          }
          if (!is_invalid) {
            for (size_t j=0; j<embedded_fld_array_m.size(); j++) {
              if (embedded_fld_array_m.get_nth_key(j)>fld_idx) {
                is_invalid = true;
                break;
              }
            }
          }
          if (is_invalid) {
            act_field_err.qualifier->error(
              "Omitting fields after field `%s' would invalidate previously specified erroneous data",
              act_field_err.qualifier->get_stringRepr().c_str());
            continue;
          }
          // save valid omit after data
          omit_after_qual = act_field_err.qualifier;
          err_descr->omit_after = fld_idx;
          err_descr->omit_after_name = omit_after_qual->get_stringRepr();
          continue;
        }
        // if not before/after omit then save this into values_m
        bool has_key = err_descr->values_m.has_key(fld_idx);
        ErroneousValues* evs = has_key ? err_descr->values_m[fld_idx] : new ErroneousValues(act_field_err.qualifier->get_stringRepr());
        switch (act_indicator) {
        case ErroneousAttributeSpec::I_BEFORE: evs->before = act_field_err.err_attr; break;
        case ErroneousAttributeSpec::I_VALUE: evs->value = act_field_err.err_attr; break;
        case ErroneousAttributeSpec::I_AFTER: evs->after = act_field_err.err_attr; break;
        default: FATAL_ERROR("ErroneousAttributes::build_descr_tree()");
        }
        if (!has_key) {
          err_descr->values_m.add(fld_idx, evs);
        }
      } else { // embedded err.value
        if ((err_descr->values_m.has_key(fld_idx)) && (err_descr->values_m[fld_idx]->value)) {
          act_field_err.qualifier->error(
            "Field `%s' is embedded into a field which was previously overwritten or omitted",
            act_field_err.qualifier->get_stringRepr().c_str());
          continue;
        }
        // add the embedded field to the map
        bool has_idx = embedded_fld_array_m.has_key(fld_idx);
        dynamic_array<field_err_t>* emb_fld_array = has_idx ? embedded_fld_array_m[fld_idx] : new dynamic_array<field_err_t>();
        field_err_t emb_field_err = act_field_err;
        emb_field_err.subrefs_array.remove(0); // remove the first reference
        emb_field_err.type_array.remove(0);
        emb_fld_array->add(emb_field_err);
        if (!has_idx) {
          embedded_fld_array_m.add(fld_idx, emb_fld_array);
        }
      }
    }
    // recursive calls to create embedded descriptors
    for (size_t i=0; i<embedded_fld_array_m.size(); i++) {
      dynamic_array<field_err_t>* emb_fld_array = embedded_fld_array_m.get_nth_elem(i);
      err_descr->descr_m.add(embedded_fld_array_m.get_nth_key(i), build_descr_tree(*emb_fld_array));
      delete emb_fld_array;
    }
    embedded_fld_array_m.clear();
    return err_descr;
  }

  // ==== AttributeSpec ====

  AttributeSpec* AttributeSpec::clone() const
  {
    return new AttributeSpec(*this);
  }

  void AttributeSpec::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
  }

  void AttributeSpec::dump(unsigned level) const
  {
    DEBUG(level,"spec: %s", spec.c_str());
  }

  // ==== SingleWithAttrib ====

  SingleWithAttrib::SingleWithAttrib(const SingleWithAttrib& p)
    : Node(p), Location(p), attribKeyword(p.attribKeyword),
      hasOverride(p.hasOverride)
  {
    attribQualifiers = p.attribQualifiers ? p.attribQualifiers->clone() : 0;
    attribSpec = p.attribSpec->clone();
  }

  SingleWithAttrib::SingleWithAttrib(
        attribtype_t p_attribKeyword, bool p_hasOverride,
        Qualifiers *p_attribQualifiers, AttributeSpec* p_attribSpec)
    : Node(), Location(), attribKeyword(p_attribKeyword),
      hasOverride(p_hasOverride), attribQualifiers(p_attribQualifiers),
      attribSpec(p_attribSpec)
  {
    if(!p_attribSpec)
      FATAL_ERROR("SingleWithAttrib::SingleWithAttrib()");
  }

  SingleWithAttrib::~SingleWithAttrib()
  {
    delete attribQualifiers;
    delete attribSpec;
  }

  SingleWithAttrib* SingleWithAttrib::clone() const
  {
    return new SingleWithAttrib(*this);
  }

  void SingleWithAttrib::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if(attribQualifiers)
      attribQualifiers->set_fullname(p_fullname + ".<attribute qualifiers>");
    attribSpec->set_fullname(p_fullname + ".<attribute specification>");
  }

  void SingleWithAttrib::dump(unsigned level) const
  {
    DEBUG(level,"attribute");
    switch(attribKeyword)
    {
      case AT_ENCODE:
	DEBUG(level + 1,"keyword: encode");
	break;
      case AT_VARIANT:
	DEBUG(level + 1,"keyword: variant");
	break;
      case AT_DISPLAY:
       DEBUG(level + 1,"keyword: display");
       break;
      case AT_EXTENSION:
	DEBUG(level + 1,"keyword: extension");
	break;
      case AT_OPTIONAL:
        DEBUG(level + 1,"keyword: optional");
        break;
      case AT_ERRONEOUS:
        DEBUG(level+1, "keyword: erroneous");
        break;
      case AT_INVALID:
        DEBUG(level+1, "invalid keyword");
        break;
      default:
	FATAL_ERROR("SingleWithAttrib::dump()");
    }

    DEBUG(level + 1, hasOverride ? "has override" : "hasn't got override");

    if(attribSpec)
      attribSpec->dump(level + 1);
    if(attribQualifiers)
      attribQualifiers->dump(level + 1);
  }

  // ==== MultiWithAttrib ====

  MultiWithAttrib::MultiWithAttrib(const MultiWithAttrib& p)
    : Node(p), Location(p)
  {
    for(size_t i = 0; i < p.get_nof_elements(); i++)
    {
      elements.add(p.get_element(i)->clone());
    }
  }

  MultiWithAttrib* MultiWithAttrib::clone() const
  {
    return new MultiWithAttrib(*this);
  }

  void MultiWithAttrib::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for(size_t i = 0; i < elements.size(); i++)
    {
      elements[i]->set_fullname(p_fullname + ".<singlewithattribute "
        + Int2string(i) + ">");
    }
  }

  MultiWithAttrib::~MultiWithAttrib()
  {
    for(size_t i = 0; i < elements.size(); i++)
    {
      delete elements[i];
    }
    elements.clear();
  }

  const SingleWithAttrib* MultiWithAttrib::get_element(size_t p_index) const
  {
      return elements[p_index];
  }

  SingleWithAttrib* MultiWithAttrib::get_element_for_modification(
    size_t p_index)
  {
      return elements[p_index];
  }

  void MultiWithAttrib::delete_element(size_t p_index)
  {
    delete elements[p_index];
    elements.replace(p_index,1,NULL);
  }

  void MultiWithAttrib::dump(unsigned level) const
  {
    DEBUG(level,"with attrib parameters (%lu pcs)",
      static_cast<unsigned long>( elements.size() ));
    for(size_t i = 0; i < elements.size(); i++)
    {
      elements[i]->dump(level + 1);
    }
  }

  // ==== WithAttribPath ====

  WithAttribPath::WithAttribPath(const WithAttribPath& p)
    : Node(p), had_global_variants(false), attributes_checked(false),
      cached(false), s_o_encode(false), parent(p.parent)
  {
    m_w_attrib = p.m_w_attrib ? p.m_w_attrib->clone() : 0;
  }

  WithAttribPath::~WithAttribPath()
  {
    delete m_w_attrib;
    cache.clear();
  }

  WithAttribPath* WithAttribPath::clone() const
  {
    return new WithAttribPath(*this);
  }

  void WithAttribPath::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (m_w_attrib) m_w_attrib->set_fullname(p_fullname
        + ".<multiwithattribute>");
  }

  void WithAttribPath::chk_no_qualif()
  {
    if(attributes_checked)
      return;

    if(!m_w_attrib)
      return;

    const SingleWithAttrib *swa;
    for(int i = m_w_attrib->get_nof_elements() - 1; i >= 0; i--)
    {
      swa = m_w_attrib->get_element(i);
      if(  swa->get_attribQualifiers()
        && swa->get_attribQualifiers()->get_nof_qualifiers() != 0)
      {
        swa->error("field qualifiers are only allowed"
          " for record, set and union types");
        m_w_attrib->delete_element(i);
      }
    }

    attributes_checked = true;
  }
  
  void WithAttribPath::chk_only_erroneous()
  {
    if (attributes_checked || m_w_attrib == NULL) {
      return;
    }

    for (size_t i = 0; i < m_w_attrib->get_nof_elements(); ++i) {
      const SingleWithAttrib* attrib = m_w_attrib->get_element(i);
      if (attrib->get_attribKeyword() != SingleWithAttrib::AT_ERRONEOUS) {
        attrib->error("Only `erroneous' attributes are allowed in an `@update' "
          "statement");
      }
    }

    attributes_checked = true;
  }

  void WithAttribPath::dump(unsigned int level) const
  {
    DEBUG(level, "WithAttribPath");
    if (!m_w_attrib) return;
    DEBUG(level+1, "%lu elements",
      static_cast<unsigned long>( m_w_attrib->get_nof_elements() ));
    for (size_t i=0; i < m_w_attrib->get_nof_elements(); ++i) {
      const SingleWithAttrib* a = m_w_attrib->get_element(i);
      if (!a) continue;
      a->dump(level+1);
    }
  }

  /**
   *  Checks whether there is inconsistency among global attributes or not.
   *  Only the last encode can have effect so we can throw out the others.
   *   This is because encode is not an attribute, but a "context".
   *  If there is an overriding variant/display/extension then the
   *   following attributes from the same type should be omitted.
   */
  void WithAttribPath::chk_global_attrib(bool erroneous_allowed)
  {
    if(!m_w_attrib)
      return;

    if (!erroneous_allowed) {
      for(size_t i = m_w_attrib->get_nof_elements(); i > 0; i--) {
        const SingleWithAttrib* const temp_attrib =
          m_w_attrib->get_element(i-1);
        if (temp_attrib->get_attribKeyword()==SingleWithAttrib::AT_ERRONEOUS) {
          temp_attrib->error("The `erroneous' attribute can be used only on "
            "template and constant definitions");
        }
      }
    }

    bool has_encode = false;
    bool has_override_variant = false;
    bool has_override_display = false;
    bool has_override_extension = false;
    bool has_override_optional = false;
    for(size_t i = m_w_attrib->get_nof_elements(); i > 0; i--)
    {
      const SingleWithAttrib* const temp_attrib =
        m_w_attrib->get_element(i-1);
      switch(temp_attrib->get_attribKeyword())
      {
        case SingleWithAttrib::AT_ENCODE:
        {
          if(has_encode)
          {
            temp_attrib->warning("Only the last encode "
              "of the with statement will have effect");
            m_w_attrib->delete_element(i-1);
          }else{
            has_encode = true;
          }
        }
        break;
        case SingleWithAttrib::AT_ERRONEOUS:
        {
          if (temp_attrib->has_override()) {
            temp_attrib->error("Override cannot be used with erroneous");
          }
        }
        break;
        default:
        break;
      }
    }

    for(size_t i = 0; i < m_w_attrib->get_nof_elements();)
    {
      const SingleWithAttrib* const temp_attrib = m_w_attrib->get_element(i);
      switch(temp_attrib->get_attribKeyword())
      {
        case SingleWithAttrib::AT_VARIANT:
        {
          if(has_override_variant)
          {
            temp_attrib->warning("Only the first override"
            " variant of the with statement will have effect");
            m_w_attrib->delete_element(i);
          }else{
            if(temp_attrib->has_override())
              has_override_variant = true;
            i++;
          }
        }
        break;
        case SingleWithAttrib::AT_DISPLAY:
        {
          if(has_override_display)
          {
            temp_attrib->warning("Only the first override"
            " display of the with statement will have effect");
            m_w_attrib->delete_element(i);
          }else{
            if(temp_attrib->has_override())
              has_override_display = true;
            i++;
          }
        }
        break;
        case SingleWithAttrib::AT_EXTENSION:
        {
          if(has_override_extension)
          {
            temp_attrib->warning("Only the first override"
            " extension of the with statement will have effect");
            m_w_attrib->delete_element(i);
          }else{
            if(temp_attrib->has_override())
              has_override_extension = true;
            i++;
          }
        }
        break;
        case SingleWithAttrib::AT_OPTIONAL:
        {
          if ("implicit omit" != temp_attrib->get_attribSpec().get_spec() &&
              "explicit omit" != temp_attrib->get_attribSpec().get_spec()) {
            temp_attrib->error("Value of optional attribute can only be "
              "either 'explicit omit' or 'implicit omit' not '%s'",
              temp_attrib->get_attribSpec().get_spec().c_str());
          }
          if(has_override_optional)
          {
            temp_attrib->warning("Only the first override"
              " optional of the with statement will have effect");
            m_w_attrib->delete_element(i);
          }else{
            if(temp_attrib->has_override())
              has_override_optional = true;
            i++;
          }
        }
        break;
        default:
        i++;
        break;
      } // switch
    } // next i
  }

  void WithAttribPath::set_with_attr(MultiWithAttrib* p_m_w_attr)
  {
    if(m_w_attrib) FATAL_ERROR("WithAttribPath::set_with_attr()");
    m_w_attrib = p_m_w_attr;
    attributes_checked = false;
  }

  /**
   * Finds the real attributes checking the inherited ones with its own.
   * Only qualifierless attributes are handled.
   * The stepped_over_encode is needed because it can happen that we
   *  override an encode and later (inner) find variants without encode.
   *  As those were the overridden encode's variants we can not add them to
   *   our list.
   */
  void WithAttribPath::qualifierless_attrib_finder(
    vector<SingleWithAttrib>& p_result,
    bool& stepped_over_encode)
  {
    if(cached)
    {
      for(size_t i = 0; i < cache.size(); i++)
      {
        p_result.add(cache[i]);
      }
      stepped_over_encode = s_o_encode;
      return;
    }

    if(parent)
      parent->qualifierless_attrib_finder(p_result,stepped_over_encode);
    else
      stepped_over_encode = false;

    if(m_w_attrib)
    {
      // These two refer only to the attributes of this type
      int self_encode_index = -1; // the index of the "encode" attribute
      bool self_has_variant = false; // flag for the presence of a "variant"
      // The following refer to all the attributes, including those collected
      // from the parent and all the ancestors.
      bool par_has_override_encode = false;
      bool par_has_encode = false;
      // True if there is an encode attribute in the local attribute list,
      // it differs from the parents encode
      // and the parent does not overwrite it.
      bool new_local_encode_context = false;
      bool par_has_override_variant = false;
      bool par_has_override_display = false;
      bool par_has_override_extension = false;
      bool par_has_override_optional = false;

      //checking the owned attributes
      const SingleWithAttrib* act_single;
      const Qualifiers* act_qualifiers;
      const size_t m_w_attrib_nof_elements = m_w_attrib->get_nof_elements();
      for(size_t i = 0; i < m_w_attrib_nof_elements;i++)
      {
        act_single = m_w_attrib->get_element(i);
        act_qualifiers = act_single->get_attribQualifiers();

        //global attribute
        if(!act_qualifiers || act_qualifiers->get_nof_qualifiers() == 0)
        {
          switch(act_single->get_attribKeyword())
          {
          case SingleWithAttrib::AT_ENCODE:
            self_encode_index = i;
            break;

          case SingleWithAttrib::AT_VARIANT: {
            // Ignore JSON variants, these should not produce warnings
            const string& spec = act_single->get_attribSpec().get_spec();
            size_t i2 = 0;
            while (i2 < spec.size()) {
              if (spec[i2] != ' ' && spec[i2] != '\t') {
                break;
              }
              ++i2;
            }
            if (i2 == spec.size() || spec.find("JSON", i2) != i2) {
              self_has_variant = true;
            }
            break; }

          case SingleWithAttrib::AT_DISPLAY:
          case SingleWithAttrib::AT_EXTENSION:
          case SingleWithAttrib::AT_OPTIONAL:
          case SingleWithAttrib::AT_ERRONEOUS:
          case SingleWithAttrib::AT_INVALID:
            break;

          default:
            FATAL_ERROR("WithAttribPath::attrib_finder()");
          }
        } // if global
      } // next i

      // Here p_result contains attributes collected from outer scopes only.
      size_t p_result_size = p_result.size();
      // gather information on the attributes collected from the parents
      for(size_t i = 0; i < p_result_size; i++)
      {
        act_single = p_result[i];

        switch(act_single->get_attribKeyword())
        {
        case SingleWithAttrib::AT_ENCODE:
          par_has_encode = true;
          par_has_override_encode |= act_single->has_override();
          if(self_encode_index != -1)
          {
            // We also have an encode. See if they differ.
            new_local_encode_context = (act_single->get_attribSpec().get_spec()
              != m_w_attrib->get_element(self_encode_index)->
              get_attribSpec().get_spec());
          }
          break;

        case SingleWithAttrib::AT_VARIANT:
          par_has_override_variant |= act_single->has_override();
          break;
        case SingleWithAttrib::AT_DISPLAY:
          par_has_override_display |= act_single->has_override();
          break;
        case SingleWithAttrib::AT_EXTENSION:
          par_has_override_extension |= act_single->has_override();
          break;
        case SingleWithAttrib::AT_OPTIONAL:
          par_has_override_optional |= act_single->has_override();
          break;
        case SingleWithAttrib::AT_ERRONEOUS:
        case SingleWithAttrib::AT_INVALID:
          // ignore
          break;
        default:
          FATAL_ERROR("WithAttribPath::attrib_finder()");
        }
      }

      if(!par_has_encode && self_encode_index == -1 && self_has_variant)
      {
        // There is no encode, but there is at least one variant.
        // Find them and issue warnings.
        for(size_t i = 0; i < m_w_attrib_nof_elements; i++)
        {
          act_single = m_w_attrib->get_element(i);
          if(act_single->get_attribKeyword() == SingleWithAttrib::AT_VARIANT)
            act_single->warning("This variant does not belong to an encode");
        }
      }

      // If we have an encode (self_encode_index != -1) and it differs from
      // the outer encode (new_local_encode_context) and the outer wasn't
      // an "override encode" (!par_has_override_encode),
      // remove the outer encode and all the variants that belong to it.
      for(size_t i = p_result.size(); i > 0; i--)
      {
        switch(p_result[i-1]->get_attribKeyword())
        {
          case SingleWithAttrib::AT_ENCODE:
          case SingleWithAttrib::AT_VARIANT:
            if (self_encode_index != -1 && new_local_encode_context
              && !par_has_override_encode)
              p_result.replace(i-1,1,NULL);
          break;
          case SingleWithAttrib::AT_DISPLAY:
          case SingleWithAttrib::AT_EXTENSION:
          case SingleWithAttrib::AT_OPTIONAL:
          case SingleWithAttrib::AT_ERRONEOUS:
          case SingleWithAttrib::AT_INVALID:
          break;
        }
      }

      //adding the right ones from the local attributes
      for(size_t i = 0; i < m_w_attrib_nof_elements; i++)
      {
        act_single = m_w_attrib->get_element(i);
        act_qualifiers = act_single->get_attribQualifiers();
        if(!act_qualifiers || act_qualifiers->get_nof_qualifiers() == 0)
        {
          switch(act_single->get_attribKeyword())
          {
            case SingleWithAttrib::AT_ENCODE:
              if((par_has_encode && !par_has_override_encode
                    && new_local_encode_context)
                || (!par_has_encode))
              {
                p_result.add_front(m_w_attrib->get_element_for_modification(i));
                stepped_over_encode = false;
              }else if(new_local_encode_context){
                //par_has_encode && par_has_override_encode
                stepped_over_encode = true;
              }else{
                stepped_over_encode = false;
              }
            break;
            case SingleWithAttrib::AT_VARIANT:
              if((!par_has_encode && !par_has_override_variant)
                || (par_has_encode && self_encode_index == -1
                    && !stepped_over_encode && !par_has_override_variant)
                || (par_has_encode && self_encode_index != -1
                    && !par_has_override_encode && !par_has_override_variant)
                || (!par_has_encode && self_encode_index != -1))
                p_result.add(m_w_attrib->get_element_for_modification(i));
            break;
            case SingleWithAttrib::AT_DISPLAY:
              if(!par_has_override_display)
                p_result.add(m_w_attrib->get_element_for_modification(i));
            break;
            case SingleWithAttrib::AT_EXTENSION:
              if(!par_has_override_extension)
                p_result.add(m_w_attrib->get_element_for_modification(i));
            break;
            case SingleWithAttrib::AT_OPTIONAL:
              if (!par_has_override_optional)
            	p_result.add(m_w_attrib->get_element_for_modification(i));
            break;
            case SingleWithAttrib::AT_ERRONEOUS:
            case SingleWithAttrib::AT_INVALID:
              // ignore
            break;
          }
        }
      }
    }
  }

/*
 * Be very cautious this function gives back only the qualifierless attributes.
 * Because of types giving back all the attribs, so that they are already
 *  in their final encode context would mean that attributes coming from
 *  the parent path would need to be cloned and qualified as many times
 *  as many components the type has.
 */
  const vector<SingleWithAttrib>& WithAttribPath::get_real_attrib()
  {
    if (!cached) {
      qualifierless_attrib_finder(cache,s_o_encode);
      cached = true;
    }
    return cache;
  }

  bool WithAttribPath::has_attribs()
  {
    if (had_global_variants) return true;
    else if (get_real_attrib().size() > 0) return true;
    else if (m_w_attrib) {
      for (size_t i = 0; i < m_w_attrib->get_nof_elements(); i++) {
	const Qualifiers *qualifiers =
	  m_w_attrib->get_element(i)->get_attribQualifiers();
        if (qualifiers && qualifiers->get_nof_qualifiers() > 0) return true;
      }
    }
    return false;
  }

}
