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
#ifndef LOGGER_HH
#define LOGGER_HH

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>

#include "Types.h"

struct log_mask_info;
struct Logging_Bits;
class LoggerPlugin;
class LoggerPluginManager;
class CHARSTRING;
struct logging_setting_t;

/** @brief Logger class

Acts as a namespace.  No instance data or methods.

*/
class TTCN_Logger
{
  friend class LoggerPlugin;
  friend class LoggerPluginManager;
public:
  static LoggerPluginManager *plugins_;
  // TODO: This should be plug-in specific, but the configuration file parsers
  // are using this type...
  enum disk_full_action_type_t { DISKFULL_ERROR, DISKFULL_STOP,
    DISKFULL_RETRY, DISKFULL_DELETE };
  struct disk_full_action_t {
    disk_full_action_type_t type;
    size_t retry_interval;
  };
  enum timestamp_format_t { TIMESTAMP_TIME, TIMESTAMP_DATETIME,
    TIMESTAMP_SECONDS };
  enum source_info_format_t { SINFO_NONE, SINFO_SINGLE, SINFO_STACK };
  enum log_event_types_t { LOGEVENTTYPES_NO, LOGEVENTTYPES_YES,
    LOGEVENTTYPES_SUBCATEGORIES };

  enum emergency_logging_behaviour_t { BUFFER_ALL, BUFFER_MASKED };

  enum matching_verbosity_t { VERBOSITY_COMPACT, VERBOSITY_FULL };
  enum extcommand_t { EXTCOMMAND_START, EXTCOMMAND_SUCCESS };

  /** Values and templates can be logged in the following formats */
  enum data_log_format_t { LF_LEGACY, LF_TTCN };

  /** @brief Titan logging severities

      @note The values are not bit masks. Expressions like
      @code LOG_ALL | TTCN_DEBUG @endcode will not work correctly.

      @note This enum must start at 0 and have no gaps until
      NUMBER_OF_LOGSEVERITIES.
  */
  enum Severity
  {
    NOTHING_TO_LOG = 0, // for compatibility

    ACTION_UNQUALIFIED,

    DEFAULTOP_ACTIVATE,
    DEFAULTOP_DEACTIVATE,
    DEFAULTOP_EXIT,
    DEFAULTOP_UNQUALIFIED, //5

    ERROR_UNQUALIFIED,

    EXECUTOR_RUNTIME,
    EXECUTOR_CONFIGDATA,
    EXECUTOR_EXTCOMMAND,
    EXECUTOR_COMPONENT, //10
    EXECUTOR_LOGOPTIONS,
    EXECUTOR_UNQUALIFIED,

    FUNCTION_RND,
    FUNCTION_UNQUALIFIED,

    PARALLEL_PTC, //15
    PARALLEL_PORTCONN,
    PARALLEL_PORTMAP,
    PARALLEL_UNQUALIFIED,

    TESTCASE_START,
    TESTCASE_FINISH, //20
    TESTCASE_UNQUALIFIED,

    PORTEVENT_PQUEUE,
    PORTEVENT_MQUEUE,
    PORTEVENT_STATE,
    PORTEVENT_PMIN, //25
    PORTEVENT_PMOUT,
    PORTEVENT_PCIN,
    PORTEVENT_PCOUT,
    PORTEVENT_MMRECV,
    PORTEVENT_MMSEND, //30
    PORTEVENT_MCRECV,
    PORTEVENT_MCSEND,
    PORTEVENT_DUALRECV,
    PORTEVENT_DUALSEND,
    PORTEVENT_UNQUALIFIED, //35
    PORTEVENT_SETSTATE,

    STATISTICS_VERDICT,
    STATISTICS_UNQUALIFIED,

    TIMEROP_READ,
    TIMEROP_START, //40
    TIMEROP_GUARD,
    TIMEROP_STOP,
    TIMEROP_TIMEOUT,
    TIMEROP_UNQUALIFIED,

    USER_UNQUALIFIED, //45

    VERDICTOP_GETVERDICT,
    VERDICTOP_SETVERDICT,
    VERDICTOP_FINAL,
    VERDICTOP_UNQUALIFIED,

    WARNING_UNQUALIFIED, //50

