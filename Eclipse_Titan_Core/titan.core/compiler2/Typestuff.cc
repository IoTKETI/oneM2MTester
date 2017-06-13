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
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "Typestuff.hh"
#include "CompField.hh"
#include "asn1/Tag.hh"
#include "main.hh"

namespace Common {

  using Asn::TagDefault;

  // =================================
  // ===== ExcSpec
  // =================================

  ExcSpec::ExcSpec(Type *p_type, Value *p_value)
    : Node()
  {
    if(!p_value)
      FATAL_ERROR("NULL parameter: Asn::ExcSpec::ExcSpec()");
    type=p_type?p_type:new Type(Type::T_INT);
    type->set_ownertype(Type::OT_EXC_SPEC, this);
    value=p_value;
  }

  ExcSpec::ExcSpec(const ExcSpec& p)
    : Node(p)
  {
    type=p.type->clone();
    value=p.value->clone();
  }

  ExcSpec::~ExcSpec()
  {
    delete type;
    delete value;
  }

  ExcSpec *ExcSpec::clone() const
  {
    return new ExcSpec(*this);
  }

  void ExcSpec::set_my_scope(Scope *p_scope)
  {
    type->set_my_scope(p_scope);
    value->set_my_scope(p_scope);
  }

  void ExcSpec::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    type->set_fullname(p_fullname);
    value->set_fullname(p_fullname);
  }

  // =================================
  // ===== CTs
  // =================================

  CTs::~CTs()
  {
    for(size_t i=0; i<cts.size(); i++) delete cts[i];
    cts.clear();
  }

  CTs::CTs(const CTs& p)
  : Node(p)
  {
    for(size_t i=0; i<p.cts.size(); i++)
      cts.add(p.cts[i]->clone());
  }

  CTs *CTs::clone() const
  {
    return new CTs(*this);
  }

  void CTs::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for(size_t i=0; i<cts.size(); i++)
      // cts[i]->set_fullname(get_fullname()+"."+Int2string(i+1));
      cts[i]->set_fullname(get_fullname());
  }

  void CTs::set_my_scope(Scope *p_scope)
  {
    for(size_t i=0; i<cts.size(); i++)
      cts[i]->set_my_scope(p_scope);
  }

  size_t CTs::get_nof_comps() const
  {
    size_t n=0;
    for(size_t i=0; i<cts.size(); i++)
      n+=cts[i]->get_nof_comps();
    return n;
  }

  CompField* CTs::get_comp_byIndex(size_t n) const
  {
    size_t offset = n;
    for(size_t i = 0; i < cts.size(); i++) {
      size_t size = cts[i]->get_nof_comps();
      if (offset < size) return cts[i]->get_comp_byIndex(offset);
      else offset -= size;
    }
    FATAL_ERROR("%s: Requested index %lu does not exist.", \
      get_fullname().c_str(), (unsigned long) n);
    return 0;
  }

  bool CTs::has_comp_withName(const Identifier& p_name) const
  {
    for(size_t i=0; i<cts.size(); i++)
      if(cts[i]->has_comp_withName(p_name)) return true;
    return false;
  }

  CompField* CTs::get_comp_byName(const Identifier& p_name) const
  {
    for(size_t i=0; i<cts.size(); i++)
      if(cts[i]->has_comp_withName(p_name))
        return cts[i]->get_comp_byName(p_name);
    FATAL_ERROR("`%s': No component with name `%s'", \
                get_fullname().c_str(), p_name.get_dispname().c_str());
    return 0;
  }

  void CTs::tr_compsof(ReferenceChain *refch, bool is_set)
  {
    for(size_t i = 0; i < cts.size(); i++)
      cts[i]->tr_compsof(refch, is_set);
  }

  void CTs::add_ct(CT* p_ct)
  {
    if(!p_ct)
      FATAL_ERROR("NULL parameter: Asn::CTs::add_ct()");
    cts.add(p_ct);
  }

  void CTs::dump(unsigned level) const
  {
    for(size_t i=0; i<cts.size(); i++)
      cts[i]->dump(level);
  }

  // =================================
  // ===== CTs_EE_CTs
  // =================================

  CTs_EE_CTs::CTs_EE_CTs(CTs *p_cts1, ExtAndExc *p_ee, CTs *p_cts2)
    : Node(), my_type(0), checked(false)
  {
    cts1 = p_cts1? p_cts1 : new CTs;
    ee = p_ee;
    cts2 = p_cts2 ? p_cts2 : new CTs;
  }

  CTs_EE_CTs::~CTs_EE_CTs()
  {
    delete cts1;
    delete ee;
    delete cts2;
    comps_v.clear();
    comps_m.clear();
  }

  CTs_EE_CTs::CTs_EE_CTs(const CTs_EE_CTs& p)
  : Node(p), my_type(p.my_type), checked(false)
  {
    cts1 = p.cts1->clone();
    ee   = p.ee ? p.ee->clone() : 0;
    cts2 = p.cts2->clone();
  }

  CTs_EE_CTs *CTs_EE_CTs::clone() const
  {
    return new CTs_EE_CTs(*this);
  }

  void CTs_EE_CTs::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    cts1->set_fullname(p_fullname);
    if (ee) ee->set_fullname(p_fullname);
    cts2->set_fullname(p_fullname);
  }

  void CTs_EE_CTs::set_my_scope(Scope *p_scope)
  {
    cts1->set_my_scope(p_scope);
    if (ee) ee->set_my_scope(p_scope);
    cts2->set_my_scope(p_scope);
  }

  size_t CTs_EE_CTs::get_nof_comps()
  {
    if (!checked) chk();
    return comps_v.size();
  }

  size_t CTs_EE_CTs::get_nof_root_comps()
  {
    return cts1->get_nof_comps() + cts2->get_nof_comps();
  }

  CompField* CTs_EE_CTs::get_comp_byIndex(size_t n)
  {
    if (!checked) chk();
    return comps_v[n];
  }

  CompField* CTs_EE_CTs::get_root_comp_byIndex(size_t n)
  {
    size_t cts1_size = cts1->get_nof_comps();
    if (n < cts1_size) return cts1->get_comp_byIndex(n);
    else return cts2->get_comp_byIndex(n - cts1_size);
  }

  bool CTs_EE_CTs::has_comp_withName(const Identifier& p_name)
  {
    if (!checked) chk();
    return comps_m.has_key(p_name.get_name());
  }

  CompField* CTs_EE_CTs::get_comp_byName(const Identifier& p_name)
  {
    if (!checked) chk();
    return comps_m[p_name.get_name()];
  }

  void CTs_EE_CTs::tr_compsof(ReferenceChain *refch, bool in_ellipsis)
  {
    if (!my_type) FATAL_ERROR("NULL parameter: CTs_EE_CTs::tr_compsof()");
    bool is_set = my_type->get_typetype() == Type::T_SET_A;
    if (in_ellipsis) {
      if (ee) ee->tr_compsof(refch, is_set);
    } else {
      cts1->tr_compsof(refch, is_set);
      cts2->tr_compsof(refch, is_set);
    }
  }

  bool CTs_EE_CTs::needs_auto_tags()
  {
    if (!my_type) FATAL_ERROR("NULL parameter: CTs_EE_CTs::needs_auto_tags()");
    Asn::Module *m = dynamic_cast<Asn::Module*>
      (my_type->get_my_scope()->get_scope_mod());
    if (!m) FATAL_ERROR("CTs_EE_CTs::needs_auto_tags()");
    if (m->get_tagdef() != TagDefault::AUTOMATIC) return false;
    for (size_t i = 0; i < cts1->get_nof_comps(); i++) {
      if (cts1->get_comp_byIndex(i)->get_type()->is_tagged())
        return false;
    }
    for (size_t i = 0; i < cts2->get_nof_comps(); i++) {
      if (cts2->get_comp_byIndex(i)->get_type()->is_tagged())
        return false;
    }
    if (ee) {
      bool error_flag = false;
      for (size_t i = 0; i < ee->get_nof_comps(); i++) {
        CompField *cf = ee->get_comp_byIndex(i);
        Type *type = cf->get_type();
        if (type->is_tagged()) {
          type->error("Extension addition `%s' cannot have tags because "
                      "the extension root has no tags",
                      cf->get_name().get_dispname().c_str());
	  error_flag = true;
	}
      }
      if (error_flag) return false;
    }
    return true;
  }

  void CTs_EE_CTs::add_auto_tags()
  {
    Int tagvalue = 0;
    for (size_t i = 0; i < cts1->get_nof_comps(); i++) {
      Tag *tag = new Tag(Tag::TAG_DEFPLICIT, Tag::TAG_CONTEXT, tagvalue++);
      tag->set_automatic();
      cts1->get_comp_byIndex(i)->get_type()->add_tag(tag);
    }
    for (size_t i = 0; i < cts2->get_nof_comps(); i++) {
      Tag *tag = new Tag(Tag::TAG_DEFPLICIT, Tag::TAG_CONTEXT, tagvalue++);
      tag->set_automatic();
      cts2->get_comp_byIndex(i)->get_type()->add_tag(tag);
    }
    if (ee) {
      for (size_t i = 0; i < ee->get_nof_comps(); i++) {
	Tag *tag = new Tag(Tag::TAG_DEFPLICIT, Tag::TAG_CONTEXT, tagvalue++);
	tag->set_automatic();
	ee->get_comp_byIndex(i)->get_type()->add_tag(tag);
      }
    }
  }

  void CTs_EE_CTs::chk()
  {
//  Hack: for COMPONENTS OF transformation
//  Type::chk() should not be called until the transformation is finished,
//  but Type::get_type_refd() calls it from CT_CompsOf::tr_compsof().
//  if (checked) return;
    if (!my_type) FATAL_ERROR("CTs_EE_CTs::chk()");
    checked = true;
    comps_v.clear();
    comps_m.clear();
    const char *type_name;
    const char *comp_name;
    switch (my_type->get_typetype()) {
    case Type::T_SEQ_A:
      type_name = "SEQUENCE";
      comp_name = "component";
      break;
    case Type::T_SET_A:
      type_name = "SET";
      comp_name = "component";
      break;
    case Type::T_CHOICE_A:
      type_name = "CHOICE";
      comp_name = "alternative";
      break;
    default:
      type_name = "<unknown>";
      comp_name = "component";
      break;
    }
    size_t cts1_size = cts1->get_nof_comps();
    for (size_t i = 0; i < cts1_size; i++) {
      chk_comp_field(cts1->get_comp_byIndex(i), type_name, comp_name);
    }
    if (ee) {
      size_t ee_size = ee->get_nof_comps();
      for (size_t i = 0; i < ee_size; i++) {
	chk_comp_field(ee->get_comp_byIndex(i), type_name, comp_name);
      }
    }
    size_t cts2_size = cts2->get_nof_comps();
    for (size_t i = 0; i < cts2_size; i++) {
      chk_comp_field(cts2->get_comp_byIndex(i), type_name, comp_name);
    }
    for (size_t i=0; i<comps_v.size(); i++) {
      CompField *cf=comps_v[i];
      const Identifier& id = cf->get_name();
      const char *dispname = id.get_dispname().c_str();
      Type *type=cf->get_type();
      type->set_genname(my_type->get_genname_own(), id.get_name());
      type->set_parent_type(my_type);
      {
        Error_Context cntxt(cf, "In type of %s %s `%s'",
                            type_name, comp_name, dispname);
        type->chk();
      }
      if(cf->has_default()) {
        Value* defval=cf->get_defval();
        defval->set_my_governor(type);
        Error_Context cntxt(cf, "In default value of %s %s `%s'", type_name,
                            comp_name, dispname);
        type->chk_this_value_ref(defval);
        type->chk_this_value(defval, 0, Type::EXPECTED_CONSTANT,
          INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
	if (!semantic_check_only) {
	  defval->set_genname_prefix("const_");
	  defval->set_genname_recursive(string(type->get_genname_own()) +
	    "_defval_");
	  defval->set_code_section(GovernedSimple::CS_PRE_INIT);
	}
      }
    }
  }

  void CTs_EE_CTs::chk_tags()
  {
    if (!my_type) FATAL_ERROR("NULL parameter: CTs_EE_CTs::chk_tags()");
    switch (my_type->get_typetype()) {
    case Type::T_CHOICE_A:
      chk_tags_choice();
      break;
    case Type::T_SEQ_A:
    case Type::T_SEQ_T:
      chk_tags_seq();
      break;
    case Type::T_SET_A:
    case Type::T_SET_T:
      chk_tags_set();
      break;
    default:
      FATAL_ERROR("CTs_EE_CTs::chk_tags(): invalid typetype");
    }
  }

  void CTs_EE_CTs::chk_comp_field(CompField *cf,
                                  const char *type_name,
                                  const char *comp_name)
  {
    const Identifier& id = cf->get_name();
    const string& name = id.get_name();
    if(comps_m.has_key(name)) {
      const char *dispname_str = id.get_dispname().c_str();
      cf->error("Duplicate %s identifier in %s: `%s'", comp_name, type_name,
        dispname_str);
      comps_m[name]->note("%s `%s' is already defined here", comp_name,
        dispname_str);
    } else {
      comps_m.add(name, cf);
      comps_v.add(cf);
      if(!id.get_has_valid(Identifier::ID_TTCN))
        cf->warning("The identifier `%s' is not reachable from TTCN-3",
                    id.get_dispname().c_str());

    }
  }

  void CTs_EE_CTs::chk_tags_choice()
  {
    TagCollection collection;
    collection.set_location(*my_type);
    for (size_t i = 0; i < cts1->get_nof_comps(); i++) {
      CompField *cf = cts1->get_comp_byIndex(i);
      Type *type = cf->get_type();
      if (type->has_multiple_tags()) {
        Error_Context cntxt(type, "In tags of alternative `%s'",
                            cf->get_name().get_dispname().c_str());
        get_multiple_tags(collection, type);
      }
      else {
        const Tag *tag = type->get_tag();
        if (collection.hasTag(tag))
          type->error("Alternative `%s' in CHOICE has non-distinct tag",
                      cf->get_name().get_dispname().c_str());
        else collection.addTag(tag);
      }
    }
    if (cts2->get_nof_comps() > 0)
      FATAL_ERROR("CTs_EE_CTs::chk_tags_choice(): cts2 is not empty");
    if (ee) {
      collection.setExtensible();
      Tag greatest_tag(Tag::TAG_EXPLICIT, Tag::TAG_NONE, (Int)0);
      for (size_t i = 0; i < ee->get_nof_comps(); i++) {
        CompField *cf = ee->get_comp_byIndex(i);
        Type *type = cf->get_type();
        if (type->has_multiple_tags()) {
          TagCollection coll2;
          coll2.set_location(*type);
          get_multiple_tags(coll2, type);
          if (!coll2.isEmpty()) {
            if (collection.hasTags(&coll2))
              type->error
                ("Alternative `%s' in CHOICE has non-distinct tag(s)",
                 cf->get_name().get_dispname().c_str());
            else collection.addTags(&coll2);
            if (greatest_tag < *coll2.getSmallestTag())
              greatest_tag = *coll2.getGreatestTag();
            else type->error
                   ("Alternative `%s' must have canonically greater tag(s)"
	            " than all previously added extension alternatives",
                    cf->get_name().get_dispname().c_str());
          }
        } else {
          const Tag *tag = type->get_tag();
          if (collection.hasTag(tag))
            type->error("Alternative `%s' in CHOICE has non-distinct tag",
                        cf->get_name().get_dispname().c_str());
          else collection.addTag(tag);
          if (greatest_tag < *tag) greatest_tag = *tag;
          else type->error
                 ("Alternative `%s' must have canonically greater tag"
                  " than all previously added extension alternatives",
                  cf->get_name().get_dispname().c_str());
        }
      }
    }
  }

  void CTs_EE_CTs::chk_tags_seq()
  {
    TagCollection forbidden_tags;
    forbidden_tags.set_location(*my_type);
    for (size_t i = 0; i < cts1->get_nof_comps(); i++) {
      CompField *cf = cts1->get_comp_byIndex(i);
      bool mandatory = !cf->get_is_optional() && !cf->has_default();
      chk_tags_seq_comp(forbidden_tags, cf, mandatory);
    }
    size_t j = 0;
    if (ee) {
      forbidden_tags.setExtensible();
      TagCollection forbidden_tags2;
      forbidden_tags2.set_location(*my_type);
      forbidden_tags2.setExtensible();
      for ( ; j < cts2->get_nof_comps(); j++) {
        CompField *cf = cts2->get_comp_byIndex(j);
        bool mandatory = !cf->get_is_optional() && !cf->has_default();
	chk_tags_seq_comp(forbidden_tags, cf, false);
	chk_tags_seq_comp(forbidden_tags2, cf, false);
	if (mandatory) {
	  j++;
	  break;
	}
      }
      for (size_t i = 0; i < ee->get_nof_comps(); i++) {
	CompField *cf = ee->get_comp_byIndex(i);
        bool mandatory = !cf->get_is_optional() && !cf->has_default();
	chk_tags_seq_comp(forbidden_tags, cf, mandatory);
	if (mandatory) {
	  forbidden_tags.clear();
          forbidden_tags.addTags(&forbidden_tags2);
	}
      }
    }
    forbidden_tags.clear();
    for ( ; j < cts2->get_nof_comps(); j++) {
      CompField *cf = cts2->get_comp_byIndex(j);
      bool mandatory = !cf->get_is_optional() && !cf->has_default();
      chk_tags_seq_comp(forbidden_tags, cf, mandatory);
    }
  }

  void CTs_EE_CTs::chk_tags_seq_comp(TagCollection& coll, CompField *cf,
                                     bool is_mandatory)
  {
    bool is_empty = coll.isEmpty();
    if (!is_mandatory || !is_empty) {
      Type *type = cf->get_type();
      if (type->has_multiple_tags()) {
        Error_Context cntxt(type, "While checking tags of component `%s'",
                            cf->get_name().get_dispname().c_str());
        get_multiple_tags(coll, type);
      }
      else {
        const Tag *tag = type->get_tag();
        if(coll.hasTag(tag))
          type->error("Tag of component `%s' is not allowed "
                      "in this context of SEQUENCE type",
                      cf->get_name().get_dispname().c_str());
        else coll.addTag(tag);
      }
    }
    if (is_mandatory && !is_empty) coll.clear();
  }

  void CTs_EE_CTs::chk_tags_set()
  {
    TagCollection collection;
    collection.set_location(*my_type);
    for (size_t i = 0; i < cts1->get_nof_comps(); i++) {
      CompField *cf = cts1->get_comp_byIndex(i);
      Type *type = cf->get_type();
      if (type->has_multiple_tags()) {
        Error_Context cntxt(type, "While checking tags of component `%s'",
                            cf->get_name().get_dispname().c_str());
        get_multiple_tags(collection, type);
      }
      else {
        const Tag *tag = type->get_tag();
        if (collection.hasTag(tag))
          type->error("Component `%s' in SET has non-distinct tag",
                      cf->get_name().get_dispname().c_str());
        else collection.addTag(tag);
      }
    }
    for (size_t i = 0; i < cts2->get_nof_comps(); i++) {
      CompField *cf = cts2->get_comp_byIndex(i);
      Type *type = cf->get_type();
      if (type->has_multiple_tags()) {
        Error_Context cntxt(type, "While checking tags of component `%s'",
                            cf->get_name().get_dispname().c_str());
        get_multiple_tags(collection, type);
      }
      else {
        const Tag *tag = type->get_tag();
        if (collection.hasTag(tag))
          type->error("Component `%s' in SET has non-distinct tag",
                      cf->get_name().get_dispname().c_str());
        else collection.addTag(tag);
      }
    }
    if (ee) {
      collection.setExtensible();
      Tag greatest_tag(Tag::TAG_EXPLICIT, Tag::TAG_NONE, (Int)0);
      for (size_t i = 0; i < ee->get_nof_comps(); i++) {
        CompField *cf = ee->get_comp_byIndex(i);
        Type *type = cf->get_type();
        if (type->has_multiple_tags()) {
          TagCollection coll2;
          coll2.set_location(*type);
          get_multiple_tags(coll2, type);
          if (!coll2.isEmpty()) {
            if (collection.hasTags(&coll2))
              type->error("Component `%s' in SET has non-distinct tag(s)",
                          cf->get_name().get_dispname().c_str());
            else collection.addTags(&coll2);
            if (greatest_tag < *coll2.getSmallestTag())
              greatest_tag = *coll2.getGreatestTag();
            else type->error
                   ("Component `%s' must have "
                    "canonically greater tag(s) than all previously added "
                    "extension components",
                    cf->get_name().get_dispname().c_str());
          }
        } else {
          const Tag *tag = type->get_tag();
          if (collection.hasTag(tag))
            type->error("Component `%s' in SET has non-distinct tag",
                        cf->get_name().get_dispname().c_str());
          else collection.addTag(tag);
          if (greatest_tag < *tag) greatest_tag = *tag;
          else type->error
                 ("Component `%s' must have canonically greater "
                  "tag than all previously added extension components",
                  cf->get_name().get_dispname().c_str());
        }
      }
    }
  }

  /** \todo revise */
  void CTs_EE_CTs::get_multiple_tags(TagCollection& coll, Type *type)
  {
    Type *t=type->get_type_refd_last();
    if(t->get_typetype()!=Type::T_CHOICE_A)
      FATAL_ERROR("CTs_EE_CTs::get_multiple_tags()");
    map<Type*, void> chain;
    if(my_type->get_typetype()==Type::T_CHOICE_A)
      chain.add(my_type, 0);
    t->get_tags(coll, chain);
    chain.clear();
  }

  void CTs_EE_CTs::dump(unsigned level) const
  {
    if(cts1) cts1->dump(level);
    if(ee) ee->dump(level);
    if(cts2) cts2->dump(level);
  }

  // =================================
  // ===== ExtAdd
  // =================================

  // =================================
  // ===== ExtAdds
  // =================================

  ExtAdds::~ExtAdds()
  {
    for(size_t i=0; i<eas.size(); i++) delete eas[i];
    eas.clear();
  }

  ExtAdds *ExtAdds::clone() const
  {
    FATAL_ERROR("ExtAdds::clone");
  }

  void ExtAdds::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for(size_t i=0; i<eas.size(); i++)
      // eas[i]->set_fullname(get_fullname()+"."+Int2string(i+1));
      eas[i]->set_fullname(get_fullname());
  }

  void ExtAdds::set_my_scope(Scope *p_scope)
  {
    for(size_t i=0; i<eas.size(); i++)
      eas[i]->set_my_scope(p_scope);
  }

  size_t ExtAdds::get_nof_comps() const
  {
    size_t n=0;
    for(size_t i=0; i<eas.size(); i++)
      n+=eas[i]->get_nof_comps();
    return n;
  }

  CompField* ExtAdds::get_comp_byIndex(size_t n) const
  {
    size_t offset = n;
    for(size_t i = 0; i < eas.size(); i++) {
      size_t size = eas[i]->get_nof_comps();
      if (offset < size) return eas[i]->get_comp_byIndex(offset);
      else offset -= size;
    }
    FATAL_ERROR("%s: Requested index %lu does not exist.", \
      get_fullname().c_str(), (unsigned long) n);
    return 0;
  }

  bool ExtAdds::has_comp_withName(const Identifier& p_name) const
  {
    for(size_t i=0; i<eas.size(); i++)
      if(eas[i]->has_comp_withName(p_name)) return true;
    return false;
  }

  CompField* ExtAdds::get_comp_byName(const Identifier& p_name) const
  {
    for(size_t i=0; i<eas.size(); i++)
      if(eas[i]->has_comp_withName(p_name))
        return eas[i]->get_comp_byName(p_name);
    FATAL_ERROR("%s: No component with name `%s'", \
                get_fullname().c_str(), p_name.get_dispname().c_str());
    return 0;
  }

  void ExtAdds::tr_compsof(ReferenceChain *refch, bool is_set)
  {
    for(size_t i = 0; i < eas.size(); i++)
      eas[i]->tr_compsof(refch, is_set);
  }

  void ExtAdds::add_ea(ExtAdd* p_ea)
  {
    if(!p_ea)
      FATAL_ERROR("NULL parameter: Asn::ExtAdds::add_ea()");
    eas.add(p_ea);
  }

  void ExtAdds::dump(unsigned level) const
  {
    for(size_t i=0; i<eas.size(); i++)
      eas[i]->dump(level);
  }

  // =================================
  // ===== ExtAndExc
  // =================================

  ExtAndExc::ExtAndExc(ExcSpec *p_excSpec, ExtAdds *p_eas)
    : Node()
  {
    excSpec=p_excSpec;
    eas=p_eas?p_eas:new ExtAdds();
  }

  ExtAndExc::~ExtAndExc()
  {
    delete excSpec;
    delete eas;
  }

  ExtAndExc *ExtAndExc::clone() const
  {
    FATAL_ERROR("ExtAndExc::clone");
  }

  void ExtAndExc::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if(excSpec) excSpec->set_fullname(p_fullname+".<exc>");
    // eas->set_fullname(p_fullname+".<ext>");
    eas->set_fullname(p_fullname);
  }

  void ExtAndExc::set_my_scope(Scope *p_scope)
  {
    if(excSpec) excSpec->set_my_scope(p_scope);
    eas->set_my_scope(p_scope);
  }

  void ExtAndExc::set_eas(ExtAdds *p_eas)
  {
    delete eas;
    eas=p_eas;
  }

  void ExtAndExc::dump(unsigned level) const
  {
    DEBUG(level, "...%s", excSpec?" !":"");
    if(eas) eas->dump(level);
    DEBUG(level, "...");
  }

  // =================================
  // ===== ExtAddGrp
  // =================================

  ExtAddGrp::ExtAddGrp(Value* p_versionnumber, CTs *p_cts)
    : ExtAdd()
  {
    if(!p_cts)
      FATAL_ERROR("NULL parameter: Asn::ExtAddGrp::ExtAddGrp()");
    versionnumber=p_versionnumber;
    cts=p_cts;
  }

  ExtAddGrp::~ExtAddGrp()
  {
    delete versionnumber;
    delete cts;
  }

  ExtAddGrp *ExtAddGrp::clone() const
  {
    FATAL_ERROR("ExtAddGrp::clone");
  }

  void ExtAddGrp::set_fullname(const string& p_fullname)
  {
    ExtAdd::set_fullname(p_fullname);
    if(versionnumber) versionnumber->set_fullname(p_fullname
                                                  +".<versionnumber>");
    cts->set_fullname(p_fullname);
  }

  void ExtAddGrp::set_my_scope(Scope *p_scope)
  {
    if(versionnumber) versionnumber->set_my_scope(p_scope);
    cts->set_my_scope(p_scope);
  }

  size_t ExtAddGrp::get_nof_comps() const
  {
    return cts->get_nof_comps();
  }

  CompField* ExtAddGrp::get_comp_byIndex(size_t n) const
  {
    return cts->get_comp_byIndex(n);
  }

  bool ExtAddGrp::has_comp_withName(const Identifier& p_name) const
  {
    return cts->has_comp_withName(p_name);
  }

  CompField* ExtAddGrp::get_comp_byName(const Identifier& p_name) const
  {
    return cts->get_comp_byName(p_name);
  }

  void ExtAddGrp::tr_compsof(ReferenceChain *refch, bool is_set)
  {
    cts->tr_compsof(refch, is_set);
  }

  void ExtAddGrp::dump(unsigned level) const
  {
    DEBUG(level, "[[");
    cts->dump(level);
    DEBUG(level, "]]");
  }

  // =================================
  // ===== CT
  // =================================

  // =================================
  // ===== CT_reg
  // =================================

  CT_reg::CT_reg(CompField *p_comp)
    : CT()
  {
    if(!p_comp)
      FATAL_ERROR("NULL parameter: Asn::CT_reg::CT_reg()");
    comp=p_comp;
  }

  CT_reg::~CT_reg()
  {
    delete comp;
  }

  CT_reg *CT_reg::clone() const
  {
    FATAL_ERROR("CT_reg::clone");
  }

  void CT_reg::set_fullname(const string& p_fullname)
  {
    comp->set_fullname(p_fullname);
  }

  void CT_reg::set_my_scope(Scope *p_scope)
  {
    comp->set_my_scope(p_scope);
  }

  size_t CT_reg::get_nof_comps() const
  {
    return 1;
  }

  CompField *CT_reg::get_comp_byIndex(size_t n) const
  {
    if (n == 0) return comp;
    FATAL_ERROR("%s: Requested index %lu does not exist.", \
      get_fullname().c_str(), (unsigned long) n);
    return 0;
  }

  bool CT_reg::has_comp_withName(const Identifier& p_name) const
  {
    return comp->get_name() == p_name;
  }

  CompField *CT_reg::get_comp_byName(const Identifier& p_name) const
  {
    if (comp->get_name() == p_name)
      return comp;
    FATAL_ERROR("`%s': No component with name `%s'", \
                get_fullname().c_str(), p_name.get_dispname().c_str());
    return 0;
  }

  void CT_reg::tr_compsof(ReferenceChain *, bool)
  {
  }

  void CT_reg::dump(unsigned level) const
  {
    comp->dump(level);
  }


  // =================================
  // ===== CT_CompsOf
  // =================================

  CT_CompsOf::CT_CompsOf(Type *p_compsoftype)
    : CT(), compsoftype(p_compsoftype), tr_compsof_ready(false), cts(0)
  {
    if(!p_compsoftype)
      FATAL_ERROR("NULL parameter: Asn::CT_CompsOf::CT_CompsOf()");
    compsoftype->set_ownertype(Type::OT_COMPS_OF, this);
  }

  CT_CompsOf::~CT_CompsOf()
  {
    delete compsoftype;
    delete cts;
  }

  CT_CompsOf *CT_CompsOf::clone() const
  {
    FATAL_ERROR("CT_CompsOf::clone");
  }

  void CT_CompsOf::set_fullname(const string& p_fullname)
  {
    ExtAdd::set_fullname(p_fullname);
    if(compsoftype) compsoftype->set_fullname(p_fullname+".<CompsOfType>");
    if(cts) cts->set_fullname(p_fullname);
  }

  void CT_CompsOf::set_my_scope(Scope *p_scope)
  {
    if (compsoftype) compsoftype->set_my_scope(p_scope);
    if (cts) cts->set_my_scope(p_scope);
  }

  size_t CT_CompsOf::get_nof_comps() const
  {
    if (cts) return cts->get_nof_comps();
    else return 0;
  }

  CompField* CT_CompsOf::get_comp_byIndex(size_t n) const
  {
    if (!cts) FATAL_ERROR("CT_CompsOf::get_comp_byIndex()");
    return cts->get_comp_byIndex(n);
  }

  bool CT_CompsOf::has_comp_withName(const Identifier& p_name) const
  {
    if (cts) return cts->has_comp_withName(p_name);
    else return false;
  }

  CompField* CT_CompsOf::get_comp_byName(const Identifier& p_name) const
  {
    if (!cts) FATAL_ERROR("CT_CompsOf::get_comp_byName()");
    return cts->get_comp_byName(p_name);
  }

  void CT_CompsOf::tr_compsof(ReferenceChain *refch, bool is_set)
  {
    if (tr_compsof_ready) return;
    compsoftype->set_genname(string("<dummy>"));
    Type *t=compsoftype->get_type_refd_last();
    // to avoid re-entering in case of infinite recursion
    if (tr_compsof_ready) return;
    bool error_flag = true;
    if(is_set) {
      switch (t->get_typetype()) {
      case Type::T_SET_A:
        error_flag = false;
        break;
      case Type::T_ERROR:
	break;
      case Type::T_SEQ_A:
        error_flag = false;
        // no break
      default:
        t->error("COMPONENTS OF in a SET type shall refer to another SET type"
          " instead of `%s'", t->get_fullname().c_str());
        break;
      }
    } else {
      switch (t->get_typetype()) {
      case Type::T_SEQ_A:
        error_flag = false;
        break;
      case Type::T_ERROR:
	break;
      case Type::T_SET_A:
        error_flag = false;
        // no break
      default:
        t->error("COMPONENTS OF in a SEQUENCE type shall refer to another"
          " SEQUENCE type instead of `%s'", t->get_fullname().c_str());
        break;
      }
    }
    if (error_flag) {
      tr_compsof_ready = true;
      return;
    }
    t->tr_compsof(refch);
    // another emergency exit for the case of infinite recursion
    if (tr_compsof_ready) return;
    cts=new CTs;
    size_t n_comps=t->get_nof_root_comps();
    for(size_t i=0; i<n_comps; i++) {
      CompField *cf=t->get_root_comp_byIndex(i)->clone();
      cf->get_type()->cut_auto_tags();
      cf->set_location(*this);
      cts->add_ct(new CT_reg(cf));
    }
    cts->set_my_scope(compsoftype->get_my_scope());
    cts->set_fullname(get_fullname());
    tr_compsof_ready=true;
    // compsoftype must not be deleted because the above t->get_type_refd()
    // call may modify it in case of infinite recursion
  }

  void CT_CompsOf::dump(unsigned level) const
  {
    if(compsoftype) {
      DEBUG(level, "COMPONENTS OF");
      compsoftype->dump(level+1);
    }
    if(cts) cts->dump(level);
  }

} // namespace Common
