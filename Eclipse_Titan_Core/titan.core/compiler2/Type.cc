/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Beres, Szabolcs
 *   Bibo, Zoltan
 *   Cserveni, Akos
 *   Delic, Adam
 *   Dimitrov, Peter
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Nagy, Lenard
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szabo, Bence Janos
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "../common/dbgnew.hh"
#include "Type.hh"
#include <ctype.h>
#include "Typestuff.hh" // FIXME CTs
#include "CompType.hh"
#include "TypeCompat.hh"
#include "CompField.hh"
#include "SigParam.hh"
#include "EnumItem.hh"

#include "Valuestuff.hh"
#include "ttcn3/ArrayDimensions.hh"
#include "asn1/Tag.hh"
#include "asn1/Block.hh"
#include "asn1/Ref.hh"
#include "Constraint.hh"
#include "main.hh"
#include "../common/pattern.hh"
#include "ttcn3/Attributes.hh"
#include "XerAttributes.hh"
#include "ttcn3/Ttcnstuff.hh"
#include "ttcn3/TtcnTemplate.hh"
#include "ttcn3/Templatestuff.hh"

#include "../common/static_check.h"
#include "PredefFunc.hh"

// implemented in coding_attrib_p.y
extern Ttcn::ExtensionAttributes * parse_extattributes(
  Ttcn::WithAttribPath *w_attrib_path);

namespace Common {

  using Ttcn::MultiWithAttrib;
  using Ttcn::SingleWithAttrib;
  using Ttcn::WithAttribPath;

  const char* Type::type_as_string[] = {
  "undefined",                                   // T_UNDEF
  "erroneous",                                   // T_ERROR
  "null(ASN)",                                   // T_NULL
  "boolean",                                     // T_BOOL
  "integer",                                     // T_INT
  "integer(ASN.1)",                              // T_INT_A
  "real/float",                                  // T_REAL
  "enumerated(ASN.1)",                           // T_ENUM_A
  "enumerated(TTCN-3)",                          // T_ENUM_T
  "bitstring",                                   // T_BSTR
  "bitstring(ASN)",                              // T_BSTR_A
  "hexstring(TTCN-3)",                           // T_HSTR
  "octetstring",                                 // T_OSTR
  "charstring (TTCN-3)",                         // T_CSTR
  "universal charstring(TTCN-3)",                // T_USTR
  "UTF8String(ASN.1)",                           // T_UTF8STRING
  "NumericString(ASN.1)",                        // T_NUMERICSTRING
  "PrintableString(ASN.1)",                      // T_PRINTABLESTRING
  "TeletexString(ASN.1)",                        //T_TELETEXSTRING
  "VideotexString(ASN.1)",                       // T_VIDEOTEXSTRING
  "IA5String(ASN.1)",                            // T_IA5STRING
  "GraphicString(ASN.1)",                        // T_GRAPHICSTRING,
  "VisibleString(ASN.1)",                        // T_VISIBLESTRING
  "GeneralString (ASN.1)",                       // T_GENERALSTRING
  "UniversalString (ASN.1)",                     // T_UNIVERSALSTRING
  "BMPString (ASN.1)",                           // T_BMPSTRING
  "UnrestrictedCharacterString(ASN.1)",          // T_UNRESTRICTEDSTRING
  "UTCTime(ASN.1)",                              // T_UTCTIME
  "GeneralizedTime(ASN.1)",                      // T_GENERALIZEDTIME
  "Object descriptor, a kind of string (ASN.1)", // T_OBJECTDESCRIPTOR
  "object identifier",                           // T_OID
  "relative OID(ASN.1)",                         // T_ROID
  "choice(ASN-1)",                               // T_CHOICE_A
  "union(TTCN-3)",                               // T_CHOICE_T
  "sequence (record) of",                        // T_SEQOF
  "set of",                                      // T_SETOF
  "sequence(ASN-1)",                             // T_SEQ_A
  "record(TTCN-3)",                              // T_SEQ_T
  "set(ASN.1)",                                  // T_SET_A
  "set(TTCN-3)",                                 // T_SET_T
  "ObjectClassFieldType(ASN.1)",                 // T_OCFT
  "open type(ASN.1)",                            // T_OPENTYPE
  "ANY(deprecated ASN.1)",                       // T_ANY
  "external(ASN.1)",                             // T_EXTERNAL
  "embedded PDV(ASN.1)",                         // T_EMBEDDED_PDV
  "referenced",                                  // T_REFD
  "special referenced(by pointer, not by name)", // T_REFDSPEC
  "selection type(ASN.1)",                       // T_SELTYPE
  "verdict type(TTCN-3)",                        // T_VERDICT
  "port type(TTCN-3)",                           // T_PORT
  "component type(TTCN-3)",                      // T_COMPONENT
  "address type(TTCN-3)",                        // T_ADDRESS
  "default type (TTCN-3)",                       // T_DEFAULT
  "array(TTCN-3)",                               // T_ARRAY
  "signature(TTCN-3)",                           // T_SIGNATURE
  "function reference(TTCN-3)",                  // T_FUNCTION
  "altstep reference(TTCN-3)",                   // T_ALTSTEP
  "testcase reference(TTCN-3)",                  // T_TESTCASE
  "anytype(TTCN-3)",                             // T_ANYTYPE
  };

  // =================================
  // ===== Type
  // =================================
  const char* Type::asString() const {
    if (this->get_typetype() < Type::T_LAST && Type::T_UNDEF < this->get_typetype()) {
      return type_as_string[this->get_typetype()];
    }
    else {
      return type_as_string[Type::T_UNDEF];
    }
  }

  const char* Type::asString(Type::typetype_t type) {
    if (type < Type::T_LAST && Type::T_UNDEF < type) {
      return type_as_string[type];
    }
    else {
      return type_as_string[Type::T_UNDEF];
    }
  }

  // Used by dump() for user-readable messages, by Def_ExtFunction::generate_*
  // The text returned must match the case label without the "CT_" !
  const char *Type::get_encoding_name(MessageEncodingType_t encoding_type)
  {
    ENSURE_EQUAL(Type::T_UNDEF, 0);
    ENSURE_EQUAL(Type::OT_UNKNOWN, 0);
    switch (encoding_type) {
    case CT_BER:
      return "BER";
    case CT_PER:
      return "PER";
    case CT_XER:
      return "XER";
    case CT_RAW:
      return "RAW";
    case CT_TEXT:
      return "TEXT";
    case CT_JSON:
      return "JSON";
    case CT_CUSTOM:
      return "custom";
    default:
      return "<unknown encoding>";
    }
  }

  Type *Type::get_stream_type(MessageEncodingType_t encoding_type, int stream_variant)
  {
    if (stream_variant == 0) {
      return get_pooltype(T_BSTR);
    }
    switch (encoding_type) {
    case CT_BER:
    case CT_PER:
    case CT_RAW:
    case CT_XER: // UTF-8 doesn't fit into charstring and universal is wasteful
    case CT_JSON:
      return get_pooltype(T_OSTR);
    case CT_TEXT:
      if(stream_variant==1){
        return get_pooltype(T_CSTR);
      } else {
        return get_pooltype(T_OSTR);
      }
    default:
      FATAL_ERROR("Type::get_stream_type()");
      return 0;
    }
  }

  map<Type::typetype_t, Type> *Type::pooltypes = 0;

  Type* Type::get_pooltype(typetype_t p_typetype)
  {
    p_typetype=get_typetype_ttcn3(p_typetype);
    switch(p_typetype) {
    case T_NULL:
    case T_BOOL:
    case T_INT:
    case T_REAL:
    case T_BSTR:
    case T_HSTR:
    case T_OSTR:
    case T_CSTR:
    case T_USTR:
    case T_OID:
    case T_ERROR:
    case T_VERDICT:
    case T_COMPONENT:
    case T_DEFAULT:
      break; // we have a pool type
    default:
      return 0; // no pool type for you!
    } // switch
    if (!pooltypes) pooltypes = new map<typetype_t, Type>; // lazy init
    else if (pooltypes->has_key(p_typetype)) return (*pooltypes)[p_typetype];
    Type *t;
    if (p_typetype == T_COMPONENT)
      t = new Type(T_COMPONENT, new ComponentTypeBody());
    else t = new Type(p_typetype);
    t->ownertype = OT_POOL;
    pooltypes->add(p_typetype, t);
    return t;
  }

  void Type::destroy_pooltypes()
  {
    if(pooltypes) {
      for(size_t i=0; i<pooltypes->size(); i++)
        delete pooltypes->get_nth_elem(i);
      pooltypes->clear();
      delete pooltypes;
      pooltypes=0;
    }
  }

  Tag *Type::get_default_tag()
  {
    typetype_t t_typetype;
    switch (typetype) {
    case T_INT:
      t_typetype = T_INT_A;
      break;
    case T_BSTR:
      t_typetype = T_BSTR_A;
      break;
    case T_ENUM_T:
      t_typetype = T_ENUM_A;
      break;
    case T_SEQ_T:
    case T_SEQOF:
      t_typetype = T_SEQ_A;
      break;
    case T_SET_T:
    case T_SETOF:
      t_typetype = T_SET_A;
      break;
    case T_OPENTYPE:
      t_typetype = T_ANY;
      break;
    case T_REFD:
    case T_REFDSPEC:
    case T_SELTYPE:
    case T_OCFT:
      return get_type_refd()->get_tag();
    default:
      t_typetype = typetype;
      break;
    }
    if (!default_tags) default_tags = new map<typetype_t, Tag>;
    else if (default_tags->has_key(t_typetype))
      return (*default_tags)[t_typetype];
    Tag *tag;
    switch (t_typetype) {
    case T_ANY:
      tag = new Tag(Tag::TAG_EXPLICIT, Tag::TAG_ALL, (Int)0);
      break;
    case T_ERROR:
      tag = new Tag(Tag::TAG_EXPLICIT, Tag::TAG_ERROR, (Int)0);
      break;
    default: {
      int tagnumber = get_default_tagnumber(t_typetype);
      if (tagnumber < 0) FATAL_ERROR ("Type::get_default_tag():type `%s' "
        "does not have default tag", get_typename().c_str());
      tag = new Tag(Tag::TAG_EXPLICIT, Tag::TAG_UNIVERSAL, Int(tagnumber));
      break; }
    }
    default_tags->add(t_typetype, tag);
    return tag;
  }

  int Type::get_default_tagnumber(typetype_t p_tt)
  {
    switch (p_tt) {
    // note: tag number 0 is reserved for internal use
    case T_BOOL:
      return 1;
    case T_INT_A:
      return 2;
    case T_BSTR_A:
      return 3;
    case T_OSTR:
      return 4;
    case T_NULL:
      return 5;
    case T_OID:
      return 6;
    case T_OBJECTDESCRIPTOR:
      return 7;
    case T_EXTERNAL:
      return 8;
    case T_REAL:
      return 9;
    case T_ENUM_A:
      return 10;
    case T_EMBEDDED_PDV:
      return 11;
    case T_UTF8STRING:
      return 12;
    case T_ROID:
      return 13;
    // note: tag numbers 14 and 15 are reserved for future use
    case T_SEQ_A:
      return 16;
    case T_SET_A:
      return 17;
    case T_NUMERICSTRING:
      return 18;
    case T_PRINTABLESTRING:
      return 19;
    case T_TELETEXSTRING:
      return 20;
    case T_VIDEOTEXSTRING:
      return 21;
    case T_IA5STRING:
      return 22;
    case T_UTCTIME:
      return 23;
    case T_GENERALIZEDTIME:
      return 24;
    case T_GRAPHICSTRING:
      return 25;
    case T_VISIBLESTRING:
      return 26;
    case T_GENERALSTRING:
      return 27;
    case T_UNIVERSALSTRING:
      return 28;
    case T_UNRESTRICTEDSTRING:
      return 29;
    case T_BMPSTRING:
      return 30;
    default:
      return -1;
    }
  }

  map<Type::typetype_t, Tag> *Type::default_tags = 0;

  void Type::destroy_default_tags()
  {
    if (default_tags) {
      size_t nof_tags = default_tags->size();
      for (size_t i = 0; i < nof_tags; i++)
        delete default_tags->get_nth_elem(i);
      default_tags->clear();
      delete default_tags;
      default_tags = 0;
    }
  }