    // MATCHING and DEBUG should be at the end (not included in LOG_ALL)
    MATCHING_DONE,
    MATCHING_TIMEOUT,
    MATCHING_PCSUCCESS,
    MATCHING_PCUNSUCC,
    MATCHING_PMSUCCESS, //55
    MATCHING_PMUNSUCC,
    MATCHING_MCSUCCESS,
    MATCHING_MCUNSUCC,
    MATCHING_MMSUCCESS,
    MATCHING_MMUNSUCC, //60
    MATCHING_PROBLEM,
    MATCHING_UNQUALIFIED,

    DEBUG_ENCDEC,
    DEBUG_TESTPORT,
    DEBUG_USER,
    DEBUG_FRAMEWORK,
    DEBUG_UNQUALIFIED, //67

    NUMBER_OF_LOGSEVERITIES, // must follow the last individual severity
    LOG_ALL_IMPORTANT
  };
  /// Number of main severities (plus one, because it includes LOG_NOTHING).
  static const size_t number_of_categories = 16;
  /// Main severity names.
  static const char* severity_category_names[number_of_categories];
  /// Sub-category suffixes.
  static const char* severity_subcategory_names[NUMBER_OF_LOGSEVERITIES];
  static const TTCN_Logger::Severity sev_categories[number_of_categories];

  static const unsigned int major_version = 2;
  static const unsigned int minor_version = 2;

  // buffer used by the log match algorithms to buffer the output
  static char* logmatch_buffer;
  // length of the logmatch buffer
  static size_t logmatch_buffer_len;
  // the size of the log match buffer (memory allocated for it)
  static size_t logmatch_buffer_size;
  // true if the logmatch buffer was already printed in the actual log event.
  static boolean logmatch_printed;

  // length of the emergencylogging buffer
  static size_t emergency_logging;
  static void set_emergency_logging(size_t size);
  static size_t get_emergency_logging();

  static emergency_logging_behaviour_t emergency_logging_behaviour;
  static void set_emergency_logging_behaviour(emergency_logging_behaviour_t behaviour);
  static emergency_logging_behaviour_t get_emergency_logging_behaviour();
  
  static boolean emergency_logging_for_fail_verdict;
  static void set_emergency_logging_for_fail_verdict(boolean b);
  static boolean get_emergency_logging_for_fail_verdict();

  /** @brief returns the actual length of the logmatch buffer
  This way it can be stored for later, when the buffer needs to be reverted.
  @return the length
   */
  static size_t get_logmatch_buffer_len();

  /** @brief sets the length of the logmatch buffer
  Is used to effectively revert the buffer to a previous state.
  @param new_size the new length to be set.
  */
  static void set_logmatch_buffer_len(size_t new_size);

  /** @brief prints the contents of the logmatch buffer to the current event
  Writes contents of the logmatch buffer into the actual event, prefixed with
  a ',' if needed.
  */
  static void print_logmatch_buffer();

  /** @brief Format a string into the current logmatch buffer.
  @param fmt_str printf-style format string
  @callgraph
  */
  static void log_logmatch_info(const char *fmt_str, ...)
    __attribute__ ((__format__ (__printf__, 1, 2)));

  struct log_mask_struct;  // Forward declaration.
private:
  static log_mask_struct  console_log_mask;
  static log_mask_struct  file_log_mask;
  static log_mask_struct  emergency_log_mask;


  static matching_verbosity_t matching_verbosity;

  static timestamp_format_t timestamp_format;

  static source_info_format_t source_info_format;

  static log_event_types_t log_event_types;

  static struct timeval start_time;

  static char *executable_name;

  static boolean log_entity_name;

  static data_log_format_t data_log_format;

  /// Always return the single instance of the LoggerPluginManager.
  static LoggerPluginManager *get_logger_plugin_manager();
  /// Returns the actual global logger options.  The returned string must be
  /// freed by the caller.
  static char *get_logger_settings_str();

public:
  /** @brief Initialize the logger.

  @pre initialize_logger has not been called previously
  */
  static void initialize_logger();

  /// Frees any resources held by the logger.
  static void terminate_logger();

  static boolean is_logger_up();

