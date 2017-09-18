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
//  File:               HTTPmsg_PT.cc
//  Description:        HTTP test port implementation
//  Rev:                R8G
//  Prodnr:             CNL 113 469


#include "HTTPmsg_PT.hh"

#include <ctype.h>
#include <arpa/inet.h>

#ifdef AS_USE_SSL
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#endif

static bool report_lf=true;

namespace HTTPmsg__PortType {

HTTPmsg__PT::HTTPmsg__PT(const char *par_port_name)
	: HTTPmsg__PT_BASE(par_port_name)
{
    parameter_set(use_connection_ASPs_name(), "yes");
    parameter_set(server_backlog_name(), "1024");
    use_notification_ASPs = false;
    set_ttcn_buffer_usercontrol(true);
    set_handle_half_close(true);
    adding_client_connection = false;
    adding_ssl_connection = false;
    server_use_ssl = false;
#ifdef AS_USE_SSL
    
    set_ssl_use_ssl(true);
#endif
}

HTTPmsg__PT::~HTTPmsg__PT()
{

}

void HTTPmsg__PT::set_parameter(const char *parameter_name,
	const char *parameter_value)
{
    log_debug("entering HTTPmsg__PT::set_parameter(%s, %s)", parameter_name, parameter_value);
    if(strcasecmp(parameter_name, use_notification_ASPs_name()) == 0) {
        if (strcasecmp(parameter_value,"yes")==0) use_notification_ASPs = true;
        else if (strcasecmp(parameter_value,"no")==0) use_notification_ASPs = false;
        else log_error("Parameter value '%s' not recognized for parameter '%s'", parameter_value, use_notification_ASPs_name());
    }
    else if((strcasecmp(parameter_name, use_connection_ASPs_name()) == 0) || !parameter_set(parameter_name ,parameter_value)) {
        log_warning("HTTPmsg__PT::set_parameter(): Unsupported Test Port parameter: %s", parameter_name);
    }
    log_debug("leaving HTTPmsg__PT::set_parameter(%s, %s)", parameter_name, parameter_value);
}

void HTTPmsg__PT::Handle_Fd_Event(int fd,
    boolean is_readable, boolean is_writable, boolean is_error)
{
    log_debug("-------------- entering HTTPmsg__PT::Handle_Fd_Event() - event received on a connection");
    Handle_Socket_Event(fd, is_readable, is_writable, is_error);
    log_debug("leaving HTTPmsg__PT::Handle_Fd_Event()");
}

void HTTPmsg__PT::Handle_Timeout(double time_since_last_call)
{
    log_debug("entering HTTPmsg__PT::Handle_Timeout()");
    Handle_Timeout_Event(time_since_last_call);
    log_debug("leaving HTTPmsg__PT::Handle_Timeout()");
}

void HTTPmsg__PT::user_map(const char *system_port)
{
    log_debug("entering HTTPmsg__PT::user_map(%s)",system_port);
    if(TTCN_Logger::log_this_event(TTCN_DEBUG)) {
        if(!get_socket_debugging())
            log_warning("%s: to switch on HTTP test port debugging, set the '*.%s.http_debugging := \"yes\" in the port's parameters.", get_name(), get_name());
    }
    map_user();
    log_debug("leaving HTTPmsg__PT::user_map()");
}

void HTTPmsg__PT::user_unmap(const char *system_port)
{
  log_debug("entering HTTPmsg__PT::user_unmap(%s)",system_port);

  unmap_user();

  log_debug("leaving HTTPmsg__PT::user_unmap()");
}

void HTTPmsg__PT::user_start()
{
}

void HTTPmsg__PT::user_stop()
{
}

void HTTPmsg__PT::outgoing_send(const HTTPmsg__Types::Close& send_par)
{
    log_debug("entering HTTPmsg__PT::outgoing_send(Close)");

    if(send_par.client__id().ispresent())
        remove_client((int)send_par.client__id()());
    else
        remove_all_clients();

    log_debug("leaving HTTPmsg__PT::outgoing_send(Close)");
}

void HTTPmsg__PT::outgoing_send(const HTTPmsg__Types::Connect& send_par)
{
    log_debug("entering HTTPmsg__PT::outgoing_send(Connect)");

    adding_ssl_connection = send_par.use__ssl();
    adding_client_connection = true;

#ifndef AS_USE_SSL
    if(adding_ssl_connection)
    {
        log_error("%s: HTTP test port is not compiled to support SSL connections. Please check the User's Guide for instructions on compiling the HTTP test port with SSL support.", get_name());
    }
#endif

   int client_id = open_client_connection(send_par.hostname(),int2str((INTEGER)send_par.portnumber()),NULL,NULL);

    adding_ssl_connection = false;
    adding_client_connection = false;

    log_debug("leaving HTTPmsg__PT::outgoing_send(Connect),client_id: %d", client_id);
}

void HTTPmsg__PT::outgoing_send(const HTTPmsg__Types::Listen& send_par)
{
    log_debug("entering HTTPmsg__PT::outgoing_send(Listen)");

    server_use_ssl = send_par.use__ssl();

    if(server_use_ssl)
        {
    #ifndef AS_USE_SSL
            log_error("%s: HTTP test port is not compiled to support SSL connections. Please check the User's Guide for instructions on compiling the HTTP test port with SSL support.", get_name());
    #endif
        }

    if(send_par.local__hostname().ispresent())
		{

       	open_listen_port(send_par.local__hostname()(),int2str((INTEGER)send_par.portnumber()));

		}
    else
    {
        log_debug("using IN_ADDR_ANY as local host name");
        open_listen_port(NULL,int2str((INTEGER)send_par.portnumber()));

    }

    log_debug("leaving HTTPmsg__PT::outgoing_send(Listen)");

}

void HTTPmsg__PT::outgoing_send(const HTTPmsg__Types::Half__close& send_par)
{
    log_debug("entering HTTPmsg__PT::outgoing_send(Half_close)");

    if(send_par.client__id().ispresent())
        send_shutdown((int)send_par.client__id()());
    else
        send_shutdown();

    log_debug("leaving HTTPmsg__PT::outgoing_send(Half_close)");

}

void HTTPmsg__PT::outgoing_send(const HTTPmsg__Types::Shutdown& /*send_par*/)
{
    log_debug("entering HTTPmsg__PT::outgoing_send(Shutdown)");

    close_listen_port();

    log_debug("leaving HTTPmsg__PT::outgoing_send(Shutdown)");
}

void HTTPmsg__PT::outgoing_send(const HTTPmsg__Types::HTTPMessage& send_par)
{
    log_debug("entering HTTPmsg__PT::outgoing_send(HTTPMessage)");

    TTCN_Buffer snd_buf;
    int client_id = -1;

    switch(send_par.get_selection())
    {
        case HTTPmsg__Types::HTTPMessage::ALT_request:
        {
            if(send_par.request().client__id().ispresent())
                client_id = send_par.request().client__id()();
            break;
        }
        case HTTPmsg__Types::HTTPMessage::ALT_request__binary:
        {
            if(send_par.request__binary().client__id().ispresent())
                client_id = send_par.request__binary().client__id()();
            break;
        }
        case HTTPmsg__Types::HTTPMessage::ALT_response:
        {
            if(send_par.response().client__id().ispresent())
                client_id = send_par.response().client__id()();
            break;
        }
        case HTTPmsg__Types::HTTPMessage::ALT_response__binary:
        {
            if(send_par.response__binary().client__id().ispresent())
                client_id = send_par.response__binary().client__id()();
            break;
        }
        case HTTPmsg__Types::HTTPMessage::ALT_erronous__msg:
        {
            if(send_par.erronous__msg().client__id().ispresent())
                client_id = send_par.erronous__msg().client__id()();
            break;
        }
        default:
            TTCN_error("Unknown HTTP_Message type to encode and send!");
    }

    f_HTTP_encodeCommon(send_par, snd_buf);

    if(client_id >= 0)
        send_outgoing(snd_buf.get_data(), snd_buf.get_len(), client_id);
    else
        send_outgoing(snd_buf.get_data(), snd_buf.get_len());

    log_debug("leaving HTTPmsg__PT::outgoing_send(HTTPMessage)");
}

void HTTPmsg__PT::client_connection_opened(int client_id)
{
    log_debug("entering HTTPmsg__PT::client_connection_opened(%d)", client_id);

    if(use_notification_ASPs)
    {
        HTTPmsg__Types::Connect__result asp;
        asp.client__id() = client_id;
        incoming_message(asp);
    }
    else if(client_id < 0)
        log_error("Cannot connect to server");

    log_debug("leaving HTTPmsg__PT::client_connection_opened()");
}

void HTTPmsg__PT::listen_port_opened(int port_number)
{
  log_debug("entering HTTPmsg__PT::listen_port_opened(%d)", port_number);

  if(use_notification_ASPs)
  {
    HTTPmsg__Types::Listen__result asp;
    asp.portnumber() = port_number;
    incoming_message(asp);
  }
  else if(port_number < 0)
    log_error("Cannot listen at port");

  log_debug("leaving HTTPmsg__PT::listen_port_opened()");
}

void HTTPmsg__PT::message_incoming(const unsigned char* /*msg*/, int /*messageLength*/, int client_id)
{
    log_debug("entering HTTPmsg__PT::message_incoming()");

    TTCN_Buffer* buf_p = get_buffer(client_id);

    while(buf_p->get_read_len() > 0)
    {
        log_debug("HTTPmsg__PT::message_incoming(): decoding next message, len: %d", (int)buf_p->get_read_len());
        if(!HTTP_decode(buf_p, client_id))
            break;
    }

    log_debug("leaving HTTPmsg__PT::message_incoming()");
}

void HTTPmsg__PT::peer_half_closed(int client_id)
{
    log_debug("entering HTTPmsg__PT::peer_half_closed(client_id: %d)", client_id);

    TTCN_Buffer* buf_p = get_buffer(client_id);
    buf_p->rewind();
    while(buf_p->get_read_len() > 0)
    {
        log_debug("HTTPmsg__PT::remove_client(): decoding next message, len: %d", (int)buf_p->get_read_len());
        if(!HTTP_decode(buf_p, client_id,true))
            break;
    }

    HTTPmsg__Types::Half__close asp;
    asp.client__id() = client_id;
    incoming_message(asp);

    log_debug("leaving HTTPmsg__PT::peer_half_closed(client_id: %d)", client_id);
}

void HTTPmsg__PT::peer_disconnected(int client_id)
{
    log_debug("entering HTTPmsg__PT::peer_disconnected(client_id: %d)", client_id);

    if(use_notification_ASPs)
    {
        HTTPmsg__Types::Close asp;
        asp.client__id() = client_id;
        incoming_message(asp);
    }
    else Abstract_Socket::peer_disconnected(client_id);

    log_debug("leaving HTTPmsg__PT::peer_disconnected(client_id: %d)", client_id);
}

//void HTTPmsg__PT::peer_connected(int client_id, sockaddr_in& addr)

void HTTPmsg__PT::peer_connected(int client_id, const char * host, const int port)
{
    log_debug("entering HTTPmsg__PT::peer_connected(%d)", client_id);

    if(use_notification_ASPs)
    {
        HTTPmsg__Types::Client__connected asp;
        asp.hostname() = host;
        asp.portnumber() = port;
        asp.client__id() = client_id;

        incoming_message(asp);
    }
    else Abstract_Socket::peer_connected(client_id, host, port);

    log_debug("leaving HTTPmsg__PT::peer_connected()");
}

bool HTTPmsg__PT::add_user_data(int client_id)
{
    log_debug("entering HTTPmsg__PT::add_user_data(client_id: %d, use_ssl: %s)",
        client_id, (adding_client_connection && adding_ssl_connection) || (server_use_ssl && !adding_ssl_connection) ? "yes" : "no");

    set_server_mode(!adding_client_connection);

    if((adding_client_connection && !adding_ssl_connection) || (!adding_client_connection && !server_use_ssl))
    {
        log_debug("leaving HTTPmsg__PT::add_user_data() with returning Abstract_Socket::add_user_data()");
        return Abstract_Socket::add_user_data(client_id);
    }
    else
    {
#ifdef AS_USE_SSL
        log_debug("leaving HTTPmsg__PT::add_user_data() with returning SSL_Socket::add_user_data()");
        return SSL_Socket::add_user_data(client_id);
#else
        log_error("%s: HTTP test port is not compiled to support SSL connections. Please check the User's Guide for instructions on compiling the HTTP test port with SSL support.", get_name());
#endif
    }

    // Programming error in HTTPmsg__PT::add_user_data()
    return false;
}

bool HTTPmsg__PT::remove_user_data(int client_id)
{
    log_debug("entering HTTPmsg__PT::remove_user_data(client_id: %d", client_id);

#ifdef AS_USE_SSL
    if(get_user_data(client_id))
    {
        // INFO: it is assumed that only SSL_Socket assigns user data to each peer
        log_debug("leaving HTTPmsg__PT::remove_user_data() with returning SSL_Socket::remove_user_data()");
        return SSL_Socket::remove_user_data(client_id);
    }
#endif

    log_debug("leaving HTTPmsg__PT::remove_user_data() with returning Abstract_Socket::remove_user_data()");
    return Abstract_Socket::remove_user_data(client_id);
}

int HTTPmsg__PT::receive_message_on_fd(int client_id)
{
    log_debug("entering HTTPmsg__PT::receive_message_on_fd(client_id: %d)", client_id);

#ifdef AS_USE_SSL
    if(get_user_data(client_id))
    {
        // INFO: it is assumed that only SSL_Socket assigns user data to each peer
        log_debug("leaving HTTPmsg__PT::receive_message_on_fd() with returning SSL_Socket::receive_message_on_fd()");
        return SSL_Socket::receive_message_on_fd(client_id);
    }
#endif

    log_debug("leaving HTTPmsg__PT::receive_message_on_fd() with returning Abstract_Socket::receive_message_on_fd()");
    return Abstract_Socket::receive_message_on_fd(client_id);
}

void HTTPmsg__PT::remove_client(int client_id)
{
    log_debug("entering HTTPmsg__PT::remove_client(client_id: %d)", client_id);

    TTCN_Buffer* buf_p = get_buffer(client_id);

    while(buf_p->get_read_len() > 0)
    {
        log_debug("HTTPmsg__PT::remove_client(): decoding next message, len: %d", (int)buf_p->get_read_len());
        if(!HTTP_decode(buf_p, client_id,true))
            break;
    }

#ifdef AS_USE_SSL
    if(get_user_data(client_id))
    {
        // INFO: it is assumed that only SSL_Socket assigns user data to each peer
        log_debug("leaving HTTPmsg__PT::remove_client() with returning SSL_Socket::remove_client()");
        return SSL_Socket::remove_client(client_id);
    }
#endif

    log_debug("leaving HTTPmsg__PT::remove_client() with returning Abstract_Socket::remove_client()");
    return Abstract_Socket::remove_client(client_id);
}

int HTTPmsg__PT::send_message_on_fd(int client_id, const unsigned char * message_buffer, int length_of_message)
{
    log_debug("entering HTTPmsg__PT::send_message_on_fd(client_id: %d)", client_id);

#ifdef AS_USE_SSL
    if(get_user_data(client_id))
    {
        // INFO: it is assumed that only SSL_Socket assigns user data to each peer
        log_debug("leaving HTTPmsg__PT::send_message_on_fd() with returning SSL_Socket::send_message_on_fd()");
        return SSL_Socket::send_message_on_fd(client_id, message_buffer, length_of_message);
    }
#endif

    log_debug("leaving HTTPmsg__PT::send_message_on_fd() with returning Abstract_Socket::send_message_on_fd()");
    return Abstract_Socket::send_message_on_fd(client_id, message_buffer, length_of_message);
}

int HTTPmsg__PT::send_message_on_nonblocking_fd(int client_id, const unsigned char * message_buffer, int length_of_message)
{
    log_debug("entering HTTPmsg__PT::(client_id: %d)", client_id);

#ifdef AS_USE_SSL
    if(get_user_data(client_id))
    {
        // INFO: it is assumed that only SSL_Socket assigns user data to each peer
        log_debug("leaving HTTPmsg__PT::send_message_on_nonblocking_fd() with returning SSL_Socket::send_message_on_nonblocking_fd()");
        return SSL_Socket::send_message_on_nonblocking_fd(client_id, message_buffer, length_of_message);
    }
#endif

    log_debug("leaving HTTPmsg__PT::send_message_on_nonblocking_fd() with returning Abstract_Socket::send_message_on_nonblocking_fd()");
    return Abstract_Socket::send_message_on_nonblocking_fd(client_id, message_buffer, length_of_message);
}

// HTTP specific functions

// replaced by f_HTTP_encodeCommon:
// void HTTPmsg__PT::HTTP_encode(const HTTPmsg__Types::HTTPMessage& msg, TTCN_Buffer& buf)
// {
//   f_HTTP_encodeCommon( msg, buf);
// }

//Encodes msg type of "HTTPMessage" into buffer
void f_HTTP_encodeCommon(const HTTPmsg__Types::HTTPMessage& msg, TTCN_Buffer& buf)
{
    buf.clear();
    if( msg.get_selection() == HTTPmsg__Types::HTTPMessage::ALT_erronous__msg )
        buf.put_cs(msg.erronous__msg().msg());
    else
    {
        const HTTPmsg__Types::HeaderLines* header = NULL;
        const HTTPmsg__Types::HTTPRequest* request = NULL;
        const HTTPmsg__Types::HTTPResponse* response = NULL;
        const HTTPmsg__Types::HTTPRequest__binary__body* request_binary = NULL;
        const HTTPmsg__Types::HTTPResponse__binary__body* response_binary = NULL;
        const CHARSTRING* body = NULL;
        const OCTETSTRING* body_binary = NULL;

        if(msg.get_selection() == HTTPmsg__Types::HTTPMessage::ALT_request)
        {
            request = &msg.request();
            header = &request->header();
            body = &request->body();
            buf.put_cs(request->method());
            buf.put_c(' ');
            buf.put_cs(request->uri());
            buf.put_cs(" HTTP/");
            buf.put_cs(int2str(request->version__major()));
            buf.put_c('.');
            buf.put_cs(int2str(request->version__minor()));
            buf.put_cs("\r\n");
        }
        else if(msg.get_selection() == HTTPmsg__Types::HTTPMessage::ALT_response)
        {
            response = &msg.response();
            header = &response->header();
            body = &response->body();
            buf.put_cs("HTTP/");
            buf.put_cs(int2str(response->version__major()));
            buf.put_c('.');
            buf.put_cs(int2str(response->version__minor()));
            buf.put_c(' ');
            buf.put_cs(int2str(response->statuscode()));
            buf.put_c(' ');
            buf.put_cs(response->statustext());
            buf.put_cs("\r\n");
        }
        else if(msg.get_selection() == HTTPmsg__Types::HTTPMessage::ALT_request__binary)
        {
            request_binary = &msg.request__binary();
            header = &request_binary->header();
            body_binary = &request_binary->body();
            buf.put_cs(request_binary->method());
            buf.put_c(' ');
            buf.put_cs(request_binary->uri());
            buf.put_cs(" HTTP/");
            buf.put_cs(int2str(request_binary->version__major()));
            buf.put_c('.');
            buf.put_cs(int2str(request_binary->version__minor()));
            buf.put_cs("\r\n");
        }
        else if(msg.get_selection() == HTTPmsg__Types::HTTPMessage::ALT_response__binary)
        {
            response_binary = &msg.response__binary();
            header = &response_binary->header();
            body_binary = &response_binary->body();
            buf.put_cs("HTTP/");
            buf.put_cs(int2str(response_binary->version__major()));
            buf.put_c('.');
            buf.put_cs(int2str(response_binary->version__minor()));
            buf.put_c(' ');
            buf.put_cs(int2str(response_binary->statuscode()));
            buf.put_c(' ');
            buf.put_cs(response_binary->statustext());
            buf.put_cs("\r\n");
        }

        for( int i = 0; i < header->size_of(); i++ )
        {
            buf.put_cs((*header)[i].header__name());
            buf.put_cs(": ");
            buf.put_cs((*header)[i].header__value());
            buf.put_cs("\r\n");
        }

        buf.put_cs("\r\n");

        if(body && body->lengthof() > 0)
        {
            buf.put_cs(*body);
        }
        else if(body_binary && body_binary->lengthof() > 0)
        {
            buf.put_os(*body_binary);
        }
    }
}

bool HTTPmsg__PT::HTTP_decode(TTCN_Buffer* buffer, const int client_id, const bool connection_closed)
{

  //HTTPmsg__Types::HTTPMessage * msg = new HTTPmsg__Types::HTTPMessage();

  HTTPmsg__Types::HTTPMessage msg;

  if(f_HTTP_decodeCommon(buffer, msg, connection_closed, get_socket_debugging(), test_port_type, test_port_name ))
  {
    TTCN_Logger::log(TTCN_DEBUG,"HTTPmsg__PT::HTTP_decode, before calling incoming_message");
    f_setClientId(msg,client_id);
    incoming_message(msg);
    TTCN_Logger::log(TTCN_DEBUG,"HTTPmsg__PT::HTTP_decode, after calling incoming_message");
    return true;
  }
  return false;
}

void f_setClientId( HTTPmsg__Types::HTTPMessage& msg, const int client_id)
{
  switch(msg.get_selection())
  {
    case HTTPmsg__Types::HTTPMessage::ALT_request:
    {
      msg.request().client__id()=client_id;
      break;
    }
    case HTTPmsg__Types::HTTPMessage::ALT_request__binary:
    {
      msg.request__binary().client__id()=client_id;
      break;
    }
    case HTTPmsg__Types::HTTPMessage::ALT_response:
    {
      msg.response().client__id()=client_id;
      break;
    }
    case HTTPmsg__Types::HTTPMessage::ALT_response__binary:
    {
      msg.response__binary().client__id()=client_id;
      break;
    }
    case HTTPmsg__Types::HTTPMessage::ALT_erronous__msg:  //is this case redundant code(?)
    {
      msg.erronous__msg().client__id()=OMIT_VALUE;
      break;
    }
    default:
      break;
  }//switch
  return;
}//f_setClientId

//
// returns with true if the buffer is not empty and it contain valid message
// Postcondition: if buffer contains valid message, msg will contain the first decoded HTTP message, the decoded part will be removed from the buffer
bool f_HTTP_decodeCommon( TTCN_Buffer* buffer, HTTPmsg__Types::HTTPMessage& msg, const bool connection_closed,
    const bool socket_debugging, const char *test_port_type, const char *test_port_name)
{

    TTCN_Logger::log(TTCN_DEBUG, "starting f_HTTP_decodeCommon ");
    if(buffer->get_read_len() <= 0)
        return FALSE;

    buffer->rewind();

    Decoding_Params decoding_params;
    decoding_params.non_persistent_connection = FALSE;
    decoding_params.chunked_body = FALSE;
    decoding_params.content_length = -1;
    decoding_params.error = FALSE;
    decoding_params.isMessage = TRUE;

    if (TTCN_Logger::log_this_event(TTCN_DEBUG))
    {
        if( test_port_name!= NULL)
          TTCN_Logger::log(TTCN_DEBUG, "%s DECODER: <%s>\n", test_port_name,
            (const char*)CHARSTRING(buffer->get_read_len(), (const char*)buffer->get_read_data()));
        else
          TTCN_Logger::log(TTCN_DEBUG, "DECODER: <%s>\n",
            (const char*)CHARSTRING(buffer->get_read_len(), (const char*)buffer->get_read_data()));
    }

    CHARSTRING first;
    bool isResponse;

    // Decoding the first line

    switch(get_line(buffer, first, false))
    {
    case TRUE: // The first line is available
        {
            //HTTPmsg__Types::HTTPMessage msg;
            HTTPmsg__Types::HeaderLines header = NULL_VALUE;
            OCTETSTRING body=OCTETSTRING(0, (const unsigned char*)"");
            const char *cc_first = (const char *)first;
            //fprintf(stderr, "first: %s\n", cc_first);
            int version__major, version__minor, statusCode;

            char* method_name;
            const char* pos = strchr(cc_first, ' ');
            if(pos == NULL)
            {
                TTCN_Logger::log(TTCN_DEBUG, "could not find space in the first line of response: <%s>", cc_first);
                decoding_params.isMessage = FALSE;
                decoding_params.error = TRUE;
                break;
            }
            method_name = (char*)Malloc(pos - cc_first + 1);
            strncpy(method_name, cc_first, pos - cc_first);
            method_name[pos - cc_first] = '\0';

            char* stext = (char*)Malloc(strlen(cc_first));
            stext[0] = '\0';
	    
            TTCN_Logger::log(TTCN_DEBUG, "method_name: <%s>", method_name);
            if(strncasecmp(method_name, "HTTP/", 5) == 0)
            {
                // The first line contains a response like HTTP/1.1 200 OK
                isResponse = true;

                if(sscanf(cc_first, "HTTP/%d.%d %d %[^\r]", &version__major, &version__minor,
                          &statusCode, stext) < 3)
                {
                    decoding_params.isMessage = FALSE;
                    decoding_params.error = TRUE;
            Free(method_name);
            Free(stext);
                    break;
                }
                if (version__minor == 0)
                    decoding_params.non_persistent_connection = TRUE;
            }
            else
            {
                isResponse = false;
                // The first line contains a request
                // like "POST / HTTP/1.0"
                if(sscanf(pos + 1, "%s HTTP/%d.%d",
                          stext, &version__major, &version__minor ) != 3)
                {
                    decoding_params.isMessage = FALSE;
                    decoding_params.error = TRUE;
            Free(method_name);
            Free(stext);
                    break;
                }
            }

            // Additional header lines
            TTCN_Logger::log(TTCN_DEBUG, "Decoding the headers");
            HTTP_decode_header(buffer, header, decoding_params, socket_debugging, isResponse, test_port_type, test_port_name);
            TTCN_Logger::log(TTCN_DEBUG, "Headers decoded. %s headers.", decoding_params.isMessage ? "Valid" : "Invalid");

            if(isResponse && decoding_params.content_length==-1){
              if( (statusCode>99 && statusCode <200) || statusCode==204 || statusCode==304 ) decoding_params.content_length=0;
            }

            if(decoding_params.isMessage)
                HTTP_decode_body(buffer, body, decoding_params, connection_closed, socket_debugging, test_port_type, test_port_name);

            if(decoding_params.isMessage)
            {
                TTCN_Logger::log(TTCN_DEBUG, "Message successfully decoded");
                bool foundBinaryCharacter = false;

                int len = body.lengthof();
                const unsigned char* ptr = (const unsigned char*)body;
                for(int i = 0; i < len && !foundBinaryCharacter; i++)
                {
                    if(!isascii(ptr[i]))
                        foundBinaryCharacter = true;
                }
                if(foundBinaryCharacter)
                    TTCN_Logger::log(TTCN_DEBUG, "Binary data found");
                if(isResponse)
                {
                  if(foundBinaryCharacter)
                  {
                    HTTPmsg__Types::HTTPResponse__binary__body& response_binary = msg.response__binary();
                    response_binary.client__id() = OMIT_VALUE;
                    response_binary.version__major() = version__major;
                    response_binary.version__minor() = version__minor;
                    response_binary.statuscode() = statusCode;
                    if(strlen(stext) > 0)
                        response_binary.statustext() = CHARSTRING(stext);
                    else
                        response_binary.statustext() = "";
                    response_binary.header() = header;
                    response_binary.body() = body;
                  }
                  else
                  {
                    HTTPmsg__Types::HTTPResponse& response = msg.response();
                    response.client__id() = OMIT_VALUE;
                    response.version__major() = version__major;
                    response.version__minor() = version__minor;
                    response.statuscode() = statusCode;
                    if(strlen(stext) > 0)
                        response.statustext() = CHARSTRING(stext);
                    else
                        response.statustext() = "";
                    response.header() = header;
                    response.body() = oct2char(body);
                  }
                }
                else
                {
                  if(foundBinaryCharacter)
                  {
                    HTTPmsg__Types::HTTPRequest__binary__body& request_binary = msg.request__binary();
                    request_binary.client__id() = OMIT_VALUE;
                    request_binary.method() = CHARSTRING(method_name);
                    request_binary.uri() = CHARSTRING(stext);
                    request_binary.version__major() = version__major;
                    request_binary.version__minor() = version__minor;
                    request_binary.header() = header;
                    request_binary.body() = body;
                  }
                  else
                  {
                    HTTPmsg__Types::HTTPRequest& request = msg.request();
                    request.client__id() = OMIT_VALUE;
                    request.method() = CHARSTRING(method_name);
                    request.uri() = CHARSTRING(stext);
                    request.version__major() = version__major;
                    request.version__minor() = version__minor;
                    request.header() = header;
                    request.body() = oct2char(body);
                  }
                }
                //incoming_message(msg); <- outer function calls if necessary
            }
            Free(method_name);
            Free(stext);
        }
        break;
    case BUFFER_CRLF:
    case BUFFER_FAIL:
        decoding_params.error = TRUE;
    case FALSE:
        decoding_params.isMessage = FALSE;
    }

    if(decoding_params.error)
    {

        if(buffer->get_read_len() > 0)
          msg.erronous__msg().msg() = CHARSTRING(buffer->get_read_len(), (const char*)buffer->get_read_data());
        else
          msg.erronous__msg().msg() = "The previous message is erronous.";
        msg.erronous__msg().client__id() = OMIT_VALUE;
        //incoming_message(msg);
        buffer->clear();
        decoding_params.isMessage = TRUE;
    }

    if(decoding_params.isMessage)
    {
        buffer->cut();
    }

    return decoding_params.isMessage;
}

void HTTP_decode_header(TTCN_Buffer* buffer, HTTPmsg__Types::HeaderLines& headers, Decoding_Params& decoding_params,
    const bool socket_debugging, const bool resp, const char *test_port_type, const char *test_port_name)
{
    CHARSTRING cstr;
    const char* separator;
    char* header_name = NULL;
    bool length_received = false;

    for(int i = 0; ; i++)
    {
        switch(get_line(buffer, cstr, true))
        {
        case TRUE:
            {
                char h[cstr.lengthof() + 1];
                strcpy(h, (const char*)cstr);
                separator = strchr(h, ':');
                if(separator)
                {
                    header_name = (char*)Realloc(header_name, separator - h + 1);
                    strncpy(header_name, h, separator - h);
                    header_name[separator - h] = '\0';
                    separator++;
                    while(*separator && isspace(separator[0]))
                        separator++;
                    char* end = h + strlen(h);
                    while(isspace((end - 1)[0]))
                    {
                        end--;
                        *end = '\0';
                    }
                    headers[i] = HTTPmsg__Types::HeaderLine(header_name, separator);
                    HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name, "+Header line: <%s: %s>", header_name, separator);

                    if(!strcasecmp(header_name, "Content-Length"))
                        { sscanf(separator, "%d", &decoding_params.content_length); length_received=true;}
                    else if(!strcasecmp(header_name, "Connection") && !strcasecmp(separator, "close"))
                        decoding_params.non_persistent_connection = TRUE;
                    else if(!strcasecmp(header_name, "Connection") && !strcasecmp(separator, "keep-alive"))
                        decoding_params.non_persistent_connection = FALSE;
                    else if(!strcasecmp(header_name, "Transfer-Encoding") && !strcasecmp(separator, "chunked"))
                        decoding_params.chunked_body = TRUE;

                }
                continue;
            }
        case BUFFER_FAIL:
            HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name, "BUFFER_FAIL in HTTP_decode_header!");
            HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name, "whole bufer now: <%s>", (const char*)buffer->get_data());
            log_to_hexa(buffer);
            decoding_params.error = TRUE;
        case FALSE:
            decoding_params.isMessage = FALSE;
        case BUFFER_CRLF:
            break;
        }
        break;
    }
    if(decoding_params.isMessage && !resp && !length_received && !decoding_params.chunked_body) decoding_params.content_length=0;
    Free(header_name);
}

