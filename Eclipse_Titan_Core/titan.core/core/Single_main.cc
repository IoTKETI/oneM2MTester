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
 *   Beres, Szabolcs
 *   Delic, Adam
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "../common/dbgnew.hh"
#include "../common/version_internal.h"
#include "Logger.hh"
#include "Snapshot.hh"
#include "Port.hh"
#include "Module_list.hh"
#include "Runtime.hh"
#include "Component.hh"
#include "Encdec.hh"
#include "TitanLoggerApi.hh"
#include "TCov.hh"
#ifdef LINUX
#include <execinfo.h>
#endif  

#ifdef LICENSE
#include "../common/license.h"
#endif

#include <signal.h>

const char * stored_argv = "Unidentified program";

static const char segfault[] = ": Segmentation fault occurred\n";
static const char abortcall[] = ": Abort was called\n";

void signal_handler(int signum)
{
  int retval;
  retval = write(STDERR_FILENO, stored_argv, strlen(stored_argv));
  if(signum==SIGSEGV){
  retval = write(STDERR_FILENO, segfault , sizeof(segfault)-1); // sizeof includes \0
  } else {
  retval = write(STDERR_FILENO, abortcall , sizeof(abortcall)-1); // sizeof includes \0
  }
#ifdef LINUX
  int nptrs;
  void *buffer[100];
  nptrs = backtrace(buffer, 100);
  backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
  
  fflush(stderr);
#endif
  (void)retval;
  TTCN_Logger::close_file();

  signal(SIGABRT, SIG_DFL);
  abort();
}

