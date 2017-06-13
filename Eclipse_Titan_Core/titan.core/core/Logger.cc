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
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#include "../common/memory.h"
#include "Logger.hh"
#include "Communication.hh"
#include "Component.hh"
#include "LoggingBits.hh"
#include "LoggerPluginManager.hh"
#include "Charstring.hh"
#include "LoggingParam.hh"
#include "TCov.hh"

/** work-around for missing va_copy() in GCC */
#if defined(__GNUC__) && !defined(va_copy)
# ifdef __va_copy
#  define va_copy(dest, src) __va_copy(dest, src)
# else
#  define va_copy(dest, src) (dest) = (src)
# endif
#endif

#define MIN_BUFFER_SIZE 1024


/** @brief Return a string identifying the component

If \p comp.id_selector is COMPONENT_ID_NAME, returns \p comp.id_name

If \p comp.id_selector is COMPONENT_ID_COMPREF, returns a pointer
to a static buffer, containing a string representation of \p comp.id_compref

If \p comp.id_selector is COMPONENT_ID_ALL, returns "All".

If \p comp.id_selector is COMPONENT_ID_SYSTEM, returns "System".

@param comp component identifier
@return string
*/
expstring_t component_string(const component_id_t& comp);


/** @name Private data structures
    @{
*/

/** Log mask

For TTCN_Logger internal use, no user serviceable parts.
*/
struct TTCN_Logger::log_mask_struct
{
    component_id_t   component_id;  /**< Component */
    Logging_Bits     mask;          /**< Settings for logging to console or file */
};

/** @} */

class LogEventType;

/* Static members */

LoggerPluginManager *TTCN_Logger::plugins_ = NULL;

/* No point in initializing here; will be done in initialize_logger() */
TTCN_Logger::log_mask_struct TTCN_Logger::console_log_mask;
TTCN_Logger::log_mask_struct TTCN_Logger::file_log_mask;
TTCN_Logger::log_mask_struct TTCN_Logger::emergency_log_mask;

TTCN_Logger::emergency_logging_behaviour_t TTCN_Logger::emergency_logging_behaviour = BUFFER_MASKED;
size_t TTCN_Logger::emergency_logging;
boolean TTCN_Logger::emergency_logging_for_fail_verdict = FALSE;

TTCN_Logger::matching_verbosity_t TTCN_Logger::matching_verbosity = VERBOSITY_COMPACT;

/** Timestamp format in the log files. */
TTCN_Logger::timestamp_format_t TTCN_Logger::timestamp_format = TIMESTAMP_TIME;

/** LogSourceInfo/SourceInfoFormat */
TTCN_Logger::source_info_format_t TTCN_Logger::source_info_format = SINFO_SINGLE;

/** LogEventTypes */
TTCN_Logger::log_event_types_t TTCN_Logger::log_event_types = LOGEVENTTYPES_NO;

// This array should contain all the _UNQUALIFIED severities.
const TTCN_Logger::Severity TTCN_Logger::sev_categories[] =
{
  TTCN_Logger::NOTHING_TO_LOG, // 0
  TTCN_Logger::ACTION_UNQUALIFIED, // 1
  TTCN_Logger::DEFAULTOP_UNQUALIFIED, // 5
  TTCN_Logger::ERROR_UNQUALIFIED, // 6
  TTCN_Logger::EXECUTOR_UNQUALIFIED, // 12
  TTCN_Logger::FUNCTION_UNQUALIFIED, // 14
  TTCN_Logger::PARALLEL_UNQUALIFIED, // 18
  TTCN_Logger::TESTCASE_UNQUALIFIED, // 21
  TTCN_Logger::PORTEVENT_UNQUALIFIED, // 35
  TTCN_Logger::STATISTICS_UNQUALIFIED, // 37
  TTCN_Logger::TIMEROP_UNQUALIFIED, // 43
  TTCN_Logger::USER_UNQUALIFIED, // 44
  TTCN_Logger::VERDICTOP_UNQUALIFIED, // 48
  TTCN_Logger::WARNING_UNQUALIFIED, // 49
  TTCN_Logger::MATCHING_UNQUALIFIED, // 61
  TTCN_Logger::DEBUG_UNQUALIFIED, // 66
};

const char* TTCN_Logger::severity_category_names[] =
{
  NULL,
  "ACTION",
  "DEFAULTOP",
  "ERROR",
  "EXECUTOR",
  "FUNCTION",
  "PARALLEL",
  "TESTCASE",
  "PORTEVENT",
  "STATISTICS",
  "TIMEROP",
  "USER",
  "VERDICTOP",
  "WARNING",
  "MATCHING",
  "DEBUG",
};


/** Sub-category names for all Severity enum values,
  * used when TTCN_Logger::log_event_types is set to log sub-categories */
const char* TTCN_Logger::severity_subcategory_names[NUMBER_OF_LOGSEVERITIES] = {
  "",
  // ACTION:
  "UNQUALIFIED",
  // DEFAULTOP:
  "ACTIVATE",
  "DEACTIVATE",
  "EXIT",
  "UNQUALIFIED",
  // ERROR:
  "UNQUALIFIED",
  // EXECUTOR:
  "RUNTIME",
  "CONFIGDATA",
  "EXTCOMMAND",
  "COMPONENT",
  "LOGOPTIONS",
  "UNQUALIFIED",
  // FUNCTION:
  "RND",
  "UNQUALIFIED",
  // PARALLEL:
  "PTC",
  "PORTCONN",
  "PORTMAP",
  "UNQUALIFIED",
  // TESTCASE:
  "START",
  "FINISH",
  "UNQUALIFIED",
  // PORTEVENT:
  "PQUEUE",
  "MQUEUE",
  "STATE",
  "PMIN",
  "PMOUT",
  "PCIN",
  "PCOUT",
  "MMRECV",
  "MMSEND",
  "MCRECV",
  "MCSEND",
  "DUALRECV",
  "DUALSEND",
  "UNQUALIFIED",
  "SETSTATE",
  // STATISTICS:
  "VERDICT",
  "UNQUALIFIED",
  // TIMEROP:
  "READ",
  "START",
  "GUARD",
  "STOP",
  "TIMEOUT",
  "UNQUALIFIED",
  // USER:
  "UNQUALIFIED",
  // VERDICTOP:
  "GETVERDICT",
  "SETVERDICT",
  "FINAL",
  "UNQUALIFIED",
  // WARNING:
  "UNQUALIFIED",
  // MATCHING:
  "DONE",
  "TIMEOUT",
  "PCSUCCESS",
  "PCUNSUCC",
  "PMSUCCESS",
  "PMUNSUCC",
  "MCSUCCESS",
  "MCUNSUCC",
  "MMSUCCESS",
  "MMUNSUCC",
  "PROBLEM",
  "UNQUALIFIED",
  // DEBUG:
  "ENCDEC",
  "TESTPORT",
  "USER",
  "FRAMEWORK",
  "UNQUALIFIED"
};

