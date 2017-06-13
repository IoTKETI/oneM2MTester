/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef MODULE_LIST_HH
#define MODULE_LIST_HH

#include <stdio.h>
#include "Types.h"
#ifdef USAGE_STATS
#include <pthread.h>
#endif

class Text_Buf;
class TTCN_Module;
class Module_Param;
class Module_Param_Name;
class ModuleVersion;
struct namespace_t;
#ifdef USAGE_STATS
struct thread_data;
#endif

typedef void (*genericfunc_t)(void);

class Module_List {
  static TTCN_Module *list_head, *list_tail;

public:
  static void add_module(TTCN_Module *module_ptr);
  static void remove_module(TTCN_Module *module_ptr);
  static TTCN_Module *lookup_module(const char *module_name);
  static TTCN_Module *single_control_part();

  static void pre_init_modules();
  static void post_init_modules();

  static void start_function(const char *module_name,
    const char *function_name, Text_Buf& function_arguments);

  static void initialize_component(const char *module_name,
    const char *component_type, boolean init_base_comps);

  static void set_param(Module_Param& param);
#ifdef TITAN_RUNTIME_2
  static Module_Param* get_param(Module_Param_Name& param_name,
    const Module_Param* caller);
#endif
  static void log_param();

  static void execute_control(const char *module_name);
  static void execute_testcase(const char *module_name,
    const char *testcase_name);
  static void execute_all_testcases(const char *module_name);

  static void print_version();
  
#ifdef USAGE_STATS
  static void send_usage_stats(pthread_t& thread, thread_data*& data);
  static void clean_up_usage_stats(pthread_t thread, thread_data* data);
#endif
  static void list_testcases();
  static void push_version(Text_Buf& text_buf);

  static void encode_function(Text_Buf& text_buf,
    genericfunc_t function_address);
  static void decode_function(Text_Buf& text_buf,
    genericfunc_t *function_addr_ptr);
  static void log_function(genericfunc_t function_address);
  static void encode_altstep(Text_Buf& text_buf,
    genericfunc_t altstep_address);
  static void decode_altstep(Text_Buf& text_buf,
    genericfunc_t *altstep_addr_ptr);
  static void log_altstep(genericfunc_t altstep_address);
  static void encode_testcase(Text_Buf& text_buf,
    genericfunc_t testcase_address);
  static void decode_testcase(Text_Buf& text_buf,
    genericfunc_t *testcase_addr_ptr);
  static void log_testcase(genericfunc_t testcase_address);

  static genericfunc_t get_fat_null();

  static genericfunc_t lookup_start_by_function_address(
    genericfunc_t function_address);
  static genericfunc_t lookup_standalone_address_by_altstep_address(
    genericfunc_t altstep_address);
  static genericfunc_t lookup_activate_address_by_altstep_address(
    genericfunc_t altstep_address);

private:
  static boolean lookup_function_by_address(genericfunc_t function_address,
    const char*& module_name, const char*& function_name);
  static boolean lookup_altstep_by_address(genericfunc_t altstep_address,
    const char*& module_name, const char*& altstep_name);
  static boolean lookup_testcase_by_address(genericfunc_t testcase_address,
    const char*& module_name, const char*& testcase_name);
};

