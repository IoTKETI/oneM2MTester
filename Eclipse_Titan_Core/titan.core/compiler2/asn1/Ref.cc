/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Cserveni, Akos
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include "Ref.hh"
#include "../Identifier.hh"
#include "AST_asn1.hh"
#include "Block.hh"
#include "TokenBuf.hh"

namespace Asn {

  // =================================
  // ===== Ref_pard
  // =================================

  Ref_pard::Ref_pard(Ref_defd_simple *p_refds, Block *p_block)
    : Ref_defd(), ref_parass(p_refds), block(p_block),
      refd_ass_is_not_pard(false), asss(0), ref_ds(0)
  {
    if (!p_refds || !p_block)
      FATAL_ERROR("NULL parameter: Asn::Ref_pard::Ref_pard()");
  }

  Ref_pard::Ref_pard(const Ref_pard& p)
    : Ref_defd(p), refd_ass_is_not_pard(p.refd_ass_is_not_pard),
      asss(0), ref_ds(0)
  {
    ref_parass = p.ref_parass->clone();
    block=p.block?p.block->clone():0;
    asss=p.asss?p.asss->clone():0;
  }

  Ref_pard::~Ref_pard()
  {
    delete ref_parass;
    delete block;
    delete asss;
    delete ref_ds;
  }

  Ref_pard *Ref_pard::clone() const
  {
    return new Ref_pard(*this);
  }

  void Ref_pard::set_fullname(const string& p_fullname)
  {
    Ref_defd::set_fullname(p_fullname);
    ref_parass->set_fullname(p_fullname);
    if (block) block->set_fullname(p_fullname + ".<block>");
    if (asss) asss->set_fullname(p_fullname + "." +
      ref_parass->get_dispname() + "{}");
    if (ref_ds) ref_ds->set_fullname(p_fullname);
  }

  void Ref_pard::set_my_scope(Scope *p_scope)
  {
    Ref_defd::set_my_scope(p_scope);
    ref_parass->set_my_scope(p_scope);
    if (ref_ds) ref_ds->set_my_scope(p_scope);
  }

  bool Ref_pard::get_is_erroneous()
  {
    get_ref_defd_simple();
    return is_erroneous;
  }

  string Ref_pard::get_dispname()
  {
    if (ref_ds) return ref_ds->get_dispname();
    else return ref_parass->get_dispname()+"{}";
  }

