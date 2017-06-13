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

#include "Debugger.hh"
#include "DebugCommands.hh"
#include "Communication.hh"
#include "../common/pattern.hh"
#include "Param_Types.hh"
#include "DebuggerUI.hh"
#include <unistd.h>
#include <pwd.h>

//////////////////////////////////////////////////////
////////////////// TTCN3_Debugger ////////////////////
//////////////////////////////////////////////////////

#define BUFFER_INCREASE 100

TTCN3_Debugger ttcn3_debugger;

void TTCN3_Debugger::switch_state(const char* p_state_str)
{
  if (!strcmp(p_state_str, "on")) {
    if (active) {
      print(DRET_NOTIFICATION, "The debugger is already switched on.");
    }
    else {
      active = TRUE;
      print(DRET_SETTING_CHANGE, "Debugger switched on.");
    }
  }
  else if(!strcmp(p_state_str, "off")) {
    if (!active) {
      print(DRET_NOTIFICATION, "The debugger is already switched off.");
    }
    else {
      active = FALSE;
      print(DRET_SETTING_CHANGE, "Debugger switched off.");
    }
  }
  else {
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'on' or 'off'.");
  }
}

static boolean is_numeric(const char* p_str)
{
  size_t len = strlen(p_str);
  for (size_t i = 0; i < len; ++i) {
    if (p_str[i] < '0' || p_str[i] > '9') {
      return FALSE;
    }
  }
  return TRUE;
}

void TTCN3_Debugger::set_breakpoint(const char* p_module, const char* p_location,
                                    const char* batch_file)
{
  int line = 0;
  char* function = NULL;
  if (is_numeric(p_location)) {
    // it's a line breakpoint
    line = strtol(p_location, NULL, 10);
  }
  else {
    // it's a function breakpoint
    function = mcopystr(p_location);
  }
  char* loc_str = function != NULL ? mprintf("function '%s'", function) :
    mprintf("line %d", line);
  size_t pos = find_breakpoint(p_module, line, function);
  if (pos == breakpoints.size()) {
    breakpoint_t bp;
    bp.module = mcopystr(p_module);
    bp.line = line;
    bp.function = function;
    bp.batch_file = batch_file != NULL ? mcopystr(batch_file) : NULL;
    breakpoints.push_back(bp);
    print(DRET_SETTING_CHANGE, "Breakpoint added in module '%s' at %s %s%s%s.",
      p_module, loc_str,
      batch_file != NULL ? "with batch file '" : "with no batch file",
      batch_file != NULL ? batch_file : "", batch_file != NULL ? "'" : "");
  }
  else {
    Free(function);
    if (breakpoints[pos].batch_file != NULL) {
      if (batch_file != NULL) {
        if (!strcmp(batch_file, breakpoints[pos].batch_file)) {
          print(DRET_NOTIFICATION, "Breakpoint already set in module '%s' at "
            "%s with batch file '%s'.",
            p_module, loc_str, batch_file);
        }
        else {
          print(DRET_SETTING_CHANGE, "Batch file was changed from '%s' to '%s' for "
            "breakpoint in module '%s' at %s.", breakpoints[pos].batch_file,
            batch_file, p_module, loc_str);
        }
      }
      else {
        print(DRET_SETTING_CHANGE, "Batch file '%s' removed from breakpoint in "
          "module '%s' at %s.", breakpoints[pos].batch_file, p_module, loc_str);
      }
      Free(breakpoints[pos].batch_file);
    }
    else {
      if (batch_file != NULL) {
        print(DRET_SETTING_CHANGE, "Batch file '%s' added to breakpoint in module "
          "'%s' at %s.", batch_file, p_module, loc_str);
      }
      else {
        print(DRET_NOTIFICATION, "Breakpoint already set in module '%s' at %s "
          "with no batch file.", p_module, loc_str);
      }
    }
    breakpoints[pos].batch_file = batch_file != NULL ? mcopystr(batch_file) : NULL;
  }
  Free(loc_str);
}

void TTCN3_Debugger::remove_breakpoint(const char* p_module, const char* p_location)
{
  boolean all_breakpoints = !strcmp(p_module, "all");
  if (p_location != NULL) {
    if (!strcmp(p_location, "all")) {
      boolean found = FALSE;
      for (size_t i = breakpoints.size(); i > 0; --i) {
        if (!strcmp(breakpoints[i - 1].module, p_module)) {
          found = TRUE;
          Free(breakpoints[i - 1].module);
          Free(breakpoints[i - 1].batch_file);
          breakpoints.erase_at(i - 1);
        }
      }
      if (found) {
        print(DRET_SETTING_CHANGE, "Removed all breakpoints in module '%s'.", p_module);
      }
      else {
        print(DRET_NOTIFICATION, "No breakpoints found in module '%s'.", p_module);
      }
      return;
    }
    else {
      if (all_breakpoints) {
        print(DRET_NOTIFICATION, "Unexpected 2nd argument, when the first "
          "argument is 'all'.");
        return;
      }
      int line = 0;
      char* function = NULL;
      if (is_numeric(p_location)) {
        // it's a line breakpoint
        line = strtol(p_location, NULL, 10);
      }
      else {
        // it's a function breakpoint
        function = mcopystr(p_location);
      }
      char* loc_str = function != NULL ? mprintf("function '%s'", function) :
        mprintf("line %d", line);
      size_t pos = find_breakpoint(p_module, line, function);
      if (pos != breakpoints.size()) {
        Free(breakpoints[pos].module);
        Free(breakpoints[pos].function);
        Free(breakpoints[pos].batch_file);
        breakpoints.erase_at(pos);
        print(DRET_SETTING_CHANGE, "Breakpoint removed in module '%s' from %s.",
          p_module, loc_str);
      }
      else {
        print(DRET_NOTIFICATION, "No breakpoint found in module '%s' at %s.",
          p_module, loc_str);
      }
      Free(loc_str);
      return;
    }
  }
  else if (!all_breakpoints) {
    print(DRET_NOTIFICATION, "2 arguments expected, when the first argument is "
      "not 'all'.");
    return;
  }
  // delete all breakpoints if we got this far
  if (breakpoints.empty()) {
    print(DRET_NOTIFICATION, "No breakpoints found.");
  }
  else {
    for (size_t i = 0; i < breakpoints.size(); ++i) {
      Free(breakpoints[i].module);
      Free(breakpoints[i].function);
      Free(breakpoints[i].batch_file);
    }
    breakpoints.clear();
    print(DRET_SETTING_CHANGE, "Removed all breakpoints.");
  }
}

void TTCN3_Debugger::set_automatic_breakpoint(const char* p_event_str,
                                              const char* p_state_str,
                                              const char* p_batch_file)
{
  boolean new_state;
  if (!strcmp(p_state_str, "on")) {
    new_state = TRUE;
  }
  else if(!strcmp(p_state_str, "off")) {
    new_state = FALSE;
  }
  else {
    print(DRET_NOTIFICATION, "Argument 2 is invalid. Expected 'on' or 'off'.");
    return;
  }
  const char* sbp_type_str;
  boolean state_changed;
  char** old_batch_file_ptr;
  if (!strcmp(p_event_str, "fail")) {
    state_changed = (fail_behavior.trigger != new_state);
    fail_behavior.trigger = new_state;
    old_batch_file_ptr = &fail_behavior.batch_file;
    sbp_type_str = "fail verdict";
  }
  else if (!strcmp(p_event_str, "error")) {
    state_changed = (error_behavior.trigger != new_state);
    error_behavior.trigger = new_state;
    old_batch_file_ptr = &error_behavior.batch_file;
    sbp_type_str = "error verdict";
  }
  else {
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'error' or 'fail'.");
    return;
  }
  if (state_changed) {
    print(DRET_SETTING_CHANGE, "Automatic breakpoint at %s switched %s%s%s%s.",
      sbp_type_str, new_state ? "on" : "off",
      new_state ? (p_batch_file != NULL ? " with batch file '" : " with no batch file") : "",
      (p_batch_file != NULL && new_state) ? p_batch_file : "",
      (p_batch_file != NULL && new_state) ? "'" : "");
  }
  else {
    if (new_state) {
      if (*old_batch_file_ptr != NULL) {
        if (p_batch_file != NULL) {
          if (!strcmp(p_batch_file, *old_batch_file_ptr)) {
            print(DRET_NOTIFICATION, "Automatic breakpoint at %s was already "
              "switched on with batch file '%s'.", sbp_type_str, p_batch_file);
          }
          else {
            print(DRET_SETTING_CHANGE, "Batch file was changed from '%s' to '%s' "
              "for automatic breakpoint at %s.", *old_batch_file_ptr, p_batch_file,
              sbp_type_str);
          }
        }
        else {
          print(DRET_SETTING_CHANGE, "Batch file '%s' removed from automatic "
            "breakpoint at %s.", *old_batch_file_ptr, sbp_type_str);
        }
      }
      else {
        if (p_batch_file != NULL) {
          print(DRET_SETTING_CHANGE, "Batch file '%s' added to automatic breakpoint "
            "at %s.", p_batch_file, sbp_type_str);
        }
        else {
          print(DRET_NOTIFICATION, "Automatic breakpoint at %s was already "
              "switched on with no batch file.", sbp_type_str);
        }
      }
    }
    else {
      print(DRET_NOTIFICATION, "Automatic breakpoint at %s was already switched off.");
    }
  }
  Free(*old_batch_file_ptr);
  *old_batch_file_ptr = p_batch_file != NULL ? mcopystr(p_batch_file) : NULL;
}

