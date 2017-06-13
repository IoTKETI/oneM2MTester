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
 *   Delic, Adam
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include "../common/dbgnew.hh"
#include "Type.hh"
#include "CompField.hh"
#include "EnumItem.hh"
#include "SigParam.hh"
#include "main.hh"

#include "enum.h"
#include "record.h"
#include "union.h"
#include "record_of.h"
#include "functionref.h"
#include "XSD_Types.hh"


#include "ttcn3/Ttcnstuff.hh"
#include "ttcn3/ArrayDimensions.hh"
#include "ttcn3/Attributes.hh"
#include "ttcn3/signature.h"
#include "XerAttributes.hh"

#include "asn1/TableConstraint.hh"
#include "asn1/Object.hh"
#include "asn1/Tag.hh"
#include "asn1/Ref.hh"

#include "CodeGenHelper.hh"

#include "../common/JSON_Tokenizer.hh"

namespace Common {

using Asn::Tags;
using Ttcn::SingleWithAttrib;

void Type::generate_code(output_struct *target)
{
  if (code_generated) return;
  generate_code_embedded_before(target);
  CodeGenHelper::update_intervals(target);  // TODO: class and template separate everywhere?
  // escape from recursion loops
  if (code_generated) return;
  code_generated = TRUE;
  generate_code_typedescriptor(target);
  string sourcefile = get_sourcefile_attribute();
  if (!sourcefile.empty()) {
    generate_code_include(sourcefile, target);
  } else {
    switch(typetype) {
    case T_ENUM_A:
    case T_ENUM_T:
      generate_code_Enum(target);
      break;
    case T_CHOICE_A:
    case T_CHOICE_T:
    case T_OPENTYPE:
    case T_ANYTYPE:
      generate_code_Choice(target);
      break;
    case T_SEQ_A:
    case T_SEQ_T:
    case T_SET_A:
    case T_SET_T:
      generate_code_Se(target);
      break;
    case T_SEQOF:
    case T_SETOF:
      generate_code_SeOf(target);
      break;
    case T_PORT:
      u.port->generate_code(target);
      break;
    case T_ARRAY:
      generate_code_Array(target);
      break;
    case T_SIGNATURE:
      generate_code_Signature(target);
      break;
    case T_FUNCTION:
    case T_ALTSTEP:
    case T_TESTCASE:
      generate_code_Fat(target);
      break;
    default:
      generate_code_alias(target);
      break;
    } // switch
  }
  CodeGenHelper::update_intervals(target);
  generate_code_embedded_after(target);
  CodeGenHelper::update_intervals(target);
  if (!is_asn1()) {
    if (has_done_attribute()) generate_code_done(target);
    if (sub_type) sub_type->generate_code(*target);
  }
  CodeGenHelper::update_intervals(target);
}

void Type::generate_code_include(const string& sourcefile, output_struct *target)
{
  const char* name = get_genname_own().c_str();
  const char* dispname = get_fullname().c_str();
  target->header.class_decls = mputprintf(target->header.class_decls,
    "class %s;\n"
    "class %s_template;\n",
    name, name);

  target->header.class_defs = mputprintf(target->header.class_defs,
    "// Implementation of type %s\n"
    "#include \"%s.hh\"\n",
    dispname, sourcefile.c_str());
}

void Type::generate_code_embedded_before(output_struct *target)
{
  switch (typetype) {
  case T_SEQ_A:
  case T_SEQ_T:
  case T_SET_A:
  case T_SET_T: {
    size_t nof_comps = get_nof_comps();
    for (size_t i = 0; i < nof_comps; i++) {
      CompField *cf = get_comp_byIndex(i);
      if (!cf->get_is_optional()) {
        // generate code for mandatory record/set fields only
        cf->get_type()->generate_code(
          CodeGenHelper::GetInstance().get_outputstruct(
            cf->get_type()->get_type_refd_last()
          )
        );
        CodeGenHelper::GetInstance().finalize_generation(
          cf->get_type()->get_type_refd_last()
        );
        // escape from recursion loops
        if (code_generated) break;
      }
    }
    break; }
  case T_REFD:
  case T_SELTYPE:
  case T_REFDSPEC:
  case T_OCFT: {
    Type *type_refd = get_type_refd();
    // generate code for the referenced type only if it is defined
    // in the same module
    if (my_scope->get_scope_mod_gen() ==
      type_refd->my_scope->get_scope_mod_gen())
      type_refd->generate_code(target);
    break; }
  case T_SIGNATURE:
    // the parameter types and the return type shall be generated
    if (u.signature.parameters) {
      size_t nof_params = u.signature.parameters->get_nof_params();
      for (size_t i = 0; i < nof_params; i++) {
        u.signature.parameters->get_param_byIndex(i)->get_type()
          ->generate_code(target);
      }
    }
    if (u.signature.return_type)
      u.signature.return_type->generate_code(target);
    break;
  case T_ARRAY:
    u.array.element_type->generate_code(target);
    break;
  default:
    break;
  }
}

void Type::generate_code_embedded_after(output_struct *target)
{
  switch (typetype) {
  case T_SEQ_A:
  case T_SEQ_T:
  case T_SET_A:
  case T_SET_T: {
    size_t nof_comps = get_nof_comps();
    for (size_t i = 0; i < nof_comps; i++) {
      CompField *cf = get_comp_byIndex(i);
      if (cf->get_is_optional()) {
        // generate code for optional record/set fields only
        // mandatory fields are already completed
        Type *t = cf->get_type();
        if (!t->is_pure_refd()) t->generate_code(target);
      }
    }
    break; }
  case T_ANYTYPE:
  case T_CHOICE_A:
  case T_CHOICE_T: {
    size_t nof_comps = get_nof_comps();
    for (size_t i = 0; i < nof_comps; i++) {
      // generate code for all union fields
      Type *t = get_comp_byIndex(i)->get_type();
      if (!t->is_pure_refd()) t->generate_code(target);
    }
    break; }
  case T_OPENTYPE:
    if (u.secho.my_tableconstraint) {
      // generate code for all embedded settings of the object set
      // that is used in the table constraint
      Asn::ObjectSet *os = u.secho.my_tableconstraint->get_os();
      if (os->get_my_scope()->get_scope_mod_gen() ==
          my_scope->get_scope_mod_gen()) os->generate_code(target);
    }
    break;
  case T_SEQOF:
  case T_SETOF:
    // generate code for the embedded type
    if (!u.seof.ofType->is_pure_refd()) u.seof.ofType->generate_code(target);
    break;
  case T_FUNCTION:
  case T_ALTSTEP:
  case T_TESTCASE: {
    size_t nof_params = u.fatref.fp_list->get_nof_fps();
    for(size_t i = 0; i < nof_params; i++) {
      u.fatref.fp_list->get_fp_byIndex(i)->get_Type()
        ->generate_code(target);
    }
    break; }
  default:
    break;
  }
}

void Type::generate_code_typedescriptor(output_struct *target)
{
  bool force_xer = FALSE;
  switch (get_type_refd_last()->typetype) {
  case T_PORT:
  case T_SIGNATURE:
    // do not generate any type descriptor for these non-data types
    return;
  case T_ARRAY:
    // no force xer
    break;

  default:
    switch (ownertype) {
    case OT_TYPE_ASS:
    case OT_TYPE_DEF:
    case OT_COMP_FIELD:
    case OT_RECORD_OF:
    case OT_REF_SPEC:
      force_xer = has_encoding(CT_XER); // && (is_ref() || (xerattrib && !xerattrib->empty()));
      break;
    default:
      break;
    } // switch(ownertype)
    break;
  } // switch

  const string& gennameown = get_genname_own();
  const char *gennameown_str = gennameown.c_str();
  const string& gennametypedescriptor = get_genname_typedescriptor(my_scope);
//printf("generate_code_typedescriptor(%s)\n", gennameown_str);

  // FIXME: force_xer should be elminated. if a type needs a descriptor,
  // it should say so via get_genname_typedescriptor()

  /* genname{type,ber,raw,text,xer}descriptor == gennameown is true if
   * the type needs its own {type,ber,raw,text,xer}descriptor
   * and can't use the descriptor of one of the built-in types.
   */
  if (gennametypedescriptor == gennameown
    || force_xer) {
    // the type has its own type descriptor
    bool generate_ber = has_encoding(CT_BER) && enable_ber();
    const string& gennameberdescriptor = get_genname_berdescriptor();
    if (generate_ber && gennameberdescriptor == gennameown)
      generate_code_berdescriptor(target);

    bool generate_raw = has_encoding(CT_RAW) && enable_raw();
    const string& gennamerawdescriptor = get_genname_rawdescriptor();
    if (generate_raw && gennamerawdescriptor == gennameown)
      generate_code_rawdescriptor(target);

    bool generate_text = has_encoding(CT_TEXT) && enable_text();
    const string& gennametextdescriptor = get_genname_textdescriptor();
    if (generate_text && gennametextdescriptor == gennameown)
      generate_code_textdescriptor(target);

    bool generate_xer = has_encoding(CT_XER) && enable_xer();
    const string& gennamexerdescriptor = get_genname_xerdescriptor();
    if (generate_xer && gennamexerdescriptor == gennameown)
      generate_code_xerdescriptor(target);
    else target->source.global_vars=mputprintf(target->source.global_vars,
      "// No XER for %s\n", gennamexerdescriptor.c_str());
    
    const string& gennamejsondescriptor = get_genname_jsondescriptor();
    bool generate_json = has_encoding(CT_JSON) && enable_json() && 
      gennamejsondescriptor == gennameown;
    if (generate_json) {
      generate_code_jsondescriptor(target);
    }

    // the type descriptor must be always exported.
    // embedded (possibly unnamed) types can be referenced from other modules
    // using field/array sub-references
    target->header.global_vars = mputprintf(target->header.global_vars,
      "extern const TTCN_Typedescriptor_t %s_descr_;\n", gennameown_str);
    target->source.global_vars = mputprintf(target->source.global_vars,
      "const TTCN_Typedescriptor_t %s_descr_ = { \"%s\", ", gennameown_str,
      get_fullname().c_str());

    if(generate_ber)
      target->source.global_vars=mputprintf
        (target->source.global_vars,
         "&%s_ber_, ", gennameberdescriptor.c_str());
    else
      target->source.global_vars=mputstr
        (target->source.global_vars, "NULL, ");

    if (generate_raw)
      target->source.global_vars=mputprintf
        (target->source.global_vars,
         "&%s_raw_, ", gennamerawdescriptor.c_str());
    else
      target->source.global_vars=mputstr
        (target->source.global_vars, "NULL, ");

    if (generate_text)
      target->source.global_vars=mputprintf
        (target->source.global_vars,
         "&%s_text_, ", gennametextdescriptor.c_str());
    else
      target->source.global_vars=mputstr
        (target->source.global_vars, "NULL, ");

    if (generate_xer)
      target->source.global_vars = mputprintf(target->source.global_vars,
        "&%s_xer_, ", gennamexerdescriptor.c_str());
    else
      target->source.global_vars = mputprintf(target->source.global_vars,
        "NULL, ");
    
    if (generate_json) {
      target->source.global_vars = mputprintf(target->source.global_vars,
        "&%s_json_, ", gennamejsondescriptor.c_str());
    } else {
      switch(get_type_refd_last()->typetype) {
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
        // use predefined JSON descriptors instead of null pointers for basic types
        target->source.global_vars = mputprintf(target->source.global_vars,
          "&%s_json_, ", gennamejsondescriptor.c_str());
        break;
      case T_ENUM_T:
      case T_ENUM_A:
        // use a predefined JSON descriptor for enumerated types
        target->source.global_vars = mputstr(target->source.global_vars,
          "&ENUMERATED_json_, ");
        break;
      default:
        target->source.global_vars = mputstr(target->source.global_vars,
          "NULL, ");
      }
    }
    
    if (T_SEQOF == get_type_refd_last()->typetype || 
        T_SETOF == get_type_refd_last()->typetype) {
      target->source.global_vars=mputprintf(target->source.global_vars,
        "&%s_descr_, ", get_type_refd_last()->u.seof.ofType->get_genname_typedescriptor(my_scope).c_str());
    }
    else {
      target->source.global_vars = mputstr(target->source.global_vars,
        "NULL, ");
    }

    target->source.global_vars=mputprintf(target->source.global_vars,
      "TTCN_Typedescriptor_t::%s };\n"
#ifndef NDEBUG
      "\n"
#endif
      , get_genname_typedescr_asnbasetype());
  } else {
    // the type uses the type descriptor of another type
    if (needs_alias()) {
      // we need to generate an aliased type descriptor only if the type is
      // directly accessible by the user
      target->header.global_vars = mputprintf(target->header.global_vars,
        "extern const TTCN_Typedescriptor_t& %s_descr_;\n", gennameown_str);
      target->source.global_vars = mputprintf(target->source.global_vars,
        "const TTCN_Typedescriptor_t& %s_descr_ = %s_descr_;\n",
        gennameown_str, gennametypedescriptor.c_str());
    }
#ifndef NDEBUG
    else {
      target->source.global_vars = mputprintf( target->source.global_vars,
        "// %s_descr_ not needed, use %s_descr_\n",
        gennameown_str, gennametypedescriptor.c_str());
    } // if(needs_alias())
#endif
    
  } // if (gennameown == gennametypedescriptor)
}

void Type::generate_code_berdescriptor(output_struct *target)
{
  const char *gennameown_str = get_genname_own().c_str();
  char *str = mprintf("%sconst ASN_Tag_t %s_tag_[] = { ",
    split_to_slices ? "" : "static ", gennameown_str);
  Tags *joinedtags = build_tags_joined();
  size_t tagarraysize = joinedtags->get_nof_tags();
  for (size_t i = 0; i < tagarraysize; i++) {
    if (i > 0) str = mputstr(str, ", ");
    Tag *t_tag = joinedtags->get_tag_byIndex(i);
    str = mputprintf(str, "{ %s, %su }", t_tag->get_tagclass_str(),
      Int2string(t_tag->get_tagvalue()).c_str());
  }
  delete joinedtags;
  str = mputstr(str, "};\n");
  target->source.global_vars = mputstr(target->source.global_vars, str);
  Free(str);
  target->header.global_vars = mputprintf(target->header.global_vars,
    "extern const ASN_BERdescriptor_t %s_ber_;\n", gennameown_str);
  target->source.global_vars = mputprintf(target->source.global_vars,
    "const ASN_BERdescriptor_t %s_ber_ = { %luu, %s_tag_ };\n",
    gennameown_str, (unsigned long)tagarraysize, gennameown_str);
}

static const char* whitespace_action[3] = {"PRESERVE", "REPLACE", "COLLAPSE"};

extern void change_name(string &name, XerAttributes::NameChange change);
// implemented in Type_chk.cc

void Type::generate_code_xerdescriptor(output_struct* target)
{
  const char *gennameown_str = get_genname_own().c_str();
  target->header.global_vars = mputprintf(target->header.global_vars,
    "extern const XERdescriptor_t %s_xer_;\n", gennameown_str);
  string last_s;

  Type *ot = this;
  for(;;) {
    string full_s  = ot->get_fullname();
    size_t dot_pos = full_s.rfind('.');
    if (full_s.size() == dot_pos) dot_pos = 0;
    last_s = full_s.substr(dot_pos+1); // FIXME: may be better to use replace(pos, n, s)

    if ('&'==last_s[0]    // object class field ?
      ||'<'==last_s[0]) { // <oftype> and the like
      if (ot->is_ref()) {
        ot = ot->get_type_refd();
        /* Fetch the referenced type and use that. Do not use
         * get_type_refd_last() here. In case of a record-of user-defined type:
         * <ttcn>type integer MyInt; type record of MyInt MyRecof;</ttcn>
         * we want "MyInt" and not "integer" */
        continue;
      }
      else { // probably a built-in type, punt with the C++ class name
        last_s = ot->get_genname_value(0);
        break;
      }
    }
    break;
  }

  // Name for basic XER which ignores all the EXER fanciness
  string bxer_name(last_s);

  long ns_index = -1;
//fprintf(stderr, "%2d   gno='%s'\tfn='%s'\n", typetype, gennameown_str, last_s.c_str());
  int atrib=0, any_atr=0, any_elem=0, base64=0, decimal=0, embed=0, list=0,
  text=0, untagged=0, use_nil=0, use_number=0, use_order=0, use_qname=0,
  use_type_attr=0, ws=0, has_1untag=0, form_qualified=0, any_from=0, 
  any_except=0, nof_ns_uris=0, blocked=0, fractionDigits=-1;
  const char* xsd_type = "XSD_NONE";
  const char* dfe_str = 0;
  char** ns_uris = 0;
  char* oftype_descr_name = 0;
  if (xerattrib) {
    change_name(last_s, xerattrib->name_);

    if (xerattrib->namespace_.uri && xerattrib->namespace_.prefix) {
      ns_index = my_scope->get_scope_mod()->get_ns_index(
        xerattrib->namespace_.prefix);
      // This is known to have succeeded
    }

    any_atr = has_aa(xerattrib);
    any_elem= has_ae(xerattrib);
    atrib   = xerattrib->attribute_;
    base64  = xerattrib->base64_;
    blocked = xerattrib->abstract_ || xerattrib->block_;
    decimal = xerattrib->decimal_;
    embed   = xerattrib->embedValues_;
    form_qualified = (xerattrib->form_ & XerAttributes::QUALIFIED)
      || (xerattrib->element_); // a global element is always qualified
    fractionDigits = xerattrib->has_fractionDigits_ ? xerattrib->fractionDigits_ : -1;
    list    = xerattrib->list_;
    untagged= xerattrib->untagged_;
    ws      = xerattrib->whitespace_;
    // only set TEXT if it has no TextToBeUsed (plain "text" for a bool)
    text      = xerattrib->num_text_ && xerattrib->text_->prefix == 0;
    use_nil   = xerattrib->useNil_;
    use_number= xerattrib->useNumber_;
    use_order = xerattrib->useOrder_;
    use_qname = xerattrib->useQName_;
    // In ASN.1, the use of a type identification attribute is optional
    // (encoder's choice) for USE-UNION. However, TTCN-3 removes this choice:
    // it is mandatory to use it when possible (valid choice for ASN.1 too).
    use_type_attr = xerattrib->useType_ || xerattrib->useUnion_;

    xsd_type = XSD_type_to_string(xerattrib->xsd_type);

    if (xerattrib->defaultValue_) {
      Type *t = xerattrib->defaultValue_->get_my_governor();
      dfe_str = xerattrib->defaultValue_->get_genname_own().c_str();
      const_def cdef;
      Code::init_cdef(&cdef);
      t->generate_code_object(&cdef, xerattrib->defaultValue_);
      // Generate the initialization of the dfe values in the post init function
      // because the module params are not initialized in the pre init function
      target->functions.post_init = xerattrib->defaultValue_->generate_code_init
        (target->functions.post_init, xerattrib->defaultValue_->get_lhs_name().c_str());
      Code::merge_cdef(target, &cdef);
      Code::free_cdef(&cdef);
    }
    
    if (any_elem) {
      // data needed for "anyElement from ..." and "anyElement except ..." encoding instructions
      any_from = xerattrib->anyElement_.type_ == NamespaceRestriction::FROM;
      any_except = xerattrib->anyElement_.type_ == NamespaceRestriction::EXCEPT;
      nof_ns_uris = xerattrib->anyElement_.nElements_;
      ns_uris = xerattrib->anyElement_.uris_;
    }
    else if (any_atr) {
      // data needed for "anyAttributes from ..." and "anyAttributes except ..." encoding instructions
      any_from = xerattrib->anyAttributes_.type_ == NamespaceRestriction::FROM;
      any_except = xerattrib->anyAttributes_.type_ == NamespaceRestriction::EXCEPT;
      nof_ns_uris = xerattrib->anyAttributes_.nElements_;
      ns_uris = xerattrib->anyAttributes_.uris_;
    }
  }
  else if (ownertype == OT_COMP_FIELD
    && parent_type && parent_type->xerattrib) {
    //no xerattrib, this must be an element; apply element default
    form_qualified = (parent_type->xerattrib->form_
      & XerAttributes::ELEMENT_DEFAULT_QUALIFIED);
  }

  Type *last = get_type_refd_last();
  has_1untag= last->is_secho() && last->u.secho.has_single_charenc; // does not come from xerattrib

  /* If this is a string type whose grandparent is a record
   * (containing a record-of (this)string) which has EMBED-VALUES,
   * then reuse this string's any_element field in the XER descriptor
   * to signal this (ANY-ELEMENT causes the tag to be omitted,
   * which is what we want in EMBED-VALUES) */
  if (parent_type && parent_type->parent_type) switch (last->typetype) {
  case T_UTF8STRING:
  case T_USTR: // the TTCN equivalent of UTF8String
    if (  T_SEQOF == parent_type->typetype
      && (T_SEQ_T == parent_type->parent_type->typetype
        ||T_SEQ_A == parent_type->parent_type->typetype)
      && parent_type->parent_type->xerattrib) {
      embed |= (parent_type->parent_type->xerattrib->embedValues_);
    }
    break;
  default:
    break;
  }
  size_t last_len = 2 + last_s.size();    // 2 for > \n
  size_t bxer_len = 2 + bxer_name.size(); // 2 for > \n
  
  if ((T_SEQOF == last->typetype || T_SETOF == last->typetype)) {
    oftype_descr_name = mprintf("&%s_xer_", last->u.seof.ofType->get_genname_typedescriptor(my_scope).c_str());
  }
  
  // Generate a separate variable for the namespace URIs, if there are any
  char* ns_uris_var = 0;
  if (ns_uris && nof_ns_uris) {
    ns_uris_var = mputprintf(ns_uris_var, "%s_ns_uris_", gennameown_str);
    target->source.global_vars = mputprintf(target->source.global_vars,
      "const char* %s[] = {", ns_uris_var);
    for (int idx = 0; idx < nof_ns_uris; ++idx) {
      // The unqualified namespace is sometimes stored as an empty string and
      // sometimes as a null pointer -> unify it, always store it as an empty string
      target->source.global_vars = mputprintf(target->source.global_vars,
        "%s\"%s\"", (idx != 0) ? ", " : "", ns_uris[idx] ? ns_uris[idx] : "");
    }
    target->source.global_vars = mputstrn(target->source.global_vars, "};\n", 3);
  }
    
  // Generate the XER descriptor itself
  target->source.global_vars = mputprintf(target->source.global_vars,
    "const XERdescriptor_t       %s_xer_ = { {\"%s>\\n\", \"%s>\\n\"},"
    " {%lu, %lu}, %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s, WHITESPACE_%s, %c%s, "
    "&%s, %ld, %u, %s, %s, %i, %s };\n",
    gennameown_str,
    bxer_name.c_str(), last_s.c_str(), // names
    (unsigned long)bxer_len, (unsigned long)last_len, // lengths
    (any_atr  ? "ANY_ATTRIBUTES" : "0"),
    (any_elem ? " |ANY_ELEMENT" : ""),
    (atrib    ? " |XER_ATTRIBUTE" : ""),
    (base64   ? " |BASE_64" : ""),
    (blocked  ? " |BLOCKED" : ""),
    (decimal  ? " |XER_DECIMAL" : ""),
    (embed    ? " |EMBED_VALUES" : ""),
    (list     ? " |XER_LIST" : ""),
    (text     ? " |XER_TEXT" : ""),
    (untagged   ? " |UNTAGGED" : ""),
    (use_nil    ? " |USE_NIL" : ""),
    (use_number ? " |USE_NUMBER" : ""),
    (use_order  ? " |USE_ORDER" : ""),
    (use_qname  ? " |USE_QNAME" : ""),
    (use_type_attr   ? " |USE_TYPE_ATTR" : ""),
    (has_1untag ? " |HAS_1UNTAGGED" : ""),
    (form_qualified ? "" : " |FORM_UNQUALIFIED"),
    (any_from   ? " |ANY_FROM" : ""),
    (any_except ? " |ANY_EXCEPT" : ""),
    (is_optional_field() ? " |XER_OPTIONAL" : ""),
    whitespace_action[ws],
    (dfe_str ? '&' : ' '), (dfe_str ? dfe_str : "NULL"),
    "module_object",
    ns_index,
    nof_ns_uris,
    (ns_uris_var ? ns_uris_var : "NULL"),
    (oftype_descr_name ? oftype_descr_name : "NULL"),
    fractionDigits,
    xsd_type
    );
  
  Free(ns_uris_var);
  Free(oftype_descr_name);
}

void Type::generate_code_rawdescriptor(output_struct *target)
{
  const char *gennameown_str = get_genname_own().c_str();
  target->header.global_vars = mputprintf(target->header.global_vars,
    "extern const TTCN_RAWdescriptor_t %s_raw_;\n", gennameown_str);
  char *str = mprintf("const TTCN_RAWdescriptor_t %s_raw_ = {",
    gennameown_str);
  if (rawattrib->intx) {
    str = mputstr(str, "RAW_INTX,");
  }
  else {
    str = mputprintf(str, "%d,", rawattrib->fieldlength);
  }
  if (rawattrib->comp == XDEFCOMPL) str = mputstr(str, "SG_2COMPL,");
  else if (rawattrib->comp == XDEFSIGNBIT) str = mputstr(str, "SG_SG_BIT,");
  else str = mputstr(str, "SG_NO,");
  if (rawattrib->byteorder == XDEFLAST) str = mputstr(str, "ORDER_MSB,");
  else str = mputstr(str, "ORDER_LSB,");
  if (rawattrib->align == XDEFLEFT) str = mputstr(str, "ORDER_MSB,");
  else str = mputstr(str, "ORDER_LSB,");
  if (rawattrib->bitorderinfield == XDEFMSB)
    str = mputstr(str, "ORDER_MSB,");
  else str = mputstr(str, "ORDER_LSB,");
  if (rawattrib->bitorderinoctet == XDEFMSB)
    str = mputstr(str, "ORDER_MSB,");
  else str = mputstr(str, "ORDER_LSB,");
  if (rawattrib->extension_bit == XDEFYES)
    str = mputstr(str, "EXT_BIT_YES,");
  else if (rawattrib->extension_bit == XDEFREVERSE)
    str = mputstr(str, "EXT_BIT_REVERSE,");
  else str = mputstr(str, "EXT_BIT_NO,");
  if (rawattrib->hexorder == XDEFHIGH) str = mputstr(str, "ORDER_MSB,");
  else str = mputstr(str, "ORDER_LSB,");
  if (rawattrib->fieldorder == XDEFMSB) str = mputstr(str, "ORDER_MSB,");
  else str = mputstr(str, "ORDER_LSB,");
  if (rawattrib->topleveleind) {
    if (rawattrib->toplevel.bitorder==XDEFLSB)
      str = mputstr(str, "TOP_BIT_LEFT,");
    else str = mputstr(str, "TOP_BIT_RIGHT,");
  } else str = mputstr(str, "TOP_BIT_INHERITED,");
  str = mputprintf(str, "%d,", rawattrib->padding);
  str = mputprintf(str, "%d,", rawattrib->prepadding);
  str = mputprintf(str, "%d,", rawattrib->ptroffset);
  str = mputprintf(str, "%d,", rawattrib->unit);
  str = mputprintf(str, "%d,", rawattrib->padding_pattern_length);
  if (rawattrib->padding_pattern_length > 0)
    str = mputprintf(str, "%s,", my_scope->get_scope_mod_gen()
      ->add_padding_pattern(string(rawattrib->padding_pattern)).c_str());
  else str = mputstr(str, "NULL,");
  str = mputprintf(str, "%d,", rawattrib->length_restrition);
  str = mputprintf(str, "CharCoding::%s};\n",
      rawattrib->stringformat == CharCoding::UTF_8 ? "UTF_8" :
      (rawattrib->stringformat == CharCoding::UTF16 ? "UTF16" : "UNKNOWN"));
  target->source.global_vars = mputstr(target->source.global_vars, str);
  Free(str);
}

void Type::generate_code_textdescriptor(output_struct *target)
{
  const char *gennameown_str = get_genname_own().c_str();
  char *union_member_name=NULL;
  Common::Module *mymod=my_scope->get_scope_mod();
  Type *t = get_type_refd_last();
  switch (t->typetype) {
   case T_BOOL:
    if (textattrib->true_params || textattrib->false_params) {
      target->source.global_vars = mputprintf(target->source.global_vars,
        "%sconst TTCN_TEXTdescriptor_bool %s_bool_ = {", split_to_slices ? "" : "static ", gennameown_str);
      if (split_to_slices) {
        target->header.global_vars = mputprintf(target->header.global_vars,
          "extern const TTCN_TEXTdescriptor_bool %s_bool_;\n", gennameown_str);
      }
      if (textattrib->true_params &&
          textattrib->true_params->encode_token) {
        target->source.global_vars = mputprintf(target->source.global_vars,
          "&%s,", mymod->add_charstring_literal(
          string(textattrib->true_params->encode_token)).c_str());
      } else {
        target->source.global_vars=mputstr(target->source.global_vars,
             "NULL,");
      }
      if (textattrib->true_params &&
          textattrib->true_params->decode_token) {
        char *pstr = make_posix_str_code(
            textattrib->true_params->decode_token,
            textattrib->true_params->case_sensitive);
        target->source.global_vars = mputprintf(target->source.global_vars,
          "&%s,", mymod->add_matching_literal(string(pstr)).c_str());
        Free(pstr);
      } else if (textattrib->true_params &&
                 !textattrib->true_params->case_sensitive) {
        target->source.global_vars = mputprintf(target->source.global_vars,
          "&%s,", mymod->add_matching_literal(
          string("N^(true).*$")).c_str());
      } else {
        target->source.global_vars = mputstr(target->source.global_vars,
          "NULL,");
      }
      if (textattrib->false_params &&
          textattrib->false_params->encode_token) {
        target->source.global_vars = mputprintf(target->source.global_vars,
          "&%s,",mymod->add_charstring_literal(
          string(textattrib->false_params->encode_token)).c_str());
      } else {
        target->source.global_vars = mputstr(target->source.global_vars,
          "NULL,");
      }
      if (textattrib->false_params &&
          textattrib->false_params->decode_token) {
        char *pstr = make_posix_str_code(
            textattrib->false_params->decode_token,
            textattrib->false_params->case_sensitive);
        target->source.global_vars=mputprintf(target->source.global_vars,
             "&%s};\n", mymod->add_matching_literal(string(pstr)).c_str());
        Free(pstr);
      } else if (textattrib->false_params &&
                !textattrib->false_params->case_sensitive) {
        target->source.global_vars = mputprintf(target->source.global_vars,
          "&%s};\n", mymod->add_matching_literal(
          string("N^(false).*$")).c_str());
      } else {
        target->source.global_vars = mputstr(target->source.global_vars,
          "NULL};\n");
      }
      union_member_name = mprintf("(TTCN_TEXTdescriptor_param_values*)"
                                "&%s_bool_", gennameown_str);
    }
    break;
   case T_ENUM_T:
    target->source.global_vars = mputprintf(target->source.global_vars,
      "%sconst TTCN_TEXTdescriptor_enum %s_enum_[] = { ",
        split_to_slices ? "" : "static ", gennameown_str);
    if (split_to_slices) {
      target->header.global_vars = mputprintf(target->header.global_vars,
        "extern const TTCN_TEXTdescriptor_enum %s_enum_[];\n", gennameown_str);
    }
    for (size_t i = 0; i < t->u.enums.eis->get_nof_eis(); i++) {
      if (i > 0) target->source.global_vars =
        mputstr(target->source.global_vars, ", ");
      target->source.global_vars =
        mputstr(target->source.global_vars, "{ ");
      if (textattrib->field_params && textattrib->field_params[i] &&
          textattrib->field_params[i]->value.encode_token) {
        // the encode token is present
        target->source.global_vars = mputprintf(target->source.global_vars,
          "&%s, ", mymod->add_charstring_literal(
          string(textattrib->field_params[i]->value.encode_token)).c_str());
      } else {
        // the encode token is not present: generate a NULL pointer and the
        // RTE will substitute the enumerated value
        target->source.global_vars = mputstr(target->source.global_vars,
          "NULL, ");
      }
      // a pattern must be always present for decoding
      const char *decode_token;
      bool case_sensitive;
      if (textattrib->field_params && textattrib->field_params[i]) {
        if (textattrib->field_params[i]->value.decode_token) {
          // the decode token is present
          decode_token = textattrib->field_params[i]->value.decode_token;
        } else {
          // there is an attribute for the enumerated value,
          // but the decode token is omitted
          // use the value as decode token
          decode_token = t->u.enums.eis->get_ei_byIndex(i)->get_name()
            .get_dispname().c_str();
        }
        // take the case sensitivity from the attribute
        case_sensitive = textattrib->field_params[i]->value.case_sensitive;
      } else {
        // there is no attribute for the enumerated value
        // use the value as decode token
        decode_token = t->u.enums.eis->get_ei_byIndex(i)->get_name()
          .get_dispname().c_str();
        // it is always case sensitive
        case_sensitive = TRUE;
      }
      char *pstr = make_posix_str_code(decode_token, case_sensitive);
      target->source.global_vars = mputprintf(target->source.global_vars,
        " &%s }", mymod->add_matching_literal(string(pstr)).c_str());
      Free(pstr);
    }
    target->source.global_vars = mputstr(target->source.global_vars,
      " };\n");
    union_member_name = mprintf(
             "(TTCN_TEXTdescriptor_param_values*)%s_enum_", gennameown_str);
    break;
   case T_INT:
   case T_CSTR:
    if(textattrib->coding_params.leading_zero ||
       textattrib->coding_params.min_length!=-1 ||
       textattrib->coding_params.max_length!=-1 ||
       textattrib->coding_params.convert!=0 ||
       textattrib->coding_params.just!=1 ||
       textattrib->decoding_params.leading_zero ||
       textattrib->decoding_params.min_length!=-1 ||
       textattrib->decoding_params.max_length!=-1 ||
       textattrib->decoding_params.convert!=0 ||
       textattrib->decoding_params.just!=1 ){
      target->source.global_vars=mputprintf(target->source.global_vars,
        "%sconst TTCN_TEXTdescriptor_param_values %s_par_ = {",
        split_to_slices ? "" : "static ", gennameown_str);
      if (split_to_slices) {
        target->header.global_vars=mputprintf(target->header.global_vars,
          "extern const TTCN_TEXTdescriptor_param_values %s_par_;\n", gennameown_str);
      }
      target->source.global_vars=mputprintf(target->source.global_vars,
           "{%s,%s,%i,%i,%i,%i},{%s,%s,%i,%i,%i,%i}};\n"
           ,textattrib->coding_params.leading_zero?"TRUE":"FALSE"
           ,textattrib->coding_params.repeatable?"TRUE":"FALSE"
           ,textattrib->coding_params.min_length
           ,textattrib->coding_params.max_length
           ,textattrib->coding_params.convert
           ,textattrib->coding_params.just
           ,textattrib->decoding_params.leading_zero?"TRUE":"FALSE"
           ,textattrib->decoding_params.repeatable?"TRUE":"FALSE"
           ,textattrib->decoding_params.min_length
           ,textattrib->decoding_params.max_length
           ,textattrib->decoding_params.convert
           ,textattrib->decoding_params.just);

      union_member_name=mprintf("&%s_par_", gennameown_str);
    }
    break;
   case T_SEQOF:
   case T_SETOF:
    target->source.global_vars=mputprintf(target->source.global_vars,
      "%sconst TTCN_TEXTdescriptor_param_values %s_par_ = {",
      split_to_slices ? "" : "static ", gennameown_str);
    if (split_to_slices) {
      target->header.global_vars=mputprintf(target->header.global_vars,
        "extern const TTCN_TEXTdescriptor_param_values %s_par_;\n", gennameown_str);
    }
    target->source.global_vars=mputprintf(target->source.global_vars,
         "{%s,%s,%i,%i,%i,%i},{%s,%s,%i,%i,%i,%i}};\n"
         ,textattrib->coding_params.leading_zero?"TRUE":"FALSE"
         ,textattrib->coding_params.repeatable?"TRUE":"FALSE"
         ,textattrib->coding_params.min_length
         ,textattrib->coding_params.max_length
         ,textattrib->coding_params.convert
         ,textattrib->coding_params.just
         ,textattrib->decoding_params.leading_zero?"TRUE":"FALSE"
         ,textattrib->decoding_params.repeatable?"TRUE":"FALSE"
         ,textattrib->decoding_params.min_length
         ,textattrib->decoding_params.max_length
         ,textattrib->decoding_params.convert
         ,textattrib->decoding_params.just);

    union_member_name=mprintf("&%s_par_", gennameown_str);
    break;
   default:
    break;
  }

  target->header.global_vars = mputprintf(target->header.global_vars,
    "extern const TTCN_TEXTdescriptor_t %s_text_;\n", gennameown_str);
  target->source.global_vars = mputprintf(target->source.global_vars,
    "const TTCN_TEXTdescriptor_t %s_text_ = {", gennameown_str);

  if (textattrib->begin_val && textattrib->begin_val->encode_token) {
    target->source.global_vars = mputprintf(target->source.global_vars,
      "&%s,", mymod->add_charstring_literal(
      string(textattrib->begin_val->encode_token)).c_str());
  } else {
    target->source.global_vars = mputstr(target->source.global_vars,
      "NULL,");
  }
  if(textattrib->begin_val && textattrib->begin_val->decode_token){
    char *pstr = make_posix_str_code(
      textattrib->begin_val->decode_token,
      textattrib->begin_val->case_sensitive);
    target->source.global_vars = mputprintf(target->source.global_vars,
      "&%s,", mymod->add_matching_literal(string(pstr)).c_str());
    Free(pstr);
  } else {
    target->source.global_vars = mputstr(target->source.global_vars,
      "NULL,");
  }
  if (textattrib->end_val && textattrib->end_val->encode_token) {
    target->source.global_vars = mputprintf(target->source.global_vars,
      "&%s,",mymod->add_charstring_literal(
      string(textattrib->end_val->encode_token)).c_str());
  } else {
    target->source.global_vars = mputstr(target->source.global_vars,
      "NULL,");
  }
  if (textattrib->end_val && textattrib->end_val->decode_token) {
    char *pstr = make_posix_str_code(
      textattrib->end_val->decode_token,
      textattrib->end_val->case_sensitive);
    target->source.global_vars = mputprintf(target->source.global_vars,
      "&%s,", mymod->add_matching_literal(string(pstr)).c_str());
    Free(pstr);
  } else {
    target->source.global_vars = mputstr(target->source.global_vars,
      "NULL,");
  }

  if (textattrib->separator_val &&
      textattrib->separator_val->encode_token) {
    target->source.global_vars = mputprintf(target->source.global_vars,
      "&%s,", mymod->add_charstring_literal(
      string(textattrib->separator_val->encode_token)).c_str());
  } else {
    target->source.global_vars = mputstr(target->source.global_vars,
      "NULL,");
  }
  if(textattrib->separator_val &&
     textattrib->separator_val->decode_token) {
    char *pstr = make_posix_str_code(
      textattrib->separator_val->decode_token,
      textattrib->separator_val->case_sensitive);
    target->source.global_vars = mputprintf(target->source.global_vars,
      "&%s,", mymod->add_matching_literal(string(pstr)).c_str());
    Free(pstr);
  } else {
    target->source.global_vars=mputstr(target->source.global_vars,
      "NULL,");
  }

  if (textattrib->decode_token) {
    char *pstr = make_posix_str_code(textattrib->decode_token,
      textattrib->case_sensitive);
    target->source.global_vars = mputprintf(target->source.global_vars,
      "&%s,", mymod->add_matching_literal(string(pstr)).c_str());
    Free(pstr);
  } else {
    target->source.global_vars = mputstr(target->source.global_vars,
      "NULL,");
  }

  if (union_member_name) {
    target->source.global_vars = mputprintf(target->source.global_vars,
      "{%s}};\n", union_member_name);
    Free(union_member_name);
  } else {
    target->source.global_vars = mputstr(target->source.global_vars,
      "{NULL}};\n");
  }
}

void Type::generate_code_jsondescriptor(output_struct *target)
{
  target->header.global_vars = mputprintf(target->header.global_vars,
    "extern const TTCN_JSONdescriptor_t %s_json_;\n", get_genname_own().c_str());
  
  if (NULL == jsonattrib) {
    target->source.global_vars = mputprintf(target->source.global_vars,
      "const TTCN_JSONdescriptor_t %s_json_ = { FALSE, NULL, FALSE, NULL, FALSE };\n"
      , get_genname_own().c_str());
  } else {
    char* alias = jsonattrib->alias ? mputprintf(NULL, "\"%s\"", jsonattrib->alias) : NULL;
    char* def_val = jsonattrib->default_value ?
      mputprintf(NULL, "\"%s\"", jsonattrib->default_value) : NULL;
    target->source.global_vars = mputprintf(target->source.global_vars,
      "const TTCN_JSONdescriptor_t %s_json_ = { %s, %s, %s, %s, %s };\n"
      , get_genname_own().c_str() 
      , jsonattrib->omit_as_null ? "TRUE" : "FALSE"
      , alias ? alias : "NULL"
      , jsonattrib->as_value ? "TRUE" : "FALSE"
      , def_val ? def_val : "NULL"
      , jsonattrib->metainfo_unbound ? "TRUE" : "FALSE");
    Free(alias);
    Free(def_val);
  }
  
}

void Type::generate_code_alias(output_struct *target)
{
  if (!needs_alias()) return;

  const string& t_genname = get_genname_value(my_scope);
  const char *refd_name = t_genname.c_str();
  const char *own_name = get_genname_own().c_str();

  Type *t_last = get_type_refd_last();
  switch (t_last->typetype) {
  case T_PORT: // only value class exists
    target->header.typedefs = mputprintf(target->header.typedefs,
      "typedef %s %s;\n", refd_name, own_name);
    break;
  case T_SIGNATURE: // special classes (7 pcs.) exist
    target->header.typedefs = mputprintf(target->header.typedefs,
      "typedef %s_call %s_call;\n"
      "typedef %s_call_redirect %s_call_redirect;\n",
      refd_name, own_name, refd_name, own_name);
    if (!t_last->is_nonblocking_signature()) {
      target->header.typedefs = mputprintf(target->header.typedefs,
        "typedef %s_reply %s_reply;\n"
        "typedef %s_reply_redirect %s_reply_redirect;\n",
        refd_name, own_name, refd_name, own_name);
    }
    if (t_last->get_signature_exceptions()) {
      target->header.typedefs = mputprintf(target->header.typedefs,
        "typedef %s_exception %s_exception;\n"
        "typedef %s_exception_template %s_exception_template;\n",
        refd_name, own_name, refd_name, own_name);
    }
    target->header.typedefs = mputprintf(target->header.typedefs,
      "typedef %s_template %s_template;\n",
      refd_name, own_name);
    break;
  default: // value and template classes exist
    target->header.typedefs = mputprintf(target->header.typedefs,
#ifndef NDEBUG
    "// written by %s in " __FILE__ " at %d\n"
#endif
      "typedef %s %s;\n"
      "typedef %s_template %s_template;\n",
#ifndef NDEBUG
    __FUNCTION__, __LINE__,
#endif
      refd_name, own_name, refd_name, own_name);
    break;
  }
}

void Type::generate_code_Enum(output_struct *target)
{
  stringpool pool;
  enum_def e_def;
  memset(&e_def, 0, sizeof(e_def));
  e_def.name = get_genname_own().c_str();
  e_def.dispname = get_fullname().c_str();
  e_def.isASN1 = is_asn1();
  e_def.nElements = u.enums.eis->get_nof_eis();
  e_def.elements = (enum_field*)
    Malloc(e_def.nElements*sizeof(*e_def.elements));
  e_def.firstUnused = u.enums.first_unused;
  e_def.secondUnused = u.enums.second_unused;
  e_def.hasText = textattrib!=NULL;
  e_def.hasRaw = rawattrib!=NULL;
  e_def.hasXer = has_encoding(CT_XER);
  e_def.hasJson = has_encoding(CT_JSON);
  if (xerattrib) {
    e_def.xerUseNumber = xerattrib->useNumber_;
  }
  for (size_t i = 0; i < e_def.nElements; i++) {
    EnumItem *ei = u.enums.eis->get_ei_byIndex(i);
    e_def.elements[i].name = ei->get_name().get_name().c_str();
    e_def.elements[i].dispname = ei->get_name().get_ttcnname().c_str();
    if (ei->get_text().empty()) e_def.elements[i].text = 0;
    else {
      e_def.xerText = TRUE;
      e_def.elements[i].text = ei->get_text().c_str();
    }
    e_def.elements[i].value = ei->get_int_val()->get_val();
  }

  defEnumClass(&e_def, target);
  defEnumTemplate(&e_def, target);

  Free(e_def.elements);
}

void Type::generate_code_Choice(output_struct *target)
{
  stringpool pool;
  struct_def sdef;
  memset(&sdef, 0, sizeof(sdef));
  sdef.name = get_genname_own().c_str();
  sdef.dispname=get_fullname().c_str();
  if (T_ANYTYPE==typetype) {
    if (0 == get_nof_comps()) {
      //return; // don't generate code for empty anytype
      // XXX what to do with empty anytype ?
      // Easy: make sure it doesn't happen by filling it from the AST!
    }
    sdef.kind = ANYTYPE;
  }
  else sdef.kind = UNION;
  sdef.isASN1 = is_asn1();
  sdef.hasText = textattrib!=NULL;
  sdef.hasXer = has_encoding(CT_XER);
  sdef.hasJson = has_encoding(CT_JSON);
  sdef.has_opentypes = get_has_opentypes();
  sdef.opentype_outermost = get_is_opentype_outermost();
  sdef.ot = generate_code_ot(pool);
  sdef.nElements = get_nof_comps();
  sdef.isOptional = FALSE;
  if (parent_type != NULL) {
    switch (parent_type->typetype) {
    case T_SEQ_T:
    case T_SEQ_A:
    case T_SET_T:
    case T_SET_A:
      for (size_t x = 0; x < parent_type->get_nof_comps(); ++x) {
        CompField * cf = parent_type->get_comp_byIndex(x);
        if (cf->get_type() == this && cf->get_is_optional()) {
          sdef.isOptional = TRUE;
          break; // from the for loop
        }
      }
      break;
    default:
      break;
    }
  }
  sdef.elements = (struct_field*)
    Malloc(sdef.nElements*sizeof(*sdef.elements));
  memset(sdef.elements, 0, sdef.nElements*sizeof(*sdef.elements));
  sdef.exerMaybeEmptyIndex = -1;
  if (jsonattrib) {
    sdef.jsonAsValue = jsonattrib->as_value;
  }
  for(size_t i = 0; i < sdef.nElements; i++) {
    CompField *cf = get_comp_byIndex(i);
    const Identifier& id = cf->get_name();
    Type *cftype = cf->get_type();
    string type_name = cftype->get_genname_value(my_scope);
    if (T_ANYTYPE != typetype &&
        cftype->get_my_scope()->get_scope_mod_gen() == my_scope->get_scope_mod_gen()) {
      for (size_t j = 0; j < sdef.nElements; ++j) {
        if (get_comp_byIndex(j)->get_name().get_name() == type_name) {
          // the field's type name clashes with the name of a field in this union,
          // which would cause a C++ compilation error
          // always prefix the type with the namespace in this case
          type_name = my_scope->get_scope_mod_gen()->get_modid().get_name() +
            string("::") + type_name;
          break;
        }
      }
    }
    sdef.elements[i].type = pool.add(type_name);
    sdef.elements[i].typedescrname =
      pool.add(cftype->get_genname_typedescriptor(my_scope));
    sdef.elements[i].typegen = pool.add(cftype->get_genname_xerdescriptor());
    sdef.elements[i].name = id.get_name().c_str();
    sdef.elements[i].dispname = id.get_ttcnname().c_str();
    if (xerattrib) {
      if (cftype->has_empty_xml()) sdef.exerMaybeEmptyIndex = i;
      // This will overwrite lower values, which is what we want.
      
      if (cftype->xerattrib != NULL) {
        sdef.elements[i].xsd_type = cftype->xerattrib->xsd_type;
        sdef.elements[i].xerUseUnion = cftype->xerattrib->useUnion_;
      }
    }
    if (sdef.hasJson && enable_json()) {
      // Determine the JSON value type of each field to make decoding faster
      typetype_t tt = cftype->get_type_refd_last()->typetype;
      switch(tt) {
      case T_INT:
      case T_INT_A:
        sdef.elements[i].jsonValueType = JSON_NUMBER;
        break;
      case T_REAL:
        sdef.elements[i].jsonValueType = JSON_NUMBER | JSON_STRING;
        break;
      case T_BOOL:
        sdef.elements[i].jsonValueType = JSON_BOOLEAN;
        break;
      case T_NULL:
        sdef.elements[i].jsonValueType = JSON_NULL;
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
      case T_VERDICT:
      case T_ENUM_T:
      case T_ENUM_A:
      case T_OID:
      case T_ROID:
      case T_ANY:
        sdef.elements[i].jsonValueType = JSON_STRING;
        break;
      case T_SEQ_T:
      case T_SEQ_A:
      case T_SET_T:
      case T_SET_A:
        if (cftype->get_type_refd_last()->get_nof_comps() > 1) {
          sdef.elements[i].jsonValueType = JSON_OBJECT;
          break;
        }
        // else fall through
      case T_CHOICE_T:
      case T_CHOICE_A:
      case T_ANYTYPE:
      case T_OPENTYPE:
        sdef.elements[i].jsonValueType = JSON_ANY_VALUE;
        break;
      case T_SEQOF:
      case T_SETOF:
      case T_ARRAY:
        sdef.elements[i].jsonValueType = JSON_ARRAY;
        break;
      default:
        FATAL_ERROR("Type::generate_code_Choice - invalid field type %d", tt);
      }
    }
    if (cftype->jsonattrib) {
      sdef.elements[i].jsonAlias = cftype->jsonattrib->alias;
    }
  }
  if(rawattrib) {
    copy_rawAST_to_struct(rawattrib,&(sdef.raw));
    sdef.hasRaw=TRUE;
    // building taglist
    for(int c=0;c<rawattrib->taglist.nElements;c++){
      if(rawattrib->taglist.tag[c].nElements)
        sdef.raw.taglist.list[c].fields=
           (rawAST_coding_field_list*)
                   Malloc(rawattrib->taglist.tag[c].nElements
                                *sizeof(rawAST_coding_field_list));
      else sdef.raw.taglist.list[c].fields=NULL;
      sdef.raw.taglist.list[c].nElements=
                                rawattrib->taglist.tag[c].nElements;
      sdef.raw.taglist.list[c].fieldName=
                      rawattrib->taglist.tag[c].fieldName->get_name().c_str();
      Identifier *idf=rawattrib->taglist.tag[c].fieldName;
      sdef.raw.taglist.list[c].fieldnum=get_comp_index_byName(*idf);
      for(int a=0;a<rawattrib->taglist.tag[c].nElements;a++){
        rawAST_coding_field_list* key=
               sdef.raw.taglist.list[c].fields+a;
        key->nElements=
               rawattrib->taglist.tag[c].keyList[a].keyField->nElements+1;
        key->value=rawattrib->taglist.tag[c].keyList[a].value;
        key->start_pos=0;
        key->fields=(rawAST_coding_fields*)
               Malloc(key->nElements*sizeof(rawAST_coding_fields));
        CompField *cf=get_comp_byIndex(sdef.raw.taglist.list[c].fieldnum);
        Type *t=cf->get_type()->get_type_refd_last();

        key->fields[0].nthfield = sdef.raw.taglist.list[c].fieldnum;
        key->fields[0].nthfieldname =
          rawattrib->taglist.tag[c].fieldName->get_name().c_str();
        key->fields[0].fieldtype = UNION_FIELD;
        key->fields[0].type = pool.add(t->get_genname_value(my_scope));
        key->fields[0].typedescr =
          pool.add(t->get_genname_typedescriptor(my_scope));

        for (int b = 1; b < key->nElements; b++) {
          Identifier *idf2 =
            rawattrib->taglist.tag[c].keyList[a].keyField->names[b-1];
          size_t comp_index = t->get_comp_index_byName(*idf2);
          CompField *cf2 = t->get_comp_byIndex(comp_index);
          key->fields[b].nthfield = comp_index;
          key->fields[b].nthfieldname = idf2->get_name().c_str();
          if (t->typetype == T_CHOICE_T)
            key->fields[b].fieldtype = UNION_FIELD;
          else if (cf2->get_is_optional()){
            key->fields[b].fieldtype = OPTIONAL_FIELD;
          }else key->fields[b].fieldtype = MANDATORY_FIELD;
          Type *field_type = cf2->get_type();
          key->fields[b].type =
            pool.add(field_type->get_genname_value(my_scope));
          key->fields[b].typedescr =
            pool.add(field_type->get_genname_typedescriptor(my_scope));
          if (field_type->typetype == T_SEQ_T && field_type->rawattrib
              && (field_type->rawattrib->pointerto
                  || field_type->rawattrib->lengthto_num))
            key->start_pos = -1;

          if(t->typetype != T_CHOICE_T && t->typetype != T_SET_T){
            Type *t2;
            for(size_t i = 0; i < comp_index && key->start_pos >=0; i++)
            {
              t2 = t->get_comp_byIndex(i)->get_type();
              if(t2->get_raw_length() >= 0){
                if(t2->rawattrib)
                  key->start_pos += t2->rawattrib->padding;
                key->start_pos += t2->get_raw_length();
              }else key->start_pos = -1;
            }
          }
          t = field_type->get_type_refd_last();
        }
      }
    }
  } else sdef.hasRaw=FALSE;
  if (xerattrib) {
    Module *my_module = get_my_scope()->get_scope_mod();
    sdef.xerHasNamespaces = my_module->get_nof_ns() != 0;
    const char *ns, *prefix;
    my_module->get_controlns(ns, prefix);
    sdef.control_ns_prefix = prefix;
    sdef.xerUseUnion = xerattrib->useUnion_;
    sdef.xerUseTypeAttr  = xerattrib->useType_ || xerattrib->useUnion_;
  }
  defUnionClass(&sdef, target);
  defUnionTemplate(&sdef, target);

  free_code_ot(sdef.ot);
  sdef.ot=0;
  if (rawattrib) {
    free_raw_attrib_struct(&sdef.raw);
  }
  Free(sdef.elements);
}

Opentype_t *Type::generate_code_ot(stringpool& pool)
{
  using namespace Asn;
  if(typetype!=T_OPENTYPE)
    return 0;
  if(!u.secho.my_tableconstraint
     || !u.secho.my_tableconstraint->get_ans()) {
    DEBUG(1, "Opentype ObjectClassFieldType without"
          " ComponentRelationConstraint: `%s'",
          get_fullname().c_str());
    return 0;
  }
  const AtNotations *ans=u.secho.my_tableconstraint->get_ans();
  Opentype_t *ot=(Opentype_t*)Malloc(sizeof(*ot));
  ot->anl.nElements = ans->get_nof_ans();
  ot->anl.elements = (AtNotation_t*)
    Malloc(ot->anl.nElements * sizeof(*ot->anl.elements));
  for(size_t i=0; i<ans->get_nof_ans(); i++) {
    AtNotation *an=ans->get_an_byIndex(i);
    AtNotation_t *an_t = ot->anl.elements + i;
    an_t->dispname = pool.add(an->get_dispname());
    an_t->parent_level=an->get_levels();
    an_t->parent_typename =
      pool.add(an->get_firstcomp()->get_genname_value(my_scope));
    an_t->type_name =
      pool.add(an->get_lastcomp()->get_genname_value(my_scope));
    an_t->sourcecode=memptystr();
    FieldName* cids=an->get_cids();
    Type *t_type=an->get_firstcomp();
    for(size_t j=0; j<cids->get_nof_fields(); j++) {
      CompField *cf=
        t_type->get_comp_byName(*cids->get_field_byIndex(j));
      if(j) an_t->sourcecode=mputstr(an_t->sourcecode, ".");
      an_t->sourcecode=mputprintf
        (an_t->sourcecode, "%s()",
         cf->get_name().get_name().c_str());
      if(cf->get_is_optional())
        an_t->sourcecode=mputstr(an_t->sourcecode, "()");
      t_type=cf->get_type();
    } // for j
  } // i
  const Identifier *oc_fieldname_t
    =u.secho.my_tableconstraint->get_oc_fieldname();
  Objects *objs
    =u.secho.my_tableconstraint->get_os()->get_refd_last()->get_objs();
  ot->oal.nElements = objs->get_nof_objs();
  ot->oal.elements = (OpentypeAlternative_t*)
    Malloc(ot->oal.nElements * sizeof(*ot->oal.elements));
  size_t nElements_missing=0;
  Value **val_prev=(Value**)
    Malloc(ans->get_nof_ans()*sizeof(*val_prev));
  boolean differs_from_prev=TRUE;
  for(size_t i=0; i<objs->get_nof_objs(); i++) {
    Obj_defn *obj=objs->get_obj_byIndex(i);
    if(!obj->has_fs_withName_dflt(*oc_fieldname_t)) {
      nElements_missing++;
      continue;
    }
    OpentypeAlternative_t *oa_t = ot->oal.elements + i - nElements_missing;
    Type *t_type=dynamic_cast<Type*>
      (obj->get_setting_byName_dflt(*oc_fieldname_t));
    bool is_strange;
    const Identifier& altname = t_type->get_otaltname(is_strange);
    oa_t->alt = pool.add(altname.get_name());
    oa_t->alt_dispname = pool.add(altname.get_asnname());
    oa_t->alt_typename = pool.add(t_type->get_genname_value(my_scope));
    oa_t->alt_typedescrname =
      pool.add(t_type->get_genname_typedescriptor(my_scope));
    oa_t->valuenames=(const char**)Malloc
      (ans->get_nof_ans()*sizeof(*oa_t->valuenames));
    oa_t->const_valuenames=(const char**)Malloc
      (ans->get_nof_ans()*sizeof(*oa_t->const_valuenames));
    for(size_t j=0; j<ans->get_nof_ans(); j++) {
      AtNotation *an=ans->get_an_byIndex(j);
      const Identifier *oc_fieldname_v=an->get_oc_fieldname();
      Value *t_value=dynamic_cast<Value*>
        (obj->get_setting_byName_dflt(*oc_fieldname_v));
      oa_t->valuenames[j] = pool.add(t_value->get_genname_own(my_scope));
      if(!differs_from_prev && *val_prev[j]==*t_value)
        oa_t->const_valuenames[j]=0;
      else {
        oa_t->const_valuenames[j] =
          pool.add(t_value->get_genname_own(my_scope));
        differs_from_prev=TRUE;
      }
      val_prev[j]=t_value;
    } //j
    differs_from_prev=FALSE;
  } // i
  Free(val_prev);
  ot->oal.nElements -= nElements_missing;
  ot->oal.elements = (OpentypeAlternative_t*)
    Realloc(ot->oal.elements,
            ot->oal.nElements * sizeof(*ot->oal.elements));
  return ot;
}

void Type::free_code_ot(Opentype_t* p_ot)
{
  if (!p_ot) return;
  for (size_t i = 0; i < p_ot->oal.nElements; i++) {
    Free(p_ot->oal.elements[i].valuenames);
    Free(p_ot->oal.elements[i].const_valuenames);
  }
  Free(p_ot->oal.elements);
  for (size_t i = 0; i < p_ot->anl.nElements; i++)
    Free(p_ot->anl.elements[i].sourcecode);
  Free(p_ot->anl.elements);
  Free(p_ot);
}

size_t Type::get_codegen_index(size_t index)
{
  // This sorting is because of CER coding of SET types, see X.690 9.3.
  // see: Type::generate_code_Se()
  // TODO: maybe result should be cached into this type
  //       ( inside u.secho as dynamic_array<size_t>* codegen_indexes ? )
  if (typetype==T_SET_A) {
    size_t nof_comps = get_nof_comps();
    map<Tag, void> se_index_map;
    for (size_t i=0; i<nof_comps; i++) {
      Tag *tag = get_comp_byIndex(i)->get_type()->get_smallest_tag();
      se_index_map.add(*tag, (void*)i); // hack: store size_t in void* to avoid Malloc()
      delete tag;
    }
    for(size_t i=0; i<nof_comps; i++) {
      if (se_index_map.get_nth_elem(i)==(void*)index) {
        se_index_map.clear();
        return i;
      }
    }
    FATAL_ERROR("Type::get_codegen_index()");
  }
  return index;
}

void Type::generate_code_Se(output_struct *target)
{
  stringpool pool;
  struct_def sdef;
  Type * last_field_type = 0;
  memset(&sdef, 0, sizeof(sdef));
  sdef.name = get_genname_own().c_str();
  sdef.dispname = get_fullname().c_str();
//printf("generate_code_Se(%s)\n", sdef.dispname);
  switch(typetype) {
  case T_SEQ_A:
    sdef.kind=RECORD;
    sdef.isASN1=TRUE;
    break;
  case T_SEQ_T:
    sdef.kind=RECORD;
    sdef.isASN1=FALSE;
    break;
  case T_SET_A:
    sdef.kind=SET;
    sdef.isASN1=TRUE;
    break;
  case T_SET_T:
    sdef.kind=SET;
    sdef.isASN1=FALSE;
    break;
  default:
    FATAL_ERROR("Type::generate_code_Se()");
  } // switch
  sdef.hasText = textattrib!=NULL;
  sdef.nElements = sdef.totalElements = get_nof_comps();
  sdef.has_opentypes = get_has_opentypes();
  sdef.opentype_outermost = get_is_opentype_outermost();
  sdef.ot = NULL;
  sdef.hasXer = has_encoding(CT_XER);
  sdef.hasJson = has_encoding(CT_JSON);
  if (xerattrib){
    Module *my_module = get_my_scope()->get_scope_mod();
    sdef.xerHasNamespaces = my_module->get_nof_ns() != 0;
    const char *ns, *prefix;
    my_module->get_controlns(ns, prefix);
    sdef.control_ns_prefix = prefix;
    sdef.xerUntagged = xerattrib->untagged_;
    sdef.xerUntaggedOne = u.secho.has_single_charenc;
    sdef.xerUseNilPossible   = use_nil_possible;
    sdef.xerEmbedValuesPossible = embed_values_possible;
    sdef.xerUseOrderPossible = use_order_possible;
    if (xerattrib->useOrder_ && xerattrib->useNil_) {
      // We need information about the fields of the USE-NIL component
      const CompField *cf = get_comp_byIndex(sdef.totalElements-1);
      last_field_type = cf->get_type()->get_type_refd_last();
      sdef.totalElements += last_field_type->get_nof_comps();
    }
    sdef.xerUseQName = xerattrib->useQName_;
    if (xerattrib->useType_ || xerattrib->useUnion_) {
      FATAL_ERROR("Type::generate_code_Se()"); // union only, not for record
    }
  }
  sdef.elements = (struct_field*)
    Malloc(sdef.totalElements*sizeof(*sdef.elements));
  memset(sdef.elements, 0, sdef.totalElements * sizeof(*sdef.elements));

  /* This sorting is because of CER coding of SET types, see X.690
     9.3. */
  vector<CompField> se_comps;
  if(typetype==T_SET_A) {
    map<Tag, CompField> se_comps_map;
    for(size_t i=0; i<sdef.nElements; i++) {
      CompField* cf=get_comp_byIndex(i);
      Tag *tag = cf->get_type()->get_smallest_tag();
      se_comps_map.add(*tag, cf);
      delete tag;
    }
    for(size_t i=0; i<sdef.nElements; i++)
      se_comps.add(se_comps_map.get_nth_elem(i));
    se_comps_map.clear();
  }
  else {
    for(size_t i=0; i<sdef.nElements; i++)
      se_comps.add(get_comp_byIndex(i));
  }

  for(size_t i = 0; i < sdef.nElements; i++) {
    struct_field &cur = sdef.elements[i];
    CompField *cf = se_comps[i];
    const Identifier& id = cf->get_name();
    Type *type = cf->get_type();
    string type_name = type->get_genname_value(my_scope);
    if (type->get_my_scope()->get_scope_mod_gen() == my_scope->get_scope_mod_gen()) {
      for (size_t j = 0; j < sdef.nElements; ++j) {
        if (se_comps[j]->get_name().get_name() == type_name) {
          // the field's type name clashes with the name of a field in this record/set,
          // which would cause a C++ compilation error
          // always prefix the type with the namespace in this case
          type_name = my_scope->get_scope_mod_gen()->get_modid().get_name() +
            string("::") + type_name;
          break;
        }
      }
    }
    cur.type    = pool.add(type_name);
    cur.typegen = pool.add(type->get_genname_own());
    cur.of_type = type->get_type_refd_last()->is_seof();
    cur.typedescrname =
      pool.add(type->get_genname_typedescriptor(my_scope));
    cur.name = id.get_name().c_str();
    cur.dispname = id.get_ttcnname().c_str();
    cur.isOptional = cf->get_is_optional();
    cur.isDefault = cf->has_default();
    cur.optimizedMemAlloc = cur.of_type && (type->get_optimize_attribute() == "memalloc");
    if (cur.isDefault) {
      Value *defval = cf->get_defval();
      const_def cdef;
      Code::init_cdef(&cdef);
      type->generate_code_object(&cdef, defval);
      cdef.init = defval->generate_code_init
        (cdef.init, defval->get_lhs_name().c_str());
      Code::merge_cdef(target, &cdef);
      Code::free_cdef(&cdef);
      cur.defvalname = defval->get_genname_own().c_str();
    }

    if (type->xerattrib) {
      cur.xerAttribute = type->xerattrib->attribute_;

      if (has_aa(type->xerattrib)) {
        cur.xerAnyNum  = type->xerattrib->anyAttributes_.nElements_;
        cur.xerAnyKind = ANY_ATTRIB_BIT |
          (type->xerattrib->anyAttributes_.type_ == NamespaceRestriction::FROM ?
            ANY_FROM_BIT : ANY_EXCEPT_BIT);
        if (cur.xerAnyNum > 0)
          cur.xerAnyUris = (char**)Malloc(cur.xerAnyNum * sizeof(char*));
        for (size_t uu=0; uu<cur.xerAnyNum; ++uu)
          cur.xerAnyUris[uu] = type->xerattrib->anyAttributes_.uris_[uu];
      }
      else if(has_ae(type->xerattrib)) {
        cur.xerAnyNum  = type->xerattrib->anyElement_.nElements_;
        cur.xerAnyKind = ANY_ELEM_BIT |
          (type->xerattrib->anyElement_.type_ == NamespaceRestriction::FROM ?
            ANY_FROM_BIT : ANY_EXCEPT_BIT);
        if (cur.xerAnyNum > 0)
          cur.xerAnyUris = (char**)Malloc(cur.xerAnyNum* sizeof(char*));
        for (size_t uu=0; uu<cur.xerAnyNum; ++uu)
          cur.xerAnyUris[uu] = type->xerattrib->anyElement_.uris_[uu];
      }
    } // if xerattrib
    if (type->jsonattrib) {
      cur.jsonOmitAsNull = type->jsonattrib->omit_as_null;
      cur.jsonAlias = type->jsonattrib->alias;
      cur.jsonDefaultValue = type->jsonattrib->default_value;
      cur.jsonMetainfoUnbound = type->jsonattrib->metainfo_unbound;
    } // if jsonattrib
  } // next element

  if (last_field_type)
  for (size_t i = sdef.nElements; i < sdef.totalElements; i++) {
    struct_field &cur = sdef.elements[i];
    CompField *cf = last_field_type->get_comp_byIndex(i - sdef.nElements);
    const Identifier& id = cf->get_name();
    Type *type = cf->get_type();
    cur.type    = pool.add(type->get_genname_value(my_scope));
    cur.typegen = pool.add(type->get_genname_own());
    cur.of_type = type->get_type_refd_last()->is_seof();
    cur.typedescrname =
      pool.add(type->get_genname_typedescriptor(my_scope));
    cur.name = id.get_name().c_str();
    cur.dispname = id.get_ttcnname().c_str();
    cur.isOptional = cf->get_is_optional();
  }
  se_comps.clear();

  if(rawattrib) {
    copy_rawAST_to_struct(rawattrib,&(sdef.raw));
    sdef.hasRaw=TRUE;
    // building taglist
    for(int c=0;c<rawattrib->taglist.nElements;c++) {
      if(rawattrib->taglist.tag[c].nElements)
        sdef.raw.taglist.list[c].fields=
          (rawAST_coding_field_list*)
             Malloc(rawattrib->taglist.tag[c].nElements
               *sizeof(rawAST_coding_field_list));
      else sdef.raw.taglist.list[c].fields=NULL;
      sdef.raw.taglist.list[c].nElements=
        rawattrib->taglist.tag[c].nElements;
      sdef.raw.taglist.list[c].fieldName=
        rawattrib->taglist.tag[c].fieldName->get_name().c_str();
      Identifier *idf=rawattrib->taglist.tag[c].fieldName;
      sdef.raw.taglist.list[c].fieldnum=get_comp_index_byName(*idf);
      for(int a=0;a<rawattrib->taglist.tag[c].nElements;a++){
        rawAST_coding_field_list* key=
          sdef.raw.taglist.list[c].fields+a;
        key->nElements=
          rawattrib->taglist.tag[c].keyList[a].keyField->nElements+1;
        key->value=rawattrib->taglist.tag[c].keyList[a].value;
        key->start_pos=0;
        key->fields=(rawAST_coding_fields*)
          Malloc(key->nElements*sizeof(rawAST_coding_fields));

        CompField *cf=get_comp_byIndex(sdef.raw.taglist.list[c].fieldnum);
        Type *t=cf->get_type()->get_type_refd_last();

        key->fields[0].nthfield = sdef.raw.taglist.list[c].fieldnum;
        key->fields[0].nthfieldname =
          rawattrib->taglist.tag[c].fieldName->get_name().c_str();
        if (cf->get_is_optional())
          key->fields[0].fieldtype = OPTIONAL_FIELD;
        else key->fields[0].fieldtype = MANDATORY_FIELD;
        key->fields[0].type = pool.add(t->get_genname_value(my_scope));
        key->fields[0].typedescr =
          pool.add(t->get_genname_typedescriptor(my_scope));

        CompField *cf2;
        for (int b = 1; b < key->nElements; b++) {
          Identifier *idf2 =
            rawattrib->taglist.tag[c].keyList[a].keyField->names[b-1];
          size_t comp_index = t->get_comp_index_byName(*idf2);
          cf2 = t->get_comp_byIndex(comp_index);
          key->fields[b].nthfield = comp_index;
          key->fields[b].nthfieldname = idf2->get_name().c_str();
          if (t->typetype == T_CHOICE_T)
            key->fields[b].fieldtype = UNION_FIELD;
          else if (cf2->get_is_optional())
            key->fields[b].fieldtype = OPTIONAL_FIELD;
          else key->fields[b].fieldtype = MANDATORY_FIELD;
          Type *field_type = cf2->get_type();
          key->fields[b].type =
            pool.add(field_type->get_genname_value(my_scope));
          key->fields[b].typedescr =
            pool.add(field_type->get_genname_typedescriptor(my_scope));
          if (field_type->typetype == T_SEQ_T && field_type->rawattrib
              && (field_type->rawattrib->pointerto
                  || field_type->rawattrib->lengthto_num))
            key->start_pos = -1;

          if(t->typetype != T_CHOICE_T && t->typetype != T_SET_T){
            Type *t2;
            for(size_t i = 0; i < comp_index && key->start_pos >=0; i++)
            {
              t2 = t->get_comp_byIndex(i)->get_type();
              if(t2->get_raw_length() >= 0){
                if(t2->rawattrib)
                  key->start_pos += t2->rawattrib->padding;
                key->start_pos += t2->get_raw_length();
              }else key->start_pos = -1;
            }
          }
          t = field_type->get_type_refd_last();
        }
      }
    }
    // building presence list
    for(int a=0;a<rawattrib->presence.nElements;a++) {
      rawAST_coding_field_list* presences=sdef.raw.presence.fields+a;
      presences->nElements=
                          rawattrib->presence.keyList[a].keyField->nElements;
      presences->value=rawattrib->presence.keyList[a].value;
      presences->fields=(rawAST_coding_fields*)
        Malloc(presences->nElements*sizeof(rawAST_coding_fields));
      Type *t = this;
      for (int b = 0; b < presences->nElements; b++) {
        Identifier *idf = rawattrib->presence.keyList[a].keyField->names[b];
        size_t comp_index = t->get_comp_index_byName(*idf);
        CompField *cf = t->get_comp_byIndex(comp_index);
        presences->fields[b].nthfield = comp_index;
        presences->fields[b].nthfieldname = idf->get_name().c_str();
        if (t->typetype == T_CHOICE_T)
          presences->fields[b].fieldtype = UNION_FIELD;
        else if (cf->get_is_optional())
          presences->fields[b].fieldtype = OPTIONAL_FIELD;
        else presences->fields[b].fieldtype = MANDATORY_FIELD;
        Type *field_type = cf->get_type();
        presences->fields[b].type =
          pool.add(field_type->get_genname_value(my_scope));
        presences->fields[b].typedescr =
          pool.add(field_type->get_genname_typedescriptor(my_scope));
        t = field_type->get_type_refd_last();
      }
    }
    for(int c=0;c<rawattrib->ext_bit_goup_num;c++){
      Identifier *idf=rawattrib->ext_bit_groups[c].from;
      Identifier *idf2=rawattrib->ext_bit_groups[c].to;
      sdef.raw.ext_bit_groups[c].ext_bit=rawattrib->ext_bit_groups[c].ext_bit;
      sdef.raw.ext_bit_groups[c].from=(int)get_comp_index_byName(*idf);
      sdef.raw.ext_bit_groups[c].to=(int)get_comp_index_byName(*idf2);
    }
    for(size_t i=0; i<sdef.totalElements; i++) {
      CompField *cf = get_comp_byIndex(i);
      Type *t_field = cf->get_type();
      Type *t_field_last = t_field->get_type_refd_last();
      RawAST *rawpar = t_field->rawattrib;
      if(rawpar) {
        copy_rawAST_to_struct(rawpar,&(sdef.elements[i].raw));
        for(int j=0; j<rawpar->lengthto_num;j++){
          Identifier *idf=rawpar->lengthto[j];
          sdef.elements[i].raw.lengthto[j]=get_comp_index_byName(*idf);
        }
        if (rawpar->lengthto_num && rawpar->lengthindex) {
          Identifier *idf = rawpar->lengthindex->names[0];
          size_t comp_index = t_field_last->get_comp_index_byName(*idf);
          sdef.elements[i].raw.lengthindex->nthfield = comp_index;
          sdef.elements[i].raw.lengthindex->nthfieldname =
            idf->get_name().c_str();
          CompField *cf2 = t_field_last->get_comp_byIndex(comp_index);
          Type *t_field2 = cf2->get_type();
          if (t_field2->typetype == T_CHOICE_T)
            sdef.elements[i].raw.lengthindex->fieldtype = UNION_FIELD;
          else if (cf2->get_is_optional())
            sdef.elements[i].raw.lengthindex->fieldtype = OPTIONAL_FIELD;
          else sdef.elements[i].raw.lengthindex->fieldtype = MANDATORY_FIELD;
          sdef.elements[i].raw.lengthindex->type =
            pool.add(t_field2->get_genname_value(my_scope));
          sdef.elements[i].raw.lengthindex->typedescr =
            pool.add(t_field2->get_genname_typedescriptor(my_scope));
        }
        if (rawpar->lengthto_num && !rawpar->lengthindex &&
            t_field_last->is_secho()) {
          int comp_num=(int)t_field_last->get_nof_comps();
          sdef.elements[i].raw.union_member_num=comp_num;
          sdef.elements[i].raw.member_name=
                (const char **)Malloc((comp_num+1)*sizeof(const char*));
          sdef.elements[i].raw.member_name[0] =
            pool.add(t_field_last->get_genname_value(my_scope));
          for(int m=1;m<comp_num+1;m++){
            CompField *compf=t_field_last->get_comp_byIndex(m-1);
            sdef.elements[i].raw.member_name[m]=
                compf->get_name().get_name().c_str();
          }
        }
        if(rawpar->pointerto){
          Identifier *idf=rawpar->pointerto;
          sdef.elements[i].raw.pointerto=get_comp_index_byName(*idf);
          if(rawpar->ptrbase){
            Identifier *idf2=rawpar->ptrbase;
            sdef.elements[i].raw.pointerbase=get_comp_index_byName(*idf2);
          } else sdef.elements[i].raw.pointerbase=i;
        }
        // building presence list
        for(int a=0;a<rawpar->presence.nElements;a++) {
          rawAST_coding_field_list* presences=
            sdef.elements[i].raw.presence.fields+a;
          presences->nElements=
                              rawpar->presence.keyList[a].keyField->nElements;
          presences->value=rawpar->presence.keyList[a].value;
          presences->fields=(rawAST_coding_fields*)
            Malloc(presences->nElements*sizeof(rawAST_coding_fields));
          Type *t = this;
          for (int b = 0; b < presences->nElements; b++) {
            Identifier *idf = rawpar->presence.keyList[a].keyField->names[b];
            size_t comp_index = t->get_comp_index_byName(*idf);
            CompField *cf2 = t->get_comp_byIndex(comp_index);
            presences->fields[b].nthfield = comp_index;
            presences->fields[b].nthfieldname = idf->get_name().c_str();
            if (t->typetype == T_CHOICE_T)
              presences->fields[b].fieldtype = UNION_FIELD;
            else if (cf2->get_is_optional())
              presences->fields[b].fieldtype = OPTIONAL_FIELD;
            else presences->fields[b].fieldtype = MANDATORY_FIELD;
            Type *field_type = cf2->get_type();
            presences->fields[b].type =
              pool.add(field_type->get_genname_value(my_scope));
            presences->fields[b].typedescr =
              pool.add(field_type->get_genname_typedescriptor(my_scope));
            t = field_type->get_type_refd_last();
          }
        }
        // building crosstaglist
        for(int c=0;c<rawpar->crosstaglist.nElements;c++){
          if(rawpar->crosstaglist.tag[c].nElements)
            sdef.elements[i].raw.crosstaglist.list[c].fields=
              (rawAST_coding_field_list*)
                 Malloc(rawpar->crosstaglist.tag[c].nElements
                   *sizeof(rawAST_coding_field_list));
          else sdef.elements[i].raw.crosstaglist.list[c].fields=NULL;
          sdef.elements[i].raw.crosstaglist.list[c].nElements=
            rawpar->crosstaglist.tag[c].nElements;
          sdef.elements[i].raw.crosstaglist.list[c].fieldName=
            rawpar->crosstaglist.tag[c].fieldName->get_name().c_str();
          Identifier *idf=rawpar->crosstaglist.tag[c].fieldName;
          sdef.elements[i].raw.crosstaglist.list[c].fieldnum=
            t_field_last->get_comp_index_byName(*idf);
          sdef.elements[i].raw.crosstaglist.list[c].fieldnum=
            t_field_last->get_comp_index_byName(*idf);
          for(int a=0;a<rawpar->crosstaglist.tag[c].nElements;a++) {
            rawAST_coding_field_list* key=
              sdef.elements[i].raw.crosstaglist.list[c].fields+a;
            key->nElements=
              rawpar->crosstaglist.tag[c].keyList[a].keyField->nElements;
            key->value=rawpar->crosstaglist.tag[c].keyList[a].value;
            key->fields=(rawAST_coding_fields*)
              Malloc(key->nElements*sizeof(rawAST_coding_fields));
            Type *t = this;
            for (int b = 0; b < key->nElements; b++) {
              Identifier *idf2 =
                rawpar->crosstaglist.tag[c].keyList[a].keyField->names[b];
              size_t comp_index = t->get_comp_index_byName(*idf2);
              CompField *cf2 = t->get_comp_byIndex(comp_index);
              key->fields[b].nthfield = comp_index;
              key->fields[b].nthfieldname = idf2->get_name().c_str();
              if (t->typetype == T_CHOICE_T)
                key->fields[b].fieldtype = UNION_FIELD;
              else if (cf2->get_is_optional())
                key->fields[b].fieldtype = OPTIONAL_FIELD;
              else key->fields[b].fieldtype = MANDATORY_FIELD;
              Type *field_type = cf2->get_type();
              key->fields[b].type =
                pool.add(field_type->get_genname_value(my_scope));
              key->fields[b].typedescr =
                pool.add(field_type->get_genname_typedescriptor(my_scope));
              t = field_type->get_type_refd_last();
            }
          }
        }
        sdef.elements[i].raw.length = t_field->get_raw_length();
        sdef.elements[i].hasRaw=TRUE;
      }
      else {
        sdef.elements[i].hasRaw=FALSE;
      }
    }
  }
  else {
    for(size_t i = 0; i < sdef.totalElements; i++) {
      sdef.elements[i].hasRaw=FALSE;
    }
    sdef.hasRaw=FALSE;
  }

  defRecordClass(&sdef, target);
  defRecordTemplate(&sdef, target);

  for(size_t i = 0; i < sdef.totalElements; i++) {
    // free the array but not the strings
    if (sdef.elements[i].xerAnyNum > 0) Free(sdef.elements[i].xerAnyUris);
  } // next i

  if (rawattrib) {
    free_raw_attrib_struct(&sdef.raw);
    for (size_t i = 0; i < sdef.totalElements; i++) {
      if (sdef.elements[i].hasRaw) {
        free_raw_attrib_struct(&sdef.elements[i].raw);
      }
    }
  }
  Free(sdef.elements);
}

bool Type::is_untagged() const { return xerattrib && xerattrib->untagged_; }

void Type::generate_code_SeOf(output_struct *target)
{
  const Type *oftypelast = u.seof.ofType->get_type_refd_last();
  const string& oftypename = u.seof.ofType->get_genname_value(my_scope);
  boolean optimized_memalloc = !use_runtime_2 && get_optimize_attribute() == "memalloc";
  
  if (is_pregenerated()) {
    switch(oftypelast->typetype) {
    case T_USTR:
    case T_UTF8STRING:
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
    case T_GRAPHICSTRING:
    case T_GENERALSTRING:
    case T_UNIVERSALSTRING:
    case T_BMPSTRING:
    case T_OBJECTDESCRIPTOR:
      target->header.class_decls = mputprintf(target->header.class_decls,
        "typedef PreGenRecordOf::PREGEN__%s__OF__UNIVERSAL__CHARSTRING%s %s;\n"
        "typedef PreGenRecordOf::PREGEN__%s__OF__UNIVERSAL__CHARSTRING%s_template %s_template;\n",
        (typetype == T_SEQOF) ? "RECORD" : "SET",
        optimized_memalloc ? "__OPTIMIZED" : "", get_genname_own().c_str(),
        (typetype == T_SEQOF) ? "RECORD" : "SET",
        optimized_memalloc ? "__OPTIMIZED" : "", get_genname_own().c_str());
      return;
    default:
      // generate these in the class declarations part, they need to be
      // outside of the include guard in case of circular imports
      target->header.class_decls = mputprintf(target->header.class_decls,
        "typedef PreGenRecordOf::PREGEN__%s__OF__%s%s %s;\n"
        "typedef PreGenRecordOf::PREGEN__%s__OF__%s%s_template %s_template;\n",
        (typetype == T_SEQOF) ? "RECORD" : "SET", oftypename.c_str(), 
        optimized_memalloc ? "__OPTIMIZED" : "", get_genname_own().c_str(),
        (typetype == T_SEQOF) ? "RECORD" : "SET", oftypename.c_str(),
        optimized_memalloc ? "__OPTIMIZED" : "",  get_genname_own().c_str());
      return;
    }
  }
  
  stringpool pool;
  struct_of_def sofdef;
  memset(&sofdef, 0, sizeof(sofdef));
  sofdef.name = get_genname_own().c_str();
  sofdef.dispname = get_fullname().c_str();
  sofdef.kind = typetype == T_SEQOF ? RECORD_OF : SET_OF;
  sofdef.isASN1 = is_asn1();
  sofdef.hasText = textattrib!=NULL;
  sofdef.hasXer = has_encoding(CT_XER);
  sofdef.hasJson = has_encoding(CT_JSON);
  if (xerattrib) {
    //sofdef.xerList      = xerattrib->list_;
    sofdef.xerAttribute = xerattrib->attribute_;
  }
  // If a record of UTF8String, we need to prepare for ANY-ATTRIBUTES and
  // ANY-ELEMENT
  sofdef.xerAnyAttrElem = oftypelast->typetype == T_USTR
  ||                      oftypelast->typetype == T_UTF8STRING;
  sofdef.type = oftypename.c_str();
  sofdef.has_opentypes = get_has_opentypes();
  const string& oftypedescrname =
    u.seof.ofType->get_genname_typedescriptor(my_scope);
  sofdef.oftypedescrname = oftypedescrname.c_str();

  if (xerattrib && xerattrib->untagged_
    && ((u.seof.ofType->xerattrib && has_ae(u.seof.ofType->xerattrib))
      ||               (xerattrib && has_ae(xerattrib)))) {
    // An untagged record-of which has an embedded type with ANY-ELEMENT,
    // or itself has ANY-ELEMENT
    if (parent_type && parent_type->typetype == T_SEQ_T) {
      /* The record-of needs to know the optional siblings following it,
       * to be able to stop consuming XML elements that belong
       * to the following fields. This is achieved by generating
       * a can_start() for the record-of which returns false for XML elements
       * that belong to those following fields. */
      size_t n_parent_comps = parent_type->get_nof_comps();
      boolean found_self = FALSE;
      /* Go through the fields of the parent; skip everything until we find
       * the field that is this record-of; then collect fields until
       * the first non-disappearing field. */
      for (size_t pc = 0; pc < n_parent_comps; ++pc) {
        CompField *pcf = parent_type->get_comp_byIndex(pc); //"ParentCompField"
        Type *pcft = pcf->get_type();
        if (found_self) {
          const Identifier& cfid = pcf->get_name();
          sofdef.followers = (struct_field*)Realloc(sofdef.followers,
            (++sofdef.nFollowers) * sizeof(struct_field));
          sofdef.followers[sofdef.nFollowers-1].name = pool.add(cfid.get_name());
          sofdef.followers[sofdef.nFollowers-1].type =
            pool.add(pcft->get_genname_value(my_scope));
          sofdef.followers[sofdef.nFollowers-1].typegen =
            pool.add(pcft->get_genname_own());

          Type *pcft_last = pcft->get_type_refd_last();
          if (pcf->get_is_optional()
            || (pcft->is_untagged() && pcft_last->has_empty_xml()))
          {} // can disappear, continue
          else break;
        }
        else if (pcft == this) found_self = TRUE;
      } // next field
    } // if parent is record
  } // if A-E

  switch (oftypelast->typetype) { // X.680/2002, Table 5 under 25.5
  // T_CHOICE_A and T_CHOICE_T do not set xmlValueList because choice types
  // already omit their own tag.
  case T_BOOL:
  case T_ENUM_A:   case T_ENUM_T:
  case T_NULL:
    sofdef.xmlValueList = TRUE;
    break;

  default:
    sofdef.xmlValueList = FALSE;
    break;
  }

  if(rawattrib) {
    copy_rawAST_to_struct(rawattrib,&(sofdef.raw));
    sofdef.hasRaw=TRUE;
  } else sofdef.hasRaw=FALSE;

  if (optimized_memalloc) {
    defRecordOfClassMemAllocOptimized(&sofdef, target);
  } else {
    defRecordOfClass(&sofdef, target);
  }
  defRecordOfTemplate(&sofdef, target);

  if (sofdef.nFollowers) {
    Free(sofdef.followers);
  }
}

void Type::generate_code_Array(output_struct *target)
{
  if (!u.array.in_typedef) return;
  const char *own_name = get_genname_own().c_str();
  if (has_encoding(CT_JSON)) {
    target->header.class_decls = mputprintf(target->header.class_decls,
      "class %s;\n", own_name);
    target->header.class_defs = mputprintf(target->header.class_defs,
      "class %s : public %s {\n"
      "const TTCN_Typedescriptor_t* get_elem_descr() const;\n"
      "};\n\n",
      own_name, 
      u.array.dimension->get_value_type(u.array.element_type, my_scope).c_str());
    target->source.methods = mputprintf(target->source.methods,
      "const TTCN_Typedescriptor_t* %s::get_elem_descr() const { return &%s_descr_; }\n\n",
      own_name, u.array.element_type->get_genname_typedescriptor(my_scope).c_str());
  } else {
    target->header.typedefs = mputprintf(target->header.typedefs,
#ifndef NDEBUG
      "// written by %s in " __FILE__ " at %d\n"
#endif
      "typedef %s %s;\n",
#ifndef NDEBUG
      __FUNCTION__, __LINE__,
#endif
      u.array.dimension->get_value_type(u.array.element_type, my_scope).c_str(),
      own_name);
  }
  target->header.typedefs = mputprintf(target->header.typedefs,
    "typedef %s %s_template;\n",
    u.array.dimension->get_template_type(u.array.element_type, my_scope).c_str(),
    own_name);
}

void Type::generate_code_Fat(output_struct *target)
{
  funcref_def fdef;
  memset(&fdef, 0, sizeof(fdef));
  fdef.name = get_genname_own().c_str();
  fdef.dispname = get_fullname().c_str();
  switch(typetype) {
  case T_FUNCTION:
    if(u.fatref.return_type)
      if(u.fatref.returns_template)
        fdef.return_type = mcopystr(u.fatref.return_type->
          get_genname_template(my_scope).c_str());
      else fdef.return_type = mcopystr(u.fatref.return_type->
          get_genname_value(my_scope).c_str());
    else fdef.return_type = NULL;
    fdef.type = FUNCTION;
    break;
  case T_ALTSTEP:
    fdef.return_type = NULL;
    fdef.type = ALTSTEP;
    break;
  case T_TESTCASE:
    fdef.return_type = NULL;
    fdef.type = TESTCASE;
    break;
  default:
    FATAL_ERROR("Type::generate_code_Fat()");
  }
  fdef.runs_on_self = u.fatref.runs_on.self ? TRUE : FALSE;
  fdef.is_startable = u.fatref.is_startable;
  fdef.formal_par_list = u.fatref.fp_list->generate_code(memptystr(),
    u.fatref.fp_list->get_nof_fps());
  u.fatref.fp_list->generate_code_defval(target);
  fdef.actual_par_list = u.fatref.fp_list
                          ->generate_code_actual_parlist(memptystr(),"");
  if (typetype == T_TESTCASE) {
    if (u.fatref.fp_list->get_nof_fps() > 0) {
      fdef.formal_par_list = mputstr(fdef.formal_par_list, ", ");
      fdef.actual_par_list = mputstr(fdef.actual_par_list, ", ");
    }
    fdef.formal_par_list = mputstr(fdef.formal_par_list,
      "boolean has_timer, double timer_value");
    fdef.actual_par_list = mputstr(fdef.actual_par_list,
      "has_timer, timer_value");
  }
  fdef.nElements = u.fatref.fp_list->get_nof_fps();
  fdef.parameters = (const char**)
                      Malloc(fdef.nElements * sizeof(*fdef.parameters));
  for(size_t i = 0;i < fdef.nElements; i++) {
    fdef.parameters[i] = u.fatref.fp_list->get_fp_byIndex(i)
                          ->get_id().get_name().c_str();
  }

  defFunctionrefClass(&fdef, target);
  defFunctionrefTemplate(&fdef, target);
  Free(fdef.return_type);
  Free(fdef.formal_par_list);
  Free(fdef.actual_par_list);
  Free(fdef.parameters);
}

void Type::generate_code_Signature(output_struct *target)
{
  stringpool pool;
  signature_def sdef;
  memset(&sdef, 0, sizeof(sdef));
  sdef.name = get_genname_own().c_str();
  sdef.dispname = get_fullname().c_str();
  if (u.signature.return_type) {
    sdef.return_type =
      pool.add(u.signature.return_type->get_genname_value(my_scope));
    sdef.return_type_w_no_prefix = pool.add(u.signature.return_type->get_genname_value(
      u.signature.return_type->get_type_refd_last()->get_my_scope()));
  }
  else sdef.return_type = NULL;
  if (u.signature.parameters) {
    sdef.parameters.nElements = u.signature.parameters->get_nof_params();
    sdef.parameters.elements = (signature_par*)
      Malloc(sdef.parameters.nElements * sizeof(*sdef.parameters.elements));
    for (size_t i = 0; i < sdef.parameters.nElements; i++) {
      SignatureParam *param = u.signature.parameters->get_param_byIndex(i);
      switch (param->get_direction()) {
      case SignatureParam::PARAM_IN:
        sdef.parameters.elements[i].direction = PAR_IN;
        break;
      case SignatureParam::PARAM_OUT:
        sdef.parameters.elements[i].direction = PAR_OUT;
        break;
      case SignatureParam::PARAM_INOUT:
        sdef.parameters.elements[i].direction = PAR_INOUT;
        break;
      default:
        FATAL_ERROR("Type::generate_code_Signature()");
      }
      sdef.parameters.elements[i].type =
        pool.add(param->get_type()->get_genname_value(my_scope));
      sdef.parameters.elements[i].name = param->get_id().get_name().c_str();
      sdef.parameters.elements[i].dispname =
        param->get_id().get_ttcnname().c_str();
    }
  } else {
    sdef.parameters.nElements = 0;
    sdef.parameters.elements = NULL;
  }
  sdef.is_noblock = u.signature.no_block;
  if (u.signature.exceptions) {
    sdef.exceptions.nElements = u.signature.exceptions->get_nof_types();
    sdef.exceptions.elements = (signature_exception*)
      Malloc(sdef.exceptions.nElements * sizeof(*sdef.exceptions.elements));
    for (size_t i = 0; i < sdef.exceptions.nElements; i++) {
      Type *type = u.signature.exceptions->get_type_byIndex(i);
      sdef.exceptions.elements[i].name =
        pool.add(type->get_genname_value(my_scope));
      sdef.exceptions.elements[i].dispname = pool.add(type->get_typename());
      sdef.exceptions.elements[i].altname = pool.add(type->get_genname_altname());
      sdef.exceptions.elements[i].name_w_no_prefix =
        pool.add(type->get_genname_value(type->get_type_refd_last()->get_my_scope()));
    }
  } else {
    sdef.exceptions.nElements = 0;
    sdef.exceptions.elements = NULL;
  }
  defSignatureClasses(&sdef, target);
  Free(sdef.parameters.elements);
  Free(sdef.exceptions.elements);
}

bool Type::needs_alias()
{
  /** The decision is actually based on the fullname of the type. If it
   * contains two or more dot (.) characters false is returned.
   * The following attributes cannot be used for the decision:
   * - parent_type, my_scope: types within ASN.1 object classes, objects
   * look the same as top-level aliased types, but they do not need alias. */
  const string& full_name = get_fullname();
  size_t fullname_len = full_name.size();
  size_t first_dot = full_name.find('.', 0);
  if (first_dot >= fullname_len) return TRUE; // no dots
  else if (full_name.find('.', first_dot + 1) < fullname_len) return FALSE;
  else return TRUE;
}


void Type::generate_code_done(output_struct *target)
{
  const string& t_genname = get_genname_value(my_scope);
  const char *genname_str = t_genname.c_str();
  const string& dispname = get_typename();
  const char *dispname_str = dispname.c_str();

  // the done function
  target->header.function_prototypes = mputprintf
    (target->header.function_prototypes,
     "extern alt_status done(const COMPONENT& component_reference, "
     "const %s_template& value_template, %s *value_redirect, Index_Redirect*);\n",
     genname_str, use_runtime_2 ? "Value_Redirect_Interface" : genname_str);
  target->source.function_bodies = mputprintf
    (target->source.function_bodies,
     "alt_status done(const COMPONENT& component_reference, "
     "const %s_template& value_template, %s *value_redirect, Index_Redirect*)\n"
     "{\n"
     "if (!component_reference.is_bound()) "
     "TTCN_error(\"Performing a done operation on an unbound component "
     "reference.\");\n"
     "if (value_template.get_selection() == ANY_OR_OMIT) "
     "TTCN_error(\"Done operation using '*' as matching template\");\n"
     "Text_Buf *text_buf;\n"
     "alt_status ret_val = TTCN_Runtime::component_done("
       "(component)component_reference, \"%s\", text_buf);\n"
     "if (ret_val == ALT_YES) {\n"
     "%s return_value;\n"
     "return_value.decode_text(*text_buf);\n"
     "if (value_template.match(return_value)) {\n"
     "if (value_redirect != NULL) {\n", genname_str,
     use_runtime_2 ? "Value_Redirect_Interface" : genname_str, dispname_str, genname_str);
  if (use_runtime_2) {
    target->source.function_bodies = mputstr(target->source.function_bodies,
      "value_redirect->set_values(&return_value);\n");
  }
  else {
    target->source.function_bodies = mputstr(target->source.function_bodies,
      "*value_redirect = return_value;\n");
  }
  target->source.function_bodies = mputprintf(target->source.function_bodies,
     "}\n"
     "TTCN_Logger::begin_event(TTCN_Logger::PARALLEL_PTC);\n"
     "TTCN_Logger::log_event_str(\"PTC with component reference \");\n"
     "component_reference.log();\n"
     "TTCN_Logger::log_event_str(\" is done. Return value: %s : \");\n"
     "return_value.log();\n"
     "TTCN_Logger::end_event();\n"
     "return ALT_YES;\n"
     "} else {\n"
     "if (TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_DONE)) {\n"
     "TTCN_Logger::begin_event(TTCN_Logger::MATCHING_DONE);\n"
     "TTCN_Logger::log_event_str(\"Done operation with type %s on"
     " component reference \");\n"
     "component_reference.log();\n"
     "TTCN_Logger::log_event_str(\" failed: Return value does not match "
     "the template: \");\n"
     "value_template.log_match(return_value%s);\n"
     "TTCN_Logger::end_event();\n"
     "}\n"
     "return ALT_NO;\n"
     "}\n"
     "} else return ret_val;\n"
     "}\n\n",
     dispname_str, dispname_str, omit_in_value_list ? ", TRUE" : "");
  if (needs_any_from_done) {
    // 'done' function for component arrays
    // this needs to be generated in the header, because it has template parameters
    target->header.function_prototypes = mputprintf(
      target->header.function_prototypes,
      "\ntemplate <typename T_type, unsigned int array_size, int index_offset>\n"
      "alt_status done(const VALUE_ARRAY<T_type, array_size, index_offset>& "
      "component_array, const %s_template& value_template, "
      "%s* value_redirect, Index_Redirect* index_redirect)\n"
      "{\n"
      "if (index_redirect != NULL) {\n"
      "index_redirect->incr_pos();\n"
      "}\n"
      "alt_status result = ALT_NO;\n"
      "for (unsigned int i = 0; i < array_size; ++i) {\n"
      "alt_status ret_val = done(component_array[i], value_template, value_redirect, "
      "index_redirect);\n"
      "if (ret_val == ALT_YES) {\n"
      "if (index_redirect != NULL) {\n"
      "index_redirect->add_index((int)i + index_offset);\n"
      "}\n"
      "result = ret_val;\n"
      "break;\n"
      "}\n"
      "else if (ret_val == ALT_REPEAT || (ret_val == ALT_MAYBE && result == ALT_NO)) {\n"
      "result = ret_val;\n"
      "}\n"
      "}\n"
      "if (index_redirect != NULL) {\n"
      "index_redirect->decr_pos();\n"
      "}\n"
      "return result;\n"
      "}\n\n",
      genname_str, use_runtime_2 ? "Value_Redirect_Interface" : genname_str);
  }
}

bool Type::ispresent_anyvalue_embedded_field(Type* t,
  Ttcn::FieldOrArrayRefs *subrefs, size_t begin_index)
{
  if (!subrefs) return TRUE;
  size_t nof_refs = subrefs->get_nof_refs();
  for (size_t i = begin_index; i < nof_refs; i++) {
    t = t->get_type_refd_last();
    Ttcn::FieldOrArrayRef *ref = subrefs->get_ref(i);
    switch (ref->get_type()) {
    case Ttcn::FieldOrArrayRef::FIELD_REF: {
      CompField* cf = t->get_comp_byName(*ref->get_id());
      switch (t->typetype) {
      case T_CHOICE_T:
      case T_CHOICE_A:
      case T_OPENTYPE:
      case T_ANYTYPE:
        return FALSE;
      case T_SEQ_T:
      case T_SET_T:
      case T_SEQ_A:
      case T_SET_A:
        if (cf->get_is_optional()) return FALSE;
        break;
      default:
        FATAL_ERROR("Type::ispresent_anyvalue_embedded_field()");
      }
      t = cf->get_type();
    } break;
    case Ttcn::FieldOrArrayRef::ARRAY_REF:
      switch (t->typetype) {
      case T_SEQOF:
      case T_SETOF:
        return FALSE; // (the existence of a record of element is optional)
	    case T_ARRAY:
	      t = t->u.array.element_type;
        break;
      default:
        return TRUE; // string types
      }
      break;
    default:
      FATAL_ERROR("Type::ispresent_anyvalue_embedded_field()");
    }
  }
  return TRUE;
}

void Type::generate_code_ispresentbound(expression_struct *expr,
    Ttcn::FieldOrArrayRefs *subrefs, Common::Module* module,
    const string& global_id, const string& external_id, const bool is_template,
    const bool isbound)
{
  if (!subrefs) return;

  Type *t = this;
  Type *next_t;
  bool next_o; // next is optional value
  size_t nof_refs = subrefs->get_nof_refs();
  subrefs->clear_string_element_ref();
  char *tmp_generalid_str = mcopystr(external_id.c_str());
  expstring_t closing_brackets = memptystr(); //The closing parts collected
  for (size_t i = 0; i < nof_refs; i++) {
    t = t->get_type_refd_last();
    // stop immediately if current type t is erroneous
    // (e.g. because of circular reference)
    if (t->typetype == T_ERROR) return;

    if (is_template) {
      bool anyval_ret_val = TRUE;
      if (!isbound) {
        anyval_ret_val = ispresent_anyvalue_embedded_field(t, subrefs, i);
      }
      expr->expr = mputprintf(expr->expr, "if(%s) {\n",global_id.c_str());
      expr->expr = mputprintf(expr->expr,
        "switch (%s.get_selection()) {\n"
        "case UNINITIALIZED_TEMPLATE:\n"
        "%s = FALSE;\n"
        "break;\n"
        "case ANY_VALUE:\n"
        "%s = %s;\n"
        "break;\n"
        "case SPECIFIC_VALUE: {\n",
        tmp_generalid_str, global_id.c_str(), global_id.c_str(),
        anyval_ret_val ? "TRUE" : "FALSE");

      expstring_t closing_brackets_switch = mprintf(
        "break;}\n"
        "default:\n"
        "%s = FALSE;\n"
        "break;\n"
        "}\n"
        "}\n"
        "%s",
        global_id.c_str(), closing_brackets);
      Free(closing_brackets);
      closing_brackets = closing_brackets_switch;
    }

    Ttcn::FieldOrArrayRef *ref = subrefs->get_ref(i);
    switch (ref->get_type()) {
    case Ttcn::FieldOrArrayRef::FIELD_REF: {
      const Identifier& id = *ref->get_id();
      CompField* cf = t->get_comp_byName(id);
      next_t = cf->get_type();
      next_o = !is_template && cf->get_is_optional();

      switch (t->typetype) {
      case T_CHOICE_A:
      case T_CHOICE_T:
      case T_OPENTYPE:
        expr->expr = mputprintf(expr->expr, "if(%s) {\n",global_id.c_str());
        expr->expr = mputprintf(expr->expr,
          "%s = %s.ischosen(%s::ALT_%s);\n", global_id.c_str(),
          tmp_generalid_str,
          t->get_genname_value(module).c_str(),
          id.get_name().c_str());
        expr->expr = mputstr(expr->expr, "}\n");
      // intentionally missing break
      case T_SEQ_A:
      case T_SEQ_T:
      case T_SET_A:
      case T_SET_T:
      case T_ANYTYPE:
        break;
      default:
        FATAL_ERROR("Type::generate_code_isbound()");
      }

      if (next_o) {
        expr->expr = mputprintf(expr->expr, "if(%s) {\n",global_id.c_str());
        expstring_t closing_brackets2 = mprintf("}\n%s", closing_brackets);
        Free(closing_brackets);
        closing_brackets = closing_brackets2;

        const string& tmp_id = module->get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        expr->expr = mputprintf(expr->expr,
          "const OPTIONAL<%s%s>& %s = %s.%s();\n",
          next_t->get_genname_value(module).c_str(),
          is_template?"_template":"", tmp_id_str, tmp_generalid_str,
          id.get_name().c_str());

        if (i==(nof_refs-1)) {
          // we are at the end of the reference chain
          expr->expr = mputprintf(expr->expr,
            "switch (%s.get_selection()) {\n"
            "case OPTIONAL_UNBOUND:\n"
            "%s = FALSE;\n"
            "break;\n"
            "case OPTIONAL_OMIT:\n"
            "%s = %s;\n"
            "break;\n"
            "default:\n",
            tmp_id_str, global_id.c_str(),global_id.c_str(),
            isbound ? "TRUE" : "FALSE");
          Free(tmp_generalid_str);
          tmp_generalid_str = mcopystr(tmp_id_str);

          expr->expr = mputstr(expr->expr, "{\n");
          const string& tmp_id2 = module->get_temporary_id();
          const char *tmp_id2_str = tmp_id2.c_str();
          expr->expr = mputprintf(expr->expr,
            "const %s%s& %s = (const %s%s&) %s;\n",
            next_t->get_genname_value(module).c_str(),
            is_template?"_template":"", tmp_id2_str,
            next_t->get_genname_value(module).c_str(),
            is_template?"_template":"", tmp_id_str);

          expr->expr = mputprintf(expr->expr,
            "%s = %s.%s(%s);\n", global_id.c_str(),
            tmp_id2_str, isbound ? "is_bound" : "is_present",
            (!isbound && is_template && omit_in_value_list) ? "TRUE" : "");
          Free(tmp_generalid_str);
          tmp_generalid_str = mcopystr(tmp_id2_str);

          expr->expr = mputprintf(expr->expr,
              "break;}\n"
            "}\n");
        } else {
          expr->expr = mputprintf(expr->expr,
            "switch (%s.get_selection()) {\n"
            "case OPTIONAL_UNBOUND:\n"
            "case OPTIONAL_OMIT:\n"
            "%s = FALSE;\n"
            "break;\n"
            "default:\n"
            "break;\n"
            "}\n",
            tmp_id_str, global_id.c_str());
          Free(tmp_generalid_str);
          tmp_generalid_str = mcopystr(tmp_id_str);

          expr->expr = mputprintf(expr->expr, "if(%s) {\n",global_id.c_str());
          closing_brackets2 = mprintf("}\n%s", closing_brackets);
          Free(closing_brackets);
          closing_brackets = closing_brackets2;

          const string& tmp_id2 = module->get_temporary_id();
          const char *tmp_id2_str = tmp_id2.c_str();
          expr->expr = mputprintf(expr->expr,
            "const %s%s& %s = (const %s%s&) %s;\n",
            next_t->get_genname_value(module).c_str(),
            is_template?"_template":"", tmp_id2_str,
            next_t->get_genname_value(module).c_str(),
            is_template?"_template":"", tmp_id_str);

          expr->expr = mputprintf(expr->expr,
            "%s = %s.is_bound();\n", global_id.c_str(),
            tmp_id2_str);
          Free(tmp_generalid_str);
          tmp_generalid_str = mcopystr(tmp_id2_str);
        }
      } else {
        expr->expr = mputprintf(expr->expr, "if(%s) {\n",global_id.c_str());
        expstring_t closing_brackets2 = mprintf("}\n%s", closing_brackets);
        Free(closing_brackets);
        closing_brackets = closing_brackets2;
      
        const string& tmp_id = module->get_temporary_id();
        const string& tmp_id2 = module->get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        const char *tmp_id2_str = tmp_id2.c_str();
        
        // Own const ref to the temp value
        expr->expr = mputprintf(expr->expr,
          "const %s%s& %s = %s;\n",
          t->get_genname_value(module).c_str(),
          is_template ? "_template" : "",
          tmp_id_str, tmp_generalid_str);
        // Get the const ref of the field from the previous const ref
        // If we would get the const ref of the field immediately then the
        // value in the const ref would be free-d instantly.
        expr->expr = mputprintf(expr->expr,
          "const %s%s& %s = %s.%s%s();\n",
          next_t->get_genname_value(module).c_str(),
          is_template ? "_template" : "",
          tmp_id2_str, tmp_id_str,
          t->typetype == T_ANYTYPE ? "AT_" : "", id.get_name().c_str());

        expr->expr = mputprintf(expr->expr,
          "%s = %s.%s(%s);\n", global_id.c_str(),
          tmp_id2_str, isbound||(i!=(nof_refs-1)) ? "is_bound" : "is_present",
          (!(isbound||(i!=(nof_refs-1))) && is_template && omit_in_value_list) ? "TRUE" : "");
        Free(tmp_generalid_str);
        tmp_generalid_str = mcopystr(tmp_id2_str);
      }

      t = next_t;
      break; }
    case Ttcn::FieldOrArrayRef::ARRAY_REF: {
      Type *embedded_type = 0;
      bool is_string = TRUE;
      bool is_string_element = FALSE;
      switch (t->typetype) {
      case T_SEQOF:
      case T_SETOF:
        embedded_type = t->u.seof.ofType;
        is_string = FALSE;
        break;
      case T_ARRAY:
        embedded_type = t->u.array.element_type;
        is_string = FALSE;
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
          FATAL_ERROR("Type::generate_code_isbound()");
        } else {
          subrefs->set_string_element_ref();
          // string elements have the same type as the string itself
          embedded_type = t;
          is_string_element = TRUE;
          break;
        }
      default:
         FATAL_ERROR("Type::generate_code_isbound()");
      }

      next_t = embedded_type;

      // check the index value
      Value *index_value = ref->get_val();
      Value *v_last = index_value->get_value_refd_last();

      const string& tmp_index_id = module->get_temporary_id();
      const char *tmp_index_id_str = tmp_index_id.c_str();
      expr->expr = mputprintf(expr->expr, "if(%s) {\n",global_id.c_str());

      expstring_t closing_brackets2 = mprintf("}\n%s", closing_brackets);
      Free(closing_brackets);
      closing_brackets = closing_brackets2;
      
      expr->expr = mputprintf(expr->expr, "const int %s = ", tmp_index_id_str);
      v_last->generate_code_expr_mandatory(expr);
      expr->expr = mputstr(expr->expr, ";\n");
      expr->expr = mputprintf(expr->expr, "%s = (%s >= 0) && (%s.%s > %s);\n",
          global_id.c_str(), tmp_index_id_str, tmp_generalid_str,
          is_string ? "lengthof()": ( is_template ? "n_elem()" : "size_of()" ),
          tmp_index_id_str);
      expr->expr = mputprintf(expr->expr, "if(%s) {\n",global_id.c_str());

      closing_brackets2 = mprintf("}\n%s", closing_brackets);
      Free(closing_brackets);
      closing_brackets = closing_brackets2;

      const string& tmp_id = module->get_temporary_id();
      const char *tmp_id_str = tmp_id.c_str();

      if (is_string_element) {
        expr->expr = mputprintf(expr->expr,
          "%s = %s[%s].%s(%s);\n", global_id.c_str(),
          tmp_generalid_str, tmp_index_id_str,
          isbound||(i!=(nof_refs-1)) ? "is_bound" : "is_present",
          (!(isbound||(i!=(nof_refs-1))) && is_template && omit_in_value_list) ? "TRUE" : "");
      } else {
        if (is_template) {
            expr->expr = mputprintf(expr->expr,
            "const %s& %s = %s[%s];\n",
            next_t->get_genname_template(module).c_str(),
            tmp_id_str, tmp_generalid_str,
            tmp_index_id_str);
        } else {
            expr->expr = mputprintf(expr->expr,
            "const %s%s& %s = %s[%s];\n",
            next_t->get_genname_value(module).c_str(),
            is_template?"_template":"", tmp_id_str, tmp_generalid_str,
            tmp_index_id_str);
        }
        
        expr->expr = mputprintf(expr->expr,
          "%s = %s.%s(%s);\n", global_id.c_str(), tmp_id_str, 
          isbound||(i!=(nof_refs-1)) ? "is_bound" : "is_present",
          (!(isbound||(i!=(nof_refs-1))) && is_template && omit_in_value_list) ? "TRUE" : "");
      }

      Free(tmp_generalid_str);
      tmp_generalid_str = mcopystr(tmp_id_str);

      // change t to the embedded type
      t = next_t;
      break; }
      default:
        FATAL_ERROR("Type::generate_code_isbound(): invalid reference type");
      }
    }

