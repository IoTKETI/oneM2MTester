/*******************************************************************************
* Copyright (c) 2000-2017 Ericsson Telecom AB
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*
* Contributors:
*     Zoltan Janos Szabo (Ericsson) - initial architecture design and implementation
*     Roland Gecse (Ericsson) - initial architecture design
*     Akos Cserveni (Ericsson) - Basic AST in compiler, semantic checking
*     Gabor Szalai (Ericsson) – RAW and TEXT codecs
*     Matyas Forstner (Ericsson) - ASN.1 extension of the compiler and BER/CER/DER codecs
*     Kristof Szabados  (Ericsson) - Eclipse Designer, Executor, Titanium UIs
*     Szabolcs Beres (Ericsson) - Eclipse LogViewer
*     Ferenc Kovacs (Ericsson) – log interfaces, big number support, subtype checking
*     Csaba Raduly (Ericsson) – ASN.1 additions, XML encoder/decoder
*     Adam Delic (Ericsson) – template restrictions, try&catch, support of pre-processor directives in Eclipse
*     Krisztian Pandi (Ericsson) – import of imports
*     Peter Dimitrov (Ericsson)- maintenance
*     Balazs Andor Zalanyi (Ericsson) – code splitting
*     Gabor Szalai (Ericsson) – RAW encoding/decoding
*     Jeno Attila Balasko (Ericsson) – tests
*     Csaba Feher (Ericsson) – epoll support
*     Tamas Buti (Ericsson)- maintenance
*     Matyas Ormandi (Ericsson) - maintenance
*     Botond Baranyi (Ericsson) - JSON encoder
*     Arpad Lovassy (Ericsson) - Java Executor API
*     Laszlo Baji (Ericsson) - maintenance
*     Marton Godar (Ericsson) - xsd2ttcn converter
*******************************************************************************/
//
//  File:               PIPEasp_PT.hh
//  Description:        Header file of PIPE testport implementation
//  Rev:                <RnXnn>
//  Prodnr:             CNL 113 334
//  Updated:            2008-06-03
//  Contact:            http://ttcn.ericsson.se
//


#ifndef PIPEasp__PT_HH
#define PIPEasp__PT_HH

#include "PIPEasp_PortType.hh"

namespace PIPEasp__PortType {

class PIPEasp__PT : public PIPEasp__PT_BASE {
public:
	PIPEasp__PT(const char *par_port_name = NULL);
	~PIPEasp__PT();

	void set_parameter(const char *parameter_name,
		const char *parameter_value);

	void Event_Handler(const fd_set *read_fds,
		const fd_set *write_fds, const fd_set *error_fds,
		double time_since_last_call);

protected:
	void user_map(const char *system_port);
	void user_unmap(const char *system_port);

	void user_start();
	void user_stop();

	void outgoing_send(const PIPEasp__Types::ASP__PExecute& send_par);
	void outgoing_send(const PIPEasp__Types::ASP__PExecuteBinary& send_par);
	void outgoing_send(const PIPEasp__Types::ASP__PExecuteBackground& send_par);
	void outgoing_send(const PIPEasp__Types::ASP__PStdin& send_par);
	void outgoing_send(const PIPEasp__Types::ASP__PStdinBinary& send_par);
	void outgoing_send(const PIPEasp__Types::ASP__PKill& send_par);
	void outgoing_send(const PIPEasp__Types::ASP__PLineMode& send_par);
	void outgoing_send(const PIPEasp__Types::ASP__PEndOfInput& send_par);
private:
        int execCommand(const char* command);
        void handle_childDeath();
        void sendStdout();
        void sendStderr();
        void sendExitCode();
        void sendResult();
        void sendError(const char* error_msg);
        void log(const char *fmt, ...);

private:
  bool lineMode; // true if lineMode is enabled
  bool processExecuting; // true if process is executing: disable new processes
  bool binaryMode; // true if result should be returned in as binary data
  bool disableSend; // if true sendStdout/err is disabled

  fd_set readfds;     // fd set for event handler
  int processPid;     // pid of the process currently executing
  int processStdin;   // fd of stdin of the process
  int processStdout;  // fd of stdout of the process
  int processStderr;  // fd of stderr of the process

  TTCN_Buffer stdout_buffer; // data sent to stdout 
  TTCN_Buffer stderr_buffer; // data sent to stderr
  int processExitCode;       // exit code of the process

};

}//namespace

#endif
