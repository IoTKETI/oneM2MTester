/******************************************************************************
 * Copyright (c) 2016 EfficiOS Inc., Philippe Proulx
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Proulx, Philippe
 ******************************************************************************/

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <dlfcn.h>

#ifndef TITAN_RUNTIME_2
#include "RT1/TitanLoggerApi.hh"
#else
#include "RT2/TitanLoggerApi.hh"
#endif

#include "LTTngUSTLogger.hh"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE

#include "tp.hh"

#ifndef RTLD_NODELETE
# define RTLD_NODELETE 0
#endif

#define TPP_SO_FILENAME "lttngust-logger-tp.so"

extern "C"
{
  ILoggerPlugin *create_plugin() { return new LTTngUSTLogger(); }
  void destroy_plugin(ILoggerPlugin *plugin) { delete plugin; }
}

LTTngUSTLogger::LTTngUSTLogger()
{
  major_version_ = 1;
  minor_version_ = 0;
  name_ = mcopystr("LTTngUSTLogger");
  help_ = mcopystr("LTTngUSTLogger writes CTF traces using LTTng-UST");
  is_configured_ = true;

  /* Open tracepoint provider. */
  hasTpp = (dlopen(TPP_SO_FILENAME, RTLD_NOW | RTLD_NODELETE) != NULL);

  if (!hasTpp) {
    TTCN_warning("Cannot open \"%s\" to load LTTng-UST tracepoint provider. "
                 "The LTTngUSTLogger plugin will not emit LTTng-UST events.",
                 TPP_SO_FILENAME);
  }
}

LTTngUSTLogger::~LTTngUSTLogger()
{
  Free(name_);
  name_ = NULL;
  Free(help_);
  help_ = NULL;
}

CHARSTRING LTTngUSTLogger::joinStrings(const TitanLoggerApi::Strings& strings)
{
  CHARSTRING mergedString("");

  for (int i = 0; i < strings.str__list().n_elem(); i++) {
    if (i != 0) {
      mergedString += '\n';
    }

    mergedString += strings.str__list()[i];
  }

  return mergedString;
}

