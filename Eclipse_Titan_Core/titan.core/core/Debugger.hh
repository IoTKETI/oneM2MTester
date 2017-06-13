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

#ifndef DEBUGGER_HH
#define DEBUGGER_HH

#include "Vector.hh"
#include "Basetype.hh"
#include "Charstring.hh"
#ifdef TITAN_RUNTIME_2
#include "RT2/PreGenRecordOf.hh"
#else
#include "RT1/PreGenRecordOf.hh"
#endif
#include <regex.h>

/** alias for record of charstring */
typedef PreGenRecordOf::PREGEN__RECORD__OF__CHARSTRING charstring_list;

// debugger forward declarations
class TTCN3_Debug_Scope;
class TTCN3_Debug_Function;

// other forward declarations
class Module_Param;


//////////////////////////////////////////////////////
////////////////// TTCN3_Debugger ////////////////////
//////////////////////////////////////////////////////

/** main debugger class
  * 
  * instantiated once per process at the beginning of the current process,
  * destroyed at the end of the current process */
class TTCN3_Debugger {
public:
  
  struct variable_t;
  
  typedef CHARSTRING (*print_function_t)(const variable_t&);
  typedef boolean (*set_function_t)(variable_t&, Module_Param&);
  
  /** type for keeping track of a variable */
  struct variable_t {
    /** pointer to the variable object, not owned */
    union {
      /** container for non-constant variable objects */
      void* value;
      /** container for constant variable objects */
      const void* cvalue;
    };
    /** variable name (used for looking up variables), not owned */
    const char* name;
    /** name of the variable's type, not owned */
    const char* type_name;
    /** name of the module the variable was declared in (only set for global
      * variables, otherwise NULL), not owned */
    const char* module;
    /** variable printing function (using the variable object's log() function) */
    print_function_t print_function;
    /** variable setting (overwriting) function (using the variable object's
      * set_param() function) - set to NULL for constant variables */
    set_function_t set_function;
  };
  
  /** this type pairs a debug scope with a name, used for storing global and
    * component scopes */
  struct named_scope_t {
    /** scope name (module name for global scopes, or component type name for
      * component scopes), not owned */
    const char* name;
    /** scope pointer, owned */
    TTCN3_Debug_Scope* scope;
  };
  
  /** type for storing a function call in the call stack */
  struct function_call_t {
    /** pointer to the debug function object */
    TTCN3_Debug_Function* function;
    /** TTCN-3 line number, where the function was called from */
    int caller_line;
  };
  
  /** type for storing breakpoints */
  struct breakpoint_t {
    /** module name, owned */
    char* module;
    /** line number (if it's a line breakpoint, otherwise 0) */
    int line;
    /** function name (if it's a function breakpoint, otherwise NULL), owned */
    char* function;
    /** batch file to be executed when the breakpoint is reached (optional), owned */
    char* batch_file;
  };
  
  /** type for storing data related to a breakpoint entry */
  struct breakpoint_entry_t {
    /** module name, not owned */
    const char* module;
    /** line number */
    int line;
    /** size of the call stack */
    size_t stack_size;
  };
  
  /** special breakpoint types, passed to breakpoint_entry() as the line parameter,
    * so these need to be 0 or negative, to never conflict with any line number */
  enum special_breakpoint_t {
    /** triggered when the local verdict is set to ERROR (by dynamic test case errors) */
    SBP_ERROR_VERDICT = 0,
    /** triggered when the local verdict is set to FAIL (by the user) */
    SBP_FAIL_VERDICT = -1
  };
  
  /** type for storing the settings of automatic breakpoints */
  struct automatic_breakpoint_behavior_t {
    /** indicates whether the breakpoint should be triggered by the associated event */
    boolean trigger;
    /** batch file to be executed if the breakpoint is triggered (optional), owned */
    char* batch_file;
  };
  
  /** possible function call data handling configurations */
  enum function_call_data_config_t {
    /** function call data is printed directly to file and not stored by the
      * debugger */
    CALLS_TO_FILE,
    /** function call data is stored in a ring buffer of a set size 
      * (i.e. when the buffer is full, adding a new function call automatically
      * deletes the oldest function call)
      * the buffer size can be zero, in which case no calls are stored */
    CALLS_RING_BUFFER,
    /** function call data is stored in a buffer of variable length (i.e. when
      * the buffer is full, its size is increased) */
    CALLS_STORE_ALL
  };
  
