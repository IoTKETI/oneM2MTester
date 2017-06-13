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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "Communication.hh"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include "../common/dbgnew.hh"
#include "Types.h"
#include "Message_types.hh"
#include "Module_list.hh"
#include "Verdicttype.hh"
#include "Fd_And_Timeout_User.hh"
#include "Runtime.hh"
#include "Logger.hh"
#include "Port.hh"
#include "Component.hh"

#include "TitanLoggerApiSimple.hh"
#include "../common/version.h"

#include "Event_Handler.hh"
#include "Debugger.hh"
#include "DebugCommands.hh"

class MC_Connection : public Fd_And_Timeout_Event_Handler {
  virtual void Handle_Fd_Event(int fd,
    boolean is_readable, boolean is_writable, boolean is_error);
  virtual void Handle_Timeout(double time_since_last_call);
public:
  MC_Connection(const int * fd, Text_Buf * buf) :
    Fd_And_Timeout_Event_Handler(), mc_fd(fd), incoming_buf(buf) {}
  virtual void log() const;
private:
  const int * mc_fd;
  Text_Buf * incoming_buf;
};

int TTCN_Communication::mc_fd = -1;
HCNetworkHandler TTCN_Communication::hcnh;
boolean TTCN_Communication::local_addr_set = FALSE,
    TTCN_Communication::mc_addr_set = FALSE,
    TTCN_Communication::is_connected = FALSE;
Text_Buf TTCN_Communication::incoming_buf;
MC_Connection TTCN_Communication::mc_connection(
    &TTCN_Communication::mc_fd, &TTCN_Communication::incoming_buf);
double TTCN_Communication::call_interval = 0.0;

void TTCN_Communication::set_local_address(const char *host_name)
{
  if (local_addr_set)
    TTCN_warning("The local address has already been set.");
  if (is_connected)
    TTCN_error("Trying to change the local address, but there is an existing "
               "control connection to MC.");
  if (host_name == NULL){
    fprintf(stderr,"TTCN_Communication::set_local_address: internal error: "  // There is no connection to the MC
               "invalid host name.\r\n");                                       // We should log to the console also
    TTCN_error("TTCN_Communication::set_local_address: internal error: "
               "invalid host name.");
  }
  if (!hcnh.set_local_addr(host_name, 0)){
    fprintf(stderr,"Could not get the IP address for the local address "  // There is no connection to the MC
               "(%s): Host name lookup failure.\r\n", host_name);             // We should log to the console also
    TTCN_error("Could not get the IP address for the local address "
               "(%s): Host name lookup failure.", host_name);
  }
  TTCN_Logger::log_executor_misc(TitanLoggerApiSimple::ExecutorUnqualified_reason::local__address__was__set,
    hcnh.get_local_host_str(), hcnh.get_local_addr_str(), 0);
  local_addr_set = TRUE;
}

const IPAddress *TTCN_Communication::get_local_address()
{
  if (!local_addr_set)
    TTCN_error("TTCN_Communication::get_local_address: internal error: the "
               "local address has not been set.");
  return hcnh.get_local_addr();
}

void TTCN_Communication::set_mc_address(const char *host_name,
  unsigned short tcp_port)
{
  if (mc_addr_set)
    TTCN_warning("The address of MC has already been set.");
  if (is_connected)
    TTCN_error("Trying to change the address of MC, but there is an existing connection.");
  if (host_name == NULL){
    fprintf(stderr,"TTCN_Communication::set_mc_address: internal error: invalid host name.\r\n");
    TTCN_error("TTCN_Communication::set_mc_address: internal error: invalid host name.");
  }
  if (tcp_port <= 0){
    fprintf(stderr,"TTCN_Communication::set_mc_address: internal error: invalid TCP port. %hu\r\n",tcp_port);
    TTCN_error("TTCN_Communication::set_mc_address: internal error: invalid TCP port.");
  }
  hcnh.set_family(host_name);
  if (!hcnh.set_mc_addr(host_name, tcp_port)){
    fprintf(stderr,"Could not get the IP address of MC (%s): Host name lookup "
               "failure.\r\n", host_name);
    TTCN_error("Could not get the IP address of MC (%s): Host name lookup "
               "failure.", host_name);
  }
  if ((hcnh.get_mc_addr())->is_local()){
    fprintf(stderr,"The address of MC was set to a local IP address. This may "
                 "cause incorrect behavior if a HC from a remote host also "
                 "connects to MC.\r\n");
    TTCN_warning("The address of MC was set to a local IP address. This may "
                 "cause incorrect behavior if a HC from a remote host also "
                 "connects to MC.");
  }
  TTCN_Logger::log_executor_misc(TitanLoggerApiSimple::ExecutorUnqualified_reason::address__of__mc__was__set,
    hcnh.get_mc_host_str(), hcnh.get_mc_addr_str(), 0);
  mc_addr_set = TRUE;
}

const IPAddress *TTCN_Communication::get_mc_address()
{
  if (!mc_addr_set)
    TTCN_error("TTCN_Communication::get_mc_address: internal error: the "
               "address of MC has not been set.");
  return hcnh.get_mc_addr();
}

boolean TTCN_Communication::is_mc_connected()
{
  return is_connected;
}

void TTCN_Communication::connect_mc()
{
  if (is_connected) TTCN_error("Trying to re-connect to MC, but there is an "
    "existing connection.");
  if (!mc_addr_set) TTCN_error("Trying to connect to MC, but the address of "
    "MC has not yet been set.");

  // Trying to connect to local mc through unix domain socket
  // TODO: Disable if config file parameter is set
  if ((hcnh.get_mc_addr())->is_local()
      || (local_addr_set && *(hcnh.get_mc_addr()) == *(hcnh.get_local_addr()))) {
    sockaddr_un localaddr_unix;
    memset(&localaddr_unix, 0, sizeof(localaddr_unix));
    localaddr_unix.sun_family = AF_UNIX;
    snprintf(localaddr_unix.sun_path, sizeof(localaddr_unix.sun_path),
      "/tmp/ttcn3-mctr-%u", hcnh.get_mc_port());
    mc_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (mc_fd >= (int)FD_SETSIZE) {
      close(mc_fd);
    } else if (mc_fd >= 0) {
      if (connect(mc_fd, (struct sockaddr*)&localaddr_unix,
        sizeof(localaddr_unix)) == 0) {
        goto unix_end; // connected successfully
      } else {
        close(mc_fd);
      }
    }
  }

#ifdef WIN32
again:
#endif
  mc_fd = hcnh.socket();
  if (mc_fd < 0) {
    fprintf(stderr,"Socket creation failed when connecting to MC.");
    TTCN_error("Socket creation failed when connecting to MC.");
  }
  else if (mc_fd >= (int)FD_SETSIZE) {
    close(mc_fd);
    fprintf(stderr,"When connecting to MC: "
               "The file descriptor returned by the operating system (%d) is "
               "too large for use with the select() system call.\r\n", mc_fd);
    TTCN_error("When connecting to MC: "
               "The file descriptor returned by the operating system (%d) is "
               "too large for use with the select() system call.", mc_fd);
  }

  if (local_addr_set) {
    if (hcnh.bind_local_addr(mc_fd)) {
      fprintf(stderr,"Binding IP address %s to the local endpoint of the "
                 "control connection failed when connecting to MC.\r\n", hcnh.get_local_addr_str());
      TTCN_error("Binding IP address %s to the local endpoint of the "
                 "control connection failed when connecting to MC.",
                 hcnh.get_local_addr_str());
      close(mc_fd);
    }
  }

  if (hcnh.connect_to_mc(mc_fd)) {
#ifdef WIN32
    if (errno == EADDRINUSE) {
      close(mc_fd);
      errno = 0;
      TTCN_warning("connect() returned error code EADDRINUSE. "
                   "Perhaps this is a Cygwin bug. Trying to connect again.");
      goto again;
    }
#endif
    fprintf(stderr,"Connecting to MC failed. MC address: %s:%hu %s\r\n",hcnh.get_mc_addr_str(),hcnh.get_mc_port(),strerror(errno));
    TTCN_error("Connecting to MC failed.");
    close(mc_fd);
  }

  if (!local_addr_set) {
    if (hcnh.getsockname_local_addr(mc_fd)) {
      close(mc_fd);
      TTCN_error("getsockname() system call failed on the socket of the "
                 "control connection to MC.");
    }
    TTCN_Logger::log_executor_misc(
      TitanLoggerApiSimple::ExecutorUnqualified_reason::address__of__control__connection,
      NULL, hcnh.get_local_addr_str(), 0);
    local_addr_set = TRUE;
  }

  if (!set_tcp_nodelay(mc_fd)) {
    close(mc_fd);
    TTCN_error("Setting the TCP_NODELAY flag failed on the socket of "
               "the control connection to MC.");
  }

unix_end:

  if (!set_close_on_exec(mc_fd)) {
    close(mc_fd);
    TTCN_error("Setting the close-on-exec flag failed on the socket of "
               "the control connection to MC.");
  }

  Fd_And_Timeout_User::add_fd(mc_fd, &mc_connection, FD_EVENT_RD);

  TTCN_Logger::log_executor_runtime(
    TitanLoggerApiSimple::ExecutorRuntime_reason::connected__to__mc);

  is_connected = TRUE;
}

