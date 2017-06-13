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
 *
 ******************************************************************************/
#include "Error.hh"
#include "TitanLoggerControl.hh"
#include "Logger.hh"
#include "LoggingBits.hh"
#include "Component.hh"

// TODO: We should ask somebody instead...
const char LegacyLoggerName[] = "LegacyLogger";

void validate_plugin_name(CHARSTRING const& plugin)
{
  if (strcmp(LegacyLoggerName, (const char *)plugin))
    TTCN_error("Only `%s' can be configured dynamically.", LegacyLoggerName);
}

namespace TitanLoggerControl {

void set__log__file(CHARSTRING const& plugin, CHARSTRING const& filename)
{
  validate_plugin_name(plugin);

  TTCN_Logger::set_file_name(filename, FALSE);
  TTCN_Logger::open_file();
}

void set__log__entity__name(CHARSTRING const& plugin, BOOLEAN const& log_it)
{
  validate_plugin_name(plugin);

  TTCN_Logger::set_log_entity_name(log_it);
}

BOOLEAN get__log__entity__name(CHARSTRING const& plugin)
{
  validate_plugin_name(plugin);

  return BOOLEAN(TTCN_Logger::get_log_entity_name());
}

void set__matching__verbosity(CHARSTRING const& plugin, verbosity const& v)
{
  validate_plugin_name(plugin);

  TTCN_Logger::set_matching_verbosity(
    static_cast<TTCN_Logger::matching_verbosity_t>(v.as_int()));
}

verbosity get__matching__verbosity(CHARSTRING const& plugin)
{
  validate_plugin_name(plugin);

  verbosity retval(TTCN_Logger::get_matching_verbosity());
  return retval;
}

Severities get__file__mask(CHARSTRING const& plugin)
{
  validate_plugin_name(plugin);

  Severities retval(NULL_VALUE);  // Empty but initialized.
  Logging_Bits const& lb = TTCN_Logger::get_file_mask();
  for (int i = 1, s = 0; i < TTCN_Logger::NUMBER_OF_LOGSEVERITIES; ++i) {
    if (lb.bits[i]) {
      retval[s++] = Severity(i);
      // operator[] creates a new element and we assign a temporary to it
    }
  }

  return retval;
}

Severities get__console__mask(CHARSTRING const& plugin)
{
  validate_plugin_name(plugin);

  Severities retval(NULL_VALUE);  // Empty but initialized.
  Logging_Bits const& lb = TTCN_Logger::get_console_mask();
  for (int i = 1, s = 0; i < TTCN_Logger::NUMBER_OF_LOGSEVERITIES; ++i) {
    if (lb.bits[i]) {
      retval[s++] = Severity(i);
      // operator[] creates a new element and we assign a temporary to it
    }
  }

  return retval;
}

void set__console__mask(CHARSTRING const& plugin, Severities const& mask)
{
  validate_plugin_name(plugin);

  Logging_Bits lb(Logging_Bits::log_nothing);
  for (int B = mask.size_of() - 1; B >= 0; B--) {
    lb.add_sev(static_cast<TTCN_Logger::Severity>((int)mask[B]));
  }
  component_id_t cmpt_id = { COMPONENT_ID_COMPREF, { self } };
  TTCN_Logger::set_console_mask(cmpt_id, lb);
}

void set__file__mask(CHARSTRING const& plugin, Severities const& mask)
{
  validate_plugin_name(plugin);

  Logging_Bits lb(Logging_Bits::log_nothing);
  for (int B = mask.size_of() - 1; B >= 0; B--) {
    lb.add_sev(static_cast<TTCN_Logger::Severity>((int)mask[B]));
  }
  component_id_t cmpt_id = { COMPONENT_ID_COMPREF, { self } };
  TTCN_Logger::set_file_mask(cmpt_id, lb);
}

void add__to__console__mask(CHARSTRING const& plugin, Severities const& mask)
{
  validate_plugin_name(plugin);

  Logging_Bits lb(TTCN_Logger::get_console_mask());
  for (int B = mask.size_of() - 1; B >= 0; B--) {
    lb.add_sev(static_cast<TTCN_Logger::Severity>((int)mask[B]));
  }
  component_id_t cmpt_id = { COMPONENT_ID_COMPREF, { self } };
  TTCN_Logger::set_console_mask(cmpt_id, lb);
}

void add__to__file__mask(CHARSTRING const& plugin, Severities const& mask)
{
  validate_plugin_name(plugin);

  Logging_Bits lb(TTCN_Logger::get_file_mask());
  for (int B = mask.size_of() - 1; B >= 0; B--) {
    lb.add_sev(static_cast<TTCN_Logger::Severity>((int)mask[B]));
  }
  component_id_t cmpt_id = { COMPONENT_ID_COMPREF, { self } };
  TTCN_Logger::set_file_mask(cmpt_id, lb);
}

void remove__from__console__mask(CHARSTRING const& plugin, Severities const& mask)
{
  validate_plugin_name(plugin);

  Logging_Bits lb(TTCN_Logger::get_console_mask());
  for (int B = mask.size_of() - 1; B >= 0; B--) {
    TTCN_Logger::Severity sev = static_cast<TTCN_Logger::Severity>((int)mask[B]);
    if (sev > 0 && sev < TTCN_Logger::NUMBER_OF_LOGSEVERITIES) {
      lb.bits[sev] = FALSE;
    }
  }
  component_id_t cmpt_id = { COMPONENT_ID_COMPREF, { self } };
  TTCN_Logger::set_console_mask(cmpt_id, lb);
}

void remove__from__file__mask(CHARSTRING const& plugin, Severities const& mask)
{
  validate_plugin_name(plugin);

  Logging_Bits lb(TTCN_Logger::get_file_mask());
  for (int B = mask.size_of() - 1; B >= 0; B--) {
    TTCN_Logger::Severity sev = static_cast<TTCN_Logger::Severity>((int)mask[B]);
    if (sev > 0 && sev < TTCN_Logger::NUMBER_OF_LOGSEVERITIES) {
      lb.bits[sev] = FALSE;
    }
  }
  component_id_t cmpt_id = { COMPONENT_ID_COMPREF, { self } };
  TTCN_Logger::set_file_mask(cmpt_id, lb);
}

} // namespace
