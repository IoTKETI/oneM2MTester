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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include "../../common/dbgnew.hh"
#include "AST_ttcn3.hh"
#include "../Identifier.hh"
#include "../CompilerError.hh"
#include "../Setting.hh"
#include "../Type.hh"
#include "../CompField.hh"
#include "../CompType.hh"
#include "../TypeCompat.hh"
#include "../Valuestuff.hh"
#include "../Value.hh"
#include "Ttcnstuff.hh"
#include "TtcnTemplate.hh"
#include "Templatestuff.hh"
#include "ArrayDimensions.hh"
#include "compiler.h"
#include "../main.hh"
#include "Statement.hh"
#include "ILT.hh"
#include "Attributes.hh"
#include "PatternString.hh"
#include "../../common/version_internal.h"
#include "../CodeGenHelper.hh"
#include "../../common/JSON_Tokenizer.hh"
#include "../DebuggerStuff.hh"
#include <limits.h>

// implemented in coding_attrib_p.y
extern Ttcn::ExtensionAttributes * parse_extattributes(
  Ttcn::WithAttribPath *w_attrib_path);

// implemented in compiler.y
extern Ttcn::ErroneousAttributeSpec* ttcn3_parse_erroneous_attr_spec_string(
  const char* p_str, const Common::Location& str_loc);


extern void init_coding_attrib_lex(const Ttcn::AttributeSpec& attrib);
extern int coding_attrib_parse();
extern void cleanup_coding_attrib_lex();
extern Ttcn::ExtensionAttributes *extatrs;

/** Create a field name in the anytype
 *
 * The output of this function will be used to create an identifier
 * to be used as the field name in the anytype.
 * The type_name may be a built-in type (e.g. "integer") or a user-defined
 * type.
 *
 * If the name has multiple components (a fullname?), it keeps just the last
 * component without any dots. *
 * Also, the space in "universal charstring" needs to be replaced
 * with an underscore to make it an identifier.
 *
 * Note: Prefixing with "AT_" is not done here, but in defUnionClass().
 *
 * @param type_name string
 * @return string to be used as the identifier.
 */
string anytype_field(const string& type_name)
{
  string retval(type_name);

  // keep just the last part of the name
  // TODO check if there's a way to get just the last component (note that fetching the string is done outside of this function)
  size_t dot = retval.rfind('.');
  if (dot >= retval.size()) dot = 0;
  else ++dot;
  retval.replace(0, dot, "");

  return retval;
}

extern Common::Modules *modules; // in main.cc

namespace {
static const string _T_("_T_");
}

namespace Ttcn {

  using namespace Common;

  // =================================
  // ===== FieldOrArrayRef
  // =================================

  FieldOrArrayRef::FieldOrArrayRef(const FieldOrArrayRef& p)
    : Node(p), Location(p), ref_type(p.ref_type)
  {
    switch (p.ref_type) {
    case FIELD_REF:
      u.id = p.u.id->clone();
      break;
    case ARRAY_REF:
      u.arp = p.u.arp->clone();
      break;
    default:
      FATAL_ERROR("FieldOrArrayRef::FieldOrArrayRef()");
    }
  }

  FieldOrArrayRef::FieldOrArrayRef(Identifier *p_id)
    : Node(), Location(), ref_type(FIELD_REF)
  {
    if (!p_id) FATAL_ERROR("FieldOrArrayRef::FieldOrArrayRef()");
    u.id = p_id;
  }

  FieldOrArrayRef::FieldOrArrayRef(Value *p_arp)
    : Node(), Location(), ref_type(ARRAY_REF)
  {
    if (!p_arp) FATAL_ERROR("FieldOrArrayRef::FieldOrArrayRef()");
    u.arp = p_arp;
  }

  FieldOrArrayRef::~FieldOrArrayRef()
  {
    switch (ref_type) {
    case FIELD_REF:
      delete u.id;
      break;
    case ARRAY_REF:
      delete u.arp;
      break;
    default:
      FATAL_ERROR("FieldOrArrayRef::~FieldOrArrayRef()");
    }
  }

  FieldOrArrayRef *FieldOrArrayRef::clone() const
  {
    return new FieldOrArrayRef(*this);
  }

  void FieldOrArrayRef::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (ref_type == ARRAY_REF)
      u.arp->set_fullname(p_fullname + ".<array_index>");
  }

  void FieldOrArrayRef::set_my_scope(Scope *p_scope)
  {
    if (ref_type == ARRAY_REF) u.arp->set_my_scope(p_scope);
  }

  const Identifier* FieldOrArrayRef::get_id() const
  {
    if (ref_type != FIELD_REF) FATAL_ERROR("FieldOrArrayRef::get_id()");
    return u.id;
  }

  Value *FieldOrArrayRef::get_val() const
  {
    if (ref_type != ARRAY_REF) FATAL_ERROR("FieldOrArrayRef::get_val()");
    return u.arp;
  }

  void FieldOrArrayRef::append_stringRepr(string& str) const
  {
    switch (ref_type) {
    case FIELD_REF:
      str += '.';
      str += u.id->get_dispname();
      break;
    case ARRAY_REF:
      str += '[';
      str += u.arp->get_stringRepr();
      str += ']';
      break;
    default:
      str += "<unknown sub-reference>";
    }
  }
  
  void FieldOrArrayRef::set_field_name_to_lowercase()
  {
    if (ref_type != FIELD_REF) FATAL_ERROR("FieldOrArrayRef::set_field_name_to_lowercase()");
    string new_name = u.id->get_name();
    if (isupper(new_name[0])) {
      new_name[0] = static_cast<char>( tolower(new_name[0] ));
      if (new_name[new_name.size() - 1] == '_') {
        // an underscore is inserted at the end of the field name if it's
        // a basic type's name (since it would conflict with the class generated
        // for that type)
        // remove the underscore, it won't conflict with anything if its name
        // starts with a lowercase letter
        new_name.replace(new_name.size() - 1, 1, "");
      }
      delete u.id;
      u.id = new Identifier(Identifier::ID_NAME, new_name);
    }
  }

  // =================================
  // ===== FieldOrArrayRefs
  // =================================

  FieldOrArrayRefs::FieldOrArrayRefs(const FieldOrArrayRefs& p)
    : Node(p), refs_str_element(false)
  {
    for (size_t i = 0; i < p.refs.size(); i++) refs.add(p.refs[i]->clone());
  }

  FieldOrArrayRefs::~FieldOrArrayRefs()
  {
    for (size_t i = 0; i < refs.size(); i++) delete refs[i];
    refs.clear();
  }

  FieldOrArrayRefs *FieldOrArrayRefs::clone() const
  {
    return new FieldOrArrayRefs(*this);
  }

  void FieldOrArrayRefs::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for (size_t i = 0; i < refs.size(); i++)
      refs[i]->set_fullname(p_fullname +
        ".<sub_reference" + Int2string(i + 1) + ">");
  }

  void FieldOrArrayRefs::set_my_scope(Scope *p_scope)
  {
    for (size_t i = 0; i < refs.size(); i++) refs[i]->set_my_scope(p_scope);
  }

  bool FieldOrArrayRefs::has_unfoldable_index() const
  {
    for (size_t i = 0; i < refs.size(); i++) {
      FieldOrArrayRef *ref = refs[i];
      if (ref->get_type() == FieldOrArrayRef::ARRAY_REF) {
        Value *v = ref->get_val();
        v->set_lowerid_to_ref();
        if (v->is_unfoldable()) return true;
      }
    }
    return false;
  }

  void FieldOrArrayRefs::remove_refs(size_t n)
  {
    for (size_t i = 0; i < n; i++) delete refs[i];
    refs.replace(0, n, NULL);
    set_fullname(get_fullname());
  }

  /* remove_last_field is used when unfolding references for
     ischosen and ispresent function operands.
     In this case it is NOT sure the last field exists.
     Calling remove_last_field previously
     will avoid getting the "variable...Has no member called..." error message.
     The last field component will be checked as a separate step.
     Warning: the removed Identifier has to be deleted later */

  Identifier* FieldOrArrayRefs::remove_last_field()
  {
    if (refs.size() == 0) return 0;
    size_t last_elem_ind = refs.size() - 1;
    FieldOrArrayRef* last_elem = refs[last_elem_ind];
    if (last_elem->get_type() == FieldOrArrayRef::FIELD_REF) {
      Identifier *ret_val = last_elem->get_id()->clone();
      delete last_elem;
      refs.replace(last_elem_ind, 1, NULL);
      return ret_val;
    } else return 0;
  }

  void FieldOrArrayRefs::generate_code(expression_struct *expr,
    Common::Assignment *ass, size_t nof_subrefs /* = UINT_MAX*/)
  {
    Type *type = 0;
    bool is_template = false;
    switch (ass->get_asstype()) {
    case Common::Assignment::A_CONST:             // a Def_Const
    case Common::Assignment::A_EXT_CONST:         // a Def_ExtConst
    case Common::Assignment::A_MODULEPAR:         // a Def_Modulepar
    case Common::Assignment::A_VAR:               // a Def_Var
    case Common::Assignment::A_FUNCTION_RVAL:     // a Def_Function
    case Common::Assignment::A_EXT_FUNCTION_RVAL: // a Def_ExtFunction
    case Common::Assignment::A_PAR_VAL_IN:        // a FormalPar
    case Common::Assignment::A_PAR_VAL_OUT:       // a FormalPar
    case Common::Assignment::A_PAR_VAL_INOUT:     // a FormalPar
      // The type is important since the referred entities are value objects.
      type = ass->get_Type();
      break;
    case Common::Assignment::A_MODULEPAR_TEMP:  // a Def_Modulepar_Template
    case Common::Assignment::A_TEMPLATE:        // a Def_Template
    case Common::Assignment::A_VAR_TEMPLATE:    // a Def_Var_Template
    case Common::Assignment::A_PAR_TEMPL_IN:    // a FormalPar
    case Common::Assignment::A_PAR_TEMPL_OUT:   // a FormalPar
    case Common::Assignment::A_PAR_TEMPL_INOUT: // a FormalPar
      // The type is semi-important because fields of anytype templates
      // need the prefix.
      type = ass->get_Type();
      is_template = true;
      break;
    case Common::Assignment::A_TIMER:              // a Def_Timer
    case Common::Assignment::A_PORT:               // a Def_Port
    case Common::Assignment::A_FUNCTION_RTEMP:     // a Def_Function
    case Common::Assignment::A_EXT_FUNCTION_RTEMP: // a Def_ExtFunction
    case Common::Assignment::A_PAR_TIMER:          // a FormalPar
    case Common::Assignment::A_PAR_PORT:           // a FormalPar
      // The type is not relevant (i.e. the optional fields do not require
      // special handling).
      type = 0;
      break;
    default:
      // Reference to other definitions cannot occur during code generation.
      FATAL_ERROR("FieldOrArrayRefs::generate_code()");
      type = 0;
    }
    generate_code(expr, type, is_template, nof_subrefs);
  }
  
  void FieldOrArrayRefs::generate_code(expression_struct* expr, Type* type,
                                       bool is_template, /* = false */
                                       size_t nof_subrefs /* = UINT_MAX */)
  {
    size_t n_refs = (nof_subrefs != UINT_MAX) ? nof_subrefs : refs.size();
    for (size_t i = 0; i < n_refs; i++) {
      if (type) type = type->get_type_refd_last();
      //  type changes inside the loop; need to recompute "last" every time.
      FieldOrArrayRef *ref = refs[i];
      if (ref->get_type() == FieldOrArrayRef::FIELD_REF) {
        // Write a call to the field accessor method.
        // Fields of the anytype get a special prefix; see also:
        // Template::generate_code_init_se, TypeConv::gen_conv_func_choice_anytype,
        // defUnionClass and defUnionTemplate.
        const Identifier& id = *ref->get_id();
        expr->expr = mputprintf(expr->expr, ".%s%s()",
          ((type!=0 && type->get_typetype()==Type::T_ANYTYPE) ? "AT_" : ""),
          id.get_name().c_str());
        if (type) {
          CompField *cf = type->get_comp_byName(id);
          // If the field is optional, the return type of the accessor is an
          // OPTIONAL<T>. Write a call to OPTIONAL<T>::operator(),
          // which "reaches into" the OPTIONAL to get the contained type T.
          // Don't do this at the end of the reference chain.
          // Accessor methods for a foo_template return a bar_template
          // and OPTIONAL<> is not involved, hence no "()".
          if (!is_template && i < n_refs - 1 && cf->get_is_optional())
            expr->expr = mputstr(expr->expr, "()");
          // Follow the field type.
          type = cf->get_type();
        }
      } else {
        // Generate code for array reference.
        Value* v = ref->get_val();
        Type * pt = v->get_expr_governor_last();
        // If the value is indexed with an array or record of then generate
        // the indexes of the array or record of into the code, one by one.
        if (pt->get_typetype() == Type::T_ARRAY || pt->get_typetype() == Type::T_SEQOF) {
          int len = 0, start = 0;
          if (pt->get_typetype() == Type::T_ARRAY) {
            len = static_cast<int>( pt->get_dimension()->get_size() );
            start = pt->get_dimension()->get_offset();
          } else if (pt->get_typetype() == Type::T_SEQOF) {
            len = pt->get_sub_type()->get_length_restriction();
          }
          // Generate the indexes as [x][y]...
          for (int j = start; j < start + len; j++) {
            expr->expr = mputc(expr->expr, '[');
            v->generate_code_expr(expr);
            expr->expr = mputprintf(expr->expr, "[%i]]", j);
          }
        } else {
          expr->expr = mputc(expr->expr, '[');
          v->generate_code_expr(expr);
          expr->expr = mputc(expr->expr, ']');
        }
        if (type) {
          // Follow the embedded type.
          switch (type->get_typetype()) {
          case Type::T_SEQOF:
          case Type::T_SETOF:
          case Type::T_ARRAY:
            type = type->get_ofType();
            break;
          default:
            // The index points to a string element.
            // There are no further sub-references.
            type = 0;
          } // switch
        } // if (type)
      } // if (ref->get_type)
    } // next reference
  }

  void FieldOrArrayRefs::append_stringRepr(string& str) const
  {
    for (size_t i = 0; i < refs.size(); i++) refs[i]->append_stringRepr(str);
  }

  // =================================
  // ===== Ref_base
  // =================================

  Ref_base::Ref_base(const Ref_base& p)
    : Ref_simple(p), subrefs(p.subrefs)
  {
    modid = p.modid ? p.modid->clone() : 0;
    id = p.id ? p.id->clone() : 0;
    params_checked = p.is_erroneous;
  }

  Ref_base::Ref_base(Identifier *p_modid, Identifier *p_id)
    : Ref_simple(), modid(p_modid), id(p_id), params_checked(false)
      , usedInIsbound(false)
  {
    if (!p_id)
      FATAL_ERROR("NULL parameter: Ttcn::Ref_base::Ref_base()");
  }

  Ref_base::~Ref_base()
  {
    delete modid;
    delete id;
  }

  void Ref_base::set_fullname(const string& p_fullname)
  {
    Ref_simple::set_fullname(p_fullname);
    subrefs.set_fullname(p_fullname);
  }

  void Ref_base::set_my_scope(Scope *p_scope)
  {
    Ref_simple::set_my_scope(p_scope);
    subrefs.set_my_scope(p_scope);
  }

  /* returns the referenced variable's base type or value */
  Setting* Ref_base::get_refd_setting()
  {
    Common::Assignment *ass = get_refd_assignment();
    if (ass) return ass->get_Setting();
    else return 0;
  }

  FieldOrArrayRefs *Ref_base::get_subrefs()
  {
    if (!id) get_modid();
    if (subrefs.get_nof_refs() == 0) return 0;
    else return &subrefs;
  }

  bool Ref_base::has_single_expr()
  {
    Common::Assignment *ass = get_refd_assignment();
    if (!ass) FATAL_ERROR("Ref_base::has_single_expr()");
    for (size_t i = 0; i < subrefs.get_nof_refs(); i++) {
      FieldOrArrayRef *ref = subrefs.get_ref(i);
      if (ref->get_type() == FieldOrArrayRef::ARRAY_REF &&
        !ref->get_val()->has_single_expr()) return false;
    }
    return true;
  }

  void Ref_base::set_code_section(
    GovernedSimple::code_section_t p_code_section)
  {
    for (size_t i = 0; i < subrefs.get_nof_refs(); i++) {
      FieldOrArrayRef *ref = subrefs.get_ref(i);
      if (ref->get_type() == FieldOrArrayRef::ARRAY_REF)
        ref->get_val()->set_code_section(p_code_section);
    }
  }

  void Ref_base::generate_code_const_ref(expression_struct_t */*expr*/)
  {
    FATAL_ERROR("Ref_base::generate_code_const_ref()");
  }

  // =================================
  // ===== Reference
  // =================================

  Reference::Reference(Identifier *p_id)
    : Ref_base(), parlist(0)
  {
    subrefs.add(new FieldOrArrayRef(p_id));
  }

  Reference::~Reference()
  {
    if (parlist) {
      delete parlist;
    }
  }

  /* Called by:
   *         Common::PortTypeBody::PortTypeBody
   *         Common::Type::Type
   *         Common::TypeMappingTarget::TypeMappingTarget
   *         Common::PatternString::ps_elem_t::chk_ref */
  Reference *Reference::clone() const
  {
    return new Reference(*this);
  }

  string Reference::get_dispname()
  {
    string ret_val;
    if (id) {
      if (modid) {
        ret_val += modid->get_dispname();
        ret_val += '.';
      }
      ret_val += id->get_dispname();
      subrefs.append_stringRepr(ret_val);
    } else {
      subrefs.append_stringRepr(ret_val);
      // cut the leading dot
      if (!ret_val.empty() && ret_val[0] == '.')
        ret_val.replace(0, 1, "");
    }
    return ret_val;
  }

  Common::Assignment* Reference::get_refd_assignment(bool check_parlist)
  {
    Common::Assignment *ass = Ref_base::get_refd_assignment(check_parlist);
    // In fact calls        Ref_simple::get_refd_assignment
    if (ass && check_parlist && !params_checked) {
      params_checked = true;
      FormalParList *fplist = ass->get_FormalParList();
      if (fplist) {
        if (fplist->has_only_default_values()
          && Common::Assignment::A_TEMPLATE == ass->get_asstype()) {
          Ttcn::ParsedActualParameters params;
          Error_Context cntxt(&params, "In actual parameter list of %s",
              ass->get_description().c_str());
          parlist = new ActualParList();
          is_erroneous = fplist->fold_named_and_chk(&params, parlist);
          parlist->set_fullname(get_fullname());
          parlist->set_my_scope(my_scope);
        } else {
          error("Reference to parameterized definition `%s' without "
            "actual parameter list", ass->get_id().get_dispname().c_str());
        }
      }
    }
    return ass;
  }

  const Identifier* Reference::get_modid()
  {
    if (!id) detect_modid();
    return modid;
  }

  const Identifier* Reference::get_id()
  {
    if (!id) detect_modid();
    return id;
  }

  Type *Reference::chk_variable_ref()
  {
    Common::Assignment *t_ass = get_refd_assignment();
    if (!t_ass) return 0;
    switch (t_ass->get_asstype()) {
    case Common::Assignment::A_PAR_VAL_IN:
      t_ass->use_as_lvalue(*this);
      // no break
    case Common::Assignment::A_VAR:
    case Common::Assignment::A_PAR_VAL_OUT:
    case Common::Assignment::A_PAR_VAL_INOUT:
      break;
    default:
      error("Reference to a variable or value parameter was "
        "expected instead of %s", t_ass->get_description().c_str());
      return 0;
    }
    FieldOrArrayRefs *t_subrefs = get_subrefs();
    Type *ret_val = t_ass->get_Type()->get_field_type(t_subrefs,
      Type::EXPECTED_DYNAMIC_VALUE);
    if (ret_val && t_subrefs && t_subrefs->refers_to_string_element()) {
      error("Reference to a string element of type `%s' cannot be used in "
        "this context", ret_val->get_typename().c_str());
    }
    return ret_val;
  }

  Type *Reference::chk_comptype_ref()
  {
    Common::Assignment *ass = get_refd_assignment();
    if (ass) {
      if (ass->get_asstype() == Common::Assignment::A_TYPE) {
        Type *t = ass->get_Type()->get_type_refd_last();
        switch (t->get_typetype()) {
        case Type::T_ERROR:
          // remain silent
          break;
        case Type::T_COMPONENT:
          return t;
        default:
          error("Reference `%s' does not refer to a component type",
            get_dispname().c_str());
        }
      } else {
        error("Reference `%s' does not refer to a type",
          get_dispname().c_str());
      }
    }
    return 0;
  }
  
  bool Reference::has_single_expr()
  {
    if (!Ref_base::has_single_expr()) {
      return false;
    }
    if (parlist != NULL) {
      for (size_t i = 0; i < parlist->get_nof_pars(); i++) {
        if (!parlist->get_par(i)->has_single_expr()) {
          return false;
        }
      }
    }
    return true;
  }
  
  void Reference::ref_usage_found()
  {
    Common::Assignment *ass = get_refd_assignment();
    if (!ass) FATAL_ERROR("Reference::ref_usage_found()");
    switch (ass->get_asstype()) {
    case Common::Assignment::A_PAR_VAL_OUT:
    case Common::Assignment::A_PAR_TEMPL_OUT:
    case Common::Assignment::A_PAR_VAL:
    case Common::Assignment::A_PAR_VAL_IN:
    case Common::Assignment::A_PAR_VAL_INOUT:
    case Common::Assignment::A_PAR_TEMPL_IN:
    case Common::Assignment::A_PAR_TEMPL_INOUT:
    case Common::Assignment::A_PAR_PORT:
    case Common::Assignment::A_PAR_TIMER: {
      FormalPar *fpar = dynamic_cast<FormalPar*>(ass);
      if (fpar == NULL) {
        FATAL_ERROR("Reference::ref_usage_found()");
      }
      fpar->set_usage_found();
      break; }
    case Common::Assignment::A_EXT_CONST: {
      Def_ExtConst* def = dynamic_cast<Def_ExtConst*>(ass);
      if (def == NULL) {
        FATAL_ERROR("Reference::ref_usage_found()");
      }
      def->set_usage_found();
      break; }
    default:
      break;
    }
  }

  void Reference::generate_code(expression_struct_t *expr)
  {
    ref_usage_found();
    Common::Assignment *ass = get_refd_assignment();
    if (!ass) FATAL_ERROR("Reference::generate_code()");
    if (parlist) {
      // reference without parameters to a template that has only default formal parameters.
      // if @lazy: nothing to do, it's a C++ function call just like in case of Ref_pard::generate_code()
      expr->expr = mputprintf(expr->expr, "%s(",
        ass->get_genname_from_scope(my_scope).c_str());
      parlist->generate_code_alias(expr, ass->get_FormalParList(),
          ass->get_RunsOnType(), false);
      expr->expr = mputc(expr->expr, ')');
    } else {
      expr->expr = mputstr(expr->expr,
        LazyFuzzyParamData::in_lazy_or_fuzzy() ?
        LazyFuzzyParamData::add_ref_genname(ass, my_scope).c_str() :
        ass->get_genname_from_scope(my_scope).c_str());
    }
    if (subrefs.get_nof_refs() > 0) subrefs.generate_code(expr, ass);
  }

  void Reference::generate_code_const_ref(expression_struct_t *expr)
  {
    FieldOrArrayRefs *t_subrefs = get_subrefs();
    if (!t_subrefs || t_subrefs->get_nof_refs() == 0) {
      generate_code(expr);
      return;
    }
    
    ref_usage_found();
    Common::Assignment *ass = get_refd_assignment();
    if (!ass) FATAL_ERROR("Reference::generate_code_const_ref()");

    bool is_template;
    switch (ass->get_asstype()) {
    case Common::Assignment::A_MODULEPAR:
    case Common::Assignment::A_VAR:
    case Common::Assignment::A_FUNCTION_RVAL:
    case Common::Assignment::A_EXT_FUNCTION_RVAL:
    case Common::Assignment::A_PAR_VAL_IN:
    case Common::Assignment::A_PAR_VAL_OUT:
    case Common::Assignment::A_PAR_VAL_INOUT: {
      is_template = false;
      break; }
    case Common::Assignment::A_MODULEPAR_TEMP:
    case Common::Assignment::A_TEMPLATE:
    case Common::Assignment::A_VAR_TEMPLATE:
    case Common::Assignment::A_PAR_TEMPL_IN:
    case Common::Assignment::A_PAR_TEMPL_OUT:
    case Common::Assignment::A_PAR_TEMPL_INOUT: {
      is_template = true;
      break; }
    case Common::Assignment::A_CONST:
    case Common::Assignment::A_EXT_CONST:
    default:
       generate_code(expr);
       return;
    }

    Type *refd_gov = ass->get_Type();
    if (is_template) {
      expr->expr = mputprintf(expr->expr, "const_cast< const %s&>(",
        refd_gov->get_genname_template(get_my_scope()).c_str() );
    } else {
      expr->expr = mputprintf(expr->expr, "const_cast< const %s&>(",
        refd_gov->get_genname_value(get_my_scope()).c_str());
    }
    if (parlist) {
      // reference without parameters to a template that has only default formal parameters.
      // if @lazy: nothing to do, it's a C++ function call just like in case of Ref_pard::generate_code()
      expr->expr = mputprintf(expr->expr, "%s(",
        ass->get_genname_from_scope(my_scope).c_str());
      parlist->generate_code_alias(expr, ass->get_FormalParList(),
          ass->get_RunsOnType(), false);
      expr->expr = mputc(expr->expr, ')');
    } else {
      expr->expr = mputstr(expr->expr,
        LazyFuzzyParamData::in_lazy_or_fuzzy() ?
        LazyFuzzyParamData::add_ref_genname(ass, my_scope).c_str() :
        ass->get_genname_from_scope(my_scope).c_str());
    }
    expr->expr = mputstr(expr->expr, ")");

    if (t_subrefs && t_subrefs->get_nof_refs() > 0)
      t_subrefs->generate_code(expr, ass);
  }

  void Reference::generate_code_portref(expression_struct_t *expr,
    Scope *p_scope)
  {
    ref_usage_found();
    Common::Assignment *ass = get_refd_assignment();
    if (!ass) FATAL_ERROR("Reference::generate_code_portref()");
    expr->expr = mputstr(expr->expr,
      ass->get_genname_from_scope(p_scope).c_str());
    if (subrefs.get_nof_refs() > 0) subrefs.generate_code(expr, ass);
  }

  //FIXME quick hack
  void Reference::generate_code_ispresentbound(expression_struct_t *expr,
    bool is_template, const bool isbound)
  {
    ref_usage_found();
    Common::Assignment *ass = get_refd_assignment();
    const string& ass_id = ass->get_genname_from_scope(my_scope);
    const char *ass_id_str = ass_id.c_str();
    
    expression_struct_t t;
    Code::init_expr(&t);
    // Generate the default parameters if needed
    if (parlist) {
      parlist->generate_code_alias(&t, ass->get_FormalParList(),
        ass->get_RunsOnType(), false);
    }
    string ass_id2 = ass_id;
    if (t.expr != NULL) {
      ass_id2 = ass_id2 + "(" + t.expr + ")";
      ass_id_str = ass_id2.c_str();
    }
    Code::free_expr(&t);
    
    if (subrefs.get_nof_refs() > 0) {
      const string& tmp_generalid = my_scope->get_scope_mod_gen()
          ->get_temporary_id();
      const char *tmp_generalid_str = tmp_generalid.c_str();
      
      expression_struct isbound_expr;
      Code::init_expr(&isbound_expr);
      isbound_expr.preamble = mputprintf(isbound_expr.preamble,
          "boolean %s = %s.is_bound();\n", tmp_generalid_str,
          ass_id_str);
      ass->get_Type()->generate_code_ispresentbound(&isbound_expr, &subrefs, my_scope->get_scope_mod_gen(),
          tmp_generalid, ass_id2, is_template, isbound);

      expr->preamble = mputstr(expr->preamble, isbound_expr.preamble);
      expr->preamble = mputstr(expr->preamble, isbound_expr.expr);
      Code::free_expr(&isbound_expr);

      expr->expr = mputprintf(expr->expr, "%s", tmp_generalid_str);
    } else {
      expr->expr = mputprintf(expr->expr, "%s.%s(%s)", ass_id_str,
        isbound ? "is_bound":"is_present",
        (!isbound && is_template && omit_in_value_list) ? "TRUE" : "");
    }
  }

  void Reference::detect_modid()
  {
    // do nothing if detection is already performed
    if (id) return;
    // the first element of subrefs must be an <id>
    const Identifier *first_id = subrefs.get_ref(0)->get_id(), *second_id = 0;
    if (subrefs.get_nof_refs() > 1) {
      FieldOrArrayRef *second_ref = subrefs.get_ref(1);
      if (second_ref->get_type() == FieldOrArrayRef::FIELD_REF) {
        // the reference begins with <id>.<id> (most complicated case)
        // there are 3 possible situations:
        // 1. first_id points to a local definition (this has the priority)
        //      modid: 0, id: first_id
        // 2. first_id points to an imported module (trivial case)
        //      modid: first_id, id: second_id
        // 3. none of the above (first_id might be an imported symbol)
        //      modid: 0, id: first_id
        // Note: Rule 1 has the priority because it can be overridden using
        // the notation <id>.objid { ... }.<id> (modid and id are set in the
        // constructor), but there is no work-around in the reverse way.
        if (!my_scope->has_ass_withId(*first_id)
            && my_scope->is_valid_moduleid(*first_id)) {
          // rule 1 is not fulfilled, but rule 2 is fulfilled
          second_id = second_ref->get_id();
        }
      } // else: the reference begins with <id>[<arrayref>] -> there is no modid
    } // else: the reference consists of a single <id>  -> there is no modid
    if (second_id) {
      modid = first_id->clone();
      id = second_id->clone();
      subrefs.remove_refs(2);
    } else {
      modid = 0;
      id = first_id->clone();
      subrefs.remove_refs(1);
    }
  }

  // =================================
  // ===== Ref_pard
  // =================================

  Ref_pard::Ref_pard(const Ref_pard& p)
    : Ref_base(p), parlist(p.parlist), expr_cache(0)
  {
    params = p.params ? p.params->clone() : 0;
  }

  Ref_pard::Ref_pard(Identifier *p_modid, Identifier *p_id,
    ParsedActualParameters *p_params)
    : Ref_base(p_modid, p_id), parlist(), params(p_params), expr_cache(0)
  {
    if (!p_params)
      FATAL_ERROR("Ttcn::Ref_pard::Ref_pard(): NULL parameter");
  }

  Ref_pard::~Ref_pard()
  {
    delete params;
    Free(expr_cache);
  }

  Ref_pard *Ref_pard::clone() const
  {
    return new Ref_pard(*this);
  }

  void Ref_pard::set_fullname(const string& p_fullname)
  {
    Ref_base::set_fullname(p_fullname);
    parlist.set_fullname(p_fullname);
    if (params) params->set_fullname(p_fullname);
  }

  void Ref_pard::set_my_scope(Scope *p_scope)
  {
    Ref_base::set_my_scope(p_scope);
    parlist.set_my_scope(p_scope);
    if (params) params->set_my_scope(p_scope);
  }

  string Ref_pard::get_dispname()
  {
    if (is_erroneous) return string("erroneous");
    string ret_val;
    if (modid) {
      ret_val += modid->get_dispname();
      ret_val += '.';
    }
    ret_val += id->get_dispname();
    ret_val += '(';
    if (params_checked) {
      // used after semantic analysis
      for (size_t i = 0; i < parlist.get_nof_pars(); i++) {
        if (i > 0) ret_val += ", ";
        parlist.get_par(i)->append_stringRepr(ret_val);
      }
    } else {
      // used before semantic analysis
      for (size_t i = 0; i < params->get_nof_tis(); i++) {
        if (i > 0) ret_val += ", ";
        params->get_ti_byIndex(i)->append_stringRepr(ret_val);
      }
    }
    ret_val += ')';
    subrefs.append_stringRepr(ret_val);
    return ret_val;
  }

  Common::Assignment* Ref_pard::get_refd_assignment(bool check_parlist)
  {
    Common::Assignment *ass = Ref_base::get_refd_assignment(check_parlist);
    if (ass && check_parlist && !params_checked) {
      params_checked = true;
      FormalParList *fplist = ass->get_FormalParList();
      if (fplist) {
        Error_Context cntxt(params, "In actual parameter list of %s",
          ass->get_description().c_str());
        is_erroneous = fplist->fold_named_and_chk(params, &parlist);
        parlist.set_fullname(get_fullname());
        parlist.set_my_scope(my_scope);
        // the parsed parameter list is no longer needed
        delete params;
        params = 0;
      } else {
        params->error("The referenced %s cannot have actual parameters",
          ass->get_description().c_str());
      }
    }
    return ass;
  }

  const Identifier* Ref_pard::get_modid()
  {
    return modid;
  }

  const Identifier* Ref_pard::get_id()
  {
    return id;
  }

  ActualParList *Ref_pard::get_parlist()
  {
    if (!params_checked) FATAL_ERROR("Ref_pard::get_parlist()");
    return &parlist;
  }

  bool Ref_pard::chk_activate_argument()
  {
    Common::Assignment *t_ass = get_refd_assignment();
    if (!t_ass) return false;
    if (t_ass->get_asstype() != Common::Assignment::A_ALTSTEP) {
      error("Reference to an altstep was expected in the argument instead of "
        "%s", t_ass->get_description().c_str());
      return false;
    }
    my_scope->chk_runs_on_clause(t_ass, *this, "activate");
    // the altstep reference cannot have sub-references
    if (get_subrefs()) FATAL_ERROR("Ref_pard::chk_activate_argument()");
    FormalParList *fp_list = t_ass->get_FormalParList();
    // the altstep must have formal parameter list
    if (!fp_list) FATAL_ERROR("Ref_pard::chk_activate_argument()");
    return fp_list->chk_activate_argument(&parlist,
      t_ass->get_description().c_str());
  }

  bool Ref_pard::has_single_expr()
  {
    if (!Ref_base::has_single_expr()) return false;
    for (size_t i = 0; i < parlist.get_nof_pars(); i++)
      if (!parlist.get_par(i)->has_single_expr()) return false;
    // if any formal parameter has lazy or fuzzy evaluation 
    Common::Assignment *ass = get_refd_assignment();
    if (ass) {
      const FormalParList *fplist = ass->get_FormalParList();
      if (fplist) {
        size_t num_formal = fplist->get_nof_fps();
        for (size_t i=0; i<num_formal; ++i) {
          const FormalPar *fp = fplist->get_fp_byIndex(i);
          if (fp->get_eval_type() != NORMAL_EVAL) return false;
        }
      }
    }
    return true;
  }

  void Ref_pard::set_code_section(
    GovernedSimple::code_section_t p_code_section)
  {
    Ref_base::set_code_section(p_code_section);
    for (size_t i = 0; i < parlist.get_nof_pars(); i++)
      parlist.get_par(i)->set_code_section(p_code_section);
  }

  void Ref_pard::generate_code(expression_struct_t *expr)
  {
    Common::Assignment *ass = get_refd_assignment();
    // C++ function reference with actual parameter list
    expr->expr = mputprintf(expr->expr, "%s(",
      ass->get_genname_from_scope(my_scope).c_str());
    parlist.generate_code_alias(expr, ass->get_FormalParList(),
      ass->get_RunsOnType(),false);
    expr->expr = mputc(expr->expr, ')');
    // subreferences
    if (subrefs.get_nof_refs() > 0) subrefs.generate_code(expr, ass);
  }

  void Ref_pard::generate_code_cached(expression_struct_t *expr)
  {
    if (expr_cache) {
      expr->expr = mputstr(expr->expr, expr_cache);
    }
    else {
      generate_code(expr);
      expr_cache = mputstr(expr_cache, expr->expr);
    }
  }

  void Ref_pard::generate_code_const_ref(expression_struct_t *expr)
  {
    FieldOrArrayRefs *t_subrefs = get_subrefs();
    if (!t_subrefs || t_subrefs->get_nof_refs() == 0) {
      generate_code(expr);
      return;
    }

    Common::Assignment *ass = get_refd_assignment();
    if (!ass) FATAL_ERROR("Ref_pard::generate_code_const_ref()");

    bool is_template;
    switch (ass->get_asstype()) {
    case Common::Assignment::A_TEMPLATE:
      if (NULL == ass->get_FormalParList()) {
        // not a parameterized template
        is_template = true;
        break;
      }
      // else fall through
    case Common::Assignment::A_CONST:
    case Common::Assignment::A_EXT_CONST:
    case Common::Assignment::A_ALTSTEP:
    case Common::Assignment::A_TESTCASE:
    case Common::Assignment::A_FUNCTION:
    case Common::Assignment::A_EXT_FUNCTION:
    case Common::Assignment::A_FUNCTION_RVAL:
    case Common::Assignment::A_EXT_FUNCTION_RVAL:
    case Common::Assignment::A_FUNCTION_RTEMP:
    case Common::Assignment::A_EXT_FUNCTION_RTEMP:
      generate_code(expr);
      return;
    case Common::Assignment::A_MODULEPAR:
    case Common::Assignment::A_VAR:
    case Common::Assignment::A_PAR_VAL_IN:
    case Common::Assignment::A_PAR_VAL_OUT:
    case Common::Assignment::A_PAR_VAL_INOUT: {
      is_template = false;
      break; }
    case Common::Assignment::A_MODULEPAR_TEMP:
    case Common::Assignment::A_VAR_TEMPLATE:
    case Common::Assignment::A_PAR_TEMPL_IN:
    case Common::Assignment::A_PAR_TEMPL_OUT:
    case Common::Assignment::A_PAR_TEMPL_INOUT: {
      is_template = true;
      break; }
    default:
      is_template = false;
      break;
    }

    Type *refd_gov = ass->get_Type();
    if (!refd_gov) {
      generate_code(expr);
      return;
    }

    if (is_template) {
      expr->expr = mputprintf(expr->expr, "const_cast< const %s&>(",
        refd_gov->get_genname_template(get_my_scope()).c_str() );
    } else {
      expr->expr = mputprintf(expr->expr, "const_cast< const %s%s&>(",
        refd_gov->get_genname_value(get_my_scope()).c_str(),
          is_template ? "_template":"");
    }

    expr->expr = mputprintf(expr->expr, "%s(",
        ass->get_genname_from_scope(my_scope).c_str());
    parlist.generate_code_alias(expr, ass->get_FormalParList(),
          ass->get_RunsOnType(), false);
    expr->expr = mputstr(expr->expr, "))");

    t_subrefs->generate_code(expr, ass);
  }

  // =================================
  // ===== NameBridgingScope
  // =================================
  string NameBridgingScope::get_scopeMacro_name() const
  {
    if (scopeMacro_name.empty()) FATAL_ERROR("NameBridgingScope::get_scopeMacro_name()");
    return scopeMacro_name;
  }

  NameBridgingScope *NameBridgingScope::clone() const
  {
    FATAL_ERROR("NameBridgingScope::clone");
  }

  Common::Assignment* NameBridgingScope::get_ass_bySRef(Ref_simple *p_ref)
  {
    return get_parent_scope()->get_ass_bySRef(p_ref);
  }

  // =================================
  // ===== RunsOnScope
  // =================================

  RunsOnScope::RunsOnScope(Type *p_comptype)
    : Scope(), component_type(p_comptype)
  {
    if (!p_comptype || p_comptype->get_typetype() != Type::T_COMPONENT)
      FATAL_ERROR("RunsOnScope::RunsOnScope()");
    component_type->set_ownertype(Type::OT_RUNSON_SCOPE, this);
    component_defs = p_comptype->get_CompBody();
    set_scope_name("runs on `" + p_comptype->get_fullname() + "'");
  }

  RunsOnScope *RunsOnScope::clone() const
  {
    FATAL_ERROR("RunsOnScope::clone()");
  }

  void RunsOnScope::chk_uniq()
  {
    // do not perform this check if the component type is defined in the same
    // module as the 'runs on' clause
    if (parent_scope->get_scope_mod() == component_defs->get_scope_mod())
      return;
    size_t nof_defs = component_defs->get_nof_asss();
    for (size_t i = 0; i < nof_defs; i++) {
      Common::Assignment *comp_def = component_defs->get_ass_byIndex(i);
      const Identifier& id = comp_def->get_id();
      if (parent_scope->has_ass_withId(id)) {
        comp_def->warning("Imported component element definition `%s' hides a "
          "definition at module scope", comp_def->get_fullname().c_str());
        Reference ref(0, id.clone());
        Common::Assignment *hidden_ass = parent_scope->get_ass_bySRef(&ref);
        hidden_ass->warning("Hidden definition `%s' is here",
          hidden_ass->get_fullname().c_str());
      }

    }
  }

  RunsOnScope *RunsOnScope::get_scope_runs_on()
  {
    return this;
  }

  Common::Assignment *RunsOnScope::get_ass_bySRef(Ref_simple *p_ref)
  {
    if (!p_ref) FATAL_ERROR("Ttcn::RunsOnScope::get_ass_bySRef()");
    if (p_ref->get_modid()) return parent_scope->get_ass_bySRef(p_ref);
    else {
      const Identifier& id = *p_ref->get_id();
      if (component_defs->has_local_ass_withId(id)) {
        Common::Assignment* ass = component_defs->get_local_ass_byId(id);
        if (!ass) FATAL_ERROR("Ttcn::RunsOnScope::get_ass_bySRef()");

        if (component_defs->is_own_assignment(ass)) return ass;
        else if (ass->get_visibility() == PUBLIC) {
          return ass;
        }

        p_ref->error("The member definition `%s' in component type `%s'"
          " is not visible in this scope", id.get_dispname().c_str(),
            component_defs->get_id()->get_dispname().c_str());
        return 0;
      } else return parent_scope->get_ass_bySRef(p_ref);
    }
  }

  bool RunsOnScope::has_ass_withId(const Identifier& p_id)
  {
    return component_defs->has_ass_withId(p_id)
      || parent_scope->has_ass_withId(p_id);
  }
  
  // =================================
  // ===== PortScope
  // =================================

  PortScope::PortScope(Type *p_porttype)
    : Scope(), port_type(p_porttype)
  {
    if (!p_porttype || p_porttype->get_typetype() != Type::T_PORT)
      FATAL_ERROR("PortScope::PortScope()");
    port_type->set_ownertype(Type::OT_PORT_SCOPE, this);
    PortTypeBody* ptb = p_porttype->get_PortBody();
    if (!ptb)
      FATAL_ERROR("PortScope::PortScope()");
    vardefs = ptb->get_vardefs();
    set_scope_name("port `" + p_porttype->get_fullname() + "'");
  }

  PortScope *PortScope::clone() const
  {
    FATAL_ERROR("PortScope::clone()");
  }

  void PortScope::chk_uniq()
  {
    if (!vardefs) return;
    if (vardefs->get_nof_asss() == 0) return;
    // do not perform this check if the port type is defined in the same
    // module as the 'port' clause
    if (parent_scope->get_scope_mod() == vardefs->get_scope_mod())
      return;
    
    size_t nof_defs = vardefs->get_nof_asss();
    for (size_t i = 0; i < nof_defs; i++) {
      Common::Assignment *vardef = vardefs->get_ass_byIndex(i);
      const Identifier& id = vardef->get_id();
      if (parent_scope->has_ass_withId(id)) {
        vardef->warning("Imported port element definition `%s' hides a "
          "definition at module scope", vardef->get_fullname().c_str());
        Reference ref(0, id.clone());
        Common::Assignment *hidden_ass = parent_scope->get_ass_bySRef(&ref);
        hidden_ass->warning("Hidden definition `%s' is here",
          hidden_ass->get_fullname().c_str());
      }
    }
  }

  PortScope *PortScope::get_scope_port()
  {
    return this;
  }

  Common::Assignment *PortScope::get_ass_bySRef(Ref_simple *p_ref)
  {
    if (!p_ref) FATAL_ERROR("Ttcn::PortScope::get_ass_bySRef()");
    if (p_ref->get_modid()) return parent_scope->get_ass_bySRef(p_ref);
    else {
      const Identifier& id = *p_ref->get_id();
      if (vardefs->has_local_ass_withId(id)) {
        Common::Assignment* ass = vardefs->get_local_ass_byId(id);
        if (!ass) FATAL_ERROR("Ttcn::PortScope::get_ass_bySRef()");
        return ass;
      } else return parent_scope->get_ass_bySRef(p_ref);
    }
  }

  bool PortScope::has_ass_withId(const Identifier& p_id)
  {
    return vardefs->has_ass_withId(p_id)
      || parent_scope->has_ass_withId(p_id);
  }

  // =================================
  // ===== FriendMod
  // =================================

  FriendMod::FriendMod(Identifier *p_modid)
    : Node(), modid(p_modid), w_attrib_path(0), parentgroup(0), checked(false)
  {
    if (!p_modid) FATAL_ERROR("NULL parameter: Ttcn::FriendMod::FriendMod()");
    set_fullname("<friends>."+modid->get_dispname());
  }

  FriendMod::~FriendMod()
  {
    delete modid;

    delete w_attrib_path;
  }

  FriendMod *FriendMod::clone() const
  {
    FATAL_ERROR("Ttcn::FriendMod::clone()");
  }

  void FriendMod::set_fullname(const string& p_fullname)
  {
    if(w_attrib_path) w_attrib_path->set_fullname(p_fullname + ".<attribpath>");
  }

  void FriendMod::chk()
  {
    if (checked) return;

    Error_Context cntxt(this, "In friend module declaration");

    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
    }

    checked = true;
  }

  void FriendMod::set_with_attr(MultiWithAttrib* p_attrib)
  {
    if (w_attrib_path) FATAL_ERROR("FriendMod::set_with_attr()");
    w_attrib_path = new WithAttribPath();
    if (p_attrib && p_attrib->get_nof_elements() > 0) {
      w_attrib_path->set_with_attr(p_attrib);
    }
  }

  WithAttribPath* FriendMod::get_attrib_path()
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    return w_attrib_path;
  }

  void FriendMod::set_parent_path(WithAttribPath* p_path)
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    w_attrib_path->set_parent(p_path);
  }

  void FriendMod::set_parent_group(Group* p_group)
  {
    if(parentgroup) FATAL_ERROR("FriendMod::set_parent_group");
    parentgroup = p_group;
  }

  // =================================
  // ===== ImpMod
  // =================================

  ImpMod::ImpMod(Identifier *p_modid)
    : Node(), mod(0), my_mod(0), imptype(I_UNDEF), modid(p_modid),
      language_spec(0), is_recursive(false),
      w_attrib_path(0), parentgroup(0), visibility(PRIVATE)
  {
    if (!p_modid) FATAL_ERROR("NULL parameter: Ttcn::ImpMod::ImpMod()");
    set_fullname("<imports>." + modid->get_dispname());
  }

  ImpMod::~ImpMod()
  {
    delete modid;
    delete language_spec;

    delete w_attrib_path;
  }

  ImpMod *ImpMod::clone() const
  {
    FATAL_ERROR("No clone for you!");
  }

  void ImpMod::set_fullname(const string& p_fullname)
  {
    if(w_attrib_path) w_attrib_path->set_fullname(p_fullname + ".<attribpath>");
  }

  void ImpMod::chk()
  {
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
    }
  }

  void ImpMod::chk_imp(ReferenceChain& refch, vector<Common::Module>& moduleStack)
  {
      if (imptype == I_DEPENDENCY) {
        // these are added during semantic analysis, including their module pointer
        return;
      }
      Error_Context cntxt(this, "In import definition");

      if (!modules->has_mod_withId(*modid)) {
        error("There is no module with identifier `%s'",
            modid->get_dispname().c_str());
        return;
      }

      Common::Module *m = modules->get_mod_byId(*modid);
      if (m == NULL)
        return;

      set_mod(m);
      if (m == my_mod) {
        error("A module cannot import from itself");
        return;
      }
      chk();

      moduleStack.add(m);

      refch.mark_state();
      if (refch.exists(my_mod->get_fullname())) {
        if(my_mod->get_moduletype()!=Common::Module::MOD_ASN){  // Do not warning for circular import in ASN.1 module. It is legal
          my_mod->warning("Circular import chain is not recommended: %s",
            refch.get_dispstr(my_mod->get_fullname()).c_str());
        }
        refch.prev_state();
        return;
      } else {
        refch.add(my_mod->get_fullname());

        if (ImpMod::I_IMPORTIMPORT == imptype){
          Ttcn::Module* ttcnmodule =static_cast<Ttcn::Module*>(m);
          const Imports& imp = ttcnmodule->get_imports();

          for (size_t t = 0; t < imp.impmods_v.size(); t++) {
            const ImpMod *im = imp.impmods_v[t];
            const Identifier& im_id = im->get_modid();
            Common::Module *cm = modules->get_mod_byId(im_id); // never NULL

            refch.mark_state();
            if (PRIVATE != im->get_visibility()) {
              if (refch.exists(m->get_fullname())) {
                if(m->get_moduletype()!=Common::Module::MOD_ASN){  // Do not warn for circular import in ASN.1 module. It is legal
                  m->warning("Circular import chain is not recommended: %s",
                    refch.get_dispstr(m->get_fullname()).c_str());
                }
                refch.prev_state();
                continue;
              } else {
                refch.add(m->get_fullname());
                cm->chk_imp(refch, moduleStack);
              }
            }
            refch.prev_state();
          }
        } else {
          //refch.mark_state();
          m->chk_imp(refch, moduleStack);
          //refch.prev_state();
        }
      }
      refch.prev_state();

      size_t state=moduleStack.size();
      moduleStack.replace(state, moduleStack.size() - state);
  }

  bool ImpMod::has_imported_def(const Identifier& p_source_modid,
      const Identifier& p_id, const Location *loc) const
    {
    if (!mod) return false;
    else {
      switch (imptype) {
      case I_ALL:
      case I_IMPORTSPEC:
      {
        Common::Assignment* return_assignment = mod->importAssignment(p_source_modid, p_id);
        if (return_assignment != NULL) {
          return true;
        } else {
          return false;
        }
        break;
      }
      case I_IMPORTIMPORT:
      {
        Ttcn::Module *tm = static_cast<Ttcn::Module*>(mod); // B

        const Imports & imps = tm->get_imports();

        vector<ImpMod> tempusedImpMods;
        for (size_t i = 0, num = imps.impmods_v.size(); i < num; ++i) {
          ReferenceChain* referencechain = new ReferenceChain(this, "NEW IMPORT REFERNCECHAIN");
          Common::Assignment* return_assignment = imps.impmods_v[i]->
              get_imported_def(p_source_modid, p_id, loc, referencechain, tempusedImpMods); // C
          referencechain->reset();
            delete referencechain;

          if (return_assignment != NULL) {
            return true;
          } else {
            return false;
          }
        }
        //satisfy destructor
        tempusedImpMods.clear();

        break;
      }
      case I_DEPENDENCY:
        return false;
      default:
        FATAL_ERROR("ImpMod::get_imported_def");
      }
    }
    return false;
  }

  Common::Assignment *ImpMod::get_imported_def(
    const Identifier& p_source_modid, const Identifier& p_id,
    const Location *loc, ReferenceChain* refch,
    vector<ImpMod>& usedImpMods) const
  {
    if (!mod) return 0;
    else {

      Common::Assignment* result = NULL;

      switch (imptype) {
      case I_ALL:
      case I_IMPORTSPEC:
        result = mod->importAssignment(p_source_modid, p_id);
        if (result != NULL) {
              usedImpMods.add(const_cast<Ttcn::ImpMod*>(this));
        }
        return result;
        break;
      case I_IMPORTIMPORT:
      {
          Ttcn::Module *tm = static_cast<Ttcn::Module*>(mod);

          const Imports & imps = tm->get_imports();
          Common::Assignment* t_ass = NULL;

          for (size_t i = 0, num = imps.impmods_v.size(); i < num; ++i) {
            vector<ImpMod> tempusedImpMods;

            refch->mark_state();
            if (imps.impmods_v[i]->get_visibility() == PUBLIC) {
              t_ass = imps.impmods_v[i]->get_imported_def(p_source_modid, p_id, loc, refch, tempusedImpMods );
            }
            else if (imps.impmods_v[i]->get_visibility() == FRIEND) {
              t_ass = imps.impmods_v[i]->get_imported_def(p_source_modid, p_id, loc, refch, tempusedImpMods );
              if (t_ass != NULL){
                tempusedImpMods.add(imps.impmods_v[i]);
              }
            }
            refch->prev_state();

            if (t_ass != NULL) {
              bool visible = true;
              for (size_t j = 0; j < tempusedImpMods.size(); j++) {
                ImpMod* impmod_l = tempusedImpMods[j];
                //check whether the module is TTCN
                if (impmod_l->get_mod()->get_moduletype() == Common::Module::MOD_TTCN) {
                  // cast to ttcn module
                  Ttcn::Module *ttcn_m = static_cast<Ttcn::Module*>(impmod_l->get_mod());
                  if (!ttcn_m->is_visible(mod->get_modid(), impmod_l->get_visibility())) {
                    visible= false;
                  }
                }
              }
              if (visible) {
                for (size_t t = 0; i< tempusedImpMods.size(); i++) {
                  usedImpMods.add(tempusedImpMods[t]);
                }

                if (!result) {
                  result = t_ass;
                } else if(result != t_ass) {
                  if (loc == NULL) {
                    result = NULL;
                  } else {
                  loc->error(
                  "It is not possible to resolve the reference unambiguously"
                  ", as it can be resolved to `%s' and to `%s'",
                       result->get_fullname().c_str(), t_ass->get_fullname().c_str());
                  }
                }
              }
              t_ass = NULL;
            }
            tempusedImpMods.clear();
          }

          if (result != NULL) {
              usedImpMods.add(const_cast<Ttcn::ImpMod*>(this));
          }
          return result;
        break;
      }
      case I_DEPENDENCY:
        return NULL;
      default:
        FATAL_ERROR("ImpMod::get_imported_def");
      }
    }
  }

  void ImpMod::set_language_spec(const char *p_language_spec)
  {
    if (language_spec) FATAL_ERROR("ImpMod::set_language_spec()");
    if (p_language_spec) language_spec = new string(p_language_spec);
  }

  void ImpMod::generate_code(output_struct *target)
  {
    const char *module_name = modid->get_name().c_str();

    target->header.includes = mputprintf(target->header.includes,
      "#include \"%s.hh\"\n",
      duplicate_underscores ? module_name : modid->get_ttcnname().c_str());

    target->functions.pre_init = mputprintf(target->functions.pre_init,
        "%s%s.pre_init_module();\n", module_name,
  "::module_object");

    if (mod->get_moduletype() == Common::Module::MOD_TTCN) {
      target->functions.post_init = mputprintf(target->functions.post_init,
          "%s%s.post_init_module();\n", module_name,
    "::module_object");

    }
  }

  void ImpMod::dump(unsigned level) const
  {
    DEBUG(level, "Import from module %s",  modid->get_dispname().c_str());
    if (w_attrib_path) {
      MultiWithAttrib *attrib = w_attrib_path->get_with_attr();
      if (attrib) {
        DEBUG(level + 1, "Attributes:");
        attrib->dump(level + 2);
      }
    }
  }

  void ImpMod::set_with_attr(MultiWithAttrib* p_attrib)
  {
    if (w_attrib_path) FATAL_ERROR("ImpMod::set_with_attr()");
    w_attrib_path = new WithAttribPath();
    if (p_attrib && p_attrib->get_nof_elements() > 0) {
      w_attrib_path->set_with_attr(p_attrib);
    }
  }

  WithAttribPath* ImpMod::get_attrib_path()
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    return w_attrib_path;
  }

  void ImpMod::set_parent_path(WithAttribPath* p_path)
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    w_attrib_path->set_parent(p_path);
  }

  void ImpMod::set_parent_group(Group* p_group)
  {
    if(parentgroup) FATAL_ERROR("ImpMod::set_parent_group");
    parentgroup = p_group;
  }

  // =================================
  // ===== Imports
  // =================================

  Imports::~Imports()
  {
    for (size_t i = 0; i< impmods_v.size(); i++)
      delete impmods_v[i];
    impmods_v.clear();
  }

  Imports *Imports::clone() const
  {
    FATAL_ERROR("Ttcn::Imports::clone()");
  }

  void Imports::add_impmod(ImpMod *p_impmod)
  {
    if (!p_impmod) FATAL_ERROR("Ttcn::Imports::add_impmod()");
    if (p_impmod->get_imptype() == ImpMod::I_DEPENDENCY) {
      // this is just an extra dependency, not an actual 'import' in TTCN-3 code,
      // only insert it if it's needed
      for (size_t i = 0; i < impmods_v.size(); ++i) {
        if (p_impmod->get_mod() == impmods_v[i]->get_mod()) {
          delete p_impmod;
          return;
        }
      }
    }
    impmods_v.add(p_impmod);
    p_impmod->set_my_mod(my_mod);
  }

  void Imports::set_my_mod(Module *p_mod)
  {
    my_mod = p_mod;
    for(size_t i = 0; i < impmods_v.size(); i++)
      impmods_v[i]->set_my_mod(my_mod);
  }

  bool Imports::has_impmod_withId(const Identifier& p_id) const
  {
    for (size_t i = 0, size = impmods_v.size(); i < size; ++i) {
      const ImpMod* im = impmods_v[i];
      const Identifier& im_id = im->get_modid();
      const string& im_name = im_id.get_name();
      if (p_id.get_name() == im_name) {
        // The identifier represents a module imported in the current module
        return true;
      }
    }
    return false;
  }

  void Imports::chk_imp(ReferenceChain& refch, vector<Common::Module>& moduleStack)
  {
    if (impmods_v.size() <= 0) return;

    if (!my_mod) FATAL_ERROR("Ttcn::Imports::chk_imp()");

    checked = true;

    //Do some checks
    for(size_t n = 0; n < impmods_v.size(); n++)
    {
      impmods_v[n]->chk();
    }

    //TODO this whole thing should be moved into impmod::chk
    Identifier address_id(Identifier::ID_TTCN, string("address"));
    for (size_t n = 0; n < impmods_v.size(); n++) {
      ImpMod *im = impmods_v[n];
      im->chk_imp(refch, moduleStack);

      const Identifier& im_id = im->get_modid();
      Common::Module *m = modules->get_mod_byId(im_id);
      if (m == NULL)
        continue;
      if (m->get_gen_code()) my_mod->set_gen_code();
    } // next assignment
  }

  bool Imports::has_imported_def(const Identifier& p_id, const Location *loc) const
  {
    for (size_t n = 0; n < impmods_v.size(); n++) {
      ImpMod *im = impmods_v[n];
      bool return_bool = im->has_imported_def(my_mod->get_modid(), p_id, loc);
      if (return_bool) return true;
    }
    return false;
  }

  Common::Assignment *Imports::get_imported_def(
    const Identifier& p_source_modid, const Identifier& p_id,
    const Location *loc, ReferenceChain* refch)
  {
    vector<ImpMod> tempusedImpMods;
    Common::Assignment* result = NULL;
    for (size_t n = 0; n < impmods_v.size(); n++) {
      ImpMod *im = impmods_v[n];
      refch->mark_state();
      Common::Assignment* ass = im->get_imported_def(
          p_source_modid, p_id, loc, refch, tempusedImpMods);
      tempusedImpMods.clear();
      refch->prev_state();

      if(ass) {
        if (!result) {
          result = ass;
        } else if(result != ass && loc) {
          if(loc == NULL) {
            result = NULL;
          } else {
          loc->error(
          "It is not possible to resolve the reference unambiguously"
          ", as it can be resolved to `%s' and to `%s'",
            result->get_fullname().c_str(), ass->get_fullname().c_str());
        }
        }
      }
    }
    return result;
  }

  void Imports::get_imported_mods(Module::module_set_t& p_imported_mods) const
  {
    for (size_t i = 0; i < impmods_v.size(); i++) {
      ImpMod *im = impmods_v[i];
      Common::Module *m = im->get_mod();
      if (!m) continue;
      if (!p_imported_mods.has_key(m)) {
        p_imported_mods.add(m, 0);
        m->get_visible_mods(p_imported_mods);
      }
    }
  }

  void Imports::generate_code(output_struct *target)
  {
    target->header.includes = mputstr(target->header.includes,
      "#include <TTCN3.hh>\n");
    for (size_t i = 0; i < impmods_v.size(); i++) {
      ImpMod *im = impmods_v[i];
      Common::Module *m = im->get_mod();
      // inclusion of m's header file can be eliminated if we find another
      // imported module that imports m
      bool covered = false;
      for (size_t j = 0; j < impmods_v.size(); j++) {
        // skip over the same import definition
        if (j == i) continue;
        ImpMod *im2 = impmods_v[j];
        Common::Module *m2 = im2->get_mod();
        // a module that is equivalent to the current module due to
        // circular imports cannot be used to cover anything
        if (m2->is_visible(my_mod)) continue;
        if (m2->is_visible(m) && !m->is_visible(m2)) {
          // m2 covers m (i.e. m is visible from m2)
          // and they are not in the same import loop
          covered = true;
          break;
        }
      }
      // do not generate the #include if a covering module is found
      if (!covered) im->generate_code(target);
    }
  }

  void Imports::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  void Imports::dump(unsigned level) const
  {
    DEBUG(level, "Imports (%lu pcs.)", static_cast<unsigned long>( impmods_v.size() ));
    for (size_t i = 0; i < impmods_v.size(); i++)
      impmods_v[i]->dump(level + 1);
  }

  // =================================
  // ===== Definitions
  // =================================

  Definitions::~Definitions()
  {
    for(size_t i = 0; i < ass_v.size(); i++) delete ass_v[i];
    ass_v.clear();
    ass_m.clear();
  }

  Definitions *Definitions::clone() const
  {
    FATAL_ERROR("Definitions::clone");
  }

  void Definitions::add_ass(Definition *p_ass)
  {
    // it is too late to add a new one after it has been checked.
    if (checked || !p_ass) FATAL_ERROR("Ttcn::OtherDefinitions::add_ass()");
    ass_v.add(p_ass);
    p_ass->set_my_scope(this);
  }

  void Definitions::set_fullname(const string& p_fullname)
  {
    Common::Assignments::set_fullname(p_fullname);
    for(size_t i = 0; i < ass_v.size(); i++) {
      Definition *ass = ass_v[i];
      ass->set_fullname(p_fullname + "." + ass->get_id().get_dispname());
    }
  }

  bool Definitions::has_local_ass_withId(const Identifier& p_id)
  {
    if (!checked) chk_uniq();
    return ass_m.has_key(p_id.get_name());
  }

  Common::Assignment* Definitions::get_local_ass_byId(const Identifier& p_id)
  {
    if (!checked) chk_uniq();
    return ass_m[p_id.get_name()];
  }

  size_t Definitions::get_nof_asss()
  {
    if (!checked) chk_uniq();
    return ass_m.size();
  }

  size_t Definitions::get_nof_raw_asss()
  {
    return ass_v.size();
  }

  Common::Assignment* Definitions::get_ass_byIndex(size_t p_i)
  {
    if (!checked) chk_uniq();
    return ass_m.get_nth_elem(p_i);
  }

  Ttcn::Definition* Definitions::get_raw_ass_byIndex(size_t p_i) {
    return ass_v[p_i];
  }

  void Definitions::chk_uniq()
  {
    if (checked) return;
    ass_m.clear();
    for (size_t i = 0; i < ass_v.size(); i++) {
      Definition *ass = ass_v[i];
      const Identifier& id = ass->get_id();
      const string& name = id.get_name();
      if (ass_m.has_key(name)) {
        const char *dispname_str = id.get_dispname().c_str();
        ass->error("Duplicate definition with name `%s'", dispname_str);
        ass_m[name]->note("Previous definition of `%s' is here", dispname_str);
      } else {
        ass_m.add(name, ass);
        if (parent_scope->is_valid_moduleid(id)) {
          ass->warning("Definition with name `%s' hides a module identifier",
            id.get_dispname().c_str());
        }
      }
    }
    checked = true;
  }

  void Definitions::chk()
  {
    for (size_t i = 0; i < ass_v.size(); i++) ass_v[i]->chk();
  }

  void Definitions::chk_for()
  {
    if (checked) return;
    checked = true;
    ass_m.clear();
    // all checks are done in one iteration because
    // forward referencing is not allowed between the definitions
    for (size_t i = 0; i < ass_v.size(); i++) {
      Definition *def = ass_v[i];
      const Identifier& id = def->get_id();
      const string& name = id.get_name();
      if (ass_m.has_key(name)) {
        const char *dispname_str = id.get_dispname().c_str();
        def->error("Duplicate definition with name `%s'", dispname_str);
        ass_m[name]->note("Previous definition of `%s' is here", dispname_str);
      } else {
        ass_m.add(name, def);
        if (parent_scope) {
          if (parent_scope->has_ass_withId(id)) {
            const char *dispname_str = id.get_dispname().c_str();
            def->error("Definition with identifier `%s' is not unique in the "
              "scope hierarchy", dispname_str);
            Reference ref(0, id.clone());
            Common::Assignment *ass = parent_scope->get_ass_bySRef(&ref);
            if (!ass) FATAL_ERROR("OtherDefinitions::chk_for()");
            ass->note("Previous definition with identifier `%s' in higher "
              "scope unit is here", dispname_str);
          } else if (parent_scope->is_valid_moduleid(id)) {
            def->warning("Definition with name `%s' hides a module identifier",
              id.get_dispname().c_str());
          }
        }
      }
      def->chk();
    }
  }

  void Definitions::set_genname(const string& prefix)
  {
    for (size_t i = 0; i < ass_v.size(); i++) {
      Definition *def = ass_v[i];
      def->set_genname(prefix + def->get_id().get_name());
    }
  }

  void Definitions::generate_code(output_struct* target)
  {
    for(size_t i = 0; i < ass_v.size(); i++) ass_v[i]->generate_code(target);
  }

  void Definitions::generate_code(CodeGenHelper& cgh) {
    // FIXME: implement
    for(size_t i = 0; i < ass_v.size(); i++) {
      ass_v[i]->generate_code(cgh);
      CodeGenHelper::update_intervals(cgh.get_current_outputstruct());
    }
  }

  char* Definitions::generate_code_str(char *str)
  {
    for(size_t i=0; i<ass_v.size(); i++) {
      str = ass_v[i]->update_location_object(str);
      str=ass_v[i]->generate_code_str(str);
    }
    return str;
  }

  void Definitions::ilt_generate_code(ILT *ilt)
  {
    for(size_t i=0; i<ass_v.size(); i++)
      ass_v[i]->ilt_generate_code(ilt);
  }


  void Definitions::dump(unsigned level) const
  {
    DEBUG(level, "Definitions: (%lu pcs.)", static_cast<unsigned long>( ass_v.size() ));
    for(size_t i = 0; i < ass_v.size(); i++) ass_v[i]->dump(level + 1);
  }

  // =================================
  // ===== Group
  // =================================

  Group::Group(Identifier *p_id)
    : Node(), Location(), parent_group(0), w_attrib_path(0), id(p_id),
    checked(false)
  {
    if (!p_id) FATAL_ERROR("Group::Group()");
  }

  Group::~Group()
  {
    delete w_attrib_path;
    ass_v.clear();
    ass_m.clear();
    for (size_t i = 0; i < group_v.size(); i++) delete group_v[i];
    group_v.clear();
    group_m.clear();
    impmods_v.clear();
    friendmods_v.clear();
    delete id;
  }

  Group* Group::clone() const
  {
    FATAL_ERROR("Ttcn::Group::clone()");
  }

  void Group::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for(size_t i = 0; i < group_v.size() ; i++)
    {
      group_v[i]->set_fullname(p_fullname + ".<group " + Int2string(i) + ">");
    }
    if (w_attrib_path) w_attrib_path->set_fullname( p_fullname
      + ".<attribpath>");
  }

  void Group::add_ass(Definition* p_ass)
  {
     ass_v.add(p_ass);
     p_ass->set_parent_group(this);
  }

  void Group::add_group(Group* p_group)
  {
    group_v.add(p_group);
  }

  void Group::set_parent_group(Group* p_parent_group)
  {
    if (parent_group) FATAL_ERROR("Group::set_parent_group()");
    parent_group = p_parent_group;
  }

  void Group::set_with_attr(MultiWithAttrib* p_attrib)
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    w_attrib_path->set_with_attr(p_attrib);
  }

  WithAttribPath* Group::get_attrib_path()
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    return w_attrib_path;
  }

  void Group::set_parent_path(WithAttribPath* p_path)
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    w_attrib_path->set_parent(p_path);
  }

  void Group::chk_uniq()
  {
    if (checked) return;
    ass_m.clear();
    group_m.clear();

    for (size_t i = 0; i < ass_v.size(); i++) {
      Definition *ass = ass_v[i];
      const string& ass_name = ass->get_id().get_name();
      if (!ass_m.has_key(ass_name)) ass_m.add(ass_name, ass);
    }

    for(size_t i = 0; i < group_v.size(); i++) {
      Group *group = group_v[i];
      const Identifier& group_id = group->get_id();
      const string& group_name = group_id.get_name();
      if (ass_m.has_key(group_name)) {
        group->error("Group name `%s' clashes with a definition",
          group_id.get_dispname().c_str());
        ass_m[group_name]->note("Definition of `%s' is here",
          group_id.get_dispname().c_str());
      }
      if (group_m.has_key(group_name)) {
        group->error("Duplicate group with name `%s'",
          group_id.get_dispname().c_str());
        group_m[group_name]->note("Group `%s' is already defined here",
          group_id.get_dispname().c_str());
      } else group_m.add(group_name, group);
    }
    checked = true;
  }

  void Group::chk()
  {
    Error_Context cntxt(this, "In group `%s'", id->get_dispname().c_str());

    chk_uniq();

    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
    }

    for(size_t i = 0; i < group_v.size(); i++) group_v[i]->chk();
  }

  void Group::add_impmod(ImpMod *p_impmod)
  {
    impmods_v.add(p_impmod);
    p_impmod->set_parent_group(this);
  }

  void Group::add_friendmod(FriendMod *p_friendmod)
  {
    friendmods_v.add(p_friendmod);
    p_friendmod->set_parent_group(this);
  }

  void Group::dump(unsigned level) const
  {
    DEBUG(level, "Group: %s", id->get_dispname().c_str());
    DEBUG(level + 1, "Nested groups: (%lu pcs.)",
      static_cast<unsigned long>( group_v.size() ));
    for(size_t i = 0; i < group_v.size(); i++) group_v[i]->dump(level + 2);
    DEBUG(level + 1, "Nested definitions: (%lu pcs.)",
      static_cast<unsigned long>( ass_v.size() ));
    for(size_t i = 0; i < ass_v.size(); i++) ass_v[i]->dump(level + 2);
    DEBUG(level + 1, "Nested imports: (%lu pcs.)",
      static_cast<unsigned long>( impmods_v.size() ));
    for(size_t i = 0; i < impmods_v.size(); i++) impmods_v[i]->dump(level + 2);
    if (w_attrib_path) {
      MultiWithAttrib *attrib = w_attrib_path->get_with_attr();
      if (attrib) {
        DEBUG(level + 1, "Group Attributes:");
        attrib->dump(level + 2);
      }
    }
  }

  // =================================
  // ===== ControlPart
  // =================================

  ControlPart::ControlPart(StatementBlock* p_block)
    : Node(), Location(), block(p_block), w_attrib_path(0)
  {
    if (!p_block) FATAL_ERROR("ControlPart::ControlPart()");
  }

  ControlPart::~ControlPart()
  {
    delete block;
    delete w_attrib_path;
  }

  ControlPart* ControlPart::clone() const
  {
    FATAL_ERROR("ControlPart::clone");
  }

  void ControlPart::set_fullname(const string& p_fullname)
  {
    block->set_fullname(p_fullname);
    if(w_attrib_path) w_attrib_path->set_fullname(p_fullname + ".<attribpath>");
  }

  void ControlPart::set_my_scope(Scope *p_scope)
  {
    bridgeScope.set_parent_scope(p_scope);
    string temp("control");
    bridgeScope.set_scopeMacro_name(temp);

    block->set_my_scope(&bridgeScope);
  }

  void ControlPart::chk()
  {
    Error_Context cntxt(this, "In control part");
    block->chk();
    if (!semantic_check_only)
      block->set_code_section(GovernedSimple::CS_INLINE);
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
    }
  }

  void ControlPart::generate_code(output_struct *target, Module *my_module)
  {
    const char *module_dispname = my_module->get_modid().get_dispname().c_str();
    target->functions.control =
      create_location_object(target->functions.control, "CONTROLPART",
      module_dispname);
    target->functions.control = mputprintf(target->functions.control,
      "TTCN_Runtime::begin_controlpart(\"%s\");\n", module_dispname);
    if (debugger_active) {
      target->functions.control = mputprintf(target->functions.control,
        "charstring_list no_params = NULL_VALUE;\n"
        "TTCN3_Debug_Function debug_scope(NULL, \"control\", \"%s\", no_params, no_params, NULL);\n"
        "debug_scope.initial_snapshot();\n", module_dispname);
    }
    target->functions.control =
      block->generate_code(target->functions.control, target->header.global_vars,
      target->source.global_vars);
    target->functions.control = mputstr(target->functions.control,
      "TTCN_Runtime::end_controlpart();\n");
  }

  void ControlPart::set_with_attr(MultiWithAttrib* p_attrib)
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    w_attrib_path->set_with_attr(p_attrib);
  }

  WithAttribPath* ControlPart::get_attrib_path()
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    return w_attrib_path;
  }

  void ControlPart::set_parent_path(WithAttribPath* p_path)
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    w_attrib_path->set_parent(p_path);
    block->set_parent_path(w_attrib_path);
  }

  void ControlPart::dump(unsigned level) const
  {
    DEBUG(level, "Control part");
    block->dump(level + 1);
    if (w_attrib_path) {
      MultiWithAttrib *attrib = w_attrib_path->get_with_attr();
      if (attrib) {
        DEBUG(level + 1, "Attributes:");
        attrib->dump(level + 2);
      }
    }
  }

  // =================================
  // ===== Module
  // =================================

  Module::Module(Identifier *p_modid)
    : Common::Module(MOD_TTCN, p_modid), language_spec(0), w_attrib_path(0),
      controlpart(0)
  {
    asss = new Definitions();
    asss->set_parent_scope(this);
    imp = new Imports();
    imp->set_my_mod(this);
    //modified_encodings = true; // Assume always true for TTCN modules
  }

  Module::~Module()
  {
    delete language_spec;
    delete asss;
    for (size_t i = 0; i < group_v.size(); i++) delete group_v[i];
    group_v.clear();
    group_m.clear();
    delete imp;
    for (size_t i = 0; i < friendmods_v.size(); i++) delete friendmods_v[i];
    friendmods_v.clear();
    delete controlpart;
    for (size_t i = 0; i < runs_on_scopes.size(); i++)
      delete runs_on_scopes[i];
    runs_on_scopes.clear();
    for (size_t i = 0; i < port_scopes.size(); i++)
      delete port_scopes[i];
    port_scopes.clear();
    delete w_attrib_path;
  }

  void Module::add_group(Group* p_group)
  {
    group_v.add(p_group);
  }

  void Module::add_friendmod(FriendMod *p_friendmod)
  {
    friendmods_v.add(p_friendmod);
    p_friendmod->set_my_mod(this);
  }

  Module *Module::clone() const
  {
    FATAL_ERROR("Ttcn::Module::clone()");
  }

  Common::Assignment *Module::importAssignment(const Identifier& p_modid,
      const Identifier& p_id) const
  {
    if (asss->has_local_ass_withId(p_id)) {
      Common::Assignment* ass = asss->get_local_ass_byId(p_id);
      if (!ass) FATAL_ERROR("Ttcn::Module::importAssignment()");

      switch(ass->get_visibility()) {
      case FRIEND:
        for (size_t i = 0; i < friendmods_v.size(); i++) {
          if (friendmods_v[i]->get_modid() == p_modid) return ass;
        }
        return 0;
      case PUBLIC:
        return ass;
      case PRIVATE:
        return 0;
      default:
        FATAL_ERROR("Ttcn::Module::importAssignment()");
        return 0;
      }
    } else return 0;
  }

  void Module::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    asss->set_fullname(p_fullname);
    if (controlpart) controlpart->set_fullname(p_fullname + ".control");
    for(size_t i = 0; i < group_v.size(); i++)
    {
      group_v[i]->set_fullname(p_fullname + ".<group " + Int2string(i) + ">");
    }
    if (w_attrib_path) w_attrib_path->set_fullname(p_fullname
      + ".<attribpath>");
  }

  Common::Assignments *Module::get_scope_asss()
  {
    return asss;
  }

  bool Module::has_imported_ass_withId(const Identifier& p_id)
  {
    const Location *loc = NULL;
    for (size_t i = 0, num = imp->get_imports_size(); i < num; ++i) {
      const ImpMod* im = imp->get_impmod(i);
      //TODO use a reference instead of an identifier
      if(im->has_imported_def(*modid, p_id, loc)) return true;
    }
    return false;
  }

  Common::Assignment* Module::get_ass_bySRef(Ref_simple *p_ref)
  {
    const Identifier *r_modid = p_ref->get_modid();
    const Identifier *r_id = p_ref->get_id();
    if (r_modid) {
      // the reference contains a module name
      if (r_modid->get_name() != modid->get_name()) {
        // the reference points to another module
        bool has_impmod_with_name = false;
        Common::Assignment* result_ass = NULL;
        for (size_t i = 0, num = imp->get_imports_size(); i < num; ++i) {
          const ImpMod* im = imp->get_impmod(i);
          const Identifier& im_id = im->get_modid();
          const string& im_name = im_id.get_name();
          if (r_modid->get_name() == im_name) {
            has_impmod_with_name = true;
            vector<ImpMod> tempusedImpMods;
            ReferenceChain* referencechain = new ReferenceChain(this, "NEW IMPORT REFERNCECHAIN");
            Common::Assignment *t_ass = im->get_imported_def(*modid, *r_id, p_ref, referencechain, tempusedImpMods);
            referencechain->reset();
            delete referencechain;

            if (t_ass != NULL) {
              Ttcn::Module *ttcn_m = static_cast<Ttcn::Module*>(im->get_mod());
              if (!ttcn_m->is_visible(*modid, t_ass->get_visibility())) {
                t_ass = NULL;
              }

              if (t_ass != NULL) {
                if (result_ass == NULL) {
                  result_ass = t_ass;
                } else if(result_ass != t_ass) {
                  p_ref->error(
                  "It is not possible to resolve the reference unambiguously"
                  ", as it can be resolved to `%s' and to `%s'",
                    result_ass->get_fullname().c_str(), t_ass->get_fullname().c_str());
                }
              }
            }
            tempusedImpMods.clear();
          }
        }
        if (result_ass) return result_ass;

        if (has_impmod_with_name) {
          p_ref->error("There is no definition with name `%s' visible from "
                  "module `%s'", r_id->get_dispname().c_str(),
                  r_modid->get_dispname().c_str());
        } else {
          if (modules->has_mod_withId(*r_modid)) {
            Common::Module *m = modules->get_mod_byId(*r_modid);
            if (m->get_asss()->has_ass_withId(*r_id)) {
              p_ref->error("Definition with name `%s' is not imported from "
                  "module `%s'", r_id->get_dispname().c_str(),
                  r_modid->get_dispname().c_str());
            } else {
              p_ref->error("There is no definition with name `%s' in "
                  "module `%s'", r_id->get_dispname().c_str(),
                  r_modid->get_dispname().c_str());
            }
          } else {
            p_ref->error("There is no module with name `%s'",
                r_modid->get_dispname().c_str());
          }
          return 0;
        }
      } else {
        // the reference points to the own module
        if (asss->has_local_ass_withId(*r_id)) {
          return asss->get_local_ass_byId(*r_id);
        } else {
          p_ref->error("There is no definition with name `%s' in "
              "module `%s'", r_id->get_dispname().c_str(),
              r_modid->get_dispname().c_str());
        }
      }
    } else {
      // no module name is given in the reference
      if (asss->has_local_ass_withId(*r_id)) {
        return asss->get_local_ass_byId(*r_id);
      } else {
        // the reference was not found locally -> look at the import list
        Common::Assignment *t_result = NULL, *t_ass = NULL;
        for (size_t i = 0, num = imp->get_imports_size(); i < num; ++i) {
          const ImpMod* im = imp->get_impmod(i);

          vector<ImpMod> tempusedImpMods;
          ReferenceChain* referencechain = new ReferenceChain(this, "NEW IMPORT REFERNCECHAIN");
          t_ass = im->get_imported_def(*modid, *r_id, p_ref, referencechain, tempusedImpMods);
          referencechain->reset();

          delete referencechain;
          if (t_ass != NULL) {
            Ttcn::Module *ttcn_m = static_cast<Ttcn::Module*>(im->get_mod());
            if (!ttcn_m->is_visible(*modid, t_ass->get_visibility())) {
              t_ass = NULL;
            }

            if (t_ass != NULL) {
              if (t_result == NULL) {
                t_result = t_ass;
              } else if(t_result != t_ass) {
                p_ref->error(
                "It is not possible to resolve the reference unambiguously"
                ", as it can be resolved to `%s' and to `%s'",
                  t_result->get_fullname().c_str(), t_ass->get_fullname().c_str());
              }
            }
          }
          tempusedImpMods.clear();
        }

        if (t_result) return t_result;

        p_ref->error("There is no local or imported definition with name `%s'"
          ,r_id->get_dispname().c_str());
      }
    }
    return 0;
  }

  bool Module::is_valid_moduleid(const Identifier& p_id)
  {
    // The identifier represents the current module
    if (p_id.get_name() == modid->get_name()) return true;
    // The identifier represents a module imported in the current module
    if(imp->has_impmod_withId(p_id)) return true;
    return false;
  }

  Common::Assignments *Module::get_asss()
  {
    return asss;
  }

  bool Module::exports_sym(const Identifier&)
  {
    FATAL_ERROR("Ttcn::Module::exports_sym()");
    return true;
  }

  Type *Module::get_address_type()
  {
    Identifier address_id(Identifier::ID_TTCN, string("address"));
    // return NULL if address type is not defined
    if (!asss->has_local_ass_withId(address_id)) return 0;
    Common::Assignment *t_ass = asss->get_local_ass_byId(address_id);
    if (t_ass->get_asstype() != Common::Assignment::A_TYPE)
      FATAL_ERROR("Module::get_address_type(): address is not a type");
    return t_ass->get_Type();
  }

  void Module::chk_imp(ReferenceChain& refch, vector<Common::Module>& moduleStack)
  {
    if (imp_checked) return;
    const string& module_name = modid->get_dispname();

    Error_Context backup;
    Error_Context cntxt(this, "In TTCN-3 module `%s'", module_name.c_str());
    imp->chk_imp(refch, moduleStack);
    imp_checked = true;
    asss->chk_uniq();
    collect_visible_mods();
  }

  void Module::chk()
  {
    DEBUG(1, "Checking TTCN-3 module `%s'", modid->get_dispname().c_str());
    Error_Context cntxt(this, "In TTCN-3 module `%s'",
      modid->get_dispname().c_str());
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();

      // Check "extension" attributes in the module's "with" statement
      MultiWithAttrib *multi = w_attrib_path->get_with_attr();
      if (multi) for (size_t i = 0; i < multi->get_nof_elements(); ++i) {
        const SingleWithAttrib *single = multi->get_element(i);
        if (single->get_attribKeyword() != SingleWithAttrib::AT_EXTENSION) continue;
        // Parse the extension attribute
        // We circumvent parse_extattributes() in coding_attrib_p.y because
        // it processes all attributes in the "with" statement and
        // doesn't allow the removal on a case-by-case basis.
        extatrs = 0;
        init_coding_attrib_lex(single->get_attribSpec());
        int result = coding_attrib_parse();// 0=OK, 1=error, 2=out of memory
        cleanup_coding_attrib_lex();
        if (result != 0) {
          delete extatrs;
          extatrs = 0;
          continue;
        }

        const size_t num_parsed = extatrs->size();
        for (size_t a = 0; a < num_parsed; ++a) {
          Ttcn::ExtensionAttribute& ex = extatrs->get(0);

          switch (ex.get_type()) {
          case Ttcn::ExtensionAttribute::VERSION_TEMPLATE:
          case Ttcn::ExtensionAttribute::VERSION: {
            char* act_product_number;
            unsigned int act_suffix, act_rel, act_patch, act_build;
            char* extra_junk;
            (void)ex.get_id(act_product_number, act_suffix, act_rel, act_patch, act_build, extra_junk);

            if (release != UINT_MAX) {
              ex.error("Duplicate 'version' attribute");
            }
            else {
              product_number = mcopystr(act_product_number);
              suffix = act_suffix;
              release = act_rel;
              patch   = act_patch;
              build   = act_build;
              extra = mcopystr(extra_junk);
            }
            // Avoid propagating the attribute needlessly
            multi->delete_element(i--);
            single = 0;
            break; }

          case Ttcn::ExtensionAttribute::REQUIRES: {
            // Imports have already been checked
            char* exp_product_number;
            unsigned int exp_suffix, exp_rel, exp_patch, exp_build;
            char* exp_extra;
            Common::Identifier *req_id = ex.get_id(exp_product_number,
                exp_suffix, exp_rel, exp_patch, exp_build, exp_extra);
            // We own req_id
            if (imp->has_impmod_withId(*req_id)) {
              Common::Module* m = modules->get_mod_byId(*req_id);
              if (m->product_number == NULL && exp_product_number != NULL) {
                ex.error("Module '%s' requires module '%s' of product %s"
                  ", but it is not specified",
                    this->modid->get_dispname().c_str(), req_id->get_dispname().c_str(),
                    exp_product_number);
                multi->delete_element(i--);
                single = 0;
                break;
              } else if (exp_product_number == NULL &&
                m->product_number != NULL && strcmp(m->product_number, "") > 0){
                ex.warning("Module '%s' requires module '%s' of any product"
                  ", while it specifies '%s'",
                    this->modid->get_dispname().c_str(),
                    req_id->get_dispname().c_str(), m->product_number);
              } else if (m->product_number != NULL && exp_product_number != NULL
                  &&  0 != strcmp(m->product_number, exp_product_number)) {
                char *req_product_identifier =
                    get_product_identifier(exp_product_number,
                    exp_suffix, exp_rel, exp_patch, exp_build);
                char *mod_product_identifier =
                    get_product_identifier(m->product_number,
                    m->suffix, m->release, m->patch, m->build);

                ex.error("Module '%s' requires version %s of module"
                  " '%s', but only %s is available",
                    this->modid->get_dispname().c_str(), req_product_identifier,
                    req_id->get_dispname().c_str(), mod_product_identifier);
                Free(req_product_identifier);
                Free(mod_product_identifier);
                multi->delete_element(i--);
                single = 0;
                break;
              }
              // different suffixes are always incompatible
              // unless the special version number is used
              if (m->suffix != exp_suffix && (m->suffix != UINT_MAX)) {
                char *req_product_identifier =
                    get_product_identifier(exp_product_number,exp_suffix, exp_rel, exp_patch, exp_build);
                char *mod_product_identifier =
                    get_product_identifier(m->product_number,
                    m->suffix, m->release, m->patch, m->build);

                ex.error("Module '%s' requires version %s of module"
                  " '%s', but only %s is available",
                    this->modid->get_dispname().c_str(), req_product_identifier,
                    req_id->get_dispname().c_str(), mod_product_identifier);
                Free(req_product_identifier);
                Free(mod_product_identifier);
                multi->delete_element(i--);
                single = 0;
                break;
              }
              if ( m->release < exp_rel
                ||(m->release== exp_rel   && m->patch < exp_patch)
                ||(m->patch  == exp_patch && m->build < exp_build)) {
                char *mod_bld_str = buildstr(m->build);
                char *exp_bld_str = buildstr(exp_build);
                if (mod_bld_str==0 || exp_bld_str==0) FATAL_ERROR(
                  "Ttcn::Module::chk() invalid build number");
                ex.error("Module '%s' requires version R%u%c%s of module"
                  " '%s', but only R%u%c%s is available",
                  this->modid->get_dispname().c_str(),
                  exp_rel, eri(exp_patch), exp_bld_str,
                  req_id->get_dispname().c_str(),
                  m->release, eri(m->patch), mod_bld_str);
                Free(exp_bld_str);
                Free(mod_bld_str);
              }
            } else {
              single->error("No imported module named '%s'",
                req_id->get_dispname().c_str());
            }
            multi->delete_element(i--);
            single = 0;
            break; }

          case Ttcn::ExtensionAttribute::REQ_TITAN: {
            char* exp_product_number;
            unsigned int exp_suffix, exp_minor, exp_patch, exp_build;
            char* exp_extra;
            (void)ex.get_id(exp_product_number, exp_suffix, exp_minor, exp_patch, exp_build, exp_extra);
            if (exp_product_number != NULL && strcmp(exp_product_number,"CRL 113 200") != 0) {
              ex.error("This module needs to be compiled with TITAN, but "
                " product number %s is not TITAN"
                , exp_product_number);
            }
            if (0 == exp_suffix) {
              exp_suffix = 1; // previous version number format did not list the suffix part
            }
            // TTCN3_MAJOR is always 1
            int expected_version = exp_suffix * 1000000
              + exp_minor * 10000 + exp_patch * 100 + exp_build;
            if (expected_version > TTCN3_VERSION_MONOTONE) {
              char *exp_product_identifier =
                    get_product_identifier(exp_product_number, exp_suffix, exp_minor, exp_patch, exp_build);
              ex.error("This module needs to be compiled with TITAN version"
                " %s or higher; version %s detected"
                , exp_product_identifier, PRODUCT_NUMBER);
              Free(exp_product_identifier);
            }
            multi->delete_element(i--);
            single = 0;
            break; }
          case Ttcn::ExtensionAttribute::PRINTING: {
            ex.error("Attribute 'printing' not allowed at module level");
            multi->delete_element(i--);
            single = 0;
            break;
          }

          default:
            // Let everything else propagate into the module.
            // Extension attributes in the module's "with" statement
            // may be impractical, but not outright erroneous.
            break;
          } // switch
        } // next a
        delete extatrs;
      } // next i
    }
    chk_friends();
    chk_groups();
    asss->chk_uniq();
    asss->chk();
    if (controlpart) controlpart->chk();
    if (control_ns && !*control_ns) { // set but empty
      error("Invalid URI value for control namespace");
    }
    if (control_ns_prefix && !*control_ns_prefix) { // set but empty
      error("Empty NCName for the control namespace prefix is not allowed");
    }
    // TODO proper URI and NCName validation
  }

  void Module::chk_friends()
  {
    map<string, FriendMod> friends_m;

    for(size_t i = 0; i < friendmods_v.size(); i++)
    {
      FriendMod* temp_friend = friendmods_v[i];
      const Identifier& friend_id = temp_friend->get_modid();
      const string& friend_name = friend_id.get_name();
      if(friends_m.has_key(friend_name))
      {
        temp_friend->error("Duplicate friend module with name `%s'",
          friend_id.get_dispname().c_str());
        friends_m[friend_name]->note("Friend module `%s' is already defined here",
          friend_id.get_dispname().c_str());
      } else {
        friends_m.add(friend_name, temp_friend);
      }

      friendmods_v[i]->chk();
    }

    friends_m.clear();
  }

  /** \todo revise */
  void Module::chk_groups()
  {
    map<string,Common::Assignment> ass_m;

    for(size_t i = 0; i < asss->get_nof_asss(); i++)
    {
      Common::Assignment *temp_ass = asss->get_ass_byIndex(i);
      if(!temp_ass->get_parent_group())
      {
        const string& ass_name = temp_ass->get_id().get_name();
        if (!ass_m.has_key(ass_name)) ass_m.add(ass_name, temp_ass);
      }
    }

    for(size_t i = 0; i < group_v.size(); i++)
    {
      const Group* group = group_v[i];
      const Identifier& group_id = group->get_id();
      const string& group_name = group_id.get_name();
      if(ass_m.has_key(group_name))
      {
        group->error("Group name `%s' clashes with a definition",
          group_id.get_dispname().c_str());
        ass_m[group_name]->note("Definition of `%s' is here",
          group_id.get_dispname().c_str());
      }
      if(group_m.has_key(group_name))
      {
        group->error("Duplicate group with name `%s'",
          group_id.get_dispname().c_str());
        group_m[group_name]->note("Group `%s' is already defined here",
          group_id.get_dispname().c_str());
      }else{
        group_m.add(group_name,group_v[i]);
      }
    }

    ass_m.clear();

    for(size_t i = 0; i < group_v.size(); i++)
    {
      group_v[i]->chk();
    }
  }

  void Module::get_imported_mods(module_set_t& p_imported_mods)
  {
    imp->get_imported_mods(p_imported_mods);
  }

  void Module::generate_code_internal(CodeGenHelper& cgh) {
    imp->generate_code(cgh);
    asss->generate_code(cgh);
    if (controlpart)
      controlpart->generate_code(cgh.get_outputstruct(modid->get_ttcnname()), this);
  }

  RunsOnScope *Module::get_runs_on_scope(Type *comptype)
  {
    RunsOnScope *ret_val = new RunsOnScope(comptype);
    runs_on_scopes.add(ret_val);
    ret_val->set_parent_scope(asss);
    ret_val->chk_uniq();
    return ret_val;
  }
  
  PortScope *Module::get_port_scope(Type *porttype)
  {
    PortScope *ret_val = new PortScope(porttype);
    port_scopes.add(ret_val);
    ret_val->set_parent_scope(asss);
    ret_val->chk_uniq();
    return ret_val;
  }

  void Module::dump(unsigned level) const
  {
    DEBUG(level, "TTCN-3 module: %s", modid->get_dispname().c_str());
    level++;
    if(imp) imp->dump(level);
    if(asss) asss->dump(level);

    for(size_t i = 0; i < group_v.size(); i++)
    {
      group_v[i]->dump(level);
    }

    if(controlpart) controlpart->dump(level);

    if (w_attrib_path) {
      MultiWithAttrib *attrib = w_attrib_path->get_with_attr();
      if (attrib) {
        DEBUG(level, "Module Attributes:");
        attrib->dump(level + 1);
      }
    }
  }

  void Module::set_language_spec(const char *p_language_spec)
  {
    if (language_spec) FATAL_ERROR("Module::set_language_spec()");
    if (p_language_spec) language_spec = new string(p_language_spec);
  }

  void Module::add_ass(Definition* p_ass)
  {
    asss->add_ass(p_ass);
  }

  void Module::add_impmod(ImpMod *p_impmod)
  {
    imp->add_impmod(p_impmod);
  }

  void Module::add_controlpart(ControlPart* p_controlpart)
  {
    if (!p_controlpart || controlpart) FATAL_ERROR("Module::add_controlpart()");
    controlpart = p_controlpart;
    controlpart->set_my_scope(asss);
  }

  void Module::set_with_attr(MultiWithAttrib* p_attrib)
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    w_attrib_path->set_with_attr(p_attrib);
  }

  WithAttribPath* Module::get_attrib_path()
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    return w_attrib_path;
  }

  void Module::set_parent_path(WithAttribPath* p_path)
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    w_attrib_path->set_parent(p_path);
  }

  bool Module::is_visible(const Identifier& id, visibility_t visibility){

    if (visibility== PUBLIC) {
      return true;
    }
    if (visibility== FRIEND) {
      for (size_t i = 0; i < friendmods_v.size(); i++) {
        if (friendmods_v[i]->get_modid()  == id) {
          return true;
        }
      }
    }
    return false;
  }
  
  void Module::generate_json_schema(JSON_Tokenizer& json, map<Type*, JSON_Tokenizer>& json_refs)
  {
    // add a new property for this module
    json.put_next_token(JSON_TOKEN_NAME, modid->get_ttcnname().c_str());
    
    // add type definitions into an object
    json.put_next_token(JSON_TOKEN_OBJECT_START);
    
    // cycle through each type, generate schema segment and reference when needed
    for (size_t i = 0; i < asss->get_nof_asss(); ++i) {
      Def_Type* def = dynamic_cast<Def_Type*>(asss->get_ass_byIndex(i));
      if (def != NULL) {
        Type* t = def->get_Type();
        if (t->has_encoding(Type::CT_JSON)) {
          // insert type's schema segment
          t->generate_json_schema(json, false, false);
          
          if (json_refs_for_all_types && !json_refs.has_key(t)) {
            // create JSON schema reference for the type
            JSON_Tokenizer* json_ref = new JSON_Tokenizer;
            json_refs.add(t, json_ref);
            t->generate_json_schema_ref(*json_ref);
          }
        }
      }
    }
    
    // end of type definitions
    json.put_next_token(JSON_TOKEN_OBJECT_END);
    
    // insert function data
    for (size_t i = 0; i < asss->get_nof_asss(); ++i) {
      Def_ExtFunction* def = dynamic_cast<Def_ExtFunction*>(asss->get_ass_byIndex(i));
      if (def != NULL) {
        def->generate_json_schema_ref(json_refs);
      }
    }
  }
  
  void Module::generate_debugger_init(output_struct* output)
  {
    static boolean first = TRUE;
    // create the initializer function
    output->source.global_vars = mputprintf(output->source.global_vars,
      "\n/* Initializing the TTCN-3 debugger */\n"
      "void init_ttcn3_debugger()\n"
      "{\n"
      "%s", first ? "  ttcn3_debugger.activate();\n" : "");
    first = FALSE;
    
    // initialize global scope and variables (including imported variables)
    char* str_glob = generate_debugger_global_vars(NULL, this);
    for (size_t i = 0; i < imp->get_imports_size(); ++i) {
      str_glob = imp->get_impmod(i)->get_mod()->generate_debugger_global_vars(str_glob, this);
    }
    if (str_glob != NULL) {
      // only add the global scope if it actually has variables
      output->source.global_vars = mputprintf(output->source.global_vars,
        "  /* global variables */\n"
        "  TTCN3_Debug_Scope* global_scope = ttcn3_debugger.add_global_scope(\"%s\");\n"
        "%s",
        get_modid().get_dispname().c_str(), str_glob);
      Free(str_glob);
    }
    
    // initialize components' scopes and their variables
    for (size_t i = 0; i < asss->get_nof_asss(); ++i) {
      Def_Type* def = dynamic_cast<Def_Type*>(asss->get_ass_byIndex(i));
      if (def != NULL) {
        Type* comp_type = def->get_Type();
        if (comp_type->get_typetype() == Type::T_COMPONENT) {
          char* str_comp = NULL;
          ComponentTypeBody* comp_body = comp_type->get_CompBody();
          for (size_t j = 0; j < comp_body->get_nof_asss(); ++j) {
            str_comp = generate_code_debugger_add_var(str_comp, comp_body->get_ass_byIndex(j),
              this, comp_type->get_dispname().c_str());
          }
          if (str_comp != NULL) {
            // only add the component if it actually has variables
            output->source.global_vars = mputprintf(output->source.global_vars,
              "  /* variables of component %s */\n"
              "  TTCN3_Debug_Scope* %s_scope = ttcn3_debugger.add_component_scope(\"%s\");\n"
              "%s"
              , comp_type->get_dispname().c_str(), comp_type->get_dispname().c_str()
              , comp_type->get_dispname().c_str(), str_comp);
            Free(str_comp);
          }
        }
      }
    }
    
    // close the initializer function
    output->source.global_vars = mputstr(output->source.global_vars, "}\n");
  }
  
  char* Module::generate_debugger_global_vars(char* str, Common::Module* current_mod)
  {
    for (size_t i = 0; i < asss->get_nof_asss(); ++i) {
      Common::Assignment* ass = asss->get_ass_byIndex(i);
      switch (ass->get_asstype()) {
      case Common::Assignment::A_TEMPLATE:
        if (ass->get_FormalParList() != NULL) {
          // don't add parameterized templates, since they are functions in C++
          break;
        }
        // else fall through
      case Common::Assignment::A_CONST:
      case Common::Assignment::A_MODULEPAR:
      case Common::Assignment::A_MODULEPAR_TEMP:
        str = generate_code_debugger_add_var(str, ass, current_mod, "global");
        break;
      case Common::Assignment::A_EXT_CONST: {
        Def_ExtConst* def = dynamic_cast<Def_ExtConst*>(ass);
        if (def == NULL) {
          FATAL_ERROR("Module::generate_debugger_global_vars");
        }
        if (def->is_used()) {
          str = generate_code_debugger_add_var(str, ass, current_mod, "global");
        }
        break; }
      default:
        break;
      }
    }
    return str;
  }
  
  void Module::generate_debugger_functions(output_struct *output)
  {
    char* print_str = NULL;
    char* overwrite_str = NULL;
    for (size_t i = 0; i < asss->get_nof_asss(); ++i) {
      Def_Type* def = dynamic_cast<Def_Type*>(asss->get_ass_byIndex(i));
      if (def != NULL) {
        Type* t = def->get_Type();
        if (!t->is_ref() && t->get_typetype() != Type::T_COMPONENT &&
            t->get_typetype() != Type::T_PORT) {
          // don't generate code for subtypes
          if (t->get_typetype() != Type::T_SIGNATURE) {
            print_str = mputprintf(print_str, 
              "  %sif (!strcmp(p_var.type_name, \"%s\")) {\n"
              "    ((const %s*)ptr)->log();\n"
              "  }\n"
              , (print_str != NULL) ? "else " : ""
              , t->get_dispname().c_str(), t->get_genname_value(this).c_str());
            overwrite_str = mputprintf(overwrite_str,
              "  %sif (!strcmp(p_var.type_name, \"%s\")) {\n"
              "    ((%s*)p_var.value)->set_param(p_new_value);\n"
              "  }\n"
              , (overwrite_str != NULL) ? "else " : ""
              , t->get_dispname().c_str(), t->get_genname_value(this).c_str());
          }
          print_str = mputprintf(print_str,
            "  %sif (!strcmp(p_var.type_name, \"%s template\")) {\n"
            "    ((const %s_template*)ptr)->log();\n"
            "  }\n"
            , (print_str != NULL) ? "else " : ""
            , t->get_dispname().c_str(), t->get_genname_value(this).c_str());
          if (t->get_typetype() != Type::T_SIGNATURE) {
            overwrite_str = mputprintf(overwrite_str,
              "  %sif (!strcmp(p_var.type_name, \"%s template\")) {\n"
              "    ((%s_template*)p_var.value)->set_param(p_new_value);\n"
              "  }\n"
              , (overwrite_str != NULL) ? "else " : ""
              , t->get_dispname().c_str(), t->get_genname_value(this).c_str());
          }
        }
      }
    }
    if (print_str != NULL) {
      // don't generate an empty printing function
      output->header.class_defs = mputprintf(output->header.class_defs,
        "/* Debugger printing and overwriting functions for types declared in this module */\n\n"
        "extern CHARSTRING print_var_%s(const TTCN3_Debugger::variable_t& p_var);\n",
        get_modid().get_ttcnname().c_str());
      output->source.global_vars = mputprintf(output->source.global_vars,
        "\n/* Debugger printing function for types declared in this module */\n"
        "CHARSTRING print_var_%s(const TTCN3_Debugger::variable_t& p_var)\n"
        "{\n"
        "  const void* ptr = p_var.set_function != NULL ? p_var.value : p_var.cvalue;\n"
        "  TTCN_Logger::begin_event_log2str();\n"
        "%s"
        "  else {\n"
        "    TTCN_Logger::log_event_str(\"<unrecognized value or template>\");\n"
        "  }\n"
        "  return TTCN_Logger::end_event_log2str();\n"
        "}\n", get_modid().get_ttcnname().c_str(), print_str);
      Free(print_str);
    }
    if (overwrite_str != NULL) {
      // don't generate an empty overwriting function
      output->header.class_defs = mputprintf(output->header.class_defs,
        "extern boolean set_var_%s(TTCN3_Debugger::variable_t& p_var, Module_Param& p_new_value);\n",
        get_modid().get_ttcnname().c_str());
      output->source.global_vars = mputprintf(output->source.global_vars,
        "\n/* Debugger overwriting function for types declared in this module */\n"
        "boolean set_var_%s(TTCN3_Debugger::variable_t& p_var, Module_Param& p_new_value)\n"
        "{\n"
        "%s"
        "  else {\n"
        "    return FALSE;\n"
        "  }\n"
        "  return TRUE;\n"
        "}\n", get_modid().get_ttcnname().c_str(), overwrite_str);
      Free(overwrite_str);
    }
  }

  // =================================
  // ===== Definition
  // =================================

  string Definition::get_genname() const
  {
    if (!genname.empty()) return genname;
    else return id->get_name();
  }

  namedbool Definition::has_implicit_omit_attr() const {
    if (w_attrib_path) {
      const vector<SingleWithAttrib>& real_attribs =
        w_attrib_path->get_real_attrib();
      for (size_t in = real_attribs.size(); in > 0; in--) {
        if (SingleWithAttrib::AT_OPTIONAL ==
          real_attribs[in-1]->get_attribKeyword()) {
          if ("implicit omit" ==
            real_attribs[in-1]->get_attribSpec().get_spec()) {
            return IMPLICIT_OMIT;
          } else if ("explicit omit" ==
            real_attribs[in-1]->get_attribSpec().get_spec()) {
            return NOT_IMPLICIT_OMIT;
          }  // error reporting for other values is in chk_global_attrib
        }
      }
    }
    return NOT_IMPLICIT_OMIT;
  }

  Definition::~Definition()
  {
    delete w_attrib_path;
    delete erroneous_attrs;
  }

  void Definition::set_fullname(const string& p_fullname)
  {
    Common::Assignment::set_fullname(p_fullname);
    if (w_attrib_path) w_attrib_path->set_fullname(p_fullname + ".<attribpath>");
    if (erroneous_attrs) erroneous_attrs->set_fullname(p_fullname+".<erroneous_attributes>");
  }

  bool Definition::is_local() const
  {
    if (!my_scope) FATAL_ERROR("Definition::is_local()");
    for (Scope *scope = my_scope; scope; scope = scope->get_parent_scope()) {
      if (dynamic_cast<StatementBlock*>(scope)) return true;
    }
    return false;
  }

  bool Definition::chk_identical(Definition *)
  {
    FATAL_ERROR("Definition::chk_identical()");
    return false;
  }

  ErroneousAttributes* Definition::chk_erroneous_attr(WithAttribPath* p_attrib_path, Type* p_type,
                                                      Scope* p_scope, string p_fullname,
                                                      bool in_update_stmt)
  {
    if (!p_attrib_path) return NULL;
    const Ttcn::MultiWithAttrib* attribs = p_attrib_path->get_local_attrib();
    if (!attribs) return NULL;
    ErroneousAttributes* erroneous_attrs = NULL;
    for (size_t i = 0; i < attribs->get_nof_elements(); i++) {
      const Ttcn::SingleWithAttrib* act_attr = attribs->get_element(i);
      if (act_attr->get_attribKeyword()==Ttcn::SingleWithAttrib::AT_ERRONEOUS) {
        if (!use_runtime_2) {
          attribs->error("`erroneous' attributes can be used only with the Function Test Runtime");
          attribs->note("If you need negative testing use the -R flag when generating the makefile");
          return NULL;
        }
        size_t nof_qualifiers = act_attr->get_attribQualifiers() ? act_attr->get_attribQualifiers()->get_nof_qualifiers() : 0;
        dynamic_array<Type*> refd_type_array(nof_qualifiers); // only the qualifiers pointing to existing fields will be added to erroneous_attrs objects
        if (nof_qualifiers==0) {
          act_attr->error("At least one qualifier must be specified for the `erroneous' attribute");
        } else {
          // check if qualifiers point to existing fields
          for (size_t qi=0; qi<nof_qualifiers; qi++) {
            Qualifier* act_qual = const_cast<Qualifier*>(act_attr->get_attribQualifiers()->get_qualifier(qi));
            act_qual->set_my_scope(p_scope);
            Type* field_type = p_type->get_field_type(act_qual, Type::EXPECTED_CONSTANT);
            if (field_type) {
              dynamic_array<size_t> subrefs_array;
              dynamic_array<Type*> type_array;
              bool valid_indexes = p_type->get_subrefs_as_array(act_qual, subrefs_array, type_array);
              if (!valid_indexes) field_type = NULL;
              if (act_qual->refers_to_string_element()) {
                act_qual->error("Reference to a string element cannot be used in this context");
                field_type = NULL;
              }
            }
            refd_type_array.add(field_type);
          }
        }
        // parse the attr. spec.
        ErroneousAttributeSpec* err_attr_spec = ttcn3_parse_erroneous_attr_spec_string(
          act_attr->get_attribSpec().get_spec().c_str(), act_attr->get_attribSpec());
        if (err_attr_spec) {
          if (!erroneous_attrs) erroneous_attrs = new ErroneousAttributes(p_type);
          // attr.spec will be owned by erroneous_attrs object
          erroneous_attrs->add_spec(err_attr_spec);
          err_attr_spec->set_fullname(p_fullname);
          err_attr_spec->set_my_scope(p_scope);
          err_attr_spec->chk(in_update_stmt);
          // create qualifier - err.attr.spec. pairs
          for (size_t qi=0; qi<nof_qualifiers; qi++) {
            if (refd_type_array[qi] && (err_attr_spec->get_indicator()!=ErroneousAttributeSpec::I_INVALID)) {
              erroneous_attrs->add_pair(act_attr->get_attribQualifiers()->get_qualifier(qi), err_attr_spec);
            }
          }
        }
      }
    }
    if (erroneous_attrs) erroneous_attrs->chk();
    return erroneous_attrs;
  }

  char* Definition::generate_code_str(char *str)
  {
    FATAL_ERROR("Definition::generate_code_str()");
    return str;
  }

  void Definition::ilt_generate_code(ILT *)
  {
    FATAL_ERROR("Definition::ilt_generate_code()");
  }

  char *Definition::generate_code_init_comp(char *str, Definition *)
  {
    FATAL_ERROR("Definition::generate_code_init_comp()");
    return str;
  }

  void Definition::set_with_attr(MultiWithAttrib* p_attrib)
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    w_attrib_path->set_with_attr(p_attrib);
  }

  WithAttribPath* Definition::get_attrib_path()
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    return w_attrib_path;
  }

  void Definition::set_parent_path(WithAttribPath* p_path)
  {
    if (!w_attrib_path) w_attrib_path = new WithAttribPath();
    w_attrib_path->set_parent(p_path);
  }

  void Definition::set_parent_group(Group* p_group)
  {
    if(parentgroup) // there would be a leak!
      FATAL_ERROR("Definition::set_parent_group()");
    parentgroup = p_group;
  }

  Group* Definition::get_parent_group()
  {
    return parentgroup;
  }

  void Definition::dump_internal(unsigned level) const
  {
    DEBUG(level, "Move along, nothing to see here");
  }

  void Definition::dump(unsigned level) const
  {
    dump_internal(level);
    if (w_attrib_path) {
      MultiWithAttrib *attrib = w_attrib_path->get_with_attr();
      if (attrib) {
        DEBUG(level + 1, "Definition Attributes:");
        attrib->dump(level + 2);
      }
    }
    if (erroneous_attrs) erroneous_attrs->dump(level+1);
  }

  // =================================
  // ===== Def_Type
  // =================================

  Def_Type::Def_Type(Identifier *p_id, Type *p_type)
    : Definition(A_TYPE, p_id), type(p_type)
  {
    if(!p_type) FATAL_ERROR("Ttcn::Def_Type::Def_Type()");
    type->set_ownertype(Type::OT_TYPE_DEF, this);
  }

  Def_Type::~Def_Type()
  {
    delete type;
  }

  Def_Type *Def_Type::clone() const
  {
    FATAL_ERROR("Def_Type::clone");
  }

  void Def_Type::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    type->set_fullname(p_fullname);
  }

  void Def_Type::set_my_scope(Scope *p_scope)
  {
    bridgeScope.set_parent_scope(p_scope);
    bridgeScope.set_scopeMacro_name(id->get_dispname());

    Definition::set_my_scope(&bridgeScope);
    type->set_my_scope(&bridgeScope);

  }

  Setting *Def_Type::get_Setting()
  {
    return get_Type();
  }

  Type *Def_Type::get_Type()
  {
    chk();
    return type;
  }

  void Def_Type::chk()
  {
    if (checked) return;
    checked = true;
    Error_Context cntxt(this, "In %s definition `%s'",
      type->get_typetype() == Type::T_SIGNATURE ? "signature" : "type",
      id->get_dispname().c_str());
    type->set_genname(get_genname());
    if (!semantic_check_only && type->get_typetype() == Type::T_COMPONENT) {
      // the prefix of embedded definitions must be set before the checking
      type->get_CompBody()->set_genname(get_genname() + "_component_");
    }

    while (w_attrib_path) { // not a loop, but we can _break_ out of it
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
      if (type->get_typetype() != Type::T_ANYTYPE) break;
      // This is the anytype; it must be empty (we're about to add the fields)
      if (type->get_nof_comps() > 0) FATAL_ERROR("Def_Type::chk");

      Ttcn::ExtensionAttributes *extattrs = parse_extattributes(w_attrib_path);
      if (extattrs == 0) break; // NULL means parsing error

      size_t num_atrs = extattrs->size();
      for (size_t k = 0; k < num_atrs; ++k) {
        ExtensionAttribute &ea = extattrs->get(k);
        switch (ea.get_type()) {
        case ExtensionAttribute::ANYTYPELIST: {
          Types *anytypes = ea.get_types();
          // List of types to be converted into fields for the anytype.
          // Make sure scope is set on all types in the list.
          anytypes->set_my_scope(get_my_scope());

          // Convert the list of types into field names for the anytype
          for (size_t i=0; i < anytypes->get_nof_types(); ++i) {
            Type *t = anytypes->extract_type_byIndex(i);
            // we are now the owner of the Type.
            if (t->get_typetype()==Type::T_ERROR) { // should we give up?
              delete t;
              continue;
            }

            string field_name;
            const char* btn = Type::get_typename_builtin(t->get_typetype());
            if (btn) {
              field_name = btn;
            }
            else if (t->get_typetype() == Type::T_REFD) {
              // Extract the identifier
              Common::Reference *ref = t->get_Reference();
              Ttcn::Reference *tref = dynamic_cast<Ttcn::Reference*>(ref);
              if (!tref) FATAL_ERROR("Def_Type::chk, wrong kind of reference");
              const Common::Identifier *modid = tref->get_modid();
              if (modid) {
                ea.error("Qualified name '%s' cannot be added to the anytype",
                  tref->get_dispname().c_str());
                delete t;
                continue;
              }
              field_name = tref->get_id()->get_ttcnname();
            }
            else {
              // Can't happen here
              FATAL_ERROR("Unexpected type %d", t->get_typetype());
            }

            const string& at_field = anytype_field(field_name);
            Identifier *field_id = new Identifier(Identifier::ID_TTCN, at_field);
            CompField  *cf = new CompField(field_id, t, false, 0);
            cf->set_location(ea);
            cf->set_fullname(get_fullname());
            type->add_comp(cf);
          } // next i
          delete anytypes;
          break; }
        default:
          w_attrib_path->get_with_attr()->error("Type def can only have anytype");
          break;
        } // switch
      } // next attribute

      delete extattrs;
      break; // do not loop
    }

    // Now we can check the type
    type->chk();
    type->chk_constructor_name(*id);
    if (id->get_ttcnname() == "address") type->chk_address();
    ReferenceChain refch(type, "While checking embedded recursions");
    type->chk_recursions(refch);

    if (type->get_typetype()==Type::T_FUNCTION
      ||type->get_typetype()==Type::T_ALTSTEP
      ||type->get_typetype()==Type::T_TESTCASE) {
      // TR 922. This is a function/altstep/testcase reference.
      // Set this definition as the definition for the formal parameters.
      type->get_fat_parameters()->set_my_def(this);
    }
  }

  void Def_Type::generate_code(output_struct *target, bool)
  {
    type->generate_code(target);
    if (type->get_typetype() == Type::T_COMPONENT) {
      // the C++ equivalents of embedded component element definitions must be
      // generated from outside Type::generate_code() because the function can
      // call itself recursively and create invalid (overlapped) initializer
      // sequences
      type->get_CompBody()->generate_code(target);
    }
  }

  void Def_Type::generate_code(CodeGenHelper& cgh) {
    type->generate_code(cgh.get_outputstruct(get_Type()));
    if (type->get_typetype() == Type::T_COMPONENT) {
      // the C++ equivalents of embedded component element definitions must be
      // generated from outside Type::generate_code() because the function can
      // call itself recursively and create invalid (overlapped) initializer
      // sequences
      type->get_CompBody()->generate_code(cgh.get_current_outputstruct());
    }
    cgh.finalize_generation(get_Type());
  }


  void Def_Type::dump_internal(unsigned level) const
  {
    DEBUG(level, "Type def: %s @ %p", id->get_dispname().c_str(), static_cast<const void*>(this));
    type->dump(level + 1);
  }

  void Def_Type::set_with_attr(MultiWithAttrib* p_attrib)
  {
    if (!w_attrib_path) {
      w_attrib_path = new WithAttribPath();
      type->set_parent_path(w_attrib_path);
    }
    type->set_with_attr(p_attrib);
  }

  WithAttribPath* Def_Type::get_attrib_path()
  {
    if (!w_attrib_path) {
      w_attrib_path = new WithAttribPath();
      type->set_parent_path(w_attrib_path);
    }
    return w_attrib_path;
  }

  void Def_Type::set_parent_path(WithAttribPath* p_path)
  {
    if (!w_attrib_path) {
      w_attrib_path = new WithAttribPath();
      type->set_parent_path(w_attrib_path);
    }
    w_attrib_path->set_parent(p_path);
  }

  // =================================
  // ===== Def_Const
  // =================================

  Def_Const::Def_Const(Identifier *p_id, Type *p_type, Value *p_value)
    : Definition(A_CONST, p_id)
  {
    if (!p_type || !p_value) FATAL_ERROR("Ttcn::Def_Const::Def_Const()");
    type=p_type;
    type->set_ownertype(Type::OT_CONST_DEF, this);
    value=p_value;
    value_under_check=false;
  }

  Def_Const::~Def_Const()
  {
    delete type;
    delete value;
  }

  Def_Const *Def_Const::clone() const
  {
    FATAL_ERROR("Def_Const::clone");
  }

  void Def_Const::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    type->set_fullname(p_fullname + ".<type>");
    value->set_fullname(p_fullname);
  }

  void Def_Const::set_my_scope(Scope *p_scope)
  {
    Definition::set_my_scope(p_scope);
    type->set_my_scope(p_scope);
    value->set_my_scope(p_scope);
  }

  Setting *Def_Const::get_Setting()
  {
    return get_Value();
  }

  Type *Def_Const::get_Type()
  {
    chk();
    return type;
  }


  Value *Def_Const::get_Value()
  {
    chk();
    return value;
  }

  void Def_Const::chk()
  {
    if(checked) {
      if (value_under_check) {
        error("Circular reference in constant definition `%s'", 
          id->get_dispname().c_str());
        value_under_check = false; // only report the error once for this definition
      }
      return;
    }
    Error_Context cntxt(this, "In constant definition `%s'",
      id->get_dispname().c_str());
    type->set_genname(_T_, get_genname());
    type->chk();
    value->set_my_governor(type);
    type->chk_this_value_ref(value);
    checked=true;
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib(true);
      switch (type->get_type_refd_last()->get_typetype_ttcn3()) {
      case Type::T_SEQ_T:
      case Type::T_SET_T:
      case Type::T_CHOICE_T:
        // These types may have qualified attributes
        break;
      case Type::T_SEQOF: case Type::T_SETOF:
        break;
      default:
        w_attrib_path->chk_no_qualif();
        break;
      }
    }
    Type *t = type->get_type_refd_last();
    switch (t->get_typetype()) {
    case Type::T_PORT:
      error("Constant cannot be defined for port type `%s'",
        t->get_fullname().c_str());
      break;
    case Type::T_SIGNATURE:
      error("Constant cannot be defined for signature `%s'",
        t->get_fullname().c_str());
      break;
    default:
      value_under_check = true;
      type->chk_this_value(value, 0, Type::EXPECTED_CONSTANT, WARNING_FOR_INCOMPLETE,
        OMIT_NOT_ALLOWED, SUB_CHK, has_implicit_omit_attr());
      value_under_check = false;
      erroneous_attrs = chk_erroneous_attr(w_attrib_path, type, get_my_scope(),
        get_fullname(), false);
      if (erroneous_attrs) value->add_err_descr(NULL, erroneous_attrs->get_err_descr());
    {
      ReferenceChain refch(type, "While checking embedded recursions");
      value->chk_recursions(refch);
    }
      break;
    }
    if (!semantic_check_only) {
      value->set_genname_prefix("const_");
      value->set_genname_recursive(get_genname());
      value->set_code_section(GovernedSimple::CS_PRE_INIT);
    }
  }

  bool Def_Const::chk_identical(Definition *p_def)
  {
    chk();
    p_def->chk();
    if (p_def->get_asstype() != A_CONST) {
      const char *dispname_str = id->get_dispname().c_str();
      error("Local definition `%s' is a constant, but the definition "
        "inherited from component type `%s' is a %s", dispname_str,
        p_def->get_my_scope()->get_fullname().c_str(), p_def->get_assname());
      p_def->note("The inherited definition of `%s' is here", dispname_str);
      return false;
    }
    Def_Const *p_def_const = dynamic_cast<Def_Const*>(p_def);
    if (!p_def_const) FATAL_ERROR("Def_Const::chk_identical()");
    if (!type->is_identical(p_def_const->type)) {
      const char *dispname_str = id->get_dispname().c_str();
      type->error("Local constant `%s' has type `%s', but the constant "
        "inherited from component type `%s' has type `%s'", dispname_str,
        type->get_typename().c_str(),
        p_def_const->get_my_scope()->get_fullname().c_str(),
        p_def_const->type->get_typename().c_str());
      p_def_const->note("The inherited constant `%s' is here", dispname_str);
      return false;
    } else if (!(*value == *p_def_const->value)) {
      const char *dispname_str = id->get_dispname().c_str();
      value->error("Local constant `%s' and the constant inherited from "
        "component type `%s' have different values", dispname_str,
        p_def_const->get_my_scope()->get_fullname().c_str());
      p_def_const->note("The inherited constant `%s' is here", dispname_str);
      return false;
    } else return true;
  }

  void Def_Const::generate_code(output_struct *target, bool)
  {
    type->generate_code(target);
    const_def cdef;
    Code::init_cdef(&cdef);
    type->generate_code_object(&cdef, value);
    cdef.init = update_location_object(cdef.init);
    cdef.init = value->generate_code_init(cdef.init,
      value->get_lhs_name().c_str());
    Code::merge_cdef(target, &cdef);
    Code::free_cdef(&cdef);
  }

  void Def_Const::generate_code(Common::CodeGenHelper& cgh) {
    // constant definitions always go to its containing module
    generate_code(cgh.get_current_outputstruct());
  }

  char *Def_Const::generate_code_str(char *str)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    if (value->has_single_expr()) {
      // the value can be represented by a single C++ expression
      // the object is initialized by the constructor
      str = mputprintf(str, "%s %s(%s);\n",
        type->get_genname_value(my_scope).c_str(), genname_str,
        value->get_single_expr().c_str());
    } else {
      // use the default constructor
      str = mputprintf(str, "%s %s;\n",
        type->get_genname_value(my_scope).c_str(), genname_str);
      // the value is assigned using subsequent statements
      str = value->generate_code_init(str, genname_str);
    }
    if (debugger_active) {
      str = generate_code_debugger_add_var(str, this);
    }
    return str;
  }

  void Def_Const::ilt_generate_code(ILT *ilt)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    char*& def=ilt->get_out_def();
    char*& init=ilt->get_out_branches();
    def = mputprintf(def, "%s %s;\n", type->get_genname_value(my_scope).c_str(),
      genname_str);
    init = value->generate_code_init(init, genname_str);
  }

  char *Def_Const::generate_code_init_comp(char *str, Definition *)
  {
    /* This function actually does nothing as \a this and \a base_defn are
     * exactly the same. */
    return str;
  }

  void Def_Const::dump_internal(unsigned level) const
  {
    DEBUG(level, "Constant: %s @%p", id->get_dispname().c_str(), static_cast<const void*>(this));
    type->dump(level + 1);
    value->dump(level + 1);
  }

  // =================================
  // ===== Def_ExtConst
  // =================================

  Def_ExtConst::Def_ExtConst(Identifier *p_id, Type *p_type)
    : Definition(A_EXT_CONST, p_id)
  {
    if (!p_type) FATAL_ERROR("Ttcn::Def_ExtConst::Def_ExtConst()");
    type = p_type;
    type->set_ownertype(Type::OT_CONST_DEF, this);
    usage_found = false;
  }

  Def_ExtConst::~Def_ExtConst()
  {
    delete type;
  }

  Def_ExtConst *Def_ExtConst::clone() const
  {
    FATAL_ERROR("Def_ExtConst::clone");
  }

  void Def_ExtConst::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    type->set_fullname(p_fullname + ".<type>");
  }

  void Def_ExtConst::set_my_scope(Scope *p_scope)
  {
    Definition::set_my_scope(p_scope);
    type->set_my_scope(p_scope);
  }

  Type *Def_ExtConst::get_Type()
  {
    chk();
    return type;
  }

  void Def_ExtConst::chk()
  {
    if(checked) return;
    Error_Context cntxt(this, "In external constant definition `%s'",
      id->get_dispname().c_str());
    type->set_genname(_T_, get_genname());
    type->chk();
    checked=true;
    Type *t = type->get_type_refd_last();
    switch (t->get_typetype()) {
    case Type::T_PORT:
      error("External constant cannot be defined for port type `%s'",
        t->get_fullname().c_str());
      break;
    case Type::T_SIGNATURE:
      error("External constant cannot be defined for signature `%s'",
        t->get_fullname().c_str());
      break;
    default:
      break;
    }
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      switch (type->get_type_refd_last()->get_typetype()) {
      case Type::T_SEQ_T:
      case Type::T_SET_T:
      case Type::T_CHOICE_T:
        // These types may have qualified attributes
        break;
      case Type::T_SEQOF: case Type::T_SETOF:
        break;
      default:
        w_attrib_path->chk_no_qualif();
        break;
      }
    }
  }

  void Def_ExtConst::generate_code(output_struct *target, bool)
  {
    type->generate_code(target);
    target->header.global_vars = mputprintf(target->header.global_vars,
      "extern const %s& %s;\n", type->get_genname_value(my_scope).c_str(),
      get_genname().c_str());
  }

  void Def_ExtConst::generate_code(Common::CodeGenHelper& cgh) {
    // constant definitions always go to its containing module
    generate_code(cgh.get_current_outputstruct());
  }

  void Def_ExtConst::dump_internal(unsigned level) const
  {
    DEBUG(level, "External constant: %s @ %p", id->get_dispname().c_str(), static_cast<const void*>(this));
    type->dump(level + 1);
  }

  // =================================
  // ===== Def_Modulepar
  // =================================

  Def_Modulepar::Def_Modulepar(Identifier *p_id, Type *p_type, Value *p_defval)
    : Definition(A_MODULEPAR, p_id)
  {
    if (!p_type) FATAL_ERROR("Ttcn::Def_Modulepar::Def_Modulepar()");
    type = p_type;
    type->set_ownertype(Type::OT_MODPAR_DEF, this);
    def_value = p_defval;
  }

  Def_Modulepar::~Def_Modulepar()
  {
    delete type;
    delete def_value;
  }

  Def_Modulepar* Def_Modulepar::clone() const
  {
    FATAL_ERROR("Def_Modulepar::clone");
  }

  void Def_Modulepar::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    type->set_fullname(p_fullname + ".<type>");
    if (def_value) def_value->set_fullname(p_fullname + ".<default_value>");
  }

  void Def_Modulepar::set_my_scope(Scope *p_scope)
  {
    Definition::set_my_scope(p_scope);
    type->set_my_scope(p_scope);
    if (def_value) def_value->set_my_scope(p_scope);
  }

  Type *Def_Modulepar::get_Type()
  {
    chk();
    return type;
  }

  void Def_Modulepar::chk()
  {
    if(checked) return;
    Error_Context cntxt(this, "In module parameter definition `%s'",
      id->get_dispname().c_str());
    type->set_genname(_T_, get_genname());
    type->chk();
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      switch (type->get_type_refd_last()->get_typetype()) {
      case Type::T_SEQ_T:
      case Type::T_SET_T:
      case Type::T_CHOICE_T:
        // These types may have qualified attributes
        break;
      case Type::T_SEQOF: case Type::T_SETOF:
        break;
      default:
        w_attrib_path->chk_no_qualif();
        break;
      }
    }
    map<Type*,void> type_chain;
    map<Type::typetype_t, void> not_allowed;
    not_allowed.add(Type::T_PORT, 0);
    Type *t = type->get_type_refd_last();
    // if the type is valid the original will be returned
    Type::typetype_t tt = t->search_for_not_allowed_type(type_chain, not_allowed);
    type_chain.clear();
    not_allowed.clear();
    switch (tt) {
    case Type::T_PORT:
      error("Type of module parameter cannot be or embed port type `%s'",
        t->get_fullname().c_str());
      break;
    case Type::T_SIGNATURE:
      error("Type of module parameter cannot be signature `%s'",
        t->get_fullname().c_str());
      break;
    case Type::T_FUNCTION:
    case Type::T_ALTSTEP:
    case Type::T_TESTCASE:
      if (t->get_fat_runs_on_self()) {
        error("Type of module parameter cannot be of function reference type"
            " `%s' which has runs on self clause", t->get_fullname().c_str());
        break;
      }
    default:
#if defined(MINGW)
      checked = true;
#else
      if (def_value) {
        Error_Context cntxt2(def_value, "In default value");
        def_value->set_my_governor(type);
        type->chk_this_value_ref(def_value);
        checked = true;
        type->chk_this_value(def_value, 0, Type::EXPECTED_CONSTANT, INCOMPLETE_ALLOWED,
          OMIT_NOT_ALLOWED, SUB_CHK, has_implicit_omit_attr());
        if (!semantic_check_only) {
          def_value->set_genname_prefix("modulepar_");
          def_value->set_genname_recursive(get_genname());
          def_value->set_code_section(GovernedSimple::CS_PRE_INIT);
        }
      } else checked = true;
#endif
      break;
    }
  }

  void Def_Modulepar::generate_code(output_struct *target, bool)
  {
    type->generate_code(target);
    const_def cdef;
    Code::init_cdef(&cdef);
    const string& t_genname = get_genname();
    const char *name = t_genname.c_str();
    type->generate_code_object(&cdef, my_scope, t_genname, "modulepar_", false, false);
    if (def_value) {
      cdef.init = update_location_object(cdef.init);
      cdef.init = def_value->generate_code_init(cdef.init, def_value->get_lhs_name().c_str());
    }
    Code::merge_cdef(target, &cdef);
    Code::free_cdef(&cdef);

    if (IMPLICIT_OMIT == has_implicit_omit_attr()) {
      target->functions.post_init = mputprintf(target->functions.post_init,
        "modulepar_%s.set_implicit_omit();\n", name);
    }

    const char *dispname = id->get_dispname().c_str();
    target->functions.set_param = mputprintf(target->functions.set_param,
      "if (!strcmp(par_name, \"%s\")) {\n"
      "modulepar_%s.set_param(param);\n"
      "return TRUE;\n"
      "} else ", dispname, name);
    if (use_runtime_2) {
      target->functions.get_param = mputprintf(target->functions.get_param,
        "if (!strcmp(par_name, \"%s\")) {\n"
        "return modulepar_%s.get_param(param_name);\n"
        "} else ", dispname, name);
    }

    if (target->functions.log_param) {
      // this is not the first modulepar
      target->functions.log_param = mputprintf(target->functions.log_param,
        "TTCN_Logger::log_event_str(\", %s := \");\n", dispname);
    } else {
      // this is the first modulepar
      target->functions.log_param = mputprintf(target->functions.log_param,
        "TTCN_Logger::log_event_str(\"%s := \");\n", dispname);
    }
    target->functions.log_param = mputprintf(target->functions.log_param,
      "%s.log();\n", name);
  }

  void Def_Modulepar::generate_code(Common::CodeGenHelper& cgh) {
    // module parameter definitions always go to its containing module
    generate_code(cgh.get_current_outputstruct());
  }

  void Def_Modulepar::dump_internal(unsigned level) const
  {
    DEBUG(level, "Module parameter: %s @ %p", id->get_dispname().c_str(), static_cast<const void*>(this));
    type->dump(level + 1);    
    if (def_value) def_value->dump(level + 1);
    else DEBUG(level + 1, "No default value");
  }

  // =================================
  // ===== Def_Modulepar_Template
  // =================================

  Def_Modulepar_Template::Def_Modulepar_Template(Identifier *p_id, Type *p_type, Template *p_deftmpl)
    : Definition(A_MODULEPAR_TEMP, p_id)
  {
    if (!p_type) FATAL_ERROR("Ttcn::Def_Modulepar_Template::Def_Modulepar_Template()");
    type = p_type;
    type->set_ownertype(Type::OT_MODPAR_DEF, this);
    def_template = p_deftmpl;
  }

  Def_Modulepar_Template::~Def_Modulepar_Template()
  {
    delete type;
    delete def_template;
  }

  Def_Modulepar_Template* Def_Modulepar_Template::clone() const
  {
    FATAL_ERROR("Def_Modulepar_Template::clone");
  }

  void Def_Modulepar_Template::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    type->set_fullname(p_fullname + ".<type>");
    if (def_template) def_template->set_fullname(p_fullname + ".<default_template>");
  }

  void Def_Modulepar_Template::set_my_scope(Scope *p_scope)
  {
    Definition::set_my_scope(p_scope);
    type->set_my_scope(p_scope);
    if (def_template) def_template->set_my_scope(p_scope);
  }

  Type *Def_Modulepar_Template::get_Type()
  {
    chk();
    return type;
  }

  void Def_Modulepar_Template::chk()
  {
    if(checked) return;
    Error_Context cntxt(this, "In template module parameter definition `%s'",
      id->get_dispname().c_str());
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      switch (type->get_type_refd_last()->get_typetype()) {
      case Type::T_SEQ_T:
      case Type::T_SET_T:
      case Type::T_CHOICE_T:
        // These types may have qualified attributes
        break;
      case Type::T_SEQOF: case Type::T_SETOF:
        break;
      default:
        w_attrib_path->chk_no_qualif();
        break;
      }
    }
    type->set_genname(_T_, get_genname());
    type->chk();
    Type *t = type->get_type_refd_last();
    switch (t->get_typetype()) {
    case Type::T_PORT:
      error("Type of template module parameter cannot be port type `%s'",
        t->get_fullname().c_str());
      break;
    case Type::T_SIGNATURE:
      error("Type of template module parameter cannot be signature `%s'",
        t->get_fullname().c_str());
      break;
    case Type::T_FUNCTION:
    case Type::T_ALTSTEP:
    case Type::T_TESTCASE:
      if (t->get_fat_runs_on_self()) {
        error("Type of template module parameter cannot be of function reference type"
            " `%s' which has runs on self clause", t->get_fullname().c_str());
      }
      break;
    default:
      if (IMPLICIT_OMIT == has_implicit_omit_attr()) {
        error("Implicit omit not supported for template module parameters");
      }
#if defined(MINGW)
      checked = true;
#else
      if (def_template) {
        Error_Context cntxt2(def_template, "In default template");
        def_template->set_my_governor(type);
        def_template->flatten(false);
        if (def_template->get_templatetype() == Template::CSTR_PATTERN &&
          type->get_type_refd_last()->get_typetype() == Type::T_USTR) {
          def_template->set_templatetype(Template::USTR_PATTERN);
          def_template->get_ustr_pattern()->set_pattern_type(
            PatternString::USTR_PATTERN);
        }
        type->chk_this_template_ref(def_template);
        checked = true;
        type->chk_this_template_generic(def_template, INCOMPLETE_ALLOWED,
          OMIT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK, IMPLICIT_OMIT == has_implicit_omit_attr() ? IMPLICIT_OMIT : NOT_IMPLICIT_OMIT, 0);
        if (!semantic_check_only) {
          def_template->set_genname_prefix("modulepar_");
          def_template->set_genname_recursive(get_genname());
          def_template->set_code_section(GovernedSimple::CS_PRE_INIT);
        }
      } else checked = true;
#endif
      break;
    }
  }

  void Def_Modulepar_Template::generate_code(output_struct *target, bool)
  {
    type->generate_code(target);
    const_def cdef;
    Code::init_cdef(&cdef);
    const string& t_genname = get_genname();
    const char *name = t_genname.c_str();
    type->generate_code_object(&cdef, my_scope, t_genname, "modulepar_", true, false);
    if (def_template) {
      cdef.init = update_location_object(cdef.init);
      cdef.init = def_template->generate_code_init(cdef.init, def_template->get_lhs_name().c_str());
    }
    Code::merge_cdef(target, &cdef);
    Code::free_cdef(&cdef);

    if (IMPLICIT_OMIT == has_implicit_omit_attr()) {
      FATAL_ERROR("Def_Modulepar_Template::generate_code()");
    }

    const char *dispname = id->get_dispname().c_str();
    target->functions.set_param = mputprintf(target->functions.set_param,
      "if (!strcmp(par_name, \"%s\")) {\n"
      "modulepar_%s.set_param(param);\n"
      "return TRUE;\n"
      "} else ", dispname, name);
    target->functions.get_param = mputprintf(target->functions.get_param,
      "if (!strcmp(par_name, \"%s\")) {\n"
      "return modulepar_%s.get_param(param_name);\n"
      "} else ", dispname, name);

    if (target->functions.log_param) {
      // this is not the first modulepar
      target->functions.log_param = mputprintf(target->functions.log_param,
        "TTCN_Logger::log_event_str(\", %s := \");\n", dispname);
    } else {
      // this is the first modulepar
      target->functions.log_param = mputprintf(target->functions.log_param,
        "TTCN_Logger::log_event_str(\"%s := \");\n", dispname);
    }
    target->functions.log_param = mputprintf(target->functions.log_param,
      "%s.log();\n", name);
  }

  void Def_Modulepar_Template::generate_code(Common::CodeGenHelper& cgh) {
    // module parameter definitions always go to its containing module
    generate_code(cgh.get_current_outputstruct());
  }

  void Def_Modulepar_Template::dump_internal(unsigned level) const
  {
    DEBUG(level, "Module parameter: %s @ %p", id->get_dispname().c_str(), static_cast<const void*>(this));
    type->dump(level + 1);
    if (def_template) def_template->dump(level + 1);
    else DEBUG(level + 1, "No default template");
  }

  // =================================
  // ===== Def_Template
  // =================================

  Def_Template::Def_Template(template_restriction_t p_template_restriction,
    Identifier *p_id, Type *p_type, FormalParList *p_fpl,
    Reference *p_derived_ref, Template *p_body)
    : Definition(A_TEMPLATE, p_id), type(p_type), fp_list(p_fpl),
    derived_ref(p_derived_ref), base_template(0), recurs_deriv_checked(false),
    body(p_body), template_restriction(p_template_restriction),
    gen_restriction_check(false)
  {
    if (!p_type || !p_body) FATAL_ERROR("Ttcn::Def_Template::Def_Template()");
    type->set_ownertype(Type::OT_TEMPLATE_DEF, this);
    if (fp_list) fp_list->set_my_def(this);
  }

  Def_Template::~Def_Template()
  {
    delete type;
    delete fp_list;
    delete derived_ref;
    delete body;
  }

  Def_Template *Def_Template::clone() const
  {
    FATAL_ERROR("Def_Template::clone");
  }

  void Def_Template::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    type->set_fullname(p_fullname + ".<type>");
    if (fp_list) fp_list->set_fullname(p_fullname + ".<formal_par_list>");
    if (derived_ref)
      derived_ref->set_fullname(p_fullname + ".<derived_reference>");
    body->set_fullname(p_fullname);
  }

  void Def_Template::set_my_scope(Scope *p_scope)
  {
    bridgeScope.set_parent_scope(p_scope);
    bridgeScope.set_scopeMacro_name(id->get_dispname());

    Definition::set_my_scope(&bridgeScope);
    type->set_my_scope(&bridgeScope);
    if (derived_ref) derived_ref->set_my_scope(&bridgeScope);
    if (fp_list) {
      fp_list->set_my_scope(&bridgeScope);
      body->set_my_scope(fp_list);
    } else body->set_my_scope(&bridgeScope);
  }

  Setting *Def_Template::get_Setting()
  {
    return get_Template();
  }

  Type *Def_Template::get_Type()
  {
    if (!checked) chk();
    return type;
  }

  Template *Def_Template::get_Template()
  {
    if (!checked) chk();
    return body;
  }

  FormalParList *Def_Template::get_FormalParList()
  {
    if (!checked) chk();
    return fp_list;
  }

  void Def_Template::chk()
  {
    if (checked) return;
    Error_Context cntxt(this, "In template definition `%s'",
      id->get_dispname().c_str());
    const string& t_genname = get_genname();
    type->set_genname(_T_, t_genname);
    type->chk();
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib(true);
      switch (type->get_type_refd_last()->get_typetype_ttcn3()) {
      case Type::T_SEQ_T:
      case Type::T_SET_T:
      case Type::T_CHOICE_T:
        // These types may have qualified attributes
        break;
      case Type::T_SEQOF: case Type::T_SETOF:
        break;
      default:
        w_attrib_path->chk_no_qualif();
        break;
      }
    }
    if (fp_list) {
      chk_default();
      fp_list->chk(asstype);
      if (local_scope) error("Parameterized local template `%s' not supported",
        id->get_dispname().c_str());
    }

    // Merge the elements of "all from" into the list
    body->flatten(false);

    body->set_my_governor(type);

    if (body->get_templatetype() == Template::CSTR_PATTERN &&
      type->get_type_refd_last()->get_typetype() == Type::T_USTR) {
      body->set_templatetype(Template::USTR_PATTERN);
      body->get_ustr_pattern()->set_pattern_type(PatternString::USTR_PATTERN);
    }

    type->chk_this_template_ref(body);
    checked = true;
    Type *t = type->get_type_refd_last();
    if (t->get_typetype() == Type::T_PORT) {
      error("Template cannot be defined for port type `%s'",
        t->get_fullname().c_str());
    }
    chk_modified();
    chk_recursive_derivation();
    type->chk_this_template_generic(body,
      derived_ref != NULL ? INCOMPLETE_ALLOWED : WARNING_FOR_INCOMPLETE,
      OMIT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK,
      IMPLICIT_OMIT == has_implicit_omit_attr() ? IMPLICIT_OMIT : NOT_IMPLICIT_OMIT, 0);

    erroneous_attrs = chk_erroneous_attr(w_attrib_path, type, get_my_scope(),
      get_fullname(), false);
    if (erroneous_attrs) body->add_err_descr(NULL, erroneous_attrs->get_err_descr());

    {
      ReferenceChain refch(type, "While checking embedded recursions");
      body->chk_recursions(refch);
    }
    if (template_restriction!=TR_NONE) {
      Error_Context ec(this, "While checking template restriction `%s'",
                       Template::get_restriction_name(template_restriction));
      gen_restriction_check =
        body->chk_restriction("template definition", template_restriction, body);
      if (fp_list && template_restriction!=TR_PRESENT) {
        size_t nof_fps = fp_list->get_nof_fps();
        for (size_t i=0; i<nof_fps; i++) {
          FormalPar* fp = fp_list->get_fp_byIndex(i);
          // if formal par is not template then skip restriction checking,
          // templates can have only `in' parameters
          if (fp->get_asstype()!=A_PAR_TEMPL_IN) continue;
          template_restriction_t fp_tr = fp->get_template_restriction();
          switch (template_restriction) {
          case TR_VALUE:
          case TR_OMIT:
            switch (fp_tr) {
            case TR_VALUE:
            case TR_OMIT:
              // allowed
              break;
            case TR_PRESENT:
              fp->error("Formal parameter with template restriction `%s' "
                "not allowed here", Template::get_restriction_name(fp_tr));
              break;
            case TR_NONE:
              fp->error("Formal parameter without template restriction "
                "not allowed here");
              break;
            default:
              FATAL_ERROR("Ttcn::Def_Template::chk()");
            }
            break;
          default:
            FATAL_ERROR("Ttcn::Def_Template::chk()");
          }
        }
      }
    }
    if (!semantic_check_only) {
      if (fp_list) fp_list->set_genname(t_genname);
      body->set_genname_prefix("template_");
      body->set_genname_recursive(t_genname);
      body->set_code_section(fp_list ? GovernedSimple::CS_INLINE :
        GovernedSimple::CS_POST_INIT);
    }

  }

  void Def_Template::chk_default() const
  {
    if (!fp_list) FATAL_ERROR("Def_Template::chk_default()");
    if (!derived_ref) {
      if (fp_list->has_notused_defval())
        fp_list->error("Only modified templates are allowed to use the not "
                       "used symbol (`-') as the default parameter");
      return;
    }
    Common::Assignment *ass = derived_ref->get_refd_assignment(false);
    if (!ass || ass->get_asstype() != A_TEMPLATE) return;  // Work locally.
    Def_Template *base = dynamic_cast<Def_Template *>(ass);
    if (!base) FATAL_ERROR("Def_Template::chk_default()");
    FormalParList *base_fpl = base->get_FormalParList();
    size_t nof_base_fps = base_fpl ? base_fpl->get_nof_fps() : 0;
    size_t nof_local_fps = fp_list ? fp_list->get_nof_fps() : 0;
    size_t min_fps = nof_base_fps;
    if (nof_local_fps < nof_base_fps) min_fps = nof_local_fps;
    for (size_t i = 0; i < min_fps; i++) {
      FormalPar *local_fp = fp_list->get_fp_byIndex(i);
      if (local_fp->has_notused_defval()) {
        FormalPar *base_fp = base_fpl->get_fp_byIndex(i);
        if (base_fp->has_defval()) {
          local_fp->set_defval(base_fp->get_defval());
        } else {
          local_fp->error("Not used symbol (`-') doesn't have the "
                          "corresponding default parameter in the "
                          "base template");
        }
      }
    }
    // Additional parameters in the derived template with using the not used
    // symbol.  TODO: Merge the loops.
    for (size_t i = nof_base_fps; i < nof_local_fps; i++) {
      FormalPar *local_fp = fp_list->get_fp_byIndex(i);
      if (local_fp->has_notused_defval())
        local_fp->error("Not used symbol (`-') doesn't have the "
                        "corresponding default parameter in the "
                        "base template");
    }
  }

  void Def_Template::chk_modified()
  {
    if (!derived_ref) return;
    // Do not check the (non-existent) actual parameter list of the derived
    // reference against the formal parameter list of the base template.
    // According to TTCN-3 syntax the derived reference cannot have parameters
    // even if the base template is parameterized.
    Common::Assignment *ass = derived_ref->get_refd_assignment(false);
    // Checking the existence and type compatibility of the base template.
    if (!ass) return;
    if (ass->get_asstype() != A_TEMPLATE) {
      derived_ref->error("Reference to a template was expected in the "
                         "`modifies' definition instead of %s",
                         ass->get_description().c_str());
      return;
    }
    base_template = dynamic_cast<Def_Template*>(ass);
    if (!base_template) FATAL_ERROR("Def_Template::chk_modified()");
    Type *base_type = base_template->get_Type();
    TypeCompatInfo info_base(my_scope->get_scope_mod(), type, base_type, true,
                             false, true);
    TypeChain l_chain_base;
    TypeChain r_chain_base;
    if (!type->is_compatible(base_type, &info_base, this, &l_chain_base,
                             &r_chain_base)) {
      if (info_base.is_subtype_error()) {
        type->error("%s", info_base.get_subtype_error().c_str());
      } else
      if (!info_base.is_erroneous()) {
        type->error("The modified template has different type than base "
                    "template `%s': `%s' was expected instead of `%s'",
                    ass->get_fullname().c_str(),
                    base_type->get_typename().c_str(),
                    type->get_typename().c_str());
      } else {
        // Always use the format string.
        type->error("%s", info_base.get_error_str_str().c_str());
      }
    } else {
      if (info_base.needs_conversion())
        body->set_needs_conversion();
    }

    // Checking formal parameter lists.
    FormalParList *base_fpl = base_template->get_FormalParList();
    size_t nof_base_fps = base_fpl ? base_fpl->get_nof_fps() : 0;
    size_t nof_local_fps = fp_list ? fp_list->get_nof_fps() : 0;
    size_t min_fps;
    if (nof_local_fps < nof_base_fps) {
      error("The modified template has fewer formal parameters than base "
        "template `%s': at least %lu parameter%s expected instead of %lu",
        ass->get_fullname().c_str(), static_cast<unsigned long>(nof_base_fps),
        nof_base_fps > 1 ? "s were" : " was", static_cast<unsigned long>(nof_local_fps));
      min_fps = nof_local_fps;
    } else min_fps = nof_base_fps;

    for (size_t i = 0; i < min_fps; i++) {
      FormalPar *base_fp = base_fpl->get_fp_byIndex(i);
      FormalPar *local_fp = fp_list->get_fp_byIndex(i);
      Error_Context cntxt(local_fp, "In formal parameter #%lu",
        static_cast<unsigned long>(i + 1));
      // Check for parameter kind equivalence (value or template).
      if (base_fp->get_asstype() != local_fp->get_asstype())
        local_fp->error("The kind of parameter is not the same as in base "
                        "template `%s': %s was expected instead of %s",
                        ass->get_fullname().c_str(), base_fp->get_assname(),
                        local_fp->get_assname());
      // Check for type compatibility.
      Type *base_fp_type = base_fp->get_Type();
      Type *local_fp_type = local_fp->get_Type();
      TypeCompatInfo info_par(my_scope->get_scope_mod(), base_fp_type,
                              local_fp_type, true, false);
      TypeChain l_chain_par;
      TypeChain r_chain_par;
      if (!base_fp_type->is_compatible(local_fp_type, &info_par, this,
                                       &l_chain_par, &r_chain_par)) {
        if (info_par.is_subtype_error()) {
          local_fp_type->error("%s", info_par.get_subtype_error().c_str());
        } else
        if (!info_par.is_erroneous()) {
          local_fp_type->error("The type of parameter is not the same as in "
                               "base template `%s': `%s' was expected instead "
                               "of `%s'",
                               ass->get_fullname().c_str(),
                               base_fp_type->get_typename().c_str(),
                               local_fp_type->get_typename().c_str());
         } else {
           local_fp_type->error("%s", info_par.get_error_str_str().c_str());
         }
      } else {
        if (info_par.needs_conversion())
          body->set_needs_conversion();
      }
      // Check for name equivalence.
      const Identifier& base_fp_id = base_fp->get_id();
      const Identifier& local_fp_id = local_fp->get_id();
      if (!(base_fp_id == local_fp_id))
        local_fp->error("The name of parameter is not the same as in base "
                        "template `%s': `%s' was expected instead of `%s'",
                        ass->get_fullname().c_str(),
                        base_fp_id.get_dispname().c_str(),
                        local_fp_id.get_dispname().c_str());
      // Check for restrictions: the derived must be same or more restrictive.
      if (base_fp->get_asstype()==local_fp->get_asstype() &&
          Template::is_less_restrictive(base_fp->get_template_restriction(),
            local_fp->get_template_restriction())) {
        local_fp->error("The restriction of parameter is not the same or more "
          "restrictive as in base template `%s'", ass->get_fullname().c_str());
      }
    }
    // Set the pointer to the body of base template.
    body->set_base_template(base_template->get_Template());
  }

  void Def_Template::chk_recursive_derivation()
  {
    if (recurs_deriv_checked) return;
    if (base_template) {
      ReferenceChain refch(this, "While checking the chain of base templates");
      refch.add(get_fullname());
      for (Def_Template *iter = base_template; iter; iter = iter->base_template)
      {
        if (iter->recurs_deriv_checked) break;
        else if (refch.add(iter->get_fullname()))
          iter->recurs_deriv_checked = true;
        else break;
      }
    }
    recurs_deriv_checked = true;
  }

  void Def_Template::generate_code(output_struct *target, bool)
  {
    type->generate_code(target);
    if (body->get_err_descr() != NULL && body->get_err_descr()->has_descr(NULL)) {
        target->functions.post_init = body->get_err_descr()->generate_code_init_str(
          NULL, target->functions.post_init, body->get_lhs_name());
    }
    if (fp_list) {
      // Parameterized template. Generate code for a function which returns
      // a $(genname)_template and has the appropriate parameters.
      const string& t_genname = get_genname();
      const char *template_name = t_genname.c_str();
      const char *template_dispname = id->get_dispname().c_str();
      const string& type_genname = type->get_genname_template(my_scope);
      const char *type_genname_str = type_genname.c_str();
      
      // assemble the function body first (this also determines which parameters
      // are never used)
      size_t nof_base_pars = 0;
      char* function_body = create_location_object(memptystr(), "TEMPLATE",
        template_dispname);
      if (debugger_active) {
        function_body = generate_code_debugger_function_init(function_body, this);
      }
      if (base_template) {
        // modified template
        function_body = mputprintf(function_body, "%s ret_val(%s",
          type_genname_str,
          base_template->get_genname_from_scope(my_scope).c_str());
        if (base_template->fp_list) {
          // the base template is also parameterized
          function_body = mputc(function_body, '(');
          nof_base_pars = base_template->fp_list->get_nof_fps();
          for (size_t i = 0; i < nof_base_pars; i++) {
            if (i > 0) function_body = mputstr(function_body, ", ");
            function_body = mputstr(function_body,
              fp_list->get_fp_byIndex(i)->get_id().get_name().c_str());
          }
          function_body = mputc(function_body, ')');
        }
        function_body = mputstr(function_body, ");\n");
      } else {
        // simple template
        function_body = mputprintf(function_body, "%s ret_val;\n",
          type_genname_str);
      }
      function_body = body->generate_code_init(function_body, "ret_val");
      if (template_restriction!=TR_NONE && gen_restriction_check)
        function_body = Template::generate_restriction_check_code(function_body,
                          "ret_val", template_restriction);
      if (debugger_active) {
        function_body = mputstr(function_body,
          "ttcn3_debugger.set_return_value((TTCN_Logger::begin_event_log2str(), "
          "ret_val.log(), TTCN_Logger::end_event_log2str()));\n");
      }
      if (ErroneousDescriptors::can_have_err_attribs(type)) {
        // these are always generated, not just if the template has erroneous
        // descriptors, so adding '@update' statements in other modules does not
        // require this module's code to be regenerated
        target->source.global_vars = mputprintf(target->source.global_vars,
          "Erroneous_descriptor_t* %s_err_descr_ptr = NULL;\n",
          body->get_lhs_name().c_str());
        target->header.global_vars = mputprintf(target->header.global_vars,
          "extern Erroneous_descriptor_t* %s_err_descr_ptr;\n",
          body->get_lhs_name().c_str());
        function_body = mputprintf(function_body,
          "ret_val.set_err_descr(%s_err_descr_ptr);\n",
          body->get_lhs_name().c_str());
      }
      if (body->get_err_descr() != NULL && body->get_err_descr()->has_descr(NULL)) {
        target->source.global_vars = body->get_err_descr()->generate_code_str(NULL,
          target->source.global_vars, target->header.global_vars, body->get_lhs_name());
        target->functions.post_init = mputprintf(target->functions.post_init,
          "%s_err_descr_ptr = &%s_%lu_err_descr;\n",
          body->get_lhs_name().c_str(), body->get_lhs_name().c_str(),
          static_cast<unsigned long>( body->get_err_descr()->get_descr_index(NULL) ));
      }
      function_body = mputstr(function_body, "return ret_val;\n");
      // if the template modifies a parameterized template, then the inherited
      // formal parameters must always be displayed, otherwise generate a smart
      // formal parameter list (where the names of unused parameters are omitted)
      char *formal_par_list = fp_list->generate_code(memptystr(), nof_base_pars);
      fp_list->generate_code_defval(target);
      
      target->header.function_prototypes =
        mputprintf(target->header.function_prototypes,
          "extern %s %s(%s);\n",
          type_genname_str, template_name, formal_par_list);
      target->source.function_bodies = mputprintf(target->source.function_bodies,
        "%s %s(%s)\n"
        "{\n"
        "%s"
        "}\n\n", type_genname_str, template_name, formal_par_list, function_body);
      Free(formal_par_list);
      Free(function_body);
    } else {
      // non-parameterized template
      const_def cdef;
      Code::init_cdef(&cdef);
      type->generate_code_object(&cdef, body);
      cdef.init = update_location_object(cdef.init);
      if (base_template) {
        // modified template
        if (base_template->my_scope->get_scope_mod_gen() ==
            my_scope->get_scope_mod_gen()) {
          // if the base template is in the same module its body has to be
          // initialized first
          cdef.init = base_template->body->generate_code_init(cdef.init,
            base_template->body->get_lhs_name().c_str());
        }
        if (use_runtime_2 && body->get_needs_conversion()) {
          Type *body_type = body->get_my_governor()->get_type_refd_last();
          Type *base_type = base_template->body->get_my_governor()
            ->get_type_refd_last();
          if (!body_type || !base_type)
            FATAL_ERROR("Def_Template::generate_code()");
          const string& tmp_id = body->get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          // base template initialization
          cdef.init = mputprintf(cdef.init,
            "%s %s;\n"
            "if (!%s(%s, %s)) TTCN_error(\"Values or templates of types `%s' "
            "and `%s' are not compatible at run-time\");\n"
            "%s = %s;\n",
            body_type->get_genname_template(my_scope).c_str(), tmp_id_str,
            TypeConv::get_conv_func(base_type, body_type, my_scope
            ->get_scope_mod()).c_str(), tmp_id_str, base_template
            ->get_genname_from_scope(my_scope).c_str(), base_type
            ->get_typename().c_str(), body_type->get_typename().c_str(),
            body->get_lhs_name().c_str(), tmp_id_str);
        } else {
          cdef.init = mputprintf(cdef.init, "%s = %s;\n",
            body->get_lhs_name().c_str(),
            base_template->get_genname_from_scope(my_scope).c_str());
        }
      }
      if (use_runtime_2 && TypeConv::needs_conv_refd(body))
        cdef.init = TypeConv::gen_conv_code_refd(cdef.init,
          body->get_lhs_name().c_str(), body);
      else
        cdef.init = body->generate_code_init(cdef.init,
          body->get_lhs_name().c_str());
      if (template_restriction != TR_NONE && gen_restriction_check)
        cdef.init = Template::generate_restriction_check_code(cdef.init,
          body->get_lhs_name().c_str(), template_restriction);
      if (body->get_err_descr() != NULL && body->get_err_descr()->has_descr(NULL)) {
        cdef.init = mputprintf(cdef.init, "%s.set_err_descr(&%s_%lu_err_descr);\n",
          body->get_lhs_name().c_str(), body->get_lhs_name().c_str(),
          static_cast<unsigned long>( body->get_err_descr()->get_descr_index(NULL) ));
      }
      target->header.global_vars = mputstr(target->header.global_vars,
        cdef.decl);
      target->source.global_vars = mputstr(target->source.global_vars,
        cdef.def);
      target->functions.post_init = mputstr(target->functions.post_init,
        cdef.init);
      Code::free_cdef(&cdef);
    }
  }

  void Def_Template::generate_code(Common::CodeGenHelper& cgh) {
    generate_code(cgh.get_outputstruct(this));
  }

  char *Def_Template::generate_code_str(char *str)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    const string& type_genname = type->get_genname_template(my_scope);
    const char *type_genname_str = type_genname.c_str();
    if (fp_list) {
      const char *dispname_str = id->get_dispname().c_str();
      NOTSUPP("Code generation for parameterized local template `%s'",
              dispname_str);
      str = mputprintf(str, "/* NOT SUPPORTED: template %s */\n",
                       dispname_str);
    } else {
      if (base_template) {
        // non-parameterized modified template
        if (use_runtime_2 && body->get_needs_conversion()) {
          Type *body_type = body->get_my_governor()->get_type_refd_last();
          Type *base_type = base_template->body->get_my_governor()
            ->get_type_refd_last();
          if (!body_type || !base_type)
            FATAL_ERROR("Def_Template::generate_code_str()");
          const string& tmp_id = body->get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          str = mputprintf(str,
            "%s %s;\n"
            "if (!%s(%s, %s)) TTCN_error(\"Values or templates of types `%s' "
            "and `%s' are not compatible at run-time\");\n"
            "%s %s(%s);\n",
            body_type->get_genname_template(my_scope).c_str(), tmp_id_str,
            TypeConv::get_conv_func(base_type, body_type, my_scope
            ->get_scope_mod()).c_str(), tmp_id_str, base_template
            ->get_genname_from_scope(my_scope).c_str(), base_type
            ->get_typename().c_str(), body_type->get_typename().c_str(),
            type_genname_str, genname_str, tmp_id_str);
        } else {
          // the object is initialized from the base template by the
          // constructor
          str = mputprintf(str, "%s %s(%s);\n", type_genname_str, genname_str,
            base_template->get_genname_from_scope(my_scope).c_str());
        }
        // the modified body is assigned in the subsequent statements
        str = body->generate_code_init(str, genname_str);
      } else {
        // non-parameterized non-modified template
        if (body->has_single_expr()) {
          // the object is initialized by the constructor
          str = mputprintf(str, "%s %s(%s);\n", type_genname_str,
          genname_str, body->get_single_expr(false).c_str());
          // make sure the template's code is not generated twice (TR: HU56425)
          body->set_code_generated();
        } else {
          // the default constructor is used
          str = mputprintf(str, "%s %s;\n", type_genname_str, genname_str);
          // the body is assigned in the subsequent statements
          str = body->generate_code_init(str, genname_str);
        }
      }
      if (template_restriction != TR_NONE && gen_restriction_check)
        str = Template::generate_restriction_check_code(str, genname_str,
          template_restriction);
    }
    if (debugger_active) {
      str = generate_code_debugger_add_var(str, this);
    }
    return str;
  }

  void Def_Template::ilt_generate_code(ILT *ilt)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    char*& def=ilt->get_out_def();
    char*& init=ilt->get_out_branches();
    if (fp_list) {
      const char *dispname_str = id->get_dispname().c_str();
      NOTSUPP("Code generation for parameterized local template `%s'",
        dispname_str);
      def = mputprintf(def, "/* NOT SUPPORTED: template %s */\n", dispname_str);
      init = mputprintf(init, "/* NOT SUPPORTED: template %s */\n",
        dispname_str);
    } else {
      // non-parameterized template
      // use the default constructor for initialization
      def = mputprintf(def, "%s %s;\n",
        type->get_genname_template(my_scope).c_str(), genname_str);
      if (base_template) {
        // copy the base template with an assignment
        init = mputprintf(init, "%s = %s;\n", genname_str,
          base_template->get_genname_from_scope(my_scope).c_str());
      }
      // finally assign the body
      init = body->generate_code_init(init, genname_str);
      if (template_restriction!=TR_NONE && gen_restriction_check)
        init = Template::generate_restriction_check_code(init, genname_str,
                template_restriction);
    }
  }

  void Def_Template::dump_internal(unsigned level) const
  {
    DEBUG(level, "Template: %s", id->get_dispname().c_str());
    if (fp_list) fp_list->dump(level + 1);
    if (derived_ref)
      DEBUG(level + 1, "modifies: %s", derived_ref->get_dispname().c_str());
    if (template_restriction!=TR_NONE)
      DEBUG(level + 1, "restriction: %s",
        Template::get_restriction_name(template_restriction));
    type->dump(level + 1);
    body->dump(level + 1);
  }

  // =================================
  // ===== Def_Var
  // =================================

  Def_Var::Def_Var(Identifier *p_id, Type *p_type, Value *p_initial_value)
    : Definition(A_VAR, p_id), type(p_type), initial_value(p_initial_value)
  {
    if (!p_type) FATAL_ERROR("Ttcn::Def_Var::Def_Var()");
    type->set_ownertype(Type::OT_VAR_DEF, this);
  }

  Def_Var::~Def_Var()
  {
    delete type;
    delete initial_value;
  }

  Def_Var *Def_Var::clone() const
  {
    FATAL_ERROR("Def_Var::clone");
  }

  void Def_Var::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    type->set_fullname(p_fullname + ".<type>");
    if (initial_value)
      initial_value->set_fullname(p_fullname + ".<initial_value>");
  }

  void Def_Var::set_my_scope(Scope *p_scope)
  {
    Definition::set_my_scope(p_scope);
    type->set_my_scope(p_scope);
    if (initial_value) initial_value->set_my_scope(p_scope);
  }

  Type *Def_Var::get_Type()
  {
    chk();
    return type;
  }

  void Def_Var::chk()
  {
    if(checked) return;
    Error_Context cntxt(this, "In variable definition `%s'",
      id->get_dispname().c_str());
    type->set_genname(_T_, get_genname());
    type->chk();
    checked = true;
    Type *t = type->get_type_refd_last();
    switch (t->get_typetype()) {
    case Type::T_PORT:
      error("Variable cannot be defined for port type `%s'",
        t->get_fullname().c_str());
      break;
    case Type::T_SIGNATURE:
      error("Variable cannot be defined for signature `%s'",
        t->get_fullname().c_str());
      break;
    default:
    if (initial_value) {
      initial_value->set_my_governor(type);
      type->chk_this_value_ref(initial_value);
      type->chk_this_value(initial_value, this, is_local() ?
        Type::EXPECTED_DYNAMIC_VALUE : Type::EXPECTED_STATIC_VALUE,
        INCOMPLETE_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
      if (!semantic_check_only) {
        initial_value->set_genname_recursive(get_genname());
        initial_value->set_code_section(GovernedSimple::CS_INLINE);
      }
    }
      break;
    }

    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
    }
  }

  bool Def_Var::chk_identical(Definition *p_def)
  {
    chk();
    p_def->chk();
    if (p_def->get_asstype() != A_VAR) {
      const char *dispname_str = id->get_dispname().c_str();
      error("Local definition `%s' is a variable, but the definition "
        "inherited from component type `%s' is a %s", dispname_str,
        p_def->get_my_scope()->get_fullname().c_str(), p_def->get_assname());
      p_def->note("The inherited definition of `%s' is here", dispname_str);
      return false;
    }
    Def_Var *p_def_var = dynamic_cast<Def_Var*>(p_def);
    if (!p_def_var) FATAL_ERROR("Def_Var::chk_identical()");
    if (!type->is_identical(p_def_var->type)) {
      const char *dispname_str = id->get_dispname().c_str();
      type->error("Local variable `%s' has type `%s', but the variable "
        "inherited from component type `%s' has type `%s'", dispname_str,
        type->get_typename().c_str(),
        p_def_var->get_my_scope()->get_fullname().c_str(),
        p_def_var->type->get_typename().c_str());
      p_def_var->note("The inherited variable `%s' is here", dispname_str);
      return false;
    }
    if (initial_value) {
      if (p_def_var->initial_value) {
        if (!initial_value->is_unfoldable() &&
            !p_def_var->initial_value->is_unfoldable() &&
            !(*initial_value == *p_def_var->initial_value)) {
          const char *dispname_str = id->get_dispname().c_str();
          initial_value->warning("Local variable `%s' and the variable "
            "inherited from component type `%s' have different initial values",
            dispname_str, p_def_var->get_my_scope()->get_fullname().c_str());
          p_def_var->note("The inherited variable `%s' is here", dispname_str);
        }
      } else {
        const char *dispname_str = id->get_dispname().c_str();
        initial_value->warning("Local variable `%s' has initial value, but "
          "the variable inherited from component type `%s' does not",
          dispname_str, p_def_var->get_my_scope()->get_fullname().c_str());
        p_def_var->note("The inherited variable `%s' is here", dispname_str);
      }
    } else if (p_def_var->initial_value) {
      const char *dispname_str = id->get_dispname().c_str();
      warning("Local variable `%s' does not have initial value, but the "
        "variable inherited from component type `%s' has", dispname_str,
        p_def_var->get_my_scope()->get_fullname().c_str());
      p_def_var->note("The inherited variable `%s' is here", dispname_str);
    }
    return true;
  }

  void Def_Var::generate_code(output_struct *target, bool clean_up)
  {
    type->generate_code(target);
    const_def cdef;
    Code::init_cdef(&cdef);
    type->generate_code_object(&cdef, my_scope, get_genname(), 0, false, false);
    Code::merge_cdef(target, &cdef);
    Code::free_cdef(&cdef);
    if (initial_value) {
      target->functions.init_comp =
      initial_value->generate_code_init(target->functions.init_comp,
        initial_value->get_lhs_name().c_str());
    } else if (clean_up) {  // No initial value.
      target->functions.init_comp = mputprintf(target->functions.init_comp,
        "%s.clean_up();\n", get_genname().c_str());
    }
  }

  void Def_Var::generate_code(CodeGenHelper& cgh)
  {
    generate_code(cgh.get_outputstruct(this));
  }

  char *Def_Var::generate_code_str(char *str)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    if (initial_value && initial_value->has_single_expr()) {
      // the initial value can be represented by a single C++ expression
      // the object is initialized by the constructor
      str = mputprintf(str, "%s %s(%s);\n",
        type->get_genname_value(my_scope).c_str(), genname_str,
        initial_value->get_single_expr().c_str());
    } else {
      // use the default constructor
      str = mputprintf(str, "%s %s;\n",
        type->get_genname_value(my_scope).c_str(), genname_str);
      if (initial_value) {
        // the initial value is assigned using subsequent statements
        str = initial_value->generate_code_init(str, genname_str);
      }
    }
    if (debugger_active) {
      str = generate_code_debugger_add_var(str, this);
    }
    return str;
  }

  void Def_Var::ilt_generate_code(ILT *ilt)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    char*& def=ilt->get_out_def();
    char*& init=ilt->get_out_branches();
    def = mputprintf(def, "%s %s;\n", type->get_genname_value(my_scope).c_str(),
      genname_str);
    if (initial_value)
      init = initial_value->generate_code_init(init, genname_str);
  }

  char *Def_Var::generate_code_init_comp(char *str, Definition *base_defn)
  {
    if (initial_value) {
      str = initial_value->generate_code_init(str,
        base_defn->get_genname_from_scope(my_scope).c_str());
    }
    return str;
  }

  void Def_Var::dump_internal(unsigned level) const
  {
    DEBUG(level, "Variable %s", id->get_dispname().c_str());
    type->dump(level + 1);
    if (initial_value) initial_value->dump(level + 1);
  }

  // =================================
  // ===== Def_Var_Template
  // =================================

  Def_Var_Template::Def_Var_Template(Identifier *p_id, Type *p_type,
    Template *p_initial_value, template_restriction_t p_template_restriction)
    : Definition(A_VAR_TEMPLATE, p_id), type(p_type),
    initial_value(p_initial_value), template_restriction(p_template_restriction)
  {
    if (!p_type) FATAL_ERROR("Ttcn::Def_Var_Template::Def_Var_Template()");
    type->set_ownertype(Type::OT_VARTMPL_DEF, this);
  }

  Def_Var_Template::~Def_Var_Template()
  {
    delete type;
    delete initial_value;
  }

  Def_Var_Template *Def_Var_Template::clone() const
  {
    FATAL_ERROR("Def_Var_Template::clone");
  }

  void Def_Var_Template::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    type->set_fullname(p_fullname + ".<type>");
    if (initial_value)
      initial_value->set_fullname(p_fullname + ".<initial_value>");
  }

  void Def_Var_Template::set_my_scope(Scope *p_scope)
  {
    Definition::set_my_scope(p_scope);
    type->set_my_scope(p_scope);
    if (initial_value) initial_value->set_my_scope(p_scope);
  }

  Type *Def_Var_Template::get_Type()
  {
    chk();
    return type;
  }

  void Def_Var_Template::chk()
  {
    if(checked) return;
    Error_Context cntxt(this, "In template variable definition `%s'",
      id->get_dispname().c_str());
    type->set_genname(_T_, get_genname());
    type->chk();
    checked = true;
    Type *t = type->get_type_refd_last();
    if (t->get_typetype() == Type::T_PORT) {
      error("Template variable cannot be defined for port type `%s'",
        t->get_fullname().c_str());
    }

    if (initial_value) {
      initial_value->set_my_governor(type);
      initial_value->flatten(false);

      if (initial_value->get_templatetype() == Template::CSTR_PATTERN &&
        type->get_type_refd_last()->get_typetype() == Type::T_USTR) {
        initial_value->set_templatetype(Template::USTR_PATTERN);
        initial_value->get_ustr_pattern()->set_pattern_type(
          PatternString::USTR_PATTERN);
      }

      type->chk_this_template_ref(initial_value);
      type->chk_this_template_generic(initial_value, INCOMPLETE_ALLOWED,
        OMIT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK, IMPLICIT_OMIT, 0);
      gen_restriction_check =
        initial_value->chk_restriction("template variable definition",
                                       template_restriction, initial_value);
      if (!semantic_check_only) {
        initial_value->set_genname_recursive(get_genname());
        initial_value->set_code_section(GovernedSimple::CS_INLINE);
      }
    }
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
    }
  }

  bool Def_Var_Template::chk_identical(Definition *p_def)
  {
    chk();
    p_def->chk();
    if (p_def->get_asstype() != A_VAR_TEMPLATE) {
      const char *dispname_str = id->get_dispname().c_str();
      error("Local definition `%s' is a template variable, but the definition "
        "inherited from component type `%s' is a %s", dispname_str,
        p_def->get_my_scope()->get_fullname().c_str(), p_def->get_assname());
      p_def->note("The inherited definition of `%s' is here", dispname_str);
      return false;
    }
    Def_Var_Template *p_def_var_template =
      dynamic_cast<Def_Var_Template*>(p_def);
    if (!p_def_var_template) FATAL_ERROR("Def_Var_Template::chk_identical()");
    if (!type->is_identical(p_def_var_template->type)) {
      const char *dispname_str = id->get_dispname().c_str();
      type->error("Local template variable `%s' has type `%s', but the "
        "template variable inherited from component type `%s' has type `%s'",
        dispname_str, type->get_typename().c_str(),
        p_def_var_template->get_my_scope()->get_fullname().c_str(),
        p_def_var_template->type->get_typename().c_str());
      p_def_var_template->note("The inherited template variable `%s' is here",
        dispname_str);
      return false;
    }
    if (initial_value) {
      if (!p_def_var_template->initial_value) {
        const char *dispname_str = id->get_dispname().c_str();
        initial_value->warning("Local template variable `%s' has initial "
          "value, but the template variable inherited from component type "
          "`%s' does not", dispname_str,
          p_def_var_template->get_my_scope()->get_fullname().c_str());
        p_def_var_template->note("The inherited template variable `%s' is here",
          dispname_str);
      }
    } else if (p_def_var_template->initial_value) {
      const char *dispname_str = id->get_dispname().c_str();
      warning("Local template variable `%s' does not have initial value, but "
        "the template variable inherited from component type `%s' has",
        dispname_str,
        p_def_var_template->get_my_scope()->get_fullname().c_str());
      p_def_var_template->note("The inherited template variable `%s' is here",
        dispname_str);
    }
    return true;
  }

  void Def_Var_Template::generate_code(output_struct *target, bool clean_up)
  {
    type->generate_code(target);
    const_def cdef;
    Code::init_cdef(&cdef);
    type->generate_code_object(&cdef, my_scope, get_genname(), 0, true, false);
    Code::merge_cdef(target, &cdef);
    Code::free_cdef(&cdef);
    if (initial_value) {
      if (Common::Type::T_SEQOF == initial_value->get_my_governor()->get_typetype() ||
          Common::Type::T_ARRAY == initial_value->get_my_governor()->get_typetype()) {
        target->functions.init_comp = mputprintf(target->functions.init_comp, 
          "%s.remove_all_permutations();\n", initial_value->get_lhs_name().c_str());
      }
      target->functions.init_comp =
        initial_value->generate_code_init(target->functions.init_comp,
          initial_value->get_lhs_name().c_str());
      if (template_restriction!=TR_NONE && gen_restriction_check)
        target->functions.init_comp = Template::generate_restriction_check_code(
          target->functions.init_comp, initial_value->get_lhs_name().c_str(),
          template_restriction);
    } else if (clean_up) {  // No initial value.
      // Always reset component variables/variable templates on component
      // reinitialization.  Fix for HM79493.
      target->functions.init_comp = mputprintf(target->functions.init_comp,
        "%s.clean_up();\n", get_genname().c_str());
    }
  }

  void Def_Var_Template::generate_code(CodeGenHelper& cgh)
  {
    generate_code(cgh.get_outputstruct(this));
  }

  char *Def_Var_Template::generate_code_str(char *str)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    if (initial_value && initial_value->has_single_expr()) {
      // The initial value can be represented by a single C++ expression
      // the object is initialized by the constructor.
      str = mputprintf(str, "%s %s(%s);\n",
        type->get_genname_template(my_scope).c_str(), genname_str,
        initial_value->get_single_expr(false).c_str());
    } else {
      // Use the default constructor.
      str = mputprintf(str, "%s %s;\n",
        type->get_genname_template(my_scope).c_str(), genname_str);
      if (initial_value) {
        // The initial value is assigned using subsequent statements.
        if (use_runtime_2 && TypeConv::needs_conv_refd(initial_value))
          str = TypeConv::gen_conv_code_refd(str, genname_str, initial_value);
        else str = initial_value->generate_code_init(str, genname_str);
      }
    }
    if (initial_value && template_restriction != TR_NONE
        && gen_restriction_check)
      str = Template::generate_restriction_check_code(str, genname_str,
                                                      template_restriction);
    if (debugger_active) {
      str = generate_code_debugger_add_var(str, this);
    }
    return str;
  }

  void Def_Var_Template::ilt_generate_code(ILT *ilt)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    char*& def=ilt->get_out_def();
    char*& init=ilt->get_out_branches();
    def = mputprintf(def, "%s %s;\n",
      type->get_genname_template(my_scope).c_str(), genname_str);
    if (initial_value) {
      init = initial_value->generate_code_init(init, genname_str);
      if (template_restriction!=TR_NONE && gen_restriction_check)
        init = Template::generate_restriction_check_code(init, genname_str,
                 template_restriction);
    }
  }

  char *Def_Var_Template::generate_code_init_comp(char *str,
    Definition *base_defn)
  {
    if (initial_value) {
      str = initial_value->generate_code_init(str,
        base_defn->get_genname_from_scope(my_scope).c_str());
      if (template_restriction != TR_NONE && gen_restriction_check)
        str = Template::generate_restriction_check_code(str,
                base_defn->get_genname_from_scope(my_scope).c_str(),
                template_restriction);
    }
    return str;
  }

  void Def_Var_Template::dump_internal(unsigned level) const
  {
    DEBUG(level, "Template variable %s", id->get_dispname().c_str());
    if (template_restriction!=TR_NONE)
      DEBUG(level + 1, "restriction: %s",
        Template::get_restriction_name(template_restriction));
    type->dump(level + 1);
    if (initial_value) initial_value->dump(level + 1);
  }

  // =================================
  // ===== Def_Timer
  // =================================

  Def_Timer::~Def_Timer()
  {
    delete dimensions;
    delete default_duration;
  }

  Def_Timer *Def_Timer::clone() const
  {
    FATAL_ERROR("Def_Timer::clone");
  }

  void Def_Timer::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    if (dimensions) dimensions->set_fullname(p_fullname + ".<dimensions>");
    if (default_duration)
      default_duration->set_fullname(p_fullname + ".<default_duration>");
  }

  void Def_Timer::set_my_scope(Scope *p_scope)
  {
    Definition::set_my_scope(p_scope);
    if (dimensions) dimensions->set_my_scope(p_scope);
    if (default_duration) default_duration->set_my_scope(p_scope);
  }

  ArrayDimensions *Def_Timer::get_Dimensions()
  {
    if (!checked) chk();
    return dimensions;
  }

  void Def_Timer::chk()
  {
    if(checked) return;
    Error_Context cntxt(this, "In timer definition `%s'",
      id->get_dispname().c_str());
    if (dimensions) dimensions->chk();
    if (default_duration) {
      Error_Context cntxt2(default_duration, "In default duration");
      if (dimensions) chk_array_duration(default_duration);
      else chk_single_duration(default_duration);
      if (!semantic_check_only) {
        default_duration->set_code_section(GovernedSimple::CS_POST_INIT);
      }
    }
    checked = true;
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
    }
  }

  bool Def_Timer::chk_identical(Definition *p_def)
  {
    chk();
    p_def->chk();
    if (p_def->get_asstype() != A_TIMER) {
      const char *dispname_str = id->get_dispname().c_str();
      error("Local definition `%s' is a timer, but the definition inherited "
        "from component type `%s' is a %s", dispname_str,
        p_def->get_my_scope()->get_fullname().c_str(), p_def->get_assname());
      p_def->note("The inherited definition of `%s' is here", dispname_str);
      return false;
    }
    Def_Timer *p_def_timer = dynamic_cast<Def_Timer*>(p_def);
    if (!p_def_timer) FATAL_ERROR("Def_Timer::chk_identical()");
    if (dimensions) {
      if (p_def_timer->dimensions) {
        if (!dimensions->is_identical(p_def_timer->dimensions)) {
          const char *dispname_str = id->get_dispname().c_str();
          error("Local timer `%s' and the timer inherited from component type "
            "`%s' have different array dimensions", dispname_str,
            p_def_timer->get_my_scope()->get_fullname().c_str());
          p_def_timer->note("The inherited timer `%s' is here", dispname_str);
          return false;
        }
      } else {
        const char *dispname_str = id->get_dispname().c_str();
        error("Local definition `%s' is a timer array, but the definition "
          "inherited from component type `%s' is a single timer", dispname_str,
          p_def_timer->get_my_scope()->get_fullname().c_str());
        p_def_timer->note("The inherited timer `%s' is here", dispname_str);
        return false;
      }
    } else if (p_def_timer->dimensions) {
      const char *dispname_str = id->get_dispname().c_str();
      error("Local definition `%s' is a single timer, but the definition "
        "inherited from component type `%s' is a timer array", dispname_str,
        p_def_timer->get_my_scope()->get_fullname().c_str());
      p_def_timer->note("The inherited timer `%s' is here", dispname_str);
      return false;
    }
    if (default_duration) {
      if (p_def_timer->default_duration) {
        if (!default_duration->is_unfoldable() &&
            !p_def_timer->default_duration->is_unfoldable() &&
            !(*default_duration == *p_def_timer->default_duration)) {
          const char *dispname_str = id->get_dispname().c_str();
          default_duration->warning("Local timer `%s' and the timer inherited "
            "from component type `%s' have different default durations",
            dispname_str, p_def_timer->get_my_scope()->get_fullname().c_str());
          p_def_timer->note("The inherited timer `%s' is here", dispname_str);
        }
      } else {
        const char *dispname_str = id->get_dispname().c_str();
        default_duration->error("Local timer `%s' has default duration, but "
          "the timer inherited from component type `%s' does not", dispname_str,
          p_def_timer->get_my_scope()->get_fullname().c_str());
        p_def_timer->note("The inherited timer `%s' is here", dispname_str);
        return false;
      }
    } else if (p_def_timer->default_duration) {
      const char *dispname_str = id->get_dispname().c_str();
      error("Local timer `%s' does not have default duration, but the timer "
        "inherited from component type `%s' has", dispname_str,
        p_def_timer->get_my_scope()->get_fullname().c_str());
      p_def_timer->note("The inherited timer `%s' is here", dispname_str);
      return false;
    }
    return true;
  }

  bool Def_Timer::has_default_duration(FieldOrArrayRefs *p_subrefs)
  {
    // return true in case of any uncertainity
    if (!default_duration) return false;
    else if (!dimensions || !p_subrefs) return true;
    Value *v = default_duration;
    size_t nof_dims = dimensions->get_nof_dims();
    size_t nof_refs = p_subrefs->get_nof_refs();
    size_t upper_limit = nof_dims < nof_refs ? nof_dims : nof_refs;
    for (size_t i = 0; i < upper_limit; i++) {
      v = v->get_value_refd_last();
      if (v->get_valuetype() != Value::V_SEQOF) break;
      FieldOrArrayRef *ref = p_subrefs->get_ref(i);
      if (ref->get_type() != FieldOrArrayRef::ARRAY_REF) return true;
      Value *v_index = ref->get_val()->get_value_refd_last();
      if (v_index->get_valuetype() != Value::V_INT) return true;
      Int index = v_index->get_val_Int()->get_val()
        - dimensions->get_dim_byIndex(i)->get_offset();
      if (index >= 0 && index < static_cast<Int>(v->get_nof_comps()))
        v = v->get_comp_byIndex(index);
      else return true;
    }
    return v->get_valuetype() != Value::V_NOTUSED;
  }

  void Def_Timer::chk_single_duration(Value *dur)
  {
    dur->chk_expr_float(is_local() ?
      Type::EXPECTED_DYNAMIC_VALUE : Type::EXPECTED_STATIC_VALUE);
    Value *v = dur->get_value_refd_last();
    if (v->get_valuetype() == Value::V_REAL) {
      ttcn3float v_real = v->get_val_Real();
      if ( (v_real<0.0) || isSpecialFloatValue(v_real) ) {
        dur->error("A non-negative float value was expected "
          "as timer duration instead of `%s'", Real2string(v_real).c_str());
      }
    }
  }

  void Def_Timer::chk_array_duration(Value *dur, size_t start_dim)
  {
    ArrayDimension *dim = dimensions->get_dim_byIndex(start_dim);
    bool array_size_known = !dim->get_has_error();
    size_t array_size = 0;
    if (array_size_known) array_size = dim->get_size();
    Value *v = dur->get_value_refd_last();
    switch (v->get_valuetype()) {
    case Value::V_ERROR:
      return;
    case Value::V_SEQOF: {
      size_t nof_vs = v->get_nof_comps();
      // Value-list notation.
      if (!v->is_indexed()) {
        if (array_size_known) {
          if (array_size > nof_vs) {
            dur->error("Too few elements in the default duration of timer "
                       "array: %lu was expected instead of %lu",
                       static_cast<unsigned long>(array_size), static_cast<unsigned long>(nof_vs));
          } else if (array_size < nof_vs) {
            dur->error("Too many elements in the default duration of timer "
                       "array: %lu was expected instead of %lu",
                       static_cast<unsigned long>(array_size), static_cast<unsigned long>(nof_vs));
          }
        }
        bool last_dimension = start_dim + 1 >= dimensions->get_nof_dims();
        for (size_t i = 0; i < nof_vs; i++) {
          Value *array_v = v->get_comp_byIndex(i);
          if (array_v->get_valuetype() == Value::V_NOTUSED) continue;
          if (last_dimension) chk_single_duration(array_v);
          else chk_array_duration(array_v, start_dim + 1);
        }
      } else {
        // Indexed-notation.
        bool last_dimension = start_dim + 1 >= dimensions->get_nof_dims();
        map<Int, Int> index_map;
        for (size_t i = 0; i < nof_vs; i++) {
          Value *array_v = v->get_comp_byIndex(i);
          if (array_v->get_valuetype() == Value::V_NOTUSED) continue;
          if (last_dimension) chk_single_duration(array_v);
          else chk_array_duration(array_v, start_dim + 1);
          Error_Context cntxt(this, "In timer array element %lu",
                              static_cast<unsigned long>(i + 1));
          Value *index = v->get_index_byIndex(i);
          dim->chk_index(index, Type::EXPECTED_DYNAMIC_VALUE);
          if (index->get_value_refd_last()->get_valuetype() == Value::V_INT) {
            const int_val_t *index_int = index->get_value_refd_last()
                                         ->get_val_Int();
            if (*index_int > INT_MAX) {
              index->error("An integer value less than `%d' was expected for "
                           "indexing timer array instead of `%s'", INT_MAX,
                           (index_int->t_str()).c_str());
              index->set_valuetype(Value::V_ERROR);
            } else {
              Int index_val = index_int->get_val();
              if (index_map.has_key(index_val)) {
                index->error("Duplicate index value `%s' for timer array "
                             "elements `%s' and `%s'",
                             Int2string(index_val).c_str(),
                             Int2string(static_cast<Int>(i) + 1).c_str(),
                             Int2string(*index_map[index_val]).c_str());
                index->set_valuetype(Value::V_ERROR);
              } else {
                index_map.add(index_val, new Int(static_cast<Int>(i + 1)));
              }
            }
          }
        }
        // It's not possible to have "index_map.size() > array_size", since we
        // add only correct constant-index values into the map.  It's possible
        // to create partially initialized timer arrays.
        for (size_t i = 0; i < index_map.size(); i++)
          delete index_map.get_nth_elem(i);
        index_map.clear();
      }
      break; }
    default:
      if (array_size_known) {
        dur->error("An array value (with %lu elements) was expected as "
                   "default duration of timer array",
                   static_cast<unsigned long>(array_size));
      } else {
        dur->error("An array value was expected as default duration of timer "
                   "array");
      }
      dur->set_valuetype(Value::V_ERROR);
      return;
    }
  }

  void Def_Timer::generate_code(output_struct *target, bool)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    const string& dispname = id->get_dispname();
    if (dimensions) {
      // timer array
      const string& array_type = dimensions->get_timer_type();
      const char *array_type_str = array_type.c_str();
      target->header.global_vars = mputprintf(target->header.global_vars,
        "extern %s %s;\n", array_type_str, genname_str);
      target->source.global_vars = mputprintf(target->source.global_vars,
        "%s %s;\n", array_type_str, genname_str);
      target->functions.pre_init = mputstr(target->functions.pre_init, "{\n"
        "static const char * const timer_name = \"");
      target->functions.pre_init = mputstr(target->functions.pre_init,
        dispname.c_str());
      target->functions.pre_init = mputprintf(target->functions.pre_init,
        "\";\n"
        "%s.set_name(timer_name);\n"
        "}\n", genname_str);
      if (default_duration) target->functions.post_init =
        generate_code_array_duration(target->functions.post_init, genname_str,
          default_duration);
    } else {
      // single timer
      target->header.global_vars = mputprintf(target->header.global_vars,
        "extern TIMER %s;\n", genname_str);
      if (default_duration) {
        // has default duration
        Value *v = default_duration->get_value_refd_last();
        if (v->get_valuetype() == Value::V_REAL) {
          // duration is known at compilation time -> set in the constructor
          target->source.global_vars = mputprintf(target->source.global_vars,
            "TIMER %s(\"%s\", %s);\n", genname_str, dispname.c_str(),
            v->get_single_expr().c_str());
        } else {
          // duration is known only at runtime -> set in post_init
          target->source.global_vars = mputprintf(target->source.global_vars,
            "TIMER %s(\"%s\");\n", genname_str, dispname.c_str());
          expression_struct expr;
          Code::init_expr(&expr);
          expr.expr = mputprintf(expr.expr, "%s.set_default_duration(",
            genname_str);
          default_duration->generate_code_expr(&expr);
          expr.expr = mputc(expr.expr, ')');
          target->functions.post_init =
            Code::merge_free_expr(target->functions.post_init, &expr);
        }
      } else {
        // does not have default duration
        target->source.global_vars = mputprintf(target->source.global_vars,
          "TIMER %s(\"%s\");\n", genname_str, dispname.c_str());
      }
    }
  }

  void Def_Timer::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  char *Def_Timer::generate_code_array_duration(char *str,
    const char *object_name, Value *dur, size_t start_dim)
  {
    ArrayDimension *dim = dimensions->get_dim_byIndex(start_dim);
    size_t dim_size = dim->get_size();
    Value *v = dur->get_value_refd_last();
    if (v->get_valuetype() != Value::V_SEQOF
        || (v->get_nof_comps() != dim_size && !v->is_indexed()))
      FATAL_ERROR("Def_Timer::generate_code_array_duration()");
    // Value-list notation.
    if (!v->is_indexed()) {
      if (start_dim + 1 < dimensions->get_nof_dims()) {
        // There are more dimensions, the elements of "v" are arrays a
        // temporary reference shall be introduced if the next dimension has
        // more than 1 elements.
        bool temp_ref_needed =
          dimensions->get_dim_byIndex(start_dim + 1)->get_size() > 1;
        for (size_t i = 0; i < dim_size; i++) {
          Value *v_elem = v->get_comp_byIndex(i);
          if (v_elem->get_valuetype() == Value::V_NOTUSED) continue;
          if (temp_ref_needed) {
            const string& tmp_id = my_scope->get_scope_mod_gen()
              ->get_temporary_id();
            const char *tmp_str = tmp_id.c_str();
            str = mputprintf(str, "{\n"
                             "%s& %s = %s.array_element(%lu);\n",
                             dimensions->get_timer_type(start_dim + 1).c_str(),
                             tmp_str, object_name, static_cast<unsigned long>(i));
            str = generate_code_array_duration(str, tmp_str, v_elem,
                                               start_dim + 1);
            str = mputstr(str, "}\n");
          } else {
            char *tmp_str = mprintf("%s.array_element(%lu)", object_name,
                                    static_cast<unsigned long>(i));
            str = generate_code_array_duration(str, tmp_str, v_elem,
                                               start_dim + 1);
            Free(tmp_str);
          }
        }
      } else {
        // We are in the last dimension, the elements of "v" are floats.
        for (size_t i = 0; i < dim_size; i++) {
          Value *v_elem = v->get_comp_byIndex(i);
          if (v_elem->get_valuetype() == Value::V_NOTUSED) continue;
          expression_struct expr;
          Code::init_expr(&expr);
          expr.expr = mputprintf(expr.expr,
                                 "%s.array_element(%lu).set_default_duration(",
                                 object_name, static_cast<unsigned long>(i));
          v_elem->generate_code_expr(&expr);
          expr.expr = mputc(expr.expr, ')');
          str = Code::merge_free_expr(str, &expr);
        }
      }
    // Indexed-list notation.
    } else {
      if (start_dim + 1 < dimensions->get_nof_dims()) {
        bool temp_ref_needed =
          dimensions->get_dim_byIndex(start_dim + 1)->get_size() > 1;
        for (size_t i = 0; i < v->get_nof_comps(); i++) {
          Value *v_elem = v->get_comp_byIndex(i);
          if (v_elem->get_valuetype() == Value::V_NOTUSED) continue;
          if (temp_ref_needed) {
            const string& tmp_id = my_scope->get_scope_mod_gen()
              ->get_temporary_id();
            const string& idx_id = my_scope->get_scope_mod_gen()
              ->get_temporary_id();
            const char *tmp_str = tmp_id.c_str();
            str = mputstr(str, "{\n");
            str = mputprintf(str, "int %s;\n", idx_id.c_str());
            str = v->get_index_byIndex(i)->generate_code_init(str,
                                                              idx_id.c_str());
            str = mputprintf(str, "%s& %s = %s.array_element(%s);\n",
                             dimensions->get_timer_type(start_dim + 1).c_str(),
                             tmp_str, object_name, idx_id.c_str());
            str = generate_code_array_duration(str, tmp_str, v_elem,
                                               start_dim + 1);
            str = mputstr(str, "}\n");
          } else {
            const string& idx_id = my_scope->get_scope_mod_gen()
              ->get_temporary_id();
            str = mputstr(str, "{\n");
            str = mputprintf(str, "int %s;\n", idx_id.c_str());
            str = v->get_index_byIndex(i)->generate_code_init(str,
                                                              idx_id.c_str());
            char *tmp_str = mprintf("%s.array_element(%s)", object_name,
                                    idx_id.c_str());
            str = generate_code_array_duration(str, tmp_str, v_elem,
                                               start_dim + 1);
            str = mputstr(str, "}\n");
            Free(tmp_str);
          }
        }
      } else {
        for (size_t i = 0; i < v->get_nof_comps(); i++) {
          Value *v_elem = v->get_comp_byIndex(i);
          if (v_elem->get_valuetype() == Value::V_NOTUSED) continue;
          expression_struct expr;
          Code::init_expr(&expr);
          str = mputstr(str, "{\n");
          const string& idx_id = my_scope->get_scope_mod_gen()
            ->get_temporary_id();
          str = mputprintf(str, "int %s;\n", idx_id.c_str());
          str = v->get_index_byIndex(i)->generate_code_init(str,
                                                            idx_id.c_str());
          str = mputprintf(str,
                           "%s.array_element(%s).set_default_duration(",
                           object_name, idx_id.c_str());
          v_elem->generate_code_expr(&expr);
          expr.expr = mputc(expr.expr, ')');
          str = Code::merge_free_expr(str, &expr);
          str = mputstr(str, "}\n");
        }
      }
    }
    return str;
  }

  char *Def_Timer::generate_code_str(char *str)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    const string& dispname = id->get_dispname();
    if (dimensions) {
      // timer array
      const string& array_type = dimensions->get_timer_type();
      const char *array_type_str = array_type.c_str();
      str = mputprintf(str, "%s %s;\n", array_type_str, genname_str);
      str = mputstr(str, "{\n"
        "static const char * const timer_name =  \"");
      str = mputstr(str, dispname.c_str());
      str = mputprintf(str, "\";\n"
        "%s.set_name(timer_name);\n"
        "}\n", genname_str);
      if (default_duration) str = generate_code_array_duration(str,
        genname_str, default_duration);
    } else {
      // single timer
      if (default_duration && default_duration->has_single_expr()) {
        // the default duration can be passed to the constructor
        str = mputprintf(str, "TIMER %s(\"%s\", %s);\n", genname_str,
          dispname.c_str(), default_duration->get_single_expr().c_str());
      } else {
        // only the name is passed to the constructor
        str = mputprintf(str, "TIMER %s(\"%s\");\n", genname_str,
          dispname.c_str());
        if (default_duration) {
          // the default duration is set explicitly
          expression_struct expr;
          Code::init_expr(&expr);
          expr.expr = mputprintf(expr.expr, "%s.set_default_duration(",
            genname_str);
          default_duration->generate_code_expr(&expr);
          expr.expr = mputc(expr.expr, ')');
          str = Code::merge_free_expr(str, &expr);
        }
      }
    }
    if (debugger_active) {
      str = generate_code_debugger_add_var(str, this);
    }
    return str;
  }

  void Def_Timer::ilt_generate_code(ILT *ilt)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    const string& dispname = id->get_dispname();

    char*& def = ilt->get_out_def();
    char*& init = ilt->get_out_branches();

    if (dimensions) {
      // timer array
      const string& array_type = dimensions->get_timer_type();
      const char *array_type_str = array_type.c_str();
      def = mputprintf(def, "%s %s;\n", array_type_str, genname_str);
      def = mputstr(def, "{\n"
        "static const char * const timer_names[] = { ");
      def = dimensions->generate_element_names(def, dispname);
      def = mputprintf(def, " };\n"
        "%s.set_name(%lu, timer_names);\n"
        "}\n", genname_str, static_cast<unsigned long>( dimensions->get_array_size() ));
      if (default_duration) init = generate_code_array_duration(init,
        genname_str, default_duration);
    } else {
      // single timer
      if (default_duration) {
        // has default duration
        Value *v = default_duration->get_value_refd_last();
        if (v->get_valuetype() == Value::V_REAL) {
          // duration is known at compilation time -> set in the constructor
          def = mputprintf(def, "TIMER %s(\"%s\", %s);\n", genname_str,
            dispname.c_str(), v->get_single_expr().c_str());
        } else {
          // duration is known only at runtime -> set when control reaches the
          // timer definition
          def = mputprintf(def, "TIMER %s(\"%s\");\n", genname_str,
            dispname.c_str());
          expression_struct expr;
          Code::init_expr(&expr);
          expr.expr = mputprintf(expr.expr, "%s.set_default_duration(",
            genname_str);
          default_duration->generate_code_expr(&expr);
          expr.expr = mputc(expr.expr, ')');
          init = Code::merge_free_expr(init, &expr);
        }
      } else {
        // does not have default duration
        def = mputprintf(def, "TIMER %s(\"%s\");\n", genname_str,
          dispname.c_str());
      }
    }
  }

  char *Def_Timer::generate_code_init_comp(char *str, Definition *base_defn)
  {
    if (default_duration) {
      Def_Timer *base_timer_defn = dynamic_cast<Def_Timer*>(base_defn);
      if (!base_timer_defn || !base_timer_defn->default_duration)
        FATAL_ERROR("Def_Timer::generate_code_init_comp()");
      // initializer is not needed if the default durations are the same
      // constants in both timers
      if (default_duration->is_unfoldable() ||
          base_timer_defn->default_duration->is_unfoldable() ||
          !(*default_duration == *base_timer_defn->default_duration)) {
        if (dimensions) {
          str = generate_code_array_duration(str,
            base_timer_defn->get_genname_from_scope(my_scope).c_str(),
            default_duration);
        } else {
          expression_struct expr;
          Code::init_expr(&expr);
          expr.expr = mputprintf(expr.expr, "%s.set_default_duration(",
            base_timer_defn->get_genname_from_scope(my_scope).c_str());
          default_duration->generate_code_expr(&expr);
          expr.expr = mputc(expr.expr, ')');
          str = Code::merge_free_expr(str, &expr);
        }
      }
    }
    return str;
  }

  void Def_Timer::dump_internal(unsigned level) const
  {
    DEBUG(level, "Timer: %s", id->get_dispname().c_str());
    if (dimensions) dimensions->dump(level + 1);
    if (default_duration) {
      DEBUG(level + 1, "Default duration:");
      default_duration->dump(level + 1);
    }
  }

  // =================================
  // ===== Def_Port
  // =================================

  Def_Port::Def_Port(Identifier *p_id, Reference *p_tref,
    ArrayDimensions *p_dims)
    : Definition(A_PORT, p_id), type_ref(p_tref), port_type(0),
    dimensions(p_dims)
  {
    if (!p_tref) FATAL_ERROR("Def_Port::Def_Port()");
  }

  Def_Port::~Def_Port()
  {
    delete type_ref;
    delete dimensions;
  }

  Def_Port *Def_Port::clone() const
  {
    FATAL_ERROR("Def_Port::clone");
  }

  void Def_Port::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    type_ref->set_fullname(p_fullname + ".<type_ref>");
    if (dimensions) dimensions->set_fullname(p_fullname);
  }

  void Def_Port::set_my_scope(Scope *p_scope)
  {
    Definition::set_my_scope(p_scope);
    type_ref->set_my_scope(p_scope);
    if (dimensions) dimensions->set_my_scope(p_scope);
  }

  Type *Def_Port::get_Type()
  {
    chk();
    return port_type;
  }

  ArrayDimensions *Def_Port::get_Dimensions()
  {
    if (!checked) chk();
    return dimensions;
  }

  void Def_Port::chk()
  {
    if (checked) return;
    checked = true;
    Error_Context cntxt(this, "In port definition `%s'",
      id->get_dispname().c_str());
    Common::Assignment *ass = type_ref->get_refd_assignment();
    if (ass) {
      if (ass->get_asstype() == A_TYPE) {
        Type *t = ass->get_Type()->get_type_refd_last();
        if (t->get_typetype() == Type::T_PORT) port_type = t;
        else type_ref->error("Type reference `%s' does not refer to a "
          "port type", type_ref->get_dispname().c_str());
      } else type_ref->error("Reference `%s' does not refer to a "
        "type", type_ref->get_dispname().c_str());
    }
    if (dimensions) dimensions->chk();
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
    }
  }

  bool Def_Port::chk_identical(Definition *p_def)
  {
    chk();
    p_def->chk();
    if (p_def->get_asstype() != A_PORT) {
      const char *dispname_str = id->get_dispname().c_str();
      error("Local definition `%s' is a port, but the definition inherited "
        "from component type `%s' is a %s", dispname_str,
        p_def->get_my_scope()->get_fullname().c_str(), p_def->get_assname());
      p_def->note("The inherited definition of `%s' is here", dispname_str);
      return false;
    }
    Def_Port *p_def_port = dynamic_cast<Def_Port*>(p_def);
    if (!p_def_port) FATAL_ERROR("Def_Port::chk_identical()");
    if (port_type && p_def_port->port_type &&
        port_type != p_def_port->port_type) {
      const char *dispname_str = id->get_dispname().c_str();
      type_ref->error("Local port `%s' has type `%s', but the port inherited "
        "from component type `%s' has type `%s'", dispname_str,
        port_type->get_typename().c_str(),
        p_def_port->get_my_scope()->get_fullname().c_str(),
        p_def_port->port_type->get_typename().c_str());
      p_def_port->note("The inherited port `%s' is here", dispname_str);
      return false;
    }
    if (dimensions) {
      if (p_def_port->dimensions) {
        if (!dimensions->is_identical(p_def_port->dimensions)) {
          const char *dispname_str = id->get_dispname().c_str();
          error("Local port `%s' and the port inherited from component type "
            "`%s' have different array dimensions", dispname_str,
            p_def_port->get_my_scope()->get_fullname().c_str());
          p_def_port->note("The inherited port `%s' is here", dispname_str);
          return false;
        }
      } else {
        const char *dispname_str = id->get_dispname().c_str();
        error("Local definition `%s' is a port array, but the definition "
          "inherited from component type `%s' is a single port", dispname_str,
          p_def_port->get_my_scope()->get_fullname().c_str());
        p_def_port->note("The inherited port `%s' is here", dispname_str);
        return false;
      }
    } else if (p_def_port->dimensions) {
      const char *dispname_str = id->get_dispname().c_str();
      error("Local definition `%s' is a single port, but the definition "
        "inherited from component type `%s' is a port array", dispname_str,
        p_def_port->get_my_scope()->get_fullname().c_str());
      p_def_port->note("The inherited port `%s' is here", dispname_str);
      return false;
    }
    return true;
  }

  void Def_Port::generate_code(output_struct *target, bool)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    const string& type_genname = port_type->get_genname_value(my_scope);
    const string& dispname = id->get_dispname();
    if (dimensions) {
      // port array
      const string& array_type = dimensions->get_port_type(type_genname);
      const char *array_type_str = array_type.c_str();
      target->header.global_vars = mputprintf(target->header.global_vars,
        "extern %s %s;\n", array_type_str, genname_str);
      target->source.global_vars = mputprintf(target->source.global_vars,
        "%s %s;\n", array_type_str, genname_str);
      target->functions.pre_init = mputstr(target->functions.pre_init, "{\n"
        "static const char * const port_name = \"");
      target->functions.pre_init = mputstr(target->functions.pre_init,
        dispname.c_str());
      target->functions.pre_init = mputprintf(target->functions.pre_init,
        "\";\n"
        "%s.set_name(port_name);\n"
        "}\n", genname_str);
    } else {
      // single port
      const char *type_genname_str = type_genname.c_str();
      target->header.global_vars = mputprintf(target->header.global_vars,
        "extern %s %s;\n", type_genname_str, genname_str);
      target->source.global_vars = mputprintf(target->source.global_vars,
        "%s %s(\"%s\");\n", type_genname_str, genname_str, dispname.c_str());
    }
    target->functions.init_comp = mputprintf(target->functions.init_comp,
      "%s.activate_port();\n", genname_str);
  }

  void Def_Port::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  char *Def_Port::generate_code_init_comp(char *str, Definition *base_defn)
  {
    return mputprintf(str, "%s.activate_port();\n",
      base_defn->get_genname_from_scope(my_scope).c_str());
  }

  void Def_Port::dump_internal(unsigned level) const
  {
    DEBUG(level, "Port: %s", id->get_dispname().c_str());
    DEBUG(level + 1, "Port type:");
    type_ref->dump(level + 2);
    if (dimensions) dimensions->dump(level + 1);
  }

  // =================================
  // ===== Def_Function_Base
  // =================================

  Def_Function_Base::asstype_t Def_Function_Base::determine_asstype(
    bool is_external, bool has_return_type, bool returns_template)
  {
    if (is_external) {
      if (has_return_type) {
        if (returns_template) return A_EXT_FUNCTION_RTEMP;
        else return A_EXT_FUNCTION_RVAL;
      } else {
        if (returns_template)
          FATAL_ERROR("Def_Function_Base::determine_asstype()");
        return A_EXT_FUNCTION;
      }
    } else { // not an external function
      if (has_return_type) {
        if (returns_template) return A_FUNCTION_RTEMP;
        else return A_FUNCTION_RVAL;
      } else {
        if (returns_template)
          FATAL_ERROR("Def_Function_Base::determine_asstype()");
        return A_FUNCTION;
      }
    }
  }

  Def_Function_Base::Def_Function_Base(const Def_Function_Base& p)
    : Definition(p), prototype(PROTOTYPE_NONE), input_type(0), output_type(0)
  {
    fp_list = p.fp_list->clone();
    fp_list->set_my_def(this);
    return_type = p.return_type ? p.return_type->clone() : 0;
    template_restriction = p.template_restriction;
  }

  Def_Function_Base::Def_Function_Base(bool is_external, Identifier *p_id,
    FormalParList *p_fpl, Type *p_return_type, bool returns_template,
    template_restriction_t p_template_restriction)
    : Definition(determine_asstype(is_external, p_return_type != 0,
        returns_template), p_id), fp_list(p_fpl), return_type(p_return_type),
        prototype(PROTOTYPE_NONE), input_type(0), output_type(0),
        template_restriction(p_template_restriction)
  {
    if (!p_fpl) FATAL_ERROR("Def_Function_Base::Def_Function_Base()");
    fp_list->set_my_def(this);
    if (return_type) return_type->set_ownertype(Type::OT_FUNCTION_DEF, this);
  }

  Def_Function_Base::~Def_Function_Base()
  {
    delete fp_list;
    delete return_type;
  }

  void Def_Function_Base::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    fp_list->set_fullname(p_fullname + ".<formal_par_list>");
    if (return_type) return_type->set_fullname(p_fullname + ".<return_type>");
  }

  void Def_Function_Base::set_my_scope(Scope *p_scope)
  {
    Definition::set_my_scope(p_scope);
    fp_list->set_my_scope(p_scope);
    if (return_type) return_type->set_my_scope(p_scope);
  }

  Type *Def_Function_Base::get_Type()
  {
    if (!checked) chk();
    return return_type;
  }

  FormalParList *Def_Function_Base::get_FormalParList()
  {
    if (!checked) chk();
    return fp_list;
  }

  const char *Def_Function_Base::get_prototype_name() const
  {
    switch (prototype) {
    case PROTOTYPE_NONE:
      return "<no prototype>";
    case PROTOTYPE_CONVERT:
      return "convert";
    case PROTOTYPE_FAST:
      return "fast";
    case PROTOTYPE_BACKTRACK:
      return "backtrack";
    case PROTOTYPE_SLIDING:
      return "sliding";
    default:
      return "<unknown prototype>";
    }
  }

  void Def_Function_Base::chk_prototype()
  {
    switch (prototype) {
    case PROTOTYPE_NONE:
      // return immediately
      return;
    case PROTOTYPE_CONVERT:
    case PROTOTYPE_FAST:
    case PROTOTYPE_BACKTRACK:
    case PROTOTYPE_SLIDING:
      // perform the checks below
      break;
    default:
      FATAL_ERROR("Def_Function_Base::chk_prototype()");
    }
    // checking the formal parameter list
    if (prototype == PROTOTYPE_CONVERT) {
      if (fp_list->get_nof_fps() == 1) {
        FormalPar *par = fp_list->get_fp_byIndex(0);
        if (par->get_asstype() == A_PAR_VAL_IN) {
          input_type = par->get_Type();
        } else {
          par->error("The parameter must be an `in' value parameter for "
            "attribute `prototype(%s)' instead of %s", get_prototype_name(),
            par->get_assname());
        }
      } else {
        fp_list->error("The function must have one parameter instead of %lu "
          "for attribute `prototype(%s)'", static_cast<unsigned long>( fp_list->get_nof_fps() ),
          get_prototype_name());
      }
    } else { // not PROTOTYPE_CONVERT
      if (fp_list->get_nof_fps() == 2) {
        FormalPar *first_par = fp_list->get_fp_byIndex(0);
        if (prototype == PROTOTYPE_SLIDING) {
          if (first_par->get_asstype() == A_PAR_VAL_INOUT) {
            Type *first_par_type = first_par->get_Type();
            switch (first_par_type->get_type_refd_last()
                    ->get_typetype_ttcn3()) {
            case Type::T_ERROR:
            case Type::T_OSTR:
            case Type::T_CSTR:
            case Type::T_BSTR:
              input_type = first_par_type;
              break;
            default:
              first_par_type->error("The type of the first parameter must be "
                "`octetstring' or `charstring' or `bitstring' for attribute "
                "`prototype(%s)' instead of `%s'", get_prototype_name(),
                first_par_type->get_typename().c_str());
            }
          } else {
            first_par->error("The first parameter must be an `inout' value "
              "parameter for attribute `prototype(%s)' instead of %s",
              get_prototype_name(), first_par->get_assname());
          }
        } else {
          if (first_par->get_asstype() == A_PAR_VAL_IN) {
            input_type = first_par->get_Type();
          } else {
            first_par->error("The first parameter must be an `in' value "
              "parameter for attribute `prototype(%s)' instead of %s",
              get_prototype_name(), first_par->get_assname());
          }
        }
        FormalPar *second_par = fp_list->get_fp_byIndex(1);
        if (second_par->get_asstype() == A_PAR_VAL_OUT) {
          output_type = second_par->get_Type();
        } else {
          second_par->error("The second parameter must be an `out' value "
            "parameter for attribute `prototype(%s)' instead of %s",
            get_prototype_name(), second_par->get_assname());
        }
      } else {
        fp_list->error("The function must have two parameters for attribute "
          "`prototype(%s)' instead of %lu", get_prototype_name(),
          static_cast<unsigned long>( fp_list->get_nof_fps()) );
      }
    }
    // checking the return type
    if (prototype == PROTOTYPE_FAST) {
      if (return_type) {
        return_type->error("The function cannot have return type for "
          "attribute `prototype(%s)'", get_prototype_name());
      }
    } else {
      if (return_type) {
        if (asstype == A_FUNCTION_RTEMP || asstype == A_EXT_FUNCTION_RTEMP)
          return_type->error("The function must return a value instead of a "
            "template for attribute `prototype(%s)'", get_prototype_name());
        if (prototype == PROTOTYPE_CONVERT) {
          output_type = return_type;
        } else {
          switch (return_type->get_type_refd_last()->get_typetype_ttcn3()) {
          case Type::T_ERROR:
          case Type::T_INT:
            break;
          default:
            return_type->error("The return type of the function must be "
              "`integer' instead of `%s' for attribute `prototype(%s)'",
              return_type->get_typename().c_str(), get_prototype_name());
          }
        }
      } else {
        error("The function must have return type for attribute "
          "`prototype(%s)'", get_prototype_name());
      }
    }
    // checking the 'runs on' clause
    if (get_RunsOnType()) {
      error("The function cannot have `runs on' clause for attribute "
        "`prototype(%s)'", get_prototype_name());
    }
  }

  Type *Def_Function_Base::get_input_type()
  {
    if (!checked) chk();
    return input_type;
  }

  Type *Def_Function_Base::get_output_type()
  {
    if (!checked) chk();
    return output_type;
  }


  // =================================
  // ===== Def_Function
  // =================================

  Def_Function::Def_Function(Identifier *p_id, FormalParList *p_fpl,
                             Reference *p_runs_on_ref, Reference *p_port_ref,
                             Type *p_return_type,
                             bool returns_template,
                             template_restriction_t p_template_restriction,
                             StatementBlock *p_block)
    : Def_Function_Base(false, p_id, p_fpl, p_return_type, returns_template,
        p_template_restriction),
        runs_on_ref(p_runs_on_ref), runs_on_type(0),
        port_ref(p_port_ref), port_type(0), block(p_block),
        is_startable(false), transparent(false)
  {
    if (!p_block) FATAL_ERROR("Def_Function::Def_Function()");
    block->set_my_def(this);
  }

  Def_Function::~Def_Function()
  {
    delete runs_on_ref;
    delete port_ref;
    delete block;
  }

  Def_Function *Def_Function::clone() const
  {
    FATAL_ERROR("Def_Function::clone");
  }

  void Def_Function::set_fullname(const string& p_fullname)
  {
    Def_Function_Base::set_fullname(p_fullname);
    if (runs_on_ref) runs_on_ref->set_fullname(p_fullname + ".<runs_on_type>");
    if (port_ref) port_ref->set_fullname(p_fullname + ".<port_type>");
    block->set_fullname(p_fullname + ".<statement_block>");
  }

  void Def_Function::set_my_scope(Scope *p_scope)
  {
    bridgeScope.set_parent_scope(p_scope);
    bridgeScope.set_scopeMacro_name(id->get_dispname());

    Def_Function_Base::set_my_scope(&bridgeScope);
    if (runs_on_ref) runs_on_ref->set_my_scope(&bridgeScope);
    if (port_ref) port_ref->set_my_scope(&bridgeScope);
    block->set_my_scope(fp_list);
  }

  Type *Def_Function::get_RunsOnType()
  {
    if (!checked) chk();
    return runs_on_type;
  }
  
  Type *Def_Function::get_PortType()
  {
    if (!checked) chk();
    return port_type;
  }

  RunsOnScope *Def_Function::get_runs_on_scope(Type *comptype)
  {
    Module *my_module = dynamic_cast<Module*>(my_scope->get_scope_mod());
    if (!my_module) FATAL_ERROR("Def_Function::get_runs_on_scope()");
    return my_module->get_runs_on_scope(comptype);
  }
  
  PortScope *Def_Function::get_port_scope(Type *porttype)
  {
    Module *my_module = dynamic_cast<Module*>(my_scope->get_scope_mod());
    if (!my_module) FATAL_ERROR("Def_Function::get_port_scope()");
    return my_module->get_port_scope(porttype);
  }

  void Def_Function::chk()
  {
    if (checked) return;
    checked = true;
    Error_Context cntxt(this, "In function definition `%s'",
      id->get_dispname().c_str());
    // `runs on' clause and `port' clause are mutually exclusive
    if (runs_on_ref && port_ref) {
      runs_on_ref->error("A `runs on' and a `port' clause cannot be present at the same time.");
    }
    // checking the `runs on' clause
    if (runs_on_ref) {
      Error_Context cntxt2(runs_on_ref, "In `runs on' clause");
      runs_on_type = runs_on_ref->chk_comptype_ref();
      // override the scope of the formal parameter list
      if (runs_on_type) {
        Scope *runs_on_scope = get_runs_on_scope(runs_on_type);
        runs_on_scope->set_parent_scope(my_scope);
        fp_list->set_my_scope(runs_on_scope);
      }
    }
    
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
      Ttcn::ExtensionAttributes * extattrs = parse_extattributes(w_attrib_path);
      if (extattrs != 0) { // NULL means parsing error
        size_t num_atrs = extattrs->size();
        for (size_t i=0; i < num_atrs; ++i) {
          ExtensionAttribute &ea = extattrs->get(i);
          switch (ea.get_type()) {
          case ExtensionAttribute::PROTOTYPE: {
            if (get_prototype() != Def_Function_Base::PROTOTYPE_NONE) {
              ea.error("Duplicate attribute `prototype'");
            }
            Def_Function_Base::prototype_t proto = ea.get_proto();
            set_prototype(proto);
            break; }

          case ExtensionAttribute::ANYTYPELIST: // ignore it
          case ExtensionAttribute::NONE: // erroneous, do not issue an error
            break;

          case ExtensionAttribute::TRANSPARENT:
            transparent = true;
            break;

          case ExtensionAttribute::ENCODE:
          case ExtensionAttribute::DECODE:
          case ExtensionAttribute::ERRORBEHAVIOR:
          case ExtensionAttribute::PRINTING:
            ea.error("Extension attribute 'encode', 'decode', 'errorbehavior'"
              " or 'printing' can only be applied to external functions");
            // fall through

          default: // complain
            ea.error("Function definition can only have the 'prototype'"
              " extension attribute");
            break;
          }
        }
        delete extattrs;
      }
    }
    chk_prototype();
    
    // checking the `port' clause
   if (port_ref) {
      Error_Context cntxt2(port_ref, "In `port' clause");
      Assignment *ass = port_ref->get_refd_assignment();
      if (ass) {
        port_type = ass->get_Type();
        if (port_type) {
          switch (port_type->get_typetype()) {
            case Type::T_PORT: {
              Scope *port_scope = get_port_scope(port_type);
              port_scope->set_parent_scope(my_scope);
              fp_list->set_my_scope(port_scope);
              break; }
            default:
              port_ref->error(
                "Reference `%s' does not refer to a port type.",
                port_ref->get_dispname().c_str());
          }
        } else {
          FATAL_ERROR("Def_Function::chk()");
        }
      }
    }
    // checking the formal parameter list
    fp_list->chk(asstype);
    // checking of return type
    if (return_type) {
      Error_Context cntxt2(return_type, "In return type");
      return_type->chk();
      return_type->chk_as_return_type(asstype == A_FUNCTION_RVAL,"function");
    }
    // decision of startability
    is_startable = runs_on_ref != 0;
    if (is_startable && !fp_list->get_startability()) is_startable = false;
    if (is_startable && return_type && return_type->is_component_internal())
          is_startable = false;
    // checking of statement block
    block->chk();
    if (return_type) {
      // checking the presence of return statements
      switch (block->has_return()) {
      case StatementBlock::RS_NO:
        error("The function has return type, but it does not have any return "
          "statement");
        break;
      case StatementBlock::RS_MAYBE:
            error("The function has return type, but control might leave it "
          "without reaching a return statement");
      default:
        break;
      }
    }
    if (!semantic_check_only) {
      fp_list->set_genname(get_genname());
      block->set_code_section(GovernedSimple::CS_INLINE);
    }
  }
    
  bool Def_Function::chk_startable()
  {
    if (!checked) chk();
    if (is_startable) return true;
    if (!runs_on_ref) error("Function `%s' cannot be started on a parallel "
      "test component because it does not have `runs on' clause",
      get_fullname().c_str());
    fp_list->chk_startability("Function", get_fullname().c_str());
    if (return_type && return_type->is_component_internal()) {
      map<Type*,void> type_chain;
      char* err_str = mprintf("the return type or embedded in the return type "
        "of function `%s' if it is started on a parallel test component",
        get_fullname().c_str());
      return_type->chk_component_internal(type_chain, err_str);
      Free(err_str);
    }
    return false;
  }

  void Def_Function::generate_code(output_struct *target, bool clean_up)
  {
    // Functions with 'port' clause are generated into the port type's class def
    // Reuse of clean_up variable to allow or disallow the generation.
    // clean_up is true when it is called from PortTypeBody::generate_code())
    if (port_type && !clean_up) {
      return;
    }
    transparency_holder glass(*this);
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    const char *dispname_str = id->get_dispname().c_str();
    string return_type_name;
    switch (asstype) {
    case A_FUNCTION:
      return_type_name = "void";
      break;
    case A_FUNCTION_RVAL:
      return_type_name = return_type->get_genname_value(my_scope);
      break;
    case A_FUNCTION_RTEMP:
      return_type_name = return_type->get_genname_template(my_scope);
      break;
    default:
      FATAL_ERROR("Def_Function::generate_code()");
    }
    const char *return_type_str = return_type_name.c_str();
    
    // assemble the function body first (this also determines which parameters
    // are never used)
    char* body = create_location_object(memptystr(), "FUNCTION", dispname_str);
    if (!enable_set_bound_out_param)
      body = fp_list->generate_code_set_unbound(body); // conform the standard out parameter is unbound
    body = fp_list->generate_shadow_objects(body);
    if (debugger_active) {
      body = generate_code_debugger_function_init(body, this);
    }
    body = block->generate_code(body, target->header.global_vars,
      target->source.global_vars);
    // smart formal parameter list (names of unused parameters are omitted)
    char *formal_par_list = fp_list->generate_code(memptystr());
    fp_list->generate_code_defval(target);
    // function prototype
    target->header.function_prototypes =
      mputprintf(target->header.function_prototypes, "%s%s %s(%s);\n",
        get_PortType() && clean_up ? "" : "extern ",
        return_type_str, genname_str, formal_par_list);

    // function body    
    target->source.function_bodies = mputprintf(target->source.function_bodies,
      "%s %s%s%s%s(%s)\n"
      "{\n"
      "%s"
      "}\n\n",
      return_type_str,
      port_type && clean_up ? port_type->get_genname_own().c_str() : "",
      port_type && clean_up && port_type->get_PortBody()->get_testport_type() != PortTypeBody::TP_INTERNAL ? "_BASE" : "",
      port_type && clean_up ? "::" : "",
      genname_str, formal_par_list, body);
    Free(formal_par_list);
    Free(body);

    if (is_startable) {
      size_t nof_fps = fp_list->get_nof_fps();
      // use the full list of formal parameters here (since they are all logged)
      char *full_formal_par_list = fp_list->generate_code(memptystr(), nof_fps);
      // starter function (stub)
        // function prototype
        target->header.function_prototypes =
          mputprintf(target->header.function_prototypes,
            "extern void start_%s(const COMPONENT& component_reference%s%s);\n",
            genname_str, nof_fps>0?", ":"", full_formal_par_list);
        // function body
        body = mprintf("void start_%s(const COMPONENT& component_reference%s"
            "%s)\n"
          "{\n"
          "TTCN_Logger::begin_event(TTCN_Logger::PARALLEL_PTC);\n"
          "TTCN_Logger::log_event_str(\"Starting function %s(\");\n",
          genname_str, nof_fps>0?", ":"", full_formal_par_list, dispname_str);
        for (size_t i = 0; i < nof_fps; i++) {
          if (i > 0) body = mputstr(body,
             "TTCN_Logger::log_event_str(\", \");\n");
          body = mputprintf(body, "%s.log();\n",
            fp_list->get_fp_byIndex(i)->get_reference_name(my_scope).c_str());
        }
        body = mputprintf(body,
          "TTCN_Logger::log_event_str(\") on component \");\n"
          "component_reference.log();\n"
          "TTCN_Logger::log_char('.');\n"
          "TTCN_Logger::end_event();\n"
          "Text_Buf text_buf;\n"
          "TTCN_Runtime::prepare_start_component(component_reference, "
            "\"%s\", \"%s\", text_buf);\n",
          my_scope->get_scope_mod()->get_modid().get_dispname().c_str(),
          dispname_str);
        for (size_t i = 0; i < nof_fps; i++) {
          body = mputprintf(body, "%s.encode_text(text_buf);\n",
            fp_list->get_fp_byIndex(i)->get_reference_name(my_scope).c_str());
        }
        body = mputstr(body, "TTCN_Runtime::send_start_component(text_buf);\n"
          "}\n\n");
        target->source.function_bodies = mputstr(target->source.function_bodies,
          body);
        Free(body);

      // an entry in start_ptc_function
      body = mprintf("if (!strcmp(function_name, \"%s\")) {\n",
        dispname_str);
      if (nof_fps > 0) {
        body = fp_list->generate_code_object(body, "", ' ', true);
        for (size_t i = 0; i < nof_fps; i++) {
          body = mputprintf(body, "%s.decode_text(function_arguments);\n",
            fp_list->get_fp_byIndex(i)->get_reference_name(my_scope).c_str());
        }
        body = mputprintf(body,
          "TTCN_Logger::begin_event(TTCN_Logger::PARALLEL_PTC);\n"
          "TTCN_Logger::log_event_str(\"Starting function %s(\");\n",
          dispname_str);
        for (size_t i = 0; i < nof_fps; i++) {
          if (i > 0) body = mputstr(body,
             "TTCN_Logger::log_event_str(\", \");\n");
          body = mputprintf(body, "%s.log();\n",
            fp_list->get_fp_byIndex(i)->get_reference_name(my_scope).c_str());
        }
        body = mputstr(body, "TTCN_Logger::log_event_str(\").\");\n"
          "TTCN_Logger::end_event();\n");
      } else {
        body = mputprintf(body,
          "TTCN_Logger::log_str(TTCN_Logger::PARALLEL_PTC, \"Starting function "
          "%s().\");\n", dispname_str);
      }
      body = mputstr(body,
        "TTCN_Runtime::function_started(function_arguments);\n");
      char *actual_par_list =
        fp_list->generate_code_actual_parlist(memptystr(), "");
      bool return_value_kept = false;
      if (asstype == A_FUNCTION_RVAL) {
        // the return value is kept only if the function returns a value
        // (rather than a template) and the return type has the "done"
        // extension attribute
        for (Type *t = return_type; ; t = t->get_type_refd()) {
          if (t->has_done_attribute()) {
            return_value_kept = true;
            break;
          } else if (!t->is_ref()) break;
        }
      }
      if (return_value_kept) {
        const string& return_type_dispname = return_type->get_typename();
        const char *return_type_dispname_str = return_type_dispname.c_str();
        body = mputprintf(body, "%s ret_val(%s(%s));\n"
          "TTCN_Logger::begin_event(TTCN_PARALLEL);\n"
          "TTCN_Logger::log_event_str(\"Function %s returned %s : \");\n"
          "ret_val.log();\n"
          "Text_Buf text_buf;\n"
          "TTCN_Runtime::prepare_function_finished(\"%s\", text_buf);\n"
          "ret_val.encode_text(text_buf);\n"
          "TTCN_Runtime::send_function_finished(text_buf);\n",
          return_type_str, genname_str, actual_par_list, dispname_str,
          return_type_dispname_str, return_type_dispname_str);
      } else {
        body = mputprintf(body, "%s(%s);\n"
          "TTCN_Runtime::function_finished(\"%s\");\n",
          genname_str, actual_par_list, dispname_str);
      }
      Free(actual_par_list);
      body = mputstr(body, "return TRUE;\n"
        "} else ");
      target->functions.start = mputstr(target->functions.start, body);
      Free(body);
      Free(full_formal_par_list);
    }

    target->functions.pre_init = mputprintf(target->functions.pre_init,
      "%s.add_function(\"%s\", (genericfunc_t)&%s, ", get_module_object_name(),
      dispname_str, genname_str);
    if(is_startable)
      target->functions.pre_init = mputprintf(target->functions.pre_init,
        "(genericfunc_t)&start_%s);\n", genname_str);
    else
      target->functions.pre_init = mputstr(target->functions.pre_init,
        "NULL);\n");
  }

  void Def_Function::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  void Def_Function::dump_internal(unsigned level) const
  {
    DEBUG(level, "Function: %s", id->get_dispname().c_str());
    DEBUG(level + 1, "Parameters:");
    fp_list->dump(level + 1);
    if (runs_on_ref) {
      DEBUG(level + 1, "Runs on clause:");
      runs_on_ref->dump(level + 2);
    }
    if (return_type) {
      DEBUG(level + 1, "Return type:");
      return_type->dump(level + 2);
      if (asstype == A_FUNCTION_RTEMP) DEBUG(level + 1, "Returns template");
    }
    if (prototype != PROTOTYPE_NONE)
      DEBUG(level + 1, "Prototype: %s", get_prototype_name());
    //DEBUG(level + 1, "Statement block:");
    block->dump(level + 1);
  }

  void Def_Function::set_parent_path(WithAttribPath* p_path) {
    Def_Function_Base::set_parent_path(p_path);
    block->set_parent_path(w_attrib_path);
  }

  // =================================
  // ===== Def_ExtFunction
  // =================================

  Def_ExtFunction::~Def_ExtFunction()
  {
    delete encoding_options;
    delete eb_list;
    if (NULL != json_printing) {
      delete json_printing;
    }
  }

  Def_ExtFunction *Def_ExtFunction::clone() const
  {
    FATAL_ERROR("Def_ExtFunction::clone");
  }

  void Def_ExtFunction::set_fullname(const string& p_fullname)
  {
    Def_Function_Base::set_fullname(p_fullname);
    if (eb_list) eb_list->set_fullname(p_fullname + ".<errorbehavior_list>");
  }

  void Def_ExtFunction::set_encode_parameters(Type::MessageEncodingType_t
    p_encoding_type, string *p_encoding_options)
  {
    function_type = EXTFUNC_ENCODE;
    encoding_type = p_encoding_type;
    delete encoding_options;
    encoding_options = p_encoding_options;
  }

  void Def_ExtFunction::set_decode_parameters(Type::MessageEncodingType_t
    p_encoding_type, string *p_encoding_options)
  {
    function_type = EXTFUNC_DECODE;
    encoding_type = p_encoding_type;
    delete encoding_options;
    encoding_options = p_encoding_options;
  }

  void Def_ExtFunction::add_eb_list(Ttcn::ErrorBehaviorList *p_eb_list)
  {
    if (!p_eb_list) FATAL_ERROR("Def_ExtFunction::add_eb_list()");
    if (eb_list) {
      eb_list->steal_ebs(p_eb_list);
      delete p_eb_list;
    } else {
      eb_list = p_eb_list;
      eb_list->set_fullname(get_fullname() + ".<errorbehavior_list>");
    }
  }

  void Def_ExtFunction::chk_function_type()
  {
    switch (function_type) {
    case EXTFUNC_MANUAL:
      if (eb_list) {
        eb_list->error("Attribute `errorbehavior' can only be used together "
          "with `encode' or `decode'");
        eb_list->chk();
      }
      break;
    case EXTFUNC_ENCODE:
      switch (prototype) {
      case PROTOTYPE_NONE:
        error("Attribute `encode' cannot be used without `prototype'");
        break;
      case PROTOTYPE_BACKTRACK:
      case PROTOTYPE_SLIDING:
        error("Attribute `encode' cannot be used with `prototype(%s)'",
          get_prototype_name());
      default: /* CONVERT and FAST allowed */
        break;
      }

      if (input_type) {
        if (!input_type->has_encoding(encoding_type, encoding_options)) {
          if (Common::Type::CT_CUSTOM == encoding_type) {
            input_type->error("Input type `%s' does not support custom encoding '%s'",
              input_type->get_typename().c_str(), encoding_options->c_str());
          }
          else {
            // First collect the fields of a record, set, union type which
            // does not support the encoding, then write it in the error message.
            char* message = NULL;
            Type *t = input_type;
            if (t->is_ref()) t = t->get_type_refd();
            switch(t->get_typetype()) {
              case Type::T_SEQ_T:
              case Type::T_SET_T:
              case Type::T_CHOICE_T: {
                for (size_t i = 0; i < t->get_nof_comps(); i++) {
                  if (!t->get_comp_byIndex(i)->get_type()->has_encoding(encoding_type, encoding_options)) {
                    if (i == 0) {
                      message = mputprintf(message, " The following fields do not support %s encoding: ",
                        Type::get_encoding_name(encoding_type));
                    } else {
                      message = mputstr(message, ", ");
                    }
                    message = mputstr(message, t->get_comp_id_byIndex(i).get_ttcnname().c_str());
                  }
                }
                break; }
              default:
                break;
            }
            input_type->error("Input type `%s' does not support %s encoding.%s",
              input_type->get_typename().c_str(),
              Type::get_encoding_name(encoding_type), message == NULL ? "" : message);
            Free(message);
          }
        }
        else {
          if (Common::Type::CT_XER == encoding_type) {
            Type* last = input_type->get_type_refd_last();
            if (last->is_untagged() &&
               ((last->get_typetype() != Type::T_CHOICE_A &&
               last->get_typetype() != Type::T_CHOICE_T &&
               last->get_typetype() != Type::T_ANYTYPE) || legacy_untagged_union == TRUE)) {
              // "untagged" on the (toplevel) input type will have no effect,
              // unless it is union or anytype
              warning("UNTAGGED encoding attribute is ignored on top-level type");
            }
          }
          if (Common::Type::CT_CUSTOM == encoding_type ||
              Common::Type::CT_PER == encoding_type) {
            if (PROTOTYPE_CONVERT != prototype) {
              error("Only `prototype(convert)' is allowed for %s encoding functions",
                Type::get_encoding_name(encoding_type));
            }
            else if (input_type->is_ref()) {
              // let the input type know that this is its encoding function
              input_type->get_type_refd()->set_coding_function(true, this);
              // treat this as a manual external function during code generation
              function_type = EXTFUNC_MANUAL;
            }
          }
          else if (input_type->is_ref() && input_type->get_type_refd()->is_asn1()) {
            // let the input ASN.1 type know that this is its encoding type
            input_type->get_type_refd()->set_asn_coding(true, encoding_type);
          }
        }
      }
      if (output_type) {
        if(encoding_type == Common::Type::CT_TEXT) {
          // TEXT encoding supports bitstring, octetstring and charstring stream types
          Type *stream_type = Type::get_stream_type(encoding_type,0);
          Type *stream_type2 = Type::get_stream_type(encoding_type,1);
          Type *stream_type3 = Type::get_stream_type(encoding_type,2);
          if ( (!stream_type->is_identical(output_type)) &&
               (!stream_type2->is_identical(output_type)) &&
               (!stream_type3->is_identical(output_type))) {
            output_type->error("The output type of %s encoding should be `%s', "
              "`%s' or `%s' instead of `%s'", Type::get_encoding_name(encoding_type),
              stream_type->get_typename().c_str(),
              stream_type2->get_typename().c_str(),
              stream_type3->get_typename().c_str(),
              output_type->get_typename().c_str());
          }
        }
        else if (encoding_type == Common::Type::CT_CUSTOM ||
                 encoding_type == Common::Type::CT_PER) {
          // custom and PER encodings only support the bitstring stream type
          Type *stream_type = Type::get_stream_type(encoding_type);
          if (!stream_type->is_identical(output_type)) {
            output_type->error("The output type of %s encoding should be `%s' "
              "instead of `%s'", Type::get_encoding_name(encoding_type),
              stream_type->get_typename().c_str(),
              output_type->get_typename().c_str());
          }
        }
        else {
          // all other encodings support bitstring and octetstring stream types
          Type *stream_type = Type::get_stream_type(encoding_type, 0);
          Type *stream_type2 = Type::get_stream_type(encoding_type, 1);
          if (!stream_type->is_identical(output_type) &&
              !stream_type2->is_identical(output_type)) {
            output_type->error("The output type of %s encoding should be `%s' "
              "or '%s' instead of `%s'", Type::get_encoding_name(encoding_type),
              stream_type->get_typename().c_str(),
              stream_type2->get_typename().c_str(),
              output_type->get_typename().c_str());
          }
        }
      }
      if (eb_list) eb_list->chk();
      chk_allowed_encode();
      break;
    case EXTFUNC_DECODE:
      if (prototype == PROTOTYPE_NONE) {
        error("Attribute `decode' cannot be used without `prototype'");
      }
      if (input_type) {
        if(encoding_type == Common::Type::CT_TEXT) {
          // TEXT encoding supports bitstring, octetstring and charstring stream types
          Type *stream_type = Type::get_stream_type(encoding_type,0);
          Type *stream_type2 = Type::get_stream_type(encoding_type,1);
          Type *stream_type3 = Type::get_stream_type(encoding_type,2);
          if ( (!stream_type->is_identical(input_type)) &&
               (!stream_type2->is_identical(input_type)) &&
               (!stream_type3->is_identical(input_type))) {
            input_type->error("The input type of %s decoding should be `%s', "
            "`%s' or `%s' instead of `%s'", Type::get_encoding_name(encoding_type),
              stream_type->get_typename().c_str(),
              stream_type2->get_typename().c_str(),
              stream_type3->get_typename().c_str(),
              input_type->get_typename().c_str());
          }
        }
        else if (encoding_type == Common::Type::CT_CUSTOM ||
                 encoding_type == Common::Type::CT_PER) {
          // custom and PER encodings only support the bitstring stream type
          Type *stream_type = Type::get_stream_type(encoding_type);
          if (!stream_type->is_identical(input_type)) {
            input_type->error("The input type of %s decoding should be `%s' "
              "instead of `%s'", Type::get_encoding_name(encoding_type),
              stream_type->get_typename().c_str(),
              input_type->get_typename().c_str());
          }
        }
        else {
          // all other encodings support bitstring and octetstring stream types
          Type *stream_type = Type::get_stream_type(encoding_type, 0);
          Type *stream_type2 = Type::get_stream_type(encoding_type, 1);
          if (!stream_type->is_identical(input_type) &&
              !stream_type2->is_identical(input_type)) {
            input_type->error("The input type of %s decoding should be `%s' "
              "or `%s' instead of `%s'", Type::get_encoding_name(encoding_type),
              stream_type->get_typename().c_str(),
              stream_type2->get_typename().c_str(),
              input_type->get_typename().c_str());
          }
        }
      }
      if (output_type && !output_type->has_encoding(encoding_type, encoding_options)) {
        if (Common::Type::CT_CUSTOM == encoding_type) {
          output_type->error("Output type `%s' does not support custom encoding '%s'",
            output_type->get_typename().c_str(), encoding_options->c_str());
        }
        else {
          output_type->error("Output type `%s' does not support %s encoding",
            output_type->get_typename().c_str(),
            Type::get_encoding_name(encoding_type));
        }
      }
      else {
        if (Common::Type::CT_CUSTOM == encoding_type ||
            Common::Type::CT_PER == encoding_type) {
          if (PROTOTYPE_SLIDING != prototype) {
            error("Only `prototype(sliding)' is allowed for %s decoding functions",
              Type::get_encoding_name(encoding_type));
          }
          else if (output_type != NULL && output_type->is_ref()) {
            // let the output type know that this is its decoding function
            output_type->get_type_refd()->set_coding_function(false, this);
            // treat this as a manual external function during code generation
            function_type = EXTFUNC_MANUAL;
          }
        }
        else if (output_type != NULL && output_type->is_ref() &&
                 output_type->get_type_refd()->is_asn1()) {
          // let the output ASN.1 type know that this is its decoding type
          output_type->get_type_refd()->set_asn_coding(false, encoding_type);
        }
      }
      if (eb_list) eb_list->chk();
      chk_allowed_encode();
      break;
    default:
      FATAL_ERROR("Def_ExtFunction::chk()");
    }
  }

  void Def_ExtFunction::chk_allowed_encode()
  {
    switch (encoding_type) {
    case Type::CT_BER:
      if (enable_ber()) return; // ok
      break;
    case Type::CT_RAW:
      if (enable_raw()) return; // ok
      break;
    case Type::CT_TEXT:
      if (enable_text()) return; // ok
      break;
    case Type::CT_XER:
      if (enable_xer()) return; // ok
      break;
    case Type::CT_PER:
      if (enable_per()) return; // ok?
      break;
    case Type::CT_JSON:
      if (enable_json()) return;
      break;
    case Type::CT_CUSTOM:
      return; // cannot be disabled
    default:
      FATAL_ERROR("Def_ExtFunction::chk_allowed_encode");
      break;
    }

    error("%s encoding is disallowed by license or commandline options",
      Type::get_encoding_name(encoding_type));
  }

  void Def_ExtFunction::chk()
  {
    if (checked) return;
    checked = true;
    Error_Context cntxt(this, "In external function definition `%s'",
      id->get_dispname().c_str());
    fp_list->chk(asstype);
    if (return_type) {
      Error_Context cntxt2(return_type, "In return type");
      return_type->chk();
      return_type->chk_as_return_type(asstype == A_EXT_FUNCTION_RVAL,
        "external function");
    }
    if (!semantic_check_only) fp_list->set_genname(get_genname());
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
      const Ttcn::ExtensionAttributes * extattrs = parse_extattributes(w_attrib_path);
      if (extattrs != 0) {
        size_t num_atrs = extattrs->size();
        for (size_t i=0; i < num_atrs; ++i) {
          ExtensionAttribute &ea = extattrs->get(i);
          switch (ea.get_type()) {
          case ExtensionAttribute::PROTOTYPE: {
            if (get_prototype() != Def_Function_Base::PROTOTYPE_NONE) {
              ea.error("Duplicate attribute `prototype'");
            }
            Def_Function_Base::prototype_t proto = ea.get_proto();
            set_prototype(proto);
            break; }

          case ExtensionAttribute::ENCODE: {
            switch (get_function_type()) {
            case Def_ExtFunction::EXTFUNC_MANUAL:
              break;
            case Def_ExtFunction::EXTFUNC_ENCODE: {
              ea.error("Duplicate attribute `encode'");
              break; }
            case Def_ExtFunction::EXTFUNC_DECODE: {
              ea.error("Attributes `decode' and `encode' "
                "cannot be used at the same time");
              break; }
            default:
              FATAL_ERROR("coding_attrib_parse(): invalid external function type");
            }
            Type::MessageEncodingType_t et;
            string *opt;
            ea.get_encdec_parameters(et, opt);
            set_encode_parameters(et, opt);
            break; }

          case ExtensionAttribute::ERRORBEHAVIOR: {
            add_eb_list(ea.get_eb_list());
            break; }

          case ExtensionAttribute::DECODE: {
            switch (get_function_type()) {
            case Def_ExtFunction::EXTFUNC_MANUAL:
              break;
            case Def_ExtFunction::EXTFUNC_ENCODE: {
              ea.error("Attributes `encode' and `decode' "
                "cannot be used at the same time");
              break; }
            case Def_ExtFunction::EXTFUNC_DECODE: {
              ea.error("Duplicate attribute `decode'");
              break; }
            default:
              FATAL_ERROR("coding_attrib_parse(): invalid external function type");
            }
            Type::MessageEncodingType_t et;
            string *opt;
            ea.get_encdec_parameters(et, opt);
            set_decode_parameters(et, opt);
            break; }
          
          case ExtensionAttribute::PRINTING: {
            json_printing = ea.get_printing();
            break; }

          case ExtensionAttribute::ANYTYPELIST:
            // ignore, because we can't distinguish between a local
            // "extension anytype" (which is bogus) and an inherited one
            // (which was meant for a type definition)
            break;

          case ExtensionAttribute::NONE:
            // Ignore, do not issue "wrong type" error
            break;

          default:
            ea.error(
              "Only the following extension attributes may be applied to "
              "external functions: 'prototype', 'encode', 'decode', 'errorbehavior'");
            break;
          } // switch type
        } // next attribute
        delete extattrs;
      } // if extatrs
    }
    chk_prototype();
    chk_function_type();
    
    if (NULL != json_printing && (EXTFUNC_ENCODE != function_type ||
        Type::CT_JSON != encoding_type)) {
      error("Attribute 'printing' is only allowed for JSON encoding functions.");
    }
  }

  char *Def_ExtFunction::generate_code_encode(char *str)
  {
    const char *function_name = id->get_dispname().c_str();
    const char *first_par_name =
      fp_list->get_fp_byIndex(0)->get_id().get_name().c_str();
    // producing debug printout of the input PDU
    str = mputprintf(str,
#ifndef NDEBUG
      "// written by %s in " __FILE__ " at %d\n"
#endif
      "if (TTCN_Logger::log_this_event(TTCN_Logger::DEBUG_ENCDEC)) {\n"
      "TTCN_Logger::begin_event(TTCN_Logger::DEBUG_ENCDEC);\n"
      "TTCN_Logger::log_event_str(\"%s(): Encoding %s: \");\n"
      "%s.log();\n"
      "TTCN_Logger::end_event();\n"
      "}\n"
#ifndef NDEBUG
      , __FUNCTION__, __LINE__
#endif
      , function_name, input_type->get_typename().c_str(), first_par_name);
    // setting error behavior
    if (eb_list) str = eb_list->generate_code(str);
    else str = mputstr(str, "TTCN_EncDec::set_error_behavior("
      "TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_DEFAULT);\n");
    // encoding PDU into the buffer
    str = mputstr(str, "TTCN_Buffer ttcn_buffer;\n");
    str = mputprintf(str, "%s.encode(%s_descr_, ttcn_buffer, TTCN_EncDec::CT_%s",
      first_par_name,
      input_type->get_genname_typedescriptor(my_scope).c_str(),
      Type::get_encoding_name(encoding_type));
    if (encoding_type == Type::CT_JSON) {
      if (json_printing != NULL) {
        str = json_printing->generate_code(str);
      } else {
        str = mputstr(str, ", 0");
      }
    }
    if (encoding_options) str = mputprintf(str, ", %s",
      encoding_options->c_str());
    str = mputstr(str, ");\n");
    const char *result_name;
    switch (prototype) {
    case PROTOTYPE_CONVERT:
      result_name = "ret_val";
      // creating a local variable for the result stream
      str = mputprintf(str, "%s ret_val;\n",
        output_type->get_genname_value(my_scope).c_str());
      break;
    case PROTOTYPE_FAST:
      result_name = fp_list->get_fp_byIndex(1)->get_id().get_name().c_str();
      break;
    default:
      FATAL_ERROR("Def_ExtFunction::generate_code_encode()");
      result_name = 0;
    }
    // taking the result from the buffer and producing debug printout
    if (output_type->get_type_refd_last()->get_typetype_ttcn3() ==
        Common::Type::T_BSTR) {
      // cannot extract a bitstring from the buffer, use temporary octetstring
      // and convert it to bitstring
      str = mputprintf(str,
        "OCTETSTRING tmp_os;\n"
        "ttcn_buffer.get_string(tmp_os);\n"
        "%s = oct2bit(tmp_os);\n", result_name);
    }
    else {
      str = mputprintf(str, "ttcn_buffer.get_string(%s);\n", result_name);
    }
    str = mputprintf(str,
      "if (TTCN_Logger::log_this_event(TTCN_Logger::DEBUG_ENCDEC)) {\n"
      "TTCN_Logger::begin_event(TTCN_Logger::DEBUG_ENCDEC);\n"
      "TTCN_Logger::log_event_str(\"%s(): Stream after encoding: \");\n"
      "%s.log();\n"
      "TTCN_Logger::end_event();\n"
      "}\n", function_name, result_name);
    // returning the result stream if necessary
    if (prototype == PROTOTYPE_CONVERT) {
      if (debugger_active) {
        str = mputstr(str,
          "ttcn3_debugger.set_return_value((TTCN_Logger::begin_event_log2str(), "
          "ret_val.log(), TTCN_Logger::end_event_log2str()));\n");
      }
      str = mputstr(str, "return ret_val;\n");
    }
    return str;
  }

  char *Def_ExtFunction::generate_code_decode(char *str)
  {
    const char *function_name = id->get_dispname().c_str();
    const char *first_par_name =
      fp_list->get_fp_byIndex(0)->get_id().get_name().c_str();
    // producing debug printout of the input stream
    str = mputprintf(str,
#ifndef NDEBUG
      "// written by %s in " __FILE__ " at %d\n"
#endif
      "if (TTCN_Logger::log_this_event(TTCN_Logger::DEBUG_ENCDEC)) {\n"
      "TTCN_Logger::begin_event(TTCN_Logger::DEBUG_ENCDEC);\n"
      "TTCN_Logger::log_event_str(\"%s(): Stream before decoding: \");\n"
      "%s.log();\n"
      "TTCN_Logger::end_event();\n"
      "}\n"
#ifndef NDEBUG
      , __FUNCTION__, __LINE__
#endif
      , function_name, first_par_name);
    // setting error behavior
    if (eb_list) str = eb_list->generate_code(str);
    else if (prototype == PROTOTYPE_BACKTRACK || prototype == PROTOTYPE_SLIDING) {
            str = mputstr(str, "TTCN_EncDec::set_error_behavior("
      "TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);\n");
    } else str = mputstr(str, "TTCN_EncDec::set_error_behavior("
      "TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_DEFAULT);\n");
    str = mputstr(str, "TTCN_EncDec::clear_error();\n");
    // creating a buffer from the input stream
    if (input_type->get_type_refd_last()->get_typetype_ttcn3() ==
        Common::Type::T_BSTR) {
      // cannot create a buffer from a bitstring, convert it to octetstring
      str = mputprintf(str, "TTCN_Buffer ttcn_buffer(bit2oct(%s));\n",
        first_par_name);
    }
    else {
      str = mputprintf(str, "TTCN_Buffer ttcn_buffer(%s);\n", first_par_name);
    }
    const char *result_name;
    if (prototype == PROTOTYPE_CONVERT) {
      // creating a local variable for the result
      str = mputprintf(str, "%s ret_val;\n",
        output_type->get_genname_value(my_scope).c_str());
      result_name = "ret_val";
    } else {
      result_name = fp_list->get_fp_byIndex(1)->get_id().get_name().c_str();
    }
    if(encoding_type==Type::CT_TEXT){
    str = mputprintf(str,
      "if (TTCN_Logger::log_this_event(TTCN_Logger::DEBUG_ENCDEC)) {\n"
      "  TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_LOG_MATCHING, TTCN_EncDec::EB_WARNING);\n"
      "}\n");
    }
    str = mputprintf(str, "%s.decode(%s_descr_, ttcn_buffer, "
      "TTCN_EncDec::CT_%s", result_name,
      output_type->get_genname_typedescriptor(my_scope).c_str(),
      Type::get_encoding_name(encoding_type));
    if (encoding_options) str = mputprintf(str, ", %s",
      encoding_options->c_str());
    str = mputstr(str, ");\n");
    // producing debug printout of the result PDU
    str = mputprintf(str,
      "if (TTCN_Logger::log_this_event(TTCN_Logger::DEBUG_ENCDEC)) {\n"
      "TTCN_Logger::begin_event(TTCN_Logger::DEBUG_ENCDEC);\n"
      "TTCN_Logger::log_event_str(\"%s(): Decoded %s: \");\n"
      "%s.log();\n"
      "TTCN_Logger::end_event();\n"
      "}\n", function_name, output_type->get_typename().c_str(), result_name);
    if (prototype != PROTOTYPE_SLIDING) {
      // checking for remaining data in the buffer if decoding was successful
      str = mputprintf(str, "if (TTCN_EncDec::get_last_error_type() == "
          "TTCN_EncDec::ET_NONE) {\n"
        "if (ttcn_buffer.get_pos() < ttcn_buffer.get_len()-1 && "
          "TTCN_Logger::log_this_event(TTCN_WARNING)) {\n"
        "ttcn_buffer.cut();\n"
        "%s remaining_stream;\n",
        input_type->get_genname_value(my_scope).c_str());
      if (input_type->get_type_refd_last()->get_typetype_ttcn3() ==
          Common::Type::T_BSTR) {
        str = mputstr(str,
          "OCTETSTRING tmp_os;\n"
          "ttcn_buffer.get_string(tmp_os);\n"
          "remaining_stream = oct2bit(tmp_os);\n");
      }
      else {
        str = mputstr(str, "ttcn_buffer.get_string(remaining_stream);\n");
      }
      str = mputprintf(str,
        "TTCN_Logger::begin_event(TTCN_WARNING);\n"
        "TTCN_Logger::log_event_str(\"%s(): Warning: Data remained at the end "
          "of the stream after successful decoding: \");\n"
        "remaining_stream.log();\n"
        "TTCN_Logger::end_event();\n"
        "}\n", function_name);
      // closing the block and returning the appropriate result or status code
      if (prototype == PROTOTYPE_BACKTRACK) {
        if (debugger_active) {
          str = mputstr(str, "ttcn3_debugger.set_return_value(\"0\");\n");
        }
        str = mputstr(str,
          "return 0;\n"
          "} else {\n");
        if (debugger_active) {
          str = mputstr(str, "ttcn3_debugger.set_return_value(\"1\");\n");
        }
        str = mputstr(str,  
          "return 1;\n"
          "}\n");
      } else {
        str = mputstr(str, "}\n");
        if (prototype == PROTOTYPE_CONVERT) {
          if (debugger_active) {
            str = mputstr(str,
              "ttcn3_debugger.set_return_value((TTCN_Logger::begin_event_log2str(), "
              "ret_val.log(), TTCN_Logger::end_event_log2str()));\n");
          }
          str = mputstr(str, "return ret_val;\n");
        }
      }
    } else {
      // result handling and debug printout for sliding decoders
      str = mputstr(str, "switch (TTCN_EncDec::get_last_error_type()) {\n"
        "case TTCN_EncDec::ET_NONE:\n"
        // TTCN_Buffer::get_string will call OCTETSTRING::clean_up()
        "ttcn_buffer.cut();\n");
      if (input_type->get_type_refd_last()->get_typetype_ttcn3() ==
          Common::Type::T_BSTR) {
        str = mputprintf(str,
          "OCTETSTRING tmp_os;\n"
          "ttcn_buffer.get_string(tmp_os);\n"
          "%s = oct2bit(tmp_os);\n", first_par_name);
      }
      else {
        str = mputprintf(str, "ttcn_buffer.get_string(%s);\n", first_par_name);
      }
      str = mputprintf(str,
        "if (TTCN_Logger::log_this_event(TTCN_Logger::DEBUG_ENCDEC)) {\n"
        "TTCN_Logger::begin_event(TTCN_Logger::DEBUG_ENCDEC);\n"
        "TTCN_Logger::log_event_str(\"%s(): Stream after decoding: \");\n"
        "%s.log();\n"
        "TTCN_Logger::end_event();\n"
        "}\n"
        "%sreturn 0;\n"
        "case TTCN_EncDec::ET_INCOMPL_MSG:\n"
        "case TTCN_EncDec::ET_LEN_ERR:\n"
        "%sreturn 2;\n"
        "default:\n"
        "%sreturn 1;\n"
        "}\n", function_name, first_par_name,
        debugger_active ? "ttcn3_debugger.set_return_value(\"0\");\n" : "",
        debugger_active ? "ttcn3_debugger.set_return_value(\"2\");\n" : "",
        debugger_active ? "ttcn3_debugger.set_return_value(\"1\");\n" : "");
    }
    return str;
  }

  void Def_ExtFunction::generate_code(output_struct *target, bool)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    string return_type_name;
    switch (asstype) {
    case A_EXT_FUNCTION:
      return_type_name = "void";
      break;
    case A_EXT_FUNCTION_RVAL:
      return_type_name = return_type->get_genname_value(my_scope);
      break;
    case A_EXT_FUNCTION_RTEMP:
      return_type_name = return_type->get_genname_template(my_scope);
      break;
    default:
      FATAL_ERROR("Def_ExtFunction::generate_code()");
    }
    const char *return_type_str = return_type_name.c_str();
    char *formal_par_list = fp_list->generate_code(memptystr(), fp_list->get_nof_fps());
    fp_list->generate_code_defval(target);
    // function prototype
    target->header.function_prototypes =
      mputprintf(target->header.function_prototypes, "extern %s %s(%s);\n",
        return_type_str, genname_str, formal_par_list);

    if (function_type != EXTFUNC_MANUAL) {
      // function body written by the compiler
      char *body = 0;
#ifndef NDEBUG
      body = mprintf("// written by %s in " __FILE__ " at %d\n"
        , __FUNCTION__, __LINE__);
#endif
      body = mputprintf(body,
        "%s %s(%s)\n"
        "{\n"
        , return_type_str, genname_str, formal_par_list);
      if (debugger_active) {
        body = generate_code_debugger_function_init(body, this);
      }
      switch (function_type) {
      case EXTFUNC_ENCODE:
        body = generate_code_encode(body);
        break;
      case EXTFUNC_DECODE:
        body = generate_code_decode(body);
        break;
      default:
        FATAL_ERROR("Def_ExtFunction::generate_code()");
      }
      body = mputstr(body, "}\n\n");
      target->source.function_bodies = mputstr(target->source.function_bodies,
        body);
      Free(body);
    }

    Free(formal_par_list);

    target->functions.pre_init = mputprintf(target->functions.pre_init,
      "%s.add_function(\"%s\", (genericfunc_t)&%s, NULL);\n",
      get_module_object_name(), id->get_dispname().c_str(), genname_str);
  }

  void Def_ExtFunction::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  void Def_ExtFunction::dump_internal(unsigned level) const
  {
    DEBUG(level, "External function: %s", id->get_dispname().c_str());
    DEBUG(level + 1, "Parameters:");
    fp_list->dump(level + 2);
    if (return_type) {
      DEBUG(level + 1, "Return type:");
      return_type->dump(level + 2);
      if(asstype == A_EXT_FUNCTION_RTEMP) DEBUG(level + 1, "Returns template");
    }
    if (prototype != PROTOTYPE_NONE)
      DEBUG(level + 1, "Prototype: %s", get_prototype_name());
    if (function_type != EXTFUNC_MANUAL) {
      DEBUG(level + 1, "Automatically generated: %s",
        function_type == EXTFUNC_ENCODE ? "encoder" : "decoder");
      DEBUG(level + 2, "Encoding type: %s",
        Type::get_encoding_name(encoding_type));
      if (encoding_options)
        DEBUG(level + 2, "Encoding options: %s", encoding_options->c_str());
    }
    if (eb_list) eb_list->dump(level + 1);
  }
  
  void Def_ExtFunction::generate_json_schema_ref(map<Type*, JSON_Tokenizer>& json_refs)
  {
    // only do anything if this is a JSON encoding or decoding function
    if (encoding_type == Type::CT_JSON && 
        (function_type == EXTFUNC_ENCODE || function_type == EXTFUNC_DECODE)) {
      // retrieve the encoded type
      Type* type = NULL;
      if (function_type == EXTFUNC_ENCODE) {
        // for encoding functions it's always the first parameter
        type = fp_list->get_fp_byIndex(0)->get_Type();
      } else {
        // for decoding functions it depends on the prototype
        switch (prototype) {
        case PROTOTYPE_CONVERT:
          type = return_type;
          break;
        case PROTOTYPE_FAST:
        case PROTOTYPE_BACKTRACK:
        case PROTOTYPE_SLIDING:
          type = fp_list->get_fp_byIndex(1)->get_Type();
          break;
        default:
          FATAL_ERROR("Def_ExtFunction::generate_json_schema_ref");
        }
      }
      
      // step over the type reference created for this function
      type = type->get_type_refd();
      
      JSON_Tokenizer* json = NULL;
      if (json_refs.has_key(type)) {
        // the schema segment containing the type's reference already exists
        json = json_refs[type];
      } else {
        // the schema segment doesn't exist yet, create it and insert the reference
        json = new JSON_Tokenizer;
        json_refs.add(type, json);
        type->generate_json_schema_ref(*json);
      }
      
      // insert a property to specify which function this is (encoding or decoding)
      json->put_next_token(JSON_TOKEN_NAME, (function_type == EXTFUNC_ENCODE) ?
        "encoding" : "decoding");
      
      // place the function's info in an object
      json->put_next_token(JSON_TOKEN_OBJECT_START);
      
      // insert information related to the function's prototype in an array
      json->put_next_token(JSON_TOKEN_NAME, "prototype");
      json->put_next_token(JSON_TOKEN_ARRAY_START);
      
      // 1st element: external function prototype name (as string)
      switch(prototype) {
      case PROTOTYPE_CONVERT:
        json->put_next_token(JSON_TOKEN_STRING, "\"convert\"");
        break;
      case PROTOTYPE_FAST:
        json->put_next_token(JSON_TOKEN_STRING, "\"fast\"");
        break;
      case PROTOTYPE_BACKTRACK:
        json->put_next_token(JSON_TOKEN_STRING, "\"backtrack\"");
        break;
      case PROTOTYPE_SLIDING:
        json->put_next_token(JSON_TOKEN_STRING, "\"sliding\"");
        break;
      default:
        FATAL_ERROR("Def_ExtFunction::generate_json_schema_ref");
      }
      
      // 2nd element: external function name
      char* func_name_str = mprintf("\"%s\"", id->get_dispname().c_str());
      json->put_next_token(JSON_TOKEN_STRING, func_name_str);
      Free(func_name_str);
      
      // the rest of the elements contain the names of the function's parameters (1 or 2)
      for (size_t i = 0; i < fp_list->get_nof_fps(); ++i) {
        char* param_str = mprintf("\"%s\"", 
          fp_list->get_fp_byIndex(i)->get_id().get_dispname().c_str());
        json->put_next_token(JSON_TOKEN_STRING, param_str);
        Free(param_str);
      }
      
      // end of the prototype's array
      json->put_next_token(JSON_TOKEN_ARRAY_END);
      
      // insert error behavior data
      if (eb_list != NULL) {
        json->put_next_token(JSON_TOKEN_NAME, "errorBehavior");
        json->put_next_token(JSON_TOKEN_OBJECT_START);
        
        // add each error behavior modification as a property
        for (size_t i = 0; i < eb_list->get_nof_ebs(); ++i) {
          ErrorBehaviorSetting* eb = eb_list->get_ebs_byIndex(i);
          json->put_next_token(JSON_TOKEN_NAME, eb->get_error_type().c_str());
          char* handling_str = mprintf("\"%s\"", eb->get_error_handling().c_str());
          json->put_next_token(JSON_TOKEN_STRING, handling_str);
          Free(handling_str);
        }
        
        json->put_next_token(JSON_TOKEN_OBJECT_END);
      }
      
      // insert printing type
      if (json_printing != NULL) {
        json->put_next_token(JSON_TOKEN_NAME, "printing");
        json->put_next_token(JSON_TOKEN_STRING, 
          (json_printing->get_printing() == PrintingType::PT_PRETTY) ? 
          "\"pretty\"" : "\"compact\"");
      }
      
      // end of this function's object
      json->put_next_token(JSON_TOKEN_OBJECT_END);
    }
  }

  // =================================
  // ===== Def_Altstep
  // =================================

  Def_Altstep::Def_Altstep(Identifier *p_id, FormalParList *p_fpl,
                           Reference *p_runs_on_ref, StatementBlock *p_sb,
                           AltGuards *p_ags)
    : Definition(A_ALTSTEP, p_id), fp_list(p_fpl), runs_on_ref(p_runs_on_ref),
      runs_on_type(0), sb(p_sb), ags(p_ags)
  {
    if (!p_fpl || !p_sb || !p_ags)
      FATAL_ERROR("Def_Altstep::Def_Altstep()");
    fp_list->set_my_def(this);
    sb->set_my_def(this);
    ags->set_my_def(this);
    ags->set_my_sb(sb, 0);
  }

  Def_Altstep::~Def_Altstep()
  {
    delete fp_list;
    delete runs_on_ref;
    delete sb;
    delete ags;
  }

  Def_Altstep *Def_Altstep::clone() const
  {
    FATAL_ERROR("Def_Altstep::clone");
  }

  void Def_Altstep::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    fp_list->set_fullname(p_fullname + ".<formal_par_list>");
    if (runs_on_ref) runs_on_ref->set_fullname(p_fullname + ".<runs_on_type>");
    sb->set_fullname(p_fullname+".<block>");
    ags->set_fullname(p_fullname + ".<guards>");
  }

  void Def_Altstep::set_my_scope(Scope *p_scope)
  {
    bridgeScope.set_parent_scope(p_scope);
    bridgeScope.set_scopeMacro_name(id->get_dispname());

    Definition::set_my_scope(&bridgeScope);
    // the scope of the parameter list is set during checking
    if (runs_on_ref) runs_on_ref->set_my_scope(&bridgeScope);
    sb->set_my_scope(fp_list);
    ags->set_my_scope(sb);
  }

  Type *Def_Altstep::get_RunsOnType()
  {
    if (!checked) chk();
    return runs_on_type;
  }

  FormalParList *Def_Altstep::get_FormalParList()
  {
    if (!checked) chk();
    return fp_list;
  }

  RunsOnScope *Def_Altstep::get_runs_on_scope(Type *comptype)
  {
    Module *my_module = dynamic_cast<Module*>(my_scope->get_scope_mod());
    if (!my_module) FATAL_ERROR("Def_Altstep::get_runs_on_scope()");
    return my_module->get_runs_on_scope(comptype);
  }

  void Def_Altstep::chk()
  {
    if (checked) return;
    checked = true;
    Error_Context cntxt(this, "In altstep definition `%s'",
      id->get_dispname().c_str());
    Scope *parlist_scope = my_scope;
    if (runs_on_ref) {
      Error_Context cntxt2(runs_on_ref, "In `runs on' clause");
      runs_on_type = runs_on_ref->chk_comptype_ref();
      if (runs_on_type) {
        Scope *runs_on_scope = get_runs_on_scope(runs_on_type);
        runs_on_scope->set_parent_scope(my_scope);
        parlist_scope = runs_on_scope;
      }
    }
    fp_list->set_my_scope(parlist_scope);
    fp_list->chk(asstype);
    sb->chk();
    ags->set_is_altstep();
    ags->set_my_ags(ags);
    ags->set_my_laic_stmt(ags, 0);
    ags->chk();
    if (!semantic_check_only) {
      fp_list->set_genname(get_genname());
      sb->set_code_section(GovernedSimple::CS_INLINE);
      ags->set_code_section(GovernedSimple::CS_INLINE);
    }
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
    }
  }

  void Def_Altstep::generate_code(output_struct *target, bool)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    const char *dispname_str = id->get_dispname().c_str();

    // function for altstep instance:
    // assemble the function body first (this also determines which parameters
    // are never used)
    char* body = create_location_object(memptystr(), "ALTSTEP", dispname_str);
    if (debugger_active) {
      body = generate_code_debugger_function_init(body, this);
    }
    body = sb->generate_code(body, target->header.global_vars,
      target->source.global_vars);
    body = ags->generate_code_altstep(body, target->header.global_vars,
      target->source.global_vars);
    // generate a smart formal parameter list (omits unused parameter names)
    char *formal_par_list = fp_list->generate_code(memptystr());
    fp_list->generate_code_defval(target);

    // function for altstep instance: prototype
    target->header.function_prototypes =
      mputprintf(target->header.function_prototypes,
        "extern alt_status %s_instance(%s);\n", genname_str, formal_par_list);

    // generate shadow objects for parameters if needed
    // (this needs be done after the body is generated, so it to knows which 
    // parameters are never used)
    char* shadow_objects = fp_list->generate_shadow_objects(memptystr());
    
    // function for altstep instance: body
    target->source.function_bodies = mputprintf(target->source.function_bodies,
      "alt_status %s_instance(%s)\n"
      "{\n"
      "%s%s"
      "}\n\n", genname_str, formal_par_list, shadow_objects, body);
    Free(formal_par_list);
    Free(shadow_objects);
    Free(body);

    char *actual_par_list =
      fp_list->generate_code_actual_parlist(memptystr(), "");
    
    // use a full formal parameter list for the rest of the functions
    char *full_formal_par_list = fp_list->generate_code(memptystr(),
      fp_list->get_nof_fps());

    // wrapper function for stand-alone instantiation: prototype
    target->header.function_prototypes =
      mputprintf(target->header.function_prototypes,
        "extern void %s(%s);\n", genname_str, full_formal_par_list);

    // wrapper function for stand-alone instantiation: body
    target->source.function_bodies =
      mputprintf(target->source.function_bodies, "void %s(%s)\n"
        "{\n"
        "altstep_begin:\n"
        "boolean block_flag = FALSE;\n"
        "alt_status altstep_flag = ALT_UNCHECKED, "
        "default_flag = ALT_UNCHECKED;\n"
        "for ( ; ; ) {\n"
        "TTCN_Snapshot::take_new(block_flag);\n"
        "if (altstep_flag != ALT_NO) {\n"
        "altstep_flag = %s_instance(%s);\n"
        "if (altstep_flag == ALT_YES || altstep_flag == ALT_BREAK) return;\n"
        "else if (altstep_flag == ALT_REPEAT) goto altstep_begin;\n"
        "}\n"
        "if (default_flag != ALT_NO) {\n"
        "default_flag = TTCN_Default::try_altsteps();\n"
        "if (default_flag == ALT_YES || default_flag == ALT_BREAK) return;\n"
        "else if (default_flag == ALT_REPEAT) goto altstep_begin;\n"
        "}\n"
        "if (altstep_flag == ALT_NO && default_flag == ALT_NO) "
        "TTCN_error(\"None of the branches can be chosen in altstep %s.\");\n"
        "else block_flag = TRUE;\n"
        "}\n"
        "}\n\n", genname_str, full_formal_par_list, genname_str, actual_par_list,
        dispname_str);

    // class for keeping the altstep in the default context
    // the class is for internal use, we do not need to publish it in the
    // header file
    char* str = mprintf("class %s_Default : public Default_Base {\n", genname_str);
    str = fp_list->generate_code_object(str, "par_");
    str = mputprintf(str, "public:\n"
      "%s_Default(%s);\n"
      "alt_status call_altstep();\n"
      "};\n\n", genname_str, full_formal_par_list);
    target->source.class_defs = mputstr(target->source.class_defs, str);
    Free(str);
    // member functions of the class
    str = mprintf("%s_Default::%s_Default(%s)\n"
        " : Default_Base(\"%s\")", genname_str, genname_str, full_formal_par_list,
        dispname_str);
    for (size_t i = 0; i < fp_list->get_nof_fps(); i++) {
      const char *fp_name_str =
        fp_list->get_fp_byIndex(i)->get_id().get_name().c_str();
      str = mputprintf(str, ", par_%s(%s)", fp_name_str, fp_name_str);
    }
    str = mputstr(str, "\n{\n}\n\n");
    char *actual_par_list_prefixed =
      fp_list->generate_code_actual_parlist(memptystr(), "par_");
    str = mputprintf(str, "alt_status %s_Default::call_altstep()\n"
      "{\n"
      "return %s_instance(%s);\n"
      "}\n\n", genname_str, genname_str, actual_par_list_prefixed);
    Free(actual_par_list_prefixed);
    target->source.methods = mputstr(target->source.methods, str);
    Free(str);

    // function for default activation: prototype
    target->header.function_prototypes =
      mputprintf(target->header.function_prototypes,
        "extern Default_Base *activate_%s(%s);\n", genname_str,
        full_formal_par_list);

    // function for default activation: body
    str = mprintf("Default_Base *activate_%s(%s)\n"
      "{\n", genname_str, full_formal_par_list);
    str = mputprintf(str, "return new %s_Default(%s);\n"
      "}\n\n", genname_str, actual_par_list);
    target->source.function_bodies = mputstr(target->source.function_bodies,
      str);
    Free(str);

    Free(full_formal_par_list);
    Free(actual_par_list);

    target->functions.pre_init = mputprintf(target->functions.pre_init,
      "%s.add_altstep(\"%s\", (genericfunc_t)&%s_instance, (genericfunc_t )&activate_%s, "
        "(genericfunc_t )&%s);\n", get_module_object_name(), dispname_str, genname_str,
      genname_str, genname_str);
  }

  void Def_Altstep::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  void Def_Altstep::dump_internal(unsigned level) const
  {
    DEBUG(level, "Altstep: %s", id->get_dispname().c_str());
    DEBUG(level + 1, "Parameters:");
    fp_list->dump(level + 1);
    if (runs_on_ref) {
      DEBUG(level + 1, "Runs on clause:");
      runs_on_ref->dump(level + 2);
    }
    /*
    DEBUG(level + 1, "Local definitions:");
    sb->dump(level + 2);
    */
    DEBUG(level + 1, "Guards:");
    ags->dump(level + 2);
  }

  void Def_Altstep::set_parent_path(WithAttribPath* p_path) {
    Definition::set_parent_path(p_path);
    sb->set_parent_path(w_attrib_path);
  }

  // =================================
  // ===== Def_Testcase
  // =================================

  Def_Testcase::Def_Testcase(Identifier *p_id, FormalParList *p_fpl,
                             Reference *p_runs_on_ref, Reference *p_system_ref,
                             StatementBlock *p_block)
    : Definition(A_TESTCASE, p_id), fp_list(p_fpl), runs_on_ref(p_runs_on_ref),
      runs_on_type(0), system_ref(p_system_ref), system_type(0), block(p_block)
  {
    if (!p_fpl || !p_runs_on_ref || !p_block)
      FATAL_ERROR("Def_Testcase::Def_Testcase()");
    fp_list->set_my_def(this);
    block->set_my_def(this);
  }

  Def_Testcase::~Def_Testcase()
  {
    delete fp_list;
    delete runs_on_ref;
    delete system_ref;
    delete block;
  }

  Def_Testcase *Def_Testcase::clone() const
  {
    FATAL_ERROR("Def_Testcase::clone");
  }

  void Def_Testcase::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    fp_list->set_fullname(p_fullname + ".<formal_par_list>");
    runs_on_ref->set_fullname(p_fullname + ".<runs_on_type>");
    if (system_ref) system_ref->set_fullname(p_fullname + ".<system_type>");
    block->set_fullname(p_fullname + ".<statement_block>");
  }

  void Def_Testcase::set_my_scope(Scope *p_scope)
  {
    bridgeScope.set_parent_scope(p_scope);
    bridgeScope.set_scopeMacro_name(id->get_dispname());

    Definition::set_my_scope(&bridgeScope);
    // the scope of the parameter list is set during checking
    runs_on_ref->set_my_scope(&bridgeScope);
    if (system_ref) system_ref->set_my_scope(&bridgeScope);
    block->set_my_scope(fp_list);
  }

  Type *Def_Testcase::get_RunsOnType()
  {
    if (!checked) chk();
    return runs_on_type;
  }

  Type *Def_Testcase::get_SystemType()
  {
    if (!checked) chk();
    return system_type;
  }

  FormalParList *Def_Testcase::get_FormalParList()
  {
    if (!checked) chk();
    return fp_list;
  }

  RunsOnScope *Def_Testcase::get_runs_on_scope(Type *comptype)
  {
    Module *my_module = dynamic_cast<Module*>(my_scope->get_scope_mod());
    if (!my_module) FATAL_ERROR("Def_Testcase::get_runs_on_scope()");
    return my_module->get_runs_on_scope(comptype);
  }

  void Def_Testcase::chk()
  {
    if (checked) return;
    checked = true;
    Error_Context cntxt(this, "In testcase definition `%s'",
      id->get_dispname().c_str());
    Scope *parlist_scope = my_scope;
    {
      Error_Context cntxt2(runs_on_ref, "In `runs on' clause");
      runs_on_type = runs_on_ref->chk_comptype_ref();
      if (runs_on_type) {
        Scope *runs_on_scope = get_runs_on_scope(runs_on_type);
        runs_on_scope->set_parent_scope(my_scope);
        parlist_scope = runs_on_scope;
      }
    }
    if (system_ref) {
      Error_Context cntxt2(system_ref, "In `system' clause");
      system_type = system_ref->chk_comptype_ref();;
    }
    fp_list->set_my_scope(parlist_scope);
    fp_list->chk(asstype);
    block->chk();
    if (!semantic_check_only) {
      fp_list->set_genname(get_genname());
      block->set_code_section(GovernedSimple::CS_INLINE);
    }
    if (w_attrib_path) {
      w_attrib_path->chk_global_attrib();
      w_attrib_path->chk_no_qualif();
    }
  }

  void Def_Testcase::generate_code(output_struct *target, bool)
  {
    const string& t_genname = get_genname();
    const char *genname_str = t_genname.c_str();
    const char *dispname_str = id->get_dispname().c_str();
    
    // assemble the function body first (this also determines which parameters
    // are never used)
    
    // Checking whether the testcase was invoked from another one.
    // At this point the location information should refer to the execute()
    // statement rather than this testcase.
    char* body = mputstr(memptystr(), "TTCN_Runtime::check_begin_testcase(has_timer, "
        "timer_value);\n");
    body = create_location_object(body, "TESTCASE", dispname_str);
    body = fp_list->generate_shadow_objects(body);
    if (debugger_active) {
      body = generate_code_debugger_function_init(body, this);
    }
    body = mputprintf(body, "try {\n"
      "TTCN_Runtime::begin_testcase(\"%s\", \"%s\", ",
      my_scope->get_scope_mod()->get_modid().get_dispname().c_str(),
      dispname_str);
    ComponentTypeBody *runs_on_body = runs_on_type->get_CompBody();
    body = runs_on_body->generate_code_comptype_name(body);
    body = mputstr(body, ", ");
    if (system_type)
      body = system_type->get_CompBody()->generate_code_comptype_name(body);
    else body = runs_on_body->generate_code_comptype_name(body);
    body = mputstr(body, ", has_timer, timer_value);\n");
    body = block->generate_code(body, target->header.global_vars,
      target->source.global_vars);
    body = mputprintf(body,
      "} catch (const TC_Error& tc_error) {\n"
      "} catch (const TC_End& tc_end) {\n"
      "TTCN_Logger::log_str(TTCN_FUNCTION, \"Test case %s was stopped.\");\n"
      "}\n", dispname_str);
    body = mputstr(body, "return TTCN_Runtime::end_testcase();\n");
    
    // smart formal parameter list (names of unused parameters are omitted)
    char *formal_par_list = fp_list->generate_code(memptystr());
    fp_list->generate_code_defval(target);
    if (fp_list->get_nof_fps() > 0)
      formal_par_list = mputstr(formal_par_list, ", ");
    formal_par_list = mputstr(formal_par_list,
      "boolean has_timer, double timer_value");

    // function prototype
    target->header.function_prototypes =
      mputprintf(target->header.function_prototypes,
        "extern verdicttype testcase_%s(%s);\n", genname_str, formal_par_list);

    // function body
    target->source.function_bodies = mputprintf(target->source.function_bodies,
      "verdicttype testcase_%s(%s)\n"
      "{\n"
      "%s"
      "}\n\n", genname_str, formal_par_list, body);
    Free(formal_par_list);
    Free(body);

    if (fp_list->get_nof_fps() == 0) {
      // adding to the list of startable testcases
      target->functions.pre_init = mputprintf(target->functions.pre_init,
        "%s.add_testcase_nonpard(\"%s\", testcase_%s);\n",
        get_module_object_name(), dispname_str, genname_str);
    } else {
      target->functions.pre_init = mputprintf(target->functions.pre_init,
        "%s.add_testcase_pard(\"%s\", (genericfunc_t)&testcase_%s);\n",
        get_module_object_name(), dispname_str, genname_str);

      // If every formal parameter has a default value, the testcase
      // might be callable after all.
      bool callable = true;
      for (size_t i = 0; i < fp_list->get_nof_fps(); ++i) {
        FormalPar *fp = fp_list->get_fp_byIndex(i);
        if (!fp->has_defval()) {
          callable = false;
          break;
        }
      }

      if (callable) {
        // Write a wrapper, which acts as a no-param testcase
        // by calling the parameterized testcase with the default values.
        target->header.function_prototypes =
          mputprintf(target->header.function_prototypes,
            "extern verdicttype testcase_%s_defparams(boolean has_timer, double timer_value);\n",
            genname_str);
        target->source.function_bodies = mputprintf(target->source.function_bodies,
          "verdicttype testcase_%s_defparams(boolean has_timer, double timer_value) {\n"
          "  return testcase_%s(",
          genname_str, genname_str);

        for (size_t i = 0; i < fp_list->get_nof_fps(); ++i) {
          FormalPar *fp = fp_list->get_fp_byIndex(i);
          ActualPar *ap = fp->get_defval();
          switch (ap->get_selection()) {
          case ActualPar::AP_VALUE:
            target->source.function_bodies = mputstr(target->source.function_bodies,
              ap->get_Value()->get_genname_own(my_scope).c_str());
            break;
          case ActualPar::AP_TEMPLATE:
            target->source.function_bodies = mputstr(target->source.function_bodies,
              ap->get_TemplateInstance()->get_Template()->get_genname_own(my_scope).c_str());
            break;
          case ActualPar::AP_REF:
            target->source.function_bodies = mputstr(target->source.function_bodies,
              ap->get_Ref()->get_refd_assignment()->get_genname_from_scope(my_scope).c_str());
            break;
          case ActualPar::AP_DEFAULT:
            // Can't happen. This ActualPar was created by
            // Ttcn::FormalPar::chk_actual_par as the default value for
            // a FormalPar, and it only ever creates vale, template or ref.
            // no break
          default:
            FATAL_ERROR("Def_Testcase::generate_code()");
          }

          // always append a comma, because has_timer and timer_value follows
          target->source.function_bodies = mputstrn(target->source.function_bodies,
            ", ", 2);
        }

        target->source.function_bodies = mputstr(target->source.function_bodies,
          "has_timer, timer_value);\n"
          "}\n\n");
        // Add the non-parameterized wrapper *after* the parameterized one,
        // with the same name. Linear search will always find the first
        // (user-visible, parameterized) testcase.
        // TTCN_Module::execute_testcase knows that if after a parameterized
        // testcase another testcase with the same name follows,
        // it's the callable, non-parameterized wrapper.
        //
        // TTCN_Module::list_testcases skips parameterized testcases;
        // it will now list the non-parameterized wrapper.
        target->functions.pre_init = mputprintf(target->functions.pre_init,
          "%s.add_testcase_nonpard(\"%s\", testcase_%s_defparams);\n",
          get_module_object_name(), dispname_str, genname_str);
      }
    } // has formal parameters
  }

  void Def_Testcase::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  void Def_Testcase::dump_internal(unsigned level) const
  {
    DEBUG(level, "Testcase: %s", id->get_dispname().c_str());
    DEBUG(level + 1, "Parameters:");
    fp_list->dump(level + 1);
    DEBUG(level + 1, "Runs on clause:");
    runs_on_ref->dump(level + 2);
    if (system_ref) {
      DEBUG(level + 1, "System clause:");
      system_ref->dump(level + 2);
    }
    DEBUG(level + 1, "Statement block:");
    block->dump(level + 2);
  }

  void Def_Testcase::set_parent_path(WithAttribPath* p_path) {
    Definition::set_parent_path(p_path);
    if (block)
      block->set_parent_path(w_attrib_path);
  }

  // =================================
  // ===== FormalPar
  // =================================

  FormalPar::FormalPar(asstype_t p_asstype, Type *p_type, Identifier* p_name,
    TemplateInstance *p_defval, param_eval_t p_eval)
    : Definition(p_asstype, p_name), type(p_type), my_parlist(0),
    used_as_lvalue(false), template_restriction(TR_NONE),
    eval(p_eval), defval_generated(false), usage_found(false)
  {
    switch (p_asstype) {
    case A_PAR_VAL:
    case A_PAR_VAL_IN:
    case A_PAR_VAL_OUT:
    case A_PAR_VAL_INOUT:
    case A_PAR_TEMPL_IN:
    case A_PAR_TEMPL_OUT:
    case A_PAR_TEMPL_INOUT:
    case A_PAR_PORT:
      break;
    default:
      FATAL_ERROR("Ttcn::FormalPar::FormalPar(): invalid parameter type");
    }
    if (!p_type)
      FATAL_ERROR("Ttcn::FormalPar::FormalPar(): NULL pointer");
    type->set_ownertype(Type::OT_FORMAL_PAR, this);
    defval.ti = p_defval;
  }

  FormalPar::FormalPar(asstype_t p_asstype,
    template_restriction_t p_template_restriction, Type *p_type,
    Identifier* p_name, TemplateInstance *p_defval, param_eval_t p_eval)
    : Definition(p_asstype, p_name), type(p_type), my_parlist(0),
    used_as_lvalue(false), template_restriction(p_template_restriction),
    eval(p_eval), defval_generated(false), usage_found(false)
  {
    switch (p_asstype) {
    case A_PAR_TEMPL_IN:
    case A_PAR_TEMPL_OUT:
    case A_PAR_TEMPL_INOUT:
      break;
    default:
      FATAL_ERROR("Ttcn::FormalPar::FormalPar(): parameter not template");
    }
    if (!p_type)
      FATAL_ERROR("Ttcn::FormalPar::FormalPar(): NULL pointer");
    type->set_ownertype(Type::OT_FORMAL_PAR, this);
    defval.ti = p_defval;
  }

  FormalPar::FormalPar(asstype_t p_asstype, Identifier* p_name,
    TemplateInstance *p_defval)
    : Definition(p_asstype, p_name), type(0), my_parlist(0),
    used_as_lvalue(false), template_restriction(TR_NONE), eval(NORMAL_EVAL),
    defval_generated(false), usage_found(false)
  {
    if (p_asstype != A_PAR_TIMER)
      FATAL_ERROR("Ttcn::FormalPar::FormalPar(): invalid parameter type");
    defval.ti = p_defval;
  }

  FormalPar::~FormalPar()
  {
    delete type;
    if (checked) delete defval.ap;
    else delete defval.ti;
  }

  FormalPar* FormalPar::clone() const
  {
    FATAL_ERROR("FormalPar::clone");
  }

  void FormalPar::set_fullname(const string& p_fullname)
  {
    Definition::set_fullname(p_fullname);
    if (type) type->set_fullname(p_fullname + ".<type>");
    if (checked) {
      if (defval.ap) defval.ap->set_fullname(p_fullname + ".<default_value>");
    } else {
      if (defval.ti) defval.ti->set_fullname(p_fullname + ".<default_value>");
    }
  }

  void FormalPar::set_my_scope(Scope *p_scope)
  {
    Definition::set_my_scope(p_scope);
    if (type) type->set_my_scope(p_scope);
    if (checked) {
      if (defval.ap) defval.ap->set_my_scope(p_scope);
    } else {
      if (defval.ti) defval.ti->set_my_scope(p_scope);
    }
  }

  bool FormalPar::is_local() const
  {
    return true;
  }

  Type *FormalPar::get_Type()
  {
    if (!checked) chk();
    if (!type) FATAL_ERROR("FormalPar::get_Type()");
    return type;
  }

  void FormalPar::chk()
  {
    if (checked) return;
    checked = true;
    TemplateInstance *default_value = defval.ti;
    defval.ti = 0;
    if (type) {
      type->chk();
      Type *t = type->get_type_refd_last();
      // checks for forbidden type <-> parameter combinations
      switch (t->get_typetype()) {
      case Type::T_PORT:
        switch (asstype) {
        case A_PAR_VAL:
        case A_PAR_VAL_INOUT:
          asstype = A_PAR_PORT;
          break;
        default:
          error("Port type `%s' cannot be used as %s",
            t->get_fullname().c_str(), get_assname());
        }
        break;
      case Type::T_SIGNATURE:
        switch (asstype) {
        case A_PAR_TEMPL_IN:
        case A_PAR_TEMPL_OUT:
        case A_PAR_TEMPL_INOUT:
          break;
        default:
          error("Signature `%s' cannot be used as %s",
            t->get_fullname().c_str(), get_assname());
        }
        break;
      default:
        switch (asstype) {
        case A_PAR_PORT:
        case A_PAR_TIMER:
          FATAL_ERROR("FormalPar::chk()");
        case A_PAR_VAL:
          asstype = A_PAR_VAL_IN;
        default:
          break;
        }
      }
    } else if (asstype != A_PAR_TIMER) FATAL_ERROR("FormalPar::chk()");

    if (default_value) {
      Error_Context cntxt(default_value, "In default value");
      defval.ap = chk_actual_par(default_value, Type::EXPECTED_STATIC_VALUE);
      delete default_value;
      if (!semantic_check_only)
        defval.ap->set_code_section(GovernedSimple::CS_POST_INIT);
    }
    
    if (use_runtime_2 && eval == NORMAL_EVAL && my_parlist->get_my_def() != NULL &&
        my_parlist->get_my_def()->get_asstype() == Definition::A_ALTSTEP) {
      // altstep 'in' parameters are always shadowed in RT2, because if a default
      // altstep deactivates itself, then its parameters are deleted;
      // update the genname so that all references in the generated code
      // will point to the shadow object
      switch (asstype) {
      case A_PAR_VAL:
      case A_PAR_VAL_IN:
      case A_PAR_TEMPL_IN:
        set_genname(id->get_name() + "_shadow");
        break;
      default:
        break;
      }
    }
  }

  bool FormalPar::has_defval() const
  {
    if (checked) return defval.ap != 0;
    else return defval.ti != 0;
  }

  bool FormalPar::has_notused_defval() const
  {
    if (checked) FATAL_ERROR("FormalPar::has_notused_defval");
    if (!defval.ti || !defval.ti->get_Template())
      return false;
    return defval.ti->get_Template()->get_templatetype()
      == Template::TEMPLATE_NOTUSED;
  }

  ActualPar *FormalPar::get_defval() const
  {
    if (!checked) FATAL_ERROR("FormalPar::get_defval()");
    return defval.ap;
  }

  // Extract the TemplateInstance from an ActualPar.
  void FormalPar::set_defval(ActualPar *defpar)
  {
    // ActualPar::clone() is not implemented, since we need such a function
    // only here only for AP_{VALUE,TEMPLATE} parameters.  AP_ERROR can also
    // happen for Def_Template nodes, but they will be errors later.
    // FIXME: This function is Def_Template specific.
    if (!defval.ti->get_Template() || defval.ti->get_Template()
        ->get_templatetype() != Template::TEMPLATE_NOTUSED)
      FATAL_ERROR("FormalPar::set_defval()");
    TemplateInstance *reversed_ti = 0;
    switch (defpar->get_selection()) {
    case ActualPar::AP_VALUE:
      reversed_ti = new TemplateInstance(type->clone(), 0, new Template
        (defpar->get_Value()->clone()));  // Trust the clone().
      break;
    case ActualPar::AP_TEMPLATE:
      reversed_ti = defpar->get_TemplateInstance()->clone();
      break;
    case ActualPar::AP_ERROR:
      break;  // Can happen, but let it go.
    case ActualPar::AP_REF:
    case ActualPar::AP_DEFAULT:
    default:
      FATAL_ERROR("FormalPar::set_defval()");
    }
    if (reversed_ti) {
      delete defval.ti;
      reversed_ti->set_my_scope(get_my_scope());
      defval.ti = reversed_ti;
    }
  }

  ActualPar *FormalPar::chk_actual_par(TemplateInstance *actual_par,
    Type::expected_value_t exp_val)
  {
    if (!checked) chk();
    switch (asstype) {
    case A_PAR_VAL:
    case A_PAR_VAL_IN:
      return chk_actual_par_value(actual_par, exp_val);
    case A_PAR_VAL_OUT:
    case A_PAR_VAL_INOUT:
      return chk_actual_par_by_ref(actual_par, false, exp_val);
    case A_PAR_TEMPL_IN:
      return chk_actual_par_template(actual_par, exp_val);
    case A_PAR_TEMPL_OUT:
    case A_PAR_TEMPL_INOUT:
      return chk_actual_par_by_ref(actual_par, true, exp_val);
    case A_PAR_TIMER:
      return chk_actual_par_timer(actual_par, exp_val);
    case A_PAR_PORT:
      return chk_actual_par_port(actual_par, exp_val);
    default:
      FATAL_ERROR("FormalPar::chk_actual_par()");
    }
    return 0; // to avoid warnings
  }

  ActualPar *FormalPar::chk_actual_par_value(TemplateInstance *actual_par,
    Type::expected_value_t exp_val)
  {
    actual_par->chk_Type(type);
    Ref_base *derived_ref = actual_par->get_DerivedRef();
    if (derived_ref) {
      derived_ref->error("An in-line modified template cannot be used as %s",
          get_assname());
      actual_par->chk_DerivedRef(type);
    }
    Template *ap_template = actual_par->get_Template();
    if (ap_template->is_Value()) {
      Value *v = ap_template->get_Value();
      v->set_my_governor(type);
      type->chk_this_value_ref(v);
      type->chk_this_value(v, 0, exp_val, INCOMPLETE_NOT_ALLOWED,
        OMIT_NOT_ALLOWED, SUB_CHK);
      return new ActualPar(v);
    } else {
      actual_par->error("A specific value without matching symbols "
        "was expected for a %s", get_assname());
      return new ActualPar();
    }
  }

  static void chk_defpar_value(const Value* v)
  {
    Common::Reference *vref = v->get_reference();
    Common::Assignment *ass2 = vref->get_refd_assignment();
    ass2->chk();
    Scope *scope = ass2->get_my_scope();
    ComponentTypeBody *ctb = dynamic_cast<ComponentTypeBody *>(scope);
    if (ctb) { // this is a component variable
      v->error("default value cannot refer to"
        " a template field of the component in the `runs on' clause");
    }
  }

  static void chk_defpar_template(const Template *body,
    Type::expected_value_t exp_val)
  {
    switch (body->get_templatetype()) {
    case Template::TEMPLATE_ERROR:
      break; // could be erroneous in the source; skip it
    case Template::TEMPLATE_NOTUSED:
    case Template::OMIT_VALUE:
    case Template::ANY_VALUE:
    case Template::ANY_OR_OMIT:
      break; // acceptable (?)
    case Template::TEMPLATE_INVOKE: // calling a function is not acceptable
      body->error("default value can not be a function invocation");
      break;
    case Template::VALUE_RANGE: {
      ValueRange *range = body->get_value_range();
      Value *low  = range->get_min_v();
      Type::typetype_t tt_low = low->get_expr_returntype(exp_val);
      Value *high = range->get_max_v();
      Type::typetype_t tt_high = high->get_expr_returntype(exp_val);
      if (tt_low == tt_high) break;
      break; }

    case Template::BSTR_PATTERN:
    case Template::HSTR_PATTERN:
    case Template::OSTR_PATTERN:
    case Template::CSTR_PATTERN:
    case Template::USTR_PATTERN:
      break; // should be acceptable in all cases (if only fixed strings possible)

    case Template::SPECIFIC_VALUE: {
      Common::Value *v = body->get_specific_value();
      if (v->get_valuetype() == Value::V_REFD) chk_defpar_value(v);
      break; }

    case Template::ALL_FROM:
      FATAL_ERROR("should have been flattened");
      break;
    case Template::SUPERSET_MATCH:
    case Template::SUBSET_MATCH:
    case Template::PERMUTATION_MATCH:
    case Template::TEMPLATE_LIST:
    case Template::COMPLEMENTED_LIST:
    case Template::VALUE_LIST: {
      // in template charstring par := charstring : ("foo", "bar", "baz")
      size_t num = body->get_nof_comps();
      for (size_t i = 0; i < num; ++i) {
        const Template *tpl = body->get_temp_byIndex(i);
        chk_defpar_template(tpl, exp_val);
      }
      break; }

    case Template::NAMED_TEMPLATE_LIST: {
      size_t num = body->get_nof_comps();
      for (size_t i = 0; i < num; ++i) {
        const NamedTemplate *nt = body->get_namedtemp_byIndex(i);
        const Template *tpl = nt->get_template();
        chk_defpar_template(tpl, exp_val);
      }
      break; }

    case Template::INDEXED_TEMPLATE_LIST: {
      size_t num = body->get_nof_comps();
      for (size_t i = 0; i < num; ++i) {
        const IndexedTemplate *it = body->get_indexedtemp_byIndex(i);
        const Template *tpl = it->get_template();
        chk_defpar_template(tpl, exp_val);
      }
      break; }

    case Template::TEMPLATE_REFD: {
      Ref_base *ref = body->get_reference();

      Ttcn::ActualParList *aplist = ref->get_parlist();
      if (!aplist) break;
      size_t num = aplist->get_nof_pars();
      for (size_t i = 0; i < num; ++i) {
        const Ttcn::ActualPar *ap = aplist->get_par(i);
        deeper:
        switch (ap->get_selection()) {
        case ActualPar::AP_ERROR: {
          break; }
        case ActualPar::AP_VALUE: {
          Value *v = ap->get_Value(); // "v_variable" as the parameter of the template
          v->chk();
          switch (v->get_valuetype()) {
          case Value::V_REFD: {
            chk_defpar_value(v);
            break; }
          default:
            break;
          }
          break; }
        case ActualPar::AP_TEMPLATE: {
          // A component cannot contain a template definition, parameterized or not.
          // Therefore the template this actual par references, cannot be
          // a field of a component => no error possible, nothing to do.
          break; }
        case ActualPar::AP_REF: {
          // A template cannot have an out/inout parameter
          FATAL_ERROR("Template with out parameter?");
          break; }
        case ActualPar::AP_DEFAULT: {
          ap = ap->get_ActualPar();
          goto deeper;
          break; }
        // no default
        } // switch actual par selection
      } // next

      break; }
    case Template::DECODE_MATCH:
      chk_defpar_template(body->get_decode_target()->get_Template(), exp_val);
      break;
    case Template::TEMPLATE_CONCAT:
      if (!use_runtime_2) {
        FATAL_ERROR("template concatenation in default parameter");
      }
      chk_defpar_template(body->get_concat_operand(true), exp_val);
      chk_defpar_template(body->get_concat_operand(false), exp_val);
      break;
    } // switch templatetype

  }

  // This function is called in two situations:
  // 1. FormalParList::chk calls FormalPar::chk to compute the default value
  //    (make an ActualPar from a TemplateInstance).
  //    In this case, defval.ti==0, and actual_par contains its old value.
  //    This case is called only if the formal par has a default value.
  // 2. FormalParList::chk_actual_parlist calls FormalPar::chk_actual_par
  //    to check the parameters supplied by the execute statement to the tc.
  //    In this case, defval.ap has the value computed in case 1.
  ActualPar *FormalPar::chk_actual_par_template(TemplateInstance *actual_par,
    Type::expected_value_t exp_val)
  {
    actual_par->chk(type);
    // actual_par->template_body may change: SPECIFIC_VALUE to TEMPLATE_REFD
    Definition *fplist_def = my_parlist->get_my_def();
    // The parameter list belongs to this definition. If it's a function
    // or testcase, it may have a "runs on" clause.
    Def_Function *parent_fn = dynamic_cast<Def_Function *>(fplist_def);
    Type *runs_on_type = 0;
    if (parent_fn) runs_on_type = parent_fn->get_RunsOnType();
    else { // not a function; maybe a testcase
      Def_Testcase *parent_tc = dynamic_cast<Def_Testcase *>(fplist_def);
      if (parent_tc) runs_on_type = parent_tc->get_RunsOnType();
    }
    if (runs_on_type) {
      // If it _has_ a runs on clause, the type must be a component.
      if (runs_on_type->get_typetype() != Type::T_COMPONENT) FATAL_ERROR("not component?");
      // The default value "shall not refer to elements of the component type
      // in the runs on clause"
      ComponentTypeBody *runs_on_component = runs_on_type->get_CompBody();
      size_t compass = runs_on_component->get_nof_asss();
      for (size_t c = 0; c < compass; c++) {
        Assignment *ass = runs_on_component->get_ass_byIndex(c);
        (void)ass;
      }
    }

    if (exp_val == Type::EXPECTED_STATIC_VALUE
      ||exp_val == Type::EXPECTED_CONSTANT) {
      Ttcn::Template * body = actual_par->get_Template();
      chk_defpar_template(body, exp_val);
    }
    // Rip out the type, derived ref and template from actual_par
    // (which may come from a function invocation or the definition
    // of the default value) and give it to the new ActualPar.
    ActualPar *ret_val = new ActualPar(
      new TemplateInstance(actual_par->get_Type(),
        actual_par->get_DerivedRef(), actual_par->get_Template()));
    // Zero out these members because the caller will soon call delete
    // on actual_par, but they now belong to ret_val.
    // FIXME: should this really be in here, or outside in the caller before the delete ?
    actual_par->release();

    if (template_restriction!=TR_NONE) {
      bool needs_runtime_check =
        ret_val->get_TemplateInstance()->chk_restriction(
          "template formal parameter", template_restriction,
          ret_val->get_TemplateInstance());
      if (needs_runtime_check)
        ret_val->set_gen_restriction_check(template_restriction);
    }
    return ret_val;
  }

  ActualPar *FormalPar::chk_actual_par_by_ref(TemplateInstance *actual_par,
    bool is_template, Type::expected_value_t exp_val)
  {
    Type *ap_type = actual_par->get_Type();
    if (ap_type) {
      ap_type->warning("Explicit type specification is useless for an %s",
        get_assname());
      actual_par->chk_Type(type);
    }
    Ref_base *derived_ref = actual_par->get_DerivedRef();
    if (derived_ref) {
      derived_ref->error("An in-line modified template cannot be used as %s",
        get_assname());
      actual_par->chk_DerivedRef(type);
    }
    // needed for the error messages
    const char *expected_string = is_template ?
      "template variable or template parameter" :
      "variable or value parameter";
    Template *ap_template = actual_par->get_Template();
    if (ap_template->is_Ref()) {
      Ref_base *ref = ap_template->get_Ref();
      Common::Assignment *ass = ref->get_refd_assignment();
      if (!ass) {
        delete ref;
        return new ActualPar();
      }
      bool asstype_correct = false;
      switch (ass->get_asstype()) {
      case A_PAR_VAL_IN:
        ass->use_as_lvalue(*ref);
        if (get_asstype() == A_PAR_VAL_OUT || get_asstype() == A_PAR_TEMPL_OUT) {
          ass->warning("Passing an `in' parameter as another function's `out' parameter");
        }
        // no break
      case A_VAR:
      case A_PAR_VAL_OUT:
      case A_PAR_VAL_INOUT:
        if (!is_template) asstype_correct = true;
        break;
      case A_PAR_TEMPL_IN:
        ass->use_as_lvalue(*ref);
        if (get_asstype() == A_PAR_VAL_OUT || get_asstype() == A_PAR_TEMPL_OUT) {
          ass->warning("Passing an `in' parameter as another function's `out' parameter");
        }
        // no break
      case A_VAR_TEMPLATE:
      case A_PAR_TEMPL_OUT:
      case A_PAR_TEMPL_INOUT:
        if (is_template) asstype_correct = true;
        break;
      default:
        break;
      }
      if (asstype_correct) {
        FieldOrArrayRefs *t_subrefs = ref->get_subrefs();
        Type *ref_type = ass->get_Type()->get_field_type(t_subrefs, exp_val);
        if (ref_type) {
          TypeCompatInfo info(my_scope->get_scope_mod(), type, ref_type, true,
            false, is_template);
          TypeChain l_chain_base;
          TypeChain r_chain_base;
          if (!type->is_compatible(ref_type, &info, actual_par,
              &l_chain_base, &r_chain_base)) {
            if (info.is_subtype_error()) {
              ref->error("%s", info.get_subtype_error().c_str());
            }
            else if (!info.is_erroneous()) {
              ref->error("Type mismatch: Reference to a %s of type "
                "`%s' was expected instead of `%s'", expected_string,
              type->get_typename().c_str(), ref_type->get_typename().c_str());
            }
            else {
              // Always use the format string.
              ref->error("%s", info.get_error_str_str().c_str());
            }
          }
          else if ((asstype == A_PAR_VAL_OUT || asstype == A_PAR_VAL_INOUT ||
                   asstype == A_PAR_TEMPL_OUT || asstype == A_PAR_TEMPL_INOUT) &&
                   !ref_type->is_compatible(type, &info, NULL,
                   &l_chain_base, &r_chain_base)) {
            // run the type compatibility check in the reverse order, too, for 
            // 'out' and 'inout' parameters (they need to be converted back after
            // the function call)
            // this should never fail if the first type compatibility succeeded
            FATAL_ERROR("FormalPar::chk_actual_par_by_ref");
          }
          if (t_subrefs && t_subrefs->refers_to_string_element()) {
            ref->error("Reference to a string element of type `%s' cannot be "
              "used in this context", ref_type->get_typename().c_str());
          }
        }
      } else {
        ref->error("Reference to a %s was expected for an %s instead of %s",
          expected_string, get_assname(), ass->get_description().c_str());
      }
      ActualPar* ret_val_ap = new ActualPar(ref);
      // restriction checking if this is a reference to a template variable
      // this is an 'out' or 'inout' template parameter
      if (is_template && asstype_correct) {
        template_restriction_t refd_tr;
        switch (ass->get_asstype()) {
        case A_VAR_TEMPLATE: {
          Def_Var_Template* dvt = dynamic_cast<Def_Var_Template*>(ass);
          if (!dvt) FATAL_ERROR("FormalPar::chk_actual_par_by_ref()");
          refd_tr = dvt->get_template_restriction();
        } break;
        case A_PAR_TEMPL_IN:
        case A_PAR_TEMPL_OUT:
        case A_PAR_TEMPL_INOUT: {
          FormalPar* fp = dynamic_cast<FormalPar*>(ass);
          if (!fp) FATAL_ERROR("FormalPar::chk_actual_par_by_ref()");
          refd_tr = fp->get_template_restriction();
        } break;
        default:
          FATAL_ERROR("FormalPar::chk_actual_par_by_ref()");
          break;
        }
        refd_tr = Template::get_sub_restriction(refd_tr, ref);
        if (template_restriction!=refd_tr) {
          bool pre_call_check =
            Template::is_less_restrictive(template_restriction, refd_tr);
          bool post_call_check =
            Template::is_less_restrictive(refd_tr, template_restriction);
          if (pre_call_check || post_call_check) {
          ref->warning("Inadequate restriction on the referenced %s `%s', "
            "this may cause a dynamic test case error at runtime",
            ass->get_assname(), ref->get_dispname().c_str());
          ass->note("Referenced %s is here", ass->get_assname());
        }
          if (pre_call_check)
            ret_val_ap->set_gen_restriction_check(template_restriction);
          if (post_call_check)
            ret_val_ap->set_gen_post_restriction_check(refd_tr);
        }
        // for out and inout template parameters of external functions
        // always check because we do not trust user written C++ code
        if (refd_tr!=TR_NONE) {
        switch (my_parlist->get_my_def()->get_asstype()) {
        case A_EXT_FUNCTION:
        case A_EXT_FUNCTION_RVAL:
        case A_EXT_FUNCTION_RTEMP:
            ret_val_ap->set_gen_post_restriction_check(refd_tr);
          break;
        default:
          break;
        }
      }
      }
      return ret_val_ap;
    } else {
      actual_par->error("Reference to a %s was expected for an %s",
        expected_string, get_assname());
      return new ActualPar();
    }
  }

  ActualPar *FormalPar::chk_actual_par_timer(TemplateInstance *actual_par,
    Type::expected_value_t exp_val)
  {
    Type *ap_type = actual_par->get_Type();
    if (ap_type) {
      ap_type->error("Explicit type specification cannot be used for a "
        "timer parameter");
      actual_par->chk_Type(0);
    }
    Ref_base *derived_ref = actual_par->get_DerivedRef();
    if (derived_ref) {
      derived_ref->error("An in-line modified template cannot be used as "
        "timer parameter");
      actual_par->chk_DerivedRef(0);
    }
    Template *ap_template = actual_par->get_Template();
    if (ap_template->is_Ref()) {
      Ref_base *ref = ap_template->get_Ref();
      Common::Assignment *ass = ref->get_refd_assignment();
      if (!ass) {
        delete ref;
        return new ActualPar();
      }
      switch (ass->get_asstype()) {
      case A_TIMER: {
        ArrayDimensions *dims = ass->get_Dimensions();
        if (dims) dims->chk_indices(ref, "timer", false, exp_val);
        else if (ref->get_subrefs()) ref->error("Reference to single %s "
          "cannot have field or array sub-references",
          ass->get_description().c_str());
        break; }
      case A_PAR_TIMER:
        if (ref->get_subrefs()) ref->error("Reference to %s cannot have "
          "field or array sub-references", ass->get_description().c_str());
        break;
      default:
        ref->error("Reference to a timer or timer parameter was expected for "
          "a timer parameter instead of %s", ass->get_description().c_str());
      }
      return new ActualPar(ref);
    } else {
      actual_par->error("Reference to a timer or timer parameter was "
        "expected for a timer parameter");
      return new ActualPar();
    }
  }

  ActualPar *FormalPar::chk_actual_par_port(TemplateInstance *actual_par,
    Type::expected_value_t exp_val)
  {
    Type *ap_type = actual_par->get_Type();
    if (ap_type) {
      ap_type->warning("Explicit type specification is useless for a port "
        "parameter");
      actual_par->chk_Type(type);
    }
    Ref_base *derived_ref = actual_par->get_DerivedRef();
    if (derived_ref) {
      derived_ref->error("An in-line modified template cannot be used as "
        "port parameter");
      actual_par->chk_DerivedRef(type);
    }
    Template *ap_template = actual_par->get_Template();
    if (ap_template->is_Ref()) {
      Ref_base *ref = ap_template->get_Ref();
      Common::Assignment *ass = ref->get_refd_assignment();
      if (!ass) {
        delete ref;
        return new ActualPar();
      }
      bool asstype_correct = false;
      switch (ass->get_asstype()) {
      case A_PORT: {
        ArrayDimensions *dims = ass->get_Dimensions();
        if (dims) dims->chk_indices(ref, "port", false, exp_val);
        else if (ref->get_subrefs()) ref->error("Reference to single %s "
          "cannot have field or array sub-references",
          ass->get_description().c_str());
        asstype_correct = true;
        break; }
      case A_PAR_PORT:
        if (ref->get_subrefs()) ref->error("Reference to %s cannot have "
          "field or array sub-references", ass->get_description().c_str());
        asstype_correct = true;
        break;
      default:
        ref->error("Reference to a port or port parameter was expected for a "
          "port parameter instead of %s", ass->get_description().c_str());
      }
      if (asstype_correct) {
        Type *ref_type = ass->get_Type();
        if (ref_type && !type->is_identical(ref_type))
          ref->error("Type mismatch: Reference to a port or port parameter "
            "of type `%s' was expected instead of `%s'",
            type->get_typename().c_str(), ref_type->get_typename().c_str());
      }
      return new ActualPar(ref);
    } else {
      actual_par->error("Reference to a port or port parameter was expected "
        "for a port parameter");
      return new ActualPar();
    }
  }

  void FormalPar::use_as_lvalue(const Location& p_loc)
  {
    switch (asstype) {
    case A_PAR_VAL_IN:
    case A_PAR_TEMPL_IN:
      break;
    default:
      FATAL_ERROR("FormalPar::use_as_lvalue()");
    }
    if (!used_as_lvalue) {
      Definition *my_def = my_parlist->get_my_def();
      if (!my_def) FATAL_ERROR("FormalPar::use_as_lvalue()");
      if (my_def->get_asstype() == A_TEMPLATE)
        p_loc.error("Parameter `%s' of the template cannot be passed further "
          "as `out' or `inout' parameter", id->get_dispname().c_str());
      else {
        // update the genname so that all references in the generated code
        // will point to the shadow object
        if (eval == NORMAL_EVAL) {
          set_genname(id->get_name() + "_shadow");
        }
        used_as_lvalue = true;
      }
    }
  }
  
  char* FormalPar::generate_code_defval(char* str)
  {
    if (!defval.ap || defval_generated) return str;
    defval_generated = true;
    switch (defval.ap->get_selection()) {
    case ActualPar::AP_VALUE: {
      Value *val = defval.ap->get_Value();
      if (use_runtime_2 && TypeConv::needs_conv_refd(val)) {
        str = TypeConv::gen_conv_code_refd(str, val->get_lhs_name().c_str(), val);
      } else {
        str = val->generate_code_init(str, val->get_lhs_name().c_str());
      }
      break; }
    case ActualPar::AP_TEMPLATE: {
      TemplateInstance *ti = defval.ap->get_TemplateInstance();
      Template *temp = ti->get_Template();
      Ref_base *dref = ti->get_DerivedRef();
      if (dref) {
        expression_struct expr;
        Code::init_expr(&expr);
        expr.expr = mputprintf(expr.expr, "%s = ",
                               temp->get_lhs_name().c_str());
        dref->generate_code(&expr);
        str = Code::merge_free_expr(str, &expr);
      }
      if (use_runtime_2 && TypeConv::needs_conv_refd(temp)) {
        str = TypeConv::gen_conv_code_refd(str, temp->get_lhs_name().c_str(), temp);
      } else {
        str = temp->generate_code_init(str, temp->get_lhs_name().c_str());
      }
      if (defval.ap->get_gen_restriction_check() != TR_NONE) {
        str = Template::generate_restriction_check_code(str,
          temp->get_lhs_name().c_str(), defval.ap->get_gen_restriction_check());
      }
      break; }
    case ActualPar::AP_REF:
      break;
    default:
      FATAL_ERROR("FormalPar::generate_code()");
    }
    return str;
  }

  void FormalPar::generate_code_defval(output_struct *target, bool)
  {
    if (!defval.ap) return;
    switch (defval.ap->get_selection()) {
    case ActualPar::AP_VALUE: {
      Value *val = defval.ap->get_Value();
      const_def cdef;
      Code::init_cdef(&cdef);
      type->generate_code_object(&cdef, val);
      Code::merge_cdef(target, &cdef);
      Code::free_cdef(&cdef);
      break; }
    case ActualPar::AP_TEMPLATE: {
      TemplateInstance *ti = defval.ap->get_TemplateInstance();
      Template *temp = ti->get_Template();
      const_def cdef;
      Code::init_cdef(&cdef);
      type->generate_code_object(&cdef, temp);
      Code::merge_cdef(target, &cdef);
      Code::free_cdef(&cdef);
      break; }
    case ActualPar::AP_REF:
      break;
    default:
      FATAL_ERROR("FormalPar::generate_code()");
    }
    target->functions.post_init = generate_code_defval(target->functions.post_init);
  }

  char *FormalPar::generate_code_fpar(char *str, bool display_unused /* = false */)
  {
    // the name of the parameter should not be displayed if the parameter is not
    // used (to avoid a compiler warning)
    bool display_name = (usage_found || display_unused || debugger_active ||
      (!enable_set_bound_out_param && (asstype == A_PAR_VAL_OUT || asstype == A_PAR_TEMPL_OUT)));
    const char *name_str = display_name ? id->get_name().c_str() : "";
    switch (asstype) {
    case A_PAR_VAL_IN:
      if (eval != NORMAL_EVAL) {
        str = mputprintf(str, "Lazy_Fuzzy_Expr<%s>& %s", type->get_genname_value(my_scope).c_str(), name_str);
      } else {
        str = mputprintf(str, "const %s& %s", type->get_genname_value(my_scope).c_str(), name_str);
      }
      break;
    case A_PAR_VAL_OUT:
    case A_PAR_VAL_INOUT:
    case A_PAR_PORT:
      str = mputprintf(str, "%s& %s", type->get_genname_value(my_scope).c_str(),
        name_str);
      break;
    case A_PAR_TEMPL_IN:
      if (eval != NORMAL_EVAL) {
        str = mputprintf(str, "Lazy_Fuzzy_Expr<%s>& %s", type->get_genname_template(my_scope).c_str(), name_str);
      } else {
        str = mputprintf(str, "const %s& %s", type->get_genname_template(my_scope).c_str(), name_str);
      }
      break;
    case A_PAR_TEMPL_OUT:
    case A_PAR_TEMPL_INOUT:
      str = mputprintf(str, "%s& %s",
        type->get_genname_template(my_scope).c_str(), name_str);
      break;
    case A_PAR_TIMER:
      str = mputprintf(str, "TIMER& %s", name_str);
      break;
    default:
      FATAL_ERROR("FormalPar::generate_code()");
    }
    return str;
  }

  string FormalPar::get_reference_name(Scope* scope) const
  {
    string ret_val;
    if (eval != NORMAL_EVAL) {
      ret_val += "((";
      switch (asstype) {
      case A_PAR_TEMPL_IN:
        ret_val += type->get_genname_template(scope);
        break;
      default:
        ret_val += type->get_genname_value(scope);
        break;
      }
      ret_val += "&)";
    }
    ret_val += get_id().get_name();
    if (eval != NORMAL_EVAL) {
      ret_val += ")";
    }
    return ret_val;
  }

  char *FormalPar::generate_code_object(char *str, const char *p_prefix, char refch, bool gen_init)
  {
    const char *name_str = id->get_name().c_str();
    switch (asstype) {
    case A_PAR_VAL_IN:
      if (eval != NORMAL_EVAL) {
        str = mputprintf(str, "Lazy_Fuzzy_Expr<%s> %s%s",
          type->get_genname_value(my_scope).c_str(), p_prefix, name_str);
        if (gen_init) {
          str = mputprintf(str, "(%s)", eval == LAZY_EVAL ? "FALSE" : "TRUE");
        }
        str = mputstr(str, ";\n");
      } else {
        str = mputprintf(str, "%s %s%s;\n", type->get_genname_value(my_scope).c_str(), p_prefix, name_str);
      }
      break;
    case A_PAR_VAL_OUT:
    case A_PAR_VAL_INOUT:
    case A_PAR_PORT:
      str = mputprintf(str, "%s%c %s%s;\n",
        type->get_genname_value(my_scope).c_str(), refch, p_prefix, name_str);
      break;
    case A_PAR_TEMPL_IN:
      if (eval != NORMAL_EVAL) {
        str = mputprintf(str, "Lazy_Fuzzy_Expr<%s> %s%s",
          type->get_genname_template(my_scope).c_str(), p_prefix, name_str);
        if (gen_init) {
          str = mputprintf(str, "(%s)", eval == LAZY_EVAL ? "FALSE" : "TRUE");
        }
        str = mputstr(str, ";\n");
      } else {
        str = mputprintf(str, "%s %s%s;\n", type->get_genname_template(my_scope).c_str(), p_prefix, name_str);
      }
      break;
    case A_PAR_TEMPL_OUT:
    case A_PAR_TEMPL_INOUT:
      str = mputprintf(str, "%s%c %s%s;\n",
        type->get_genname_template(my_scope).c_str(), refch, p_prefix, name_str);
      break;
    case A_PAR_TIMER:
      str = mputprintf(str, "TIMER& %s%s;\n", p_prefix, name_str);
      break;
    default:
      FATAL_ERROR("FormalPar::generate_code_object()");
    }
    return str;
  }

  char *FormalPar::generate_shadow_object(char *str) const
  {
    if ((used_as_lvalue || (use_runtime_2 && usage_found &&
        my_parlist->get_my_def()->get_asstype() == Definition::A_ALTSTEP))
        && eval == NORMAL_EVAL) {
      const string& t_genname = get_genname();
      const char *genname_str = t_genname.c_str();
      const char *name_str = id->get_name().c_str();
      switch (asstype) {
      case A_PAR_VAL_IN:
        str = mputprintf(str, "%s %s(%s);\n",
          type->get_genname_value(my_scope).c_str(), genname_str, name_str);
        break;
      case A_PAR_TEMPL_IN:
        str = mputprintf(str, "%s %s(%s);\n",
          type->get_genname_template(my_scope).c_str(), genname_str, name_str);
        break;
      default:
        break;
      }
    }
    return str;
  }

  char *FormalPar::generate_code_set_unbound(char *str) const
  {
    switch (asstype) {
    case A_PAR_TEMPL_OUT:
    case A_PAR_VAL_OUT:
      str = mputprintf(str, "%s.clean_up();\n", id->get_name().c_str());
      break;
    default:
      break;
    }
    return str;
  }

  void FormalPar::dump_internal(unsigned level) const
  {
    DEBUG(level, "%s: %s", get_assname(), id->get_dispname().c_str());
    if (type) type->dump(level + 1);
    if (checked) {
      if (defval.ap) {
        DEBUG(level + 1, "default value:");
        defval.ap->dump(level + 2);
      }
    } else {
      if (defval.ti) {
        DEBUG(level + 1, "default value:");
        defval.ti->dump(level + 2);
      }
    }
  }

  // =================================
  // ===== FormalParList
  // =================================

  FormalParList::~FormalParList()
  {
    size_t nof_pars = pars_v.size();
    for (size_t i = 0; i < nof_pars; i++) delete pars_v[i];
    pars_v.clear();
    pars_m.clear();
  }

  FormalParList *FormalParList::clone() const
  {
    FATAL_ERROR("FormalParList::clone");
  }

  void FormalParList::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for (size_t i = 0; i < pars_v.size(); i++) {
      FormalPar *par = pars_v[i];
      par->set_fullname(p_fullname + "." + par->get_id().get_dispname());
    }
  }

  void FormalParList::set_my_scope(Scope *p_scope)
  {
    set_parent_scope(p_scope);
    Node::set_my_scope(p_scope);
    // the scope of parameters is set to the parent scope instead of this
    // because they cannot refer to each other
    for (size_t i = 0; i < pars_v.size(); i++) pars_v[i]->set_my_scope(p_scope);
  }

  void FormalParList::add_fp(FormalPar *p_fp)
  {
    if (!p_fp) FATAL_ERROR("NULL parameter: Ttcn::FormalParList::add_fp()");
    pars_v.add(p_fp);
    p_fp->set_my_parlist(this);
    checked = false;
  }

  bool FormalParList::has_notused_defval() const
  {
    for (size_t i = 0; i < pars_v.size(); i++) {
      if (pars_v[i]->has_notused_defval())
        return true;
    }
    return false;
  }

  bool FormalParList::has_only_default_values() const
  {
    for (size_t i = 0; i < pars_v.size(); i++) {
      if (!pars_v[i]->has_defval()) {
        return false;
      }
    }

    return true;
  }

  bool FormalParList::has_fp_withName(const Identifier& p_name)
  {
    if (!checked) chk(Definition::A_UNDEF);
    return pars_m.has_key(p_name.get_name());
  }

  FormalPar *FormalParList::get_fp_byName(const Identifier& p_name)
  {
    if (!checked) chk(Definition::A_UNDEF);
    return pars_m[p_name.get_name()];
  }

  bool FormalParList::get_startability()
  {
    if(!checked) FATAL_ERROR("FormalParList::get_startability()");
    return is_startable;
  }

  Common::Assignment *FormalParList::get_ass_bySRef(Common::Ref_simple *p_ref)
  {
    if (!p_ref || !checked) FATAL_ERROR("FormalParList::get_ass_bySRef()");
    if (p_ref->get_modid()) return parent_scope->get_ass_bySRef(p_ref);
    else {
      const string& name = p_ref->get_id()->get_name();
      if (pars_m.has_key(name)) return pars_m[name];
      else return parent_scope->get_ass_bySRef(p_ref);
    }
  }

  bool FormalParList::has_ass_withId(const Identifier& p_id)
  {
    if (!checked) FATAL_ERROR("Ttcn::FormalParList::has_ass_withId()");
    return pars_m.has_key(p_id.get_name())
      || parent_scope->has_ass_withId(p_id);
  }

  void FormalParList::set_genname(const string& p_prefix)
  {
    for (size_t i = 0; i < pars_v.size(); i++) {
      FormalPar *par = pars_v[i];
      const string& par_name = par->get_id().get_name();
      if (par->get_asstype() != Definition::A_PAR_TIMER)
        par->get_Type()->set_genname(p_prefix, par_name);
      if (par->has_defval()) {
        string embedded_genname(p_prefix);
        embedded_genname += '_';
        embedded_genname += par_name;
        embedded_genname += "_defval";
        ActualPar *defval = par->get_defval();
        switch (defval->get_selection()) {
        case ActualPar::AP_ERROR:
        case ActualPar::AP_REF:
          break;
        case ActualPar::AP_VALUE: {
          Value *v = defval->get_Value();
          v->set_genname_prefix("const_");
          v->set_genname_recursive(embedded_genname);
          break; }
        case ActualPar::AP_TEMPLATE: {
          Template *t = defval->get_TemplateInstance()->get_Template();
          t->set_genname_prefix("template_");
          t->set_genname_recursive(embedded_genname);
          break; }
        default:
          FATAL_ERROR("FormalParList::set_genname()");
        }
      }
    }
  }

  void FormalParList::chk(Definition::asstype_t deftype)
  {
    if (checked) return;
    checked = true;
    min_nof_pars = 0;
    is_startable = true;
    Error_Context cntxt(this, "In formal parameter list");
    for (size_t i = 0; i < pars_v.size(); i++) {
      FormalPar *par = pars_v[i];
      const Identifier& id = par->get_id();
      const string& name = id.get_name();
      const char *dispname = id.get_dispname().c_str();
      if (pars_m.has_key(name)) {
        par->error("Duplicate parameter with name `%s'", dispname);
        pars_m[name]->note("Previous definition of `%s' is here", dispname);
      } else {
        pars_m.add(name, par);
        if (parent_scope && parent_scope->has_ass_withId(id)) {
          par->error("Parameter name `%s' is not unique in the scope "
            "hierarchy", dispname);
          Reference ref(0, id.clone());
          Common::Assignment *ass = parent_scope->get_ass_bySRef(&ref);
          if (!ass) FATAL_ERROR("FormalParList::chk()");
          ass->note("Symbol `%s' is already defined here in a higher scope "
            "unit", dispname);
        }
      }
      Error_Context cntxt2(par, "In parameter `%s'", dispname);
      par->chk();
      // check whether the parameter type is allowed
      switch (deftype) {
      case Definition::A_TEMPLATE:
        switch (par->get_asstype()) {
        case Definition::A_PAR_VAL_IN:
        case Definition::A_PAR_TEMPL_IN:
          // these are allowed
          break;
        default:
          par->error("A template cannot have %s", par->get_assname());
        }
        break;
      case Definition::A_TESTCASE:
        switch (par->get_asstype()) {
        case Definition::A_PAR_TIMER:
        case Definition::A_PAR_PORT:
          // these are forbidden
          par->error("A testcase cannot have %s", par->get_assname());
        default:
          break;
        }
      default:
        // everything is allowed for functions and altsteps
        break;
      }
      //startability chk
      switch(par->get_asstype()) {
      case Common::Assignment::A_PAR_VAL_IN:
      case Common::Assignment::A_PAR_TEMPL_IN:
      case Common::Assignment::A_PAR_VAL_INOUT:
      case Common::Assignment::A_PAR_TEMPL_INOUT:
        if (is_startable && par->get_Type()->is_component_internal())
          is_startable = false;
        break;
      default:
        is_startable = false;
        break;
      }
      if (!par->has_defval()) min_nof_pars = i + 1;
      // the last parameter without a default value determines the minimum
    }
  }

  // check that @lazy and @fuzzy paramterization are not used in cases currently unsupported
  void FormalParList::chk_noLazyFuzzyParams() {
    Error_Context cntxt(this, "In formal parameter list");
    for (size_t i = 0; i < pars_v.size(); i++) {
      FormalPar *par = pars_v[i];
      if (par->get_eval_type() != NORMAL_EVAL) {
        par->error("Formal parameter `%s' cannot be @%s, not supported in this case.",
          par->get_id().get_dispname().c_str(),
          par->get_eval_type() == LAZY_EVAL ? "lazy" : "fuzzy");
      }
    }
  }

  void FormalParList::chk_startability(const char *p_what, const char *p_name)
  {
    if(!checked) FATAL_ERROR("FormalParList::chk_startability()");
    if (is_startable) return;
    for (size_t i = 0; i < pars_v.size(); i++) {
      FormalPar *par = pars_v[i];
      switch (par->get_asstype()) {
      case Common::Assignment::A_PAR_VAL_IN:
      case Common::Assignment::A_PAR_TEMPL_IN:
      case Common::Assignment::A_PAR_VAL_INOUT:
      case Common::Assignment::A_PAR_TEMPL_INOUT:
        if (par->get_Type()->is_component_internal()) {
          map<Type*,void> type_chain;
          char* err_str = mprintf("a parameter or embedded in a parameter of "
            "a function used in a start operation. "
            "%s `%s' cannot be started on a parallel test component "
            "because of `%s'", p_what, p_name, par->get_description().c_str());
          par->get_Type()->chk_component_internal(type_chain, err_str);
          Free(err_str);
        }
        break;
      default:
        par->error("%s `%s' cannot be started on a parallel test component "
          "because it has %s", p_what, p_name, par->get_description().c_str());
      }
    }
  }

  void FormalParList::chk_compatibility(FormalParList* p_fp_list,
                                        const char* where)
  {
    size_t nof_type_pars = pars_v.size();
    size_t nof_function_pars = p_fp_list->pars_v.size();
    // check for the number of parameters
    if (nof_type_pars != nof_function_pars) {
      p_fp_list->error("Too %s parameters: %lu was expected instead of %lu",
        nof_type_pars < nof_function_pars ? "many" : "few",
        static_cast<unsigned long>( nof_type_pars ), static_cast<unsigned long>( nof_function_pars)) ;
    }
    size_t upper_limit =
      nof_type_pars < nof_function_pars ? nof_type_pars : nof_function_pars;
    for (size_t i = 0; i < upper_limit; i++) {
      FormalPar *type_par = pars_v[i];
      FormalPar *function_par = p_fp_list->pars_v[i];
      Error_Context cntxt(function_par, "In parameter #%lu",
      static_cast<unsigned long> (i + 1));
      FormalPar::asstype_t type_par_asstype = type_par->get_asstype();
      FormalPar::asstype_t function_par_asstype = function_par->get_asstype();
      // check for parameter kind equivalence
      // (in, out or inout / value or template)
      if (type_par_asstype != function_par_asstype) {
        function_par->error("The kind of the parameter is not the same as in "
          "type `%s': %s was expected instead of %s", where,
          type_par->get_assname(), function_par->get_assname());
      }
      // check for type equivalence
      if (type_par_asstype != FormalPar::A_PAR_TIMER &&
          function_par_asstype != FormalPar::A_PAR_TIMER) {
        Type *type_par_type = type_par->get_Type();
        Type *function_par_type = function_par->get_Type();
        if (!type_par_type->is_identical(function_par_type)) {
          function_par_type->error("The type of the parameter is not the same "
            "as in type `%s': `%s' was expected instead of `%s'", where,
            type_par_type->get_typename().c_str(),
            function_par_type->get_typename().c_str());
        } else if (type_par_type->get_sub_type() && function_par_type->get_sub_type() &&
        (type_par_type->get_sub_type()->get_subtypetype()==function_par_type->get_sub_type()->get_subtypetype()) &&
        (!type_par_type->get_sub_type()->is_compatible(function_par_type->get_sub_type()))) {
    // TODO: maybe equivalence should be checked, or maybe that is too strict
    function_par_type->error(
      "Subtype mismatch: subtype %s has no common value with subtype %s",
      type_par_type->get_sub_type()->to_string().c_str(),
      function_par_type->get_sub_type()->to_string().c_str());
  }
  }
      // check for template restriction equivalence
      if (type_par->get_template_restriction()!=
          function_par->get_template_restriction()) {
        function_par->error("The template restriction of the parameter is "
          "not the same as in type `%s': %s restriction was expected instead "
          "of %s restriction", where,
          type_par->get_template_restriction()==TR_NONE ? "no" :
          Template::get_restriction_name(type_par->get_template_restriction()),
          function_par->get_template_restriction()==TR_NONE ? "no" :
          Template::get_restriction_name(function_par->
            get_template_restriction()));
      }
      // check for @lazy equivalence
      if (type_par->get_eval_type()!=function_par->get_eval_type()) {
        function_par->error("Parameter evaluation type (normal, @lazy or @fuzzy) mismatch");
      }
      // check for name equivalence
      const Identifier& type_par_id = type_par->get_id();
      const Identifier& function_par_id = function_par->get_id();
      if (type_par_id != function_par_id) {
        function_par->warning("The name of the parameter is not the same "
          "as in type `%s': `%s' was expected instead of `%s'", where,
          type_par_id.get_dispname().c_str(),
          function_par_id.get_dispname().c_str());
      }
    }
  }

  bool FormalParList::fold_named_and_chk(ParsedActualParameters *p_paps,
                                         ActualParList *p_aplist)
  {
    const size_t num_named   = p_paps->get_nof_nps();
    const size_t num_unnamed = p_paps->get_nof_tis();
    size_t num_actual = num_unnamed;

    // Construct a map to tell us what index a FormalPar has
    typedef map<FormalPar*, size_t> formalpar_map_t;
    formalpar_map_t formalpar_map;

    size_t num_fp = get_nof_fps();
    for (size_t fpx = 0; fpx < num_fp; ++fpx) {
      FormalPar *fp = get_fp_byIndex(fpx);
      formalpar_map.add(fp, new size_t(fpx));
    }

    // Go through the named parameters
    for (size_t i = 0; i < num_named; ++i) {
      NamedParam *np = p_paps->extract_np_byIndex(i);
      // We are now responsible for np.

      if (has_fp_withName(*np->get_name())) {
              // there is a formal parameter with that name
        FormalPar *fp = get_fp_byName(*np->get_name());
        const size_t is_at = *formalpar_map[fp]; // the index of the formal par
        if (is_at >= num_actual) {
          // There is no actual par in the unnamed part.
          // Create one from the named param.

          // First, pad the gap with '-'
          for (; num_actual < is_at; ++num_actual) {
            Template *not_used;
            if (pars_v[num_actual]->has_defval()) {
              not_used = new Template(Template::TEMPLATE_NOTUSED);
            }
            else { // cannot use '-' if no default value
              not_used = new Template(Template::TEMPLATE_ERROR);
            }
            TemplateInstance *new_ti = new TemplateInstance(0, 0, not_used);
            // Conjure a location info at the beginning of the unnamed part
            // (that is, the beginning of the actual parameter list)
            new_ti->set_location(p_paps->get_tis()->get_filename(),
              p_paps->get_tis()->get_first_line(),
              p_paps->get_tis()->get_first_column(), 0, 0);
            p_paps->get_tis()->add_ti(new_ti);
          }
          TemplateInstance * namedti = np->extract_ti();
          p_paps->get_tis()->add_ti(namedti);
          ++num_actual;
        } else {
          // There is already an actual par at that position, fetch it
          TemplateInstance * ti = p_paps->get_tis()->get_ti_byIndex(is_at);
          Template::templatetype_t tt = ti->get_Template()->get_templatetype();

          if (is_at >= num_unnamed && !ti->get_Type() && !ti->get_DerivedRef()
            && (tt == Template::TEMPLATE_NOTUSED || tt == Template::TEMPLATE_ERROR)) {
            // NotUsed in the named part => padding
            np->error("Named parameter `%s' out of order",
              np->get_name()->get_dispname().c_str());
          } else {
            // attempt to override an original unnamed param with a named one
            np->error("Formal parameter `%s' assigned more than once",
              np->get_name()->get_dispname().c_str());
          }
        }
      }
      else { // no formal parameter with that name
        char * nam = 0;
        switch (my_def->get_asstype()) {
        case Common::Assignment::A_TYPE: {
          Type *t = my_def->get_Type();

          switch (t ? t->get_typetype() : 0) {
          case Type::T_FUNCTION:
            nam = mcopystr("Function reference");
            break;
          case Type::T_ALTSTEP:
            nam = mcopystr("Altstep reference");
            break;
          case Type::T_TESTCASE:
            nam = mcopystr("Testcase reference");
            break;
          default:
            FATAL_ERROR("FormalParList::chk_actual_parlist() "
                        "Unexpected type %s", t->get_typename().c_str());
          } // switch(typetype)
          break; }
        default:
          nam = mcopystr(my_def->get_assname());
          break;
        } // switch(asstype)

        *nam &= ~('a'-'A'); // Make the first letter uppercase
        p_paps->get_tis()->error("%s `%s' has no formal parameter `%s'",
          nam,
          my_def->get_fullname().c_str(),
          np->get_name()->get_dispname().c_str());
        Free(nam);
      }
      delete np;
    }

    // Cleanup
    for (size_t fpx = 0; fpx < num_fp; ++fpx) {
      delete formalpar_map.get_nth_elem(fpx);
    }
    formalpar_map.clear();

    return chk_actual_parlist(p_paps->get_tis(), p_aplist);
  }

  bool FormalParList::chk_actual_parlist(TemplateInstances *p_tis,
                                         ActualParList *p_aplist)
  {
    size_t formal_pars = pars_v.size();
    size_t actual_pars = p_tis->get_nof_tis();
    // p_aplist->get_nof_pars() is usually 0 on entry
    bool error_flag = false;

    if (min_nof_pars == formal_pars) {
      // none of the parameters have default value
      if (actual_pars != formal_pars) {
        p_tis->error("Too %s parameters: %lu was expected "
          "instead of %lu", actual_pars < formal_pars ? "few" : "many",
          static_cast<unsigned long>( formal_pars), static_cast<unsigned long>( actual_pars));
        error_flag = true;
      }
    } else {
      // some parameters have default value
      if (actual_pars < min_nof_pars) {
        p_tis->error("Too few parameters: at least %lu "
          "was expected instead of %lu",
          static_cast<unsigned long>( min_nof_pars), static_cast<unsigned long>( actual_pars));
        error_flag = true;
      } else if (actual_pars > formal_pars) {
        p_tis->error("Too many parameters: at most %lu "
          "was expected instead of %lu",
          static_cast<unsigned long>( formal_pars), static_cast<unsigned long>( actual_pars));
        error_flag = true;
      }
    }

    // Do not check actual parameters in excess of the formal ones
    size_t upper_limit = actual_pars < formal_pars ? actual_pars : formal_pars;
    for (size_t i = 0; i < upper_limit; i++) {
      TemplateInstance *ti = p_tis->get_ti_byIndex(i);

      // the formal parameter for the current actual parameter
      FormalPar *fp = pars_v[i];
      Error_Context cntxt(ti, "In parameter #%lu for `%s'",
        static_cast<unsigned long> (i + 1), fp->get_id().get_dispname().c_str());
      if (!ti->get_Type() && !ti->get_DerivedRef() && ti->get_Template()
          ->get_templatetype() == Template::TEMPLATE_NOTUSED) {
        if (fp->has_defval()) {
          ActualPar *defval = fp->get_defval();
          p_aplist->add(new ActualPar(defval));
          if (defval->is_erroneous()) error_flag = true;
        } else {
          ti->error("Not used symbol (`-') cannot be used for parameter "
            "that does not have default value");
          p_aplist->add(new ActualPar());
          error_flag = true;
        }
      } else if (!ti->get_Type() && !ti->get_DerivedRef() && ti->get_Template()
          ->get_templatetype() == Template::TEMPLATE_ERROR) {
        ti->error("Parameter not specified");
      } else {
        ActualPar *ap = fp->chk_actual_par(ti, Type::EXPECTED_DYNAMIC_VALUE);
        p_aplist->add(ap);
        if (ap->is_erroneous()) error_flag = true;
      }
    }

    // The rest of formal parameters have no corresponding actual parameters.
    // Create actual parameters for them based on their default values
    // (which must exist).
    for (size_t i = upper_limit; i < formal_pars; i++) {
      FormalPar *fp = pars_v[i];
      if (fp->has_defval()) {
        ActualPar *defval = fp->get_defval();
        p_aplist->add(new ActualPar(defval));
        if (defval->is_erroneous()) error_flag = true;
      } else {
        p_aplist->add(new ActualPar()); // erroneous
        error_flag = true;
      }
    }
    return error_flag;
  }

  bool FormalParList::chk_activate_argument(ActualParList *p_aplist,
                                            const char* p_description)
  {
    bool ret_val = true;
    for(size_t i = 0; i < p_aplist->get_nof_pars(); i++) {
      ActualPar *t_ap = p_aplist->get_par(i);
      if(t_ap->get_selection() != ActualPar::AP_REF) continue;
      FormalPar *t_fp = pars_v[i];
      switch(t_fp->get_asstype()) {
      case Common::Assignment::A_PAR_VAL_OUT:
      case Common::Assignment::A_PAR_VAL_INOUT:
      case Common::Assignment::A_PAR_TEMPL_OUT:
      case Common::Assignment::A_PAR_TEMPL_INOUT:
      case Common::Assignment::A_PAR_TIMER:
        //the checking shall be performed for these parameter types
        break;
      case Common::Assignment::A_PAR_PORT:
        // port parameters are always correct because ports can be defined
        // only in component types
        continue;
      default:
        FATAL_ERROR("FormalParList::chk_activate_argument()");
      }
      Ref_base *t_ref = t_ap->get_Ref();
      Common::Assignment *t_par_ass = t_ref->get_refd_assignment();
      if(!t_par_ass) FATAL_ERROR("FormalParList::chk_activate_argument()");
      switch (t_par_ass->get_asstype()) {
      case Common::Assignment::A_VAR:
      case Common::Assignment::A_VAR_TEMPLATE:
      case Common::Assignment::A_TIMER:
        // it is not allowed to pass references of local variables or timers
        if (t_par_ass->is_local()) {
          t_ref->error("Parameter #%lu of %s refers to %s, which is a local "
            "definition within a statement block and may have shorter "
            "lifespan than the activated default. Only references to "
            "variables and timers defined in the component type can be passed "
            "to activated defaults", static_cast<unsigned long> (i + 1), p_description,
            t_par_ass->get_description().c_str());
          ret_val = false;
        }
        break;
      case Common::Assignment::A_PAR_VAL_IN:
      case Common::Assignment::A_PAR_VAL_OUT:
      case Common::Assignment::A_PAR_VAL_INOUT:
      case Common::Assignment::A_PAR_TEMPL_IN:
      case Common::Assignment::A_PAR_TEMPL_OUT:
      case Common::Assignment::A_PAR_TEMPL_INOUT:
      case Common::Assignment::A_PAR_TIMER: {
        // it is not allowed to pass references pointing to formal parameters
        // except for activate() statements within testcases
        // note: all defaults are deactivated at the end of the testcase
        FormalPar *t_refd_fp = dynamic_cast<FormalPar*>(t_par_ass);
        if (!t_refd_fp) FATAL_ERROR("FormalParList::chk_activate_argument()");
        FormalParList *t_fpl = t_refd_fp->get_my_parlist();
        if (!t_fpl || !t_fpl->my_def)
          FATAL_ERROR("FormalParList::chk_activate_argument()");
        if (t_fpl->my_def->get_asstype() != Common::Assignment::A_TESTCASE) {
          t_ref->error("Parameter #%lu of %s refers to %s, which may have "
            "shorter lifespan than the activated default. Only references to "
            "variables and timers defined in the component type can be passed "
            "to activated defaults", static_cast<unsigned long> (i + 1), p_description,
            t_par_ass->get_description().c_str());
          ret_val = false;
        } }
      default:
        break;
      }
    }
    return ret_val;
  }

  char *FormalParList::generate_code(char *str, size_t display_unused /* = 0 */)
  {
    for (size_t i = 0; i < pars_v.size(); i++) {
      if (i > 0) str = mputstr(str, ", ");
      str = pars_v[i]->generate_code_fpar(str, i < display_unused);
    }
    return str;
  }
  
  char* FormalParList::generate_code_defval(char* str)
  {
    for (size_t i = 0; i < pars_v.size(); i++) {
      str = pars_v[i]->generate_code_defval(str);
    }
    return str;
  }

  void FormalParList::generate_code_defval(output_struct *target)
  {
    for (size_t i = 0; i < pars_v.size(); i++) {
      pars_v[i]->generate_code_defval(target);
    }
  }

  char *FormalParList::generate_code_actual_parlist(char *str,
    const char *p_prefix)
  {
    for (size_t i = 0; i < pars_v.size(); i++) {
      if (i > 0) str = mputstr(str, ", ");
      str = mputstr(str, p_prefix);
      str = mputstr(str, pars_v[i]->get_id().get_name().c_str());
    }
    return str;
  }

  char *FormalParList::generate_code_object(char *str, const char *p_prefix, char refch, bool gen_init)
  {
    for (size_t i = 0; i < pars_v.size(); i++)
      str = pars_v[i]->generate_code_object(str, p_prefix, refch, gen_init);
    return str;
  }

  char *FormalParList::generate_shadow_objects(char *str) const
  {
    for (size_t i = 0; i < pars_v.size(); i++)
      str = pars_v[i]->generate_shadow_object(str);
    return str;
  }

  char *FormalParList::generate_code_set_unbound(char *str) const
  {
    if (enable_set_bound_out_param) return str;
    for (size_t i = 0; i < pars_v.size(); i++)
      str = pars_v[i]->generate_code_set_unbound(str);
    return str;
  }


  void FormalParList::dump(unsigned level) const
  {
    size_t nof_pars = pars_v.size();
    DEBUG(level, "formal parameters: %lu pcs.", static_cast<unsigned long>( nof_pars));
    for(size_t i = 0; i < nof_pars; i++) pars_v[i]->dump(level + 1);
  }

  // =================================
  // ===== ActualPar
  // =================================

  ActualPar::ActualPar(Value *v)
    : Node(), selection(AP_VALUE), my_scope(0), gen_restriction_check(TR_NONE),
      gen_post_restriction_check(TR_NONE)
  {
    if (!v) FATAL_ERROR("ActualPar::ActualPar()");
    val = v;
  }

  ActualPar::ActualPar(TemplateInstance *t)
    : Node(), selection(AP_TEMPLATE), my_scope(0),
      gen_restriction_check(TR_NONE), gen_post_restriction_check(TR_NONE)
  {
    if (!t) FATAL_ERROR("ActualPar::ActualPar()");
    temp = t;
  }

  ActualPar::ActualPar(Ref_base *r)
    : Node(), selection(AP_REF), my_scope(0), gen_restriction_check(TR_NONE),
      gen_post_restriction_check(TR_NONE)
  {
    if (!r) FATAL_ERROR("ActualPar::ActualPar()");
    ref = r;
  }

  ActualPar::ActualPar(ActualPar *a)
    : Node(), selection(AP_DEFAULT), my_scope(0),
      gen_restriction_check(TR_NONE), gen_post_restriction_check(TR_NONE)
  {
    if (!a) FATAL_ERROR("ActualPar::ActualPar()");
    act = a;
  }

  ActualPar::~ActualPar()
  {
    switch(selection) {
    case AP_ERROR:
      break;
    case AP_VALUE:
      delete val;
      break;
    case AP_TEMPLATE:
      delete temp;
      break;
    case AP_REF:
      delete ref;
      break;
    case AP_DEFAULT:
      break; // nothing to do with act
    default:
      FATAL_ERROR("ActualPar::~ActualPar()");
    }
  }

  ActualPar *ActualPar::clone() const
  {
    FATAL_ERROR("ActualPar::clone");
  }

  void ActualPar::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    switch(selection) {
    case AP_ERROR:
      break;
    case AP_VALUE:
      val->set_fullname(p_fullname);
      break;
    case AP_TEMPLATE:
      temp->set_fullname(p_fullname);
      break;
    case AP_REF:
      ref->set_fullname(p_fullname);
      break;
    case AP_DEFAULT:
      break;
    default:
      FATAL_ERROR("ActualPar::set_fullname()");
    }
  }

  void ActualPar::set_my_scope(Scope *p_scope)
  {
    my_scope = p_scope;
    switch(selection) {
    case AP_ERROR:
      break;
    case AP_VALUE:
      val->set_my_scope(p_scope);
      break;
    case AP_TEMPLATE:
      temp->set_my_scope(p_scope);
      break;
    case AP_REF:
      ref->set_my_scope(p_scope);
      break;
    case AP_DEFAULT:
      switch (act->selection) {
      case AP_REF:
        ref->set_my_scope(p_scope);
        break;
      case AP_VALUE:
        break;
      case AP_TEMPLATE:
        break;
      default:
        FATAL_ERROR("ActualPar::set_my_scope()");
      }
      break;
    default:
      FATAL_ERROR("ActualPar::set_my_scope()");
    }
  }

  Value *ActualPar::get_Value() const
  {
    if (selection != AP_VALUE) FATAL_ERROR("ActualPar::get_Value()");
    return val;
  }

  TemplateInstance *ActualPar::get_TemplateInstance() const
  {
    if (selection != AP_TEMPLATE)
      FATAL_ERROR("ActualPar::get_TemplateInstance()");
    return temp;
  }

  Ref_base *ActualPar::get_Ref() const
  {
    if (selection != AP_REF) FATAL_ERROR("ActualPar::get_Ref()");
    return ref;
  }

  ActualPar *ActualPar::get_ActualPar() const
  {
    if (selection != AP_DEFAULT) FATAL_ERROR("ActualPar::get_ActualPar()");
    return act;
  }

  void ActualPar::chk_recursions(ReferenceChain& refch)
  {
    switch (selection) {
    case AP_VALUE:
      refch.mark_state();
      val->chk_recursions(refch);
      refch.prev_state();
      break;
    case AP_TEMPLATE: {
      Ref_base *derived_ref = temp->get_DerivedRef();
      if (derived_ref) {
        ActualParList *parlist = derived_ref->get_parlist();
        if (parlist) {
          refch.mark_state();
          parlist->chk_recursions(refch);
          refch.prev_state();
        }
      }

      Ttcn::Def_Template* defTemp = temp->get_Referenced_Base_Template();
      if (defTemp) {
        refch.mark_state();
        refch.add(defTemp->get_fullname());
        refch.prev_state();
      }
      refch.mark_state();
      temp->get_Template()->chk_recursions(refch);
      refch.prev_state();
    }
    default:
      break;
    }
  }

  bool ActualPar::has_single_expr()
  {
    switch (selection) {
    case AP_VALUE:
      return val->has_single_expr();
    case AP_TEMPLATE:
      if (gen_restriction_check!=TR_NONE ||
          gen_post_restriction_check!=TR_NONE) return false;
      return temp->has_single_expr();
    case AP_REF:
      if (gen_restriction_check!=TR_NONE ||
          gen_post_restriction_check!=TR_NONE) return false;
      if (use_runtime_2 && ref->get_subrefs() != NULL) {
        FieldOrArrayRefs* subrefs = ref->get_subrefs();
        for (size_t i = 0; i < subrefs->get_nof_refs(); ++i) {
          if (FieldOrArrayRef::ARRAY_REF == subrefs->get_ref(i)->get_type()) {
            return false;
          }
        }
      }
      return ref->has_single_expr();
    case AP_DEFAULT:
      return true;
    default:
      FATAL_ERROR("ActualPar::has_single_expr()");
      return false;
    }
  }

  void ActualPar::set_code_section(
    GovernedSimple::code_section_t p_code_section)
  {
    switch (selection) {
    case AP_VALUE:
      val->set_code_section(p_code_section);
      break;
    case AP_TEMPLATE:
      temp->set_code_section(p_code_section);
      break;
    case AP_REF:
      ref->set_code_section(p_code_section);
    default:
      break;
    }
  }

  void ActualPar::generate_code(expression_struct *expr, bool copy_needed,
                                FormalPar* formal_par) const
  {
    param_eval_t param_eval = formal_par != NULL ? formal_par->get_eval_type() : NORMAL_EVAL;
    bool used_as_lvalue = formal_par != NULL ? formal_par->get_used_as_lvalue() : false;
    switch (selection) {
    case AP_VALUE:
      if (param_eval != NORMAL_EVAL) { // copy_needed doesn't matter in this case
        LazyFuzzyParamData::init(used_as_lvalue);
        LazyFuzzyParamData::generate_code(expr, val, my_scope, param_eval == LAZY_EVAL);
        LazyFuzzyParamData::clean();
        if (val->get_valuetype() == Value::V_REFD) {
          // check if the reference is a parameter, mark it as used if it is
          Reference* r = dynamic_cast<Reference*>(val->get_reference());
          if (r != NULL) {
            r->ref_usage_found();
          }
        }
      } else {
        char* expr_expr = NULL;
        if (use_runtime_2 && TypeConv::needs_conv_refd(val)) {
          // Generate everything to preamble to be able to tackle the wrapper
          // constructor call.  TODO: Reduce the number of temporaries created.
          const string& tmp_id = val->get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          expr->preamble = mputprintf(expr->preamble, "%s %s;\n",
            val->get_my_governor()->get_genname_value(my_scope).c_str(),
            tmp_id_str);
          expr->preamble = TypeConv::gen_conv_code_refd(expr->preamble,
            tmp_id_str, val);
          expr_expr = mputstr(expr_expr, tmp_id_str);
        } else {
          expression_struct val_expr;
          Code::init_expr(&val_expr);
          val->generate_code_expr(&val_expr);
          if (val_expr.preamble != NULL) {
            expr->preamble = mputstr(expr->preamble, val_expr.preamble);
          }
          if (val_expr.postamble == NULL) {
            expr_expr = mputstr(expr_expr, val_expr.expr);
          }
          else {
            // make sure the postambles of the parameters are executed before the
            // function call itself (needed if the value contains function calls
            // with lazy or fuzzy parameters)
            const string& tmp_id = val->get_temporary_id();
            expr->preamble = mputprintf(expr->preamble, "%s %s(%s);\n",
              val->get_my_governor()->get_genname_value(my_scope).c_str(),
              tmp_id.c_str(), val_expr.expr);
            expr->preamble = mputstr(expr->preamble, val_expr.postamble);
            expr_expr = mputstr(expr_expr, tmp_id.c_str());
            copy_needed = false; // already copied
          }
          Code::free_expr(&val_expr);
        }
        if (copy_needed) expr->expr = mputprintf(expr->expr, "%s(",
          val->get_my_governor()->get_genname_value(my_scope).c_str());
        expr->expr = mputstr(expr->expr, expr_expr);
        Free(expr_expr);
        if (copy_needed) expr->expr = mputc(expr->expr, ')');
      }
      break;
    case AP_TEMPLATE:
      if (param_eval != NORMAL_EVAL) { // copy_needed doesn't matter in this case
        LazyFuzzyParamData::init(used_as_lvalue);
        LazyFuzzyParamData::generate_code(expr, temp, gen_restriction_check, my_scope,
           param_eval == LAZY_EVAL);
        LazyFuzzyParamData::clean();
        if (temp->get_DerivedRef() != NULL ||
            temp->get_Template()->get_templatetype() == Template::TEMPLATE_REFD) {
          // check if the reference is a parameter, mark it as used if it is
          Reference* r = dynamic_cast<Reference*>(temp->get_DerivedRef() != NULL ?
            temp->get_DerivedRef() : temp->get_Template()->get_reference());
          if (r != NULL) {
            r->ref_usage_found();
          }
        }
      } else {
        char* expr_expr = NULL;
        if (use_runtime_2 && TypeConv::needs_conv_refd(temp->get_Template())) {
          const string& tmp_id = temp->get_Template()->get_temporary_id();
          const char *tmp_id_str = tmp_id.c_str();
          expr->preamble = mputprintf(expr->preamble, "%s %s;\n",
            temp->get_Template()->get_my_governor()
              ->get_genname_template(my_scope).c_str(), tmp_id_str);
          expr->preamble = TypeConv::gen_conv_code_refd(expr->preamble,
            tmp_id_str, temp->get_Template());
          // Not incorporated into gen_conv_code() yet.
          if (gen_restriction_check != TR_NONE)
            expr->preamble = Template::generate_restriction_check_code(
              expr->preamble, tmp_id_str, gen_restriction_check);
          expr_expr = mputstr(expr_expr, tmp_id_str);
        } else {
          expression_struct temp_expr;
          Code::init_expr(&temp_expr);
          temp->generate_code(&temp_expr, gen_restriction_check);
          if (temp_expr.preamble != NULL) {
            expr->preamble = mputstr(expr->preamble, temp_expr.preamble);
          }
          if (temp_expr.postamble == NULL) {
            expr_expr = mputstr(expr_expr, temp_expr.expr);
          }
          else {
            // make sure the postambles of the parameters are executed before the
            // function call itself (needed if the template contains function calls
            // with lazy or fuzzy parameters)
            const string& tmp_id = val->get_temporary_id();
            expr->preamble = mputprintf(expr->preamble, "%s %s(%s);\n",
              temp->get_Template()->get_my_governor()->get_genname_template(my_scope).c_str(),
              tmp_id.c_str(), temp_expr.expr);
            expr->preamble = mputstr(expr->preamble, temp_expr.postamble);
            expr_expr = mputstr(expr_expr, tmp_id.c_str());
            copy_needed = false; // already copied
          }
          Code::free_expr(&temp_expr);
        }
        if (copy_needed)
          expr->expr = mputprintf(expr->expr, "%s(", temp->get_Template()
            ->get_my_governor()->get_genname_template(my_scope).c_str());
        expr->expr = mputstr(expr->expr, expr_expr);
        Free(expr_expr);
        if (copy_needed) expr->expr = mputc(expr->expr, ')');
      }
      break;
    case AP_REF: {
      if (param_eval != NORMAL_EVAL) FATAL_ERROR("ActualPar::generate_code()"); // syntax error should have already happened
      if (copy_needed) FATAL_ERROR("ActualPar::generate_code()");
      bool is_restricted_template = gen_restriction_check != TR_NONE ||
        gen_post_restriction_check != TR_NONE;
      bool is_template_par = false;
      Type* actual_par_type = NULL;
      Type* formal_par_type = NULL;
      bool needs_conversion = false;
      if (formal_par != NULL &&
          formal_par->get_asstype() != Common::Assignment::A_PAR_TIMER &&
          formal_par->get_asstype() != Common::Assignment::A_PAR_PORT) {
        if (formal_par->get_asstype() == Common::Assignment::A_PAR_TEMPL_INOUT ||
            formal_par->get_asstype() == Common::Assignment::A_PAR_TEMPL_OUT) {
          is_template_par = true;
        }
        Common::Assignment *ass = ref->get_refd_assignment();
        actual_par_type = ass->get_Type()->get_field_type(ref->get_subrefs(),
          is_template_par ? Type::EXPECTED_TEMPLATE : Type::EXPECTED_DYNAMIC_VALUE)->
          get_type_refd_last();
        formal_par_type = formal_par->get_Type()->get_type_refd_last();
        needs_conversion = use_runtime_2 && my_scope->get_scope_mod()->
          needs_type_conv(actual_par_type, formal_par_type);
      }
      if (is_restricted_template || needs_conversion) {
        // generate runtime check for restricted templates and/or generate
        // type conversion to the formal parameter's type and back
        const string& tmp_id= my_scope->get_scope_mod_gen()->get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        expression_struct ref_expr;
        Code::init_expr(&ref_expr);
        ref->generate_code_const_ref(&ref_expr);
        ref_expr.preamble = mputprintf(ref_expr.preamble, "%s& %s = %s;\n",
          is_template_par ? actual_par_type->get_genname_template(my_scope).c_str() :
          actual_par_type->get_genname_value(my_scope).c_str(), tmp_id_str, ref_expr.expr);
        if (gen_restriction_check != TR_NONE) {
          ref_expr.preamble = Template::generate_restriction_check_code(
            ref_expr.preamble, tmp_id_str, gen_restriction_check);
        }
        if (needs_conversion) {
          // create another temporary, this time of the formal parameter's type,
          // containing the converted parameter
          const string& tmp_id2 = my_scope->get_scope_mod_gen()->get_temporary_id();
          const char *tmp_id2_str = tmp_id2.c_str();
          ref_expr.preamble = mputprintf(ref_expr.preamble,
            "%s %s;\n"
            "if (%s.is_bound() && !%s(%s, %s)) TTCN_error(\"Values or templates "
            "of types `%s' and `%s' are not compatible at run-time\");\n",
            is_template_par ? formal_par_type->get_genname_template(my_scope).c_str() :
            formal_par_type->get_genname_value(my_scope).c_str(), tmp_id2_str,
            tmp_id_str, TypeConv::get_conv_func(actual_par_type, formal_par_type,
            my_scope->get_scope_mod()).c_str(), tmp_id2_str, tmp_id_str,
            actual_par_type->get_typename().c_str(), formal_par_type->get_typename().c_str());
          // pass the new temporary to the function instead of the original reference
          expr->expr = mputprintf(expr->expr, "%s", tmp_id2_str);
          // convert the temporary's new value back to the actual parameter's type
          ref_expr.postamble = mputprintf(ref_expr.postamble,
            "if (%s.is_bound() && !%s(%s, %s)) TTCN_error(\"Values or templates "
            "of types `%s' and `%s' are not compatible at run-time\");\n",
            tmp_id2_str, TypeConv::get_conv_func(formal_par_type, actual_par_type, 
            my_scope->get_scope_mod()).c_str(), tmp_id_str, tmp_id2_str,
            formal_par_type->get_typename().c_str(), actual_par_type->get_typename().c_str());
        }
        else { // is_restricted_template
          expr->expr = mputprintf(expr->expr, "%s", tmp_id_str);
        }
        if (gen_post_restriction_check != TR_NONE) {
          ref_expr.postamble = Template::generate_restriction_check_code(
            ref_expr.postamble, tmp_id_str, gen_post_restriction_check);
        }
        // copy content of ref_expr to expr
        expr->preamble = mputstr(expr->preamble, ref_expr.preamble);
        expr->postamble = mputstr(expr->postamble, ref_expr.postamble);
        Code::free_expr(&ref_expr);
      } else {
        ref->generate_code(expr);
      }
      break; }
    case AP_DEFAULT:
      if (copy_needed) FATAL_ERROR("ActualPar::generate_code()");
      switch (act->selection) {
      case AP_REF:
        if (param_eval != NORMAL_EVAL) {
          LazyFuzzyParamData::generate_code_ap_default_ref(expr, act->ref, my_scope,
            param_eval == LAZY_EVAL);
        } else {
          act->ref->generate_code(expr);
        }
        break;
      case AP_VALUE:
        if (param_eval != NORMAL_EVAL) {
          LazyFuzzyParamData::generate_code_ap_default_value(expr, act->val, my_scope,
            param_eval == LAZY_EVAL);
        } else {
          expr->expr = mputstr(expr->expr, act->val->get_genname_own(my_scope).c_str());
        }
        break;
      case AP_TEMPLATE:
        if (param_eval != NORMAL_EVAL) {
          LazyFuzzyParamData::generate_code_ap_default_ti(expr, act->temp, my_scope,
            param_eval == LAZY_EVAL);
        } else {
          expr->expr = mputstr(expr->expr, act->temp->get_Template()->get_genname_own(my_scope).c_str());
        }
        break;
      default:
        FATAL_ERROR("ActualPar::generate_code()");
      }
      break;
    default:
      FATAL_ERROR("ActualPar::generate_code()");
    }
  }

  char *ActualPar::rearrange_init_code(char *str, Common::Module* usage_mod)
  {
    switch (selection) {
    case AP_VALUE:
      str = val->rearrange_init_code(str, usage_mod);
      break;
    case AP_TEMPLATE:
      str = temp->rearrange_init_code(str, usage_mod);
    case AP_REF:
      break;
    case AP_DEFAULT:
      str = act->rearrange_init_code_defval(str, usage_mod);
      break;
    default:
      FATAL_ERROR("ActualPar::rearrange_init_code()");
    }
    return str;
  }

  char *ActualPar::rearrange_init_code_defval(char *str, Common::Module* usage_mod)
  {
    switch (selection) {
    case AP_VALUE:
      if (val->get_my_scope()->get_scope_mod_gen() == usage_mod) {
        str = val->generate_code_init(str, val->get_lhs_name().c_str());
      }
      break;
    case AP_TEMPLATE: {
      str = temp->rearrange_init_code(str, usage_mod);
      Template *t = temp->get_Template();
      if (t->get_my_scope()->get_scope_mod_gen() == usage_mod) {
        Ref_base *dref = temp->get_DerivedRef();
        if (dref) {
          expression_struct expr;
          Code::init_expr(&expr);
          expr.expr = mputprintf(expr.expr, "%s = ", t->get_lhs_name().c_str());
          dref->generate_code(&expr);
          str = Code::merge_free_expr(str, &expr);
        }
        str = t->generate_code_init(str, t->get_lhs_name().c_str());
      }
      break; }
    default:
      FATAL_ERROR("ActualPar::rearrange_init_code_defval()");
    }
    return str;
  }

  void ActualPar::append_stringRepr(string& str) const
  {
    switch (selection) {
    case AP_VALUE:
      str += val->get_stringRepr();
      break;
    case AP_TEMPLATE:
      temp->append_stringRepr(str);
      break;
    case AP_REF:
      str += ref->get_dispname();
      break;
    case AP_DEFAULT:
      str += '-';
      break;
    default:
      str += "<erroneous actual parameter>";
    }
  }

  void ActualPar::dump(unsigned level) const
  {
    switch (selection) {
    case AP_VALUE:
      DEBUG(level, "actual parameter: value");
      val->dump(level + 1);
      break;
    case AP_TEMPLATE:
      DEBUG(level, "actual parameter: template");
      temp->dump(level + 1);
      break;
    case AP_REF:
      DEBUG(level, "actual parameter: reference");
      ref->dump(level + 1);
      break;
    case AP_DEFAULT:
      DEBUG(level, "actual parameter: default");
      break;
    default:
      DEBUG(level, "actual parameter: erroneous");
    }
  }

  // =================================
  // ===== ActualParList
  // =================================

  ActualParList::ActualParList(const ActualParList& p)
    : Node(p)
  {
    size_t nof_pars = p.params.size();
    for (size_t i = 0; i < nof_pars; i++) params.add(p.params[i]->clone());
  }

  ActualParList::~ActualParList()
  {
    size_t nof_pars = params.size();
    for (size_t i = 0; i < nof_pars; i++) delete params[i];
    params.clear();
  }

  ActualParList *ActualParList::clone() const
  {
    return new ActualParList(*this);
  }

  void ActualParList::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    size_t nof_pars = params.size();
    for(size_t i = 0; i < nof_pars; i++)
      params[i]->set_fullname(p_fullname +
        ".<parameter" + Int2string(i + 1) + ">");
  }

  void ActualParList::set_my_scope(Scope *p_scope)
  {
    size_t nof_pars = params.size();
    for (size_t i = 0; i < nof_pars; i++) params[i]->set_my_scope(p_scope);
  }

  void ActualParList::chk_recursions(ReferenceChain& refch)
  {
    size_t nof_pars = params.size();
    for (size_t i = 0; i < nof_pars; i++)
      params[i]->chk_recursions(refch);
  }

  void ActualParList::generate_code_noalias(expression_struct *expr, FormalParList *p_fpl)
  {
    size_t nof_pars = params.size();
    for (size_t i = 0; i < nof_pars; i++) {
      if (i > 0) expr->expr = mputstr(expr->expr, ", ");
      params[i]->generate_code(expr, false, p_fpl != NULL ? p_fpl->get_fp_byIndex(i) : NULL);
    }
  }

  void ActualParList::generate_code_alias(expression_struct *expr,
    FormalParList *p_fpl, Type *p_comptype, bool p_compself)
  {
    size_t nof_pars = params.size();
    // collect all value and template definitions that are passed by reference
    map<Common::Assignment*, void> value_refs, template_refs;
    for (size_t i = 0; i < nof_pars; i++) {
      ActualPar *par = params[i];
      if (par->get_selection() == ActualPar::AP_DEFAULT)
        par = par->get_ActualPar();
      if (par->get_selection() == ActualPar::AP_REF) {
        Common::Assignment *ass = par->get_Ref()->get_refd_assignment();
        switch (ass->get_asstype()) {
        case Common::Assignment::A_VAR:
        case Common::Assignment::A_PAR_VAL_IN:
        case Common::Assignment::A_PAR_VAL_OUT:
        case Common::Assignment::A_PAR_VAL_INOUT:
          if (!value_refs.has_key(ass)) value_refs.add(ass, 0);
          break;
        case Common::Assignment::A_VAR_TEMPLATE:
        case Common::Assignment::A_PAR_TEMPL_IN:
        case Common::Assignment::A_PAR_TEMPL_OUT:
        case Common::Assignment::A_PAR_TEMPL_INOUT:
          if (!template_refs.has_key(ass)) template_refs.add(ass, 0);
        default:
          break;
        }
      }
    }
    // walk through the parameter list and generate the code
    // add an extra copy constructor call to the referenced value and template
    // parameters if the referred definition is also passed by reference to
    // another parameter
    bool all_in_params_shadowed = use_runtime_2 && p_fpl != NULL &&
      p_fpl->get_my_def()->get_asstype() == Common::Assignment::A_ALTSTEP;
    for (size_t i = 0; i < nof_pars; i++) {
      if (i > 0) expr->expr = mputstr(expr->expr, ", ");
      ActualPar *par = params[i];
      bool copy_needed = false;
      // the copy constructor call is not needed if the parameter is copied
      // into a shadow object in the body of the called function
      bool shadowed = false;
      if (p_fpl != NULL) {
        switch (p_fpl->get_fp_byIndex(i)->get_asstype()) {
        case Common::Assignment::A_PAR_VAL:
        case Common::Assignment::A_PAR_VAL_IN:
        case Common::Assignment::A_PAR_TEMPL_IN:
          // all 'in' parameters are shadowed in altsteps in RT2, otherwise an
          // 'in' parameter is shadowed if it is used as lvalue
          shadowed = all_in_params_shadowed ? true :
            p_fpl->get_fp_byIndex(i)->get_used_as_lvalue();
          break;
        default:
          break;
        }
      }
      if (!shadowed) {
        switch (par->get_selection()) {
        case ActualPar::AP_VALUE: {
          Value *v = par->get_Value();
          if (v->get_valuetype() == Value::V_REFD) {
            Common::Assignment *t_ass =
              v->get_reference()->get_refd_assignment();
            if (value_refs.has_key(t_ass)) {
              // a reference to the same variable is also passed to the called
              // definition
              copy_needed = true;
            } else if (p_comptype || p_compself) {
              // the called definition has a 'runs on' clause so it can access
              // component variables
              switch (t_ass->get_asstype()) {
              case Common::Assignment::A_PAR_VAL_OUT:
              case Common::Assignment::A_PAR_VAL_INOUT:
                // the parameter may be an alias of a component variable
                copy_needed = true;
                break;
              case Common::Assignment::A_VAR:
                // copy is needed if t_ass is a component variable that is
                // visible by the called definition
                if (!t_ass->is_local()) copy_needed = true;
                /** \todo component type compatibility: check whether t_ass is
                 * visible from p_comptype (otherwise copy is not needed) */
              default:
                break;
              }
            }
          }
          break; }
        case ActualPar::AP_TEMPLATE: {
          TemplateInstance *ti = par->get_TemplateInstance();
          if (!ti->get_DerivedRef()) {
            Template *t = ti->get_Template();
            if (t->get_templatetype() == Template::TEMPLATE_REFD) {
              Common::Assignment *t_ass =
                t->get_reference()->get_refd_assignment();
              if (template_refs.has_key(t_ass)) {
                // a reference to the same variable is also passed to the called
                // definition
                copy_needed = true;
              } else if (p_comptype || p_compself) {
                // the called definition has a 'runs on' clause so it can access
                // component variables
                switch (t_ass->get_asstype()) {
                case Common::Assignment::A_PAR_TEMPL_OUT:
                case Common::Assignment::A_PAR_TEMPL_INOUT:
                  // the parameter may be an alias of a component variable
                  copy_needed = true;
                  break;
                case Common::Assignment::A_VAR_TEMPLATE:
                  // copy is needed if t_ass is a component variable that is
                  // visible by the called definition
                  if (!t_ass->is_local()) copy_needed = true;
                  /** \todo component type compatibility: check whether t_ass is
                   * visible from p_comptype (otherwise copy is not needed) */
                default:
                  break;
                }
              }
            }
          } }
        default:
          break;
        }
      }
      
      if (use_runtime_2 && ActualPar::AP_REF == par->get_selection()) {
        // if the parameter references an element of a record of/set of, then
        // the record of object needs to know, so it doesn't delete the referenced
        // element
        Ref_base* ref = par->get_Ref();
        FieldOrArrayRefs* subrefs = ref->get_subrefs();
        if (subrefs != NULL) {
          Common::Assignment* ass = ref->get_refd_assignment();
          size_t ref_i;
          for (ref_i = 0; ref_i < subrefs->get_nof_refs(); ++ref_i) {
            FieldOrArrayRef* subref = subrefs->get_ref(ref_i);
            if (FieldOrArrayRef::ARRAY_REF == subref->get_type()) {
              // set the referenced index in each array in the subrefs
              expression_struct array_expr;
              Code::init_expr(&array_expr);
              // the array object's name contains the reference, followed by
              // the subrefs before the current array ref
              array_expr.expr = mcopystr(LazyFuzzyParamData::in_lazy_or_fuzzy() ?
                LazyFuzzyParamData::add_ref_genname(ass, ref->get_my_scope()).c_str() :
                ass->get_genname_from_scope(ref->get_my_scope()).c_str());
              if (ref_i > 0) {
                subrefs->generate_code(&array_expr, ass, ref_i);
              }
              expression_struct index_expr;
              Code::init_expr(&index_expr);
              subrefs->get_ref(ref_i)->get_val()->generate_code_expr(&index_expr);
              // insert any preambles the array object or the index might have
              if (array_expr.preamble != NULL) {
                expr->preamble = mputstr(expr->preamble, array_expr.preamble);
                expr->postamble = mputstr(expr->postamble, array_expr.preamble);
              }
              if (index_expr.preamble != NULL) {
                expr->preamble = mputstr(expr->preamble, index_expr.preamble);
                expr->postamble = mputstr(expr->postamble, index_expr.preamble);
              }
              // let the array object know that the index is referenced before
              // calling the function, and let it know that it's now longer
              // referenced after the function call (this is done with the help
              // of the RefdIndexHandler's constructor and destructor)
              string tmp_id = ref->get_my_scope()->get_scope_mod_gen()->get_temporary_id();
              expr->preamble = mputprintf(expr->preamble,
                "RefdIndexHandler %s(&%s, %s);\n",
                tmp_id.c_str(), array_expr.expr, index_expr.expr);
              // insert any postambles the array object or the index might have
              if (array_expr.postamble != NULL) {
                expr->preamble = mputstr(expr->preamble, array_expr.postamble);
                expr->postamble = mputstr(expr->postamble, array_expr.postamble);
              }
              if (index_expr.postamble != NULL) {
                expr->preamble = mputstr(expr->preamble, index_expr.postamble);
                expr->postamble = mputstr(expr->postamble, index_expr.postamble);
              }
              Code::free_expr(&array_expr);
              Code::free_expr(&index_expr);
            } // if (FieldOrArrayRef::ARRAY_REF == subref->get_type())
          } // for cycle
        } // if (subrefs != NULL)
      } // if (ActualPar::AP_REF == par->get_selection())
      
      par->generate_code(expr, copy_needed, p_fpl != NULL ? p_fpl->get_fp_byIndex(i) : NULL);
    }
    value_refs.clear();
    template_refs.clear();
  }

  char *ActualParList::rearrange_init_code(char *str, Common::Module* usage_mod)
  {
    for (size_t i = 0; i < params.size(); i++)
      str = params[i]->rearrange_init_code(str, usage_mod);
    return str;
  }

  void ActualParList::dump(unsigned level) const
  {
    DEBUG(level, "actual parameter list: %lu parameters",
      static_cast<unsigned long>( params.size()));
    for (size_t i = 0; i < params.size(); i++)
      params[i]->dump(level + 1);
  }
}
