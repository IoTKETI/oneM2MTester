/******************************************************************************
* Copyright (c) 2004, 2014  Ericsson AB
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*
* Contributors:
*   Zoltan Bibo - initial implementation and initial documentation
*   Gergely Futo
*   Oliver Ferenc Czerman
*   Balasko Jeno
*   Zoltan Bibo
*   Eduard Czimbalmos
*   Kulcsár Endre
*   Gabor Szalai
*   Jozsef Gyurusi
*   Csöndes Tibor
*   Zoltan Jasz
******************************************************************************/
//
//  File:               Abstract_Socket.hh
//  Description:        Abstract_Socket header file
//  Rev:                R8C
//  Prodnr:             CNL 113 384
//


#ifndef Abstract_Socket_HH
#define Abstract_Socket_HH

#ifdef AS_USE_SSL
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <TTCN3.hh>

// to support systems not supporting IPv6 define AF_INET6 to some dummy value:
#ifndef AF_INET6
#define AF_INET6 (-255)
#endif

class PacketHeaderDescr {
public:
  // Byte order in the header
  enum HeaderByteOrder{ Header_MSB, Header_LSB };
private:
  unsigned long length_offset;
  unsigned long nr_bytes_in_length;
  HeaderByteOrder byte_order;
  long value_offset;
  unsigned long length_multiplier;
public:
  PacketHeaderDescr(unsigned long p_length_offset,
    unsigned long p_nr_bytes_in_length, HeaderByteOrder p_byte_order,
    long p_value_offset = 0, unsigned long p_length_multiplier = 1)
    : length_offset(p_length_offset), nr_bytes_in_length(p_nr_bytes_in_length),
    byte_order(p_byte_order), value_offset(p_value_offset),
    length_multiplier(p_length_multiplier) { }

  // returns the message length
  unsigned long Get_Message_Length(const unsigned char* buffer_pointer) const;
  // returns the number of bytes needed to have a valid message length
  inline unsigned long Get_Valid_Header_Length() const
   { return length_offset + nr_bytes_in_length; }
};

class Abstract_Socket
{
protected:
  enum TCP_STATES {CLOSED, LISTEN, ESTABLISHED, CLOSE_WAIT, FIN_WAIT};
  enum READING_STATES {STATE_DONT_RECEIVE, STATE_WAIT_FOR_RECEIVE_CALLBACK, STATE_BLOCK_FOR_SENDING, STATE_DONT_CLOSE, STATE_NORMAL};
  // client data
  struct as_client_struct {
    void *user_data;       // pointer to any additional data needed by the user
    TTCN_Buffer *fd_buff;  // pointer to the data buffer
    struct sockaddr_storage clientAddr;// client address
#if defined LINUX || defined FREEBSD || defined SOLARIS8
      socklen_t
#else /* SOLARIS or WIN32 */
        int
#endif
    clientAddrlen;
    TCP_STATES tcp_state;  // TCP state
    READING_STATES reading_state; //used when SSL_write returns SSL_ERROR_WANT_READ an we are using non-blocking socket
  };

  Abstract_Socket();
  Abstract_Socket(const char *testport_type, const char *testport_name);
  virtual ~Abstract_Socket();

  // Shall be called from set_parameter()
  bool parameter_set(const char *parameter_name, const char *parameter_value);
  // Shall be called from user_map()
  void map_user();
  // Shall be called from user_unmap()
  void unmap_user();

  // puts the IP address in the addr
  void get_host_id(const char* hostName, struct sockaddr_in *addr); /* This function should not be used! Use getaddrinfo instead! */

  // Closes the current listening port and opens the specified one
  int open_listen_port(const struct sockaddr_in & localAddr); /* This function should be removed! Deprecated by: */
  int open_listen_port(const char* localHostname, const char* localServicename);
  // Closes the current listening port
  void close_listen_port();

  virtual void listen_port_opened(int port_number);

  // Opens a new client connection
  int open_client_connection(const struct sockaddr_in & new_remote_addr, const struct sockaddr_in & new_local_addr); /* This function should be removed! Deprecated by: */
  int open_client_connection(const char* remoteHostname, const char* remoteService, const char* localHostname, const char* localService);

  virtual void client_connection_opened(int client_id);

  // Shall be called from Handle_Fd_Event()
  void Handle_Socket_Event(int fd, boolean is_readable, boolean is_writable, boolean is_error);
  // Shall be called from Handle_Timeout() - for possible future development
  void Handle_Timeout_Event(double /*time_since_last_call*/) {};

  // Shall be called from outgoing_send()
  void send_outgoing(const unsigned char* message_buffer, int length, int client_id = -1);
  void send_shutdown(int client_id = -1);