char* TTCN_Logger::logmatch_buffer = NULL;
size_t TTCN_Logger::logmatch_buffer_len = 0;
size_t TTCN_Logger::logmatch_buffer_size = 0;
boolean TTCN_Logger::logmatch_printed = FALSE;

// TODO: Matching related stuff stays here for now.  It just appends the
// string to the end of the current (active) event.
size_t TTCN_Logger::get_logmatch_buffer_len()
{
  return logmatch_buffer_len;
}

void TTCN_Logger::set_logmatch_buffer_len(size_t new_len)
{
  logmatch_buffer_len = new_len;
  logmatch_buffer_size = MIN_BUFFER_SIZE;
  while (logmatch_buffer_size < new_len)
    logmatch_buffer_size *= 2;
  logmatch_buffer = (char *)Realloc(logmatch_buffer, logmatch_buffer_size);
  logmatch_buffer[new_len] = '\0';
}

void TTCN_Logger::print_logmatch_buffer()
{
  if (logmatch_printed) TTCN_Logger::log_event_str(" , ");
  else logmatch_printed = TRUE;
  if (logmatch_buffer_size > 0)
    TTCN_Logger::log_event_str(logmatch_buffer);
}

void TTCN_Logger::log_logmatch_info(const char *fmt_str, ...)
{
  va_list p_var;
  va_start(p_var, fmt_str);

  if (fmt_str == NULL) fmt_str = "<NULL format string>";
  for ( ; ; ) {
    size_t free_space = logmatch_buffer_size - logmatch_buffer_len;
    // Make a copy of p_var to allow multiple calls of vsnprintf().
    va_list p_var2;
    va_copy(p_var2, p_var);
    int fragment_len = vsnprintf(logmatch_buffer + logmatch_buffer_len,
                                 free_space, fmt_str, p_var2);
    va_end(p_var2);
    if (fragment_len < 0) set_logmatch_buffer_len(logmatch_buffer_size * 2);
    else if ((size_t)fragment_len >= free_space)
      set_logmatch_buffer_len(logmatch_buffer_len + fragment_len + 1);
    else {
      logmatch_buffer_len += fragment_len;
      break;
    }
  }
  va_end(p_var);
}

/** The "beginning of time" for the logger (seconds and microseconds
 *  since the Epoch)
 */
struct timeval TTCN_Logger::start_time;

/** The base name of the current executable (no path, no extension) */
char *TTCN_Logger::executable_name = NULL;

/// True to log type (controlpart/altstep/testcase/function/...) and name
/// of the entity responsible for the log message, false to suppress it.
boolean TTCN_Logger::log_entity_name = FALSE;

/// The default log format is the legacy (original) format.
TTCN_Logger::data_log_format_t TTCN_Logger::data_log_format = LF_LEGACY;

#include <assert.h>

/** @brief Equality operator for component_id_t

@param left  component identifier
@param right component identifier
@return true if \p left and \p right refer to the same component

@note This functions tests the equality of the component identifiers,
not the components themselves. It can't detect that component named "foo" is
the same as component number 3.

@li If the selectors differ, returns \c false.
@li If both selectors are COMPONENT_ID_NAME, compares the names.
@li If both selectors are COMPONENT_ID_COMPREF, compares the comp. references.
@li If both selectors are COMPONENT_ID_ALL or COMPONENT_ID_SYSTEM,
returns \c true.

*/
boolean operator==(const component_id_t& left, const component_id_t& right)
{
  if (left.id_selector != right.id_selector)
    return FALSE;

  // The selectors are the same.
  switch (left.id_selector) {
  case COMPONENT_ID_NAME:
    return !strcmp(left.id_name, right.id_name);
  case COMPONENT_ID_COMPREF:
    return left.id_compref == right.id_compref;
  default: // Should be MTC or SYSTEM; identified by the selector.
    return TRUE;
  }
}

char *TTCN_Logger::mputstr_severity(char *str, const TTCN_Logger::Severity& severity)
{
  switch (severity) {
  case TTCN_Logger::ACTION_UNQUALIFIED:
    return mputstr(str, "ACTION");
  case TTCN_Logger::DEFAULTOP_ACTIVATE:
  case TTCN_Logger::DEFAULTOP_DEACTIVATE:
  case TTCN_Logger::DEFAULTOP_EXIT:
  case TTCN_Logger::DEFAULTOP_UNQUALIFIED:
    return mputstr(str, "DEFAULTOP");
  case TTCN_Logger::ERROR_UNQUALIFIED:
    return mputstr(str, "ERROR");
  case TTCN_Logger::EXECUTOR_RUNTIME:
  case TTCN_Logger::EXECUTOR_CONFIGDATA:
  case TTCN_Logger::EXECUTOR_EXTCOMMAND:
  case TTCN_Logger::EXECUTOR_COMPONENT:
  case TTCN_Logger::EXECUTOR_LOGOPTIONS:
  case TTCN_Logger::EXECUTOR_UNQUALIFIED:
    return mputstr(str, "EXECUTOR");
  case TTCN_Logger::FUNCTION_RND:
  case TTCN_Logger::FUNCTION_UNQUALIFIED:
    return mputstr(str, "FUNCTION");
  case TTCN_Logger::PARALLEL_PTC:
  case TTCN_Logger::PARALLEL_PORTCONN:
  case TTCN_Logger::PARALLEL_PORTMAP:
  case TTCN_Logger::PARALLEL_UNQUALIFIED:
    return mputstr(str, "PARALLEL");
  case TTCN_Logger::TESTCASE_START:
  case TTCN_Logger::TESTCASE_FINISH:
  case TTCN_Logger::TESTCASE_UNQUALIFIED:
    return mputstr(str, "TESTCASE");
  case TTCN_Logger::PORTEVENT_PQUEUE:
  case TTCN_Logger::PORTEVENT_MQUEUE:
  case TTCN_Logger::PORTEVENT_STATE:
  case TTCN_Logger::PORTEVENT_PMIN:
  case TTCN_Logger::PORTEVENT_PMOUT:
  case TTCN_Logger::PORTEVENT_PCIN:
  case TTCN_Logger::PORTEVENT_PCOUT:
  case TTCN_Logger::PORTEVENT_MMRECV:
  case TTCN_Logger::PORTEVENT_MMSEND:
  case TTCN_Logger::PORTEVENT_MCRECV:
  case TTCN_Logger::PORTEVENT_MCSEND:
  case TTCN_Logger::PORTEVENT_DUALRECV:
  case TTCN_Logger::PORTEVENT_DUALSEND:
  case TTCN_Logger::PORTEVENT_UNQUALIFIED:
  case TTCN_Logger::PORTEVENT_SETSTATE:
    return mputstr(str, "PORTEVENT");
  case TTCN_Logger::STATISTICS_VERDICT:
  case TTCN_Logger::STATISTICS_UNQUALIFIED:
    return mputstr(str, "STATISTICS");
  case TTCN_Logger::TIMEROP_READ:
  case TTCN_Logger::TIMEROP_START:
  case TTCN_Logger::TIMEROP_GUARD:
  case TTCN_Logger::TIMEROP_STOP:
  case TTCN_Logger::TIMEROP_TIMEOUT:
  case TTCN_Logger::TIMEROP_UNQUALIFIED:
    return mputstr(str, "TIMEROP");
  case TTCN_Logger::USER_UNQUALIFIED:
    return mputstr(str, "USER");
    break;
  case TTCN_Logger::VERDICTOP_GETVERDICT:
  case TTCN_Logger::VERDICTOP_SETVERDICT:
  case TTCN_Logger::VERDICTOP_FINAL:
  case TTCN_Logger::VERDICTOP_UNQUALIFIED:
    return mputstr(str, "VERDICTOP");
  case TTCN_Logger::WARNING_UNQUALIFIED:
    return mputstr(str, "WARNING");
  case TTCN_Logger::MATCHING_DONE:
  case TTCN_Logger::MATCHING_TIMEOUT:
  case TTCN_Logger::MATCHING_PCSUCCESS:
  case TTCN_Logger::MATCHING_PCUNSUCC:
  case TTCN_Logger::MATCHING_PMSUCCESS:
  case TTCN_Logger::MATCHING_PMUNSUCC:
  case TTCN_Logger::MATCHING_MCSUCCESS:
  case TTCN_Logger::MATCHING_MCUNSUCC:
  case TTCN_Logger::MATCHING_MMSUCCESS:
  case TTCN_Logger::MATCHING_MMUNSUCC:
  case TTCN_Logger::MATCHING_PROBLEM:
  case TTCN_Logger::MATCHING_UNQUALIFIED:
    return mputstr(str, "MATCHING");
  case TTCN_Logger::DEBUG_ENCDEC:
  case TTCN_Logger::DEBUG_TESTPORT:
  case TTCN_Logger::DEBUG_USER:
  case TTCN_Logger::DEBUG_FRAMEWORK:
  case TTCN_Logger::DEBUG_UNQUALIFIED:
    return mputstr(str, "DEBUG");
  default:
    return mputstr(str, "UNKNOWN");
  }
}

