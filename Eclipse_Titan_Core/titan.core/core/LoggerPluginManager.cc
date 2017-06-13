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
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include "LoggerPluginManager.hh"
#include "LoggerPlugin.hh"
#include "TitanLoggerApi.hh"
#include "Runtime.hh"
#include "Logger.hh"
#include "ILoggerPlugin.hh"

#include "../common/dbgnew.hh"
#include "../core/Error.hh"
#include "../common/static_check.h"

#include <assert.h>
#include <string.h>


#ifdef WIN32
// On Cygwin/MinGW it's called DLL.
#define SO_EXTENSION "dll"
#else
#define SO_EXTENSION "so"
#endif

namespace API = TitanLoggerApi;

extern "C" cb_create_plugin create_legacy_logger;

RingBuffer::~RingBuffer()
{
  if (buffer != NULL) {
    delete[] buffer;
  }
}

boolean RingBuffer::get(TitanLoggerApi::TitanLogEvent& data)
{
  if(tail == head)
    return FALSE;

  data = buffer[tail];
  tail = (tail +1) % (size + 1);

  return TRUE;
}

void RingBuffer::put(TitanLoggerApi::TitanLogEvent data)
{
  buffer[head] = data;
  head = (head + 1) % (size + 1);

  if (head == tail) {
    tail = (tail +1 ) % (size + 1);
  }
}

void RingBuffer::set_size(unsigned int new_size)
{
  if (buffer != NULL) return;

  size = new_size;
  buffer = new TitanLoggerApi::TitanLogEvent[size + 1];
}

void RingBuffer::clear()
{
  head = tail = 0;
}

LoggerPluginManager::LoggerPluginManager()
: n_plugins_(1), plugins_(new LoggerPlugin*[1]), entry_list_(NULL),
  current_event_(NULL), logparams_head(NULL), logparams_tail(NULL),
  logplugins_head(NULL), logplugins_tail(NULL)
{
  this->plugins_[0] = new LoggerPlugin(&create_legacy_logger);
  this->plugins_[0]->load();
}

LoggerPluginManager::~LoggerPluginManager()
{
  // It can happen that there're some unlogged events still in the buffer.  E.g.
  // an exception is thrown and no `end_event()' etc. is called.
  while (this->entry_list_ != NULL) {
    LogEntry *next_entry = this->entry_list_->next_entry_;
    for (size_t i = 0; i < this->n_plugins_; ++i) {
      if (this->plugins_[i]->is_configured()) {
        this->plugins_[i]->log(this->entry_list_->event_, TRUE, FALSE, FALSE);
      }
    }
    delete this->entry_list_;
    this->entry_list_ = next_entry;
  }
  this->entry_list_ = NULL;

  for (size_t i = 0; i < this->n_plugins_; ++i) {
    delete this->plugins_[i];
  }
  delete [] this->plugins_;
  this->plugins_ = NULL;
  this->n_plugins_ = 0;

  if (this->current_event_ != NULL) {
    fputs("Some logging events in the buffer were not finished properly in "
          "the plug-in manager.\n", stderr);
    while (this->current_event_) {
      ActiveEvent *outer_event = this->current_event_->outer_event_;
      Free(this->current_event_->event_str_);
      delete this->current_event_;
      this->current_event_ = outer_event;
    }
    this->current_event_ = NULL;
  }
}

void LoggerPluginManager::ring_buffer_dump(boolean do_close_file)
{
  // in case of buffer all, flush the content of the ring buffer
  if (TTCN_Logger::get_emergency_logging_behaviour() == TTCN_Logger::BUFFER_ALL) {
    TitanLoggerApi::TitanLogEvent ring_event;
    // get all the events from the ring: buffer
    while (!ring_buffer.isEmpty()) {
      if (ring_buffer.get(ring_event)) {
        internal_log_to_all(ring_event, TRUE, FALSE, FALSE);  // buffer all: DO NOT log in separate file
      }
    }
  }

  if (do_close_file) {
    for (size_t i = 0; i < this->n_plugins_; ++i) {
      plugins_[i]->close_file();
    }
  }

  ring_buffer.clear();
}

void LoggerPluginManager::register_plugin(const component_id_t comp,
                                          char *identifier, char *filename)
{
  logging_plugin_t *newplugin = new logging_plugin_t;
  newplugin->component.id_selector = comp.id_selector;
  switch (newplugin->component.id_selector) {
  case COMPONENT_ID_NAME: newplugin->component.id_name = mcopystr(comp.id_name); break;
  case COMPONENT_ID_COMPREF: newplugin->component.id_compref = comp.id_compref; break;
  default: newplugin->component.id_name = NULL; break;
  }
  newplugin->identifier = identifier;
  newplugin->filename = filename;
  newplugin->next = NULL;
  if (logplugins_head == NULL) logplugins_head = newplugin;
  if (logplugins_tail != NULL) logplugins_tail->next = newplugin;
  logplugins_tail = newplugin;
}

void LoggerPluginManager::load_plugins(component component_reference,
                                       const char *component_name)
{
  if (logplugins_head == NULL) {
    // The LoggerPlugins option was not used in the configuration file.
    // The LegacyLogger plug-in is already active; nothing to do.
    return;
  }

  for (logging_plugin_t *p = logplugins_head; p != NULL; p = p->next) {
    switch (p->component.id_selector) {
    case COMPONENT_ID_NAME:
      if (component_name != NULL &&
          !strcmp(p->component.id_name, component_name))
        load_plugin(p->identifier, p->filename);
      break;
    case COMPONENT_ID_COMPREF:
      if (p->component.id_compref == component_reference)
        load_plugin(p->identifier, p->filename);
      break;
    case COMPONENT_ID_ALL:
      load_plugin(p->identifier, p->filename);
      break;
    default:
      break;
    }
  }
}

void LoggerPluginManager::load_plugin(const char *identifier,
                                      const char *filename)
{
  boolean is_legacylogger =
    !strncasecmp(identifier, "LegacyLogger", 12) ? TRUE : FALSE;
  static boolean legacylogger_needed = FALSE;
  if (!legacylogger_needed && is_legacylogger) legacylogger_needed = TRUE;
  // LegacyLogger was listed explicitly.  Otherwise, it's disabled.  It is
  // always loaded as the first element of the list.
  this->plugins_[0]->set_configured(legacylogger_needed);

  if (is_legacylogger) {
    if (filename != NULL)
      TTCN_warning("The `LegacyLogger' plug-in should not have a path");
    return;  // It's already in the list.
  }

  char *pluginname = (filename != NULL && strlen(filename) > 0) ?
    mcopystr(filename) : mputprintf(NULL, "%s.%s", identifier, SO_EXTENSION);
  size_t pluginname_length = strlen(pluginname);
  for (size_t i = 0; i < this->n_plugins_; ++i) {
    // Our only static plug-in doesn't have a name, skip it.  If we have a
    // name, we have a dynamic plug-in.
    if (!this->plugins_[i]->filename_)
      continue;
    if (!strncmp(pluginname, this->plugins_[i]->filename_,
                 pluginname_length)) {
      TTCN_warning("A plug-in from the same path `%s' is already active, "
                   "skipping plug-in", pluginname);
      Free(pluginname);
      return;
    }
  }

  this->plugins_ = (LoggerPlugin **)Realloc(this->plugins_,
    ++this->n_plugins_ * sizeof(LoggerPlugin *));
  this->plugins_[this->n_plugins_ - 1] = new LoggerPlugin(pluginname);
  // Doesn't matter if load fails...
  Free(pluginname);
  this->plugins_[this->n_plugins_ - 1]->load();
}

extern boolean operator==(const component_id_t& left, const component_id_t& right);

