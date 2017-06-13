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
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#ifndef ILOGGER_PLUGIN_HH
#define ILOGGER_PLUGIN_HH

#include "Types.h"
#include "Logger.hh"
#include "TTCN3.hh"
#include "Charstring.hh"

/// Forward declarations.
namespace TitanLoggerApi
{
  class TitanLogEvent;
}

class ILoggerPlugin
{
public:
  ILoggerPlugin() :
    major_version_(0), minor_version_(0), name_(NULL), help_(NULL), is_configured_(FALSE) { }
  virtual ~ILoggerPlugin() { }

  virtual boolean is_static() = 0;
  virtual void init(const char *options = NULL) = 0;
  virtual void fini() = 0;
  virtual void reset() { }
  virtual void fatal_error(const char */*err_msg*/, ...) { }

  virtual boolean is_log2str_capable() { return FALSE; }
  virtual CHARSTRING log2str(const TitanLoggerApi::TitanLogEvent& /*event*/)
    { return CHARSTRING(); }

  inline unsigned int major_version() const { return this->major_version_; }
  inline unsigned int minor_version() const { return this->minor_version_; }
  inline const char *plugin_name() const { return this->name_; }
  inline const char *plugin_help() const { return this->help_; }
  inline boolean is_configured() const { return this->is_configured_; }
  inline void set_configured(boolean configured) { this->is_configured_ = configured; }

  virtual void log(const TitanLoggerApi::TitanLogEvent& event, boolean log_buffered,
      boolean separate_file, boolean use_emergency_mask) = 0;

  /// Backward compatibility functions.
  virtual void open_file(boolean /*is_first*/) { }
  virtual void close_file() { }
  virtual void set_file_name(const char */*new_filename_skeleton*/,
                             boolean /*from_config*/) { }
  virtual void set_append_file(boolean /*new_append_file*/) { }
  virtual boolean set_file_size(int /*size*/) { return FALSE; }
  virtual boolean set_file_number(int /*number*/) { return FALSE; }
  virtual boolean set_disk_full_action(TTCN_Logger::disk_full_action_t /*disk_full_action*/) { return FALSE; }
  virtual void set_parameter(const char */*parameter_name*/, const char */*parameter_value*/) { }

protected:
  unsigned int major_version_;
  unsigned int minor_version_;
  char *name_;
  char *help_;
  boolean is_configured_;
};

#endif  // ILOGGER_PLUGIN_HH
