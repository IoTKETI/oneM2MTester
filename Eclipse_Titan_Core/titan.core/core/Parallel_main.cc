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
 *   Kovacs, Ferenc
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>

#include "../common/dbgnew.hh"
#include "../common/version_internal.h"
#include "Logger.hh"
#include "Snapshot.hh"
#include "Port.hh"
#include "Module_list.hh"
#include "Runtime.hh"
#include "Component.hh"
#include "Error.hh"
#include "Encdec.hh"
#include "TCov.hh"
#ifdef LINUX
#include <execinfo.h>
#endif  

#ifdef LICENSE
#include "../common/license.h"
#endif

#include <signal.h>

const char * stored_argv = "Unidentified program";

//static const char segfault[] = " : Segmentation fault occurred\n";

void signal_handler(int signum)
{
  time_t now=time(0);
  char ts[60];
  ts[0]='\0';
  struct tm *tmp;
  tmp=localtime(&now);
  if(tmp==NULL){
    fprintf(stderr,"<Unknown> %s: %s\n",stored_argv, signum==SIGABRT?"Abort was called":"Segmentation fault occurred");
  } else {
    fprintf(stderr,"%s %s: %s\n",ts,stored_argv,signum==SIGABRT?"Abort was called":"Segmentation fault occurred");
  }
  fflush(stderr);
#ifdef LINUX
  int nptrs;
  void *buffer[100];
  nptrs = backtrace(buffer, 100);
  backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
  
  fflush(stderr);
#endif  
  signal(SIGABRT, SIG_DFL);
  abort();
}

static void usage(const char* program_name)
{
  fprintf(stderr, "\n"
    "usage: %s [-s local_addr] MC_host MC_port\n"
    "   or  %s -l\n"
    "   or  %s -v\n"
    "\n"
    "OPTIONS:\n"
    "	-s local_addr:	use the given source IP address for control "
    "connections\n"
    "	-l:		list startable test cases and control parts\n"
    "	-v:		show version and module information\n",
    program_name, program_name, program_name);
}

/** Returns whether the caller should exit immediately */
static boolean process_options(int argc, char *argv[], int& ret_val,
    const char*& local_addr, const char*& MC_host, unsigned short& MC_port)
{
  boolean lflag = FALSE, sflag = FALSE, vflag = FALSE, errflag = FALSE;
  for ( ; ; ) {
    int c = getopt(argc, argv, "ls:v");
    if (c == -1) break;
    switch (c) {
    case 'l':
      if (lflag || sflag || vflag) errflag = TRUE;
      else lflag = TRUE;
      break;
    case 's':
      if (lflag || sflag || vflag) errflag = TRUE;
      else {
        local_addr = optarg;
        sflag = TRUE;
      }
      break;
    case 'v':
      if (lflag || sflag || vflag) errflag = TRUE;
      else vflag = TRUE;
      break;
    default:
      errflag = TRUE;
      break;
    }
  }

  if (lflag || vflag) {
    if (optind != argc) errflag = TRUE;
  } else {
    if (optind == argc - 2) {
      MC_host = argv[optind++];
      int port_num = atoi(argv[optind]);
      if (port_num > 0 && port_num < 65536) MC_port = port_num;
      else {
        fprintf(stderr, "Invalid MC port: %s\n", argv[optind]);
        errflag = TRUE;
      }
    } else errflag = TRUE;
  }

  if (errflag) {
    // syntax error in command line
    if (argc == 1) fputs("TTCN-3 Host Controller (parallel mode), version "
      PRODUCT_NUMBER "\n", stderr);
    usage(argv[0]);
    ret_val = EXIT_FAILURE;
    return TRUE;
  } else if (lflag) {
    // list of testcases
    try {
      // create buffer for error messages
      TTCN_Runtime::install_signal_handlers();
      TTCN_Logger::initialize_logger();
      Module_List::pre_init_modules();
      Module_List::list_testcases();
    } catch (...) {
      ret_val = EXIT_FAILURE;
    }
    TTCN_Logger::terminate_logger();
    return TRUE;
  } else if (vflag) {
    // version printout
    fputs("TTCN-3 Host Controller (parallel mode)\n"
      "Product number: " PRODUCT_NUMBER "\n"
      "Build date (Base Library): " __DATE__ " " __TIME__ "\n"
      "Base Library was compiled with: " C_COMPILER_VERSION "\n\n"
      COPYRIGHT_STRING "\n\n", stderr);
#ifdef LICENSE
    print_license_info();
    putc('\n', stderr);
#endif
    fputs("Module information:\n", stderr);
    Module_List::print_version();
    return TRUE;
  }
  // normal usage (HC operation)
  fputs("TTCN-3 Host Controller (parallel mode), version " PRODUCT_NUMBER
    "\n", stderr);
#ifdef LICENSE
  init_openssl();
  license_struct lstr;
  load_license(&lstr);
  if (!verify_license(&lstr)) {
    free_license(&lstr);
    free_openssl();
    exit(EXIT_FAILURE);
  }
  if (!check_feature(&lstr, FEATURE_HC)) {
    fputs("The license key does not allow the starting of TTCN-3 "
      "Host Controllers.\n", stderr);
    ret_val = EXIT_FAILURE;
  }
  free_license(&lstr);
  free_openssl();
  if (ret_val != EXIT_SUCCESS) return TRUE;
#endif
  return FALSE;
}

