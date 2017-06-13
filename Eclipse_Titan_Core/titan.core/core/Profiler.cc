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
 *
 ******************************************************************************/

#include "Profiler.hh"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>
#include "memory.h"
#include "Runtime.hh"
#include "Logger.hh"

////////////////////////////////////
////////// TTCN3_Profiler //////////
////////////////////////////////////

TTCN3_Profiler ttcn3_prof;

TTCN3_Profiler::TTCN3_Profiler()
: stopped(FALSE), disable_profiler(FALSE), disable_coverage(FALSE)
, aggregate_data(FALSE), disable_stats(FALSE), stats_flags(Profiler_Tools::STATS_ALL)
{
  database_filename = mcopystr("profiler.db");
  stats_filename = mcopystr("profiler.stats");
  reset();
}

TTCN3_Profiler::~TTCN3_Profiler()
{
  if (!profiler_db.empty() && !TTCN_Runtime::is_undefined() &&
      (!disable_profiler || !disable_coverage)) {
    if (aggregate_data && (TTCN_Runtime::is_single() || TTCN_Runtime::is_hc())) {
      // import the data from the previous run
      import_data();
    }
    if (TTCN_Runtime::is_hc()) {
      // import the data gathered by the other processes (the import function
      // waits for them to finish exporting)
      for (size_t i = 0; i < pid_list.size(); ++i) {
        import_data(pid_list[i]);
      }
    }
    export_data();
    if (!disable_stats && (TTCN_Runtime::is_single() || TTCN_Runtime::is_hc())) {
      print_stats();
    }
  }
  for (size_t i = 0; i < profiler_db.size(); ++i) {
    Free(profiler_db[i].filename);
    for (size_t j = 0; j < profiler_db[i].functions.size(); ++j) {
      Free(profiler_db[i].functions[j].name);
    }
  }
  Free(database_filename);
  Free(stats_filename);
}

void TTCN3_Profiler::start()
{
  if (stopped) {
    set_prev(disable_profiler ? -1 : TTCN3_Stack_Depth::depth(), NULL, -1);
    stopped = FALSE;
  }
}

void TTCN3_Profiler::stop()
{
  if (!stopped) {
    if (NULL != prev_file) {
      // update the previous line's time
      timeval elapsed = Profiler_Tools::subtract_timeval(get_time(), prev_time);
      add_line_time(elapsed, get_element(prev_file), prev_line);
      TTCN3_Stack_Depth::update_stack_elapsed(elapsed);
    }
    stopped = TRUE;
  }
}

void TTCN3_Profiler::set_disable_profiler(boolean p_disable_profiler)
{
  disable_profiler = p_disable_profiler;
}

void TTCN3_Profiler::set_disable_coverage(boolean p_disable_coverage)
{
  disable_coverage = p_disable_coverage;
}

// handles metacharacters in the database or statistics file name
static char* finalize_filename(const char* p_filename_skeleton)
{
  size_t len = strlen(p_filename_skeleton);
  size_t next_idx = 0;
  char* ret_val = NULL;
  for (size_t i = 0; i < len - 1; ++i) {
    if ('%' == p_filename_skeleton[i]) {
      ret_val = mputstrn(ret_val, p_filename_skeleton + next_idx, i - next_idx);
      switch (p_filename_skeleton[i + 1]) {
      case 'e': // %e -> executable name
        ret_val = mputstr(ret_val, TTCN_Logger::get_executable_name());
        break;
      case 'h': // %h -> host name
        ret_val = mputstr(ret_val, TTCN_Runtime::get_host_name());
        break;
      case 'p': // %p -> process ID
        ret_val = mputprintf(ret_val, "%ld", (long)getpid());
        break;
      case 'l': { // %l -> login name
        setpwent();
        struct passwd *p = getpwuid(getuid());
        if (NULL != p) {
          ret_val = mputstr(ret_val, p->pw_name);
        }
        endpwent();
        break; }
      case '%': // %% -> single %
        ret_val = mputc(ret_val, '%');
        break;
      default: // unknown sequence -> leave it as it is 
        ret_val = mputstrn(ret_val, p_filename_skeleton + i, 2);
        break;
      }
      next_idx = i + 2;
      ++i;
    }
  }
  if (next_idx < len) {
    ret_val = mputstr(ret_val, p_filename_skeleton + next_idx);
  }
  return ret_val;
}