class TTCN_Module {
  friend class Module_List;
public:
  enum module_type_enum { TTCN3_MODULE, ASN1_MODULE, CPLUSPLUS_MODULE };
  typedef void (*init_func_t)();
  typedef boolean (*set_param_func_t)(Module_Param& param);
  typedef Module_Param* (*get_param_func_t)(Module_Param_Name& param_name);
  typedef void (*log_param_func_t)();
  typedef boolean (*initialize_component_func_t)(const char *component_type,
    boolean init_base_comps);
  typedef boolean (*start_func_t)(const char *function_name,
    Text_Buf& function_arguments);
  typedef void (*control_func_t)();
  typedef verdicttype (*testcase_t)(boolean has_timer, double timer_value);

private:
  TTCN_Module *list_prev, *list_next;
  module_type_enum module_type;
  const char *module_name;
  const char *compilation_date;
  const char *compilation_time;
  const unsigned char *md5_checksum;
  const char* product_number;
  unsigned int suffix;
  unsigned int release;
  unsigned int patch;
  unsigned int build;
  const char* extra;
  size_t num_namespaces;
  const namespace_t *xer_namespaces;
  // FIXME instead of each module having its own list of namespaces with gaps,
  // it may be better to have one global list in e.g. Module_list.
  // Trouble is, Module_list is not really a class: there is no instance of it
  // and it only has static members.
  init_func_t pre_init_func, post_init_func;
  boolean pre_init_called, post_init_called;
  set_param_func_t set_param_func;
  get_param_func_t get_param_func;
  log_param_func_t log_param_func;
  initialize_component_func_t initialize_component_func;
  start_func_t start_func;
  control_func_t control_func;
  struct function_list_item;
  function_list_item *function_head, *function_tail;
  struct altstep_list_item;
  altstep_list_item *altstep_head, *altstep_tail;
  struct testcase_list_item;
  testcase_list_item *testcase_head, *testcase_tail;

  /// Copy constructor disabled
  TTCN_Module(const TTCN_Module&);
  /// Assignment disabled
  TTCN_Module& operator=(const TTCN_Module&);
public:
  TTCN_Module(const char *par_module_name,
    const char *par_compilation_date,
    const char *par_compilation_time,
    const unsigned char *par_md5_checksum,
    init_func_t par_pre_init_func,
    const char* par_product_number,
    unsigned int par_suffix,
    unsigned int par_release,
    unsigned int par_patch,
    unsigned int par_build,
    const char* par_extra,
    size_t par_num_namespace,
    const namespace_t *par_namespaces,
    init_func_t par_post_init_func,
    set_param_func_t par_set_param_func,
    get_param_func_t par_get_param_func,
    log_param_func_t par_log_param_func,
    initialize_component_func_t par_initialize_component_func,
    start_func_t par_start_func,
    control_func_t par_control_func);
  TTCN_Module(const char *par_module_name,
    const char *par_compilation_date,
    const char *par_compilation_time,
    const unsigned char *par_md5_checksum,
    init_func_t par_init_func);
  TTCN_Module(const char *par_module_name,
    const char *par_compilation_date,
    const char *par_compilation_time,
    init_func_t par_init_func = NULL);
  ~TTCN_Module();
  const char *get_name() const { return module_name; }

  void pre_init_module();
  void post_init_module();

  void add_function(const char *function_name, genericfunc_t function_address,
    genericfunc_t start_address);
  void add_altstep(const char *altstep_name, genericfunc_t altstep_address,
    genericfunc_t activate_address, genericfunc_t standalone_address);
  void add_testcase_nonpard(const char *testcase_name,
    testcase_t testcase_function);
  void add_testcase_pard(const char *testcase_name,
    genericfunc_t testcase_address);

  void execute_testcase(const char *testcase_name);
  void execute_all_testcases();

  const char *get_function_name_by_address(genericfunc_t function_address);
  genericfunc_t get_function_address_by_name(const char *function_name);
  genericfunc_t get_function_start_by_address(genericfunc_t function_address);
  const char *get_altstep_name_by_address(genericfunc_t altstep_address);
  genericfunc_t get_altstep_address_by_name(const char *altstep_name);
  boolean get_altstep_data_by_address(genericfunc_t altstep_address,
    genericfunc_t& standalone_address, genericfunc_t& activate_address);
  const char *get_testcase_name_by_address(genericfunc_t testcase_address);
  genericfunc_t get_testcase_address_by_name(const char *testcase_name);

  void print_version();
  ModuleVersion* get_version() const;
  void list_testcases();
  size_t get_num_ns() const { return num_namespaces; }
  const namespace_t *get_ns(size_t p_index) const;
  const namespace_t *get_controlns() const;
};

#endif
