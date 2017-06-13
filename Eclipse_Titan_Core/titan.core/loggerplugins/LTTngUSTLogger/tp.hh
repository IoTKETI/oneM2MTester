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

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER titan_core

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./tp.hh"

#if !defined(_TITAN_CORE_TP_HH) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define _TITAN_CORE_TP_HH

#include <lttng/tracepoint.h>
#include <limits.h>

#define _null_string "(null)"

#define _ctf_xsd_integer(_field, _member)                               \
  ctf_integer(int, _field, static_cast<int>(_member))

#define _ctf_xsd_boolean(_field, _member)                               \
  ctf_enum(titan_core, bool, unsigned char, _field,                     \
           static_cast<boolean>(_member) ? 1 : 0)

#define _ctf_xsd_float(_field, _member)                                 \
  ctf_float(double, _field, static_cast<double>(_member))

#define _ctf_xsd_charstring(_field, _member)                            \
  ctf_string(_field, (_member).is_bound() ?                             \
                       static_cast<const char *>(_member) : _null_string)

#define _ctf_xsd_enum(_enum, _field, _member)                           \
  ctf_enum(titan_core, _enum, int, _field,                              \
           (_member).is_bound() ?                                       \
             ((TitanLoggerApi::_enum::enum_type) (_member) < TitanLoggerApi::_enum::UNKNOWN_VALUE ? \
               (int) ((TitanLoggerApi::_enum::enum_type) (_member)) : INT_MAX \
             ) : INT_MAX)

