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
 *   Bene, Tamas
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *   Roland Gecse - author
 *
 ******************************************************************************/
//
// Description:           Implementation file for Cli
//
//----------------------------------------------------------------------------
#include "Cli.h"
#include "../mctr/MainController.h"

#include <stdio.h>
#include "../editline/libedit/src/editline/readline.h"
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "../../common/version_internal.h"
#include "../../common/memory.h"
#include "../../common/config_preproc.h"
#include "../../core/DebugCommands.hh"

#define PROMPT "MC2> "
#define CMTC_TEXT "cmtc"
#define SMTC_TEXT "smtc"
#define EMTC_TEXT "emtc"
#define STOP_TEXT "stop"
#define PAUSE_TEXT "pause"
#define CONTINUE_TEXT "continue"
#define INFO_TEXT "info"
#define HELP_TEXT "help"
#define RECONF_TEXT "reconf"
#define LOG_TEXT "log"
#define SHELL_TEXT "!"
#define EXIT_TEXT "quit"
#define EXIT_TEXT2 "exit"
#define BATCH_TEXT "batch"
#define SHELL_ESCAPE '!'
#define TTCN3_HISTORY_FILENAME ".ttcn3_history"

using mctr::MainController;
using namespace cli;

/**
 * shell_mode == TRUE while editing a command line that is passed to the shell
 */
static boolean shell_mode;

struct Command {
  const char *name;
  callback_t callback_function;
  const char *synopsis;
  const char *description;
};

static const Command command_list[] = {
  { CMTC_TEXT, &Cli::cmtcCallback, CMTC_TEXT " [hostname]",
    "Create the MTC." },
  { SMTC_TEXT, &Cli::smtcCallback,
    SMTC_TEXT " [module_name[[.control]|.testcase_name|.*]",
    "Start MTC with control part, test case or all test cases." },
  { STOP_TEXT, &Cli::stopCallback, STOP_TEXT,
    "Stop test execution." },
  { PAUSE_TEXT, &Cli::pauseCallback, PAUSE_TEXT " [on|off]",
    "Set whether to interrupt test execution after each "
    "test case." },
  { CONTINUE_TEXT, &Cli::continueCallback, CONTINUE_TEXT,
    "Resumes interrupted test execution." },
  { EMTC_TEXT, &Cli::emtcCallback, EMTC_TEXT, "Terminate MTC." },
  { LOG_TEXT, &Cli::logCallback, LOG_TEXT " [on|off]",
    "Enable/disable console logging." },
  { INFO_TEXT, &Cli::infoCallback, INFO_TEXT,
    "Display test configuration information." },
  { RECONF_TEXT, &Cli::reconfCallback, RECONF_TEXT " [config_file]",
    "Reload configuration file." },
  { HELP_TEXT, &Cli::helpCallback, HELP_TEXT " <command>",
    "Display help on command." },
  { SHELL_TEXT, &Cli::shellCallback, SHELL_TEXT "[shell cmds]",
    "Execute commands in subshell." },
  { EXIT_TEXT, &Cli::exitCallback, EXIT_TEXT, "Exit Main Controller." },
  { EXIT_TEXT2, &Cli::exitCallback, EXIT_TEXT2, "Exit Main Controller." },
  { BATCH_TEXT, &Cli::executeBatchFile, BATCH_TEXT " <batch_file>",
    "Run commands from batch file." },
  { NULL, NULL, NULL, NULL }
};

struct DebugCommand {
  const char *name;
  int commandID;
  const char *synopsis;
  const char *description;
};

