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
 *   Cserveni, Akos
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szabo, Bence Janos
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include "../../common/dbgnew.hh"
#include "Templatestuff.hh"
#include "../Identifier.hh"
#include "TtcnTemplate.hh"
#include "ArrayDimensions.hh"

#include <limits.h>

namespace Ttcn {

  // =================================
  // ===== ValueRange
  // =================================

  ValueRange::~ValueRange()
  {
    delete min_v;
    delete max_v;
  }

  ValueRange *ValueRange::clone() const
  {
    return new ValueRange(*this);
  }

  ValueRange::ValueRange(const ValueRange& other)
  : Node(other)
  , min_v(other.min_v ? other.min_v->clone() : 0)
  , max_v(other.max_v ? other.max_v->clone() : 0)
  , min_exclusive(other.min_exclusive)
  , max_exclusive(other.max_exclusive)
  , type(other.type)
  {}

  void ValueRange::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (min_v) min_v->set_fullname(p_fullname + ".<lower_bound>");
    if (max_v) max_v->set_fullname(p_fullname + ".<upper_bound>");
  }

  void ValueRange::set_my_scope(Scope *p_scope)
  {
    Node::set_my_scope(p_scope);
    if (min_v) min_v->set_my_scope(p_scope);
    if (max_v) max_v->set_my_scope(p_scope);
  }

  void ValueRange::set_lowerid_to_ref()
  {
    if (min_v) min_v->set_lowerid_to_ref();
    if (max_v) max_v->set_lowerid_to_ref();
  }

  Type::typetype_t ValueRange::get_expr_returntype(
    Type::expected_value_t exp_val)
  {
    if (min_v) {
      Type::typetype_t tt = min_v->get_expr_returntype(exp_val);
      if (tt != Type::T_UNDEF) return tt;
    }
    if (max_v) {
      Type::typetype_t tt = max_v->get_expr_returntype(exp_val);
      if (tt != Type::T_UNDEF) return tt;
    }
    return Type::T_UNDEF;
  }

  Type *ValueRange::get_expr_governor(Type::expected_value_t exp_val)
  {
    if (min_v) {
      Type *t = min_v->get_expr_governor(exp_val);
      if (t) return t;
    }
    if (max_v) {
      Type *t = max_v->get_expr_governor(exp_val);
      if (t) return t;
    }
    return 0;
  }

  void ValueRange::set_code_section
    (GovernedSimple::code_section_t p_code_section)
  {
    if (min_v) min_v->set_code_section(p_code_section);
    if (max_v) max_v->set_code_section(p_code_section);
  }
  
  void ValueRange::set_typetype(Type::typetype_t t) {
    type = t;
  }

  char *ValueRange::generate_code_init(char *str, const char *name)
  {
    expression_struct expr;
    Code::init_expr(&expr);
    char *init_stmt = mprintf("%s.set_type(VALUE_RANGE);\n", name);
    if (min_v) {
      min_v->generate_code_expr(&expr);
      init_stmt = mputprintf(init_stmt, "%s.set_min(%s);\n", name, expr.expr);
    }
    if (min_exclusive) {
      switch (type) {
        case Type::T_INT:
        case Type::T_REAL:
        case Type::T_CSTR:
        case Type::T_USTR:
          init_stmt = mputprintf(init_stmt, "%s.set_min_exclusive(true);\n", name);
          break;
        default:
          FATAL_ERROR("ValueRange::generate_code_init()");
      }
    }
    if (max_v) {
      Code::clean_expr(&expr);
      max_v->generate_code_expr(&expr);
      init_stmt = mputprintf(init_stmt, "%s.set_max(%s);\n", name, expr.expr);
    }
    if (max_exclusive) {
      switch (type) {
        case Type::T_INT:
        case Type::T_REAL:
        case Type::T_CSTR:
        case Type::T_USTR:
          init_stmt = mputprintf(init_stmt, "%s.set_max_exclusive(true);\n", name);
          break;
        default:
          FATAL_ERROR("ValueRange::generate_code_init()");
      }
    }
    if (expr.preamble || expr.postamble) {
      str = mputstr(str, "{\n");
      str = mputstr(str, expr.preamble);
      str = mputstr(str, init_stmt);
      str = mputstr(str, expr.postamble);
      str = mputstr(str, "}\n");
    } else str = mputstr(str, init_stmt);
    Code::free_expr(&expr);
    Free(init_stmt);
    return str;
  }

  char *ValueRange::rearrange_init_code(char *str, Common::Module* usage_mod)
  {
    if (min_v) str = min_v->rearrange_init_code(str, usage_mod);
    if (max_v) str = max_v->rearrange_init_code(str, usage_mod);
    return str;
  }

  void ValueRange::append_stringRepr(string& str) const
  {
    str += "(";
    if (min_v) str += min_v->get_stringRepr();
    else str += "-infinity";
    str += " .. ";
    if (max_v) str += max_v->get_stringRepr();
    else str += "infinity";
    str += ")";
  }

  void ValueRange::dump(unsigned level) const
  {
    DEBUG(level, "(");
    if (min_v) min_v->dump(level + 1);
    else DEBUG(level + 1, "-infinity");
    DEBUG(level + 1, "..");
    if (max_v) max_v->dump(level + 1);
    else DEBUG(level + 1, "infinity");
    DEBUG(level, ")");
  }

  // =================================
  // ===== Templates
  // =================================

  Templates::~Templates()
  {
     for (size_t i = 0; i < ts.size(); i++) delete ts[i];
     ts.clear();
  }

  Templates::Templates(const Templates& other)
  : Node(), ts()
  {
    for (size_t i = 0, lim = other.ts.size(); i < lim; ++i) {
      ts.add(other.ts[i]->clone());
    }
  }

  Templates *Templates::clone() const
  {
    return new Templates(*this);
  }

  void Templates::set_my_scope(Scope *p_scope)
  {
    for (size_t i = 0; i < ts.size(); i++) ts[i]->set_my_scope(p_scope);
  }

  void Templates::add_t(Template *p_t)
  {
     if (!p_t) FATAL_ERROR("NULL pointer:Templates::add_t()");
     ts.add(p_t);
  }

  void Templates::add_front_t(Template *p_t)
  {
     if (!p_t) FATAL_ERROR("NULL pointer:Templates::add_front_t()");
     ts.add_front(p_t);
  }

  void Templates::append_stringRepr(string& str) const
  {
    for (size_t i = 0; i < ts.size(); i++) {
      if (i > 0) str += ", ";
      str += ts[i]->get_stringRepr();
    }
  }

  // =================================
  // ===== IndexedTemplate
  // =================================
  
  IndexedTemplate::IndexedTemplate(const IndexedTemplate& p)
  : Node(p), Location(p)
  {
    index = p.index->clone();
    temp = p.temp->clone();
  }

  IndexedTemplate::IndexedTemplate(FieldOrArrayRef *p_i, Template *p_t)
  {
    if (!p_i || !p_t) FATAL_ERROR("IndexedTemplate::IndexedTemplate()");
    index = p_i;
    temp = p_t;
  }

  IndexedTemplate::~IndexedTemplate()
  {
    delete index;
    delete temp;
  }

  IndexedTemplate *IndexedTemplate::clone() const
  {
    return new IndexedTemplate(*this);
  }

  void IndexedTemplate::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    temp->set_fullname(p_fullname);
    index->set_fullname(p_fullname);
  }

  void IndexedTemplate::set_my_scope(Scope *p_scope)
  {
    Node::set_my_scope(p_scope);
    temp->set_my_scope(p_scope);
    index->set_my_scope(p_scope);
  }

  void IndexedTemplate::set_code_section
    (GovernedSimple::code_section_t p_code_section)
  {
    index->get_val()->set_code_section(p_code_section);
    temp->set_code_section(p_code_section);
  }

  void IndexedTemplate::dump(unsigned level) const
  {
    index->dump(level);
    temp->dump(level);
  }

  // =================================
  // ===== IndexedTemplates
  // =================================
  
  IndexedTemplates::IndexedTemplates(const IndexedTemplates& p)
  : Node(p)
  {
    for (size_t i = 0; i < p.its_v.size(); i++) {
      its_v.add(p.its_v[i]->clone());
    }
  }

  IndexedTemplates::~IndexedTemplates()
  {
    for (size_t i = 0; i < its_v.size(); i++) delete its_v[i];
    its_v.clear();
  }

  IndexedTemplates *IndexedTemplates::clone() const
  {
    return new IndexedTemplates(*this);
  }

  void IndexedTemplates::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for(size_t i = 0; i < its_v.size(); i++) {
      IndexedTemplate *it = its_v[i];
      string t_fullname = p_fullname;
      (it->get_index()).append_stringRepr(t_fullname);
      it->set_fullname(t_fullname);
    }
  }

  void IndexedTemplates::set_my_scope(Scope *p_scope)
  {
    Node::set_my_scope(p_scope);
    for (size_t i = 0; i < its_v.size(); i++)
      its_v[i]->set_my_scope(p_scope);
  }

  void IndexedTemplates::add_it(IndexedTemplate *p_it)
  {
    if (!p_it) FATAL_ERROR("NULL pointer: IndexedTemplates::add_it()");
    its_v.add(p_it);
  }

  IndexedTemplate *IndexedTemplates::get_it_byIndex(size_t p_i)
  {
    return its_v[p_i];
  }

  // =================================
  // ===== NamedTemplate
  // =================================

  NamedTemplate::NamedTemplate(Identifier *p_n, Template *p_t)
  {
    if (!p_n || !p_t) FATAL_ERROR("NamedTemplate::NamedTemplate()");
    name = p_n;
    temp = p_t;
  }
  
  NamedTemplate::NamedTemplate(const NamedTemplate& p)
  : Node(p), Location(p)
  {
    name = p.name->clone();
    temp = p.temp->clone();
  }

  NamedTemplate::~NamedTemplate()
  {
    delete name;
    delete temp;
  }

  NamedTemplate *NamedTemplate::clone() const
  {
    return new NamedTemplate(*this);
  }

  void NamedTemplate::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    temp->set_fullname(p_fullname);
  }

  void NamedTemplate::set_my_scope(Scope *p_scope)
  {
    Node::set_my_scope(p_scope);
    temp->set_my_scope(p_scope);
  }
  
  void NamedTemplate::set_name_to_lowercase()
  {
    string new_name = name->get_name();
    if (isupper(new_name[0])) {
      new_name[0] = (char)tolower(new_name[0]);
      if (new_name[new_name.size() - 1] == '_') {
        // an underscore is inserted at the end of the alternative name if it's
        // a basic type's name (since it would conflict with the class generated
        // for that type)
        // remove the underscore, it won't conflict with anything if its name
        // starts with a lowercase letter
        new_name.replace(new_name.size() - 1, 1, "");
      }
      delete name;
      name = new Identifier(Identifier::ID_NAME, new_name);
    }
  }

  Template* NamedTemplate::extract_template()
  {
    Template * ret = temp;
    temp = 0;
    return ret;
  }

  void NamedTemplate::dump(unsigned level) const
  {
    name->dump(level);
    temp->dump(level);
  }

  // =================================
  // ===== NamedTemplates
  // =================================
  
  NamedTemplates::NamedTemplates(const NamedTemplates& p)
  : Node(p), checked(p.checked)
  {
    for (size_t i = 0; i < p.nts_v.size(); i++) {
      NamedTemplate* nt = p.nts_v[i]->clone();
      nts_v.add(nt);
      if (checked) {
        nts_m.add(p.nts_m.get_nth_key(i), nt);
      }
    }
  }

  NamedTemplates::~NamedTemplates()
  {
    for (size_t i = 0; i < nts_v.size(); i++) delete nts_v[i];
    nts_v.clear();
    nts_m.clear();
  }

  NamedTemplates *NamedTemplates::clone() const
  {
    return new NamedTemplates(*this);
  }

  void NamedTemplates::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for(size_t i = 0; i < nts_v.size(); i++) {
      NamedTemplate *nt = nts_v[i];
      nt->set_fullname(p_fullname + "." + nt->get_name().get_dispname());
    }
  }

  void NamedTemplates::set_my_scope(Scope *p_scope)
  {
    Node::set_my_scope(p_scope);
    for(size_t i = 0; i < nts_v.size(); i++) nts_v[i]->set_my_scope(p_scope);
  }

  void NamedTemplates::add_nt(NamedTemplate *p_nt)
  {
    if (!p_nt) FATAL_ERROR("NULL pointer:NamedTemplates::add_nt()");
    nts_v.add(p_nt);
    if (checked) {
      const string& name = p_nt->get_name().get_name();
      if (!nts_m.has_key(name)) nts_m.add(name, p_nt);
    }
  }

  bool NamedTemplates::has_nt_withName(const Identifier& p_name)
  {
    if (!checked) chk_dupl_id(false);
    return nts_m.has_key(p_name.get_name());
  }

  NamedTemplate *NamedTemplates::get_nt_byName(const Identifier& p_name)
  {
    if (!checked) chk_dupl_id(false);
    return nts_m[p_name.get_name()];
  }

  void NamedTemplates::chk_dupl_id(bool report_errors)
  {
    if (checked) nts_m.clear();
    for (size_t i = 0; i < nts_v.size(); i++) {
      NamedTemplate *nt = nts_v[i];
      const Identifier& id = nt->get_name();
      const string& name = id.get_name();
      if (!nts_m.has_key(name)) nts_m.add(name, nt);
      else if (report_errors) {
	const char *disp_name = id.get_dispname().c_str();
	nt->error("Duplicate field name `%s'", disp_name);
	nts_m[name]->note("Field `%s' is already given here", disp_name);
      }
    }
    checked = true;
  }

  // =================================
  // ===== LengthRestriction
  // =================================

  LengthRestriction::LengthRestriction(Value* p_val)
    : Node(), checked(false), is_range(false)
  {
    if (!p_val)
      FATAL_ERROR("LengthRestriction::LengthRestriction()");
    single = p_val;
  }

  LengthRestriction::LengthRestriction(Value* p_lower, Value* p_upper)
    : Node(), checked(false), is_range(true)
  {
    if (!p_lower)
      FATAL_ERROR("LengthRestriction::LengthRestriction()");
    range.lower = p_lower;
    range.upper = p_upper;
  }
  
  LengthRestriction::LengthRestriction(const LengthRestriction& p)
    : Node(), checked(p.checked), is_range(p.is_range)
  {
    if (is_range) {
      range.lower = p.range.lower->clone();
      range.upper = p.range.upper != NULL ? p.range.upper->clone() : NULL;
    }
    else {
      single = p.single->clone();
    }
  }

  LengthRestriction::~LengthRestriction()
  {
    if (is_range) {
      delete range.lower;
      delete range.upper;
    } else delete single;
  }

  LengthRestriction *LengthRestriction::clone() const
  {
    return new LengthRestriction(*this);
  }

  void LengthRestriction::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (is_range) {
      range.lower->set_fullname(p_fullname + ".<lower>");
      if (range.upper) range.upper->set_fullname(p_fullname + ".<upper>");
    } else single->set_fullname(p_fullname);
  }

  void LengthRestriction::set_my_scope(Scope *p_scope)
  {
    if (is_range) {
      range.lower->set_my_scope(p_scope);
      if (range.upper) range.upper->set_my_scope(p_scope);
    } else single->set_my_scope(p_scope);
  }

  void LengthRestriction::chk(Type::expected_value_t expected_value)
  {
    if (checked) return;
    checked = true;
    Type *pool_int = Type::get_pooltype(Type::T_INT);
    if (is_range) {
      range.lower->set_my_governor(pool_int);
      {
	Error_Context cntxt(range.lower, "In lower boundary of the length "
	  "restriction");
	pool_int->chk_this_value_ref(range.lower);
	pool_int->chk_this_value(range.lower, 0, expected_value,
	  INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
      }
      Value *v_lower = range.lower->get_value_refd_last();
      int_val_t v_int_lower_int;
      if (v_lower->get_valuetype() == Value::V_INT) {
        v_int_lower_int = *(v_lower->get_val_Int());
	if (v_int_lower_int < 0) {
	  range.lower->error("The lower boundary of the length restriction "
	    "must be a non-negative integer value instead of %s",
	    v_int_lower_int.t_str().c_str());
	  range.lower->set_valuetype(Value::V_ERROR);
	  v_int_lower_int = int_val_t((Int)0);
	}
      } else {
        v_int_lower_int = int_val_t((Int)0);
      }
      if (range.upper) {
	range.upper->set_my_governor(pool_int);
	{
	  Error_Context cntxt(range.lower, "In upper boundary of the length "
	    "restriction");
	  pool_int->chk_this_value_ref(range.upper);
	  pool_int->chk_this_value(range.upper, 0, expected_value,
	    INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
	}
	Value *v_upper = range.upper->get_value_refd_last();
	if (v_upper->get_valuetype() == Value::V_INT) {
	  int_val_t v_int_upper_int(*(v_upper->get_val_Int()));
	  if (v_int_upper_int < 0) {
	    range.upper->error("The upper boundary of the length "
	      "restriction must be a non-negative integer value instead "
	      "of %s", v_int_upper_int.t_str().c_str());
	    range.upper->set_valuetype(Value::V_ERROR);
	  } else if (v_int_upper_int < v_int_lower_int) {
	    error("The upper boundary of the length restriction (%s) "
	      "cannot be smaller than the lower boundary (%s)",
	      v_int_upper_int.t_str().c_str(),
	      v_int_lower_int.t_str().c_str());
	    range.upper->set_valuetype(Value::V_ERROR);
	  }
	}
      }
    } else {
      single->set_my_governor(pool_int);
      {
	Error_Context cntxt(single, "In length restriction");
	pool_int->chk_this_value_ref(single);
	pool_int->chk_this_value(single, 0, expected_value,
	  INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
      }
      Value *v_single = single->get_value_refd_last();
      if (v_single->get_valuetype() == Value::V_INT) {
        const int_val_t *v_int_int = v_single->get_val_Int();
	if (*v_int_int < 0) {
	  single->error("The length restriction must be a non-negative "
	    "integer value instead of %s", (v_int_int->t_str()).c_str());
	  single->set_valuetype(Value::V_ERROR);
	}
      }
    }
  }

  void LengthRestriction::chk_array_size(ArrayDimension *p_dim)
  {
    if (!checked) FATAL_ERROR("LengthRestriction::chk_array_size()");
    bool error_flag = false;
    if (!p_dim->get_has_error()) {
      size_t array_size = p_dim->get_size();
      if (is_range) {
	Value *v_lower_last = range.lower->get_value_refd_last();
	if (v_lower_last->get_valuetype() == Value::V_INT) {
	  const int_val_t *lower_len_int = v_lower_last->get_val_Int();
	  if (*lower_len_int > INT_MAX) {
	    range.lower->error("An integer value less than `%d' was expected "
	      "as the lower boundary of the length restriction instead of "
	      "`%s'", INT_MAX, (lower_len_int->t_str()).c_str());
	    error_flag = true;
	  } else {
	    Int lower_len = lower_len_int->get_val();
	    if (lower_len > static_cast<Int>(array_size)) {
	      range.lower->error("The number of elements allowed by the "
	        "length restriction (at least %s) contradicts the array size "
	        "(%lu)", Int2string(lower_len).c_str(),
	        (unsigned long)array_size);
	      error_flag = true;
	    }
	  }
	}
	if (range.upper) {
	  Value *v_upper_last = range.upper->get_value_refd_last();
	  if (v_upper_last->get_valuetype() == Value::V_INT) {
	    const int_val_t *upper_len_int = v_upper_last->get_val_Int();
	    if (*upper_len_int > INT_MAX) {
	      range.upper->error("An integer value less than `%d' was "
	        "expected as the upper boundary of the length restriction "
	        "instead of `%s'", INT_MAX, (upper_len_int->t_str()).c_str());
	      error_flag = true;
	    } else {
	      Int upper_len = upper_len_int->get_val();
	      if (upper_len < static_cast<Int>(array_size)) {
	        range.upper->error("The number of elements allowed by the "
	          "length restriction (at most %s) contradicts the array "
	          "size (%lu)",	Int2string(upper_len).c_str(),
                  (unsigned long)array_size);
	        error_flag = true;
	      }
	    }
	  }
	}
      } else {
	Value *v_last = single->get_value_refd_last();
	if (v_last->get_valuetype() == Value::V_INT) {
	  int_val_t single_len_int(*(v_last->get_val_Int()));
	  if (single_len_int != static_cast<Int>(array_size)) {
	    single->error("The number of elements allowed by the length "
	      "restriction (%s) contradicts the array size (%lu)",
	      single_len_int.t_str().c_str(), (unsigned long)array_size);
	    error_flag = true;
	  }
	}
      }
    }
    if (!error_flag)
      warning("Length restriction is useless for an array template");
  }

  void LengthRestriction::chk_nof_elements(size_t nof_elements,
    bool has_anyornone, const Location& p_loc, const char *p_what)
  {
    if (!checked) FATAL_ERROR("LengthRestriction::chk_nof_elements()");
    if (is_range) {
      if (!has_anyornone) {
	Value *v_lower_last = range.lower->get_value_refd_last();
	if (v_lower_last->get_valuetype() == Value::V_INT) {
	  const int_val_t *lower_len_int = v_lower_last->get_val_Int();
	  if (*lower_len_int > static_cast<Int>(nof_elements)) {
	    p_loc.error("There are fewer (%lu) elements in the %s than it "
	      "is allowed by the length restriction (at least %s)",
	      (unsigned long)nof_elements, p_what,
	      (lower_len_int->t_str()).c_str());
	  }
	}
      }
      if (range.upper) {
	Value *v_upper_last = range.upper->get_value_refd_last();
	if (v_upper_last->get_valuetype() == Value::V_INT) {
	  const int_val_t *upper_len_int = v_upper_last->get_val_Int();
	  if (*upper_len_int < static_cast<Int>(nof_elements)) {
	    p_loc.error("There are more (%s%lu) elements in the %s than it "
	      "is allowed by the length restriction (at most %s)",
	      has_anyornone ? "at least " : "", (unsigned long)nof_elements,
	      p_what, (upper_len_int->t_str()).c_str());
	  }
	}
      }
    } else {
      Value *v_last = single->get_value_refd_last();
      if (v_last->get_valuetype() == Value::V_INT) {
        // Cheaper than creating a local variable?
        const int_val_t *single_len_int = v_last->get_val_Int();
        if (*single_len_int < static_cast<Int>(nof_elements)) {
          p_loc.error("There are more (%s%lu) elements in the %s than it "
            "is allowed by the length restriction (%s)", has_anyornone ?
            "at least " : "", (unsigned long)nof_elements, p_what,
            (single_len_int->t_str()).c_str());
        } else if (*single_len_int > static_cast<Int>(nof_elements) &&
                   !has_anyornone) {
          p_loc.error("There are fewer (%lu) elements in the %s than it "
            "is allowed by the length restriction (%s)",
            (unsigned long)nof_elements, p_what,
            (single_len_int->t_str()).c_str());
        }
      }
    }
  }

  Value* LengthRestriction::get_single_value()
  {
    if (is_range) FATAL_ERROR("LengthRestriction::get_single_value()");
    //if (!checked) FATAL_ERROR("LengthRestriction::get_single_value()");
    return single->get_value_refd_last();
  }

  Value *LengthRestriction::get_lower_value()
  {
    if (!is_range) FATAL_ERROR("LengthRestriction::get_lower_value()");
    //if (!checked) FATAL_ERROR("LengthRestriction::get_lower_value()");
    return range.lower->get_value_refd_last();
  }

  Value *LengthRestriction::get_upper_value()
  {
    if (!is_range) FATAL_ERROR("LengthRestriction::get_upper_value()");
    //if (!checked) FATAL_ERROR("LengthRestriction::get_upper_value()");
    return range.upper ? range.upper->get_value_refd_last() : 0;
  }

  void LengthRestriction::set_code_section
    (GovernedSimple::code_section_t p_code_section)
  {
    if (is_range) {
      range.lower->set_code_section(p_code_section);
      if (range.upper) range.upper->set_code_section(p_code_section);
    } else single->set_code_section(p_code_section);
  }

  char *LengthRestriction::generate_code_init(char *str, const char *name)
  {
    expression_struct expr;
    Code::init_expr(&expr);
    char *init_stmt;
    if (is_range) {
      range.lower->generate_code_expr(&expr);
      init_stmt = mprintf("%s.set_min_length(%s);\n", name, expr.expr);
      if (range.upper) {
        Code::clean_expr(&expr);
	range.upper->generate_code_expr(&expr);
        init_stmt = mputprintf(init_stmt, "%s.set_max_length(%s);\n", name,
	  expr.expr);
      }
    } else {
      single->generate_code_expr(&expr);
      init_stmt = mprintf("%s.set_single_length(%s);\n", name, expr.expr);
    }
    if (expr.preamble || expr.postamble) {
      str = mputstr(str, "{\n");
      str = mputstr(str, expr.preamble);
      str = mputstr(str, init_stmt);
      str = mputstr(str, expr.postamble);
      str = mputstr(str, "}\n");
    } else str = mputstr(str, init_stmt);
    Code::free_expr(&expr);
    Free(init_stmt);
    return str;
  }

  char *LengthRestriction::rearrange_init_code(char *str, Common::Module* usage_mod)
  {
    if (is_range) {
      str = range.lower->rearrange_init_code(str, usage_mod);
      if (range.upper) str = range.upper->rearrange_init_code(str, usage_mod);
    } else str = single->rearrange_init_code(str, usage_mod);
    return str;
  }

  void LengthRestriction::append_stringRepr(string& str) const
  {
    str += "length(";
    if (is_range) {
      str += range.lower->get_stringRepr();
      str += " .. ";
      if (range.upper) str += range.upper->get_stringRepr();
      else str += "infinity";
    } else str += single->get_stringRepr();
    str += ")";
  }

  void LengthRestriction::dump(unsigned level) const
  {
    DEBUG(level, "length restriction:");
    if (is_range) {
      range.lower->dump(level + 1);
      DEBUG(level, "..");
      if (range.upper) range.upper->dump(level + 1);
      else DEBUG(level + 1, "infinity");
    } else single->dump(level + 1);
  }

  // =================================
  // ===== TemplateInstances
  // =================================

  TemplateInstances::TemplateInstances(const TemplateInstances& p)
   : Node(p), Location(p), tis()
  {
    for (size_t i = 0; i < p.tis.size(); i++)
      tis.add(p.tis[i]->clone());
  }

  TemplateInstances::~TemplateInstances()
  {
     for (size_t i = 0; i < tis.size(); i++) delete tis[i];
     tis.clear();
  }

  TemplateInstances *TemplateInstances::clone() const
  {
    return new TemplateInstances(*this);
  }

  void TemplateInstances::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for (size_t i = 0; i < tis.size(); i++)
      tis[i]->set_fullname(p_fullname + "[" + Int2string(i + 1) + "]");
  }

  void TemplateInstances::set_my_scope(Scope *p_scope)
  {
    for (size_t i = 0; i < tis.size(); i++)
    {
      TemplateInstance*& tir = tis[i];
      tir->set_my_scope(p_scope);
    }
  }

  void TemplateInstances::add_ti(TemplateInstance *p_ti)
  {
     if (!p_ti) FATAL_ERROR("NULL pointer:TemplateInstances::add_ti()");
     tis.add(p_ti);
  }

  void TemplateInstances::set_code_section
  (GovernedSimple::code_section_t p_code_section)
  {
    for (size_t i=0; i<tis.size(); i++)
      tis[i]->set_code_section(p_code_section);
  }

  void TemplateInstances::dump(unsigned level) const
  {
    DEBUG(level, "TemplateInstances %p, has %lu", (const void*)this,
      (unsigned long) tis.size());
    for (size_t i = 0; i < tis.size(); ++i) {
      tis[i]->dump(level+1);
    }
  }
  // =================================
  // ===== NamedParam
  // =================================

  NamedParam::NamedParam(Identifier *id, TemplateInstance *t)
  : Node(), Location(), name(id), tmpl(t)
  {
  }

  NamedParam::NamedParam(const NamedParam& p)
  : Node(p), Location(p), name(p.name->clone()), tmpl(p.tmpl->clone())
  {
  }

  NamedParam::~NamedParam()
  {
    delete tmpl;
    delete name;
  }

  NamedParam * NamedParam::clone() const {
    return new NamedParam(*this);
  }

  void NamedParam::set_my_scope(Scope *p_scope)
  {
    tmpl->set_my_scope(p_scope);
  }

  void NamedParam::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    tmpl->set_fullname(p_fullname);
  }

  TemplateInstance * NamedParam::extract_ti()
  {
    TemplateInstance *retval = tmpl;
    tmpl = 0;
    return retval;
  }

  void NamedParam::dump(unsigned int level) const
  {
    name->dump(level+1);
    tmpl->dump(level+1);
  }
  // =================================
  // ===== NamedParams
  // =================================

  NamedParams::NamedParams()
  : Node(), Location(), nps()
  {
  }

  NamedParams * NamedParams::clone() const
  {
    FATAL_ERROR("NamedParams::clone");
  }

  NamedParams::~NamedParams()
  {
    for (size_t i = 0; i < nps.size(); i++) delete nps[i];
    nps.clear();
  }

  void NamedParams::set_my_scope(Scope *p_scope)
  {
    for (size_t i = 0; i < nps.size(); i++) nps[i]->set_my_scope(p_scope);
  }

  void NamedParams::set_fullname(const string& p_fullname)
  {
    for (size_t i = 0; i < nps.size(); i++) nps[i]->set_fullname(p_fullname);
  }

  void NamedParams::add_np(NamedParam *p)
  {
    if (!p) FATAL_ERROR("NULL pointer: NamedParams::add_np()");
    nps.add(p);
  }

  size_t NamedParams::get_nof_nps() const
  {
    return nps.size();
  }

  NamedParam *NamedParams::extract_np_byIndex(size_t p_i)
  {
    NamedParam * retval = nps[p_i];
    nps[p_i] = 0;
    return retval;
  }

  void NamedParams::dump(unsigned int level) const
  {
    DEBUG(level, "NamedParams %p, has %lu", (const void*)this,
      (unsigned long) nps.size());
    for (unsigned int i = 0; i < nps.size(); ++i) {
      if (nps[i] != 0) {
	nps[i]->dump(level+1);
      } else {
	DEBUG(level+1, "NULL");
      }
    }
  }
  // =================================
  // ===== ParsedActualParameters
  // =================================

  ParsedActualParameters::ParsedActualParameters(TemplateInstances *p_ti,
                                                 NamedParams *p_np)
  : Node(), Location(), unnamedpart(p_ti ? p_ti : new TemplateInstances), namedpart(p_np)
  {
  }

  ParsedActualParameters::ParsedActualParameters(const ParsedActualParameters& p)
  : Node(p), Location(p), unnamedpart(p.unnamedpart->clone()),
    namedpart (p.namedpart ? p.namedpart->clone() : 0)
  {
  }

  ParsedActualParameters* ParsedActualParameters::clone() const
  {
    return new ParsedActualParameters(*this);
  }

  ParsedActualParameters::~ParsedActualParameters()
  {
    delete namedpart;
    delete unnamedpart;
  }

  void ParsedActualParameters::add_np(NamedParam *np)
  {
    if (!namedpart) // we got away without one until now
    {
      namedpart = new NamedParams;
    }
    namedpart->add_np(np);
  }

  TemplateInstances* ParsedActualParameters::steal_tis()
  {
    TemplateInstances* temp = unnamedpart;
    unnamedpart = new TemplateInstances;
    return temp;
  }

  size_t ParsedActualParameters::get_nof_nps() const
  {
    return namedpart ? namedpart->get_nof_nps() : 0;
  }

  void ParsedActualParameters::set_my_scope(Common::Scope *p_scope)
  {
    unnamedpart->set_my_scope(p_scope);
    if (namedpart) {
      namedpart->set_my_scope(p_scope);
    }
  }

  void ParsedActualParameters::set_fullname(const string& p_fullname)
  {
    unnamedpart->set_fullname(p_fullname);
    if (namedpart) {
      namedpart->set_fullname(p_fullname);
    }
  }

  void ParsedActualParameters::set_location(const char *p_filename, int p_lineno)
  {
    Location   ::set_location(p_filename, p_lineno);
    if (namedpart)
      namedpart->set_location(p_filename, p_lineno);
    unnamedpart->set_location(p_filename, p_lineno);
  }

  void ParsedActualParameters::set_location(const char *p_filename,
                                            const YYLTYPE& p_yyloc)
  {
    Location   ::set_location(p_filename, p_yyloc);
    if (namedpart)
      namedpart->set_location(p_filename, p_yyloc);
    unnamedpart->set_location(p_filename, p_yyloc);
  }

  void ParsedActualParameters::set_location(const char *p_filename,
                                            const YYLTYPE& p_firstloc,
                                            const YYLTYPE & p_lastloc)
  {
    Location   ::set_location(p_filename, p_firstloc, p_lastloc);
    if (namedpart)
      namedpart->set_location(p_filename, p_firstloc, p_lastloc);
    unnamedpart->set_location(p_filename, p_firstloc, p_lastloc);
  }

  void ParsedActualParameters::set_location(const char *p_filename, int p_first_line,
    int p_first_column, int p_last_line, int p_last_column)
  {
    Location   ::set_location(p_filename, p_first_line, p_first_column,
                                          p_last_line, p_last_column);
    if (namedpart)
      namedpart->set_location(p_filename, p_first_line, p_first_column,
                                          p_last_line, p_last_column);
    unnamedpart->set_location(p_filename, p_first_line, p_first_column,
                                          p_last_line, p_last_column);
  }


  void ParsedActualParameters::dump(unsigned int level) const
  {
    DEBUG(level, "ParsedActualParameters at %p", (const void*)this);
    unnamedpart->dump(level+1);
    if (namedpart) {
      namedpart->dump(level+1);
    }
  }
}
