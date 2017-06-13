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
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "Tag.hh"
#include "../Type.hh"
#include "../Value.hh"
#include <limits.h>

namespace Asn {

  // =================================
  // ===== Tag
  // =================================

  Tag::Tag(tagplicit_t p_plicit, tagclass_t p_tagclass, Value *p_tagvalue)
    : Node(), Location(),
      plicit(p_plicit), tagclass(p_tagclass), tagvalue(p_tagvalue),
      is_auto(false)
  {
    if(!p_tagvalue)
      FATAL_ERROR("NULL parameter: Asn::Tag::Tag()");
  }

  Tag::Tag(tagplicit_t p_plicit, tagclass_t p_tagclass, Int p_tagval)
    : Node(), Location(),
      plicit(p_plicit), tagclass(p_tagclass), tagvalue(0),
      tagval(p_tagval), is_auto(false)
  {
    if(tagval<0) FATAL_ERROR("Asn::Tag::Tag(): negative value");
  }

  Tag::Tag(const Tag& p)
    : Node(p), Location(p),
      plicit(p.plicit), tagclass(p.tagclass),
      is_auto(p.is_auto)
  {
    if (p.tagvalue) tagvalue = p.tagvalue->clone();
    else {
      tagvalue = NULL;
      tagval = p.tagval;
    }
  }

  Tag::~Tag()
  {
    delete tagvalue;
  }

  Tag& Tag::operator=(const Tag& p)
  {
    if (&p != this) {
      delete tagvalue;
      plicit = p.plicit;
      tagclass = p.tagclass;
      if (p.tagvalue) tagvalue = p.tagvalue->clone();
      else {
	tagvalue = NULL;
	tagval = p.tagval;
      }
      is_auto = p.is_auto;
    }
    return *this;
  }

  bool Tag::operator==(const Tag& p) const
  {
    if (tagvalue || p.tagvalue) FATAL_ERROR("Comparison of unchecked tags.");
    return tagclass == p.tagclass && tagval == p.tagval;
  }

  bool Tag::operator<(const Tag& p) const
  {
    if (tagvalue || p.tagvalue) FATAL_ERROR("Comparison of unchecked tags.");
    if (tagclass < p.tagclass) return true;
    else if (tagclass > p.tagclass) return false;
    else return tagval < p.tagval;
  }

  const char *Tag::get_tagclass_str() const
  {
    switch (tagclass) {
    case TAG_UNIVERSAL:
      return "ASN_TAG_UNIV";
    case TAG_APPLICATION:
      return "ASN_TAG_APPL";
    case TAG_CONTEXT:
      return "ASN_TAG_CONT";
    case TAG_PRIVATE:
      return "ASN_TAG_PRIV";
    default:
      FATAL_ERROR("Tag::get_tagclass_str()");
      return NULL;
    }
  }

  Int Tag::get_tagvalue()
  {
    chk();
    return tagval;
  }

  void Tag::set_tagvalue(const Int& p_tagval)
  {
    if(tagvalue) {delete tagvalue; tagvalue=0;}
    tagval=p_tagval;
  }

  void Tag::set_my_scope(Scope *p_scope)
  {
    if (tagvalue) tagvalue->set_my_scope(p_scope);
  }

  void Tag::chk()
  {
    if (!tagvalue) return;
    Error_Context cntxt(this, "In tag");
    Value *v = tagvalue->get_value_refd_last();
    switch (v->get_valuetype()) {
    case Value::V_INT: {
      const int_val_t *tagval_int = tagvalue->get_val_Int();
      if (*tagval_int < 0 || *tagval_int > INT_MAX) {
        error("Integer value in range 0..%d was expected instead of `%s' "
          "for tag value", INT_MAX, (tagval_int->t_str()).c_str());
        goto error;
      }
      tagval = tagval_int->get_val();
      break; }
    case Value::V_ERROR:
      goto error;
      break;
    default:
      error("INTEGER value was expected for tag value");
      goto error;
    }
    goto end;
  error:
    tagclass=TAG_ERROR;
    tagval=0;
  end:
    delete tagvalue;
    tagvalue = 0;
  }

