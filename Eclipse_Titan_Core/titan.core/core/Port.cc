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
 *   Baranyi, Botond
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Pandi, Krisztian
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include "Port.hh"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include "../common/platform.h"

#include "../common/memory.h"
#include "Component.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Event_Handler.hh"
#include "Fd_And_Timeout_User.hh"
#include "Snapshot.hh"
#include "Communication.hh"
#include "Runtime.hh"
#include "Octetstring.hh"
#include "TitanLoggerApi.hh"

  // maximum number of iterations for binding the UNIX server socket
#define UNIX_BIND_MAX_ITER 100

#include "../common/dbgnew.hh"

PORT *PORT::list_head = NULL, *PORT::list_tail = NULL;

void PORT::add_to_list()
{
  // check for duplicate names
  for (PORT *p = list_head; p != NULL; p = p->list_next) {
    // do nothing if this is already a member of the list
    if (p == this) return;
    else if (!strcmp(p->port_name, port_name))
      TTCN_error("Internal error: There are more than one ports with "
        "name %s.", port_name);
  }
  // append this to the list
  if (list_head == NULL) list_head = this;
  else if (list_tail != NULL) list_tail->list_next = this;
  list_prev = list_tail;
  list_next = NULL;
  list_tail = this;
}

void PORT::remove_from_list()
{
  if (list_prev != NULL) list_prev->list_next = list_next;
  else if (list_head == this) list_head = list_next;
  if (list_next != NULL) list_next->list_prev = list_prev;
  else if (list_tail == this) list_tail = list_prev;
  list_prev = NULL;
  list_next = NULL;
}

PORT *PORT::lookup_by_name(const char *par_port_name)
{
  for (PORT *port = list_head; port != NULL; port = port->list_next)
    if (!strcmp(par_port_name, port->port_name)) return port;
  return NULL;
}

struct PORT::port_parameter {
  component_id_t component_id;
  char *port_name;
  char *parameter_name;
  char *parameter_value;
  struct port_parameter *next_par;
} *PORT::parameter_head = NULL, *PORT::parameter_tail = NULL;

void PORT::apply_parameter(port_parameter *par_ptr)
{
  if (par_ptr->port_name != NULL) {
    // the parameter refers to a specific port
    PORT *port = lookup_by_name(par_ptr->port_name);
    if (port != NULL) port->set_parameter(par_ptr->parameter_name,
      par_ptr->parameter_value);
  } else {
    // the parameter refers to all ports (*)
    for (PORT *port = list_head; port != NULL; port = port->list_next)
      port->set_parameter(par_ptr->parameter_name,
        par_ptr->parameter_value);
  }
}

void PORT::set_system_parameters(const char *system_port)
{
  for (port_parameter *par = parameter_head; par != NULL; par = par->next_par)
    if (par->component_id.id_selector == COMPONENT_ID_SYSTEM &&
      (par->port_name == NULL || !strcmp(par->port_name, system_port)))
      set_parameter(par->parameter_name, par->parameter_value);
}

void PORT::add_parameter(const component_id_t& component_id,
  const char *par_port_name, const char *parameter_name,
  const char *parameter_value)
{
  port_parameter *new_par = new port_parameter;

  new_par->component_id.id_selector = component_id.id_selector;
  switch (component_id.id_selector) {
  case COMPONENT_ID_NAME:
    new_par->component_id.id_name = mcopystr(component_id.id_name);
    break;
  case COMPONENT_ID_COMPREF:
    new_par->component_id.id_compref = component_id.id_compref;
    break;
  default:
    break;
  }

  if (par_port_name == NULL) new_par->port_name = NULL;
  else new_par->port_name = mcopystr(par_port_name);
  new_par->parameter_name = mcopystr(parameter_name);
  new_par->parameter_value = mcopystr(parameter_value);

  new_par->next_par = NULL;
  if (parameter_head == NULL) parameter_head = new_par;
  if (parameter_tail != NULL) parameter_tail->next_par = new_par;
  parameter_tail = new_par;
}

void PORT::clear_parameters()
{
  while (parameter_head != NULL) {
    port_parameter *next_par = parameter_head->next_par;
    if (parameter_head->component_id.id_selector == COMPONENT_ID_NAME)
      Free(parameter_head->component_id.id_name);
    Free(parameter_head->port_name);
    Free(parameter_head->parameter_name);
    Free(parameter_head->parameter_value);
    delete parameter_head;
    parameter_head = next_par;
  }
}

void PORT::set_parameters(component component_reference,
  const char *component_name)
{
  for (port_parameter *par = parameter_head; par != NULL; par = par->next_par)
    switch (par->component_id.id_selector) {
    case COMPONENT_ID_NAME:
      if (component_name != NULL &&
        !strcmp(par->component_id.id_name, component_name))
        apply_parameter(par);
      break;
    case COMPONENT_ID_COMPREF:
      if (par->component_id.id_compref == component_reference)
        apply_parameter(par);
      break;
    case COMPONENT_ID_ALL:
      apply_parameter(par);
      break;
    default:
      break;
    }
}

enum connection_data_type_enum {
  CONN_DATA_LAST = 0, CONN_DATA_MESSAGE = 1, CONN_DATA_CALL = 2,
    CONN_DATA_REPLY = 3, CONN_DATA_EXCEPTION = 4
};

enum connection_state_enum {
  CONN_IDLE, CONN_LISTENING, CONN_CONNECTED, CONN_LAST_MSG_SENT,
  CONN_LAST_MSG_RCVD
};

struct port_connection : public Fd_Event_Handler {
  PORT *owner_port;
  connection_state_enum connection_state;
  component remote_component;
  char *remote_port;
  transport_type_enum transport_type;
  union {
    struct {
      PORT *port_ptr;
    } local;
    struct {
      int comm_fd;
      Text_Buf *incoming_buf;
    } stream;
  };
  struct port_connection *list_prev, *list_next;
  OCTETSTRING sliding_buffer;

  virtual void Handle_Fd_Event(int fd,
    boolean is_readable, boolean is_writeable, boolean is_error);
  virtual ~port_connection();
  virtual void log() const;
};

void port_connection::Handle_Fd_Event(int,
  boolean is_readable, boolean /*is_writeable*/, boolean /*is_error*/)
{
  // Note event for connection with TRANSPORT_LOCAL transport_type
  // may not arrive.
  if (transport_type == TRANSPORT_INET_STREAM
    || transport_type == TRANSPORT_UNIX_STREAM
  ) {
    if (is_readable) {
      if (connection_state == CONN_LISTENING)
        owner_port->handle_incoming_connection(this);
      else owner_port->handle_incoming_data(this);
    }
  } else
    TTCN_error("Internal error: Invalid transport type (%d) in port "
      "connection between %s and %d:%s.", transport_type,
      owner_port->get_name(), remote_component, remote_port);
}

void port_connection::log() const
  {
  TTCN_Logger::log_event("port connection between ");
  owner_port->log(); TTCN_Logger::log_event(" and ");
  TTCN_Logger::log_event(remote_component); TTCN_Logger::log_event(":");
  TTCN_Logger::log_event("%s", remote_port);
  }

port_connection::~port_connection()
{
  if (transport_type == TRANSPORT_INET_STREAM
    || transport_type == TRANSPORT_UNIX_STREAM
  ) {
    if (stream.comm_fd != -1) {
      TTCN_warning_begin("Internal Error: File descriptor %d not "
        "closed/removed in ", stream.comm_fd); log();
      TTCN_warning_end();
    }
  }
  sliding_buffer.clean_up();
}

PORT::PORT(const char *par_port_name)
{
  port_name = par_port_name != NULL ? par_port_name : "<unknown>";
  is_active = FALSE;
  is_started = FALSE;
  is_halted = FALSE;
  list_prev = NULL;
  list_next = NULL;

  connection_list_head = NULL;
  connection_list_tail = NULL;
  n_system_mappings = 0;
  system_mappings = NULL;
}

PORT::~PORT()
{
  if (is_active) deactivate_port();
}

void PORT::set_name(const char * name)
{
  if (name == NULL) TTCN_error("Internal error: Setting an "
    "invalid name for a single element of a port array.");
  port_name = name;
}

void PORT::log() const
  {
  TTCN_Logger::log_event("port %s", port_name);
  }

void PORT::activate_port()
{
  if (!is_active) {
    add_to_list();
    is_active = TRUE;
    msg_head_count = 0;
    msg_tail_count = 0;
    proc_head_count = 0;
    proc_tail_count = 0;
    
    // Only has effect when the translation port has port variables with
    // default values. Only call when it is activated.
    if (n_system_mappings == 0) {
      init_port_variables();
    }
  }
}

void PORT::deactivate_port()
{
  if (is_active) {
    /* In order to proceed with the deactivation we must ignore the
     * following errors:
     * - errors in user code of Test Port (i.e. user_stop, user_unmap)
     * - failures when sending messages to MC (the link may be down)
     */
    boolean is_parallel = !TTCN_Runtime::is_single();
    // terminate all connections
    while (connection_list_head != NULL) {
      TTCN_Logger::log_port_misc(
        TitanLoggerApi::Port__Misc_reason::removing__unterminated__connection,
        port_name,
        connection_list_head->remote_component, connection_list_head->remote_port);
      if (is_parallel) {
        try {
          TTCN_Communication::send_disconnected(port_name,
            connection_list_head->remote_component,
            connection_list_head->remote_port);
        } catch (const TC_Error&) { }
      }
      remove_connection(connection_list_head);
    }
    // terminate all mappings
    while (n_system_mappings > 0) {
      // we must make a copy of the string because unmap() will destroy it
      char *system_port = mcopystr(system_mappings[0]);
      TTCN_Logger::log_port_misc(
        TitanLoggerApi::Port__Misc_reason::removing__unterminated__mapping,
        port_name, NULL_COMPREF, system_port);
      try {
        unmap(system_port);
      } catch (const TC_Error&) { }
      if (is_parallel) {
        try {
          TTCN_Communication::send_unmapped(port_name, system_port);
        } catch (const TC_Error&) { }
      }
      Free(system_port);
    }
    // the previous disconnect/unmap operations may generate incoming events
    // so we should stop and clear the queue after them
    if (is_started || is_halted) {
      try {
        stop();
      } catch (const TC_Error&) { }
    }
    clear_queue();
    // deactivate all event handlers
    Fd_And_Timeout_User::remove_all_fds(this);
    Fd_And_Timeout_User::set_timer(this, 0.0);
    // File descriptor events of port connections are removed
    // in remove_connection
    remove_from_list();
    is_active = FALSE;
  }
}

void PORT::deactivate_all()
{
  while (list_head != NULL) list_head->deactivate_port();
}