  // Access to private variables
  bool get_nagling() const {return nagling;}
  bool get_use_non_blocking_socket() const {return use_non_blocking_socket;};
  bool get_server_mode() const {return server_mode;}
  bool get_socket_debugging() const {return socket_debugging;}
  bool get_halt_on_connection_reset() const {return halt_on_connection_reset;}
  bool get_use_connection_ASPs() const {return use_connection_ASPs;}
  bool get_handle_half_close() const {return handle_half_close;}
  int  get_socket_fd() const;
  int  get_listen_fd() const {return listen_fd;}
    
  //set non-blocking mode
  int set_non_block_mode(int fd, bool enable_nonblock);

  //increase buffer size
  bool increase_send_buffer(int fd, int &old_size, int& new_size);

  const char* get_local_host_name(){return local_host_name; };
  unsigned int get_local_port_number(){return local_port_number; };
  const char* get_remote_host_name(){return remote_host_name; };
  unsigned int get_remote_port_number(){return remote_port_number; };
  const struct sockaddr_in & get_remote_addr() {return remoteAddr; }; /* FIXME: This function is deprecated and should be removed! */
  const struct sockaddr_in & get_local_addr() {return localAddr; };   /* FIXME: This function is deprecated and should be removed! */
  const int& get_ai_family() const {return ai_family;}
  void set_ai_family(int parameter_value) {ai_family=parameter_value;}
  bool get_ttcn_buffer_usercontrol() const {return ttcn_buffer_usercontrol; }
  void set_nagling(bool parameter_value) {nagling=parameter_value;}
  void set_server_mode(bool parameter_value) {server_mode=parameter_value;}
  void set_handle_half_close(bool parameter_value) {handle_half_close=parameter_value;}
  void set_socket_debugging(bool parameter_value) {socket_debugging=parameter_value;}
  void set_halt_on_connection_reset(bool parameter_value) {halt_on_connection_reset=parameter_value;}
  void set_ttcn_buffer_usercontrol(bool parameter_value) {ttcn_buffer_usercontrol=parameter_value;}
  const char *test_port_type;
  const char *test_port_name;

  // Called when a message is received
  virtual void message_incoming(const unsigned char* message_buffer, int length, int client_id = -1) = 0;

  virtual void Add_Fd_Read_Handler(int fd) = 0;
  virtual void Add_Fd_Write_Handler(int fd) = 0;
  virtual void Remove_Fd_Read_Handler(int fd) = 0;
  virtual void Remove_Fd_Write_Handler(int fd) = 0;
  virtual void Remove_Fd_All_Handlers(int fd) = 0;
  virtual void Handler_Uninstall() = 0;
  virtual void Timer_Set_Handler(double call_interval, boolean is_timeout = TRUE,
    boolean call_anyway = TRUE, boolean is_periodic = TRUE) = 0; // unused - for possible future development
  virtual const PacketHeaderDescr* Get_Header_Descriptor() const;

  // Logging functions
  void log_debug(const char *fmt, ...) const
    __attribute__ ((__format__ (__printf__, 2, 3)));
  void log_warning(const char *fmt, ...) const
    __attribute__ ((__format__ (__printf__, 2, 3)));
  void log_error(const char *fmt, ...) const
    __attribute__ ((__format__ (__printf__, 2, 3), __noreturn__));
  void log_hex(const char *prompt, const unsigned char *msg, size_t length) const;

  // Called when a message is to be received (an event detected)
  virtual int receive_message_on_fd(int client_id);
  // Called when a message is to be sent
  virtual int send_message_on_fd(int client_id, const unsigned char* message_buffer, int message_length);
  virtual int send_message_on_nonblocking_fd(int client_id, const unsigned char *message_buffer, int message_length);
  // Called after a peer is connected
  virtual void peer_connected(int client_id, sockaddr_in& remote_addr); /* This function should be removed! deprecated by: */
  virtual void peer_connected(int /*client_id*/, const char * /*host*/, const int /*port*/) {};
  // Called after a peer is disconnected
  virtual void peer_disconnected(int client_id);
  // Called when a peer shut down its fd for writing
  virtual void peer_half_closed(int client_id);
  // Called after a send error
  virtual void report_error(int client_id, int msg_length, int sent_length, const unsigned char* msg, const char* error_text);

  // Test port parameters
  virtual const char* local_port_name();
  virtual const char* remote_address_name();
  virtual const char* local_address_name();
  virtual const char* remote_port_name();
  virtual const char* ai_family_name();
  virtual const char* use_connection_ASPs_name();
  virtual const char* halt_on_connection_reset_name();
  virtual const char* client_TCP_reconnect_name();
  virtual const char* TCP_reconnect_attempts_name();
  virtual const char* TCP_reconnect_delay_name();
  virtual const char* server_mode_name();
  virtual const char* socket_debugging_name();
  virtual const char* nagling_name();
  virtual const char* use_non_blocking_socket_name();
  virtual const char* server_backlog_name();

