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
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef COMMUNICATION_HH
#define COMMUNICATION_HH

#include <sys/types.h>
struct in_addr;
struct sockaddr_in;
struct sockaddr_un;

#include <time.h>

#include "Types.h"
#include "Textbuf.hh"
#include "NetworkHandler.hh"

class MC_Connection;

class TTCN_Communication {
  static int mc_fd;
  static HCNetworkHandler hcnh;
  static boolean local_addr_set, mc_addr_set, is_connected;
  static Text_Buf incoming_buf;
  static MC_Connection mc_connection;
  static double call_interval;

public:
  static const NetworkFamily& get_network_family() { return hcnh.get_family(); }
  static boolean has_local_address() { return local_addr_set; }
  static void set_local_address(const char *host_name);
  static const IPAddress *get_local_address();
  static void set_mc_address(const char *host_name,
    unsigned short tcp_port);
  static const IPAddress *get_mc_address();
  static boolean is_mc_connected();
  static void connect_mc();
  static void disconnect_mc();
  static void close_mc_connection();

  static boolean transport_unix_stream_supported();

  static boolean set_close_on_exec(int fd);
  static boolean set_non_blocking_mode(int fd, boolean enable_nonblock);

  static boolean set_tcp_nodelay(int fd);
  static boolean increase_send_buffer(int fd, int &old_size,
    int& new_size);

  static void enable_periodic_call();
  static void increase_call_interval();
  static void disable_periodic_call();

  static void process_all_messages_hc();
  static void process_all_messages_tc();
  static void process_debug_messages();

  static void send_version();
  static void send_configure_ack();
  static void send_configure_nak();
  static void send_create_nak(component component_reference,
    const char *fmt_str, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));

  static void send_hc_ready();

  static void send_create_req(const char *component_type_module,
    const char *component_type_name,
    const char *component_name,
    const char *component_location, boolean is_alive);
  static void prepare_start_req(Text_Buf& text_buf,
    component component_reference, const char *module_name,
    const char *function_name);
  static void send_stop_req(component component_reference);
  static void send_kill_req(component component_reference);
  static void send_is_running(component component_reference);
  static void send_is_alive(component component_reference);
  static void send_done_req(component component_reference);
  static void send_killed_req(component component_reference);
  static void send_cancel_done_ack(component component_reference);
  static void send_connect_req(component src_component,
    const char *src_port, component dst_component,
    const char *dst_port);
  static void send_connect_listen_ack_inet_stream(const char *local_port,
    component remote_component, const char *remote_port,
    const IPAddress *local_address);

  static void send_connect_listen_ack_unix_stream(const char *local_port,
    component remote_component, const char *remote_port,
    const struct sockaddr_un *local_address);

  static void send_connected(const char *local_port,
    component remote_component, const char *remote_port);
  static void send_connect_error(const char *local_port,
    component remote_component, const char *remote_port,
    const char *fmt_str, ...)
  __attribute__ ((__format__ (__printf__, 4, 5)));
  static void send_disconnect_req(component src_component,
    const char *src_port, component dst_component,
    const char *dst_port);
  static void send_disconnected(const char *local_port,
    component remote_component, const char *remote_port);
  static void send_map_req(component src_component, const char *src_port,
    const char *system_port);
  static void send_mapped(const char *local_port,
    const char *system_port);
  static void send_unmap_req(component src_component,
    const char *src_port, const char *system_port);
  static void send_unmapped(const char *local_port,
    const char *system_port);

  static void send_mtc_created();
  static void send_testcase_started(const char *testcase_module,
    const char *testcase_name, const char *mtc_comptype_module,
    const char *mtc_comptype_name,
    const char *system_comptype_module,
    const char *system_comptype_name);
  static void send_testcase_finished(verdicttype final_verdict,
    const char* reason = "");
  static void send_mtc_ready();

  static void send_ptc_created(component component_reference);
  static void prepare_stopped(Text_Buf& text_buf,
    const char *return_type);
  static void send_stopped();
  static void prepare_stopped_killed(Text_Buf& text_buf,
    verdicttype final_verdict, const char *return_type,
    const char* reason = "");
  static void send_stopped_killed(verdicttype final_verdict,
    const char* reason = "");
  static void send_killed(verdicttype final_verdict, const char* reason = "");
  
  static void send_debug_return_value(int return_type, const char* message);
  static void send_debug_halt_req();
  static void send_debug_continue_req();
  static void send_debug_batch(const char* batch_file);

  /** @brief Send a log message to the MC.

  @param timestamp_sec integral part of timestamp (seconds since 1970)
  @param timestamp_usec fractional part of timestamp
  @param event_severity a TTCN_Logger::Severity value converted to an integer
  @param message_text_len length of message string
  @param message_text the message itself (does not need to be 0-terminated)

  If connected, constructs a Text_Buf and calls send_message().

  @return TRUE if sending the message appears to be successful or
  the message doesn't need to be logged to the console.
  @return FALSE if sending appears to fail or the message should be logged
  to the console in any case.
  */
  static boolean send_log(time_t timestamp_sec, long timestamp_usec,
    unsigned int event_severity, size_t message_text_len,
    const char *message_text);

  /// Constructs a Text_Buf and calls send_message().
  static void send_error(const char *fmt_str, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

  static void send_message(Text_Buf& text_buf);

private:
  /** @name Handlers of various messages
  *   @{
  */
  static void process_configure(int msg_end, boolean to_mtc);
  static void process_create_mtc();
  static void process_create_ptc();
  static void process_kill_process();
  static void process_exit_hc();

  static void process_create_ack();
  static void process_start_ack();
  static void process_stop();
  static void process_stop_ack();
  static void process_kill_ack();
  static void process_running();
  static void process_alive();
  static void process_done_ack(int msg_end);
  static void process_killed_ack();
  static void process_cancel_done_mtc();
  static void process_cancel_done_ptc();
  static void process_component_status_mtc(int msg_end);
  static void process_component_status_ptc(int msg_end);
  static void process_connect_listen();
  static void process_connect();
  static void process_connect_ack();
  static void process_disconnect();
  static void process_disconnect_ack();
  static void process_map();
  static void process_map_ack();
  static void process_unmap();
  static void process_unmap_ack();

  static void process_execute_control();
  static void process_execute_testcase();
  static void process_ptc_verdict();
  static void process_continue();
  static void process_exit_mtc();

  static void process_start();
  static void process_kill();

  static void process_error();
  static void process_unsupported_message(int msg_type, int msg_end);
  
  static void process_debug_command();
  /** @} */
};

#endif
