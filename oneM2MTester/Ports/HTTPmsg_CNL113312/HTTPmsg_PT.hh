/******************************************************************************
* Copyright (c) 2004, 2014  Ericsson AB
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*
* Contributors:
*   Eduard Czimbalmos - initial implementation and initial documentation
*   Istvan Ovary
*   Peter Dimitrov
*   Balasko Jeno
*   Gabor Szalai
******************************************************************************/
//
//  File:               HTTPmsg_PT.hh
//  Description:        HTTP test port header file
//  Rev:                R8G
//  Prodnr:             CNL 113 469


#ifndef HTTPmsg__PT_HH
#define HTTPmsg__PT_HH

#include "HTTPmsg_PortType.hh"
#include "Abstract_Socket.hh"

#define BUFFER_FAIL 2
#define BUFFER_CRLF 3
//==============================
namespace HTTPmsg__PortType {
//==============================
typedef struct {
    bool non_persistent_connection;
    bool chunked_body;
    int content_length;
    bool error;
    bool isMessage;
} Decoding_Params;

#ifdef AS_USE_SSL
class HTTPmsg__PT : public SSL_Socket, public HTTPmsg__PT_BASE {
#else
  class HTTPmsg__PT : public Abstract_Socket, public HTTPmsg__PT_BASE {
#endif

public:

    HTTPmsg__PT(const char *par_port_name = NULL);
    ~HTTPmsg__PT();

    void set_parameter(const char *parameter_name, const char *parameter_value);

protected:
    void user_map(const char *system_port);
    void user_unmap(const char *system_port);

    void user_start();
    void user_stop();

    void outgoing_send(const HTTPmsg__Types::Close& send_par);
    void outgoing_send(const HTTPmsg__Types::Connect& send_par);
    void outgoing_send(const HTTPmsg__Types::Listen& send_par);
    void outgoing_send(const HTTPmsg__Types::Half__close& send_par);
    void outgoing_send(const HTTPmsg__Types::Shutdown& send_par);
    void outgoing_send(const HTTPmsg__Types::HTTPMessage& send_par);

    const char* local_port_name();
    const char* remote_address_name();
    const char* local_address_name();
    const char* remote_port_name();
    const char* use_notification_ASPs_name();
    const char* halt_on_connection_reset_name();
    const char* server_mode_name();
    const char* socket_debugging_name();
    const char* nagling_name();
    const char* server_backlog_name();
    const char* ssl_use_ssl_name();
    const char* ssl_use_session_resumption_name();
    const char* ssl_private_key_file_name();
    const char* ssl_trustedCAlist_file_name();
    const char* ssl_certificate_file_name();
    const char* ssl_password_name();
    const char* ssl_verifycertificate_name();

    void message_incoming(const unsigned char* msg, int length, int client_id = -1);
    void Add_Fd_Read_Handler(int fd) { Handler_Add_Fd_Read(fd); }
    void Add_Fd_Write_Handler(int fd) { Handler_Add_Fd_Write(fd); }
    void Remove_Fd_Read_Handler(int fd) { Handler_Remove_Fd_Read(fd); }
    void Remove_Fd_Write_Handler(int fd) { Handler_Remove_Fd_Write(fd); }
    void Remove_Fd_All_Handlers(int fd) { Handler_Remove_Fd(fd); }
    void Handler_Uninstall() { Uninstall_Handler(); }
    void Timer_Set_Handler(double call_interval, boolean is_timeout = TRUE,
      boolean call_anyway = TRUE, boolean is_periodic = TRUE) {
      Handler_Set_Timer(call_interval, is_timeout, call_anyway, is_periodic);
    }
    
// overriden functions in order to distinguish between normal and SSL connections
    virtual bool add_user_data(int client_id);
    virtual bool remove_user_data(int client_id);
    virtual int  send_message_on_fd(int client_id, const unsigned char * message_buffer, int length_of_message);
    virtual int  send_message_on_nonblocking_fd(int client_id, const unsigned char * message_buffer, int length_of_message);
    virtual int  receive_message_on_fd(int client_id);
    virtual void client_connection_opened(int client_id);
    virtual void listen_port_opened(int port_number);    
    virtual void peer_connected(int client_id, const char * host, const int port);


    virtual void peer_disconnected(int client_id);
    virtual void peer_half_closed(int client_id);
    virtual void remove_client(int client_id);
    
// HTTP specific functions
    
    // returns encoded message in buf
    //void HTTP_encode(const HTTPmsg__Types::HTTPMessage &msg, TTCN_Buffer& buf); //replaced by f_HTTP_encodeCommon
    bool HTTP_decode(TTCN_Buffer*, const int, const bool connection_closed = false);
    //void HTTP_decode_header(TTCN_Buffer*, HTTPmsg__Types::HeaderLines&,  Decoding_Params&); //moved outside the class
    //void HTTP_decode_body(TTCN_Buffer*, OCTETSTRING&, Decoding_Params&, const bool); //moved outside the class
    //void HTTP_decode_chunked_body(TTCN_Buffer*, OCTETSTRING&, Decoding_Params&);  //moved outside the class
    
private:
    void Handle_Fd_Event(int fd, boolean is_readable, boolean is_writable, boolean is_error);
    void Handle_Timeout(double time_since_last_call);

    //int get_line(TTCN_Buffer*, CHARSTRING&, const bool concatenate_header_lines = true);
    //void log_to_hexa(TTCN_Buffer*);

    bool adding_ssl_connection;
    bool adding_client_connection;
    bool server_use_ssl;
    
    bool use_notification_ASPs;
};
//===================================
//== Functions outside the class: ===
//===================================
void f_setClientId( HTTPmsg__Types::HTTPMessage& msg, const int client_id);
void f_HTTP_encodeCommon(const HTTPmsg__Types::HTTPMessage& msg, TTCN_Buffer& buf);
bool f_HTTP_decodeCommon(TTCN_Buffer* buffer,HTTPmsg__Types::HTTPMessage& msg, const bool connection_closed,
    const bool socket_debugging, const char *test_port_type, const char *test_port_name);
int get_line(TTCN_Buffer* buffer, CHARSTRING& to, const bool concatenate_header_lines);
void log_to_hexa(TTCN_Buffer*);

void HTTP_decode_header(TTCN_Buffer*, HTTPmsg__Types::HeaderLines&,  Decoding_Params&,const bool socket_debugging, const bool resp,const char *test_port_type, const char *test_port_name);

void HTTP_decode_body(TTCN_Buffer*, OCTETSTRING&, Decoding_Params&, const bool,
    const bool socket_debugging, const char *test_port_type, const char *test_port_name);

void HTTP_decode_chunked_body(TTCN_Buffer*, OCTETSTRING&, Decoding_Params&,
    const bool socket_debugging, const char *test_port_type, const char *test_port_name);
}//namespace

//==============================
namespace HTTPmsg__Types
//===============================
{
void log_debug(const bool socket_debugging, const char *test_port_type, const char *test_port_name, const char *fmt, ...);
void log_warning(const char *test_port_type, const char *test_port_name, const char *fmt, ...);
}

#endif
