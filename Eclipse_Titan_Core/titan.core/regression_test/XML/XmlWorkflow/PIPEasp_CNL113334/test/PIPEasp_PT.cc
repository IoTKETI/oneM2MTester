/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *
 ******************************************************************************/
/*******************************************************************************
* Contributors:
*     Zoltan Janos Szabo (Ericsson) - initial architecture design and implementation
*     Roland Gecse (Ericsson) - initial architecture design
*     Akos Cserveni (Ericsson) - Basic AST in compiler, semantic checking
*     Gabor Szalai (Ericsson) - RAW and TEXT codecs
*     Matyas Forstner (Ericsson) - ASN.1 extension of the compiler and BER/CER/DER codecs
*     Kristof Szabados  (Ericsson) - Eclipse Designer, Executor, Titanium UIs
*     Szabolcs Beres (Ericsson) - Eclipse LogViewer
*     Ferenc Kovacs (Ericsson) - log interfaces, big number support, subtype checking
*     Csaba Raduly (Ericsson) - ASN.1 additions, XML encoder/decoder
*     Adam Delic (Ericsson) - template restrictions, try&catch, support of pre-processor directives in Eclipse
*     Krisztian Pandi (Ericsson) -  import of imports
*     Peter Dimitrov (Ericsson)- maintenance
*     Balazs Andor Zalanyi (Ericsson) - code splitting
*     Gabor Szalai (Ericsson) - RAW encoding/decoding
*     Jeno Attila Balasko (Ericsson) - tests, maintenance
*     Csaba Feher (Ericsson) - epoll support
*     Tamas Buti (Ericsson)- maintenance
*     Matyas Ormandi (Ericsson) - maintenance
*     Botond Baranyi (Ericsson) - JSON encoder
*     Arpad Lovassy (Ericsson) - Java Executor API
*     Laszlo Baji (Ericsson) - maintenance
*     Marton Godar (Ericsson) - xsd2ttcn converter
*******************************************************************************/
//
//  File:               PIPEasp_PT.cc
//  Description:        Source code of PIPE testport implementation
//  Rev:                <RnXnn>
//  Prodnr:             CNL 113 334
//  Updated:            2008-06-03
//  Contact:            http://ttcn.ericsson.se
//


#include "PIPEasp_PT.hh"
#include <signal.h> //kill
#include <unistd.h> //pipe
#include <errno.h>  //errno
#include <ctype.h>  //isspace
#include <sys/select.h>  //FD_ZERO
#include <stdio.h>      // sys_errlist
#include <sys/types.h>  //wait
#include <sys/wait.h>   //wait

#ifndef PIPE_BUF_SIZE
#define PIPE_BUF_SIZE 655536
#endif