void TTCN3_Debugger::print_settings()
{
  // on/off switch
  add_to_result("Debugger is switched %s.\n", active ? "on" : "off");
  // output
  char* final_file_name = finalize_file_name(output_file_name);
  char* file_str = output_file != NULL ? mprintf("file '%s'", final_file_name) : NULL;
  Free(final_file_name);
  add_to_result("Output is printed to %s%s%s.\n",
    send_to_console ? "the console" : "",
    (send_to_console && output_file != NULL) ? " and to " : "",
    output_file != NULL ? file_str : "");
  Free(file_str);
  // global batch file
  add_to_result("Global batch file%s%s.\n", global_batch_file != NULL ? ": " : "",
    global_batch_file != NULL ? global_batch_file : " not set");
  // function call data configuration
  add_to_result("Function call data ");
  if (function_calls.cfg == CALLS_TO_FILE) {
    char* final_file_name2 = finalize_file_name(function_calls.file.name);
    add_to_result("sent to file '%s'.\n", final_file_name2);
    Free(final_file_name2);
  }
  else {
    add_to_result("buffer size: ");
    if (function_calls.cfg == CALLS_STORE_ALL) {
      add_to_result("infinite.\n");
    }
    else {
      add_to_result("%d.\n", function_calls.buffer.size);
    }
  }
  // user breakpoints
  if (breakpoints.empty()) {
    add_to_result("No user breakpoints.\n");
  }
  else {
    add_to_result("User breakpoints:\n");
    for (size_t i = 0; i < breakpoints.size(); ++i) {
      const breakpoint_t& bp = breakpoints[i];
      add_to_result("%s ", bp.module);
      if (bp.function == NULL) {
        add_to_result("%d", bp.line);
      }
      else {
        add_to_result("%s", bp.function);
      }
      if (bp.batch_file != NULL) {
        add_to_result(" %s", bp.batch_file);
      }
      add_to_result("\n");
    }
  }
  // automatic breakpoints
  add_to_result("Automatic breakpoints:\nerror %s %s\nfail %s %s",
    error_behavior.trigger ? "on" : "off",
    error_behavior.batch_file != NULL ? error_behavior.batch_file : "",
    fail_behavior.trigger ? "on" : "off",
    fail_behavior.batch_file != NULL ? fail_behavior.batch_file : "");
}

#define STACK_LEVEL (stack_level >= 0) ? (size_t)stack_level : (call_stack.size() - 1)

void TTCN3_Debugger::print_call_stack()
{
  for (size_t i = call_stack.size(); i != 0; --i) {
    add_to_result("%d.\t", (int)call_stack.size() - (int)i + 1);
    call_stack[i - 1].function->print_function();
    if (i - 1 == (STACK_LEVEL)) {
      // mark the active stack level with an asterisk
      add_to_result("*");
    }
    if (i != 1) {
      add_to_result("\n");
    }
  }
}

void TTCN3_Debugger::set_stack_level(int new_level)
{
  if (!halted) {
    print(DRET_NOTIFICATION, "Stack level can only be set if test execution is halted.");
  }
  else if (new_level <= 0 || (size_t)new_level > call_stack.size()) {
    print(DRET_NOTIFICATION, "Invalid new stack level. Expected 1 - %d.",
      (int)call_stack.size());
  }
  else {
    stack_level = (int)call_stack.size() - new_level;
    call_stack[stack_level].function->print_function();
    print(DRET_NOTIFICATION, "Stack level set to:\n%d.\t%s", new_level, command_result);
    Free(command_result);
    command_result = NULL;
  }
}

void TTCN3_Debugger::print_variable(const char* p_var_name)
{
  const variable_t* var = call_stack[STACK_LEVEL].function->find_variable(p_var_name);
  if (var != NULL) {
    add_to_result("[%s] %s%s%s := %s", var->type_name,
      var->module != NULL ? var->module : "", var->module != NULL ? "." : "",
      var->name, (const char*)var->print_function(*var));
  }
  else {
    add_to_result("Variable '%s' not found.", p_var_name);
  }
}

void TTCN3_Debugger::overwrite_variable(const char* p_var_name,
                                        int p_value_element_count,
                                        char** p_value_elements)
{
  variable_t* var = call_stack[STACK_LEVEL].function->find_variable(p_var_name);
  if (var != NULL) {
    if (var->set_function == NULL) {
      print(DRET_NOTIFICATION, "Constant variables cannot be overwritten.");
    }
    else {
      char* new_value_str = NULL;
      for (int i = 0; i < p_value_element_count; ++i) {
        if (i != 0) {
          new_value_str = mputc(new_value_str, ' ');
        }
        new_value_str = mputstr(new_value_str, p_value_elements[i]);
      }
      Module_Param* parsed_mp = process_config_debugger_value(new_value_str);
      // an error message has already been displayed if parsed_mp is NULL
      if (parsed_mp != NULL) {
        try {
          Debugger_Value_Parsing debug_value_parsing;
          boolean handled = var->set_function(*var, *parsed_mp);
          if (!handled) {
            print(DRET_NOTIFICATION, "Variables of type '%s' cannot be overwritten.",
              var->type_name);
          }
          else {
            add_to_result("[%s] %s := %s", var->type_name, var->name,
              (const char*)var->print_function(*var));
          }
        }
        catch (const TC_Error&) {
          // do nothing, an error message has already been displayed in this case
        }
        delete parsed_mp;
      }
    }
  }
  else {
    print(DRET_NOTIFICATION, "Variable '%s' not found.", p_var_name);
  }
}

void TTCN3_Debugger::clean_up_function_calls()
{
  if (function_calls.cfg == CALLS_TO_FILE) {
    if (!TTCN_Runtime::is_hc()) {
      fclose(function_calls.file.ptr);
    }
    Free(function_calls.file.name);
  }
  else if (!TTCN_Runtime::is_hc()) {
    if (function_calls.buffer.size != 0) {
      if (function_calls.buffer.end != -1) {
        for (int i = function_calls.buffer.start;
             i != function_calls.buffer.end;
             i = (i + 1) % function_calls.buffer.size) {
          Free(function_calls.buffer.ptr[i]);
        }
        Free(function_calls.buffer.ptr[function_calls.buffer.end]);
      }
      Free(function_calls.buffer.ptr);
    }
  }
}

void TTCN3_Debugger::configure_function_calls(const char* p_config,
                                              const char* p_file_name)
{
  function_call_data_config_t cfg;
  // check the command's parameters before actually changing anything
  if (strcmp(p_config, "file") == 0) {
    cfg = CALLS_TO_FILE;
  }
  else if (strcmp(p_config, "all") == 0) {
    cfg = CALLS_STORE_ALL;
  }
  else if (is_numeric(p_config)) {
    cfg = CALLS_RING_BUFFER;
  }
  else {
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'file', 'all' or "
      "ring buffer size.");
    return;
  }
  FILE* new_fp = NULL;
  char* final_file_name = NULL;
  boolean same_setting = FALSE;
  int new_size = 0;
  switch (cfg) {
  case CALLS_TO_FILE:
    if (p_file_name != NULL) {
      if (function_calls.cfg == CALLS_TO_FILE &&
          strcmp(p_file_name, function_calls.file.name) == 0) {
        // don't reopen it if it's the same file as before
        same_setting = TRUE;
      }
      else if (!TTCN_Runtime::is_hc()) {
        // don't open any files on HCs, just store settings for future PTCs
        final_file_name = finalize_file_name(p_file_name);
        new_fp = fopen(final_file_name, TTCN_Runtime::is_mtc() ? "w" : "a");
        if (new_fp == NULL) {
          print(DRET_NOTIFICATION, "Failed to open file '%s' for writing.", final_file_name);
          Free(final_file_name);
          return;
        }
      }
    }
    else {
      print(DRET_NOTIFICATION, "Argument 2 (file name) is missing.");
      return;
    }
    break;
  case CALLS_RING_BUFFER:
    new_size = strtol(p_config, NULL, 10);
    same_setting = function_calls.cfg == CALLS_RING_BUFFER &&
      function_calls.buffer.size == new_size;
    break;
  case CALLS_STORE_ALL:
    same_setting = function_calls.cfg == CALLS_STORE_ALL;
    break;
  }
  if (!same_setting) {
    clean_up_function_calls();
    function_calls.cfg = cfg;
    switch (cfg) {
    case CALLS_TO_FILE:
      function_calls.file.name = mcopystr(p_file_name);
      if (!TTCN_Runtime::is_hc()) {
        function_calls.file.ptr = new_fp;
      }
      break;
    case CALLS_RING_BUFFER:
      function_calls.buffer.size = new_size;
      // no break
    case CALLS_STORE_ALL:
      function_calls.buffer.start = 0;
      function_calls.buffer.end = -1;
      if (new_size != 0 && !TTCN_Runtime::is_hc()) {
        // don't allocate buffers on HCs, just store settings for future PTCs
        function_calls.buffer.ptr = (char**)Malloc(new_size * sizeof(char*));
      }
      else {
        function_calls.buffer.ptr = NULL;
      }
      break;
    }
  }
  switch (cfg) {
  case CALLS_TO_FILE:
    print(DRET_SETTING_CHANGE, "Debugger %sset to not store function call data, but to "
      "send them to file '%s'.", same_setting ? "was already " : "", final_file_name);
    Free(final_file_name);
    break;
  case CALLS_RING_BUFFER:
    if (new_size == 0) {
      print(DRET_SETTING_CHANGE, "Debugger %sset to not store function call data.",
        same_setting ? "was already " : "");
    }
    else {
      print(DRET_SETTING_CHANGE, "Debugger %sset to store only the last %d function calls.",
        same_setting ? "was already " : "", new_size);
    }
    break;
  case CALLS_STORE_ALL:
    print(DRET_SETTING_CHANGE, "Debugger %sset to store all function call data.",
      same_setting ? "was already " : "");
    break;
  }
}