  // Fetch/Set user data pointer
  void* get_user_data(int client_id) {return get_peer(client_id)->user_data;}
  void  set_user_data(int client_id, void *uptr) {get_peer(client_id)->user_data = uptr;}
  // Called after a TCP connection is established
  virtual bool add_user_data(int client_id);
  // Called before the TCP connection is drop down
  virtual bool remove_user_data(int client_id);
  // Called when a client shall be removed
  virtual void remove_client(int client_id);
  // Called when all clients shall be removed
  virtual void remove_all_clients();
  // Called at the beginning of map() to check mandatory parameter presence
  virtual bool user_all_mandatory_configparameters_present();
  TTCN_Buffer *get_buffer(int client_id) {return get_peer(client_id)->fd_buff; }

  // Client data management functions
  // add peer to the list
  as_client_struct *peer_list_add_peer(int client_id);
  // remove peer from list
  void peer_list_remove_peer(int client_id);
  // remove all peers from list
  void peer_list_reset_peer();
  // returns back the structure of the peer
  as_client_struct *get_peer(int client_id, bool no_error=false) const;
  // length of the list
  int peer_list_get_length() const { return peer_list_length; }
  // number of peers in the list
  int peer_list_get_nr_of_peers() const;
  // fd of the last peer in the list
  int peer_list_get_last_peer() const;
  // fd of the first peer in the list
  int peer_list_get_first_peer() const;


private:
  void handle_message(int client_id = -1);
  void all_mandatory_configparameters_present();
  bool halt_on_connection_reset_set;
  bool halt_on_connection_reset;
  bool client_TCP_reconnect;
  int TCP_reconnect_attempts;
  int TCP_reconnect_delay;
  bool server_mode;
  bool use_connection_ASPs;
  bool handle_half_close;
  bool socket_debugging;
  bool nagling;
  bool use_non_blocking_socket;
  bool ttcn_buffer_usercontrol;
  char* local_host_name;
  unsigned int local_port_number;
  char* remote_host_name;
  unsigned int remote_port_number;
  int ai_family; // address family to use
  // remoteAddr and localAddr is filled when map_user is called
  struct sockaddr_in remoteAddr; /* FIXME: not used! should be removed */
  struct sockaddr_in localAddr;  /* FIXME: not used! should be removed */
  int  server_backlog;
  int  deadlock_counter;
  int  listen_fd;
  int  peer_list_length;

  // Client data management functions
  as_client_struct **peer_list_root;
  void peer_list_resize_list(int client_id);
};



#ifdef AS_USE_SSL

class SSL_Socket: public Abstract_Socket
{

protected:
  SSL_Socket();
  SSL_Socket(const char *tp_type, const char *tp_name);
  virtual ~SSL_Socket();

  bool         parameter_set(const char * parameter_name, const char * parameter_value);
  // Called after a TCP connection is established (client side or server accepted a connection).
  // It will create a new SSL conenction on the top of the TCP connection.
  virtual bool add_user_data(int client_id);
  // Called after a TCP connection is closed.
  // It will delete the SSL conenction.
  virtual bool remove_user_data(int client_id);
  // Called from all_mandatory_configparameters_present() function
  // during map() operation to check mandatory parameter presents.
  virtual bool user_all_mandatory_configparameters_present();
  // Called after an SSL connection is established (handshake finished) for further
  // authentication. Shall return 'true' if verification
  // is OK, otherwise 'false'. If return value was 'true', the connection is kept, otherwise
  // the connection will be shutted down.
  virtual bool  ssl_verify_certificates();
  // Call during SSL handshake (and rehandshake as well) by OpenSSL
  // Return values:
  // ==1: user authentication is passed, go on with handshake
  // ==0: user authentication failed, refuse the connection to the other peer
  // <0 : user don't care, go on with default basic checks
  virtual int   ssl_verify_certificates_at_handshake(int preverify_ok, X509_STORE_CTX *ssl_ctx);
  // Called to receive from the socket if data is available (select()).
  // Shall return with 0 if the peer is disconnected or with the number of bytes read.
  // If error occured, execution shall stop in the function by calling log_error()
  virtual int  receive_message_on_fd(int client_id);
  // Called to send a message on the socket.
  // Shall return with 0 if the peer is disconnected or with the number of bytes written.
  // If error occured, execution shall stop in the function by calling log_error()
  virtual int  send_message_on_fd(int client_id, const unsigned char * message_buffer, int length_of_message);
  virtual int  send_message_on_nonblocking_fd(int client_id, const unsigned char * message_buffer, int length_of_message);

