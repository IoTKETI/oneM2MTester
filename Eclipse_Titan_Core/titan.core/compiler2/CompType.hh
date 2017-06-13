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
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef COMPTYPE_HH_
#define COMPTYPE_HH_

#include "Setting.hh"

struct output_struct_t;

namespace Ttcn {
class Definition;
class WithAttribPath;
class Reference;
}

namespace Common {

class CompTypeRefList;

/** Class to represent the definition of a component type */
class ComponentTypeBody : public Scope, public Location {
public:
  /** represents the check state of this component
    * every check increments the value */
  enum check_state_t {
    CHECK_INITIAL,    /**< initial state before checks */
    CHECK_WITHATTRIB, /**< with attribute checked,
                           set by chk_extends_attr() */
    CHECK_EXTENDS,    /**< component extension checked,
                           set by chk_extends() */
    CHECK_DEFUNIQ,    /**< definitions id uniqueness checked,
                           set by chk_defs_uniq() */
    CHECK_OWNDEFS,    /**< own definitions checked,
                           set by chk_my_defs() */
    CHECK_EXTDEFS,    /**< definitions inherited by attribute checked,
                           set by chk_attr_ext_defs() */
    CHECK_FINAL       /**< everything was checked, set by chk() */
  };
private:
  /** Identifier of this component */
  const Identifier *comp_id;
  /** the Type that contains this, needed to reach w_attrib_path */
  Type *my_type;
  /** tells which checks have already been performed */
  check_state_t check_state;
  /** true when chk() is running */
  bool checking;
  /** Vector containing own definitions. Used for building. */
  vector<Ttcn::Definition> def_v;
  /** component references from the "extends" part or NULL if there were no */
  CompTypeRefList *extends_refs;
  /** component references from the extends attribute or NULL */
  CompTypeRefList *attr_extends_refs;
  /** map of own definitions equal to inherited by extension attribute
    * needed to initialize the the value with own value,
    * pointers to the defs are owned by def_v */
  map<string, Ttcn::Definition> orig_defs_m;
  /** map of my and inherited definitions */
  map<string, Ttcn::Definition> all_defs_m;
  /** compatibility map for is_compatible() function, filled in chk(),
    * all components inherited along pure extends-keyword path have the value
    * equal to the key, others have NULL value */
  map<ComponentTypeBody*, ComponentTypeBody> compatible_m;
  void init_compatibility(CompTypeRefList *crl, bool is_standard_extends);
private:
  /** Copy constructor not implemented */
  ComponentTypeBody(const ComponentTypeBody& p);
  /** Assignment not implemented */
  ComponentTypeBody& operator=(const ComponentTypeBody& p);
public:
  ComponentTypeBody()
  : Scope(), comp_id(0), my_type(0), check_state(CHECK_INITIAL),
    checking(false), def_v(), extends_refs(0), attr_extends_refs(0),
    orig_defs_m(), all_defs_m(), compatible_m() { }
  ~ComponentTypeBody();
  ComponentTypeBody *clone() const;

  void set_id(const Identifier *p_id) { comp_id = p_id; }
  Identifier const * get_id() const { return comp_id; }
  void set_my_type(Type *p_type) { my_type = p_type; }
  Type* get_my_type() { return my_type; }

  virtual void set_fullname(const string& p_fullname);
  virtual void set_my_scope(Scope *p_scope);

  bool has_local_ass_withId(const Identifier& p_id);
  Assignment* get_local_ass_byId(const Identifier& p_id);
  bool has_ass_withId(const Identifier& p_id);
  bool is_own_assignment(const Assignment* p_ass);
  size_t get_nof_asss();
  Assignment* get_ass_byIndex(size_t p_i);
  virtual Assignment *get_ass_bySRef(Ref_simple *p_ref);

  void add_extends(CompTypeRefList *p_crl);

  /** Returns if this component is compatible with the given other component.
    * This component is compatible with other component if this extends the
    * other component (with standard extends or with extension attribute)
    * directly or this extends a component which is compatible with other */
  bool is_compatible(ComponentTypeBody *other);
  /** Adds the assignment p_ass and becomes the owner of it.
   *  The uniqueness of the identifier is not checked. */
  void add_ass(Ttcn::Definition *p_ass);
  /** Prints the contents of the component. */
  void dump(unsigned level) const;
private:
  void chk_extends_attr();
public:
  /** Check component extends recursions (extends keyword and attribute) */
  void chk_recursion(ReferenceChain& refch);
private:
  void collect_defs_from_standard_extends();
  void collect_defs_from_attribute_extends();
  /** Checks the AST for correct component extensions. */
  void chk_extends();
  /** Checks definition id uniqueness */
  void chk_defs_uniq();
  /** Checks all own definitions */
  void chk_my_defs();
  /** Checks definitions inherited with attribute extends */
  void chk_attr_ext_defs();
public:
  /** perform all checks */
  void chk(check_state_t required_check_state = CHECK_FINAL);
  /** Sets the genname of embedded definitions using \a prefix. */
  void set_genname(const string& prefix);
  void generate_code(output_struct_t *target);
  /** Generates a pair of C++ strings that contain the module name and name
   * of the component type and appends them to \a str. */
  char *generate_code_comptype_name(char *str);

  void set_parent_path(Ttcn::WithAttribPath* p_path);
};

/** Class to represent a list of references to components */
class CompTypeRefList : public Node, public Location {
private:
  bool checked;
  vector<Ttcn::Reference> comp_ref_v;
  /** Pointers to ComponentTypeBody objects in the AST, filled by chk() */
  map<ComponentTypeBody*,Ttcn::Reference> comp_body_m;
  /** Contains the keys of comp_body_m in the order determined by comp_ref_v,
    * used to perform checks in deterministic order ( the order of keys in
    * comp_body_m is platform dependent */
  vector<ComponentTypeBody> comp_body_v;
  /** Copy constructor not implemented */
  CompTypeRefList(const CompTypeRefList& p);
  /** Assignment disabled */
  CompTypeRefList& operator=(const CompTypeRefList& p);
public:
  CompTypeRefList()
  : checked(false), comp_ref_v(), comp_body_m(), comp_body_v() { }
  ~CompTypeRefList();
  CompTypeRefList *clone() const;
  void add_ref(Ttcn::Reference *p_ref);
  void dump(unsigned level) const;
  virtual void set_fullname(const string& p_fullname);
  void chk_uniq();
  void chk(ComponentTypeBody::check_state_t required_check_state);
  void chk_recursion(ReferenceChain& refch);
  virtual void set_my_scope(Scope *p_scope);

  /** utility functions to access comp_body_m */
  size_t get_nof_comps();
  ComponentTypeBody *get_nth_comp_body(size_t i);
  Ttcn::Reference *get_nth_comp_ref(size_t i);
  bool has_comp_body(ComponentTypeBody *cb);
};

} /* namespace Common */
#endif /* COMPTYPE_HH_ */
