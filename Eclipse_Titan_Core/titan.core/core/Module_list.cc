/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Feher, Csaba
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include <set>
#include <sstream>
#include <string>

#include "Module_list.hh"
#include "Logger.hh"
#include "Error.hh"
#include "Textbuf.hh"
#include "Types.h"
#include "Param_Types.hh"
#include "Basetype.hh"
#include "Encdec.hh"

#include "../common/ModuleVersion.hh"
#ifdef USAGE_STATS
#include "../common/usage_stats.hh"
#endif

#include "../common/dbgnew.hh"
#include "Debugger.hh"

#include <stdio.h>
#include <string.h>
#include <limits.h>

TTCN_Module *Module_List::list_head = NULL, *Module_List::list_tail = NULL;

void fat_null() {}

void Module_List::add_module(TTCN_Module *module_ptr)
{
  if (module_ptr->list_next == NULL && module_ptr != list_tail) {
    TTCN_Module *list_iter = list_head;
    while (list_iter != NULL) {
      if (strcmp(list_iter->module_name, module_ptr->module_name) > 0)
        break;
      list_iter = list_iter->list_next;
    }
    if (list_iter != NULL) {
      // inserting before list_iter
      module_ptr->list_prev = list_iter->list_prev;
      if (list_iter->list_prev != NULL)
        list_iter->list_prev->list_next = module_ptr;
      list_iter->list_prev = module_ptr;
    } else {
      // inserting at the end of list
      module_ptr->list_prev = list_tail;
      if (list_tail != NULL) list_tail->list_next = module_ptr;
      list_tail = module_ptr;
    }
    module_ptr->list_next = list_iter;
    if (list_iter == list_head) list_head = module_ptr;
  }
}

void Module_List::remove_module(TTCN_Module *module_ptr)
{
  if (module_ptr->list_prev == NULL) list_head = module_ptr->list_next;
  else module_ptr->list_prev->list_next = module_ptr->list_next;
  if (module_ptr->list_next == NULL) list_tail = module_ptr->list_prev;
  else module_ptr->list_next->list_prev = module_ptr->list_prev;

  module_ptr->list_prev = NULL;
  module_ptr->list_next = NULL;
}

TTCN_Module *Module_List::lookup_module(const char *module_name)
{
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next)
    if (!strcmp(list_iter->module_name, module_name)) return list_iter;
  return NULL;
}

TTCN_Module *Module_List::single_control_part()
{
  TTCN_Module *retval = 0;
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next)
    if (list_iter->control_func != 0) {
      if (retval != 0) return 0; // more than one control part => fail
      else retval = list_iter;
    }
  return retval;
}


void Module_List::pre_init_modules()
{
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) list_iter->pre_init_module();
}

void Module_List::post_init_modules()
{
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) list_iter->post_init_called = FALSE;
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) list_iter->post_init_module();
}

void Module_List::start_function(const char *module_name,
  const char *function_name, Text_Buf& function_arguments)
{
  TTCN_Module *module_ptr = lookup_module(module_name);
  if (module_ptr == NULL) {
    // the START message must be dropped here
    function_arguments.cut_message();
    TTCN_error("Internal error: Module %s does not exist.", module_name);
  } else if (module_ptr->start_func == NULL) {
    // the START message must be dropped here
    function_arguments.cut_message();
    TTCN_error("Internal error: Module %s does not have startable "
      "functions.", module_name);
  } else if (!module_ptr->start_func(function_name, function_arguments)) {
    // the START message must be dropped here
    function_arguments.cut_message();
    TTCN_error("Internal error: Startable function %s does not exist in "
      "module %s.", function_name, module_name);
  }
}

void Module_List::initialize_component(const char *module_name,
  const char *component_type, boolean init_base_comps)
{
  TTCN_Module *module_ptr = lookup_module(module_name);
  if (module_ptr == NULL)
    TTCN_error("Internal error: Module %s does not exist.", module_name);
  else if (module_ptr->initialize_component_func == NULL)
    TTCN_error("Internal error: Module %s does not have component types.",
      module_name);
  else if (!module_ptr->initialize_component_func(component_type,
    init_base_comps))
    TTCN_error("Internal error: Component type %s does not exist in "
      "module %s.", component_type, module_name);
}