  /** @brief Initializes the logger configuration.

  \li sets the logfilename template
  \li sets \c file_mask to LOG_ALL
  \li sets \c console_mask to TTCN_ERROR | TTCN_WARNING | TTCN_ACTION | TTCN_TESTCASE | TTCN_STATISTICS
  \li sets \c timestamp_format = TIMESTAMP_TIME;
  \li sets \c source_info_format = SINFO_NONE;
  \li sets \c log_event_types = LOGEVENTTYPES_NO;
  \li sets \c append_file = FALSE;
  \li sets \c log_entity_name = FALSE;

  @pre initialize_logger() has been called.

  */
  static void reset_configuration();

  /** @name Setting internal members
  @{
  */
  /** @brief Sets executable_name.

  \p argv_0 is stripped of its extension and path (if any) and the result is
  assigned to \c executable_name.

  @param argv_0 string containing the name of the program ( \c argv[0] )
  */
  static void set_executable_name(const char *argv_0);
  static inline char *get_executable_name() { return executable_name; }

  static data_log_format_t get_log_format() { return data_log_format; }
  static void set_log_format(data_log_format_t p_data_log_format) { data_log_format = p_data_log_format; }

  static boolean add_parameter(const logging_setting_t& logging_param);
  static void set_plugin_parameters(component component_reference, const char* component_name);
  static void load_plugins(component component_reference, const char* component_name);

  /** @brief Set the log filename skeleton.

  @param new_filename_skeleton this string is copied to \c filename_skeleton
  @param from_config TRUE if set from config file value
  */
  static void set_file_name(const char *new_filename_skeleton,
                            boolean from_config = TRUE);

  /// Sets start_time by calling \c gettimeofday()
  static void set_start_time();

  /** @brief Set the logging mask to a log file.

  The mask is simply stored internally.
  @see set_component()

  @param cmpt a component_id_t identifying the component.
  @param new_file_mask a \c LoggingBits containing the categories
  to be logged to a file.
  */
  static void set_file_mask(component_id_t const& cmpt,
                            const Logging_Bits& new_file_mask);

  /** @brief Set the logging mask to the console.

  The mask is simply stored internally.
  @see set_component()

  @param cmpt a component_id_t identifying the component.
  @param new_console_mask a \c LoggingBits containing the categories to be
  logged to the console.
  */
  static void set_console_mask(component_id_t const& cmpt,
                               const Logging_Bits& new_console_mask);

  /** @brief Set the logging mask to the console.

    The mask is simply stored internally.
    @see set_component()

    @param cmpt a component_id_t identifying the component.
    @param new_logging_mask
    */
  static void set_emergency_logging_mask(component_id_t const& cmpt,
    const Logging_Bits& new_logging_mask);


  static Logging_Bits const& get_file_mask();

  static Logging_Bits const& get_console_mask();

  static Logging_Bits const& get_emergency_logging_mask();

  static void set_timestamp_format(timestamp_format_t new_timestamp_format);
  static inline timestamp_format_t get_timestamp_format()
    { return timestamp_format; }
  static void set_source_info_format(source_info_format_t new_source_info_format);
  static inline source_info_format_t get_source_info_format()
    { return source_info_format; }
  static void set_log_event_types(log_event_types_t new_log_event_types);
  static inline log_event_types_t get_log_event_types()
    { return log_event_types; }
  static void set_append_file(boolean new_append_file);
  static void set_log_entity_name(boolean new_log_entity_name);
  /** @} */

  static CHARSTRING get_timestamp_str(timestamp_format_t p_timestamp_format);
  static CHARSTRING get_source_info_str(source_info_format_t p_source_info_format);
  static boolean get_log_entity_name() { return log_entity_name; }
  static char *mputstr_severity(char *str, const TTCN_Logger::Severity& sev);
  static char *mputstr_timestamp(char *str,
                                 timestamp_format_t p_timestamp_format,
                                 const struct timeval *tv);
  // Register a logger plug-in into the LoggerPluginManager from the
  // configuration file.
  static void register_plugin(const component_id_t comp, char *identifier, char *filename);

  static boolean set_file_size(component_id_t const& comp, int p_size);
  static boolean set_file_number(component_id_t const& comp, int p_number);
  static boolean set_disk_full_action(component_id_t const& comp,
                                   disk_full_action_t p_disk_full_action);