static void usage(const char* program_name)
{
  fprintf(stderr, "\n"
    "usage: %s [-h] [-b file] configuration_file\n"
    "   or  %s -l\n"
    "   or  %s -v\n"
    "\n"
    "OPTIONS:\n"
    "	-b file:	run specified batch file at start (debugger must be activated)\n"
    "	-h:		automatically halt execution at start (debugger must be activated)\n"
    "	-l:		list startable test cases and control parts\n"
    "	-v:		show version and module information\n",
    program_name, program_name, program_name);
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
  errno = 0;
  int c, i, ret_val = EXIT_SUCCESS;
  boolean bflag = FALSE, hflag = FALSE, lflag = FALSE, vflag = FALSE, errflag = FALSE;
  const char *config_file = NULL;
  TTCN_Module *only_runnable = Module_List::single_control_part();

  while ((c = getopt(argc, argv, "b:hlv")) != -1) {
    switch (c) {
    case 'b':
      if (bflag || lflag || vflag) errflag = TRUE; // duplicate or conflicting
      else {
        bflag = TRUE;
        ttcn3_debugger.set_initial_batch_file(optarg);
      }
      break;
    case 'h':
      if (hflag || lflag || vflag) errflag = TRUE; // duplicate or conflicting
      else {
        hflag = TRUE;
        ttcn3_debugger.set_halt_at_start();
      }
      break;
    case 'l':
      if (lflag || vflag) errflag = TRUE; // duplicate or conflicting
      else lflag = TRUE;
      break;
    case 'v':
      if (lflag || vflag) errflag = TRUE; // duplicate or conflicting
      else vflag = TRUE;
      break;
    default:
      errflag = TRUE;
    }
  }

  if (!errflag) {
    if (lflag || vflag) {
      if (optind != argc) errflag = TRUE; // -l or -v and non-option arg
    } else {
      if (optind > argc - 1) { // no config file argument
        errflag = (only_runnable == 0);
      }
      else config_file = argv[optind];
    }
  }

  if (errflag) {
    if (argc == 1) fputs("TTCN-3 Test Executor (single mode), version "
      PRODUCT_NUMBER "\n", stderr);
    usage(argv[0]);
    TCov::close_file();
    return EXIT_FAILURE;
  } else if (lflag) {
    try {
      // create buffer for error messages
      TTCN_Logger::initialize_logger();
      Module_List::pre_init_modules();
      Module_List::list_testcases();
    } catch (...) {
      ret_val = EXIT_FAILURE;
    }
    TTCN_Logger::terminate_logger();
    TCov::close_file();
    return ret_val;
  } else if (vflag) {
    fputs("TTCN-3 Test Executor (single mode)\n"
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
    TCov::close_file();
    return EXIT_SUCCESS;
  }

  fputs("TTCN-3 Test Executor (single mode), version " PRODUCT_NUMBER "\n",
    stderr);

#ifdef LICENSE
  init_openssl();
  license_struct lstr;
  load_license(&lstr);
  if (!verify_license(&lstr)) {
    free_license(&lstr);
    free_openssl();
    exit(EXIT_FAILURE);
  }
  if (!check_feature(&lstr, FEATURE_SINGLE)) {
    fputs("The license key does not allow test execution in single mode.\n",
      stderr);
    TCov::close_file();
    return EXIT_FAILURE;
  }
  free_license(&lstr);
  free_openssl();
#endif

  self = MTC_COMPREF;
  TTCN_Runtime::set_state(TTCN_Runtime::SINGLE_CONTROLPART);
  TTCN_Runtime::install_signal_handlers();
  TTCN_Snapshot::initialize();
  TTCN_Logger::initialize_logger();
  TTCN_Logger::set_executable_name(argv[0]);
  TTCN_Logger::set_start_time();

  try {
    TTCN_Logger::log_executor_runtime(
      TitanLoggerApi::ExecutorRuntime_reason::executor__start__single__mode);
    Module_List::pre_init_modules();

    if (config_file != 0) {
      fprintf(stderr, "Using configuration file: `%s'\n", config_file);
      TTCN_Logger::log_configdata(
        TitanLoggerApi::ExecutorConfigdata_reason::using__config__file, config_file);
    }

    TTCN_Snapshot::check_fd_setsize();

    boolean config_file_failure =
      config_file && !process_config_file(config_file);
    TTCN_Runtime::load_logger_plugins();
    // Quick return if no config file.
    TTCN_Runtime::set_logger_parameters();
    TTCN_Logger::open_file();
    TTCN_Logger::write_logger_settings();
    if (config_file_failure) goto fail;
    // Config file parsed or no config file: we may be able to run.
    if (config_file) {
      if (++optind != argc) {
        // There are further commandline arguments after config file.
        // Override testcase list.
        // First, throw away the old list parsed from the config file.
        for (i = 0; i < execute_list_len; i++) {
          Free(execute_list[i].module_name);
          Free(execute_list[i].testcase_name);
        }

        // The new execute list length is known in advance.
        execute_list_len = argc - optind;
        execute_list = (execute_list_item *)Realloc(
          execute_list, execute_list_len * sizeof(*execute_list));

        expstring_t testcase_names = memptystr(); // collects names for printout

        for (i = optind; i < argc; ++i) {
          testcase_names = mputstr(testcase_names, argv[i]);
          testcase_names = mputc(testcase_names, '\t');

          char *dot = strchr(argv[i], '.');
          if (dot != 0) {
            *dot++ = '\0'; // cut the string into two
            if (!strcmp(dot, "control"))
              dot = 0;
          }
          execute_list[i-optind].module_name = mcopystr(argv[i]);
          execute_list[i-optind].testcase_name = dot ? mcopystr(dot) : dot;
          // do not copy NULL pointer, it results in non-0 empty string
        } // next i
        fprintf(stderr, "Overriding testcase list: %s\n", testcase_names);
        TTCN_Logger::log_configdata(
          TitanLoggerApi::ExecutorConfigdata_reason::overriding__testcase__list,
          testcase_names);
        Free(testcase_names);
      }
    }

    if (execute_list_len == 0 && only_runnable) {
      // No config file or correct config file without EXECUTE section,
      // AND precisely one control part: run that one.
      execute_list_len = 1;
      execute_list = (execute_list_item *)Malloc(sizeof(*execute_list));
      execute_list[0].module_name = mcopystr(only_runnable->get_name());
      execute_list[0].testcase_name = 0; // control part
    }

    if (execute_list_len > 0) { // we have something to run
      Module_List::log_param();
      Module_List::post_init_modules();

      for (i = 0; i < execute_list_len; i++) {
        if (ttcn3_debugger.is_exiting()) {
          break;
        }
        if (execute_list[i].testcase_name == NULL)
          Module_List::execute_control(execute_list[i].module_name);
        else if (!strcmp(execute_list[i].testcase_name, "*"))
          Module_List::execute_all_testcases(
            execute_list[i].module_name);
        else
          Module_List::execute_testcase(execute_list[i].module_name,
            execute_list[i].testcase_name);
      }
    } else {
      TTCN_warning("Nothing to run!");
fail:
      ret_val = EXIT_FAILURE;
    }
  } catch (...) {
    TTCN_Logger::log_str(TTCN_Logger::ERROR_UNQUALIFIED,
      "Fatal error. Aborting execution.");
    ret_val = EXIT_FAILURE;
  }
  TTCN_Runtime::restore_signal_handlers();
  TTCN_Runtime::log_verdict_statistics();
  TTCN_Logger::log_executor_runtime(
    TitanLoggerApi::ExecutorRuntime_reason::executor__finish__single__mode);
  TTCN_Logger::close_file();
  TCov::close_file();
  // close_file() must be called before the information is lost
  // close_file() WRITES to log

  TTCN_Logger::clear_parameters();
  PORT::clear_parameters();
  COMPONENT::clear_component_names();
  TTCN_EncDec::clear_error();

  for (i = 0; i < execute_list_len; i++) {
    Free(execute_list[i].module_name);
    Free(execute_list[i].testcase_name);
  }
  Free(execute_list);

  TTCN_Logger::terminate_logger();
  TTCN_Snapshot::terminate();
  TTCN_Runtime::clean_up();

  return ret_val;
}