void Module_List::set_param(Module_Param& param)
{
#ifndef TITAN_RUNTIME_2
    if (param.get_id()->get_nof_names() > (size_t)2) {
      param.error("Module parameter cannot be set. Field names and array "
        "indexes are not supported in the Load Test Runtime.");
    }
#endif
  // The first segment in the parameter name can either be the module name,
  // or the module parameter name - both must be checked
  const char* const first_name = param.get_id()->get_current_name();
  const char* second_name = NULL;
  boolean param_found = FALSE;

  // Check if the first name segment is an existing module name 
  TTCN_Module *module_ptr = lookup_module(first_name);
  if (module_ptr != NULL && module_ptr->set_param_func != NULL && param.get_id()->next_name()) {
    param_found = module_ptr->set_param_func(param);
    if (!param_found) {
      second_name = param.get_id()->get_current_name(); // for error messages
    }
  }
  
  // If not found, check if the first name segment was the module parameter name
  // (even if it matched a module name)
  if (!param_found) {
#ifndef TITAN_RUNTIME_2
    if (param.get_id()->get_nof_names() == (size_t)2) {
      const char* note = "(Note: field names and array indexes are not "
        "supported in the Load Test Runtime).";
      if (module_ptr == NULL) {
        param.error("Module parameter cannot be set, because module '%s' "
          "does not exist. %s", first_name, note);
      }
      else if (module_ptr->set_param_func == NULL) {
        param.error("Module parameter cannot be set, because module '%s' "
          "does not have parameters. %s", first_name, note);
      }
      else {
        param.error("Module parameter cannot be set, because no parameter with "
          " name '%s' exists in module '%s'. %s", second_name, first_name, note);
      }
    }
#endif
    
    param.get_id()->reset(); // set the position back to the first segment
    for (TTCN_Module *list_iter = list_head; list_iter != NULL;
      list_iter = list_iter->list_next) {
      if (list_iter->set_param_func != NULL &&
          list_iter->set_param_func(param)) {
        param_found = TRUE;
      }
    }
  }
  
  // Still not found -> error
  if (!param_found) {
    if (module_ptr == NULL) {
      param.error("Module parameter cannot be set, because module `%s' does not exist, "
        "and no parameter with name `%s' exists in any module.", 
        first_name, first_name);
    } else if (module_ptr->set_param_func == NULL) {
      param.error("Module parameter cannot be set, because module `%s' does not have "
        "parameters, and no parameter with name `%s' exists in other modules.", 
        first_name, first_name);
    } else {
      param.error("Module parameter cannot be set, because no parameter with name `%s' "
        "exists in module `%s', and no parameter with name `%s' exists in any module.",
        second_name, first_name, first_name);
    }
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* Module_List::get_param(Module_Param_Name& param_name,
                                     const Module_Param* caller)
{
  // The first segment in the parameter name can either be the module name,
  // or the module parameter name - both must be checked
  const char* const first_name = param_name.get_current_name();
  const char* second_name = NULL;
  Module_Param* param = NULL;

  // Check if the first name segment is an existing module name 
  TTCN_Module *module_ptr = lookup_module(first_name);
  if (module_ptr != NULL && module_ptr->get_param_func != NULL && param_name.next_name()) {
    param = module_ptr->get_param_func(param_name);
    if (param == NULL) {
      second_name = param_name.get_current_name(); // for error messages
    }
  }
  
  // If not found, check if the first name segment was the module parameter name
  // (even if it matched a module name)
  if (param == NULL) {
    param_name.reset(); // set the position back to the first segment
    for (TTCN_Module *list_iter = list_head; list_iter != NULL;
      list_iter = list_iter->list_next) {
      if (list_iter->get_param_func != NULL) {
        param = list_iter->get_param_func(param_name);
        if (param != NULL) {
          break;
        }
      }
    }
  }
  
  // Still not found -> error
  if (param == NULL) {
    if (module_ptr == NULL) {
      caller->error("Referenced module parameter cannot be found. Module `%s' does not exist, "
        "and no parameter with name `%s' exists in any module.", 
        first_name, first_name);
    } else if (module_ptr->get_param_func == NULL) {
      caller->error("Referenced module parameter cannot be found. Module `%s' does not have "
        "parameters, and no parameter with name `%s' exists in other modules.", 
        first_name, first_name);
    } else {
      caller->error("Referenced module parameter cannot be found. No parameter with name `%s' "
        "exists in module `%s', and no parameter with name `%s' exists in any module.",
        second_name, first_name, first_name);
    }
  }
  else if (param->get_type() == Module_Param::MP_Unbound) {
    delete param;
    caller->error("Referenced module parameter '%s' is unbound.", param_name.get_str());
  }

  return param;
}
#endif

void Module_List::log_param()
{
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    if (list_iter->log_param_func != NULL) {
      TTCN_Logger::begin_event(TTCN_Logger::EXECUTOR_CONFIGDATA);
      TTCN_Logger::log_event("Module %s has the following parameters: "
        "{ ", list_iter->module_name);
      list_iter->log_param_func();
      TTCN_Logger::log_event_str(" }");
      TTCN_Logger::end_event();
    }
  }
}

void Module_List::execute_control(const char *module_name)
{
  TTCN_Module *module_ptr = lookup_module(module_name);
  if (module_ptr != NULL) {
    if (module_ptr->control_func != NULL) {
      try {
        module_ptr->control_func();
      } catch (const TC_Error& tc_error) {
        TTCN_Logger::log(TTCN_Logger::ERROR_UNQUALIFIED,
          "Unrecoverable error in control part of module %s. Execution aborted.",
          module_name);
      } catch (const TC_End& tc_end) {
        TTCN_Logger::log(TTCN_Logger::FUNCTION_UNQUALIFIED,
          "Control part of module %s was stopped.", module_name);
      }
    } else TTCN_error("Module %s does not have control part.", module_name);
  } else TTCN_error("Module %s does not exist.", module_name);
}

void Module_List::execute_testcase(const char *module_name,
  const char *testcase_name)
{
  TTCN_Module *module_ptr = lookup_module(module_name);
  if (module_ptr != NULL) module_ptr->execute_testcase(testcase_name);
  else TTCN_error("Module %s does not exist.", module_name);
}

void Module_List::execute_all_testcases(const char *module_name)
{
  TTCN_Module *module_ptr = lookup_module(module_name);
  if (module_ptr != NULL) module_ptr->execute_all_testcases();
  else TTCN_error("Module %s does not exist.", module_name);
}

void Module_List::print_version()
{
  fputs(
    "Module name       Language  Compilation time   MD5 checksum        "
    "             Version\n"
    "-------------------------------------------------------------------"
    "--------------------\n", stderr);
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) list_iter->print_version();
  fputs("-------------------------------------------------------------------"
    "--------------------\n", stderr);
}

