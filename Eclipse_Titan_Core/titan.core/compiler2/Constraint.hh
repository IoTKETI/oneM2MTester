/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_Constraint_HH
#define _Common_Constraint_HH

#include "Setting.hh"

namespace Asn {
  class Block;
}

namespace Common {

  /**
   * \ingroup Type
   *
   * \defgroup Constraint Constraint specifications
   *
   * These classes provide a unified interface for constraints.
   *
   * @{
   */

  class Constraint;
  class Constraints;

  // not defined here
  class Type;
  class Value;
  class SubtypeConstraint;
  class Identifier;

  using Asn::Block;

  /**
   * Class to represent Constraint.
   */
  class Constraint : public Node, public Location {
  public:
    enum constrtype_t {
      CT_ELEMENTSETSPEC, /** ElementSetSpecsConstraint */
      CT_IGNORE, /**< Constraint : used if erroneous or not yet supported */
      CT_TABLE, /**< TableConstraint */
      CT_SINGLEVALUE, /**< SingleValueConstraint */
      CT_CONTAINEDSUBTYPE, /**< ContainedSubtypeConstraint */
      CT_VALUERANGE, /**< ValueRangeConstraint */
      CT_SIZE, /**< SizeConstraint */
      CT_PERMITTEDALPHABET, /**< PermittedAlphabetConstraint */
      CT_PATTERN, /**< PatternConstraint */
      CT_SETOPERATION, /**< SetOperationConstraint */
      CT_FULLSET, /**< FullSetConstraint */
      CT_SINGLEINNERTYPE, /**< SingleInnerTypeConstraint */
      CT_NAMED, /**< NamedConstraint */
      CT_UNPARSEDMULTIPLEINNERTYPE, /**< UnparsedMultipleInnerTypeConstraint */
      CT_MULTIPLEINNERTYPE /**< MultipleInnerTypeConstraint */
    };
  protected:
    constrtype_t constrtype;
    Type* my_type; /**< the type to which this constraint belongs to */
    Scope* my_scope; /**< scope or NULL, if NULL then get scope from my_type */
    Constraint* my_parent; /**< the parent constraint node in AST or NULL */
    Constraints* my_cons; /**< the Constraints object or NULL, it is set if this
                               is the root node of a constraint tree */
    bool checked; /**< set to true by chk() */
    SubtypeConstraint* subtype; /**< NULL or set by chk() if constraint is valid, this is the root part */
    bool extendable; /**< set by chk() */
    SubtypeConstraint* extension; /**< NULL if there is no extension or not valid, set by chk() */

    Constraint(const Constraint& p);
    /// Assignment disabled
    Constraint& operator=(const Constraint& p);
  public:
    Constraint(constrtype_t p_constrtype);
    virtual ~Constraint();
    virtual Constraint* clone() const = 0;
    constrtype_t get_constrtype() const { return constrtype; }
    void set_my_type(Type *p_my_type) { my_type = p_my_type; }
    void set_my_parent(Constraint* p_my_parent) { my_parent = p_my_parent; }
    Constraint* get_my_parent() const { return my_parent; }
    void set_my_scope(Scope* p_my_scope) { my_scope = p_my_scope; }
    Scope* get_my_scope();
    void set_my_cons(Constraints* p_my_cons);
    Constraints* get_my_cons();
    virtual void chk() = 0;
    /** return the subtype constraint */
    SubtypeConstraint* get_subtype() const { return subtype; }
    bool is_extendable() const { return extendable; }
    SubtypeConstraint* get_extension() const { return extension; }
    /** returns the name of this constraint which can be used in error messages */
    virtual const char* get_name() const = 0;
    /** if this constraint is inside a FROM() constraint then we are in
        a char context */
    bool in_char_context() const;
    /** returns true if this constraint is inside a single or multiple inner type constraint */
    bool in_inner_constraint() const;
    /** returns if this kind of constraint is ignored, when ignored the 
        full set is used as it's subtype value
        TODO: set operations which have operands that are ignored constraints should
             somehow drop or simplify their constraint, example: (inner subtype) EXCEPT (inner subtype)
    */
    virtual bool is_ignored() const { return false; }
  };

  /**
   * Class to represent constraints.
   */
  class Constraints : public Node {
  private:
    vector<Constraint> cons;
    Type *my_type;
    SubtypeConstraint* subtype; /**< NULL or set by chk() if constraint is valid, this is the root part */
    bool extendable; /**< set by chk() */
    SubtypeConstraint* extension; /**< NULL if there is no extension or not valid, set by chk() */