void TTCN_Communication::disconnect_mc()
{
  if (is_connected) {
    shutdown(mc_fd, 1);
    int recv_len;
    do {
      char buf[1024];
      recv_len = recv(mc_fd, buf, sizeof(buf), 0);
    } while (recv_len > 0);
    errno = 0;
    close_mc_connection();
    TTCN_Logger::log_executor_runtime(
      TitanLoggerApiSimple::ExecutorRuntime_reason::disconnected__from__mc);
  }
}

void TTCN_Communication::close_mc_connection()
{
  if (is_connected) {
    int tmp_mc_fd = mc_fd;
    call_interval = 0.0;
    close(mc_fd);
    mc_fd = -1;
    is_connected = FALSE;
    incoming_buf.reset();
    // Removing the fd has to be done after closing the mc connection
    // to prevent segmentation fault or broken pipe error
    // in case remove_fd would try to print an error log.
    Fd_And_Timeout_User::remove_fd(tmp_mc_fd, &mc_connection, FD_EVENT_RD);
    Fd_And_Timeout_User::set_timer(&mc_connection, 0.0);
  }
}

boolean TTCN_Communication::transport_unix_stream_supported()
{
  int fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (fd >= 0) {
    close(fd);
    TTCN_Logger::log_executor_misc(
      TitanLoggerApiSimple::ExecutorUnqualified_reason::host__support__unix__domain__sockets,
      NULL, NULL, 0);
    return TRUE;
  } else {
    TTCN_Logger::log_executor_misc(
      TitanLoggerApiSimple::ExecutorUnqualified_reason::host__support__unix__domain__sockets,
      NULL, NULL, errno);
    return FALSE;
  }
}

boolean TTCN_Communication::set_close_on_exec(int fd)
{
  int flags = fcntl(fd, F_GETFD);
  if (flags < 0) {
    TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
    TTCN_Logger::log_event("System call fcntl(F_GETFD) failed on file "
      "descriptor %d.", fd);
    TTCN_Logger::OS_error();
    TTCN_Logger::end_event();
    return FALSE;
  }

  flags |= FD_CLOEXEC;

  if (fcntl(fd, F_SETFD, flags) == -1) {
    TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
    TTCN_Logger::log_event("System call fcntl(F_SETFD) failed on file "
      "descriptor %d.", fd);
    TTCN_Logger::OS_error();
    TTCN_Logger::end_event();
    return FALSE;
  }
  return TRUE;
}

boolean TTCN_Communication::set_non_blocking_mode(int fd,
  boolean enable_nonblock)
{
  int flags = fcntl(fd, F_GETFL);
  if (flags < 0) {
    TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
    TTCN_Logger::log_event("System call fcntl(F_GETFL) failed on file "
      "descriptor %d.", fd);
    TTCN_Logger::OS_error();
    TTCN_Logger::end_event();
    return FALSE;
  }

  if (enable_nonblock) flags |= O_NONBLOCK;
  else flags &= ~O_NONBLOCK;

  if (fcntl(fd, F_SETFL, flags) == -1) {
    TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
    TTCN_Logger::log_event("System call fcntl(F_SETFL) failed on file "
      "descriptor %d.", fd);
    TTCN_Logger::OS_error();
    TTCN_Logger::end_event();
    return FALSE;
  }
  return TRUE;
}

boolean TTCN_Communication::set_tcp_nodelay(int fd)
{
  const int on = 1;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&on,
    sizeof(on))) {
    TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
    TTCN_Logger::log_event("System call setsockopt(TCP_NODELAY) failed on "
      "file descriptor %d.", fd);
    TTCN_Logger::OS_error();
    TTCN_Logger::end_event();
    return FALSE;
  }
  return TRUE;
}

boolean TTCN_Communication::increase_send_buffer(int fd,
  int &old_size, int& new_size)
{
  int set_size;
  socklen_type optlen = sizeof(old_size);
  // obtaining the current buffer size first
  if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&old_size, &optlen))
    goto getsockopt_failure;
  if (old_size <= 0) {
    TTCN_Logger::log(TTCN_Logger::ERROR_UNQUALIFIED,
      "System call getsockopt(SO_SNDBUF) returned invalid buffer size (%d) "
      "on file descriptor %d.", old_size, fd);
    return FALSE;
  }
  // trying to double the buffer size
  set_size = 2 * old_size;
  if (set_size > old_size) {
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&set_size,
      sizeof(set_size))) {
      // the operation failed
      switch (errno) {
      case ENOMEM:
      case ENOBUFS:
        errno = 0;
        break;
      default:
        // other error codes indicate a fatal error
        goto setsockopt_failure;
      }
    } else {
      // the operation was successful
      goto success;
    }
  }
  // trying to perform a binary search to determine the maximum buffer size
  set_size = old_size;
  for (int size_step = old_size / 2; size_step > 0; size_step /= 2) {
    int tried_size = set_size + size_step;
    if (tried_size > set_size) {
      if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&tried_size,
        sizeof(tried_size))) {
        // the operation failed
        switch (errno) {
        case ENOMEM:
        case ENOBUFS:
          errno = 0;
          break;
        default:
          // other error codes indicate a fatal error
          goto setsockopt_failure;
        }
      } else {
        // the operation was successful
        set_size = tried_size;
      }
    }
  }
  if (set_size <= old_size) return FALSE;
  success:
  // querying the new effective buffer size (it might be smaller
  // than set_size but should not be smaller than old_size)
  optlen = sizeof(new_size);
  if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&new_size,
    &optlen)) goto getsockopt_failure;
  if (new_size > old_size) return TRUE;
  else {
    if (new_size < old_size) TTCN_Logger::log(TTCN_Logger::ERROR_UNQUALIFIED,
      "System call getsockopt(SO_SNDBUF) returned unexpected buffer size "
      "(%d, after increasing it from %d to %d) on file descriptor %d.",
      new_size, old_size, set_size, fd);
    return FALSE;
  }
  getsockopt_failure:
  TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
  TTCN_Logger::log_event("System call getsockopt(SO_SNDBUF) failed on file "
    "descriptor %d.", fd);
  TTCN_Logger::OS_error();
  TTCN_Logger::end_event();
  return FALSE;
  setsockopt_failure:
  TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
  TTCN_Logger::log_event("System call setsockopt(SO_SNDBUF) failed on file "
    "descriptor %d.", fd);
  TTCN_Logger::OS_error();
  TTCN_Logger::end_event();
  return FALSE;
}

#define INITIAL_CALL_INTERVAL 1.0
#define CALL_INTERVAL_INCREMENT 2.0

void TTCN_Communication::enable_periodic_call()
{
  call_interval = INITIAL_CALL_INTERVAL;
  Fd_And_Timeout_User::set_timer(&mc_connection, call_interval, TRUE,
    FALSE /*call_anyway*/);
}

