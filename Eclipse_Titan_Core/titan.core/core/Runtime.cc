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
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#if defined(LINUX) && ! defined(_GNU_SOURCE)
  // in order to get the prototype of non-standard strsignal()
# define _GNU_SOURCE
#endif

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#ifdef SOLARIS8
#include <sys/utsname.h>
#endif

#include "../common/memory.h"
#include "../common/version_internal.h"
#include "Runtime.hh"
#include "Communication.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Snapshot.hh"
#include "Port.hh"
#include "Timer.hh"
#include "Module_list.hh"
#include "Component.hh"
#include "Default.hh"
#include "Verdicttype.hh"
#include "Charstring.hh"
#include "Fd_And_Timeout_User.hh"
#include <TitanLoggerApi.hh>
#include "Profiler.hh"
#include "Integer.hh"
#include "Port.hh"

namespace API = TitanLoggerApi;

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif

#include "../common/dbgnew.hh"

TTCN_Runtime::executor_state_enum
    TTCN_Runtime::executor_state = UNDEFINED_STATE;

qualified_name TTCN_Runtime::component_type = { NULL, NULL };
char *TTCN_Runtime::component_name = NULL;
boolean TTCN_Runtime::is_alive = FALSE;

const char *TTCN_Runtime::control_module_name = NULL;
qualified_name TTCN_Runtime::testcase_name = { NULL, NULL };

char *TTCN_Runtime::host_name = NULL;

verdicttype TTCN_Runtime::local_verdict = NONE;
unsigned int TTCN_Runtime::verdict_count[5] = { 0, 0, 0, 0, 0 },
    TTCN_Runtime::control_error_count = 0;
CHARSTRING TTCN_Runtime::verdict_reason(0, ""); // empty string

boolean TTCN_Runtime::in_ttcn_try_block = FALSE;

char *TTCN_Runtime::begin_controlpart_command = NULL,
    *TTCN_Runtime::end_controlpart_command = NULL,
    *TTCN_Runtime::begin_testcase_command = NULL,
    *TTCN_Runtime::end_testcase_command = NULL;

component TTCN_Runtime::create_done_killed_compref = NULL_COMPREF;
boolean TTCN_Runtime::running_alive_result = FALSE;

alt_status TTCN_Runtime::any_component_done_status = ALT_UNCHECKED,
    TTCN_Runtime::all_component_done_status = ALT_UNCHECKED,
    TTCN_Runtime::any_component_killed_status = ALT_UNCHECKED,
    TTCN_Runtime::all_component_killed_status = ALT_UNCHECKED;
int TTCN_Runtime::component_status_table_size = 0;
component TTCN_Runtime::component_status_table_offset = FIRST_PTC_COMPREF;
struct TTCN_Runtime::component_status_table_struct {
    alt_status done_status, killed_status;
    char *return_type;
    Text_Buf *return_value;
} *TTCN_Runtime::component_status_table = NULL;

struct TTCN_Runtime::component_process_struct {
  component component_reference;
  pid_t process_id;
  boolean process_killed;
  struct component_process_struct *prev_by_compref, *next_by_compref;
  struct component_process_struct *prev_by_pid, *next_by_pid;
} **TTCN_Runtime::components_by_compref = NULL,
  **TTCN_Runtime::components_by_pid = NULL;

boolean TTCN_Runtime::translation_flag = FALSE;
PORT* TTCN_Runtime::p = NULL;

void TTCN_Runtime::set_port_state(const INTEGER& state, const CHARSTRING& info, boolean by_system) {
  if (translation_flag) {
    if (p != NULL) {
      int lowed_end = by_system ? -1 : 0;
      if (state < lowed_end || state > 3) {
        TTCN_error("The value of the first parameter in the setstate operation must be 0, 1, 2 or 3.");
      }
      p->change_port_state((translation_port_state)((int)state));
      TTCN_Logger::log_setstate(p->get_name(), (translation_port_state)((int)state), info);
    } else {
      TTCN_error("Internal error: TTCN_Runtime::set_port_state: The port is NULL.");
    }
  } else {
    TTCN_error("setstate operation was called without being in a translation procedure.");
  }
}

void TTCN_Runtime::set_translation_mode(boolean enabled, PORT* port) {
  translation_flag = enabled;
  p = port;
}

boolean TTCN_Runtime::is_idle()
{
  switch (executor_state) {
  case HC_IDLE:
  case HC_ACTIVE:
  case HC_OVERLOADED:
  case MTC_IDLE:
  case PTC_IDLE:
  case PTC_STOPPED:
    return TRUE;
  default:
    return FALSE;
  }
}

boolean TTCN_Runtime::verdict_enabled()
{
  return executor_state == SINGLE_TESTCASE ||
    (executor_state >= MTC_TESTCASE && executor_state <= MTC_EXIT) ||
    (executor_state >= PTC_INITIAL && executor_state <= PTC_EXIT);
}

void TTCN_Runtime::wait_for_state_change()
{
  executor_state_enum old_state = executor_state;
  do {
    TTCN_Snapshot::take_new(TRUE);
  } while (old_state == executor_state);
}

void TTCN_Runtime::clear_qualified_name(qualified_name& q_name)
{
  Free(q_name.module_name);
  q_name.module_name = NULL;
  Free(q_name.definition_name);
  q_name.definition_name = NULL;
}

void TTCN_Runtime::clean_up()
{
  clear_qualified_name(component_type);
  Free(component_name);
  component_name = NULL;
  control_module_name = NULL;
  clear_qualified_name(testcase_name);
  Free(host_name);
  host_name = NULL;
  clear_external_commands();
}

void TTCN_Runtime::initialize_component_type()
{
  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::init__component__start,
    component_type.module_name, component_type.definition_name, 0, NULL,
    TTCN_Runtime::get_testcase_name());

  Module_List::initialize_component(component_type.module_name,
    component_type.definition_name, TRUE);
  PORT::set_parameters((component)self, component_name);
  PORT::all_start();

  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::init__component__finish,
    component_type.module_name, component_type.definition_name);

  local_verdict = NONE;
  verdict_reason = "";
}

void TTCN_Runtime::terminate_component_type()
{
  if (component_type.module_name != NULL &&
    component_type.definition_name != NULL) {
    TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::terminating__component,
      component_type.module_name, component_type.definition_name);

    TTCN_Default::deactivate_all();
    TIMER::all_stop();
    PORT::deactivate_all();

    TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::component__shut__down,
      component_type.module_name, component_type.definition_name, 0, NULL,
      TTCN_Runtime::get_testcase_name());

    clear_qualified_name(component_type);
    Free(component_name);
    component_name = NULL;
  }
}

void TTCN_Runtime::set_component_type(const char *component_type_module,
  const char *component_type_name)
{
  if (component_type_module == NULL || component_type_module[0] == '\0' ||
    component_type_name == NULL || component_type_name[0] == '\0')
    TTCN_error("Internal error: TTCN_Runtime::set_component_type: "
      "Trying to set an invalid component type.");
  if (component_type.module_name != NULL ||
    component_type.definition_name != NULL)
    TTCN_error("Internal error: TTCN_Runtime::set_component_type: "
      "Trying to set component type %s.%s while another one is active.",
      component_type_module, component_type_name);

  component_type.module_name = mcopystr(component_type_module);
  component_type.definition_name = mcopystr(component_type_name);
}

void TTCN_Runtime::set_component_name(const char *new_component_name)
{
  Free(component_name);
  if (new_component_name != NULL && new_component_name[0] != '\0')
    component_name = mcopystr(new_component_name);
  else component_name = NULL;
}

void TTCN_Runtime::set_testcase_name(const char *par_module_name,
  const char *par_testcase_name)
{
  if (par_module_name == NULL || par_module_name[0] == '\0' ||
    par_testcase_name == NULL || par_testcase_name[0] == '\0')
    TTCN_error("Internal error: TTCN_Runtime::set_testcase_name: "
      "Trying to set an invalid testcase name.");
  if (testcase_name.module_name != NULL ||
    testcase_name.definition_name != NULL)
    TTCN_error("Internal error: TTCN_Runtime::set_testcase_name: "
      "Trying to set testcase name %s.%s while another one is active.",
      par_module_name, par_testcase_name);

  testcase_name.module_name = mcopystr(par_module_name);
  testcase_name.definition_name = mcopystr(par_testcase_name);
}

const char *TTCN_Runtime::get_host_name()
{
  if (host_name == NULL) {
#if defined(SOLARIS8)
    // Workaround for Solaris10 (lumped under SOLARIS8) + dynamic linking.
    // "g++ -shared" seems to produce a very strange kind of symbol
    // for gethostname in the .so, and linking fails with the infamous
    // "ld: libttcn3-dynamic.so: gethostname: invalid version 3 (max 0)"
    // The workaround is to use uname instead of gethostname.
    struct utsname uts;
    if (uname(&uts) < 0) {
      TTCN_Logger::begin_event(TTCN_Logger::WARNING_UNQUALIFIED);
      TTCN_Logger::log_event_str("System call uname() failed.");
      TTCN_Logger::OS_error();
      TTCN_Logger::end_event();
      host_name = mcopystr("unknown");
    } else {
      host_name = mcopystr(uts.nodename);
    }
#else
    char tmp_str[MAXHOSTNAMELEN + 1];
    if (gethostname(tmp_str, MAXHOSTNAMELEN)) {
      TTCN_Logger::begin_event(TTCN_Logger::WARNING_UNQUALIFIED);
      TTCN_Logger::log_event_str("System call gethostname() failed.");
      TTCN_Logger::OS_error();
      TTCN_Logger::end_event();
      tmp_str[0] = '\0';
    } else tmp_str[MAXHOSTNAMELEN] = '\0';
    if (tmp_str[0] != '\0') host_name = mcopystr(tmp_str);
    else host_name = mcopystr("unknown");
#endif
  }
  return host_name;
}

CHARSTRING TTCN_Runtime::get_host_address(const CHARSTRING& type)
{
  if (type != "Ipv4orIpv6" && type != "Ipv4" && type != "Ipv6") {
      TTCN_error("The argument of hostid function must be Ipv4orIpv6 or Ipv4"
        "or Ipv6. %s is not a valid argument.", (const char*)type);
  }
  
  // Return empty if no host address could be retrieved
  if (!TTCN_Communication::has_local_address()) {
    return CHARSTRING("");
  }
  const IPAddress *address = TTCN_Communication::get_local_address();
  
  // Return empty if the type is not matching the address type
  if (type == "Ipv4") {
    const IPv4Address * ipv4 = dynamic_cast<const IPv4Address*>(address);
    if (ipv4 == NULL) {
      return CHARSTRING("");
    }
  }
  if (type == "Ipv6") {
#if defined(LINUX) || defined(CYGWIN17)
    const IPv6Address * ipv6 = dynamic_cast<const IPv6Address*>(address);
    if (ipv6 == NULL) 
#endif // LINUX || CYGWIN17
      return CHARSTRING("");
  }
  // Return the string representation of the address
  return CHARSTRING(address->get_addr_str());
}

CHARSTRING TTCN_Runtime::get_testcase_id_macro()
{
  if (in_controlpart()) TTCN_error("Macro %%testcaseId cannot be used from "
    "the control part outside test cases.");
  if (testcase_name.definition_name == NULL ||
    testcase_name.definition_name[0] == '\0')
    TTCN_error("Internal error: Evaluating macro %%testcaseId, but the "
      "name of the current testcase is not set.");
  return CHARSTRING(testcase_name.definition_name);
}

CHARSTRING TTCN_Runtime::get_testcasename()
{
  if (in_controlpart() || is_hc()) return CHARSTRING("");  // No error here.

  if (!testcase_name.definition_name || testcase_name.definition_name[0] == 0)
    TTCN_error("Internal error: Evaluating predefined function testcasename()"
      ", but the name of the current testcase is not set.");

  return CHARSTRING(testcase_name.definition_name);
}

void TTCN_Runtime::load_logger_plugins()
{
  TTCN_Logger::load_plugins((component)self, component_name);
}

void TTCN_Runtime::set_logger_parameters()
{
  TTCN_Logger::set_plugin_parameters((component)self, component_name);
}

const char *TTCN_Runtime::get_signal_name(int signal_number)
{
  const char *signal_name = strsignal(signal_number);
  if (signal_name != NULL) return signal_name;
  else return "Unknown signal";
}

static void sigint_handler(int signum)
{
  if (signum != SIGINT) {
    TTCN_warning("Unexpected signal %d (%s) was caught by the handler of "
      "SIGINT.", signum, TTCN_Runtime::get_signal_name(signum));
    return;
  }
  if (TTCN_Runtime::is_single()) {
    TTCN_Logger::log_str(TTCN_Logger::WARNING_UNQUALIFIED,
      "Execution was interrupted by the user.");
    if (TTCN_Runtime::get_state() == TTCN_Runtime::SINGLE_TESTCASE) {
      TTCN_Logger::log_executor_runtime(
        API::ExecutorRuntime_reason::stopping__current__testcase);
      TTCN_Runtime::end_testcase();
    } else {
      TIMER::all_stop();
    }
    TTCN_Logger::log_executor_runtime(
      API::ExecutorRuntime_reason::exiting);
    exit(EXIT_FAILURE);
  }
}


void TTCN_Runtime::set_signal_handler(int signal_number,
  const char *signal_name, signal_handler_type signal_handler)
{
  struct sigaction sig_act;
  if (sigaction(signal_number, NULL, &sig_act))
    TTCN_error("System call sigaction() failed when getting signal "
      "handling information for %s.", signal_name);
  sig_act.sa_handler = signal_handler;
  sig_act.sa_flags = 0;
  if (sigaction(signal_number, &sig_act, NULL))
    TTCN_error("System call sigaction() failed when changing the signal "
      "handling settings for %s.", signal_name);
}

