/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Forstner, Matyas
 *   Herrlin, Thomas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include "Error.hh"
#include "TitanLoggerApi.hh"

#include <stdarg.h>
#include <stdint.h>

#include "../common/memory.h"
#include "Logger.hh"
#include "Runtime.hh"
#include "Charstring.hh"

#ifdef LINUX
#include <ucontext.h>
#include <dlfcn.h>
#include <execinfo.h>

#ifndef NO_CPP_DEMANGLE
#include <cxxabi.h>
using namespace __cxxabiv1;
#endif

#endif

#ifdef LINUX

const size_t MAX_DEPTH=100;

void stacktrace(const ucontext_t& ctx)
{
  TTCN_Logger::log_event_str("\nStack trace:\n");

  (void)ctx;
  void *array[MAX_DEPTH];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, MAX_DEPTH);
  strings = backtrace_symbols (array, size);

  //printf ("Obtained %lu stack frames.\n", (unsigned long)size);

  for (i = 0; i < size; i++) {
    const char * symname = strings[i];

    const char *begin = 0, *end = 0;
    // The format is: /path/to/exe(_Z11mangledname+0xBAAD) [0x...]
    // Mangled name starts here ---^   ends here--^
    for (const char *j = symname; *j; ++j) {
      if (*j == '(')
        begin = j+1;
      else if (*j == '+')
        end = j;
    }

    char *dem = 0;
    if (begin && end) {
      size_t len = end - begin;
      char *mangled = (char*)malloc(len + 1);
      memcpy(mangled, begin, len);
      mangled[len] = '\0';

      int status;
      dem = __cxa_demangle(mangled, NULL, 0, &status);

      if(status == 0 && dem)
        symname = dem;
    }

    if (TTCN_Logger::is_logger_up()) {
      TTCN_Logger::log_event ("%2lu: %s%s\n", (unsigned long)i, symname, (end ? end : ""));
    }
    else {
      fprintf (stderr, "%2lu: %s%s\n", (unsigned long)i, symname, (end ? end : ""));
    }

    if (dem)
      free(dem);
  }

  free (strings);
}

void where_am_i(TTCN_Logger::Severity sev = TTCN_Logger::USER_UNQUALIFIED)
{
  ucontext_t uc;
  int er = getcontext(&uc);
  if (!er) {
    TTCN_Logger::begin_event(sev);
    stacktrace(uc);
    TTCN_Logger::end_event();
  }
  else {
    perror("getcontext");
  }
}


static void signal_segv(int signum, siginfo_t* info, void*ptr) {
  static const char *si_codes[3] = {"", "SEGV_MAPERR", "SEGV_ACCERR"};
  ucontext_t *ucontext = (ucontext_t*)ptr;
  fprintf(stderr, "\n\n!!! Segmentation Fault !!!\n\n");
  fprintf(stderr, "info.si_signo = %d\n", signum);
  fprintf(stderr, "info.si_errno = %d\n", info->si_errno);
  fprintf(stderr, "info.si_code  = %d (%s)\n", info->si_code, si_codes[info->si_code]);
  fprintf(stderr, "info.si_addr  = %p\n", info->si_addr);

  TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
  stacktrace(*ucontext);
  TTCN_Logger::end_event();

  fprintf(stderr, "\nGoodbye, cruel world!\n");
  exit(-1);
}

int setup_sigsegv() {
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = signal_segv;
  action.sa_flags = SA_SIGINFO;
  if(sigaction(SIGSEGV, &action, NULL) < 0) {
    perror("sigaction");
    return 0;
  }
  return 1;
}

static void __attribute((constructor)) init(void) {
  setup_sigsegv();
}

#elif defined(SOLARIS) || defined(SOLARIS8)

/*
  walks up call stack, printing library:routine+offset for each routine
  */

#include <dlfcn.h>
#include <setjmp.h>
#include<sys/types.h>
#include <sys/reg.h>
#include <sys/frame.h>

#if defined(sparc) || defined(__sparc)
#define FLUSHWIN() asm("ta 3");
#define FRAME_PTR_INDEX 1
#define SKIP_FRAMES 0
#endif