void PORT::clear()
{
  if (!is_active) TTCN_error("Internal error: Inactive port %s cannot "
    "be cleared.", port_name);
  if (!is_started && !is_halted) {
    TTCN_warning("Performing clear operation on port %s, which is "
      "already stopped. The operation has no effect.", port_name);
  }
  clear_queue();
  TTCN_Logger::log_port_misc(
    TitanLoggerApi::Port__Misc_reason::port__was__cleared, port_name);
}

void PORT::all_clear()
{
  for (PORT *port = list_head; port != NULL; port = port->list_next)
    port->clear();
}

void PORT::start()
{
  if (!is_active) TTCN_error("Internal error: Inactive port %s cannot "
    "be started.", port_name);
  if (is_started) {
    TTCN_warning("Performing start operation on port %s, which is "
      "already started. The operation will clear the incoming queue.",
      port_name);
    clear_queue();
  } else {
    if (is_halted) {
      // the queue might contain old messages which has to be discarded
      clear_queue();
      is_halted = FALSE;
    }
    user_start();
    is_started = TRUE;
  }
  TTCN_Logger::log_port_state(TitanLoggerApi::Port__State_operation::started,
    port_name);
}

void PORT::all_start()
{
  for (PORT *port = list_head; port != NULL; port = port->list_next)
    port->start();
}

void PORT::stop()
{
  if (!is_active) TTCN_error("Internal error: Inactive port %s cannot "
    "be stopped.", port_name);
  if (is_started) {
    is_started = FALSE;
    is_halted = FALSE;
    user_stop();
    // dropping all messages from the queue because they cannot be
    // extracted by receiving operations anymore
    clear_queue();
  } else if (is_halted) {
    is_halted = FALSE;
    clear_queue();
  } else {
    TTCN_warning("Performing stop operation on port %s, which is "
      "already stopped. The operation has no effect.", port_name);
  }
  TTCN_Logger::log_port_state(TitanLoggerApi::Port__State_operation::stopped,
    port_name);
}

void PORT::all_stop()
{
  for (PORT *port = list_head; port != NULL; port = port->list_next)
    port->stop();
}

void PORT::halt()
{
  if (!is_active) TTCN_error("Internal error: Inactive port %s cannot "
    "be halted.", port_name);
  if (is_started) {
    is_started = FALSE;
    is_halted = TRUE;
    user_stop();
    // keep the messages in the queue
  } else if (is_halted) {
    TTCN_warning("Performing halt operation on port %s, which is "
      "already halted. The operation has no effect.", port_name);
  } else {
    TTCN_warning("Performing halt operation on port %s, which is "
      "already stopped. The operation has no effect.", port_name);
  }
  TTCN_Logger::log_port_state(TitanLoggerApi::Port__State_operation::halted,
    port_name);
}

void PORT::all_halt()
{
  for (PORT *port = list_head; port != NULL; port = port->list_next)
    port->halt();
}

alt_status PORT::receive(const COMPONENT_template&, COMPONENT *, Index_Redirect*)
{
  TTCN_Logger::log_matching_problem(
    TitanLoggerApi::MatchingProblemType_reason::no__incoming__types,
    TitanLoggerApi::MatchingProblemType_operation::receive__,
    FALSE, FALSE, port_name);
  return ALT_NO;
}

alt_status PORT::any_receive(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr)
{
  if (list_head != NULL) {
    alt_status ret_val = ALT_NO;
    for (PORT *port = list_head; port != NULL; port = port->list_next) {
      switch (port->receive(sender_template, sender_ptr)) {
      case ALT_YES:
        return ALT_YES;
      case ALT_MAYBE:
        ret_val = ALT_MAYBE;
        break;
      case ALT_NO:
        break;
      default:
        TTCN_error("Internal error: Receive operation returned "
          "unexpected status code on port %s while evaluating "
          "`any port.receive'.", port->port_name);
      }
    }
    return ret_val;
  } else {
    TTCN_Logger::log_matching_problem(
      TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports,
      TitanLoggerApi::MatchingProblemType_operation::receive__,
      TRUE, FALSE);
    return ALT_NO;
  }
}

alt_status PORT::check_receive(const COMPONENT_template&, COMPONENT *, Index_Redirect*)
{
  TTCN_Logger::log_matching_problem(
    TitanLoggerApi::MatchingProblemType_reason::no__incoming__types,
    TitanLoggerApi::MatchingProblemType_operation::receive__,
    FALSE, TRUE, port_name);
  return ALT_NO;
}

alt_status PORT::any_check_receive(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr)
{
  if (list_head != NULL) {
    alt_status ret_val = ALT_NO;
    for (PORT *port = list_head; port != NULL; port = port->list_next) {
      switch (port->check_receive(sender_template, sender_ptr)) {
      case ALT_YES:
        return ALT_YES;
      case ALT_MAYBE:
        ret_val = ALT_MAYBE;
        break;
      case ALT_NO:
        break;
      default:
        TTCN_error("Internal error: Check-receive operation returned "
          "unexpected status code on port %s while evaluating "
          "`any port.check(receive)'.", port->port_name);
      }
    }
    return ret_val;
  } else {
    TTCN_Logger::log_matching_problem(
      TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports,
      TitanLoggerApi::MatchingProblemType_operation::receive__,
      TRUE, TRUE);
    return ALT_NO;
  }
}

alt_status PORT::trigger(const COMPONENT_template&, COMPONENT *, Index_Redirect*)
{
  TTCN_Logger::log_matching_problem(
    TitanLoggerApi::MatchingProblemType_reason::no__incoming__types,
    TitanLoggerApi::MatchingProblemType_operation::trigger__,
    FALSE, FALSE, port_name);
  return ALT_NO;
}

alt_status PORT::any_trigger(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr)
{
  if (list_head != NULL) {
    alt_status ret_val = ALT_NO;
    for (PORT *port = list_head; port != NULL; port = port->list_next) {
      switch (port->trigger(sender_template, sender_ptr)) {
      case ALT_YES:
        return ALT_YES;
      case ALT_MAYBE:
        ret_val = ALT_MAYBE;
        break;
      case ALT_NO:
        break;
      case ALT_REPEAT:
        return ALT_REPEAT;
      default:
        TTCN_error("Internal error: Trigger operation returned "
          "unexpected status code on port %s while evaluating "
          "`any port.trigger'.", port->port_name);
      }
    }
    return ret_val;
  } else {
    TTCN_Logger::log_matching_problem(
      TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports,
      TitanLoggerApi::MatchingProblemType_operation::trigger__,
      TRUE, FALSE);
    return ALT_NO;
  }
}

alt_status PORT::getcall(const COMPONENT_template&, COMPONENT *, Index_Redirect*)
{
//  ToDo:Unnecessary log matching problem warning removed.
//  Question: does it unnecessary?
//  TTCN_Logger::log_matching_problem(
//    TitanLoggerApi::MatchingProblemType_reason::no__incoming__signatures,
//    TitanLoggerApi::MatchingProblemType_operation::getcall__,
//    false, false, port_name);
  return ALT_NO;
}

alt_status PORT::any_getcall(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr)
{
  if (list_head != NULL) {
    alt_status ret_val = ALT_NO;
    for (PORT *port = list_head; port != NULL; port = port->list_next) {
      switch (port->getcall(sender_template, sender_ptr)) {
      case ALT_YES:
        return ALT_YES;
      case ALT_MAYBE:
        ret_val = ALT_MAYBE;
        break;
      case ALT_NO:
        break;
      default:
        TTCN_error("Internal error: Getcall operation returned "
          "unexpected status code on port %s while evaluating "
          "`any port.getcall'.", port->port_name);
      }
    }
    return ret_val;
  } else {
    TTCN_Logger::log_matching_problem(
      TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports,
      TitanLoggerApi::MatchingProblemType_operation::getcall__,
      TRUE, FALSE);
    return ALT_NO;
  }
}

alt_status PORT::check_getcall(const COMPONENT_template&, COMPONENT *, Index_Redirect*)
{
//  ToDo:Unnecessary log matching problem warning removed.
//  Question: does it unnecessary
//  TTCN_Logger::log_matching_problem(
//    TitanLoggerApi::MatchingProblemType_reason::no__incoming__signatures,
//    TitanLoggerApi::MatchingProblemType_operation::getcall__,
//    false, false, port_name);
  return ALT_NO;
}

alt_status PORT::any_check_getcall(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr)
{
  if (list_head != NULL) {
    alt_status ret_val = ALT_NO;
    for (PORT *port = list_head; port != NULL; port = port->list_next) {
      switch (port->check_getcall(sender_template, sender_ptr)) {
      case ALT_YES:
        return ALT_YES;
      case ALT_MAYBE:
        ret_val = ALT_MAYBE;
        break;
      case ALT_NO:
        break;
      default:
        TTCN_error("Internal error: Check-getcall operation returned "
          "unexpected status code on port %s while evaluating "
          "`any port.check(getcall)'.", port->port_name);
      }
    }
    return ret_val;
  } else {
    TTCN_Logger::log_matching_problem(
      TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports,
      TitanLoggerApi::MatchingProblemType_operation::getcall__,
      TRUE, TRUE);
    return ALT_NO;
  }
}

alt_status PORT::getreply(const COMPONENT_template&, COMPONENT *, Index_Redirect*)
{
//  ToDo:Unnecessary log matching problem warning removed.
//  Question: does it unnecessary
//  TTCN_Logger::log_matching_problem(
//    TitanLoggerApi::MatchingProblemType_reason::no__outgoing__blocking__signatures,
//    TitanLoggerApi::MatchingProblemType_operation::getreply__,
//    false, false, port_name);
  return ALT_NO;
}

alt_status PORT::any_getreply(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr)
{
  if (list_head != NULL) {
    alt_status ret_val = ALT_NO;
    for (PORT *port = list_head; port != NULL; port = port->list_next) {
      switch (port->getreply(sender_template, sender_ptr)) {
      case ALT_YES:
        return ALT_YES;
      case ALT_MAYBE:
        ret_val = ALT_MAYBE;
        break;
      case ALT_NO:
        break;
      default:
        TTCN_error("Internal error: Getreply operation returned "
          "unexpected status code on port %s while evaluating "
          "`any port.getreply'.", port->port_name);
      }
    }
    return ret_val;
  } else {
    TTCN_Logger::log_matching_problem(
      TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports,
      TitanLoggerApi::MatchingProblemType_operation::getreply__,
      TRUE, FALSE);
    return ALT_NO;
  }
}

alt_status PORT::check_getreply(const COMPONENT_template&, COMPONENT *, Index_Redirect*)
{
//  ToDo:Unnecessary log matching problem warning removed.
//  Question: does it unnecessary
//  TTCN_Logger::log_matching_problem(
//    TitanLoggerApi::MatchingProblemType_reason::no__outgoing__blocking__signatures,
//    TitanLoggerApi::MatchingProblemType_operation::getreply__,
//    false, true, port_name);
  return ALT_NO;
}