void TTCN_Communication::increase_call_interval()
{
  if (call_interval <= 0.0) TTCN_error("Internal error: "
    "TTCN_Communication::increase_call_interval() was called when call "
    "interval is not set.");
  call_interval *= CALL_INTERVAL_INCREMENT;
  Fd_And_Timeout_User::set_timer(&mc_connection, call_interval, TRUE,
    FALSE /*call_anyway*/);
}

void TTCN_Communication::disable_periodic_call()
{
  Fd_And_Timeout_User::set_timer(&mc_connection, 0.0);
  call_interval = 0.0;
}

void MC_Connection::Handle_Fd_Event(int fd, boolean is_readable, boolean,
  boolean is_error)
{
  if (fd != *mc_fd)
    TTCN_error("MC_Connection::Fd_And_Timeout_Event_Handler: unexpected "
      "file descriptor"); // not necessary - debugging
  if (is_error)
    TTCN_warning("Error occurred on the control connection to MC");
  if (is_readable) {
    char *buf_ptr;
    int buf_len;
    incoming_buf->get_end(buf_ptr, buf_len);

    int recv_len = recv(*mc_fd, buf_ptr, buf_len, 0);

    if (recv_len > 0) {
      // reason: data has arrived
      incoming_buf->increase_length(recv_len);
      // If the component is idle the processing is done in the outer
      // stack frame (i.e. in TTCN_Runtime::xxx_main()).
      if (!TTCN_Runtime::is_idle())
        TTCN_Communication::process_all_messages_tc();
    } else {
      // First closing the TCP connection to avoid EPIPE ("Broken pipe")
      // errors and/or SIGPIPE signals when trying to send anything
      // (e.g. log events or error messages) towards MC.
      TTCN_Communication::close_mc_connection();
      if (recv_len == 0) {
        // reason: TCP connection was closed by peer
        TTCN_error("Control connection was closed unexpectedly by MC.");
      } else {
        // reason: error occurred
        TTCN_error("Receiving data on the control connection from MC "
          "failed.");
      }
    }
  }
}

void MC_Connection::Handle_Timeout(double /*time_since_last_call*/)
{
  if (TTCN_Runtime::get_state() == TTCN_Runtime::HC_OVERLOADED) {
    // indicate the timeout to be handled in process_all_messages_hc()
    TTCN_Runtime::set_state(TTCN_Runtime::HC_OVERLOADED_TIMEOUT);
  } else {
    TTCN_warning("Unexpected timeout occurred on the control "
      "connection to MC.");
    TTCN_Communication::disable_periodic_call();
  }
}

void MC_Connection::log() const
{
  TTCN_Logger::log_event("mc connection");
}

void TTCN_Communication::process_all_messages_hc()
{
  if (!TTCN_Runtime::is_hc()) TTCN_error("Internal error: "
    "TTCN_Communication::process_all_messages_hc() was called in invalid "
    "state.");
  TTCN_Runtime::wait_terminated_processes();
  boolean wait_flag = FALSE;
  boolean check_overload = TTCN_Runtime::is_overloaded();
  while (incoming_buf.is_message()) {
    wait_flag = TRUE;
    int msg_len = incoming_buf.pull_int().get_val();
    int msg_end = incoming_buf.get_pos() + msg_len;
    int msg_type = incoming_buf.pull_int().get_val();
    // messages: MC -> HC
    switch (msg_type) {
    case MSG_ERROR:
      process_error();
      break;
    case MSG_CONFIGURE:
      process_configure(msg_end, FALSE);
      break;
    case MSG_CREATE_MTC:
      process_create_mtc();
      TTCN_Runtime::wait_terminated_processes();
      wait_flag = FALSE;
      check_overload = FALSE;
      break;
    case MSG_CREATE_PTC:
      process_create_ptc();
      TTCN_Runtime::wait_terminated_processes();
      wait_flag = FALSE;
      check_overload = FALSE;
      break;
    case MSG_KILL_PROCESS:
      process_kill_process();
      TTCN_Runtime::wait_terminated_processes();
      wait_flag = FALSE;
      break;
    case MSG_EXIT_HC:
      process_exit_hc();
      break;
    case MSG_DEBUG_COMMAND:
      process_debug_command();
      break;
    default:
      process_unsupported_message(msg_type, msg_end);
    }
  }
  if (wait_flag) TTCN_Runtime::wait_terminated_processes();
  if (check_overload && TTCN_Runtime::is_overloaded())
    TTCN_Runtime::check_overload();
}

void TTCN_Communication::process_all_messages_tc()
{
  if (!TTCN_Runtime::is_tc()) TTCN_error("Internal error: "
    "TTCN_Communication::process_all_messages_tc() was called in invalid "
    "state.");
  while (incoming_buf.is_message()) {
    int msg_len = incoming_buf.pull_int().get_val();
    int msg_end = incoming_buf.get_pos() + msg_len;
    int msg_type = incoming_buf.pull_int().get_val();
    // messages: MC -> TC
    switch (msg_type) {
    case MSG_ERROR:
      process_error();
      break;
    case MSG_CREATE_ACK:
      process_create_ack();
      break;
    case MSG_START_ACK:
      process_start_ack();
      break;
    case MSG_STOP:
      process_stop();
      break;
    case MSG_STOP_ACK:
      process_stop_ack();
      break;
    case MSG_KILL_ACK:
      process_kill_ack();
      break;
    case MSG_RUNNING:
      process_running();
      break;
    case MSG_ALIVE:
      process_alive();
      break;
    case MSG_DONE_ACK:
      process_done_ack(msg_end);
      break;
    case MSG_KILLED_ACK:
      process_killed_ack();
      break;
    case MSG_CANCEL_DONE:
      if (TTCN_Runtime::is_mtc()) process_cancel_done_mtc();
      else process_cancel_done_ptc();
      break;
    case MSG_COMPONENT_STATUS:
      if (TTCN_Runtime::is_mtc()) process_component_status_mtc(msg_end);
      else process_component_status_ptc(msg_end);
      break;
    case MSG_CONNECT_LISTEN:
      process_connect_listen();
      break;
    case MSG_CONNECT:
      process_connect();
      break;
    case MSG_CONNECT_ACK:
      process_connect_ack();
      break;
    case MSG_DISCONNECT:
      process_disconnect();
      break;
    case MSG_DISCONNECT_ACK:
      process_disconnect_ack();
      break;
    case MSG_MAP:
      process_map();
      break;
    case MSG_MAP_ACK:
      process_map_ack();
      break;
    case MSG_UNMAP:
      process_unmap();
      break;
    case MSG_UNMAP_ACK:
      process_unmap_ack();
      break;
    case MSG_DEBUG_COMMAND:
      process_debug_command();
      break;
    default:
      if (TTCN_Runtime::is_mtc()) {
        // messages: MC -> MTC
        switch (msg_type) {
        case MSG_EXECUTE_CONTROL:
          process_execute_control();
          break;
        case MSG_EXECUTE_TESTCASE:
          process_execute_testcase();
          break;
        case MSG_PTC_VERDICT:
          process_ptc_verdict();
          break;
        case MSG_CONTINUE:
          process_continue();
          break;
        case MSG_EXIT_MTC:
          process_exit_mtc();
          break;
        case MSG_CONFIGURE:
          process_configure(msg_end, TRUE);
          break;
        default:
          process_unsupported_message(msg_type, msg_end);
        }
      } else {
        // messages: MC -> PTC
        switch (msg_type) {
        case MSG_START:
          process_start();
          break;
        case MSG_KILL:
          process_kill();
          break;
        default:
          process_unsupported_message(msg_type, msg_end);
        }
      }
    }
  }
}

