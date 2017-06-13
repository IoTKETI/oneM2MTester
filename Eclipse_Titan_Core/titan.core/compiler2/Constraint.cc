/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Cserveni, Akos
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "Constraint.hh"
#include "CompilerError.hh"
#include "Int.hh"
#include "Type.hh"
#include "Value.hh"
#include "subtype.hh"
#include "Identifier.hh"
#include "CompField.hh"
#include "asn1/Block.hh"
#include "asn1/TokenBuf.hh"

namespace Common {

  // =================================
  // ===== Constraints
  // =================================

  Constraints::Constraints(const Constraints& p)
    : Node(p), cons(), my_type(0), subtype(0), extendable(false), extension(0)
  {
    size_t nof_cons = p.cons.size();
    for (size_t i = 0; i < nof_cons; i++) cons.add(p.cons[i]->clone());
  }

  Constraints::~Constraints()
  {
    size_t nof_cons = cons.size();
    for (size_t i = 0; i < nof_cons; i++) delete cons[i];
    cons.clear();
    delete subtype;
    delete extension;
  }

  Constraints *Constraints::clone() const
  {
    return new Constraints(*this);
  }

  void Constraints::add_con(Constraint *p_con)
  {
    if (!p_con) FATAL_ERROR("Constraints::add_con()");
    cons.add(p_con);
    if (my_type) p_con->set_my_type(my_type);
  }

  Constraint* Constraints::get_tableconstraint() const
  {
    size_t nof_cons = cons.size();
    for (size_t i = 0; i < nof_cons; i++) {
      Constraint *con = cons[i];
      if (con->get_constrtype() == Constraint::CT_TABLE) return con;
    }
    return 0;
  }

  void Constraints::set_my_type(Type *p_my_type)
  {
    my_type=p_my_type;
    size_t nof_cons = cons.size();
    for (size_t i = 0; i < nof_cons; i++) cons[i]->set_my_type(p_my_type);
  }

  void Constraints::chk_table()
  {
    if(!my_type) FATAL_ERROR("Constraints::chk_table()");
    size_t nof_cons = cons.size();
    for (size_t i = 0; i < nof_cons; i++) {
      Error_Context cntxt(my_type, "In constraint #%lu of type `%s'",
       (unsigned long) (i + 1), my_type->get_typename().c_str());
      Constraint::constrtype_t cst = cons[i]->get_constrtype();
      if (cst==Constraint::CT_TABLE) cons[i]->chk();
    }
  }

  void Constraints::chk(SubtypeConstraint* parent_subtype)
  {
    if(!my_type) FATAL_ERROR("Common::Constraints::chk()");
    if (parent_subtype) {
      subtype = new SubtypeConstraint(my_type->get_subtype_type());
      subtype->copy(parent_subtype);
    }
    size_t nof_cons = cons.size();
    for (size_t i = 0; i < nof_cons; i++) {
      Error_Context cntxt(my_type, "In constraint #%lu of type `%s'",
        (unsigned long) (i + 1), my_type->get_typename().c_str());
      cons[i]->set_fullname(my_type->get_fullname()+".<constraint_"+Int2string(i+1)+">.<"+string(cons[i]->get_name())+">");
      cons[i]->set_my_cons(this);
      Constraint::constrtype_t cst = cons[i]->get_constrtype();
      if (cst!=Constraint::CT_TABLE) cons[i]->chk();
      if ( (cst==Constraint::CT_IGNORE) || (cst==Constraint::CT_TABLE) ) continue; // ignore
      SubtypeConstraint* sc = cons[i]->get_subtype();
      SubtypeConstraint* sc_ext = cons[i]->get_extension();
      if (!sc) break; // stop on error
      extendable = cons[i]->is_extendable();
      if (extension) { // only the root part shall be kept
        delete extension;
        extension = 0;
      }
      if (subtype) {
        if (sc->is_subset(subtype)==TFALSE) {
          cons[i]->error("Constraint #%lu is %s, this is not a subset of %s",
            (unsigned long) (i + 1),
            sc->to_string().c_str(),
            subtype->to_string().c_str());
          break; // stop on error
        }
        if (sc_ext && (sc_ext->is_subset(subtype)==TFALSE)) {
          cons[i]->error("Extension addition of constraint #%lu is %s, this is not a subset of %s",
            (unsigned long) (i + 1),
            sc_ext->to_string().c_str(),
            subtype->to_string().c_str());
          break; // stop on error
        }
        if (sc_ext) {
          extension = new SubtypeConstraint(my_type->get_subtype_type());
          extension->copy(sc_ext);
          extension->intersection(subtype);
        }
        subtype->intersection(sc);
      } else {
        subtype = new SubtypeConstraint(my_type->get_subtype_type());
        subtype->copy(sc);
        if (sc_ext) {
          extension = new SubtypeConstraint(my_type->get_subtype_type());
          extension->copy(sc_ext);
        }
      }
    }
  }

  // =================================
  // ===== Constraint
  // =================================