    Free(tmp_generalid_str);
    expr->expr = mputstr(expr->expr, closing_brackets);
    Free(closing_brackets);
}

string Type::get_optimize_attribute()
{
  if(w_attrib_path)
  {
    vector<SingleWithAttrib> const &real_attribs
      = w_attrib_path->get_real_attrib();
    for (size_t i = 0; i < real_attribs.size(); i++) {
      SingleWithAttrib * temp_single = real_attribs[i];
      if (temp_single->get_attribKeyword()
            == SingleWithAttrib::AT_EXTENSION
         && (!temp_single->get_attribQualifiers()
            || (temp_single->get_attribQualifiers()
                  ->get_nof_qualifiers() == 0)))
      {
        const string& spec = temp_single->get_attribSpec().get_spec();
        // TODO: use a real parser to allow whitespaces, etc.
        if (spec.find("optimize:")==0 && spec.size()>9)
        {
          string spec_optimize_for_what = spec.substr(9);
          return spec_optimize_for_what;
        }
      }
    }
  }
  return string();
}

string Type::get_sourcefile_attribute()
{
  if(w_attrib_path)
  {
    vector<SingleWithAttrib> const &real_attribs
      = w_attrib_path->get_real_attrib();

    for (size_t i = 0; i < real_attribs.size(); i++) {
      SingleWithAttrib * temp_single = real_attribs[i];
      if (temp_single->get_attribKeyword()
            == SingleWithAttrib::AT_EXTENSION
         && (!temp_single->get_attribQualifiers()
            || (temp_single->get_attribQualifiers()
                  ->get_nof_qualifiers() == 0)))
      {
        const string& spec = temp_single->get_attribSpec().get_spec();
        if (spec.find("sourcefile:")==0 && spec.size()>11)
        {
          string spec_filename = spec.substr(11);
          // TODO: check if string can be a valid filename
          return spec_filename;
        }
      }
    }
  }
  return string();
}

