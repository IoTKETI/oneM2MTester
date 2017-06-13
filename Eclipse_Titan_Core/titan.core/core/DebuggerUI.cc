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

#include "DebuggerUI.hh"
#include "DebugCommands.hh"
#include "Debugger.hh"
#include <stdio.h>
#include <ctype.h>

#ifdef ADVANCED_DEBUGGER_UI
#include "../mctr2/editline/libedit/src/editline/readline.h"
// use a different file, than the MCTR CLI, since not all commands are the same
#define TTCN3_HISTORY_FILENAME ".ttcn3_history_single"
#endif

#define PROMPT_TEXT "DEBUG> "
#define BATCH_TEXT "batch"
#define HELP_TEXT "help"

const TTCN_Debugger_UI::command_t TTCN_Debugger_UI::debug_command_list[] = {
  { D_SWITCH_TEXT, D_SWITCH, D_SWITCH_TEXT " on|off",
    "Switch the debugger on or off." },
  { D_SET_BREAKPOINT_TEXT, D_SET_BREAKPOINT,
    D_SET_BREAKPOINT_TEXT " <module> <line>|<function> [<batch_file>]",
    "Add a breakpoint at the specified location, or change the batch file of "
    "an existing breakpoint." },
  { D_REMOVE_BREAKPOINT_TEXT, D_REMOVE_BREAKPOINT,
    D_REMOVE_BREAKPOINT_TEXT " all|<module> [all|<line>|<function>]",
    "Remove a breakpoint, or all breakpoints from a module, or all breakpoints "
    "from all modules." },
  { D_SET_AUTOMATIC_BREAKPOINT_TEXT, D_SET_AUTOMATIC_BREAKPOINT,
    D_SET_AUTOMATIC_BREAKPOINT_TEXT " error|fail on|off [<batch_file>]",
    "Switch an automatic breakpoint (truggered by an event) on or off, and/or "
    "change its batch file." },
  { D_SET_OUTPUT_TEXT, D_SET_OUTPUT,
    D_SET_OUTPUT_TEXT " console|file|both [file_name]",
    "Set the output of the debugger." },
  { D_SET_GLOBAL_BATCH_FILE_TEXT, D_SET_GLOBAL_BATCH_FILE,
    D_SET_GLOBAL_BATCH_FILE_TEXT " on|off [batch_file_name]",
    "Set whether a batch file should be executed automatically when test execution "
    "is halted (breakpoint-specific batch files override this setting)." },
  { D_FUNCTION_CALL_CONFIG_TEXT, D_FUNCTION_CALL_CONFIG,
    D_FUNCTION_CALL_CONFIG_TEXT " file|<limit>|all [<file_name>]",
    "Configure the storing of function call data." },
  { D_PRINT_SETTINGS_TEXT, D_PRINT_SETTINGS, D_PRINT_SETTINGS_TEXT,
    "Prints the debugger's settings." },
  { D_PRINT_CALL_STACK_TEXT, D_PRINT_CALL_STACK, D_PRINT_CALL_STACK_TEXT,
    "Print call stack." },
  { D_SET_STACK_LEVEL_TEXT, D_SET_STACK_LEVEL, D_SET_STACK_LEVEL_TEXT " <level>",
    "Set the stack level to print debug information from." },
  { D_LIST_VARIABLES_TEXT, D_LIST_VARIABLES,
    D_LIST_VARIABLES_TEXT " [local|global|comp|all] [pattern]",
    "List variable names." },
  { D_PRINT_VARIABLE_TEXT, D_PRINT_VARIABLE,
    D_PRINT_VARIABLE_TEXT " <variable_name>|$ [{ <variable_name>|$}]",
    "Print current value of one or more variables ('$' is substituted with the "
    "result of the last " D_LIST_VARIABLES_TEXT " command)." },
  { D_OVERWRITE_VARIABLE_TEXT, D_OVERWRITE_VARIABLE,
    D_OVERWRITE_VARIABLE_TEXT " <variable_name> <value>",
    "Overwrite the current value of a variable." },
  { D_PRINT_FUNCTION_CALLS_TEXT, D_PRINT_FUNCTION_CALLS,
    D_PRINT_FUNCTION_CALLS_TEXT " [all|<amount>]",
    "Print function call data." },
  { D_STEP_OVER_TEXT, D_STEP_OVER, D_STEP_OVER_TEXT,
    "Resume test execution until the next line of code (in this function or the "
    "caller function)." },
  { D_STEP_INTO_TEXT, D_STEP_INTO, D_STEP_INTO_TEXT,
    "Resume test execution until the next line of code (on any stack level)." },
  { D_STEP_OUT_TEXT, D_STEP_OUT, D_STEP_OUT_TEXT,
    "Resume test execution until the next line of code in the caller function." },
  { D_RUN_TO_CURSOR_TEXT, D_RUN_TO_CURSOR,
    D_RUN_TO_CURSOR_TEXT " <module> <line>|<function>",
    "Resume test execution until the specified location." },
  { D_HALT_TEXT, D_HALT, D_HALT_TEXT, "Halt test execution." },
  { D_CONTINUE_TEXT, D_CONTINUE, D_CONTINUE_TEXT, "Resume halted test execution." },
  { D_EXIT_TEXT, D_EXIT, D_EXIT_TEXT " test|all",
    "Exit the current test or the execution of all tests." },
  { NULL, D_ERROR, NULL, NULL }
};

