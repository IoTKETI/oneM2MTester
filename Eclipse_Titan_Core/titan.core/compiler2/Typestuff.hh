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
#ifndef _Common_Typestuff_HH
#define _Common_Typestuff_HH

#include "Setting.hh"

namespace Asn {
  class TagCollection;
} // namespace Asn

namespace Ttcn {
  class WithAttribPath;
}

namespace Common {
  class CompField;

  /**
   * \addtogroup AST_Type
   *
   * @{
   */

  /**
   * ExceptionSpecification
   */
  class ExcSpec : public Node {
  private:
    Type *type;
    Value *value;
    /** Copy constructor not implemented */
    ExcSpec(const ExcSpec& p);
    /** Assignment disabled */
    ExcSpec& operator=(const ExcSpec& p);
  public:
    ExcSpec(Type *p_type, Value *p_value);
    virtual ~ExcSpec();
    virtual ExcSpec *clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    Type *get_type() const { return type; }
    Value *get_value() const { return value; }
  };

  class CTs_EE_CTs;
  class CTs;
  class ExtAndExc;
  class ExtAdds;
  class ExtAddGrp;
  class ExtAdd;
  class CT;
  class CT_CompsOf;
  class CT_reg;

  /**
   * ComponentTypeList
   */
  class CTs : public Node {
  private:
    vector<CT> cts;
    /** Copy constructor not implemented */
    CTs(const CTs& p);
    /** Assignment disabled */
    CTs& operator=(const CTs& p);
  public:
    CTs() : Node(), cts() { }
    virtual ~CTs();
    virtual CTs *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    size_t get_nof_comps() const;
    CompField* get_comp_byIndex(size_t n) const;
    bool has_comp_withName(const Identifier& p_name) const;
    CompField* get_comp_byName(const Identifier& p_name) const;
    void tr_compsof(ReferenceChain *refch, bool is_set);
    void add_ct(CT* p_ct);
    virtual void dump(unsigned level) const;
  };

  /**
   * ComponentTypeList ExtensionAndException ComponentTypeList
   */
  class CTs_EE_CTs : public Node {
    CTs *cts1;
    ExtAndExc *ee;
    CTs *cts2;
    /** Pointer to the owner type */
    Type *my_type;
    /** Indicates whether the uniqueness of components has been checked */
    bool checked;
    /** Shortcut for all components */
    vector<CompField> comps_v;
    /** Map for all components (indexed by component name) */
    map<string, CompField> comps_m;
    /** Copy constructor not implemented */
    CTs_EE_CTs(const CTs_EE_CTs& p);
    /** Assignment disabled */
    CTs_EE_CTs& operator=(const CTs_EE_CTs& p);
  public:
    CTs_EE_CTs(CTs *p_cts1, ExtAndExc *p_ee, CTs *p_cts2);
    virtual ~CTs_EE_CTs();
    virtual CTs_EE_CTs *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void set_my_type(Type *p_my_type) { my_type = p_my_type; }
    size_t get_nof_comps();
    size_t get_nof_root_comps();
    CompField* get_comp_byIndex(size_t n);
    CompField* get_root_comp_byIndex(size_t n);
    bool has_comp_withName(const Identifier& p_name);
    CompField* get_comp_byName(const Identifier& p_name);
    void tr_compsof(ReferenceChain *refch, bool in_ellipsis);
    bool has_ellipsis() const { return ee != 0; }
    bool needs_auto_tags();
    void add_auto_tags();
    /** Checks the uniqueness of components and builds the shortcut map and
      * vectors */
    void chk();
    void chk_tags();
    virtual void dump(unsigned level) const;
  private:
    void chk_comp_field(CompField *cf, const char *type_name,
        const char *comp_name);
    void chk_tags_choice();
    void chk_tags_seq();
    void chk_tags_seq_comp(Asn::TagCollection& coll, CompField *cf,
        bool is_mandatory);
    void chk_tags_set();
    void get_multiple_tags(Asn::TagCollection& coll, Type *type);
  };

  /**
   * ExtensionAddition (abstract class).
   */
  class ExtAdd : public Node {
  public:
    virtual ExtAdd *clone() const = 0;
    virtual size_t get_nof_comps() const = 0;
    virtual CompField* get_comp_byIndex(size_t n) const = 0;
    virtual bool has_comp_withName(const Identifier& p_name) const = 0;
    virtual CompField* get_comp_byName(const Identifier& p_name) const = 0;
    virtual void tr_compsof(ReferenceChain *refch, bool is_set) = 0;
  };