void TTCN3_Debugger::print_function_calls(const char* p_amount)
{
  if (function_calls.cfg == CALLS_TO_FILE || function_calls.buffer.size == 0 ||
      function_calls.buffer.end == -1) {
    print(DRET_NOTIFICATION, "No function calls are stored.");
    return;
  }
  int amount;
  int limit = (function_calls.cfg == CALLS_RING_BUFFER &&
               function_calls.buffer.start == (function_calls.buffer.end + 1) %
               function_calls.buffer.size) ?
    function_calls.buffer.size : function_calls.buffer.end + 1;
  boolean invalid_arg = FALSE;
  if (p_amount == NULL || strcmp(p_amount, "all") == 0) {
    amount = limit;
  }
  else if (is_numeric(p_amount)) {
    amount = strtol(p_amount, NULL, 10);
    if (amount == 0) {
      invalid_arg = TRUE;
    }
    else if (amount > limit) {
      amount = limit;
    }
  }
  else {
    invalid_arg = TRUE;
  }
  if (invalid_arg) {
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'all' or non-zero "
      "integer value (number of calls).");
    return;
  }
  for (int i = (function_calls.buffer.end - amount + function_calls.buffer.size + 1) %
       function_calls.buffer.size; amount > 0;
       i = (i + 1) % function_calls.buffer.size, --amount) {
    add_to_result(function_calls.buffer.ptr[i]);
    if (amount > 1) {
      add_to_result("\n");
    }
  }
}

void TTCN3_Debugger::set_output(const char* p_output_type, const char* p_file_name)
{
  FILE* new_fp = NULL;
  boolean file, console;
  boolean same_file = FALSE;
  char* final_file_name = NULL;
  // check the command's parameters before actually changing anything
  if (!strcmp(p_output_type, "console")) {
    file = FALSE;
    console = TRUE;
  }
  else if (!strcmp(p_output_type, "file")) {
    file = TRUE;
    console = FALSE;
  }
  else if (!strcmp(p_output_type, "both")) {
    file = TRUE;
    console = TRUE;
  }
  else {
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'console', 'file' or 'both'.");
    return;
  }
  if (file) {
    if (p_file_name == NULL) {
      print(DRET_NOTIFICATION, "Argument 2 (output file name) is missing.");
      return;
    }
    if (output_file_name != NULL && !strcmp(p_file_name, output_file_name)) {
      // don't reopen it if it's the same file as before
      same_file = TRUE;
    }
    else if (!TTCN_Runtime::is_hc()) {
      // don't open any files on HCs, just store settings for future PTCs
      final_file_name = finalize_file_name(p_file_name);
      new_fp = fopen(final_file_name, TTCN_Runtime::is_mtc() ? "w" : "a");
      if (new_fp == NULL) {
        print(DRET_NOTIFICATION, "Failed to open file '%s' for writing.", final_file_name);
        Free(final_file_name);
        return;
      }
    }
  }
  // print the change notification to the old output
  char* file_str = file ? mprintf("file '%s'", final_file_name) : NULL;
  Free(final_file_name);
  print(DRET_SETTING_CHANGE, "Debugger set to print its output to %s%s%s.",
    console ? "the console" : "", (console && file) ? " and to " : "",
    file ? file_str : "");
  if (file) {
    Free(file_str);
  }
  if (!same_file && !TTCN_Runtime::is_hc()) {
    if (output_file != NULL) {
      fclose(output_file);
    }
    output_file = new_fp;
  }
  send_to_console = console;
  Free(output_file_name);
  if (file) {
    output_file_name = mcopystr(p_file_name);
  }
}

void TTCN3_Debugger::set_global_batch_file(const char* p_state_str,
                                           const char* p_file_name)
{
  boolean delete_old = FALSE;
  boolean copy_new = FALSE;
  if (!strcmp(p_state_str, "on")) {
    if (p_file_name != NULL) {
      if (global_batch_file != NULL) {
        if (!strcmp(p_file_name, global_batch_file)) {
          print(DRET_NOTIFICATION, "Global batch file was already switched on "
            "and set to '%s'.", p_file_name);
        }
        else {
          print(DRET_SETTING_CHANGE, "Global batch file changed from '%s' to '%s'.",
            global_batch_file, p_file_name);
          delete_old = TRUE;
          copy_new = TRUE;
        }
      }
      else {
        print(DRET_SETTING_CHANGE, "Global batch file switched on and set to '%s'.",
          p_file_name);
        copy_new = TRUE;
      }
    }
    else {
      print(DRET_NOTIFICATION, "Missing batch file name argument.");
    }
  }
  else if (!strcmp(p_state_str, "off")) {
    if (global_batch_file != NULL) {
      print(DRET_SETTING_CHANGE, "Global batch file switched off.");
      delete_old = TRUE;
    }
    else {
      print(DRET_NOTIFICATION, "Global batch file was already switched off.");
    }
  }
  else {
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'on' or 'off'.");
  }
  if (delete_old) {
    Free(global_batch_file);
    global_batch_file = NULL;
  }
  if (copy_new) {
    global_batch_file = mcopystr(p_file_name);
  }
}

void TTCN3_Debugger::step(stepping_t p_stepping_type)
{
  if (!halted) {
    print(DRET_NOTIFICATION, "Stepping commands can only be used when test "
      "execution is halted.");
    return;
  }
  stepping_type = p_stepping_type;
  stepping_stack_size = call_stack.size();
  if (!TTCN_Runtime::is_single()) {
    TTCN_Communication::send_debug_continue_req();
  }
  resume();
}

void TTCN3_Debugger::run_to_cursor(const char* p_module, const char* p_location)
{
  // all processes receive this command, since the specified location might be
  // reached in any process, even a PTC that hasn't been created yet
  if (!halted) {
    print(DRET_NOTIFICATION, "The 'run to' command can only be used when test "
      "execution is halted.");
    return;
  }
  temporary_breakpoint.module = mcopystr(p_module);
  if (is_numeric(p_location)) {
    temporary_breakpoint.line = strtol(p_location, NULL, 10);
    temporary_breakpoint.function = NULL;
  }
  else {
    temporary_breakpoint.line = 0;
    temporary_breakpoint.function = mcopystr(p_location);
  }
  resume();
}

void TTCN3_Debugger::halt(const char* p_batch_file, boolean p_run_global_batch)
{
  if (!halted) {
    halted = TRUE;
    Free(temporary_breakpoint.module);
    temporary_breakpoint.module = NULL;
    temporary_breakpoint.line = 0;
    Free(temporary_breakpoint.function);
    temporary_breakpoint.function = NULL;
    if (!TTCN_Runtime::is_hc()) {
      stepping_type = NOT_STEPPING;
      stack_level = call_stack.size() - 1;
      print(DRET_NOTIFICATION, "Test execution halted.");
      if (p_batch_file != NULL) {
        if (TTCN_Runtime::is_single()) {
          TTCN_Debugger_UI::execute_batch_file(p_batch_file);
        }
        else {
          TTCN_Communication::send_debug_batch(p_batch_file);
        }
      }
      else if (p_run_global_batch && global_batch_file != NULL) {
        if (TTCN_Runtime::is_single()) {
          TTCN_Debugger_UI::execute_batch_file(global_batch_file);
        }
        else {
          TTCN_Communication::send_debug_batch(global_batch_file);
        }
      }
      if (TTCN_Runtime::is_single()) {
        if (halted && !halt_at_start) {
          resume();
        }
        else {
          TTCN_Debugger_UI::read_loop();
        }
      }
      else {
        TTCN_Communication::process_debug_messages();
      }
    }
  }
  else {
    print(DRET_NOTIFICATION, "Test execution is already halted.");
  }
}

void TTCN3_Debugger::resume()
{
  if (halted) {
    halted = FALSE;
    stack_level = -1;
    print(DRET_NOTIFICATION, "Test execution resumed.");
  }
  else {
    print(DRET_NOTIFICATION, "Test execution is not halted.");
  }
}

void TTCN3_Debugger::exit_(const char* p_what)
{
  if (!strcmp(p_what, "test")) {
    exiting = FALSE;
  }
  else if (!strcmp(p_what, "all")) {
    exiting = TRUE;
  }
  else {
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'test' or 'all'.");
    return;
  }
  halted = FALSE;
  if (!TTCN_Runtime::is_hc()) {
    print((exiting && TTCN_Runtime::is_mtc()) ? DRET_EXIT_ALL : DRET_NOTIFICATION,
      "Exiting %s.", exiting ? "test execution" : "current test");
    TTCN_Runtime::stop_execution();
  }
}