char *TTCN_Logger::mputstr_timestamp(char *str,
                                     timestamp_format_t p_timestamp_format,
                                     const struct timeval *tv)
{
  if (p_timestamp_format == TIMESTAMP_SECONDS) {
    struct timeval diff;
    if (tv->tv_usec < start_time.tv_usec) {
      diff.tv_sec = tv->tv_sec - start_time.tv_sec - 1;
      diff.tv_usec = tv->tv_usec + (1000000L - start_time.tv_usec);
    } else {
      diff.tv_sec = tv->tv_sec - start_time.tv_sec;
      diff.tv_usec = tv->tv_usec - start_time.tv_usec;
    }
    str = mputprintf(str, "%ld.%06ld", (long)diff.tv_sec, diff.tv_usec);
  } else {
    time_t tv_sec = tv->tv_sec;
    struct tm *lt = localtime(&tv_sec);
    if (lt == NULL) fatal_error("localtime() call failed.");
    if (p_timestamp_format == TIMESTAMP_TIME) {
      str = mputprintf(str, "%02d:%02d:%02d.%06ld",
                       lt->tm_hour, lt->tm_min, lt->tm_sec, tv->tv_usec);
    } else {
      static const char * const month_names[] = { "Jan", "Feb", "Mar",
        "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
      str = mputprintf(str, "%4d/%s/%02d %02d:%02d:%02d.%06ld",
                       lt->tm_year + 1900, month_names[lt->tm_mon],
                       lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec,
                       tv->tv_usec);
    }
  }
  return str;
}

CHARSTRING TTCN_Logger::get_timestamp_str(timestamp_format_t p_timestamp_format)
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL) == -1)
    fatal_error("gettimeofday() system call failed.");
  char *str = mputstr_timestamp(NULL, p_timestamp_format, &tv);
  CHARSTRING ret_val(mstrlen(str), str);
  Free(str);
  return ret_val;
}

CHARSTRING TTCN_Logger::get_source_info_str(source_info_format_t
                                            p_source_info_format)
{
  if (p_source_info_format == SINFO_NONE) return CHARSTRING();
  char *source_info = TTCN_Location::print_location(
    p_source_info_format == SINFO_STACK, TRUE, log_entity_name);
  if (source_info == NULL) return CHARSTRING('-');
  CHARSTRING ret_val(mstrlen(source_info),source_info);
  Free(source_info);
  return ret_val;
}

/** @brief Logs a fatal error and terminates the application.
This function doesn't return: it calls \c exit(EXIT_FAILURE);
@param err_msg printf-style format string
*/
void TTCN_Logger::fatal_error(const char *err_msg, ...)
{
  fputs("Fatal error during logging: ", stderr);
  va_list p_var;
  va_start(p_var, err_msg);
  vfprintf(stderr, err_msg, p_var);
  va_end(p_var);
  OS_error();
  fputs(" Exiting.\n", stderr);
  exit(EXIT_FAILURE);
}

void TTCN_Logger::initialize_logger()
{
  console_log_mask.component_id.id_selector = COMPONENT_ID_ALL;
  console_log_mask.component_id.id_compref = ALL_COMPREF;
  // setting id_compref is optional when id_selector==COMPONENT_ID_ALL
  console_log_mask.mask = Logging_Bits::default_console_mask;

  file_log_mask.component_id.id_selector = COMPONENT_ID_ALL;
  file_log_mask.component_id.id_compref = ALL_COMPREF;
  // setting id_compref is optional when id_selector==COMPONENT_ID_ALL
  file_log_mask.mask = Logging_Bits::log_all;

  emergency_log_mask.component_id.id_selector = COMPONENT_ID_ALL;
  emergency_log_mask.component_id.id_compref = ALL_COMPREF;
  // setting id_compref is optional when id_selector==COMPONENT_ID_ALL
  emergency_log_mask.mask = Logging_Bits::log_all;

  logmatch_buffer = (char*)Malloc(MIN_BUFFER_SIZE);
  logmatch_buffer[0] = '\0';
  logmatch_buffer_len = 0;
  logmatch_buffer_size = MIN_BUFFER_SIZE;
}