static const DebugCommand debug_command_list[] = {
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
  { D_LIST_COMPONENTS_TEXT, D_LIST_COMPONENTS, D_LIST_COMPONENTS_TEXT,
    "List the test components currently running debuggable code." },
  { D_SET_COMPONENT_TEXT, D_SET_COMPONENT,
    D_SET_COMPONENT_TEXT " <component name>|<component_reference>",
    "Set the test component to print debug information from." },
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

Cli::Cli()
{
  loggingEnabled = TRUE;
  exitFlag = FALSE;
  waitState = WAIT_NOTHING;
  executeListIndex = 0;

  if (pthread_mutex_init(&mutex, NULL)) {
    perror("Cli::Cli: pthread_mutex_init failed.");
    exit(EXIT_FAILURE);
  }
  if (pthread_cond_init(&cond, NULL)) {
    perror("Cli::Cli: pthread_cond_init failed.");
    exit(EXIT_FAILURE);
  }
  cfg_file_name = NULL;
}

Cli::~Cli()
{
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
  Free(cfg_file_name);
}

//----------------------------------------------------------------------------
// MANDATORY

int Cli::enterLoop(int argc, char *argv[])
{
  // Parameter check: mctr [config file name]
  if (argc > 2) {
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  printWelcome();

  if (argc == 2) {
    cfg_file_name = mcopystr(argv[1]);
    printf("Using configuration file: %s\n", cfg_file_name);
    if (process_config_read_file(cfg_file_name, &mycfg)) {
      puts("Error was found in the configuration file. Exiting.");
      cleanUp();
      return EXIT_FAILURE;
    }
    else {
      MainController::set_kill_timer(mycfg.kill_timer);

      for (int i = 0; i < mycfg.group_list_len; ++i) {
        MainController::add_host(mycfg.group_list[i].group_name,
          mycfg.group_list[i].host_name);
      }

      for (int i = 0; i < mycfg.component_list_len; ++i) {
        MainController::assign_component(mycfg.component_list[i].host_or_group,
          mycfg.component_list[i].component);
      }
    }
  }

  int ret_val;
  if (mycfg.num_hcs <= 0) ret_val = interactiveMode();
  else ret_val = batchMode();

  cleanUp();
  return ret_val;
}

//----------------------------------------------------------------------------
// MANDATORY

void Cli::status_change()
{
  lock();
  if (waitState != WAIT_NOTHING && conditionHolds(waitState)) {
    waitState = WAIT_NOTHING;
    signal();
  }
  unlock();
}

//----------------------------------------------------------------------------
// MANDATORY

void Cli::error(int /*severity*/, const char *message)
{
  printf("Error: %s\n", message);
  fflush(stdout);
  // TODO: Error handling based on the MC state where the error happened
}

//----------------------------------------------------------------------------
// MANDATORY

void Cli::notify(const struct timeval *timestamp, const char *source,
  int /*severity*/, const char *message)
{
  if (loggingEnabled)
  {
    switch(mycfg.tsformat){
      case TSF_TIME:       // handled together
      case TSF_DATE_TIME:
        {
          time_t tv_sec = timestamp->tv_sec;
          struct tm *lt = localtime(&tv_sec);
          if (lt == NULL) {
            printf("localtime() call failed.");
            printf("%s: %s\n", source, message);
            fflush(stdout);
            break;
          }
          if (mycfg.tsformat == TSF_TIME) {
            printf("%02d:%02d:%02d.%06ld %s: %s\n",
                  lt->tm_hour, lt->tm_min, lt->tm_sec, timestamp->tv_usec,source, message);
          } else {
            static const char * const month_names[] = { "Jan", "Feb", "Mar",
              "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
            printf("%4d/%s/%02d %02d:%02d:%02d.%06ld  %s: %s\n",
                             lt->tm_year + 1900, month_names[lt->tm_mon],
                             lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec,
                             timestamp->tv_usec
                             ,source, message);
          }
        }
        fflush(stdout);
        break;
      case TSF_SEC:
        printf("%ld.%06ld %s: %s\n", static_cast<long>( timestamp->tv_sec ), timestamp->tv_usec, source, message);
        fflush(stdout);
      
        break;
      default:
        printf("%s: %s\n", source, message);
        fflush(stdout);
        break;
    }
  }
}

//----------------------------------------------------------------------------
// PRIVATE

void Cli::cleanUp()
{
}

//----------------------------------------------------------------------------
// PRIVATE

void Cli::printWelcome()
{
  printf("\n"
    "*************************************************************************\n"
    "* TTCN-3 Test Executor - Main Controller 2                              *\n"
    "* Version: %-40s                     *\n"
    "* Copyright (c) 2000-2017 Ericsson Telecom AB                           *\n"
    "* All rights reserved. This program and the accompanying materials      *\n"
    "* are made available under the terms of the Eclipse Public License v1.0 *\n"
    "* which accompanies this distribution, and is available at              *\n"
    "* http://www.eclipse.org/legal/epl-v10.html                             *\n"
    "*************************************************************************\n"
    "\n", PRODUCT_NUMBER);
}

void Cli::printUsage(const char *prg_name)
{
  fprintf(stderr,
    "TTCN-3 Test Executor - Main Controller 2\n"
    "Version: " PRODUCT_NUMBER "\n\n"
    "usage: %s [configuration_file]\n"
    "where: the optional 'configuration_file' parameter specifies the name "
    "and\nlocation of the main controller configuration file"
    "\n", prg_name);
}

//----------------------------------------------------------------------------
// PRIVATE

int Cli::interactiveMode()
{
  // Initialize history library
  using_history();
  // Tailor $HOME/...
  const char *home_directory = getenv("HOME");
  if(home_directory == NULL)
    home_directory = ".";
  char *ttcn3_history_filename = mprintf("%s/%s", home_directory,
    TTCN3_HISTORY_FILENAME);
  // Read history from file, don't bother if it does not exist!
  read_history(ttcn3_history_filename);
  // Set our own command completion function
  rl_completion_entry_function = (Function*)completeCommand;
  // Override rl_getc() in order to detect shell mode
  rl_getc_function = getcWithShellDetection;

  // TCP listen port parameter, returns TCP port on which it listens!
  // Returns 0 on error.
  if (MainController::start_session(mycfg.local_addr, mycfg.tcp_listen_port,
    mycfg.unix_sockets_enabled) == 0) {
    puts("Initialization of TCP server failed. Exiting.");
    Free(ttcn3_history_filename);
    return EXIT_FAILURE;
  }

  do {
    char *line_read = readline(PROMPT);
    if (line_read != NULL) {
      shell_mode = FALSE;
      stripLWS(line_read);
      if (*line_read) {
        add_history(line_read); // history maintains its own copy
        // process and free line
        processCommand(line_read);
        free(line_read);
        line_read = NULL;
      }
    } else {
      // EOF was received
      puts("exit");
      exitCallback("");
    }
  } while (!exitFlag);

  if (write_history(ttcn3_history_filename))
    perror("Could not save history.");
  Free(ttcn3_history_filename);

  return EXIT_SUCCESS;
}

int Cli::batchMode()
{
  printf("Entering batch mode. Waiting for %d HC%s to connect...\n",
    mycfg.num_hcs, mycfg.num_hcs > 1 ? "s" : "");
  if (mycfg.execute_list_len <= 0) {
    puts("No [EXECUTE] section was given in the configuration file. "
      "Exiting.");
    return EXIT_FAILURE;
  }
  boolean error_flag = FALSE;
  // start to listen on TCP port
  if (MainController::start_session(mycfg.local_addr, mycfg.tcp_listen_port,
    mycfg.unix_sockets_enabled) == 0) {
    puts("Initialization of TCP server failed. Exiting.");
    return EXIT_FAILURE;
  }
  waitMCState(WAIT_HC_CONNECTED);
  // download config file
  MainController::configure(mycfg.config_read_buffer);
  waitMCState(WAIT_ACTIVE);
  if (MainController::get_state() != mctr::MC_ACTIVE) {
    puts("Error during initialization. Cannot continue in batch mode.");
    error_flag = TRUE;
  }
  if (!error_flag) {
    // create MTC on firstly connected HC
    MainController::create_mtc(0);
    waitMCState(WAIT_MTC_CREATED);
    if (MainController::get_state() != mctr::MC_READY) {
      puts("Creation of MTC failed. Cannot continue in batch mode.");
      error_flag = TRUE;
    }
  }
  if (!error_flag) {
    // execute each item of the list
    for (int i = 0; i < mycfg.execute_list_len; i++) {
      executeFromList(i);
      waitMCState(WAIT_MTC_READY);
      if (MainController::get_state() != mctr::MC_READY) {
        puts("MTC terminated unexpectedly. Cannot continue in batch "
          "mode.");
        error_flag = TRUE;
        break;
      }
    }
  }
  if (!error_flag) {
    // terminate the MTC
    MainController::exit_mtc();
    waitMCState(WAIT_MTC_TERMINATED);
  }
  // now MC must be in state MC_ACTIVE anyway
  // shutdown MC
  MainController::shutdown_session();
  waitMCState(WAIT_SHUTDOWN_COMPLETE);
  if (error_flag) return EXIT_FAILURE;
  else return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
// PRIVATE

void Cli::processCommand(char *line_read)
{
  for (const Command *command = command_list; command->name != NULL;
       command++) {
    size_t command_name_len = strlen(command->name);
    if (!strncmp(line_read, command->name, command_name_len)) {
      memset(line_read, ' ', command_name_len);
      stripLWS(line_read);
      (this->*command->callback_function)(line_read);
      return;
    }
  }
  for (const DebugCommand* command = debug_command_list; command->name != NULL;
       command++) {
    size_t command_name_len = strlen(command->name);
    if (!strncmp(line_read, command->name, command_name_len)) {
      memset(line_read, ' ', command_name_len);
      stripLWS(line_read);
      MainController::debug_command(command->commandID, line_read);
      if (waitState == WAIT_EXECUTE_LIST && command->commandID == D_EXIT &&
          !strcmp(line_read, "all")) {
        // stop executing the list from the config file
        waitState = WAIT_NOTHING;
      }
      return;
    }
  }
  puts("Unknown command, try again...");
}

//----------------------------------------------------------------------------
// PRIVATE
// Create Main Test Component

void Cli::cmtcCallback(const char *arguments)
{
  int hostIndex;
  if(*arguments == 0) hostIndex = 0;
  else {
    hostIndex = getHostIndex(arguments);
    if (hostIndex < 0) return;
  }
  switch (MainController::get_state()) {
  case mctr::MC_LISTENING:
  case mctr::MC_LISTENING_CONFIGURED:
    puts("Waiting for HC to connect...");
    waitMCState(WAIT_HC_CONNECTED);
  case mctr::MC_HC_CONNECTED:
    MainController::configure(mycfg.config_read_buffer);
    waitMCState(WAIT_ACTIVE);
    if (MainController::get_state() != mctr::MC_ACTIVE) {
      puts("Error during initialization. Cannot create MTC.");
      break;
    }
  case mctr::MC_ACTIVE:
    MainController::create_mtc(hostIndex);
    waitMCState(WAIT_MTC_CREATED);
    break;
  default:
    puts("MTC already exists.");
  }
  fflush(stdout);
}

//----------------------------------------------------------------------------
// PRIVATE
// Start Main Test Component and execute the
// a) control part or testcase (or *) given in arguments
// b) EXECUTE part of supplied configuration file -- if arguments==NULL

void Cli::smtcCallback(const char *arguments)
{
  switch (MainController::get_state()) {
  case mctr::MC_LISTENING:
  case mctr::MC_LISTENING_CONFIGURED:
  case mctr::MC_HC_CONNECTED:
  case mctr::MC_ACTIVE:
    puts("MTC does not exist.");
    break;
  case mctr::MC_READY:
    if (*arguments == 0) {
      // Execute configuration file's execute section
      if (mycfg.execute_list_len > 0) {
        puts("Executing all items of [EXECUTE] section.");
        waitState = WAIT_EXECUTE_LIST;
        executeListIndex = 0;
        executeFromList(0);
      } else {
        puts("No [EXECUTE] section was given in the configuration "
          "file.");
      }
    } else { // Check the arguments
      size_t doti = 0, alen = strlen(arguments), state = 0;
      for (size_t r = 0; r < alen; r++) {
        switch (arguments[r]) {
        case '.':
          ++state;
          doti = r;
          break;
        case ' ':
        case '\t':
          state = 3;
        }
      }

      if(state > 1) { // incorrect argument
        puts("Incorrect argument format.");
        helpCallback(SMTC_TEXT);
      } else {
        if(state == 0) { // only modulename is given in arguments
          MainController::execute_control(arguments);
        } else { // modulename.something in arguments
          expstring_t arg_copy = mcopystr(arguments);
          arg_copy[doti++] = '\0';
          if (!strcmp(arg_copy + doti, "*"))
            MainController::execute_testcase(arg_copy, NULL);
          else if (!strcmp(arg_copy + doti, "control"))
            MainController::execute_control(arg_copy);
          else MainController::execute_testcase(arg_copy,
            arg_copy + doti);
          Free(arg_copy);
        }
      }
    }
    break;
  default:
    puts("MTC is busy.");
  }
  fflush(stdout);
}

//----------------------------------------------------------------------------
// PRIVATE
// Stops test execution

void Cli::stopCallback(const char *arguments)
{
  if (*arguments == 0) {
    switch (MainController::get_state()) {
    case mctr::MC_TERMINATING_TESTCASE:
    case mctr::MC_EXECUTING_CONTROL:
    case mctr::MC_EXECUTING_TESTCASE:
    case mctr::MC_PAUSED:
      MainController::stop_execution();
      if (waitState == WAIT_EXECUTE_LIST) waitState = WAIT_NOTHING;
      break;
    default:
      puts("Tests are not running.");
    }
  } else helpCallback(STOP_TEXT);
}

//----------------------------------------------------------------------------
// PRIVATE
// Sets whether to interrupt test execution after testcase

void Cli::pauseCallback(const char *arguments)
{
  if (arguments[0] != '\0') {
    if (!strcmp(arguments, "on")) {
      if (!MainController::get_stop_after_testcase()) {
        MainController::stop_after_testcase(TRUE);
        puts("Pause function is enabled. "
          "Execution will stop at the end of each testcase.");
      } else puts("Pause function is already enabled.");
    } else if (!strcmp(arguments, "off")) {
      if (MainController::get_stop_after_testcase()) {
        MainController::stop_after_testcase(FALSE);
        puts("Pause function is disabled. "
          "Execution will continue at the end of each testcase.");
      } else puts("Pause function is already disabled.");
    } else helpCallback(PAUSE_TEXT);
  } else printf("Pause function is %s.\n",
    MainController::get_stop_after_testcase() ? "enabled" : "disabled");
}

//----------------------------------------------------------------------------
// PRIVATE
// Resumes interrupted test execution

void Cli::continueCallback(const char *arguments)
{
  if (*arguments == 0) {
    switch (MainController::get_state()) {
    case mctr::MC_TERMINATING_TESTCASE:
    case mctr::MC_EXECUTING_CONTROL:
    case mctr::MC_EXECUTING_TESTCASE:
      puts("Execution is not paused.");
      break;
    case mctr::MC_PAUSED:
      MainController::continue_testcase();
      break;
    default:
      puts("Tests are not running.");
    }
  } else helpCallback(CONTINUE_TEXT);
}

//----------------------------------------------------------------------------
// PRIVATE
// Exit Main Test Component

void Cli::emtcCallback(const char *arguments)
{
  if(*arguments == 0) {
    switch (MainController::get_state()) {
    case mctr::MC_LISTENING:
    case mctr::MC_LISTENING_CONFIGURED:
    case mctr::MC_HC_CONNECTED:
    case mctr::MC_ACTIVE:
      puts("MTC does not exist.");
      break;
    case mctr::MC_READY:
      MainController::exit_mtc();
      waitMCState(WAIT_MTC_TERMINATED);
      break;
    default:
      puts("MTC cannot be terminated.");
    }
  } else {
    helpCallback(EMTC_TEXT);
  }
}

//----------------------------------------------------------------------------
// PRIVATE
// Controls console logging

void Cli::logCallback(const char *arguments)
{
  if (arguments[0] != '\0') {
    if (!strcmp(arguments, "on")) {
      loggingEnabled = TRUE;
      puts("Console logging is enabled.");
    } else if (!strcmp(arguments, "off")) {
      loggingEnabled = FALSE;
      puts("Console logging is disabled.");
    } else helpCallback(LOG_TEXT);
  } else printf("Console logging is %s.\n",
    loggingEnabled ? "enabled" : "disabled");
}

//----------------------------------------------------------------------------
// PRIVATE
// Print connection information

void Cli::infoCallback(const char *arguments)
{
  if (*arguments == 0) printInfo();
  else helpCallback(INFO_TEXT);
}

//----------------------------------------------------------------------------
// PRIVATE
// Reconfigure MC and HCs

void Cli::reconfCallback(const char *arguments)
{
  if (!MainController::start_reconfiguring()) {
    return;
  }
  if (*arguments != 0) {
    Free(cfg_file_name);
    cfg_file_name = mcopystr(arguments);
  }
  
  printf("Using configuration file: %s\n", cfg_file_name);
  if (process_config_read_file(cfg_file_name, &mycfg)) {
    puts("Error was found in the configuration file. Exiting.");
    cleanUp();
    puts("exit");
    exitCallback("");
  }
  else {
    MainController::set_kill_timer(mycfg.kill_timer);

    for (int i = 0; i < mycfg.group_list_len; ++i) {
      MainController::add_host(mycfg.group_list[i].group_name,
        mycfg.group_list[i].host_name);
    }

    for (int i = 0; i < mycfg.component_list_len; ++i) {
      MainController::assign_component(mycfg.component_list[i].host_or_group,
        mycfg.component_list[i].component);
    }
    
    if (MainController::get_state() == mctr::MC_RECONFIGURING) {
      MainController::configure(mycfg.config_read_buffer);
    }
  }
}

//----------------------------------------------------------------------------
// PRIVATE
// Print general help or help on command usage to console

void Cli::helpCallback(const char *arguments)
{
  if (*arguments == 0) {
    puts("Help is available for the following commands:");
    for (const Command *command = command_list;
      command->name != NULL; command++) {
      printf("%s ", command->name);
    }
    for (const DebugCommand *command = debug_command_list;
         command->name != NULL; command++) {
      printf("%s ", command->name);
    }
    putchar('\n');
  } else {
    for (const Command *command = command_list;
      command->name != NULL; command++) {
      if (!strncmp(arguments, command->name,
        strlen(command->name))) {
        printf("%s usage: %s\n%s\n", command->name,
          command->synopsis,
          command->description);
        return;
      }
    }
    for (const DebugCommand *command = debug_command_list;
         command->name != NULL; command++) {
      if (!strncmp(arguments, command->name,
        strlen(command->name))) {
        printf("%s usage: %s\n%s\n", command->name,
          command->synopsis,
          command->description);
        return;
      }
    }
    printf("No help for %s.\n", arguments);
  }
}

//----------------------------------------------------------------------------
// PRIVATE
// Pass command to shell

void Cli::shellCallback(const char *arguments)
{
  if(system(arguments) == -1) {
    perror("Error executing command in subshell.");
  }
}

//----------------------------------------------------------------------------
// PRIVATE
// User initiated MC termination, exits the program.
// Save history into file and terminate CLI session.

void Cli::exitCallback(const char *arguments)
{
  if (*arguments == 0) {
    switch (MainController::get_state()) {
    case mctr::MC_READY:
    case mctr::MC_RECONFIGURING:
      MainController::exit_mtc();
      waitMCState(WAIT_MTC_TERMINATED);
    case mctr::MC_LISTENING:
    case mctr::MC_LISTENING_CONFIGURED:
    case mctr::MC_HC_CONNECTED:
    case mctr::MC_ACTIVE:
      MainController::shutdown_session();
      waitMCState(WAIT_SHUTDOWN_COMPLETE);
      exitFlag = TRUE;
      break;
    default:
      puts("Cannot exit until execution is finished.");
    }
  } else {
    helpCallback(EXIT_TEXT);
  }
}

void Cli::executeBatchFile(const char* filename)
{
  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Failed to open file '%s' for reading.\n", filename);
    return;
  }
  else {
    printf("Executing batch file '%s'.\n", filename);
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
      processCommand(line);
    }
  }
  if (!feof(fp)) {
    printf("Error occurred while reading batch file '%s' (error code: %d).\n",
      filename, ferror(fp));
  }
  fclose(fp);
}

//----------------------------------------------------------------------------
// PRIVATE
// Command completion function implementation for readline() library.
// Heavily uses the ``static Command command_list[]'' array!

char *Cli::completeCommand(const char *prefix, int state)
{
  static int command_index;
  static int debug_command_index;
  static size_t prefix_len;
  const char *command_name;

  if(shell_mode)
    return rl_filename_completion_function(prefix, state);

  if(state == 0) {
    command_index = 0;
    debug_command_index = 0;
    prefix_len = strlen(prefix);
  }

  while((command_name = command_list[command_index].name)) {
    ++command_index;
    if(strncmp(prefix, command_name, prefix_len) == 0) {
      // Must allocate buffer for returned string (readline frees it)
      return strdup(command_name);
    }
  }
  
  while ((command_name = debug_command_list[debug_command_index].name)) {
    ++debug_command_index;
    if (strncmp(prefix, command_name, prefix_len) == 0) {
      // Must allocate buffer for returned string (readline frees it)
      return strdup(command_name);
    }
  }
  // No match found
  return NULL;
}

//----------------------------------------------------------------------------
// PRIVATE
// Character input function implementation for readline() library.

int Cli::getcWithShellDetection(FILE *fp)
{
  int input_char = getc(fp);
  if(input_char == SHELL_ESCAPE)
    shell_mode = TRUE;
  return input_char;
}


//----------------------------------------------------------------------------
// PRIVATE

void Cli::stripLWS(char *input_text)
{
  if(input_text == NULL) {
    puts("stripLWS() called with NULL.");
    exit(EXIT_FAILURE); // This shall never happen
  }
  size_t head_index, tail_index, input_len = strlen(input_text);
  if(input_len < 1) return;
  for(head_index = 0; isspace(input_text[head_index]); head_index++) ;
  for(tail_index = input_len - 1; tail_index >= head_index &&
  isspace(input_text[tail_index]); tail_index--) ;
  size_t output_len = tail_index - head_index + 1;
  memmove(input_text, input_text + head_index, output_len);
  memset(input_text + output_len, 0, input_len - output_len);
}

const char *Cli::verdict2str(verdicttype verdict)
{
  switch (verdict) {
  case NONE:
    return "none";
  case PASS:
    return "pass";
  case INCONC:
    return "inconc";
  case FAIL:
    return "fail";
  case ERROR:
    return "error";
  default:
    return "unknown";
  }
}

//----------------------------------------------------------------------------
// PRIVATE

void Cli::printInfo()
{
  puts("MC information:");
  printf(" MC state: %s\n",
    MainController::get_mc_state_name(MainController::get_state()));
  puts(" host information:");
  int host_index = 0;
  for ( ; ; host_index++) {
    mctr::host_struct *host = MainController::get_host_data(host_index);
    if (host != NULL) {
      printf("  - %s", host->hostname);
      const char *ip_addr = host->ip_addr->get_addr_str();
      // if the hostname differs from the IP address
      // (i.e. the host has a DNS entry)
      if (strcmp(ip_addr, host->hostname)) printf(" [%s]", ip_addr);
      // if the local hostname differs from the prefix of the DNS name
      if (strncmp(host->hostname, host->hostname_local,
        strlen(host->hostname_local)))
        printf(" (%s)", host->hostname_local);
      puts(":");
      printf("     operating system: %s %s on %s\n", host->system_name,
        host->system_release, host->machine_type);
      printf("     HC state: %s\n",
        MainController::get_hc_state_name(host->hc_state));

      puts("     test component information:");
      // make a copy of the array containing component references
      int n_components = host->n_components;
      component *components = (component*)Malloc(n_components *
        sizeof(component));
      memcpy(components, host->components, n_components *
        sizeof(component));
      // the host structure has to be released in order to get access
      // to the component structures
      MainController::release_data();
      for (int component_index = 0; component_index < n_components;
        component_index++) {
        mctr::component_struct *comp = MainController::
          get_component_data(components[component_index]);
        // if the component has a name
        if (comp->comp_name != NULL)
          printf("      - name: %s, component reference: %d\n",
            comp->comp_name, comp->comp_ref);
        else printf("      - component reference: %d\n",
          comp->comp_ref);
        if (comp->comp_type.definition_name != NULL) {
          printf("         component type: ");
          if (comp->comp_type.module_name != NULL)
            printf("%s.", comp->comp_type.module_name);
          printf("%s\n", comp->comp_type.definition_name);
        }
        printf("         state: %s\n",
          MainController::get_tc_state_name(comp->tc_state));
        if (comp->tc_fn_name.definition_name != NULL) {
          printf("         executed %s: ",
            comp->comp_ref == MTC_COMPREF ?
              "test case" : "function");
          if (comp->tc_fn_name.module_name != NULL)
            printf("%s.", comp->tc_fn_name.module_name);
          printf("%s\n", comp->tc_fn_name.definition_name);
        }
        if (comp->tc_state == mctr::TC_EXITING ||
          comp->tc_state == mctr::TC_EXITED)
          printf("         local verdict: %s\n",
            verdict2str(comp->local_verdict));
        MainController::release_data();
      }
      if (n_components == 0) puts("      no components on this host");
      Free(components);
    } else {
      MainController::release_data();
      break;
    }
  }
  if (host_index == 0) puts("  no HCs are connected");
  printf(" pause function: %s\n", MainController::get_stop_after_testcase() ?
    "enabled" : "disabled");
  printf(" console logging: %s\n", loggingEnabled ?
    "enabled" : "disabled");
  fflush(stdout);
}

//----------------------------------------------------------------------------
// PRIVATE

void Cli::lock()
{
  if (pthread_mutex_lock(&mutex)) {
    perror("Cli::lock: pthread_mutex_lock failed.");
    exit(EXIT_FAILURE);
  }
}

void Cli::unlock()
{
  if (pthread_mutex_unlock(&mutex)) {
    perror("Cli::unlock: pthread_mutex_unlock failed.");
    exit(EXIT_FAILURE);
  }
}

void Cli::wait()
{
  if (pthread_cond_wait(&cond, &mutex)) {
    perror("Cli::wait: pthread_cond_wait failed.");
    exit(EXIT_FAILURE);
  }
}

void Cli::signal()
{
  if (pthread_cond_signal(&cond)) {
    perror("Cli::signal: pthread_cond_signal failed.");
    exit(EXIT_FAILURE);
  }
}

void Cli::waitMCState(waitStateEnum newWaitState)
{
  lock();
  if (newWaitState != WAIT_NOTHING) {
    if (conditionHolds(newWaitState)) {
      waitState = WAIT_NOTHING;
    } else {
      waitState = newWaitState;
      wait();
    }
  } else {
    fputs("Cli::waitMCState: invalid argument", stderr);
    exit(EXIT_FAILURE);
  }
  unlock();
}

boolean Cli::conditionHolds(waitStateEnum askedState)
{
  switch (askedState) {
  case WAIT_HC_CONNECTED:
    if (MainController::get_state() == mctr::MC_HC_CONNECTED) {
      if (mycfg.num_hcs > 0) {
        return MainController::get_nof_hosts() >= mycfg.num_hcs;
      }
      else return TRUE;
    } else return FALSE;
  case WAIT_ACTIVE:
    switch (MainController::get_state()) {
    case mctr::MC_ACTIVE: // normal case
    case mctr::MC_HC_CONNECTED: // error happened with config file
    case mctr::MC_LISTENING: // even more strange situations
      return TRUE;
    default:
      return FALSE;
    }
    case WAIT_MTC_CREATED:
    case WAIT_MTC_READY:
      switch (MainController::get_state()) {
      case mctr::MC_READY: // normal case
      case mctr::MC_ACTIVE: // MTC crashed unexpectedly
      case mctr::MC_LISTENING_CONFIGURED: // MTC and all HCs are crashed
        // at the same time
      case mctr::MC_HC_CONNECTED: // even more strange situations
        return TRUE;
      default:
        return FALSE;
      }
      case WAIT_MTC_TERMINATED:
        return MainController::get_state() == mctr::MC_ACTIVE;
      case WAIT_SHUTDOWN_COMPLETE:
        return MainController::get_state() == mctr::MC_INACTIVE;
      case WAIT_EXECUTE_LIST:
        if (MainController::get_state() == mctr::MC_READY) {
          if (++executeListIndex < mycfg.execute_list_len) {
            unlock();
            executeFromList(executeListIndex);
            lock();
          } else {
            puts("Execution of [EXECUTE] section finished.");
            waitState = WAIT_NOTHING;
          }
        }
        return FALSE;
      default:
        return FALSE;
  }
}

int Cli::getHostIndex(const char* hostname)
{
  int hostname_len = strlen(hostname);
  int index, found = -1;
  for (index = 0; ; index++) {
    const mctr::host_struct *host =
      MainController::get_host_data(index);
    if (host != NULL) {
      if (!strncmp(host->hostname, hostname, hostname_len) ||
        !strncmp(host->hostname_local, hostname, hostname_len)) {
        MainController::release_data();
        if (found == -1) found = index;
        else {
          printf("Hostname %s is ambiguous.\n", hostname);
          found = -1;
          break;
        }
      } else MainController::release_data();
    } else {
      MainController::release_data();
      if (found == -1) printf("No such host: %s.\n", hostname);
      break;
    }
  }
  return found;
}

void Cli::executeFromList(int index)
{
  if (index >= mycfg.execute_list_len) {
    fputs("Cli::executeFromList: invalid argument", stderr);
    exit(EXIT_FAILURE);
  }
  if (mycfg.execute_list[index].testcase_name == NULL) {
    MainController::execute_control(mycfg.execute_list[index].module_name);
  } else if (!strcmp(mycfg.execute_list[index].testcase_name, "*")) {
    MainController::execute_testcase(mycfg.execute_list[index].module_name,
      NULL);
  } else {
    MainController::execute_testcase(mycfg.execute_list[index].module_name,
      mycfg.execute_list[index].testcase_name);
  }
}

//----------------------------------------------------------------------------
// Local Variables:
// mode: C++
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:

extern boolean error_flag; // in config_read.y
extern std::string get_cfg_read_current_file(); // in config_read.y
extern int config_read_lineno; // in config_read.l
extern char *config_read_text; // in config_read.l

void config_read_warning(const char *warning_str, ...)
{
    fprintf(stderr, "Warning: in configuration file `%s', line %d: ",
            get_cfg_read_current_file().c_str(), config_read_lineno);
    va_list pvar;
    va_start(pvar, warning_str);
    vfprintf(stderr, warning_str, pvar);
    va_end(pvar);
    putc('\n', stderr);
}

void config_read_error(const char *error_str, ...)
{
    fprintf(stderr, "Parse error in configuration file `%s': in line %d, "
            "at or before token `%s': ",
            get_cfg_read_current_file().c_str(), config_read_lineno, config_read_text);
    va_list pvar;
    va_start(pvar, error_str);
    vfprintf(stderr, error_str, pvar);
    va_end(pvar);
    putc('\n', stderr);
    error_flag = TRUE;
}

void config_preproc_error(const char *error_str, ...)
{
    fprintf(stderr, "Parse error while pre-processing configuration file "
            "`%s': in line %d: ",
            get_cfg_preproc_current_file().c_str(),
            config_preproc_yylineno);
    va_list pvar;
    va_start(pvar, error_str);
    vfprintf(stderr, error_str, pvar);
    va_end(pvar);
    putc('\n', stderr);
    error_flag = TRUE;
}

// called by functions in path.c
void path_error(const char *fmt, ...)
{
  fprintf(stderr, "File error: ");
  va_list parameters;
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  putc('\n', stderr);
}
