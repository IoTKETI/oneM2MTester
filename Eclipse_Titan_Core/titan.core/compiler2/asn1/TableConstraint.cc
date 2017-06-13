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
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include "TableConstraint.hh"
#include "../Identifier.hh"
#include "Object.hh"
#include "Ref.hh"
#include "TokenBuf.hh"
// FIXME TableConstraint does not use Token or TokenBuf at all.
// TokenBuf.hh just happens to drag in everything that TableConstraint needs.

#include "../CompField.hh"

namespace Asn {

  // =================================
  // ===== AtNotation
  // =================================

  AtNotation::AtNotation(int p_levels, FieldName *p_cids)
    : Node(), levels(p_levels), cids(p_cids), oc_fieldname(0),
      firstcomp(0), lastcomp(0)
  {
    if(!p_cids)
      FATAL_ERROR("NULL parameter: Asn::AtNotation::AtNotation()");
  }

  AtNotation::AtNotation(const AtNotation& p)
    : Node(p), levels(p.levels), oc_fieldname(0),
      firstcomp(0), lastcomp(0)
  {
    cids=p.cids->clone();
  }

  AtNotation::~AtNotation()
  {
    delete cids;
    delete oc_fieldname;
  }

  void AtNotation::set_oc_fieldname(const Identifier& p_oc_fieldname)
  {
    if(oc_fieldname)
      FATAL_ERROR("Asn::AtNotation::set_ocfieldname()");
    oc_fieldname=p_oc_fieldname.clone();
  }

  string AtNotation::get_dispname() const
  {
    string s('@');
    for(int i=0; i<levels; i++) s+=".";
    s+=cids->get_dispname();
    return s;
  }

  // =================================
  // ===== AtNotations
  // =================================

  AtNotations::AtNotations(const AtNotations& p)
    : Node(p)
  {
    for(size_t i=0; i<p.ans.size(); i++)
      ans.add(p.ans[i]->clone());
  }

  AtNotations::~AtNotations()
  {
    for(size_t i=0; i<ans.size(); i++) delete ans[i];
    ans.clear();
  }

  void AtNotations::add_an(AtNotation *p_an)
  {
    if(!p_an)
      FATAL_ERROR("NULL parameter: Asn::AtNotations::add_an()");
    ans.add(p_an);
  }

  // =================================
  // ===== TableConstraint
  // =================================

  TableConstraint::TableConstraint(Block *p_block_os, Block *p_block_ans)
    : Constraint(CT_TABLE), block_os(p_block_os), block_ans(p_block_ans),
      os(0), ans(0), consdtype(0), oc_fieldname(0)
  {
    if(!block_os)
      FATAL_ERROR("NULL parameter: Asn::TableConstraint::TableConstraint()");
  }

  TableConstraint::TableConstraint(const TableConstraint& p)
    : Constraint(p), consdtype(0), oc_fieldname(0)
  {
    block_os=p.block_os?p.block_os->clone():0;
    block_ans=p.block_ans?p.block_ans->clone():0;
    os=p.os?p.os->clone():0;
    ans=p.ans?p.ans->clone():0;
  }

  TableConstraint::~TableConstraint()
  {
    delete block_os;
    delete block_ans;
    delete os;
    delete ans;
  }