alt_status PORT::any_check_getreply(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr)
{
  if (list_head != NULL) {
    alt_status ret_val = ALT_NO;
    for (PORT *port = list_head; port != NULL; port = port->list_next) {
      switch (port->check_getreply(sender_template, sender_ptr)) {
      case ALT_YES:
        return ALT_YES;
      case ALT_MAYBE:
        ret_val = ALT_MAYBE;
        break;
      case ALT_NO:
        break;
      default:
        TTCN_error("Internal error: Check-getreply operation returned "
          "unexpected status code on port %s while evaluating "
          "`any port.check(getreply)'.", port->port_name);
      }
    }
    return ret_val;
  } else {
    TTCN_Logger::log_matching_problem(
      TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports,
      TitanLoggerApi::MatchingProblemType_operation::getreply__,
      TRUE, TRUE);
    return ALT_NO;
  }
}

alt_status PORT::get_exception(const COMPONENT_template&, COMPONENT *, Index_Redirect*)
{
//  ToDo:Unnecessary log matching problem warning removed.
//  Question: does it unnecessary
//  TTCN_Logger::log_matching_problem(
//    TitanLoggerApi::MatchingProblemType_reason::no__outgoing__blocking__signatures__that__support__exceptions,
//    TitanLoggerApi::MatchingProblemType_operation::catch__,
//    false, false, port_name);
  return ALT_NO;
}

alt_status PORT::any_catch(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr)
{
  if (list_head != NULL) {
    alt_status ret_val = ALT_NO;
    for (PORT *port = list_head; port != NULL; port = port->list_next) {
      switch (port->get_exception(sender_template, sender_ptr)) {
      case ALT_YES:
        return ALT_YES;
      case ALT_MAYBE:
        ret_val = ALT_MAYBE;
        break;
      case ALT_NO:
        break;
      default:
        TTCN_error("Internal error: Catch operation returned "
          "unexpected status code on port %s while evaluating "
          "`any port.catch'.", port->port_name);
      }
    }
    return ret_val;
  } else {
    TTCN_Logger::log_matching_problem(
      TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports,
      TitanLoggerApi::MatchingProblemType_operation::catch__,
      TRUE, FALSE);
    return ALT_NO;
  }
}

alt_status PORT::check_catch(const COMPONENT_template& ,
  COMPONENT *, Index_Redirect*)
{
//  ToDo:Unnecessary log matching problem warning removed.
//  Question: does it unnecessary
//  TTCN_Logger::log_matching_problem(
//    TitanLoggerApi::MatchingProblemType_reason::no__outgoing__blocking__signatures__that__support__exceptions,
//    TitanLoggerApi::MatchingProblemType_operation::catch__,
//    false, TRUE, port_name);
  return ALT_NO;
}

alt_status PORT::any_check_catch(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr)
{
  if (list_head != NULL) {
    alt_status ret_val = ALT_NO;
    for (PORT *port = list_head; port != NULL; port = port->list_next) {
      switch (port->check_catch(sender_template, sender_ptr)) {
      case ALT_YES:
        return ALT_YES;
      case ALT_MAYBE:
        ret_val = ALT_MAYBE;
        break;
      case ALT_NO:
        break;
      default:
        TTCN_error("Internal error: Check-catch operation returned "
          "unexpected status code on port %s while evaluating "
          "`any port.check(catch)'.", port->port_name);
      }
    }
    return ret_val;
  } else {
    TTCN_Logger::log_matching_problem(
      TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports,
      TitanLoggerApi::MatchingProblemType_operation::catch__,
      TRUE, TRUE);
    return ALT_NO;
  }
}

alt_status PORT::check(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr, Index_Redirect*)
{
  alt_status ret_val = ALT_NO;
  // the procedure-based queue must have the higher priority
  switch (check_getcall(sender_template, sender_ptr)) {
  case ALT_YES:
    return ALT_YES;
  case ALT_MAYBE:
    ret_val = ALT_MAYBE;
    break;
  case ALT_NO:
    break;
  default:
    TTCN_error("Internal error: Check-getcall operation returned "
      "unexpected status code on port %s.", port_name);
  }
  if (ret_val != ALT_MAYBE) {
    // don't try getreply if the procedure-based queue is empty
    // (i.e. check_getcall() returned ALT_MAYBE)
    switch (check_getreply(sender_template, sender_ptr)) {
    case ALT_YES:
      return ALT_YES;
    case ALT_MAYBE:
      ret_val = ALT_MAYBE;
      break;
    case ALT_NO:
      break;
    default:
      TTCN_error("Internal error: Check-getreply operation returned "
        "unexpected status code on port %s.", port_name);
    }
  }
  if (ret_val != ALT_MAYBE) {
    // don't try catch if the procedure-based queue is empty
    // (i.e. check_getcall() or check_getreply() returned ALT_MAYBE)
    switch (check_catch(sender_template, sender_ptr)) {
    case ALT_YES:
      return ALT_YES;
    case ALT_MAYBE:
      ret_val = ALT_MAYBE;
      break;
    case ALT_NO:
      break;
    default:
      TTCN_error("Internal error: Check-catch operation returned "
        "unexpected status code on port %s.", port_name);
    }
  }
  switch (check_receive(sender_template, sender_ptr)) {
  case ALT_YES:
    return ALT_YES;
  case ALT_MAYBE:
    ret_val = ALT_MAYBE;
    break;
  case ALT_NO:
    break;
  default:
    TTCN_error("Internal error: Check-receive operation returned "
      "unexpected status code on port %s.", port_name);
  }
  return ret_val;
}

alt_status PORT::any_check(const COMPONENT_template& sender_template,
  COMPONENT *sender_ptr)
{
  if (list_head != NULL) {
    alt_status ret_val = ALT_NO;
    for (PORT *port = list_head; port != NULL; port = port->list_next) {
      switch (port->check(sender_template, sender_ptr)) {
      case ALT_YES:
        return ALT_YES;
      case ALT_MAYBE:
        ret_val = ALT_MAYBE;
        break;
      case ALT_NO:
        break;
      default:
        TTCN_error("Internal error: Check operation returned "
          "unexpected status code on port %s while evaluating "
          "`any port.check'.", port->port_name);
      }
    }
    return ret_val;
  } else {
    TTCN_Logger::log_matching_problem(
      TitanLoggerApi::MatchingProblemType_reason::component__has__no__ports,
      TitanLoggerApi::MatchingProblemType_operation::check__,
      TRUE, FALSE);
    return ALT_NO;
  }
}

void PORT::set_parameter(const char *parameter_name, const char *)
{
  TTCN_warning("Test port parameter %s is not supported on port %s.",
    parameter_name, port_name);
}

void PORT::append_to_msg_queue(msg_queue_item_base* new_item)
{
  new_item->next_item = NULL;
  if (msg_queue_tail == NULL) msg_queue_head = new_item;
  else msg_queue_tail->next_item = new_item;
  msg_queue_tail = new_item;
}

void PORT::Handle_Fd_Event(int fd, boolean is_readable, boolean is_writable,
  boolean is_error)
{
  // The port intends to use the finer granularity event handler functions
  if (is_error) {
    Handle_Fd_Event_Error(fd);
    if (!is_writable && !is_readable) return;
    fd_event_type_enum event = Fd_And_Timeout_User::getCurReceivedEvent();
    if ((event & FD_EVENT_WR) == 0) is_writable = FALSE;
    if ((event & FD_EVENT_RD) == 0) is_readable = FALSE;
  }
  if (is_writable) {
    Handle_Fd_Event_Writable(fd);
    if (!is_readable) return;
    if ((Fd_And_Timeout_User::getCurReceivedEvent() & FD_EVENT_RD) == 0)
      return;
  }
  if (is_readable)
    Handle_Fd_Event_Readable(fd);
}

void PORT::Handle_Fd_Event_Error(int)
{
  // Silently ignore
  // A port need not wait for error events
  // Note: error events always cause event handler invocation
}

void PORT::Handle_Fd_Event_Writable(int)
{
  TTCN_error("There is no Handle_Fd_Event_Writable member function "
    "implemented in port %s. "
    "This method or the Handle_Fd_Event method has to be implemented in "
    "the port if the port waits for any file descriptor to be writable - "
    "unless the port uses Install_Handler to specify the file descriptor "
    "and timeout events for which the port waits.", port_name);
}

void PORT::Handle_Fd_Event_Readable(int)
{
  TTCN_error("There is no Handle_Fd_Event_Readable member function "
    "implemented in port %s. "
    "This method or the Handle_Fd_Event method has to be implemented in "
    "the port if the port waits for any file descriptor to be readable - "
    "unless the port uses Install_Handler to specify the file descriptor "
    "and timeout events for which the port waits.", port_name);
}

void PORT::Handle_Timeout(double /*time_since_last_call*/)
{
  TTCN_error("There is no Handle_Timeout member function implemented in "
    "port %s. "
    "This method has to be implemented in the port if the port waits for "
    "timeouts unless the port uses Install_Handler to specify the timeout.",
    port_name);
}

void PORT::Event_Handler(const fd_set * /*read_fds*/, const fd_set * /*write_fds*/,
  const fd_set * /*error_fds*/, double /*time_since_last_call*/)
{
  TTCN_error("There is no Event_Handler implemented in port %s. "
    "Event_Handler has to be implemented in the port if "
    "Install_Handler is used to specify the file descriptor and timeout "
    "events for which the port waits.", port_name);
}

void PORT::Handler_Add_Fd(int fd, Fd_Event_Type event_mask)
{
  Fd_And_Timeout_User::add_fd(fd, this,
    static_cast<fd_event_type_enum>(
      static_cast<int>(event_mask)));
}

void PORT::Handler_Add_Fd_Read(int fd)
{
  Fd_And_Timeout_User::add_fd(fd, this, FD_EVENT_RD);
}

void PORT::Handler_Add_Fd_Write(int fd)
{
  Fd_And_Timeout_User::add_fd(fd, this, FD_EVENT_WR);
}

void PORT::Handler_Remove_Fd(int fd, Fd_Event_Type event_mask)
{
  Fd_And_Timeout_User::remove_fd(fd, this,
    static_cast<fd_event_type_enum>(
      static_cast<int>(event_mask)));
}

void PORT::Handler_Remove_Fd_Read(int fd)
{
  Fd_And_Timeout_User::remove_fd(fd, this, FD_EVENT_RD);
}

void PORT::Handler_Remove_Fd_Write(int fd)
{
  Fd_And_Timeout_User::remove_fd(fd, this, FD_EVENT_WR);
}

void PORT::Handler_Set_Timer(double call_interval, boolean is_timeout,
  boolean call_anyway, boolean is_periodic)
{
  Fd_And_Timeout_User::set_timer(this, call_interval, is_timeout, call_anyway,
    is_periodic);
}