void TTCN_Runtime::restore_default_handler(int signal_number,
  const char *signal_name)
{
  struct sigaction sig_act;
  if (sigaction(signal_number, NULL, &sig_act))
    TTCN_error("System call sigaction() failed when getting signal "
      "handling information for %s.", signal_name);
  sig_act.sa_handler = SIG_DFL;
  sig_act.sa_flags = 0;
  if (sigaction(signal_number, &sig_act, NULL))
    TTCN_error("System call sigaction() failed when restoring the "
      "default signal handling settings for %s.", signal_name);
}

void TTCN_Runtime::ignore_signal(int signal_number, const char *signal_name)
{
  struct sigaction sig_act;
  if (sigaction(signal_number, NULL, &sig_act))
    TTCN_error("System call sigaction() failed when getting signal "
      "handling information for %s.", signal_name);
  sig_act.sa_handler = SIG_IGN;
  sig_act.sa_flags = 0;
  if (sigaction(signal_number, &sig_act, NULL))
    TTCN_error("System call sigaction() failed when disabling signal "
      "%s.", signal_name);
}

void TTCN_Runtime::enable_interrupt_handler()
{
  set_signal_handler(SIGINT, "SIGINT", sigint_handler);
}

void TTCN_Runtime::disable_interrupt_handler()
{
  ignore_signal(SIGINT, "SIGINT");
}

void TTCN_Runtime::install_signal_handlers()
{
  if (is_single()) set_signal_handler(SIGINT, "SIGINT", sigint_handler);
  ignore_signal(SIGPIPE, "SIGPIPE");
}

void TTCN_Runtime::restore_signal_handlers()
{
  if (is_single()) restore_default_handler(SIGINT, "SIGINT");
  restore_default_handler(SIGPIPE, "SIGPIPE");
}

int TTCN_Runtime::hc_main(const char *local_addr, const char *MC_addr,
  unsigned short MC_port)
{
  int ret_val = EXIT_SUCCESS;
  executor_state = HC_INITIAL;
  TTCN_Logger::log_HC_start(get_host_name());
  TTCN_Logger::write_logger_settings();
  TTCN_Snapshot::check_fd_setsize();
#ifdef USAGE_STATS
  pthread_t stats_thread = 0;
  thread_data* stats_data = NULL;
#endif
  try {
    if (local_addr != NULL)
      TTCN_Communication::set_local_address(local_addr);
    TTCN_Communication::set_mc_address(MC_addr, MC_port);
    TTCN_Communication::connect_mc();
#ifdef USAGE_STATS
    Module_List::send_usage_stats(stats_thread, stats_data);
#endif
    executor_state = HC_IDLE;
    TTCN_Communication::send_version();
    initialize_component_process_tables();
    do {
      TTCN_Snapshot::take_new(TRUE);
      TTCN_Communication::process_all_messages_hc();
    } while (executor_state >= HC_IDLE && executor_state < HC_EXIT);
    if (executor_state == HC_EXIT) {
      // called only on the HC
      TTCN_Communication::disconnect_mc();
      clean_up();
    }
  } catch (const TC_Error& tc_error) {
    ret_val = EXIT_FAILURE;
    clean_up();
  }
  // called on the newly created MTC and PTCs as well because
  // the hashtables are inherited with fork()
  clear_component_process_tables();

  if (is_hc())
    TTCN_Logger::log_executor_runtime(
      API::ExecutorRuntime_reason::host__controller__finished);

#ifdef USAGE_STATS
  Module_List::clean_up_usage_stats(stats_thread, stats_data);
#endif

  return ret_val;
}

int TTCN_Runtime::mtc_main()
{
  int ret_val = EXIT_SUCCESS;
  TTCN_Runtime::load_logger_plugins();
  TTCN_Runtime::set_logger_parameters();
  TTCN_Logger::open_file();
  TTCN_Logger::log_executor_component(API::ExecutorComponent_reason::mtc__started);
  TTCN_Logger::write_logger_settings();
  try {
    TTCN_Communication::connect_mc();
    executor_state = MTC_IDLE;
    TTCN_Communication::send_mtc_created();
    do {
      TTCN_Snapshot::take_new(TRUE);
      TTCN_Communication::process_all_messages_tc();
    } while (executor_state != MTC_EXIT);
    TTCN_Logger::close_file();
    TTCN_Communication::disconnect_mc();
    clean_up();
  } catch (const TC_Error& tc_error) {
    ret_val = EXIT_FAILURE;
  }
  TTCN_Logger::log_executor_component(API::ExecutorComponent_reason::mtc__finished);
  return ret_val;
}

int TTCN_Runtime::ptc_main()
{
  int ret_val = EXIT_SUCCESS;
  TTCN_Runtime::load_logger_plugins();
  TTCN_Runtime::set_logger_parameters();
  TTCN_Logger::open_file();
  TTCN_Logger::begin_event(TTCN_Logger::EXECUTOR_COMPONENT);
  TTCN_Logger::log_event("TTCN-3 Parallel Test Component started on %s. "
    "Component reference: ", get_host_name());
  self.log();
  TTCN_Logger::log_event(", component type: %s.%s",
    component_type.module_name, component_type.definition_name);
  if (component_name != NULL)
    TTCN_Logger::log_event(", component name: %s", component_name);
  TTCN_Logger::log_event_str(". Version: " PRODUCT_NUMBER ".");
  TTCN_Logger::end_event();
  TTCN_Logger::write_logger_settings();
  try {
    TTCN_Communication::connect_mc();
    executor_state = PTC_IDLE;
    TTCN_Communication::send_ptc_created((component)self);
    try {
      initialize_component_type();
    } catch (const TC_Error& tc_error) {
      TTCN_Logger::log_executor_component(API::ExecutorComponent_reason::component__init__fail);
      ret_val = EXIT_FAILURE;
    }
    if (ret_val == EXIT_SUCCESS) {
      if (ttcn3_debugger.is_activated()) {
        ttcn3_debugger.init_PTC_settings();
      }
      try {
        do {
          TTCN_Snapshot::take_new(TRUE);
          TTCN_Communication::process_all_messages_tc();
        } while (executor_state != PTC_EXIT);
      } catch (const TC_Error& tc_error) {
        TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::error__idle__ptc);
        ret_val = EXIT_FAILURE;
      }
    }
    if (ret_val != EXIT_SUCCESS) {
      // ignore errors in subsequent operations
      try {
        terminate_component_type();
      } catch (const TC_Error& tc_error) { }
      try {
        TTCN_Communication::send_killed(local_verdict, (const char *)verdict_reason);
      } catch (const TC_Error& tc_error) { }
      TTCN_Logger::log_final_verdict(TRUE, local_verdict, local_verdict,
                                     local_verdict, (const char *)verdict_reason);
      executor_state = PTC_EXIT;
    }
    TTCN_Communication::disconnect_mc();
    clear_component_status_table();
    clean_up();
  } catch (const TC_Error& tc_error) {
    ret_val = EXIT_FAILURE;
  }
  TTCN_Logger::log_executor_component(API::ExecutorComponent_reason::ptc__finished);
  return ret_val;
}

component TTCN_Runtime::create_component(
  const char *created_component_type_module,
  const char *created_component_type_name, const char *created_component_name,
  const char *created_component_location, boolean created_component_alive)
{
  if (in_controlpart())
    TTCN_error("Create operation cannot be performed in the control part.");
  else if (is_single())
    TTCN_error("Create operation cannot be performed in single mode.");

  if (created_component_name != NULL &&
    created_component_name[0] == '\0') {
    TTCN_warning("Empty charstring value was ignored as component name "
      "in create operation.");
    created_component_name = NULL;
  }
  if (created_component_location != NULL &&
    created_component_location[0] == '\0') {
    TTCN_warning("Empty charstring value was ignored as component location "
      "in create operation.");
    created_component_location = NULL;
  }

  TTCN_Logger::begin_event(TTCN_Logger::PARALLEL_UNQUALIFIED);
  TTCN_Logger::log_event("Creating new %sPTC with component type %s.%s",
    created_component_alive ? "alive ": "", created_component_type_module,
      created_component_type_name);
  if (created_component_name != NULL)
    TTCN_Logger::log_event(", component name: %s", created_component_name);
  if (created_component_location != NULL)
    TTCN_Logger::log_event(", location: %s", created_component_location);
  TTCN_Logger::log_char('.');
  TTCN_Logger::end_event();

  switch (executor_state) {
  case MTC_TESTCASE:
    executor_state = MTC_CREATE;
    break;
  case PTC_FUNCTION:
    executor_state = PTC_CREATE;
    break;
  default:
    TTCN_error("Internal error: Executing create operation in invalid "
      "state.");
  }
  TTCN_Communication::send_create_req(created_component_type_module,
    created_component_type_name, created_component_name,
    created_component_location, created_component_alive);
  if (is_mtc()) {
    // updating the component status flags
    // 'any component.done' and 'any component.killed' might be successful
    // from now since the PTC can terminate by itself
    if (any_component_done_status == ALT_NO)
      any_component_done_status = ALT_UNCHECKED;
    if (any_component_killed_status == ALT_NO)
      any_component_killed_status = ALT_UNCHECKED;
    // 'all component.killed' must be re-evaluated later
    all_component_killed_status = ALT_UNCHECKED;
  }
  wait_for_state_change();

  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::ptc__created,
    created_component_type_module, created_component_type_name,
    create_done_killed_compref, created_component_name,
    created_component_location, created_component_alive);

  COMPONENT::register_component_name(create_done_killed_compref,
    created_component_name);
  return create_done_killed_compref;
}

void TTCN_Runtime::prepare_start_component(const COMPONENT& component_reference,
  const char *module_name, const char *function_name, Text_Buf& text_buf)
{
  if (in_controlpart()) TTCN_error("Start test component operation cannot "
    "be performed in the control part.");
  else if (is_single()) TTCN_error("Start test component operation cannot "
    "be performed in single mode.");
  if (!component_reference.is_bound()) TTCN_error("Performing a start "
    "operation on an unbound component reference.");
  component compref = (component)component_reference;
  switch (compref) {
  case NULL_COMPREF:
    TTCN_error("Start operation cannot be performed on the null "
      "component reference.");
  case MTC_COMPREF:
    TTCN_error("Start operation cannot be performed on the component "
      "reference of MTC.");
  case SYSTEM_COMPREF:
    TTCN_error("Start operation cannot be performed on the component "
      "reference of system.");
  case ANY_COMPREF:
    TTCN_error("Internal error: 'any component' cannot be started.");
  case ALL_COMPREF:
    TTCN_error("Internal error: 'all component' cannot be started.");
  default:
    break;
  }
  if (self == compref) TTCN_error("Start operation cannot be performed on "
    "the own component reference of the initiating component (i.e. "
    "'self.start' is not allowed).");
  if (in_component_status_table(compref)) {
    if (get_killed_status(compref) == ALT_YES) {
      TTCN_error("PTC with component reference %d is not alive anymore. "
        "Start operation cannot be performed on it.", compref);
    }
    // the done status of the PTC shall be invalidated
    cancel_component_done(compref);
  }
  TTCN_Communication::prepare_start_req(text_buf, compref, module_name,
    function_name);
}

void TTCN_Runtime::send_start_component(Text_Buf& text_buf)
{
  switch (executor_state) {
  case MTC_TESTCASE:
    executor_state = MTC_START;
    break;
  case PTC_FUNCTION:
    executor_state = PTC_START;
    break;
  default:
    TTCN_error("Internal error: Executing component start operation "
      "in invalid state.");
  }
  // text_buf already contains a complete START_REQ message.
  TTCN_Communication::send_message(text_buf);
  if (is_mtc()) {
    // updating the component status flags
    // 'all component.done' must be re-evaluated later
    all_component_done_status = ALT_UNCHECKED;
  }
  wait_for_state_change();
  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::function__started);
}

void TTCN_Runtime::start_function(const char *module_name,
  const char *function_name, Text_Buf& text_buf)
{
  switch (executor_state) {
  case PTC_IDLE:
  case PTC_STOPPED:
    break;
  default:
    // the START message must be dropped here because normally it is
    // dropped in function_started()
    text_buf.cut_message();
    TTCN_error("Internal error: Message START arrived in invalid state.");
  }
  try {
    Module_List::start_function(module_name, function_name, text_buf);
    // do nothing: the function terminated normally
    // the message STOPPED or STOPPED_KILLED is already sent out
    // and the state variable is updated
    return;
  } catch (const TC_End& TC_end) {
    // executor_state is already set by stop_execution or kill_execution
    switch (executor_state) {
    case PTC_STOPPED:
      TTCN_Logger::log(TTCN_Logger::PARALLEL_UNQUALIFIED,
        "Function %s was stopped. PTC remains alive and is waiting for next start.",
        function_name);
      // send a STOPPED message without return value
      TTCN_Communication::send_stopped();
      // return and do nothing else
      return;
    case PTC_EXIT:
      TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::function__stopped, NULL,
        function_name);
      break;
    default:
      TTCN_error("Internal error: PTC was stopped in invalid state.");
    }
  } catch (const TC_Error& TC_error) {
    TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::function__error, NULL,
      function_name);
    executor_state = PTC_EXIT;
  }
  // the control reaches this code if the PTC has to be terminated

  // first terminate all ports and timers
  // this may affect the final verdict
  terminate_component_type();
  // send a STOPPED_KILLED message without return value
  TTCN_Communication::send_stopped_killed(local_verdict, verdict_reason);
  TTCN_Logger::log_final_verdict(TRUE, local_verdict, local_verdict,
                                 local_verdict, (const char *)verdict_reason);
}