  Ref_defd_simple* Ref_pard::get_ref_defd_simple()
  {
    if(ref_ds) return ref_ds;
    if(is_erroneous) return 0;
    if(refd_ass_is_not_pard) return ref_parass;
    Module *my_mod = dynamic_cast<Module*>(my_scope->get_scope_mod());
    if(!my_mod) FATAL_ERROR("Asn::Ref_pard::get_ref_defd_simple()");

    /* search the referenced parameterized assignment */
    Assignment *parass=ref_parass->get_refd_ass();
    if(!parass) {
      is_erroneous=true;
      delete block;
      block=0;
      return 0;
    }
    Ass_pard *ass_pard=parass->get_ass_pard();
    if(!ass_pard) {
      /* error: this is not a parameterized assignment */
      refd_ass_is_not_pard=true;
      delete block;
      block=0;
      return ref_parass;
    }
    size_t nof_formal_pars=ass_pard->get_nof_pars();
    if(nof_formal_pars==0) {
      /* error in parameterized assignment */
      is_erroneous=true;
      return 0;
    }

    Error_Context cntxt
      (this, "While instantiating parameterized reference `%s'",
       ref_parass->get_dispname().c_str());

    if(block) {
      /* splitting the actual parameterlist */
      vector<TokenBuf> act_pars; // actual parameters
      {
        TokenBuf *act_pars_tb=block->get_TokenBuf();
        TokenBuf *tmp_tb=new TokenBuf();
        for(bool ok=false; !ok; ) {
          Token *token=act_pars_tb->pop_front_token();
          switch(token->get_token()) {
          case '\0': // EOF
            ok=true;
            // no break
          case ',':
            token->set_token('\0');
            tmp_tb->push_back_token(token);
            act_pars.add(tmp_tb);
            tmp_tb=new TokenBuf();
            break;
          default:
            tmp_tb->push_back_token(token);
          } // switch(token->token)
        }
        delete tmp_tb;
        delete block;
        block=0;
      }

      /* checking the number of parameters */
      size_t nof_act_pars=act_pars.size();
      if(nof_act_pars != nof_formal_pars)
        error("Too %s parameters: %lu was expected instead of %lu",
              nof_act_pars<nof_formal_pars?"few":"many",
              static_cast<unsigned long>( nof_formal_pars), static_cast<unsigned long>( nof_act_pars));

      /* building the new assignments */
      asss=new Assignments();
      for(size_t i=0; i<nof_formal_pars; i++) {
        const Identifier& tmp_id=ass_pard->get_nth_dummyref(i);
        Error_Context cntxt2(this, "While examining parameter `%s'",
                             tmp_id.get_dispname().c_str());
        Assignment *tmp_ass=0;
        if(i<nof_act_pars) {
          TokenBuf *tmp_tb=ass_pard->clone_nth_governor(i);
          tmp_tb->move_tokens_from(act_pars[i]);
          tmp_tb->push_back_kw_token('\0');
          Block *tmp_block=new Block(tmp_tb);
          Node *tmp_node=tmp_block->parse(KW_Block_Assignment);
          delete tmp_block;
          tmp_ass=dynamic_cast<Assignment*>(tmp_node);
          if(!tmp_ass) delete tmp_node;
        }
        // substitution of missing parameters (for error recovery)
        if(!tmp_ass) tmp_ass=new Ass_Error(tmp_id.clone(), 0);
        tmp_ass->set_location(*this);
        asss->add_ass(tmp_ass);
      } // for i

      for(size_t i=0; i<nof_act_pars; i++) delete act_pars[i];
      act_pars.clear();
    }

    Assignment *new_ass = parass->new_instance(my_scope->get_scope_mod_gen());
    my_mod->add_ass(new_ass);
    const Identifier& new_ass_id = new_ass->get_id();
    const string& new_ass_dispname = new_ass_id.get_dispname();

    asss->set_right_scope(my_scope);
    asss->set_parent_scope(parass->get_my_scope());
    asss->set_parent_scope_gen(my_scope);
    asss->set_scope_name(new_ass_dispname);
    asss->set_fullname(get_fullname() + "." + ref_parass->get_dispname() +
      "{}");
    asss->chk();

    new_ass->set_fullname(my_mod->get_fullname() + "." + new_ass_dispname);
    new_ass->set_my_scope(asss);
    new_ass->set_location(*this);
    new_ass->set_dontgen();
    new_ass->chk();
    
    if (Common::Assignment::A_TYPE == new_ass->get_asstype()) {
      new_ass->get_Type()->set_pard_type_instance();
    }
    
    ref_ds=new Ref_defd_simple(new Identifier(my_mod->get_modid()),
                               new Identifier(new_ass_id));
    ref_ds->set_fullname(get_fullname());
    ref_ds->set_my_scope(my_mod);
    return ref_ds;
  }

  void Ref_pard::dump(unsigned level) const
  {
    DEBUG(level, "Parameterized reference: %s", const_cast<Ref_pard*>(this)->get_dispname().c_str());
    if(asss) {
      DEBUG(level, "with parameters");
      level++;
      asss->dump(level);
    }
  }

  // =================================
  // ===== FieldName
  // =================================

  FieldName::FieldName(const FieldName& p)
    : Node(p)
  {
    for(size_t i=0; i<p.fields.size(); i++)
      add_field(p.fields[i]->clone());
  }

  FieldName::~FieldName()
  {
    for(size_t i=0; i<fields.size(); i++) delete fields[i];
    fields.clear();
  }

  FieldName *FieldName::clone() const
  {
    return new FieldName(*this);
  }

  void FieldName::add_field(Identifier *p_id)
  {
    if(!p_id)
      FATAL_ERROR("NULL parameter: Asn::FieldName::add_field()");
    fields.add(p_id);
  }

  string FieldName::get_dispname() const
  {
    string s;
    size_t nof_fields = fields.size();
    for (size_t i = 0; i < nof_fields; i++) {
      s += '.';
      s += fields[i]->get_dispname();
    }
    return s;
  }

  // =================================
  // ===== FromObj
  // =================================

  FromObj::FromObj(Ref_defd *p_ref_defd, FieldName *p_fn)
    : Common::Reference(), setting(0), setting_cache(0)
  {
    if (!p_ref_defd || !p_fn)
      FATAL_ERROR("NULL parameter: Asn::FromObj::FromObj()");
    ref_defd=p_ref_defd;
    fn=p_fn;
  }

