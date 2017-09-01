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
*   Kulcs�r Endre
*   Gabor Szalai
*   Jozsef Gyurusi
*   Cs�ndes Tibor
*   Zoltan Jasz
******************************************************************************/
//
//  File:               Abstract_Socket.cc
//  Description:        Abstract_Socket implementation file
//  Rev:                R8C
//  Prodnr:             CNL 113 384
//

#include "Abstract_Socket.hh"

#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#if defined SOLARIS8
# include <signal.h>
#endif


#define AS_TCP_CHUNCK_SIZE 4096
#define AS_SSL_CHUNCK_SIZE 16384
// Used for the 'address already in use' bug workaround
#define AS_DEADLOCK_COUNTER 16
// character buffer length to store temporary SSL informations, 256 is usually enough
#define SSL_CHARBUF_LENGTH 256
// number of bytes to read from the random devices
#define SSL_PRNG_LENGTH 1024

#ifndef NI_MAXHOST
#define NI_MAXHOST 1024
#endif
#ifndef NI_MAXSERV
#define NI_MAXSERV 32
#endif

/********************************
 **  PacketHeaderDescr
 **  used for fragmentation and concatenation
 **  of fixed format messages
 *********************************/

unsigned long PacketHeaderDescr::Get_Message_Length(const unsigned char* buff) const
{
  unsigned long m_length = 0;
  for (unsigned long i = 0; i < nr_bytes_in_length; i++) {
    unsigned long shift_count =
      byte_order == Header_MSB ? nr_bytes_in_length - 1 - i : i;
    m_length |= buff[length_offset + i] << (8 * shift_count);
  }
  m_length *= length_multiplier;
  if (value_offset < 0 && (long)m_length < -value_offset) return 0;
  else return m_length + value_offset;
}


////////////////////////////////////////////////////////////////////////
/////    Default log functions
////////////////////////////////////////////////////////////////////////
void Abstract_Socket::log_debug(const char *fmt, ...) const
{
  if (socket_debugging) {
    TTCN_Logger::begin_event(TTCN_DEBUG);
    if (test_port_type != NULL && test_port_name != NULL)
      TTCN_Logger::log_event("%s test port (%s): ", test_port_type,
        test_port_name);
    else TTCN_Logger::log_event_str("Abstract socket: ");
    va_list args;
    va_start(args, fmt);
    TTCN_Logger::log_event_va_list(fmt, args);
    va_end(args);
    TTCN_Logger::end_event();
  }
}

void Abstract_Socket::log_warning(const char *fmt, ...) const
{
  TTCN_Logger::begin_event(TTCN_WARNING);
  if (test_port_type != NULL && test_port_name != NULL)
    TTCN_Logger::log_event("%s test port (%s): warning: ", test_port_type,
      test_port_name);
  else TTCN_Logger::log_event_str("Abstract socket: warning: ");
  va_list args;
  va_start(args, fmt);
  TTCN_Logger::log_event_va_list(fmt, args);
  va_end(args);
  TTCN_Logger::end_event();
}


void Abstract_Socket::log_error(const char *fmt, ...) const
{
  va_list args;
  va_start(args, fmt);
  char *error_str = mprintf_va_list(fmt, args);
  va_end(args);
  try {
    if (test_port_type != NULL && test_port_name != NULL)
      TTCN_error("%s test port (%s): %s", test_port_type, test_port_name,
	error_str);
    else TTCN_error("Abstract socket: %s", error_str);
  } catch (...) {
    Free(error_str);
    throw;
  }
  Free(error_str);
}

void Abstract_Socket::log_hex(const char *prompt, const unsigned char *msg,
  size_t length) const
{
  if (socket_debugging) {
    TTCN_Logger::begin_event(TTCN_DEBUG);
    if (test_port_type != NULL && test_port_name != NULL)
       TTCN_Logger::log_event("%s test port (%s): ", test_port_type,
         test_port_name);
    else TTCN_Logger::log_event_str("Abstract socket: ");
    if (prompt != NULL) TTCN_Logger::log_event_str(prompt);
    TTCN_Logger::log_event("Size: %lu, Msg:", (unsigned long)length);
    for (size_t i = 0; i < length; i++) TTCN_Logger::log_event(" %02x", msg[i]);
    TTCN_Logger::end_event();
  }
}


/********************************
 **  Abstract_Socket
 **  abstract base type for TCP socket handling
 *********************************/

Abstract_Socket::Abstract_Socket() {
  server_mode=false;
  socket_debugging=false;
  nagling=false;
  use_non_blocking_socket=false;
  halt_on_connection_reset=true;
  halt_on_connection_reset_set=false;
  client_TCP_reconnect=false;
  TCP_reconnect_attempts=5;
  TCP_reconnect_delay=1;
  listen_fd=-1;
  memset(&remoteAddr, 0, sizeof(remoteAddr));
  memset(&localAddr, 0, sizeof(localAddr));
  server_backlog=1;
  peer_list_length=0;
  local_host_name = NULL;
  local_port_number = 0;
  remote_host_name = NULL;
  remote_port_number = 0;
  ai_family = AF_UNSPEC; // default: Auto
  test_port_type=NULL;
  test_port_name=NULL;
  ttcn_buffer_usercontrol=false;
  use_connection_ASPs=false;
  handle_half_close = false;
  peer_list_root = NULL;
}

Abstract_Socket::Abstract_Socket(const char *tp_type, const char *tp_name) {
  server_mode=false;
  socket_debugging=false;
  nagling=false;
  use_non_blocking_socket=false;
  halt_on_connection_reset=true;
  halt_on_connection_reset_set=false;
  client_TCP_reconnect=false;
  TCP_reconnect_attempts=5;
  TCP_reconnect_delay=1;
  listen_fd=-1;
  memset(&remoteAddr, 0, sizeof(remoteAddr));
  memset(&localAddr, 0, sizeof(localAddr));
  server_backlog=1;
  peer_list_length=0;
  local_host_name = NULL;
  local_port_number = 0;
  remote_host_name = NULL;
  remote_port_number = 0;
  ai_family = AF_UNSPEC; // default: Auto
  test_port_type=tp_type;
  test_port_name=tp_name;
  ttcn_buffer_usercontrol=false;
  use_connection_ASPs=false;
  handle_half_close = false;
  peer_list_root = NULL;
}

Abstract_Socket::~Abstract_Socket() {
  peer_list_reset_peer();
  Free(local_host_name);
  Free(remote_host_name);
}

bool Abstract_Socket::parameter_set(const char *parameter_name,
                                    const char *parameter_value)
{
  //log_debug("entering Abstract_Socket::parameter_set(%s, %s)", parameter_name, parameter_value);

  if (strcmp(parameter_name, socket_debugging_name()) == 0) {
    if (strcasecmp(parameter_value,"yes")==0) socket_debugging = true;
    else if (strcasecmp(parameter_value,"no")==0) socket_debugging = false;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, socket_debugging_name());
  } else if (strcmp(parameter_name, server_mode_name()) == 0) {
    if (strcasecmp(parameter_value,"yes")==0) server_mode = true;
    else if (strcasecmp(parameter_value,"no")==0) server_mode = false;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, server_mode_name());
  } else if (strcmp(parameter_name, use_connection_ASPs_name()) == 0) {
    if (strcasecmp(parameter_value,"yes")==0) use_connection_ASPs = true;
    else if (strcasecmp(parameter_value,"no")==0) use_connection_ASPs = false;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, use_connection_ASPs_name());
  } else if (strcmp(parameter_name, halt_on_connection_reset_name()) == 0) {
    halt_on_connection_reset_set=true;
    if (strcasecmp(parameter_value,"yes")==0) halt_on_connection_reset = true;
    else if (strcasecmp(parameter_value,"no")==0) halt_on_connection_reset = false;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, halt_on_connection_reset_name());
  } else if (strcmp(parameter_name, client_TCP_reconnect_name()) == 0) {
    if (strcasecmp(parameter_value,"yes")==0) client_TCP_reconnect = true;
    else if (strcasecmp(parameter_value,"no")==0) client_TCP_reconnect = false;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, client_TCP_reconnect_name());
  } else if (strcmp(parameter_name, TCP_reconnect_attempts_name()) == 0) {
	if (sscanf(parameter_value, "%d", &TCP_reconnect_attempts)!=1) log_error("Invalid input as TCP_reconnect_attempts counter given: %s", parameter_value);
    if (TCP_reconnect_attempts<=0) log_error("TCP_reconnect_attempts must be greater than 0, %d is given", TCP_reconnect_attempts);
  } else if (strcmp(parameter_name, TCP_reconnect_delay_name()) == 0) {
	if (sscanf(parameter_value, "%d", &TCP_reconnect_delay)!=1) log_error("Invalid input as TCP_reconnect_delay given: %s", parameter_value);
    if (TCP_reconnect_delay<0) log_error("TCP_reconnect_delay must not be less than 0, %d is given", TCP_reconnect_delay);
  } else if(strcmp(parameter_name, remote_address_name()) == 0){
    Free(remote_host_name);
    remote_host_name = mcopystr(parameter_value);
  } else if(strcmp(parameter_name, local_address_name()) == 0){ // only for backward compatibility
    Free(local_host_name);
    local_host_name = mcopystr(parameter_value);
  } else if(strcmp(parameter_name, remote_port_name()) == 0){
    int a;
    if (sscanf(parameter_value, "%d", &a)!=1) log_error("Invalid input as port number given: %s", parameter_value);
    if (a>65535 || a<0){ log_error("Port number must be between 0 and 65535, %d is given", remote_port_number);}
    else {remote_port_number=a;}
  } else if(strcmp(parameter_name, ai_family_name()) == 0){
    if (strcasecmp(parameter_value,"IPv6")==0 || strcasecmp(parameter_value,"AF_INET6")==0) ai_family = AF_INET6;
    else if (strcasecmp(parameter_value,"IPv4")==0 || strcasecmp(parameter_value,"AF_INET")==0) ai_family = AF_INET;
    else if (strcasecmp(parameter_value,"UNSPEC")==0 || strcasecmp(parameter_value,"AF_UNSPEC")==0) ai_family = AF_UNSPEC;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ai_family_name());
  } else if(strcmp(parameter_name, local_port_name()) == 0){
    int a;
    if (sscanf(parameter_value, "%d", &a)!=1) log_error("Invalid input as port number given: %s", parameter_value);
    if (a>65535 || a<0) {log_error("Port number must be between 0 and 65535, %d is given", local_port_number);}
    else {local_port_number=a;}
  } else if (strcmp(parameter_name, nagling_name()) == 0) {
    if (strcasecmp(parameter_value,"yes")==0) nagling = true;
    else if (strcasecmp(parameter_value,"no")==0) nagling = false;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, nagling_name());
  } else if (strcmp(parameter_name, use_non_blocking_socket_name()) == 0){
    if (strcasecmp(parameter_value, "yes") == 0) use_non_blocking_socket = true;
    else if (strcasecmp(parameter_value, "no") == 0) use_non_blocking_socket = false;
  } else if (strcmp(parameter_name, server_backlog_name()) == 0) {
    if (sscanf(parameter_value, "%d", &server_backlog)!=1) log_error("Invalid input as server backlog given: %s", parameter_value);
  } else {
    //log_debug("leaving Abstract_Socket::parameter_set(%s, %s)", parameter_name, parameter_value);
    return false;
  }

  //log_debug("leaving Abstract_Socket::parameter_set(%s, %s)", parameter_name, parameter_value);
  return true;
}

