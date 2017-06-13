/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Bene, Tamas
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Vilmos Varga - author
 *
 ******************************************************************************/
//
// Description:           Header file for Cli
//
#ifndef CLI_CLI_H
#define CLI_CLI_H
//----------------------------------------------------------------------------

#include <stdio.h>
#include <pthread.h>

#include "../../core/Types.h"
#include "../mctr/UserInterface.h"
#include "../mctr/MainController.h"
#include "../mctr/config_data.h"

//----------------------------------------------------------------------------

namespace cli {

//----------------------------------------------------------------------------

/**
 * User interface cli implementation.
 */
class Cli : public mctr::UserInterface
{
public:
  /**
   * Constructor, destructor.
   */
  Cli();
  ~Cli();

  /**
   * Enters the main loop.
   */
  virtual int enterLoop(int argc, char* argv[]);

  /**
   * Status of MC has changed.
   */
  virtual void status_change();

  /**
   * Error message from MC.
   */
  virtual void error(int severity, const char *message);

  /*
   * Indicates whether console logging is enabled.
   */
  boolean loggingEnabled;
  /**
   * General notification from MC.
   */
  virtual void notify(const struct timeval *timestamp, const char *source,
    int severity, const char *message);

  /*
   * Callback functions for command processing.
   * (All callback functions MUST be public!!!)
   */
  void cmtcCallback(const char *arguments);
  void smtcCallback(const char *arguments);
  void stopCallback(const char *arguments);
  void pauseCallback(const char *arguments);
  void continueCallback(const char *arguments);
  void emtcCallback(const char *arguments);
  void logCallback(const char *arguments);
  void infoCallback(const char *arguments);
  void reconfCallback(const char *arguments);
  void helpCallback(const char *arguments);
  void shellCallback(const char *arguments);
  void exitCallback(const char *arguments);
  
  virtual void executeBatchFile(const char* filename);

private:
  /**
   * Deallocates the memory used by global variables
   */
  static void cleanUp();

  /**
   * Print the welcome text.
   */
  static void printWelcome();
  /**
   * Print program usage information.
   */
  static void printUsage(const char *prg_name);

  /**
   * The main cli event loop.
   */
  int interactiveMode();

  /**
   * Execution in batch mode.
   */
  int batchMode();

  /**
   * Process the command to perform the action accordingly.
   */
  void processCommand(char *);

  /*
   * Readline command completion function for MC commands.
   */
  static char *completeCommand(const char *, int);

  /*
   * Customized character input function for readline.
   */
  static int getcWithShellDetection(FILE *);

  /**
   * Remove surrounding LWS, move string content to the beginning of
   * the buffer and zero the rest of the buffer.
   */
  void stripLWS(char *);

  /*
   * Flag for indicating program termination.
   */
  boolean exitFlag;

  /*
   * Transforms internal MC state codes to printable strings.
   */
  static const char *verdict2str(verdicttype verdict);

  /*
   * Prints information about the current state of MC.
   */
  void printInfo();

  /*
   * Mutex and condition variable for synchronization between threads.
   */
  pthread_mutex_t mutex;
  pthread_cond_t cond;

  /*
   * Primitives for controlling the mutex and the condition variable.
   */
  void lock();
  void unlock();
  void wait();
  void signal();

  /*
   * Functions for waiting until MC reaches the desired state.
   */
  enum waitStateEnum {
    WAIT_NOTHING, WAIT_HC_CONNECTED,
    WAIT_ACTIVE, WAIT_MTC_CREATED, WAIT_MTC_READY,
    WAIT_MTC_TERMINATED, WAIT_SHUTDOWN_COMPLETE,
    WAIT_EXECUTE_LIST
  } waitState;
  boolean conditionHolds(waitStateEnum askedState);
  void waitMCState(waitStateEnum newWaitState);

  int getHostIndex(const char*);

  /*
   * Executes the index-th element of the execute list
   */
  void executeFromList(int index);
  /*
   * Shows which item of the list is currently being executed
   */
  int executeListIndex;

  char* cfg_file_name;
  config_data mycfg;
};

// pointer to member methods
typedef void (Cli::*callback_t)(const char *text);

//----------------------------------------------------------------------------

} /* namespace cli */

//----------------------------------------------------------------------------
#endif // CLI_CLI_H

// Local Variables:
// mode: C++
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