int main(int argc, char *argv[])
{
  stored_argv = argv[0];
  struct sigaction act;
  act.sa_handler = signal_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGSEGV, &act, 0);
  sigaction(SIGABRT, &act, 0);

#ifdef MEMORY_DEBUG
  debug_new_counter.set_program_name(argv[0]);
#endif
  int ret_val = EXIT_SUCCESS;
  const char *local_addr = NULL, *MC_host = NULL;
  unsigned short MC_port = 0;
  errno = 0;

    if (process_options(argc, argv, ret_val, local_addr, MC_host, MC_port)) {
      TCov::close_file();
      return ret_val;
    }

  try {
    TTCN_Runtime::install_signal_handlers();
    TTCN_Snapshot::initialize();
    TTCN_Logger::initialize_logger();
    TTCN_Logger::set_executable_name(argv[0]);
    TTCN_Logger::set_start_time();

    // the log file will be opened immediately after processing
    // configuration data received from the MC

    try {
      Module_List::pre_init_modules();
      ret_val = TTCN_Runtime::hc_main(local_addr, MC_host, MC_port);
      if (!TTCN_Runtime::is_hc()) {
        // this code runs on the child processes (MTC and PTCs)
        // forget about the component names inherited from the HC
        COMPONENT::clear_component_names();
        // the log file is inherited from the parent process
        // it has to be closed first
        TTCN_Logger::close_file();
        TCov::close_file();
        // the baseline of relative timestamps has to be reset
        TTCN_Logger::set_start_time();

        if (TTCN_Runtime::is_mtc()) ret_val = TTCN_Runtime::mtc_main();
        else if (TTCN_Runtime::is_ptc())
          ret_val = TTCN_Runtime::ptc_main();
        else TTCN_error("Internal error: Invalid executor state after "
          "finishing HC activities.");
      }
    } catch (const TC_Error& TC_error) {
      ret_val = EXIT_FAILURE;
    }
  } catch (...) {
    TTCN_Logger::log_str(TTCN_Logger::ERROR_UNQUALIFIED,
      "Fatal error. Aborting execution.");
    ret_val = EXIT_FAILURE;
  }
  // the final cleanup tasks are common for all processes
  TTCN_Runtime::restore_signal_handlers();
  TTCN_Logger::close_file();
  TCov::close_file();
  // close_file() must be called before the information is lost
  // close_file() WRITES to log
  TTCN_Logger::clear_parameters();
  PORT::clear_parameters();
  COMPONENT::clear_component_names();
  TTCN_EncDec::clear_error();

  TTCN_Logger::terminate_logger();
  TTCN_Snapshot::terminate();

  return ret_val;
}