#if defined(i386) || defined(__i386)
#define FLUSHWIN()
#define FRAME_PTR_INDEX 3
#define SKIP_FRAMES 1
#endif

// TODO: Values for __x86_64 are only guesses, not tested.
#if defined(__x86_64) 
#define FLUSHWIN()
#define FRAME_PTR_INDEX 7
#define SKIP_FRAMES 1
#endif

#if defined(ppc) || defined(__ppc)
#define FLUSHWIN()
#define FRAME_PTR_INDEX 0
#define SKIP_FRAMES 2
#endif

#include <cxxabi.h>


/*
 this function walks up call stack, calling user-supplied
  function once for each stack frame, passing the pc and the user-supplied
  usrarg as the argument.
  */

int cs_operate(int (*func)(void *, void *), void * usrarg)
{
  struct frame *sp;
  jmp_buf env;
  int i;
  int * iptr;

  FLUSHWIN();

  setjmp(env);
  iptr = (int*) env;

  sp = (struct frame *) iptr[FRAME_PTR_INDEX];

  for(i=0;i<SKIP_FRAMES && sp;i++)
    sp = (struct frame*) sp->fr_savfp;

  i = 0;

  while(sp && sp->fr_savpc && ++i && (*func)((void*)sp->fr_savpc, usrarg)) {
    sp = (struct frame*) sp->fr_savfp;
  }

  return(i);
}

void print_my_stack()
{
  int print_address(void *, void *);
  cs_operate(print_address, NULL);
}

int print_address(void *pc, void * usrarg)
{
  Dl_info info;
  const char * func;
  const char * lib;

  if(dladdr(pc, & info) == 0) {
    func = "??";
    lib = "??";
  }
  else {
    lib =  info.dli_fname;
    func = info.dli_sname;
  }

  size_t len = strlen(lib);
  for (--len; len > 0; --len) {
    if (lib[len] == '/') break;
  }
  if (len > 0) lib += ++len;

  int status = 42;
  char *demangled = abi::__cxa_demangle(func, NULL, 0, &status);
  if (status == 0) func = demangled;

  if (TTCN_Logger::is_logger_up()) {
    TTCN_Logger::log_event("%s:%s+%p\n",
      lib,
      func,
      (void *)((uintptr_t)pc - (uintptr_t)info.dli_saddr)
      );
  }
  else {
    fprintf(stderr, "%s:%s+%p\n",
      lib,
      func,
      (void *)((uintptr_t)pc - (uintptr_t)info.dli_saddr)
      );
  }

  if (status == 0) free(demangled);
  return(1);
}

void where_am_i(TTCN_Logger::Severity sev = TTCN_Logger::USER_UNQUALIFIED)
{
  TTCN_Logger::begin_event(sev);
  print_my_stack();
  TTCN_Logger::end_event();
}

#else

void where_am_i(TTCN_Logger::Severity /*sev*/ = TTCN_Logger::USER_UNQUALIFIED)
{
    // You are on your own
}
#endif // LINUX


TTCN_Error::~TTCN_Error()
{
  Free(error_msg);
}


void TTCN_error(const char *err_msg, ...)
{
  if (TTCN_Runtime::is_in_ttcn_try_block()) {
    // Add location data as if it were logged
    char* error_str = TTCN_Location::print_location(
      TTCN_Logger::SINFO_STACK == TTCN_Logger::get_source_info_format(), 
      TTCN_Logger::SINFO_NONE != TTCN_Logger::get_source_info_format(), 
      TTCN_Logger::get_log_entity_name());
    if (error_str) {
      error_str = mputstr(error_str, " ");
    }
    error_str = mputstr(error_str, "Dynamic test case error: ");
    va_list p_var;
    va_start(p_var, err_msg);
    error_str = mputprintf_va_list(error_str, err_msg, p_var);
    va_end(p_var);
    throw TTCN_Error(error_str);
  } else {
    TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
    if (TTCN_Logger::SINFO_NONE == TTCN_Logger::get_source_info_format())
    {
      //Always print some location info in case of dynamic testcase error
      char * loc = TTCN_Location::print_location(FALSE, TRUE, FALSE);
      if (loc) {
        TTCN_Logger::log_event_str(loc);
        TTCN_Logger::log_event_str(": ");
        Free(loc);
      }
    }
    TTCN_Logger::log_event_str("Dynamic test case error: ");
    va_list p_var;
    va_start(p_var, err_msg);
    TTCN_Logger::log_event_va_list(err_msg, p_var);
    va_end(p_var);
    TTCN_Logger::OS_error();
    TTCN_Logger::end_event();
#ifndef NDEBUG
// the current implementation of the stack trace printout is misleading and cause more
// trouble and misunderstanding than it solves
// So it disabled in production compilation
// It remains active in debug compilation
    where_am_i(TTCN_Logger::ERROR_UNQUALIFIED);
#endif
    TTCN_Runtime::set_error_verdict();
    TTCN_Logger::log_executor_runtime(
      TitanLoggerApi::ExecutorRuntime_reason::performing__error__recovery);
    throw TC_Error();
  }
}