  void Tag::dump(unsigned level) const
  {
    string s('[');
    switch(tagclass) {
    case TAG_ERROR:
      s+="<ERROR> "; break;
    case TAG_NONE:
      s+="<NONE> "; break;
    case TAG_ALL:
      s+="<ALL> "; break;
    case TAG_UNIVERSAL:
      s+="UNIVERSAL "; break;
    case TAG_APPLICATION:
      s+="APPLICATION "; break;
    case TAG_CONTEXT:
      break;
    case TAG_PRIVATE:
      s+="PRIVATE "; break;
    default:
      s+=" <UNKNOWN CLASS>"; break;
    } // switch tagclass
    if(!tagvalue) s+=Int2string(tagval);
    else s+="<value>";
    s+="]";
    switch(plicit) {
    case TAG_DEFPLICIT:
      break;
    case TAG_EXPLICIT:
      s+=" EXPLICIT"; break;
    case TAG_IMPLICIT:
      s+=" IMPLICIT"; break;
    default:
      s+=" <UNKNOWN PLICIT>"; break;
    } // switch plicit
    if(is_auto) s+=" (auto)";
    DEBUG(level, "Tag: %s", s.c_str());
    if(tagvalue) tagvalue->dump(level+1);
  }

  // =================================
  // ===== Tags
  // =================================

  Tags::Tags() : Node()
  {
    set_fullname(string("<tags>"));
    my_scope=0;
  }

  Tags::Tags(const Tags& p) : Node(p)
  {
    for(size_t i=0; i<p.tags.size(); i++)
      add_tag(p.tags[i]->clone());
    my_scope=0;
  }

  Tags::~Tags()
  {
    for(size_t i=0; i<tags.size(); i++) delete tags[i];
    tags.clear();
  }