void HTTP_decode_body(TTCN_Buffer* buffer, OCTETSTRING& body, Decoding_Params& decoding_params, const bool connection_closed,
    const bool socket_debugging, const char *test_port_type, const char *test_port_name)
{
    if(buffer->get_read_len() > 0)
        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "Decoding body, buffer length: %d", buffer->get_read_len());

    if (decoding_params.chunked_body)
    {
        HTTP_decode_chunked_body(buffer, body, decoding_params, socket_debugging, test_port_type, test_port_name);
        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "--------- After chunked body decoding:");
        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "--------- non_persistent_connection: %s",  decoding_params.non_persistent_connection ? "yes" : "no");
        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "--------- chunked_body: %s",  decoding_params.chunked_body ? "yes" : "no");
        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "--------- content_length: %d",  decoding_params.content_length);
        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "--------- error: %s",  decoding_params.error ? "yes" : "no");
        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "--------- isMessage: %s",  decoding_params.isMessage ? "yes" : "no");

    }
    else if(decoding_params.content_length >= 0)
    {
        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "lengthof body: %d, content_length given: %d", buffer->get_read_len(), decoding_params.content_length);
        if(buffer->get_read_len() >= (unsigned)decoding_params.content_length)
        {
            body = OCTETSTRING(decoding_params.content_length, buffer->get_read_data());
            buffer->set_pos(buffer->get_pos() + decoding_params.content_length);
        }
        else
        {
            decoding_params.isMessage = FALSE;
            HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "The decoder's body length %d is less than the Content_length in the message header %d; The HTTP port is waiting for additional data.", buffer->get_read_len(), decoding_params.content_length);
            buffer->set_pos(buffer->get_pos() + buffer->get_read_len());
        }
    }
    else if(connection_closed)
    {
       /* if(buffer->get_read_len() >= 0)*/ // Always true
        {
            body = OCTETSTRING(buffer->get_read_len(), buffer->get_read_data());
            buffer->set_pos(buffer->get_pos() + buffer->get_read_len());
        }
    } else {
            decoding_params.isMessage = FALSE;
            HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "The HTTP port is waiting for additional data.");
            buffer->set_pos(buffer->get_pos() + buffer->get_read_len());
    }
}

