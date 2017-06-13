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

#ifndef PROFILERTOOLS_HH
#define PROFILERTOOLS_HH

#include "Vector.hh"
#include "Types.h"
#include <sys/time.h>

namespace Profiler_Tools {
  
  /** Reads a timeval value from the given string. The parameter must contain the
    * string representation of a real number with 6 digits after the decimal dot. */
  extern timeval string2timeval(const char* str);
  
  /** Returns the string representation of a real number (with 6 digits after the
    * decimal dot) equivalent to the timeval parameter. 
    * The returned character pointer needs to be freed. */
  extern char* timeval2string(timeval tv);
  
  /** Adds the two timeval parameters together and returns the result. */
  extern timeval add_timeval(const timeval operand1, const timeval operand2);
  
  /** Subtracts the second timeval parameter from the first one and returns the result. */
  extern timeval subtract_timeval(const timeval operand1, const timeval operand2);

  /** Database entry for one file */
  struct profiler_db_item_t {
    /** Database entry for one line */
    struct profiler_line_data_t {
      /** Line number */
      int lineno;
      /** The line's total execution time */
      timeval total_time;
      /** The number of times this line was executed */
      int exec_count;
    };
    /** Database entry for one function (including test cases, alt steps, the control part, etc.) */
    struct profiler_function_data_t {
      /** Function name (owned) */
      char* name;
      /** Function starting line */
      int lineno;
      /** The function's total execution time */
      timeval total_time;
      /** The number of times this function was executed */
      int exec_count;
    };
    /** TTCN-3 File name (relative path, owned) */
    char* filename;
    /** Contains database entries for all the lines in this file */
    Vector<profiler_line_data_t> lines;
    /** Contains database entries for all the functions in this file */
    Vector<profiler_function_data_t> functions;
  };
  
  /** Profiler database type */
  typedef Vector<profiler_db_item_t> profiler_db_t;
  
  enum profiler_stats_flag_t {
    // flags for each statistics entry
    STATS_NUMBER_OF_LINES          = 0x0000001,
    STATS_LINE_DATA_RAW            = 0x0000002,
    STATS_FUNC_DATA_RAW            = 0x0000004,
    STATS_LINE_AVG_RAW             = 0x0000008,
    STATS_FUNC_AVG_RAW             = 0x0000010,
    STATS_LINE_TIMES_SORTED_BY_MOD = 0x0000020,
    STATS_FUNC_TIMES_SORTED_BY_MOD = 0x0000040,
    STATS_LINE_TIMES_SORTED_TOTAL  = 0x0000080,
    STATS_FUNC_TIMES_SORTED_TOTAL  = 0x0000100,
    STATS_LINE_COUNT_SORTED_BY_MOD = 0x0000200,
    STATS_FUNC_COUNT_SORTED_BY_MOD = 0x0000400,
    STATS_LINE_COUNT_SORTED_TOTAL  = 0x0000800,
    STATS_FUNC_COUNT_SORTED_TOTAL  = 0x0001000,
    STATS_LINE_AVG_SORTED_BY_MOD   = 0x0002000,
    STATS_FUNC_AVG_SORTED_BY_MOD   = 0x0004000,
    STATS_LINE_AVG_SORTED_TOTAL    = 0x0008000,
    STATS_FUNC_AVG_SORTED_TOTAL    = 0x0010000,
    STATS_TOP10_LINE_TIMES         = 0x0020000,
    STATS_TOP10_FUNC_TIMES         = 0x0040000,
    STATS_TOP10_LINE_COUNT         = 0x0080000,
    STATS_TOP10_FUNC_COUNT         = 0x0100000,
    STATS_TOP10_LINE_AVG           = 0x0200000,
    STATS_TOP10_FUNC_AVG           = 0x0400000,
    STATS_UNUSED_LINES             = 0x0800000,
    STATS_UNUSED_FUNC              = 0x1000000,
    // grouped entries
    STATS_ALL_RAW_DATA             = 0x000001E,
    STATS_LINE_DATA_SORTED_BY_MOD  = 0x0002220,
    STATS_FUNC_DATA_SORTED_BY_MOD  = 0x0004440,
    STATS_LINE_DATA_SORTED_TOTAL   = 0x0008880,
    STATS_FUNC_DATA_SORTED_TOTAL   = 0x0011100,
    STATS_LINE_DATA_SORTED         = 0x000AAA0,
    STATS_FUNC_DATA_SORTED         = 0x0015540,
    STATS_ALL_DATA_SORTED          = 0x001FFE0,
    STATS_TOP10_LINE_DATA          = 0x02A0000,
    STATS_TOP10_FUNC_DATA          = 0x0540000,
    STATS_TOP10_ALL_DATA           = 0x07E0000,
    STATS_UNUSED_DATA              = 0x1800000,
    STATS_ALL                      = 0x1FFFFFF
  };
  
#define STATS_MAX_HEX_DIGITS 7
  
  /** Returns the index of a TTCN-3 function's entry in the specified database
    * @param p_db profiler database
    * @param p_element index of the file (where the function is declared)
    * @param p_lineno function start line */
  extern int get_function(const profiler_db_t& p_db, int p_element, int p_lineno);
  
  /** Creates a new TTCN-3 function entry and inserts it in the specified database 
    * @param p_db profiler database
    * @param p_element file entry's index
    * @param p_lineno function start line
    * @param p_function_name name of the function */
  extern void create_function(profiler_db_t& p_db, int p_element, int p_lineno,
    const char* p_function_name);
  
  /** Returns the index of a TTCN-3 code line's entry in the specified database */
  extern int get_line(const profiler_db_t& p_db, int p_element, int p_lineno);
  
  /** Creates a new TTCN-3 code line entry and inserts it into the specified database */
  extern void create_line(profiler_db_t& p_db, int p_element, int p_lineno);
  
  /** Adds the data from the database file to the local database 
    * @param p_db local database
    * @param p_filename database file name
    * @param p_error_function callback function for displaying error messages */
  extern void import_data(profiler_db_t& p_db, const char* p_filename,
    void (*p_error_function)(const char*, ...));
  
  /** Writes the local database to the database file (overwrites the file) 
    * @param p_db local database
    * @param p_filename database file name
    * @param p_disable_profiler discard profiling data
    * @param p_disable_coverage discard code coverage data
    * @param p_error_function callback function for displaying error messages */
  extern void export_data(profiler_db_t& p_db, const char* p_filename,
    boolean p_disable_profiler, boolean p_disable_coverage,
    void (*p_error_function)(const char*, ...));
  
  /** Calculates and prints statistics from the gathered data 
    * @param p_db local database
    * @param p_filename database file name
    * @param p_disable_profiler discard profiling data
    * @param p_disable_coverage discard code coverage data
    * @param p_flags statistics filter (determines which statistics entries are printed)
    * @param p_error_function callback function for displaying error messages */
  extern void print_stats(profiler_db_t& p_db, const char* p_filename,
    boolean p_disable_profiler, boolean p_disable_coverage,
    unsigned int p_flags, void (*p_error_function)(const char*, ...));
  
}

#endif /* PROFILERTOOLS_HH */