void TTCN_Logger::terminate_logger()
{
  // Get rid of plug-ins at first.
  if (plugins_) {
    plugins_->unload_plugins();
    delete plugins_;
    plugins_ = NULL;
  }

  Free(executable_name);
  executable_name = NULL;

  // Clean up the log masks. *_log_masks (which points to the start of
  // actual_*_log_mask) is borrowed as the "next" pointer.
  if (COMPONENT_ID_NAME == console_log_mask.component_id.id_selector) {
    Free(console_log_mask.component_id.id_name);
  }

  if (COMPONENT_ID_NAME == file_log_mask.component_id.id_selector) {
    Free(file_log_mask.component_id.id_name);
  }

  if (COMPONENT_ID_NAME == emergency_log_mask.component_id.id_selector) {
      Free(emergency_log_mask.component_id.id_name);
  }

  Free(logmatch_buffer);
  logmatch_buffer = NULL;
}

boolean TTCN_Logger::is_logger_up()
{
  if (logmatch_buffer == NULL) return FALSE;
  return get_logger_plugin_manager()->plugins_ready();
}

void TTCN_Logger::reset_configuration()
{
  file_log_mask   .mask = Logging_Bits::log_all;
  console_log_mask.mask = Logging_Bits::default_console_mask;
  emergency_log_mask.mask = Logging_Bits::log_all;

  timestamp_format = TIMESTAMP_TIME;
  source_info_format = SINFO_NONE;
  log_event_types = LOGEVENTTYPES_NO;
  log_entity_name = FALSE;

  get_logger_plugin_manager()->reset();
}

void TTCN_Logger::set_executable_name(const char *argv_0)
{
  Free(executable_name);
  size_t name_end = strlen(argv_0);
  // Cut the '.exe' suffix from the end (if present).
  if (name_end >= 4 && !strncasecmp(argv_0 + name_end - 4, ".exe", 4))
    name_end -= 4;
    size_t name_begin = 0;
    // Find the last '/' (if present) to cut the leading directory part.
    for (int i = name_end - 1; i >= 0; i--) {
      if (argv_0[i] == '/') {
        name_begin = i + 1;
        break;
      }
    }
  int name_len = name_end - name_begin;
  if (name_len > 0) {
    executable_name = (char*)Malloc(name_len + 1);
    memcpy(executable_name, argv_0 + name_begin, name_len);
    executable_name[name_len] = '\0';
  } else executable_name = NULL;
}

boolean TTCN_Logger::add_parameter(const logging_setting_t& logging_param)
{
  return get_logger_plugin_manager()->add_parameter(logging_param);
}

void TTCN_Logger::set_plugin_parameters(component component_reference,
                                        const char *component_name)
{
  get_logger_plugin_manager()->set_parameters(component_reference,
                                              component_name);
}

void TTCN_Logger::load_plugins(component component_reference,
                               const char *component_name)
{
  get_logger_plugin_manager()->load_plugins(component_reference,
                                            component_name);
}

void TTCN_Logger::set_file_name(const char *new_filename_skeleton,
                                boolean from_config)
{
  // Pass the file's name to all plug-ins loaded.  Not all of them is
  // interested in this.
  get_logger_plugin_manager()->
    set_file_name(new_filename_skeleton, from_config);
}

boolean TTCN_Logger::set_file_size(component_id_t const& comp, int p_size)
{
  return get_logger_plugin_manager()->set_file_size(comp, p_size);
}

boolean TTCN_Logger::set_file_number(component_id_t const& comp, int p_number)
{
  return get_logger_plugin_manager()->set_file_number(comp, p_number);
}

boolean TTCN_Logger::set_disk_full_action(component_id_t const& comp,
                                       disk_full_action_t p_disk_full_action)
{
  return get_logger_plugin_manager()
    ->set_disk_full_action(comp, p_disk_full_action);
}

void TTCN_Logger::set_start_time()
{
  if (gettimeofday(&start_time, NULL) == -1) {
    fatal_error("gettimeofday() system call failed.");
  }
}

LoggerPluginManager *TTCN_Logger::get_logger_plugin_manager()
{
  if (!plugins_) plugins_ = new LoggerPluginManager();
  return plugins_;
}


// These masks can control not only file and console!  They're general purpose
// event filters, hence their name is not changed.  Stay here.
void TTCN_Logger::set_file_mask(component_id_t const& cmpt, const Logging_Bits& new_file_mask)
{
  // If FileMask was set with a component-specific value, do not allow
  // overwriting with a generic value.
  if (file_log_mask.component_id.id_selector == COMPONENT_ID_COMPREF
    && cmpt.id_selector == COMPONENT_ID_ALL) return;
  file_log_mask.mask = new_file_mask;
  if (cmpt.id_selector == COMPONENT_ID_NAME) { // deep copy needed
    if (file_log_mask.component_id.id_selector == COMPONENT_ID_NAME)
      Free(file_log_mask.component_id.id_name);
    file_log_mask.component_id.id_selector = COMPONENT_ID_NAME;
    file_log_mask.component_id.id_name = mcopystr(cmpt.id_name);
  } else file_log_mask.component_id = cmpt;
}

void TTCN_Logger::set_console_mask(component_id_t const& cmpt, const Logging_Bits& new_console_mask)
{
  // If ConsoleMask was set with a component-specific value, do not allow
  // overwriting with a generic value.
  if (console_log_mask.component_id.id_selector == COMPONENT_ID_COMPREF
    && cmpt.id_selector == COMPONENT_ID_ALL) return;
  console_log_mask.mask = new_console_mask;
  if (cmpt.id_selector == COMPONENT_ID_NAME) { // deep copy needed
    if (console_log_mask.component_id.id_selector == COMPONENT_ID_NAME)
      Free(console_log_mask.component_id.id_name);
    console_log_mask.component_id.id_selector = COMPONENT_ID_NAME;
    console_log_mask.component_id.id_name = mcopystr(cmpt.id_name);
  } else console_log_mask.component_id = cmpt;
}

void TTCN_Logger::set_emergency_logging_mask(component_id_t const& cmpt, const Logging_Bits& new_logging_mask)
{
  // If Emergency Logging Mask was set with a component-specific value, do not allow
  // overwriting with a generic value.
  if (emergency_log_mask.component_id.id_selector == COMPONENT_ID_COMPREF
    && cmpt.id_selector == COMPONENT_ID_ALL) return;
  emergency_log_mask.mask = new_logging_mask;
  if (cmpt.id_selector == COMPONENT_ID_NAME) { // deep copy needed
    if (emergency_log_mask.component_id.id_selector == COMPONENT_ID_NAME)
      Free(emergency_log_mask.component_id.id_name);
    emergency_log_mask.component_id.id_selector = COMPONENT_ID_NAME;
    emergency_log_mask.component_id.id_name = mcopystr(cmpt.id_name);
  } else emergency_log_mask.component_id = cmpt;
}

void TTCN_Logger::set_emergency_logging_behaviour(emergency_logging_behaviour_t behaviour)
{
  emergency_logging_behaviour= behaviour;
}

TTCN_Logger::emergency_logging_behaviour_t TTCN_Logger::get_emergency_logging_behaviour()
{
  return emergency_logging_behaviour;
}

