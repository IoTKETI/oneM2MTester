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
#include "CompType.hh"

#include "ttcn3/AST_ttcn3.hh"
#include "ttcn3/Attributes.hh"

// implemented in comptype_attrib_la.l
extern void parseExtendsCompTypeRefList(Ttcn::AttributeSpec const& attrib,
  Common::CompTypeRefList*& attr_comp_refs);

namespace Common {

// =================================
// ===== ComponentTypeBody
// =================================

ComponentTypeBody::~ComponentTypeBody()
{
  for(size_t i = 0; i < def_v.size(); i++) delete def_v[i];
  def_v.clear();
  delete extends_refs;
  delete attr_extends_refs;
  all_defs_m.clear();
  orig_defs_m.clear();
  compatible_m.clear();
}

ComponentTypeBody *ComponentTypeBody::clone() const
{
  FATAL_ERROR("ComponentTypeBody::clone()");
}

void ComponentTypeBody::set_fullname(const string& p_fullname)
{
  Scope::set_fullname(p_fullname);
  for(size_t i = 0; i < def_v.size(); i++) {
    Ttcn::Definition *def = def_v[i];
    def->set_fullname(p_fullname + "." + def->get_id().get_dispname());
  }
  if (extends_refs)
    extends_refs->set_fullname(p_fullname+".<extends>");
  if (attr_extends_refs)
    attr_extends_refs->set_fullname(get_fullname()+".<extends attribute>");
}

void ComponentTypeBody::set_my_scope(Scope *p_scope)
{
  set_parent_scope(p_scope);
  if (extends_refs) extends_refs->set_my_scope(p_scope);
  if (attr_extends_refs) attr_extends_refs->set_my_scope(p_scope);
}

void ComponentTypeBody::add_extends(CompTypeRefList *p_crl)
{
  if (extends_refs || !p_crl || check_state!=CHECK_INITIAL)
    FATAL_ERROR("ComponentTypeBody::add_extends()");
  extends_refs = p_crl;
}

bool ComponentTypeBody::is_compatible(ComponentTypeBody *other)
{
  if (other==this) return true;
  chk(CHECK_EXTENDS);
  other->chk(CHECK_EXTENDS);
  // a component with no definitions is compatible with all components
  if (def_v.size()==0 && !extends_refs && !attr_extends_refs) return true;
  return other->compatible_m.has_key(this);
}

void ComponentTypeBody::add_ass(Ttcn::Definition *p_ass)
{
  if (check_state!=CHECK_INITIAL || !p_ass)
    FATAL_ERROR("ComponentTypeBody::add_ass()");
  p_ass->set_my_scope(this);
  def_v.add(p_ass);
}

bool ComponentTypeBody::has_local_ass_withId(const Identifier& p_id)
{
  chk();
  return all_defs_m.has_key(p_id.get_name());
}

Assignment* ComponentTypeBody::get_local_ass_byId(const Identifier& p_id)
{
  chk();
  return all_defs_m[p_id.get_name()];
}

size_t ComponentTypeBody::get_nof_asss()
{
  chk();
  return all_defs_m.size();
}

bool ComponentTypeBody::has_ass_withId(const Identifier& p_id)
{
  if (has_local_ass_withId(p_id)) return true;
  else if (parent_scope) return parent_scope->has_ass_withId(p_id);
  else return false;
}

Assignment* ComponentTypeBody::get_ass_byIndex(size_t p_i)
{
  chk();
  return all_defs_m.get_nth_elem(p_i);
}

bool ComponentTypeBody::is_own_assignment(const Assignment* p_ass)
{
  for(size_t i = 0; i < def_v.size(); i++) {
    if (def_v[i] == p_ass) return true;
  }
  return false;
}

Assignment* ComponentTypeBody::get_ass_bySRef(Ref_simple *p_ref)
{
  if (!p_ref || !parent_scope)
    FATAL_ERROR("ComponentTypeBody::get_ass_bySRef()");
  if (!p_ref->get_modid()) {
    const Identifier *id = p_ref->get_id();
    if (id && has_local_ass_withId(*id)) {
      Assignment* ass = get_local_ass_byId(*id);
      if (!ass) FATAL_ERROR("ComponentTypeBody::get_ass_bySRef()");

      for(size_t i = 0; i < def_v.size(); i++) {
        if (def_v[i] == ass) return ass;
      }

      if (ass->get_visibility() == PUBLIC) {
        return ass;
      }

      p_ref->error("The member definition `%s' in component type `%s'"
        " is not visible in this scope", id->get_dispname().c_str(),
          comp_id->get_dispname().c_str());
      return 0;
    }
  }
  return parent_scope->get_ass_bySRef(p_ref);
}

void ComponentTypeBody::dump(unsigned level) const
{
  if (extends_refs) {
    DEBUG(level, "Extends references:");
    extends_refs->dump(level+1);
  }
  DEBUG(level, "Definitions: (%lu pcs.)", (unsigned long) def_v.size());
  for(size_t i = 0; i < def_v.size(); i++) def_v[i]->dump(level + 1);
  if (attr_extends_refs) {
    DEBUG(level, "Extends attribute references:");
    attr_extends_refs->dump(level+1);
  }
}

// read out the contents of the with attribute extension,
// parse it to the the attr_extends_refs member variable,
// collects all extends components from all attributes along the attrib path
void ComponentTypeBody::chk_extends_attr()
{
  if (check_state>=CHECK_WITHATTRIB) return;
  check_state = CHECK_WITHATTRIB;
  Ttcn::WithAttribPath* w_attrib_path = my_type->get_attrib_path();
  if(w_attrib_path)
  {
    vector<Ttcn::SingleWithAttrib> const &real_attribs =
      w_attrib_path->get_real_attrib();
    for (size_t i = 0; i < real_attribs.size(); i++)
    {
      Ttcn::SingleWithAttrib *act_attr = real_attribs[i];
      if (act_attr->get_attribKeyword()==Ttcn::SingleWithAttrib::AT_EXTENSION)
        parseExtendsCompTypeRefList(act_attr->get_attribSpec(),
                                    attr_extends_refs);
    }
  }
  if (attr_extends_refs)
  {
    attr_extends_refs->set_fullname(get_fullname()+".<extends attribute>");
    attr_extends_refs->set_my_scope(parent_scope);
  }
}

void ComponentTypeBody::chk_recursion(ReferenceChain& refch)
{
  if (refch.add(get_fullname())) {
    if (extends_refs) extends_refs->chk_recursion(refch);
    if (attr_extends_refs) attr_extends_refs->chk_recursion(refch);
  }
}

// contained components must already be checked,
// having valid compatible_m maps
void ComponentTypeBody::init_compatibility(CompTypeRefList *crl,
                                           bool is_standard_extends)
{
  for (size_t i = 0; i < crl->get_nof_comps(); i++)
  {
    ComponentTypeBody *cb = crl->get_nth_comp_body(i);
    if (!compatible_m.has_key(cb))
      compatible_m.add(cb, is_standard_extends ? cb : NULL);
    // compatible with all components which are compatible with the compatible
    // component
    for (size_t j = 0; j < cb->compatible_m.size(); j++)
    {
      ComponentTypeBody *ccb = cb->compatible_m.get_nth_key(j);
      ComponentTypeBody *ccbval = cb->compatible_m.get_nth_elem(j);
      if (!compatible_m.has_key(ccb))
        compatible_m.add(ccb, is_standard_extends ? ccbval : NULL);
    }
  }
}

void ComponentTypeBody::chk_extends()
{
  if (check_state>=CHECK_EXTENDS) return;
  check_state = CHECK_EXTENDS;
  // check the references in component extends parts (keyword and attribute)
  if (extends_refs) extends_refs->chk(CHECK_EXTENDS);
  if (attr_extends_refs) attr_extends_refs->chk(CHECK_EXTENDS);
  // check for circular reference chains
  ReferenceChain refch(this, "While checking component type extensions");
  chk_recursion(refch);
  // build compatibility map for faster is_compatible() and generate_code()
  // contained components must already have valid compatible_m map
  // keys used in is_compatible(), values used in generate_code()
  if (extends_refs) init_compatibility(extends_refs, true);
  if (attr_extends_refs) init_compatibility(attr_extends_refs, false);
}

void ComponentTypeBody::collect_defs_from_standard_extends()
{
  Assignments *module_defs = parent_scope->get_scope_asss();
  for (size_t i = 0; i < extends_refs->get_nof_comps(); i++) {
    ComponentTypeBody *parent = extends_refs->get_nth_comp_body(i);
    // iterate through all definitions of the parent
    // parent is already checked (has it's inherited defs included)
    for (size_t j = 0; j < parent->all_defs_m.size(); j++) {
      Ttcn::Definition *def = parent->all_defs_m.get_nth_elem(j);
      const Identifier& id = def->get_id();
      const string& name = id.get_name();
      if (all_defs_m.has_key(name)) {
        Ttcn::Definition *my_def = all_defs_m[name];
        if (my_def != def) {
          // it is not the same definition inherited on two paths
          const char *dispname_str = id.get_dispname().c_str();
          if (my_def->get_my_scope() == this) {
            my_def->error("Local definition `%s' collides with definition "
              "inherited from component type `%s'", dispname_str,
              def->get_my_scope()->get_fullname().c_str());
            def->note("Inherited definition of `%s' is here", dispname_str);
          } else {
            error("Definition `%s' inherited from component type `%s' "
              "collides with definition inherited from `%s'", dispname_str,
              def->get_my_scope()->get_fullname().c_str(),
              my_def->get_my_scope()->get_fullname().c_str());
            def->note("Definition of `%s' in `%s' is here", dispname_str,
              def->get_my_scope()->get_fullname().c_str());
            my_def->note("Definition of `%s' in `%s' is here", dispname_str,
              my_def->get_my_scope()->get_fullname().c_str());
          }
        }
      } else {
        all_defs_m.add(name, def);
        if (def->get_my_scope()->get_scope_mod() !=
            module_defs->get_scope_mod()) {
          // def comes from another module
          if (module_defs->has_local_ass_withId(id)) {
            const char *dispname_str = id.get_dispname().c_str();
            error("The name of inherited definition `%s' is not unique in "
              "the scope hierarchy", dispname_str);
            module_defs->get_local_ass_byId(id)->note("Symbol `%s' is "
              "already defined here in a higher scope unit", dispname_str);
            def->note("Definition `%s' inherited from component type `%s' "
              "is here", dispname_str,
              def->get_my_scope()->get_fullname().c_str());
          } else if (module_defs->is_valid_moduleid(id)) {
            def->warning("Inherited definition with name `%s' hides a "
              "module identifier", id.get_dispname().c_str());
          }
        }
      }
    }
  }
}

void ComponentTypeBody::collect_defs_from_attribute_extends()
{
  for (size_t i = 0; i < attr_extends_refs->get_nof_comps(); i++) {
    ComponentTypeBody *parent = attr_extends_refs->get_nth_comp_body(i);
    // iterate through all definitions of the parent
    // parent is already checked (has it's inherited defs included)
    for (size_t j = 0; j < parent->all_defs_m.size(); j++) {
      Ttcn::Definition *def = parent->all_defs_m.get_nth_elem(j);
      const Identifier& id = def->get_id();
      const char *dispname_str = id.get_dispname().c_str();
      const string& name = id.get_name();
      // the definition to be imported must be already in the all_defs_m map
      // and must belong to owner (not be inherited)
      // orig_defs_map contains backed up own definitions
      if (all_defs_m.has_key(name)) {
        Ttcn::Definition *my_def = all_defs_m[name];
        if (my_def->get_my_scope() == this) {
          // my_def is a local definition
          if (orig_defs_m.has_key(name)) FATAL_ERROR( \
            "ComponentTypeBody::collect_defs_from_attribute_extends()");
          orig_defs_m.add(name, my_def); // backup for init
          all_defs_m[name] = def; // point to inherited def
        } else {
          // my_def comes from another component type
          // exclude case when the same definition is inherited on different
          // paths, there must be a local definition
          if (def!=my_def || !orig_defs_m.has_key(name)) {
            error("Definition `%s' inherited from component type `%s' "
              "collides with definition inherited from `%s'", dispname_str,
              def->get_my_scope()->get_fullname().c_str(),
              my_def->get_my_scope()->get_fullname().c_str());
            def->note("Definition of `%s' in `%s' is here",
              dispname_str, def->get_my_scope()->get_fullname().c_str());
            my_def->note("Definition of `%s' in `%s' is here",
              dispname_str, my_def->get_my_scope()->get_fullname().c_str());
            if (orig_defs_m.has_key(name)) {
              // the same definition is also present locally
              orig_defs_m[name]->note("Local definition of `%s' is here",
                dispname_str);
            }
          }
        }
      } else {
        error("Missing local definition of `%s', which was inherited from "
          "component type `%s'", dispname_str,
          def->get_my_scope()->get_fullname().c_str());
        def->note("Inherited definition of `%s' is here", dispname_str);
      }
    }
  }
}

void ComponentTypeBody::chk_defs_uniq()
{
  if (check_state>=CHECK_DEFUNIQ) return;
  check_state = CHECK_DEFUNIQ;
  if (extends_refs) extends_refs->chk(CHECK_DEFUNIQ);
  if (attr_extends_refs) attr_extends_refs->chk(CHECK_DEFUNIQ);
  size_t nof_defs = def_v.size();
  if (nof_defs > 0) {
    Error_Context cntxt(this, "While checking uniqueness of component "
      "element definitions");
    Assignments *module_defs = parent_scope->get_scope_asss();
    // add own definitions to all_def_m
    for (size_t i = 0; i < nof_defs; i++) {
      Ttcn::Definition *def = def_v[i];
      const Identifier& id = def->get_id();
      const string& name = id.get_name();
      if (all_defs_m.has_key(name)) {
        const char *dispname_str = id.get_dispname().c_str();
        def->error("Duplicate definition with name `%s'", dispname_str);
        all_defs_m[name]->note("Previous definition of `%s' is here",
          dispname_str);
      } else {
        all_defs_m.add(name, def);
        if (module_defs->has_local_ass_withId(id)) {
          const char *dispname_str = id.get_dispname().c_str();
          def->error("Component element name `%s' is not unique in the "
            "scope hierarchy", dispname_str);
          module_defs->get_local_ass_byId(id)->note("Symbol `%s' is "
            "already defined here in a higher scope unit", dispname_str);
        } else if (module_defs->is_valid_moduleid(id)) {
          def->warning("Definition with name `%s' hides a module identifier",
            id.get_dispname().c_str());
        }
      }
    }
  }
  if (extends_refs || attr_extends_refs) {
    // collect all inherited definitions
    Error_Context cntxt(this, "While checking uniqueness of inherited "
      "component element definitions");
    if (extends_refs) collect_defs_from_standard_extends();
    if (attr_extends_refs) collect_defs_from_attribute_extends();
  }
}

void ComponentTypeBody::chk_my_defs()
{
  if (check_state>=CHECK_OWNDEFS) return;
  check_state = CHECK_OWNDEFS;
  Error_Context cntxt(this, "In component element definitions");
  for (size_t i = 0; i < def_v.size(); i++) def_v[i]->chk();
}

void ComponentTypeBody::chk_attr_ext_defs()
{
  if (check_state>=CHECK_EXTDEFS) return;
  check_state = CHECK_EXTDEFS;
  size_t nof_inherited_defs = orig_defs_m.size();
  if (nof_inherited_defs > 0) {
    Error_Context cntxt(this, "While checking whether the local and "
      "inherited component element definitions are identical");
    // for definitions inherited with attribute extends check if
    // local and inherited definitions are identical
    for (size_t i = 0; i < nof_inherited_defs; i++) {
      const string& key = orig_defs_m.get_nth_key(i);
      Ttcn::Definition *original_def = orig_defs_m.get_nth_elem(i);
      Ttcn::Definition *inherited_def = all_defs_m[key];
      if (!original_def->chk_identical(inherited_def))
        all_defs_m[key] = original_def;
    }
  }
}

void ComponentTypeBody::chk(check_state_t required_check_state)
{
  if (checking || check_state==CHECK_FINAL) return;
  checking = true;
  switch (check_state)
  {
  case CHECK_INITIAL:
    chk_extends_attr();
    if (required_check_state==CHECK_WITHATTRIB) break;
    // no break
  case CHECK_WITHATTRIB:
    chk_extends();
    if (required_check_state==CHECK_EXTENDS) break;
    // no break
  case CHECK_EXTENDS:
    chk_defs_uniq();
    if (required_check_state==CHECK_DEFUNIQ) break;
    // no break
  case CHECK_DEFUNIQ:
    chk_my_defs();
    if (required_check_state==CHECK_OWNDEFS) break;
    // no break
  case CHECK_OWNDEFS:
    chk_attr_ext_defs();
    if (required_check_state==CHECK_EXTDEFS) break;
    // no break
  case CHECK_EXTDEFS:
    check_state = CHECK_FINAL;
    // no break
  case CHECK_FINAL:
    break;
  default:
    FATAL_ERROR("ComponentTypeBody::chk()");
  }
  checking = false;
}

void ComponentTypeBody::set_genname(const string& prefix)
{
  for (size_t i = 0; i < def_v.size(); i++) {
    Ttcn::Definition *def = def_v[i];
    def->set_genname(prefix + def->get_id().get_name());
  }
}


void ComponentTypeBody::generate_code(output_struct* target)
{
  if (check_state != CHECK_FINAL || !comp_id)
    FATAL_ERROR("ComponentTypeBody::generate_code()");
  // beginning of block in init_comp
  target->functions.init_comp = mputprintf(target->functions.init_comp,
    "if (!strcmp(component_type, \"%s\")) {\n",
    comp_id->get_dispname().c_str());

  // call init_comp for all components which are compatible
  // with this component along standard extends paths
  if (extends_refs) {
    bool has_base_comps = false;
    for (size_t i = 0; i < compatible_m.size(); i++) {
      ComponentTypeBody *cb = compatible_m.get_nth_elem(i);
      // recursive initialization is needed if the component type is
      // inherited using the standard "extends" mechanism and the component
      // type has its own definitions
      if (cb && cb->def_v.size() > 0) {
        if (!has_base_comps) {
          target->functions.init_comp = mputstr(target->functions.init_comp,
            "if (init_base_comps) {\n");
          has_base_comps = true;
        }
        const char *ct_dispname_str = cb->get_id()->get_dispname().c_str();
        Module *ct_mod = cb->get_scope_mod_gen();
        if (ct_mod == get_scope_mod_gen()) {
          // the other component type is in the same module
          // call the initializer function recursively
          target->functions.init_comp = mputprintf(
            target->functions.init_comp, "init_comp_type(\"%s\", FALSE);\n",
            ct_dispname_str);
        } else {
          // the other component type is imported from another module
          // use the module list for initialization
          target->functions.init_comp = mputprintf(
            target->functions.init_comp,
            "Module_List::initialize_component(\"%s\", \"%s\", FALSE);\n",
            ct_mod->get_modid().get_dispname().c_str(), ct_dispname_str);
        }
      }
    }
    if (has_base_comps) {
      // closing of the if() above
      target->functions.init_comp = mputstr(target->functions.init_comp,
        "}\n");
    }
  }

  // code generation for embedded definitions
  size_t nof_defs = def_v.size();
  for (size_t i = 0; i < nof_defs; i++) {
    Ttcn::Definition *def = def_v[i];
    const string& name = def->get_id().get_name();
    if (orig_defs_m.has_key(name)) {
      // def is hidden by an inherited definiton
      // only an initializer shall be generated
      target->functions.init_comp = def->generate_code_init_comp(
        target->functions.init_comp, all_defs_m[name]);
    } else {
      // def is a normal (non-inherited) definition
      def->generate_code(target, true);
    }
  }

  // end of block in init_comp
  target->functions.init_comp = mputstr(target->functions.init_comp,
    "return TRUE;\n"
    "} else ");
}

char *ComponentTypeBody::generate_code_comptype_name(char *str)
{
  if (!comp_id)
    FATAL_ERROR("ComponentTypeBody::generate_code_comtype_name()");
  return mputprintf(str, "\"%s\", \"%s\"",
    parent_scope->get_scope_mod_gen()->get_modid().get_dispname().c_str(),
    comp_id->get_dispname().c_str());
}

void ComponentTypeBody::set_parent_path(Ttcn::WithAttribPath* p_path) {
  for (size_t i = 0; i < def_v.size(); i++)
    def_v[i]->set_parent_path(p_path);
}

// =================================
// ===== CompTypeRefList
// =================================

CompTypeRefList::~CompTypeRefList()
{
  for (size_t i=0; i<comp_ref_v.size(); i++) delete comp_ref_v[i];
  comp_ref_v.clear();
  comp_body_m.clear();
  comp_body_v.clear();
}

CompTypeRefList *CompTypeRefList::clone() const
{
  FATAL_ERROR("CompTypeRefList::clone()");
}

void CompTypeRefList::add_ref(Ttcn::Reference *p_ref)
{
  if (!p_ref || checked) FATAL_ERROR("CompTypeRefList::add_ref()");
  comp_ref_v.add(p_ref);
}

void CompTypeRefList::dump(unsigned level) const
{
  for(size_t i = 0; i < comp_ref_v.size(); i++) comp_ref_v[i]->dump(level+1);
}

void CompTypeRefList::set_fullname(const string& p_fullname)
{
  Node::set_fullname(p_fullname);
  for (size_t i=0; i<comp_ref_v.size(); i++)
    comp_ref_v[i]->set_fullname(p_fullname + ".<ref" + Int2string(i+1) + ">");
}

void CompTypeRefList::chk_uniq()
{
  if (checked) return;
  checked = true;
  for (size_t i = 0; i < comp_ref_v.size(); i++)
  {
    Ttcn::Reference *ref = comp_ref_v[i];
    Type *type = ref->chk_comptype_ref();
    if (type)
    {
      // add the component body to the map or error if it's duplicate
      ComponentTypeBody *cb = type->get_CompBody();
      if (cb)
      {
        if (comp_body_m.has_key(cb))
        {
          const string& type_name = type->get_typename();
          const char *name = type_name.c_str();
          ref->error("Duplicate reference to component with name `%s'", name);
          comp_body_m[cb]->note("Previous reference to `%s' is here", name);
        }
        else
        {
          // map and vector must always have same content
          comp_body_m.add(cb, ref);
          comp_body_v.add(cb);
        }
      }
    }
  }
}

void CompTypeRefList::chk(ComponentTypeBody::check_state_t
                          required_check_state)
{
  chk_uniq();
  for (size_t i=0; i<comp_body_v.size(); i++)
    comp_body_v[i]->chk(required_check_state);
}

void CompTypeRefList::chk_recursion(ReferenceChain& refch)
{
  for (size_t i = 0; i < comp_body_v.size(); i++) {
    refch.mark_state();
    comp_body_v[i]->chk_recursion(refch);
    refch.prev_state();
  }
}

void CompTypeRefList::set_my_scope(Scope *p_scope)
{
  for (size_t i=0; i<comp_ref_v.size(); i++)
    comp_ref_v[i]->set_my_scope(p_scope);
}

size_t CompTypeRefList::get_nof_comps()
{
  if (!checked) chk_uniq();
  return comp_body_v.size();
}

ComponentTypeBody *CompTypeRefList::get_nth_comp_body(size_t i)
{
  if (!checked) chk_uniq();
  return comp_body_v[i];
}

Ttcn::Reference *CompTypeRefList::get_nth_comp_ref(size_t i)
{
  if (!checked) chk_uniq();
  return comp_body_m[comp_body_v[i]];
}

bool CompTypeRefList::has_comp_body(ComponentTypeBody *cb)
{
  if (!checked) chk_uniq();
  return comp_body_m.has_key(cb);
}

} /* namespace Common */