void LTTngUSTLogger::log(const TitanLoggerApi::TitanLogEvent& event,
                         bool log_buffered, bool separate_file,
                         bool use_emergency_mask)
{
  using namespace TitanLoggerApi;

  if (!hasTpp) {
    return;
  }

  /* Read timestamp and severity (common arguments). */
  const TimestampType& timestamp = event.timestamp();
  int severity = (int) event.severity();

  /* Log source infos of this event. */
  for (int i = 0; i < event.sourceInfo__list().n_elem(); i++) {
    tracepoint(titan_core, sourceInfo, timestamp, severity,
               event.sourceInfo__list()[i]);
  }

  /* Log specific event type. */
  const LogEventType_choice& eventTypeChoice = event.logEvent().choice();

  switch (eventTypeChoice.get_selection()) {
  case LogEventType_choice::ALT_actionEvent:
    tracepoint(titan_core, actionEvent, timestamp, severity,
               (const char *) joinStrings(eventTypeChoice.actionEvent()));
    break;

  case LogEventType_choice::ALT_defaultEvent:
  {
    const DefaultEvent_choice& defaultEventChoice =
      eventTypeChoice.defaultEvent().choice();

    switch (defaultEventChoice.get_selection()) {
    case DefaultEvent_choice::ALT_defaultopActivate:
      tracepoint(titan_core, defaultEvent_defaultopActivate,
                 timestamp, severity, defaultEventChoice.defaultopActivate());
      break;

    case DefaultEvent_choice::ALT_defaultopDeactivate:
      tracepoint(titan_core, defaultEvent_defaultopDeactivate,
                 timestamp, severity, defaultEventChoice.defaultopDeactivate());
      break;

    case DefaultEvent_choice::ALT_defaultopExit:
      tracepoint(titan_core, defaultEvent_defaultopExit,
                 timestamp, severity, defaultEventChoice.defaultopExit());
      break;

    default: break;
    }
    break;
  }

  case LogEventType_choice::ALT_errorLog:
    tracepoint(titan_core, errorLog, timestamp, severity,
               eventTypeChoice.errorLog());
    break;

  case LogEventType_choice::ALT_executorEvent:
  {
    const ExecutorEvent_choice& executorEventChoice =
      eventTypeChoice.executorEvent().choice();

    switch (executorEventChoice.get_selection()) {
    case ExecutorEvent_choice::ALT_executorRuntime:
      tracepoint(titan_core, executorEvent_executorRuntime,
                 timestamp, severity, executorEventChoice.executorRuntime());
      break;

    case ExecutorEvent_choice::ALT_executorConfigdata:
      tracepoint(titan_core, executorEvent_executorConfigdata,
                 timestamp, severity, executorEventChoice.executorConfigdata());
      break;

    case ExecutorEvent_choice::ALT_extcommandStart:
      tracepoint(titan_core, executorEvent_extcommandStart,
                 timestamp, severity, executorEventChoice.extcommandStart());
      break;

    case ExecutorEvent_choice::ALT_extcommandSuccess:
      tracepoint(titan_core, executorEvent_extcommandSuccess,
                 timestamp, severity, executorEventChoice.extcommandSuccess());
      break;

    case ExecutorEvent_choice::ALT_executorComponent:
      tracepoint(titan_core, executorEvent_executorComponent,
                 timestamp, severity, executorEventChoice.executorComponent());
      break;

    case ExecutorEvent_choice::ALT_logOptions:
      tracepoint(titan_core, executorEvent_logOptions,
                 timestamp, severity, executorEventChoice.logOptions());
      break;

    case ExecutorEvent_choice::ALT_executorMisc:
      tracepoint(titan_core, executorEvent_executorMisc,
                 timestamp, severity, executorEventChoice.executorMisc());
      break;

    default: break;
    }
    break;
  }

  case LogEventType_choice::ALT_functionEvent:
  {
    const FunctionEvent_choice& functionEventChoice =
      eventTypeChoice.functionEvent().choice();

    switch (functionEventChoice.get_selection()) {
    case FunctionEvent_choice::ALT_unqualified:
      tracepoint(titan_core, functionEvent_unqualified,
                 timestamp, severity,
                 (const char *) functionEventChoice.unqualified());
      break;

    case FunctionEvent_choice::ALT_random:
      tracepoint(titan_core, functionEvent_random,
                 timestamp, severity, functionEventChoice.random());
      break;

    default: break;
    }
    break;
  }

  case LogEventType_choice::ALT_parallelEvent:
  {
    const ParallelEvent_choice& parallelEventChoice =
      eventTypeChoice.parallelEvent().choice();

    switch (parallelEventChoice.get_selection()) {
    case ParallelEvent_choice::ALT_parallelPTC:
      tracepoint(titan_core, parallelEvent_parallelPTC,
                 timestamp, severity,
                 parallelEventChoice.parallelPTC());
      break;

    case ParallelEvent_choice::ALT_parallelPTC__exit:
      tracepoint(titan_core, parallelEvent_parallelPTC_exit,
                 timestamp, severity,
                 parallelEventChoice.parallelPTC__exit());
      break;

    case ParallelEvent_choice::ALT_parallelPort:
      tracepoint(titan_core, parallelEvent_parallelPort,
                 timestamp, severity,
                 parallelEventChoice.parallelPort());
      break;

    default: break;
    }
    break;
  }

  case LogEventType_choice::ALT_testcaseOp:
  {
    const TestcaseEvent_choice& testcaseEventChoice =
      eventTypeChoice.testcaseOp().choice();

    switch (testcaseEventChoice.get_selection()) {
    case TestcaseEvent_choice::ALT_testcaseStarted:
      tracepoint(titan_core, testcaseOp_testcaseStarted,
                 timestamp, severity,
                 testcaseEventChoice.testcaseStarted());
      break;

    case TestcaseEvent_choice::ALT_testcaseFinished:
      tracepoint(titan_core, testcaseOp_testcaseFinished,
                 timestamp, severity,
                 testcaseEventChoice.testcaseFinished());
      break;

    default: break;
    }
    break;
  }

  case LogEventType_choice::ALT_portEvent:
  {
    const PortEvent_choice& portEventChoice =
      eventTypeChoice.portEvent().choice();

    switch (portEventChoice.get_selection()) {
    case PortEvent_choice::ALT_portQueue:
      tracepoint(titan_core, portEvent_portQueue,
                 timestamp, severity,
                 portEventChoice.portQueue());
      break;

    case PortEvent_choice::ALT_portState:
      tracepoint(titan_core, portEvent_portState,
                 timestamp, severity,
                 portEventChoice.portState());
      break;

    case PortEvent_choice::ALT_procPortSend:
      tracepoint(titan_core, portEvent_procPortSend,
                 timestamp, severity,
                 portEventChoice.procPortSend());
      break;

    case PortEvent_choice::ALT_procPortRecv:
      tracepoint(titan_core, portEvent_procPortRecv,
                 timestamp, severity,
                 portEventChoice.procPortRecv());
      break;

    case PortEvent_choice::ALT_msgPortSend:
      tracepoint(titan_core, portEvent_msgPortSend,
                 timestamp, severity,
                 portEventChoice.msgPortSend());
      break;

    case PortEvent_choice::ALT_msgPortRecv:
      tracepoint(titan_core, portEvent_msgPortRecv,
                 timestamp, severity,
                 portEventChoice.msgPortRecv());
      break;

    case PortEvent_choice::ALT_dualMapped:
      tracepoint(titan_core, portEvent_dualMapped,
                 timestamp, severity,
                 portEventChoice.dualMapped());
      break;

    case PortEvent_choice::ALT_dualDiscard:
      tracepoint(titan_core, portEvent_dualDiscard,
                 timestamp, severity,
                 portEventChoice.dualDiscard());
      break;

    case PortEvent_choice::ALT_portMisc:
      tracepoint(titan_core, portEvent_portMisc,
                 timestamp, severity,
                 portEventChoice.portMisc());
      break;

    default: break;
    }
    break;
  }

  case LogEventType_choice::ALT_statistics:
  {
    const StatisticsType_choice& statsChoice =
      eventTypeChoice.statistics().choice();

    switch (statsChoice.get_selection()) {
    case StatisticsType_choice::ALT_verdictStatistics:
      tracepoint(titan_core, statistics_verdictStatistics,
                 timestamp, severity,
                 statsChoice.verdictStatistics());
      break;

    case StatisticsType_choice::ALT_controlpartStart:
      tracepoint(titan_core, statistics_controlpartStart,
                 timestamp, severity,
                 statsChoice.controlpartStart());
      break;

    case StatisticsType_choice::ALT_controlpartFinish:
      tracepoint(titan_core, statistics_controlpartFinish,
                 timestamp, severity,
                 statsChoice.controlpartFinish());
      break;

    case StatisticsType_choice::ALT_controlpartErrors:
      tracepoint(titan_core, statistics_controlpartErrors,
                 timestamp, severity,
                 statsChoice.controlpartErrors());
      break;

    default: break;
    }
    break;
  }

  case LogEventType_choice::ALT_timerEvent:
  {
    const TimerEvent_choice& timerEventChoice =
      eventTypeChoice.timerEvent().choice();

    switch (timerEventChoice.get_selection()) {
    case TimerEvent_choice::ALT_readTimer:
      tracepoint(titan_core, timerEvent_readTimer,
                 timestamp, severity,
                 timerEventChoice.readTimer());
      break;

    case TimerEvent_choice::ALT_startTimer:
      tracepoint(titan_core, timerEvent_startTimer,
                 timestamp, severity,
                 timerEventChoice.startTimer());
      break;

    case TimerEvent_choice::ALT_guardTimer:
      tracepoint(titan_core, timerEvent_guardTimer,
                 timestamp, severity,
                 timerEventChoice.guardTimer());
      break;

    case TimerEvent_choice::ALT_stopTimer:
      tracepoint(titan_core, timerEvent_stopTimer,
                 timestamp, severity,
                 timerEventChoice.stopTimer());
      break;

    case TimerEvent_choice::ALT_timeoutTimer:
      tracepoint(titan_core, timerEvent_timeoutTimer,
                 timestamp, severity,
                 timerEventChoice.timeoutTimer());
      break;

    case TimerEvent_choice::ALT_timeoutAnyTimer:
      tracepoint(titan_core, timerEvent_timeoutAnyTimer,
                 timestamp, severity,
                 timerEventChoice.timeoutAnyTimer());
      break;

    case TimerEvent_choice::ALT_unqualifiedTimer:
      tracepoint(titan_core, timerEvent_unqualifiedTimer,
                 timestamp, severity,
                 (const char *) timerEventChoice.unqualifiedTimer());
      break;

    default: break;
    }
    break;
  }

  case LogEventType_choice::ALT_userLog:
    tracepoint(titan_core, userLog, timestamp, severity,
               (const char *) joinStrings(eventTypeChoice.userLog()));
    break;

  case LogEventType_choice::ALT_verdictOp:
  {
    const VerdictOp_choice& verdictOpChoice =
      eventTypeChoice.verdictOp().choice();

    switch (verdictOpChoice.get_selection()) {
    case VerdictOp_choice::ALT_setVerdict:
      tracepoint(titan_core, verdictOp_setVerdict,
                 timestamp, severity,
                 verdictOpChoice.setVerdict());
      break;

    case VerdictOp_choice::ALT_getVerdict:
      tracepoint(titan_core, verdictOp_getVerdict,
                 timestamp, severity,
                 verdictOpChoice.getVerdict());
      break;

    case VerdictOp_choice::ALT_finalVerdict:
    {
      const FinalVerdictType_choice& finalVerdictTypeChoice =
        verdictOpChoice.finalVerdict().choice();

      switch (finalVerdictTypeChoice.get_selection()) {
      case FinalVerdictType_choice::ALT_info:
        tracepoint(titan_core, verdictOp_finalVerdict_info,
                   timestamp, severity,
                   finalVerdictTypeChoice.info());
        break;

      case FinalVerdictType_choice::ALT_notification:
        tracepoint(titan_core, verdictOp_finalVerdict_notification,
                   timestamp, severity,
                   finalVerdictTypeChoice.notification());
        break;

      default: break;
      }
      break;
    }

    default: break;
    }
    break;
  }

  case LogEventType_choice::ALT_warningLog:
    tracepoint(titan_core, warningLog, timestamp, severity,
               eventTypeChoice.warningLog());
    break;

  case LogEventType_choice::ALT_matchingEvent:
  {
    const MatchingEvent_choice& matchingEventChoice =
      eventTypeChoice.matchingEvent().choice();

    switch (matchingEventChoice.get_selection()) {
    case MatchingEvent_choice::ALT_matchingDone:
      tracepoint(titan_core, matchingEvent_matchingDone,
                 timestamp, severity,
                 matchingEventChoice.matchingDone());
      break;

    case MatchingEvent_choice::ALT_matchingSuccess:
      tracepoint(titan_core, matchingEvent_matchingSuccess,
                 timestamp, severity,
                 matchingEventChoice.matchingSuccess());
      break;

    case MatchingEvent_choice::ALT_matchingFailure:
      switch (matchingEventChoice.matchingFailure().choice().get_selection()) {
      case MatchingFailureType_choice::ALT_system__:
        tracepoint(titan_core, matchingEvent_matchingFailure_system,
                   timestamp, severity,
                   matchingEventChoice.matchingFailure());
        break;

      case MatchingFailureType_choice::ALT_compref:

        tracepoint(titan_core, matchingEvent_matchingFailure_compref,
                   timestamp, severity,
                   matchingEventChoice.matchingFailure());
        break;

      default: break;
      }
      break;

    case MatchingEvent_choice::ALT_matchingProblem:
      tracepoint(titan_core, matchingEvent_matchingProblem,
                 timestamp, severity,
                 matchingEventChoice.matchingProblem());
      break;

    case MatchingEvent_choice::ALT_matchingTimeout:
      tracepoint(titan_core, matchingEvent_matchingTimeout,
                 timestamp, severity,
                 matchingEventChoice.matchingTimeout());
      break;

    default: break;
    }
    break;
  }

  case LogEventType_choice::ALT_debugLog:
    tracepoint(titan_core, debugLog, timestamp, severity,
               eventTypeChoice.debugLog());
    break;

  case LogEventType_choice::ALT_executionSummary:
    tracepoint(titan_core, executionSummary, timestamp, severity,
               eventTypeChoice.executionSummary());
    break;

  case LogEventType_choice::ALT_unhandledEvent:
    tracepoint(titan_core, unhandledEvent, timestamp, severity,
               (const char *) eventTypeChoice.unhandledEvent());
    break;

  default: break;
  }
}