size_t TTCN_Logger::get_emergency_logging()
{
  return emergency_logging;
}

void TTCN_Logger::set_emergency_logging(size_t size)
{
  emergency_logging = size;
}

boolean TTCN_Logger::get_emergency_logging_for_fail_verdict()
{
  return emergency_logging_for_fail_verdict;
}

void TTCN_Logger::set_emergency_logging_for_fail_verdict(boolean b)
{
  emergency_logging_for_fail_verdict = b;
}

Logging_Bits const& TTCN_Logger::get_file_mask()
{
  return file_log_mask.mask;
}

Logging_Bits const& TTCN_Logger::get_console_mask()
{
  return console_log_mask.mask;
}

Logging_Bits const& TTCN_Logger::get_emergency_logging_mask()
{
  return emergency_log_mask.mask;
}

void TTCN_Logger::register_plugin(const component_id_t comp, char *identifier, char *filename)
{
  get_logger_plugin_manager()->register_plugin(comp, identifier, filename);
}

char *TTCN_Logger::get_logger_settings_str()
{
  static const char *timestamp_format_names[] = {
    "Time", "DateTime", "Seconds"
  };
  static const char *logeventtype_names[] = {
    "No", "Yes", "Subcategories"
  };
  static const char *source_info_format_names[] = {
    "None", "Single", "Stack"
  };

  expstring_t filemask_origin =
    component_string(file_log_mask.component_id);
  expstring_t consolemask_origin =
    component_string(console_log_mask.component_id);
  expstring_t filemask_description    = file_log_mask.mask.describe();
  expstring_t consolemask_description = console_log_mask.mask.describe();

  // Global options, common stuff for all plug-ins.
  expstring_t new_log_message = mprintf("TTCN Logger v%d.%d options: "
    "TimeStampFormat:=%s; LogEntityName:=%s; LogEventTypes:=%s; "
    "SourceInfoFormat:=%s; %s.FileMask:=%s; %s.ConsoleMask:=%s;",
    TTCN_Logger::major_version, TTCN_Logger::minor_version,
    timestamp_format_names[timestamp_format],
    logeventtype_names[log_entity_name],
    logeventtype_names[log_event_types],
    source_info_format_names[source_info_format], filemask_origin,
    filemask_description, consolemask_origin, consolemask_description);

  Free(filemask_origin);
  Free(consolemask_origin);
  Free(filemask_description);
  Free(consolemask_description);

  return new_log_message;
}

void TTCN_Logger::write_logger_settings(boolean /*opening*/)
{
  expstring_t new_log_message = get_logger_settings_str();

  // If we get called too early (and become buffered), the logger options
  // must be updated.  By default the initial values are used.
  get_logger_plugin_manager()->log_log_options(new_log_message,
                                               mstrlen(new_log_message));

  Free(new_log_message);
}

boolean TTCN_Logger::should_log_to_file(TTCN_Logger::Severity sev)
{
  if (sev > 0 && sev < TTCN_Logger::NUMBER_OF_LOGSEVERITIES) {
    return file_log_mask.mask.bits[sev];
  }
  return FALSE;
}

boolean TTCN_Logger::should_log_to_console(TTCN_Logger::Severity sev)
{
  // CR_TR00015406: Always log external scripts to the console | MCTR.
  if (sev == TTCN_Logger::EXECUTOR_EXTCOMMAND) return TRUE;
  if (sev > 0 && sev < TTCN_Logger::NUMBER_OF_LOGSEVERITIES) {
    return console_log_mask.mask.bits[sev];
  }
  return FALSE;
}

boolean TTCN_Logger::should_log_to_emergency(TTCN_Logger::Severity sev)
{
  if (sev > 0 && sev < TTCN_Logger::NUMBER_OF_LOGSEVERITIES) {
    return emergency_log_mask.mask.bits[sev];
  }
  return FALSE;
}

void TTCN_Logger::set_timestamp_format(timestamp_format_t new_timestamp_format)
{
  timestamp_format = new_timestamp_format;
}

void TTCN_Logger::set_source_info_format(source_info_format_t new_source_info_format)
{
  source_info_format = new_source_info_format;
}

void TTCN_Logger::set_log_event_types(log_event_types_t new_log_event_types)
{
  log_event_types = new_log_event_types;
}

void TTCN_Logger::set_append_file(boolean new_append_file)
{
  get_logger_plugin_manager()->set_append_file(new_append_file);
}

void TTCN_Logger::set_log_entity_name(boolean new_log_entity_name)
{
  log_entity_name = new_log_entity_name;
}

void TTCN_Logger::open_file()
{
  get_logger_plugin_manager()->open_file();
}

void TTCN_Logger::close_file()
{
  get_logger_plugin_manager()->close_file();
}

void TTCN_Logger::ring_buffer_dump(boolean do_close_file)
{
  get_logger_plugin_manager()->ring_buffer_dump(do_close_file);
}

unsigned int TTCN_Logger::get_mask()
{
  TTCN_warning("TTCN_Logger::get_mask() is deprecated, please use "
               "TTCN_Logger::should_log_to_file() or "
               "TTCN_Logger::should_log_to_console() instead.");
  return LOG_ALL_IMPORTANT | MATCHING_UNQUALIFIED | DEBUG_UNQUALIFIED;
}

TTCN_Logger::matching_verbosity_t TTCN_Logger::get_matching_verbosity()
{
  return matching_verbosity;
}

void TTCN_Logger::set_matching_verbosity(TTCN_Logger::matching_verbosity_t v)
{
  matching_verbosity = v;
}

// Called from the generated code and many more places...  Stay here.  The
// existence of the file descriptors etc. is the responsibility of the
// plug-ins.
boolean TTCN_Logger::log_this_event(TTCN_Logger::Severity event_severity)
{
  //ToDO: emergency logging = true
  if (should_log_to_file(event_severity)) return TRUE;
  else if (should_log_to_console(event_severity)) return TRUE;
  else if (should_log_to_emergency(event_severity) && (get_emergency_logging() > 0)) return TRUE;
  else return FALSE;
}

void TTCN_Logger::log(TTCN_Logger::Severity msg_severity,
                      const char *fmt_str, ...)
{
  va_list p_var;
  va_start(p_var, fmt_str);
  log_va_list(msg_severity, fmt_str, p_var);
  va_end(p_var);
}

void TTCN_Logger::send_event_as_error()
{
  char* error_msg = get_logger_plugin_manager()->get_current_event_str();
  if (!error_msg)
    return;

  if (TTCN_Communication::is_mc_connected()) {
    TTCN_Communication::send_error("%s", error_msg);
  } else {
    fprintf(stderr, "%s\n", error_msg);
  }
  Free(error_msg);
}