bool Type::has_done_attribute()
{
  if(w_attrib_path)
  {
    vector<SingleWithAttrib> const &real_attribs
      = w_attrib_path->get_real_attrib();

    for (size_t i = 0; i < real_attribs.size(); i++) {
      SingleWithAttrib * temp_single = real_attribs[i];
      if (temp_single->get_attribKeyword()
            == SingleWithAttrib::AT_EXTENSION
         && (!temp_single->get_attribQualifiers()
            || (temp_single->get_attribQualifiers()
                  ->get_nof_qualifiers() == 0))
         && temp_single->get_attribSpec().get_spec() == "done")
      {
            return TRUE;
      }
    }
  }
  return FALSE;
}

void Type::generate_code_object(const_def *cdef, Scope *p_scope,
  const string& name, const char *prefix, bool is_template, bool has_err_descr)
{
  string type_name;
  if (is_template) type_name = get_genname_template(p_scope);
  else type_name = get_genname_value(p_scope);
  const char *name_str = name.c_str();
  const char *type_name_str = type_name.c_str();
  if (prefix) {
    cdef->decl = mputprintf(cdef->decl, "extern const %s& %s;\n",
      type_name_str, name_str);
    if (split_to_slices || has_err_descr) {
      cdef->decl = mputprintf(cdef->decl, "extern %s %s%s;\n", type_name_str, prefix, name_str);
    }
    cdef->def = mputprintf(cdef->def, "%s%s %s%s;\n"
      "const %s& %s = %s%s;\n", split_to_slices || has_err_descr ? "" : "static ",
      type_name_str, prefix, name_str, type_name_str, name_str, prefix, name_str);
  } else {
    cdef->decl = mputprintf(cdef->decl, "extern %s %s;\n",
      type_name_str, name_str);
    cdef->def = mputprintf(cdef->def, "%s %s;\n",
      type_name_str, name_str);
  }
}

