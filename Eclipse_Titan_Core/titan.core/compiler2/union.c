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
 *   Cserveni, Akos
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Kremer, Peter
 *   Ormandi, Matyas
 *   Pandi, Krisztian
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include <string.h>

#include "../common/memory.h"
#include "datatypes.h"
#include "union.h"
#include "encdec.h"
#include "XSD_Types.hh"

#include "main.hh"
#include "ttcn3/compiler.h"

/** used in the RAW_decode functions generation to store information
 * about the temporal variables
 * type is the type of the variable
 * typedescr is the type's descriptor
 * start_pos is where the decoding should begin
 *   it is >= 0 if known at compile time, otherwise -1
 * use_counter stores how many times this type will be used
 *   if it is >1 than we create the variable "globally" so that that
 *     we don't need to create it again in the next component's decode try
 *   if it is ==1 than it is a local variable only referred at one place
 *     we create it only where it is needed, so if at runtime the decoding
 *     comes to a conclusion before the use of this variable, then its
 *     overhead won't slow down the decoding
 * decoded_for_element is the index of the element in the union for which
 *     this variable was last used for
 *   if it is -1 if the variable hasn't been decoded yet
 *     only in this case it is needed to generate the code to do the decoding
 *   if it is >=0 than we use it generate the conditions
 *     if it doesn't equal to the actual component's index, then we have to
 *       generate the condition and the decoding... parts to it.
 *     if it equals the actual component's index, then we only add its test
 *       to the actual condition
 */
typedef struct{
  const char* type;
  const char* typedescr;
  int start_pos;
  int use_counter;
  int decoded_for_element;
}temporal_variable;