    Constraints(const Constraints& p);
    /// Assignment disabled
    Constraints& operator=(const Constraints& p);
  public:
    Constraints() : Node(), cons(), my_type(0), subtype(0), extendable(false), extension(0) {}
    virtual ~Constraints();
    virtual Constraints *clone() const;
    void add_con(Constraint *p_con);
    size_t get_nof_cons() const {return cons.size();}
    Constraint* get_con_byIndex(size_t p_i) const { return cons[p_i]; }
    Constraint* get_tableconstraint() const;
    void set_my_type(Type *p_my_type);
    Type* get_my_type() const { return my_type; }
    void chk(SubtypeConstraint* parent_subtype);
    void chk_table();
    /* return the subtype constraint */
    SubtypeConstraint* get_subtype() const { return subtype; }
    bool is_extendable() const { return extendable; }
    SubtypeConstraint* get_extension() const { return extension; }
  };

  /** @} end of Constraint group */

  
  // =================================
  // ===== ElementSetSpecsConstraint
  // =================================
  
  /* This AST element is used only if "..." is present, otherwise it's not
     created, therefore the extendable bool is always true in this constraint */
  class ElementSetSpecsConstraint : public Constraint
  {
    Constraint* root_constr;
    Constraint* ext_constr; /* NULL if no extension addition was present */
    ElementSetSpecsConstraint(const ElementSetSpecsConstraint& p);
    /// Assignment disabled
    ElementSetSpecsConstraint& operator=(const ElementSetSpecsConstraint& p);
  public:
    ElementSetSpecsConstraint(Constraint* p_root_constr, Constraint* p_ext_constr);
    virtual ~ElementSetSpecsConstraint();
    ElementSetSpecsConstraint* clone() const { return new ElementSetSpecsConstraint(*this); }
    void chk();
    const char* get_name() const { return "ElementSetSpecs constraint"; }
    void set_fullname(const string& p_fullname);
  };

  // =================================
  // ===== IgnoredConstraint
  // =================================

  class IgnoredConstraint : public Constraint
  {
    const char* my_name;
    IgnoredConstraint(const IgnoredConstraint& p);
    /// Assignment disabled
    IgnoredConstraint& operator=(const IgnoredConstraint& p);
  public:
    IgnoredConstraint(const char* p_name);
    IgnoredConstraint* clone() const { return new IgnoredConstraint(*this); }
    void chk();
    const char* get_name() const { return my_name; }
    bool is_ignored() const { return true; }
  };


  // =================================
  // ===== SingleValueConstraint
  // =================================

  class SingleValueConstraint : public Constraint
  {
    Value* value;
    SingleValueConstraint(const SingleValueConstraint& p);
    /// Assignment disabled
    SingleValueConstraint& operator=(const SingleValueConstraint& p);
  public:
    SingleValueConstraint(Value* p_value);
    virtual ~SingleValueConstraint();
    SingleValueConstraint* clone() const { return new SingleValueConstraint(*this); }
    void chk();
    const char* get_name() const { return "single value constraint"; }
    void set_fullname(const string& p_fullname);
  };

  // =================================
  // ===== ContainedSubtypeConstraint
  // =================================

  class ContainedSubtypeConstraint : public Constraint
  {
    Type* type;
    bool has_includes; /**< INCLUDES keyword was used, then this cannot be a TypeConstraint,
                            otherwise it can be both TypeConstraint and ContainedSubtypeConstraint */
    ContainedSubtypeConstraint(const ContainedSubtypeConstraint& p);
    /// Assignment disabled
    ContainedSubtypeConstraint& operator=(const ContainedSubtypeConstraint& p);
  public:
    ContainedSubtypeConstraint(Type* p_type, bool p_has_includes);
    virtual ~ContainedSubtypeConstraint();
    ContainedSubtypeConstraint* clone() const { return new ContainedSubtypeConstraint(*this); }
    void chk();
    const char* get_name() const { return "contained subtype constraint"; }
    void set_fullname(const string& p_fullname);
  };

  // =================================
  // ===== RangeEndpoint
  // =================================

  class ValueRangeConstraint;