void PORT::Install_Handler(const fd_set *read_fds, const fd_set *write_fds,
  const fd_set *error_fds, double call_interval)
{
  if (!is_active) TTCN_error("Event handler cannot be installed for "
    "inactive port %s.", port_name);

  if ((long) FdMap::getFdLimit() > (long) FD_SETSIZE) {
    static boolean once = TRUE;
    if (once) {
      TTCN_warning("The maximum number of open file descriptors (%i)"
        " is greater than FD_SETSIZE (%li)."
        " Ensure that Test Ports using Install_Handler do not try to"
        " wait for events of file descriptors with values greater than"
        " FD_SETSIZE (%li)."
        " (Current caller of Install_Handler is \"%s\")",
        FdMap::getFdLimit(), (long) FD_SETSIZE, (long) FD_SETSIZE,
        port_name);
    }
    once = FALSE;
  }

  Fd_And_Timeout_User::set_fds_with_fd_sets(this, read_fds, write_fds,
    error_fds);
  Fd_And_Timeout_User::set_timer(this, call_interval);
}

void PORT::Uninstall_Handler()
{
  Fd_And_Timeout_User::remove_all_fds(this);
  Fd_And_Timeout_User::set_timer(this, 0.0);
}

void PORT::user_map(const char *)
{

}

void PORT::user_unmap(const char *)
{

}

void PORT::user_start()
{

}

void PORT::user_stop()
{

}

void PORT::clear_queue()
{

}

component PORT::get_default_destination()
{
  if (connection_list_head != NULL) {
    if (n_system_mappings > 0) TTCN_error("Port %s has both connection(s) "
      "and mapping(s). Message can be sent on it only with explicit "
      "addressing.", port_name);
    else if (connection_list_head->list_next != NULL) TTCN_error("Port %s "
      "has more than one active connections. Message can be sent on it "
      "only with explicit addressing.", port_name);
    return connection_list_head->remote_component;
  } else {
    if (n_system_mappings > 1) {
      TTCN_error("Port %s has more than one mappings. Message cannot "
        "be sent on it to system.", port_name);
    } else if (n_system_mappings < 1) {
      TTCN_error("Port %s has neither connections nor mappings. "
        "Message cannot be sent on it.", port_name);
    }
    return SYSTEM_COMPREF;
  }
}

void PORT::prepare_message(Text_Buf& outgoing_buf, const char *message_type)
{
  outgoing_buf.push_int(CONN_DATA_MESSAGE);
  outgoing_buf.push_string(message_type);
}

void PORT::prepare_call(Text_Buf& outgoing_buf, const char *signature_name)
{
  outgoing_buf.push_int(CONN_DATA_CALL);
  outgoing_buf.push_string(signature_name);
}

void PORT::prepare_reply(Text_Buf& outgoing_buf, const char *signature_name)
{
  outgoing_buf.push_int(CONN_DATA_REPLY);
  outgoing_buf.push_string(signature_name);
}

void PORT::prepare_exception(Text_Buf& outgoing_buf, const char *signature_name)
{
  outgoing_buf.push_int(CONN_DATA_EXCEPTION);
  outgoing_buf.push_string(signature_name);
}

void PORT::send_data(Text_Buf &outgoing_buf,
  const COMPONENT& destination_component)
{
  if (!destination_component.is_bound()) TTCN_error("Internal error: "
    "The destination component reference is unbound when sending data on "
    "port %s.", port_name);
  component destination_compref = (component)destination_component;
  boolean is_unique;
  port_connection *conn_ptr =
    lookup_connection_to_compref(destination_compref, &is_unique);
  if (conn_ptr == NULL)
    TTCN_error("Data cannot be sent on port %s to component %d "
      "because there is no connection towards component %d.", port_name,
      destination_compref, destination_compref);
  else if (!is_unique)
    TTCN_error("Data cannot be sent on port %s to component %d "
      "because there are more than one connections towards component "
      "%d.", port_name, destination_compref, destination_compref);
  else if (conn_ptr->connection_state != CONN_CONNECTED)
    TTCN_error("Data cannot be sent on port %s to component %d "
      "because the connection is not in active state.",
      port_name, destination_compref);
  switch (conn_ptr->transport_type) {
  case TRANSPORT_LOCAL:
    send_data_local(conn_ptr, outgoing_buf);
    break;
  case TRANSPORT_INET_STREAM:
  case TRANSPORT_UNIX_STREAM:
    send_data_stream(conn_ptr, outgoing_buf, FALSE);
    break;
  default:
    TTCN_error("Internal error: Invalid transport type (%d) in port "
      "connection between %s and %d:%s.", conn_ptr->transport_type,
      port_name, conn_ptr->remote_component, conn_ptr->remote_port);
  }
}

void PORT::process_data(port_connection *conn_ptr, Text_Buf& incoming_buf)
{
  connection_data_type_enum conn_data_type =
    (connection_data_type_enum)incoming_buf.pull_int().get_val();
  if (conn_data_type != CONN_DATA_LAST) {
    switch (conn_ptr->connection_state) {
    case CONN_CONNECTED:
    case CONN_LAST_MSG_SENT:
      break;
    case CONN_LAST_MSG_RCVD:
    case CONN_IDLE:
      TTCN_warning("Data arrived after the indication of connection "
        "termination on port %s from %d:%s. Data is ignored.",
        port_name, conn_ptr->remote_component, conn_ptr->remote_port);
      return;
    default:
      TTCN_error("Internal error: Connection of port %s with %d:%s has "
        "invalid state (%d).", port_name, conn_ptr->remote_component,
        conn_ptr->remote_port, conn_ptr->connection_state);
    }
    char *message_type = incoming_buf.pull_string();
    try {
      switch (conn_data_type) {
      case CONN_DATA_MESSAGE:
        if (!process_message(message_type, incoming_buf,
          conn_ptr->remote_component, conn_ptr->sliding_buffer)) {
          TTCN_error("Port %s does not support incoming message "
            "type %s, which has arrived on the connection from "
            "%d:%s.", port_name, message_type,
            conn_ptr->remote_component, conn_ptr->remote_port);
        }
        break;
      case CONN_DATA_CALL:
        if (!process_call(message_type, incoming_buf,
          conn_ptr->remote_component)) {
          TTCN_error("Port %s does not support incoming call of "
            "signature %s, which has arrived on the connection "
            "from %d:%s.", port_name, message_type,
            conn_ptr->remote_component, conn_ptr->remote_port);
        }
        break;
      case CONN_DATA_REPLY:
        if (!process_reply(message_type, incoming_buf,
          conn_ptr->remote_component)) {
          TTCN_error("Port %s does not support incoming reply of "
            "signature %s, which has arrived on the connection "
            "from %d:%s.", port_name, message_type,
            conn_ptr->remote_component, conn_ptr->remote_port);
        }
        break;
      case CONN_DATA_EXCEPTION:
        if (!process_exception(message_type, incoming_buf,
          conn_ptr->remote_component)) {
          TTCN_error("Port %s does not support incoming exception "
            "of signature %s, which has arrived on the connection "
            "from %d:%s.", port_name, message_type,
            conn_ptr->remote_component, conn_ptr->remote_port);
        }
        break;
      default:
        TTCN_error("Internal error: Data with invalid selector (%d) "
          "was received on port %s from %d:%s.", conn_data_type,
          port_name, conn_ptr->remote_component,
          conn_ptr->remote_port);
      }
    } catch (...) {
      // avoid memory leak
      delete [] message_type;
      throw;
    }
    delete [] message_type;
  } else process_last_message(conn_ptr);
}

boolean PORT::process_message(const char *, Text_Buf&, component, OCTETSTRING&)
{
  return FALSE;
}

boolean PORT::process_call(const char *, Text_Buf&, component)
{
  return FALSE;
}

boolean PORT::process_reply(const char *, Text_Buf&, component)
{
  return FALSE;
}

boolean PORT::process_exception(const char *, Text_Buf&, component)
{
  return FALSE;
}

port_connection *PORT::add_connection(component remote_component,
  const char *remote_port, transport_type_enum transport_type)
{
  port_connection *conn_ptr;
  for (conn_ptr = connection_list_head; conn_ptr != NULL;
    conn_ptr = conn_ptr->list_next) {
    if (conn_ptr->remote_component == remote_component) {
      int ret_val = strcmp(conn_ptr->remote_port, remote_port);
      if (ret_val == 0) return conn_ptr;
      else if (ret_val > 0) break;
    } else if (conn_ptr->remote_component > remote_component) break;
  }
  
  if (n_system_mappings > 0) {
    TTCN_error("Connect operation cannot be performed on a mapped port (%s).", port_name);
  }
  
  port_connection *new_conn = new port_connection;
  new_conn->owner_port = this;

  new_conn->connection_state = CONN_IDLE;
  new_conn->remote_component = remote_component;
  new_conn->remote_port = mcopystr(remote_port);
  new_conn->transport_type = transport_type;
  new_conn->sliding_buffer = OCTETSTRING(0, 0);
  switch (transport_type) {
  case TRANSPORT_LOCAL:
    new_conn->local.port_ptr = NULL;
    break;
  case TRANSPORT_INET_STREAM:
  case TRANSPORT_UNIX_STREAM:
    new_conn->stream.comm_fd = -1;
    new_conn->stream.incoming_buf = NULL;
    break;
  default:
    delete new_conn;
    TTCN_error("Internal error: PORT::add_connection(): invalid transport "
      "type (%d).", transport_type);
  }

  new_conn->list_next = conn_ptr;
  if (conn_ptr != NULL) {
    // new_conn will be inserted before conn_ptr in the ordered list
    new_conn->list_prev = conn_ptr->list_prev;
    conn_ptr->list_prev = new_conn;
    if (new_conn->list_prev != NULL)
      new_conn->list_prev->list_next = new_conn;
  } else {
    // new_conn will be inserted to the end of the list
    new_conn->list_prev = connection_list_tail;
    if (connection_list_tail != NULL)
      connection_list_tail->list_next = new_conn;
    connection_list_tail = new_conn;
  }
  if (conn_ptr == connection_list_head) connection_list_head = new_conn;

  return new_conn;
}

void PORT::remove_connection(port_connection *conn_ptr)
{
  Free(conn_ptr->remote_port);

  switch (conn_ptr->transport_type) {
  case TRANSPORT_LOCAL:
    break;
  case TRANSPORT_INET_STREAM:
  case TRANSPORT_UNIX_STREAM:
    if (conn_ptr->stream.comm_fd >= 0) {
      Fd_And_Timeout_User::remove_fd(conn_ptr->stream.comm_fd, conn_ptr,
        FD_EVENT_RD);
      if (conn_ptr->connection_state == CONN_LISTENING &&
        conn_ptr->transport_type == TRANSPORT_UNIX_STREAM)
        unlink_unix_pathname(conn_ptr->stream.comm_fd);
      close(conn_ptr->stream.comm_fd);
      conn_ptr->stream.comm_fd = -1;
    }
    delete conn_ptr->stream.incoming_buf;
    break;
  default:
    TTCN_error("Internal error: PORT::remove_connection(): invalid "
      "transport type.");
  }

  if (conn_ptr->list_prev != NULL)
    conn_ptr->list_prev->list_next = conn_ptr->list_next;
  else if (connection_list_head == conn_ptr)
    connection_list_head = conn_ptr->list_next;
  if (conn_ptr->list_next != NULL)
    conn_ptr->list_next->list_prev = conn_ptr->list_prev;
  else if (connection_list_tail == conn_ptr)
    connection_list_tail = conn_ptr->list_prev;

  delete conn_ptr;
}