void HTTP_decode_chunked_body(TTCN_Buffer* buffer, OCTETSTRING& body, Decoding_Params& decoding_params,
    const bool socket_debugging, const char *test_port_type, const char *test_port_name)
{
    OCTETSTRING chunk;
    CHARSTRING line;
    unsigned int chunk_size = 1;

    while(chunk_size > 0)
    {
        switch(get_line(buffer, line, false))
        {
            case TRUE:
                HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "line: <%s>", (const char*)line);
                if(sscanf((const char *)line, "%x", &chunk_size) != 1)
                {
                    HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "No chunksize found");
                    body = body + OCTETSTRING(line.lengthof(), (const unsigned char*)(const char*)line);
                    chunk_size = 0;
                    decoding_params.error = TRUE;
                }
                else
                {
                    if(chunk_size == 0)
                    {
                        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name, "chunk_size 0 -> closing chunk");
                        if(get_line(buffer, line, false) == BUFFER_CRLF)
                            HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "Trailing \\r\\n ok!");
                        else
                            TTCN_Logger::log(TTCN_WARNING,"Trailing \\r\\n after the closing chunk is not present, instead it is <%s>!", (const char*)line);
                    }
/*                    else if(chunk_size < 0) // the chunk_size is unsigned, never true
                    {
                        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "chunk_size less than 0");
                        decoding_params.error = TRUE;
                        chunk_size = 0;
                    }*/
                    else // chunk_size > 0
                    {
                        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "processing next chunk, size: %d", chunk_size);
                        if(buffer->get_read_len() < chunk_size)
                        {
                            HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "chunk size is greater than the buffer length, more data is needed");
                            decoding_params.isMessage = FALSE;
                            chunk_size = 0;
                        }
                    }
                }
                break;
            case FALSE:
                HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "buffer does not contain a whole line, more data is needed");
                decoding_params.isMessage = FALSE;
                chunk_size = 0;
                break;
            case BUFFER_CRLF:
                HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "beginning CRLF removed");
                continue;
            case BUFFER_FAIL:
                HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "BUFFER_FAIL");
                decoding_params.error = FALSE;
                chunk_size = 0;
                break;
            default:
                decoding_params.isMessage = FALSE;
                chunk_size = 0;
                HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "more data is needed");
        }

        body = body + OCTETSTRING(chunk_size, buffer->get_read_data());
        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "pull %d bytes from %d", chunk_size, buffer->get_read_len());
        buffer->set_pos(buffer->get_pos() + chunk_size);
        // hack
        if(buffer->get_read_len() && buffer->get_read_data()[0] == '\n')  // don't read from the buffer if there is nothing in it.
        {
            HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,"hack: adjusted buffer position after the '\\n'");
            buffer->set_pos(buffer->get_pos() + 1);
        }
        HTTPmsg__Types::log_debug(socket_debugging, test_port_type, test_port_name,  "remaining data: <%s>, len: %d", (const char *)CHARSTRING(buffer->get_read_len(), (const char*)buffer->get_read_data()), buffer->get_read_len());
    }
}