  Constraint::Constraint(const Constraint& p):
    Node(p), Location(p), constrtype(p.constrtype),
    my_type(0), my_scope(0), my_parent(0), my_cons(0),
    checked(false), subtype(0), extendable(false), extension(0)
  {
  }

  Constraint::Constraint(constrtype_t p_constrtype):
    Node(), Location(), constrtype(p_constrtype),
    my_type(0), my_scope(0), my_parent(0), my_cons(0),
    checked(false), subtype(0), extendable(false), extension(0)
  {
  }

  Constraint::~Constraint()
  {
    delete subtype;
    delete extension;
  }

  Scope* Constraint::get_my_scope()
  {
    if (!my_type) FATAL_ERROR("Constraint::get_my_scope()");
    return my_scope ? my_scope : my_type->get_my_scope();
  }

  void Constraint::set_my_cons(Constraints* p_my_cons)
  {
    my_cons = p_my_cons;
  }

  Constraints* Constraint::get_my_cons()
  {
    Constraint* c = this;
    while (c) {
      if (c->my_cons) return c->my_cons;
      c = c->my_parent;
    }
    FATAL_ERROR("Constraint::get_my_cons()");
    return NULL;
  }

  bool Constraint::in_char_context() const
  {
    Constraint* parent = my_parent;
    while (parent) {
      if (parent->get_constrtype()==CT_PERMITTEDALPHABET) return true;
      parent = parent->get_my_parent();
    }
    return false;
  }

  bool Constraint::in_inner_constraint() const
  {
    Constraint* parent = my_parent;
    while (parent) {
      switch (parent->get_constrtype()) {
      case CT_SINGLEINNERTYPE:
      case CT_MULTIPLEINNERTYPE:
        return true;
      default:
        break;
      }
      parent = parent->get_my_parent();
    }
    return false;
  }

  // =================================
  // ===== ElementSetSpecsConstraint
  // =================================

  ElementSetSpecsConstraint::ElementSetSpecsConstraint(const ElementSetSpecsConstraint& p)
    : Constraint(p)
  {
    extendable = true;
    root_constr = p.root_constr ? p.root_constr->clone() : 0;
    ext_constr = p.ext_constr ? p.ext_constr->clone() : 0;
  }

  ElementSetSpecsConstraint::ElementSetSpecsConstraint(Constraint* p_root_constr, Constraint* p_ext_constr)
    : Constraint(CT_ELEMENTSETSPEC), root_constr(p_root_constr), ext_constr(p_ext_constr)
  {
    if (!p_root_constr) FATAL_ERROR("ElementSetSpecsConstraint::ElementSetSpecsConstraint()");
    // p_ext_constr can be NULL
    extendable = true;
  }

  ElementSetSpecsConstraint::~ElementSetSpecsConstraint()
  {
    delete root_constr;
    delete ext_constr;
  }

