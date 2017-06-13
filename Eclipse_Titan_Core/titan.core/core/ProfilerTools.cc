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

#include "ProfilerTools.hh"
#include "JSON_Tokenizer.hh"
#include "memory.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace Profiler_Tools {
  
  ////////////////////////////////////
  //////// timeval operations ////////
  ////////////////////////////////////

  timeval string2timeval(const char* str)
  {
    // read and store the first part (atoi will read until the decimal dot)
    long int sec = atoi(str);
    timeval tv;
    tv.tv_sec = sec;

    do {
      // step over each digit
      sec /= 10;
      ++str;
    }
    while (sec > 9);

    // step over the decimal dot and read the second part of the number
    tv.tv_usec = atoi(str + 1);
    return tv;
  }
  
  char* timeval2string(timeval tv)
  {
    // convert the first part and set the second part to all zeros
    char* str = mprintf("%ld.000000", tv.tv_sec);

    // go through each digit of the second part and add them to the zeros in the string
    size_t pos = mstrlen(str) - 1;
    while (tv.tv_usec > 0) {
      str[pos] += tv.tv_usec % 10;
      tv.tv_usec /= 10;
      --pos;
    }
    return str;
  }

  timeval add_timeval(const timeval operand1, const timeval operand2)
  {
    timeval tv;
    tv.tv_usec = operand1.tv_usec + operand2.tv_usec;
    tv.tv_sec = operand1.tv_sec + operand2.tv_sec;
    if (tv.tv_usec >= 1000000) {
      ++tv.tv_sec;
      tv.tv_usec -= 1000000;
    }
    return tv;
  }

  timeval subtract_timeval(const timeval operand1, const timeval operand2)
  {
    timeval tv;
    tv.tv_usec = operand1.tv_usec - operand2.tv_usec;
    tv.tv_sec = operand1.tv_sec - operand2.tv_sec;
    if (tv.tv_usec < 0) {
      --tv.tv_sec;
      tv.tv_usec += 1000000;
    }
    return tv;
  }
  
  ////////////////////////////////////
  ///// profiler data operations /////
  ////////////////////////////////////
  
  int get_function(const profiler_db_t& p_db, int p_element, int p_lineno)
  {
    for (size_t i = 0; i < p_db[p_element].functions.size(); ++i) {
      if (p_db[p_element].functions[i].lineno == p_lineno) {
        return i;
      }
    }
    return -1;
  }
  
  void create_function(profiler_db_t& p_db, int p_element, int p_lineno,
                       const char* p_function_name)
  {
    profiler_db_item_t::profiler_function_data_t func_data;
    func_data.lineno = p_lineno;
    func_data.total_time.tv_sec = 0;
    func_data.total_time.tv_usec = 0;
    func_data.exec_count = 0;
    func_data.name = mcopystr(p_function_name);
    p_db[p_element].functions.push_back(func_data);
  }
  
  int get_line(const profiler_db_t& p_db, int p_element, int p_lineno)
  {
    for (size_t i = 0; i < p_db[p_element].lines.size(); ++i) {
      if (p_db[p_element].lines[i].lineno == p_lineno) {
        return i;
      }
    }
    return -1;
  }
  
  void create_line(profiler_db_t& p_db, int p_element, int p_lineno)
  {
    profiler_db_item_t::profiler_line_data_t line_data;
    line_data.lineno = p_lineno;
    line_data.total_time.tv_sec = 0;
    line_data.total_time.tv_usec = 0;
    line_data.exec_count = 0;
    p_db[p_element].lines.push_back(line_data);
  }
  