void Abstract_Socket::Handle_Socket_Event(int fd, boolean is_readable, boolean is_writable, boolean is_error)
{
  //log_debug("entering Abstract_Socket::Handle_Socket_Event(): fd: %d%s%s%s", fd, is_readable ? " readable" : "", is_writable ? " writable" : "", is_error ? " error" : "");

  if (fd != listen_fd /* on server the connection requests are handled after the user messages */
      && peer_list_root[fd] != NULL && (is_readable || is_writable)
      && get_peer(fd)->reading_state != STATE_DONT_RECEIVE) {
    //log_debug("start receiving message....");
    int messageLength = receive_message_on_fd(fd);
    //log_debug(int2str(messageLength));
    if (messageLength == 0) { // peer disconnected
      as_client_struct * client_data = get_peer(fd);
      //log_debug("Abstract_Socket::Handle_Socket_Event(). Client %d closed connection caused by zero messagelength", fd);
      switch (client_data->reading_state) {
        case STATE_BLOCK_FOR_SENDING:
          //log_debug("Abstract_Socket::Handle_Socket_Event(): state is STATE_BLOCK_FOR_SENDING, don't close connection.");
          Remove_Fd_Read_Handler(fd);
          client_data->reading_state = STATE_DONT_CLOSE;
          //log_debug("Abstract_Socket::Handle_Socket_Event(): setting socket state to STATE_DONT_CLOSE");
          break;
        case STATE_DONT_CLOSE:
          //log_debug("Abstract_Socket::Handle_Socket_Event(): state is STATE_DONT_CLOSE, don't close connection.");
          break;
        default:
          if((client_data->tcp_state == CLOSE_WAIT) || (client_data->tcp_state == FIN_WAIT)) {
            remove_client(fd);
            peer_disconnected(fd);
          } else {
            if(shutdown(fd, SHUT_RD) != 0) {
              if(errno == ENOTCONN) {
                remove_client(fd);
                peer_disconnected(fd);
                errno = 0;
              } else
                log_error("shutdown(SHUT_RD) system call failed");
            } else {
              client_data->tcp_state = CLOSE_WAIT;
	      Remove_Fd_Read_Handler(fd);
              peer_half_closed(fd);
            }
          }
      } // switch (client_data->reading_state)
    } else if (messageLength > 0) {
      as_client_struct *client_data=get_peer(fd);
      if (socket_debugging) {
        struct sockaddr_storage clientAddr = client_data->clientAddr;
#ifdef WIN32
        log_debug("Message received from address %s:%d", inet_ntoa(((struct sockaddr_in*)&clientAddr)->sin_addr), ntohs(((struct sockaddr_in *)&clientAddr)->sin_port));
#else
        char hname[NI_MAXHOST];
        char sname[NI_MAXSERV];
#if defined LINUX || defined FREEBSD || defined SOLARIS8
      socklen_t
#else /* SOLARIS or WIN32 */
        int
#endif
        clientAddrlen = client_data->clientAddrlen;
        int error = getnameinfo((struct sockaddr *)&clientAddr, clientAddrlen,
 	  hname, sizeof (hname), sname, sizeof (sname), NI_NUMERICHOST|NI_NUMERICSERV);
        if (error) 
              log_error("AbstractSocket: getnameinfo 2: %s\n", gai_strerror(error));
          log_debug("Message received from address (addr) %s/%s", hname, sname);
#endif
      }
      //log_hex("Message received, buffer content: ", get_buffer(fd)->get_data(), get_buffer(fd)->get_len());
      handle_message(fd);
    } /* else if (messageLength == -2) =>
          used in case of SSL: means that reading would bloc.
          in this case I stop receiving message on the file descriptor */
  } // if ... (not new connection request)

  if (fd == listen_fd && is_readable) {
    // new connection request arrived
    log_debug("waiting for accept");
    // receiving new connection on the TCP server
    struct sockaddr_storage clientAddr;

#if defined LINUX || defined FREEBSD || defined SOLARIS8
    socklen_t
#else /* SOLARIS or WIN32 */
      int
#endif
      clientAddrlen = sizeof(clientAddr);
#if defined LINUX || defined FREEBSD || defined SOLARIS8
    int newclient_fd  = accept(listen_fd, (struct sockaddr *) &clientAddr, (socklen_t*)&clientAddrlen);
#else
    int newclient_fd  = accept(listen_fd, (struct sockaddr *) &clientAddr, (int*)&clientAddrlen);
#endif
    if(newclient_fd < 0) log_error("Cannot accept connection at port");

    as_client_struct *client_data=peer_list_add_peer(newclient_fd);
    Add_Fd_Read_Handler(newclient_fd); // Done here - as in case of error: remove_client expects the handler as added
    log_debug("Abstract_Socket::Handle_Socket_Event(). Handler set to other fd %d", newclient_fd);
    client_data->fd_buff = new TTCN_Buffer;
    client_data->clientAddr = clientAddr;
    client_data->clientAddrlen = clientAddrlen;
    client_data->tcp_state = ESTABLISHED;
    client_data->reading_state = STATE_NORMAL;
    if (add_user_data(newclient_fd)) {
      char hname[NI_MAXHOST];
      int clientPort = 0;
#ifdef WIN32
      clientPort=ntohs(((struct sockaddr_in *)&clientAddr)->sin_port);
      char* tmp=inet_ntoa(((struct sockaddr_in*)&clientAddr)->sin_addr);
      strcpy(hname,tmp);
#else
      int error;
      char sname[NI_MAXSERV];
      error = getnameinfo((struct sockaddr *)&clientAddr, clientAddrlen,
 	  hname, sizeof (hname), sname, sizeof (sname), NI_NUMERICHOST|NI_NUMERICSERV);
      if (error) {
 	      log_error("AbstractSocket: getnameinfo: %s\n",
		  gai_strerror(error));
      }
              clientPort = atoi(sname);
#endif
      log_debug("Client %d connected from address %s/%d", newclient_fd, hname, clientPort);
      peer_connected(newclient_fd, hname, clientPort);
      peer_connected(newclient_fd, *((struct sockaddr_in *)&clientAddr)); /* calling deprecated function also */
      log_debug("Handle_Socket_Event updated with client %d ", newclient_fd);
      
      if (set_non_block_mode(newclient_fd, use_non_blocking_socket) < 0) {
        log_error("Set blocking mode failed.");
      }
      
    } else {
      remove_client(newclient_fd);
      peer_disconnected(newclient_fd);
    }
  } // if (fd == listen_fd && is_readable)

  //log_debug("leaving Abstract_Socket::Handle_Socket_Event()");
}

int Abstract_Socket::receive_message_on_fd(int client_id)
{  //log_debug("Receive message from client ID: %d", client_id);
  as_client_struct * client_data = get_peer(client_id);
  TTCN_Buffer* recv_tb = client_data->fd_buff;
  unsigned char *end_ptr;
  size_t end_len=AS_TCP_CHUNCK_SIZE;
  recv_tb->get_end(end_ptr, end_len);
  int messageLength = recv(client_id, (char *)end_ptr, end_len, 0);
  if (messageLength==0) return messageLength; // peer disconnected
  else if (messageLength < 0) {
    log_warning("Error when reading the received TCP PDU: %s", strerror(errno));
    errno = 0;
    return 0;
  }
  recv_tb->increase_length(messageLength);
  return messageLength;
}

int Abstract_Socket::send_message_on_fd(int client_id, const unsigned char* send_par, int message_length)
{
  get_peer(client_id);
  return send(client_id, (const char *)send_par, message_length, 0);
}


//Tthe EAGAIN errno value set by the send operation means that
//the sending operation would block.
//First I try to increase the length of the sending buffer (increase_send_buffer()).
//If the outgoing buffer cannot be increased, the block_for_sending function will
//be called. This function will block until the file descriptor given as its argument
//is ready to write. While the block for sending operation calls the Event_Handler,
//states must be used to indicate that the Event_Handler is called when the
//execution is blocking.
//STATE_BLOCK_FOR_SENDING: the block for sending operation has been called
//STATE_DONT_CLOSE: if the other side close the connection before the block_for_sending
//                  operation returns, in the Event_Handler the connection
//                  must not be closed and the block_for_sending must return before we can
//                  close the connection. This state means that the other side closed the connection
//                  during the block_for_sending operation
//STATE_NORMAL: normal state
int Abstract_Socket::send_message_on_nonblocking_fd(int client_id,
                                                const unsigned char* send_par,
                                                int length){

    log_debug("entering Abstract_Socket::" "send_message_on_nonblocking_fd(id: %d)", client_id);
    as_client_struct * client_data;
    int sent_len = 0;
    while(sent_len < length){
        int ret;
        log_debug("Abstract_Socket::send_message_on_nonblocking_fd(id: %d): new iteration", client_id);
        client_data = get_peer(client_id);
        if (client_data->reading_state == STATE_DONT_CLOSE){
            goto client_closed_connection;
        } else ret = send(client_id, send_par + sent_len, length - sent_len, 0);

        if (ret > 0) sent_len+=ret;
        else{
            switch(errno){
                case EINTR:{ //signal: do nothing, try again
                    errno = 0;
                    break;
                }
                case EPIPE:{ //client closed connection
                    goto client_closed_connection;
                }
                case EAGAIN:{ // the output buffer is full:
                              //try to increase it if possible
                    errno = 0;
	            int old_bufsize, new_bufsize;

                    if (increase_send_buffer(
                            client_id, old_bufsize, new_bufsize)) {
                        log_warning("Sending data on on file descriptor %d",
                                                    client_id);
                        log_warning("The sending operation would"
                                                "block execution. The size of the "
                                                "outgoing buffer was increased from %d to "
	                            "%d bytes.",old_bufsize,
	                             new_bufsize);
                    } else {
                        log_warning("Sending data on file descriptor %d",
                                                client_id);
                        log_warning("The sending operation would block "
                                                "execution and it is not possible to "
                                                "further increase the size of the "
	                            "outgoing buffer. Trying to process incoming"
                                                "data to avoid deadlock.");
                        log_debug("Abstract_Socket::"
                                  "send_message_on_nonblocking_fd():"
                                  " setting socket state to "
                                  "STATE_BLOCK_FOR_SENDING");
                        client_data->reading_state = STATE_BLOCK_FOR_SENDING;
                        TTCN_Snapshot::block_for_sending(client_id);
                    }
                    break;
                }
                default:{
                    log_debug("Abstract_Socket::"
                                "send_message_on_nonblocking_fd(): "
                                "setting socket state to STATE_NORMAL");
                    client_data->reading_state = STATE_NORMAL;
                    log_debug("leaving Abstract_Socket::"
                                "send_message_on_nonblocking_fd(id: %d)"
                                " with error", client_id);
                    return -1;
                }
             } //end of switch
         }//end of else
      } //end of while

      log_debug("Abstract_Socket::send_message_on_nonblocking_fd():"
                    "setting socket state to STATE_NORMAL");
      client_data->reading_state = STATE_NORMAL;
      log_debug("leaving Abstract_Socket::"
                    "send_message_on_nonblocking_fd(id: %d)", client_id);
      return sent_len;

client_closed_connection:
    log_debug("Abstract_Socket::send_message_on_nonblocking_fd(): setting socket state to STATE_NORMAL");
    client_data->reading_state = STATE_NORMAL;
    log_debug("leaving Abstract_Socket::"
                "send_message_on_nonblocking_fd(id: %d)", client_id);
    errno = EPIPE;
    return -1;
}

const PacketHeaderDescr* Abstract_Socket::Get_Header_Descriptor() const
{
  return NULL;
}

void Abstract_Socket::peer_connected(int /*client_id*/, sockaddr_in& /*remote_addr*/)
{
}

void Abstract_Socket::handle_message(int client_id)
{
  const PacketHeaderDescr* head_descr = Get_Header_Descriptor();
  as_client_struct * client_data = get_peer(client_id);
  TTCN_Buffer *recv_tb = client_data->fd_buff;

  if(!head_descr){
    message_incoming(recv_tb->get_data(), recv_tb->get_len(), client_id);
    if (!ttcn_buffer_usercontrol) recv_tb->clear();
  } else {
    recv_tb->rewind();
    unsigned long valid_header_length = head_descr->Get_Valid_Header_Length();
    while (recv_tb->get_len() > 0) {
      if ((unsigned long)recv_tb->get_len() < valid_header_length) {
        // this is a message without a valid header
        // recv_tb->handle_fragment();
        return;
      }
      unsigned long message_length =
	head_descr->Get_Message_Length(recv_tb->get_data());
      if (message_length < valid_header_length) {
        // this is a message with a malformed length
	log_error("Malformed message: invalid length: %lu. The length should "
	  "be at least %lu.", message_length, valid_header_length);
      }
      if((unsigned long)recv_tb->get_len() < message_length){
        // this is a fragmented message with a valid header
        // recv_tb->handle_fragment();
        return;
      }
      // this a valid message
      message_incoming(recv_tb->get_data(), message_length, client_id);
      if (!ttcn_buffer_usercontrol) {
         recv_tb->set_pos(message_length);
         recv_tb->cut();
      }
    }
  }
  log_debug("leaving Abstract_Socket::handle_message()");
}

void Abstract_Socket::map_user()
{
  log_debug("entering Abstract_Socket::map_user()");
#if defined SOLARIS8
  sigignore(SIGPIPE);
#endif
  if(!use_connection_ASPs)
  {
    // If halt_on_connection_reset is not set explicitly
    // set it to the default value: true on clients, false on servers
    if (!halt_on_connection_reset_set) {
      if (local_port_number != 0) halt_on_connection_reset=false;
      else halt_on_connection_reset=true;
    }
  }

  all_mandatory_configparameters_present();

  char remotePort[6];
  char localPort[6];
  sprintf(localPort, "%u", local_port_number);
  sprintf(remotePort, "%u", remote_port_number);

  if(!use_connection_ASPs)
  {
    if(server_mode) {
      //open_listen_port(localAddr);
      open_listen_port(local_host_name,(char*)&localPort);
    } else {
      //open_client_connection(remoteAddr, localAddr);
      open_client_connection(remote_host_name,(char*)&remotePort,local_host_name,(char*)&localPort);
    }
  }

  log_debug("leaving Abstract_Socket::map_user()");
}

int Abstract_Socket::open_listen_port(const struct sockaddr_in & new_local_addr)
{
#ifndef WIN32
  log_debug("**** DEPRECATED FUNCTION CALLED: Abstract_Socket::open_listen_port(const struct sockaddr_in & new_local_addr)."
    " USE Abstract_Socket::open_listen_port(const char* localHostname, const char* localServicename) INSTEAD! ****");
#endif
  log_debug("Local address: %s:%d", inet_ntoa(new_local_addr.sin_addr), ntohs(new_local_addr.sin_port));

  close_listen_port();

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_fd<0) {
    if(use_connection_ASPs)
    {
      log_warning("Cannot open socket when trying to open the listen port: %s", strerror(errno));
      listen_port_opened(-1);
      errno = 0;
      return -1;
    }
    else log_error("Cannot open socket");
  }

  if(!nagling) {
    int on = 1;
    setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
  }
  int val = 1;
  if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val)) < 0) {
    close(listen_fd);
    listen_fd = -1;
    if(use_connection_ASPs)
    {
      log_warning("Setsockopt failed when trying to open the listen port: %s", strerror(errno));
      listen_port_opened(-1);
      errno = 0;
      return -1;
    }
    else log_error("Setsockopt failed");
  }

  int rc = 0;

  log_debug("Bind to port...");
  rc = bind(listen_fd, (const struct sockaddr *)&new_local_addr, sizeof(new_local_addr));
  if(rc<0) {
    close(listen_fd);
    listen_fd = -1;
    if(use_connection_ASPs)
    {
      log_warning("Cannot bind to port when trying to open the listen port: %s", strerror(errno));
      listen_port_opened(-1);
      errno = 0;
      return -1;
    }
    else log_error("Cannot bind to port");
  }
  log_debug("Bind successful on server.");

  rc = listen(listen_fd, server_backlog);
  if(rc<0) {
    close(listen_fd);
    listen_fd = -1;
    if(use_connection_ASPs)
    {
      log_warning("Cannot listen at port when trying to open the listen port: %s", strerror(errno));
      listen_port_opened(-1);
      errno = 0;
      return -1;
    }
    else log_error("Cannot listen at port");
  }

  // to avoid dead-locks and make possible
  // handling of multiple clients "accept" is placed in the Event_Handler