  class RangeEndpoint : public Node, public Location
  {
  public:
    enum endpoint_t {
      MIN, /**< minimum value */
      VALUE, /**< value */
      MAX /**< maximum value */
    };
  private:
    endpoint_t type; /**< type of endpoint */
    Value* value; /**< a value or NULL if type!=VALUE */
    bool inclusive; /**< true if the endpoint is part of the range */
    RangeEndpoint(const RangeEndpoint& p);
    RangeEndpoint& operator=(const RangeEndpoint& p); // = disabled
  public:
    RangeEndpoint(endpoint_t p_type);
    RangeEndpoint(Value* p_value);
    void set_exclusive() { inclusive = false; }
    bool get_exclusive() const { return !inclusive; }
    endpoint_t get_type() const { return type; }
    Value* get_value() const { return value; }
    ~RangeEndpoint();
    RangeEndpoint* clone() const { return new RangeEndpoint(*this); }
    void chk(Type* my_type, ValueRangeConstraint* constraint);
    void set_fullname(const string& p_fullname);
  };

  // =================================
  // ===== ValueRangeConstraint
  // =================================

  class ValueRangeConstraint : public Constraint
  {
    RangeEndpoint* lower_endpoint;
    RangeEndpoint* upper_endpoint;
    ValueRangeConstraint(const ValueRangeConstraint& p);
    /// Assignment disabled
    ValueRangeConstraint& operator=(const ValueRangeConstraint& p);
  public:
    ValueRangeConstraint(RangeEndpoint* p_lower, RangeEndpoint* p_upper);
    virtual ~ValueRangeConstraint() { delete lower_endpoint; delete upper_endpoint; }
    ValueRangeConstraint* clone() const { return new ValueRangeConstraint(*this); }
    void chk();
    const char* get_name() const { return "value range constraint"; }
    void set_fullname(const string& p_fullname);
  };

  // =================================
  // ===== SizeConstraint
  // =================================

  class SizeConstraint : public Constraint
  {
    Constraint* constraint;
    SizeConstraint(const SizeConstraint& p);
    /// Assignment disabled
    SizeConstraint& operator=(const SizeConstraint& p);
  public:
    SizeConstraint(Constraint* p);
    virtual ~SizeConstraint() { delete constraint; }
    SizeConstraint* clone() const { return new SizeConstraint(*this); }
    void chk();
    const char* get_name() const { return "size constraint"; }
    void set_fullname(const string& p_fullname);
  };

  // =================================
  // ===== PermittedAlphabetConstraint
  // =================================

  class PermittedAlphabetConstraint : public Constraint
  {
    Constraint* constraint;
    PermittedAlphabetConstraint(const PermittedAlphabetConstraint& p);
    /// Assignment disabled
    PermittedAlphabetConstraint& operator=(const PermittedAlphabetConstraint& p);
  public:
    PermittedAlphabetConstraint(Constraint* p);
    virtual ~PermittedAlphabetConstraint() { delete constraint; }
    PermittedAlphabetConstraint* clone() const { return new PermittedAlphabetConstraint(*this); }
    void chk();
    const char* get_name() const { return "permitted alphabet constraint"; }
    void set_fullname(const string& p_fullname);
  };

  // =================================
  // ===== SetOperationConstraint
  // =================================

  class SetOperationConstraint : public Constraint
  {
  public:
    enum operationtype_t {
      UNION,
      INTERSECTION,
      EXCEPT
    };
  private:
    operationtype_t operationtype;
    Constraint* operand_a;
    Constraint* operand_b;
    SetOperationConstraint(const SetOperationConstraint& p);
    /// Assignment disabled
    SetOperationConstraint& operator=(const SetOperationConstraint& p);
  public:
    SetOperationConstraint(Constraint* p_a, operationtype_t p_optype, Constraint* p_b);
    virtual ~SetOperationConstraint() { delete operand_a; delete operand_b; }
    void set_first_operand(Constraint* p_a);
    SetOperationConstraint* clone() const { return new SetOperationConstraint(*this); }
    const char* get_operationtype_str() const;
    void chk();
    const char* get_name() const { return get_operationtype_str(); }
    void set_fullname(const string& p_fullname);
  };
  
  // =================================
  // ===== FullSetConstraint
  // =================================

  class FullSetConstraint : public Constraint
  {
    FullSetConstraint(const FullSetConstraint& p): Constraint(p) {}
    /// Assignment disabled
    FullSetConstraint& operator=(const FullSetConstraint& p);
  public:
    FullSetConstraint(): Constraint(CT_FULLSET) {}
    FullSetConstraint* clone() const { return new FullSetConstraint(*this); }
    void chk();
    const char* get_name() const { return "ALL"; }
  };

  // =================================
  // ===== PatternConstraint
  // =================================