#ifdef USAGE_STATS
void Module_List::send_usage_stats(pthread_t& thread, thread_data*& data)
{
  std::set<ModuleVersion> versions;
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
      list_iter = list_iter->list_next)  {
    ModuleVersion* version = list_iter->get_version();
    if (version->hasProductNumber()) {
      versions.insert(*version);
    }
    delete version;
  }

  std::stringstream stream;
  stream << "runtime";
  for (std::set<ModuleVersion>::iterator it = versions.begin(); it != versions.end(); ++it) {
     if (it == versions.begin()) {
        stream << "&products="  << it->toString();
      } else {
        stream << "," << it->toString();
      }
  }

  data = new thread_data;
  data->sndr = new HttpSender;
  thread = UsageData::getInstance().sendDataThreaded(stream.str().c_str(), data);
}

void Module_List::clean_up_usage_stats(pthread_t thread, thread_data* data)
{
  if (thread != 0) {
    // cancel the usage stats thread if it hasn't finished yet
    pthread_cancel(thread);
    pthread_join(thread, NULL);
  }
  if (data != NULL) {
    if (data->sndr != NULL) {
      delete data->sndr;
    }
    delete data;
  }
}
#endif // USAGE_STATS   

void Module_List::list_testcases()
{
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) list_iter->list_testcases();
}

void Module_List::push_version(Text_Buf& text_buf)
{
  int n_modules = 0;
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) n_modules++;
  text_buf.push_int(n_modules);
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    text_buf.push_string(list_iter->module_name);
    if (list_iter->md5_checksum != NULL) {
      text_buf.push_int(16);
      text_buf.push_raw(16, list_iter->md5_checksum);
    } else text_buf.push_int((RInt)0);
  }
}

void Module_List::encode_function(Text_Buf& text_buf,
  genericfunc_t function_address)
{
  if (function_address == NULL)
    TTCN_error("Text encoder: Encoding an unbound function reference.");
  else if (function_address == fat_null) text_buf.push_string("");
  else {
    const char *module_name, *function_name;
    if (lookup_function_by_address(function_address, module_name,
      function_name)) {
      text_buf.push_string(module_name);
      text_buf.push_string(function_name);
    } else TTCN_error("Text encoder: Encoding function reference %p, "
      "which does not point to a valid function.",
      (void*)(unsigned long)function_address);
  }
}

void Module_List::decode_function(Text_Buf& text_buf,
  genericfunc_t *function_addr_ptr)
{
  char *module_name = text_buf.pull_string();
  if (module_name[0] != '\0') {
    TTCN_Module* module_ptr = lookup_module(module_name);
    if (module_ptr == NULL) {
      try {
        TTCN_error("Text decoder: Module %s does not exist when trying "
          "to decode a function reference.", module_name);
      } catch (...) {
        // to prevent from memory leaks
        delete [] module_name;
        throw;
      }
    }
    char *function_name = text_buf.pull_string();
    genericfunc_t function_address =
      module_ptr->get_function_address_by_name(function_name);
    if (function_address != NULL) *function_addr_ptr = function_address;
    else {
      try {
        TTCN_error("Text decoder: Reference to non-existent function "
          "%s.%s was received.", module_name, function_name);
      } catch (...) {
        // to prevent from memory leaks
        delete [] module_name;
        delete [] function_name;
        throw;
      }
    }
    delete [] function_name;
  } else *function_addr_ptr = fat_null;
  delete [] module_name;
}

void Module_List::log_function(genericfunc_t function_address)
{
  if (function_address == NULL) TTCN_Logger::log_event_str("<unbound>");
  else if (function_address == fat_null) TTCN_Logger::log_event_str("null");
  else {
    const char *module_name, *function_name;
    if (lookup_function_by_address(function_address, module_name,
      function_name)) TTCN_Logger::log_event("refers(%s.%s)",
        module_name, function_name);
    else TTCN_Logger::log_event("<invalid function reference: %p>",
      (void*)(unsigned long)function_address);
  }
}