  FromObj::FromObj(const FromObj& p)
    : Common::Reference(p), setting(0), setting_cache(0)
  {
    ref_defd=p.ref_defd->clone();
    fn=p.fn->clone();
  }

  FromObj::~FromObj()
  {
    delete ref_defd;
    delete fn;
    delete setting;
  }

  FromObj *FromObj::clone() const
  {
    return new FromObj(*this);
  }

  void FromObj::set_fullname(const string& p_fullname)
  {
    Common::Reference::set_fullname(p_fullname);
    ref_defd->set_fullname(p_fullname);
    fn->set_fullname(p_fullname + ".<fieldnames>");
  }

  void FromObj::set_my_scope(Scope *p_scope)
  {
    Common::Reference::set_my_scope(p_scope);
    ref_defd->set_my_scope(p_scope);
  }

  string FromObj::get_dispname()
  {
    return ref_defd->get_dispname()+fn->get_dispname();
  }

  bool FromObj::get_is_erroneous()
  {
    get_refd_setting();
    return is_erroneous;
  }

  /** \todo enhance */
  Setting* FromObj::get_refd_setting()
  {
    if(setting_cache) return setting_cache;
    Error_Context ec_0
      (this, "In InformationFromObjects construct `%s'",
       get_dispname().c_str());
    enum { UNKNOWN, OC, OS, O } curr = UNKNOWN;
    OC_defn *t_oc=0;
    OS_defn *t_os=0;
    Obj_defn *t_o=0;
    OS_defn *fromobjs = 0; // temporary storage
    size_t nof_fields = fn->get_nof_fields();
    Setting *t_setting = ref_defd->get_refd_setting();
    if(!t_setting) goto error;

    /* the first part */
    switch(t_setting->get_st()) {
    case Setting::S_OS: {
      curr=OS;
      t_os=dynamic_cast<ObjectSet*>(t_setting)->get_refd_last();
      t_oc=t_os->get_my_governor()->get_refd_last();
      OSEV_objcollctr objcoll(t_os, t_oc);
      objcoll.visit_ObjectSet(*t_os);
      fromobjs=new OS_defn(objcoll.give_objs());
      fromobjs->set_my_governor(t_oc);
      break; }
    case Setting::S_O:
      curr=O;
      t_o=dynamic_cast<Object*>(t_setting)->get_refd_last();
      t_oc=t_o->get_my_governor()->get_refd_last();
      break;
    case Setting::S_OC:
      curr=OC;
      t_oc=dynamic_cast<ObjectClass*>(t_setting)->get_refd_last();
      break;
    case Setting::S_ERROR:
      goto error;
    default:
      error("Invalid reference `%s' "
            "(ObjectClass, ObjectSet or Object reference was expected)",
            get_dispname().c_str());
      goto error;
    } // switch st

    /* the middle part */
    for(size_t i=0; i<nof_fields-1; i++) {
      Identifier *curr_fn = fn->get_field_byIndex(i);
      Error_Context ec_1(this, "While examining part #%lu (`%s') of FieldName",
	static_cast<unsigned long> (i + 1), curr_fn->get_dispname().c_str());
      FieldSpec *curr_fspec =
	t_oc->get_fss()->get_fs_byId(*curr_fn)->get_last();
      if(curr_fspec->get_fstype()==FieldSpec::FS_ERROR) goto error;
      switch(curr) {
      case OC:
        switch(curr_fspec->get_fstype()) {
        case FieldSpec::FS_O: {
          FieldSpec_O *tmp_fspec=
            dynamic_cast<FieldSpec_O*>(curr_fspec);
          t_oc=tmp_fspec->get_oc()->get_refd_last();
          break;}
        case FieldSpec::FS_OS: {
          FieldSpec_OS *tmp_fspec=
            dynamic_cast<FieldSpec_OS*>(curr_fspec);
          t_oc=tmp_fspec->get_oc()->get_refd_last();
          break;}
        case FieldSpec::FS_ERROR:
          goto error;
        default:
          error("This notation is not permitted (object or objectset"
                " fieldreference was expected)");
          goto error;
        } // switch fspec.fstype
        break;
      case OS:
        switch(curr_fspec->get_fstype()) {
        case FieldSpec::FS_O: {
          FieldSpec_O *tmp_fspec=
            dynamic_cast<FieldSpec_O*>(curr_fspec);
          t_oc=tmp_fspec->get_oc()->get_refd_last();
          OSEV_objcollctr objcoll(this, t_oc);
          Objects *t_objs = fromobjs->get_objs();
          for(size_t j=0; j<t_objs->get_nof_objs(); j++) {
            t_o=t_objs->get_obj_byIndex(j)->get_refd_last();
            if(!t_o->has_fs_withName_dflt(*curr_fn)) continue;
            t_setting=t_o->get_setting_byName_dflt(*curr_fn);
            t_o=dynamic_cast<Object*>(t_setting)->get_refd_last();
            objcoll.visit_Object(*t_o);
          }
          delete fromobjs;
          fromobjs=new OS_defn(objcoll.give_objs());
          fromobjs->set_location(*this);
          fromobjs->set_my_governor(t_oc);
          break;}
        case FieldSpec::FS_OS: {
          FieldSpec_OS *tmp_fspec=
            dynamic_cast<FieldSpec_OS*>(curr_fspec);
          t_oc=tmp_fspec->get_oc()->get_refd_last();
          OSEV_objcollctr objcoll(this, t_oc);
          Objects *t_objs = fromobjs->get_objs();
          for(size_t j=0; j<t_objs->get_nof_objs(); j++) {
            t_o=t_objs->get_obj_byIndex(j)->get_refd_last();
            if(!t_o->has_fs_withName_dflt(*curr_fn)) continue;
            t_setting=t_o->get_setting_byName_dflt(*curr_fn);
            t_os=dynamic_cast<ObjectSet*>(t_setting)->get_refd_last();
            objcoll.visit_ObjectSet(*t_os);
          }
          delete fromobjs;
          fromobjs=new OS_defn(objcoll.give_objs());
          fromobjs->set_location(*this);
          fromobjs->set_my_governor(t_oc);
          break;}
        case FieldSpec::FS_ERROR:
          goto error;
        default:
          error("This notation is not permitted (object or objectset"
                " fieldreference was expected)");
          goto error;
        } // switch fspec.fstype
        break;
      case O:
        switch(curr_fspec->get_fstype()) {
        case FieldSpec::FS_O: {
          FieldSpec_O *tmp_fspec=
            dynamic_cast<FieldSpec_O*>(curr_fspec);
          t_oc=tmp_fspec->get_oc()->get_refd_last();
          t_setting=t_o->get_setting_byName_dflt(*curr_fn);
          t_o=dynamic_cast<Object*>(t_setting)->get_refd_last();
          break;}
        case FieldSpec::FS_OS: {
          curr=OS;
          FieldSpec_OS *tmp_fspec=
            dynamic_cast<FieldSpec_OS*>(curr_fspec);
          t_oc=tmp_fspec->get_oc()->get_refd_last();
          OSEV_objcollctr objcoll(*fromobjs);
          Objects *t_objs = fromobjs->get_objs();
          for(size_t j=0; j<t_objs->get_nof_objs(); j++) {
            t_o=t_objs->get_obj_byIndex(j)->get_refd_last();
            if(!t_o->has_fs_withName_dflt(*curr_fn)) continue;
            t_setting=t_o->get_setting_byName_dflt(*curr_fn);
            t_os=dynamic_cast<ObjectSet*>(t_setting)->get_refd_last();
            objcoll.visit_ObjectSet(*t_os);
          }
          delete fromobjs;
          fromobjs=new OS_defn(objcoll.give_objs());
          fromobjs->set_location(*this);
          fromobjs->set_my_governor(t_oc);
          break;}
        case FieldSpec::FS_ERROR:
          goto error;
        default:
          error("This notation is not permitted (object or objectset"
                " fieldreference was expected)");
          goto error;
        } // switch fspec.fstype
        break;
      default:
        FATAL_ERROR("Asn::FromObj::get_refd_setting()");
      } // switch curr
    } // for i

    /* and the last part... */
    {
      Identifier *curr_fn = fn->get_field_byIndex(nof_fields-1);
      Error_Context ec_1
        (this,
         "While examining last part (`%s') of FieldName",
         curr_fn->get_dispname().c_str());
      FieldSpec *curr_fspec =
	t_oc->get_fss()->get_fs_byId(*curr_fn)->get_last();
      switch(curr) {
      case OC:
        switch(curr_fspec->get_fstype()) {
        case FieldSpec::FS_T: {
          Type *opentype = new Type(Type::T_OPENTYPE, t_oc, curr_fn);
          opentype->set_location(*this);
	  setting = opentype;
          break; }
        case FieldSpec::FS_V_FT: {
          FieldSpec_V_FT *tmp_fspec=
            dynamic_cast<FieldSpec_V_FT*>(curr_fspec);
          Type *ocft =
	    new Type(Type::T_OCFT, tmp_fspec->get_type(), t_oc, curr_fn);
          ocft->set_location(*this);
	  setting = ocft;
          break;}
        case FieldSpec::FS_V_VT:
        case FieldSpec::FS_VS_FT:
        case FieldSpec::FS_VS_VT:
          error("Sorry, this construct is not supported");
          goto error;
        case FieldSpec::FS_O:
        case FieldSpec::FS_OS:
          error("This notation is not permitted (type, (fixed- or"
                " variabletype) value or valueset fieldreference was"
                " expected)");
          // no break
        case FieldSpec::FS_ERROR:
          goto error;
        default:
          FATAL_ERROR("Unhandled fieldspec type");
        } // switch fspec.fstype
        break;
      case OS:
        switch(curr_fspec->get_fstype()) {
        case FieldSpec::FS_O: {
          FieldSpec_O *tmp_fspec=
            dynamic_cast<FieldSpec_O*>(curr_fspec);
          t_oc=tmp_fspec->get_oc()->get_refd_last();
          OSEV_objcollctr objcoll(this, t_oc);
          Objects *t_objs = fromobjs->get_objs();
          for(size_t j=0; j<t_objs->get_nof_objs(); j++) {
            t_o=t_objs->get_obj_byIndex(j)->get_refd_last();
            if(!t_o->has_fs_withName_dflt(*curr_fn)) continue;
            t_setting=t_o->get_setting_byName_dflt(*curr_fn);
            t_o=dynamic_cast<Object*>(t_setting)->get_refd_last();
            objcoll.visit_Object(*t_o);
          }
          delete fromobjs;
          fromobjs=new OS_defn(objcoll.give_objs());
          fromobjs->set_location(*this);
          fromobjs->set_my_governor(t_oc);
          fromobjs->set_my_scope(my_scope);
	  setting = fromobjs;
          break;}
        case FieldSpec::FS_OS: {
          FieldSpec_OS *tmp_fspec=
            dynamic_cast<FieldSpec_OS*>(curr_fspec);
          t_oc=tmp_fspec->get_oc()->get_refd_last();
          OSEV_objcollctr objcoll(this, t_oc);
          Objects *t_objs = fromobjs->get_objs();
          for(size_t j=0; j<t_objs->get_nof_objs(); j++) {
            t_o=t_objs->get_obj_byIndex(j)->get_refd_last();
            if(!t_o->has_fs_withName_dflt(*curr_fn)) continue;
            t_setting=t_o->get_setting_byName_dflt(*curr_fn);
            t_os=dynamic_cast<ObjectSet*>(t_setting)->get_refd_last();
            objcoll.visit_ObjectSet(*t_os);
          }
          delete fromobjs;
          fromobjs=new OS_defn(objcoll.give_objs());
          fromobjs->set_location(*this);
          fromobjs->set_my_governor(t_oc);
          fromobjs->set_my_scope(my_scope);
	  setting = fromobjs;
          break;}
        case FieldSpec::FS_V_FT:
        case FieldSpec::FS_VS_FT:
          error("Sorry, ValueSetFromObjects not supported");
          // no break
        case FieldSpec::FS_ERROR:
          goto error;
        default:
          error("This notation is not permitted (object, objectset,"
                " (fixed type) value or valueset fieldreference was"
                " expected)");
          goto error;
        } // switch fspec.fstype
        break;
      case O:
        setting_cache = t_o->get_setting_byName_dflt(*curr_fn);
        break;
      default:
        FATAL_ERROR("Asn::FromObj::get_refd_setting()");
      } // switch curr
    } // errorcontext-block
    if (!setting_cache) setting_cache = setting;
    goto end;
  error:
    delete fromobjs;
    is_erroneous=true;
    setting_cache=get_refd_setting_error();
  end:
    return setting_cache;
  }

  Common::Assignment* FromObj::get_refd_assignment(bool check_parlist)
  {
    return ref_defd->get_refd_assignment(check_parlist);
  }

  bool FromObj::has_single_expr()
  {
    FATAL_ERROR("FromObj::has_single_expr()");
    return false;
  }

  void FromObj::generate_code(expression_struct_t *)
  {
    FATAL_ERROR("FromObj::generate_code()");
  }

  void FromObj::generate_code_const_ref(expression_struct_t */*expr*/)
  {
    FATAL_ERROR("FromObj::generate_code_const_ref()");
  }

} // namespace Asn