#ifdef ADVANCED_DEBUGGER_UI
char* TTCN_Debugger_UI::ttcn3_history_filename = NULL;
#endif

/** local function for extracting the command name and its arguments from an
  * input line
  * @param arguments [in] input line
  * @param len [in] length of the input line
  * @param start [in] indicates the position to start searching from
  * @param start [out] the next argument's start position (set to len if no further
  * arguments were found)
  * @param end [out] the position of the first character after the next argument */
static void get_next_argument_loc(const char* arguments, size_t len, size_t& start, size_t& end)
{
  while (start < len && isspace(arguments[start])) {
    ++start;
  }
  end = start;
  while (end < len && !isspace(arguments[end])) {
    ++end;
  }
}

void TTCN_Debugger_UI::process_command(const char* p_line_read)
{
  // locate the command text
  size_t len = strlen(p_line_read);
  size_t start = 0;
  size_t end = 0;
  get_next_argument_loc(p_line_read, len, start, end);
  if (start == len) {
    // empty command
    return;
  }
#ifdef ADVANCED_DEBUGGER_UI
  add_history(p_line_read + start);
#endif
  for (const command_t *command = debug_command_list; command->name != NULL;
       ++command) {
    if (!strncmp(p_line_read + start, command->name, end - start)) {
      // count the arguments
      int argument_count = 0;
      size_t start_tmp = start;
      size_t end_tmp = end;
      while (start_tmp < len) {
        start_tmp = end_tmp;
        get_next_argument_loc(p_line_read, len, start_tmp, end_tmp);
        if (start_tmp < len) {
          ++argument_count;
        }
      }
      // extract the arguments into a string array
      char** arguments;
      if (argument_count > 0) {
        arguments = new char*[argument_count];
        for (int i = 0; i < argument_count; ++i) {
          start = end;
          get_next_argument_loc(p_line_read, len, start, end);
          arguments[i] = mcopystrn(p_line_read + start, end - start);
        }
      }
      else {
        arguments = NULL;
      }
      ttcn3_debugger.execute_command(command->commandID, argument_count, arguments);
      if (argument_count > 0) {
        for (int i = 0; i < argument_count; ++i) {
          Free(arguments[i]);
        }
        delete [] arguments;
      }
      return;
    }
  }
  if (!strncmp(p_line_read + start, BATCH_TEXT, end - start)) {
    start = end;
    get_next_argument_loc(p_line_read, len, start, end); // just to skip to the argument
    // the entire argument list is treated as one file name (even if it contains spaces)
    execute_batch_file(p_line_read + start);
    return;
  }
  if (!strncmp(p_line_read + start, HELP_TEXT, end - start)) {
    start = end;
    get_next_argument_loc(p_line_read, len, start, end); // just to skip to the argument
    help(p_line_read + start);
    return;
  }
  puts("Unknown command, try again...");
}