void Module_List::encode_altstep(Text_Buf& text_buf,
  genericfunc_t altstep_address)
{
  if (altstep_address == NULL)
    TTCN_error("Text encoder: Encoding an unbound altstep reference.");
  else if (altstep_address == fat_null) text_buf.push_string("");
  else {
    const char *module_name, *altstep_name;
    if (lookup_altstep_by_address(altstep_address, module_name,
      altstep_name)) {
      text_buf.push_string(module_name);
      text_buf.push_string(altstep_name);
    } else TTCN_error("Text encoder: Encoding altstep reference %p, "
      "which does not point to a valid altstep.",
      (void*)(unsigned long)altstep_address);
  }
}

void Module_List::decode_altstep(Text_Buf& text_buf,
  genericfunc_t *altstep_addr_ptr)
{
  char *module_name = text_buf.pull_string();
  if (module_name[0] != '\0') {
    TTCN_Module* module_ptr = lookup_module(module_name);
    if (module_ptr == NULL) {
      try {
        TTCN_error("Text decoder: Module %s does not exist when trying "
          "to decode an altstep reference.", module_name);
      } catch (...) {
        // to prevent from memory leaks
        delete [] module_name;
        throw;
      }
    }
    char *altstep_name = text_buf.pull_string();
    genericfunc_t altstep_address =
      module_ptr->get_altstep_address_by_name(altstep_name);
    if (altstep_address != NULL) *altstep_addr_ptr = altstep_address;
    else {
      try {
        TTCN_error("Text decoder: Reference to non-existent altstep "
          "%s.%s was received.", module_name, altstep_name);
      } catch (...) {
        // to prevent from memory leaks
        delete [] module_name;
        delete [] altstep_name;
        throw;
      }
    }
    delete [] altstep_name;
  } else *altstep_addr_ptr = fat_null;
  delete [] module_name;
}

void Module_List::log_altstep(genericfunc_t altstep_address)
{
  if (altstep_address == NULL) TTCN_Logger::log_event_str("<unbound>");
  else if (altstep_address == fat_null) TTCN_Logger::log_event_str("null");
  else {
    const char *module_name, *altstep_name;
    if (lookup_altstep_by_address(altstep_address, module_name,
      altstep_name)) TTCN_Logger::log_event("refers(%s.%s)",
        module_name, altstep_name);
    else TTCN_Logger::log_event("<invalid altstep reference: %p>",
      (void*)(unsigned long)altstep_address);
  }
}

// called by testcase_name::encode_text in the generated code
void Module_List::encode_testcase(Text_Buf& text_buf,
  genericfunc_t testcase_address)
{
  if (testcase_address == NULL)
    TTCN_error("Text encoder: Encoding an unbound testcase reference.");
  else if (testcase_address == fat_null) text_buf.push_string("");
  else {
    const char *module_name, *testcase_name;
    if (lookup_testcase_by_address(testcase_address, module_name,
      testcase_name)) {
      text_buf.push_string(module_name);
      text_buf.push_string(testcase_name);
    } else TTCN_error("Text encoder: Encoding testcase reference %p, "
      "which does not point to a valid testcase.",
      (void*)(unsigned long)testcase_address);
  }
}

// called by testcase_name::decode_text in the generated code
void Module_List::decode_testcase(Text_Buf& text_buf,
  genericfunc_t *testcase_addr_ptr)
{
  char *module_name = text_buf.pull_string();
  if (module_name[0] != '\0') {
    TTCN_Module* module_ptr = lookup_module(module_name);
    if (module_ptr == NULL) {
      try {
        TTCN_error("Text decoder: Module %s does not exist when trying "
          "to decode a testcase reference.", module_name);
      } catch (...) {
        // to prevent from memory leaks
        delete [] module_name;
        throw;
      }
    }
    char *testcase_name = text_buf.pull_string();
    genericfunc_t testcase_address =
      module_ptr->get_testcase_address_by_name(testcase_name);
    if (testcase_address != NULL) *testcase_addr_ptr = testcase_address;
    else {
      try {
        TTCN_error("Text decoder: Reference to non-existent testcase "
          "%s.%s was received.", module_name, testcase_name);
      } catch (...) {
        // to prevent from memory leaks
        delete [] module_name;
        delete [] testcase_name;
        throw;
      }
    }
    delete [] testcase_name;
  } else *testcase_addr_ptr = fat_null;
  delete [] module_name;
}

void Module_List::log_testcase(genericfunc_t testcase_address)
{
  if (testcase_address == NULL) TTCN_Logger::log_event_str("<unbound>");
  else if (testcase_address == fat_null) TTCN_Logger::log_event_str("null");
  else {
    const char *module_name, *testcase_name;
    if (lookup_testcase_by_address(testcase_address, module_name,
      testcase_name)) TTCN_Logger::log_event("refers(%s.%s)",
        module_name, testcase_name);
    else TTCN_Logger::log_event("<invalid testcase reference: %p>",
      (void*)(unsigned long)testcase_address);
  }
}

genericfunc_t Module_List::get_fat_null()
{
  return fat_null;
}