void Type::generate_code_object(const_def *cdef, GovernedSimple *p_setting)
{
  bool is_template = FALSE;
  switch (p_setting->get_st()) {
  case S_TEMPLATE:
    is_template = TRUE;
    break;
  case S_V:
    break;
  default:
    FATAL_ERROR("Type::generate_code_object()");
  }
  if (p_setting->get_err_descr() != NULL &&
      p_setting->get_err_descr()->has_descr(NULL)) {
    cdef->def = p_setting->get_err_descr()->generate_code_str(NULL, cdef->def,
      cdef->decl, p_setting->get_genname_prefix() + p_setting->get_genname_own());
  }
  // allways pass 'true' to the 'has_err_descr' parameter while in Runtime2, not
  // just if the value/template has erroneous descriptors, so adding an '@update'
  // statement in a different module does not require this module's code to be
  // regenerated
  generate_code_object(cdef, p_setting->get_my_scope(),
    p_setting->get_genname_own(), p_setting->get_genname_prefix(),
    is_template, use_runtime_2);
}

void Type::generate_json_schema(JSON_Tokenizer& json, bool embedded, bool as_value)
{
  // add a new property for the type if it has its own definition
  if (!embedded) {
    json.put_next_token(JSON_TOKEN_NAME, get_dispname().c_str());
  }
  
  // create an object containing the type's schema
  json.put_next_token(JSON_TOKEN_OBJECT_START);
  
  // if this is a field of a record/set/union with an alias, the field's
  // original name must be stored ("originalName" property), it also needs to be
  // stored if this is a field of a union with the "as value" coding instruction
  if (ownertype == OT_COMP_FIELD) {
    CompField* cf = static_cast<CompField*>(owner);
    if (as_value || (cf->get_type()->jsonattrib != NULL
        && cf->get_type()->jsonattrib->alias != NULL)) {
      json.put_next_token(JSON_TOKEN_NAME, "originalName");
      char* field_str = mprintf("\"%s\"", cf->get_name().get_ttcnname().c_str());
      json.put_next_token(JSON_TOKEN_STRING, field_str);
      Free(field_str);
    }
    
    // if the parent is a union with the "as value" coding instruction AND the field
    // has an alias, then the alias needs to be stored as well ("unusedAlias" property)
    if (as_value && cf->get_type()->jsonattrib != NULL
        && cf->get_type()->jsonattrib->alias != NULL) {
      json.put_next_token(JSON_TOKEN_NAME, "unusedAlias");
      char* alias_str = mprintf("\"%s\"", cf->get_type()->jsonattrib->alias);
      json.put_next_token(JSON_TOKEN_STRING, alias_str);
      Free(alias_str);
    }
  }

  // get the type at the end of the reference chain
  Type* last = get_type_refd_last();
  
  // check if this is a reference to another type that has its own definition
  Type* refd_type = NULL;
  if (is_ref()) {
    Type* iter = this;
    while (iter->is_ref()) {
      iter = iter->get_type_refd();
      if (iter->ownertype == OT_TYPE_DEF || /* TTCN-3 type definition */
          iter->ownertype == OT_TYPE_ASS) { /* ASN.1 type assignment */
        refd_type = iter;
        break;
      }
    }
  }
  
  // check if there are any type restrictions
  boolean has_restrictions = sub_type != NULL && sub_type->has_json_schema();

  // if it's a referenced type, then its schema already exists, only add a pointer to it
  // exception: instances of ASN.1 parameterized types, always embed their schemas
  if (refd_type != NULL && !get_type_refd()->pard_type_instance) {
    if (has_restrictions) {
      // an 'allOf' structure is needed if this is a subtype,
      // insert the pointer in the first part
      json.put_next_token(JSON_TOKEN_NAME, "allOf");
      json.put_next_token(JSON_TOKEN_ARRAY_START);
      json.put_next_token(JSON_TOKEN_OBJECT_START);
    }
    json.put_next_token(JSON_TOKEN_NAME, "$ref");
    char* ref_str = mprintf("\"#/definitions/%s/%s\"",
      refd_type->my_scope->get_scope_mod()->get_modid().get_ttcnname().c_str(),
      refd_type->get_dispname().c_str());
    json.put_next_token(JSON_TOKEN_STRING, ref_str);
    Free(ref_str);
    if (has_restrictions) {
      // close the first part of the 'allOf' and insert the type restrictions
      // in the second part
      json.put_next_token(JSON_TOKEN_OBJECT_END);
      json.put_next_token(JSON_TOKEN_OBJECT_START);
      
      // pass the tokenizer to the subtype to insert the type restrictions' schema
      sub_type->generate_json_schema(json);
      
      // close the second part and the 'allOf' structure itself
      json.put_next_token(JSON_TOKEN_OBJECT_END);
      json.put_next_token(JSON_TOKEN_ARRAY_END);
    }
  } else {
    // generate the schema for the referenced type
    switch (last->typetype) {
    case T_BOOL:
      // use the JSON boolean type
      json.put_next_token(JSON_TOKEN_NAME, "type");
      json.put_next_token(JSON_TOKEN_STRING, "\"boolean\"");
      break;
    case T_INT:
    case T_INT_A:
      // use the JSON integer type
      json.put_next_token(JSON_TOKEN_NAME, "type");
      json.put_next_token(JSON_TOKEN_STRING, "\"integer\"");
      break;
    case T_REAL:
      if (has_restrictions) {
        // adding restrictions after the type's schema wouldn't work here
        // if the restrictions affect the special values
        // use a special function that generates the schema segment for both
        // the float type and its restrictions
        sub_type->generate_json_schema_float(json);
        has_restrictions = FALSE; // so they aren't generated twice
      }
      else {
        // any of: JSON number or the special values as strings (in an enum)
        json.put_next_token(JSON_TOKEN_NAME, "anyOf");
        json.put_next_token(JSON_TOKEN_ARRAY_START);
        json.put_next_token(JSON_TOKEN_OBJECT_START);
        json.put_next_token(JSON_TOKEN_NAME, "type");
        json.put_next_token(JSON_TOKEN_STRING, "\"number\"");
        json.put_next_token(JSON_TOKEN_OBJECT_END);
        json.put_next_token(JSON_TOKEN_OBJECT_START);
        json.put_next_token(JSON_TOKEN_NAME, "enum");
        json.put_next_token(JSON_TOKEN_ARRAY_START);
        json.put_next_token(JSON_TOKEN_STRING, "\"not_a_number\"");
        json.put_next_token(JSON_TOKEN_STRING, "\"infinity\"");
        json.put_next_token(JSON_TOKEN_STRING, "\"-infinity\"");
        json.put_next_token(JSON_TOKEN_ARRAY_END);
        json.put_next_token(JSON_TOKEN_OBJECT_END);
        json.put_next_token(JSON_TOKEN_ARRAY_END);
      }
      break;
    case T_BSTR:
    case T_BSTR_A:
    case T_HSTR:
    case T_OSTR:
    case T_ANY:
      // use the JSON string type and add a pattern to only allow bits or hex digits
      json.put_next_token(JSON_TOKEN_NAME, "type");
      json.put_next_token(JSON_TOKEN_STRING, "\"string\"");
      json.put_next_token(JSON_TOKEN_NAME, "subType");
      json.put_next_token(JSON_TOKEN_STRING, 
        (last->typetype == T_OSTR || last->typetype == T_ANY) ? "\"octetstring\"" :
        ((last->typetype == T_HSTR) ? "\"hexstring\"" : "\"bitstring\""));
      json.put_next_token(JSON_TOKEN_NAME, "pattern");
      json.put_next_token(JSON_TOKEN_STRING, 
        (last->typetype == T_OSTR || last->typetype == T_ANY) ? "\"^([0-9A-Fa-f][0-9A-Fa-f])*$\"" :
        ((last->typetype == T_HSTR) ? "\"^[0-9A-Fa-f]*$\"" : "\"^[01]*$\""));
      break;
    case T_CSTR:
    case T_NUMERICSTRING:
    case T_PRINTABLESTRING:
    case T_IA5STRING:
    case T_VISIBLESTRING:
      // use the JSON string type and add a "subType" property to distinguish it from
      // universal charstring types
      json.put_next_token(JSON_TOKEN_NAME, "type");
      json.put_next_token(JSON_TOKEN_STRING, "\"string\"");
      json.put_next_token(JSON_TOKEN_NAME, "subType");
      json.put_next_token(JSON_TOKEN_STRING, "\"charstring\"");
      break;
    case T_USTR:
    case T_GENERALSTRING:
    case T_UNIVERSALSTRING:
    case T_UTF8STRING:
    case T_BMPSTRING:
    case T_GRAPHICSTRING:
    case T_TELETEXSTRING:
    case T_VIDEOTEXSTRING:
      // use the JSON string type and add a "subType" property to distinguish it from
      // charstring types
      json.put_next_token(JSON_TOKEN_NAME, "type");
      json.put_next_token(JSON_TOKEN_STRING, "\"string\"");
      json.put_next_token(JSON_TOKEN_NAME, "subType");
      json.put_next_token(JSON_TOKEN_STRING, "\"universal charstring\"");
      break;
    case T_OID:
    case T_ROID:
      json.put_next_token(JSON_TOKEN_NAME, "type");
      json.put_next_token(JSON_TOKEN_STRING, "\"string\"");
      json.put_next_token(JSON_TOKEN_NAME, "subType");
      json.put_next_token(JSON_TOKEN_STRING, "\"objid\"");
      json.put_next_token(JSON_TOKEN_NAME, "pattern");
      json.put_next_token(JSON_TOKEN_STRING, "\"^[0-2][.][1-3]?[0-9]([.][0-9]|([1-9][0-9]+))*$\"");
      break;
    case T_VERDICT:
      if (has_restrictions) {
        // the restrictions would only add another JSON enum (after the one
        /// generated below), instead just insert the one with the restrictions
        sub_type->generate_json_schema(json);
        has_restrictions = FALSE; // so they aren't generated twice
      }
      else {
        // enumerate the possible values
        json.put_next_token(JSON_TOKEN_NAME, "enum");
        json.put_next_token(JSON_TOKEN_ARRAY_START);
        json.put_next_token(JSON_TOKEN_STRING, "\"none\"");
        json.put_next_token(JSON_TOKEN_STRING, "\"pass\"");
        json.put_next_token(JSON_TOKEN_STRING, "\"inconc\"");
        json.put_next_token(JSON_TOKEN_STRING, "\"fail\"");
        json.put_next_token(JSON_TOKEN_STRING, "\"error\"");
        json.put_next_token(JSON_TOKEN_ARRAY_END);
      }
      break;
    case T_ENUM_T:
    case T_ENUM_A:
      // enumerate the possible values
      json.put_next_token(JSON_TOKEN_NAME, "enum");
      json.put_next_token(JSON_TOKEN_ARRAY_START);
      for (size_t i = 0; i < last->u.enums.eis->get_nof_eis(); ++i) {
        char* enum_str = mprintf("\"%s\"", last->get_ei_byIndex(i)->get_name().get_ttcnname().c_str());
        json.put_next_token(JSON_TOKEN_STRING, enum_str);
        Free(enum_str);
      }
      json.put_next_token(JSON_TOKEN_ARRAY_END);
      // list the numeric values for the enumerated items
      json.put_next_token(JSON_TOKEN_NAME, "numericValues");
      json.put_next_token(JSON_TOKEN_ARRAY_START);
      for (size_t i = 0; i < last->u.enums.eis->get_nof_eis(); ++i) {
        char* num_val_str = mprintf("%lli", last->get_ei_byIndex(i)->get_int_val()->get_val());
        json.put_next_token(JSON_TOKEN_NUMBER, num_val_str);
        Free(num_val_str);
      }
      json.put_next_token(JSON_TOKEN_ARRAY_END);
      break;
    case T_NULL:
      // use the JSON null value for the ASN.1 NULL type
      json.put_next_token(JSON_TOKEN_NAME, "type");
      json.put_next_token(JSON_TOKEN_STRING, "\"null\"");
      break;
    case T_SEQOF:
    case T_SETOF:
    case T_ARRAY:
      last->generate_json_schema_array(json);
      break;
    case T_SEQ_T:
    case T_SEQ_A:
    case T_SET_T:
    case T_SET_A:
      last->generate_json_schema_record(json);
      break;
    case T_CHOICE_T:
    case T_CHOICE_A:
    case T_ANYTYPE:
    case T_OPENTYPE:
      last->generate_json_schema_union(json);
      break;
    default:
      FATAL_ERROR("Type::generate_json_schema");
    }
    
    if (has_restrictions) {
      // pass the tokenizer to the subtype to insert the type restrictions' schema
      sub_type->generate_json_schema(json);
    }
  }
  
  // insert default value (if any)
  if (jsonattrib != NULL && jsonattrib->default_value != NULL) {
    json.put_next_token(JSON_TOKEN_NAME, "default");
    switch (last->typetype) {
    case T_BOOL:
      json.put_next_token((jsonattrib->default_value[0] == 't') ?
        JSON_TOKEN_LITERAL_TRUE : JSON_TOKEN_LITERAL_FALSE);
      break;
    case T_INT:
    case T_REAL:
      if (jsonattrib->default_value[0] != 'n' && jsonattrib->default_value[0] != 'i'
          && jsonattrib->default_value[1] != 'i') {
        json.put_next_token(JSON_TOKEN_NUMBER, jsonattrib->default_value);
        break;
      }
      // no break, insert the special float values as strings
    case T_BSTR:
    case T_HSTR:
    case T_OSTR:
    case T_CSTR:
    case T_USTR:
    case T_VERDICT:
    case T_ENUM_T: {
      char* default_str = mprintf("\"%s\"", jsonattrib->default_value);
      json.put_next_token(JSON_TOKEN_STRING, default_str);
      Free(default_str);
      break; }
    default:
      FATAL_ERROR("Type::generate_json_schema");
    }
  }
  
  // insert schema extensions (if any)
  if (jsonattrib != NULL) {
    for (size_t i = 0; i < jsonattrib->schema_extensions.size(); ++i) {
      json.put_next_token(JSON_TOKEN_NAME, jsonattrib->schema_extensions[i]->key);
      char* value_str = mprintf("\"%s\"", jsonattrib->schema_extensions[i]->value);
      json.put_next_token(JSON_TOKEN_STRING, value_str);
      Free(value_str);
    }
  }

  // end of type's schema
  json.put_next_token(JSON_TOKEN_OBJECT_END);
}