#if defined LINUX || defined FREEBSD || defined SOLARIS8
  socklen_t
#else /* SOLARIS or WIN32 */
  int
#endif
  addr_len = sizeof(new_local_addr);
  if (getsockname(listen_fd, (struct sockaddr*)&new_local_addr, &addr_len)) {
    close(listen_fd);
    listen_fd = -1;
    if(use_connection_ASPs)
    {
      log_warning("getsockname() system call failed on the server socket when trying to open the listen port: %s", strerror(errno));
      listen_port_opened(-1);
      errno = 0;
      return -1;
    }
    else log_error("getsockname() system call failed on the server socket");
  }
  log_debug("Listen successful on server port %d", ntohs(new_local_addr.sin_port));

  Add_Fd_Read_Handler(listen_fd); // Done here - after all error checks: as closed fd should not be left added
  log_debug("Abstract_Socket::open_listen_port(): Handler set to socket fd %d", listen_fd);

  //localAddr = new_local_addr;

  if(use_connection_ASPs)
    listen_port_opened(ntohs(new_local_addr.sin_port));

  return new_local_addr.sin_port;
}

int Abstract_Socket::open_listen_port(const char* localHostname, const char* localServicename) {
  log_debug("Local address: %s/%s", (localHostname!=NULL)?localHostname:"UNSPEC",(localServicename!=NULL)?localServicename:"UNSPEC");

#ifdef WIN32
  struct sockaddr_in new_local_addr;
  memset(&new_local_addr, 0, sizeof(new_local_addr));
  if(localHostname!=NULL){
    get_host_id(localHostname,&new_local_addr);
  }
  if(localServicename!=NULL){
    new_local_addr.sin_port=htons(atoi(localServicename));
  }
  return open_listen_port(new_local_addr);
#else
  close_listen_port();
  
  struct addrinfo		*aip;
  struct addrinfo		hints;
  int			sock_opt;
  int			error;

  /* Set up a socket to listen for connections. */
  bzero(&hints, sizeof (hints));
  hints.ai_flags = /*AI_ALL|*/AI_ADDRCONFIG|AI_PASSIVE;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = ai_family;

  error = getaddrinfo(localHostname, localServicename, &hints, &aip);
  if (error != 0) {
    if(use_connection_ASPs)
    {
      log_warning("getaddrinfo: %s for host %s service %s", gai_strerror(error),
        (localHostname!=NULL)?localHostname:"UNSPEC",(localServicename!=NULL)?localServicename:"UNSPEC");
      listen_port_opened(-1);
      return -1;
    }
    else log_error("getaddrinfo: %s for host %s service %s", gai_strerror(error),
      (localHostname!=NULL)?localHostname:"UNSPEC",(localServicename!=NULL)?localServicename:"UNSPEC");
  }

  struct addrinfo		*res;
  if (socket_debugging) {
    /* count the returned addresses: */
    int counter = 0;
    for (res = aip; res != NULL; res = res->ai_next,++counter) {};
    log_debug("Number of local addresses: %d\n", counter);
  }


  for (res = aip; res != NULL; res = res->ai_next) {
    listen_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    log_debug("Waiting for connection via: %s\n",
 	        ((res->ai_family==AF_INET)?"IPv4":
                  ((res->ai_family==AF_INET6)?"IPv6":"unknown")));
    if (listen_fd == -1) {
      if(use_connection_ASPs)
      {
        log_warning("Cannot open socket when trying to open the listen port: %s", strerror(errno));
        listen_port_opened(-1);
        errno = 0;
        freeaddrinfo(aip);
        return -1;
      }
      else log_error("Cannot open socket");
    }

    /* Tell the system to allow local addresses to be reused. */
    sock_opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&sock_opt,
        sizeof (sock_opt)) == -1) {

      close(listen_fd);
      listen_fd = -1;
      if(use_connection_ASPs)
      {
        log_warning("Setsockopt failed when trying to open the listen port: %s", strerror(errno));
        listen_port_opened(-1);
        errno = 0;
        freeaddrinfo(aip);
        return -1;
      }
      else log_error("Setsockopt failed");
    }

    if(!nagling) {
      int on = 1;
      setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
    }

    log_debug("Bind to port...");
    if (bind(listen_fd, res->ai_addr, res->ai_addrlen) == -1) {
      error = errno; // save it for the warning message
      close(listen_fd);
      listen_fd = -1;
      log_debug("Cannot bind to port when trying to open the listen port: %s", strerror(errno));
      errno = 0;
      continue;
    }
    log_debug("Bind successful on server.");
    break;
  }
  if (res==NULL) {
    if(use_connection_ASPs)
    {
      log_warning("Cannot bind to port when trying to open the listen port: %s", strerror(error));
      listen_port_opened(-1);
      error = 0;
        freeaddrinfo(aip);
      return -1;
    }
    else log_error("Cannot bind to port");
  }

  if (listen(listen_fd, server_backlog) == -1) {
    close(listen_fd);
    listen_fd = -1;
    if(use_connection_ASPs)
    {
      log_warning("Cannot listen at port when trying to open the listen port: %s", strerror(errno));
      listen_port_opened(-1);
      errno = 0;
        freeaddrinfo(aip);
      return -1;
    }
    else log_error("Cannot listen at port");
  }
  

  // to avoid dead-locks and make possible
  // handling of multiple clients "accept" is placed in Handle_Socket_Event

  // to determine the local address:
  if (getsockname(listen_fd, res->ai_addr, &res->ai_addrlen)) {
    close(listen_fd);
    listen_fd = -1;
    if(use_connection_ASPs)
    {
      log_warning("getsockname() system call failed on the server socket when trying to open the listen port: %s", strerror(errno));
      listen_port_opened(-1);
      errno = 0;
        freeaddrinfo(aip);
      return -1;
    }
    else log_error("getsockname() system call failed on the server socket");
  }
  char			hname[NI_MAXHOST];
  char			sname[NI_MAXSERV];
/*  error = getnameinfo(res->ai_addr, res->ai_addrlen,
      hname, sizeof (hname), sname, sizeof (sname), NI_NUMERICSERV);
  if (error) {
    close(listen_fd);
    listen_fd = -1;
    if(use_connection_ASPs)
    {
      log_warning("getnameinfo() system call failed on the server socket when trying to open the listen port: %s", gai_strerror(error));
      listen_port_opened(-1);
        freeaddrinfo(aip);
      return -1;
    }
    else log_error("getsockname() system call failed on the server socket");
  } else {
 	  log_debug("Listening on (name): %s/%s\n",
 	      hname, sname);
  }*/
  error = getnameinfo(res->ai_addr, res->ai_addrlen,
      hname, sizeof (hname), sname, sizeof (sname), NI_NUMERICHOST|NI_NUMERICSERV);
  if (error) {
    close(listen_fd);
    listen_fd = -1;
    if(use_connection_ASPs)
    {
      log_warning("getnameinfo() system call failed on the server socket when trying to open the listen port: %s", gai_strerror(error));
      listen_port_opened(-1);
        freeaddrinfo(aip);
      return -1;
    }
    else log_error("getsockname() system call failed on the server socket");
  } else {
 	  log_debug("Listening on (addr): %s/%s\n",
 	      hname, sname);
  }

  Add_Fd_Read_Handler(listen_fd); // Done here - after all error checks: as closed fd should not be left added
  log_debug("Abstract_Socket::open_listen_port(): Handler set to socket fd %d", listen_fd);

  log_debug("new_local_addr Addr family: %s\n",
    ((res->ai_addr->sa_family==AF_INET)?"IPv4":
              ((res->ai_addr->sa_family==AF_INET6)?"IPv6":"unknown"))
  );
  

  int listenPort = atoi(sname);
  if(use_connection_ASPs)
    listen_port_opened(listenPort);

        freeaddrinfo(aip);
  return listenPort;
#endif
}

void Abstract_Socket::listen_port_opened(int /*port_number*/)
{
  // Intentionally blank
}

void Abstract_Socket::close_listen_port()
{
  // close current listening port if it is alive
  if(listen_fd != -1)
  {
    Remove_Fd_Read_Handler(listen_fd);
    close(listen_fd);
    log_debug("Closed listening port of fd: %d", listen_fd);
    listen_fd = -1;
  }
}

int Abstract_Socket::get_socket_fd() const{
  if(server_mode) return listen_fd;
  if(peer_list_get_nr_of_peers()==0) return -1;
  return peer_list_get_first_peer();
}

int Abstract_Socket::open_client_connection(const struct sockaddr_in & new_remote_addr, const struct sockaddr_in & new_local_addr)
{
#ifdef WIN32
  log_debug("**** DEPRECATED FUNCTION CALLED: Abstract_Socket::open_client_connection(const struct sockaddr_in & new_remote_addr, const struct sockaddr_in & new_local_addr)."
    " USE open_client_connection(const char* remoteHostname, const char* remoteServicename, const char* localHostname, const char* localServicename) INSTEAD! ****");
#endif
  log_debug("Remote address: %s:%d", inet_ntoa(new_remote_addr.sin_addr), ntohs(new_remote_addr.sin_port));

  int deadlock_counter = AS_DEADLOCK_COUNTER;
  int TCP_reconnect_counter = TCP_reconnect_attempts;

  // workaround for the 'address already used' bug
  // used also when TCP reconnect is used
  as_start_connecting:

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd<0) {
      if(use_connection_ASPs)
      {
        log_warning("Cannot open socket when trying to open client connection: %s", strerror(errno));
        client_connection_opened(-1);
        errno = 0;
        return -1;
      }
      else log_error("Cannot open socket.");
    }

    if(!nagling) {
      int on = 1;
      setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
    }

    int rc;

    // when using client mode there is no separate file_desriptor for listening and target
    log_debug("Connecting to server from address %s:%d", inet_ntoa(new_local_addr.sin_addr), ntohs(new_local_addr.sin_port));
    if (new_local_addr.sin_port != ntohs(0)) { // specific port to use
      int val = 1;
      if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val)) < 0) {
        if(use_connection_ASPs)
        {
          log_warning("Setsockopt failed when trying to open client connection: %s", strerror(errno));
          client_connection_opened(-1);
          errno = 0;
          return -1;
        }
        else log_error("Setsockopt failed.");
      }
      rc = bind(socket_fd, (const struct sockaddr *)&new_local_addr, sizeof(new_local_addr));
      if(rc<0) {
        if(use_connection_ASPs)
        {
          log_warning("Cannot bind to port when trying to open client connection: %s", strerror(errno));
          client_connection_opened(-1);
          errno = 0;
          return -1;
        }
        else log_error("Cannot bind to port.");
      }
      log_debug("Bind successful on client.");
    }
    rc = connect(socket_fd, (const struct sockaddr *)&new_remote_addr, sizeof(new_remote_addr));

    if(rc<0){
      if (errno == EADDRINUSE) {
        log_warning("connect() returned error code EADDRINUSE. Perhaps this is a kernel bug. Trying to connect again.");
        close(socket_fd);
        errno = 0;
        deadlock_counter--;
        if (deadlock_counter<0) {
          if(use_connection_ASPs)
          {
            log_warning("Already tried %d times, giving up when trying to open client connection: %s", AS_DEADLOCK_COUNTER, strerror(errno));
            client_connection_opened(-1);
            errno = 0;
            return -1;
          }
          else log_error("Already tried %d times, giving up", AS_DEADLOCK_COUNTER);
        }
        goto as_start_connecting;
      } else if (client_TCP_reconnect && errno != 0) {
        log_warning("connect() returned error code %d, trying to connect again (TCP reconnect mode).", errno);
        close(socket_fd);
        errno = 0;
        TCP_reconnect_counter--;
        if (TCP_reconnect_counter<0) {
          if(use_connection_ASPs)
          {
            log_warning("Already tried %d times, giving up when trying to open client connection: %s", TCP_reconnect_attempts, strerror(errno));
            client_connection_opened(-1);
            errno = 0;
            return -1;
          }
          else log_error("Already tried %d times, giving up", TCP_reconnect_attempts);
        }
        sleep(TCP_reconnect_delay);
        goto as_start_connecting;
      }

      if(use_connection_ASPs)
      {
        log_warning("Cannot connect to server when trying to open client connection: %s", strerror(errno));
        client_connection_opened(-1);
        errno = 0;
        return -1;
      }
      else log_error("Cannot connect to server");
    }

    // Non-blocking mode is set before updating bookkeping to handle the error case properly.
    if (set_non_block_mode(socket_fd, use_non_blocking_socket) < 0){
        close(socket_fd);
        if (use_connection_ASPs){
            client_connection_opened(-1);
            errno = 0;
            return -1;
        }
        else log_error("Set blocking mode failed.");
    }

    as_client_struct * client_data=peer_list_add_peer(socket_fd);
    Add_Fd_Read_Handler(socket_fd); // Done here - as in case of error: remove_client expects the handler as added
    log_debug("Abstract_Socket::open_client_connection(). Handler set to socket fd %d", socket_fd);
    client_data->fd_buff = new TTCN_Buffer;
