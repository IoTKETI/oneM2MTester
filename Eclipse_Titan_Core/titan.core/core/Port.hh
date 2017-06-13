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
 *   Feher, Csaba
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#ifndef PORT_HH
#define PORT_HH

#include <sys/types.h>

#include "Types.h"
#include "Event_Handler.hh"
#include <stddef.h> // only for NULL
#include <sys/select.h>

class COMPONENT;
class COMPONENT_template;
class Text_Buf;
class OCTETSTRING;
class CHARSTRING;
class Index_Redirect;

extern const COMPONENT_template& any_compref;

struct port_connection; // no user serviceable parts inside

/** Base class for all test ports */
class PORT : public Fd_And_Timeout_Event_Handler {
  friend class PORT_LIST;
  friend struct port_connection;

  static PORT *list_head, *list_tail;

  void add_to_list();
  void remove_from_list();
  static PORT *lookup_by_name(const char *par_port_name);

  struct port_parameter; // no user serviceable parts inside
  static port_parameter *parameter_head, *parameter_tail;

  /** @brief Apply the given port parameter.
  *
  *  Applies the parameter to the appropriate port, or all ports if it's
  *  a parameter for all ports ( "*" )
  *  @param par_ptr pointer to the port parameter
  */
  static void apply_parameter(port_parameter *par_ptr);
  void set_system_parameters(const char *system_port);

public:
  struct msg_queue_item_base {
    struct msg_queue_item_base *next_item;
  };
  msg_queue_item_base *msg_queue_head, *msg_queue_tail;
public:
  /** @brief Store a port parameter
  *
  *  @param component_id    component identifier
  *  @param par_port_name   string, name of the port, NULL if the parameter
  *                         refers to all ports ( "*" )
  *  @param parameter_name  string, name of the parameter
  *  @param parameter_value string, the value
  */
  static void add_parameter(const component_id_t& component_id,
    const char *par_port_name, const char *parameter_name,
    const char *parameter_value);
  /** Deallocates the list of port parameters */
  static void clear_parameters();
  /** @brief Apply port parameters to component

  Iterates through all known port parameters and

  @param component_reference
  @param component_name
  */
  static void set_parameters(component component_reference,
    const char *component_name);

protected:
  const char *port_name;
  unsigned int msg_head_count, msg_tail_count, proc_head_count,
  proc_tail_count;
  boolean is_active, is_started, is_halted;
  
private:
  int n_system_mappings;
  char **system_mappings;
  PORT *list_prev, *list_next;
  port_connection *connection_list_head, *connection_list_tail;

private:
  /// Copy constructor disabled.
  PORT(const PORT& other_port);
  /// Assignment disabled.
  PORT& operator=(const PORT& other_port);

public:
  PORT(const char *par_port_name);
  virtual ~PORT();

  inline const char *get_name() const { return port_name; }
  void set_name(const char * name);

  virtual void log() const;

  void activate_port();
  void deactivate_port();
  static void deactivate_all();

  void clear();
  static void all_clear();
  void start();
  static void all_start();
  void stop();
  static void all_stop();
  void halt();
  static void all_halt();
  
  boolean check_port_state(const CHARSTRING& type) const;
  static boolean any_check_port_state(const CHARSTRING& type);
  static boolean all_check_port_state(const CHARSTRING& type);
  
  // Used by the setstate operation through TTCN_Runtime
  virtual void change_port_state(translation_port_state state);

  virtual alt_status receive(const COMPONENT_template& sender_template =
    any_compref, COMPONENT *sender_ptr = NULL,
    Index_Redirect* index_redirect = NULL);
  static alt_status any_receive(const COMPONENT_template& sender_template =
    any_compref, COMPONENT *sender_ptr = NULL);
  virtual alt_status check_receive(const COMPONENT_template&
    sender_template = any_compref, COMPONENT *sender_ptr = NULL,
    Index_Redirect* index_redirect = NULL);
  static alt_status any_check_receive(const COMPONENT_template&
    sender_template = any_compref, COMPONENT *sender_ptr = NULL);

  virtual alt_status trigger(const COMPONENT_template& sender_template =
    any_compref, COMPONENT *sender_ptr = NULL,
    Index_Redirect* index_redirect = NULL);
  static alt_status any_trigger(const COMPONENT_template& sender_template =
    any_compref, COMPONENT *sender_ptr = NULL);

