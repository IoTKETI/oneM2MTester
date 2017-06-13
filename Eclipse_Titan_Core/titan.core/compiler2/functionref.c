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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include <string.h>
#include "../common/memory.h"
#include "functionref.h"
#include "encdec.h"

#include "main.hh"
#include "error.h"

void defFunctionrefClass(const funcref_def *fdef, output_struct *output)
{
  char *def = NULL, *src = NULL;
  const char *name = fdef->name;
  const char *dispname = fdef->dispname;

  const char *return_type, *fat_string;

  if (fdef->return_type != NULL) return_type = fdef->return_type;
  else return_type = "void";

  switch (fdef->type) {
  case FUNCTION:
    fat_string = "function";
    break;
  case ALTSTEP:
    fat_string = "altstep";
    break;
  case TESTCASE:
    fat_string = "testcase";
    break;
  default:
    fat_string = NULL;
    FATAL_ERROR("defFunctionrefClass(): invalid type");
  }

  /* class declaration code */
  output->header.class_decls = mputprintf(output->header.class_decls,
    "class %s;\n", name);

  /* class definition */
  def = mputprintf(def,
#ifndef NDEBUG
    "// written by %s in " __FILE__ " at %d\n"
#endif
    "class %s : public Base_Type {\n"
    "public:\n"
#ifndef NDEBUG
      , __FUNCTION__, __LINE__
#endif
    , name);

  switch(fdef->type){
  case FUNCTION:
    def = mputstr(def, "typedef ");
    /* work-around for GCC versions earlier than 3.4:
     * parse error occurs within the typedef if the return type is the same
     * as the function type itself */
    if (!strcmp(name, return_type)) def = mputstr(def, "class ");
    def = mputprintf(def, "%s (*function_pointer)(%s);\n"
      "typedef void (*start_pointer)(const COMPONENT& "
	"component_reference", return_type, fdef->formal_par_list);
    if (fdef->formal_par_list[0] != '\0') def = mputstr(def, ", ");
    def = mputprintf(def, "%s);\n", fdef->formal_par_list);
    break;
  case ALTSTEP:
    def = mputprintf(def, "typedef void (*standalone_pointer)(%s);\n"
      "typedef Default_Base* (*activate_pointer)(%s);\n"
      "typedef alt_status (*function_pointer)(%s);\n", fdef->formal_par_list,
      fdef->formal_par_list, fdef->formal_par_list);
    break;
  case TESTCASE:
    def = mputprintf(def, "typedef verdicttype (*function_pointer)(%s);\n",
      fdef->formal_par_list);
    break;
  }

  def = mputprintf(def,
    "private:\n"
    "friend class %s_template;\n"
    "friend boolean operator==(%s::function_pointer value, "
      "const %s& other_value);\n"
    "function_pointer referred_function;\n"
    , name, name, name);

  /* default constructor */
  def = mputprintf(def,"public:\n"
    "%s();\n", name);
  src = mputprintf(src,"%s::%s()\n"
    "{\n"
    "referred_function = NULL;\n"
    "}\n\n", name, name);

    def = mputprintf(def,"%s(function_pointer other_value);\n", name);
    src = mputprintf(src,"%s::%s(function_pointer other_value)\n"
      "{\n"
      "referred_function = other_value;\n"
      "}\n\n", name, name);

  /* copy constructor */
  def = mputprintf(def,"%s(const %s& other_value);\n", name, name);
  src = mputprintf(src,"%s::%s(const %s& other_value)\n"
    ": Base_Type()" /* call the *default* constructor as before */
    "{\n"
    "other_value.must_bound(\"Copying an unbound %s value.\");\n"
    "referred_function = other_value.referred_function;\n"
    "}\n\n", name, name, name, dispname);

  /* operator= */
  def =mputprintf(def,"%s& operator=(function_pointer other_value);\n", name);
  src =mputprintf(src,"%s& %s::operator=(function_pointer other_value)\n"
    "{\n"
    "referred_function = other_value;\n"
    "return *this;\n"
    "}\n\n", name, name);

  def = mputprintf(def,"%s& operator=(const %s& other_value);\n",
    name, name);
  src = mputprintf(src,"%s& %s::operator=(const %s& other_value)\n"
    "{\n"
    "other_value.must_bound(\"Assignment of an unbound value.\");\n"
    "referred_function = other_value.referred_function;\n"
    "return *this;\n"
    "}\n\n", name, name, name);

  /* operator ==*/
  def = mputstr(def,"boolean operator==(function_pointer other_value) "
      "const;\n");
  src = mputprintf(src,"boolean %s::operator==(function_pointer other_value) "
      "const\n"
    "{\n"
    "must_bound(\"Unbound left operand of %s comparison.\");\n"
    "return referred_function == other_value;\n"
    "}\n\n", name, dispname);
  def = mputprintf(def,"boolean operator==(const %s& other_value) const;\n"
    , name);
  src = mputprintf(src,"boolean %s::operator==(const %s& other_value) const\n"
    "{\n"
    "must_bound(\"Unbound left operand of %s comparison.\");\n"
    "other_value.must_bound(\"Unbound right operand of %s comparison.\");\n"
    "return referred_function == other_value.referred_function;\n"
    "}\n\n", name, name, dispname, dispname);

  /* operator != */
  def = mputprintf(def,"inline boolean operator!=(function_pointer other_value)"
      " const\n"
    "{ return !(*this == other_value); }\n"
    "inline boolean operator!=(const %s& other_value) const\n"
    "{ return !(*this == other_value); }\n\n", name);

  switch(fdef->type) {
  case FUNCTION:
    def = mputprintf(def,"%s invoke(%s) const;\n"
      , return_type, fdef->formal_par_list);
    src = mputprintf(src,"%s %s::invoke(%s) const\n"
      "{\n"
      "must_bound(\"Call of unbound function.\");\n"
      "if(referred_function == "
        "(%s::function_pointer)Module_List::get_fat_null())\n"
      "TTCN_error(\"null reference cannot be invoked.\");\n"
      "%sreferred_function(%s);\n"
      "}\n\n", return_type, name, fdef->formal_par_list, name
        , fdef->return_type!= NULL? "return ": "", fdef->actual_par_list);
    if(fdef->is_startable) {
      def = mputprintf(def,"void start(const COMPONENT& component_reference%s"
          "%s) const;\n", strcmp(fdef->formal_par_list,"")?", ":""
        , fdef->formal_par_list);
      src = mputprintf(src,"void %s::start(const COMPONENT& "
          "component_reference%s%s) const\n{\n"
        "((%s::start_pointer)Module_List::lookup_start_by_function_address"
          "((genericfunc_t)referred_function))(component_reference%s%s);\n"
        "}\n\n", name, strcmp(fdef->formal_par_list,"")?", ":""
        , fdef->formal_par_list, name, strcmp(fdef->formal_par_list,"")?", ":""
        , fdef->actual_par_list);
    }
    break;
  case ALTSTEP:
    def = mputprintf(def,"void invoke_standalone(%s) const;\n",
      fdef->formal_par_list);
    src = mputprintf(src,"void %s::invoke_standalone(%s) const\n"
      "{\n"
      "((%s::standalone_pointer)"
        "Module_List::lookup_standalone_address_by_altstep_address("
        "(genericfunc_t)referred_function))(%s);\n"
      "}\n\n", name, fdef->formal_par_list, name, fdef->actual_par_list);

    def = mputprintf(def,"Default_Base *activate(%s) const;\n"
      , fdef->formal_par_list);
    src = mputprintf(src,"Default_Base *%s::activate(%s) const\n"
    "{\n"
    "return ((%s::activate_pointer)"
        "Module_List::lookup_activate_address_by_altstep_address("
        "(genericfunc_t)referred_function))(%s);\n"
    "}\n\n", name, fdef->formal_par_list, name, fdef->actual_par_list);

    def = mputprintf(def,"alt_status invoke(%s) const;\n"
      , fdef->formal_par_list);
    src = mputprintf(src,"alt_status %s::invoke(%s) const\n"
      "{\n"
      "must_bound(\"Call of an unbound altstep.\");\n"
      "if(referred_function == "
        "(%s::function_pointer)Module_List::get_fat_null())\n"
      "TTCN_error(\"null reference cannot be invoked.\");\n"
      "return referred_function(%s);\n"
      "}\n", name, fdef->formal_par_list, name, fdef->actual_par_list);

    break;
  case TESTCASE:
    def = mputprintf(def,"verdicttype execute(%s) const;\n",
      fdef->formal_par_list);
    src = mputprintf(src,"verdicttype %s::execute(%s) const\n"
      "{\n"
      "must_bound(\"Call of unbound testcase.\");\n"
      "if(referred_function == "
        "(%s::function_pointer)Module_List::get_fat_null())\n"
      "TTCN_error(\"null reference cannot be executed.\");\n"
      "return referred_function(%s);\n"
      "}\n\n", name, fdef->formal_par_list, name, fdef->actual_par_list);
    break;
  }

  /* bound check */
  def = mputstr(def,"inline boolean is_bound() "
    "const { return referred_function != NULL; }\n");
  /* value check */
  def = mputstr(def,"inline boolean is_value() "
    "const { return referred_function != NULL; }\n");
  def = mputstr(def,"inline void clean_up() "
    "{ referred_function = NULL; }\n");
  def = mputstr(def,"inline void must_bound(const char *err_msg) const\n"
    "{ if (referred_function == NULL) TTCN_error(\"%s\", err_msg); }\n\n");




  if (use_runtime_2) {
    /* functions in alternative runtime */
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

  /* log */
  def = mputstr(def,"void log() const;\n");
  src = mputprintf(src,"void %s::log() const\n"
    "{\n"
    "Module_List::log_%s((genericfunc_t)referred_function);\n"
    "}\n\n",name, fat_string);

  /* set_param */
  def = mputstr(def,"void set_param(Module_Param& param);\n");
  src = mputprintf(src,"void %s::set_param(Module_Param& param)\n"
    "{\n"
    "  param.error(\"Not supported.\");\n"
    "}\n\n", name);
  
  /* get_param, RT2 only */
  if (use_runtime_2) {
    def = mputstr(def,"Module_Param* get_param(Module_Param_Name& param_name) const;\n");
    src = mputprintf(src,"Module_Param* %s::get_param(Module_Param_Name& /* param_name */) const\n"
      "{\n"
      "  return NULL;\n"
      "}\n\n", name);
  }

  /* encode_text / decode_text */
  def = mputstr(def,"void encode_text(Text_Buf& text_buf) const;\n");
  src = mputprintf(src,"void %s::encode_text(Text_Buf&", name);
  if (fdef->runs_on_self) {
    src = mputprintf(src, ") const\n"
      "{\n"
      "TTCN_error(\"Values of type %s cannot be sent to "
      "other test components.\");\n", dispname);
  } else {
    src = mputprintf(src, " text_buf) const\n"
      "{\n"
      "Module_List::encode_%s(text_buf,"
      "(genericfunc_t)referred_function);\n", fat_string);
  }
  src = mputstr(src,"}\n\n");
  def = mputstr(def,"void decode_text(Text_Buf& text_buf);\n");
  src = mputprintf(src,"void %s::decode_text(Text_Buf&", name);
  if (fdef->runs_on_self) {
    src = mputprintf(src, ")\n"
      "{\n"
      "TTCN_error(\"Values of type %s cannot be received "
      "from other test components.\");\n", dispname);
  } else {
    src = mputprintf(src, " text_buf)\n"
      "{\n"
      "Module_List::decode_%s(text_buf,"
      "(genericfunc_t*)&referred_function);\n", fat_string);
  }
  src = mputstr(src,"}\n\n");
  def = mputstr(def, "};\n\n");

  def = mputprintf(def,"extern boolean operator==(%s::function_pointer value,"
      " const %s& other_value);\n", name, name);
  src = mputprintf(src,"boolean operator==(%s::function_pointer value, "
      "const %s& other_value)\n"
    "{\n"
    "other_value.must_bound(\"Unbound right operand of %s comparison.\");\n"
    "return value == other_value.referred_function;\n"
    "}\n\n", name, name, dispname);
  def = mputprintf(def,"inline boolean operator!=(%s::function_pointer value,"
      " const %s& other_value)\n"
    "{ return !(value == other_value); } \n\n", name, name);

  output->header.class_defs = mputstr(output->header.class_defs, def);
  Free(def);

  output->source.methods = mputstr(output->source.methods, src);
  Free(src);
}

void defFunctionrefTemplate(const funcref_def *fdef, output_struct *output)
{
  char *def = NULL, *src = NULL;
  const char *name = fdef->name;
  const char *dispname = fdef->dispname;
  char *fat_string = NULL;

  switch(fdef->type) {
  case FUNCTION:
    fat_string = mputstr(fat_string, "function");
    break;
  case ALTSTEP:
    fat_string = mputstr(fat_string, "altstep");
    break;
  case TESTCASE:
    fat_string = mputstr(fat_string, "testcase");
    break;
  }

  /* class declaration */
    output->header.class_decls = mputprintf(output->header.class_decls,
      "class %s_template;\n", name);

  /* class definition */
  def = mputprintf(def,"class %s_template : public Base_Template {\n"
    "union {\n"
    "%s::function_pointer single_value;\n"
    "struct {\n"
    "unsigned int n_values;\n"
    "%s_template *list_value;\n"
    "} value_list;\n"
    "};\n\n", name, name, name);

  /* copy template */
  def = mputprintf(def,"  void copy_template(const %s_template& other_value);\n"
    , name);
  src = mputprintf(src,"void %s_template::copy_template(const %s_template& "
      "other_value)\n"
    "{\n"
    "switch(other_value.template_selection) {\n"
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
    "for(unsigned int i = 0; i < value_list.n_values; i++)\n"
    "value_list.list_value[i] = other_value.value_list.list_value[i];\n"
    "break;\n"
    "default:\n"
    "TTCN_error(\"Copying an unitialized/unsupported %s template.\");\n"
    "}\n"
    "set_selection(other_value);\n"
    "}\n\n", name, name, name, dispname);

  /* constructors */
  def = mputprintf(def,"public:\n"
    "%s_template();\n", name);
  src = mputprintf(src,"%s_template::%s_template()\n"
    "{\n}\n\n", name, name);
  def = mputprintf(def,"%s_template(template_sel other_value);\n", name);
  src = mputprintf(src,"%s_template::%s_template(template_sel other_value)\n"
    "  : Base_Template(other_value)\n"
    "{\n"
    "check_single_selection(other_value);\n"
    "}\n\n", name, name);
  def = mputprintf(def,"%s_template(%s::function_pointer other_value);\n"
    , name, name);
  src = mputprintf(src,"%s_template::%s_template(%s::function_pointer "
      "other_value)\n"
    "  : Base_Template(SPECIFIC_VALUE)\n"
    "{\n"
    "single_value = other_value;\n"
    "}\n\n", name, name, name);
  def = mputprintf(def,"%s_template(const %s& other_value);\n", name, name);
  src = mputprintf(src,"%s_template::%s_template(const %s& other_value)\n"
    "  :Base_Template(SPECIFIC_VALUE)\n"
    "{\n"
    "other_value.must_bound(\"Creating a template from an unbound %s value."
      "\");\n"
    "single_value = other_value.referred_function;\n"
    "}\n\n", name, name, name, dispname);
  def = mputprintf(def,"%s_template(const OPTIONAL<%s>& other_value);\n"
    , name, name);
  src = mputprintf(src,"%s_template::%s_template(const OPTIONAL<%s>& "
      "other_value)\n"
    "{\n"
    "if(other_value.ispresent()) {\n"
    "set_selection(SPECIFIC_VALUE);\n"
    "single_value = ((const %s&)other_value()).referred_function;\n"
    "} else set_selection(OMIT_VALUE);\n"
    "}\n\n", name, name, name, name);
  def = mputprintf(def,"%s_template(const %s_template& other_value);\n"
    , name, name);
  src = mputprintf(src,"%s_template::%s_template(const %s_template& "
      "other_value)\n"
    "  :Base_Template()\n" /* yes, the default constructor */
    "{\n"
    "copy_template(other_value);\n"
    "}\n\n", name, name, name);

  /* destructor */
  def = mputprintf(def,"~%s_template();\n", name);
  src = mputprintf(src,"%s_template::~%s_template()\n"
    "{\n"
    "  clean_up();\n"
    "}\n\n", name, name);

  /* clean up */
  def = mputstr(def,"void clean_up();\n");
  src = mputprintf(src,"void %s_template::clean_up()"
    "{\n"
    "if(template_selection == VALUE_LIST ||\n"
    "template_selection == COMPLEMENTED_LIST)\n"
    "delete[] value_list.list_value;\n"
    "template_selection = UNINITIALIZED_TEMPLATE;\n"
    "}\n\n", name);

  /* operator = */
  def = mputprintf(def,"%s_template& operator=(template_sel other_value);\n"
    , name);
  src = mputprintf(src,"%s_template& %s_template::operator=(template_sel "
      "other_value)\n"
    "{\n"
    "check_single_selection(other_value);\n"
    "clean_up();\n"
    "set_selection(other_value);\n"
    "return *this;\n"
    "}\n\n", name, name);
  def = mputprintf(def,"%s_template& operator=(%s::function_pointer "
      "other_value);\n", name, name);
  src = mputprintf(src,"%s_template& %s_template::operator="
      "(%s::function_pointer other_value)\n"
    "{\n"
    "clean_up();\n"
    "set_selection(SPECIFIC_VALUE);\n"
    "single_value = other_value;\n"
    "return *this;"
    "}\n\n", name, name, name);
  def = mputprintf(def,"%s_template& operator=(const %s& other_value);\n"
    , name, name);
  src = mputprintf(src,"%s_template& %s_template::operator="
      "(const %s& other_value)\n"
    "{\n"
    "other_value.must_bound(\"Assignment of an unbound %s value to a "
      "template.\");\n"
    "clean_up();\n"
    "set_selection(SPECIFIC_VALUE);\n"
    "single_value = other_value.referred_function;\n"
    "return *this;\n"
    "}\n\n", name, name, name, dispname);
  def = mputprintf(def,"%s_template& operator=(const OPTIONAL<%s>& "
      "other_value);\n", name, name);
  src = mputprintf(src,"%s_template& %s_template::operator=(const "
      "OPTIONAL<%s>& other_value)\n"
    "{\n"
    "clean_up();\n"
    "if(other_value.ispresent()) { \n"
    "set_selection(SPECIFIC_VALUE);\n"
    "single_value = ((const %s&)other_value()).referred_function;\n"
    "} else set_selection(OMIT_VALUE);\n"
    "return *this;"
    "}\n\n", name, name, name, name);
  def = mputprintf(def,"%s_template& operator=(const %s_template& "
      "other_value);\n", name, name);
  src = mputprintf(src,"%s_template& %s_template::operator=(const %s_template& "
      "other_value)\n"
    "{\n"
    "if(&other_value != this) {\n"
    "clean_up();"
    "copy_template(other_value);\n"
    "}\n"
    "return *this;\n"
    "}\n\n", name ,name, name);

  /* match functions */
  def = mputprintf(def,"boolean match(%s::function_pointer "
      "other_value, boolean legacy = FALSE) const;\n", name);
  src = mputprintf(src,"boolean %s_template::match(%s::function_pointer "
      "other_value, boolean) const\n"
    "{\n"
    "switch(template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "return single_value == other_value;\n"
    "case OMIT_VALUE:\n"
    "return FALSE;\n"
    "case ANY_VALUE:\n"
    "case ANY_OR_OMIT:\n"
    "return TRUE;\n"
    "case VALUE_LIST:\n"
    "case COMPLEMENTED_LIST:\n"
    "for(unsigned int i = 0; i < value_list.n_values; i++)\n"
    "if(value_list.list_value[i].match(other_value))\n"
    "return template_selection == VALUE_LIST;\n"
    "return template_selection == COMPLEMENTED_LIST;\n"
    "default:\n"
    "TTCN_error(\"Matching with an unitialized/unsupported %s template."
      "\");\n"
    "};\n"
    "return FALSE;\n"
    "}\n\n", name, name, dispname);
  def = mputprintf(def,"boolean match(const %s& other_value, boolean legacy "
    "= FALSE) const;\n", name);
  src = mputprintf(src,"boolean %s_template::match(const %s& other_value, "
    "boolean) const\n"
    "{\n"
    "  if (!other_value.is_bound()) return FALSE;\n"
    "return match(other_value.referred_function);\n"
    "}\n\n", name, name);

  /* value of function */
  def = mputprintf(def,"%s valueof() const;\n", name);
  src = mputprintf(src,"%s %s_template::valueof() const\n"
    "{\n"
    "if(template_selection != SPECIFIC_VALUE || is_ifpresent)\n"
    "TTCN_error(\"Performing a valueof or send operation on a "
      "non-specific %s template.\");\n"
    "return single_value;\n}\n\n", name, name, dispname);

  /* set type */
  def = mputstr(def,"void set_type(template_sel template_type, "
      "unsigned int list_length);\n");
  src = mputprintf(src,"void %s_template::set_type(template_sel template_type, "
      "unsigned int list_length)\n"
    "{\n"
    "if(template_type != VALUE_LIST && "
    "template_type != COMPLEMENTED_LIST)\n"
    "TTCN_error(\"Setting an invalid type for an %s template.\");\n"
    "clean_up();\n"
    "set_selection(template_type);\n"
    "value_list.n_values = list_length;\n"
    "value_list.list_value = new %s_template[list_length];\n"
    "}\n\n", name, dispname, name);

  /* list item */
  def = mputprintf(def,"%s_template& list_item(unsigned int list_index) "
      "const;\n", name);
  src = mputprintf(src,"%s_template& %s_template::list_item("
      "unsigned int list_index) const\n"
    "{\n"
    "if(template_selection != VALUE_LIST && "
    "template_selection != COMPLEMENTED_LIST)\n"
    "TTCN_error(\"Accessing a list element of a non-list template of "
      "type %s\");\n"
    "if(list_index >= value_list.n_values)\n"
    "TTCN_error(\"Index overflow in a value list template of type %s."
      "\");\n"
    "return value_list.list_value[list_index];\n"
    "}\n\n", name, name, dispname, dispname);

  if (use_runtime_2) {
    /* functions in alternative runtime */
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
  def = mputstr(def,"void log() const;\n");
  src = mputprintf(src,"void %s_template::log() const\n"
    "{\n"
    "switch(template_selection) {\n"
    "case SPECIFIC_VALUE:\n"
    "Module_List::log_%s((genericfunc_t)single_value);\n"
    "break;\n"
    "case COMPLEMENTED_LIST:\n"
    "TTCN_Logger::log_event_str(\"complement \");\n"
    "case VALUE_LIST:\n"
    "TTCN_Logger::log_char('(');\n"
    "for(unsigned int i = 0; i < value_list.n_values; i++) {\n"
    "if(i > 0) TTCN_Logger::log_event_str(\", \");\n"
    "value_list.list_value[i].log();\n"
    "}\n"
    "TTCN_Logger::log_char(')');\n"
    "break;\n"
    "default:\n"
    "log_generic();\n"
    "}\n"
    "log_ifpresent();\n"
    "}\n\n", name, fat_string);

  /* log_match function */
  def = mputprintf(def,"void log_match(const %s& match_value, "
    "boolean legacy = FALSE) const;\n", name);
  src = mputprintf(src,"void %s_template::log_match(const %s& match_value, "
    "boolean legacy) const\n"
    "{\n"
    "log();\n"
    "TTCN_Logger::log_event_str(\" with \");\n"
    "match_value.log();\n"
    "if(match(match_value, legacy)) TTCN_Logger::log_event_str(\" matched\");\n"
    "else TTCN_Logger::log_event_str(\" unmatched\");\n"
    "}\n\n", name, name);

  /* encode_text / decode_text */
  def = mputstr(def,"void encode_text(Text_Buf& text_buf) const;\n");
  src = mputprintf(src,"void %s_template::encode_text(Text_Buf&", name);
  if (fdef->runs_on_self) {
    src = mputprintf(src, ") const\n"
      "{\n"
      "TTCN_error(\"Templates of type %s cannot be sent to "
      "other test components.\");\n", dispname);
  } else {
    src = mputprintf(src, " text_buf) const\n"
      "{\n"
      "encode_text_base(text_buf);\n"
      "switch(template_selection) {\n"
      "case OMIT_VALUE:\n"
      "case ANY_VALUE:\n"
      "case ANY_OR_OMIT:\n"
      "break;\n"
      "case SPECIFIC_VALUE:\n"
      "Module_List::encode_%s(text_buf, (genericfunc_t)single_value);\n"
      "break;\n"
      "case VALUE_LIST:\n"
      "case COMPLEMENTED_LIST:\n"
      "text_buf.push_int(value_list.n_values);\n"
      "for(unsigned int i = 0; i < value_list.n_values; i++)\n"
      "value_list.list_value[i].encode_text(text_buf);\n"
      "break;\n"
      "default:\n"
      "TTCN_error(\"Text encoder: Encoding an uninitialized/unsupported template "
        "of type %s.\");\n"
      "}\n", fat_string, dispname);
  }
  src = mputstr(src,"}\n\n");
  def = mputstr(def,"void decode_text(Text_Buf& text_buf);\n");
  src = mputprintf(src,"void %s_template::decode_text(Text_Buf&", name);
  if (fdef->runs_on_self) {
    src = mputprintf(src, ")\n"
      "{\n"
      "TTCN_error(\"Templates of type %s cannot be received "
      "from other test components.\");\n", dispname);
  } else {
    src = mputprintf(src, " text_buf)\n"
      "{\n"
      "clean_up();\n"
      "decode_text_base(text_buf);\n"
      "switch(template_selection) {\n"
      "case OMIT_VALUE:\n"
      "case ANY_VALUE:\n"
      "case ANY_OR_OMIT:\n"
      "break;\n"
      "case SPECIFIC_VALUE:\n"
      "Module_List::decode_%s(text_buf,(genericfunc_t*)&single_value);\n"
      "break;\n"
      "case VALUE_LIST:\n"
      "case COMPLEMENTED_LIST:\n"
      "value_list.n_values = text_buf.pull_int().get_val();\n"
      "value_list.list_value = new %s_template[value_list.n_values];\n"
      "for(unsigned int i = 0; i < value_list.n_values; i++)\n"
      "value_list.list_value[i].decode_text(text_buf);\n"
      "default:\n"
      "TTCN_error(\"Text decoder: An unknown/unsupported selection was received "
        "in a template of type %s.\");\n"
      "}\n", fat_string, name, dispname);
  }
  src = mputstr(src,"}\n\n");

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

  /* set_param */
  def = mputstr(def,"void set_param(Module_Param& param);\n");
  src = mputprintf(src,"void %s_template::set_param(Module_Param& param)\n"
    "{\n"
    "  param.error(\"Not supported.\");\n"
    "}\n\n", name);
  
  /* get_param, RT2 only */
  if (use_runtime_2) {
    def = mputstr(def,"Module_Param* get_param(Module_Param_Name& param_name) const;\n");
    src = mputprintf(src,"Module_Param* %s_template::get_param(Module_Param_Name& /* param_name */) const\n"
      "{\n"
      "  return NULL;\n"
      "}\n\n", name);
  }

  if (!use_runtime_2) {
    /* check template restriction */
    def = mputstr(def, "void check_restriction(template_res t_res, "
      "const char* t_name=NULL, boolean legacy = FALSE) const;\n");
    src = mputprintf(src,
      "void %s_template::check_restriction"
        "(template_res t_res, const char* t_name, boolean legacy) const\n"
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

  def = mputstr(def,"};\n\n");

  Free(fat_string);

  output->header.class_defs = mputstr(output->header.class_defs, def);
  Free(def);

  output->source.methods = mputstr(output->source.methods, src);
  Free(src);
}