int get_line(TTCN_Buffer* buffer, CHARSTRING& to, const bool concatenate_header_lines)
{
    unsigned int i = 0;
    const unsigned char *cc_to = buffer->get_read_data();

    if(!buffer->get_read_len())
        return FALSE;

    while(1)
    {
        for( ; i < buffer->get_read_len() && cc_to[i] != '\0' && cc_to[i] != '\r' && cc_to[i] != '\n'; i++);

        if(i >= buffer->get_read_len())
        {
            to = CHARSTRING("");
            return FALSE;
        }
        else
        {
            if(cc_to[i] == '\n') {//
              if(report_lf){
                switch(HTTPmsg__Types::crlf__mode){
                  case HTTPmsg__Types::strict__crlf__mode::ERROR_:
                    return BUFFER_FAIL;
                    break;
                  case HTTPmsg__Types::strict__crlf__mode::WARNING__ONCE:
                    report_lf=false;
                    // no break
                  case HTTPmsg__Types::strict__crlf__mode::WARNING:
                    TTCN_warning("Missing '\\r'.");
                    break;
                  default:
                    break;
                }
              }
              if(i > 0 && (i + 1) < buffer->get_read_len() && concatenate_header_lines && (cc_to[i+1] == ' ' || cc_to[i+1] == '\t'))
                    i += 1;
                  else
                  {
                      to = CHARSTRING(i, (const char*)cc_to);
                      buffer->set_pos(buffer->get_pos() + i + 1);
                      return i == 0 ? BUFFER_CRLF : TRUE;
                  }

            } else
            {
                if((i + 1) < buffer->get_read_len() && cc_to[i + 1] != '\n')
                  return BUFFER_FAIL;
                else if(i > 0 && (i + 2) < buffer->get_read_len() && concatenate_header_lines && (cc_to[i+2] == ' ' || cc_to[i+2] == '\t'))
                  i += 2;
                else
                {
                    to = CHARSTRING(i, (const char*)cc_to);
                    buffer->set_pos(buffer->get_pos() + i + 2);
                    return i == 0 ? BUFFER_CRLF : TRUE;
                }
            }
        }
    }
}