  void Tags::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for(size_t i=0; i<tags.size(); i++)
      tags[i]->set_fullname(get_fullname()+"."+Int2string(i));
  }

  void Tags::set_my_scope(Scope *p_scope)
  {
    my_scope=p_scope;
    for(size_t i=0; i<tags.size(); i++)
      tags[i]->set_my_scope(p_scope);
  }

  void Tags::add_tag(Tag *p_tag)
  {
    if(!p_tag)
      FATAL_ERROR("NULL parameter: Asn::Tags::add_tag()");
    tags.add(p_tag);
    p_tag->set_fullname(get_fullname()+"."+Int2string(tags.size()));
    p_tag->set_my_scope(my_scope);
  }

  void Tags::chk()
  {
    for(size_t i=0; i<tags.size(); i++)
      tags[i]->chk();
  }

  void Tags::set_plicit(Type *p_type)
  {
    if (!p_type) FATAL_ERROR("NULL parameter: Asn::Tags::set_plicit()");
    Asn::Module *m = dynamic_cast<Asn::Module*>
      (p_type->get_my_scope()->get_scope_mod());
    if (!m) FATAL_ERROR("Asn::Tags::set_plicit()");
    bool type_needs_explicit = p_type->needs_explicit_tag();
    bool module_uses_explicit = (m->get_tagdef() == TagDefault::EXPLICIT);
    Tag *innermost = tags[0];
    switch (innermost->get_plicit()) {
    case Tag::TAG_DEFPLICIT:
      if (module_uses_explicit || type_needs_explicit)
        innermost->set_plicit(Tag::TAG_EXPLICIT);
      else innermost->set_plicit(Tag::TAG_IMPLICIT);
      break;
    case Tag::TAG_IMPLICIT:
      if (type_needs_explicit) {
        p_type->error("Type cannot have IMPLICIT tag");
        innermost->set_plicit(Tag::TAG_EXPLICIT);
      }
    default:
      break;
    }
    for (size_t i = 1; i < tags.size(); i++) {
      Tag *tag = tags[i];
      if (tag->get_plicit() == Tag::TAG_DEFPLICIT) {
	if (module_uses_explicit) tag->set_plicit(Tag::TAG_EXPLICIT);
	else tag->set_plicit(Tag::TAG_IMPLICIT);
      }
    }
  }

  void Tags::cut_auto_tags()
  {
    size_t i = 0;
    while (i < tags.size()) {
      Tag *tag = tags[i];
      if (tag->is_automatic()) {
	delete tag;
	tags.replace(i, 1, NULL);
      } else i++;
    }
  }

  void Tags::dump(unsigned level) const
  {
    for(size_t i=0; i<tags.size(); i++)
      tags[i]->dump(level);
  }

  // =================================
  // ===== TagCollection
  // =================================

  TagCollection::TagCollection()
    : has_all(NULL), is_extensible(false)
  {
  }

  TagCollection::TagCollection(const TagCollection& p)
    : Node(p), Location(p),
      has_all(p.has_all), is_extensible(p.is_extensible)
  {
    for (size_t i = 0; i < p.tag_map.size(); i++)
      tag_map.add(p.tag_map.get_nth_key(i), 0);
  }

  TagCollection::~TagCollection()
  {
    tag_map.clear();
  }

  void TagCollection::addTag(const Tag *p_tag)
  {
    if (!p_tag)
      FATAL_ERROR("Asn::TagCollection::addTag()");
    if (p_tag->get_tagclass() == Tag::TAG_ALL) {
      if (has_all)
        FATAL_ERROR("Asn::TagCollection::addTag(): tag 'all'");
      has_all = p_tag;
    }
    else if(p_tag->get_tagclass() == Tag::TAG_ERROR)
      return;
    else if(!tag_map.has_key(*p_tag))
      tag_map.add(*p_tag, 0);
    else
      FATAL_ERROR("Asn::TagCollection::addTag(): tag is already in collection");
  }

  bool TagCollection::hasTag(const Tag *p_tag) const
  {
    if (!p_tag) FATAL_ERROR("NULL parameter: Asn::TagCollection::hasTag()");
    if (has_all) return true;
    else if (p_tag->get_tagclass() == Tag::TAG_ALL) return !tag_map.empty();
    else if (p_tag->get_tagclass() == Tag::TAG_ERROR) return false;
    else return tag_map.has_key(*p_tag);
  }

  void TagCollection::addTags(const TagCollection *p_tags)
  {
    if (!p_tags) FATAL_ERROR("NULL parameter: Asn::TagCollection::addTags()");
    if (p_tags->is_extensible) setExtensible();
    if (p_tags->has_all) {
      if (has_all) FATAL_ERROR("Asn::TagCollection::addTags(): tag 'all'");
      has_all = p_tags->has_all;
    }
    for (size_t i = 0; i < p_tags->tag_map.size(); i++) {
      const Tag& tag = p_tags->tag_map.get_nth_key(i);
      if (!tag_map.has_key(tag)) tag_map.add(tag, 0);
      else FATAL_ERROR("Asn::TagCollection::addTags(): tag is already in collection");
    }
  }

  bool TagCollection::hasTags(const TagCollection *p_tags) const
  {
    if (!p_tags) FATAL_ERROR("NULL parameter: Asn::TagCollection::hasTags()");
    if (has_all) return !p_tags->isEmpty();
    else if (p_tags->has_all) return !isEmpty();
    for (size_t i = 0; i < p_tags->tag_map.size(); i++)
      if (tag_map.has_key(p_tags->tag_map.get_nth_key(i))) return true;
    return false;
  }

  bool TagCollection::greaterTags(const TagCollection *p_tags) const
  {
    if (!p_tags)
      FATAL_ERROR("NULL parameter: Asn::TagCollection::greaterTags()");
    if (has_all) return !p_tags->isEmpty();
    else if (p_tags->has_all) return !isEmpty();
    else if (tag_map.empty() || p_tags->tag_map.empty()) return true;
    else return tag_map.get_nth_key(tag_map.size() - 1) <
      p_tags->tag_map.get_nth_key(0);
  }

  const Tag *TagCollection::getSmallestTag() const
  {
    if (has_all) return has_all;
    if (tag_map.empty()) FATAL_ERROR("TagCollection::getSmallestTag()");
    return &tag_map.get_nth_key(0);
  }

  const Tag *TagCollection::getGreatestTag() const
  {
    if (has_all) return has_all;
    if (tag_map.empty()) FATAL_ERROR("TagCollection::getGreatestTag()");
    return &tag_map.get_nth_key(tag_map.size() - 1);
  }

  void TagCollection::setExtensible()
  {
    if(is_extensible)
      error("Illegal use of extensibility notation (possible tag conflict)");
    else is_extensible=true;
  }

  bool TagCollection::isEmpty() const
  {
    if (has_all || is_extensible) return false;
    else return tag_map.empty();
  }

  void TagCollection::clear()
  {
    tag_map.clear();
    has_all = NULL;
    is_extensible = false;
  }

} // namespace Asn
