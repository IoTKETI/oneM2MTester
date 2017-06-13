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
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include "../common/memory.h"
#include "datatypes.h"
#include "enum.h"
#include "encdec.h"

#include <stdlib.h>

#include "main.hh"
#include "ttcn3/compiler.h"

void defEnumClass(const enum_def *edef, output_struct *output)
{
  size_t i;
  char *def = NULL, *src = NULL;
  const char *name = edef->name, *dispname = edef->dispname;
  boolean ber_needed = edef->isASN1 && enable_ber();
  boolean raw_needed = edef->hasRaw && enable_raw();
  boolean text_needed= edef->hasText && enable_text();
  boolean xer_needed = edef->hasXer && enable_xer();
  boolean json_needed = edef->hasJson && enable_json();

  char *enum_type, *qualified_enum_type, *unknown_value, *unbound_value;
  enum_type = mcopystr("enum_type");
  qualified_enum_type = mprintf("%s::enum_type", name);
  unknown_value = mcopystr("UNKNOWN_VALUE");
  unbound_value = mcopystr("UNBOUND_VALUE");

  /* Class declaration */
  output->header.class_decls = mputprintf(output->header.class_decls,
    "class %s;\n", name);

  /* Class definition */
  def = mputprintf(def,
#ifndef NDEBUG
      "// written by %s in " __FILE__ " at %d\n"
#endif
    "class %s : public %s { // enum\n"
    "friend class %s_template;\n"
#ifndef NDEBUG
      , __FUNCTION__, __LINE__
#endif
      , name, (use_runtime_2) ? "Enum_Type" : "Base_Type", name);
  def = mputstr(def, "public:\n"
    "enum enum_type { ");
  for (i = 0; i < edef->nElements; i++) {
    def = mputprintf(def, "%s = %d, ", edef->elements[i].name,
    edef->elements[i].value);
  }
  def = mputprintf(def, "UNKNOWN_VALUE = %d, UNBOUND_VALUE = %d };\n"
    "private:\n", edef->firstUnused, edef->secondUnused);
  def = mputprintf(def, "%s enum_value;\n\n"
    "public:\n", enum_type);

  /* constructors */
  def = mputprintf(def, "%s();\n", name);
  src = mputprintf(src, "%s::%s()\n"
    "{\n"
    "enum_value = %s;\n"
    "}\n\n", name, name, unbound_value);

  def = mputprintf(def, "%s(int other_value);\n", name);
  src = mputprintf(src,
    "%s::%s(int other_value)\n"
    "{\n"
    "if (!is_valid_enum(other_value)) "
    "TTCN_error(\"Initializing a variable of enumerated type %s with "
    "invalid numeric value %%d.\", other_value);\n"
    "enum_value = (%s)other_value;\n"
    "}\n\n",
    name, name, dispname, enum_type);

  def = mputprintf(def, "%s(%s other_value);\n", name, enum_type);
  src = mputprintf(src,
    "%s::%s(%s other_value)\n"
    "{\n"
    "enum_value = other_value;\n"
    "}\n\n", name, name, enum_type);

  def = mputprintf(def, "%s(const %s& other_value);\n\n", name, name);
  src = mputprintf
    (src,
     "%s::%s(const %s& other_value)\n"
     ": %s()\n" /* Base class DEFAULT constructor*/
     "{\n"
     "if (other_value.enum_value == %s) "
     "TTCN_error(\"Copying an unbound value of enumerated type %s.\");\n"
     "enum_value = other_value.enum_value;\n"
     "}\n\n", name, name, name, (use_runtime_2) ? "Enum_Type" : "Base_Type",
     unbound_value, dispname);

  /* assignment operators */
  def = mputprintf(def, "%s& operator=(int other_value);\n", name);
  src = mputprintf(src,
    "%s& %s::operator=(int other_value)\n"
    "{\n"
    "if (!is_valid_enum(other_value)) "
    "TTCN_error(\"Assigning unknown numeric value %%d to a variable "
    "of enumerated type %s.\", other_value);\n"
    "enum_value = (%s)other_value;\n"
    "return *this;\n"
    "}\n\n", name, name, dispname, enum_type);

  def = mputprintf(def, "%s& operator=(%s other_value);\n", name, enum_type);
  src = mputprintf(src,
    "%s& %s::operator=(%s other_value)\n"
    "{\n"
    "enum_value = other_value;\n"
    "return *this;\n"
    "}\n\n", name, name, enum_type);

  def = mputprintf(def, "%s& operator=(const %s& other_value);\n\n", name,
                   name);
  src = mputprintf(src,
    "%s& %s::operator=(const %s& other_value)\n"
    "{\n"
    "if (other_value.enum_value == %s) "
    "TTCN_error(\"Assignment of an unbound value of enumerated type %s.\");\n"
    "enum_value = other_value.enum_value;\n"
    "return *this;\n"
    "}\n\n", name, name, name, unbound_value, dispname);

  /* Comparison operators */
  def = mputprintf(def, "boolean operator==(%s other_value) const;\n",
                   enum_type);
  src = mputprintf(src,
    "boolean %s::operator==(%s other_value) const\n"
    "{\n"
    "if (enum_value == %s) "
    "TTCN_error(\"The left operand of comparison is an unbound value of "
    "enumerated type %s.\");\n"
    "return enum_value == other_value;\n"
    "}\n\n", name, enum_type, unbound_value, dispname);

  def = mputprintf(def, "boolean operator==(const %s& other_value) const;\n",
                   name);
  src = mputprintf(src,
    "boolean %s::operator==(const %s& other_value) const\n"
    "{\n"
    "if (enum_value == %s) "
    "TTCN_error(\"The left operand of comparison is an unbound value of "
    "enumerated type %s.\");\n"
    "if (other_value.enum_value == %s) "
    "TTCN_error(\"The right operand of comparison is an unbound value of "
    "enumerated type %s.\");\n"
    "return enum_value == other_value.enum_value;\n"
    "}\n\n", name, name, unbound_value, dispname, unbound_value, dispname);

  def = mputprintf(def, "inline boolean operator!=(%s other_value) "
                   "const { return !(*this == other_value); }\n", enum_type);

  def = mputprintf(def, "inline boolean operator!=(const %s& other_value) "
                   "const { return !(*this == other_value); }\n", name);

  def = mputprintf(def, "boolean operator<(%s other_value) const;\n",
                   enum_type);
  src = mputprintf(src,
    "boolean %s::operator<(%s other_value) const\n"
    "{\n"
    "if (enum_value == %s) "
    "TTCN_error(\"The left operand of comparison is an unbound value of "
    "enumerated type %s.\");\n"
    "return enum_value < other_value;\n"
    "}\n\n", name, enum_type, unbound_value, dispname);

  def = mputprintf(def, "boolean operator<(const %s& other_value) const;\n",
                   name);
  src = mputprintf(src,
    "boolean %s::operator<(const %s& other_value) const\n"
    "{\n"
    "if (enum_value == %s) "
    "TTCN_error(\"The left operand of comparison is an unbound value of "
    "enumerated type %s.\");\n"
    "if (other_value.enum_value == %s) "
    "TTCN_error(\"The right operand of comparison is an unbound value of "
    "enumerated type %s.\");\n"
    "return enum_value < other_value.enum_value;\n"
    "}\n\n", name, name, unbound_value, dispname, unbound_value, dispname);

  def = mputprintf(def, "boolean operator>(%s other_value) const;\n",
                   enum_type);
  src = mputprintf(src,
    "boolean %s::operator>(%s other_value) const\n"
    "{\n"
    "if (enum_value == %s) "
    "TTCN_error(\"The left operand of comparison is an unbound value of "
    "enumerated type %s.\");\n"
    "return enum_value > other_value;\n"
    "}\n\n", name, enum_type, unbound_value, dispname);

  def = mputprintf(def, "boolean operator>(const %s& other_value) const;\n",
                   name);
  src = mputprintf(src,
    "boolean %s::operator>(const %s& other_value) const\n"
    "{\n"
    "if (enum_value == %s) "
    "TTCN_error(\"The left operand of comparison is an unbound value of "
    "enumerated type %s.\");\n"
    "if (other_value.enum_value == %s) "
    "TTCN_error(\"The right operand of comparison is an unbound value of "
    "enumerated type %s.\");\n"
    "return enum_value > other_value.enum_value;\n"
    "}\n\n", name, name, unbound_value, dispname, unbound_value, dispname);

  def = mputprintf(def, "inline boolean operator<=(%s other_value) "
                   "const { return !(*this > other_value); }\n", enum_type);

  def = mputprintf(def, "inline boolean operator<=(const %s& other_value) "
                   "const { return !(*this > other_value); }\n", name);

  def = mputprintf(def, "inline boolean operator>=(%s other_value) "
                   "const { return !(*this < other_value); }\n", enum_type);

  def = mputprintf(def, "inline boolean operator>=(const %s& other_value) "
                   "const { return !(*this < other_value); }\n\n", name);

  /* Conversion function: enum_to_str */
  def = mputprintf(def, "static const char *enum_to_str(%s enum_par%s);\n",
    enum_type,
    edef->xerText ? ", boolean txt = FALSE" : "");
  src = mputprintf(src, "const char *%s::enum_to_str(%s enum_par%s)\n"
    "{\n"
    "switch (enum_par) {\n", name, enum_type,
    edef->xerText ? ", boolean txt" : "");
  for (i = 0; i < edef->nElements; i++) {
    if (edef->elements[i].text) {
      src = mputprintf(src,
        "case %s: if (txt) return \"%s\"; else return \"%s\";\n",
        edef->elements[i].name, edef->elements[i].text, edef->elements[i].dispname);
    }
    else {
      src = mputprintf(src, "case %s: return \"%s\";\n",
        edef->elements[i].name, edef->elements[i].dispname);
    }
  }
  src = mputstr(src, "default: return \"<unknown>\";\n"
    "}\n"
    "}\n\n");

  /* Conversion function: str_to_enum */
  def = mputprintf(def, "static %s str_to_enum(const char *str_par);\n",
    enum_type);
  src = mputprintf(src, "%s %s::str_to_enum(const char *str_par)\n"
    "{\n", qualified_enum_type, name);
  for (i = 0; i < edef->nElements; i++) {
    if (edef->elements[i].text) {
      src = mputprintf(src, "if (!strcmp(str_par, \"%s\") || !strcmp(str_par, \"%s\")) return %s;\n"
        "else ", edef->elements[i].text, edef->elements[i].dispname, edef->elements[i].name);
    }
    else {
      src = mputprintf(src, "if (!strcmp(str_par, \"%s\")) return %s;\n"
        "else ", edef->elements[i].dispname, edef->elements[i].name);
    }
  }
  src = mputprintf(src, "return %s;\n"
    "}\n\n", unknown_value);

  /* Checking function: is_valid_enum */
  def = mputstr(def, "static boolean is_valid_enum(int int_par);\n\n");
  src = mputprintf(src, "boolean %s::is_valid_enum(int int_par)\n"
    "{\n"
    "switch (int_par) {\n", name);
  for (i = 0; i < edef->nElements; i++) {
    src = mputprintf(src, "case %d:\n", edef->elements[i].value);
  }
  src = mputstr(src, "return TRUE;\n"
    "default:\n"
    "return FALSE;\n"
    "}\n"
    "}\n\n");

  /* TTCN-3 predefined function enum2int() */
  def = mputprintf(def, "static int enum2int(%s enum_par);\n", enum_type);
  src = mputprintf(src, "int %s::enum2int(%s enum_par)\n"
    "{\n"
    "if (enum_par==%s || enum_par==%s) TTCN_error(\"The argument of function "
      "enum2int() is an %%s value of enumerated type %s.\", "
      "enum_par==%s?\"unbound\":\"invalid\");\n"
    "return enum_par;\n"
    "}\n\n",
    name, enum_type, unbound_value, unknown_value, dispname, unbound_value);
  def = mputprintf(def, "static int enum2int(const %s& enum_par);\n", name);
  src = mputprintf(src, "int %s::enum2int(const %s& enum_par)\n"
    "{\n"
    "if (enum_par.enum_value==%s || enum_par.enum_value==%s) "
      "TTCN_error(\"The argument of function "
      "enum2int() is an %%s value of enumerated type %s.\", "
      "enum_par==%s?\"unbound\":\"invalid\");\n"
    "return enum_par.enum_value;\n"
    "}\n\n",
    name, name, unbound_value, unknown_value, dispname, unbound_value);
  def = mputstr(def,
    "int as_int() const { return enum2int(enum_value); }\n"
    "void from_int(int p_val) { *this = p_val; }\n");
  
  /* TTCN-3 predefined function int2enum() */
  def = mputstr(def, "void int2enum(int int_val);\n");
  src = mputprintf(src, "void %s::int2enum(int int_val)\n"
    "{\n"
    "if (!is_valid_enum(int_val)) "
    "TTCN_error(\"Assigning invalid numeric value %%d to a variable of "
    "enumerated type %s.\", int_val);\n"
    "enum_value = (%s)int_val;\n"
    "}\n\n", name, dispname, enum_type);

  /* miscellaneous members */
  def = mputprintf(def, "operator %s() const;\n", enum_type);
  src = mputprintf(src,
    "%s::operator %s() const\n"
    "{\n"
    "if (enum_value == %s) "
    "TTCN_error(\"Using the value of an unbound variable of enumerated "
    "type %s.\");\n"
    "return enum_value;\n"
    "}\n\n", name, qualified_enum_type, unbound_value, dispname);

  def = mputprintf(def, "inline boolean is_bound() const "
    "{ return enum_value != %s; }\n", unbound_value);
  def = mputprintf(def, "inline boolean is_value() const "
    "{ return enum_value != %s; }\n", unbound_value);
  def = mputprintf(def, "inline void clean_up()"
    "{ enum_value = %s; }\n", unbound_value);

  if (use_runtime_2) {
    def = mputstr(def,
      "boolean is_equal(const Base_Type* other_value) const;\n"
      "void set_value(const Base_Type* other_value);\n"
      "Base_Type* clone() const;\n"
      "const TTCN_Typedescriptor_t* get_descriptor() const;\n");
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

  def = mputstr(def, "void log() const;\n");
  src = mputprintf(src,
    "void %s::log() const\n"
    "{\n"
    "if (enum_value != %s) TTCN_Logger::log_event_enum(enum_to_str(enum_value), enum_value);\n"
    "else TTCN_Logger::log_event_unbound();\n"
    "}\n\n", name, unbound_value);

  def = mputstr(def, "void set_param(Module_Param& param);\n");
  src = mputprintf
    (src,
     "void %s::set_param(Module_Param& param)\n"
     "{\n"
     "  param.basic_check(Module_Param::BC_VALUE, \"enumerated value\");\n", name);
  if (use_runtime_2) {
    src = mputprintf(src,
     "  Module_Param_Ptr m_p = &param;\n"
     "  if (param.get_type() == Module_Param::MP_Reference) {\n"
     /* enumerated values are also treated as references (containing only 1 name) by the parser;
        first check if the reference name is a valid enumerated value */
     "    char* enum_name = param.get_enumerated();\n"
     /* get_enumerated() returns NULL if the reference contained more than one name */
     "    enum_value = (enum_name != NULL) ? str_to_enum(enum_name) : %s;\n"
     "    if (is_valid_enum(enum_value)) {\n"
     "      return;\n"
     "    }\n"
     /* it's not a valid enum value => dereference it! */
     "    m_p = param.get_referenced_param();\n"
     "  }\n", unknown_value);
  }
  src = mputprintf(src,
     "  if (%sget_type()!=Module_Param::MP_Enumerated) param.type_error(\"enumerated value\", \"%s\");\n"
     "  enum_value = str_to_enum(%sget_enumerated());\n"
     "  if (!is_valid_enum(enum_value)) {\n"
     "    param.error(\"Invalid enumerated value for type %s.\");\n"
     "  }\n"
     "}\n\n", use_runtime_2 ? "m_p->" : "param.", dispname,
     use_runtime_2 ? "m_p->" : "param.", dispname);
  
  if (use_runtime_2) {
    def = mputstr(def, "Module_Param* get_param(Module_Param_Name& param_name) const;\n");
    src = mputprintf
      (src,
      "Module_Param* %s::get_param(Module_Param_Name& /* param_name */) const\n"
      "{\n"
      "  if (!is_bound()) {\n"
      "    return new Module_Param_Unbound();\n"
      "  }\n"
      "  return new Module_Param_Enumerated(mcopystr(enum_to_str(enum_value)));\n"
      "}\n\n", name);
  }

  /* encoders/decoders */
  def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
  src = mputprintf(src,
    "void %s::encode_text(Text_Buf& text_buf) const\n"
    "{\n"
    "if (enum_value == %s) "
    "TTCN_error(\"Text encoder: Encoding an unbound value of enumerated "
    "type %s.\");\n"
    "text_buf.push_int(enum_value);\n"
    "}\n\n", name, unbound_value, dispname);

  def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
  src = mputprintf(src,
    "void %s::decode_text(Text_Buf& text_buf)\n"
    "{\n"
    "enum_value = (%s)text_buf.pull_int().get_val();\n"
    "if (!is_valid_enum(enum_value)) "
    "TTCN_error(\"Text decoder: Unknown numeric value %%d was "
    "received for enumerated type %s.\", enum_value);\n"
    "}\n\n", name, enum_type, dispname);

  /* BER functions */
  if(ber_needed || raw_needed || text_needed || xer_needed || json_needed)
    def_encdec(name, &def, &src, ber_needed,
               raw_needed,text_needed, xer_needed, json_needed, TRUE);
  if(ber_needed) {
    src=mputprintf(src,
       "ASN_BER_TLV_t* %s::BER_encode_TLV(const TTCN_Typedescriptor_t&"
       " p_td, unsigned p_coding) const\n"
       "{\n"
       "  BER_chk_descr(p_td);\n"
       "  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());\n"
       "  if(!new_tlv) {\n"
       "    BER_encode_chk_enum_valid(p_td, is_valid_enum(enum_value),"
       " enum_value);\n"
       "    new_tlv=BER_encode_TLV_INTEGER(p_coding, enum_value);\n"
       "  }\n"
       "  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);\n"
       "  return new_tlv;\n"
       "}\n"
       "\n"
       "boolean %s::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,"
       " const ASN_BER_TLV_t& p_tlv, unsigned L_form)\n"
       "{\n"
       "  enum_value = %s;\n"
       "  BER_chk_descr(p_td);\n"
       "  ASN_BER_TLV_t stripped_tlv;\n"
       "  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);\n"
       "  TTCN_EncDec_ErrorContext ec(\"While decoding ENUMERATED type %s: "
	"\");\n"
       "  int tmp_mfr;\n"
       "  if (BER_decode_TLV_INTEGER(stripped_tlv, L_form, tmp_mfr)) {\n"
       "    BER_decode_chk_enum_valid(p_td, is_valid_enum(tmp_mfr), tmp_mfr);\n"
       "    enum_value = (%s)tmp_mfr;\n"
       "    return TRUE;\n"
       "  } else return FALSE;\n"
       "}\n\n"
       , name, name, unbound_value, dispname, enum_type);
  } /* if ber_needed */
  /* new TEXT functions */
  if(text_needed){
    src = mputprintf(src,
      "int %s::TEXT_encode(const TTCN_Typedescriptor_t& p_td,"
      "TTCN_Buffer& p_buf) const{\n"
      "int encoded_length=0;\n"
      "if(p_td.text->begin_encode){\n"
      "  p_buf.put_cs(*p_td.text->begin_encode);\n"
      "  encoded_length+=p_td.text->begin_encode->lengthof();\n"
      "}\n"
      "  if (enum_value == %s) {\n"
      "    TTCN_EncDec_ErrorContext::error\n"
      "      (TTCN_EncDec::ET_UNBOUND, \"Encoding an unbound value of "
      "enumerated type %s.\");\n"
      "    if(p_td.text->end_encode){\n"
      "      p_buf.put_cs(*p_td.text->end_encode);\n"
      "      encoded_length+=p_td.text->end_encode->lengthof();\n"
      "    }\n"
      "    return encoded_length;\n"
      "  }\n"
      "if(p_td.text->val.enum_values==NULL){\n"
      "  int len=strlen(enum_to_str(enum_value));\n"
      "  p_buf.put_s(len,(const unsigned char*)enum_to_str(enum_value));\n"
      "  encoded_length+=len;\n"
      "} else {\n"
      "switch(enum_value){\n"
      , name, unbound_value, dispname
      );
    for(i=0;i<edef->nElements;i++){
      src = mputprintf(src,
        "case %s: \n"
        "if(p_td.text->val.enum_values[%lu].encode_token){\n"
        " p_buf.put_cs(*p_td.text->val.enum_values[%lu].encode_token);\n"
        " encoded_length+=p_td.text->val.enum_values[%lu]"
        ".encode_token->lengthof();\n"
        "} else { "
        "  int len=strlen(enum_to_str(enum_value));\n"
        "  p_buf.put_s(len,(const unsigned char*)enum_to_str(enum_value));\n"
        "  encoded_length+=len;\n"
        "}\n"
        "break;\n"
         ,edef->elements[i].name,
         (unsigned long) i,(unsigned long) i,(unsigned long) i
       );
    }

    src = mputstr(src,
      " default:\n"
      "break;\n"
      "}\n"
      "}\n"
      " if(p_td.text->end_encode){\n"
      "   p_buf.put_cs(*p_td.text->end_encode);\n"
      "   encoded_length+=p_td.text->end_encode->lengthof();\n"
      " }\n"
      " return encoded_length;\n"
      "}\n"
      );
    src = mputprintf(src,
      "int %s::TEXT_decode(const TTCN_Typedescriptor_t& p_td,"
      " TTCN_Buffer& p_buf, Limit_Token_List&, boolean no_err, boolean){\n"
      "  int decoded_length=0;\n"
      "  int str_len=0;\n"
      "  if(p_td.text->begin_decode){\n"
      "    int tl;\n"
      "    if((tl=p_td.text->begin_decode->match_begin(p_buf))<0){\n"
      "          if(no_err)return -1;\n"
      "          TTCN_EncDec_ErrorContext::error\n"
      "              (TTCN_EncDec::ET_TOKEN_ERR, \"The specified token '%%s'"
      " not found for '%%s': \",(const char*)*(p_td.text->begin_decode)"
      ", p_td.name);\n"
      "          return 0;\n"
      "        }\n"
      "    decoded_length+=tl;\n"
      "    p_buf.increase_pos(tl);\n"
      "  }\n"
      "  if(p_buf.get_read_len()<1 && no_err) return -1;\n"
      ,name
     );


    for(i=0;i<edef->nElements;i++){
      src = mputprintf(src,
       "if((str_len=p_td.text->val.enum_values[%lu].decode_token->"
       "match_begin(p_buf))!=-1){\n"
       "  enum_value=%s;\n"
       "} else "
       ,(unsigned long) i,edef->elements[i].name
      );
    }

    src = mputstr(src,
      " {\n"
      "    if(no_err)return -1;\n"
      "    TTCN_EncDec_ErrorContext::error"
      "(TTCN_EncDec::ET_TOKEN_ERR, \"No enum token found for '%s': \""
      ",p_td.name);\n"
      "    return decoded_length;\n"
      "}\n"
      "  decoded_length+=str_len;\n"
      "  p_buf.increase_pos(str_len);\n"
      "  if(p_td.text->end_decode){\n"
      "    int tl;\n"
      "    if((tl=p_td.text->end_decode->match_begin(p_buf))<0){\n"
      "          if(no_err)return -1;\n"
      "          TTCN_EncDec_ErrorContext::error"
      "(TTCN_EncDec::ET_TOKEN_ERR, \"The specified token '%s'"
      " not found for '%s': \",(const char*)*(p_td.text->end_decode)"
      ",p_td.name);\n"
      "          return decoded_length;\n"
      "        }\n"
      "    decoded_length+=tl;\n"
      "    p_buf.increase_pos(tl);\n"
      "  }\n"
      "  return decoded_length;\n"
      "}\n"
     );
  }
  /* new RAW functions */
  if (raw_needed) {
    int min_bits = 0;
    int max_val = edef->firstUnused;
    size_t a;
    for (a = 0; a < edef->nElements; a++) {
      int val = edef->elements[a].value;
      if (abs(max_val) < abs(val)) max_val = val;
    }
    if (max_val < 0) {
      min_bits = 1;
      max_val = -max_val;
    }
    while (max_val) {
      min_bits++;
      max_val /= 2;
    }
    src = mputprintf(src,
      "int %s::RAW_decode(const TTCN_Typedescriptor_t& p_td,TTCN_Buffer& p_buf,"
      "int limit, raw_order_t top_bit_ord, boolean no_err, int, boolean)\n"
      "{\n"
      "  int decoded_value = 0;\n"
      "  int decoded_length = RAW_decode_enum_type(p_td, p_buf, limit, "
      "top_bit_ord, decoded_value, %d, no_err);\n"
      "  if (decoded_length < 0) return decoded_length;\n"
      "  if (is_valid_enum(decoded_value)) "
      "enum_value = (%s)decoded_value;\n"
      "  else {\n"
      "    if(no_err){\n"
      "     return -1;\n"
      "    } else {\n"
      "    TTCN_EncDec_ErrorContext::error\n"
      "      (TTCN_EncDec::ET_ENC_ENUM, \"Invalid enum value '%%d'"
      " for '%%s': \",decoded_value, p_td.name);\n"
      "    enum_value = %s;\n"
      "    }\n"
      "  }\n"
      "  return decoded_length;\n"
      "}\n\n", name, min_bits, enum_type, unknown_value);
    src = mputprintf(src,
      "int %s::RAW_encode(const TTCN_Typedescriptor_t& p_td, "
      "RAW_enc_tree& myleaf) const\n"
      "{\n"
      "  return RAW_encode_enum_type(p_td, myleaf, (int)enum_value, %d);\n"
      "}\n\n", name, min_bits);
  } /* if raw_needed */

  if (xer_needed) { /* XERSTUFF encoder codegen for enum */
    /* FIXME This is redundant,
     * because the code is identical to BaseType::can_start()
     * and enum types are derived from BaseType or EnumType (which, in turn,
     * is derived from BaseType). However, the declaration of can_start()
     * is written by def_encdec() in encdec.c, which doesn't know
     * that this is an enum type. Maybe we need to pass an is_enum to
     * def_encdec, and then we can omit generating can_start() for enums.
     */
    src = mputprintf(src,
      "boolean %s::can_start(const char *name, const char *uri, "
      "const XERdescriptor_t& xd, unsigned int flavor, unsigned int) {\n"
      "  boolean exer = is_exer(flavor);\n"
      "  return check_name(name, xd, exer) && (!exer || check_namespace(uri, xd));\n"
      "}\n\n"
      , name
      );

    src = mputprintf(src,
      "int %s::XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,"
      " unsigned int p_flavor, unsigned int, int p_indent, embed_values_enc_struct_t*) const\n"
      "{\n"
      "  int encoded_length=(int)p_buf.get_len();\n"
      "  const boolean e_xer = is_exer(p_flavor);\n"
      "  p_flavor |= (SIMPLE_TYPE | BXER_EMPTY_ELEM);\n"
      "  if (begin_xml(p_td, p_buf, p_flavor, p_indent, FALSE) == -1) "
      "--encoded_length;\n"
      "  if (!e_xer) p_buf.put_c('<');\n"
      , name
    );
    if (edef->xerUseNumber) {
      src = mputstr(src,
        "  if (e_xer) {\n" /* compile-time instead of p_td.useNumber */
        "    char sval[24];\n" /* unsigned 64 bits fit in 20 characters */
        "    int slen = snprintf(sval, 24, \"%d\", enum_value);\n"
        "    if (slen > 0) p_buf.put_s((size_t)slen, (const unsigned char*)sval);\n"
        "  }\n"
        "  else" /* no newline, will take over following curly */
      );
    }
    src = mputprintf(src,
      "  {\n"
      "    const char * enumval = enum_to_str(enum_value%s);\n"
      "    p_buf.put_s(strlen(enumval), (const unsigned char*)enumval);\n"
      "  }\n"
      "  if (!e_xer) p_buf.put_s(2, (const unsigned char*)\"/>\");\n"
      "  end_xml(p_td, p_buf, p_flavor, p_indent, FALSE);\n"
      , edef->xerText ? ", e_xer" : ""
    );
    src = mputstr(src,
      "  return (int)p_buf.get_len() - encoded_length;\n"
      "}\n\n");

    src = mputprintf(src, /* XERSTUFF decoder codegen for enum */
#ifndef NDEBUG
      "// written by %s in " __FILE__ " at %d\n"
#endif
      "int %s::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& p_reader,"
      " unsigned int p_flavor,  unsigned int /*p_flavor2*/, embed_values_dec_struct_t*)\n"
      "{\n"
      "  int rd_ok = 1, type;\n"

      "  const boolean e_xer = is_exer(p_flavor);\n"
      "  const boolean name_tag = !((!e_xer && is_record_of(p_flavor)) || (e_xer && ((p_td.xer_bits & UNTAGGED) ||(is_record_of(p_flavor) && is_exerlist(p_flavor)))));\n"
      "  if (e_xer && ((p_td.xer_bits & XER_ATTRIBUTE) || is_exerlist(p_flavor))) {\n"
      "    if ((p_td.xer_bits & XER_ATTRIBUTE)) verify_name(p_reader, p_td, e_xer);\n"
      "    const char * value = (const char *)p_reader.Value();\n"
      "    if (value) {\n"
#ifndef NDEBUG
      , __FUNCTION__, __LINE__
#endif
      , name);
    if (edef->xerUseNumber) {
      src = mputprintf(src,
        "        int tempvalue;\n"
        "        sscanf(value, \"%%d\", &tempvalue);\n"
        "        if (is_valid_enum(tempvalue)) enum_value = (%s)tempvalue;\n" /* static_cast would be nice */
        "        else enum_value = %s;\n", enum_type, unknown_value
      );
    }
    else {
      src = mputstr(src, "        enum_value = str_to_enum(value);\n");
    }
    src = mputstr(src,
      "    }\n"
      /* The caller should do AdvanceAttribute() */
      "  }\n"
      "  else {\n"
      "    if (name_tag)"
      /* Go past the opening tag with the type name */
      "      for (; rd_ok == 1; rd_ok = p_reader.Read()) {\n"
      "        type = p_reader.NodeType();\n"
      "        if (XML_READER_TYPE_ELEMENT == type) {\n"
      "          rd_ok = p_reader.Read();\n"
      "          break;\n"
      "        }\n"
      "      }\n"
      /* Go to the element with the actual data (EmptyElementEnumerated) */
      "    for (; rd_ok == 1; rd_ok = p_reader.Read()) {\n"
      "      type = p_reader.NodeType();\n"
      "      if (!e_xer && XML_READER_TYPE_ELEMENT == type) break;\n"
      "      if (XML_READER_TYPE_TEXT == type) break;\n"
      "    }\n"
      "    const char *local_name = e_xer ? (const char *)p_reader.Value() : (const char *)p_reader.Name();\n"
      /*                                       TextEnumerated                EmptyElementEnumerated */
      "    if (!local_name) ; else");
    if (edef->xerUseNumber) {
      src = mputprintf(src,
        "    if (e_xer) {\n"
        "      int tempvalue;\n"
        "      sscanf(local_name, \"%%d\", &tempvalue);\n"
        "      if (is_valid_enum(tempvalue)) enum_value = (%s)tempvalue;\n" /* static_cast would be nice */
        "      else enum_value = %s;\n"
        "    }\n"
        "    else" /* no newline */ , enum_type, unknown_value
        );
    }
    {
      src = mputstr(src,
        "    {\n"
        "      for (; '\\t'==*local_name || '\\n'==*local_name; ++local_name) ;\n" /* crutch while default-for-empty always puts in a newline */
        "      enum_value = str_to_enum(local_name);\n"
        "    }\n");
    }
    src = mputprintf(src,
      "    if (name_tag)\n"
      "      for (rd_ok = p_reader.Read(); rd_ok == 1; rd_ok = p_reader.Read()) {\n"
      "        type = p_reader.NodeType();\n"
      "        if (XML_READER_TYPE_END_ELEMENT == type) {\n"
      "          rd_ok = p_reader.Read();\n"
      "          break;\n"
      "        }\n"
      "      }\n"
      "    else rd_ok = p_reader.Read();\n"
      "  }\n"
      "  if (e_xer && (p_flavor & EXIT_ON_ERROR) && %s == enum_value) clean_up();\n" // set to unbound if decoding failed
      "  int decoded_length = 0;\n"
      "  return decoded_length;\n"
      "}\n\n"
      , unknown_value);
  }
  if (json_needed) {
    // JSON encode
    src = mputprintf(src,
      "int %s::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const\n"
      "{\n"
      "  if (enum_value == %s) {\n"
      "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,\n"
      "      \"Encoding an unbound value of enumerated type %s.\");\n"
      "    return -1;\n"
      "  }\n\n"
      "  char* tmp_str = mprintf(\"\\\"%%s\\\"\", enum_to_str(enum_value));\n"
      "  int enc_len = p_tok.put_next_token(JSON_TOKEN_STRING, tmp_str);\n"
      "  Free(tmp_str);\n"
      "  return enc_len;\n"
      "}\n\n"
      , name, unbound_value, dispname);
    
    // JSON decode
    src = mputprintf(src,
      "int %s::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)\n"
      "{\n"
      "  json_token_t token = JSON_TOKEN_NONE;\n"
      "  char* value = 0;\n"
      "  size_t value_len = 0;\n"
      "  boolean error = FALSE;\n"
      "  size_t dec_len = 0;\n"
      "  boolean use_default = p_td.json->default_value && 0 == p_tok.get_buffer_length();\n"
      "  if (use_default) {\n"
      // No JSON data in the buffer -> use default value
      "    value = const_cast<char*>(p_td.json->default_value);\n"
      "    value_len = strlen(value);\n"
      "  } else {\n"
      "    dec_len = p_tok.get_next_token(&token, &value, &value_len);\n"
      "  }\n"
      "  if (JSON_TOKEN_ERROR == token) {\n"
      "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, \"\");\n"
      "    return JSON_ERROR_FATAL;\n"
      "  }\n"
      "  else if (JSON_TOKEN_STRING == token || use_default) {\n"
      "    if (use_default || (value_len > 2 && value[0] == '\\\"' && value[value_len - 1] == '\\\"')) {\n"
      "      if (!use_default) value[value_len - 1] = 0;\n"
      "      enum_value = str_to_enum(value + (use_default ? 0 : 1));\n"
      "      if (!use_default) value[value_len - 1] = '\\\"';\n"
      "      if (%s == enum_value) {\n"
      "        error = TRUE;\n"
      "      }\n"
      "    } else {\n"
      "      error = TRUE;\n"
      "    }\n"
      "  } else {\n"
      "    enum_value = %s;\n"
      "    return JSON_ERROR_INVALID_TOKEN;\n"
      "  }\n\n"
      "  if (error) {\n"
      "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FORMAT_ERROR, \"string\", \"enumerated\");\n"
      "    enum_value = %s;\n"
      "    return JSON_ERROR_FATAL;\n"
      "  }\n"
      "  return (int)dec_len;\n"
      "}\n\n"
      , name, unknown_value, unbound_value, unbound_value);
  }
  /* end of class */
  def = mputstr(def, "};\n\n");

  output->header.class_defs = mputstr(output->header.class_defs, def);
  Free(def);
  output->source.methods = mputstr(output->source.methods, src);
  Free(src);


  Free(enum_type);
  Free(qualified_enum_type);
  Free(unknown_value);
  Free(unbound_value);
}

void defEnumTemplate(const enum_def *edef, output_struct *output)
{
  char *def = NULL, *src = NULL;
  const char *name = edef->name, *dispname = edef->dispname;

  char *enum_type, *unbound_value, *unknown_value;
  enum_type = mprintf("%s::enum_type", name);
  unbound_value = mprintf("%s::UNBOUND_VALUE", name);
  unknown_value = mprintf("%s::UNKNOWN_VALUE", name);

  /* Class declaration */
  output->header.class_decls = mputprintf(output->header.class_decls,
    "class %s_template;\n", name);

  /* Class definition */
  def = mputprintf(def, "class %s_template : public Base_Template {\n"
    "union {\n"
    "%s single_value;\n"
    "struct {\n"
    "unsigned int n_values;\n"
    "%s_template *list_value;\n"
    "} value_list;\n"
    "};\n\n", name, enum_type, name);

  /* private members */
  def = mputprintf(def, "void copy_template(const %s_template& "
                   "other_value);\n", name);
  src = mputprintf(src,
    "void %s_template::copy_template(const %s_template& "
    "other_value)\n"
    "{\n"
    "set_selection(other_value);\n"
    "switch (template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "single_value = other_value.single_value;\n"
    "break;\n"
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
    "TTCN_error(\"Copying an uninitialized/unsupported template of "
    "enumerated type %s.\");\n"
    "}\n"
    "}\n\n", name, name, name, dispname);

  def = mputstr(def, "\npublic:\n");

  /* constructors */
  def = mputprintf(def, "%s_template();\n", name);
  src = mputprintf(src,
    "%s_template::%s_template()\n"
    "{\n"
    "}\n\n", name, name);

  def = mputprintf(def, "%s_template(template_sel other_value);\n", name);
  src = mputprintf(src,
    "%s_template::%s_template(template_sel other_value)\n"
    " : Base_Template(other_value)\n"
    "{\n"
    "check_single_selection(other_value);\n"
    "}\n\n", name, name);

  def = mputprintf(def, "%s_template(int other_value);\n", name);
  src = mputprintf(src,
    "%s_template::%s_template(int other_value)\n"
    " : Base_Template(SPECIFIC_VALUE)\n"
    "{\n"
    "if (!%s::is_valid_enum(other_value)) "
    "TTCN_error(\"Initializing a template of enumerated type %s with "
    "unknown numeric value %%d.\", other_value);\n"
    "single_value = (%s)other_value;\n"
    "}\n\n", name, name, name, dispname, enum_type);

  def = mputprintf(def, "%s_template(%s other_value);\n", name, enum_type);
  src = mputprintf(src,
    "%s_template::%s_template(%s other_value)\n"
    " : Base_Template(SPECIFIC_VALUE)\n"
    "{\n"
    "single_value = other_value;\n"
    "}\n\n", name, name, enum_type);

  def = mputprintf(def, "%s_template(const %s& other_value);\n", name, name);
  src = mputprintf(src,
    "%s_template::%s_template(const %s& other_value)\n"
    " : Base_Template(SPECIFIC_VALUE)\n"
    "{\n"
    "if (other_value.enum_value == %s) "
    "TTCN_error(\"Creating a template from an unbound value of enumerated "
    "type %s.\");\n"
    "single_value = other_value.enum_value;\n"
    "}\n\n", name, name, name, unbound_value, dispname);

  def = mputprintf(def, "%s_template(const OPTIONAL<%s>& other_value);\n",
                   name, name);
  src = mputprintf(src,
    "%s_template::%s_template(const OPTIONAL<%s>& other_value)\n"
    "{\n"
    "switch (other_value.get_selection()) {\n"
    "case OPTIONAL_PRESENT:\n"
    "set_selection(SPECIFIC_VALUE);\n"
    "single_value = (%s)(const %s&)other_value;\n"
    "break;\n"
    "case OPTIONAL_OMIT:\n"
    "set_selection(OMIT_VALUE);\n"
    "break;\n"
    "default:\n"
    "TTCN_error(\"Creating a template of enumerated type %s from an unbound "
      "optional field.\");\n"
    "}\n"
    "}\n\n", name, name, name, enum_type, name, dispname);

  def = mputprintf(def, "%s_template(const %s_template& other_value);\n",
                   name, name);
  src = mputprintf(src,
    "%s_template::%s_template(const %s_template& other_value)\n"
    " : Base_Template()\n"
    "{\n"
    "copy_template(other_value);\n"
    "}\n\n", name, name, name);

  /* destructor */
  def = mputprintf(def, "~%s_template();\n\n", name);
  src = mputprintf(src,
    "%s_template::~%s_template()\n"
    "{\n"
    "clean_up();\n"
    "}\n\n", name, name);

      /* is_bound */
    def = mputstr(def, "boolean is_bound() const;\n");
    src = mputprintf(src, "boolean %s_template::is_bound() const\n"
        "{\n"
        "if (template_selection == UNINITIALIZED_TEMPLATE && !is_ifpresent) "
        "return FALSE;\n", name);
    src = mputstr(src, "return TRUE;\n"
        "}\n\n");

    /* is_value */
    def = mputstr(def, "boolean is_value() const;\n");
    src = mputprintf(src, "boolean %s_template::is_value() const\n"
        "{\n"
        "if (template_selection != SPECIFIC_VALUE || is_ifpresent) "
        "return FALSE;\n"
        "return single_value != %s;\n"
        "}\n\n", name, unbound_value);

  /* clean_up */
  def = mputstr(def, "void clean_up();\n");
  src = mputprintf(src,
    "void %s_template::clean_up()\n"
    "{\n"
    "if (template_selection == VALUE_LIST || "
    "template_selection == COMPLEMENTED_LIST) "
    "delete [] value_list.list_value;\n"
    "template_selection = UNINITIALIZED_TEMPLATE;\n"
    "}\n\n", name);

  /* assignment operators */
  def = mputprintf(def, "%s_template& operator=(template_sel other_value);\n",
                   name);
  src = mputprintf(src,
    "%s_template& %s_template::operator=(template_sel other_value)\n"
    "{\n"
    "check_single_selection(other_value);\n"
    "clean_up();\n"
    "set_selection(other_value);\n"
    "return *this;\n"
    "}\n\n", name, name);

  def = mputprintf(def, "%s_template& operator=(int other_value);\n",
                   name);
  src = mputprintf(src,
    "%s_template& %s_template::operator=(int other_value)\n"
    "{\n"
    "if (!%s::is_valid_enum(other_value)) "
    "TTCN_warning(\"Assigning unknown numeric value %%d to a template "
    "of enumerated type %s.\", other_value);\n"
    "clean_up();\n"
    "set_selection(SPECIFIC_VALUE);\n"
    "single_value = (%s)other_value;\n"
    "return *this;\n"
    "}\n\n", name, name, name, dispname, enum_type);

  def = mputprintf(def, "%s_template& operator=(%s other_value);\n",
                   name, enum_type);
  src = mputprintf(src,
    "%s_template& %s_template::operator=(%s other_value)\n"
    "{\n"
    "clean_up();\n"
    "set_selection(SPECIFIC_VALUE);\n"
    "single_value = other_value;\n"
    "return *this;\n"
    "}\n\n", name, name, enum_type);

  def = mputprintf(def, "%s_template& operator=(const %s& other_value);\n",
                   name, name);
  src = mputprintf(src,
    "%s_template& %s_template::operator=(const %s& other_value)\n"
    "{\n"
    "if (other_value.enum_value == %s) "
    "TTCN_error(\"Assignment of an unbound value of enumerated type %s to a "
    "template.\");\n"
    "clean_up();\n"
    "set_selection(SPECIFIC_VALUE);\n"
    "single_value = other_value.enum_value;\n"
    "return *this;\n"
    "}\n\n", name, name, name, unbound_value, dispname);

  def = mputprintf(def, "%s_template& operator=(const OPTIONAL<%s>& "
                   "other_value);\n", name, name);
  src = mputprintf(src,
    "%s_template& %s_template::operator=(const OPTIONAL<%s>& other_value)\n"
    "{\n"
    "clean_up();\n"
    "switch (other_value.get_selection()) {\n"
    "case OPTIONAL_PRESENT:\n"
    "set_selection(SPECIFIC_VALUE);\n"
    "single_value = (%s)(const %s&)other_value;\n"
    "break;\n"
    "case OPTIONAL_OMIT:\n"
    "set_selection(OMIT_VALUE);\n"
    "break;\n"
    "default:\n"
    "TTCN_error(\"Assignment of an unbound optional field to a template of "
      "enumerated type %s.\");\n"
    "}\n"
    "return *this;\n"
    "}\n\n", name, name, name, enum_type, name, dispname);

  def = mputprintf(def, "%s_template& operator=(const %s_template& "
                   "other_value);\n\n", name, name);
  src = mputprintf(src,
    "%s_template& %s_template::operator=(const %s_template& other_value)\n"
    "{\n"
    "if (&other_value != this) {\n"
    "clean_up();\n"
    "copy_template(other_value);\n"
    "}\n"
    "return *this;\n"
    "}\n\n", name, name, name);

  /* match operators */
  def = mputprintf(def, "boolean match(%s other_value, boolean legacy = FALSE) "
    "const;\n", enum_type);
  src = mputprintf(src,
    "boolean %s_template::match(%s other_value, boolean) const\n"
    "{\n"
    "switch (template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "return single_value == other_value;\n"
    "case OMIT_VALUE:\n"
    "return FALSE;\n"
    "case ANY_VALUE:\n"
    "case ANY_OR_OMIT:\n"
    "return TRUE;\n"
    "case VALUE_LIST:\n"
    "case COMPLEMENTED_LIST:\n"
    "for (unsigned int list_count = 0; list_count < value_list.n_values; "
    "list_count++)\n"
    "if (value_list.list_value[list_count].match(other_value)) "
    "return template_selection == VALUE_LIST;\n"
    "return template_selection == COMPLEMENTED_LIST;\n"
    "default:\n"
    "TTCN_error(\"Matching an uninitialized/unsupported template of "
    "enumerated type %s.\");\n"
    "}\n"
    "return FALSE;\n"
    "}\n\n", name, enum_type, dispname);

  def = mputprintf(def, "boolean match(const %s& other_value, boolean legacy "
    "= FALSE) const;\n", name);
  src = mputprintf(src,
    "boolean %s_template::match(const %s& other_value, boolean) const\n"
    "{\n"
    "if (other_value.enum_value == %s) "
    "TTCN_error(\"Matching a template of enumerated type %s with an unbound "
    "value.\");\n"
    "return match(other_value.enum_value);\n"
    "}\n\n", name, name, unbound_value, dispname);

  /* valueof operator */
  def = mputprintf(def, "%s valueof() const;\n", enum_type);
  src = mputprintf(src,
    "%s %s_template::valueof() const\n"
    "{\n"
    "if (template_selection != SPECIFIC_VALUE || is_ifpresent) "
    "TTCN_error(\"Performing a valueof or send operation on a "
    "non-specific template of enumerated type %s.\");\n"
    "return single_value;\n"
    "}\n\n", enum_type, name, dispname);

  /* value list handling operators */
  def = mputstr
    (def,
     "void set_type(template_sel template_type, unsigned int list_length);\n");
  src = mputprintf(src,
    "void %s_template::set_type(template_sel template_type, "
    "unsigned int list_length)\n"
    "{\n"
    "if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST) "
    "TTCN_error(\"Setting an invalid list type for a template of enumerated "
    "type %s.\");\n"
    "clean_up();\n"
    "set_selection(template_type);\n"
    "value_list.n_values = list_length;\n"
    "value_list.list_value = new %s_template[list_length];\n"
    "}\n\n", name, dispname, name);

  def = mputprintf(def, "%s_template& list_item(unsigned int list_index);\n",
    name);
  src = mputprintf(src,
    "%s_template& %s_template::list_item(unsigned int list_index)\n"
    "{\n"
    "if (template_selection != VALUE_LIST && "
    "template_selection != COMPLEMENTED_LIST) "
    "TTCN_error(\"Accessing a list element in a non-list template of "
    "enumerated type %s.\");\n"
    "if (list_index >= value_list.n_values) "
    "TTCN_error(\"Index overflow in a value list template of enumerated type "
    "%s.\");\n"
    "return value_list.list_value[list_index];\n"
    "}\n\n", name, name, dispname, dispname);

  if (use_runtime_2) {
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

  /* logging functions */
  def = mputstr(def, "void log() const;\n");
  src = mputprintf
    (src,
     "void %s_template::log() const\n"
     "{\n"
     "switch (template_selection) {\n"
     "case SPECIFIC_VALUE:\n"
     "TTCN_Logger::log_event_enum(%s::enum_to_str(single_value), single_value);\n"
     "break;\n"
     "case COMPLEMENTED_LIST:\n"
     "TTCN_Logger::log_event_str(\"complement \");\n"
     "case VALUE_LIST:\n"
     "TTCN_Logger::log_char('(');\n"
     "for (unsigned int elem_count = 0; elem_count < value_list.n_values; "
     "elem_count++) {\n"
     "if (elem_count > 0) TTCN_Logger::log_event_str(\", \");\n"
     "value_list.list_value[elem_count].log();\n"
     "}\n"
     "TTCN_Logger::log_char(')');\n"
     "break;\n"
     "default:\n"
     "log_generic();\n"
     "}\n"
     "log_ifpresent();\n"
     "}\n\n", name, name);

  def = mputprintf(def, "void log_match(const %s& match_value, "
    "boolean legacy = FALSE) const;\n", name);
  src = mputprintf(src,
    "void %s_template::log_match(const %s& match_value, boolean) const\n"
    "{\n"
    "match_value.log();\n"
    "TTCN_Logger::log_event_str(\" with \");\n"
    "log();\n"
    "if (match(match_value)) TTCN_Logger::log_event_str(\" matched\");\n"
    "else TTCN_Logger::log_event_str(\" unmatched\");\n"
    "}\n\n", name, name);

  /* encoding/decoding functions */
  def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
  src = mputprintf(src,
    "void %s_template::encode_text(Text_Buf& text_buf) "
    "const\n"
    "{\n"
    "encode_text_base(text_buf);\n"
    "switch (template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "text_buf.push_int(single_value);\n"
    "case OMIT_VALUE:\n"
    "case ANY_VALUE:\n"
    "case ANY_OR_OMIT:\n"
    "break;\n"
    "case VALUE_LIST:\n"
    "case COMPLEMENTED_LIST:\n"
    "text_buf.push_int(value_list.n_values);\n"
    "for (unsigned int elem_count = 0; elem_count < value_list.n_values; "
    "elem_count++)\n"
    "value_list.list_value[elem_count].encode_text(text_buf);\n"
    "break;\n"
    "default:\n"
    "TTCN_error(\"Text encoder: Encoding an uninitialized/unsupported "
    "template of enumerated type %s.\");\n"
    "}\n"
    "}\n\n", name, dispname);

  def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
  src = mputprintf(src,
    "void %s_template::decode_text(Text_Buf& text_buf)\n"
    "{\n"
    "clean_up();\n"
    "decode_text_base(text_buf);\n"
    "switch (template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "single_value = (%s)text_buf.pull_int().get_val();\n"
    "if (!%s::is_valid_enum(single_value)) "
    "TTCN_error(\"Text decoder: Unknown numeric value %%d was "
    "received for a template of enumerated type %s.\", single_value);\n"
    "case OMIT_VALUE:\n"
    "case ANY_VALUE:\n"
    "case ANY_OR_OMIT:\n"
    "break;\n"
    "case VALUE_LIST:\n"
    "case COMPLEMENTED_LIST:\n"
    "value_list.n_values = text_buf.pull_int().get_val();\n"
    "value_list.list_value = new %s_template[value_list.n_values];\n"
    "for (unsigned int elem_count = 0; elem_count < value_list.n_values; "
    "elem_count++)\n"
    "value_list.list_value[elem_count].decode_text(text_buf);\n"
    "break;\n"
    "default:\n"
    "TTCN_error(\"Text decoder: An unknown/unsupported selection was "
    "received for a template of enumerated type %s.\");\n"
    "}\n"
    "}\n\n", name, enum_type, name, dispname, name, dispname);

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
    "for (unsigned int i=0; i<value_list.n_values; i++)\n"
    "if (value_list.list_value[i].match_omit())\n"
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
    "  param.basic_check(Module_Param::BC_TEMPLATE, \"enumerated template\");\n"
    "  Module_Param_Ptr m_p = &param;\n", name);
  if (use_runtime_2) {
    src = mputprintf(src,
    "  if (param.get_type() == Module_Param::MP_Reference) {\n"
    /* enumerated values are also treated as references (containing only 1 name) by the parser;
       first check if the reference name is a valid enumerated value */
    "    char* enum_name = param.get_enumerated();\n"
    /* get_enumerated() returns NULL if the reference contained more than one name */
    "    %s enum_val = (enum_name != NULL) ? %s::str_to_enum(enum_name) : %s;\n"
    "    if (%s::is_valid_enum(enum_val)) {\n"
    "      *this = enum_val;\n"
    "      is_ifpresent = param.get_ifpresent() || m_p->get_ifpresent();\n"
    "      return;\n"
    "    }\n"
    /* it's not a valid enum value => dereference it! */
    "    m_p = param.get_referenced_param();\n"
    "  }\n", enum_type, name, unknown_value, name);
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
    "  case Module_Param::MP_Enumerated: {\n"
    "    %s enum_val = %s::str_to_enum(m_p->get_enumerated());\n"
    "    if (!%s::is_valid_enum(enum_val)) {\n"
    "      param.error(\"Invalid enumerated value for type %s.\");\n"
    "    }\n"
    "    *this = enum_val;\n"
    "  } break;\n"
    "  default:\n"
    "    param.type_error(\"enumerated template\", \"%s\");\n"
    "  }\n"
    "  is_ifpresent = param.get_ifpresent()%s;\n"
    "}\n\n", name, enum_type, name, name, dispname, dispname,
    use_runtime_2 ? " || m_p->get_ifpresent()" : "");

  /* get_param(), RT2 only */
  if (use_runtime_2) {
    def = mputstr(def, "Module_Param* get_param(Module_Param_Name& param_name) const;\n");
    src = mputprintf
      (src,
      "Module_Param* %s_template::get_param(Module_Param_Name& param_name) const\n"
      "{\n"
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
      "  case SPECIFIC_VALUE:\n"
      "    m_p = new Module_Param_Enumerated(mcopystr(%s::enum_to_str(single_value)));\n"
      "    break;\n"
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
      "}\n\n", name, name);
  }

  if (!use_runtime_2) {
    /* check template restriction */
    def = mputstr(def, "void check_restriction(template_res t_res, "
      "const char* t_name=NULL, boolean legacy = FALSE) const;\n");
    src = mputprintf(src,
      "void %s_template::check_restriction(template_res t_res, const char* t_name,\n"
      "boolean legacy) const\n"
      "{\n"
      "if (template_selection==UNINITIALIZED_TEMPLATE) return;\n"
      "switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {\n"
      "case TR_VALUE:\n"
      "if (!is_ifpresent && template_selection==SPECIFIC_VALUE) return;\n"
      "break;\n"
      "case TR_OMIT:\n"
      "if (!is_ifpresent && (template_selection==OMIT_VALUE || "
        "template_selection==SPECIFIC_VALUE)) return;\n"
      "break;\n"
      "case TR_PRESENT:\n"
      "if (!match_omit(legacy)) return;\n"
      "break;\n"
      "default:\n"
      "return;\n"
      "}\n"
      "TTCN_error(\"Restriction `%%s' on template of type %%s violated.\", "
        "get_res_name(t_res), t_name ? t_name : \"%s\");\n"
      "}\n\n", name, dispname);
  }

  /* end of class */
  def = mputstr(def, "};\n\n");

  output->header.class_defs = mputstr(output->header.class_defs, def);
  Free(def);
  output->source.methods = mputstr(output->source.methods, src);
  Free(src);

  Free(enum_type);
  Free(unbound_value);
  Free(unknown_value);
}