void TTCN_Communication::process_debug_messages()
{
  // receives and processes messages from the MC, while test execution is halted
  // by the debugger
  char *buf_ptr;
  int buf_len;
  Text_Buf storage_buf;
  while (ttcn3_debugger.is_halted()) {
    incoming_buf.get_end(buf_ptr, buf_len);

    int recv_len = recv(mc_fd, buf_ptr, buf_len, 0);

    if (recv_len > 0) {
      incoming_buf.increase_length(recv_len);

      while (incoming_buf.is_message() && ttcn3_debugger.is_halted()) {
        int msg_len = incoming_buf.pull_int().get_val();
        int msg_end = incoming_buf.get_pos() + msg_len;
        int msg_type = incoming_buf.pull_int().get_val();
        // process only debug commands and 'stop' messages, store the rest
        switch (msg_type) {
        case MSG_DEBUG_COMMAND:
          process_debug_command();
          break;
        case MSG_STOP:
          process_stop();
          break;
        default: {
          // store all other messages in a different buffer
          int data_len = msg_end - incoming_buf.get_pos();
          char* msg_data = new char[data_len];
          incoming_buf.pull_raw(data_len, msg_data);
          incoming_buf.cut_message();
          storage_buf.push_int(msg_type);
          storage_buf.push_raw(data_len, msg_data);
          delete [] msg_data;
          storage_buf.calculate_length();
          break; }
        }
      }
    }
  }
  // append the stored messages to the beginning of the main buffer and
  // process them
  if (storage_buf.is_message()) {
    incoming_buf.push_raw_front(storage_buf.get_len(), storage_buf.get_data());
    process_all_messages_tc();
  }
}

void TTCN_Communication::send_version()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_VERSION);
  text_buf.push_int(TTCN3_MAJOR);
  text_buf.push_int(TTCN3_MINOR);
  text_buf.push_int(TTCN3_PATCHLEVEL);
#ifdef TTCN3_BUILDNUMBER
  text_buf.push_int(TTCN3_BUILDNUMBER);
#else
  text_buf.push_int((RInt)0);
#endif
  Module_List::push_version(text_buf);
  struct utsname uts;
  if (uname(&uts) < 0) TTCN_error("System call uname() failed.");
  text_buf.push_string(uts.nodename);
  text_buf.push_string(uts.machine);
  text_buf.push_string(uts.sysname);
  text_buf.push_string(uts.release);
  text_buf.push_string(uts.version);
  boolean unix_stream_supported = transport_unix_stream_supported();

  // LOCAL (software loop) and INET_STREAM (TCP) transports are always
  // supported
  int n_supported_transports = 2;

  if (unix_stream_supported) n_supported_transports++;
  text_buf.push_int(n_supported_transports);
  text_buf.push_int(TRANSPORT_LOCAL);
  text_buf.push_int(TRANSPORT_INET_STREAM);
  if (unix_stream_supported)
    text_buf.push_int(TRANSPORT_UNIX_STREAM);
  send_message(text_buf);
}

void TTCN_Communication::send_configure_ack()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONFIGURE_ACK);
  send_message(text_buf);
}

void TTCN_Communication::send_configure_nak()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONFIGURE_NAK);
  send_message(text_buf);
}

void TTCN_Communication::send_create_nak(component component_reference,
  const char *fmt_str, ...)
{
  va_list ap;
  va_start(ap, fmt_str);
  char *error_str = mprintf_va_list(fmt_str, ap);
  va_end(ap);
  Text_Buf text_buf;
  text_buf.push_int(MSG_CREATE_NAK);
  text_buf.push_int(component_reference);
  text_buf.push_string(error_str);
  Free(error_str);
  send_message(text_buf);
}

void TTCN_Communication::send_hc_ready()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_HC_READY);
  send_message(text_buf);
}

void TTCN_Communication::send_create_req(const char *component_type_module,
  const char *component_type_name, const char *component_name,
  const char *component_location, boolean is_alive)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CREATE_REQ);
  text_buf.push_string(component_type_module);
  text_buf.push_string(component_type_name);
  text_buf.push_string(component_name);
  text_buf.push_string(component_location);
  text_buf.push_int(is_alive ? 1 : 0);
  send_message(text_buf);
}

void TTCN_Communication::prepare_start_req(Text_Buf& text_buf,
  component component_reference, const char *module_name,
  const char *function_name)
{
  text_buf.push_int(MSG_START_REQ);
  text_buf.push_int(component_reference);
  text_buf.push_string(module_name);
  text_buf.push_string(function_name);
}

void TTCN_Communication::send_stop_req(component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_STOP_REQ);
  text_buf.push_int(component_reference);
  send_message(text_buf);
}

void TTCN_Communication::send_kill_req(component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_KILL_REQ);
  text_buf.push_int(component_reference);
  send_message(text_buf);
}

void TTCN_Communication::send_is_running(component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_IS_RUNNING);
  text_buf.push_int(component_reference);
  send_message(text_buf);
}

void TTCN_Communication::send_is_alive(component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_IS_ALIVE);
  text_buf.push_int(component_reference);
  send_message(text_buf);
}

void TTCN_Communication::send_done_req(component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DONE_REQ);
  text_buf.push_int(component_reference);
  send_message(text_buf);
}

void TTCN_Communication::send_killed_req(component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_KILLED_REQ);
  text_buf.push_int(component_reference);
  send_message(text_buf);
}

void TTCN_Communication::send_cancel_done_ack(component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CANCEL_DONE_ACK);
  text_buf.push_int(component_reference);
  send_message(text_buf);
}

void TTCN_Communication::send_connect_req(component src_component,
  const char *src_port, component dst_component, const char *dst_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONNECT_REQ);
  text_buf.push_int(src_component);
  text_buf.push_string(src_port);
  text_buf.push_int(dst_component);
  text_buf.push_string(dst_port);
  send_message(text_buf);
}

void TTCN_Communication::send_connect_listen_ack_inet_stream(
  const char *local_port, component remote_component,
  const char *remote_port, const IPAddress *local_address)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONNECT_LISTEN_ACK);
  text_buf.push_string(local_port);
  text_buf.push_int(remote_component);
  text_buf.push_string(remote_port);
  text_buf.push_int(TRANSPORT_INET_STREAM);
  local_address->push_raw(text_buf);
  send_message(text_buf);
}

void TTCN_Communication::send_connect_listen_ack_unix_stream(
  const char *local_port, component remote_component,
  const char *remote_port, const struct sockaddr_un *local_address)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONNECT_LISTEN_ACK);
  text_buf.push_string(local_port);
  text_buf.push_int(remote_component);
  text_buf.push_string(remote_port);
  text_buf.push_int(TRANSPORT_UNIX_STREAM);
  text_buf.push_string(local_address->sun_path);
  send_message(text_buf);
}

void TTCN_Communication::send_connected(const char *local_port,
  component remote_component, const char *remote_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONNECTED);
  text_buf.push_string(local_port);
  text_buf.push_int(remote_component);
  text_buf.push_string(remote_port);
  send_message(text_buf);
}

void TTCN_Communication::send_connect_error(const char *local_port,
  component remote_component, const char *remote_port,
  const char *fmt_str, ...)
{
  va_list ap;
  va_start(ap, fmt_str);
  char *error_str = mprintf_va_list(fmt_str, ap);
  va_end(ap);
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONNECT_ERROR);
  text_buf.push_string(local_port);
  text_buf.push_int(remote_component);
  text_buf.push_string(remote_port);
  text_buf.push_string(error_str);
  Free(error_str);
  send_message(text_buf);
}

void TTCN_Communication::send_disconnect_req(component src_component,
  const char *src_port, component dst_component, const char *dst_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DISCONNECT_REQ);
  text_buf.push_int(src_component);
  text_buf.push_string(src_port);
  text_buf.push_int(dst_component);
  text_buf.push_string(dst_port);
  send_message(text_buf);
}

void TTCN_Communication::send_disconnected(const char *local_port,
  component remote_component, const char *remote_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DISCONNECTED);
  text_buf.push_string(local_port);
  text_buf.push_int(remote_component);
  text_buf.push_string(remote_port);
  send_message(text_buf);
}

void TTCN_Communication::send_map_req(component src_component,
  const char *src_port, const char *system_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_MAP_REQ);
  text_buf.push_int(src_component);
  text_buf.push_string(src_port);
  text_buf.push_string(system_port);
  send_message(text_buf);
}

