/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Beres, Szabolcs
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#ifndef LOGGER_PLUGIN_MANAGER_HH
#define LOGGER_PLUGIN_MANAGER_HH

#include "Types.h"
#include "Logger.hh"
#include "LoggerPlugin.hh"
#include "TitanLoggerApi.hh"
// The above includes TTCN3.hh, which pulls in everything in the runtime
#include "LoggingParam.hh"

struct Logging_Bits;
struct component_id_t;
struct disk_full_action_t;

#ifdef __GNUC__
#define MUST_CHECK __attribute__((__warn_unused_result__))
#else
#define MUST_CHECK
#endif

namespace LoggerAPI
{
  class TitanLogEvent;
}


class RingBuffer
{
  TitanLoggerApi::TitanLogEvent* buffer;
  unsigned int head;
  unsigned int tail;
  unsigned int size;

public:
  explicit RingBuffer() : buffer(NULL), head(0), tail(0),
    size(TTCN_Logger::get_emergency_logging()) {}
  ~RingBuffer();

  boolean get(TitanLoggerApi::TitanLogEvent& data) MUST_CHECK;
  void put(TitanLoggerApi::TitanLogEvent data);
  void clear();
  unsigned int get_size() const { return size; }
  void set_size(unsigned int new_size);
  boolean isFull() const { return (head + 1) % (size + 1) == tail; }
  boolean isEmpty() const {return head == tail; }
};

class LoggerPluginManager
{
  friend class ILoggerPlugin;

public:
  void ring_buffer_dump(boolean do_close_file);

  // Sends a single log event to all logger plugins.
  void internal_log_to_all(const TitanLoggerApi::TitanLogEvent& event,
      boolean log_buffered, boolean separate_file, boolean use_emergency_mask);
  // If an event appears before any logger is configured we have to pre-buffer it.
  void internal_prebuff_logevent(const TitanLoggerApi::TitanLogEvent& event);
  // When the loggers get configured we have to log everything we have buffered so far
  void internal_log_prebuff_logevent();
public:
  explicit LoggerPluginManager();
  ~LoggerPluginManager();

  void register_plugin(const component_id_t comp, char *identifier, char *filename);
  void load_plugins(component component_reference, const char *component_name);
  void unload_plugins();
  void reset();

  boolean add_parameter(const logging_setting_t& logging_param);
  void set_parameters(component component_reference, const char *component_name);

  boolean plugins_ready() const;

  /// Backward compatibility functions to handle top level configuration file
  /// parameters.  All logger plug-ins will receive these settings, but they
  /// can simply ignore them.
  boolean set_file_mask(component_id_t const& comp, const Logging_Bits& new_file_mask);
  boolean set_console_mask(component_id_t const& comp,
                        const Logging_Bits& new_console_mask);
  void set_file_name(const char *new_filename_skeleton, boolean from_config);
  void set_append_file(boolean new_append_file);
  /// Return true if the given configuration file parameter was set multiple
  /// times.  (The return value is used by the configuration file parser.)
  boolean set_file_size(component_id_t const& comp, int p_size);
  boolean set_file_number(component_id_t const& cmpt, int p_number);
  boolean set_disk_full_action(component_id_t const& comp,
    TTCN_Logger::disk_full_action_t p_disk_full_action);
  void open_file();
  void close_file();
  void fill_common_fields(TitanLoggerApi::TitanLogEvent& event,
                          const TTCN_Logger::Severity& severity);
  void append_event_str(const char *str);

  /// returns a copy of the current event string
  char* get_current_event_str();

