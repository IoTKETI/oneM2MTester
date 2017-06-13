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
 *   Bene, Tamas
 *   Beres, Szabolcs
 *   Czimbalmos, Eduard
 *   Delic, Adam
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
//
// Description:           Implementation file for main
// Author:                Vilmos Varga
// mail:                  ethvva@eth.ericsson.se
//
// Copyright (c) 2000-2017 Ericsson Telecom AB
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>

// STL include stuff
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

#include "../../common/memory.h"
#include "../../common/path.h"
#include "../../common/version_internal.h"

#ifdef LICENSE
#include "../../common/license.h"
#endif

#include "../../common/openssl_version.h"

#include "MainController.h"

#if defined CLI
#include "../cli/Cli.h"
#else 
#error "CLI must be defined"
#endif

extern int config_read_debug;

/* ASCII ART STUFF */

std::vector<std::string> split(const char separator, const std::string& source_str, bool drop_empty=true)
{
  std::vector<std::string> str_vec;
  const char* begin_pos = source_str.data();
  const char* end_pos = begin_pos + source_str.length(); // points behind the last char
  for (const char* act_pos = begin_pos; act_pos<end_pos; act_pos++)
  {
    if ( *act_pos==separator )
    {
      if (!drop_empty || (act_pos>begin_pos))
        str_vec.push_back(std::string(begin_pos, act_pos-begin_pos));
      begin_pos = act_pos+1;
    }
  }
  if (!drop_empty || (end_pos>begin_pos)) str_vec.push_back(std::string(begin_pos, end_pos-begin_pos));
  return str_vec;
}

std::vector<std::string> get_directory_files(const std::string& directory)
{
  std::vector<std::string> file_list;
  DIR *dir = opendir(directory.c_str());
  if (dir==NULL) return file_list;
  struct dirent *ent;
  while ((ent=readdir(dir))!=NULL) {
    std::string file_name = ent->d_name;
    if (get_path_status((directory+"/"+file_name).c_str())==PS_FILE) file_list.push_back(file_name);
  }
  closedir(dir);
  return file_list;
}

void print_ascii_art(const std::string& file_path)
{
  try {
    std::ifstream ifs(file_path.c_str(), std::ifstream::in);
    while (ifs.good()) {
      int c = (char)ifs.get();
      if (c<0) c = '\n';
      std::cout << (char)c;
    }
    ifs.close();
  }
  catch (...) { /* stfu */ }
}

struct current_condition_data
{
  std::string user;
  struct tm time;
  current_condition_data();
};
current_condition_data::current_condition_data()
{
  const char* user_name = getlogin(); // TODO: use getpwuid(getuid())
  user = user_name ? std::string(user_name) : "";
  time_t rawtime;
  ::time(&rawtime);
  struct tm * curr_tm = localtime(&rawtime);
  time = *curr_tm;
}

typedef bool (*condition_function)(const std::string& value, const current_condition_data& curr_data);
typedef std::map<std::string,condition_function> condition_map;

bool condition_user(const std::string& value, const current_condition_data& curr_data)
{
  return value==curr_data.user;
}

// sets the interval or returns false if invalid input
bool get_interval(const std::string& value, int& a, int& b)
{
  std::vector<std::string> nums = split('_', value);
  switch (nums.size()) {
  case 1:
    a = b = atoi(nums[0].c_str());
    return true;
  case 2:
    a = atoi(nums[0].c_str());
    b = atoi(nums[1].c_str());
    return true;
  default: ;
  }
  return false;
}

bool is_in_interval(const std::string& interval, int i)
{
  int a,b;
  if (!get_interval(interval, a, b)) return false;
  return a<=i && i<=b;
}

bool condition_weekday(const std::string& value, const current_condition_data& curr_data)
{
  return is_in_interval(value, curr_data.time.tm_wday ? curr_data.time.tm_wday : 7); // 1 - 7
}

bool condition_day(const std::string& value, const current_condition_data& curr_data)
{
  return is_in_interval(value, curr_data.time.tm_mday); // 1 - 31
}

bool condition_month(const std::string& value, const current_condition_data& curr_data)
{
  return is_in_interval(value, curr_data.time.tm_mon+1); // 1 - 12
}

bool condition_year(const std::string& value, const current_condition_data& curr_data)
{
  return is_in_interval(value, curr_data.time.tm_year+1900);
}

bool condition_hour(const std::string& value, const current_condition_data& curr_data)
{
  return is_in_interval(value, curr_data.time.tm_hour); // 0 - 23
}

bool condition_minute(const std::string& value, const current_condition_data& curr_data)
{
  return is_in_interval(value, curr_data.time.tm_min); // 0 - 59
}

bool condition_second(const std::string& value, const current_condition_data& curr_data)
{
  return is_in_interval(value, curr_data.time.tm_sec); // 0 - 61
}