  /** @brief Whether a message with the given severity should be logged to the
      log file.

  Checks the actual logging bits selected by set_component()

  @param sev logging severity
  @return \c true if it should be logged, \c false otherwise
  */
  static boolean should_log_to_file(Severity sev);

  /// Like should_log_to_file() but for logging to the console.
  static boolean should_log_to_console(Severity sev);

  /// Like should_log_to_file() but for logging to the emergency.
  static boolean should_log_to_emergency(Severity sev);

  /** @brief Get the log event mask.

  @deprecated Please use TTCN_Logger::should_log_to_file() or
  TTCN_Logger::should_log_to_console() instead.
  */
  static unsigned int get_mask();

  static matching_verbosity_t get_matching_verbosity();
  static void set_matching_verbosity(matching_verbosity_t v);

  /** @brief Handle logging of the logger's configuration settings.

  This function is called from Single_main.cc and Runtime.cc immediately
  after the "Host controller/Executor/Component started" message.
  However, in some cases (e.g. in the HC) the logger's settings aren't known yet
  so the content of the log message cannot be determined.
  In this situation, an empty log message is stored until the configuration
  is received, then the stored message is updated before writing.
  This method performs all three of these operations.

  @param opening true if called from TTCN_Logger::open_file() while writing
  the buffered messages, false otherwise.
  */
  static void write_logger_settings(boolean opening = false);

  /** @brief Opens the log file.

  Opens the file with the name returned by get_filename(). If append_file
  is true, new events are written to the end of an existing log file.
  Otherwise, the log file is truncated.
  If there are buffered events, they are written to the file.
  */
  static void open_file();
  static void close_file();
  /** @brief dump all events from ring buffer to log file
      @param do_close_file if true, close the files afterwards
   */
  static void ring_buffer_dump(boolean do_close_file);

  /** @brief Should this event be logged?

  @return \c true if the the event with severity specified by
  \p event_severity should be logged to either the console or the log file,
  \c false otherwise.

  If the logger was not configured yet, it returns true.  Otherwise, it
  masks \p event_severity with console_mask and, if the log file is opened,
  file_mask.
  */
  static boolean log_this_event(Severity event_severity);

  /** @brief Format and log a message.

  Calls log_va_list.

  @param msg_severity severity
  @param fmt_str printf-style format string
  @callgraph
  */
  static void log(Severity msg_severity, const char *fmt_str, ...)
                  __attribute__ ((__format__ (__printf__, 2, 3) /*, deprecated (one day) */ ));

  /** @brief Sends the current event string as an error message to the MC.
   */
  static void send_event_as_error();

  static void fatal_error(const char *err_msg, ...)
                          __attribute__ ((__format__ (__printf__, 1, 2),
                                          __noreturn__));

  /** @brief Log a string without formatting.

  Description:

  @param msg_severity severity
  @param str_ptr the string
  @callgraph
  */
  static void log_str(Severity msg_severity, const char *str_ptr );

  /** @brief Format and log a list of arguments.

  Description:

  @param msg_severity severity
  @param fmt_str      printf-style format string
  @param p_var        variable arguments
  @callgraph
  */
  static void log_va_list(Severity msg_severity, const char *fmt_str,
                          va_list p_var);

  /** @brief Begin an event

  Events are held on a stack; the new event becomes the current event.

  If the logger is not yet initialized, this function does nothing.

  @param msg_severity severity
  @callgraph
  */
  static void begin_event(Severity msg_severity, boolean log2str = FALSE);

  /** begin an event that logs to CHARSTRING */
  static void begin_event_log2str() { begin_event(USER_UNQUALIFIED, TRUE); }

  /** @brief End event

  @pre current_event != NULL

  This is when the event is actually logged.

  @callgraph
  */
  static void end_event();

  static CHARSTRING end_event_log2str();

  /** @brief Finish event.

  @pre current_event != NULL

  @callgraph
  */
  static void finish_event();

  /** @brief Format a string into the current event.

  @pre current_event != NULL

  @param fmt_str printf-style format string
  @callgraph
  */
  static void log_event(const char *fmt_str, ...)
    __attribute__ ((__format__ (__printf__, 1, 2)));

  /** @brief Log event str

  Stores a string in the current log event without formatting.

  @pre current_event != NULL

  @param str_ptr the message
  @callgraph
  */
  static void log_event_str(const char *str_ptr);

