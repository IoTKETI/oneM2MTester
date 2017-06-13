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
 *   Bene, Tamas
 *   Czimbalmos, Eduard
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
//
// Description:           Header file for MainController
// Author:                Janos Zoltan Szabo
// mail:                  tmpjsz@eth.ericsson.se
//
// Copyright (c) 2000-2017 Ericsson Telecom AB
//
#ifndef MCTR_MAINCONTROLLER_H
#define MCTR_MAINCONTROLLER_H
//----------------------------------------------------------------------------

#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "../../core/Types.h"
#include "../../common/NetworkHandler.hh"
class Text_Buf;

#include "UserInterface.h"

#ifdef USE_EPOLL
struct epoll_event;
#else
struct pollfd;
#endif

struct sigaction;

//----------------------------------------------------------------------------

namespace mctr {

//----------------------------------------------------------------------------

/* Type definitions */

/** For representing the global state of MC */
enum mc_state_enum {
  MC_INACTIVE, MC_LISTENING, MC_LISTENING_CONFIGURED, MC_HC_CONNECTED,
  MC_CONFIGURING, MC_ACTIVE, MC_SHUTDOWN, MC_CREATING_MTC, MC_READY,
  MC_TERMINATING_MTC, MC_EXECUTING_CONTROL, MC_EXECUTING_TESTCASE,
  MC_TERMINATING_TESTCASE, MC_PAUSED, MC_RECONFIGURING
};

/** Data structure for unknown incoming connections (before receiving
 *  the first message) */
struct unknown_connection {
  int fd;
  IPAddress *ip_addr;
  Text_Buf *text_buf;
  unknown_connection *prev, *next;
  bool unix_socket; // true only if the connection is through unix domain socket
};

/** Data structure for describing the component location
 *  constraints */
struct string_set {
  int n_elements;
  char **elements;
};

/** Data structure for describing the component location
 *  constraints */
struct host_group_struct {
  char *group_name;
  boolean has_all_hosts, has_all_components;
  string_set host_members, assigned_components;
};

/** Possible states of a HC */
enum hc_state_enum { HC_IDLE, HC_CONFIGURING, HC_ACTIVE, HC_OVERLOADED,
  HC_CONFIGURING_OVERLOADED, HC_EXITING, HC_DOWN };

/** Data structure for each host (and the corresponding HC) */
struct host_struct {
  IPAddress *ip_addr;
  char *hostname; /**< hostname retrieved from DNS */
  char *hostname_local; /**< hostname sent in VERSION message */
  char *machine_type;
  char *system_name;
  char *system_release;
  char *system_version;
  boolean transport_supported[TRANSPORT_NUM];
  char *log_source;
  hc_state_enum hc_state;
  int hc_fd;
  Text_Buf *text_buf;
  int n_components;
  component *components;
  /* to implement load balancing mechanisms */
  string_set allowed_components;
  boolean all_components_allowed;
  boolean local_hostname_different;
  int n_active_components;
};

struct component_struct;

/** Container of test components (when a pending operation can be
 *  requested by several components) */
struct requestor_struct {
  int n_components;
  union {
    component_struct *the_component;
    component_struct **components;
  };
};

/** Possible states of a port connection or mapping */
enum conn_state_enum { CONN_LISTENING, CONN_CONNECTING, CONN_CONNECTED,
  CONN_DISCONNECTING, CONN_MAPPING, CONN_MAPPED, CONN_UNMAPPING };

/** Data structure for representing a port connection */
struct port_connection {
  conn_state_enum conn_state;
  transport_type_enum transport_type;
  struct {
    component comp_ref;
    char *port_name;
    port_connection *next, *prev;
  } head, tail;
  requestor_struct requestors;
};


/** Structure for timers */
struct timer_struct {
  double expiration;
  union {
    void *dummy_ptr;
    component_struct *component_ptr;
  } timer_argument;
  timer_struct *prev, *next;
};

/** Possible states of a TC (MTC or PTC) */
enum tc_state_enum { TC_INITIAL, TC_IDLE, TC_CREATE, TC_START, TC_STOP, TC_KILL,
  TC_CONNECT, TC_DISCONNECT, TC_MAP, TC_UNMAP, TC_STOPPING, TC_EXITING,
  TC_EXITED,
  MTC_CONTROLPART, MTC_TESTCASE, MTC_ALL_COMPONENT_STOP,
  MTC_ALL_COMPONENT_KILL, MTC_TERMINATING_TESTCASE, MTC_PAUSED,
  PTC_FUNCTION, PTC_STARTING, PTC_STOPPED, PTC_KILLING, PTC_STOPPING_KILLING,
  PTC_STALE, TC_SYSTEM, MTC_CONFIGURING };

/** Data structure for each TC */
struct component_struct {
  component comp_ref;
  qualified_name comp_type;
  char *comp_name;
  char *log_source; /**< used for console log messages. format: name\@host */
  host_struct *comp_location;
  tc_state_enum tc_state;
  verdicttype local_verdict;
  char* verdict_reason;
  int tc_fd;
  Text_Buf *text_buf;
  /** Identifier of the TTCN-3 testcase or function that is currently being
   * executed on the test component */
  qualified_name tc_fn_name;
  /* fields for implementing the construct 'value returning done' */
  char *return_type;
  int return_value_len;
  void *return_value;
  boolean is_alive;
  boolean stop_requested; /**< only for 'all component.running' */
  boolean process_killed;
  union {
    /** used in state TC_INITIAL */
    struct {
      component_struct *create_requestor;
      char *location_str;
    } initial;
    /** used in state PTC_STARTING */
    struct {
      component_struct *start_requestor;
      int arguments_len;
      void *arguments_ptr;
      requestor_struct cancel_done_sent_to;
    } starting;
    /** used in states TC_STOPPING, PTC_STOPPING_KILLING, PTC_KILLING */
    struct {
      requestor_struct stop_requestors;
      requestor_struct kill_requestors;
    } stopping_killing;
  };
  requestor_struct done_requestors;
  requestor_struct killed_requestors;
  requestor_struct cancel_done_sent_for;
  timer_struct *kill_timer;
  /* fields for registering port connections */
  port_connection *conn_head_list, *conn_tail_list;
  int conn_head_count, conn_tail_count;
};

/** Selector for the table of file descriptors */
enum fd_type_enum { FD_UNUSED, FD_PIPE, FD_SERVER, FD_UNKNOWN, FD_HC, FD_TC };

/** Element of the file descriptor table. The table is indexed by the
 *  fd itself. */
struct fd_table_struct {
  fd_type_enum fd_type;
  union {
    unknown_connection *unknown_ptr;
    host_struct *host_ptr;
    component_struct *component_ptr;
    void *dummy_ptr;
  };
};

/** Structure for storing the checksum of a module */
struct module_version_info {
  char *module_name;
  int checksum_length;
  unsigned char *module_checksum;
};

/** Possible reasons for waking up the MC thread from the main thread. */
enum wakeup_reason_t { REASON_NOTHING, REASON_SHUTDOWN, REASON_MTC_KILL_TIMER };

/** Structure for storing the settings needed to initialize the debugger of a
  * newly connected HC */
struct debugger_settings_struct {
  char* on_switch;
  char* output_type;
  char* output_file;
  char* error_behavior;
  char* error_batch_file;
  char* fail_behavior;
  char* fail_batch_file;
  char* global_batch_state;
  char* global_batch_file;
  char* function_calls_cfg;
  char* function_calls_file;
  int nof_breakpoints;
  struct breakpoint_struct {
    char* module;
    char* line;
    char* batch_file;
  }* breakpoints;
};

struct debug_command_struct {
  int command;
  char* arguments;
};

/** The MainController class. The collection of all functions and data
 *   structures */
class MainController {
  /* private members */
  static UserInterface *ui;
  static NetworkHandler nh;