  void ElementSetSpecsConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!root_constr || !my_type) FATAL_ERROR("ElementSetSpecsConstraint::chk()");
    Error_Context cntxt(this, "While checking ElementSetSpecs constraint");
    root_constr->set_my_type(my_type);
    if (ext_constr) ext_constr->set_my_type(my_type);
    root_constr->set_my_scope(get_my_scope());
    if (ext_constr) ext_constr->set_my_scope(get_my_scope());
    root_constr->set_my_parent(this);
    if (ext_constr) ext_constr->set_my_parent(this);
    root_constr->chk();
    if (ext_constr) ext_constr->chk();
    SubtypeConstraint::subtype_t my_subtype_type = my_type->get_subtype_type();
    // naming ((r1,x1),...,(r2,x2)) according to X.680(11/2008) I.4.3.8
    SubtypeConstraint* r1 = root_constr->get_subtype();
    if (!r1) return; // root_constr is invalid, error already reported there
    SubtypeConstraint* x1 = root_constr->get_extension();
    if (!ext_constr) {
      // only root part exists
      subtype = new SubtypeConstraint(my_subtype_type);
      subtype->copy(r1);
      if (x1) {
        extension = new SubtypeConstraint(my_subtype_type);
        extension->copy(x1);
      }
      return;
    }
    SubtypeConstraint* r2 = ext_constr->get_subtype();
    if (!r2) return; // ext_constr is invalid, error already reported there
    SubtypeConstraint* x2 = ext_constr->get_extension();
    subtype = new SubtypeConstraint(my_subtype_type);
    subtype->copy(r1);
    extension = new SubtypeConstraint(my_subtype_type);
    extension->copy(r2);
    if (x1) extension->union_(x1);
    if (x2) extension->union_(x2);
    extension->except(r1);
  }

  void ElementSetSpecsConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    root_constr->set_fullname(p_fullname+".<root_part>.<"+string(root_constr->get_name())+">");
    if (ext_constr)
      ext_constr->set_fullname(p_fullname+".<extension_part>.<"+string(ext_constr->get_name())+">");
  }

  // =================================
  // ===== IgnoredConstraint
  // =================================

  IgnoredConstraint::IgnoredConstraint(const IgnoredConstraint& p)
    : Constraint(p), my_name(p.my_name)
  {
  }

  IgnoredConstraint::IgnoredConstraint(const char* p_name)
    : Constraint(CT_IGNORE), my_name(p_name)
  {
    if (p_name==NULL) FATAL_ERROR("IgnoredConstraint::IgnoredConstraint()");
  }

  void IgnoredConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!my_type) FATAL_ERROR("IgnoredConstraint::chk()");
    if (in_char_context()) {
      error("%s not allowed inside a permitted alphabet constraint", get_name());
      return;
    } else {
      if (my_parent) warning("%s inside a %s is treated as `ALL' (constraint is dropped)", get_name(), my_parent->get_name());
      //else warning("%s is ignored", get_name());
    }
    // ignored constraint does not constrain the type, set to full set
    subtype = new SubtypeConstraint(my_type->get_subtype_type());
  }

  // =================================
  // ===== SingleValueConstraint
  // =================================

  SingleValueConstraint::SingleValueConstraint(const SingleValueConstraint& p)
    : Constraint(p)
  {
    value = p.value ? p.value->clone() : 0;
  }

  SingleValueConstraint::~SingleValueConstraint()
  {
    delete value;
  }

  SingleValueConstraint::SingleValueConstraint(Value* p_value)
    : Constraint(CT_SINGLEVALUE), value(p_value)
  {
    if (!p_value) FATAL_ERROR("SingleValueConstraint::SingleValueConstraint()");
  }

  void SingleValueConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!value || !my_type) FATAL_ERROR("SingleValueConstraint::chk()");
    Error_Context cntxt(this, "While checking single value constraint");
    value->set_my_scope(get_my_scope());
    value->set_my_governor(my_type);
    my_type->chk_this_value_ref(value);
    my_type->chk_this_value(value, 0, Type::EXPECTED_CONSTANT,
      INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, NO_SUB_CHK);
    if (in_char_context()) {
      subtype = SubtypeConstraint::create_from_asn_charvalues(my_type, value);
    } else {
      subtype = SubtypeConstraint::create_from_asn_value(my_type, value);
    }
  }

  void SingleValueConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    value->set_fullname(p_fullname+".<value>");
  }

  // =================================
  // ===== ContainedSubtypeConstraint
  // =================================

  ContainedSubtypeConstraint::ContainedSubtypeConstraint(const ContainedSubtypeConstraint& p)
    : Constraint(p)
  {
    type = p.type ? p.type->clone() : 0;
  }

  ContainedSubtypeConstraint::~ContainedSubtypeConstraint()
  {
    delete type;
  }

  ContainedSubtypeConstraint::ContainedSubtypeConstraint(Type* p_type, bool p_has_includes)
    : Constraint(CT_CONTAINEDSUBTYPE), type(p_type), has_includes(p_has_includes)
  {
    if (!p_type) FATAL_ERROR("ContainedSubtypeConstraint::ContainedSubtypeConstraint()");
  }

  void ContainedSubtypeConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!type || !my_type) FATAL_ERROR("ContainedSubtypeConstraint::chk()");
    Error_Context cntxt(this, "While checking contained subtype constraint");

    type->set_my_scope(get_my_scope());
    type->chk();
    if (type->get_typetype()==Type::T_ERROR) return;

    if (my_type->get_type_refd_last()->get_typetype()==Type::T_OPENTYPE) {
      // TODO: open type and anytype should have their own ST_ANY subtype, now this is only ignored
      subtype = new SubtypeConstraint(my_type->get_subtype_type());
      return;
    }

    if (!type->is_identical(my_type)) {
      error("Contained subtype constraint is invalid, constrained and constraining types have different root type");
      return;
    }
    // check subtype of referenced type
    SubType* t_st = type->get_sub_type();
    if (t_st==NULL) {
      error("Contained subtype constraint is invalid, the constraining type has no subtype constraint");
      return;
    }

    // check circular subtype reference
    Type* my_owner = get_my_cons()->get_my_type();
    if (!my_owner || !my_owner->get_sub_type()) FATAL_ERROR("ContainedSubtypeConstraint::chk()");
    if (!my_owner->get_sub_type()->add_parent_subtype(t_st)) return;

    if (t_st->get_subtypetype()==SubtypeConstraint::ST_ERROR) return;
    if (t_st->get_subtypetype()!=my_type->get_subtype_type()) FATAL_ERROR("ContainedSubtypeConstraint::chk()");

    subtype = SubtypeConstraint::create_from_contained_subtype(t_st->get_root(), in_char_context(), this);
  }

  void ContainedSubtypeConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    type->set_fullname(p_fullname+".<type>");
  }

  // =================================
  // ===== RangeEndpoint
  // =================================

  RangeEndpoint::RangeEndpoint(const RangeEndpoint& p):
    Node(p), Location(p), type(p.type), inclusive(p.inclusive)
  {
    value = p.value ? p.value->clone() : 0;
  }

  RangeEndpoint::~RangeEndpoint()
  {
    delete value;
  }

  RangeEndpoint::RangeEndpoint(endpoint_t p_type):
    Node(), Location(), type(p_type), value(0), inclusive(true)
  {
    if (type==VALUE) FATAL_ERROR("RangeEndpoint::RangeEndpoint()");
  }

  RangeEndpoint::RangeEndpoint(Value* p_value):
    Node(), Location(), type(RangeEndpoint::VALUE), value(p_value), inclusive(true)
  {
    if (!p_value) FATAL_ERROR("RangeEndpoint::RangeEndpoint()");
  }

  void RangeEndpoint::chk(Type* my_type, ValueRangeConstraint* constraint)
  {
    if (value) {
      value->set_my_scope(constraint->get_my_scope());
      my_type->chk_this_value_ref(value);
      my_type->chk_this_value(value, 0, Type::EXPECTED_CONSTANT,
        INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, NO_SUB_CHK);
    }
  }

  void RangeEndpoint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (value) value->set_fullname(p_fullname+".<value>");
  }

  // =================================
  // ===== ValueRangeConstraint
  // =================================

  ValueRangeConstraint::ValueRangeConstraint(const ValueRangeConstraint& p)
    : Constraint(p)
  {
    lower_endpoint = p.lower_endpoint ? p.lower_endpoint->clone() : 0;
    upper_endpoint = p.upper_endpoint ? p.upper_endpoint->clone() : 0;
  }

  ValueRangeConstraint::ValueRangeConstraint(RangeEndpoint* p_lower, RangeEndpoint* p_upper)
    : Constraint(CT_VALUERANGE), lower_endpoint(p_lower), upper_endpoint(p_upper)
  {
    if (!p_lower || !p_upper) FATAL_ERROR("ValueRangeConstraint::ValueRangeConstraint()");
  }

  void ValueRangeConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!lower_endpoint || !upper_endpoint || !my_type) FATAL_ERROR("ValueRangeConstraint::chk()");
    Error_Context cntxt(this, "While checking value range constraint");
    SubtypeConstraint::subtype_t my_subtype_type = my_type->get_subtype_type();
    switch (my_subtype_type) {
    case SubtypeConstraint::ST_INTEGER:
    case SubtypeConstraint::ST_FLOAT:
      break;
    case SubtypeConstraint::ST_CHARSTRING:
      switch (my_type->get_type_refd_last()->get_typetype()) {
      case Type::T_NUMERICSTRING:
      case Type::T_PRINTABLESTRING:
      case Type::T_IA5STRING:
      case Type::T_VISIBLESTRING:
        if (!in_char_context()) {
          error("Value range constraint must be inside a permitted alphabet or size constraint");
          return;
        }
        break;
      default:
        error("Value range constraint is not allowed for type `%s'", my_type->get_typename().c_str());
        return;
      }
      break;
    case SubtypeConstraint::ST_UNIVERSAL_CHARSTRING:
      switch (my_type->get_type_refd_last()->get_typetype()) {
      case Type::T_UTF8STRING:
      case Type::T_UNIVERSALSTRING:
      case Type::T_BMPSTRING:
        if (!in_char_context()) {
          error("Value range constraint must be inside a permitted alphabet or size constraint");
          return;
        }
        break;
      default:
        error("Value range constraint is not allowed for type `%s'", my_type->get_typename().c_str());
        return;
      }
      break;
    default:
      error("Value range constraint is not allowed for type `%s'", my_type->get_typename().c_str());
      return;
    }
    lower_endpoint->chk(my_type, this);
    upper_endpoint->chk(my_type, this);
    Value* vmin = lower_endpoint->get_value();
    if (vmin) vmin = vmin->get_value_refd_last();
    Value* vmax = upper_endpoint->get_value();
    if (vmax) vmax = vmax->get_value_refd_last();
    // the subtype of the Constraints object at the time of calling this chk()
    // function is constructed from all the previously calculated constraints
    // (it is not yet the final subtype). This is needed to calculate the
    // current MIN and MAX values.
    SubtypeConstraint* parent_subtype = get_my_cons()->get_subtype();
    subtype = SubtypeConstraint::create_from_asn_range(
      vmin, lower_endpoint->get_exclusive(),
      vmax, upper_endpoint->get_exclusive(),
      this, my_subtype_type,
      in_inner_constraint() ? NULL /*the parent is invalid for an inner field*/ : parent_subtype);
  }

  void ValueRangeConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    lower_endpoint->set_fullname(p_fullname+".<lower_endpoint>");
    upper_endpoint->set_fullname(p_fullname+".<upper_endpoint>");
  }

  // =================================
  // ===== SizeConstraint
  // =================================

  SizeConstraint::SizeConstraint(const SizeConstraint& p)
    : Constraint(p)
  {
    constraint = p.constraint ? p.constraint->clone() : 0;
  }

  SizeConstraint::SizeConstraint(Constraint* p)
    : Constraint(CT_SIZE), constraint(p)
  {
    if (!p) FATAL_ERROR("SizeConstraint::SizeConstraint()");
  }

  void SizeConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!constraint || !my_type) FATAL_ERROR("SizeConstraint::chk()");
    Error_Context cntxt(this, "While checking size constraint");
    constraint->set_my_type(Type::get_pooltype(Type::T_INT_A));
    constraint->set_my_scope(get_my_scope());
    constraint->set_my_parent(this);
    constraint->chk();
    subtype = SubtypeConstraint::create_asn_size_constraint(
      constraint->get_subtype(), in_char_context(), my_type, this);
    extendable = constraint->is_extendable();
    if (constraint->get_extension()) {
      extension = SubtypeConstraint::create_asn_size_constraint(
        constraint->get_extension(), in_char_context(), my_type, this);
    }
  }

  void SizeConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    constraint->set_fullname(p_fullname+".<"+string(constraint->get_name())+">");
  }

  // =================================
  // ===== PermittedAlphabetConstraint
  // =================================

  PermittedAlphabetConstraint::PermittedAlphabetConstraint(const PermittedAlphabetConstraint& p)
    : Constraint(p)
  {
    constraint = p.constraint ? p.constraint->clone() : 0;
  }

  PermittedAlphabetConstraint::PermittedAlphabetConstraint(Constraint* p)
    : Constraint(CT_PERMITTEDALPHABET), constraint(p)
  {
    if (!p) FATAL_ERROR("PermittedAlphabetConstraint::PermittedAlphabetConstraint()");
  }

  void PermittedAlphabetConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!constraint || !my_type) FATAL_ERROR("PermittedAlphabetConstraint::chk()");
    Error_Context cntxt(this, "While checking permitted alphabet constraint");
    constraint->set_my_type(my_type);
    constraint->set_my_scope(get_my_scope());
    constraint->set_my_parent(this);
    constraint->chk();
    subtype = SubtypeConstraint::create_permitted_alphabet_constraint(
      constraint->get_subtype(), in_char_context(), my_type, this);
    extendable = constraint->is_extendable();
    if (constraint->get_extension()) {
      extension = SubtypeConstraint::create_permitted_alphabet_constraint(
        constraint->get_extension(), in_char_context(), my_type, this);
    }
  }

  void PermittedAlphabetConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    constraint->set_fullname(p_fullname+".<"+string(constraint->get_name())+">");
  }

  // =================================
  // ===== SetOperationConstraint
  // =================================

  SetOperationConstraint::SetOperationConstraint(const SetOperationConstraint& p)
    : Constraint(p), operationtype(p.operationtype)
  {
    operand_a = p.operand_a ? p.operand_a->clone() : 0;
    operand_b = p.operand_b ? p.operand_b->clone() : 0;
  }

  void SetOperationConstraint::set_first_operand(Constraint* p_a)
  {
    if (operand_a || !p_a) FATAL_ERROR("SetOperationConstraint::set_first_operand()");
    operand_a = p_a;
  }

  SetOperationConstraint::SetOperationConstraint(Constraint* p_a, operationtype_t p_optype, Constraint* p_b)
    : Constraint(CT_SETOPERATION), operationtype(p_optype), operand_a(p_a), operand_b(p_b)
  {
    // p_a can be null
    if (!p_b) FATAL_ERROR("SetOperationConstraint::SetOperationConstraint()");
  }

  const char* SetOperationConstraint::get_operationtype_str() const
  {
    switch (operationtype) {
    case UNION:
      return "union";
    case INTERSECTION:
      return "intersection";
    case EXCEPT:
      return "except";
    }
    return "<invalid>";
  }

  void SetOperationConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!operand_a || !operand_b || !my_type) FATAL_ERROR("SetOperationConstraint::chk()");
    Error_Context cntxt(this, "While checking %s operation", get_operationtype_str());
    operand_a->set_my_type(my_type);
    operand_b->set_my_type(my_type);
    operand_a->set_my_scope(get_my_scope());
    operand_b->set_my_scope(get_my_scope());
    operand_a->set_my_parent(this);
    operand_b->set_my_parent(this);
    operand_a->chk();
    operand_b->chk();

    extendable = (operationtype!=EXCEPT) ?
      (operand_a->is_extendable() || operand_b->is_extendable()) :
      operand_a->is_extendable();
    SubtypeConstraint::subtype_t my_subtype_type = my_type->get_subtype_type();
    // naming ((r1,x1),...,(r2,x2)) according to X.680(11/2008) I.4.3.8
    SubtypeConstraint* r1 = operand_a->get_subtype();
    SubtypeConstraint* x1 = operand_a->get_extension();
    SubtypeConstraint* r2 = operand_b->get_subtype();
    SubtypeConstraint* x2 = operand_b->get_extension();
    if (!r1 || !r2) return; // something invalid, error already reported there
    subtype = new SubtypeConstraint(my_subtype_type);
    subtype->copy(r1);
    switch (operationtype) {
    case UNION:
      subtype->union_(r2);
      if (x1 || x2) {
        extension = new SubtypeConstraint(my_subtype_type);
        if (x1 && x2) {
          extension->copy(subtype);
          extension->union_(x1);
          extension->union_(x2);
          extension->except(subtype);
        } else {
          extension->copy(x1?x1:x2);
        }
      }
      break;
    case INTERSECTION:
      subtype->intersection(r2);
      if (x1 || x2) {
        extension = new SubtypeConstraint(my_subtype_type);
        if (x1 && x2) {
          extension->copy(r1);
          extension->union_(x1);
          SubtypeConstraint* ext_tmp = new SubtypeConstraint(my_subtype_type);
          ext_tmp->copy(r2);
          ext_tmp->union_(x2);
          extension->intersection(ext_tmp);
          delete ext_tmp;
          extension->except(subtype);
        } else {
          extension->copy(r1);
          extension->intersection(x1?x1:x2);
        }
      }
      break;
    case EXCEPT:
      subtype->except(r2);
      if (x1) {
        extension = new SubtypeConstraint(my_subtype_type);
        if (x2) {
          extension->copy(x1);
          SubtypeConstraint* ext_tmp = new SubtypeConstraint(my_subtype_type);
          ext_tmp->copy(r2);
          ext_tmp->union_(x2);
          extension->except(ext_tmp);
          delete ext_tmp;
          extension->except(subtype);
        } else {
          extension->copy(x1);
          extension->except(r2);
          extension->except(subtype);
        }
      }
      break;
    default:
      FATAL_ERROR("SetOperationConstraint::chk()");
    }
  }

  void SetOperationConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    operand_a->set_fullname(p_fullname+".<first_operand>.<"+string(operand_a->get_name())+">");
    operand_b->set_fullname(p_fullname+".<second_operand>.<"+string(operand_b->get_name())+">");
  }

  // =================================
  // ===== FullSetConstraint
  // =================================

  void FullSetConstraint::chk()
  {
    SubtypeConstraint::subtype_t my_subtype_type = my_type->get_subtype_type();
    subtype = new SubtypeConstraint(my_subtype_type);
  }

  // =================================
  // ===== PatternConstraint
  // =================================

  PatternConstraint::PatternConstraint(const PatternConstraint& p)
    : Constraint(p)
  {
    value = p.value ? p.value->clone() : 0;
  }

  PatternConstraint::~PatternConstraint()
  {
    delete value;
  }

  PatternConstraint::PatternConstraint(Value* p_value)
    : Constraint(CT_PATTERN), value(p_value)
  {
    if (!p_value) FATAL_ERROR("PatternConstraint::PatternConstraint()");
  }

  void PatternConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!value || !my_type) FATAL_ERROR("PatternConstraint::chk()");
    Error_Context cntxt(this, "While checking pattern constraint");
    switch (my_type->get_subtype_type()) {
    case SubtypeConstraint::ST_CHARSTRING:
    case SubtypeConstraint::ST_UNIVERSAL_CHARSTRING:
      break;
    default:
      error("Pattern constraint is not allowed for type `%s'", my_type->get_typename().c_str());
      return;
    }
    value->set_my_scope(get_my_scope());
    value->set_my_governor(my_type);
    my_type->chk_this_value_ref(value);
    my_type->chk_this_value(value, 0, Type::EXPECTED_CONSTANT,
      INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, NO_SUB_CHK);
    // ignore ASN.1 pattern constraint (9.1 Transformation rules, point 4.)
    if (in_char_context()) {
      error("%s not allowed inside a permitted alphabet constraint", get_name());
      return;
    } else {
      if (my_parent) warning("%s inside a %s is treated as `ALL' (constraint is dropped)",
                             get_name(), my_parent->get_name());
      else warning("%s is ignored", get_name());
    }
    subtype = new SubtypeConstraint(my_type->get_subtype_type());
  }

  void PatternConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    value->set_fullname(p_fullname+".<value>");
  }

  // =================================
  // ===== SingleInnerTypeConstraint
  // =================================

  SingleInnerTypeConstraint::SingleInnerTypeConstraint(const SingleInnerTypeConstraint& p)
    : Constraint(p)
  {
    constraint = p.constraint ? p.constraint->clone() : 0;
  }

  SingleInnerTypeConstraint::SingleInnerTypeConstraint(Constraint* p)
    : Constraint(CT_SINGLEINNERTYPE), constraint(p)
  {
    if (!p) FATAL_ERROR("SingleInnerTypeConstraint::SingleInnerTypeConstraint()");
  }

  void SingleInnerTypeConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!constraint || !my_type) FATAL_ERROR("SingleInnerTypeConstraint::chk()");
    Error_Context cntxt(this, "While checking inner type constraint");
    Type* t = my_type->get_type_refd_last();
    if (!t->is_seof()) {
      error("Single inner type constraint (WITH COMPONENT) cannot be used on type `%s'", my_type->get_typename().c_str());
      return;
    }
    Type* field_type = t->get_ofType(); // determine the type of the field to which it refers
    constraint->set_my_type(field_type);
    constraint->set_my_scope(get_my_scope());
    constraint->set_my_parent(this);
    constraint->chk();
    //if (constraint->get_subtype()) { // if the constraint was not erroneous
    // TODO: this could be added to the a tree structure constraint on my_type,
    //       tree structure needed because of set operations, constraint cannot
    //       be added to field_type
    //}
    // inner subtype is ignored ( ETSI ES 201 873-7 V4.1.2 -> 9.1 Transformation rules )
    subtype = new SubtypeConstraint(my_type->get_subtype_type());
  }

  void SingleInnerTypeConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    constraint->set_fullname(p_fullname+".<"+string(constraint->get_name())+">");
  }

  // =================================
  // ===== NamedConstraint
  // =================================

  NamedConstraint::NamedConstraint(Identifier* p_id, Constraint* p_value_constraint, presence_constraint_t p_presence_constraint):
    Constraint(CT_NAMED), id(p_id), value_constraint(p_value_constraint), presence_constraint(p_presence_constraint)
  {
    if (!id) FATAL_ERROR("NamedConstraint::NamedConstraint()");
  }

  NamedConstraint::NamedConstraint(const NamedConstraint& p)
    : Constraint(p)
  {
    id = p.id->clone();
    value_constraint = p.value_constraint ? p.value_constraint->clone() : 0;
    presence_constraint = p.presence_constraint;
  }

  NamedConstraint::~NamedConstraint()
  {
    delete id;
    delete value_constraint;
  }

  const char* NamedConstraint::get_presence_constraint_name() const
  {
    switch (presence_constraint) {
    case PC_NONE: return "";
    case PC_PRESENT: return "PRESENT";
    case PC_ABSENT: return "ABSENT";
    case PC_OPTIONAL: return "OPTIONAL";
    default: FATAL_ERROR("NamedConstraint::get_presence_constraint_name()");
    }
  }

  void NamedConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!my_type) FATAL_ERROR("NamedConstraint::chk()");
    Error_Context cntxt(this, "While checking named constraint");
    if (value_constraint) {
      value_constraint->set_my_type(my_type);
      value_constraint->set_my_scope(get_my_scope());
      value_constraint->set_my_parent(this);
      value_constraint->chk();
    }
    subtype = new SubtypeConstraint(my_type->get_subtype_type()); // ignored
  }

  void NamedConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (value_constraint) {
      value_constraint->set_fullname(p_fullname+".<"+string(value_constraint->get_name())+">");
    }
  }

  // =================================
  // ===== MultipleInnerTypeConstraint
  // =================================

  MultipleInnerTypeConstraint::MultipleInnerTypeConstraint(const MultipleInnerTypeConstraint& p)
    : Constraint(p)
  {
    partial = p.partial;
    for (size_t i=0; i<p.named_con_vec.size(); i++) {
      named_con_vec.add( p.named_con_vec[i]->clone() );
    }
  }

  MultipleInnerTypeConstraint::~MultipleInnerTypeConstraint()
  {
    named_con_map.clear();
    for (size_t i=0; i<named_con_vec.size(); i++) {
      delete named_con_vec[i];
    }
    named_con_vec.clear();
  }

  void MultipleInnerTypeConstraint::addNamedConstraint(NamedConstraint* named_con)
  {
    if (!named_con) FATAL_ERROR("MultipleInnerTypeConstraint::addNamedConstraint()");
    if (checked) FATAL_ERROR("MultipleInnerTypeConstraint::addNamedConstraint()");
    named_con_vec.add(named_con);
  }

  void MultipleInnerTypeConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!my_type) FATAL_ERROR("MultipleInnerTypeConstraint::chk()");
    Type* t = my_type->get_type_refd_last();

    switch (t->get_typetype()) {
    case Type::T_REAL: { // associated ASN.1 type is a SEQUENCE
      Identifier t_id(Identifier::ID_ASN, string("REAL"));
      t = t->get_my_scope()->get_scope_asss()->get_local_ass_byId(t_id)->get_Type();
      if (t->get_typetype()!=Type::T_SEQ_A) FATAL_ERROR("MultipleInnerTypeConstraint::chk()");
    } break;
    case Type::T_CHOICE_A:
    case Type::T_SEQ_A: // T_EXTERNAL, T_EMBEDDED_PDV, T_UNRESTRICTEDSTRING
    case Type::T_SET_A:
      break;
    default:
      FATAL_ERROR("MultipleInnerTypeConstraint::chk()");
    }
    // check that all NamedConstraints refer to an existing component
    // if it exists add it to the map<> and check uniqueness
    // for SEQUENCE check order of fields
    size_t max_idx = 0; // current highest field index, to detect invalid order
    bool invalid_order = false;
    size_t present_count = 0;
    size_t absent_count = 0;
    for (size_t i=0; i<named_con_vec.size(); i++) {
      const Identifier& id = named_con_vec[i]->get_id();
      if (t->has_comp_withName(id)) {
        if (named_con_map.has_key(id)) {
          named_con_vec[i]->error("Duplicate reference to field `%s' of type `%s'",
            id.get_dispname().c_str(), my_type->get_typename().c_str());
          named_con_map[id]->note("Previous reference to field `%s' is here",
            id.get_dispname().c_str());
        } else {
          named_con_map.add(id, named_con_vec[i]);
          if (t->get_typetype()==Type::T_SEQ_A) {
            size_t curr_idx = t->get_comp_index_byName(id);
            if (curr_idx<max_idx) invalid_order = true;
            else max_idx = curr_idx;
          }
	  switch (named_con_vec[i]->get_presence_constraint()) {
	  case NamedConstraint::PC_PRESENT:
	    present_count++;
	    break;
	  case NamedConstraint::PC_ABSENT:
	    absent_count++;
	    break;
	  default:
	    break;
	  }
        }
        CompField* cf = t->get_comp_byName(id);
        switch (t->get_typetype()) {
        case Type::T_SEQ_A:
        case Type::T_SET_A:
	  if (!cf->get_is_optional() && (named_con_vec[i]->get_presence_constraint()!=NamedConstraint::PC_NONE)) {
	    named_con_vec[i]->error("Presence constraint `%s' cannot be used on mandatory field `%s'",
	      named_con_vec[i]->get_presence_constraint_name(), id.get_dispname().c_str());
	  }
	  break;
	default:
	  break;
        }
        Type* field_type = cf->get_type();
        named_con_vec[i]->set_my_type(field_type);
        named_con_vec[i]->set_my_scope(get_my_scope());
        named_con_vec[i]->set_my_parent(this);
        named_con_vec[i]->chk();
      } else {
        named_con_vec[i]->error("Type `%s' does not have a field named `%s'",
          my_type->get_typename().c_str(), id.get_dispname().c_str());
      }
    }
    if (invalid_order) {
      error("The order of fields must be the same as in the definition of type `%s'",
            my_type->get_typename().c_str());
    }
    if (t->get_typetype()==Type::T_CHOICE_A) {
      if (present_count>1) {
        error("CHOICE type `%s' cannot have more than one `PRESENT' field", my_type->get_typename().c_str());
      }
      // in FullSpecification all not listed fields that can be absent are implicitly ABSENT
      size_t real_absent_count = absent_count + ((!get_partial())?(t->get_nof_comps()-named_con_map.size()):0);
      if (real_absent_count>=t->get_nof_comps()) {
        error("All fields of CHOICE type `%s' are `ABSENT'", my_type->get_typename().c_str());
	if (real_absent_count>absent_count) {
	  note("%ld not listed field%s implicitly `ABSENT' because FullSpecification was used",
	    (long)(real_absent_count-absent_count), ((real_absent_count-absent_count)>1)?"s are":" is");
	}
      }
    }

    // inner subtype is ignored ( ETSI ES 201 873-7 V4.1.2 -> 9.1 Transformation rules )
    subtype = new SubtypeConstraint(my_type->get_subtype_type());
  }

  void MultipleInnerTypeConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for (size_t i=0; i<named_con_vec.size(); i++) {
      named_con_vec[i]->set_fullname(p_fullname+".<"+string(named_con_vec[i]->get_name())+" "+Int2string(i)+">");
    }
  }

  // =================================
  // ===== UnparsedMultipleInnerTypeConstraint
  // =================================

  UnparsedMultipleInnerTypeConstraint::UnparsedMultipleInnerTypeConstraint(const UnparsedMultipleInnerTypeConstraint& p)
    : Constraint(p)
  {
    block = p.block ? p.block->clone() : 0;
    constraint = 0;
  }

  UnparsedMultipleInnerTypeConstraint::UnparsedMultipleInnerTypeConstraint(Block* p_block)
    : Constraint(CT_UNPARSEDMULTIPLEINNERTYPE), block(p_block), constraint(0)
  {
    if (!block) FATAL_ERROR("UnparsedMultipleInnerTypeConstraint::UnparsedMultipleInnerTypeConstraint()");
  }

  UnparsedMultipleInnerTypeConstraint::~UnparsedMultipleInnerTypeConstraint()
  {
    delete block;
    delete constraint;
  }

  void UnparsedMultipleInnerTypeConstraint::chk()
  {
    if (checked) return;
    checked = true;
    if (!my_type) FATAL_ERROR("UnparsedMultipleInnerTypeConstraint::chk()");
    Error_Context cntxt(this, "While checking inner type constraint");
    Type* t = my_type->get_type_refd_last();
    switch (t->get_typetype()) {
    case Type::T_REAL: // associated ASN.1 type is a SEQUENCE
    case Type::T_CHOICE_A:
    case Type::T_SEQ_A: // also refd by T_EXTERNAL, T_EMBEDDED_PDV and T_UNRESTRICTEDSTRING
    case Type::T_SET_A:
      break;
    default:
      error("Multiple inner type constraint (WITH COMPONENTS) cannot be used on type `%s'", my_type->get_typename().c_str());
      return;
    }
    Node *node = block->parse(KW_Block_MultipleTypeConstraints);
    constraint = dynamic_cast<MultipleInnerTypeConstraint*>(node);
    if (!constraint) {
      delete node;
      return; // parsing error was already reported
    }
    constraint->set_my_type(my_type);
    constraint->set_my_scope(get_my_scope());
    constraint->set_my_parent(this);
    constraint->chk();
    subtype = new SubtypeConstraint(my_type->get_subtype_type()); // ignored
  }

  void UnparsedMultipleInnerTypeConstraint::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    block->set_fullname(p_fullname);
    if (constraint) constraint->set_fullname(p_fullname);
  }

} // namespace Common