  /** structure containing the function call data and information related to
    * their handling */
  struct function_call_data_t {
    /** current function call data configuration (this also indicates which 
      * field of the following union is selected) */
    function_call_data_config_t cfg;
    union {
      /** information related to the file, that function calls are printed to
        * (in case of CALLS_TO_FILE) */
      struct {
        /** name of the target file (may contain metacharacters), owned */
        char* name;
        /** the target file's handler, owned */
        FILE* ptr;
      } file;
      /** information related to the storing of function calls 
        * (in case of CALLS_RING_BUFFER or CALLS_STORE_ALL) */
      struct {
        /** size of the buffer used for storing function calls (this value is 
          * fixed in case of CALLS_RING_BUFFER and dynamic in case of CALLS_STORE_ALL) */
        int size;
        /** stores the index of the first function call in the buffer
          * (is always 0 in case of CALLS_STORE_ALL) */
        int start;
        /** stores the index of the last function call in the buffer 
          * (its value is -1 if no function calls are currently stored) */
        int end;
        /** buffer containing the function call data, owned */
        char** ptr;
      } buffer;
    };
  };
  
  /** stepping type */
  enum stepping_t {
    NOT_STEPPING,
    STEP_OVER,
    STEP_INTO,
    STEP_OUT
  };
  
private:
  
  /** indicates whether the debugger has been activated, meaning that the debugger's
    * command line option (-n) was used during the build (switched automatically
    * by generated code) */
  boolean enabled;
  
  /** the debugger's on/off switch (switched by the user) */
  boolean active;
  
  /** true if test execution has been halted (by a breakpoint or by the user) */
  boolean halted;
  
  /** the debugger's output file handler (NULL if the debugger's output is only
    * sent to the console); owned */
  FILE* output_file;
  
  /** name of the debugger's output file (NULL if the debugger's output is only
    * sent to the console); may contain metacharacters; owned */
  char* output_file_name;
  
  /** indicates whether the debugger's output should be sent to the console */
  boolean send_to_console;
  
  /** list of all global and component variables, elements are owned */
  Vector<variable_t*> variables;
  
  /** list of global scopes */
  Vector<named_scope_t> global_scopes;
  
  /** list of component scopes */
  Vector<named_scope_t> component_scopes;
  
  /** pointers to debug function objects and the lines they were called from
    * (resembling a call stack), the current function is always the last element
    * in the array (the top element in the stack) */
  Vector<function_call_t> call_stack;
  
  /** list of breakpoints */
  Vector<breakpoint_t> breakpoints;
  
  /** structure containing function call data */
  function_call_data_t function_calls;
  
  /** stores the last line hit by breakpoint_entry() */
  breakpoint_entry_t last_breakpoint_entry;
  
  /** current stack level (reset when test execution is halted or resumed) */
  int stack_level;
  
  /** automatic breakpoint triggered by setting the local verdict to FAIL */
  automatic_breakpoint_behavior_t fail_behavior;
  
  /** automatic breakpoint triggered by setting the local verdict to ERROR */
  automatic_breakpoint_behavior_t error_behavior;
  
  /** batch file executed automatically when test execution is halted
    * NULL if switched off (owned)
    * is overridden by breakpoint-specific batch files */
  char* global_batch_file;
  
  /** result of the currently executing command, owned */
  char* command_result;
  
  /** result of the last D_LIST_VARIABLES command, owned */
  char* last_variable_list;
  
  /** stores which stepping option was requested by the user (if any) */
  stepping_t stepping_type;
  
  /** stores the size of the call stack when the last stepping operation was
    * initiated */
  size_t stepping_stack_size;
  
  /** temporary breakpoint set by the 'run to cursor' operation */
  breakpoint_t temporary_breakpoint;
  
  /** true if an 'exit all' command was issued
    * switched to false when test execution is restarted */
  boolean exiting;
  
  /** test execution is automatically halted at start if set to true */
  boolean halt_at_start;
  
  /** batch file executed automatically at the start of test execution
    * if not set to NULL (not owned) */
  const char* initial_batch_file;
  
  //////////////////////////////////////////////////////
  ///////////////// internal functions /////////////////
  //////////////////////////////////////////////////////
  
  /** switches the debugger on or off
    * handles the D_SWITCH command */
  void switch_state(const char* p_state_str);
  