// Part of the external interface.  Don't touch.
void TTCN_Logger::log_str(TTCN_Logger::Severity msg_severity,
                          const char *str_ptr)
{
  if (!log_this_event(msg_severity)) return;
  if (str_ptr == NULL)
    str_ptr = "<NULL pointer>";
  get_logger_plugin_manager()->log_unhandled_event(msg_severity,
                                                   str_ptr, strlen(str_ptr));
  logmatch_printed = FALSE;
}

void TTCN_Logger::log_va_list(TTCN_Logger::Severity msg_severity,
                              const char *fmt_str, va_list p_var)
{
  get_logger_plugin_manager()->log_va_list(msg_severity, fmt_str, p_var);
  logmatch_printed = FALSE;
}

void TTCN_Logger::begin_event(TTCN_Logger::Severity msg_severity, boolean log2str)
{
  get_logger_plugin_manager()->begin_event(msg_severity, log2str);
}

void TTCN_Logger::end_event()
{
  get_logger_plugin_manager()->end_event();
  // TODO: Find another place for these...
  logmatch_printed = FALSE;
}

CHARSTRING TTCN_Logger::end_event_log2str()
{
  CHARSTRING ret_val = get_logger_plugin_manager()->end_event_log2str();
  logmatch_printed = FALSE;
  return ret_val;
}

void TTCN_Logger::finish_event()
{
  get_logger_plugin_manager()->finish_event();
}

void TTCN_Logger::log_event(const char *fmt_str, ...)
{
  va_list p_var;
  va_start(p_var, fmt_str);
  log_event_va_list(fmt_str, p_var);
  va_end(p_var);
}

void TTCN_Logger::log_event_str(const char *str_ptr)
{
  get_logger_plugin_manager()->log_event_str(str_ptr);
  logmatch_printed = FALSE;
}

void TTCN_Logger::log_event_va_list(const char *fmt_str, va_list p_var)
{
  get_logger_plugin_manager()->log_event_va_list(fmt_str, p_var);
  logmatch_printed = FALSE;
}

void TTCN_Logger::log_event_unbound()
{
  switch (data_log_format) {
  case LF_LEGACY:
    log_event_str("<unbound>");
    break;
  case LF_TTCN:
    log_char('-');
    break;
  default:
    log_event_str("<unknown>");
  }
}

void TTCN_Logger::log_event_uninitialized()
{
  switch (data_log_format) {
  case LF_LEGACY:
    log_event_str("<uninitialized template>");
    break;
  case LF_TTCN:
    log_char('-');
    break;
  default:
    log_event_str("<unknown>");
  }
}

void TTCN_Logger::log_event_enum(const char* enum_name_str, int enum_value)
{
  switch (data_log_format) {
  case LF_LEGACY:
    TTCN_Logger::log_event("%s (%d)", enum_name_str, enum_value);
    break;
  case LF_TTCN:
    log_event_str(enum_name_str);
    break;
  default:
    log_event_str("<unknown>");
  }
}

void TTCN_Logger::log_char(char c)
{
  get_logger_plugin_manager()->log_char(c);
  logmatch_printed = FALSE;
}

boolean TTCN_Logger::is_printable(unsigned char c)
{
  if (!isascii(c)) return FALSE;
  else if (isprint(c)) return TRUE;
  else {
    switch (c) {
    case '\a':
    case '\b':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
      return TRUE;
    default:
      return FALSE;
    }
  }
}

void TTCN_Logger::log_char_escaped(unsigned char c)
{
  switch (c) {
  case '\n':
    log_event_str("\\n");
    break;
  case '\t':
    log_event_str("\\t");
    break;
  case '\v':
    log_event_str("\\v");
    break;
  case '\b':
    log_event_str("\\b");
    break;
  case '\r':
    log_event_str("\\r");
    break;
  case '\f':
    log_event_str("\\f");
    break;
  case '\a':
    log_event_str("\\a");
    break;
  case '\\':
    log_event_str("\\\\");
    break;
  case '"':
    log_event_str("\\\"");
    break;
  default:
    if (isprint(c)) log_char(c);
    else log_event("\\%03o", c);
    break;
  }
}

void TTCN_Logger::log_char_escaped(unsigned char c, char*& p_buffer) {
  switch (c) {
  case '\n':
    p_buffer = mputstr(p_buffer, "\\n");
    break;
  case '\t':
    p_buffer = mputstr(p_buffer, "\\t");
    break;
  case '\v':
    p_buffer = mputstr(p_buffer, "\\v");
    break;
  case '\b':
    p_buffer = mputstr(p_buffer, "\\b");
    break;
  case '\r':
    p_buffer = mputstr(p_buffer, "\\r");
    break;
  case '\f':
    p_buffer = mputstr(p_buffer, "\\f");
    break;
  case '\a':
    p_buffer = mputstr(p_buffer, "\\a");
    break;
  case '\\':
    p_buffer = mputstr(p_buffer, "\\\\");
    break;
  case '"':
    p_buffer = mputstr(p_buffer, "\\\"");
    break;
  default:
    if (isprint(c)) p_buffer = mputc(p_buffer, c);
    else
      p_buffer = mputprintf(p_buffer, "\\%03o", c);
    break;
  }
}

static const char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

void TTCN_Logger::log_hex(unsigned char nibble)
{
  if (nibble < 16) log_char(hex_digits[nibble]);
  else log_event_str("<unknown>");
}

void TTCN_Logger::log_octet(unsigned char octet)
{
  log_char(hex_digits[octet >> 4]);
  log_char(hex_digits[octet & 0x0F]);
}

void TTCN_Logger::OS_error()
{
  if (errno != 0) {
    const char *error_string = strerror(errno);
    if (error_string != NULL) log_event(" (%s)", error_string);
    else log_event(" (Unknown error: errno = %d)", errno);
    errno = 0;
  }
}

void TTCN_Logger::log_timer_read(const char *timer_name,
                                 double timeout_val)
{
  get_logger_plugin_manager()->log_timer_read(timer_name, timeout_val);
}

void TTCN_Logger::log_timer_start(const char *timer_name, double start_val)
{
  get_logger_plugin_manager()->log_timer_start(timer_name, start_val);
}

void TTCN_Logger::log_timer_guard(double start_val)
{
  get_logger_plugin_manager()->log_timer_guard(start_val);
}

void TTCN_Logger::log_timer_stop(const char *timer_name, double stop_val)
{
  get_logger_plugin_manager()->log_timer_stop(timer_name, stop_val);
}

void TTCN_Logger::log_timer_timeout(const char *timer_name,
                                    double timeout_val)
{
  get_logger_plugin_manager()->log_timer_timeout(timer_name, timeout_val);
}

void TTCN_Logger::log_timer_any_timeout()
{
  get_logger_plugin_manager()->log_timer_any_timeout();
}

void TTCN_Logger::log_timer_unqualified(const char *message)
{
  get_logger_plugin_manager()->log_timer_unqualified(message);
}