void defUnionClass(struct_def const *sdef, output_struct *output)
{
  size_t i;
  const char *name = sdef->name, *dispname = sdef->dispname;
  const char *at_field = sdef->kind==ANYTYPE ? "AT_" : "";
  /* at_field is used to prefix the name of accessor member functions
   * for the anytype, otherwise the generated "INTEGER()" will look like
   * a constructor, upsetting the C++ compiler. AT_INTEGER() is ok. */
  char *def = NULL, *src = NULL;
  boolean ber_needed = sdef->isASN1 && enable_ber();
  boolean raw_needed = sdef->hasRaw && enable_raw();
  boolean text_needed = sdef->hasText && enable_text();
  boolean xer_needed = sdef->hasXer && enable_xer();
  boolean json_needed = sdef->hasJson && enable_json();

  char *selection_type, *unbound_value, *selection_prefix;
  selection_type = mcopystr("union_selection_type");
  unbound_value = mcopystr("UNBOUND_VALUE");
  selection_prefix = mcopystr("ALT");


  /* class declaration */
  output->header.class_decls = mputprintf(output->header.class_decls,
    "class %s;\n", name);

  /* class definition and source code */

  /* class header and data fields*/
  def = mputprintf(def,
#ifndef NDEBUG
    "// written by defUnionClass in " __FILE__ " at %d\n"
#endif
    "class %s : public Base_Type {\n"
#ifndef NDEBUG
    , __LINE__
#endif
    , name);
    /* selection type (enum) definition */
    def = mputstr(def, "public:\n"
      "enum union_selection_type { UNBOUND_VALUE = 0");
    for (i = 0; i < sdef->nElements; i++) {
      def = mputprintf(def, ", ALT_%s = %lu", sdef->elements[i].name,
      (unsigned long) (i + 1));
    }
    def = mputstr(def, " };\n"
      "private:\n");

  def = mputprintf(def, "%s union_selection;\n"
    "union {\n", selection_type);
  for (i = 0; i < sdef->nElements; i++) {
    def = mputprintf(def, "%s *field_%s;\n", sdef->elements[i].type,
      sdef->elements[i].name);
  }
  def = mputstr(def, "};\n");

  if(ber_needed && sdef->ot) {
    def=mputstr(def, "ASN_BER_TLV_t tlv_opentype;\n");
  }

  if (use_runtime_2) {
    def=mputstr(def, "Erroneous_descriptor_t* err_descr;\n");
  }

  /* copy_value function */
  def = mputprintf(def, "void copy_value(const %s& other_value);\n", name);
  src = mputprintf(src, "void %s::copy_value(const %s& other_value)\n"
    "{\n"
    "switch (other_value.union_selection) {\n", name, name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n", selection_prefix, sdef->elements[i].name);
    if (legacy_unbound_union_fields) {
      src = mputprintf(src, "if (other_value.field_%s->is_bound()) ",
        sdef->elements[i].name);
    }
    src = mputprintf(src, "field_%s = new %s(*other_value.field_%s);\n",
      sdef->elements[i].name, sdef->elements[i].type, sdef->elements[i].name);
    if (legacy_unbound_union_fields) {
      src = mputprintf(src,
        "else {\n"
        "field_%s = new %s;\n"
        "TTCN_warning(\"Assignment of a union value with an unbound selected "
        "alternative\");\n"      
        "}\n", sdef->elements[i].name, sdef->elements[i].type);
    }
    src = mputstr(src, "break;\n");
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Assignment of an unbound union value of type %s.\");\n"
    "}\n"
    "union_selection = other_value.union_selection;\n", dispname);

  if (use_runtime_2) {
    src = mputstr(src, "err_descr = other_value.err_descr;\n");
  }

  src = mputstr(src, "}\n\n");

  /* default constructor */
  def = mputprintf(def, "\npublic:\n"
    "%s();\n", name);
  src = mputprintf(src, "%s::%s()%s\n"
    "{\n"
    "union_selection = %s;\n"
    "}\n\n", name, name,
    use_runtime_2 ? ": err_descr(NULL)" : "",
    unbound_value);

  /* copy constructor */
  def = mputprintf(def, "%s(const %s& other_value);\n", name, name);
  src = mputprintf(src, "%s::%s(const %s& other_value)\n"
    ": Base_Type()" /* call the *default* constructor as before */
    "{\n"
    "copy_value(other_value);\n"
    "}\n\n", name, name, name);

  /* destructor */
  def = mputprintf(def, "~%s();\n", name);
  src = mputprintf(src, "%s::~%s()\n"
    "{\n"
    "clean_up();\n"
    "}\n\n", name, name);

  /* assignment operator */
  def = mputprintf(def, "%s& operator=(const %s& other_value);\n", name, name);
  src = mputprintf(src, "%s& %s::operator=(const %s& other_value)\n"
    "{\n"
    "if (this != &other_value) {\n"
    "clean_up();\n"
    "copy_value(other_value);\n"
    "}\n"
    "return *this;\n"
    "}\n\n", name, name, name);

  /* comparison operator */
  def = mputprintf(def, "boolean operator==(const %s& other_value) const;\n",
    name);
  src = mputprintf(src, "boolean %s::operator==(const %s& other_value) const\n"
    "{\n"
    "if (union_selection == %s) TTCN_error(\"The left operand of comparison "
      "is an unbound value of union type %s.\");\n"
    "if (other_value.union_selection == %s) TTCN_error(\"The right operand of "
      "comparison is an unbound value of union type %s.\");\n"
    "if (union_selection != other_value.union_selection) return FALSE;\n"
    "switch (union_selection) {\n", name, name, unbound_value, dispname,
    unbound_value, dispname);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "return *field_%s == *other_value.field_%s;\n", selection_prefix,
      sdef->elements[i].name, sdef->elements[i].name, sdef->elements[i].name);
  }
  src = mputstr(src, "default:\n"
    "return FALSE;\n"
    "}\n"
    "}\n\n");

  /* != and () operator */
  def = mputprintf(def, "inline boolean operator!=(const %s& other_value) "
    "const { return !(*this == other_value); }\n", name);

  /* field access functions */
  for (i = 0; i < sdef->nElements; i++) {
    def = mputprintf(def, "%s& %s%s();\n", sdef->elements[i].type,
      at_field, sdef->elements[i].name);
    src = mputprintf(src, "%s& %s::%s%s()\n"
      "{\n"
      "if (union_selection != %s_%s) {\n"
      "clean_up();\n"
      "field_%s = new %s;\n"
      "union_selection = %s_%s;\n"
      "}\n"
      "return *field_%s;\n"
      "}\n\n", sdef->elements[i].type, name, at_field, sdef->elements[i].name,
      selection_prefix, sdef->elements[i].name, sdef->elements[i].name,
      sdef->elements[i].type, selection_prefix, sdef->elements[i].name,
      sdef->elements[i].name);

    def = mputprintf(def, "const %s& %s%s() const;\n", sdef->elements[i].type,
      at_field, sdef->elements[i].name);
    src = mputprintf(src, "const %s& %s::%s%s() const\n"
      "{\n"
      "if (union_selection != %s_%s) TTCN_error(\"Using non-selected field "
	"%s in a value of union type %s.\");\n"
      "return *field_%s;\n"
      "}\n\n",
      sdef->elements[i].type, name, at_field, sdef->elements[i].name, selection_prefix,
      sdef->elements[i].name, sdef->elements[i].dispname, dispname,
      sdef->elements[i].name);
  }

  /* get_selection function */
  def = mputprintf(def, "inline %s get_selection() const "
    "{ return union_selection; }\n", selection_type);

  /* ischosen function */
  def = mputprintf(def, "boolean ischosen(%s checked_selection) const;\n",
    selection_type);
  src = mputprintf(src, "boolean %s::ischosen(%s checked_selection) const\n"
    "{\n"
    "if (checked_selection == %s) TTCN_error(\"Internal error: Performing "
      "ischosen() operation on an invalid field of union type %s.\");\n"
    "if (union_selection == %s) TTCN_error(\"Performing ischosen() operation "
      "on an unbound value of union type %s.\");\n"
    "return union_selection == checked_selection;\n"
    "}\n\n", name, selection_type, unbound_value, dispname, unbound_value,
    dispname);

  /* is_bound function */
  def = mputstr   (def, "boolean is_bound() const;\n");
  src = mputprintf(src, "boolean %s::is_bound() const\n"
    "{\n"
    "  return union_selection != %s;\n"
    "}\n\n", name, unbound_value);

  /* is_value function */
  def = mputstr   (def, "boolean is_value() const;\n");
  src = mputprintf(src, "boolean %s::is_value() const\n"
    "{\n"
    "switch (union_selection) {\n"
    "case %s: return FALSE;\n",
    name, unbound_value);
  for (i = 0; i < sdef->nElements; ++i) {
    src = mputprintf(src, "case %s_%s: return field_%s->is_value();\n",
      selection_prefix, sdef->elements[i].name, sdef->elements[i].name);
  }
  src = mputstr(src, "default: TTCN_error(\"Invalid selection in union is_bound\");"
    "}\n"
    "}\n\n");

    /* clean_up function */
  def = mputstr   (def, "void clean_up();\n");
  src = mputprintf(src, "void %s::clean_up()\n"
    "{\n"
    "switch (union_selection) {\n",
    name);
  for (i = 0; i < sdef->nElements; ++i) {
    src = mputprintf(src, "case %s_%s:\n"
      "  delete field_%s;\n"
      "  break;\n",
      selection_prefix, sdef->elements[i].name, sdef->elements[i].name);
  }
  src = mputprintf(src, "default:\n"
    "  break;\n"
    "}\n"
    "union_selection = %s;\n"
    "}\n\n", unbound_value);

  if (use_runtime_2) {
    def = mputstr(def,
      "boolean is_equal(const Base_Type* other_value) const;\n"
      "void set_value(const Base_Type* other_value);\n"
      "Base_Type* clone() const;\n"
      "const TTCN_Typedescriptor_t* get_descriptor() const;\n"
      "void set_err_descr(Erroneous_descriptor_t* p_err_descr) { err_descr=p_err_descr; }\n"
      "Erroneous_descriptor_t* get_err_descr() const { return err_descr; }\n");
    src = mputprintf(src,
      "boolean %s::is_equal(const Base_Type* other_value) const "
        "{ return *this == *(static_cast<const %s*>(other_value)); }\n"
      "void %s::set_value(const Base_Type* other_value) "
        "{ *this = *(static_cast<const %s*>(other_value)); }\n"
      "Base_Type* %s::clone() const { return new %s(*this); }\n"
      "const TTCN_Typedescriptor_t* %s::get_descriptor() const "
        "{ return &%s_descr_; }\n",
      name, name,
      name, name,
      name, name,
      name, name);
  } else {
    def = mputstr(def,
      "inline boolean is_present() const { return is_bound(); }\n");
  }

  /* log function */
  def = mputstr(def, "void log() const;\n");
  src = mputprintf(src, "void %s::log() const\n"
    "{\n"
    "switch (union_selection) {\n", name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "TTCN_Logger::log_event_str(\"{ %s := \");\n"
      "field_%s->log();\n"
      "TTCN_Logger::log_event_str(\" }\");\n"
      "break;\n", selection_prefix, sdef->elements[i].name,
      sdef->elements[i].dispname, sdef->elements[i].name);
  }
  src = mputstr(src, "default:\n"
    "TTCN_Logger::log_event_unbound();\n"
    "}\n");
  if (use_runtime_2) {
    src = mputstr(src, "if (err_descr) err_descr->log();\n");
  }
  src = mputstr(src, "}\n\n");

  /* set_param function */
  def = mputstr(def, "void set_param(Module_Param& param);\n");
  src = mputprintf(src, "void %s::set_param(Module_Param& param)\n"
     "{\n", name);
  if (use_runtime_2) {
    src = mputprintf(src,
     "  if (dynamic_cast<Module_Param_Name*>(param.get_id()) != NULL &&\n"
     "      param.get_id()->next_name()) {\n"
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the fields, not to the whole union
     "    char* param_field = param.get_id()->get_current_name();\n"
     "    if (param_field[0] >= '0' && param_field[0] <= '9') {\n"
     "      param.error(\"Unexpected array index in module parameter, expected a valid field\"\n"
     "        \" name for union type `%s'\");\n"
     "    }\n"
     "    ", dispname);
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src,
       "if (strcmp(\"%s\", param_field) == 0) {\n"
       "      %s%s().set_param(param);\n"
       "      return;\n"
       "    } else ",
       sdef->elements[i].dispname, at_field, sdef->elements[i].name);
    }
    src = mputprintf(src,
     "param.error(\"Field `%%s' not found in union type `%s'\", param_field);\n"
     "  }\n", dispname);
  }
  src = mputstr(src,
     "  param.basic_check(Module_Param::BC_VALUE, \"union value\");\n"
     "  Module_Param_Ptr m_p = &param;\n");
  if (use_runtime_2) {
    src = mputstr(src,
     "  if (param.get_type() == Module_Param::MP_Reference) {\n"
     "    m_p = param.get_referenced_param();\n"
     "  }\n");
  }
  src = mputstr(src,
     "  if (m_p->get_type()==Module_Param::MP_Value_List && m_p->get_size()==0) return;\n"
     "  if (m_p->get_type()!=Module_Param::MP_Assignment_List) {\n"
     "    param.error(\"union value with field name was expected\");\n"
     "  }\n"
     "  Module_Param* mp_last = m_p->get_elem(m_p->get_size()-1);\n");

  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, 
      "  if (!strcmp(mp_last->get_id()->get_name(), \"%s\")) {\n"
      "    %s%s().set_param(*mp_last);\n"
      "    if (!%s%s().is_bound()) "
      , sdef->elements[i].dispname, at_field, sdef->elements[i].name
      , at_field, sdef->elements[i].name);
    if (legacy_unbound_union_fields) {
      src = mputprintf(src,
        "TTCN_warning(\"Alternative '%s' was selected for union of type '%s', "
        "but its value is unbound\");\n"
        , sdef->elements[i].dispname, sdef->dispname);
    }
    else {
      // a union's alternative cannot be unbound
      src = mputstr(src, "clean_up();\n");
    }
    src = mputstr(src,
      "    return;\n"
      "  }\n");
  }
  src = mputprintf(src,
    "  mp_last->error(\"Field %%s does not exist in type %s.\", mp_last->get_id()->get_name());\n"
    "}\n\n", dispname);
  
  /* get param function, RT2 only */
  if (use_runtime_2) {
    def = mputstr(def, "Module_Param* get_param(Module_Param_Name& param_name) const;\n");
    src = mputprintf(src,
      "Module_Param* %s::get_param(Module_Param_Name& param_name) const\n"
      "{\n"
      "  if (!is_bound()) {\n"
      "    return new Module_Param_Unbound();\n"
      "  }\n"
      "  if (param_name.next_name()) {\n"
      // Haven't reached the end of the module parameter name
      // => the name refers to one of the fields, not to the whole union
      "    char* param_field = param_name.get_current_name();\n"
      "    if (param_field[0] >= '0' && param_field[0] <= '9') {\n"
      "      TTCN_error(\"Unexpected array index in module parameter reference, \"\n"
      "        \"expected a valid field name for union type `%s'\");\n"
      "    }\n"
      "    ", name, dispname);
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src,
       "if (strcmp(\"%s\", param_field) == 0) {\n"
       "      return %s%s().get_param(param_name);\n"
       "    } else ",
       sdef->elements[i].dispname, at_field, sdef->elements[i].name);
    }
    src = mputprintf(src,
      "TTCN_error(\"Field `%%s' not found in union type `%s'\", param_field);\n"
      "  }\n"
      "  Module_Param* mp_field = NULL;\n"
      "  switch(union_selection) {\n"
      , name);
      for (i = 0; i < sdef->nElements; ++i) {
        src = mputprintf(src, 
          "  case %s_%s:\n"
          "    mp_field = field_%s->get_param(param_name);\n"
          "    mp_field->set_id(new Module_Param_FieldName(mcopystr(\"%s\")));\n"
          "    break;\n"
          , selection_prefix, sdef->elements[i].name
          , sdef->elements[i].name, sdef->elements[i].dispname);
      }
    src = mputstr(src,
      "  default:\n"
      "    break;\n"
      "  }\n"
      "  Module_Param_Assignment_List* m_p = new Module_Param_Assignment_List();\n"
      "  m_p->add_elem(mp_field);\n"
      "  return m_p;\n"
      "}\n\n");
  }

  /* set implicit omit function, recursive */
  def = mputstr(def, "  void set_implicit_omit();\n");
  src = mputprintf(src,
    "void %s::set_implicit_omit()\n{\n"
    "switch (union_selection) {\n", name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src,
      "case %s_%s:\n"
      "field_%s->set_implicit_omit(); break;\n",
      selection_prefix, sdef->elements[i].name, sdef->elements[i].name);
  }
  src = mputstr(src, "default: break;\n}\n}\n\n");

  /* encode_text function */
  def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
  src = mputprintf(src, "void %s::encode_text(Text_Buf& text_buf) const\n"
    "{\n"
    "text_buf.push_int(union_selection);\n"
    "switch (union_selection) {\n", name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "field_%s->encode_text(text_buf);\n"
      "break;\n", selection_prefix, sdef->elements[i].name,
      sdef->elements[i].name);
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Text encoder: Encoding an unbound value of union type "
      "%s.\");\n"
    "}\n"
    "}\n\n", dispname);

  /* decode_text function */
  def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
  src = mputprintf(src, "void %s::decode_text(Text_Buf& text_buf)\n"
    "{\n"
    "switch ((%s)text_buf.pull_int().get_val()) {\n", name, selection_type);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "%s%s().decode_text(text_buf);\n"
      "break;\n", selection_prefix, sdef->elements[i].name,
      at_field, sdef->elements[i].name);
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Text decoder: Unrecognized union selector was received "
      "for type %s.\");\n"
    "}\n"
    "}\n\n", dispname);

  if(ber_needed || raw_needed || text_needed || xer_needed || json_needed)
    def_encdec(name, &def, &src, ber_needed, raw_needed, text_needed,
               xer_needed, json_needed, TRUE);

  /* BER functions */
  if(ber_needed) {
    def = mputstr(def, "private:\n"
      "boolean BER_decode_set_selection(const ASN_BER_TLV_t& p_tlv);\n"
      "public:\n"
      "boolean BER_decode_isMyMsg(const TTCN_Typedescriptor_t& p_td, "
	"const ASN_BER_TLV_t& p_tlv);\n");

    /* BER_encode_TLV() */
    src = mputprintf(src,
#ifndef NDEBUG
      "// written by %s in " __FILE__ " at %d\n"
#endif
      "ASN_BER_TLV_t *%s::BER_encode_TLV("
	"const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const\n"
      "{\n"
#ifndef NDEBUG
      , __FUNCTION__, __LINE__
#endif
      , name);
    if (use_runtime_2) {
      src = mputstr(src,
        "  if (err_descr) return BER_encode_TLV_negtest(err_descr, p_td, p_coding);\n");
    }
    src = mputstr(src,
      "  BER_chk_descr(p_td);\n"
      "  ASN_BER_TLV_t *new_tlv;\n"
      "  TTCN_EncDec_ErrorContext ec_0(\"Alternative '\");\n"
      "  TTCN_EncDec_ErrorContext ec_1;\n"
      "  switch (union_selection) {\n");
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src, "  case %s_%s:\n"
        "    ec_1.set_msg(\"%s': \");\n"
        "    new_tlv = field_%s->BER_encode_TLV(%s_descr_, p_coding);\n"
        "    break;\n", selection_prefix, sdef->elements[i].name,
	sdef->elements[i].dispname,
	sdef->elements[i].name, sdef->elements[i].typedescrname);
    } /* for i */
    src = mputprintf(src, "  case %s:\n"
      "    new_tlv = BER_encode_chk_bound(FALSE);\n"
      "    break;\n"
      "  default:\n"
      "    TTCN_EncDec_ErrorContext::error_internal(\"Unknown selection.\");\n"
      "    new_tlv = NULL;\n"
      "  }\n" /* switch */
      "  return ASN_BER_V2TLV(new_tlv, p_td, p_coding);\n"
      "}\n\n", unbound_value);

    if (use_runtime_2) { /* BER_encode_TLV_negtest() */
      def = mputstr(def,
        "ASN_BER_TLV_t* BER_encode_TLV_negtest(const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const;\n");
      src = mputprintf(src,
        "ASN_BER_TLV_t *%s::BER_encode_TLV_negtest(const Erroneous_descriptor_t* p_err_descr, "
          "const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const\n"
        "{\n"
        "  BER_chk_descr(p_td);\n"
        "  ASN_BER_TLV_t *new_tlv = NULL;\n"
        "  TTCN_EncDec_ErrorContext ec_0(\"Alternative '\");\n"
        "  TTCN_EncDec_ErrorContext ec_1;\n"
        "  const Erroneous_values_t* err_vals = NULL;\n"
        "  const Erroneous_descriptor_t* emb_descr = NULL;\n"
        "  switch (union_selection) {\n", name);
      for (i = 0; i < sdef->nElements; i++) {
        src = mputprintf(src, "  case %s_%s:\n"
          "    err_vals = p_err_descr->get_field_err_values(%d);\n"
          "    emb_descr = p_err_descr->get_field_emb_descr(%d);\n"
          "    if (err_vals && err_vals->value) {\n"
          "      if (err_vals->value->errval) {\n"
          "        ec_1.set_msg(\"%s'(erroneous value): \");\n"
          "        if (err_vals->value->raw) {\n"
          "          new_tlv = err_vals->value->errval->BER_encode_negtest_raw();\n"
          "        } else {\n"
          "          if (err_vals->value->type_descr==NULL) TTCN_error(\"internal error: erroneous value typedescriptor missing\");\n"
          "          new_tlv = err_vals->value->errval->BER_encode_TLV(*err_vals->value->type_descr, p_coding);\n"
          "        }\n"
          "      }\n"
          "    } else {\n"
          "      ec_1.set_msg(\"%s': \");\n"
          "      if (emb_descr) new_tlv = field_%s->BER_encode_TLV_negtest(emb_descr, %s_descr_, p_coding);\n"
          "      else new_tlv = field_%s->BER_encode_TLV(%s_descr_, p_coding);\n"
          "    }\n"
          "    break;\n", selection_prefix, sdef->elements[i].name,
          (int)i, (int)i, sdef->elements[i].dispname, sdef->elements[i].dispname,
          sdef->elements[i].name, sdef->elements[i].typedescrname,
          sdef->elements[i].name, sdef->elements[i].typedescrname);
      } /* for i */
      src = mputprintf(src, "  case %s:\n"
        "    new_tlv = BER_encode_chk_bound(FALSE);\n"
        "    break;\n"
        "  default:\n"
        "    TTCN_EncDec_ErrorContext::error_internal(\"Unknown selection.\");\n"
        "    new_tlv = NULL;\n"
        "  }\n" /* switch */
        "  if (new_tlv==NULL) new_tlv=ASN_BER_TLV_t::construct(NULL);\n"
        "  return ASN_BER_V2TLV(new_tlv, p_td, p_coding);\n"
        "}\n\n", unbound_value);
    }

    /* BER_decode_set_selection() */
    src = mputprintf(src, "boolean %s::BER_decode_set_selection("
	"const ASN_BER_TLV_t& p_tlv)\n"
      "{\n"
      "  clean_up();\n", name);
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src, "  field_%s = new %s;\n"
        "  union_selection = %s_%s;\n"
        "  if (field_%s->BER_decode_isMyMsg(%s_descr_, p_tlv)) return TRUE;\n"
        "  delete field_%s;\n", sdef->elements[i].name, sdef->elements[i].type,
	selection_prefix, sdef->elements[i].name, sdef->elements[i].name,
	sdef->elements[i].typedescrname, sdef->elements[i].name);
    } /* for i */
    src = mputprintf(src, "  union_selection = %s;\n"
      "  return FALSE;\n"
      "}\n\n", unbound_value);

    /* BER_decode_isMyMsg() */
    src = mputprintf(src, "boolean %s::BER_decode_isMyMsg("
	"const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv)\n"
      "{\n"
      "  if (p_td.ber->n_tags == 0) {\n"
      "    %s tmp_type;\n"
      "    return tmp_type.BER_decode_set_selection(p_tlv);\n"
      "  } else return Base_Type::BER_decode_isMyMsg(p_td, p_tlv);\n"
      "}\n\n", name, name);

    /* BER_decode_TLV() */
    src = mputprintf(src, "boolean %s::BER_decode_TLV("
	"const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, "
	"unsigned L_form)\n"
      "{\n"
      "  BER_chk_descr(p_td);\n"
      "  ASN_BER_TLV_t stripped_tlv;\n"
      "  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);\n"
      "  TTCN_EncDec_ErrorContext ec_0(\"While decoding '%s' type: \");\n",
      name, dispname);
    if (sdef->ot) {
      src = mputprintf(src, "  if (!BER_decode_TLV_CHOICE(*p_td.ber, "
	  "stripped_tlv, L_form, tlv_opentype)) {\n"
        "    tlv_opentype.Tlen = 0;\n"
        "    return FALSE;\n"
        "  }\n"
        "  clean_up();\n"
        "  union_selection = %s;\n", unbound_value);
    } else {
      src = mputstr(src, "  ASN_BER_TLV_t tmp_tlv;\n"
	"  if (!BER_decode_TLV_CHOICE(*p_td.ber, stripped_tlv, L_form, "
	  "tmp_tlv) || "
	  "!BER_decode_CHOICE_selection(BER_decode_set_selection(tmp_tlv), "
	  "tmp_tlv)) return FALSE;\n"
	"  TTCN_EncDec_ErrorContext ec_1(\"Alternative '\");\n"
	"  TTCN_EncDec_ErrorContext ec_2;\n"
	"  switch (union_selection) {\n");
      for(i = 0; i < sdef->nElements; i++) {
        src = mputprintf(src, "  case %s_%s:\n"
          "    ec_2.set_msg(\"%s': \");\n"
          "    field_%s->BER_decode_TLV(%s_descr_, tmp_tlv, L_form);\n"
          "    break;\n", selection_prefix, sdef->elements[i].name,
	  sdef->elements[i].dispname,
	  sdef->elements[i].name, sdef->elements[i].typedescrname);
      } /* for i */
      src = mputstr(src, "  default:\n"
        "    return FALSE;\n"
        "  }\n");
      if (sdef->opentype_outermost) {
        src = mputstr(src,
	  "  TTCN_EncDec_ErrorContext ec_1(\"While decoding opentypes: \");\n"
	  "  TTCN_Type_list p_typelist;\n"
	  "  BER_decode_opentypes(p_typelist, L_form);\n");
      } /* if sdef->opentype_outermost */
    }
    src = mputstr(src, "  return TRUE;\n"
      "}\n\n");

    if (sdef->ot || sdef->has_opentypes) { /* theoretically, these are
                                             mutually exlusive */
      /* BER_decode_opentypes() */
      def = mputstr(def, "void BER_decode_opentypes("
	  "TTCN_Type_list& p_typelist, unsigned L_form);\n");
      src = mputprintf(src, "void %s::BER_decode_opentypes("
	  "TTCN_Type_list& p_typelist, unsigned L_form)\n"
	"{\n", name);
      if (sdef->ot) {
	AtNotationList_t *anl = &sdef->ot->anl;
	OpentypeAlternativeList_t *oal = &sdef->ot->oal;
        src = mputprintf(src,
	  "  if (union_selection != %s) return;\n"
	  "  TTCN_EncDec_ErrorContext ec_0(\"While decoding open type '%s': "
	    "\");\n", unbound_value, dispname);
        if (oal->nElements > 0) {
	  size_t oal_i, anl_i;
	  char *s2;
          /* variable declarations - the referenced components */
          for (anl_i = 0; anl_i < anl->nElements; anl_i++) {
            AtNotation_t *an = anl->elements + anl_i;
            src = mputprintf(src, "  const %s& f_%lu = static_cast<const %s*>"
	      "(p_typelist.get_nth(%lu))->%s;\n", an->type_name,
              (unsigned long) (anl_i + 1), an->parent_typename,
	      (unsigned long) an->parent_level, an->sourcecode);
          } /* for anl_i */
          src = mputstr(src, "  {\n"
	    "    TTCN_EncDec_ErrorContext ec_1(\"Alternative '\");\n"
	    "    TTCN_EncDec_ErrorContext ec_2;\n");
          s2 = mprintf("%*s", (int)(anl->nElements + 2) * 2, "");
          for (oal_i = 0; oal_i < oal->nElements; oal_i++) {
	    size_t if_level;
	    OpentypeAlternative_t *oa = oal->elements + oal_i;
            if (oal_i > 0) {
              for (if_level = 0; if_level < anl->nElements; if_level++)
                if (oa->const_valuenames[if_level]) break;
              for (i = anl->nElements; i > if_level; i--)
                src = mputprintf(src, "%*s}\n", (int)(i + 1) * 2, "");
            } /* if oal_i */
            else if_level = 0;
            for (anl_i = if_level; anl_i < anl->nElements; anl_i++) {
	      src = mputprintf(src, "%*s%sif (f_%lu == %s) {\n",
		(int)(anl_i + 2) * 2, "",
		oal_i && anl_i <= if_level ? "else " : "",
                (unsigned long) (anl_i + 1), oa->const_valuenames[anl_i]);
            } /* for anl_i */
	    src = mputprintf(src, "%sunion_selection = %s_%s;\n"
	      "%sfield_%s = new %s;\n"
	      "%sec_2.set_msg(\"%s': \");\n"
	      "%sfield_%s->BER_decode_TLV(%s_descr_, tlv_opentype, L_form);\n",
	      s2, selection_prefix, oa->alt, s2, oa->alt, oa->alt_typename, s2,
	      oa->alt_dispname, s2, oa->alt, oa->alt_typedescrname);
          } /* for oal_i */
          Free(s2);
          if (oal->nElements > 0)
            for (i = anl->nElements; i > 0; i--)
              src = mputprintf(src, "%*s}\n", (int)(i+1)*2, "");
          src = mputprintf(src, "  }\n"
	    "  if (union_selection == %s) {\n"
	    "    ec_0.error(TTCN_EncDec::ET_DEC_OPENTYPE, \"Cannot decode "
	      "open type: broken component relation constraint.\");\n"
	    "    if (TTCN_EncDec::get_error_behavior("
	      "TTCN_EncDec::ET_DEC_OPENTYPE) != TTCN_EncDec::EB_IGNORE) {\n"
	    "      TTCN_Logger::log_str(TTCN_WARNING, \"The value%s of"
	    " constraining component%s:\");\n", unbound_value,
	    anl->nElements > 1 ? "s" : "", anl->nElements > 1 ? "s" : "");
          for (anl_i = 0; anl_i < anl->nElements; anl_i++) {
            AtNotation_t *an = anl->elements + anl_i;
            src = mputprintf(src,
	      "      TTCN_Logger::begin_event(TTCN_WARNING);\n"
	      "      TTCN_Logger::log_event_str(\"Component '%s': \");\n"
	      "      f_%lu.log();\n"
	      "      TTCN_Logger::end_event();\n", an->dispname,
              (unsigned long) (anl_i + 1));
          } /* for anl_i */
          src = mputstr(src, "    }\n"
	    "  }\n");
        } /* if oal->nElements>0 */
        else {
          src = mputstr(src, "  ec_0.error(TTCN_EncDec::ET_DEC_OPENTYPE, "
	      "\"Cannot decode open type: the constraining object set is "
	      "empty.\");\n");
        } /* oal->nElements==0 */
      } /* if sdef->ot */
      else { /* if !sdef->ot (but has_opentypes) */
        src = mputstr(src,
	  "  p_typelist.push(this);\n"
	  "  TTCN_EncDec_ErrorContext ec_0(\"Alternative '\");\n"
	  "  TTCN_EncDec_ErrorContext ec_1;\n"
	  "  switch (union_selection) {\n");
        for (i = 0; i < sdef->nElements; i++) {
          src = mputprintf(src, "  case %s_%s:\n"
	    "    ec_1.set_msg(\"%s': \");\n"
	    "    field_%s->BER_decode_opentypes(p_typelist, L_form);\n"
	    "    break;\n", selection_prefix, sdef->elements[i].name,
	    sdef->elements[i].dispname, sdef->elements[i].name);
        } /* for i */
        src = mputstr(src, "  default:\n"
	  "    break;\n"
	  "  }\n"
	  "  p_typelist.pop();\n");
      } /* if has opentypes */
      src = mputstr(src, "}\n"
	"\n");
    } /* if sdef->ot || sdef->has_opentypes */
  } /* if ber_needed */

  if (raw_needed) {
    size_t nTemp_variables = 0;
    temporal_variable* temp_variable_list = NULL;
    int* tag_type = (int*) Malloc(sdef->nElements*sizeof(int));
    memset(tag_type, 0, sdef->nElements * sizeof(int));
    if (sdef->hasRaw) { /* fill tag_type. 0-No tag, >0 index of the tag + 1 */
      for (i = 0; i < sdef->raw.taglist.nElements; i++) {
        if (sdef->raw.taglist.list[i].nElements) {
          boolean found = FALSE;
          size_t v;
          for (v = 0; v < sdef->raw.taglist.list[i].nElements; v++) {
            if (sdef->raw.taglist.list[i].fields[v].start_pos >= 0) {
              found = TRUE;
              break;
            }
          }
          if (found)
            tag_type[sdef->raw.taglist.list[i].fieldnum] = i + 1;
          else
            tag_type[sdef->raw.taglist.list[i].fieldnum] = -i - 1;
        }
      }
    }
    src = mputprintf(src, "int %s::RAW_decode(\n"
      "const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, int limit, \n"
      "raw_order_t top_bit_ord, boolean no_err, int sel_field, boolean)\n"
      "{\n"
      "  int prepaddlength=p_buf.increase_pos_padd(p_td.raw->prepadding);\n"
      "  limit-=prepaddlength;\n"
      "  int decoded_length=0;\n"
      "  int starting_pos=p_buf.get_pos_bit();\n"
      "  if(sel_field!=-1){\n"
      "    switch(sel_field){\n", name);
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src,
        "    case %lu:\n"
        "      decoded_length = %s().RAW_decode(%s_descr_, p_buf, limit, "
        "top_bit_ord, no_err);\n"
        "      break;\n", (unsigned long) i, sdef->elements[i].name,
        sdef->elements[i].typedescrname);
    }
    src = mputstr(src, "    default: break;\n"
      "    }\n"
      "    return decoded_length + p_buf.increase_pos_padd(p_td.raw->padding) + "
      "prepaddlength;\n"
      "  } else {\n");
    /* only generate this variable if it will be used */
    for (i = 0; i < sdef->nElements; i++) {
      if ((tag_type[i] > 0)
        && (sdef->raw.taglist.list + tag_type[i] - 1)->nElements) {
        src = mputstr(src, "    boolean already_failed = FALSE;\n");
        break;
      }
    }

    /* precalculate what we know about the temporal variables*/
    for (i = 0; i < sdef->nElements; i++) {
      if ((tag_type[i] > 0)
        && (sdef->raw.taglist.list + tag_type[i] - 1)->nElements) {
        rawAST_coding_taglist* cur_choice = sdef->raw.taglist.list + tag_type[i] - 1;
        size_t j;
        for (j = 0; j < cur_choice->nElements; j++) {
          rawAST_coding_field_list *fieldlist = cur_choice->fields + j;
          if (fieldlist->start_pos >= 0) {
            size_t k;
            boolean found = FALSE;
            for (k = 0; k < nTemp_variables; k++) {
              if (temp_variable_list[k].start_pos == fieldlist->start_pos
                && !strcmp(temp_variable_list[k].typedescr,
                  fieldlist->fields[fieldlist->nElements - 1].typedescr)) {
                temp_variable_list[k].use_counter++;
                fieldlist->temporal_variable_index = k;
                found = TRUE;
                break;
              }
            }
            if (!found) {
              temp_variable_list
                = (temporal_variable*) Realloc(temp_variable_list,
                  (nTemp_variables + 1) * sizeof(*temp_variable_list));
              temp_variable_list[nTemp_variables].type
                = fieldlist->fields[fieldlist->nElements - 1].type;
              temp_variable_list[nTemp_variables].typedescr
                = fieldlist->fields[fieldlist->nElements - 1].typedescr;
              temp_variable_list[nTemp_variables].start_pos
                = fieldlist->start_pos;
              temp_variable_list[nTemp_variables].use_counter = 1;
              temp_variable_list[nTemp_variables].decoded_for_element
                = -1;
              fieldlist->temporal_variable_index = nTemp_variables;
              nTemp_variables++;
            }
          }
        }
      }
    }

    for (i = 0; i < nTemp_variables; i++) {
      if (temp_variable_list[i].use_counter > 1) {
        src = mputprintf(src, "    %s temporal_%lu;\n"
          "    int decoded_%lu_length;\n", temp_variable_list[i].type,
          (unsigned long) i, (unsigned long) i);
      }
    }

    for (i = 0; i < sdef->nElements; i++) { /* fields with tag */
      if ((tag_type[i] > 0)
        && (sdef->raw.taglist.list + tag_type[i] - 1)->nElements) {
        rawAST_coding_taglist* cur_choice = sdef->raw.taglist.list
          + tag_type[i] - 1;
        size_t j;
        src = mputstr(src, "    already_failed = FALSE;\n");
        /* first check the fields we can precode
         * try to decode those key variables whose position we know
         * this way we might be able to step over bad values faster
         */
        for (j = 0; j < cur_choice->nElements; j++) {
          rawAST_coding_field_list *cur_field_list = cur_choice->fields + j;
          if (cur_field_list->start_pos >= 0) {
            size_t k;
            size_t variable_index = cur_field_list->temporal_variable_index;
            if (temp_variable_list[variable_index].decoded_for_element == i) continue;
            src = mputstr(src, "    if (!already_failed) {\n");
            if (temp_variable_list[variable_index].use_counter == 1) {
              src = mputprintf(src, "      %s temporal_%lu;\n"
                "      int decoded_%lu_length;\n",
                temp_variable_list[variable_index].type,
                (unsigned long) variable_index, (unsigned long) variable_index);
            }
            if (temp_variable_list[variable_index].decoded_for_element
              == -1) {
              src = mputprintf(src,
                "      p_buf.set_pos_bit(starting_pos + %d);\n"
                "      decoded_%lu_length = temporal_%lu.RAW_decode("
                "%s_descr_, p_buf, limit, top_bit_ord, TRUE);\n",
                cur_field_list->start_pos, (unsigned long) variable_index,
                (unsigned long) variable_index,
                temp_variable_list[variable_index].typedescr);
            }
            temp_variable_list[variable_index].decoded_for_element = i;
            src = mputprintf(src, "      if (decoded_%lu_length > 0) {\n"
              "        if (temporal_%lu == %s", (unsigned long) variable_index,
              (unsigned long) variable_index, cur_field_list->value);
            for (k = j + 1; k < cur_choice->nElements; k++) {
              if (cur_choice->fields[k].temporal_variable_index
                == variable_index) {
                src = mputprintf(src, " || temporal_%lu == %s",
                  (unsigned long) variable_index, cur_choice->fields[k].value);
              }
            }
            src = mputprintf(src, ") {\n"
              "          p_buf.set_pos_bit(starting_pos);\n"
              "          decoded_length = %s().RAW_decode(%s_descr_, p_buf, "
              "limit, top_bit_ord, TRUE);\n"
              "          if (decoded_length > 0) {\n", sdef->elements[i].name,
              sdef->elements[i].typedescrname);
            src = mputstr(src, "             if (");
            src = genRawFieldChecker(src, cur_choice, TRUE);
            src = mputstr(src, ") {\n"
              "               return decoded_length + "
              "p_buf.increase_pos_padd(p_td.raw->padding) + prepaddlength;\n"
              "             }else already_failed = TRUE;\n"
              "          }\n"
              "        }\n"
              "      }\n"
              "    }\n");
          }
        }
        /* if there is one tag key whose position we don't know
         * and we couldn't decide yet if the element can be decoded or not
         * than we have to decode it.
         * note that this is not actually a cycle because of the break
         */
        for (j = 0; j < cur_choice->nElements; j++) {
          if (cur_choice->fields[j].start_pos < 0) {
            src = mputprintf(src, "    if (already_failed) {\n"
              "      p_buf.set_pos_bit(starting_pos);\n"
              "      decoded_length = %s().RAW_decode(%s_descr_, p_buf, limit, "
              "top_bit_ord, TRUE);\n"
              "      if (decoded_length > 0) {\n", sdef->elements[i].name,
              sdef->elements[i].typedescrname);
            src = mputstr(src, "        if (");
            src = genRawFieldChecker(src, cur_choice, TRUE);
            src = mputstr(src, ") {\n"
              "          return decoded_length + "
              "p_buf.increase_pos_padd(p_td.raw->padding) + prepaddlength;\n"
              "        }\n"
              "      }\n"
              "    }\n");
            break;
          }
        }
      }
    }
    Free(temp_variable_list);
    for (i = 0; i < sdef->nElements; i++) { /* fields with only variable tag */
      if ((tag_type[i] < 0)
        && (sdef->raw.taglist.list - tag_type[i] - 1)->nElements) {
        rawAST_coding_taglist* cur_choice = sdef->raw.taglist.list
          - tag_type[i] - 1;
        src = mputprintf(src, "      p_buf.set_pos_bit(starting_pos);\n"
          "      decoded_length = %s().RAW_decode(%s_descr_, p_buf, limit, "
          "top_bit_ord, TRUE);\n"
          "      if (decoded_length >= 0) {\n", sdef->elements[i].name,
          sdef->elements[i].typedescrname);
        src = mputstr(src, "        if (");
        src = genRawFieldChecker(src, cur_choice, TRUE);
        src = mputstr(src, ") {\n"
          "         return decoded_length + "
          "p_buf.increase_pos_padd(p_td.raw->padding) + prepaddlength;\n"
          "       }\n"
          "    }\n");
      }
    }/**/
    for (i = 0; i < sdef->nElements; i++) { /* fields without tag */
      if (!tag_type[i]) {
        src = mputprintf(src, "      p_buf.set_pos_bit(starting_pos);\n"
          "      decoded_length = %s().RAW_decode(%s_descr_, p_buf, limit, "
          "top_bit_ord, TRUE);\n"
          "      if (decoded_length >= 0) {\n", sdef->elements[i].name,
          sdef->elements[i].typedescrname);
        src = mputstr(src, "         return decoded_length + "
          "p_buf.increase_pos_padd(p_td.raw->padding) + prepaddlength;\n"
          "       }\n");
      }
    }/**/
    src = mputstr(src, " }\n"
      " clean_up();\n"
      " return -1;\n"
      "}\n\n");
    /* encoder */
    src = mputprintf(src, "int %s::RAW_encode("
      "const TTCN_Typedescriptor_t&%s, RAW_enc_tree& myleaf) const\n"
      "{\n", name, use_runtime_2 ? " p_td" : "");
    if (use_runtime_2) {
      src = mputstr(src, "  if (err_descr) return RAW_encode_negtest(err_descr, p_td, myleaf);\n");
    }
    src = mputprintf(src,
      "  int encoded_length = 0;\n"
      "  myleaf.isleaf = FALSE;\n"
      "  myleaf.body.node.num_of_nodes = %lu;"
      "  myleaf.body.node.nodes = init_nodes_of_enc_tree(%lu);\n"
      "  memset(myleaf.body.node.nodes, 0, %lu * sizeof(RAW_enc_tree *));\n"
      "  switch (union_selection) {\n", (unsigned long)sdef->nElements,
      (unsigned long)sdef->nElements, (unsigned long)sdef->nElements);
    for (i = 0; i < sdef->nElements; i++) {
      int t_type = tag_type[i] > 0 ? tag_type[i] : -tag_type[i];
      src = mputprintf(src, "  case %s_%s:\n"
        "    myleaf.body.node.nodes[%lu] = new RAW_enc_tree(TRUE, &myleaf, "
        "&myleaf.curr_pos, %lu, %s_descr_.raw);\n"
        "    encoded_length = field_%s->RAW_encode(%s_descr_, "
        "*myleaf.body.node.nodes[%lu]);\n"
        "    myleaf.body.node.nodes[%lu]->coding_descr = &%s_descr_;\n",
        selection_prefix, sdef->elements[i].name, (unsigned long) i,
        (unsigned long) i, sdef->elements[i].typedescrname,
        sdef->elements[i].name, sdef->elements[i].typedescrname,
        (unsigned long) i, (unsigned long) i, sdef->elements[i].typedescrname);
      if (t_type && (sdef->raw.taglist.list + t_type - 1)->nElements) {
        rawAST_coding_taglist* cur_choice = sdef->raw.taglist.list + t_type - 1;
        src = mputstr(src, "    if (");
        src = genRawFieldChecker(src, cur_choice, FALSE);
        src = mputstr(src, ") {\n");
        src = genRawTagChecker(src, cur_choice);
        src = mputstr(src, "    }\n");
      }
      src = mputstr(src, "    break;\n");
    }
    src = mputstr(src, "  default:\n"
      "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND, "
      "\"Encoding an unbound value.\");\n"
      "  }\n"
      "  return encoded_length;\n"
      "}\n\n");
    if (use_runtime_2) {
      def = mputstr(def, "int RAW_encode_negtest(const Erroneous_descriptor_t *, "
        "const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;\n");
      src = mputprintf(src, "int %s::RAW_encode_negtest(const Erroneous_descriptor_t *p_err_descr, "
        "const TTCN_Typedescriptor_t& /*p_td*/, RAW_enc_tree& myleaf) const\n"
        "{\n"
        "  const Erroneous_values_t *err_vals = NULL;\n"
        "  const Erroneous_descriptor_t *emb_descr = NULL;\n"
        "  int encoded_length = 0;\n"
        "  myleaf.isleaf = FALSE;\n"
        "  myleaf.body.node.num_of_nodes = %lu;\n"
        "  myleaf.body.node.nodes = init_nodes_of_enc_tree(%lu);\n"
        "  memset(myleaf.body.node.nodes, 0, %lu * sizeof(RAW_enc_tree *));\n"
        "  switch (union_selection) {\n", name, (unsigned long)sdef->nElements,
        (unsigned long)sdef->nElements, (unsigned long)sdef->nElements);
      for (i = 0; i < sdef->nElements; i++) {
        int t_type = tag_type[i] > 0 ? tag_type[i] : -tag_type[i];
        src = mputprintf(src,
          "  case %s_%s: {\n",
          selection_prefix, sdef->elements[i].name);
        if (t_type && (sdef->raw.taglist.list + t_type - 1)->nElements) {
          src = mputstr(src, "    boolean negtest_confl_tag = FALSE;\n");
        }
        src = mputprintf(src,
          "    err_vals = p_err_descr->get_field_err_values(%d);\n"
          "    emb_descr = p_err_descr->get_field_emb_descr(%d);\n"
          "    if (err_vals && err_vals->value) {\n"
          "      if (err_vals->value->raw) {\n"
          "        myleaf.body.node.nodes[%lu] =\n"
          "          new RAW_enc_tree(TRUE, &myleaf, &myleaf.curr_pos, %lu, "
          "err_vals->value->errval->get_descriptor()->raw);\n"
          "        encoded_length = err_vals->value->errval->RAW_encode_negtest_raw(*myleaf.body.node.nodes[%lu]);\n"
          "        myleaf.body.node.nodes[%lu]->coding_descr = err_vals->value->errval->get_descriptor();\n"
          "      } else {\n"
          "        if (err_vals->value->errval) {\n"
          "          if (err_vals->value->type_descr == NULL)\n"
          "            TTCN_error(\"internal error: erroneous value typedescriptor missing\");\n"
          "          myleaf.body.node.nodes[%lu] = new RAW_enc_tree(TRUE, &myleaf, &myleaf.curr_pos, %lu, err_vals->value->type_descr->raw);\n"
          "          encoded_length = err_vals->value->errval->RAW_encode(*err_vals->value->type_descr, *myleaf.body.node.nodes[%lu]);\n"
          "          myleaf.body.node.nodes[%lu]->coding_descr = err_vals->value->type_descr;\n"
          "        }\n"
          "      }\n",
          (int)i, (int)i,
          (unsigned long)i, (unsigned long)i,
          (unsigned long)i, (unsigned long)i, (unsigned long)i,
          (unsigned long)i, (unsigned long)i, (unsigned long)i);
        if (t_type && (sdef->raw.taglist.list + t_type - 1)->nElements) {
          /* Avoid TAGs.  */
          src = mputprintf(src, "      negtest_confl_tag = TRUE;\n");
        }
        src = mputprintf(src,
          "    } else {\n"
          "      myleaf.body.node.nodes[%lu] = new RAW_enc_tree(TRUE, "
          "&myleaf, &myleaf.curr_pos, %lu, %s_descr_.raw);\n"
          "      if (emb_descr) {\n",
          (unsigned long)i, (unsigned long)i, sdef->elements[i].typedescrname);
        if (t_type && (sdef->raw.taglist.list + t_type - 1)->nElements) {
          /* Avoid TAGs.  */
          src = mputprintf(src, "        negtest_confl_tag = TRUE;\n");
        }
        src = mputprintf(src,
          "        encoded_length = field_%s->RAW_encode_negtest(emb_descr, %s_descr_, *myleaf.body.node.nodes[%lu]);\n"
          "      } else encoded_length = field_%s->RAW_encode(%s_descr_, *myleaf.body.node.nodes[%lu]);\n"
          "      myleaf.body.node.nodes[%lu]->coding_descr = &%s_descr_;\n"
          "    }\n",
          sdef->elements[i].name, sdef->elements[i].typedescrname,
          (unsigned long)i, sdef->elements[i].name,
          sdef->elements[i].typedescrname, (unsigned long)i, (unsigned long)i,
          sdef->elements[i].typedescrname);
        if (t_type && (sdef->raw.taglist.list + t_type - 1)->nElements) {
          rawAST_coding_taglist *cur_choice = sdef->raw.taglist.list + t_type - 1;
          src = mputprintf(src,
            "    if (negtest_confl_tag) {\n"
            "      TTCN_EncDec_ErrorContext e_c;\n"
            "      e_c.set_msg(\"Field '%s': \");\n"
            "      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
            "        \"Conflicting negative testing attributes, TAG attribute "
            "will be ignored\");\n"
            "    }\n"
            "    if (!negtest_confl_tag && (", sdef->elements[i].name);
          src = genRawFieldChecker(src, cur_choice, FALSE);
          src = mputstr(src, ")) {\n");
          src = genRawTagChecker(src, cur_choice);
          src = mputstr(src, "    }\n");
        }
        src = mputstr(src, "    break; }\n");
      }
      src = mputstr(src, "  default:\n"
        "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND, "
        "\"Encoding an unbound value.\");\n"
        "  }\n"
      "  return encoded_length;\n"
      "}\n\n");
    }
    Free(tag_type);
  } /* if raw_needed */

  if (text_needed) {
    src = mputprintf(src, "int %s::TEXT_encode("
      "const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf) const\n"
      "{\n", name);
    if (use_runtime_2) {
      src = mputstr(src, "  if (err_descr) return TEXT_encode_negtest(err_descr, p_td, p_buf);\n");
    }
    src = mputstr(src,  "  int encoded_length=0;\n"
      "  if (p_td.text->begin_encode) {\n"
      "    p_buf.put_cs(*p_td.text->begin_encode);\n"
      "    encoded_length += p_td.text->begin_encode->lengthof();\n"
      "  }\n"
      "  switch(union_selection){\n");
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src, "  case %s_%s:\n"
	"   encoded_length += field_%s->TEXT_encode(%s_descr_,p_buf);\n"
	"   break;\n", selection_prefix, sdef->elements[i].name,
	sdef->elements[i].name, sdef->elements[i].typedescrname);
    }
    src = mputstr(src, "  default:\n"
      "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND, "
	"\"Encoding an unbound value.\");\n"
      "    break;\n"
      "  }\n"
      "  if (p_td.text->end_encode) {\n"
      "    p_buf.put_cs(*p_td.text->end_encode);\n"
      "    encoded_length += p_td.text->end_encode->lengthof();\n"
      "  }\n"
      "  return encoded_length;\n"
      "}\n\n");
    if (use_runtime_2) {/*TEXT_encde_negtest()*/
      def = mputstr(def,
        "int TEXT_encode_negtest(const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf) const;\n");
      src = mputprintf(src, "int %s::TEXT_encode_negtest("
        "const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf) const\n"
            "{\n"
            "  int encoded_length=0;\n"
            "  const Erroneous_values_t* err_vals = NULL;\n"
            "  const Erroneous_descriptor_t* emb_descr = NULL;\n"
            "  if (p_td.text->begin_encode) {\n"
            "    p_buf.put_cs(*p_td.text->begin_encode);\n"
            "    encoded_length += p_td.text->begin_encode->lengthof();\n"
            "  }\n"
            "  switch(union_selection){\n", name);
          for (i = 0; i < sdef->nElements; i++) {
            src = mputprintf(src, "  case %s_%s:\n"
              "   err_vals = p_err_descr->get_field_err_values(%d);\n"
              "   emb_descr = p_err_descr->get_field_emb_descr(%d);\n"
              "   if (err_vals && err_vals->value){\n"
              "     if (err_vals->value->errval) {\n"
              "       if(err_vals->value->raw){\n"
              "         encoded_length += err_vals->value->errval->encode_raw(p_buf);\n"
              "       }else{\n"
              "         if (err_vals->value->type_descr==NULL) TTCN_error(\"internal error: erroneous value typedescriptor missing\");\n"
              "         encoded_length += err_vals->value->errval->TEXT_encode(*err_vals->value->type_descr,p_buf);\n"
              "       }\n"
              "     }\n"
              "   }else{\n"
              "     if (emb_descr) encoded_length += field_%s->TEXT_encode_negtest(emb_descr,%s_descr_,p_buf);\n"
              "     else field_%s->TEXT_encode(%s_descr_,p_buf);\n"
              "   }\n"
        "   break;\n", selection_prefix, sdef->elements[i].name,(int)i,(int)i,
        sdef->elements[i].name, sdef->elements[i].typedescrname, sdef->elements[i].name, sdef->elements[i].typedescrname);
          }
          src = mputstr(src, "  default:\n"
            "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND, "
        "\"Encoding an unbound value.\");\n"
            "    break;\n"
            "  }\n"
            "  if (p_td.text->end_encode) {\n"
            "    p_buf.put_cs(*p_td.text->end_encode);\n"
            "    encoded_length += p_td.text->end_encode->lengthof();\n"
            "  }\n"
            "  return encoded_length;\n"
            "}\n\n");
    }

    src = mputprintf(src, "int %s::TEXT_decode("
	"const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, "
	"Limit_Token_List& limit, boolean no_err, boolean)\n"
      "{\n"
      "  int decoded_length = 0;\n"
      "  if(p_td.text->begin_decode){\n"
      "    int tl = p_td.text->begin_decode->match_begin(p_buf);\n"
      "    if (tl >= 0) {\n"
      "      decoded_length += tl;\n"
      "      p_buf.increase_pos(tl);\n"
      "    } else {\n"
      "      if (no_err) return -1;\n"
      "      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR, "
	"\"The specified token '%%s' not found for '%%s': \", "
	"(const char*)(*p_td.text->begin_decode), p_td.name);\n"
      "      return 0;\n"
      "    }\n"
      "  }\n"
      "  if (p_buf.get_read_len() < 1 && no_err) return -1;\n"
      "%s"
      "  int ml = 0;\n"
      "  boolean found = FALSE;\n"
      "  if (p_td.text->end_decode) {\n"
      "    limit.add_token(p_td.text->end_decode);\n"
      "    ml++;\n"
      "  }\n", name, sdef->nElements > 1 ? "  size_t pos = p_buf.get_pos();\n" : "");
    if (sdef->nElements > 0) {
      src = mputprintf(src,
	"  int str_len = %s().TEXT_decode(%s_descr_, p_buf, limit, TRUE);\n"
	"  if (str_len >= 0) found = TRUE;\n", sdef->elements[0].name,
	sdef->elements[0].typedescrname);
    }
    for (i = 1; i < sdef->nElements; i++) {
      src = mputprintf(src, "  if (!found) {\n"
	"    p_buf.set_pos(pos);\n"
	"    str_len = %s().TEXT_decode(%s_descr_, p_buf, limit, TRUE);\n"
	"    if (str_len >= 0) found = TRUE;\n"
	"  }\n", sdef->elements[i].name, sdef->elements[i].typedescrname);
    }
    src = mputstr(src, "  limit.remove_tokens(ml);\n"
      "  if (found) {\n"
      "    decoded_length += str_len;\n"
      "    if (p_td.text->end_decode) {\n"
      "      int tl = p_td.text->end_decode->match_begin(p_buf);\n"
      "      if (tl >= 0){\n"
      "        decoded_length += tl;\n"
      "        p_buf.increase_pos(tl);\n"
      "      } else {\n"
      "        if (no_err) return -1;\n"
      "        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR, "
	"\"The specified token '%s' not found for '%s': \", "
	"(const char*)(*p_td.text->end_decode), p_td.name);\n"
      "      }\n"
      "    }\n"
      "  } else {\n"
      "    if (no_err) return -1;\n"
      "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR, "
	"\"No union member found for '%s': \", p_td.name);\n"
      "    clean_up();\n"
      "  }\n"
      "  return decoded_length;\n"
      "}\n");
  }

  if (xer_needed) { /* XERSTUFF encoder functions for union */
    def = mputstr(def,
      "char **collect_ns(const XERdescriptor_t& p_td, size_t& num, boolean& def_ns, unsigned int flavor = 0) const;\n");
    src=mputprintf(src,
      "boolean %s::can_start(const char *name, const char *uri,"
      " const XERdescriptor_t& xd, unsigned int flavor, unsigned int flavor2) {\n"
      "  boolean exer = is_exer(flavor);\n"
      "  if (!exer || (!(xd.xer_bits & UNTAGGED) && !(flavor & (USE_NIL|(exer ? XER_LIST : XER_RECOF)))%s)) "
      /* If the union has no own tag, there is nothing to check. */
      "return check_name(name, xd, exer)" /* if false, return immediately */
      " && (!exer || (flavor & USE_TYPE_ATTR) || check_namespace(uri, xd));\n"
      /* else check the ns, unless Basic XER (which has no namespaces, ever)
       * or USE_TYPE (where we only have a name from the type id attribute) */
      , name
      , sdef->xerUseUnion ? " && !(flavor2 & FROM_UNION_USETYPE)" : ""
      );
    src = mputstr(src, "  flavor &= ~XER_RECOF;\n");

    /* An untagged union can start with the start tag of any alternative */
    for (i = 0; i < sdef->nElements; i++) {
      src=mputprintf(src,
        "  if (%s::can_start(name, uri, %s_xer_, flavor, flavor2)) return TRUE;\n"
        , sdef->elements[i].type, sdef->elements[i].typegen
      );
    }
    src = mputstr(src, "  return FALSE;\n}\n\n");

    src = mputprintf(src,
      "char ** %s::collect_ns(const XERdescriptor_t& p_td, size_t& num, boolean& def_ns, unsigned int flavor) const {\n"
      "  size_t num_collected;\n"
      "  char **collected_ns = Base_Type::collect_ns(p_td, num_collected, def_ns, flavor);\n"
      /* Two-level new memory allocated */
      "  char **new_ns;\n"
      "  size_t num_new;\n"
      "  boolean need_type = FALSE;\n"
      "  try {\n"
      "    boolean def_ns_1 = FALSE;\n"
      "    switch (union_selection) {\n"
      , name
      );
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src,
        "    case %s_%s:\n"
        "      new_ns = field_%s->collect_ns(%s_xer_, num_new, def_ns_1, flavor);\n"
        "      def_ns = def_ns || def_ns_1;\n" /* alas, no ||= */
        "      merge_ns(collected_ns, num_collected, new_ns, num_new);\n"
        /* merge_ns() deallocated new_ns and duplicated strings,
         * copied the new ones into the expanded new_ns */
        , selection_prefix, sdef->elements[i].name
        , sdef->elements[i].name
        , sdef->elements[i].typegen
      );
      /* Type id attribute not needed for the first field in case of USE-TYPE */
      // TODO: if USE-UNION there is no need to check the namelens
      if (sdef->xerUseUnion || i > 0) src = mputprintf(src,
          "      need_type = (%s_xer_.namelens[1] > 2) || %s_xer_.xsd_type != XSD_NONE;\n"
          , sdef->elements[i].typegen, sdef->elements[i].typegen);
      src = mputstr(src, "      break;\n");
    }

    src = mputstr(src,
      "    default: break;\n"
      "    }\n" /* switch */
      "    if ((p_td.xer_bits & USE_TYPE_ATTR) && !(p_td.xer_bits & XER_ATTRIBUTE) && need_type) {\n"
      /*     control ns for type attribute */
      "      new_ns = (char**)Malloc(sizeof(char*));\n"
      "      num_new = 1;\n"
      "      const namespace_t *c_ns = p_td.my_module->get_controlns();\n"
      "      new_ns[0] = mprintf(\" xmlns:%s='%s'\", c_ns->px, c_ns->ns);\n"
      "      merge_ns(collected_ns, num_collected, new_ns, num_new);\n"
      "    }\n"
      "  }\n"
      "  catch (...) {\n"
      /* Probably a TC_Error thrown from field_%s->collect_ns() if e.g.
       * encoding an unbound value. */
      "    while (num_collected > 0) Free(collected_ns[--num_collected]);\n"
      "    Free(collected_ns);\n"
      "    throw;\n"
      "  }\n"
      "  num = num_collected;\n"
      "  return collected_ns;\n"
      "}\n\n"
      );

    src = mputprintf(src, /* XERSTUFF XER_encode for union */
      "int %s::XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf, "
      "unsigned int p_flavor, unsigned int p_flavor2, int p_indent, embed_values_enc_struct_t*) const\n"
      "{\n"
      "%s"
      "  if (%s==union_selection) {\n"
      "    TTCN_error(\"Attempt to XER-encode an unbound union value.\");\n"
      "    return 0;\n"
      "  }\n"
      "  TTCN_EncDec_ErrorContext ec_0(\"Alternative '\");\n"
      "  TTCN_EncDec_ErrorContext ec_1;\n"
      "  int encoded_length=(int)p_buf.get_len();\n"
      , name
      , (use_runtime_2 ? "  if (err_descr) return XER_encode_negtest"
        "(err_descr, p_td, p_buf, p_flavor, p_flavor2, p_indent, 0);\n" : "")
      , unbound_value
    );

    if (sdef->xerUseTypeAttr) {
      src = mputstr(src,
        "  const boolean e_xer = is_exer(p_flavor);\n"
        "  boolean need_schema = FALSE;\n"
        "  char *schema_prefix = NULL;\n"
        "  char *type_atr = NULL;\n"
        "  if (e_xer && (p_td.xer_bits & USE_TYPE_ATTR)) {\n"
        "    char *type_name = 0;\n"
        "    const namespace_t *control_ns;\n"
        "    schema_prefix = mprintf(\"xsd\");\n"
        "    int counter = 0;\n"
        "    // Find a unique prefix for the xsd schema\n"
        "    while (1) {\n"
        "      boolean changed = FALSE;\n"
        "      for (size_t i = 0; i < p_td.my_module->get_num_ns(); i++) {\n"
        "        if (p_td.my_module->get_ns(i)->px != NULL &&\n"
        "            strcmp(p_td.my_module->get_ns(i)->px, schema_prefix) == 0) {\n"
        "          Free(schema_prefix);\n"
        "          schema_prefix = mprintf(\"xsd%i\", counter);\n"
        "          counter++;\n"
        "          changed = TRUE;\n"
        "          break; // new maybe unique prefix found\n"
        "        }\n"
        "      }\n"
        "      if (!changed) {\n"
        "        break; //break when a unique prefix found\n"
        "      }\n"
        "    }\n"
        "    switch (union_selection) {\n");
      /* In case of USE-TYPE the first field won't need the type attribute */
      int start_at = sdef->xerUseUnion ? 0 : 1;
      for (i = start_at; i < sdef->nElements; i++) {
        src = mputprintf(src,
          "    case %s_%s:\n"
          , selection_prefix, sdef->elements[i].name);
        // UseType and not UseUnion on field
        if (!sdef->xerUseUnion && !sdef->elements[i].xerUseUnion) {
          src = mputprintf(src,
            "      if (%s_xer_.my_module != 0 && %s_xer_.ns_index != -1 &&\n"
            "          %s_xer_.namelens[1] > 2) {\n"
            /* add the namespace prefix to the type attribute (if the name is not empty) */
            "        const namespace_t *my_ns = %s_xer_.my_module->get_ns(%s_xer_.ns_index);\n"
            "        if (my_ns->px[0] != 0) {\n"
            "          type_name = mprintf(\"%%s:\", my_ns->px);\n"
            "        }\n"
            "      }\n"
            "      type_name = mputstrn(type_name, %s_xer_.names[1], %s_xer_.namelens[1] - 2);\n"
            , sdef->elements[i].typegen, sdef->elements[i].typegen
            , sdef->elements[i].typegen, sdef->elements[i].typegen
            , sdef->elements[i].typegen, sdef->elements[i].typegen
            , sdef->elements[i].typegen);
        }
        if (sdef->elements[i].xsd_type != XSD_NONE) {
          src = mputprintf(src,
            "      if (type_name == NULL) {\n"
            "        type_name = mputprintf(type_name, \"%%s:%s\", schema_prefix);\n"
            "        need_schema = TRUE;\n"
            "      }\n"
            , XSD_type_to_xml_type(sdef->elements[i].xsd_type));
        }
        src = mputprintf(src,
          "      %s\n"
          , i < sdef->nElements - 1 ? "goto write_atr;" : "" /* no break */
        );
      }
      src = mputprintf(src,
        "%s" /* label only if more than two elements total */
        "      if (mstrlen(type_name) > 0) {\n"
        "        control_ns = p_td.my_module->get_controlns();\n"
        "        type_atr = mcopystr(\" \");\n"
        "        if (need_schema) {\n"
        "          type_atr = mputprintf(type_atr, \"xmlns:%%s=\'http://www.w3.org/2001/XMLSchema\' \", schema_prefix);\n"
        "        }\n"
        "        type_atr = mputstr(type_atr, control_ns->px);\n"
        "        type_atr = mputstr(type_atr, \":type='\");\n"
        "        type_atr = mputstr(type_atr, type_name);\n"
        "        type_atr = mputc  (type_atr, '\\'');\n"
        "        Free(type_name);\n"
        "      }\n"
        "      break;\n"
        "    default: break;\n"
        "    }\n" /* switch */
        "  p_flavor &= ~XER_RECOF;\n"
        "  Free(schema_prefix);\n"
        "  }\n" /* if e_xer */
        , (sdef->nElements > start_at + 1 ? "write_atr:\n" : "")


        );
    } /* if UseTypeAttr */
    src = mputprintf(src,
      "  unsigned int flavor_1 = p_flavor;\n"
      "  unsigned int flavor_2 = p_flavor2;\n"
      "  if (is_exer(p_flavor)) flavor_1 &= ~XER_RECOF;\n"
      "  if (!(p_flavor & XER_LIST)) flavor_2 |= FROM_UNION_USETYPE;\n"
      "  boolean omit_tag = begin_xml(p_td, p_buf, flavor_1, p_indent, FALSE, "
      "(collector_fn)&%s::collect_ns%s, flavor_2%s);\n"
      , sdef->name
      , sdef->xerUseTypeAttr ? ", type_atr" : ", 0"
      , legacy_untagged_union == FALSE ? " | THIS_UNION" : "");
    if (legacy_untagged_union == FALSE) {
      // Top level union can be untagged, so don't increase the indentation
      src = mputstr(src,
        "  int p_indent_tmp = (is_exer(p_flavor) && p_indent == 0 && (p_td.xer_bits & UNTAGGED)) ? p_indent : p_indent + (!p_indent || !omit_tag);\n");
    }
    src = mputprintf(src,
      "  unsigned int flavor_0 = (p_flavor & XER_MASK)%s;\n"
      "  switch (union_selection) {\n"
      , sdef->xerUseTypeAttr ? " | USE_TYPE_ATTR" : "");
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src, "  case %s_%s:\n"
	"    ec_1.set_msg(\"%s': \");\n"
	"    field_%s->XER_encode(%s_xer_, p_buf, flavor_0, "
	"flavor_2, p_indent%s, 0);\n"
	"    break;\n",
	selection_prefix, sdef->elements[i].name,
	sdef->elements[i].dispname,
	sdef->elements[i].name, sdef->elements[i].typegen,
  legacy_untagged_union == FALSE ? "_tmp" : " + (!p_indent || !omit_tag)");
    }
    src = mputprintf(src, "  case %s:\n"
      "    (void)flavor_0;\n" /* warning reduction for empty union */
      "    break;\n"
      "  } //switch\n"
      , unbound_value);
    if (sdef->xerUseTypeAttr) {
      src = mputstr(src, "  if (p_buf.get_data()[p_buf.get_len()-1] != '\\n') flavor_1 |= SIMPLE_TYPE;\n");
    }
    src = mputprintf(src,
      "  end_xml(p_td, p_buf, flavor_1, p_indent, 0, flavor_2%s);\n"
      "  return (int)p_buf.get_len() - encoded_length;\n"
      "}\n\n"
      , legacy_untagged_union == FALSE ? " | THIS_UNION" : "");
      
    if (use_runtime_2) {
      def = mputstr(def,
        "int XER_encode_negtest(const Erroneous_descriptor_t* p_err_descr, "
        "const XERdescriptor_t& p_td, TTCN_Buffer& p_buf, "
        "unsigned int p_flavor, unsigned int p_flavor2, int p_indent, embed_values_enc_struct_t*) const;\n");
      src = mputprintf(src, /* XERSTUFF XER_encode for union */
        "int %s::XER_encode_negtest(const Erroneous_descriptor_t* p_err_descr, "
        "const XERdescriptor_t& p_td, TTCN_Buffer& p_buf, "
        "unsigned int p_flavor, unsigned int p_flavor2, int p_indent, embed_values_enc_struct_t*) const\n"
        "{\n"
        "  if (%s==union_selection) {\n"
        "    TTCN_error(\"Attempt to XER-encode an unbound union value.\");\n"
        "    return 0;\n"
        "  }\n"
        "  TTCN_EncDec_ErrorContext ec_0(\"Alternative '\");\n"
        "  TTCN_EncDec_ErrorContext ec_1;\n"
        "  int encoded_length=(int)p_buf.get_len();\n"
        , name
        , unbound_value
        );
      if (sdef->xerUseTypeAttr) {
        src = mputstr(src,
          "  const boolean e_xer = is_exer(p_flavor);\n"
          "  boolean need_schema = FALSE;\n"
          "  char *schema_prefix = NULL;\n"
          "  char *type_atr = NULL;\n"
          "  if (e_xer && (p_td.xer_bits & USE_TYPE_ATTR)) {\n"
          "    char *type_name = 0;\n"
          "    const namespace_t *control_ns;\n"
          "    schema_prefix = mprintf(\"xsd\");\n"
          "    int counter = 0;\n"
          "    // Find a unique prefix for the xsd schema\n"
          "    while (1) {\n"
          "      boolean changed = FALSE;\n"
          "      for (size_t i = 0; i < p_td.my_module->get_num_ns(); i++) {\n"
          "        if (p_td.my_module->get_ns(i)->px != NULL &&\n"
          "            strcmp(p_td.my_module->get_ns(i)->px, schema_prefix) == 0) {\n"
          "          Free(schema_prefix);\n"
          "          schema_prefix = mprintf(\"xsd%i\", counter);\n"
          "          counter++;\n"
          "          changed = TRUE;\n"
          "          break; // new maybe unique prefix found\n"
          "        }\n"
          "      }\n"
          "      if (!changed) {\n"
          "        break; //break when a unique prefix found\n"
          "      }\n"
          "    }\n"
          "    switch (union_selection) {\n");
        int start_at = sdef->xerUseUnion ? 0 : 1;
        for (i = start_at; i < sdef->nElements; i++) {
          src = mputprintf(src,
            "    case %s_%s:\n"
            , selection_prefix, sdef->elements[i].name);
          if (!sdef->xerUseUnion) { // UseType
            src = mputprintf(src,
              "      if (%s_xer_.my_module != 0 && %s_xer_.ns_index != -1 &&\n"
              "          %s_xer_.namelens[1] > 2) {\n"
              /* add the namespace prefix to the type attribute (if the name is not empty) */
              "        const namespace_t *my_ns = %s_xer_.my_module->get_ns(%s_xer_.ns_index);\n"
              "        if (my_ns->px[0] != 0) {\n"
              "          type_name = mprintf(\"%%s:\", my_ns->px);\n"
              "        }\n"
              "      }\n"
              "      type_name = mputstrn(type_name, %s_xer_.names[1], %s_xer_.namelens[1] - 2);\n"
              , sdef->elements[i].typegen, sdef->elements[i].typegen
              , sdef->elements[i].typegen, sdef->elements[i].typegen
              , sdef->elements[i].typegen, sdef->elements[i].typegen
              , sdef->elements[i].typegen);
          }
          if (sdef->elements[i].xsd_type != XSD_NONE) {
            src = mputprintf(src,
              "      if (type_name == NULL) {\n"
              "        type_name = mputprintf(type_name, \"%%s:%s\", schema_prefix);\n"
              "        need_schema = TRUE;\n"
              "      }\n"
              , XSD_type_to_xml_type(sdef->elements[i].xsd_type));
          }
          src = mputprintf(src,
            "      %s\n"
            , i < sdef->nElements - 1 ? "goto write_atr;" : "" /* no break */
          );
        }
        src = mputprintf(src,
          "%s" /* label only if more than two elements total */
          "      if (mstrlen(type_name) > 0) {\n" /* 38.3.8, no atr if NAME AS "" */
          "        control_ns = p_td.my_module->get_controlns();\n"
          "        type_atr = mcopystr(\" \");\n"
          "        if (need_schema) {\n"
          "          type_atr = mputprintf(type_atr, \"xmlns:%%s=\'http://www.w3.org/2001/XMLSchema\' \", schema_prefix);\n"
          "        }\n"
          "        type_atr = mputstr(type_atr, control_ns->px);\n"
          "        type_atr = mputstr(type_atr, \":type='\");\n"
          "        type_atr = mputstr(type_atr, type_name);\n"
          "        type_atr = mputc  (type_atr, '\\'');\n"
          "        Free(type_name);\n"
          "      }\n"
          "      break;\n"
          "    default: break;\n"
          "    }\n" /* switch */
          "  p_flavor &= ~XER_RECOF;\n"
          "  Free(schema_prefix);\n"
          "  }\n" /* if e_xer */
          , (sdef->nElements > start_at + 1 ? "write_atr:\n" : "")
          );
      } /* if UseTypeAttr */

      src = mputprintf(src,
        "  boolean omit_tag = begin_xml(p_td, p_buf, p_flavor, p_indent, FALSE, "
        "(collector_fn)&%s::collect_ns%s);\n"
        , sdef->name
        , sdef->xerUseTypeAttr ? ", type_atr" : "");
      /* begin_xml will Free type_atr */
      src = mputprintf(src,
        "  unsigned int flavor_0 = (p_flavor & XER_MASK)%s;\n"
        "  const Erroneous_values_t* err_vals = NULL;\n"
        "  const Erroneous_descriptor_t* emb_descr = NULL;\n"
        "  switch (union_selection) {\n"
        , sdef->xerUseTypeAttr ? " | USE_TYPE_ATTR" : "");
      for (i = 0; i < sdef->nElements; i++) {
        src = mputprintf(src, "  case %s_%s:\n"
          "    err_vals = p_err_descr->get_field_err_values(%lu);\n"
          "    emb_descr = p_err_descr->get_field_emb_descr(%lu);\n"
          "    if (err_vals && err_vals->value) {\n"
          "      if (err_vals->value->errval) {\n"
          "        ec_1.set_msg(\"%s'(erroneous value): \");\n"
          "        if (err_vals->value->raw) {\n"
          "          err_vals->value->errval->encode_raw(p_buf);\n"
          "        } else {\n"
          "          if (err_vals->value->type_descr==NULL) TTCN_error"
          "(\"internal error: erroneous value typedescriptor missing\");\n"
          "          else err_vals->value->errval->XER_encode("
          "*err_vals->value->type_descr->xer, p_buf, flavor_0, "
          "p_flavor2, p_indent + (!p_indent || !omit_tag), 0);\n"
          "        }\n"
          "      }\n"
          "    } else {\n"
          "      ec_1.set_msg(\"%s': \");\n"
          "      if (emb_descr) field_%s->XER_encode_negtest(emb_descr, "
          "%s_xer_, p_buf, flavor_0, p_flavor2%s, p_indent + (!p_indent || !omit_tag), 0);\n"
          "      else field_%s->XER_encode(%s_xer_, p_buf, flavor_0, "
          "p_flavor2, p_indent + (!p_indent || !omit_tag), 0);\n"
          "    }\n"
          "    break;\n",
          selection_prefix, sdef->elements[i].name, /* case label */
          (unsigned long)i, (unsigned long)i,
          sdef->elements[i].dispname, /* set_msg (erroneous) */
          sdef->elements[i].dispname, /* set_msg */
          sdef->elements[i].name,
          sdef->elements[i].typegen,
          sdef->xerUseTypeAttr && !sdef->xerUseUnion ? "| FROM_UNION_USETYPE" : "",
          sdef->elements[i].name, sdef->elements[i].typegen /* field_%s, %s_descr */
          );
      }
      src = mputprintf(src, "  case %s:\n"
        "    (void)flavor_0;\n" /* warning reduction for empty union */
        "    break;\n"
        "  } //switch\n"
        "  end_xml(p_td, p_buf, p_flavor, p_indent, 0);\n"
        "  return (int)p_buf.get_len() - encoded_length;\n"
        "}\n\n"
        , unbound_value);
    }

