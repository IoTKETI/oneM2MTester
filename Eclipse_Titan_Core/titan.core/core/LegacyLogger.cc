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
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szalai, Gabor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include "LegacyLogger.hh"

#include "TitanLoggerApi.hh"
#include "Component.hh"

#include "ILoggerPlugin.hh"
#include "LoggingBits.hh"
#include "Communication.hh"
#include "Runtime.hh"
#include "Logger.hh"
#include "../common/version_internal.h"

#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
// for WIFEXITED & co.

#include <pwd.h>

#ifdef SOLARIS
typedef long suseconds_t;
#endif

extern "C"
{
// It's a static plug-in, destruction is done in the destructor.  We need
// `extern "C"' for some reason.
ILoggerPlugin *create_legacy_logger() { return new LegacyLogger(); }
}


static char *event_to_str(const TitanLoggerApi::TitanLogEvent& event,
                   boolean without_header = FALSE);
static char *append_header(char *ret_val, const struct timeval& timestamp,
                    const TTCN_Logger::Severity severity,
                    const char *sourceinfo);

static void timer_event_str(char *& ret_val, const TitanLoggerApi::TimerEvent_choice& tec);
static void defaultop_event_str(char *& ret_val, const TitanLoggerApi::DefaultEvent_choice& dec);
static void executor_event_str(char *& ret_val, const TitanLoggerApi::ExecutorEvent_choice& eec);
static void portevent_str(char *& ret_val, const TitanLoggerApi::PortEvent_choice& pec);
//void extcommand_str(char *& ret_val, const TitanLoggerApi::ExecutorEvent_choice& choice);
static void matchingop_str(char *& ret_val, const TitanLoggerApi::MatchingEvent_choice& choice);
static void parallel_str(char *& ret_val, const TitanLoggerApi::ParallelEvent_choice& choice);
static void testcaseop_str(char *& ret_val, const TitanLoggerApi::TestcaseEvent_choice& choice);
static void verdictop_str(char *& ret_val, const TitanLoggerApi::VerdictOp_choice& choice);
static void statistics_str(char *& ret_val, const TitanLoggerApi::StatisticsType_choice& choice);

namespace API = TitanLoggerApi;

// Defined in Logger.cc.  Compares the name or the component identifier.
extern boolean operator==(const component_id_t& left,
                       const component_id_t& right);

LegacyLogger* LegacyLogger::myself = 0;

LegacyLogger::LegacyLogger()
: log_fp_(NULL), er_(NULL), logfile_bytes_(0), logfile_size_(0), logfile_number_(1),
  logfile_index_(1), filename_skeleton_(NULL), skeleton_given_(FALSE),
  append_file_(FALSE), is_disk_full_(FALSE), format_c_present_(FALSE),
  format_t_present_(FALSE), current_filename_(NULL)
{
  if (myself != 0) {
    // Whoa, déjà vu!
    // A déjà vu is usually a glitch in the Matrix.
    // It happens when they change something.
    fputs("Only one LegacyLogger allowed! Aborting.\n", stderr);
    abort(); // Don't use TTCN_error. Debugging infinite recursion is not fun.
  }
  else {
    myself = this;
  }
  this->name_ = mputstr(this->name_, "LegacyLogger");
  this->help_ = mputstr(this->help_, "LegacyLogger");
  this->disk_full_action_.type = TTCN_Logger::DISKFULL_ERROR;
  this->disk_full_action_.retry_interval = 0;
}

LegacyLogger::~LegacyLogger()
{
  assert(this->name_ && this->help_);
  Free(this->name_);
  Free(this->help_);
  this->name_ = NULL;
  this->help_ = NULL;
  Free(this->filename_skeleton_);
  this->filename_skeleton_ = NULL;
  Free(this->current_filename_);
  this->current_filename_ = NULL;

  myself = 0;
}

void LegacyLogger::reset()
{
  this->disk_full_action_.type = TTCN_Logger::DISKFULL_ERROR;
  this->disk_full_action_.retry_interval = 0;
  this->logfile_size_ = 0;
  this->logfile_number_ = 1;
  this->logfile_bytes_ = 0;
  this->logfile_index_ = 1;
  this->is_disk_full_ = FALSE;
  this->skeleton_given_ = FALSE;
  this->append_file_ = FALSE;
  this->is_configured_ = FALSE;
}

void LegacyLogger::init(const char */*options*/)
{
  // Print some version information, handle general options etc. later...
}

void LegacyLogger::fini()
{
  close_file();
}

void LegacyLogger::fatal_error(const char *err_msg, ...)
{
  fputs("Fatal error during logging: ", stderr);
  va_list p_var;
  va_start(p_var, err_msg);
  vfprintf(stderr, err_msg, p_var);
  va_end(p_var);
  if (errno != 0) {
    const char *error_string = strerror(errno);
    if (error_string != NULL) fprintf(stderr, " (%s)", error_string);
    else fprintf(stderr, " (Unknown error: errno = %d)", errno);
    errno = 0;
  }
  fputs(" Exiting.\n", stderr);
  exit(EXIT_FAILURE);
}

/** @brief Log a message to the log file and console (if required).

    For logging to file:

    If the log file is open (\c is_configured is TRUE), it calls
    should_log_to_file() to decide whether logging to file is wanted; calls
    log_file(), which calls log_to_file() if affirmative.

    If the log file is not open (\c is_configured is FALSE), it stores the
    message until the file is opened.

    For logging to the console:

    If should_log_to_console() returns TRUE, sends the log message to the main
    controller via TTCN_Communication::send_log() and possibly writes it to
    the standard error depending on the return value.

    @param [in] event
    @param [in] log_buffered
**/
void LegacyLogger::log(const TitanLoggerApi::TitanLogEvent& event,
    boolean log_buffered, boolean separate_file, boolean use_emergency_mask)
{
  const TTCN_Logger::Severity& severity = (const TTCN_Logger::Severity)(int)event.severity();

  if (separate_file) {
    log_file_emerg(event);
  } else {
    if (use_emergency_mask) {
      if (TTCN_Logger::should_log_to_emergency(severity)
          ||
          TTCN_Logger::should_log_to_file(severity))
      {
        log_file(event, log_buffered);
      }
      if (TTCN_Logger::should_log_to_console(severity)) {
        log_console(event, severity);
      }

    } else {
      if (TTCN_Logger::should_log_to_file(severity)) {
        log_file(event, log_buffered);
      }
      if (TTCN_Logger::should_log_to_console(severity)) {
        log_console(event, severity);
      }
    }
  }
}

boolean LegacyLogger::log_file_emerg(const TitanLoggerApi::TitanLogEvent& event)
{
  boolean write_success = TRUE;
  char *event_str = event_to_str(event);
  if (event_str == NULL) {
    TTCN_warning("No text for event");
    return TRUE;
  }
  size_t bytes_to_log = mstrlen(event_str);

  if (er_ == NULL) {
    char *filename_emergency = get_file_name(0);

    if (filename_emergency == NULL) {
      // Valid filename is not available, use specific one.
      filename_emergency = mcopystr("emergency.log");
    } else {
      filename_emergency = mputprintf(filename_emergency, "_emergency");
    }
    er_ = fopen(filename_emergency, "w");

    if (er_ == NULL)
      fatal_error("Opening of log file `%s' for writing failed.",
        filename_emergency);

    Free(filename_emergency);
  }

  write_success = TRUE;

  if (bytes_to_log > 0 && fwrite(event_str, bytes_to_log, 1, er_) != 1)
    write_success = FALSE;

  fputc('\n', er_);

  fflush(er_);

  Free(event_str);

  return write_success;
}

/** @brief Logs a message to the already opened log file.
    @pre The log file is already open.

    Writes the timestamp in the appropriate format, the event type (severity)
    if needed, the source information if available, the message and a newline.

    @param [in] event a properly initialized TITAN log event
    @param [in] log_buffered TRUE if this is a buffered message, used to avoid
                possible recursion of log_to_file() -> open_file() ->
                log_to_file() -> open_file() -> ...
**/
boolean LegacyLogger::log_file(const TitanLoggerApi::TitanLogEvent& event,
                            boolean log_buffered)
{
  if (!this->log_fp_) return FALSE;

  struct timeval event_timestamp = { (time_t)event.timestamp().seconds(),
    (suseconds_t)event.timestamp().microSeconds() };
  if (this->is_disk_full_) {
    if (this->disk_full_action_.type == TTCN_Logger::DISKFULL_RETRY) {
      struct timeval diff;
      // If the specified time period has elapsed retry logging to file.
      if (event_timestamp.tv_usec < this->disk_full_time_.tv_usec) {
        diff.tv_sec = event_timestamp.tv_sec -
          this->disk_full_time_.tv_sec - 1;
        diff.tv_usec = event_timestamp.tv_usec +
          (1000000L - this->disk_full_time_.tv_usec);
      } else {
        diff.tv_sec = event_timestamp.tv_sec - this->disk_full_time_.tv_sec;
        diff.tv_usec = event_timestamp.tv_usec -
          this->disk_full_time_.tv_usec;
      }
      if ((size_t)diff.tv_sec >= this->disk_full_action_.retry_interval)
        this->is_disk_full_ = FALSE;
      else return FALSE;
    } else {
      return FALSE;
    }
  }

  // Check if there is a size limitation and the file is not empty, if the
  // file is still empty the size limitation is ignored.  If the size
  // limitation is reached open a new log file and check the file number
  // limitation.  When writing the buffered messages the file size limitation
  // is ignored.  TODO: Replace length calculation with custom function call.
  // Event buffering is not necessarily implemented in the plug-ins.
  char *event_str = event_to_str(event);
  if (event_str == NULL) {
    TTCN_warning("No text for event");
    return TRUE;
  }
  size_t bytes_to_log = mstrlen(event_str) + 1;
  if (this->logfile_size_ != 0 && this->logfile_bytes_ != 0 && !log_buffered) {
    if ((bytes_to_log + this->logfile_bytes_ + 1023) / 1024 > this->logfile_size_) {
      // Close current log file and open the next one.
      close_file();
      this->logfile_index_++;
      // Delete oldest log file if there is a file number limitation.
      if (this->logfile_number_ > 1) {
        if (this->logfile_index_ > this->logfile_number_) {
          char* filename_to_delete =
            get_file_name(this->logfile_index_ - this->logfile_number_);
          remove(filename_to_delete);
          Free(filename_to_delete);
        }
      }
      open_file(FALSE);
    }
  }

  if (!log_buffered && (this->format_c_present_ || this->format_t_present_)) {
    switch (TTCN_Runtime::get_state()) {
    case TTCN_Runtime::HC_EXIT:
    case TTCN_Runtime::MTC_EXIT:
    case TTCN_Runtime::PTC_EXIT:
      // Can't call get_filename(), because it may call
      // TTCN_Runtime::get_host_name(), and TTCN_Runtime::clean_up() (which is
      // called once) has already happened.
      break;
    default: {
      char *new_filename = get_file_name(this->logfile_index_);
      if (strcmp(new_filename, this->current_filename_)) {
        // Different file name, have to switch.  Reuse timestamp etc. of the
        // current event.
        expstring_t switched = mprintf("Switching to log file `%s'",
                                       new_filename);
        TitanLoggerApi::TitanLogEvent switched_event;
        switched_event.timestamp() = event.timestamp();
        switched_event.sourceInfo__list() = event.sourceInfo__list();
        switched_event.severity() = (int)TTCN_Logger::EXECUTOR_RUNTIME;
        switched_event.logEvent().choice().unhandledEvent() =
          CHARSTRING(switched);
        log_file(switched_event, TRUE);
        Free(switched);
        close_file();
        open_file(FALSE);  // calls get_filename again :(
      }
      Free(new_filename);
      break; }
    }
  }

  // Write out the event_str, if it failed assume that the disk is full, then
  // act according to DiskFullAction.
  boolean print_success = log_to_file(event_str);
  if (!print_success) {
    switch (this->disk_full_action_.type) {
    case TTCN_Logger::DISKFULL_ERROR:
      fatal_error("Writing to log file failed.");
    case TTCN_Logger::DISKFULL_STOP:
      this->is_disk_full_ = TRUE;
      break;
    case TTCN_Logger::DISKFULL_RETRY:
      this->is_disk_full_ = TRUE;
      // Time of failure.  TODO: Find a better way to transfer the timestamp.
      this->disk_full_time_.tv_sec = event.timestamp().seconds();
      this->disk_full_time_.tv_usec = event.timestamp().microSeconds();
      break;
    case TTCN_Logger::DISKFULL_DELETE:
      // Try to delete older logfiles while writing fails, must leave at least
      // two log files.  Stop with error if cannot delete more files and
      // cannot write log.
      if (this->logfile_number_ == 0)
        this->logfile_number_ = this->logfile_index_;
      while (!print_success && this->logfile_number_ > 2) {
        this->logfile_number_--;
        if (this->logfile_index_ > this->logfile_number_) {
          char *filename_to_delete =
            get_file_name(this->logfile_index_ - this->logfile_number_);
          int remove_ret_val = remove(filename_to_delete);
          Free(filename_to_delete);
          // File deletion failed.
          if (remove_ret_val != 0) break;
          print_success = log_to_file(event_str);
        }
      }
      if (!print_success)
        fatal_error("Writing to log file failed.");
      else this->logfile_bytes_ += bytes_to_log;
      break;
    default:
      fatal_error("LegacyLogger::log(): invalid DiskFullAction type.");
    }
  } else {
    this->logfile_bytes_ += bytes_to_log;
  }
  Free(event_str);
  return TRUE;
}