//    client_data->clientAddr = *(struct sockaddr_storage*)&new_remote_addr;
    memset(&client_data->clientAddr,0,sizeof(client_data->clientAddr));
    memcpy(&client_data->clientAddr,&new_remote_addr,sizeof(new_remote_addr));
    client_data->clientAddrlen = sizeof(new_remote_addr);
    client_data->tcp_state = ESTABLISHED;
    client_data->reading_state = STATE_NORMAL;
    if (!add_user_data(socket_fd)) {
      remove_client(socket_fd);
      peer_disconnected(socket_fd);
      return -1;
    }


//    localAddr = new_local_addr;
//    remoteAddr = new_remote_addr;

    client_connection_opened(socket_fd);

    return socket_fd;
}

int Abstract_Socket::open_client_connection(const char* remoteHostname, const char* remoteServicename, const char* localHostname, const char* localServicename) {
  log_debug("Abstract_Socket::open_client_connection(remoteAddr: %s/%s, localAddr: %s/%s) called",
   remoteHostname,remoteServicename,
   (localHostname!=NULL)?localHostname:"UNSPEC",(localServicename!=NULL)?localServicename:"UNSPEC");
#ifdef WIN32
  struct sockaddr_in new_local_addr;
  struct sockaddr_in new_remote_addr;
  memset(&new_local_addr, 0, sizeof(new_local_addr));
  memset(&new_local_addr, 0, sizeof(new_remote_addr));
  if(localHostname!=NULL){
    get_host_id(localHostname,&new_local_addr);
  }
  if(localServicename!=NULL){
    new_local_addr.sin_port=htons(atoi(localServicename));
  }
  if(remoteHostname!=NULL){
    get_host_id(remoteHostname,&new_remote_addr);
  }
  if(remoteServicename!=NULL){
    new_remote_addr.sin_port=htons(atoi(remoteServicename));
  }
  return open_client_connection(new_remote_addr,new_local_addr);
#else

  int deadlock_counter = AS_DEADLOCK_COUNTER;
  int TCP_reconnect_counter = TCP_reconnect_attempts;


  struct addrinfo *res, *aip;
  struct addrinfo hints;
  int socket_fd = -1;
  int    error;

  /* Get host address.  Any type of address will do. */
  bzero(&hints, sizeof (hints));
  hints.ai_flags = AI_ADDRCONFIG; /* |AI_ALL*/
  if (localHostname!=NULL || localServicename!=NULL) { /* use specific local address */
    hints.ai_flags |= AI_PASSIVE;
  }
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = ai_family;

  error = getaddrinfo(remoteHostname, remoteServicename, &hints, &res);
  if (error != 0) {
      if(use_connection_ASPs)
      {
        log_warning("getaddrinfo: %s for host %s service %s",
         gai_strerror(error), remoteHostname, remoteServicename);
        client_connection_opened(-1);
        return -1;
      }
      else { log_error("getaddrinfo: %s for host %s service %s",
         gai_strerror(error), remoteHostname, remoteServicename);
      }
  }

  if (socket_debugging) {
    /* count the returned addresses: */
    int counter = 0;
    for (aip = res; aip != NULL; aip = aip->ai_next,++counter) {};
    log_debug("Number of remote addresses: %d\n", counter);
  }

  // workaround for the 'address already used' bug
  // used also when TCP reconnect is used
  as_start_connecting:

 /* Try all returned addresses until one works */
  for (aip = res; aip != NULL; aip = aip->ai_next) {
    /*
     * Open socket.  The address type depends on what
     * getaddrinfo() gave us.
     */
    socket_fd = socket(aip->ai_family, aip->ai_socktype,
        aip->ai_protocol);
    if (socket_fd == -1) {
      if(use_connection_ASPs)
      {
        log_warning("Cannot open socket when trying to open client connection: %s", strerror(errno));
        client_connection_opened(-1);
        freeaddrinfo(res);
        return -1;
      }
      else {
        freeaddrinfo(res);
        log_error("Cannot open socket.");
      }
    }
    
    log_debug("Using address family for socket %d: %s",socket_fd,
      ((aip->ai_family==AF_INET)?"IPv4":
       ((aip->ai_family==AF_INET6)?"IPv6":"unknown"))
    );


    if(!nagling) {
      int on = 1;
      setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
    }

    // when using client mode there is no separate file_descriptor for listening and target
    //log_debug("Connecting to server from address %s/%s", (localHostname!=NULL)?localHostname:"UNSPEC",(localServicename!=NULL)?localServicename:"UNSPEC");
    if (localHostname!=NULL || localServicename!=NULL) { // specific localaddress/port to use
      int val = 1;
      if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val)) < 0) {
        if(use_connection_ASPs)
        {
          log_warning("Setsockopt failed when trying to open client connection: %s", strerror(errno));
          client_connection_opened(-1);
          errno = 0;
          return -1;
        }
        else log_error("Setsockopt failed.");
      }

      // determine the local address:
      struct addrinfo *localAddrinfo;
      /* Get host address.  Any type of address will do. */
      bzero(&hints, sizeof (hints));
      hints.ai_flags = AI_PASSIVE;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_family = ai_family;//aip->ai_family; // NOTE: On solaris 10 if is set to aip->ai_family, getaddrinfo will crash for IPv4-mapped addresses!

      error = getaddrinfo(localHostname, localServicename, &hints, &localAddrinfo);
      if (error != 0) {
          if(use_connection_ASPs)
          {
            log_warning("getaddrinfo: %s for host %s service %s",
             gai_strerror(error), (localHostname!=NULL)?localHostname:"UNSPEC",(localServicename!=NULL)?localServicename:"UNSPEC");
            client_connection_opened(-1);
            return -1;
          }
          else { log_error("getaddrinfo: %s for host %s service %s",
             gai_strerror(error), (localHostname!=NULL)?localHostname:"UNSPEC",(localServicename!=NULL)?localServicename:"UNSPEC");
          }
      }

      if (socket_debugging) {
        /* count the returned addresses: */
        int counter = 0;
        for (struct addrinfo* aip2 = localAddrinfo; aip2 != NULL; aip2 = aip2->ai_next,++counter) {};
        log_debug("Number of local addresses: %d\n", counter);
      }

      /* Try all returned addresses until one works */
      struct addrinfo* aip2;
      for (aip2 = localAddrinfo; aip2 != NULL; aip2 = aip2->ai_next) {
        log_debug("Using address family for bind: %s",
          ((aip2->ai_family==AF_INET)?"IPv4":
           ((aip2->ai_family==AF_INET6)?"IPv6":"unknown"))
        );

        if(bind(socket_fd, aip2->ai_addr, aip2->ai_addrlen)<0) {
/*          if(use_connection_ASPs) // the if else branches are the same
          {*/
            log_debug("Cannot bind to port when trying to open client connection: %s", strerror(errno));
            //client_connection_opened(-1);
            //freeaddrinfo(localAddrinfo);
            errno = 0;
            continue; //aip2 cycle
            //return -1;
/*          }
          else {
            //freeaddrinfo(localAddrinfo);
            //log_error("Cannot bind to port.");
            log_debug("Cannot bind to port when trying to open client connection: %s", strerror(errno));
            errno = 0;
            continue; //aip2 cycle
          }*/
        }
        log_debug("Bind successful on client.");
        freeaddrinfo(localAddrinfo);
        break;
      }
      if (aip2==NULL) {
        log_debug("Bind failed for all local addresses.");
        freeaddrinfo(localAddrinfo);
        continue; // aip cycle
      }
    }

    /* Connect to the host. */
    if (connect(socket_fd, aip->ai_addr, aip->ai_addrlen) == -1) {
      if (errno == EADDRINUSE) {
        log_warning("connect() returned error code EADDRINUSE. Perhaps this is a kernel bug. Trying to connect again.");
        close(socket_fd);
        socket_fd = -1;
        errno = 0;
        deadlock_counter--;
        if (deadlock_counter<0) {
          if(use_connection_ASPs)
          {
            log_warning("Already tried %d times, giving up when trying to open client connection: %s", AS_DEADLOCK_COUNTER, strerror(errno));
            client_connection_opened(-1);
            errno = 0;
            return -1;
          }
          else log_error("Already tried %d times, giving up", AS_DEADLOCK_COUNTER);
        }
        goto as_start_connecting;
      } else if (client_TCP_reconnect && errno != 0) {
        log_warning("connect() returned error code %d (%s), trying to connect again (TCP reconnect mode).", errno, strerror(errno));
        close(socket_fd);
        socket_fd = -1;
        errno = 0;
        if (aip->ai_next==NULL) { /* Last address is tried and there is still an error */
          TCP_reconnect_counter--;
          if (TCP_reconnect_counter<0) {
            if(use_connection_ASPs)
            {
              log_warning("Already tried %d times, giving up when trying to open client connection: %s", TCP_reconnect_attempts, strerror(errno));
              client_connection_opened(-1);
              errno = 0;
              return -1;
            }
            else { log_error("Already tried %d times, giving up", TCP_reconnect_attempts); }
          }
        }
        sleep(TCP_reconnect_delay);
        goto as_start_connecting;
      } else {
        log_debug("Cannot connect to server: %s", strerror(errno));
        (void) close(socket_fd);
        socket_fd = -1;
      }

      if (aip->ai_next==NULL) {
        if(use_connection_ASPs)
        {
          log_warning("Cannot connect to server when trying to open client connection: %s", strerror(errno));
          client_connection_opened(-1);
          errno = 0;
          return -1;
        }
        else log_error("Cannot connect to server");
      }
      continue; //aip cycle
    }

    // to determine the local address:
    if (getsockname(socket_fd, aip->ai_addr, &aip->ai_addrlen)) {
      close(socket_fd);
      if(use_connection_ASPs) {
        log_warning("getsockname() system call failed on the client socket when trying to connect to server: %s", strerror(errno));
        client_connection_opened(-1);
        errno = 0;
        return -1;
      }
      else log_error("getsockname() system call failed on the client socket when trying to connect to server: %s", strerror(errno));
    }
    char			hname[NI_MAXHOST];
    char			sname[NI_MAXSERV];
/*    error = getnameinfo(aip->ai_addr, aip->ai_addrlen,
        hname, sizeof (hname), sname, sizeof (sname), NI_NUMERICSERV);
    if (error) {
      close(socket_fd);
      if(use_connection_ASPs)
      {
        log_warning("getnameinfo() system call failed on the client socket when trying to connect to server: %s", gai_strerror(error));
        client_connection_opened(-1);
        return -1;
      }
      else log_error("getnameinfo() system call failed on the client socket when trying to connect to server: %s", gai_strerror(error));
    } else {
      log_debug("Connection established (name): %s/%s -> %s/%s\n",
          hname, sname,
          remoteHostname, remoteServicename);
    }*/
    error = getnameinfo(aip->ai_addr, aip->ai_addrlen,
        hname, sizeof (hname), sname, sizeof (sname), NI_NUMERICHOST|NI_NUMERICSERV);
    if (error) {
/*      close(socket_fd);
      if(use_connection_ASPs)
      {
        log_warning("getnameinfo() system call failed on the client socket when trying to connect to server: %s", gai_strerror(error));
//        client_connection_opened(-1);
//        return -1;
      }
      else*/
       log_warning("getnameinfo() system call failed on the client socket when trying to connect to server: %s", gai_strerror(error));
    } else {
      log_debug("Connection established (addr): %s/%s -> %s/%s\n",
          hname, sname,
          remoteHostname, remoteServicename);
    }

    log_debug(
      "connected to: host %s service %s via address family %s\n",
      remoteHostname, remoteServicename,
      ((aip->ai_family==AF_INET)?"IPv4":
        ((aip->ai_family==AF_INET6)?"IPv6":"unknown")));
    break;
  }
  if (aip==NULL) {
    if(use_connection_ASPs)
    {
      log_warning("Cannot connect to server");
      client_connection_opened(-1);
      freeaddrinfo(res);
      return -1;
    }
    else log_error("Cannot connect to server");
  }

  // Non-blocking mode is set before updating bookkeping to handle the error case properly.
  if (set_non_block_mode(socket_fd, use_non_blocking_socket) < 0) {
    freeaddrinfo(res);
    close(socket_fd);
    if (use_connection_ASPs){
        log_warning("Set blocking mode failed.");
        client_connection_opened(-1);
        errno = 0;
        return -1;
      }
      else log_error("Set blocking mode failed.");
  }

    as_client_struct * client_data=peer_list_add_peer(socket_fd);
    Add_Fd_Read_Handler(socket_fd); // Done here - as in case of error: remove_client expects the handler as added
    //log_debug("Abstract_Socket::open_client_connection(). Handler set to socket fd %d", socket_fd);
    client_data->fd_buff = new TTCN_Buffer;
//    client_data->clientAddr = *(struct sockaddr_storage*)aip->ai_addr;
    memset(&client_data->clientAddr,0,sizeof(client_data->clientAddr));
    memcpy(&client_data->clientAddr,aip->ai_addr,sizeof(*aip->ai_addr));
    client_data->clientAddrlen = aip->ai_addrlen;
    client_data->tcp_state = ESTABLISHED;
    client_data->reading_state = STATE_NORMAL;

    freeaddrinfo(res);

    if (!add_user_data(socket_fd)) {
      remove_client(socket_fd);
      peer_disconnected(socket_fd);
      return -1;
    }

    client_connection_opened(socket_fd);

    return socket_fd;
