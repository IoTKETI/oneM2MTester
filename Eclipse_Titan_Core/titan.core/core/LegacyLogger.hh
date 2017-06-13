/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#ifndef LegacyLogger_HH
#define LegacyLogger_HH

#include "Logger.hh"
#include "ILoggerPlugin.hh"
#include "Charstring.hh"

namespace TitanLoggerApi
{
  class TitanLogEvent;
}

struct component_id_t;
struct logfile_data_struct;

/// This class is responsible for configuration file parameters: ConsoleMask
/// and FileMask as well.
class LegacyLogger: public ILoggerPlugin
{
public:
  LegacyLogger();
  virtual ~LegacyLogger();
  inline boolean is_static() { return TRUE; }
  inline boolean is_log2str_capable() { return TRUE; }
  CHARSTRING log2str(const TitanLoggerApi::TitanLogEvent& event);
  void init(const char *options = 0);
  void fini();
  void reset();
  void fatal_error(const char *err_msg, ...);

  void log(const TitanLoggerApi::TitanLogEvent& event, boolean log_buffered,
      boolean separate_file, boolean use_emergency_mask);
  static char * plugin_specific_settings();

protected:
  explicit LegacyLogger(const LegacyLogger&);
  LegacyLogger& operator=(const LegacyLogger&);

  char *get_file_name(size_t idx);
  void set_file_name(const char *new_filename_skeleton, boolean from_config);
  boolean set_file_size(int p_size);
  void set_append_file(boolean new_append_file);
  boolean set_file_number(int p_number);
  boolean set_disk_full_action(TTCN_Logger::disk_full_action_t p_disk_full_action);
  void open_file(boolean is_first = TRUE);
  void close_file();
  boolean log_file(const TitanLoggerApi::TitanLogEvent& event, boolean log_buffered);
  boolean log_file_emerg(const TitanLoggerApi::TitanLogEvent& event);
  boolean log_console(const TitanLoggerApi::TitanLogEvent& event,
                   const TTCN_Logger::Severity& severity);
  boolean log_to_file(const char *event_str);
  void create_parent_directories(const char *path_name);
  /// Checks for invalid combinations of LogFileSize, LogFileNumber and
  /// DiskFullAction.
  void chk_logfile_data();
protected:
  FILE *log_fp_;
  FILE *er_;
  size_t logfile_bytes_;
  size_t logfile_size_;
  size_t logfile_number_;
  size_t logfile_index_;
  /** @brief "Format string" for the log file name.
   *  The following format specifiers will be interpreted:
   *  \li \%c -> name of the current testcase (only on PTCs)
   *  \li \%e -> name of executable
   *  \li \%h -> hostname
   *  \li \%l -> login name
   *  \li \%n -> component name (only on PTCs, optional)
   *  \li \%p -> process id
   *  \li \%r -> component reference
   *  \li \%s -> default suffix (currently: always "log")
   *  \li \%t -> component type (only on PTCs)
   *  \li \%i -> log file index
   *  \li \%\% -> \%
   **/
  char *filename_skeleton_;
  TTCN_Logger::disk_full_action_t disk_full_action_;
  struct timeval disk_full_time_;

  /// True if `filename_skeleton_' was set from the configuration file.
  boolean skeleton_given_;
  /// True to open the log file and append to it, false to truncate.
  boolean append_file_;
  boolean is_disk_full_;
  boolean format_c_present_;
  boolean format_t_present_;
  char *current_filename_;
  static LegacyLogger *myself;
};

#endif  // LegacyLogger_HH