void TTCN3_Profiler::set_database_filename(const char* p_database_filename)
{
  Free(database_filename);
  database_filename = finalize_filename(p_database_filename);
}

void TTCN3_Profiler::set_aggregate_data(boolean p_aggregate_data)
{
  aggregate_data = p_aggregate_data;
}

void TTCN3_Profiler::set_stats_filename(const char* p_stats_filename)
{
  Free(stats_filename);
  stats_filename = finalize_filename(p_stats_filename);
}

void TTCN3_Profiler::set_disable_stats(boolean p_disable_stats)
{
  disable_stats = p_disable_stats;
}

void TTCN3_Profiler::reset_stats_flags()
{
  stats_flags = 0;
}

void TTCN3_Profiler::add_stats_flags(unsigned int p_flags)
{
  stats_flags |= p_flags;
}

boolean TTCN3_Profiler::is_profiler_disabled() const
{
  return disable_profiler;
}

boolean TTCN3_Profiler::is_running() const
{
  return !stopped;
}

void TTCN3_Profiler::add_child_process(pid_t p_pid)
{
  pid_list.push_back(p_pid);
}

void TTCN3_Profiler::import_data(pid_t p_pid /* = 0 */)
{
  char* file_name = NULL;
  if (0 == p_pid) {
    // this is the main database file (from the previous run), no suffix needed
    file_name = database_filename;
  }
  else {
    // this is the database for one of the PTCs or the MTC,
    // suffix the file name with the component's PID
    file_name = mprintf("%s.%d", database_filename, p_pid);
    
    // wait for the process to finish
    int status = 0;
    waitpid(p_pid, &status, 0);
  }
  
  Profiler_Tools::import_data(profiler_db, file_name, TTCN_warning);
  
  if (0 != p_pid) {
    // the process-specific database file is no longer needed
    remove(file_name);
    Free(file_name);
  }
}

void TTCN3_Profiler::export_data()
{
  char* file_name = NULL;
  if (TTCN_Runtime::is_single() || TTCN_Runtime::is_hc()) {
    // this is the main database file, no suffix needed
    file_name = database_filename;
  }
  else {
    // this is the database for one of the PTCs or the MTC,
    // suffix the file name with the component's PID
    file_name = mprintf("%s.%d", database_filename, (int)getpid());
  }
  
  Profiler_Tools::export_data(profiler_db, file_name, disable_profiler,
    disable_coverage, TTCN_warning);
  
  if (file_name != database_filename) {
    Free(file_name);
  }
}

void TTCN3_Profiler::print_stats() 
{
  if (profiler_db.empty()) {
    return;
  }
  
  Profiler_Tools::print_stats(profiler_db, stats_filename, disable_profiler,
    disable_coverage, stats_flags, TTCN_warning);
}

void TTCN3_Profiler::reset()
{
  prev_time.tv_sec = 0;
  prev_time.tv_usec = 0;
  prev_file = NULL;
  prev_line = -1;
  prev_stack_len = -1;
}

timeval TTCN3_Profiler::get_time() 
{
  timeval tv;
  gettimeofday(&tv, NULL);
  return tv;
}

void TTCN3_Profiler::enter_function(const char* filename, int lineno)
{
  if (disable_profiler && disable_coverage) {
    return;
  }
  
  // Note that the execution time of the last line in a function
  // is measured by using the stack depth.
  execute_line(filename, lineno);
  
  if (!stopped) {
    // store function data
    if (!disable_coverage) {
      unsigned int element = get_element(filename);
      ++profiler_db[element].functions[get_function(element, lineno)].exec_count;
    }
  }
}