  // The following members can be called to fetch the current values
  bool         get_ssl_use_ssl() const                {return ssl_use_ssl;}
  bool         get_ssl_verifycertificate() const      {return ssl_verify_certificate;}
  bool         get_ssl_use_session_resumption() const {return ssl_use_session_resumption;}
  bool         get_ssl_initialized() const            {return ssl_initialized;}
  char       * get_ssl_key_file() const               {return ssl_key_file;}
  char       * get_ssl_certificate_file() const       {return ssl_certificate_file;}
  char       * get_ssl_trustedCAlist_file() const     {return ssl_trustedCAlist_file;}
  char       * get_ssl_cipher_list() const            {return ssl_cipher_list;}
  char       * get_ssl_password() const;
  const unsigned char * get_ssl_server_auth_session_id_context() const {return ssl_server_auth_session_id_context;}
//  const SSL_METHOD * get_current_ssl_method() const         {return ssl_method;}
//  const SSL_CIPHER * get_current_ssl_cipher() const         {return ssl_cipher;}
  SSL_SESSION* get_current_ssl_session() const        {return ssl_session;}
  SSL_CTX    * get_current_ssl_ctx() const            {return ssl_ctx;}
  SSL        * get_current_ssl() const                {return ssl_current_ssl;}

  // The following members can be called to set the current values
  // NOTE that in case the parameter_value is a char *pointer, the old character
  // array is deleted by these functions automatically.
  void         set_ssl_use_ssl(bool parameter_value);
  void         set_ssl_verifycertificate(bool parameter_value);
  void         set_ssl_use_session_resumption(bool parameter_value);
  void         set_ssl_key_file(char * parameter_value);
  void         set_ssl_certificate_file(char * parameter_value);
  void         set_ssl_trustedCAlist_file(char * parameter_value);
  void         set_ssl_cipher_list(char * parameter_value);
  void         set_ssl_server_auth_session_id_context(const unsigned char * parameter_value);

  // The following members can be called to fetch the default test port parameter names
  virtual const char* ssl_use_ssl_name();
  virtual const char* ssl_use_session_resumption_name();
  virtual const char* ssl_private_key_file_name();
  virtual const char* ssl_trustedCAlist_file_name();
  virtual const char* ssl_certificate_file_name();
  virtual const char* ssl_password_name();
  virtual const char* ssl_cipher_list_name();
  virtual const char* ssl_verifycertificate_name();
  virtual const char* ssl_disable_SSLv2();
  virtual const char* ssl_disable_SSLv3();
  virtual const char* ssl_disable_TLSv1();
  virtual const char* ssl_disable_TLSv1_1();
  virtual const char* ssl_disable_TLSv1_2();

private:
  bool ssl_verify_certificate;     // verify other part's certificate or not
  bool ssl_use_ssl;                // whether to use SSL
  bool ssl_initialized;            // whether SSL already initialized or not
  bool ssl_use_session_resumption; // use SSL sessions or not

  bool SSLv2;
  bool SSLv3;
  bool TLSv1;
  bool TLSv1_1;
  bool TLSv1_2;


  char *ssl_key_file;              // private key file
  char *ssl_certificate_file;      // own certificate file
  char *ssl_trustedCAlist_file;    // trusted CA list file
  char *ssl_cipher_list;           // ssl_cipher list restriction to apply
  char *ssl_password;              // password to decode the private key
  static const unsigned char * ssl_server_auth_session_id_context;

//  const SSL_METHOD  *ssl_method;         // SSL context method
  SSL_CTX     *ssl_ctx;            // SSL context
//  const SSL_CIPHER  *ssl_cipher;         // used SSL ssl_cipher
  SSL_SESSION *ssl_session;        // SSL ssl_session
  SSL         *ssl_current_ssl;    // currently used SSL object
  static void *ssl_current_client; // current SSL object, used only during authentication

  void ssl_actions_to_seed_PRNG(); // Seed the PRNG with enough random data
  void ssl_init_SSL();             // Initialize SSL libraries and create the SSL context
  void ssl_log_SSL_info();         // Log the currently used SSL setting (debug)
  int  ssl_getresult(int result_code); // Fetch and log the SSL error code from I/O operation result codes
  // Callback function to pass the password to OpenSSL. Called by OpenSSL
  // during SSL handshake.
  static int ssl_password_cb(char * password_buffer, int length_of_password, int rw_flag, void * user_data);
  // Callback function to perform authentication during SSL handshake. Called by OpenSSL.
  // NOTE: for further authentication, use ssl_verify_certificates().
  static int ssl_verify_callback(int preverify_status, X509_STORE_CTX * ssl_context);
};
#endif

#endif