  /** adds a new breakpoint at the specified location (line or function) with the
    * specified batch file (if not NULL), or changes the batch file of an existing
    * breakpoint
    * handles the D_SET_BREAKPOINT command */
  void set_breakpoint(const char* p_module, const char* p_location,
    const char* batch_file);
  
  /** removes the breakpoint from the specified location, if it exists
    * can also be used to remove all breakpoints from the specified module or
    * all breakpoints in all modules
    * handles the D_REMOVE_BREAKPOINT command */
  void remove_breakpoint(const char* p_module, const char* p_location);
  
  /** switches an automatic breakpoint related to the specified event on or off
    * and/or sets the batch file to run when the breakpoint is triggered
    * @param p_event_str string representation of the event that triggers the
    * breakpoint (either "error" or "fail")
    * @param p_state_str "on" or "off", indicates the new state of the breakpoint
    * @param p_batch_file name of the batch file to execute (NULL means don't
    * execute anything)
    * handles the D_SET_AUTOMATIC_BREAKPOINT command */
  void set_automatic_breakpoint(const char* p_event_str, const char* p_state_str,
    const char* p_batch_file);
  
  /** prints the debugger's settings 
    * handles the D_PRINT_SETTINGS command */
  void print_settings();
  
  /** prints the current call stack
    * handles the D_PRINT_CALL_STACK command */
  void print_call_stack();
  
  /** sets the current stack level to the specified value
    * handles the D_SET_STACK_LEVEL command */
  void set_stack_level(int new_level);
  
  /** finds the variable with the specified name, and prints its value or an
    * error message
    * the variable's value is printed using its log() function
    * handles (one parameter of) the D_PRINT_VARIABLE command */
  void print_variable(const char* p_var_name);
  
  /** finds the variable with the specified name, and overwrites its value or
    * displays an error message
    * the new value is received in a string array; this is concatenated into one
    * string (with one space separating each string element); then it is passed 
    * to the module parameter parser, which creates a Module_Param object from it
    * (if its syntax is correct)
    * the variable's value is overwritten by passing the Module_Param object to
    * its set_param() function
    * handles the D_OVERWRITE_VARIABLE command */
  void overwrite_variable(const char* p_var_name, int p_value_element_count,
    char** p_value_elements);
  
  /** frees all resources related to the handling of function call data */
  void clean_up_function_calls();
  
  /** changes the method of handling function call data
    * handles the D_FUNCTION_CALL_CONFIG command */
  void configure_function_calls(const char* p_config, const char* p_file_name);
  
  /** prints the last stored function calls
    * handles the D_PRINT_FUNCTION_CALLS command
    * @param p_amount amount of function calls to print or "all" */
  void print_function_calls(const char* p_amount);
  
  /** sets the debugger's output to the console and/or a text file
    * handles the D_SET_OUTPUT command
    * @param p_output_type "console", "file" or "both"
    * @param p_file_name output file name or NULL */
  void set_output(const char* p_output_type, const char* p_file_name);
  
  /** switches the global batch file on or off
    * handles the D_SET_GLOBAL_BATCH_FILE command */
  void set_global_batch_file(const char* p_state_str, const char* p_file_name);
  
  /** resumes execution until the next breakpoint entry (in the stack levels
    * specified by the stepping type arguments) is reached (or until test execution
    * is halted for any other reason)
    * handles the D_STEP_OVER, D_STEP_INTO and D_STEP_OUT commands */
  void step(stepping_t p_stepping_type);
  
  /** resumes execution until the specified location (line or function) is reached 
    * (or until test execution is halted for any other reason)
    * handles the D_RUN_TO_CURSOR command */
  void run_to_cursor(const char* p_module, const char* p_location);
  
  /** halts test execution, processing only debug commands
    * @param p_batch_file batch file executed after the halt (if not NULL)
    * @param p_run_global_batch indicates whether the global batch file should
    * be executed after the halt (only if p_batch_file is NULL)
    * handles the D_HALT command */
  void halt(const char* p_batch_file, boolean p_run_global_batch);
  
  /** resumes the halted test execution
    * handles the D_CONTINUE command */
  void resume();
  
  /** exits the current test or the execution of all tests
    * handles the D_EXIT command */
  void exit_(const char* p_what);
  