  void TableConstraint::chk()
  {
    if(checked) return;
    checked=true;
    parse_blocks();
    if(!my_type)
      FATAL_ERROR("Asn::TableConstraint::chk()");
    os->set_my_scope(my_type->get_my_scope());
    os->set_fullname(my_type->get_fullname()+".<tableconstraint-os>");
    os->set_genname(my_type->get_genname_own(), string("os"));
    // search the constrained type (not the reference to it)
    consdtype=my_type;
    bool ok=false;
    while(true) {
      Type::typetype_t t=consdtype->get_typetype();
      if(t==Type::T_OPENTYPE || t==Type::T_OCFT) {
        ok=true;
        break;
      }
      else if(t==Type::T_ERROR) {
        return;
      }
      else if(consdtype->is_ref())
        consdtype=consdtype->get_type_refd();
      else break;
    }
    if(!ok) {
      my_type->error("TableConstraint can only be applied to"
                     " ObjectClassFieldType");
      return;
    }
    // consdtype is ok
    if(consdtype->get_typetype()==Type::T_OCFT) {
      // componentrelationconstraint ignored for this...
      oc_fieldname=&consdtype->get_oc_fieldname();
      os->set_my_governor(consdtype->get_my_oc());
      os->chk();
    }
    else if(consdtype->get_typetype()==Type::T_OPENTYPE) {
      consdtype->set_my_tableconstraint(this);
      oc_fieldname=&consdtype->get_oc_fieldname();
      os->set_my_governor(consdtype->get_my_oc());
      os->chk();
      if(ans) {
        // componentrelationconstraint...
        // search the outermost textually enclosing seq, set or choice
        Type *outermostparent=0;
        Type *t_type=my_type;
        do {
          if(t_type->is_secho())
            outermostparent=t_type;
        } while((t_type=t_type->get_parent_type()));
        if(!outermostparent) {
          my_type->error("Invalid use of ComponentRelationConstraint"
                         " (cannot determine parent type)");
          return;
        }
        outermostparent->set_opentype_outermost();
        // set has_opentypes for enclosing types
        t_type=my_type;
        do {
          t_type=t_type->get_parent_type();
          t_type->set_has_opentypes();
        } while(t_type!=outermostparent);
        // checking of atnotations
        for(size_t i=0; i<ans->get_nof_ans(); i++) {
          AtNotation *an=ans->get_an_byIndex(i);
          Type *parent = NULL;
          if(an->get_levels()==0) parent=outermostparent;
          else {
            parent=my_type;
            for(int level=an->get_levels(); level>0; level--) {
              parent=parent->get_parent_type();
              if(!parent) {
                my_type->error("Too many dots. This component has only"
                               " %d parents.", an->get_levels()-level);
                return;
              } // ! parent
            } // for level
          } // not the outermost
          t_type=parent;
          an->set_firstcomp(parent);
          // component identifiers... do they exist?
          FieldName* cids=an->get_cids();
          for(size_t j=0; j<cids->get_nof_fields(); j++) {
            if(!t_type->is_secho()) {
              my_type->error("Type `%s' is not a SEQUENCE, SET of CHOICE"
                             " type.", t_type->get_fullname().c_str());
              return;
            } // ! secho
            const Identifier *id=cids->get_field_byIndex(j);
            if(!t_type->has_comp_withName(*id)) {
              my_type->error("Type `%s' has no component with name `%s'.",
                             t_type->get_fullname().c_str(),
                             id->get_dispname().c_str());
              return;
            } // no component with that name
            t_type=t_type->get_comp_byName(*id)->get_type();
          } // for j
          an->set_lastcomp(t_type);
          /* check if the referenced component is constrained by the
           * same objectset... */
          ok=false;
          do {
            // t_type->chk_constraints();
            Constraints *t_cons=t_type->get_constraints();
            if(!t_cons) break;
            t_cons->chk_table();
            TableConstraint *t_tc=dynamic_cast<TableConstraint*>
              (t_cons->get_tableconstraint());
            if(!t_tc) break;
            Type *t_ocft=t_tc->consdtype;
            if(t_ocft->get_typetype()!=Type::T_OCFT) break;
            an->set_oc_fieldname(t_ocft->get_oc_fieldname());
            // is the same objectset?
            if(t_tc->os->get_refd_last()!=os->get_refd_last()) break;
            ok=true;
          } while(false);
          if(!ok) {
            my_type->error
              ("The referenced components must be value (set) fields"
               " constrained by the same objectset"
               " as the referencing component");
            return;
          }
        } // for i
          // well, the atnotations seems to be ok, let's produce the
          // alternatives for the opentype
        Type *t_ot=consdtype; // opentype
        DEBUG(1, "Adding alternatives to open type `%s'",
              t_ot->get_fullname().c_str());
        Objects *objs=os->get_refd_last()->get_objs();
        for(size_t i=0; i<objs->get_nof_objs(); i++) {
          Obj_defn *obj=objs->get_obj_byIndex(i);
          if(!obj->has_fs_withName_dflt(*oc_fieldname))
            continue;
          t_type=dynamic_cast<Type*>
            (obj->get_setting_byName_dflt(*oc_fieldname));
	  bool is_strange;
	  const Common::Identifier& altname = t_type->get_otaltname(is_strange);
          if(!t_ot->has_comp_withName(altname)) {
            Type * otype = new Type(Type::T_REFDSPEC, t_type);
            otype->set_genname(t_type->get_genname_own());
            t_ot->add_comp(new CompField(altname.clone(),
					 otype));
	    const char *dispname_str = altname.get_dispname().c_str();
	    DEBUG(2, "Alternative `%s' added", dispname_str);
	    if (is_strange)
	      t_ot->warning("Strange alternative name (`%s') was added to "
		"open type `%s'", dispname_str, t_ot->get_fullname().c_str());
          } // t_ot ! has the type
        } // for i (objs)
        t_ot->set_my_scope(t_ot->get_my_scope());
        t_ot->set_fullname(t_ot->get_fullname());
        t_ot->chk();
      } // if ans
    } // opentype check

    // this cannot be inside another constraint
    if (my_parent) {
      error("Table constraint cannot be inside a %s", my_parent->get_name());
    }

  } // member function

  void TableConstraint::parse_blocks()
  {
    if(block_os) {
      if(block_ans) { // ComponentRelationConstraint
        Node *node=block_os->parse(KW_Block_DefinedObjectSetBlock);
        os=dynamic_cast<ObjectSet*>(node);
        delete block_os; block_os=0;
        node=block_ans->parse(KW_Block_AtNotationList);
        ans=dynamic_cast<AtNotations*>(node);
        delete block_ans; block_ans=0;
        if(!ans)
          /* syntax error */
          ans=new AtNotations();
      }
      else { // SimpleTableConstraint
        Node *node=block_os->parse(KW_Block_ObjectSetSpec);
        os=dynamic_cast<ObjectSet*>(node);
        delete block_os; block_os=0;
      }
      if(!os) {
        /* syntax error */
        os=new OS_defn();
      }
    }
  }

} // namespace Asn