  /** @brief Format a list of arguments into the current event.

  @pre current_event != NULL

  Increases the current event's buffer as needed.

  @param fmt_str printf-style format string
  @param p_var variable arguments
  @callgraph
  */
  static void log_event_va_list(const char *fmt_str, va_list p_var);

  // Log an unbound/uninitialized/enum value according to the current format setting
  static void log_event_unbound();
  static void log_event_uninitialized();
  static void log_event_enum(const char* enum_name_str, int enum_value);

  /** @brief Log one character into the current event.

  @pre current_event != NULL

  Appends the character \c c to the buffer of the current event.

  @param c the character
  @callgraph
  */
  static void log_char(char c);

  /// Return \c true if \c c is a printable character, \c false otherwise.
  static boolean is_printable(unsigned char c);

  /** @brief Log a char.

  Description:

  @param c the character
  @callgraph
  */
  static void log_char_escaped(unsigned char c);

  /** @brief Log a char into expstring_t buffer.

  Description:

  @param c the character
  @param p_buffer expstring_t buffer
  @callgraph
  */
  static void log_char_escaped(unsigned char c, char*& p_buffer);

  /** @brief Log hex.

  Description:

  @param nibble
  @callgraph
  */
  static void log_hex(unsigned char nibble);

  /** @brief Log an octet.

  Description:

  @param octet
  @callgraph
  */
  static void log_octet(unsigned char octet);

  /** @brief Synonymous to log_char.

  Description:

  @param c
  @callgraph
  */
  static inline void log_event(char c) { log_char(c); }

  /** @brief Log the OS error based on the current errno.
      Description:
      @callgraph
  */
  static void OS_error();

