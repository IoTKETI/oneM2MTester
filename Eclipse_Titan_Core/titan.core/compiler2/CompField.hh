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
 *
 ******************************************************************************/
#ifndef COMPFIELD_HH_
#define COMPFIELD_HH_

#include "Setting.hh"

class RawAST;

namespace Common {

/**
 * A class to represent a field of a compound type (sequence/record,
 * set and choice/union).
 */
class CompField : public Node, public Location {
private:
  /** The name of the field. Owned by the CompField. */
  Identifier *name;
  /** The type of the field. Owned. */
  Type *type;
  bool is_optional;
  /** Default value or 0 if no default value. Owned. */
  Value *defval;
  /** Raw attributes or 0. Owned */
  RawAST *rawattrib;
  /** Copy constructor not implemented */
  CompField(const CompField& p);
  /** Assignment disabled */
  CompField& operator=(const CompField& p);
public:
  CompField(Identifier *p_name, Type *p_type, bool p_is_optional=false,
    Value *p_defval=0);
  virtual ~CompField();
  virtual CompField *clone() const;
  virtual void set_fullname(const string& p_fullname);
  void set_my_scope(Scope *p_scope);
  const Identifier& get_name() const { return *name; }
  Type *get_type() const { return type; }
  RawAST* get_raw_attrib() const { return rawattrib; }
  void set_raw_attrib(RawAST* r_attr);
  bool get_is_optional() const { return is_optional; }
  bool has_default() const { return defval != 0; }
  Value *get_defval() const { return defval; }
  virtual void dump(unsigned level) const;
};

/**
 * Class to represent a map of TTCN-3 StructFieldDef (BNF
 * 23). Create a CompFieldMap and use add_comp(CompField *) to add
 * each StructFieldDef (a.k.a. CompRef) into it! Finally, pass
 * CompFieldMap* to constructors of record/set/union types!
 */
class CompFieldMap : public Node {
private:
  /** Maps the name of the component to the actual component. */
  map<string, CompField> m;
  /** Contains pointers to the individual CompField s.
   * The CompFieldMap owns the CompFields and will free them. */
  vector<CompField> v;
  /** Points to the owner type, which shall be a TTCN-3 record, set or
   * union or an ASN.1 open type.
   * The CompFieldMap does not own the type. */
  Type *my_type;
  /** Indicates whether the uniqueness of field identifiers has been
   * verified. */
  bool checked;

  CompFieldMap(const CompFieldMap& p);
  /** Assignment disabled */
  CompFieldMap& operator=(const CompFieldMap& p);
public:
  CompFieldMap() : Node(), m(), v(), my_type(0), checked(false) {}
  virtual ~CompFieldMap();
  virtual CompFieldMap *clone() const;
  virtual void set_fullname(const string& p_fullname);
  virtual void set_my_scope(Scope *p_scope);
  void set_my_type(Type *p_my_type) { my_type = p_my_type; }
  void add_comp(CompField *comp);
  size_t get_nof_comps() const { return v.size(); }
  CompField* get_comp_byIndex(size_t n) const { return v[n]; }
  bool has_comp_withName(const Identifier& p_name);
  CompField* get_comp_byName(const Identifier& p_name);
private:
  const char *get_typetype_name() const;
  /** Check the uniqueness of field identifiers.
   * Builds the map \c m in the process. */
  void chk_uniq();
public:
  void chk();
  virtual void dump(unsigned level) const;
};


} /* namespace Common */
#endif /* COMPFIELD_HH_ */
