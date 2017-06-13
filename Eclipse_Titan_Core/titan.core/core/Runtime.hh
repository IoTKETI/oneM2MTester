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
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef RUNTIME_HH
#define RUNTIME_HH

#include <sys/types.h>
#include "Types.h"

class Text_Buf;
class COMPONENT;
class VERDICTTYPE;
class CHARSTRING;
class INTEGER;
class PORT;

extern "C" {
typedef void (*signal_handler_type)(int);
}

/** @brief
*/
class TTCN_Runtime {
public:
  enum executor_state_enum {
    UNDEFINED_STATE, // 0

    SINGLE_CONTROLPART, SINGLE_TESTCASE, // 1,2

    HC_INITIAL, HC_IDLE, HC_CONFIGURING, HC_ACTIVE, HC_OVERLOADED, // 3-7
    HC_OVERLOADED_TIMEOUT, HC_EXIT, // 8-9

    MTC_INITIAL, MTC_IDLE, MTC_CONTROLPART, MTC_TESTCASE, // 10-13
    MTC_TERMINATING_TESTCASE, MTC_TERMINATING_EXECUTION, MTC_PAUSED, // 14-16
    MTC_CREATE, MTC_START, MTC_STOP, MTC_KILL, MTC_RUNNING, MTC_ALIVE, // 17-22
    MTC_DONE, MTC_KILLED, MTC_CONNECT, MTC_DISCONNECT, MTC_MAP, MTC_UNMAP, // 23-28
    MTC_CONFIGURING, MTC_EXIT, // 30

    PTC_INITIAL, PTC_IDLE, PTC_FUNCTION, PTC_CREATE, PTC_START, PTC_STOP, // 31-36
    PTC_KILL, PTC_RUNNING, PTC_ALIVE, PTC_DONE, PTC_KILLED, PTC_CONNECT, // 37-42
    PTC_DISCONNECT, PTC_MAP, PTC_UNMAP, PTC_STOPPED, PTC_EXIT // 43-47
  };
private:
  static executor_state_enum executor_state;

  static qualified_name component_type;
  static char *component_name;
  static boolean is_alive;

  static const char *control_module_name;
  static qualified_name testcase_name;

  static char *host_name;

  static verdicttype local_verdict;
  static unsigned int verdict_count[5], control_error_count;
  static CHARSTRING verdict_reason;

  /** TTCN_TryBlock uses the private member in_ttcn_try_block */
  friend class TTCN_TryBlock;
  /** true if execution is currently inside a TTCN-3 try{} */
  static boolean in_ttcn_try_block;

  static char *begin_controlpart_command, *end_controlpart_command,
  *begin_testcase_command, *end_testcase_command;

  static component create_done_killed_compref;
  static boolean running_alive_result;

  static alt_status any_component_done_status, all_component_done_status,
  any_component_killed_status, all_component_killed_status;
  static int component_status_table_size;
  static component component_status_table_offset;
  struct component_status_table_struct;
  static component_status_table_struct *component_status_table;

  struct component_process_struct;
  static component_process_struct **components_by_compref,
  **components_by_pid;
  
  // Translation flag is set to true before each port translation function called,
  // and set to false after each port translation function.
  static boolean translation_flag;
  // The port which state will be changed by change_port_state
  static PORT* p;

public:
  inline static executor_state_enum get_state() { return executor_state; }
  inline static void set_state(executor_state_enum new_state)
  { executor_state = new_state; }

  /** @name Identifying the type
  *   @{
  */
  inline static boolean is_hc()
  { return executor_state >= HC_INITIAL && executor_state <= HC_EXIT; }
  inline static boolean is_mtc()
  { return executor_state >= MTC_INITIAL && executor_state <= MTC_EXIT; }
  inline static boolean is_ptc()
  { return executor_state >= PTC_INITIAL && executor_state <= PTC_EXIT; }
  inline static boolean is_tc()
  { return executor_state >= MTC_INITIAL && executor_state <= PTC_EXIT; }
  inline static boolean is_single()
  { return executor_state >= SINGLE_CONTROLPART &&
    executor_state <= SINGLE_TESTCASE; }
  inline static boolean is_undefined() /* e.g.: when listing test cases (<EXE> -l) */
  { return executor_state == UNDEFINED_STATE; }
  static boolean is_idle();
  inline static boolean is_overloaded()
  { return executor_state == HC_OVERLOADED ||
    executor_state == HC_OVERLOADED_TIMEOUT; }
  /** @} */

