/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Baranyi, Botond – initial implementation
 *
 ******************************************************************************/

#include "DebuggerStuff.hh"
#include "AST.hh"
#include "Type.hh"
#include "ttcn3/AST_ttcn3.hh"
#include "ttcn3/ArrayDimensions.hh"

namespace Common {
 
/** Returns a string, that contains the template parameters of the debugger's
  * port array or timer array printing function.
  * 
  * Recursive (handles one array dimension per call).
  * 
  * @param p_dims the array's dimensions
  * @param p_element_type name of the array's element C++ class
  * @param p_array_type name of the array's C++ class ("PORT_ARRAY" or "TIMER_ARRAY")
  * @param index the index of the currently handled dimension in \a p_dims
  */
string function_params_for_array_dims(Ttcn::ArrayDimensions* p_dims,
                                      string p_element_type,
                                      string p_array_type,
                                      size_t index = 0)
{
  if (index == p_dims->get_nof_dims()) {
    return p_element_type;
  }
  string ret_val;
  if (index != 0) {
    ret_val = p_array_type + string("<");
  }
  ret_val += function_params_for_array_dims(p_dims, p_element_type, p_array_type, index + 1);
  Ttcn::ArrayDimension* dim = p_dims->get_dim_byIndex(index);
  ret_val += string(", ") + Int2string(dim->get_size()) +
    string(", ") + Int2string(dim->get_offset());
  if (index != 0) {
    ret_val += string(">");
  }
  return ret_val;
}

/** Returns a string, that contains the template parameters of the debugger's
  * value array or template array printing function.
  * 
  * Recursive (handles one array dimension per call).
  * 
  * @param p_type the current type (either an array or the array element)
  * @param p_scope the current scope
  * @param p_templ indicates whether it's a template array or value array
  * @param p_first indicates whether this is the first call or a recursive call
  */
string function_params_for_array_type(Type* p_type,
                                      Scope* p_scope,
                                      bool p_templ,
                                      bool p_first = true)
{
  string ret_val;
  if (p_type->get_typetype() != Type::T_ARRAY) {
    ret_val = p_type->get_genname_value(p_scope);
    if (p_templ) {
      ret_val += "_template";
    }
  }
  else {
    if (!p_first) {
      if (p_templ) {
        ret_val = "TEMPLATE_ARRAY<";
      }
      else {
        ret_val = "VALUE_ARRAY<";
      }
    }
    Type* elem_type = p_type->get_ofType()->get_type_refd_last();
    if (p_templ) {
      ret_val += function_params_for_array_type(elem_type, p_scope, false, false) +
        ", " + function_params_for_array_type(elem_type, p_scope, true, false);
    }
    else {
      ret_val += function_params_for_array_type(elem_type, p_scope, false, false);
    }
    Ttcn::ArrayDimension* dim = p_type->get_dimension();
    ret_val += string(", ") + Int2string(dim->get_size()) +
      string(", ") + Int2string(dim->get_offset());
    if (!p_first) {
      ret_val += string(">");
    }
  }
  return ret_val;
}

/** Appends the string representations of the specified array dimensions. */
string array_dimensions_to_string(Ttcn::ArrayDimensions* p_dims)
{
  string ret_val;
  for (size_t i = 0; i < p_dims->get_nof_dims(); ++i) {
    ret_val += p_dims->get_dim_byIndex(i)->get_stringRepr();
  }
  return ret_val;
}

void calculate_type_name_and_debug_functions_from_type(Type* p_type,
                                                       Type* p_type_last,
                                                       Module* p_module,
                                                       string& p_type_name,
                                                       string& p_print_function,
                                                       string& p_set_function)
{
  if (p_type_last->get_typetype() == Type::T_COMPONENT) {
    p_type_name = "component";
  }
  else if (p_type_last->is_structured_type() ||
           p_type_last->get_typetype() == Type::T_ENUM_A ||
           p_type_last->get_typetype() == Type::T_ENUM_T ||
           p_type_last->get_typetype() == Type::T_SIGNATURE ||
           p_type_last->get_typetype() == Type::T_FUNCTION ||
           p_type_last->get_typetype() == Type::T_ALTSTEP ||
           p_type_last->get_typetype() == Type::T_TESTCASE) {
    // user-defined type
    if (p_type_last->is_pard_type_instance()) {
      // if the referenced type is an instance of an ASN.1 parameterized type,
      // then use the last non-parameterized type in the reference chain to
      // calculate the type name
      Type* t = p_type;
      while (t->is_ref() && !t->is_pard_type_instance()) {
        p_type_name = t->get_dispname();
        t = t->get_type_refd();
      }
    }
    else {
      p_type_name = p_type_last->get_dispname();
    }
    const Module* var_type_mod = p_type_last->get_my_scope()->get_scope_mod();
    string module_prefix;
    if (var_type_mod != p_module) {
      module_prefix = var_type_mod->get_modid().get_name() + "::";
    }
    p_print_function = module_prefix + "print_var_" +
      var_type_mod->get_modid().get_ttcnname();
    if (p_type_last->get_typetype() != Type::T_SIGNATURE &&
        p_type_last->get_typetype() != Type::T_PORT) {
      p_set_function = module_prefix + "set_var_" +
        var_type_mod->get_modid().get_ttcnname();
    }
  }
  else {
    // built-in type, get the TTCN-3 version of the type if possible
    switch (p_type_last->get_typetype()) {
    case Type::T_GENERALSTRING:
    case Type::T_GRAPHICSTRING:
    case Type::T_TELETEXSTRING:
    case Type::T_VIDEOTEXSTRING:
      // these ASN.1 string types are not converted right by Type::get_typetype_ttcn3()
      p_type_name = "universal charstring";
      break;
    case Type::T_PORT:
      p_type_name = "port";
      break;
    case Type::T_UNRESTRICTEDSTRING:
    case Type::T_EMBEDDED_PDV:
    case Type::T_EXTERNAL:
      // these are converted to T_SEQ_T by Type::get_typetype_ttcn3()
      p_type_name = Type::get_typename_builtin(p_type_last->get_typetype());
      break;
    default:
      p_type_name = Type::get_typename_builtin(p_type_last->get_typetype_ttcn3());
      break;
    }
  }
}

char* generate_code_debugger_add_var(char* str, Common::Assignment* var_ass,
                                     Module* current_mod /* = NULL */,
                                     const char* scope_name /* = NULL */)
{
  if (current_mod == NULL) {
    current_mod = var_ass->get_my_scope()->get_scope_mod();
  }
  
  param_eval_t eval = NORMAL_EVAL;
  bool is_constant = false;
  switch (var_ass->get_asstype()) {
  case Common::Assignment::A_PAR_VAL:
  case Common::Assignment::A_PAR_VAL_IN:
  case Common::Assignment::A_PAR_TEMPL_IN: {
    // lazy and fuzzy parameters have their own printing functions
    eval = var_ass->get_eval_type();
    Ttcn::FormalPar* fpar = dynamic_cast<Ttcn::FormalPar*>(var_ass);
    is_constant = fpar == NULL || !fpar->get_used_as_lvalue();
    break; }
  case Common::Assignment::A_CONST:
  case Common::Assignment::A_EXT_CONST:
  case Common::Assignment::A_MODULEPAR:
  case Common::Assignment::A_MODULEPAR_TEMP:
  case Common::Assignment::A_TEMPLATE:
    is_constant = true; //scope_name != NULL;
  default:
    break;
  }
  
  // recreate the TTCN-3 version of the type name and determine the type's 
  // printing and overwriting functions
  string type_name, print_function, set_function;
  switch (eval) {
  case NORMAL_EVAL:
    print_function = "TTCN3_Debugger::print_base_var";
    break;
  case LAZY_EVAL:
    print_function = "TTCN3_Debugger::print_lazy_param<";
    break;
  case FUZZY_EVAL:
    print_function = "TTCN3_Debugger::print_fuzzy_param";
    break;
  default:
    FATAL_ERROR("generate_code_debugger_add_var");
  }
  set_function = "TTCN3_Debugger::set_base_var";
  if (var_ass->get_asstype() == Common::Assignment::A_TIMER ||
      var_ass->get_asstype() == Common::Assignment::A_PAR_TIMER) {
    type_name = "timer";
    if (var_ass->get_Dimensions() != NULL) {
      // timer array
      type_name += array_dimensions_to_string(var_ass->get_Dimensions());
      print_function = string("TTCN3_Debugger::print_timer_array<") +
        function_params_for_array_dims(var_ass->get_Dimensions(),
                                       string("TIMER"), string("TIMER_ARRAY")) +
        string(">");
    }
  }
  else {
    Common::Type* var_type = var_ass->get_Type();
    // get the type at the end of the reference chain, but don't go through
    // CHARACTER STRINGs, EMBEDDED PDVs and EXTERNALs
    while (var_type->is_ref() && var_type->get_typetype() != Type::T_EXTERNAL &&
           var_type->get_typetype() != Type::T_EMBEDDED_PDV &&
           var_type->get_typetype() != Type::T_UNRESTRICTEDSTRING) {
      var_type = var_type->get_type_refd();
    }
    if (eval == LAZY_EVAL) {
      print_function += var_type->get_genname_value(current_mod);
    }
    if (var_type->get_typetype() == Type::T_PORT && var_ass->get_Dimensions() != NULL) {
      // port array
      type_name = var_type->get_dispname() +
        array_dimensions_to_string(var_ass->get_Dimensions());
      if (eval == NORMAL_EVAL) {
        print_function = string("TTCN3_Debugger::print_port_array<") +
          function_params_for_array_dims(var_ass->get_Dimensions(),
                                    var_type->get_genname_value(current_mod),
                                    string("PORT_ARRAY")) +
          string(">");
      }
    }
    else if (var_type->get_typetype() == Type::T_ARRAY) {
      string dims_str;
      Type* t = var_type;
      while (t->get_typetype() == Type::T_ARRAY) {
        dims_str += t->get_dimension()->get_stringRepr();
        t = t->get_ofType()->get_type_refd_last();
      }
      string dummy1, dummy2;
      calculate_type_name_and_debug_functions_from_type(t, t, current_mod,
        type_name, dummy1, dummy2);
      type_name += dims_str;
      if (eval == NORMAL_EVAL) {
        switch (var_ass->get_asstype()) {
        case Common::Assignment::A_MODULEPAR_TEMP:
        case Common::Assignment::A_TEMPLATE:
        case Common::Assignment::A_VAR_TEMPLATE:
        case Common::Assignment::A_PAR_TEMPL_IN:
        case Common::Assignment::A_PAR_TEMPL_OUT:
        case Common::Assignment::A_PAR_TEMPL_INOUT:
          // template array
          print_function = string("TTCN3_Debugger::print_template_array<") +
            function_params_for_array_type(var_type, current_mod, true) +
            string(">");
          set_function = string("TTCN3_Debugger::set_template_array<") +
            function_params_for_array_type(var_type, current_mod, true) +
            string(">");
          break;
        default:
          // value array
          print_function = string("TTCN3_Debugger::print_value_array<") +
            function_params_for_array_type(var_type, current_mod, false) +
            string(">");
          set_function = string("TTCN3_Debugger::set_value_array<") +
            function_params_for_array_type(var_type, current_mod, false) +
            string(">");
          break;
        }
      }
    }
    else {
      string dummy;
      calculate_type_name_and_debug_functions_from_type(var_ass->get_Type(),
        var_type, current_mod, type_name, eval != NORMAL_EVAL ? dummy : print_function,
        set_function);
    }
  }
  
  switch (var_ass->get_asstype()) {
  case Common::Assignment::A_MODULEPAR_TEMP:
  case Common::Assignment::A_TEMPLATE:
  case Common::Assignment::A_VAR_TEMPLATE:
  case Common::Assignment::A_PAR_TEMPL_IN:
  case Common::Assignment::A_PAR_TEMPL_OUT:
  case Common::Assignment::A_PAR_TEMPL_INOUT:
    // add a suffix, if it's a template
    type_name += " template";
    if (eval == LAZY_EVAL) {
      print_function += "_template";
    }
    break;
  default:
    break;
  }
  
  if (eval == LAZY_EVAL) {
    print_function += ">";
  }
  
  string module_str;
  if (scope_name != NULL && !strcmp(scope_name, "global")) {
    // only store the module name for global variables
    module_str = string("\"") +
      var_ass->get_my_scope()->get_scope_mod()->get_modid().get_ttcnname() + string("\"");
  }
  else {
    module_str = "NULL";
  }
  
  return mputprintf(str, "%s%s_scope%sadd_variable(&%s, \"%s\", \"%s\", %s, %s%s%s);\n",
    scope_name != NULL ? "  " : "", // add indenting for global variables
    scope_name != NULL ? scope_name : "debug", // the prefix of the debugger scope:
    // ("global" for global variables, "debug" for local variables,
    // or the component name for component variables)
    scope_name != NULL ? "->" : ".", // global scopes are pointers, local scopes
    // are local variables
    var_ass->get_genname_from_scope(current_mod, "").c_str(), // variable name in C++
    // (HACK: an empty string is passed as the prefix parameter to get_genname_from_scope,
    // so the lazy parameter evaluation code is not generated)
    var_ass->get_id().get_ttcnname().c_str(), // variable name in TTCN-3
    type_name.c_str(), // variable type in TTCN-3, with a suffix if it's a template
    module_str.c_str(), // module name, where the variable was defined
    print_function.c_str(), // variable printing function
    is_constant ? "" : ", ", is_constant ? "" : set_function.c_str());
    // variable overwriting function (not generated for constants)
}

char* generate_code_debugger_function_init(char* str, Common::Assignment* func_ass)
{
  string comp_str = func_ass->get_RunsOnType() == NULL ? string("NULL") :
    string("\"") + func_ass->get_RunsOnType()->get_dispname() + string("\"");
  const char* func_type_str = NULL;
  switch (func_ass->get_asstype()) {
  case Common::Assignment::A_FUNCTION:
  case Common::Assignment::A_FUNCTION_RVAL:
  case Common::Assignment::A_FUNCTION_RTEMP:
    func_type_str = "function";
    break;
  case Common::Assignment::A_EXT_FUNCTION:
  case Common::Assignment::A_EXT_FUNCTION_RVAL:
  case Common::Assignment::A_EXT_FUNCTION_RTEMP:
    func_type_str = "external function";
    break;
  case Common::Assignment::A_TESTCASE:
    func_type_str = "testcase";
    break;
  case Common::Assignment::A_ALTSTEP:
    func_type_str = "altstep";
    break;
  case Common::Assignment::A_TEMPLATE: // parameterized template
    func_type_str = "template";
    break;
  default:
    break;
  }
  Ttcn::FormalParList* fp_list = func_ass != NULL ? func_ass->get_FormalParList() : NULL;
  if (fp_list != NULL && fp_list->get_nof_fps() != 0) {
    // has parameters
    char* fp_names_str = NULL;
    char* fp_types_str = NULL;
    char* fp_add_var_str = NULL;
    for (size_t i = 0; i < fp_list->get_nof_fps(); ++i) {
      // gather everything needed for this parameter in sub-strings
      Ttcn::FormalPar* fp = fp_list->get_fp_byIndex(i);
      const char* fp_type_str = NULL;
      switch (fp->get_asstype()) {
      case Common::Assignment::A_PAR_VAL:
      case Common::Assignment::A_PAR_VAL_IN:
      case Common::Assignment::A_PAR_TEMPL_IN:
        fp_type_str = "in";
        break;
      case Common::Assignment::A_PAR_VAL_INOUT:
      case Common::Assignment::A_PAR_TEMPL_INOUT:
      case Common::Assignment::A_PAR_TIMER: // treat timers and ports as 'inout' parameters
      case Common::Assignment::A_PAR_PORT:
        fp_type_str = "inout";
        break;
      case Common::Assignment::A_PAR_VAL_OUT:
      case Common::Assignment::A_PAR_TEMPL_OUT:
        fp_type_str = "out";
        break;
      default:
        break;
      }
      fp_names_str = mputprintf(fp_names_str,
        "param_names[%d] = \"%s\";\n", static_cast<int>(i), fp->get_id().get_ttcnname().c_str());
      fp_types_str = mputprintf(fp_types_str,
        "param_types[%d] = \"%s\";\n", static_cast<int>(i), fp_type_str);
      fp_add_var_str = generate_code_debugger_add_var(fp_add_var_str, fp);
    }
    str = mputprintf(str,
      "charstring_list param_names;\n"
      "%s"
      "charstring_list param_types;\n"
      "%s"
      "TTCN3_Debug_Function debug_scope(\"%s\", \"%s\", \"%s\", param_names, param_types, %s);\n"
      "%s"
      "debug_scope.initial_snapshot();\n"
      , fp_names_str, fp_types_str
      , func_ass->get_id().get_dispname().c_str(), func_type_str
      , func_ass->get_my_scope()->get_scope_mod()->get_modid().get_ttcnname().c_str()
      , comp_str.c_str(), fp_add_var_str);
    Free(fp_names_str);
    Free(fp_types_str);
    Free(fp_add_var_str);
  }
  else {
    // no parameters
    str = mputprintf(str,
      "charstring_list no_params = NULL_VALUE;\n"
      "TTCN3_Debug_Function debug_scope(\"%s\", \"%s\", \"%s\", no_params, no_params, %s);\n"
      "debug_scope.initial_snapshot();\n"
      , func_ass->get_id().get_dispname().c_str(), func_type_str
      , func_ass->get_my_scope()->get_scope_mod()->get_modid().get_ttcnname().c_str()
      , comp_str.c_str());
  }
  return str;
}

}