size_t TTCN3_Debugger::find_breakpoint(const char* p_module, int p_line,
                                       const char* p_function) const
{
  for (size_t i = 0; i < breakpoints.size(); ++i) {
    if (!strcmp(breakpoints[i].module, p_module) && 
        ((p_line != 0 && breakpoints[i].line == p_line) || 
         (p_function != NULL && breakpoints[i].function != NULL &&
          strcmp(breakpoints[i].function, p_function) == 0))) {
      return i;
    }
  }
  return breakpoints.size();
}

TTCN3_Debugger::variable_t* TTCN3_Debugger::find_variable(const void* p_value) const
{
  for (size_t i = 0; i < variables.size(); ++i) {
    if (variables[i]->value == p_value) {
      return variables[i];
    }
  }
  return NULL;
}

char* TTCN3_Debugger::finalize_file_name(const char* p_file_name_skeleton)
{
  if (p_file_name_skeleton == NULL) {
    return NULL;
  }
  size_t len = strlen(p_file_name_skeleton);
  size_t next_idx = 0;
  char* ret_val = NULL;
  for (size_t i = 0; i < len - 1; ++i) {
    if (p_file_name_skeleton[i] == '%') {
      ret_val = mputstrn(ret_val, p_file_name_skeleton + next_idx, i - next_idx);
      switch (p_file_name_skeleton[i + 1]) {
      case 'e': // %e -> executable name
        ret_val = mputstr(ret_val, TTCN_Logger::get_executable_name());
        break;
      case 'h': // %h -> host name
        ret_val = mputstr(ret_val, TTCN_Runtime::get_host_name());
        break;
      case 'p': // %p -> process ID
        ret_val = mputprintf(ret_val, "%ld", (long)getpid());
        break;
      case 'l': { // %l -> login name
        setpwent();
        struct passwd *p = getpwuid(getuid());
        if (NULL != p) {
          ret_val = mputstr(ret_val, p->pw_name);
        }
        endpwent();
        break; }
      case 'r': // %r -> component reference
        if (TTCN_Runtime::is_single()) {
          ret_val = mputstr(ret_val, "single");
        }
        else if (TTCN_Runtime::is_mtc()) {
          ret_val = mputstr(ret_val, "mtc");
        }
        else if (TTCN_Runtime::is_ptc()) {
          ret_val = mputprintf(ret_val, "%d", (component)self);
        }
        break;
      case 'n': // %n -> component name
        if (TTCN_Runtime::is_mtc()) {
          ret_val = mputstr(ret_val, "MTC");
        }
        else if (TTCN_Runtime::is_ptc()) {
          ret_val = mputstr(ret_val, TTCN_Runtime::get_component_name());
        }
        break;
      case '%': // %% -> single %
        ret_val = mputc(ret_val, '%');
        break;
      default: // unknown sequence -> leave it as it is 
        ret_val = mputstrn(ret_val, p_file_name_skeleton + i, 2);
        break;
      }
      next_idx = i + 2;
      ++i;
    }
  }
  if (next_idx < len) {
    ret_val = mputstr(ret_val, p_file_name_skeleton + next_idx);
  }
  return ret_val;
}

void TTCN3_Debugger::test_execution_started()
{
  if (function_calls.cfg != CALLS_TO_FILE) {
    if (function_calls.buffer.size != 0 && function_calls.buffer.end != -1) {
      for (int i = function_calls.buffer.start;
           i != function_calls.buffer.end;
           i = (i + 1) % function_calls.buffer.size) {
        Free(function_calls.buffer.ptr[i]);
      }
      Free(function_calls.buffer.ptr[function_calls.buffer.end]);
    }
    if (function_calls.cfg == CALLS_STORE_ALL) {
      Free(function_calls.buffer.ptr);
      function_calls.buffer.ptr = NULL;
      function_calls.buffer.size = 0;
    }
    function_calls.buffer.start = 0;
    function_calls.buffer.end = -1;
  }
  exiting = FALSE;
  if (TTCN_Runtime::is_single()) {
    TTCN_Debugger_UI::init();
    if (initial_batch_file) {
      halt(initial_batch_file, FALSE);
    }
    else if (halt_at_start) {
      halt(NULL, FALSE);
    }
  }
  halt_at_start = TRUE;
}

void TTCN3_Debugger::test_execution_finished()
{
  stepping_type = NOT_STEPPING;
  Free(temporary_breakpoint.module);
  temporary_breakpoint.module = NULL;
  temporary_breakpoint.line = 0;
  Free(temporary_breakpoint.function);
  temporary_breakpoint.function = NULL;
  last_breakpoint_entry.module = NULL;
  last_breakpoint_entry.line = 0;
  last_breakpoint_entry.stack_size = 0;
  if (TTCN_Runtime::is_single()) {
    TTCN_Debugger_UI::clean_up();
  }
}

void TTCN3_Debugger::print(int return_type, const char* fmt, ...) const
{
  if (TTCN_Runtime::is_hc()) {
    // don't display anything while on the HC process
    return;
  }
  va_list parameters;
  va_start(parameters, fmt);
  char* str = mprintf_va_list(fmt, parameters);
  va_end(parameters);
  if (TTCN_Runtime::is_single()) {
    if (send_to_console) {
      TTCN_Debugger_UI::print(str);
    }
  }
  else {
    TTCN_Communication::send_debug_return_value(return_type, send_to_console ? str : NULL);
  }
  if (output_file != NULL) {
    fseek(output_file, 0, SEEK_END); // in case multiple processes are writing the same file
    fputs(str, output_file);
    fputc('\n', output_file);
    fflush(output_file);
  }
  Free(str);
}

TTCN3_Debugger::TTCN3_Debugger()
{
  enabled = FALSE;
  active = FALSE;
  halted = FALSE;
  output_file = NULL;
  output_file_name = NULL;
  send_to_console = TRUE;
  function_calls.cfg = CALLS_STORE_ALL;
  function_calls.buffer.size = 0;
  function_calls.buffer.start = 0;
  function_calls.buffer.end = -1;
  function_calls.buffer.ptr = NULL;
  last_breakpoint_entry.module = NULL;
  last_breakpoint_entry.line = 0;
  last_breakpoint_entry.stack_size = 0;
  stack_level = -1;
  fail_behavior.trigger = FALSE;
  fail_behavior.batch_file = NULL;
  error_behavior.trigger = FALSE;
  error_behavior.batch_file = NULL;
  global_batch_file = NULL;
  command_result = NULL;
  last_variable_list = NULL;
  stepping_type = NOT_STEPPING;
  stepping_stack_size = 0;
  temporary_breakpoint.module = NULL;
  temporary_breakpoint.line = 0;
  temporary_breakpoint.function = NULL;
  temporary_breakpoint.batch_file = NULL; // not used
  exiting = FALSE;
  halt_at_start = FALSE;
  initial_batch_file = NULL;
}

TTCN3_Debugger::~TTCN3_Debugger()
{
  if (output_file != NULL) {
    fclose(output_file);
    Free(output_file_name);
  }
  for (size_t i = 0; i < breakpoints.size(); ++i) {
    Free(breakpoints[i].module);
    Free(breakpoints[i].function);
    Free(breakpoints[i].batch_file);
  }
  for (size_t i = 0; i < global_scopes.size(); ++i) {
    delete global_scopes[i].scope;
  }
  for (size_t i = 0; i < component_scopes.size(); ++i) {
    delete component_scopes[i].scope;
  }
  for (size_t i = 0; i < variables.size(); ++i) {
    delete variables[i];
  }
  Free(fail_behavior.batch_file);
  Free(error_behavior.batch_file);
  Free(global_batch_file);
  clean_up_function_calls();
  Free(last_variable_list);
}

TTCN3_Debug_Scope* TTCN3_Debugger::add_global_scope(const char* p_module)
{
  named_scope_t global_scope;
  global_scope.name = p_module;
  global_scope.scope = new TTCN3_Debug_Scope();
  global_scopes.push_back(global_scope);
  return global_scope.scope;
}

TTCN3_Debug_Scope* TTCN3_Debugger::add_component_scope(const char* p_component)
{
  named_scope_t component_scope;
  component_scope.name = p_component;
  component_scope.scope = new TTCN3_Debug_Scope();
  component_scopes.push_back(component_scope);
  return component_scope.scope;
}

void TTCN3_Debugger::set_return_value(const CHARSTRING& p_value)
{
  if (active && !call_stack.empty()) {
    call_stack[call_stack.size() - 1].function->set_return_value(p_value);
  }
}