port_connection *PORT::lookup_connection_to_compref(
  component remote_component, boolean *is_unique)
{
  for (port_connection *conn = connection_list_head; conn != NULL;
    conn = conn->list_next) {
    if (conn->remote_component == remote_component) {
      if (is_unique != NULL) {
        port_connection *nxt = conn->list_next;
        if (nxt != NULL && nxt->remote_component == remote_component)
          *is_unique = FALSE;
        else *is_unique = TRUE;
      }
      return conn;
    } else if (conn->remote_component > remote_component) break;
  }
  return NULL;
}

port_connection *PORT::lookup_connection(component remote_component,
  const char *remote_port)
{
  for (port_connection *conn = connection_list_head; conn != NULL;
    conn = conn->list_next) {
    if (conn->remote_component == remote_component) {
      int ret_val = strcmp(conn->remote_port, remote_port);
      if (ret_val == 0) return conn;
      else if (ret_val > 0) break;
    } else if (conn->remote_component > remote_component) break;
  }
  return NULL;
}

void PORT::add_local_connection(PORT *other_endpoint)
{
  port_connection *conn_ptr = add_connection(self, other_endpoint->port_name,
    TRANSPORT_LOCAL);
  conn_ptr->connection_state = CONN_CONNECTED;
  conn_ptr->local.port_ptr = other_endpoint;
  TTCN_Logger::log_port_misc(TitanLoggerApi::Port__Misc_reason::local__connection__established,
    port_name, NULL_COMPREF, other_endpoint->port_name);
}

void PORT::remove_local_connection(port_connection *conn_ptr)
{
  if (conn_ptr->transport_type != TRANSPORT_LOCAL)
    TTCN_error("Internal error: The transport type used by the connection "
      "between port %s and %d:%s is not LOCAL.", port_name,
      conn_ptr->remote_component, conn_ptr->remote_port);
  PORT *other_endpoint = conn_ptr->local.port_ptr;
  remove_connection(conn_ptr);
  TTCN_Logger::log_port_misc(TitanLoggerApi::Port__Misc_reason::local__connection__terminated,
    port_name, NULL_COMPREF, other_endpoint->port_name);
}

unsigned int PORT::get_connection_hash(component local_component,
  const char *local_port, component remote_component, const char *remote_port)
{
  const size_t N = sizeof(unsigned int);
  unsigned char hash_buffer[N];

  // fill the buffer with an initial pattern
  for (size_t i = 0; i < N; i++) hash_buffer[i] = i % 2 ? 0x55 : 0xAA;

  // add the PID of the current process to the buffer
  pid_t pid = getpid();
  for (size_t i = 0; i < sizeof(pid); i++)
    hash_buffer[i % N] ^= (pid >> (8 * i)) & 0xFF;

  // add the local and remote component reference and port name to the buffer
  for (size_t i = 0; i < sizeof(local_component); i++)
    hash_buffer[(N - 1) - i % N] ^= (local_component >> (8 * i)) & 0xFF;
  for (size_t i = 0; local_port[i] != '\0'; i++)
    hash_buffer[(N - 1) - i % N] ^= local_port[i];
  for (size_t i = 0; i < sizeof(remote_component); i++)
    hash_buffer[i % N] ^= (remote_component >> (8 * i)) & 0xFF;
  for (size_t i = 0; remote_port[i] != '\0'; i++)
    hash_buffer[i % N] ^= remote_port[i];

  // convert the buffer to an integer value
  unsigned int ret_val = 0;
  for (size_t i = 0; i < N; i++)
    ret_val = (ret_val << 8) | hash_buffer[i];
  return ret_val;
}

void PORT::unlink_unix_pathname(int socket_fd)
{
  struct sockaddr_un local_addr;
  // querying the local pathname used by socket_fd
  socklen_type addr_len = sizeof(local_addr);
  if (getsockname(socket_fd, (struct sockaddr*)&local_addr, &addr_len)) {
    TTCN_warning_begin("System call getsockname() failed on UNIX socket "
      "file descriptor %d.", socket_fd);
    TTCN_Logger::OS_error();
    TTCN_Logger::log_event_str(" The associated socket file will not be "
      "removed from the file system.");
    TTCN_warning_end();
  } else if (local_addr.sun_family != AF_UNIX) {
    TTCN_warning("System call getsockname() returned invalid address "
      "family for UNIX socket file descriptor %d. The associated socket "
      "file will not be removed from the file system.", socket_fd);
  } else if (unlink(local_addr.sun_path)) {
    if (errno != ENOENT) {
      TTCN_warning_begin("System call unlink() failed when trying to "
        "remove UNIX socket file %s.", local_addr.sun_path);
      TTCN_Logger::OS_error();
      TTCN_Logger::log_event_str(" The file will remain in the file "
        "system.");
      TTCN_warning_end();
    } else errno = 0;
  }
}

void PORT::connect_listen_inet_stream(component remote_component,
  const char *remote_port)
{
  // creating the TCP server socket
  int server_fd = NetworkHandler::socket(TTCN_Communication::get_network_family());
  if (server_fd < 0) {
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Creation of the TCP server socket failed. (%s)",
      strerror(errno));
    errno = 0;
    return;
  }

  // binding the socket to an ephemeral TCP port
  // using the same local IP address as the control connection to MC
  IPAddress *local_addr = IPAddress::create_addr(TTCN_Communication::get_network_family());
  *local_addr = *TTCN_Communication::get_local_address();
  local_addr->set_port(0);
  if (bind(server_fd, local_addr->get_addr(), local_addr->get_addr_len())) {
    close(server_fd);
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Binding of server socket to an ephemeral TCP port "
      "failed. (%s)", strerror(errno));
    errno = 0;
    delete local_addr;
    return;
  }

  // zero backlog is enough since we are waiting for only one client
  if (listen(server_fd, 0)) {
    close(server_fd);
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Listening on an ephemeral TCP port failed. (%s)",
      strerror(errno));
    errno = 0;
    delete local_addr;
    return;
  }

  // querying the IP address and port number used by the TCP server
  if (local_addr->getsockname(server_fd)) {
    close(server_fd);
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "System call getsockname() failed on the TCP server "
      "socket. (%s)", strerror(errno));
    errno = 0;
    delete local_addr;
    return;
  }

  if (!TTCN_Communication::set_close_on_exec(server_fd)) {
    close(server_fd);
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Setting the close-on-exec flag failed on the TCP "
      "server socket.");
    delete local_addr;
    return;
  }

  port_connection *new_connection = add_connection(remote_component,
    remote_port, TRANSPORT_INET_STREAM);
  new_connection->connection_state = CONN_LISTENING;
  new_connection->stream.comm_fd = server_fd;
  Fd_And_Timeout_User::add_fd(server_fd, new_connection, FD_EVENT_RD);

  TTCN_Communication::send_connect_listen_ack_inet_stream(port_name,
    remote_component, remote_port, local_addr);

  TTCN_Logger::log_port_misc(
    TitanLoggerApi::Port__Misc_reason::port__is__waiting__for__connection__tcp,
    port_name, remote_component, remote_port);
  delete local_addr;
}

void PORT::connect_listen_unix_stream(component remote_component,
  const char *remote_port)
{
  // creating the UNIX server socket
  int server_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (server_fd < 0) {
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Creation of the UNIX server socket failed. (%s)",
      strerror(errno));
    errno = 0;
    return;
  }

  // binding the socket to a temporary file
  struct sockaddr_un local_addr;
  // the file name is constructed using a hash function
  unsigned int hash_value =
    get_connection_hash(self, port_name, remote_component, remote_port);
  for (unsigned int i = 1; ; i++) {
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sun_family = AF_UNIX;
    snprintf(local_addr.sun_path, sizeof(local_addr.sun_path),
      "/tmp/ttcn3-portconn-%x", hash_value);
    if (bind(server_fd, (struct sockaddr*)&local_addr, sizeof(local_addr))
      == 0) {
      // the operation was successful, jump out of the loop
      break;
    } else if (errno == EADDRINUSE) {
      // the temporary file name is already used by someone else
      errno = 0;
      if (i < UNIX_BIND_MAX_ITER) {
        // try another hash value
        hash_value++;
      } else {
        close(server_fd);
        TTCN_Communication::send_connect_error(port_name,
          remote_component, remote_port, "Could not find a free "
          "pathname to bind the UNIX server socket to after %u "
          "iterations.", i);
        errno = 0;
        return;
      }
    } else {
      close(server_fd);
      TTCN_Communication::send_connect_error(port_name, remote_component,
        remote_port, "Binding of UNIX server socket to pathname %s "
        "failed. (%s)", local_addr.sun_path, strerror(errno));
      errno = 0;
      return;
    }
  }

  // zero backlog is enough since we are waiting for only one client
  if (listen(server_fd, 0)) {
    close(server_fd);
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Listening on UNIX pathname %s failed. (%s)",
      local_addr.sun_path, strerror(errno));
    errno = 0;
    return;
  }

  if (!TTCN_Communication::set_close_on_exec(server_fd)) {
    close(server_fd);
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Setting the close-on-exec flag failed on the UNIX "
      "server socket.");
    return;
  }

  port_connection *new_connection = add_connection(remote_component,
    remote_port, TRANSPORT_UNIX_STREAM);
  new_connection->connection_state = CONN_LISTENING;
  new_connection->stream.comm_fd = server_fd;
  Fd_And_Timeout_User::add_fd(server_fd, new_connection, FD_EVENT_RD);

  TTCN_Communication::send_connect_listen_ack_unix_stream(port_name,
    remote_component, remote_port, &local_addr);

  TTCN_Logger::log_port_misc(
    TitanLoggerApi::Port__Misc_reason::port__is__waiting__for__connection__unix,
    port_name, remote_component, remote_port, local_addr.sun_path);
}

void PORT::connect_local(component remote_component, const char *remote_port)
{
  if (self != remote_component) {
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Message CONNECT with transport type LOCAL refers "
      "to a port of another component (%d).", remote_component);
    return;
  }
  PORT *remote_ptr = lookup_by_name(remote_port);
  if (remote_ptr == NULL) {
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Port %s does not exist.", remote_port);
    return;
  } else if (!remote_ptr->is_active) TTCN_error("Internal error: Port %s is "
    "inactive when trying to connect it to local port %s.", remote_port,
    port_name);
  add_local_connection(remote_ptr);
  if (this != remote_ptr) remote_ptr->add_local_connection(this);
  TTCN_Communication::send_connected(port_name, remote_component,
    remote_port);
}