boolean LoggerPluginManager::add_parameter(const logging_setting_t& logging_param)
{
  boolean duplication_warning = FALSE;

  for (logging_setting_t *par = logparams_head; par != NULL; par = par->nextparam) {
    boolean for_all_components = logging_param.component.id_selector == COMPONENT_ID_ALL || par->component.id_selector == COMPONENT_ID_ALL;
    boolean for_all_plugins = logging_param.plugin_id == NULL || par->plugin_id == NULL ||
      !strcmp(logging_param.plugin_id, "*") || !strcmp(par->plugin_id, "*");
    boolean component_overlaps = for_all_components || logging_param.component == par->component;
    boolean plugin_overlaps = for_all_plugins || !strcmp(logging_param.plugin_id, par->plugin_id);
    boolean parameter_overlaps = logging_param.logparam.log_param_selection == par->logparam.log_param_selection;
    if (parameter_overlaps && logging_param.logparam.log_param_selection == LP_PLUGIN_SPECIFIC)
      parameter_overlaps = strcmp(logging_param.logparam.param_name, par->logparam.param_name) == 0;
    duplication_warning = component_overlaps && plugin_overlaps && parameter_overlaps;
    if (duplication_warning)
      break;
  }

  logging_setting_t *newparam = new logging_setting_t(logging_param);
  newparam->nextparam = NULL;
  if (logparams_head == NULL) logparams_head = newparam;
  if (logparams_tail != NULL) logparams_tail->nextparam = newparam;
  logparams_tail = newparam;

  return duplication_warning;
}

void LoggerPluginManager::set_parameters(component component_reference,
                                         const char *component_name)
{
  if (logparams_head == NULL) return;
  for (logging_setting_t *par = logparams_head; par != NULL; par = par->nextparam)
    switch (par->component.id_selector) {
    case COMPONENT_ID_NAME:
      if (component_name != NULL &&
          !strcmp(par->component.id_name, component_name)) {
        apply_parameter(*par);
      }
      break;
    case COMPONENT_ID_COMPREF:
      if (par->component.id_compref == component_reference)
        apply_parameter(*par);
      break;
    case COMPONENT_ID_ALL:
      apply_parameter(*par);
      break;
    default:
      break;
    }
}

void LoggerPluginManager::apply_parameter(const logging_setting_t& logparam)
{
  if (logparam.plugin_id && !(strlen(logparam.plugin_id) == 1 && !strncmp(logparam.plugin_id, "*", 1))) {
    // The parameter refers to a specific plug-in.  If the plug-in is not
    // found the execution will stop.
    LoggerPlugin *plugin = find_plugin(logparam.plugin_id);
    if (plugin != NULL) {
      send_parameter_to_plugin(plugin, logparam);
    } else {
      TTCN_Logger::fatal_error("Logger plug-in with name `%s' was not found.", logparam.plugin_id);
    }
  } else {
    // The parameter refers to all plug-ins.
    for (size_t i = 0; i < this->n_plugins_; i++) {
      send_parameter_to_plugin(this->plugins_[i], logparam);
    }
  }
}

void LoggerPluginManager::send_parameter_to_plugin(LoggerPlugin *plugin,
                                                   const logging_setting_t& logparam)
{
  switch (logparam.logparam.log_param_selection) {
  case LP_FILEMASK:
    TTCN_Logger::set_file_mask(logparam.component,
      logparam.logparam.logoptions_val);
    break;
  case LP_CONSOLEMASK:
    TTCN_Logger::set_console_mask(logparam.component,
      logparam.logparam.logoptions_val);
    break;
  case LP_LOGFILESIZE:
    plugin->set_file_size(logparam.logparam.int_val);
    break;
  case LP_LOGFILENUMBER:
    plugin->set_file_number(logparam.logparam.int_val);
    break;
  case LP_DISKFULLACTION:
    plugin->set_disk_full_action(logparam.logparam.disk_full_action_value);
    break;
  case LP_LOGFILE:
    plugin->set_file_name(logparam.logparam.str_val, TRUE);
    break;
  case LP_TIMESTAMPFORMAT:
    TTCN_Logger::set_timestamp_format(logparam.logparam.timestamp_value);
    break;
  case LP_SOURCEINFOFORMAT:
    TTCN_Logger::set_source_info_format(logparam.logparam.source_info_value);
    break;
  case LP_APPENDFILE:
    plugin->set_append_file(logparam.logparam.bool_val);
    break;
  case LP_LOGEVENTTYPES:
    TTCN_Logger::set_log_event_types(logparam.logparam.log_event_types_value);
    break;
  case LP_LOGENTITYNAME:
    TTCN_Logger::set_log_entity_name(logparam.logparam.bool_val);
    break;
  case LP_MATCHINGHINTS:
    TTCN_Logger::set_matching_verbosity(logparam.logparam.matching_verbosity_value);
    break;
  case LP_PLUGIN_SPECIFIC:
    plugin->set_parameter(logparam.logparam.param_name, logparam.logparam.str_val);
    break;
  case LP_EMERGENCY:
    TTCN_Logger::set_emergency_logging(logparam.logparam.emergency_logging);
    ring_buffer.set_size(TTCN_Logger::get_emergency_logging());
    break;
  case LP_EMERGENCYBEHAVIOR:
    TTCN_Logger::set_emergency_logging_behaviour(logparam.logparam.emergency_logging_behaviour_value);
    break;
  case LP_EMERGENCYMASK:
    TTCN_Logger::set_emergency_logging_mask(logparam.component,
          logparam.logparam.logoptions_val);
    break;
  case LP_EMERGENCYFORFAIL:
    TTCN_Logger::set_emergency_logging_for_fail_verdict(logparam.logparam.bool_val);
    break;
  default:
    break;
  }
}

void LoggerPluginManager::clear_param_list()
{
  for (logging_setting_t *par = logparams_head; par != NULL;) {
    Free(par->plugin_id);
    switch (par->logparam.log_param_selection) {
    case LP_PLUGIN_SPECIFIC:
      Free(par->logparam.param_name);
      // no break
    case LP_LOGFILE:
      Free(par->logparam.str_val);
      break;
    default:
      break;
    }
    if (par->component.id_selector == COMPONENT_ID_NAME)
      Free(par->component.id_name);
    logging_setting_t *tmp = par;
    par = par->nextparam;
    delete tmp;
  }
  logparams_head = logparams_tail = NULL;
}

void LoggerPluginManager::clear_plugin_list()
{
  for (logging_plugin_t *plugin = logplugins_head; plugin != NULL;) {
    if (plugin->component.id_selector == COMPONENT_ID_NAME)
      Free(plugin->component.id_name);
    Free(plugin->identifier);
    Free(plugin->filename);
    logging_plugin_t *tmp = plugin;
    plugin = plugin->next;
    delete tmp;
  }
  logplugins_head = logplugins_tail = NULL;
}

void LoggerPluginManager::set_file_name(const char *new_filename_skeleton,
                                        boolean from_config)
{
  for (size_t i = 0; i < this->n_plugins_; ++i)
    this->plugins_[i]->set_file_name(new_filename_skeleton, from_config);
}

void LoggerPluginManager::reset()
{
  for (size_t i = 0; i < this->n_plugins_; ++i)
    this->plugins_[i]->reset();
}

void LoggerPluginManager::set_append_file(boolean new_append_file)
{
  for (size_t i = 0; i < this->n_plugins_; ++i)
    this->plugins_[i]->set_append_file(new_append_file);
}

boolean LoggerPluginManager::set_file_size(component_id_t const& /*comp*/, int p_size)
{
  boolean ret_val = FALSE;
  for (size_t i = 0; i < this->n_plugins_; ++i)
    if (this->plugins_[i]->set_file_size(p_size))
      ret_val = TRUE;
  return ret_val;
}

boolean LoggerPluginManager::set_file_number(component_id_t const& /*comp*/, int p_number)
{
  boolean ret_val = FALSE;
  for (size_t i = 0; i < this->n_plugins_; ++i)
    if (this->plugins_[i]->set_file_number(p_number))
      ret_val = TRUE;
  return ret_val;
}

boolean LoggerPluginManager::set_disk_full_action(component_id_t const& /*comp*/,
  TTCN_Logger::disk_full_action_t p_disk_full_action)
{
  boolean ret_val = FALSE;
  for (size_t i = 0; i < this->n_plugins_; ++i)
    if (this->plugins_[i]->set_disk_full_action(p_disk_full_action))
      ret_val = TRUE;
  return ret_val;
}