void Type::generate_json_schema_array(JSON_Tokenizer& json)
{
  // use the JSON array type
  json.put_next_token(JSON_TOKEN_NAME, "type");
  json.put_next_token(JSON_TOKEN_STRING, "\"array\"");
  
  if (typetype != T_ARRAY) {
    // use the "subType" property to distinguish 'record of' from 'set of'
    json.put_next_token(JSON_TOKEN_NAME, "subType");
    json.put_next_token(JSON_TOKEN_STRING, (typetype == T_SEQOF) ?
      "\"record of\"" : "\"set of\"");
  } else {
    // set the number of elements for arrays
    char* size_str = mprintf("%lu", (unsigned long)(get_nof_comps()));
    json.put_next_token(JSON_TOKEN_NAME, "minItems");
    json.put_next_token(JSON_TOKEN_NUMBER, size_str);
    json.put_next_token(JSON_TOKEN_NAME, "maxItems");
    json.put_next_token(JSON_TOKEN_NUMBER, size_str);
    Free(size_str);
  }
  
  // set the element type
  json.put_next_token(JSON_TOKEN_NAME, "items");
  
  // pass the tokenizer to the elements' type object to insert its schema
  get_ofType()->generate_json_schema(json, TRUE, FALSE);
}

void Type::generate_json_schema_record(JSON_Tokenizer& json)
{
  // use the JSON object type
  json.put_next_token(JSON_TOKEN_NAME, "type");
  json.put_next_token(JSON_TOKEN_STRING, "\"object\"");
  
  // use the "subType" property to distinguish records from sets
  json.put_next_token(JSON_TOKEN_NAME, "subType");
  json.put_next_token(JSON_TOKEN_STRING, (typetype == T_SEQ_T || typetype == T_SEQ_A) ?
    "\"record\"" : "\"set\"");
  
  // set the fields
  json.put_next_token(JSON_TOKEN_NAME, "properties");
  json.put_next_token(JSON_TOKEN_OBJECT_START);
  size_t field_count = get_nof_comps();
  bool has_non_optional = FALSE;
  for (size_t i = 0; i < field_count; ++i) {
    Type* field = get_comp_byIndex(i)->get_type();
    
    // use the field's alias if it has one
    json.put_next_token(JSON_TOKEN_NAME, 
      (field->jsonattrib != NULL && field->jsonattrib->alias != NULL) ?
      field->jsonattrib->alias : get_comp_byIndex(i)->get_name().get_ttcnname().c_str());
    
    // optional fields can also get the JSON null value
    if (get_comp_byIndex(i)->get_is_optional()) {
      // special case: ASN NULL type, since it uses the JSON literal "null" as a value
      if (T_NULL != field->get_type_refd_last()->typetype) {
        json.put_next_token(JSON_TOKEN_OBJECT_START);
        json.put_next_token(JSON_TOKEN_NAME, "anyOf");
        json.put_next_token(JSON_TOKEN_ARRAY_START);
        json.put_next_token(JSON_TOKEN_OBJECT_START);
        json.put_next_token(JSON_TOKEN_NAME, "type");
        json.put_next_token(JSON_TOKEN_STRING, "\"null\"");
        json.put_next_token(JSON_TOKEN_OBJECT_END);
      }
    } else if (!has_non_optional) {
      has_non_optional = TRUE;
    }
    
    // pass the tokenizer to the field's type to insert its schema
    field->generate_json_schema(json, TRUE, FALSE);
    
    // for optional fields: specify the presence of the "omit as null" coding instruction
    // and close structures
    if (get_comp_byIndex(i)->get_is_optional() &&
        T_NULL != field->get_type_refd_last()->typetype) {
      json.put_next_token(JSON_TOKEN_ARRAY_END);
      json.put_next_token(JSON_TOKEN_NAME, "omitAsNull");
      json.put_next_token((field->jsonattrib != NULL && field->jsonattrib->omit_as_null) ?
        JSON_TOKEN_LITERAL_TRUE : JSON_TOKEN_LITERAL_FALSE);
      json.put_next_token(JSON_TOKEN_OBJECT_END);
    }
  }
  
  // end of properties
  json.put_next_token(JSON_TOKEN_OBJECT_END);
  
  // do not accept additional fields
  json.put_next_token(JSON_TOKEN_NAME, "additionalProperties");
  json.put_next_token(JSON_TOKEN_LITERAL_FALSE);
  
  // set the field order
  if (field_count > 1) {
    json.put_next_token(JSON_TOKEN_NAME, "fieldOrder");
    json.put_next_token(JSON_TOKEN_ARRAY_START);
    for (size_t i = 0; i < field_count; ++i) {
      Type* field = get_comp_byIndex(i)->get_type();
      // use the field's alias if it has one
      char* field_str = mprintf("\"%s\"", 
        (field->jsonattrib != NULL && field->jsonattrib->alias != NULL) ?
        field->jsonattrib->alias : get_comp_byIndex(i)->get_name().get_ttcnname().c_str());
      json.put_next_token(JSON_TOKEN_STRING, field_str);
      Free(field_str);
    }
    json.put_next_token(JSON_TOKEN_ARRAY_END);
  }
  
  // set the required (non-optional) fields
  if (has_non_optional) {
    json.put_next_token(JSON_TOKEN_NAME, "required");
    json.put_next_token(JSON_TOKEN_ARRAY_START);
    for (size_t i = 0; i < field_count; ++i) {
      if (!get_comp_byIndex(i)->get_is_optional()) {
        Type* field = get_comp_byIndex(i)->get_type();
        // use the field's alias if it has one
        char* field_str = mprintf("\"%s\"", 
          (field->jsonattrib != NULL && field->jsonattrib->alias != NULL) ?
          field->jsonattrib->alias : get_comp_byIndex(i)->get_name().get_ttcnname().c_str());
        json.put_next_token(JSON_TOKEN_STRING, field_str);
        Free(field_str);
      }
    }
    json.put_next_token(JSON_TOKEN_ARRAY_END);
  }
}