condition_map create_condition_map()
{
  condition_map cond_map;
  cond_map.insert(make_pair("user",condition_user));
  cond_map.insert(make_pair("weekday",condition_weekday));
  cond_map.insert(make_pair("day",condition_day));
  cond_map.insert(make_pair("month",condition_month));
  cond_map.insert(make_pair("year",condition_year));
  cond_map.insert(make_pair("hour",condition_hour));
  cond_map.insert(make_pair("minute",condition_minute));
  cond_map.insert(make_pair("second",condition_second));
  return cond_map;
}

// display random ascii art, if there was an error don't report it
void display_ascii_art()
{
  std::string asciiart_dir;
  const char* ttcn3_asciiart_dir = getenv("TTCN3_ASCIIART_DIR");
  if (ttcn3_asciiart_dir!=NULL) {
    if (strlen(ttcn3_asciiart_dir)==0) return;
    asciiart_dir = ttcn3_asciiart_dir;
  } else {
    const char* ttcn3_dir = getenv("TTCN3_DIR");
    asciiart_dir = ttcn3_dir ? ttcn3_dir : "";
    asciiart_dir += (asciiart_dir.size()>0 && asciiart_dir[asciiart_dir.size()-1]=='/') ? "" : "/";
    asciiart_dir += "etc/asciiart";
  }
  if (get_path_status(asciiart_dir.c_str())!=PS_DIRECTORY) return;
  std::vector<std::string> asciiart_files = get_directory_files(asciiart_dir);
  std::vector<std::string> normal_active_files;  // files that don't have special filter
  std::vector<std::string> special_active_files; // files that have special filter
  condition_map cond_map = create_condition_map();
  current_condition_data curr_data; // calculate current data for conditions only once, before entering loops
  for (std::vector<std::string>::iterator file_name_i = asciiart_files.begin(); file_name_i<asciiart_files.end(); file_name_i++) {
    std::vector<std::string> file_name_parts = split('.', *file_name_i);
    bool is_special = false;
    bool is_active = true;
    for (std::vector<std::string>::iterator name_part_i = file_name_parts.begin(); name_part_i<file_name_parts.end(); name_part_i++) {
      std::vector<std::string> condition = split('-', *name_part_i);
      if (condition.size()==2) {
        const std::string& cond_name = condition[0];
        const std::string& cond_value = condition[1];
        condition_map::iterator cond_iter = cond_map.find(cond_name);
        if (cond_iter!=cond_map.end()) {
          is_special = true;
          is_active = is_active && (cond_iter->second)(cond_value, curr_data);
        }
      }
    }
    if (is_active) {
      if (is_special) special_active_files.push_back(*file_name_i);
      else normal_active_files.push_back(*file_name_i);
    }
  }
  srand(time(NULL));
  if (special_active_files.size()>0) {
    print_ascii_art(asciiart_dir + "/" + special_active_files[rand()%special_active_files.size()]);
  } else if (normal_active_files.size()>0) {
    print_ascii_art(asciiart_dir + "/" + normal_active_files[rand()%normal_active_files.size()]);
  }
}

/* Main function */

using namespace mctr;

int main(int argc, char *argv[])
{
#ifndef NDEBUG
  config_read_debug = (getenv("DEBUG_CONFIG") != NULL);
#endif

  display_ascii_art();

  if (argc == 2 && !strcmp(argv[1], "-v")) {
    fputs("Main Controller "
#if defined CLI
      "(CLI)"
#endif
      " for the TTCN-3 Test Executor\n"
      "Product number: " PRODUCT_NUMBER "\n"
      "Build date: " __DATE__ " " __TIME__ "\n"
      "Compiled with: " C_COMPILER_VERSION "\n", stderr);
    fputs("Using ", stderr);
    fputs(openssl_version_str(), stderr);
    fputs("\n\n" COPYRIGHT_STRING "\n\n", stderr);
#ifdef LICENSE
    print_license_info();
    fputs("\n\n", stderr);
#endif
    return 0;
  }

  int max_ptcs;
#ifdef LICENSE
  license_struct lstr;
  init_openssl();
  load_license(&lstr);
  if (!verify_license(&lstr)) {
    free_license(&lstr);
    free_openssl();
    exit(EXIT_FAILURE);
  }
  if (!check_feature(&lstr, FEATURE_MCTR)) {
    fputs("The license key does not allow the starting of TTCN-3 "
      "Main Controller.\n", stderr);
    return 2;
  }
  max_ptcs = lstr.max_ptcs;
  free_license(&lstr);
  free_openssl();
#else
  max_ptcs = -1;
#endif
  int ret_val;
  {
#if defined CLI
    cli::Cli
#endif
    userInterface;

    userInterface.initialize();
    MainController::initialize(userInterface, max_ptcs);

    ret_val = userInterface.enterLoop(argc, argv);

    MainController::terminate();
  }
  check_mem_leak(argv[0]);

  return ret_val;
}

// Local Variables:
// mode: C++
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