  /// Do the actual call to the plug-ins with EVENT.  Flush the buffers if
  /// necessary.  It is called at the end of each log_* function.  The
  /// complete event handling is part of LoggerPluginManager.
  void log(const TitanLoggerApi::TitanLogEvent& event);
  void log_va_list(TTCN_Logger::Severity msg_severity, const char *fmt_str,
                   va_list p_var);
  void buffer_event(const TitanLoggerApi::TitanLogEvent& event);
  void begin_event(TTCN_Logger::Severity msg_severity, boolean log2str);
  void end_event();
  CHARSTRING end_event_log2str();
  void finish_event();
  void log_event_str(const char *str_ptr);
  void log_char(char c);
  void log_event_va_list(const char *fmt_str, va_list p_var);
  void log_unhandled_event(TTCN_Logger::Severity severity,
                           const char *message_ptr, size_t message_len);
  void log_log_options(const char *message_ptr, size_t message_len);

  /** @name New, one-per-event log functions.
   *  @{ * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  void log_timer_read(const char *timer_name, double start_val);
  void log_timer_start(const char *timer_name, double start_val);
  void log_timer_guard(double start_val);
  void log_timer_stop(const char *timer_name, double stop_val);
  void log_timer_timeout(const char *timer_name, double timeout_val);
  void log_timer_any_timeout();
  void log_timer_unqualified(const char *message);

  void log_testcase_started (const qualified_name& testcase_name);
  void log_testcase_finished(const qualified_name& testcase_name,
    verdicttype verdict, const char *reason);

  void log_controlpart_start_stop(const char *module_name, int finished);
  void log_controlpart_errors(unsigned int error_count);

  void log_setverdict(verdicttype new_verdict, verdicttype old_verdict,
                      verdicttype local_verdict, const char *old_reason = NULL, const char *new_reason = NULL);
  void log_getverdict(verdicttype verdict);
  void log_final_verdict(boolean is_ptc, verdicttype ptc_verdict,
    verdicttype local_verdict, verdicttype new_verdict,
    const char *verdict_reason = NULL, int notification = -1,
    int ptc_compref = UNBOUND_COMPREF, const char *ptc_name = NULL);

  void log_verdict_statistics(size_t none_count, double none_percent,
                              size_t pass_count, double pass_percent,
                              size_t inconc_count, double inconc_percent,
                              size_t fail_count, double fail_percent,
                              size_t error_count, double error_percent);

  void log_defaultop_activate  (const char *name, int id);
  void log_defaultop_deactivate(const char *name, int id);
  void log_defaultop_exit      (const char *name, int id, int x);

  /// EXECUTOR_RUNTIME, fixed strings only (no params)
  void log_executor_runtime(TitanLoggerApi::ExecutorRuntime_reason reason);
  // EXECUTOR_RUNTIME with parameters
  void log_HC_start(const char *host);
  void log_fd_limits(int fd_limit, long fd_set_size);
  void log_not_overloaded(int pid);
  void log_testcase_exec(const char *tc, const char *module);
  void log_module_init(const char *module, boolean finish);
  void log_mtc_created(long pid);
  /// EXECUTOR_CONFIGDATA
  void log_configdata(int reason, const char *str);

  void log_executor_component(int reason); // and some more

  void log_executor_misc(int reason, const char *name, const char *address,
    int port);

  void log_extcommand(TTCN_Logger::extcommand_t action, const char *cmd);

  void log_matching_done(TitanLoggerApi::MatchingDoneType_reason reason,
    const char *type, int ptc, const char *return_type);

  void log_matching_problem(int reason, int operation,
    boolean check, boolean anyport, const char *port_name);

  void log_matching_success(int port_type, const char *port_name, int compref,
    const CHARSTRING& info);
  void log_matching_failure(int port_type, const char *port_name, int compref,
    int reason, const CHARSTRING& info);

  void log_matching_timeout(const char *timer_name);

  void log_random(int action, double v, unsigned long u);

  void log_portconnmap(int operation, int src_compref, const char *src_port,
    int dst_compref, const char *dst_port);

  void log_par_ptc(int reason, const char *module, const char *name, int compref,
    const char *compname, const char *tc_loc, int alive_pid, int status);

  void log_port_queue(int operation, const char *port_name, int compref, int id,
    const CHARSTRING& address, const CHARSTRING& param);

  void log_port_state(int operation, const char *port_name);

  void log_procport_send(const char *portname, int operation,
    int compref, const CHARSTRING& system, const CHARSTRING& param);
  void log_procport_recv(const char *portname, int operation, int compref,
    boolean check, const CHARSTRING& param, int id);
  void log_msgport_send(const char *portname, int compref,
    const CHARSTRING& param);
  void log_msgport_recv(const char *portname, int operation, int compref,
    const CHARSTRING& system, const CHARSTRING& param, int id);

  void log_dualport_map(boolean incoming, const char *target_type,
    const CHARSTRING& value, int id);
  void log_dualport_discard(boolean incoming, const char *target_type,
    const char *port_name, boolean unhandled);
  void log_setstate(const char *port_name, translation_port_state state,
    const CHARSTRING& info);

  void log_port_misc(int reason, const char *port_name, int remote_component,
    const char *remote_port, const char *ip_address, int tcp_port, int new_size);

  /** @} * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  LoggerPlugin *find_plugin(const char *name);

  void clear_param_list();
  void clear_plugin_list();

private:
  explicit LoggerPluginManager(const LoggerPluginManager&);
  LoggerPluginManager& operator=(const LoggerPluginManager&);

  void apply_parameter(const logging_setting_t& logparam);
  void send_parameter_to_plugin(LoggerPlugin* plugin, const logging_setting_t& logparam);

  void load_plugin(const char *identifier, const char *filename);

  enum event_destination_t
  {
     ED_NONE,  // To be discarded.
     ED_FILE,  // Event goes to log file or console, it's a historic name.
     ED_STRING // Event goes to CHARSTRING.
  };

private:
  /// Circular buffer for emergency logging
  RingBuffer ring_buffer;

  /// Number of loaded plug-ins. Should be at least 1.
  size_t n_plugins_;

  /// List of active (dynamic/static) logger plug-ins.
  LoggerPlugin **plugins_;

  // This is for the fast events.
  struct LogEntry
  {
    TitanLoggerApi::TitanLogEvent event_;
    LogEntry *next_entry_;
  } *entry_list_;

  // This is for the active events (~ stack).
  struct ActiveEvent
  {
    /// Space for a TitanLogEvent aligned as a long int.
    long event_[(sizeof(TitanLoggerApi::TitanLogEvent) - 1 + sizeof(long))
                / sizeof(long)];

    char *event_str_;  // Speed up event string handling.
    size_t event_str_len_;  ///< Actual length of the string.
    size_t event_str_size_;  ///< Size of the allocated memory.
    event_destination_t event_destination_;  // Used only be active events.
    ActiveEvent *outer_event_;
    // For better space efficiency, all the pieces are kept concatenated as always,
    // but we remember offsets into the concatenated string.
    size_t  num_pieces_;
    size_t *pieces_; // the end of each piece
    // Only (num_pieces_-1) elements are allocated, so the last accessible
    // element is at num_pieces_-2. The end of the last piece is event_str_len_

    /// True if the event is for log2str, in which case @p event_ is blank.
    boolean fake_;

    /** Constructor
     *
     * @param fake_event true if the event is for log2str, in which case
     *                   @p event_ is not initialized.
     * @param dest event destination (logfile, string or none)
     */
    ActiveEvent(boolean fake_event, event_destination_t dest);
    ~ActiveEvent();
    TitanLoggerApi::TitanLogEvent& get_event()
    {
      return *reinterpret_cast<TitanLoggerApi::TitanLogEvent*>((void*)&event_);
    }
  } *current_event_;

  logging_setting_t* logparams_head;
  logging_setting_t* logparams_tail;

  logging_plugin_t* logplugins_head;
  logging_plugin_t* logplugins_tail;
};

#endif // LOGGER_PLUGIN_MANAGER_HH