  /** returns the index of the specified breakpoint, if found,
    * otherwise returns breakpoints.size() */
  size_t find_breakpoint(const char* p_module, int p_line,
    const char* p_function) const;
  
  /** returns the specified variable, if found, otherwise returns NULL */
  TTCN3_Debugger::variable_t* find_variable(const void* p_value) const;
  
  /** handles metacharacters in the specified file name skeleton
    * @return final file name (must be freed by caller) */
  static char* finalize_file_name(const char* p_file_name_skeleton);
  
  /** initialization function, called when the first function is added to the
    * call stack */
  void test_execution_started();
  
  /** clean up function, called when the last function is removed from the
    * call stack */
  void test_execution_finished();

public:
  /** constructor - called once per process (at the beginning) */
  TTCN3_Debugger();
  
  /** destructor - called once per process (at the end) */
  ~TTCN3_Debugger();
  
  //////////////////////////////////////////////////////
  ////// methods called from TITAN generated code //////
  //////////////////////////////////////////////////////
  
  /** activates the debugger */
  void activate() { enabled = TRUE; }
  
  /** creates, stores and returns a new global scope for the specified module
    * (this scope contains all global variables visible in the module) */
  TTCN3_Debug_Scope* add_global_scope(const char* p_module);
  
  /** creates, stores and returns a new component scope for the specified component
    * type (this scope contains all variables declared in the component type) */
  TTCN3_Debug_Scope* add_component_scope(const char* p_component);
  
  /** stores the string representation of the current function's return value
    * (only if the debugger is switched on) */
  void set_return_value(const CHARSTRING& p_value);
  
  /** activates a breakpoint if the specified line and the current function's module
    * match any of the breakpoints stored in the debugger 
    * the special parameter values (SBP_ERROR_VERDICT and SBP_FAIL_VERDICT) only
    * trigger a breakpoint if their respective behaviors have been set to do so
    * (does nothing if the debugger is switched off) */
  void breakpoint_entry(int p_line);
  
  /** variable printing function for base types */
  static CHARSTRING print_base_var(const variable_t& p_var);
  
  /** variable setting function for base types */
  static boolean set_base_var(variable_t& p_var, Module_Param& p_new_value);
  
  /** variable printing function for value arrays */
  template <typename T_type, unsigned int array_size, int index_offset>
  static CHARSTRING print_value_array(const variable_t& p_var)
  {
    const void* ptr = p_var.set_function != NULL ? p_var.value : p_var.cvalue;
    TTCN_Logger::begin_event_log2str();
    ((VALUE_ARRAY<T_type, array_size, index_offset>*)ptr)->log();
    return TTCN_Logger::end_event_log2str();
  }
  
  /** variable setting function for value arrays */
  template <typename T_type, unsigned int array_size, int index_offset>
  static boolean set_value_array(variable_t& p_var, Module_Param& p_new_value)
  {
    ((VALUE_ARRAY<T_type, array_size, index_offset>*)p_var.value)->set_param(p_new_value);
    return TRUE;
  }
  
  /** variable printing function for template arrays */
  template <typename T_value_type, typename T_template_type,
    unsigned int array_size, int index_offset>
  static CHARSTRING print_template_array(const variable_t& p_var)
  {
    const void* ptr = p_var.set_function != NULL ? p_var.value : p_var.cvalue;
    TTCN_Logger::begin_event_log2str();
    ((TEMPLATE_ARRAY<T_value_type, T_template_type, array_size,
      index_offset>*)ptr)->log();
    return TTCN_Logger::end_event_log2str();
  }
  
  /** variable setting function for template arrays */
  template <typename T_value_type, typename T_template_type,
    unsigned int array_size, int index_offset>
  static boolean set_template_array(variable_t& p_var, Module_Param& p_new_value)
  {
    ((TEMPLATE_ARRAY<T_value_type, T_template_type, array_size,
      index_offset>*)p_var.value)->set_param(p_new_value);
    return TRUE;
  }
  
  /** variable printing function for port arrays */
  template <typename T_type, unsigned int array_size, int index_offset>
  static CHARSTRING print_port_array(const variable_t& p_var)
  {
    const void* ptr = p_var.set_function != NULL ? p_var.value : p_var.cvalue;
    TTCN_Logger::begin_event_log2str();
    ((PORT_ARRAY<T_type, array_size, index_offset>*)ptr)->log();
    return TTCN_Logger::end_event_log2str();
  }
  
