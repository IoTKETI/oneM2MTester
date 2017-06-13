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

#ifndef DEBUGGER_UI_HH
#define DEBUGGER_UI_HH

/** Command line interface class for the TTCN-3 debugger in single mode
  * Mimics the functionality of the Main Controller CLI in most cases
  * Uses the editline package for reading commands (which provides command
  * completion and command history tracking) */
class TTCN_Debugger_UI {
  
  /** structure for storing a command */
  struct command_t {
    /** command name */
    const char *name;
    /** debugger command ID */
    int commandID;
    /** command usage text */
    const char *synopsis;
    /** command description text */
    const char *description;
  };
  
  /** list of commands */
  static const command_t debug_command_list[];

#ifdef ADVANCED_DEBUGGER_UI
  /** name of the file, where the command history is stored */
  static char* ttcn3_history_filename;
#endif
  
  /** processes the command in the specified input line
    * if it's a valid command, then it is added to the command history and 
    * passed to the debugger 
    * if it's not valid, an error message is displayed */
  static void process_command(const char* p_line_read);
  
  /** displays help for the specified command, or lists available commands */
  static void help(const char* p_argument);
  
public:
  
  /** initializes the UI */
  static void init();
  
  /** cleans up the UI's resources */
  static void clean_up();
  
  /** reads commands from the standard input and passes them on for processing,
    * until test execution is no longer halted */
  static void read_loop();
  
  /** executes the commands in the specified batch file
    * each line is treated as a separate command */
  static void execute_batch_file(const char* p_file_name);
  
  /** prints the specified text to the standard output */
  static void print(const char* p_str);
  
#ifdef ADVANCED_DEBUGGER_UI
  /** command completion function for editline */
  static char* complete_command(const char* p_prefix, int p_state);
#endif
};

#endif /* DEBUGGER_UI_HH */