#endif
}


void Abstract_Socket::client_connection_opened(int /*client_id*/)
{
  // Intentionally blank
}

void Abstract_Socket::unmap_user()
{
  log_debug("entering Abstract_Socket::unmap_user()");
  remove_all_clients();
  close_listen_port();
  Handler_Uninstall(); // For robustness only
  log_debug("leaving Abstract_Socket::unmap_user()");
}

void Abstract_Socket::peer_disconnected(int /*fd*/)
{
  // virtual peer_disconnected() needs to be overriden in test ports!
  if(!use_connection_ASPs) {
    if (halt_on_connection_reset)
      log_error("Connection was interrupted by the other side.");
    if (client_TCP_reconnect){
        log_warning("TCP connection was interrupted by the other side, trying to reconnect again...");
        unmap_user();
        map_user();
        log_warning("TCP reconnect successfuly finished");
    }
  }
}

void Abstract_Socket::peer_half_closed(int fd)
{
  log_debug("Entering Abstract_Socket::peer_half_closed()");
  remove_client(fd);
  peer_disconnected(fd);
  log_debug("Leaving Abstract_Socket::peer_half_closed()");
}

void Abstract_Socket::send_shutdown(int client_id)
{
  log_debug("entering Abstract_Socket::send_shutdown()");
  int dest_fd = client_id;

  if (dest_fd == -1) {
    if(peer_list_get_nr_of_peers() > 1)
      log_error("Client Id not specified altough not only 1 client exists");
    else if(peer_list_get_nr_of_peers() == 0)
      log_error("[Internet Connection Error!!!]There is no connection alive, connect before sending anything.");
    dest_fd = peer_list_get_first_peer();
  }
  as_client_struct * client_data = get_peer(dest_fd);
  if(client_data->tcp_state != ESTABLISHED)
    log_error("TCP state of client nr %i does not allow to shut down its connection for writing!", dest_fd);

  if(shutdown(dest_fd, SHUT_WR) != 0)
  {
    if(errno == ENOTCONN)
    {
      remove_client(dest_fd);
      peer_disconnected(dest_fd);
      errno = 0;
    }
    else
      log_error("shutdown() system call failed");
  }
  else client_data->tcp_state = FIN_WAIT;

  // dest_fd is not removed from readfds, data can be received

  log_debug("leaving Abstract_Socket::send_shutdown()");
}

void Abstract_Socket::send_outgoing(const unsigned char* send_par, int length, int client_id)
{
  //log_debug("entering Abstract_Socket::send_outgoing()");
  //log_hex("Sending data: ", send_par, length);
  int dest_fd;
  int nrOfBytesSent;

  dest_fd = client_id;

  if (dest_fd == -1) {
    if(peer_list_get_nr_of_peers() > 1)
      log_error("Client Id not specified altough not only 1 client exists");
    else if(peer_list_get_nr_of_peers() == 0)
      log_error("[Internet Connection Error!!!]There is no connection alive, use a Connect ASP before sending anything.");
    dest_fd = peer_list_get_first_peer();
  }
  as_client_struct * client_data = get_peer(dest_fd,true);
  if(!client_data || ((client_data->tcp_state != ESTABLISHED) && (client_data->tcp_state != CLOSE_WAIT))){
    char *error_text=mprintf("client nr %i has no established connection", dest_fd);
    report_error(client_id,length,-2,send_par,error_text);
    Free(error_text);
    //log_debug("leaving Abstract_Socket::send_outgoing()");
    return;
  }

  nrOfBytesSent = use_non_blocking_socket ? send_message_on_nonblocking_fd(dest_fd, send_par, length) :
                                            send_message_on_fd(dest_fd, send_par, length);

  if (nrOfBytesSent == -1 && errno == EPIPE){  // means connection was interrupted by peer
    errno = 0;
    log_debug("Client %d closed connection", client_id);
    remove_client(dest_fd);
    peer_disconnected(dest_fd);
  }else if (nrOfBytesSent != length) {
    char *error_text=mprintf("Send system call failed: %d bytes were sent instead of %d", nrOfBytesSent, length);
    report_error(client_id,length,nrOfBytesSent,send_par,error_text);
    Free(error_text);
  } else {
    log_debug("Abstract_Socket::send_outgoing: Number of bytes sent = %d", nrOfBytesSent);
  }
  //log_debug("leaving Abstract_Socket::send_outgoing()");
}

void Abstract_Socket::report_error(int /*client_id*/, int /*msg_length*/, int /*sent_length*/, const unsigned char* /*msg*/, const char* error_text)
{
  log_error("%s",error_text);
}

void Abstract_Socket::all_mandatory_configparameters_present()
{
  if(!use_connection_ASPs)
  {
    if(server_mode) {
      if(local_port_number == 0) {
        log_error("%s is not defined in the configuration file", local_port_name());
      }
    }
    else { // client mode
      if (remote_host_name == NULL) {
        log_error("%s is not defined in the configuration file", remote_address_name());
      }
      if(remote_port_number == 0){
        log_error("%s is not defined in the configuration file", remote_port_name());
      }
    }
  }
  user_all_mandatory_configparameters_present();
}


void Abstract_Socket::get_host_id(const char* hostName, struct sockaddr_in *addr)
{
  log_debug("Abstract_Socket::get_host_id called");
  unsigned int port = addr->sin_port;
  memset(addr, 0, sizeof(*addr));
  addr->sin_family = AF_INET;
  addr->sin_port = port;
  struct hostent *hptr;
  if(strcmp("localhost", hostName) != 0)
  {
    hptr = gethostbyname(hostName);
    if (hptr != NULL) memcpy(&addr->sin_addr, hptr->h_addr_list[0], hptr->h_length);
    else log_error("The host name %s is not valid in the configuration file.", hostName);
    log_debug("The address set to %s[%s]", hptr->h_name, inet_ntoa(addr->sin_addr));
  }
  else
  {
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    log_debug("The address set to [%s]", inet_ntoa(addr->sin_addr));
  }
}

void Abstract_Socket::remove_client(int fd)
{
  log_debug("entering Abstract_Socket::remove_client(%d)", fd);
  if(fd != listen_fd) {
    get_peer(fd); // check if client exists, log_error && fail if not
    // TODO FIXME: remove the Add_Fd_Read_Handler(fd); if TITAN is fixed
    Add_Fd_Read_Handler(fd);
    Remove_Fd_All_Handlers(fd);
    remove_user_data(fd);
    delete get_peer(fd)->fd_buff;
    peer_list_remove_peer(fd);
    close(fd);
    log_debug("Removed client %d.", fd);
  }
  else log_warning("Abstract_Socket::remove_client: %d is the server listening port, can not be removed!", fd);
  log_debug("leaving Abstract_Socket::remove_client(%d)", fd);
}

void Abstract_Socket::remove_all_clients()
{
  log_debug("entering Abstract_Socket::remove_all_clients");
  for(int i = 0; peer_list_root != NULL && i < peer_list_length; i++)
  {
    if(i != listen_fd && peer_list_root[i] != NULL)
      remove_client(i);
  }
  // check if no stucked data
  while (peer_list_get_nr_of_peers()) {
     int client_id = peer_list_get_first_peer();
     if (client_id >= 0) log_warning("Client %d has not been removed, programming error", client_id);
     else log_error("Number of clients<>0 but cannot get first client, programming error");
     peer_list_remove_peer(client_id);
  }

  log_debug("leaving Abstract_Socket::remove_all_clients");
}

int Abstract_Socket::set_non_block_mode(int fd, bool enable_nonblock){

    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
	log_warning("System call fcntl(F_GETFL) failed on file "
	    "descriptor %d.", fd);
	return -1;
    }

    if (enable_nonblock) flags |= O_NONBLOCK;
    else flags &= ~O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
	log_warning("System call fcntl(F_SETFL) failed on file "
	    "descriptor %d.", fd);
	return -1;
    }
    return 0;

}

bool Abstract_Socket::increase_send_buffer(int fd,
     int &old_size, int& new_size)
{
    int set_size;
#if defined LINUX || defined FREEBSD || defined SOLARIS8
    socklen_t
#else /* SOLARIS or WIN32 */
    int
#endif
	optlen = sizeof(old_size);
    // obtaining the current buffer size first
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&old_size, &optlen))
	goto getsockopt_failure;
    if (old_size <= 0) {
	log_warning("System call getsockopt(SO_SNDBUF) "
	    "returned invalid buffer size (%d) on file descriptor %d.",
	    old_size, fd);
	return false;
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
    if (set_size <= old_size) return false;
success:
    // querying the new effective buffer size (it might be smaller
    // than set_size but should not be smaller than old_size)
    optlen = sizeof(new_size);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&new_size,
	&optlen)) goto getsockopt_failure;
    if (new_size > old_size) return true;
    else {
	if (new_size < old_size)
            log_warning("System call getsockopt(SO_SNDBUF) returned unexpected buffer size "
	    "(%d, after increasing it from %d to %d) on file descriptor %d.",
	    new_size, old_size, set_size, fd);
	return false;
    }
getsockopt_failure:
    log_warning("System call getsockopt(SO_SNDBUF) failed on file "
	"descriptor %d. (%s)", fd, strerror(errno));
    return false;
setsockopt_failure:
    log_warning("System call setsockopt(SO_SNDBUF) failed on file "
	"descriptor %d. (%s)", fd, strerror(errno));
    return false;
}


const char* Abstract_Socket::local_port_name()              { return "serverPort";}
const char* Abstract_Socket::remote_address_name()          { return "destIPAddr";}
const char* Abstract_Socket::local_address_name()           { return "serverIPAddr";}
const char* Abstract_Socket::remote_port_name()             { return "destPort";}
const char* Abstract_Socket::ai_family_name()               { return "ai_family";}
const char* Abstract_Socket::use_connection_ASPs_name()     { return "use_connection_ASPs";}
const char* Abstract_Socket::halt_on_connection_reset_name(){ return "halt_on_connection_reset";}
const char* Abstract_Socket::client_TCP_reconnect_name()	{ return "client_TCP_reconnect";}
const char* Abstract_Socket::TCP_reconnect_attempts_name()	{ return "TCP_reconnect_attempts";}
const char* Abstract_Socket::TCP_reconnect_delay_name()		{ return "TCP_reconnect_delay";}
const char* Abstract_Socket::server_mode_name()             { return "server_mode";}
const char* Abstract_Socket::socket_debugging_name()        { return "socket_debugging";}
const char* Abstract_Socket::nagling_name()                 { return "nagling";}
const char* Abstract_Socket::use_non_blocking_socket_name() { return "use_non_blocking_socket";}
const char* Abstract_Socket::server_backlog_name()          { return "server_backlog";}
bool Abstract_Socket::add_user_data(int) {return true;}
bool Abstract_Socket::remove_user_data(int) {return true;}
bool Abstract_Socket::user_all_mandatory_configparameters_present() { return true; }



////////////////////////////////////////////////////////////////////////
/////    Peer handling functions
////////////////////////////////////////////////////////////////////////

void Abstract_Socket::peer_list_reset_peer() {
   log_debug("Abstract_Socket::peer_list_reset_peer: Resetting peer array");
   for (int i = 0; i < peer_list_length; i++)
      if (peer_list_root[i] != NULL) {
        delete peer_list_root[i];
        peer_list_root[i] = NULL;
      }

   peer_list_resize_list(-1);
   log_debug("Abstract_Socket::peer_list_reset_peer: New length is %d", peer_list_length);
}

void Abstract_Socket::peer_list_resize_list(int client_id) {
   int new_length=client_id;
   if (new_length<0) new_length = peer_list_get_last_peer();
   new_length++; // index starts from 0
   //log_debug("Abstract_Socket::peer_list_resize_list: Resizing to %d", new_length);
   peer_list_root = (as_client_struct **)Realloc(peer_list_root, new_length*sizeof(as_client_struct *));

   // initialize new entries
   for (int i = peer_list_length; i < new_length; i++)
      peer_list_root[i] = NULL;

   peer_list_length = new_length;
   //log_debug("Abstract_Socket::peer_list_resize_list: New length is %d", peer_list_length);
}

int Abstract_Socket::peer_list_get_first_peer() const {
   log_debug("Abstract_Socket::peer_list_get_first_peer: Finding first peer of the peer array");
   for (int i = 0; i < peer_list_length; i++) {
      if (peer_list_root[i] != NULL) {
         log_debug("Abstract_Socket::peer_list_get_first_peer: First peer is %d", i);
         return i;
      }
   }
   log_debug("Abstract_Socket::peer_list_get_first_peer: No active peer found");
   return -1; // this indicates an empty list
}

int Abstract_Socket::peer_list_get_last_peer() const
{
   //log_debug("Abstract_Socket::peer_list_get_last_peer: Finding last peer of the peer array");
   if (peer_list_length==0) {
      log_debug("Abstract_Socket::peer_list_get_last_peer: No active peer found");
      return -1;
   }
   for (int i = peer_list_length - 1; i >= 0; i--) {
      if (peer_list_root[i] != NULL) {
         //log_debug("Abstract_Socket::peer_list_get_last_peer: Last peer is %u", i);
         return i;
      }
   }
   //log_debug("Abstract_Socket::peer_list_get_last_peer: No active peer found");
   return -1; // this indicates an empty list
}

int Abstract_Socket::peer_list_get_nr_of_peers() const
{
  int nr=0;
  for (int i = 0; i < peer_list_length; i++)
     if (peer_list_root[i] != NULL) nr++;
  //log_debug("Abstract_Socket::peer_list_get_nr_of_peers: Number of active peers = %d", nr);
  return nr;
}