void TTCN_Runtime::function_started(Text_Buf& text_buf)
{
  // The buffer still contains the incoming START message.
  text_buf.cut_message();
  executor_state = PTC_FUNCTION;
  // The remaining messages must be processed now.
  TTCN_Communication::process_all_messages_tc();
}

void TTCN_Runtime::prepare_function_finished(const char *return_type,
  Text_Buf& text_buf)
{
  if (executor_state != PTC_FUNCTION)
    TTCN_error("Internal error: PTC behaviour function finished in invalid "
               "state.");
  if (is_alive) {
    // Prepare a STOPPED message with the possible return value.
    TTCN_Communication::prepare_stopped(text_buf, return_type);
  } else {
    // First the ports and timers must be stopped and deactivated.  The
    // user_unmap and user_stop functions of Test Ports may detect errors
    // that must be considered in the final verdict of the PTC.
    terminate_component_type();
    // Prepare a STOPPED_KILLED message with the final verdict and the
    // possible return value.
    TTCN_Communication::prepare_stopped_killed(text_buf, local_verdict,
      return_type, verdict_reason);
  }
}

void TTCN_Runtime::send_function_finished(Text_Buf& text_buf)
{
  // send out the STOPPED or STOPPED_KILLED message, which is already
  // complete and contains the return value
  TTCN_Communication::send_message(text_buf);
  // log the final verdict if necessary and update the state variable
  if (is_alive) executor_state = PTC_STOPPED;
  else {
    TTCN_Logger::log_final_verdict(TRUE, local_verdict, local_verdict,
                                   local_verdict, (const char *)verdict_reason);
    executor_state = PTC_EXIT;
  }
}

void TTCN_Runtime::function_finished(const char *function_name)
{
  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::function__finished, NULL,
                           function_name, 0, NULL, NULL, is_alive);
  Text_Buf text_buf;
  prepare_function_finished(NULL, text_buf);
  send_function_finished(text_buf);
}

alt_status TTCN_Runtime::component_done(component component_reference)
{
  if (in_controlpart()) TTCN_error("Done operation cannot be performed "
    "in the control part.");
  switch (component_reference) {
  case NULL_COMPREF:
    TTCN_error("Done operation cannot be performed on the null "
      "component reference.");
  case MTC_COMPREF:
    TTCN_error("Done operation cannot be performed on the component "
      "reference of MTC.");
  case SYSTEM_COMPREF:
    TTCN_error("Done operation cannot be performed on the component "
      "reference of system.");
  case ANY_COMPREF:
    return any_component_done();
  case ALL_COMPREF:
    return all_component_done();
  default:
    return ptc_done(component_reference);
  }
}

alt_status TTCN_Runtime::component_done(component component_reference,
  const char *return_type, Text_Buf*& text_buf)
{
  if (in_controlpart()) TTCN_error("Done operation cannot be performed "
    "in the control part.");
  switch (component_reference) {
  case NULL_COMPREF:
    TTCN_error("Done operation cannot be performed on the null "
      "component reference.");
  case MTC_COMPREF:
    TTCN_error("Done operation cannot be performed on the component "
      "reference of MTC.");
  case SYSTEM_COMPREF:
    TTCN_error("Done operation cannot be performed on the component "
      "reference of system.");
  case ANY_COMPREF:
    TTCN_error("Done operation with return value cannot be performed on "
      "'any component'.");
  case ALL_COMPREF:
    TTCN_error("Done operation with return value cannot be performed on "
      "'all component'.");
  default:
    // the argument refers to a PTC
    break;
  }
  if (is_single()) TTCN_error("Done operation on a component reference "
    "cannot be performed in single mode.");
  if (self == component_reference) {
    TTCN_warning("Done operation on the component reference of self "
      "will never succeed.");
    return ALT_NO;
  } else {
    int index = get_component_status_table_index(component_reference);
    // we cannot use the killed status because we need the return value
    switch (component_status_table[index].done_status) {
    case ALT_UNCHECKED:
      switch (executor_state) {
      case MTC_TESTCASE:
        executor_state = MTC_DONE;
        break;
      case PTC_FUNCTION:
        executor_state = PTC_DONE;
        break;
      default:
        TTCN_error("Internal error: Executing done operation in "
          "invalid state.");
      }
      TTCN_Communication::send_done_req(component_reference);
      component_status_table[index].done_status = ALT_MAYBE;
      create_done_killed_compref = component_reference;
      // wait for DONE_ACK
      wait_for_state_change();
      // always re-evaluate the current alternative using a new snapshot
      return ALT_REPEAT;
      case ALT_YES:
        if (component_status_table[index].return_type != NULL) {
          if (!strcmp(component_status_table[index].return_type,
            return_type)) {
            component_status_table[index].return_value->rewind();
            text_buf = component_status_table[index].return_value;
            return ALT_YES;
          } else {
            TTCN_Logger::log_matching_done(return_type, component_reference,
              component_status_table[index].return_type,
              API::MatchingDoneType_reason::done__failed__wrong__return__type);
            return ALT_NO;
          }
        } else {
          TTCN_Logger::log_matching_done(return_type, component_reference, NULL,
            API::MatchingDoneType_reason::done__failed__no__return);
          return ALT_NO;
        }
      default:
        return ALT_MAYBE;
    }
  }
}

alt_status TTCN_Runtime::component_killed(component component_reference)
{
  if (in_controlpart()) TTCN_error("Killed operation cannot be performed "
    "in the control part.");
  switch (component_reference) {
  case NULL_COMPREF:
    TTCN_error("Killed operation cannot be performed on the null "
      "component reference.");
  case MTC_COMPREF:
    TTCN_error("Killed operation cannot be performed on the component "
      "reference of MTC.");
  case SYSTEM_COMPREF:
    TTCN_error("Killed operation cannot be performed on the component "
      "reference of system.");
  case ANY_COMPREF:
    return any_component_killed();
  case ALL_COMPREF:
    return all_component_killed();
  default:
    return ptc_killed(component_reference);
  }
}

boolean TTCN_Runtime::component_running(component component_reference)
{
  if (in_controlpart()) TTCN_error("Component running operation "
    "cannot be performed in the control part.");
  switch (component_reference) {
  case NULL_COMPREF:
    TTCN_error("Running operation cannot be performed on the null "
      "component reference.");
  case MTC_COMPREF:
    TTCN_error("Running operation cannot be performed on the component "
      "reference of MTC.");
  case SYSTEM_COMPREF:
    TTCN_error("Running operation cannot be performed on the component "
      "reference of system.");
  case ANY_COMPREF:
    return any_component_running();
  case ALL_COMPREF:
    return all_component_running();
  default:
    return ptc_running(component_reference);
  }
}

boolean TTCN_Runtime::component_alive(component component_reference)
{
  if (in_controlpart()) TTCN_error("Alive operation cannot be performed "
    "in the control part.");
  switch (component_reference) {
  case NULL_COMPREF:
    TTCN_error("Alive operation cannot be performed on the null "
      "component reference.");
  case MTC_COMPREF:
    TTCN_error("Alive operation cannot be performed on the component "
      "reference of MTC.");
  case SYSTEM_COMPREF:
    TTCN_error("Alive operation cannot be performed on the component "
      "reference of system.");
  case ANY_COMPREF:
    return any_component_alive();
  case ALL_COMPREF:
    return all_component_alive();
  default:
    return ptc_alive(component_reference);
  }
}

void TTCN_Runtime::stop_component(component component_reference)
{
  if (in_controlpart()) TTCN_error("Component stop operation cannot be "
    "performed in the control part.");

  if (self == component_reference) stop_execution();
  switch (component_reference) {
  case NULL_COMPREF:
    TTCN_error("Stop operation cannot be performed on the null component "
      "reference.");
  case MTC_COMPREF:
    stop_mtc();
    break;
  case SYSTEM_COMPREF:
    TTCN_error("Stop operation cannot be performed on the component "
      "reference of system.");
  case ANY_COMPREF:
    TTCN_error("Internal error: 'any component' cannot be stopped.");
  case ALL_COMPREF:
    stop_all_component();
    break;
  default:
    stop_ptc(component_reference);
  }
}

void TTCN_Runtime::stop_execution()
{
  if (in_controlpart()) {
    TTCN_Logger::log_executor_runtime(
      API::ExecutorRuntime_reason::stopping__control__part__execution);
  } else {
    TTCN_Logger::log_str(TTCN_Logger::PARALLEL_UNQUALIFIED,
      "Stopping test component execution.");
    if (is_ptc()) {
      // the state variable indicates whether the component remains alive
      // after termination or not
      if (is_alive) executor_state = PTC_STOPPED;
      else executor_state = PTC_EXIT;
    }
  }
  throw TC_End();
}

void TTCN_Runtime::kill_component(component component_reference)
{
  if (in_controlpart()) TTCN_error("Kill operation cannot be performed in "
    "the control part.");

  if (self == component_reference) kill_execution();
  switch (component_reference) {
  case NULL_COMPREF:
    TTCN_error("Kill operation cannot be performed on the null component "
      "reference.");
  case MTC_COMPREF:
    // 'mtc.kill' means exactly the same as 'mtc.stop'
    stop_mtc();
    break;
  case SYSTEM_COMPREF:
    TTCN_error("Kill operation cannot be performed on the component "
      "reference of system.");
  case ANY_COMPREF:
    TTCN_error("Internal error: 'any component' cannot be killed.");
  case ALL_COMPREF:
    kill_all_component();
    break;
  default:
    kill_ptc(component_reference);
  }
}

void TTCN_Runtime::kill_execution()
{
  TTCN_Logger::log_str(TTCN_Logger::PARALLEL_UNQUALIFIED,
    "Terminating test component execution.");
  if (is_ptc()) executor_state = PTC_EXIT;
  throw TC_End();
}

alt_status TTCN_Runtime::ptc_done(component component_reference)
{
  if (is_single()) TTCN_error("Done operation on a component reference "
    "cannot be performed in single mode.");
  if (self == component_reference) {
    TTCN_warning("Done operation on the component reference of self "
      "will never succeed.");
    return ALT_NO;
  }
  int index = get_component_status_table_index(component_reference);
  // a successful killed operation on the component reference implies done
  if (component_status_table[index].killed_status == ALT_YES)
    goto success;
  switch (component_status_table[index].done_status) {
  case ALT_UNCHECKED:
    switch (executor_state) {
    case MTC_TESTCASE:
      executor_state = MTC_DONE;
      break;
    case PTC_FUNCTION:
      executor_state = PTC_DONE;
      break;
    default:
      TTCN_error("Internal error: Executing done operation in "
        "invalid state.");
    }
    TTCN_Communication::send_done_req(component_reference);
    component_status_table[index].done_status = ALT_MAYBE;
    create_done_killed_compref = component_reference;
    // wait for DONE_ACK
    wait_for_state_change();
    // always re-evaluate the current alternative using a new snapshot
    return ALT_REPEAT;
    case ALT_YES:
      goto success;
    default:
      return ALT_MAYBE;
  }
  success:
  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::ptc__done,
    NULL, NULL, component_reference);
  return ALT_YES;
}

alt_status TTCN_Runtime::any_component_done()
{
  // the operation is never successful in single mode
  if (is_single()) goto failure;
  if (!is_mtc()) TTCN_error("Operation 'any component.done' can only be "
    "performed on the MTC.");
  // the operation is successful if there is a component reference with a
  // successful done or killed operation
  for (int i = 0; i < component_status_table_size; i++) {
    if (component_status_table[i].done_status == ALT_YES ||
      component_status_table[i].killed_status == ALT_YES) goto success;
  }
  // a successful 'any component.killed' implies 'any component.done'
  if (any_component_killed_status == ALT_YES) goto success;
  switch (any_component_done_status) {
  case ALT_UNCHECKED:
    if (executor_state != MTC_TESTCASE) TTCN_error("Internal error: "
      "Executing 'any component.done' in invalid state.");
    executor_state = MTC_DONE;
    TTCN_Communication::send_done_req(ANY_COMPREF);
    any_component_done_status = ALT_MAYBE;
    create_done_killed_compref = ANY_COMPREF;
    // wait for DONE_ACK
    wait_for_state_change();
    // always re-evaluate the current alternative using a new snapshot
    return ALT_REPEAT;
  case ALT_YES:
    goto success;
  case ALT_NO:
    goto failure;
  default:
    return ALT_MAYBE;
  }
  success:
  TTCN_Logger::log_matching_done(0, 0, 0,
    API::MatchingDoneType_reason::any__component__done__successful);
  return ALT_YES;
  failure:
  TTCN_Logger::log_matching_done(0, 0, 0,
    API::MatchingDoneType_reason::any__component__done__failed);
  return ALT_NO;
}

alt_status TTCN_Runtime::all_component_done()
{
  // the operation is always successful in single mode
  if (is_single()) goto success;
  if (!is_mtc()) TTCN_error("Operation 'all component.done' can only be "
    "performed on the MTC.");
  // a successful 'all component.killed' implies 'all component.done'
  if (all_component_killed_status == ALT_YES) goto success;
  switch (all_component_done_status) {
  case ALT_UNCHECKED:
    if (executor_state != MTC_TESTCASE) TTCN_error("Internal error: "
      "Executing 'all component.done' in invalid state.");
    executor_state = MTC_DONE;
    TTCN_Communication::send_done_req(ALL_COMPREF);
    all_component_done_status = ALT_MAYBE;
    create_done_killed_compref = ALL_COMPREF;
    // wait for DONE_ACK
    wait_for_state_change();
    // always re-evaluate the current alternative using a new snapshot
    return ALT_REPEAT;
  case ALT_YES:
    goto success;
  default:
    return ALT_MAYBE;
  }
  success:
  TTCN_Logger::log_matching_done(0, 0, 0,
    API::MatchingDoneType_reason::all__component__done__successful);
  return ALT_YES;
}