  virtual alt_status getcall(const COMPONENT_template& sender_template =
    any_compref, COMPONENT *sender_ptr = NULL,
    Index_Redirect* index_redirect = NULL);
  static alt_status any_getcall(const COMPONENT_template& sender_template =
    any_compref, COMPONENT *sender_ptr = NULL);
  virtual alt_status check_getcall(const COMPONENT_template&
    sender_template = any_compref, COMPONENT *sender_ptr = NULL,
    Index_Redirect* index_redirect = NULL);
  static alt_status any_check_getcall(const COMPONENT_template&
    sender_template = any_compref, COMPONENT *sender_ptr = NULL);

  virtual alt_status getreply(const COMPONENT_template& sender_template =
    any_compref, COMPONENT *sender_ptr = NULL,
    Index_Redirect* index_redirect = NULL);
  static alt_status any_getreply(const COMPONENT_template& sender_template =
    any_compref, COMPONENT *sender_ptr = NULL);
  virtual alt_status check_getreply(const COMPONENT_template&
    sender_template = any_compref, COMPONENT *sender_ptr = NULL,
    Index_Redirect* index_redirect = NULL);
  static alt_status any_check_getreply(const COMPONENT_template&
    sender_template = any_compref, COMPONENT *sender_ptr = NULL);

  virtual alt_status get_exception(const COMPONENT_template&
    sender_template = any_compref, COMPONENT *sender_ptr = NULL,
    Index_Redirect* index_redirect = NULL);
  static alt_status any_catch(const COMPONENT_template& sender_template =
    any_compref, COMPONENT *sender_ptr = NULL);
  virtual alt_status check_catch(const COMPONENT_template&
    sender_template = any_compref, COMPONENT *sender_ptr = NULL,
    Index_Redirect* index_redirect = NULL);
  static alt_status any_check_catch(const COMPONENT_template&
    sender_template = any_compref, COMPONENT *sender_ptr = NULL);

  alt_status check(const COMPONENT_template& sender_template = any_compref,
    COMPONENT *sender_ptr = NULL, Index_Redirect* index_redirect = NULL);
  static alt_status any_check(const COMPONENT_template& sender_template =
    any_compref, COMPONENT *sender_ptr = NULL);

  /** Set a parameter on the port.
  *  @param parameter_name string
  *  @param parameter_value
  *
  *  The implementation in the PORT base class issues a warning and
  *  does nothing. Derived classes need to override this method.
  */
  virtual void set_parameter(const char *parameter_name,
    const char *parameter_value);

  void append_to_msg_queue(msg_queue_item_base*);
private:
  /** Callback interface for handling events - introduced in TITAN R7E
  * To use the finer granularity interface, this method must not be
  * overridden in the descendant Test Port class.
  * Note: Error event always triggers event handler call and is indicated in
  * the is_error parameter even if not requested.
  */
  virtual void Handle_Fd_Event(int fd,
    boolean is_readable, boolean is_writable, boolean is_error);

  /** Callback interface for handling timeout - introduced in TITAN R7E
  * May not be overridden in the descendant Test Port class if
  * timeout is not used.
  */
  virtual void Handle_Timeout(double time_since_last_call);

  /** Callback interface for handling events - introduced in TITAN R7E
  * These methods in the descendant Test Port class are called only if
  * Handle_Fd_Event is not overridden in the descendant Test Port class.
  * The method handling an event which is not used in the Test Port
  * may not be overridden in the descendant Test Port class.
  * This is true even for the error event handler, although error events
  * always trigger event handler call even if not requested.
  * (There is an empty default implementation for the error event handler.)
  */
  virtual void Handle_Fd_Event_Error(int fd);
  virtual void Handle_Fd_Event_Writable(int fd);
  virtual void Handle_Fd_Event_Readable(int fd);

public:
  /** Callback interface for handling events
  * This method is provided for Test Ports developed for TITAN versions
  * before R7E.
  * (It is called only if event handler has been registered with
  * Install_Handler.)
  */
  virtual void Event_Handler(const fd_set *read_fds, const fd_set *write_fds,
    const fd_set *error_fds, double time_since_last_call);

protected:
  /** Interface for handling events and timeout - introduced in TITAN R7E */
  typedef enum {
    EVENT_RD = FD_EVENT_RD, EVENT_WR = FD_EVENT_WR,
      EVENT_ERR = FD_EVENT_ERR,
      EVENT_ALL = FD_EVENT_RD | FD_EVENT_WR | FD_EVENT_ERR
  } Fd_Event_Type;
  void Handler_Add_Fd(int fd, Fd_Event_Type event_mask = EVENT_ALL);
  void Handler_Add_Fd_Read(int fd);
  void Handler_Add_Fd_Write(int fd);
  void Handler_Remove_Fd(int fd, Fd_Event_Type event_mask = EVENT_ALL);
  void Handler_Remove_Fd_Read(int fd);
  void Handler_Remove_Fd_Write(int fd);
  void Handler_Set_Timer(double call_interval, boolean is_timeout = TRUE,
    boolean call_anyway = TRUE, boolean is_periodic = TRUE);