  /**
   * ExtensionAdditionList
   */
  class ExtAdds : public Node {
  private:
    vector<ExtAdd> eas;
    /** Copy constructor not implemented */
    ExtAdds(const ExtAdds& p);
    /** Assignment disabled */
    ExtAdds& operator=(const ExtAdds& p);
  public:
    ExtAdds() : Node(), eas() { }
    virtual ~ExtAdds();
    virtual ExtAdds *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    size_t get_nof_comps() const;
    CompField* get_comp_byIndex(size_t n) const;
    bool has_comp_withName(const Identifier& p_name) const;
    CompField* get_comp_byName(const Identifier& p_name) const;
    void tr_compsof(ReferenceChain *refch, bool is_set);
    void add_ea(ExtAdd* p_ea);
    virtual void dump(unsigned level) const;
  };

  /**
   * ExtensionAndException
   */
  class ExtAndExc : public Node {
  private:
    /** optional exception specification */
    ExcSpec *excSpec;
    ExtAdds *eas;
    /** Copy constructor not implemented */
    ExtAndExc(const ExtAndExc& p);
    /** Assignment disabled */
    ExtAndExc& operator=(const ExtAndExc& p);
  public:
    ExtAndExc(ExcSpec *p_excSpec, ExtAdds *p_eas=0);
    virtual ~ExtAndExc();
    virtual ExtAndExc *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    size_t get_nof_comps() const { return eas->get_nof_comps(); }
    CompField* get_comp_byIndex(size_t n) const
      { return eas->get_comp_byIndex(n); }
    bool has_comp_withName(const Identifier& p_name) const
      { return eas->has_comp_withName(p_name); }
    CompField* get_comp_byName(const Identifier& p_name) const
      { return eas->get_comp_byName(p_name); }
    void tr_compsof(ReferenceChain *refch, bool is_set)
      { eas->tr_compsof(refch, is_set); }
    void set_eas(ExtAdds *p_eas);
    virtual void dump(unsigned level) const;
  };

  /**
   * ExtensionAdditionGroup
   */
  class ExtAddGrp : public ExtAdd {
  private:
    /** can be NULL if not present */
    Value *versionnumber;
    CTs *cts;
    /** Copy constructor not implemented */
    ExtAddGrp(const ExtAddGrp& p);
    /** Assignment disabled */
    ExtAddGrp& operator=(const ExtAddGrp& p);
  public:
    ExtAddGrp(Value* p_versionnumber, CTs *p_cts);
    virtual ~ExtAddGrp();
    virtual ExtAddGrp *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual size_t get_nof_comps() const;
    virtual CompField* get_comp_byIndex(size_t n) const;
    virtual bool has_comp_withName(const Identifier& p_name) const;
    virtual CompField* get_comp_byName(const Identifier& p_name) const;
    virtual void tr_compsof(ReferenceChain *refch, bool is_set);
    virtual void dump(unsigned level) const;
  };

  /**
   * ComponentType (abstract class).
   */
  class CT : public ExtAdd, public Location {
  public:
    virtual CT *clone() const = 0;
  };

  /**
   * ComponentType/regular (Contains only a Component).
   */
  class CT_reg : public CT {
  private:
    CompField *comp;
    /** Copy constructor not implemented */
    CT_reg(const CT_reg& p);
    /** Assignment disabled */
    CT_reg& operator=(const CT_reg& p);
  public:
    CT_reg(CompField *p_comp);
    virtual ~CT_reg();
    virtual CT_reg *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual size_t get_nof_comps() const;
    virtual CompField* get_comp_byIndex(size_t n) const;
    virtual bool has_comp_withName(const Identifier& p_name) const;
    virtual CompField* get_comp_byName(const Identifier& p_name) const;
    virtual void tr_compsof(ReferenceChain *refch, bool is_set);
    virtual void dump(unsigned level) const;
  };

  /**
   * ComponentsOf
   */
  class CT_CompsOf : public CT {
  private:
    Type *compsoftype;
    bool tr_compsof_ready;
    CTs *cts;
    /** Copy constructor not implemented */
    CT_CompsOf(const CT_CompsOf& p);
    /** Assignment disabled */
    CT_CompsOf& operator=(const CT_CompsOf& p);
  public:
    CT_CompsOf(Type *p_compsoftype);
    virtual ~CT_CompsOf();
    virtual CT_CompsOf *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual size_t get_nof_comps() const;
    virtual CompField* get_comp_byIndex(size_t n) const;
    virtual bool has_comp_withName(const Identifier& p_name) const;
    virtual CompField* get_comp_byName(const Identifier& p_name) const;
    virtual void tr_compsof(ReferenceChain *refch, bool is_set);
    virtual void dump(unsigned level) const;
  };

  /** @} end of AST_Type group */

} // namespace Common

#endif // _Common_Typestuff_HH