#ifndef NDEBUG
  src = mputprintf(src, "// %s has%s%s%s%s%s%s%s%s%s\n"
    "// written by %s in " __FILE__ " at %d\n"
    , name
    , (sdef->xerUntagged            ? " UNTAGGED" : "")
    , (sdef->xerUntaggedOne         ? "1" : "")
    , (sdef->xerUseNilPossible      ? " USE_NIL?" : "")
    , (sdef->xerUseOrderPossible    ? " USE_ORDER?" : "")
    , (sdef->xerUseQName            ? " USE_QNAME" : "")
    , (sdef->xerUseTypeAttr         ? " USE_TYPE_ATTR" : "")
    , (sdef->xerUseUnion            ? " USE-UNION" : "")
    , (sdef->xerHasNamespaces       ? " namespace" : "")
    , (sdef->xerEmbedValuesPossible ? " EMBED?" : "")
    , __FUNCTION__, __LINE__
  );
#endif
    src = mputprintf(src, /* XERSTUFF decoder functions for union */
      "int %s::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& p_reader,"
      " unsigned int p_flavor, unsigned int p_flavor2, embed_values_dec_struct_t*)\n"
      "{\n"
      "  boolean e_xer = is_exer(p_flavor);\n"
      "  int type = 0;\n" /* None */
      "  int rd_ok=1, xml_depth=-1;\n"
      "%s%s"
      "  unsigned long xerbits = p_td.xer_bits;\n"
      "%s"
      "  if (xerbits & USE_TYPE_ATTR) p_flavor &= ~XER_RECOF;\n"
      "  boolean own_tag = !(e_xer && ((xerbits & (ANY_ELEMENT | UNTAGGED)) "
      "|| (p_flavor & (USE_NIL|(e_xer ? XER_LIST : XER_RECOF)))));\n"
      "  int other_attributes = 0;\n"
      "  (void)other_attributes;\n"
      "  if ((e_xer || !is_record_of(p_flavor)) && own_tag)\n"
      /* Loop until the start tag */
      "  %sfor (rd_ok = p_reader.Ok(); rd_ok == 1; rd_ok = p_reader.Read()) {\n"
      "    type = p_reader.NodeType();\n"
      "    if (type == XML_READER_TYPE_ELEMENT%s) {\n"
      "      %sverify_name(p_reader, p_td, e_xer);\n"
      "      xml_depth = p_reader.Depth();\n"
      , name
      , sdef->xerUseTypeAttr ? "  const char * typeatr = 0;\n" : ""
      , sdef->xerUseUnion ? "  boolean attribute = (p_td.xer_bits & XER_ATTRIBUTE) ? TRUE : FALSE;\n" : ""
      , legacy_untagged_union ? "  if (p_flavor & XER_TOPLEVEL) xerbits &= ~UNTAGGED;\n" : ""
      , sdef->xerUseUnion ? "if (!attribute) " : ""
      , sdef->xerUseUnion ? " || (p_flavor & USE_TYPE_ATTR)" : ""
      , sdef->xerUseUnion ? "if (!(p_flavor & USE_TYPE_ATTR)) " : ""
    );
    if (sdef->xerUseTypeAttr) {
      src = mputprintf(src,
        "      if (e_xer) {\n"
        "        for (rd_ok = p_reader.MoveToFirstAttribute(); rd_ok == 1;"
        " rd_ok = p_reader.MoveToNextAttribute()) {\n"
        "          if (p_reader.IsNamespaceDecl()) continue;\n"
        "          const char *attr_name = (const char*)p_reader.Name();\n"
        "          if (!strcmp(attr_name, \"%s:type\"))\n"
        "          {\n"
        "            typeatr = (const char*)p_reader.Value();\n"
        "          }\n"
        "          else ++other_attributes;\n"
        "        }\n" /* for */
        "%s"
        , sdef->control_ns_prefix
        , sdef->xerUseUnion ? "        rd_ok = p_reader.MoveToElement() | 1;\n" : "");
      src = mputstr(src,
        "      }\n" /* if exer */);
    }

    src = mputprintf(src,
      "      if (%s(p_td.xer_bits & USE_TYPE_ATTR)%s)\n"
      "        && !p_reader.IsEmptyElement()) { rd_ok = p_reader.Read(); }\n"
      "      break;\n"
      "    }\n"
      "  }\n"
      "  unsigned int flavor_1 = (p_flavor & XER_MASK);\n" /* also, not toplevel */
      /* Loop until the content. Normally an element, unless UNTAGGED
       * USE-UNION or some other shenanigans. */
      "  %s%sfor (rd_ok = p_reader.Ok(); rd_ok == 1; rd_ok = p_reader.Read()) {\n"
      "    type = p_reader.NodeType();\n"
      "    %sif (type == XML_READER_TYPE_ELEMENT) break;\n"
      "    else if (type == XML_READER_TYPE_END_ELEMENT) break;\n"
      "  }\n"
      "  if (%s) {\n"
      "    TTCN_EncDec_ErrorContext ec_1(\"Alternative '\");\n"
      "    TTCN_EncDec_ErrorContext ec_2;\n"
      "    const char *elem_name;\n"
      "    const char *ns_uri = 0;\n"
      , sdef->xerUseTypeAttr && !sdef->xerUseUnion ? "!e_xer && !(" : "!(e_xer && "
      , sdef->xerUseTypeAttr ? " && other_attributes > 0" : ""
      , sdef->xerUseTypeAttr ? "if (!e_xer) " : ""
      , sdef->xerUseUnion ? "if (!attribute) " : ""
      , sdef->xerUseTypeAttr ?
        "if (e_xer) { if (type == XML_READER_TYPE_TEXT) break; }\n"
        "    else " : ""
      , sdef->xerUseTypeAttr && !sdef->xerUseUnion ? "TRUE" : "rd_ok"
    );

    if (sdef->xerUseTypeAttr) {
      src = mputstr(src,
        "    if (e_xer) {\n" /* USE-TYPE => no XML element, use typeatr */
        "      char* typeatr_cpy = mcopystr(typeatr);\n" /* copy the string so it can be tokenized */
        "      char *token_1 = strtok(typeatr_cpy, \":\");\n" /* extract the namespace (if any) */
        "      char *token_2 = strtok(NULL, \":\");\n"
        "      if (token_2) {\n" /* namespace found */
        "        elem_name = typeatr + (token_2 - typeatr_cpy);\n" /* save the token's address in the original string, not in the copy */
        "        ns_uri = get_ns_uri_from_prefix(token_1, p_td);\n"
        "      }\n"
        "      else {\n" /* no namespace */
        "        elem_name = typeatr;\n"
        "      }\n"
        "      Free(typeatr_cpy);\n"
        "      flavor_1 |= USE_TYPE_ATTR;\n"
        "    }\n"
        "    else" /* no newline, gobbles up the next {} */);
    }
    src = mputstr(src,
      "    {\n"
      "      elem_name = (const char*)p_reader.LocalName();\n"
      "      ns_uri    = (const char*)p_reader.NamespaceUri();\n"
      "    }\n");

    if (sdef->xerUseUnion) {
      src = mputstr(src,
        "    int matched = -1;\n"
        "    if (typeatr == NULL) flavor_1 |= EXIT_ON_ERROR;\n"
        "    for (;;) {\n");
      /* TODO unify the two alternatives */
      for (i = 0; i < sdef->nElements; i++) {
        src = mputprintf(src,
          /* If decoding EXER, there is no tag to apply check_name
           *
           */
          "    if ((e_xer && (typeatr == NULL || !(p_td.xer_bits & USE_TYPE_ATTR))) "
          "|| can_start(elem_name, ns_uri, %s_xer_, flavor_1, p_flavor2)%s%s%s) {\n"
          "      ec_2.set_msg(\"%s': \");\n"
          "      if (%s==union_selection) {\n"
          "        matched = %d;\n"
          "        %s().XER_decode(%s_xer_, p_reader, flavor_1, p_flavor2, 0);\n"
          "      }\n"
          "      if (field_%s->is_bound()) break; else clean_up();\n"
          "    }\n",
          sdef->elements[i].typegen,
          sdef->elements[i].xsd_type != XSD_NONE ? " || strcmp(elem_name, \"" : "",
          sdef->elements[i].xsd_type != XSD_NONE ? XSD_type_to_xml_type(sdef->elements[i].xsd_type) : "",
          sdef->elements[i].xsd_type != XSD_NONE ? "\") == 0" : "",
          sdef->elements[i].dispname,
          unbound_value, (int)i,
          sdef->elements[i].name,    sdef->elements[i].typegen,
          sdef->elements[i].name);
      }
      src = mputstr(src,
        "    if (typeatr == NULL) {\n"
        /* No type attribute found and none of the fields managed to decode the value (USE-UNION only) */
        "      ec_1.set_msg(\" \");\n"
        "      ec_2.set_msg(\" \");\n"
        "      const char* value = (const char*)p_reader.Value();\n"
        "      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,\n"
        "        \"'%s' could not be decoded by any of the union's fields.\", value);\n"
        "    } else if (matched >= 0) {\n"
        /* Alternative found, but it couldn't be decoded */
        "      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,\n"
        "        \"Failed to decode field.\");\n"
        "    } else {\n"
        /* Type attribute found, but it didn't match any of the fields */
        "      ec_1.set_msg(\" \");\n"
        "      ec_2.set_msg(\" \");\n"
        "      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,\n"
        "        \"'%s' does not match any alternative.\", elem_name);\n"
        "    }\n"
        "    if (xml_depth >= 0) for (; rd_ok == 1 "
        "&& p_reader.Depth() > xml_depth; rd_ok = p_reader.Read()) ;\n"
        "    break;\n"
        "    }\n"); /* for ever */
    }
    else /* not USE-UNION */
    {
      src = mputstr(src,
        "    unsigned int flavor2 = p_flavor2;\n");
      if (sdef->xerUseTypeAttr) {
        src = mputstr(src,
          "    if (e_xer && (p_td.xer_bits & USE_TYPE_ATTR)) {\n"
          "      flavor2 |= FROM_UNION_USETYPE;\n"
          "    }\n");
      }
      if (sdef->exerMaybeEmptyIndex >= 0) {
        /* There is a field which can be empty XML. Add code to detect this
         * and jump to the appropriate field which can decode "nothing". */
        src = mputstr(src,
          "    if (type!=XML_READER_TYPE_ELEMENT || "
          "(own_tag && p_reader.IsEmptyElement())) goto empty_xml;\n");
        /* If the choice itself is untagged, then an empty element belongs to
         * one of the fields and does not mean that the choice is empty */
      }
      /* FIXME some hashing should be implemented */
      /* The field which can be empty should be checked last, otherwise we create an infinity loop with memory bomb.
       * So move that field to the last elseif branch*/
      for (i = 0; i < sdef->nElements; i++) {
        if(sdef->exerMaybeEmptyIndex != i) {
          src = mputprintf(src,
            "    %sif (%s%s::can_start(elem_name, ns_uri, %s_xer_, flavor_1, flavor2) || (%s_xer_.xer_bits & ANY_ELEMENT)%s%s%s%s) {\n"
            "      ec_2.set_msg(\"%s': \");\n"
            "      if (e_xer && (%s_xer_.xer_bits & BLOCKED)) {\n"
            "        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,\n"
            "          \"Attempting to decode blocked or abstract field.\");\n"
            "      }\n",
            i && !(i==1 && sdef->exerMaybeEmptyIndex==0) ? "else " : "",  /*  print "if(" if generate code for the first field or if the first field is the MaybeEmpty field and we generate the code for the second one*/
            i == 0 && sdef->xerUseTypeAttr ? "(e_xer && typeatr == NULL) || " : "", /* USE-TYPE: No type attribute means the first alternative */
            sdef->elements[i].type, sdef->elements[i].typegen, sdef->elements[i].typegen,
            sdef->elements[i].xsd_type != XSD_NONE ? " || strcmp(elem_name, \"" : "",
            sdef->elements[i].xsd_type != XSD_NONE ? XSD_type_to_xml_type(sdef->elements[i].xsd_type) : "",
            sdef->elements[i].xsd_type != XSD_NONE ? "\") == 0" : "",
            sdef->elements[i].xsd_type == XSD_NONE && sdef->elements[i].xerUseUnion ? " || (flavor2 & FROM_UNION_USETYPE)" : "",
            sdef->elements[i].dispname,
            sdef->elements[i].typegen);
          if (sdef->xerUseTypeAttr) {
            src = mputprintf(src,
              "      if (e_xer && !(%s_xer_.xer_bits & USE_TYPE_ATTR)) {\n"
              "        rd_ok = p_reader.MoveToElement() | 1;\n"
              "      }\n",
              sdef->elements[i].typegen);
          }
          if (!sdef->elements[i].xerUseUnion && sdef->xerUseTypeAttr) {
            src = mputprintf(src,
              "      if (e_xer && !((p_td.xer_bits & USE_TYPE_ATTR) && other_attributes > 0)\n"
              "        && !p_reader.IsEmptyElement()) { rd_ok = p_reader.Read(); }\n");
          }
          src = mputprintf(src,
            "      %s%s().XER_decode(%s_xer_, p_reader, flavor_1, flavor2, 0);\n"
            "      if (!%s%s().is_bound()) {\n"
            "        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG, \"Failed to decode field.\");\n"
            "      }\n"
            "    }\n",
            at_field, sdef->elements[i].name,
            sdef->elements[i].typegen,
            at_field, sdef->elements[i].name);
          }
      }
      if(sdef->exerMaybeEmptyIndex>=0 ){
        i=sdef->exerMaybeEmptyIndex;
        src = mputprintf(src,
          "    %sif (%s(e_xer && (type==XML_READER_TYPE_END_ELEMENT || !own_tag)) || %s::can_start(elem_name, ns_uri, %s_xer_, flavor_1, flavor2) || (%s_xer_.xer_bits & ANY_ELEMENT)%s%s%s) {\n"
          "empty_xml:  ec_2.set_msg(\"%s': \");\n"
          "      if (e_xer && (%s_xer_.xer_bits & BLOCKED)) {\n"
          "        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,\n"
          "          \"Attempting to decode blocked or abstract field.\");\n"
          "      }\n"
          "      if (e_xer && !(%s_xer_.xer_bits & USE_TYPE_ATTR)) {\n"
          "        rd_ok = p_reader.MoveToElement() | 1;\n"
          "      }\n"
          "      %s%s().XER_decode(%s_xer_, p_reader, flavor_1, flavor2, 0);\n"
          "      if (!%s%s().is_bound()) {\n"
          "        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG, \"Failed to decode field.\");\n"
          "      }\n"
          "    }\n",
          sdef->nElements>0 ? "else " : "",
          i == 0 && sdef->xerUseTypeAttr  ? "(e_xer && typeatr == NULL) || " : "", /* USE-TYPE: No type attribute means the first alternative */
          sdef->elements[i].type, sdef->elements[i].typegen, sdef->elements[i].typegen,
          sdef->elements[i].xsd_type != XSD_NONE ? " || strcmp(elem_name, \"" : "",
          sdef->elements[i].xsd_type != XSD_NONE ? XSD_type_to_xml_type(sdef->elements[i].xsd_type) : "",
          sdef->elements[i].xsd_type != XSD_NONE ? "\") == 0" : "",
          sdef->elements[i].dispname,
          sdef->elements[i].typegen,
          sdef->elements[i].typegen,
          at_field, sdef->elements[i].name,    sdef->elements[i].typegen,
          at_field, sdef->elements[i].name);
      }
      if (!sdef->isOptional) {
        src = mputstr(src,
          "    else {\n"
          "      ec_1.set_msg(\" \");\n"
          "      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG, "
          "\"'%s' does not match any alternative\", elem_name);\n"
          "      if (xml_depth >= 0) for (; rd_ok == 1 "
          "&& p_reader.Depth() > xml_depth; rd_ok = p_reader.Read()) ;\n"
          "    }\n"
          );
      }
    }

    src = mputprintf(src,
      "  }\n" /* end if(rd_ok) */
      "  if (%s(e_xer || !is_record_of(p_flavor)) && own_tag && !(p_flavor2 & FROM_UNION_USETYPE))\n"
      "  for (; rd_ok == 1; rd_ok = p_reader.Read()) {\n"
      "    type = p_reader.NodeType();\n"
      "    if (type == XML_READER_TYPE_END_ELEMENT) {\n"
      "      verify_end(p_reader, p_td, xml_depth, e_xer);\n"
      "      rd_ok = p_reader.Read(); // one last time\n"
      "      break;\n"
      "    }\n"
      "  }\n"
      "  return 1;\n"
      "}\n\n", sdef->xerUseUnion ? "!attribute && " : "");
  }
  if (json_needed) {
    // JSON encode
    src = mputprintf(src,
      "int %s::JSON_encode(const TTCN_Typedescriptor_t&%s, JSON_Tokenizer& p_tok) const\n"
      "{\n", name, (use_runtime_2 || !sdef->jsonAsValue) ? " p_td" : "");
    if (use_runtime_2) {
      src = mputstr(src, "  if (err_descr) return JSON_encode_negtest(err_descr, p_td, p_tok);\n");
    }
    if (!sdef->jsonAsValue) {
      // 'as value' is not set for the base type, but it might still be set in
      // the type descriptor
      src = mputstr(src, 
        "  boolean as_value = NULL != p_td.json && p_td.json->as_value;\n"
        "  int enc_len = as_value ? 0 : "
        "p_tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL);\n");
    } else {
      src = mputstr(src, "  int enc_len = 0;\n");
    }
    src = mputstr(src, "  switch(union_selection) {\n");
      
    for (i = 0; i < sdef->nElements; ++i) {
      src = mputprintf(src, "  case %s_%s:\n", selection_prefix, sdef->elements[i].name);
      if (!sdef->jsonAsValue) {
        src = mputprintf(src,
          "    if (!as_value) {\n"
          "      enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, \"%s\");\n"
          "    }\n"
          , sdef->elements[i].jsonAlias ? sdef->elements[i].jsonAlias : sdef->elements[i].dispname);
      }
      src = mputprintf(src, 
	      "    enc_len += field_%s->JSON_encode(%s_descr_, p_tok);\n"
	      "    break;\n"
        , sdef->elements[i].name, sdef->elements[i].typedescrname);
    }
    src = mputprintf(src, 
      "  default:\n"
      "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND, \n"
	    "      \"Encoding an unbound value of type %s.\");\n"
      "    return -1;\n"
      "  }\n\n"
      , dispname);
    if (!sdef->jsonAsValue) {
      src = mputstr(src,
        "  if (!as_value) {\n"
        "    enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);\n"
        "  }\n");
    }
    src = mputstr(src,
      "  return enc_len;\n"
      "}\n\n");
    
    if (use_runtime_2) {
      // JSON encode for negative testing
      def = mputstr(def,
        "int JSON_encode_negtest(const Erroneous_descriptor_t*, "
        "const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;\n");
      src = mputprintf(src,
        "int %s::JSON_encode_negtest(const Erroneous_descriptor_t* p_err_descr, "
        "const TTCN_Typedescriptor_t&%s, JSON_Tokenizer& p_tok) const\n"
        "{\n", name, !sdef->jsonAsValue ? " p_td" : "");
      if (!sdef->jsonAsValue) {
        // 'as value' is not set for the base type, but it might still be set in
        // the type descriptor
        src = mputstr(src, 
          "  boolean as_value = NULL != p_td.json && p_td.json->as_value;\n"
          "  int enc_len = as_value ? 0 : "
          "p_tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL);\n");
      } else {
        src = mputstr(src, "  int enc_len = 0;\n\n");
      }
      src = mputstr(src,
        "  const Erroneous_values_t* err_vals = NULL;\n"
        "  const Erroneous_descriptor_t* emb_descr = NULL;\n"
        "  switch(union_selection) {\n");
        
      for (i = 0; i < sdef->nElements; ++i) {
        src = mputprintf(src, 
          "  case %s_%s:\n"
          "    err_vals = p_err_descr->get_field_err_values(%d);\n"
          "    emb_descr = p_err_descr->get_field_emb_descr(%d);\n"
          "    if (NULL != err_vals && NULL != err_vals->value) {\n"
          "      if (NULL != err_vals->value->errval) {\n"
          "        if(err_vals->value->raw){\n"
          "          enc_len += err_vals->value->errval->JSON_encode_negtest_raw(p_tok);\n"
          "        } else {\n"
          "          if (NULL == err_vals->value->type_descr) {\n"
          "            TTCN_error(\"internal error: erroneous value typedescriptor missing\");\n"
          "          }\n"
          , selection_prefix, sdef->elements[i].name, (int)i, (int)i);
        if (!sdef->jsonAsValue) {
          src = mputprintf(src,
            "          if (!as_value) {\n"
            "            enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, \"%s\");\n"
            "          }\n"
            , sdef->elements[i].jsonAlias ? sdef->elements[i].jsonAlias : sdef->elements[i].dispname);
        }
        src = mputstr(src,
          "          enc_len += err_vals->value->errval->JSON_encode(*err_vals->value->type_descr, p_tok);\n"
          "        }\n"
          "      }\n"
          "    } else {\n");
        if (!sdef->jsonAsValue) {
          src = mputprintf(src,
            "      if (!as_value) {\n"
            "        enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, \"%s\");\n"
            "      }\n"
            , sdef->elements[i].jsonAlias ? sdef->elements[i].jsonAlias : sdef->elements[i].dispname);
        }
        src = mputprintf(src,
          "      if (NULL != emb_descr) {\n"
          "        enc_len += field_%s->JSON_encode_negtest(emb_descr, %s_descr_, p_tok);\n"
          "      } else {\n"
          "        enc_len += field_%s->JSON_encode(%s_descr_, p_tok);\n"
          "      }\n"
          "    }\n"
          "    break;\n"
          , sdef->elements[i].name, sdef->elements[i].typedescrname
          , sdef->elements[i].name, sdef->elements[i].typedescrname);
      }
      src = mputprintf(src, 
        "  default:\n"
        "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND, \n"
        "      \"Encoding an unbound value of type %s.\");\n"
        "    return -1;\n"
        "  }\n\n"
        , dispname);
      if (!sdef->jsonAsValue) {
        src = mputstr(src,
          "  if (!as_value) {\n"
          "    enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);\n"
          "}\n");
      }
      src = mputstr(src,
        "  return enc_len;\n"
        "}\n\n");
    }
    
    // JSON decode
    src = mputprintf(src,
      "int %s::JSON_decode(const TTCN_Typedescriptor_t&%s, JSON_Tokenizer& p_tok, boolean p_silent)\n"
      "{\n"
      "  json_token_t j_token = JSON_TOKEN_NONE;\n"
      , name, !sdef->jsonAsValue ? " p_td" : "");
    if (!sdef->jsonAsValue) {
      src = mputstr(src,
        " if (NULL != p_td.json && p_td.json->as_value) {\n");
    }
    src = mputstr(src,
      "  size_t buf_pos = p_tok.get_buf_pos();\n"
      "  p_tok.get_next_token(&j_token, NULL, NULL);\n"
      "  int ret_val = 0;\n"
      "  switch(j_token) {\n"
      "  case JSON_TOKEN_NUMBER: {\n");
    for (i = 0; i < sdef->nElements; ++i) {
      if (JSON_NUMBER & sdef->elements[i].jsonValueType) {
        src = mputprintf(src,
          "    p_tok.set_buf_pos(buf_pos);\n"
          "    ret_val = %s%s().JSON_decode(%s_descr_, p_tok, TRUE);\n"
          "    if (0 <= ret_val) {\n"
          "      return ret_val;\n"
          "    }\n"
          , at_field, sdef->elements[i].name, sdef->elements[i].typedescrname);
      }
    }
    src = mputstr(src,
      "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_AS_VALUE_ERROR, \"number\");\n"
      "    clean_up();\n"
      "    return JSON_ERROR_FATAL;\n"
      "  }\n"
      "  case JSON_TOKEN_STRING: {\n");
    for (i = 0; i < sdef->nElements; ++i) {
      if (JSON_STRING & sdef->elements[i].jsonValueType) {
        src = mputprintf(src,
          "    p_tok.set_buf_pos(buf_pos);\n"
          "    ret_val = %s%s().JSON_decode(%s_descr_, p_tok, TRUE);\n"
          "    if (0 <= ret_val) {\n"
          "      return ret_val;\n"
          "    }\n"
          , at_field, sdef->elements[i].name, sdef->elements[i].typedescrname);
      }
    }
    src = mputstr(src,
      "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_AS_VALUE_ERROR, \"string\");\n"
      "    clean_up();\n"
      "    return JSON_ERROR_FATAL;\n"
      "  }\n"
      "  case JSON_TOKEN_LITERAL_TRUE:\n"
      "  case JSON_TOKEN_LITERAL_FALSE: {\n");
    for (i = 0; i < sdef->nElements; ++i) {
      if (JSON_BOOLEAN & sdef->elements[i].jsonValueType) {
        src = mputprintf(src,
          "    p_tok.set_buf_pos(buf_pos);\n"
          "    ret_val = %s%s().JSON_decode(%s_descr_, p_tok, TRUE);\n"
          "    if (0 <= ret_val) {\n"
          "      return ret_val;\n"
          "    }\n"
          , at_field, sdef->elements[i].name, sdef->elements[i].typedescrname);
      }
    }
    src = mputstr(src,
      "    char* literal_str = mprintf(\"literal (%s)\",\n"
      "      (JSON_TOKEN_LITERAL_TRUE == j_token) ? \"true\" : \"false\");\n"
      "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_AS_VALUE_ERROR, literal_str);\n"
      "    Free(literal_str);\n"
      "    clean_up();\n"
      "    return JSON_ERROR_FATAL;\n"
      "  }\n"
      "  case JSON_TOKEN_ARRAY_START: {\n");
    for (i = 0; i < sdef->nElements; ++i) {
      if (JSON_ARRAY & sdef->elements[i].jsonValueType) {
        src = mputprintf(src,
          "    p_tok.set_buf_pos(buf_pos);\n"
          "    ret_val = %s%s().JSON_decode(%s_descr_, p_tok, TRUE);\n"
          "    if (0 <= ret_val) {\n"
          "      return ret_val;\n"
          "    }\n"
          , at_field, sdef->elements[i].name, sdef->elements[i].typedescrname);
      }
    }
    src = mputstr(src,
      "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_AS_VALUE_ERROR, \"array\");\n"
      "    clean_up();\n"
      "    return JSON_ERROR_FATAL;\n"
      "  }\n"
      "  case JSON_TOKEN_OBJECT_START: {\n");
    for (i = 0; i < sdef->nElements; ++i) {
      if (JSON_OBJECT & sdef->elements[i].jsonValueType) {
        src = mputprintf(src,
          "    p_tok.set_buf_pos(buf_pos);\n"
          "    ret_val = %s%s().JSON_decode(%s_descr_, p_tok, TRUE);\n"
          "    if (0 <= ret_val) {\n"
          "      return ret_val;\n"
          "    }\n"
          , at_field, sdef->elements[i].name, sdef->elements[i].typedescrname);
      }
    }
    src = mputstr(src,
      "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_AS_VALUE_ERROR, \"object\");\n"
      "    clean_up();\n"
      "    return JSON_ERROR_FATAL;\n"
      "  }\n"
      "  case JSON_TOKEN_LITERAL_NULL: {\n");
    for (i = 0; i < sdef->nElements; ++i) {
      if (JSON_NULL & sdef->elements[i].jsonValueType) {
        src = mputprintf(src,
          "    p_tok.set_buf_pos(buf_pos);\n"
          "    ret_val = %s%s().JSON_decode(%s_descr_, p_tok, TRUE);\n"
          "    if (0 <= ret_val) {\n"
          "      return ret_val;\n"
          "    }\n"
          , at_field, sdef->elements[i].name, sdef->elements[i].typedescrname);
      }
    }
    src = mputstr(src,
      "    clean_up();\n"
      // the caller might be able to decode the null value if it's an optional field
      // only return an invalid token error, not a fatal error
      "    return JSON_ERROR_INVALID_TOKEN;\n"
      "  }\n"
      "  case JSON_TOKEN_ERROR:\n"
      "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, \"\");\n"
      "    return JSON_ERROR_FATAL;\n"
      "  default:\n"
      "    return JSON_ERROR_INVALID_TOKEN;\n"
      "  }\n"
      "  return ret_val;\n");
    if (!sdef->jsonAsValue) {
      src = mputprintf(src,
        " }\n"
        " else {\n" // if there is no 'as value' set in the type descriptor
        "  size_t dec_len = p_tok.get_next_token(&j_token, NULL, NULL);\n"
        "  if (JSON_TOKEN_ERROR == j_token) {\n"
        "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, \"\");\n"
        "    return JSON_ERROR_FATAL;\n"
        "  }\n"
        "  else if (JSON_TOKEN_OBJECT_START != j_token) {\n"
        "    return JSON_ERROR_INVALID_TOKEN;\n"
        "  }\n\n"
        "  char* fld_name = 0;\n"
        "  size_t name_len = 0;\n"
        "  dec_len += p_tok.get_next_token(&j_token, &fld_name, &name_len);\n"
        "  if (JSON_TOKEN_NAME != j_token) {\n"
        "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_NAME_TOKEN_ERROR);\n"
        "    return JSON_ERROR_FATAL;\n"
        "  } else {\n"
        "    union_selection = %s;\n    "
        , unbound_value);
      for (i = 0; i < sdef->nElements; ++i) {
        src = mputprintf(src,
          "if (0 == strncmp(fld_name, \"%s\", name_len)) {\n"
          "      int ret_val = %s%s().JSON_decode(%s_descr_, p_tok, p_silent);\n"
          "      if (0 > ret_val) {\n"
          "        if (JSON_ERROR_INVALID_TOKEN) {\n"
          "          JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FIELD_TOKEN_ERROR, %lu, \"%s\");\n"
          "        }\n"
          "        return JSON_ERROR_FATAL;\n"
          "      } else {\n"
          "        dec_len += (size_t)ret_val;\n"
          "      }\n"
          "    } else "
          , sdef->elements[i].jsonAlias ? sdef->elements[i].jsonAlias : sdef->elements[i].dispname
          , at_field, sdef->elements[i].name, sdef->elements[i].typedescrname
          , (unsigned long) strlen(sdef->elements[i].dispname), sdef->elements[i].dispname);
      }
      src = mputstr(src,
        "{\n"
        "      JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_INVALID_NAME_ERROR, (int)name_len, fld_name);\n"
        "      return JSON_ERROR_FATAL;\n"
        "    }\n"
        "  }\n\n"
        "  dec_len += p_tok.get_next_token(&j_token, NULL, NULL);\n"
        "  if (JSON_TOKEN_OBJECT_END != j_token) {\n"
        "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_STATIC_OBJECT_END_TOKEN_ERROR, \"\");\n"
        "    return JSON_ERROR_FATAL;\n"
        "  }\n\n"
        "  return (int)dec_len;\n"
        " }\n");
    }
    src = mputstr(src, "}\n\n");
  }

  /* end of class definition */
  def = mputstr(def, "};\n\n");

  output->header.class_defs = mputstr(output->header.class_defs, def);
  Free(def);

  output->source.methods = mputstr(output->source.methods, src);
  Free(src);

  Free(selection_type);
  Free(unbound_value);
  Free(selection_prefix);
}