#define IMPORT_FORMAT_ERROR(cond) \
  if (cond) { \
    p_error_function("Failed to load profiling and/or code coverage database. Invalid format."); \
    return; \
  }
  
  void import_data(profiler_db_t& p_db, const char* p_filename,
                   void (*p_error_function)(const char*, ...))
  {
    // open the file, if it exists
    FILE* file = fopen(p_filename, "r");
    if (NULL == file) {
      p_error_function("Profiler database file '%s' does not exist.", p_filename);
      return;
    }
    
    // get the file size
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    rewind(file);

    // read the entire file into a character buffer
    char* buffer = (char*)Malloc(file_size);
    int bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    if (bytes_read != file_size) {
      p_error_function("Error reading database file.");
      return;
    }

    // initialize a JSON tokenizer with the buffer
    JSON_Tokenizer json(buffer, file_size);
    Free(buffer);

    // attempt to read tokens from the buffer
    // if the format is invalid, abort the importing process
    json_token_t token = JSON_TOKEN_NONE;
    char* value = NULL;
    size_t value_len = 0;

    // start of main array
    json.get_next_token(&token, NULL, NULL);
    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token);

    // read objects (one for each TTCN-3 file), until the main array end mark is reached
    json.get_next_token(&token, NULL, NULL);
    while (JSON_TOKEN_OBJECT_START == token) {
      size_t file_index = 0;

      // file name:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 4 ||
        0 != strncmp(value, "file", value_len));

      // read the file name and see if its record already exists
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_STRING != token);
      for (file_index = 0; file_index < p_db.size(); ++file_index) {
        if (strlen(p_db[file_index].filename) == value_len - 2 &&
            0 == strncmp(p_db[file_index].filename, value + 1, value_len - 2)) {
          break;
        }
      }

      // insert a new element if the file was not found
      if (p_db.size() == file_index) {
        profiler_db_item_t item;
        item.filename = mcopystrn(value + 1, value_len - 2);
        p_db.push_back(item);
      }

      // functions:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 9 ||
        0 != strncmp(value, "functions", value_len));

      // read and store the functions (an array of objects, same as before)
      json.get_next_token(&token, NULL, NULL);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token);
      json.get_next_token(&token, NULL, NULL);
      while (JSON_TOKEN_OBJECT_START == token) {
        size_t function_index = 0;

        // function name:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 4 ||
          0 != strncmp(value, "name", value_len));

        // read the function name, it will be checked later
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_STRING != token);
        char* function_name = mcopystrn(value + 1, value_len - 2);

        // function start line:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 10 ||
          0 != strncmp(value, "start line", value_len));

        // read the start line and check if the function already exists
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
        int start_line = atoi(value);
        for (function_index = 0; function_index < p_db[file_index].functions.size(); ++function_index) {
          if (p_db[file_index].functions[function_index].lineno == start_line &&
              0 == strcmp(p_db[file_index].functions[function_index].name, function_name)) {
            break;
          }
        }

        // insert a new element if the function was not found
        if (p_db[file_index].functions.size() == function_index) {
          profiler_db_item_t::profiler_function_data_t func_data;
          func_data.name = function_name;
          func_data.lineno = start_line;
          func_data.exec_count = 0;
          func_data.total_time.tv_sec = 0;
          func_data.total_time.tv_usec = 0;
          p_db[file_index].functions.push_back(func_data);
        }

        // function execution count:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 15 ||
          0 != strncmp(value, "execution count", value_len));

        // read the execution count and add it to the current data
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
        p_db[file_index].functions[function_index].exec_count += atoi(value);

        // total function execution time:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 10 ||
          0 != strncmp(value, "total time", value_len));

        // read the total time and add it to the current data
        // note: the database contains a real number, this needs to be split into 2 integers
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
        p_db[file_index].functions[function_index].total_time = add_timeval(
          p_db[file_index].functions[function_index].total_time, string2timeval(value));

        // end of the function's object
        json.get_next_token(&token, NULL, NULL);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token);

        // read the next token (either the start of another object or the function array end)
        json.get_next_token(&token, NULL, NULL);
      }

      // function array end
      IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_END != token);

      // lines:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 5 ||
        0 != strncmp(value, "lines", value_len));

      // read and store the lines (an array of objects, same as before)
      json.get_next_token(&token, NULL, NULL);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token);
      json.get_next_token(&token, NULL, NULL);
      while (JSON_TOKEN_OBJECT_START == token) {
        int line_index = 0;

        // line number:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 6 ||
          0 != strncmp(value, "number", value_len));

        // read the line number and check if the line already exists
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
        int lineno = atoi(value);
        IMPORT_FORMAT_ERROR(lineno < 0);
        line_index = get_line(p_db, file_index, lineno);
        if (-1 == line_index) {
          create_line(p_db, file_index, lineno);
          line_index = p_db[file_index].lines.size() - 1;
        }

        // line execution count:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 15 ||
          0 != strncmp(value, "execution count", value_len));

        // read the execution count and add it to the current data
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
        p_db[file_index].lines[line_index].exec_count += atoi(value);

        // total line execution time:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 10 ||
          0 != strncmp(value, "total time", value_len));

        // read the total time and add it to the current data
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
        p_db[file_index].lines[line_index].total_time = add_timeval(
          p_db[file_index].lines[line_index].total_time, string2timeval(value));

        // end of the line's object
        json.get_next_token(&token, NULL, NULL);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token);

        // read the next token (either the start of another object or the line array end)
        json.get_next_token(&token, NULL, NULL);
      }

      // line array end
      IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_END != token);

      // end of the file's object
      json.get_next_token(&token, NULL, NULL);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token);

      // read the next token (either the start of another object or the main array end)
      json.get_next_token(&token, NULL, NULL);
    }

    // main array end
    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_END != token);
  }
  
  void export_data(profiler_db_t& p_db, const char* p_filename,
                   boolean p_disable_profiler, boolean p_disable_coverage,
                   void (*p_error_function)(const char*, ...))
  {
    // check whether the file can be opened for writing
    FILE* file = fopen(p_filename, "w");
    if (NULL == file) {
      p_error_function("Could not open file '%s' for writing. Profiling and/or code coverage "
        "data will not be saved.", p_filename);
      return;
    }

    // use the JSON tokenizer to create a JSON document from the database
    JSON_Tokenizer json(TRUE);

    // main array, contains an element for each file
    json.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
    for (size_t i = 0; i < p_db.size(); ++i) {

      // each file's data is stored in an object
      json.put_next_token(JSON_TOKEN_OBJECT_START, NULL);

      // store the file name
      json.put_next_token(JSON_TOKEN_NAME, "file");
      char* p_filename_str = mprintf("\"%s\"", p_db[i].filename);
      json.put_next_token(JSON_TOKEN_STRING, p_filename_str);
      Free(p_filename_str);

      // store the function data in an array (one element for each function)
      json.put_next_token(JSON_TOKEN_NAME, "functions");
      json.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
      for (size_t j = 0; j < p_db[i].functions.size(); ++j) {

        // the data is stored in an object for each function
        json.put_next_token(JSON_TOKEN_OBJECT_START, NULL);

        // store the function name
        json.put_next_token(JSON_TOKEN_NAME, "name");
        char* func_name_str = mprintf("\"%s\"", p_db[i].functions[j].name);
        json.put_next_token(JSON_TOKEN_STRING, func_name_str);
        Free(func_name_str);

        // store the function start line
        json.put_next_token(JSON_TOKEN_NAME, "start line");
        char* start_line_str = mprintf("%d", p_db[i].functions[j].lineno);
        json.put_next_token(JSON_TOKEN_NUMBER, start_line_str);
        Free(start_line_str);

        // store the function execution count
        json.put_next_token(JSON_TOKEN_NAME, "execution count");
        char* exec_count_str = mprintf("%d", p_disable_coverage ? 0 :
          p_db[i].functions[j].exec_count);
        json.put_next_token(JSON_TOKEN_NUMBER, exec_count_str);
        Free(exec_count_str);

        // store the function's total execution time
        json.put_next_token(JSON_TOKEN_NAME, "total time");
        if (p_disable_profiler) {
          json.put_next_token(JSON_TOKEN_NUMBER, "0.000000");
        }
        else {
          char* total_time_str = timeval2string(p_db[i].functions[j].total_time);
          json.put_next_token(JSON_TOKEN_NUMBER, total_time_str);
          Free(total_time_str);
        }

        // end of function object
        json.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
      }

      // end of function data array
      json.put_next_token(JSON_TOKEN_ARRAY_END, NULL);

      // store the line data in an array (one element for each line with useful data)
      json.put_next_token(JSON_TOKEN_NAME, "lines");
      json.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
      for (size_t j = 0; j < p_db[i].lines.size(); ++j) {
        
        // store line data in an object
        json.put_next_token(JSON_TOKEN_OBJECT_START, NULL);

        // store the line number
        json.put_next_token(JSON_TOKEN_NAME, "number");
        char* line_number_str = mprintf("%d", p_db[i].lines[j].lineno);
        json.put_next_token(JSON_TOKEN_NUMBER, line_number_str);
        Free(line_number_str);

        // store the line execution count
        json.put_next_token(JSON_TOKEN_NAME, "execution count");
        char* exec_count_str = mprintf("%d", p_disable_coverage ? 0 :
          p_db[i].lines[j].exec_count);
        json.put_next_token(JSON_TOKEN_NUMBER, exec_count_str);
        Free(exec_count_str);

        // store the line's total execution time
        json.put_next_token(JSON_TOKEN_NAME, "total time");
        if (p_disable_profiler) {
          json.put_next_token(JSON_TOKEN_NUMBER, "0.000000");
        }
        else {
          char* total_time_str = timeval2string(p_db[i].lines[j].total_time);
          json.put_next_token(JSON_TOKEN_NUMBER, total_time_str);
          Free(total_time_str);
        }

        // end of this line's object
        json.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
      }

      // end of line data array
      json.put_next_token(JSON_TOKEN_ARRAY_END, NULL);

      // end of this file's object
      json.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
    }

    // end of main array
    json.put_next_token(JSON_TOKEN_ARRAY_END, NULL);

    // write the JSON document into the file
    fprintf(file, "%s\n", json.get_buffer());
    fclose(file);
  }
  
  // Structure for one code line or function, used by print_stats for sorting
  struct stats_data_t {
    const char* filename; // not owned
    const char* funcname; // not owned, NULL for code lines that don't start a function
    int lineno;
    timeval total_time;
    int exec_count;
  };

  // Compare function for sorting stats data based on total execution time (descending)
  int stats_data_cmp_time(const void* p_left, const void* p_right) {
    const stats_data_t* p_left_data = (const stats_data_t*)p_left;
    const stats_data_t* p_right_data = (const stats_data_t*)p_right;
    if (p_left_data->total_time.tv_sec > p_right_data->total_time.tv_sec) return -1;
    if (p_left_data->total_time.tv_sec < p_right_data->total_time.tv_sec) return 1;
    if (p_left_data->total_time.tv_usec > p_right_data->total_time.tv_usec) return -1;
    if (p_left_data->total_time.tv_usec < p_right_data->total_time.tv_usec) return 1;
    return 0;
  }

  // Compare function for sorting stats data based on execution count (descending)
  int stats_data_cmp_count(const void* p_left, const void* p_right) {
    return ((const stats_data_t*)p_right)->exec_count - ((const stats_data_t*)p_left)->exec_count;
  }

  // Compare function for sorting stats data based on total time per execution count (descending)
  int stats_data_cmp_avg(const void* p_left, const void* p_right) {
    const stats_data_t* p_left_data = (const stats_data_t*)p_left;
    const stats_data_t* p_right_data = (const stats_data_t*)p_right;
    double left_time = p_left_data->total_time.tv_sec + p_left_data->total_time.tv_usec / 1000000.0;
    double right_time = p_right_data->total_time.tv_sec + p_right_data->total_time.tv_usec / 1000000.0;
    double diff = (right_time / p_right_data->exec_count) - (left_time / p_left_data->exec_count);
    if (diff < 0) return -1;
    if (diff > 0) return 1;
    return 0;
  }
  
  void print_stats(profiler_db_t& p_db, const char* p_filename,
                   boolean p_disable_profiler, boolean p_disable_coverage,
                   unsigned int p_flags, void (*p_error_function)(const char*, ...))
  {
    // title
    char* title_str = mprintf(
      "##################################################\n"
      "%s## TTCN-3 %s%s%sstatistics ##%s\n"
      "##################################################\n\n\n"
      , p_disable_profiler ? "#######" : (p_disable_coverage ? "#########" : "")
      , p_disable_profiler ? "" : "profiler "
      , (p_disable_profiler || p_disable_coverage) ? "" : "and "
      , p_disable_coverage ? "" : "code coverage "
      , p_disable_profiler ? "######" : (p_disable_coverage ? "#########" : ""));

    char* line_func_count_str = NULL;
    if (p_flags & STATS_NUMBER_OF_LINES) {
      line_func_count_str = mcopystr(
        "--------------------------------------\n"
        "- Number of code lines and functions -\n"
        "--------------------------------------\n");
    }

    // line data
    char* line_data_str = NULL;
    if (p_flags & STATS_LINE_DATA_RAW) {
      line_data_str = mprintf(
        "-------------------------------------------------\n"
        "%s- Code line data (%s%s%s) -%s\n"
        "-------------------------------------------------\n"
        , p_disable_profiler ? "-------" : (p_disable_coverage ? "---------" : "")
        , p_disable_profiler ? "" : "total time"
        , (p_disable_profiler || p_disable_coverage) ? "" : " / "
        , p_disable_coverage ? "" : "execution count"
        , p_disable_profiler ? "------" : (p_disable_coverage ? "---------" : ""));
    }

    // average time / exec count for lines
    char* line_avg_str = NULL;
    if (!p_disable_coverage && !p_disable_profiler && (p_flags & STATS_LINE_AVG_RAW)) {
      line_avg_str = mcopystr(
        "-------------------------------------------\n"
        "- Average time / execution for code lines -\n"
        "-------------------------------------------\n");
    }

    // function data
    char* func_data_str = NULL;
    if (p_flags & STATS_FUNC_DATA_RAW) {
      func_data_str = mprintf(
        "------------------------------------------------\n"
        "%s- Function data (%s%s%s) -%s\n"
        "------------------------------------------------\n"
        , p_disable_profiler ? "-------" : (p_disable_coverage ? "---------" : "")
        , p_disable_profiler ? "" : "total time"
        , (p_disable_profiler || p_disable_coverage) ? "" : " / "
        , p_disable_coverage ? "" : "execution count"
        , p_disable_profiler ? "------" : (p_disable_coverage ? "---------" : ""));
    }

    // average time / exec count for functions
    char* func_avg_str = NULL;
    if (!p_disable_coverage && !p_disable_profiler && (p_flags & STATS_FUNC_AVG_RAW)) {
      func_avg_str = mcopystr(
        "------------------------------------------\n"
        "- Average time / execution for functions -\n"
        "------------------------------------------\n");
    }

    char* line_time_sorted_mod_str = NULL;
    if (!p_disable_profiler && (p_flags & STATS_LINE_TIMES_SORTED_BY_MOD)) {
      line_time_sorted_mod_str = mcopystr(
        "------------------------------------------------\n"
        "- Total time of code lines, sorted, per module -\n"
        "------------------------------------------------\n");
    }

    char* line_count_sorted_mod_str = NULL;
    if (!p_disable_coverage && (p_flags & STATS_LINE_COUNT_SORTED_BY_MOD)) {
      line_count_sorted_mod_str = mcopystr(
        "-----------------------------------------------------\n"
        "- Execution count of code lines, sorted, per module -\n"
        "-----------------------------------------------------\n");
    }

    char* line_avg_sorted_mod_str = NULL;
    if (!p_disable_profiler && !p_disable_coverage && (p_flags & STATS_LINE_AVG_SORTED_BY_MOD)) {
      line_avg_sorted_mod_str = mcopystr(
        "--------------------------------------------------------------\n"
        "- Average time / execution of code lines, sorted, per module -\n"
        "--------------------------------------------------------------\n");
    }

    char* line_time_sorted_tot_str = NULL;
    if (!p_disable_profiler && (p_flags & STATS_LINE_TIMES_SORTED_TOTAL)) {
      line_time_sorted_tot_str = mcopystr(
        "-------------------------------------------\n"
        "- Total time of code lines, sorted, total -\n"
        "-------------------------------------------\n");
    }

    char* line_count_sorted_tot_str = NULL;
    if (!p_disable_coverage && (p_flags & STATS_LINE_COUNT_SORTED_TOTAL)) {
      line_count_sorted_tot_str = mcopystr(
        "------------------------------------------------\n"
        "- Execution count of code lines, sorted, total -\n"
        "------------------------------------------------\n");
    }

    char* line_avg_sorted_tot_str = NULL;
    if (!p_disable_profiler && !p_disable_coverage && (p_flags & STATS_LINE_AVG_SORTED_TOTAL)) {
      line_avg_sorted_tot_str = mcopystr(
        "---------------------------------------------------------\n"
        "- Average time / execution of code lines, sorted, total -\n"
        "---------------------------------------------------------\n");
    }

    char* func_time_sorted_mod_str = NULL;
    if (!p_disable_profiler && (p_flags & STATS_FUNC_TIMES_SORTED_BY_MOD)) {
      func_time_sorted_mod_str = mcopystr(
        "-----------------------------------------------\n"
        "- Total time of functions, sorted, per module -\n"
        "-----------------------------------------------\n");
    }

    char* func_count_sorted_mod_str = NULL;
    if (!p_disable_coverage && (p_flags & STATS_FUNC_COUNT_SORTED_BY_MOD)) {
      func_count_sorted_mod_str = mcopystr(
        "----------------------------------------------------\n"
        "- Execution count of functions, sorted, per module -\n"
        "----------------------------------------------------\n");
    }

    char* func_avg_sorted_mod_str = NULL;
    if (!p_disable_profiler && !p_disable_coverage && (p_flags & STATS_FUNC_AVG_SORTED_BY_MOD)) {
      func_avg_sorted_mod_str = mcopystr(
        "-------------------------------------------------------------\n"
        "- Average time / execution of functions, sorted, per module -\n"
        "-------------------------------------------------------------\n");
    }

    char* func_time_sorted_tot_str = NULL;
    if (!p_disable_profiler && (p_flags & STATS_FUNC_TIMES_SORTED_TOTAL)) {
      func_time_sorted_tot_str = mcopystr(
        "------------------------------------------\n"
        "- Total time of functions, sorted, total -\n"
        "------------------------------------------\n");
    }

    char* func_count_sorted_tot_str = NULL;
    if (!p_disable_coverage && (p_flags & STATS_FUNC_COUNT_SORTED_TOTAL)) {
      func_count_sorted_tot_str = mcopystr(
        "-----------------------------------------------\n"
        "- Execution count of functions, sorted, total -\n"
        "-----------------------------------------------\n");
    }

    char* func_avg_sorted_tot_str = NULL;
    if (!p_disable_profiler && !p_disable_coverage && (p_flags & STATS_FUNC_AVG_SORTED_TOTAL)) {
      func_avg_sorted_tot_str = mcopystr(
        "--------------------------------------------------------\n"
        "- Average time / execution of functions, sorted, total -\n"
        "--------------------------------------------------------\n");
    }

    char* line_time_sorted_top10_str = NULL;
    if (!p_disable_profiler && (p_flags & STATS_TOP10_LINE_TIMES)) {
      line_time_sorted_top10_str = mcopystr(
        "------------------------------------\n"
        "- Total time of code lines, top 10 -\n"
        "------------------------------------\n");
    }

    char* line_count_sorted_top10_str = NULL;
    if (!p_disable_coverage && (p_flags & STATS_TOP10_LINE_COUNT)) {
      line_count_sorted_top10_str = mcopystr(
        "-----------------------------------------\n"
        "- Execution count of code lines, top 10 -\n"
        "-----------------------------------------\n");
    }

    char* line_avg_sorted_top10_str = NULL;
    if (!p_disable_profiler && !p_disable_coverage && (p_flags & STATS_TOP10_LINE_AVG)) {
      line_avg_sorted_top10_str = mcopystr(
        "--------------------------------------------------\n"
        "- Average time / execution of code lines, top 10 -\n"
        "--------------------------------------------------\n");
    }

    char* func_time_sorted_top10_str = NULL;
    if (!p_disable_profiler && (p_flags & STATS_TOP10_FUNC_TIMES)) {
      func_time_sorted_top10_str = mcopystr(
        "-----------------------------------\n"
        "- Total time of functions, top 10 -\n"
        "-----------------------------------\n");
    }

    char* func_count_sorted_top10_str = NULL;
    if (!p_disable_coverage && (p_flags & STATS_TOP10_FUNC_COUNT)) {
      func_count_sorted_top10_str = mcopystr(
        "----------------------------------------\n"
        "- Execution count of functions, top 10 -\n"
        "----------------------------------------\n");
    }

    char* func_avg_sorted_top10_str = NULL;
    if (!p_disable_profiler && !p_disable_coverage && (p_flags & STATS_TOP10_FUNC_AVG)) {
      func_avg_sorted_top10_str = mcopystr(
        "-------------------------------------------------\n"
        "- Average time / execution of functions, top 10 -\n"
        "-------------------------------------------------\n");
    }

    char* unused_lines_str = NULL;
    char* unused_func_str = NULL;
    if (!p_disable_coverage && (p_flags & STATS_UNUSED_LINES)) {
      unused_lines_str = mcopystr(
        "---------------------\n"
        "- Unused code lines -\n"
        "---------------------\n");
    }
    if (!p_disable_coverage && (p_flags & STATS_UNUSED_FUNC)) {
      unused_func_str = mcopystr(
        "--------------------\n"
        "- Unused functions -\n"
        "--------------------\n");
    }

    // variables for counting totals, and for determining the amount of unused lines/functions
    size_t total_code_lines = 0;
    size_t total_functions = 0;
    size_t used_code_lines = 0;
    size_t used_functions = 0;

    // cached sizes of statistics data segments, needed to determine whether a separator
    // is needed or not
    size_t line_data_str_len = mstrlen(line_data_str);
    size_t func_data_str_len = mstrlen(func_data_str);
    size_t unused_lines_str_len = mstrlen(unused_lines_str);
    size_t unused_func_str_len = mstrlen(unused_func_str);
    size_t line_avg_str_len = mstrlen(line_avg_str);
    size_t func_avg_str_len = mstrlen(func_avg_str);

    // cycle through the database and gather the necessary data
    for (size_t i = 0; i < p_db.size(); ++i) {
      if (i > 0) {
        // add separators between files (only add them if the previous file actually added something)
        if ((p_flags & STATS_LINE_DATA_RAW) && line_data_str_len != mstrlen(line_data_str)) {
          line_data_str = mputstr(line_data_str, "-------------------------------------------------\n");
          line_data_str_len = mstrlen(line_data_str);
        }
        if ((p_flags & STATS_FUNC_DATA_RAW) && func_data_str_len != mstrlen(func_data_str)) {
          func_data_str = mputstr(func_data_str, "------------------------------------------------\n");
          func_data_str_len = mstrlen(func_data_str);
        }
        if (!p_disable_coverage) {
          if ((p_flags & STATS_UNUSED_LINES) && unused_lines_str_len != mstrlen(unused_lines_str)) {
            unused_lines_str = mputstr(unused_lines_str, "---------------------\n");
            unused_lines_str_len = mstrlen(unused_lines_str);
          }
          if ((p_flags & STATS_UNUSED_FUNC) && unused_func_str_len != mstrlen(unused_func_str)) {
            unused_func_str = mputstr(unused_func_str, "--------------------\n");
            unused_func_str_len = mstrlen(unused_func_str);
          }
          if (!p_disable_profiler) {
            if ((p_flags & STATS_LINE_AVG_RAW) && line_avg_str_len != mstrlen(line_avg_str)) {
              line_avg_str = mputstr(line_avg_str, "-------------------------------------------\n");
              line_avg_str_len = mstrlen(line_avg_str);
            }
            if ((p_flags & STATS_FUNC_AVG_RAW) && func_avg_str_len != mstrlen(func_avg_str)) {
              func_avg_str = mputstr(func_avg_str, "------------------------------------------\n");
              func_avg_str_len = mstrlen(func_avg_str);
            }
          }
        }
      }

      // lines
      for (size_t j = 0; j < p_db[i].lines.size(); ++j) {
        // line specification (including function name for the function's start line)
        char* line_spec_str = mprintf("%s:%d", p_db[i].filename,
          p_db[i].lines[j].lineno);
        int func = get_function(p_db, i, p_db[i].lines[j].lineno);
        if (-1 != func) {
          line_spec_str = mputprintf(line_spec_str, " [%s]", p_db[i].functions[func].name);
        }
        line_spec_str = mputstrn(line_spec_str, "\n", 1);

        if (p_disable_coverage || 0 != p_db[i].lines[j].exec_count) {
          if (!p_disable_profiler) {
            if (p_flags & STATS_LINE_DATA_RAW) {
              char* total_time_str = timeval2string(p_db[i].lines[j].total_time);
              line_data_str = mputprintf(line_data_str, "%ss", total_time_str);
              Free(total_time_str);
            }
            if (!p_disable_coverage) {
              if (p_flags & STATS_LINE_DATA_RAW) {
                line_data_str = mputstrn(line_data_str, "\t/\t", 3);
              }
              if (p_flags & STATS_LINE_AVG_RAW) {
                double avg = (p_db[i].lines[j].total_time.tv_sec +
                  p_db[i].lines[j].total_time.tv_usec / 1000000.0) /
                  p_db[i].lines[j].exec_count;
                char* total_time_str = timeval2string(p_db[i].lines[j].total_time);
                line_avg_str = mputprintf(line_avg_str, "%.6lfs\t(%ss / %d)", 
                  avg, total_time_str, p_db[i].lines[j].exec_count);
                Free(total_time_str);
              }
            }
          }
          if (!p_disable_coverage && (p_flags & STATS_LINE_DATA_RAW)) {
            line_data_str = mputprintf(line_data_str, "%d", p_db[i].lines[j].exec_count);
          }

          // add the line spec string to the other strings
          if (p_flags & STATS_LINE_DATA_RAW) {
            line_data_str = mputprintf(line_data_str, "\t%s", line_spec_str);
          }
          if (!p_disable_profiler && !p_disable_coverage && (p_flags & STATS_LINE_AVG_RAW)) {
            line_avg_str = mputprintf(line_avg_str, "\t%s", line_spec_str);
          }
          ++used_code_lines;
        }
        else if (p_flags & STATS_UNUSED_LINES) {
          // unused line
          unused_lines_str = mputstr(unused_lines_str, line_spec_str);
        }
        Free(line_spec_str);
      }

      // functions
      for (size_t j = 0; j < p_db[i].functions.size(); ++j) {
        // functions specification
        char* func_spec_str = mprintf("%s:%d [%s]\n", p_db[i].filename,
          p_db[i].functions[j].lineno, p_db[i].functions[j].name);

        if (p_disable_coverage || 0 != p_db[i].functions[j].exec_count) {
          if (!p_disable_profiler) {
            if (p_flags & STATS_FUNC_DATA_RAW) {
              char* total_time_str = timeval2string(p_db[i].functions[j].total_time);
              func_data_str = mputprintf(func_data_str, "%ss", total_time_str);
              Free(total_time_str);
            }
            if (!p_disable_coverage) {
              if (p_flags & STATS_FUNC_DATA_RAW) {
                func_data_str = mputstrn(func_data_str, "\t/\t", 3);
              }
              if (p_flags & STATS_FUNC_AVG_RAW) {
                double avg = (p_db[i].functions[j].total_time.tv_sec +
                  p_db[i].functions[j].total_time.tv_usec / 1000000.0) /
                  p_db[i].functions[j].exec_count;
                char* total_time_str = timeval2string(p_db[i].functions[j].total_time);
                func_avg_str = mputprintf(func_avg_str, "%.6lfs\t(%ss / %d)", 
                  avg, total_time_str, p_db[i].functions[j].exec_count);
                Free(total_time_str);
              }
            }
          }
          if (!p_disable_coverage && (p_flags & STATS_FUNC_DATA_RAW)) {
            func_data_str = mputprintf(func_data_str, "%d", p_db[i].functions[j].exec_count);
          }

          // add the line spec string to the other strings
          if (p_flags & STATS_FUNC_DATA_RAW) {
            func_data_str = mputprintf(func_data_str, "\t%s", func_spec_str);
          }
          if (!p_disable_profiler && !p_disable_coverage && (p_flags & STATS_FUNC_AVG_RAW)) {
            func_avg_str = mputprintf(func_avg_str, "\t%s", func_spec_str);
          }

          ++used_functions;
        }
        else if (p_flags & STATS_UNUSED_FUNC) {
          // unused function
          unused_func_str = mputstr(unused_func_str, func_spec_str);
        }
        Free(func_spec_str);
      }

      // number of lines and functions
      if (p_flags & STATS_NUMBER_OF_LINES) {
        line_func_count_str = mputprintf(line_func_count_str, "%s:\t%lu lines,\t%lu functions\n",
           p_db[i].filename, (unsigned long)(p_db[i].lines.size()), (unsigned long)(p_db[i].functions.size()));
      }
      total_code_lines += p_db[i].lines.size();
      total_functions += p_db[i].functions.size();
    }
    if (p_flags & STATS_NUMBER_OF_LINES) {
      line_func_count_str = mputprintf(line_func_count_str,
        "--------------------------------------\n"
        "Total:\t%lu lines,\t%lu functions\n", (unsigned long)total_code_lines, (unsigned long)total_functions);
    }

    if (p_flags & (STATS_TOP10_ALL_DATA | STATS_ALL_DATA_SORTED)) {
      // copy code line and function info into stats_data_t containers for sorting
      stats_data_t* code_line_stats = (stats_data_t*)Malloc(used_code_lines * sizeof(stats_data_t));
      stats_data_t* function_stats = (stats_data_t*)Malloc(used_functions * sizeof(stats_data_t));
      int line_index = 0;
      int func_index = 0;

      for (size_t i = 0; i < p_db.size(); ++i) {
        for (size_t j = 0; j < p_db[i].lines.size(); ++j) {
          if (p_disable_coverage || 0 != p_db[i].lines[j].exec_count) {
            code_line_stats[line_index].filename = p_db[i].filename;
            code_line_stats[line_index].funcname = NULL;
            code_line_stats[line_index].lineno = p_db[i].lines[j].lineno;
            code_line_stats[line_index].total_time = p_db[i].lines[j].total_time;
            code_line_stats[line_index].exec_count = p_db[i].lines[j].exec_count;
            int func = get_function(p_db, i, p_db[i].lines[j].lineno);
            if (-1 != func) {
              code_line_stats[line_index].funcname = p_db[i].functions[func].name;
            }
            ++line_index;
          }
        }
        for (size_t j = 0; j < p_db[i].functions.size(); ++j) {
          if (p_disable_coverage || 0 != p_db[i].functions[j].exec_count) {
            function_stats[func_index].filename = p_db[i].filename;
            function_stats[func_index].funcname = p_db[i].functions[j].name;
            function_stats[func_index].lineno = p_db[i].functions[j].lineno;
            function_stats[func_index].total_time = p_db[i].functions[j].total_time;
            function_stats[func_index].exec_count = p_db[i].functions[j].exec_count;
            ++func_index;
          }
        }
      }

      if (!p_disable_profiler) {
        // sort the code lines and functions by total time
        qsort(code_line_stats, used_code_lines, sizeof(stats_data_t), &stats_data_cmp_time);
        qsort(function_stats, used_functions, sizeof(stats_data_t), &stats_data_cmp_time);

        if (p_flags & (STATS_LINE_TIMES_SORTED_TOTAL | STATS_TOP10_LINE_TIMES)) {
          // cycle through the sorted code lines and gather the necessary data
          for (size_t i = 0; i < used_code_lines; ++i) {
            char* total_time_str = timeval2string(code_line_stats[i].total_time);
            char* the_data = mprintf("%ss\t%s:%d", total_time_str,
              code_line_stats[i].filename, code_line_stats[i].lineno);
            Free(total_time_str);
            if (NULL != code_line_stats[i].funcname) {
              the_data = mputprintf(the_data, " [%s]", code_line_stats[i].funcname);
            }
            the_data = mputstrn(the_data, "\n", 1);
            if (p_flags & STATS_LINE_TIMES_SORTED_TOTAL) {
              line_time_sorted_tot_str = mputstr(line_time_sorted_tot_str, the_data);
            }
            if (i < 10 && (p_flags & STATS_TOP10_LINE_TIMES)) {
              line_time_sorted_top10_str = mputprintf(line_time_sorted_top10_str,
                "%2lu.\t%s", i + 1, the_data);
            }
            Free(the_data);
          }
        }

        if (p_flags & (STATS_FUNC_TIMES_SORTED_TOTAL | STATS_TOP10_FUNC_TIMES)) {
          // cycle through the sorted functions and gather the necessary data
          for (size_t i = 0; i < used_functions; ++i) {
            char* total_time_str = timeval2string(function_stats[i].total_time);
            char* the_data = mprintf("%ss\t%s:%d [%s]\n", total_time_str,
              function_stats[i].filename, function_stats[i].lineno, function_stats[i].funcname);
            Free(total_time_str);
            if (p_flags & STATS_FUNC_TIMES_SORTED_TOTAL) {
              func_time_sorted_tot_str = mputstr(func_time_sorted_tot_str, the_data);
            }
            if (i < 10 && (p_flags & STATS_TOP10_FUNC_TIMES)) {
              func_time_sorted_top10_str = mputprintf(func_time_sorted_top10_str,
                "%2lu.\t%s", i + 1, the_data);
            }
            Free(the_data);
          }
        }

        if (p_flags & (STATS_LINE_TIMES_SORTED_BY_MOD | STATS_FUNC_TIMES_SORTED_BY_MOD)) {
          // cached string lengths, to avoid multiple separators after each other
          size_t line_time_sorted_mod_str_len = mstrlen(line_time_sorted_mod_str);
          size_t func_time_sorted_mod_str_len = mstrlen(func_time_sorted_mod_str);

          // cycle through the sorted statistics and gather the necessary data per module
          for (size_t i = 0; i < p_db.size(); ++i) {
            if (i > 0) {
              if ((p_flags & STATS_LINE_TIMES_SORTED_BY_MOD) &&
                  line_time_sorted_mod_str_len != mstrlen(line_time_sorted_mod_str)) {
                line_time_sorted_mod_str = mputstr(line_time_sorted_mod_str,
                  "------------------------------------------------\n");
                line_time_sorted_mod_str_len = mstrlen(line_time_sorted_mod_str);
              }
              if ((p_flags & STATS_FUNC_TIMES_SORTED_BY_MOD) &&
                  func_time_sorted_mod_str_len != mstrlen(func_time_sorted_mod_str)) {
                func_time_sorted_mod_str = mputstr(func_time_sorted_mod_str,
                  "-----------------------------------------------\n");
                func_time_sorted_mod_str_len = mstrlen(func_time_sorted_mod_str);
              }
            }
            if (p_flags & STATS_LINE_TIMES_SORTED_BY_MOD) {
              for (size_t j = 0; j < used_code_lines; ++j) {
                if (0 == strcmp(code_line_stats[j].filename, p_db[i].filename)) {
                  char* total_time_str = timeval2string(code_line_stats[j].total_time);
                  line_time_sorted_mod_str = mputprintf(line_time_sorted_mod_str,
                    "%ss\t%s:%d", total_time_str, code_line_stats[j].filename,
                    code_line_stats[j].lineno);
                  Free(total_time_str);
                  if (NULL != code_line_stats[j].funcname) {
                    line_time_sorted_mod_str = mputprintf(line_time_sorted_mod_str,
                      " [%s]", code_line_stats[j].funcname);
                  }
                  line_time_sorted_mod_str = mputstrn(line_time_sorted_mod_str, "\n", 1);
                }
              }
            }
            if (p_flags & STATS_FUNC_TIMES_SORTED_BY_MOD) {
              for (size_t j = 0; j < used_functions; ++j) {
                if (0 == strcmp(function_stats[j].filename, p_db[i].filename)) {
                  char* total_time_str = timeval2string(function_stats[j].total_time);
                  func_time_sorted_mod_str = mputprintf(func_time_sorted_mod_str,
                    "%ss\t%s:%d [%s]\n", total_time_str, function_stats[j].filename,
                    function_stats[j].lineno, function_stats[j].funcname);
                  Free(total_time_str);
                }
              }
            }
          }
        }
      }

      if (!p_disable_coverage) {
        // sort the code lines and functions by execution count
        qsort(code_line_stats, used_code_lines, sizeof(stats_data_t), &stats_data_cmp_count);
        qsort(function_stats, used_functions, sizeof(stats_data_t), &stats_data_cmp_count);

        if (p_flags & (STATS_LINE_COUNT_SORTED_TOTAL | STATS_TOP10_LINE_COUNT)) {
          // cycle through the sorted code lines and gather the necessary data
          for (size_t i = 0; i < used_code_lines; ++i) {
            char* the_data = mprintf("%d\t%s:%d", code_line_stats[i].exec_count,
              code_line_stats[i].filename, code_line_stats[i].lineno);
            if (NULL != code_line_stats[i].funcname) {
              the_data = mputprintf(the_data, " [%s]", code_line_stats[i].funcname);
            }
            the_data = mputstrn(the_data, "\n", 1);
            if (p_flags & STATS_LINE_COUNT_SORTED_TOTAL) {
              line_count_sorted_tot_str = mputstr(line_count_sorted_tot_str, the_data);
            }
            if (i < 10 && (p_flags & STATS_TOP10_LINE_COUNT)) {
              line_count_sorted_top10_str = mputprintf(line_count_sorted_top10_str,
                "%2lu.\t%s", i + 1, the_data);
            }
            Free(the_data);
          }
        }

        if (p_flags & (STATS_FUNC_COUNT_SORTED_TOTAL | STATS_TOP10_FUNC_COUNT)) {
          // cycle through the sorted functions and gather the necessary data
          for (size_t i = 0; i < used_functions; ++i) {
            char* the_data = mprintf("%d\t%s:%d [%s]\n",
              function_stats[i].exec_count, function_stats[i].filename,
              function_stats[i].lineno, function_stats[i].funcname);
            if (p_flags & STATS_FUNC_COUNT_SORTED_TOTAL) {
              func_count_sorted_tot_str = mputstr(func_count_sorted_tot_str, the_data);
            }
            if (i < 10 && (p_flags & STATS_TOP10_FUNC_COUNT)) {
              func_count_sorted_top10_str = mputprintf(func_count_sorted_top10_str,
                "%2lu.\t%s", i + 1, the_data);
            }
            Free(the_data);
          }
        }

        if (p_flags & (STATS_LINE_COUNT_SORTED_BY_MOD | STATS_FUNC_COUNT_SORTED_BY_MOD)) {
          // cached string lengths, to avoid multiple separators after each other
          size_t line_count_sorted_mod_str_len = mstrlen(line_count_sorted_mod_str);
          size_t func_count_sorted_mod_str_len = mstrlen(func_count_sorted_mod_str);

          // cycle through the sorted statistics and gather the necessary data per module
          for (size_t i = 0; i < p_db.size(); ++i) {
            if (i > 0) {
              if ((p_flags & STATS_LINE_COUNT_SORTED_BY_MOD) &&
                  line_count_sorted_mod_str_len != mstrlen(line_count_sorted_mod_str)) {
                line_count_sorted_mod_str = mputstr(line_count_sorted_mod_str,
                  "-----------------------------------------------------\n");
                line_count_sorted_mod_str_len = mstrlen(line_count_sorted_mod_str);
              }
              if ((p_flags & STATS_FUNC_COUNT_SORTED_BY_MOD) &&
                  func_count_sorted_mod_str_len != mstrlen(func_count_sorted_mod_str)) {
                func_count_sorted_mod_str = mputstr(func_count_sorted_mod_str,
                  "----------------------------------------------------\n");
                func_count_sorted_mod_str_len = mstrlen(func_count_sorted_mod_str);
              }
            }
            if (p_flags & STATS_LINE_COUNT_SORTED_BY_MOD) {
              for (size_t j = 0; j < used_code_lines; ++j) {
                if (0 == strcmp(code_line_stats[j].filename, p_db[i].filename)) {
                  line_count_sorted_mod_str = mputprintf(line_count_sorted_mod_str,
                    "%d\t%s:%d", code_line_stats[j].exec_count, code_line_stats[j].filename,
                    code_line_stats[j].lineno);
                  if (NULL != code_line_stats[j].funcname) {
                    line_count_sorted_mod_str = mputprintf(line_count_sorted_mod_str,
                      " [%s]", code_line_stats[j].funcname);
                  }
                  line_count_sorted_mod_str = mputstrn(line_count_sorted_mod_str, "\n", 1);
                }
              }
            }
            if (p_flags & STATS_FUNC_COUNT_SORTED_BY_MOD) {
              for (size_t j = 0; j < used_functions; ++j) {
                if (0 == strcmp(function_stats[j].filename, p_db[i].filename)) {
                  func_count_sorted_mod_str = mputprintf(func_count_sorted_mod_str,
                    "%d\t%s:%d [%s]\n", function_stats[j].exec_count, function_stats[j].filename,
                    function_stats[j].lineno, function_stats[j].funcname);
                }
              }
            }
          }
        }
      }

      if (!p_disable_profiler && !p_disable_coverage) {
        // sort the code lines and functions by average time / execution
        qsort(code_line_stats, used_code_lines, sizeof(stats_data_t), &stats_data_cmp_avg);
        qsort(function_stats, used_functions, sizeof(stats_data_t), &stats_data_cmp_avg);

        if (p_flags & (STATS_LINE_AVG_SORTED_TOTAL | STATS_TOP10_LINE_AVG)) {
          // cycle through the sorted code lines and gather the necessary data
          for (size_t i = 0; i < used_code_lines; ++i) {
            double avg = (code_line_stats[i].total_time.tv_sec +
              code_line_stats[i].total_time.tv_usec / 1000000.0) /
              code_line_stats[i].exec_count;
            char* total_time_str = timeval2string(code_line_stats[i].total_time);
            char* the_data = mprintf("%.6lfs\t(%ss / %d)\t%s:%d",
              avg, total_time_str, code_line_stats[i].exec_count,
              code_line_stats[i].filename, code_line_stats[i].lineno);
            Free(total_time_str);
            if (NULL != code_line_stats[i].funcname) {
              the_data = mputprintf(the_data, " [%s]", code_line_stats[i].funcname);
            }
            the_data = mputstrn(the_data, "\n", 1);
            if (p_flags & STATS_LINE_AVG_SORTED_TOTAL) {
              line_avg_sorted_tot_str = mputstr(line_avg_sorted_tot_str, the_data);
            }
            if (i < 10 && (p_flags & STATS_TOP10_LINE_AVG)) {
              line_avg_sorted_top10_str = mputprintf(line_avg_sorted_top10_str,
                "%2lu.\t%s", i + 1, the_data);
            }
            Free(the_data);
          }
        }

        if (p_flags & (STATS_FUNC_AVG_SORTED_TOTAL | STATS_TOP10_FUNC_AVG)) {
          // cycle through the sorted functions and gather the necessary data
          for (size_t i = 0; i < used_functions; ++i) {
            double avg = (function_stats[i].total_time.tv_sec +
              function_stats[i].total_time.tv_usec / 1000000.0) /
              function_stats[i].exec_count;
            char* total_time_str = timeval2string(function_stats[i].total_time);
            char* the_data = mprintf("%.6lfs\t(%ss / %d)\t%s:%d [%s]\n",
              avg, total_time_str, function_stats[i].exec_count,
              function_stats[i].filename, function_stats[i].lineno, function_stats[i].funcname);
            Free(total_time_str);
            if (p_flags & STATS_FUNC_AVG_SORTED_TOTAL) {
              func_avg_sorted_tot_str = mputstr(func_avg_sorted_tot_str, the_data);
            }
            if (i < 10 && (p_flags & STATS_TOP10_FUNC_AVG)) {
              func_avg_sorted_top10_str = mputprintf(func_avg_sorted_top10_str,
                "%2lu.\t%s", i + 1, the_data);
            }
            Free(the_data);
          }
        }

        if (p_flags & (STATS_LINE_AVG_SORTED_BY_MOD | STATS_FUNC_AVG_SORTED_BY_MOD)) {
          // cached string lengths, to avoid multiple separators after each other
          size_t line_avg_sorted_mod_str_len = mstrlen(line_avg_sorted_mod_str);
          size_t func_avg_sorted_mod_str_len = mstrlen(func_avg_sorted_mod_str);

          // cycle through the sorted statistics and gather the necessary data per module
          for (size_t i = 0; i < p_db.size(); ++i) {
            if (i > 0) {
              if ((p_flags & STATS_LINE_AVG_SORTED_BY_MOD) &&
                  line_avg_sorted_mod_str_len != mstrlen(line_avg_sorted_mod_str)) {
                line_avg_sorted_mod_str = mputstr(line_avg_sorted_mod_str,
                  "--------------------------------------------------------------\n");
                line_avg_sorted_mod_str_len = mstrlen(line_avg_sorted_mod_str);
              }
              if ((p_flags & STATS_FUNC_AVG_SORTED_BY_MOD) &&
                  func_avg_sorted_mod_str_len != mstrlen(func_avg_sorted_mod_str)) {
                func_avg_sorted_mod_str = mputstr(func_avg_sorted_mod_str,
                  "-------------------------------------------------------------\n");
                func_avg_sorted_mod_str_len = mstrlen(func_avg_sorted_mod_str);
              }
            }
            if (p_flags & STATS_LINE_AVG_SORTED_BY_MOD) {
              for (size_t j = 0; j < used_code_lines; ++j) {
                if (0 == strcmp(code_line_stats[j].filename, p_db[i].filename)) {
                  double avg = (code_line_stats[j].total_time.tv_sec +
                    code_line_stats[j].total_time.tv_usec / 1000000.0) /
                    code_line_stats[j].exec_count;
                  char* total_time_str = timeval2string(code_line_stats[j].total_time);
                  line_avg_sorted_mod_str = mputprintf(line_avg_sorted_mod_str,
                    "%.6lfs\t(%ss / %d)\t%s:%d",
                    avg, total_time_str, code_line_stats[j].exec_count,
                    code_line_stats[j].filename, code_line_stats[j].lineno);
                  Free(total_time_str);
                  if (NULL != code_line_stats[j].funcname) {
                    line_avg_sorted_mod_str = mputprintf(line_avg_sorted_mod_str,
                      " [%s]", code_line_stats[j].funcname);
                  }
                  line_avg_sorted_mod_str = mputstrn(line_avg_sorted_mod_str, "\n", 1);
                }
              }
            }
            if (p_flags & STATS_FUNC_AVG_SORTED_BY_MOD) {
              for (size_t j = 0; j < used_functions; ++j) {
                if (0 == strcmp(function_stats[j].filename, p_db[i].filename)) {
                  double avg = (function_stats[j].total_time.tv_sec +
                    function_stats[j].total_time.tv_usec / 1000000.0) /
                    function_stats[j].exec_count;
                  char* total_time_str = timeval2string(function_stats[j].total_time);
                  func_avg_sorted_mod_str = mputprintf(func_avg_sorted_mod_str,
                    "%.6lfs\t(%ss / %d)\t%s:%d [%s]\n",
                    avg, total_time_str, function_stats[j].exec_count,
                    function_stats[j].filename, function_stats[j].lineno, function_stats[j].funcname);
                  Free(total_time_str);
                }
              }
            }
          }
        }
      }

      // free the stats data
      Free(code_line_stats);
      Free(function_stats);
    }

    // add new lines at the end of each segment
    if (p_flags & STATS_NUMBER_OF_LINES) {
      line_func_count_str = mputstrn(line_func_count_str, "\n", 1);
    }
    if (p_flags & STATS_LINE_DATA_RAW) {
      line_data_str = mputstrn(line_data_str, "\n", 1);
    }
    if (p_flags & STATS_FUNC_DATA_RAW) {
      func_data_str = mputstrn(func_data_str, "\n", 1);
    }
    if (!p_disable_profiler) {
      if (p_flags & STATS_LINE_TIMES_SORTED_BY_MOD) {
        line_time_sorted_mod_str = mputstrn(line_time_sorted_mod_str, "\n", 1);
      }
      if (p_flags & STATS_LINE_TIMES_SORTED_TOTAL) {
        line_time_sorted_tot_str = mputstrn(line_time_sorted_tot_str, "\n", 1);
      }
      if (p_flags & STATS_FUNC_TIMES_SORTED_BY_MOD) {
        func_time_sorted_mod_str = mputstrn(func_time_sorted_mod_str, "\n", 1);
      }
      if (p_flags & STATS_FUNC_TIMES_SORTED_TOTAL) {
        func_time_sorted_tot_str = mputstrn(func_time_sorted_tot_str, "\n", 1);
      }
      if (p_flags & STATS_TOP10_LINE_TIMES) {
        line_time_sorted_top10_str = mputstrn(line_time_sorted_top10_str, "\n", 1);
      }
      if (p_flags & STATS_TOP10_FUNC_TIMES) {
        func_time_sorted_top10_str = mputstrn(func_time_sorted_top10_str, "\n", 1);
      }
      if (!p_disable_coverage) {
        if (p_flags & STATS_LINE_AVG_RAW) {
          line_avg_str = mputstrn(line_avg_str, "\n", 1);
        }
        if (p_flags & STATS_LINE_AVG_RAW) {
          func_avg_str = mputstrn(func_avg_str, "\n", 1);
        }
        if (p_flags & STATS_LINE_AVG_SORTED_BY_MOD) {
          line_avg_sorted_mod_str = mputstrn(line_avg_sorted_mod_str, "\n", 1);
        }
        if (p_flags & STATS_LINE_AVG_SORTED_TOTAL) {
          line_avg_sorted_tot_str = mputstrn(line_avg_sorted_tot_str, "\n", 1);
        }
        if (p_flags & STATS_FUNC_AVG_SORTED_BY_MOD) {
          func_avg_sorted_mod_str = mputstrn(func_avg_sorted_mod_str, "\n", 1);
        }
        if (p_flags & STATS_FUNC_AVG_SORTED_TOTAL) {
          func_avg_sorted_tot_str = mputstrn(func_avg_sorted_tot_str, "\n", 1);
        }
        if (p_flags & STATS_TOP10_LINE_AVG) {
          line_avg_sorted_top10_str = mputstrn(line_avg_sorted_top10_str, "\n", 1);
        }
        if (p_flags & STATS_TOP10_FUNC_AVG) {
          func_avg_sorted_top10_str = mputstrn(func_avg_sorted_top10_str, "\n", 1);
        }
      }
    }
    if (!p_disable_coverage) {
      if (p_flags & STATS_LINE_COUNT_SORTED_BY_MOD) {
        line_count_sorted_mod_str = mputstrn(line_count_sorted_mod_str, "\n", 1);
      }
      if (p_flags & STATS_LINE_COUNT_SORTED_TOTAL) {
        line_count_sorted_tot_str = mputstrn(line_count_sorted_tot_str, "\n", 1);
      }
      if (p_flags & STATS_FUNC_COUNT_SORTED_BY_MOD) {
        func_count_sorted_mod_str = mputstrn(func_count_sorted_mod_str, "\n", 1);
      }
      if (p_flags & STATS_FUNC_COUNT_SORTED_TOTAL) {
        func_count_sorted_tot_str = mputstrn(func_count_sorted_tot_str, "\n", 1);
      }
      if (p_flags & STATS_TOP10_LINE_COUNT) {
        line_count_sorted_top10_str = mputstrn(line_count_sorted_top10_str, "\n", 1);
      }
      if (p_flags & STATS_TOP10_FUNC_COUNT) {
        func_count_sorted_top10_str = mputstrn(func_count_sorted_top10_str, "\n", 1);
      }
      if (p_flags & STATS_UNUSED_LINES) {
        unused_lines_str = mputstrn(unused_lines_str, "\n", 1);
      }
      if (p_flags & STATS_UNUSED_FUNC) {
        unused_func_str = mputstrn(unused_func_str, "\n", 1);
      }
    }

    // write the statistics to the specified file
    FILE* file = fopen(p_filename, "w");
    if (NULL == file) {
      p_error_function("Could not open file '%s' for writing. Profiling and/or "
        "code coverage statistics will not be saved.", p_filename);
      return;
    }
    // by now the strings for all disabled statistics entries should be null
    fprintf(file, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"
      , title_str
      , (NULL != line_func_count_str) ? line_func_count_str : ""
      , (NULL != line_data_str) ? line_data_str : ""
      , (NULL != line_avg_str) ? line_avg_str : ""
      , (NULL != func_data_str) ? func_data_str : ""
      , (NULL != func_avg_str) ? func_avg_str : ""
      , (NULL != line_time_sorted_mod_str) ? line_time_sorted_mod_str : ""
      , (NULL != line_time_sorted_tot_str) ? line_time_sorted_tot_str : ""
      , (NULL != func_time_sorted_mod_str) ? func_time_sorted_mod_str : ""
      , (NULL != func_time_sorted_tot_str) ? func_time_sorted_tot_str : ""
      , (NULL != line_count_sorted_mod_str) ? line_count_sorted_mod_str : ""
      , (NULL != line_count_sorted_tot_str) ? line_count_sorted_tot_str : ""
      , (NULL != func_count_sorted_mod_str) ? func_count_sorted_mod_str : ""
      , (NULL != func_count_sorted_tot_str) ? func_count_sorted_tot_str : ""
      , (NULL != line_avg_sorted_mod_str) ? line_avg_sorted_mod_str : ""
      , (NULL != line_avg_sorted_tot_str) ? line_avg_sorted_tot_str : ""
      , (NULL != func_avg_sorted_mod_str) ? func_avg_sorted_mod_str : ""
      , (NULL != func_avg_sorted_tot_str) ? func_avg_sorted_tot_str : ""
      , (NULL != line_time_sorted_top10_str) ? line_time_sorted_top10_str : ""
      , (NULL != func_time_sorted_top10_str) ? func_time_sorted_top10_str : ""
      , (NULL != line_count_sorted_top10_str) ? line_count_sorted_top10_str : ""
      , (NULL != func_count_sorted_top10_str) ? func_count_sorted_top10_str : ""
      , (NULL != line_avg_sorted_top10_str) ? line_avg_sorted_top10_str : ""
      , (NULL != func_avg_sorted_top10_str) ? func_avg_sorted_top10_str : ""
      , (NULL != unused_lines_str) ? unused_lines_str : ""
      , (NULL != unused_func_str) ? unused_func_str : "");

    fclose(file);

    // free the strings
    Free(title_str);
    Free(line_func_count_str);
    Free(line_data_str);
    Free(line_avg_str);
    Free(func_data_str);
    Free(func_avg_str);
    Free(line_time_sorted_mod_str);
    Free(line_time_sorted_tot_str);
    Free(func_time_sorted_mod_str);
    Free(func_time_sorted_tot_str);
    Free(line_count_sorted_mod_str);
    Free(line_count_sorted_tot_str);
    Free(func_count_sorted_mod_str);
    Free(func_count_sorted_tot_str);
    Free(line_avg_sorted_mod_str);
    Free(line_avg_sorted_tot_str);
    Free(func_avg_sorted_mod_str);
    Free(func_avg_sorted_tot_str);
    Free(line_time_sorted_top10_str);
    Free(func_time_sorted_top10_str);
    Free(line_count_sorted_top10_str);
    Free(func_count_sorted_top10_str);
    Free(line_avg_sorted_top10_str);
    Free(func_avg_sorted_top10_str);
    Free(unused_lines_str);
    Free(unused_func_str);
  }
  
} // namespace