void TTCN_Communication::send_mapped(const char *local_port,
  const char *system_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_MAPPED);
  text_buf.push_string(local_port);
  text_buf.push_string(system_port);
  send_message(text_buf);
}

void TTCN_Communication::send_unmap_req(component src_component,
  const char *src_port, const char *system_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_UNMAP_REQ);
  text_buf.push_int(src_component);
  text_buf.push_string(src_port);
  text_buf.push_string(system_port);
  send_message(text_buf);
}

void TTCN_Communication::send_unmapped(const char *local_port,
  const char *system_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_UNMAPPED);
  text_buf.push_string(local_port);
  text_buf.push_string(system_port);
  send_message(text_buf);
}

void TTCN_Communication::send_mtc_created()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_MTC_CREATED);
  send_message(text_buf);
}

void TTCN_Communication::send_testcase_started(const char *testcase_module,
  const char *testcase_name, const char *mtc_comptype_module,
  const char *mtc_comptype_name, const char *system_comptype_module,
  const char *system_comptype_name)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_TESTCASE_STARTED);
  text_buf.push_string(testcase_module);
  text_buf.push_string(testcase_name);
  text_buf.push_string(mtc_comptype_module);
  text_buf.push_string(mtc_comptype_name);
  text_buf.push_string(system_comptype_module);
  text_buf.push_string(system_comptype_name);
  send_message(text_buf);
}

void TTCN_Communication::send_testcase_finished(verdicttype final_verdict,
  const char* reason)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_TESTCASE_FINISHED);
  text_buf.push_int(final_verdict);
  text_buf.push_string(reason);
  send_message(text_buf);
}

void TTCN_Communication::send_mtc_ready()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_MTC_READY);
  send_message(text_buf);
}

void TTCN_Communication::send_ptc_created(component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_PTC_CREATED);
  text_buf.push_int(component_reference);
  send_message(text_buf);
}

void TTCN_Communication::prepare_stopped(Text_Buf& text_buf,
  const char *return_type)
{
  text_buf.push_int(MSG_STOPPED);
  text_buf.push_string(return_type);
}

void TTCN_Communication::send_stopped()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_STOPPED);
  // add an empty return type
  text_buf.push_string(NULL);
  send_message(text_buf);
}

void TTCN_Communication::prepare_stopped_killed(Text_Buf& text_buf,
  verdicttype final_verdict, const char *return_type, const char* reason)
{
  text_buf.push_int(MSG_STOPPED_KILLED);
  text_buf.push_int(final_verdict);
  text_buf.push_string(reason);
  text_buf.push_string(return_type);
}

void TTCN_Communication::send_stopped_killed(verdicttype final_verdict,
  const char* reason)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_STOPPED_KILLED);
  text_buf.push_int(final_verdict);
  text_buf.push_string(reason);
  // add an empty return type
  text_buf.push_string(NULL);
  send_message(text_buf);
}

void TTCN_Communication::send_killed(verdicttype final_verdict,
  const char* reason)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_KILLED);
  text_buf.push_int(final_verdict);
  text_buf.push_string(reason);
  send_message(text_buf);
}

void TTCN_Communication::send_debug_return_value(int return_type, const char* message)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DEBUG_RETURN_VALUE);
  text_buf.push_int(return_type);
  if (message != NULL) {
    timeval tv;
    gettimeofday(&tv, NULL);
    text_buf.push_int(tv.tv_sec);
    text_buf.push_int(tv.tv_usec);
    text_buf.push_string(message);
  }
  send_message(text_buf);
}

void TTCN_Communication::send_debug_halt_req()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DEBUG_HALT_REQ);
  send_message(text_buf);
}

void TTCN_Communication::send_debug_continue_req()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DEBUG_CONTINUE_REQ);
  send_message(text_buf);
}

void TTCN_Communication::send_debug_batch(const char* batch_file)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DEBUG_BATCH);
  text_buf.push_string(batch_file);
  send_message(text_buf);
}

boolean TTCN_Communication::send_log(time_t timestamp_sec, long timestamp_usec,
  unsigned int event_severity, size_t message_text_len,
  const char *message_text)
{
  if (is_connected) {
    Text_Buf text_buf;
    text_buf.push_int(MSG_LOG);
    text_buf.push_int(timestamp_sec);
    text_buf.push_int(timestamp_usec);
    text_buf.push_int(event_severity);
    text_buf.push_int(message_text_len);
    text_buf.push_raw(message_text_len, message_text);
    send_message(text_buf);
    /* If an ERROR message (indicating a version mismatch) arrives from MC
	   in state HC_IDLE (i.e. before CONFIGURE) it shall be
	   printed to the console as well. */
    if (TTCN_Runtime::get_state() == TTCN_Runtime::HC_IDLE) return FALSE;
    else return TRUE;
  } else {
    switch (TTCN_Runtime::get_state()) {
    case TTCN_Runtime::HC_EXIT:
    case TTCN_Runtime::MTC_INITIAL:
    case TTCN_Runtime::MTC_EXIT:
    case TTCN_Runtime::PTC_INITIAL:
    case TTCN_Runtime::PTC_EXIT:
      /* Do not print the first/last few lines of logs to the console
	       even if ConsoleMask is set to LOG_ALL */
      return TRUE;
    default:
      return FALSE;
    }
  }
}

void TTCN_Communication::send_error(const char *fmt_str, ...)
{
  va_list ap;
  va_start(ap, fmt_str);
  char *error_str = mprintf_va_list(fmt_str, ap);
  va_end(ap);
  Text_Buf text_buf;
  text_buf.push_int((RInt)MSG_ERROR);
  text_buf.push_string(error_str);
  Free(error_str);
  send_message(text_buf);
}

void TTCN_Communication::send_message(Text_Buf& text_buf)
{
  if (!is_connected) TTCN_error("Trying to send a message to MC, but the "
    "control connection is down.");
  text_buf.calculate_length();
  const char *msg_ptr = text_buf.get_data();
  size_t msg_len = text_buf.get_len(), sent_len = 0;
  while (sent_len < msg_len) {
    int ret_val = send(mc_fd, msg_ptr + sent_len, msg_len - sent_len, 0);
    if (ret_val > 0) sent_len += ret_val;
    else {
      switch (errno) {
      case EINTR:
        // a signal occurred: do nothing, just try again
        errno = 0;
        break;
      default:
        close_mc_connection();
        TTCN_error("Sending data on the control connection to MC "
          "failed.");
      }
    }
  }
}

void TTCN_Communication::process_configure(int msg_end, boolean to_mtc)
{
  switch (TTCN_Runtime::get_state()) {
  case TTCN_Runtime::HC_IDLE:
  case TTCN_Runtime::HC_ACTIVE:
  case TTCN_Runtime::HC_OVERLOADED:
    if (!to_mtc) {
      break;
    }
    // no break
  case TTCN_Runtime::MTC_IDLE:
    if (to_mtc) {
      break;
    }
    // no break
  default:
    incoming_buf.cut_message();
    send_error("Message CONFIGURE arrived in invalid state.");
    return;
  }

  TTCN_Runtime::set_state(to_mtc ? TTCN_Runtime::MTC_CONFIGURING : TTCN_Runtime::HC_CONFIGURING);
  TTCN_Logger::log_configdata(TitanLoggerApiSimple::ExecutorConfigdata_reason::received__from__mc);

  // take the config string directly from the buffer for efficiency reasons
  int config_str_len = incoming_buf.pull_int().get_val();
  int config_str_begin = incoming_buf.get_pos();
  if (config_str_begin + config_str_len != msg_end) {
    incoming_buf.cut_message();
    send_error("Malformed message CONFIGURE was received.");
    return;
  }
  const char *config_str = incoming_buf.get_data() + config_str_begin;
  boolean success = process_config_string(config_str, config_str_len);

  // Only non component specific settings will be applied.  The plug-ins need
  // to be loaded due to resetting.
  TTCN_Logger::load_plugins(NULL_COMPREF, "");
  TTCN_Logger::set_plugin_parameters(NULL_COMPREF, "");
  TTCN_Logger::open_file();
  if (success) {
    try {
      Module_List::log_param();
      Module_List::post_init_modules();
    } catch (const TC_Error& TC_error) {
      TTCN_Logger::log_executor_runtime(
        TitanLoggerApiSimple::ExecutorRuntime_reason::initialization__of__modules__failed);
      success = FALSE;
    }
  } else {
    TTCN_Logger::log_configdata(
      TitanLoggerApiSimple::ExecutorConfigdata_reason::processing__failed, NULL);
  }

  if (success) {
    send_configure_ack();
    TTCN_Runtime::set_state(to_mtc ? TTCN_Runtime::MTC_IDLE : TTCN_Runtime::HC_ACTIVE);
    TTCN_Logger::log_configdata(
      TitanLoggerApiSimple::ExecutorConfigdata_reason::processing__succeeded);
  } else {
    send_configure_nak();
    TTCN_Runtime::set_state(to_mtc ? TTCN_Runtime::MTC_IDLE : TTCN_Runtime::HC_IDLE);
  }

  incoming_buf.cut_message();
}

