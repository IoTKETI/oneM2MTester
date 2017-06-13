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
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef ENUM_HH_
#define ENUM_HH_

#include "Setting.hh"
#include "Identifier.hh"

namespace Common {

class int_val_t;
  
/**
 * Class to represent an Enumeration item. Is like a NamedValue, but
 * the value can be NULL, and a(n integer) value can be assigned
 * later.
 */
class EnumItem : public Node, public Location {
private:
  Identifier *name;
  Value *value;
  string text; ///< for TEXT encoding instruction
  int_val_t *int_value;
  /** Copy constructor not implemented */
  EnumItem(const EnumItem&);
  /** Assignment disabled */
  EnumItem& operator=(const EnumItem&);
public:
  EnumItem(Identifier *p_name, Value *p_value);
  virtual ~EnumItem();
  virtual EnumItem *clone() const;
  virtual void set_fullname(const string& p_fullname);
  const Identifier& get_name() const { return *name; }
  Value *get_value() const { return value; }
  void set_value(Value *p_value);
  const string& get_text() const { return text; }
  void set_text(const string& p_text);
  virtual void set_my_scope(Scope *p_scope);
  bool calculate_int_value();
  int_val_t* get_int_val() const;
  virtual void dump(unsigned level) const;
};

/**
 * Class to represent a collection of enumerated items.
 */
class EnumItems : public Node {
private:
  Scope *my_scope;
  vector<EnumItem> eis_v;
  /** Stores the first occurrence of EnumItem with id. The string key
   * refers to the id of the ei, the value is the ei itself. */
  map<string, EnumItem> eis_m;
  /** Copy constructor not implemented */
  EnumItems(const EnumItems& p);
  /** Assignment disabled */
  EnumItems& operator=(const EnumItems& p);
public:
  EnumItems() : Node(), my_scope(0), eis_v(), eis_m() { }
  virtual ~EnumItems();
  /** Clears the vector and the map, but does not delete the items */
  void release_eis();
  virtual EnumItems *clone() const;
  virtual void set_fullname(const string& p_fullname);
  void set_my_scope(Scope *p_scope);
  void add_ei(EnumItem *p_nv);
  size_t get_nof_eis() const { return eis_v.size(); }
  EnumItem* get_ei_byIndex(const size_t p_i) { return eis_v[p_i]; }
  bool has_ei_withName(const Identifier& p_name) const
    { return eis_m.has_key(p_name.get_name()); }
  EnumItem* get_ei_byName(const Identifier& p_name) const
    { return eis_m[p_name.get_name()]; }
  virtual void dump(unsigned level) const;
};

}

#endif /* ENUM_HH_ */
