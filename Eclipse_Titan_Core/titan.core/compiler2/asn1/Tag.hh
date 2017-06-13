/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Asn_Tag_HH
#define _Asn_Tag_HH

#include "../Int.hh"
#include "AST_asn1.hh"
#include "../Code.hh"

namespace Asn {

  using namespace Common;

  /**
   * Class to represent ASN tag.
   */
  class Tag : public Node, public Location {
  public:
    enum tagplicit_t {TAG_DEFPLICIT, TAG_EXPLICIT, TAG_IMPLICIT};
    enum tagclass_t {
      TAG_ERROR, TAG_NONE,
      TAG_ALL,
      TAG_UNIVERSAL,
      TAG_APPLICATION,
      TAG_CONTEXT,
      TAG_PRIVATE
    };
  private:
    tagplicit_t plicit;
    tagclass_t tagclass;
    Value *tagvalue;
    Int tagval;
    bool is_auto;

  public:
    Tag(tagplicit_t p_plicit, tagclass_t p_tagclass, Value *p_tagvalue);
    Tag(tagplicit_t p_plicit, tagclass_t p_tagclass, Int p_tagval);
    Tag(const Tag& p);
    virtual ~Tag();
    virtual Tag* clone() const
    {return new Tag(*this);}
    Tag& operator=(const Tag& p);
    bool operator==(const Tag& p) const;
    bool operator<(const Tag& p) const;
    tagplicit_t get_plicit() const {return plicit;}
    void set_plicit(tagplicit_t p_plicit) {plicit=p_plicit;}
    tagclass_t get_tagclass() const {return tagclass;}
    const char *get_tagclass_str() const;
    void set_tagclass(tagclass_t p_tagclass) {tagclass=p_tagclass;}
    bool is_automatic() const { return is_auto; }
    void set_automatic() {is_auto = true;}
    Int get_tagvalue();
    void set_tagvalue(const Int& p_tagval);
    void set_my_scope(Scope *p_scope);
    void chk();
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent a series of tags applied to a type. Index 0 is
   * the innermost tag.
   */
  class Tags : public Node {
  private:
    Scope *my_scope;
    /** tags */
    vector<Tag> tags;

    Tags(const Tags&);
  public:
    Tags();
    virtual ~Tags();
    virtual Tags* clone() const
    {return new Tags(*this);}
    virtual void set_fullname(const string& p_fullname);
    void set_my_scope(Scope *p_scope);
    void add_tag(Tag *p_tag);
    size_t get_nof_tags() const
    {return tags.size();}
    Tag* get_tag_byIndex(const size_t p_i)
    {return tags[p_i];}
    void chk();
    void set_plicit(Type *p_type);
    void cut_auto_tags();
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent a collection of tags. This can be used to
   * check tag clashes, "monoton increment"-requirements of tags etc.
   */
  class TagCollection : public Node, public Location {
  private:
    map<Tag, void> tag_map;
    const Tag *has_all;
    bool is_extensible;

    TagCollection(const TagCollection& p);
  public:
    TagCollection();
    virtual ~TagCollection();
    virtual TagCollection* clone() const
    {return new TagCollection(*this);}
    /** Adds the tag to the collection. p_tag is copied; the
     * tagcollection does not become the owner of p_tag. */
    void addTag(const Tag *p_tag);
    bool hasTag(const Tag *p_tag) const;
    /** Adds (copies) every tag of p_tags to this. */
    void addTags(const TagCollection *p_tags);
    bool hasTags(const TagCollection *p_tags) const;
    /** Return true if all the tags of p_tags are greater than all the
     * tags of this. */
    bool greaterTags(const TagCollection *p_tags) const;
    /** Returns the smallest or largest tag of the collection.
     * Applicable only of the collection is not empty. */
    const Tag *getSmallestTag() const;
    const Tag *getGreatestTag() const;
    bool isExtensible() const { return is_extensible; }
    void setExtensible();
    bool isEmpty() const;
    /** Makes it empty. */
    void clear();
  };

} // namespace Asn

#endif // _Asn_Tag_HH