void TTCN_Communication::process_create_mtc()
{
  incoming_buf.cut_message();
  TTCN_Runtime::process_create_mtc();
}

void TTCN_Communication::process_create_ptc()
{
  component component_reference = (component)incoming_buf.pull_int().get_val();
  if (component_reference < FIRST_PTC_COMPREF) {
    incoming_buf.cut_message();
    send_error("Message CREATE_PTC refers to invalid "
      "component reference %d.", component_reference);
    return;
  }
  qualified_name component_type;
  incoming_buf.pull_qualified_name(component_type);
  if (component_type.module_name == NULL ||
    component_type.definition_name == NULL) {
    incoming_buf.cut_message();
    delete [] component_type.module_name;
    delete [] component_type.definition_name;
    send_error("Message CREATE_PTC with component reference %d contains "
      "an invalid component type.", component_reference);
    return;
  }
  char *component_name = incoming_buf.pull_string();
  boolean is_alive = incoming_buf.pull_int().get_val();
  qualified_name current_testcase;
  incoming_buf.pull_qualified_name(current_testcase);
  incoming_buf.cut_message();

  try {
    TTCN_Runtime::process_create_ptc(component_reference,
      component_type.module_name, component_type.definition_name,
      component_name, is_alive, current_testcase.module_name,
      current_testcase.definition_name);
  } catch (...) {
    // to prevent from memory leaks
    delete [] component_type.module_name;
    delete [] component_type.definition_name;
    delete [] component_name;
    delete [] current_testcase.module_name;
    delete [] current_testcase.definition_name;
    throw;
  }

  delete [] component_type.module_name;
  delete [] component_type.definition_name;
  delete [] component_name;
  delete [] current_testcase.module_name;
  delete [] current_testcase.definition_name;
}

void TTCN_Communication::process_kill_process()
{
  component component_reference = (component)incoming_buf.pull_int().get_val();
  incoming_buf.cut_message();
  TTCN_Runtime::process_kill_process(component_reference);
}

void TTCN_Communication::process_exit_hc()
{
  incoming_buf.cut_message();
  TTCN_Logger::log_executor_runtime(
    TitanLoggerApiSimple::ExecutorRuntime_reason::exit__requested__from__mc__hc);
  TTCN_Runtime::set_state(TTCN_Runtime::HC_EXIT);
}

void TTCN_Communication::process_create_ack()
{
  component component_reference = incoming_buf.pull_int().get_val();
  incoming_buf.cut_message();
  TTCN_Runtime::process_create_ack(component_reference);
}

void TTCN_Communication::process_start_ack()
{
  incoming_buf.cut_message();

  switch (TTCN_Runtime::get_state()) {
  case TTCN_Runtime::MTC_START:
    TTCN_Runtime::set_state(TTCN_Runtime::MTC_TESTCASE);
  case TTCN_Runtime::MTC_TERMINATING_TESTCASE:
    break;
  case TTCN_Runtime::PTC_START:
    TTCN_Runtime::set_state(TTCN_Runtime::PTC_FUNCTION);
    break;
  default:
    TTCN_error("Internal error: Message START_ACK arrived in invalid "
      "state.");
  }
}

void TTCN_Communication::process_stop()
{
  incoming_buf.cut_message();
  switch (TTCN_Runtime::get_state()) {
  case TTCN_Runtime::MTC_IDLE:
    TTCN_Logger::log_executor_runtime(
      TitanLoggerApiSimple::ExecutorRuntime_reason::stop__was__requested__from__mc__ignored__on__idle__mtc);
    break;
  case TTCN_Runtime::MTC_PAUSED:
    TTCN_Logger::log_executor_runtime(
      TitanLoggerApiSimple::ExecutorRuntime_reason::stop__was__requested__from__mc);
    TTCN_Runtime::set_state(TTCN_Runtime::MTC_TERMINATING_EXECUTION);
    break;
  case TTCN_Runtime::PTC_IDLE:
  case TTCN_Runtime::PTC_STOPPED:
    TTCN_Logger::log_executor_runtime(
      TitanLoggerApiSimple::ExecutorRuntime_reason::stop__was__requested__from__mc__ignored__on__idle__ptc);
    break;
  case TTCN_Runtime::PTC_EXIT:
    // silently ignore
    break;
  default:
    TTCN_Logger::log_executor_runtime(
      TitanLoggerApiSimple::ExecutorRuntime_reason::stop__was__requested__from__mc);
    TTCN_Runtime::stop_execution();
    break;
  }
}

void TTCN_Communication::process_stop_ack()
{
  incoming_buf.cut_message();
  switch (TTCN_Runtime::get_state()) {
  case TTCN_Runtime::MTC_STOP:
    TTCN_Runtime::set_state(TTCN_Runtime::MTC_TESTCASE);
  case TTCN_Runtime::MTC_TERMINATING_TESTCASE:
    break;
  case TTCN_Runtime::PTC_STOP:
    TTCN_Runtime::set_state(TTCN_Runtime::PTC_FUNCTION);
    break;
  default:
    TTCN_error("Internal error: Message STOP_ACK arrived in invalid "
      "state.");
  }
}

void TTCN_Communication::process_kill_ack()
{
  incoming_buf.cut_message();
  switch (TTCN_Runtime::get_state()) {
  case TTCN_Runtime::MTC_KILL:
    TTCN_Runtime::set_state(TTCN_Runtime::MTC_TESTCASE);
  case TTCN_Runtime::MTC_TERMINATING_TESTCASE:
    break;
  case TTCN_Runtime::PTC_KILL:
    TTCN_Runtime::set_state(TTCN_Runtime::PTC_FUNCTION);
    break;
  default:
    TTCN_error("Internal error: Message KILL_ACK arrived in invalid "
      "state.");
  }
}

void TTCN_Communication::process_running()
{
  boolean answer = incoming_buf.pull_int().get_val();
  incoming_buf.cut_message();
  TTCN_Runtime::process_running(answer);
}

void TTCN_Communication::process_alive()
{
  boolean answer = incoming_buf.pull_int().get_val();
  incoming_buf.cut_message();
  TTCN_Runtime::process_alive(answer);
}

void TTCN_Communication::process_done_ack(int msg_end)
{
  // decoding the mandatory attributes
  boolean answer = incoming_buf.pull_int().get_val();
  char *return_type = incoming_buf.pull_string();
  // the return value starts here
  int return_value_begin = incoming_buf.get_pos();

  try {
    TTCN_Runtime::process_done_ack(answer, return_type,
      msg_end - return_value_begin,
      incoming_buf.get_data() + return_value_begin);
  } catch (...) {
    // avoid memory leaks in case of error
    incoming_buf.cut_message();
    delete [] return_type;
    throw;
  }

  incoming_buf.cut_message();
  delete [] return_type;
}

