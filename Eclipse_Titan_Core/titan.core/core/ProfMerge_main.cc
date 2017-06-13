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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <getopt.h>
#include <string.h>
#include "ProfilerTools.hh"
#include "version_internal.h"

#ifdef LICENSE
#include "license.h"
#endif

/** the name of the executable */
const char* program_name;

/** error flag, set automatically when an error occurs */
boolean erroneous = FALSE;

/** prints usage information */
void usage()
{
  fprintf(stderr,
    "usage:	%s [-pc] [-o file] [-s file] [-f filter] db_file1 [db_file2 ...]\n"
    "	or %s -v\n\n"
    "options:\n"
    "	-p:		discard profiling data\n"
    "	-c:		discard code coverage data\n"
    "	-o file:	write merged database into file\n"
    "	-s file:	generate statistics file from merged database\n"
    "	-f filter:	filter the generated statistics file (the filter is a hexadecimal number)\n"
    "	-v:		show version\n", program_name, program_name);
}

/** displays an error message and sets the erroneous flag */
void error(const char *fmt, ...)
{
  fprintf(stderr, "%s: error: ", program_name);
  va_list parameters;
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  putc('\n', stderr);
  fflush(stderr);
  erroneous = TRUE;
}

/** checks if the specified file exists */
boolean file_exists(const char* p_filename)
{
  FILE* file = fopen(p_filename, "r");
  if (NULL != file) {
    fclose(file);
    return TRUE;
  }
  return FALSE;
}


int main(int argc, char* argv[])
{
  // store the executable name
  program_name = argv[0];
  
  // initialize variables for command line options
  const char* out_file = NULL;
  const char* stats_file = NULL;
  boolean disable_profiler = FALSE;
  boolean disable_coverage = FALSE;
  unsigned int stats_flags = Profiler_Tools::STATS_ALL;
  boolean has_stats_flag = FALSE;
  boolean print_version = FALSE;
  
  if (1 == argc) {
    // no command line options
    usage();
    return EXIT_FAILURE;
  }
  
  for (;;) {
    // read the next command line option (and its argument)
    int c = getopt(argc, argv, "o:s:pcf:v");
    if (-1 == c) {
      break;
    }
    switch (c) {
    case 'o': // output database file
      out_file = optarg;
      break;
    case 's': // statistics file
      stats_file = optarg;
      break;
    case 'p':
      disable_profiler = TRUE;
      break;
    case 'c':
      disable_coverage = TRUE;
      break;
    case 'f': { // statistics filter (hex number)
      has_stats_flag = TRUE;
      size_t len = strlen(optarg);
      size_t start = 0;
      if (len > STATS_MAX_HEX_DIGITS) {
        // the rest of the bits are not needed, and stats_flags might run out of bits
        start = len - STATS_MAX_HEX_DIGITS;
      }
      stats_flags = 0;
      // extract the hex digits from the argument
      for (size_t i = start; i < len; ++i) {
        stats_flags *= 16;
        if ('0' <= optarg[i] && '9' >= optarg[i]) {
          stats_flags += optarg[i] - '0';
        }
        else if ('a' <= optarg[i] && 'f' >= optarg[i]) {
          stats_flags += optarg[i] - 'a' + 10;
        }
        else if ('A' <= optarg[i] && 'F' >= optarg[i]) {
          stats_flags += optarg[i] - 'A' + 10;
        }
        else {
          error("Invalid statistics filter. Expected hexadecimal value.");
          return EXIT_FAILURE;
        }
      }
      break; }
    case 'v':
      print_version = TRUE;
      break;
    default:
      usage();
      return EXIT_FAILURE;
    }
  }
  
  if (print_version) {
    // no other flags are allowed when printing version info
    if (disable_profiler || disable_coverage || has_stats_flag ||
        NULL != out_file || NULL != stats_file) {
      usage();
      return EXIT_FAILURE;
    }
    else {
      fputs("Profiler and Code Coverage Merge Tool for the TTCN-3 Test Executor\n"
	    "Product number: " PRODUCT_NUMBER "\n"
	    "Build date: " __DATE__ " " __TIME__ "\n"
	    "Compiled with: " C_COMPILER_VERSION "\n\n"
	    COPYRIGHT_STRING "\n\n", stderr);
#ifdef LICENSE
      print_license_info();
#endif
      return EXIT_SUCCESS;
    }
  }
  
  if (optind == argc) {
    error("No input files specified.");
    usage();
    return EXIT_FAILURE;
  }
  
  if (disable_profiler && disable_coverage) {
    error("Both profiling and code coverage data are discarded, nothing to do.");
    return EXIT_FAILURE;
  }
  
  if (NULL == out_file && NULL == stats_file) {
    error("No output files specified (either the output database file or the "
      "statistics file must be set).");
    usage();
    return EXIT_FAILURE;
  }
  
  if (has_stats_flag && NULL == stats_file) {
    fprintf(stderr, "Notify: No statistics file specified, the statistics filter "
      "will be ignored.");
  }
  
  // create the local database
  Profiler_Tools::profiler_db_t profiler_db;
  
  for (int i = optind; i < argc; i++) {
    // import each input file's contents into the local database
    fprintf(stderr, "Notify: Importing database file '%s'...\n", argv[i]);
    Profiler_Tools::import_data(profiler_db, argv[i], error);
    if (erroneous) {
      // an import failed -> exit
      return EXIT_FAILURE;
    }
  }

  boolean out_file_success = TRUE;
  if (NULL != out_file) {
    // print the local database into the output file
    boolean update = file_exists(out_file);
    Profiler_Tools::export_data(profiler_db, out_file, disable_profiler,
      disable_coverage, error);
    out_file_success = !erroneous;
    if (out_file_success) {
      fprintf(stderr, "Notify: Database file '%s' was %s.\n", out_file,
        update ? "updated" : "generated");
    }
  }
  
  boolean stats_file_success = TRUE;
  if (NULL != stats_file) {
    // reset the error flag, in case export_data failed
    erroneous = FALSE;
    // print the statistics into the designated file
    boolean update = file_exists(stats_file);
    Profiler_Tools::print_stats(profiler_db, stats_file, disable_profiler,
      disable_coverage, stats_flags, error);
    stats_file_success = !erroneous;
    if (stats_file_success) {
      fprintf(stderr, "Notify: Statistics file '%s' was %s.\n", stats_file,
        update ? "updated" : "generated");
    }
  }
  
  // return an error code if printing either output file failed
  return (out_file_success && stats_file_success) ? EXIT_SUCCESS : EXIT_FAILURE;
}

// this is needed by version.h
reffer::reffer(const char*) {}