void LoggerPluginManager::open_file()
{
  static boolean is_first = TRUE;
  boolean free_entry_list = FALSE;
  assert(this->n_plugins_ > 0);
  // In case of `EXECUTOR_LOGOPTIONS' write updated
  // `write_logger_settings(TRUE)'.  Try to log the buffered events, they not
  // necessarily be logged otherwise.
  for (size_t i = 0; i < this->n_plugins_; ++i) {
    this->plugins_[i]->open_file(is_first);
    if (this->plugins_[i]->is_configured()) {
      free_entry_list = TRUE;
      LogEntry *entry = this->entry_list_, *next_entry = NULL;
      while (entry != NULL) {
        next_entry = entry->next_entry_;
        if ((TTCN_Logger::Severity)(int)entry->event_.severity() ==
            TTCN_Logger::EXECUTOR_LOGOPTIONS) {
          char *new_log_message = TTCN_Logger::get_logger_settings_str();
          entry->event_.logEvent().choice().executorEvent().choice().logOptions() =
            CHARSTRING(mstrlen(new_log_message), new_log_message);
          Free(new_log_message);
        }
        this->plugins_[i]->log(entry->event_, TRUE, FALSE, FALSE);
        entry = next_entry;
      }
    }
  }
  if (free_entry_list) {
    while (this->entry_list_ != NULL) {
      LogEntry *next_entry = this->entry_list_->next_entry_;
      delete this->entry_list_;
      this->entry_list_ = next_entry;
    }
    this->entry_list_ = NULL;
  }
  is_first = FALSE;
}

void LoggerPluginManager::close_file()
{

  while (this->current_event_ != NULL)
    finish_event();

  ring_buffer_dump(TRUE);
}

void LoggerPluginManager::unload_plugins()
{
  for (size_t i = 0; i < n_plugins_; ++i)
    plugins_[i]->unload();
}

void LoggerPluginManager::fill_common_fields(API::TitanLogEvent& event,
                                             const TTCN_Logger::Severity& severity)
{
  // Check at compile time that entity type can be directly assigned.
  ENSURE_EQUAL(API::LocationInfo_ent__type::unknown    , TTCN_Location::LOCATION_UNKNOWN);
  ENSURE_EQUAL(API::LocationInfo_ent__type::controlpart, TTCN_Location::LOCATION_CONTROLPART);
  ENSURE_EQUAL(API::LocationInfo_ent__type::testcase__ , TTCN_Location::LOCATION_TESTCASE);
  ENSURE_EQUAL(API::LocationInfo_ent__type::altstep__  , TTCN_Location::LOCATION_ALTSTEP);
  ENSURE_EQUAL(API::LocationInfo_ent__type::function__ , TTCN_Location::LOCATION_FUNCTION);
  ENSURE_EQUAL(API::LocationInfo_ent__type::external__function, TTCN_Location::LOCATION_EXTERNALFUNCTION);
  ENSURE_EQUAL(API::LocationInfo_ent__type::template__ , TTCN_Location::LOCATION_TEMPLATE);

  // The detailed timestamp is used by the plug-ins.  Don't stringify in this
  // stage.  E.g. DISKFULL_RETRY uses these for comparison.  TODO: Severity
  // should be an optional field, since the type of the event (in the XSD) is
  // always clear.  It's now used to handle unhandled events.
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0)
    TTCN_Logger::fatal_error("The gettimeofday() system call failed.");
  event.timestamp() = API::TimestampType(tv.tv_sec, tv.tv_usec);
  TTCN_Logger::source_info_format_t source_info_format =
    TTCN_Logger::get_source_info_format();
  API::TitanLogEvent_sourceInfo__list& srcinfo = event.sourceInfo__list();
  srcinfo = NULL_VALUE;  // Make sure it's bound.
  if (source_info_format != TTCN_Logger::SINFO_NONE) {
    if (TTCN_Location::innermost_location != NULL) {
      size_t num_locations = 0;
      for (TTCN_Location *iter = TTCN_Location::outermost_location; iter != NULL; iter = iter->inner_location) {
        API::LocationInfo& loc = srcinfo[num_locations++];
        loc.filename() = iter->file_name;
        loc.line() = iter->line_number;
        loc.ent__type() = iter->entity_type;
        loc.ent__name() = iter->entity_name;
      }
    }
  }
  event.severity() = severity;
}

void LoggerPluginManager::internal_prebuff_logevent(const TitanLoggerApi::TitanLogEvent& event)
{
  LogEntry *new_entry = new LogEntry;
  new_entry->event_ = event;
  new_entry->next_entry_ = NULL;
  if (!this->entry_list_) {
    this->entry_list_ = new_entry;
  } else {
    LogEntry *current_entry = this->entry_list_;
    for (; current_entry; current_entry = current_entry->next_entry_)
      if (!current_entry->next_entry_) break;
    current_entry->next_entry_ = new_entry;
  }
}

void LoggerPluginManager::internal_log_prebuff_logevent()
{
  LogEntry *entry = this->entry_list_, *next_entry = NULL;
  while (entry != NULL) {
    next_entry = entry->next_entry_;
    if ((TTCN_Logger::Severity)(int)entry->event_.severity() ==
        TTCN_Logger::EXECUTOR_LOGOPTIONS) {
      char *new_log_message = TTCN_Logger::get_logger_settings_str();
      entry->event_.logEvent().choice().executorEvent().choice().logOptions() =
          CHARSTRING(mstrlen(new_log_message), new_log_message);
      Free(new_log_message);
    }
    internal_log_to_all(entry->event_, TRUE, FALSE, FALSE);
    delete entry;
    entry = next_entry;
  }

  this->entry_list_ = NULL;
}

void LoggerPluginManager::internal_log_to_all(const TitanLoggerApi::TitanLogEvent& event,
      boolean log_buffered, boolean separate_file, boolean use_emergency_mask)
{
  for (size_t i = 0; i < this->n_plugins_; ++i) {
    if (this->plugins_[i]->is_configured()) {
      this->plugins_[i]->log(event, log_buffered, separate_file, use_emergency_mask);
    }
  }
}

boolean LoggerPluginManager::plugins_ready() const
{
  for (size_t i = 0; i < this->n_plugins_; ++i) {
    if (this->plugins_[i]->is_configured()) {
      return TRUE;
    }
  }
  return FALSE;
}

void LoggerPluginManager::log(const API::TitanLogEvent& event)
{
  if (!plugins_ready()) {
    // buffer quick events
    internal_prebuff_logevent(event);
    return;
  }

  // Init phase, log prebuffered events first if any.
  internal_log_prebuff_logevent();

  if (TTCN_Logger::get_emergency_logging() == 0) {
    // If emergency buffering is not needed log the event
    internal_log_to_all(event, FALSE, FALSE, FALSE);
    return;
  }

  // Log the buffered events first, if any.  In the current scheme,
  // open_file() has already flushed this buffer at this time.
  // Send the event to the plugin if it is necessary.
  // ring_buffer content fill up
  if (TTCN_Logger::get_emergency_logging_behaviour() == TTCN_Logger::BUFFER_MASKED) {
    //ToDo: do it nicer
    //if(TTCN_Logger::log_this_event((TTCN_Logger::Severity)(int)event.severity())){
    internal_log_to_all(event, TRUE, FALSE, FALSE);
    if (!TTCN_Logger::should_log_to_file((TTCN_Logger::Severity)(int)event.severity()) &&
        TTCN_Logger::should_log_to_emergency((TTCN_Logger::Severity)(int)event.severity())) {
      ring_buffer.put(event);
    }
  } else if (TTCN_Logger::get_emergency_logging_behaviour() == TTCN_Logger::BUFFER_ALL) {
    if (ring_buffer.isFull()) {
      TitanLoggerApi::TitanLogEvent ring_event;
      // the ring buffer is full, get the the oldest event
      // check does the user want this to be logged
      if (ring_buffer.get(ring_event)) {
        internal_log_to_all(ring_event, TRUE, FALSE, FALSE);
      } else {
        // it is not wanted by the user, throw it away
      }
    }
    ring_buffer.put(event);
  }
  // ERROR or setverdict(fail), flush the ring buffer content
  if ((TTCN_Logger::Severity)(int)event.severity() == TTCN_Logger::ERROR_UNQUALIFIED ||
      (TTCN_Logger::get_emergency_logging_for_fail_verdict() &&
      (TTCN_Logger::Severity)(int)event.severity() == TTCN_Logger::VERDICTOP_SETVERDICT &&
       event.logEvent().choice().verdictOp().choice().setVerdict().newVerdict() == API::Verdict::v3fail)) {
    TitanLoggerApi::TitanLogEvent ring_event;
    // get all the events from the ring: buffer
    while (!ring_buffer.isEmpty()) {
      if (ring_buffer.get(ring_event)) {
        if (TTCN_Logger::get_emergency_logging_behaviour() == TTCN_Logger::BUFFER_MASKED) {
          internal_log_to_all(ring_event, TRUE, TRUE, FALSE);  // log in separate file
        } else if (TTCN_Logger::get_emergency_logging_behaviour() == TTCN_Logger::BUFFER_ALL) {
          internal_log_to_all(ring_event, TRUE, FALSE, TRUE);  // DO NOT log in separate file
        }
      }
    }

    ring_buffer.clear();
  }
}