void defUnionTemplate(const struct_def *sdef, output_struct *output)
{
  int i;
  const char *name = sdef->name, *dispname = sdef->dispname;
  const char *at_field = sdef->kind==ANYTYPE ? "AT_" : "";
  char *def = NULL, *src = NULL;

  char *selection_type, *unbound_value, *selection_prefix;
    selection_type = mprintf("%s::union_selection_type", name);
    unbound_value = mprintf("%s::UNBOUND_VALUE", name);
    selection_prefix = mprintf("%s::ALT", name);

  /* template class */

  output->header.class_decls = mputprintf(output->header.class_decls,
                                          "class %s_template;\n", name);

  /* template class header and data members */
  def = mputprintf(def, "class %s_template : public Base_Template {\n"
    "union {\n"
    "struct {\n"
    "%s union_selection;\n"
    "union {\n", name, selection_type);
  for (i = 0; i < sdef->nElements; i++) {
    def = mputprintf(def, "%s_template *field_%s;\n",
      sdef->elements[i].type, sdef->elements[i].name);
  }
  def = mputprintf(def, "};\n"
    "} single_value;\n"
    "struct {\n"
    "unsigned int n_values;\n"
    "%s_template *list_value;\n"
    "} value_list;\n"
    "};\n", name);
  if (use_runtime_2) {
    def = mputstr(def,
      "Erroneous_descriptor_t* err_descr;\n");
  }

  /* copy_value function */
  def = mputprintf(def, "void copy_value(const %s& other_value);\n\n", name);
  src = mputprintf(src, "void %s_template::copy_value(const %s& other_value)\n"
    "{\n"
    "single_value.union_selection = other_value.get_selection();\n"
    "switch (single_value.union_selection) {\n", name, name);
  for (i = 0; i<sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "single_value.field_%s = new %s_template(other_value.%s%s());\n"
      "break;\n", selection_prefix, sdef->elements[i].name,
      sdef->elements[i].name, sdef->elements[i].type,
      at_field, sdef->elements[i].name);
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Initializing a template with an unbound value of type "
      "%s.\");\n"
    "}\n"
    "set_selection(SPECIFIC_VALUE);\n"
    "%s"
    "}\n\n", dispname,
    use_runtime_2 ? "err_descr = other_value.get_err_descr();\n" : "");

  /* copy_template function */
  def = mputprintf(def, "void copy_template(const %s_template& "
      "other_value);\n", name);
  src = mputprintf(src, "void %s_template::copy_template(const "
      "%s_template& other_value)\n"
    "{\n"
    "switch (other_value.template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "single_value.union_selection = "
      "other_value.single_value.union_selection;\n"
    "switch (single_value.union_selection) {\n", name, name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "single_value.field_%s = "
	"new %s_template(*other_value.single_value.field_%s);\n"
      "break;\n", selection_prefix, sdef->elements[i].name,
      sdef->elements[i].name, sdef->elements[i].type, sdef->elements[i].name);
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Internal error: Invalid union selector in a specific value "
      "when copying a template of type %s.\");\n"
    "}\n"
    "case OMIT_VALUE:\n"
    "case ANY_VALUE:\n"
    "case ANY_OR_OMIT:\n"
    "break;\n"
    "case VALUE_LIST:\n"
    "case COMPLEMENTED_LIST:\n"
    "value_list.n_values = other_value.value_list.n_values;\n"
    "value_list.list_value = new %s_template[value_list.n_values];\n"
    "for (unsigned int list_count = 0; list_count < value_list.n_values; "
      "list_count++)\n"
    "value_list.list_value[list_count].copy_template("
    "other_value.value_list.list_value[list_count]);\n"
    "break;\n"
    "default:\n"
    "TTCN_error(\"Copying an uninitialized template of union type %s.\");\n"
    "}\n"
    "set_selection(other_value);\n"
    "%s"
    "}\n\n", dispname, name, dispname,
    use_runtime_2 ? "err_descr = other_value.err_descr;\n" : "");

  /* default constructor */
  def = mputprintf(def, "\npublic:\n"
    "%s_template();\n", name);
  src = mputprintf(src, "%s_template::%s_template()%s\n"
    "{\n"
    "}\n\n", name, name,
    use_runtime_2 ? ": err_descr(NULL)" : "");

  /* constructor (template_sel) */
  def = mputprintf(def, "%s_template(template_sel other_value);\n", name);
  src = mputprintf(src, "%s_template::%s_template(template_sel other_value)\n"
    " : Base_Template(other_value)%s\n"
    "{\n"
    "check_single_selection(other_value);\n"
    "}\n\n", name, name,
    use_runtime_2 ? ", err_descr(NULL)" : "");

  /* constructor (value) */
  def = mputprintf(def, "%s_template(const %s& other_value);\n", name, name);
  src = mputprintf(src, "%s_template::%s_template(const %s& other_value)\n"
    "{\n"
    "copy_value(other_value);\n"
    "}\n\n", name, name, name);

  /* constructor (optional value) */
  def = mputprintf(def, "%s_template(const OPTIONAL<%s>& other_value);\n", name,
    name);
  src = mputprintf(src, "%s_template::%s_template(const OPTIONAL<%s>& "
      "other_value)\n"
    "{\n"
    "switch (other_value.get_selection()) {\n"
    "case OPTIONAL_PRESENT:\n"
    "copy_value((const %s&)other_value);\n"
    "break;\n"
    "case OPTIONAL_OMIT:\n"
    "set_selection(OMIT_VALUE);\n"
    "%s"
    "break;\n"
    "default:\n"
    "TTCN_error(\"Creating a template of union type %s from an unbound "
      "optional field.\");\n"
    "}\n"
    "}\n\n", name, name, name, name,
    use_runtime_2 ? "err_descr = NULL;\n" : "",
    dispname);

  /* copy constructor */
  def = mputprintf(def, "%s_template(const %s_template& other_value);\n", name,
    name);
  src = mputprintf(src, "%s_template::%s_template(const %s_template& "
    "other_value)\n"
    ": Base_Template()" /* yes, the base class _default_ constructor */
    "{\n"
    "copy_template(other_value);\n"
    "}\n\n", name, name, name);

  /* destructor */
  def = mputprintf(def, "~%s_template();\n", name);
  src = mputprintf(src, "%s_template::~%s_template()\n"
    "{\n"
    "clean_up();\n"
    "}\n\n", name, name);

  /* clean_up function */
  def = mputstr(def, "void clean_up();\n");
  src = mputprintf(src, "void %s_template::clean_up()\n"
    "{\n"
    "switch (template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "switch (single_value.union_selection) {\n", name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "delete single_value.field_%s;\n",
      selection_prefix, sdef->elements[i].name, sdef->elements[i].name);
    if (i < sdef->nElements - 1) src = mputstr(src, "break;\n");
  }
  src = mputstr(src, "default:\n"
    "break;\n"
    "}\n"
    "break;\n"
    "case VALUE_LIST:\n"
    "case COMPLEMENTED_LIST:\n"
    "delete [] value_list.list_value;\n"
    "default:\n"
    "break;\n"
    "}\n"
    "template_selection = UNINITIALIZED_TEMPLATE;\n"
    "}\n\n");

  /* assignment operator (template_sel) */
  def = mputprintf(def, "%s_template& operator=(template_sel other_value);\n",
    name);
  src = mputprintf(src,
    "%s_template& %s_template::operator=(template_sel "
    "other_value)\n"
    "{\n"
    "check_single_selection(other_value);\n"
    "clean_up();\n"
    "set_selection(other_value);\n"
    "%s"
    "return *this;\n"
    "}\n\n", name, name, use_runtime_2 ? "err_descr = NULL;\n" : "");

  /* assignment operator (value) */
  def = mputprintf(def, "%s_template& operator=(const %s& other_value);\n",
    name, name);
  src = mputprintf(src, "%s_template& %s_template::operator=(const %s& "
      "other_value)\n"
    "{\n"
    "clean_up();\n"
    "copy_value(other_value);\n"
    "return *this;\n"
    "}\n\n", name, name, name);

  /* assignment operator <- optional value */
  def = mputprintf(def, "%s_template& operator=(const OPTIONAL<%s>& "
    "other_value);\n", name, name);
  src = mputprintf(src,
    "%s_template& %s_template::operator=(const OPTIONAL<%s>& other_value)\n"
    "{\n"
    "clean_up();\n"
    "switch (other_value.get_selection()) {\n"
    "case OPTIONAL_PRESENT:\n"
    "copy_value((const %s&)other_value);\n"
    "break;\n"
    "case OPTIONAL_OMIT:\n"
    "set_selection(OMIT_VALUE);\n"
    "%s"
    "break;\n"
    "default:\n"
    "TTCN_error(\"Assignment of an unbound optional field to a template of "
      "union type %s.\");\n"
    "}\n"
    "return *this;\n"
    "}\n\n", name, name, name, name,
    use_runtime_2 ? "err_descr = NULL;\n" : "", dispname);

  /* assignment operator (template) */
  def = mputprintf(def, "%s_template& operator=(const %s_template& "
    "other_value);\n", name, name);
  src = mputprintf(src,
    "%s_template& %s_template::operator=(const %s_template& other_value)\n"
    "{\n"
    "if (&other_value != this) {\n"
    "clean_up();\n"
    "copy_template(other_value);\n"
    "}\n"
    "return *this;\n"
    "}\n\n", name, name, name);

  /* match function */
  def = mputprintf(def, "boolean match(const %s& other_value, boolean legacy "
    "= FALSE) const;\n", name);
  src = mputprintf(src, "boolean %s_template::match(const %s& other_value, "
    "boolean legacy) const\n"
    "{\n"
    "if (!other_value.is_bound()) return FALSE;\n"
    "switch (template_selection) {\n"
    "case ANY_VALUE:\n"
    "case ANY_OR_OMIT:\n"
    "return TRUE;\n"
    "case OMIT_VALUE:\n"
    "return FALSE;\n"
    "case SPECIFIC_VALUE:\n"
    "{\n"
    "%s value_selection = other_value.get_selection();\n"
    "if (value_selection == %s) return FALSE;\n"
    "if (value_selection != single_value.union_selection) return FALSE;\n"
    "switch (value_selection) {\n", name, name, selection_type, unbound_value);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "return single_value.field_%s->match(other_value.%s%s(), legacy);\n",
      selection_prefix, sdef->elements[i].name, sdef->elements[i].name,
      at_field, sdef->elements[i].name);
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Internal error: Invalid selector in a specific value when "
      "matching a template of union type %s.\");\n"
    "}\n"
    "}\n"
    "case VALUE_LIST:\n"
    "case COMPLEMENTED_LIST:\n"
    "for (unsigned int list_count = 0; list_count < value_list.n_values; "
      "list_count++)\n"
    "if (value_list.list_value[list_count].match(other_value, legacy)) "
    "return template_selection == VALUE_LIST;\n"
    "return template_selection == COMPLEMENTED_LIST;\n"
    "default:\n"
    "TTCN_error (\"Matching an uninitialized template of union type %s.\");\n"
    "}\n"
    "return FALSE;\n"
    "}\n\n", dispname, dispname);

  /* isvalue */
  def = mputstr(def, "boolean is_value() const;");
  src = mputprintf(src, "boolean %s_template::is_value() const\n"
    "{\n"
    "if (template_selection != SPECIFIC_VALUE || is_ifpresent) return FALSE;\n"
    "switch (single_value.union_selection) {\n"
    , name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "return single_value.field_%s->is_value();\n", selection_prefix,
      sdef->elements[i].name, sdef->elements[i].name);
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Internal error: Invalid selector in a specific value when "
      "performing is_value operation on a template of union type %s.\");\n"
    "}\n"
    "}\n\n", dispname);

  /* valueof member function */
  def = mputprintf(def, "%s valueof() const;\n", name);
  src = mputprintf(src, "%s %s_template::valueof() const\n"
    "{\n"
    "if (template_selection != SPECIFIC_VALUE || is_ifpresent)\n"
    "TTCN_error(\"Performing a valueof or send operation on a non-specific "
      "template of union type %s.\");\n"
    "%s ret_val;\n"
    "switch (single_value.union_selection) {\n", name, name, dispname, name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "ret_val.%s%s() = single_value.field_%s->valueof();\n"
      "break;\n", selection_prefix, sdef->elements[i].name,
      at_field, sdef->elements[i].name, sdef->elements[i].name);
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Internal error: Invalid selector in a specific value when "
      "performing valueof operation on a template of union type %s.\");\n"
    "}\n"
    "%s"
    "return ret_val;\n"
    "}\n\n", dispname,
    use_runtime_2 ? "ret_val.set_err_descr(err_descr);\n" : "");

  /* list_item(int) function */
  def = mputprintf(def, "%s_template& list_item(unsigned int list_index) "
    "const;\n", name);
  src = mputprintf(src,
    "%s_template& %s_template::list_item(unsigned int list_index) const\n"
    "{\n"
    "if (template_selection != VALUE_LIST && "
      "template_selection != COMPLEMENTED_LIST) TTCN_error(\"Internal error: "
      "Accessing a list element of a non-list template of union type %s.\");\n"
    "if (list_index >= value_list.n_values) "
      "TTCN_error(\"Internal error: Index overflow in a value list template "
      "of union type %s.\");\n"
    "return value_list.list_value[list_index];\n"
    "}\n", name, name, dispname, dispname);

  /* void set_type(template_sel, int) function */
  def = mputstr(def, "void set_type(template_sel template_type, "
    "unsigned int list_length);\n");
  src = mputprintf(src,
    "void %s_template::set_type(template_sel template_type, "
      "unsigned int list_length)\n"
    "{\n"
    "if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST) "
      "TTCN_error (\"Internal error: Setting an invalid list for a template "
      "of union type %s.\");\n"
    "clean_up();\n"
    "set_selection(template_type);\n"
    "value_list.n_values = list_length;\n"
    "value_list.list_value = new %s_template[list_length];\n"
    "}\n\n", name, dispname, name);

  /* field access functions */
  for (i = 0; i < sdef->nElements; i++) {
    def = mputprintf(def, "%s_template& %s%s();\n", sdef->elements[i].type,
      at_field, sdef->elements[i].name);
    src = mputprintf(src, "%s_template& %s_template::%s%s()\n"
      "{\n"
      "if (template_selection != SPECIFIC_VALUE || "
	"single_value.union_selection != %s_%s) {\n"
      "template_sel old_selection = template_selection;\n"
      "clean_up();\n"
      "if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) "
	"single_value.field_%s = new %s_template(ANY_VALUE);\n"
      "else single_value.field_%s = new %s_template;\n"
      "single_value.union_selection = %s_%s;\n"
      "set_selection(SPECIFIC_VALUE);\n"
      "}\n"
      "return *single_value.field_%s;\n"
      "}\n\n", sdef->elements[i].type, name, at_field, sdef->elements[i].name,
      selection_prefix, sdef->elements[i].name, sdef->elements[i].name,
      sdef->elements[i].type, sdef->elements[i].name, sdef->elements[i].type,
      selection_prefix, sdef->elements[i].name, sdef->elements[i].name);

    def = mputprintf(def, "const %s_template& %s%s() const;\n",
      sdef->elements[i].type, at_field, sdef->elements[i].name);
    src = mputprintf(src, "const %s_template& %s_template::%s%s() const\n"
      "{\n"
      "if (template_selection != SPECIFIC_VALUE) TTCN_error(\"Accessing field "
	"%s in a non-specific template of union type %s.\");\n"
      "if (single_value.union_selection != %s_%s) "
	"TTCN_error(\"Accessing non-selected field %s in a template of union "
	  "type %s.\");\n"
      "return *single_value.field_%s;\n"
      "}\n\n", sdef->elements[i].type, name, at_field, sdef->elements[i].name,
      sdef->elements[i].dispname, dispname, selection_prefix,
      sdef->elements[i].name, sdef->elements[i].dispname, dispname,
      sdef->elements[i].name);
  }

  /* ischosen function */
  def = mputprintf(def, "boolean ischosen(%s checked_selection) const;\n",
    selection_type);
  src = mputprintf(src, "boolean %s_template::ischosen(%s checked_selection) "
      "const\n"
    "{\n"
    "if (checked_selection == %s) TTCN_error(\"Internal error: Performing "
      "ischosen() operation on an invalid field of union type %s.\");\n"
    "switch (template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "if (single_value.union_selection == %s) TTCN_error(\"Internal error: "
      "Invalid selector in a specific value when performing ischosen() "
      "operation on a template of union type %s.\");\n"
    "return single_value.union_selection == checked_selection;\n"
    "case VALUE_LIST:\n"
    "{\n"
    "if (value_list.n_values < 1)\n"
    "TTCN_error(\"Internal error: Performing ischosen() operation on a "
      "template of union type %s containing an empty list.\");\n"
    "boolean ret_val = "
      "value_list.list_value[0].ischosen(checked_selection);\n"
    "boolean all_same = TRUE;\n"
    "for (unsigned int list_count = 1; list_count < value_list.n_values; "
      "list_count++) {\n"
    "if (value_list.list_value[list_count].ischosen(checked_selection) != "
      "ret_val) {\n"
    "all_same = FALSE;\n"
    "break;\n"
    "}\n"
    "}\n"
    "if (all_same) return ret_val;\n"
    "}\n"
    "case ANY_VALUE:\n"
    "case ANY_OR_OMIT:\n"
    "case OMIT_VALUE:\n"
    "case COMPLEMENTED_LIST:\n"
    "TTCN_error(\"Performing ischosen() operation on a template of union type "
      "%s, which does not determine unambiguously the chosen field of the "
      "matching values.\");\n"
    "default:\n"
    "TTCN_error(\"Performing ischosen() operation on an uninitialized "
      "template of union type %s\");\n"
    "}\n"
    "return FALSE;\n"
    "}\n\n", name, selection_type, unbound_value, dispname, unbound_value,
    dispname, dispname, dispname, dispname);

  if (use_runtime_2) {
    def = mputstr(def,
      "void set_err_descr(Erroneous_descriptor_t* p_err_descr) { err_descr=p_err_descr; }\n");
    /** virtual stuff */
    def = mputstr(def,
      "void valueofv(Base_Type* value) const;\n"
      "void set_value(template_sel other_value);\n"
      "void copy_value(const Base_Type* other_value);\n"
      "Base_Template* clone() const;\n"
      "const TTCN_Typedescriptor_t* get_descriptor() const;\n"
      "boolean matchv(const Base_Type* other_value, boolean legacy) const;\n"
      "void log_matchv(const Base_Type* match_value, boolean legacy) const;\n");
    src = mputprintf(src,
      "void %s_template::valueofv(Base_Type* value) const "
        "{ *(static_cast<%s*>(value)) = valueof(); }\n"
      "void %s_template::set_value(template_sel other_value) "
        "{ *this = other_value; }\n"
      "void %s_template::copy_value(const Base_Type* other_value) "
        "{ *this = *(static_cast<const %s*>(other_value)); }\n"
      "Base_Template* %s_template::clone() const "
        "{ return new %s_template(*this); }\n"
      "const TTCN_Typedescriptor_t* %s_template::get_descriptor() const "
        "{ return &%s_descr_; }\n"
      "boolean %s_template::matchv(const Base_Type* other_value, "
        "boolean legacy) const "
        "{ return match(*(static_cast<const %s*>(other_value)), legacy); }\n"
      "void %s_template::log_matchv(const Base_Type* match_value, "
        "boolean legacy) const "
        " { log_match(*(static_cast<const %s*>(match_value)), legacy); }\n",
      name, name,
      name,
      name, name,
      name, name,
      name, name,
      name, name,
      name, name);
  }

  /* log function */
  def = mputstr(def, "void log() const;\n");
  src = mputprintf(src,"void %s_template::log() const\n"
    "{\n"
    "switch (template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "switch (single_value.union_selection) {\n", name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "TTCN_Logger::log_event_str(\"{ %s := \");\n"
      "single_value.field_%s->log();\n"
      "TTCN_Logger::log_event_str(\" }\");\n"
      "break;\n", selection_prefix, sdef->elements[i].name,
      sdef->elements[i].dispname, sdef->elements[i].name);
  }
  src = mputstr(src, "default:\n"
    "TTCN_Logger::log_event_str(\"<invalid selector>\");\n"
    "}\n"
    "break;\n"
    "case COMPLEMENTED_LIST:\n"
    "TTCN_Logger::log_event_str(\"complement \");\n"
    "case VALUE_LIST:\n"
    "TTCN_Logger::log_char('(');\n"
    "for (unsigned int list_count = 0; list_count < value_list.n_values; "
      "list_count++) {\n"
    "if (list_count > 0) TTCN_Logger::log_event_str(\", \");\n"
    "value_list.list_value[list_count].log();\n"
    "}\n"
    "TTCN_Logger::log_char(')');\n"
    "break;\n"
    "default:\n"
    "log_generic();\n"
    "}\n"
    "log_ifpresent();\n");
    if (use_runtime_2) {
      src = mputstr(src, "if (err_descr) err_descr->log();\n");
    }
    src = mputstr(src, "}\n\n");

  /* log_match function */
  def = mputprintf(def, "void log_match(const %s& match_value, "
    "boolean legacy = FALSE) const;\n", name);
  src = mputprintf(src,
    "void %s_template::log_match(const %s& match_value, boolean legacy) const\n"
    "{\n"
    "if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity() "
    "&& match(match_value, legacy)){\n"
    "TTCN_Logger::print_logmatch_buffer();\n"
    "TTCN_Logger::log_event_str(\" matched\");\n"
    "return;\n"
    "}\n"
    "if (template_selection == SPECIFIC_VALUE && "
    "single_value.union_selection == match_value.get_selection()) {\n"
    "switch (single_value.union_selection) {\n", name, name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
    "if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){\n"
    "TTCN_Logger::log_logmatch_info(\".%s\");\n"
    "single_value.field_%s->log_match(match_value.%s%s(), legacy);\n"
    "} else {\n"
      "TTCN_Logger::log_event_str(\"{ %s := \");\n"
      "single_value.field_%s->log_match(match_value.%s%s(), legacy);\n"
    "TTCN_Logger::log_event_str(\" }\");\n"
    "}\n"
    "break;\n", selection_prefix, sdef->elements[i].name,
      sdef->elements[i].dispname,
      sdef->elements[i].name, at_field, sdef->elements[i].name,
      sdef->elements[i].dispname, sdef->elements[i].name,
      at_field, sdef->elements[i].name);
  }
  src = mputstr(src, "default:\n"
    "TTCN_Logger::print_logmatch_buffer();\n"
    "TTCN_Logger::log_event_str(\"<invalid selector>\");\n"
    "}\n"
    "} else {\n"
    "TTCN_Logger::print_logmatch_buffer();\n"
    "match_value.log();\n"
    "TTCN_Logger::log_event_str(\" with \");\n"
    "log();\n"
    "if (match(match_value, legacy)) TTCN_Logger::log_event_str(\" matched\");\n"
    "else TTCN_Logger::log_event_str(\" unmatched\");\n"
    "}\n"
    "}\n\n");

  /* encode_text function */
  def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
  src = mputprintf(src,
    "void %s_template::encode_text(Text_Buf& text_buf) const\n"
    "{\n"
    "encode_text_base(text_buf);\n"
    "switch (template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "text_buf.push_int(single_value.union_selection);\n"
    "switch (single_value.union_selection) {\n", name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "single_value.field_%s->encode_text(text_buf);\n"
      "break;\n", selection_prefix, sdef->elements[i].name,
      sdef->elements[i].name);
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Internal error: Invalid selector in a specific value when "
      "encoding a template of union type %s.\");\n"
    "}\n"
    "case OMIT_VALUE:\n"
    "case ANY_VALUE:\n"
    "case ANY_OR_OMIT:\n"
    "break;\n"
    "case VALUE_LIST:\n"
    "case COMPLEMENTED_LIST:\n"
    "text_buf.push_int(value_list.n_values);\n"
    "for (unsigned int list_count = 0; list_count < value_list.n_values; "
      "list_count++)\n"
    "value_list.list_value[list_count].encode_text(text_buf);\n"
    "break;\n"
    "default:\n"
    "TTCN_error(\"Text encoder: Encoding an uninitialized template of type "
      "%s.\");\n"
    "}\n"
    "}\n\n", dispname, dispname);

  /* decode_text function */
  def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
  src = mputprintf(src, "void %s_template::decode_text(Text_Buf& text_buf)\n"
    "{\n"
    "clean_up();\n"
    "decode_text_base(text_buf);\n"
    "switch (template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "{\n"
    "single_value.union_selection = %s;\n"
    "%s new_selection = (%s)text_buf.pull_int().get_val();\n"
    "switch (new_selection) {\n", name, unbound_value, selection_type,
    selection_type);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "single_value.field_%s = new %s_template;\n"
      "single_value.field_%s->decode_text(text_buf);\n"
      "break;\n", selection_prefix, sdef->elements[i].name,
      sdef->elements[i].name, sdef->elements[i].type, sdef->elements[i].name);
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Text decoder: Unrecognized union selector was received for "
      "a template of type %s.\");\n"
    "}\n"
    "single_value.union_selection = new_selection;\n"
    "}\n"
    "case OMIT_VALUE:\n"
    "case ANY_VALUE:\n"
    "case ANY_OR_OMIT:\n"
    "break;\n"
    "case VALUE_LIST:\n"
    "case COMPLEMENTED_LIST:\n"
    "value_list.n_values = text_buf.pull_int().get_val();\n"
    "value_list.list_value = new %s_template[value_list.n_values];\n"
    "for (unsigned int list_count = 0; list_count < value_list.n_values; "
      "list_count++)\n"
    "value_list.list_value[list_count].decode_text(text_buf);\n"
    "break;\n"
    "default:\n"
    "TTCN_error(\"Text decoder: Unrecognized selector was received in a "
      "template of type %s.\");\n"
    "}\n"
    "}\n\n", dispname, name, dispname);

  /* TTCN-3 ispresent() function */
  def = mputstr(def, "boolean is_present(boolean legacy = FALSE) const;\n");
  src = mputprintf(src,
    "boolean %s_template::is_present(boolean legacy) const\n"
    "{\n"
    "if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;\n"
    "return !match_omit(legacy);\n"
    "}\n\n", name);

  /* match_omit() */
  def = mputstr(def, "boolean match_omit(boolean legacy = FALSE) const;\n");
  src = mputprintf(src,
    "boolean %s_template::match_omit(boolean legacy) const\n"
    "{\n"
    "if (is_ifpresent) return TRUE;\n"
    "switch (template_selection) {\n"
    "case OMIT_VALUE:\n"
    "case ANY_OR_OMIT:\n"
    "return TRUE;\n"
    "case VALUE_LIST:\n"
    "case COMPLEMENTED_LIST:\n"
    "if (legacy) {\n"
    "for (unsigned int v_idx=0; v_idx<value_list.n_values; v_idx++)\n"
    "if (value_list.list_value[v_idx].match_omit())\n"
    "return template_selection==VALUE_LIST;\n"
    "return template_selection==COMPLEMENTED_LIST;\n"
    "} // else fall through\n"
    "default:\n"
    "return FALSE;\n"
    "}\n"
    "return FALSE;\n"
    "}\n\n", name);

  /* set_param() */
  def = mputstr(def, "void set_param(Module_Param& param);\n");
  src = mputprintf(src,
    "void %s_template::set_param(Module_Param& param)\n"
    "{\n"
    "  if (dynamic_cast<Module_Param_Name*>(param.get_id()) != NULL &&\n"
    "      param.get_id()->next_name()) {\n"
   // Haven't reached the end of the module parameter name
   // => the name refers to one of the fields, not to the whole union
    "    char* param_field = param.get_id()->get_current_name();\n"
    "    if (param_field[0] >= '0' && param_field[0] <= '9') {\n"
    "      param.error(\"Unexpected array index in module parameter, expected a valid field\"\n"
    "        \" name for union template type `%s'\");\n"
    "    }\n"
    "    ", name, dispname);
 for (i = 0; i < sdef->nElements; i++) {
   src = mputprintf(src,
    "if (strcmp(\"%s\", param_field) == 0) {\n"
    "      %s%s().set_param(param);\n"
    "      return;\n"
    "    } else ",
    sdef->elements[i].dispname, at_field, sdef->elements[i].name);
 }
 src = mputprintf(src,
    "param.error(\"Field `%%s' not found in union template type `%s'\", param_field);\n"
    "  }\n"
    "  param.basic_check(Module_Param::BC_TEMPLATE, \"union template\");\n"
    "  Module_Param_Ptr m_p = &param;\n", dispname);
 if (use_runtime_2) {
   src = mputstr(src,
    "  if (param.get_type() == Module_Param::MP_Reference) {\n"
    "    m_p = param.get_referenced_param();\n"
    "  }\n");
 }
 src = mputprintf(src,
    "  switch (m_p->get_type()) {\n"
    "  case Module_Param::MP_Omit:\n"
    "    *this = OMIT_VALUE;\n"
    "    break;\n"
    "  case Module_Param::MP_Any:\n"
    "    *this = ANY_VALUE;\n"
    "    break;\n"
    "  case Module_Param::MP_AnyOrNone:\n"
    "    *this = ANY_OR_OMIT;\n"
    "    break;\n"
    "  case Module_Param::MP_List_Template:\n"
    "  case Module_Param::MP_ComplementList_Template: {\n"
    "    %s_template new_temp;\n"
    "    new_temp.set_type(m_p->get_type()==Module_Param::MP_List_Template ? "
    "VALUE_LIST : COMPLEMENTED_LIST, m_p->get_size());\n"
    "    for (size_t p_i=0; p_i<m_p->get_size(); p_i++) {\n"
    "      new_temp.list_item(p_i).set_param(*m_p->get_elem(p_i));\n"
    "    }\n"
    "    *this = new_temp;\n"
    "    break; }\n"
    "  case Module_Param::MP_Value_List:\n"
    "    if (m_p->get_size()==0) break;\n" /* for backward compatibility */
    "    param.type_error(\"union template\", \"%s\");\n"
    "    break;\n"
    "  case Module_Param::MP_Assignment_List: {\n"
    "    Module_Param* mp_last = m_p->get_elem(m_p->get_size()-1);\n",
    name, dispname);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, 
    "    if (!strcmp(mp_last->get_id()->get_name(), \"%s\")) {\n"
    "      %s%s().set_param(*mp_last);\n"
    "      break;\n"
    "    }\n", sdef->elements[i].dispname, at_field, sdef->elements[i].name);
  }
  src = mputprintf(src,
    "    mp_last->error(\"Field %%s does not exist in type %s.\", mp_last->get_id()->get_name());\n"
    "  } break;\n"
    "  default:\n"
    "    param.type_error(\"union template\", \"%s\");\n"
    "  }\n"
    "  is_ifpresent = param.get_ifpresent()%s;\n"
    "}\n\n", dispname, dispname, use_runtime_2 ? " || m_p->get_ifpresent()" : "");
  
  /* get_param(), RT2 only */
  if (use_runtime_2) {
    def = mputstr(def, "Module_Param* get_param(Module_Param_Name& param_name) const;\n");
    src = mputprintf(src,
      "Module_Param* %s_template::get_param(Module_Param_Name& param_name) const\n"
      "{\n"
      "  if (param_name.next_name()) {\n"
      // Haven't reached the end of the module parameter name
      // => the name refers to one of the fields, not to the whole union
      "    char* param_field = param_name.get_current_name();\n"
      "    if (param_field[0] >= '0' && param_field[0] <= '9') {\n"
      "      TTCN_error(\"Unexpected array index in module parameter reference, \"\n"
      "        \"expected a valid field name for union template type `%s'\");\n"
      "    }\n"
      "    ", name, dispname);
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src,
       "if (strcmp(\"%s\", param_field) == 0) {\n"
       "      return %s%s().get_param(param_name);\n"
       "    } else ",
       sdef->elements[i].dispname, at_field, sdef->elements[i].name);
    }
    src = mputprintf(src,
      "TTCN_error(\"Field `%%s' not found in union type `%s'\", param_field);\n"
      "  }\n"
      "  Module_Param* m_p = NULL;\n"
      "  switch (template_selection) {\n"
      "  case UNINITIALIZED_TEMPLATE:\n"
      "    m_p = new Module_Param_Unbound();\n"
      "    break;\n"
      "  case OMIT_VALUE:\n"
      "    m_p = new Module_Param_Omit();\n"
      "    break;\n"
      "  case ANY_VALUE:\n"
      "    m_p = new Module_Param_Any();\n"
      "    break;\n"
      "  case ANY_OR_OMIT:\n"
      "    m_p = new Module_Param_AnyOrNone();\n"
      "    break;\n"
      "  case SPECIFIC_VALUE: {\n"
      "    Module_Param* mp_field = NULL;\n"
      "    switch(single_value.union_selection) {\n"
      , name);
      for (i = 0; i < sdef->nElements; ++i) {
        src = mputprintf(src, 
          "    case %s_%s:\n"
          "      mp_field = single_value.field_%s->get_param(param_name);\n"
          "      mp_field->set_id(new Module_Param_FieldName(mcopystr(\"%s\")));\n"
          "      break;\n"
          , selection_prefix, sdef->elements[i].name
          , sdef->elements[i].name, sdef->elements[i].dispname);
      }
    src = mputstr(src,
      "    default:\n"
      "      break;\n"
      "    }\n"
      "    m_p = new Module_Param_Assignment_List();\n"
      "    m_p->add_elem(mp_field);\n"
      "    break; }\n"
      "  case VALUE_LIST:\n"
      "  case COMPLEMENTED_LIST: {\n"
      "    if (template_selection == VALUE_LIST) {\n"
      "      m_p = new Module_Param_List_Template();\n"
      "    }\n"
      "    else {\n"
      "      m_p = new Module_Param_ComplementList_Template();\n"
      "    }\n"
      "    for (size_t i_i = 0; i_i < value_list.n_values; ++i_i) {\n"
      "      m_p->add_elem(value_list.list_value[i_i].get_param(param_name));\n"
      "    }\n"
      "    break; }\n"
      "  default:\n"
      "    break;\n"
      "  }\n"
      "  if (is_ifpresent) {\n"
      "    m_p->set_ifpresent();\n"
      "  }\n"
      "  return m_p;\n"
      "}\n\n");
  }

  /* check template restriction */
  def = mputstr(def, "void check_restriction(template_res t_res, "
    "const char* t_name=NULL, boolean legacy = FALSE) const;\n");
  src = mputprintf(src,
    "void %s_template::check_restriction("
      "template_res t_res, const char* t_name, boolean legacy) const\n"
    "{\n"
    "if (template_selection==UNINITIALIZED_TEMPLATE) return;\n"
    "switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {\n"
    "case TR_OMIT:\n"
    "if (template_selection==OMIT_VALUE) return;\n"
    "case TR_VALUE:\n"
    "if (template_selection!=SPECIFIC_VALUE || is_ifpresent) break;\n"
    "switch (single_value.union_selection) {\n"
    , name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "case %s_%s:\n"
      "single_value.field_%s->check_restriction("
        "t_res, t_name ? t_name : \"%s\");\n"
      "return;\n",
      selection_prefix, sdef->elements[i].name,
      sdef->elements[i].name, dispname);
  }
  src = mputprintf(src, "default:\n"
    "TTCN_error(\"Internal error: Invalid selector in a specific value when "
      "performing check_restriction operation on a template of union type %s.\");\n"
    "}\n"
    "case TR_PRESENT:\n"
    "if (!match_omit(legacy)) return;\n"
    "break;\n"
    "default:\n"
    "return;\n"
    "}\n"
    "TTCN_error(\"Restriction `%%s' on template of type %%s violated.\", "
      "get_res_name(t_res), t_name ? t_name : \"%s\");\n"
    "}\n\n", dispname, dispname);

  def = mputstr(def, "};\n\n");

  output->header.class_defs = mputstr(output->header.class_defs, def);
  Free(def);

  output->source.methods = mputstr(output->source.methods, src);
  Free(src);

  Free(selection_type);
  Free(unbound_value);
  Free(selection_prefix);
}