  /** variable printing function for timer arrays */
  template <typename T_type, unsigned int array_size, int index_offset>
  static CHARSTRING print_timer_array(const variable_t& p_var)
  {
    const void* ptr = p_var.set_function != NULL ? p_var.value : p_var.cvalue;
    TTCN_Logger::begin_event_log2str();
    ((TIMER_ARRAY<T_type, array_size, index_offset>*)ptr)->log();
    return TTCN_Logger::end_event_log2str();
  }
  
  /** variable printing function for lazy parameters */
  template <typename EXPR_TYPE>
  static CHARSTRING print_lazy_param(const variable_t& p_var)
  {
    const void* ptr = p_var.set_function != NULL ? p_var.value : p_var.cvalue;
    TTCN_Logger::begin_event_log2str();
    ((Lazy_Fuzzy_Expr<EXPR_TYPE>*)ptr)->log();
    return TTCN_Logger::end_event_log2str();
  }
  
  static CHARSTRING print_fuzzy_param(const variable_t&)
  {
    return CHARSTRING("<fuzzy value>");
  }
  
  //////////////////////////////////////////////////////
  ////// methods called by other debugger classes //////
  //////////////////////////////////////////////////////
  
  /** returns true if the debugger is activated (through the compiler switch) */
  boolean is_activated() const { return enabled; }
  
  /** returns true if the debugger is switched on */
  boolean is_on() const { return active; }
  
  /** returns true if test execution has been halted by the debugger */
  boolean is_halted() const { return halted; }
  
  /** prints the formatted string to the console and/or output file
    * (used for printing notifications or error messages) */
  void print(int return_type, const char* fmt, ...) const;
  
  /** adds the formatted string to the currently executed command's result string */
  void add_to_result(const char* fmt, ...);
  
  /** adds the specified function object pointer to the call stack
    * (only if the debugger is switched on) */
  void add_function(TTCN3_Debug_Function* p_function);
  
  /** adds the specified scope object pointer to the current function's scope list
    * (only if the debugger is switched on and the call stack is not empty) */
  void add_scope(TTCN3_Debug_Scope* p_scope);
  
  /** removes the specified function object pointer from the call stack, if it is
    * the function at the top of the stack */
  void remove_function(TTCN3_Debug_Function* p_function);
  
  /** removes the specified scope object pointer from the current function's scope list
    * (only if the call stack is not empty) */
  void remove_scope(TTCN3_Debug_Scope* p_scope);
  
  /** finds or creates, and returns the constant variable entry specified by
    * the parameters
    *
    * if the call stack is empty, an entry for a global or component variable is
    * created and stored in the main debugger object (if it doesn't already exist);
    * if the call stack is not empty (and if the debugger is switched on), the 
    * variable entry for a local variable is created and stored by the current function */
  variable_t* add_variable(const void* p_value, const char* p_name, const char* p_type,
    const char* p_module, print_function_t p_print_function);
  
  /** same as before, but for non-constant variables */
  variable_t* add_variable(void* p_value, const char* p_name, const char* p_type,
    const char* p_module, print_function_t p_print_function,
    set_function_t p_set_function);
  
  /** removes the variable entry for the specified local variable in the current
    * function (only if the call stack is not empty) */
  void remove_variable(const variable_t* p_var);
  
  /** returns the global scope object associated with the specified module */
  const TTCN3_Debug_Scope* get_global_scope(const char* p_module) const;
  
  /** returns the component scope object associated with the specified component type */
  const TTCN3_Debug_Scope* get_component_scope(const char* p_component) const;
  
  /** stores the specified snapshot of a function call, together with a time stamp
    * (the debugger is responsible for freeing the string parameter) */
  void store_function_call(char* p_snapshot);
  
  /** executes a command received from the user interface */
  void execute_command(int p_command, int p_argument_count, char** p_arguments);
  
  /** called when a PTC is forked from the HC process
    * contains supplementary initializations (i.e. opening of file pointers and
    * allocations of buffers) that the HC's debugger does not perform, but are
    * needed by the PTC's debugger */
  void init_PTC_settings();
  
  /** indicates whether an 'exit all' command has been issued 
    * (this causes the execution of tests in the current queue to stop) */
  boolean is_exiting() const { return exiting; }
  
