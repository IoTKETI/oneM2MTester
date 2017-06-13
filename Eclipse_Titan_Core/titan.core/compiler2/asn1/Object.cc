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
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "Object.hh"
#include "../Identifier.hh"
#include "Block.hh"
#include "TokenBuf.hh"
#include "../Type.hh"
#include "../Value.hh"
#include "OCSV.hh"
#include "../main.hh"

namespace Asn {

  // =================================
  // ===== ObjectClass
  // =================================

  // =================================
  // ===== OC_defn
  // =================================

  OC_defn::OC_defn()
    : ObjectClass(), block_fs(0), block_ws(0), fss(0),
      ocs(0), is_erroneous(true), is_generated(false)
  {
  }

  OC_defn::OC_defn(Block *p_block_fs, Block *p_block_ws)
    : ObjectClass(), block_fs(p_block_fs), block_ws(p_block_ws), fss(0),
      ocs(0), is_erroneous(false), is_generated(false)
  {
    if(!p_block_fs)
      FATAL_ERROR("NULL parameter: Asn::OC_defn::OC_defn()");
  }

  OC_defn::OC_defn(const OC_defn& p)
    : ObjectClass(p), is_erroneous(p.is_erroneous),
      is_generated(false)
  {
    block_fs=p.block_fs?p.block_fs->clone():0;
    block_ws=p.block_ws?p.block_ws->clone():0;
    fss=p.fss?p.fss->clone():0;
    if(fss) fss->set_my_oc(this);
    /* warning: this is not implemented */
    // ocs=p.ocs?p.ocs->clone():0;
    ocs=0;
  }

  OC_defn::~OC_defn()
  {
    delete block_fs;
    delete block_ws;
    delete fss;
    delete ocs;
  }

  void OC_defn::set_fullname(const string& p_fullname)
  {
    ObjectClass::set_fullname(p_fullname);
    if(fss) fss->set_fullname(p_fullname);
  }

  void OC_defn::chk()
  {
    if(checked) return;
    checked=true;
    parse_block_fs();
    if(!ocs) {
      TokenBuf *t_tb=block_ws?block_ws->get_TokenBuf():0;
      ocs=new OCS_root();
      Error_Context ec(this, "In syntax of ObjectClass `%s'",
                       this->get_fullname().c_str());
      OCSV_Builder *ocsv_builder=new OCSV_Builder(t_tb, fss);
      ocs->accept(*ocsv_builder);
      delete ocsv_builder;
      delete block_ws; block_ws=0;
    }
    fss->chk();
  }

  void OC_defn::chk_this_obj(Object *p_obj)
  {
    if(!p_obj)
      FATAL_ERROR("NULL parameter: Asn::OC_defn::chk_this_obj()");
    if(is_erroneous) return;
    Obj_defn *obj=p_obj->get_refd_last();
    Error_Context ec(obj, "In object definition `%s'",
                     obj->get_fullname().c_str());
    for(size_t i=0; i<fss->get_nof_fss(); i++) {
      FieldSpec *fspec=fss->get_fs_byIndex(i)->get_last();
      if(!obj->has_fs_withName(fspec->get_id())) {
        if(!(fspec->get_is_optional() || fspec->has_default())
           && !(obj->get_is_erroneous())) {
          obj->error("Missing setting for `%s'",
                     fspec->get_id().get_dispname().c_str());
          obj->set_is_erroneous();
        }
        continue;
      }
      FieldSetting *fset=obj->get_fs_byName(fspec->get_id());
      if(!fset) continue;
      Error_Context ec2(fset, "In setting for `%s'",
                        fspec->get_id().get_dispname().c_str());
      fset->set_genname(obj->get_genname_own(), fspec->get_id().get_name());
      fset->chk(fspec);
    } // for i
  }

  void OC_defn::parse_block_fs()
  {
    if(fss) return;
    if(!block_fs) {
      if(!is_erroneous) FATAL_ERROR("Asn::OC_defn::parse_block_fs()");
    }
    else {
      Node *node=block_fs->parse(KW_Block_FieldSpecList);
      delete block_fs; block_fs=0;
      fss=dynamic_cast<FieldSpecs*>(node);
    }
    if(!fss) fss=new FieldSpecs();
    fss->set_fullname(get_fullname());
    fss->set_my_oc(this);
  }

  OCS_root* OC_defn::get_ocs()
  {
    chk();
    return ocs;
  }

  FieldSpecs* OC_defn::get_fss()
  {
    chk();
    return fss;
  }

  void OC_defn::generate_code(output_struct *target)
  {
    if(is_generated) return;
    is_generated=true;
    fss->generate_code(target);
  }

  void OC_defn::dump(unsigned level) const
  {
    DEBUG(level, "ObjectClass definition");
    if(block_fs)
      DEBUG(level, "with unparsed block");
    if(fss) {
      DEBUG(level, "with fieldspecs (%lu pcs):",
        static_cast<unsigned long>( fss->get_nof_fss()));
      fss->dump(level+1);
    }
    if(ocs) {
      DEBUG(level, "with syntax definition:");
      ocs->dump(level+1);
    }
  }

  // =================================
  // ===== OC_refd
  // =================================

  OC_refd::OC_refd(Reference *p_ref)
    : ObjectClass(), ref(p_ref), oc_error(0), oc_refd(0), oc_defn(0)
  {
    if(!p_ref)
      FATAL_ERROR("NULL parameter: Asn::OC_refd::OC_refd()");
  }

  OC_refd::OC_refd(const OC_refd& p)
    : ObjectClass(p), oc_error(0), oc_refd(0), oc_defn(0)
  {
    ref=p.ref->clone();
  }

  OC_refd::~OC_refd()
  {
    delete ref;
    delete oc_error;
  }

  void OC_refd::set_my_scope(Scope *p_scope)
  {
    ObjectClass::set_my_scope(p_scope);
    ref->set_my_scope(p_scope);
  }

  void OC_refd::set_fullname(const string& p_fullname)
  {
    ObjectClass::set_fullname(p_fullname);
    ref->set_fullname(p_fullname);
  }

  OC_defn* OC_refd::get_refd_last(ReferenceChain *refch)
  {
    if(!oc_defn) {
      bool destroy_refch=false;
      if(!refch) {
        refch=new ReferenceChain
          (ref, "While searching referenced ObjectClass");
        destroy_refch=true;
      }
      oc_defn=get_refd(refch)->get_refd_last(refch);
      if(destroy_refch) delete refch;
    }
    return oc_defn;
  }

  ObjectClass* OC_refd::get_refd(ReferenceChain *refch)
  {
    if(refch) refch->add(get_fullname());
    if(!oc_refd) {
      Setting *setting=0;
      Common::Assignment *ass=ref->get_refd_assignment();
      if(!ass) goto error;
      setting=ref->get_refd_setting();
      if(setting->get_st()==Setting::S_ERROR) goto error;
      oc_refd=dynamic_cast<ObjectClass*>(setting);
      if(!oc_refd) {
        error("This is not an objectclassreference: `%s'",
              ref->get_dispname().c_str());
        goto error;
      }
      goto end;
    error:
      oc_error=new OC_defn();
      oc_refd=oc_error;
    }
  end:
    return oc_refd;
  }

  void OC_refd::chk()
  {
    if(checked) return;
    checked=true;
    get_refd_last();
  }

  void OC_refd::chk_this_obj(Object *obj)
  {
    get_refd_last()->chk_this_obj(obj);
  }

  OCS_root* OC_refd::get_ocs()
  {
    chk();
    return get_refd_last()->get_ocs();
  }

  FieldSpecs* OC_refd::get_fss()
  {
    chk();
    return get_refd_last()->get_fss();
  }

  void OC_refd::generate_code(output_struct *target)
  {
    OC_defn *refd_last = get_refd_last();
    if(my_scope->get_scope_mod_gen() ==
       refd_last->get_my_scope()->get_scope_mod_gen())
      refd_last->generate_code(target);
  }