// Start of externally callable log functions
void LoggerPluginManager::log_unhandled_event(TTCN_Logger::Severity severity,
                                              const char *message_ptr,
                                              size_t message_len)
{
  if (!TTCN_Logger::log_this_event(severity) && (TTCN_Logger::get_emergency_logging()<=0)) return;
  API::TitanLogEvent event;
  fill_common_fields(event, severity);

  event.logEvent().choice().unhandledEvent() = CHARSTRING(message_len, message_ptr);
  log(event);
}

void LoggerPluginManager::log_log_options(const char *message_ptr,
                                          size_t message_len)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_LOGOPTIONS) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_LOGOPTIONS);

  event.logEvent().choice().executorEvent().choice().logOptions() =
    CHARSTRING(message_len, message_ptr);
  log(event);
}

void LoggerPluginManager::log_timer_read(const char *timer_name,
                                         double start_val)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::TIMEROP_READ) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::TIMEROP_READ);

  API::TimerType& timer = event.logEvent().choice().timerEvent().choice().readTimer();
  timer.name() = timer_name;
  timer.value__() = start_val;
  log(event);
}

void LoggerPluginManager::log_timer_start(const char *timer_name,
                                          double start_val)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::TIMEROP_START) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::TIMEROP_START);

  API::TimerType& timer = event.logEvent().choice().timerEvent().choice().startTimer();
  timer.name() = timer_name;
  timer.value__() = start_val;
  log(event);
}

void LoggerPluginManager::log_timer_guard(double start_val)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::TIMEROP_GUARD) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::TIMEROP_GUARD);

  API::TimerGuardType& timer = event.logEvent().choice().timerEvent().choice().guardTimer();
  timer.value__() = start_val;
  log(event);
}

void LoggerPluginManager::log_timer_stop(const char *timer_name,
                                         double stop_val)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::TIMEROP_STOP) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::TIMEROP_STOP);

  API::TimerType& timer = event.logEvent().choice().timerEvent().choice().stopTimer();
  timer.name() = timer_name;
  timer.value__() = stop_val;
  log(event);
}

void LoggerPluginManager::log_timer_timeout(const char *timer_name,
                                            double timeout_val)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::TIMEROP_TIMEOUT) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::TIMEROP_TIMEOUT);

  API::TimerType& timer = event.logEvent().choice().timerEvent().choice().timeoutTimer();
  timer.name() = timer_name;
  timer.value__() = timeout_val;
  log(event);
}

void LoggerPluginManager::log_timer_any_timeout()
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::TIMEROP_TIMEOUT) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::TIMEROP_TIMEOUT);

  // Make `ALT_timeoutAnyTimer' selected in the union.  No data fields here.
  event.logEvent().choice().timerEvent().choice().timeoutAnyTimer() = NULL_VALUE;
  log(event);
}

void LoggerPluginManager::log_timer_unqualified(const char *message)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::TIMEROP_UNQUALIFIED) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::TIMEROP_UNQUALIFIED);

  event.logEvent().choice().timerEvent().choice().unqualifiedTimer() = message;
  log(event);
}

void LoggerPluginManager::log_testcase_started(const qualified_name& testcase_name)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::TESTCASE_START) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::TESTCASE_START);

  API::QualifiedName& qname = event.logEvent().choice().testcaseOp().choice().testcaseStarted();
  qname.module__name  () = testcase_name.module_name;
  qname.testcase__name() = testcase_name.definition_name;
  log(event);
}

void LoggerPluginManager::log_testcase_finished(const qualified_name& testcase_name,
                                                verdicttype verdict,
                                                const char *reason)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::TESTCASE_FINISH) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::TESTCASE_FINISH);

  API::TestcaseType& testcase = event.logEvent().choice().testcaseOp().choice().testcaseFinished();
  API::QualifiedName& qname = testcase.name();
  qname.module__name  () = testcase_name.module_name;
  qname.testcase__name() = testcase_name.definition_name;
  testcase.verdict() = verdict;
  testcase.reason() = reason;
  // FIXME: our caller had a CHARSTRING but gave us a C string. Now we have to measure it again :(
  log(event);
}

void LoggerPluginManager::log_controlpart_start_stop(const char *module_name, int finished)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::STATISTICS_UNQUALIFIED) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::STATISTICS_UNQUALIFIED);

  API::StatisticsType& stats = event.logEvent().choice().statistics();
  if (finished) stats.choice().controlpartFinish() = module_name;
  else stats.choice().controlpartStart() = module_name;
  log(event);
}

void LoggerPluginManager::log_controlpart_errors(unsigned int error_count)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::STATISTICS_UNQUALIFIED) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::STATISTICS_UNQUALIFIED);

  event.logEvent().choice().statistics().choice().controlpartErrors() = error_count;
  log(event);
}

void LoggerPluginManager::log_setverdict(verdicttype new_verdict, verdicttype old_verdict,
  verdicttype local_verdict, const char *old_reason, const char *new_reason)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::VERDICTOP_SETVERDICT) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::VERDICTOP_SETVERDICT);
  API::SetVerdictType& set = event.logEvent().choice().verdictOp().choice().setVerdict();
  set.newVerdict() = new_verdict;
  set.oldVerdict() = old_verdict;
  set.localVerdict() = local_verdict;
  if (old_reason != NULL) set.oldReason() = old_reason;
  else set.oldReason() = OMIT_VALUE;
  if (new_reason != NULL) set.newReason() = new_reason;
  else set.newReason() = OMIT_VALUE;
  log(event);
}

void LoggerPluginManager::log_getverdict(verdicttype verdict)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::VERDICTOP_GETVERDICT) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::VERDICTOP_GETVERDICT);
  event.logEvent().choice().verdictOp().choice().getVerdict() = verdict;
  log(event);
}

void LoggerPluginManager::log_final_verdict(boolean is_ptc,
  verdicttype ptc_verdict, verdicttype local_verdict, verdicttype new_verdict,
  const char *verdict__reason, int notification, int ptc_compref,
  const char *ptc_name)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::VERDICTOP_FINAL) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::VERDICTOP_FINAL);
  API::FinalVerdictType& final = event.logEvent().choice().verdictOp().choice().finalVerdict();
  if (notification >= 0) {
    final.choice().notification() = notification;
  } else {
    final.choice().info().is__ptc() = is_ptc;
    final.choice().info().ptc__verdict() = ptc_verdict;
    final.choice().info().local__verdict() = local_verdict;
    final.choice().info().new__verdict() = new_verdict;
    final.choice().info().ptc__compref() = ptc_compref;
    if (verdict__reason != NULL)
      final.choice().info().verdict__reason() = verdict__reason;
    else final.choice().info().verdict__reason() = OMIT_VALUE;
    if (ptc_name != NULL)
      final.choice().info().ptc__name() = ptc_name;
    else final.choice().info().ptc__name() = OMIT_VALUE;
  }
  log(event);
}

void LoggerPluginManager::log_verdict_statistics(size_t none_count, double none_percent,
                                                 size_t pass_count, double pass_percent,
                                                 size_t inconc_count, double inconc_percent,
                                                 size_t fail_count, double fail_percent,
                                                 size_t error_count, double error_percent)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::STATISTICS_VERDICT) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::STATISTICS_VERDICT);

  API::StatisticsType& statistics = event.logEvent().choice().statistics();
  statistics.choice().verdictStatistics().none__() = none_count;
  statistics.choice().verdictStatistics().nonePercent() = none_percent;
  statistics.choice().verdictStatistics().pass__() = pass_count;
  statistics.choice().verdictStatistics().passPercent() = pass_percent;
  statistics.choice().verdictStatistics().inconc__() = inconc_count;
  statistics.choice().verdictStatistics().inconcPercent() = inconc_percent;
  statistics.choice().verdictStatistics().fail__() = fail_count;
  statistics.choice().verdictStatistics().failPercent() = fail_percent;
  statistics.choice().verdictStatistics().error__() = error_count;
  statistics.choice().verdictStatistics().errorPercent() = error_percent;
  log(event);
}