namespace PIPEasp__PortType {

PIPEasp__PT::PIPEasp__PT(const char *par_port_name)
	: PIPEasp__PT_BASE(par_port_name)
        , lineMode(true)
        , processExecuting(false)
        , binaryMode(false)
        , disableSend(false)
        , processPid(-1)     // pid of the process currently executing
        , processStdin(-1)   // fd of stdin of the process
        , processStdout(-1)  // fd of stdout of the process
        , processStderr(-1)  // fd of stderr of the process
        , processExitCode(0) // exit code of the process
{
  FD_ZERO(&readfds);
  stdout_buffer.clear();
  stderr_buffer.clear();
}

PIPEasp__PT::~PIPEasp__PT()
{
// nothing to do
}

void PIPEasp__PT::set_parameter(const char *parameter_name,
	const char *parameter_value)
{
// no config parameters
}

void PIPEasp__PT::Event_Handler(const fd_set *read_fds,
	const fd_set *write_fds, const fd_set *error_fds,
	double time_since_last_call)
{
  log("PIPEasp__PT::Event_Handler called");
  if (processStdout != -1 && FD_ISSET(processStdout, read_fds)) {
    if (!processExecuting) {
      TTCN_warning("Unexpected message from stdout, no command is executing");
    } else {
      log("Incoming stdout message received from process");
    }
    
    long nBytes;
    int r;

    nBytes = PIPE_BUF_SIZE;
    unsigned char* buffer;
    size_t end_len = nBytes;
    stdout_buffer.get_end(buffer, end_len);
    r = read(processStdout,buffer,(int)nBytes);
    if (r <= 0) {
	log("ttcn_pipe_port: read problem on stdout.\n");
        // close the pipe and remove it from event handler
        close(processStdout);
        FD_CLR(processStdout, &readfds);
        Install_Handler(&readfds, NULL, NULL, 0.0);
        processStdout = -1;

        // check if stderr is closed:
        if (processStderr == -1) {
          // child died
          log("Child might have died.");
          handle_childDeath();
        }
    }
    else {
      log("incoming stdout %ld bytes\n", r);
      stdout_buffer.increase_length(r);
      sendStdout();
    }
    return;
  }
  if (processStderr != -1 && FD_ISSET(processStderr, read_fds)) {
    if (!processExecuting) {
      TTCN_warning("Unexpected message from stderr, no command is executing");
    } else {
      log("Incoming stderr message received from process");
    }
    
    long nBytes;
    int r;

    nBytes = PIPE_BUF_SIZE;
    unsigned char* buffer;
    size_t end_len = nBytes;
    stderr_buffer.get_end(buffer, end_len);
    r = read(processStderr,buffer,(int)nBytes);
    if (r <= 0) {
	log("ttcn_pipe_port: read problem on stderr.\n");
        // close the pipe and remove it from event handler
        close(processStderr);
        FD_CLR(processStderr, &readfds);
        Install_Handler(&readfds, NULL, NULL, 0.0);
        processStderr = -1;

        // check if stdout is closed:
        if (processStdout == -1) {
          // child died
          log("Child might have died.");
          handle_childDeath();
        }
    }
    else {
      log("incoming stderr %ld bytes\n", r);
      stderr_buffer.increase_length(r);
      sendStderr();
    }
    return;
  }
}

void PIPEasp__PT::user_map(const char *system_port)
{
// nothing to do
}

void PIPEasp__PT::user_unmap(const char *system_port)
{
// nothing to do
}

void PIPEasp__PT::user_start()
{
// nothing to do
}

void PIPEasp__PT::user_stop()
{
// nothing to do
}

/*************************************
*  Specific outgoing_send functions
*************************************/
void PIPEasp__PT::outgoing_send(const PIPEasp__Types::ASP__PExecute& send_par) {
  log("PIPEasp__PT::outgoing_send_PExecute called");
  // disable sendStdout, sendStderr until process exits
  if (processExecuting) {
    sendError("Pipe Test Port: Command already executing");
    TTCN_Logger::begin_event(TTCN_DEBUG);
    TTCN_Logger::log_event("PIPE test port (%s): Command already executing. Following ASP is ignored: ", get_name());
    send_par.log();
    TTCN_Logger::end_event();
    return;
  }
  PIPEasp__Types::ASP__PExecuteBackground message_PExecuteBackground;
  // starting command
  message_PExecuteBackground.command() = send_par.command();
  outgoing_send(message_PExecuteBackground);
  // sending input
  PIPEasp__Types::ASP__PStdin message_PStdin;
  message_PStdin.stdin_() = send_par.stdin_();
  outgoing_send(message_PStdin);
  disableSend = true;

  // closing stdin pipe:
  outgoing_send(PIPEasp__Types::ASP__PEndOfInput());
  
  log("PIPEasp__PT::outgoing_send_PExecute exited");
}

void PIPEasp__PT::outgoing_send(const PIPEasp__Types::ASP__PExecuteBinary& send_par) {
  log("PIPEasp__PT::outgoing_send_PExecuteBinary called");
  // disable sendStdout, sendStderr until process exits
  if (processExecuting) {
    sendError("Pipe Test Port: Command already executing");
    TTCN_Logger::begin_event(TTCN_DEBUG);
    TTCN_Logger::log_event("PIPE test port (%s): Command already executing. Following ASP is ignored: ", get_name());
    send_par.log();
    TTCN_Logger::end_event();
    return;
  }
  PIPEasp__Types::ASP__PExecuteBackground message_PExecuteBackground;
  // starting command
  message_PExecuteBackground.command() = send_par.command();
  outgoing_send(message_PExecuteBackground);
  // sending input
  PIPEasp__Types::ASP__PStdinBinary message_PStdinBinary;
  message_PStdinBinary.stdin_() = send_par.stdin_();
  outgoing_send(message_PStdinBinary);
  disableSend = true;
  
  // closing stdin pipe:
  outgoing_send(PIPEasp__Types::ASP__PEndOfInput());
  
  log("PIPEasp__PT::outgoing_send_PExecuteBinary exited");
}

void PIPEasp__PT::outgoing_send(const PIPEasp__Types::ASP__PExecuteBackground& send_par) {
  log("PIPEasp__PT::outgoing_send_PExecuteBackground called");
  
  if (processExecuting) {
    log("Process already executing. Cannot start new process.");
    sendError("Pipe Test Port: Command already executing");
    TTCN_Logger::begin_event(TTCN_DEBUG);
    TTCN_Logger::log_event("PIPE test port (%s): Command already executing. Following ASP is ignored: ", get_name());
    send_par.log();
    TTCN_Logger::end_event();
    log("PIPEasp__PT::outgoing_send_PExecuteBackground exited");
    return;
  }

  // creating pipes for process
  int pipesStdin[2];
  int pipesStdout[2];
  int pipesStderr[2];
				    
  if (pipe(pipesStdin) != 0) {
      sendError("Pipe Test Port: Cannot create stdin pipe");
      log("PIPEasp__PT::outgoing_send_PExecuteBackground exited");
      return;
  }
  if (pipe(pipesStdout) != 0) {
      sendError("Pipe Test Port: Cannot create stdout pipe");
      log("PIPEasp__PT::outgoing_send_PExecuteBackground exited");
      return;
  }
  if (pipe(pipesStderr) != 0) {
      sendError("Pipe Test Port: Cannot create stderr pipe");
      log("PIPEasp__PT::outgoing_send_PExecuteBackground exited");
      return;
  }
  
  processStdin = pipesStdin[1];
  processStdout = pipesStdout[0];
  processStderr = pipesStderr[0];

  processPid = fork();
  if (processPid < 0) {
    //
    // Error
    //

    // close the pipes
    close(pipesStdin[0]);
    close(pipesStdout[1]);
    close(pipesStderr[1]);

    close(processStdin);
    close(processStdout);
    close(processStderr);

    sendError("Pipe Test Port: Cannot fork");
  }
  else if (processPid == 0) {

      //
      // Child process
      //

      // close the parent end of the pipes
      close(processStdin);
      close(processStdout);
      close(processStderr);

      /*// set these to the other end of the pipe
      processStdin = pipesStdin[0];
      processStdout = pipesStdout[1];
      processStderr = pipesStderr[1];*/

      int r;
      // redirect pipeStdin to stdin
      r = dup2(pipesStdin[0], 0);
      if (r<0) {
        TTCN_error("Cannot redirect stdin");
        exit(errno);
      }

      // redirect pipeStdout to stdout
      r = dup2(pipesStdout[1], 1);
      if (r<0) {
        TTCN_error("Cannot redirect stdout");
        exit(errno);
      }

      // redirect pipeStderr to stderr
      r = dup2(pipesStderr[1], 2);
      if (r<0) {
        TTCN_error("Cannot redirect stderr");
        exit(errno);
      }

      processExitCode = execCommand((const char*)send_par.command());
      
      // There is a problem executing the command
      // Exiting...
      
      fflush(stdout);
      fflush(stderr);
      
      //closing pipes:
      close(pipesStdin[0]);
      close(pipesStdout[1]);
      close(pipesStderr[1]);
      
      exit(processExitCode); // end of child process
  }
  else {
					
    //
    // Parent process
    //

    log("Process started with pid: %d", processPid);
    // close child end of the pipes
    close(pipesStdin[0]);
    close(pipesStdout[1]);
    close(pipesStderr[1]);
    
    
    processExecuting = true;

    // install handler for the process pipes:
    //FD_SET(processStdin, &readfds);
    FD_SET(processStdout, &readfds);
    FD_SET(processStderr, &readfds);
    
    Install_Handler(&readfds, NULL, NULL, 0.0);
 }
  
  log("PIPEasp__PT::outgoing_send_PExecuteBackground exited");
}

void PIPEasp__PT::outgoing_send(const PIPEasp__Types::ASP__PStdin& send_par) {
  
  log("PIPEasp__PT::outgoing_send_PStdin called");
  binaryMode = false;
  if (!processExecuting) {  
    sendError("Pipe Test Port: No command executing");
    return;
  }
  if (disableSend) {  // process was started with PExecute(Binary)
    sendError("Pipe Test Port: PStdin is not sent: current process is not started with PExecuteBackground!");
    return;
  }

  log("will now write to stdin: '%s'",
		 (const char*)(send_par.stdin_()+((lineMode)?"\n":"")));
  write(processStdin,
	(const char*)(send_par.stdin_()+((lineMode)?"\n":"")),
	send_par.stdin_().lengthof()+((lineMode)?1:0));
  
  log("PIPEasp__PT::outgoing_send_PStdin exited");
}

void PIPEasp__PT::outgoing_send(const PIPEasp__Types::ASP__PStdinBinary& send_par) {
  log("PIPEasp__PT::outgoing_send_PStdinBinary called");
  binaryMode = true;
  if (!processExecuting) {  
    sendError("Pipe Test Port: No command executing");
    return;
  }
  if (disableSend) {  // process was started with PExecute(Binary)
    sendError("Pipe Test Port: PStdinBinary is not sent: current process is not started with PExecuteBackground!");
    return;
  }

  TTCN_Logger::begin_event(TTCN_DEBUG);
  TTCN_Logger::log_event("PIPE test port (%s): will now write binary data to stdin: ", get_name());
  send_par.stdin_().log();
  TTCN_Logger::end_event();

  write(processStdin,
	(const char*)(const unsigned char*)(send_par.stdin_()),
	send_par.stdin_().lengthof());
  log("PIPEasp__PT::outgoing_send_PStdinBinary exited");
}

void PIPEasp__PT::outgoing_send(const PIPEasp__Types::ASP__PKill& send_par) {
  log("PIPEasp__PT::outgoing_send_PKill called");
  if (!processExecuting) {
    // no process is running
    log("No process executing.");
    sendError("Pipe Test Port: No command executing");
    log("PIPEasp__PT::outgoing_send_PKill exited");
    return;
  }
  
  int signo = (int)send_par.signal();
  if (signo<1 || signo>31) {
    // signo out of range;
    log("Signo out of range.");
    sendError(
      "Pipe Test port: Signal number should be "
      "between 1 and 31");
    log("PIPEasp__PT::outgoing_send_PKill exited");
    return;
  }
  // killing process
  log("Killing process %d, signo: %d", processPid, signo);
  int r = kill(processPid, signo);
  log("Kill process returned %d", r);
  log("PIPEasp__PT::outgoing_send_PKill exited");
}

void PIPEasp__PT::outgoing_send(const PIPEasp__Types::ASP__PLineMode& send_par) {
  log("PIPEasp__PT::outgoing_send_PLineMode called");
  lineMode = (bool)send_par.lineMode();
  log("LineMode is set to %s", (lineMode)?"TRUE":"FALSE");
  log("PIPEasp__PT::outgoing_send_PLineMode exited");
}

void PIPEasp__PT::outgoing_send(const PIPEasp__Types::ASP__PEndOfInput& send_par) {
  log("PIPEasp__PT::outgoing_send_PEndOfInput called");
  // close stdin pipe
  close(processStdin);
  processStdin = -1;
  log("stdin closed");
  log("PIPEasp__PT::outgoing_send_PEndOfInput exited");
}

/********************************
* Execute the given command
* returns the exitcode of the process
*********************************/
int PIPEasp__PT::execCommand(const char* command) {
  log("PIPEasp__PT::execCommand called");
  log("Executing command: %s", command);

  // with this it is not possible to access the pid of the process
  //return system(command);

  int argc = 0;
  char* argv[1024];

  CHARSTRING temp = "";
  for (int i = 0; command[i] != 0; i++) {
      if (isspace(command[i])) {
	  argv[argc++] = strdup(temp);
          log("command argument added: %s", (const char*)temp);
	  while (command[i] != '0' && isspace(command[i])) i++;
	  i--;
	  temp = "";
      } else {
	  temp = temp + CHARSTRING(1, command+i);
      }
  }

  if (temp != "") {
      argv[argc++] = strdup(temp);
      log("command argument added: %s", (const char*)temp);
  }

  argv[argc++] = (char*)NULL;

  log("execCommand(%s,%d)\n", argv[0], argc);
  execvp(argv[0],argv);

  fprintf(stderr,"Error executing command %s (%d): %s\n",
    argv[0], errno, strerror(errno));
  fflush(stderr);
//  exit(errno);
  return errno;
}

/***********************************
* if the the child process died, gets
* the exit code and sends it to TTCN
* should be called when stdout/err are closed
************************************/
void PIPEasp__PT::handle_childDeath() {
  log("Getting process status for pid %d...", processPid);
  processExitCode = 0;  // reset the exitcode
  int pid = wait(&processExitCode);
  //waitpid(processPid,&processExitCode, 0);
  if (pid!=processPid) {
    log("other child died with pid: %d, exit code: %d", pid, processExitCode);
    return;
  }

  log("Child process exit status is: %d", processExitCode);
  // send code to TTCN:
  sendExitCode();
  // send result to TTCN
  sendResult();

  // removing fd-s installed for the process:
  Uninstall_Handler(); // no handler is needed
  FD_ZERO(&readfds);
  /*
  // equivalent with:
  //FD_CLR(processStdin, &readfds);
  FD_CLR(processStdout, &readfds);
  FD_CLR(processStderr, &readfds);
  Install_Handler(&readfds, NULL, NULL, 0.0);
  */

  // closing pipes:
  close(processStdin);
  //close(processStdout); // already closed
  //close(processStderr); // already closed

  processStdin = -1;
  //processStdout = -1;
  //processStderr = -1;

  processExecuting = false;
  disableSend = false;
}

/***************************
* Send stdout msg to TTCN
***************************/
void PIPEasp__PT::sendStdout() {
  if (disableSend) return;

  PIPEasp__Types::ASP__PStdout message_PStdout;
  PIPEasp__Types::ASP__PStdoutBinary message_PStdoutBinary;
  if (lineMode && !binaryMode) {
    // send complete lines from buffer
    const unsigned char* pos = stdout_buffer.get_read_data();
    for(unsigned int i=0; i<stdout_buffer.get_read_len(); i++) {
      // not end of line:
      if (pos[i] != '\n') {
        continue;
      }
      
      // at end of line
      // length of data is i (+1 is for \n and is not sent)
      message_PStdout.stdout_() = CHARSTRING(i, (const char*)pos);
      
      // send message
      incoming_message(message_PStdout);
      
      // remove the complete line from buffer,
      // also set i and pos to the beginning of buffer
      stdout_buffer.set_pos(i+1);
      stdout_buffer.cut();
      i = 0;
      pos = stdout_buffer.get_read_data();
    }
  } else {
    // lineMode false or binaryMode true
    if (binaryMode) {
      message_PStdoutBinary.stdout_() =
        OCTETSTRING(stdout_buffer.get_read_len(), stdout_buffer.get_read_data());
      stdout_buffer.clear();
      incoming_message(message_PStdoutBinary);
    }
    else {
      message_PStdout.stdout_() = 
        CHARSTRING(stdout_buffer.get_read_len(), (const char*)stdout_buffer.get_read_data());
      stdout_buffer.clear();
      incoming_message(message_PStdout);
    }
//    incoming_message(message);
  }
}


/***************************
* Send stderr msg to TTCN
***************************/
void PIPEasp__PT::sendStderr() {
  if (disableSend) return;

  PIPEasp__Types::ASP__PStderr message_PStderr;
  PIPEasp__Types::ASP__PStderrBinary message_PStderrBinary;
  if (lineMode && !binaryMode) {
    // send complete lines from buffer
    const unsigned char* pos = stderr_buffer.get_read_data();
    for(unsigned int i=0; i<stderr_buffer.get_read_len(); i++) {
      // not end of line:
      if (pos[i] != '\n') {
        continue;
      }
      
      // at end of line
      // length of data is i (+1 is for \n and is not sent)
      message_PStderr.stderr_() = CHARSTRING(i, (const char*)pos);
      
      // send message
      incoming_message(message_PStderr);
      
      // remove the complete line from buffer,
      // also set i and pos to the beginning of buffer
      stderr_buffer.set_pos(i+1);
      stderr_buffer.cut();
      i = 0;
      pos = stderr_buffer.get_read_data();
    }
  } else {
    // lineMode false or binaryMode true
    if (binaryMode) {
      message_PStderrBinary.stderr_() =
        OCTETSTRING(stderr_buffer.get_read_len(), stderr_buffer.get_read_data());
      stderr_buffer.clear();
      incoming_message(message_PStderrBinary);
    }
    else {
      message_PStderr.stderr_() = 
        CHARSTRING(stderr_buffer.get_read_len(), (const char*)stderr_buffer.get_read_data());
      stderr_buffer.clear();
      incoming_message(message_PStderr);
    }
//    incoming_message(message);
  }
}


/***************************
* Send exitcode msg to TTCN
***************************/
void PIPEasp__PT::sendExitCode() {
  if (disableSend) return;

  log("Sending ExitCode to TTCN");
  PIPEasp__Types::ASP__PExit message_PExit;
  message_PExit.code() = processExitCode;
  incoming_message(message_PExit);
}


/***************************
* Send error msg to TTCN
***************************/
void PIPEasp__PT::sendError(const char* error_msg) {
  PIPEasp__Types::ASP__PError message_PError;
  message_PError.errorMessage() = error_msg;
  incoming_message(message_PError);
}


/***************************
* Send Result msg to TTCN
***************************/
void PIPEasp__PT::sendResult() {
  if (!disableSend) return; // do not send result if process was started by PExecuteBackground
  
  log("Sending result to TTCN...");
  PIPEasp__Types::ASP__PResult message_PResult;
  PIPEasp__Types::ASP__PResultBinary message_PResultBinary;
  if (binaryMode) {
    message_PResultBinary.stdout_() =
     OCTETSTRING(stdout_buffer.get_read_len(), stdout_buffer.get_read_data());
    message_PResultBinary.stderr_() =
     OCTETSTRING(stderr_buffer.get_read_len(), stderr_buffer.get_read_data());
    message_PResultBinary.code() = processExitCode;
    incoming_message(message_PResultBinary);
  } else {
    int messageLen = stdout_buffer.get_read_len();
    const char* messageData = (const char*)stdout_buffer.get_read_data();
    
    if (lineMode && messageData[messageLen-1]=='\n') {
      messageLen--; // remove newline from the end
    }
    
    message_PResult.stdout_() = CHARSTRING(messageLen, messageData);

    messageLen = stderr_buffer.get_read_len();
    messageData = (const char*)stderr_buffer.get_read_data();
    
    if (lineMode && messageData[messageLen-1]=='\n') {
      messageLen--; // remove newline from the end
    }
    
    message_PResult.stderr_() = CHARSTRING(messageLen, messageData);
    message_PResult.code() = processExitCode;
    incoming_message(message_PResult);
  }

  // clearing the buffers
  stdout_buffer.clear();
  stderr_buffer.clear();
  //incoming_message(message);
}


////////////////
// Log function
////////////////
void PIPEasp__PT::log(const char *fmt, ...)
{ 
  TTCN_Logger::begin_event(TTCN_DEBUG);
  TTCN_Logger::log_event("PIPE test port (%s): ", get_name());
  va_list ap;
  va_start(ap, fmt);
  TTCN_Logger::log_event_va_list(fmt, ap);
  va_end(ap);
  TTCN_Logger::end_event();
}

}//namespace