  static mc_state_enum mc_state;
  static char *mc_hostname;

  static int server_fd;
  static int server_fd_unix; // for efficient local communication
  static boolean server_fd_disabled;
  static void disable_server_fd();
  static void enable_server_fd();

  static pthread_mutex_t mutex;
  static void lock();
  static void unlock();

#ifdef USE_EPOLL
  static const int EPOLL_SIZE_HINT = 1000;
  static const int EPOLL_MAX_EVENTS = 250;
  static epoll_event *epoll_events;
  static int epfd;
#else
  static unsigned int nfds, new_nfds;
  static struct pollfd *ufds, *new_ufds;
  static boolean pollfds_modified;
  static void update_pollfds();
#endif
  static void add_poll_fd(int fd);
  static void remove_poll_fd(int fd);

  static int fd_table_size;
  static fd_table_struct *fd_table;
  static void add_fd_to_table(int fd);
  static void remove_fd_from_table(int fd);

  static void set_close_on_exec(int fd);

  static unknown_connection *unknown_head, *unknown_tail;
  static unknown_connection *new_unknown_connection(bool unix_socket);
  static void delete_unknown_connection(unknown_connection *conn);
  static void close_unknown_connection(unknown_connection *conn);

  static void init_string_set(string_set *set);
  static void free_string_set(string_set *set);
  static void add_string_to_set(string_set *set, const char *str);
  static void remove_string_from_set(string_set *set, const char *str);
  static boolean set_has_string(const string_set *set, const char *str);
  static const char *get_string_from_set(const string_set *set, int index);

