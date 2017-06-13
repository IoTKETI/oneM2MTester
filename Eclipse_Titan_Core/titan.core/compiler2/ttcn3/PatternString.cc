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
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "PatternString.hh"
#include "../../common/pattern.hh"
#include "../CompilerError.hh"
#include "../Code.hh"
#include "../../common/JSON_Tokenizer.hh"
#include "../main.hh"

#include "TtcnTemplate.hh"

namespace Ttcn {

  // =================================
  // ===== PatternString::ps_elem_t
  // =================================

  struct PatternString::ps_elem_t {
    enum kind_t {
      PSE_STR,
      PSE_REF,
      PSE_REFDSET
    } kind;
    string *str;
    Ttcn::Reference *ref;
    Type * t; // The type of the reference in the case of PSE_REFDSET
    boolean with_N; // If the reference was given as \N{ref} in the pattern
    boolean is_charstring; // \N{charstring}
    boolean is_universal_charstring; // \N{universal charstring}
    ps_elem_t(kind_t p_kind, const string& p_str);
    ps_elem_t(kind_t p_kind, Ttcn::Reference *p_ref, boolean N);
    ps_elem_t(const ps_elem_t& p);
    ~ps_elem_t();
    ps_elem_t* clone() const;
    void set_fullname(const string& p_fullname);
    void set_my_scope(Scope *p_scope);
    void chk_ref(PatternString::pstr_type_t pstr_type, Type::expected_value_t expected_value);
    void set_code_section(GovernedSimple::code_section_t p_code_section);
  };

  PatternString::ps_elem_t::ps_elem_t(kind_t p_kind, const string& p_str)
    : kind(p_kind), ref(NULL), t(NULL), with_N(FALSE), is_charstring(FALSE),
    is_universal_charstring(FALSE)
  {
    str = new string(p_str);
  }

  PatternString::ps_elem_t::ps_elem_t(kind_t p_kind, Ttcn::Reference *p_ref, boolean N)
    : kind(p_kind), str(NULL), with_N(N), is_charstring(FALSE),
    is_universal_charstring(FALSE)
  {
    if (!p_ref) FATAL_ERROR("PatternString::ps_elem_t::ps_elem_t()");
    ref = p_ref;
  }

  PatternString::ps_elem_t::~ps_elem_t()
  {
    switch(kind) {
    case PSE_STR:
      delete str;
      // fall through
    case PSE_REF:
    case PSE_REFDSET:
      delete ref;
      // do not delete t
      break;
    } // switch kind
  }

  PatternString::ps_elem_t* PatternString::ps_elem_t::clone() const
  {
    FATAL_ERROR("PatternString::ps_elem_t::clone");
  }

  void PatternString::ps_elem_t::set_fullname(const string& p_fullname)
  {
    switch(kind) {
    case PSE_REF:
    case PSE_REFDSET:
      ref->set_fullname(p_fullname);
      break;
    default:
      ;
    } // switch kind
  }

  void PatternString::ps_elem_t::set_my_scope(Scope *p_scope)
  {
    switch(kind) {
    case PSE_REF:
    case PSE_REFDSET:
      ref->set_my_scope(p_scope);
      break;
    default:
      ;
    } // switch kind
  }

