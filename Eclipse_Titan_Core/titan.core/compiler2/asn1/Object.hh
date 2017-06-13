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
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Asn_Object_HH
#define _Asn_Object_HH

#include "Object0.hh"
#include "../Type.hh"
#include "../Value.hh"
#include "../Code.hh"

namespace Asn {

  using namespace Common;

  class OC_defn;
  class OC_refd;
  class FieldSpec;
  class FieldSpec_Undef;
  class FieldSpec_Error;
  class FieldSpec_T;
  class FieldSpec_V_FT;
  /*
  class FieldSpec_V_VT;
  class FieldSpec_VS_FT;
  class FieldSpec_VS_VT;
  */
  class FieldSpec_O;
  class FieldSpec_OS;

  class FieldSpecs;

  class OCS_Visitor;
  class OCS_Node;
  class OCS_seq;
  class OCS_root;
  class OCS_literal;
  class OCS_setting;

  class FieldSetting;
  class FieldSetting_Type;
  class FieldSetting_Value;
  class FieldSetting_O;
  class FieldSetting_OS;
  class Obj_defn;
  class Obj_refd;

  class Objects;
  class OSE_Visitor; // in Object0.hh
  class OS_defn;
  class OS_refd;

  // not defined here
  class Block;

  /**
   * Class to represent ObjectClassDefinition
   */
  class OC_defn : public ObjectClass {
  private:
    Block *block_fs; /**< field specs block */
    Block *block_ws; /**< WITH SYNTAX block */
    FieldSpecs *fss;  /**< field specs */
    OCS_root *ocs; /**< object class syntax */
    bool is_erroneous;
    bool is_generated;