  static int n_host_groups;
  static host_group_struct *host_groups;
  static string_set assigned_components;
  static boolean all_components_assigned;
  static host_group_struct *add_host_group(const char *group_name);
  static host_group_struct *lookup_host_group(const char *group_name);
  static boolean is_similar_hostname(const char *host1, const char *host2);
  static boolean host_has_name(const host_struct *host, const char *name);
  static boolean member_of_group(const host_struct *host,
    const host_group_struct *group);
  static void add_allowed_components(host_struct *host);
  static host_struct *choose_ptc_location(const char *component_type,
    const char *component_name, const char *component_location);

  static int n_hosts;
  static host_struct **hosts;
  static char *config_str;
  static debugger_settings_struct debugger_settings;
  static debug_command_struct last_debug_command;
  static host_struct *add_new_host(unknown_connection *conn);
  static void close_hc_connection(host_struct *hc);
  static boolean is_hc_in_state(hc_state_enum checked_state);
  static boolean all_hc_in_state(hc_state_enum checked_state);
  static void configure_host(host_struct *host, boolean should_notify);
  static void configure_mtc();
  static void check_all_hc_configured();
  static void add_component_to_host(host_struct *host,
    component_struct *comp);
  static void remove_component_from_host(component_struct *comp);

  static boolean version_known;
  static int n_modules;
  static module_version_info *modules;
  static boolean check_version(unknown_connection *conn);

  static int n_components, n_active_ptcs, max_ptcs;
  static component_struct **components;
  static component_struct *mtc, *system;
  static const component_struct* debugger_active_tc;
  static component next_comp_ref, tc_first_comp_ref;
  static boolean any_component_done_requested, any_component_done_sent,
  all_component_done_requested, any_component_killed_requested,
  all_component_killed_requested;
  static void add_component(component_struct *comp);
  static component_struct *lookup_component(component comp_ref);
  static void destroy_all_components();
  static void close_tc_connection(component_struct *comp);
  static boolean stop_after_tc, stop_requested;
  static boolean ready_to_finish_testcase();
  static void finish_testcase();
  static boolean message_expected(component_struct *from,
    const char *message_name);
  static boolean request_allowed(component_struct *from,
    const char *message_name);
  static boolean valid_endpoint(component component_reference,
    boolean new_connection, component_struct *requestor,
    const char *operation);
  static void destroy_connection(port_connection *conn, component_struct *tc);
  static void destroy_mapping(port_connection *conn);
  static boolean stop_all_components();
  static void check_all_component_stop();
  static void send_stop_ack_to_requestors(component_struct *tc);
  static boolean kill_all_components(boolean testcase_ends);
  static void check_all_component_kill();
  static void send_kill_ack_to_requestors(component_struct *tc);
  static void send_component_status_to_requestor(component_struct *tc,
    component_struct *requestor, boolean done_status,
    boolean killed_status);
  static void component_stopped(component_struct *tc);
  static void component_terminated(component_struct *tc);
  static void done_cancelled(component_struct *from,
    component_struct *started_tc);
  static void start_kill_timer(component_struct *tc);

  static boolean component_is_alive(component_struct *tc);
  static boolean component_is_running(component_struct *tc);
  static boolean component_is_done(component_struct *tc);
  static boolean is_any_component_alive();
  static boolean is_all_component_alive();
  static boolean is_any_component_running();
  static boolean is_all_component_running();
  static boolean is_any_component_done();

  static void init_connections(component_struct *tc);
  static void add_connection(port_connection *c);
  static void remove_connection(port_connection *c);
  static port_connection *find_connection(component head_comp,
    const char *head_port, component tail_comp, const char *tail_port);
  static void remove_all_connections(component head_or_tail);
  static transport_type_enum choose_port_connection_transport(
    component head_comp, component tail_comp);
  static void send_connect_ack_to_requestors(port_connection *conn);
  static void send_error_to_connect_requestors(port_connection *conn,
    const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));
  static void send_disconnect_to_server(port_connection *conn);
  static void send_disconnect_ack_to_requestors(port_connection *conn);

  static void init_requestors(requestor_struct *reqs, component_struct *tc);
  static void add_requestor(requestor_struct *reqs, component_struct *tc);
  static void remove_requestor(requestor_struct *reqs, component_struct *tc);
  static boolean has_requestor(const requestor_struct *reqs,
    component_struct *tc);
  static component_struct *get_requestor(const requestor_struct *reqs,
    int index);
  static void free_requestors(requestor_struct *reqs);