void TTCN_Logger::log_testcase_started(const qualified_name& testcase_name)
{
  get_logger_plugin_manager()->log_testcase_started(testcase_name);
}

void TTCN_Logger::log_testcase_finished(const qualified_name& testcase_name,
                                        verdicttype verdict,
                                        const char *reason)
{
  get_logger_plugin_manager()->log_testcase_finished(testcase_name, verdict, reason);
}

void TTCN_Logger::log_setverdict(verdicttype new_verdict, verdicttype old_verdict,
                                 verdicttype local_verdict, const char *old_reason, const char *new_reason)
{
  get_logger_plugin_manager()->log_setverdict(new_verdict, old_verdict,
                                              local_verdict, old_reason, new_reason);
}

void TTCN_Logger::log_getverdict(verdicttype verdict)
{
  get_logger_plugin_manager()->log_getverdict(verdict);
}

void TTCN_Logger::log_final_verdict(boolean is_ptc, verdicttype ptc_verdict,
  verdicttype local_verdict, verdicttype new_verdict,
  const char *verdict_reason, int notification, int ptc_compref,
  const char *ptc_name)
{
  get_logger_plugin_manager()->log_final_verdict(is_ptc, ptc_verdict,
    local_verdict, new_verdict, verdict_reason, notification, ptc_compref, ptc_name);
}

void TTCN_Logger::log_controlpart_start_stop(const char *module_name, int finished)
{
  get_logger_plugin_manager()->log_controlpart_start_stop(module_name, finished);
}

void TTCN_Logger::log_controlpart_errors(unsigned int error_count)
{
  get_logger_plugin_manager()->log_controlpart_errors(error_count);
}

void TTCN_Logger::log_verdict_statistics(size_t none_count, double none_percent,
                                         size_t pass_count, double pass_percent,
                                         size_t inconc_count, double inconc_percent,
                                         size_t fail_count, double fail_percent,
                                         size_t error_count, double error_percent)
{
  get_logger_plugin_manager()->log_verdict_statistics(none_count, none_percent,
                                                      pass_count, pass_percent,
                                                      inconc_count, inconc_percent,
                                                      fail_count, fail_percent,
                                                      error_count, error_percent);
}

void TTCN_Logger::log_defaultop_activate(const char *name, int id)
{
  get_logger_plugin_manager()->log_defaultop_activate(name, id);
}

void TTCN_Logger::log_defaultop_deactivate(const char *name, int id)
{
  get_logger_plugin_manager()->log_defaultop_deactivate(name, id);
}

void TTCN_Logger::log_defaultop_exit(const char *name, int id, int x)
{
  get_logger_plugin_manager()->log_defaultop_exit(name, id, x);
}

void TTCN_Logger::log_executor_runtime(int reason)
{
  get_logger_plugin_manager()->log_executor_runtime(reason);
}

void TTCN_Logger::log_HC_start(const char *host)
{
  get_logger_plugin_manager()->log_HC_start(host);
}

void TTCN_Logger::log_fd_limits(int fd_limit, long fd_set_size)
{
  get_logger_plugin_manager()->log_fd_limits(fd_limit, fd_set_size);
}

void TTCN_Logger::log_testcase_exec(const char *module, const char *tc)
{
  get_logger_plugin_manager()->log_testcase_exec(module, tc);
}

void TTCN_Logger::log_module_init(const char *module, boolean finish)
{
  get_logger_plugin_manager()->log_module_init(module, finish);
}

void TTCN_Logger::log_mtc_created(long pid)
{
  get_logger_plugin_manager()->log_mtc_created(pid);
}

void TTCN_Logger::log_configdata(int reason, const char *str)
{
  get_logger_plugin_manager()->log_configdata(reason, str);
}

void TTCN_Logger::log_executor_component(int reason)
{
  get_logger_plugin_manager()->log_executor_component(reason);
}

void TTCN_Logger::log_executor_misc(int reason, const char *name,
  const char *address, int port)
{
  get_logger_plugin_manager()->log_executor_misc(reason, name, address, port);
}

void TTCN_Logger::log_extcommand(extcommand_t action, const char *cmd)
{
  get_logger_plugin_manager()->log_extcommand(action, cmd);
}

void TTCN_Logger::log_matching_done(const char *type, int ptc,
  const char *return_type, int reason)
{
  get_logger_plugin_manager()->log_matching_done(reason, type, ptc, return_type);
}

void TTCN_Logger::log_matching_problem(int reason, int operation, boolean check,
  boolean anyport, const char *port_name)
{
  get_logger_plugin_manager()->log_matching_problem(reason, operation,
    check, anyport, port_name);
}

void TTCN_Logger::log_matching_success(int port_type, const char *port_name,
  int compref, const CHARSTRING& info)
{
  get_logger_plugin_manager()->log_matching_success(port_type, port_name,
    compref, info);
}

void TTCN_Logger::log_matching_failure(int port_type, const char *port_name,
  int compref, int reason, const CHARSTRING& info)
{
  get_logger_plugin_manager()->log_matching_failure(port_type, port_name,
    compref, reason, info);
}

void TTCN_Logger::log_matching_timeout(const char *timer_name)
{
  get_logger_plugin_manager()->log_matching_timeout(timer_name);
}

void TTCN_Logger::log_portconnmap(int operation, int src_compref, const char *src_port,
  int dst_compref, const char *dst_port)
{
  get_logger_plugin_manager()->log_portconnmap(operation, src_compref, src_port,
    dst_compref, dst_port);
}

void TTCN_Logger::log_par_ptc(int reason, const char *module, const char *name,
  int compref, const char *compname, const char *tc_loc, int alive_pid, int status)
{
  get_logger_plugin_manager()->log_par_ptc(reason, module, name, compref,
    compname, tc_loc, alive_pid, status);
}

void TTCN_Logger::log_port_queue(int operation, const char *port_name, int compref,
  int id, const CHARSTRING& address, const CHARSTRING& param)
{
  get_logger_plugin_manager()->log_port_queue(operation, port_name, compref,
    id, address, param);
}

void TTCN_Logger::log_port_state(int operation, const char *port_name)
{
  get_logger_plugin_manager()->log_port_state(operation, port_name);
}

void TTCN_Logger::log_procport_send(const char *portname, int operation, int compref,
  const CHARSTRING& system, const CHARSTRING& param)
{
  get_logger_plugin_manager()->log_procport_send(portname, operation, compref,
    system, param);
}

void TTCN_Logger::log_procport_recv(const char *portname, int operation,
  int compref, boolean check, const CHARSTRING& param, int id)
{
  get_logger_plugin_manager()->log_procport_recv(portname, operation, compref,
    check, param, id);
}

void TTCN_Logger::log_msgport_send(const char *portname, int compref,
  const CHARSTRING& param)
{
  get_logger_plugin_manager()->log_msgport_send(portname, compref, param);
}