void LoggerPluginManager::log_defaultop_activate(const char *name, int id)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::DEFAULTOP_ACTIVATE) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::DEFAULTOP_ACTIVATE);

  API::DefaultOp& defaultop = event.logEvent().choice().defaultEvent().choice().defaultopActivate();
  defaultop.name() = name;
  defaultop.id() = id;
  defaultop.end() = API::DefaultEnd::UNKNOWN_VALUE; // don't care
  log(event);
}

void LoggerPluginManager::log_defaultop_deactivate(const char *name, int id)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::DEFAULTOP_DEACTIVATE) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::DEFAULTOP_DEACTIVATE);

  API::DefaultOp& defaultop = event.logEvent().choice().defaultEvent().choice().defaultopDeactivate();
  defaultop.name() = name;
  defaultop.id() = id;
  defaultop.end() = API::DefaultEnd::UNKNOWN_VALUE; // don't care
  log(event); // whoa, deja vu!
}

void LoggerPluginManager::log_defaultop_exit(const char *name, int id, int x)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::DEFAULTOP_EXIT) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::DEFAULTOP_EXIT);

  API::DefaultOp& defaultop = event.logEvent().choice().defaultEvent().choice().defaultopExit();
  defaultop.name() = name;
  defaultop.id() = id;
  defaultop.end() = x;
  log(event);
}

void LoggerPluginManager::log_executor_runtime(API::ExecutorRuntime_reason reason)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_RUNTIME) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_RUNTIME);

  API::ExecutorRuntime& exec = event.logEvent().choice().executorEvent().choice().executorRuntime();
  exec.reason() = reason;
  exec.module__name() = OMIT_VALUE;
  exec.testcase__name() = OMIT_VALUE;
  exec.pid() = OMIT_VALUE;
  exec.fd__setsize() = OMIT_VALUE;
  log(event);
}

void LoggerPluginManager::log_HC_start(const char *host)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_RUNTIME) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_RUNTIME);

  API::ExecutorRuntime& exec = event.logEvent().choice().executorEvent().choice().executorRuntime();
  exec.reason() = API::ExecutorRuntime_reason::host__controller__started;
  exec.module__name() = host; // "reuse" charstring member
  exec.testcase__name() = OMIT_VALUE;
  exec.pid() = OMIT_VALUE;
  exec.fd__setsize() = OMIT_VALUE;
  log(event);
}

void LoggerPluginManager::log_fd_limits(int fd_limit, long fd_set_size)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_RUNTIME) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_RUNTIME);

  API::ExecutorRuntime& exec = event.logEvent().choice().executorEvent().choice().executorRuntime();
  exec.reason() = API::ExecutorRuntime_reason::fd__limits;
  exec.module__name() = OMIT_VALUE;
  exec.testcase__name() = OMIT_VALUE;
  exec.pid() = fd_limit;
  exec.fd__setsize() = fd_set_size;
  log(event);
}

void LoggerPluginManager::log_not_overloaded(int pid)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_RUNTIME) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_RUNTIME);

  API::ExecutorRuntime& exec = event.logEvent().choice().executorEvent().choice().executorRuntime();
  exec.reason() = API::ExecutorRuntime_reason::overloaded__no__more;
  exec.module__name() = OMIT_VALUE;
  exec.testcase__name() = OMIT_VALUE;
  exec.pid() = pid;
  exec.fd__setsize() = OMIT_VALUE;
  log(event);
}

void LoggerPluginManager::log_testcase_exec(const char *tc, const char *module)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_RUNTIME) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_RUNTIME);

  API::ExecutorRuntime& exec = event.logEvent().choice().executorEvent().choice().executorRuntime();
  exec.reason() = API::ExecutorRuntime_reason::executing__testcase__in__module;
  exec.module__name() = module;
  exec.testcase__name() = tc;
  exec.pid() = OMIT_VALUE;
  exec.fd__setsize() = OMIT_VALUE;
  log(event);
}

void LoggerPluginManager::log_module_init(const char *module, boolean finish)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_RUNTIME) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_RUNTIME);

  API::ExecutorRuntime& exec = event.logEvent().choice().executorEvent().choice().executorRuntime();
  exec.reason() = finish
    ? API::ExecutorRuntime_reason::initialization__of__module__finished
    : API::ExecutorRuntime_reason::initializing__module;
  exec.module__name() = module;
  exec.testcase__name() = OMIT_VALUE;
  exec.pid() = OMIT_VALUE;
  exec.fd__setsize() = OMIT_VALUE;
  log(event);
}

void LoggerPluginManager::log_mtc_created(long pid)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_RUNTIME) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_RUNTIME);

  API::ExecutorRuntime& exec = event.logEvent().choice().executorEvent().choice().executorRuntime();
  exec.reason() = API::ExecutorRuntime_reason::mtc__created;
  exec.module__name() = OMIT_VALUE;
  exec.testcase__name() = OMIT_VALUE;
  exec.pid() = pid;
  exec.fd__setsize() = OMIT_VALUE;
  log(event);
}

void LoggerPluginManager::log_configdata(int reason, const char *str)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_CONFIGDATA) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_CONFIGDATA);

  API::ExecutorConfigdata& cfg = event.logEvent().choice().executorEvent().choice().executorConfigdata();
  cfg.reason() = reason;
  if (str != NULL) cfg.param__() = str;
  else cfg.param__() = OMIT_VALUE;
  log(event);
}

void LoggerPluginManager::log_executor_component(int reason)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_COMPONENT) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_COMPONENT);

  API::ExecutorComponent& ec = event.logEvent().choice().executorEvent().choice().executorComponent();
  ec.reason() = reason;
  ec.compref() = OMIT_VALUE;
  log(event);
}

void LoggerPluginManager::log_executor_misc(int reason, const char *name,
  const char *address, int port)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_UNQUALIFIED) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_UNQUALIFIED);

  API::ExecutorUnqualified& ex = event.logEvent().choice().executorEvent().choice().executorMisc();
  ex.reason() = reason;
  ex.name() = name;
  ex.addr() = address;
  ex.port__() = port;

  log(event);
}

void LoggerPluginManager::log_extcommand(TTCN_Logger::extcommand_t action,
  const char *cmd)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::EXECUTOR_EXTCOMMAND) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::EXECUTOR_EXTCOMMAND);

  CHARSTRING& str = (action == TTCN_Logger::EXTCOMMAND_START)
    ? event.logEvent().choice().executorEvent().choice().extcommandStart()
    : event.logEvent().choice().executorEvent().choice().extcommandSuccess();

  str = cmd;
  log(event);
}

void LoggerPluginManager::log_matching_done(API::MatchingDoneType_reason reason,
  const char *type, int ptc, const char *return_type)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_DONE) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::MATCHING_DONE);

  API::MatchingDoneType& mp = event.logEvent().choice().matchingEvent().choice().matchingDone();
  mp.reason() = reason;
  mp.type__() = type;
  mp.ptc()    = ptc;
  mp.return__type() = return_type;
  log(event);
}

void LoggerPluginManager::log_matching_problem(int reason, int operation,
  boolean check, boolean anyport, const char *port_name)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_PROBLEM) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::MATCHING_PROBLEM);

  API::MatchingProblemType& mp = event.logEvent().choice().matchingEvent().choice().matchingProblem();
  mp.reason()    = reason;
  mp.any__port() = anyport;
  mp.check__()   = check;
  mp.operation() = operation;
  mp.port__name()= port_name;
  log(event);
}