void TTCN3_Profiler::execute_line(const char* filename, int lineno)
{
  if (disable_profiler && disable_coverage) {
    return;
  }
  
  if (!disable_profiler && TTCN3_Stack_Depth::depth() > prev_stack_len) {
    // this line is in a different function than the last one, don't measure anything
    TTCN3_Stack_Depth::add_stack(prev_stack_len, prev_file, filename, prev_line, lineno);
  }
  
  if (!stopped) {
    if (!disable_profiler && NULL != prev_file) {
      // this line is in the same function as the previous one, measure the time difference
      timeval elapsed = Profiler_Tools::subtract_timeval(get_time(), prev_time);

      // add the elapsed time to the total time of the previous line
      add_line_time(elapsed, get_element(prev_file), prev_line);

      TTCN3_Stack_Depth::update_stack_elapsed(elapsed);
    }

    // functions starting at line 0 are: pre_init_module and post_init_module,
    // don't include them in the database (as they don't appear in the TTCN-3 code),
    // but include any actual code lines they may contain
    // also, several instructions could be in the same line, only count the line once
    if (0 != lineno && !disable_coverage && (lineno != prev_line || NULL == prev_file || 
                                             0 != strcmp(prev_file, filename))) {
      unsigned int element = get_element(filename);

      // increase line execution count
      ++profiler_db[element].lines[get_line(element, lineno)].exec_count;
    }
  }
  
  // store the current location as previous for the next call
  set_prev(disable_profiler ? -1 : TTCN3_Stack_Depth::depth(), filename, lineno);
}

unsigned int TTCN3_Profiler::get_element(const char* filename) 
{
  for (size_t i = 0; i < profiler_db.size(); ++i) {
    if (0 == strcmp(profiler_db[i].filename, filename)) {
      return i;
    }
  }
  
  Profiler_Tools::profiler_db_item_t item;
  item.filename = mcopystr(filename);
  profiler_db.push_back(item);
  return profiler_db.size() - 1;
}

int TTCN3_Profiler::get_function(unsigned int element, int lineno)
{
  return Profiler_Tools::get_function(profiler_db, element, lineno);
}

void TTCN3_Profiler::create_function(unsigned int element, int lineno, const char* function_name)
{
  Profiler_Tools::create_function(profiler_db, element, lineno, function_name);
}

int TTCN3_Profiler::get_line(unsigned int element, int lineno)
{
  return Profiler_Tools::get_line(profiler_db, element, lineno);
}

void TTCN3_Profiler::create_line(unsigned int element, int lineno)
{
  Profiler_Tools::create_line(profiler_db, element, lineno);
}

void TTCN3_Profiler::add_line_time(timeval elapsed, unsigned int element, int lineno) 
{
  if (0 == lineno) {
    return;
  }
  profiler_db[element].lines[get_line(element, lineno)].total_time = Profiler_Tools::add_timeval(
    profiler_db[element].lines[get_line(element, lineno)].total_time, elapsed);
}

void TTCN3_Profiler::add_function_time(timeval elapsed, unsigned int element, int lineno)
{
  int func = get_function(element, lineno);
  if (-1 == func) {
    return;
  }
  profiler_db[element].functions[func].total_time = Profiler_Tools::add_timeval(
    profiler_db[element].functions[func].total_time, elapsed);
}

void TTCN3_Profiler::update_last()
{
  if (stopped || (0 == prev_time.tv_sec && 0 == prev_time.tv_usec) || NULL == prev_file) {
    return;
  }

  timeval elapsed = Profiler_Tools::subtract_timeval(get_time(), prev_time);

  unsigned int element = get_element(prev_file);
  
  // add the elapsed time to the total time of the previous line
  add_line_time(elapsed, element, prev_line);
  TTCN3_Stack_Depth::update_stack_elapsed(elapsed);

  // reset measurement
  prev_time.tv_sec = 0;
  prev_time.tv_usec = 0;
}

void TTCN3_Profiler::set_prev(int stack_len, const char* filename, int lineno)
{
  prev_file = filename;
  prev_line = lineno;
  if (!disable_profiler) {
    prev_time = get_time();
    prev_stack_len = stack_len;
  }
}

/////////////////////////////////////
///////// TTCN3_Stack_Depth /////////
/////////////////////////////////////

int TTCN3_Stack_Depth::current_depth = -1;
Vector<TTCN3_Stack_Depth::call_stack_timer_item_t> TTCN3_Stack_Depth::call_stack_timer_db;
boolean TTCN3_Stack_Depth::net_line_times = FALSE;
boolean TTCN3_Stack_Depth::net_func_times = FALSE;

TTCN3_Stack_Depth::TTCN3_Stack_Depth() 
{
  if (ttcn3_prof.is_profiler_disabled()) {
    return;
  }
  ++current_depth;
}

