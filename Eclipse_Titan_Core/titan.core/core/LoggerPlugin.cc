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
#include "LoggerPlugin.hh"

#include "../common/memory.h"

#include "TitanLoggerApi.hh"
#include "Error.hh"
#include "Logger.hh"
#include "ILoggerPlugin.hh"

#include <assert.h>

// constructor for dynamic plug-in
LoggerPlugin::LoggerPlugin(const char *path) :
  ref_(NULL), handle_(NULL), filename_(NULL), create_(NULL),
  is_log2str_capable_(FALSE)
{
  assert(path != NULL);
  this->filename_ = mcopystr(path);
}

// constructor for static plug-in
// Rant. The logical thing would be for the type of the parameter to be
// cb_create_plugin (which is already a pointer). But calling
// LoggerPlugin(create_legacy_logger) results in the wrong value being passed
// to the function. So the call has to be written as
// LoggerPlugin(&create_legacy_logger), which means that the type
// of the parameter has to be _pointer_to_ cb_create_plugin.
LoggerPlugin::LoggerPlugin(cb_create_plugin *create) :
  ref_(NULL), handle_(NULL), filename_(NULL),
  // We want the create_ member to be cb_create_plugin, to be able to call
  // this->create_(). Hence the need for this hideous cast.
  create_((cb_create_plugin)(unsigned long)create),
  is_log2str_capable_(FALSE)
{
}

LoggerPlugin::~LoggerPlugin()
{
  Free(this->filename_);
}

//void LoggerPlugin::load()

//void LoggerPlugin::unload()

void LoggerPlugin::reset()
{
  this->ref_->reset();
}

void LoggerPlugin::open_file(boolean is_first)
{
  this->ref_->open_file(is_first);
}

void LoggerPlugin::close_file()
{
  this->ref_->close_file();
}

void LoggerPlugin::set_file_name(const char *new_filename_skeleton,
                                boolean from_config)
{
  this->ref_->set_file_name(new_filename_skeleton, from_config);
}

void LoggerPlugin::set_append_file(boolean new_append_file)
{
  this->ref_->set_append_file(new_append_file);
}

boolean LoggerPlugin::set_file_size(int p_size)
{
  return this->ref_->set_file_size(p_size);
}

boolean LoggerPlugin::set_file_number(int p_number)
{
  return this->ref_->set_file_number(p_number);
}

boolean LoggerPlugin::set_disk_full_action(TTCN_Logger::disk_full_action_t p_disk_full_action)
{
  return this->ref_->set_disk_full_action(p_disk_full_action);
}

void LoggerPlugin::set_parameter(const char* param_name, const char* param_value)
{
  this->ref_->set_parameter(param_name, param_value);
}

int LoggerPlugin::log(const TitanLoggerApi::TitanLogEvent& event,
    boolean log_buffered, boolean separate_file, boolean use_emergency_mask)
{
  if (!this->ref_) return 1;
  this->ref_->log(event, log_buffered, separate_file, use_emergency_mask);
  return 0;
}

CHARSTRING LoggerPlugin::log2str(const TitanLoggerApi::TitanLogEvent& event) const
{
  assert(this->ref_);
  if (!this->is_log2str_capable_) return CHARSTRING();
  return this->ref_->log2str(event);
}

const char *LoggerPlugin::plugin_name() const
{
  assert(this->ref_);
  return this->ref_->plugin_name();
}

boolean LoggerPlugin::is_configured() const
{
  return this->ref_ != NULL && this->ref_->is_configured();
}

void LoggerPlugin::set_configured(boolean configured)
{
  if (this->ref_ != NULL)
    this->ref_->set_configured(configured);
}