Abstract_Socket::as_client_struct *Abstract_Socket::get_peer (int client_id, bool no_error) const
{
   if (client_id >= peer_list_length){
     if(no_error) return NULL;
     else log_error ("Index %d exceeds length of peer list.", client_id);
   }
   if (peer_list_root[client_id]==NULL){
     if(no_error) return NULL;
     else log_error("Abstract_Socket::get_peer: Client %d does not exist", client_id);
   }
   return peer_list_root[client_id];
}

Abstract_Socket::as_client_struct * Abstract_Socket::peer_list_add_peer (int client_id) {
  //log_debug("Abstract_Socket::peer_list_add_peer: Adding client %d to peer list", client_id);
  if (client_id<0)   log_error("Invalid Client Id is given: %d.", client_id);
  if (client_id>peer_list_get_last_peer()) peer_list_resize_list(client_id);
  peer_list_root[client_id] = new as_client_struct;
  peer_list_root[client_id]->user_data = NULL;
  peer_list_root[client_id]->fd_buff = NULL;
  peer_list_root[client_id]->tcp_state = CLOSED;
  peer_list_root[client_id]->reading_state = STATE_NORMAL;
  return peer_list_root[client_id];
}

void Abstract_Socket::peer_list_remove_peer (int client_id) {

  log_debug("Abstract_Socket::peer_list_remove_peer: Removing client %d from peer list", client_id);
  if (client_id >= peer_list_length || client_id<0)   log_error("Invalid Client Id is given: %d.", client_id);
  if (peer_list_root[client_id] == NULL) log_error("Peer %d does not exist.", client_id);

  delete peer_list_root[client_id];
  peer_list_root[client_id] = NULL;

  peer_list_resize_list(-1);
}



#ifdef AS_USE_SSL
/*
 * Server mode
 When the mode is server, first a TCP socket is created. The server starts
 to listen on this port. Once a TCP connect request is received, the TCP
 connection is setup. After this the SSL handshake begins.
 The SSL is mapped to the file descriptor of the TCP socket. The BIO is
 automatically created by OpenSSL inheriting the characteristics of the
 socket (non-blocking mode). The BIO is completely transparent.
 The server always sends its certificate to the client. If configured so,
 the server will request the certificate of the client and check if it is
 a valid certificate. If not, the SSL connection is refused.
 If configured not to verify the certificate, the server will not request
 it from the client and the SSL connection is accepted.
 If usage of the SSL ssl_session resumption is enabled and
 the client refers to a previous ssl_session, the server will accept it,
 unless it is not found in the SSL context cache.
 Once the connection is negotiated, data can be sent/received.
 The SSL connection is shutted down on an unmap() operation. The shutdown
 process does not follow the standard. The server simply shuts down and
 does not expect any acknowledgement from the client.
 Clients connected to the server are distinguished with their file
 descriptor numbers. When a message is received, the file descriptor
 number is also passed, so the client can be identified.
 * Client mode
 When the mode is client, first a TCP connection is requested to the
 server. Once accepted, the SSL endpoint is created.
 If configured so, the client tries to use the ssl_session Id from the
 previous connection, if available (e.g. not the first connection).
 If no ssl_session Id is available or the server does not accept it,
 a full handshake if performed.
 If configured so, the certificate of the server is verified.
 If the verification fails, the SSL connection is interrupted by the
 client. If no verification required, the received certificate is
 still verified, however the result does not affect the connection
 (might fail).
 * ssl_verify_certificates() is a virtual function. It is called after
 SSL connection is up. Testports may use it to check other peer's
 certificate and do actions. If the return value is 0, then the
 SSL connection is closed. In case of a client, the test port
 exits with an error (verification_error). The server just removes
 client data, but keeps running.
 If ssl_verifiycertificate == "yes", then accept connections only
 where certificate is valid
 Further checks can be done using SSL_Socket::ssl_verify_certificates()
 after the SSL connection is established with the following function call
 sequence:
 <other test port related cleanup>
 remove_client(dest_fd);
 peer_disconnected(dest_fd);

*/


// ssl_session ID context of the server
static unsigned char ssl_server_context_name[] = "McHalls&EduardWasHere";
const unsigned char * SSL_Socket::ssl_server_auth_session_id_context = ssl_server_context_name;
// Password pointer
void  *SSL_Socket::ssl_current_client = NULL;


SSL_Socket::SSL_Socket()
{
  ssl_use_ssl=false;
  ssl_initialized=false;
  ssl_key_file=NULL;
  ssl_certificate_file=NULL;
  ssl_trustedCAlist_file=NULL;
  ssl_cipher_list=NULL;
  ssl_verify_certificate=false;
  ssl_use_session_resumption=true;
  ssl_session=NULL;
  ssl_password=NULL;
  test_port_type=NULL;
  test_port_name=NULL;
  ssl_ctx = NULL;
  ssl_current_ssl = NULL;
  SSLv2=true;
  SSLv3=true;
  TLSv1=true;
  TLSv1_1=true;
  TLSv1_2=true;
}

SSL_Socket::SSL_Socket(const char *tp_type, const char *tp_name)
{
  ssl_use_ssl=false;
  ssl_initialized=false;
  ssl_key_file=NULL;
  ssl_certificate_file=NULL;
  ssl_trustedCAlist_file=NULL;
  ssl_cipher_list=NULL;
  ssl_verify_certificate=false;
  ssl_use_session_resumption=true;
  ssl_session=NULL;
  ssl_password=NULL;
  test_port_type=tp_type;
  test_port_name=tp_name;
  ssl_ctx = NULL;
  ssl_current_ssl = NULL;
  SSLv2=true;
  SSLv3=true;
  TLSv1=true;
  TLSv1_1=true;
  TLSv1_2=true;
}

SSL_Socket::~SSL_Socket()
{
  // now SSL context can be removed
  if (ssl_use_ssl && ssl_ctx!=NULL) {
    SSL_CTX_free(ssl_ctx);
  }
  delete [] ssl_key_file;
  delete [] ssl_certificate_file;
  delete [] ssl_trustedCAlist_file;
  delete [] ssl_cipher_list;
  delete [] ssl_password;
}


bool SSL_Socket::parameter_set(const char *parameter_name,
                               const char *parameter_value)
{
  log_debug("entering SSL_Socket::parameter_set(%s, %s)", parameter_name, parameter_value);

  if(strcmp(parameter_name, ssl_use_ssl_name()) == 0) {
    if(strcasecmp(parameter_value, "yes") == 0) ssl_use_ssl = true;
    else if(strcasecmp(parameter_value, "no") == 0) ssl_use_ssl = false;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ssl_use_ssl_name());
  } else if(strcmp(parameter_name, ssl_use_session_resumption_name()) == 0) {
    if(strcasecmp(parameter_value, "yes") == 0) ssl_use_session_resumption = true;
    else if(strcasecmp(parameter_value, "no") == 0) ssl_use_session_resumption = false;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ssl_use_session_resumption_name());
  } else if(strcmp(parameter_name, ssl_private_key_file_name()) == 0) {
    delete [] ssl_key_file;
    ssl_key_file=new char[strlen(parameter_value)+1];
    strcpy(ssl_key_file, parameter_value);
  } else if(strcmp(parameter_name, ssl_trustedCAlist_file_name()) == 0) {
    delete [] ssl_trustedCAlist_file;
    ssl_trustedCAlist_file=new char[strlen(parameter_value)+1];
    strcpy(ssl_trustedCAlist_file, parameter_value);
  } else if(strcmp(parameter_name, ssl_certificate_file_name()) == 0) {
    delete [] ssl_certificate_file;
    ssl_certificate_file=new char[strlen(parameter_value)+1];
    strcpy(ssl_certificate_file, parameter_value);
  } else if(strcmp(parameter_name, ssl_cipher_list_name()) == 0) {
    delete [] ssl_cipher_list;
    ssl_cipher_list=new char[strlen(parameter_value)+1];
    strcpy(ssl_cipher_list, parameter_value);
  } else if(strcmp(parameter_name, ssl_password_name()) == 0) {
    ssl_password=new char[strlen(parameter_value)+1];
    strcpy(ssl_password, parameter_value);
  } else if(strcmp(parameter_name, ssl_verifycertificate_name()) == 0) {
    if(strcasecmp(parameter_value, "yes") == 0) ssl_verify_certificate = true;
    else if(strcasecmp(parameter_value, "no") == 0) ssl_verify_certificate = false;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ssl_verifycertificate_name());
  } else if(strcasecmp(parameter_name, ssl_disable_SSLv2()) == 0) {
    if(strcasecmp(parameter_value, "yes") == 0)  SSLv2= false;
    else if(strcasecmp(parameter_value, "no") == 0) SSLv2 = true;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ssl_disable_SSLv2());
  } else if(strcasecmp(parameter_name, ssl_disable_SSLv3()) == 0) {
    if(strcasecmp(parameter_value, "yes") == 0) SSLv2 = false;
    else if(strcasecmp(parameter_value, "no") == 0) SSLv2 = true;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ssl_disable_SSLv3());
  } else if(strcasecmp(parameter_name, ssl_disable_TLSv1()) == 0) {
    if(strcasecmp(parameter_value, "yes") == 0)  TLSv1= false;
    else if(strcasecmp(parameter_value, "no") == 0) TLSv1 = true;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ssl_disable_TLSv1());
  } else if(strcasecmp(parameter_name, ssl_disable_TLSv1_1()) == 0) {
    if(strcasecmp(parameter_value, "yes") == 0) TLSv1_1 = false;
    else if(strcasecmp(parameter_value, "no") == 0) TLSv1_1 = true;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ssl_disable_TLSv1_1());
  } else if(strcasecmp(parameter_name, ssl_disable_TLSv1_2()) == 0) {
    if(strcasecmp(parameter_value, "yes") == 0) TLSv1_2 = false;
    else if(strcasecmp(parameter_value, "no") == 0) TLSv1_2 = true;
    else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ssl_disable_TLSv1_2());
  } else {
    log_debug("leaving SSL_Socket::parameter_set(%s, %s)", parameter_name, parameter_value);
    return Abstract_Socket::parameter_set(parameter_name, parameter_value);
  }
  log_debug("leaving SSL_Socket::parameter_set(%s, %s)", parameter_name, parameter_value);
  return true;
}


bool SSL_Socket::add_user_data(int client_id) {

  log_debug("entering SSL_Socket::add_user_data()");
  if (!ssl_use_ssl) {
    log_debug("leaving SSL_Socket::add_user_data()");
    return Abstract_Socket::add_user_data(client_id);
  }

  ssl_init_SSL();

  log_debug("Create a new SSL object");
  if (ssl_ctx==NULL)
    log_error("No SSL CTX found, SSL not initialized");
  ssl_current_ssl=SSL_new(ssl_ctx);

  if (ssl_current_ssl==NULL)
    log_error("Creation of SSL object failed");
#ifdef SSL_OP_NO_SSLv2
  if(!SSLv2){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_SSLv2);
  }
#endif
#ifdef SSL_OP_NO_SSLv3
  if(!SSLv3){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_SSLv3);
  }
#endif
#ifdef SSL_OP_NO_TLSv1
  if(!TLSv1){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_TLSv1);
  }
#endif
#ifdef SSL_OP_NO_TLSv1_1
  if(!TLSv1_1){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_TLSv1_1);
  }
#endif
#ifdef SSL_OP_NO_TLSv1_2
  if(!TLSv1_2){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_TLSv1_2);
  }
#endif
    
  set_user_data(client_id, ssl_current_ssl);
  log_debug("New client added with key '%d'", client_id);
  log_debug("Binding SSL to the socket");
  if (SSL_set_fd(ssl_current_ssl, client_id)!=1)
    log_error("Binding of SSL object to socket failed");

  // Conext change for SSL objects may come here in the
  // future.

  if (Abstract_Socket::get_server_mode()) {
    log_debug("Accept SSL connection request");
    if (ssl_current_client!=NULL) log_warning("Warning: race condition while setting current client object pointer");
    ssl_current_client=(SSL_Socket *)this;
    if (ssl_getresult(SSL_accept(ssl_current_ssl))!=SSL_ERROR_NONE) {
      log_warning("Connection from client %d is refused", client_id);
      ssl_current_client=NULL;
      log_debug("leaving SSL_Socket::add_user_data()");
      return false;
    }
    ssl_current_client=NULL;

  } else {
    if (ssl_use_session_resumption && ssl_session!=NULL) {
      log_debug("Try to use ssl_session resumption");
      if (ssl_getresult(SSL_set_session(ssl_current_ssl, ssl_session))!=SSL_ERROR_NONE)
        log_error("SSL error occured");
    }

    log_debug("Connect to server");
    if (ssl_current_client!=NULL) log_warning("Warning: race condition while setting current client object pointer");
    ssl_current_client=(SSL_Socket *)this;
	//

	while(true)
	{
		int res = ssl_getresult(SSL_connect(ssl_current_ssl));
		switch (res) {
		case SSL_ERROR_NONE: break;
		case SSL_ERROR_WANT_WRITE:
			ssl_current_client = NULL;
			TTCN_Snapshot::block_for_sending(client_id);
			continue;
		case SSL_ERROR_WANT_READ:
			for(;;) {
			  pollfd pollClientFd = { client_id, POLLIN, 0 };
			  int nEvents = poll(&pollClientFd, 1, 0);
			  if (nEvents == 1 && (pollClientFd.revents & (POLLIN | POLLHUP)) != 0)
			  	break;
			  if(nEvents < 0 && errno != EINTR)
			    log_error("System call poll() failed on file descriptor %d", client_id);
			}
			continue;
		default:
	    	log_warning("Connection to server is refused");
	    	ssl_current_client=NULL;
		    log_debug("leaving SSL_Socket::add_user_data()");
		    return false;
	    }
		break;
	} //while


    ssl_current_client=NULL;
    if (ssl_use_session_resumption) {
      log_debug("Connected, get new ssl_session");
      ssl_session=SSL_get1_session(ssl_current_ssl);
      if (ssl_session==NULL)
        log_warning("Server did not send a session ID");
    }
  }

  if (ssl_use_session_resumption) {
    if (SSL_session_reused(ssl_current_ssl)) log_debug("Session was reused");
    else log_debug("Session was not reused");
  }

  if (!ssl_verify_certificates()) { // remove client
    log_warning("Verification failed");
    log_debug("leaving SSL_Socket::add_user_data()");
    return false;

  }
  log_debug("leaving SSL_Socket::add_user_data()");
  return true;
}