void TTCN3_Debugger::breakpoint_entry(int p_line)
{
  if (active && !call_stack.empty()) {
    const char* module_name = call_stack[call_stack.size() - 1].function->get_module_name();
    boolean trigger = FALSE;
    const char* trigger_type;
    int actual_line;
    const char* batch_file = NULL;
    switch (p_line) {
    case SBP_FAIL_VERDICT:
      trigger = fail_behavior.trigger;
      trigger_type = "Automatic breakpoint (fail verdict) reached at";
      actual_line = TTCN_Location::get_line_number();
      batch_file = fail_behavior.batch_file;
      break;
    case SBP_ERROR_VERDICT:
      trigger = error_behavior.trigger;
      trigger_type = "Automatic breakpoint (error verdict) reached at";
      actual_line = TTCN_Location::get_line_number();
      batch_file = error_behavior.batch_file;
      break;
    default: // code lines
      // first: make sure it's not the same breakpoint entry as last time
      if (p_line == last_breakpoint_entry.line &&
          module_name == last_breakpoint_entry.module) {
        break;
      }
      actual_line = p_line;
      // second: check if a stepping operation ends here
      if (stepping_type == STEP_INTO ||
          (stepping_type == STEP_OVER && call_stack.size() <= stepping_stack_size) ||
          (stepping_type == STEP_OUT && call_stack.size() < stepping_stack_size)) {
        trigger = TRUE;
        trigger_type = "Stepped to";
        break;
      }
      const char* function_name =
        call_stack[call_stack.size() - 1].function->get_function_name();
      // third: check if this is the destination of a 'run to cursor' operation
      if (temporary_breakpoint.module != NULL &&
          strcmp(module_name, temporary_breakpoint.module) == 0 &&
          (p_line == temporary_breakpoint.line ||
           (temporary_breakpoint.function != NULL &&
            last_breakpoint_entry.stack_size == call_stack.size() - 1 &&
            strcmp(temporary_breakpoint.function, function_name) == 0))) {
        trigger = TRUE;
        trigger_type = "Ran to";
        break;
      }
      // fourth: check if the location matches one of the breakpoints in the list
      size_t pos = find_breakpoint(module_name, p_line, NULL);
      if (pos == breakpoints.size() &&
          last_breakpoint_entry.stack_size == call_stack.size() - 1) {
        // this is the first breakpoint entry in the function,
        // check for a matching function breakpoint
        pos = find_breakpoint(module_name, 0, function_name);
      }
      if (pos != breakpoints.size()) {
        trigger = TRUE;
        batch_file = breakpoints[pos].batch_file;
      }
      trigger_type = "User breakpoint reached at";
      break;
    }
    if (trigger) {
      print(DRET_NOTIFICATION, "%s line %d in module '%s'.",
        trigger_type, actual_line, module_name);
      if (!TTCN_Runtime::is_single()) {
        TTCN_Communication::send_debug_halt_req();
      }
      halt(batch_file, TRUE);
    }
    last_breakpoint_entry.module = module_name;
    last_breakpoint_entry.line = p_line;
    last_breakpoint_entry.stack_size = call_stack.size();
  }
}

