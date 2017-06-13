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
 *   Kovacs, Ferenc
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#ifndef LOGGINGPARAM_H
#define LOGGINGPARAM_H

#include "Types.h"
#include "Logger.hh"
#include "LoggingBits.hh"

typedef enum
{
  LP_FILEMASK,
  LP_CONSOLEMASK,
  LP_LOGFILESIZE,
  LP_LOGFILENUMBER,
  LP_DISKFULLACTION,
  LP_LOGFILE,
  LP_TIMESTAMPFORMAT,
  LP_SOURCEINFOFORMAT,
  LP_APPENDFILE,
  LP_LOGEVENTTYPES,
  LP_LOGENTITYNAME,
  LP_MATCHINGHINTS,
  LP_PLUGIN_SPECIFIC,
  LP_UNKNOWN,
  LP_EMERGENCY,
  LP_EMERGENCYBEHAVIOR,
  LP_EMERGENCYMASK,
  LP_EMERGENCYFORFAIL
} logging_param_type;

struct logging_param_t
{
  logging_param_type log_param_selection;
  char *param_name;  // Used to store name of plugin specific param.
  union {
    char *str_val;
    int int_val;
    boolean bool_val;
    Logging_Bits logoptions_val;
    TTCN_Logger::disk_full_action_t disk_full_action_value;
    TTCN_Logger::timestamp_format_t timestamp_value;
    TTCN_Logger::source_info_format_t source_info_value;
    TTCN_Logger::log_event_types_t log_event_types_value;
    TTCN_Logger::matching_verbosity_t matching_verbosity_value;
    size_t emergency_logging;
    TTCN_Logger::emergency_logging_behaviour_t emergency_logging_behaviour_value;
  };
};

struct logging_setting_t
{
  component_id_t component;
  char *plugin_id;
  logging_param_t logparam;
  logging_setting_t *nextparam;
};

struct logging_plugin_t
{
  component_id_t component;
  char *identifier;
  char *filename;
  logging_plugin_t *next;
};

#endif