genericfunc_t Module_List::lookup_start_by_function_address(
  genericfunc_t function_address)
{
  if (function_address == NULL) TTCN_error("Performing a start test "
    "component operation with an unbound function reference.");
  else if (function_address == fat_null) TTCN_error("Start test component "
    "operation cannot be performed with a null function reference.");
  for (TTCN_Module* list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    genericfunc_t function_start =
      list_iter->get_function_start_by_address(function_address);
    if (function_start != NULL) return function_start;
  }
  TTCN_error("Function reference %p in start test component operation does "
    "not point to a valid function.",
    (void*)(unsigned long)function_address);
  // to avoid warnings
  return NULL;
}

genericfunc_t Module_List::lookup_standalone_address_by_altstep_address(
  genericfunc_t altstep_address)
{
  if (altstep_address == NULL) TTCN_error("Performing an invoke operation "
    "on an unbound altstep reference.");
  else if (altstep_address == fat_null) TTCN_error("Invoke operation "
    "cannot be performed on a null altstep reference.");
  for (TTCN_Module* list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    genericfunc_t standalone_address, activate_address;
    if (list_iter->get_altstep_data_by_address(altstep_address,
      standalone_address, activate_address)) {
      if (standalone_address == NULL)
        TTCN_error("Internal error: Altstep reference %p cannot be "
          "instantiated as a stand-alone alt statement.",
          (void*)(unsigned long)altstep_address);
      return standalone_address;
    }
  }
  TTCN_error("Altstep reference %p in invoke operation does not point to a "
    "valid altstep.", (void*)(unsigned long)altstep_address);
  // to avoid warnings
  return NULL;
}

genericfunc_t Module_List::lookup_activate_address_by_altstep_address(
  genericfunc_t altstep_address)
{
  if (altstep_address == NULL) TTCN_error("Performing an activate operation "
    "on an unbound altstep reference.");
  else if (altstep_address == fat_null) TTCN_error("Activate operation "
    "cannot be performed on a null altstep reference.");
  for (TTCN_Module* list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    genericfunc_t standalone_address, activate_address;
    if (list_iter->get_altstep_data_by_address(altstep_address,
      standalone_address, activate_address)) {
      if (activate_address == NULL)
        TTCN_error("Internal error: Altstep reference %p cannot be "
          "activated as a default.",
          (void*)(unsigned long)altstep_address);
      return activate_address;
    }
  }
  TTCN_error("Altstep reference %p in activate operation does not point to "
    "a valid altstep.", (void*)(unsigned long)altstep_address);
  // to avoid warnings
  return NULL;
}

boolean Module_List::lookup_function_by_address(genericfunc_t function_address,
  const char*& module_name, const char*& function_name)
{
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    function_name =
      list_iter->get_function_name_by_address(function_address);
    if (function_name != NULL) {
      module_name = list_iter->module_name;
      return TRUE;
    }
  }
  return FALSE;
}

boolean Module_List::lookup_altstep_by_address(genericfunc_t altstep_address,
  const char*& module_name, const char*& altstep_name)
{
  for (TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    altstep_name = list_iter->get_altstep_name_by_address(altstep_address);
    if (altstep_name != NULL) {
      module_name = list_iter->module_name;
      return TRUE;
    }
  }
  return FALSE;
}

boolean Module_List::lookup_testcase_by_address(genericfunc_t testcase_address,
  const char*& module_name, const char*& testcase_name)
{
  for(TTCN_Module *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    testcase_name =
      list_iter->get_testcase_name_by_address(testcase_address);
    if (testcase_name != NULL) {
      module_name = list_iter->module_name;
      return TRUE;
    }
  }
  return FALSE;
}

// ======================= TTCN_Module =======================

struct TTCN_Module::function_list_item {
  const char *function_name;
  genericfunc_t function_address;
  genericfunc_t start_address;
  function_list_item *next_function;
};

struct TTCN_Module::altstep_list_item {
  const char *altstep_name;
  genericfunc_t altstep_address; //instance
  genericfunc_t activate_address;
  genericfunc_t standalone_address;
  altstep_list_item *next_altstep;
};

struct TTCN_Module::testcase_list_item {
  const char *testcase_name;
  boolean is_pard;
  union {
    testcase_t testcase_function;
    genericfunc_t testcase_address;
  };
  testcase_list_item *next_testcase;
};

/** Constructor for TTCN modules */
TTCN_Module::TTCN_Module(const char *par_module_name,
  const char *par_compilation_date,
  const char *par_compilation_time,
  const unsigned char *par_md5_checksum,
  init_func_t par_pre_init_func,
  const char* par_product_number,
  unsigned int par_suffix,
  unsigned int par_release,
  unsigned int par_patch,
  unsigned int par_build,
  const char* par_extra,
  size_t par_num_namespace,
  const namespace_t *par_namespaces,
  init_func_t par_post_init_func,
  set_param_func_t par_set_param_func,
  get_param_func_t par_get_param_func,
  log_param_func_t par_log_param_func,
  initialize_component_func_t par_initialize_component_func,
  start_func_t par_start_func,
  control_func_t par_control_func)