boolean LegacyLogger::log_console(const TitanLoggerApi::TitanLogEvent& event,
                               const TTCN_Logger::Severity& severity)
{
  char *event_str = event_to_str(event, TRUE);
  if (event_str == NULL) {
    TTCN_warning("No text for event");
    return FALSE;
  }
  size_t event_str_len = mstrlen(event_str);
  if (!TTCN_Communication::send_log((time_t)event.timestamp().seconds(),
      (suseconds_t)event.timestamp().microSeconds(), severity,
      event_str_len, event_str)) {
    // The event text shall be printed to stderr when there is no control
    // connection towards MC (e.g. in single mode or in case of network
    // error).
    if (event_str_len > 0) {
      // Write the location info to the console for user logs only.
      // TODO: some switch to turn on/off source info on console.
      // Perhaps None/User/All?
      int stackdepth;
      if (TTCN_Logger::USER_UNQUALIFIED == severity && ':' == *event_str
        && (stackdepth = event.sourceInfo__list().lengthof()) > 0) {
        API::LocationInfo const& loc = event.sourceInfo__list()[stackdepth-1];
        if (fprintf(stderr, "%s:%d", (const char*)loc.filename(), (int)loc.line()) < 0)
          fatal_error("fprintf(sourceinfo) call failed on stderr. %s",
            strerror(errno));
      }

      if (fwrite(event_str, event_str_len, 1, stderr) != 1)
        fatal_error("fwrite(message) call failed on stderr. %s",
          strerror(errno));
    }
    if (putc('\n', stderr) == EOF)
      fatal_error("putc() call failed on stderr. %s", strerror(errno));
  }
  Free(event_str);
  // It's always...
  return TRUE;
}

boolean LegacyLogger::log_to_file(const char *message_ptr)
{
  // Retry and Delete: To avoid writing partial messages remember the initial
  // file position to be able to remove partial message if writing failed.
  fpos_t initial_pos;
  int fgetpos_ret_val = 0;
  if (this->disk_full_action_.type == TTCN_Logger::DISKFULL_RETRY ||
      this->disk_full_action_.type == TTCN_Logger::DISKFULL_DELETE) {
    fgetpos_ret_val = fgetpos(this->log_fp_, &initial_pos);
  }
  size_t message_len = strlen(message_ptr);
  boolean write_success = TRUE;
  if (message_len > 0 &&
      fwrite(message_ptr, message_len, 1, this->log_fp_) != 1)
    write_success = FALSE;
  if (write_success && putc('\n', this->log_fp_) == EOF) write_success = FALSE;
  if (write_success && fflush(this->log_fp_)) write_success = FALSE;
  if ((this->disk_full_action_.type == TTCN_Logger::DISKFULL_RETRY ||
       this->disk_full_action_.type == TTCN_Logger::DISKFULL_DELETE) &&
       !write_success && fgetpos_ret_val == 0) {
    // Overwrite with spaces until EOF.
    fsetpos(this->log_fp_, &initial_pos);
    while (!feof(this->log_fp_)) if (putc(' ', this->log_fp_) == EOF) break;
    fsetpos(this->log_fp_, &initial_pos);
  }
  return write_success;
}

char *append_header(char *ret_val,
  const struct timeval& timestamp, const TTCN_Logger::Severity severity,
  const char *sourceinfo)
{
  ret_val = TTCN_Logger::mputstr_timestamp(ret_val,
    TTCN_Logger::get_timestamp_format(), &timestamp);
  ret_val = mputc(ret_val, ' ');

  if (TTCN_Logger::get_log_event_types() != TTCN_Logger::LOGEVENTTYPES_NO) {
    ret_val = TTCN_Logger::mputstr_severity(ret_val, severity);
    if (TTCN_Logger::get_log_event_types() ==
        TTCN_Logger::LOGEVENTTYPES_SUBCATEGORIES)
      ret_val = mputprintf(ret_val, "_%s",
                           TTCN_Logger::severity_subcategory_names[severity]);
    ret_val = mputc(ret_val, ' ');
  }

  if (sourceinfo != NULL)
    ret_val = mputprintf(ret_val, "%s ", sourceinfo);

  return ret_val;
}

#if HAVE_GCC(4,6)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch-enum"
// Use the compiler to verify that all enum values are handled in switches.
// Wswitch-enum will report those even if there is a "default" label.
#endif

void timer_event_str(char *& ret_val, const TitanLoggerApi::TimerEvent_choice& tec)
{
  switch (tec.get_selection()) {
  case TitanLoggerApi::TimerEvent_choice::ALT_readTimer: {
    const TitanLoggerApi::TimerType& timer = tec.readTimer();
    ret_val = mputprintf(ret_val, "Read timer %s: %g s",
                         (const char *)timer.name(), (double)timer.value__());
    break; }

  case TitanLoggerApi::TimerEvent_choice::ALT_startTimer: {
    const TitanLoggerApi::TimerType& timer = tec.startTimer();
    ret_val = mputprintf(ret_val, "Start timer %s: %g s",
                         (const char *)timer.name(), (double)timer.value__());
    break; }

  case TitanLoggerApi::TimerEvent_choice::ALT_guardTimer: {
    const TitanLoggerApi::TimerGuardType& timer = tec.guardTimer();
    ret_val = mputprintf(ret_val, "Test case guard timer was set to %g s.",
                         (double)timer.value__());
    break; }

  case TitanLoggerApi::TimerEvent_choice::ALT_stopTimer: {
    const TitanLoggerApi::TimerType& timer = tec.stopTimer();
    ret_val = mputprintf(ret_val, "Stop timer %s: %g s",
                         (const char *)timer.name(), (double)timer.value__());
    break; }

  case TitanLoggerApi::TimerEvent_choice::ALT_timeoutTimer: {
    const TitanLoggerApi::TimerType& timer = tec.timeoutTimer();
    ret_val = mputprintf(ret_val, "Timeout %s: %g s",
                         (const char *)timer.name(), (double)timer.value__());
    break; }

  case TitanLoggerApi::TimerEvent_choice::ALT_timeoutAnyTimer: {
    ret_val = mputstr(ret_val, "Operation `any timer.timeout' was successful.");
    break; }

  case TitanLoggerApi::TimerEvent_choice::ALT_unqualifiedTimer: {
    ret_val = mputstr(ret_val, (const char *)tec.unqualifiedTimer());
    break; }

  case TitanLoggerApi::TimerEvent_choice::UNBOUND_VALUE:
  default:
    ret_val = NULL;
    return;
  }
}

void defaultop_event_str(char *& ret_val, const TitanLoggerApi::DefaultEvent_choice& dec)
{
  switch (dec.get_selection()) {
  case TitanLoggerApi::DefaultEvent_choice::ALT_defaultopActivate: {
    TitanLoggerApi::DefaultOp const& dflt = dec.defaultopActivate();
    ret_val = mputprintf(ret_val, "Altstep %s was activated as default, id %u",
      (const char*)dflt.name(), (int)dflt.id());
    break; }
  case TitanLoggerApi::DefaultEvent_choice::ALT_defaultopDeactivate: {
    TitanLoggerApi::DefaultOp const& dflt = dec.defaultopDeactivate();
    if (dflt.name().lengthof()) ret_val = mputprintf(ret_val,
      "Default with id %u (altstep %s) was deactivated.",
      (int)dflt.id(), (const char*)dflt.name());
    else ret_val = mputprintf(ret_val,
      "Deactivate operation on a null default reference was ignored.");
    break; }
  case TitanLoggerApi::DefaultEvent_choice::ALT_defaultopExit: {
    TitanLoggerApi::DefaultOp const& dflt = dec.defaultopExit();
    // First the common part
    ret_val = mputprintf(ret_val, "Default with id %u (altstep %s) ",
      (int)dflt.id(), (const char*)dflt.name());

    // Now the specific
    switch (dflt.end()) {
    case TitanLoggerApi::DefaultEnd::UNBOUND_VALUE:
    case TitanLoggerApi::DefaultEnd::UNKNOWN_VALUE:
      // Something has gone horribly wrong
      ret_val = NULL;
      return;
    case TitanLoggerApi::DefaultEnd::finish:
      ret_val = mputstr(ret_val, "finished. Skipping current alt statement "
        "or receiving operation.");
      break;
    case TitanLoggerApi::DefaultEnd::break__:
      ret_val = mputstr(ret_val, "has reached a repeat statement.");
      break;
    case TitanLoggerApi::DefaultEnd::repeat__:
      ret_val = mputstr(ret_val, "has reached a break statement."
        " Skipping current alt statement or receiving operation.");
      break;
    }
    break; }

  case TitanLoggerApi::DefaultEvent_choice::UNBOUND_VALUE:
  default:
    ret_val = NULL;
    return;
  }
}