  Type::Type(const Type& p)
    : Governor(p), typetype(p.typetype)
  {
    init();
    if (p.w_attrib_path != NULL) FATAL_ERROR("Type::Type()");
    tags=p.tags?p.tags->clone():0;
    if(p.constraints) {
      constraints=p.constraints->clone();
      constraints->set_my_type(this);
    }
    else constraints=0;
    if(p.parsed_restr!=NULL) {
      parsed_restr=new vector<SubTypeParse>;
      for(size_t i=0;i<p.parsed_restr->size();i++) {
        SubTypeParse *stp = 0;
        switch((*p.parsed_restr)[i]->get_selection()) {
        case SubTypeParse::STP_SINGLE:
          stp=new SubTypeParse((*p.parsed_restr)[i]->Single());
          break;
        case SubTypeParse::STP_RANGE:
          stp=new SubTypeParse((*p.parsed_restr)[i]->Min(),
            (*p.parsed_restr)[i]->MinExclusive(),
            (*p.parsed_restr)[i]->Max(),
            (*p.parsed_restr)[i]->MaxExclusive());
          break;
        case SubTypeParse::STP_LENGTH:
          FATAL_ERROR("Type::Type(Type&): STP_LENGTH");
          break;
        default: FATAL_ERROR("Type::Type()");
        }
        parsed_restr->add(stp);
      }
    }
    else parsed_restr=0;
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
    case T_BSTR_A:
      u.namednums.block=p.u.namednums.block?p.u.namednums.block->clone():0;
      u.namednums.nvs=p.u.namednums.nvs?p.u.namednums.nvs->clone():0;
      break;
    case T_ENUM_A:
      u.enums.block=p.u.enums.block?p.u.enums.block->clone():0;
      u.enums.eis1=p.u.enums.eis1?p.u.enums.eis1->clone():0;
      u.enums.ellipsis=p.u.enums.ellipsis;
      u.enums.excSpec=p.u.enums.excSpec?p.u.enums.excSpec->clone():0;
      u.enums.eis2=p.u.enums.eis2?p.u.enums.eis2->clone():0;
      // no break
    case T_ENUM_T:
      u.enums.eis=p.u.enums.eis->clone();
      u.enums.eis_by_name=0;
      break;
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_ANYTYPE:
      u.secho.cfm=p.u.secho.cfm->clone();
      u.secho.field_by_name = 0;
      u.secho.component_internal = false;
      u.secho.has_single_charenc = false;
      break;
    case T_SEQ_A:
    case T_SET_A:
      u.secho.tr_compsof_ready=p.u.secho.tr_compsof_ready;
      // no break
    case T_CHOICE_A:
      u.secho.cfm = 0;
      u.secho.block=p.u.secho.block?p.u.secho.block->clone():0;
      u.secho.ctss=p.u.secho.ctss?p.u.secho.ctss->clone():0;
      u.secho.field_by_name = 0;
      u.secho.component_internal = false;
      u.secho.has_single_charenc = false;
      break;
    case T_SEQOF:
    case T_SETOF:
      u.seof.ofType=p.u.seof.ofType->clone();
      u.seof.component_internal = false;
      break;
    case T_REFD:
      u.ref.ref=p.u.ref.ref->clone();
      u.ref.type_refd=0;
      u.ref.component_internal = false;
      break;
    case T_OCFT:
      u.ref.oc_defn=p.u.ref.oc_defn;
      u.ref.oc_fieldname=p.u.ref.oc_fieldname;
      // no break
    case T_REFDSPEC:
      u.ref.type_refd=p.u.ref.type_refd;
      u.ref.component_internal = false;
      break;
    case T_SELTYPE:
      u.seltype.id=p.u.seltype.id->clone();
      u.seltype.type=p.u.seltype.type->clone();
      u.seltype.type_refd=0;
      break;
    case T_OPENTYPE:
      u.secho.cfm=new CompFieldMap();
      u.secho.cfm->set_my_type(this);
      u.secho.oc_defn=p.u.secho.oc_defn;
      u.secho.oc_fieldname=p.u.secho.oc_fieldname;
      u.secho.my_tableconstraint=0;
      u.secho.field_by_name = 0;
      u.secho.component_internal = false;
      u.secho.has_single_charenc = false;
      break;
    case T_ARRAY:
      u.array.element_type=p.u.array.element_type->clone();
      u.array.dimension = p.u.array.dimension->clone();
      u.array.in_typedef = p.u.array.in_typedef;
      u.array.component_internal = false;
      break;
    case T_PORT:
      u.port = p.u.port->clone();
      break;
    case T_COMPONENT:
      u.component = p.u.component->clone();
      break;
    case T_ADDRESS:
      u.address = 0;
      break;
    case T_SIGNATURE:
      u.signature.parameters = p.u.signature.parameters ?
        p.u.signature.parameters->clone() : 0;
      u.signature.return_type = p.u.signature.return_type ?
        p.u.signature.return_type->clone() : 0;
      u.signature.no_block = p.u.signature.no_block;
      u.signature.exceptions = p.u.signature.exceptions ?
        p.u.signature.exceptions->clone() : 0;
      u.signature.component_internal = false;
      break;
    case T_FUNCTION:
    case T_ALTSTEP:
      u.fatref.fp_list = p.u.fatref.fp_list->clone();
      u.fatref.runs_on.ref = p.u.fatref.runs_on.ref ?
        p.u.fatref.runs_on.ref->clone() : 0;
      u.fatref.runs_on.self = p.u.fatref.runs_on.self;
      u.fatref.runs_on.type = 0;
      u.fatref.return_type = p.u.fatref.return_type ?
        p.u.fatref.return_type->clone() : 0;
      u.fatref.is_startable = false;
      u.fatref.returns_template = p.u.fatref.returns_template;
      u.fatref.template_restriction = p.u.fatref.template_restriction;
      break;
    case T_TESTCASE:
      u.fatref.fp_list = p.u.fatref.fp_list->clone();
      u.fatref.runs_on.ref = p.u.fatref.runs_on.ref ?
        p.u.fatref.runs_on.ref->clone() : 0;
      u.fatref.runs_on.self = false;
      u.fatref.runs_on.type = 0;
      u.fatref.system.ref = p.u.fatref.system.ref ?
          p.u.fatref.system.ref->clone() : 0;
      u.fatref.system.type = 0;
      u.fatref.is_startable = false;
      u.fatref.returns_template = false;
      u.fatref.template_restriction = TR_NONE;
      break;
    default:
      FATAL_ERROR("Type::Type()");
    } // switch
  }

  void Type::init()
  {
    tags_checked = false;
    tbl_cons_checked = false;
    text_checked = false;
    json_checked = false;
    raw_parsed  = false;
    raw_checked = false;
    xer_checked = false;
    raw_length_calculated = false;
    has_opentypes = false;
    opentype_outermost = false;
    code_generated = false;
    embed_values_possible = false;
    use_nil_possible = false;
    use_order_possible = false;
    raw_length = -1;
    parent_type = 0;
    tags = 0;
    constraints = 0;
    w_attrib_path = 0;
    encode_attrib_path = 0;
    rawattrib = 0;
    textattrib = 0;
    xerattrib = 0;
    berattrib = 0;
    jsonattrib = 0;
    sub_type = 0;
    parsed_restr = 0;
    ownertype = OT_UNKNOWN;
    owner = 0;
    chk_finished = false;
    pard_type_instance = false;
    needs_any_from_done = false;
    default_encoding.type = CODING_UNSET;
    default_decoding.type = CODING_UNSET;
  }

  void Type::clean_up()
  {
    switch (typetype) {
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
    case T_REFDSPEC:
    case T_OCFT:
    case T_VERDICT:
    case T_ADDRESS:
    case T_DEFAULT:
      break;
    case T_INT_A:
    case T_BSTR_A:
      delete u.namednums.block;
      delete u.namednums.nvs;
      break;
    case T_ENUM_A:
      delete u.enums.block;
      if(u.enums.eis1) {
        u.enums.eis1->release_eis();
        delete u.enums.eis1;
      }
      if(u.enums.eis2) {
        u.enums.eis2->release_eis();
        delete u.enums.eis2;
      }
      /* no break */
    case T_ENUM_T:
      delete u.enums.eis;
      if (u.enums.eis_by_name) {
        for (size_t a = 0; a < u.enums.eis_by_name->size(); a++) {
          delete u.enums.eis_by_name->get_nth_elem(a);
        }
        u.enums.eis_by_name->clear();
        delete u.enums.eis_by_name;
      }
      break;
    case T_CHOICE_A:
    case T_SEQ_A:
    case T_SET_A:
      delete u.secho.block;
      delete u.secho.ctss;
      /* no break */
    case T_ANYTYPE:
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_OPENTYPE:
      delete u.secho.cfm;
      if (u.secho.field_by_name) {
        for(size_t a = 0; a < u.secho.field_by_name->size(); a++) {
          delete u.secho.field_by_name->get_nth_elem(a);
        }
        u.secho.field_by_name->clear();
        delete u.secho.field_by_name;
      }
      break;
    case T_SEQOF:
    case T_SETOF:
      delete u.seof.ofType;
      break;
    case T_REFD:
      delete u.ref.ref;
      break;
    case T_SELTYPE:
      delete u.seltype.id;
      delete u.seltype.type;
      break;
    case T_ARRAY:
      delete u.array.element_type;
      delete u.array.dimension;
      break;
    case T_PORT:
      delete u.port;
      break;
    case T_COMPONENT:
      delete u.component;
      break;
    case T_SIGNATURE:
      delete u.signature.parameters;
      delete u.signature.return_type;
      delete u.signature.exceptions;
      break;
    case T_FUNCTION:
    case T_ALTSTEP:
      delete u.fatref.fp_list;
      delete u.fatref.runs_on.ref;
      delete u.fatref.return_type;
      break;
    case T_TESTCASE:
      delete u.fatref.fp_list;
      delete u.fatref.runs_on.ref;
      delete u.fatref.system.ref;
      break;
    default:
      FATAL_ERROR("Type::clean_up()");
    } // switch
    typetype = T_ERROR;
    delete tags;
    tags = 0;
    delete constraints;
    constraints = 0;
    delete rawattrib;
    rawattrib = 0;
    delete textattrib;
    textattrib = 0;
    delete xerattrib;
    xerattrib = 0;
    delete sub_type;
    sub_type = 0;
    delete berattrib;
    berattrib = 0;
    delete jsonattrib;
    jsonattrib = 0;
    if (parsed_restr) {
      for (size_t i = 0; i < parsed_restr->size(); i++)
        delete (*parsed_restr)[i];
      parsed_restr->clear();
      delete parsed_restr;
      parsed_restr = 0;
    }
    delete w_attrib_path;
    w_attrib_path = 0;
    delete encode_attrib_path;
    encode_attrib_path = 0;
  }

  Type::Type(typetype_t p_tt)
    : Governor(S_T), typetype(p_tt)
  {
    init();
    switch(p_tt) {
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
    case T_ANYTYPE: {
      u.secho.cfm = new CompFieldMap;
      u.secho.cfm->set_my_type(this);
      u.secho.block=0;
      u.secho.ctss=0;
      u.secho.field_by_name = 0;
      u.secho.component_internal = false;
      u.secho.has_single_charenc = false;
      break; }
    case T_INT_A:
    case T_BSTR_A:
      u.namednums.block=0;
      u.namednums.nvs=0;
      break;
    case T_ADDRESS:
      u.address = 0;
      break;
    default:
      FATAL_ERROR("Type::Type()");
    } // switch
  }

  Type::Type(typetype_t p_tt, EnumItems *p_eis)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_ENUM_T || !p_eis) FATAL_ERROR("Type::Type()");
    init();
    u.enums.eis=p_eis;
    u.enums.eis_by_name=0;
  }

  Type::Type(typetype_t p_tt, Block *p_block)
    : Governor(S_T), typetype(p_tt)
  {
    if (!p_block) FATAL_ERROR("NULL parameter");
    init();
    switch(p_tt) {
    case T_INT_A:
    case T_BSTR_A:
      u.namednums.block=p_block;
      u.namednums.nvs=0;
      break;
    case T_ENUM_A:
      u.enums.eis=new EnumItems();
      u.enums.block=p_block;
      u.enums.eis1=0;
      u.enums.ellipsis=false;
      u.enums.excSpec=0;
      u.enums.eis2=0;
      u.enums.eis_by_name=0;
      break;
    case T_SEQ_A:
    case T_SET_A:
      u.secho.tr_compsof_ready=false;
      // no break
    case T_CHOICE_A:
      u.secho.cfm = 0;
      u.secho.block=p_block;
      u.secho.ctss=0;
      u.secho.field_by_name = 0;
      u.secho.component_internal = false;
      u.secho.has_single_charenc = false;
      break;
    default:
      FATAL_ERROR("Type::Type()");
    } // switch
  }

  Type::Type(typetype_t p_tt,
             EnumItems *p_eis1, bool p_ellipsis, EnumItems *p_eis2)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_ENUM_A || !p_eis1) FATAL_ERROR("Type::Type()");
    init();
    u.enums.eis=new EnumItems();
    u.enums.block=0;
    u.enums.eis1=p_eis1;
    u.enums.ellipsis=p_ellipsis;
    u.enums.eis2=p_eis2;
    u.enums.eis_by_name=0;
  }

  Type::Type(typetype_t p_tt, CompFieldMap *p_cfm)
    : Governor(S_T), typetype(p_tt)
  {
    if (!p_cfm) FATAL_ERROR("NULL parameter");
    init();
    switch (p_tt) {
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
      u.secho.cfm=p_cfm;
      u.secho.field_by_name = 0;
      u.secho.component_internal = false;
      u.secho.has_single_charenc = false;
      break;
    default:
      FATAL_ERROR("Type::Type()");
    } // switch
  }

  Type::Type(typetype_t p_tt, Type *p_type)
    : Governor(S_T), typetype(p_tt)
  {
    if (!p_type) FATAL_ERROR("NULL parameter");
    init();
    switch (p_tt) {
    case T_SEQOF:
    case T_SETOF:
      u.seof.ofType=p_type;
      u.seof.ofType->set_ownertype(OT_RECORD_OF, this);
      u.seof.component_internal = false;
      break;
    case T_REFDSPEC:
      u.ref.type_refd=p_type;
      u.ref.type_refd->set_ownertype(OT_REF_SPEC, this);
      u.ref.component_internal = false;
      break;
    default:
      FATAL_ERROR("Type::Type()");
    } // switch
  }

  Type::Type(typetype_t p_tt, Identifier *p_id, Type *p_type)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_SELTYPE || !p_id || !p_type) FATAL_ERROR("Type::Type()");
    init();
    u.seltype.id=p_id;
    u.seltype.type=p_type;
    u.seltype.type->set_ownertype(OT_SELTYPE, this);
    u.seltype.type_refd=0;
  }

  Type::Type(typetype_t p_tt, Type *p_type, Ttcn::ArrayDimension *p_dim,
             bool p_in_typedef)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_ARRAY || !p_type || !p_dim) FATAL_ERROR("Type::Type()");
    init();
    u.array.element_type = p_type;
    u.array.element_type->set_ownertype(OT_ARRAY, this);
    u.array.dimension = p_dim;
    u.array.in_typedef = p_in_typedef;
    u.array.component_internal = false;
  }

  Type::Type(typetype_t p_tt, Type *p_type, OC_defn *p_oc_defn,
             const Identifier *p_id)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_OCFT || !p_type ||!p_oc_defn || !p_id)
      FATAL_ERROR("Type::Type()");
    init();
    u.ref.type_refd=p_type;
    u.ref.type_refd->set_ownertype(OT_OCFT, this);
    u.ref.oc_defn=p_oc_defn;
    u.ref.oc_fieldname=p_id;
    u.ref.component_internal = false;
  }

  Type::Type(typetype_t p_tt, OC_defn *p_oc_defn,
             const Identifier *p_id)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_OPENTYPE || !p_oc_defn || !p_id) FATAL_ERROR("Type::Type()");
    init();
    has_opentypes=true;
    u.secho.cfm=new CompFieldMap();
    u.secho.cfm->set_my_type(this);
    u.secho.oc_defn=p_oc_defn;
    u.secho.oc_fieldname=p_id;
    u.secho.my_tableconstraint=0;
    u.secho.field_by_name = 0;
    u.secho.component_internal = false;
    u.secho.has_single_charenc = false;
  }

  Type::Type(typetype_t p_tt, Reference *p_ref)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_REFD || !p_ref) FATAL_ERROR("Type::Type()");
    init();
    u.ref.ref=p_ref;
    u.ref.type_refd=0;
    u.ref.component_internal = false;
  }

  Type::Type(typetype_t p_tt, Ttcn::PortTypeBody *p_pb)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_PORT || !p_pb) FATAL_ERROR("Type::Type()");
    init();
    u.port = p_pb;
    p_pb->set_my_type(this);
  }

  Type::Type(typetype_t p_tt, ComponentTypeBody *p_cb)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_COMPONENT || !p_cb) FATAL_ERROR("Type::Type()");
    init();
    u.component = p_cb;
    p_cb->set_my_type(this);
  }

  Type::Type(typetype_t p_tt, SignatureParamList *p_params, Type *p_returntype,
             bool p_noblock, SignatureExceptions *p_exceptions)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_SIGNATURE || (p_returntype && p_noblock))
      FATAL_ERROR("Type::Type()");
    init();
    u.signature.parameters = p_params;
    if ((u.signature.return_type = p_returntype)) { // check assignment for 0
      u.signature.return_type->set_ownertype(OT_SIGNATURE, this);
    }
    u.signature.no_block = p_noblock;
    u.signature.exceptions = p_exceptions;
    u.signature.component_internal = false;
  }

  Type::Type(typetype_t p_tt,Ttcn::FormalParList *p_params,
        Ttcn::Reference* p_runs_on_ref, bool p_runs_on_self,
        Type *p_returntype, bool p_returns_template,
        template_restriction_t p_template_restriction)
    : Governor(S_T), typetype(p_tt)
  {
    if (p_tt != T_FUNCTION || !p_params || (!p_returntype && p_returns_template)
        || (p_runs_on_ref && p_runs_on_self)) FATAL_ERROR("Type::Type()");
    init();
    u.fatref.fp_list = p_params;
    u.fatref.runs_on.ref = p_runs_on_ref;
    u.fatref.runs_on.self = p_runs_on_self;
    u.fatref.runs_on.type = 0;
    if ((u.fatref.return_type = p_returntype)) { // check assignment for 0
      u.fatref.return_type->set_ownertype(OT_FUNCTION, this);
    }
    u.fatref.is_startable = false;
    u.fatref.returns_template = p_returns_template;
    u.fatref.template_restriction = p_template_restriction;
  }

  Type::Type(typetype_t p_tt,Ttcn::FormalParList *p_params,
        Ttcn::Reference* p_runs_on_ref, bool p_runs_on_self)
    : Governor(S_T), typetype(p_tt)
  {
    if(p_tt != T_ALTSTEP || !p_params || (p_runs_on_ref && p_runs_on_self))
      FATAL_ERROR("Type::Type()");
    init();
    u.fatref.fp_list = p_params;
    u.fatref.runs_on.ref = p_runs_on_ref;
    u.fatref.runs_on.self = p_runs_on_self;
    u.fatref.runs_on.type = 0;
    u.fatref.return_type = 0;
    u.fatref.is_startable = false;
    u.fatref.returns_template = false;
    u.fatref.template_restriction = TR_NONE;
  }

  Type::Type(typetype_t p_tt,Ttcn::FormalParList *p_params,
        Ttcn::Reference* p_runs_on_ref, Ttcn::Reference *p_system_ref)
    : Governor(S_T), typetype(p_tt)
  {
    if(p_tt != T_TESTCASE || !p_params || !p_runs_on_ref)
      FATAL_ERROR("Type::Type()");
    init();
    u.fatref.fp_list = p_params;
    u.fatref.runs_on.ref = p_runs_on_ref;
    u.fatref.runs_on.self = false;
    u.fatref.runs_on.type = 0;
    u.fatref.system.ref = p_system_ref;
    u.fatref.system.type = 0;
    u.fatref.is_startable = false;
    u.fatref.returns_template = false;
    u.fatref.template_restriction = TR_NONE;
  }

  Type::~Type()
  {
    clean_up();
  }

  void Type::free_pools()
  {
    destroy_default_tags();// Additionally: R&S license warning
    destroy_pooltypes();// Additionally: R&S license checkin/disconnect/shutdown
  }

  Type *Type::clone() const
  {
    return new Type(*this);
  }

  Type::typetype_t Type::get_typetype_ttcn3(typetype_t p_tt)
  {
    switch (p_tt) {
    case T_INT_A:
      return T_INT;
    case T_ENUM_A:
      return T_ENUM_T;
    case T_BSTR_A:
      return T_BSTR;
    case T_UTF8STRING:
    case T_BMPSTRING:
    case T_UNIVERSALSTRING:
      return T_USTR;
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
    case T_GRAPHICSTRING:
    case T_OBJECTDESCRIPTOR:
    case T_GENERALSTRING:
      // iso2022str
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_IA5STRING:
    case T_VISIBLESTRING:
    case T_UTCTIME:
    case T_GENERALIZEDTIME:
      return T_CSTR;
    case T_ROID:
      return T_OID;
    case T_ANY:
      return T_OSTR;
    case T_CHOICE_A:
    case T_OPENTYPE:
      return T_CHOICE_T;
    case T_SEQ_A:
    case T_EXTERNAL:
    case T_EMBEDDED_PDV:
    case T_UNRESTRICTEDSTRING:
      return T_SEQ_T;
    case T_SET_A:
      return T_SET_T;
    default:
      return p_tt;
    } // switch typetype
  }

  bool Type::is_asn1() const
  {
    if (my_scope) return Setting::is_asn1();
    // the type might be a pool type, which is considered to be a TTCN-3 type
    typetype_t t_typetype = get_typetype_ttcn3(typetype);
    if (pooltypes && pooltypes->has_key(t_typetype) &&
        (*pooltypes)[t_typetype] == this) return false;
    else FATAL_ERROR("Type::is_asn1()");
  }

  bool Type::is_ref() const
  {
    switch(typetype) {
    case T_UNRESTRICTEDSTRING:
    case T_OCFT:
    case T_EXTERNAL:
    case T_EMBEDDED_PDV:
    case T_REFD:
    case T_REFDSPEC:
    case T_SELTYPE:
    case T_ADDRESS:
      return true;
    default:
      return false;
    } // switch
  }

  bool Type::is_secho() const
  {
    switch(typetype) {
    case T_ANYTYPE:
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_OPENTYPE:
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SET_A:
    case T_SET_T:
      return true;
    default:
      return false;
    } // switch
  }

  Type::truth Type::is_charenc()
  {
    switch(typetype) {
    case T_CHOICE_A:
    case T_CHOICE_T:
    {
      bool possible = true;
      size_t ncomp = u.secho.cfm->get_nof_comps();
      for (size_t i=0; i<ncomp; ++i) {
        CompField * cf = u.secho.cfm->get_comp_byIndex(i);
        if (cf->get_type()->is_charenc() == No) {
          possible = false; break;
        }
      } // next i
      if (possible) {
        return (xerattrib && (xerattrib->useUnion_ || xerattrib->useType_)) ? Yes : No;
      }
    }
    // no break
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SET_A:
    case T_SET_T:
    case T_OPENTYPE:
      // UNTAGGED cannot be used to make a type character-encodable!
      // But USE-QNAME can!
      return (xerattrib && xerattrib->useQName_) ? Yes : No;

    case T_SEQOF: // A record-of is character-encodable if it has the "list"
    case T_SETOF: // attribute and its element is character-encodable.
      return (xerattrib && xerattrib->list_ && (u.seof.ofType->is_charenc()==Yes))
      ? Yes : No;

    case T_ENUM_A:
    case T_ENUM_T:
    case T_VERDICT:
      return Yes;

    default:
      if (is_ref()) {
        truth retval = get_type_refd_last()->is_charenc();
        if (retval == Yes) return Yes;
        else if (retval == Maybe) {
          if (xerattrib && xerattrib->useUnion_) return Yes;
        }
        // else fall through to No
      }
      return No;

    case T_BSTR:
    case T_OSTR:
    case T_HSTR:
    case T_CSTR:
    case T_USTR:
    case T_UTF8STRING:
      // TODO ASN.1 restricted character string types when (if) ASN.1 gets XER
      // TODO check subtype; strings must be restricted to not contain
      // control characters (0..0x1F except 9,0x0A,0x0D)
    case T_INT_A:
    case T_INT:
    case T_BOOL:
    case T_REAL:
      // TODO more types
      /* FIXME : this kind of check should be applied to elements of secho,
       *  not to the type of the element ! */
      return Yes;
    }
  }

  bool Type::has_empty_xml() {
    bool answer = false;
    switch (typetype) {
    case T_SEQ_A: case T_SEQ_T:
    case T_SET_A: case T_SET_T: {
      answer = true; // If all components are optional.
      size_t n_comps = get_nof_comps();
      for (size_t i = 0; i < n_comps; ++i) {
        CompField* cf = get_comp_byIndex(i);
        if (!cf->get_is_optional()) {
          answer = false;
          break; // the loop
        }
      }
      break; }
    case T_SEQOF: case T_SETOF:
      // _If_ there is a length restriction, 0 length must be allowed.
      // By this time parsed_restr has been absorbed into sub_type.
      answer = (sub_type==0) || sub_type->zero_length_allowed();
      break;
    default:
      break;
    } // switch
    return answer;
  }

  void Type::set_fullname(const string& p_fullname)
  {
    Governor::set_fullname(p_fullname);
    switch(typetype) {
    case T_INT_A:
    case T_BSTR_A:
      if(u.namednums.block) u.namednums.block->set_fullname(p_fullname);
      if(u.namednums.nvs)
        u.namednums.nvs->set_fullname(p_fullname+".<namedvalues>");
      break;
    case T_ENUM_A:
      if(u.enums.eis1) u.enums.eis1->set_fullname(p_fullname);
      if(u.enums.eis2) u.enums.eis2->set_fullname(p_fullname);
      // no break
    case T_ENUM_T:
      u.enums.eis->set_fullname(p_fullname);
      break;
    case T_ANYTYPE:
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_OPENTYPE:
      u.secho.cfm->set_fullname(p_fullname);
      break;
    case T_SEQ_A:
    case T_SET_A:
    case T_CHOICE_A:
      if (u.secho.ctss) u.secho.ctss->set_fullname(p_fullname);
      break;
    case T_SEQOF:
    case T_SETOF: {
      string subtypename(".<oftype>");
      Type * t = u.seof.ofType;
      /* Do NOT call get_type_refd_last() or else fatal_error !
       * The AST is not fully set up. */

      /* XER will use these strings */
      switch (t->typetype)
      {
      case T_EMBEDDED_PDV: case T_EXTERNAL:
      case T_SEQ_A: case T_SEQ_T:
        subtypename = ".SEQUENCE";
        break;

      case T_SET_A: case T_SET_T:
        subtypename = ".SET";
        break;

      case T_SEQOF:
        subtypename = ".SEQUENCE_OF";
        break;

      case T_SETOF:
        subtypename = ".SET_OF";
        break;

      case T_BSTR_A:
        subtypename = ".BITSTRING";
        break;

      case T_BOOL:
        subtypename = ".BOOLEAN";
        break;

      case T_CHOICE_A: case T_CHOICE_T:
        subtypename = ".CHOICE";
        break;

      case T_ENUM_A: case T_ENUM_T:
        subtypename = ".ENUMERATED";
        break;

      case T_INT_A: case T_INT:
        subtypename = ".INTEGER";
        break;

      default:
        break;
      }
      u.seof.ofType->set_fullname(p_fullname+subtypename);
      break; }
    case T_REFD:
      u.ref.ref->set_fullname(p_fullname);
      break;
    case T_SELTYPE:
      u.seltype.type->set_fullname(p_fullname+".<selection>");
      break;
    case T_PORT:
      u.port->set_fullname(p_fullname);
      break;
    case T_COMPONENT:
      u.component->set_fullname(p_fullname);
      break;
    case T_ARRAY:
      u.array.element_type->set_fullname(p_fullname + ".<element_type>");
      u.array.dimension->set_fullname(p_fullname + ".<dimension>");
      break;
    case T_SIGNATURE:
      if (u.signature.parameters)
        u.signature.parameters->set_fullname(p_fullname);
      if (u.signature.return_type)
        u.signature.return_type->set_fullname(p_fullname + ".<return_type>");
      if (u.signature.exceptions)
        u.signature.exceptions->set_fullname(p_fullname + ".<exception_list>");
      break;
    case T_FUNCTION:
    case T_ALTSTEP:
      u.fatref.fp_list->set_fullname(p_fullname + "<formal_par_list>");
      if (u.fatref.runs_on.ref)
        u.fatref.runs_on.ref->set_fullname(p_fullname + "<runs_on_type>");
      if (u.fatref.return_type)
        u.fatref.return_type->set_fullname(p_fullname + "<return type>");
      break;
    case T_TESTCASE:
      u.fatref.fp_list->set_fullname(p_fullname + ".<formal_par_list>");
      if (u.fatref.runs_on.ref)
        u.fatref.runs_on.ref->set_fullname(p_fullname+".<runs_on_type>");
      if (u.fatref.system.ref)
        u.fatref.system.ref->set_fullname(p_fullname + ".<system_type>");
      break;
    default:
      break;
    } // switch
  }

  void Type::set_my_scope(Scope *p_scope)
  {
    Governor::set_my_scope(p_scope);
    if(tags) tags->set_my_scope(p_scope);
    switch(typetype) {
    case T_INT_A:
    case T_BSTR_A:
      if(u.namednums.nvs) u.namednums.nvs->set_my_scope(p_scope);
      break;
    case T_ENUM_A:
      if(u.enums.eis1) u.enums.eis1->set_my_scope(p_scope);
      if(u.enums.eis2) u.enums.eis2->set_my_scope(p_scope);
      // no break
    case T_ENUM_T:
      u.enums.eis->set_my_scope(p_scope);
      break;
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_OPENTYPE:
    case T_ANYTYPE:
      u.secho.cfm->set_my_scope(p_scope);
      break;
    case T_SEQ_A:
    case T_SET_A:
    case T_CHOICE_A:
      if(u.secho.ctss) u.secho.ctss->set_my_scope(p_scope);
      break;
    case T_SEQOF:
    case T_SETOF:
      u.seof.ofType->set_my_scope(p_scope);
      break;
    case T_REFD:
      u.ref.ref->set_my_scope(p_scope);
      break;
    case T_SELTYPE:
      u.seltype.type->set_my_scope(p_scope);
      break;
    case T_ARRAY:
      u.array.element_type->set_my_scope(p_scope);
      u.array.dimension->set_my_scope(p_scope);
      break;
    case T_PORT:
      u.port->set_my_scope(p_scope);
      break;
    case T_SIGNATURE:
      if (u.signature.parameters)
        u.signature.parameters->set_my_scope(p_scope);
      if (u.signature.return_type)
        u.signature.return_type->set_my_scope(p_scope);
      if (u.signature.exceptions)
        u.signature.exceptions->set_my_scope(p_scope);
      break;
    case T_COMPONENT:
      u.component->set_my_scope(p_scope);
      break;
    case T_FUNCTION:
    case T_ALTSTEP:
      // the scope of parameter list is set later in chk_Fat()
      if (u.fatref.runs_on.ref)
        u.fatref.runs_on.ref->set_my_scope(p_scope);
      if (u.fatref.return_type)
        u.fatref.return_type->set_my_scope(p_scope);
      break;
    case T_TESTCASE:
      // the scope of parameter list is set later in chk_Fat()
      if (u.fatref.runs_on.ref)
        u.fatref.runs_on.ref->set_my_scope(p_scope);
      if (u.fatref.system.ref)
        u.fatref.system.ref->set_my_scope(p_scope);
      break;
    default:
      break;
    } // switch
  }

  Type* Type::get_type_refd(ReferenceChain *refch)
  {
    switch(typetype) {
    case T_REFD: {
      if(refch && !refch->add(get_fullname())) goto error;
      if(!u.ref.type_refd) {
        Assignment *ass = u.ref.ref->get_refd_assignment();
        if (!ass) goto error; // The referenced assignment is not found
        switch (ass->get_asstype()) {
        case Assignment::A_ERROR:
          goto error;
        case Assignment::A_TYPE:
        case Assignment::A_VS:
          u.ref.type_refd = ass->get_Type()->get_field_type(
            u.ref.ref->get_subrefs(), EXPECTED_DYNAMIC_VALUE, refch);
          if (!u.ref.type_refd) goto error;
          break;
          //case Assignment::A_VS:
          //u.ref.type_refd = ass->get_Type();
          //      if(!u.ref.type_refd) goto error;
          //break;
        case Assignment::A_OC:
        case Assignment::A_OBJECT:
        case Assignment::A_OS: {
          Setting *setting = u.ref.ref->get_refd_setting();
          if (!setting || setting->get_st() == Setting::S_ERROR) goto error;
          /* valueset? */
          u.ref.type_refd = dynamic_cast<Type*>(setting);
          if(!u.ref.type_refd) {
            error("`%s' is not a reference to a type",
              u.ref.ref->get_dispname().c_str());
            goto error;
          }

          if (u.ref.type_refd->ownertype == OT_UNKNOWN) {
            u.ref.type_refd->set_ownertype(OT_REF, this);
          }

          break;}
        default:
          error("`%s' is not a reference to a type",
            u.ref.ref->get_dispname().c_str());
          goto error;
        } // switch
        if(!u.ref.type_refd->get_my_scope()) {
          // opentype or OCFT
          u.ref.type_refd->set_my_scope(get_my_scope());
          u.ref.type_refd->set_parent_type(get_parent_type());
          u.ref.type_refd->set_genname(get_genname_own(), string("type"));
          u.ref.type_refd->set_fullname(get_fullname()+".type");
        }
        if (u.ref.type_refd->typetype == T_OPENTYPE && !constraints)
          warning("An open type without table constraint is useless in TTCN-3");
      }
      return u.ref.type_refd;
      break;}
    case T_SELTYPE: {
      if(refch && !refch->add(get_fullname())) goto error;
      if(!u.seltype.type_refd) {
        Type *t=u.seltype.type->get_type_refd_last(refch);
        if(t->typetype==T_ERROR) goto error;
        if(t->typetype!=T_CHOICE_A) {
          error("(Reference to) a CHOICE type was expected"
            " in selection type.");
          goto error;
        }
        if(!t->has_comp_withName(*u.seltype.id)) {
          error("No alternative with name `%s' in the given type `%s'.",
            u.seltype.id->get_dispname().c_str(),
            t->get_fullname().c_str());
          goto error;
        }
        u.seltype.type_refd=t->get_comp_byName(*u.seltype.id)->get_type();
      }
      return u.seltype.type_refd;
      break;}
    case T_REFDSPEC:
    case T_OCFT:
      if(refch && !refch->add(get_fullname())) goto error;
      return u.ref.type_refd;
      break;
    case T_EXTERNAL: {
      if (!my_scope) FATAL_ERROR("Type::get_type_refd()");
      Identifier t_id(Identifier::ID_ASN, string("EXTERNAL"));
      return my_scope->get_scope_asss()->get_local_ass_byId(t_id)->get_Type(); }
    case T_EMBEDDED_PDV: {
      if (!my_scope) FATAL_ERROR("Type::get_type_refd()");
      Identifier t_id(Identifier::ID_ASN, string("EMBEDDED PDV"));
      return my_scope->get_scope_asss()->get_local_ass_byId(t_id)->get_Type(); }
    case T_UNRESTRICTEDSTRING: {
      if (!my_scope) FATAL_ERROR("Type::get_type_refd()");
      Identifier t_id(Identifier::ID_ASN, string("CHARACTER STRING"));
      return my_scope->get_scope_asss()->get_local_ass_byId(t_id)->get_Type(); }
    case T_ADDRESS:
      if (refch && !refch->add(get_fullname())) goto error;
      if (u.address) return u.address;
      if (!my_scope) FATAL_ERROR("Type::get_type_refd()");
      u.address = my_scope->get_scope_mod()->get_address_type();
      if (!u.address) {
        error("Type `address' is not defined in this module");
        goto error;
      }
      return u.address;
    default:
      FATAL_ERROR("Type::get_type_refd()");
      return 0;
    } // switch
    error:
    clean_up();
    return this;
  }

  Type* Type::get_type_refd_last(ReferenceChain *refch)
  {
    Type *t=this;
    while(t->is_ref()) t=t->get_type_refd(refch);
    return t;
  }

  Type *Type::get_field_type(Ttcn::FieldOrArrayRefs *subrefs,
    expected_value_t expected_index, ReferenceChain *refch,
    bool interrupt_if_optional)
  {
    if (!subrefs) return this;
    Type *t = this;
    if (expected_index == EXPECTED_TEMPLATE)
      expected_index = EXPECTED_DYNAMIC_VALUE;
    size_t nof_refs = subrefs->get_nof_refs();
    subrefs->clear_string_element_ref();
    for (size_t i = 0; i < nof_refs; i++) {
      if (refch) refch->mark_state();
      t = t->get_type_refd_last(refch);
      if (refch) refch->prev_state();
      // stop immediately if current type t is erroneous
      // (e.g. because of circular reference)
      if (t->typetype == T_ERROR) return 0;
      Ttcn::FieldOrArrayRef *ref = subrefs->get_ref(i);
      switch (ref->get_type()) {
      case Ttcn::FieldOrArrayRef::FIELD_REF: {
        if (t->typetype == T_OPENTYPE) {
          // allow the alternatives of open types as both lower and upper identifiers
          ref->set_field_name_to_lowercase();
        }
        const Identifier& id = *ref->get_id();
        switch (t->typetype) {
        case T_CHOICE_A:
        case T_CHOICE_T:
        case T_OPENTYPE:
        case T_SEQ_A:
        case T_SEQ_T:
        case T_SET_A:
        case T_SET_T:
        case T_ANYTYPE:
          break;
	case T_COMPONENT:
	  ref->error("Referencing fields of a component is not allowed");
	  return 0;
        default:
          ref->error("Invalid field reference `%s': type `%s' "
            "does not have fields", id.get_dispname().c_str(),
            t->get_typename().c_str());
          return 0;
        }
        if (!t->has_comp_withName(id)) {
          ref->error("Reference to non-existent field `%s' in type `%s'",
            id.get_dispname().c_str(),
            t->get_typename().c_str());
          return 0;
        }
        CompField* cf = t->get_comp_byName(id);
        if (interrupt_if_optional && cf->get_is_optional()) return 0;
        t = cf->get_type();
        break; }
      case Ttcn::FieldOrArrayRef::ARRAY_REF: {
        Type *embedded_type = 0;
        switch (t->typetype) {
        case T_SEQOF:
        case T_SETOF:
          embedded_type = t->u.seof.ofType;
          break;
        case T_ARRAY:
          embedded_type = t->u.array.element_type;
          break;
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
        case T_OBJECTDESCRIPTOR:
          if (subrefs->refers_to_string_element()) {
            ref->error("A string element of type `%s' cannot be indexed",
              t->get_typename().c_str());
            return 0;
          } else {
            subrefs->set_string_element_ref();
            // string elements have the same type as the string itself
            embedded_type = t;
            break;
          }
        default:
          ref->error("Type `%s' cannot be indexed",
            t->get_typename().c_str());
          return 0;
        }
        // check the index value
        Value *index_value = ref->get_val();
        index_value->set_lowerid_to_ref();
        
        // pt is the type with the indexing is made, while t is the type on the
        // indexing is applied.
        Type* pt = index_value->get_expr_governor_last();
        if (pt != NULL &&
          // The indexer type is an array or record of
           (pt->get_typetype() == T_ARRAY || pt->get_typetype() == T_SEQOF) &&
          // The indexed type is a record of or set of or array
           (t->get_typetype() == T_SEQOF || t->get_typetype() == T_SETOF || t->get_typetype() == T_ARRAY)) {
          
          // The indexer type must be of type integer
          if (pt->get_ofType()->get_type_refd_last()->get_typetype() != T_INT) {
            ref->error("Only fixed length array or record of integer types are allowed for short-hand notation for nested indexes.");
            return 0;
          }
          int len = 0;
          // Get the length of the array or record of
          if (pt->get_typetype() == T_ARRAY) {
            len = (int)pt->get_dimension()->get_size();
          } else if (pt->get_typetype() == T_SEQOF) {
            SubType* sub = pt->get_sub_type();
            if (sub == NULL) {
              ref->error("The type `%s' must have single size length restriction when used as a short-hand notation for nested indexes.",
                pt->get_typename().c_str());
              return 0;
            }
            len = pt->get_sub_type()->get_length_restriction();
            if (len == -1) {
              ref->error("The type `%s' must have single size length restriction when used as a short-hand notation for nested indexes.",
                pt->get_typename().c_str());
              return 0;
            }
          }
          embedded_type = embedded_type->get_type_refd_last();
          int j = 0;
          // Get the len - 1'th inner type
          while (j < len - 1) {
            switch (embedded_type->get_typetype()) {
              case T_SEQOF:
              case T_SETOF:
              case T_ARRAY:
                embedded_type = embedded_type->get_ofType()->get_type_refd_last();
                break;
              default:
                ref->error("The type `%s' contains too many indexes (%i) in the short-hand notation for nested indexes.",
                  pt->get_typename().c_str(), len);
                return 0;
            }
            j++;
          }
        } else if (t->typetype == T_ARRAY) {
          // checking of array index is performed by the array dimension
          t->u.array.dimension->chk_index(index_value, expected_index);
        } else {
          // perform a generic index check for other types
          if (refch == 0 // variable assignment
            || index_value->get_valuetype() != Value::V_NOTUSED) {
            Error_Context cntxt(index_value, "In index value");
            index_value->chk_expr_int(expected_index);
          }
          Value *v_last = index_value->get_value_refd_last();
          if (v_last->get_valuetype() == Value::V_INT) {
            const int_val_t *index_int = v_last->get_val_Int();
            if (*index_int > INT_MAX) {
              index_value->error("Integer value `%s' is too big for indexing "
                "type `%s'", (index_int->t_str()).c_str(),
                (t->get_typename()).c_str());
              index_value->set_valuetype(Value::V_ERROR);
            } else {
              if (*index_int < 0) {
                index_value->error("A non-negative integer value was "
                  "expected for indexing type `%s' instead of `%s'",
                  t->get_typename().c_str(), (index_int->t_str()).c_str());
                index_value->set_valuetype(Value::V_ERROR);
              }
            }
          }
        }
        // change t to the embedded type
        t = embedded_type;
        break; }
      default:
        FATAL_ERROR("Type::get_field_type(): invalid reference type");
      }
    }
    return t;
  }

  bool Type::get_subrefs_as_array(const Ttcn::FieldOrArrayRefs *subrefs, dynamic_array<size_t>& subrefs_array, dynamic_array<Type*>& type_array)
  {
    if (!subrefs) FATAL_ERROR("Type::get_subrefs_as_array()");
    Type *t = this;
    size_t nof_refs = subrefs->get_nof_refs();
    for (size_t i = 0; i < nof_refs; i++) {
      t = t->get_type_refd_last();
      type_array.add(t);
      if (t->typetype == T_ERROR) FATAL_ERROR("Type::get_subrefs_as_array()");
      Ttcn::FieldOrArrayRef *ref = subrefs->get_ref(i);
      size_t field_index=0;
      switch (ref->get_type()) {
      case Ttcn::FieldOrArrayRef::FIELD_REF: {
        const Identifier& id = *ref->get_id();
        if (!t->has_comp_withName(id)) FATAL_ERROR("Type::get_subrefs_as_array()");
        CompField* cf = t->get_comp_byName(id);
        field_index = t->get_comp_index_byName(id);
        field_index = t->get_codegen_index(field_index);
        t = cf->get_type();
        break; }
      case Ttcn::FieldOrArrayRef::ARRAY_REF: {
        Value *index_value = ref->get_val();
        Value *v_last = index_value->get_value_refd_last();
        if (v_last->get_valuetype()!=Value::V_INT) {
          // workaround: get_field_type() does not return NULL if the index
          // value is invalid, this function returns false in this case
          return false;
        }
        const int_val_t *index_int = v_last->get_val_Int();
        if (!index_int->is_native() || index_int->is_negative()) {
          return false;
        }
        field_index = (size_t)index_int->get_val();
        Type *embedded_type = 0;
        switch (t->typetype) {
        case T_SEQOF:
        case T_SETOF:
          embedded_type = t->u.seof.ofType;
          break;
        case T_ARRAY:
          embedded_type = t->u.array.element_type;
          break;
        default:
          embedded_type = t;
          break;
        }
        // change t to the embedded type
        t = embedded_type;
        break; }
      default:
        FATAL_ERROR("Type::get_subrefs_as_array()");
      }
      subrefs_array.add(field_index);
    }
    return true;
  }

  bool Type::is_optional_field() const {
    if (ownertype == OT_COMP_FIELD) {
        const CompField* const myOwner = (CompField*) owner;
        return myOwner && myOwner->get_is_optional();
    }
    return false;
  }

  bool Type::field_is_optional(Ttcn::FieldOrArrayRefs *subrefs)
  {
    // handling trivial cases
    if (!subrefs) return false;
    size_t nof_subrefs = subrefs->get_nof_refs();
    if (nof_subrefs < 1) return false;
    Ttcn::FieldOrArrayRef *last_ref = subrefs->get_ref(nof_subrefs - 1);
    if (last_ref->get_type() == Ttcn::FieldOrArrayRef::ARRAY_REF) return false;
    // following the embedded types
    Type *t=get_type_refd_last();
    for (size_t i = 0; i < nof_subrefs - 1; i++) {
      Ttcn::FieldOrArrayRef *ref = subrefs->get_ref(i);
      if (ref->get_type() == Ttcn::FieldOrArrayRef::FIELD_REF)
        t = t->get_comp_byName(*ref->get_id())->get_type();
      else t = t->get_ofType();
      t=t->get_type_refd_last();
    }
    // now last_ref refers to a field of t
    return t->get_comp_byName(*last_ref->get_id())->get_is_optional();
  }

  bool Type::is_root_basic(){
    Type *t=get_type_refd_last();
    switch(t->typetype){
    case T_INT:
    case T_BOOL:
    case T_BSTR:
    case T_HSTR:
    case T_OSTR:
    case T_CSTR:
      return true;
      break;
    default:
      break;
    }
    return false;
  }

  int Type::get_raw_length(){
    if(!raw_checked) FATAL_ERROR("Type::get_raw_length()");
    if(raw_length_calculated) return raw_length;
    raw_length_calculated=true;
    switch(typetype) {
    case T_REFD:
      raw_length=get_type_refd()->get_raw_length();
      break;
    case T_INT:
      if(rawattrib) raw_length=rawattrib->fieldlength;
      else raw_length=8;
      break;
    case T_BOOL:
      if(rawattrib) raw_length=rawattrib->fieldlength;
      else raw_length=1;
      break;
    case T_BSTR:
    case T_HSTR:
    case T_OSTR:
    case T_CSTR:
      if(rawattrib && rawattrib->fieldlength) raw_length=rawattrib->fieldlength;
      else raw_length=-1;
      break;
    case T_ENUM_T:
      if(rawattrib && rawattrib->fieldlength) raw_length=rawattrib->fieldlength;
      else{
        int min_bits=0;
        int max_val=u.enums.first_unused;
        for(size_t a=0;a<u.enums.eis->get_nof_eis();a++){
          int val=u.enums.eis->get_ei_byIndex(a)->get_int_val()->get_val();
          if((max_val<0?-max_val:max_val)<(val<0?-val:val)) max_val=val;
        }
        if(max_val<0){ min_bits=1;max_val=-max_val;}
        while(max_val){ min_bits++; max_val/=2;}
        raw_length=min_bits;
      }
      break;
    case T_SEQ_T:
    case T_SET_T:
      raw_length=0;
      for(size_t i = 0; i < get_nof_comps(); i++){
        int l=0;
        CompField* cf=get_comp_byIndex(i);
        if(cf->get_is_optional()){
          raw_length=-1;
          return raw_length;
        }
        l=cf->get_type()->get_raw_length();
        if(l==-1){
          raw_length=-1;
          return raw_length;
        }
        if(cf->get_type()->rawattrib
        && (cf->get_type()->rawattrib->pointerto
          || cf->get_type()->rawattrib->lengthto_num)){
          raw_length=-1;
          return raw_length;
        }
        raw_length+=l;
      }
      break;
    // TODO: case T_ANYTYPE: for get_raw_length needed ?
    case T_CHOICE_T:
      for(size_t i = 0; i < get_nof_comps(); i++){
        CompField *cf=get_comp_byIndex(i);
        int l=0;
        l=cf->get_type()->get_raw_length();
        if(l==-1){
          raw_length=-1;
          return raw_length;
        }
        if(i){
          if(raw_length!=l){
            raw_length=-1;
            return raw_length;
          }
        }
        else raw_length=l;
      }
      break;
    default:
      raw_length=-1;
      break;
    }
    return raw_length;
  }

  /** \todo: add extra checks and warnings for unsupported attributes
   * e.g. when TAG refers to a record/set field which has union type */
  void Type::chk_raw()
  {
    bool self_ref = false;
    if (raw_checked) return;
    raw_checked = true;
    if (!enable_raw()) return;
    int restrlength=-1;
    if(sub_type)
      restrlength=(int)sub_type->get_length_restriction();
    if(restrlength!=-1){
      if(!rawattrib){
        Type *t=get_type_refd_last();
        typetype_t basic_type=t->typetype;
        rawattrib=new RawAST(basic_type==T_INT);
        if(basic_type==T_REAL) rawattrib->fieldlength=64;
      }
      rawattrib->length_restrition=restrlength;
    }
    if(!rawattrib) return;
    switch(typetype) {
    case T_REFD:
      get_type_refd()->force_raw();
      if(rawattrib->fieldlength==0 && rawattrib->length_restrition!=-1){
        typetype_t basic_type=get_type_refd_last()->typetype;
        switch(basic_type){
          case T_BSTR:
            rawattrib->fieldlength=rawattrib->length_restrition;
            rawattrib->length_restrition=-1;
            break;
          case T_HSTR:
            rawattrib->fieldlength=rawattrib->length_restrition*4;
            rawattrib->length_restrition=-1;
            break;
          case T_OSTR:
            rawattrib->fieldlength=rawattrib->length_restrition*8;
            rawattrib->length_restrition=-1;
            break;
          case T_CSTR:
          case T_USTR:
            rawattrib->fieldlength=rawattrib->length_restrition*8;
            rawattrib->length_restrition=-1;
            break;
          case T_SEQOF:
          case T_SETOF:
            rawattrib->fieldlength=rawattrib->length_restrition;
            rawattrib->length_restrition=-1;
            break;
          default:
            break;
        }
      }
      break;
    case T_CHOICE_T:
      if(rawattrib){
        size_t nof_comps = get_nof_comps();
        for (size_t i = 0; i < nof_comps; i++)
          get_comp_byIndex(i)->get_type()->force_raw();
        for(int c=0;c<rawattrib->taglist.nElements;c++){
          Identifier *idf=rawattrib->taglist.tag[c].fieldName;
          if(!has_comp_withName(*idf)){
            error("Invalid field name `%s' in RAW parameter TAG for type `%s'",
              idf->get_dispname().c_str(), get_typename().c_str());
            continue;
          }
          size_t fieldnum = get_comp_index_byName(*idf);
          for(int a=0;a<rawattrib->taglist.tag[c].nElements;a++){
            bool hiba=false;
            CompField *cf=get_comp_byIndex(fieldnum);
            Type *t=cf->get_type()->get_type_refd_last();
            for(int b=0;b<rawattrib->taglist.tag[c].keyList[a].
                  keyField->nElements;b++){
              Identifier *idf2=
                      rawattrib->taglist.tag[c].keyList[a].keyField->names[b];
              if(!t->is_secho()){
                error("Invalid fieldmember type in RAW parameter TAG"
                      " for field %s."
                      ,cf->get_name().get_dispname().c_str());
                hiba=true;
                break;
              }
              if(!t->has_comp_withName(*idf2)){
                error("Invalid field member name `%s' in RAW parameter TAG "
                  "for field `%s'", idf2->get_dispname().c_str(),
                  cf->get_name().get_dispname().c_str());
                hiba=true;
                break;
              }
              size_t comp_index=t->get_comp_index_byName(*idf2);
              CompField *cf2=t->get_comp_byIndex(comp_index);
              t=cf2->get_type()->get_type_refd_last();
            }
            if(!hiba){
              Error_Context cntx(this, "In Raw parmeter TAG");
              Value *v = rawattrib->taglist.tag[c].keyList[a].v_value;
              v->set_my_scope(get_my_scope());
              v->set_my_governor(t);
              t->chk_this_value_ref(v);
              self_ref = t->chk_this_value(v, 0, EXPECTED_CONSTANT,
                INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
              Value::valuetype_t vt = v->get_valuetype();
              if (vt == Value::V_ENUM || vt == Value::V_REFD) {
                Free(rawattrib->taglist.tag[c].keyList[a].value);
                rawattrib->taglist.tag[c].keyList[a].value =
                  mcopystr(v->get_single_expr().c_str());
              }
            }
          }
        }
      }
      break;
    case T_SEQ_T:
    case T_SET_T: {
      if(rawattrib){
        size_t fieldnum;
        for(int c=0;c<rawattrib->taglist.nElements;c++) { // check TAG
          Identifier *idf=rawattrib->taglist.tag[c].fieldName;
          if(!has_comp_withName(*idf)){
            error("Invalid field name `%s' in RAW parameter TAG "
              "for type `%s'", idf->get_dispname().c_str(),
              get_typename().c_str());
            continue;
          }
          fieldnum=get_comp_index_byName(*idf);
          for(int a=0;a<rawattrib->taglist.tag[c].nElements;a++){
            bool hiba=false;
            CompField *cf=get_comp_byIndex(fieldnum);
            Type *t=cf->get_type()->get_type_refd_last();
            for(int b=0;b<rawattrib->taglist.tag[c].keyList[a].
                  keyField->nElements;b++){
              Identifier *idf2=
                        rawattrib->taglist.tag[c].keyList[a].keyField->names[b];
              if(!t->is_secho()){
                error("Invalid fieldmember type in RAW parameter TAG"
                      " for field %s."
                      ,cf->get_name().get_dispname().c_str());
                hiba=true;
                break;
              }
              if(!t->has_comp_withName(*idf2)){
                error("Invalid field member name `%s' in RAW parameter TAG "
                  "for field `%s'", idf2->get_dispname().c_str(),
                  cf->get_name().get_dispname().c_str());
                hiba=true;
                break;
              }
              size_t comp_index=t->get_comp_index_byName(*idf2);
              CompField *cf2=t->get_comp_byIndex(comp_index);
              t=cf2->get_type()->get_type_refd_last();
            }
            if(!hiba){
              Error_Context cntx(this, "In Raw parmeter TAG");
              Value *v = rawattrib->taglist.tag[c].keyList[a].v_value;
              v->set_my_scope(get_my_scope());
              v->set_my_governor(t);
              t->chk_this_value_ref(v);
              self_ref = t->chk_this_value(v, 0, EXPECTED_CONSTANT,
                INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
              Value::valuetype_t vt = v->get_valuetype();
              if (vt == Value::V_ENUM || vt == Value::V_REFD) {
                Free(rawattrib->taglist.tag[c].keyList[a].value);
                rawattrib->taglist.tag[c].keyList[a].value =
                  mcopystr(v->get_single_expr().c_str());
              }
            }
          }
        }
        for(int a=0; a<rawattrib->ext_bit_goup_num;a++){ // EXTENSION_BIT_GROUP
          Identifier *idf=rawattrib->ext_bit_groups[a].from;
          Identifier *idf2=rawattrib->ext_bit_groups[a].to;
          bool hiba=false;
          if(!has_comp_withName(*idf)){
            error("Invalid field name `%s' in RAW parameter "
              "EXTENSION_BIT_GROUP for type `%s'",
              idf->get_dispname().c_str(), get_typename().c_str());
            hiba=true;
          }
          if(!has_comp_withName(*idf2)){
            error("Invalid field name `%s' in RAW parameter "
              "EXTENSION_BIT_GROUP for type `%s'",
              idf2->get_dispname().c_str(), get_typename().c_str());
            hiba=true;
          }
          if(!hiba){
            size_t kezd=get_comp_index_byName(*idf);
            size_t veg=get_comp_index_byName(*idf2);
            if(kezd>veg){
              error("Invalid field order in RAW parameter "
                "EXTENSION_BIT_GROUP for type `%s': `%s', `%s'",
                get_typename().c_str(), idf->get_dispname().c_str(),
                idf2->get_dispname().c_str());
              hiba=true;
            }
          }
        }
        if(rawattrib->paddall!=XDEFDEFAULT){ // PADDALL
          for(size_t i = 0; i < get_nof_comps(); i++) {
            CompField *cfield=get_comp_byIndex(i);
            RawAST *field_rawattr=cfield->get_type()->rawattrib;
            if(field_rawattr==NULL){
              Type *t=cfield->get_type()->get_type_refd_last();
              typetype_t basic_type=t->typetype;
              t=cfield->get_type();
              if(t->is_ref()) t=t->get_type_refd();
              while(!t->rawattrib && t->is_ref()) t=t->get_type_refd();
              field_rawattr= new RawAST(t->rawattrib,basic_type==T_INT);
              if(!t->rawattrib && basic_type==T_REAL) field_rawattr->fieldlength=64;
              cfield->get_type()->rawattrib=field_rawattr;
            }
            if(field_rawattr->padding==0)
              field_rawattr->padding=rawattrib->padding;
            if(field_rawattr->prepadding==0)
              field_rawattr->prepadding=rawattrib->prepadding;
            if (field_rawattr->padding_pattern_length == 0 &&
                rawattrib->padding_pattern_length > 0) {
              Free(field_rawattr->padding_pattern);
              field_rawattr->padding_pattern =
                mcopystr(rawattrib->padding_pattern);
              field_rawattr->padding_pattern_length =
                rawattrib->padding_pattern_length;
            }
          }
        }
        if(rawattrib->fieldorder!=XDEFDEFAULT){ // FIELDORDER
          for(size_t i = 0; i < get_nof_comps(); i++) {
            CompField *cfield=get_comp_byIndex(i);
            RawAST *field_rawattr=cfield->get_type()->rawattrib;
            if(field_rawattr==NULL){
              Type *t=cfield->get_type()->get_type_refd_last();
              typetype_t basic_type=t->typetype;
              t=cfield->get_type();
              if(t->is_ref()) t=t->get_type_refd();
              while(!t->rawattrib && t->is_ref()) t=t->get_type_refd();
              field_rawattr= new RawAST(t->rawattrib,basic_type==T_INT);
              if(!t->rawattrib && basic_type==T_REAL) field_rawattr->fieldlength=64;
              cfield->get_type()->rawattrib=field_rawattr;
            }
            if(field_rawattr->fieldorder==XDEFDEFAULT)
              field_rawattr->fieldorder=rawattrib->fieldorder;
          }
        }
      }
      for(int a=0;a<rawattrib->presence.nElements;a++){  //PRESENCE
        Type *t=this;
        bool hiba=false;
        for(int b=0;b<rawattrib->presence.keyList[a].keyField->nElements;b++){
          Identifier *idf=rawattrib->presence.keyList[a].keyField->names[b];
          if(!t->is_secho()){
            error("Invalid fieldmember type in RAW parameter PRESENCE"
                  " for the record %s."
                  ,get_typename().c_str());
            hiba=true;
            break;
          }
          if(!t->has_comp_withName(*idf)){
            error("Invalid fieldname in RAW parameter"
                  " PRESENCE for the record %s: %s"
                  ,get_typename().c_str()
                  ,rawattrib->presence.keyList[a].keyField->names[b]
                                                  ->get_dispname().c_str());
            hiba=true;
            break;
          }
          t=t->get_comp_byName(*idf)->get_type()->get_type_refd_last();
        }
        if(!hiba){
          Error_Context cntx(this, "In Raw parameter PRESENCE");
          Value *v = rawattrib->presence.keyList[a].v_value;
          v->set_my_scope(get_my_scope());
          v->set_my_governor(t);
          t->chk_this_value_ref(v);
          self_ref = t->chk_this_value(v, 0, EXPECTED_CONSTANT,
            INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
          Value::valuetype_t vt = v->get_valuetype();
          if (vt == Value::V_ENUM || vt == Value::V_REFD) {
            Free(rawattrib->presence.keyList[a].value);
            rawattrib->presence.keyList[a].value =
              mcopystr(v->get_single_expr().c_str());
          }
        }
      }
      int used_bits = 0; // number of bits used to store all previous fields
      for(size_t i = 0; i < get_nof_comps(); i++) { // field attributes
        CompField *cf = get_comp_byIndex(i);
        const Identifier& field_id = cf->get_name();
        Type *field_type = cf->get_type();
        Type *field_type_last = field_type->get_type_refd_last();
        field_type->force_raw();
        RawAST *rawpar = field_type->rawattrib;
        if (rawpar) {
          if (rawpar->prepadding != 0) {
            used_bits = (used_bits + rawpar->prepadding - 1) / rawpar->prepadding *
              rawpar->prepadding;
          }
          if (rawpar->intx && field_type_last->get_typetype() == T_INT) { // IntX
            if (used_bits % 8 != 0 &&
                (!rawattrib || rawattrib->fieldorder != XDEFMSB)) {
              error("Using RAW parameter IntX in a record/set with FIELDORDER "
                "set to 'lsb' is only supported if the IntX field starts at "
                "the beginning of a new octet. There are %d unused bits in the "
                "last octet before field %s.", 8 - (used_bits % 8),
                field_id.get_dispname().c_str());
            }
          }
          else {
            used_bits += rawpar->fieldlength;
          }
          if (rawpar->padding != 0) {
            used_bits = (used_bits + rawpar->padding - 1) / rawpar->padding *
              rawpar->padding;
          }
          for (int j = 0; j < rawpar->lengthto_num; j++) { // LENGTHTO
            Identifier *idf = rawpar->lengthto[j];
            if (!has_comp_withName(*idf)) {
              error("Invalid fieldname in RAW parameter "
                    "LENGTHTO for field %s: %s",
                    field_id.get_dispname().c_str(),
                    rawpar->lengthto[j]->get_dispname().c_str());
            }
          }
          if (rawpar->lengthto_num) {
            Type *ft = field_type;
            if (ft->get_typetype() == T_REFD) ft = ft->get_type_refd_last();
            typetype_t ftt = ft->get_typetype();
            switch (ftt) {
            case T_INT:
            case T_INT_A:
              break;
            case T_CHOICE_T:
            case T_CHOICE_A:
              for (size_t fi = 0; fi < ft->get_nof_comps(); fi++) {
                typetype_t uftt = ft->get_comp_byIndex(fi)->get_type()
                  ->get_typetype();
                if (uftt != T_INT && uftt != T_INT_A)
                  error("The union type LENGTHTO field must contain only "
                    "integer fields");
              }
              break;
            case T_ANYTYPE:
            case T_OPENTYPE:
            case T_SEQ_A:
            case T_SEQ_T:
            case T_SET_A:
            case T_SET_T:
              if (rawpar->lengthindex) break; // Will be checked in the next step.
              // Else continue with default.
            default:
              error("The LENGTHTO field must be an integer or union type "
                "instead of `%s'", ft->get_typename().c_str());
              break;
            }
          }
          if(rawpar->lengthto_num && rawpar->lengthindex){   // LENGTHINDEX
            Identifier *idf=rawpar->lengthindex->names[0];
            if(!field_type_last->is_secho()){
              error("Invalid fieldmember type in RAW parameter LENGTHINDEX"
                    " for field %s."
                    ,field_id.get_dispname().c_str());
              break;
            }
            if(!field_type_last->has_comp_withName(*idf))
              error("Invalid fieldname in RAW parameter"
                    " LENGTHINDEX for field %s: %s"
                    ,field_id.get_dispname().c_str()
                    ,rawpar->lengthindex->names[0]->get_dispname().c_str());
          }
          if(rawpar->pointerto){    // POINTERTO
            Identifier *idf=rawpar->pointerto;
            bool hiba=false;
            size_t pointed=0;
            if(!has_comp_withName(*idf)){
              error("Invalid fieldname in RAW"
                    " parameter POINTERTO for field %s: %s"
                    ,field_id.get_dispname().c_str()
                    ,rawpar->pointerto->get_dispname().c_str());
              hiba=true;
            }
            if(!hiba && (pointed=get_comp_index_byName(*idf))<=i){
              error("Pointer must precede the pointed field. Incorrect field "
                    "name `%s' in RAW parameter POINTERTO for field `%s'",
                    rawpar->pointerto->get_dispname().c_str(),
                    field_id.get_dispname().c_str());
              hiba=true;
            }
            if(!hiba && rawpar->ptrbase){    // POINTERTO
              Identifier *idf2=rawpar->ptrbase;
              if(!has_comp_withName(*idf2)){
                error("Invalid field name `%s' in RAW parameter PTROFFSET for "
                      "field `%s'", rawpar->ptrbase->get_dispname().c_str(),
                      field_id.get_dispname().c_str());
                hiba=true;
              }
              if(!hiba && get_comp_index_byName(*idf2)>pointed){
                error("Pointer base must precede the pointed field. Incorrect "
                      "field name `%s' in RAW parameter PTROFFSET for field "
                      "`%s'", rawpar->ptrbase->get_dispname().c_str(),
                      field_id.get_dispname().c_str());
              }
            }
          }
          for(int a=0;a<rawpar->presence.nElements;a++){  //PRESENCE
            Type *t=this;
            bool hiba=false;
            for(int b=0;b<rawpar->presence.keyList[a].keyField->nElements;b++){
              Identifier *idf=rawpar->presence.keyList[a].keyField->names[b];
              if(!t->is_secho()){
                error("Invalid fieldmember type in RAW parameter PRESENCE"
                      " for field %s."
                      ,field_id.get_dispname().c_str());
                hiba=true;
                break;
              }
              if(!t->has_comp_withName(*idf)){
                error("Invalid fieldname `%s' in RAW parameter PRESENCE for "
                      "field `%s'", rawpar->presence.keyList[a].keyField
                        ->names[b]->get_dispname().c_str(),
                      field_id.get_dispname().c_str());
                hiba=true;
                break;
              }
              if(b==0 && !(get_comp_index_byName(*rawpar->presence.keyList[a]
                                                       .keyField->names[0])<i)){
                error("The PRESENCE field `%s' must precede the optional field "
                      "in RAW parameter PRESENCE for field `%s'"
                      ,rawpar->presence.keyList[a].keyField->names[0]
                                                      ->get_dispname().c_str()
                      ,field_id.get_dispname().c_str());
                hiba=true;
                break;
              }
              t=t->get_comp_byName(*idf)->get_type()->get_type_refd_last();
            }
            if(!hiba){
              Error_Context cntx(this, "In Raw parmeter PRESENCE");
              Value *v = rawpar->presence.keyList[a].v_value;
              v->set_my_scope(get_my_scope());
              v->set_my_governor(t);
              t->chk_this_value_ref(v);
              self_ref = t->chk_this_value(v, 0, EXPECTED_CONSTANT,
                INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
              Value::valuetype_t vt = v->get_valuetype();
              if (vt == Value::V_ENUM || vt == Value::V_REFD) {
                Free(rawpar->presence.keyList[a].value);
                rawpar->presence.keyList[a].value =
                  mcopystr(v->get_single_expr().c_str());
              }
            }
          }
          for(int c=0;c<rawpar->crosstaglist.nElements;c++) { // CROSSTAG
            Identifier *idf=rawpar->crosstaglist.tag[c].fieldName;
            if(!field_type_last->is_secho()){
              error("Invalid fieldmember type in RAW parameter CROSSTAG"
                    " for field %s."
                    ,field_id.get_dispname().c_str());
              break;
            }
            if(!field_type_last->has_comp_withName(*idf)){
              error("Invalid fieldmember name in RAW parameter CROSSTAG"
                    " for field %s: %s"
                    ,field_id.get_dispname().c_str()
                    ,rawpar->crosstaglist.tag[c].fieldName
                                                      ->get_dispname().c_str());
              break;
            }
            for(int a=0;a<rawpar->crosstaglist.tag[c].nElements;a++){
              Type *t2=this;
              bool hiba=false;
              bool allow_omit = false;
              for(int b=0;
              b<rawpar->crosstaglist.tag[c].keyList[a].keyField->nElements;b++){
                Identifier *idf2=
                      rawpar->crosstaglist.tag[c].keyList[a].keyField->names[b];
                if(!t2->is_secho()){
                  error("Invalid fieldmember type in RAW parameter CROSSTAG"
                        " for field %s."
                        ,field_id.get_dispname().c_str());
                  hiba=true;
                  break;
                }
                if(!t2->has_comp_withName(*idf2)){
                  error("Invalid fieldname in RAW parameter CROSSTAG"
                        " for field %s: %s"
                        ,field_id.get_dispname().c_str()
                        ,idf2->get_dispname().c_str());
                  hiba=true;
                  break;
                }
                if (b == 0) {
                  size_t field_idx = get_comp_index_byName(*idf2);
                  if (field_idx == i) {
                    error("RAW parameter CROSSTAG for field `%s' cannot refer "
                      "to the field itself", idf2->get_dispname().c_str());
                  } else if (field_idx > i) {
                    if (cf->get_is_optional() ||
                        field_type->get_raw_length() < 0)
                      error("Field `%s' that CROSSTAG refers to must precede "
                        "field `%s' or field `%s' must be mandatory with "
                        "fixed length", idf2->get_dispname().c_str(),
                        field_id.get_dispname().c_str(),
                        field_id.get_dispname().c_str());
                  }
                }
                CompField *cf2=t2->get_comp_byName(*idf2);
                t2=cf2->get_type()->get_type_refd_last();
                if (b == rawpar->crosstaglist.tag[c].keyList[a].keyField
                    ->nElements - 1 && cf2->get_is_optional())
                  allow_omit = true;
              }
              if(!hiba){
                Error_Context cntx(this, "In Raw parmeter CROSSTAG");
                Value *v = rawpar->crosstaglist.tag[c].keyList[a].v_value;
                v->set_my_scope(get_my_scope());
                v->set_my_governor(t2);
                t2->chk_this_value_ref(v);
                self_ref = t2->chk_this_value(v, 0, EXPECTED_CONSTANT,
                  INCOMPLETE_NOT_ALLOWED,
                  (allow_omit ? OMIT_ALLOWED : OMIT_NOT_ALLOWED), SUB_CHK);
                Value::valuetype_t vt = v->get_valuetype();
                if (vt == Value::V_ENUM || vt == Value::V_REFD) {
                  Free(rawpar->crosstaglist.tag[c].keyList[a].value);
                  rawpar->crosstaglist.tag[c].keyList[a].value =
                    mcopystr(v->get_single_expr().c_str());
                }
              }
            }
          }
        }
      }
      break; }
      case T_BSTR:
        if(rawattrib->fieldlength==0 && rawattrib->length_restrition!=-1){
          rawattrib->fieldlength=rawattrib->length_restrition;
          rawattrib->length_restrition=-1;
        }
        break;
      case T_HSTR:
        if(rawattrib->fieldlength==0 && rawattrib->length_restrition!=-1){
          rawattrib->fieldlength=rawattrib->length_restrition*4;
          rawattrib->length_restrition=-1;
        }
        break;
      case T_OSTR:
        if(rawattrib->fieldlength==0 && rawattrib->length_restrition!=-1){
          rawattrib->fieldlength=rawattrib->length_restrition*8;
          rawattrib->length_restrition=-1;
        }
        break;
      case T_CSTR:
      case T_USTR:
        if(rawattrib->fieldlength==0 && rawattrib->length_restrition!=-1){
          rawattrib->fieldlength=rawattrib->length_restrition*8;
          rawattrib->length_restrition=-1;
        }
        break;
      case T_SEQOF:
      case T_SETOF:
        get_ofType()->force_raw();
        if(rawattrib->fieldlength==0 && rawattrib->length_restrition!=-1){
          rawattrib->fieldlength=rawattrib->length_restrition;
          rawattrib->length_restrition=-1;
        }
        if(rawattrib->length_restrition!=-1 &&
                      rawattrib->length_restrition!=rawattrib->fieldlength){
          error("Invalid length specified in parameter FIELDLENGTH for %s of "
            "type `%s'. The FIELDLENGTH must be equal to specified length "
            "restriction", typetype == T_SEQOF ? "record" : "set",
            get_fullname().c_str());
        }
        break;
      case T_REAL:
        if(rawattrib->fieldlength!=64 && rawattrib->fieldlength!=32){
          error("Invalid length (%d) specified in parameter FIELDLENGTH for "
            "float type `%s'. The FIELDLENGTH must be single (32) or double "
            "(64)", rawattrib->fieldlength, get_fullname().c_str());
        }
        break;
      case T_INT:
        if (rawattrib->intx) {
          rawattrib->bitorderinfield = XDEFMSB;
          rawattrib->bitorderinoctet = XDEFMSB;
          rawattrib->byteorder = XDEFMSB;
        }
        break;
      case T_ENUM_T:
      case T_BOOL:
    default:
      // nothing to do, ASN1 types or types without defined raw attribute
      break;
    } // switch

    (void)self_ref;
  }

  void Type::force_raw()
  {
    if (!rawattrib)
    {
      switch (typetype) {
      case T_SEQOF:
      case T_SETOF:
      case T_CHOICE_T:
        // TODO case T_ANYTYPE: for force_raw ?
      case T_ENUM_T:
      case T_SEQ_T:
      case T_SET_T:
        rawattrib = new RawAST(false);
        break;
      default:
        if (is_ref()) get_type_refd()->force_raw();
        break;
      }
    }
    
    // Don't run chk_raw() on unchecked types
    if (chk_finished) 
      chk_raw();
  }

  void Type::chk_text()
  {
    if (text_checked) return;
    text_checked = true;
    if (!textattrib || !enable_text()) return;
//textattrib->print_TextAST();

    chk_text_matching_values(textattrib->begin_val, "BEGIN");
    chk_text_matching_values(textattrib->end_val, "END");
    chk_text_matching_values(textattrib->separator_val, "SEPARATOR");

    switch (typetype) {
    case T_ANYTYPE:
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T: {
      size_t nof_comps = get_nof_comps();
      for (size_t i = 0; i < nof_comps; i++)
        get_comp_byIndex(i)->get_type()->force_text();
      break; }
    case T_SEQOF:
    case T_SETOF:
      get_ofType()->force_text();
      break;
    default:
      if (is_ref()) get_type_refd()->force_text();
      break;
    }

    switch (get_type_refd_last()->typetype) {
    case T_BOOL:
      chk_text_matching_values(textattrib->true_params, "true value");
      chk_text_matching_values(textattrib->false_params, "false value");
      break;
    case T_ENUM_T:
      if(textattrib->nof_field_params){
        Type *t=get_type_refd_last();
        size_t nof_comps = t->u.enums.eis->get_nof_eis();
        textAST_enum_def **params=(textAST_enum_def**)
                Malloc(nof_comps*sizeof(textAST_enum_def*));
        memset(params,0,nof_comps*sizeof(textAST_enum_def*));
        for (size_t a = 0; a < textattrib->nof_field_params; a++) {
          const Identifier& id = *textattrib->field_params[a]->name;
          if (t->u.enums.eis->has_ei_withName(id)) {
            size_t index = t->get_eis_index_byName(id);
            if (params[index]) FATAL_ERROR("Type::chk_text(): duplicate " \
              "attribute for enum `%s'", id.get_dispname().c_str());
            params[index] = textattrib->field_params[a];
            char *attrib_name = mprintf("enumerated value `%s'",
              id.get_dispname().c_str());
            chk_text_matching_values(&params[index]->value, attrib_name);
            Free(attrib_name);
          } else {
              error("Coding attribute refers to non-existent enumerated value "
                "`%s'", id.get_dispname().c_str());
              Free(textattrib->field_params[a]->value.encode_token);
              Free(textattrib->field_params[a]->value.decode_token);
              delete textattrib->field_params[a]->name;
              Free(textattrib->field_params[a]);
          }
        }
        Free(textattrib->field_params);
        textattrib->field_params=params;
        textattrib->nof_field_params=nof_comps;
      }
      break;
    case T_OSTR:
    case T_CSTR:
    case T_INT:
      if (textattrib->decode_token) {
        char *tmp = textattrib->decode_token;
        textattrib->decode_token = process_decode_token(tmp, *this);
        Free(tmp);
        tmp = TTCN_pattern_to_regexp(textattrib->decode_token);
        if (tmp) Free(tmp);
        else {
          error("Incorrect select token expression: `%s'",
            textattrib->decode_token);
        }
      }
      break;
    default:
      break;
    }
//textattrib->print_TextAST();
  }

  void Type::chk_text_matching_values(textAST_matching_values *matching_values,
    const char *attrib_name)
  {
    if (!matching_values) return;
    if (matching_values->decode_token) {
      // check whether decode token is a correct TTCN-3 pattern
      char *tmp = matching_values->decode_token;
      matching_values->decode_token = process_decode_token(tmp, *this);
      Free(tmp);
      tmp = TTCN_pattern_to_regexp(matching_values->decode_token);
      if (tmp) Free(tmp);
      else {
        error("Incorrect matching expression for %s: `%s'", attrib_name,
          matching_values->decode_token);
      }
    } else if (matching_values->encode_token) {
      // the decode token is not present, but there is an encode token
      // derive the decode token from the encode token
      matching_values->generated_decode_token = true;
      matching_values->decode_token =
        convert_charstring_to_pattern(matching_values->encode_token);
    }
  }

  void Type::force_text()
  {
    if (!textattrib)
    {
      switch (typetype) {
      case T_SEQOF:
      case T_SETOF:
      case T_CHOICE_T:
        // TODO case T_ANYTYPE: for force_text ?
      case T_ENUM_T:
      case T_SEQ_T:
      case T_SET_T:
        textattrib = new TextAST;
        break;
      default:
        if (is_ref()) get_type_refd()->force_text();
        break;
      }
    }
    if (chk_finished)
      chk_text();
  }
  
  static const char* JSON_SCHEMA_KEYWORDS[] = {
    // built-in JSON schema keywords
    "$ref", "type", "properties", "items", "anyOf", "enum", "pattern",
    "default", "minItems", "maxItems", "additionalProperties", "fieldOrder",
    "required", "$schema", "minLength", "maxLength", "minimum", "maximum",
    "excludeMinimum", "excludeMaximum", "allOf"
    // TITAN-specific keywords
    "originalName", "unusedAlias", "subType", "numericValues", "omitAsNull",
    "encoding", "decoding", "valueList"
  };
  
  void Type::chk_json()
  {
    if (json_checked) return;
    json_checked = true;
    if ((NULL == jsonattrib && !hasEncodeAttr(get_encoding_name(CT_JSON))) || !enable_json()) return;

    switch (typetype) {
    case T_ANYTYPE:
    case T_CHOICE_T:
    case T_CHOICE_A:
    case T_SEQ_T:
    case T_SEQ_A:
    case T_SET_T:
    case T_SET_A: {
      size_t nof_comps = get_nof_comps();
      for (size_t i = 0; i < nof_comps; i++)
        get_comp_byIndex(i)->get_type()->force_json();
      break; }
    case T_SEQOF:
    case T_SETOF:
    case T_ARRAY:
      get_ofType()->force_json();
      break;
    default:
      if (is_ref()) get_type_refd()->force_json();
      break;
    }
    
    if (NULL != jsonattrib) {
      if (jsonattrib->omit_as_null && !is_optional_field()) {
        error("Invalid attribute, 'omit as null' requires optional "
          "field of a record or set.");
      }

      if (jsonattrib->as_value) {
        Type* last = get_type_refd_last();
        switch (last->typetype) {
        case T_CHOICE_T:
        case T_ANYTYPE:
          break; // OK
        case T_SEQ_T:
        case T_SET_T:
          if (last->get_nof_comps() == 1) {
            break; // OK
          }
          // else fall through
        default:
          error("Invalid attribute, 'as value' is only allowed for unions, "
            "the anytype, or records or sets with one field");
        }
      }

      if (NULL != jsonattrib->alias) {
        Type* parent = get_parent_type();
        if (NULL == parent || (T_SEQ_T != parent->typetype && 
            T_SET_T != parent->typetype && T_CHOICE_T != parent->typetype)) {
          error("Invalid attribute, 'name as ...' requires field of a "
            "record, set or union.");
        }
        if (NULL != parent && NULL != parent->jsonattrib && 
            parent->jsonattrib->as_value) {
          const char* parent_type_name = NULL;
          switch (parent->typetype) {
          case T_SEQ_T:
            if (parent->get_nof_comps() == 1) {
              parent_type_name = "record";
            }
            break;
          case T_SET_T:
            if (parent->get_nof_comps() == 1) {
              parent_type_name = "set";
            }
            break;
          case T_CHOICE_T:
            parent_type_name = "union";
            break;
          case T_ANYTYPE:
            parent_type_name = "anytype";
            break;
          default:
            break;
          }
          if (parent_type_name != NULL) {
            // parent_type_name remains null if the 'as value' attribute is set
            // for an invalid type
            warning("Attribute 'name as ...' will be ignored, because parent %s "
              "is encoded without field names.", parent_type_name);
          }
        }
      }

      if (NULL != jsonattrib->default_value) {
        chk_json_default();
      }
      
      const size_t nof_extensions = jsonattrib->schema_extensions.size();
      if (0 != nof_extensions) {
        const size_t nof_keywords = sizeof(JSON_SCHEMA_KEYWORDS) / sizeof(char*);
        
        // these keep track of erroneous extensions so each warning is only
        // displayed once
        char* checked_extensions = new char[nof_extensions];
        char* checked_keywords = new char[nof_keywords];
        memset(checked_extensions, 0, nof_extensions);
        memset(checked_keywords, 0, nof_keywords);

        for (size_t i = 0; i < nof_extensions; ++i) {
          for (size_t j = 0; j < nof_keywords; ++j) {
            if (0 == checked_extensions[i] && 0 == checked_keywords[j] &&
                0 == strcmp(jsonattrib->schema_extensions[i]->key,
                JSON_SCHEMA_KEYWORDS[j])) {
              // only report the warning once for each keyword
              warning("JSON schema keyword '%s' should not be used as the key of "
                "attribute 'extend'", JSON_SCHEMA_KEYWORDS[j]);
              checked_keywords[j] = 1;
              checked_extensions[i] = 1;
              break;
            }
          }
          if (0 == checked_extensions[i]) {
            for (size_t k = i + 1; k < nof_extensions; ++k) {
              if (0 == strcmp(jsonattrib->schema_extensions[i]->key,
                  jsonattrib->schema_extensions[k]->key)) {
                if (0 == checked_extensions[i]) {
                  // only report the warning once for each unique key
                  warning("Key '%s' is used multiple times in 'extend' attributes "
                    "of type '%s'", jsonattrib->schema_extensions[i]->key,
                    get_typename().c_str());
                  checked_extensions[i] = 1;
                }
                checked_extensions[k] = 1;
              }
            }
          }
        }
        delete[] checked_extensions;
        delete[] checked_keywords;
      }
      if (jsonattrib->metainfo_unbound) {
        Type* last = get_type_refd_last();
        Type* parent = get_parent_type();
        if (T_SEQ_T == last->typetype || T_SET_T == last->typetype) {
          // if it's set for the record/set, pass it onto its fields
          size_t nof_comps = last->get_nof_comps();
          if (jsonattrib->as_value && nof_comps == 1) {
            warning("Attribute 'metainfo for unbound' will be ignored, because "
              "the %s is encoded without field names.",
              T_SEQ_T == last->typetype ? "record" : "set");
          }
          else {
            for (size_t i = 0; i < nof_comps; i++) {
              Type* comp_type = last->get_comp_byIndex(i)->get_type();
              if (NULL == comp_type->jsonattrib) {
                comp_type->jsonattrib = new JsonAST;
              }
              comp_type->jsonattrib->metainfo_unbound = true;
            }
          }
        }
        else if (T_SEQOF != last->typetype && T_SETOF != last->typetype &&
                 T_ARRAY != last->typetype && (NULL == parent ||
                 (T_SEQ_T != parent->typetype && T_SET_T != parent->typetype))) {
          // only allowed if it's an array type or a field of a record/set
          error("Invalid attribute 'metainfo for unbound', requires record, set, "
            "record of, set of, array or field of a record or set");
        }
      }
    }
  }
  
  void Type::chk_json_default() 
  {
    const char* dval = jsonattrib->default_value;
    const size_t dval_len = strlen(dval);
    Type *last = get_type_refd_last();
    bool err = false;
    switch (last->typetype) {
    case T_BOOL:
      if (strcmp(dval, "true") != 0 && strcmp(dval, "false") != 0) {
        err = true;
      }
      break;
    case T_INT:
      for (size_t i = (dval[0] == '-') ? 1 : 0; i < dval_len; ++i) {
        if (dval[i] < '0' || dval[i] > '9') {
          err = true;
          break; // from the loop
        }
      }
      break;
    case T_REAL: {
      if (strcmp(dval, "infinity") == 0 || strcmp(dval, "-infinity") == 0 ||
          strcmp(dval, "not_a_number") == 0) {
        // special float values => skip the rest of the check
        break;
      }
      
      boolean first_digit = false; // first non-zero digit reached
      boolean zero = false; // first zero digit reached
      boolean decimal_point = false; // decimal point (.) reached
      boolean exponent_mark = false; // exponential mark (e or E) reached
      boolean exponent_sign = false; // sign of the exponential (- or +) reached

      size_t i = (dval[0] == '-') ? 1 : 0;
      while(!err && i < dval_len) {
        switch (dval[i]) {
        case '.':
          if (decimal_point || exponent_mark || (!first_digit && !zero)) {
            err = true;
          }
          decimal_point = true;
          first_digit = false;
          zero = false;
          break;
        case 'e':
        case 'E':
          if (exponent_mark || (!first_digit && !zero)) {
            err = true;
          }
          exponent_mark = true;
          first_digit = false;
          zero = false;
          break;
        case '0':
          if (!first_digit && (exponent_mark || (!decimal_point && zero))) {
            err = true;
          }
          zero = true;
          break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          if (!first_digit && zero && (!decimal_point || exponent_mark)) {
            err = true;
          }
          first_digit = true;
          break;
        case '-':
        case '+':
          if (exponent_sign || !exponent_mark || zero || first_digit) {
            err = true;
          }
          exponent_sign = true;
          break;
        default:
          err = true;
        }
        ++i;
      }
      err = !first_digit && !zero;
      break; }
    case T_BSTR:
      for (size_t i = 0; i < dval_len; ++i) {
        if (dval[i] < '0' || dval[i] > '1') {
          err = true;
          break; // from the loop
        }
      }
      break;
    case T_OSTR:
      if (dval_len % 2 != 0) {
        err = true;
        break;
      }
      // no break
    case T_HSTR:
      for (size_t i = 0; i < dval_len; ++i) {
        if ((dval[i] < '0' || dval[i] > '9') && (dval[i] < 'a' || dval[i] > 'f') &&
            (dval[i] < 'A' || dval[i] > 'F')) {
          err = true;
          break; // from the loop
        }
      }
      break;
    case T_CSTR:
    case T_USTR: {
      size_t i = 0;
      while(!err && i < dval_len) {
        if (dval[i] < 0 && last->typetype == T_CSTR) {
          err = true;
        }
        else if (dval[i] == '\\') {
          if (i == dval_len - 1) {
            err = true;
          } else {
            ++i;
            switch (dval[i]) {
            case '\\':
            case '\"':
            case 'n':
            case 't':
            case 'r':
            case 'f':
            case 'b':
            case '/':
              break; // these are OK
            case 'u': {
              if (i + 4 >= dval_len) {
                err = true;
              } else if (last->typetype == T_CSTR && 
                         (dval[i + 1] != '0' || dval[i + 2] != '0' ||
                         dval[i + 3] < '0' || dval[i + 3] > '7')) {
                err = true;
              } else {
                for (size_t j = (last->typetype == T_CSTR) ? 4 : 1; j <= 4; ++j) {
                  if ((dval[i + j] < '0' || dval[i + j] > '9') && 
                      (dval[i + j] < 'a' || dval[i + j] > 'f') &&
                      (dval[i + j] < 'A' || dval[i + j] > 'F')) {
                    err = true;
                    break; // from the loop
                  }
                }
              }
              i += 4;
              break; }
            default:
              err = true;
              break;
            }
          }
        }
        ++i;
      }
      break; }
    case T_ENUM_T: {
      Common::Identifier id(Identifier::ID_TTCN, string(dval));
      if (!last->has_ei_withName(id)) {
        err = true;
      }
      break; }
    case T_VERDICT:
      if (strcmp(dval, "none") != 0 && strcmp(dval, "pass") != 0 &&
          strcmp(dval, "inconc") != 0 && strcmp(dval, "fail") != 0 &&
          strcmp(dval, "error") != 0) {
        err = true;
      }
      break;
    default:
      error("JSON default values are not available for type `%s'",
        last->get_stringRepr().c_str());
      return;
    }
    
    if (err) {
      if (last->typetype == T_ENUM_T) {
        error("Invalid JSON default value for enumerated type `%s'",
          last->get_stringRepr().c_str());
      } else {
        error("Invalid %s JSON default value", get_typename_builtin(last->typetype));
      }
    }
  }
  
  void Type::force_json() 
  {
    if (!jsonattrib)
    {
      switch (typetype) {
      case T_SEQOF:
      case T_SETOF:
      case T_CHOICE_T:
      case T_CHOICE_A:
      case T_ENUM_T:
      case T_ENUM_A:
      case T_SEQ_T:
      case T_SEQ_A:
      case T_SET_T:
      case T_SET_A:
        jsonattrib = new JsonAST;
        break;
      default:
        if (is_ref()) get_type_refd()->force_json();
        break;
      }
    }
    if (chk_finished)
      chk_json();
  }


  int Type::get_length_multiplier()
  {
    switch(typetype) {
    case T_REFD:
      return get_type_refd()->get_length_multiplier();
      break;
    case T_HSTR:
      return 4;
      break;
    case T_OSTR:
    case T_CSTR:
    case T_USTR:
      return 8;
    default:
      return 1;
      break;
    }
    return 1;
  }

  /** \todo review, especially the string types... */
  bool Type::is_compatible_tt_tt(typetype_t p_tt1, typetype_t p_tt2,
                                 bool p_is_asn11, bool p_is_asn12,
                                 bool same_module)
  {
    if (p_tt2 == T_ERROR) return true;
    switch (p_tt1) {
      // error type is compatible with everything
    case T_ERROR:
      return true;
      // unambiguous built-in types
    case T_NULL:
    case T_BOOL:
    case T_REAL:
    case T_HSTR:
    case T_SEQOF:
    case T_SETOF:
    case T_VERDICT:
    case T_DEFAULT:
    case T_COMPONENT:
    case T_SIGNATURE:
    case T_PORT:
    case T_ARRAY:
    case T_FUNCTION:
    case T_ALTSTEP:
    case T_TESTCASE:
      return p_tt1 == p_tt2;
    case T_OSTR:
      return p_tt2==T_OSTR || (!p_is_asn11 && p_tt2==T_ANY);
    case T_USTR:
      switch (p_tt2) {
      case T_USTR:
      case T_UTF8STRING:
      case T_BMPSTRING:
      case T_UNIVERSALSTRING:
      case T_TELETEXSTRING:
      case T_VIDEOTEXSTRING:
      case T_GRAPHICSTRING:
      case T_OBJECTDESCRIPTOR:
      case T_GENERALSTRING:
      case T_CSTR:
      case T_NUMERICSTRING:
      case T_PRINTABLESTRING:
      case T_IA5STRING:
      case T_VISIBLESTRING:
      case T_UTCTIME:
      case T_GENERALIZEDTIME:
        return true;
      default:
        return false;
      }
    // character string group 1
    case T_UTF8STRING:
    case T_BMPSTRING:
    case T_UNIVERSALSTRING:
      switch (p_tt2) {
      case T_USTR:
      case T_UTF8STRING:
      case T_BMPSTRING:
      case T_UNIVERSALSTRING:
      case T_CSTR:
      case T_NUMERICSTRING:
      case T_PRINTABLESTRING:
      case T_IA5STRING:
      case T_VISIBLESTRING:
      case T_UTCTIME:
      case T_GENERALIZEDTIME:
        return true;
      default:
        return false;
      }
    // character string group 2
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
    case T_GRAPHICSTRING:
    case T_OBJECTDESCRIPTOR:
    case T_GENERALSTRING:
      switch (p_tt2) {
      case T_TELETEXSTRING:
      case T_VIDEOTEXSTRING:
      case T_GRAPHICSTRING:
      case T_OBJECTDESCRIPTOR:
      case T_GENERALSTRING:
      case T_CSTR:
      case T_NUMERICSTRING:
      case T_PRINTABLESTRING:
      case T_IA5STRING:
      case T_VISIBLESTRING:
      case T_UTCTIME:
      case T_GENERALIZEDTIME:
        return true;
      case T_USTR:
        // maybe :) is ustr.is_cstr()
        return true;
      default:
        return false;
      }
    // character string group 3
    case T_CSTR:
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_IA5STRING:
    case T_VISIBLESTRING:
    case T_UTCTIME:
    case T_GENERALIZEDTIME:
      switch (p_tt2) {
      case T_CSTR:
      case T_NUMERICSTRING:
      case T_PRINTABLESTRING:
      case T_IA5STRING:
      case T_VISIBLESTRING:
      case T_UTCTIME:
      case T_GENERALIZEDTIME:
        return true;
      default:
        return false;
      }
      // polymorphic built-in types
    case T_BSTR:
    case T_BSTR_A:
      return p_tt2 == T_BSTR || p_tt2 == T_BSTR_A;
    case T_INT:
    case T_INT_A:
      return p_tt2 == T_INT || p_tt2 == T_INT_A;
      // ROID is visible as OID from TTCN-3
    case T_OID:
      return p_tt2 == T_OID ||
        (!p_is_asn11 && p_tt2 == T_ROID);
    case T_ROID:
      return p_tt2 == T_ROID ||
        (!p_is_asn12 && p_tt2 == T_OID);
    case T_ENUM_A:
    case T_ENUM_T:
      return p_tt2==T_ENUM_A || p_tt2==T_ENUM_T;
    case T_CHOICE_T:
    case T_CHOICE_A:
    case T_OPENTYPE:
      return p_tt2==T_CHOICE_T || p_tt2==T_CHOICE_A || p_tt2==T_OPENTYPE;
    case T_SEQ_A:
    case T_SEQ_T:
      return p_tt2==T_SEQ_A || p_tt2==T_SEQ_T;
    case T_SET_A:
    case T_SET_T:
      return p_tt2==T_SET_A || p_tt2==T_SET_T;
    case T_ANY:
      return p_tt2 == T_ANY || p_tt2 == T_OSTR;
    case T_ANYTYPE:
      return p_tt2 == T_ANYTYPE && same_module; // Need same module
      // these should never appear?
    case T_REFD:
    case T_REFDSPEC:
    case T_OCFT:
    case T_ADDRESS:
      return false;
    default:
      FATAL_ERROR("Type::is_compatible_tt_tt()");
      return false;
    }
  }

  bool Type::is_compatible_tt(typetype_t p_tt, bool p_is_asn1, Type* p_type)
  {
    chk();
    Type *t1=get_type_refd_last();
    if (p_tt == T_ERROR) return true;
    switch (t1->typetype) {
      // these should never appear
    case T_REFD:
    case T_REFDSPEC:
    case T_OCFT:
    case T_ADDRESS:
      FATAL_ERROR("Type::is_compatible_tt()");
      return false;
    default: {
      bool same_mod = false;
      if (p_type && p_type->get_my_scope()) {
        if (t1->get_my_scope()->get_scope_mod() ==
            p_type->get_my_scope()->get_scope_mod()) {
          same_mod = true;
        }
      }
      return is_compatible_tt_tt(t1->typetype, p_tt, is_asn1(), p_is_asn1, same_mod);
    }
    }
  }

  bool Type::is_compatible(Type *p_type, TypeCompatInfo *p_info, Location* p_loc,
                           TypeChain *p_left_chain, TypeChain *p_right_chain,
                           bool p_is_inline_template)
  {
    chk();
    p_type->chk();
    Type *t1 = get_type_refd_last();
    Type *t2 = p_type->get_type_refd_last();
    // Error type is compatible with everything.
    if (t1->typetype == T_ERROR || t2->typetype == T_ERROR) return true;
    bool is_type_comp;
    switch (t1->typetype) {
    // Unambiguous built-in types.
    case T_NULL:
    case T_BOOL:
    case T_REAL:
    case T_HSTR:
    case T_VERDICT:
    case T_DEFAULT:
      is_type_comp = (t1->typetype == t2->typetype);
      break;
    case T_OSTR:
      is_type_comp = ( t2->typetype==T_OSTR || (!is_asn1() && t2->typetype==T_ANY) );
      break;
    case T_USTR:
      switch (t2->typetype) {
      case T_USTR:
      case T_UTF8STRING:
      case T_BMPSTRING:
      case T_UNIVERSALSTRING:
      case T_TELETEXSTRING:
      case T_VIDEOTEXSTRING:
      case T_GRAPHICSTRING:
      case T_OBJECTDESCRIPTOR:
      case T_GENERALSTRING:
      case T_CSTR:
      case T_NUMERICSTRING:
      case T_PRINTABLESTRING:
      case T_IA5STRING:
      case T_VISIBLESTRING:
      case T_UTCTIME:
      case T_GENERALIZEDTIME:
        is_type_comp = true;
        break;
      default:
        is_type_comp = false;
        break;
      }
      break;
    // Character string group 1.
    case T_UTF8STRING:
    case T_BMPSTRING:
    case T_UNIVERSALSTRING:
      switch (t2->typetype) {
      case T_USTR:
      case T_UTF8STRING:
      case T_BMPSTRING:
      case T_UNIVERSALSTRING:
      case T_CSTR:
      case T_NUMERICSTRING:
      case T_PRINTABLESTRING:
      case T_IA5STRING:
      case T_VISIBLESTRING:
      case T_UTCTIME:
      case T_GENERALIZEDTIME:
        is_type_comp = true;
        break;
      default:
        is_type_comp = false;
        break;
      }
      break;
    // Character string group 2.
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
    case T_GRAPHICSTRING:
    case T_OBJECTDESCRIPTOR:
    case T_GENERALSTRING:
      switch (t2->typetype) {
      case T_TELETEXSTRING:
      case T_VIDEOTEXSTRING:
      case T_GRAPHICSTRING:
      case T_OBJECTDESCRIPTOR:
      case T_GENERALSTRING:
      case T_CSTR:
      case T_NUMERICSTRING:
      case T_PRINTABLESTRING:
      case T_IA5STRING:
      case T_VISIBLESTRING:
      case T_UTCTIME:
      case T_GENERALIZEDTIME:
        is_type_comp = true;
        break;
      case T_USTR:
        // Maybe :) is ustr.is_cstr().
        is_type_comp = true;
        break;
      default:
        is_type_comp = false;
        break;
      }
      break;
    // Character string group 3.
    case T_CSTR:
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_IA5STRING:
    case T_VISIBLESTRING:
    case T_UTCTIME:
    case T_GENERALIZEDTIME:
      switch (t2->typetype) {
      case T_CSTR:
      case T_NUMERICSTRING:
      case T_PRINTABLESTRING:
      case T_IA5STRING:
      case T_VISIBLESTRING:
      case T_UTCTIME:
      case T_GENERALIZEDTIME:
        is_type_comp = true;
        break;
      default:
        is_type_comp = false;
        break;
      }
      break;
    // Polymorphic built-in types.
    case T_BSTR:
    case T_BSTR_A:
      is_type_comp = ( t2->typetype == T_BSTR || t2->typetype == T_BSTR_A );
      break;
    case T_INT:
    case T_INT_A:
      is_type_comp = ( t2->typetype == T_INT || t2->typetype == T_INT_A );
      break;
    // ROID is visible as OID from TTCN-3.
    case T_OID:
      is_type_comp = ( t2->typetype == T_OID || (!is_asn1() && t2->typetype == T_ROID) );
      break;
    case T_ROID:
      is_type_comp = ( t2->typetype == T_ROID || (!p_type->is_asn1() && t2->typetype == T_OID) );
      break;
    case T_COMPONENT:
      is_type_comp = ( t2->typetype == T_COMPONENT && t1->u.component->is_compatible(t2->u.component) );
      break;
    case T_SEQ_A:
    case T_SEQ_T:
      is_type_comp = t1->is_compatible_record(t2, p_info, p_loc, p_left_chain, p_right_chain, p_is_inline_template);
      break;
    case T_SEQOF:
      is_type_comp = t1->is_compatible_record_of(t2, p_info, p_loc, p_left_chain, p_right_chain, p_is_inline_template);
      break;
    case T_SET_A:
    case T_SET_T:
      is_type_comp = t1->is_compatible_set(t2, p_info, p_loc, p_left_chain, p_right_chain, p_is_inline_template);
      break;
    case T_SETOF:
      is_type_comp = t1->is_compatible_set_of(t2, p_info, p_loc, p_left_chain, p_right_chain, p_is_inline_template);
      break;
    case T_ARRAY:
      is_type_comp = t1->is_compatible_array(t2, p_info, p_loc, p_left_chain, p_right_chain, p_is_inline_template);
      break;
    case T_CHOICE_T:
    case T_CHOICE_A:
    case T_ANYTYPE:
      is_type_comp = t1->is_compatible_choice_anytype(t2, p_info, p_loc, p_left_chain, p_right_chain, p_is_inline_template);
      break;
    case T_ENUM_A:
    case T_ENUM_T:
    case T_SIGNATURE:
    case T_PORT:
    case T_OPENTYPE:
      is_type_comp = ( t1 == t2 );
      break;
    case T_FUNCTION:
    case T_ALTSTEP:
    case T_TESTCASE:
      // TODO: Compatibility.
      is_type_comp = ( t1 == t2 );
      break;
    case T_ANY:
      is_type_comp = ( t2->typetype == T_ANY || t2->typetype == T_OSTR );
      break;
    default:
      FATAL_ERROR("Type::is_compatible()");
    }
    // if types are compatible then check subtype compatibility
    // skip check if p_info is NULL
    if ((p_info!=NULL) && is_type_comp && (sub_type!=NULL) && (p_type->sub_type!=NULL) &&
        (sub_type->get_subtypetype()==p_type->sub_type->get_subtypetype()))
    {
      if (p_info->get_str1_elem()) {
        if (p_info->get_str2_elem()) {
          // both are string elements -> nothing to do
        } else {
          // char <-> string
          if (!p_type->sub_type->is_compatible_with_elem()) {
            is_type_comp = false;
            p_info->set_subtype_error(
              string("Subtype mismatch: string element has no common value with subtype ")+
              p_type->sub_type->to_string());
          }
        }
      } else {
        if (p_info->get_str2_elem()) {
          // string <-> char
          if (!sub_type->is_compatible_with_elem()) {
            is_type_comp = false;
            p_info->set_subtype_error(string("Subtype mismatch: subtype ")+
              sub_type->to_string()+string(" has no common value with a string element"));
          }
        } else {
          // string <-> string
          if (!sub_type->is_compatible(p_type->sub_type)) {
            is_type_comp = false;
            p_info->set_subtype_error(string("Subtype mismatch: subtype ")+
              sub_type->to_string()+string(" has no common value with subtype ")+
              p_type->sub_type->to_string());
          }
        }
      }
    }
    if (is_type_comp && p_loc != NULL) {
      // issue a warning for deprecated compatibilities:
      // - records and record ofs/arrays
      // - sets and set ofs
      bool is_record = t1->typetype == T_SEQ_T || t1->typetype == T_SEQ_A ||
        t2->typetype == T_SEQ_T || t2->typetype == T_SEQ_A;
      bool is_record_of = t1->typetype == T_SEQOF || t2->typetype == T_SEQOF;
      bool is_array = t1->typetype == T_ARRAY || t2->typetype == T_ARRAY;
      bool is_set = t1->typetype == T_SET_T || t1->typetype == T_SET_A ||
        t2->typetype == T_SET_T || t2->typetype == T_SET_A;
      bool is_set_of = t1->typetype == T_SETOF || t2->typetype == T_SETOF;
      if ((is_record && (is_record_of || is_array)) ||
          (is_set && is_set_of)) {
        p_loc->warning("Compatibility between %s types and %s types is "
          "deprecated.", is_record ? "record" : "set",
          is_record_of ? "record of" : (is_set_of ? "set of" : "array"));
      }
    }
    return is_type_comp;
  }

  bool Type::is_structured_type() const
  {
    switch (typetype) {
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SEQOF:
    case T_ARRAY:
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_ANYTYPE:
    case T_SET_A:
    case T_SET_T:
    case T_SETOF:
      return true;
    default:
      return false;
    }
  }

  bool Type::is_subtype_length_compatible(Type *p_type)
  {
    if (typetype != T_SEQOF && typetype != T_SETOF)
      FATAL_ERROR("Type::is_subtype_length_compatible()");
    if (!sub_type) return true;
    SubtypeConstraint::subtype_t st_t = typetype == T_SEQOF ?
      SubtypeConstraint::ST_RECORDOF : SubtypeConstraint::ST_SETOF;
    switch (p_type->typetype) {
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SET_A:
    case T_SET_T: {
      vector<SubTypeParse> p_stp_v;
      Value *p_nof_comps = new Value(Value::V_INT,
                                     new int_val_t(p_type->get_nof_comps()));
      p_stp_v.add(new SubTypeParse(new Ttcn::LengthRestriction(p_nof_comps)));
      SubType p_st(st_t, this, NULL, &p_stp_v, NULL);
      p_st.chk();
      delete p_stp_v[0];
      p_stp_v.clear();
      return sub_type->is_length_compatible(&p_st); }
    case T_SEQOF:
    case T_SETOF:
      if (!p_type->sub_type) return true;
      else return sub_type->is_length_compatible(p_type->sub_type);
    case T_ARRAY: {
      if (p_type->u.array.dimension->get_has_error()) return false;
      vector<SubTypeParse> p_stp_v;
      Value *p_nof_comps
        = new Value(Value::V_INT,
                    new int_val_t(p_type->u.array.dimension->get_size()));
      p_stp_v.add(new SubTypeParse(new Ttcn::LengthRestriction(p_nof_comps)));
      SubType p_st(st_t, this, NULL, &p_stp_v, NULL);
      p_st.chk();  // Convert SubTypeParse to SubType.
      delete p_stp_v[0];
      p_stp_v.clear();
      return sub_type->is_length_compatible(&p_st); }
    default:
      FATAL_ERROR("Type::is_subtype_length_compatible()");
    }
  }

  // Errors and warnings are reported in an upper level.  We just make a
  // simple decision here.
  bool Type::is_compatible_record(Type *p_type, TypeCompatInfo *p_info,
                                  Location* p_loc,
                                  TypeChain *p_left_chain,
                                  TypeChain *p_right_chain,
                                  bool p_is_inline_template)
  {
    if (typetype != T_SEQ_A && typetype != T_SEQ_T)
      FATAL_ERROR("Type::is_compatible_record()");
    // The get_type_refd_last() was called for both Types at this point.  All
    // this code runs in both run-times.
    if (this == p_type) return true;
    else if (!use_runtime_2 || !p_info
             || (p_info && p_info->is_strict())) return false;
    size_t nof_comps = get_nof_comps();
    switch (p_type->typetype) {
    case T_SEQ_A:
    case T_SEQ_T: {
      // According to 6.3.2.2 the # of fields and the optionality must be
      // the same for record types.  It's good news for compile-time checks.
      // Conversion is always from "p_type -> this".
      size_t p_nof_comps = p_type->get_nof_comps();
      if (nof_comps != p_nof_comps) {
        p_info->set_is_erroneous(this, p_type, string("The number of fields in "
                                 "record/SEQUENCE types must be the same"));
        return false;
      }
      // If p_info is present we have the chains as well.
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      for (size_t i = 0; i < nof_comps; i++) {
        CompField *cf = get_comp_byIndex(i);
        CompField *p_cf = p_type->get_comp_byIndex(i);
        Type *cf_type = cf->get_type()->get_type_refd_last();
        Type *p_cf_type = p_cf->get_type()->get_type_refd_last();
        string cf_name = cf->get_name().get_dispname();
        string p_cf_name = p_cf->get_name().get_dispname();
        if (cf->get_is_optional() != p_cf->get_is_optional()) {
          p_info->append_ref_str(0, "." + cf_name);
          p_info->append_ref_str(1, "." + p_cf_name);
          p_info->set_is_erroneous(cf_type, p_cf_type, string("The optionality of "
                                   "fields in record/SEQUENCE types must be "
                                   "the same"));
          return false;
        }
        TypeCompatInfo info_tmp(p_info->get_my_module(), cf_type, p_cf_type,
                                false, false);
        p_left_chain->mark_state();
        p_right_chain->mark_state();
        p_left_chain->add(cf_type);
        p_right_chain->add(p_cf_type);
        if (cf_type != p_cf_type
            && !(p_left_chain->has_recursion() && p_right_chain->has_recursion())
            && !cf_type->is_compatible(p_cf_type, &info_tmp, p_loc, p_left_chain,
                                       p_right_chain, p_is_inline_template)) {
          p_info->append_ref_str(0, "." + cf_name + info_tmp.get_ref_str(0));
          p_info->append_ref_str(1, "." + p_cf_name + info_tmp.get_ref_str(1));
          p_info->set_is_erroneous(info_tmp.get_type(0), info_tmp.get_type(1),
                                   info_tmp.get_error_str());
          p_left_chain->previous_state();
          p_right_chain->previous_state();
          return false;
        }
        p_left_chain->previous_state();
        p_right_chain->previous_state();
      }
      if (!p_is_inline_template) {
        p_info->set_needs_conversion(true);
        p_info->add_type_conversion(p_type, this);
      }
      return true; }
    case T_SEQOF:
      if (!p_type->is_subtype_length_compatible(this)) {
        p_info->set_is_erroneous(this, p_type, string("Incompatible record of/SEQUENCE "
                                 "OF subtypes"));
        return false;
      }
      // no break
    case T_ARRAY: {
      Type *p_of_type = p_type->get_ofType()->get_type_refd_last();
      if (p_type->typetype == T_ARRAY) {
        if (p_of_type->get_typetype() == T_ARRAY) {
          p_info->set_is_erroneous(this, p_type, string("record/SEQUENCE types are "
                                   "compatible only with single-dimension "
                                   "arrays"));
          return false;
        }
        size_t nof_opt_fields = 0;
        for (size_t i = 0; i < nof_comps; i++)
          if (get_comp_byIndex(i)->get_is_optional()) nof_opt_fields++;
        if (p_type->u.array.dimension->get_size()
            < nof_comps - nof_opt_fields) {
          p_info->set_is_erroneous(this, p_type, string("The dimension of the array "
                                   "must be >= than the number of mandatory "
                                   "fields in the record/SEQUENCE type"));
          return false;
        }
      }
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      for (size_t i = 0; i < nof_comps; i++) {
        Type *cf_type = get_comp_byIndex(i)->get_type()->get_type_refd_last();
        TypeCompatInfo info_tmp(p_info->get_my_module(), cf_type, p_of_type,
                                false, false);
        p_left_chain->mark_state();
        p_right_chain->mark_state();
        p_left_chain->add(cf_type);
        p_right_chain->add(p_of_type);
        if (cf_type != p_of_type
            && !(p_left_chain->has_recursion() && p_right_chain->has_recursion())
            && !cf_type->is_compatible(p_of_type, &info_tmp, p_loc, p_left_chain,
                                       p_right_chain, p_is_inline_template)) {
          p_info->append_ref_str(0, "." + get_comp_byIndex(i)
            ->get_name().get_dispname() + info_tmp.get_ref_str(0));
          p_info->append_ref_str(1, "[]" + info_tmp.get_ref_str(1));
          p_info->set_is_erroneous(info_tmp.get_type(0), info_tmp.get_type(1),
                                   info_tmp.get_error_str());
          p_left_chain->previous_state();
          p_right_chain->previous_state();
          return false;
        }
        p_left_chain->previous_state();
        p_right_chain->previous_state();
      }
      if (!p_is_inline_template) {
        p_info->set_needs_conversion(true);
        p_info->add_type_conversion(p_type, this);
      }
      return true; }
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_ANYTYPE:
      // 6.3.2.4 makes our job very easy...
      p_info->set_is_erroneous(this, p_type, string("union/CHOICE/anytype types are "
                               "compatible only with other "
                               "union/CHOICE/anytype types"));
      return false;
    case T_SET_A:
    case T_SET_T:
    case T_SETOF:
      // Only set/set of types are compatible with other set/set of types.
      // 6.3.2.3 is a little bit unclear about set of types, but we treat them
      // this way.  Otherwise, it would be possible to use compatibility with
      // a "middle-man" set of variable between record/set types:
      // type set S  { integer f1, integer f2 }
      // type record { integer f1, integer f2 }
      // type set of integer SO
      // var S s := { 1, 2 }
      // var R r := { 1, 2 }
      // var SO so
      // so := s
      // if (r == s)  { ... }  // Not allowed.
      // if (r == so) { ... }  // Not allowed.  (?)
      // Seems to be a fair decision.  If we would want compatibility between
      // variables of record/set types, we should allow it directly.
      p_info->set_is_erroneous(this, p_type, string("set/SET and set of/SET OF "
                               "types are compatible only with other set/SET "
                               "set of/SET OF types"));
      return false;
    default:
      return false;
    }
  }

  bool Type::is_compatible_record_of(Type *p_type, TypeCompatInfo *p_info,
                                     Location* p_loc,
                                     TypeChain *p_left_chain,
                                     TypeChain *p_right_chain,
                                     bool p_is_inline_template)
  {
    if (typetype != T_SEQOF) FATAL_ERROR("Type::is_compatible_record_of()");
    if (this == p_type) return true;
    else if (T_SEQOF == p_type->get_type_refd_last()->typetype &&
             is_pregenerated() && p_type->is_pregenerated() &&
             get_ofType()->get_type_refd_last()->typetype ==
             p_type->get_ofType()->get_type_refd_last()->typetype &&
             (use_runtime_2 || get_optimize_attribute() == p_type->get_optimize_attribute())) {
      // Pre-generated record-ofs of the same element type are compatible with 
      // each other (in RT1 optimized record-ofs are not compatible with non-optimized ones)
      if (!is_subtype_length_compatible(p_type)) {
        p_info->set_is_erroneous(this, p_type, string("Incompatible "
                                 "record of/SEQUENCE OF subtypes"));
        return false;
      }
      return true;
    }
    else if (!use_runtime_2 || !p_info
             || (p_info && p_info->is_strict())) return false;
    switch (p_type->typetype) {
    case T_SEQ_A:
    case T_SEQ_T: {
      if (!is_subtype_length_compatible(p_type)) {
        p_info->set_is_erroneous(this, p_type, string("Incompatible "
                                 "record of/SEQUENCE OF subtypes"));
        return false;
      }
      Type *of_type = get_ofType()->get_type_refd_last();
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      for (size_t i = 0; i < p_type->get_nof_comps(); i++) {
        CompField *p_cf = p_type->get_comp_byIndex(i);
        Type *p_cf_type = p_cf->get_type()->get_type_refd_last();
        TypeCompatInfo info_tmp(p_info->get_my_module(), of_type, p_cf_type,
                                false, false);
        p_left_chain->mark_state();
        p_right_chain->mark_state();
        p_left_chain->add(of_type);
        p_right_chain->add(p_cf_type);
        if (of_type != p_cf_type
            && !(p_left_chain->has_recursion() && p_right_chain->has_recursion())
            && !of_type->is_compatible(p_cf_type, &info_tmp, p_loc, p_left_chain,
                                       p_right_chain, p_is_inline_template)) {
          p_info->append_ref_str(0, "[]" + info_tmp.get_ref_str(0));
          p_info->append_ref_str(1, "." + p_cf->get_name().get_dispname() +
                                 info_tmp.get_ref_str(1));
          p_info->set_is_erroneous(info_tmp.get_type(0),
                                   info_tmp.get_type(1),
                                   info_tmp.get_error_str());
          p_left_chain->previous_state();
          p_right_chain->previous_state();
          return false;
        }
        p_left_chain->previous_state();
        p_right_chain->previous_state();
      }
      if (!p_is_inline_template) {
        p_info->set_needs_conversion(true);
        p_info->add_type_conversion(p_type, this);
      }
      return true; }
    case T_SEQOF:
    case T_ARRAY: {
      if (!is_subtype_length_compatible(p_type)) {
        p_info->set_is_erroneous(this, p_type, string("Incompatible "
                                 "record of/SEQUENCE OF subtypes"));
        return false;
      }
      Type *of_type = get_ofType()->get_type_refd_last();
      Type *p_of_type = p_type->get_ofType()->get_type_refd_last();
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      TypeCompatInfo info_tmp(p_info->get_my_module(), of_type, p_of_type,
                              false, false);
      p_left_chain->mark_state();
      p_right_chain->mark_state();
      p_left_chain->add(of_type);
      p_right_chain->add(p_of_type);
      if (of_type == p_of_type
          || (p_left_chain->has_recursion() && p_right_chain->has_recursion())
          || of_type->is_compatible(p_of_type, &info_tmp, p_loc, p_left_chain,
                                    p_right_chain, p_is_inline_template)) {
        if (!p_is_inline_template) {
          p_info->set_needs_conversion(true);
          p_info->add_type_conversion(p_type, this);
        }
        p_left_chain->previous_state();
        p_right_chain->previous_state();
        return true;
      }
      p_left_chain->previous_state();
      p_right_chain->previous_state();
      p_info->append_ref_str(0, "[]" + info_tmp.get_ref_str(0));
      // Arrays already have the "[]" in their names.
      if (p_type->get_typetype() != T_ARRAY) p_info->append_ref_str(1, string("[]"));
      p_info->append_ref_str(1, info_tmp.get_ref_str(1));
      p_info->set_is_erroneous(info_tmp.get_type(0), info_tmp.get_type(1),
                               info_tmp.get_error_str());
      return false; }
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_ANYTYPE:
      p_info->set_is_erroneous(this, p_type, string("union/CHOICE/anytype types are "
                               "compatible only with other "
                               "union/CHOICE/anytype types"));
      return false;
    case T_SET_A:
    case T_SET_T:
    case T_SETOF:
      p_info->set_is_erroneous(this, p_type, string("set/SET and set of/SET OF "
                               "types are compatible only with other set/SET "
                               "set of/SET OF types"));
      return false;
    default:
      return false;
    }
  }

  bool Type::is_compatible_array(Type *p_type, TypeCompatInfo *p_info,
                                 Location* p_loc,
                                 TypeChain *p_left_chain,
                                 TypeChain *p_right_chain,
                                 bool p_is_inline_template)
  {
    if (typetype != T_ARRAY) FATAL_ERROR("Type::is_compatible_array()");
    // Copied from the original checker code.  The type of the elements and
    // the dimension of the array must be the same.
    if (this == p_type) return true;
    if (p_type->typetype == T_ARRAY && u.array.element_type
        ->is_compatible(p_type->u.array.element_type, NULL, p_loc, NULL, NULL, p_is_inline_template)
        && u.array.dimension->is_identical(p_type->u.array.dimension))
      return true;
    else if (!use_runtime_2 || !p_info
             || (p_info && p_info->is_strict())) return false;
    Type *of_type = get_ofType()->get_type_refd_last();
    switch (p_type->get_typetype()) {
    case T_SEQ_A:
    case T_SEQ_T: {
      if (of_type->get_typetype() == T_ARRAY) {
        p_info->set_is_erroneous(this, p_type, string("record/SEQUENCE types are "
                                 "compatible only with single-dimension "
                                 "arrays"));
        return false;
      }
      size_t p_nof_comps = p_type->get_nof_comps();
      size_t p_nof_opt_fields = 0;
      for (size_t i = 0; i < p_nof_comps; i++)
        if (p_type->get_comp_byIndex(i)->get_is_optional())
          p_nof_opt_fields++;
      if (u.array.dimension->get_size() < p_nof_comps - p_nof_opt_fields) {
        p_info->set_is_erroneous(this, p_type, string("The dimension of the array "
                                 "must be >= than the number of mandatory "
                                 "fields in the record/SEQUENCE type"));
        return false;
      }
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      for (size_t i = 0; i < p_nof_comps; ++i) {
        CompField *p_cf = p_type->get_comp_byIndex(i);
        Type *p_cf_type = p_cf->get_type()->get_type_refd_last();
        string p_cf_name = p_cf->get_name().get_dispname();
        TypeCompatInfo info_tmp(p_info->get_my_module(), of_type, p_cf_type,
                                false, false);
        p_left_chain->mark_state();
        p_right_chain->mark_state();
        p_left_chain->add(of_type);
        p_right_chain->add(p_cf_type);
        if (of_type != p_cf_type
            && !(p_left_chain->has_recursion() && p_right_chain->has_recursion())
            && !of_type->is_compatible(p_cf_type, &info_tmp, p_loc, p_left_chain,
                                       p_right_chain, p_is_inline_template)) {
          p_info->append_ref_str(0, info_tmp.get_ref_str(0));
          p_info->append_ref_str(1, "." + p_cf->get_name().get_dispname() +
                                 info_tmp.get_ref_str(1));
          p_info->set_is_erroneous(info_tmp.get_type(0),
                                   info_tmp.get_type(1),
                                   info_tmp.get_error_str());
          p_left_chain->previous_state();
          p_right_chain->previous_state();
          return false;
        }
        p_left_chain->previous_state();
        p_right_chain->previous_state();
      }
      if (!p_is_inline_template) {
        p_info->set_needs_conversion(true);
        p_info->add_type_conversion(p_type, this);
      }
      return true; }
    case T_SEQOF:
      if (!p_type->is_subtype_length_compatible(this)) {
        p_info->set_is_erroneous(this, p_type, string("Incompatible "
                                 "record of/SEQUENCE OF subtypes"));
        return false;
      }  // Don't break.
    case T_ARRAY: {
      if (p_type->get_typetype() == T_ARRAY
          && !u.array.dimension->is_identical(p_type->u.array.dimension)) {
        p_info->set_is_erroneous(this, p_type, string("Array types should have "
                                 "the same dimension"));
        return false;
      }
      Type *p_of_type = p_type->get_ofType()->get_type_refd_last();
      TypeCompatInfo info_tmp(p_info->get_my_module(), of_type, p_of_type,
                              false, false);
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      p_left_chain->mark_state();
      p_right_chain->mark_state();
      p_left_chain->add(of_type);
      p_right_chain->add(p_of_type);
      if (of_type == p_of_type
          || (p_left_chain->has_recursion() && p_right_chain->has_recursion())
          || of_type->is_compatible(p_of_type, &info_tmp, p_loc, p_left_chain,
                                    p_right_chain, p_is_inline_template)) {
        if (!p_is_inline_template) {
          p_info->set_needs_conversion(true);
          p_info->add_type_conversion(p_type, this);
        }
        p_left_chain->previous_state();
        p_right_chain->previous_state();
        return true;
      }
      p_left_chain->previous_state();
      p_right_chain->previous_state();
      p_info->append_ref_str(0, info_tmp.get_ref_str(0));
      if (p_type->get_typetype() != T_ARRAY) p_info->append_ref_str(1, string("[]"));
      p_info->append_ref_str(1, info_tmp.get_ref_str(1));
      p_info->set_is_erroneous(info_tmp.get_type(0), info_tmp.get_type(1),
                               info_tmp.get_error_str());
      return false; }
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_ANYTYPE:
      p_info->set_is_erroneous(this, p_type, string("union/CHOICE/anytype types are "
                               "compatible only with other "
                               "union/CHOICE/anytype types"));
      return false;
    case T_SET_A:
    case T_SET_T:
    case T_SETOF:
      p_info->set_is_erroneous(this, p_type, string("set/SET and set of/SET OF "
                               "types are compatible only with other set/SET "
                               "set of/SET OF types"));
      return false;
    default:
      return false;
    }
  }

  bool Type::is_compatible_set(Type *p_type, TypeCompatInfo *p_info,
                               Location* p_loc,
                               TypeChain *p_left_chain,
                               TypeChain *p_right_chain,
                               bool p_is_inline_template)
  {
    if (typetype != T_SET_A && typetype != T_SET_T)
      FATAL_ERROR("Type::is_compatible_set()");
    if (this == p_type) return true;
    else if (!use_runtime_2 || !p_info
             || (p_info && p_info->is_strict())) return false;
    size_t nof_comps = get_nof_comps();
    switch (p_type->typetype) {
    case T_SET_A:
    case T_SET_T: {
      // The standard is very generous.  We don't need to check for a possible
      // combination of compatible fields.  According to 6.3.2.3, simply do
      // the same thing as for T_SEQ_{A,T} types.  The fields are in their
      // textual order.
      size_t p_nof_comps = p_type->get_nof_comps();
      if (nof_comps != p_nof_comps) {
        p_info->set_is_erroneous(this, p_type, string("The number of fields in "
                                 "set/SET types must be the same"));
        return false;
      }
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      for (size_t i = 0; i < nof_comps; i++) {
        CompField *cf = get_comp_byIndex(i);
        CompField *p_cf = p_type->get_comp_byIndex(i);
        Type *cf_type = cf->get_type()->get_type_refd_last();
        Type *p_cf_type = p_cf->get_type()->get_type_refd_last();
        string cf_name = cf->get_name().get_dispname();
        string p_cf_name = p_cf->get_name().get_dispname();
        if (cf->get_is_optional() != p_cf->get_is_optional()) {
          p_info->append_ref_str(0, "." + cf_name);
          p_info->append_ref_str(1, "." + p_cf_name);
          p_info->set_is_erroneous(cf_type, p_cf_type, string("The optionality of "
                                   "fields in set/SET types must be the "
                                   "same"));
          return false;
        }
        TypeCompatInfo info_tmp(p_info->get_my_module(), cf_type, p_cf_type,
                                false, false);
        p_left_chain->mark_state();
        p_right_chain->mark_state();
        p_left_chain->add(cf_type);
        p_right_chain->add(p_cf_type);
        if (cf_type != p_cf_type
            && !(p_left_chain->has_recursion() && p_right_chain->has_recursion())
            && !cf_type->is_compatible(p_cf_type, &info_tmp, p_loc, p_left_chain,
                                       p_right_chain, p_is_inline_template)) {
          p_info->append_ref_str(0, "." + cf_name + info_tmp.get_ref_str(0));
          p_info->append_ref_str(1, "." + p_cf_name + info_tmp.get_ref_str(1));
          p_info->set_is_erroneous(info_tmp.get_type(0), info_tmp.get_type(1),
                                   info_tmp.get_error_str());
          p_left_chain->previous_state();
          p_right_chain->previous_state();
          return false;
        }
        p_left_chain->previous_state();
        p_right_chain->previous_state();
      }
      if (!p_is_inline_template) {
        p_info->set_needs_conversion(true);
        p_info->add_type_conversion(p_type, this);
      }
      return true; }
    case T_SETOF: {
      if (!p_type->is_subtype_length_compatible(this)) {
        p_info->set_is_erroneous(this, p_type, string("Incompatible set of/SET OF "
                                 "subtypes"));
        return false;
      }
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      Type *p_of_type = p_type->get_ofType()->get_type_refd_last();
      for (size_t i = 0; i < nof_comps; i++) {
        Type *cf_type = get_comp_byIndex(i)->get_type()->get_type_refd_last();
        TypeCompatInfo info_tmp(p_info->get_my_module(), cf_type, p_of_type,
                                false, false);
        p_left_chain->mark_state();
        p_right_chain->mark_state();
        p_left_chain->add(cf_type);
        p_right_chain->add(p_of_type);
        if (cf_type != p_of_type
            && !(p_left_chain->has_recursion() && p_right_chain->has_recursion())
            && !cf_type->is_compatible(p_of_type, &info_tmp, p_loc, p_left_chain,
                                       p_right_chain, p_is_inline_template)) {
          p_info->append_ref_str(0, "." + get_comp_byIndex(i)
            ->get_name().get_dispname() + info_tmp.get_ref_str(0));
          p_info->append_ref_str(1, "[]" + info_tmp.get_ref_str(1));
          p_info->set_is_erroneous(info_tmp.get_type(0), info_tmp.get_type(1),
                                   info_tmp.get_error_str());
          p_left_chain->previous_state();
          p_right_chain->previous_state();
          return false;
        }
        p_left_chain->previous_state();
        p_right_chain->previous_state();
      }
      if (!p_is_inline_template) {
        p_info->set_needs_conversion(true);
        p_info->add_type_conversion(p_type, this);
      }
      return true; }
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_ANYTYPE:
      p_info->set_is_erroneous(this, p_type, string("union/CHOICE/anytype types are "
                               "compatible only with other "
                               "union/CHOICE/anytype types"));
      return false;
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SEQOF:
    case T_ARRAY:
      p_info->set_is_erroneous(this, p_type, string("set/SET and set of/SET OF "
                               "types are compatible only with other set/SET "
                               "set of/SET OF types"));
      return false;
    default:
      return false;
    }
  }

  bool Type::is_compatible_set_of(Type *p_type, TypeCompatInfo *p_info,
                                  Location* p_loc,
                                  TypeChain *p_left_chain,
                                  TypeChain *p_right_chain,
                                  bool p_is_inline_template)
  {
    if (typetype != T_SETOF) FATAL_ERROR("Type::is_compatible_set_of()");
    if (this == p_type) return true;
    else if (T_SETOF == p_type->get_type_refd_last()->typetype &&
             is_pregenerated() && p_type->is_pregenerated() &&
             get_ofType()->get_type_refd_last()->typetype ==
             p_type->get_ofType()->get_type_refd_last()->typetype &&
             (use_runtime_2 || get_optimize_attribute() == p_type->get_optimize_attribute())) {
      // Pre-generated set-ofs of the same element type are compatible with 
      // each other (in RT1 optimized set-ofs are not compatible with non-optimized ones)
      if (!is_subtype_length_compatible(p_type)) {
        p_info->set_is_erroneous(this, p_type, string("Incompatible "
                                 "set of/SET OF subtypes"));
        return false;
      }
      return true;
    }
    else if (!use_runtime_2 || !p_info
             || (p_info && p_info->is_strict())) return false;
    Type *of_type = get_ofType();
    switch (p_type->get_typetype()) {
    case T_SET_A:
    case T_SET_T: {
      if (!is_subtype_length_compatible(p_type)) {
        p_info->set_is_erroneous(this, p_type, string("Incompatible set of/SET OF "
                                 "subtypes"));
        return false;
      }
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      for (size_t i = 0; i < p_type->get_nof_comps(); i++) {
        CompField *p_cf = p_type->get_comp_byIndex(i);
        Type *p_cf_type = p_cf->get_type()->get_type_refd_last();
        TypeCompatInfo info_tmp(p_info->get_my_module(), of_type, p_cf_type,
                                false, false);
        p_left_chain->mark_state();
        p_right_chain->mark_state();
        p_left_chain->add(of_type);
        p_right_chain->add(p_cf_type);
        if (of_type != p_cf_type
            && !(p_left_chain->has_recursion() && p_right_chain->has_recursion())
            && !of_type->is_compatible(p_cf_type, &info_tmp, p_loc, p_left_chain,
                                       p_right_chain, p_is_inline_template)) {
          p_info->append_ref_str(0, "[]" + info_tmp.get_ref_str(0));
          p_info->append_ref_str(1, "." + p_cf->get_name().get_dispname() +
                                 info_tmp.get_ref_str(1));
          p_info->set_is_erroneous(info_tmp.get_type(0), info_tmp.get_type(1),
                                   info_tmp.get_error_str());
          p_left_chain->previous_state();
          p_right_chain->previous_state();
          return false;
        }
        p_left_chain->previous_state();
        p_right_chain->previous_state();
      }
      if (!p_is_inline_template) {
        p_info->set_needs_conversion(true);
        p_info->add_type_conversion(p_type, this);
      }
      return true; }
    case T_SETOF: {
      if (!is_subtype_length_compatible(p_type)) {
        p_info->set_is_erroneous(this, p_type, string("Incompatible set of/SET OF "
                                 "subtypes"));
        return false;
      }
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      Type *p_of_type = p_type->get_ofType()->get_type_refd_last();
      TypeCompatInfo info_tmp(p_info->get_my_module(), of_type, p_of_type,
                              false, false);
      p_left_chain->mark_state();
      p_right_chain->mark_state();
      p_left_chain->add(of_type);
      p_right_chain->add(p_of_type);
      if (of_type == p_of_type
          || (p_left_chain->has_recursion() && p_right_chain->has_recursion())
          || of_type->is_compatible(p_of_type, &info_tmp, p_loc, p_left_chain,
                                    p_right_chain, p_is_inline_template)) {
        if (!p_is_inline_template) {
          p_info->set_needs_conversion(true);
          p_info->add_type_conversion(p_type, this);
        }
        p_left_chain->previous_state();
        p_right_chain->previous_state();
        return true;
      }
      p_info->append_ref_str(0, "[]" + info_tmp.get_ref_str(0));
      p_info->append_ref_str(1, "[]" + info_tmp.get_ref_str(1));
      p_info->set_is_erroneous(info_tmp.get_type(0), info_tmp.get_type(1),
                               info_tmp.get_error_str());
      p_left_chain->previous_state();
      p_right_chain->previous_state();
      return false; }
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_ANYTYPE:
      p_info->set_is_erroneous(this, p_type, string("union/CHOICE/anytype "
                               "types are compatible only with other "
                               "union/CHOICE/anytype types"));
      return false;
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SEQOF:
    case T_ARRAY:
      p_info->set_is_erroneous(this, p_type, string("set/SET and set of/SET OF "
                               "types are compatible only with other set/SET "
                               "set of/SET OF types"));
      return false;
    default:
      return false;
    }
  }

  bool Type::is_compatible_choice_anytype(Type *p_type,
                                          TypeCompatInfo *p_info,
                                          Location* p_loc,
                                          TypeChain *p_left_chain,
                                          TypeChain *p_right_chain,
                                          bool p_is_inline_template)
  {
    if (typetype != T_ANYTYPE && typetype != T_CHOICE_A
        && typetype != T_CHOICE_T)
      FATAL_ERROR("Type::is_compatible_choice_anytype()");
    if (this == p_type) return true;  // Original "true" leaf...
    else if (!use_runtime_2 || !p_info
             || (p_info && p_info->is_strict())) return false;  // ...and "false".
    if ((typetype == T_ANYTYPE && p_type->get_typetype() != T_ANYTYPE)
        || (p_type->get_typetype() == T_ANYTYPE && typetype != T_ANYTYPE)) {
      p_info->set_is_erroneous(this, p_type, string("Type anytype is compatible only "
                               "with other anytype types"));
      return false;
    }
    switch (p_type->get_typetype()) {
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SEQOF:
    case T_ARRAY:
    case T_SET_A:
    case T_SET_T:
    case T_SETOF:
      p_info->set_is_erroneous(this, p_type, string("union/CHOICE/anytype types are "
                               "compatible only with other union/CHOICE/anytype "
                               "types"));
      return false;
    case T_CHOICE_A:
    case T_CHOICE_T:
      if (typetype != T_CHOICE_A && typetype != T_CHOICE_T) {
        p_info->set_is_erroneous(this, p_type, string("union/CHOICE types are "
                                 "compatible only with other union/CHOICE "
                                 "types"));
        return false;
      }
      // no break
    case T_ANYTYPE: {
      if (p_left_chain->empty()) p_left_chain->add(this);
      if (p_right_chain->empty()) p_right_chain->add(p_type);
      // Find a field with the same name and with compatible type.  There can
      // be more alternatives, we need to generate all conversion functions.
      // The same field types must be avoided.  (For anytypes the "field
      // name = module name + field name".)
      bool alles_okay = false;
      for (size_t i = 0; i < get_nof_comps(); i++) {
        CompField *cf = get_comp_byIndex(i);
        Type *cf_type = cf->get_type()->get_type_refd_last();
        for (size_t j = 0; j < p_type->get_nof_comps(); ++j) {
          CompField *p_cf = p_type->get_comp_byIndex(j);
          Type *p_cf_type = p_cf->get_type()->get_type_refd_last();
          if (cf->get_name().get_name() != p_cf->get_name().get_name())
            continue;
          // Don't report errors for each incompatible field, it would be a
          // complete mess.  Use this temporary for all fields.  And forget
          // the contents.
          TypeCompatInfo info_tmp(p_info->get_my_module(), cf_type, p_cf_type,
                                  false, false);
          p_left_chain->mark_state();
          p_right_chain->mark_state();
          p_left_chain->add(cf_type);
          p_right_chain->add(p_cf_type);
          if (cf_type == p_cf_type
              || (p_left_chain->has_recursion() && p_right_chain->has_recursion())
              || cf_type->is_compatible(p_cf_type, &info_tmp, p_loc, p_left_chain,
                                        p_right_chain, p_is_inline_template)) {
            if (cf_type != p_cf_type && cf_type->is_structured_type()
                && p_cf_type->is_structured_type()) {
              if (typetype == T_ANYTYPE && cf_type->get_my_scope()
                  ->get_scope_mod() != p_cf_type->get_my_scope()
                  ->get_scope_mod()) {
                p_left_chain->previous_state();
                p_right_chain->previous_state();
                continue;
              }
              p_info->add_type_conversion(p_cf_type, cf_type);
            }
            alles_okay = true;
          }
          p_left_chain->previous_state();
          p_right_chain->previous_state();
        }
      }
      if (alles_okay && !p_is_inline_template) {
        p_info->set_needs_conversion(true);
        p_info->add_type_conversion(p_type, this);
        return true;
      }
      p_info->set_is_erroneous(this, p_type, string("No compatible "
                               "union/CHOICE/anytype field found"));
      return false;
    }
    default:
      return false;
    }
  }

  /** \todo consider subtype constraints */
  bool Type::is_identical(Type *p_type)
  {
    chk();
    p_type->chk();
    Type *t1 = get_type_refd_last();
    Type *t2 = p_type->get_type_refd_last();
    if (t2->typetype == T_ERROR) return true;
    switch (t1->typetype) {
    case T_ERROR:
      // error type is identical to everything
      return true;
    case T_ENUM_A:
    case T_ENUM_T:
    case T_CHOICE_T:
    case T_CHOICE_A:
    case T_ANYTYPE:
    case T_SEQOF:
    case T_SETOF:
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SET_A:
    case T_SET_T:
    case T_SIGNATURE:
    case T_PORT:
    case T_COMPONENT:
    case T_OPENTYPE:
    case T_FUNCTION:
    case T_ALTSTEP:
    case T_TESTCASE:
      // user-defined structured types must be identical
      return t1 == t2;
    case T_ARRAY:
      // the embedded type and the dimension must be identical in case of arrays
      return t2->typetype == T_ARRAY &&
        t1->u.array.element_type->is_identical(t2->u.array.element_type) &&
        t1->u.array.dimension->is_identical(t2->u.array.dimension);
    default:
      // in case of built-in types the TTCN-3 view of typetype must be the same
      return get_typetype_ttcn3(t1->typetype) ==
        get_typetype_ttcn3(t2->typetype);
    }
  }

  void Type::tr_compsof(ReferenceChain *refch)
  {
    if (typetype!=T_SEQ_A && typetype!=T_SET_A)
      FATAL_ERROR("Type::tr_compsof()");
    if (u.secho.tr_compsof_ready) return;
    if (u.secho.block) parse_block_Se();
    bool auto_tagging=u.secho.ctss->needs_auto_tags();
    if (refch) {
      refch->mark_state();
      if (refch->add(get_fullname())) u.secho.ctss->tr_compsof(refch, false);
      refch->prev_state();
      u.secho.tr_compsof_ready = true;
      u.secho.ctss->tr_compsof(0, true);
    } else {
      ReferenceChain refch2(this, "While resolving COMPONENTS OF");
      refch2.add(get_fullname());
      Error_Context cntxt(this, "While resolving COMPONENTS OF");
      u.secho.ctss->tr_compsof(&refch2, false);
      u.secho.tr_compsof_ready = true;
      u.secho.ctss->tr_compsof(0, true);
    }
    if(auto_tagging) u.secho.ctss->add_auto_tags();
  }

  bool Type::is_startable()
  {
    if(typetype != T_FUNCTION)
      FATAL_ERROR("Type::is_startable()");
    if(!checked) chk();
    return u.fatref.is_startable;
  }

  bool Type::is_list_type(bool allow_array)
  {
    switch (get_type_refd_last()->get_typetype_ttcn3()) {
    case Type::T_ARRAY:
      return allow_array;
    case Type::T_CSTR:
    case Type::T_USTR:
    case Type::T_BSTR:
    case Type::T_HSTR:
    case Type::T_OSTR:
    case Type::T_SEQOF:
    case Type::T_SETOF:
      return true;
    default:
      return false;
    }
  }
  
  void Type::set_coding_function(bool encode, Assignment* function_def)
  {
    if (function_def == NULL) {
      FATAL_ERROR("Type::set_coding_function");
    }
    coding_t& coding = encode ? default_encoding : default_decoding;
    if (coding.type == CODING_UNSET) {
      // no coding method has been set yet -> OK
      coding.type = CODING_BY_FUNCTION;
      coding.function_def = function_def;
    }
    else {
      // something has already been set -> not OK
      // (the type might never be encoded/decoded, so don't report an error yet)
      coding.type = CODING_MULTIPLE;
    }
  }
  
  void Type::set_asn_coding(bool encode, Type::MessageEncodingType_t new_coding)
  {
    coding_t& coding = encode ? default_encoding : default_decoding;
    if (coding.type == CODING_UNSET) {
      // no coding method has been set yet -> OK
      coding.type = CODING_BUILT_IN;
      coding.built_in_coding = new_coding;
    }
    else if (coding.type != CODING_BUILT_IN ||
             coding.built_in_coding != new_coding) {
      // a different codec or a coder function has already been set -> not OK
      // (the type might never be encoded/decoded, so don't report an error yet)
      coding.type = CODING_MULTIPLE;
    }
  }

  void Type::chk_coding(bool encode, Module* usage_mod, bool delayed /* = false */) {
    coding_t& coding = encode ? default_encoding : default_decoding;
    switch (coding.type) {
    case CODING_BY_FUNCTION:
      {
        Module* func_mod = coding.function_def->get_my_scope()->get_scope_mod();
        if (usage_mod != func_mod) {
          // add a phantom import for the coder function's module, to make sure
          // it is visible from the module that requested the coding
          Ttcn::Module* usage_mod_ttcn = dynamic_cast<Ttcn::Module*>(usage_mod);
          if (usage_mod_ttcn == NULL) {
            FATAL_ERROR("Type::chk_coding");
          }
          Ttcn::ImpMod* new_imp = new Ttcn::ImpMod(func_mod->get_modid().clone());
          new_imp->set_mod(func_mod);
          new_imp->set_imptype(Ttcn::ImpMod::I_DEPENDENCY);
          usage_mod_ttcn->get_imports().add_impmod(new_imp);
        }
      }
      // fall through
    case CODING_BUILT_IN:
      if (!delayed) {
        // a coding method has been set by an external function's checker,
        // but there might still be unchecked external functions out there;
        // delay this function until everything else has been checked
        Modules::delay_type_encode_check(this, usage_mod, encode);
      }
      return;
    case CODING_UNSET:
      if (!delayed) {
        // this is the only case in the switch that doesn't return immediately
        break;
      }
      // else fall through
    case CODING_MULTIPLE:
      error("Cannot determine the %s rules for type `%s'. "
        "%s %s external functions found%s", encode ? "encoding" : "decoding",
        get_typename().c_str(), coding.type == CODING_UNSET ? "No" : "Multiple",
        encode ? "encoding" : "decoding",
        (coding.type == CODING_MULTIPLE && is_asn1()) ? " with different rules" : "");
      return;
    default:
      FATAL_ERROR("Type::chk_coding");
    }

    if (!is_asn1()) { // TTCN-3 types
      if (!w_attrib_path) {
        error("No coding rule specified for type '%s'", get_typename().c_str());
        return;
      }

      // Checking extension attributes
      Ttcn::ExtensionAttributes * extatrs = parse_extattributes(w_attrib_path);
      if (extatrs != 0) { // NULL means parsing error
        for (size_t k = 0; k < extatrs->size(); ++k) {
          Ttcn::ExtensionAttribute &ea = extatrs->get(k);
          Ttcn::TypeMappings *inmaps = 0, *maps = 0;
          Ttcn::TypeMapping* mapping = 0;
          Ttcn::TypeMappingTarget* target = 0;
          Type* t = 0;
          switch (ea.get_type()) {
          case Ttcn::ExtensionAttribute::ENCDECVALUE:
            ea.get_encdecvalue_mappings(inmaps, maps);
            maps = encode ? maps : inmaps;
            maps->set_my_scope(this->get_my_scope());
            // The first param should be true, the other is not important if the first is true
            maps->chk(this, true, false);
            // look for coding settings
            t = encode ? this : Type::get_pooltype(T_BSTR);
            mapping = maps->get_mapping_byType(t);
            if (mapping->get_nof_targets() == 0)
              goto end_ext;
            else {
              for (size_t ind = 0; ind < mapping->get_nof_targets(); ind++) {
                target = mapping->get_target_byIndex(ind);
                t = target->get_target_type();
                if ((encode && (t->get_typetype() == T_BSTR)) ||
                  (!encode && (t->get_typename() == this->get_typename())))
                {
                  if (target->get_mapping_type() ==
                    Ttcn::TypeMappingTarget::TM_FUNCTION) {
                    if (coding.type != CODING_UNSET) {
                      target->error("Multiple definition of this target");
                    }
                    else {
                      coding.type = CODING_BY_FUNCTION;
                      coding.function_def = target->get_function();
                    }
                  } else {
                    target->error("Only function is supported to do this mapping");
                  }
                }
              }
              if (coding.type == CODING_UNSET) {
                ea.warning("Extension attribute is found for %s but without "
                "typemappings", encode ? "encvalue" : "decvalue");
              }
            }
            break;

          case Ttcn::ExtensionAttribute::ANYTYPELIST:
            break; // ignore (may be inherited from the module)

          case Ttcn::ExtensionAttribute::NONE:
            break; // ignore erroneous attribute

          default:
            ea.error("A type can only have type mapping extension attribute: "
              "in(...) or out(...)");
            break;
          }
        }
        delete extatrs;
      }

      if (coding.type != CODING_UNSET)
        return;
  end_ext:

      const vector<SingleWithAttrib>& real_attribs
                  = w_attrib_path->get_real_attrib();
      bool found = false;
      for (size_t i = real_attribs.size(); i > 0 && !found; i--) {
        if (real_attribs[i-1]->get_attribKeyword()
                                                == SingleWithAttrib::AT_ENCODE) {
          found = true;
          coding.type = CODING_BUILT_IN;
          coding.built_in_coding = get_enc_type(*real_attribs[i-1]);
        }
      }
      if (coding.type == CODING_UNSET) {
        // no "encode" attribute found
        error("No coding rule specified for type '%s'", get_typename().c_str());
        return;
      }
      if (coding.built_in_coding == CT_CUSTOM) {
        // the type has a custom encoding attribute, but no external coder
        // function with that encoding has been found yet;
        // delay this function until everything else has been checked
        Modules::delay_type_encode_check(this, usage_mod, encode);
        // set the coding type back to UNSET for delayed call
        coding.type = CODING_UNSET;
      }
      else if (!has_encoding(coding.built_in_coding)) {
        error("Type '%s' cannot be coded with the selected method '%s'",
          get_typename().c_str(), get_encoding_name(coding.built_in_coding));
      }
    }
    else { // ASN.1 type
      // delay this function until everything else has been checked, in case
      // there is an encoder/decoder external function out there for this type
      Modules::delay_type_encode_check(this, usage_mod, encode);
    }
  }

  bool Type::is_coding_by_function(bool encode) const {
    return (encode ? default_encoding : default_decoding).type == CODING_BY_FUNCTION;
  }
  
  string Type::get_coding(bool encode) const
  {
    const coding_t& coding = encode ? default_encoding : default_decoding;
    if (coding.type != CODING_BUILT_IN) {
      FATAL_ERROR("Type::get_built_in_coding");
    }
    switch (coding.built_in_coding) {
    case CT_RAW:
      return string("RAW");
    case CT_TEXT:
      return string("TEXT");
    case CT_XER:
      return string("XER, XER_EXTENDED"); // TODO: fine tuning this parameter
    case CT_JSON:
      if (encode) {
        return string("JSON, FALSE"); // with compact printing
      }
      else {
        return string("JSON");
      }
    case CT_BER: {
      string coding_str = string("BER, ");
      BerAST* ber = berattrib;
      if (!ber)  // use default settings if attributes are not specified
        ber = new BerAST;
      if (encode)
        coding_str += ber->get_encode_str();
      else
        coding_str += ber->get_decode_str();
      if (!berattrib)
        delete ber;
      return coding_str; }
    default:
      FATAL_ERROR("Type::get_built_in_coding");
    }
  }
  
  Assignment* Type::get_coding_function(bool encode) const
  {
    const coding_t& coding = encode ? default_encoding : default_decoding;
    if (coding.type != CODING_BY_FUNCTION) {
      FATAL_ERROR("Type::get_coding_function");
    }
    return coding.function_def;
  }

  namespace { // unnamed
  const string ex_emm_ell("XML"), ex_ee_arr("XER");
  }

  Type::MessageEncodingType_t Type::get_enc_type(const SingleWithAttrib& atr) {
    const string& enc = atr.get_attribSpec().get_spec();
    if (enc == "RAW")
      return CT_RAW;
    else if (enc == "TEXT")
      return CT_TEXT;
    else if (enc == "JSON")
      return CT_JSON;
    else if (enc == "BER:2002" || enc == "CER:2002" || enc == "DER:2002")
      return CT_BER;
    else if (enc == ex_emm_ell)
      return CT_XER;
    else if (enc == ex_ee_arr) {
      atr.warning("The correct name of the encoding is ''XML''");
      return CT_XER;
    }
    else if (enc == "PER")
      return CT_PER;
    else
      return CT_CUSTOM;
  }
  bool Type::has_ei_withName(const Identifier& p_id) const
  {
    switch (typetype) {
    case T_ENUM_T:
    case T_ENUM_A:
      if (checked) break;
      // no break
    default:
      FATAL_ERROR("Type::has_ei_withName()");
    }
    return u.enums.eis->has_ei_withName(p_id);
  }

  EnumItem *Type::get_ei_byName(const Identifier& p_id) const
  {
    switch (typetype) {
    case T_ENUM_T:
    case T_ENUM_A:
      if (checked) break;
      // no break
    default:
      FATAL_ERROR("Type::get_ei_byName()");
    }
    return u.enums.eis->get_ei_byName(p_id);
  }

  EnumItem *Type::get_ei_byIndex(size_t n) const
  {
    switch (typetype) {
    case T_ENUM_T:
    case T_ENUM_A:
      if (checked) break;
      // no break
    default:
      FATAL_ERROR("Type::get_ei_byIndex()");
    }
    return u.enums.eis->get_ei_byIndex(n);
  }

  size_t Type::get_nof_comps()
  {
    switch(typetype) {
    case T_ANYTYPE:
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_OPENTYPE:
      return u.secho.cfm->get_nof_comps();
    case T_SEQ_A:
    case T_SET_A:
      if(u.secho.block) parse_block_Se();
      return u.secho.ctss->get_nof_comps();
    case T_CHOICE_A:
      if(u.secho.block) parse_block_Choice();
      return u.secho.ctss->get_nof_comps();
    case T_ARRAY:
      return u.array.dimension->get_size();
    case T_SIGNATURE:
      if (u.signature.parameters)
        return u.signature.parameters->get_nof_params();
      else return 0;
    default:
      FATAL_ERROR("Type::get_nof_comps(%d)", typetype);
      return 0;
    } // switch
  }

  const Identifier& Type::get_comp_id_byIndex(size_t n)
  {
    switch (typetype) {
    case T_ANYTYPE:
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_OPENTYPE:
      return u.secho.cfm->get_comp_byIndex(n)->get_name();
    case T_SEQ_A:
    case T_SET_A:
      if(u.secho.block) parse_block_Se();
      return u.secho.ctss->get_comp_byIndex(n)->get_name();
    case T_CHOICE_A:
      if(u.secho.block) parse_block_Choice();
      return u.secho.ctss->get_comp_byIndex(n)->get_name();
    case T_SIGNATURE:
      return u.signature.parameters->get_param_byIndex(n)->get_id();
    default:
      FATAL_ERROR("Type::get_comp_id_byIndex()");
    } // switch
    // to avoid warnings
    const Identifier *fake = 0;
    return *fake;
  }

  CompField* Type::get_comp_byIndex(size_t n)
  {
    switch(typetype) {
    case T_ANYTYPE:
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_OPENTYPE:
      return u.secho.cfm->get_comp_byIndex(n);
    case T_SEQ_A:
    case T_SET_A:
      if(u.secho.block) parse_block_Se();
      return u.secho.ctss->get_comp_byIndex(n);
    case T_CHOICE_A:
      if(u.secho.block) parse_block_Choice();
      return u.secho.ctss->get_comp_byIndex(n);
    default:
      FATAL_ERROR("Type::get_comp_byIndex()");
      return 0;
    } // switch
  }

  size_t Type::get_comp_index_byName(const Identifier& p_name)
  {
    Type *t = get_type_refd_last();
    if (!t->is_secho())
      FATAL_ERROR("Type::get_comp_index_byName()");
    if (!t->u.secho.field_by_name) {
      t->u.secho.field_by_name = new map<string, size_t>;
      size_t nof_comps = t->get_nof_comps();
      for (size_t i = 0; i < nof_comps; i++) {
        const string& field_name =
          t->get_comp_byIndex(i)->get_name().get_name();
        if (!t->u.secho.field_by_name->has_key(field_name))
          t->u.secho.field_by_name->add(field_name, new size_t(i));
      }
    }
    return *(*t->u.secho.field_by_name)[p_name.get_name()];
  }

  size_t Type::get_eis_index_byName(const Identifier& p_name)
  {
    Type *t = get_type_refd_last();
    switch (t->typetype) {
    case T_ENUM_A:
    case T_ENUM_T:
      break;
    default:
      FATAL_ERROR("Type::get_eis_index_byName()");
    }
    if (!t->u.enums.eis_by_name) {
      t->u.enums.eis_by_name = new map<string, size_t>;
      size_t nof_eis = t->u.enums.eis->get_nof_eis();
      for (size_t i = 0; i < nof_eis; i++) {
        const string& enum_name =
          t->u.enums.eis->get_ei_byIndex(i)->get_name().get_name();
        if (!t->u.enums.eis_by_name->has_key(enum_name))
          t->u.enums.eis_by_name->add(enum_name, new size_t(i));
      }
    }
    return *(*t->u.enums.eis_by_name)[p_name.get_name()];
  }

  const Int& Type::get_enum_val_byId(const Identifier& p_name)
  {
    if(!checked) FATAL_ERROR("Type::get_enum_val_byId(): Not checked.");
    switch(typetype) {
    case T_ENUM_T:
    case T_ENUM_A:
      break;
    default:
      FATAL_ERROR("Type::get_enum_val_byId()");
    }
    return u.enums.eis->get_ei_byName(p_name)->get_int_val()->get_val();
  }

  size_t Type::get_nof_root_comps()
  {
    switch(typetype) {
    case T_SEQ_A:
    case T_SET_A:
      if(u.secho.block) parse_block_Se();
      return u.secho.ctss->get_nof_root_comps();
      break;
    default:
      FATAL_ERROR("Type::get_nof_root_comps()");
      return 0;
    } // switch
  }

  CompField* Type::get_root_comp_byIndex(size_t n)
  {
    switch(typetype) {
    case T_SEQ_A:
    case T_SET_A:
      if(u.secho.block) parse_block_Se();
      return u.secho.ctss->get_root_comp_byIndex(n);
      break;
    default:
      FATAL_ERROR("Type::get_root_comp_byIndex()");
      return 0;
    } // switch
  }

  bool Type::has_comp_withName(const Identifier& p_name)
  {
    Type *t = get_type_refd_last();
    switch (t->typetype) {
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_OPENTYPE:
    case T_ANYTYPE:
      return t->u.secho.cfm->has_comp_withName(p_name);
    case T_SEQ_A:
    case T_SET_A:
      if (t->u.secho.block) t->parse_block_Se();
      return t->u.secho.ctss->has_comp_withName(p_name);
    case T_CHOICE_A:
      if (t->u.secho.block) t->parse_block_Choice();
      return t->u.secho.ctss->has_comp_withName(p_name);
    case T_SIGNATURE:
      if (t->u.signature.parameters)
        return t->u.signature.parameters->has_param_withName(p_name);
      else return false;
    default:
      FATAL_ERROR("Type::has_comp_withName()");
      return 0;
    } // switch
  }

  CompField* Type::get_comp_byName(const Identifier& p_name)
  {
    Type *t = get_type_refd_last();
    switch (t->typetype) {
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_OPENTYPE:
    case T_ANYTYPE:
      return t->u.secho.cfm->get_comp_byName(p_name);
    case T_SEQ_A:
    case T_SET_A:
      if (t->u.secho.block) t->parse_block_Se();
      return t->u.secho.ctss->get_comp_byName(p_name);
    case T_CHOICE_A:
      if(t->u.secho.block) t->parse_block_Choice();
      return t->u.secho.ctss->get_comp_byName(p_name);
    default:
      FATAL_ERROR("Type::get_comp_byName()");
      return 0;
    } // switch
  }

  void Type::add_comp(CompField *p_cf)
  {
    switch(typetype) {
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_OPENTYPE:
    case T_ANYTYPE:
      u.secho.cfm->add_comp(p_cf);
      break;
    default:
      FATAL_ERROR("Type::add_comp()");
    } // switch
  }

  Type *Type::get_ofType()
  {
    Type *t=get_type_refd_last();
    switch (t->typetype) {
    case T_SEQOF:
    case T_SETOF:
      return t->u.seof.ofType;
    case T_ARRAY:
      return t->u.array.element_type;
    default:
      FATAL_ERROR("Type::get_ofType()");
      return 0;
    }
  }

  OC_defn* Type::get_my_oc()
  {
    switch(typetype) {
    case T_OCFT:
      return u.ref.oc_defn;
      break;
    case T_OPENTYPE:
      return u.secho.oc_defn;
      break;
    default:
      FATAL_ERROR("Type::get_my_oc()");
      return 0;
    } // switch
  }

  const Identifier& Type::get_oc_fieldname()
  {
    switch(typetype) {
    case T_OCFT:
      return *u.ref.oc_fieldname;
      break;
    case T_OPENTYPE:
      return *u.secho.oc_fieldname;
      break;
    default:
      FATAL_ERROR("Type::get_oc_fieldname()");
      // to avoid warning...
      return *u.secho.oc_fieldname;
    } // switch
  }

  void Type::set_my_tableconstraint(const TableConstraint *p_tc)
  {
    switch(typetype) {
    case T_OPENTYPE:
      u.secho.my_tableconstraint=p_tc;
      break;
    default:
      FATAL_ERROR("Type::set_my_tableconstraint()");
    } // switch
  }

  const TableConstraint* Type::get_my_tableconstraint()
  {
    switch(typetype) {
    case T_OPENTYPE:
      return u.secho.my_tableconstraint;
      break;
    default:
      FATAL_ERROR("Type::get_my_tableconstraint()");
      return 0;
    } // switch
  }

  Ttcn::ArrayDimension *Type::get_dimension() const
  {
    if (typetype != T_ARRAY) FATAL_ERROR("Type::get_dimension()");
    return u.array.dimension;
  }

  Ttcn::PortTypeBody *Type::get_PortBody() const
  {
    if (typetype != T_PORT) FATAL_ERROR("Type::get_PortBody()");
    return u.port;
  }

  ComponentTypeBody *Type::get_CompBody() const
  {
    if (typetype != T_COMPONENT) FATAL_ERROR("Type::get_CompBody()");
    return u.component;
  }

  SignatureParamList *Type::get_signature_parameters() const
  {
    if (typetype != T_SIGNATURE || !checked)
      FATAL_ERROR("Type::get_signature_parameters()");
    return u.signature.parameters;
  }

  SignatureExceptions *Type::get_signature_exceptions() const
  {
    if (typetype != T_SIGNATURE || !checked)
      FATAL_ERROR("Type::get_signature_exceptions()");
    return u.signature.exceptions;
  }

  Type *Type::get_signature_return_type() const
  {
    if (typetype != T_SIGNATURE || !checked)
      FATAL_ERROR("Type::get_signature_return_type()");
    return u.signature.return_type;
  }

  bool Type::is_nonblocking_signature() const
  {
    if (typetype != T_SIGNATURE || !checked)
      FATAL_ERROR("Type::is_nonblocking_signature()");
    return u.signature.no_block;
  }

  Ttcn::FormalParList *Type::get_fat_parameters()
  {
    if(!checked) FATAL_ERROR("Type::get_fat_parameteres()");
    switch(typetype) {
    case T_FUNCTION:
    case T_ALTSTEP:
    case T_TESTCASE:
      return u.fatref.fp_list;
    default:
      FATAL_ERROR("Type::get_fat_parameteres()");
      return 0;
    }
  }

  Type *Type::get_function_return_type()
  {
    if(!checked) FATAL_ERROR("Type::get_function_return_type()");
    switch(typetype) {
    case T_FUNCTION:
      return u.fatref.return_type;
    default:
      FATAL_ERROR("Type::get_function_return_type()");
      return 0;
    }
  }

  Type *Type::get_fat_runs_on_type()
  {
    if(!checked) FATAL_ERROR("Type::get_fat_runs_on_type()");
    switch(typetype) {
    case T_FUNCTION:
    case T_ALTSTEP:
    case T_TESTCASE:
      return u.fatref.runs_on.type;
    default:
      FATAL_ERROR("Type::get_fat_runs_on_type()");
      return 0;
    }
  }

  bool Type::get_fat_runs_on_self()
  {
    if(!checked) FATAL_ERROR("Type::get_fat_runs_on_self()");
    switch(typetype) {
    case T_FUNCTION:
    case T_ALTSTEP:
    case T_TESTCASE:
      return u.fatref.runs_on.self;
    default:
      FATAL_ERROR("Type::get_fat_runs_on_self()");
      return false;
    }
  }

  bool Type::get_returns_template()
  {
    if (!checked || typetype != T_FUNCTION)
      FATAL_ERROR("Type::Returns_template()");
    return u.fatref.returns_template;
  }

  void Type::add_tag(Tag *p_tag)
  {
    if(!tags) tags=new Tags();
    tags->add_tag(p_tag);
  }

  void Type::add_constraints(Constraints *p_constraints)
  {
    if(!p_constraints) return;
    if(constraints)
      FATAL_ERROR("This type already has its constraints");
    constraints=p_constraints;
    constraints->set_my_type(this);
  }

  Reference* Type::get_Reference()
  {
    return u.ref.ref;
  }

  void Type::chk_table_constraints()
  {
    if(!tbl_cons_checked) {
      tbl_cons_checked=true;
      if (constraints) constraints->chk_table();
      switch (typetype) {
      case T_CHOICE_A:
      case T_CHOICE_T:
      case T_SEQ_A:
      case T_SEQ_T:
      case T_SET_A:
      case T_SET_T:
      case T_OPENTYPE:
        for(size_t i=0; i<get_nof_comps(); i++)
          get_comp_byIndex(i)->get_type()->chk_table_constraints();
        break;
      case T_SEQOF:
      case T_SETOF:
        u.seof.ofType->chk_table_constraints();
        break;
      default:
        break;
      }
    }
  }

  void Type::check_subtype_constraints()
  {
    if (sub_type!=NULL) FATAL_ERROR("Type::check_subtype_constraints()");

    // get parent subtype or NULL if it doesn't exist
    SubType* parent_subtype = NULL;
    if (is_ref()) parent_subtype = get_type_refd()->sub_type;

    // if the parent subtype is erroneous then ignore it, the error was already
    // reported there
    if ( (parent_subtype!=NULL) &&
         (parent_subtype->get_subtypetype()==SubtypeConstraint::ST_ERROR) )
      parent_subtype = NULL;

    // return if there are neither inherited nor own constraints
    if ( (parent_subtype==NULL) && (parsed_restr==NULL) && (constraints==NULL)
         && (SubtypeConstraint::get_asn_type_constraint(this)==NULL) ) return;

    // the subtype type is determined by the type of this type
    if (get_type_refd_last()->get_typetype()==T_ERROR) return;
    SubtypeConstraint::subtype_t s_t = get_subtype_type();
    if (s_t==SubtypeConstraint::ST_ERROR) {
      error("Subtype constraints are not applicable to type `%s'",
            get_typename().c_str());
      return;
    }

    // create the aggregate subtype for this type
    sub_type = new SubType(s_t, this, parent_subtype, parsed_restr, constraints);
    sub_type->chk();
  }

  bool Type::has_multiple_tags()
  {
    if (tags) return false;
    switch (typetype) {
    case T_CHOICE_A:
    case T_CHOICE_T:
      return true;
    case T_REFD:
    case T_SELTYPE:
    case T_REFDSPEC:
    case T_OCFT:
      return get_type_refd()->has_multiple_tags();
    default:
      return false;
    }
  }

  Tag *Type::get_tag()
  {
    if (tags) {
      Tag *tag=tags->get_tag_byIndex(tags->get_nof_tags()-1);
      tag->chk();
      return tag;
    }
    else return get_default_tag();
  }

  void Type::get_tags(TagCollection& coll, map<Type*, void>& chain)
  {
    if(typetype!=T_CHOICE_A)
      FATAL_ERROR("Type::get_tags()");
    if (chain.has_key(this)) return;
    chain.add(this, 0);
    if(u.secho.block) parse_block_Choice();
    size_t n_alts = u.secho.ctss->get_nof_comps();
    for(size_t i=0; i<n_alts; i++) {
      CompField *cf = u.secho.ctss->get_comp_byIndex(i);
      Type *type = cf->get_type();
      if(type->has_multiple_tags()) {
        type=type->get_type_refd_last();
        type->get_tags(coll, chain);
      }
      else {
        const Tag *tag=type->get_tag();
        if(coll.hasTag(tag))
          error("Alternative `%s' in CHOICE has non-distinct tag",
                cf->get_name().get_dispname().c_str());
        else coll.addTag(tag);
      }
    } // for i
    if(u.secho.ctss->has_ellipsis()) coll.setExtensible();
  }

  Tag *Type::get_smallest_tag()
  {
    if(!has_multiple_tags()) return get_tag()->clone();
    Type *t=get_type_refd_last();
    TagCollection tagcoll;
    map<Type*, void> chain;
    t->get_tags(tagcoll, chain);
    chain.clear();
    return tagcoll.getSmallestTag()->clone();
  }

  bool Type::needs_explicit_tag()
  {
    switch (typetype) {
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_ANY:
    case T_OPENTYPE:
      return true;
    case T_REFD: {
      if(!dynamic_cast<Asn::Ref_pard*>(u.ref.ref)) {
        Scope *s=u.ref.ref->get_refd_assignment()->get_my_scope();
        if(s->get_parent_scope()!=s->get_scope_mod()) {
          // Not in the module scope, so it is a dummyreference (X.680
          // 30.6c)
          /*
          WARNING("%s is a dummyreference, i give him an explicit tag :)",
                  get_fullname().c_str());
          WARNING("0: %s", s->get_scope_name().c_str());
          WARNING("1: %s", s->get_parent_scope()->get_scope_name().c_str());
          WARNING("2: %s", s->get_scope_mod()->get_scope_name().c_str());
          */
          return true;
        }
      }
      // no break;
    } // case T_REFD
    case T_SELTYPE:
    case T_REFDSPEC:
    case T_OCFT: {
      Type *t = get_type_refd();
      if(t->is_tagged()) return false;
      else return t->needs_explicit_tag();
      break;}
    default:
      // T_ANYTYPE probably does not need explicit tagging
      return false;
    }
  }

  void Type::cut_auto_tags()
  {
    if (tags) {
      tags->cut_auto_tags();
      if (tags->get_nof_tags() == 0) {
        delete tags;
        tags = 0;
      }
    }
  }

  /**
   * I suppose in this function that tags->chk() and
   * tags->set_plicit() are already done.
   */
  Tags* Type::build_tags_joined(Tags *p_tags)
  {
    if(!p_tags) p_tags=new Tags();
    switch(typetype) {
      // is_ref() true:
    case T_REFD:
    case T_SELTYPE:
    case T_REFDSPEC:
    case T_OCFT:
      get_type_refd()->build_tags_joined(p_tags);
      break;
    case T_CHOICE_A:
    case T_CHOICE_T:
      //TODO case T_ANYTYPE: for build_tags_joined ?
    case T_OPENTYPE:
    case T_ANY:
      break;
    default:
      p_tags->add_tag(get_default_tag()->clone());
      break;
    } // switch
    if(tags) {
      for(size_t i=0; i<tags->get_nof_tags(); i++) {
        Tag *tag=tags->get_tag_byIndex(i);
        switch(tag->get_plicit()) {
        case Tag::TAG_EXPLICIT:
          p_tags->add_tag(tag->clone());
          break;
        case Tag::TAG_IMPLICIT: {
          Tag *t_p_tag=p_tags->get_tag_byIndex(p_tags->get_nof_tags()-1);
          t_p_tag->set_tagclass(tag->get_tagclass());
          t_p_tag->set_tagvalue(tag->get_tagvalue());
          break;}
        default:
          FATAL_ERROR("Type::build_tags_joined()");
        } // switch
      } // for
    } // if tags
    return p_tags;
  }

  void Type::set_with_attr(Ttcn::MultiWithAttrib* p_attrib)
  {
    if(!w_attrib_path)
    {
      w_attrib_path = new WithAttribPath();
    }

    w_attrib_path->set_with_attr(p_attrib);
  }

  void Type::set_parent_path(WithAttribPath* p_path)
  {
    if(!w_attrib_path)
    {
      w_attrib_path = new WithAttribPath();
    }
    w_attrib_path->set_parent(p_path);
    if (typetype == T_COMPONENT)
      u.component->set_parent_path(w_attrib_path);
  }

  WithAttribPath* Type::get_attrib_path() const
  {
    return w_attrib_path;
  }

  bool Type::hasRawAttrs()
  {
    if(rawattrib) return true;

    if(w_attrib_path)
    {
      if(w_attrib_path->get_had_global_variants()) return true;

      vector<SingleWithAttrib> const &real_attribs
        = w_attrib_path->get_real_attrib();

      for (size_t i = 0; i < real_attribs.size(); i++)
        if (real_attribs[i]->get_attribKeyword()
          == SingleWithAttrib::AT_VARIANT)
        {
          return true;
        }

      MultiWithAttrib* temp_attrib = w_attrib_path->get_with_attr();
      if(temp_attrib)
        for(size_t i = 0; i < temp_attrib->get_nof_elements(); i++)
          if(temp_attrib->get_element(i)->get_attribKeyword()
            == SingleWithAttrib::AT_VARIANT
            && (!temp_attrib->get_element(i)->get_attribQualifiers()
                || temp_attrib->get_element(i)->get_attribQualifiers()
                    ->get_nof_qualifiers() == 0))
          {
            w_attrib_path->set_had_global_variants( true );
            return true;
          }
    }

    return false;
  }

  bool Type::hasNeedofRawAttrs()
  {
    if(rawattrib) return true;
    size_t nof_comps;
    switch(typetype){
    case T_CHOICE_T:
    case T_ANYTYPE:
    case T_SEQ_T:
    case T_SET_T:
      nof_comps = get_nof_comps();
      for(size_t i=0; i < nof_comps; i++)
      {
        if(get_comp_byIndex(i)->get_type()->hasNeedofRawAttrs())
        {
          return true;
        }
      }
      break;
    default:
      break;
    }
    return false;
  }

  bool Type::hasNeedofTextAttrs()
  {
    if(textattrib) return true;
    size_t nof_comps;
    switch(typetype){
    case T_CHOICE_T:
    case T_ANYTYPE:
    case T_SEQ_T:
    case T_SET_T:
      nof_comps = get_nof_comps();
      for(size_t i=0; i < nof_comps; i++)
      {
        if(get_comp_byIndex(i)->get_type()->hasNeedofTextAttrs())
        {
          return true;
        }
      }
      break;
    default:
      break;
    }
    return false;
  }
  
  bool Type::hasNeedofJsonAttrs()
  {
    if(jsonattrib) return true;
    size_t nof_comps;
    switch(typetype) {
    case T_CHOICE_T:
    case T_ANYTYPE:
    case T_SEQ_T:
    case T_SET_T:
      nof_comps = get_nof_comps();
      for (size_t i = 0; i < nof_comps; ++i)
      {
        if (get_comp_byIndex(i)->get_type()->hasNeedofJsonAttrs())
        {
          return true;
        }
      }
      break;
    default:
      break;
    }
    return false;
  }

  bool Type::hasNeedofXerAttrs()
  {
    if(xerattrib && !xerattrib->empty()) return true;
    size_t nof_comps;
    switch(typetype){
    case T_CHOICE_T:
    case T_ANYTYPE:
    case T_SEQ_T:
    case T_SET_T:
      nof_comps = get_nof_comps();
      for(size_t i=0; i < nof_comps; i++)
      {
        if(get_comp_byIndex(i)->get_type()->hasNeedofXerAttrs())
        {
          return true;
        }
      }
      break;
    default:
      break;
    }
    return false;
  }

  bool Type::hasVariantAttrs()
  {
    if(w_attrib_path)
    {
      if(w_attrib_path->get_had_global_variants()) return true;

      vector<SingleWithAttrib> const &real_attribs
        = w_attrib_path->get_real_attrib();

      for (size_t i = 0; i < real_attribs.size(); i++)
        if (real_attribs[i]->get_attribKeyword()
          == SingleWithAttrib::AT_VARIANT)
        {
          return true;
        }

      MultiWithAttrib* temp_attrib = w_attrib_path->get_with_attr();
      if(temp_attrib)
        for(size_t i = 0; i < temp_attrib->get_nof_elements(); i++)
          if(temp_attrib->get_element(i)->get_attribKeyword()
            == SingleWithAttrib::AT_VARIANT
            && (!temp_attrib->get_element(i)->get_attribQualifiers()
                || temp_attrib->get_element(i)->get_attribQualifiers()
                    ->get_nof_qualifiers() != 0))
          {
            w_attrib_path->set_had_global_variants( true );
            return true;
          }
    }

    return false;
  }
  
  bool Type::hasEncodeAttr(const char* encoding_name)
  {
    if (0 == strcmp(encoding_name, "JSON") && (implicit_json_encoding
        || is_asn1() || (is_ref() && get_type_refd()->is_asn1()))) {
      // ASN.1 types automatically support JSON encoding
      return true;
    }
    // Check the type itself first, then the root type
    WithAttribPath *aps[2] = { 0, 0 };
    size_t num_aps = ((aps[0] = get_attrib_path()) != 0);
    // assign, compare, then add 0 or 1
    if (is_ref()) {
      num_aps += ((aps[num_aps] = get_type_refd()->get_attrib_path()) != 0);
    }
    for (size_t a = 0; a < num_aps; ++a) {
      const vector<SingleWithAttrib>& real = aps[a]->get_real_attrib();
      const size_t num_atr = real.size();
      for (size_t i = 0; i < num_atr; ++i) {
        const SingleWithAttrib& s = *real[i];
        if (s.get_attribKeyword() == SingleWithAttrib::AT_ENCODE) {
          const string& spec = s.get_attribSpec().get_spec();
          if (spec == encoding_name) {
            return true;
          }
        } // if ENCODE
      } // for
    } // next a
    return false;
  }

  namespace { // unnamed

  enum state { PROCESSING = -1, ANSWER_NO, ANSWER_YES };

  struct memoizer : private map<Type*, state> {
    memoizer() : map<Type*, state>() {}

    ~memoizer() {
      for (int i = size()-1; i >= 0; --i) {
        delete get_nth_elem(i);
      }
      clear();
    }

    bool remember (Type *t, state s) {
      if (has_key(t)) {
        *operator[](t) = s;
      }
      else {
        add(t, new state(s));
      }
      return s == ANSWER_YES;
    }

    bool has_key(Type *t) {
      return map<Type*, state>::has_key(t);
    }

    state* get(Type *t) {
      return operator [](t);
    }
  };

  }

  bool Type::has_encoding(MessageEncodingType_t encoding_type, const string* custom_encoding /* = NULL */)
  {
    static memoizer memory;
    static memoizer json_mem;
    Type *t = this;
    switch (encoding_type) {
    case CT_BER:
    case CT_PER:
      for ( ; ; ) {
        if (t->is_asn1()) return true;
        else if (t->is_ref()) t = t->get_type_refd();
        else {
          switch (t->typetype) {
          case T_ERROR:
          case T_BOOL:
          case T_INT:
          case T_REAL:
          case T_BSTR:
          case T_OSTR:
          case T_OID:
            // these basic TTCN-3 types have ASN.1 equivalents
            return true;
          default:
            return false;
          }
        }
      }

    case CT_XER: {
      if (memory.has_key(this)) {
        state  *s = memory.get(this);
        switch (*s){
        case PROCESSING:
          break;
        case ANSWER_NO:
          return false;
        case ANSWER_YES:
          return true;
        }
      }

      for (;;) {
        // For ASN.1 types, the answer depends solely on the -a switch.
        // They are all considered to have Basic (i.e. useless) XER,
        // unless the -a switch says removes XER from all ASN.1 types.
        if (t->is_asn1()) return memory.remember(t,
          asn1_xer ? ANSWER_YES : ANSWER_NO);
        else if (t->is_ref()) t = t->get_type_refd();
        else { // at the end of the ref. chain
          switch (t->typetype) {
          case T_BOOL:
          case T_INT:
          case T_REAL:
          case T_BSTR:
          case T_OSTR:
            // The octetstring type can always be encoded in XER.
            // XSD:base64Binary is only needed for Type::is_charenc()
          case T_OID:
          case T_HSTR:    // TTCN-3 hexstring
          case T_VERDICT: // TTCN-3 verdict
          case T_CSTR:    // TTCN3 charstring
          case T_USTR:    // TTCN3 universal charstring
            return memory.remember(t, ANSWER_YES);

          case T_ENUM_T:
            break; // the switch; skip to checking if it has encode "XML";

          case T_PORT:      // TTCN-3 port (the list of in's, out's, inout's)
          case T_COMPONENT: // TTCN-3 comp. type (extends, and { ... })
          case T_DEFAULT:   // TTCN-3
          case T_SIGNATURE: // TTCN-3
          case T_FUNCTION:  // TTCN-3
          case T_ALTSTEP:   // TTCN-3
          case T_TESTCASE:  // TTCN-3
            return memory.remember(t, ANSWER_NO);

          case T_UNDEF:
          case T_ERROR:
          case T_REFDSPEC:
            return false; // why don't we remember these ?

          case T_SEQ_T:
          case T_SET_T:
          case T_CHOICE_T:
          case T_ANYTYPE:   // TTCN-3 anytype
          {
            // No field may reject XER
            size_t ncomp = t->get_nof_comps();
            for (size_t i = 0; i < ncomp; ++i) {
              Type *t2 = t->get_comp_byIndex(i)->get_type();
              bool subresult = false;
              if (memory.has_key(t2)) {
                switch (*memory.get(t2)) {
                case PROCESSING:
                  // This type contains itself and is in the process
                  // of being checked. Pretend it doesn't exist.
                  // The answer will be determined by the other fields,
                  // and it will propagate back up.
                  // Avoids infinite recursion for self-referencing types.
                  continue;
                case ANSWER_NO:
                  subresult = false;
                  break;
                case ANSWER_YES:
                  subresult = true;
                  break;
                }
              }
              else {
                memory.remember(t2, PROCESSING);
                subresult = t2->has_encoding(CT_XER);
              }

              if (subresult) memory.remember(t2, ANSWER_YES);
              else {
                // Remember the t type as NO also, because at least one of its
                // field rejected xer encoding.
                memory.remember(t, ANSWER_NO);
                return memory.remember(t2, ANSWER_NO);
              }
              // Note: return only if the answer (false) is known.
              // If the answer is true, keep checking.
            } // next i
            // Empty record, or all fields supported XER: answer maybe yes.
            break; }

          case T_SEQOF:
          case T_SETOF: {
            bool subresult = false;
            Type *t2 = t->u.seof.ofType;
            if (memory.has_key(t2)) {
              switch (*memory.get(t2)) {
              case PROCESSING:
                // Recursive record-of. This is OK because the recursion
                // can always be broken with an empty record-of.
                subresult = true;
                break;
              case ANSWER_NO:
                subresult = false;
                break;
              case ANSWER_YES:
                subresult = true;
                break;
              }
            }
            else {
              memory.remember(t2, PROCESSING);
              // Check the contained type
              subresult = t2->has_encoding(CT_XER);
            }
            if (subresult) break; // continue checking
            else return memory.remember(t, ANSWER_NO); // No means no.
          }

          case T_NULL: // ASN.1 null
          case T_INT_A: // ASN.1 integer
          case T_ENUM_A:// ASN.1 enum
          case T_BSTR_A:// ASN.1 bitstring
          case T_UTF8STRING:         // ASN.1
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
          case T_UNRESTRICTEDSTRING: // still ASN.1
          case T_UTCTIME:            // ASN.1 string
          case T_GENERALIZEDTIME:    // ASN.1 string
          case T_OBJECTDESCRIPTOR:   // ASN.1 string
          case T_ROID:               // relative OID (ASN.1)

          case T_CHOICE_A:           //
          case T_SEQ_A:              // ASN.1 versions of choice,sequence,set
          case T_SET_A:              //

          case T_OCFT:     // ObjectClassFieldType (ASN.1)
          case T_OPENTYPE: // ASN.1 open type
          case T_ANY:      // deprecated ASN.1 ANY

          case T_EXTERNAL:     // ASN.1 external
          case T_EMBEDDED_PDV: // ASN.1 embedded pdv
          case T_SELTYPE:      // selection type (ASN.1)
            FATAL_ERROR("Type::has_encoding(): typetype %d should be asn1",
              t->typetype);
            break; // not reached

          case T_REFD:         // reference to another type
          case T_ADDRESS:      // TTCN-3 address type
          case T_ARRAY:        // TTCN-3 array
          default: // FIXME: if compiling with -Wswitch, the default should be removed
            return memory.remember(t, ANSWER_NO);
          } // switch t->typetype

          // Check to see if it has an encode "XML"; first the type itself,
          // then the root type.
          WithAttribPath *aps[2] = {0,0};
          size_t num_aps = ((aps[0] = this->get_attrib_path()) != 0);
          // assign, compare, then add 0 or 1
          if (this != t) {
            num_aps += ((aps[num_aps] = t->get_attrib_path()) != 0);
          }
          for (size_t a = 0; a < num_aps; ++a) {
            const vector<SingleWithAttrib>& real = aps[a]->get_real_attrib();
            const size_t num_atr = real.size();
            for (size_t i = 0; i < num_atr; ++i) {
              const SingleWithAttrib& s = *real[i];
              if (s.get_attribKeyword() == SingleWithAttrib::AT_ENCODE) {
                const string& spec = s.get_attribSpec().get_spec();
                if (spec == ex_emm_ell // the right answer
                  ||spec == ex_ee_arr) // the acceptable answer
                  return memory.remember(t, ANSWER_YES);
              } // if ENCODE
            } // for
          } // next a
          return memory.remember(t, ANSWER_NO); // no encode XER
        } // if(!asn1)
      } // for(ever)
      return memory.remember(t, ANSWER_NO); }

    case CT_RAW:
      for ( ; ; ) {
        if (t->rawattrib) return true;
        else if (t->is_ref()) t = t->get_type_refd();
        else {
          switch (t->typetype) {
          case T_ERROR:
          case T_BOOL:
          case T_INT:
          case T_REAL:
          case T_BSTR:
          case T_HSTR:
          case T_OSTR:
          case T_CSTR:
          case T_USTR:
            // these basic types support RAW encoding by default
            return true;
          default:
            return false;
          }
        }
      }

    case CT_TEXT:
      for ( ; ; ) {
        if (t->textattrib) return true;
        else if (t->is_ref()) t = t->get_type_refd();
        else {
          switch (t->typetype) {
          case T_ERROR:
          case T_BOOL:
          case T_INT:
          case T_OSTR:
          case T_CSTR:
          case T_USTR:    // TTCN3 universal charstring
            // these basic types support TEXT encoding by default
            return true;
          default:
            return false;
          }
        }
      }
      
    case CT_JSON:
      while (true) {
        if (json_mem.has_key(t)) {
          switch (*json_mem.get(t)) {
          case PROCESSING:
            break;
          case ANSWER_NO:
            return false;
          case ANSWER_YES:
            return true;
          }
        }
        if (t->jsonattrib) {
          return json_mem.remember(t, ANSWER_YES);
        }
        if (t->is_ref()) {
          t = t->get_type_refd();
        }
        else {
          switch (t->typetype) {
          case T_ERROR:
          case T_BOOL:
          case T_INT:
          case T_INT_A:
          case T_REAL:
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
          case T_VERDICT:
          case T_NULL:
          case T_OID:
          case T_ROID:
          case T_ANY:
            // these basic types support JSON encoding by default
            return json_mem.remember(t, ANSWER_YES);
          case T_SEQ_T:
          case T_SEQ_A:
          case T_OPENTYPE:
          case T_SET_T:
          case T_SET_A:
          case T_CHOICE_T:
          case T_CHOICE_A:
          case T_ANYTYPE: {
            // all fields must also support JSON encoding
            size_t ncomp = t->get_nof_comps();
            for (size_t i = 0; i < ncomp; ++i) {
              Type *t2 = t->get_comp_byIndex(i)->get_type();
              if (json_mem.has_key(t2)) {
                switch (*json_mem.get(t2)) {
                case ANSWER_YES:
                  // This field is OK, but we still need to check the others
                case PROCESSING:
                  // This type contains itself and is in the process
                  // of being checked. Pretend it doesn't exist.
                  // The answer will be determined by the other fields,
                  // and it will propagate back up.
                  // Avoids infinite recursion for self-referencing types. 
                  continue;
                case ANSWER_NO:
                  // One field is not OK => the structure is not OK
                  return json_mem.remember(t, ANSWER_NO);
                }
              }
              else {
                json_mem.remember(t2, PROCESSING);
                bool enabled = t2->has_encoding(CT_JSON);
                json_mem.remember(t2, enabled ? ANSWER_YES : ANSWER_NO);
                if (!enabled) {
                  // One field is not OK => the structure is not OK
                  return json_mem.remember(t, ANSWER_NO);
                }
              }
            }
            break; // check for an encode attribute
          }
          case T_SEQOF:
          case T_SETOF:
          case T_ARRAY: {
            Type *t2 = t->u.seof.ofType;
            if (json_mem.has_key(t2)) {
              switch (*json_mem.get(t2)) {
              case ANSWER_YES:
                // Continue checking
              case PROCESSING:
                // Recursive record-of. This is OK because the recursion
                // can always be broken with an empty record-of.
                break;
              case ANSWER_NO:
                return json_mem.remember(t, ANSWER_NO);
                break;
              }
            }
            else {
              json_mem.remember(t2, PROCESSING);
              bool enabled = t2->has_encoding(CT_JSON);
              json_mem.remember(t2, enabled ? ANSWER_YES : ANSWER_NO);
              if (!enabled) {
                // One field is not OK => the structure is not OK
                return json_mem.remember(t, ANSWER_NO);
              }
            }
            break; // check for an encode attribute
          }
          case T_ENUM_T:
          case T_ENUM_A:
            break; // check for an encode attribute
          default:
            return json_mem.remember(t, ANSWER_NO);
          } // switch
          return json_mem.remember(t, hasEncodeAttr(get_encoding_name(CT_JSON)) ? ANSWER_YES : ANSWER_NO);
        } // else
      } // while
      
    case CT_CUSTOM:
      // the encoding name parameter has to be the same as the encoding name
      // specified for the type
      return custom_encoding ? hasEncodeAttr(custom_encoding->c_str()) : false;

    default:
      FATAL_ERROR("Type::has_encoding()");
      return false;
    }
  }


  bool Type::is_pure_refd()
  {
    switch (typetype) {
    case T_REFD:
      // ASN.1 parameterized references are not pure :)
      if(dynamic_cast<Asn::Ref_pard*>(u.ref.ref)) return false;
      // no break;
    case T_REFDSPEC:
    case T_SELTYPE:
    case T_OCFT:
      if (sub_type || constraints) return false;
      else if (tags && enable_ber()) return false;
      else if (rawattrib && enable_raw()) return false;
      else if (textattrib && enable_text()) return false;
      else if (enable_xer()) return false;
      else if (jsonattrib && enable_json()) return false;
      else return true;
    default:
      return false;
    }
  }

  string Type::create_stringRepr()
  {
    if(is_tagged() || hasRawAttrs())
      return get_genname_own();
    switch(typetype) {
    case T_NULL:
      return string("NULL");
    case T_BOOL:
      return string("BOOLEAN");
    case T_INT:
    case T_INT_A:
      return string("INTEGER");
    case T_REAL:
      return string("REAL");
    case T_BSTR:
    case T_BSTR_A:
      return string("BIT__STRING");
    case T_HSTR:
      return string("HEX__STRING");
    case T_OSTR:
      return string("OCTET__STRING");
    case T_CSTR:
      return string("CHAR__STRING");
    case T_USTR:
      return string("UNIVERSAL__CHARSTRING");
    case T_UTF8STRING:
      return string("UTF8String");
    case T_NUMERICSTRING:
      return string("NumericString");
    case T_PRINTABLESTRING:
      return string("PrintableString");
    case T_TELETEXSTRING:
      return string("TeletexString");
    case T_VIDEOTEXSTRING:
      return string("VideotexString");
    case T_IA5STRING:
      return string("IA5String");
    case T_GRAPHICSTRING:
      return string("GraphicString");
    case T_VISIBLESTRING:
      return string("VisibleString");
    case T_GENERALSTRING:
      return string("GeneralString");
    case T_UNIVERSALSTRING:
      return string("UniversalString");
    case T_BMPSTRING:
      return string("BMPString");
    case T_UNRESTRICTEDSTRING:
      return string("CHARACTER__STRING");
    case T_UTCTIME:
      return string("UTCTime");
    case T_GENERALIZEDTIME:
      return string("GeneralizedTime");
    case T_OBJECTDESCRIPTOR:
      return string("ObjectDescriptor");
    case T_OID:
      return string("OBJECT__IDENTIFIER");
    case T_ROID:
      return string("RELATIVE__OID");
    case T_ANY:
      return string("ANY");
    case T_REFD:
    case T_SELTYPE:
    case T_REFDSPEC:
    case T_OCFT:
      if (tags || constraints ||
          (w_attrib_path && w_attrib_path->has_attribs()))
        return get_genname_own();
      else return get_type_refd()->get_stringRepr();
    case T_ERROR:
      return string("<Error_type>");
    default:
      return get_genname_own();
    } // switch
  }

  Identifier Type::get_otaltname(bool& is_strange)
  {
    string s;
    if (is_tagged() || is_constrained() || hasRawAttrs()) {
      s = get_genname_own();
      is_strange = true;
    } else if (typetype == T_REFD) {
      Ref_simple* t_ref=dynamic_cast<Ref_simple*>(u.ref.ref);
      if (t_ref) {
        const Identifier *id = t_ref->get_id();
        const string& dispname = id->get_dispname();
        if (dispname.find('.') < dispname.size()) {
          // id is not regular because t_ref is a parameterized reference
          // use that id anyway
          s += id->get_name();
          is_strange = true;
        } else {
          Scope *ass_scope = t_ref->get_refd_assignment()->get_my_scope();
          if (ass_scope->get_parent_scope() == ass_scope->get_scope_mod()) {
            // t_ref points to an assignment at module scope
            // use the simple id of the reference (in lowercase)
            s = id->get_name();
            is_strange = false;
          } else {
            // t_ref is a dummy reference in a parameterized assignment
            // (i.e. it points to a parameter assignment of an instantiation)
            // perform the same examination recursively on the referenced type
            // (which is the actual parameter)
            return get_type_refd()->get_otaltname(is_strange);
          }
        }
      } else {
        // the type comes from an information object [class]
        // examine the referenced type recursively
        return get_type_refd()->get_otaltname(is_strange);
      }
    } else {
      s = get_stringRepr();
      // throw away the leading @ if this is an instantiated type
      // (e.g. an in-line SEQUENCE from a parameterized reference)
      if (!strncmp(s.c_str(), "_root_", 6)) s.replace(0, 6, "");
      // the name is strange if it contains a single underscore
      string s2(s);
      // transform "__" -> "-"
      for (size_t pos = 0; ; ) {
        pos = s2.find("__", pos);
        if (pos < s2.size()) {
          s2.replace(pos, 2, "-");
          pos++;
        } else break;
      }
      is_strange = s2.find('_') < s2.size();
    }
    /*
    size_t pos=s.find_if(0, s.size(), isupper);
    if(pos==s.size()) FATAL_ERROR("Type::get_otaltname() (`%s')", s.c_str());
    s[pos]=(char)tolower(s[pos]);
    */
    s[0]=(char)tolower(s[0]);
    Identifier tmp_id(Identifier::ID_NAME, s, true);
    /* This is because the origin of the returned ID must be ASN. */
    return Identifier(Identifier::ID_ASN, tmp_id.get_asnname());
  }

  string Type::get_genname_value(Scope *p_scope)
  {
    Type *t = get_type_refd_last();
    switch (t->typetype) {
    case T_UNDEF:
    case T_ERROR:
    case T_UNRESTRICTEDSTRING:
    case T_OCFT:
    case T_EXTERNAL:
    case T_EMBEDDED_PDV:
    case T_REFD:
    case T_REFDSPEC:
    case T_SELTYPE:
      FATAL_ERROR("Type::get_genname_value()");
    case T_NULL:
      return string("ASN_NULL");
    case T_BOOL:
      return string("BOOLEAN");
    case T_INT:
    case T_INT_A:
      return string("INTEGER");
    case T_REAL:
      return string("FLOAT");
    case T_BSTR:
    case T_BSTR_A:
      return string("BITSTRING");
    case T_HSTR:
      return string("HEXSTRING");
    case T_OSTR:
      return string("OCTETSTRING");
    case T_CSTR:
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_IA5STRING:
    case T_VISIBLESTRING:
    case T_UTCTIME:
    case T_GENERALIZEDTIME:
      return string("CHARSTRING");
    case T_USTR: // ttcn3 universal charstring
    case T_UTF8STRING:
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
    case T_GRAPHICSTRING:
    case T_GENERALSTRING:
    case T_UNIVERSALSTRING:
    case T_BMPSTRING:
    case T_OBJECTDESCRIPTOR:
      return string("UNIVERSAL_CHARSTRING");
    case T_OID:
    case T_ROID:
      return string("OBJID");
    case T_ANY:
      return string("ASN_ANY");
    case T_VERDICT:
      return string("VERDICTTYPE");
    case T_COMPONENT:
      return string("COMPONENT");
    case T_DEFAULT:
      return string("DEFAULT");
    case T_ARRAY:
      if (!t->u.array.in_typedef)
        return t->u.array.dimension->get_value_type(t->u.array.element_type,
          p_scope);
      // no break
    default:
      return t->get_genname_own(p_scope);
    } // switch
  }

  string Type::get_genname_template(Scope *p_scope)
  {
    Type *t = get_type_refd_last();
    string ret_val;
    switch (t->typetype) {
    case T_ERROR:
    case T_PORT:
      // template classes do not exist for these types
      FATAL_ERROR("Type::get_genname_template()");
    case T_ARRAY:
      // a template class has to be instantiated in case of arrays
      // outside type definitions
      if (!t->u.array.in_typedef) {
        ret_val = t->u.array.dimension->get_template_type(
          t->u.array.element_type, p_scope);
        break;
      }
      // no break
    default:
      // in case of other types the name of the template class is derived
      // from the value class by appending a suffix
      ret_val = t->get_genname_value(p_scope);
      ret_val += "_template";
      break;
    }
    return ret_val;
  }

  string Type::get_genname_altname()
  {
    Type *t_last = get_type_refd_last();
    Scope *t_scope = t_last->get_my_scope();
    switch (t_last->typetype) {
    case T_UNDEF:
    case T_ERROR:
    case T_UNRESTRICTEDSTRING:
    case T_OCFT:
    case T_EXTERNAL:
    case T_EMBEDDED_PDV:
    case T_REFD:
    case T_REFDSPEC:
    case T_SELTYPE:
      FATAL_ERROR("Type::get_genname_altname()");
    case T_ENUM_A:
    case T_ENUM_T:
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_SEQOF:
    case T_SETOF:
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SET_A:
    case T_SET_T:
    case T_OPENTYPE:
    case T_ANYTYPE: // FIXME this does not yet work
    case T_PORT:
    case T_COMPONENT:
    case T_ARRAY:
    case T_SIGNATURE:
    case T_FUNCTION:
    case T_ALTSTEP:
    case T_TESTCASE: {
      // user-defined types
      // always use the qualified name (including module identifier)
      string ret_val(t_scope->get_scope_mod_gen()->get_modid().get_name());
      ret_val += '_';
      ret_val += t_last->get_genname_own();
      return ret_val; }
    default:
      // built-in types
      // use the simple class name from the base library
      return t_last->get_genname_value(t_scope);
    }
  }

  string Type::get_typename()
  {
    Type *t = get_type_refd_last();
    const char* tn = get_typename_builtin(t->typetype);
    if (tn != 0) return string(tn);
    switch (t->typetype) {
    case T_COMPONENT:
    case T_SIGNATURE:
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_ANYTYPE:
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SET_A:
    case T_SET_T:
    case T_SEQOF:
    case T_SETOF:
    case T_ENUM_A:
    case T_ENUM_T:
    case T_PORT:
    case T_FUNCTION:
    case T_ALTSTEP:
    case T_TESTCASE:
      return t->get_fullname();
    case T_ARRAY: {
      string dimensions(t->u.array.dimension->get_stringRepr());
      t = t->u.array.element_type;
      while (t->typetype == T_ARRAY) {
        dimensions += t->u.array.dimension->get_stringRepr();
        t = t->u.array.element_type;
      }
      return t->get_typename() + dimensions; }
    default:
      FATAL_ERROR("Type::get_typename()");
      return string();
    } // switch
  }

  // static
  const char* Type::get_typename_builtin(typetype_t tt)
  {
    switch (tt) {
    case T_ERROR:
      return "Erroneous type";
    case T_NULL:
      return "NULL";
    case T_BOOL:
      return "boolean";
    case T_INT:
    case T_INT_A:
      return "integer";
    case T_REAL:
      return "float";
    case T_BSTR:
    case T_BSTR_A:
      return "bitstring";
    case T_HSTR:
      return "hexstring";
    case T_OSTR:
      return "octetstring";
    case T_CSTR:
      return "charstring";
    case T_USTR:
      return "universal charstring";
    case T_UTF8STRING:
      return "UTF8String";
    case T_NUMERICSTRING:
      return "NumericString";
    case T_PRINTABLESTRING:
      return "PrintableString";
    case T_TELETEXSTRING:
      return "TeletexString";
    case T_VIDEOTEXSTRING:
      return "VideotexString";
    case T_IA5STRING:
      return "IA5String";
    case T_GRAPHICSTRING:
      return "GraphicString";
    case T_VISIBLESTRING:
      return "VisibleString";
    case T_GENERALSTRING:
      return "GeneralString";
    case T_UNIVERSALSTRING:
      return "UniversalString";
    case T_BMPSTRING:
      return "BMPString";
    case T_UTCTIME:
      return "UTCTime";
    case T_GENERALIZEDTIME:
      return "GeneralizedTime";
    case T_OBJECTDESCRIPTOR:
      return "ObjectDescriptor";
    case T_OID:
    case T_ROID:
      return "objid";
    case T_ANY:
      return "ANY";
    case T_VERDICT:
      return "verdicttype";
    case T_DEFAULT:
      return "default";
    case T_EXTERNAL:
      return "EXTERNAL";
    case T_EMBEDDED_PDV:
      return "EMBEDDED PDV";
    case T_UNRESTRICTEDSTRING:
      return "CHARACTER STRING";
    case T_OPENTYPE:
      return "open type";
    case T_ADDRESS:
      return "address";
    default:
      return 0;
    }
  }

  string Type::get_genname_typedescriptor(Scope *p_scope)
  {
    Type *t = this;
    for ( ; ; ) {
      /* If it has tags or encoding attributes, then its encoding may be
       * different from the other "equivalent" types and needs to have its own
       * descriptor.
       */
      if (t->is_tagged() || t->rawattrib || t->textattrib || t->jsonattrib ||
          (t->xerattrib && !t->xerattrib->empty() ))
      {
        return t->get_genname_own(p_scope);
      }
      else if (t->is_ref()) {
        if (t->has_encoding(CT_XER)) {
          // just fetch the referenced type and return
          return t->get_type_refd()->get_genname_own(p_scope);
        }
        else
        { // follow the white rabbit
          t = t->get_type_refd();
        }
      }
      else break;
    }
    return t->get_genname_typename(p_scope);
  }

  string Type::get_genname_typename(Scope *p_scope)
  {
    Type *t = get_type_refd_last();
    switch (t->typetype) {
    case T_UTF8STRING:
      return string("UTF8String");
    case T_NUMERICSTRING:
      return string("NumericString");
    case T_PRINTABLESTRING:
      return string("PrintableString");
    case T_TELETEXSTRING:
      return string("TeletexString");
    case T_VIDEOTEXSTRING:
      return string("VideotexString");
    case T_IA5STRING:
      return string("IA5String");
    case T_GRAPHICSTRING:
      return string("GraphicString");
    case T_VISIBLESTRING:
      return string("VisibleString");
    case T_GENERALSTRING:
      return string("GeneralString");
    case T_UNIVERSALSTRING:
      return string("UniversalString");
    case T_BMPSTRING:
      return string("BMPString");
    case T_UTCTIME:
      return string("ASN_UTCTime");
    case T_GENERALIZEDTIME:
      return string("ASN_GeneralizedTime");
    case T_OBJECTDESCRIPTOR:
      return string("ObjectDescriptor");
    case T_ROID:
      return string("ASN_ROID");
    default:
      return t->get_genname_value(p_scope);
    } // switch
  }

  string Type::get_genname_berdescriptor()
  {
    Type *t = this;
    for ( ; ; ) {
      if (t->is_tagged()) return t->get_genname_own(my_scope);
      else if (t->is_ref()) t = t->get_type_refd();
      else break;
    }
    switch (t->typetype) {
    case T_ENUM_A:
    case T_ENUM_T:
      return string("ENUMERATED");
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_OPENTYPE:
      return string("CHOICE");
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SEQOF:
      return string("SEQUENCE");
    case T_SET_A:
    case T_SET_T:
    case T_SETOF:
      return string("SET");
    default:
      return t->get_genname_typename(my_scope);
    } // switch
  }

  string Type::get_genname_rawdescriptor()
  {
    Type *t = this;
    for ( ; ; ) {
      if (t->rawattrib) return t->get_genname_own(my_scope);
      else if (t->is_ref()) t = t->get_type_refd();
      else break;
    }
    return t->get_genname_typename(my_scope);
  }

  string Type::get_genname_textdescriptor()
  {
    Type *t = this;
    for ( ; ; ) {
      if (t->textattrib) return t->get_genname_own(my_scope);
      else if (t->is_ref()) t = t->get_type_refd();
      else break;
    }
    return t->get_genname_typename(my_scope);
  }

  string Type::get_genname_xerdescriptor()
  {
    if (T_REFDSPEC == typetype) {
      return get_genname_typedescriptor(my_scope);
    }
    else return genname;
  }
  
  string Type::get_genname_jsondescriptor()
  {
    Type *t = this;
    while (true) {
      if (t->jsonattrib) return t->get_genname_own(my_scope);
      else if (t->is_ref()) t = t->get_type_refd();
      else break;
    }
    return t->get_genname_typename(my_scope);
  }

  const char* Type::get_genname_typedescr_asnbasetype()
  {
    switch (get_type_refd_last()->typetype) {
    case T_BMPSTRING:
      return "BMPSTRING";
    case T_UNIVERSALSTRING:
      return "UNIVERSALSTRING";
    case T_UTF8STRING:
      return "UTF8STRING";
    case T_TELETEXSTRING:
      return "TELETEXSTRING";
    case T_VIDEOTEXSTRING:
      return "VIDEOTEXSTRING";
    case T_OBJECTDESCRIPTOR:
    case T_GRAPHICSTRING:
      return "GRAPHICSTRING";
    case T_GENERALSTRING:
      return "GENERALSTRING";
    case T_OID:
      return "OBJID";
    case T_ROID:
      return "ROID";
    default:
      return "DONTCARE";
    } // switch
  }

  void Type::dump(unsigned level) const
  {
    DEBUG(level, "Type @ %p,  '%s'", (const void*)this, get_fullname().c_str());
    switch(typetype) {
    case T_ERROR:
      DEBUG(level, "Type: <erroneous>");
      break;
    case T_NULL:
      DEBUG(level, "Type: NULL");
      break;
    case T_BOOL:
      DEBUG(level, "Type: boolean");
      break;
    case T_INT:
      DEBUG(level, "Type: integer");
      break;
    case T_INT_A:
      DEBUG(level, "Type: INTEGER");
      if(u.namednums.block)
        DEBUG(level, "with unparsed block");
      if(u.namednums.nvs) {
        DEBUG(level, "with named numbers (%lu pcs.)",
              (unsigned long) u.namednums.nvs->get_nof_nvs());
        u.namednums.nvs->dump(level+1);
      }
      break;
    case T_REAL:
      DEBUG(level, "Type: float/REAL");
      break;
    case T_ENUM_A:
    case T_ENUM_T:
      DEBUG(level, "Type: enumerated");
      u.enums.eis->dump(level+1);
      break;
    case T_BSTR:
      DEBUG(level, "Type: bitstring");
      break;
    case T_BSTR_A:
      DEBUG(level, "Type: BIT STRING");
      if(u.namednums.block)
        DEBUG(level, "with unparsed block");
      if(u.namednums.nvs) {
        DEBUG(level, "with named numbers (%lu pcs.)",
              (unsigned long) u.namednums.nvs->get_nof_nvs());
        u.namednums.nvs->dump(level+1);
      }
      break;
    case T_HSTR:
      DEBUG(level, "Type: hexstring");
      break;
    case T_OSTR:
      DEBUG(level, "Type: octetstring");
      break;
    case T_CSTR:
      DEBUG(level, "Type: charstring");
      break;
    case T_USTR:
      DEBUG(level, "Type: universal charstring");
      break;
    case T_UTF8STRING:
      DEBUG(level, "Type: UTF8String");
      break;
    case T_NUMERICSTRING:
      DEBUG(level, "Type: NumericString");
      break;
    case T_PRINTABLESTRING:
      DEBUG(level, "Type: PrintableString");
      break;
    case T_TELETEXSTRING:
      DEBUG(level, "Type: TeletexString");
      break;
    case T_VIDEOTEXSTRING:
      DEBUG(level, "Type: VideotexString");
      break;
    case T_IA5STRING:
      DEBUG(level, "Type: IA5String");
      break;
    case T_GRAPHICSTRING:
      DEBUG(level, "Type: GraphicString");
      break;
    case T_VISIBLESTRING:
      DEBUG(level, "Type: VisibleString");
      break;
    case T_GENERALSTRING:
      DEBUG(level, "Type: GeneralString");
      break;
    case T_UNIVERSALSTRING:
      DEBUG(level, "Type: UniversalString");
      break;
    case T_BMPSTRING:
      DEBUG(level, "Type: BMPString");
      break;
    case T_UNRESTRICTEDSTRING:
      DEBUG(level, "Type: CHARACTER STRING");
      break;
    case T_UTCTIME:
      DEBUG(level, "Type: UTCTime");
      break;
    case T_GENERALIZEDTIME:
      DEBUG(level, "Type: GeneralizedTime");
      break;
    case T_OBJECTDESCRIPTOR:
      DEBUG(level, "Type: OBJECT DESCRIPTOR");
      break;
    case T_OID:
      DEBUG(level, "Type: objid/OBJECT IDENTIFIER");
      break;
    case T_ROID:
      DEBUG(level, "Type: RELATIVE-OID");
      break;
    case T_ANYTYPE:
      DEBUG(level, "Type: anytype!!!");
      u.secho.cfm->dump(level+1);
      break;
    case T_CHOICE_T:
      DEBUG(level, "Type: union");
      u.secho.cfm->dump(level+1);
      break;
    case T_CHOICE_A:
      DEBUG(level, "Type: CHOICE");
      if(u.secho.block)
        DEBUG(level, "with unparsed block");
      if(u.secho.ctss) {
        DEBUG(level, "with alternatives (%lu pcs.)",
              (unsigned long) u.secho.ctss->get_nof_comps());
        u.secho.ctss->dump(level+1);
      }
      break;
    case T_SEQOF:
      DEBUG(level, "Type: record of/SEQUENCE OF");
      DEBUG(level+1, "of type:");
      u.seof.ofType->dump(level+2);
      break;
    case T_SETOF:
      DEBUG(level, "Type: set of/SET OF");
      DEBUG(level+1, "of type:");
      u.seof.ofType->dump(level+2);
      break;
    case T_SEQ_T:
      DEBUG(level, "Type: record");
      u.secho.cfm->dump(level+1);
      break;
    case T_SET_T:
      DEBUG(level, "Type: set");
      u.secho.cfm->dump(level+1);
      break;
    case T_SEQ_A:
      DEBUG(level, "Type: SEQUENCE");
      if(u.secho.block)
        DEBUG(level, "with unparsed block");
      if(u.secho.ctss) {
        DEBUG(level, "with components (%lu pcs.)",
              (unsigned long) u.secho.ctss->get_nof_comps());
        u.secho.ctss->dump(level+1);
      }
      break;
    case T_SET_A:
      DEBUG(level, "Type: SET");
      if(u.secho.block)
        DEBUG(level, "with unparsed block");
      if(u.secho.ctss) {
        DEBUG(level, "with components (%lu pcs.)",
              (unsigned long) u.secho.ctss->get_nof_comps());
        u.secho.ctss->dump(level+1);
      }
      break;
    case T_OCFT:
      DEBUG(level, "Type: ObjectClassFieldType (%s)",
            const_cast<Type*>(this)->get_type_refd()->get_stringRepr().c_str());
      break;
    case T_OPENTYPE:
      DEBUG(level, "Type: opentype (mapped to CHOICE)");
      u.secho.cfm->dump(level+1);
      break;
    case T_ANY:
      DEBUG(level, "Type: ANY");
      break;
    case T_EXTERNAL:
      DEBUG(level, "Type: EXTERNAL");
      break;
    case T_EMBEDDED_PDV:
      DEBUG(level, "Type: EMBEDDED PDV");
      break;
    case T_REFD:
      DEBUG(level, "Type: reference");
      u.ref.ref->dump(level+1);
      if(u.ref.type_refd && u.ref.type_refd->typetype==T_OPENTYPE)
        u.ref.type_refd->dump(level+1);
      break;
    case T_REFDSPEC:
      DEBUG(level, "Type: reference (spec) to %s:",
            u.ref.type_refd->get_fullname().c_str());
      u.ref.type_refd->dump(level + 1);
      break;
    case T_SELTYPE:
      DEBUG(level, "Type: selection type");
      DEBUG(level+1, "`%s' <", u.seltype.id->get_dispname().c_str());
      u.seltype.type->dump(level+1);
      break;
    case T_VERDICT:
      DEBUG(level, "Type: verdicttype");
      break;
    case T_PORT:
      DEBUG(level, "Type: port");
      u.port->dump(level + 1);
      break;
    case T_COMPONENT:
      DEBUG(level, "Type: component");
      u.component->dump(level + 1);
      break;
    case T_ADDRESS:
      DEBUG(level, "Type: address");
      break;
    case T_DEFAULT:
      DEBUG(level, "Type: default");
      break;
    case T_ARRAY:
      DEBUG(level, "Type: array");
      DEBUG(level + 1, "element type:");
      u.array.element_type->dump(level + 2);
      DEBUG(level + 1, "dimension:");
      u.array.dimension->dump(level + 2);
      break;
    case T_SIGNATURE:
      DEBUG(level, "Type: signature");
      if (u.signature.parameters) {
        DEBUG(level+1,"parameter(s):");
        u.signature.parameters->dump(level+2);
      }
      if (u.signature.return_type) {
        DEBUG(level+1,"return type");
        u.signature.return_type->dump(level+2);
      }
      if (u.signature.no_block) DEBUG(level+1,"no block");
      if (u.signature.exceptions) {
        DEBUG(level+1,"exception(s):");
        u.signature.exceptions->dump(level+2);
      }
      break;
    case T_FUNCTION:
      DEBUG(level, "Type: function");
      DEBUG(level+1, "Parameters:");
      u.fatref.fp_list->dump(level+2);
      if (u.fatref.return_type) {
        if (!u.fatref.returns_template) {
        DEBUG(level+1, "Return type:");
        } else {
          if (u.fatref.template_restriction==TR_OMIT)
            DEBUG(level+1, "Returns template of type:");
          else
            DEBUG(level+1, "Returns template(%s) of type:",
              Template::get_restriction_name(u.fatref.template_restriction));
        }
        u.fatref.return_type->dump(level+2);
      }
      if(u.fatref.runs_on.ref) {
        DEBUG(level+1, "Runs on clause:");
        u.fatref.runs_on.ref->dump(level+2);
      } else {
        if (u.fatref.runs_on.self) DEBUG(level+1, "Runs on self");
      }
      break;
    case T_ALTSTEP:
      DEBUG(level, "Type: altstep");
      DEBUG(level+1, "Parameters:");
      u.fatref.fp_list->dump(level+2);
      if(u.fatref.runs_on.ref) {
        DEBUG(level+1, "Runs on clause:");
        u.fatref.runs_on.ref->dump(level+2);
      } else {
        if (u.fatref.runs_on.self) DEBUG(level+1, "Runs on self");
      }
      break;
    case T_TESTCASE:
      DEBUG(level, "Type: testcase");
      DEBUG(level+1, "Parameters:");
      u.fatref.fp_list->dump(level+2);
      if(u.fatref.runs_on.ref) {
        DEBUG(level+1, "Runs on clause:");
        u.fatref.runs_on.ref->dump(level+2);
      }
      if(u.fatref.system.ref) {
        DEBUG(level+1, "System clause:");
        u.fatref.system.ref->dump(level+2);
      }
      break;
    default:
      DEBUG(level, "type (%d - %s)", typetype, const_cast<Type*>(this)->get_stringRepr().c_str());
    } // switch
    DEBUG(level, "ownertype %2d", ownertype);
    if(sub_type!=NULL) {
      DEBUG(level, "with subtype");
      sub_type->dump(level+1);
    }
    if(tags) {
      DEBUG(level, "with tags");
      tags->dump(level+1);
    }

    if(w_attrib_path && w_attrib_path->get_with_attr())
    {
      DEBUG(level, "Attributes");
      w_attrib_path->dump(level);
      //w_attrib_path->get_with_attr()->dump(level);
    }

    if (xerattrib) {
      xerattrib->print(get_fullname().c_str());
    }
  }

  SubtypeConstraint::subtype_t Type::get_subtype_type()
  {
    Type* t = get_type_refd_last();
    switch (t->get_typetype()) {
    case T_INT:
    case T_INT_A:
      return SubtypeConstraint::ST_INTEGER;
    case T_REAL:
      return SubtypeConstraint::ST_FLOAT;
    case T_BOOL:
      return SubtypeConstraint::ST_BOOLEAN;
    case T_VERDICT:
      return SubtypeConstraint::ST_VERDICTTYPE;
    case T_OID:
    case T_ROID:
      return SubtypeConstraint::ST_OBJID;
    case T_BSTR:
    case T_BSTR_A:
      return SubtypeConstraint::ST_BITSTRING;
    case T_HSTR:
      return SubtypeConstraint::ST_HEXSTRING;
    case T_OSTR:
      return SubtypeConstraint::ST_OCTETSTRING;
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
    case T_GRAPHICSTRING:
    case T_GENERALSTRING:
    case T_OBJECTDESCRIPTOR:
      // iso2022str
    case T_CSTR:
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_IA5STRING:
    case T_VISIBLESTRING:
    case T_UTCTIME:
    case T_GENERALIZEDTIME:
      return SubtypeConstraint::ST_CHARSTRING;
    case T_USTR:
    case T_UTF8STRING:
    case T_UNIVERSALSTRING:
    case T_BMPSTRING:
      return SubtypeConstraint::ST_UNIVERSAL_CHARSTRING;
    case T_ENUM_T:
    case T_ENUM_A:
    case T_NULL: // FIXME: this should have it's own ST_NULL case
      return SubtypeConstraint::ST_ENUM;
    case T_CHOICE_T:
    case T_CHOICE_A:
    case T_ANYTYPE: // (titan's hacked anytype is a choice)
    case T_OPENTYPE:
      return SubtypeConstraint::ST_UNION;
    case T_SEQOF:
      return SubtypeConstraint::ST_RECORDOF;
    case T_SETOF:
      return SubtypeConstraint::ST_SETOF;
    case T_SEQ_T:
    case T_SEQ_A:
    case T_EXTERNAL: // associated ASN.1 type is a SEQUENCE
    case T_EMBEDDED_PDV: // associated ASN.1 type is a SEQUENCE
    case T_UNRESTRICTEDSTRING: // associated ASN.1 type is a SEQUENCE
      return SubtypeConstraint::ST_RECORD;
    case T_SET_T:
    case T_SET_A:
      return SubtypeConstraint::ST_SET;
    case T_FUNCTION:
      return SubtypeConstraint::ST_FUNCTION;
    case T_ALTSTEP:
      return SubtypeConstraint::ST_ALTSTEP;
    case T_TESTCASE:
      return SubtypeConstraint::ST_TESTCASE;
    default:
      return SubtypeConstraint::ST_ERROR;
    }
  }

  void Type::set_parsed_restrictions(vector<SubTypeParse> *stp)
  {
    if(!parsed_restr)parsed_restr=stp;
    else FATAL_ERROR("Type::set_parsed_restrictions(): restrictions "
      "are already set.");
  }

  bool Type::is_component_internal()
  {
    if (!checked) chk();
    switch (typetype) {
    case T_DEFAULT:
    case T_PORT:
      return true;
    case T_FUNCTION:
    case T_ALTSTEP:
      return u.fatref.runs_on.self;
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
      return u.secho.component_internal;
    case T_SEQOF:
    case T_SETOF:
      return u.seof.component_internal;
    case T_ARRAY:
      return u.array.component_internal;
    case T_SIGNATURE:
      return u.signature.component_internal;
    case T_REFD:
    case T_REFDSPEC:
      return u.ref.component_internal;
    default:
      return false;
    } //switch
  }

  void Type::chk_component_internal(map<Type*,void>& type_chain,
    const char* p_what)
  {
    Type* t_last = get_type_refd_last();
    switch (t_last->typetype) {
    // types that cannot be sent
    case T_DEFAULT:
      error("Default type cannot be %s", p_what);
      break;
    case T_PORT:
      error("Port type `%s' cannot be %s", t_last->get_typename().c_str(),
        p_what);
      break;
    case T_FUNCTION:
      if (t_last->u.fatref.runs_on.self) {
        error("Function type `%s' with 'runs on self' clause cannot be %s",
          t_last->get_typename().c_str(), p_what);
      }
      break;
    case T_ALTSTEP:
      if (t_last->u.fatref.runs_on.self) {
        error("Altstep type `%s' with 'runs on self' clause cannot be %s",
          t_last->get_typename().c_str(), p_what);
      }
      break;
    // structured types that may contain types that cannot be sent
    case T_CHOICE_T:
    case T_SEQ_T:
    case T_SET_T:
    case T_SEQOF:
    case T_SETOF:
    case T_ARRAY:
    case T_SIGNATURE: {
      if (type_chain.has_key(t_last)) break;
      type_chain.add(t_last, 0);
      Error_Context cntxt(this, "In type `%s'", get_typename().c_str());
      switch (t_last->typetype) {
      case T_CHOICE_T:
      case T_SEQ_T:
      case T_SET_T: {
        size_t nof_comps = t_last->get_nof_comps();
        for (size_t i=0; i<nof_comps; i++) {
          Type* t = t_last->get_comp_byIndex(i)->get_type();
          if (t->is_component_internal())
            t->chk_component_internal(type_chain, p_what);
        }
        } break;
      case T_SEQOF:
      case T_SETOF:
        if (t_last->u.seof.ofType->is_component_internal())
          t_last->u.seof.ofType->chk_component_internal(type_chain, p_what);
        break;
      case T_ARRAY:
        if (t_last->u.array.element_type->is_component_internal())
          t_last->u.array.element_type->chk_component_internal(type_chain,
            p_what);
        break;
      case T_SIGNATURE:
        if (t_last->u.signature.parameters) {
          size_t nof_params = t_last->u.signature.parameters->get_nof_params();
          for (size_t i=0; i<nof_params; i++) {
            Type* t = t_last->u.signature.parameters->
              get_param_byIndex(i)->get_type();
            if (t->is_component_internal())
              t->chk_component_internal(type_chain, p_what);
          }
        }
        if (t_last->u.signature.return_type &&
            t_last->u.signature.return_type->is_component_internal()) {
          t_last->u.signature.return_type->chk_component_internal(type_chain,
            p_what);
        }
        if (t_last->u.signature.exceptions) {
          size_t nof_types = t_last->u.signature.exceptions->get_nof_types();
          for (size_t i=0; i<nof_types; i++) {
            Type* t = t_last->u.signature.exceptions->get_type_byIndex(i);
            if (t->is_component_internal())
              t->chk_component_internal(type_chain, p_what);
          }
        }
        break;
      default:
        FATAL_ERROR("Type::chk_component_internal()");
      }
      type_chain.erase(t_last);
    } break;
    default: //all other types are Ok.
      break;
    } // switch
  }

  Type::typetype_t Type::search_for_not_allowed_type(map<Type*,void>& type_chain,
                                         map<typetype_t, void>& not_allowed)
  {
    if (!checked) chk();
    Type* t_last = get_type_refd_last();
    Type::typetype_t ret = t_last->typetype;

    if (not_allowed.has_key(t_last->typetype)) {
      return ret;
    }

    switch (t_last->typetype) {
      case T_CHOICE_T:
      case T_SEQ_T:
      case T_SET_T:
      case T_SEQOF:
      case T_SETOF:
      case T_ARRAY: {
        if (type_chain.has_key(t_last)) {
          break;
        }
        type_chain.add(t_last, 0);
        switch (t_last->typetype) {
          case T_CHOICE_T:
          case T_SEQ_T:
          case T_SET_T: {
            size_t nof_comps = t_last->get_nof_comps();
            for (size_t i = 0; i < nof_comps; ++i) {
              Type* t = t_last->get_comp_byIndex(i)->get_type();
              ret = t->search_for_not_allowed_type(type_chain, not_allowed);
              if (not_allowed.has_key(ret)) {
                return ret;
              }
            }
          } break;
          case T_SEQOF:
          case T_SETOF:
          case T_ARRAY:
            ret = t_last->get_ofType()->search_for_not_allowed_type(type_chain, not_allowed);
            if (not_allowed.has_key(ret)) {
              return ret;
            }
            break;
          default:
            break;
        }
        type_chain.erase(t_last);
      }
      break;
      default:
        break;
    }
    return t_last->typetype;
  }
  
  string Type::get_dispname() const
  {
    string dispname = genname;
    size_t pos = 0;
    while(pos < dispname.size()) {
      pos = dispname.find("__", pos);
      if (pos == dispname.size()) {
        break;
      }
      dispname.replace(pos, 1, "");
      ++pos;
    }
    return dispname;
  }
  
  bool Type::is_pregenerated()
  {
    // records/sets of base types are already pre-generated, only a type alias will be generated
    // exception: record of universal charstring with the XER coding instruction "anyElement"
    if (!force_gen_seof && (T_SEQOF == get_type_refd_last()->typetype ||
        T_SETOF == get_type_refd_last()->typetype) &&
        (NULL == xerattrib || /* check for "anyElement" at the record of type */
        NamespaceRestriction::UNUSED == xerattrib->anyElement_.type_) &&
        (NULL == u.seof.ofType->xerattrib || /* check for "anyElement" at the element type */
        NamespaceRestriction::UNUSED == u.seof.ofType->xerattrib->anyElement_.type_)) {
      switch(u.seof.ofType->get_type_refd_last()->typetype) {
      case T_BOOL:
      case T_INT:
      case T_INT_A:
      case T_REAL:
      case T_BSTR:
      case T_BSTR_A:
      case T_HSTR:
      case T_OSTR:
      case T_CSTR:
      case T_NUMERICSTRING:
      case T_PRINTABLESTRING:
      case T_IA5STRING:
      case T_VISIBLESTRING:
      case T_UNRESTRICTEDSTRING:
      case T_UTCTIME:
      case T_GENERALIZEDTIME:
      case T_USTR:
      case T_UTF8STRING:
      case T_TELETEXSTRING:
      case T_VIDEOTEXSTRING:
      case T_GRAPHICSTRING:
      case T_GENERALSTRING:
      case T_UNIVERSALSTRING:
      case T_BMPSTRING:
      case T_OBJECTDESCRIPTOR:
        return true;
      default:
        return false;
      }
    }
    return false;
  }
  
  bool Type::has_as_value_union()
  {
    if (jsonattrib != NULL && jsonattrib->as_value) {
      return true;
    }
    Type* t = get_type_refd_last();
    switch (t->get_typetype_ttcn3()) {
    case T_CHOICE_T:
      if (t->jsonattrib != NULL && t->jsonattrib->as_value) {
        return true;
      }
      // no break, check alternatives
    case T_SEQ_T:
    case T_SET_T:
      for (size_t i = 0; i < t->get_nof_comps(); ++i) {
        if (t->get_comp_byIndex(i)->get_type()->has_as_value_union()) {
          return true;
        }
      }
      return false;
    case T_SEQOF:
    case T_ARRAY:
      return t->get_ofType()->has_as_value_union();
    default:
      return false;
    }
  }

} // namespace Common