alt_status TTCN_Runtime::ptc_killed(component component_reference)
{
  if (is_single()) TTCN_error("Killed operation on a component reference "
    "cannot be performed in single mode.");
  if (self == component_reference) {
    TTCN_warning("Killed operation on the component reference of self "
      "will never succeed.");
    return ALT_NO;
  }
  int index = get_component_status_table_index(component_reference);
  switch (component_status_table[index].killed_status) {
  case ALT_UNCHECKED:
    switch (executor_state) {
    case MTC_TESTCASE:
      executor_state = MTC_KILLED;
      break;
    case PTC_FUNCTION:
      executor_state = PTC_KILLED;
      break;
    default:
      TTCN_error("Internal error: Executing killed operation in "
        "invalid state.");
    }
    TTCN_Communication::send_killed_req(component_reference);
    component_status_table[index].killed_status = ALT_MAYBE;
    create_done_killed_compref = component_reference;
    // wait for KILLED_ACK
    wait_for_state_change();
    // always re-evaluate the current alternative using a new snapshot
    return ALT_REPEAT;
    case ALT_YES:
      goto success;
    default:
      return ALT_MAYBE;
  }
  success:
  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::ptc__killed,
    NULL, NULL, component_reference);
  return ALT_YES;
}

alt_status TTCN_Runtime::any_component_killed()
{
  // the operation is never successful in single mode
  if (is_single()) goto failure;
  if (!is_mtc()) TTCN_error("Operation 'any component.killed' can only be "
    "performed on the MTC.");
  // the operation is successful if there is a component reference with a
  // successful killed operation
  for (int i = 0; i < component_status_table_size; i++) {
    if (component_status_table[i].killed_status == ALT_YES) goto success;
  }
  switch (any_component_killed_status) {
  case ALT_UNCHECKED:
    if (executor_state != MTC_TESTCASE) TTCN_error("Internal error: "
      "Executing 'any component.killed' in invalid state.");
    executor_state = MTC_KILLED;
    TTCN_Communication::send_killed_req(ANY_COMPREF);
    any_component_killed_status = ALT_MAYBE;
    create_done_killed_compref = ANY_COMPREF;
    // wait for KILLED_ACK
    wait_for_state_change();
    // always re-evaluate the current alternative using a new snapshot
    return ALT_REPEAT;
  case ALT_YES:
    goto success;
  case ALT_NO:
    goto failure;
  default:
    return ALT_MAYBE;
  }
  success:
  TTCN_Logger::log_matching_done(0, 0, 0,
    API::MatchingDoneType_reason::any__component__killed__successful);
  return ALT_YES;
  failure:
  TTCN_Logger::log_matching_done(0, 0, 0,
    API::MatchingDoneType_reason::any__component__killed__failed);
  return ALT_NO;
}

alt_status TTCN_Runtime::all_component_killed()
{
  // the operation is always successful in single mode
  if (is_single()) goto success;
  if (!is_mtc()) TTCN_error("Operation 'all component.killed' can only be "
    "performed on the MTC.");
  switch (all_component_killed_status) {
  case ALT_UNCHECKED:
    if (executor_state != MTC_TESTCASE) TTCN_error("Internal error: "
      "Executing 'all component.killed' in invalid state.");
    executor_state = MTC_KILLED;
    TTCN_Communication::send_killed_req(ALL_COMPREF);
    all_component_killed_status = ALT_MAYBE;
    create_done_killed_compref = ALL_COMPREF;
    // wait for KILLED_ACK
    wait_for_state_change();
    // always re-evaluate the current alternative using a new snapshot
    return ALT_REPEAT;
  case ALT_YES:
    goto success;
  default:
    return ALT_MAYBE;
  }
  success:
  TTCN_Logger::log_matching_done(0, 0, 0,
    API::MatchingDoneType_reason::all__component__killed__successful);
  return ALT_YES;
}

boolean TTCN_Runtime::ptc_running(component component_reference)
{
  if (is_single()) TTCN_error("Running operation on a component reference "
    "cannot be performed in single mode.");
  // the answer is always true if the operation refers to self
  if (self == component_reference) {
    TTCN_warning("Running operation on the component reference of self "
      "always returns true.");
    return TRUE;
  }
  // look into the component status tables
  if (in_component_status_table(component_reference)) {
    int index = get_component_status_table_index(component_reference);
    // the answer is false if a successful done or killed operation was
    // performed on the component reference
    if (component_status_table[index].done_status == ALT_YES ||
      component_status_table[index].killed_status == ALT_YES)
      return FALSE;
  }
  // status flags all_component_done or all_component_killed cannot be used
  // because the component reference might be invalid (e.g. stale)

  // the decision cannot be made locally, MC must be asked
  switch (executor_state) {
  case MTC_TESTCASE:
    executor_state = MTC_RUNNING;
    break;
  case PTC_FUNCTION:
    executor_state = PTC_RUNNING;
    break;
  default:
    TTCN_error("Internal error: Executing component running operation "
      "in invalid state.");
  }
  TTCN_Communication::send_is_running(component_reference);
  // wait for RUNNING
  wait_for_state_change();
  return running_alive_result;
}

boolean TTCN_Runtime::any_component_running()
{
  // the answer is always false in single mode
  if (is_single()) return FALSE;
  if (!is_mtc()) TTCN_error("Operation 'any component.running' can only be "
    "performed on the MTC.");
  // the answer is false if 'all component.done' or 'all component.killed'
  // operation was successful
  if (all_component_done_status == ALT_YES ||
    all_component_killed_status == ALT_YES) return FALSE;
  // the decision cannot be made locally, MC must be asked
  if (executor_state != MTC_TESTCASE) TTCN_error("Internal error: "
    "Executing 'any component.running' in invalid state.");
  TTCN_Communication::send_is_running(ANY_COMPREF);
  executor_state = MTC_RUNNING;
  // wait for RUNNING
  wait_for_state_change();
  // update the status of 'all component.done' in case of negative answer
  if (!running_alive_result) all_component_done_status = ALT_YES;
  return running_alive_result;
}

boolean TTCN_Runtime::all_component_running()
{
  // the answer is always true in single mode
  if (is_single()) return TRUE;
  if (!is_mtc()) TTCN_error("Operation 'all component.running' can only be "
    "performed on the MTC.");
  // return true if no PTCs exist
  if (any_component_done_status == ALT_NO) return TRUE;
  // the done and killed status flags cannot be used since the components
  // that were explicitly stopped or killed must be ignored

  // the decision cannot be made locally, MC must be asked
  if (executor_state != MTC_TESTCASE) TTCN_error("Internal error: "
    "Executing 'all component.running' in invalid state.");
  TTCN_Communication::send_is_running(ALL_COMPREF);
  executor_state = MTC_RUNNING;
  // wait for RUNNING
  wait_for_state_change();
  return running_alive_result;
}

boolean TTCN_Runtime::ptc_alive(component component_reference)
{
  if (is_single()) TTCN_error("Alive operation on a component reference "
    "cannot be performed in single mode.");
  // the answer is always true if the operation refers to self
  if (self == component_reference) {
    TTCN_warning("Alive operation on the component reference of self "
      "always returns true.");
    return TRUE;
  }
  // the answer is false if a successful killed operation was performed
  // on the component reference
  if (in_component_status_table(component_reference) &&
    get_killed_status(component_reference) == ALT_YES) return FALSE;
  // status flag of 'all component.killed' cannot be used because the
  // component reference might be invalid (e.g. stale)

  // the decision cannot be made locally, MC must be asked
  switch (executor_state) {
  case MTC_TESTCASE:
    executor_state = MTC_ALIVE;
    break;
  case PTC_FUNCTION:
    executor_state = PTC_ALIVE;
    break;
  default:
    TTCN_error("Internal error: Executing component running operation "
      "in invalid state.");
  }
  TTCN_Communication::send_is_alive(component_reference);
  // wait for ALIVE
  wait_for_state_change();
  return running_alive_result;
}

boolean TTCN_Runtime::any_component_alive()
{
  // the answer is always false in single mode
  if (is_single()) return FALSE;
  if (!is_mtc()) TTCN_error("Operation 'any component.alive' can only be "
    "performed on the MTC.");
  // the answer is false if 'all component.killed' operation was successful
  if (all_component_killed_status == ALT_YES) return FALSE;
  // the decision cannot be made locally, MC must be asked
  if (executor_state != MTC_TESTCASE) TTCN_error("Internal error: "
    "Executing 'any component.alive' in invalid state.");
  TTCN_Communication::send_is_alive(ANY_COMPREF);
  executor_state = MTC_ALIVE;
  // wait for ALIVE
  wait_for_state_change();
  // update the status of 'all component.killed' in case of negative answer
  if (!running_alive_result) all_component_killed_status = ALT_YES;
  return running_alive_result;
}

boolean TTCN_Runtime::all_component_alive()
{
  // the answer is always true in single mode
  if (is_single()) return TRUE;
  if (!is_mtc()) TTCN_error("Operation 'all component.alive' can only be "
    "performed on the MTC.");
  // return true if no PTCs exist
  if (any_component_killed_status == ALT_NO) return TRUE;
  // return false if at least one PTC has been created and
  // 'all component.killed' was successful after the create operation
  if (all_component_killed_status == ALT_YES) return FALSE;
  // the operation is successful if there is a component reference with a
  // successful killed operation
  for (int i = 0; i < component_status_table_size; i++) {
    if (component_status_table[i].killed_status == ALT_YES) return FALSE;
  }

  // the decision cannot be made locally, MC must be asked
  if (executor_state != MTC_TESTCASE) TTCN_error("Internal error: "
    "Executing 'all component.alive' in invalid state.");
  TTCN_Communication::send_is_alive(ALL_COMPREF);
  executor_state = MTC_ALIVE;
  // wait for ALIVE
  wait_for_state_change();
  return running_alive_result;
}

void TTCN_Runtime::stop_mtc()
{
  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::stopping__mtc);
  TTCN_Communication::send_stop_req(MTC_COMPREF);
  stop_execution();
}

void TTCN_Runtime::stop_ptc(component component_reference)
{
  if (is_single()) TTCN_error("Stop operation on a component reference "
    "cannot be performed in single mode.");
  // do nothing if a successful done or killed operation was performed on
  // the component reference
  if (in_component_status_table(component_reference)) {
    int index = get_component_status_table_index(component_reference);
    if (component_status_table[index].done_status == ALT_YES ||
      component_status_table[index].killed_status == ALT_YES)
      goto ignore;
  }
  // status flags all_component_done or all_component_killed cannot be used
  // because the component reference might be invalid (e.g. stale)

  // MC must be asked to stop the PTC
  switch (executor_state) {
  case MTC_TESTCASE:
    executor_state = MTC_STOP;
    break;
  case PTC_FUNCTION:
    executor_state = PTC_STOP;
    break;
  default:
    TTCN_error("Internal error: Executing component stop operation "
      "in invalid state.");
  }
  TTCN_Logger::log(TTCN_Logger::PARALLEL_UNQUALIFIED,
    "Stopping PTC with component reference %d.", component_reference);
  TTCN_Communication::send_stop_req(component_reference);
  // wait for STOP_ACK
  wait_for_state_change();
  // done status of the PTC cannot be updated because its return type and
  // return value is unknown
  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::ptc__stopped,
    NULL, NULL, component_reference);
  return;
  ignore:
  TTCN_Logger::log(TTCN_Logger::PARALLEL_UNQUALIFIED,
    "PTC with component reference %d is not running. "
    "Stop operation had no effect.", component_reference);
}

void TTCN_Runtime::stop_all_component()
{
  // do nothing in single mode
  if (is_single()) goto ignore;
  if (!is_mtc()) TTCN_error("Operation 'all component.stop' can only be "
    "performed on the MTC.");
  // do nothing if 'all component.done' or 'all component.killed'
  // was successful
  if (all_component_done_status == ALT_YES ||
    all_component_killed_status == ALT_YES) goto ignore;
  // a request must be sent to MC
  if (executor_state != MTC_TESTCASE) TTCN_error("Internal error: "
    "Executing 'all component.stop' in invalid state.");
  executor_state = MTC_STOP;
  TTCN_Logger::log_str(TTCN_Logger::PARALLEL_UNQUALIFIED, "Stopping all components.");
  TTCN_Communication::send_stop_req(ALL_COMPREF);
  // wait for STOP_ACK
  wait_for_state_change();
  // 'all component.done' will be successful later
  all_component_done_status = ALT_YES;
  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::all__comps__stopped);
  return;
  ignore:
  TTCN_Logger::log_str(TTCN_Logger::PARALLEL_UNQUALIFIED, "No PTCs are running. "
    "Operation 'all component.stop' had no effect.");
}