  /** sets the debugger to halt test execution at start (only in single mode) */
  void set_halt_at_start() { halt_at_start = TRUE; }
  
  /** sets the specified batch file to be executed at the start of test execution
    * (only in single mode) */
  void set_initial_batch_file(const char* p_batch_file) {
    initial_batch_file = p_batch_file;
  }
};

/** the main debugger object */
extern TTCN3_Debugger ttcn3_debugger;


//////////////////////////////////////////////////////
//////////////// TTCN3_Debug_Scope ///////////////////
//////////////////////////////////////////////////////

/** debugger scope class
  *
  * instantiated at the beginning of every code block in the TTCN-3 code (except
  * for the code blocks of functions), plus one (global scope) instance is created
  * for every module and one (component scope) for every component type 
  * 
  * the class' main purpose is to track which local variables were created in the
  * current code block or to track which of the main debugger object's variables
  * belong to which global or component scope */
class TTCN3_Debug_Scope {
  
  /** list of pointers to local variable entries from the current function object or
    * global or component variable entries from the main debugger object
    * (the elements are not owned)*/
  Vector<TTCN3_Debugger::variable_t*> variables;
  
public:
  
  /** constructor - lets the current function know of this new scope */
  TTCN3_Debug_Scope();
  
  /** destructor - tells the current function to delete the variable entries listed
    * in this instance */
  ~TTCN3_Debug_Scope();
  
  //////////////////////////////////////////////////////
  ////// methods called from TITAN generated code //////
  //////////////////////////////////////////////////////
  
  /** passes the parameters to the main debugger or current function object to 
    * create and store a (constant) variable entry from them, and tracks this new
    * variable by storing a pointer to it
    * (local variables are only created and stored if the debugger is switched on) */
  void add_variable(const void* p_value, const char* p_name, const char* p_type,
    const char* p_module, TTCN3_Debugger::print_function_t p_print_function);
  
  /** same as before, but for non-constant variables */
  void add_variable(void* p_value, const char* p_name, const char* p_type,
    const char* p_module, TTCN3_Debugger::print_function_t p_print_function,
    TTCN3_Debugger::set_function_t p_set_function);
  
  //////////////////////////////////////////////////////
  ////// methods called by other debugger classes //////
  //////////////////////////////////////////////////////
  
  /** returns true if there is at least one variable in the scope object */
  boolean has_variables() const { return !variables.empty(); }
  
  /** returns the specified variable, if found, otherwise returns NULL 
    * (the name searched for can also be prefixed with the module name in
    * case of global variables) */
  TTCN3_Debugger::variable_t* find_variable(const char* p_name) const;
  
  /** prints the names of variables in this scope that match the specified;
    * names of imported global variables are prefixed with their module's name
    * @param p_posix_regexp the pattern converted into a POSIX regex structure
    * @param p_first true if no variables have been printed yet
    * @param p_module name of the current module, if it's a global scope,
    * otherwise NULL */
  void list_variables(regex_t* p_posix_regexp, bool& p_first,
    const char* p_module) const;
};


//////////////////////////////////////////////////////
/////////////// TTCN3_Debug_Function /////////////////
//////////////////////////////////////////////////////

/** debugger function class
  *
  * instantiated at the beginning of every function, destroyed when function execution ends
  *
  * tracks all variables created during the function's execution (local variables),
  * including the function's parameters, and stores the function's return value */
class TTCN3_Debug_Function {
  
  /** name of the function, not owned */
  const char* function_name;
  
  /** the TTCN-3 keyword(s) used to define the function ("function", "testcase",
    * "altstep", "template" or "external function"), not owned */
  const char* function_type;
  
  /** name of the module this function is defined in, not owned */
  const char* module_name;
  
  /** names of the function's parameters (in the order of their declaration), owned */
  charstring_list* parameter_names;
  
  /** types (directions) of the function's parameters ("in", "inout" or "out"), owned */
  charstring_list* parameter_types;
  
  /** list of local variables tracked by this object, the array elements are owned */
  Vector<TTCN3_Debugger::variable_t*> variables;
  
  /** list of pointers to the scope objects of code blocks in the function,
    * the elements are not owned 
    * (currently not used for anything) */
  Vector<TTCN3_Debug_Scope*> scopes;
  
  /** pointer to the global scope object, not owned
    * (may be NULL, if the module's global scope is empty) */
  const TTCN3_Debug_Scope* global_scope;
  