void log_to_hexa(TTCN_Buffer* buffer)
{
    int len = buffer->get_read_len();
    const unsigned char* ptr = buffer->get_read_data();
    for(int i = buffer->get_pos(); i < len; i++)
    {
        TTCN_Logger::log_event(" %02X", ptr[i]);
    }
}


const char* HTTPmsg__PT::local_port_name()              { return "";}
const char* HTTPmsg__PT::remote_address_name()          { return "";}
const char* HTTPmsg__PT::local_address_name()           { return "";}
const char* HTTPmsg__PT::remote_port_name()             { return "";}
const char* HTTPmsg__PT::use_notification_ASPs_name()   { return "use_notification_ASPs";}
const char* HTTPmsg__PT::halt_on_connection_reset_name(){ return "";}
const char* HTTPmsg__PT::server_mode_name()             { return "";}
const char* HTTPmsg__PT::socket_debugging_name()        { return "http_debugging";}
const char* HTTPmsg__PT::nagling_name()                 { return "";}
const char* HTTPmsg__PT::server_backlog_name()          { return "server_backlog";}
const char* HTTPmsg__PT::ssl_use_ssl_name()                { return "";}
const char* HTTPmsg__PT::ssl_use_session_resumption_name() { return "";}
const char* HTTPmsg__PT::ssl_private_key_file_name()       { return "KEYFILE";}
const char* HTTPmsg__PT::ssl_trustedCAlist_file_name()     { return "TRUSTEDCALIST_FILE";}
const char* HTTPmsg__PT::ssl_certificate_file_name()       { return "CERTIFICATEFILE";}
const char* HTTPmsg__PT::ssl_password_name()               { return "PASSWORD";}
const char* HTTPmsg__PT::ssl_verifycertificate_name()      { return "VERIFYCERTIFICATE";}


} //eof namespace "HTTPmsg__PortType"