  // To preserve the semantic meaning of the log messages...
  /** @name New, one-per-event log functions
   *  @{ * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  static void log_timer_read(const char *timer_name, double start_val);
  static void log_timer_start(const char *timer_name, double start_val);
  static void log_timer_guard(double start_val);
  static void log_timer_stop(const char *timer_name, double stop_val);
  static void log_timer_timeout(const char *timer_name, double timeout_val);
  static void log_timer_any_timeout();
  static void log_timer_unqualified(const char *message);

  static void log_setverdict(verdicttype new_verdict, verdicttype old_verdict,
		             verdicttype local_verdict, const char *old_reason = NULL, const char *new_reason = NULL);
  static void log_getverdict(verdicttype verdict);
  // Handle all VERDICTOP_FINAL events.  ptc_verdict/new_verdict are used only
  // for detailed PTC statistics, in all other cases they're the same.
  // - is_ptc: true if we're setting the final verdict for a PTC, false for MTC
  // - ptc_verdict: final verdict of the PTC
  // - local_verdict: the local verdict until now
  // - new_verdict: new verdict after PTC is finished
  // - verdict_reason: final verdict reason for PTC/MTC if applicable
  // - notification: enumeration value for the two fixed notification messages
  // - ptc_compref: only for detailed PTC statistics
  // - ptc_name: only for detailed PTC statistics
  static void log_final_verdict(boolean is_ptc, verdicttype ptc_verdict,
    verdicttype local_verdict, verdicttype new_verdict,
    const char *verdict_reason = NULL, int notification = -1,
    int ptc_compref = UNBOUND_COMPREF, const char *ptc_name = NULL);

  static void log_testcase_started (const qualified_name& testcase_name);
  static void log_testcase_finished(const qualified_name& testcase_name,
                                    verdicttype verdict,
                                    const char *reason);
  static void log_controlpart_start_stop(const char *module_name, int finished);
  static void log_controlpart_errors(unsigned int error_count);

  static void log_verdict_statistics(size_t none_count, double none_percent,
                                     size_t pass_count, double pass_percent,
                                     size_t inconc_count, double inconc_percent,
                                     size_t fail_count, double fail_percent,
                                     size_t error_count, double error_percent);

  static void log_defaultop_activate  (const char *name, int id);
  static void log_defaultop_deactivate(const char *name, int id);
  static void log_defaultop_exit      (const char *name, int id, int x);

  /// EXECUTOR_RUNTIME, fixed strings only (no params)
  static void log_executor_runtime(int reason);
  /// EXECUTOR_RUNTIME, messages with parameters
  static void log_HC_start(const char *host);
  static void log_fd_limits(int fd_limit, long fd_set_size);
  static void log_testcase_exec(const char *module, const char *tc);
  static void log_module_init(const char *module, boolean finish = FALSE);
  static void log_mtc_created(long pid);

  /// EXECUTOR_CONFIGDATA
  /// @param str module name, config file or NULL
  static void log_configdata(int reason, const char *str = NULL);

  static void log_executor_component(int reason);

  static void log_executor_misc(int reason, const char *name, const char *address,
    int port);

  static void log_extcommand(extcommand_t action, const char *cmd);
  //static void log_extcommand_success(const char *cmd);

  static void log_matching_done(const char *type, int ptc,
    const char *return_type, int reason);

  static void log_matching_problem(int reason, int operation,
    boolean check, boolean anyport, const char *port_name = NULL);

  static void log_matching_success(int port_type, const char *port_name, int compref,
    const CHARSTRING& info);
  static void log_matching_failure(int port_type, const char *port_name, int compref,
    int reason, const CHARSTRING& info);

  static void log_matching_timeout(const char *timer_name);

  static void log_portconnmap(int operation, int src_compref, const char *src_port,
    int dst_compref, const char *dst_port);

  static void log_par_ptc(int reason,
    const char *module = NULL, const char *name = NULL, int compref = 0,
    const char *compname = NULL, const char *tc_loc = NULL,
    int alive_pid = 0, int status = 0);

  static void log_port_queue(int operation, const char *port_name, int compref,
    int id, const CHARSTRING& address, const CHARSTRING& param);

  static void log_port_state(int operation, const char *port_name);

  static void log_procport_send(const char *portname, int operation, int compref,
    const CHARSTRING& system, const CHARSTRING& param);
  static void log_procport_recv(const char *portname, int operation, int compref,
    boolean check, const CHARSTRING& param, int id);
  static void log_msgport_send(const char *portname, int compref,
    const CHARSTRING& param);
  static void log_msgport_recv(const char *portname, int operation, int compref,
    const CHARSTRING& system, const CHARSTRING& param, int id);

  static void log_dualport_map(boolean incoming, const char *target_type,
    const CHARSTRING& value, int id);
  static void log_dualport_discard(boolean incoming, const char *target_type,
    const char *port_name, boolean unhaldled);
  static void log_setstate(const char *port_name, translation_port_state state,
    const CHARSTRING& info);

  static void log_port_misc(int reason, const char *port_name,
    int remote_component = NULL_COMPREF, const char *remote_port = NULL,
    const char *ip_address = NULL, int tcp_port = -1, int new_size = 0);

  static void log_random(int action, double v, unsigned long u);

  static void clear_parameters();
  /** @} TODO: More * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
};

/** @name #defines for backward-compatibility.
    These #defines (TTCN_ERROR, TTCN_WARNING, ... TTCN_DEBUG)
    are source-compatible (but \b not binary compatible) with the original #defines

    @note The values are not bit masks. Expressions like
    @code LOG_ALL | TTCN_DEBUG @endcode will not work correctly.

    @{
*/
#define TTCN_ERROR      TTCN_Logger::ERROR_UNQUALIFIED
#define TTCN_WARNING    TTCN_Logger::WARNING_UNQUALIFIED
#define TTCN_PORTEVENT  TTCN_Logger::PORTEVENT_UNQUALIFIED
#define TTCN_TIMEROP    TTCN_Logger::TIMEROP_UNQUALIFIED
#define TTCN_VERDICTOP  TTCN_Logger::VERDICTOP_UNQUALIFIED
#define TTCN_DEFAULTOP  TTCN_Logger::DEFAULTOP_UNQUALIFIED
#define TTCN_ACTION     TTCN_Logger::ACTION_UNQUALIFIED
#define TTCN_TESTCASE   TTCN_Logger::TESTCASE_UNQUALIFIED
#define TTCN_FUNCTION   TTCN_Logger::FUNCTION_UNQUALIFIED
#define TTCN_USER       TTCN_Logger::USER_UNQUALIFIED
#define TTCN_STATISTICS TTCN_Logger::STATISTICS_UNQUALIFIED
#define TTCN_PARALLEL   TTCN_Logger::PARALLEL_UNQUALIFIED
#define TTCN_EXECUTOR   TTCN_Logger::EXECUTOR_UNQUALIFIED
#define TTCN_MATCHING   TTCN_Logger::MATCHING_UNQUALIFIED
#define TTCN_DEBUG      TTCN_Logger::DEBUG_UNQUALIFIED
#define LOG_NOTHING     TTCN_Logger::NOTHING_TO_LOG
#define LOG_ALL         TTCN_Logger::LOG_ALL_IMPORTANT
/** @} */

extern TTCN_Logger TTCN_logger;

class TTCN_Location {
public:
  enum entity_type_t { LOCATION_UNKNOWN, LOCATION_CONTROLPART,
    LOCATION_TESTCASE, LOCATION_ALTSTEP, LOCATION_FUNCTION,
    LOCATION_EXTERNALFUNCTION, LOCATION_TEMPLATE };
protected:
  const char *file_name;
  unsigned int line_number;
  entity_type_t entity_type;
  const char *entity_name;
  TTCN_Location *inner_location, *outer_location;
  static TTCN_Location *innermost_location, *outermost_location;
  friend class LoggerPluginManager;
public:
  /** @brief Create a TTCN_Location object.

  Objects of this class are created by code generated by the TTCN-3 compiler.

  @param par_file_name source file of the call site
  @param par_line_number line number of the call site
  @param par_entity_type controlpart/testcase/altstep/function/...etc
  @param par_entity_name entity name of the caller

  @note The constructor copies the par_file_name and par_entity_name
  pointers but not the strings. The caller must ensure that the strings
  don't go out of scope before the TTCN_Location object.
  */
  TTCN_Location(const char *par_file_name, unsigned int par_line_number,
                entity_type_t par_entity_type = LOCATION_UNKNOWN,
                const char *par_entity_name = NULL);
  virtual ~TTCN_Location();