: list_prev(NULL), list_next(NULL)
, module_type(TTCN3_MODULE)
, module_name(par_module_name)
, compilation_date(par_compilation_date)
, compilation_time(par_compilation_time)
, md5_checksum(par_md5_checksum)
, product_number(par_product_number)
, suffix(par_suffix)
, release(par_release)
, patch(par_patch)
, build(par_build)
, extra(par_extra)
, num_namespaces(par_num_namespace)
, xer_namespaces(par_namespaces)
, pre_init_func(par_pre_init_func)
, post_init_func(par_post_init_func)
, pre_init_called(FALSE)
, post_init_called(FALSE)
, set_param_func(par_set_param_func)
, get_param_func(par_get_param_func)
, log_param_func(par_log_param_func)
, initialize_component_func(par_initialize_component_func)
, start_func(par_start_func)
, control_func(par_control_func)
, function_head(NULL)
, function_tail(NULL)
, altstep_head(NULL)
, altstep_tail(NULL)
, testcase_head(NULL)
, testcase_tail(NULL)
{
  Module_List::add_module(this);
}

/** Constructor for ASN.1 modules */
TTCN_Module::TTCN_Module(const char *par_module_name,
  const char *par_compilation_date,
  const char *par_compilation_time,
  const unsigned char par_md5_checksum[16],
  init_func_t par_init_func)
: list_prev(NULL), list_next(NULL)
, module_type(ASN1_MODULE)
, module_name(par_module_name)
, compilation_date(par_compilation_date)
, compilation_time(par_compilation_time)
, md5_checksum(par_md5_checksum)
, product_number(NULL)
, suffix(0)
, release(UINT_MAX)
, patch(UINT_MAX)
, build(UINT_MAX)
, extra(NULL)
, num_namespaces(0)
, xer_namespaces(NULL) // no EXER, no namespaces for ASN.1
, pre_init_func(par_init_func)
, post_init_func(NULL)
, pre_init_called(FALSE)
, post_init_called(FALSE)
, set_param_func(NULL)
, get_param_func(NULL)
, log_param_func(NULL)
, initialize_component_func(NULL)
, start_func(NULL)
, control_func(NULL)
, function_head(NULL)
, function_tail(NULL)
, altstep_head(NULL)
, altstep_tail(NULL)
, testcase_head(NULL)
, testcase_tail(NULL)
{
  Module_List::add_module(this);
}

/** Constructor for C++ modules (?) */
TTCN_Module::TTCN_Module(const char *par_module_name,
  const char *par_compilation_date,
  const char *par_compilation_time,
  init_func_t par_init_func)
: list_prev(NULL), list_next(NULL)
, module_type(CPLUSPLUS_MODULE)
, module_name(par_module_name ? par_module_name : "<unknown>")
, compilation_date(par_compilation_date ? par_compilation_date : "<unknown>")
, compilation_time(par_compilation_time ? par_compilation_time : "<unknown>")
, md5_checksum(NULL)
, product_number(NULL)
, suffix(0)
, release(UINT_MAX)
, patch(UINT_MAX)
, build(UINT_MAX)
, extra(NULL)
, num_namespaces(0)
, xer_namespaces(NULL)
, pre_init_func(par_init_func)
, post_init_func(NULL)
, pre_init_called(FALSE)
, post_init_called(FALSE)
, set_param_func(NULL)
, log_param_func(NULL)
, initialize_component_func(NULL)
, start_func(NULL)
, control_func(NULL)
, function_head(NULL)
, function_tail(NULL)
, altstep_head(NULL)
, altstep_tail(NULL)
, testcase_head(NULL)
, testcase_tail(NULL)
{
  Module_List::add_module(this);
 }

TTCN_Module::~TTCN_Module()
{
  Module_List::remove_module(this);
  while (function_head != NULL) {
    function_list_item *tmp_ptr=function_head->next_function;
    delete function_head;
    function_head = tmp_ptr;
  }
  while (altstep_head != NULL) {
    altstep_list_item *tmp_ptr=altstep_head->next_altstep;
    delete altstep_head;
    altstep_head = tmp_ptr;
  }
  while (testcase_head != NULL) {
    testcase_list_item *tmp_ptr = testcase_head->next_testcase;
    delete testcase_head;
    testcase_head = tmp_ptr;
  }
}

void TTCN_Module::pre_init_module()
{
  if (pre_init_called) return;
  pre_init_called = TRUE;
  if (pre_init_func == NULL) return;
  try {
    pre_init_func();
  } catch (...) {
    TTCN_Logger::log(TTCN_Logger::ERROR_UNQUALIFIED,
      "An error occurred while initializing the constants of module %s.",
      module_name);
    throw;
  }
}

void TTCN_Module::post_init_module()
{
  if (post_init_called) return;
  post_init_called = TRUE;
  TTCN_Logger::log_module_init(module_name);
  if (post_init_func != NULL) post_init_func();
  TTCN_Logger::log_module_init(module_name, TRUE);
}