  void OC_refd::dump(unsigned level) const
  {
    DEBUG(level, "objectclassreference");
    level++;
    ref->dump(level);
  }

  // =================================
  // ===== FieldSpec
  // =================================

  FieldSpec::FieldSpec(fstype_t p_fstype, Identifier *p_id,
                       bool p_is_optional=false)
    : Node(), Location(), fstype(p_fstype), id(p_id),
      is_optional(p_is_optional), my_oc(0), checked(false)
  {
    if(!p_id)
      FATAL_ERROR("NULL parameter: Asn::FieldSpec::FieldSpec()");
  }

  FieldSpec::FieldSpec(const FieldSpec& p)
    : Node(p), Location(p), fstype(p.fstype), is_optional(p.is_optional),
      my_oc(0), checked(false)
  {
    id=p.id->clone();
  }

  FieldSpec::~FieldSpec()
  {
    delete id;
  }

  void FieldSpec::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
  }

  void FieldSpec::set_my_oc(OC_defn *p_oc)
  {
    my_oc=p_oc;
  }

  // =================================
  // ===== FieldSpec_Undef
  // =================================

  FieldSpec_Undef::FieldSpec_Undef(Identifier *p_id, Ref_defd *p_govref,
                                   bool p_is_optional, Node *p_defsetting)
    : FieldSpec(FS_UNDEF, p_id, p_is_optional),
      govref(p_govref), defsetting(p_defsetting), fs(0)
  {
    if(!p_govref)
      FATAL_ERROR("FieldSpec_Undef::FieldSpec_Undef()");
  }

  FieldSpec_Undef::FieldSpec_Undef(const FieldSpec_Undef& p)
    : FieldSpec(p)
  {
    govref=p.govref?p.govref->clone():0;
    defsetting=p.defsetting?p.defsetting->clone():0;
    fs=p.fs?p.fs->clone():0;
  }

  FieldSpec_Undef::~FieldSpec_Undef()
  {
    delete govref;
    delete defsetting;
    if(fs) delete fs;
  }

  /** \todo Enhance! (FS_VS_FT + refch!) */
  void FieldSpec_Undef::classify_fs(ReferenceChain *refch)
  {
    if(fstype!=FS_UNDEF) return;
    bool destroy_refch=false;
    if(!refch) {
      refch=new ReferenceChain(this, "While examining kind of fieldspec");
      destroy_refch=true;
    }
    else refch->mark_state();

    if(is_optional && defsetting) {
      error("OPTIONAL and DEFAULT are mutual exclusive");
      is_optional=false;
    }

    /* temporary pointers */
    Reference *t_ref=0;
    Reference *t_ref2=0;
    Block *t_block=0;

    if(!refch->add(get_fullname()))
      goto error;

    if((t_ref=dynamic_cast<Reference*>(govref))) {
      t_ref->set_my_scope(my_oc->get_my_scope());
      if(t_ref->refers_to_st(Setting::S_ERROR, refch))
        goto error;
    }
    if((t_ref=dynamic_cast<Reference*>(defsetting)))
      t_ref->set_my_scope(my_oc->get_my_scope());
    govref->set_fullname(get_fullname());
    if(defsetting) defsetting->set_fullname(get_fullname());

    if(id->isvalid_asn_objsetfieldref()
       && (t_ref=dynamic_cast<Ref_defd_simple*>(govref))
       && t_ref->refers_to_st(Setting::S_OC, refch)
       ) {
      ObjectSet *defobjset=0;
      if(defsetting) {
        if((t_block=dynamic_cast<Block*>(defsetting))) {
          defobjset=new OS_defn(t_block);
        }
        else {
          error("Cannot recognize DEFAULT setting"
                " for this ObjectSetFieldSpec");
          delete defobjset;
          defobjset=new OS_defn();
        }
      } // if defsetting
      fs=new FieldSpec_OS(id->clone(), new OC_refd(t_ref),
                          is_optional, defobjset);
      govref=0;
      defsetting=0;
      fstype=FS_OS;
    }
    else if(id->isvalid_asn_objfieldref()
            && (t_ref=dynamic_cast<Ref_defd_simple*>(govref))
            && t_ref->refers_to_st(Setting::S_OC, refch)
            ) {
      Object *defobj=0;
      if(defsetting) {
        if((t_block=dynamic_cast<Block*>(defsetting))) {
          defobj=new Obj_defn(t_block);
        }
        else if((t_ref2=dynamic_cast<Reference*>(defsetting))) {
          defobj=new Obj_refd(t_ref2);
        }
        else {
          error("Cannot recognize DEFAULT setting"
                " for this ObjectFieldSpec");
          delete defobj;
          defobj=new Obj_defn();
        }
      } // if defsetting
      fs=new FieldSpec_O(id->clone(), new OC_refd(t_ref),
                         is_optional, defobj);
      govref=0;
      defsetting=0;
      fstype=FS_O;
    }
    else if(id->isvalid_asn_valfieldref()
            && (t_ref=dynamic_cast<Ref_defd*>(govref))
            && (t_ref->refers_to_st(Setting::S_T, refch)
                || t_ref->refers_to_st(Setting::S_VS, refch))
            ) {
      Value *defval=0;
      if(defsetting) {
        if((t_block=dynamic_cast<Block*>(defsetting))) {
          defval=new Value(Value::V_UNDEF_BLOCK, t_block);
        }
        else if((t_ref2=dynamic_cast<Reference*>(defsetting))) {
          Ref_defd_simple *t_ref3=dynamic_cast<Ref_defd_simple*>(t_ref2);
          if(t_ref3 && !t_ref3->get_modid()) {
            defval=new Value(Value::V_UNDEF_LOWERID,
                             t_ref3->get_id()->clone());
            delete t_ref3;
          }
          else {
            defval=new Value(Value::V_REFD, t_ref2);
          }
        }
        else {
          error("Cannot recognize DEFAULT setting"
                " for this FixedTypeValueFieldSpec");
          delete defsetting;
          defval=new Value(Value::V_ERROR);
        }
      } // if defsetting
      fs=new FieldSpec_V_FT(id->clone(), new Type(Type::T_REFD, t_ref),
                            false, is_optional, defval);
      govref=0;
      defsetting=0;
      fstype=FS_V_FT;
    }
    else {
      goto error;
    }
    goto end;
  error:
    fstype=FS_ERROR;
    error("Cannot recognize fieldspec");
    fs=new FieldSpec_Error(id->clone(), is_optional, defsetting!=0);
    delete defsetting; defsetting=0;
  end:
    fs->set_fullname(get_fullname());
    if(my_oc) fs->set_my_oc(my_oc);
    fs->set_location(*this);
    if(destroy_refch) delete refch;
    else refch->prev_state();
  }

  FieldSpec* FieldSpec_Undef::clone() const
  {
    if(fs) return fs->clone();
    else return new FieldSpec_Undef(*this);
  }

  void FieldSpec_Undef::set_fullname(const string& p_fullname)
  {
    FieldSpec::set_fullname(p_fullname);
    if(fs) fs->set_fullname(p_fullname);
  }

  FieldSpec::fstype_t FieldSpec_Undef::get_fstype()
  {
    classify_fs();
    return fstype;
  }

  void FieldSpec_Undef::set_my_oc(OC_defn *p_oc)
  {
    FieldSpec::set_my_oc(p_oc);
    if(fs) fs->set_my_oc(p_oc);
  }

  bool FieldSpec_Undef::has_default()
  {
    if(fs) return fs->has_default();
    else return defsetting;
  }

  void FieldSpec_Undef::chk()
  {
    if(checked) return;
    classify_fs();
    fs->chk();
    checked=true;
  }

  Setting* FieldSpec_Undef::get_default()
  {
    classify_fs();
    return fs->get_default();
  }

  FieldSpec* FieldSpec_Undef::get_last()
  {
    classify_fs();
    return fs->get_last();
  }

  void FieldSpec_Undef::generate_code(output_struct *target)
  {
    classify_fs();
    fs->generate_code(target);
  }

  // =================================
  // ===== FieldSpec_Error
  // =================================

  FieldSpec_Error::FieldSpec_Error(Identifier *p_id, bool p_is_optional,
                                   bool p_has_default)
    : FieldSpec(FS_ERROR, p_id, p_is_optional), setting_error(0),
      has_default_flag(p_has_default)
  {
  }

  FieldSpec_Error::FieldSpec_Error(const FieldSpec_Error& p)
    : FieldSpec(p), setting_error(0), has_default_flag(p.has_default_flag)
  {
  }

  FieldSpec_Error::~FieldSpec_Error()
  {
    delete setting_error;
  }

  FieldSpec_Error *FieldSpec_Error::clone() const
  {
    return new FieldSpec_Error(*this);
  }

  bool FieldSpec_Error::has_default()
  {
    return has_default_flag;
  }

  Setting* FieldSpec_Error::get_default()
  {
    if(!setting_error && has_default_flag)
      setting_error=new Common::Setting_Error();
    return setting_error;
  }

  void FieldSpec_Error::chk()
  {

  }

  void FieldSpec_Error::generate_code(output_struct *)
  {

  }

  // =================================
  // ===== FieldSpec_T
  // =================================

  FieldSpec_T::FieldSpec_T(Identifier *p_id, bool p_is_optional,
                           Type *p_deftype)
    : FieldSpec(FS_T, p_id, p_is_optional), deftype(p_deftype)
  {
    if(is_optional && deftype) {
      error("OPTIONAL and DEFAULT are mutual exclusive");
      is_optional=false;
    }
    if (deftype) deftype->set_ownertype(Type::OT_TYPE_FLD, this);
  }

  FieldSpec_T::FieldSpec_T(const FieldSpec_T& p)
    : FieldSpec(p)
  {
    deftype=p.deftype?p.deftype->clone():0;
  }

  FieldSpec_T::~FieldSpec_T()
  {
    delete deftype;
  }

  void FieldSpec_T::set_fullname(const string& p_fullname)
  {
    FieldSpec::set_fullname(p_fullname);
    if(deftype) deftype->set_fullname(p_fullname);
  }

  void FieldSpec_T::set_my_oc(OC_defn *p_oc)
  {
    FieldSpec::set_my_oc(p_oc);
    if(deftype) deftype->set_my_scope(my_oc->get_my_scope());
  }

  Type* FieldSpec_T::get_default()
  {
    return deftype;
  }

  void FieldSpec_T::chk()
  {
    if(checked) return;
    if(deftype) {
      deftype->set_genname(my_oc->get_genname_own(), id->get_name());
      deftype->chk();
    }
    checked=true;
  }

  void FieldSpec_T::generate_code(output_struct *target)
  {
    if (deftype) deftype->generate_code(target);
  }

  // =================================
  // ===== FieldSpec_V_FT
  // =================================

  FieldSpec_V_FT::FieldSpec_V_FT(Identifier *p_id, Type *p_fixtype,
                                 bool p_is_unique, bool p_is_optional,
                                 Value *p_defval)
    : FieldSpec(FS_V_FT, p_id, p_is_optional),
      fixtype(p_fixtype), is_unique(p_is_unique), defval(p_defval)
  {
    if(!p_fixtype)
      FATAL_ERROR("NULL parameter: FieldSpec_V_FT::FieldSpec_V_FT()");
    fixtype->set_ownertype(Type::OT_FT_V_FLD, this);
    if(is_optional && defval) {
      error("OPTIONAL and DEFAULT are mutual exclusive");
      is_optional=false;
    }
    if(is_unique && defval)
      error("UNIQUE and DEFAULT are mutual exclusive");
  }

  FieldSpec_V_FT::FieldSpec_V_FT(const FieldSpec_V_FT& p)
    : FieldSpec(p), is_unique(p.is_unique)
  {
    fixtype=p.fixtype->clone();
    defval=p.defval?p.defval->clone():0;
  }

  FieldSpec_V_FT::~FieldSpec_V_FT()
  {
    delete fixtype;
    delete defval;
  }

  void FieldSpec_V_FT::set_fullname(const string& p_fullname)
  {
    FieldSpec::set_fullname(p_fullname);
    fixtype->set_fullname(p_fullname);
    if(defval) defval->set_fullname(p_fullname);
  }

  void FieldSpec_V_FT::set_my_oc(OC_defn *p_oc)
  {
    FieldSpec::set_my_oc(p_oc);
    Scope *scope=my_oc->get_my_scope();
    fixtype->set_my_scope(scope);
    if(defval) defval->set_my_scope(scope);
  }

  Value* FieldSpec_V_FT::get_default()
  {
    return defval;
  }

  void FieldSpec_V_FT::chk()
  {
    if(checked) return;
    fixtype->set_genname(my_oc->get_genname_own(), id->get_name());
    fixtype->chk();
    if(defval) {
      defval->set_my_governor(fixtype);
      fixtype->chk_this_value_ref(defval);
      fixtype->chk_this_value(defval, 0, Type::EXPECTED_CONSTANT,
        INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
      if (!semantic_check_only) {
        defval->set_genname_prefix("const_");
        defval->set_genname_recursive(string(fixtype->get_genname_own()) +
	  "_defval_");
	defval->set_code_section(GovernedSimple::CS_PRE_INIT);
      }
    }
    checked=true;
  }

  void FieldSpec_V_FT::generate_code(output_struct *target)
  {
    fixtype->generate_code(target);
    if(defval) {
      const_def cdef;
      Code::init_cdef(&cdef);
      fixtype->generate_code_object(&cdef, defval);
      cdef.init = defval->generate_code_init(cdef.init,
        defval->get_lhs_name().c_str());
      Code::merge_cdef(target, &cdef);
      Code::free_cdef(&cdef);
    }
  }

  // =================================
  // ===== FieldSpec_V_VT
  // =================================

  // =================================
  // ===== FieldSpec_VS_FT
  // =================================

  // =================================
  // ===== FieldSpec_VS_VT
  // =================================

  // =================================
  // ===== FieldSpec_O
  // =================================

  FieldSpec_O::FieldSpec_O(Identifier *p_id, ObjectClass *p_oc,
                           bool p_is_optional, Object *p_defobj)
    : FieldSpec(FS_O, p_id, p_is_optional),
      oc(p_oc), defobj(p_defobj)
  {
    if(!p_oc)
      FATAL_ERROR("NULL parameter: FieldSpec_O::FieldSpec_O()");
    if(is_optional && defobj) {
      error("OPTIONAL and DEFAULT are mutual exclusive");
      is_optional=false;
    }
  }

  FieldSpec_O::FieldSpec_O(const FieldSpec_O& p)
    : FieldSpec(p)
  {
    oc=p.oc->clone();
    defobj=p.defobj?p.defobj->clone():0;
  }

  FieldSpec_O::~FieldSpec_O()
  {
    delete oc;
    delete defobj;
  }

  void FieldSpec_O::set_fullname(const string& p_fullname)
  {
    FieldSpec::set_fullname(p_fullname);
    oc->set_fullname(p_fullname);
    if(defobj) defobj->set_fullname(p_fullname);
  }

  void FieldSpec_O::set_my_oc(OC_defn *p_oc)
  {
    FieldSpec::set_my_oc(p_oc);
    Scope *scope=my_oc->get_my_scope();
    oc->set_my_scope(scope);
    if(defobj) defobj->set_my_scope(scope);
  }

  void FieldSpec_O::chk()
  {
    if(checked) return;
    oc->set_genname(my_oc->get_genname_own(), id->get_name());
    oc->chk();
    if(defobj) {
      defobj->set_my_governor(oc);
      defobj->set_genname(oc->get_genname_own(), string("_defobj_"));
      defobj->chk();
    }
    checked=true;
  }

  void FieldSpec_O::generate_code(output_struct *target)
  {
    oc->generate_code(target);
    if (defobj) defobj->generate_code(target);
  }

  // =================================
  // ===== FieldSpec_OS
  // =================================

  FieldSpec_OS::FieldSpec_OS(Identifier *p_id, ObjectClass *p_oc,
                             bool p_is_optional, ObjectSet *p_defobjset)
    : FieldSpec(FS_OS, p_id, p_is_optional),
      oc(p_oc), defobjset(p_defobjset)
  {
    if(!p_oc)
      FATAL_ERROR("NULL parameter: FieldSpec_OS::FieldSpec_OS()");
    if(is_optional && defobjset) {
      error("OPTIONAL and DEFAULT are mutual exclusive");
      is_optional=false;
    }
  }

  FieldSpec_OS::FieldSpec_OS(const FieldSpec_OS& p)
    : FieldSpec(p)
  {
    oc=p.oc->clone();
    defobjset=p.defobjset?p.defobjset->clone():0;
  }

  FieldSpec_OS::~FieldSpec_OS()
  {
    delete oc;
    delete defobjset;
  }

  void FieldSpec_OS::set_fullname(const string& p_fullname)
  {
    FieldSpec::set_fullname(p_fullname);
    oc->set_fullname(p_fullname);
    if(defobjset) defobjset->set_fullname(p_fullname);
  }

  void FieldSpec_OS::set_my_oc(OC_defn *p_oc)
  {
    FieldSpec::set_my_oc(p_oc);
    Scope *scope=my_oc->get_my_scope();
    oc->set_my_scope(scope);
    if(defobjset) defobjset->set_my_scope(scope);
  }

  void FieldSpec_OS::chk()
  {
    if(checked) return;
    oc->set_genname(my_oc->get_genname_own(), id->get_name());
    oc->chk();
    if(defobjset) {
      defobjset->set_my_governor(oc);
      defobjset->set_genname(oc->get_genname_own(), string("_defobj_"));
      defobjset->chk();
    }
    checked=true;
  }

  void FieldSpec_OS::generate_code(output_struct *target)
  {
    oc->generate_code(target);
    if (defobjset) defobjset->generate_code(target);
  }

  // =================================
  // ===== FieldSpecs
  // =================================

  FieldSpecs::FieldSpecs()
    : Node(), fs_error(0), my_oc(0)
  {
  }

  FieldSpecs::FieldSpecs(const FieldSpecs& p)
    : Node(p), fs_error(0), my_oc(0)
  {
    for(size_t i=0; i<p.fss_v.size(); i++)
      add_fs(p.fss_v[i]->clone());
  }

  FieldSpecs::~FieldSpecs()
  {
    for(size_t i=0; i<fss_v.size(); i++)
      delete fss_v[i];
    fss.clear();
    fss_v.clear();
    delete fs_error;
  }

  void FieldSpecs::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for(size_t i=0; i<fss_v.size(); i++) {
      FieldSpec *fs=fss_v[i];
      fs->set_fullname(p_fullname+"."+fs->get_id().get_dispname());
    }
  }

  void FieldSpecs::set_my_oc(OC_defn *p_oc)
  {
    my_oc=p_oc;
    for(size_t i=0; i<fss_v.size(); i++)
      fss_v[i]->set_my_oc(p_oc);
  }

  void FieldSpecs::add_fs(FieldSpec *p_fs)
  {
    if(!p_fs)
      FATAL_ERROR("NULL parameter: Asn::FieldSpecs::add_fs()");
    if(fss.has_key(p_fs->get_id().get_name())) {
      p_fs->error("A fieldspec with identifier `%s' already exists",
                  p_fs->get_id().get_dispname().c_str());
      delete p_fs;
      return;
    }
    fss.add(p_fs->get_id().get_name(), p_fs);
    fss_v.add(p_fs);
    p_fs->set_fullname(get_fullname()+"."+p_fs->get_id().get_dispname());
    if(my_oc) p_fs->set_my_oc(my_oc);
  }

  bool FieldSpecs::has_fs_withId(const Identifier& p_id)
  {
    return fss.has_key(p_id.get_name());
  }

  FieldSpec* FieldSpecs::get_fs_byId(const Identifier& p_id)
  {
    if(fss.has_key(p_id.get_name()))
      return fss[p_id.get_name()];
    my_oc->error("No fieldspec with name `%s'", p_id.get_dispname().c_str());
    return get_fs_error();
  }

  FieldSpec* FieldSpecs::get_fs_byIndex(size_t p_i)
  {
    if(p_i <fss_v.size())
      return fss_v[p_i];
    my_oc->error("No fieldspec at index %s", Int2string(p_i).c_str());
    return get_fs_error();
  }

  FieldSpec* FieldSpecs::get_fs_error()
  {
    if(!fs_error)
      fs_error=new FieldSpec_Error
        (new Identifier(Identifier::ID_NAME, string("<error>")), true, false);
    return fs_error;
  }

  void FieldSpecs::chk()
  {
    for(size_t i=0; i<fss_v.size(); i++) {
      FieldSpec *fs=fss_v[i];
      Error_Context ec(fs, "In fieldspec `%s'",
                       fs->get_id().get_dispname().c_str());
      fs->chk();
    }
  }

  void FieldSpecs::generate_code(output_struct *target)
  {
    for(size_t i=0; i<fss_v.size(); i++)
      fss_v[i]->generate_code(target);
  }

  void FieldSpecs::dump(unsigned level) const
  {
    DEBUG(level, "Fieldspecs (%lu pcs.)", static_cast<unsigned long>( fss.size()));
    level++;
    for(size_t i=0; i<fss_v.size(); i++)
      fss_v[i]->dump(level);
  }

  // =================================
  // ===== OCS_Visitor
  // =================================

  OCS_Visitor* OCS_Visitor::clone() const
  {
    FATAL_ERROR("Asn::OCS_Visitor::clone()");
    return 0;
  }

  // =================================
  // ===== OCS_Node
  // =================================

  OCS_Node* OCS_Node::clone() const
  {
    FATAL_ERROR("Asn::OCS_Node::clone()");
    return 0;
  }

  void OCS_Node::dump(unsigned level) const
  {
    DEBUG(level, "OCS_Node");
  }

  // =================================
  // ===== OCS_seq
  // =================================

  OCS_seq::~OCS_seq()
  {
    for(size_t i=0; i<ocss.size(); i++)
      delete ocss[i];
    ocss.clear();
  }

  void OCS_seq::add_node(OCS_Node *p_node)
  {
    if(!p_node)
      FATAL_ERROR("NULL parameter: Asn::OCS_seq::add_node()");
    ocss.add(p_node);
  }

  OCS_Node* OCS_seq::get_nth_node(size_t p_i)
  {
    if(ocss.size()<=p_i)
      FATAL_ERROR("Asn::OCS_seq::get_nth_node()");
    return ocss[p_i];
  }

  string OCS_seq::get_dispname() const
  {
    string s;
    if(is_opt) s="[";
    for(size_t i=0; i<ocss.size(); i++) {
      if(i) s+=" ";
      s+=ocss[i]->get_dispname();
    }
    if(is_opt) s+="]";
    return s;
  }

  void OCS_seq::dump(unsigned level) const
  {
    DEBUG(level, "%soptional sequence of tokens:", is_opt?"":"not ");
    level++;
    if(opt_first_comma)
      DEBUG(level, "[,]");
    for(size_t i=0; i<ocss.size(); i++)
      ocss[i]->dump(level);
  }

  // =================================
  // ===== OCS_root
  // =================================

  string OCS_root::get_dispname() const
  {
    return seq.get_dispname();
  }

  void OCS_root::dump(unsigned level) const
  {
    seq.dump(level);
  }

  // =================================
  // ===== OCS_literal
  // =================================

  OCS_literal::OCS_literal(Identifier *p_word)
    : OCS_Node(), word(p_word)
  {
    if(!p_word)
      FATAL_ERROR("NULL parameter: Asn::OCS_literal::OCS_literal()");
  }

  OCS_literal::~OCS_literal()
  {
    delete word;
  }

  string OCS_literal::get_dispname() const
  {
    string s('`');
    if(word) s+=word->get_dispname().c_str();
    else s+=Token::get_token_name(keyword);
    s+='\'';
    return s;
  }

  void OCS_literal::dump(unsigned level) const
  {
    DEBUG(level, "literal: %s", get_dispname().c_str());
  }

  // =================================
  // ===== OCS_setting
  // =================================

  OCS_setting::OCS_setting(settingtype_t p_st, Identifier *p_id)
    : OCS_Node(), st(p_st), id(p_id)
  {
    if(!p_id)
      FATAL_ERROR("NULL parameter: Asn::OCS_setting::OCS_setting()");
  }

  OCS_setting::~OCS_setting()
  {
    delete id;
  }

  string OCS_setting::get_dispname() const
  {
    string s('<');
    switch(st) {
    case S_T: s+="Type"; break;
    case S_V: s+="Value"; break;
    case S_VS: s+="ValueSet"; break;
    case S_O: s+="Object"; break;
    case S_OS: s+="ObjectSet"; break;
    default: s+="???";
    }
    s+='>';
    return s;
  }

  void OCS_setting::dump(unsigned level) const
  {
    DEBUG(level, "%s (setting for `%s')",
          get_dispname().c_str(),
          id->get_dispname().c_str());
  }

  // =================================
  // ===== OS_Element
  // =================================

  OS_Element::~OS_Element()
  {

  }

  // =================================
  // ===== Object
  // =================================

  void Object::set_my_governor(ObjectClass *p_gov)
  {
    if(!p_gov)
      FATAL_ERROR("NULL parameter: Asn::Object::set_my_governor()");
    my_governor=p_gov;
  }

  OS_Element *Object::clone_ose() const
  {
    return clone();
  }

  void Object::set_fullname_ose(const string& p_fullname)
  {
    set_fullname(p_fullname);
  }

  void Object::set_genname_ose(const string& p_prefix, const string& p_suffix)
  {
    set_genname(p_prefix, p_suffix);
  }

  void Object::set_my_scope_ose(Scope *p_scope)
  {
    set_my_scope(p_scope);
  }

  // =================================
  // ===== FieldSetting
  // =================================

  FieldSetting::FieldSetting(Identifier *p_name)
    : Node(), Location(), name(p_name), checked(false)
  {
    if(!p_name)
      FATAL_ERROR("NULL parameter: Asn::FieldSetting::FieldSetting()");
  }

  FieldSetting::FieldSetting(const FieldSetting& p)
    : Node(p), Location(p), checked(false)
  {
    name=p.name->clone();
  }

  FieldSetting::~FieldSetting()
  {
    delete name;
  }

  void FieldSetting::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    get_setting()->set_fullname(p_fullname);
  }

  void FieldSetting::set_my_scope(Scope *p_scope)
  {
    get_setting()->set_my_scope(p_scope);
  }

  void FieldSetting::set_genname(const string& p_prefix,
                                 const string& p_suffix)
  {
    get_setting()->set_genname(p_prefix, p_suffix);
  }

  // =================================
  // ===== FieldSetting_Type
  // =================================

  FieldSetting_Type::FieldSetting_Type(Identifier *p_name, Type *p_setting)
    : FieldSetting(p_name), setting(p_setting)
  {
    if(!p_setting)
      FATAL_ERROR
        ("NULL parameter: Asn::FieldSetting_Type::FieldSetting_Type()");
    setting->set_ownertype(Type::OT_FIELDSETTING, this);
  }

  FieldSetting_Type::FieldSetting_Type(const FieldSetting_Type& p)
    : FieldSetting(p)
  {
    setting=p.setting->clone();
  }

  FieldSetting_Type::~FieldSetting_Type()
  {
    delete setting;
  }

  void FieldSetting_Type::chk(FieldSpec *p_fspec)
  {
    if(checked) return;
    if(p_fspec->get_fstype()!=FieldSpec::FS_T) {
      error("Type setting was expected");
      delete setting;
      setting=new Type(Type::T_ERROR);
    }
    checked=true;
    setting->chk();
    ReferenceChain refch(setting, "While checking embedded recursions");
    setting->chk_recursions(refch);
  }

  void FieldSetting_Type::generate_code(output_struct *target)
  {
    setting->generate_code(target);
  }

  void FieldSetting_Type::dump(unsigned level) const
  {
    DEBUG(level, "field setting (type): %s", name->get_dispname().c_str());
    level++;
    setting->dump(level);
  }

  // =================================
  // ===== FieldSetting_Value
  // =================================

  FieldSetting_Value::FieldSetting_Value(Identifier *p_name, Value *p_setting)
    : FieldSetting(p_name), setting(p_setting)
  {
    if(!p_setting)
      FATAL_ERROR
        ("NULL parameter: Asn::FieldSetting_Value::FieldSetting_Value()");
  }

  FieldSetting_Value::FieldSetting_Value(const FieldSetting_Value& p)
    : FieldSetting(p)
  {
    setting=p.setting->clone();
  }

  FieldSetting_Value::~FieldSetting_Value()
  {
    delete setting;
  }

  void FieldSetting_Value::chk(FieldSpec *p_fspec)
  {
    if(checked) return;
    if(p_fspec->get_fstype()!=FieldSpec::FS_V_FT) {
      error("Value setting was expected");
      setting->set_valuetype(Value::V_ERROR);
    }
    FieldSpec_V_FT *fs=dynamic_cast<FieldSpec_V_FT*>(p_fspec);
    Type *type=fs->get_type();
    setting->set_my_governor(type);
    type->chk_this_value_ref(setting);
    checked=true;
    type->chk_this_value(setting, 0, Type::EXPECTED_CONSTANT,
      INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
    {
      ReferenceChain refch(setting, "While checking embedded recursions");
      setting->chk_recursions(refch);
    }
    if (!semantic_check_only) {
      setting->set_genname_prefix("const_");
      setting->set_genname_recursive(setting->get_genname_own());
      setting->set_code_section(GovernedSimple::CS_PRE_INIT);
    }
  }

  void FieldSetting_Value::generate_code(output_struct *target)
  {
    const_def cdef;
    Code::init_cdef(&cdef);
    Type *type = setting->get_my_governor();
    type->generate_code_object(&cdef, setting);
    cdef.init = setting->generate_code_init(cdef.init,
      setting->get_lhs_name().c_str());
    Code::merge_cdef(target, &cdef);
    Code::free_cdef(&cdef);
  }

  void FieldSetting_Value::dump(unsigned level) const
  {
    DEBUG(level, "field setting (value): %s", name->get_dispname().c_str());
    level++;
    setting->dump(level);
  }

  // =================================
  // ===== FieldSetting_O
  // =================================

  FieldSetting_O::FieldSetting_O(Identifier *p_name, Object *p_setting)
    : FieldSetting(p_name), setting(p_setting)
  {
    if(!p_setting)
      FATAL_ERROR
        ("NULL parameter: Asn::FieldSetting_O::FieldSetting_O()");
  }

  FieldSetting_O::FieldSetting_O(const FieldSetting_O& p)
    : FieldSetting(p)
  {
    setting=p.setting->clone();
  }

  FieldSetting_O::~FieldSetting_O()
  {
    delete setting;
  }

  void FieldSetting_O::chk(FieldSpec *p_fspec)
  {
    if(checked) return;
    if(p_fspec->get_fstype()!=FieldSpec::FS_O) {
      error("Object setting was expected");
      delete setting;
      setting=new Obj_defn();
    }
    FieldSpec_O *fs=dynamic_cast<FieldSpec_O*>(p_fspec);
    ObjectClass *oc=fs->get_oc();
    setting->set_my_governor(oc);
    checked=true;
    setting->chk();
  }

  void FieldSetting_O::generate_code(output_struct *target)
  {
    setting->generate_code(target);
  }

  void FieldSetting_O::dump(unsigned level) const
  {
    DEBUG(level, "field setting (object): %s", name->get_dispname().c_str());
    level++;
    setting->dump(level);
  }

  // =================================
  // ===== FieldSetting_OS
  // =================================

  FieldSetting_OS::FieldSetting_OS(Identifier *p_name, ObjectSet *p_setting)
    : FieldSetting(p_name), setting(p_setting)
  {
    if(!p_setting)
      FATAL_ERROR
        ("NULL parameter: Asn::FieldSetting_OS::FieldSetting_OS()");
  }

  FieldSetting_OS::FieldSetting_OS(const FieldSetting_OS& p)
    : FieldSetting(p)
  {
    setting=p.setting->clone();
  }

  FieldSetting_OS::~FieldSetting_OS()
  {
    delete setting;
  }

  void FieldSetting_OS::chk(FieldSpec *p_fspec)
  {
    if(checked) return;
    if(p_fspec->get_fstype()!=FieldSpec::FS_OS) {
      error("ObjectSet setting was expected");
      delete setting;
      setting=new OS_defn();
    }
    FieldSpec_OS *fs=dynamic_cast<FieldSpec_OS*>(p_fspec);
    ObjectClass *oc=fs->get_oc();
    setting->set_my_governor(oc);
    checked=true;
    setting->chk();
  }

  void FieldSetting_OS::generate_code(output_struct *target)
  {
    setting->generate_code(target);
  }

  void FieldSetting_OS::dump(unsigned level) const
  {
    DEBUG(level, "field setting (objectset): %s",
          name->get_dispname().c_str());
    level++;
    setting->dump(level);
  }

  // =================================
  // ===== Obj_defn
  // =================================

  Obj_defn::Obj_defn(Block *p_block)
    : Object(), block(p_block), is_generated(false)
  {
    if(!p_block)
      FATAL_ERROR("NULL parameter: Asn::Obj_defn::Obj_defn()");
  }

  Obj_defn::Obj_defn()
    : Object(), block(0), is_generated(false)
  {
    set_is_erroneous();
  }

  Obj_defn::Obj_defn(const Obj_defn& p)
    : Object(p), is_generated(false)
  {
    block=p.block?p.block->clone():0;
    for(size_t i=0; i<p.fss.size(); i++)
      add_fs(p.fss.get_nth_elem(i)->clone());
  }

  Obj_defn::~Obj_defn()
  {
    delete block;
    for(size_t i=0; i<fss.size(); i++)
      delete fss.get_nth_elem(i);
    fss.clear();
  }

  void Obj_defn::set_fullname(const string& p_fullname)
  {
    Object::set_fullname(p_fullname);
    for(size_t i=0; i<fss.size(); i++) {
      FieldSetting *fs=fss.get_nth_elem(i);
      fs->set_fullname(p_fullname+"."+fs->get_name().get_dispname());
    }
  }

  void Obj_defn::set_my_scope(Scope *p_scope)
  {
    Object::set_my_scope(p_scope);
    for(size_t i=0; i<fss.size(); i++)
      fss.get_nth_elem(i)->set_my_scope(p_scope);
  }

  void Obj_defn::chk()
  {
    if(checked) return;
    if(block) parse_block();
    if(!my_governor)
      FATAL_ERROR("Asn::Obj_defn::chk()");
    my_governor->chk_this_obj(this);
    checked=true;
  }

  void Obj_defn::add_fs(FieldSetting *p_fs)
  {
    if(!p_fs)
      FATAL_ERROR("NULL parameter: Asn::Obj_defn::add_fs()");
    if(fss.has_key(p_fs->get_name().get_name())) {
      error("This object already has a field setting with name `%s'",
            p_fs->get_name().get_dispname().c_str());
      delete p_fs;
      return;
    }
    fss.add(p_fs->get_name().get_name(), p_fs);
    p_fs->set_my_scope(my_scope);
    p_fs->set_fullname(get_fullname()+"."+p_fs->get_name().get_dispname());
  }

  bool Obj_defn::has_fs_withName(const Identifier& p_name)
  {
    if(block) parse_block();
    return fss.has_key(p_name.get_name());
  }

  FieldSetting* Obj_defn::get_fs_byName(const Identifier& p_name)
  {
    if(block) parse_block();
    const string& s=p_name.get_name();
    if(fss.has_key(s))
      return fss[s];
    if(!is_erroneous)
      error("No field setting with identifier `%s' in object `%s'",
            p_name.get_dispname().c_str(), get_fullname().c_str());
    return 0;
  }

  bool Obj_defn::has_fs_withName_dflt(const Identifier& p_name)
  {
    chk();
    if(is_erroneous) return true;
    if(fss.has_key(p_name.get_name())) return true;
    if(my_governor->get_fss()->has_fs_withId(p_name)
       && my_governor->get_fss()->get_fs_byId(p_name)->has_default())
      return true;
    return false;
  }

  Setting* Obj_defn::get_setting_byName_dflt(const Identifier& p_name)
  {
    chk();
    const string& s=p_name.get_name();
    if(fss.has_key(s))
      return fss[s]->get_setting();
    if(my_governor->get_fss()->has_fs_withId(p_name)) {
      FieldSpec *fs=my_governor->get_fss()->get_fs_byId(p_name);
      if(fs->has_default())
        return fs->get_default();
    }
    if(!is_erroneous)
      error("No field setting or default with identifier `%s' in object `%s'",
            p_name.get_dispname().c_str(), get_fullname().c_str());
    return 0;
  }

  void Obj_defn::parse_block()
  {
    if(!block) return;
    if(!my_governor)
      FATAL_ERROR("Asn::Obj_defn::parse_block():"
                               " my_governor not set");
    OCSV_Parser *ocsv_parser=new OCSV_Parser(block->get_TokenBuf(), this);
    my_governor->get_ocs()->accept(*ocsv_parser);
    delete ocsv_parser;
    delete block; block=0;
  }

  void Obj_defn::generate_code(output_struct *target)
  {
    if(is_generated) return;
    is_generated=true;
    for(size_t i=0; i<fss.size(); i++)
      fss.get_nth_elem(i)->generate_code(target);
  }

  void Obj_defn::dump(unsigned level) const
  {
    DEBUG(level, "Object definition");
    if(block)
      DEBUG(level, "with unparsed block");
    else {
      DEBUG(level, "with field settings:");
      level++;
      for(size_t i=0; i<fss.size(); i++)
        fss.get_nth_elem(i)->dump(level);
    }
  }

  // =================================
  // ===== Obj_refd
  // =================================

  Obj_refd::Obj_refd(Reference *p_ref)
    : Object(), ref(p_ref), o_error(0), o_refd(0), o_defn(0)
  {
    if(!p_ref)
      FATAL_ERROR("NULL parameter: Asn::Obj_refd::Obj_refd()");
  }

  Obj_refd::Obj_refd(const Obj_refd& p)
    : Object(p), o_error(0), o_refd(0), o_defn(0)
  {
    ref=p.ref->clone();
  }

  Obj_refd::~Obj_refd()
  {
    delete ref;
    delete o_error;
  }

  void Obj_refd::set_my_scope(Scope *p_scope)
  {
    Object::set_my_scope(p_scope);
    ref->set_my_scope(p_scope);
  }

  void Obj_refd::set_fullname(const string& p_fullname)
  {
    Object::set_fullname(p_fullname);
    ref->set_fullname(p_fullname);
  }

  Obj_defn* Obj_refd::get_refd_last(ReferenceChain *refch)
  {
    if(!o_defn) {
      bool destroy_refch=false;
      if(!refch) {
        refch=new ReferenceChain(ref, "While searching referenced Object");
        destroy_refch=true;
      }
      o_defn=get_refd(refch)->get_refd_last(refch);
      if(destroy_refch) delete refch;
    }
    return o_defn;
  }

  Object* Obj_refd::get_refd(ReferenceChain *refch)
  {
    if(refch && !refch->add(get_fullname())) goto error;
    if(!o_refd) {
      Common::Assignment *ass=ref->get_refd_assignment();
      if(!ass) goto error;
      Setting *setting=ref->get_refd_setting();
      if (!setting || setting->get_st()==Setting::S_ERROR) goto error;
      o_refd=dynamic_cast<Object*>(setting);
      if(!o_refd) {
        error("This is not an objectreference: `%s'",
              ref->get_dispname().c_str());
        goto error;
      }
    }
    goto end;
  error:
    o_error=new Obj_defn();
    o_error->set_my_governor(my_governor);
    o_refd=o_error;
  end:
    return o_refd;
  }

  void Obj_refd::chk()
  {
    if(checked) return;
    checked=true;
    if(!my_governor)
      FATAL_ERROR("Asn::Obj_refd::chk()");
    if(get_refd_last()->get_my_governor()->get_refd_last()
       !=my_governor->get_refd_last()) {
      error("ObjectClass mismatch: Object of class `%s' was expected"
            " instead of `%s'",
            my_governor->get_refd_last()->get_fullname().c_str(),
            get_refd_last()->get_my_governor()->get_refd_last()
            ->get_fullname().c_str());
      o_error=new Obj_defn();
      o_error->set_is_erroneous();
      o_error->set_my_governor(my_governor);
      o_refd=o_error;
    }
  }

  void Obj_refd::generate_code(output_struct *target)
  {
    Obj_defn *refd_last = get_refd_last();
    if (my_scope->get_scope_mod_gen() ==
       refd_last->get_my_scope()->get_scope_mod_gen())
      refd_last->generate_code(target);
  }

  void Obj_refd::dump(unsigned level) const
  {
    DEBUG(level, "objectreference");
    level++;
    ref->dump(level);
  }

  // =================================
  // ===== OSE_Visitor
  // =================================

  OSE_Visitor *OSE_Visitor::clone() const
  {
    FATAL_ERROR("Asn::OSE_Visitor::clone()");
    return 0;
  }

  // =================================
  // ===== ObjectSet
  // =================================

  void ObjectSet::set_my_governor(ObjectClass *p_gov)
  {
    if(!p_gov)
      FATAL_ERROR("NULL parameter: Asn::ObjectSet::set_my_governor()");
    my_governor=p_gov;
  }

  // =================================
  // ===== Objects
  // =================================

  Objects::Objects(const Objects& p)
    : Node(p)
  {
    objs.replace(0, 0, &p.objs);
  }

  Objects::~Objects()
  {
    objs.clear();
  }

  void Objects::add_objs(Objects *p_objs)
  {
    if(!p_objs)
      FATAL_ERROR("NULL parameter: Asn::Objects::add_objs()");
    objs.replace(objs.size(), 0, &p_objs->objs);
  }

  void Objects::dump(unsigned level) const
  {
    for(size_t i=0; i<objs.size(); i++)
      objs[i]->dump(level);
  }

  // =================================
  // ===== OSEV_objcollctr
  // =================================

  OSEV_objcollctr::OSEV_objcollctr(ObjectSet& parent)
    : OSE_Visitor(0)
  {
    governor=parent.get_my_governor()->get_refd_last();
    loc=&parent;
    objs=new Objects();
    visdes.add(parent.get_refd_last(), 0);
  }

  OSEV_objcollctr::OSEV_objcollctr(const Location *p_loc,
                                   ObjectClass *p_governor)
    : OSE_Visitor(p_loc)
  {
    if(!p_governor)
      FATAL_ERROR("Asn::OSEV_objcollctr::OSEV_objcollctr()");
    governor=p_governor->get_refd_last();
    objs=new Objects();
  }

  OSEV_objcollctr::~OSEV_objcollctr()
  {
    delete objs;
    visdes.clear();
  }

  void OSEV_objcollctr::visit_Object(Object& p)
  {
    Obj_defn *o = p.get_refd_last();
    if(o->get_is_erroneous()) return;
    if(visdes.has_key(o)) return;
    if(o->get_my_governor()->get_refd_last()!=governor) {
      loc->error("Objects of objectclass `%s' are expected; "
                 "`%s' is object of class `%s'",
                 governor->get_fullname().c_str(),
                 p.get_fullname().c_str(),
                 p.get_my_governor()->get_refd_last()->get_fullname().c_str());
      return;
    }
    visdes.add(o, 0);
    objs->add_o(o);
  }

  void OSEV_objcollctr::visit_OS_refd(OS_refd& p)
  {
    visit_ObjectSet(p);
  }

  void OSEV_objcollctr::visit_ObjectSet(ObjectSet& p, bool force)
  {
    OS_defn& os=*p.get_refd_last();
    if(os.get_my_governor()->get_refd_last()!=governor) {
      loc->error("Objects of objectclass `%s' are expected; "
                 "`%s' is objectset of class `%s'",
                 governor->get_fullname().c_str(),
                 p.get_fullname().c_str(),
                 os.get_my_governor()->get_refd_last()->get_fullname().c_str());
      return;
    }
    if(visdes.has_key(&os)) {
      if(!force) return;
    }
    else visdes.add(&os, 0);
    Objects *other_objs=os.get_objs();
    for(size_t i=0; i<other_objs->get_nof_objs(); i++)
      visit_Object(*other_objs->get_obj_byIndex(i));
  }

  Objects* OSEV_objcollctr::give_objs()
  {
    Objects *tmp_objs=objs;
    objs=0;
    return tmp_objs;
  }

  // =================================
  // ===== OSEV_checker
  // =================================

  OSEV_checker::OSEV_checker(const Location *p_loc, ObjectClass *p_governor)
    : OSE_Visitor(p_loc), governor(p_governor)
  {
    if(!p_governor)
      FATAL_ERROR("NULL parameter: Asn::OSEV_checker::OSEV_checker()");
    gov_defn=p_governor->get_refd_last();
  }

  void OSEV_checker::visit_Object(Object& p)
  {
    p.set_my_governor(governor);
    p.chk();
  }

  void OSEV_checker::visit_OS_refd(OS_refd& p)
  {
    p.set_my_governor(governor);
    p.chk();
  }

  // =================================
  // ===== OSEV_codegen
  // =================================

  void OSEV_codegen::visit_Object(Object& p)
  {
    p.generate_code(target);
  }

  void OSEV_codegen::visit_OS_refd(OS_refd& p)
  {
    p.generate_code(target);
  }

  // =================================
  // ===== OS_defn
  // =================================

  OS_defn::OS_defn()
    : ObjectSet(), block(0), objs(0), is_generated(false)
  {
    oses=new vector<OS_Element>();
  }

  OS_defn::OS_defn(Block *p_block)
    : ObjectSet(), block(p_block), objs(0), is_generated(false)
  {
    if(!p_block)
      FATAL_ERROR("NULL parameter: Asn::OS_defn::OS_defn()");
    oses=new vector<OS_Element>();
  }

  OS_defn::OS_defn(Objects *p_objs)
    : ObjectSet(), block(0), objs(p_objs), is_generated(false)
  {
    if(!p_objs)
      FATAL_ERROR("NULL parameter: Asn::OS_defn::OS_defn()");
    oses=new vector<OS_Element>();
  }

  OS_defn::OS_defn(const OS_defn& p)
    : ObjectSet(p), objs(0), is_generated(false)
  {
    block=p.block?p.block->clone():0;
    oses=new vector<OS_Element>();
    for(size_t i=0; i<p.oses->size(); i++)
      oses->add((*p.oses)[i]->clone_ose());
  }

  OS_defn::~OS_defn()
  {
    delete block;
    delete objs;
    for(size_t i=0; i<oses->size(); i++)
      delete (*oses)[i];
    oses->clear();
    delete oses;
  }

  void OS_defn::steal_oses(OS_defn *other_os)
  {
    if(!oses || objs || !other_os || !other_os->oses || other_os->objs)
      FATAL_ERROR("Asn::OS_defn::steal_oses()");
    oses->replace(oses->size(), 0, other_os->oses);
    other_os->oses->clear();
    delete other_os;
  }

  OS_defn* OS_defn::get_refd_last(ReferenceChain *refch)
  {
    if(oses->size()!=1) return this;
    OS_refd* ref=dynamic_cast<OS_refd*>((*oses)[0]);
    if(!ref) return this;
    OS_defn* reflast=0;
    bool destroy_refch=false;
    if(!refch) {
      refch=new ReferenceChain
        (this, "While searching quasi-referenced ObjectSet");
      destroy_refch=true;
    }
    refch->add(get_fullname());
    reflast=ref->get_refd_last(refch);
    if(destroy_refch) delete refch;
    return reflast;
  }

  void OS_defn::set_fullname(const string& p_fullname)
  {
    ObjectSet::set_fullname(p_fullname);
    for(size_t i=0; i<oses->size(); i++)
      (*oses)[i]->set_fullname_ose(p_fullname+"."+Int2string(i+1));
  }

  void OS_defn::set_my_scope(Scope *p_scope)
  {
    ObjectSet::set_my_scope(p_scope);
    for(size_t i=0; i<oses->size(); i++)
      (*oses)[i]->set_my_scope_ose(p_scope);
  }

  void OS_defn::add_ose(OS_Element *p_ose)
  {
    if(!p_ose)
      FATAL_ERROR("NULL parameter: Asn::OS_defn::add_ose");
    oses->add(p_ose);
  }

  size_t OS_defn::get_nof_objs()
  {
    if(!objs) create_objs();
    return objs->get_nof_objs();
  }

  Object* OS_defn::get_obj_byIndex(size_t p_i)
  {
    if(!objs) create_objs();
    return objs->get_obj_byIndex(p_i);
  }

  Objects* OS_defn::get_objs()
  {
    if(!objs) create_objs();
    return objs;
  }

  void OS_defn::parse_block()
  {
    if(!block) return;
    Node *node=block->parse(KW_Block_ObjectSetSpec);
    OS_defn *tmp=dynamic_cast<OS_defn*>(node);
    if(tmp){
		set_location(*tmp);
		delete block; block=0;
		delete oses;
		oses=tmp->oses;
		tmp->oses=new vector<OS_Element>();
		delete tmp;
    }else{
    	delete block; block=0;
		delete oses;
    	oses = new vector<OS_Element>();
    }
    set_fullname(get_fullname());
    set_my_scope(get_my_scope());
  }

  void OS_defn::chk()
  {
    if(checked) return;
    if(block) parse_block();
    if(!my_governor)
      FATAL_ERROR("Asn::OS_defn::chk()");
    Error_Context cntxt(this, "In objectset definition `%s'",
                        get_fullname().c_str());
    OSEV_checker osev(this, my_governor);
    for(size_t i=0; i<oses->size(); i++) {
      OS_Element *ose=(*oses)[i];
      ose->set_genname_ose(get_genname_own(), Int2string(i + 1));
      ose->accept(osev);
    }
    checked=true;
    //create_objs();
  }

  void OS_defn::create_objs()
  {
    if(objs) return;
    if(!my_governor)
      FATAL_ERROR("Asn::OS_defn::create_objs()");
    if(!checked) chk();
    Error_Context cntxt(this, "While exploring objectset definition `%s'",
                        get_fullname().c_str());
    OSEV_objcollctr osev(*this);
    for(size_t i=0; i<oses->size(); i++)
      (*oses)[i]->accept(osev);
    objs=osev.give_objs();
  }

  void OS_defn::generate_code(output_struct *target)
  {
    if (is_generated) return;
    is_generated = true;
    OSEV_codegen osev(this, target);
    for(size_t i=0; i<oses->size(); i++)
      (*oses)[i]->accept(osev);
  }

  void OS_defn::dump(unsigned level) const
  {
    DEBUG(level, "ObjectSet definition");
    if(block) DEBUG(level, "with unparsed block");
    if(objs) {
      DEBUG(level, "with collected objects (%lu pcs)",
        static_cast<unsigned long>( objs->get_nof_objs()));
      objs->dump(level+1);
    }
    else DEBUG(level, "with uncollected objects");
  }

  // =================================
  // ===== OS_refd
  // =================================

  OS_refd::OS_refd(Reference *p_ref)
    : ObjectSet(), OS_Element(), ref(p_ref), os_error(0),
      os_refd(0), os_defn(0)
  {
    if(!p_ref)
      FATAL_ERROR("NULL parameter: Asn::OS_refd::OS_refd()");
  }

  OS_refd::OS_refd(const OS_refd& p)
    : ObjectSet(p), OS_Element(p), os_error(0), os_refd(0), os_defn(0)
  {
    ref=p.ref->clone();
  }

  string OS_refd::create_stringRepr()
  {
    return get_refd_last()->get_stringRepr();
  }

  OS_refd::~OS_refd()
  {
    delete ref;
    delete os_error;
  }

  void OS_refd::set_my_scope(Scope *p_scope)
  {
    ObjectSet::set_my_scope(p_scope);
    ref->set_my_scope(p_scope);
  }

  ObjectSet* OS_refd::get_refd(ReferenceChain *refch)
  {
    if(refch) refch->add(get_fullname());
    if(!os_refd) {
      Setting *setting=0;
      Common::Assignment *ass=ref->get_refd_assignment();
      if(!ass) goto error;
      setting=ref->get_refd_setting();
      if(setting->get_st()==Setting::S_ERROR) goto error;
      os_refd=dynamic_cast<ObjectSet*>(setting);
      if(!os_refd) {
        error("This is not an objectsetreference: `%s'",
              ref->get_dispname().c_str());
        goto error;
      }
      goto end;
    error:
      /* create an empty ObjectSet */
      os_error=new OS_defn();
      os_error->set_my_governor(my_governor);
      os_refd=os_error;
    }
  end:
    return os_refd;
  }

  OS_defn* OS_refd::get_refd_last(ReferenceChain *refch)
  {
    if(!os_defn) {
      bool destroy_refch=false;
      if(!refch) {
        refch=new ReferenceChain(ref, "While searching referenced ObjectSet");
        destroy_refch=true;
      }
      ObjectSet *t_os = get_refd(refch);
      os_defn= t_os->get_refd_last(refch);
      if(destroy_refch) delete refch;
    }
    return os_defn;
  }

  size_t OS_refd::get_nof_objs()
  {
    return get_refd_last()->get_nof_objs();
  }

  Object* OS_refd::get_obj_byIndex(size_t p_i)
  {
    return get_refd_last()->get_obj_byIndex(p_i);
  }

  void OS_refd::chk()
  {
    if(checked) return;
    checked=true;
    if(!my_governor)
      FATAL_ERROR("Asn::OS_refd::chk()");
    ObjectClass *my_class = my_governor->get_refd_last();
    ObjectClass *refd_class =
      get_refd_last()->get_my_governor()->get_refd_last();
    if (my_class != refd_class) {
      error("ObjectClass mismatch: ObjectSet of class `%s' was expected "
            "instead of `%s'", my_class->get_fullname().c_str(),
            refd_class->get_fullname().c_str());
      os_error=new OS_defn();
      os_error->set_my_governor(my_governor);
      os_refd=os_error;
    }
  }

  void OS_refd::generate_code(output_struct *target)
  {
    OS_defn *refd_last = get_refd_last();
    if(my_scope->get_scope_mod_gen() ==
       refd_last->get_my_scope()->get_scope_mod_gen())
      refd_last->generate_code(target);
  }

  void OS_refd::dump(unsigned level) const
  {
    DEBUG(level, "objectsetreference");
    level++;
    ref->dump(level);
  }

  OS_Element *OS_refd::clone_ose() const
  {
    return clone();
  }

  void OS_refd::set_fullname_ose(const string& p_fullname)
  {
    set_fullname(p_fullname);
  }

  void OS_refd::set_genname_ose(const string& p_prefix, const string& p_suffix)
  {
    set_genname(p_prefix, p_suffix);
  }

  void OS_refd::set_my_scope_ose(Scope *p_scope)
  {
    set_my_scope(p_scope);
  }

} // namespace Asn