  /** Interface for handling events and timeout
  * This method is provided for Test Ports developed for TITAN versions
  * before R7E.
  */
  void Install_Handler(const fd_set *read_fds, const fd_set *write_fds,
    const fd_set *error_fds, double call_interval);
  /** Interface for handling events and timeout
  * This method is in use in Test Ports developed for TITAN versions
  * before R7E.
  * It can be used together with the interface introduced in TITAN R7E.
  */
  void Uninstall_Handler();

  virtual void user_map(const char *system_port);
  virtual void user_unmap(const char *system_port);

  virtual void user_start();
  virtual void user_stop();

  virtual void clear_queue();

  component get_default_destination();

  static void prepare_message(Text_Buf& outgoing_buf,
    const char *message_type);
  static void prepare_call(Text_Buf& outgoing_buf,
    const char *signature_name);
  static void prepare_reply(Text_Buf& outgoing_buf,
    const char *signature_name);
  static void prepare_exception(Text_Buf& outgoing_buf,
    const char *signature_name);
  void send_data(Text_Buf& outgoing_buf,
    const COMPONENT& destination_component);

  void process_data(port_connection *conn_ptr, Text_Buf& incoming_buf);
  virtual boolean process_message(const char *message_type,
    Text_Buf& incoming_buf, component sender_component, OCTETSTRING&);
  virtual boolean process_call(const char *signature_name,
    Text_Buf& incoming_buf, component sender_component);
  virtual boolean process_reply(const char *signature_name,
    Text_Buf& incoming_buf, component sender_component);
  virtual boolean process_exception(const char *signature_name,
    Text_Buf& incoming_buf, component sender_component);
  
  // Resets the port type variables to NULL after unmap
  virtual void reset_port_variables();
  
  // Initializes the port variables after map
  virtual void init_port_variables();
  
private:
  port_connection *add_connection(component remote_component,
    const char *remote_port, transport_type_enum transport_type);
  void remove_connection(port_connection *conn_ptr);
  port_connection *lookup_connection_to_compref(component remote_component,
    boolean *is_unique);
  port_connection *lookup_connection(component remote_component,
    const char *remote_port);
  void add_local_connection(PORT *other_endpoint);
  void remove_local_connection(port_connection *conn_ptr);

  static unsigned int get_connection_hash(component local_component,
    const char *local_port, component remote_component,
    const char *remote_port);
  static void unlink_unix_pathname(int socket_fd);
  void connect_listen_inet_stream(component remote_component,
    const char *remote_port);
  void connect_listen_unix_stream(component remote_component,
    const char *remote_port);
  void connect_local(component remote_component, const char *remote_port);
  void connect_stream(component remote_component, const char *remote_port,
    transport_type_enum transport_type, Text_Buf& text_buf);
  void disconnect_local(port_connection *conn_ptr);
  void disconnect_stream(port_connection *conn_ptr);

  void send_data_local(port_connection *conn_ptr, Text_Buf& outgoing_data);
  boolean send_data_stream(port_connection *conn_ptr, Text_Buf& outgoing_data,
    boolean ignore_peer_disconnect);

  void handle_incoming_connection(port_connection *conn_ptr);
  void handle_incoming_data(port_connection *conn_ptr);
  void process_last_message(port_connection *conn_ptr);

  void map(const char *system_port);
  void unmap(const char *system_port);

public:
  static void process_connect_listen(const char *local_port,
    component remote_component, const char *remote_port,
    transport_type_enum transport_type);
  static void process_connect(const char *local_port,
    component remote_component, const char *remote_port,
    transport_type_enum transport_type, Text_Buf& text_buf);
  static void process_disconnect(const char *local_port,
    component remote_component, const char *remote_port);
  static void make_local_connection(const char *src_port,
    const char *dest_port);
  static void terminate_local_connection(const char *src_port,
    const char *dest_port);

  static void map_port(const char *component_port, const char *system_port);
  static void unmap_port(const char *component_port, const char *system_port);
};

#endif