void TTCN_Module::add_function(const char *function_name,
  genericfunc_t function_address, genericfunc_t start_address)
{
  function_list_item *new_item = new function_list_item;
  new_item->function_name = function_name;
  new_item->function_address = function_address;
  new_item->start_address = start_address;
  new_item->next_function = NULL;
  if(function_head == NULL) function_head = new_item;
  else function_tail->next_function = new_item;
  function_tail = new_item;
}

void TTCN_Module::add_altstep(const char *altstep_name,
  genericfunc_t altstep_address, genericfunc_t activate_address,
  genericfunc_t standalone_address)
{
  altstep_list_item *new_item = new altstep_list_item;
  new_item->altstep_name = altstep_name;
  new_item->altstep_address = altstep_address;
  new_item->activate_address = activate_address;
  new_item->standalone_address = standalone_address;
  new_item->next_altstep = NULL;
  if(altstep_head == NULL) altstep_head = new_item;
  else altstep_tail->next_altstep = new_item;
  altstep_tail = new_item;
}

void TTCN_Module::add_testcase_nonpard(const char *testcase_name,
  testcase_t testcase_function)
{
  testcase_list_item *new_item = new testcase_list_item;
  new_item->testcase_name = testcase_name;
  new_item->is_pard = FALSE;
  new_item->testcase_function = testcase_function;
  new_item->next_testcase = NULL;
  if (testcase_head == NULL) testcase_head = new_item;
  else testcase_tail->next_testcase = new_item;
  testcase_tail = new_item;
}

void TTCN_Module::add_testcase_pard(const char *testcase_name,
  genericfunc_t testcase_address)
{
  testcase_list_item *new_item = new testcase_list_item;
  new_item->testcase_name = testcase_name;
  new_item->is_pard = TRUE;
  new_item->testcase_address = testcase_address;
  new_item->next_testcase = NULL;
  if(testcase_head == NULL) testcase_head = new_item;
  else testcase_tail->next_testcase = new_item;
  testcase_tail = new_item;
}

void TTCN_Module::execute_testcase(const char *testcase_name)
{
  for (testcase_list_item *list_iter = testcase_head; list_iter != NULL;
    list_iter = list_iter->next_testcase) {
    if (!strcmp(list_iter->testcase_name, testcase_name)) {
      if (list_iter->is_pard) {
        // Testcase has parameters. However, there might be a chance...
        // Move to the next one (if any) and check that it has the same name.
        list_iter = list_iter->next_testcase;
        if (list_iter == NULL
          || strcmp(list_iter->testcase_name, testcase_name)) {
          TTCN_error("Test case %s in module %s "
            "cannot be executed individually (without control part) "
            "because it has parameters.", testcase_name, module_name);
          continue; // not reached
        }
        // else it has the same name, fall through
      }

      list_iter->testcase_function(FALSE, 0.0);
      return;
    }
  }
  TTCN_error("Test case %s does not exist in module %s.", testcase_name,
    module_name);
}

void TTCN_Module::execute_all_testcases()
{
  boolean found = FALSE;
  for (testcase_list_item *list_iter = testcase_head; list_iter != NULL;
    list_iter = list_iter->next_testcase) {
    if (ttcn3_debugger.is_exiting()) {
      break;
    }
    if (!list_iter->is_pard) {
      list_iter->testcase_function(FALSE, 0.0);
      found = TRUE;
    }
  }
  if (!found) {
    if (testcase_head != NULL) TTCN_warning("Module %s does not contain "
      "non-parameterized test cases, which can be executed individually "
      "without control part.", module_name);
    else TTCN_warning("Module %s does not contain test cases.",
      module_name);
  }
}

const char *TTCN_Module::get_function_name_by_address(
  genericfunc_t function_address)
{
  for (function_list_item *list_iter = function_head; list_iter != NULL;
    list_iter = list_iter->next_function)
    if (list_iter->function_address == function_address)
      return list_iter->function_name;
  return NULL;
}

genericfunc_t TTCN_Module::get_function_address_by_name(const char *func_name)
{
  for (function_list_item *list_iter = function_head; list_iter != NULL;
    list_iter = list_iter->next_function)
    if (!strcmp(list_iter->function_name, func_name))
      return list_iter->function_address;
  return NULL;
}

genericfunc_t TTCN_Module::get_function_start_by_address(
  genericfunc_t function_address)
{
  for (function_list_item *list_iter = function_head; list_iter != NULL;
    list_iter = list_iter->next_function) {
    if (list_iter->function_address == function_address) {
      if (list_iter->start_address != NULL)
        return list_iter->start_address;
      else TTCN_error("Function %s.%s cannot be started on a parallel "
        "test component.", module_name, list_iter->function_name);
    }
  }
  return NULL;
}

const char *TTCN_Module::get_altstep_name_by_address(
  genericfunc_t altstep_address)
{
  for(altstep_list_item *list_iter = altstep_head; list_iter != NULL;
    list_iter = list_iter->next_altstep) {
    if (list_iter->altstep_address == altstep_address)
      return list_iter->altstep_name;
  }
  return NULL;
}