void executor_event_str(char *& ret_val, const TitanLoggerApi::ExecutorEvent_choice& eec)
{
  switch (eec.get_selection()) {
  case API::ExecutorEvent_choice::ALT_executorRuntime: {
    TitanLoggerApi::ExecutorRuntime const& rt = eec.executorRuntime();
    // another monster-switch
    switch (rt.reason()) {
    case TitanLoggerApi::ExecutorRuntime_reason::UNBOUND_VALUE:
    case TitanLoggerApi::ExecutorRuntime_reason::UNKNOWN_VALUE:
      ret_val = NULL;
      return;
    case TitanLoggerApi::ExecutorRuntime_reason::connected__to__mc:
      ret_val = mputstr(ret_val, "Connected to MC.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::disconnected__from__mc:
      ret_val = mputstr(ret_val, "Disconnected from MC.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::initialization__of__modules__failed:
      ret_val = mputstr(ret_val, "Initialization of modules failed.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::exit__requested__from__mc__hc:
      ret_val = mputstr(ret_val, "Exit was requested from MC. Terminating HC.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::exit__requested__from__mc__mtc:
      ret_val = mputstr(ret_val, "Exit was requested from MC. Terminating MTC.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::stop__was__requested__from__mc:
      ret_val = mputstr(ret_val, "Stop was requested from MC.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::stop__was__requested__from__mc__ignored__on__idle__mtc:
      ret_val = mputstr(ret_val, "Stop was requested from MC. Ignored on idle MTC.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::stop__was__requested__from__mc__ignored__on__idle__ptc:
      ret_val = mputstr(ret_val, "Stop was requested from MC. Ignored on idle PTC.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::executing__testcase__in__module:
      ret_val = mputprintf(ret_val, "Executing test case %s in module %s.",
        (const char *)rt.testcase__name()(), (const char *)rt.module__name()());
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::performing__error__recovery:
      ret_val = mputstr(ret_val, "Performing error recovery.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::executor__start__single__mode:
      ret_val = mputstr(ret_val, "TTCN-3 Test Executor "
        "started in single mode. Version: " PRODUCT_NUMBER ".");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::executor__finish__single__mode:
      ret_val = mputstr(ret_val, "TTCN-3 Test Executor finished in single mode.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::exiting:
      ret_val = mputstr(ret_val, "Exiting");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::fd__limits:
      ret_val = mputprintf(ret_val, "Maximum number of open file descriptors: %i,"
        "   FD_SETSIZE = %li", (int)rt.pid()(), (long)rt.fd__setsize()());
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::host__controller__started:
      ret_val = mputprintf(ret_val, "TTCN-3 Host Controller started on %s."
        " Version: " PRODUCT_NUMBER ".", (const char *)rt.module__name()());
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::host__controller__finished:
      ret_val = mputstr(ret_val, "TTCN-3 Host Controller finished.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::initializing__module:
      ret_val = mputprintf(ret_val, "Initializing module %s.",
        (const char *)rt.module__name()());
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::initialization__of__module__finished:
      ret_val = mputprintf(ret_val, "Initialization of module %s finished.",
        (const char *)rt.module__name()());
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::mtc__created:
      ret_val = mputprintf(ret_val, "MTC was created. Process id: %ld.",
        (long)rt.pid()());
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::overload__check:
      ret_val = mputstr(ret_val, "Trying to create a dummy child process"
        " to verify if the host is still overloaded.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::overload__check__fail:
      ret_val = mputstr(ret_val, "Creation of the dummy child process failed.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::overloaded__no__more:
      // FIXME   ret_val = mputstr(ret_val, "");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::resuming__execution:
      ret_val = mputstr(ret_val, "Resuming execution.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::stopping__control__part__execution:
      ret_val = mputstr(ret_val, "Resuming control part execution.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::stopping__current__testcase:
      ret_val = mputstr(ret_val, "Stopping current testcase.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::stopping__test__component__execution:
      ret_val = mputstr(ret_val, "Stopping test component execution.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::terminating__execution:
      ret_val = mputstr(ret_val, "Terminating execution.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::user__paused__waiting__to__resume:
      ret_val = mputstr(ret_val, "User has paused execution. Waiting for continue.");
      break;
    case TitanLoggerApi::ExecutorRuntime_reason::waiting__for__ptcs__to__finish:
      ret_val = mputstr(ret_val, "Waiting for PTCs to finish.");
      break;
    }
    break; }

  case TitanLoggerApi::ExecutorEvent_choice::ALT_executorConfigdata: {
    TitanLoggerApi::ExecutorConfigdata const& cfg = eec.executorConfigdata();
    switch (cfg.reason()) {
    case TitanLoggerApi::ExecutorConfigdata_reason::UNBOUND_VALUE:
    case TitanLoggerApi::ExecutorConfigdata_reason::UNKNOWN_VALUE:
      ret_val = NULL;
      return;

    case TitanLoggerApi::ExecutorConfigdata_reason::received__from__mc:
      ret_val = mputstr(ret_val, "Processing configuration data received from MC.");
      break;
    case TitanLoggerApi::ExecutorConfigdata_reason::processing__failed:
      ret_val = mputstr(ret_val, "Processing of configuration data failed.");
      break;
    case TitanLoggerApi::ExecutorConfigdata_reason::processing__succeeded:
      ret_val = mputstr(ret_val, "Configuration data was processed successfully.");
      break;
    case TitanLoggerApi::ExecutorConfigdata_reason::module__has__parameters:
      // TODO: module logs its own parameters directly, change it ?
      break;
    case TitanLoggerApi::ExecutorConfigdata_reason::using__config__file:
      ret_val = mputprintf( ret_val, "Using configuration file: `%s'.",
        (const char *)cfg.param__()());
      break;
    case TitanLoggerApi::ExecutorConfigdata_reason::overriding__testcase__list:
      ret_val = mputprintf(ret_val, "Overriding testcase list: %s.",
        (const char *)cfg.param__()());
      break;
    }
    break; } // executorConfigdata

  case TitanLoggerApi::ExecutorEvent_choice::ALT_executorComponent: {
    TitanLoggerApi::ExecutorComponent const& cm = eec.executorComponent();
    switch (cm.reason()) {
    case TitanLoggerApi::ExecutorComponent_reason::UNBOUND_VALUE:
    case TitanLoggerApi::ExecutorComponent_reason::UNKNOWN_VALUE:
    default:
      ret_val = NULL;
      return;

    case TitanLoggerApi::ExecutorComponent_reason::mtc__started:
      ret_val = mputprintf(ret_val, "TTCN-3 Main Test Component started on %s."
        " Version: " PRODUCT_NUMBER ".", TTCN_Runtime::get_host_name());
      break;

    case TitanLoggerApi::ExecutorComponent_reason::mtc__finished:
      ret_val = mputstr(ret_val, "TTCN-3 Main Test Component finished.");
      break;

    case TitanLoggerApi::ExecutorComponent_reason::ptc__started:
      break;

    case TitanLoggerApi::ExecutorComponent_reason::ptc__finished:
      ret_val = mputstr(ret_val, "TTCN-3 Parallel Test Component finished.");
      break;

    case TitanLoggerApi::ExecutorComponent_reason::component__init__fail:
      ret_val = mputstr(ret_val, "Component type initialization failed. PTC terminates.");
      break;
    }
    break; } // executorComponent

  case TitanLoggerApi::ExecutorEvent_choice::ALT_executorMisc: {
    TitanLoggerApi::ExecutorUnqualified const& ex = eec.executorMisc();
    const char *name = ex.name(), *ip_addr_str = ex.addr();
    switch (ex.reason()) {
    case TitanLoggerApi::ExecutorUnqualified_reason::UNBOUND_VALUE:
    case TitanLoggerApi::ExecutorUnqualified_reason::UNKNOWN_VALUE:
    default:
      ret_val = NULL;
      return;
    case TitanLoggerApi::ExecutorUnqualified_reason::address__of__mc__was__set:
      if (strcmp(name, ip_addr_str)) { // the IP address has a canonical name
        ret_val = mputprintf(ret_val, "The address of MC was set to %s[%s]:%d.",
          name, ip_addr_str, (int)ex.port__());
      } else { // the IP address does not have a canonical name
        ret_val = mputprintf(ret_val, "The address of MC was set to %s:%d.",
          ip_addr_str, (int)ex.port__());
      }
      break;
    case TitanLoggerApi::ExecutorUnqualified_reason::address__of__control__connection:
      ret_val = mputprintf(ret_val,
        "The local IP address of the control connection to MC is %s.",
        ip_addr_str);
      break;
    case TitanLoggerApi::ExecutorUnqualified_reason::host__support__unix__domain__sockets:
      if (ex.port__() == 0) { // success
        ret_val = mputstr(ret_val, "This host supports UNIX domain sockets for local communication.");
      }
      else { // failed with the errno
        ret_val = mputstr(ret_val,
          "This host does not support UNIX domain sockets for local communication.");
        // TODO errno
      }
      break;
    case TitanLoggerApi::ExecutorUnqualified_reason::local__address__was__set:
      if (strcmp(name, ip_addr_str)) { // the IP address has a canonical name
        ret_val = mputprintf(ret_val,
          "The local address was set to %s[%s].", name, ip_addr_str);
      } else { // the IP address does not have a canonical name
        ret_val = mputprintf(ret_val,
          "The local address was set to %s.", ip_addr_str);
      }
      break;
    }
    break; }

  case TitanLoggerApi::ExecutorEvent_choice::ALT_logOptions: {
    // Append plug-in specific settings to the logger options.  It's still a
    // plain string with no structure.
    ret_val = mputstr(ret_val, eec.logOptions());
    char * pso = LegacyLogger::plugin_specific_settings();
    //FindPlugin
    ret_val = mputstr(ret_val, pso);
    Free(pso);
    break; }

  case TitanLoggerApi::ExecutorEvent_choice::ALT_extcommandStart:
    ret_val = mputprintf(ret_val, "Starting external command `%s'.",
      (const char*)eec.extcommandStart());
    break;
  case TitanLoggerApi::ExecutorEvent_choice::ALT_extcommandSuccess:
    ret_val = mputprintf(ret_val,
      "External command `%s' was executed successfully (exit status: 0).",
      (const char*)eec.extcommandSuccess());
    break;

  case API::ExecutorEvent_choice::UNBOUND_VALUE:
  default:
    ret_val = NULL;
    return;
  }
}

char *LegacyLogger::plugin_specific_settings()
{
  const char *disk_full_action_type_names[] =
    { "Error", "Stop", "Retry", "Delete" };
  char *disk_full_action_str = NULL;
  char *ret_val;
  if (myself->disk_full_action_.type == TTCN_Logger::DISKFULL_RETRY)
    disk_full_action_str = mprintf("Retry(%lu)",
      (unsigned long)myself->disk_full_action_.retry_interval);
  else
    disk_full_action_str = mcopystr(
      disk_full_action_type_names[myself->disk_full_action_.type]);
  ret_val = mprintf(
    " LogFileSize:=%lu; LogFileNumber:=%lu; DiskFullAction:=%s",
    (unsigned long)myself->logfile_size_,
    (unsigned long)myself->logfile_number_,
    disk_full_action_str);
  Free(disk_full_action_str);
  return ret_val;
}

void portevent_str(char *& ret_val, const TitanLoggerApi::PortEvent_choice& pec)
{
  switch (pec.get_selection()) {
  case TitanLoggerApi::PortEvent_choice::ALT_portQueue: {
    TitanLoggerApi::Port__Queue const& pq = pec.portQueue();
    switch (pq.operation()) {
    case API::Port__Queue_operation::UNKNOWN_VALUE:
    case API::Port__Queue_operation::UNBOUND_VALUE:
    default:
      ret_val = NULL;
      return; // can't happen
    case API::Port__Queue_operation::enqueue__msg:
      ret_val = mputstr(ret_val, "Message");
      goto enqueue_common;
      break;
    case API::Port__Queue_operation::enqueue__call:
      ret_val = mputstr(ret_val, "Call");
      goto enqueue_common;
      break;
    case API::Port__Queue_operation::enqueue__reply:
      ret_val = mputstr(ret_val, "Reply");
      goto enqueue_common;
      break;
    case API::Port__Queue_operation::enqueue__exception:
      ret_val = mputstr(ret_val, "Exception");
enqueue_common: {
      char * cmpts = COMPONENT::get_component_string(pq.compref());
      ret_val = mputprintf(ret_val, " enqueued on %s from %s%s%s id %u",
        (const char*)pq.port__name(), cmpts,
        (const char*)pq.address__(), (const char*)pq.param__(),
        (unsigned int)pq.msgid()
      );
      Free(cmpts);
      break; }
    case API::Port__Queue_operation::extract__msg:
      ret_val = mputstr(ret_val, "Message");
      goto extract_common;
      break;
    case API::Port__Queue_operation::extract__op:
      ret_val = mputstr(ret_val, "Operation");
extract_common:
      ret_val = mputprintf(ret_val, " with id %u was extracted from the queue of %s.",
        (unsigned int)pq.msgid(), (const char*)pq.port__name());
      break;
    }
    break; }

  case API::PortEvent_choice::ALT_portState: {
    API::Port__State const& ps = pec.portState();
    const char *what = NULL;
    switch (ps.operation()) {
    case API::Port__State_operation::UNKNOWN_VALUE:
    case API::Port__State_operation::UNBOUND_VALUE:
    default:
      ret_val = NULL;
      return; // can't happen

    case API::Port__State_operation::started:
      what = "started";
      break;
    case API::Port__State_operation::stopped:
      what = "stopped";
      break;
    case API::Port__State_operation::halted:
      what = "halted";
      break;
    }
    ret_val = mputprintf(ret_val, "Port %s was %s.",
      (const char*)ps.port__name(), what);
    break; }

  case API::PortEvent_choice::ALT_procPortSend: { // PORTEVENT_P[MC]OUT
    API::Proc__port__out const& ps = pec.procPortSend();
    const char * dest;
    if (ps.compref() == SYSTEM_COMPREF) { // it's mapped
      dest = ps.sys__name();
    }
    else {
      dest = COMPONENT::get_component_string(ps.compref());
    }

    switch (ps.operation()) {
    case API::Port__oper::UNBOUND_VALUE:
    case API::Port__oper::UNKNOWN_VALUE:
    default:
      ret_val = NULL;
      return;

    case API::Port__oper::call__op:
      ret_val = mputstr(ret_val, "Called");
      break;

    case API::Port__oper::reply__op:
      ret_val = mputstr(ret_val, "Replied");
      break;

    case API::Port__oper::exception__op:
      ret_val = mputstr(ret_val, "Raised");
      break;
    }
    ret_val = mputprintf(ret_val, " on %s to %s %s",
      (const char*)ps.port__name(),
      dest,
      (const char*)ps.parameter());
    if (ps.compref() != SYSTEM_COMPREF) Free(const_cast<char*>(dest));
    break; }

  case API::PortEvent_choice::ALT_procPortRecv: { // PORTEVENT_P[MC]IN
    API::Proc__port__in const& ps = pec.procPortRecv();
    const char *op2;
    switch (ps.operation()) {
    case API::Port__oper::UNBOUND_VALUE:
    case API::Port__oper::UNKNOWN_VALUE:
    default:
      ret_val = NULL;
      return;

    case API::Port__oper::call__op:
      ret_val = mputstr(ret_val, ps.check__() ? "Check-getcall" : "Getcall");
      op2 = "call";
      break;

    case API::Port__oper::reply__op:
      ret_val = mputstr(ret_val, ps.check__() ? "Check-getreply" : "Getreply");
      op2 = "reply";
      break;

    case API::Port__oper::exception__op:
      ret_val = mputstr(ret_val, ps.check__() ? "Check-catch" : "Catch");
      op2 = "exception";
      break;
    }

    const char *source = COMPONENT::get_component_string(ps.compref());
    ret_val = mputprintf(ret_val, " operation on port %s succeeded, %s from %s: %s id %d",
      (const char*)ps.port__name(), op2, source, (const char*)ps.parameter(),
      (int)ps.msgid());
    Free(const_cast<char*>(source));
    break; }

  case API::PortEvent_choice::ALT_msgPortSend: { // PORTEVENT_M[MC]SEND
    API::Msg__port__send const& ms = pec.msgPortSend();

    const char *dest = COMPONENT::get_component_string(ms.compref());
    ret_val = mputprintf(ret_val, "Sent on %s to %s%s",
      (const char*)ms.port__name(), dest, (const char*)ms.parameter());
    //if (ms.compref() != SYSTEM_COMPREF)
    Free(const_cast<char*>(dest));
    break; }

  case API::PortEvent_choice::ALT_msgPortRecv: { // PORTEVENT_M[MC]RECV
    API::Msg__port__recv const& ms = pec.msgPortRecv();
    switch (ms.operation()) {
    case API::Port__oper::UNBOUND_VALUE:
    case API::Port__oper::UNKNOWN_VALUE:
    default:
      ret_val = NULL;
      return;
    case API::Msg__port__recv_operation::receive__op:
      ret_val = mputstr(ret_val, "Receive");
      break;
    case API::Msg__port__recv_operation::check__receive__op:
      ret_val = mputstr(ret_val, "Check-receive");
      break;
    case API::Msg__port__recv_operation::trigger__op:
      ret_val = mputstr(ret_val, "Trigger");
      break;
    }
    ret_val = mputprintf(ret_val, " operation on port %s succeeded, message from ",
                         (const char *)ms.port__name());

    if (ms.compref() == SYSTEM_COMPREF) { // mapped
      const char *sys = ms.sys__name(); // not dynamic alloc
      ret_val = mputprintf(ret_val, "system(%s)", sys);
    } else {
      const char *sys = COMPONENT::get_component_string(ms.compref()); // dynamic alloc
      ret_val = mputstr(ret_val, sys);
      Free(const_cast<char *>(sys));
    }
    const char *ms_parameter = (const char *)ms.parameter();
    ret_val = mputprintf(ret_val, "%s id %d", ms_parameter, (int)ms.msgid());
    // Inconsistent dot...
    if (ms_parameter == NULL || strlen(ms_parameter) == 0)
      ret_val = mputc(ret_val, '.');
    break; }

  case API::PortEvent_choice::ALT_dualMapped: {
    API::Dualface__mapped const& dual = pec.dualMapped();
    ret_val = mputprintf(ret_val, "%s message was mapped to %s : %s",
      (dual.incoming() ? "Incoming" : "Outgoing"),
      (const char*)dual.target__type(), (const char*)dual.value__());

    if (dual.incoming()) {
      ret_val = mputprintf(ret_val, " id %d", (int)dual.msgid());
    }
    break; }

  case API::PortEvent_choice::ALT_dualDiscard: {
    API::Dualface__discard const& dualop = pec.dualDiscard();
    ret_val = mputprintf(ret_val, "%s message of type %s ",
      (dualop.incoming() ? "Incoming" : "Outgoing"),
      (const char*)dualop.target__type());

    ret_val = mputprintf(ret_val, (dualop.unhandled()
      ? "could not be handled by the type mapping rules on port %s."
        " The message was discarded."
      : "was discarded on port %s."),
      (const char*) dualop.port__name());
    break; }
  
  case API::PortEvent_choice::ALT_setState: {
    API::Setstate const& setstate = pec.setState();
    ret_val = mputprintf(ret_val, "The state of the %s port was changed by a setstate operation to %s.",
      (const char*)setstate.port__name(), (const char*)setstate.state());
    if (setstate.info().lengthof() != 0) {
      ret_val = mputprintf(ret_val, " Information: %s", (const char*)setstate.info());
    }
    break; }

  case API::PortEvent_choice::ALT_portMisc: {
    API::Port__Misc const& pmisc = pec.portMisc();
    const char *comp_str = COMPONENT::get_component_string(pmisc.remote__component());
    switch (pmisc.reason()) {
    case API::Port__Misc_reason::UNBOUND_VALUE:
    case API::Port__Misc_reason::UNKNOWN_VALUE:
    default:
      ret_val = NULL;
      return;

    case API::Port__Misc_reason::removing__unterminated__connection:
      ret_val = mputprintf(ret_val,
        "Removing unterminated connection between port %s and %s:%s.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::removing__unterminated__mapping:
      ret_val = mputprintf(ret_val,
        "Removing unterminated mapping between port %s and system:%s.",
        (const char*)pmisc.port__name(), (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::port__was__cleared:
      ret_val = mputprintf(ret_val, "Port %s was cleared.",
        (const char*)pmisc.port__name());
      break;
    case API::Port__Misc_reason::local__connection__established:
      ret_val = mputprintf(ret_val,
      "Port %s has established the connection with local port %s.",
      (const char*)pmisc.port__name(), (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::local__connection__terminated:
      ret_val = mputprintf(ret_val,
      "Port %s has terminated the connection with local port %s.",
      (const char*)pmisc.port__name(), (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::port__is__waiting__for__connection__tcp:
      ret_val = mputprintf(ret_val,
        "Port %s is waiting for connection from %s:%s on TCP port %s:%d.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port(),
        (const char*)pmisc.ip__address(), (int)pmisc.tcp__port());
      break;
    case API::Port__Misc_reason::port__is__waiting__for__connection__unix:
      ret_val = mputprintf(ret_val,
        "Port %s is waiting for connection from %s:%s on UNIX pathname %s.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port(),
        (const char*)pmisc.ip__address());
      break;
    case API::Port__Misc_reason::connection__established:
      ret_val = mputprintf(ret_val,
        "Port %s has established the connection with %s:%s using transport type %s.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port(),
        (const char*)pmisc.ip__address());
      break;
    case API::Port__Misc_reason::destroying__unestablished__connection:
      ret_val = mputprintf(ret_val,
        "Destroying unestablished connection of port %s to %s:%s"
        " because the other endpoint has terminated.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::terminating__connection:
      ret_val = mputprintf(ret_val,
        "Terminating the connection of port %s to %s:%s."
        " No more messages can be sent through this connection.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::sending__termination__request__failed:
      ret_val = mputprintf(ret_val, "Sending the connection termination request"
        " on port %s to remote endpoint %s:%s failed.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::termination__request__received:
      ret_val = mputprintf(ret_val,
        "Connection termination request was received on port %s from %s:%s."
        " No more data can be sent or received through this connection.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::acknowledging__termination__request__failed:
      ret_val = mputprintf(ret_val,
        "Sending the acknowledgment for connection termination request"
        " on port %s to remote endpoint %s:%s failed.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::sending__would__block:
      ret_val = mputprintf(ret_val,
        "Sending data on the connection of port %s to %s:%s would block execution."
        " The size of the outgoing buffer was increased from %d to %d bytes.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port(),
        (int)pmisc.tcp__port(), (int)pmisc.new__size());
      break;
    case API::Port__Misc_reason::connection__accepted:
      ret_val = mputprintf(ret_val, "Port %s has accepted the connection from %s:%s.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::connection__reset__by__peer:
      ret_val = mputprintf(ret_val, "Connection of port %s to %s:%s was reset by the peer.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::connection__closed__by__peer:
      ret_val = mputprintf(ret_val,
        "Connection of port %s to %s:%s was closed unexpectedly by the peer.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::port__disconnected:
      ret_val = mputprintf(ret_val, "Port %s was disconnected from %s:%s.",
        (const char*)pmisc.port__name(), comp_str, (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::port__was__mapped__to__system:
      ret_val = mputprintf(ret_val, "Port %s was mapped to system:%s.",
        (const char*)pmisc.port__name(), (const char*)pmisc.remote__port());
      break;
    case API::Port__Misc_reason::port__was__unmapped__from__system:
      ret_val = mputprintf(ret_val, "Port %s was unmapped from system:%s.",
        (const char*)pmisc.port__name(), (const char*)pmisc.remote__port());
      break;

    }
    Free(const_cast<char*>(comp_str));
    break; }


  default:
  case TitanLoggerApi::PortEvent_choice::UNBOUND_VALUE:
    break;
  }
}

void matchingop_str(char *& ret_val, const TitanLoggerApi::MatchingEvent_choice& choice)
{
  switch (choice.get_selection()) {
  case TitanLoggerApi::MatchingEvent_choice::ALT_matchingDone: {
    TitanLoggerApi::MatchingDoneType const& md = choice.matchingDone();
    switch (md.reason()) {
    case TitanLoggerApi::MatchingDoneType_reason::UNBOUND_VALUE:
    case TitanLoggerApi::MatchingDoneType_reason::UNKNOWN_VALUE:
    default:
      ret_val = NULL;
      return;
    case TitanLoggerApi::MatchingDoneType_reason::done__failed__no__return:
      ret_val = mputprintf(ret_val, "Done operation with type %s on PTC %d"
        " failed: The started function did not return a value.",
        (const char*)md.type__(), (int)md.ptc());
      break;
    case TitanLoggerApi::MatchingDoneType_reason::done__failed__wrong__return__type:
      ret_val = mputprintf(ret_val, "Done operation with type %s on PTC %d"
        " failed: The started function returned a value of type %s.",
        (const char*)md.type__(), (int)md.ptc(), (const char*)md.return__type());
      break;
    case TitanLoggerApi::MatchingDoneType_reason::any__component__done__successful:
      ret_val = mputstr(ret_val, "Operation 'any component.done' was successful.");
      break;
    case TitanLoggerApi::MatchingDoneType_reason::any__component__done__failed:
      ret_val = mputstr(ret_val, "Operation 'any component.done' "
        "failed because no PTCs were created in the testcase.");
      break;
    case TitanLoggerApi::MatchingDoneType_reason::all__component__done__successful:
      ret_val = mputstr(ret_val, "Operation 'all component.done' was successful.");
      break;
    case TitanLoggerApi::MatchingDoneType_reason::any__component__killed__successful:
      ret_val = mputstr(ret_val, "Operation 'any component.killed' was successful.");
      break;
    case TitanLoggerApi::MatchingDoneType_reason::any__component__killed__failed:
      ret_val = mputstr(ret_val, "Operation 'any component.killed' "
        "failed because no PTCs were created in the testcase.");
      break;
    case TitanLoggerApi::MatchingDoneType_reason::all__component__killed__successful:
      ret_val = mputstr(ret_val, "Operation 'all component.killed' was successful.");
      break;
    }
    break; }

  case TitanLoggerApi::MatchingEvent_choice::ALT_matchingFailure: {
    boolean is_call = FALSE;
    TitanLoggerApi::MatchingFailureType const& mf = choice.matchingFailure();
    switch (mf.reason()) {
    case TitanLoggerApi::MatchingFailureType_reason::UNBOUND_VALUE:
    case TitanLoggerApi::MatchingFailureType_reason::UNKNOWN_VALUE:
    default:
      ret_val = NULL;
      return; // can't happen
    case TitanLoggerApi::MatchingFailureType_reason::message__does__not__match__template:
      ret_val = mputprintf(ret_val,
      "Matching on port %s "
      "%s: First message in the queue does not match the template: ",
        (const char*)mf.port__name(), (const char*)mf.info());
      break;
    case TitanLoggerApi::MatchingFailureType_reason::exception__does__not__match__template:
      ret_val = mputprintf(ret_val,
        "Matching on port %s failed: "
        "The first exception in the queue does not match the template: %s",
        (const char*)mf.port__name(), (const char*)mf.info());
      break;
    case TitanLoggerApi::MatchingFailureType_reason::parameters__of__call__do__not__match__template:
      is_call = TRUE; // fall through
    case TitanLoggerApi::MatchingFailureType_reason::parameters__of__reply__do__not__match__template:
      ret_val = mputprintf(ret_val,
        "Matching on port %s failed: "
        "The parameters of the first %s in the queue do not match the template: %s",
        (is_call ? "call" : "reply"),
        (const char*)mf.port__name(), (const char*)mf.info());
      break;
    case TitanLoggerApi::MatchingFailureType_reason::sender__does__not__match__from__clause:
      ret_val = mputprintf(ret_val,
        "Matching on port %s failed: "
        "Sender of the first entity in the queue does not match the from clause: %s",
        (const char*)mf.port__name(), (const char*)mf.info());
      break;
    case TitanLoggerApi::MatchingFailureType_reason::sender__is__not__system:
      ret_val = mputprintf(ret_val,
        "Matching on port %s failed: "
        "Sender of the first entity in the queue is not the system.",
        (const char*)mf.port__name());
      break;
    case TitanLoggerApi::MatchingFailureType_reason::not__an__exception__for__signature:
      ret_val = mputprintf(ret_val,
        "Matching on port %s failed: "
        "The first entity in the queue is not an exception for signature %s.",
        (const char*)mf.port__name(), (const char*)mf.info());
      break;
    }
    break; }

  case TitanLoggerApi::MatchingEvent_choice::ALT_matchingSuccess: {
    TitanLoggerApi::MatchingSuccessType const& ms = choice.matchingSuccess();
    ret_val = mputprintf(ret_val, "Matching on port %s succeeded: %s",
      (const char*)ms.port__name(), (const char*)ms.info());
    break; }

  case TitanLoggerApi::MatchingEvent_choice::ALT_matchingProblem: {
    TitanLoggerApi::MatchingProblemType const& mp = choice.matchingProblem();
    ret_val = mputstr(ret_val, "Operation `");
    if (mp.any__port()) ret_val = mputstr(ret_val, "any port.");
    if (mp.check__()) ret_val = mputstr(ret_val, "check(");
    switch (mp.operation()) {
    case TitanLoggerApi::MatchingProblemType_operation::UNBOUND_VALUE:
    case TitanLoggerApi::MatchingProblemType_operation::UNKNOWN_VALUE:
      ret_val = NULL;
      return;
    case TitanLoggerApi::MatchingProblemType_operation::receive__:
      ret_val = mputstr(ret_val, "receive");
      break;
    case TitanLoggerApi::MatchingProblemType_operation::trigger__:
      ret_val = mputstr(ret_val, "trigger");
      break;
    case TitanLoggerApi::MatchingProblemType_operation::getcall__:
      ret_val = mputstr(ret_val, "getcall");
      break;
    case TitanLoggerApi::MatchingProblemType_operation::getreply__:
      ret_val = mputstr(ret_val, "getreply");
      break;
    case TitanLoggerApi::MatchingProblemType_operation::catch__:
      ret_val = mputstr(ret_val, "catch");
      break;
    case TitanLoggerApi::MatchingProblemType_operation::check__:
      ret_val = mputstr(ret_val, "check");
      break;
    }
    if (mp.check__()) ret_val = mputstr(ret_val, ")");
    ret_val = mputstr(ret_val, "' ");

    if (mp.port__name().is_bound()) ret_val = mputprintf(ret_val,
      "on port %s ", (const char*)mp.port__name());
    // we could also check that any__port is false

    ret_val = mputstr(ret_val, "failed: ");

    switch (mp.reason()) {
    case TitanLoggerApi::MatchingProblemType_reason::UNBOUND_VALUE:
    case TitanLoggerApi::MatchingProblemType_reason::UNKNOWN_VALUE:
      ret_val = NULL;
      return;
    case TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports:
      ret_val = mputstr(ret_val, "The test component does not have ports.");
      break;
    case TitanLoggerApi::MatchingProblemType_reason::no__incoming__signatures:
      ret_val = mputstr(ret_val, "The port type does not have any incoming signatures.");
      break;
    case TitanLoggerApi::MatchingProblemType_reason::no__incoming__types:
      ret_val = mputstr(ret_val, "The port type does not have any incoming message types.");
      break;
    case TitanLoggerApi::MatchingProblemType_reason::no__outgoing__blocking__signatures:
      ret_val = mputstr(ret_val, "The port type does not have any outgoing blocking signatures.");
      break;
    case TitanLoggerApi::MatchingProblemType_reason::no__outgoing__blocking__signatures__that__support__exceptions:
      ret_val = mputstr(ret_val, "The port type does not have any outgoing blocking signatures that support exceptions.");
      break;
    case TitanLoggerApi::MatchingProblemType_reason::port__not__started__and__queue__empty:
      ret_val = mputstr(ret_val, "Port is not started and the queue is empty.");
      break;
    }
    break; } // matchingProblem

  case TitanLoggerApi::MatchingEvent_choice::ALT_matchingTimeout: {
    TitanLoggerApi::MatchingTimeout const& mt = choice.matchingTimeout();
    if (mt.timer__name().ispresent()) {
      ret_val = mputprintf(ret_val, "Timeout operation on timer %s failed:"
        " The timer is not started.", (const char*)mt.timer__name()());
    }
    else {
      ret_val = mputstr(ret_val, "Operation `any timer.timeout' failed:"
        " The test component does not have active timers.");
    }
    break; }

  case TitanLoggerApi::MatchingEvent_choice::UNBOUND_VALUE:
  default:
    ret_val = NULL;
    return;
  }
}

void parallel_str(char *& ret_val, const TitanLoggerApi::ParallelEvent_choice& choice)
{
  switch (choice.get_selection()) {
  case TitanLoggerApi::ParallelEvent_choice::ALT_parallelPTC: {
    TitanLoggerApi::ParallelPTC const& ptc = choice.parallelPTC();
    switch (ptc.reason()) {
    case TitanLoggerApi::ParallelPTC_reason::UNKNOWN_VALUE:
    case TitanLoggerApi::ParallelPTC_reason::UNBOUND_VALUE:
    default:
      ret_val = NULL;
      return;
    case TitanLoggerApi::ParallelPTC_reason::init__component__start:
      ret_val = mputprintf(ret_val,
        "Initializing variables, timers and ports of component type %s.%s",
        (const char*)ptc.module__(), (const char*)ptc.name());
      if (ptc.tc__loc().lengthof()) ret_val = mputprintf(ret_val,
        " inside testcase %s", (const char*)ptc.tc__loc());
      ret_val = mputc(ret_val, '.');
      break;
    case TitanLoggerApi::ParallelPTC_reason::init__component__finish:
      ret_val = mputprintf(ret_val, "Component type %s.%s was initialized.",
        (const char*)ptc.module__(), (const char*)ptc.name());
      break;
    case TitanLoggerApi::ParallelPTC_reason::terminating__component:
      ret_val = mputprintf(ret_val, "Terminating component type %s.%s.",
        (const char*)ptc.module__(), (const char*)ptc.name());
      break;
    case TitanLoggerApi::ParallelPTC_reason::component__shut__down:
      ret_val = mputprintf(ret_val, "Component type %s.%s was shut down",
        (const char*)ptc.module__(), (const char*)ptc.name());
      if (ptc.tc__loc().lengthof()) ret_val = mputprintf(ret_val,
        " inside testcase %s", (const char*)ptc.tc__loc());
      ret_val = mputc(ret_val, '.');
      break;
    case TitanLoggerApi::ParallelPTC_reason::error__idle__ptc:
      ret_val = mputstr(ret_val,
        "Error occurred on idle PTC. The component terminates.");
      break;
    case TitanLoggerApi::ParallelPTC_reason::ptc__created:
      ret_val = mputprintf(ret_val, "PTC was created."
        " Component reference: %d, alive: %s, type: %s.%s",
        (int)ptc.compref(), (ptc.alive__pid() ? "yes" : "no"),
        (const char*)ptc.module__(), (const char*)ptc.name());
      // The "testcase" member is used for the component name
      if (ptc.compname().lengthof() != 0) ret_val = mputprintf(ret_val,
        ", component name: %s", (const char*)ptc.compname());
      if (ptc.tc__loc().lengthof() != 0) ret_val = mputprintf(ret_val,
      ", location: %s", (const char*)ptc.tc__loc());
      ret_val = mputc(ret_val, '.');
      break;

    case API::ParallelPTC_reason::ptc__created__pid:
      ret_val = mputprintf(ret_val, "PTC was created. Component reference: %d, "
        "component type: %s.%s", (int)ptc.compref(),
        (const char*)ptc.module__(), (const char*)ptc.name());
      if (ptc.compname().lengthof() != 0) ret_val = mputprintf(ret_val,
        ", component name: %s", (const char*)ptc.compname());
      if (ptc.tc__loc().lengthof() != 0) ret_val = mputprintf(ret_val,
        ", testcase name: %s", (const char*)ptc.tc__loc());
      ret_val = mputprintf(ret_val, ", process id: %d.", (int)ptc.alive__pid());
      break;

    case TitanLoggerApi::ParallelPTC_reason::function__started:
      ret_val = mputstr(ret_val, "Function was started.");
      break;
    case TitanLoggerApi::ParallelPTC_reason::function__stopped:
      ret_val = mputprintf(ret_val, "Function %s was stopped. PTC terminates.",
        (const char*)ptc.name());
      break;
    case TitanLoggerApi::ParallelPTC_reason::function__finished:
      ret_val = mputprintf(ret_val, "Function %s finished. PTC %s.",
        (const char*)ptc.name(),
        ptc.alive__pid() ? "remains alive and is waiting for next start" : "terminates");
      break;
    case TitanLoggerApi::ParallelPTC_reason::function__error:
      ret_val = mputprintf(ret_val, "Function %s finished with an error. PTC terminates.",
        (const char*)ptc.name());
      break;
    case TitanLoggerApi::ParallelPTC_reason::ptc__done:
      ret_val = mputprintf(ret_val, "PTC with component reference %d is done.",
        (int)ptc.compref());
      break;
    case TitanLoggerApi::ParallelPTC_reason::ptc__killed:
      ret_val = mputprintf(ret_val, "PTC with component reference %d is killed.",
        (int)ptc.compref());
      break;
    case TitanLoggerApi::ParallelPTC_reason::stopping__mtc:
      ret_val = mputstr(ret_val, "Stopping MTC. The current test case will be terminated.");
      break;
    case TitanLoggerApi::ParallelPTC_reason::ptc__stopped:
      ret_val = mputprintf(ret_val, "PTC with component reference %d was stopped.",
        (int)ptc.compref());
      break;
    case TitanLoggerApi::ParallelPTC_reason::all__comps__stopped:
      ret_val = mputstr(ret_val, "All components were stopped.");
      break;
    case TitanLoggerApi::ParallelPTC_reason::ptc__was__killed:
      ret_val = mputprintf(ret_val, "PTC with component reference %d was killed.",
        (int)ptc.compref());
      break;
    case TitanLoggerApi::ParallelPTC_reason::all__comps__killed:
      ret_val = mputstr(ret_val, "All components were killed.");
      break;
    case TitanLoggerApi::ParallelPTC_reason::kill__request__frm__mc:
      ret_val = mputstr(ret_val, "Kill was requested from MC. Terminating idle PTC.");
      break;
    case TitanLoggerApi::ParallelPTC_reason::mtc__finished:
      ret_val = mputstr(ret_val, "MTC finished.");
      goto mpt_ptc_together;
    case TitanLoggerApi::ParallelPTC_reason::ptc__finished: {
      if (ptc.compname().lengthof() != 0) {
        ret_val = mputprintf(ret_val, "PTC %s(%d) finished.",
          (const char*)ptc.compname(), (int)ptc.compref());
      } else {
        ret_val = mputprintf(ret_val, "PTC with component reference %d "
          "finished.", (int)ptc.compref());
      }
      mpt_ptc_together:
      ret_val = mputprintf(ret_val, " Process statistics: { "
        "process id: %d, ", (int)ptc.alive__pid());
      int statuscode = ptc.status();
      if (WIFEXITED(statuscode)) {
        ret_val = mputprintf(ret_val, "terminated normally, "
          "exit status: %d, ", WEXITSTATUS(statuscode));
      } else if (WIFSIGNALED(statuscode)) {
        int signal_number = WTERMSIG(statuscode);
        ret_val = mputprintf(ret_val, "terminated by a signal, "
          "signal number: %d (%s), ", signal_number,
          TTCN_Runtime::get_signal_name(signal_number));
#ifdef WCOREDUMP
        // this macro is not available on all platforms
        if (WCOREDUMP(statuscode))
          ret_val = mputstr(ret_val, "core dump was created, ");
#endif
      } else {
        ret_val = mputprintf(ret_val, "termination reason and status is "
          "unknown: %d, ", statuscode);
      }
      ret_val = mputstr(ret_val, (const char*)ptc.tc__loc());
      break; }
    case TitanLoggerApi::ParallelPTC_reason::starting__function:
      break;
    }
    break; } // parallelPTC

  case TitanLoggerApi::ParallelEvent_choice::ALT_parallelPTC__exit: {
    TitanLoggerApi::PTC__exit const& px = choice.parallelPTC__exit();
    const int compref = px.compref();
    if (compref == MTC_COMPREF) {
      ret_val = mputstr(ret_val, "MTC finished.");
    } else {
      const char *comp_name = COMPONENT::get_component_name(px.compref());
      if (comp_name) ret_val = mputprintf(ret_val, "PTC %s(%d) finished.",
        comp_name, compref);
      else ret_val = mputprintf(ret_val, "PTC with component reference %d "
        "finished.", compref);
    }
    ret_val = mputprintf(ret_val, " Process statistics: { process id: %d, ",
      (int)px.pid());

    //ret_val =
    break; }

  case TitanLoggerApi::ParallelEvent_choice::ALT_parallelPort: {
    TitanLoggerApi::ParPort const& pp = choice.parallelPort();
    const char *direction = "on";
    const char *preposition = "and";
    switch(pp.operation()) {
    case API::ParPort_operation::UNBOUND_VALUE:
    case API::ParPort_operation::UNKNOWN_VALUE:
    default:
      ret_val = NULL;
      return;
    case API::ParPort_operation::connect__:
      ret_val = mputstr(ret_val, "Connect");
      break;
    case API::ParPort_operation::disconnect__:
      ret_val = mputstr(ret_val, "Disconnect");
      break;
    case API::ParPort_operation::map__:
      ret_val = mputstr(ret_val, "Map");
      direction = "of";
      preposition = "to";
      break;
    case API::ParPort_operation::unmap__:
      ret_val = mputstr(ret_val, "Unmap");
      direction = "of";
      preposition = "from";
      break;
    }
    char *src = COMPONENT::get_component_string(pp.srcCompref());
    char *dst = COMPONENT::get_component_string(pp.dstCompref());
    ret_val = mputprintf(ret_val, " operation %s %s:%s %s %s:%s finished.",
      direction, src, (const char*)pp.srcPort(), preposition,
      dst, (const char*)pp.dstPort());
    Free(src);
    Free(dst);
    break; }

  case TitanLoggerApi::ParallelEvent_choice::UNBOUND_VALUE:
  default:
    ret_val = NULL;
    return;
  }
}

void testcaseop_str(char *& ret_val, const TitanLoggerApi::TestcaseEvent_choice& choice)
{
  switch (choice.get_selection()) {
  case TitanLoggerApi::TestcaseEvent_choice::ALT_testcaseStarted: {
    ret_val = mputprintf(ret_val, "Test case %s started.",
      (const char *)choice.testcaseStarted().testcase__name());
    break; }

  case TitanLoggerApi::TestcaseEvent_choice::ALT_testcaseFinished: {
    const TitanLoggerApi::TestcaseType& testcase = choice.testcaseFinished();
    ret_val = mputprintf(ret_val, "Test case %s finished. Verdict: %s",
                         (const char *)testcase.name().testcase__name(),
                         verdict_name[testcase.verdict()]);
    if (testcase.reason().lengthof() > 0) {
      ret_val = mputprintf(ret_val, " reason: %s", (const char *)testcase.reason());
    }
    break; }

  case TitanLoggerApi::TestcaseEvent_choice::UNBOUND_VALUE:
  default:
    ret_val = NULL;
    return;
  }
}

void verdictop_str(char *& ret_val, const TitanLoggerApi::VerdictOp_choice& choice)
{
  switch (choice.get_selection()) {
  case TitanLoggerApi::VerdictOp_choice::ALT_setVerdict: {
    const TitanLoggerApi::SetVerdictType& set = choice.setVerdict();
    if (set.newVerdict() > set.oldVerdict()) { // the verdict is changing
      if (!set.oldReason().ispresent() && !set.newReason().ispresent()) {
        ret_val = mputprintf(ret_val, "setverdict(%s): %s -> %s",
          verdict_name[set.newVerdict()], verdict_name[set.oldVerdict()],
          verdict_name[set.localVerdict()]);
      } else {
        // The reason text (oldReason() and newReason()) is always the same
        // for some reason.  Why?
        ret_val = mputprintf(ret_val, "setverdict(%s): %s -> %s reason: "
          "\"%s\", new component reason: \"%s\"",
          verdict_name[set.newVerdict()], verdict_name[set.oldVerdict()],
          verdict_name[set.localVerdict()], (const char *)set.oldReason()(),
          (const char *)set.newReason()());
      }
    } else {
      if (!set.oldReason().ispresent() && !set.newReason().ispresent()) {
        ret_val = mputprintf(ret_val, "setverdict(%s): %s -> %s, component "
          "reason not changed", verdict_name[set.newVerdict()],
          verdict_name[set.oldVerdict()], verdict_name[set.localVerdict()]);
      } else {
        ret_val = mputprintf(ret_val, "setverdict(%s): %s -> %s reason: "
          "\"%s\", component reason not changed",
          verdict_name[set.newVerdict()], verdict_name[set.oldVerdict()],
          verdict_name[set.localVerdict()], (const char *)set.oldReason()());
      }
    }
    break; }

  case TitanLoggerApi::VerdictOp_choice::ALT_getVerdict: {
    ret_val = mputprintf(ret_val, "getverdict: %s",
                         verdict_name[choice.getVerdict()]);
    break; }

  case TitanLoggerApi::VerdictOp_choice::ALT_finalVerdict: {
    switch (choice.finalVerdict().choice().get_selection()) {
    case TitanLoggerApi::FinalVerdictType_choice::ALT_info: {
      const TitanLoggerApi::FinalVerdictInfo& info = choice.finalVerdict().choice().info();
      if (info.is__ptc()) {
        if ((int)info.ptc__compref().ispresent() && (int)info.ptc__compref()() != UNBOUND_COMPREF) {
          if (info.ptc__name().ispresent() && info.ptc__name()().lengthof() > 0) {
            ret_val = mputprintf(ret_val, "Local verdict of PTC %s(%d): ",
              (const char *)info.ptc__name()(), (int)info.ptc__compref()());
          } else {
            ret_val = mputprintf(ret_val, "Local verdict of PTC with "
                                 "component reference %d: ", (int)info.ptc__compref()());
          }
          ret_val = mputprintf(ret_val, "%s (%s -> %s)",
            verdict_name[info.ptc__verdict()], verdict_name[info.local__verdict()],
            verdict_name[info.new__verdict()]);
          if (info.verdict__reason().ispresent() &&
              info.verdict__reason()().lengthof() > 0)
            ret_val = mputprintf(ret_val, " reason: \"%s\"",
                                 (const char *)info.verdict__reason()());
        } else {
          ret_val = mputprintf(ret_val, "Final verdict of PTC: %s",
                               verdict_name[info.local__verdict()]);
          if (info.verdict__reason().ispresent() &&
              info.verdict__reason()().lengthof() > 0) {
            ret_val = mputprintf(ret_val, " reason: \"%s\"",
                                 (const char *)info.verdict__reason()());
          }
        }
      } else {
        ret_val = mputprintf(ret_val, "Local verdict of MTC: %s",
                             verdict_name[info.local__verdict()]);
        if (info.verdict__reason().ispresent() &&
            info.verdict__reason()().lengthof() > 0) {
          ret_val = mputprintf(ret_val, " reason: \"%s\"",
                               (const char *)info.verdict__reason()());
        }
      }
      break; }
    case TitanLoggerApi::FinalVerdictType_choice::ALT_notification:
      switch (choice.finalVerdict().choice().notification()) {
      case TitanLoggerApi::FinalVerdictType_choice_notification::no__ptcs__were__created:
        ret_val = mputstr(ret_val, "No PTCs were created.");
        break;
      case TitanLoggerApi::FinalVerdictType_choice_notification::setting__final__verdict__of__the__test__case:
        ret_val = mputstr(ret_val, "Setting final verdict of the test case.");
        break;
      case TitanLoggerApi::FinalVerdictType_choice_notification::UNBOUND_VALUE:
      case TitanLoggerApi::FinalVerdictType_choice_notification::UNKNOWN_VALUE:
      default:
        ret_val = NULL;
        break; }
      break;
    case TitanLoggerApi::FinalVerdictType_choice::UNBOUND_VALUE:
    default:
      ret_val = NULL;
      break; }
    break; }

  case TitanLoggerApi::VerdictOp_choice::UNBOUND_VALUE:
  default:
    ret_val = NULL;
    return;
  }
}

void statistics_str(char *& ret_val, const TitanLoggerApi::StatisticsType_choice& choice)
{
  switch (choice.get_selection()) {
  case TitanLoggerApi::StatisticsType_choice::ALT_verdictStatistics: {
    const TitanLoggerApi::StatisticsType_choice_verdictStatistics& statistics =
      choice.verdictStatistics();
    unsigned long none_count = (size_t)statistics.none__(),
      pass_count   = (size_t)statistics.pass__(),
      inconc_count = (size_t)statistics.inconc__(),
      fail_count   = (size_t)statistics.fail__(),
      error_count  = (size_t)statistics.error__();
    if (none_count > 0 || pass_count > 0 || inconc_count > 0 ||
        fail_count > 0 || error_count > 0) {
      ret_val = mputprintf(ret_val,
        "Verdict statistics: %lu none (%.2f %%), %lu pass (%.2f %%), %lu inconc "
        "(%.2f %%), %lu fail (%.2f %%), %lu error (%.2f %%).",
        none_count,   (float)statistics.nonePercent(),
        pass_count,   (float)statistics.passPercent(),
        inconc_count, (float)statistics.inconcPercent(),
        fail_count,   (float)statistics.failPercent(),
        error_count,  (float)statistics.errorPercent());
    } else {
      ret_val = mputstr(ret_val,
        "Verdict statistics: 0 none, 0 pass, 0 inconc, 0 fail, 0 error.");
    }
    break; }

  case TitanLoggerApi::StatisticsType_choice::ALT_controlpartStart:
    ret_val = mputprintf(ret_val, "Execution of control part in module %s %sed.",
      (const char*)choice.controlpartStart(), "start");
    break;

  case TitanLoggerApi::StatisticsType_choice::ALT_controlpartFinish:
    // The compiler might fold the two identical format strings
    ret_val = mputprintf(ret_val, "Execution of control part in module %s %sed.",
      (const char*)choice.controlpartFinish(), "finish");
    break;

  case TitanLoggerApi::StatisticsType_choice::ALT_controlpartErrors:
    ret_val = mputprintf(ret_val, "Number of errors outside test cases: %u",
      (unsigned int)choice.controlpartErrors());
    break;

  case TitanLoggerApi::StatisticsType_choice::UNBOUND_VALUE:
  default:
    ret_val = NULL;
    return;
  }
}

char *event_to_str(const TitanLoggerApi::TitanLogEvent& event,
                                 boolean without_header)
{
  char *ret_val = NULL;
  if (!without_header) {
    struct timeval timestamp = { (time_t)event.timestamp().seconds(),
      (suseconds_t)event.timestamp().microSeconds() };
    char *sourceinfo = NULL;
    if (event.sourceInfo__list().is_bound()) {
      TTCN_Logger::source_info_format_t source_info_format =
        TTCN_Logger::get_source_info_format();
      int stack_size = event.sourceInfo__list().size_of();
      if (stack_size > 0) {
        int i = 0;
        switch (source_info_format) {
        case TTCN_Logger::SINFO_NONE:
          i = stack_size; // start == end; nothing printed
          break;
        case TTCN_Logger::SINFO_SINGLE:
          i = stack_size - 1; // print just the last one
          break;
        case TTCN_Logger::SINFO_STACK:
          // do nothing, start from the beginning
          break;
        }

        for (; i < stack_size; ++i) {
          API::LocationInfo const& loc = event.sourceInfo__list()[i];

          if (sourceinfo) sourceinfo = mputstr(sourceinfo, "->");
          const char *file_name = loc.filename();
          int line_number = loc.line();
          const char *entity_name = loc.ent__name();
          API::LocationInfo_ent__type entity_type = loc.ent__type();
          sourceinfo = mputprintf(sourceinfo, "%s:%u", file_name, line_number);
          if (TTCN_Logger::get_log_entity_name()) {
            switch (entity_type) {
            case API::LocationInfo_ent__type::controlpart:
              sourceinfo = mputprintf(sourceinfo, "(controlpart:%s)", entity_name);
              break;
            case API::LocationInfo_ent__type::testcase__:
              sourceinfo = mputprintf(sourceinfo, "(testcase:%s)", entity_name);
              break;
            case API::LocationInfo_ent__type::altstep__:
              sourceinfo = mputprintf(sourceinfo, "(altstep:%s)", entity_name);
              break;
            case API::LocationInfo_ent__type::function__:
              sourceinfo = mputprintf(sourceinfo, "(function:%s)", entity_name);
              break;
            case API::LocationInfo_ent__type::external__function:
              sourceinfo = mputprintf(sourceinfo, "(externalfunction:%s)", entity_name);
              break;
            case API::LocationInfo_ent__type::template__:
              sourceinfo = mputprintf(sourceinfo, "(template:%s)", entity_name);
              break;
            case API::LocationInfo_ent__type::UNBOUND_VALUE:
            case API::LocationInfo_ent__type::UNKNOWN_VALUE:
            case API::LocationInfo_ent__type::unknown:
            default:
              break;
            }
          }
        }
      } else {
        if (source_info_format == TTCN_Logger::SINFO_SINGLE ||
          source_info_format == TTCN_Logger::SINFO_STACK)
          sourceinfo = mputc(sourceinfo, '-');
      }
    }

    ret_val = append_header(ret_val, timestamp,
      (const TTCN_Logger::Severity)(int)event.severity(), sourceinfo);
    Free(sourceinfo);
  }

  // Big, ugly switch to append the event message.
  // TODO: We need a fancier implementation.
  const TitanLoggerApi::LogEventType_choice& choice = event.logEvent().choice();
  switch (choice.get_selection()) {
  case TitanLoggerApi::LogEventType_choice::UNBOUND_VALUE:
    return NULL;

  case TitanLoggerApi::LogEventType_choice::ALT_unhandledEvent: {
    ret_val = mputstr(ret_val,
      (const char *)choice.unhandledEvent());
    break; }

  case API::LogEventType_choice::ALT_timerEvent:
    timer_event_str(ret_val, choice.timerEvent().choice());
    break;

  case TitanLoggerApi::LogEventType_choice::ALT_statistics:
    statistics_str(ret_val, choice.statistics().choice());
    break;

  case TitanLoggerApi::LogEventType_choice::ALT_verdictOp:
    verdictop_str(ret_val, choice.verdictOp().choice());
    break;


  case TitanLoggerApi::LogEventType_choice::ALT_testcaseOp:
    testcaseop_str(ret_val, choice.testcaseOp().choice());
    break;

  case TitanLoggerApi::LogEventType_choice::ALT_actionEvent:
    // fall through
  case TitanLoggerApi::LogEventType_choice::ALT_userLog: {
    API::Strings_str__list const& slist =
      (choice.get_selection() == TitanLoggerApi::LogEventType_choice::ALT_userLog)
      ? choice.userLog().str__list()
      : choice.actionEvent().str__list();
    for (int i = 0, size = slist.size_of(); i < size; ++i) {
      ret_val = mputstr(ret_val, (const char *)slist[i]);
    }
    break; }

  case TitanLoggerApi::LogEventType_choice::ALT_debugLog: {
    // TODO: We support multiple type of DEBUG_* events.
    ret_val = mputstr(ret_val, (const char *)choice.debugLog().text());
    break; }

  case TitanLoggerApi::LogEventType_choice::ALT_errorLog: {
    ret_val = mputstr(ret_val, (const char *)choice.errorLog().text());
    break; }

  case TitanLoggerApi::LogEventType_choice::ALT_warningLog: {
    ret_val = mputstr(ret_val, (const char *)choice.warningLog().text());
    break; }

  case TitanLoggerApi::LogEventType_choice::ALT_defaultEvent:
    defaultop_event_str(ret_val, choice.defaultEvent().choice());
    break;

  case TitanLoggerApi::LogEventType_choice::ALT_executionSummary: {
    // FIXME why is this empty?
    break; }

  case TitanLoggerApi::LogEventType_choice::ALT_executorEvent: {
    // En Taro Adun, executor!
    executor_event_str(ret_val, choice.executorEvent().choice());
    break; } // executorRuntime

  case TitanLoggerApi::LogEventType_choice::ALT_matchingEvent:
    matchingop_str(ret_val, choice.matchingEvent().choice());
    break;

  case TitanLoggerApi::LogEventType_choice::ALT_functionEvent:
    switch (choice.functionEvent().choice().get_selection()) {
    case TitanLoggerApi::FunctionEvent_choice::ALT_random: {
      TitanLoggerApi::FunctionEvent_choice_random const& ra = choice.functionEvent().choice().random();
      switch (ra.operation()) {
      case TitanLoggerApi::RandomAction::seed: {
      long integer_seed = ra.intseed();
      ret_val = mputprintf(ret_val, "Random number generator was initialized with seed %f: "
      "srand48(%ld).", (double)ra.retval(), integer_seed);
        break; }
      case TitanLoggerApi::RandomAction::read__out:
        ret_val = mputprintf(ret_val, "Function rnd() returned %f.", (double)ra.retval());
        break;
      case TitanLoggerApi::RandomAction::UNBOUND_VALUE:
      case TitanLoggerApi::RandomAction::UNKNOWN_VALUE:
      default:
        return NULL; // something went wrong
        break;
      }
      break; }

    case TitanLoggerApi::FunctionEvent_choice::ALT_unqualified:
      break;

    case TitanLoggerApi::FunctionEvent_choice::UNBOUND_VALUE:
    default:
      break;
    }
    break;

  case TitanLoggerApi::LogEventType_choice::ALT_parallelEvent:
    parallel_str(ret_val, choice.parallelEvent().choice());
    break;

  case TitanLoggerApi::LogEventType_choice::ALT_portEvent:
    portevent_str(ret_val, choice.portEvent().choice());
    break;

  default:
    return NULL; // can't happen
  } // the big ugly switch
  return ret_val;
}

#if HAVE_GCC(4,6)
#pragma GCC diagnostic pop
#endif

void LegacyLogger::open_file(boolean is_first)
{
  if (is_first) {
    chk_logfile_data();
    if (!this->skeleton_given_)
      set_file_name(TTCN_Runtime::is_single()
      ? (this->logfile_number_ == 1 ? "%e.%s"       : "%e-part%i.%s")
      : (this->logfile_number_ == 1 ? "%e.%h-%r.%s" : "%e.%h-%r-part%i.%s"),
      FALSE);
  }
  // Logging_Bits have only effect on the actual plug-in.  (Filtering is
  // implemented at higher level.)
  Free(this->current_filename_);
  this->current_filename_ = get_file_name(this->logfile_index_);
  if (this->current_filename_ != NULL) {
    create_parent_directories(this->current_filename_);
    this->log_fp_ = fopen(this->current_filename_,
                          this->append_file_ ? "a" : "w");
    if (this->log_fp_ == NULL)
      fatal_error("Opening of log file `%s' for writing failed.",
                  this->current_filename_);
    if (!TTCN_Communication::set_close_on_exec(fileno(this->log_fp_))) {
      fclose(this->log_fp_);
      fatal_error("Setting the close-on-exec flag failed on log file `%s'.",
                  this->current_filename_);
    }
  }

  this->is_configured_= TRUE;
  this->logfile_bytes_ = 0;
}

void LegacyLogger::close_file()
{
  if (!this->log_fp_) return;
  fclose(this->log_fp_);
  this->log_fp_ = NULL;
}

/** @brief Construct the log file name, performs substitutions.

    @return NULL if filename_skeleton is NULL or if the result would have been
    the empty string.
    @return an expstring_t with the actual filename.  The caller is
    responsible for deallocating it with Free().
**/
char *LegacyLogger::get_file_name(size_t idx)
{
  if (this->filename_skeleton_ == NULL) return NULL;
  enum { SINGLE, HC, MTC, PTC } whoami;
  if (TTCN_Runtime::is_single()) whoami = SINGLE;
  else if (TTCN_Runtime::is_hc()) whoami = HC;
  else if (TTCN_Runtime::is_mtc()) whoami = MTC;
  else whoami = PTC;
  boolean h_present = FALSE, p_present = FALSE, r_present = FALSE,
       i_present = FALSE;
  this->format_c_present_ = FALSE;
  this->format_t_present_ = FALSE;
  char *ret_val = memptystr();
  for (size_t i = 0; this->filename_skeleton_[i] != '\0'; i++) {
    if (this->filename_skeleton_[i] != '%') {
      ret_val = mputc(ret_val, this->filename_skeleton_[i]);
      continue;
    }
    switch (this->filename_skeleton_[++i]) {
    case 'c': // %c -> name of the current testcase (only on PTCs)
      ret_val = mputstr(ret_val, TTCN_Runtime::get_testcase_name());
      this->format_c_present_ = TRUE;
      break;
    case 'e': // %e -> name of executable
      ret_val = mputstr(ret_val, TTCN_Logger::get_executable_name());
      break;
    case 'h': // %h -> hostname
      ret_val = mputstr(ret_val, TTCN_Runtime::get_host_name());
      h_present = TRUE;
      break;
    case 'l': { // %l -> login name
      setpwent();
      struct passwd *p = getpwuid(getuid());
      if (p != NULL) ret_val = mputstr(ret_val, p->pw_name);
      endpwent();
      break; }
    case 'n': // %n -> component name (optional)
      switch (whoami) {
      case SINGLE:
      case MTC:
        ret_val = mputstr(ret_val, "MTC");
        break;
      case HC:
        ret_val = mputstr(ret_val, "HC");
        break;
      case PTC:
        ret_val = mputstr(ret_val, TTCN_Runtime::get_component_name());
        break;
      default:
        break; }
      break;
    case 'p': // %p -> process id
      ret_val = mputprintf(ret_val, "%ld", (long)getpid());
      p_present = TRUE;
      break;
    case 'r': // %r -> component reference
      switch (whoami) {
      case SINGLE:
        ret_val = mputstr(ret_val, "single");
        break;
      case HC:
        ret_val = mputstr(ret_val, "hc");
        break;
      case MTC:
        ret_val = mputstr(ret_val, "mtc");
        break;
      case PTC:
      default:
        ret_val = mputprintf(ret_val, "%d", (component)self);
        break;
      }
      r_present = TRUE;
      break;
    case 's': // %s -> default suffix (currently: always "log")
      ret_val = mputstr(ret_val, "log");
      break;
    case 't': // %t -> component type (only on PTCs)
      ret_val = mputstr(ret_val, TTCN_Runtime::get_component_type());
      this->format_t_present_ = TRUE;
      break;
    case 'i': // %i -> log file index
      if (this->logfile_number_ != 1)
        ret_val = mputprintf(ret_val, "%lu", (unsigned long)idx);
        i_present = TRUE;
        break;
    case '\0': // trailing single %: leave as it is
      i--; // to avoid over-indexing in next iteration
      // no break
    case '%': // escaping: %% -> %
      ret_val = mputc(ret_val, '%');
      break;
    default: // unknown sequence: leave as it is
      ret_val = mputc(ret_val, '%');
      ret_val = mputc(ret_val, this->filename_skeleton_[i]);
      break;
    }
  }

  static boolean already_warned = FALSE;
  if (ret_val[0] == '\0') { // result is empty
    Free(ret_val);
    ret_val = NULL;
  } else if (whoami == HC && !already_warned) {
    already_warned = TRUE;
    if (!h_present || (!p_present && !r_present))
      TTCN_warning("Skeleton `%s' does not guarantee unique log file name "
                   "for every test system process. It may cause "
                   "unpredictable results if several test components try to "
                   "write into the same log file.", this->filename_skeleton_);
  }
  if (this->logfile_number_ != 1 && !i_present) {
    TTCN_warning("LogFileNumber = %lu, but `%%i' is missing from the log "
                 "file name skeleton. `%%i' was appended to the skeleton.",
                 (unsigned long)this->logfile_number_);
    this->filename_skeleton_ = mputstr(this->filename_skeleton_, "%i");
    ret_val = mputprintf(ret_val, "%lu", (unsigned long)idx);
  }
  return ret_val;
}

/** @brief Create all directories in a full path.

    Ensures that all directories in the specified path exist (similar to mkdir
    -p).  Existing directories are skipped, non-existing directories are
    created.  Calls fatal_error() if a directory cannot be stat'ed or a
    non-existing directory cannot be created.

    @param path_name full path to log file name
**/
void LegacyLogger::create_parent_directories(const char *path_name)
{
  char *path_backup = NULL;
  boolean umask_saved = FALSE;
  mode_t old_umask = 0;
  size_t i = 0;
  // if path_name is absolute skip the leading '/'(s)
  while (path_name[i] == '/') i++;
  while (path_name[i] != '\0') {
    if (path_name[i] != '/') {
      i++;
      continue;
    }
    // path_name up to index i should contain a directory name
    if (path_backup == NULL) path_backup = mcopystr(path_name);
    path_backup[i] = '\0';
    struct stat buf;
    if (stat(path_backup, &buf) < 0) {
      if (errno == ENOENT) {
        // if the directory does not exist: create it
        errno = 0;
        if (!umask_saved) {
          old_umask = umask(0);
          umask_saved = TRUE;
        }
        if (mkdir(path_backup, 0755) < 0) {
          fatal_error("Creation of directory `%s' failed when trying to open "
                      "log file `%s'.", path_backup, path_name);
        }
      } else {
        fatal_error("stat() system call failed on `%s' when creating parent "
                    "directories for log file `%s'.", path_backup, path_name);
      }
    }
    path_backup[i] = '/';
    // skip over the duplicated slashes
    while (path_name[++i] == '/') ;
  }
  if (umask_saved) umask(old_umask);
  Free(path_backup);
}

void LegacyLogger::chk_logfile_data()
{
  if (this->logfile_size_ == 0 && this->logfile_number_ != 1) {
    TTCN_warning("Invalid combination of LogFileSize (= %lu) and "
                 "LogFileNumber (= %lu). LogFileNumber was reset to 1.",
                 (unsigned long)this->logfile_size_,
                 (unsigned long)this->logfile_number_);
    this->logfile_number_ = 1;
  }
  if (this->logfile_size_ > 0 && this->logfile_number_ == 1) {
    TTCN_warning("Invalid combination of LogFileSize (= %lu) and "
                 "LogFileNumber (= %lu). LogFileSize was reset to 0.",
                 (unsigned long)this->logfile_size_,
                 (unsigned long)this->logfile_number_);
    this->logfile_size_ = 0;
  }
  if (this->logfile_number_ == 1 &&
      this->disk_full_action_.type == TTCN_Logger::DISKFULL_DELETE) {
    TTCN_warning("Invalid combination of LogFileNumber (= 1) and "
                 "DiskFullAction (= Delete). DiskFullAction was reset to "
                 "Error.");
    this->disk_full_action_.type = TTCN_Logger::DISKFULL_ERROR;
  }
  if (this->logfile_number_ != 1 && this->append_file_) {
    TTCN_warning("Invalid combination of LogFileNumber (= %lu) and "
                 "AppendFile (= Yes). AppendFile was reset to No.",
                 (unsigned long)this->logfile_number_);
    this->append_file_ = FALSE;
  }
}

void LegacyLogger::set_file_name(const char *new_filename_skeleton,
                                 boolean from_config)
{
  Free(this->filename_skeleton_);
  this->filename_skeleton_ = mcopystr(new_filename_skeleton);
  if (from_config) this->skeleton_given_ = TRUE;
}

boolean LegacyLogger::set_file_size(int p_size)
{
  this->logfile_size_ = p_size;
  return  TRUE;
}

boolean LegacyLogger::set_file_number(int p_number)
{
  this->logfile_number_ = p_number;
  return TRUE;
}

boolean LegacyLogger::set_disk_full_action(
  TTCN_Logger::disk_full_action_t p_disk_full_action)
{
  this->disk_full_action_ = p_disk_full_action;
  return TRUE;
}

// LegacyLogger should be the only provider of log2str().
CHARSTRING LegacyLogger::log2str(const TitanLoggerApi::TitanLogEvent& event)
{
  char *event_str = event_to_str(event, TRUE);
  // mstrlen can handle NULL; it will result in a bound but empty CHARSTRING
  CHARSTRING ret_val(mstrlen(event_str), event_str);
  if (event_str == NULL) {
    TTCN_warning("No text for event");
  }
  else Free(event_str);
  return ret_val;
}

void LegacyLogger::set_append_file(boolean new_append_file)
{
  this->append_file_ = new_append_file;
}