void TTCN_Runtime::kill_ptc(component component_reference)
{
  if (is_single()) TTCN_error("Kill operation on a component reference "
    "cannot be performed in single mode.");
  // do nothing if a successful killed operation was performed on
  // the component reference
  if (in_component_status_table(component_reference) &&
    get_killed_status(component_reference) == ALT_YES) goto ignore;
  // status flags all_component_killed cannot be used because the component
  // reference might be invalid (e.g. stale)

  // MC must be asked to kill the PTC
  switch (executor_state) {
  case MTC_TESTCASE:
    executor_state = MTC_KILL;
    break;
  case PTC_FUNCTION:
    executor_state = PTC_KILL;
    break;
  default:
    TTCN_error("Internal error: Executing kill operation in invalid "
      "state.");
  }
  TTCN_Logger::log(TTCN_Logger::PARALLEL_UNQUALIFIED,
    "Killing PTC with component reference %d.", component_reference);
  TTCN_Communication::send_kill_req(component_reference);
  // wait for KILL_ACK
  wait_for_state_change();
  // updating the killed status of the PTC
  {
    int index = get_component_status_table_index(component_reference);
    component_status_table[index].killed_status = ALT_YES;
  }
  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::ptc__killed, NULL, NULL,
    component_reference);
  return;
  ignore:
  TTCN_Logger::log(TTCN_Logger::PARALLEL_UNQUALIFIED,
    "PTC with component reference %d is not alive anymore. "
    "Kill operation had no effect.", component_reference);
}

void TTCN_Runtime::kill_all_component()
{
  // do nothing in single mode
  if (is_single()) goto ignore;
  if (!is_mtc()) TTCN_error("Operation 'all component.kill' can only be "
    "performed on the MTC.");
  // do nothing if 'all component.killed' was successful
  if (all_component_killed_status == ALT_YES) goto ignore;
  // a request must be sent to MC
  if (executor_state != MTC_TESTCASE) TTCN_error("Internal error: "
    "Executing 'all component.kill' in invalid state.");
  executor_state = MTC_KILL;
  TTCN_Logger::log_str(TTCN_Logger::PARALLEL_UNQUALIFIED, "Killing all components.");
  TTCN_Communication::send_kill_req(ALL_COMPREF);
  // wait for KILL_ACK
  wait_for_state_change();
  // 'all component.done' and 'all component.killed' will be successful later
  all_component_done_status = ALT_YES;
  all_component_killed_status = ALT_YES;
  TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::all__comps__killed);
  return;
  ignore:
  TTCN_Logger::log_str(TTCN_Logger::PARALLEL_UNQUALIFIED,
    "There are no alive PTCs. Operation 'all component.kill' had no effect.");
}

void TTCN_Runtime::check_port_name(const char *port_name,
  const char *operation_name, const char *which_argument)
{
  if (port_name == NULL)
    TTCN_error("Internal error: The port name in the %s argument of %s "
      "operation is a NULL pointer.", which_argument, operation_name);
  if (port_name[0] == '\0')
    TTCN_error("Internal error: The %s argument of %s operation contains "
      "an empty string as port name.", which_argument, operation_name);
  /** \todo check whether port_name contains a valid TTCN-3 identifier
  * (and array index) */
}

void TTCN_Runtime::connect_port(
  const COMPONENT& src_compref, const char *src_port,
  const COMPONENT& dst_compref, const char *dst_port)
{
  check_port_name(src_port, "connect", "first");
  check_port_name(dst_port, "connect", "second");

  TTCN_Logger::begin_event(TTCN_Logger::PARALLEL_UNQUALIFIED);
  TTCN_Logger::log_event_str("Connecting ports ");
  COMPONENT::log_component_reference(src_compref);
  TTCN_Logger::log_event(":%s and ", src_port);
  COMPONENT::log_component_reference(dst_compref);
  TTCN_Logger::log_event(":%s.", dst_port);
  TTCN_Logger::end_event();

  if (!src_compref.is_bound()) TTCN_error("The first argument of connect "
    "operation contains an unbound component reference.");
  component src_component = src_compref;
  switch (src_component) {
  case NULL_COMPREF:
    TTCN_error("The first argument of connect operation contains the "
      "null component reference.");
  case SYSTEM_COMPREF:
    TTCN_error("The first argument of connect operation refers to a "
      "system port.");
  default:
    break;
  }
  if (!dst_compref.is_bound()) TTCN_error("The second argument of connect "
    "operation contains an unbound component reference.");
  component dst_component = dst_compref;
  switch (dst_component) {
  case NULL_COMPREF:
    TTCN_error("The second argument of connect operation contains the "
      "null component reference.");
  case SYSTEM_COMPREF:
    TTCN_error("The second argument of connect operation refers to a "
      "system port.");
  default:
    break;
  }

  switch (executor_state) {
  case SINGLE_TESTCASE:
    if (src_component != MTC_COMPREF || dst_component != MTC_COMPREF)
      TTCN_error("Both endpoints of connect operation must refer to "
        "ports of mtc in single mode.");
    PORT::make_local_connection(src_port, dst_port);
    break;
  case MTC_TESTCASE:
    TTCN_Communication::send_connect_req(src_component, src_port,
      dst_component, dst_port);
    executor_state = MTC_CONNECT;
    wait_for_state_change();
    break;
  case PTC_FUNCTION:
    TTCN_Communication::send_connect_req(src_component, src_port,
      dst_component, dst_port);
    executor_state = PTC_CONNECT;
    wait_for_state_change();
    break;
  default:
    if (in_controlpart()) {
      TTCN_error("Connect operation cannot be performed in the "
        "control part.");
    } else {
      TTCN_error("Internal error: Executing connect operation "
        "in invalid state.");
    }
  }

  TTCN_Logger::log_portconnmap(API::ParPort_operation::connect__,
    src_compref, src_port, dst_compref, dst_port);
}

void TTCN_Runtime::disconnect_port(
  const COMPONENT& src_compref, const char *src_port,
  const COMPONENT& dst_compref, const char *dst_port)
{
  check_port_name(src_port, "disconnect", "first");
  check_port_name(dst_port, "disconnect", "second");

  TTCN_Logger::begin_event(TTCN_Logger::PARALLEL_UNQUALIFIED);
  TTCN_Logger::log_event_str("Disconnecting ports ");
  COMPONENT::log_component_reference(src_compref);
  TTCN_Logger::log_event(":%s and ", src_port);
  COMPONENT::log_component_reference(dst_compref);
  TTCN_Logger::log_event(":%s.", dst_port);
  TTCN_Logger::end_event();

  if (!src_compref.is_bound()) TTCN_error("The first argument of disconnect "
    "operation contains an unbound component reference.");
  component src_component = src_compref;
  switch (src_component) {
  case NULL_COMPREF:
    TTCN_error("The first argument of disconnect operation contains the "
      "null component reference.");
  case SYSTEM_COMPREF:
    TTCN_error("The first argument of disconnect operation refers to a "
      "system port.");
  default:
    break;
  }
  if (!dst_compref.is_bound()) TTCN_error("The second argument of disconnect "
    "operation contains an unbound component reference.");
  component dst_component = dst_compref;
  switch (dst_component) {
  case NULL_COMPREF:
    TTCN_error("The second argument of disconnect operation contains the "
      "null component reference.");
  case SYSTEM_COMPREF:
    TTCN_error("The second argument of disconnect operation refers to a "
      "system port.");
  default:
    break;
  }

  switch (executor_state) {
  case SINGLE_TESTCASE:
    if (src_component != MTC_COMPREF || dst_component != MTC_COMPREF)
      TTCN_error("Both endpoints of disconnect operation must refer to "
        "ports of mtc in single mode.");
    PORT::terminate_local_connection(src_port, dst_port);
    break;
  case MTC_TESTCASE:
    TTCN_Communication::send_disconnect_req(src_component, src_port,
      dst_component, dst_port);
    executor_state = MTC_DISCONNECT;
    wait_for_state_change();
    break;
  case PTC_FUNCTION:
    TTCN_Communication::send_disconnect_req(src_component, src_port,
      dst_component, dst_port);
    executor_state = PTC_DISCONNECT;
    wait_for_state_change();
    break;
  default:
    if (in_controlpart()) {
      TTCN_error("Disonnect operation cannot be performed in the "
        "control part.");
    } else {
      TTCN_error("Internal error: Executing disconnect operation "
        "in invalid state.");
    }
  }

  TTCN_Logger::log_portconnmap(API::ParPort_operation::disconnect__,
    src_compref, src_port, dst_compref, dst_port);
}

void TTCN_Runtime::map_port(
  const COMPONENT& src_compref, const char *src_port,
  const COMPONENT& dst_compref, const char *dst_port)
{
  check_port_name(src_port, "map", "first");
  check_port_name(dst_port, "map", "second");

  TTCN_Logger::begin_event(TTCN_Logger::PARALLEL_UNQUALIFIED);
  TTCN_Logger::log_event_str("Mapping port ");
  COMPONENT::log_component_reference(src_compref);
  TTCN_Logger::log_event(":%s to ", src_port);
  COMPONENT::log_component_reference(dst_compref);
  TTCN_Logger::log_event(":%s.", dst_port);
  TTCN_Logger::end_event();

  if (!src_compref.is_bound()) TTCN_error("The first argument of map "
    "operation contains an unbound component reference.");
  component src_component = src_compref;
  if (src_component == NULL_COMPREF) TTCN_error("The first argument of "
    "map operation contains the null component reference.");
  if (!dst_compref.is_bound()) TTCN_error("The second argument of map "
    "operation contains an unbound component reference.");
  component dst_component = dst_compref;
  if (dst_component == NULL_COMPREF) TTCN_error("The second argument of "
    "map operation contains the null component reference.");

  component comp_reference;
  const char *comp_port, *system_port;

  if (src_component == SYSTEM_COMPREF) {
    if (dst_component == SYSTEM_COMPREF) TTCN_error("Both arguments of "
      "map operation refer to system ports.");
    comp_reference = dst_component;
    comp_port = dst_port;
    system_port = src_port;
  } else if (dst_component == SYSTEM_COMPREF) {
    comp_reference = src_component;
    comp_port = src_port;
    system_port = dst_port;
  } else {
    TTCN_error("Both arguments of map operation refer to test component "
      "ports.");
    // to avoid warnings
    return;
  }

  switch (executor_state) {
  case SINGLE_TESTCASE:
    if (comp_reference != MTC_COMPREF) TTCN_error("Only the ports of mtc "
      "can be mapped in single mode.");
    PORT::map_port(comp_port, system_port);
    break;
  case MTC_TESTCASE:
    TTCN_Communication::send_map_req(comp_reference, comp_port,
      system_port);
    executor_state = MTC_MAP;
    wait_for_state_change();
    break;
  case PTC_FUNCTION:
    TTCN_Communication::send_map_req(comp_reference, comp_port,
      system_port);
    executor_state = PTC_MAP;
    wait_for_state_change();
    break;
  default:
    if (in_controlpart()) {
      TTCN_error("Map operation cannot be performed in the "
        "control part.");
    } else {
      TTCN_error("Internal error: Executing map operation "
        "in invalid state.");
    }
  }

  TTCN_Logger::log_portconnmap(API::ParPort_operation::map__,
    src_compref, src_port, dst_compref, dst_port);
}

void TTCN_Runtime::unmap_port(
  const COMPONENT& src_compref, const char *src_port,
  const COMPONENT& dst_compref, const char *dst_port)
{
  check_port_name(src_port, "unmap", "first");
  check_port_name(dst_port, "unmap", "second");

  TTCN_Logger::begin_event(TTCN_Logger::PARALLEL_UNQUALIFIED);
  TTCN_Logger::log_event_str("Unmapping port ");
  COMPONENT::log_component_reference(src_compref);
  TTCN_Logger::log_event(":%s from ", src_port);
  COMPONENT::log_component_reference(dst_compref);
  TTCN_Logger::log_event(":%s.", dst_port);
  TTCN_Logger::end_event();

  if (!src_compref.is_bound()) TTCN_error("The first argument of unmap "
    "operation contains an unbound component reference.");
  component src_component = src_compref;
  if (src_component == NULL_COMPREF) TTCN_error("The first argument of "
    "unmap operation contains the null component reference.");
  if (!dst_compref.is_bound()) TTCN_error("The second argument of unmap "
    "operation contains an unbound component reference.");
  component dst_component = dst_compref;
  if (dst_component == NULL_COMPREF) TTCN_error("The second argument of "
    "unmap operation contains the null component reference.");

  component comp_reference;
  const char *comp_port, *system_port;

  if (src_component == SYSTEM_COMPREF) {
    if (dst_component == SYSTEM_COMPREF) TTCN_error("Both arguments of "
      "unmap operation refer to system ports.");
    comp_reference = dst_component;
    comp_port = dst_port;
    system_port = src_port;
  } else if (dst_component == SYSTEM_COMPREF) {
    comp_reference = src_component;
    comp_port = src_port;
    system_port = dst_port;
  } else {
    TTCN_error("Both arguments of unmap operation refer to test component "
      "ports.");
    // to avoid warnings
    return;
  }

  switch (executor_state) {
  case SINGLE_TESTCASE:
    if (comp_reference != MTC_COMPREF) TTCN_error("Only the ports of mtc "
      "can be unmapped in single mode.");
    PORT::unmap_port(comp_port, system_port);
    break;
  case MTC_TESTCASE:
    TTCN_Communication::send_unmap_req(comp_reference, comp_port,
      system_port);
    executor_state = MTC_UNMAP;
    wait_for_state_change();
    break;
  case PTC_FUNCTION:
    TTCN_Communication::send_unmap_req(comp_reference, comp_port,
      system_port);
    executor_state = PTC_UNMAP;
    wait_for_state_change();
    break;
  default:
    if (in_controlpart()) {
      TTCN_error("Unmap operation cannot be performed in the "
        "control part.");
    } else {
      TTCN_error("Internal error: Executing unmap operation "
        "in invalid state.");
    }
  }

  TTCN_Logger::log_portconnmap(API::ParPort_operation::unmap__,
    src_compref, src_port, dst_compref, dst_port);
}

