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
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#ifndef _Asn_Ref_HH
#define _Asn_Ref_HH

#include "AST_asn1.hh"
#include "../Type.hh"

namespace Asn {

  /**
   * \addtogroup AST
   *
   * @{
   */

  using namespace Common;

  class Ref_pard;
  class FieldName;
  class FromObj;

  /**
   * Parameterized reference.
   */
  class Ref_pard : public Ref_defd {
  private:
    Ref_defd_simple *ref_parass; /**< ref. to the par.d ass. */
    Block *block; /**< actual parameter list */
    bool refd_ass_is_not_pard;
    Assignments *asss;
    Ref_defd_simple *ref_ds; /**< name of the instantiated stuff */

    Ref_pard(const Ref_pard& p);
  public:
    Ref_pard(Ref_defd_simple *p_refds, Block *p_block);
    virtual ~Ref_pard();
    /** Virtual constructor. */
    virtual Ref_pard *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual bool get_is_erroneous();
    virtual string get_dispname();
    /** Instantiates the reference. */
    virtual Ref_defd_simple* get_ref_defd_simple();
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent FieldName. FieldName is a sequence of
   * PrimitiveFieldNames.
   */
  class FieldName : public Node {
  private:
    vector<Identifier> fields;

    FieldName(const FieldName& p);
  public:
    FieldName() : Node() { }
    virtual ~FieldName();
    virtual FieldName *clone() const;
    void add_field(Identifier *p_id);
    virtual string get_dispname() const;
    size_t get_nof_fields() const { return fields.size(); }
    Identifier* get_field_byIndex(size_t p_i) const { return fields[p_i]; }
  };

  /**
   * Class to represent InformationFromObjects.
   */
  class FromObj : public Common::Reference {
  private:
    /** ObjectClass, Object or ObjectSet */
    Ref_defd *ref_defd;
    FieldName *fn;
    Setting *setting; /**< contains the ObejctClassFieldType, OpenType or
			   OS_defn that was created in get_refd_setting()
			   and owned by this */
    Setting *setting_cache;

    FromObj(const FromObj& p);
  public:
    FromObj(Ref_defd *p_ref_defd, FieldName *p_fn);
    virtual ~FromObj();
    virtual FromObj *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual bool get_is_erroneous();
    virtual string get_dispname();
    virtual Setting* get_refd_setting();
    virtual Common::Assignment* get_refd_assignment(bool check_parlist = true);
    virtual bool has_single_expr();
    virtual void generate_code          (expression_struct_t *expr);
    virtual void generate_code_const_ref(expression_struct_t *expr);
  };

  /** @} end of AST group */

} // namespace Asn

#endif // _Asn_Ref_HH