void TTCN_Communication::process_killed_ack()
{
  boolean answer = incoming_buf.pull_int().get_val();
  incoming_buf.cut_message();
  TTCN_Runtime::process_killed_ack(answer);
}

void TTCN_Communication::process_cancel_done_mtc()
{
  component component_reference = incoming_buf.pull_int().get_val();
  boolean cancel_any = incoming_buf.pull_int().get_val();
  incoming_buf.cut_message();
  TTCN_Runtime::cancel_component_done(component_reference);
  if (cancel_any) TTCN_Runtime::cancel_component_done(ANY_COMPREF);
  send_cancel_done_ack(component_reference);
}

void TTCN_Communication::process_cancel_done_ptc()
{
  component component_reference = incoming_buf.pull_int().get_val();
  incoming_buf.cut_message();
  TTCN_Runtime::cancel_component_done(component_reference);
  send_cancel_done_ack(component_reference);
}

void TTCN_Communication::process_component_status_mtc(int msg_end)
{
  // decoding the mandatory attributes
  component component_reference = incoming_buf.pull_int().get_val();
  boolean is_done = incoming_buf.pull_int().get_val();
  boolean is_killed = incoming_buf.pull_int().get_val();
  boolean is_any_done = incoming_buf.pull_int().get_val();
  boolean is_all_done = incoming_buf.pull_int().get_val();
  boolean is_any_killed = incoming_buf.pull_int().get_val();
  boolean is_all_killed = incoming_buf.pull_int().get_val();
  if (is_done) {
    // the return type and value is valid
    char *return_type = incoming_buf.pull_string();
    int return_value_begin = incoming_buf.get_pos();
    try {
      TTCN_Runtime::set_component_done(component_reference, return_type,
        msg_end - return_value_begin,
        incoming_buf.get_data() + return_value_begin);
    } catch (...) {
      // avoid memory leaks
      incoming_buf.cut_message();
      delete [] return_type;
      throw;
    }
    delete [] return_type;
  }
  if (is_killed) TTCN_Runtime::set_component_killed(component_reference);
  if (is_any_done)
    TTCN_Runtime::set_component_done(ANY_COMPREF, NULL, 0, NULL);
  if (is_all_done)
    TTCN_Runtime::set_component_done(ALL_COMPREF, NULL, 0, NULL);
  if (is_any_killed) TTCN_Runtime::set_component_killed(ANY_COMPREF);
  if (is_all_killed) TTCN_Runtime::set_component_killed(ALL_COMPREF);
  incoming_buf.cut_message();
  if (!is_done && !is_killed && (component_reference != NULL_COMPREF ||
    (!is_any_done && !is_all_done && !is_any_killed && !is_all_killed)))
    TTCN_error("Internal error: Malformed COMPONENT_STATUS message was "
      "received.");
}

void TTCN_Communication::process_component_status_ptc(int msg_end)
{
  // decoding the mandatory attributes
  component component_reference = incoming_buf.pull_int().get_val();
  boolean is_done = incoming_buf.pull_int().get_val();
  boolean is_killed = incoming_buf.pull_int().get_val();
  if (is_done) {
    // the return type and value is valid
    char *return_type = incoming_buf.pull_string();
    int return_value_begin = incoming_buf.get_pos();
    try {
      TTCN_Runtime::set_component_done(component_reference, return_type,
        msg_end - return_value_begin,
        incoming_buf.get_data() + return_value_begin);
    } catch (...) {
      // avoid memory leaks
      incoming_buf.cut_message();
      delete [] return_type;
      throw;
    }
    delete [] return_type;
  }
  if (is_killed) TTCN_Runtime::set_component_killed(component_reference);
  incoming_buf.cut_message();
  if (!is_done && !is_killed) TTCN_error("Internal error: Malformed "
    "COMPONENT_STATUS message was received.");
}

void TTCN_Communication::process_connect_listen()
{
  char *local_port = incoming_buf.pull_string();
  component remote_component = incoming_buf.pull_int().get_val();
  char *remote_component_name = incoming_buf.pull_string();
  char *remote_port = incoming_buf.pull_string();
  transport_type_enum transport_type =
    (transport_type_enum)incoming_buf.pull_int().get_val();
  incoming_buf.cut_message();

  try {
    if (remote_component != MTC_COMPREF && self != remote_component)
      COMPONENT::register_component_name(remote_component,
        remote_component_name);
    PORT::process_connect_listen(local_port, remote_component, remote_port,
      transport_type);
  } catch (...) {
    delete [] local_port;
    delete [] remote_component_name;
    delete [] remote_port;
    throw;
  }

  delete [] local_port;
  delete [] remote_component_name;
  delete [] remote_port;
}

void TTCN_Communication::process_connect()
{
  char *local_port = incoming_buf.pull_string();
  component remote_component = incoming_buf.pull_int().get_val();
  char *remote_component_name = incoming_buf.pull_string();
  char *remote_port = incoming_buf.pull_string();
  transport_type_enum transport_type =
    (transport_type_enum)incoming_buf.pull_int().get_val();

  try {
    if (remote_component != MTC_COMPREF && self != remote_component)
      COMPONENT::register_component_name(remote_component,
        remote_component_name);
    PORT::process_connect(local_port, remote_component, remote_port,
      transport_type, incoming_buf);
  } catch (...) {
    incoming_buf.cut_message();
    delete [] local_port;
    delete [] remote_component_name;
    delete [] remote_port;
    throw;
  }

  incoming_buf.cut_message();
  delete [] local_port;
  delete [] remote_component_name;
  delete [] remote_port;
}

void TTCN_Communication::process_connect_ack()
{
  incoming_buf.cut_message();

  switch (TTCN_Runtime::get_state()) {
  case TTCN_Runtime::MTC_CONNECT:
    TTCN_Runtime::set_state(TTCN_Runtime::MTC_TESTCASE);
  case TTCN_Runtime::MTC_TERMINATING_TESTCASE:
    break;
  case TTCN_Runtime::PTC_CONNECT:
    TTCN_Runtime::set_state(TTCN_Runtime::PTC_FUNCTION);
    break;
  default:
    TTCN_error("Internal error: Message CONNECT_ACK arrived in invalid "
      "state.");
  }
}

void TTCN_Communication::process_disconnect()
{
  char *local_port = incoming_buf.pull_string();
  component remote_component = incoming_buf.pull_int().get_val();
  char *remote_port = incoming_buf.pull_string();
  incoming_buf.cut_message();

  try {
    PORT::process_disconnect(local_port, remote_component, remote_port);
  } catch (...) {
    delete [] local_port;
    delete [] remote_port;
    throw;
  }

  delete [] local_port;
  delete [] remote_port;
}

void TTCN_Communication::process_disconnect_ack()
{
  incoming_buf.cut_message();

  switch (TTCN_Runtime::get_state()) {
  case TTCN_Runtime::MTC_DISCONNECT:
    TTCN_Runtime::set_state(TTCN_Runtime::MTC_TESTCASE);
  case TTCN_Runtime::MTC_TERMINATING_TESTCASE:
    break;
  case TTCN_Runtime::PTC_DISCONNECT:
    TTCN_Runtime::set_state(TTCN_Runtime::PTC_FUNCTION);
    break;
  default:
    TTCN_error("Internal error: Message DISCONNECT_ACK arrived in invalid "
      "state.");
  }
}

void TTCN_Communication::process_map()
{
  char *local_port = incoming_buf.pull_string();
  char *system_port = incoming_buf.pull_string();
  incoming_buf.cut_message();

  try {
    PORT::map_port(local_port, system_port);
  } catch (...) {
    delete [] local_port;
    delete [] system_port;
    throw;
  }

  delete [] local_port;
  delete [] system_port;
}

void TTCN_Communication::process_map_ack()
{
  incoming_buf.cut_message();

  switch (TTCN_Runtime::get_state()) {
  case TTCN_Runtime::MTC_MAP:
    TTCN_Runtime::set_state(TTCN_Runtime::MTC_TESTCASE);
  case TTCN_Runtime::MTC_TERMINATING_TESTCASE:
    break;
  case TTCN_Runtime::PTC_MAP:
    TTCN_Runtime::set_state(TTCN_Runtime::PTC_FUNCTION);
    break;
  default:
    TTCN_error("Internal error: Message MAP_ACK arrived in invalid state.");
  }
}

