/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
Binary file (standard input) matches
 *
 ******************************************************************************/
#include "../common/dbgnew.hh"
#include "Type.hh"
#include "Typestuff.hh" // FIXME CTs
#include "CompField.hh"
#include "CompType.hh"
#include "TypeCompat.hh"
#include "EnumItem.hh"
#include "SigParam.hh"

#include "Valuestuff.hh"
#include "main.hh"

#include "ttcn3/Ttcnstuff.hh"
#include "ttcn3/TtcnTemplate.hh"
#include "ttcn3/Templatestuff.hh"
#include "ttcn3/Attributes.hh"
#include "ttcn3/ArrayDimensions.hh"
#include "ttcn3/PatternString.hh"

#include "asn1/Tag.hh"
#include "XerAttributes.hh"

#include <ctype.h>
#include <stdlib.h> // for qsort
#include <string.h>

extern int rawAST_debug;

namespace Common {

using Ttcn::MultiWithAttrib;
using Ttcn::SingleWithAttrib;
using Ttcn::WithAttribPath;
using Ttcn::Qualifier;
using Ttcn::Qualifiers;

void Type::chk()
{
  if(w_attrib_path) w_attrib_path->chk_global_attrib();
  parse_attributes();
  if(!tags_checked) {
    tags_checked = true;
    if(tags) tags->chk();
  }
  if (checked) return;
  switch(typetype) {
  case T_ERROR:
  case T_NULL:
  case T_BOOL:
  case T_INT:
  case T_REAL:
  case T_BSTR:
  case T_HSTR:
  case T_OSTR:
  case T_CSTR:
  case T_USTR:
  case T_UTF8STRING:
  case T_NUMERICSTRING:
  case T_PRINTABLESTRING:
  case T_TELETEXSTRING:
  case T_VIDEOTEXSTRING:
  case T_IA5STRING:
  case T_GRAPHICSTRING:
  case T_VISIBLESTRING:
  case T_GENERALSTRING:
  case T_UNIVERSALSTRING:
  case T_BMPSTRING:
  case T_UTCTIME:
  case T_GENERALIZEDTIME:
  case T_OBJECTDESCRIPTOR:
  case T_OID:
  case T_ROID:
  case T_ANY:
  case T_EXTERNAL:
  case T_EMBEDDED_PDV:
  case T_UNRESTRICTEDSTRING:
  case T_VERDICT:
  case T_DEFAULT:
    break;
  case T_INT_A:
    chk_Int_A();
    break;
  case T_ENUM_A:
    chk_Enum_A();
    break;
  case T_ENUM_T:
    chk_Enum_T();
    break;
  case T_BSTR_A:
    chk_BStr_A();
    break;
  case T_OPENTYPE:
    chk_SeCho_T();
    break;
  case T_ANYTYPE:
    // TODO maybe check for address type and add it automagically, then fall through
    if(!xerattrib) {
      xerattrib = new XerAttributes;
    }
    xerattrib->untagged_ = true;
  case T_SEQ_T:
  case T_SET_T:
  case T_CHOICE_T:
    chk_SeCho_T();
    // If this sequence type has no attributes but one of its fields does,
    // create an empty attribute structure.
    if(!rawattrib && hasVariantAttrs() && hasNeedofRawAttrs())
      rawattrib = new RawAST;
    if(!textattrib && hasVariantAttrs() && hasNeedofTextAttrs())
      textattrib = new TextAST;
    if(!xerattrib && hasVariantAttrs() &&  hasNeedofXerAttrs())
      xerattrib = new XerAttributes;
    if (!jsonattrib && (hasVariantAttrs() || hasEncodeAttr(get_encoding_name(CT_JSON)) || hasNeedofJsonAttrs())) {
      jsonattrib = new JsonAST;
    }
    break;
  case T_CHOICE_A:
    chk_Choice_A();
    // TODO create an empty XerAttrib as above, when ASN.1 gets XER ?
    // The code was originally for TTCN-only encodings.
    break;
  case T_SEQ_A:
  case T_SET_A:
    chk_Se_A();
    // TODO create an empty XerAttrib as above, when ASN.1 gets XER ?
    break;
  case T_SEQOF:
  case T_SETOF:
    chk_SeOf();
    break;
  case T_REFD:
  case T_ADDRESS:
    chk_refd();
    break;
  case T_SELTYPE:
    chk_seltype();
    break;
  case T_REFDSPEC:
  case T_OCFT:
    u.ref.type_refd->chk();
    u.ref.component_internal = u.ref.type_refd->is_component_internal();
    break;
  case T_ARRAY:
    chk_Array();
    break;
  case T_PORT:
    u.port->chk();
    if (w_attrib_path) u.port->chk_attributes(w_attrib_path);
    break;
  case T_SIGNATURE:
    chk_Signature();
    break;
  case T_COMPONENT:
    u.component->chk();
    break;
  case T_FUNCTION:
  case T_ALTSTEP:
  case T_TESTCASE:
    chk_Fat();
    break;
  default:
    FATAL_ERROR("Type::chk()");
  } // switch

  if(w_attrib_path) {
    switch (get_type_refd_last()->typetype) {
    case T_SEQ_T:
    case T_SET_T:
    case T_CHOICE_T:
      // These types may have qualified attributes
      break;
    case T_SEQOF: case T_SETOF:
      break;
    default:
      w_attrib_path->chk_no_qualif();
      break;
    }
  }
  checked = true;
  if(tags) tags->set_plicit(this);

  /*
  Check all non-table subtype constraints. Table/relational constraints
  are ignored here.
  TODO: non-relational table constraints shall not be ignored.
  */
  if (check_subtype) check_subtype_constraints();

  /*
   * Checking the constraints can be done only if the entire type
   * (including the nested typedefs) is checked, because the
   * component relation constraint has to 'look' into other
   * components.
   */
  if (!parent_type) chk_table_constraints();

  if(rawattrib || is_root_basic()){
    chk_raw();
  }
  if(textattrib || is_root_basic()) {
    chk_text();
  }
  
  if (jsonattrib || is_root_basic()) {
    chk_json();
  }
  
  // We need to call chk_xer() always because it is collecting
  // XER attributes from parent types.
  chk_xer();
  
  chk_finished = true;
}

void Type::parse_attributes()
{
  if (raw_parsed) return;

  // The type has no attributes of its own; connect it to the nearest group
  // or the module. This allows global attributes to propagate.
  for (Type *t = this; t && w_attrib_path == 0; t = t->parent_type) {
    switch (t->ownertype) {
    case OT_TYPE_DEF: {
      Ttcn::Def_Type *pwn = static_cast<Ttcn::Def_Type*>(t->owner);
      Ttcn::Group *nearest_group = pwn->get_parent_group();

      w_attrib_path = new WithAttribPath;
      if (nearest_group) { // there is a group
        w_attrib_path->set_parent(nearest_group->get_attrib_path());
      }
      else { // no group, use the module
        Common::Module *mymod = t->my_scope->get_scope_mod();
        // OT_TYPE_DEF is always from a TTCN-3 module
        Ttcn::Module *my_ttcn_module = static_cast<Ttcn::Module *>(mymod);
        w_attrib_path->set_parent(my_ttcn_module->get_attrib_path());
      }
      break; }

    case OT_RECORD_OF:
    case OT_COMP_FIELD:
      continue; // try the enclosing type

    default:
      break;
    }
    break;
  }

  if ((hasVariantAttrs())
      && (enable_text() || enable_raw() || enable_xer())) {
#ifndef NDEBUG
    if (rawAST_debug) {
      const char *fn = get_fullname().c_str();
      fprintf(stderr, "parse_attributes for %s\n", fn);
    }
#endif
    bool new_raw=false;      // a RawAST  object was allocated here
    bool new_text=false;     // a TextAST object was allocated here
    bool new_xer=false;      // a XerAttribute object was allocated here
    bool new_ber=false;      // a BerAST object was allocated here
    bool new_json = false;   // a JsonAST object was allocated here
    bool raw_found=false;    // a raw  attribute was found by the parser
    bool text_found=false;   // a text attribute was found by the parser
    bool xer_found=false;    // a XER  attribute was found by the parser
    bool ber_found=false;    // a BER attribute was found by the parser
    bool json_found = false; // a JSON attribute was found by the parser
    raw_checked=false;
    text_checked=false;
    json_checked = false;
    xer_checked=false;
    bool override_ref=false;
    // Parse RAW attributes
    switch(typetype) {
    case T_REFD: {
      ReferenceChain refch(this, "While checking attributes");
      if(w_attrib_path)
      {
        // Get all the attributes that apply (from outer scopes too).
        // Outer (generic) first, inner (specific) last.
        const vector<SingleWithAttrib> & real_attribs
          = w_attrib_path->get_real_attrib();

        // see if there's an encode with override
        for(size_t i = real_attribs.size(); i > 0 && !override_ref; i--)
        {
          if(real_attribs[i-1]->has_override()
            && real_attribs[i-1]->get_attribKeyword()
              != SingleWithAttrib::AT_ENCODE)
            override_ref = true;
        }
      }
      if(!rawattrib && !override_ref){
        Type *t=get_type_refd_last(&refch);
        typetype_t basic_type=t->typetype;
        t=this; // go back to the beginning
        refch.reset();
        while(!t->rawattrib && t->is_ref()) t=t->get_type_refd(&refch);
        rawattrib=new RawAST(t->rawattrib,basic_type==T_INT);
        if(!t->rawattrib && basic_type==T_REAL) rawattrib->fieldlength=64;
        new_raw=true;
      }
      if(!textattrib && !override_ref){
        Type *t=this;
        refch.reset();
        while(!t->textattrib && t->is_ref()) t=t->get_type_refd(&refch);
        textattrib=new TextAST(t->textattrib);
        new_text=true;
      }
      if (!jsonattrib && !override_ref){
        Type *t = this;
        refch.reset();
        while (!t->jsonattrib && t->is_ref()) {
          t = t->get_type_refd(&refch);
        }
        jsonattrib = new JsonAST(t->jsonattrib);
        new_json = true;
      }
    }
    // no break
    case T_BOOL:
    case T_INT:
    case T_REAL:
    case T_BSTR:
    case T_HSTR:
    case T_OSTR:
    case T_CSTR: // TTCN-3 charstring
    case T_ENUM_T:
      /* The list of types supporting RAW encoding is described in the
       * API guide, section 3.3; this batch of labels plus the next three
       * accurately match the list.
       *
       * For TEXT, it's section 3.4; the list does not include real and
       * bin/hex/octet strings
       */
    case T_USTR: // TTCN-3 universal charstring, this is an addition for XER attributes
    case T_VERDICT: // TTCN-3 verdict, for XER

      if(rawattrib==NULL){
        rawattrib= new RawAST(typetype==T_INT);
        if(typetype==T_REAL) rawattrib->fieldlength=64;
        new_raw=true;
      }
      if(textattrib==NULL){textattrib= new TextAST; new_text=true;}
      if (xerattrib==NULL) {
        xerattrib = new XerAttributes; new_xer = true;
      }
      if (berattrib==NULL) {
        berattrib = new BerAST;
        new_ber = true;
      }
      
      if (NULL == jsonattrib) {
        jsonattrib = new JsonAST;
        new_json = true;
      }

      if(w_attrib_path)
      {
        vector<SingleWithAttrib> const &real_attribs
          = w_attrib_path->get_real_attrib();
        // These are attributes without qualifiers.

        size_t nof_elements = real_attribs.size();
        for(size_t i = 0; i < nof_elements; i++)
        {
          SingleWithAttrib *temp_single = real_attribs[i];
          if (temp_single->get_attribKeyword() ==
            SingleWithAttrib::AT_VARIANT) { // only "variant" is parsed
            parse_rawAST(rawattrib, textattrib, xerattrib, berattrib, jsonattrib,
              temp_single->get_attribSpec(), get_length_multiplier(),
              my_scope->get_scope_mod(), 
              raw_found, text_found, xer_found, ber_found, json_found);
//            textattrib->print_TextAST();
//            rawattrib->print_RawAST();
//            xerattrib->print(get_fullname().c_str());
          } // if AT_VARIANT
        } // next i
      }

      if(!raw_found && new_raw){ delete rawattrib; rawattrib=NULL;}
      if(!text_found && new_text){ delete textattrib; textattrib=NULL;}
      if(!xer_found && new_xer){ delete xerattrib; xerattrib = NULL; }
      if(!ber_found && new_ber){ delete berattrib; berattrib = NULL; }
      if (!json_found && new_json) {
        delete jsonattrib;
        jsonattrib = NULL;
      }
      break;
    case T_SEQOF:
    case T_SETOF:
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_ANYTYPE:
    case T_ARRAY:
      if(rawattrib==NULL) {rawattrib= new RawAST; new_raw=true;}
      if(textattrib==NULL){textattrib= new TextAST; new_text=true;}
      if(xerattrib==NULL) {xerattrib = new XerAttributes; new_xer = true;}
      if(berattrib==NULL) {berattrib = new BerAST; new_ber = true;}
      if (NULL == jsonattrib) {
        jsonattrib = new JsonAST;
        new_json = true;
      }

      if(w_attrib_path)
      {
        vector<SingleWithAttrib> const &real_attribs
          = w_attrib_path->get_real_attrib();

        //calculate the type's attributes (the qualifierless ones)
        size_t nof_elements = real_attribs.size();
        for(size_t i = 0; i < nof_elements; i++)
        {
          SingleWithAttrib *temp_single = real_attribs[i];
          if(temp_single->get_attribKeyword() == SingleWithAttrib::AT_VARIANT
            && (!temp_single->get_attribQualifiers()
              || temp_single->get_attribQualifiers()->get_nof_qualifiers()
                  == 0)){
            // raw/text/xer/ber/json attributes for the whole record/seq.
            // (own or inherited)
            parse_rawAST(rawattrib, textattrib, xerattrib, berattrib, jsonattrib,
              temp_single->get_attribSpec(), get_length_multiplier(),
              my_scope->get_scope_mod(),
              raw_found, text_found, xer_found, ber_found, json_found);
          }
        }
        //calculate the components attributes
        MultiWithAttrib* self_attribs = w_attrib_path->get_with_attr();
        if(self_attribs)
        {
          MultiWithAttrib* new_self_attribs = new MultiWithAttrib;
          SingleWithAttrib* swa = 0;

          // copy all the "encode" attributes
          for(size_t i = 0; i < self_attribs->get_nof_elements(); i++)
          {
            if(self_attribs->get_element(i)->get_attribKeyword()
              == SingleWithAttrib::AT_ENCODE)
            {
              // Copy the attribute without qualifiers
              const SingleWithAttrib* swaref = self_attribs->get_element(i);
              swa = new SingleWithAttrib(swaref->get_attribKeyword(),
                swaref->has_override(), 0, swaref->get_attribSpec().clone());
              new_self_attribs->add_element(swa);
            }
          }

          if(new_self_attribs->get_nof_elements() > 0)
          { // One or more "encode"s were copied; create a context for them.
            // This is a member because is has to be owned by this Type
            // (the components only refer to it).
            encode_attrib_path = new WithAttribPath;
            encode_attrib_path->set_with_attr(new_self_attribs);
            encode_attrib_path->set_parent(w_attrib_path->get_parent());
          }
          else delete new_self_attribs;

          // This should be bool, but gcc 4.1.2-sol8 generates incorrect code
          // with -O2 :(
          const int se_of = (typetype == T_SEQOF || typetype == T_SETOF ||
            typetype == T_ARRAY);
          const size_t nof_comps = se_of ? 1 : get_nof_comps();

          // Distribute the attributes with qualifiers to the components.
          // If the type is a sequence-of or set-of, we pretend it to have
          // one component with the name "_0" (a valid TTCN-3 identifier
          // can't begin with an underscore). compiler.y has created
          // the appropriate identifier in the qualifier for a "[-]".
          for(size_t i = 0; i < nof_comps; i++)
          {
            const Identifier& comp_id =
              se_of ? underscore_zero : get_comp_id_byIndex(i);
            MultiWithAttrib* component_attribs = new MultiWithAttrib;

            for(size_t j = 0; j < self_attribs->get_nof_elements(); j++)
            {
              const SingleWithAttrib *temp_single =
                self_attribs->get_element(j);
              Qualifiers* temp_qualifiers =
                temp_single->get_attribQualifiers();
              if( !temp_qualifiers
                || temp_qualifiers->get_nof_qualifiers() == 0) continue;

              Qualifiers* calculated_qualifiers = new Qualifiers;
              bool qualifier_added = false;
              for(size_t k=0; k < temp_qualifiers->get_nof_qualifiers(); )
              {
                const Qualifier* temp_qualifier =
                  temp_qualifiers->get_qualifier(k);
                if(temp_qualifier->get_nof_identifiers() > 0
                  && (*temp_qualifier->get_identifier(0) == comp_id))
                {
                  // Found a qualifier whose first identifier matches
                  //  the component name. Remove the qualifier from the
                  //  enclosing type, chop off its head,
                  //  and add it to the component's qualifiers.
                  calculated_qualifiers->add_qualifier(
                    temp_qualifier->get_qualifier_without_first_id());
                  temp_qualifiers->delete_qualifier(k);
                  qualifier_added = true;
                }
                else k++;
              } // next qualifier

              if(qualifier_added)
              {
                // A copy of temp_single, with new qualifiers
                SingleWithAttrib* temp_single2
                = new SingleWithAttrib(temp_single->get_attribKeyword(),
                  temp_single->has_override(),
                  calculated_qualifiers,
                  temp_single->get_attribSpec().clone());
                temp_single2->set_location(*temp_single);
                component_attribs->add_element(temp_single2);
              }
              else delete calculated_qualifiers;
            } // next attrib

            if (component_attribs->get_nof_elements() > 0) {
              Type* component_type = se_of ?
                get_ofType() : get_comp_byIndex(i)->get_type();

              if(encode_attrib_path)
                // The record's "encode" attributes (only) apply to the fields.
                // Interpose them in the path of the field.
                component_type->set_parent_path(encode_attrib_path);
              else
                component_type->set_parent_path(w_attrib_path->get_parent());

              component_type->set_with_attr(component_attribs);
            }
            else delete component_attribs;
          } // next component index

          // Any remaining attributes with qualifiers are erroneous
          for(size_t i = 0; i < self_attribs->get_nof_elements();)
          {
            Qualifiers *temp_qualifiers = self_attribs->get_element(i)
                ->get_attribQualifiers();
            if(temp_qualifiers && temp_qualifiers->get_nof_qualifiers() != 0)
            {
              size_t nof_qualifiers = temp_qualifiers->get_nof_qualifiers();
              for(size_t j = 0; j < nof_qualifiers; j++)
              {
                const Qualifier *temp_qualifier =
                  temp_qualifiers->get_qualifier(j);
                const Identifier& tmp_id = *temp_qualifier->get_identifier(0);
                // Special case when trying to reference the inner type
                // of a record-of when it wasn't a record-of.
                if (tmp_id == underscore_zero) temp_qualifier->error(
                  "Invalid field qualifier [-]");
                else temp_qualifier->error("Invalid field qualifier %s",
                  tmp_id.get_dispname().c_str());
              }
              self_attribs->delete_element(i);
            }else{
              i++;
            }
          } // next i
        } // end if(self_attribs)
      } // end if(w_attrib_path)
      if (!raw_found && new_raw)  { delete rawattrib;  rawattrib = NULL; }
      if (!text_found && new_text){ delete textattrib; textattrib= NULL; }
      if (!xer_found && new_xer)  { delete xerattrib;  xerattrib = NULL; }
      if (!ber_found && new_ber)  { delete berattrib;  berattrib = NULL; }
      if (!json_found && new_json) {
        delete jsonattrib;
        jsonattrib = NULL;
      }
      break;
    default:
      // nothing to do, ASN1 types or types without defined raw attribute
      break;
    } // switch
    if (rawattrib && !enable_raw()) { delete rawattrib;  rawattrib = NULL;}
    if (textattrib&& !enable_text()){ delete textattrib; textattrib= NULL;}
    if (xerattrib && !enable_xer()) { delete xerattrib;  xerattrib = NULL;}
    if (berattrib && !enable_ber()) { delete berattrib;  berattrib = NULL;}
    if (NULL != jsonattrib && !enable_json()) {
      delete jsonattrib;
      jsonattrib = NULL;
    }
  } // endif( hasVariantAttrs && enable_{raw,text,xer} )

  raw_parsed = true;
}

// Implements "NAME AS ..." transformations.
void change_name(string &name, XerAttributes::NameChange change) {
  switch (change.kw_) {
  case NamespaceSpecification::NO_MANGLING:
    break; // cool, nothing to do!

  case NamespaceSpecification::UPPERCASED:
    // Walking backwards calls size() only once. Loop var must be signed.
    for (int i = name.size()-1; i >= 0; --i) {
      name[i] = (char)toupper(name[i]);
    }
    break;

  case NamespaceSpecification::LOWERCASED:
    for (int i = name.size()-1; i >= 0; --i) {
      name[i] = (char)tolower(name[i]);
    }
    break;

  case NamespaceSpecification::CAPITALIZED:
    name[0] = (char)toupper(name[0]);
    break;

  case NamespaceSpecification::UNCAPITALIZED:
    name[0] = (char)tolower(name[0]);
    break;

  default: // explicitly specified name
    name = change.nn_;
    break;
  } // switch for NAME
}

void Type::chk_xer_any_attributes()
{
  Type * const last = get_type_refd_last();
  switch (last->typetype == T_SEQOF ?
    last->u.seof.ofType->get_type_refd_last()->typetype : 0) {
  case T_UTF8STRING: // SEQUENCE OF UTF8String, ASN.1
  case T_USTR:       // record of universal charstring
    break;
    // fall through
  default:
    error("ANY-ATTRIBUTES can only be applied to record of string");
    break;
  } // switch

  switch (parent_type != NULL ? parent_type->typetype : 0) {
  case T_SEQ_A: case T_SET_A:
  case T_SEQ_T: case T_SET_T:
    for (size_t x = 0; x < parent_type->get_nof_comps(); ++x) {
      CompField * cf = parent_type->get_comp_byIndex(x);
      if (cf->get_type() != this) continue;
      if (cf->has_default()) {
        error("The field with ANY-ATTRIBUTES cannot have DEFAULT");
      }
    }
    break;
  default:
    error("ANY-ATTRIBUTES can only be applied to a member of "
      "SEQUENCE, SET, record or set");
    break;
  }

  if (xerattrib->untagged_
    || (parent_type && parent_type->xerattrib && parent_type->xerattrib->untagged_)) {
    error("Neither the type with ANY-ATTRIBUTES, nor its enclosing type "
      "may be marked UNTAGGED");
  }
}

void Type::chk_xer_any_element()
{
  Type *const last = get_type_refd_last();
  switch (last->typetype) {
  case T_UTF8STRING: // UTF8String, ASN.1
  case T_USTR:       // universal charstring
    break; // acceptable
  case T_SEQOF: {
    /* A special case for TTCN-3 where applying ANY-ELEMENT to the record-of
     * has the same effect as applying it to the string member.
     * This should no longer be necessary now that we can refer
     * to the embedded type of a record-of with [-], but it has to stay
     * unless the standard deprecates it. */
    Type *oftype = last->u.seof.ofType;
    if (oftype->xerattrib == 0) oftype->xerattrib = new XerAttributes;
    // Transfer the ANY-ATTRIBUTE from the record-of to its member type
    oftype->xerattrib->anyElement_ = xerattrib->anyElement_;
    xerattrib->anyElement_.nElements_ = 0;
    xerattrib->anyElement_.uris_ = 0;
    // Re-check the member type since we fiddled with it
    const char * type_name = "record of";
    Error_Context cntxt(this, "In embedded type of %s", type_name);
    oftype->xer_checked = false;
    oftype->chk_xer();
    break; }
  default:
    error("ANY-ELEMENT can only be applied to UTF8String "
      "or universal charstring type");
    break;
  } // switch

  if (xerattrib->attribute_ || xerattrib->base64_ || xerattrib->untagged_
    || (xerattrib->defaultForEmpty_ != NULL || xerattrib->defaultForEmptyIsRef_)
    || xerattrib->whitespace_ != XerAttributes::PRESERVE) {
    error("A type with ANY-ELEMENT may not have any of the following encoding instructions: "
      "ATTRIBUTE, BASE64, DEFAULT-FOR-EMPTY, PI-OR-COMMENT, UNTAGGED or WHITESPACE");
  }
}

void Type::chk_xer_attribute()
{
  if (xerattrib->element_) {
    error("ELEMENT and ATTRIBUTE are incompatible");
  }

  switch (parent_type!=NULL ? parent_type->typetype :-0) {
  case 0: // no parent accepted in case a field of this type is declared
  case T_SEQ_A: case T_SEQ_T:
  case T_SET_A: case T_SET_T:
    break; // acceptable
  default:
    error("A type with ATTRIBUTE must be a member of "
      "SEQUENCE, SET, record or set");
    break;
  }

  if (xerattrib->untagged_
    || (parent_type && parent_type->xerattrib && parent_type->xerattrib->untagged_)) {
    error("Neither the type with ATTRIBUTE, nor its enclosing type "
      "may be marked UNTAGGED");
  }

  if (has_ae(xerattrib)) {
    // TODO: || (xerattrib->defaultForEmpty_ && it is an ASN.1 type)
    // DEFAULT-FOR-EMPTY is allowed only for TTCN-3
    error("A type with ATTRIBUTE shall not also have any of the final "
      "encoding instructions ANY-ELEMENT" /*", DEFAULT-FOR-EMPTY"*/ " or PI-OR-COMMENT");
  }
}

/// CompField cache
struct CFCache {
  /// Pointer to the field
  CompField *cf;
  /// The type of the field
  Type *top;
  /// The ultimate type, top->get_type_refd_last()
  Type *last;
  /// The typetype of last, on which we sort. Its name is meant to be
  /// mnemonic (it belongs to \c last, not \c top)
  Type::typetype_t lastt;
};

/// Comparison function for CFCache based on typetype
int tcomp(const void *l, const void *r)
{
  int retval = ((const CFCache*)l)->lastt - ((const CFCache*)r)->lastt;
  return retval;
}

/** Find the original component name if it was changed by TEXT
 *
 * If there is a TEXT whose \a new_text matches \p text, return
 * the corresponding \a target.
 *
 * If there are no TEXT coding instructions, always returns \p text.
 *
 * @param[in,out] text on input, a name possibly modified by any TEXT encoding
 * instruction; on output, the actual component name
 */
void Type::target_of_text(string & text)
{
  for (size_t t = 0; t < xerattrib->num_text_; ++t) {
    NamespaceSpecification& txt = xerattrib->text_[t];

    if ((unsigned long)txt.prefix == 0
      ||(unsigned long)txt.prefix == NamespaceSpecification::ALL) {
      FATAL_ERROR("Type::target_of_text()");
      continue;
    }

    string target(txt.target);

    switch ((unsigned long)txt.new_text) {
    case NamespaceSpecification::CAPITALIZED: // tARGET -> TARGET
      target[0] = (char)toupper(target[0]);
      break;
    case NamespaceSpecification::UNCAPITALIZED:
      target[0] = (char)tolower(target[0]); // TARGET -> tARGET
      break;
    case NamespaceSpecification::UPPERCASED:
      for (int si = target.size() - 1; si >= 0; --si) {
        target[si] = (char)toupper(target[si]);
      }
      break;
    case NamespaceSpecification::LOWERCASED:
      for (int si = target.size() - 1; si >= 0; --si) {
        target[si] = (char)tolower(target[si]);
      }
      break;

    case 0: // "text" not possible
      FATAL_ERROR("Type::target_of_text() Text with no target and DFE");
      break;

    default: // it's a string, "text 'field' as 'string'"
      target = txt.uri;
      break;
    } // switch new_text

    if (target == text) {
      text = txt.target; // we want the value before the change
      break;
    }
  } // next text
}

// The suffix of the name of the variable which contains the D-F-E value.
static const string dfe_suffix("_dfe");

/** Construct a Value to represent defaultForEmpty
 *
 * @param last pointer to a Type which is the end of the reference chain,
 * usually this->get_type_refd_last()
 * @param dfe_str string containing the value from defaultForEmpty
 * @param ref Reference containing the reference in defaultForEmpty
 * @param is_ref_dfe true if reference is used instead of text in defaultForempty
 * @return a newly allocated Common::Value
 */
Value *Type::new_value_for_dfe(Type *last, const char *dfe_str, Common::Reference* ref, bool is_ref_dfe)
{
  string defaultstring;
  if (is_ref_dfe) {
    Value* v = new Value(Value::V_REFD, ref->clone());
    v->set_my_scope(get_my_scope()->get_scope_mod());
    Type * t = v->get_expr_governor_last();
    if (t == NULL) {
      delete v;
      return 0;
    }
    bool same_mod = false;
    if (last->get_my_scope()->get_scope_mod() == t->get_my_scope()->get_scope_mod()) {
      same_mod = true;
    }
    if (!is_compatible_tt_tt(last->typetype, t->typetype, last->is_asn1(), t->is_asn1(), same_mod)) {
      v->get_reference()->error("Incompatible types were given to defaultForEmpty variant: `%s' instead of `%s'.\n",
        v->get_expr_governor_last()->get_typename().c_str(), last->get_typename().c_str());
      delete v;
      return 0;
    }
    if (!v->is_unfoldable()) {
      switch(v->get_value_refd_last()->get_valuetype()) {
        case Value::V_CSTR:
          defaultstring = v->get_val_str().c_str();
          break;
        case Value::V_USTR:
        case Value::V_ISO2022STR:
          defaultstring = v->get_val_ustr().get_stringRepr_for_pattern();
          break;
        case Value::V_INT:
          defaultstring = Int2string(v->get_val_Int()->get_val());
          break;
        case Value::V_REAL:
          defaultstring = Real2string(v->get_val_Real());
          break;
        case Value::V_BOOL:
          if (v->get_val_bool()) {
            defaultstring = "true";
          } else {
            defaultstring = "false";
          }
          break;
        case Value::V_ENUM: {
          Value* v2 = v->get_value_refd_last();
          defaultstring = v2->get_val_id()->get_ttcnname().c_str();
          break; }
        //case Value::V_CHOICE: //In the switch below the choice is handled but
                                //it is not possible to write DFE for unions???
        default:
          break;
      }
      dfe_str = defaultstring.c_str();
    } else if (v->get_reference()->get_refd_assignment()->get_asstype() ==  Assignment::A_MODULEPAR) {
      return v;
    } else {
       v->get_reference()->error("Only strings, constants and module parameters are allowed for a defaultForEmpty value.\n");
       delete v;
       return 0;
    }
    delete v;
  }
  if (!is_ref_dfe) {
    defaultstring = dfe_str;
  }
  switch (last->typetype) {
  case T_CSTR:
  case T_USTR:
  case T_UTF8STRING:
      return new Value(Common::Value::V_CSTR,
        new string(defaultstring));

  case T_INT:
  case T_INT_A:
    return new Value(Common::Value::V_INT,
      new int_val_t(dfe_str, *this));

  case T_REAL: {
    const Real& rval = string2Real(dfe_str, *this);
    return new Value(Value::V_REAL, rval);
  }

  case T_BOOL: {
    if (!strcmp(dfe_str, "true")
      ||!strcmp(dfe_str, "1")) {
      return new Value(Value::V_BOOL, true);
    }
    else if (!strcmp(dfe_str, "false")
      ||     !strcmp(dfe_str, "0")) {
      return new Value(Value::V_BOOL, false);
    }
    else error("Invalid boolean default value");
    break;
  }

  case T_ENUM_A: case T_ENUM_T: {
    // If there is a TEXT, the DFE value corresponds to TextToBeUsed.
    // Fetch the "real" name of the field (Target).

    target_of_text(defaultstring);

    Identifier *val_id = new Identifier(Common::Identifier::ID_TTCN, // FIXME when ASN1 is supported
      defaultstring);

    if (!last->has_ei_withName(*val_id)) {
      error("No enumeration item item '%s'", defaultstring.c_str());
#ifndef NDEBUG
      for (size_t ee=0; ee < last->u.enums.eis->get_nof_eis(); ++ee) {
        note("Maybe %s", last->u.enums.eis->get_ei_byIndex(ee)->get_name().get_name().c_str());
      }
#endif
    }

    return new Value(Common::Value::V_ENUM, val_id);
  }
  case T_CHOICE_A: case T_CHOICE_T: case T_ANYTYPE: {
    // Try to guess which alternative the given DFE text belongs to.
    // Sort the fields based on typetype, so BOOL, INT, REAL, ENUM
    // are tried before the various string types
    // (any string looks 'right' for a string value).
    size_t num_comps = last->get_nof_comps();
    CFCache *sorted = new CFCache[num_comps];
    for (size_t c = 0; c < num_comps; c++) {
      CompField *cf = last->get_comp_byIndex(c);
      Type *cft = cf->get_type();
      Type *cftlast = cft->get_type_refd_last();
      sorted[c].cf = cf;
      sorted[c].top  = cft;
      sorted[c].last = cftlast;
      sorted[c].lastt = cftlast->typetype;
    }
    qsort(sorted, num_comps, sizeof(CFCache), tcomp);

    Value * retval = 0;
    size_t c;
    for (c = 0; c < num_comps && retval == 0; c++) {
      CFCache &current = sorted[c];
      // We can't just call new_value_for_dfe(), because some alternatives
      // would generate errors even if a later type could accept the value.
      switch (current.lastt) {
      case T_BOOL:
        if (!strcmp(dfe_str, "true")
          ||!strcmp(dfe_str, "1")) {
          retval = new Value(Value::V_BOOL, true);
        }
        else if (!strcmp(dfe_str, "false")
          ||     !strcmp(dfe_str, "0")) {
          retval = new Value(Value::V_BOOL, false);
        }
        break;

      case T_INT: case T_INT_A: {
        const char *start = dfe_str, *end;
        while (isspace((const unsigned char)*start)) ++start;
        if (*start == '+') ++start;
        int ndigits = BN_dec2bn(NULL, start); // includes any '-' sign
        end = start + ndigits;
        // Pretend that all trailing whitespace characters were digits
        while (isspace(*end)) ++ndigits, ++end;

        // Check that all the string was used up in the conversion,
        // otherwise "3.1415" and "1e6" would appear as integers.
        if (defaultstring.size() == (size_t)ndigits + (start - dfe_str )) {
          retval = current.top->new_value_for_dfe(current.last, start);
        }
        break; }

      case T_REAL: {
        float f;
        char tail[2];
        int num_converted = sscanf(dfe_str, "%f %1s", &f, tail);
        // If tail was converted (num_converted>1) that's an error
        if (num_converted == 1) { // sscanf was happy
          retval = current.top->new_value_for_dfe(current.last, dfe_str);
        }
        break; }

      case T_ENUM_A: case T_ENUM_T: {
        current.top->target_of_text(defaultstring);
        Identifier alt(Identifier::ID_TTCN, defaultstring);
        if (current.last->has_ei_withName(alt)) {
          retval = current.top->new_value_for_dfe(current.last, dfe_str);
        }
        break; }

      case T_CSTR:
      case T_USTR:
      case T_UTF8STRING: {
        retval = current.top->new_value_for_dfe(current.last, dfe_str);
        break; }

      default:
        break;
      } // switch
    } // next c

    if (retval != 0) {
      c--;
      retval->set_genname(sorted[c].top->genname, dfe_suffix);
      retval->set_my_scope(sorted[c].top->my_scope);
      retval->set_my_governor(sorted[c].top);
      Value *choice_retval = new Value(Value::V_CHOICE,
        new Identifier(sorted[c].cf->get_name()), retval);
      retval = choice_retval;
    }

    delete [] sorted;
    return retval; }

  /* Useless without a properly constructed Value(V_SEQ)
  case T_SEQ_A: case T_SEQ_T: {
    NamedValues *nvs = new NamedValues;
    //NamedValue  *nv  = new NamedValue(new Identifier, new Value);
    nvs->add_nv(nv);
    xerattrib->defaultValue_ = new Value(Value::V_SEQ, nvs);
    break; }
   */

  default: // complain later
    break;
  } // switch
  return 0;
}

void Type::chk_xer_dfe()
{
  Type * const last = get_type_refd_last();

  if (/* TODO xerattrib->attribute_ is error for ASN.1 only */
    xerattrib->untagged_ || has_ae(xerattrib)) {
    error("A type with DEFAULT-FOR-EMPTY shall not have any of the final "
      "encoding instructions ANY-ELEMENT, ATTRIBUTE, UNTAGGED."); // 23.2.8
  }

  if (is_charenc() == Yes) {
    xerattrib->defaultValue_ = new_value_for_dfe(last, xerattrib->defaultForEmpty_, xerattrib->defaultForEmptyRef_, xerattrib->defaultForEmptyIsRef_);
    if (xerattrib->defaultValue_ != 0) {
      xerattrib->defaultValue_->set_genname(this->genname, dfe_suffix);
      xerattrib->defaultValue_->set_my_scope(this->my_scope);
      xerattrib->defaultValue_->set_my_governor(last);
    }
    else {
      error("DEFAULT-FOR-EMPTY not supported for character-encodable type %s",
        last->get_stringRepr().c_str());
    }
  }
  else if (last->typetype == T_SEQ_A || last->typetype == T_SEQ_T) {
    // If DEFAULT-FOR-EMPTY applies to a record (SEQUENCE), then only one
    // component can produce element content, and it should be the last,
    // because all the others should have ATTRIBUTE or ANY-ATTRIBUTE,
    // and those are moved to the front. 23.2.2 b)
    // FIXME only b) appears to have this restriction, not c)d)e)
    const size_t num_cf = last->get_nof_comps();
    if (num_cf > 0) {
      CompField *cf = last->get_comp_byIndex(num_cf-1); // last field
      Type *cft = cf->get_type(); // cft: CompField type
      cft = cft->get_type_refd_last();
      //typetype_t cftt = cft->get_typetype();

      xerattrib->defaultValue_ = new_value_for_dfe(cft, xerattrib->defaultForEmpty_, xerattrib->defaultForEmptyRef_, xerattrib->defaultForEmptyIsRef_);
      if (xerattrib->defaultValue_ != 0) {
        xerattrib->defaultValue_->set_genname(last->genname, string("_dfe"));
        xerattrib->defaultValue_->set_my_scope(cft->my_scope);
        xerattrib->defaultValue_->set_my_governor(cft);
      }
      else {
        error("DEFAULT-FOR-EMPTY not supported for fields of type %s",
          cft->get_stringRepr().c_str());
      }
    } // endif comps >0
  } // if SEQ/SET
  else {
    error("DEFAULT-FOR-EMPTY not applicable to type");
  }
}

void Type::chk_xer_embed_values(int num_attributes)
{
  Type * const last = get_type_refd_last();

  enum complaint_type { ALL_GOOD, HAVE_DEFAULT, UNTAGGED_EMBEDVAL,
    NOT_SEQUENCE, EMPTY_SEQUENCE, FIRST_NOT_SEQOF, SEQOF_NOT_STRING,
    SEQOF_BAD_LENGTH, UNTAGGED_OTHER } ;
  complaint_type complaint = ALL_GOOD;
  size_t expected_length = (size_t)-1;
  Type *cf0t = 0; // type of first component
  CompField *cf = 0;
  switch (last->typetype) {
  case T_SEQ_A: case T_SEQ_T: { // 25.2.1, the type must be a sequence
    const size_t num_cf = last->get_nof_comps();
    if (num_cf == 0) {
      complaint = EMPTY_SEQUENCE; // must have a "first component"
      break;
    }
    CompField *cf0 = last->get_comp_byIndex(0);
    cf0t = cf0->get_type()->get_type_refd_last();
    if (cf0->has_default()) {
      complaint = HAVE_DEFAULT;
      break; // 25.2.1 first component cannot have default
    }

    switch (cf0t->get_typetype()) { // check the first component
    case T_SEQOF: {
      Type *cfot = cf0t->get_ofType(); // embedded type
      switch (cfot->get_type_refd_last()->get_typetype()) {
      case T_UTF8STRING:
      case T_USTR: { // hooray, a SEQUENCE OF some string
        if ( (cf0t->xerattrib && cf0t->xerattrib->untagged_)
          || (cfot->xerattrib && cfot->xerattrib->untagged_)) {
          complaint = UNTAGGED_EMBEDVAL; // 25.2.2
          break;
        }

        // Check length restriction on the record of. If there is one,
        // it better be the correct number.
        // FIXME: if there is also a USE-NIL, it cannot have a length restriction; only check at runtime
        const SubType *sub =  cf0t->sub_type;
        const Int len = sub ? sub->get_length_restriction() :-1;
        // get_length_restriction() itself may return -1
        expected_length = num_cf -num_attributes -xerattrib->useOrder_;
        if (len > 0 && (size_t)len != expected_length) {
          // The +1 from 25.2.6 b) is compensated because
          // the EMBED-VALUES member itself is ignored.
          complaint = SEQOF_BAD_LENGTH;
          break;
        }
        break; } //acceptable

      default: // 25.2.1
        complaint = SEQOF_NOT_STRING;
        break;
      } // switch
      break; }

    default: // 25.2.1
      complaint = FIRST_NOT_SEQOF;
      break;
    } // switch(type of first component)

    for (size_t c = 1; c < num_cf; ++c) { // check the other components
      cf = last->get_comp_byIndex(c);
      Type *cft = cf->get_type()->get_type_refd_last();
      if (cft->xerattrib && cft->xerattrib->untagged_ && (cft->is_charenc() == Yes)){
        complaint = UNTAGGED_OTHER;
        break;
      }
    }
    break; } // case T_SEQ*

  default:
    complaint = NOT_SEQUENCE;
    break;
  } // switch typetype

  // TODO 25.2.4, 25.2.5
  if (complaint == ALL_GOOD) embed_values_possible = true;
  else if (xerattrib->embedValues_) {
    switch (complaint) {
    case ALL_GOOD: // Not possible; present in the switch so GCC checks
      // that all enum values are handled (make sure there's no default).
      break;
    case NOT_SEQUENCE:
    case EMPTY_SEQUENCE:
    case FIRST_NOT_SEQOF:
    case SEQOF_NOT_STRING:
    case HAVE_DEFAULT:
      error("A type with EMBED-VALUES must be a sequence type. "
        "The first component of the sequence shall be SEQUENCE OF UTF8String "
        "and shall not be marked DEFAULT");
      break;
    case SEQOF_BAD_LENGTH:
      cf0t->error("Wrong length of SEQUENCE-OF for EMBED-VALUES, should be %lu",
        (unsigned long)expected_length);
      break;
    case UNTAGGED_EMBEDVAL:
      error("Neither the SEQUENCE-OF supporting EMBED-VALUES,"
        "nor its component shall have UNTAGGED."); // 25.2.2
      break;
    case UNTAGGED_OTHER:
      cf->error("There shall be no UNTAGGED on any character-encodable "
        "component of a type with DEFAULT-FOR-EMPTY"); // 25.2.3
      break;
    } // switch(complaint)
  } // if complaint and embedValues
}
/** Wraps a C string but compares by contents, not by pointer */
class stringval {
  const char * str;
public:
  explicit stringval(const char *s = 0) : str(s) {}
  // no destructor
  bool operator<(const stringval& right) const {
    if (       str > (const char*)NamespaceSpecification::ALL
      && right.str > (const char*)NamespaceSpecification::ALL)
    {
      return strcmp(str, right.str) < 0;
    }
    else return str < right.str;
  }
  bool operator==(const stringval& right) const {
    if (       str > (const char*)NamespaceSpecification::ALL
      && right.str > (const char*)NamespaceSpecification::ALL)
    {
      return strcmp(str, right.str) == 0;
    }
    else return str == right.str;
  }
  //bool operator!() const { return str==0; }
  const char *c_str() const { return str; }
};


void Type::chk_xer_text()
{
  if (xerattrib->num_text_ == 0
    ||xerattrib->text_ == 0 ) FATAL_ERROR("Type::chk_xer_text()");
  Type * const last = get_type_refd_last();
  static const stringval empty; // NULL pointer

  // Check the type and quit early if wrong
  switch (last->typetype) {
  case T_BOOL:
    break;
  case T_ENUM_A: // not yet
    error("No XER yet for ASN.1 enumerations");
    return;
  case T_ENUM_T:
    break;
  case T_BSTR_A: // ASN.1 bit string with named bits, not yet
    error("No XER yet for ASN.1 bit strings");
    return;
  case T_INT_A: // ASN.1 integer with named numbers, not yet
    error("No XER yet for ASN.1 named numbers");
    return;
  default:
    error("TEXT not allowed for type %s", get_typename().c_str());
    return;
  }

  // Build a map to eliminate duplicates (new assignment to the same
  // enum item/field overwrites previous text).
  // Keys are the identifiers, values are the texts.
  typedef map<stringval, char> text_map_t;
  text_map_t text_map;

  for (size_t t = 0; t < xerattrib->num_text_; ++t) {
    NamespaceSpecification & txt = xerattrib->text_[t];
    switch ((unsigned long)txt.target) {
    case 0: { // just "TEXT".
      // target=0 and new_text!=0 would have to come from "TEXT AS ..."
      // but that's not allowed by syntax, hence FATAL_ERROR.
      if (txt.new_text) FATAL_ERROR("Type::chk_xer_text");
      if (!text_map.has_key(empty)) {
        text_map.add(empty, txt.new_text);
      }
      if (last->typetype != T_BOOL) {
        error("Lone 'TEXT' only allowed for boolean"); // only in TTCN-3 !
      }
      break; }

    case NamespaceSpecification::ALL: {// TEXT ALL AS ...
      switch (txt.keyword) {
      case NamespaceSpecification::NO_MANGLING:
        // Not possible due to syntax; there is no TTCN source from which
        // the bison parser would create such a NamespaceSpecification.
        FATAL_ERROR("Type::chk_xer_text");
        break;
      case NamespaceSpecification::CAPITALIZED:
      case NamespaceSpecification::UNCAPITALIZED:
      case NamespaceSpecification::LOWERCASED:
      case NamespaceSpecification::UPPERCASED:
        break; // OK

      default: // TEXT ALL AS "some string" is not allowed
        error("text all as 'string' is not allowed");
        continue;
      } // switch(keyword)

      // Expand the "all"
      switch (last->typetype) {
      case T_BOOL:
        text_map.add(stringval(mcopystr("true")), txt.new_text);
        text_map.add(stringval(mcopystr("false")), txt.new_text);
        break;

      //case T_ENUM_A:
      case T_ENUM_T: {
        size_t n_eis = last->u.enums.eis->get_nof_eis();
        for (size_t i = 0; i < n_eis; ++i) {
          EnumItem *ei = last->u.enums.eis->get_ei_byIndex(i);
          const stringval enum_name(mcopystr(ei->get_name().get_ttcnname().c_str()));
          if (text_map.has_key(enum_name)) {
            // Duplicate enum name, flagged elsewhere as error.
            // Don't bother with: text_map[enum_name] = txt.new_text;
            Free(const_cast<char*>(enum_name.c_str()));
          }
          else text_map.add(enum_name, txt.new_text);
        }
        break; }

      default:
        FATAL_ERROR("Type::chk_xer_text");
        break;
      } // switch (typetype)
      break;
    }

    default: {// a string: TEXT 'member' AS ...
      if (txt.keyword == NamespaceSpecification::NO_MANGLING) {
        // Attribute syntax does not allow this combination
        FATAL_ERROR("Type::chk_xer_text");
      }
      // HR39956: empty string is disregarded
      if (txt.keyword>NamespaceSpecification::LOWERCASED && !strcmp(txt.new_text,""))
      {
        txt.new_text = (char*)Realloc(txt.new_text, sizeof(" "));
        strcpy(txt.new_text," ");
      }
      stringval ttarget(txt.target);
      if (text_map.has_key(ttarget)) {
        // Override the earlier TEXT instruction
        free_name_or_kw(text_map[ttarget]);
        text_map[ttarget] = txt.new_text;
        free_name_or_kw(txt.target);
      }
      else text_map.add(ttarget, txt.new_text);
      break; }
    } // switch(target)
  } // next text

  xerattrib->text_ = (NamespaceSpecification*)Realloc(xerattrib->text_,
    text_map.size() * sizeof(NamespaceSpecification));

  // Zero out the newly allocated part
  if (text_map.size() > xerattrib->num_text_) {
    memset(
      xerattrib->text_ + xerattrib->num_text_,
      0,
      (text_map.size() - xerattrib->num_text_) * sizeof(NamespaceSpecification)
    );
  }

  xerattrib->num_text_ = text_map.size(); // accept the new size

  // Another map, to check for duplicate text (decoding would be impossible)
  text_map_t map2;

  // Reconstruct the TEXT structure from the map
  for (size_t t = 0; t < xerattrib->num_text_; ++t) {
    const stringval& k = text_map.get_nth_key(t);
    char * v = text_map.get_nth_elem(t);
    char * newstr = const_cast<char*>(k.c_str());

    xerattrib->text_[t].target   = newstr;
    xerattrib->text_[t].new_text = v;

    stringval txtval(v); // somebody else owns v
    if (map2.has_key(txtval)) {
      switch (xerattrib->text_[t].keyword) {
      case NamespaceSpecification::NO_MANGLING:
        FATAL_ERROR("nope");
        break; // not possible

      case NamespaceSpecification::CAPITALIZED:
      case NamespaceSpecification::UNCAPITALIZED:
      case NamespaceSpecification::LOWERCASED:
      case NamespaceSpecification::UPPERCASED:
        // Duplication may have been caused by expanding TEXT ALL ...
        break; // accept it

      default: // string must be unique
        error("Duplicate text '%s'", v);
        break;
      }
    }
    else map2.add(txtval, newstr);
  }
  text_map.clear();
  map2.clear();
  // // //
  for (size_t t = 0; t < xerattrib->num_text_; ++t) {
    if (xerattrib->useNumber_) {
      error("USE-NUMBER and TEXT are incompatible");
      break;
    }
    NamespaceSpecification & txt = xerattrib->text_[t];

    switch (last->typetype) {
    case T_BOOL:
      /* In ASN.1, Only Booleantype:ALL can have TEXT
       * Currently for TTCN, only the following three case are supported:
       * "text"
       * "text 'true'  as '1'"
       * "text 'false' as '0'"
       * and we convert the last two to the first
       */
      switch ((unsigned long)txt.prefix) {
      case 0: // no Target (ok);
        if (txt.uri != 0) error("Only \"text\" implemented for boolean");
        break;
      case NamespaceSpecification::ALL: // Target = 'all' not allowed for boolean
        error("TEXT all not implemented for boolean");
        break;
      default: // a string, must be "true" or "false"
        if (!strcmp(txt.prefix, "true")) {
          // only "1" is allowed for "true"
          switch ((unsigned long)txt.uri) {
          default: // it's a string
            if (txt.uri[0] == '1' && txt.uri[1] == '\0') {
              // Free the strings to pretend it was a simple "text"
              Free(txt.prefix); txt.prefix = 0;
              Free(txt.uri); txt.uri = 0;
              // These should come in pairs, warn if not
              if (xerattrib->num_text_ == 1) warning("\"text 'false' as '0'\" was implied");
              break;
            }
            // else fall through
          case NamespaceSpecification::CAPITALIZED:
          case NamespaceSpecification::UNCAPITALIZED:
          case NamespaceSpecification::UPPERCASED:
          case NamespaceSpecification::LOWERCASED:
            error("Only '1' is supported for 'true'");
            break;

          case 0: // "text 'true'" is not correct syntax, cannot get here
            FATAL_ERROR("Type::chk_xer_text()");
          } // switch uri
        }
        else if (!strcmp(txt.prefix, "false")) {
          // only "0" is allowed for "false"
          switch ((unsigned long)txt.uri) {
          default: // it's a string
            if (txt.uri[0] == '0' && txt.uri[1] == '\0') {
              // Free the strings to pretend it was a simple "text"
              Free(txt.prefix); txt.prefix = 0;
              Free(txt.uri); txt.uri = 0;
              // These should come in pairs, warn if not
              if (xerattrib->num_text_ == 1) warning("\"text 'true' as '1'\" was implied");
              break;
            }
            // else fall through
          case NamespaceSpecification::CAPITALIZED:
          case NamespaceSpecification::UNCAPITALIZED:
          case NamespaceSpecification::UPPERCASED:
          case NamespaceSpecification::LOWERCASED:
            error("Only '0' is supported for 'false'");
            break;

          case 0: // "text 'false'" is not correct syntax, cannot get here
            FATAL_ERROR("Type::chk_xer_text()");
          } // switch uri
        } // if ("true")
        break;
      } // switch prefix

      break;

    //case T_ENUM_A: // fall through
    case T_ENUM_T: {
      switch ((unsigned long)txt.target) {
      case 0: // "text as ..."
      case NamespaceSpecification::ALL: { // "text all as ..."
        size_t neis = last->u.enums.eis->get_nof_eis();
        for (size_t i = 0; i < neis; ++i) {
          EnumItem *ei = last->u.enums.eis->get_ei_byIndex(i);
          string ei_name(ei->get_name().get_dispname()); // use the element's own name
          XerAttributes::NameChange chg;
          chg.nn_ = txt.uri;
          change_name(ei_name, chg);
          ei->set_text(ei_name);
        }
        break; }

      default: { // target is member name, from "text 'member' as ..."
        // FIXME: ID_TTCN will not be right if we implement XER for ASN.1
        Common::Identifier id(Identifier::ID_TTCN, string(txt.prefix));
        if (last->u.enums.eis->get_nof_eis()==0) FATAL_ERROR("No enum items!");
        if (last->u.enums.eis->has_ei_withName(id)) {
          EnumItem *ei = last->u.enums.eis->get_ei_byName(id);
          string ei_name(id.get_dispname());
          XerAttributes::NameChange chg;
          chg.nn_ = txt.uri;
          change_name(ei_name, chg);
          ei->set_text(ei_name);
        }
        else {
          error("No enumeration item %s", txt.prefix);
        }
        break; }
      } // switch(target)

      break; }

    //case T_INT_A:
      /*
      if (last->u.namednums.nvs->get_nof_nvs()) {
        Common::Identifier id(Identifier::ID_TTCN, txt.uri);
        NamedValue *nv = last->u.namednums.nvs->get_nv_byName(id);
        if (nv != 0) {
          // good
        }
        else {
          error("No component %s in %s", txt.uri, fn);
        }
      }
      else {
        error("TEXT cannot be assigned to an integer without named numbers");
      }
      */
      //break;

    default:
      FATAL_ERROR("Type::chk_xer_text");
      break;
    } // switch
  } // next type for TEXT
}

void Type::chk_xer_untagged()
{
  Type * const last = get_type_refd_last();
  switch (parent_type ? parent_type->typetype :-0) {
  case 0: // "no parent" is acceptable
    // Do not nag ("UNTAGGED encoding attribute is ignored on top-level type");
    // do it in Def_ExtFunction::chk_function_type when the type is actually
    // used as input for an encoding function.
    // fall through
  case T_SEQ_A: case T_SEQ_T:
  case T_SET_A: case T_SET_T:
  case T_ANYTYPE:
  case T_CHOICE_A: case T_CHOICE_T:
  case T_SEQOF: case T_SETOF:
    break; // acceptable
  default:
    error("UNTAGGED can only be applied to a member of sequence, set, "
      "choice, sequence-of, or set-of type"); // X.693amd1, 32.2.1
    break;
  }

  if ( has_aa(xerattrib)
    || has_ae(xerattrib)
    || xerattrib->attribute_   || 0 != xerattrib->defaultForEmpty_
    || xerattrib->defaultForEmptyIsRef_
    || xerattrib->embedValues_ || xerattrib->useNil_
    || (xerattrib->useOrder_ && is_asn1()) || xerattrib->useType_) {
    error("A type with final encoding attribute UNTAGGED shall not have"
      " any of the final encoding instructions ANY-ATTRIBUTES, ANY-ELEMENT,"
      " ATTRIBUTE, DEFAULT-FOR-EMPTY, EMBED-VALUES, PI-OR-COMMENT,"
      " USE-NIL%s or USE-TYPE",
      is_asn1() ? ", USE-ORDER" : ""); // X.693amd1, 32.2.6
  }

  bool can_become_empty= last->has_empty_xml();
  if (can_become_empty) { // checking 32.2.4
    switch (parent_type ? parent_type->typetype :-0) {
    case 0:
      break; // no parent, no problem

    case T_SEQ_A: case T_SEQ_T:
    case T_SET_A: case T_SET_T: {
      // This type can not have OPTIONAL or DEFAULT, 32.2.4 a)
      // No get_comp_byType(); do a linear search.
      size_t num_fields = parent_type->get_nof_comps();
      for (size_t i = 0; i < num_fields; ++i) {
        CompField *cf = parent_type->get_comp_byIndex(i);
        if (cf->get_type() != this) continue;
        // found the component
        if (cf->get_is_optional() || cf->get_defval() != 0) {
          error("Type with final encoding attribute UNTAGGED"
            " shall not have OPTIONAL or DEFAULT");
        }
        break;
      }
      break; }

    case T_SEQOF: case T_SETOF: // X.693amd1, 32.2.4 b)
      error("UNTAGGED type with possibly empty XML value can not be "
        "the member of a sequence-of or set-of"); // X.693amd1, 32.2.4 b)
      break;

    case T_CHOICE_T:
    case T_ANYTYPE: {
      size_t num_fields = parent_type->get_nof_comps();
      size_t num_empty = 0;
      for (size_t i = 0; i < num_fields; ++i) {
        CompField *cf = parent_type->get_comp_byIndex(i);
        Type *cft = cf->get_type();
        if (cft->has_empty_xml()) ++num_empty;
      }
      if (num_empty > 1) { // X.693amd1, 32.2.4 c)
        /* FIXME: this should be error */
        warning("More than one alternative can be empty and has UNTAGGED");
      }
      break; }

    default: // do nothing
      break;
    }
  } // end if(can_become_empty)
}

void Type::chk_xer_use_nil()
{
  Type * const last = get_type_refd_last();

  enum complaint_type { ALL_GOOD, NO_CONTROLNS, NOT_SEQUENCE, EMPTY_SEQUENCE,
    UNTAGGED_USENIL, COMPONENT_NOT_ATTRIBUTE, LAST_IS_ATTRIBUTE,
    LAST_NOT_OPTIONAL, INCOMPATIBLE, WRONG_OPTIONAL_TYPE, EMBED_CHARENC,
    NOT_COMPATIBLE_WITH_USEORDER, BAD_ENUM, FIRST_OPTIONAL, NOTHING_TO_ORDER,
    FIRST_NOT_RECORD_OF_ENUM, ENUM_GAP };
  complaint_type complaint = ALL_GOOD;
  CompField *cf = 0;
  CompField *cf_last = 0;
  const char *ns, *prefix;
  Type *the_enum = 0;
  my_scope->get_scope_mod()->get_controlns(ns, prefix);

  if (!prefix) complaint = NO_CONTROLNS; // don't bother checking further
  else switch (last->typetype) {
  default:
    complaint = NOT_SEQUENCE;
    break;

  case T_SEQ_A:
    error("No XER yet for ASN.1 sequences");
    // no break
  case T_SEQ_T: {
    const size_t num_cf = last->get_nof_comps();
    if (num_cf == 0) { // 33.2.1 ...must have a component...
      complaint = EMPTY_SEQUENCE;
      break; // stop checking to prevent accessing non-existing components
    }
    if (xerattrib->untagged_) { // 33.2.2
      complaint = UNTAGGED_USENIL;
      break;
    }

    // Skip components supporting USE-ORDER or EMBED-VALUES
    size_t i = (xerattrib->useOrder_) + (xerattrib->embedValues_);
    // 33.2.1 All the others except the last must be (any)attributes
    for (; i < num_cf-1; ++i) {
      cf = last->get_comp_byIndex(i);
      Type *cft = cf->get_type();
      if (! (cft->xerattrib
        && ( cft->xerattrib->attribute_ || has_aa(cft->xerattrib))))
      {
        complaint = COMPONENT_NOT_ATTRIBUTE;
        break;
      }
    }
    // 33.2.1 The last component must be an OPTIONAL non-attribute
    cf_last = last->get_comp_byIndex(num_cf-1);
    Type *cft = cf_last->get_type();
    if (!cf_last->get_is_optional()) {
      complaint = LAST_NOT_OPTIONAL;
    }
    
    if(xerattrib->useOrder_ && cft->get_type_refd_last()->get_typetype() != T_SEQ_A
       && cft->get_type_refd_last()->get_typetype() != T_SEQ_T){
      complaint = NOT_COMPATIBLE_WITH_USEORDER;
    }else if(xerattrib->useOrder_) {
      //This check needed, because if the record that has useOrder only
      //has one field that is a sequence type, then the useNilPossible
      //would be always true, that would lead to incorrect code generation.
      Type * inner = cft->get_type_refd_last();
      size_t useorder_index = xerattrib->embedValues_;
      CompField *uo_field = last->get_comp_byIndex(useorder_index);
      Type *uot = uo_field->get_type();
      if (uot->get_type_refd_last()->typetype == T_SEQOF) {
        the_enum = uot->get_ofType()->get_type_refd_last();
        if(the_enum->typetype != T_ENUM_A && the_enum->typetype != T_ENUM_T){
          complaint = FIRST_NOT_RECORD_OF_ENUM;
          break;
        }else if (uo_field->get_is_optional() || uo_field->get_defval() != 0) {
          complaint = FIRST_OPTIONAL;
          break;
        }

        size_t expected_enum_items = inner->get_nof_comps();
        size_t enum_index = 0;
        if (expected_enum_items == 0)
          complaint = NOTHING_TO_ORDER;
        else if (the_enum->u.enums.eis->get_nof_eis() != expected_enum_items)
          complaint = BAD_ENUM;
        else for (size_t index = 0; index < expected_enum_items; ++index) {
          CompField *inner_cf = inner->get_comp_byIndex(index);
          Type *inner_cft = inner_cf->get_type();
          if (inner_cft->xerattrib && inner_cft->xerattrib->attribute_) continue;
          // Found a non-attribute component. Its name must match an enumval
          const Identifier& field_name = inner_cf->get_name();
          const EnumItem *ei = the_enum->get_ei_byIndex(enum_index);
          const Identifier& enum_name  = ei->get_name();
          if (field_name != enum_name) {// X.693amd1 35.2.2.1 and 35.2.2.2
            complaint = BAD_ENUM;
            break;
          }
          const int_val_t *ival = ei->get_int_val();
          const Int enumval = ival->get_val();
          if ((size_t)enumval != enum_index) {
            complaint = ENUM_GAP; // 35.2.2.3
            break;
          }
          ++enum_index;
        }
      }
    }

    if (cft->xerattrib) {
      if ( cft->xerattrib->attribute_
        || has_aa(cft->xerattrib)) {
        complaint = LAST_IS_ATTRIBUTE;
        break;
      }

      if (has_ae(cft->xerattrib)
        ||has_aa(cft->xerattrib)
        ||cft->xerattrib->defaultForEmpty_ != 0
        ||cft->xerattrib->defaultForEmptyIsRef_
        ||cft->xerattrib->untagged_
        ||cft->xerattrib->useNil_
        ||cft->xerattrib->useOrder_
        ||cft->xerattrib->useType_) { // or PI-OR-COMMENT
        complaint = INCOMPATIBLE; // 33.2.3
      }
    }

    if (cft->is_charenc() == Yes) {
      // In a sequence EMBED-VALUES and USE-NIL, the optional component
      // supporting USE-NIL shall not be a character-encodable type.
      // So says OSS, and rightly so (there's no way to separate
      // the last field from the surrounding "embed" strings).
      if (xerattrib->embedValues_) complaint = EMBED_CHARENC;
    }
    else switch (cft->get_type_refd_last()->typetype) {
    case T_SEQ_T:
      // more checking
    case T_SEQ_A:
    case T_SET_T:
    case T_SET_A:
    case T_ANYTYPE:
    case T_CHOICE_T:
    case T_CHOICE_A:
    case T_SEQOF:
    case T_SETOF:
    case T_OPENTYPE:
      // or an octetstring or bitstring with a contained "Type" and without ENCODED BY
      break; // acceptable

    default:
      complaint = WRONG_OPTIONAL_TYPE;
      break;
    }

    break;}
  } // else switch(last->typetype)

  if (complaint == ALL_GOOD) use_nil_possible = true;
  else if (xerattrib->useNil_) {
    switch (complaint) {
    case ALL_GOOD: // Not possible because of the if.
      // Present so GCC checks that all enum values are handled (no default!)
      break;
    case NO_CONTROLNS:
      error("Type has USE-NIL, but the module has no control namespace set");
      break;
    case NOT_SEQUENCE:
      error("The target of an USE-NIL encoding instruction must be a record (SEQUENCE) or set type");
      break;
    case EMPTY_SEQUENCE:
      error("The target of an USE-NIL must have at least one component");
      break;
    case UNTAGGED_USENIL:
      error("The target of an USE-NIL encoding instruction shall not have"
        " a final UNTAGGED encoding instruction");
      break;
    case COMPONENT_NOT_ATTRIBUTE:
      cf->error("Component '%s' of USE-NIL not ATTRIBUTE", cf->get_name().get_name().c_str());
      break;
    case LAST_IS_ATTRIBUTE:
      cf_last->error("Last component of USE-NIL must not have ATTRIBUTE");
      break;
    case LAST_NOT_OPTIONAL:
      cf_last->error("Last component of USE-NIL must be OPTIONAL");
      break;
    case INCOMPATIBLE:
      cf_last->error("The OPTIONAL component of USE-NIL cannot have any of the "
        "following encoding instructions: ANY-ATTRIBUTES, ANY-ELEMENT, "
        "DEFAULT-FOR-EMPTY, PI-OR-COMMENT, UNTAGGED, "
        "USE-NIL, USE-ORDER, USE-TYPE.");
      break;
    case WRONG_OPTIONAL_TYPE:
      cf_last->error("The OPTIONAL component of USE-NIL must be "
        "a character-encodable type, or a sequence, set, choice, "
        "sequence-of, set-of or open type.");
      break;
    case EMBED_CHARENC:
      cf_last->error("In a sequence type with EMBED-VALUES and USE-NIL, "
        "the optional component supporting USE-NIL shall not be "
        "a character-encodable type.");
      break;
    case NOT_COMPATIBLE_WITH_USEORDER:
      cf_last->error("The OTIONAL component of USE-NIL must be "
        "a SEQUENCE/record when USE-ORDER is set for the parent type.");
      break;
    case BAD_ENUM:
      if (!the_enum) FATAL_ERROR("Type::chk_xer_use_order()");
      the_enum->error("Enumeration items should match the"
        " non-attribute components of the field %s",
        cf_last->get_name().get_dispname().c_str());
      break;
    case FIRST_OPTIONAL:
      error("The record-of for USE-ORDER shall not be marked"
        " OPTIONAL or DEFAULT"); // X.693amd1 35.2.3
      break;
    case NOTHING_TO_ORDER:
      error("The component (%s) should have at least one non-attribute"
        " component if USE-ORDER is present",
        cf_last->get_name().get_dispname().c_str());
      break;
    case FIRST_NOT_RECORD_OF_ENUM:
      error("The type with USE-ORDER should have a component "
        "which is a record-of enumerated");
      break;
    case ENUM_GAP:
      if (!the_enum) FATAL_ERROR("Type::chk_xer_use_order()");
      the_enum->error("Enumeration values must start at 0 and have no gaps");
      break;
    } // switch
  } // if USE-NIL
}

void Type::chk_xer_use_order(int num_attributes)
{
  Type * const last = get_type_refd_last();

  enum complaint_type { ALL_GOOD, NOT_SEQUENCE, NOT_ENOUGH_MEMBERS,
    FIRST_NOT_RECORD_OF, FIRST_NOT_RECORD_OF_ENUM, FIRST_OPTIONAL,
    LAST_NOT_RECORD, BAD_ENUM, ENUM_GAP, NOTHING_TO_ORDER };
  complaint_type complaint = ALL_GOOD;
  Type *the_enum = 0; // Well, it's supposed to be an enum.
  switch (last->typetype) {
  case T_SEQ_A: case T_SEQ_T: { // record/SEQUENCE acceptable
    size_t useorder_index = xerattrib->embedValues_;
    // The first (or second, if the first is taken by EMBED_VALUES)
    // member must be a record of enumerated
    if (useorder_index >= last->get_nof_comps()) {
      complaint = NOT_ENOUGH_MEMBERS;
      break;
    }
    CompField *uo_field = last->get_comp_byIndex(useorder_index);
    Type *uot = uo_field->get_type();
    if (uot->get_type_refd_last()->typetype == T_SEQOF) {
      the_enum = uot->get_ofType()->get_type_refd_last();
      switch (the_enum->typetype) {
      case T_ENUM_A: case T_ENUM_T: // acceptable
        if (uo_field->get_is_optional() || uo_field->get_defval() != 0) {
          complaint = FIRST_OPTIONAL;
        }
        break;
      default:
        complaint = FIRST_NOT_RECORD_OF_ENUM;
        goto complain2;
      } // switch enum type
      size_t ncomps = last->get_nof_comps();
      // the components of this sequence will have to match the enum
      Type *sequence_type = 0;
      size_t expected_enum_items, first_non_attr;

      if (xerattrib->useNil_) { // useNil in addition to useOrder
        // This is an additional complication because USE-ORDER
        // will affect the optional component, rather than the type itself
        CompField *cf = last->get_comp_byIndex(ncomps-1);
        sequence_type = cf->get_type()->get_type_refd_last();
        if (sequence_type->typetype == T_SEQ_T
          ||sequence_type->typetype == T_SEQ_A) {

          // No need to check that it has at least one component (35.2.1)
          // it can't match up with the enum which always has one
          ncomps = sequence_type->get_nof_comps();
          expected_enum_items = ncomps;
          first_non_attr = 0;
        }
        else { // Whoops, not a sequence type!
          complaint = LAST_NOT_RECORD;
          break; // the switch(typetype)
        }
      }
      else {
        sequence_type = last;
        first_non_attr = useorder_index+1+num_attributes;
        expected_enum_items = ncomps - first_non_attr;
      } // if (use-nil)

      size_t enum_index = 0;
      if (expected_enum_items == 0)
        complaint = NOTHING_TO_ORDER;
      else if (the_enum->u.enums.eis->get_nof_eis() != expected_enum_items)
        complaint = BAD_ENUM;
      else for (size_t i = first_non_attr; i < ncomps; ++i) {
        CompField *cf = sequence_type->get_comp_byIndex(i);
        Type *cft = cf->get_type();
        if (cft->xerattrib && cft->xerattrib->attribute_) continue;
        // Found a non-attribute component. Its name must match an enumval
        const Identifier& field_name = cf->get_name();
        // don't use get_eis_index_byName(); fatal error if not found :(
        const EnumItem *ei = the_enum->get_ei_byIndex(enum_index);
        const Identifier& enum_name  = ei->get_name();
        if (field_name != enum_name) {// X.693amd1 35.2.2.1 and 35.2.2.2
          complaint = BAD_ENUM;
          break;
        }
        const int_val_t *ival = ei->get_int_val();
        const Int enumval = ival->get_val();
        if ((size_t)enumval != enum_index) {
          complaint = ENUM_GAP; // 35.2.2.3
          break;
        }
        ++enum_index;
      } // next enum component
    }
    else {
      complaint = FIRST_NOT_RECORD_OF;
      complain2:;
    }
    break; }
  default:
    complaint = NOT_SEQUENCE;
    break;
  } // switch typetype

  if (complaint == ALL_GOOD) use_order_possible = true;
  else if (xerattrib->useOrder_) {
    switch (complaint) {
    case ALL_GOOD: // Not possible; present in the switch so GCC checks
      // that all enum values are handled (make sure there's no default).
      break;
    case NOT_SEQUENCE:
      error("USE-ORDER can only be assigned to a SEQUENCE/record type.");
      break;
    case NOT_ENOUGH_MEMBERS:
    case FIRST_NOT_RECORD_OF:
    case FIRST_NOT_RECORD_OF_ENUM:
      error("The type with USE-ORDER should have a component "
        "which is a record-of enumerated");
      break;
    case FIRST_OPTIONAL:
      error("The record-of for USE-ORDER shall not be marked"
        " OPTIONAL or DEFAULT"); // X.693amd1 35.2.3
      break;
    case NOTHING_TO_ORDER:
      error("The type with USE-ORDER should have at least one "
        "non-attribute component");
      break;
    case LAST_NOT_RECORD:
      error("The OPTIONAL component supporting the USE-NIL "
        "encoding instruction should be a SEQUENCE/record");
      break;
    case BAD_ENUM:
      if (!the_enum) FATAL_ERROR("Type::chk_xer_use_order()");
      the_enum->error("Enumeration items should match the"
        " non-attribute components of the sequence");
      break;
    case ENUM_GAP:
      if (!the_enum) FATAL_ERROR("Type::chk_xer_use_order()");
      the_enum->error("Enumeration values must start at 0 and have no gaps");
      break;
    } //switch
  } // if USE-ORDER
}

void Type::chk_xer_use_type()
{
  Type * const last = get_type_refd_last();

  const char *ns, *prefix;
  my_scope->get_scope_mod()->get_controlns(ns, prefix);
  if (!prefix) error("Type has USE-TYPE, but the module has no control namespace set");

  switch (last->typetype) {
  case T_CHOICE_A: case T_CHOICE_T: { // must be CHOICE; 37.2.1
    if (xerattrib->untagged_ || xerattrib->useUnion_) { // 37.2.5
      error("A type with USE-TYPE encoding instruction shall not also have"
        " any of the final encoding instructions UNTAGGED or USE-UNION");
    }
    // Now check the alternatives.
    // iterating backwards calls get_nof_comps only once
    for (int i = last->get_nof_comps() - 1; i >= 0; --i) {
      CompField *cf = last->get_comp_byIndex(i);
      Type *cft = cf->get_type();
      if (cft->xerattrib && cft->xerattrib->untagged_) {
        cf->error("Alternative of a union with USE-TYPE should not have UNTAGGED"); // 37.2.2
        break;
      }
      switch (cft->typetype) {
      case T_CHOICE_A: case T_CHOICE_T:
        if (cft->xerattrib && cft->xerattrib->useType_) {
          cf->error("Alternative of a CHOICE type with USE-TYPE shall not be"
            " a CHOICE type with a USE-TYPE encoding instruction"); // 37.2.3
        }
        break;
      default:
        break;
      }
    }
    break; }
  case T_ANYTYPE:
    error("USE-TYPE cannot be applied to anytype");
    break;
  default:
    error("USE-TYPE can only applied to a CHOICE/union type");
    break;
  } // switch
}

void Type::chk_xer_use_union()
{
  Type * const last = get_type_refd_last();
  switch (last->typetype) {
  case T_CHOICE_A: case T_CHOICE_T: {
    // Now check the alternatives.
    // iterating backwards calls get_nof_comps only once
    for (int i = last->get_nof_comps() - 1; i >= 0; --i) {
      CompField *cf = last->get_comp_byIndex(i);
      Type *cft = cf->get_type();
      if (cft->is_charenc() && !(cft->xerattrib && cft->xerattrib->useType_) &&
          !(cft->xerattrib && cft->xerattrib->useQName_)) { // currently not supported
        if (cft->xerattrib && cft->xerattrib->useUnion_) // it must be a union
          cf->error("Alternative of a CHOICE/union with USE-UNION"
            " can not itself be a CHOICE/union with USE-UNION");
      }
      else cf->error("Alternative of a CHOICE/union with USE-UNION must be character-encodable");
    }
    break; }
  case T_ANYTYPE:
    error("USE-UNION cannot be applied to anytype");
    break;
  default:
    error("USE-UNION can only applied to a CHOICE/union type"); // 38.2.1
    break;
  }

}

static const char *xml98 = "http://www.w3.org/XML/1998/namespace";

void Type::chk_xer() { // XERSTUFF semantic check
  // the type (and everything it contains) is fully checked now

  if (xer_checked) return;
  xer_checked = true;

  Type *last = get_type_refd_last();

  // Check XER attributes if the type belongs to a type definition,
  // a field of a record/set/union, or is the embedded type of a record-of.
  if (ownertype==OT_TYPE_DEF
    ||ownertype==OT_COMP_FIELD
    ||ownertype==OT_RECORD_OF) {
    if (is_ref()) {
      // Merge XER attributes from the referenced type.
      // This implements X.693amd1 clause 15.1.2
      XerAttributes * newx = new XerAttributes;
      Type *t1 = get_type_refd();
      // chk_refd() (called by chk() for T_REFD) does not check
      // the referenced type; do it now. This makes it fully recursive.
      t1->chk();

      size_t old_text = 0;
      if (t1->xerattrib && !t1->xerattrib->empty()) {
        old_text = t1->xerattrib->num_text_;
        *newx |= *t1->xerattrib;  // get the ancestor's attributes, except...
        newx->attribute_ = false; // attribute is not inherited
        newx->element_   = false; // element is not inherited

        if (ownertype == OT_TYPE_DEF
          ||ownertype == OT_COMP_FIELD) {
          // Name,Namespace is not inherited, X.693 amd1, 13.6
          // are you sure about the namespace?
          XerAttributes::FreeNameChange(newx->name_);
          //XerAttributes::FreeNamespace(newx->namespace_); // HR39678 bugfix beta
        }
      }
      // Now merge/override with our own attributes
      if (xerattrib && !xerattrib->empty()) {
        if (xerattrib->num_text_ > 0 && old_text > 0
          && (last->typetype == T_ENUM_T || last->typetype == T_ENUM_A)) {
          // Adding more TEXT attributes does not work right.
          error("Adding more TEXT attributes is not supported "
            "by the implementation.");
        }
        *newx |= *xerattrib;
      }

      if (newx->empty()) delete newx;
      else { // something interesting was found
        delete xerattrib;
        xerattrib = newx;

        if (xerattrib->attribute_
          && t1->xerattrib && t1->xerattrib->attribute_
          && t1->ownertype == OT_TYPE_DEF
          && !(xerattrib->form_ & XerAttributes::LOCALLY_SET) ) {
          // The referenced type came from a global XSD attribute.
          // This type's form will be qualified, unless it has an explicit
          // "form as ..." (which overrides everything).
          xerattrib->form_ |= XerAttributes::QUALIFIED;
        }

        if (t1->xerattrib && t1->xerattrib->element_
          // The referenced type came from a global XSD element.
          && !(xerattrib->form_ & XerAttributes::LOCALLY_SET))
        { // and it doesn't have an explicit "form as ..."
          xerattrib->form_ |= XerAttributes::QUALIFIED;
        }
      }
    } // if is_ref
  } // if ownertype

  if (!xerattrib) return;

//const char *fn = get_fullname().c_str();
//printf("chk_xer(%s)\n", fn);

  // In general, "last" should be used instead of "this" in the checks below
  // when accessing information about the type's characteristics, except
  // this->xerattrib MUST be used (and not last->xerattrib!).

  switch (ownertype) {
  case OT_COMP_FIELD:
  case OT_TYPE_DEF:
  case OT_RECORD_OF:
    // acceptable
    break;

  default:
    FATAL_ERROR("Unexpected ownertype %d", ownertype);
  }

  int num_attributes = 0;
  switch (last->typetype) {
  case T_SEQ_A: case T_SEQ_T:
  case T_SET_A: case T_SET_T: {
    // Count attributes (ANY-ATTRIBUTES counts as one)
    const size_t num_cf = last->get_nof_comps();
    int specials = xerattrib->embedValues_ + xerattrib->useOrder_;
    int num_any_attributes = 0;
    for (int x = num_cf - 1; x >= specials; --x) {
      CompField *cf = last->get_comp_byIndex(x);
      Type *cft = cf->get_type() /* NOT get_type_refd_last() */;
      if (cft->xerattrib
        && (cft->xerattrib->attribute_ || has_aa(cft->xerattrib)))
      {
        if (cft->xerattrib->attribute_) ++num_attributes;
        else if (++num_any_attributes > 1) {
          cf->error("There can be at most one field with ANY-ATTRIBUTES");
        }
      }
      else if (num_attributes + num_any_attributes > 0) {
        // Found a non-attribute when there was an attribute after it
        cf->error("Non-attribute field before attribute not supported");
      }
    } // next

    num_attributes += num_any_attributes;
    break; }

  default:
    break;
  }

  /* * * Check restrictions set out in X.693 amd1, clauses 18-39 * * */

  if (has_aa(xerattrib)) {
    chk_xer_any_attributes();
  } // if ANY-ATTRIBUTES

  if (has_ae(xerattrib)) {
    chk_xer_any_element();
  } // if ANY-ELEMENT

  if (xerattrib->attribute_) {
    chk_xer_attribute();
    // It's an attribute, check for the attributeFormQualified bit
    // and transform it into unconditionally qualified.
    if (!(xerattrib->form_ & XerAttributes::LOCALLY_SET)
      && (xerattrib->form_ & XerAttributes::ATTRIBUTE_DEFAULT_QUALIFIED)) {
      xerattrib->form_ |= XerAttributes::QUALIFIED;
    }
  }
  else {
    // Element, check the elementFormQualified bit.
    if (!(xerattrib->form_ & XerAttributes::LOCALLY_SET)
      && (xerattrib->form_ & XerAttributes::ELEMENT_DEFAULT_QUALIFIED)) {
      xerattrib->form_ |= XerAttributes::QUALIFIED;
    }
  } // if ATTRIBUTE

  if (xerattrib->base64_) {
    switch (last->typetype) {
    case T_OSTR: // OCTET STRING
    case T_HSTR: // hexstring
    case T_OPENTYPE:
    case T_BMPSTRING:
    case T_GENERALSTRING:
    case T_GRAPHICSTRING:
    case T_IA5STRING:
      // ISO646String ?
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_TELETEXSTRING:
      // T61
    case T_UNIVERSALSTRING:
    case T_UTF8STRING:
    case T_VIDEOTEXSTRING:
    case T_VISIBLESTRING:
      // TTCN types
    case T_CSTR:
    case T_USTR:
      break; // acceptable
    default:
      error("BASE64 can only be applied to OCTET STRING, open type or "
        "restricted character string type");
      break;
    } // switch

    if (has_ae(xerattrib) || xerattrib->whitespace_ != XerAttributes::PRESERVE) {
      error("A type with BASE64 shall not have any of the final "
        "encoding instructions ANY-ELEMENT or WHITESPACE");
    }
  } // if BASE64

  if (xerattrib->decimal_) {
    if (last->typetype != T_REAL) {
      error("DECIMAL shall only be assigned to a real type");
    }
    if (xerattrib->has_fractionDigits_) {
      if (xerattrib->fractionDigits_ < 0) {
        error("The value of fractionDigits must be equal or greater than zero");
      }
    }
  } else if (xerattrib->has_fractionDigits_) {
    error("The fractionDigits encoding instruction shall be used with XSD.Decimal types.");
  } // if DECIMAL 

  if (xerattrib->defaultForEmpty_ != 0 || xerattrib->defaultForEmptyIsRef_) {
    chk_xer_dfe();
  } // if defaultForEmpty

  chk_xer_embed_values(num_attributes); // always

  if (xerattrib->list_) {
    switch (last->typetype) {
    case T_SEQOF: case T_SETOF:
      break; // acceptable
    default:
      error("LIST can only be assigned to SEQUENCE-OF or SET-OF");// 27.2.1
      break;
    }

    if (has_aa(xerattrib)) {
      error("A type with LIST shall not have ANY-ATTRIBUTES");// 27.2.3
    }
  } // if LIST

  // NAME is handled later when generate_code_xerdescriptor call change_name

  if ((unsigned long)xerattrib->namespace_.keyword
    > (unsigned long)NamespaceSpecification::LOWERCASED) {
    // Now both are proper non-NULL strings
    if (*xerattrib->namespace_.prefix != 0) { // there is a prefix, check it
      char first[5] = {0,0,0,0,0};
      strncpy(first, xerattrib->namespace_.prefix, 4);
      bool xml_start = !memcmp(first, "xml", 3);
      first[0] = (char)toupper(first[0]);
      first[1] = (char)toupper(first[1]);
      first[2] = (char)toupper(first[2]);
      if ((xml_start // It _is_ "xml" or starts with "xml"
        // Or it matches (('X'|'x') ('M'|'m') ('L'|'l'))
        || (first[3] == 0 && !memcmp(first, "XML", 3)))
        // but the W3C XML namespace gets an exemption
        && strcmp(xerattrib->namespace_.uri, xml98)) error(
          "Prefix shall not start with 'xml' and shall not match the pattern"
          " (('X'|'x')('M'|'m')('L'|'l'))");
        // X.693 (11/2008), clause 29.1.7
    }

    Common::Module::add_namespace(
      xerattrib->namespace_.uri, xerattrib->namespace_.prefix);
  }
  else xerattrib->namespace_.uri = NULL; // really no namespace

  // PI-OR-COMMENT not supported

  if (xerattrib->num_text_ > 0) {
    chk_xer_text();
  }

  if (xerattrib->untagged_) {
    chk_xer_untagged();
  } // if UNTAGGED

  chk_xer_use_nil(); // always

  if (xerattrib->useNumber_) {
    switch (last->typetype) {
    case T_ENUM_A:
      error("No XER yet for ASN.1 enumerations");
      // no break
    case T_ENUM_T:
      break; // acceptable
    default:
      warning("USE-NUMBER ignored unless assigned to an enumerated type");
      xerattrib->useNumber_ = false;
      break;
    }

    if (xerattrib->num_text_) {
      error("A type with USE-NUMBER shall not have TEXT");
    }
  } // if USE-NUMBER

  chk_xer_use_order(num_attributes); //always

  if (xerattrib->useQName_) {
    switch (last->typetype) {
    case T_SEQ_A: case T_SEQ_T: {
      if (last->get_nof_comps() != 2) goto complain;
      const CompField * cf = last->get_comp_byIndex(0);
      const Type * cft = cf->get_type()->get_type_refd_last();
      if (cft->typetype != T_USTR && cft->typetype != T_UTF8STRING)
        cft->error ("Both components must be UTF8String or universal charstring");
      if (!cf->get_is_optional()) cft->error(
        "The first component of a type with USE-QNAME must be optional");

      cf = last->get_comp_byIndex(1);
      cft = cf->get_type()->get_type_refd_last();
      if (cft->typetype != T_USTR && cft->typetype != T_UTF8STRING)
        cft->error ("Both components must be UTF8String or universal charstring");
      if (cf->get_is_optional()) cft->error(
        "The second component of a type with USE-QNAME must NOT be optional");
      break; }

    default:
      complain:
      error("A type with USE-QNAME must be a sequence type with exactly two components.");
      break;
    }
    if (xerattrib->useNil_) error("A type with USE-QNAME shall not have USE-NIL"); // 36.2.4
  }

  if (xerattrib->useType_) {
    chk_xer_use_type();
  } // if USE-TYPE

  if (xerattrib->useUnion_) {
    chk_xer_use_union();
  }

  if (is_secho()) {
    CompFieldMap& cfm = *u.secho.cfm;
    const size_t ncomps = cfm.get_nof_comps();
    CompField *the_one = 0; // ...and only untagged character-encodable field
    map<size_t, CompField> empties; // potentially empties

    for (size_t i=0; i < ncomps; ++i) {
      CompField * cf = cfm.get_comp_byIndex(i);
      Type *cft = cf->get_type();

      if (cft->xerattrib && cft->xerattrib->untagged_) {
        /* This check could be in chk_xer_untagged(), but then we couldn't
         * access the CompField in the parent type. */
        if (cft->is_charenc() == Yes) {
          if (the_one) { // already has one
            cf->error("More than one UNTAGGED character-encodable field");
            // break? to report only once
          }
          else {
            the_one = cf; // this is used for further checks below
            u.secho.has_single_charenc = true; // used while generating code
            switch (typetype) {
            case T_SEQ_A: case T_SEQ_T:
              if (xerattrib->untagged_) {
                error("Enclosing type of an UNTAGGED character-encodable type "
                  "must not be UNTAGGED");
              }
              break; // the small switch
            default:
              error("Enclosing type of an UNTAGGED character-encodable type "
                "is not record.");
              break;
            } // switch
          } // if (the_one)
        }
        else { // untagged, not charenc
          switch (cft->get_type_refd_last()->typetype) {
          case T_SEQ_A: case T_SEQ_T:
          case T_SET_A: case T_SET_T:
          case T_ANYTYPE:
          case T_CHOICE_A: case T_CHOICE_T:
          case T_SEQOF:
          case T_SETOF:
            //case T_OSTR:   with a contained type
            //case T_BSTR_A: with a contained type
            //case T_OPENTYPE:
            break; //ok

          default: // 32.2.3 "If the type is not character-encodable..."
            cft->error("UNTAGGED type should be sequence, set, choice, sequence-of, or set-of type");
            break;
          }
        } // if(is_charenc)
      } // if untagged

      if (cft->has_empty_xml()) {
        empties.add(i, cf);
      }
    } // next component

    if (the_one) {
      if (the_one->get_is_optional() || the_one->has_default()) {
        the_one->error("UNTAGGED field should not be marked OPTIONAL or DEFAULT");
      }
      // Check the other compfields. They must all be (ANY)?ATTRIBUTE
      for (size_t i=0; i < ncomps; ++i) {
        CompField *cf = cfm.get_comp_byIndex(i);
        if (cf == the_one) continue;

        Type *cft = cf->get_type();
        if (  !cft->xerattrib // cannot be attribute, error
          || (!cft->xerattrib->attribute_ && !has_aa(cft->xerattrib))) {
          the_one->note("Due to this UNTAGGED component");
          cf->error("All the other components should be ATTRIBUTE or ANY-ATTRIBUTE");
          // X.693 (2008) 32.2.2
          break;
        }
      } // next i
    } // if the_one

    if (empties.size() > 1
      && (typetype==T_CHOICE_A || typetype==T_CHOICE_T || typetype==T_ANYTYPE)) {
      warning("More than one field can have empty XML. Decoding of empty"
        " XML is ambiguous, %s chosen arbitrarily.",
        empties.get_nth_elem(empties.size()-1)->get_name().get_name().c_str());
    }
    empties.clear();
  } // if secho
  
  if (xerattrib->abstract_ || xerattrib->block_) {
    switch (ownertype) {
    case OT_COMP_FIELD:
      if (parent_type->typetype == T_CHOICE_A ||
          parent_type->typetype == T_CHOICE_T) {
        if (parent_type->xerattrib != NULL && parent_type->xerattrib->useUnion_) {
          error("ABSTRACT and BLOCK cannot be used on fields of a union with "
            "attribute USE-UNION.");
        }
        break;
      }
      // else fall through
    case OT_RECORD_OF:
    case OT_TYPE_DEF:
      warning("ABSTRACT and BLOCK only affects union fields.");
      break;
    default:
      break;
    }
  }

}


void Type::chk_Int_A()
{
  if(checked) return;
  checked=true;
  if(!u.namednums.block) return;
  parse_block_Int();
  if(typetype==T_ERROR) return;
  /* check named numbers */
  if(!u.namednums.nvs) return;
  map<Int, NamedValue> value_map;
  Error_Context cntxt(this, "In named numbers");
  u.namednums.nvs->chk_dupl_id();
  for (size_t i = 0; i < u.namednums.nvs->get_nof_nvs(); i++) {
    NamedValue *nv = u.namednums.nvs->get_nv_byIndex(i);
    Value *value = nv->get_value();
    Value *v = value->get_value_refd_last();
    switch (v->get_valuetype()) {
    case Value::V_INT: {
      const int_val_t *int_val_int = v->get_val_Int();
      if (*int_val_int > INT_MAX) {
        value->error("Integer value `%s' is too big to be used as a named "
          "number", (int_val_int->t_str()).c_str());
      } else {
        Int int_val = int_val_int->get_val();
        if (value_map.has_key(int_val)) {
          value->error("Duplicate number %s for name `%s'",
            Int2string(int_val).c_str(),
            nv->get_name().get_dispname().c_str());
          NamedValue *nv2 = value_map[int_val];
          nv2->note("Number %s is already assigned to name `%s'",
            Int2string(int_val).c_str(),
            nv2->get_name().get_dispname().c_str());
        } else {
          value_map.add(int_val, nv);
        }
      }
      break; }
    case Value::V_ERROR:
      break;
    default:
      nv->error("INTEGER value was expected for named number `%s'",
                nv->get_name().get_dispname().c_str());
      break;
    }
  }
  value_map.clear();
}

void Type::chk_Enum_A()
{
  if(checked) return;
  if(!u.enums.block) return;
  parse_block_Enum();
  if(typetype==T_ERROR) return;
  /* checking enumerations */
  map<Int, EnumItem> value_map;
  /* checking values before the ellipsis */
  for (size_t i = 0; i < u.enums.eis1->get_nof_eis(); i++)
    chk_Enum_item(u.enums.eis1->get_ei_byIndex(i), false, value_map);
  /* assigning default values */
  Int& first_unused = u.enums.first_unused;
  for (first_unused = 0; value_map.has_key(first_unused); first_unused++) ;
  for (size_t i = 0; i < u.enums.eis1->get_nof_eis(); i++) {
    EnumItem *ei = u.enums.eis1->get_ei_byIndex(i);
    if (!ei->get_value()) {
      ei->set_value(new Value(Value::V_INT, first_unused));
      value_map.add(first_unused, ei);
      while (value_map.has_key(++first_unused)) ;
    } else {
      ei->calculate_int_value();
    }
  }
  /* checking values after the ellipsis */
  if(u.enums.eis2) {
    for(size_t i=0; i < u.enums.eis2->get_nof_eis(); i++)
      chk_Enum_item(u.enums.eis2->get_ei_byIndex(i), true, value_map);
  }
  /* determining the first two unused non-negative integer values
   * for code generation */
  for (first_unused = 0; value_map.has_key(first_unused); first_unused++) ;
  Int& second_unused = u.enums.second_unused;
  for (second_unused = first_unused + 1; value_map.has_key(second_unused);
    second_unused++) ;
  value_map.clear();
  u.enums.eis1->release_eis();
  delete u.enums.eis1;
  u.enums.eis1 = 0;
  if(u.enums.eis2) {
    u.enums.eis2->release_eis();
    delete u.enums.eis2;
    u.enums.eis2 = 0;
  }
}

void Type::chk_Enum_item(EnumItem *ei, bool after_ellipsis,
                         map<Int, EnumItem>& value_map)
{
  const Identifier& name = ei->get_name();
  const char *dispname_str = name.get_dispname().c_str();
  if (u.enums.eis->has_ei_withName(name)) {
    ei->error("Duplicate ENUMERATED identifier: `%s'", dispname_str);
    u.enums.eis->get_ei_byName(name)->note("Previous definition of `%s' "
      "is here", dispname_str);
  } else if (!name.get_has_valid(Identifier::ID_TTCN)) {
    ei->warning("The identifier `%s' is not reachable from TTCN-3",
      dispname_str);
  }
  u.enums.eis->add_ei(ei);
  Int& first_unused=u.enums.first_unused;
  Value *value = ei->get_value();
  if (value) {
    ei->calculate_int_value();
    Value *v;
    {
      Error_Context cntxt(ei, "In enumeration `%s'", dispname_str);
      v = value->get_value_refd_last();
    }
    switch (v->get_valuetype()) {
    case Value::V_INT:
      break;
    case Value::V_ERROR:
      return;
    default:
      value->error("INTEGER value was expected for enumeration `%s'",
                   dispname_str);
      return;
    }
    // This function is only called for ASN1 enumerated types.
    // The enum_value can be get by calling v->get_int_val() which also supports
    // binary, hex, oct values. ASN1 enumerated fields cannot have  
    // binary, hex, or oct value.
    Int enum_value = v->get_val_Int()->get_val();
    if (static_cast<Int>(static_cast<int>(enum_value)) != enum_value) {
      value->error("The numeric value of enumeration `%s' (%s) is too "
        "large for being represented in memory", dispname_str,
        Int2string(enum_value).c_str());
    }
    if (after_ellipsis) {
      if (enum_value >= first_unused) {
        value_map.add(enum_value, ei);
        for (first_unused = enum_value + 1; value_map.has_key(first_unused);
             first_unused++) ;
      } else {
        value->error("ENUMERATED values shall be monotonically growing after "
          "the ellipsis: the value of `%s' must be at least %s instead of %s",
          dispname_str, Int2string(first_unused).c_str(),
          Int2string(enum_value).c_str());
      }
    } else {
      if (value_map.has_key(enum_value)) {
        value->error("Duplicate numeric value %s for enumeration `%s'",
          Int2string(enum_value).c_str(), dispname_str);
        EnumItem *ei2 = value_map[enum_value];
        ei2->note("Value %s is already assigned to `%s'",
          Int2string(enum_value).c_str(),
          ei2->get_name().get_dispname().c_str());
      } else value_map.add(enum_value, ei);
    }
  } else { // the item has no value
    if (after_ellipsis) {
      ei->set_value(new Value(Value::V_INT, first_unused));
      value_map.add(first_unused, ei);
      while (value_map.has_key(++first_unused)) ;
    }
  }
}

void Type::chk_Enum_T()
{
  if(checked) return;
  map<Int, EnumItem> value_map;
  map<string, EnumItem> name_map;
  bool error_flag = false;
  /* checking the uniqueness of identifiers and values */
  size_t nof_eis = u.enums.eis->get_nof_eis();
  for(size_t i = 0; i < nof_eis; i++) {
    EnumItem *ei = u.enums.eis->get_ei_byIndex(i);
    const Identifier& id = ei->get_name();
    const string& name = id.get_name();
    if (name_map.has_key(name)) {
      const char *dispname_str = id.get_dispname().c_str();
      ei->error("Duplicate enumeration identifier `%s'", dispname_str);
      name_map[name]->note("Previous definition of `%s' is here",
        dispname_str);
      error_flag = true;
    } else {
      name_map.add(name, ei);
    }
    Value *value = ei->get_value();
    if (value) {
      /* Handle the error node created by the parser.  */
      if (value->get_valuetype() == Value::V_ERROR) {
        value->error("INTEGER or BITSTRING or OCTETSTRING or HEXSTRING value was expected for enumeration `%s'",
          id.get_dispname().c_str());
        error_flag = true;
      } else {
        if (ei->calculate_int_value()) {
          const int_val_t *enum_value_int = ei->get_int_val();
          // It used to be the same check.
          if (*enum_value_int > INT_MAX ||
              static_cast<Int>(static_cast<int>(enum_value_int->get_val()))
              != enum_value_int->get_val()) {
            value->error("The numeric value of enumeration `%s' (%s) is "
              "too large for being represented in memory",
              id.get_dispname().c_str(), (enum_value_int->t_str()).c_str());
            error_flag = true;
          } else {
            Int enum_value = enum_value_int->get_val();
            if (value_map.has_key(enum_value)) {
              const char *dispname_str = id.get_dispname().c_str();
              value->error("Duplicate numeric value %s for enumeration `%s'",
                Int2string(enum_value).c_str(), dispname_str);
              EnumItem *ei2 = value_map[enum_value];
              ei2->note("Value %s is already assigned to `%s'",
                Int2string(enum_value).c_str(),
                ei2->get_name().get_dispname().c_str());
              error_flag = true;
            } else {
              value_map.add(enum_value, ei);
            }
          }
        } else {
          error_flag = true;
        }
      }
    }
  }
  if (!error_flag) {
    /* assigning default values */
    Int& first_unused=u.enums.first_unused;
    for(first_unused=0; value_map.has_key(first_unused); first_unused++) ;
    for(size_t i = 0; i < nof_eis; i++) {
      EnumItem *ei = u.enums.eis->get_ei_byIndex(i);
      if(!ei->get_value()) {
        ei->set_value(new Value(Value::V_INT, first_unused));
        value_map.add(first_unused, ei);
        while (value_map.has_key(++first_unused)) ;
      } else {
        ei->calculate_int_value();
      }
    }
    Int& second_unused = u.enums.second_unused;
    for (second_unused = first_unused + 1; value_map.has_key(second_unused);
      second_unused++) ;
  }
  value_map.clear();
  name_map.clear();
}

void Type::chk_BStr_A()
{
  if(!u.namednums.block) return;
  parse_block_BStr();
  if(typetype==T_ERROR) return;
  if(u.namednums.nvs) {
    /* check named bits */
    map<Int, NamedValue> value_map;
    {
      Error_Context cntxt(this, "In named bits");
      u.namednums.nvs->chk_dupl_id();
    }
    for(size_t i = 0; i < u.namednums.nvs->get_nof_nvs(); i++) {
      NamedValue *nv = u.namednums.nvs->get_nv_byIndex(i);
      const char *dispname_str = nv->get_name().get_dispname().c_str();
      Value *value = nv->get_value();
      Value *v;
      {
        Error_Context cntxt(this, "In named bit `%s'", dispname_str);
        v = value->get_value_refd_last();
      }
      switch (v->get_valuetype()) {
      case Value::V_INT:
        break;
      case Value::V_ERROR:
        continue;
      default:
        v->error("INTEGER value was expected for named bit `%s'",
          dispname_str);
        continue;
      }
      const int_val_t *int_val_int = v->get_val_Int();
      if (*int_val_int > INT_MAX) {
        value->error("An INTEGER value less than `%d' was expected for "
          "named bit `%s' instead of %s", INT_MAX, dispname_str,
          (int_val_int->t_str()).c_str());
        continue;
      }
      Int int_val = int_val_int->get_val();
      if(int_val < 0) {
        value->error("A non-negative INTEGER value was expected for named "
          "bit `%s' instead of %s", dispname_str,
          Int2string(int_val).c_str());
        continue;
      }
      if (value_map.has_key(int_val)) {
        value->error("Duplicate value %s for named bit `%s'",
          Int2string(int_val).c_str(), dispname_str);
        NamedValue *nv2 = value_map[int_val];
        nv2->note("Bit %s is already assigned to name `%s'",
                   Int2string(int_val).c_str(),
                   nv2->get_name().get_dispname().c_str());
      } else value_map.add(int_val, nv);
    }
    value_map.clear();
  }
}

void Type::chk_SeCho_T()
{
  u.secho.cfm->set_my_type(this);
  checked = true;
  u.secho.cfm->chk();
  u.secho.component_internal = false;
  size_t nof_comps = u.secho.cfm->get_nof_comps();
  for (size_t i=0; i<nof_comps; i++) {
    Type* cft = u.secho.cfm->get_comp_byIndex(i)->get_type();
    if (cft && cft->is_component_internal()) {
      u.secho.component_internal = true;
      break;
    }
  }
}

void Type::chk_Choice_A()
{
  if(u.secho.block) parse_block_Choice();
  if(typetype==T_ERROR) return;
  checked=true;
  if(u.secho.ctss->needs_auto_tags())
    u.secho.ctss->add_auto_tags();
  u.secho.ctss->chk();
  if (u.secho.ctss->get_nof_comps() <= 0)
    FATAL_ERROR("CHOICE type must have at least one alternative");
  u.secho.ctss->chk_tags();
}

void Type::chk_Se_A()
{
  if(u.secho.block) parse_block_Se();
  if(typetype==T_ERROR) return;
  checked=true;
  tr_compsof();
  u.secho.ctss->chk();
  u.secho.ctss->chk_tags();
}

void Type::chk_SeOf()
{
  u.seof.ofType->set_genname(get_genname_own(), string('0'));
  u.seof.ofType->set_parent_type(this);
  checked=true;
  const char *type_name;
  bool asn1 = is_asn1();
  switch (typetype) {
  case T_SEQOF:
    type_name = asn1 ? "SEQUENCE OF" : "record of";
    break;
  case T_SETOF:
    type_name = asn1 ? "SET OF" : "set of";
    break;
  default:
    type_name = "<unknown>";
    break;
  }
  Error_Context cntxt(this, "In embedded type of %s", type_name);
  u.seof.ofType->chk();
  if (!asn1) u.seof.ofType->chk_embedded(true, "embedded into another type");
  u.seof.component_internal = u.seof.ofType->is_component_internal();
}

void Type::chk_refd()
{
  checked=true;
  ReferenceChain refch(this, "While checking referenced type");
  Type* t_last = get_type_refd_last(&refch);
  u.ref.component_internal = t_last->is_component_internal();
}

void Type::chk_seltype()
{
  checked=true;
  u.seltype.type->set_genname(get_genname_own(), string('0'));
  ReferenceChain refch(this, "In selection type");
  get_type_refd_last(&refch);
  if(typetype==T_ERROR) return;
  u.seltype.type->chk();
}

void Type::chk_Array()
{
  u.array.element_type->set_genname(get_genname_own(), string('0'));
  u.array.element_type->set_parent_type(this);
  checked = true;
  {
    Error_Context cntxt(this, "In element type of array");
    u.array.element_type->chk();
    u.array.element_type->chk_embedded(true, "embedded into an array type");
  }
  u.array.dimension->chk();
  u.array.component_internal = u.array.element_type->is_component_internal();
}

void Type::chk_Signature()
{
  checked = true;
  u.signature.component_internal = false;
  if (u.signature.parameters) {
    u.signature.parameters->chk(this);
    size_t nof_params = u.signature.parameters->get_nof_params();
    for (size_t i=0; i<nof_params; i++) {
      Type* spt = u.signature.parameters->get_param_byIndex(i)->get_type();
      if (spt && spt->is_component_internal()) {
        u.signature.component_internal = true;
        break;
      }
    }
  }
  if (u.signature.return_type) {
    Error_Context cntxt(u.signature.return_type, "In return type");
    u.signature.return_type->set_genname(get_genname_own(), string('0'));
    u.signature.return_type->set_parent_type(this);
    u.signature.return_type->chk();
    u.signature.return_type->chk_embedded(false,
      "the return type of a signature");
    if (!u.signature.component_internal &&
      u.signature.return_type->is_component_internal())
      u.signature.component_internal = true;
  }
  if (u.signature.exceptions) {
    u.signature.exceptions->chk(this);
    if (!u.signature.component_internal) {
      size_t nof_types = u.signature.exceptions->get_nof_types();
      for (size_t i=0; i<nof_types; i++) {
        if (u.signature.exceptions->get_type_byIndex(i)->
          is_component_internal()) {
          u.signature.component_internal = true;
          break;
        }
      }
    }
  }
}

void Type::chk_Fat()
{
  if(checked) return;
  checked = true;
  Scope *parlist_scope = my_scope;
  // the runs on clause must be checked before the formal parameter list
  // in order to determine the right scope
  if (u.fatref.runs_on.ref) {
    if (typetype == T_FUNCTION) u.fatref.is_startable = true;
    Error_Context cntxt2(u.fatref.runs_on.ref, "In `runs on' clause");
    u.fatref.runs_on.type = u.fatref.runs_on.ref->chk_comptype_ref();
    if(u.fatref.runs_on.type) {
      Ttcn::Module *my_module =
        dynamic_cast<Ttcn::Module*>(my_scope->get_scope_mod());
      if (!my_module) FATAL_ERROR("Type::chk_Fat()");
      parlist_scope = my_module->get_runs_on_scope(u.fatref.runs_on.type);
    }
  }
  u.fatref.fp_list->set_my_scope(parlist_scope);
  switch (typetype) {
  case T_FUNCTION:
    u.fatref.fp_list->chk(Ttcn::Definition::A_FUNCTION);
    u.fatref.fp_list->chk_noLazyFuzzyParams();
    if (u.fatref.is_startable && !u.fatref.fp_list->get_startability())
      u.fatref.is_startable = false;
    if (u.fatref.return_type) {
      Error_Context cntxt2(u.fatref.return_type, "In return type");
      u.fatref.return_type->chk();
      u.fatref.return_type->chk_as_return_type(!u.fatref.returns_template,
        "function type");
      if (u.fatref.is_startable && u.fatref.return_type->get_type_refd_last()
          ->get_typetype() == T_DEFAULT) u.fatref.is_startable = false;

    }
    break;
  case T_ALTSTEP:
    u.fatref.fp_list->chk(Ttcn::Definition::A_ALTSTEP);
    u.fatref.fp_list->chk_noLazyFuzzyParams();
    break;
  case T_TESTCASE:
    u.fatref.fp_list->chk(Ttcn::Definition::A_TESTCASE);
    u.fatref.fp_list->chk_noLazyFuzzyParams();
    if (u.fatref.system.ref) {
      Error_Context cntxt2(u.fatref.runs_on.ref, "In `system' clause");
      u.fatref.system.type = u.fatref.system.ref->chk_comptype_ref();
    }
    break;
  default:
    FATAL_ERROR("Type::chk_Fat()");
  }
  if (!semantic_check_only)
    u.fatref.fp_list->set_genname(get_genname_own());
}

void Type::chk_address()
{
  Type *t = get_type_refd_last();
  switch (t->typetype) {
  case T_PORT:
    error("Port type `%s' cannot be the address type",
      t->get_typename().c_str());
    break;
  case T_COMPONENT:
    error("Component type `%s' cannot be the address type",
      t->get_typename().c_str());
    break;
  case T_SIGNATURE:
    error("Signature `%s' cannot be the address type",
      t->get_typename().c_str());
    break;
  case T_DEFAULT:
    error("Default type cannot be the address type");
    break;
  case T_ANYTYPE:
    error("TTCN-3 anytype cannot be the address type");
    break;
  default:
    return;
  }
  clean_up();
}

void Type::chk_embedded(bool default_allowed, const char *error_msg)
{
  Type *t=get_type_refd_last();
  switch (t->typetype) {
  case T_PORT:
    error("Port type `%s' cannot be %s", t->get_typename().c_str(),
      error_msg);
    break;
  case T_SIGNATURE:
    error("Signature `%s' cannot be %s", t->get_typename().c_str(),
      error_msg);
    break;
  case T_DEFAULT:
    if (!default_allowed) error("Default type cannot be %s", error_msg);
    break;
  default:
    break;
  }
}

void Type::chk_recursions(ReferenceChain& refch)
{
  if (recurs_checked) return;
  Type *t = this;
  for ( ; ; ) {
    if (!refch.add(t->get_fullname())) {
      // an error has been found (and reported by refch.add)
      recurs_checked = true;
      return;
    }
    if (!t->is_ref()) break;
    t = t->get_type_refd();
  }
  switch (t->typetype) {
  case T_SEQ_A:
  case T_SEQ_T:
  case T_SET_A:
  case T_SET_T:
    for(size_t i = 0; i < t->get_nof_comps(); i++) {
      CompField *cf = t->get_comp_byIndex(i);
      if(!cf->get_is_optional()) {
        refch.mark_state();
        cf->get_type()->chk_recursions(refch);
        refch.prev_state();
      }
      // an optional field can "stop" the recursion
    } // for i
    break;
  case T_CHOICE_A:
  case T_CHOICE_T: {
    refch.set_error_reporting(false);
    refch.mark_error_state();
    for (size_t i = 0; i < t->get_nof_comps(); ++i) {
      refch.mark_state();
      CompField* cf = t->get_comp_byIndex(i);
      cf->get_type()->chk_recursions(refch);
      refch.prev_state();
    }

    if (refch.nof_errors() == t->get_nof_comps()) {
      refch.report_errors();
    }

    refch.prev_error_state();
    refch.set_error_reporting(true);
    break;
  }
  case T_ANYTYPE:
  case T_OPENTYPE:
    if(t->get_nof_comps()==1)
      t->get_comp_byIndex(0)->get_type()->chk_recursions(refch);
    break;
  case T_ARRAY:
    t->get_ofType()->chk_recursions(refch);
    break;
  default:
    break;
  }
  recurs_checked = true;
}

void Type::chk_constructor_name(const Identifier& p_id)
{
  switch (typetype) {
  case T_SEQ_T:
  case T_SEQ_A:
  case T_SET_T:
  case T_SET_A:
  case T_CHOICE_T:
  case T_CHOICE_A:
  case T_OPENTYPE:
    // T_ANYTYPE can not have a field of type anytype, no need to check.
    if (has_comp_withName(p_id)) {
      // HL26011: a field has the same name as the typedef.
      // Titan can't generate valid C++ code; better report an error.
      get_comp_byName(p_id)->error("Field name clashes with type name");
    }
    break;
  default: // can't have fields
    break;
  }
}

bool Type::chk_startability()
{
  if(typetype != T_FUNCTION) FATAL_ERROR("Type::chk_startable()");
  if(!checked) chk_Fat();
  if(u.fatref.is_startable) return true;
  if (!u.fatref.runs_on.ref) error("Functions of type `%s' cannot be started "
    "on a parallel test component because the type does not have `runs on' "
    "clause", get_typename().c_str());
  u.fatref.fp_list->chk_startability("Functions of type",
    get_typename().c_str());
  if (u.fatref.return_type && u.fatref.return_type->is_component_internal()) {
    map<Type*,void> type_chain;
    char* err_str = mprintf("the return type or embedded in the return type "
      "of function type `%s' if it is started on a parallel test component",
      get_typename().c_str());
    u.fatref.return_type->chk_component_internal(type_chain, err_str);
    Free(err_str);
  }
  return false;
}

void Type::chk_as_return_type(bool as_value, const char* what)
{
  Type *t = get_type_refd_last();
  switch(t->get_typetype()) {
  case Type::T_PORT:
    error("Port type `%s' cannot be the return type of a %s"
      , t->get_fullname().c_str(), what);
    break;
  case Type::T_SIGNATURE:
    if(as_value) error("A value of signature `%s' cannot be the "
      "return type of a %s", t->get_fullname().c_str(), what);
    break;
  default:
    break;
  }
}

void Type::chk_this_value_ref(Value *value)
{
  switch (value->get_valuetype()) {
  case Value::V_UNDEF_LOWERID:
    switch(typetype) {
    case T_INT_A:
      if(value->is_asn1()) {
        chk_Int_A();
        if(u.namednums.nvs
           && u.namednums.nvs->has_nv_withName(*value->get_val_id())) {
          value->set_valuetype(Value::V_NAMEDINT);
          value->set_my_governor(this);
          return;
        }
      }
      break;
    case T_ENUM_A:
    case T_ENUM_T:
      if (has_ei_withName(*value->get_val_id())) {
        value->set_valuetype(Value::V_ENUM);
        value->set_my_governor(this);
        return;
      }
      break;
    case T_REFD:
    case T_SELTYPE:
    case T_REFDSPEC:
    case T_OCFT:
      get_type_refd()->chk_this_value_ref(value);
      return;
    default:
      break;
    } // switch
    /* default behavior: interpret as reference */
    value->set_valuetype(Value::V_REFD);
    break;
  default:
    break;
  }
}

bool Type::chk_this_value(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
                          namedbool incomplete_allowed, namedbool omit_allowed,
                          namedbool sub_chk, namedbool implicit_omit, namedbool is_str_elem)
{
  bool self_ref = false;
  chk();
  Value *v_last = value->get_value_refd_last(0, expected_value);
  if (v_last->get_valuetype() == Value::V_OMIT) {
    if (OMIT_NOT_ALLOWED == omit_allowed) {
      value->error("`omit' value is not allowed in this context");
      value->set_valuetype(Value::V_ERROR);
    }
    return false;
  }

  switch (value->get_valuetype()) {
  case Value::V_ERROR:
    return false;
  case Value::V_REFD:
    return chk_this_refd_value(value, lhs, expected_value, 0, is_str_elem);
  case Value::V_INVOKE:
    chk_this_invoked_value(value, lhs, expected_value);
    return false; // assumes no self-reference in invoke
  case Value::V_EXPR:
    if (lhs) self_ref = value->chk_expr_self_ref(lhs);
    // no break
  case Value::V_MACRO:
    if (value->is_unfoldable(0, expected_value)) {
      typetype_t tt = value->get_expr_returntype(expected_value);
      if (!is_compatible_tt(tt, value->is_asn1(), value->get_expr_governor_last())) {
        value->error("Incompatible value: `%s' value was expected",
                     get_typename().c_str());
        value->set_valuetype(Value::V_ERROR);
      }
      return self_ref;
    }
    break;
  case Value::V_ANY_VALUE:
  case Value::V_ANY_OR_OMIT:
    value->error("%s is not allowed in this context",
      value->get_valuetype() == Value::V_ANY_VALUE ? "any value" : "any or omit");
    value->set_valuetype(Value::V_ERROR);
    return self_ref;
  default:
    break;
  }

  switch(typetype) {
  case T_ERROR:
    break;
  case T_NULL:
    chk_this_value_Null(value);
    break;
  case T_BOOL:
    chk_this_value_Bool(value);
    break;
  case T_INT:
    chk_this_value_Int(value);
    break;
  case T_INT_A:
    chk_this_value_Int_A(value);
    break;
  case T_REAL:
    chk_this_value_Real(value);
    break;
  case T_ENUM_A:
  case T_ENUM_T:
    chk_this_value_Enum(value);
    break;
  case T_BSTR:
    chk_this_value_BStr(value);
    break;
  case T_BSTR_A:
    chk_this_value_BStr_A(value);
    break;
  case T_HSTR:
    chk_this_value_HStr(value);
    break;
  case T_OSTR:
    chk_this_value_OStr(value);
    break;
  case T_CSTR:
  case T_USTR:
  case T_UTF8STRING:
  case T_NUMERICSTRING:
  case T_PRINTABLESTRING:
  case T_TELETEXSTRING:
  case T_VIDEOTEXSTRING:
  case T_IA5STRING:
  case T_GRAPHICSTRING:
  case T_VISIBLESTRING:
  case T_GENERALSTRING:
  case T_UNIVERSALSTRING:
  case T_BMPSTRING:
  case T_UTCTIME:
  case T_GENERALIZEDTIME:
  case T_OBJECTDESCRIPTOR:
    chk_this_value_CStr(value);
    break;
  case T_OID:
    chk_this_value_OID(value);
    break;
  case T_ROID:
    chk_this_value_ROID(value);
    break;
  case T_ANY:
    chk_this_value_Any(value);
    break;
  case T_CHOICE_A:
  case T_CHOICE_T:
  case T_OPENTYPE:
  case T_ANYTYPE:
    self_ref = chk_this_value_Choice(value, lhs, expected_value, incomplete_allowed,
      implicit_omit);
    break;
  case T_SEQ_T:
  case T_SET_T:
  case T_SEQ_A:
  case T_SET_A:
    self_ref = chk_this_value_Se(value, lhs, expected_value, incomplete_allowed,
      implicit_omit);
    break;
  case T_SEQOF:
  case T_SETOF:
    self_ref = chk_this_value_SeOf(value, lhs, expected_value, incomplete_allowed,
      implicit_omit);
    break;
  case T_REFD:
    self_ref = get_type_refd()->chk_this_value(value, lhs, expected_value,
      incomplete_allowed, omit_allowed, NO_SUB_CHK, implicit_omit, is_str_elem);
    break;
  case T_UNRESTRICTEDSTRING:
  case T_OCFT:
  case T_EXTERNAL:
  case T_EMBEDDED_PDV:
  case T_REFDSPEC:
  case T_SELTYPE:
  case T_ADDRESS:
    self_ref = get_type_refd()->chk_this_value(value, lhs, expected_value,
      incomplete_allowed, omit_allowed, NO_SUB_CHK, NOT_IMPLICIT_OMIT, is_str_elem);
    break;
  case T_VERDICT:
    chk_this_value_Verdict(value);
    break;
  case T_DEFAULT:
    chk_this_value_Default(value);
    break;
  case T_ARRAY:
    self_ref = chk_this_value_Array(value, lhs, expected_value, incomplete_allowed, implicit_omit);
    break;
  case T_PORT:
    // Remain silent. The error has already been reported in the definition
    // that the value belongs to.
    break;
  case T_SIGNATURE:
    self_ref = chk_this_value_Signature(value, lhs, expected_value, incomplete_allowed);
    break;
  case T_COMPONENT:
    chk_this_value_Component(value);
    break;
  case T_FUNCTION:
  case T_ALTSTEP:
  case T_TESTCASE:
    chk_this_value_FAT(value);
    break;
  default:
    FATAL_ERROR("Type::chk_this_value()");
  } // switch

  // check value against subtype
  if(SUB_CHK == sub_chk && (sub_type!=NULL)) sub_type->chk_this_value(value);

  return self_ref;
}

bool Type::chk_this_refd_value(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
                               ReferenceChain* refch, namedbool str_elem)
{
  Reference *ref = value->get_reference();
  Assignment *ass = ref->get_refd_assignment();
  if (!ass) {
    value->set_valuetype(Value::V_ERROR);
    return false;
  }

  ass->chk();
  bool self_ref = (ass == lhs);

  // Exit immediately in case of infinite recursion.
  if (value->get_valuetype() != Value::V_REFD) return self_ref;
  bool is_const = false, error_flag = false, chk_runs_on = false;
  Type *governor = NULL;
  switch (ass->get_asstype()) {
  case Assignment::A_ERROR:
    value->set_valuetype(Value::V_ERROR);
    return self_ref;
  case Assignment::A_CONST:
    is_const = true;
    break;
  case Assignment::A_OBJECT:
  case Assignment::A_OS: {
    Setting *setting = ref->get_refd_setting();
    if (!setting || setting->get_st() == Setting::S_ERROR) {
      value->set_valuetype(Value::V_ERROR);
      return self_ref;
    }
    if (setting->get_st() != Setting::S_V) {
      ref->error("This InformationFromObjects construct does not refer to "
                 "a value: `%s'", value->get_fullname().c_str());
      value->set_valuetype(Value::V_ERROR);
      return self_ref;
    }
    governor = dynamic_cast<Value*>(setting)->get_my_governor();
    if (!governor) FATAL_ERROR("Type::chk_this_refd_value()");
    is_const = true;
    break; }
  case Assignment::A_MODULEPAR:
#ifdef MINGW
	break;
#endif
  case Assignment::A_EXT_CONST:
    if (expected_value == EXPECTED_CONSTANT) {
      value->error("Reference to an (evaluable) constant value was "
                   "expected instead of %s",
                   ass->get_description().c_str());
      error_flag = true;
    }
    break;
  case Assignment::A_VAR:
  case Assignment::A_PAR_VAL:
  case Assignment::A_PAR_VAL_IN:
  case Assignment::A_PAR_VAL_OUT:
  case Assignment::A_PAR_VAL_INOUT:
    switch (expected_value) {
    case EXPECTED_CONSTANT:
      value->error("Reference to a constant value was expected instead of "
                   "%s", ass->get_description().c_str());
      error_flag = true;
      break;
    case EXPECTED_STATIC_VALUE:
      value->error("Reference to a static value was expected instead of %s",
                   ass->get_description().c_str());
      error_flag = true;
      break;
    default:
      break;
    }
    break;
  case Assignment::A_MODULEPAR_TEMP:
  case Assignment::A_TEMPLATE:
  case Assignment::A_VAR_TEMPLATE:
  case Assignment::A_PAR_TEMPL_IN:
  case Assignment::A_PAR_TEMPL_OUT:
  case Assignment::A_PAR_TEMPL_INOUT:
    if (expected_value != EXPECTED_TEMPLATE) {
      value->error("Reference to a value was expected instead of %s",
                   ass->get_description().c_str());
      error_flag = true;
    }
    break;
  case Assignment::A_FUNCTION_RVAL:
    chk_runs_on = true;
    // No break
  case Assignment::A_EXT_FUNCTION_RVAL:
    switch (expected_value) {
    case EXPECTED_CONSTANT:
      value->error("Reference to a constant value was expected instead of "
                   "the return value of %s",
                   ass->get_description().c_str());
      error_flag = true;
      break;
    case EXPECTED_STATIC_VALUE:
      value->error("Reference to a static value was expected instead of "
                   "the return value of %s",
                   ass->get_description().c_str());
      error_flag = true;
      break;
    default:
      break;
    }
    break;
  case Assignment::A_FUNCTION_RTEMP:
    chk_runs_on = true;
    // No break
  case Assignment::A_EXT_FUNCTION_RTEMP:
    if (expected_value != EXPECTED_TEMPLATE) {
      value->error("Reference to a value was expected instead of a call of "
                   "%s, which returns a template",
                   ass->get_description().c_str());
      error_flag = true;
    }
    break;
  case Assignment::A_FUNCTION:
  case Assignment::A_EXT_FUNCTION:
    value->error("Reference to a %s was expected instead of a call of %s, "
                 "which does not have return type",
                 expected_value == EXPECTED_TEMPLATE
                 ? "value or template" : "value",
                 ass->get_description().c_str());
    value->set_valuetype(Value::V_ERROR);
    return self_ref;
  default:
    value->error("Reference to a %s was expected instead of %s",
                 expected_value == EXPECTED_TEMPLATE
                 ? "value or template" : "value",
                 ass->get_description().c_str());
    value->set_valuetype(Value::V_ERROR);
    return self_ref;
  } // switch ass->get_asstype()
  if (chk_runs_on)
    ref->get_my_scope()->chk_runs_on_clause(ass, *ref, "call");
  if (!governor)
    governor = ass->get_Type()->get_field_type(ref->get_subrefs(),
                                               expected_value);
  if (!governor) {
    value->set_valuetype(Value::V_ERROR);
    return self_ref;
  }
  TypeCompatInfo info(value->get_my_scope()->get_scope_mod(), this,
                      governor, true, false);
  info.set_str1_elem(IS_STR_ELEM == str_elem);
  if (ref->get_subrefs()) info.set_str2_elem(ref->get_subrefs()->refers_to_string_element());
  TypeChain l_chain;
  TypeChain r_chain;
  if (!is_compatible(governor, &info, value, &l_chain, &r_chain)) {
    // Port or signature values do not exist at all.  These errors are
    // already reported at those definitions.  Extra errors should not be
    // reported here.
    Type *t = get_type_refd_last();
    switch (t->typetype) {
    case T_PORT:
      // Neither port values nor templates exist.
      break;
    case T_SIGNATURE:
      // Only signature templates may exist.
      if (expected_value == EXPECTED_TEMPLATE)
        value->error("Type mismatch: a signature template of type `%s' was "
                     "expected instead of `%s'", get_typename().c_str(),
                     governor->get_typename().c_str());
      break;
    default:
      if (info.is_subtype_error()) {
        value->error("%s", info.get_subtype_error().c_str());
      } else if (!info.is_erroneous()) {
        value->error("Type mismatch: a %s of type `%s' was expected "
                     "instead of `%s'", expected_value == EXPECTED_TEMPLATE
                     ? "value or template" : "value",
                     get_typename().c_str(),
                     governor->get_typename().c_str());
      } else {
        // The semantic error was found by the new code.  It was better to
        // do the assembly inside TypeCompatInfo.
        value->error("%s", info.get_error_str_str().c_str());
      }
      break;
    }
    error_flag = true;
  } else {
    if (info.needs_conversion()) value->set_needs_conversion();
  }

  if (error_flag) {
    value->set_valuetype(Value::V_ERROR);
    return self_ref;
  }
  // Checking for circular references.
  Value *v_last = value->get_value_refd_last(refch, expected_value);
  if (is_const && v_last->get_valuetype() != Value::V_ERROR) {
    // If a referenced universal charstring value points to a literal
    // charstring then drop the reference and create a literal ustring to
    // avoid run-time conversion.
    if (v_last->get_valuetype() == Value::V_CSTR
        && get_typetype_ttcn3(get_type_refd_last()->typetype) == T_USTR)
      value->set_valuetype(Value::V_USTR);
    // Check v_last against the subtype constraints.
    if (sub_type!=NULL) sub_type->chk_this_value(value);
  }

  return self_ref;
}

void Type::chk_this_invoked_value(Value *value, Common::Assignment *,
                                  expected_value_t expected_value)
{
  value->get_value_refd_last(NULL, expected_value);
  if (value->get_valuetype() == Value::V_ERROR) return;
  Type *invoked_t = value->get_invoked_type(expected_value);
  if (invoked_t->get_typetype() == T_ERROR) return;
  invoked_t = invoked_t->get_type_refd_last();
  Type *t = 0;
  switch (invoked_t->get_typetype()) {
  case Type::T_FUNCTION:
    t = invoked_t->get_function_return_type();
    break;
  default:
    FATAL_ERROR("Type::chk_this_invoked_value()");
    break;
  }
  if (!t)
    value->error("The type `%s' has no return type, `%s' expected",
                 invoked_t->get_typename().c_str(), get_typename().c_str());
  else if (!is_compatible(t, NULL, value)) {
    value->error("Incompatible value: `%s' value was expected",
                  get_typename().c_str());
  }
}

void Type::chk_this_value_Null(Value *value)
{
  Value *v=value->get_value_refd_last();
  switch(v->get_valuetype()) {
  case Value::V_NULL:
    break;
  default:
    value->error("NULL value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

void Type::chk_this_value_Bool(Value *value)
{
  Value *v=value->get_value_refd_last();
  switch(v->get_valuetype()) {
  case Value::V_BOOL:
    break;
  default:
    value->error("%s value was expected",
                 value->is_asn1() ? "BOOLEAN" : "boolean");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

void Type::chk_this_value_Int(Value *value)
{
  Value *v=value->get_value_refd_last();
  switch(v->get_valuetype()) {
  case Value::V_INT:
    break;
  default:
    value->error("integer value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
  return;
}

void Type::chk_this_value_Int_A(Value *value)
{
  Value *v=value->get_value_refd_last();
  switch(v->get_valuetype()) {
  case Value::V_INT:
    break;
  case Value::V_NAMEDINT:
    if(!u.namednums.nvs)
      FATAL_ERROR("Type::chk_this_value_Int_A()");
    v->set_valuetype
      (Value::V_INT, u.namednums.nvs->
        get_nv_byName(*v->get_val_id())->get_value()->get_val_Int()->get_val());
    break;
  default:
    value->error("INTEGER value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

void Type::chk_this_value_Real(Value *value)
{
  Value *v = value->get_value_refd_last();
  if (value->is_asn1()) {
    if (value->is_ref()) {
      Type *t = v->get_my_governor()->get_type_refd_last();
      if (t->get_typetype() != T_REAL) {
        value->error("REAL value was expected");
        value->set_valuetype(Value::V_ERROR);
        return;
      }
    }
    switch (v->get_valuetype()) {
    case Value::V_REAL:
      break;
    case Value::V_UNDEF_BLOCK:{
      v->set_valuetype(Value::V_SEQ);
      Identifier t_id(Identifier::ID_ASN, string("REAL"));
      bool self_ref = get_my_scope()->get_scope_asss()->get_local_ass_byId(t_id)
        ->get_Type()->chk_this_value(v, 0, EXPECTED_CONSTANT, INCOMPLETE_NOT_ALLOWED,
          OMIT_NOT_ALLOWED, SUB_CHK);
      (void)self_ref;
      v->set_valuetype(Value::V_REAL);
      break;}
    case Value::V_INT:
      v->set_valuetype(Value::V_REAL);
      break;
    default:
      value->error("REAL value was expected");
      value->set_valuetype(Value::V_ERROR);
      break;
    }
  } else {
    switch(v->get_valuetype()) {
    case Value::V_REAL:
      break;
    default:
      value->error("float value was expected");
      value->set_valuetype(Value::V_ERROR);
      break;
    }
  }
}

void Type::chk_this_value_Enum(Value *value)
{
  switch(value->get_valuetype()) {
  case Value::V_ENUM:
    break;
  default:
    value->error("%s value was expected",
                 value->is_asn1() ? "ENUMERATED" : "enumerated");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

void Type::chk_this_value_BStr(Value *value)
{
  Value *v = value->get_value_refd_last();
  switch(v->get_valuetype()) {
  case Value::V_BSTR:
    break;
  default:
    value->error("bitstring value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

void Type::chk_this_value_namedbits(Value *value)
{
  if (!value) FATAL_ERROR("Type::chk_this_value_namedbits()");
  value->set_valuetype(Value::V_NAMEDBITS);
  if(!u.namednums.nvs) {
    value->error("No named bits are defined in type `%s'",
                 get_typename().c_str());
    value->set_valuetype(Value::V_ERROR);
    return;
  }
  string *bstring = new string("");
  for(size_t i=0; i < value->get_nof_ids(); i++) {
    Identifier *id = value->get_id_byIndex(i);
    if(!u.namednums.nvs->has_nv_withName(*id)) {
      delete bstring;
      error("No named bit with name `%s' is defined in type `%s'",
            id->get_dispname().c_str(), get_typename().c_str());
      value->set_valuetype(Value::V_ERROR);
      return;
    }
    size_t bitnum = static_cast<size_t>
      (u.namednums.nvs->get_nv_byName(*id)->get_value()->get_val_Int()
        ->get_val());
    if(bstring->size() < bitnum + 1) bstring->resize(bitnum + 1, '0');
    (*bstring)[bitnum] = '1';
  }
  value->set_valuetype(Value::V_BSTR, bstring);
}

void Type::chk_this_value_BStr_A(Value *value)
{
  Value *v = value->get_value_refd_last();
  if (value->is_asn1()) {
    if(value->is_ref()) {
      Type *t = v->get_my_governor()->get_type_refd_last();
      if(t->get_typetype()!=T_BSTR_A && t->get_typetype()!=T_BSTR) {
        value->error("(reference to) BIT STRING value was expected");
        value->set_valuetype(Value::V_ERROR);
        return;
      }
    }
    switch(v->get_valuetype()) {
    case Value::V_BSTR:
      break;
    case Value::V_HSTR:
      if (v != value) FATAL_ERROR("Common::Type::chk_this_value_BStr_A()");
      v->set_valuetype(Value::V_BSTR);
      break;
    case Value::V_UNDEF_BLOCK:
      if (v != value) FATAL_ERROR("Type::chk_this_value_BStr_A()");
      chk_this_value_namedbits(v);
      break;
    default:
      value->error("BIT STRING value was expected");
      value->set_valuetype(Value::V_ERROR);
      return;
    }
  } else {
    switch(v->get_valuetype()) {
    case Value::V_BSTR:
      break;
    default:
      value->error("bitstring value was expected");
      value->set_valuetype(Value::V_ERROR);
      break;
    }
  }
}

void Type::chk_this_value_HStr(Value *value)
{
  Value *v = value->get_value_refd_last();
  switch (v->get_valuetype()) {
  case Value::V_HSTR:
    break;
  default:
    value->error("hexstring value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

void Type::chk_this_value_OStr(Value *value)
{
  Value *v = value->get_value_refd_last();
  if (value->is_asn1()) {
    if (value->is_ref()) {
      Type *t = v->get_my_governor()->get_type_refd_last();
      if(t->get_typetype() != T_OSTR) {
        value->error("(reference to) OCTET STRING value was expected");
        value->set_valuetype(Value::V_ERROR);
        return;
      }
    }
    switch(v->get_valuetype()) {
    case Value::V_OSTR:
      break;
    case Value::V_BSTR:
    case Value::V_HSTR:
      if (v != value) FATAL_ERROR("Type::chk_this_value_OStr()");
      v->set_valuetype(Value::V_OSTR);
      break;
    default:
      value->error("OCTET STRING value was expected");
      value->set_valuetype(Value::V_ERROR);
      break;
    }
  } else {
    switch(v->get_valuetype()) {
    case Value::V_OSTR:
      break;
    default:
      value->error("octetstring value was expected");
      value->set_valuetype(Value::V_ERROR);
      break;
    }
  }
}

/** \todo enhance */
void Type::chk_this_value_CStr(Value *value)
{

  switch(value->get_valuetype()) {
  case Value::V_UNDEF_BLOCK:
    value->set_valuetype(Value::V_CHARSYMS);
    if (value->get_valuetype() == Value::V_ERROR) return;
    // no break
  case Value::V_CHARSYMS:
    switch (typetype) {
    case T_USTR:
    case T_UTF8STRING:
    case T_BMPSTRING:
    case T_UNIVERSALSTRING:
      value->set_valuetype(Value::V_USTR);
      break;
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
    case T_GRAPHICSTRING:
    case T_OBJECTDESCRIPTOR:
    case T_GENERALSTRING:
      value->set_valuetype(Value::V_ISO2022STR);
      break;
    case T_CSTR:
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_IA5STRING:
    case T_VISIBLESTRING:
    case T_UTCTIME:
    case T_GENERALIZEDTIME:
      value->set_valuetype(Value::V_CSTR);
      break;
    default:
      FATAL_ERROR("chk_this_value_CStr()");
    } // switch
    break;
  case Value::V_USTR:
    switch (typetype) {
    case T_USTR:
    case T_UTF8STRING:
    case T_BMPSTRING:
    case T_UNIVERSALSTRING:
      break;
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
    case T_GRAPHICSTRING:
    case T_OBJECTDESCRIPTOR:
    case T_GENERALSTRING:
      // accept it
      /*
      if(value->get_val_ustr().is_cstr()) {
        value->warning("ISO-2022 string value was expected (converting"
                       " simple ISO-10646 string).");
        value->set_valuetype(Value::V_CSTR);
      }
      else {
        value->error("ISO-2022 string value was expected (instead of"
                     " ISO-10646 string).");
        value->set_valuetype(Value::V_ERROR);
      }
      */
      break;
    case T_CSTR:
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_IA5STRING:
    case T_VISIBLESTRING:
    case T_UTCTIME:
    case T_GENERALIZEDTIME:
      value->set_valuetype(Value::V_CSTR);
      break;
    default:
      FATAL_ERROR("Type::chk_this_value_CStr()");
    } // switch
    break;
  case Value::V_CSTR:
    switch (typetype) {
    case T_USTR:
    case T_UTF8STRING:
    case T_BMPSTRING:
    case T_UNIVERSALSTRING:
      value->set_valuetype(Value::V_USTR);
      break;
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
    case T_GRAPHICSTRING:
    case T_OBJECTDESCRIPTOR:
    case T_GENERALSTRING:
      value->set_valuetype(Value::V_ISO2022STR);
      break;
    case T_CSTR:
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_IA5STRING:
    case T_VISIBLESTRING:
    case T_UTCTIME:
    case T_GENERALIZEDTIME:
      break;
    default:
      FATAL_ERROR("chk_this_value_CStr()");
    } // switch
    break;
  case Value::V_ISO2022STR:
    switch (typetype) {
    case T_USTR:
    case T_UTF8STRING:
    case T_BMPSTRING:
    case T_UNIVERSALSTRING:
      value->error("ISO-10646 string value was expected (instead of"
                   " ISO-2022 string).");
      value->set_valuetype(Value::V_ERROR);
      break;
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
    case T_GRAPHICSTRING:
    case T_OBJECTDESCRIPTOR:
    case T_GENERALSTRING:
      break;
    case T_CSTR:
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_IA5STRING:
    case T_VISIBLESTRING:
    case T_UTCTIME:
    case T_GENERALIZEDTIME:
      value->set_valuetype(Value::V_ISO2022STR);
      break;
    default:
      FATAL_ERROR("chk_this_value_CStr()");
    } // switch
    break;
  default:
    value->error("character string value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

void Type::chk_this_value_OID(Value *value)
{
  Value *v = value->get_value_refd_last();
  if (value->is_asn1()) {
    if (value->is_ref()) {
      Type *t = v->get_my_governor()->get_type_refd_last();
      if (t->get_typetype() != T_OID) {
        value->error("(reference to) OBJECT IDENTIFIER value was "
                     "expected");
        value->set_valuetype(Value::V_ERROR);
        return;
      }
    }
    switch (v->get_valuetype()) {
    case Value::V_OID:
      break;
    case Value::V_UNDEF_BLOCK:
      if (v != value) FATAL_ERROR("Type::chk_this_value_OID()");
      v->set_valuetype(Value::V_OID);
      v->chk();
      break;
    default:
      value->error("OBJECT IDENTIFIER value was expected");
      value->set_valuetype(Value::V_ERROR);
      break;
    }
  } else {
    switch (v->get_valuetype()) {
    case Value::V_OID:
      v->chk();
      break;
    default:
      value->error("objid value was expected");
      value->set_valuetype(Value::V_ERROR);
      break;
    }
  }
}

void Type::chk_this_value_ROID(Value *value)
{
  Value *v = value->get_value_refd_last();
  if(value->is_ref()) {
    Type *t = v->get_my_governor()->get_type_refd_last();
    if (t->get_typetype() != T_ROID) {
      value->error("(reference to) RELATIVE-OID value was expected");
      value->set_valuetype(Value::V_ERROR);
      return;
    }
  }
  switch(v->get_valuetype()) {
  case Value::V_ROID:
    break;
  case Value::V_UNDEF_BLOCK:
    if (v != value) FATAL_ERROR("Type::chk_this_value_ROID()");
    v->set_valuetype(Value::V_ROID);
    v->chk();
    break;
  default:
    value->error("RELATIVE-OID value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

void Type::chk_this_value_Any(Value *value)
{
  Value *v = value->get_value_refd_last();
  if (value->is_asn1()) {
    if (value->is_ref()) {
      Type *t = v->get_my_governor()->get_type_refd_last();
      if(t->get_typetype()!=T_ANY && t->get_typetype()!=T_OSTR) {
        value->error("(reference to) OCTET STRING or ANY type value"
                     " was expected");
        value->set_valuetype(Value::V_ERROR);
        return;
      }
    }
    switch(v->get_valuetype()) {
    case Value::V_OSTR:
      break;
    case Value::V_HSTR:
      if (v!=value) FATAL_ERROR("Type::chk_this_value_Any()");
      v->set_valuetype(Value::V_OSTR);
      break;
    default:
      value->error("ANY (OCTET STRING) value was expected");
      value->set_valuetype(Value::V_ERROR);
      break;
    }
  }
  else { // is ttcn
    switch(v->get_valuetype()) {
    case Value::V_OSTR:
      break;
    default:
      value->error("octetstring value was expected for ASN ANY type");
      value->set_valuetype(Value::V_ERROR);
      break;
    }
  }
}

/** Return a string containing the names of existing fields (components).
 *
 * @param t a Type object
 * @return a string (l-value, temporary)
 * @pre t must be a sequence, choice, set, anytype, open type
 */
static string actual_fields(Type& t) // alas, not even get_nof_comps() is const
{
  static const string has_no(" has no ");
  size_t ncomps = t.get_nof_comps();
  const string fields_or_comps(t.is_asn1() ? "components" : "fields");
  if (ncomps) {
    string msg("Valid ");
    msg += fields_or_comps;
    msg += ": ";
    msg += t.get_comp_byIndex(0)->get_name().get_dispname();
    for (size_t ci=1; ci < ncomps; ++ci) {
      msg += ',';
      msg += t.get_comp_byIndex(ci)->get_name().get_dispname();
    }
    return msg;
  }
  else return t.get_fullname() + has_no + fields_or_comps;
}


bool Type::chk_this_value_Choice(Value *value, Common::Assignment *lhs,
  expected_value_t expected_value, namedbool incomplete_allowed, namedbool implicit_omit)
{
  bool self_ref = false;
  switch(value->get_valuetype()) {
  case Value::V_SEQ:
    if (value->is_asn1()) {
      value->error("CHOICE value was expected for type `%s'",
                   get_fullname().c_str());
      value->set_valuetype(Value::V_ERROR);
      break;
    } else {
      // the valuetype can be ERROR if the value has no fields at all
      if (value->get_valuetype() == Value::V_ERROR) return false;
      // The value notation for TTCN record/union types
      // cannot be distinguished during parsing.  Now we know it's a union.
      value->set_valuetype(Value::V_CHOICE);
    }
    // no break
  case Value::V_CHOICE: {
    if (!value->is_asn1() && typetype == T_OPENTYPE) {
      // allow the alternatives of open types as both lower and upper identifiers
      value->set_alt_name_to_lowercase();
    }
    const Identifier& alt_name = value->get_alt_name();
    if(!has_comp_withName(alt_name)) {
      if (value->is_asn1()) {
        value->error("Reference to non-existent alternative `%s' in CHOICE"
                     " value for type `%s'",
                     alt_name.get_dispname().c_str(),
                     get_fullname().c_str());
      } else {
        value->error("Reference to non-existent field `%s' in union "
                     "value for type `%s'",
                     alt_name.get_dispname().c_str(),
                     get_fullname().c_str());
      }
      value->set_valuetype(Value::V_ERROR);
      value->note("%s", actual_fields(*this).c_str());
      return self_ref;
    }
    Type *alt_type = get_comp_byName(alt_name)->get_type();
    Value *alt_value = value->get_alt_value();
    Error_Context cntxt(value, "In value for %s `%s'",
                        value->is_asn1() ? "alternative" : "union field",
                        alt_name.get_dispname().c_str());
    alt_value->set_my_governor(alt_type);
    alt_type->chk_this_value_ref(alt_value);
    self_ref |= alt_type->chk_this_value(alt_value, lhs, expected_value,
      incomplete_allowed, OMIT_NOT_ALLOWED, SUB_CHK, implicit_omit);
    if (alt_value->get_valuetype() == Value::V_NOTUSED) {
      value->set_valuetype(Value::V_NOTUSED);
    }
    break;}
  default:
    value->error("%s value was expected for type `%s'",
                 value->is_asn1() ? "CHOICE" : "union",
                 get_fullname().c_str());
    value->set_valuetype(Value::V_ERROR);
    break;
  }
  return self_ref;
}

bool Type::chk_this_value_Se(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
  namedbool incomplete_allowed, namedbool implicit_omit)
{
  if (value->is_asn1())
    return chk_this_value_Se_A(value, lhs, expected_value, implicit_omit);
  else
    return chk_this_value_Se_T(value, lhs, expected_value, incomplete_allowed,
                                                               implicit_omit);
}

bool Type::chk_this_value_Se_T(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
                               namedbool incomplete_allowed, namedbool implicit_omit)
{
  switch (value->get_valuetype()) {
  case Value::V_SEQ:
    if (typetype == T_SET_A || typetype == T_SET_T) {
      value->set_valuetype(Value::V_SET);
      return chk_this_value_Set_T(value, lhs, expected_value, incomplete_allowed,
                           implicit_omit);
    } else {
      return chk_this_value_Seq_T(value, lhs, expected_value, incomplete_allowed,
                           implicit_omit);
    }
    break;
  case Value::V_SEQOF:
    if (typetype == T_SET_A || typetype == T_SET_T) {
      if (value->get_nof_comps() == 0) {
        if (get_nof_comps() == 0) {
          value->set_valuetype(Value::V_SET);
        } else {
          value->error("A non-empty set value was expected for type `%s'",
                       get_fullname().c_str());
          value->set_valuetype(Value::V_ERROR);
        }
      } else {
        // This will catch the indexed assignment notation as well.
        value->error("Value list notation cannot be used for "
                     "%s type `%s'", typetype == T_SET_A ? "SET" : "set",
                     get_fullname().c_str());
        value->set_valuetype(Value::V_ERROR);
      }
    } else {
      if (value->is_indexed()) {
        value->error("Indexed assignment notation cannot be used for %s "
                     "type `%s'", typetype == T_SEQ_A
                     ? "SEQUENCE" : "record",
                     get_fullname().c_str());
        value->set_valuetype(Value::V_ERROR);
      } else {
        value->set_valuetype(Value::V_SEQ);
        return chk_this_value_Seq_T(value, lhs, expected_value, incomplete_allowed,
                             implicit_omit);
      }
    }
    break;
  default:
    value->error("%s value was expected for type `%s'",
                 typetype == T_SEQ_T ? "Record" : "Set",
                 get_fullname().c_str());
    value->set_valuetype(Value::V_ERROR);
    break;
  }
  return false;
}

bool Type::chk_this_value_Seq_T(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
  namedbool incomplete_allowed, namedbool implicit_omit)
{
  bool self_ref = false;
  map<string, NamedValue> comp_map;
  // it is set to false if we have lost the ordering
  bool in_synch = true;
  const size_t n_type_comps = get_nof_comps();
  size_t n_value_comps = value->get_nof_comps();
  // for incomplete values
  CompField *last_cf = 0;
  size_t next_index = 0;
  size_t seq_index = 0;
  bool is_empty = n_type_comps > 0; // don't do this check if the record type has no fields
  for (size_t v_i = 0; v_i < n_value_comps; v_i++, seq_index++) {
    NamedValue *nv = value->get_se_comp_byIndex(v_i);
    const Identifier& value_id = nv->get_name();
    const string& value_name = value_id.get_name();
    const char *value_dispname_str = value_id.get_dispname().c_str();
    if (!has_comp_withName(value_id)) {
      nv->error("Reference to non-existent field `%s' in record value for "
        "type `%s'", value_dispname_str, get_typename().c_str());
      nv->note("%s", actual_fields(*this).c_str());
      in_synch = false;
      continue;
    } else if (comp_map.has_key(value_name)) {
      nv->error("Duplicate record field `%s'", value_dispname_str);
      comp_map[value_name]->note("Field `%s' is already given here",
        value_dispname_str);
      in_synch = false;
    } else comp_map.add(value_name, nv);
    CompField *cf = get_comp_byName(value_id);
    // check the ordering of fields
    if (in_synch) {
      if (INCOMPLETE_NOT_ALLOWED != incomplete_allowed) {
        bool found = false;
        for (size_t i = next_index; i < n_type_comps; i++) {
          CompField *cf2 = get_comp_byIndex(i);
          if (value_name == cf2->get_name().get_name()) {
            last_cf = cf2;
            next_index = i + 1;
            found = true;
            break;
          }
        }
        if (!found) {
          nv->error("Field `%s' cannot appear after field `%s' in record "
            "value", value_dispname_str,
            last_cf->get_name().get_dispname().c_str());
          in_synch = false;
        }
      } else {
        // the value must be complete unless implicit omit is set
        CompField *cf2 = get_comp_byIndex(seq_index);
        CompField *cf2_orig = cf2;
        while (IMPLICIT_OMIT == implicit_omit && seq_index < n_type_comps && cf2 != cf &&
          cf2->get_is_optional())
          cf2 = get_comp_byIndex(++seq_index);
        if (seq_index >= n_type_comps || cf2 != cf) {
          nv->error("Unexpected field `%s' in record value, "
            "expecting `%s'", value_dispname_str,
            cf2_orig->get_name().get_dispname().c_str());
          in_synch = false;
        }
      }
    }
    Type *type = cf->get_type();
    Value *comp_value = nv->get_value();
    comp_value->set_my_governor(type);
    if (comp_value->get_valuetype() == Value::V_NOTUSED) {
      if (IMPLICIT_OMIT == implicit_omit) {
        comp_value->set_valuetype(Value::V_OMIT);
      } else {
        continue;
      }
    }
    Error_Context cntxt(nv, "In value for record field `%s'",
      value_dispname_str);
    type->chk_this_value_ref(comp_value);
    self_ref |= type->chk_this_value(comp_value, lhs, expected_value, incomplete_allowed,
      cf->get_is_optional() ? OMIT_ALLOWED : OMIT_NOT_ALLOWED, SUB_CHK, implicit_omit);
    if (comp_value->get_valuetype() != Value::V_NOTUSED) {
      is_empty = false;
    }
  }
  if (INCOMPLETE_ALLOWED != incomplete_allowed || IMPLICIT_OMIT == implicit_omit) {
    for (size_t i = 0; i < n_type_comps; i++) {
      const Identifier& id = get_comp_byIndex(i)->get_name();
      if (!comp_map.has_key(id.get_name())) {
        if (get_comp_byIndex(i)->get_is_optional() && IMPLICIT_OMIT == implicit_omit) {
          value->add_se_comp(new NamedValue(new Identifier(id),
            new Value(Value::V_OMIT)));
          is_empty = false;
        }
        else if (INCOMPLETE_NOT_ALLOWED == incomplete_allowed)
          value->error("Field `%s' is missing from record value",
                     id.get_dispname().c_str());
        else if (WARNING_FOR_INCOMPLETE == incomplete_allowed ) {
          value->warning("Field `%s' is missing from record value",
            id.get_dispname().c_str());
        }
      }
    }
  }
  if (is_empty) {
    // all of the record's fields are unused (-), set the record to unused
    // to avoid unnecessary code generation
    value->set_valuetype(Value::V_NOTUSED);
  }
  comp_map.clear();
  return self_ref;
}

bool Type::chk_this_value_Set_T(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
  namedbool incomplete_allowed, namedbool implicit_omit)
{
  bool self_ref = false;
  map<string, NamedValue> comp_map;
  size_t n_type_comps = get_nof_comps();
  size_t n_value_comps = value->get_nof_comps();
  bool is_empty = n_type_comps > 0; // don't do this check if the set type has no fields
  for(size_t v_i = 0; v_i < n_value_comps; v_i++) {
    NamedValue *nv = value->get_se_comp_byIndex(v_i);
    const Identifier& value_id = nv->get_name();
    const string& value_name = value_id.get_name();
    const char *value_dispname_str = value_id.get_dispname().c_str();
    if (!has_comp_withName(value_id)) {
      nv->error("Reference to non-existent field `%s' in set value for "
        "type `%s'", value_dispname_str, get_typename().c_str());
      nv->note("%s", actual_fields(*this).c_str());
      continue;
    } else if (comp_map.has_key(value_name)) {
      nv->error("Duplicate set field `%s'", value_dispname_str);
      comp_map[value_name]->note("Field `%s' is already given here",
        value_dispname_str);
    } else comp_map.add(value_name, nv);
    CompField *cf = get_comp_byName(value_id);
    Type *type = cf->get_type();
    Value *comp_value = nv->get_value();
    comp_value->set_my_governor(type);
    if (comp_value->get_valuetype() == Value::V_NOTUSED) {
      if (IMPLICIT_OMIT == implicit_omit) {
        comp_value->set_valuetype(Value::V_OMIT);
      } else {
        continue;
      }
    }
    Error_Context cntxt(nv, "In value for set field `%s'",
      value_dispname_str);
    type->chk_this_value_ref(comp_value);
    self_ref |= type->chk_this_value(comp_value, lhs, expected_value, incomplete_allowed,
      cf->get_is_optional() ? OMIT_ALLOWED : OMIT_NOT_ALLOWED, SUB_CHK, implicit_omit);
    if (comp_value->get_valuetype() != Value::V_NOTUSED) {
      is_empty = false;
    }
  }
  if (INCOMPLETE_ALLOWED != incomplete_allowed || IMPLICIT_OMIT == implicit_omit) {
    for (size_t i = 0; i < n_type_comps; i++) {
      const Identifier& id = get_comp_byIndex(i)->get_name();
      if(!comp_map.has_key(id.get_name())) {
        if (get_comp_byIndex(i)->get_is_optional() && IMPLICIT_OMIT == implicit_omit) {
          value->add_se_comp(new NamedValue(new Identifier(id),
            new Value(Value::V_OMIT)));
          is_empty = false;
        }
        else if (INCOMPLETE_NOT_ALLOWED == incomplete_allowed)
          value->error("Field `%s' is missing from set value",
            id.get_dispname().c_str());
        else if (WARNING_FOR_INCOMPLETE == incomplete_allowed ) {
          value->warning("Field `%s' is missing from set value",
            id.get_dispname().c_str());
        }
      }
    }
  }
  if (is_empty) {
    // all of the set's fields are unused (-), set the set to unused to avoid
    // unnecessary code generation
    value->set_valuetype(Value::V_NOTUSED);
  }
  comp_map.clear();
  return self_ref;
}

bool Type::chk_this_value_Se_A(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
  namedbool implicit_omit)
{
  if(value->get_valuetype()==Value::V_UNDEF_BLOCK) {
    if(typetype == T_SEQ_A) value->set_valuetype(Value::V_SEQ);
    else value->set_valuetype(Value::V_SET);
  } // if V_UNDEF
  switch(value->get_valuetype()) {
  case Value::V_SEQ:
    return chk_this_value_Seq_A(value, lhs, expected_value, implicit_omit);
  case Value::V_SET:
    return chk_this_value_Set_A(value, lhs, expected_value, implicit_omit);
  default:
    value->error("%s value was expected",
                 typetype == T_SEQ_A ? "SEQUENCE" : "SET");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
  return false;
}

bool Type::chk_this_value_Seq_A(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
  namedbool implicit_omit)
{
  bool self_ref = false;
  map<string, NamedValue> comp_map;
  // it is set to false if we have lost the ordering
  bool in_synch = true;
  size_t n_type_comps = u.secho.ctss->get_nof_comps();
  size_t n_value_comps = value->get_nof_comps();
  size_t t_i = 0;
  for (size_t v_i = 0; v_i < n_value_comps; v_i++) {
    NamedValue *nv = value->get_se_comp_byIndex(v_i);
    const Identifier& value_id = nv->get_name();
    const string& value_name = value_id.get_name();
    const char *value_dispname_str = value_id.get_dispname().c_str();
    if (!u.secho.ctss->has_comp_withName(value_id)) {
      nv->error("Reference to non-existent component `%s' of SEQUENCE "
                "type `%s'", value_dispname_str, get_typename().c_str());
      nv->note("%s", actual_fields(*this).c_str());
      in_synch = false;
      continue;
    } else if (comp_map.has_key(value_name)) {
      nv->error("Duplicate SEQUENCE component `%s'", value_dispname_str);
      comp_map[value_name]->note("Component `%s' is already given here",
        value_dispname_str);
      in_synch = false;
    } else comp_map.add(value_name, nv);
    CompField *cf = u.secho.ctss->get_comp_byName(value_id);
    if (in_synch) {
      CompField *cf2 = 0;
      for ( ; t_i < n_type_comps; t_i++) {
        cf2 = u.secho.ctss->get_comp_byIndex(t_i);
        if (cf2 == cf || (!cf2->get_is_optional() && !cf2->has_default() &&
          IMPLICIT_OMIT != implicit_omit))
          break;
      }
      if (cf2 != cf) {
        if (t_i < n_type_comps) {
          nv->error("Unexpected component `%s' in SEQUENCE value, "
            "expecting `%s'", value_dispname_str,
            cf2->get_name().get_dispname().c_str());
        } else {
          nv->error("Unexpected component `%s' in SEQUENCE value",
            value_dispname_str);
        }
        in_synch = false;
      } else t_i++;
    }
    Type *type = cf->get_type();
    Error_Context cntxt(nv, "In value for SEQUENCE component `%s'",
      value_dispname_str);
    Value *comp_value = nv->get_value();
    comp_value->set_my_governor(type);
    type->chk_this_value_ref(comp_value);
    self_ref |= type->chk_this_value(comp_value, lhs, expected_value,
      INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK, implicit_omit);
  }
  for (size_t i = 0; i < n_type_comps; i++) {
    CompField *cf = u.secho.ctss->get_comp_byIndex(i);
    const Identifier& id = cf->get_name();
    if (!comp_map.has_key(id.get_name())) {
      if (!cf->get_is_optional() && !cf->has_default())
      value->error("Mandatory component `%s' is missing from SEQUENCE value",
                   id.get_dispname().c_str());
      else if (cf->get_is_optional() && IMPLICIT_OMIT == implicit_omit)
        value->add_se_comp(new NamedValue(new Identifier(id),
          new Value(Value::V_OMIT)));
    }
  }
  comp_map.clear();
  return self_ref;
}

bool Type::chk_this_value_Set_A(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
  namedbool implicit_omit)
{
  bool self_ref = false;
  map<string, NamedValue> comp_map;
  size_t n_type_comps = u.secho.ctss->get_nof_comps();
  size_t n_value_comps = value->get_nof_comps();
  for(size_t v_i = 0; v_i < n_value_comps; v_i++) {
    NamedValue *nv = value->get_se_comp_byIndex(v_i);
    const Identifier& value_id = nv->get_name();
    const string& value_name = value_id.get_name();
    const char *value_dispname_str = value_id.get_dispname().c_str();
    if (!u.secho.ctss->has_comp_withName(value_id)) {
      value->error("Reference to non-existent component `%s' of SET type "
        "`%s'", value_dispname_str, get_typename().c_str());
      nv->note("%s", actual_fields(*this).c_str());
      continue;
    } else if (comp_map.has_key(value_name)) {
      nv->error("Duplicate SET component `%s'", value_dispname_str);
      comp_map[value_name]->note("Component `%s' is already given here",
        value_dispname_str);
    } else comp_map.add(value_name, nv);
    Type *type =
      u.secho.ctss->get_comp_byName(value_id)->get_type();
    Value *comp_value = nv->get_value();
    Error_Context cntxt(nv, "In value for SET component `%s'",
      value_dispname_str);
    comp_value->set_my_governor(type);
    type->chk_this_value_ref(comp_value);
    self_ref |= type->chk_this_value(comp_value, lhs, expected_value,
      INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK, implicit_omit);
  }
  for (size_t i = 0; i < n_type_comps; i++) {
    CompField *cf = u.secho.ctss->get_comp_byIndex(i);
    const Identifier& id = cf->get_name();
    if (!comp_map.has_key(id.get_name())) {
      if (!cf->get_is_optional() && !cf->has_default())
      value->error("Mandatory component `%s' is missing from SET value",
                   id.get_dispname().c_str());
      else if (cf->get_is_optional() && IMPLICIT_OMIT == implicit_omit)
        value->add_se_comp(new NamedValue(new Identifier(id),
          new Value(Value::V_OMIT)));
  }
  }
  comp_map.clear();
  return self_ref;
}

bool Type::chk_this_value_SeOf(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
  namedbool incomplete_allowed, namedbool implicit_omit)
{
  bool self_ref = false;
  const char *valuetypename;
  if (value->is_asn1()) {
    if (typetype == T_SEQOF) valuetypename = "SEQUENCE OF";
    else valuetypename = "SET OF";
  } else {
    if (typetype == T_SEQOF) valuetypename = "record of";
    else valuetypename = "set of";
  }
  if (value->get_valuetype() == Value::V_UNDEF_BLOCK) {
    if (typetype == T_SEQOF) value->set_valuetype(Value::V_SEQOF);
    else value->set_valuetype(Value::V_SETOF);
  } // if V_UNDEF
  switch (value->get_valuetype()) {
  case Value::V_SEQOF:
    if (typetype == T_SETOF) value->set_valuetype(Value::V_SETOF);
    // no break
  case Value::V_SETOF: {
    if (!value->is_indexed()) {
      for (size_t i = 0; i < value->get_nof_comps(); i++) {
        Value *v_comp = value->get_comp_byIndex(i);
        Error_Context cntxt(v_comp, "In component #%lu",
                            (unsigned long)(i + 1));
        v_comp->set_my_governor(u.seof.ofType);
        if (v_comp->get_valuetype() == Value::V_NOTUSED) {
          if (INCOMPLETE_NOT_ALLOWED == incomplete_allowed)
            v_comp->error("Not used symbol `-' is not allowed in this "
                          "context");
          else if (incomplete_allowed == WARNING_FOR_INCOMPLETE) {
            v_comp->warning("Not used symbol `-' in %s value", valuetypename);
          }
        } else {
          u.seof.ofType->chk_this_value_ref(v_comp);
          self_ref |= u.seof.ofType->chk_this_value(v_comp, lhs, expected_value,
            incomplete_allowed, OMIT_NOT_ALLOWED, SUB_CHK, implicit_omit);
        }
      }
    } else {
      // Only constant values with constant indicies can be checked at
      // compile time.  Constant expressions evaluated.
      bool check_holes = expected_value == EXPECTED_CONSTANT;
      Int max_index = -1;
      map<Int, Int> index_map;
      for (size_t i = 0; i < value->get_nof_comps(); i++) {
        Error_Context cntxt(this, "In component #%lu",
                            (unsigned long)(i + 1));
        Value *val = value->get_comp_byIndex(i);
        Value *index = value->get_index_byIndex(i);
        index->chk_expr_int(expected_value);
        if (index->get_valuetype() != Value::V_ERROR &&
            index->get_value_refd_last()->get_valuetype() == Value::V_INT) {
          const int_val_t *index_int = index->get_value_refd_last()
            ->get_val_Int();
          if (*index_int > INT_MAX) {
            index->error("An integer value less than `%d' was expected for "
                         "indexing type `%s' instead of `%s'", INT_MAX,
                         get_typename().c_str(),
                         (index_int->t_str()).c_str());
            index->set_valuetype(Value::V_ERROR);
            check_holes = false;
          } else {
            Int index_val = index_int->get_val();
            if (index_val < 0) {
              index->error("A non-negative integer value was expected for "
                           "indexing type `%s' instead of `%s'",
                           get_typename().c_str(),
                           Int2string(index_val).c_str());
              index->set_valuetype(Value::V_ERROR);
              check_holes = false;
            } else if (index_map.has_key(index_val)) {
              index->error("Duplicate index value `%s' for components `%s' "
                           "and `%s'", Int2string(index_val).c_str(),
                           Int2string((Int)i + 1).c_str(),
                           Int2string(*index_map[index_val]).c_str());
              index->set_valuetype(Value::V_ERROR);
              check_holes = false;
            } else {
              index_map.add(index_val, new Int((Int)i + 1));
              if (max_index < index_val)
                max_index = index_val;
            }
          }
        } else {
          // The index cannot be determined.
          check_holes = false;
        }
        val->set_my_governor(u.seof.ofType);
        u.seof.ofType->chk_this_value_ref(val);
        self_ref |= u.seof.ofType->chk_this_value(val, lhs, expected_value,
          incomplete_allowed, OMIT_NOT_ALLOWED, SUB_CHK);
      }
      if (check_holes) {
        if ((size_t)max_index != index_map.size() - 1) {
          value->error("It's not allowed to create hole(s) in constant "
                       "values");
          value->set_valuetype(Value::V_ERROR);
        }
      }
      for (size_t i = 0; i < index_map.size(); i++)
        delete index_map.get_nth_elem(i);
      index_map.clear();
    }
    break; }
  default:
    value->error("%s value was expected", valuetypename);
    value->set_valuetype(Value::V_ERROR);
    break;
  }
  return self_ref;
}

void Type::chk_this_value_Verdict(Value *value)
{
  Value *v = value->get_value_refd_last();
  switch (v->get_valuetype()) {
  case Value::V_VERDICT:
    break;
  default:
    value->error("verdict value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

/** \todo enhance to handle activate */
void Type::chk_this_value_Default(Value *value)
{
  switch (value->get_valuetype()) {
  case Value::V_TTCN3_NULL:
    value->set_valuetype(Value::V_DEFAULT_NULL);
    break;
  default:
    value->error("default value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

bool Type::chk_this_value_Array(Value *value, Common::Assignment *lhs, expected_value_t expected_value,
  namedbool incomplete_allowed, namedbool implicit_omit)
{
  bool self_ref = false;
  switch (value->get_valuetype()) {
  case Value::V_SEQOF:
    value->set_valuetype(Value::V_ARRAY);
    // no break
  case Value::V_ARRAY: {
    size_t n_value_comps = value->get_nof_comps();
    Ttcn::ArrayDimension *dim = u.array.dimension;
    if (!value->is_indexed()) {
      // Value-list notation.
      if (!dim->get_has_error()) {
        size_t array_size = dim->get_size();
        if (array_size != n_value_comps)
          value->error("Too %s elements in the array value: %lu was "
                       "expected instead of %lu",
                       array_size > n_value_comps ? "few" : "many",
                       (unsigned long)array_size,
                       (unsigned long)n_value_comps);
      }
      for (size_t i = 0; i < n_value_comps; i++) {
        Error_Context cntxt(this, "In array element %lu",
                            (unsigned long)(i + 1));
        Value *v_comp = value->get_comp_byIndex(i);
        v_comp->set_my_governor(u.array.element_type);
        if (v_comp->get_valuetype() == Value::V_NOTUSED) {
          if (INCOMPLETE_NOT_ALLOWED == incomplete_allowed)
            v_comp->error("Not used symbol `-' is not allowed in this "
                          "context");
          else if (incomplete_allowed == WARNING_FOR_INCOMPLETE) {
            v_comp->warning("Not used symbol `-' in array value");
          }
        } else {
          u.array.element_type->chk_this_value_ref(v_comp);
          self_ref |= u.array.element_type->chk_this_value(v_comp, lhs,
            expected_value, incomplete_allowed, OMIT_NOT_ALLOWED, SUB_CHK,
            implicit_omit);
        }
      }
    } else {
      // Indexed-list notation.
      bool array_size_known = !dim->get_has_error();
      bool check_holes = array_size_known
                         && expected_value == EXPECTED_CONSTANT;
      size_t array_size = 0;
      if (array_size_known) array_size = dim->get_size();
      map<Int, Int> index_map;
      for (size_t i = 0; i < n_value_comps; i++) {
        Error_Context cntxt(this, "In array element #%lu",
                            (unsigned long)(i + 1));
        Value *val = value->get_comp_byIndex(i);
        Value *index = value->get_index_byIndex(i);
        dim->chk_index(index, expected_value);
        if (index->get_value_refd_last()->get_valuetype() == Value::V_INT) {
          const int_val_t *index_int = index->get_value_refd_last()
                                       ->get_val_Int();
          if (*index_int > INT_MAX) {
            index->error("An integer value less than `%d' was expected for "
                         "indexing type `%s' instead of `%s'", INT_MAX,
                         get_typename().c_str(),
                         (index_int->t_str()).c_str());
            index->set_valuetype(Value::V_ERROR);
            check_holes = false;
          } else {
            // Index overflows/underflows are already checked by
            // ArrayDimensions::chk_index() at this point.  The only thing
            // we need to check is the uniqueness of constant index values.
            Int index_val = index_int->get_val();
            if (index_map.has_key(index_val)) {
              index->error("Duplicate index value `%s' for components `%s' "
                           "and `%s'", Int2string(index_val).c_str(),
                           Int2string((Int)i + 1).c_str(),
                           Int2string(*index_map[index_val]).c_str());
              index->set_valuetype(Value::V_ERROR);
              check_holes = false;
            } else {
              index_map.add(index_val, new Int((Int)i + 1));
            }
          }
        } else {
          check_holes = false;
        }
        val->set_my_governor(u.array.element_type);
        u.array.element_type->chk_this_value_ref(val);
        self_ref |= u.array.element_type->chk_this_value(val, lhs, expected_value,
          incomplete_allowed, OMIT_NOT_ALLOWED, SUB_CHK, implicit_omit);
      }
      if (check_holes) {
        if (index_map.size() < array_size) {
          // It's a constant array assigned with constant index values.  The
          // # of constant index values doesn't reach the size of the array.
          value->error("It's not allowed to create hole(s) in constant "
                       "values");
          value->set_valuetype(Value::V_ERROR);
        }
      }
      for (size_t i = 0; i < index_map.size(); i++)
        delete index_map.get_nth_elem(i);
      index_map.clear();
    }
    break; }
  default:
    value->error("array value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }

  return self_ref;
}

bool Type::chk_this_value_Signature(Value *value, Common::Assignment *lhs,
                                    expected_value_t expected_value,
                                    namedbool incomplete_allowed)
{
  bool self_ref = false;
  switch(value->get_valuetype()) {
  case Value::V_SEQOF:
    value->set_valuetype(Value::V_SEQ);
    // no break
  case Value::V_SEQ:{
    map<string, NamedValue> comp_map;
    // it is set to false if we have lost the ordering
    bool in_synch = true;
    size_t n_type_comps = get_nof_comps();
    size_t n_value_comps = value->get_nof_comps();
    // for incomplete values
    CompField *last_cf = 0;
    size_t next_index = 0;
    for (size_t v_i = 0; v_i < n_value_comps; v_i++) {
      NamedValue *nv = value->get_se_comp_byIndex(v_i);
      const Identifier& value_id = nv->get_name();
      const string& value_name = value_id.get_name();
      const char *value_dispname_str = value_id.get_dispname().c_str();
      if (!has_comp_withName(value_id)) {
        nv->error("Reference to non-existent parameter `%s' in signature value for "
          "type `%s'", value_dispname_str, get_typename().c_str());
        in_synch = false;
        continue;
      } else if (comp_map.has_key(value_name)) {
        nv->error("Duplicate signature parameter `%s'", value_dispname_str);
        comp_map[value_name]->note("Parameter `%s' is already given here",
          value_dispname_str);
        in_synch = false;
      } else comp_map.add(value_name, nv);
      CompField *cf = get_comp_byName(value_id);
      // check the ordering of fields
      if (in_synch) {
        if (INCOMPLETE_NOT_ALLOWED != incomplete_allowed) {
          bool found = false;
          for (size_t i = next_index; i < n_type_comps; i++) {
            CompField *cf2 = get_comp_byIndex(i);
            if (value_name == cf2->get_name().get_name()) {
              last_cf = cf2;
              next_index = i + 1;
              found = true;
              break;
            }
          }
          if (!found) {
            nv->error("Field `%s' cannot appear after parameter `%s' in signature "
              "value", value_dispname_str,
              last_cf->get_name().get_dispname().c_str());
            in_synch = false;
          }
        } else {
          // the value must be complete
          CompField *cf2 = get_comp_byIndex(v_i);
          if (cf2 != cf) {
            nv->error("Unexpected field `%s' in record value, "
              "expecting `%s'", value_dispname_str,
              cf2->get_name().get_dispname().c_str());
            in_synch = false;
          }
        }
      }
      Type *type = cf->get_type();
      Value *comp_value = nv->get_value();
      comp_value->set_my_governor(type);
      Error_Context cntxt(nv, "In value for signature parameter `%s'",
        value_dispname_str);
      type->chk_this_value_ref(comp_value);
      self_ref |= type->chk_this_value(comp_value, lhs, expected_value,
        INCOMPLETE_NOT_ALLOWED,
        cf->get_is_optional() ? OMIT_ALLOWED : OMIT_NOT_ALLOWED, SUB_CHK);
    }
    if (INCOMPLETE_NOT_ALLOWED == incomplete_allowed) {
      for (size_t i = 0; i < u.signature.parameters->get_nof_in_params(); i++) {
        const Identifier& id = get_comp_byIndex(i)->get_name();   //u.signature.parameters->get_param_byIndex(n)
        if (!comp_map.has_key(id.get_name()) && SignatureParam::PARAM_OUT != u.signature.parameters->get_in_param_byIndex(i)->get_direction()) {
            value->error("Field `%s' is missing from signature value",
              id.get_dispname().c_str());
        }
      }
    }
    comp_map.clear();
    break;}
  default:
    value->error("Signature value was expected for type `%s'",
                 get_fullname().c_str());
    value->set_valuetype(Value::V_ERROR);
    break;
  }

  return self_ref;
}

/** \todo enhance to handle mtc, system, etc. */
void Type::chk_this_value_Component(Value *value)
{
  switch (value->get_valuetype()) {
  case Value::V_TTCN3_NULL:
    value->set_valuetype_COMP_NULL();
    break;
  case Value::V_EXPR:
    if (value->get_optype() == Value::OPTYPE_COMP_NULL)
      break; // set_valuetype_COMP_NULL was called already
    // no break
  default:
    value->error("component reference value was expected");
    value->set_valuetype(Value::V_ERROR);
    break;
  }
}

void Type::chk_this_value_FAT(Value *value)
{
  switch (value->get_valuetype()) {
  case Value::V_TTCN3_NULL:
    value->set_valuetype(Value::V_FAT_NULL);
    return;
  case Value::V_REFER:
    // see later
    break;
  case Value::V_FUNCTION:
  case Value::V_ALTSTEP:
  case Value::V_TESTCASE:
  case Value::V_FAT_NULL:
    // This function (chk_this_value_FAT) might have transformed the value
    // from V_REFER or V_TTCN3_NULL to one of these. Return now,
    // otherwise spurious errors are generated.
    return;
  default:
    switch (typetype) {
    case T_FUNCTION:
      value->error("Reference to a function or external function was "
        "expected");
      break;
    case T_ALTSTEP:
      value->error("Reference to an altstep was expected");
      break;
    case T_TESTCASE:
      value->error("Reference to a testcase was expected");
      break;
    default:
      FATAL_ERROR("Type::chk_this_value_FAT()");
    }
    value->set_valuetype(Value::V_ERROR);
    return;
  } // switch valuetype

  // Here value->valuetype is V_REFER
  Assignment *t_ass = value->get_refered()->get_refd_assignment(false);
  if (!t_ass) {
    value->set_valuetype(Value::V_ERROR);
    return;
  }
  t_ass->chk();
  // check whether refers() points to the right definition
  Assignment::asstype_t t_asstype = t_ass->get_asstype();
  switch (typetype) {
  case T_FUNCTION:
    switch (t_asstype) {
    case Assignment::A_FUNCTION:
    case Assignment::A_FUNCTION_RVAL:
    case Assignment::A_FUNCTION_RTEMP:
    case Assignment::A_EXT_FUNCTION:
    case Assignment::A_EXT_FUNCTION_RVAL:
    case Assignment::A_EXT_FUNCTION_RTEMP:
      value->set_valuetype(Value::V_FUNCTION, t_ass);
      break;
    default:
      value->error("Reference to a function or external function was "
        "expected instead of %s", t_ass->get_description().c_str());
      value->set_valuetype(Value::V_ERROR);
      return;
    }
    break;
  case T_ALTSTEP:
    if (t_ass->get_asstype() == Assignment::A_ALTSTEP) {
      value->set_valuetype(Value::V_ALTSTEP, t_ass);
      break;
    } else {
      value->error("Reference to an altstep was expected instead of %s",
        t_ass->get_description().c_str());
      value->set_valuetype(Value::V_ERROR);
      return;
    }
  case T_TESTCASE:
    if (t_ass->get_asstype() == Assignment::A_TESTCASE) {
      value->set_valuetype(Value::V_TESTCASE, t_ass);
      break;
    } else {
      value->error("Reference to a testcase was expected instead of %s",
        t_ass->get_description().c_str());
      value->set_valuetype(Value::V_ERROR);
      return;
    }
  default:
    FATAL_ERROR("Type::chk_this_value_FAT()");
  }
  // comparison of formal parameter lists
  {
    Error_Context cntxt(value, "In comparing formal parameter lists of "
      "type `%s' and %s", get_typename().c_str(),
      t_ass->get_description().c_str());
    u.fatref.fp_list->chk_compatibility(t_ass->get_FormalParList(),
      get_typename().c_str());
  }
  // comparison of 'runs on' clauses
  Type *t_runs_on_type = t_ass->get_RunsOnType();
  if (t_runs_on_type) {
    if (u.fatref.runs_on.self) {
      if (u.fatref.runs_on.ref) {
        FATAL_ERROR("Type::chk_this_value_FAT()");
      } else { // "runs on self" case
        // check against the runs on component type of the scope of the value
        Scope* value_scope = value->get_my_scope();
        if (!value_scope) FATAL_ERROR("Type::chk_this_value_FAT()");
        Ttcn::RunsOnScope *t_ros = value_scope->get_scope_runs_on();
        if (t_ros) {
          Type *scope_comptype = t_ros->get_component_type();
          if (!t_runs_on_type->is_compatible(scope_comptype, NULL, NULL)) {
            value->error("Runs on clause mismatch: type `%s' has a "
              "'runs on self' clause and the current scope expects component "
              "type `%s', but %s runs on `%s'", get_typename().c_str(),
              scope_comptype->get_typename().c_str(),
              t_ass->get_description().c_str(),
              t_runs_on_type->get_typename().c_str());
          }
        } else { // does not have 'runs on' clause
          // if the value's scope is a component body then check the runs on
          // compatibility using this component type as the scope
          ComponentTypeBody* ct_body =
            dynamic_cast<ComponentTypeBody*>(value_scope);
          if (ct_body) {
            if (!t_runs_on_type->is_compatible(ct_body->get_my_type(), NULL, NULL)) {
              value->error("Runs on clause mismatch: type `%s' has a "
                "'runs on self' clause and the current component definition "
                "is of type `%s', but %s runs on `%s'",
                get_typename().c_str(),
                ct_body->get_my_type()->get_typename().c_str(),
                t_ass->get_description().c_str(),
                t_runs_on_type->get_typename().c_str());
            }
          } else {
            value->error("Type `%s' has a 'runs on self' clause and the "
              "current scope does not have a 'runs on' clause, "
              "but %s runs on `%s'", get_typename().c_str(),
              t_ass->get_description().c_str(),
            t_runs_on_type->get_typename().c_str());
        }
      }
      }
    } else {
      if (u.fatref.runs_on.ref) {
        if (u.fatref.runs_on.type &&
            !t_runs_on_type->is_compatible(u.fatref.runs_on.type, NULL, NULL)) {
          value->error("Runs on clause mismatch: type `%s' expects component "
            "type `%s', but %s runs on `%s'", get_typename().c_str(),
            u.fatref.runs_on.type->get_typename().c_str(),
            t_ass->get_description().c_str(),
            t_runs_on_type->get_typename().c_str());
        }
      } else {
        value->error("Type `%s' does not have 'runs on' clause, but %s runs "
          "on `%s'", get_typename().c_str(), t_ass->get_description().c_str(),
          t_runs_on_type->get_typename().c_str());
      }
    }
  }
  if (typetype == T_TESTCASE) {
    // comparison of system clauses
    Ttcn::Def_Testcase *t_testcase = dynamic_cast<Ttcn::Def_Testcase*>(t_ass);
    if (!t_testcase) FATAL_ERROR("Type::chk_this_value_FAT()");
    Type *t_system_type = t_testcase->get_SystemType();
    //  if the system clause is missing we shall examine the component type
    // that is given in the 'runs on' clause
    if (!t_system_type) t_system_type = t_runs_on_type;
    Type *my_system_type =
      u.fatref.system.ref ? u.fatref.system.type : u.fatref.runs_on.type;
    if (t_system_type && my_system_type &&
        !t_system_type->is_compatible(my_system_type, NULL, NULL)) {
      value->error("System clause mismatch: testcase type `%s' expects "
        "component type `%s', but %s has `%s'", get_typename().c_str(),
        my_system_type->get_typename().c_str(),
        t_ass->get_description().c_str(),
        t_system_type->get_typename().c_str());
    }
  } else if (typetype == T_FUNCTION) {
    // comparison of return types
    bool t_returns_template = false;
    switch (t_asstype) {
    case Assignment::A_FUNCTION:
    case Assignment::A_EXT_FUNCTION:
      if (u.fatref.return_type) {
        value->error("Type `%s' expects a function or external function "
          "that returns a %s of type `%s', but %s does not have return type",
          get_typename().c_str(),
          u.fatref.returns_template ? "template" : "value",
          u.fatref.return_type->get_typename().c_str(),
          t_ass->get_description().c_str());
      }
      break;
    case Assignment::A_FUNCTION_RTEMP:
    case Assignment::A_EXT_FUNCTION_RTEMP: {
      // comparison of template restrictions
      Ttcn::Def_Function_Base* dfb =
        dynamic_cast<Ttcn::Def_Function_Base*>(t_ass);
      if (!dfb) FATAL_ERROR("Type::chk_this_value_FAT()");
      template_restriction_t refd_tr = dfb->get_template_restriction();
      if (refd_tr!=u.fatref.template_restriction) {
        value->error("Type `%s' expects a function or external function "
          "that returns a template with %s restriction, "
          "but %s returns a template with %s restriction",
          get_typename().c_str(),
          u.fatref.template_restriction==TR_NONE ? "no" :
          Template::get_restriction_name(u.fatref.template_restriction),
          t_ass->get_description().c_str(),
          refd_tr==TR_NONE ? "no" : Template::get_restriction_name(refd_tr));
      }
    }
      t_returns_template = true;
      // no break
    case Assignment::A_FUNCTION_RVAL:
    case Assignment::A_EXT_FUNCTION_RVAL:
      if (u.fatref.return_type) {
        Type *t_return_type = t_ass->get_Type();
        if (!u.fatref.return_type->is_identical(t_return_type)) {
          value->error("Return type mismatch: type `%s' expects a function "
            "or external function that returns a %s of type `%s', but %s "
            "returns a %s of type `%s'", get_typename().c_str(),
            u.fatref.returns_template ? "template" : "value",
            u.fatref.return_type->get_typename().c_str(),
            t_ass->get_description().c_str(),
            t_returns_template ? "template" : "value",
            t_return_type->get_typename().c_str());
        } else if (u.fatref.return_type->get_sub_type() && t_return_type->get_sub_type() &&
          (u.fatref.return_type->get_sub_type()->get_subtypetype()==t_return_type->get_sub_type()->get_subtypetype()) &&
          (!u.fatref.return_type->get_sub_type()->is_compatible(t_return_type->get_sub_type()))) {
          // TODO: maybe equivalence should be checked, or maybe that is too strict
          value->error("Return type subtype mismatch: subtype %s has no common value with subtype %s",
                     u.fatref.return_type->get_sub_type()->to_string().c_str(),
                     t_return_type->get_sub_type()->to_string().c_str());
        } else if (u.fatref.returns_template != t_returns_template) {
          value->error("Type `%s' expects a function or external function "
            "that returns a %s of type `%s', but %s returns a %s",
            get_typename().c_str(),
            u.fatref.returns_template ? "template" : "value",
            u.fatref.return_type->get_typename().c_str(),
            t_ass->get_description().c_str(),
            t_returns_template ? "template" : "value");
        }
      } else {
        value->error("Type `%s' expects a function or external function "
          "without return type, but %s returns a %s of type `%s'",
          get_typename().c_str(), t_ass->get_description().c_str(),
          t_returns_template ? "template" : "value",
          t_ass->get_Type()->get_typename().c_str());
      }
      break;
    default:
      FATAL_ERROR("Type::chk_this_value_FAT()");
    }
  }
}

void Type::chk_this_template_length_restriction(Template *t)
{
  Ttcn::LengthRestriction *lr = t->get_length_restriction();
  if (!lr) FATAL_ERROR("Type::chk_this_template_length_restriction()");
  // first check the length restriction itself
  lr->chk(EXPECTED_DYNAMIC_VALUE);
  if (is_ref()) {
    /** \todo check lr against the length subtype constraint of this */
    get_type_refd()->chk_this_template_length_restriction(t);
    return;
  }
  typetype_t tt = get_typetype_ttcn3(typetype);
  switch (tt) {
  case T_ERROR:
  case T_PORT:
    // return silently, the type of the template is erroneous anyway
    return;
  case T_ARRAY:
    lr->chk_array_size(u.array.dimension);
    /* no break */
  case T_BSTR:
  case T_HSTR:
  case T_OSTR:
  case T_CSTR:
  case T_USTR:
  case T_SETOF:
  case T_SEQOF:
    // length restriction is allowed for these types
    break;
  default:
    lr->error("Length restriction cannot be used in a template of type `%s'",
      get_typename().c_str());
    return;
  }
  // check whether the number of elements in t is in accordance with lr
  Template *t_last = t->get_template_refd_last();
  switch (t_last->get_templatetype()) {
  case Ttcn::Template::OMIT_VALUE:
    lr->error("Length restriction cannot be used with omit value");
    break;
  case Ttcn::Template::SPECIFIC_VALUE: {
    Value *v_last = t_last->get_specific_value()->get_value_refd_last();
    switch (v_last->get_valuetype()) {
    case Value::V_BSTR:
      if (tt == T_BSTR)
        lr->chk_nof_elements(v_last->get_val_strlen(), false, *t, "string");
      break;
    case Value::V_HSTR:
      if (tt == T_HSTR)
        lr->chk_nof_elements(v_last->get_val_strlen(), false, *t, "string");
      break;
    case Value::V_OSTR:
      if (tt == T_OSTR)
        lr->chk_nof_elements(v_last->get_val_strlen(), false, *t, "string");
      break;
    case Value::V_CSTR:
      if (tt == T_CSTR || tt == T_USTR)
        lr->chk_nof_elements(v_last->get_val_strlen(), false, *t, "string");
      break;
    case Value::V_USTR:
      if (tt == T_USTR)
        lr->chk_nof_elements(v_last->get_val_strlen(), false, *t, "string");
      break;
    case Value::V_SEQOF:
      if (tt == T_SEQOF)
        lr->chk_nof_elements(v_last->get_nof_comps(), false, *t, "value");
      break;
    case Value::V_SETOF:
      if (tt == T_SETOF)
        lr->chk_nof_elements(v_last->get_nof_comps(), false, *t, "value");
      break;
    // note: do not check array values
    // they are already verified against the array size
    case Value::V_OMIT:
      lr->error("Length restriction cannot be used with omit value");
    default:
      // we cannot verify anything on other value types
      // they are either correct or not applicable to the type
      break;
    }
    break; }
  case Ttcn::Template::TEMPLATE_LIST:
    if (tt == T_SEQOF || tt == T_SETOF)
      lr->chk_nof_elements(t_last->get_nof_comps_not_anyornone(),
        t_last->temps_contains_anyornone_symbol(), *t, "template");
    // note: do not check array templates
    // they are already verified against the array size
    break;
  case Ttcn::Template::SUPERSET_MATCH:
    if (tt == T_SETOF)
      lr->chk_nof_elements(t_last->get_nof_comps_not_anyornone(), true,
        *t, "template");
    break;
  case Ttcn::Template::BSTR_PATTERN:
    if (tt == T_BSTR)
      lr->chk_nof_elements(t_last->get_min_length_of_pattern(),
        t_last->pattern_contains_anyornone_symbol(), *t, "string");
    break;
  case Ttcn::Template::HSTR_PATTERN:
    if (tt == T_HSTR)
      lr->chk_nof_elements(t_last->get_min_length_of_pattern(),
        t_last->pattern_contains_anyornone_symbol(), *t, "string");
    break;
  case Ttcn::Template::OSTR_PATTERN:
    if (tt == T_OSTR)
      lr->chk_nof_elements(t_last->get_min_length_of_pattern(),
        t_last->pattern_contains_anyornone_symbol(), *t, "string");
    break;
  case Ttcn::Template::CSTR_PATTERN:
    if (tt == T_CSTR || tt == T_USTR)
      lr->chk_nof_elements(t_last->get_min_length_of_pattern(),
        t_last->pattern_contains_anyornone_symbol(), *t, "string");
    break;
  default:
    // We cannot verify anything on other matching mechanisms.
    // They are either correct or not applicable to the type.
    break;
  }
}

void Type::chk_this_template_ref(Template *t)
{
  switch (t->get_templatetype()) {
  case Ttcn::Template::SPECIFIC_VALUE: {
//  if (t->get_templatetype() != Ttcn::Template::SPECIFIC_VALUE) return;
  Value *v = t->get_specific_value();
  chk_this_value_ref(v);
  switch (v->get_valuetype()) {
  case Value::V_REFD: {
    // Do not check the actual parameter list of the reference yet to avoid
    // endless recursion in case of embedded circular references.
    // The parameter lists will be verified later.
    Assignment *ass = v->get_reference()->get_refd_assignment(false);
    if (ass) {
      switch (ass->get_asstype()) {
      case Assignment::A_VAR_TEMPLATE: {
        Type* type = ass->get_Type();
        switch (type->typetype) {
          case T_BSTR:
          case T_BSTR_A:
          case T_HSTR:
          case T_OSTR:
          case T_CSTR:
          case T_USTR:
          case T_UTF8STRING:
          case T_NUMERICSTRING:
          case T_PRINTABLESTRING:
          case T_TELETEXSTRING:
          case T_VIDEOTEXSTRING:
          case T_IA5STRING:
          case T_GRAPHICSTRING:
          case T_VISIBLESTRING:
          case T_GENERALSTRING:
          case T_UNIVERSALSTRING:
          case T_BMPSTRING:
          case T_UTCTIME:
          case T_GENERALIZEDTIME:
          case T_OBJECTDESCRIPTOR: {
            // TR 921 (indexing of template strings)
            Ttcn::FieldOrArrayRefs *subrefs = v->get_reference()
              ->get_subrefs();
            if (!subrefs) break;
            size_t nof_subrefs = subrefs->get_nof_refs();
            if (nof_subrefs > 0) {
              Ttcn::FieldOrArrayRef *last_ref = subrefs
                ->get_ref(nof_subrefs - 1);
              if (last_ref->get_type() == Ttcn::FieldOrArrayRef::ARRAY_REF) {
                t->error("Reference to %s can not be indexed",
                    (ass->get_description()).c_str());
                t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
                return;
              }
            }
            break; }
          default:
            break;
        }
      }
      case Assignment::A_MODULEPAR_TEMP:
      case Assignment::A_TEMPLATE:
      case Assignment::A_PAR_TEMPL_IN:
      case Assignment::A_PAR_TEMPL_OUT:
      case Assignment::A_PAR_TEMPL_INOUT:
      case Assignment::A_FUNCTION_RTEMP:
      case Assignment::A_EXT_FUNCTION_RTEMP:
        t->set_templatetype(Ttcn::Template::TEMPLATE_REFD);
      default:
        break;
      }
    } else t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
    break; }
  case Value::V_INVOKE: {
    Type *t2 = v->get_invoked_type(Type::EXPECTED_TEMPLATE);
    if(t2 && t2->get_type_refd_last()->get_returns_template())
      t->set_templatetype(Ttcn::Template::TEMPLATE_INVOKE);
    break; }
  default:
    break;
  }
  break;
  }
  case Ttcn::Template::TEMPLATE_REFD: {
        // Do not check the actual parameter list of the reference yet to avoid
    // endless recursion in case of embedded circular references.
    // The parameter lists will be verified later.
    Assignment *ass = t->get_reference()->get_refd_assignment(false);
    if (ass) {
      switch (ass->get_asstype()) {
      case Assignment::A_VAR_TEMPLATE: {
        Type* type = ass->get_Type();
        switch (type->typetype) {
          case T_BSTR:
          case T_BSTR_A:
          case T_HSTR:
          case T_OSTR:
          case T_CSTR:
          case T_USTR:
          case T_UTF8STRING:
          case T_NUMERICSTRING:
          case T_PRINTABLESTRING:
          case T_TELETEXSTRING:
          case T_VIDEOTEXSTRING:
          case T_IA5STRING:
          case T_GRAPHICSTRING:
          case T_VISIBLESTRING:
          case T_GENERALSTRING:
          case T_UNIVERSALSTRING:
          case T_BMPSTRING:
          case T_UTCTIME:
          case T_GENERALIZEDTIME:
          case T_OBJECTDESCRIPTOR: {
            // TR 921 (indexing of template strings)
            Ttcn::FieldOrArrayRefs *subrefs = t->get_reference()
              ->get_subrefs();
            if (!subrefs) break;
            size_t nof_subrefs = subrefs->get_nof_refs();
            if (nof_subrefs > 0) {
              Ttcn::FieldOrArrayRef *last_ref = subrefs
                ->get_ref(nof_subrefs - 1);
              if (last_ref->get_type() == Ttcn::FieldOrArrayRef::ARRAY_REF) {
                t->error("Reference to %s can not be indexed",
                  (ass->get_description()).c_str());
                t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
                return;
              }
            }
            break; }
          default:
            break;
        }
      }
      case Assignment::A_MODULEPAR_TEMP:
      case Assignment::A_TEMPLATE:
      case Assignment::A_PAR_TEMPL_IN:
      case Assignment::A_PAR_TEMPL_OUT:
      case Assignment::A_PAR_TEMPL_INOUT:
      case Assignment::A_FUNCTION_RTEMP:
      case Assignment::A_EXT_FUNCTION_RTEMP:
        t->set_templatetype(Ttcn::Template::TEMPLATE_REFD);
      default:
        break;
      }
    } else t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
  }
  break;
  default:
      return;
  }
}

bool Type::chk_this_template_ref_pard(Ttcn::Ref_pard* ref_pard, Common::Assignment* lhs) 
{
  // Check if the reference on the left hand side of the assignment can be found 
  // among the parameters
  Ttcn::ActualParList* par_list = ref_pard->get_parlist();
  if (par_list) {
    size_t nof_pars = par_list->get_nof_pars();
    for (size_t i = 0; i < nof_pars; ++i) {
      Ttcn::ActualPar* par = par_list->get_par(i);
      Ttcn::Ref_base* par_ref = 0;
      switch(par->get_selection()) {
      case Ttcn::ActualPar::AP_TEMPLATE: {
        Ttcn::TemplateInstance* temp_ins = par->get_TemplateInstance();
        Ttcn::Template* temp = temp_ins->get_Template();
        if (temp->get_templatetype() == Ttcn::Template::TEMPLATE_REFD) {
          par_ref = temp->get_reference();
        }
        break;
      }
      case Ttcn::ActualPar::AP_REF: {
        par_ref = par->get_Ref();
        break;
      }
      default:
        break;
      }

      if (par_ref != 0) {
        Common::Assignment* ass = par_ref->get_refd_assignment(true); // also check parameters if there are any
        if (ass == lhs)
          return true; 
        
        // In case a parameter is another function call / parametrised template
        // check their parameters as well
        if (ass->get_asstype() == Assignment::A_FUNCTION_RTEMP ||
          ass->get_asstype() == Assignment::A_EXT_FUNCTION_RTEMP ||
          ass->get_asstype() == Assignment::A_TEMPLATE) {
          ref_pard = dynamic_cast<Ttcn::Ref_pard*>(par_ref);
          if (ref_pard && chk_this_template_ref_pard(ref_pard, lhs))
            return true;
        }  
      }
    }
  }
  return false;
}

bool Type::chk_this_template_generic(Template *t, namedbool incomplete_allowed,
  namedbool allow_omit, namedbool allow_any_or_omit, namedbool sub_chk,
  namedbool implicit_omit, Common::Assignment *lhs)
{
  bool self_ref = false;
  // get_template_refd_last evaluates concatenations between templates known at
  // compile-time
  Ttcn::Template::templatetype_t tt = t->get_template_refd_last()->get_templatetype();
  if (OMIT_ALLOWED != allow_omit && tt == Ttcn::Template::OMIT_VALUE) {
      t->error("`omit' value is not allowed in this context");
  }
  if (ANY_OR_OMIT_ALLOWED != allow_any_or_omit && tt == Ttcn::Template::ANY_OR_OMIT) {
      t->error("Using `*' for mandatory field");
  }

  Ttcn::Template::templatetype_t temptype = t->get_templatetype();
  switch(temptype) {
  case Ttcn::Template::TEMPLATE_ERROR:
    break;
  case Ttcn::Template::TEMPLATE_NOTUSED:
  case Ttcn::Template::OMIT_VALUE:
  case Ttcn::Template::ANY_VALUE:
  case Ttcn::Template::ANY_OR_OMIT: {
    Type *type=get_type_refd_last();
    if (type->get_typetype() == T_SIGNATURE)
      t->error("Generic wildcard `%s' cannot be used for signature `%s'",
               t->get_templatetype_str(), type->get_fullname().c_str());
    break;}
  case Ttcn::Template::ALL_FROM: {
    Ttcn::Template *af = t->get_all_from();
    switch (af->get_templatetype()) {
    case Ttcn::Template::SPECIFIC_VALUE: {
      Value *v = af->get_specific_value();
      v->set_lowerid_to_ref();
      Reference  *ref = v->get_reference();
      Assignment *ass = ref->get_refd_assignment(true); // also check parameters if there are any
      if (!ass) break; // probably erroneous
      //warning("asstype %d", ass->get_asstype());

      switch (ass->get_asstype()) {
      case Assignment::A_VAR:
      case Assignment::A_PAR_VAL_IN:
      case Assignment::A_PAR_VAL_INOUT:
      case Assignment::A_MODULEPAR:
      case Assignment::A_CONST:
      case Assignment::A_MODULEPAR_TEMP:
        break; // acceptable
      case Assignment::A_VAR_TEMPLATE:
      case Assignment::A_PAR_TEMPL_IN:
      case Assignment::A_PAR_TEMPL_INOUT:
        if (ass == lhs)
          self_ref = true;
        break; // acceptable
      case Assignment::A_FUNCTION_RVAL:
      case Assignment::A_EXT_FUNCTION_RVAL:
      case Assignment::A_FUNCTION_RTEMP:
      case Assignment::A_EXT_FUNCTION_RTEMP:
      case Assignment::A_TEMPLATE: {
        Ttcn::Ref_pard* ref_pard = dynamic_cast<Ttcn::Ref_pard*>(ref);
        if (ref_pard)
          self_ref |= chk_this_template_ref_pard(ref_pard, lhs);
        break; // acceptable
      }
      default:
        error("A %s cannot be used as the target of `all from'",
          ass->get_assname());
        break;
      }
      //self_ref |= chk_this_value(v, lhs, EXPECTED_TEMPLATE, incomplete_allowed,
      //  allow_omit, sub_chk, implicit_omit, NOT_STR_ELEM /* ?? */);
      break; }

    default:
      error("The target of an 'all from' must be a specific value template, not a %s",
        af->get_templatetype_str());
      break;
    }

    break; }
  case Ttcn::Template::VALUE_LIST:
  case Ttcn::Template::COMPLEMENTED_LIST: {
    size_t nof_comps = t->get_nof_comps();
    for (size_t i = 0; i < nof_comps; i++) {
      Template* t_comp = t->get_temp_byIndex(i);
      Error_Context cntxt(t, "In list item %lu", (unsigned long) (i + 1));
      t_comp->set_my_governor(this);
      chk_this_template_ref(t_comp);
      self_ref |= chk_this_template_generic(t_comp, INCOMPLETE_NOT_ALLOWED,
        omit_in_value_list ? allow_omit : OMIT_NOT_ALLOWED,
        ANY_OR_OMIT_ALLOWED, sub_chk, implicit_omit, lhs);
      if(temptype==Ttcn::Template::COMPLEMENTED_LIST &&
         t_comp->get_template_refd_last()->get_templatetype() ==
         Ttcn::Template::ANY_OR_OMIT)
        t_comp->warning("`*' in complemented list."
                        " This template will not match anything");
    }
    break;}
  case Ttcn::Template::SPECIFIC_VALUE: {
    Value *v = t->get_specific_value();
    v->set_my_governor(this);
    self_ref |= chk_this_value(v, lhs, EXPECTED_TEMPLATE, incomplete_allowed,
      allow_omit, SUB_CHK);
    break; }
  case Ttcn::Template::TEMPLATE_INVOKE: {
    t->chk_invoke();
    Type *governor = t->get_expr_governor(EXPECTED_DYNAMIC_VALUE);
    if(!governor)
      t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
    else if (!is_compatible(governor, NULL, t)) {
      t->error("Type mismatch: a value or template of type `%s' was "
        "expected instead of `%s'", get_typename().c_str(),
        governor->get_typename().c_str());
      t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
    }
    break; }
  case Ttcn::Template::TEMPLATE_REFD:
    self_ref = chk_this_refd_template(t, lhs);
    break;
  case Ttcn::Template::TEMPLATE_CONCAT:
    {
      if (!use_runtime_2) {
        FATAL_ERROR("Type::chk_this_template_generic()");
      }
      Error_Context cntxt(t, "In template concatenation");
      Template* t_left = t->get_concat_operand(true);
      Template* t_right = t->get_concat_operand(false);
      {
        Error_Context cntxt2(t, "In first operand");
        self_ref |= chk_this_template_concat_operand(t_left, implicit_omit, lhs);
      }
      {
        Error_Context cntxt2(t, "In second operand");
        self_ref |= chk_this_template_concat_operand(t_right, implicit_omit, lhs);
      }
      if (t_left->get_templatetype() == Ttcn::Template::TEMPLATE_ERROR ||
          t_right->get_templatetype() == Ttcn::Template::TEMPLATE_ERROR) {
        t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
      }
    }
    break;
  default:
    self_ref = chk_this_template(t, incomplete_allowed, sub_chk, implicit_omit, lhs);
    break;
  }
  if (t->get_length_restriction()) chk_this_template_length_restriction(t);
  if (OMIT_ALLOWED != allow_omit && t->get_ifpresent())
    t->error("`ifpresent' is not allowed here");
  if(SUB_CHK == sub_chk && temptype != Ttcn::Template::PERMUTATION_MATCH) {
    /* Note: A "permuation" itself has no type - it is just a fragment. */
    if(sub_type!=NULL) sub_type->chk_this_template_generic(t);
  }

  return self_ref;
}

bool Type::chk_this_refd_template(Template *t, Common::Assignment *lhs)
{
  if (t->get_templatetype() != Template::TEMPLATE_REFD) return false;
  Reference *ref = t->get_reference();
  Assignment *ass = ref->get_refd_assignment();
  if (!ass) FATAL_ERROR("Type::chk_this_refd_template()");
  ass->chk();

  Type *governor = ass->get_Type()
    ->get_field_type(ref->get_subrefs(), EXPECTED_DYNAMIC_VALUE);
  if (!governor) {
    t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
  } else {
    TypeCompatInfo info(t->get_my_scope()->get_scope_mod(), this, governor,
                        true, false, true);
    if (ref->get_subrefs()) info.set_str2_elem(ref->get_subrefs()->refers_to_string_element());
    TypeChain l_chain;
    TypeChain r_chain;
    if (!is_compatible(governor, &info, t, &l_chain, &r_chain)) {
      Type *type = get_type_refd_last();
      switch (type->typetype) {
      case T_PORT:
        // Port templates do not exist, remain silent.
        break;
      case T_SIGNATURE:
        t->error("Type mismatch: a signature template of type `%s' was "
                 "expected instead of `%s'", get_typename().c_str(),
                 governor->get_typename().c_str());
        break;
      default:
        if (info.is_subtype_error()) {
          t->error("%s", info.get_subtype_error().c_str());
        } else if (!info.is_erroneous()) {
          t->error("Type mismatch: a value or template of type `%s' was "
                   "expected instead of `%s'", get_typename().c_str(),
                   governor->get_typename().c_str());
        } else {
          t->error("%s", info.get_error_str_str().c_str());
        }
        break;
      }
      t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
    } else {
      // Detect circular references.
      t->get_template_refd_last();

      if (info.needs_conversion()) t->set_needs_conversion();
    }
  }
  return (lhs == ass);
}

bool Type::chk_this_template_concat_operand(Template* t, namedbool implicit_omit,
                                            Common::Assignment *lhs)
{
  Type* governor = t->get_expr_governor(EXPECTED_TEMPLATE);
  if (governor == NULL) {
    governor = this;
  }
  else if (!is_compatible(governor, NULL, t)) {
    t->error("Type mismatch: a value or template of type `%s' was expected "
      "instead of `%s'", get_typename().c_str(), governor->get_typename().c_str());
    t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
    return false;
  }
  t->set_my_governor(governor);
  bool self_ref = governor->chk_this_template_generic(t, INCOMPLETE_NOT_ALLOWED,
    OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED, NO_SUB_CHK, implicit_omit, lhs);
  Template* t_last = t->get_template_refd_last();
  if (t->get_templatetype() == Ttcn::Template::TEMPLATE_ERROR ||
      t_last->get_templatetype() == Ttcn::Template::TEMPLATE_ERROR) {
    return self_ref;
  }
  typetype_t tt = get_type_refd_last()->get_typetype_ttcn3();
  switch (tt) {
  case T_BSTR:
  case T_HSTR:
  case T_OSTR:
  case T_CSTR:
  case T_USTR:
  case T_SEQOF:
  case T_SETOF:
    switch (t_last->get_templatetype()) {
    case Ttcn::Template::ANY_OR_OMIT:
      if (tt != T_CSTR && tt != T_USTR && t_last->get_length_restriction() == NULL) {
        t->error("%s with no length restriction is not a valid "
          "concatenation operand", t_last->get_templatetype_str());
        t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
        break;
      }
      // else fall through
    case Ttcn::Template::ANY_VALUE:
      if (tt != T_CSTR && tt != T_USTR) {
        Ttcn::LengthRestriction* len_res = t_last->get_length_restriction();
        if (len_res != NULL && len_res->get_is_range() &&
            (len_res->get_upper_value() == NULL ||
             (!len_res->get_lower_value()->is_unfoldable() &&
              !len_res->get_upper_value()->is_unfoldable() &&
              len_res->get_lower_value()->get_val_Int()->get_val() !=
              len_res->get_upper_value()->get_val_Int()->get_val()))) {
          // range length restriction is allowed if the upper and lower limits
          // are the same
          t->error("%s with non-fixed length restriction is not a valid "
            "concatenation operand", t_last->get_templatetype_str());
        }
      }
      else { // charstring or universal charstring
        t->error("%s is not allowed in %scharstring template concatenation",
          t_last->get_templatetype_str(), tt != T_CSTR ? "universal " : "");
        t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
      }
      break;
    case Ttcn::Template::SPECIFIC_VALUE:
    case Ttcn::Template::BSTR_PATTERN:
    case Ttcn::Template::HSTR_PATTERN:
    case Ttcn::Template::OSTR_PATTERN:
    case Ttcn::Template::TEMPLATE_CONCAT:
    case Ttcn::Template::TEMPLATE_REFD:
    case Ttcn::Template::TEMPLATE_LIST: // is every list element allowed... ?
    case Ttcn::Template::INDEXED_TEMPLATE_LIST:
      break;
    default:
      t->error("%s is not a valid concatenation operand",
        t_last->get_templatetype_str());
      t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
      break;
    }
    break;
  default:
    t->error("Templates of type `%s' cannot be concatenated",
      get_typename().c_str());
    t->set_templatetype(Ttcn::Template::TEMPLATE_ERROR);
  }
  return self_ref;
}

bool Type::chk_this_template(Template *t, namedbool incomplete_allowed, namedbool,
  namedbool implicit_omit, Common::Assignment *lhs)
{
  bool self_ref = false;
  Type *t_last = get_type_refd_last();
  t->set_my_governor(t_last);

  switch(t_last->typetype) {
  case T_ERROR:
    break;
  case T_NULL:
  case T_BOOL:
  case T_OID:
  case T_ROID:
  case T_ANY:
  case T_VERDICT:
  case T_COMPONENT:
  case T_DEFAULT:
    t_last->chk_this_template_builtin(t);
    break;
  case T_BSTR_A:
  case T_BSTR:
  case T_HSTR:
  case T_OSTR:
  case T_CSTR:
  case T_USTR:
  case T_UTF8STRING:
  case T_NUMERICSTRING:
  case T_PRINTABLESTRING:
  case T_TELETEXSTRING:
  case T_VIDEOTEXSTRING:
  case T_IA5STRING:
  case T_GRAPHICSTRING:
  case T_VISIBLESTRING:
  case T_GENERALSTRING:
  case T_UNIVERSALSTRING:
  case T_BMPSTRING:
  case T_UTCTIME:
  case T_GENERALIZEDTIME:
  case T_OBJECTDESCRIPTOR:
    self_ref = t_last->chk_this_template_Str(t, implicit_omit, lhs);
    break;
  case T_INT:
  case T_INT_A:
  case T_REAL:
    t_last->chk_this_template_Int_Real(t);
    break;
  case T_ENUM_A:
  case T_ENUM_T:
    t_last->chk_this_template_Enum(t);
    break;
  case T_CHOICE_A:
  case T_CHOICE_T:
  case T_OPENTYPE:
  case T_ANYTYPE:
    self_ref = t_last->chk_this_template_Choice(t, incomplete_allowed, implicit_omit, lhs);
    break;
  case T_SEQ_A:
  case T_SEQ_T:
    self_ref = t_last->chk_this_template_Seq(t, incomplete_allowed, implicit_omit, lhs);
    break;
  case T_SET_A:
  case T_SET_T:
    self_ref = t_last->chk_this_template_Set(t, incomplete_allowed, implicit_omit, lhs);
    break;
  case T_SEQOF:
    self_ref = t_last->chk_this_template_SeqOf(t, incomplete_allowed, implicit_omit, lhs);
    break;
  case T_SETOF:
    self_ref = t_last->chk_this_template_SetOf(t, incomplete_allowed, implicit_omit, lhs);
    break;
  case T_ARRAY:
    self_ref = t_last->chk_this_template_array(t, incomplete_allowed, implicit_omit, lhs);
    break;
  case T_PORT:
    t->error("Template cannot be defined for port type `%s'",
      get_fullname().c_str());
    break;
  case T_SIGNATURE:
    t_last->chk_this_template_Signature(t, incomplete_allowed);
    break;
  case T_FUNCTION:
  case T_ALTSTEP:
  case T_TESTCASE:
    t_last->chk_this_template_Fat(t);
    break;
  default:
    FATAL_ERROR("Type::chk_this_template()");
  } // switch
  return self_ref;
}

bool Type::chk_this_template_Str(Template *t, namedbool implicit_omit,
                                 Common::Assignment *lhs)
{
  bool self_ref = false;
  typetype_t tt = get_typetype_ttcn3(typetype);
  bool report_error = false;
  switch (t->get_templatetype()) {
  case Ttcn::Template::VALUE_RANGE:
    if (tt == T_CSTR || tt == T_USTR) {
      Error_Context cntxt(t, "In value range");
      Ttcn::ValueRange *vr = t->get_value_range();
      Value *v_lower = chk_range_boundary(vr->get_min_v(), "lower", *t);
      Value *v_upper = chk_range_boundary(vr->get_max_v(), "upper", *t);
      vr->set_typetype(get_typetype_ttcn3(typetype));
      if (v_lower && v_upper) {
        // both boundaries are available and have correct type
        if (tt == T_CSTR) {
          if (v_lower->get_val_str() > v_upper->get_val_str())
            t->error("The lower boundary has higher character code than the "
              "upper boundary");
        } else {
          if (v_lower->get_val_ustr() > v_upper->get_val_ustr())
            t->error("The lower boundary has higher character code than the "
              "upper boundary");
        }
      }
    } else report_error = true;
    break;
  case Ttcn::Template::BSTR_PATTERN:
    if (tt != T_BSTR) report_error = true;
    break;
  case Ttcn::Template::HSTR_PATTERN:
    if (tt != T_HSTR) report_error = true;
    break;
  case Ttcn::Template::OSTR_PATTERN:
    if (tt != T_OSTR) report_error = true;
    break;
  case Ttcn::Template::CSTR_PATTERN:
    if (tt == T_CSTR) {
      Ttcn::PatternString *pstr = t->get_cstr_pattern();
      Error_Context cntxt(t, "In character string pattern");
      pstr->chk_refs();
      pstr->join_strings();
      if (!pstr->has_refs()) pstr->chk_pattern();
      break;
    } else if (tt == T_USTR) {
      t->set_templatetype(Template::USTR_PATTERN);
      t->get_ustr_pattern()
        ->set_pattern_type(Ttcn::PatternString::USTR_PATTERN);
      // fall through
    } else {
      report_error = true;
      break;
    }
    // no break
  case Ttcn::Template::USTR_PATTERN:
    if (tt == T_USTR) {
      Ttcn::PatternString *pstr = t->get_ustr_pattern();
      Error_Context cntxt(t, "In universal string pattern");
      pstr->chk_refs();
      pstr->join_strings();
      if (!pstr->has_refs()) pstr->chk_pattern();
    } else report_error = true;
    break;
  case Ttcn::Template::DECODE_MATCH:
    {
      Error_Context cntxt(t, "In decoding target");
      TemplateInstance* target = t->get_decode_target();
      target->get_Template()->set_lowerid_to_ref();
      Type* target_type = target->get_expr_governor(EXPECTED_TEMPLATE);
      if (target_type == NULL) {
        target->error("Type of template instance cannot be determined");
        break;
      }
      if (target->get_Type() != NULL && target_type->is_ref()) {
        target_type = target_type->get_type_refd();
      }
      self_ref = target_type->chk_this_template_generic(
        target->get_Template(), (target->get_DerivedRef() != NULL) ?
        INCOMPLETE_ALLOWED : INCOMPLETE_NOT_ALLOWED,
        OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK, implicit_omit, lhs);
      target_type->get_type_refd_last()->chk_coding(false,
        t->get_my_scope()->get_scope_mod());
    }
    {
      Value* str_enc = t->get_string_encoding();
      if (str_enc != NULL) {
        if (tt != T_USTR) {
          str_enc->error("The encoding format parameter is only available to "
            "universal charstring templates");
          break;
        }
        self_ref |= str_enc->chk_string_encoding(lhs);
      }
    }
    break;
  default:
    report_error = true;
    break;
  }
  if (report_error) {
    t->error("%s cannot be used for type `%s'", t->get_templatetype_str(),
      get_typename().c_str());
  }
  return self_ref;
}

void Type::chk_range_boundary_infinity(Value *v, bool is_upper)
{
  if (v) {
    v->set_my_governor(this);
    {
      Error_Context cntxt2(v, "In %s boundary", is_upper ? "upper" : "lower");
      chk_this_value_ref(v);
      Value *v_last = v->get_value_refd_last(0, EXPECTED_STATIC_VALUE);
      if (v_last->get_valuetype() == Value::V_OMIT) {
        v->error("`omit' value is not allowed in this context");
        v->set_valuetype(Value::V_ERROR);
        return;
      }
      if (sub_type != NULL) {
        if (is_upper) {
          if (!sub_type->get_root()->is_upper_limit_infinity()) {
            v->error("Infinity is not a valid value for type '%s' which has subtype %s",
                     asString(), sub_type->to_string().c_str());
          }
        }
        else {
          if (!sub_type->get_root()->is_lower_limit_infinity()) {
            v->error("Infinity is not a valid value for type '%s' which has subtype %s",
                     asString(), sub_type->to_string().c_str());
          }
        }
      }
    }
  }
}

Value *Type::chk_range_boundary(Value *v, const char *which,
  const Location& loc)
{
  typetype_t tt = get_typetype_ttcn3(typetype);
  Value *ret_val;
  if (v) {
    v->set_my_governor(this);
    {
      Error_Context cntxt2(v, "In %s boundary", which);
      chk_this_value_ref(v);
      chk_this_value(v, 0, EXPECTED_DYNAMIC_VALUE, INCOMPLETE_NOT_ALLOWED,
        OMIT_NOT_ALLOWED, SUB_CHK);
    }
    ret_val = v->get_value_refd_last();
    switch (ret_val->get_valuetype()) {
    case Value::V_INT:
      if (tt != T_INT) ret_val = 0;
      break;
    case Value::V_REAL:
      if (tt != T_REAL) ret_val = 0;
      break;
    case Value::V_CSTR:
      if (tt != T_CSTR) ret_val = 0;
      break;
    case Value::V_USTR:
      if (tt != T_USTR) ret_val = 0;
      break;
    default:
      // unfoldable or erroneous
      ret_val = 0;
      break;
    }
    if ((tt == T_CSTR || tt == T_USTR) && ret_val &&
        ret_val->get_val_strlen() != 1) {
      v->error("The %s boundary must be a %scharstring value containing a "
        "single character", which, tt == T_USTR ? "universal ": "");
      ret_val = 0;
    }
  } else {
    // the boundary is + or - infinity
    if (tt == T_CSTR || tt == T_USTR) {
      loc.error("The %s boundary must be a %scharstring value", which,
        tt == T_USTR ? "universal ": "");
    }
    ret_val = 0;
  }
  return ret_val;
}

void Type::chk_this_template_builtin(Template *t)
{
  t->error("%s cannot be used for type `%s'", t->get_templatetype_str(),
    get_typename().c_str());
  if (t->get_length_restriction()) t->error("Length restriction is not "
    "allowed for type `%s'", get_typename().c_str());
}

void Type::chk_this_template_Int_Real(Template *t)
{
  switch (t->get_templatetype()) {
  case Ttcn::Template::VALUE_RANGE: {
    Error_Context cntxt(t, "In value range");
    Ttcn::ValueRange *vr = t->get_value_range();
    Value *v_lower = chk_range_boundary(vr->get_min_v(), "lower", *t);
    Value *v_upper = chk_range_boundary(vr->get_max_v(), "upper", *t);
    vr->set_typetype(get_typetype_ttcn3(typetype));
    if (v_lower && v_upper) {
      // both boundaries are available and have correct type
      switch (get_typetype_ttcn3(typetype)) {
      case T_INT: {
        if (*v_lower->get_val_Int() > *v_upper->get_val_Int())
          t->error("The lower boundary is higher than the upper boundary");
        break; }
      case T_REAL:
        if (v_lower->get_val_Real() > v_upper->get_val_Real())
          t->error("The lower boundary is higher than the upper boundary");
        break;
      default:
        FATAL_ERROR("Type::chk_this_template_Int_Real()");
      }
    }
    if (v_lower && !vr->get_max_v()) {
      chk_range_boundary_infinity(v_lower, true);
    }
    if (!vr->get_min_v() && v_upper) {
      chk_range_boundary_infinity(v_upper, false);
    }
    if (!vr->get_min_v() && vr->is_min_exclusive() && get_typetype_ttcn3(typetype) == T_INT) {
      t->error("invalid lower boundary, -infinity cannot be excluded from an integer template range");
    }
    if (!vr->get_max_v() && vr->is_max_exclusive() && get_typetype_ttcn3(typetype) == T_INT) {
      t->error("invalid upper boundary, infinity cannot be excluded from an integer template range");
    }
    break;}
  default:
    t->error("%s cannot be used for type `%s'", t->get_templatetype_str(),
      get_typename().c_str());
    break;
  }
  if (t->get_length_restriction()) t->error("Length restriction is not "
    "allowed for type `%s'", get_typename().c_str());
}

void Type::chk_this_template_Enum(Template *t)
{
  t->error("%s cannot be used for enumerated type `%s'",
    t->get_templatetype_str(), get_typename().c_str());
  if (t->get_length_restriction()) t->error("Length restriction is not "
    "allowed for enumerated type `%s'", get_typename().c_str());
}

bool Type::chk_this_template_Choice(Template *t, namedbool incomplete_allowed,
  namedbool implicit_omit, Common::Assignment *lhs)
{
  bool self_ref = false;
  switch (t->get_templatetype()) {
  case Ttcn::Template::NAMED_TEMPLATE_LIST: {
    size_t nof_nts = t->get_nof_comps();
    if (nof_nts != 1) t->error("A template for union type must contain "
      "exactly one selected field");
    // We check all named templates, even though it is an error
    // to have more than one here.
    for (size_t i = 0; i < nof_nts; i++) {
      Ttcn::NamedTemplate *nt = t->get_namedtemp_byIndex(i);
      if (typetype == T_OPENTYPE) {
        // allow the alternatives of open types as both lower and upper identifiers
        nt->set_name_to_lowercase();
      }
      const Identifier& nt_name = nt->get_name();

      if (!has_comp_withName(nt_name)) {
        nt->error("Reference to non-existent field `%s' in union "
                  "template for type `%s'", nt_name.get_dispname().c_str(),
                  get_fullname().c_str());
        nt->note("%s", actual_fields(*this).c_str());
        continue;
      }

      Type *field_type = get_comp_byName(nt_name)->get_type();
      Template *nt_templ = nt->get_template();

      Error_Context cntxt(nt_templ, "In template for union field `%s'",
                          nt_name.get_dispname().c_str());

      nt_templ->set_my_governor(field_type);
      field_type->chk_this_template_ref(nt_templ);
      bool incompl_ok = t->get_completeness_condition_choice(incomplete_allowed, nt_name) ==
        Ttcn::Template::C_MAY_INCOMPLETE;
      self_ref |= field_type->chk_this_template_generic(nt_templ,
        (incompl_ok ? incomplete_allowed : INCOMPLETE_NOT_ALLOWED),
        OMIT_NOT_ALLOWED, ANY_OR_OMIT_NOT_ALLOWED, SUB_CHK, implicit_omit, lhs);
    }
    break;}
  default:
    t->error("%s cannot be used for union type `%s'",
      t->get_templatetype_str(), get_typename().c_str());
    break;
  }
  if (t->get_length_restriction()) t->error("Length restriction is not "
    "allowed for union type `%s'", get_typename().c_str());

  return self_ref;
}

bool Type::chk_this_template_Seq(Template *t, namedbool incomplete_allowed,
  namedbool implicit_omit, Common::Assignment *lhs)
{
  bool self_ref = false;
  size_t n_type_comps = get_nof_comps();
  switch (t->get_templatetype()) {
  case Ttcn::Template::TEMPLATE_LIST:
    // conversion: value list -> assignment notation
    t->set_templatetype(Ttcn::Template::NAMED_TEMPLATE_LIST);
    // no break
  case Ttcn::Template::NAMED_TEMPLATE_LIST: {
    map<string, Ttcn::NamedTemplate> comp_map;
    // it is set to false if we have lost the ordering
    bool in_synch = true;
    size_t n_template_comps = t->get_nof_comps();
    // the two variables below are used for modified templates only
    CompField *last_cf = 0;
    size_t next_index = 0;
    for (size_t i = 0; i < n_template_comps; i++) {
      Ttcn::NamedTemplate *namedtemp = t->get_namedtemp_byIndex(i);
      const Identifier& temp_id = namedtemp->get_name();
      const string& temp_name = temp_id.get_name();
      const char *temp_dispname_str = temp_id.get_dispname().c_str();
      if (!has_comp_withName(temp_id)) {
        namedtemp->error("Reference to non-existent field `%s' in record "
          "template for type `%s'", temp_dispname_str,
          get_typename().c_str());
        namedtemp->note("%s", actual_fields(*this).c_str());
        in_synch = false;
        continue;
      } else if (comp_map.has_key(temp_name)) {
        namedtemp->error("Duplicate record field `%s' in template",
          temp_dispname_str);
        comp_map[temp_name]->note("Field `%s' is already given here",
          temp_dispname_str);
        in_synch = false;
      } else comp_map.add(temp_name, namedtemp);

      CompField *cf = get_comp_byName(temp_id);
      if (in_synch) {
        if (INCOMPLETE_NOT_ALLOWED != incomplete_allowed) {
          // missing fields are allowed, but take care of ordering
          bool found = false;
          for (size_t j = next_index; j < n_type_comps; j++) {
            CompField *cf2 = get_comp_byIndex(j);
            if (temp_name == cf2->get_name().get_name()) {
              last_cf = cf2;
              next_index = j + 1;
              found = true;
              break;
            }
          }
          if (!found) {
            namedtemp->error("Field `%s' cannot appear after field `%s' in "
              "a template for record type `%s'", temp_dispname_str,
              last_cf->get_name().get_dispname().c_str(),
              get_fullname().c_str());
            in_synch = false;
          }
        } else {
          // the template must be complete
          CompField *cf2 = get_comp_byIndex(i);
          if (cf2 != cf) {
            if (!cf2->get_is_optional() || IMPLICIT_OMIT != implicit_omit) {
              namedtemp->error("Unexpected field `%s' in record template, "
               "expecting `%s'", temp_dispname_str,
               cf2->get_name().get_dispname().c_str());
              in_synch = false;
            }
          }
        }
      }
      Type *type = cf->get_type();
      Template *comp_t = namedtemp->get_template();
      Error_Context cntxt(comp_t, "In template for record field `%s'",
        temp_dispname_str);
      comp_t->set_my_governor(type);
      type->chk_this_template_ref(comp_t);
      bool is_optional = cf->get_is_optional(); // || cf->has_default()
      self_ref |= type->chk_this_template_generic(comp_t, incomplete_allowed,
        (is_optional ? OMIT_ALLOWED : OMIT_NOT_ALLOWED),
        (is_optional ? ANY_OR_OMIT_ALLOWED : ANY_OR_OMIT_NOT_ALLOWED),
        SUB_CHK, implicit_omit, lhs);
    }
    if (INCOMPLETE_ALLOWED != incomplete_allowed  || IMPLICIT_OMIT == implicit_omit) {
      // check missing fields
      for (size_t i = 0; i < n_type_comps; i++) {
        const Identifier& id = get_comp_byIndex(i)->get_name();
        if (!comp_map.has_key(id.get_name())) {
          if (IMPLICIT_OMIT == implicit_omit && get_comp_byIndex(i)->get_is_optional()) {
            // do not overwrite base fields
            if (!t->get_base_template())
              t->add_named_temp(new Ttcn::NamedTemplate(new Identifier(id),
                new Template(Template::OMIT_VALUE)));
          } else if (INCOMPLETE_NOT_ALLOWED == incomplete_allowed) {
            t->error("Field `%s' is missing from template for record type `%s'",
              id.get_dispname().c_str(), get_typename().c_str());
          }
          else if (incomplete_allowed == WARNING_FOR_INCOMPLETE) {
            t->warning("Field `%s' is missing from template for record type `%s'",
              id.get_dispname().c_str(), get_typename().c_str());
          }
        }
      }
    }
    comp_map.clear();
    break; }
  default:
    t->error("%s cannot be used for record type `%s'",
      t->get_templatetype_str(), get_typename().c_str());
    break;
  }
  if (t->get_length_restriction()) t->error("Length restriction is not "
    "allowed for record type `%s'", get_typename().c_str());
  return self_ref;
}

bool Type::chk_this_template_Set(Template *t,
  namedbool incomplete_allowed, namedbool implicit_omit, Common::Assignment *lhs)
{
  bool self_ref = false;
  size_t n_type_comps = get_nof_comps();
  switch (t->get_templatetype()) {
  case Ttcn::Template::TEMPLATE_LIST:
    if (t->get_nof_comps() > 0) {
      t->error("Value list notation is not allowed for set type `%s'",
        get_fullname().c_str());
      break;
    } else if (n_type_comps > 0) {
      t->error("A non-empty set template was expected for type `%s'",
        get_fullname().c_str());
      break;
    }
    t->set_templatetype(Ttcn::Template::NAMED_TEMPLATE_LIST);
    // no break
  case Ttcn::Template::NAMED_TEMPLATE_LIST: {
    map<string, Ttcn::NamedTemplate> comp_map;
    size_t n_template_comps = t->get_nof_comps();
    for (size_t i = 0; i < n_template_comps; i++) {
      Ttcn::NamedTemplate *namedtemp = t->get_namedtemp_byIndex(i);
      const Identifier& temp_id=namedtemp->get_name();
      const string& temp_name = temp_id.get_name();
      const char *temp_dispname_str = temp_id.get_dispname().c_str();
      if (!has_comp_withName(temp_id)) {
        namedtemp->error("Reference to non-existent field `%s' in a "
          "template for set type `%s'", temp_dispname_str,
          get_typename().c_str());
        namedtemp->note("%s", actual_fields(*this).c_str());
        continue;
      } else if (comp_map.has_key(temp_name)) {
        namedtemp->error("Duplicate set field `%s' in template",
          temp_dispname_str);
        comp_map[temp_name]->note("Field `%s' is already given here",
          temp_dispname_str);
      } else comp_map.add(temp_name, namedtemp);
      CompField *cf = get_comp_byName(temp_id);
      Type *type = cf->get_type();
      Template *comp_t = namedtemp->get_template();
      Error_Context cntxt(comp_t, "In template for set field `%s'",
        temp_dispname_str);
      comp_t->set_my_governor(type);
      type->chk_this_template_ref(comp_t);
      bool is_optional = cf->get_is_optional();
      self_ref |= type->chk_this_template_generic(comp_t, incomplete_allowed,
        (is_optional ? OMIT_ALLOWED : OMIT_NOT_ALLOWED),
        (is_optional ? ANY_OR_OMIT_ALLOWED : ANY_OR_OMIT_NOT_ALLOWED),
        SUB_CHK, implicit_omit, lhs);
    }
    if (INCOMPLETE_ALLOWED != incomplete_allowed || IMPLICIT_OMIT == implicit_omit) {
      // check missing fields
      for (size_t i = 0; i < n_type_comps; i++) {
        const Identifier& id = get_comp_byIndex(i)->get_name();
        if (!comp_map.has_key(id.get_name())) {
          if (get_comp_byIndex(i)->get_is_optional() && IMPLICIT_OMIT == implicit_omit) {
            // do not overwrite base fields
        	if (!t->get_base_template())
              t->add_named_temp(new Ttcn::NamedTemplate(new Identifier(id),
                new Template(Template::OMIT_VALUE)));
          } else if (INCOMPLETE_NOT_ALLOWED ==  incomplete_allowed) {
            t->error("Field `%s' is missing from template for set type `%s'",
              id.get_dispname().c_str(), get_typename().c_str());
          }
          else if (incomplete_allowed == WARNING_FOR_INCOMPLETE) {
            t->warning("Field `%s' is missing from template for set type `%s'",
              id.get_dispname().c_str(), get_typename().c_str());
          }
        }
      }
    }
    comp_map.clear();
    break; }
  default:
    t->error("%s cannot be used for set type `%s'",
      t->get_templatetype_str(), get_typename().c_str());
    break;
  }
  if (t->get_length_restriction()) t->error("Length restriction is not "
    "allowed for set type `%s'", get_typename().c_str());
  return self_ref;
}

bool Type::chk_this_template_SeqOf(Template *t, namedbool incomplete_allowed,
  namedbool implicit_omit, Common::Assignment *lhs)
{
  bool self_ref = false;
  switch(t->get_templatetype()) {
  case Ttcn::Template::OMIT_VALUE:
    if(t->get_length_restriction())
      t->warning("Redundant usage of length restriction with `omit'");
    break;
  case Ttcn::Template::PERMUTATION_MATCH: {
    size_t nof_comps = t->get_nof_comps();
    for (size_t i = 0; i < nof_comps; i++) {
      Template *t_comp = t->get_temp_byIndex(i);
      Error_Context cntxt(t_comp, "In element %lu of permutation",
        (unsigned long) (i + 1));
      t_comp->set_my_governor(u.seof.ofType);
      u.seof.ofType->chk_this_template_ref(t_comp);
      // the elements of permutation must be always complete
      self_ref |= u.seof.ofType->chk_this_template_generic(t_comp,
        INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK,
        NOT_IMPLICIT_OMIT, lhs);
    }
    break; }
  case Ttcn::Template::TEMPLATE_LIST: {
    Ttcn::Template::completeness_t c =
      t->get_completeness_condition_seof(INCOMPLETE_NOT_ALLOWED != incomplete_allowed);
    Template *t_base;
    size_t nof_base_comps;
    if (c == Ttcn::Template::C_PARTIAL) {
      t_base = t->get_base_template()->get_template_refd_last();
      // it is sure that t_base is a TEMPLATE_LIST
      nof_base_comps = t_base->get_nof_comps();
    } else {
      t_base = 0;
      nof_base_comps = 0;
    }
    size_t nof_comps = t->get_nof_comps();
    for(size_t i = 0; i < nof_comps; i++) {
      Template *t_comp = t->get_temp_byIndex(i);
      Error_Context cntxt(t_comp, "In component %lu", (unsigned long)(i+1));
      t_comp->set_my_governor(u.seof.ofType);
      if (t_base && i < nof_base_comps)
        t_comp->set_base_template(t_base->get_temp_byIndex(i));
      u.seof.ofType->chk_this_template_ref(t_comp);
      switch (t_comp->get_templatetype()) {
      case Ttcn::Template::PERMUTATION_MATCH:
        // the elements of permutation has to be checked by u.seof.ofType
        // the templates within the permutation always have to be complete
        self_ref |= chk_this_template_generic(t_comp, INCOMPLETE_NOT_ALLOWED,
          OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK,
          implicit_omit, lhs);
        break;
      case Ttcn::Template::TEMPLATE_NOTUSED:
        if (c == Ttcn::Template::C_MUST_COMPLETE) {
          t_comp->error("Not used symbol `-' is not allowed in this context");
        } else if (c == Ttcn::Template::C_PARTIAL && i >= nof_base_comps) {
          t_comp->error("Not used symbol `-' cannot be used here because "
            "there is no corresponding element in the base template");
        }
        else if (incomplete_allowed == WARNING_FOR_INCOMPLETE) {
          t_comp->warning("Not used symbol `-' in record of template");
        }
        break;
      default:
        bool embedded_modified = (c == Ttcn::Template::C_MAY_INCOMPLETE) ||
          (c == Ttcn::Template::C_PARTIAL && i < nof_base_comps);
        self_ref |= u.seof.ofType->chk_this_template_generic(t_comp,
          (embedded_modified ? incomplete_allowed : INCOMPLETE_NOT_ALLOWED),
          OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK, implicit_omit, lhs);
        break;
      }
    }
    break; }
  case Ttcn::Template::INDEXED_TEMPLATE_LIST: {
    map<Int, Int> index_map;
    for (size_t i = 0; i < t->get_nof_comps(); i++) {
      Ttcn::IndexedTemplate *it = t->get_indexedtemp_byIndex(i);
      Value *index_value = (it->get_index()).get_val();
      // The index value must be Type::EXPECTED_DYNAMIC_VALUE integer, but
      // it's not required to be known at compile time.
      index_value->chk_expr_int(Type::EXPECTED_DYNAMIC_VALUE);
      Template *it_comp = it->get_template();
      Error_Context cntxt(it_comp, "In component %lu",
        (unsigned long)(i + 1));
      if (index_value->get_value_refd_last()->get_valuetype() == Value::V_INT) {
        const int_val_t *index_int = index_value->get_value_refd_last()
          ->get_val_Int();
        if (*index_int > INT_MAX) {
          index_value->error("An integer value less than `%d' was expected "
            "for indexing type `%s' instead of `%s'", INT_MAX,
            get_typename().c_str(), (index_int->t_str()).c_str());
          index_value->set_valuetype(Value::V_ERROR);
        } else {
          Int index = index_int->get_val();
          if (index < 0) {
            index_value->error("A non-negative integer value was expected "
              "for indexing type `%s' instead of `%s'",
              get_typename().c_str(), Int2string(index).c_str());
            index_value->set_valuetype(Value::V_ERROR);
          } else if (index_map.has_key(index)) {
            index_value->error("Duplicate index value `%s' for components "
              "`%s' and `%s'", Int2string(index).c_str(),
              Int2string((Int)i + 1).c_str(),
              Int2string(*index_map[index]).c_str());
            index_value->set_valuetype(Value::V_ERROR);
          } else {
            index_map.add(index, new Int((Int)i + 1));
          }
        }
      }
      it_comp->set_my_governor(u.seof.ofType);
      u.seof.ofType->chk_this_template_ref(it_comp);
      self_ref |= u.seof.ofType->chk_this_template_generic(it_comp,
        INCOMPLETE_ALLOWED, OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED,
        SUB_CHK, implicit_omit, lhs);
    }
    for (size_t i = 0; i < index_map.size(); i++)
      delete index_map.get_nth_elem(i);
    index_map.clear();
    break; }
  default:
    t->error("%s cannot be used for `record of' type `%s'",
      t->get_templatetype_str(), get_typename().c_str());
    break;
  }
  return self_ref;
}

bool Type::chk_this_template_SetOf(Template *t, namedbool incomplete_allowed,
  namedbool implicit_omit, Common::Assignment *lhs)
{
  bool self_ref = false;
  Ttcn::Template::templatetype_t temptype = t->get_templatetype();
  switch (temptype) {
  case Ttcn::Template::OMIT_VALUE:
    if(t->get_length_restriction())
      t->warning("Redundant usage of length restriction with `omit'");
    break;
  case Ttcn::Template::SUPERSET_MATCH:
  case Ttcn::Template::SUBSET_MATCH: {
    const char *settype = temptype == Ttcn::Template::SUPERSET_MATCH ?
      "superset" : "subset";
    size_t nof_comps = t->get_nof_comps();
    for (size_t i = 0; i < nof_comps; i++) {
      Template *t_comp = t->get_temp_byIndex(i);
      Error_Context cntxt(t_comp, "In element %lu of %s",
        (unsigned long) (i + 1), settype);
      t_comp->set_my_governor(u.seof.ofType);
      u.seof.ofType->chk_this_template_ref(t_comp);
      // the elements of sets must be always complete
      self_ref |= u.seof.ofType->chk_this_template_generic(t_comp,
        INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED,
        SUB_CHK, implicit_omit, lhs);
      if (t_comp->get_template_refd_last()->get_templatetype() ==
          Ttcn::Template::ANY_OR_OMIT) {
        if (temptype == Ttcn::Template::SUPERSET_MATCH)
          t_comp->warning("`*' in superset has no effect during matching");
        else t_comp->warning("`*' in subset. This template will match "
          "everything");
      }
    }
    break;}
  case Ttcn::Template::TEMPLATE_LIST: {
    Ttcn::Template::completeness_t c =
      t->get_completeness_condition_seof(INCOMPLETE_NOT_ALLOWED != incomplete_allowed);
    Template *t_base;
    size_t nof_base_comps;
    if (c == Ttcn::Template::C_PARTIAL) {
      t_base = t->get_base_template()->get_template_refd_last();
      // it is sure that t_base is a TEMPLATE_LIST
      nof_base_comps = t_base->get_nof_comps();
    } else {
      t_base = 0;
      nof_base_comps = 0;
    }
    size_t nof_comps = t->get_nof_comps();
    for(size_t i = 0; i < nof_comps; i++) {
      Template *t_comp = t->get_temp_byIndex(i);
      Error_Context cntxt(t_comp, "In component %lu", (unsigned long)(i+1));
      t_comp->set_my_governor(u.seof.ofType);
      if (t_base && i < nof_base_comps)
        t_comp->set_base_template(t_base->get_temp_byIndex(i));
      u.seof.ofType->chk_this_template_ref(t_comp);
      switch (t_comp->get_templatetype()) {
      case Ttcn::Template::PERMUTATION_MATCH:
        t_comp->error("%s cannot be used for `set of' type `%s'",
          t_comp->get_templatetype_str(), get_typename().c_str());
        break;
      case Ttcn::Template::TEMPLATE_NOTUSED:
        if (c == Ttcn::Template::C_MUST_COMPLETE) {
          t_comp->error("Not used symbol `-' is not allowed in this context");
        } else if (c == Ttcn::Template::C_PARTIAL && i >= nof_base_comps) {
          t_comp->error("Not used symbol `-' cannot be used here because "
            "there is no corresponding element in the base template");
        }
        else if (incomplete_allowed == WARNING_FOR_INCOMPLETE) {
          t_comp->warning("Not used symbol `-' in set of template");
        }
        break;
      default:
        bool embedded_modified = (c == Ttcn::Template::C_MAY_INCOMPLETE) ||
          (c == Ttcn::Template::C_PARTIAL && i < nof_base_comps);
        self_ref |= u.seof.ofType->chk_this_template_generic(t_comp,
          (embedded_modified ? incomplete_allowed : INCOMPLETE_NOT_ALLOWED),
          OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK, implicit_omit, lhs);
        break;
      }
    }
    break; }
  case Ttcn::Template::INDEXED_TEMPLATE_LIST: {
    map<Int, Int> index_map;
    for (size_t i = 0; i < t->get_nof_comps(); i++) {
      Ttcn::IndexedTemplate *it = t->get_indexedtemp_byIndex(i);
      Value *index_value = (it->get_index()).get_val();
      index_value->chk_expr_int(Type::EXPECTED_DYNAMIC_VALUE);
      Template *it_comp = it->get_template();
      Error_Context cntxt(it_comp, "In component %lu",
        (unsigned long)(i + 1));
      if (index_value->get_value_refd_last()->get_valuetype() == Value::V_INT) {
        const int_val_t *index_int = index_value->get_value_refd_last()
          ->get_val_Int();
        if (*index_int > INT_MAX) {
          index_value->error("An integer value less than `%d' was expected "
            "for indexing type `%s' instead of `%s'", INT_MAX,
            get_typename().c_str(), (index_int->t_str()).c_str());
          index_value->set_valuetype(Value::V_ERROR);
        } else {
          Int index = index_int->get_val();
          if (index < 0) {
            index_value->error("A non-negative integer value was expected "
              "for indexing type `%s' instead of `%s'",
              get_typename().c_str(), Int2string(index).c_str());
            index_value->set_valuetype(Value::V_ERROR);
          } else if (index_map.has_key(index)) {
            index_value->error("Duplicate index value `%s' for components "
              "`%s' and `%s'", Int2string(index).c_str(),
              Int2string((Int)i + 1).c_str(),
              Int2string(*index_map[index]).c_str());
            index_value->set_valuetype(Value::V_ERROR);
          } else {
            index_map.add(index, new Int((Int)i + 1));
          }
        }
      }
      it_comp->set_my_governor(u.seof.ofType);
      u.seof.ofType->chk_this_template_ref(it_comp);
      self_ref |= u.seof.ofType->chk_this_template_generic(it_comp,
        INCOMPLETE_ALLOWED, OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED,
        SUB_CHK, implicit_omit, lhs);
    }
    for (size_t i = 0; i < index_map.size(); i++)
      delete index_map.get_nth_elem(i);
    index_map.clear();
    break; }
  default:
    t->error("%s cannot be used for `set of' type `%s'",
      t->get_templatetype_str(), get_typename().c_str());
    break;
  }
  return self_ref;
}

bool Type::chk_this_template_array(Template *t, namedbool incomplete_allowed,
                                   namedbool implicit_omit, Common::Assignment *lhs)
{
  bool self_ref = false;
  switch (t->get_templatetype()) {
  case Ttcn::Template::OMIT_VALUE:
    if (t->get_length_restriction())
      t->warning("Redundant usage of length restriction with `omit'");
    break;
  case Ttcn::Template::PERMUTATION_MATCH: {
    size_t nof_comps = t->get_nof_comps();
    for (size_t i = 0; i < nof_comps; i++) {
      Template *t_comp = t->get_temp_byIndex(i);
      Error_Context cntxt(t_comp, "In element %lu of permutation",
        (unsigned long) (i + 1));
      t_comp->set_my_governor(u.array.element_type);
      u.array.element_type->chk_this_template_ref(t_comp);
      // the elements of permutation must be always complete
      self_ref |= u.array.element_type->chk_this_template_generic(t_comp,
        INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK,
        NOT_IMPLICIT_OMIT, lhs);
    }
    break; }
  case Ttcn::Template::TEMPLATE_LIST: {
    Template *t_base = t->get_base_template();
    size_t nof_base_comps = 0;
    if (t_base) {
      t_base = t_base->get_template_refd_last();
      if (t_base->get_templatetype() == Ttcn::Template::TEMPLATE_LIST)
        nof_base_comps = t_base->get_nof_comps();
      else t_base = 0; // error recovery
    }
    if (!u.array.dimension->get_has_error()) {
      size_t array_size = u.array.dimension->get_size();
      size_t template_size = t->get_nof_listitems();
      if (array_size != template_size) {
        if (t->is_flattened()){
          t->error("Too %s elements in the array template: %lu was expected "
                   "instead of %lu",
                   array_size > template_size ? "few" : "many",
                   (unsigned long)array_size, (unsigned long)template_size);
        }
        else {
          t->warning("The size of template cannot be resolved "
                     "so it could not be compared to the array size");
        }
      }
    }
    size_t nof_comps = t->get_nof_comps();
    for (size_t i = 0; i < nof_comps; i++) {
      Template *t_comp = t->get_temp_byIndex(i);
      Error_Context cntxt(t_comp, "In array element %lu",
                          (unsigned long)(i + 1));
      t_comp->set_my_governor(u.array.element_type);
      if (t_base && i < nof_base_comps)
        t_comp->set_base_template(t_base->get_temp_byIndex(i));
      u.array.element_type->chk_this_template_ref(t_comp);
      switch (t_comp->get_templatetype()) {
        case Ttcn::Template::PERMUTATION_MATCH:
          // the elements of permutation has to be checked by u.seof.ofType
          // the templates within the permutation always have to be complete
          self_ref |= chk_this_template_generic(t_comp, INCOMPLETE_NOT_ALLOWED,
            OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK,
            implicit_omit, lhs);
          break;
        case Ttcn::Template::TEMPLATE_NOTUSED:
          if (INCOMPLETE_NOT_ALLOWED == incomplete_allowed)
            t_comp->error("Not used symbol `-' is not allowed in this "
                          "context");
          else if (incomplete_allowed == WARNING_FOR_INCOMPLETE) {
            t_comp->warning("Not used symbol `-' in array template");
          }
          break;
        default:
            self_ref |= u.array.element_type->chk_this_template_generic(t_comp,
            incomplete_allowed, OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK, implicit_omit, lhs);
          break;
      }
    }
    break; }
  case Ttcn::Template::INDEXED_TEMPLATE_LIST: {
    map<Int, Int> index_map;
    for (size_t i = 0; i < t->get_nof_comps(); i++) {
      Ttcn::IndexedTemplate *it = t->get_indexedtemp_byIndex(i);
      Value *index_value = (it->get_index()).get_val();
      u.array.dimension->chk_index(index_value,
                                   Type::EXPECTED_DYNAMIC_VALUE);
      Template *it_comp = it->get_template();
      Error_Context cntxt(it_comp, "In component %lu",
                          (unsigned long)(i + 1));
      if (index_value->get_value_refd_last()
          ->get_valuetype() == Value::V_INT) {
        const int_val_t *index_int = index_value->get_value_refd_last()
                                     ->get_val_Int();
        if (*index_int > INT_MAX) {
          index_value->error("An integer value less than `%d' was expected "
                             "for indexing type `%s' instead of `%s'",
                             INT_MAX, get_typename().c_str(),
                             (index_int->t_str()).c_str());
          index_value->set_valuetype(Value::V_ERROR);
        } else {
          Int index = index_int->get_val();
          if (index_map.has_key(index)) {
            index_value->error("Duplicate index value `%s' for components "
                               "`%s' and `%s'", Int2string(index).c_str(),
                               Int2string((Int)i + 1).c_str(),
                               Int2string(*index_map[index]).c_str());
            index_value->set_valuetype(Value::V_ERROR);
          } else {
            index_map.add(index, new Int((Int)i + 1));
          }
        }
      }
      it_comp->set_my_governor(u.array.element_type);
      u.array.element_type->chk_this_template_ref(it_comp);
      self_ref |= u.array.element_type->chk_this_template_generic(it_comp,
        incomplete_allowed, OMIT_NOT_ALLOWED, ANY_OR_OMIT_ALLOWED, SUB_CHK, implicit_omit, lhs);
    }
    for (size_t i = 0; i < index_map.size(); i++)
      delete index_map.get_nth_elem(i);
    index_map.clear();
    break; }
  default:
    t->error("%s cannot be used for array type `%s'",
             t->get_templatetype_str(), get_typename().c_str());
    break;
  }
  return self_ref;
}

void Type::chk_this_template_Fat(Template *t)
{
  t->error("%s cannot be used for type `%s'",t->get_templatetype_str(),
    get_typename().c_str());
  if(t->get_length_restriction()) t->error("Length restriction is not "
    "allowed for type `%s'", get_typename().c_str());
}

void Type::chk_this_template_Signature(Template *t, namedbool incomplete_allowed)
{
  bool self_ref = false;
  size_t n_type_params = get_nof_comps();
  switch (t->get_templatetype()) {
  case Ttcn::Template::TEMPLATE_LIST:
    // conversion: value list -> assignment notation
    t->set_templatetype(Ttcn::Template::NAMED_TEMPLATE_LIST);
    // no break
  case Ttcn::Template::NAMED_TEMPLATE_LIST: {
    map<string, Ttcn::NamedTemplate> comp_map;
    // it is set to false if we have lost the ordering
    bool in_synch = true;
    size_t n_template_comps = t->get_nof_comps();
    size_t t_i = 0;
    for (size_t v_i = 0; v_i < n_template_comps; v_i++) {
      Ttcn::NamedTemplate *nt = t->get_namedtemp_byIndex(v_i);
      const Identifier& temp_id = nt->get_name();
      const string& temp_name = temp_id.get_name();
      const char *temp_dispname_str = temp_id.get_dispname().c_str();
      if (!has_comp_withName(temp_id)) {
        nt->error("Reference to non-existent parameter `%s' in template "
          "for signature `%s'", temp_dispname_str, get_typename().c_str());
        in_synch = false;
        continue;
      } else if (comp_map.has_key(temp_name)) {
        nt->error("Duplicate parameter `%s' in template for signature `%s'",
          temp_dispname_str, get_typename().c_str());
        comp_map[temp_name]->note("Parameter `%s' is already given here",
          temp_dispname_str);
        in_synch = false;
      } else comp_map.add(temp_name, nt);
      const SignatureParam *par =
        u.signature.parameters->get_param_byName(temp_id);
      if (in_synch) {
        SignatureParam *par2 = 0;
        for ( ; t_i < n_type_params; t_i++) {
          par2 = u.signature.parameters->get_param_byIndex(t_i);
          if (par2 == par) break;
        }
        if (par2 != par) {
          nt->error("Unexpected parameter `%s' in signature template",
              temp_dispname_str);
          in_synch = false;
        } else t_i++;
      }
      Type *type = par->get_type();
      Template *comp_t = nt->get_template();
      Error_Context cntxt(comp_t, "In template for signature parameter `%s'",
        temp_dispname_str);
      comp_t->set_my_governor(type);
      type->chk_this_template_ref(comp_t);
      self_ref |= type->chk_this_template_generic(comp_t, incomplete_allowed,
        OMIT_NOT_ALLOWED, ANY_OR_OMIT_NOT_ALLOWED,
        SUB_CHK, NOT_IMPLICIT_OMIT, NULL);
    }
    if(incomplete_allowed != INCOMPLETE_ALLOWED) {
      SignatureParam *first_undef_in = NULL,
                      *first_undef_out = NULL;
      for (size_t i = 0; i < n_type_params; i++) {
        SignatureParam *par = u.signature.parameters->get_param_byIndex(i);
        const Identifier& id = par->get_id();
        if (!comp_map.has_key(id.get_name()) ||
          comp_map[id.get_name()]->get_template()->get_templatetype() ==
          Template::TEMPLATE_NOTUSED) {
          switch(par->get_direction()) {
          case SignatureParam::PARAM_IN:
            if(!first_undef_in) first_undef_in = par;
            break;
          case SignatureParam::PARAM_OUT:
            if(!first_undef_out) first_undef_out = par;
            break;
          default: //inout
            if (INCOMPLETE_NOT_ALLOWED == incomplete_allowed) {
              t->error("Signature template is incomplete, because the inout "
                "parameter `%s' is missing", id.get_dispname().c_str());
            }
            else {
              t->warning("Signature template is incomplete, because the inout "
                "parameter `%s' is missing", id.get_dispname().c_str());
            }
          }
        }
      }
      if(first_undef_in!=NULL && first_undef_out!=NULL) {
        if (INCOMPLETE_NOT_ALLOWED == incomplete_allowed) {
          t->error("Signature template is incomplete, because the in parameter "
            "`%s' and the out parameter `%s' are missing",
            first_undef_in->get_id().get_dispname().c_str(),
            first_undef_out->get_id().get_dispname().c_str());
        }
        else {
          t->warning("Signature template is incomplete, because the in parameter "
            "`%s' and the out parameter `%s' are missing",
            first_undef_in->get_id().get_dispname().c_str(),
            first_undef_out->get_id().get_dispname().c_str());
        }
      }
    }
    comp_map.clear();
    break;}
  default:
    t->error("%s cannot be used for signature `%s'",
      t->get_templatetype_str(), get_typename().c_str());
    break;
  }
  if (t->get_length_restriction())
    t->error("Length restriction is not allowed in a template for "
      "signature `%s'", get_typename().c_str());
}

} // namespace Common