  static void init_qualified_name(qualified_name *name);
  static void free_qualified_name(qualified_name *name);

  static double kill_timer;
  static double time_now();
  static timer_struct *timer_head, *timer_tail;
  static void register_timer(timer_struct *timer);
  static void cancel_timer(timer_struct *timer);
  static int get_poll_timeout();
  static void handle_expired_timers();
  static void handle_kill_timer(timer_struct *timer);

  // Custom signal handling for termination signals to remove temporary
  // files /tmp/ttcn3-mctr-*. Related to HP67376.
  static struct sigaction new_action, old_action;
  static void register_termination_handlers();
  static void termination_handler(int signum);
  
  static void execute_batch_file(const char* file_name);

public:
  static void error(const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
private:
  static void notify(const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
  static void notify(const struct timeval *timestamp, const char *source,
    int severity, const char *message);
  static void status_change();

  static void fatal_error(const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 1, 2), __noreturn__));

  static void *thread_main(void *arg);
  static void dispatch_socket_event(int fd);
  static int pipe_fd[2];
  static wakeup_reason_t wakeup_reason;
  static void wakeup_thread(wakeup_reason_t reason);

  static void handle_pipe();
  static void handle_incoming_connection(int p_serverfd);
  static int recv_to_buffer(int fd, Text_Buf& text_buf,
    boolean recv_from_socket);
  static void handle_unknown_data(unknown_connection *conn);
  static void handle_hc_data(host_struct *hc, boolean recv_from_socket);
  static void handle_tc_data(component_struct *tc, boolean recv_from_socket);

  static void unlink_unix_socket(int socket_fd);

  static void shutdown_server();
  static void perform_shutdown();

  static void clean_up();

  static const char *get_host_name(const struct in_addr *ip_address);
  static boolean get_ip_address(struct in_addr *ip_address,
    const char *host_name);

  /* Messages to HCs */
  static void send_configure(host_struct *hc, const char *config_file);
  static void send_exit_hc(host_struct *hc);
  static void send_create_mtc(host_struct *hc);
  static void send_create_ptc(host_struct *hc, component component_reference,
    const qualified_name& component_type, const char *component_name,
    boolean is_alive, const qualified_name& current_testcase);
  static void send_kill_process(host_struct *hc,
    component component_reference);

  /* Messages to TCs */
  static void send_create_ack(component_struct *tc,
    component component_reference);
  static void send_start_ack(component_struct *tc);
  static void send_stop(component_struct *tc);
  static void send_stop_ack(component_struct *tc);
  static void send_kill_ack(component_struct *tc);
  static void send_running(component_struct *tc, boolean answer);
  static void send_alive(component_struct *tc, boolean answer);
  static void send_done_ack(component_struct *tc, boolean answer,
    const char *return_type, int return_value_len,
    const void *return_value);
  static void send_killed_ack(component_struct *tc, boolean answer);
  static void send_connect_listen(component_struct *tc,
    const char *local_port, component remote_comp,
    const char *remote_comp_name, const char *remote_port,
    transport_type_enum transport_type);
  static void send_connect(component_struct *tc,
    const char *local_port, component remote_comp,
    const char *remote_comp_name, const char *remote_port,
    transport_type_enum transport_type, int remote_address_len,
    const void *remote_address);
  static void send_connect_ack(component_struct *tc);
  static void send_disconnect(component_struct *tc,
    const char *local_port, component remote_comp, const char *remote_port);
  static void send_disconnect_ack(component_struct *tc);
  static void send_map(component_struct *tc,
    const char *local_port, const char *system_port);
  static void send_map_ack(component_struct *tc);
  static void send_unmap(component_struct *tc,
    const char *local_port, const char *system_port);
  static void send_unmap_ack(component_struct *tc);
  static void send_debug_command(int fd, int commandID, const char* arguments);
  static void send_debug_setup(host_struct *hc);

  /* Messages to MTC */
  static void send_cancel_done_mtc(component component_reference,
    boolean cancel_any);
  static void send_component_status_mtc(component component_reference,
    boolean is_done, boolean is_killed, boolean is_any_done,
    boolean is_all_done, boolean is_any_killed, boolean is_all_killed,
    const char *return_type, int return_value_len,
    const void *return_value);
  static void send_execute_control(const char *module_name);
  static void send_execute_testcase(const char *module_name,
    const char *testcase_name);
  static void send_ptc_verdict(boolean continue_execution);
  static void send_continue();
  static void send_exit_mtc();
  static void send_configure_mtc(const char *config_file);