namespace HTTPmsg__Types {

using namespace HTTPmsg__PortType;

//=========================================================================
//==== Working Functions independent from sending and receiving:===
//=========================================================================

//from AbstractSocket
void log_debug(const bool socket_debugging, const char *test_port_type, const char *test_port_name, const char *fmt, ...)
{
  if (socket_debugging) {
    TTCN_Logger::begin_event(TTCN_DEBUG);
    if ((test_port_type!=NULL && test_port_name!=NULL)&&(strlen(test_port_type)!=0 && strlen(test_port_name)!=0))
       TTCN_Logger::log_event("%s test port (%s): ", test_port_type, test_port_name);
    va_list args;
    va_start(args, fmt);
    TTCN_Logger::log_event_va_list(fmt, args);
    va_end(args);
    TTCN_Logger::end_event();
  }
}

void log_warning(const char *test_port_type, const char *test_port_name, const char *fmt, ...)
{
  TTCN_Logger::begin_event(TTCN_WARNING);
  if (test_port_type!=NULL && test_port_name!=NULL)
       TTCN_Logger::log_event("%s test port (%s): ", test_port_type, test_port_name);
  va_list args;
  va_start(args, fmt);
  TTCN_Logger::log_event_va_list(fmt, args);
  va_end(args);
  TTCN_Logger::end_event();
}

//=========================================================================
//==== Encoder-decoder Functions independent from sending and receiving:===
//=========================================================================

/*********************************************************
* Function: enc__HTTPMessage
*
* Purpose:
*    To encode msg type of HTTPMessage into OCTETSTRING separated from sending functionality
*    It is for users using this test port as a protocol module
*
* References:
*   RFC2616
*
* Precondition:
*  msg is filled in properly
* Postcondition:
*
*
* Parameters:
*  msg - the HTTP Message to be encoded
*
* Return Value:
*   OCTETSTRING - the encoded message
* Detailed Comments:
*   -
*
*********************************************************/
OCTETSTRING enc__HTTPMessage( const HTTPmsg__Types::HTTPMessage& msg ) {
  TTCN_Buffer buf;
  buf.clear();
  HTTPmsg__PortType::f_HTTP_encodeCommon( msg, buf);
  return OCTETSTRING(buf.get_len(), buf.get_data());
}
/*********************************************************
* Function: dec__HTTPMessage
*
* Purpose:
*    To decode msg type of OCTETSTRING into HTTPMessage separated from receiving functionality
*    It is for users using this test port as a protocol module
*
* References:
*   RFC2616
*
* Precondition:
*  stream is filled in properly
* Postcondition:
*  -
*
* Parameters:
*  stream - the message to be decoded
*  msg    - reference to the record type of HTTPMessage which will contain the decoded value if the return value less than the length of the original stream
* Return Value:
*   integer - the length of the remaining data which is not decoded yet.
* Detailed Comments:
*   If the full stream is decoded, the return value is zero
*   If nothing is decoded (decoding failed) the return value equals to the original length of the stream
*
*********************************************************/

INTEGER dec__HTTPMessage(OCTETSTRING const& stream, HTTPMessage& msg, const BOOLEAN& socket_debugging =  dec__HTTPMessage_socket__debugging_defval )
{
  TTCN_Logger::log(TTCN_DEBUG, "starting HTTPmsg__Types::dec__HTTPMessage");
  TTCN_Buffer *buf_p = new TTCN_Buffer() ;
  buf_p->put_os(stream);

  int buf_len = buf_p->get_read_len();
  if( buf_len > 0)
  {
      if(f_HTTP_decodeCommon(buf_p, msg, true, socket_debugging, NULL, NULL))
      {
        log_debug(socket_debugging,"","","dec__HTTPMessage, after decoding:\nbuf_len: %d\nget_len: %d\nget_read_len:%d",
            buf_len,
            buf_p->get_len(),
            buf_p->get_read_len());
        buf_len = buf_p->get_read_len(); //remaining data length
      }
      else
        buf_len = -1;


  } else buf_len = -1;
  delete buf_p;
  return buf_len;
}

}//namespace