  virtual void update_lineno(unsigned int new_lineno);

  /** @brief Write the current location information to a string.
   *
   * @param print_outers \c true to print all the callers
   * (in the style of SourceInfo = Stack)
   * @param print_innermost \c true to print the current (innermost) location
   * @param print_entity_name \c true to print the type
   * (control part/test case/altstep/function/template) and name
   * of the current location.
   * @note print_innermost is always TRUE (there is never a call with FALSE)
   *
   * @return an expstring_t. The caller is responsible for calling Free()
   **/
  static char *print_location(boolean print_outers, boolean print_innermost,
                              boolean print_entity_name);
  /** @brief Remove location information from a string.
   *
   * @param [in,out] par_str the string to modify.
   * @pre par_str was allocated by Malloc, or is NULL.
   * It may be an expstring_t.
   *
   * The function copies characters from \p par_str to a temporary string,
   * except characters between parentheses. Then it calls
   * \c Free(par_str) and replaces \p par_str with the temporary.
   *
   **/
  static void strip_entity_name(char*& par_str);
  
  /** @brief Return the innermost location's line number 
   **/
  static unsigned int get_line_number();
protected:
  char *append_contents(char *par_str, boolean print_entity_name) const;
};

class TTCN_Location_Statistics: public TTCN_Location
{
public:
  TTCN_Location_Statistics(const char *par_file_name, unsigned int par_line_number,
    entity_type_t par_entity_type = LOCATION_UNKNOWN,
    const char *par_entity_name = NULL);
  ~TTCN_Location_Statistics();
  void update_lineno(unsigned int new_lineno);
  static void init_file_lines(const char *file_name, const int line_nos[], size_t line_nos_len);
  static void init_file_functions(const char *file_name, const char *function_names[], size_t function_names_len);
};

/// Restores the original log format when running out of scope: use as local variable
class Logger_Format_Scope {
private:
  TTCN_Logger::data_log_format_t orig_log_format;
public:
  Logger_Format_Scope(TTCN_Logger::data_log_format_t p_log_format) { orig_log_format=TTCN_Logger::get_log_format(); TTCN_Logger::set_log_format(p_log_format); }
  ~Logger_Format_Scope() { TTCN_Logger::set_log_format(orig_log_format); }
};

#endif