CHARSTRING TTCN3_Debugger::print_base_var(const TTCN3_Debugger::variable_t& p_var)
{
  const void* ptr = p_var.set_function != NULL ? p_var.value : p_var.cvalue;
  TTCN_Logger::begin_event_log2str();
  if (!strcmp(p_var.type_name, "bitstring")) {
    ((const BITSTRING*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "bitstring template")) {
    ((const BITSTRING_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "boolean")) {
    ((const BOOLEAN*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "boolean template")) {
    ((const BOOLEAN_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "charstring")) {
    ((const CHARSTRING*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "charstring template")) {
    ((const CHARSTRING_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "float")) {
    ((const FLOAT*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "float template")) {
    ((const FLOAT_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "hexstring")) {
    ((const HEXSTRING*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "hexstring template")) {
    ((const HEXSTRING_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "integer")) {
    ((const INTEGER*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "integer template")) {
    ((const INTEGER_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "objid")) {
    ((const OBJID*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "objid template")) {
    ((const OBJID_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "octetstring")) {
    ((const OCTETSTRING*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "octetstring template")) {
    ((const OCTETSTRING_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "universal charstring")) {
    ((const UNIVERSAL_CHARSTRING*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "universal charstring template")) {
    ((const UNIVERSAL_CHARSTRING_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "verdicttype")) {
    ((const VERDICTTYPE*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "verdicttype template")) {
    ((const VERDICTTYPE_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "component")) {
    ((const COMPONENT*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "component template")) {
    ((const COMPONENT_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "port")) {
    ((const PORT*)ptr)->log(); // virtual
  }
  else if (!strcmp(p_var.type_name, "default")) {
    ((const DEFAULT*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "default template")) {
    ((const DEFAULT_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "timer")) {
    ((const TIMER*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "NULL")) {
    ((const ASN_NULL*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "NULL template")) {
    ((const ASN_NULL_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "CHARACTER STRING")) {
    ((const CHARACTER_STRING*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "CHARACTER STRING template")) {
    ((const CHARACTER_STRING_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "EMBEDDED PDV")) {
    ((const EMBEDDED_PDV*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "EMBEDDED PDV template")) {
    ((const EMBEDDED_PDV_template*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "EXTERNAL")) {
    ((const EXTERNAL*)ptr)->log();
  }
  else if (!strcmp(p_var.type_name, "EXTERNAL template")) {
    ((const EXTERNAL_template*)ptr)->log();
  }
  else {
    TTCN_Logger::log_event_str("<unrecognized value or template>");
  }
  return TTCN_Logger::end_event_log2str();
}

boolean TTCN3_Debugger::set_base_var(variable_t& p_var, Module_Param& p_new_value)
{
  if (!strcmp(p_var.type_name, "bitstring")) {
    ((BITSTRING*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "bitstring template")) {
    ((BITSTRING_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "boolean")) {
    ((BOOLEAN*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "boolean template")) {
    ((BOOLEAN_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "charstring")) {
    ((CHARSTRING*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "charstring template")) {
    ((CHARSTRING_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "float")) {
    ((FLOAT*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "float template")) {
    ((FLOAT_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "hexstring")) {
    ((HEXSTRING*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "hexstring template")) {
    ((HEXSTRING_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "integer")) {
    ((INTEGER*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "integer template")) {
    ((INTEGER_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "objid")) {
    ((OBJID*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "objid template")) {
    ((OBJID_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "octetstring")) {
    ((OCTETSTRING*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "octetstring template")) {
    ((OCTETSTRING_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "universal charstring")) {
    ((UNIVERSAL_CHARSTRING*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "universal charstring template")) {
    ((UNIVERSAL_CHARSTRING_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "verdicttype")) {
    ((VERDICTTYPE*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "verdicttype template")) {
    ((VERDICTTYPE_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "component")) {
    ((COMPONENT*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "component template")) {
    ((COMPONENT_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "default")) {
    ((DEFAULT*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "default template")) {
    ((DEFAULT_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "NULL")) {
    ((ASN_NULL*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "NULL template")) {
    ((ASN_NULL_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "CHARACTER STRING")) {
    ((CHARACTER_STRING*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "CHARACTER STRING template")) {
    ((CHARACTER_STRING_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "EMBEDDED PDV")) {
    ((EMBEDDED_PDV*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "EMBEDDED PDV template")) {
    ((EMBEDDED_PDV_template*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "EXTERNAL")) {
    ((EXTERNAL*)p_var.value)->set_param(p_new_value);
  }
  else if (!strcmp(p_var.type_name, "EXTERNAL template")) {
    ((EXTERNAL_template*)p_var.value)->set_param(p_new_value);
  }
  else {
    return FALSE;
  }
  return TRUE;
}

void TTCN3_Debugger::add_to_result(const char* fmt, ...)
{
  va_list parameters;
  va_start(parameters, fmt);
  command_result = mputprintf_va_list(command_result, fmt, parameters);
  va_end(parameters);
}

void TTCN3_Debugger::add_function(TTCN3_Debug_Function* p_function)
{
  function_call_t function_call;
  if (call_stack.empty()) {
    test_execution_started();
    function_call.caller_line = 0;
  }
  else {
    function_call.caller_line = last_breakpoint_entry.line;
  }
  function_call.function = p_function;
  call_stack.push_back(function_call);
}

void TTCN3_Debugger::add_scope(TTCN3_Debug_Scope* p_scope)
{
  if (active && !call_stack.empty()) {
    call_stack[call_stack.size() - 1].function->add_scope(p_scope);
  }
}

void TTCN3_Debugger::remove_function(TTCN3_Debug_Function* p_function)
{
  if (!call_stack.empty() && call_stack[call_stack.size() - 1].function == p_function) {
    boolean removing_test_case = call_stack[call_stack.size() - 1].function->is_test_case();
    int caller_line = call_stack[call_stack.size() - 1].caller_line;
    call_stack.erase_at(call_stack.size() - 1);
    if (call_stack.empty()) {
      test_execution_finished();
    }
    if (caller_line != 0 && (stepping_type == STEP_INTO || stepping_type == STEP_OUT ||
        (stepping_type == STEP_OVER && call_stack.size() != stepping_stack_size))) {
      breakpoint_entry(caller_line);
    }
    if (exiting && TTCN_Runtime::is_single() && !call_stack.empty() && removing_test_case &&
        call_stack[call_stack.size() - 1].function->is_control_part()) {
      // 'exit all' was requested while executing a test case called by a control
      // part, which means the test case caught the original TC_End exception;
      // another exception must be thrown to stop the control part, too
      throw TC_End();
    }
  }
}

void TTCN3_Debugger::remove_scope(TTCN3_Debug_Scope* p_scope)
{
  if (!call_stack.empty()) {
    call_stack[call_stack.size() - 1].function->remove_scope(p_scope);
  }
}

TTCN3_Debugger::variable_t* TTCN3_Debugger::add_variable(const void* p_value,
                                                         const char* p_name,
                                                         const char* p_type,
                                                         const char* p_module,
                                                         TTCN3_Debugger::print_function_t p_print_function)
{
  if (call_stack.empty()) {
    // no call stack yet, so this is a global or component variable
    variable_t* var = find_variable(p_value);
    if (var == NULL) {
      var = new TTCN3_Debugger::variable_t;
      var->cvalue = p_value;
      var->name = p_name;
      var->type_name = p_type;
      var->module = p_module;
      var->print_function = p_print_function;
      var->set_function = NULL;
      variables.push_back(var);
    }
    return var;
  }
  else if (active) {
    // it's a local variable for the top-most function
    return call_stack[call_stack.size() - 1].function->add_variable(
      p_value, p_name, p_type, p_module, p_print_function);
  }
  return NULL;
}

TTCN3_Debugger::variable_t* TTCN3_Debugger::add_variable(void* p_value,
                                                         const char* p_name,
                                                         const char* p_type,
                                                         const char* p_module,
                                                         TTCN3_Debugger::print_function_t p_print_function,
                                                         TTCN3_Debugger::set_function_t p_set_function)
{
  if (call_stack.empty()) {
    // no call stack yet, so this is a global or component variable
    variable_t* var = find_variable(p_value);
    if (var == NULL) {
      var = new TTCN3_Debugger::variable_t;
      var->value = p_value;
      var->name = p_name;
      var->type_name = p_type;
      var->module = p_module;
      var->print_function = p_print_function;
      var->set_function = p_set_function;
      variables.push_back(var);
    }
    return var;
  }
  else if (active) {
    // it's a local variable for the top-most function
    return call_stack[call_stack.size() - 1].function->add_variable(
      p_value, p_name, p_type, p_module, p_print_function, p_set_function);
  }
  return NULL;
}

void TTCN3_Debugger::remove_variable(const variable_t* p_var)
{
  if (active && !call_stack.empty()) {
    call_stack[call_stack.size() - 1].function->remove_variable(p_var);
  }
}

const TTCN3_Debug_Scope* TTCN3_Debugger::get_global_scope(const char* p_module) const
{
  for (size_t i = 0; i < global_scopes.size(); ++i) {
    if (strcmp(global_scopes[i].name, p_module) == 0) {
      return global_scopes[i].scope;
    }
  }
  return NULL;
}

const TTCN3_Debug_Scope* TTCN3_Debugger::get_component_scope(const char* p_component) const
{
  for (size_t i = 0; i < component_scopes.size(); ++i) {
    if (strcmp(component_scopes[i].name, p_component) == 0) {
      return component_scopes[i].scope;
    }
  }
  return NULL;
}

void TTCN3_Debugger::store_function_call(char* p_snapshot)
{
  if (function_calls.cfg == CALLS_RING_BUFFER && function_calls.buffer.size == 0) {
    Free(p_snapshot);
    return;
  }
  // append timestamp to the beginning of the snapshot
  timeval tv;
  gettimeofday(&tv, NULL);
  struct tm *lt = localtime(&tv.tv_sec);
  if (lt != NULL) {
    char* temp = mprintf("%02d:%02d:%02d.%06ld\t%s", lt->tm_hour, lt->tm_min,
      lt->tm_sec, tv.tv_usec, p_snapshot);
    Free(p_snapshot);
    p_snapshot = temp;
  }
  switch (function_calls.cfg) {
  case CALLS_TO_FILE:
    fseek(function_calls.file.ptr, 0, SEEK_END); // in case multiple processes are writing the same file
    fputs(p_snapshot, function_calls.file.ptr);
    Free(p_snapshot);
    fputc('\n', function_calls.file.ptr);
    fflush(function_calls.file.ptr);
    break;
  case CALLS_RING_BUFFER: {
    boolean first = function_calls.buffer.end == -1;
    function_calls.buffer.end = (function_calls.buffer.end + 1) %
      function_calls.buffer.size;
    function_calls.buffer.ptr[function_calls.buffer.end] = p_snapshot;
    if (!first && function_calls.buffer.start == function_calls.buffer.end) {
      function_calls.buffer.start = (function_calls.buffer.start + 1) %
        function_calls.buffer.size;
    }
    break; }
  case CALLS_STORE_ALL:
    if (function_calls.buffer.end == function_calls.buffer.size - 1) {
      function_calls.buffer.size += BUFFER_INCREASE;
      function_calls.buffer.ptr = (char**)Realloc(function_calls.buffer.ptr,
        function_calls.buffer.size * sizeof(char*));
    }
    ++function_calls.buffer.end;
    function_calls.buffer.ptr[function_calls.buffer.end] = p_snapshot;
    break;
  }
}

#define CHECK_NOF_ARGUMENTS(exp_num) \
  if (exp_num != p_argument_count) { \
    print(DRET_NOTIFICATION, "Invalid number of arguments. Expected %d, got %d.", \
      (int)exp_num, (int)p_argument_count); \
    return; \
  }

#define CHECK_NOF_ARGUMENTS_RANGE(min, max) \
  if ((int)min > p_argument_count || (int)max < p_argument_count) { \
    print(DRET_NOTIFICATION, "Invalid number of arguments. Expected at least %d " \
      "and at most %d, got %d.", (int)min, (int)max, p_argument_count); \
    return; \
  }

#define CHECK_NOF_ARGUMENTS_MIN(min) \
  if ((int)min > p_argument_count) { \
    print(DRET_NOTIFICATION, "Invalid number of arguments. Expected at least %d, got %d.", \
      (int)min, p_argument_count); \
    return; \
  }

#define CHECK_INT_ARGUMENT(arg_idx) \
  { \
    if (!is_numeric(p_arguments[arg_idx])) { \
      print(DRET_NOTIFICATION, "Argument %d is not an integer.", (int)(arg_idx + 1)); \
      return; \
    } \
  }

#define CHECK_CALL_STACK(print_msg) \
  if (!active) { \
    if (print_msg) { \
      print(DRET_NOTIFICATION, "This command can only be used if the debugger " \
        "is switched on."); \
    } \
    return; \
  } \
  if (call_stack.empty()) { \
    if (print_msg) { \
      print(DRET_NOTIFICATION, "This command can only be used if the debugger's " \
        "call stack is not empty."); \
    } \
    return; \
  }

void TTCN3_Debugger::execute_command(int p_command, int p_argument_count,
                                     char** p_arguments)
{
  if (!enabled) {
    return;
  }
  for (int i = 0; i < p_argument_count; ++i) {
    if (p_arguments[i] == NULL) {
      print(DRET_NOTIFICATION, "Argument %d is a null pointer.", i + 1);
      return;
    }
  }
  switch (p_command) {
  case D_SWITCH:
    CHECK_NOF_ARGUMENTS(1)
    switch_state(p_arguments[0]);
    break;
  case D_SET_BREAKPOINT:
    CHECK_NOF_ARGUMENTS_RANGE(2, 3)
    set_breakpoint(p_arguments[0], p_arguments[1],
      (p_argument_count == 3) ? p_arguments[2] : NULL);
    break;
  case D_REMOVE_BREAKPOINT:
    CHECK_NOF_ARGUMENTS_RANGE(1, 2)
    remove_breakpoint(p_arguments[0], (p_argument_count == 2) ? p_arguments[1] : NULL);
    break;
  case D_SET_AUTOMATIC_BREAKPOINT:
    CHECK_NOF_ARGUMENTS_RANGE(2, 3)
    set_automatic_breakpoint(p_arguments[0], p_arguments[1],
      (p_argument_count == 3) ? p_arguments[2] : NULL);
    break;
  case D_SET_OUTPUT:
    CHECK_NOF_ARGUMENTS_RANGE(1, 2)
    set_output(p_arguments[0], (p_argument_count == 2) ? p_arguments[1] : NULL);
    break;
  case D_SET_GLOBAL_BATCH_FILE:
    CHECK_NOF_ARGUMENTS_RANGE(1, 2)
    set_global_batch_file(p_arguments[0], (p_argument_count == 2) ? p_arguments[1] : NULL);
    break;
  case D_FUNCTION_CALL_CONFIG:
    CHECK_NOF_ARGUMENTS_RANGE(1, 2)
    configure_function_calls(p_arguments[0], (p_argument_count == 2) ? p_arguments[1] : NULL);
    break;
  case D_PRINT_SETTINGS:
    CHECK_NOF_ARGUMENTS(0)
    print_settings();
    break;
  case D_PRINT_CALL_STACK:
    CHECK_CALL_STACK(TRUE)
    CHECK_NOF_ARGUMENTS(0)
    print_call_stack();
    break;
  case D_SET_STACK_LEVEL:
    CHECK_CALL_STACK(TRUE)
    CHECK_NOF_ARGUMENTS(1)
    CHECK_INT_ARGUMENT(0)
    set_stack_level(str2int(p_arguments[0]));
    break;
  case D_LIST_VARIABLES:
    CHECK_CALL_STACK(TRUE)
    CHECK_NOF_ARGUMENTS_RANGE(0, 2)
    call_stack[STACK_LEVEL].function->list_variables(
      (p_argument_count > 0) ? p_arguments[0] : NULL,
      (p_argument_count == 2) ? p_arguments[1] : NULL);
    break;
  case D_PRINT_VARIABLE:
    CHECK_CALL_STACK(TRUE)
    CHECK_NOF_ARGUMENTS_MIN(1)
    for (int i = 0; i < p_argument_count; ++i) {
      if (i != 0) {
        add_to_result("\n");
      }
      if (!strcmp(p_arguments[i], "$")) {
        // '$' refers to the result of the last D_LIST_VARIABLES command
        // these variable names are separated by spaces
        if (last_variable_list != NULL) {
          size_t len = mstrlen(last_variable_list);
          size_t start = 0;
          for (size_t j = 0; j < len; ++j) {
            if (last_variable_list[j] == ' ') {
              // extract the variable name before this space
              char* var_name = mcopystrn(last_variable_list + start, j - start);
              print_variable(var_name);
              Free(var_name);
              add_to_result("\n");
              start = j + 1;
            }
          }
          // extract the last (or only) variable name
          char* var_name = mcopystrn(last_variable_list + start, len - start);
          print_variable(var_name);
          Free(var_name);
        }
        else {
          add_to_result("No previous " D_LIST_VARIABLES_TEXT " result.");
        }
      }
      else {
        print_variable(p_arguments[i]);
      }
    }
    break;
  case D_OVERWRITE_VARIABLE:
    CHECK_CALL_STACK(TRUE)
    CHECK_NOF_ARGUMENTS_MIN(2)
    overwrite_variable(p_arguments[0], p_argument_count - 1, p_arguments + 1);
    break;
  case D_PRINT_FUNCTION_CALLS:
    CHECK_NOF_ARGUMENTS_RANGE(0, 1)
    print_function_calls((p_argument_count > 0) ? p_arguments[0] : NULL);
    break;
  case D_STEP_OVER:
    CHECK_CALL_STACK(TRUE)
    CHECK_NOF_ARGUMENTS(0)
    step(STEP_OVER);
    break;
  case D_STEP_INTO:
    CHECK_CALL_STACK(TRUE)
    CHECK_NOF_ARGUMENTS(0)
    step(STEP_INTO);
    break;
  case D_STEP_OUT:
    CHECK_CALL_STACK(TRUE)
    CHECK_NOF_ARGUMENTS(0)
    step(STEP_OUT);
    break;
  case D_RUN_TO_CURSOR:
    if (!TTCN_Runtime::is_hc() && !TTCN_Runtime::is_single()) {
      CHECK_CALL_STACK(TTCN_Runtime::is_mtc())
    }
    CHECK_NOF_ARGUMENTS(2)
    run_to_cursor(p_arguments[0], p_arguments[1]);
    break;
  case D_HALT:
    if (!TTCN_Runtime::is_hc() && !TTCN_Runtime::is_single()) {
      CHECK_CALL_STACK(TTCN_Runtime::is_mtc())
    }
    CHECK_NOF_ARGUMENTS(0)
    halt(NULL, FALSE);
    break;
  case D_CONTINUE:
    CHECK_NOF_ARGUMENTS(0)
    resume();
    break;
  case D_EXIT:
    if (!TTCN_Runtime::is_hc() && !TTCN_Runtime::is_single()) {
      CHECK_CALL_STACK(TTCN_Runtime::is_mtc())
    }
    CHECK_NOF_ARGUMENTS(1)
    exit_(p_arguments[0]);
    break;
  case D_SETUP:
    CHECK_NOF_ARGUMENTS_MIN(11)
    if (strlen(p_arguments[0]) > 0) {
      switch_state(p_arguments[0]);
    }
    if (strlen(p_arguments[1]) > 0) {
      set_output(p_arguments[1], p_arguments[2]);
    }
    if (strlen(p_arguments[3]) > 0) {
      set_automatic_breakpoint("error", p_arguments[3],
        strlen(p_arguments[4]) > 0 ? p_arguments[4] : NULL);
    }
    if (strlen(p_arguments[5]) > 0) {
      set_automatic_breakpoint("fail", p_arguments[5],
        strlen(p_arguments[6]) > 0 ? p_arguments[6] : NULL);
    }
    if (strlen(p_arguments[7]) > 0) {
      set_global_batch_file(p_arguments[7],
        strlen(p_arguments[8]) > 0 ? p_arguments[8] : NULL);
    }
    if (strlen(p_arguments[9]) > 0) {
      configure_function_calls(p_arguments[9],
        strlen(p_arguments[10]) > 0 ? p_arguments[10] : NULL);
    }
    for (int i = 11; i < p_argument_count; i += 3) {
      set_breakpoint(p_arguments[i], p_arguments[i + 1],
        strlen(p_arguments[i + 2]) > 0 ? p_arguments[i + 2] : NULL);
    }
    break;
  default:
    print(DRET_NOTIFICATION, "Invalid command received (ID: %d).", p_command);
    return;
  }
  if (command_result != NULL) {
    print(DRET_DATA, command_result);
    if (p_command == D_LIST_VARIABLES) {
      Free(last_variable_list);
      last_variable_list = command_result;
    }
    else {
      Free(command_result);
    }
    command_result = NULL;
  }
}

void TTCN3_Debugger::init_PTC_settings()
{
  if (output_file == NULL && output_file_name != NULL) {
    char* final_file_name = finalize_file_name(output_file_name);
    output_file = fopen(final_file_name, "a");
    if (output_file == NULL) {
      print(DRET_NOTIFICATION, "Failed to open file '%s' for writing.", final_file_name);
    }
    Free(final_file_name);
  }
  if (function_calls.cfg == CALLS_TO_FILE) {
    char* final_file_name = finalize_file_name(function_calls.file.name);
    function_calls.file.ptr = fopen(final_file_name, "a");
    if (function_calls.file.ptr == NULL) {
      print(DRET_NOTIFICATION, "Failed to open file '%s' for writing.", final_file_name);
    }
    Free(final_file_name);
  }
  else if (function_calls.cfg == CALLS_RING_BUFFER && function_calls.buffer.size != 0) {
    function_calls.buffer.ptr = (char**)Malloc(function_calls.buffer.size * sizeof(char*));
  }
}

//////////////////////////////////////////////////////
//////////////// TTCN3_Debug_Scope ///////////////////
//////////////////////////////////////////////////////

TTCN3_Debug_Scope::TTCN3_Debug_Scope()
{
  ttcn3_debugger.add_scope(this);
}

TTCN3_Debug_Scope::~TTCN3_Debug_Scope()
{
  for (size_t i = 0; i < variables.size(); ++i) {
    ttcn3_debugger.remove_variable(variables[i]);
  }
  ttcn3_debugger.remove_scope(this);
}

void TTCN3_Debug_Scope::add_variable(const void* p_value,
                                     const char* p_name,
                                     const char* p_type,
                                     const char* p_module,
                                     TTCN3_Debugger::print_function_t p_print_function)
{
  TTCN3_Debugger::variable_t* var = ttcn3_debugger.add_variable(p_value, p_name,
    p_type, p_module, p_print_function);
  if (var != NULL) {
    variables.push_back(var);
  }
}

void TTCN3_Debug_Scope::add_variable(void* p_value,
                                     const char* p_name,
                                     const char* p_type,
                                     const char* p_module,
                                     TTCN3_Debugger::print_function_t p_print_function,
                                     TTCN3_Debugger::set_function_t p_set_function)
{
  TTCN3_Debugger::variable_t* var = ttcn3_debugger.add_variable(p_value, p_name,
    p_type, p_module, p_print_function, p_set_function);
  if (var != NULL) {
    variables.push_back(var);
  }
}

TTCN3_Debugger::variable_t* TTCN3_Debug_Scope::find_variable(const char* p_name) const
{
  for (size_t i = 0; i < variables.size(); ++i) {
    TTCN3_Debugger::variable_t* var = variables[i];
    if (strcmp(var->name, p_name) == 0) {
      // the string matches the variable's name
      return var;
    }
    else if (var->module != NULL) {
      size_t name_len = strlen(var->name);
      size_t mod_len = strlen(var->module);
      size_t len = strlen(p_name);
      if (len == mod_len + name_len + 1 && p_name[mod_len] == '.' &&
          strncmp(p_name, var->module, mod_len) == 0 &&
          strncmp(p_name + mod_len + 1, var->name, name_len) == 0) {
        // the string matches the variable's name prefixed by its module name
        return var;
      }
    }
  }
  return NULL;
}

void TTCN3_Debug_Scope::list_variables(regex_t* p_posix_regexp, bool& p_first,
                                       const char* p_module) const
{
  for (size_t i = 0; i < variables.size(); ++i) {
    if (p_posix_regexp == NULL ||
        regexec(p_posix_regexp, variables[i]->name, 0, NULL, 0) == 0) {
      boolean imported = p_module != NULL && strcmp(p_module, variables[i]->module) != 0;
      ttcn3_debugger.add_to_result("%s%s%s%s", p_first ? "" : " ",
        imported ? variables[i]->module : "", imported ? "." : "", variables[i]->name);
      p_first = FALSE;
    }
  }
}

//////////////////////////////////////////////////////
/////////////// TTCN3_Debug_Function /////////////////
//////////////////////////////////////////////////////

TTCN3_Debug_Function::TTCN3_Debug_Function(const char* p_name,
                                           const char* p_type,
                                           const char* p_module,
                                           const charstring_list& p_parameter_names,
                                           const charstring_list& p_parameter_types,
                                           const char* p_component_name)
: function_name(p_name), function_type(p_type), module_name(p_module)
, parameter_names(new charstring_list(p_parameter_names))
, parameter_types(new charstring_list(p_parameter_types))
{
  ttcn3_debugger.add_function(this);
  global_scope = ttcn3_debugger.get_global_scope(p_module);
  component_scope = (p_component_name != NULL) ?
    ttcn3_debugger.get_component_scope(p_component_name) : NULL;
  if (function_name == NULL) {
    function_name = p_module; // for control parts
  }
}

TTCN3_Debug_Function::~TTCN3_Debug_Function()
{
  if (ttcn3_debugger.is_on()) {
    char* snapshot = mprintf("[%s]\tfinished\t%s(", function_type, function_name);
    if (parameter_names->size_of() > 0) {
      for (int i = 0; i < parameter_names->size_of(); ++i) {
        if (i > 0) {
          snapshot = mputstr(snapshot, ", ");
        }
        snapshot = mputprintf(snapshot, "[%s] %s := ", (const char*)((*parameter_types)[i]),
          (const char*)((*parameter_names)[i]));
        if ((*parameter_types)[i] == "out" || (*parameter_types)[i] == "inout") {
          const TTCN3_Debugger::variable_t* parameter = find_variable((*parameter_names)[i]);
          snapshot = mputstr(snapshot, parameter->print_function(*parameter));
        }
        else {
          snapshot = mputc(snapshot, '-');
        }
      }
    }
    snapshot = mputc(snapshot, ')');
    if (return_value.is_bound()) {
      snapshot = mputprintf(snapshot, " returned %s", (const char*)return_value);
    }
    ttcn3_debugger.store_function_call(snapshot);
  }
  for (size_t i = 0; i < variables.size(); ++i) {
    delete variables[i];
  }
  delete parameter_names;
  delete parameter_types;
  ttcn3_debugger.remove_function(this);
}

TTCN3_Debugger::variable_t* TTCN3_Debug_Function::add_variable(const void* p_value,
                                                               const char* p_name,
                                                               const char* p_type,
                                                               const char* p_module,
                                                               TTCN3_Debugger::print_function_t p_print_function)
{
  if (ttcn3_debugger.is_on()) {
    TTCN3_Debugger::variable_t* var = new TTCN3_Debugger::variable_t;
    var->cvalue = p_value;
    var->name = p_name;
    var->type_name = p_type;
    var->module = p_module;
    var->print_function = p_print_function;
    var->set_function = NULL;
    variables.push_back(var);
    return var;
  }
  return NULL;
}

TTCN3_Debugger::variable_t* TTCN3_Debug_Function::add_variable(void* p_value,
                                                               const char* p_name,
                                                               const char* p_type,
                                                               const char* p_module,
                                                               TTCN3_Debugger::print_function_t p_print_function,
                                                               TTCN3_Debugger::set_function_t p_set_function)
{
  if (ttcn3_debugger.is_on()) {
    TTCN3_Debugger::variable_t* var = new TTCN3_Debugger::variable_t;
    var->value = p_value;
    var->name = p_name;
    var->type_name = p_type;
    var->module = p_module;
    var->print_function = p_print_function;
    var->set_function = p_set_function;
    variables.push_back(var);
    return var;
  }
  return NULL;
}

void TTCN3_Debug_Function::set_return_value(const CHARSTRING& p_value)
{
  return_value = p_value;
}

void TTCN3_Debug_Function::initial_snapshot() const
{
  if (ttcn3_debugger.is_on()) {
    char* snapshot = mprintf("[%s]\tstarted \t%s(", function_type, function_name);
    if (parameter_names->size_of() > 0) {
      for (int i = 0; i < parameter_names->size_of(); ++i) {
        if (i > 0) {
          snapshot = mputstr(snapshot, ", ");
        }
        snapshot = mputprintf(snapshot, "[%s] %s := ", (const char*)((*parameter_types)[i]),
          (const char*)((*parameter_names)[i]));
        if ((*parameter_types)[i] == "in" || (*parameter_types)[i] == "inout") {
          const TTCN3_Debugger::variable_t* parameter = find_variable((*parameter_names)[i]);
          snapshot = mputstr(snapshot, parameter->print_function(*parameter));
        }
        else {
          snapshot = mputc(snapshot, '-');
        }
      }
    }
    snapshot = mputstr(snapshot, ")");
    ttcn3_debugger.store_function_call(snapshot);
  }
}

void TTCN3_Debug_Function::add_scope(TTCN3_Debug_Scope* p_scope)
{
  scopes.push_back(p_scope);
}

void TTCN3_Debug_Function::remove_scope(TTCN3_Debug_Scope* p_scope)
{
  if (!scopes.empty() && scopes[scopes.size() - 1] == p_scope) {
    scopes.erase_at(scopes.size() - 1);
  }
}

void TTCN3_Debug_Function::remove_variable(const TTCN3_Debugger::variable_t* p_var)
{
  for (size_t i = 0; i < variables.size(); ++i) {
    if (variables[i] == p_var) {
      variables.erase_at(i);
      delete p_var;
      break;
    }
  }
}

TTCN3_Debugger::variable_t* TTCN3_Debug_Function::find_variable(const char* p_name) const
{
  for (size_t i = 0; i < variables.size(); ++i) {
    if (strcmp(variables[i]->name, p_name) == 0) {
      return variables[i];
    }
  }
  // it's not a local variable, it might still be a global or component variable
  if (component_scope != NULL) {
    TTCN3_Debugger::variable_t* res = component_scope->find_variable(p_name);
    if (res != NULL) {
      return res;
    }
  }
  return (global_scope != NULL) ? global_scope->find_variable(p_name) : NULL;
}

void TTCN3_Debug_Function::print_function() const
{
  ttcn3_debugger.add_to_result("[%s]\t%s(", function_type, function_name);
  if (parameter_names->size_of() > 0) {
    for (int i = 0; i < parameter_names->size_of(); ++i) {
      if (i > 0) {
        ttcn3_debugger.add_to_result(", ");
      }
      const TTCN3_Debugger::variable_t* parameter = find_variable((*parameter_names)[i]);
      ttcn3_debugger.add_to_result("[%s] %s := %s", (const char*)(*parameter_types)[i],
        (const char*)(*parameter_names)[i], (const char*)parameter->print_function(*parameter));
    }
  }
  ttcn3_debugger.add_to_result(")");
}

void TTCN3_Debug_Function::list_variables(const char* p_scope, const char* p_filter) const
{
  boolean first = TRUE;
  boolean list_local = FALSE;
  boolean list_global = FALSE;
  boolean list_comp = FALSE;
  if (p_scope == NULL || !strcmp(p_scope, "all")) {
    list_local = TRUE;
    list_global = TRUE;
    list_comp = TRUE;
  }
  else if (!strcmp(p_scope, "local")) {
    list_local = TRUE;
  }
  else if (!strcmp(p_scope, "global")) {
    list_global = TRUE;
  }
  else if (!strcmp(p_scope, "comp")) {
    list_comp = TRUE;
  }
  else {
    ttcn3_debugger.print(DRET_NOTIFICATION, "Argument 1 is invalid. "
      "Expected 'local', 'global', 'comp' or 'all'.");
    return;
  }
  regex_t* posix_regexp = NULL;
  if (p_filter != NULL) {
    char* posix_str = TTCN_pattern_to_regexp(p_filter);
    if (posix_str == NULL) {
      ttcn3_debugger.print(DRET_NOTIFICATION, "Argument 2 is invalid. "
        "Expected a valid TTCN-3 character pattern.");
      return;
    }
    posix_regexp = new regex_t;
    int ret_val = regcomp(posix_regexp, posix_str, REG_EXTENDED | REG_NOSUB);
    Free(posix_str);
    if (ret_val != 0) {
      char msg[512];
      regerror(ret_val, posix_regexp, msg, sizeof(msg));
      regfree(posix_regexp);
      delete posix_regexp;
      ttcn3_debugger.print(DRET_NOTIFICATION, "Compilation of POSIX regular "
        "expression failed.");
      return;
    }
  }
  if (list_local) {
    for (size_t i = 0; i < variables.size(); ++i) {
      if (posix_regexp == NULL ||
          regexec(posix_regexp, variables[i]->name, 0, NULL, 0) == 0) {
        ttcn3_debugger.add_to_result("%s%s", first ? "" : " ", variables[i]->name);
        first = FALSE;
      }
    }
  }
  if (list_global && global_scope != NULL && global_scope->has_variables()) {
    global_scope->list_variables(posix_regexp, first, module_name);
  }
  if (list_comp && component_scope != NULL && component_scope->has_variables()) {
    component_scope->list_variables(posix_regexp, first, NULL);
  }
  if (first) {
    ttcn3_debugger.print(DRET_NOTIFICATION, "No variables found.");
  }
  if (posix_regexp != NULL) {
    regfree(posix_regexp);
    delete posix_regexp;
  }
}

boolean TTCN3_Debug_Function::is_control_part() const
{
  return !strcmp(function_type, "control");
}

boolean TTCN3_Debug_Function::is_test_case() const
{
  return !strcmp(function_type, "testcase");
}