  /** pointer to the runs-on component's scope object, not owned
    * (may be NULL, if the component's scope is empty or if the function has no runs-on clause) */
  const TTCN3_Debug_Scope* component_scope;
  
  /** the function's return value (unbound if the return value has not been set yet,
    * or if the function doesn't return anything)
    * 
    * since this is only set right before the function ends, it is only accessible
    * from the destructor */
  CHARSTRING return_value;
  
public:
  
  /** constructor - initializes the instance with the specified parameters,
    * retrieves the global scope and component scope from the main debugger object */
  TTCN3_Debug_Function(const char* p_name, const char* p_type, const char* p_module,
    const charstring_list& p_parameter_names, const charstring_list& p_parameter_types, const char* p_component_name);
  
  /** destructor - frees resources and saves the function's ending snapshot
    * (including the values of 'out' and 'inout' parameters and the return value)
    * in the main debugger object (only if the debugger is switched on) */
  ~TTCN3_Debug_Function();
  
  //////////////////////////////////////////////////////
  ////// methods called from TITAN generated code //////
  //////////////////////////////////////////////////////
  
  /** creates, stores and returns the variable entry of the local (constant) variable
    * specified by the parameters (only if the debugger is switched on) */
  TTCN3_Debugger::variable_t* add_variable(const void* p_value, const char* p_name,
    const char* p_type, const char* p_module,
    TTCN3_Debugger::print_function_t p_print_function);
  
  /** same as before, but for non-constant variables */
  TTCN3_Debugger::variable_t* add_variable(void* p_value, const char* p_name,
    const char* p_type, const char* p_module,
    TTCN3_Debugger::print_function_t p_print_function,
    TTCN3_Debugger::set_function_t p_set_function);
  
  /** stores the string representation of the value returned by the function */
  void set_return_value(const CHARSTRING& p_value);
  
  /** saves the function's initial snapshot (including the values of 'in' and
    * 'inout' parameters) in the main debugger object
    * (only if the debugger is switched on) */
  void initial_snapshot() const;
  
  //////////////////////////////////////////////////////
  ////// methods called by other debugger classes //////
  //////////////////////////////////////////////////////
  
  /** adds the specified scope object pointer to the function's scope list */
  void add_scope(TTCN3_Debug_Scope* p_scope);
  
  /** removes the specified scope object pointer from the function's scope list,
    * if it is the last scope in the list */
  void remove_scope(TTCN3_Debug_Scope* p_scope);
  
  /** removes the specified variable from the variable list */
  void remove_variable(const TTCN3_Debugger::variable_t* p_var);
  
  /** searches for the variable entry with the specified name in the function's
    * local variable list, the global scope (if any) and the component scope (if any),
    * returns NULL, if the variable was not found */
  TTCN3_Debugger::variable_t* find_variable(const char* p_name) const;
  
  /** prints the function's type, name and current values of parameters */
  void print_function() const;
  
  /** returns the name of the function */
  const char* get_function_name() const { return function_name; }
  
  /** returns the name of the module the function was defined in */
  const char* get_module_name() const { return module_name; }
  
  /** prints the names of variables specified by the parameters (separated by spaces)
    * handles the D_LIST_VARIABLES debugger command 
    * @param p_scope specifies which scope to print variables from:
    * - "local" - the function's local variables (including variables in code blocks)
    * - "global" - variables in the module's global scope
    * - "comp" or "component" - variables in the function's runs-on component scope
    * - "all" - all variables visible in the function (i.e. all of the above)
    * @param p_filter a pattern to filter variable names further */
  void list_variables(const char* p_scope, const char* p_filter) const;
  
  /** returns true if this instance belongs to a control part */
  boolean is_control_part() const;
  
  /** returns true if this instance belongs to a test case */
  boolean is_test_case() const;
};

/** This macro stores a function's return value in the current function.
  * The returned value might be an expression, so it is copied to a temporary first,
  * to avoid being evaluated twice. */
#define DEBUGGER_STORE_RETURN_VALUE(tmp, ret_val) \
  (tmp = (ret_val), \
   ttcn3_debugger.set_return_value((TTCN_Logger::begin_event_log2str(), \
                                    tmp.log(), \
                                    TTCN_Logger::end_event_log2str())), \
   tmp)

#endif /* DEBUGGER_HH */