    OC_defn(const OC_defn& p);
  public:
    OC_defn();
    OC_defn(Block *p_block_fs, Block *p_block_ws);
    virtual ~OC_defn();
    virtual OC_defn* clone() const {return new OC_defn(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual OC_defn* get_refd_last(ReferenceChain * =0) {return this;}
    virtual void chk();
    virtual void chk_this_obj(Object *obj);
    virtual OCS_root* get_ocs();
    virtual FieldSpecs* get_fss();
    virtual void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
  private:
    void parse_block_fs();
  };

  /**
   * Class to represent a ReferencedObjectClass. It is a DefinedOC or
   * OCFromObject or ValueSetFromObjects.
   */
  class OC_refd : public ObjectClass {
  private:
    Reference *ref;
    OC_defn *oc_error;
    ObjectClass *oc_refd; /**< cache */
    OC_defn *oc_defn; /**< cache */

    OC_refd(const OC_refd& p);
  public:
    OC_refd(Reference *p_ref);
    virtual ~OC_refd();
    virtual OC_refd* clone() const {return new OC_refd(*this);}
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    ObjectClass* get_refd(ReferenceChain *refch=0);
    virtual OC_defn* get_refd_last(ReferenceChain *refch=0);
    virtual void chk();
    virtual void chk_this_obj(Object *p_obj);
    virtual OCS_root* get_ocs();
    virtual FieldSpecs* get_fss();
    virtual void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent a FieldSpec of an ObjectClass.
   */
  class FieldSpec : public Node, public Location {
  public:
    enum fstype_t {
      FS_UNDEF, /**< undefined */
      FS_ERROR, /**< erroneous fieldspec */
      FS_T, /**< type fieldspec */
      FS_V_FT, /**< fixedtype value fieldspec */
      FS_V_VT, /**< variabletype value fieldspec */
      FS_VS_FT, /**< fixedtype valueset fieldspec */
      FS_VS_VT, /**< variabletype valueset fieldspec */
      FS_O, /**< object fieldspec */
      FS_OS /**< objectset fieldspec */
    };
  protected: // Several derived classes need access
    fstype_t fstype;
    Identifier *id;
    bool is_optional;
    OC_defn *my_oc;
    bool checked;

    FieldSpec(const FieldSpec& p);
  public:
    FieldSpec(fstype_t p_fstype, Identifier *p_id, bool p_is_optional);
    virtual ~FieldSpec();
    virtual FieldSpec* clone() const =0;
    virtual void set_fullname(const string& p_fullname);
    virtual const Identifier& get_id() const {return *id;}
    virtual fstype_t get_fstype() {return fstype;}
    virtual void set_my_oc(OC_defn *p_oc);
    bool get_is_optional() {return is_optional;}
    virtual bool has_default() =0;
    virtual void chk() =0;
    virtual Setting* get_default() =0;
    virtual FieldSpec* get_last() {return this;}
    virtual void generate_code(output_struct *target) =0;
  };

  /**
   * Class to represent an undefined FieldSpec.
   */
  class FieldSpec_Undef : public FieldSpec {
  private:
    Ref_defd* govref;
    Node* defsetting; /**< default setting */
    FieldSpec *fs; /**< the classified fieldspec */

    void classify_fs(ReferenceChain *refch=0);
    FieldSpec_Undef(const FieldSpec_Undef& p);
  public:
    FieldSpec_Undef(Identifier *p_id, Ref_defd* p_govref,
                    bool p_is_optional, Node *p_defsetting);
    virtual ~FieldSpec_Undef();
    virtual FieldSpec* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual fstype_t get_fstype();
    virtual void set_my_oc(OC_defn *p_oc);
    virtual bool has_default();
    virtual void chk();
    virtual Setting* get_default();
    virtual FieldSpec* get_last();
    virtual void generate_code(output_struct *target);
  };

  class FieldSpec_Error : public FieldSpec {
  private:
    Setting *setting_error;
    bool has_default_flag;

    FieldSpec_Error(const FieldSpec_Error& p);
  public:
    FieldSpec_Error(Identifier *p_id, bool p_is_optional,
                    bool p_has_default);
    virtual ~FieldSpec_Error();
    virtual FieldSpec_Error* clone() const;
    virtual bool has_default();
    virtual Setting* get_default();
    virtual void chk();
    virtual void generate_code(output_struct *target);
  };

  /**
   * Class to represent a TypeFieldSpec.
   */
  class FieldSpec_T : public FieldSpec {
  private:
    Type *deftype; /**< default type */

    FieldSpec_T(const FieldSpec_T& p);
  public:
    FieldSpec_T(Identifier *p_id, bool p_is_optional, Type *p_deftype);
    virtual ~FieldSpec_T();
    virtual FieldSpec_T* clone() const
    {return new FieldSpec_T(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_oc(OC_defn *p_oc);
    virtual bool has_default() {return deftype;}
    virtual Type* get_default();
    virtual void chk();
    virtual void generate_code(output_struct *target);
  };

  /**
   * Class to represent a FixedTypeValueFieldSpec.
   */
  class FieldSpec_V_FT : public FieldSpec {
  private:
    Type *fixtype; /**< fixed type */
    bool is_unique;
    Value *defval; /**< default value */

    FieldSpec_V_FT(const FieldSpec_V_FT& p);
  public:
    FieldSpec_V_FT(Identifier *p_id, Type *p_fixtype, bool p_is_unique,
                   bool p_is_optional, Value *p_defval);
    virtual ~FieldSpec_V_FT();
    virtual FieldSpec_V_FT* clone() const
    {return new FieldSpec_V_FT(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_oc(OC_defn *p_oc);
    virtual bool has_default() {return defval;}
    virtual Value* get_default();
    Type* get_type() {return fixtype;}
    virtual void chk();
    virtual void generate_code(output_struct *target);
  };

  /*

  / **
   * Class to represent a VariableTypeValueFieldSpec.
   * /
  class FieldSpec_V_VT : public FieldSpec {
  };

  / **
   * Class to represent a FixedTypeValueSetFieldSpec.
   * /
  class FieldSpec_VS_FT : public FieldSpec {
  };

  / **
   * Class to represent a VariableTypeValueSetFieldSpec.
   * /
  class FieldSpec_VS_VT : public FieldSpec {
  };

  */

  /**
   * Class to represent an ObjectFieldSpec.
   */
  class FieldSpec_O : public FieldSpec {
  private:
    ObjectClass *oc;
    Object *defobj; /**< default object */

    FieldSpec_O(const FieldSpec_O& p);
  public:
    FieldSpec_O(Identifier *p_id, ObjectClass *p_oc,
                bool p_is_optional, Object *p_defobj);
    virtual ~FieldSpec_O();
    virtual FieldSpec_O* clone() const
    {return new FieldSpec_O(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_oc(OC_defn *p_oc);
    virtual bool has_default() {return defobj;}
    virtual Object* get_default() {return defobj;}
    ObjectClass* get_oc() {return oc;}
    virtual void chk();
    virtual void generate_code(output_struct *target);
  };

  /**
   * Class to represent an ObjectSetFieldSpec.
   */
  class FieldSpec_OS : public FieldSpec {
  private:
    ObjectClass *oc;
    ObjectSet *defobjset; /**< default objectset */

    FieldSpec_OS(const FieldSpec_OS& p);
  public:
    FieldSpec_OS(Identifier *p_id, ObjectClass *p_oc,
                bool p_is_optional, ObjectSet *p_defobjset);
    virtual ~FieldSpec_OS();
    virtual FieldSpec_OS* clone() const
    {return new FieldSpec_OS(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_oc(OC_defn *p_oc);
    virtual bool has_default() {return defobjset;}
    virtual ObjectSet* get_default() {return defobjset;}
    ObjectClass* get_oc() {return oc;}
    virtual void chk();
    virtual void generate_code(output_struct *target);
  };

  /**
   * Class to represent FieldSpecs.
   */
  class FieldSpecs : public Node {
  private:
    map<string, FieldSpec> fss; /**< the FieldSpecs */
    vector<FieldSpec> fss_v;
    FieldSpec_Error *fs_error;
    OC_defn *my_oc;

    FieldSpecs(const FieldSpecs& p);
  public:
    FieldSpecs();
    virtual ~FieldSpecs();
    virtual FieldSpecs* clone() const
    {return new FieldSpecs(*this);}
    virtual void set_fullname(const string& p_fullname);
    void set_my_oc(OC_defn *p_oc);
    void add_fs(FieldSpec *p_fs);
    bool has_fs_withId(const Identifier& p_id);
    FieldSpec* get_fs_byId(const Identifier& p_id);
    FieldSpec* get_fs_byIndex(size_t p_i);
    FieldSpec* get_fs_error();
    size_t get_nof_fss() {return fss.size();}
    void chk();
    void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
  };

  /**
   * Abstract class for OCS visitors.
   */
  class OCS_Visitor : public Node {
  private:
    OCS_Visitor(const OCS_Visitor& p);
  public:
    OCS_Visitor() : Node() {}
    virtual OCS_Visitor* clone() const;
    virtual void visit_root(OCS_root& p) =0;
    virtual void visit_seq(OCS_seq& p) =0;
    virtual void visit_literal(OCS_literal& p) =0;
    virtual void visit_setting(OCS_setting& p) =0;
  };

  /**
   * ObjectClass Syntax. Class to build, manage, ...  ObjectClass
   * Syntax.
   */
  class OCS_Node : public Node, public Location {
  private:
    bool is_builded;

    /// Copy constructor disabled.
    OCS_Node(const OCS_Node& p);
    /// Assignment disabled.
    OCS_Node& operator=(const OCS_Node& p);
  public:
    OCS_Node() : Location(), is_builded(false) {}
    virtual OCS_Node* clone() const;
    virtual void accept(OCS_Visitor& v) =0;
    bool get_is_builded() {return is_builded;}
    void set_is_builded() {is_builded=true;}
    virtual string get_dispname() const =0;
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent a (perhaps optional) sequence of OCS_Nodes.
   */
  class OCS_seq : public OCS_Node {
  private:
    vector<OCS_Node> ocss;
    bool is_opt;
    /** This is used when default syntax is active. Then, if there is
     * at least one parsed setting in the object, then the seq must
     * begin with a comma (','). */
    bool opt_first_comma;

    /// Copy constructor disabled.
    OCS_seq(const OCS_seq& p);
    /// Assignment disabled.
    OCS_seq& operator=(const OCS_seq& p);
  public:
    OCS_seq(bool p_is_opt, bool p_opt_first_comma=false)
      : OCS_Node(), is_opt(p_is_opt), opt_first_comma(p_opt_first_comma) {}
    virtual ~OCS_seq();
    virtual void accept(OCS_Visitor& v)
    {v.visit_seq(*this);}
    void add_node(OCS_Node *p_node);
    size_t get_nof_nodes() {return ocss.size();}
    OCS_Node* get_nth_node(size_t p_i);
    bool get_is_opt() {return is_opt;}
    bool get_opt_first_comma() {return opt_first_comma;}
    virtual string get_dispname() const;
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent the root of an OCS.
   */
  class OCS_root : public OCS_Node {
  private:
    OCS_seq seq;

    /// Copy constructor disabled.
    OCS_root(const OCS_root& p);
    /// Assignment disabled.
    OCS_root& operator=(const OCS_root& p);
  public:
    OCS_root() : OCS_Node(), seq(false) {}
    virtual void accept(OCS_Visitor& v)
    {v.visit_root(*this);}
    OCS_seq& get_seq() {return seq;}
    virtual string get_dispname() const;
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent a literal element in an OCS.
   */
  class OCS_literal : public OCS_Node {
  private:
    Identifier *word;
    int keyword;

    /// Copy constructor disabled.
    OCS_literal(const OCS_literal& p);
    /// Assignment disabled.
    OCS_literal& operator=(const OCS_literal& p);
  public:
    OCS_literal(int p_keyword) : OCS_Node(), word(0), keyword(p_keyword) {}
    OCS_literal(Identifier *p_word);
    virtual ~OCS_literal();
    virtual void accept(OCS_Visitor& v)
    {v.visit_literal(*this);}
    bool is_keyword() {return !word;}
    int get_keyword() {return keyword;}
    Identifier* get_word() {return word;}
    virtual string get_dispname() const;
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent a Setting in the OCS.
   */
  class OCS_setting : public OCS_Node {
  public:
    enum settingtype_t {
      S_UNDEF, /**< undefined */
      S_T, /**< Type */
      S_V, /**< Value */
      S_VS, /**< ValueSet */
      S_O, /**< Object */
      S_OS /**< ObjectSet */
    };
  private:
    settingtype_t st;
    Identifier *id;

    /// Copy constructor disabled.
    OCS_setting(const OCS_setting& p);
    /// Assignment disabled.
    OCS_setting& operator=(const OCS_setting& p);
  public:
    OCS_setting(settingtype_t p_st, Identifier *p_id);
    virtual ~OCS_setting();
    virtual void accept(OCS_Visitor& v)
    {v.visit_setting(*this);}
    settingtype_t get_st() {return st;}
    Identifier* get_id() {return id;}
    virtual string get_dispname() const;
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent FieldSettings.
   */
  class FieldSetting : public Node, public Location {
  protected: // Several derived classes need access
    Identifier *name;
    bool checked;

    FieldSetting(const FieldSetting& p);
  public:
    FieldSetting(Identifier *p_name);
    virtual ~FieldSetting();
    virtual FieldSetting* clone() const =0;
    virtual void set_fullname(const string& p_fullname);
    Identifier& get_name() {return *name;}
    virtual Setting* get_setting() =0;
    void set_my_scope(Scope *p_scope);
    void set_genname(const string& p_prefix,
                     const string& p_suffix);
    virtual void chk(FieldSpec *p_fspec) =0;
    virtual void generate_code(output_struct *target) =0;
    virtual void dump(unsigned level) const =0;
  };

  /**
   * Class to represent type FieldSettings.
   */
  class FieldSetting_Type : public FieldSetting {
  private:
    Type *setting;

    FieldSetting_Type(const FieldSetting_Type& p);
  public:
    FieldSetting_Type(Identifier *p_name, Type *p_setting);
    virtual ~FieldSetting_Type();
    virtual FieldSetting_Type* clone() const
    {return new FieldSetting_Type(*this);}
    virtual Type* get_setting() {return setting;}
    virtual void chk(FieldSpec *p_fspec);
    virtual void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent value FieldSettings.
   */
  class FieldSetting_Value : public FieldSetting {
  private:
    Value *setting;

    FieldSetting_Value(const FieldSetting_Value& p);
  public:
    FieldSetting_Value(Identifier *p_name, Value *p_setting);
    virtual ~FieldSetting_Value();
    virtual FieldSetting_Value* clone() const
    {return new FieldSetting_Value(*this);}
    virtual Value* get_setting() {return setting;}
    virtual void chk(FieldSpec *p_fspec);
    virtual void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent object FieldSettings.
   */
  class FieldSetting_O : public FieldSetting {
  private:
    Object *setting;

    FieldSetting_O(const FieldSetting_O& p);
  public:
    FieldSetting_O(Identifier *p_name, Object *p_setting);
    virtual ~FieldSetting_O();
    virtual FieldSetting_O* clone() const
    {return new FieldSetting_O(*this);}
    virtual Object* get_setting() {return setting;}
    virtual void chk(FieldSpec *p_fspec);
    virtual void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent objectset FieldSettings.
   */
  class FieldSetting_OS : public FieldSetting {
  private:
    ObjectSet *setting;

    FieldSetting_OS(const FieldSetting_OS& p);
  public:
    FieldSetting_OS(Identifier *p_name, ObjectSet *p_setting);
    virtual ~FieldSetting_OS();
    virtual FieldSetting_OS* clone() const
    {return new FieldSetting_OS(*this);}
    virtual ObjectSet* get_setting() {return setting;}
    virtual void chk(FieldSpec *p_fspec);
    virtual void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent ObjectDefinition.
   */
  class Obj_defn : public Object {
  private:
    Block *block;
    map<string, FieldSetting> fss; /**< FieldSettings */
    bool is_generated;

    Obj_defn(const Obj_defn& p);
  public:
    Obj_defn(Block *p_block);
    Obj_defn();
    virtual ~Obj_defn();
    virtual Obj_defn* clone() const {return new Obj_defn(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Obj_defn* get_refd_last(ReferenceChain * =0) {return this;}
    virtual void chk();
    void add_fs(FieldSetting *p_fs);
    virtual size_t get_nof_fss() {return fss.size();}
    virtual bool has_fs_withName(const Identifier& p_name);
    virtual FieldSetting* get_fs_byName(const Identifier& p_name);
    virtual bool has_fs_withName_dflt(const Identifier& p_name);
    /** Returns the setting (or default if absent) */
    virtual Setting* get_setting_byName_dflt(const Identifier& p_name);
    virtual void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
  private:
    void parse_block();
  };

  /**
   * Class to represent a ReferencedObject. It is a DefinedObject or
   * ObjectFromObject.
   */
  class Obj_refd : public Object {
  private:
    Reference *ref;
    Obj_defn *o_error;
    Object *o_refd; /**< cache */
    Obj_defn *o_defn; /**< cache */

    Obj_refd(const Obj_refd& p);
  public:
    Obj_refd(Reference *p_ref);
    virtual ~Obj_refd();
    virtual Obj_refd* clone() const {return new Obj_refd(*this);}
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    Object* get_refd(ReferenceChain *refch=0);
    virtual Obj_defn* get_refd_last(ReferenceChain *refch=0);
    virtual void chk();
    virtual void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
  };

  /**
   * ObjectSet elements (flat container). Warning: the objects are not
   * owned by Objects, it stores only the pointers...
   */
  class Objects : public Node {
  private:
    vector<Obj_defn> objs;

    Objects(const Objects&);
  public:
    Objects() : Node() {}
    virtual ~Objects();
    virtual Objects* clone() const {return new Objects(*this);}
    void add_o(Obj_defn *p_o) {objs.add(p_o);}
    void add_objs(Objects *p_objs);
    size_t get_nof_objs() const {return objs.size();}
    Obj_defn* get_obj_byIndex(size_t p_i) {return objs[p_i];}
    virtual void dump(unsigned level) const;
  };

  /**
   * ObjectSetElement Visitor, checker.
   */
  class OSEV_checker : public OSE_Visitor {
  private:
    ObjectClass *governor;
    OC_defn *gov_defn;

  public:
    OSEV_checker(const Location *p_loc, ObjectClass *p_governor);
    virtual void visit_Object(Object& p);
    virtual void visit_OS_refd(OS_refd& p);
  };

  /**
   * ObjectSetElement Visitor, object collector.
   */
  class OSEV_objcollctr : public OSE_Visitor {
  private:
    OC_defn *governor;
    map<void*, void> visdes; /**< visited elements */
    Objects *objs;

  public:
    OSEV_objcollctr(ObjectSet& parent);
    OSEV_objcollctr(const Location *p_loc, ObjectClass *p_governor);
    virtual ~OSEV_objcollctr();
    virtual void visit_Object(Object& p);
    virtual void visit_OS_refd(OS_refd& p);
    void visit_ObjectSet(ObjectSet& p, bool force=false);
    Objects* get_objs() {return objs;}
    Objects* give_objs();
  };

  /**
   * ObjectSetElement Visitor, code generator.
   */
  class OSEV_codegen : public OSE_Visitor {
  private:
    output_struct *target;
  public:
    OSEV_codegen(const Location *p_loc, output_struct *p_target)
      : OSE_Visitor(p_loc), target(p_target) {}
    virtual void visit_Object(Object& p);
    virtual void visit_OS_refd(OS_refd& p);
  };

  /**
   * ObjectSet definition.
   */
  class OS_defn : public ObjectSet {
  private:
    Block *block;
    vector<OS_Element> *oses;
    Objects *objs;
    bool is_generated;

    OS_defn(const OS_defn& p);
  public:
    OS_defn();
    OS_defn(Block *p_block);
    OS_defn(Objects *p_objs);
    virtual ~OS_defn();
    virtual OS_defn* clone() const {return new OS_defn(*this);}
    void steal_oses(OS_defn *other_os);
    virtual OS_defn* get_refd_last(ReferenceChain *refch=0);
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void add_ose(OS_Element *p_ose);
    virtual size_t get_nof_objs();
    virtual Object* get_obj_byIndex(size_t p_i);
    virtual Objects* get_objs();
    virtual void accept(OSEV_objcollctr& v) {v.visit_ObjectSet(*this);}
    virtual void chk();
    virtual void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
  private:
    void parse_block();
    void create_objs();
  };

  /**
   * Referenced ObjectSet.
   */
  class OS_refd : public ObjectSet, public OS_Element {
  private:
    Reference *ref;
    OS_defn *os_error;
    ObjectSet *os_refd; /**< cache */
    OS_defn *os_defn; /**< cache */

    OS_refd(const OS_refd& p);
    virtual string create_stringRepr();
  public:
    OS_refd(Reference *p_ref);
    virtual ~OS_refd();
    virtual OS_refd* clone() const {return new OS_refd(*this);}
    virtual void set_my_scope(Scope *p_scope);
    ObjectSet* get_refd(ReferenceChain *refch=0);
    virtual OS_defn* get_refd_last(ReferenceChain *refch=0);
    virtual size_t get_nof_objs();
    virtual Object* get_obj_byIndex(size_t p_i);
    virtual void accept(OSE_Visitor& v) {v.visit_OS_refd(*this);}
    virtual void accept(OSEV_objcollctr& v) {v.visit_ObjectSet(*this);}
    virtual void chk();
    virtual void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
    virtual OS_Element *clone_ose() const;
    virtual void set_fullname_ose(const string& p_fullname);
    virtual void set_genname_ose(const string& p_prefix,
				 const string& p_suffix);
    virtual void set_my_scope_ose(Scope *p_scope);
  };

} // namespace Asn

#endif // _Asn_Object_HH