bool SSL_Socket::remove_user_data(int client_id) {

  log_debug("entering SSL_Socket::remove_user_data()");
  if (!ssl_use_ssl) {
    log_debug("leaving SSL_Socket::remove_user_data()");
    return Abstract_Socket::remove_user_data(client_id);
  }
  ssl_current_ssl = (SSL*)get_user_data(client_id);
  if (ssl_current_ssl!=NULL) {
	SSL_shutdown(ssl_current_ssl);
    SSL_free(ssl_current_ssl);
  } else
    log_warning("SSL object not found for client %d", client_id);
  log_debug("leaving SSL_Socket::remove_user_data()");
  return true;
}



bool SSL_Socket::user_all_mandatory_configparameters_present() {
  if (!ssl_use_ssl) { return true; }
  if (Abstract_Socket::get_server_mode()) {
    if (ssl_certificate_file==NULL)
      log_error("%s is not defined in the configuration file", ssl_certificate_file_name());
    if (ssl_trustedCAlist_file==NULL)
      log_error("%s is not defined in the configuration file", ssl_trustedCAlist_file_name());
    if (ssl_key_file==NULL)
      log_error("%s is not defined in the configuration file", ssl_private_key_file_name());
  } else {
    if (ssl_verify_certificate && ssl_trustedCAlist_file==NULL)
      log_error("%s is not defined in the configuration file altough %s=yes", ssl_trustedCAlist_file_name(), ssl_verifycertificate_name());
  }
  return true;
}



//STATE_WAIT_FOR_RECEIVE_CALLBACK: if the SSL_read operation would
//                                  block because the socket is not ready for writing,
//                                  I set the socket state to this state and add the file
//                                  descriptor to the Event_Handler. The Event_Handler will
//                                  wake up and call the receive_message_on_fd operation
//                                  if the socket is ready to write.
//If the SSL_read operation would block because the socket is not ready for
//reading, I do nothing
int SSL_Socket::receive_message_on_fd(int client_id)
{
  log_debug("entering SSL_Socket::receive_message_on_fd()");
  if (!ssl_use_ssl) {
    log_debug("leaving SSL_Socket::receive_message_on_fd()");
    return Abstract_Socket::receive_message_on_fd(client_id);
  }

  if (ssl_current_client!=NULL) log_warning("Warning: race condition while setting current client object pointer");
  ssl_current_client=(SSL_Socket *)this;

  as_client_struct* peer = get_peer(client_id); // check if client exists
  if (peer->reading_state == STATE_WAIT_FOR_RECEIVE_CALLBACK){
    Remove_Fd_Write_Handler(client_id);
    log_debug("SSL_Socket::receive_message_on_fd: setting socket state to STATE_NORMAL");
    peer->reading_state = STATE_NORMAL;
  }
  TTCN_Buffer* recv_tb = get_buffer(client_id);
  ssl_current_ssl=(SSL*)get_user_data(client_id);
  int messageLength=0;
  size_t end_len=AS_SSL_CHUNCK_SIZE;
  unsigned char *end_ptr;
  while (messageLength<=0) {
    log_debug("  one read cycle started");
    recv_tb->get_end(end_ptr, end_len);
    messageLength = SSL_read(ssl_current_ssl, end_ptr, end_len);
    if (messageLength <= 0) {
      int res=ssl_getresult(messageLength);
      switch (res) {
      case SSL_ERROR_ZERO_RETURN:
        log_debug("SSL_Socket::receive_message_on_fd: SSL connection was interrupted by the other side");
        SSL_set_quiet_shutdown(ssl_current_ssl, 1);
        log_debug("SSL_ERROR_ZERO_RETURN is received, setting SSL SHUTDOWN mode to QUIET");
        ssl_current_client=NULL;
        log_debug("leaving SSL_Socket::receive_message_on_fd() with SSL_ERROR_ZERO_RETURN");
        return 0;
      case SSL_ERROR_WANT_WRITE://writing would block
        if (get_use_non_blocking_socket()){
            Add_Fd_Write_Handler(client_id);
            log_debug("SSL_Socket::receive_message_on_fd: setting socket state to STATE_WAIT_FOR_RECEIVE_CALLBACK");
            peer->reading_state = STATE_WAIT_FOR_RECEIVE_CALLBACK;
            ssl_current_client=NULL;
            log_debug("leaving SSL_Socket::receive_message_on_fd()");
            return -2;
        }
      case SSL_ERROR_WANT_READ: //reading would block, continue processing data
        if (get_use_non_blocking_socket()){
            log_debug("SSL_Socket::receive_message_on_fd: reading would block, leaving SSL_Socket::receive_message_on_fd()");
            ssl_current_client = NULL;
            log_debug("leaving SSL_Socket::receive_message_on_fd()");
            return -2;
        }
        log_debug("repeat the read operation to finish the pending SSL handshake");
        break;
      default:
        log_error("SSL error occured");
      }
    } else {
      recv_tb->increase_length(messageLength);
    }
  }
  ssl_current_client=NULL;
  log_debug("leaving SSL_Socket::receive_message_on_fd() with number of bytes read: %d", messageLength);
  return messageLength;
}


int SSL_Socket::send_message_on_fd(int client_id, const unsigned char* send_par, int message_length)
{
  log_debug("entering SSL_Socket::send_message_on_fd()");

  if (!ssl_use_ssl) {
    log_debug("leaving SSL_Socket::send_message_on_fd()");
    return Abstract_Socket::send_message_on_fd(client_id, send_par, message_length);
  }

  if (ssl_current_client!=NULL) log_warning("Warning: race condition while setting current client object pointer");
  ssl_current_client=(SSL_Socket *)this;

  get_peer(client_id); // check if client exists
  ssl_current_ssl=(SSL*)get_user_data(client_id);
  if (ssl_current_ssl==NULL) { log_error("No SSL data available for client %d", client_id); }
  log_debug("Client ID = %d", client_id);
  while (true) {
    log_debug("  one write cycle started");

    int res = ssl_getresult(SSL_write(ssl_current_ssl, send_par, message_length));
    switch (res) {
    case SSL_ERROR_NONE:
      ssl_current_client=NULL;
      log_debug("leaving SSL_Socket::send_message_on_fd()");
      return message_length;
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_READ:
      log_debug("repeat the write operation to finish the pending SSL handshake");
      break;
    case SSL_ERROR_ZERO_RETURN:
      log_warning("SSL_Socket::send_message_on_fd: SSL connection was interrupted by the other side");
      SSL_set_quiet_shutdown(ssl_current_ssl, 1);
      log_debug("SSL_ERROR_ZERO_RETURN is received, setting SSL SHUTDOWN mode to QUIET");
      ssl_current_client=NULL;
      log_debug("leaving SSL_Socket::send_message_on_fd()");
      return 0;
    default:
      log_error("SSL error occured");
    }
  }
  // avoid compiler warnings
  return 0;
}

//If the socket is not ready for writing, the same mechanism is used
//as described at the Abstract_Socket class
//If the socket is not ready for reading, I block the execution using
//the take_new operation while the socket is not ready for reading.
//While this operation will call the Event_Handler,
//I indicate with the STATE_DONT_RECEIVE state that from the Event_Handler the receive_message_on_fd
//operation must not be called for this socket.
int SSL_Socket::send_message_on_nonblocking_fd(int client_id, const unsigned char* send_par, int message_length){
  log_debug("entering SSL_Socket::send_message_on_nonblocking_fd()");

  if (!ssl_use_ssl) {
    log_debug("leaving SSL_Socket::send_message_on_nonblocking_fd()");
    return Abstract_Socket::send_message_on_nonblocking_fd(client_id, send_par, message_length);
  }

  as_client_struct* peer;
  if (ssl_current_client!=NULL) log_warning("Warning: race condition while setting current client object pointer");
  ssl_current_client=(SSL_Socket *)this;

  get_peer(client_id);
  ssl_current_ssl=(SSL*)get_user_data(client_id);
  if (ssl_current_ssl==NULL) { log_error("No SSL data available for client %d", client_id); }
  log_debug("Client ID = %d", client_id);
  while (true) {
    int res;
    peer = get_peer(client_id); // check if client exists
    log_debug("  one write cycle started");
    ssl_current_ssl = (SSL*)get_user_data(client_id);
    if (peer -> reading_state == STATE_DONT_CLOSE){
        goto client_closed_connection;
    }else res = ssl_getresult(SSL_write(ssl_current_ssl, send_par, message_length));

    switch (res) {
    case SSL_ERROR_NONE:
      ssl_current_client=NULL;
      log_debug("leaving SSL_Socket::send_message_on_nonblocking_fd()");
      log_debug("SSL_Socket::send_message_on_nonblocking_fd: setting socket state to STATE_NORMAL");
      peer -> reading_state = STATE_NORMAL;
      return message_length;
    case SSL_ERROR_WANT_WRITE:
        if (peer == NULL){
            log_error("SSL_Socket::send_message_on_nonblocking_fd, Client ID %d does not exist.", client_id);
        }
        int old_bufsize, new_bufsize;
        if (increase_send_buffer(client_id, old_bufsize, new_bufsize)) {
            log_debug("Sending data on on file descriptor %d",client_id);
	    log_debug("The sending operation would block execution. The "
		      "size of the outgoing buffer was increased from %d to "
                      "%d bytes.",old_bufsize,
                      new_bufsize);
        } else {
            log_warning("Sending data on file descriptor %d", client_id);
            log_warning("The sending operation would block execution and it "
                        "is not possible to further increase the size of the "
			"outgoing buffer. Trying to process incoming data to "
			"avoid deadlock.");
            ssl_current_client=NULL;
            log_debug("SSL_Socket::send_message_on_nonblocking_fd: setting socket state to STATE_BLOCK_FOR_SENDING");
            peer->reading_state = STATE_BLOCK_FOR_SENDING;
            TTCN_Snapshot::block_for_sending(client_id);
        }
        peer = get_peer(client_id); // check if client exists
        if (peer == NULL){
            log_error("SSL_Socket::send_message_on_nonblocking_fd, Client ID %d does not exist.", client_id);
        }
        break;
    case SSL_ERROR_WANT_READ:
      //receiving buffer is probably empty thus reading would block execution
      log_debug("SSL_write cannot read data from socket %d. Trying to process data to avoid deadlock.", client_id);
      log_debug("SSL_Socket::send_message_on_nonblocking_fd: setting socket state to STATE_DONT_RECEIVE");
      peer -> reading_state = STATE_DONT_RECEIVE; //don't call receive_message_on_fd() to this socket
      for (;;) {
        TTCN_Snapshot::take_new(TRUE);
        pollfd pollClientFd = { client_id, POLLIN, 0 };
        int nEvents = poll(&pollClientFd, 1, 0);
        if (nEvents == 1 && (pollClientFd.revents & (POLLIN | POLLHUP)) != 0)
          break;
        if (nEvents < 0 && errno != EINTR)
          log_error("System call poll() failed on file descriptor %d", client_id);
      } 
      log_debug("Deadlock resolved");
      break;
    case SSL_ERROR_ZERO_RETURN:
        goto client_closed_connection;
    default:
      log_error("SSL error occured");
    }
  }

client_closed_connection:
    log_warning("SSL_Socket::send_message_on_nonblocking_fd: SSL connection was interrupted by the other side");
    SSL_set_quiet_shutdown(ssl_current_ssl, 1);
    log_debug("Setting SSL SHUTDOWN mode to QUIET");
    ssl_current_client=NULL;
    log_debug("leaving SSL_Socket::send_message_on_nonblocking_fd()");
    log_debug("SSL_Socket::send_message_on_nonblocking_fd: setting socket state to STATE_NORMAL");
    peer -> reading_state = STATE_NORMAL;
    errno = EPIPE;
    return -1;

}

bool SSL_Socket::ssl_verify_certificates()
{
  char str[SSL_CHARBUF_LENGTH];

  log_debug("entering SSL_Socket::ssl_verify_certificates()");

  ssl_log_SSL_info();

  // Get the other side's certificate
  log_debug("Check certificate of the other party");
  X509 *cert = SSL_get_peer_certificate (ssl_current_ssl);
  if (cert != NULL) {

    {
      log_debug("Certificate information:");
      X509_NAME_oneline (X509_get_subject_name (cert), str, SSL_CHARBUF_LENGTH);
      log_debug("  subject: %s", str);
    }

    // We could do all sorts of certificate verification stuff here before
    // deallocating the certificate.

    // Just a basic check that the certificate is valid
    // Other checks (e.g. Name in certificate vs. hostname) shall be
    // done on application level
    if (ssl_verify_certificate)
      log_debug("Verification state is: %s", X509_verify_cert_error_string(SSL_get_verify_result(ssl_current_ssl)));
    X509_free (cert);

  } else
    log_warning("Other side does not have certificate.");

  log_debug("leaving SSL_Socket::ssl_verify_certificates()");
  return true;
}