void PORT::connect_stream(component remote_component, const char *remote_port,
  transport_type_enum transport_type, Text_Buf& text_buf)
{
#ifdef WIN32
  again:
#endif
  const char *transport_str;
  int client_fd;
  switch (transport_type) {
  case TRANSPORT_INET_STREAM:
    transport_str = "TCP";
    // creating the TCP client socket
    client_fd = NetworkHandler::socket(TTCN_Communication::get_network_family());
    break;
  case TRANSPORT_UNIX_STREAM:
    transport_str = "UNIX";
    // creating the UNIX client socket
    client_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    break;
  default:
    TTCN_error("Internal error: PORT::connect_stream(): invalid transport "
      "type (%d).", transport_type);
  }
  if (client_fd < 0) {
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Creation of the %s client socket failed. (%s)",
      transport_str, strerror(errno));
    errno = 0;
    return;
  }

  switch (transport_type) {
  case TRANSPORT_INET_STREAM: {
    // connect to the IP address and port number given in message CONNECT
    IPAddress *remote_addr = IPAddress::create_addr(TTCN_Communication::get_network_family());
    remote_addr->pull_raw(text_buf);
#ifdef SOLARIS
    if (connect(client_fd, (struct sockaddr*)remote_addr->get_addr(), remote_addr->get_addr_len())) {
#else
    if (connect(client_fd, remote_addr->get_addr(), remote_addr->get_addr_len())) {
#endif
#ifdef WIN32
      if (errno == EADDRINUSE) {
        close(client_fd);
        errno = 0;
        TTCN_warning("connect() returned error code EADDRINUSE. "
          "Perhaps this is a Cygwin bug. Trying to connect again.");
        goto again;
      }
#endif
      close(client_fd);
      TTCN_Communication::send_connect_error(port_name, remote_component,
        remote_port, "TCP connection establishment failed to %s:%d. (%s)",
        remote_addr->get_addr_str(), remote_addr->get_port(), strerror(errno));
      errno = 0;
      delete remote_addr;
      return;
    }
    delete remote_addr;
    break; }
  case TRANSPORT_UNIX_STREAM: {
    // connect to the UNIX pathname given in the message CONNECT
    struct sockaddr_un remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sun_family = AF_UNIX;
    size_t pathname_len = text_buf.pull_int().get_val();
    if (pathname_len >= sizeof(remote_addr.sun_path)) {
      close(client_fd);
      TTCN_Communication::send_connect_error(port_name, remote_component,
        remote_port, "The UNIX pathname used by the server socket is "
        "too long. It consists of %lu bytes although it should be "
        "shorter than %lu bytes to fit in the appropriate structure.",
        (unsigned long) pathname_len,
        (unsigned long) sizeof(remote_addr.sun_path));
      return;
    }
    text_buf.pull_raw(pathname_len, remote_addr.sun_path);
    if (connect(client_fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr))) {
      close(client_fd);
      TTCN_Communication::send_connect_error(port_name, remote_component,
        remote_port, "UNIX socket connection establishment failed to "
        "pathname %s. (%s)", remote_addr.sun_path, strerror(errno));
      errno = 0;
      return;
    }
    break; }
  default:
    TTCN_error("Internal error: PORT::connect_stream(): invalid transport "
      "type (%d).", transport_type);
  }

  if (!TTCN_Communication::set_close_on_exec(client_fd)) {
    close(client_fd);
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Setting the close-on-exec flag failed on the %s "
      "client socket.", transport_str);
    return;
  }

  if (!TTCN_Communication::set_non_blocking_mode(client_fd, TRUE)) {
    close(client_fd);
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Setting the non-blocking mode failed on the %s "
      "client socket.", transport_str);
    return;
  }

  if (transport_type == TRANSPORT_INET_STREAM &&
    !TTCN_Communication::set_tcp_nodelay(client_fd)) {
    close(client_fd);
    TTCN_Communication::send_connect_error(port_name, remote_component,
      remote_port, "Setting the TCP_NODELAY flag failed on the TCP "
      "client socket.");
    return;
  }

  port_connection *new_connection = add_connection(remote_component,
    remote_port, transport_type);
  new_connection->connection_state = CONN_CONNECTED;
  new_connection->stream.comm_fd = client_fd;
  Fd_And_Timeout_User::add_fd(client_fd, new_connection, FD_EVENT_RD);

  TTCN_Logger::log_port_misc(TitanLoggerApi::Port__Misc_reason::connection__established,
    port_name, remote_component, remote_port, transport_str);
}

void PORT::disconnect_local(port_connection *conn_ptr)
{
  PORT *remote_ptr = conn_ptr->local.port_ptr;
  remove_local_connection(conn_ptr);
  if (this != remote_ptr) {
    port_connection *conn2_ptr =
      remote_ptr->lookup_connection(self, port_name);
    if (conn2_ptr == NULL) TTCN_error("Internal error: Port %s is "
      "connected with local port %s, but port %s does not have a "
      "connection to %s.", port_name, remote_ptr->port_name,
      remote_ptr->port_name, port_name);
    else remote_ptr->remove_local_connection(conn2_ptr);
  }
  TTCN_Communication::send_disconnected(port_name, self,
    remote_ptr->port_name);
}

void PORT::disconnect_stream(port_connection *conn_ptr)
{
  switch (conn_ptr->connection_state) {
  case CONN_LISTENING:
    TTCN_Logger::log_port_misc(
      TitanLoggerApi::Port__Misc_reason::destroying__unestablished__connection,
      port_name, conn_ptr->remote_component, conn_ptr->remote_port);
    remove_connection(conn_ptr);
    // do not send back any acknowledgment
    break;
  case CONN_CONNECTED: {
    TTCN_Logger::log_port_misc(TitanLoggerApi::Port__Misc_reason::terminating__connection,
      port_name, conn_ptr->remote_component, conn_ptr->remote_port);
    Text_Buf outgoing_buf;
    outgoing_buf.push_int(CONN_DATA_LAST);
    if (send_data_stream(conn_ptr, outgoing_buf, TRUE)) {
      // sending the last message was successful
      // wait for the acknowledgment from the peer
      conn_ptr->connection_state = CONN_LAST_MSG_SENT;
    } else {
      TTCN_Logger::log_port_misc(
        TitanLoggerApi::Port__Misc_reason::sending__termination__request__failed,
        port_name, conn_ptr->remote_component, conn_ptr->remote_port);
      // send an acknowledgment to MC immediately to avoid deadlock
      // in case of communication failure
      // (i.e. when the peer does not send DISCONNECTED)
      TTCN_Communication::send_disconnected(port_name,
        conn_ptr->remote_component, conn_ptr->remote_port);
      TTCN_warning("The last outgoing messages on port %s may be lost.",
        port_name);
      // destroy the connection immediately
      remove_connection(conn_ptr);
    }
    break; }
  default:
    TTCN_error("The connection of port %s to %d:%s is in unexpected "
      "state when trying to terminate it.", port_name,
      conn_ptr->remote_component, conn_ptr->remote_port);
  }
}

void PORT::send_data_local(port_connection *conn_ptr, Text_Buf& outgoing_data)
{
  outgoing_data.rewind();
  PORT *dest_ptr = conn_ptr->local.port_ptr;
  if (this != dest_ptr) {
    port_connection *conn2_ptr =
      dest_ptr->lookup_connection(self, port_name);
    if (conn2_ptr == NULL) TTCN_error("Internal error: Port %s is "
      "connected with local port %s, but port %s does not have a "
      "connection to %s.", port_name, dest_ptr->port_name,
      dest_ptr->port_name, port_name);
    dest_ptr->process_data(conn2_ptr, outgoing_data);
  } else process_data(conn_ptr, outgoing_data);
}

boolean PORT::send_data_stream(port_connection *conn_ptr,
  Text_Buf& outgoing_data, boolean ignore_peer_disconnect)
{
  boolean would_block_warning=FALSE;
  outgoing_data.calculate_length();
  const char *msg_ptr = outgoing_data.get_data();
  size_t msg_len = outgoing_data.get_len(), sent_len = 0;
  while (sent_len < msg_len) {
    int ret_val = send(conn_ptr->stream.comm_fd, msg_ptr + sent_len,
      msg_len - sent_len, 0);
    if (ret_val > 0) sent_len += ret_val;
    else {
      switch (errno) {
      case EINTR:
        // a signal occurred: do nothing, just try again
        errno = 0;
        break;
      case EAGAIN: {
        // the output buffer is full: try to increase it if possible
        errno = 0;
        int old_bufsize, new_bufsize;
        if (TTCN_Communication::increase_send_buffer(
          conn_ptr->stream.comm_fd, old_bufsize, new_bufsize)) {
          TTCN_Logger::log_port_misc(
            TitanLoggerApi::Port__Misc_reason::sending__would__block,
            port_name, conn_ptr->remote_component, conn_ptr->remote_port,
            NULL, old_bufsize, new_bufsize);
        } else {
          if(!would_block_warning){
            TTCN_warning_begin("Sending data on the connection of "
              "port %s to ", port_name);
            COMPONENT::log_component_reference(
              conn_ptr->remote_component);
            TTCN_Logger::log_event(":%s would block execution and it "
              "is not possible to further increase the size of the "
              "outgoing buffer. Trying to process incoming data to "
              "avoid deadlock.", conn_ptr->remote_port);
            TTCN_warning_end();
            would_block_warning=TRUE;
          }
          TTCN_Snapshot::block_for_sending(conn_ptr->stream.comm_fd);
        }
        break; }
      case ECONNRESET:
      case EPIPE:
        if (ignore_peer_disconnect) return FALSE;
        // no break
      default:
        TTCN_error("Sending data on the connection of port %s to "
          "%d:%s failed.", port_name, conn_ptr->remote_component,
          conn_ptr->remote_port);
      }
    }
  }
  if(would_block_warning){
    TTCN_warning_begin("The message finally was sent on "
      "port %s to ", port_name);
    COMPONENT::log_component_reference(
      conn_ptr->remote_component);
    TTCN_Logger::log_event(":%s.", conn_ptr->remote_port);
    TTCN_warning_end();
  }
  return TRUE;
}