void LoggerPluginManager::log_matching_success(int port_type, const char *port_name,
  int compref, const CHARSTRING& info)
{
  TTCN_Logger::Severity sev;
  if (compref == SYSTEM_COMPREF) {
    sev = (port_type == API::PortType::message__)
      ? TTCN_Logger::MATCHING_MMSUCCESS : TTCN_Logger::MATCHING_PMSUCCESS;
  }
  else {
    sev = (port_type == API::PortType::message__)
      ? TTCN_Logger::MATCHING_MCSUCCESS : TTCN_Logger::MATCHING_PCSUCCESS;
  }
  if (!TTCN_Logger::log_this_event(sev) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, sev);

  API::MatchingSuccessType& ms = event.logEvent().choice().matchingEvent().choice().matchingSuccess();
  ms.port__type() = port_type;
  ms.port__name() = port_name;
  ms.info()       = info;
  log(event);
}

void LoggerPluginManager::log_matching_failure(int port_type, const char *port_name,
  int compref, int reason, const CHARSTRING& info)
{
  TTCN_Logger::Severity sev;
  if (compref == SYSTEM_COMPREF) {
    sev = (port_type == API::PortType::message__)
      ? TTCN_Logger::MATCHING_MMUNSUCC : TTCN_Logger::MATCHING_PMUNSUCC;
  }
  else {
    sev = (port_type == API::PortType::message__)
      ? TTCN_Logger::MATCHING_MCUNSUCC : TTCN_Logger::MATCHING_PCUNSUCC;
  }
  if (!TTCN_Logger::log_this_event(sev) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, sev);

  API::MatchingFailureType& mf = event.logEvent().choice().matchingEvent().choice().matchingFailure();
  mf.port__type() = port_type;
  mf.port__name() = port_name;
  mf.reason() = reason;

  if (compref == SYSTEM_COMPREF) {
    mf.choice().system__();
  }
  else  mf.choice().compref() = compref;

  mf.info() = info;

  log(event);
}

void LoggerPluginManager::log_matching_timeout(const char *timer_name)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_PROBLEM) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::MATCHING_PROBLEM);

  API::MatchingTimeout& mt = event.logEvent().choice().matchingEvent().choice().matchingTimeout();
  if (timer_name) mt.timer__name() = timer_name;
  else mt.timer__name() = OMIT_VALUE;
  log(event);
}

void LoggerPluginManager::log_random(int action, double v, unsigned long u)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::FUNCTION_RND) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::FUNCTION_RND);

  API::FunctionEvent_choice_random &r = event.logEvent().choice().functionEvent().choice().random();
  r.operation()= action;
  r.retval()   = v;
  r.intseed()  = u;
  log(event);
}

/** Pretend that the control part is executed by a separate component.
 *
 *  In TITAN, the control part is executed on the MTC.
 *  This does not match the standard, which states:
 *  "The MTC shall be created by the system automatically at the start
 *   of each test case execution"; so the MTC is not supposed to exist
 *  in a control part. For the purpose of logging, we pretend that in the
 *  control part the current component is not the MTC but another "artificial"
 *  component "control", similar to "null", "system", or "all".
 *
 * @param compref
 * @return the input value, but substitute CONTROLPART_COMPREF for MTC_COMPREF
 * if inside a controlpart.
 */
inline int adjust_compref(int compref)
{
  if (compref == MTC_COMPREF) {
    switch (TTCN_Runtime::get_state()) {
    case TTCN_Runtime::MTC_CONTROLPART:
    case TTCN_Runtime::SINGLE_CONTROLPART:
      compref = CONTROL_COMPREF;
      break;
    default:
      break;
    }
  }

  return compref;
}

void LoggerPluginManager::log_portconnmap(int operation, int src_compref,
  const char *src_port, int dst_compref, const char *dst_port)
{
  TTCN_Logger::Severity event_severity;
  switch (operation) {
    case API::ParPort_operation::connect__:
    case API::ParPort_operation::disconnect__:
      event_severity = TTCN_Logger::PARALLEL_PORTCONN;
      break;
    case API::ParPort_operation::map__:
    case API::ParPort_operation::unmap__:
      event_severity = TTCN_Logger::PARALLEL_PORTMAP;
      break;
    default:
      TTCN_error("Invalid operation");
  }

  if (!TTCN_Logger::log_this_event(event_severity) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, event_severity);

  API::ParPort & pp = event.logEvent().choice().parallelEvent().choice().parallelPort();
  pp.operation() = operation;
  pp.srcCompref() = adjust_compref(src_compref);
  pp.srcPort()    = src_port;
  pp.dstCompref() = adjust_compref(dst_compref);
  pp.dstPort()    = dst_port;
  log(event);
}

void LoggerPluginManager::log_par_ptc(int reason,
  const char *module, const char *name, int compref,
  const char *compname, const char *tc_loc, int alive_pid, int status)
{
  TTCN_Logger::Severity sev =
    (alive_pid && reason == API::ParallelPTC_reason::function__finished)
    ? TTCN_Logger::PARALLEL_UNQUALIFIED : TTCN_Logger::PARALLEL_PTC;
  if (!TTCN_Logger::log_this_event(sev) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, sev);

  API::ParallelPTC& ptc = event.logEvent().choice().parallelEvent().choice().parallelPTC();
  ptc.reason() = reason;
  ptc.module__() = module;
  ptc.name() = name;
  ptc.compref() = compref;
  ptc.tc__loc() = tc_loc;
  ptc.compname() = compname;
  ptc.alive__pid() = alive_pid;
  ptc.status() = status;
  log(event);
}

void LoggerPluginManager::log_port_queue(int operation, const char *port_name,
  int compref, int id, const CHARSTRING& address, const CHARSTRING& param)
{
  TTCN_Logger::Severity sev;
  switch (operation) {
  case API::Port__Queue_operation::enqueue__msg:
  case API::Port__Queue_operation::extract__msg:
    sev = TTCN_Logger::PORTEVENT_MQUEUE;
    break;
  case API::Port__Queue_operation::enqueue__call:
  case API::Port__Queue_operation::enqueue__exception:
  case API::Port__Queue_operation::enqueue__reply:
  case API::Port__Queue_operation::extract__op:
    sev = TTCN_Logger::PORTEVENT_PQUEUE;
    break;
  default:
    TTCN_error("Invalid operation");
  }

  if (!TTCN_Logger::log_this_event(sev) && (TTCN_Logger::get_emergency_logging()<=0)) return;
  API::TitanLogEvent event;
  fill_common_fields(event, sev);

  API::Port__Queue& pq = event.logEvent().choice().portEvent().choice().portQueue();
  pq.operation() = operation;
  pq.port__name() = port_name;
  pq.compref() = adjust_compref(compref);
  pq.msgid() = id;
  pq.address__() = address;
  pq.param__() = param;
  log(event);
}

void LoggerPluginManager::log_port_state(int operation, const char *port_name)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_STATE)) return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::PORTEVENT_STATE);

  API::Port__State& ps = event.logEvent().choice().portEvent().choice().portState();
  ps.operation() = operation;
  ps.port__name() = port_name;
  log(event);
}

void LoggerPluginManager::log_procport_send(const char *portname, int operation,
  int compref, const CHARSTRING& system, const CHARSTRING& param)
{
  TTCN_Logger::Severity event_severity = (compref == SYSTEM_COMPREF)
    ? TTCN_Logger::PORTEVENT_PMOUT : TTCN_Logger::PORTEVENT_PCOUT;
  if (!TTCN_Logger::log_this_event(event_severity) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, event_severity);

  API::Proc__port__out& pt = event.logEvent().choice().portEvent().choice().procPortSend();
  pt.port__name() = portname;
  pt.operation() = operation;
  pt.compref() = compref;
  if (compref == SYSTEM_COMPREF) { // it's mapped
    pt.sys__name() = system;
  }
  pt.parameter() = param;

  log(event);
}

void LoggerPluginManager::log_procport_recv(const char *portname, int operation,
  int compref, boolean check, const CHARSTRING& param, int id)
{
  TTCN_Logger::Severity event_severity = (compref == SYSTEM_COMPREF)
    ? TTCN_Logger::PORTEVENT_PMIN : TTCN_Logger::PORTEVENT_PCIN;
  if (!TTCN_Logger::log_this_event(event_severity) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, event_severity);

  API::Proc__port__in& pt = event.logEvent().choice().portEvent().choice().procPortRecv();
  pt.port__name() = portname;
  pt.operation() = operation;
  pt.compref() = compref;
  pt.check__() = check;
  pt.parameter() = param;
  pt.msgid() = id;

  log(event);
}