  class PatternConstraint : public Constraint
  {
    Value* value;
    PatternConstraint(const PatternConstraint& p);
    /// Assignment disabled
    PatternConstraint& operator=(const PatternConstraint& p);
  public:
    PatternConstraint(Value* p_value);
    virtual ~PatternConstraint();
    PatternConstraint* clone() const { return new PatternConstraint(*this); }
    void chk();
    const char* get_name() const { return "pattern constraint"; }
    void set_fullname(const string& p_fullname);
    bool is_ignored() const { return true; }
  };

  // =================================
  // ===== SingleInnerTypeConstraint
  // =================================

  class SingleInnerTypeConstraint : public Constraint
  {
    Constraint* constraint;
    SingleInnerTypeConstraint(const SingleInnerTypeConstraint& p);
    /// Assignment disabled
    SingleInnerTypeConstraint& operator=(const SingleInnerTypeConstraint& p);
  public:
    SingleInnerTypeConstraint(Constraint* p);
    virtual ~SingleInnerTypeConstraint() { delete constraint; }
    SingleInnerTypeConstraint* clone() const { return new SingleInnerTypeConstraint(*this); }
    void chk();
    void set_fullname(const string& p_fullname);
    const char* get_name() const { return "inner type constraint"; }
    bool is_ignored() const { return true; }
  };

  // =================================
  // ===== NamedConstraint
  // =================================

  class NamedConstraint : public Constraint
  {
  public:
    enum presence_constraint_t {
      PC_NONE, // presence constraint was not specified
      PC_PRESENT,
      PC_ABSENT,
      PC_OPTIONAL
    };
  private:
    Identifier* id;
    Constraint* value_constraint; // NULL if it was not specified
    presence_constraint_t presence_constraint;
    NamedConstraint(const NamedConstraint& p);
    /// Assignment disabled
    NamedConstraint& operator=(const NamedConstraint& p);
  public:
    NamedConstraint(Identifier* p_id, Constraint* p_value_constraint, presence_constraint_t p_presence_constraint);
    ~NamedConstraint();
    const Identifier& get_id() const { return *id; }
    Constraint* get_value_constraint() const { return value_constraint; }
    presence_constraint_t get_presence_constraint() const { return presence_constraint; }
    const char* get_presence_constraint_name() const;
    NamedConstraint* clone() const { return new NamedConstraint(*this); }
    void chk();
    void set_fullname(const string& p_fullname);
    const char* get_name() const { return "named constraint"; }
    bool is_ignored() const { return true; }
  };

  // =================================
  // ===== MultipleInnerTypeConstraint
  // =================================

  class MultipleInnerTypeConstraint : public Constraint
  {
    bool partial; // Full/Partial Specification
    vector<NamedConstraint> named_con_vec;
    map<Identifier,NamedConstraint> named_con_map; // values owned by named_con_vec, filled by chk()
    MultipleInnerTypeConstraint(const MultipleInnerTypeConstraint& p);
    /// Assignment disabled
    MultipleInnerTypeConstraint& operator=(const MultipleInnerTypeConstraint& p);
  public:
    MultipleInnerTypeConstraint(): Constraint(CT_MULTIPLEINNERTYPE) {}
    virtual ~MultipleInnerTypeConstraint();
    MultipleInnerTypeConstraint* clone() const { return new MultipleInnerTypeConstraint(*this); }

    void set_partial(bool b) { partial = b; }
    bool get_partial() const { return partial; }
    void addNamedConstraint(NamedConstraint* named_con);

    void chk();
    void set_fullname(const string& p_fullname);
    const char* get_name() const { return "inner type constraint"; }
    bool is_ignored() const { return true; }
  };

  // =================================
  // ===== UnparsedMultipleInnerTypeConstraint
  // =================================

  class UnparsedMultipleInnerTypeConstraint : public Constraint
  {
    Block* block;
    MultipleInnerTypeConstraint* constraint;
    UnparsedMultipleInnerTypeConstraint(const UnparsedMultipleInnerTypeConstraint& p);
    /// Assignment disabled
    UnparsedMultipleInnerTypeConstraint& operator=(const UnparsedMultipleInnerTypeConstraint& p);
  public:
    UnparsedMultipleInnerTypeConstraint(Block* p_block);
    virtual ~UnparsedMultipleInnerTypeConstraint();
    UnparsedMultipleInnerTypeConstraint* clone() const { return new UnparsedMultipleInnerTypeConstraint(*this); }
    void chk();
    void set_fullname(const string& p_fullname);
    const char* get_name() const { return "inner type constraint"; }
    bool is_ignored() const { return true; }
  };

} // namespace Common

#endif // _Common_Constraint_HH