void TTCN_Runtime::begin_controlpart(const char *module_name)
{
  control_module_name = module_name;
  execute_command(begin_controlpart_command, module_name);
  TTCN_Logger::log_controlpart_start_stop(module_name, 0);
}

void TTCN_Runtime::end_controlpart()
{
  TTCN_Default::deactivate_all();
  TTCN_Default::reset_counter();
  TIMER::all_stop();
  TTCN_Logger::log_controlpart_start_stop(control_module_name, 1);
  execute_command(end_controlpart_command, control_module_name);
  control_module_name = NULL;
}

void TTCN_Runtime::check_begin_testcase(boolean has_timer, double timer_value)
{
  if (!in_controlpart()) {
    if (is_single() || is_mtc()) TTCN_error("Test case cannot be executed "
      "while another one (%s.%s) is running.", testcase_name.module_name,
      testcase_name.definition_name);
    else if (is_ptc()) TTCN_error("Test case cannot be executed on a PTC.");
    else TTCN_error("Internal error: Executing a test case in an invalid "
      "state.");
  }
  if (has_timer && timer_value < 0.0) TTCN_error("The test case supervisor "
    "timer has negative duration (%g s).", timer_value);
}

void TTCN_Runtime::begin_testcase(
  const char *par_module_name, const char *par_testcase_name,
  const char *mtc_comptype_module, const char *mtc_comptype_name,
  const char *system_comptype_module, const char *system_comptype_name,
  boolean has_timer, double timer_value)
{
  switch (executor_state) {
  case SINGLE_CONTROLPART:
    executor_state = SINGLE_TESTCASE;
    break;
  case MTC_CONTROLPART:
    TTCN_Communication::send_testcase_started(par_module_name,
      par_testcase_name, mtc_comptype_module, mtc_comptype_name,
      system_comptype_module, system_comptype_name);
    executor_state = MTC_TESTCASE;
    break;
  default:
    TTCN_error("Internal error: Executing a test case in an invalid "
      "state.");
  }
  TIMER::save_control_timers();
  TTCN_Default::save_control_defaults();
  set_testcase_name(par_module_name, par_testcase_name);
  char *command_arguments = mprintf("%s.%s", testcase_name.module_name,
    testcase_name.definition_name);
  execute_command(begin_testcase_command, command_arguments);
  Free(command_arguments);
  TTCN_Logger::log_testcase_started(testcase_name);
  if (has_timer) testcase_timer.start(timer_value);
  set_component_type(mtc_comptype_module, mtc_comptype_name);
  initialize_component_type();
  // at the beginning of the testcase no PTCs exist
  any_component_done_status = ALT_NO;
  all_component_done_status = ALT_YES;
  any_component_killed_status = ALT_NO;
  all_component_killed_status = ALT_YES;
}

verdicttype TTCN_Runtime::end_testcase()
{
  switch (executor_state) {
  case MTC_CREATE:
  case MTC_START:
  case MTC_STOP:
  case MTC_KILL:
  case MTC_RUNNING:
  case MTC_ALIVE:
  case MTC_DONE:
  case MTC_KILLED:
  case MTC_CONNECT:
  case MTC_DISCONNECT:
  case MTC_MAP:
  case MTC_UNMAP:
    executor_state = MTC_TESTCASE;
  case MTC_TESTCASE:
    break;
  case SINGLE_TESTCASE:
    disable_interrupt_handler();
    break;
  default:
    TTCN_error("Internal error: Ending a testcase in an invalid state.");
  }
  testcase_timer.stop();
  terminate_component_type();
  if (executor_state == MTC_TESTCASE) {
    TTCN_Logger::log_executor_runtime(
      API::ExecutorRuntime_reason::waiting__for__ptcs__to__finish);
    TTCN_Communication::send_testcase_finished(local_verdict, verdict_reason);
    executor_state = MTC_TERMINATING_TESTCASE;
    wait_for_state_change();
  } else if (executor_state == SINGLE_TESTCASE) {
    executor_state = SINGLE_CONTROLPART;
    enable_interrupt_handler();
  }
  TTCN_Logger::log_testcase_finished(testcase_name, local_verdict,
    verdict_reason);
  verdict_count[local_verdict]++;
  // testcase name should come first for backward compatibility
  char *command_arguments = mprintf("%s.%s %s",
    testcase_name.module_name, testcase_name.definition_name,
    verdict_name[local_verdict]);
  execute_command(end_testcase_command, command_arguments);
  Free(command_arguments);
  clear_qualified_name(testcase_name);
  // clean up component status caches
  clear_component_status_table();
  any_component_done_status = ALT_UNCHECKED;
  all_component_done_status = ALT_UNCHECKED;
  any_component_killed_status = ALT_UNCHECKED;
  all_component_killed_status = ALT_UNCHECKED;
  // restore the control part timers and defaults
  TTCN_Default::restore_control_defaults();
  TIMER::restore_control_timers();
  if (executor_state == MTC_PAUSED) {
    TTCN_Logger::log_executor_runtime(
      API::ExecutorRuntime_reason::user__paused__waiting__to__resume);
    wait_for_state_change();
    if (executor_state != MTC_TERMINATING_EXECUTION)
    TTCN_Logger::log_executor_runtime(
      API::ExecutorRuntime_reason::resuming__execution);
  }
  if (executor_state == MTC_TERMINATING_EXECUTION) {
    executor_state = MTC_CONTROLPART;
    TTCN_Logger::log_executor_runtime(
      API::ExecutorRuntime_reason::terminating__execution);
    throw TC_End();
  }
  return local_verdict;
}

void TTCN_Runtime::log_verdict_statistics()
{
  unsigned int total_testcases = verdict_count[NONE] + verdict_count[PASS] +
    verdict_count[INCONC] + verdict_count[FAIL] + verdict_count[ERROR];

  verdicttype overall_verdict;
  if (control_error_count > 0 || verdict_count[ERROR] > 0)
    overall_verdict = ERROR;
  else if (verdict_count[FAIL] > 0) overall_verdict = FAIL;
  else if (verdict_count[INCONC] > 0) overall_verdict = INCONC;
  else if (verdict_count[PASS] > 0) overall_verdict = PASS;
  else overall_verdict = NONE;

  if (total_testcases > 0) {
    TTCN_Logger::log_verdict_statistics(verdict_count[NONE], (100.0 * verdict_count[NONE]) / total_testcases,
      verdict_count[PASS], (100.0 * verdict_count[PASS]) / total_testcases,
      verdict_count[INCONC], (100.0 * verdict_count[INCONC]) / total_testcases,
      verdict_count[FAIL], (100.0 * verdict_count[FAIL]) / total_testcases,
      verdict_count[ERROR], (100.0 * verdict_count[ERROR]) / total_testcases);
  } else {
    TTCN_Logger::log_verdict_statistics(0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0);
  }

  if (control_error_count > 0) {
    TTCN_Logger::log_controlpart_errors(control_error_count);
  }

  TTCN_Logger::log(TTCN_Logger::STATISTICS_VERDICT, "Test execution summary: "
    "%u test case%s executed. Overall verdict: %s", total_testcases,
    total_testcases > 1 ? "s were" : " was", verdict_name[overall_verdict]);

  verdict_count[NONE] = 0;
  verdict_count[PASS] = 0;
  verdict_count[INCONC] = 0;
  verdict_count[FAIL] = 0;
  verdict_count[ERROR] = 0;
  control_error_count = 0;
}

void TTCN_Runtime::begin_action()
{
  TTCN_Logger::begin_event(TTCN_Logger::ACTION_UNQUALIFIED);
  TTCN_Logger::log_event_str("Action: ");
}

void TTCN_Runtime::end_action()
{
  TTCN_Logger::end_event();
}

void TTCN_Runtime::setverdict(verdicttype new_value, const char* reason)
{
  if (verdict_enabled()) {
    if (new_value == ERROR)
      TTCN_error("Error verdict cannot be set explicitly.");
    setverdict_internal(new_value, reason);
  } else if (in_controlpart()) {
    TTCN_error("Verdict cannot be set in the control part.");
  } else {
    TTCN_error("Internal error: Setting the verdict in invalid state.");
  }
}

void TTCN_Runtime::setverdict(const VERDICTTYPE& new_value, const char* reason)
{
  if (!new_value.is_bound()) TTCN_error("The argument of setverdict "
    "operation is an unbound verdict value.");
  setverdict((verdicttype)new_value, reason);
}

void TTCN_Runtime::set_error_verdict()
{
  if (verdict_enabled()) setverdict_internal(ERROR);
  else if (is_single() || is_mtc()) control_error_count++;
}

verdicttype TTCN_Runtime::getverdict()
{
  if (verdict_enabled())
    TTCN_Logger::log_getverdict(local_verdict);
  else if (in_controlpart()) TTCN_error("Getverdict operation cannot be "
    "performed in the control part.");
  else TTCN_error("Internal error: Performing getverdict operation in "
    "invalid state.");
  return local_verdict;
}

void TTCN_Runtime::setverdict_internal(verdicttype new_value,
                                       const char* reason)
{
  if (new_value < NONE || new_value > ERROR)
    TTCN_error("Internal error: setting an invalid verdict value (%d).",
               new_value);
  verdicttype old_verdict = local_verdict;
  if (local_verdict < new_value) {
    verdict_reason = reason;
    local_verdict = new_value;
    if (reason == NULL || reason[0] == '\0')
      TTCN_Logger::log_setverdict(new_value, old_verdict, local_verdict);
    else TTCN_Logger::log_setverdict(new_value, old_verdict, local_verdict, reason, reason);
  } else if (local_verdict == new_value) {
    if (reason == NULL || reason[0] == '\0')
      TTCN_Logger::log_setverdict(new_value, old_verdict, local_verdict);
    else TTCN_Logger::log_setverdict(new_value, old_verdict, local_verdict, reason, reason);
  }
  if (new_value == FAIL) {
    ttcn3_debugger.breakpoint_entry(TTCN3_Debugger::SBP_FAIL_VERDICT);
  }
  else if (new_value == ERROR) {
    ttcn3_debugger.breakpoint_entry(TTCN3_Debugger::SBP_ERROR_VERDICT);
  }
}

void TTCN_Runtime::set_begin_controlpart_command(const char *new_command)
{
  Free(begin_controlpart_command);
  begin_controlpart_command = shell_escape(new_command);
}

void TTCN_Runtime::set_end_controlpart_command(const char *new_command)
{
  Free(end_controlpart_command);
  end_controlpart_command = shell_escape(new_command);
}

void TTCN_Runtime::set_begin_testcase_command(const char *new_command)
{
  Free(begin_testcase_command);
  begin_testcase_command = shell_escape(new_command);
}

void TTCN_Runtime::set_end_testcase_command(const char *new_command)
{
  Free(end_testcase_command);
  end_testcase_command = shell_escape(new_command);
}

void TTCN_Runtime::clear_external_commands()
{
  Free(begin_controlpart_command);
  begin_controlpart_command = NULL;
  Free(end_controlpart_command);
  end_controlpart_command = NULL;
  Free(begin_testcase_command);
  begin_testcase_command = NULL;
  Free(end_testcase_command);
  end_testcase_command = NULL;
}

char *TTCN_Runtime::shell_escape(const char *command_str)
{
  if (command_str == NULL || command_str[0] == '\0') return NULL;
  boolean has_special_char = FALSE;
  for (int i = 0; !has_special_char && command_str[i] != '\0'; i++) {
    switch (command_str[i]) {
    case ' ':
    case '*':
    case '?':
    case '[':
    case ']':
    case '<':
    case '>':
    case '|':
    case '&':
    case '$':
    case '{':
    case '}':
    case ';':
    case '(':
    case ')':
    case '#':
    case '!':
    case '=':
    case '"':
    case '`':
    case '\\':
      // special characters interpreted by the shell except '
      has_special_char = TRUE;
      break;
    default:
      // non-printable characters also need special handling
      if (!isprint(command_str[i])) has_special_char = TRUE;
    }
  }
  char *ret_val = memptystr();
  // indicates whether we are in an unclosed ' string
  boolean in_apostrophes = FALSE;
  for (int i = 0; command_str[i] != '\0'; i++) {
    if (command_str[i] == '\'') {
      if (in_apostrophes) {
        // close the open literal
        ret_val = mputc(ret_val, '\'');
        in_apostrophes = FALSE;
      }
      // substitute with \'
      ret_val = mputstr(ret_val, "\\'");
    } else {
      if (has_special_char && !in_apostrophes) {
        // open the literal
        ret_val = mputc(ret_val, '\'');
        in_apostrophes = TRUE;
      }
      // append the single character
      ret_val = mputc(ret_val, command_str[i]);
    }
  }
  // close the open literal
  if (in_apostrophes) ret_val = mputc(ret_val, '\'');
  return ret_val;
}