void TTCN_Logger::log_msgport_recv(const char *portname, int operation, int compref,
  const CHARSTRING& system, const CHARSTRING& param, int id)
{
  get_logger_plugin_manager()->log_msgport_recv(portname, operation, compref,
    system, param, id);
}

void TTCN_Logger::log_dualport_map(boolean incoming, const char *target_type,
  const CHARSTRING& value, int id)
{
  get_logger_plugin_manager()->log_dualport_map(incoming, target_type, value, id);
}

void TTCN_Logger::log_dualport_discard(boolean incoming, const char *target_type,
  const char *port_name, boolean unhandled)
{
  get_logger_plugin_manager()->log_dualport_discard(incoming, target_type, port_name,
    unhandled);
}

void TTCN_Logger::log_setstate(const char* port_name, translation_port_state state,
  const CHARSTRING& info)
{
  get_logger_plugin_manager()->log_setstate(port_name, state, info);
}

void TTCN_Logger::log_port_misc(int reason, const char *port_name,
  int remote_component, const char *remote_port,
  const char *ip_address, int tcp_port, int new_size)
{
  get_logger_plugin_manager()->log_port_misc(reason, port_name,
    remote_component, remote_port, ip_address, tcp_port, new_size);
}

void TTCN_Logger::log_random(int action, double v, unsigned long u)
{
  get_logger_plugin_manager()->log_random(action, v, u);
}

void TTCN_Logger::clear_parameters() {
  get_logger_plugin_manager()->clear_param_list();
  get_logger_plugin_manager()->clear_plugin_list();
}

// The one instance (only for backward compatibility).
TTCN_Logger TTCN_logger;

// Location related stuff.
TTCN_Location *TTCN_Location::innermost_location = NULL,
              *TTCN_Location::outermost_location = NULL;

TTCN_Location::TTCN_Location(const char *par_file_name,
  unsigned int par_line_number, entity_type_t par_entity_type,
  const char *par_entity_name)
: file_name(par_file_name), line_number(par_line_number),
  entity_type(par_entity_type), entity_name(par_entity_name),
  inner_location(NULL), outer_location(innermost_location)
{
  if (par_file_name == NULL) file_name = "<unknown file>";
  if (par_entity_type == LOCATION_UNKNOWN) entity_name = NULL;
  else if (par_entity_name == NULL) entity_name = "<unknown>";
  if (outer_location != NULL) outer_location->inner_location = this;
  else outermost_location = this;
  innermost_location = this;
}

void TTCN_Location::update_lineno(unsigned int new_lineno)
{
  line_number = new_lineno;
}

TTCN_Location::~TTCN_Location()
{
  if (inner_location == NULL) innermost_location = outer_location;
  else inner_location->outer_location = outer_location;
  if (outer_location == NULL) outermost_location = inner_location;
  else outer_location->inner_location = inner_location;
}

char *TTCN_Location::print_location(boolean print_outers,
  boolean print_innermost, boolean print_entity_name)
{
  char *ret_val = NULL;
  if (innermost_location != NULL) {
    if (print_outers) {
      for (TTCN_Location *iter = outermost_location;
           iter != NULL && iter != innermost_location;
           iter = iter->inner_location)
        ret_val = iter->append_contents(ret_val, print_entity_name);
    }
    if (print_innermost)
      ret_val = innermost_location->append_contents(ret_val,
                                                    print_entity_name);
  }
  return ret_val;
}

void TTCN_Location::strip_entity_name(char*& par_str)
{
  if (!par_str) return;
  char *new_str = NULL;
  boolean in_paren = FALSE;
  for (char *str_ptr = par_str; *str_ptr != '\0' ; str_ptr++) {
    switch (*str_ptr) {
    case '(':
      in_paren = TRUE;
      break;
    case ')':
      in_paren = FALSE;
      break;
    default:
      if (!in_paren) new_str = mputc(new_str, *str_ptr);
      break;
    }
  }
  Free(par_str);
  par_str = new_str;
}

unsigned int TTCN_Location::get_line_number()
{
  if (innermost_location != NULL) {
    return innermost_location->line_number;
  }
  return 0;
}

char *TTCN_Location::append_contents(char *par_str,
  boolean print_entity_name) const
{
  if (par_str != NULL) par_str = mputstr(par_str, "->");
  par_str = mputprintf(par_str, "%s:%u", file_name, line_number);
  if (print_entity_name) {
    switch (entity_type) {
    case LOCATION_CONTROLPART:
      par_str = mputprintf(par_str, "(controlpart:%s)", entity_name);
      break;
    case LOCATION_TESTCASE:
      par_str = mputprintf(par_str, "(testcase:%s)", entity_name);
      break;
    case LOCATION_ALTSTEP:
      par_str = mputprintf(par_str, "(altstep:%s)", entity_name);
      break;
    case LOCATION_FUNCTION:
      par_str = mputprintf(par_str, "(function:%s)", entity_name);
      break;
    case LOCATION_EXTERNALFUNCTION:
      par_str = mputprintf(par_str, "(externalfunction:%s)", entity_name);
      break;
    case LOCATION_TEMPLATE:
      par_str = mputprintf(par_str, "(template:%s)", entity_name);
      break;
    default:
      break;
    }
  }
  return par_str;
}

TTCN_Location_Statistics::TTCN_Location_Statistics(const char *par_file_name,
  unsigned int par_line_number, entity_type_t par_entity_type,
  const char *par_entity_name): TTCN_Location(par_file_name, par_line_number,
  par_entity_type, par_entity_name)
{
  TCov::hit(file_name, line_number, entity_name);
}

void TTCN_Location_Statistics::update_lineno(unsigned int new_lineno)
{
  TTCN_Location::update_lineno(new_lineno);
  TCov::hit(file_name, line_number);
}

void TTCN_Location_Statistics::init_file_lines(const char *file_name, const int line_nos[], size_t line_nos_len)
{
  TCov::init_file_lines(file_name, line_nos, line_nos_len);
}

void TTCN_Location_Statistics::init_file_functions(const char *file_name, const char *function_names[], size_t function_names_len)
{
  TCov::init_file_functions(file_name, function_names, function_names_len);
}

TTCN_Location_Statistics::~TTCN_Location_Statistics() { }

expstring_t component_string(const component_id_t& comp_id)
{
  expstring_t retval;
  switch( comp_id.id_selector ) {
  case COMPONENT_ID_NAME:
    retval = mcopystr(comp_id.id_name);
    break;
  case COMPONENT_ID_COMPREF:
    retval = mprintf("%d", comp_id.id_compref);
    break;
  case COMPONENT_ID_ALL:
    retval = mcopystr("*");
    break;
  case COMPONENT_ID_SYSTEM:
    retval = mcopystr("<System>");
    break;
  default: // Can't happen.
    retval = mcopystr("Unknown component type !");
    break;
  }
  return retval;
}