TTCN3_Stack_Depth::~TTCN3_Stack_Depth() 
{
  if (ttcn3_prof.is_profiler_disabled()) {
    return;
  }
  ttcn3_prof.update_last();
  remove_stack();
  if (0 == current_depth) {
    ttcn3_prof.reset();
  }
  --current_depth;
}

void TTCN3_Stack_Depth::set_net_line_times(boolean p_net_line_times)
{
  net_line_times = p_net_line_times;
}

void TTCN3_Stack_Depth::set_net_func_times(boolean p_net_func_times)
{
  net_func_times = p_net_func_times;
}

void TTCN3_Stack_Depth::add_stack(int stack_len, const char* caller_file, const char* func_file,
                                  int caller_line, int start_line) 
{
  call_stack_timer_item_t item;
  item.stack_len = stack_len;
  item.caller_file = caller_file;
  item.func_file = func_file;
  item.caller_line = caller_line;
  item.start_line = start_line;
  item.elapsed.tv_sec = 0;
  item.elapsed.tv_usec = 0;
  item.first_call = TRUE;
  item.recursive_call = FALSE;

  if (!net_line_times || !net_func_times) {
    // check if it's a recursive function
    for (int i = current_depth - 1; i >= 0 ; --i) {
      if (call_stack_timer_db[i].start_line == start_line &&
          0 == strcmp(call_stack_timer_db[i].func_file, func_file)) {
        item.recursive_call = TRUE;

        // check if the caller is new
        if (call_stack_timer_db[i].caller_line == caller_line &&
            ((NULL == call_stack_timer_db[i].caller_file && NULL == caller_file) ||
             (NULL != call_stack_timer_db[i].caller_file && NULL != caller_file &&
             0 == strcmp(call_stack_timer_db[i].caller_file, caller_file)))) {
          item.first_call = FALSE;
          break;
        }
      }
    }
  }
  
  call_stack_timer_db.push_back(item);
}

void TTCN3_Stack_Depth::remove_stack()
{
  // add the time gathered for this stack level to the appropriate line and function
  // except for functions starting at line 0 (pre_init_module and post_init_module)
  if (0 != call_stack_timer_db[current_depth].start_line) {
    timeval elapsed = call_stack_timer_db[current_depth].elapsed;
    if (!net_line_times && NULL != call_stack_timer_db[current_depth].caller_file &&
        call_stack_timer_db[current_depth].first_call) {
      // add the elapsed time to the caller line, if it exists
      // (only add it once for recursive functions, at the first call)
      ttcn3_prof.add_line_time(elapsed,
        ttcn3_prof.get_element(call_stack_timer_db[current_depth].caller_file),
        call_stack_timer_db[current_depth].caller_line);
    }
    if (!net_func_times && !call_stack_timer_db[current_depth].recursive_call) {
      // add the elapsed time to the called function, if it's not recursive
      // (in case of net function times this has already been done in update_stack_elapsed)
      ttcn3_prof.add_function_time(elapsed,
        ttcn3_prof.get_element(call_stack_timer_db[current_depth].func_file),
        call_stack_timer_db[current_depth].start_line);
    }
  }

  ttcn3_prof.set_prev(call_stack_timer_db[current_depth].stack_len, 
    call_stack_timer_db[current_depth].caller_file,
    call_stack_timer_db[current_depth].caller_line);
  
  call_stack_timer_db.erase_at(current_depth);
}

void TTCN3_Stack_Depth::update_stack_elapsed(timeval elapsed) 
{
  // if function times are net times, only add the elapsed time to the current function
  if (net_func_times) {
    ttcn3_prof.add_function_time(elapsed,
      ttcn3_prof.get_element(call_stack_timer_db[current_depth].func_file),
      call_stack_timer_db[current_depth].start_line);
  }
  if (!net_line_times || !net_func_times) {
    // cycle through the stack and add the elapsed time to the entries where 
    // the function/caller pair appears for the first time (marked by 'first_call')
    for(int i = 0; i <= current_depth; ++i) {
      if (call_stack_timer_db[i].first_call) {
        call_stack_timer_db[i].elapsed = Profiler_Tools::add_timeval(
          call_stack_timer_db[i].elapsed, elapsed);
      }
    }
  }
}
