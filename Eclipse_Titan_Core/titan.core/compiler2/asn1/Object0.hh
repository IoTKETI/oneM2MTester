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
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Asn_Object0_HH
#define _Asn_Object0_HH

#include "../Setting.hh"
#include "../Code.hh"

namespace Asn {

  using namespace Common;

  /**
   * \addtogroup AST
   *
   * @{
   */

  class Ref_defd;
  class Ref_defd_simple;

  // not defined here
  class Assignment;

  /**
   * Abstract parent for defined references.
   */
  class Ref_defd : public Common::Ref_simple {
  public:
    virtual Ref_defd* clone() const =0;
    virtual Ref_defd_simple* get_ref_defd_simple() =0;
    virtual const Identifier* get_modid();
    virtual const Identifier* get_id();
    /** Appends error messages when necessary. */
    virtual bool refers_to_st(Setting::settingtype_t p_st,
                              ReferenceChain* refch=0);
    virtual void generate_code          (expression_struct_t *expr);
    virtual void generate_code_const_ref(expression_struct_t *expr);
  };

  /**
   * Class to represent simple defined references to entities.
   */
  class Ref_defd_simple : public Ref_defd {
  private:
    Identifier *modid;
    Identifier *id;

    /** The copy constructor. For clone() only. */
    Ref_defd_simple(const Ref_defd_simple& p);
    /** Assignment disabled */
    Ref_defd_simple& operator=(const Ref_defd_simple& p);
  public:
    /** Constructs a new reference. \a p_modid can be 0. */
    Ref_defd_simple(Identifier *p_modid, Identifier *p_id);
    virtual ~Ref_defd_simple();
    /** Virtual constructor. */
    virtual Ref_defd_simple* clone() const
    {return new Ref_defd_simple(*this);}
    virtual Ref_defd_simple* get_ref_defd_simple() {return this;}
    /** Returns the \a modid, or 0 if not present. */
    virtual const Identifier* get_modid() {return modid;}
    /** Returns the \a id. */
    virtual const Identifier* get_id() {return id;}
    /** Returns the referenced assigment. Appends an error message
     * and returns 0 when it cannot resolve the reference */
    Assignment* get_refd_ass();
  };

  /**
   * \defgroup AST_Object Object
   *
   * @{
   */

  class ObjectClass;
  class OSE_Visitor;
  class OS_Element;
  class Object;
  class ObjectSet;

  // not defined here
  class OC_defn;
  class OCS_root;
  class FieldSpecs;
  class Obj_defn;
  class OSEV_objcollctr;
  class OS_defn;
  class OS_refd;

  /**
   * Class to represent ObjectClass.
   */
  class ObjectClass : public Governor {
  protected: // OC_defn and OC_refd need access to their parent
    ObjectClass(const ObjectClass& p) : Governor(p) {}
  public:
    ObjectClass() : Governor(S_OC) {}
    virtual ObjectClass* clone() const =0;
    virtual OC_defn* get_refd_last(ReferenceChain *refch=0) =0;
    virtual void chk_this_obj(Object *obj) =0;
    virtual OCS_root* get_ocs() =0;
    virtual FieldSpecs* get_fss() =0;
    virtual void generate_code(output_struct *target) =0;
    virtual void dump(unsigned level) const =0;
  };

  /**
   * ObjectSetElement visitor
   */
  class OSE_Visitor : public Node {
  protected: // Three derived classes need access to loc
    const Location *loc;
  private:
    /** Copy constructor not implemented */
    OSE_Visitor(const OSE_Visitor& p);
    /** Assignment not implemented */
    OSE_Visitor& operator=(const OSE_Visitor& p);
  public:
    OSE_Visitor(const Location *p_loc) : Node(), loc(p_loc) {}
    virtual OSE_Visitor* clone() const;
    virtual void visit_Object(Object& p) =0;
    virtual void visit_OS_refd(OS_refd& p) =0;
  };

  /**
   * Something that can be in an ObjectSet.
   */
  class OS_Element {
  public:
    virtual ~OS_Element();
    virtual OS_Element *clone_ose() const = 0;
    virtual void accept(OSE_Visitor& v) = 0;
    virtual void set_fullname_ose(const string& p_fullname) = 0;
    virtual void set_genname_ose(const string& p_prefix,
				 const string& p_suffix) = 0;
    virtual void set_my_scope_ose(Scope *p_scope) = 0;
  };

  /**
   * Class to represent Object.
   */
  class Object : public Governed, public OS_Element {
  protected: // Obj_defn and Obj_refd need access
    ObjectClass *my_governor;
    bool is_erroneous;

    Object(const Object& p)
      : Governed(p), OS_Element(p), my_governor(0),
    is_erroneous(p.is_erroneous) {}
  private:
    Object& operator=(const Object& p);
  public:
    Object() : Governed(S_O), OS_Element(), my_governor(0),
    is_erroneous(false) {}
    virtual Object *clone() const = 0;
    /** Sets the governor ObjectClass. */
    virtual void set_my_governor(ObjectClass *p_gov);
    /** Gets the governor ObjectClass. */
    virtual ObjectClass* get_my_governor() const {return my_governor;}
    virtual Obj_defn* get_refd_last(ReferenceChain *refch=0) =0;
    virtual void chk() =0;
    void set_is_erroneous() {is_erroneous=true;}
    bool get_is_erroneous() {return is_erroneous;}
    virtual size_t get_nof_elems() {return 1;}
    virtual void accept(OSE_Visitor& v) {v.visit_Object(*this);}
    virtual void generate_code(output_struct *target) = 0;
    virtual OS_Element *clone_ose() const;
    virtual void set_fullname_ose(const string& p_fullname);
    virtual void set_genname_ose(const string& p_prefix,
				 const string& p_suffix);
    virtual void set_my_scope_ose(Scope *p_scope);
  };

  /**
   * Class to represent ObjectSet.
   */
  class ObjectSet : public GovdSet {
  protected: // OS_defn and OS_refd need access
    ObjectClass *my_governor;

    ObjectSet(const ObjectSet& p) : GovdSet(p), my_governor(0)
    {}
  private:
    ObjectSet& operator=(const ObjectSet& p);
  public:
    ObjectSet() : GovdSet(S_OS), my_governor(0) {}
    virtual ObjectSet* clone() const =0;
    virtual void set_my_governor(ObjectClass *p_gov);
    virtual ObjectClass* get_my_governor() const {return my_governor;}
    virtual OS_defn* get_refd_last(ReferenceChain *refch=0) =0;
    virtual size_t get_nof_objs() =0;
    virtual Object* get_obj_byIndex(size_t p_i) =0;
    virtual void accept(OSEV_objcollctr& v) =0;
    virtual void chk() =0;
    virtual void generate_code(output_struct *target) =0;
  };

  /** @} end of AST_Object group */

  /** @} end of AST group */

} // namespace Asn

#endif // _Asn_Object0_HH
