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
#include "CompField.hh"
#include "Type.hh"
#include "Value.hh"
#include "CompilerError.hh"

namespace Common {

// =================================
// ===== CompField
// =================================

CompField::CompField(Identifier *p_name, Type *p_type, bool p_is_optional,
  Value *p_defval)
  : Node(), Location(), name(p_name), type(p_type),
    is_optional(p_is_optional), defval(p_defval), rawattrib(0)
{
  if(!p_name || !p_type)
    FATAL_ERROR("NULL parameter: Common::CompField::CompField()");
  type->set_ownertype(Type::OT_COMP_FIELD, this);
}

CompField::CompField(const CompField& p)
  : Node(p), Location(p), is_optional(p.is_optional), rawattrib(0)
{
  name=p.name->clone();
  type=p.type->clone();
  type->set_ownertype(Type::OT_COMP_FIELD, this);
  defval=p.defval?p.defval->clone():0;
}

CompField::~CompField()
{
  delete name;
  delete type;
  delete defval;
  delete rawattrib;
}

CompField *CompField::clone() const
{
  return new CompField(*this);
}

void CompField::set_fullname(const string& p_fullname)
{
  string base_name(p_fullname + "." + name->get_dispname());
  Node::set_fullname(base_name);
  type->set_fullname(base_name);
  if (defval) defval->set_fullname(base_name + ".<defval>");
}

void CompField::set_my_scope(Scope *p_scope)
{
  type->set_my_scope(p_scope);
  if (defval) defval->set_my_scope(p_scope);
}

void CompField::set_raw_attrib(RawAST* r_attr)
{
  delete rawattrib;
  rawattrib=r_attr;
}

void CompField::dump(unsigned level) const
{
  name->dump(level);
  type->dump(level + 1);
  if(is_optional)
    DEBUG(level + 1, "optional");
  if(defval) {
    DEBUG(level + 1, "with default value");
    defval->dump(level + 2);
  }
}

// =================================
// ===== CompFieldMap
// =================================

CompFieldMap::CompFieldMap(const CompFieldMap& p)
  : Node(p), my_type(0), checked(false)
{
  size_t nof_comps = p.v.size();
  for (size_t i = 0; i < nof_comps; i++) v.add(p.v[i]->clone());
}

CompFieldMap::~CompFieldMap()
{
  size_t nof_comps = v.size();
  for (size_t i = 0; i < nof_comps; i++) delete v[i];
  v.clear();
  m.clear();
}

CompFieldMap *CompFieldMap::clone() const
{
  return new CompFieldMap(*this);
}

void CompFieldMap::set_fullname(const string& p_fullname)
{
  Node::set_fullname(p_fullname);
  size_t nof_comps = v.size();
  for (size_t i = 0; i < nof_comps; i++) v[i]->set_fullname(p_fullname);
}

void CompFieldMap::set_my_scope(Scope *p_scope)
{
  size_t nof_comps = v.size();
  for (size_t i = 0; i < nof_comps; i++) v[i]->set_my_scope(p_scope);
}

void CompFieldMap::add_comp(CompField *comp)
{
  v.add(comp);
  if (checked) {
    const string& name = comp->get_name().get_name();
    if (m.has_key(name)) FATAL_ERROR("CompFieldMap::add_comp(%s)", name.c_str());
    m.add(name, comp);
  }
}

bool CompFieldMap::has_comp_withName(const Identifier& p_name)
{
  if (!checked) chk_uniq();
  return m.has_key(p_name.get_name());
}

CompField* CompFieldMap::get_comp_byName(const Identifier& p_name)
{
  if (!checked) chk_uniq();
  return m[p_name.get_name()];
}

const char *CompFieldMap::get_typetype_name() const
{
  if (!my_type) FATAL_ERROR("CompFieldMap::get_typetype_name()");
  switch (my_type->get_typetype()) {
  case Type::T_ANYTYPE:
    return "anytype";
  case Type::T_CHOICE_T:
    return "union";
  case Type::T_SEQ_T:
    return "record";
  case Type::T_SET_T:
    return "set";
  case Type::T_OPENTYPE:
    return "open type";
  default:
    return "<unknown>";
  }
}

void CompFieldMap::chk_uniq()
{
  if (checked) return;
  const char *typetype_name = get_typetype_name();
  size_t nof_comps = v.size();
  for (size_t i = 0; i < nof_comps; i++) {
    CompField *comp = v[i];
    const Identifier& id = comp->get_name();
    const string& name = id.get_name();
    if (m.has_key(name)) {
      const char *dispname = id.get_dispname().c_str();
      comp->error("Duplicate %s field name `%s'", typetype_name, dispname);
      m[name]->note("Field `%s' is already defined here", dispname);
    } else m.add(name, comp);
  }
  checked = true;
}

void CompFieldMap::chk()
{
  if (!checked) chk_uniq();
  const char *typetype_name = get_typetype_name();
  size_t nof_comps = v.size();
  for (size_t i = 0; i < nof_comps; i++) {
    CompField *comp = v[i];
    const Identifier& id = comp->get_name();
    Error_Context cntxt(comp, "In %s field `%s'", typetype_name,
      id.get_dispname().c_str());
    Type *t = comp->get_type();
    t->set_genname(my_type->get_genname_own(), id.get_name());
    t->set_parent_type(my_type);
    t->chk();
    t->chk_embedded(true, "embedded into another type");
  }
}

void CompFieldMap::dump(unsigned level) const
{
  size_t nof_comps = v.size();
  DEBUG(level, "component fields: (%lu pcs.) @ %p",
    (unsigned long) nof_comps, (const void*)this);
  for (size_t i = 0; i < nof_comps; i++) v[i]->dump(level + 1);
}

} /* namespace Common */