  static boolean is_in_ttcn_try_block() { return in_ttcn_try_block; }
  
  static void set_port_state(const INTEGER& state, const CHARSTRING& info, boolean by_system);
  static void set_translation_mode(boolean enabled, PORT* port);

private:
  inline static boolean in_controlpart()
  { return executor_state == SINGLE_CONTROLPART ||
    executor_state == MTC_CONTROLPART; }
  /** Whether verdict operations are allowed */
  static boolean verdict_enabled();
  static void wait_for_state_change();
  static void clear_qualified_name(qualified_name& q_name);

  static void initialize_component_type();
  static void terminate_component_type();

public:
  static void clean_up();
  static void set_component_type(const char *component_type_module,
    const char *component_type_name);
  static void set_component_name(const char *new_component_name);
  inline static void set_alive_flag(boolean par_is_alive)
  { is_alive = par_is_alive; }
  static void set_testcase_name(const char *par_module_name,
    const char *par_testcase_name);

  inline static const char *get_component_type()
  { return component_type.definition_name; }
  inline static const char *get_component_name()
  { return component_name; }
  inline static const char *get_testcase_name()
  { return testcase_name.definition_name; }

  /// Returns a string which must not be freed.
  static const char *get_host_name();
  
  static CHARSTRING get_host_address(const CHARSTRING& type);

  static CHARSTRING get_testcase_id_macro();
  static CHARSTRING get_testcasename();

  static void load_logger_plugins();
  static void set_logger_parameters();

  static const char *get_signal_name(int signal_number);
private:
  static void set_signal_handler(int signal_number, const char *signal_name,
    signal_handler_type signal_handler);
  static void restore_default_handler(int signal_number,
    const char *signal_name);
  static void ignore_signal(int signal_number, const char *signal_name);
  static void enable_interrupt_handler();
  static void disable_interrupt_handler();
public:
  static void install_signal_handlers();
  static void restore_signal_handlers();

public:
  static int hc_main(const char *local_addr, const char *MC_addr,
    unsigned short MC_port);
  static int mtc_main();
  static int ptc_main();

  static component create_component(const char *created_component_type_module,
    const char *created_component_type_name,
    const char *created_component_name,
    const char *created_component_location,
    boolean created_component_alive);
  static void prepare_start_component(const COMPONENT& component_reference,
    const char *module_name, const char *function_name,
    Text_Buf& text_buf);
  static void send_start_component(Text_Buf& text_buf);
  static void start_function(const char *module_name,
    const char *function_name, Text_Buf& text_buf);
  static void function_started(Text_Buf& text_buf);
  static void prepare_function_finished(const char *return_type,
    Text_Buf& text_buf);
  static void send_function_finished(Text_Buf& text_buf);
  static void function_finished(const char *function_name);

  static alt_status component_done(component component_reference);
  static alt_status component_done(component component_reference,
    const char *return_type, Text_Buf*& text_buf);
  static alt_status component_killed(component component_reference);
  static boolean component_running(component component_reference);
  static boolean component_alive(component component_reference);
  static void stop_component(component component_reference);
  static void stop_execution()
  __attribute__ ((__noreturn__));
  static void kill_component(component component_reference);
  static void kill_execution()
  __attribute__ ((__noreturn__));

private:
  static alt_status ptc_done(component component_reference);
  static alt_status any_component_done();
  static alt_status all_component_done();
  static alt_status ptc_killed(component component_reference);
  static alt_status any_component_killed();
  static alt_status all_component_killed();
  static boolean ptc_running(component component_reference);
  static boolean any_component_running();
  static boolean all_component_running();
  static boolean ptc_alive(component component_reference);
  static boolean any_component_alive();
  static boolean all_component_alive();
  static void stop_mtc()
  __attribute__ ((__noreturn__));
  static void stop_ptc(component component_reference);
  static void stop_all_component();
  static void kill_ptc(component component_reference);
  static void kill_all_component();