  void PatternString::ps_elem_t::chk_ref(PatternString::pstr_type_t pstr_type, Type::expected_value_t expected_value)
  {
    if (kind != PSE_REF) FATAL_ERROR("PatternString::ps_elem_t::chk_ref()");
    Value* v = 0;
    Value* v_last = 0;
    if (ref->get_id()->get_name() == "CHARSTRING") {
      is_charstring = TRUE;
      return;
    } else if (ref->get_id()->get_name() == "UNIVERSAL_CHARSTRING") {
      is_universal_charstring = TRUE;
      return;
    }
    Common::Assignment* ass = ref->get_refd_assignment();
    if (!ass)
      return;
    Ttcn::FieldOrArrayRefs* t_subrefs = ref->get_subrefs();
    Type* ref_type = ass->get_Type()->get_type_refd_last()->get_field_type(
      t_subrefs, expected_value);
    Type::typetype_t tt;
    switch (pstr_type) {
      case PatternString::CSTR_PATTERN:
        tt = Type::T_CSTR;
        if (ref_type->get_typetype() != Type::T_CSTR)
          TTCN_pattern_error("Type of the referenced %s '%s' should be "
            "'charstring'", ass->get_assname(), ref->get_dispname().c_str());
        break;
      case PatternString::USTR_PATTERN:
        tt = ref_type->get_typetype();
        if (tt != Type::T_CSTR && tt != Type::T_USTR)
          TTCN_pattern_error("Type of the referenced %s '%s' should be either "
            "'charstring' or 'universal charstring'", ass->get_assname(),
            ref->get_dispname().c_str());
        break;
      default:
        FATAL_ERROR("Unknown pattern string type");
    }
    Type* refcheckertype = Type::get_pooltype(tt);
    switch (ass->get_asstype()) {
    case Common::Assignment::A_TYPE:
      kind = PSE_REFDSET;
      t = ass->get_Type();
      break;
    case Common::Assignment::A_MODULEPAR_TEMP:
    case Common::Assignment::A_VAR_TEMPLATE: 
    case Common::Assignment::A_PAR_TEMPL_IN:
    case Common::Assignment::A_PAR_TEMPL_OUT:
    case Common::Assignment::A_PAR_TEMPL_INOUT:
      // error reporting moved up
      break;
    case Common::Assignment::A_TEMPLATE: {
      Template* templ = ass->get_Template();
      refcheckertype->chk_this_template_ref(templ);
      refcheckertype->chk_this_template_generic(templ, INCOMPLETE_ALLOWED,
        OMIT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK, NOT_IMPLICIT_OMIT, 0);
      switch (templ->get_templatetype()) {
      case Template::SPECIFIC_VALUE:
        v_last = templ->get_specific_value();
        break;
      case Template::TEMPLATE_CONCAT:
        if (!use_runtime_2) {
          FATAL_ERROR("PatternString::ps_elem_t::chk_ref()");
        }
        if (templ->is_Value()) {
          v = templ->get_Value();
          v->set_my_governor(refcheckertype);
          v->set_my_scope(ref->get_my_scope());
          v->set_location(*ref);
          refcheckertype->chk_this_value(v, 0, expected_value,
            INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
          v_last = v->get_value_refd_last();
        }
        else {
          TTCN_pattern_error("Unable to resolve referenced '%s' to character "
            "string type. Result of template concatenation is not a specific "
            "value.", ref->get_dispname().c_str());
        }
        break;
      case Template::CSTR_PATTERN: 
        if (!with_N) {
          Ttcn::PatternString* ps = templ->get_cstr_pattern();
          if (!ps->has_refs())
            v_last = ps->get_value();
          break;
        }
      case Template::USTR_PATTERN: 
        if (!with_N) {
          Ttcn::PatternString* ps = templ->get_ustr_pattern();
          if (!ps->has_refs())
            v_last = ps->get_value();
          break;
        }
      default:
        TTCN_pattern_error("Unable to resolve referenced '%s' to character "
          "string type. '%s' template cannot be used.",
          ref->get_dispname().c_str(), templ->get_templatetype_str());
        break;
      }
      break; }
    default: {
      Reference *t_ref = ref->clone();
      t_ref->set_location(*ref);
      v = new Value(Value::V_REFD, t_ref);
      v->set_my_governor(refcheckertype);
      v->set_my_scope(ref->get_my_scope());
      v->set_location(*ref);
      refcheckertype->chk_this_value(v, 0, expected_value,
        INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
      v_last = v->get_value_refd_last();
    }
    }
    if (v_last && (v_last->get_valuetype() == Value::V_CSTR ||
      v_last->get_valuetype() == Value::V_USTR)) {
      // the reference points to a constant
      // substitute the reference with the known value
      if (v_last->get_valuetype() == Value::V_CSTR) {
        if (with_N && v_last->get_val_str().size() != 1) {
          ref->error("The length of the charstring must be of length one, when it is being referenced in a pattern with \\N{ref}");
        }
        str = new string(v_last->get_val_str());
      } else {
        if (with_N && v_last->get_val_ustr().size() != 1) {
          ref->error("The length of the universal charstring must be of length one, when it is being referenced in a pattern with \\N{ref}");
        }
        str = new string(v_last->get_val_ustr().get_stringRepr_for_pattern());
      }
      kind = PSE_STR;
    }
    delete v;
  }

  void PatternString::ps_elem_t::set_code_section
    (GovernedSimple::code_section_t p_code_section)
  {
    switch(kind) {
    case PSE_REF:
    case PSE_REFDSET:
      ref->set_code_section(p_code_section);
      break;
    default:
      ;
    } // switch kind
  }

  // =================================
  // ===== PatternString
  // =================================

  PatternString::PatternString(const PatternString& p)
    : Node(p), my_scope(0), pattern_type(p.pattern_type)
  {
    size_t nof_elems = p.elems.size();
    for (size_t i = 0; i < nof_elems; i++) elems.add(p.elems[i]->clone());
  }

  PatternString::ps_elem_t *PatternString::get_last_elem() const
  {
    if (elems.empty()) return 0;
    ps_elem_t *last_elem = elems[elems.size() - 1];
    if (last_elem->kind == ps_elem_t::PSE_STR) return last_elem;
    else return 0;
  }

  PatternString::~PatternString()
  {
    size_t nof_elems = elems.size();
    for (size_t i = 0; i < nof_elems; i++) delete elems[i];
    elems.clear();
    delete cstr_value;
  }

  PatternString *PatternString::clone() const
  {
    return new PatternString(*this);
  }

  void PatternString::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    size_t nof_elems = elems.size();
    for(size_t i = 0; i < nof_elems; i++) elems[i]->set_fullname(p_fullname);
  }

  void PatternString::set_my_scope(Scope *p_scope)
  {
    my_scope = p_scope;
    size_t nof_elems = elems.size();
    for (size_t i = 0; i < nof_elems; i++) elems[i]->set_my_scope(p_scope);
  }

  void PatternString::set_code_section
    (GovernedSimple::code_section_t p_code_section)
  {
    size_t nof_elems = elems.size();
    for (size_t i = 0; i < nof_elems; i++)
      elems[i]->set_code_section(p_code_section);
  }

  void PatternString::addChar(char c)
  {
    ps_elem_t *last_elem = get_last_elem();
    if (last_elem) *last_elem->str += c;
    else elems.add(new ps_elem_t(ps_elem_t::PSE_STR, string(c)));
  }

  void PatternString::addString(const char *p_str)
  {
    ps_elem_t *last_elem = get_last_elem();
    if (last_elem) *last_elem->str += p_str;
    else elems.add(new ps_elem_t(ps_elem_t::PSE_STR, string(p_str)));
  }

  void PatternString::addString(const string& p_str)
  {
    ps_elem_t *last_elem = get_last_elem();
    if (last_elem) *last_elem->str += p_str;
    else elems.add(new ps_elem_t(ps_elem_t::PSE_STR, p_str));
  }
  
  void PatternString::addStringUSI(char **usi_str, const size_t size)
  {
    ustring s = ustring(const_cast<const char**>(usi_str), size);
    ps_elem_t *last_elem = get_last_elem();
    if (last_elem) *last_elem->str += s.get_stringRepr_for_pattern().c_str();
    else elems.add(new ps_elem_t(ps_elem_t::PSE_STR, s.get_stringRepr_for_pattern()));
  }

  void PatternString::addRef(Ttcn::Reference *p_ref, boolean N)
  {
    elems.add(new ps_elem_t(ps_elem_t::PSE_REF, p_ref, N));
  }

  string PatternString::get_full_str() const
  {
    string s;
    for(size_t i=0; i<elems.size(); i++) {
      ps_elem_t *pse=elems[i];
      switch(pse->kind) {
      case ps_elem_t::PSE_STR:
        s+=*pse->str;
        break;
      case ps_elem_t::PSE_REFDSET:
        s+="\\N";
        /* no break */
      case ps_elem_t::PSE_REF:
        s+='{';
        s+=pse->ref->get_dispname();
        s+='}';
      } // switch kind
    } // for
    return s;
  }

  void PatternString::set_pattern_type(pstr_type_t p_type) {
    pattern_type = p_type;
  }

  PatternString::pstr_type_t PatternString::get_pattern_type() const {
    return pattern_type;
  }

  bool PatternString::has_refs() const
  {
    for (size_t i = 0; i < elems.size(); i++) {
      switch (elems[i]->kind) {
      case ps_elem_t::PSE_REF:
      case ps_elem_t::PSE_REFDSET:
        return true;
      default:
        break;
      }
    }
    return false;
  }

  void PatternString::chk_refs(Type::expected_value_t expected_value)
  {
    for(size_t i=0; i<elems.size(); i++) {
      ps_elem_t *pse=elems[i];
      switch(pse->kind) {
      case ps_elem_t::PSE_STR:
        break;
      case ps_elem_t::PSE_REFDSET:
        /* actually, not supported */
        break;
      case ps_elem_t::PSE_REF:
        pse->chk_ref(pattern_type, expected_value);
        break;
      } // switch kind
    } // for
  }

  /** \todo implement */
  void PatternString::chk_recursions(ReferenceChain&)
  {

  }

  void PatternString::chk_pattern()
  {
    string str;
    for (size_t i = 0; i < elems.size(); i++) {
      ps_elem_t *pse = elems[i];
      if (pse->kind != ps_elem_t::PSE_STR)
	      FATAL_ERROR("PatternString::chk_pattern()");
      str += *pse->str;
    }
    char* posix_str = 0;
    switch (pattern_type) {
      case CSTR_PATTERN:
        posix_str = TTCN_pattern_to_regexp(str.c_str());
        break;
      case USTR_PATTERN:
        posix_str = TTCN_pattern_to_regexp_uni(str.c_str(), nocase);
    }
    Free(posix_str);
  }

  bool PatternString::chk_self_ref(Common::Assignment *lhs)
  {
    for (size_t i = 0, e = elems.size(); i < e; ++i) {
      ps_elem_t *pse = elems[i];
      switch (pse->kind) {
      case ps_elem_t::PSE_STR:
        break;
      case ps_elem_t::PSE_REFDSET:
        /* actually, not supported */
        break;
      case ps_elem_t::PSE_REF: {
        Ttcn::Assignment *ass = pse->ref->get_refd_assignment();
        if (ass == lhs) return true;
        break; }
      } // switch
    }
    return false;
  }

  void PatternString::join_strings()
  {
    // points to the previous string element otherwise it is NULL
    ps_elem_t *prev_str = 0;
    for (size_t i = 0; i < elems.size(); ) {
      ps_elem_t *pse = elems[i];
      if (pse->kind == ps_elem_t::PSE_STR) {
        const string& str = *pse->str;
	if (str.size() > 0) {
	  // the current element is a non-empty string
	  if (prev_str) {
	    // append str to prev_str and drop pse
	    *prev_str->str += str;
	    delete pse;
	    elems.replace(i, 1);
	    // don't increment i
	  } else {
	    // keep pse for the next iteration
	    prev_str = pse;
	    i++;
	  }
	} else {
	  // the current element is an empty string
	  // simply drop it
	  delete pse;
	  elems.replace(i, 1);
	  // don't increment i
	}
      } else {
        // pse is not a string
	// forget prev_str
	prev_str = 0;
	i++;
      }
    }
  }

  string PatternString::create_charstring_literals(Common::Module *p_mod, string& preamble)
  {
    /* The cast is there for the benefit of OPTIONAL<CHARSTRING>, because
     * it doesn't have operator+(). Only the first member needs the cast
     * (the others will be automagically converted to satisfy
     * CHARSTRING::operator+(const CHARSTRING&) ) */
    string s;
    if (pattern_type == CSTR_PATTERN)
      s = "CHARSTRING_template(STRING_PATTERN, (CHARSTRING)";
    else
      s = "UNIVERSAL_CHARSTRING_template(STRING_PATTERN, (CHARSTRING)";
    size_t nof_elems = elems.size();
    if (nof_elems > 0) {
      // the pattern is not empty
    for (size_t i = 0; i < nof_elems; i++) {
      if (i > 0) s += " + ";
      ps_elem_t *pse = elems[i];
      
      // \N{charstring} and \N{universal charstring}
      if (pse->is_charstring) {
        s += p_mod->add_charstring_literal(string("?"));
        continue;
      } else if (pse->is_universal_charstring) {
        s += p_mod->add_charstring_literal(string("?"));
        continue;
      }

      switch (pse->kind) {
        // Known in compile time: string literal, const etc.
        case ps_elem_t::PSE_STR:
          s += p_mod->add_charstring_literal(*pse->str);
          break;
        // Known in compile time: string type with(out) range or list
        case ps_elem_t::PSE_REFDSET: {
          if (!pse->t)
            FATAL_ERROR("PatternString::create_charstring_literals()");
          if (!pse->t->get_sub_type()) {
            // just a string type without any restrictions (or alias)
            s += "\"?\"";
            continue;
          }
          vector<Common::SubTypeParse> * vec = pse->t->get_sub_type()->get_subtype_parsed();

          while (vec == NULL) { // go through aliases to find where the restrictions are
            if (pse->t->get_Reference()) {
              pse->t = pse->t->get_Reference()->get_refd_assignment(FALSE)->get_Type();
            } else {
              break;
            }
            if (pse->t->get_sub_type()) {
              vec = pse->t->get_sub_type()->get_subtype_parsed();
            } else {
              break;
            }
          }
          if (vec == NULL) {
            // I don't think it can happen, but to be sure...
            s += "\"?\"";
            continue;
          }
          s+= "\"";
          if (vec->size() > 1) s+= "(";
          for (size_t j = 0; j < vec->size(); j++) {
            Common::SubTypeParse* stp = (*vec)[j];
            if (j > 0) {
              s+="|\"+"; // todo what if default    
            }else {
              s+= "\"+";
            }
            switch (stp->get_selection()) {
              case SubTypeParse::STP_RANGE: // type charstring ("a" .. "z")
                s+="\"[\" + ";
                switch (stp->Min()->get_valuetype()) {
                  case Value::V_CSTR:
                    s+= p_mod->add_charstring_literal(stp->Min()->get_val_str());
                    s+= "+ \"-\" +";
                    s+= p_mod->add_charstring_literal(stp->Max()->get_val_str());
                    break;
                  case Value::V_USTR:
                    s+= p_mod->add_charstring_literal(stp->Min()->get_val_ustr().get_stringRepr_for_pattern());
                    s+= "+ \"-\" +";
                    s+= p_mod->add_charstring_literal(stp->Max()->get_val_ustr().get_stringRepr_for_pattern());
                    break;
                  default:
                    FATAL_ERROR("PatternString::create_charstring_literals()");
                }
                s+=" + \"]\"";
                break;
              case SubTypeParse::STP_SINGLE: // type charstring ("a", "b")
                switch (stp->Single()->get_valuetype()) {
                  case Value::V_CSTR:
                    s+= p_mod->add_charstring_literal(stp->Single()->get_val_str());
                    break;
                  case Value::V_USTR:
                    s+= p_mod->add_charstring_literal(stp->Single()->get_val_ustr().get_stringRepr_for_pattern());
                    break;
                  default:
                    FATAL_ERROR("PatternString::create_charstring_literals()");
                    break;
                }
                break;
              default:
                FATAL_ERROR("PatternString::create_charstring_literals()");
            }
            s+= "+\"";
          }
          if (vec->size() > 1) s+= ")";
          s+= "\"";
          break; }
        // Not known in compile time
        case ps_elem_t::PSE_REF: {
          expression_struct expr;
          Code::init_expr(&expr);
          pse->ref->generate_code(&expr);
          if (expr.preamble || expr.postamble)
            FATAL_ERROR("PatternString::create_charstring_literals()");
          s += expr.expr;
          Common::Assignment* assign = pse->ref->get_refd_assignment();
          char* str = NULL;
          
          // TODO: these checks will generated each time a reference is referenced in a pattern
          // and it could be generated once
          if (pse->with_N) {
            if ((assign->get_asstype() == Common::Assignment::A_TEMPLATE
                  || assign->get_asstype() == Common::Assignment::A_MODULEPAR_TEMP
                  || assign->get_asstype() == Common::Assignment::A_VAR_TEMPLATE
                  || assign->get_asstype() == Common::Assignment::A_PAR_TEMPL_IN
                  || assign->get_asstype() == Common::Assignment::A_PAR_TEMPL_OUT
                  || assign->get_asstype() == Common::Assignment::A_PAR_TEMPL_INOUT))
            {
            string value_literal = p_mod->add_charstring_literal(string("value"));
            str = mputprintf(str,
              "if (%s.get_istemplate_kind(%s) == FALSE) {\n"
              "TTCN_error(\"Only specific value template allowed in pattern reference with \\\\N{ref}\");\n"
              "}\n"
              , expr.expr
              , value_literal.c_str());
            }
            
            str = mputprintf(str,
              "if (%s.lengthof() != 1)\n"
              "{\n"
              "TTCN_error(\"The length of the %scharstring must be of length one, when it is being referenced in a pattern with \\\\N{ref}\");\n"
              "}\n"
              , expr.expr
              , assign->get_Type()->get_typetype() == Type::T_USTR ? "universal " : "");
            preamble += str;
            Free(str);
          }
          if ((assign->get_asstype() == Common::Assignment::A_TEMPLATE
            || assign->get_asstype() == Common::Assignment::A_MODULEPAR_TEMP
            || assign->get_asstype() == Common::Assignment::A_VAR_TEMPLATE
            || assign->get_asstype() == Common::Assignment::A_PAR_TEMPL_IN
            || assign->get_asstype() == Common::Assignment::A_PAR_TEMPL_OUT
            || assign->get_asstype() == Common::Assignment::A_PAR_TEMPL_INOUT))
          {
            if ((assign->get_Type()->get_typetype() == Type::T_CSTR
              || assign->get_Type()->get_typetype() == Type::T_USTR) && !pse->with_N) {
              s += ".get_single_value()";
            }
            else if (assign->get_Type()->get_typetype() == Type::T_USTR && pse->with_N) {
              s += ".valueof().get_stringRepr_for_pattern()";
            } else {
              s += ".valueof()";
            }
          } else if ((assign->get_asstype() == Common::Assignment::A_MODULEPAR
            || assign->get_asstype() == Common::Assignment::A_VAR
            || assign->get_asstype() == Common::Assignment::A_PAR_VAL
            || assign->get_asstype() == Common::Assignment::A_PAR_VAL_IN
            || assign->get_asstype() == Common::Assignment::A_PAR_VAL_OUT
            || assign->get_asstype() == Common::Assignment::A_PAR_VAL_INOUT)
            && assign->get_Type()->get_typetype() == Type::T_USTR) {
            s += ".get_stringRepr_for_pattern()";
          }

          Code::free_expr(&expr);
          break; }
        } // switch kind
      } // for
    } else {
      // empty pattern: create an empty string literal for it
      s += p_mod->add_charstring_literal(string());
    }
    s += ", ";
    s += nocase ? "TRUE" : "FALSE";
    s += ')';
    return s;
  }

  void PatternString::dump(unsigned level) const
  {
    if (nocase) {
      DEBUG(level, "@nocase");
    }
    DEBUG(level, "%s", get_full_str().c_str());
  }

  Common::Value* PatternString::get_value() {
    if (!cstr_value && !has_refs())
      cstr_value = new Common::Value(Common::Value::V_CSTR,
        new string(get_full_str()));
    return cstr_value;
  }
  
  char* PatternString::convert_to_json()
  {
    string pstr = get_value()->get_val_str();
    
    // convert the pattern into an extended regular expression
    char* regex_str = NULL;
    if (CSTR_PATTERN == pattern_type) {
      regex_str = TTCN_pattern_to_regexp(pstr.c_str());
    }
    else { // USTR_PATTERN
      // handle the unicode characters in \q{g,p,r,c} format
      string utf8str;
      for (size_t i = 0; i < pstr.size(); ++i) {
        if ('\\' == pstr[i]) {
          if ('q' == pstr[i + 1]) {
            // extract the unicode character
            unsigned int group, plane, row, cell;
            i = pstr.find('{', i + 1);
            sscanf(pstr.c_str() + i + 1, "%u", &group);
            i = pstr.find(',', i + 1);
            sscanf(pstr.c_str() + i + 1, "%u", &plane);
            i = pstr.find(',', i + 1);
            sscanf(pstr.c_str() + i + 1, "%u", &row);
            i = pstr.find(',', i + 1);
            sscanf(pstr.c_str() + i + 1, "%u", &cell);
            i = pstr.find('}', i + 1);
            
            // convert the character to UTF-8 format
            utf8str += ustring_to_uft8(ustring(group, plane, row, cell));
            continue;
          }
          else if ('\\' == pstr[i + 1]) {
            // must be handled separately, so we don't confuse \\q with \q
            ++i;
            utf8str += '\\';
          }
        }
        utf8str += pstr[i];
      }
      
      // use the pattern converter for charstrings, the pattern should be in UTF-8
      // format now (setting the 2nd parameter will make sure that no error
      // messages are displayed for extended ASCII characters)
      regex_str = TTCN_pattern_to_regexp(utf8str.c_str(), true);
    }
    
    char* json_str = convert_to_json_string(regex_str);
    Free(regex_str);
    return json_str;
  }

} // namespace Ttcn

  // =================================
  // ===== TTCN_pattern_XXXX
  // =================================

/* These functions are used by common charstring pattern parser. */

void TTCN_pattern_error(const char *fmt, ...)
{
  char *msg=mcopystr("Charstring pattern: ");
  msg=mputstr(msg, fmt);
  va_list args;
  va_start(args, fmt);
  Common::Error_Context::report_error(0, msg, args);
  va_end(args);
  Free(msg);
}

void TTCN_pattern_warning(const char *fmt, ...)
{
  char *msg=mcopystr("Charstring pattern: ");
  msg=mputstr(msg, fmt);
  va_list args;
  va_start(args, fmt);
  Common::Error_Context::report_warning(0, msg, args);
  va_end(args);
  Free(msg);
}