void TTCN_Runtime::execute_command(const char *command_name,
  const char *argument_string)
{
  if (command_name != NULL) {
    char *command_string = mprintf("%s %s", command_name, argument_string);
    try {
      TTCN_Logger::log_extcommand(TTCN_Logger::EXTCOMMAND_START, command_string);
      int return_status = system(command_string);
      if (return_status == -1) TTCN_error("Execution of external "
        "command `%s' failed.", command_string);
      else if (WIFEXITED(return_status)) {
        int exit_status = WEXITSTATUS(return_status);
        if (exit_status == EXIT_SUCCESS)
          TTCN_Logger::log_extcommand(TTCN_Logger::EXTCOMMAND_SUCCESS, command_string);
        else TTCN_warning("External command `%s' returned "
          "unsuccessful exit status (%d).", command_string,
          exit_status);
      } else if (WIFSIGNALED(return_status)) {
        int signal_number = WTERMSIG(return_status);
        TTCN_warning("External command `%s' was terminated by signal "
          "%d (%s).", command_string, signal_number,
          get_signal_name(signal_number));
      } else {
        TTCN_warning("External command `%s' was terminated by an "
          "unknown reason (return status: %d).", command_string,
          return_status);
      }
    } catch (...) {
      // to prevent from memory leaks
      Free(command_string);
      throw;
    }
    Free(command_string);
  }
}

void TTCN_Runtime::process_create_mtc()
{
  switch (executor_state) {
  case HC_ACTIVE:
  case HC_OVERLOADED:
    break;
  default:
    TTCN_Communication::send_error("Message CREATE_MTC arrived in invalid "
      "state.");
    return;
  }

  // clean Emergency log buffer before fork, to avoid duplication
  TTCN_Logger::ring_buffer_dump(FALSE);

  pid_t mtc_pid = fork();
  if (mtc_pid < 0) {
    // fork() failed
    TTCN_Communication::send_create_nak(MTC_COMPREF, "system call fork() "
      "failed (%s)", strerror(errno));
    failed_process_creation();
    TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
    TTCN_Logger::log_event_str("System call fork() failed when creating "
      "MTC.");
    TTCN_Logger::OS_error();
    TTCN_Logger::end_event();
  } else if (mtc_pid > 0) {
    // fork() was successful, this code runs on the parent process (HC)
    TTCN_Logger::log_mtc_created(mtc_pid);
    add_component(MTC_COMPREF, mtc_pid);
    successful_process_creation();
    // let the HC's TTCN-3 Profiler know of the MTC
    ttcn3_prof.add_child_process(mtc_pid);
  } else {
    // fork() was successful, this code runs on the child process (MTC)
    // The inherited epoll fd has to be closed first, and then the mc fd
    // (The inherited epoll fd shares its database with the parent process.)
    Fd_And_Timeout_User::reopenEpollFd();
    TTCN_Communication::close_mc_connection();
    self = MTC_COMPREF;
    executor_state = MTC_INITIAL;
  }
}

void TTCN_Runtime::process_create_ptc(component component_reference,
  const char *component_type_module, const char *component_type_name,
  const char *par_component_name, boolean par_is_alive,
  const char *current_testcase_module, const char *current_testcase_name)
{
  switch (executor_state) {
  case HC_ACTIVE:
  case HC_OVERLOADED:
    break;
  default:
    TTCN_Communication::send_error("Message CREATE_PTC arrived in invalid "
      "state.");
    return;
  }

  // clean Emergency log buffer before fork, to avoid duplication
  TTCN_Logger::ring_buffer_dump(FALSE);

  pid_t ptc_pid = fork();
  if (ptc_pid < 0) {
    // fork() failed
    TTCN_Communication::send_create_nak(component_reference, "system call "
      "fork() failed (%s)", strerror(errno));
    failed_process_creation();
    TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
    TTCN_Logger::log_event("System call fork() failed when creating PTC "
      "with component reference %d.", component_reference);
    TTCN_Logger::OS_error();
    TTCN_Logger::end_event();
  } else if (ptc_pid > 0) {
    // fork() was successful, this code runs on the parent process (HC)
    TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::ptc__created__pid,
      component_type_module, component_type_name, component_reference,
      par_component_name, current_testcase_name, ptc_pid);
    add_component(component_reference, ptc_pid);
    COMPONENT::register_component_name(component_reference,
      par_component_name);
    successful_process_creation();
    // let the HC's TTCN-3 Profiler know of this new PTC
    ttcn3_prof.add_child_process(ptc_pid);
  } else {
    // fork() was successful, this code runs on the child process (PTC)
    // The inherited epoll fd has to be closed first, and then the mc fd
    // (The inherited epoll fd shares its database with the parent process.)
    Fd_And_Timeout_User::reopenEpollFd();
    TTCN_Communication::close_mc_connection();
    self = component_reference;
    set_component_type(component_type_module, component_type_name);
    set_component_name(par_component_name);
    is_alive = par_is_alive;
    set_testcase_name(current_testcase_module, current_testcase_name);
    executor_state = PTC_INITIAL;
  }
}

void TTCN_Runtime::process_create_ack(component new_component)
{
  switch (executor_state) {
  case MTC_CREATE:
    executor_state = MTC_TESTCASE;
  case MTC_TERMINATING_TESTCASE:
    break;
  case PTC_CREATE:
    executor_state = PTC_FUNCTION;
    break;
  default:
    TTCN_error("Internal error: Message CREATE_ACK arrived in invalid "
      "state.");
  }
  create_done_killed_compref = new_component;
}

void TTCN_Runtime::process_running(boolean result_value)
{
  switch (executor_state) {
  case MTC_RUNNING:
    executor_state = MTC_TESTCASE;
  case MTC_TERMINATING_TESTCASE:
    break;
  case PTC_RUNNING:
    executor_state = PTC_FUNCTION;
    break;
  default:
    TTCN_error("Internal error: Message RUNNING arrived in invalid state.");
  }
  running_alive_result = result_value;
}

void TTCN_Runtime::process_alive(boolean result_value)
{
  switch (executor_state) {
  case MTC_ALIVE:
    executor_state = MTC_TESTCASE;
  case MTC_TERMINATING_TESTCASE:
    break;
  case PTC_ALIVE:
    executor_state = PTC_FUNCTION;
    break;
  default:
    TTCN_error("Internal error: Message ALIVE arrived in invalid state.");
  }
  running_alive_result = result_value;
}

void TTCN_Runtime::process_done_ack(boolean done_status,
  const char *return_type, int return_value_len, const void *return_value)
{
  switch (executor_state) {
  case MTC_DONE:
    executor_state = MTC_TESTCASE;
  case MTC_TERMINATING_TESTCASE:
    break;
  case PTC_DONE:
    executor_state = PTC_FUNCTION;
    break;
  default:
    TTCN_error("Internal error: Message DONE_ACK arrived in invalid "
      "state.");
  }
  if (done_status) set_component_done(create_done_killed_compref,
    return_type, return_value_len, return_value);
  create_done_killed_compref = NULL_COMPREF;
}

void TTCN_Runtime::process_killed_ack(boolean killed_status)
{
  switch (executor_state) {
  case MTC_KILLED:
    executor_state = MTC_TESTCASE;
  case MTC_TERMINATING_TESTCASE:
    break;
  case PTC_KILLED:
    executor_state = PTC_FUNCTION;
    break;
  default:
    TTCN_error("Internal error: Message KILLED_ACK arrived in invalid "
      "state.");
  }
  if (killed_status) set_component_killed(create_done_killed_compref);
  create_done_killed_compref = NULL_COMPREF;
}

void TTCN_Runtime::process_ptc_verdict(Text_Buf& text_buf)
{
  if (executor_state != MTC_TERMINATING_TESTCASE)
    TTCN_error("Internal error: Message PTC_VERDICT arrived in invalid state.");

  TTCN_Logger::log_final_verdict(FALSE, local_verdict, local_verdict,
    local_verdict, (const char *)verdict_reason,
    TitanLoggerApi::FinalVerdictType_choice_notification::setting__final__verdict__of__the__test__case);
  TTCN_Logger::log_final_verdict(FALSE, local_verdict, local_verdict,
                                 local_verdict, (const char *)verdict_reason);
  int n_ptcs = text_buf.pull_int().get_val();
  if (n_ptcs > 0) {
    for (int i = 0; i < n_ptcs; i++) {
      component ptc_compref = text_buf.pull_int().get_val();
      char *ptc_name = text_buf.pull_string();
      verdicttype ptc_verdict = (verdicttype)text_buf.pull_int().get_val();
      char *ptc_verdict_reason = text_buf.pull_string();
      if (ptc_verdict < NONE || ptc_verdict > ERROR) {
        delete [] ptc_name;
        TTCN_error("Internal error: Invalid PTC verdict was "
          "received from MC: %d.", ptc_verdict);
      }
      verdicttype new_verdict = local_verdict;
      if (ptc_verdict > local_verdict) {
        new_verdict = ptc_verdict;
        verdict_reason = CHARSTRING(ptc_verdict_reason);
      }
      TTCN_Logger::log_final_verdict(TRUE, ptc_verdict, local_verdict,
        new_verdict, ptc_verdict_reason, -1, ptc_compref, ptc_name);
      delete [] ptc_name;
      delete [] ptc_verdict_reason;
      local_verdict = new_verdict;
    }
  } else {
    TTCN_Logger::log_final_verdict(FALSE, local_verdict, local_verdict,
      local_verdict, (const char *)verdict_reason,
      TitanLoggerApi::FinalVerdictType_choice_notification::no__ptcs__were__created);
  }

  boolean continue_execution = (boolean)text_buf.pull_int().get_val();
  if (continue_execution) executor_state = MTC_CONTROLPART;
  else executor_state = MTC_PAUSED;
}

void TTCN_Runtime::process_kill()
{
  if (!is_ptc())
    TTCN_error("Internal error: Message KILL arrived in invalid state.");
  switch (executor_state) {
  case PTC_IDLE:
  case PTC_STOPPED:
    TTCN_Logger::log_par_ptc(API::ParallelPTC_reason::kill__request__frm__mc);
    // This may affect the final verdict.
    terminate_component_type();
    // Send a KILLED message so that the value returned by previous behaviour
    // function remains active.
    TTCN_Communication::send_killed(local_verdict);
    TTCN_Logger::log_final_verdict(TRUE, local_verdict, local_verdict,
                                   local_verdict, (const char *)verdict_reason);
    executor_state = PTC_EXIT;
  case PTC_EXIT:
    break;
  default:
    TTCN_Logger::log_str(TTCN_Logger::PARALLEL_UNQUALIFIED,
                         "Kill was requested from MC.");
    kill_execution();
  }
}

void TTCN_Runtime::process_kill_process(component component_reference)
{
  if (!is_hc()) TTCN_error("Internal error: Message KILL_PROCESS arrived "
    "in invalid state.");
  component_process_struct *comp =
    get_component_by_compref(component_reference);
  if (comp != NULL) {
    TTCN_Logger::log(TTCN_Logger::PARALLEL_UNQUALIFIED,
      "Killing component with component reference %d, process id: %ld.",
      component_reference, (long)comp->process_id);
    if (comp->process_killed) TTCN_warning("Process with process id %ld "
      "has been already killed. Killing it again.",
      (long)comp->process_id);
    if (kill(comp->process_id, SIGKILL)) {
      if (errno == ESRCH) {
        errno = 0;
        TTCN_Logger::log(TTCN_Logger::PARALLEL_UNQUALIFIED,
          "Process with process id %ld has already terminated.", (long)comp->process_id);
      } else TTCN_error("kill() system call failed on process id %ld.",
        (long)comp->process_id);
    }
    comp->process_killed = TRUE;
  } else {
    TTCN_Logger::log(TTCN_Logger::PARALLEL_UNQUALIFIED,
      "Component with component reference %d does not exist. "
      "Request for killing was ignored.", component_reference);
  }
}

void TTCN_Runtime::set_component_done(component component_reference,
  const char *return_type, int return_value_len,
  const void *return_value)
{
  switch (component_reference) {
  case ANY_COMPREF:
    if (is_mtc()) any_component_done_status = ALT_YES;
    else TTCN_error("Internal error: TTCN_Runtime::set_component_done("
      "ANY_COMPREF): can be used only on MTC.");
    break;
  case ALL_COMPREF:
    if (is_mtc()) all_component_done_status = ALT_YES;
    else TTCN_error("Internal error: TTCN_Runtime::set_component_done("
      "ALL_COMPREF): can be used only on MTC.");
    break;
  case NULL_COMPREF:
  case MTC_COMPREF:
  case SYSTEM_COMPREF:
    TTCN_error("Internal error: TTCN_Runtime::set_component_done: "
      "invalid component reference: %d.", component_reference);
    break;
  default: {
    int index = get_component_status_table_index(component_reference);
    component_status_table[index].done_status = ALT_YES;
    Free(component_status_table[index].return_type);
    delete component_status_table[index].return_value;
    if (return_type != NULL && return_type[0] != '\0') {
      component_status_table[index].return_type = mcopystr(return_type);
      component_status_table[index].return_value = new Text_Buf;
      component_status_table[index].return_value->push_raw(
        return_value_len, return_value);
    } else {
      component_status_table[index].return_type = NULL;
      component_status_table[index].return_value = NULL;
    }
  }
  }
}

void TTCN_Runtime::set_component_killed(component component_reference)
{
  switch (component_reference) {
  case ANY_COMPREF:
    if (is_mtc()) any_component_killed_status = ALT_YES;
    else TTCN_error("Internal error: TTCN_Runtime::set_component_killed("
      "ANY_COMPREF): can be used only on MTC.");
    break;
  case ALL_COMPREF:
    if (is_mtc()) all_component_killed_status = ALT_YES;
    else TTCN_error("Internal error: TTCN_Runtime::set_component_killed("
      "ALL_COMPREF): can be used only on MTC.");
    break;
  case NULL_COMPREF:
  case MTC_COMPREF:
  case SYSTEM_COMPREF:
    TTCN_error("Internal error: TTCN_Runtime::set_component_killed: "
      "invalid component reference: %d.", component_reference);
  default:
    component_status_table[get_component_status_table_index(
      component_reference)].killed_status = ALT_YES;
  }
}