void TTCN_error_begin(const char *err_msg, ...)
{
  if (TTCN_Runtime::is_in_ttcn_try_block()) {
    TTCN_Logger::begin_event_log2str();
    // Add location data as if it were logged
    char* loc = TTCN_Location::print_location(
      TTCN_Logger::SINFO_STACK == TTCN_Logger::get_source_info_format(), 
      TTCN_Logger::SINFO_NONE != TTCN_Logger::get_source_info_format(), 
      TTCN_Logger::get_log_entity_name());
    if (loc) {
      TTCN_Logger::log_event_str(loc);
      TTCN_Logger::log_event_str(" ");
      Free(loc);
    }
    TTCN_Logger::log_event_str("Dynamic test case error: ");
    va_list p_var;
    va_start(p_var, err_msg);
    TTCN_Logger::log_event_va_list(err_msg, p_var);
    va_end(p_var);
  } else {
    TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
    TTCN_Logger::log_event_str("Dynamic test case error: ");
    va_list p_var;
    va_start(p_var, err_msg);
    TTCN_Logger::log_event_va_list(err_msg, p_var);
    va_end(p_var);
  }
}

void TTCN_error_end()
{
  if (TTCN_Runtime::is_in_ttcn_try_block()) {
    CHARSTRING error_str = TTCN_Logger::end_event_log2str();
    throw TTCN_Error(mcopystr((const char*)error_str));
  } else {
    TTCN_Logger::OS_error();
    TTCN_Logger::end_event();
    TTCN_Runtime::set_error_verdict();
    TTCN_Logger::log_executor_runtime(
      TitanLoggerApi::ExecutorRuntime_reason::performing__error__recovery);
    throw TC_Error();
  }
}

void TTCN_warning(const char *warning_msg, ...)
{
  TTCN_Logger::begin_event(TTCN_Logger::WARNING_UNQUALIFIED);
  TTCN_Logger::log_event_str("Warning: ");
  va_list p_var;
  va_start(p_var, warning_msg);
  TTCN_Logger::log_event_va_list(warning_msg, p_var);
  va_end(p_var);
  TTCN_Logger::end_event();
}

void TTCN_warning_begin(const char *warning_msg, ...)
{
  TTCN_Logger::begin_event(TTCN_Logger::WARNING_UNQUALIFIED);
  TTCN_Logger::log_event_str("Warning: ");
  va_list p_var;
  va_start(p_var, warning_msg);
  TTCN_Logger::log_event_va_list(warning_msg, p_var);
  va_end(p_var);
}

void TTCN_warning_end()
{
  TTCN_Logger::end_event();
}

void TTCN_pattern_error(const char *error_msg, ...)
{
  va_list p_var;
  va_start(p_var, error_msg);
  char *error_str = mprintf_va_list(error_msg, p_var);
  va_end(p_var);
  try {
    TTCN_error("Charstring pattern: %s", error_str);
  } catch (...) {
    Free(error_str);
    throw;
  }
}

void TTCN_pattern_warning(const char *warning_msg, ...)
{
  va_list p_var;
  va_start(p_var, warning_msg);
  char *warning_str = mprintf_va_list(warning_msg, p_var);
  va_end(p_var);
  TTCN_warning("Charstring pattern: %s", warning_str);
  Free(warning_str);
}