void PORT::handle_incoming_connection(port_connection *conn_ptr)
{
  const char *transport_str =
    conn_ptr->transport_type == TRANSPORT_INET_STREAM ? "TCP" : "UNIX";
  int comm_fd = accept(conn_ptr->stream.comm_fd, NULL, NULL);
  if (comm_fd < 0) {
    TTCN_Communication::send_connect_error(port_name,
      conn_ptr->remote_component, conn_ptr->remote_port,
      "Accepting of incoming %s connection failed. (%s)", transport_str,
      strerror(errno));
    errno = 0;
    remove_connection(conn_ptr);
    return;
  }

  if (!TTCN_Communication::set_close_on_exec(comm_fd)) {
    close(comm_fd);
    TTCN_Communication::send_connect_error(port_name,
      conn_ptr->remote_component, conn_ptr->remote_port,
      "Setting the close-on-exec flag failed on the server-side %s "
      "socket.", transport_str);
    remove_connection(conn_ptr);
    return;
  }

  if (!TTCN_Communication::set_non_blocking_mode(comm_fd, TRUE)) {
    close(comm_fd);
    TTCN_Communication::send_connect_error(port_name,
      conn_ptr->remote_component, conn_ptr->remote_port,
      "Setting the non-blocking mode failed on the server-side %s "
      "socket.", transport_str);
    remove_connection(conn_ptr);
    return;
  }

  if (conn_ptr->transport_type == TRANSPORT_INET_STREAM &&
    !TTCN_Communication::set_tcp_nodelay(comm_fd)) {
    close(comm_fd);
    TTCN_Communication::send_connect_error(port_name,
      conn_ptr->remote_component, conn_ptr->remote_port,
      "Setting the TCP_NODELAY flag failed on the server-side TCP "
      "socket.");
    remove_connection(conn_ptr);
    return;
  }

  // shutting down the server socket and replacing it with the
  // communication socket of the new connection
  Fd_And_Timeout_User::remove_fd(conn_ptr->stream.comm_fd, conn_ptr,
    FD_EVENT_RD);
  if (conn_ptr->transport_type == TRANSPORT_UNIX_STREAM)
    unlink_unix_pathname(conn_ptr->stream.comm_fd);
  close(conn_ptr->stream.comm_fd);
  conn_ptr->connection_state = CONN_CONNECTED;
  conn_ptr->stream.comm_fd = comm_fd;
  Fd_And_Timeout_User::add_fd(comm_fd, conn_ptr, FD_EVENT_RD);

  TTCN_Communication::send_connected(port_name, conn_ptr->remote_component,
    conn_ptr->remote_port);

  TTCN_Logger::log_port_misc(TitanLoggerApi::Port__Misc_reason::connection__accepted,
    port_name, conn_ptr->remote_component, conn_ptr->remote_port);
}

void PORT::handle_incoming_data(port_connection *conn_ptr)
{
  if (conn_ptr->stream.incoming_buf == NULL)
    conn_ptr->stream.incoming_buf = new Text_Buf;
  Text_Buf& incoming_buf = *conn_ptr->stream.incoming_buf;
  char *buf_ptr;
  int buf_len;
  incoming_buf.get_end(buf_ptr, buf_len);
  int recv_len = recv(conn_ptr->stream.comm_fd, buf_ptr, buf_len, 0);
  if (recv_len < 0) {
    // an error occurred
    if (errno == ECONNRESET) {
      // TCP connection was reset by peer
      errno = 0;
      TTCN_Communication::send_disconnected(port_name,
        conn_ptr->remote_component, conn_ptr->remote_port);
      TTCN_Logger::log_port_misc(
        TitanLoggerApi::Port__Misc_reason::connection__reset__by__peer,
        port_name, conn_ptr->remote_component, conn_ptr->remote_port);
      TTCN_warning("The last outgoing messages on port %s may be lost.",
        port_name);
      conn_ptr->connection_state = CONN_IDLE;
    } else {
      TTCN_error("Receiving data on the connection of port %s from "
        "%d:%s failed.", port_name, conn_ptr->remote_component,
        conn_ptr->remote_port);
    }
  } else if (recv_len > 0) {
    // data was received
    incoming_buf.increase_length(recv_len);
    // processing all messages in the buffer after each other
    while (incoming_buf.is_message()) {
      incoming_buf.pull_int(); // message_length
      process_data(conn_ptr, incoming_buf);
      incoming_buf.cut_message();
    }
  } else {
    // the connection was closed by the peer
    TTCN_Communication::send_disconnected(port_name,
      conn_ptr->remote_component, conn_ptr->remote_port);
    if (conn_ptr->connection_state != CONN_LAST_MSG_RCVD) {
      TTCN_Logger::log_port_misc(
        TitanLoggerApi::Port__Misc_reason::connection__closed__by__peer,
        port_name, conn_ptr->remote_component, conn_ptr->remote_port);
    }
    // the connection can be removed
    conn_ptr->connection_state = CONN_IDLE;
  }
  if (conn_ptr->connection_state == CONN_IDLE) {
    // terminating and removing connection
    int msg_len = incoming_buf.get_len();
    if (msg_len > 0) {
      TTCN_warning_begin("Message fragment remained in the buffer of "
        "port connection between %s and ", port_name);
      COMPONENT::log_component_reference(conn_ptr->remote_component);
      TTCN_Logger::log_event(":%s: ", conn_ptr->remote_port);
      const unsigned char *msg_ptr =
        (const unsigned char*)incoming_buf.get_data();
      for (int i = 0; i < msg_len; i++)
        TTCN_Logger::log_octet(msg_ptr[i]);
      TTCN_warning_end();
    }

    TTCN_Logger::log_port_misc(TitanLoggerApi::Port__Misc_reason::port__disconnected,
      port_name, conn_ptr->remote_component, conn_ptr->remote_port);
    remove_connection(conn_ptr);
  }
}

void PORT::process_last_message(port_connection *conn_ptr)
{
  switch (conn_ptr->transport_type) {
  case TRANSPORT_INET_STREAM:
  case TRANSPORT_UNIX_STREAM:
    break;
  default:
    TTCN_error("Internal error: Connection termination request was "
      "received on the connection of port %s with %d:%s, which has an "
      "invalid transport type (%d).", port_name,
      conn_ptr->remote_component, conn_ptr->remote_port,
      conn_ptr->transport_type);
  }
  switch (conn_ptr->connection_state) {
  case CONN_CONNECTED: {
    TTCN_Logger::log_port_misc(
      TitanLoggerApi::Port__Misc_reason::termination__request__received,
      port_name, conn_ptr->remote_component, conn_ptr->remote_port);
    Text_Buf outgoing_buf;
    outgoing_buf.push_int(CONN_DATA_LAST);
    if (send_data_stream(conn_ptr, outgoing_buf, TRUE)) {
      // sending the last message was successful
      // wait until the peer closes the transport connection
      conn_ptr->connection_state = CONN_LAST_MSG_RCVD;
    } else {
      TTCN_Logger::log_port_misc(
        TitanLoggerApi::Port__Misc_reason::acknowledging__termination__request__failed,
        port_name, conn_ptr->remote_component, conn_ptr->remote_port);
      // send an acknowledgment to MC immediately to avoid deadlock
      // in case of communication failure
      // (i.e. when the peer does not send DISCONNECTED)
      TTCN_Communication::send_disconnected(port_name,
        conn_ptr->remote_component, conn_ptr->remote_port);
      // the connection can be removed immediately
      TTCN_warning("The last outgoing messages on port %s may be lost.",
        port_name);
      conn_ptr->connection_state = CONN_IDLE;
    }
    break; }
  case CONN_LAST_MSG_SENT:
    // the connection can be removed
    conn_ptr->connection_state = CONN_IDLE;
    break;
  case CONN_LAST_MSG_RCVD:
  case CONN_IDLE:
    TTCN_warning("Unexpected data arrived after the indication of "
      "connection termination on port %s from %d:%s.", port_name,
      conn_ptr->remote_component, conn_ptr->remote_port);
    break;
  default:
    TTCN_error("Internal error: Connection of port %s with %d:%s has "
      "invalid state (%d).", port_name, conn_ptr->remote_component,
      conn_ptr->remote_port, conn_ptr->connection_state);
  }
}

void PORT::map(const char *system_port)
{
  if (!is_active) TTCN_error("Inactive port %s cannot be mapped.", port_name);

  int new_posn;
  for (new_posn = 0; new_posn < n_system_mappings; new_posn++) {
    int str_diff = strcmp(system_port, system_mappings[new_posn]);
    if (str_diff < 0) break;
    else if (str_diff == 0) {
      TTCN_warning("Port %s is already mapped to system:%s."
        " Map operation was ignored.", port_name, system_port);
      return;
    }
  }

  set_system_parameters(system_port);

  user_map(system_port);

  TTCN_Logger::log_port_misc(
    TitanLoggerApi::Port__Misc_reason::port__was__mapped__to__system,
    port_name, SYSTEM_COMPREF, system_port);

  // the mapping shall be registered in the table only if user_map() was
  // successful
  system_mappings = (char**)Realloc(system_mappings,
    (n_system_mappings + 1) * sizeof(*system_mappings));
  memmove(system_mappings + new_posn + 1, system_mappings + new_posn,
    (n_system_mappings - new_posn) * sizeof(*system_mappings));
  system_mappings[new_posn] = mcopystr(system_port);
  n_system_mappings++;

  if (n_system_mappings > 1) TTCN_warning("Port %s has now more than one "
    "mappings. Message cannot be sent on it to system even with explicit "
    "addressing.", port_name);
}

void PORT::unmap(const char *system_port)
{
  int del_posn;
  for (del_posn = 0; del_posn < n_system_mappings; del_posn++) {
    int str_diff = strcmp(system_port, system_mappings[del_posn]);
    if (str_diff == 0) break;
    else if (str_diff < 0) {
      del_posn = n_system_mappings;
      break;
    }
  }
  if (del_posn >= n_system_mappings) {
    TTCN_warning("Port %s is not mapped to system:%s. "
      "Unmap operation was ignored.", port_name, system_port);
    return;
  }

  char *unmapped_port = system_mappings[del_posn];

  // first remove the mapping from the table
  n_system_mappings--;
  memmove(system_mappings + del_posn, system_mappings + del_posn + 1,
    (n_system_mappings - del_posn) * sizeof(*system_mappings));
  system_mappings = (char**)Realloc(system_mappings,
    n_system_mappings * sizeof(*system_mappings));

  try {
    user_unmap(system_port);
  } catch (...) {
    // prevent from memory leak
    Free(unmapped_port);
    throw;
  }
  
  // Only valid for provider ports and standard like translation (user) ports.
  // Currently the requirement is that the port needs to map only when mapped 
  // to one port. If it would be mapped to more ports then this call would
  // remove all translation capability.
  if (n_system_mappings == 0) {
    reset_port_variables();
  }

  TTCN_Logger::log_port_misc(
    TitanLoggerApi::Port__Misc_reason::port__was__unmapped__from__system,
    port_name, SYSTEM_COMPREF, system_port);

  Free(unmapped_port);
}

void PORT::reset_port_variables() {
  
}

void PORT::init_port_variables() {
  
}