  /** Messages to PTCs */
  static void send_cancel_done_ptc(component_struct *tc,
    component component_reference);
  static void send_component_status_ptc(component_struct *tc,
    component component_reference,
    boolean is_done, boolean is_killed, const char *return_type,
    int return_value_len, const void *return_value);
  static void send_start(component_struct *tc,
    const qualified_name& function_name, int arg_len, const void *arg_ptr);
  static void send_kill(component_struct *tc);

  static void send_error(int fd, const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));
  static void send_error_str(int fd, const char *reason);
  static void send_message(int fd, Text_Buf& text_buf);

  /* Incoming messages on unknown connections (generic and first messages) */
  static void process_error(unknown_connection *conn);
  static void process_log(unknown_connection *conn);
  static void process_version(unknown_connection *conn);
  static void process_mtc_created(unknown_connection *conn);
  static void process_ptc_created(unknown_connection *conn);

  /* Incoming messages from HCs */
  static void process_error(host_struct *hc);
  static void process_log(host_struct *hc);
  static void process_configure_ack(host_struct *hc);
  static void process_configure_nak(host_struct *hc);
  static void process_create_nak(host_struct *hc);
  static void process_hc_ready(host_struct *hc);

  /* Incoming messages from TCs */
  static void process_error(component_struct *tc);
  static void process_log(component_struct *tc);
  static void process_create_req(component_struct *tc);
  static void process_start_req(component_struct *tc, int message_end);
  static void process_stop_req(component_struct *tc);
  static void process_kill_req(component_struct *tc);
  static void process_is_running(component_struct *tc);
  static void process_is_alive(component_struct *tc);
  static void process_done_req(component_struct *tc);
  static void process_killed_req(component_struct *tc);
  static void process_cancel_done_ack(component_struct *tc);
  static void process_connect_req(component_struct *tc);
  static void process_connect_listen_ack(component_struct *tc, int message_end);
  static void process_connected(component_struct *tc);
  static void process_connect_error(component_struct *tc);
  static void process_disconnect_req(component_struct *tc);
  static void process_disconnected(component_struct *tc);
  static void process_map_req(component_struct *tc);
  static void process_mapped(component_struct *tc);
  static void process_unmap_req(component_struct *tc);
  static void process_unmapped(component_struct *tc);
  static void process_debug_return_value(Text_Buf& text_buf, char* log_source,
    int msg_end, bool from_mtc);
  static void process_debug_broadcast_req(component_struct *tc, int commandID);
  static void process_debug_batch(component_struct *tc);

  /* Incoming messages from MTC */
  static void process_testcase_started();
  static void process_testcase_finished();
  static void process_mtc_ready();
  static void process_configure_ack_mtc();
  static void process_configure_nak_mtc();

  /* Incoming messages from PTCs */
  static void process_stopped(component_struct *tc, int message_end);
  static void process_stopped_killed(component_struct *tc, int message_end);
  static void process_killed(component_struct *tc);

public:
  static void initialize(UserInterface& par_ui, int par_max_ptcs);
  static void terminate();

  static void add_host(const char *group_name, const char *host_name);
  static void assign_component(const char *host_or_group,
    const char *component_id);
  static void destroy_host_groups();

  static void set_kill_timer(double timer_val);

  static unsigned short start_session(const char *local_address,
    unsigned short tcp_port, bool unix_sockets_enabled);
  static void shutdown_session();

  static void configure(const char *config_file);
  static bool start_reconfiguring();

  static void create_mtc(int host_index);
  static void exit_mtc();

  static void execute_control(const char *module_name);
  static void execute_testcase(const char *module_name,
    const char *testcase_name);
  static void stop_after_testcase(boolean new_state);
  static void continue_testcase();
  static void stop_execution();
  
  static void debug_command(int commandID, char* arguments);

  static mc_state_enum get_state();
  static boolean get_stop_after_testcase();

  static int get_nof_hosts();
  static host_struct *get_host_data(int host_index);
  static component_struct *get_component_data(int component_reference);
  static void release_data();

  static const char *get_mc_state_name(mc_state_enum state);
  static const char *get_hc_state_name(hc_state_enum state);
  static const char *get_tc_state_name(tc_state_enum state);
  static const char *get_transport_name(transport_type_enum transport);
};

//----------------------------------------------------------------------------

} /* namespace mctr */

//----------------------------------------------------------------------------
#endif // MCTR_MAINCONTROLLER_H

// Local Variables:
// mode: C++
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