genericfunc_t TTCN_Module::get_altstep_address_by_name(const char* altstep_name)
{
  for(altstep_list_item *list_iter = altstep_head; list_iter != NULL;
    list_iter = list_iter->next_altstep) {
    if (!strcmp(list_iter->altstep_name, altstep_name))
      return list_iter->altstep_address;
  }
  return NULL;
}

boolean TTCN_Module::get_altstep_data_by_address(genericfunc_t altstep_address,
  genericfunc_t& standalone_address, genericfunc_t& activate_address)
{
  for(altstep_list_item* list_iter = altstep_head; list_iter != NULL;
    list_iter = list_iter->next_altstep) {
    if (list_iter->altstep_address == altstep_address) {
      standalone_address = list_iter->standalone_address;
      activate_address = list_iter->activate_address;
      return TRUE;
    }
  }
  return FALSE;
}

const char *TTCN_Module::get_testcase_name_by_address(
  genericfunc_t testcase_address)
{
  for(testcase_list_item *list_iter = testcase_head; list_iter != NULL;
    list_iter = list_iter->next_testcase) {
    if (list_iter->is_pard) {
      if (list_iter->testcase_address == testcase_address)
        return list_iter->testcase_name;
    } else {
      if ((genericfunc_t)list_iter->testcase_function == testcase_address)
        return list_iter->testcase_name;
    }
  }
  return NULL;
}

genericfunc_t TTCN_Module::get_testcase_address_by_name(const char *testcase_name)
{
  for (testcase_list_item *list_iter = testcase_head; list_iter != NULL;
    list_iter = list_iter->next_testcase) {
    if (!strcmp(list_iter->testcase_name, testcase_name)) {
      if (!list_iter->is_pard) return list_iter->testcase_address;
      else return      (genericfunc_t)list_iter->testcase_function;
    }
  }
  return NULL;
}

ModuleVersion* TTCN_Module::get_version() const {
  return new ModuleVersion(product_number, suffix, release, patch, build, extra);
}

void TTCN_Module::print_version()
{
  const char *type_str;
  switch (module_type) {
  case TTCN3_MODULE:
    type_str = "TTCN-3";
    break;
  case ASN1_MODULE:
    type_str = "ASN.1";
    break;
  case CPLUSPLUS_MODULE:
    type_str = "C++";
    break;
  default:
    type_str = "???";
    break;
  }
  fprintf(stderr, "%-18s %-6s ", module_name, type_str);
  if (compilation_date != NULL && compilation_time != NULL) {
    fprintf(stderr, "%s %s", compilation_date, compilation_time);
  } else {
    fputs("<unknown>           ", stderr);
  }
  if (md5_checksum != NULL) {
    putc(' ', stderr);
    for (int i = 0; i < 16; i++) fprintf(stderr, "%02x", md5_checksum[i]);
  }
  // else it's likely not a TTCN module, so no version info

  putc(' ', stderr); // separator for the version number
  if (product_number != NULL) {
     fprintf(stderr, "%s", product_number);
     if (suffix > 0) {
       fprintf(stderr, "/%d", suffix);
     }
     putc(' ', stderr);
  }
  // release can be between 0 and 999999 inclusive
  // patch can go from 0 to 26 (English alphabet) - 6 (IOPQRW forbidden)
  // build can be between 0 and 99 inclusive
  if (release <= 999999 && patch <= ('Z'-'A'-6) && build <= 99) {
    char *build_str = buildstr(build);
    if (build_str == 0) TTCN_error("TTCN_Module::print_version()");
    if (extra != NULL ) {
      build_str = mputprintf(build_str, "%s", extra);
    }
    fprintf(stderr, "R%u%c%-4s", release, eri(patch), build_str);
    Free(build_str);
  }
  putc('\n', stderr);
}

void TTCN_Module::list_testcases()
{
  if (control_func != NULL) printf("%s.control\n", module_name);
  for (testcase_list_item *list_iter = testcase_head; list_iter != NULL;
    list_iter = list_iter->next_testcase)
    if(!list_iter->is_pard)
      printf("%s.%s\n", module_name, list_iter->testcase_name);
}

const namespace_t *TTCN_Module::get_ns(size_t p_index) const
{
  if (p_index == (size_t)-1) return NULL;
  if (p_index >= num_namespaces) TTCN_error(
    "Index overflow for namespaces, %lu instead of %lu",
    (unsigned long)p_index, (unsigned long)num_namespaces);

  return xer_namespaces + p_index;
}

const namespace_t *TTCN_Module::get_controlns() const
{
  if (xer_namespaces==NULL) {
    TTCN_error("No namespaces for module %s", module_name);
  }
  if (xer_namespaces[num_namespaces].px == NULL
    ||xer_namespaces[num_namespaces].px[0] == '\0') {
    TTCN_error("No control namespace for module %s", module_name);
  }
  return xer_namespaces+num_namespaces;
}