void PORT::change_port_state(translation_port_state /*state*/) {
  
}

void PORT::process_connect_listen(const char *local_port,
  component remote_component, const char *remote_port,
  transport_type_enum transport_type)
{
  PORT *port_ptr = lookup_by_name(local_port);
  if (port_ptr == NULL) {
    TTCN_Communication::send_connect_error(local_port, remote_component,
      remote_port, "Port %s does not exist.", local_port);
    return;
  } else if (!port_ptr->is_active) {
    TTCN_error("Internal error: Port %s is inactive when trying to "
      "connect it to %d:%s.", local_port, remote_component, remote_port);
  } else if (port_ptr->lookup_connection(remote_component, remote_port)
    != NULL) {
    TTCN_Communication::send_connect_error(local_port, remote_component,
      remote_port, "Port %s already has a connection towards %d:%s.",
      local_port, remote_component, remote_port);
    return;
  } else if (port_ptr->lookup_connection_to_compref(remote_component, NULL)
    != NULL) {
    TTCN_warning_begin("Port %s will have more than one connections with "
      "ports of test component ", local_port);
    COMPONENT::log_component_reference(remote_component);
    TTCN_Logger::log_event_str(". These connections cannot be used for "
      "sending even with explicit addressing.");
    TTCN_warning_end();
  }

  switch (transport_type) {
  case TRANSPORT_LOCAL:
    TTCN_Communication::send_connect_error(local_port, remote_component,
      remote_port, "Message CONNECT_LISTEN cannot refer to transport "
      "type LOCAL.");
    break;
  case TRANSPORT_INET_STREAM:
    port_ptr->connect_listen_inet_stream(remote_component, remote_port);
    break;
  case TRANSPORT_UNIX_STREAM:
    port_ptr->connect_listen_unix_stream(remote_component, remote_port);
    break;
  default:
    TTCN_Communication::send_connect_error(local_port, remote_component,
      remote_port, "Message CONNECT_LISTEN refers to invalid transport "
      "type (%d).", transport_type);
    break;
  }
}

void PORT::process_connect(const char *local_port,
  component remote_component, const char *remote_port,
  transport_type_enum transport_type, Text_Buf& text_buf)
{
  PORT *port_ptr = lookup_by_name(local_port);
  if (port_ptr == NULL) {
    TTCN_Communication::send_connect_error(local_port, remote_component,
      remote_port, "Port %s does not exist.", local_port);
    return;
  } else if (!port_ptr->is_active) {
    TTCN_error("Internal error: Port %s is inactive when trying to "
      "connect it to %d:%s.", local_port, remote_component, remote_port);
  } else if (port_ptr->lookup_connection(remote_component, remote_port)
    != NULL) {
    TTCN_Communication::send_connect_error(local_port, remote_component,
      remote_port, "Port %s already has a connection towards %d:%s.",
      local_port, remote_component, remote_port);
    return;
  } else if (port_ptr->lookup_connection_to_compref(remote_component, NULL)
    != NULL) {
    TTCN_warning_begin("Port %s will have more than one connections with "
      "ports of test component ", local_port);
    COMPONENT::log_component_reference(remote_component);
    TTCN_Logger::log_event_str(". These connections cannot be used for "
      "sending even with explicit addressing.");
    TTCN_warning_end();
  }

  switch (transport_type) {
  case TRANSPORT_LOCAL:
    port_ptr->connect_local(remote_component, remote_port);
    break;
  case TRANSPORT_INET_STREAM:
  case TRANSPORT_UNIX_STREAM:
    port_ptr->connect_stream(remote_component, remote_port, transport_type,
      text_buf);
    break;
  default:
    TTCN_Communication::send_connect_error(local_port, remote_component,
      remote_port, "Message CONNECT refers to invalid transport type "
      "(%d).", transport_type);
    break;
  }
}

void PORT::process_disconnect(const char *local_port,
  component remote_component, const char *remote_port)
{
  PORT *port_ptr = lookup_by_name(local_port);
  if (port_ptr == NULL) {
    TTCN_Communication::send_error("Message DISCONNECT refers to "
      "non-existent local port %s.", local_port);
    return;
  } else if (!port_ptr->is_active) {
    TTCN_error("Internal error: Port %s is inactive when trying to "
      "disconnect it from %d:%s.", local_port, remote_component,
      remote_port);
  }
  port_connection *conn_ptr = port_ptr->lookup_connection(remote_component,
    remote_port);
  if (conn_ptr == NULL) {
    // the connection does not exist
    if (self == remote_component && lookup_by_name(remote_port) == NULL) {
      // the remote endpoint is in the same component,
      // but it does not exist
      TTCN_Communication::send_error("Message DISCONNECT refers to "
        "non-existent port %s.", remote_port);
    } else {
      TTCN_Communication::send_disconnected(local_port, remote_component,
        remote_port);
    }
    return;
  }
  switch (conn_ptr->transport_type) {
  case TRANSPORT_LOCAL:
    port_ptr->disconnect_local(conn_ptr);
    break;
  case TRANSPORT_INET_STREAM:
  case TRANSPORT_UNIX_STREAM:
    port_ptr->disconnect_stream(conn_ptr);
    break;
  default:
    TTCN_error("Internal error: The connection of port %s to %d:%s has "
      "invalid transport type (%d) when trying to terminate the "
      "connection.", local_port, remote_component, remote_port,
      conn_ptr->transport_type);
  }
}

void PORT::make_local_connection(const char *src_port, const char *dest_port)
{
  PORT *src_ptr = lookup_by_name(src_port);
  if (src_ptr == NULL) TTCN_error("Connect operation refers to "
    "non-existent port %s.", src_port);
  else if (!src_ptr->is_active) TTCN_error("Internal error: Port %s is "
    "inactive when trying to connect it with local port %s.", src_port,
    dest_port);
  else if (src_ptr->lookup_connection(MTC_COMPREF, dest_port) != NULL) {
    TTCN_warning("Port %s is already connected with local port %s. "
      "Connect operation had no effect.", src_port, dest_port);
    return;
  } else if (src_ptr->lookup_connection_to_compref(MTC_COMPREF, NULL)
    != NULL) {
    TTCN_warning("Port %s will have more than one connections with local "
      "ports. These connections cannot be used for communication even "
      "with explicit addressing.", src_port);
  }
  PORT *dest_ptr = lookup_by_name(dest_port);
  if (dest_ptr == NULL) TTCN_error("Connect operation refers to "
    "non-existent port %s.", dest_port);
  else if (!dest_ptr->is_active) TTCN_error("Internal error: Port %s is "
    "inactive when trying to connect it with local port %s.", dest_port,
    src_port);
  src_ptr->add_local_connection(dest_ptr);
  if (src_ptr != dest_ptr) dest_ptr->add_local_connection(src_ptr);
}

void PORT::terminate_local_connection(const char *src_port,
  const char *dest_port)
{
  PORT *src_ptr = lookup_by_name(src_port);
  if (src_ptr == NULL) TTCN_error("Disconnect operation refers to "
    "non-existent port %s.", src_port);
  else if (!src_ptr->is_active) TTCN_error("Internal error: Port %s is "
    "inactive when trying to disconnect it from local port %s.", src_port,
    dest_port);
  port_connection *conn_ptr = src_ptr->lookup_connection(MTC_COMPREF,
    dest_port);
  if (conn_ptr != NULL) {
    PORT *dest_ptr = conn_ptr->local.port_ptr;
    src_ptr->remove_local_connection(conn_ptr);
    if (src_ptr != dest_ptr) {
      if (!dest_ptr->is_active) TTCN_error("Internal error: Port %s is "
        "inactive when trying to disconnect it from local port %s.",
        dest_port, src_port);
      port_connection *conn2_ptr =
        dest_ptr->lookup_connection(MTC_COMPREF, src_port);
      if (conn2_ptr == NULL) TTCN_error("Internal error: Port %s is "
        "connected with local port %s, but port %s does not have a "
        "connection to %s.", src_port, dest_port, dest_port, src_port);
      else dest_ptr->remove_local_connection(conn2_ptr);
    }
  } else {
    PORT *dest_ptr = lookup_by_name(dest_port);
    if (dest_ptr == NULL) TTCN_error("Disconnect operation refers to "
      "non-existent port %s.", dest_port);
    else if (src_ptr != dest_ptr) {
      if (!dest_ptr->is_active) TTCN_error("Internal error: Port %s is "
        "inactive when trying to disconnect it from local port %s.",
        dest_port, src_port);
      else if (dest_ptr->lookup_connection(MTC_COMPREF, src_port) != NULL)
        TTCN_error("Internal error: Port %s is connected with local "
          "port %s, but port %s does not have a connection to %s.",
          dest_port, src_port, src_port, dest_port);
    }
    TTCN_warning("Port %s does not have connection with local port %s. "
      "Disconnect operation had no effect.", src_port, dest_port);
  }
}

void PORT::map_port(const char *component_port, const char *system_port)
{
  PORT *port_ptr = lookup_by_name(component_port);
  if (port_ptr == NULL) TTCN_error("Map operation refers to "
    "non-existent port %s.", component_port);
  if (port_ptr->connection_list_head != NULL) {
    TTCN_error("Map operation is not allowed on a connected port (%s).", component_port);
  }
  port_ptr->map(system_port);
  if (!TTCN_Runtime::is_single())
    TTCN_Communication::send_mapped(component_port, system_port);
}

void PORT::unmap_port(const char *component_port, const char *system_port)
{
  PORT *port_ptr = lookup_by_name(component_port);
  if (port_ptr == NULL) TTCN_error("Unmap operation refers to "
    "non-existent port %s.", component_port);
  port_ptr->unmap(system_port);
  if (!TTCN_Runtime::is_single())
    TTCN_Communication::send_unmapped(component_port, system_port);
}

boolean PORT::check_port_state(const CHARSTRING& type) const
{
  if (type == "Started") {
    return is_started;
  } else if (type == "Halted") {
    return is_halted;
  } else if (type == "Stopped") {
    return (!is_started && !is_halted);
  } else if (type == "Connected") {
    return connection_list_head != NULL;
  } else if (type ==  "Mapped") {
    return n_system_mappings > 0;
  } else if (type == "Linked") {
    return (connection_list_head != NULL || n_system_mappings > 0);
  }
  TTCN_error("%s is not an allowed parameter of checkstate().", (const char*)type);
}

boolean PORT::any_check_port_state(const CHARSTRING& type)
{
  boolean result = FALSE;
  for (PORT *port = list_head; port != NULL; port = port->list_next) {
    result = port->check_port_state(type);
    if (result) {
      return TRUE;
    }
  }
  return FALSE;
}

boolean PORT::all_check_port_state(const CHARSTRING& type)
{
  boolean result = TRUE;
  for (PORT *port = list_head; port != NULL && result; port = port->list_next) {
    result = port->check_port_state(type);
  }
  return result;
}