// Data set/get functions
char       * SSL_Socket::get_ssl_password() const {return ssl_password;}
void         SSL_Socket::set_ssl_use_ssl(bool par) {ssl_use_ssl=par;}
void         SSL_Socket::set_ssl_verifycertificate(bool par) {ssl_verify_certificate=par;}
void         SSL_Socket::set_ssl_use_session_resumption(bool par) {ssl_use_session_resumption=par;}
void         SSL_Socket::set_ssl_key_file(char * par) {
  delete [] ssl_key_file;
  ssl_key_file=par;
}
void         SSL_Socket::set_ssl_certificate_file(char * par) {
  delete [] ssl_certificate_file;
  ssl_certificate_file=par;
}
void         SSL_Socket::set_ssl_trustedCAlist_file(char * par) {
  delete [] ssl_trustedCAlist_file;
  ssl_trustedCAlist_file=par;
}
void         SSL_Socket::set_ssl_cipher_list(char * par) {
  delete [] ssl_cipher_list;
  ssl_cipher_list=par;
}
void         SSL_Socket::set_ssl_server_auth_session_id_context(const unsigned char * par) {
  ssl_server_auth_session_id_context=par;
}

// Default parameter names
const char* SSL_Socket::ssl_use_ssl_name()                { return "ssl_use_ssl";}
const char* SSL_Socket::ssl_use_session_resumption_name() { return "ssl_use_session_resumption";}
const char* SSL_Socket::ssl_private_key_file_name()       { return "ssl_private_key_file";}
const char* SSL_Socket::ssl_trustedCAlist_file_name()     { return "ssl_trustedCAlist_file";}
const char* SSL_Socket::ssl_certificate_file_name()       { return "ssl_certificate_chain_file";}
const char* SSL_Socket::ssl_password_name()               { return "ssl_private_key_password";}
const char* SSL_Socket::ssl_cipher_list_name()            { return "ssl_allowed_ciphers_list";}
const char* SSL_Socket::ssl_verifycertificate_name()      { return "ssl_verify_certificate";}
const char* SSL_Socket::ssl_disable_SSLv2()      { return "ssl_disable_SSLv2";}
const char* SSL_Socket::ssl_disable_SSLv3()      { return "ssl_disable_SSLv3";}
const char* SSL_Socket::ssl_disable_TLSv1()      { return "ssl_disable_TLSv1";}
const char* SSL_Socket::ssl_disable_TLSv1_1()      { return "ssl_disable_TLSv1_1";}
const char* SSL_Socket::ssl_disable_TLSv1_2()      { return "ssl_disable_TLSv1_2";}


void SSL_Socket::ssl_actions_to_seed_PRNG() {
  struct stat randstat;

  if(RAND_status()) {
    log_debug("PRNG already initialized, no action needed");
    return;
  }
  log_debug("Seeding PRND");
  // OpenSSL tries to use random devives automatically
  // these would not be necessary
  if (!stat("/dev/urandom", &randstat)) {
    log_debug("Using installed random device /dev/urandom for seeding the PRNG with %d bytes.", SSL_PRNG_LENGTH);
    if (RAND_load_file("/dev/urandom", SSL_PRNG_LENGTH)!=SSL_PRNG_LENGTH)
      log_error("Could not read from /dev/urandom");
  } else if (!stat("/dev/random", &randstat)) {
    log_debug("Using installed random device /dev/random for seeding the PRNG with %d bytes.", SSL_PRNG_LENGTH);
    if (RAND_load_file("/dev/random", SSL_PRNG_LENGTH)!=SSL_PRNG_LENGTH)
      log_error("Could not read from /dev/random");
  } else {
    /* Neither /dev/random nor /dev/urandom are present, so add
       entropy to the SSL PRNG a hard way. */
    log_warning("Solaris patches to provide random generation devices are not installed.\nSee http://www.openssl.org/support/faq.html \"Why do I get a \"PRNG not seeded\" error message?\"\nA workaround will be used.");
    for (int i = 0; i < 10000  &&  !RAND_status(); ++i) {
      char buf[4];
      struct timeval tv;
      gettimeofday(&tv, 0);
      buf[0] = tv.tv_usec & 0xF;
      buf[2] = (tv.tv_usec & 0xF0) >> 4;
      buf[3] = (tv.tv_usec & 0xF00) >> 8;
      buf[1] = (tv.tv_usec & 0xF000) >> 12;
      RAND_add(buf, sizeof buf, 0.1);
    }
    return;
  }

  if(!RAND_status()) {
    log_error("Could not seed the Pseudo Random Number Generator with enough data.");
  } else {
    log_debug("PRNG successfully initialized.");
  }
}


void SSL_Socket::ssl_init_SSL()
{
  if (ssl_initialized) {
    log_debug("SSL already initialized, no action needed");
    return;
  }

  {
    log_debug("Init SSL started");
    log_debug("Using %s (%lx)", SSLeay_version(SSLEAY_VERSION), OPENSSL_VERSION_NUMBER);
  }

  SSL_library_init();          // initialize library
  SSL_load_error_strings();    // readable error messages

  // Create SSL method: both server and client understanding SSLv2, SSLv3, TLSv1
//  ssl_method = SSLv23_method();
//  if (ssl_method==NULL)
//    log_error("SSL method creation failed.");
  // Create context
  ssl_ctx = SSL_CTX_new (SSLv23_method());
  if (ssl_ctx==NULL)
    log_error("SSL context creation failed.");

  // valid for all SSL objects created from this context afterwards
  if(ssl_certificate_file!=NULL) {
    log_debug("Loading certificate file");
    if(SSL_CTX_use_certificate_chain_file(ssl_ctx, ssl_certificate_file)!=1)
      log_error("Can't read certificate file ");
  }

  // valid for all SSL objects created from this context afterwards
  if(ssl_key_file!=NULL) {
    log_debug("Loading key file");
    if (ssl_current_client!=NULL) log_warning("Warning: race condition while setting current client object pointer");
    ssl_current_client=(SSL_Socket *)this;
    if(ssl_password!=NULL)
      SSL_CTX_set_default_passwd_cb(ssl_ctx, ssl_password_cb);
    if(SSL_CTX_use_PrivateKey_file(ssl_ctx, ssl_key_file, SSL_FILETYPE_PEM)!=1)
      log_error("Can't read key file ");
    ssl_current_client=NULL;
  }

  if (ssl_trustedCAlist_file!=NULL) {
    log_debug("Loading trusted CA list file");
    if (SSL_CTX_load_verify_locations(ssl_ctx, ssl_trustedCAlist_file, NULL)!=1)
      log_error("Can't read trustedCAlist file ");
  }

  if (ssl_certificate_file!=NULL && ssl_key_file!=NULL) {
    log_debug("Check for consistency between private and public keys");
    if (SSL_CTX_check_private_key(ssl_ctx)!=1)
      log_warning("Private key does not match the certificate public key");
  }

  // check the other side's certificates
  if (ssl_verify_certificate) {
    log_debug("Setting verification behaviour: verification required and do not allow to continue on failure..");
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, ssl_verify_callback);
  } else {
    log_debug("Setting verification behaviour: verification not required and do allow to continue on failure..");
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, ssl_verify_callback);
  }

  if (ssl_cipher_list!=NULL) {
    log_debug("Setting ssl_cipher list restrictions");
    if (SSL_CTX_set_cipher_list(ssl_ctx, ssl_cipher_list)!=1)
      log_error("Cipher list restriction failed for %s", ssl_cipher_list);
  }

  ssl_actions_to_seed_PRNG();

  if (Abstract_Socket::get_server_mode() && ssl_use_session_resumption) {
    log_debug("Prepare server for ssl_session resumption");

    log_debug("Context is: %s; length = %lu", ssl_server_auth_session_id_context, (unsigned long)strlen((const char*)ssl_server_auth_session_id_context));
    if (SSL_CTX_set_session_id_context(ssl_ctx, ssl_server_auth_session_id_context, strlen((const char*)ssl_server_auth_session_id_context))!=1)
      log_error("Activation of SSL ssl_session resumption failed on server");
  }

  ssl_initialized=true;

  log_debug("Init SSL successfully finished");
}


void SSL_Socket::ssl_log_SSL_info()
{
  char str[SSL_CHARBUF_LENGTH];

  log_debug("Check SSL description");
  const SSL_CIPHER  *ssl_cipher=SSL_get_current_cipher(ssl_current_ssl);
  if (ssl_cipher!=NULL) {
    SSL_CIPHER_description(SSL_get_current_cipher(ssl_current_ssl), str, SSL_CHARBUF_LENGTH);
    {
      log_debug("SSL description:");
      log_debug("%s", str);
    }
  }
}



// Log the SSL error and flush the error queue
// Can be used after the followings:
// SSL_connect(), SSL_accept(), SSL_do_handshake(),
// SSL_read(), SSL_peek(), or SSL_write()
int SSL_Socket::ssl_getresult(int res)
{
  int err = SSL_get_error(ssl_current_ssl, res);

  log_debug("SSL operation result:");

  switch(err) {
  case SSL_ERROR_NONE:
    log_debug("SSL_ERROR_NONE");
    break;
  case SSL_ERROR_ZERO_RETURN:
    log_debug("SSL_ERROR_ZERO_RETURN");
    break;
  case SSL_ERROR_WANT_READ:
    log_debug("SSL_ERROR_WANT_READ");
    break;
  case SSL_ERROR_WANT_WRITE:
    log_debug("SSL_ERROR_WANT_WRITE");
    break;
  case SSL_ERROR_WANT_CONNECT:
    log_debug("SSL_ERROR_WANT_CONNECT");
    break;
  case SSL_ERROR_WANT_ACCEPT:
    log_debug("SSL_ERROR_WANT_ACCEPT");
    break;
  case SSL_ERROR_WANT_X509_LOOKUP:
    log_debug("SSL_ERROR_WANT_X509_LOOKUP");
    break;
  case SSL_ERROR_SYSCALL:
    log_debug("SSL_ERROR_SYSCALL");
    log_debug("EOF was observed that violates the protocol, peer disconnected; treated as a normal disconnect");
    return SSL_ERROR_ZERO_RETURN;
    break;
  case SSL_ERROR_SSL:
    log_debug("SSL_ERROR_SSL");
    break;
  default:
    log_error("Unknown SSL error code: %d", err);
  }
  // get the copy of the error string in readable format
  unsigned long e=ERR_get_error();
  while (e) {
    log_debug("SSL error queue content:");
    log_debug("  Library:  %s", ERR_lib_error_string(e));
    log_debug("  Function: %s", ERR_func_error_string(e));
    log_debug("  Reason:   %s", ERR_reason_error_string(e));
    e=ERR_get_error();
  }
  //It does the same but more simple:
  // ERR_print_errors_fp(stderr);
  return err;
}

int   SSL_Socket::ssl_verify_certificates_at_handshake(int /*preverify_ok*/, X509_STORE_CTX */*ssl_ctx*/) {
   // don't care by default
   return -1;
}

// Callback function used by OpenSSL.
// Called when a password is needed to decrypt the private key file.
// NOTE: not thread safe
int SSL_Socket::ssl_password_cb(char *buf, int num, int /*rwflag*/,void */*userdata*/) {

  if (ssl_current_client!=NULL) {
     char *ssl_client_password;
     ssl_client_password=((SSL_Socket *)ssl_current_client)->get_ssl_password();
     if(ssl_client_password==NULL) return 0;
     const char* pass = (const char*) ssl_client_password;
     int pass_len = strlen(pass) + 1;
     if (num < pass_len) return 0;

     strcpy(buf, pass);
     return(strlen(pass));
  } else { // go on with no password set
     fprintf(stderr, "Warning: no current SSL object found but ssl_password_cb is called, programming error\n");
     return 0;
  }
}

// Callback function used by OpenSSL.
// Called during SSL handshake with a pre-verification status.
int SSL_Socket::ssl_verify_callback(int preverify_ok, X509_STORE_CTX *ssl_ctx)
{
  SSL     *ssl_pointer;
  SSL_CTX *ctx_pointer;
  int user_result;

  ssl_pointer = (SSL *)X509_STORE_CTX_get_ex_data(ssl_ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
  ctx_pointer = SSL_get_SSL_CTX(ssl_pointer);

  if (ssl_current_client!=NULL) {
     // if ssl_verifiycertificate == "no", then always accept connections
     if(((SSL_Socket *)ssl_current_client)->ssl_verify_certificate) {
       user_result=((SSL_Socket *)ssl_current_client)->ssl_verify_certificates_at_handshake(preverify_ok, ssl_ctx);
       if (user_result>=0) return user_result;
     } else {
       return 1;
     }
  } else { // go on with default authentication
     fprintf(stderr, "Warning: no current SSL object found but ssl_verify_callback is called, programming error\n");
  }

  // if ssl_verifiycertificate == "no", then always accept connections
  if (SSL_CTX_get_verify_mode(ctx_pointer) == SSL_VERIFY_NONE)
    return 1;
  // if ssl_verifiycertificate == "yes", then accept connections only if the
  // certificate is valid
  else if (SSL_CTX_get_verify_mode(ctx_pointer) & SSL_VERIFY_PEER) {
    return preverify_ok;
  }
  // something went wrong
  else
    return 0;
}

#endif