  static void check_port_name(const char *port_name,
    const char *operation_name, const char *which_argument);
public:
  static void connect_port(
    const COMPONENT& src_compref, const char *src_port,
    const COMPONENT& dst_compref, const char *dst_port);
  static void disconnect_port(
    const COMPONENT& src_compref, const char *src_port,
    const COMPONENT& dst_compref, const char *dst_port);
  static void map_port(
    const COMPONENT& src_compref, const char *src_port,
    const COMPONENT& dst_compref, const char *dst_port);
  static void unmap_port(
    const COMPONENT& src_compref, const char *src_port,
    const COMPONENT& dst_compref, const char *dst_port);

  static void begin_controlpart(const char *module_name);
  static void end_controlpart();
  static void check_begin_testcase(boolean has_timer, double timer_value);
  static void begin_testcase(
    const char *par_module_name, const char *par_testcase_name,
    const char *mtc_comptype_module, const char *mtc_comptype_name,
    const char *system_comptype_module, const char *system_comptype_name,
    boolean has_timer, double timer_value);
  static verdicttype end_testcase();
  static void log_verdict_statistics();

  static void begin_action();
  static void end_action();

  static void setverdict(verdicttype new_value, const char* reason = "");
  static void setverdict(const VERDICTTYPE& new_value,
    const char* reason = "");
  static void set_error_verdict();
  static verdicttype getverdict();
private:
  static void setverdict_internal(verdicttype new_value,
    const char* reason = "");

public:
  /** @name Manipulating external commands
  *   @{
  */
  static void set_begin_controlpart_command(const char *new_command);
  static void set_end_controlpart_command(const char *new_command);
  static void set_begin_testcase_command(const char *new_command);
  static void set_end_testcase_command(const char *new_command);
  static void clear_external_commands();
  /** @} */
private:
  static char *shell_escape(const char *command_str);
  static void execute_command(const char *command_name,
    const char *argument_string);

public:
  static void process_create_mtc();
  static void process_create_ptc(component component_reference,
    const char *component_type_module, const char *component_type_name,
    const char *par_component_name, boolean par_is_alive,
    const char *current_testcase_module, const char *current_testcase_name);

  static void process_create_ack(component new_component);
  static void process_running(boolean result_value);
  static void process_alive(boolean result_value);
  static void process_done_ack(boolean done_status,
    const char *return_type, int return_value_len,
    const void *return_value);
  static void process_killed_ack(boolean killed_status);
  static void process_ptc_verdict(Text_Buf& text_buf);
  static void process_kill();
  static void process_kill_process(component component_reference);

  static void set_component_done(component component_reference,
    const char *return_type, int return_value_len,
    const void *return_value);
  static void set_component_killed(component component_reference);
  static void cancel_component_done(component component_reference);

private:
  static int get_component_status_table_index(component component_reference);
  static alt_status get_killed_status(component component_reference);
  static boolean in_component_status_table(component component_reference);
  static void clear_component_status_table();

  static void initialize_component_process_tables();
  static void add_component(component component_reference, pid_t process_id);
  static void remove_component(component_process_struct *comp);
  static component_process_struct *get_component_by_compref(component
    component_reference);
  static component_process_struct *get_component_by_pid(pid_t process_id);
  static void clear_component_process_tables();

  static void successful_process_creation();
  static void failed_process_creation();

public:
  static void wait_terminated_processes();
  static void check_overload();
};

/** TTCN_TryBlock must be used only as a local variable of a TTCN-3 try{} block.
    It handles the value of TTCN_Runtime::in_ttcn_try_block using C++'s RAII feature */
class TTCN_TryBlock {
  boolean outmost_try;
public:
  TTCN_TryBlock() {
    if (TTCN_Runtime::in_ttcn_try_block) {
      outmost_try = FALSE;
    } else {
      outmost_try = TRUE;
      TTCN_Runtime::in_ttcn_try_block = TRUE;
    }
  }
  ~TTCN_TryBlock() {
    if (outmost_try) {
      TTCN_Runtime::in_ttcn_try_block = FALSE;
    }
  }
};

#endif