void TTCN_Communication::process_unmap()
{
  char *local_port = incoming_buf.pull_string();
  char *system_port = incoming_buf.pull_string();
  incoming_buf.cut_message();

  try {
    PORT::unmap_port(local_port, system_port);
  } catch (...) {
    delete [] local_port;
    delete [] system_port;
    throw;
  }

  delete [] local_port;
  delete [] system_port;
}

void TTCN_Communication::process_unmap_ack()
{
  incoming_buf.cut_message();

  switch(TTCN_Runtime::get_state()){
  case TTCN_Runtime::MTC_UNMAP:
    TTCN_Runtime::set_state(TTCN_Runtime::MTC_TESTCASE);
  case TTCN_Runtime::MTC_TERMINATING_TESTCASE:
    break;
  case TTCN_Runtime::PTC_UNMAP:
    TTCN_Runtime::set_state(TTCN_Runtime::PTC_FUNCTION);
    break;
  default:
    TTCN_error("Internal error: Message UNMAP_ACK arrived in invalid "
      "state.");
  }
}

void TTCN_Communication::process_execute_control()
{
  char *module_name = incoming_buf.pull_string();
  incoming_buf.cut_message();

  if (TTCN_Runtime::get_state() != TTCN_Runtime::MTC_IDLE) {
    delete [] module_name;
    TTCN_error("Internal error: Message EXECUTE_CONTROL arrived in "
      "invalid state.");
  }

  TTCN_Logger::log(TTCN_Logger::PARALLEL_UNQUALIFIED,
    "Executing control part of module %s.", module_name);

  TTCN_Runtime::set_state(TTCN_Runtime::MTC_CONTROLPART);

  try {
    Module_List::execute_control(module_name);
  } catch (const TC_End& TC_end) {
  } catch (const TC_Error& TC_error) {
  }

  delete [] module_name;

  if (is_connected) {
    send_mtc_ready();
    TTCN_Runtime::set_state(TTCN_Runtime::MTC_IDLE);
  } else TTCN_Runtime::set_state(TTCN_Runtime::MTC_EXIT);
}

void TTCN_Communication::process_execute_testcase()
{
  char *module_name = incoming_buf.pull_string();
  char *testcase_name = incoming_buf.pull_string();
  incoming_buf.cut_message();

  if (TTCN_Runtime::get_state() != TTCN_Runtime::MTC_IDLE) {
    delete [] module_name;
    delete [] testcase_name;
    TTCN_error("Internal error: Message EXECUTE_TESTCASE arrived in "
      "invalid state.");
  }

  TTCN_Logger::log_testcase_exec(testcase_name, module_name);

  TTCN_Runtime::set_state(TTCN_Runtime::MTC_CONTROLPART);

  try {
    if (testcase_name != NULL && testcase_name[0] != '\0')
      Module_List::execute_testcase(module_name, testcase_name);
    else Module_List::execute_all_testcases(module_name);
  } catch (const TC_End& TC_end) {
  } catch (const TC_Error& TC_error) {
  }

  if (is_connected) {
    send_mtc_ready();
    TTCN_Runtime::set_state(TTCN_Runtime::MTC_IDLE);
  } else TTCN_Runtime::set_state(TTCN_Runtime::MTC_EXIT);

  delete [] module_name;
  delete [] testcase_name;
}

void TTCN_Communication::process_ptc_verdict()
{
  TTCN_Runtime::process_ptc_verdict(incoming_buf);
  incoming_buf.cut_message();
}

void TTCN_Communication::process_continue()
{
  incoming_buf.cut_message();

  if (TTCN_Runtime::get_state() != TTCN_Runtime::MTC_PAUSED)
    TTCN_error("Internal error: Message CONTINUE arrived in invalid "
      "state.");

  TTCN_Runtime::set_state(TTCN_Runtime::MTC_CONTROLPART);
}

void TTCN_Communication::process_exit_mtc()
{
  incoming_buf.cut_message();
  TTCN_Runtime::log_verdict_statistics();
  TTCN_Logger::log_executor_runtime(
    TitanLoggerApiSimple::ExecutorRuntime_reason::exit__requested__from__mc__mtc);
  TTCN_Runtime::set_state(TTCN_Runtime::MTC_EXIT);
}

void TTCN_Communication::process_start()
{
  qualified_name function_name;
  incoming_buf.pull_qualified_name(function_name);
  if (function_name.module_name == NULL ||
    function_name.definition_name == NULL) {
    incoming_buf.cut_message();
    delete [] function_name.module_name;
    delete [] function_name.definition_name;
    TTCN_error("Internal error: Message START contains an invalid "
      "function name.");
  }

  try {
    TTCN_Runtime::start_function(function_name.module_name,
      function_name.definition_name, incoming_buf);
  } catch (...) {
    // avoid memory leaks
    delete [] function_name.module_name;
    delete [] function_name.definition_name;
    throw;
  }

  delete [] function_name.module_name;
  delete [] function_name.definition_name;
}

void TTCN_Communication::process_kill()
{
  incoming_buf.cut_message();
  TTCN_Runtime::process_kill();
}

void TTCN_Communication::process_error()
{
  char *error_string = incoming_buf.pull_string();
  incoming_buf.cut_message();

  try {
    TTCN_error("Error message was received from MC: %s", error_string);
  } catch (...) {
    delete [] error_string;
    throw;
  }
}

void TTCN_Communication::process_unsupported_message(int msg_type, int msg_end)
{
  TTCN_Logger::begin_event(TTCN_Logger::WARNING_UNQUALIFIED);
  TTCN_Logger::log_event("Unsupported message was received from MC: "
    "type (decimal): %d, data (hexadecimal): ", msg_type);
  const unsigned char *msg_ptr =
    (const unsigned char*)incoming_buf.get_data();
  for (int i = incoming_buf.get_pos(); i < msg_end; i++)
    TTCN_Logger::log_octet(msg_ptr[i]);
  TTCN_Logger::end_event();
  incoming_buf.cut_message();
}

void TTCN_Communication::process_debug_command()
{
  int command = incoming_buf.pull_int().get_val();
  int argument_count = incoming_buf.pull_int().get_val();
  char** arguments = NULL;
  if (argument_count > 0) {
    arguments = new char*[argument_count];
    for (int i = 0; i < argument_count; ++i) {
      arguments[i] = incoming_buf.pull_string();
    }
  }
  incoming_buf.cut_message();
  ttcn3_debugger.execute_command(command, argument_count, arguments);
  if (argument_count > 0) {
    for (int i = 0; i < argument_count; ++i) {
      delete [] arguments[i];
    }
    delete [] arguments;
  }
}

/* * * * Temporary squatting place because it includes version.h * * * */

const struct runtime_version current_runtime_version = {
  TTCN3_MAJOR, TTCN3_MINOR, TTCN3_PATCHLEVEL, TITAN_RUNTIME_NR
};

static const char *runtime_name[] = { 0, "load", "function " };

RuntimeVersionChecker::RuntimeVersionChecker(
  int ver_major, int ver_minor, int patch_level, int rt)
{
  if ( TTCN3_MAJOR != ver_major
    || TTCN3_MINOR != ver_minor
    || TTCN3_PATCHLEVEL != patch_level)
  {
    TTCN_error(
      "Version mismatch detected: generated code %d.%d.pl%d, "
      "runtime is %d.%d.pl%d",
      ver_major, ver_minor, patch_level,
      TTCN3_MAJOR, TTCN3_MINOR, TTCN3_PATCHLEVEL);
  }

  if (TITAN_RUNTIME_NR != rt) {
    TTCN_error("Runtime mismatch detected: files compiled for the %stest"
      " runtime cannot be linked to %stest library",
      runtime_name[TITAN_RUNTIME_NR], runtime_name[rt]);
  }
}

reffer::reffer(const char*) {}