void TTCN_Runtime::cancel_component_done(component component_reference)
{
  switch (component_reference) {
  case ANY_COMPREF:
    if (is_mtc()) any_component_done_status = ALT_UNCHECKED;
    else TTCN_error("Internal error: TTCN_Runtime::cancel_component_done("
      "ANY_COMPREF): can be used only on MTC.");
    break;
  case ALL_COMPREF:
  case NULL_COMPREF:
  case MTC_COMPREF:
  case SYSTEM_COMPREF:
    TTCN_error("Internal error: TTCN_Runtime::cancel_component_done: "
      "invalid component reference: %d.", component_reference);
  default:
    if (in_component_status_table(component_reference)) {
      int index = get_component_status_table_index(component_reference);
      component_status_table[index].done_status = ALT_UNCHECKED;
      Free(component_status_table[index].return_type);
      component_status_table[index].return_type = NULL;
      delete component_status_table[index].return_value;
      component_status_table[index].return_value = NULL;
    }
  }
}

int TTCN_Runtime::get_component_status_table_index(
  component component_reference)
{
  if (component_reference < FIRST_PTC_COMPREF) {
    TTCN_error("Internal error: TTCN_Runtime::"
      "get_component_status_table_index: invalid component reference: "
      "%d.", component_reference);
  }
  if (component_status_table_size == 0) {
    // the table is empty
    // this will be the first entry
    component_status_table = (component_status_table_struct*)
	      Malloc(sizeof(*component_status_table));
    component_status_table[0].done_status = ALT_UNCHECKED;
    component_status_table[0].killed_status = ALT_UNCHECKED;
    component_status_table[0].return_type = NULL;
    component_status_table[0].return_value = NULL;
    component_status_table_size = 1;
    component_status_table_offset = component_reference;
    return 0;
  } else if (component_reference >= component_status_table_offset) {
    // the table contains at least one entry that is smaller than
    // component_reference
    int component_index =
      component_reference - component_status_table_offset;
    if (component_index >= component_status_table_size) {
      // component_reference is still not in the table
      // the table has to be extended at the end
      component_status_table = (component_status_table_struct*)
		  Realloc(component_status_table,
		    (component_index + 1) * sizeof(*component_status_table));
      // initializing the new table entries at the end
      for (int i = component_status_table_size;
        i <= component_index; i++) {
        component_status_table[i].done_status = ALT_UNCHECKED;
        component_status_table[i].killed_status = ALT_UNCHECKED;
        component_status_table[i].return_type = NULL;
        component_status_table[i].return_value = NULL;
      }
      component_status_table_size = component_index + 1;
    }
    return component_index;
  } else {
    // component_reference has to be inserted before the existing table
    int offset_diff = component_status_table_offset - component_reference;
    // offset_diff indicates how many new elements have to be inserted
    // before the existing table
    int new_size = component_status_table_size + offset_diff;
    component_status_table = (component_status_table_struct*)
	      Realloc(component_status_table,
	        new_size * sizeof(*component_status_table));
    // moving forward the existing table
    memmove(component_status_table + offset_diff, component_status_table,
      component_status_table_size * sizeof(*component_status_table));
    // initializing the first table entries
    for (int i = 0; i < offset_diff; i++) {
      component_status_table[i].done_status = ALT_UNCHECKED;
      component_status_table[i].killed_status = ALT_UNCHECKED;
      component_status_table[i].return_type = NULL;
      component_status_table[i].return_value = NULL;
    }
    component_status_table_size = new_size;
    component_status_table_offset = component_reference;
    return 0;
  }
}

alt_status TTCN_Runtime::get_killed_status(component component_reference)
{
  return component_status_table
  [get_component_status_table_index(component_reference)].killed_status;
}

boolean TTCN_Runtime::in_component_status_table(component component_reference)
{
  return component_reference >= component_status_table_offset &&
  component_reference <
  component_status_table_size + component_status_table_offset;
}

void TTCN_Runtime::clear_component_status_table()
{
  for (component i = 0; i < component_status_table_size; i++) {
    Free(component_status_table[i].return_type);
    delete component_status_table[i].return_value;
  }
  Free(component_status_table);
  component_status_table = NULL;
  component_status_table_size = 0;
  component_status_table_offset = FIRST_PTC_COMPREF;
}

#define HASHTABLE_SIZE 97

void TTCN_Runtime::initialize_component_process_tables()
{
  components_by_compref = new component_process_struct*[HASHTABLE_SIZE];
  components_by_pid = new component_process_struct*[HASHTABLE_SIZE];
  for (unsigned int i = 0; i < HASHTABLE_SIZE; i++) {
    components_by_compref[i] = NULL;
    components_by_pid[i] = NULL;
  }
}

void TTCN_Runtime::add_component(component component_reference,
  pid_t process_id)
{
  if (component_reference != MTC_COMPREF &&
    get_component_by_compref(component_reference) != NULL)
    TTCN_error("Internal error: TTCN_Runtime::add_component: "
      "duplicated component reference (%d)", component_reference);
  if (get_component_by_pid(process_id) != NULL)
    TTCN_error("Internal error: TTCN_Runtime::add_component: "
      "duplicated pid (%ld)", (long)process_id);

  component_process_struct *new_comp = new component_process_struct;
  new_comp->component_reference = component_reference;
  new_comp->process_id = process_id;
  new_comp->process_killed = FALSE;

  new_comp->prev_by_compref = NULL;
  component_process_struct*& head_by_compref =
    components_by_compref[component_reference % HASHTABLE_SIZE];
  new_comp->next_by_compref = head_by_compref;
  if (head_by_compref != NULL) head_by_compref->prev_by_compref = new_comp;
  head_by_compref = new_comp;

  new_comp->prev_by_pid = NULL;
  component_process_struct*& head_by_pid =
    components_by_pid[process_id % HASHTABLE_SIZE];
  new_comp->next_by_pid = head_by_pid;
  if (head_by_pid != NULL) head_by_pid->prev_by_pid = new_comp;
  head_by_pid = new_comp;
}

void TTCN_Runtime::remove_component(component_process_struct *comp)
{
  if (comp->next_by_compref != NULL)
    comp->next_by_compref->prev_by_compref = comp->prev_by_compref;
  if (comp->prev_by_compref != NULL)
    comp->prev_by_compref->next_by_compref = comp->next_by_compref;
  else components_by_compref[comp->component_reference % HASHTABLE_SIZE] =
    comp->next_by_compref;
  if (comp->next_by_pid != NULL)
    comp->next_by_pid->prev_by_pid = comp->prev_by_pid;
  if (comp->prev_by_pid != NULL)
    comp->prev_by_pid->next_by_pid = comp->next_by_pid;
  else components_by_pid[comp->process_id % HASHTABLE_SIZE] =
    comp->next_by_pid;
  delete comp;
}

TTCN_Runtime::component_process_struct *TTCN_Runtime::get_component_by_compref(
  component component_reference)
{
  component_process_struct *iter =
    components_by_compref[component_reference % HASHTABLE_SIZE];
  while (iter != NULL) {
    if (iter->component_reference == component_reference) break;
    iter = iter->next_by_compref;
  }
  return iter;
}

TTCN_Runtime::component_process_struct *TTCN_Runtime::get_component_by_pid(
  pid_t process_id)
{
  component_process_struct *iter =
    components_by_pid[process_id % HASHTABLE_SIZE];
  while (iter != NULL) {
    if (iter->process_id == process_id) break;
    iter = iter->next_by_pid;
  }
  return iter;
}

void TTCN_Runtime::clear_component_process_tables()
{
  if (components_by_compref == NULL) return;
  for (unsigned int i = 0; i < HASHTABLE_SIZE; i++) {
    while (components_by_compref[i] != NULL)
      remove_component(components_by_compref[i]);
    while (components_by_pid[i] != NULL)
      remove_component(components_by_pid[i]);
  }
  delete [] components_by_compref;
  components_by_compref = NULL;
  delete [] components_by_pid;
  components_by_pid = NULL;
}

void TTCN_Runtime::successful_process_creation()
{
  if (is_overloaded()) {
    TTCN_Communication::send_hc_ready();
    TTCN_Communication::disable_periodic_call();
    executor_state = HC_ACTIVE;
  }
}

void TTCN_Runtime::failed_process_creation()
{
  if (executor_state == HC_ACTIVE) {
    TTCN_Communication::enable_periodic_call();
    executor_state = HC_OVERLOADED;
  }
}

void TTCN_Runtime::wait_terminated_processes()
{
  // this function might be called from TCs too while returning from
  // TTCN_Communication::process_all_messages_hc() after fork()
  if (!is_hc()) return;
  errno = 0;
  for ( ; ; ) {
    int statuscode;
#ifdef __clang__
    struct rusage r_usage = {{0,0},{0,0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}};
#else
    struct rusage r_usage = {{0,0},{0,0},0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#endif

#ifdef INTERIX
    pid_t child_pid = waitpid(-1, &statuscode, WNOHANG);
    getrusage(RUSAGE_CHILDREN, &r_usage);
#else
    pid_t child_pid = wait3(&statuscode, WNOHANG, &r_usage);
#endif
    if (child_pid <= 0) {
      switch (errno) {
      case ECHILD:
        errno = 0;
        // no break
      case 0:
        return;
      default:
        TTCN_error("System call wait3() failed when waiting for "
          "terminated test component processes.");
      }
    }
    component_process_struct *comp = get_component_by_pid(child_pid);
    if (comp != NULL) {
      int reason;
      const char *comp_name = NULL;
      if (comp->component_reference == MTC_COMPREF) {
        reason = API::ParallelPTC_reason::mtc__finished;
      }
      else {
        reason = API::ParallelPTC_reason::ptc__finished;
        comp_name = COMPONENT::get_component_name(comp->component_reference);
      }
      char *rusage = NULL;
      rusage = mprintf(
        "user time: %ld.%06ld s, system time: %ld.%06ld s, "
        "maximum resident set size: %ld, "
        "integral resident set size: %ld, "
        "page faults not requiring physical I/O: %ld, "
        "page faults requiring physical I/O: %ld, "
        "swaps: %ld, "
        "block input operations: %ld, block output operations: %ld, "
        "messages sent: %ld, messages received: %ld, "
        "signals received: %ld, "
        "voluntary context switches: %ld, "
        "involuntary context switches: %ld }",
        (long)r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec,
        (long)r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec,
        r_usage.ru_maxrss, r_usage.ru_idrss,
        r_usage.ru_minflt, r_usage.ru_majflt, r_usage.ru_nswap,
        r_usage.ru_inblock, r_usage.ru_oublock,
        r_usage.ru_msgsnd, r_usage.ru_msgrcv, r_usage.ru_nsignals,
        r_usage.ru_nvcsw, r_usage.ru_nivcsw);
      // There are too many different integer types in the rusage structure.
      // Just format them into a string and and pass that to the logger.
      TTCN_Logger::log_par_ptc(reason, NULL, NULL,
        comp->component_reference, comp_name, rusage, child_pid, statuscode);
      Free(rusage);
      remove_component(comp);
    } else {
      TTCN_warning("wait3() system call returned unknown process id %ld.",
        (long)child_pid);
    }
  }
}

void TTCN_Runtime::check_overload()
{
  if (!is_hc()) TTCN_error("Internal error: TTCN_Runtime::check_overload() "
    "can be used on HCs only.");
  if (!is_overloaded()) return;
    TTCN_Logger::log_executor_runtime(
      API::ExecutorRuntime_reason::overload__check);
  pid_t child_pid = fork();
  if (child_pid < 0) {
    // fork failed, the host is still overloaded
    TTCN_Logger::log_executor_runtime(
      API::ExecutorRuntime_reason::overload__check__fail);
    //TODO TTCN_Logger::OS_error();
    if (executor_state == HC_OVERLOADED_TIMEOUT) {
      // increase the call interval if the function was called because of
      // a timeout
      TTCN_Communication::increase_call_interval();
      executor_state = HC_OVERLOADED;
    }
  } else if (child_pid > 0) {
    // fork was successful, this code runs on the parent process (HC)
    int statuscode;
    // wait until the dummy child terminates
    pid_t result_pid = waitpid(child_pid, &statuscode, 0);
    if (result_pid != child_pid) TTCN_error("System call waitpid() "
      "returned unexpected status code %ld when waiting for the dummy "
      "child process with PID %ld.", (long)result_pid, (long)child_pid);
    successful_process_creation();
    TTCN_Logger::log_executor_runtime(
      API::ExecutorRuntime_reason::overloaded__no__more);
    // FIXME pid is not logged; it would need a separate function

    // analyze the status code and issue a warning if something strange
    // happened
    if (WIFEXITED(statuscode)) {
      int exitstatus = WEXITSTATUS(statuscode);
      if (exitstatus != EXIT_SUCCESS) TTCN_warning("Dummy child process "
        "with PID %ld returned unsuccessful exit status (%d).",
        (long)child_pid, exitstatus);
    } else if (WIFSIGNALED(statuscode)) {
      int signum = WTERMSIG(statuscode);
      TTCN_warning("Dummy child process with PID %ld was terminated by "
        "signal %d (%s).", (long)child_pid, signum,
        get_signal_name(signum));
    } else {
      TTCN_warning("Dummy child process with PID %ld was terminated by "
        "an unknown reason (return status: %d).", (long)child_pid,
        statuscode);
    }
    // try to clean up some more zombies if possible
    wait_terminated_processes();
  } else {
    // fork was successful, this code runs on the dummy child process
    // the dummy child process shall exit immediately
    exit(EXIT_SUCCESS);
  }
}