#define _ctf_xsd_opt_string(_field, _member)                            \
  _ctf_xsd_integer(_field ## _present, (_member).is_present())          \
  ctf_string(_field, (_member).is_present() ?                           \
                       ((_member)().is_bound() ?                        \
                         static_cast<const char *>((_member)()) :       \
                         _null_string) : _null_string)

#define _ctf_xsd_opt_integer(_field, _member)                           \
  _ctf_xsd_integer(_field ## _present, (_member).is_present())          \
  _ctf_xsd_integer(_field, (_member).is_present() ?                     \
    static_cast<int>((_member)()) : -1)

#define _ctf_xsd_enum_invalid_value                                     \
  ctf_enum_value("(INVALID)", INT_MAX)

#define _common_fields                                                  \
  _ctf_xsd_integer(timestamp_seconds, timestamp.seconds())              \
  _ctf_xsd_integer(timestamp_microseconds, timestamp.microSeconds())    \
  _ctf_xsd_integer(severity, severity)

#define _common_args                                                    \
  const TitanLoggerApi::TimestampType&, timestamp, int, severity

TRACEPOINT_ENUM(
  titan_core,
  bool,
  TP_ENUM_VALUES(
    ctf_enum_value("false", 0)
    ctf_enum_value("true", 1)
  )
)

TRACEPOINT_ENUM(
  titan_core,
  LocationInfo_ent__type,
  TP_ENUM_VALUES(
    ctf_enum_value("unknown", 0)
    ctf_enum_value("controlpart", 1)
    ctf_enum_value("testcase", 2)
    ctf_enum_value("altstep", 3)
    ctf_enum_value("function", 4)
    ctf_enum_value("external_function", 5)
    ctf_enum_value("template", 6)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  sourceInfo,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::LocationInfo&, locationInfo
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_charstring(filename, locationInfo.filename())
    _ctf_xsd_integer(line, locationInfo.line())
    _ctf_xsd_charstring(ent_name, locationInfo.ent__name())
    _ctf_xsd_enum(LocationInfo_ent__type, ent_type, locationInfo.ent__type())
  )
)

TRACEPOINT_EVENT_CLASS(
  titan_core,
  Strings_class,
  TP_ARGS(
    _common_args,
    const char *, merged_string
  ),
  TP_FIELDS(
    _common_fields
    ctf_string(logEvent_merged_strings, merged_string)
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  Strings_class,
  actionEvent,
  TP_ARGS(
    _common_args,
    const char *, merged_string
  )
)

TRACEPOINT_ENUM(
  titan_core,
  DefaultEnd,
  TP_ENUM_VALUES(
    ctf_enum_value("finish", 0)
    ctf_enum_value("repeat", 1)
    ctf_enum_value("break", 2)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT_CLASS(
  titan_core,
  defaultEvent_class,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::DefaultOp&, defaultOp
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_charstring(logEvent_name, defaultOp.name())
    _ctf_xsd_integer(logEvent_id, defaultOp.id())
    _ctf_xsd_enum(DefaultEnd, logEvent_end, defaultOp.end())
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  defaultEvent_class,
  defaultEvent_defaultopActivate,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::DefaultOp&, defaultOp
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  defaultEvent_class,
  defaultEvent_defaultopDeactivate,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::DefaultOp&, defaultOp
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  defaultEvent_class,
  defaultEvent_defaultopExit,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::DefaultOp&, defaultOp
  )
)

TRACEPOINT_EVENT_CLASS(
  titan_core,
  Categorized_class,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Categorized&, event
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_integer(logEvent_category, event.category())
    _ctf_xsd_charstring(logEvent_text, event.text())
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  Categorized_class,
  errorLog,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Categorized&, event
  )
)

TRACEPOINT_LOGLEVEL(titan_core, errorLog, TRACE_ERR)

TRACEPOINT_ENUM(
  titan_core,
  ExecutorRuntime_reason,
  TP_ENUM_VALUES(
    ctf_enum_value("connected_to_mc", 0)
    ctf_enum_value("disconnected_from_mc", 1)
    ctf_enum_value("initialization_of_modules_failed", 2)
    ctf_enum_value("exit_requested_from_mc_hc", 3)
    ctf_enum_value("exit_requested_from_mc_mtc", 4)
    ctf_enum_value("stop_was_requested_from_mc_ignored_on_idle_mtc", 5)
    ctf_enum_value("stop_was_requested_from_mc", 6)
    ctf_enum_value("stop_was_requested_from_mc_ignored_on_idle_ptc", 7)
    ctf_enum_value("executing_testcase_in_module", 8)
    ctf_enum_value("performing_error_recovery", 9)
    ctf_enum_value("initializing_module", 10)
    ctf_enum_value("initialization_of_module_finished", 11)
    ctf_enum_value("stopping_current_testcase", 12)
    ctf_enum_value("exiting", 13)
    ctf_enum_value("host_controller_started", 14)
    ctf_enum_value("host_controller_finished", 15)
    ctf_enum_value("stopping_control_part_execution", 16)
    ctf_enum_value("stopping_test_component_execution", 17)
    ctf_enum_value("waiting_for_ptcs_to_finish", 18)
    ctf_enum_value("user_paused_waiting_to_resume", 19)
    ctf_enum_value("resuming_execution", 20)
    ctf_enum_value("terminating_execution", 21)
    ctf_enum_value("mtc_created", 22)
    ctf_enum_value("overload_check", 23)
    ctf_enum_value("overload_check_fail", 24)
    ctf_enum_value("overloaded_no_more", 25)
    ctf_enum_value("executor_start_single_mode", 26)
    ctf_enum_value("executor_finish_single_mode", 27)
    ctf_enum_value("fd_limits", 28)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  executorEvent_executorRuntime,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::ExecutorRuntime&, runtime
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(ExecutorRuntime_reason, logEvent_reason, runtime.reason())
    _ctf_xsd_opt_string(logEvent_module_name, runtime.module__name())
    _ctf_xsd_opt_string(logEvent_testcase_name, runtime.testcase__name())
    _ctf_xsd_opt_integer(logEvent_pid, runtime.pid())
    _ctf_xsd_opt_integer(logEvent_fd_setsize, runtime.fd__setsize())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  ExecutorConfigdata_reason,
  TP_ENUM_VALUES(
    ctf_enum_value("received_from_mc", 0)
    ctf_enum_value("processing_failed", 1)
    ctf_enum_value("processing_succeeded", 2)
    ctf_enum_value("module_has_parameters", 3)
    ctf_enum_value("using_config_file", 4)
    ctf_enum_value("overriding_testcase_list", 5)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  executorEvent_executorConfigdata,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::ExecutorConfigdata&, configData
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(ExecutorConfigdata_reason, logEvent_reason,
                  configData.reason())
    _ctf_xsd_opt_string(logEvent_param, configData.param__())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  executorEvent_extcommandStart,
  TP_ARGS(
    _common_args,
    const char *, string
  ),
  TP_FIELDS(
    _common_fields
    ctf_string(logEvent_string, string)
  )
)

TRACEPOINT_EVENT(
  titan_core,
  executorEvent_extcommandSuccess,
  TP_ARGS(
    _common_args,
    const char *, string
  ),
  TP_FIELDS(
    _common_fields
    ctf_string(logEvent_string, string)
  )
)

TRACEPOINT_ENUM(
  titan_core,
  ExecutorComponent_reason,
  TP_ENUM_VALUES(
    ctf_enum_value("mtc_started", 0)
    ctf_enum_value("mtc_finished", 1)
    ctf_enum_value("ptc_started", 2)
    ctf_enum_value("ptc_finished", 3)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  executorEvent_executorComponent,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::ExecutorComponent&, comp
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(ExecutorComponent_reason, logEvent_reason, comp.reason())
    _ctf_xsd_opt_integer(logEvent_compref, comp.compref())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  executorEvent_logOptions,
  TP_ARGS(
    _common_args,
    const char *, string
  ),
  TP_FIELDS(
    _common_fields
    ctf_string(logEvent_string, string)
  )
)

TRACEPOINT_ENUM(
  titan_core,
  ExecutorUnqualified_reason,
  TP_ENUM_VALUES(
    ctf_enum_value("local_address_was_set", 0)
    ctf_enum_value("address_of_mc_was_set", 1)
    ctf_enum_value("address_of_control_connection", 2)
    ctf_enum_value("host_support_unix_domain_sockets", 3)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  executorEvent_executorMisc,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::ExecutorUnqualified&, executor
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(ExecutorUnqualified_reason, logEvent_reason,
                  executor.reason())
    _ctf_xsd_charstring(logEvent_name, executor.name())
    _ctf_xsd_charstring(logEvent_addr, executor.addr())
    _ctf_xsd_integer(logEvent_port, executor.port__())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  functionEvent_unqualified,
  TP_ARGS(
    _common_args,
    const char *, string
  ),
  TP_FIELDS(
    _common_fields
    ctf_string(logEvent_string, string)
  )
)

TRACEPOINT_ENUM(
  titan_core,
  RandomAction,
  TP_ENUM_VALUES(
    ctf_enum_value("seed", 0)
    ctf_enum_value("read_out", 1)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  functionEvent_random,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::FunctionEvent_choice_random&, event
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(RandomAction, logEvent_random_operation,
                  event.operation())
    _ctf_xsd_float(logEvent_retval, event.retval())
    _ctf_xsd_integer(logEvent_intseed, event.intseed())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  ParallelPTC_reason,
  TP_ENUM_VALUES(
    ctf_enum_value("init_component_start", 0)
    ctf_enum_value("init_component_finish", 1)
    ctf_enum_value("terminating_component", 2)
    ctf_enum_value("component_shut_down", 3)
    ctf_enum_value("error_idle_ptc", 4)
    ctf_enum_value("ptc_created", 5)
    ctf_enum_value("ptc_created_pid", 6)
    ctf_enum_value("function_started", 7)
    ctf_enum_value("function_stopped", 8)
    ctf_enum_value("function_finished", 9)
    ctf_enum_value("function_error", 10)
    ctf_enum_value("ptc_done", 11)
    ctf_enum_value("ptc_killed", 12)
    ctf_enum_value("stopping_mtc", 13)
    ctf_enum_value("ptc_stopped", 14)
    ctf_enum_value("all_comps_stopped", 15)
    ctf_enum_value("ptc_was_killed", 16)
    ctf_enum_value("all_comps_killed", 17)
    ctf_enum_value("kill_request_frm_mc", 18)
    ctf_enum_value("mtc_finished", 19)
    ctf_enum_value("ptc_finished", 20)
    ctf_enum_value("starting_function", 21)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  parallelEvent_parallelPTC,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::ParallelPTC&, ptc
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(ParallelPTC_reason, logEvent_reason, ptc.reason())
    _ctf_xsd_charstring(logEvent_module, ptc.module__())
    _ctf_xsd_charstring(logEvent_name, ptc.name())
    _ctf_xsd_integer(logEvent_compref, ptc.compref())
    _ctf_xsd_charstring(logEvent_compname, ptc.compname())
    _ctf_xsd_charstring(logEvent_tc_loc, ptc.tc__loc())
    _ctf_xsd_integer(logEvent_alive_pid, ptc.alive__pid())
    _ctf_xsd_integer(logEvent_status, ptc.status())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  parallelEvent_parallelPTC_exit,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::PTC__exit&, ptc
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_integer(logEvent_compref, ptc.compref())
    _ctf_xsd_integer(logEvent_pid, ptc.pid())
    _ctf_xsd_integer(logEvent_statuscode, ptc.statuscode())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  ParPort_operation,
  TP_ENUM_VALUES(
    ctf_enum_value("connect", 0)
    ctf_enum_value("disconnect", 1)
    ctf_enum_value("map", 2)
    ctf_enum_value("unmap", 3)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  parallelEvent_parallelPort,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::ParPort&, port
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(ParPort_operation, logEvent_operation, port.operation())
    _ctf_xsd_integer(logEvent_srcCompref, port.srcCompref())
    _ctf_xsd_integer(logEvent_dstCompref, port.dstCompref())
    _ctf_xsd_charstring(logEvent_srcPort, port.srcPort())
    _ctf_xsd_charstring(logEvent_dstPort, port.dstPort())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  testcaseOp_testcaseStarted,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::QualifiedName&, qName
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_charstring(logEvent_module_name, qName.module__name())
    _ctf_xsd_charstring(logEvent_testcase_name, qName.testcase__name())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  Verdict,
  TP_ENUM_VALUES(
    ctf_enum_value("v0none", 0)
    ctf_enum_value("v1pass", 1)
    ctf_enum_value("v2inconc", 2)
    ctf_enum_value("v3fail", 3)
    ctf_enum_value("v4error", 4)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  testcaseOp_testcaseFinished,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::TestcaseType&, type
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_charstring(logEvent_name_module_name, type.name().module__name())
    _ctf_xsd_charstring(logEvent_name_testcase_name, type.name().testcase__name())
    _ctf_xsd_enum(Verdict, logEvent_verdict, type.verdict())
    _ctf_xsd_charstring(logEvent_reason, type.reason())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  Port__Queue_operation,
  TP_ENUM_VALUES(
    ctf_enum_value("enqueue_msg", 0)
    ctf_enum_value("enqueue_call", 1)
    ctf_enum_value("enqueue_reply", 2)
    ctf_enum_value("enqueue_exception", 3)
    ctf_enum_value("extract_msg", 4)
    ctf_enum_value("extract_op", 5)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  portEvent_portQueue,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Port__Queue&, queue
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(Port__Queue_operation, logEvent_operation, queue.operation())
    _ctf_xsd_charstring(logEvent_port_name, queue.port__name())
    _ctf_xsd_integer(logEvent_compref, queue.compref())
    _ctf_xsd_integer(logEvent_msgid, queue.msgid())
    _ctf_xsd_charstring(logEvent_address, queue.address__())
    _ctf_xsd_charstring(logEvent_param, queue.param__())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  Port__State_operation,
  TP_ENUM_VALUES(
    ctf_enum_value("started", 0)
    ctf_enum_value("stopped", 1)
    ctf_enum_value("halted", 2)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  portEvent_portState,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Port__State&, state
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(Port__State_operation, logEvent_operation, state.operation())
    _ctf_xsd_charstring(logEvent_port_name, state.port__name())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  Port__oper,
  TP_ENUM_VALUES(
    ctf_enum_value("call_op", 0)
    ctf_enum_value("reply_op", 1)
    ctf_enum_value("exception_op", 2)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  portEvent_procPortSend,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Proc__port__out&, out
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_charstring(logEvent_port_name, out.port__name())
    _ctf_xsd_enum(Port__oper, logEvent_operation, out.operation())
    _ctf_xsd_integer(logEvent_compref, out.compref())
    _ctf_xsd_charstring(logEvent_sys_name, out.sys__name())
    _ctf_xsd_charstring(logEvent_parameter, out.parameter())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  portEvent_procPortRecv,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Proc__port__in&, in
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_charstring(logEvent_port_name, in.port__name())
    _ctf_xsd_enum(Port__oper, logEvent_operation, in.operation())
    _ctf_xsd_integer(logEvent_compref, in.compref())
    _ctf_xsd_boolean(logEvent_check, in.check__())
    _ctf_xsd_charstring(logEvent_parameter, in.parameter())
    _ctf_xsd_integer(logEvent_msgid, in.msgid())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  portEvent_msgPortSend,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Msg__port__send&, msg
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_charstring(logEvent_port_name, msg.port__name())
    _ctf_xsd_integer(logEvent_compref, msg.compref())
    _ctf_xsd_charstring(logEvent_parameter, msg.parameter())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  Msg__port__recv_operation,
  TP_ENUM_VALUES(
    ctf_enum_value("receive_op", 0)
    ctf_enum_value("check_receive_op", 1)
    ctf_enum_value("trigger_op", 2)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  portEvent_msgPortRecv,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Msg__port__recv&, msg
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_charstring(logEvent_port_name, msg.port__name())
    _ctf_xsd_enum(Msg__port__recv_operation, logEvent_operation, msg.operation())
    _ctf_xsd_integer(logEvent_compref, msg.compref())
    _ctf_xsd_charstring(logEvent_sys_name, msg.sys__name())
    _ctf_xsd_charstring(logEvent_parameter, msg.parameter())
    _ctf_xsd_integer(logEvent_msgid, msg.msgid())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  portEvent_dualMapped,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Dualface__mapped&, dfm
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_boolean(logEvent_incoming, dfm.incoming())
    _ctf_xsd_charstring(logEvent_target_type, dfm.target__type())
    _ctf_xsd_charstring(logEvent_value, dfm.value__())
    _ctf_xsd_integer(logEvent_msgid, dfm.msgid())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  portEvent_dualDiscard,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Dualface__discard&, dfd
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_boolean(logEvent_incoming, dfd.incoming())
    _ctf_xsd_charstring(logEvent_target_type, dfd.target__type())
    _ctf_xsd_charstring(logEvent_port_name, dfd.port__name())
    _ctf_xsd_boolean(logEvent_unhandled, dfd.unhandled())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  Port__Misc_reason,
  TP_ENUM_VALUES(
    ctf_enum_value("removing_unterminated_connection", 0)
    ctf_enum_value("removing_unterminated_mapping", 1)
    ctf_enum_value("port_was_cleared", 2)
    ctf_enum_value("local_connection_established", 3)
    ctf_enum_value("local_connection_terminated", 4)
    ctf_enum_value("port_is_waiting_for_connection_tcp", 5)
    ctf_enum_value("port_is_waiting_for_connection_unix", 6)
    ctf_enum_value("connection_established", 7)
    ctf_enum_value("destroying_unestablished_connection", 8)
    ctf_enum_value("terminating_connection", 9)
    ctf_enum_value("sending_termination_request_failed", 10)
    ctf_enum_value("termination_request_received", 11)
    ctf_enum_value("acknowledging_termination_request_failed", 12)
    ctf_enum_value("sending_would_block", 13)
    ctf_enum_value("connection_accepted", 14)
    ctf_enum_value("connection_reset_by_peer", 15)
    ctf_enum_value("connection_closed_by_peer", 16)
    ctf_enum_value("port_disconnected", 17)
    ctf_enum_value("port_was_mapped_to_system", 18)
    ctf_enum_value("port_was_unmapped_from_system", 19)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  portEvent_portMisc,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Port__Misc&, misc
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(Port__Misc_reason, logEvent_reason, misc.reason())
    _ctf_xsd_charstring(logEvent_port_name, misc.port__name())
    _ctf_xsd_integer(logEvent_remote_component, misc.remote__component())
    _ctf_xsd_charstring(logEvent_remote_port, misc.remote__port())
    _ctf_xsd_charstring(logEvent_ip_address, misc.ip__address())
    _ctf_xsd_integer(logEvent_tcp_port, misc.tcp__port())
    _ctf_xsd_integer(logEvent_new_size, misc.new__size())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  statistics_verdictStatistics,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::StatisticsType_choice_verdictStatistics&, stats
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_integer(logEvent_none, stats.none__())
    _ctf_xsd_float(logEvent_nonePercent, stats.nonePercent())
    _ctf_xsd_integer(logEvent_pass, stats.pass__())
    _ctf_xsd_float(logEvent_passPercent, stats.passPercent())
    _ctf_xsd_integer(logEvent_inconc, stats.inconc__())
    _ctf_xsd_float(logEvent_inconcPercent, stats.inconcPercent())
    _ctf_xsd_integer(logEvent_fail, stats.fail__())
    _ctf_xsd_float(logEvent_failPercent, stats.failPercent())
    _ctf_xsd_integer(logEvent_error, stats.error__())
    _ctf_xsd_float(logEvent_errorPercent, stats.errorPercent())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  statistics_controlpartStart,
  TP_ARGS(
    _common_args,
    const char *, string
  ),
  TP_FIELDS(
    _common_fields
    ctf_string(logEvent_string, string)
  )
)

TRACEPOINT_EVENT(
  titan_core,
  statistics_controlpartFinish,
  TP_ARGS(
    _common_args,
    const char *, string
  ),
  TP_FIELDS(
    _common_fields
    ctf_string(logEvent_string, string)
  )
)

TRACEPOINT_EVENT(
  titan_core,
  statistics_controlpartErrors,
  TP_ARGS(
    _common_args,
    int, number
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_integer(logEvent_number, number)
  )
)

TRACEPOINT_EVENT_CLASS(
  titan_core,
  TimerType_class,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::TimerType&, timer
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_charstring(logEvent_name, timer.name())
    _ctf_xsd_integer(logEvent_value, timer.value__())
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  TimerType_class,
  timerEvent_readTimer,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::TimerType&, timer
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  TimerType_class,
  timerEvent_startTimer,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::TimerType&, timer
  )
)

TRACEPOINT_EVENT(
  titan_core,
  timerEvent_guardTimer,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::TimerGuardType&, timer
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_float(logEvent_value, timer.value__())
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  TimerType_class,
  timerEvent_stopTimer,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::TimerType&, timer
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  TimerType_class,
  timerEvent_timeoutTimer,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::TimerType&, timer
  )
)

TRACEPOINT_EVENT(
  titan_core,
  timerEvent_timeoutAnyTimer,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::TimerAnyTimeoutType&, timer
  ),
  TP_FIELDS()
)

TRACEPOINT_EVENT(
  titan_core,
  timerEvent_unqualifiedTimer,
  TP_ARGS(
    _common_args,
    const char *, string
  ),
  TP_FIELDS(
    _common_fields
    ctf_string(logEvent_string, string)
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  Strings_class,
  userLog,
  TP_ARGS(
    _common_args,
    const char *, merged_string
  )
)

TRACEPOINT_EVENT(
  titan_core,
  verdictOp_setVerdict,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::SetVerdictType&, verdict
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(Verdict, logEvent_newVerdict, verdict.newVerdict())
    _ctf_xsd_enum(Verdict, logEvent_oldVerdict, verdict.oldVerdict())
    _ctf_xsd_enum(Verdict, logEvent_localVerdict, verdict.localVerdict())
    _ctf_xsd_opt_string(logEvent_oldReason, verdict.oldReason())
    _ctf_xsd_opt_string(logEvent_newReason, verdict.newReason())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  verdictOp_getVerdict,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Verdict&, verdict
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(Verdict, logEvent_verdict, verdict)
  )
)

TRACEPOINT_EVENT(
  titan_core,
  verdictOp_finalVerdict_info,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::FinalVerdictInfo&, info
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_boolean(logEvent_is_ptc, info.is__ptc())
    _ctf_xsd_enum(Verdict, logEvent_ptc_verdict, info.ptc__verdict())
    _ctf_xsd_enum(Verdict, logEvent_local_verdict, info.local__verdict())
    _ctf_xsd_enum(Verdict, logEvent_new_verdict, info.new__verdict())
    _ctf_xsd_opt_string(logEvent_verdict_reason, info.verdict__reason())
    _ctf_xsd_opt_integer(logEvent_ptc_compref, info.ptc__compref())
    _ctf_xsd_opt_string(logEvent_ptc_name, info.ptc__name())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  FinalVerdictType_choice_notification,
  TP_ENUM_VALUES(
    ctf_enum_value("setting_final_verdict_of_the_test_case.", 0)
    ctf_enum_value("no_ptcs_were_created.", 1)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  verdictOp_finalVerdict_notification,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::FinalVerdictType_choice_notification&, notif
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(FinalVerdictType_choice_notification, logEvent_notification, notif)
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  Categorized_class,
  warningLog,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Categorized&, event
  )
)

TRACEPOINT_LOGLEVEL(titan_core, warningLog, TRACE_WARNING)

TRACEPOINT_ENUM(
  titan_core,
  MatchingDoneType_reason,
  TP_ENUM_VALUES(
    ctf_enum_value("done_failed._wrong_return_type", 0)
    ctf_enum_value("done_failed._no_return", 1)
    ctf_enum_value("any_component_done_successful", 2)
    ctf_enum_value("any_component_done_failed", 3)
    ctf_enum_value("all_component_done_successful", 4)
    ctf_enum_value("any_component_killed_successful", 5)
    ctf_enum_value("any_component_killed_failed", 6)
    ctf_enum_value("all_component_killed_successful", 7)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  matchingEvent_matchingDone,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::MatchingDoneType&, matchingDone
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(MatchingDoneType_reason, logEvent_reason, matchingDone.reason())
    _ctf_xsd_charstring(logEvent_type, matchingDone.type__())
    _ctf_xsd_integer(logEvent_ptc, matchingDone.ptc())
    _ctf_xsd_charstring(logEvent_return_type, matchingDone.return__type())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  PortType,
  TP_ENUM_VALUES(
    ctf_enum_value("message", 0)
    ctf_enum_value("procedure", 1)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  matchingEvent_matchingSuccess,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::MatchingSuccessType&, matchingSuccess
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(PortType, logEvent_port_type, matchingSuccess.port__type())
    _ctf_xsd_charstring(logEvent_port_name, matchingSuccess.port__name())
    _ctf_xsd_charstring(logEvent_info, matchingSuccess.info())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  MatchingFailureType_reason,
  TP_ENUM_VALUES(
    ctf_enum_value("sender_does_not_match_from_clause", 0)
    ctf_enum_value("sender_is_not_system", 1)
    ctf_enum_value("message_does_not_match_template", 2)
    ctf_enum_value("parameters_of_call_do_not_match_template", 3)
    ctf_enum_value("parameters_of_reply_do_not_match_template", 4)
    ctf_enum_value("exception_does_not_match_template", 5)
    ctf_enum_value("not_an_exception_for_signature", 6)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  matchingEvent_matchingFailure_system,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::MatchingFailureType&, matchingFailure
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(PortType, logEvent_port_type, matchingFailure.port__type())
    _ctf_xsd_charstring(logEvent_port_name, matchingFailure.port__name())
    _ctf_xsd_charstring(logEvent_system, matchingFailure.choice().system__())
    _ctf_xsd_enum(MatchingFailureType_reason, logEvent_reason, matchingFailure.reason())
    _ctf_xsd_charstring(logEvent_info, matchingFailure.info())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  matchingEvent_matchingFailure_compref,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::MatchingFailureType&, matchingFailure
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_enum(PortType, logEvent_port_type, matchingFailure.port__type())
    _ctf_xsd_charstring(logEvent_port_name, matchingFailure.port__name())
    _ctf_xsd_integer(logEvent_compref, matchingFailure.choice().compref())
    _ctf_xsd_enum(MatchingFailureType_reason, logEvent_reason, matchingFailure.reason())
    _ctf_xsd_charstring(logEvent_info, matchingFailure.info())
  )
)

TRACEPOINT_ENUM(
  titan_core,
  MatchingProblemType_reason,
  TP_ENUM_VALUES(
    ctf_enum_value("port_not_started_and_queue_empty", 0)
    ctf_enum_value("no_incoming_types", 1)
    ctf_enum_value("no_incoming_signatures", 2)
    ctf_enum_value("no_outgoing_blocking_signatures", 3)
    ctf_enum_value("no_outgoing_blocking_signatures_that_support_exceptions", 4)
    ctf_enum_value("component_has_no_ports", 5)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_ENUM(
  titan_core,
  MatchingProblemType_operation,
  TP_ENUM_VALUES(
    ctf_enum_value("receive", 0)
    ctf_enum_value("trigger", 1)
    ctf_enum_value("getcall", 2)
    ctf_enum_value("getreply", 3)
    ctf_enum_value("catch", 4)
    ctf_enum_value("check", 5)
    _ctf_xsd_enum_invalid_value
  )
)

TRACEPOINT_EVENT(
  titan_core,
  matchingEvent_matchingProblem,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::MatchingProblemType&, matchingProblem
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_charstring(logEvent_port_name, matchingProblem.port__name())
    _ctf_xsd_enum(MatchingProblemType_reason, logEvent_reason, matchingProblem.reason())
    _ctf_xsd_enum(MatchingProblemType_operation, logEvent_operation, matchingProblem.operation())
    _ctf_xsd_boolean(logEvent_check, matchingProblem.check__())
    _ctf_xsd_boolean(logEvent_any_port, matchingProblem.any__port())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  matchingEvent_matchingTimeout,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::MatchingTimeout&, matchingTimeout
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_opt_string(logEvent_timer_name, matchingTimeout.timer__name())
  )
)

TRACEPOINT_EVENT_INSTANCE(
  titan_core,
  Categorized_class,
  debugLog,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::Categorized&, event
  )
)

TRACEPOINT_LOGLEVEL(titan_core, debugLog, TRACE_DEBUG)

TRACEPOINT_EVENT(
  titan_core,
  executionSummary,
  TP_ARGS(
    _common_args,
    const TitanLoggerApi::ExecutionSummaryType&, summary
  ),
  TP_FIELDS(
    _common_fields
    _ctf_xsd_integer(logEvent_numberOfTestcases, summary.numberOfTestcases())
    _ctf_xsd_charstring(logEvent_overallStatistics, summary.overallStatistics())
  )
)

TRACEPOINT_EVENT(
  titan_core,
  unhandledEvent,
  TP_ARGS(
    _common_args,
    const char *, string
  ),
  TP_FIELDS(
    _common_fields
    ctf_string(logEvent_string, string)
  )
)

#endif /* _TITAN_CORE_TP_HH */

#include <lttng/tracepoint-event.h>