void Type::generate_json_schema_union(JSON_Tokenizer& json)
{
  // use an "anyOf" structure containing the union's alternatives
  json.put_next_token(JSON_TOKEN_NAME, "anyOf");
  json.put_next_token(JSON_TOKEN_ARRAY_START);
  
  for (size_t i = 0; i < get_nof_comps(); ++i) {
    Type* field = get_comp_byIndex(i)->get_type();
    
    if (jsonattrib != NULL && jsonattrib->as_value) {
      // only add the alternative's schema
      field->generate_json_schema(json, TRUE, TRUE);
    } else {
      // use a JSON object with one key-value pair for each alternative
      // the schema is the same as a record's with one field
      json.put_next_token(JSON_TOKEN_OBJECT_START);
      
      json.put_next_token(JSON_TOKEN_NAME, "type");
      json.put_next_token(JSON_TOKEN_STRING, "\"object\"");
      
      json.put_next_token(JSON_TOKEN_NAME, "properties");
      json.put_next_token(JSON_TOKEN_OBJECT_START);
      
      // use the alternative's alias if it has one
      json.put_next_token(JSON_TOKEN_NAME, 
        (field->jsonattrib != NULL && field->jsonattrib->alias != NULL) ?
        field->jsonattrib->alias : get_comp_byIndex(i)->get_name().get_ttcnname().c_str());
      
      // let the alternative's type insert its schema
      field->generate_json_schema(json, TRUE, FALSE);
      
      // continue the schema for the record with one field
      json.put_next_token(JSON_TOKEN_OBJECT_END);
      
      json.put_next_token(JSON_TOKEN_NAME, "additionalProperties");
      json.put_next_token(JSON_TOKEN_LITERAL_FALSE);
      
      // the one field is non-optional
      json.put_next_token(JSON_TOKEN_NAME, "required");
      json.put_next_token(JSON_TOKEN_ARRAY_START);
      
      // use the alternative's alias here as well
      char* field_str = mprintf("\"%s\"", 
        (field->jsonattrib != NULL && field->jsonattrib->alias != NULL) ?
        field->jsonattrib->alias : get_comp_byIndex(i)->get_name().get_ttcnname().c_str());
      json.put_next_token(JSON_TOKEN_STRING, field_str);
      Free(field_str);
      
      json.put_next_token(JSON_TOKEN_ARRAY_END);
      
      json.put_next_token(JSON_TOKEN_OBJECT_END);
    }
  }
  
  // close the "anyOf" array
  json.put_next_token(JSON_TOKEN_ARRAY_END);
}

void Type::generate_json_schema_ref(JSON_Tokenizer& json) 
{
  // start the object containing the reference
  json.put_next_token(JSON_TOKEN_OBJECT_START);
  
  // insert the reference
  json.put_next_token(JSON_TOKEN_NAME, "$ref");
  char* ref_str = mprintf("\"#/definitions/%s/%s\"",
    my_scope->get_scope_mod()->get_modid().get_ttcnname().c_str(),
    get_dispname().c_str());
  json.put_next_token(JSON_TOKEN_STRING, ref_str);
  Free(ref_str);
  
  // the object will be closed later, as it may contain other properties
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

} // namespace Common