void LoggerPluginManager::log_msgport_send(const char *portname, int compref,
  const CHARSTRING& param)
{
  TTCN_Logger::Severity event_severity = (compref == SYSTEM_COMPREF)
      ? TTCN_Logger::PORTEVENT_MMSEND : TTCN_Logger::PORTEVENT_MCSEND;
  if (!TTCN_Logger::log_this_event(event_severity) && (TTCN_Logger::get_emergency_logging()<=0)) return;
  API::TitanLogEvent event;
  fill_common_fields(event, event_severity);

  API::Msg__port__send& ms = event.logEvent().choice().portEvent().choice().msgPortSend();
  ms.port__name() = portname;
  ms.compref() = compref;
  ms.parameter() = param;

  log(event);
}

void LoggerPluginManager::log_msgport_recv(const char *portname, int operation,
  int compref, const CHARSTRING& system, const CHARSTRING& param, int id)
{
  TTCN_Logger::Severity event_severity = (compref == SYSTEM_COMPREF)
        ? TTCN_Logger::PORTEVENT_MMRECV : TTCN_Logger::PORTEVENT_MCRECV;
  if (!TTCN_Logger::log_this_event(event_severity) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, event_severity);

  API::Msg__port__recv& ms = event.logEvent().choice().portEvent().choice().msgPortRecv();
  ms.port__name() = portname;
  ms.compref() = compref;
  if (compref == SYSTEM_COMPREF) { // it's mapped
    ms.sys__name() = system;
  }
  ms.operation() = operation;
  ms.msgid() = id;
  ms.parameter() = param;

  log(event);
}

void LoggerPluginManager::log_dualport_map(boolean incoming, const char *target_type,
  const CHARSTRING& value, int id)
{
  TTCN_Logger::Severity event_severity = incoming
        ? TTCN_Logger::PORTEVENT_DUALRECV : TTCN_Logger::PORTEVENT_DUALSEND;
  if (!TTCN_Logger::log_this_event(event_severity) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, event_severity);

  API::Dualface__mapped& dual = event.logEvent().choice().portEvent().choice().dualMapped();
  dual.incoming() = incoming;
  dual.target__type() = target_type;
  dual.value__() = value;
  dual.msgid() = id;

  log(event);
}

void LoggerPluginManager::log_dualport_discard(boolean incoming, const char *target_type,
  const char *port_name, boolean unhandled)
{
  TTCN_Logger::Severity event_severity = incoming
    ? TTCN_Logger::PORTEVENT_DUALRECV : TTCN_Logger::PORTEVENT_DUALSEND;
  if (!TTCN_Logger::log_this_event(event_severity) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, event_severity);

  API::Dualface__discard& dualop = event.logEvent().choice().portEvent().choice().dualDiscard();
  dualop.incoming() = incoming;
  dualop.target__type() = target_type;
  dualop.port__name() = port_name;
  dualop.unhandled() = unhandled;

  log(event);
}

void LoggerPluginManager::log_setstate(const char *port_name, translation_port_state state,
  const CHARSTRING& info)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_SETSTATE) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::PORTEVENT_SETSTATE);

  API::Setstate& setstate = event.logEvent().choice().portEvent().choice().setState();
  setstate.port__name() = port_name;
  setstate.info() = (const char*)info;
  switch (state) {
    case UNSET:
      setstate.state() = "unset";
      break;
    case TRANSLATED:
      setstate.state() = "translated";
      break;
    case NOT_TRANSLATED:
      setstate.state() = "not translated";
      break;
     case FRAGMENTED:
      setstate.state() = "fragmented";
      break;
    case PARTIALLY_TRANSLATED:
      setstate.state() = "partially translated";
      break;
    default:
      TTCN_Logger::fatal_error("LoggerPluginManager::log_setstate(): unexpected port state");
  }

  log(event);
}

void LoggerPluginManager::log_port_misc(int reason, const char *port_name,
  int remote_component, const char *remote_port, const char *ip_address,
  int tcp_port, int new_size)
{
  if (!TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_UNQUALIFIED) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  API::TitanLogEvent event;
  fill_common_fields(event, TTCN_Logger::PORTEVENT_UNQUALIFIED);

  API::Port__Misc& portmisc = event.logEvent().choice().portEvent().choice().portMisc();
  portmisc.reason() = reason;
  portmisc.port__name() = port_name;
  portmisc.remote__component() = remote_component;
  portmisc.remote__port() = remote_port;
  portmisc.ip__address() = ip_address;
  portmisc.tcp__port() = tcp_port;
  portmisc.new__size() = new_size;

  log(event);
}


// End of externally callable functions

/// Construct an event string, one by one.  Event construction is not
/// supported for all event types.  E.g. those event types with fixed format
/// like TIMEROP_* events have nothing to do with this at all.
void LoggerPluginManager::append_event_str(const char *str)
{
  if (!this->current_event_) return;
  ActiveEvent& curr = *this->current_event_;
  const size_t str_len = strlen(str);
  if (!str_len) return; // don't bother with empty string

  if (curr.event_str_ != NULL) { // event_str_ already allocated
    if (!curr.fake_) {
      // pieces_ may be NULL, in which case Realloc calls Malloc
      curr.pieces_ = (size_t*)Realloc(curr.pieces_, sizeof(size_t) * curr.num_pieces_);
      // Remember the current end
      curr.pieces_[curr.num_pieces_++-1] = curr.event_str_len_;
    }
    // Don't bother remembering the pieces for a log2str event

    if (     curr.event_str_len_ + str_len > curr.event_str_size_) {
      for (; curr.event_str_len_ + str_len > curr.event_str_size_;
             curr.event_str_size_ *= 2) ; // double until it fits
      curr.event_str_ = (char *)Realloc(curr.event_str_, curr.event_str_size_);
      memset(curr.event_str_      + curr.event_str_len_, '\0',
             curr.event_str_size_ - curr.event_str_len_);
    }
    memcpy(curr.event_str_ + curr.event_str_len_, str, str_len);
    curr.event_str_len_ += str_len;
  } else { // the first piece
    curr.event_str_len_  = str_len;
    curr.event_str_size_ = str_len * 2; // must be strictly bigger than str_len
    curr.event_str_ = (char *)Malloc(curr.event_str_size_);
    memcpy(curr.event_str_, str, str_len);
    memset(curr.event_str_ + str_len, '\0',
      curr.event_str_size_ - str_len);
    curr.num_pieces_++;
    // Do not allocate memory for the first piece; it ends at event_str_len_
  }
}

char* LoggerPluginManager::get_current_event_str()
{
  if (!this->current_event_) return 0;
  size_t str_len = current_event_->event_str_len_ + 1;
  char* result = (char*)Malloc(str_len);
  memcpy(result, current_event_->event_str_, str_len - 1);
  result[str_len - 1] = '\0';
  return result;
}

void LoggerPluginManager::begin_event(TTCN_Logger::Severity msg_severity,
                                      boolean log2str)
{
  event_destination_t event_dest;
  if (log2str) event_dest = ED_STRING;
  else event_dest =
    TTCN_Logger::log_this_event(msg_severity) ? ED_FILE : ED_NONE;
  // We are in the middle of another event, allocate a new entry for this
  // event.  The other active events are available through outer events.
  // We're not (re)using the `outermost_event' buffer.
  ActiveEvent *new_event = new ActiveEvent(log2str, event_dest);
  if (!log2str) fill_common_fields(new_event->get_event(), msg_severity);
  new_event->outer_event_ = this->current_event_;
  this->current_event_ = new_event;
}