void TTCN_Debugger_UI::help(const char* p_argument)
{
  if (*p_argument == 0) {
    puts("Help is available for the following commands:");
    printf(BATCH_TEXT);
    for (const command_t *command = debug_command_list;
         command->name != NULL; command++) {
      printf(" %s", command->name);
    }
    putchar('\n');
  }
  else {
    for (const command_t *command = debug_command_list;
         command->name != NULL; command++) {
      if (!strncmp(p_argument, command->name, strlen(command->name))) {
        printf("%s usage: %s\n%s\n", command->name, command->synopsis,
          command->description);
        return;
      }
    }
    if (!strcmp(p_argument, BATCH_TEXT)) {
      puts(BATCH_TEXT " usage: " BATCH_TEXT "\nRun commands from batch file.");
    }
    else {
      printf("No help for %s.\n", p_argument);
    }
  }
}

void TTCN_Debugger_UI::init()
{
#ifdef ADVANCED_DEBUGGER_UI
  // initialize history library
  using_history();
  // calculate history file name
  const char *home_directory = getenv("HOME");
  if (home_directory == NULL) {
    home_directory = ".";
  }
  ttcn3_history_filename = mprintf("%s/%s", home_directory, TTCN3_HISTORY_FILENAME);
  // read history from file, don't bother if it does not exist
  read_history(ttcn3_history_filename);
  // set our own command completion function
  rl_completion_entry_function = (Function*)complete_command;
#endif
}

void TTCN_Debugger_UI::clean_up()
{
#ifdef ADVANCED_DEBUGGER_UI
  if (write_history(ttcn3_history_filename)) {
    puts("Could not save debugger command history.");
  }
  Free(ttcn3_history_filename);
#endif
}

void TTCN_Debugger_UI::read_loop()
{
  while (ttcn3_debugger.is_halted()) {
#ifdef ADVANCED_DEBUGGER_UI
    // print the prompt and read a line using the readline(), which
    // automatically handles command completion and command history
    char* line = readline(PROMPT_TEXT);
    if (line != NULL) {
#else
    // simplified debugger UI: use a simple fgets
    printf(PROMPT_TEXT);
    char line[1024];
    char* res = fgets(line, sizeof(line), stdin);
    if (res != NULL) {
#endif
      process_command(line);
#ifdef ADVANCED_DEBUGGER_UI     
      free(line);
#endif
    }
    else {
      // EOF was received -> exit all
      puts("exit all");
      char** args = new char*[1];
      args[0] = const_cast<char*>("all");
      ttcn3_debugger.execute_command(D_EXIT, 1, args);
      delete [] args;
    }
  }
}

void TTCN_Debugger_UI::execute_batch_file(const char* p_file_name)
{
  FILE* fp = fopen(p_file_name, "r");
  if (fp == NULL) {
    printf("Failed to open file '%s' for reading.\n", p_file_name);
    return;
  }
  else {
    printf("Executing batch file '%s'.\n", p_file_name);
  }
  char line[1024];
  while (fgets(line, sizeof(line), fp) != NULL) {
    size_t len = strlen(line);
    if (line[len - 1] == '\n') {
      line[len - 1] = '\0';
      --len;
    }
    if (len != 0) {
      printf("%s\n", line);
      process_command(line);
    }
  }
  if (!feof(fp)) {
    printf("Error occurred while reading batch file '%s' (error code: %d).\n",
      p_file_name, ferror(fp));
  }
  fclose(fp);
}

void TTCN_Debugger_UI::print(const char* p_str)
{
  puts(p_str);
}

#ifdef ADVANCED_DEBUGGER_UI
char* TTCN_Debugger_UI::complete_command(const char* p_prefix, int p_state)
{
  static int command_index;
  static size_t prefix_len;
  const char *command_name;

  if (p_state == 0) {
    command_index = 0;
    prefix_len = strlen(p_prefix);
  }
  
  while ((command_name = debug_command_list[command_index].name)) {
    ++command_index;
    if (strncmp(p_prefix, command_name, prefix_len) == 0) {
      // must allocate buffer for returned string (readline() frees it)
      return strdup(command_name);
    }
  }
  // no match found
  return NULL;
}
#endif
