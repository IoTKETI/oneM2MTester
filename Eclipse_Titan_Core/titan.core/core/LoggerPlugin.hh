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
#ifndef LOGGER_PLUGIN_HH
#define LOGGER_PLUGIN_HH

#include "Types.h"
#include "Logger.hh"

namespace TitanLoggerApi
{
  class TitanLogEvent;
}

class ILoggerPlugin;
class CHARSTRING;

// Factory functions to create and destroy dynamic logger plug-ins.
typedef ILoggerPlugin *(*cb_create_plugin)(void);
typedef void (*cb_destroy_plugin)(ILoggerPlugin *plugin);

class LoggerPlugin
{
  friend class LoggerPluginManager;
public:
  explicit LoggerPlugin(const char *path);
  explicit LoggerPlugin(cb_create_plugin *create);
  ~LoggerPlugin();

  void load();
  void unload();
  void reset();

  void set_file_name(const char *new_filename_skeleton, boolean from_config);
  void set_component(const component comp, const char *name);
  void set_append_file(boolean new_append_file);
  void open_file(boolean is_first);
  void close_file();
  boolean set_file_size(int p_size);
  boolean set_file_number(int p_number);
  boolean set_disk_full_action(TTCN_Logger::disk_full_action_t p_disk_full_action);
  void set_parameter(const char* param_name, const char* param_value);

  int log(const TitanLoggerApi::TitanLogEvent& event, boolean log_buffered,
      boolean separate_file, boolean use_emergency_mask);
  inline boolean is_log2str_capable() const { return this->is_log2str_capable_; }
  CHARSTRING log2str(const TitanLoggerApi::TitanLogEvent& event) const;
  const char *plugin_name() const;
  boolean is_configured() const;
  void set_configured(boolean configured);

private:
  explicit LoggerPlugin(const LoggerPlugin&);
  LoggerPlugin& operator=(const LoggerPlugin&);

  const char *filename() const { return this->filename_; }

  ILoggerPlugin *ref_;  // The plug-in instance.
  void *handle_;  // Shared library handle, NULL for static plug-ins.
  char *filename_;  // Linker name of the plug-in, NULL for static plug-ins.
  cb_create_plugin create_;  // Creator of static plug-ins, NULL otherwise.
  boolean is_log2str_capable_;  // True if plugin provides log2str().
};

#endif // LOGGER_PLUGIN_HH