void LoggerPluginManager::end_event()
{
  if (this->current_event_ == NULL) {
    // Invalid usage.
    const char *message = "TTCN_Logger::end_event(): not in event.";
    log_unhandled_event(TTCN_Logger::WARNING_UNQUALIFIED, message, strlen(message));
    return;
  }

  ActiveEvent& curr = *this->current_event_;
  switch (curr.event_destination_) {
  case ED_NONE:
    break;
  case ED_FILE:
    switch ((TTCN_Logger::Severity)(int)curr.get_event().severity()) {
    case TTCN_Logger::DEBUG_ENCDEC:
    case TTCN_Logger::DEBUG_TESTPORT:
    case TTCN_Logger::DEBUG_UNQUALIFIED:
      curr.get_event().logEvent().choice().debugLog().text() =
        CHARSTRING(curr.event_str_len_, curr.event_str_);
      curr.get_event().logEvent().choice().debugLog().category() = 0; // not yet used
      break;

    case TTCN_Logger::ERROR_UNQUALIFIED:
      curr.get_event().logEvent().choice().errorLog().text() =
        CHARSTRING(curr.event_str_len_, curr.event_str_);
      curr.get_event().logEvent().choice().errorLog().category() = 0; // not yet used
      break;

    case TTCN_Logger::ACTION_UNQUALIFIED:
      // fall through
    case TTCN_Logger::USER_UNQUALIFIED: {
      // Select the list for either userLog or action; treatment is the same.
      API::Strings_str__list& slist = (curr.get_event().severity() == TTCN_Logger::USER_UNQUALIFIED)
        ? curr.get_event().logEvent().choice().userLog().str__list()
        : curr.get_event().logEvent().choice().actionEvent().str__list();
      if (curr.num_pieces_ > 0) {
        // First piece. If num_pieces_==1, pieces_ is not allocated, use the full length.
        size_t len0 = curr.num_pieces_ > 1 ? curr.pieces_[0] : curr.event_str_len_;
        slist[0] = CHARSTRING(len0, curr.event_str_);
        // Subsequent pieces from pieces_[i-1] to pieces_[i] (exclusive)
        for (size_t i = 1; i < curr.num_pieces_ - 1; ++i) {
          slist[i] = CHARSTRING(
            curr.pieces_[i] - curr.pieces_[i - 1],
            curr.event_str_ + curr.pieces_[i - 1]);
        }
        // Last piece (if it's not the same as the first)
        if (curr.num_pieces_ > 1) { // i == num_pieces_-1
          slist[curr.num_pieces_ - 1]= CHARSTRING(
            curr.event_str_len_ - curr.pieces_[curr.num_pieces_-2],
            curr.event_str_     + curr.pieces_[curr.num_pieces_-2]);
        }
      }
      else slist = NULL_VALUE; // make sure it's bound but empty
      break; }

    case TTCN_Logger::WARNING_UNQUALIFIED:
      curr.get_event().logEvent().choice().warningLog().text() =
        CHARSTRING(curr.event_str_len_, curr.event_str_);
      curr.get_event().logEvent().choice().warningLog().category() = 0; // not yet used
      break;

    default:
      curr.get_event().logEvent().choice().unhandledEvent() =
        CHARSTRING(curr.event_str_len_, curr.event_str_);
      break;
    } // switch(severity)
    log(curr.get_event());
    break;
  case ED_STRING:
    TTCN_Logger::fatal_error("TTCN_Logger::end_event(): event with string "
                             "destination was found, missing call of "
                             "TTCN_Logger::end_event_log2str().");
  default:
    TTCN_Logger::fatal_error("TTCN_Logger::end_event(): invalid event "
                             "destination.");
  } // switch(event_destination)

  // Remove the current (already logged) event from the list of pending
  // events and select a higher level event on the stack.  We're not
  // following the old implementation regarding the `outermost_event' stuff.
  ActiveEvent *outer_event = curr.outer_event_;
  Free(curr.event_str_);
  Free(curr.pieces_);
  delete this->current_event_;
  this->current_event_ = outer_event;
}

CHARSTRING LoggerPluginManager::end_event_log2str()
{
  if (this->current_event_ == NULL) {
    // Invalid usage.
    const char *message = "TTCN_Logger::end_event_log2str(): not in event.";
    log_unhandled_event(TTCN_Logger::WARNING_UNQUALIFIED, message, strlen(message));
    return CHARSTRING(); // unbound!
  }

  ActiveEvent& curr = *this->current_event_;
  CHARSTRING ret_val(curr.event_str_len_, curr.event_str_);

  ActiveEvent *outer_event = curr.outer_event_;
  Free(curr.event_str_);
  Free(curr.pieces_);
  delete this->current_event_;
  // Otherwise, we don't save the current event to be `outermost_event',
  // like the previous implementation.  An initialized `outermost_event' is
  // not the sign of an initialized logger.  NULL or not, assign.
  this->current_event_ = outer_event;
  return ret_val;
}

void LoggerPluginManager::finish_event()
{
  // Drop events which have string destination in case of log2str()
  // operations.  There is no try-catch block to delete the event.  Avoid
  // data/log file corruption if there's an exception inside a log2str(),
  // which is inside a log().
  while (this->current_event_ != NULL &&
         this->current_event_->event_destination_ == ED_STRING)
    (void)end_event_log2str();

  if (this->current_event_ != NULL) {
    log_event_str("<unfinished>");
    end_event();
  }
}

// Simply append the contents STR_PTR to the end of the current event.
// Everything is a string here.
void LoggerPluginManager::log_event_str(const char *str_ptr)
{
  if (this->current_event_ != NULL) {
    if (this->current_event_->event_destination_ == ED_NONE) return;
    if (str_ptr == NULL) str_ptr = "<NULL pointer>";
    append_event_str(str_ptr);
  } else {
    // Invalid usage.
    const char *message = "TTCN_Logger::log_event_str(): not in event.";
    log_unhandled_event(TTCN_Logger::WARNING_UNQUALIFIED, message, strlen(message));
  }
}

void LoggerPluginManager::log_char(char c)
{
  if (this->current_event_ != NULL) {
    if (this->current_event_->event_destination_ == ED_NONE || c == '\0') return;
    const char c_str[2] = { c, 0 };
    append_event_str(c_str);
  } else {
    // Invalid usage.
    const char *message = "TTCN_Logger::log_char(): not in event.";
    log_unhandled_event(TTCN_Logger::WARNING_UNQUALIFIED, message, strlen(message));
  }
}

void LoggerPluginManager::log_event_va_list(const char *fmt_str,
                                            va_list p_var)
{
  if (this->current_event_ != NULL) {
    if (this->current_event_->event_destination_ == ED_NONE) return;
    if (fmt_str == NULL) fmt_str = "<NULL format string>";
    char *message = mprintf_va_list(fmt_str, p_var);
    append_event_str(message);
    Free(message);
  } else {
    // Invalid usage.
    const char *message = "TTCN_Logger::log_event(): not in event.";
    log_unhandled_event(TTCN_Logger::WARNING_UNQUALIFIED, message, strlen(message));
  }
}

void LoggerPluginManager::log_va_list(TTCN_Logger::Severity msg_severity,
                                      const char *fmt_str, va_list p_var)
{
  if (!TTCN_Logger::log_this_event(msg_severity) && (TTCN_Logger::get_emergency_logging()<=0))
    return;
  if (fmt_str == NULL) fmt_str = "<NULL format string>";
  // Allocate a temporary buffer for this event.
  char *message_buf = mprintf_va_list(fmt_str, p_var);
  log_unhandled_event(msg_severity, message_buf, mstrlen(message_buf));
  Free(message_buf);
}


LoggerPlugin *LoggerPluginManager::find_plugin(const char *name)
{
  assert(name != NULL);
  for (size_t i = 0; i < this->n_plugins_; i++) {
    const char *plugin_name = this->plugins_[i]->ref_->plugin_name();
    // TODO: If the name is not set in the plug-in, a warning should be
    // reported instead.
    if (plugin_name != NULL && !strcmp(name, plugin_name)) {
      return this->plugins_[i];
    }
  }
  return NULL;
}

#ifdef MEMORY_DEBUG
// new has been redefined in dbgnew.hh, undo it
#undef new
#endif

LoggerPluginManager::ActiveEvent::ActiveEvent(boolean fake_event, event_destination_t dest)
: event_()
, event_str_(NULL)
, event_str_len_(0)
, event_str_size_(0)
, event_destination_(dest)
, outer_event_(NULL)
, num_pieces_(0)
, pieces_(NULL)
, fake_(fake_event)
{
  if (!fake_event) {
    new (event_) TitanLoggerApi::TitanLogEvent(); // placement new
  }
}

LoggerPluginManager::ActiveEvent::~ActiveEvent()
{
  if (!fake_) {
    get_event().~TitanLogEvent(); // explicit destructor call
  }
}
