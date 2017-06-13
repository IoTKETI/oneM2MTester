/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include "config_data.h"
#include "../../common/memory.h"

void config_data::add_host(char *group_name, char *host_name)
{
  group_list = (group_item*)Realloc(group_list,
    ++group_list_len * sizeof(group_item));
  // We take copies because the same group_name pointer may be supplied
  // more than once (group:host is a one-to-many relationship).
  // This would result in double-free.
  // Copying the host_name is not strictly necessary; done for symmetry.
  group_list[group_list_len-1].group_name = mcopystr(group_name);
  // We need NULL here, not empty string.
  group_list[group_list_len-1].host_name  = host_name ? mcopystr(host_name) : NULL;
}

void config_data::add_component(char *host_or_grp, char *comp)
{
  component_list = (component_item*)Realloc(component_list,
    ++component_list_len * sizeof(component_item));
  component_list[component_list_len-1].host_or_group = host_or_grp;
  component_list[component_list_len-1].component     = comp;
}

void config_data::add_exec(const execute_list_item& exec_item)
{
  execute_list = (execute_list_item *)Realloc(execute_list,
    (execute_list_len + 1) * sizeof(*execute_list));
  execute_list[execute_list_len++] = exec_item;
}

void config_data::set_log_file(char *f)
{
  if (log_file_name != NULL)
	Free(log_file_name);
  log_file_name = f;
}

void config_data::clear()
{
  Free(config_read_buffer);
  config_read_buffer = NULL;

  Free(log_file_name);
  log_file_name = NULL;

  for(int r = 0; r < execute_list_len; r++) {
    Free(execute_list[r].module_name);
    Free(execute_list[r].testcase_name);
  }
  Free(execute_list);
  execute_list = NULL;
  execute_list_len = 0;

  for(int r = 0; r < group_list_len; r++) {
    Free(group_list[r].group_name);
    Free(group_list[r].host_name);
  }
  Free(group_list);
  group_list = NULL;
  group_list_len = 0;

  for(int r = 0; r < component_list_len; r++) {
    Free(component_list[r].host_or_group);
    Free(component_list[r].component);
  }
  Free(component_list);
  component_list = NULL;
  component_list_len = 0;

  Free(local_addr);
  local_addr = NULL;

  tcp_listen_port = 0;
  num_hcs = -1;
  kill_timer = 10.0;
  unix_sockets_enabled =
#ifdef WIN32
  false // Unix domain socket communication on Cygwin is painfully slow
#else
  true
#endif
  ;
}
