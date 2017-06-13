/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *   Szalai, Gabor
 *
 ******************************************************************************/
#ifndef CONFIG_DATA_H_
#define CONFIG_DATA_H_

typedef struct {
  char *module_name;
  char *testcase_name;
} execute_list_item;

typedef struct {
  char *group_name;
  char *host_name;
} group_item;

typedef struct {
  char *host_or_group;
  char *component;
} component_item;

typedef enum {  TSF_NONE=0,
                TSF_TIME=1,
                TSF_DATE_TIME=2,
                TSF_SEC=3
} cf_timestamp_format;

struct config_data {
  config_data()
  : config_read_buffer(0)
  , log_file_name(0)
  , execute_list(0), execute_list_len(0)
  , group_list(0), group_list_len(0)
  , component_list(0), component_list_len(0)
  , local_addr(0)
  , tcp_listen_port(0)
  , num_hcs(-1)
  , kill_timer(10.0)
  , unix_sockets_enabled(
#ifdef WIN32
  false // Unix domain socket communication on Cygwin is painfully slow
#else
  true
#endif
    )
  , tsformat(TSF_NONE)
  {}

  ~config_data() { clear(); }
  void clear();

  /** Add an entry from the [GROUPS] section.
   *
   * @param group_name; the function takes a copy of it
   * @param host_name ; the function takes a copy of it
   */
  void add_host(char *group_name, char *host_name);

  /** Add an entry from the [COMPONENTS] section.
   *
   * @param host_or_group; the function takes ownership of it
   * @param component_id ; the function takes ownership of it
   */
  void add_component(char *host_or_group, char *component_id);

  /** Add an entry from the [EXECUTE] section
   *
   * @param exec_item, a struct with two strings;
   * the function takes ownership
   */
  void add_exec(const execute_list_item& exec_item);

  /** Set the log file name
   *
   * @param f file name skeleton; the function takes ownership
   */
  void set_log_file(char *f);

  char *config_read_buffer; // really an expstring_t
  char *log_file_name;

  execute_list_item *execute_list;
  int execute_list_len;

  group_item *group_list;
  int group_list_len;

  component_item *component_list;
  int component_list_len;

  char *local_addr;
  unsigned short tcp_listen_port;
  int num_hcs; // def -1
  double kill_timer;
  bool unix_sockets_enabled; // def false on Cygwin. Keep last if possible
  cf_timestamp_format tsformat;
};

/** Process config file
 *
 * @param file_name name of the file
 * @param config struct for parsed values
 * @return 0 on success, -1 on error
 */
extern int process_config_read_file(const char *file_name, config_data* config);

#endif /* CONFIG_DATA_H_ */
