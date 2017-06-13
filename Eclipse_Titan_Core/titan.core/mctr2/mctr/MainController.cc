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
 *   Baranyi, Botond
 *   Bene, Tamas
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
//
// Description:           Implementation file for MainController
// Author:                Janos Zoltan Szabo
// mail:                  tmpjsz@eth.ericsson.se
//
// Copyright (c) 2000-2017 Ericsson Telecom AB
//
//----------------------------------------------------------------------------

#include "MainController.h"
#include "UserInterface.h"

#include "../../common/memory.h"
#include "../../common/version.h"
#include "../../common/version_internal.h"
#include "../../core/Message_types.hh"
#include "../../core/Error.hh"
#include "../../core/Textbuf.hh"
#include "../../core/Logger.hh"
#include "DebugCommands.hh"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifdef USE_EPOLL
#include <sys/epoll.h>
#else
#include <sys/poll.h>
#endif
#include <sys/utsname.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <signal.h>

reffer::reffer(const char*) {}

//----------------------------------------------------------------------------

namespace mctr {

//----------------------------------------------------------------------------

/* Static variables */

UserInterface *MainController::ui;
NetworkHandler MainController::nh;

mc_state_enum MainController::mc_state;
char *MainController::mc_hostname;

struct sigaction MainController::new_action, MainController::old_action;

int MainController::server_fd;
int MainController::server_fd_unix = -1;
boolean MainController::server_fd_disabled;

debugger_settings_struct MainController::debugger_settings;
debug_command_struct MainController::last_debug_command;

void MainController::disable_server_fd()
{
  if (!server_fd_disabled) {
    remove_poll_fd(server_fd);
    server_fd_disabled = TRUE;
  }
}

void MainController::enable_server_fd()
{
  if (server_fd_disabled) {
    add_poll_fd(server_fd);
    server_fd_disabled = FALSE;
  }
}

pthread_mutex_t MainController::mutex;

void MainController::lock()
{
  int result = pthread_mutex_lock(&mutex);
  if (result > 0) {
    fatal_error("MainController::lock: "
      "pthread_mutex_lock failed with code %d.", result);
  }
}

void MainController::unlock()
{
  int result = pthread_mutex_unlock(&mutex);
  if (result > 0) {
    fatal_error("MainController::unlock: "
      "pthread_mutex_unlock failed with code %d.", result);
  }
}

#ifdef USE_EPOLL
epoll_event *MainController::epoll_events;
int MainController::epfd;
#else
unsigned int MainController::nfds, MainController::new_nfds;
struct pollfd *MainController::ufds, *MainController::new_ufds;
boolean MainController::pollfds_modified;
#endif //USE_EPOLL

#ifdef USE_EPOLL
void MainController::add_poll_fd(int fd)
{
  if (fd < 0) return;
  epoll_event event;
  memset(&event,0,sizeof(event));
  event.events = EPOLLIN;
  event.data.fd = fd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) < 0)
    fatal_error("MainController::add_poll_fd: system call epoll_ctl"
      " failed on file descriptor %d.", fd);
}

void MainController::remove_poll_fd(int fd)
{
  if (fd < 0) return;
  epoll_event event;
  memset(&event,0,sizeof(event));
  event.events = EPOLLIN;
  event.data.fd = fd;
  if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event) < 0)
    fatal_error("MainController::remove_poll_fd: system call epoll_ctl"
      " failed on file descriptor %d.", fd);
}
#else // ! defined USE_EPOLL
void MainController::add_poll_fd(int fd)
{
  if (fd < 0) return;
  if (pollfds_modified) {
    int i;
    for (i = new_nfds - 1; i >= 0; i--) {
      if (new_ufds[i].fd < fd) break;
      else if (new_ufds[i].fd == fd) return;
    }
    i++;
    new_ufds = (struct pollfd*)Realloc(new_ufds, (new_nfds + 1) *
      sizeof(struct pollfd));
    memmove(new_ufds + i + 1, new_ufds + i, (new_nfds - i) *
      sizeof(struct pollfd));
    new_ufds[i].fd = fd;
    new_ufds[i].events = POLLIN;
    new_ufds[i].revents = 0;
    new_nfds++;
  } else {
    int i;
    for (i = nfds - 1; i >= 0; i--) {
      if (ufds[i].fd < fd) break;
      else if (ufds[i].fd == fd) return;
    }
    i++;
    new_nfds = nfds + 1;
    new_ufds = (struct pollfd*)Malloc(new_nfds * sizeof(struct pollfd));
    memcpy(new_ufds, ufds, i * sizeof(struct pollfd));
    new_ufds[i].fd = fd;
    new_ufds[i].events = POLLIN;
    new_ufds[i].revents = 0;
    memcpy(new_ufds + i + 1, ufds + i, (nfds - i) * sizeof(struct pollfd));
    pollfds_modified = TRUE;
  }
}

void MainController::remove_poll_fd(int fd)
{
  if (fd < 0) return;
  if (pollfds_modified) {
    int i;
    for (i = new_nfds - 1; i >= 0; i--) {
      if (new_ufds[i].fd == fd) break;
      else if (new_ufds[i].fd < fd) return;
    }
    if (i < 0) return;
    new_nfds--;
    memmove(new_ufds + i, new_ufds + i + 1, (new_nfds - i) *
      sizeof(struct pollfd));
    new_ufds = (struct pollfd*)Realloc(new_ufds, new_nfds *
      sizeof(struct pollfd));
  } else {
    int i;
    for (i = nfds - 1; i >= 0; i--) {
      if (ufds[i].fd == fd) break;
      else if (ufds[i].fd < fd) return;
    }
    if (i < 0) return;
    new_nfds = nfds - 1;
    new_ufds = (struct pollfd*)Malloc(new_nfds * sizeof(struct pollfd));
    memcpy(new_ufds, ufds, i * sizeof(struct pollfd));
    memcpy(new_ufds + i, ufds + i + 1, (new_nfds - i) *
      sizeof(struct pollfd));
    pollfds_modified = TRUE;
  }
}

void MainController::update_pollfds()
{
  if (pollfds_modified) {
    nfds = new_nfds;
    new_nfds = 0;
    Free(ufds);
    ufds = new_ufds;
    new_ufds = NULL;
    pollfds_modified = FALSE;
  }
}
#endif //USE_EPOLL

int MainController::fd_table_size;
fd_table_struct *MainController::fd_table;

void MainController::add_fd_to_table(int fd)
{
  if (fd >= fd_table_size) {
    fd_table = (fd_table_struct *)Realloc(fd_table, (fd + 1) *
      sizeof(fd_table_struct));
    for (int i = fd_table_size; i <= fd; i++) {
      fd_table[i].fd_type = FD_UNUSED;
      fd_table[i].dummy_ptr = NULL;
    }
    fd_table_size = fd + 1;
  }
}

void MainController::remove_fd_from_table(int fd)
{
  if (fd < fd_table_size) {
    fd_table[fd].fd_type = FD_UNUSED;
    int i;
    for (i = fd_table_size - 1; i >= 0 ; i--) {
      if (fd_table[i].fd_type != FD_UNUSED) break;
    }
    if (i < fd_table_size - 1) {
      fd_table_size = i + 1;
      fd_table = (fd_table_struct *)Realloc(fd_table, fd_table_size *
        sizeof(fd_table_struct));
    }
  }
}

void MainController::set_close_on_exec(int fd)
{
  int flags = fcntl(fd, F_GETFD);
  if (flags < 0) fatal_error("MainController::set_close_on_exec: system call "
    "fcntl(F_GETFD) failed on file descriptor %d.", fd);

  flags |= FD_CLOEXEC;

  if (fcntl(fd, F_SETFD, flags) == -1)
    fatal_error("MainController::set_close_on_exec: system call "
      "fcntl(F_SETFD) failed on file descriptor %d.", fd);
}

unknown_connection *MainController::unknown_head, *MainController::unknown_tail;

unknown_connection *MainController::new_unknown_connection(
  bool unix_socket)
{
  unknown_connection *conn = new unknown_connection;
  conn->unix_socket = unix_socket;
  conn->prev = unknown_tail;
  conn->next = NULL;
  if (unknown_tail != NULL) unknown_tail->next = conn;
  else unknown_head = conn;
  unknown_tail = conn;
  return conn;
}

void MainController::delete_unknown_connection(unknown_connection *conn)
{
  if (conn->prev != NULL) conn->prev->next = conn->next;
  else unknown_head = conn->next;
  if (conn->next != NULL) conn->next->prev = conn->prev;
  else unknown_tail = conn->prev;
  delete conn;
}

void MainController::close_unknown_connection(unknown_connection *conn)
{
  remove_poll_fd(conn->fd);
  close(conn->fd);
  remove_fd_from_table(conn->fd);
  delete conn->text_buf;
  delete_unknown_connection(conn);
  enable_server_fd();
}

void MainController::init_string_set(string_set *set)
{
  set->n_elements = 0;
  set->elements = NULL;
}

void MainController::free_string_set(string_set *set)
{
  for (int i = 0; i < set->n_elements; i++) Free(set->elements[i]);
  Free(set->elements);
  set->n_elements = 0;
  set->elements = NULL;
}

void MainController::add_string_to_set(string_set *set, const char *str)
{
  int i;
  for (i = 0; i < set->n_elements; i++) {
    int result = strcmp(set->elements[i], str);
    if (result > 0) break;
    else if (result == 0) return;
  }
  set->elements = static_cast<char**>( Realloc(set->elements,
    (set->n_elements + 1) * sizeof(*set->elements)) );
  memmove(set->elements + i + 1, set->elements + i,
    (set->n_elements - i) * sizeof(*set->elements));
  set->elements[i] = mcopystr(str);
  set->n_elements++;
}

void MainController::remove_string_from_set(string_set *set,
  const char *str)
{
  for (int i = 0; i < set->n_elements; i++) {
    int result = strcmp(set->elements[i], str);
    if (result < 0) continue;
    else if (result == 0) {
      Free(set->elements[i]);
      set->n_elements--;
      memmove(set->elements + i, set->elements + i + 1,
        (set->n_elements - i) * sizeof(*set->elements));
      set->elements = static_cast<char**>( Realloc(set->elements,
        set->n_elements * sizeof(*set->elements)) );
    }
    return;
  }
}

boolean MainController::set_has_string(const string_set *set,
  const char *str)
{
  if(str == NULL) return FALSE;
  for (int i = 0; i < set->n_elements; i++) {
    int result = strcmp(set->elements[i], str);
    if (result == 0) return TRUE;
    else if (result > 0) break;
  }
  return FALSE;
}

const char *MainController::get_string_from_set(const string_set *set,
  int index)
{
  if (index >= 0 && index < set->n_elements) return set->elements[index];
  else return NULL;
}

int MainController::n_host_groups;
host_group_struct *MainController::host_groups;
string_set MainController::assigned_components;
boolean MainController::all_components_assigned;

host_group_struct *MainController::add_host_group(const char *group_name)
{
  int i;
  for (i = 0 ; i < n_host_groups; i++) {
    host_group_struct *group = host_groups + i;
    int result = strcmp(group->group_name, group_name);
    if (result > 0) break;
    else if (result == 0) return group;
  }
  host_groups = (host_group_struct*)Realloc(host_groups,
    (n_host_groups + 1) * sizeof(*host_groups));
  host_group_struct *new_group = host_groups + i;
  memmove(new_group + 1, new_group,
    (n_host_groups - i) * sizeof(*host_groups));
  new_group->group_name = mcopystr(group_name);
  new_group->has_all_hosts = FALSE;
  new_group->has_all_components = FALSE;
  init_string_set(&new_group->host_members);
  init_string_set(&new_group->assigned_components);
  n_host_groups++;
  return new_group;
}

host_group_struct *MainController::lookup_host_group(const char *group_name)
{
  for (int i = 0; i < n_host_groups; i++) {
    host_group_struct *group = host_groups + i;
    int result = strcmp(group->group_name, group_name);
    if (result == 0) return group;
    else if (result > 0) break;
  }
  return NULL;
}

boolean MainController::is_similar_hostname(const char *host1,
  const char *host2)
{
  for (size_t i = 0; ; i++) {
    unsigned char c1 = host1[i], c2 = host2[i];
    if (c1 == '\0') {
      // if host2 is the longer one it may contain an additional domain
      // name with a leading dot (e.g. "foo" is similar to "foo.bar.com")
      // note: empty string is similar with empty string only
      if (c2 == '\0' || (i > 0 && c2 == '.')) return TRUE;
      else return FALSE;
    } else if (c2 == '\0') {
      // or vice versa
      if (c1 == '\0' || (i > 0 && c1 == '.')) return TRUE;
      else return FALSE;
    } else {
      // case insensitive comparison of the characters
      if (tolower(c1) != tolower(c2)) return FALSE;
      // continue the evaluation if they are matching
    }
  }
}

boolean MainController::host_has_name(const host_struct *host, const char *name)
{
  // name might resemble to host->hostname
  if (is_similar_hostname(host->hostname, name)) return TRUE;
  // to avoid returning true in situations when name is "foo.bar.com", but
  // host->hostname is "foo.other.com" and host->hostname_local is "foo"
  // name might resemble to host->hostname_local
  if (host->local_hostname_different &&
    is_similar_hostname(host->hostname_local, name)) return TRUE;
  // name might be an IP address or a DNS alias
  IPAddress *ip_addr = IPAddress::create_addr(nh.get_family());
  if (ip_addr->set_addr(name)) {
    // check if IP addresses match
    if (*ip_addr == *(host->ip_addr)) {
      delete ip_addr;
      return TRUE;
    }
    // try to handle such strange situations when the local hostname is
    // mapped to a loopback address by /etc/hosts
    // (i.e. host->ip_addr contains 127.x.y.z)
    // but the given name contains the real IP address of the host
    const char *canonical_name = ip_addr->get_host_str();
    if (is_similar_hostname(host->hostname, canonical_name)) {
      delete ip_addr;
      return TRUE;
    }
    if (host->local_hostname_different &&
      is_similar_hostname(host->hostname_local, canonical_name)) {
      delete ip_addr;
      return TRUE;
    }
  }
  delete ip_addr;
  return FALSE;
}

boolean MainController::member_of_group(const host_struct *host,
  const host_group_struct *group)
{
  if (group->has_all_hosts) return TRUE;
  for (int i = 0; ; i++) {
    const char *member_name = get_string_from_set(&group->host_members, i);
    if (member_name != NULL) {
      if (host_has_name(host, member_name)) return TRUE;
    } else if (i == 0) {
      // empty group: the group name is considered as a hostname
      return host_has_name(host, group->group_name);
    } else {
      // no more members
      break;
    }
  }
  return FALSE;
}

void MainController::add_allowed_components(host_struct *host)
{
  init_string_set(&host->allowed_components);
  host->all_components_allowed = FALSE;
  for (int i = 0; i < n_host_groups; i++) {
    host_group_struct *group = host_groups + i;
    if (!member_of_group(host, group)) continue;
    for (int j = 0; ; j++) {
      const char *component_id =
        get_string_from_set(&group->assigned_components, j);
      if (component_id == NULL) break;
      add_string_to_set(&host->allowed_components, component_id);
    }
    if (group->has_all_components) host->all_components_allowed = TRUE;
  }
}

host_struct *MainController::choose_ptc_location(const char *component_type,
  const char *component_name, const char *component_location)
{
  host_struct *best_candidate = NULL;
  int load_on_best_candidate = 0;
  boolean has_constraint =
    set_has_string(&assigned_components, component_type) ||
    set_has_string(&assigned_components, component_name);
  host_group_struct *group;
  if (component_location != NULL)
    group = lookup_host_group(component_location);
  else group = NULL;
  for (int i = 0; i < n_hosts; i++) {
    host_struct *host = hosts[i];
    if (host->hc_state != HC_ACTIVE) continue;
    if (best_candidate != NULL &&
      host->n_active_components >= load_on_best_candidate) continue;
    if (component_location != NULL) {
      // the explicit location has precedence over the constraints
      if (group != NULL) {
        if (!member_of_group(host, group)) continue;
      } else {
        if (!host_has_name(host, component_location)) continue;
      }
    } else if (has_constraint) {
      if (!set_has_string(&host->allowed_components, component_type) &&
        !set_has_string(&host->allowed_components, component_name))
        continue;
    } else if (all_components_assigned) {
      if (!host->all_components_allowed) continue;
    }
    best_candidate = host;
    load_on_best_candidate = host->n_active_components;
  }
  return best_candidate;
}

int MainController::n_hosts;
host_struct **MainController::hosts;

host_struct *MainController::add_new_host(unknown_connection *conn)
{
  Text_Buf *text_buf = conn->text_buf;
  int fd = conn->fd;

  host_struct *new_host = new host_struct;

  new_host->ip_addr = conn->ip_addr;
  new_host->hostname = mcopystr(new_host->ip_addr->get_host_str());
  new_host->hostname_local = text_buf->pull_string();
  new_host->machine_type = text_buf->pull_string();
  new_host->system_name = text_buf->pull_string();
  new_host->system_release = text_buf->pull_string();
  new_host->system_version = text_buf->pull_string();
  for (int i = 0; i < TRANSPORT_NUM; i++)
    new_host->transport_supported[i] = FALSE;
  int n_supported_transports = text_buf->pull_int().get_val();
  for (int i = 0; i < n_supported_transports; i++) {
    int transport_type = text_buf->pull_int().get_val();
    if (transport_type >= 0 && transport_type < TRANSPORT_NUM) {
      if (new_host->transport_supported[transport_type]) {
        send_error(fd, "Malformed VERSION message was received: "
          "Transport type %s was specified more than once.",
          get_transport_name((transport_type_enum)transport_type));
      } else new_host->transport_supported[transport_type] = TRUE;
    } else {
      send_error(fd, "Malformed VERSION message was received: "
        "Transport type code %d is invalid.", transport_type);
    }
  }
  if (!new_host->transport_supported[TRANSPORT_LOCAL]) {
    send_error(fd, "Malformed VERSION message was received: "
      "Transport type %s must be supported anyway.",
      get_transport_name(TRANSPORT_LOCAL));
  }
  if (!new_host->transport_supported[TRANSPORT_INET_STREAM]) {
    send_error(fd, "Malformed VERSION message was received: "
      "Transport type %s must be supported anyway.",
      get_transport_name(TRANSPORT_INET_STREAM));
  }
  new_host->log_source = mprintf("HC@%s", new_host->hostname_local);
  new_host->hc_state = HC_IDLE;
  new_host->hc_fd = fd;
  new_host->text_buf = text_buf;
  new_host->n_components = 0;
  new_host->components = NULL;
  // in most cases hostname and hostname_local are similar ("foo.bar.com" vs.
  // "foo") and it is enough to compare only the (fully qualified) hostname
  // when evaluating PTC location constraints
  new_host->local_hostname_different =
    !is_similar_hostname(new_host->hostname, new_host->hostname_local);
  add_allowed_components(new_host);
  new_host->n_active_components = 0;

  text_buf->cut_message();

  delete_unknown_connection(conn);

  n_hosts++;
  hosts = (host_struct**)Realloc(hosts, n_hosts * sizeof(*hosts));
  hosts[n_hosts - 1] = new_host;

  fd_table[fd].fd_type = FD_HC;
  fd_table[fd].host_ptr = new_host;

  notify("New HC connected from %s [%s]. %s: %s %s on %s.",
    new_host->hostname, new_host->ip_addr->get_addr_str(),
    new_host->hostname_local, new_host->system_name,
    new_host->system_release, new_host->machine_type);

  return new_host;
}

void MainController::close_hc_connection(host_struct *hc)
{
  if (hc->hc_state != HC_DOWN) {
    remove_poll_fd(hc->hc_fd);
    close(hc->hc_fd);
    remove_fd_from_table(hc->hc_fd);
    hc->hc_fd = -1;
    delete hc->text_buf;
    hc->text_buf = NULL;
    hc->hc_state = HC_DOWN;
    enable_server_fd();
  }
}

boolean MainController::is_hc_in_state(hc_state_enum checked_state)
{
  for (int i = 0; i < n_hosts; i++)
    if (hosts[i]->hc_state == checked_state) return TRUE;
  return FALSE;
}

boolean MainController::all_hc_in_state(hc_state_enum checked_state)
{
  for (int i = 0; i < n_hosts; i++)
    if (hosts[i]->hc_state != checked_state) return FALSE;
  return TRUE;
}

void MainController::configure_host(host_struct *host, boolean should_notify)
{
  if(config_str == NULL)
    fatal_error("MainController::configure_host: no config file");
  hc_state_enum next_state = HC_CONFIGURING;
  switch(host->hc_state) {
  case HC_CONFIGURING:
  case HC_CONFIGURING_OVERLOADED:
  case HC_EXITING:
    fatal_error("MainController::configure_host:"
      " host %s is in wrong state.",
      host->hostname);
    break;
  case HC_DOWN:
    break;
  case HC_OVERLOADED:
    next_state = HC_CONFIGURING_OVERLOADED;
    // no break
  default:
    host->hc_state = next_state;
    if (should_notify) {
      notify("Downloading configuration file to HC on host %s.",
        host->hostname);
    }
    send_configure(host, config_str);
    if (mc_state != MC_RECONFIGURING) {
      send_debug_setup(host);
    }
  }
}

void MainController::configure_mtc()
{
  if (config_str == NULL) {
    fatal_error("MainController::configure_mtc: no config file");
  }
  if (mtc->tc_state != TC_IDLE) {
    error("MainController::configure_mtc(): MTC is in wrong state.");
  }
  else {
    mtc->tc_state = MTC_CONFIGURING;
    send_configure_mtc(config_str);
  }
}

void MainController::check_all_hc_configured()
{
  bool reconf = mc_state == MC_RECONFIGURING;
  if (is_hc_in_state(HC_CONFIGURING) ||
    is_hc_in_state(HC_CONFIGURING_OVERLOADED)) return;
  if (is_hc_in_state(HC_IDLE)) {
    error("There were errors during configuring HCs.");
    mc_state = reconf ? MC_READY : MC_HC_CONNECTED;
  } else if (is_hc_in_state(HC_ACTIVE) || is_hc_in_state(HC_OVERLOADED)) {
    notify("Configuration file was processed on all HCs.");
    mc_state = reconf ? MC_READY : MC_ACTIVE;
  } else {
    error("There is no HC connection after processing the configuration "
      "file.");
    mc_state = MC_LISTENING;
  }
}

void MainController::add_component_to_host(host_struct *host,
  component_struct *comp)
{
  if (comp->comp_ref == MTC_COMPREF)
    comp->log_source = mprintf("MTC@%s", host->hostname_local);
  else if (comp->comp_name != NULL)
    comp->log_source = mprintf("%s(%d)@%s", comp->comp_name,
      comp->comp_ref, host->hostname_local);
  else comp->log_source = mprintf("%d@%s", comp->comp_ref,
    host->hostname_local);
  comp->comp_location = host;
  int i;
  for (i = host->n_components; i > 0; i--) {
    if (host->components[i - 1] < comp->comp_ref) break;
    else if (host->components[i - 1] == comp->comp_ref) return;
  }
  host->components = (component*)Realloc(host->components,
    (host->n_components + 1) * sizeof(component));
  memmove(host->components + i + 1, host->components + i,
    (host->n_components - i) * sizeof(component));
  host->components[i] = comp->comp_ref;
  host->n_components++;
}

void MainController::remove_component_from_host(component_struct *comp)
{
  Free(comp->log_source);
  comp->log_source = NULL;
  host_struct *host = comp->comp_location;
  if (host != NULL) {
    component comp_ref = comp->comp_ref;
    int i;
    for (i = host->n_components - 1; i >= 0; i--) {
      if (host->components[i] == comp_ref) break;
      else if (host->components[i] < comp_ref) return;
    }
    if (i < 0) return;
    host->n_components--;
    memmove(host->components + i, host->components + i + 1,
      (host->n_components - i) * sizeof(component));
    host->components = (component*)Realloc(host->components,
      host->n_components * sizeof(component));
  }
}

boolean MainController::version_known;
int MainController::n_modules;
module_version_info *MainController::modules;

#ifdef TTCN3_BUILDNUMBER
#define TTCN3_BUILDNUMBER_SAFE TTCN3_BUILDNUMBER
#else
#define TTCN3_BUILDNUMBER_SAFE 0
#endif


boolean MainController::check_version(unknown_connection *conn)
{
  Text_Buf& text_buf = *conn->text_buf;
  int version_major = text_buf.pull_int().get_val();
  int version_minor = text_buf.pull_int().get_val();
  int version_patchlevel = text_buf.pull_int().get_val();
  if (version_major != TTCN3_MAJOR || version_minor != TTCN3_MINOR ||
    version_patchlevel != TTCN3_PATCHLEVEL) {
    send_error(conn->fd, "Version mismatch: The TTCN-3 Main Controller has "
      "version " PRODUCT_NUMBER ", but the ETS was built with version "
      "%d.%d.pl%d.", version_major, version_minor, version_patchlevel);
    return TRUE;
  }
  int version_buildnumber = text_buf.pull_int().get_val();
  if (version_buildnumber != TTCN3_BUILDNUMBER_SAFE) {
    if (version_buildnumber > 0) send_error(conn->fd, "Build number "
      "mismatch: The TTCN-3 Main Controller has version " PRODUCT_NUMBER
      ", but the ETS was built with %d.%d.pre%d build %d.",
      version_major, version_minor, version_patchlevel,
      version_buildnumber);
    else send_error(conn->fd, "Build number mismatch: The TTCN-3 Main "
      "Controller has version " PRODUCT_NUMBER ", but the ETS was built "
      "with %d.%d.pl%d.", version_major, version_minor,
      version_patchlevel);
    return TRUE;
  }
  if (version_known) {
    int new_n_modules = text_buf.pull_int().get_val();
    if (n_modules != new_n_modules) {
      send_error(conn->fd, "The number of modules in this ETS (%d) "
        "differs from the number of modules in the firstly connected "
        "ETS (%d).", new_n_modules, n_modules);
      return TRUE;
    }
    for (int i = 0; i < n_modules; i++) {
      char *module_name = text_buf.pull_string();
      if (strcmp(module_name, modules[i].module_name)) {
        send_error(conn->fd, "The module number %d in this ETS (%s) "
          "has different name than in the firstly connected ETS (%s).",
          i, module_name, modules[i].module_name);
        delete [] module_name;
        return TRUE;
      }
      boolean checksum_differs = FALSE;
      int checksum_length = text_buf.pull_int().get_val();
      unsigned char *module_checksum;
      if (checksum_length != 0) {
        module_checksum = new unsigned char[checksum_length];
        text_buf.pull_raw(checksum_length, module_checksum);
      } else module_checksum = NULL;
      if (checksum_length != modules[i].checksum_length ||
        memcmp(module_checksum, modules[i].module_checksum,
          checksum_length)) checksum_differs = TRUE;
      delete [] module_checksum;
      if (checksum_differs) {
        send_error(conn->fd, "The checksum of module %s in this ETS "
          "is different than that of the firstly connected ETS.",
          module_name);
      }
      delete [] module_name;
      if (checksum_differs) return TRUE;
    }
  } else {
    n_modules = text_buf.pull_int().get_val();
    modules = new module_version_info[n_modules];
    for (int i = 0; i < n_modules; i++) {
      modules[i].module_name = text_buf.pull_string();
      modules[i].checksum_length = text_buf.pull_int().get_val();
      if (modules[i].checksum_length > 0) {
        modules[i].module_checksum =
          new unsigned char[modules[i].checksum_length];
        text_buf.pull_raw(modules[i].checksum_length,
          modules[i].module_checksum);
      } else modules[i].module_checksum = NULL;
    }
    version_known = TRUE;
  }
  return FALSE;
}

int MainController::n_components, MainController::n_active_ptcs,
    MainController::max_ptcs;
component_struct **MainController::components;
component_struct *MainController::mtc, *MainController::system;
const component_struct* MainController::debugger_active_tc;
component MainController::next_comp_ref, MainController::tc_first_comp_ref;

boolean MainController::any_component_done_requested,
        MainController::any_component_done_sent,
        MainController::all_component_done_requested,
        MainController::any_component_killed_requested,
        MainController::all_component_killed_requested;

void MainController::add_component(component_struct *comp)
{
  component comp_ref = comp->comp_ref;
  if (lookup_component(comp_ref) != NULL)
    fatal_error("MainController::add_component: duplicate "
      "component reference %d.", comp_ref);
  if (n_components <= comp_ref) {
    components = (component_struct**)Realloc(components, (comp_ref + 1) *
      sizeof(component_struct*));
    for (int i = n_components; i < comp_ref; i++) components[i] = NULL;
    n_components = comp_ref + 1;
  }
  components[comp_ref] = comp;
}

component_struct *MainController::lookup_component(component comp_ref)
{
  if (comp_ref >= 0 && comp_ref < n_components) return components[comp_ref];
  else return NULL;
}

void MainController::destroy_all_components()
{
  for (component i = 0; i < n_components; i++) {
    component_struct *comp = components[i];
    if (comp != NULL) {
      close_tc_connection(comp);
      remove_component_from_host(comp);
      free_qualified_name(&comp->comp_type);
      delete [] comp->comp_name;
      free_qualified_name(&comp->tc_fn_name);
      delete [] comp->return_type;
      Free(comp->return_value);
      if (comp->verdict_reason != NULL) {
        delete [] comp->verdict_reason;
        comp->verdict_reason = NULL;
      }
      switch (comp->tc_state) {
      case TC_INITIAL:
        delete [] comp->initial.location_str;
        break;
      case PTC_STARTING:
        Free(comp->starting.arguments_ptr);
        free_requestors(&comp->starting.cancel_done_sent_to);
        break;
      case TC_STOPPING:
      case PTC_STOPPING_KILLING:
      case PTC_KILLING:
        free_requestors(&comp->stopping_killing.stop_requestors);
        free_requestors(&comp->stopping_killing.kill_requestors);
      default:
        break;
      }
      free_requestors(&comp->done_requestors);
      free_requestors(&comp->killed_requestors);
      free_requestors(&comp->cancel_done_sent_for);
      remove_all_connections(i);
      delete comp;
    }
  }
  Free(components);
  components = NULL;
  n_components = 0;
  n_active_ptcs = 0;
  mtc = NULL;
  system = NULL;

  for (int i = 0; i < n_hosts; i++) hosts[i]->n_active_components = 0;

  next_comp_ref = FIRST_PTC_COMPREF;

  any_component_done_requested = FALSE;
  any_component_done_sent = FALSE;
  all_component_done_requested = FALSE;
  any_component_killed_requested = FALSE;
  all_component_killed_requested = FALSE;
}

void MainController::close_tc_connection(component_struct *comp)
{
  if (comp->tc_fd >= 0) {
    remove_poll_fd(comp->tc_fd);
    close(comp->tc_fd);
    remove_fd_from_table(comp->tc_fd);
    comp->tc_fd = -1;
    delete comp->text_buf;
    comp->text_buf = NULL;
    enable_server_fd();
  }
  if (comp->kill_timer != NULL) {
    cancel_timer(comp->kill_timer);
    comp->kill_timer = NULL;
  }
}

boolean MainController::stop_after_tc, MainController::stop_requested;

boolean MainController::ready_to_finish_testcase()
{
  for (component i = tc_first_comp_ref; i < n_components; i++) {
    switch (components[i]->tc_state) {
    case TC_EXITED:
    case PTC_STALE:
      break;
    default:
      return FALSE;
    }
  }
  return TRUE;
}

void MainController::finish_testcase()
{
  if (stop_requested) {
    send_ptc_verdict(FALSE);
    send_stop(mtc);
    mtc->tc_state = MTC_CONTROLPART;
    mtc->stop_requested = TRUE;
    start_kill_timer(mtc);
    mc_state = MC_EXECUTING_CONTROL;
  } else if (stop_after_tc) {
    send_ptc_verdict(FALSE);
    mtc->tc_state = MTC_PAUSED;
    mc_state = MC_PAUSED;
    notify("Execution has been paused.");
  } else {
    send_ptc_verdict(TRUE);
    mtc->tc_state = MTC_CONTROLPART;
    mc_state = MC_EXECUTING_CONTROL;
  }

  for (component i = tc_first_comp_ref; i < n_components; i++) {
    components[i]->tc_state = PTC_STALE;
  }
  mtc->local_verdict = NONE;
  free_qualified_name(&mtc->comp_type);
  free_qualified_name(&mtc->tc_fn_name);
  free_qualified_name(&system->comp_type);
}

boolean MainController::message_expected(component_struct *from,
  const char *message_name)
{
  switch (mc_state) {
  case MC_EXECUTING_TESTCASE:
    switch (mtc->tc_state) {
    case MTC_ALL_COMPONENT_STOP:
    case MTC_ALL_COMPONENT_KILL:
      // silently ignore
      return FALSE;
    default:
      return TRUE;
    }
    case MC_TERMINATING_TESTCASE:
      // silently ignore
      return FALSE;
    default:
      send_error(from->tc_fd, "Unexpected message %s was received.",
        message_name);
      return FALSE;
  }
}

boolean MainController::request_allowed(component_struct *from,
  const char *message_name)
{
  if (!message_expected(from, message_name)) return FALSE;

  switch (from->tc_state) {
  case MTC_TESTCASE:
    if (from == mtc) return TRUE;
    else break;
  case PTC_FUNCTION:
    if (from != mtc) return TRUE;
    else break;
  case TC_STOPPING:
  case PTC_STOPPING_KILLING:
  case PTC_KILLING:
    // silently ignore
    return FALSE;
  default:
    break;
  }
  send_error(from->tc_fd, "The sender of message %s is in "
    "unexpected state.", message_name);
  return FALSE;
}

boolean MainController::valid_endpoint(component component_reference,
  boolean new_connection, component_struct *requestor, const char *operation)
{
  switch (component_reference) {
  case NULL_COMPREF:
    send_error(requestor->tc_fd, "The %s operation refers to the null "
      "component reference.", operation);
    return FALSE;
  case SYSTEM_COMPREF:
    send_error(requestor->tc_fd, "The %s operation refers to the system "
      "component reference.", operation);
    return FALSE;
  case ANY_COMPREF:
    send_error(requestor->tc_fd, "The %s operation refers to "
      "'any component'.", operation);
    return FALSE;
  case ALL_COMPREF:
    send_error(requestor->tc_fd, "The %s operation refers to "
      "'all component'.", operation);
    return FALSE;
  default:
    break;
  }
  component_struct *comp = lookup_component(component_reference);
  if (comp == NULL) {
    send_error(requestor->tc_fd, "The %s operation refers to "
      "invalid component reference %d.", operation,
      component_reference);
    return FALSE;
  }
  switch (comp->tc_state) {
  case TC_IDLE:
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case MTC_TESTCASE:
  case PTC_FUNCTION:
  case PTC_STARTING:
  case PTC_STOPPED:
    return TRUE;
  case PTC_KILLING:
  case PTC_STOPPING_KILLING:
    if (new_connection) {
      send_error(requestor->tc_fd, "The %s operation refers to test "
        "component with component reference %d, which is currently "
        "being terminated.", operation, component_reference);
      return FALSE;
    } else return TRUE;
  case TC_EXITING:
  case TC_EXITED:
    if (new_connection) {
      send_error(requestor->tc_fd, "The %s operation refers to test "
        "component with component reference %d, which has already "
        "terminated.", operation, component_reference);
      return FALSE;
    } else return TRUE;
  case PTC_STALE:
    send_error(requestor->tc_fd, "The %s operation refers to component "
      "reference %d, which belongs to an earlier test case.",
      operation, component_reference);
    return FALSE;
  default:
    send_error(requestor->tc_fd, "The %s operation refers to component "
      "reference %d, which is in invalid state.",
      operation, component_reference);
    error("Test component with component reference %d is in invalid state "
      "when a %s operation was requested on a port of it.",
      component_reference, operation);
    return FALSE;
  }
}

void MainController::destroy_connection(port_connection *conn,
  component_struct *tc)
{
  switch (conn->conn_state) {
  case CONN_LISTENING:
  case CONN_CONNECTING:
    if (conn->transport_type != TRANSPORT_LOCAL &&
      conn->head.comp_ref != tc->comp_ref) {
      // shut down the server side if the client has terminated
      send_disconnect_to_server(conn);
    }
    send_error_to_connect_requestors(conn, "test component %d has "
      "terminated during connection setup.", tc->comp_ref);
    break;
  case CONN_CONNECTED:
    break;
  case CONN_DISCONNECTING:
    send_disconnect_ack_to_requestors(conn);
    break;
  default:
    error("The port connection %d:%s - %d:%s is in invalid state when "
      "test component %d has terminated.", conn->head.comp_ref,
      conn->head.port_name, conn->tail.comp_ref, conn->tail.port_name,
      tc->comp_ref);
  }
  remove_connection(conn);
}

void MainController::destroy_mapping(port_connection *conn)
{
  component tc_compref;
  const char *tc_port, *system_port;
  if (conn->head.comp_ref == SYSTEM_COMPREF) {
    tc_compref = conn->tail.comp_ref;
    tc_port = conn->tail.port_name;
    system_port = conn->head.port_name;
  } else {
    tc_compref = conn->head.comp_ref;
    tc_port = conn->head.port_name;
    system_port = conn->tail.port_name;
  }
  switch (conn->conn_state) {
  case CONN_UNMAPPING:
    for (int i = 0; ; i++) {
      component_struct *comp = get_requestor(&conn->requestors, i);
      if (comp == NULL) break;
      if (comp->tc_state == TC_UNMAP) {
        send_unmap_ack(comp);
        if (comp == mtc) comp->tc_state = MTC_TESTCASE;
        else comp->tc_state = PTC_FUNCTION;
      }
    }
    break;
  case CONN_MAPPING:
    for (int i = 0; ; i++) {
      component_struct *comp = get_requestor(&conn->requestors, i);
      if (comp == NULL) break;
      if (comp->tc_state == TC_MAP) {
        send_error(comp->tc_fd, "Establishment of port mapping "
          "%d:%s - system:%s failed because the test component "
          "endpoint has terminated.", tc_compref, tc_port,
          system_port);
        if (comp == mtc) comp->tc_state = MTC_TESTCASE;
        else comp->tc_state = PTC_FUNCTION;
      }
    }
  default:
    break;
  }
  remove_connection(conn);
}

boolean MainController::stop_all_components()
{
  boolean ready_for_ack = TRUE;
  for (component i = tc_first_comp_ref; i < n_components; i++) {
    component_struct *tc = components[i];
    switch (tc->tc_state) {
    case TC_INITIAL:
      // we do not have to termiate the PTC (and wait for the control
      // connection) if it is alive
      if (!tc->is_alive) ready_for_ack = FALSE;
      break;
    case TC_IDLE:
      // do nothing if the component is alive
      if (!tc->is_alive) {
        send_kill(tc);
        tc->tc_state = PTC_KILLING;
        tc->stop_requested = TRUE;
        init_requestors(&tc->stopping_killing.stop_requestors, NULL);
        init_requestors(&tc->stopping_killing.kill_requestors, NULL);
        start_kill_timer(tc);
        ready_for_ack = FALSE;
      }
      break;
    case TC_CREATE:
    case TC_START:
    case TC_STOP:
    case TC_KILL:
    case TC_CONNECT:
    case TC_DISCONNECT:
    case TC_MAP:
    case TC_UNMAP:
    case PTC_FUNCTION:
      // the PTC is executing behaviour
      if (tc->is_alive) {
        send_stop(tc);
        tc->tc_state = TC_STOPPING;
      } else {
        // STOP is never sent to non-alive PTCs
        send_kill(tc);
        tc->tc_state = PTC_STOPPING_KILLING;
      }
      tc->stop_requested = TRUE;
      init_requestors(&tc->stopping_killing.stop_requestors, NULL);
      init_requestors(&tc->stopping_killing.kill_requestors, NULL);
      start_kill_timer(tc);
      ready_for_ack = FALSE;
      break;
    case PTC_STARTING:
      // do nothing, just put it back to STOPPED state
      free_qualified_name(&tc->tc_fn_name);
      Free(tc->starting.arguments_ptr);
      free_requestors(&tc->starting.cancel_done_sent_to);
      tc->tc_state = PTC_STOPPED;
      break;
    case TC_STOPPING:
    case PTC_STOPPING_KILLING:
      free_requestors(&tc->stopping_killing.stop_requestors);
      free_requestors(&tc->stopping_killing.kill_requestors);
      ready_for_ack = FALSE;
      break;
    case PTC_KILLING:
      free_requestors(&tc->stopping_killing.stop_requestors);
      free_requestors(&tc->stopping_killing.kill_requestors);
      // we have to wait only if the PTC is non-alive
      if (!tc->is_alive) ready_for_ack = FALSE;
      break;
    case PTC_STOPPED:
    case TC_EXITING:
    case TC_EXITED:
    case PTC_STALE:
      break;
    default:
      error("Test Component %d is in invalid state when stopping all "
        "components.", tc->comp_ref);
    }
    // only mtc is preserved in done_requestors and killed_requestors
    boolean mtc_requested_done = has_requestor(&tc->done_requestors, mtc);
    free_requestors(&tc->done_requestors);
    if (mtc_requested_done) add_requestor(&tc->done_requestors, mtc);
    boolean mtc_requested_killed = has_requestor(&tc->killed_requestors,
      mtc);
    free_requestors(&tc->killed_requestors);
    if (mtc_requested_killed) add_requestor(&tc->killed_requestors, mtc);
    free_requestors(&tc->cancel_done_sent_for);
  }
  return ready_for_ack;

}

void MainController::check_all_component_stop()
{
  // MTC has requested 'all component.stop'
  // we have to send acknowledgement to MTC only
  boolean ready_for_ack = TRUE;
  for (component i = tc_first_comp_ref; i < n_components; i++) {
    component_struct *comp = components[i];
    switch (comp->tc_state) {
    case TC_INITIAL:
    case PTC_KILLING:
      if (!comp->is_alive) ready_for_ack = FALSE;
      break;
    case TC_STOPPING:
    case PTC_STOPPING_KILLING:
      ready_for_ack = FALSE;
      break;
    case TC_EXITING:
    case TC_EXITED:
    case PTC_STOPPED:
    case PTC_STALE:
      break;
    case TC_IDLE:
      // only alive components can be in idle state
      if (comp->is_alive) break;
    default:
      error("PTC %d is in invalid state when performing "
        "'all component.stop' operation.", comp->comp_ref);
    }
    if (!ready_for_ack) break;
  }
  if (ready_for_ack) {
    send_stop_ack(mtc);
    mtc->tc_state = MTC_TESTCASE;
  }
}

void MainController::send_stop_ack_to_requestors(component_struct *tc)
{
  for (int i = 0; ; i++) {
    component_struct *requestor =
      get_requestor(&tc->stopping_killing.stop_requestors, i);
    if (requestor == NULL) break;
    if (requestor->tc_state == TC_STOP) {
      send_stop_ack(requestor);
      if (requestor == mtc) requestor->tc_state = MTC_TESTCASE;
      else requestor->tc_state = PTC_FUNCTION;
    }
  }
  free_requestors(&tc->stopping_killing.stop_requestors);
}

boolean MainController::kill_all_components(boolean testcase_ends)
{
  boolean ready_for_ack = TRUE;
  for (component i = tc_first_comp_ref; i < n_components; i++) {
    component_struct *tc = components[i];
    boolean is_inactive = FALSE;
    switch (tc->tc_state) {
    case TC_INITIAL:
      // the PTC does not have an identified control connection yet
      ready_for_ack = FALSE;
      break;
    case PTC_STARTING:
      free_qualified_name(&tc->tc_fn_name);
      Free(tc->starting.arguments_ptr);
      free_requestors(&tc->starting.cancel_done_sent_to);
      // no break
    case TC_IDLE:
    case PTC_STOPPED:
      is_inactive = TRUE;
      // no break
    case TC_CREATE:
    case TC_START:
    case TC_STOP:
    case TC_KILL:
    case TC_CONNECT:
    case TC_DISCONNECT:
    case TC_MAP:
    case TC_UNMAP:
    case PTC_FUNCTION:
      send_kill(tc);
      if (is_inactive) {
        // the PTC was inactive
        tc->tc_state = PTC_KILLING;
        if (!tc->is_alive) tc->stop_requested = TRUE;
      } else {
        // the PTC was active
        tc->tc_state = PTC_STOPPING_KILLING;
        tc->stop_requested = TRUE;
      }
      init_requestors(&tc->stopping_killing.stop_requestors, NULL);
      init_requestors(&tc->stopping_killing.kill_requestors, NULL);
      start_kill_timer(tc);
      ready_for_ack = FALSE;
      break;
    case TC_STOPPING:
      send_kill(tc);
      tc->tc_state = PTC_STOPPING_KILLING;
      if (tc->kill_timer != NULL) cancel_timer(tc->kill_timer);
      start_kill_timer(tc);
      // no break
    case PTC_KILLING:
    case PTC_STOPPING_KILLING:
      free_requestors(&tc->stopping_killing.stop_requestors);
      free_requestors(&tc->stopping_killing.kill_requestors);
      ready_for_ack = FALSE;
      break;
    case TC_EXITING:
      if (testcase_ends) ready_for_ack = FALSE;
    case TC_EXITED:
    case PTC_STALE:
      break;
    default:
      error("Test Component %d is in invalid state when killing all "
        "components.", tc->comp_ref);
    }
    if (testcase_ends) {
      free_requestors(&tc->done_requestors);
      free_requestors(&tc->killed_requestors);
    } else {
      // only mtc is preserved in done_requestors and killed_requestors
      boolean mtc_requested_done = has_requestor(&tc->done_requestors,
        mtc);
      free_requestors(&tc->done_requestors);
      if (mtc_requested_done) add_requestor(&tc->done_requestors, mtc);
      boolean mtc_requested_killed = has_requestor(&tc->killed_requestors,
        mtc);
      free_requestors(&tc->killed_requestors);
      if (mtc_requested_killed)
        add_requestor(&tc->killed_requestors, mtc);
    }
    free_requestors(&tc->cancel_done_sent_for);
  }
  return ready_for_ack;
}

void MainController::check_all_component_kill()
{
  // MTC has requested 'all component.kill'
  // we have to send acknowledgement to MTC only
  boolean ready_for_ack = TRUE;
  for (component i = tc_first_comp_ref; i < n_components; i++) {
    component_struct *comp = components[i];
    switch (comp->tc_state) {
    case TC_INITIAL:
    case PTC_STOPPING_KILLING:
    case PTC_KILLING:
      ready_for_ack = FALSE;
    case TC_EXITING:
    case TC_EXITED:
    case PTC_STALE:
      break;
    default:
      error("PTC %d is in invalid state when performing "
        "'all component.kill' operation.", comp->comp_ref);
    }
    if (!ready_for_ack) break;
  }
  if (ready_for_ack) {
    send_kill_ack(mtc);
    mtc->tc_state = MTC_TESTCASE;
  }
}

void MainController::send_kill_ack_to_requestors(component_struct *tc)
{
  for (int i = 0; ; i++) {
    component_struct *requestor =
      get_requestor(&tc->stopping_killing.kill_requestors, i);
    if (requestor == NULL) break;
    if (requestor->tc_state == TC_KILL) {
      send_kill_ack(requestor);
      if (requestor == mtc) requestor->tc_state = MTC_TESTCASE;
      else requestor->tc_state = PTC_FUNCTION;
    }
  }
  free_requestors(&tc->stopping_killing.kill_requestors);
}

void MainController::send_component_status_to_requestor(component_struct *tc,
  component_struct *requestor, boolean done_status, boolean killed_status)
{
  switch (requestor->tc_state) {
  case PTC_FUNCTION:
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case PTC_STOPPED:
  case PTC_STARTING:
    if (done_status) {
      send_component_status_ptc(requestor, tc->comp_ref, TRUE,
        killed_status, tc->return_type, tc->return_value_len,
        tc->return_value);
    } else {
      send_component_status_ptc(requestor, tc->comp_ref, FALSE,
        killed_status, NULL, 0, NULL);
    }
    break;
  case PTC_STOPPING_KILLING:
  case PTC_KILLING:
  case TC_EXITING:
  case TC_EXITED:
    // the PTC requestor is not interested in the component status anymore
    break;
  default:
    error("PTC %d is in invalid state when sending out COMPONENT_STATUS "
      "message about PTC %d.", requestor->comp_ref, tc->comp_ref);
  }
}

void MainController::component_stopped(component_struct *tc)
{
  // checking and updating the state of tc
  tc_state_enum old_state = tc->tc_state;
  if (old_state == PTC_STOPPING_KILLING) tc->tc_state = PTC_KILLING;
  else {
    tc->tc_state = PTC_STOPPED;
    if (tc->kill_timer != NULL) {
      cancel_timer(tc->kill_timer);
      tc->kill_timer = NULL;
    }
  }
  switch (mc_state) {
  case MC_EXECUTING_TESTCASE:
    // this is the correct state
    break;
  case MC_TERMINATING_TESTCASE:
    // do nothing, we are waiting for the end of all PTC connections
    return;
  default:
    error("PTC %d stopped in invalid MC state.", tc->comp_ref);
    return;
  }
  if (!tc->is_alive) {
    send_error_str(tc->tc_fd, "Message STOPPED can only be sent by "
      "alive PTCs.");
    return;
  }
  // Note: the COMPONENT_STATUS message must be sent before STOP_ACK because
  // the latter may update the component status cache table to an inconsistent
  // state
  boolean send_status_to_mtc = FALSE, send_done_to_mtc = FALSE;
  // sending out COMPONENT_STATUS messages to PTCs
  for (int i = 0; ; i++) {
    component_struct *requestor = get_requestor(&tc->done_requestors, i);
    if (requestor == NULL) break;
    else if (requestor == mtc) {
      send_status_to_mtc = TRUE;
      send_done_to_mtc = TRUE;
    } else send_component_status_to_requestor(tc, requestor, TRUE, FALSE);
  }
  // do not send unsolicited 'any component.done' status
  if (any_component_done_requested) send_status_to_mtc = TRUE;
  boolean all_done_checked = FALSE, all_done_result = FALSE;
  if (all_component_done_requested) {
    all_done_checked = TRUE;
    all_done_result = !is_any_component_running();
    if (all_done_result) send_status_to_mtc = TRUE;
  }
  if (send_status_to_mtc) {
    if (!all_done_checked) all_done_result = !is_any_component_running();
    if (send_done_to_mtc) {
      // the return value was requested
      send_component_status_mtc(tc->comp_ref, TRUE, FALSE,
        any_component_done_requested, all_done_result, FALSE, FALSE,
        tc->return_type, tc->return_value_len, tc->return_value);
    } else {
      // the return value was not requested
      send_component_status_mtc(NULL_COMPREF, FALSE, FALSE,
        any_component_done_requested, all_done_result, FALSE, FALSE,
        NULL, 0, NULL);
    }
    if (any_component_done_requested) {
      any_component_done_requested = FALSE;
      any_component_done_sent = TRUE;
    }
    if (all_done_result) all_component_done_requested = FALSE;
  }
  // sending out STOP_ACK messages
  if (old_state != PTC_FUNCTION) {
    // the PTC was explicitly stopped and/or killed
    if (mtc->tc_state == MTC_ALL_COMPONENT_KILL) {
      // do nothing
    } else if (mtc->tc_state == MTC_ALL_COMPONENT_STOP) {
      check_all_component_stop();
    } else {
      send_stop_ack_to_requestors(tc);
    }
  }
}

void MainController::component_terminated(component_struct *tc)
{
  // the state variable of the PTC has to be updated first
  // because in case of 'all component.kill' or 'all component.stop'
  // we are walking through the states of all PTCs
  tc_state_enum old_state = tc->tc_state;
  tc->tc_state = TC_EXITING;
  n_active_ptcs--;
  tc->comp_location->n_active_components--;
  switch (mc_state) {
  case MC_EXECUTING_TESTCASE:
    // this is the correct state
    break;
  case MC_TERMINATING_TESTCASE:
    // do nothing, we are waiting for the end of all PTC connections
    return;
  default:
    error("PTC %d terminated in invalid MC state.", tc->comp_ref);
    return;
  }
  // sending out COMPONENT_STATUS messages
  // Notes:
  // - the COMPONENT_STATUS message must be sent before STOP_ACK and KILL_ACK
  //   because the latter may update the component status cache table to an
  //   inconsistent state
  // - unsolicited 'done' status and return value is never sent out
  // the flags below indicate whether a COMPONENT_STATUS message
  // (with or without the return value) has to be sent to the MTC
  boolean send_status_to_mtc = FALSE, send_done_to_mtc = TRUE;
  // first send out the COMPONENT_STATUS messages to PTCs
  for (int i = 0; ; i++) {
    component_struct *requestor = get_requestor(&tc->done_requestors, i);
    if (requestor == NULL) break;
    else if (requestor == mtc) {
      send_status_to_mtc = TRUE;
      send_done_to_mtc = TRUE;
    } else send_component_status_to_requestor(tc, requestor, TRUE, TRUE);
  }
  for (int i = 0; ; i++) {
    component_struct *requestor = get_requestor(&tc->killed_requestors, i);
    if (requestor == NULL) break;
    else if (requestor == mtc) send_status_to_mtc = TRUE;
    else if (!has_requestor(&tc->done_requestors, requestor)) {
      // do not send COMPONENT_STATUS twice to the same PTC
      send_component_status_to_requestor(tc, requestor, FALSE, TRUE);
    }
  }
  free_requestors(&tc->done_requestors);
  free_requestors(&tc->killed_requestors);
  // deciding whether to send a COMPONENT_STATUS message to MTC
  // 'any component.done' status can be safely sent out
  // it will not be cancelled later
  if (any_component_done_requested || any_component_killed_requested)
    send_status_to_mtc = TRUE;
  boolean all_done_checked = FALSE, all_done_result = FALSE;
  if (all_component_done_requested) {
    all_done_checked = TRUE;
    all_done_result = !is_any_component_running();
    if (all_done_result) send_status_to_mtc = TRUE;
  }
  boolean all_killed_checked = FALSE, all_killed_result = FALSE;
  if (all_component_killed_requested) {
    all_killed_checked = TRUE;
    all_killed_result = !is_any_component_alive();
    if (all_killed_result) send_status_to_mtc = TRUE;
  }
  // sending the COMPONENT_STATUS message to MTC if necessary
  if (send_status_to_mtc) {
    if (!all_done_checked) all_done_result = !is_any_component_running();
    if (!all_killed_checked) all_killed_result = !is_any_component_alive();
    if (send_done_to_mtc) {
      // the return value was requested
      send_component_status_mtc(tc->comp_ref, TRUE, TRUE, TRUE,
        all_done_result, TRUE, all_killed_result, tc->return_type,
        tc->return_value_len, tc->return_value);
    } else {
      // the return value was not requested
      send_component_status_mtc(tc->comp_ref, FALSE, TRUE, TRUE,
        all_done_result, TRUE, all_killed_result, NULL, 0, NULL);
    }
    any_component_done_requested = FALSE;
    any_component_done_sent = TRUE;
    any_component_killed_requested = FALSE;
    if (all_done_result) all_component_done_requested = FALSE;
    if (all_killed_result) all_component_killed_requested = FALSE;
  }
  // sending out STOP_ACK and KILL_ACK messages if necessary
  switch (old_state) {
  case TC_STOPPING:
  case PTC_STOPPING_KILLING:
  case PTC_KILLING:
    // the component was explicitly stopped and/or killed
    if (mtc->tc_state == MTC_ALL_COMPONENT_KILL) {
      check_all_component_kill();
    } else if (mtc->tc_state == MTC_ALL_COMPONENT_STOP) {
      check_all_component_stop();
    } else {
      send_stop_ack_to_requestors(tc);
      send_kill_ack_to_requestors(tc);
    }
  default:
    break;
  }
  // we should behave as we got all pending CANCEL_DONE_ACK messages from tc
  for (int i = 0; ; i++) {
    component_struct *comp = get_requestor(&tc->cancel_done_sent_for, i);
    if (comp == NULL) break;
    done_cancelled(tc, comp);
  }
  free_requestors(&tc->cancel_done_sent_for);
  // destroy all connections and mappings of the component
  // and send out the related messages
  while (tc->conn_head_list != NULL) {
    if (tc->conn_head_list->tail.comp_ref == SYSTEM_COMPREF)
      destroy_mapping(tc->conn_head_list);
    else destroy_connection(tc->conn_head_list, tc);
  }
  while (tc->conn_tail_list != NULL) {
    if (tc->conn_tail_list->head.comp_ref == SYSTEM_COMPREF)
      destroy_mapping(tc->conn_tail_list);
    else destroy_connection(tc->conn_tail_list, tc);
  }
  // drop the name of the currently executed function
  free_qualified_name(&tc->tc_fn_name);
}

void MainController::done_cancelled(component_struct *from,
  component_struct *started_tc)
{
  // do nothing if the PTC to be started is not in starting state anymore
  if (started_tc->tc_state != PTC_STARTING) return;
  remove_requestor(&started_tc->starting.cancel_done_sent_to, from);
  // do nothing if we are waiting for more CANCEL_DONE_ACK messages
  if (get_requestor(&started_tc->starting.cancel_done_sent_to, 0) != NULL)
    return;
  send_start(started_tc, started_tc->tc_fn_name,
    started_tc->starting.arguments_len, started_tc->starting.arguments_ptr);
  component_struct *start_requestor = started_tc->starting.start_requestor;
  if (start_requestor->tc_state == TC_START) {
    send_start_ack(start_requestor);
    if (start_requestor == mtc) start_requestor->tc_state = MTC_TESTCASE;
    else start_requestor->tc_state = PTC_FUNCTION;
  }
  Free(started_tc->starting.arguments_ptr);
  free_requestors(&started_tc->starting.cancel_done_sent_to);
  started_tc->tc_state = PTC_FUNCTION;
  status_change();
}

boolean MainController::component_is_alive(component_struct *tc)
{
  switch (tc->tc_state) {
  case TC_INITIAL:
  case TC_IDLE:
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case PTC_FUNCTION:
  case PTC_STARTING:
  case PTC_STOPPED:
  case PTC_KILLING:
  case PTC_STOPPING_KILLING:
    return TRUE;
  case TC_EXITING:
  case TC_EXITED:
    return FALSE;
  default:
    error("PTC %d is in invalid state when checking whether it is alive.",
      tc->comp_ref);
    return FALSE;
  }
}

boolean MainController::component_is_running(component_struct *tc)
{
  switch (tc->tc_state) {
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case PTC_FUNCTION:
  case PTC_STARTING:
  case PTC_STOPPING_KILLING:
    return TRUE;
  case TC_INITIAL:
  case TC_IDLE:
  case TC_EXITING:
  case TC_EXITED:
  case PTC_STOPPED:
  case PTC_KILLING:
    return FALSE;
  default:
    error("PTC %d is in invalid state when checking whether it is running.",
      tc->comp_ref);
    return FALSE;
  }
}

boolean MainController::component_is_done(component_struct *tc)
{
  switch (tc->tc_state) {
  case TC_EXITING:
  case TC_EXITED:
  case PTC_STOPPED:
    return TRUE;
  case TC_INITIAL:
  case TC_IDLE:
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case PTC_FUNCTION:
  case PTC_STARTING:
  case PTC_KILLING:
  case PTC_STOPPING_KILLING:
    return FALSE;
  default:
    error("PTC %d is in invalid state when checking whether it is done.",
      tc->comp_ref);
    return FALSE;
  }
}

boolean MainController::is_any_component_alive()
{
  for (component i = tc_first_comp_ref; i < n_components; i++)
    if (component_is_alive(components[i])) return TRUE;
  return FALSE;
}

boolean MainController::is_all_component_alive()
{
  for (component i = tc_first_comp_ref; i < n_components; i++)
    if (!component_is_alive(components[i])) return FALSE;
  return TRUE;
}

boolean MainController::is_any_component_running()
{
  for (component i = tc_first_comp_ref; i < n_components; i++)
    if (component_is_running(components[i])) return TRUE;
  return FALSE;
}

boolean MainController::is_all_component_running()
{
  for (component i = tc_first_comp_ref; i < n_components; i++) {
    component_struct *tc = components[i];
    if (tc->stop_requested) continue;
    switch (tc->tc_state) {
    case TC_EXITING:
    case TC_EXITED:
    case PTC_STOPPED:
      return FALSE;
    default:
      break;
    }
  }
  return TRUE;
}

boolean MainController::is_any_component_done()
{
  for (component i = tc_first_comp_ref; i < n_components; i++)
    if (component_is_done(components[i])) return TRUE;
  return FALSE;
}

void MainController::start_kill_timer(component_struct *tc)
{
  if (kill_timer > 0.0) {
    timer_struct *timer = new timer_struct;
    timer->expiration = time_now() + kill_timer;
    timer->timer_argument.component_ptr = tc;
    tc->kill_timer = timer;
    register_timer(timer);
  } else tc->kill_timer = NULL;
}

void MainController::init_connections(component_struct *tc)
{
  tc->conn_head_list = NULL;
  tc->conn_tail_list = NULL;
  tc->conn_head_count = 0;
  tc->conn_tail_count = 0;
}

void MainController::add_connection(port_connection *c)
{
  // Canonical ordering of endpoints so that head <= tail
  if (c->head.comp_ref > c->tail.comp_ref) {
    component tmp_comp = c->head.comp_ref;
    c->head.comp_ref = c->tail.comp_ref;
    c->tail.comp_ref = tmp_comp;
    char *tmp_port = c->head.port_name;
    c->head.port_name = c->tail.port_name;
    c->tail.port_name = tmp_port;
  } else if (c->head.comp_ref == c->tail.comp_ref &&
    strcmp(c->head.port_name, c->tail.port_name) > 0) {
    char *tmp_port = c->head.port_name;
    c->head.port_name = c->tail.port_name;
    c->tail.port_name = tmp_port;
  }
  // Double-chain in according to c->head
  component_struct *head_component = lookup_component(c->head.comp_ref);
  port_connection *head_connection = head_component->conn_head_list;
  if (head_connection == NULL) {
    c->head.next = c;
    c->head.prev = c;
  } else {
    c->head.prev = head_connection->head.prev;
    head_connection->head.prev = c;
    c->head.next = head_connection;
    c->head.prev->head.next = c;
  }
  head_component->conn_head_list = c;
  head_component->conn_head_count++;
  // Double-chain in according to c->tail
  component_struct *tail_component = lookup_component(c->tail.comp_ref);
  port_connection *tail_connection = tail_component->conn_tail_list;
  if (tail_connection == NULL) {
    c->tail.next = c;
    c->tail.prev = c;
  } else {
    c->tail.prev = tail_connection->tail.prev;
    tail_connection->tail.prev = c;
    c->tail.next = tail_connection;
    c->tail.prev->tail.next = c;
  }
  tail_component->conn_tail_list = c;
  tail_component->conn_tail_count++;
}

void MainController::remove_connection(port_connection *c)
{
  // Remove from conn_head_list
  component_struct *head_component = lookup_component(c->head.comp_ref);
  if (c->head.next == c) {
    head_component->conn_head_list = NULL;
    head_component->conn_head_count = 0;
  } else {
    c->head.prev->head.next = c->head.next;
    c->head.next->head.prev = c->head.prev;
    head_component->conn_head_list = c->head.next;
    head_component->conn_head_count--;
  }
  // Remove from conn_tail_list
  component_struct *tail_component = lookup_component(c->tail.comp_ref);
  if (c->tail.next == c) {
    tail_component->conn_tail_list = NULL;
    tail_component->conn_tail_count = 0;
  } else {
    c->tail.prev->tail.next = c->tail.next;
    c->tail.next->tail.prev = c->tail.prev;
    tail_component->conn_tail_list = c->tail.next;
    tail_component->conn_tail_count--;
  }
  // Delete the data members
  delete [] c->head.port_name;
  delete [] c->tail.port_name;
  free_requestors(&c->requestors);
  delete c;
}

port_connection *MainController::find_connection(component head_comp,
  const char *head_port, component tail_comp, const char *tail_port)
{
  // Canonical ordering of parameters so that head <= tail
  if (head_comp > tail_comp) {
    component tmp_comp = head_comp;
    head_comp = tail_comp;
    tail_comp = tmp_comp;
    const char *tmp_port = head_port;
    head_port = tail_port;
    tail_port = tmp_port;
  } else if (head_comp == tail_comp && strcmp(head_port, tail_port) > 0) {
    const char *tmp_port = head_port;
    head_port = tail_port;
    tail_port = tmp_port;
  }
  // Check whether one of the endpoints' list is empty
  component_struct *head_component = lookup_component(head_comp);
  port_connection *head_connection = head_component->conn_head_list;
  if (head_connection == NULL) return NULL;
  component_struct *tail_component = lookup_component(tail_comp);
  port_connection *tail_connection = tail_component->conn_tail_list;
  if (tail_connection == NULL) return NULL;
  // Start searching on the shorter list
  if (head_component->conn_head_count <= tail_component->conn_tail_count) {
    port_connection *iter = head_connection;
    do {
      if (iter->tail.comp_ref == tail_comp &&
        !strcmp(iter->head.port_name, head_port) &&
        !strcmp(iter->tail.port_name, tail_port)) return iter;
      iter = iter->head.next;
    } while (iter != head_connection);
    return NULL;
  } else {
    port_connection *iter = tail_connection;
    do {
      if (iter->head.comp_ref == head_comp &&
        !strcmp(iter->head.port_name, head_port) &&
        !strcmp(iter->tail.port_name, tail_port)) return iter;
      iter = iter->tail.next;
    } while (iter != tail_connection);
    return NULL;
  }
}

void MainController::remove_all_connections(component head_or_tail)
{
  component_struct *comp = lookup_component(head_or_tail);
  while (comp->conn_head_list != NULL)
    remove_connection(comp->conn_head_list);
  while (comp->conn_tail_list != NULL)
    remove_connection(comp->conn_tail_list);
}

transport_type_enum MainController::choose_port_connection_transport(
  component head_comp, component tail_comp)
{
  host_struct *head_location = components[head_comp]->comp_location;
  // use the most efficient software loop if the two endpoints are in the
  // same component and the host supports it
  if (head_comp == tail_comp &&
    head_location->transport_supported[TRANSPORT_LOCAL])
    return TRANSPORT_LOCAL;
  host_struct *tail_location = components[tail_comp]->comp_location;
  // use the efficient UNIX domain socket if the two endpoints are on the
  // same host and it is supported by the host
  if (head_location == tail_location &&
    head_location->transport_supported[TRANSPORT_UNIX_STREAM])
    return TRANSPORT_UNIX_STREAM;
  // use TCP if it is supported by the host of both endpoints
  if (head_location->transport_supported[TRANSPORT_INET_STREAM] &&
    tail_location->transport_supported[TRANSPORT_INET_STREAM])
    return TRANSPORT_INET_STREAM;
  // no suitable transport was found, return an erroneous type
  return TRANSPORT_NUM;
}

void MainController::send_connect_ack_to_requestors(port_connection *conn)
{
  for (int i = 0; ; i++) {
    component_struct *comp = get_requestor(&conn->requestors, i);
    if (comp == NULL) break;
    else if (comp->tc_state == TC_CONNECT) {
      send_connect_ack(comp);
      if (comp == mtc) comp->tc_state = MTC_TESTCASE;
      else comp->tc_state = PTC_FUNCTION;
    }
  }
  free_requestors(&conn->requestors);
}

void MainController::send_error_to_connect_requestors(port_connection *conn,
  const char *fmt, ...)
{
  char *reason = mprintf("Establishment of port connection %d:%s - %d:%s "
    "failed because ", conn->head.comp_ref, conn->head.port_name,
    conn->tail.comp_ref, conn->tail.port_name);
  va_list ap;
  va_start(ap, fmt);
  reason = mputprintf_va_list(reason, fmt, ap);
  va_end(ap);
  for (int i = 0; ; i++) {
    component_struct *comp = get_requestor(&conn->requestors, i);
    if (comp == NULL) break;
    else if (comp->tc_state == TC_CONNECT) {
      send_error_str(comp->tc_fd, reason);
      if (comp == mtc) comp->tc_state = MTC_TESTCASE;
      else comp->tc_state = PTC_FUNCTION;
    }
  }
  Free(reason);
  free_requestors(&conn->requestors);
}

void MainController::send_disconnect_to_server(port_connection *conn)
{
  component_struct *comp = components[conn->head.comp_ref];
  switch (comp->tc_state) {
  case TC_IDLE:
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case MTC_TESTCASE:
  case PTC_FUNCTION:
  case PTC_STARTING:
  case PTC_STOPPED:
    send_disconnect(comp, conn->head.port_name, conn->tail.comp_ref,
      conn->tail.port_name);
  default:
    break;
  }
}

void MainController::send_disconnect_ack_to_requestors(port_connection *conn)
{
  for (int i = 0; ; i++) {
    component_struct *comp = get_requestor(&conn->requestors, i);
    if (comp == NULL) break;
    else if (comp->tc_state == TC_DISCONNECT) {
      send_disconnect_ack(comp);
      if (comp == mtc) comp->tc_state = MTC_TESTCASE;
      else comp->tc_state = PTC_FUNCTION;
    }
  }
  free_requestors(&conn->requestors);
}

void MainController::init_requestors(requestor_struct *reqs,
  component_struct *tc)
{
  if (tc != NULL) {
    reqs->n_components = 1;
    reqs->the_component = tc;
  } else reqs->n_components = 0;
}

void MainController::add_requestor(requestor_struct *reqs, component_struct *tc)
{
  switch (reqs->n_components) {
  case 0:
    reqs->n_components = 1;
    reqs->the_component = tc;
    break;
  case 1:
    if (reqs->the_component != tc) {
      reqs->n_components = 2;
      component_struct *tmp = reqs->the_component;
      reqs->components =
        (component_struct**)Malloc(2 * sizeof(*reqs->components));
      reqs->components[0] = tmp;
      reqs->components[1] = tc;
    }
    break;
  default:
    for (int i = 0; i < reqs->n_components; i++)
      if (reqs->components[i] == tc) return;
    reqs->n_components++;
    reqs->components = (component_struct**)Realloc(reqs->components,
      reqs->n_components * sizeof(*reqs->components));
    reqs->components[reqs->n_components - 1] = tc;
  }
}

void MainController::remove_requestor(requestor_struct *reqs,
  component_struct *tc)
{
  switch (reqs->n_components) {
  case 0:
    break;
  case 1:
    if (reqs->the_component == tc) reqs->n_components = 0;
    break;
  case 2: {
    component_struct *tmp = NULL;
    if (reqs->components[0] == tc) tmp = reqs->components[1];
    else if (reqs->components[1] == tc) tmp = reqs->components[0];
    if (tmp != NULL) {
      Free(reqs->components);
      reqs->n_components = 1;
      reqs->the_component = tmp;
    }
    break; }
  default:
    for (int i = 0; i < reqs->n_components; i++)
      if (reqs->components[i] == tc) {
        reqs->n_components--;
        memmove(reqs->components + i, reqs->components + i + 1,
          (reqs->n_components - i) * sizeof(*reqs->components));
        reqs->components = (component_struct**)Realloc(reqs->components,
          reqs->n_components * sizeof(*reqs->components));
        break;
      }
  }
}

boolean MainController::has_requestor(const requestor_struct *reqs,
  component_struct *tc)
{
  switch (reqs->n_components) {
  case 0:
    return FALSE;
  case 1:
    return reqs->the_component == tc;
  default:
    for (int i = 0; i < reqs->n_components; i++)
      if (reqs->components[i] == tc) return TRUE;
    return FALSE;
  }
}

component_struct *MainController::get_requestor(const requestor_struct *reqs,
  int index)
{
  if (index >= 0 && index < reqs->n_components) {
    if (reqs->n_components == 1) return reqs->the_component;
    else return reqs->components[index];
  } else return NULL;
}

void MainController::free_requestors(requestor_struct *reqs)
{
  if (reqs->n_components > 1) Free(reqs->components);
  reqs->n_components = 0;
}

void MainController::init_qualified_name(qualified_name *name)
{
  name->module_name = NULL;
  name->definition_name = NULL;
}

void MainController::free_qualified_name(qualified_name *name)
{
  delete [] name->module_name;
  name->module_name = NULL;
  delete [] name->definition_name;
  name->definition_name = NULL;
}

double MainController::kill_timer;

double MainController::time_now()
{
  static boolean first_call = TRUE;
  static struct timeval first_time;
  if (first_call) {
    first_call = FALSE;
    if (gettimeofday(&first_time, NULL) < 0)
      fatal_error("MainController::time_now: gettimeofday() system call "
        "failed.");
    return 0.0;
  } else {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0)
      fatal_error("MainController::time_now: gettimeofday() system call "
        "failed.");
    return (tv.tv_sec - first_time.tv_sec) +
      1.0e-6 * (tv.tv_usec - first_time.tv_usec);
  }
}

timer_struct *MainController::timer_head, *MainController::timer_tail;

void MainController::register_timer(timer_struct *timer)
{
  timer_struct *iter;
  for (iter = timer_tail; iter != NULL; iter = iter->prev)
    if (iter->expiration <= timer->expiration) break;
  if (iter != NULL) {
    // inserting after iter
    timer->prev = iter;
    timer->next = iter->next;
    if (iter->next != NULL) iter->next->prev = timer;
    else timer_tail = timer;
    iter->next = timer;
  } else {
    // inserting at the beginning of list
    timer->prev = NULL;
    timer->next = timer_head;
    if (timer_head != NULL) timer_head->prev = timer;
    else timer_tail = timer;
    timer_head = timer;
  }
}

void MainController::cancel_timer(timer_struct *timer)
{
  if (timer->next != NULL) timer->next->prev = timer->prev;
  else timer_tail = timer->prev;
  if (timer->prev != NULL) timer->prev->next = timer->next;
  else timer_head = timer->next;
  delete timer;
}

int MainController::get_poll_timeout()
{
  if (timer_head != NULL) {
    double offset = timer_head->expiration - time_now();
    if (offset > 0.0) {
      return static_cast<int>(1000.0 * offset);
    } else {
      return 0;
    }
  } else {
    return -1;
  }
}

void MainController::handle_expired_timers()
{
  if (timer_head != NULL) {
    timer_struct *iter = timer_head;
    double now = time_now();
    do {
      if (iter->expiration > now) break;
      timer_struct *next = iter->next;
      handle_kill_timer(iter);
      iter = next;
    } while (iter != NULL);
  }
}

void MainController::handle_kill_timer(timer_struct *timer)
{
  component_struct *tc = timer->timer_argument.component_ptr;
  host_struct *host = tc->comp_location;
  boolean kill_process = FALSE;
  switch (tc->tc_state) {
  case TC_EXITED:
    // do nothing
    break;
  case TC_EXITING:
    if (tc == mtc) {
      error("MTC on host %s did not close its control connection in "
        "time. Trying to kill it using its HC.", host->hostname);
    } else {
      notify("PTC %d on host %s did not close its control connection in "
        "time. Trying to kill it using its HC.", tc->comp_ref,
        host->hostname);
    }
    kill_process = TRUE;
    break;
  case TC_STOPPING:
  case PTC_STOPPING_KILLING:
  case PTC_KILLING:
    // active PTCs with kill timer can be only in these states
    if (tc != mtc) {
      notify("PTC %d on host %s is not responding. Trying to kill it "
        "using its HC.", tc->comp_ref, host->hostname);
      kill_process = TRUE;
      break;
    }
    // no break
  default:
    // MTC can be in any state
    if (tc == mtc) {
      error("MTC on host %s is not responding. Trying to kill it using "
        "its HC. This will abort test execution.", host->hostname);
      kill_process = TRUE;
    } else {
      error("PTC %d is in invalid state when its kill timer expired.",
        tc->comp_ref);
    }
  }
  if (kill_process) {
    if (host->hc_state == HC_ACTIVE) {
      send_kill_process(host, tc->comp_ref);
      tc->process_killed = TRUE;
    } else {
      error("Test Component %d cannot be killed because the HC on host "
        "%s is not in active state. Kill the process manually or the "
        "test system may get into a deadlock.", tc->comp_ref,
        host->hostname);
    }
  }
  cancel_timer(timer);
  tc->kill_timer = NULL;
}

void MainController::register_termination_handlers()
{
  new_action.sa_handler = termination_handler;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = 0;

  sigaction(SIGINT, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGINT, &new_action, NULL);
  sigaction(SIGHUP, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGHUP, &new_action, NULL);
  sigaction(SIGTERM, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction(SIGTERM, &new_action, NULL);
  sigaction(SIGQUIT, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction(SIGQUIT, &new_action, NULL);
  sigaction(SIGKILL, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction(SIGKILL, &new_action, NULL);
}

void MainController::termination_handler(int signum)
{
  // Call shutdown_server() and reset handlers and re-raise the signal.
  // clean_up() or perform_shutdown() is state dependent and cannot be used
  // here... Related to HP67376.
  shutdown_server();

  new_action.sa_handler = SIG_DFL;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = 0;

  sigaction(SIGINT, &new_action, NULL);
  sigaction(SIGHUP, &new_action, NULL);
  sigaction(SIGTERM, &new_action, NULL);
  sigaction(SIGQUIT, &new_action, NULL);
  sigaction(SIGKILL, &new_action, NULL);

  raise(signum);
}

void MainController::error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *str = mprintf_va_list(fmt, ap);
  va_end(ap);
  unlock();
  ui->error(/*severity*/ 0, str);
  lock();
  Free(str);
}

void MainController::notify(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *str = mprintf_va_list(fmt, ap);
  va_end(ap);
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) fatal_error("MainController::notify: "
    "gettimeofday() system call failed.");
  notify(&tv, mc_hostname, TTCN_EXECUTOR, str);
  Free(str);
}

void MainController::notify(const struct timeval *timestamp,
  const char *source, int severity, const char *message)
{
  unlock();
  ui->notify(timestamp, source, severity, message);
  lock();
}

void MainController::status_change()
{
  unlock();
  ui->status_change();
  lock();
}

void MainController::fatal_error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  if (errno != 0) fprintf(stderr, " (%s)", strerror(errno));
  putc('\n', stderr);
  exit(EXIT_FAILURE);
}

void *MainController::thread_main(void *)
{
  lock();
  while (mc_state != MC_INACTIVE) {
    int fds_selected;
    for ( ; ; ) {
      int maxDtInMs = 0;
#ifdef USE_EPOLL
      int timeout = get_poll_timeout();
      if (maxDtInMs != 0 && (timeout < 0 || maxDtInMs < timeout))
        timeout = maxDtInMs;
      unlock();
      fds_selected = epoll_wait(epfd, epoll_events, EPOLL_MAX_EVENTS,
        timeout);
      lock();
      if (fds_selected >= 0) break;
      if (errno != EINTR) fatal_error("epoll_wait() system call failed.");
#else // ! defined USE_EPOLL
      update_pollfds();
      int timeout = get_poll_timeout();
      if (maxDtInMs != 0 && (timeout < 0 || maxDtInMs < timeout))
        timeout = maxDtInMs;
      unlock();
      fds_selected = poll(ufds, nfds, timeout);
      lock();
      if (fds_selected >= 0) break;
      if (errno != EINTR) fatal_error("poll() system call failed.");
#endif //USE_EPOLL
      errno = 0;
    }
    switch (wakeup_reason) {
    case REASON_NOTHING:
    case REASON_MTC_KILL_TIMER:
      break;
    case REASON_SHUTDOWN:
      wakeup_reason = REASON_NOTHING;
      perform_shutdown();
      continue;
    default:
      error("Invalid wakeup reason (%d) was set.", wakeup_reason);
      wakeup_reason = REASON_NOTHING;
    }
    if (fds_selected == 0) {
      handle_expired_timers();
      continue;
    }
#ifdef USE_EPOLL
    for (int i = 0; i < fds_selected; i++) {
      int fd = epoll_events[i].data.fd;
      if (epoll_events[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR)) {
        dispatch_socket_event(fd);
      }
    }
#else // ! defined USE_EPOLL
    for (unsigned int i = 0; i < nfds; i++) {
      int fd = ufds[i].fd;
      if (ufds[i].revents & POLLNVAL) {
        fatal_error("Invalid file descriptor (%d) was given for "
          "poll() system call.", fd);
      } else if (ufds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
        dispatch_socket_event(fd);
      }
    }
#endif //USE_EPOLL
    handle_expired_timers();
  }
  clean_up();
  notify("Shutdown complete.");
  unlock();
  // don't try to lock the mutex after ui->status_change() is completed
  // the main thread might call in turn terminate(), which destroys the mutex
  ui->status_change();
  return NULL;
}

void MainController::dispatch_socket_event(int fd)
{
  // a previous event might have closed the socket
  if (fd >= fd_table_size) return;
  switch (fd_table[fd].fd_type) {
  case FD_PIPE:
    handle_pipe();
    break;
  case FD_SERVER:
    handle_incoming_connection(fd);
    break;
  case FD_UNKNOWN:
    handle_unknown_data(fd_table[fd].unknown_ptr);
    break;
  case FD_HC:
    handle_hc_data(fd_table[fd].host_ptr, TRUE);
    break;
  case FD_TC:
    handle_tc_data(fd_table[fd].component_ptr, TRUE);
    break;
  default:
    fatal_error("Invalid file descriptor type (%d) for "
      "file descriptor %d.", fd_table[fd].fd_type, fd);
  }
}

int MainController::pipe_fd[2];
wakeup_reason_t MainController::wakeup_reason;

void MainController::wakeup_thread(wakeup_reason_t reason)
{
  unsigned char msg = '\0';
  if (write(pipe_fd[1], &msg, 1) != 1) {
    fatal_error("MainController::wakeup_thread: writing to pipe failed.");
  }
  wakeup_reason = reason;
}

void MainController::handle_pipe()
{
  unsigned char buf;
  if (read(pipe_fd[0], &buf, 1) != 1) {
    fatal_error("MainController::handle_pipe: reading from pipe failed.");
  }
}

void MainController::handle_incoming_connection(int p_server_fd)
{
  IPAddress *remote_addr = IPAddress::create_addr(nh.get_family());
  int fd = remote_addr->accept(p_server_fd);
  if (fd > 0) {
    set_close_on_exec(fd);
    unknown_connection *new_connection =
      new_unknown_connection(p_server_fd != MainController::server_fd);
    new_connection->fd = fd;
    if (p_server_fd == MainController::server_fd)
      new_connection->ip_addr = remote_addr;
    else { // in case of unix domain socket connection
      delete remote_addr;
      new_connection->ip_addr = IPAddress::create_addr("127.0.0.1");
    }
    new_connection->text_buf = new Text_Buf;
    add_poll_fd(fd);
    add_fd_to_table(fd);
    fd_table[fd].fd_type = FD_UNKNOWN;
    fd_table[fd].unknown_ptr = new_connection;
  } else {
    delete remote_addr;
    switch (errno) {
    case EINTR:
      errno = 0;
      break;
    case EMFILE:
    case ENFILE:
      error("New incoming connection cannot be accepted "
        "because the maximum number of open files has been reached. "
        "Try to increase this limit.");
      disable_server_fd();
      error("No incoming connections will be accepted until at least "
        "one component terminates. This may result in deadlock.");
      break;
    default:
      fatal_error("MainController::handle_incoming_connection: "
        "system call accept() failed.");
    }
  }
}

int MainController::recv_to_buffer(int fd, Text_Buf& text_buf,
  boolean recv_from_socket)
{
  // if recv_from_socket is false we are checking the messages that are
  // already in text_buf so we are emulating that recv() was successful
  if (!recv_from_socket) return 1;

  char *buf_ptr;
  int buf_len;
  text_buf.get_end(buf_ptr, buf_len);

  int recv_len = recv(fd, buf_ptr, buf_len, 0);

  if (recv_len > 0) text_buf.increase_length(recv_len);

  return recv_len;
}

void MainController::handle_unknown_data(unknown_connection *conn)
{
  Text_Buf& text_buf = *conn->text_buf;
  int recv_len = recv_to_buffer(conn->fd, text_buf, TRUE);
  boolean error_flag = FALSE;

  if (recv_len > 0) {
    try {
      while (text_buf.is_message()) {
        text_buf.pull_int(); // message length
        int message_type = text_buf.pull_int().get_val();
        // only the first message is processed in this loop
        // except when a generic message is received
        boolean process_more_messages = FALSE;
        switch (message_type) {
        case MSG_ERROR:
          process_error(conn);
          process_more_messages = TRUE;
          break;
        case MSG_LOG:
          process_log(conn);
          process_more_messages = TRUE;
          break;
        case MSG_VERSION:
          process_version(conn);
          break;
        case MSG_MTC_CREATED:
          process_mtc_created(conn);
          break;
        case MSG_PTC_CREATED:
          process_ptc_created(conn);
          break;
        default:
          error("Invalid message type (%d) was received on an "
            "unknown connection from %s [%s].", message_type,
            conn->ip_addr->get_host_str(),
            conn->ip_addr->get_addr_str());
          error_flag = TRUE;
        }
        if (process_more_messages) text_buf.cut_message();
        else break;
      }
    } catch (const TC_Error& tc_error) {
      error("Maleformed message was received on an unknown connection "
        "from %s [%s].", conn->ip_addr->get_host_str(),
        conn->ip_addr->get_addr_str());
      error_flag = TRUE;
    }
    if (error_flag) {
      send_error_str(conn->fd, "The received message was not understood "
        "by the MC.");
    }
  } else if (recv_len == 0) {
    error("Unexpected end of an unknown connection from %s [%s].",
      conn->ip_addr->get_host_str(), conn->ip_addr->get_addr_str());
    error_flag = TRUE;
  } else {
    error("Receiving of data failed on an unknown connection from %s [%s].",
      conn->ip_addr->get_host_str(), conn->ip_addr->get_addr_str());
    error_flag = TRUE;
  }
  if (error_flag) {
    close_unknown_connection(conn);
  }
}

void MainController::handle_hc_data(host_struct *hc, boolean recv_from_socket)
{
  Text_Buf& text_buf = *hc->text_buf;
  boolean error_flag = FALSE;
  int recv_len = recv_to_buffer(hc->hc_fd, text_buf, recv_from_socket);

  if (recv_len > 0) {
    try {
      while (text_buf.is_message()) {
        int msg_len = text_buf.pull_int().get_val();
        int msg_end = text_buf.get_pos() + msg_len;
        int message_type = text_buf.pull_int().get_val();
        switch (message_type) {
        case MSG_ERROR:
          process_error(hc);
          break;
        case MSG_LOG:
          process_log(hc);
          break;
        case MSG_CONFIGURE_ACK:
          process_configure_ack(hc);
          break;
        case MSG_CONFIGURE_NAK:
          process_configure_nak(hc);
          break;
        case MSG_CREATE_NAK:
          process_create_nak(hc);
          break;
        case MSG_HC_READY:
          process_hc_ready(hc);
          break;
        case MSG_DEBUG_RETURN_VALUE:
          process_debug_return_value(*hc->text_buf, hc->log_source, msg_end, false);
          break;
        default:
          error("Invalid message type (%d) was received on HC "
            "connection from %s [%s].", message_type,
            hc->hostname, hc->ip_addr->get_addr_str());
          error_flag = TRUE;
        }
        if (error_flag) break;
        text_buf.cut_message();
      }
    } catch (const TC_Error& tc_error) {
      error("Malformed message was received on HC connection "
        "from %s [%s].", hc->hostname, hc->ip_addr->get_addr_str());
      error_flag = TRUE;
    }
    if (error_flag) {
      send_error_str(hc->hc_fd, "The received message was not understood "
        "by the MC.");
    }
  } else if (recv_len == 0) {
    if (hc->hc_state == HC_EXITING) {
      close_hc_connection(hc);
      if (mc_state == MC_SHUTDOWN && all_hc_in_state(HC_DOWN))
        mc_state = MC_INACTIVE;
    } else {
      error("Unexpected end of HC connection from %s [%s].",
        hc->hostname, hc->ip_addr->get_addr_str());
      error_flag = TRUE;
    }
  } else {
    error("Receiving of data failed on HC connection from %s [%s].",
      hc->hostname, hc->ip_addr->get_addr_str());
    error_flag = TRUE;
  }
  if (error_flag) {
    close_hc_connection(hc);
    switch (mc_state) {
    case MC_INACTIVE:
    case MC_LISTENING:
    case MC_LISTENING_CONFIGURED:
      fatal_error("MC is in invalid state when a HC connection "
        "terminated.");
    case MC_HC_CONNECTED:
      if (all_hc_in_state(HC_DOWN)) mc_state = MC_LISTENING;
      break;
    case MC_CONFIGURING:
    case MC_RECONFIGURING:
      check_all_hc_configured();
      break;
    case MC_ACTIVE:
      if (all_hc_in_state(HC_DOWN)) mc_state = MC_LISTENING_CONFIGURED;
      else if (!is_hc_in_state(HC_ACTIVE) &&
        !is_hc_in_state(HC_OVERLOADED)) mc_state = MC_HC_CONNECTED;
      break;
    default:
      if (!is_hc_in_state(HC_ACTIVE)) notify("There is no active HC "
        "connection. Further create operations will fail.");
    }
    status_change();
  }
}

void MainController::handle_tc_data(component_struct *tc,
  boolean recv_from_socket)
{
  Text_Buf& text_buf = *tc->text_buf;
  boolean close_connection = FALSE;
  int recv_len = recv_to_buffer(tc->tc_fd, text_buf, recv_from_socket);

  if (recv_len > 0) {
    try {
      while (text_buf.is_message()) {
        int message_len = text_buf.pull_int().get_val();
        int message_end = text_buf.get_pos() + message_len;
        int message_type = text_buf.pull_int().get_val();
        // these messages can be received both from MTC and PTCs
        switch (message_type) {
        case MSG_ERROR:
          process_error(tc);
          break;
        case MSG_LOG:
          process_log(tc);
          break;
        case MSG_CREATE_REQ:
          process_create_req(tc);
          break;
        case MSG_START_REQ:
          process_start_req(tc, message_end);
          break;
        case MSG_STOP_REQ:
          process_stop_req(tc);
          break;
        case MSG_KILL_REQ:
          process_kill_req(tc);
          break;
        case MSG_IS_RUNNING:
          process_is_running(tc);
          break;
        case MSG_IS_ALIVE:
          process_is_alive(tc);
          break;
        case MSG_DONE_REQ:
          process_done_req(tc);
          break;
        case MSG_KILLED_REQ:
          process_killed_req(tc);
          break;
        case MSG_CANCEL_DONE_ACK:
          process_cancel_done_ack(tc);
          break;
        case MSG_CONNECT_REQ:
          process_connect_req(tc);
          break;
        case MSG_CONNECT_LISTEN_ACK:
          process_connect_listen_ack(tc, message_end);
          break;
        case MSG_CONNECTED:
          process_connected(tc);
          break;
        case MSG_CONNECT_ERROR:
          process_connect_error(tc);
          break;
        case MSG_DISCONNECT_REQ:
          process_disconnect_req(tc);
          break;
        case MSG_DISCONNECTED:
          process_disconnected(tc);
          break;
        case MSG_MAP_REQ:
          process_map_req(tc);
          break;
        case MSG_MAPPED:
          process_mapped(tc);
          break;
        case MSG_UNMAP_REQ:
          process_unmap_req(tc);
          break;
        case MSG_UNMAPPED:
          process_unmapped(tc);
          break;
        case MSG_DEBUG_RETURN_VALUE:
          process_debug_return_value(*tc->text_buf, tc->log_source, message_end,
            tc == mtc);
          break;
        case MSG_DEBUG_HALT_REQ:
          process_debug_broadcast_req(tc, D_HALT);
          break;
        case MSG_DEBUG_CONTINUE_REQ:
          process_debug_broadcast_req(tc, D_CONTINUE);
          break;
        case MSG_DEBUG_BATCH:
          process_debug_batch(tc);
          break;
        default:
          if (tc == mtc) {
            // these messages can be received only from the MTC
            switch (message_type) {
            case MSG_TESTCASE_STARTED:
              process_testcase_started();
              break;
            case MSG_TESTCASE_FINISHED:
              process_testcase_finished();
              break;
            case MSG_MTC_READY:
              process_mtc_ready();
              break;
            case MSG_CONFIGURE_ACK:
              process_configure_ack_mtc();
              break;
            case MSG_CONFIGURE_NAK:
              process_configure_nak_mtc();
              break;
            default:
              error("Invalid message type (%d) was received "
                "from the MTC at %s [%s].", message_type,
                mtc->comp_location->hostname,
                mtc->comp_location->ip_addr->get_addr_str());
              close_connection = TRUE;
            }
          } else {
            // these messages can be received only from PTCs
            switch (message_type) {
            case MSG_STOPPED:
              process_stopped(tc, message_end);
              break;
            case MSG_STOPPED_KILLED:
              process_stopped_killed(tc, message_end);
              break;
            case MSG_KILLED:
              process_killed(tc);
              break;
            default:
              notify("Invalid message type (%d) was received from "
                "PTC %d at %s [%s].", message_type,
                tc->comp_ref, tc->comp_location->hostname,
                tc->comp_location->ip_addr->get_addr_str());
              close_connection = TRUE;
            }
          }
        }
        if (close_connection) break;
        text_buf.cut_message();
      }
    } catch (const TC_Error& tc_error) {
      if (tc == mtc) {
        error("Malformed message was received from the MTC at %s "
          "[%s].", mtc->comp_location->hostname,
          mtc->comp_location->ip_addr->get_addr_str());
      } else {
        notify("Malformed message was received from PTC %d at %s [%s].",
          tc->comp_ref, tc->comp_location->hostname,
          tc->comp_location->ip_addr->get_addr_str());
      }
      close_connection = TRUE;
    }
    if (close_connection) {
      send_error_str(tc->tc_fd, "The received message was not understood "
        "by the MC.");
    }
  } else if (recv_len == 0) {
    // TCP connection is closed by peer
    if (tc->tc_state != TC_EXITING && !tc->process_killed) {
      if (tc == mtc) {
        error("Unexpected end of MTC connection from %s [%s].",
          mtc->comp_location->hostname,
          mtc->comp_location->ip_addr->get_addr_str());
      } else {
        notify("Unexpected end of PTC connection (%d) from %s [%s].",
          tc->comp_ref, tc->comp_location->hostname,
          tc->comp_location->ip_addr->get_addr_str());
      }
    }
    close_connection = TRUE;
  } else {
    if (tc->process_killed && errno == ECONNRESET) {
      // ignore TCP resets if the process was killed
      // because the last STOP or KILL message can stuck in TCP buffers
      // if the process did not receive any data
    } else {
      if (tc == mtc) {
        error("Receiving of data failed from the MTC at %s [%s]: %s",
          mtc->comp_location->hostname,
          mtc->comp_location->ip_addr->get_addr_str(), strerror(errno));
      } else {
        notify("Receiving of data failed from PTC %d at %s [%s]: %s",
          tc->comp_ref, tc->comp_location->hostname,
          tc->comp_location->ip_addr->get_addr_str(), strerror(errno));
      }
    }
    close_connection = TRUE;
  }
  if (close_connection) {
    close_tc_connection(tc);
    remove_component_from_host(tc);
    if (tc == mtc) {
      if (mc_state != MC_TERMINATING_MTC) {
        notify("The control connection to MTC is lost. "
          "Destroying all PTC connections.");
      }
      destroy_all_components();
      notify("MTC terminated.");
      if (is_hc_in_state(HC_CONFIGURING)) mc_state = MC_CONFIGURING;
      else if (is_hc_in_state(HC_IDLE)) mc_state = MC_HC_CONNECTED;
      else if (is_hc_in_state(HC_ACTIVE) ||
        is_hc_in_state(HC_OVERLOADED)) mc_state = MC_ACTIVE;
      else mc_state = MC_LISTENING_CONFIGURED;
      stop_requested = FALSE;
    } else {
      if (tc->tc_state != TC_EXITING) {
        // we have no idea about the final verdict of the PTC
        tc->local_verdict = ERROR;
        component_terminated(tc);
      }
      tc->tc_state = TC_EXITED;
      if (mc_state == MC_TERMINATING_TESTCASE &&
        ready_to_finish_testcase()) finish_testcase();
    }
    status_change();
  }
}

void MainController::unlink_unix_socket(int socket_fd) {
  struct sockaddr_un local_addr;
  // querying the local pathname used by socket_fd
  socklen_type addr_len = sizeof(local_addr);
  if (getsockname(socket_fd, (struct sockaddr*)&local_addr, &addr_len)) {
  } else if (local_addr.sun_family != AF_UNIX) {
  } else if (unlink(local_addr.sun_path)) {
    errno = 0;
  }
}

void MainController::shutdown_server()
{
  if (server_fd >= 0) {
    remove_poll_fd(server_fd);
    remove_fd_from_table(server_fd);
    close(server_fd);
    server_fd = -1;
  }

  if (server_fd_unix >= 0) {
    unlink_unix_socket(server_fd_unix);
    remove_poll_fd(server_fd_unix);
    remove_fd_from_table(server_fd_unix);
    close(server_fd_unix);
    server_fd_unix = -1;
  }
}

void MainController::perform_shutdown()
{
  boolean shutdown_complete = TRUE;
  switch (mc_state) {
  case MC_HC_CONNECTED:
  case MC_ACTIVE:
    for (int i = 0; i < n_hosts; i++) {
      host_struct *host = hosts[i];
      if (host->hc_state != HC_DOWN) {
        send_exit_hc(host);
        host->hc_state = HC_EXITING;
        shutdown_complete = FALSE;
      }
    }
    // no break
  case MC_LISTENING:
  case MC_LISTENING_CONFIGURED:
    shutdown_server();
    // don't call status_change() if shutdown is complete
    // it will be called from thread_main() later
    if (shutdown_complete) mc_state = MC_INACTIVE;
    else {
      mc_state = MC_SHUTDOWN;
      status_change();
    }
    break;
  default:
    fatal_error("MainController::perform_shutdown: called in wrong state.");
  }
}

void MainController::clean_up()
{
  shutdown_server();

  while (unknown_head != NULL) close_unknown_connection(unknown_head);

  destroy_all_components();

  for (int i = 0; i < n_hosts; i++) {
    host_struct *host = hosts[i];
    close_hc_connection(host);
    Free(host->hostname);
    delete host->ip_addr;
    delete [] host->hostname_local;
    delete [] host->machine_type;
    delete [] host->system_name;
    delete [] host->system_release;
    delete [] host->system_version;
    Free(host->log_source);
    Free(host->components);
    free_string_set(&host->allowed_components);
    delete host;
  }
  Free(hosts);
  n_hosts = 0;
  hosts = NULL;
  Free(config_str);
  config_str = NULL;
  
  Free(debugger_settings.on_switch);
  debugger_settings.on_switch = NULL;
  Free(debugger_settings.output_type);
  debugger_settings.output_type = NULL;
  Free(debugger_settings.output_file);
  debugger_settings.output_file = NULL;
  Free(debugger_settings.error_behavior);
  debugger_settings.error_behavior = NULL;
  Free(debugger_settings.error_batch_file);
  debugger_settings.error_batch_file = NULL;
  Free(debugger_settings.fail_behavior);
  debugger_settings.fail_behavior = NULL;
  Free(debugger_settings.fail_batch_file);
  debugger_settings.fail_batch_file = NULL;
  Free(debugger_settings.global_batch_state);
  debugger_settings.global_batch_state = NULL;
  Free(debugger_settings.global_batch_file);
  debugger_settings.global_batch_file = NULL;
  Free(debugger_settings.function_calls_cfg);
  debugger_settings.function_calls_cfg = NULL;
  Free(debugger_settings.function_calls_file);
  debugger_settings.function_calls_file = NULL;
  for (int i = 0; i < debugger_settings.nof_breakpoints; ++i) {
    Free(debugger_settings.breakpoints[i].module);
    Free(debugger_settings.breakpoints[i].line);
    Free(debugger_settings.breakpoints[i].batch_file);
  }
  debugger_settings.nof_breakpoints = 0;
  Free(debugger_settings.breakpoints);
  debugger_settings.breakpoints = NULL;
  Free(last_debug_command.arguments);
  last_debug_command.arguments = NULL;

  while (timer_head != NULL) cancel_timer(timer_head);

  for (int i = 0; i < n_modules; i++) {
    delete [] modules[i].module_name;
    delete [] modules[i].module_checksum;
  }
  delete [] modules;
  n_modules = 0;
  modules = NULL;
  version_known = FALSE;

#ifdef USE_EPOLL
  if (epfd >= 0) {
    if (close(epfd) < 0)
      error("MainController::clean_up: Error while closing epoll"
        " fd %d", epfd);
    epfd = -1;
  }
  Free(epoll_events);
  epoll_events = NULL;
#else // ! defined USE_EPOLL
  nfds = 0;
  Free(ufds);
  ufds = NULL;
  new_nfds = 0;
  Free(new_ufds);
  new_ufds = NULL;
  pollfds_modified = FALSE;
#endif

  fd_table_size = 0;
  Free(fd_table);
  fd_table = NULL;

  mc_state = MC_INACTIVE;

  if (pipe_fd[1] >= 0) {
    close(pipe_fd[1]);
    pipe_fd[1] = -1;
  }
  if (pipe_fd[0] >= 0) {
    close(pipe_fd[1]);
    pipe_fd[0] = -1;
  }
}

void MainController::send_configure(host_struct *hc, const char *config_file)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONFIGURE);
  text_buf.push_string(config_file);
  send_message(hc->hc_fd, text_buf);
}

void MainController::send_exit_hc(host_struct *hc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_EXIT_HC);
  send_message(hc->hc_fd, text_buf);
}

void MainController::send_create_mtc(host_struct *hc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CREATE_MTC);
  send_message(hc->hc_fd, text_buf);
}

void MainController::send_create_ptc(host_struct *hc,
  component component_reference, const qualified_name& component_type,
  const char *component_name, boolean is_alive,
  const qualified_name& current_testcase)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CREATE_PTC);
  text_buf.push_int(component_reference);
  text_buf.push_qualified_name(component_type);
  text_buf.push_string(component_name);
  text_buf.push_int(is_alive ? 1 : 0);
  text_buf.push_qualified_name(current_testcase);
  send_message(hc->hc_fd, text_buf);
}

void MainController::send_kill_process(host_struct *hc,
  component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_KILL_PROCESS);
  text_buf.push_int(component_reference);
  send_message(hc->hc_fd, text_buf);
}

void MainController::send_create_ack(component_struct *tc,
  component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CREATE_ACK);
  text_buf.push_int(component_reference);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_start_ack(component_struct *tc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_START_ACK);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_stop(component_struct *tc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_STOP);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_stop_ack(component_struct *tc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_STOP_ACK);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_kill_ack(component_struct *tc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_KILL_ACK);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_running(component_struct *tc, boolean answer)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_RUNNING);
  text_buf.push_int(answer ? 1 : 0);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_alive(component_struct *tc, boolean answer)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_ALIVE);
  text_buf.push_int(answer ? 1 : 0);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_done_ack(component_struct *tc, boolean answer,
  const char *return_type, int return_value_len, const void *return_value)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DONE_ACK);
  text_buf.push_int(answer ? 1 : 0);
  text_buf.push_string(return_type);
  text_buf.push_raw(return_value_len, return_value);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_killed_ack(component_struct *tc, boolean answer)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_KILLED_ACK);
  text_buf.push_int(answer ? 1 : 0);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_connect_listen(component_struct *tc,
  const char *local_port, component remote_comp, const char *remote_comp_name,
  const char *remote_port, transport_type_enum transport_type)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONNECT_LISTEN);
  text_buf.push_string(local_port);
  text_buf.push_int(remote_comp);
  text_buf.push_string(remote_comp_name);
  text_buf.push_string(remote_port);
  text_buf.push_int(transport_type);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_connect(component_struct *tc,
  const char *local_port, component remote_comp, const char *remote_comp_name,
  const char *remote_port, transport_type_enum transport_type,
  int remote_address_len, const void *remote_address)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONNECT);
  text_buf.push_string(local_port);
  text_buf.push_int(remote_comp);
  text_buf.push_string(remote_comp_name);
  text_buf.push_string(remote_port);
  text_buf.push_int(transport_type);
  text_buf.push_raw(remote_address_len, remote_address);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_connect_ack(component_struct *tc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONNECT_ACK);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_disconnect(component_struct *tc,
  const char *local_port, component remote_comp, const char *remote_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DISCONNECT);
  text_buf.push_string(local_port);
  text_buf.push_int(remote_comp);
  text_buf.push_string(remote_port);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_disconnect_ack(component_struct *tc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DISCONNECT_ACK);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_map(component_struct *tc,
  const char *local_port, const char *system_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_MAP);
  text_buf.push_string(local_port);
  text_buf.push_string(system_port);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_map_ack(component_struct *tc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_MAP_ACK);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_unmap(component_struct *tc,
  const char *local_port, const char *system_port)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_UNMAP);
  text_buf.push_string(local_port);
  text_buf.push_string(system_port);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_unmap_ack(component_struct *tc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_UNMAP_ACK);
  send_message(tc->tc_fd, text_buf);
}

static void get_next_argument_loc(const char* arguments, size_t len, size_t& start, size_t& end)
{
  while (start < len && isspace(arguments[start])) {
    ++start;
  }
  end = start;
  while (end < len && !isspace(arguments[end])) {
    ++end;
  }
}

void MainController::send_debug_command(int fd, int commandID, const char* arguments)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DEBUG_COMMAND);
  text_buf.push_int(commandID);
  
  size_t arg_len = strlen(arguments);
  int arg_count = 0;
  for (size_t i = 0; i < arg_len; ++i) {
    if (isspace(arguments[i]) && (i == 0 || !isspace(arguments[i - 1]))) {
      ++arg_count;
    }
  }
  if (arg_len > 0) {
    ++arg_count;
  }
  text_buf.push_int(arg_count);
  
  if (arg_count > 0) {
    size_t start = 0;
    size_t end = 0;
    while (start < arg_len) {
      get_next_argument_loc(arguments, arg_len, start, end);
      // don't use push_string, as that requires a null-terminated string
      text_buf.push_int(end - start);
      text_buf.push_raw(end - start, arguments + start);
      start = end;
    }
  }
  
  send_message(fd, text_buf);
}

void MainController::send_debug_setup(host_struct *hc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_DEBUG_COMMAND);
  text_buf.push_int(D_SETUP);
  text_buf.push_int(11 + 3 * debugger_settings.nof_breakpoints);
  text_buf.push_string(debugger_settings.on_switch);
  text_buf.push_string(debugger_settings.output_file);
  text_buf.push_string(debugger_settings.output_type);
  text_buf.push_string(debugger_settings.error_behavior);
  text_buf.push_string(debugger_settings.error_batch_file);
  text_buf.push_string(debugger_settings.fail_behavior);
  text_buf.push_string(debugger_settings.fail_batch_file);
  text_buf.push_string(debugger_settings.global_batch_state);
  text_buf.push_string(debugger_settings.global_batch_file);
  text_buf.push_string(debugger_settings.function_calls_cfg);
  text_buf.push_string(debugger_settings.function_calls_file);
  for (int i = 0; i < debugger_settings.nof_breakpoints; ++i) {
    text_buf.push_string(debugger_settings.breakpoints[i].module);
    text_buf.push_string(debugger_settings.breakpoints[i].line);
    text_buf.push_string(debugger_settings.breakpoints[i].batch_file);
  }
  send_message(hc->hc_fd, text_buf);
}

void MainController::send_cancel_done_mtc(component component_reference,
  boolean cancel_any)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CANCEL_DONE);
  text_buf.push_int(component_reference);
  text_buf.push_int(cancel_any ? 1 : 0);
  send_message(mtc->tc_fd, text_buf);
}

void MainController::send_component_status_mtc(component component_reference,
  boolean is_done, boolean is_killed, boolean is_any_done,
  boolean is_all_done, boolean is_any_killed, boolean is_all_killed,
  const char *return_type, int return_value_len, const void *return_value)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_COMPONENT_STATUS);
  text_buf.push_int(component_reference);
  text_buf.push_int(is_done ? 1 : 0);
  text_buf.push_int(is_killed ? 1 : 0);
  text_buf.push_int(is_any_done ? 1 : 0);
  text_buf.push_int(is_all_done ? 1 : 0);
  text_buf.push_int(is_any_killed ? 1 : 0);
  text_buf.push_int(is_all_killed ? 1 : 0);
  text_buf.push_string(return_type);
  text_buf.push_raw(return_value_len, return_value);
  send_message(mtc->tc_fd, text_buf);
}

void MainController::send_execute_control(const char *module_name)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_EXECUTE_CONTROL);
  text_buf.push_string(module_name);
  send_message(mtc->tc_fd, text_buf);
}

void MainController::send_execute_testcase(const char *module_name,
  const char *testcase_name)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_EXECUTE_TESTCASE);
  text_buf.push_string(module_name);
  text_buf.push_string(testcase_name);
  send_message(mtc->tc_fd, text_buf);
}

void MainController::send_ptc_verdict(boolean continue_execution)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_PTC_VERDICT);
  int n_ptcs = 0;
  for (int i = tc_first_comp_ref; i < n_components; i++)
    if (components[i]->tc_state != PTC_STALE) n_ptcs++;
  text_buf.push_int(n_ptcs);
  for (int i = tc_first_comp_ref; i < n_components; i++) {
    if (components[i]->tc_state != PTC_STALE) {
      text_buf.push_int(components[i]->comp_ref);
      text_buf.push_string(components[i]->comp_name);
      text_buf.push_int(components[i]->local_verdict);
      if (components[i]->verdict_reason != NULL)
        text_buf.push_string(components[i]->verdict_reason);
      else
        text_buf.push_string("");
    }
  }
  text_buf.push_int(continue_execution ? 1 : 0);
  send_message(mtc->tc_fd, text_buf);
}

void MainController::send_continue()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONTINUE);
  send_message(mtc->tc_fd, text_buf);
}

void MainController::send_exit_mtc()
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_EXIT_MTC);
  send_message(mtc->tc_fd, text_buf);
}

void MainController::send_configure_mtc(const char* config_file)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CONFIGURE);
  text_buf.push_string(config_file);
  send_message(mtc->tc_fd, text_buf);
}


void MainController::send_cancel_done_ptc(component_struct *tc,
  component component_reference)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_CANCEL_DONE);
  text_buf.push_int(component_reference);
  send_message(tc->tc_fd, text_buf);

}

void MainController::send_component_status_ptc(component_struct *tc,
  component component_reference, boolean is_done, boolean is_killed,
  const char *return_type, int return_value_len, const void *return_value)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_COMPONENT_STATUS);
  text_buf.push_int(component_reference);
  text_buf.push_int(is_done ? 1 : 0);
  text_buf.push_int(is_killed ? 1 : 0);
  text_buf.push_string(return_type);
  text_buf.push_raw(return_value_len, return_value);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_start(component_struct *tc,
  const qualified_name& function_name, int arg_len, const void *arg_ptr)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_START);
  text_buf.push_qualified_name(function_name);
  text_buf.push_raw(arg_len, arg_ptr);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_kill(component_struct *tc)
{
  Text_Buf text_buf;
  text_buf.push_int(MSG_KILL);
  send_message(tc->tc_fd, text_buf);
}

void MainController::send_error(int fd, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *reason = mprintf_va_list(fmt, ap);
  va_end(ap);
  send_error_str(fd, reason);
  Free(reason);
}

void MainController::send_error_str(int fd, const char *reason)
{
  Text_Buf text_buf;
  text_buf.push_int(static_cast<RInt>( MSG_ERROR ));
  text_buf.push_string(reason);
  send_message(fd, text_buf);
}

void MainController::send_message(int fd, Text_Buf& text_buf)
{
  text_buf.calculate_length();
  const char *send_ptr = text_buf.get_data();
  int send_len = text_buf.get_len();
  int sent_len = send(fd, send_ptr, send_len, 0);
  if (send_len != sent_len) {
    error("Sending of message failed: %s", strerror(errno));
  }
}

void MainController::process_error(unknown_connection *conn)
{
  Text_Buf& text_buf = *conn->text_buf;
  char *reason = text_buf.pull_string();
  error("Error message was received on an unknown connection from %s [%s]: "
    "%s.", conn->ip_addr->get_host_str(), conn->ip_addr->get_addr_str(), reason);
  delete [] reason;
  text_buf.cut_message();
  status_change();
}

void MainController::process_log(unknown_connection *conn)
{
  Text_Buf& text_buf = *conn->text_buf;
  struct timeval tv;
  tv.tv_sec = text_buf.pull_int().get_val();
  tv.tv_usec = text_buf.pull_int().get_val();
  char *source = mprintf("<unknown>@%s", conn->ip_addr->get_host_str());
  int severity = text_buf.pull_int().get_val();
  char *message = text_buf.pull_string();
  notify(&tv, source, severity, message);
  Free(source);
  delete [] message;
}

void MainController::process_version(unknown_connection *conn)
{
  if (check_version(conn)) {
    error("HC connection from %s [%s] was refused because of "
      "incorrect version.", conn->ip_addr->get_host_str(),
      conn->ip_addr->get_addr_str());
    close_unknown_connection(conn);
    return;
  }
  host_struct *hc = add_new_host(conn);
  switch (mc_state) {
  case MC_LISTENING:
    mc_state = MC_HC_CONNECTED;
  case MC_HC_CONNECTED:
    break;
  case MC_LISTENING_CONFIGURED:
  case MC_ACTIVE:
    configure_host(hc, TRUE);
    mc_state = MC_CONFIGURING;
    break;
  case MC_SHUTDOWN:
    send_exit_hc(hc);
    hc->hc_state = HC_EXITING;
    break;
  default:
    configure_host(hc, TRUE);
  }
  // handle the remaining messages that are in hc->text_buf
  handle_hc_data(hc, FALSE);
  status_change();
}

void MainController::process_mtc_created(unknown_connection *conn)
{
  int fd = conn->fd;
  if (mc_state != MC_CREATING_MTC) {
    send_error_str(fd, "Message MTC_CREATED arrived in invalid state.");
    close_unknown_connection(conn);
    return;
  }
  if (mtc == NULL || mtc->tc_state != TC_INITIAL)
    fatal_error("MainController::process_mtc_created: MTC is in invalid "
      "state.");
  if (!conn->unix_socket &&
    *(mtc->comp_location->ip_addr) != *(conn->ip_addr)) {
    send_error(fd, "Message MTC_CREATED arrived from an unexpected "
      "IP address. It is accepted only from %s.",
      mtc->comp_location->ip_addr->get_addr_str());
    close_unknown_connection(conn);
    return;
  }

  mc_state = MC_READY;
  mtc->tc_state = TC_IDLE;
  mtc->tc_fd = fd;
  fd_table[fd].fd_type = FD_TC;
  fd_table[fd].component_ptr = mtc;
  Text_Buf *text_buf = conn->text_buf;
  text_buf->cut_message();
  mtc->text_buf = text_buf;
  delete [] mtc->initial.location_str;

  delete_unknown_connection(conn);

  notify("MTC is created.");
  // handle the remaining messages that are in text_buf
  handle_tc_data(mtc, FALSE);
  status_change();
}

void MainController::process_ptc_created(unknown_connection *conn)
{
  int fd = conn->fd;

  switch (mc_state) {
  case MC_EXECUTING_TESTCASE:
  case MC_TERMINATING_TESTCASE:
    break;
  default:
    send_error_str(fd, "Message PTC_CREATED arrived in invalid state.");
    close_unknown_connection(conn);
    return;
  }

  Text_Buf *text_buf = conn->text_buf;
  component component_reference = text_buf->pull_int().get_val();

  switch (component_reference) {
  case NULL_COMPREF:
    send_error_str(fd, "Message PTC_CREATED refers to the null component "
      "reference.");
    close_unknown_connection(conn);
    return;
  case MTC_COMPREF:
    send_error_str(fd, "Message PTC_CREATED refers to the component "
      "reference of the MTC.");
    close_unknown_connection(conn);
    return;
  case SYSTEM_COMPREF:
    send_error_str(fd, "Message PTC_CREATED refers to the component "
      "reference of the system.");
    close_unknown_connection(conn);
    return;
  case ANY_COMPREF:
    send_error_str(fd, "Message PTC_CREATED refers to 'any component'.");
    close_unknown_connection(conn);
    return;
  case ALL_COMPREF:
    send_error_str(fd, "Message PTC_CREATED refers to 'all component'.");
    close_unknown_connection(conn);
    return;
  }

  component_struct *tc = lookup_component(component_reference);
  if (tc == NULL) {
    send_error(fd, "Message PTC_CREATED refers to invalid component "
      "reference %d.", component_reference);
    close_unknown_connection(conn);
    return;
  } else if (tc->tc_state != TC_INITIAL) {
    send_error(fd, "Message PTC_CREATED refers to test component "
      "%d, which is not being created.", component_reference);
    close_unknown_connection(conn);
    return;
  } else if (!conn->unix_socket && *(conn->ip_addr) != *(tc->comp_location->ip_addr)) {
    char *real_hostname = mprintf("%s [%s]", conn->ip_addr->get_host_str(),
      conn->ip_addr->get_addr_str());
    char *expected_hostname = mprintf("%s [%s]",
      tc->comp_location->hostname, tc->comp_location->ip_addr->get_addr_str());
    send_error(fd, "Invalid source host (%s) for the control "
      "connection. Expected: %s.", real_hostname, expected_hostname);
    error("Connection of PTC %d arrived from an unexpected "
      "IP address (%s). Expected: %s.", component_reference,
      real_hostname, expected_hostname);
    Free(real_hostname);
    Free(expected_hostname);
    close_unknown_connection(conn);
    return;
  }

  tc->tc_state = TC_IDLE;
  tc->tc_fd = fd;
  fd_table[fd].fd_type = FD_TC;
  fd_table[fd].component_ptr = tc;
  text_buf->cut_message();
  tc->text_buf = text_buf;
  delete [] tc->initial.location_str;

  delete_unknown_connection(conn);

  if (mc_state == MC_TERMINATING_TESTCASE || mtc->stop_requested ||
    mtc->tc_state == MTC_ALL_COMPONENT_KILL ||
    (mtc->tc_state == MTC_ALL_COMPONENT_STOP && !tc->is_alive)) {
    send_kill(tc);
    tc->tc_state = PTC_KILLING;
    if (!tc->is_alive) tc->stop_requested = TRUE;
    init_requestors(&tc->stopping_killing.stop_requestors, NULL);
    init_requestors(&tc->stopping_killing.kill_requestors, NULL);
    start_kill_timer(tc);
  } else {
    component_struct *create_requestor = tc->initial.create_requestor;
    if (create_requestor->tc_state == TC_CREATE) {
      send_create_ack(create_requestor, component_reference);
      if (create_requestor == mtc)
        create_requestor->tc_state = MTC_TESTCASE;
      else create_requestor->tc_state = PTC_FUNCTION;
    }
  }
  // handle the remaining messages that are in text_buf
  handle_tc_data(tc, FALSE);
  status_change();
}

void MainController::process_error(host_struct *hc)
{
  char *reason = hc->text_buf->pull_string();
  error("Error message was received from HC at %s [%s]: %s",
    hc->hostname, hc->ip_addr->get_addr_str(), reason);
  delete [] reason;
}

void MainController::process_log(host_struct *hc)
{
  Text_Buf& text_buf = *hc->text_buf;
  struct timeval tv;
  tv.tv_sec = text_buf.pull_int().get_val();
  tv.tv_usec = text_buf.pull_int().get_val();
  int severity = text_buf.pull_int().get_val();
  char *message = text_buf.pull_string();
  notify(&tv, hc->log_source, severity, message);
  delete [] message;
}

void MainController::process_configure_ack(host_struct *hc)
{
  switch (hc->hc_state) {
  case HC_CONFIGURING:
    hc->hc_state = HC_ACTIVE;
    break;
  case HC_CONFIGURING_OVERLOADED:
    hc->hc_state = HC_OVERLOADED;
    break;
  default:
    send_error_str(hc->hc_fd, "Unexpected message CONFIGURE_ACK was "
      "received.");
    return;
  }
  if (mc_state == MC_CONFIGURING || mc_state == MC_RECONFIGURING)
    check_all_hc_configured();
  else notify("Host %s was configured successfully.", hc->hostname);
  status_change();
}

void MainController::process_configure_nak(host_struct *hc)
{
  switch (hc->hc_state) {
  case HC_CONFIGURING:
  case HC_CONFIGURING_OVERLOADED:
    hc->hc_state = HC_IDLE;
    break;
  default:
    send_error_str(hc->hc_fd, "Unexpected message CONFIGURE_NAK was "
      "received.");
    return;
  }
  if (mc_state == MC_CONFIGURING || mc_state == MC_RECONFIGURING)
    check_all_hc_configured();
  else notify("Processing of configuration file failed on host %s.",
    hc->hostname);
  status_change();
}

void MainController::process_create_nak(host_struct *hc)
{
  switch (mc_state) {
  case MC_CREATING_MTC:
  case MC_EXECUTING_TESTCASE:
  case MC_TERMINATING_TESTCASE:
    break;
  default:
    send_error_str(hc->hc_fd, "Message CREATE_NAK arrived in invalid "
      "state.");
    return;
  }

  switch (hc->hc_state) {
  case HC_ACTIVE:
    notify("Host %s is overloaded. New components will not be created "
      "there until further notice.", hc->hostname);
    hc->hc_state = HC_OVERLOADED;
    // no break
  case HC_OVERLOADED:
    break;
  default:
    send_error_str(hc->hc_fd, "Unexpected message CREATE_NAK was received: "
      "the sender is in invalid state.");
    return;
  }

  Text_Buf& text_buf = *hc->text_buf;
  component component_reference = text_buf.pull_int().get_val();

  switch (component_reference) {
  case NULL_COMPREF:
    send_error_str(hc->hc_fd, "Message CREATE_NAK refers to the null "
      "component reference.");
    return;
  case SYSTEM_COMPREF:
    send_error_str(hc->hc_fd, "Message CREATE_NAK refers to the component "
      "reference of the system.");
    return;
  case ANY_COMPREF:
    send_error_str(hc->hc_fd, "Message CREATE_NAK refers to "
      "'any component'.");
    return;
  case ALL_COMPREF:
    send_error_str(hc->hc_fd, "Message CREATE_NAK refers to "
      "'all component'.");
    return;
  }

  component_struct *tc = lookup_component(component_reference);
  if (tc == NULL) {
    send_error(hc->hc_fd, "Message CREATE_NAK refers to invalid component "
      "reference %d.", component_reference);
    return;
  }
  if (tc->tc_state != TC_INITIAL) {
    send_error(hc->hc_fd, "Message CREATE_NAK refers to test component "
      "%d, which is not being created.", component_reference);
    return;
  }
  if (tc->comp_location != hc) {
    send_error(hc->hc_fd, "Message CREATE_NAK refers to test component "
      "%d, which was assigned to a different host (%s).",
      component_reference, tc->comp_location->hostname);
    return;
  }

  remove_component_from_host(tc);
  hc->n_active_components--;

  char *reason = text_buf.pull_string();

  if (tc == mtc) {
    if (mc_state != MC_CREATING_MTC)
      fatal_error("MainController::process_create_nak: MC is in "
        "unexpected state when CREATE_NAK refers to MTC.");
    error("Creation of MTC failed on host %s: %s.", hc->hostname, reason);
    destroy_all_components();
    mc_state = MC_ACTIVE;
  } else {
    host_struct *new_host = choose_ptc_location(
      tc->comp_type.definition_name, tc->comp_name,
      tc->initial.location_str);
    if (new_host != NULL) {
      send_create_ptc(new_host, component_reference, tc->comp_type,
        tc->comp_name, tc->is_alive, mtc->tc_fn_name);
      notify("PTC with component reference %d was relocated from host "
        "%s to %s because of overload: %s.", component_reference,
        hc->hostname, new_host->hostname, reason);
      add_component_to_host(new_host, tc);
      new_host->n_active_components++;
    } else {
      char *comp_data = mprintf("component type: %s.%s",
        tc->comp_type.module_name, tc->comp_type.definition_name);
      if (tc->comp_name != NULL)
        comp_data = mputprintf(comp_data, ", name: %s", tc->comp_name);
      if (tc->initial.location_str != NULL &&
        tc->initial.location_str[0] != '\0')
        comp_data = mputprintf(comp_data, ", location: %s",
          tc->initial.location_str);
      component_struct *create_requestor = tc->initial.create_requestor;
      if (create_requestor->tc_state == TC_CREATE) {
        send_error(create_requestor->tc_fd, "Creation of the new PTC "
          "(%s) failed on host %s: %s. Other suitable hosts to "
          "relocate the component are not available.", comp_data,
          hc->hostname, reason);
        if (create_requestor == mtc)
          create_requestor->tc_state = MTC_TESTCASE;
        else create_requestor->tc_state = PTC_FUNCTION;
      }
      delete [] tc->initial.location_str;
      tc->tc_state = PTC_STALE;
      n_active_ptcs--;
      switch (mtc->tc_state) {
      case MTC_TERMINATING_TESTCASE:
        if (ready_to_finish_testcase()) finish_testcase();
        break;
      case MTC_ALL_COMPONENT_KILL:
        check_all_component_kill();
        break;
      case MTC_ALL_COMPONENT_STOP:
        check_all_component_stop();
        break;
      default:
        break;
      }
      notify("Creation of a PTC (%s) failed on host %s: %s. "
        "Relocation to other suitable host is not possible.",
        comp_data, hc->hostname, reason);
      Free(comp_data);
    }
  }

  delete [] reason;

  status_change();
}

void MainController::process_hc_ready(host_struct *hc)
{
  switch(hc->hc_state) {
  case HC_OVERLOADED:
    hc->hc_state = HC_ACTIVE;
    break;
  case HC_CONFIGURING_OVERLOADED:
    hc->hc_state = HC_CONFIGURING;
    break;
  default:
    send_error_str(hc->hc_fd, "Unexpected message HC_READY was received.");
    return;
  }
  notify("Host %s is no more overloaded.", hc->hostname);
  status_change();
}

void MainController::process_error(component_struct *tc)
{
  char *reason = tc->text_buf->pull_string();
  if (tc == mtc) {
    error("Error message was received from the MTC at %s [%s]: %s",
      mtc->comp_location->hostname,
      mtc->comp_location->ip_addr->get_addr_str(), reason);
  } else {
    notify("Error message was received from PTC %d at %s [%s]: %s",
      tc->comp_ref, tc->comp_location->hostname,
      tc->comp_location->ip_addr->get_addr_str(), reason);
  }
  delete [] reason;
}

void MainController::process_log(component_struct *tc)
{
  Text_Buf& text_buf = *tc->text_buf;
  struct timeval tv;
  tv.tv_sec = text_buf.pull_int().get_val();
  tv.tv_usec = text_buf.pull_int().get_val();
  int severity = text_buf.pull_int().get_val();
  char *message = text_buf.pull_string();
  notify(&tv, tc->log_source, severity, message);
  delete [] message;
}

void MainController::process_create_req(component_struct *tc)
{
  if (!request_allowed(tc, "CREATE_REQ")) return;

  if (max_ptcs >= 0 && n_active_ptcs >= max_ptcs) {
    send_error(tc->tc_fd, "The license key does not allow more than %d "
      "simultaneously active PTCs.", max_ptcs);
    return;
  }

  Text_Buf& text_buf = *tc->text_buf;
  qualified_name component_type;
  text_buf.pull_qualified_name(component_type);
  char *component_name = text_buf.pull_string();
  if (component_name[0] == '\0') {
    delete [] component_name;
    component_name = NULL;
  }
  char *component_location = text_buf.pull_string();
  if (component_location[0] == '\0') {
    delete [] component_location;
    component_location = NULL;
  }
  boolean is_alive = text_buf.pull_int().get_val();

  host_struct *host = choose_ptc_location(component_type.definition_name,
    component_name, component_location);

  if (host == NULL) {
    if (!is_hc_in_state(HC_ACTIVE)) {
      send_error_str(tc->tc_fd, "There is no active HC connection. "
        "Create operation cannot be performed.");
    } else {
      char *comp_data = mprintf("component type: %s.%s",
        component_type.module_name, component_type.definition_name);
      if (component_name != NULL)
        comp_data = mputprintf(comp_data, ", name: %s", component_name);
      if (component_location != NULL)
        comp_data = mputprintf(comp_data, ", location: %s",
          component_location);
      send_error(tc->tc_fd, "No suitable host was found to create a "
        "new PTC (%s).", comp_data);
      Free(comp_data);
    }
    free_qualified_name(&component_type);
    delete [] component_name;
    delete [] component_location;
    return;
  }

  component comp_ref = next_comp_ref++;
  send_create_ptc(host, comp_ref, component_type, component_name, is_alive,
    mtc->tc_fn_name);

  tc->tc_state = TC_CREATE;

  component_struct *new_ptc = new component_struct;
  new_ptc->comp_ref = comp_ref;
  new_ptc->comp_type = component_type;
  new_ptc->comp_name = component_name;
  new_ptc->tc_state = TC_INITIAL;
  new_ptc->local_verdict = NONE;
  new_ptc->verdict_reason = NULL;
  new_ptc->tc_fd = -1;
  new_ptc->text_buf = NULL;
  init_qualified_name(&new_ptc->tc_fn_name);
  new_ptc->return_type = NULL;
  new_ptc->return_value_len = 0;
  new_ptc->return_value = NULL;
  new_ptc->is_alive = is_alive;
  new_ptc->stop_requested = FALSE;
  new_ptc->process_killed = FALSE;
  new_ptc->initial.create_requestor = tc;
  new_ptc->initial.location_str = component_location;
  init_requestors(&new_ptc->done_requestors, NULL);
  init_requestors(&new_ptc->killed_requestors, NULL);
  init_requestors(&new_ptc->cancel_done_sent_for, NULL);
  new_ptc->kill_timer = NULL;
  init_connections(new_ptc);

  add_component(new_ptc);
  add_component_to_host(host, new_ptc);
  host->n_active_components++;
  n_active_ptcs++;

  status_change();
}

void MainController::process_start_req(component_struct *tc, int message_end)
{
  if (!request_allowed(tc, "START_REQ")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component component_reference = text_buf.pull_int().get_val();
  switch (component_reference) {
  case NULL_COMPREF:
    send_error_str(tc->tc_fd, "Start operation was requested on the null "
      "component reference.");
    return;
  case MTC_COMPREF:
    send_error_str(tc->tc_fd, "Start operation was requested on the "
      "component reference of the MTC.");
    return;
  case SYSTEM_COMPREF:
    send_error_str(tc->tc_fd, "Start operation was requested on the "
      "component reference of the system.");
    return;
  case ANY_COMPREF:
    send_error_str(tc->tc_fd, "Start operation was requested on "
      "'any component'.");
    return;
  case ALL_COMPREF:
    send_error_str(tc->tc_fd, "Start operation was requested on "
      "'all component'.");
    return;
  }
  component_struct *target = lookup_component(component_reference);
  if (target == NULL) {
    send_error(tc->tc_fd, "Start operation was requested on invalid "
      "component reference: %d.", component_reference);
    return;
  }
  switch (target->tc_state) {
  case TC_IDLE:
  case PTC_STOPPED:
    // these states are correct
    break;
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case PTC_FUNCTION:
  case PTC_STARTING:
    send_error(tc->tc_fd, "PTC with component reference %d cannot be "
      "started because it is already executing function %s.%s.",
      component_reference, target->tc_fn_name.module_name,
      target->tc_fn_name.definition_name);
    return;
  case TC_STOPPING:
    send_error(tc->tc_fd, "PTC with component reference %d cannot be "
      "started because it function %s.%s is currently being stopped on "
      "it.", component_reference, target->tc_fn_name.module_name,
      target->tc_fn_name.definition_name);
    return;
  case PTC_KILLING:
  case PTC_STOPPING_KILLING:
    send_error(tc->tc_fd, "PTC with component reference %d cannot be "
      "started because it is currently being killed.",
      component_reference);
    return;
  case TC_EXITING:
  case TC_EXITED:
    send_error(tc->tc_fd, "PTC with component reference %d cannot be "
      "started because it is not alive anymore.", component_reference);
    return;
  case PTC_STALE:
    send_error(tc->tc_fd, "The argument of start operation (%d) is a "
      "component reference that belongs to an earlier testcase.",
      component_reference);
    return;
  default:
    send_error(tc->tc_fd, "Start operation was requested on component "
      "reference %d, which is in invalid state.",
      component_reference);
    return;
  }
  text_buf.pull_qualified_name(target->tc_fn_name);
  target->stop_requested = FALSE;
  int arg_begin = text_buf.get_pos();
  int arg_len = message_end - arg_begin;
  const void *arg_ptr = text_buf.get_data() + arg_begin;
  boolean send_cancel_done = FALSE, cancel_any_component_done = FALSE;
  if (target->tc_state == PTC_STOPPED) {
    // updating the state of target because 'any component.done' cannot
    // consider this component anymore
    target->tc_state = PTC_STARTING;
    // cleaning up the previous return value
    delete [] target->return_type;
    target->return_type = NULL;
    target->return_value_len = 0;
    Free(target->return_value);
    target->return_value = NULL;
    // determining which components we need to send CANCEL_DONE to
    init_requestors(&target->starting.cancel_done_sent_to, NULL);
    for (int i = 0; ; i++) {
      component_struct *comp = get_requestor(&target->done_requestors, i);
      if (comp == NULL) break;
      else if (comp == tc) {
        // the start requestor shall cancel the done status locally
        // ignore it
        continue;
      }
      switch (comp->tc_state) {
      case TC_CREATE:
      case TC_START:
      case TC_STOP:
      case TC_KILL:
      case TC_CONNECT:
      case TC_DISCONNECT:
      case TC_MAP:
      case TC_UNMAP:
      case TC_STOPPING:
      case MTC_TESTCASE:
      case PTC_FUNCTION:
      case PTC_STARTING:
      case PTC_STOPPED:
        // a CANCEL_DONE message shall be sent to comp
        send_cancel_done = TRUE;
        add_requestor(&target->starting.cancel_done_sent_to, comp);
        break;
      case TC_EXITING:
      case TC_EXITED:
      case PTC_KILLING:
      case PTC_STOPPING_KILLING:
        // CANCEL_DONE will not be sent to comp
        break;
      default:
        error("Test Component %d is in invalid state when starting "
          "PTC %d.", comp->comp_ref, component_reference);
      }
    }
    // check whether 'any component.done' needs to be cancelled
    if (any_component_done_sent && !is_any_component_done()) {
      send_cancel_done = TRUE;
      cancel_any_component_done = TRUE;
      any_component_done_sent = FALSE;
      add_requestor(&target->starting.cancel_done_sent_to, mtc);
    }
    free_requestors(&target->done_requestors);
  }
  if (send_cancel_done) {
    for (int i = 0; ; i++) {
      component_struct *comp =
        get_requestor(&target->starting.cancel_done_sent_to, i);
      if (comp == NULL) break;
      else if (comp == mtc) send_cancel_done_mtc(component_reference,
        cancel_any_component_done);
      else send_cancel_done_ptc(comp, component_reference);
      add_requestor(&comp->cancel_done_sent_for, target);
    }
    target->starting.start_requestor = tc;
    target->starting.arguments_len = arg_len;
    target->starting.arguments_ptr = Malloc(arg_len);
    memcpy(target->starting.arguments_ptr, arg_ptr, arg_len);
    tc->tc_state = TC_START;
  } else {
    send_start(target, target->tc_fn_name, arg_len, arg_ptr);
    send_start_ack(tc);
    target->tc_state = PTC_FUNCTION;
  }
  status_change();
}

void MainController::process_stop_req(component_struct *tc)
{
  if (!request_allowed(tc, "STOP_REQ")) return;

  component component_reference = tc->text_buf->pull_int().get_val();
  switch (component_reference) {
  case NULL_COMPREF:
    send_error_str(tc->tc_fd, "Stop operation was requested on the null "
      "component reference.");
    return;
  case MTC_COMPREF:
    // 'mtc.stop' initiated by a PTC terminates the current testcase
    if (tc != mtc) {
      if (!mtc->stop_requested) {
        send_stop(mtc);
        kill_all_components(TRUE);
        mtc->stop_requested = TRUE;
        start_kill_timer(mtc);
        notify("Test Component %d has requested to stop MTC. "
          "Terminating current testcase execution.", tc->comp_ref);
        status_change();
      }
    } else send_error_str(tc->tc_fd, "MTC has requested to stop itself.");
    return;
  case SYSTEM_COMPREF:
    send_error_str(tc->tc_fd, "Stop operation was requested on the "
      "component reference of the system.");
    return;
  case ANY_COMPREF:
    send_error_str(tc->tc_fd, "Stop operation was requested on "
      "'any component'.");
    return;
  case ALL_COMPREF:
    if (tc == mtc) {
      if (stop_all_components()) send_stop_ack(mtc);
      else {
        mtc->tc_state = MTC_ALL_COMPONENT_STOP;
        status_change();
      }
    } else send_error_str(tc->tc_fd, "Operation 'all component.stop' can "
      "only be performed on the MTC.");
    return;
  default:
    break;
  }
  // the operation refers to a specific PTC
  component_struct *target = lookup_component(component_reference);
  if (target == NULL) {
    send_error(tc->tc_fd, "The argument of stop operation is an "
      "invalid component reference: %d.", component_reference);
    return;
  } else if (target == tc) {
    send_error_str(tc->tc_fd, "Stop operation was requested on the "
      "requestor component itself.");
    return;
  }
  boolean target_inactive = FALSE;
  switch (target->tc_state) {
  case PTC_STOPPED:
    if (!target->is_alive) error("PTC %d cannot be in state STOPPED "
      "because it is not an alive type PTC.", component_reference);
    // no break
  case TC_IDLE:
    target_inactive = TRUE;
    // no break
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case PTC_FUNCTION:
    if (target->is_alive) {
      if (target_inactive) {
        // do nothing, just send a STOP_ACK to tc
        send_stop_ack(tc);
        break;
      } else {
        send_stop(target);
        target->tc_state = TC_STOPPING;
      }
    } else {
      // the target is not an alive type PTC: stop operation means kill
      send_kill(target);
      if (target_inactive) target->tc_state = PTC_KILLING;
      else target->tc_state = PTC_STOPPING_KILLING;
    }
    // a STOP or KILL message was sent out
    target->stop_requested = TRUE;
    init_requestors(&target->stopping_killing.stop_requestors, tc);
    init_requestors(&target->stopping_killing.kill_requestors, NULL);
    start_kill_timer(target);
    tc->tc_state = TC_STOP;
    status_change();
    break;
  case PTC_KILLING:
    if (target->is_alive) {
      // do nothing if the PTC is alive
      send_stop_ack(tc);
      break;
    }
    // no break
  case TC_STOPPING:
  case PTC_STOPPING_KILLING:
    // the PTC is currently being stopped
    add_requestor(&target->stopping_killing.stop_requestors, tc);
    tc->tc_state = TC_STOP;
    status_change();
    break;
  case TC_EXITING:
  case TC_EXITED:
    // the PTC is already terminated, do nothing
    send_stop_ack(tc);
    break;
  case PTC_STARTING:
    send_error(tc->tc_fd, "PTC with component reference %d cannot be "
      "stopped because it is currently being started.",
      component_reference);
    break;
  case PTC_STALE:
    send_error(tc->tc_fd, "The argument of stop operation (%d) is a "
      "component reference that belongs to an earlier testcase.",
      component_reference);
    break;
  default:
    send_error(tc->tc_fd, "The test component that the stop operation "
      "refers to (%d) is in invalid state.", component_reference);
  }
}

void MainController::process_kill_req(component_struct *tc)
{
  if (!request_allowed(tc, "KILL_REQ")) return;

  component component_reference = tc->text_buf->pull_int().get_val();
  switch (component_reference) {
  case NULL_COMPREF:
    send_error_str(tc->tc_fd, "Kill operation was requested on the null "
      "component reference.");
    return;
  case MTC_COMPREF:
    send_error_str(tc->tc_fd, "Kill operation was requested on the "
      "component reference of the MTC.");
    return;
  case SYSTEM_COMPREF:
    send_error_str(tc->tc_fd, "Kill operation was requested on the "
      "component reference of the system.");
    return;
  case ANY_COMPREF:
    send_error_str(tc->tc_fd, "Kill operation was requested on "
      "'any component'.");
    return;
  case ALL_COMPREF:
    if (tc == mtc) {
      if (kill_all_components(FALSE)) send_kill_ack(mtc);
      else {
        mtc->tc_state = MTC_ALL_COMPONENT_KILL;
        status_change();
      }
    } else send_error_str(tc->tc_fd, "Operation 'all component.kill' can "
      "only be performed on the MTC.");
    return;
  default:
    break;
  }
  // the operation refers to a specific PTC
  component_struct *target = lookup_component(component_reference);
  if (target == NULL) {
    send_error(tc->tc_fd, "The argument of kill operation is an "
      "invalid component reference: %d.", component_reference);
    return;
  } else if (target == tc) {
    send_error_str(tc->tc_fd, "Kill operation was requested on the "
      "requestor component itself.");
    return;
  }
  boolean target_inactive = FALSE;
  switch (target->tc_state) {
  case PTC_STOPPED:
    // the done status of this PTC is already sent out
    // and it will not be cancelled in the future
    free_requestors(&target->done_requestors);
    // no break
  case TC_IDLE:
    target_inactive = TRUE;
    // no break
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case PTC_FUNCTION:
    send_kill(target);
    if (target_inactive) {
      // the PTC was inactive
      target->tc_state = PTC_KILLING;
      if (!target->is_alive) target->stop_requested = TRUE;
    } else {
      // the PTC was active
      target->tc_state = PTC_STOPPING_KILLING;
      target->stop_requested = TRUE;
    }
    init_requestors(&target->stopping_killing.stop_requestors, NULL);
    init_requestors(&target->stopping_killing.kill_requestors, tc);
    start_kill_timer(target);
    tc->tc_state = TC_KILL;
    status_change();
    break;
  case TC_STOPPING:
    // the PTC is currently being stopped
    send_kill(target);
    target->tc_state = PTC_STOPPING_KILLING;
    if (target->kill_timer != NULL) cancel_timer(target->kill_timer);
    start_kill_timer(target);
    // no break
  case PTC_KILLING:
  case PTC_STOPPING_KILLING:
    // the PTC is currently being terminated
    add_requestor(&target->stopping_killing.kill_requestors, tc);
    tc->tc_state = TC_KILL;
    status_change();
    break;
  case TC_EXITING:
  case TC_EXITED:
    // the PTC is already terminated
    send_kill_ack(tc);
    break;
  case PTC_STARTING:
    send_error(tc->tc_fd, "PTC with component reference %d cannot be "
      "killed because it is currently being started.",
      component_reference);
    break;
  case PTC_STALE:
    send_error(tc->tc_fd, "The argument of kill operation (%d) is a "
      "component reference that belongs to an earlier testcase.",
      component_reference);
    break;
  default:
    send_error(tc->tc_fd, "The test component that the kill operation "
      "refers to (%d) is in invalid state.", component_reference);
  }
}

void MainController::process_is_running(component_struct *tc)
{
  if (!request_allowed(tc, "IS_RUNNING")) return;

  component component_reference = tc->text_buf->pull_int().get_val();
  switch (component_reference) {
  case NULL_COMPREF:
    send_error_str(tc->tc_fd, "Running operation was requested on the "
      "null component reference.");
    return;
  case MTC_COMPREF:
    send_error_str(tc->tc_fd, "Running operation was requested on the "
      "component reference of the MTC.");
    return;
  case SYSTEM_COMPREF:
    send_error_str(tc->tc_fd, "Running operation was requested on the "
      "component reference of the system.");
    return;
  case ANY_COMPREF:
    if (tc == mtc) send_running(mtc, is_any_component_running());
    else send_error_str(tc->tc_fd, "Operation 'any component.running' "
      "can only be performed on the MTC.");
    return;
  case ALL_COMPREF:
    if (tc == mtc) send_running(mtc, is_all_component_running());
    else send_error_str(tc->tc_fd, "Operation 'all component.running' "
      "can only be performed on the MTC.");
    return;
  default:
    break;
  }
  // the operation refers to a specific PTC
  component_struct *comp = lookup_component(component_reference);
  if (comp == NULL) {
    send_error(tc->tc_fd, "The argument of running operation is an "
      "invalid component reference: %d.", component_reference);
    return;
  }
  switch (comp->tc_state) {
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case PTC_FUNCTION:
  case PTC_STARTING:
  case PTC_STOPPING_KILLING:
    send_running(tc, TRUE);
    break;
  case TC_IDLE:
  case TC_EXITING:
  case TC_EXITED:
  case PTC_STOPPED:
  case PTC_KILLING:
    send_running(tc, FALSE);
    break;
  case PTC_STALE:
    send_error(tc->tc_fd, "The argument of running operation (%d) is a "
      "component reference that belongs to an earlier testcase.",
      component_reference);
    break;
  default:
    send_error(tc->tc_fd, "The test component that the running operation "
      "refers to (%d) is in invalid state.", component_reference);
  }
}

void MainController::process_is_alive(component_struct *tc)
{
  if (!request_allowed(tc, "IS_ALIVE")) return;

  component component_reference = tc->text_buf->pull_int().get_val();
  switch (component_reference) {
  case NULL_COMPREF:
    send_error_str(tc->tc_fd, "Alive operation was requested on the "
      "null component reference.");
    return;
  case MTC_COMPREF:
    send_error_str(tc->tc_fd, "Alive operation was requested on the "
      "component reference of the MTC.");
    return;
  case SYSTEM_COMPREF:
    send_error_str(tc->tc_fd, "Alive operation was requested on the "
      "component reference of the system.");
    return;
  case ANY_COMPREF:
    if (tc == mtc) send_alive(mtc, is_any_component_alive());
    else send_error_str(tc->tc_fd, "Operation 'any component.alive' "
      "can only be performed on the MTC.");
    return;
  case ALL_COMPREF:
    if (tc == mtc) send_alive(mtc, is_all_component_alive());
    else send_error_str(tc->tc_fd, "Operation 'all component.alive' "
      "can only be performed on the MTC.");
    return;
  default:
    break;
  }
  // the operation refers to a specific PTC
  component_struct *comp = lookup_component(component_reference);
  if (comp == NULL) {
    send_error(tc->tc_fd, "The argument of alive operation is an "
      "invalid component reference: %d.", component_reference);
    return;
  }
  switch (comp->tc_state) {
  case TC_IDLE:
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case PTC_FUNCTION:
  case PTC_STARTING:
  case PTC_STOPPED:
  case PTC_KILLING:
  case PTC_STOPPING_KILLING:
    send_alive(tc, TRUE);
    break;
  case TC_EXITING:
  case TC_EXITED:
    send_alive(tc, FALSE);
    break;
  case PTC_STALE:
    send_error(tc->tc_fd, "The argument of alive operation (%d) is a "
      "component reference that belongs to an earlier testcase.",
      component_reference);
    break;
  default:
    send_error(tc->tc_fd, "The test component that the alive operation "
      "refers to (%d) is in invalid state.", component_reference);
  }
}

void MainController::process_done_req(component_struct *tc)
{
  if (!request_allowed(tc, "DONE_REQ")) return;

  component component_reference = tc->text_buf->pull_int().get_val();
  switch (component_reference) {
  case NULL_COMPREF:
    send_error_str(tc->tc_fd, "Done operation was requested on the null "
      "component reference.");
    return;
  case MTC_COMPREF:
    send_error_str(tc->tc_fd, "Done operation was requested on the "
      "component reference of the MTC.");
    return;
  case SYSTEM_COMPREF:
    send_error_str(tc->tc_fd, "Done operation was requested on the "
      "component reference of the system.");
    return;
  case ANY_COMPREF:
    if (tc == mtc) {
      boolean answer = is_any_component_done();
      send_done_ack(mtc, answer, NULL, 0, NULL);
      if (answer) any_component_done_sent = TRUE;
      else any_component_done_requested = TRUE;
    } else send_error_str(tc->tc_fd, "Operation 'any component.done' can "
      "only be performed on the MTC.");
    return;
  case ALL_COMPREF:
    if (tc == mtc) {
      boolean answer = !is_any_component_running();
      send_done_ack(mtc, answer, NULL, 0, NULL);
      if (!answer) all_component_done_requested = TRUE;
    } else send_error_str(tc->tc_fd, "Operation 'all component.done' can "
      "only be performed on the MTC.");
    return;
  default:
    break;
  }
  // the operation refers to a specific PTC
  component_struct *comp = lookup_component(component_reference);
  if (comp == NULL) {
    send_error(tc->tc_fd, "The argument of done operation is an "
      "invalid component reference: %d.", component_reference);
    return;
  }
  switch (comp->tc_state) {
  case PTC_STOPPED:
    // this answer has to be cancelled when the component is re-started
    add_requestor(&comp->done_requestors, tc);
    // no break
  case TC_EXITING:
  case TC_EXITED:
  case PTC_KILLING:
    send_done_ack(tc, TRUE, comp->return_type, comp->return_value_len,
      comp->return_value);
    break;
  case TC_IDLE:
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case PTC_FUNCTION:
  case PTC_STARTING:
  case PTC_STOPPING_KILLING:
    send_done_ack(tc, FALSE, NULL, 0, NULL);
    add_requestor(&comp->done_requestors, tc);
    break;
  case PTC_STALE:
    send_error(tc->tc_fd, "The argument of done operation (%d) is a "
      "component reference that belongs to an earlier testcase.",
      component_reference);
    break;
  default:
    send_error(tc->tc_fd, "The test component that the done operation "
      "refers to (%d) is in invalid state.", component_reference);
  }
}

void MainController::process_killed_req(component_struct *tc)
{
  if (!request_allowed(tc, "KILLED_REQ")) return;

  component component_reference = tc->text_buf->pull_int().get_val();
  switch (component_reference) {
  case NULL_COMPREF:
    send_error_str(tc->tc_fd, "Killed operation was requested on the null "
      "component reference.");
    return;
  case MTC_COMPREF:
    send_error_str(tc->tc_fd, "Killed operation was requested on the "
      "component reference of the MTC.");
    return;
  case SYSTEM_COMPREF:
    send_error_str(tc->tc_fd, "Killed operation was requested on the "
      "component reference of the system.");
    return;
  case ANY_COMPREF:
    if (tc == mtc) {
      boolean answer = !is_all_component_alive();
      send_killed_ack(mtc, answer);
      if (!answer) any_component_killed_requested = TRUE;
    } else send_error_str(tc->tc_fd, "Operation 'any component.killed' can "
      "only be performed on the MTC.");
    return;
  case ALL_COMPREF:
    if (tc == mtc) {
      boolean answer = !is_any_component_alive();
      send_killed_ack(mtc, answer);
      if (!answer) all_component_killed_requested = TRUE;
    } else send_error_str(tc->tc_fd, "Operation 'all component.killed' can "
      "only be performed on the MTC.");
    return;
  default:
    break;
  }
  // the operation refers to a specific PTC
  component_struct *comp = lookup_component(component_reference);
  if (comp == NULL) {
    send_error(tc->tc_fd, "The argument of killed operation is an "
      "invalid component reference: %d.", component_reference);
    return;
  }
  switch (comp->tc_state) {
  case TC_EXITING:
  case TC_EXITED:
    send_killed_ack(tc, TRUE);
    break;
  case TC_IDLE:
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case PTC_FUNCTION:
  case PTC_STARTING:
  case PTC_STOPPED:
  case PTC_KILLING:
  case PTC_STOPPING_KILLING:
    send_killed_ack(tc, FALSE);
    add_requestor(&comp->killed_requestors, tc);
    break;
  case PTC_STALE:
    send_error(tc->tc_fd, "The argument of killed operation (%d) is a "
      "component reference that belongs to an earlier testcase.",
      component_reference);
    break;
  default:
    send_error(tc->tc_fd, "The test component that the killed operation "
      "refers to (%d) is in invalid state.", component_reference);
  }
}

void MainController::process_cancel_done_ack(component_struct *tc)
{
  component component_reference = tc->text_buf->pull_int().get_val();
  switch (component_reference) {
  case NULL_COMPREF:
    send_error_str(tc->tc_fd, "Message CANCEL_DONE_ACK refers to the null "
      "component reference.");
    return;
  case MTC_COMPREF:
    send_error_str(tc->tc_fd, "Message CANCEL_DONE_ACK refers to the "
      "component reference of the MTC.");
    return;
  case SYSTEM_COMPREF:
    send_error_str(tc->tc_fd, "Message CANCEL_DONE_ACK refers to the "
      "component reference of the system.");
    return;
  case ANY_COMPREF:
    send_error_str(tc->tc_fd, "Message CANCEL_DONE_ACK refers to "
      "'any component'.");
    return;
  case ALL_COMPREF:
    send_error_str(tc->tc_fd, "Message CANCEL_DONE_ACK refers to "
      "'all component'.");
    return;
  default:
    break;
  }
  component_struct *started_tc = lookup_component(component_reference);
  if (started_tc == NULL) {
    send_error(tc->tc_fd, "Message CANCEL_DONE_ACK refers to an invalid "
      "component reference: %d.", component_reference);
    return;
  }
  done_cancelled(tc, started_tc);
  remove_requestor(&tc->cancel_done_sent_for, started_tc);
}

void MainController::process_connect_req(component_struct *tc)
{
  if (!request_allowed(tc, "CONNECT_REQ")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component src_compref = text_buf.pull_int().get_val();
  char *src_port = text_buf.pull_string();
  component dst_compref = text_buf.pull_int().get_val();
  char *dst_port = text_buf.pull_string();

  if (!valid_endpoint(src_compref, TRUE, tc, "connect") ||
    !valid_endpoint(dst_compref, TRUE, tc, "connect")) {
    delete [] src_port;
    delete [] dst_port;
    return;
  }

  port_connection *conn = find_connection(src_compref, src_port, dst_compref,
    dst_port);
  if (conn == NULL) {
    conn = new port_connection;
    conn->transport_type =
      choose_port_connection_transport(src_compref, dst_compref);
    conn->head.comp_ref = src_compref;
    conn->head.port_name = src_port;
    conn->tail.comp_ref = dst_compref;
    conn->tail.port_name = dst_port;
    init_requestors(&conn->requestors, tc);
    add_connection(conn);
    // conn->head and tail is now in canonical order
    switch (conn->transport_type) {
    case TRANSPORT_LOCAL:
      // send an empty string instead of component name
      // the component should already know its own name
      send_connect(components[conn->head.comp_ref], conn->head.port_name,
        conn->tail.comp_ref, NULL, conn->tail.port_name,
        conn->transport_type, 0, NULL);
      conn->conn_state = CONN_CONNECTING;
      break;
    case TRANSPORT_UNIX_STREAM:
    case TRANSPORT_INET_STREAM:
      // conn->head will be the server side
      if (conn->tail.comp_ref != MTC_COMPREF &&
        conn->tail.comp_ref != conn->head.comp_ref) {
        // send the name of conn->tail
        send_connect_listen(components[conn->head.comp_ref],
          conn->head.port_name, conn->tail.comp_ref,
          components[conn->tail.comp_ref]->comp_name,
          conn->tail.port_name, conn->transport_type);
      } else {
        // send an empty string instead of the name of conn->tail if
        // it is known by conn->head
        send_connect_listen(components[conn->head.comp_ref],
          conn->head.port_name, conn->tail.comp_ref, NULL,
          conn->tail.port_name, conn->transport_type);
      }
      conn->conn_state = CONN_LISTENING;
      break;
    default:
      send_error(tc->tc_fd, "The port connection %d:%s - %d:%s cannot "
        "be established because no suitable transport mechanism is "
        "available on the corresponding host(s).", src_compref,
        src_port, dst_compref, dst_port);
      remove_connection(conn);
      return;
    }
    tc->tc_state = TC_CONNECT;
    status_change();
  } else {
    switch (conn->conn_state) {
    case CONN_LISTENING:
    case CONN_CONNECTING:
      add_requestor(&conn->requestors, tc);
      tc->tc_state = TC_CONNECT;
      status_change();
      break;
    case CONN_CONNECTED:
      send_connect_ack(tc);
      break;
    case CONN_DISCONNECTING:
      send_error(tc->tc_fd, "The port connection %d:%s - %d:%s cannot "
        "be established because a disconnect operation is in progress "
        "on it.", src_compref, src_port, dst_compref, dst_port);
      break;
    default:
      send_error(tc->tc_fd, "The port connection %d:%s - %d:%s cannot "
        "be established due to an internal error in the MC.",
        src_compref, src_port, dst_compref, dst_port);
      error("The port connection %d:%s - %d:%s is in invalid state "
        "when a connect operation was requested on it.", src_compref,
        src_port, dst_compref, dst_port);
    }
    delete [] src_port;
    delete [] dst_port;
  }
}

void MainController::process_connect_listen_ack(component_struct *tc,
  int message_end)
{
  if (!message_expected(tc, "CONNECT_LISTEN_ACK")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component src_compref = tc->comp_ref;
  char *src_port = text_buf.pull_string();
  component dst_compref = text_buf.pull_int().get_val();
  char *dst_port = text_buf.pull_string();
  transport_type_enum transport_type =
    (transport_type_enum)text_buf.pull_int().get_val();
  int local_addr_begin = text_buf.get_pos();
  int local_addr_len = message_end - local_addr_begin;
  const void *local_addr_ptr = text_buf.get_data() + local_addr_begin;

  port_connection *conn = find_connection(src_compref, src_port, dst_compref,
    dst_port);
  if (conn != NULL) {
    // this message must arrive in the right state
    // and from the server side (head)
    if (conn->conn_state != CONN_LISTENING ||
      conn->head.comp_ref != src_compref ||
      strcmp(conn->head.port_name, src_port)) {
      send_error(tc->tc_fd, "Unexpected message CONNECT_LISTEN_ACK was "
        "received for port connection %d:%s - %d:%s.",
        src_compref, src_port, dst_compref, dst_port);
      delete [] src_port;
      delete [] dst_port;
      return;
    } else if (conn->transport_type != transport_type) {
      send_error(tc->tc_fd, "Message CONNECT_LISTEN_ACK for port "
        "connection %d:%s - %d:%s contains wrong transport type: %s "
        "was expected instead of %s.", src_compref, src_port,
        dst_compref, dst_port, get_transport_name(conn->transport_type),
        get_transport_name(transport_type));
      delete [] src_port;
      delete [] dst_port;
      return;
    }
    component_struct *dst_comp = components[dst_compref];
    switch (dst_comp->tc_state) {
    case TC_IDLE:
    case TC_CREATE:
    case TC_START:
    case TC_STOP:
    case TC_KILL:
    case TC_CONNECT:
    case TC_DISCONNECT:
    case TC_MAP:
    case TC_UNMAP:
    case TC_STOPPING:
    case MTC_TESTCASE:
    case PTC_FUNCTION:
    case PTC_STARTING:
    case PTC_STOPPED:
      if (src_compref != MTC_COMPREF && src_compref != dst_compref) {
        // send the name of tc
        send_connect(dst_comp, dst_port, src_compref, tc->comp_name,
          src_port, transport_type, local_addr_len, local_addr_ptr);
      } else {
        // send an empty string instead of the name of tc if it is
        // known by dst_comp
        send_connect(dst_comp, dst_port, src_compref, NULL, src_port,
          transport_type, local_addr_len, local_addr_ptr);
      }
      conn->conn_state = CONN_CONNECTING;
      break;
    default:
      send_disconnect_to_server(conn);
      send_error_to_connect_requestors(conn, "test component %d has "
        "terminated during connection setup.", dst_compref);
      remove_connection(conn);
    }
    status_change();
  } else {
    // the connection does not exist anymore
    // check whether the transport type is valid
    switch (transport_type) {
    case TRANSPORT_LOCAL:
      send_error(tc->tc_fd, "Message CONNECT_LISTEN_ACK for port "
        "connection %d:%s - %d:%s cannot refer to transport type %s.",
        src_compref, src_port, dst_compref, dst_port,
        get_transport_name(transport_type));
      break;
    case TRANSPORT_INET_STREAM:
    case TRANSPORT_UNIX_STREAM:
      break;
    default:
      send_error(tc->tc_fd, "Message CONNECT_LISTEN_ACK for port "
        "connection %d:%s - %d:%s refers to invalid transport type %d.",
        src_compref, src_port, dst_compref, dst_port, transport_type);
    }
  }

  delete [] src_port;
  delete [] dst_port;
}

void MainController::process_connected(component_struct *tc)
{
  if (!message_expected(tc, "CONNECTED")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component src_compref = tc->comp_ref;
  char *src_port = text_buf.pull_string();
  component dst_compref = text_buf.pull_int().get_val();
  char *dst_port = text_buf.pull_string();

  port_connection *conn = find_connection(src_compref, src_port, dst_compref,
    dst_port);
  if (conn != NULL) {
    // this message must arrive in the right state
    // and from the server side (head)
    if (conn->conn_state == CONN_CONNECTING &&
      conn->head.comp_ref == src_compref &&
      !strcmp(conn->head.port_name, src_port)) {
      send_connect_ack_to_requestors(conn);
      conn->conn_state = CONN_CONNECTED;
      status_change();
    } else {
      send_error(tc->tc_fd, "Unexpected CONNECTED message was "
        "received for port connection %d:%s - %d:%s.",
        src_compref, src_port, dst_compref, dst_port);
    }
  }
  // do nothing if the connection does not exist anymore

  delete [] src_port;
  delete [] dst_port;
}

void MainController::process_connect_error(component_struct *tc)
{
  if (!message_expected(tc, "CONNECT_ERROR")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component src_compref = tc->comp_ref;
  char *src_port = text_buf.pull_string();
  component dst_compref = text_buf.pull_int().get_val();
  char *dst_port = text_buf.pull_string();
  char *reason = text_buf.pull_string();

  port_connection *conn = find_connection(src_compref, src_port, dst_compref,
    dst_port);
  if (conn != NULL) {
    switch (conn->conn_state) {
    case CONN_CONNECTING:
      // in this state both endpoints can report error
      if (conn->transport_type != TRANSPORT_LOCAL &&
        conn->tail.comp_ref == src_compref &&
        !strcmp(conn->tail.port_name, src_port)) {
        // shut down the server side (head) only if the error was reported
        // by the client side (tail)
        send_disconnect_to_server(conn);
      }
      break;
    case CONN_LISTENING:
      // in this state only the server side (head) can report the error
      if (conn->head.comp_ref == src_compref &&
        !strcmp(conn->head.port_name, src_port)) break;
    default:
      send_error(tc->tc_fd, "Unexpected message CONNECT_ERROR was "
        "received for port connection %d:%s - %d:%s.",
        src_compref, src_port, dst_compref, dst_port);
      delete [] src_port;
      delete [] dst_port;
      delete [] reason;
      return;
    }
    send_error_to_connect_requestors(conn, "test component %d reported "
      "error: %s", src_compref, reason);
    remove_connection(conn);
    status_change();
  }
  // do nothing if the connection does not exist anymore

  delete [] src_port;
  delete [] dst_port;
  delete [] reason;
}

void MainController::process_disconnect_req(component_struct *tc)
{
  if (!request_allowed(tc, "DISCONNECT_REQ")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component src_compref = text_buf.pull_int().get_val();
  char *src_port = text_buf.pull_string();
  component dst_compref = text_buf.pull_int().get_val();
  char *dst_port = text_buf.pull_string();

  if (!valid_endpoint(src_compref, FALSE, tc, "disconnect") ||
    !valid_endpoint(dst_compref, FALSE, tc, "disconnect")) {
    delete [] src_port;
    delete [] dst_port;
    return;
  }

  port_connection *conn = find_connection(src_compref, src_port, dst_compref,
    dst_port);
  if (conn != NULL) {
    switch (conn->conn_state) {
    case CONN_LISTENING:
    case CONN_CONNECTING:
      send_error(tc->tc_fd, "The port connection %d:%s - %d:%s cannot "
        "be destroyed because a connect operation is in progress "
        "on it.", src_compref, src_port, dst_compref, dst_port);
      break;
    case CONN_CONNECTED:
      send_disconnect(components[conn->tail.comp_ref],
        conn->tail.port_name, conn->head.comp_ref,
        conn->head.port_name);
      conn->conn_state = CONN_DISCONNECTING;
      // no break
    case CONN_DISCONNECTING:
      add_requestor(&conn->requestors, tc);
      tc->tc_state = TC_DISCONNECT;
      status_change();
      break;
    default:
      send_error(tc->tc_fd, "The port connection %d:%s - %d:%s cannot "
        "be destroyed due to an internal error in the MC.",
        src_compref, src_port, dst_compref, dst_port);
      error("The port connection %d:%s - %d:%s is in invalid state when "
        "a disconnect operation was requested on it.", src_compref,
        src_port, dst_compref, dst_port);
    }
  } else {
    // the connection is already terminated
    // send the acknowledgement immediately
    send_disconnect_ack(tc);
  }

  delete [] src_port;
  delete [] dst_port;
}

void MainController::process_disconnected(component_struct *tc)
{
  if (!message_expected(tc, "DISCONNECTED")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component src_compref = tc->comp_ref;
  char *src_port = text_buf.pull_string();
  component dst_compref = text_buf.pull_int().get_val();
  char *dst_port = text_buf.pull_string();

  port_connection *conn = find_connection(src_compref, src_port, dst_compref,
    dst_port);
  if (conn != NULL) {
    switch (conn->conn_state) {
    case CONN_LISTENING:
      // in this state only the server side (head) can report the end of
      // the connection
      if (conn->head.comp_ref != src_compref ||
        strcmp(conn->head.port_name, src_port)) {
        send_error(tc->tc_fd, "Unexpected message DISCONNECTED was "
          "received for port connection %d:%s - %d:%s.",
          src_compref, src_port, dst_compref, dst_port);
        break;
      }
      // no break
    case CONN_CONNECTING:
      // in this state both ends can report the end of the connection
      send_error_to_connect_requestors(conn, "test component %d "
        "reported end of the connection during connection setup.",
        src_compref);
      remove_connection(conn);
      status_change();
      break;
    case CONN_CONNECTED:
      remove_connection(conn);
      status_change();
      break;
    case CONN_DISCONNECTING:
      send_disconnect_ack_to_requestors(conn);
      remove_connection(conn);
      status_change();
      break;
    default:
      error("The port connection %d:%s - %d:%s is in invalid state when "
        "MC was notified about its termination.", src_compref, src_port,
        dst_compref, dst_port);
    }
  }

  delete [] src_port;
  delete [] dst_port;
  status_change();
}

void MainController::process_map_req(component_struct *tc)
{
  if (!request_allowed(tc, "MAP_REQ")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component src_compref = text_buf.pull_int().get_val();
  char *src_port = text_buf.pull_string();
  char *system_port = text_buf.pull_string();

  if (!valid_endpoint(src_compref, TRUE, tc, "map")) {
    delete [] src_port;
    delete [] system_port;
    return;
  }

  port_connection *conn = find_connection(src_compref, src_port,
    SYSTEM_COMPREF, system_port);
  if (conn == NULL) {
    send_map(components[src_compref], src_port, system_port);
    conn = new port_connection;
    conn->head.comp_ref = src_compref;
    conn->head.port_name = src_port;
    conn->tail.comp_ref = SYSTEM_COMPREF;
    conn->tail.port_name = system_port;
    conn->conn_state = CONN_MAPPING;
    init_requestors(&conn->requestors, tc);
    add_connection(conn);
    tc->tc_state = TC_MAP;
    status_change();
  } else {
    switch (conn->conn_state) {
    case CONN_MAPPING:
      add_requestor(&conn->requestors, tc);
      tc->tc_state = TC_MAP;
      status_change();
      break;
    case CONN_MAPPED:
      send_map_ack(tc);
      break;
    case CONN_UNMAPPING:
      send_error(tc->tc_fd, "The port mapping %d:%s - system:%s cannot "
        "be established because an unmap operation is in progress "
        "on it.", src_compref, src_port, system_port);
      break;
    default:
      send_error(tc->tc_fd, "The port mapping %d:%s - system:%s is in "
        "invalid state.", src_compref, src_port, system_port);
    }
    delete [] src_port;
    delete [] system_port;
  }
}

void MainController::process_mapped(component_struct *tc)
{
  if (!message_expected(tc, "MAPPED")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component src_compref = tc->comp_ref;
  char *src_port = text_buf.pull_string();
  char *system_port = text_buf.pull_string();

  port_connection *conn = find_connection(src_compref, src_port,
    SYSTEM_COMPREF, system_port);
  if (conn == NULL) {
    send_error(tc->tc_fd, "The MAPPED message refers to a "
      "non-existent port mapping %d:%s - system:%s.",
      src_compref, src_port, system_port);
  } else if (conn->conn_state != CONN_MAPPING) {
    send_error(tc->tc_fd, "Unexpected MAPPED message was "
      "received for port mapping %d:%s - system:%s.",
      src_compref, src_port, system_port);
  } else {
    for (int i = 0; ; i++) {
      component_struct *comp = get_requestor(&conn->requestors, i);
      if (comp == NULL) break;
      if (comp->tc_state == TC_MAP) {
        send_map_ack(comp);
        if (comp == mtc) comp->tc_state = MTC_TESTCASE;
        else comp->tc_state = PTC_FUNCTION;
      }
    }
    free_requestors(&conn->requestors);
    conn->conn_state = CONN_MAPPED;
    status_change();
  }

  delete [] src_port;
  delete [] system_port;
}

void MainController::process_unmap_req(component_struct *tc)
{
  if (!request_allowed(tc, "UNMAP_REQ")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component src_compref = text_buf.pull_int().get_val();
  char *src_port = text_buf.pull_string();
  char *system_port = text_buf.pull_string();

  if (!valid_endpoint(src_compref, FALSE, tc, "unmap")) {
    delete [] src_port;
    delete [] system_port;
    return;
  }

  port_connection *conn = find_connection(src_compref, src_port,
    SYSTEM_COMPREF, system_port);
  if (conn == NULL) {
    send_unmap_ack(tc);
  } else {
    switch (conn->conn_state) {
    case CONN_MAPPED:
      send_unmap(components[src_compref], src_port, system_port);
      conn->conn_state = CONN_UNMAPPING;
    case CONN_UNMAPPING:
      add_requestor(&conn->requestors, tc);
      tc->tc_state = TC_UNMAP;
      status_change();
      break;
    case CONN_MAPPING:
      send_error(tc->tc_fd, "The port mapping %d:%s - system:%s cannot "
        "be destroyed because a map operation is in progress "
        "on it.", src_compref, src_port, system_port);
      break;
    default:
      send_error(tc->tc_fd, "The port mapping %d:%s - system:%s is in "
        "invalid state.", src_compref, src_port, system_port);
    }
  }

  delete [] src_port;
  delete [] system_port;
}

void MainController::process_unmapped(component_struct *tc)
{
  if (!message_expected(tc, "UNMAPPED")) return;

  Text_Buf& text_buf = *tc->text_buf;
  component src_compref = tc->comp_ref;
  char *src_port = text_buf.pull_string();
  char *system_port = text_buf.pull_string();

  port_connection *conn = find_connection(src_compref, src_port,
    SYSTEM_COMPREF, system_port);
  if (conn != NULL) {
    switch (conn->conn_state) {
    case CONN_MAPPING:
    case CONN_MAPPED:
    case CONN_UNMAPPING:
      destroy_mapping(conn);
      break;
    default:
      send_error(tc->tc_fd, "Unexpected UNMAPPED message was "
        "received for port mapping %d:%s - system:%s.",
        src_compref, src_port, system_port);
    }
  }

  delete [] src_port;
  delete [] system_port;
  status_change();
}

void MainController::process_debug_return_value(Text_Buf& text_buf, char* log_source,
                                                int msg_end, bool from_mtc)
{
  int return_type = text_buf.pull_int().get_val();
  if (text_buf.get_pos() != msg_end) {
    timeval tv;
    tv.tv_sec = text_buf.pull_int().get_val();
    tv.tv_usec = text_buf.pull_int().get_val();
    char* message = text_buf.pull_string();
    if (return_type == DRET_DATA) {
      char* result = mprintf("\n%s", message);
      notify(&tv, log_source, TTCN_Logger::DEBUG_UNQUALIFIED, result);
      Free(result);
    }
    else {
      notify(&tv, log_source, TTCN_Logger::DEBUG_UNQUALIFIED, message);
    }
    delete [] message;
  }
  if (from_mtc) {
    if (return_type == DRET_SETTING_CHANGE) {
      switch (last_debug_command.command) {
      case D_SWITCH:
        Free(debugger_settings.on_switch);
        debugger_settings.on_switch = mcopystr(last_debug_command.arguments);
        break;
      case D_SET_OUTPUT: {
        Free(debugger_settings.output_type);
        Free(debugger_settings.output_file);
        debugger_settings.output_file = NULL;
        size_t args_len = mstrlen(last_debug_command.arguments);
        size_t start = 0;
        size_t end = 0;
        get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
        debugger_settings.output_type = mcopystrn(last_debug_command.arguments + start, end - start);
        if (end < args_len) {
          start = end;
          get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
          debugger_settings.output_file = mcopystrn(last_debug_command.arguments + start, end - start);
        }
        break; }
      case D_SET_AUTOMATIC_BREAKPOINT: {
        size_t args_len = mstrlen(last_debug_command.arguments);
        size_t start = 0;
        size_t end = 0;
        get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
        char* event_str = mcopystrn(last_debug_command.arguments + start, end - start);
        char** event_behavior;
        char** event_batch_file;
        if (!strcmp(event_str, "error")) {
          event_behavior = &debugger_settings.error_behavior;
          event_batch_file = &debugger_settings.error_batch_file;
        }
        else if (!strcmp(event_str, "fail")) {
          event_behavior = &debugger_settings.fail_behavior;
          event_batch_file = &debugger_settings.fail_batch_file;
        }
        else { // should never happen
          Free(event_str);
          break;
        }
        Free(event_str);
        Free(*event_behavior);
        Free(*event_batch_file);
        *event_batch_file = NULL;
        start = end;
        get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
        *event_behavior = mcopystrn(last_debug_command.arguments + start, end - start);
        if (end < args_len) {
          start = end;
          get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
          *event_batch_file = mcopystrn(last_debug_command.arguments + start, end - start);
        }
        break; }
      case D_SET_GLOBAL_BATCH_FILE: {
        Free(debugger_settings.global_batch_state);
        Free(debugger_settings.global_batch_file);
        debugger_settings.global_batch_file = NULL;
        size_t args_len = mstrlen(last_debug_command.arguments);
        size_t start = 0;
        size_t end = 0;
        get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
        debugger_settings.global_batch_state = mcopystrn(last_debug_command.arguments + start, end - start);
        if (end < args_len) {
          start = end;
          get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
          debugger_settings.global_batch_file = mcopystrn(last_debug_command.arguments + start, end - start);
        }
        break; }
      case D_SET_BREAKPOINT: {
        size_t args_len = mstrlen(last_debug_command.arguments);
        size_t start = 0;
        size_t end = 0;
        get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
        char* module = mcopystrn(last_debug_command.arguments + start, end - start);
        start = end;
        get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
        char* line = mcopystrn(last_debug_command.arguments + start, end - start);
        char* batch_file = NULL;
        if (end < args_len) {
          start = end;
          get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
          batch_file = mcopystrn(last_debug_command.arguments + start, end - start);
        }
        int pos;
        for (pos = 0; pos < debugger_settings.nof_breakpoints; ++pos) {
          if (!strcmp(debugger_settings.breakpoints[pos].module, module) &&
              !strcmp(debugger_settings.breakpoints[pos].line, line)) {
            break;
          }
        }
        if (pos == debugger_settings.nof_breakpoints) {
          // not found, add a new one
          debugger_settings.breakpoints = (debugger_settings_struct::breakpoint_struct*)
            Realloc(debugger_settings.breakpoints, (debugger_settings.nof_breakpoints + 1) *
            sizeof(debugger_settings_struct::breakpoint_struct));
          ++debugger_settings.nof_breakpoints;
          debugger_settings.breakpoints[pos].module = module;
          debugger_settings.breakpoints[pos].line = line;
        }
        else {
          Free(debugger_settings.breakpoints[pos].batch_file);
          Free(module);
          Free(line);
        }
        debugger_settings.breakpoints[pos].batch_file = batch_file;
        break; }
      case D_REMOVE_BREAKPOINT:
        if (!strcmp(last_debug_command.arguments, "all")) {
          for (int i = 0; i < debugger_settings.nof_breakpoints; ++i) {
            Free(debugger_settings.breakpoints[i].module);
            Free(debugger_settings.breakpoints[i].line);
            Free(debugger_settings.breakpoints[i].batch_file);
          }
          Free(debugger_settings.breakpoints);
          debugger_settings.breakpoints = NULL;
          debugger_settings.nof_breakpoints = 0;
        }
        else {
          size_t args_len = mstrlen(last_debug_command.arguments);
          size_t start = 0;
          size_t end = 0;
          get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
          char* module = mcopystrn(last_debug_command.arguments + start, end - start);
          start = end;
          get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
          char* line = mcopystrn(last_debug_command.arguments + start, end - start);
          bool all_in_module = !strcmp(line, "all");
          for (int i = 0; i < debugger_settings.nof_breakpoints; ++i) {
            if (!strcmp(debugger_settings.breakpoints[i].module, module) &&
                (all_in_module || !strcmp(debugger_settings.breakpoints[i].line, line))) {
              Free(debugger_settings.breakpoints[i].module);
              Free(debugger_settings.breakpoints[i].line);
              Free(debugger_settings.breakpoints[i].batch_file);
              for (int j = i; j < debugger_settings.nof_breakpoints - 1; ++j) {
                debugger_settings.breakpoints[j] = debugger_settings.breakpoints[j + 1];
              }
              --debugger_settings.nof_breakpoints;
              if (!all_in_module) {
                break;
              }
            }
          }
          debugger_settings.breakpoints = (debugger_settings_struct::breakpoint_struct*)
            Realloc(debugger_settings.breakpoints, debugger_settings.nof_breakpoints *
            sizeof(debugger_settings_struct::breakpoint_struct));
          Free(module);
          Free(line);
        }
        break;
      case D_FUNCTION_CALL_CONFIG: {
        Free(debugger_settings.function_calls_cfg);
        Free(debugger_settings.function_calls_file);
        debugger_settings.function_calls_file = NULL;
        size_t args_len = mstrlen(last_debug_command.arguments);
        size_t start = 0;
        size_t end = 0;
        get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
        debugger_settings.function_calls_cfg = mcopystrn(last_debug_command.arguments + start, end - start);
        if (end < args_len) {
          start = end;
          get_next_argument_loc(last_debug_command.arguments, args_len, start, end);
          debugger_settings.function_calls_file = mcopystrn(last_debug_command.arguments + start, end - start);
        }
        break; }
      default:
        break;
      }
    }
    else if (return_type == DRET_EXIT_ALL) {
      stop_requested = TRUE;
    }
  }
}

static bool is_tc_debuggable(const component_struct* tc)
{
  if (tc->comp_ref == MTC_COMPREF || tc->comp_ref == SYSTEM_COMPREF) {
    return true; // let these pass, they are checked later
  }
  switch (tc->tc_state) {
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case PTC_FUNCTION:
  case PTC_STARTING:
    return true;
  default:
    return false;
  }
}

void MainController::process_debug_broadcast_req(component_struct* tc, int commandID)
{
  // don't send the command back to the requesting component
  if (tc != mtc) {
    send_debug_command(mtc->tc_fd, commandID, "");
  }
  for (component i = tc_first_comp_ref; i < n_components; ++i) {
    component_struct* comp = components[i];
    if (tc != comp && is_tc_debuggable(comp)) {
      send_debug_command(comp->tc_fd, commandID, "");
    }
  }
  debugger_active_tc = tc;
  for (int i = 0; i < n_hosts; i++) {
    host_struct* host = hosts[i];
    if (host->hc_state != HC_DOWN) {
      send_debug_command(hosts[i]->hc_fd, commandID, "");
    }
  }
}

void MainController::process_debug_batch(component_struct* tc)
{
  Text_Buf& text_buf = *tc->text_buf;
  const char* batch_file = text_buf.pull_string();
  unlock();
  ui->executeBatchFile(batch_file);
  lock();
  delete [] batch_file;
}

void MainController::process_testcase_started()
{
  if (mc_state != MC_EXECUTING_CONTROL) {
    send_error_str(mtc->tc_fd, "Unexpected message TESTCASE_STARTED "
      "was received.");
    return;
  }

  Text_Buf& text_buf = *mtc->text_buf;
  text_buf.pull_qualified_name(mtc->tc_fn_name);
  text_buf.pull_qualified_name(mtc->comp_type);
  text_buf.pull_qualified_name(system->comp_type);

  mtc->tc_state = MTC_TESTCASE;
  mc_state = MC_EXECUTING_TESTCASE;
  tc_first_comp_ref = next_comp_ref;
  any_component_done_requested = FALSE;
  any_component_done_sent = FALSE;
  all_component_done_requested = FALSE;
  any_component_killed_requested = FALSE;
  all_component_killed_requested = FALSE;

  status_change();
}

void MainController::process_testcase_finished()
{
  if (mc_state != MC_EXECUTING_TESTCASE) {
    send_error_str(mtc->tc_fd, "Unexpected message TESTCASE_FINISHED "
      "was received.");
    return;
  }

  boolean ready_to_finish = kill_all_components(TRUE);

  mc_state = MC_TERMINATING_TESTCASE;
  mtc->tc_state = MTC_TERMINATING_TESTCASE;
  mtc->local_verdict = (verdicttype)mtc->text_buf->pull_int().get_val();
  mtc->verdict_reason = mtc->text_buf->pull_string();
  mtc->stop_requested = FALSE;
  if (mtc->kill_timer != NULL) {
    cancel_timer(mtc->kill_timer);
    mtc->kill_timer = NULL;
  }
  any_component_done_requested = FALSE;
  any_component_done_sent = FALSE;
  all_component_done_requested = FALSE;
  any_component_killed_requested = FALSE;
  all_component_killed_requested = FALSE;

  if (ready_to_finish) finish_testcase();

  status_change();
}

void MainController::process_mtc_ready()
{
  if (mc_state != MC_EXECUTING_CONTROL || mtc->tc_state != MTC_CONTROLPART) {
    send_error_str(mtc->tc_fd, "Unexpected message MTC_READY was "
      "received.");
    return;
  }
  mc_state = MC_READY;
  mtc->tc_state = TC_IDLE;
  mtc->stop_requested = FALSE;
  if (mtc->kill_timer != NULL) {
    cancel_timer(mtc->kill_timer);
    mtc->kill_timer = NULL;
  }
  stop_requested = FALSE;
  notify("Test execution finished.");
  status_change();
}

void MainController::process_configure_ack_mtc()
{
  if (mtc->tc_state != MTC_CONFIGURING) {
    send_error_str(mtc->tc_fd, "Unexpected message CONFIGURE_ACK was received.");
    return;
  }
  mtc->tc_state = TC_IDLE;
  notify("Configuration file was processed on the MTC.");
}

void MainController::process_configure_nak_mtc()
{
  if (mtc->tc_state != MTC_CONFIGURING) {
    send_error_str(mtc->tc_fd, "Unexpected message CONFIGURE_NAK was received.");
    return;
  }
  mtc->tc_state = TC_IDLE;
  notify("Processing of configuration file failed on the MTC.");
}

void MainController::process_stopped(component_struct *tc, int message_end)
{
  switch (tc->tc_state) {
  case TC_STOPPING:
  case PTC_FUNCTION:
  case PTC_STOPPING_KILLING:
    // only alive PTCs are allowed to send STOPPED
    if (tc->is_alive) break;
  default:
    send_error_str(tc->tc_fd, "Unexpected message STOPPED was received.");
    return;
  }
  Text_Buf& text_buf = *tc->text_buf;
  delete [] tc->return_type;
  tc->return_type = text_buf.pull_string();
  tc->return_value_len = message_end - text_buf.get_pos();
  Free(tc->return_value);
  tc->return_value = Malloc(tc->return_value_len);
  text_buf.pull_raw(tc->return_value_len, tc->return_value);
  free_qualified_name(&tc->tc_fn_name);
  component_stopped(tc);
  status_change();
}

void MainController::process_stopped_killed(component_struct *tc,
  int message_end)
{
  switch (tc->tc_state) {
  case TC_CREATE:
  case TC_START:
  case TC_STOP:
  case TC_KILL:
  case TC_CONNECT:
  case TC_DISCONNECT:
  case TC_MAP:
  case TC_UNMAP:
  case TC_STOPPING:
  case PTC_FUNCTION:
  case PTC_STOPPING_KILLING:
    break;
  default:
    send_error_str(tc->tc_fd, "Unexpected message STOPPED_KILLED was "
      "received.");
    // also notify the user because the above message may get lost
    notify("Unexpected message STOPPED_KILLED was received from PTC %d.",
      tc->comp_ref);
    return;
  }
  Text_Buf& text_buf = *tc->text_buf;
  tc->local_verdict = (verdicttype)text_buf.pull_int().get_val();
  tc->verdict_reason = text_buf.pull_string();
  tc->return_type = text_buf.pull_string();
  tc->return_value_len = message_end - text_buf.get_pos();
  tc->return_value = Malloc(tc->return_value_len);
  text_buf.pull_raw(tc->return_value_len, tc->return_value);
  // start a guard timer to detect whether the control connection is closed
  // in time
  if (tc->tc_state != PTC_STOPPING_KILLING) start_kill_timer(tc);
  component_terminated(tc);
  status_change();
}

void MainController::process_killed(component_struct *tc)
{
  switch (tc->tc_state) {
  case TC_IDLE:
  case PTC_STOPPED:
  case PTC_KILLING:
    break;
  default:
    send_error_str(tc->tc_fd, "Unexpected message KILLED was received.");
    // also notify the user because the above message may get lost
    notify("Unexpected message KILLED was received from PTC %d.",
      tc->comp_ref);
    return;
  }
  tc->local_verdict = (verdicttype)tc->text_buf->pull_int().get_val();
  tc->verdict_reason = tc->text_buf->pull_string();
  // start a guard timer to detect whether the control connection is closed
  // in time
  if (tc->tc_state != PTC_KILLING) start_kill_timer(tc);
  component_terminated(tc);
  status_change();
}

void MainController::initialize(UserInterface& par_ui, int par_max_ptcs)
{
  ui = &par_ui;

  max_ptcs = par_max_ptcs;

  mc_state = MC_INACTIVE;

  struct utsname buf;
  if (uname(&buf) < 0) fatal_error("MainController::initialize: "
    "uname() system call failed.");
  mc_hostname = mprintf("MC@%s", buf.nodename);

  server_fd = -1;

  if (pthread_mutex_init(&mutex, NULL))
    fatal_error("MainController::initialize: pthread_mutex_init failed.");

#ifdef USE_EPOLL
  epoll_events = NULL;
  epfd = -1;
#else
  nfds = 0;
  ufds = NULL;
  new_nfds = 0;
  new_ufds = NULL;
  pollfds_modified = FALSE;
#endif

  fd_table_size = 0;
  fd_table = NULL;

  unknown_head = NULL;
  unknown_tail = NULL;

  n_host_groups = 0;
  host_groups = NULL;
  init_string_set(&assigned_components);
  all_components_assigned = FALSE;

  n_hosts = 0;
  hosts = NULL;
  config_str = NULL;
  
  debugger_settings.on_switch = NULL;
  debugger_settings.output_type = NULL;
  debugger_settings.output_file = NULL;
  debugger_settings.error_behavior = NULL;
  debugger_settings.error_batch_file = NULL;
  debugger_settings.fail_behavior = NULL;
  debugger_settings.fail_batch_file = NULL;
  debugger_settings.global_batch_state = NULL;
  debugger_settings.global_batch_file = NULL;
  debugger_settings.function_calls_cfg = NULL;
  debugger_settings.function_calls_file = NULL;
  debugger_settings.nof_breakpoints = 0;
  debugger_settings.breakpoints = NULL;
  last_debug_command.command = D_ERROR;
  last_debug_command.arguments = NULL;

  version_known = FALSE;
  n_modules = 0;
  modules = NULL;

  n_components = 0;
  n_active_ptcs = 0;
  components = NULL;
  mtc = NULL;
  system = NULL;
  debugger_active_tc = NULL;
  next_comp_ref = FIRST_PTC_COMPREF;

  stop_after_tc = FALSE;
  stop_requested = FALSE;

  kill_timer = 10.0;

  timer_head = NULL;
  timer_tail = NULL;

  pipe_fd[0] = -1;
  pipe_fd[1] = -1;
  wakeup_reason = REASON_NOTHING;

  register_termination_handlers();
}

void MainController::terminate()
{
  clean_up();
  destroy_host_groups();
  Free(mc_hostname);
  pthread_mutex_destroy(&mutex);
}

void MainController::add_host(const char *group_name, const char *host_name)
{
  lock();
  if (mc_state != MC_INACTIVE) {
    error("MainController::add_host: called in wrong state.");
    unlock();
    return;
  }
  host_group_struct *group = add_host_group(group_name);
  if (host_name != NULL) {
    if (group->has_all_hosts) error("Redundant member `%s' was ignored in "
      "host group `%s'. All hosts (`*') are already the members of the "
      "group.", host_name, group_name);
    else {
      if (set_has_string(&group->host_members, host_name)) {
        error("Duplicate member `%s' was ignored in host group "
          "`%s'.", host_name, group_name);
      } else add_string_to_set(&group->host_members, host_name);
    }
  } else {
    if (group->has_all_hosts) error("Duplicate member `*' was ignored in "
      "host group `%s'.", group_name);
    else {
      for (int i = 0; ; i++) {
        const char *group_member =
          get_string_from_set(&group->host_members, i);
        if (group_member == NULL) break;
        error("Redundant member `%s' was ignored in host group `%s'. "
          "All hosts (`*') are already the members of the group.",
          group_member, group_name);
      }
      free_string_set(&group->host_members);
      group->has_all_hosts = TRUE;
    }
  }
  unlock();
}

void MainController::assign_component(const char *host_or_group,
  const char *component_id)
{
  lock();
  if (mc_state != MC_INACTIVE) {
    error("MainController::assign_component: called in wrong state.");
    unlock();
    return;
  }
  host_group_struct *group = add_host_group(host_or_group);
  if (component_id == NULL) {
    if (all_components_assigned) {
      for (int i = 0; i < n_host_groups; i++) {
        if (host_groups[i].has_all_components) {
          error("Duplicate assignment of all components (*) to host "
            "group `%s'. Previous assignment to group `%s' is "
            "ignored.", host_or_group, host_groups[i].group_name);
          host_groups[i].has_all_components = FALSE;
        }
      }
    } else all_components_assigned = TRUE;
    group->has_all_components = TRUE;
  } else {
    if (set_has_string(&assigned_components, component_id)) {
      for (int i = 0; i < n_host_groups; i++) {
        if (set_has_string(&host_groups[i].assigned_components,
          component_id)) {
          error("Duplicate assignment of component `%s' to host "
            "group `%s'. Previous assignment to group `%s' is "
            "ignored.", component_id, host_or_group,
            host_groups[i].group_name);
          remove_string_from_set(&host_groups[i].assigned_components,
            component_id);
        }
      }
    } else add_string_to_set(&assigned_components, component_id);
    add_string_to_set(&group->assigned_components, component_id);
  }
  unlock();
}

void MainController::destroy_host_groups()
{
  lock();
  if (mc_state != MC_INACTIVE)
    error("MainController::destroy_host_groups: called in wrong state.");
  else {
    for (int i = 0; i < n_host_groups; i++) {
      host_group_struct *group = host_groups + i;
      Free(group->group_name);
      free_string_set(&group->host_members);
      free_string_set(&group->assigned_components);
    }
    Free(host_groups);
    n_host_groups = 0;
    host_groups = NULL;
    free_string_set(&assigned_components);
    all_components_assigned = FALSE;
  }
  unlock();
}

void MainController::set_kill_timer(double timer_val)
{
  lock();
  switch (mc_state) {
  case MC_INACTIVE:
  case MC_LISTENING:
  case MC_HC_CONNECTED:
  case MC_RECONFIGURING:
    if (timer_val < 0.0)
      error("MainController::set_kill_timer: setting a negative kill timer "
        "value.");
    else kill_timer = timer_val;
    break;
  default:
    error("MainController::set_kill_timer: called in wrong state.");
    break;
  }
  unlock();
}

unsigned short MainController::start_session(const char *local_address,
  unsigned short tcp_port, bool unix_sockets_enabled)
{
  lock();

  if (mc_state != MC_INACTIVE) {
    error("MainController::start_session: called in wrong state.");
    unlock();
    return 0;
  }

#ifdef USE_EPOLL
  epoll_events = (epoll_event *)Malloc(EPOLL_MAX_EVENTS * sizeof(*epoll_events));
  epfd = epoll_create(EPOLL_SIZE_HINT);
  if (epfd < 0) {
    error("System call epoll_create failed: %s", strerror(errno));
    clean_up();
    unlock();
    return 0;
  }
  set_close_on_exec(epfd);
#endif //USE_EPOLL

  nh.set_family(local_address);
  server_fd = nh.socket();
  if (server_fd < 0) {
    error("Server socket creation failed: %s", strerror(errno));
    clean_up();
    unlock();
    return 0;
  }

  const int on = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*) &on,
    sizeof(on))) {
    error("System call setsockopt (SO_REUSEADDR) failed on server socket: "
      "%s", strerror(errno));
    clean_up();
    unlock();
    return 0;
  }

  if (setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY, (const char*) &on,
    sizeof(on))) {
    error("System call setsockopt (TCP_NODELAY) failed on server socket: "
      "%s", strerror(errno));
    clean_up();
    unlock();
    return 0;
  }

  IPAddress *localaddr = IPAddress::create_addr(nh.get_family());
  if (localaddr) localaddr->set_port(tcp_port);

  if (local_address != NULL) {
    if (!localaddr || !localaddr->set_addr(local_address, tcp_port)) {
      error("Cannot resolve host name `%s' to a local IP address: "
        "Host name lookup failure", local_address);
      clean_up();
      unlock();
      delete localaddr;
      return 0;
    }
  }

  if (bind(server_fd, localaddr->get_addr(), localaddr->get_addr_len())) {
    if (local_address != NULL) {
      if (tcp_port != 0) error("Binding server socket to IP address "
        "%s and TCP port %d failed: %s", localaddr->get_addr_str(),
        tcp_port, strerror(errno));
      else error("Binding server socket to IP address %s failed: %s",
        localaddr->get_addr_str(), strerror(errno));
    } else {
      if (tcp_port != 0) error("Binding server socket to TCP port %d "
        "failed: %s", tcp_port, strerror(errno));
      else error("Binding server socket to an ephemeral TCP port "
        "failed: %s", strerror(errno));
    }
    clean_up();
    unlock();
    delete localaddr;
    return 0;
  }

  if (listen(server_fd, 10)) {
    if (local_address != NULL) {
      if (tcp_port != 0) error("Listening on IP address %s and TCP port "
        "%d failed: %s", localaddr->get_addr_str(), tcp_port,
        strerror(errno));
      else error("Listening on IP address %s failed: %s",
        localaddr->get_addr_str(), strerror(errno));
    } else {
      if (tcp_port != 0) error("Listening on TCP port %d failed: %s",
        tcp_port, strerror(errno));
      else error("Listening on an ephemeral TCP port failed: %s",
        strerror(errno));
    }
    clean_up();
    unlock();
    delete localaddr;
    return 0;
  }

  if (localaddr->getsockname(server_fd)) {
    error("System call getsockname() failed on server socket: %s",
      strerror(errno));
    clean_up();
    unlock();
    delete localaddr;
    return 0;
  }
  tcp_port = localaddr->get_port();

  set_close_on_exec(server_fd);

  // Trying to open a unix socket for local communication
  if (unix_sockets_enabled) {

    server_fd_unix = socket(PF_UNIX, SOCK_STREAM, 0);
    if (server_fd_unix < 0) {
      notify("Unix server socket creation failed: %s", strerror(errno));
      errno = 0;
      goto unix_end;
    }

    struct sockaddr_un localaddr_unix;
    memset(&localaddr_unix, 0, sizeof(localaddr_unix));
    localaddr_unix.sun_family = AF_UNIX;
    snprintf(localaddr_unix.sun_path, sizeof(localaddr_unix.sun_path),
      "/tmp/ttcn3-mctr-%u", tcp_port);
    if (unlink(localaddr_unix.sun_path))
      errno = 0; // silently ignore, error handling below

    if (bind(server_fd_unix, (struct sockaddr *)&localaddr_unix,
      sizeof(localaddr_unix)) != 0) {
      if (errno == EADDRINUSE) {
        // the temporary file name is already used by someone else
        close(server_fd_unix);
        notify("Could not create Unix server socket: '%s' is already existed "
          "and cannot be removed.", localaddr_unix.sun_path);
        errno = 0;
        goto unix_end;
      } else {
        close(server_fd_unix);
        notify("Binding of Unix server socket to pathname %s failed. (%s)",
          localaddr_unix.sun_path, strerror(errno));
        errno = 0;
        goto unix_end;
      }
    }

    if (listen(server_fd_unix, 10)) {
      notify("Could not listen on the given socket. Unix domain socket "
        "communication will not be used.");
      close(server_fd_unix);
      errno = 0;
      goto unix_end;
    }

    set_close_on_exec(server_fd_unix);

    add_fd_to_table(server_fd_unix);
    fd_table[server_fd_unix].fd_type = FD_SERVER;
    add_poll_fd(server_fd_unix);

    notify("Unix server socket created successfully.");
  }
  unix_end:

  if (pipe(pipe_fd) < 0) {
    error("System call  pipe failed: %s", strerror(errno));
    clean_up();
    unlock();
    delete localaddr;
    return 0;
  }
  set_close_on_exec(pipe_fd[0]);
  set_close_on_exec(pipe_fd[1]);

  wakeup_reason = REASON_NOTHING;

  mc_state = MC_LISTENING;

  add_fd_to_table(server_fd);
  fd_table[server_fd].fd_type = FD_SERVER;
  add_poll_fd(server_fd);
  server_fd_disabled = FALSE;

  add_fd_to_table(pipe_fd[0]);
  fd_table[pipe_fd[0]].fd_type = FD_PIPE;
  add_poll_fd(pipe_fd[0]);

  pthread_attr_t attr;
  if (pthread_attr_init(&attr))
    fatal_error("MainController::start_session: pthread_attr_init failed.");
  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
    fatal_error("MainController::start_session: "
      "pthread_attr_setdetachstate failed.");

  pthread_t thread;
  if (pthread_create(&thread, &attr, thread_main, NULL))
    fatal_error("MainController::start_session: pthread_create failed.");

  if (pthread_attr_destroy(&attr))
    fatal_error("MainController::start_session: pthread_attr_destroy "
      "failed.");
  if (local_address != NULL)
    notify("Listening on IP address %s and TCP port %d.", localaddr->get_addr_str(), tcp_port);
  else notify("Listening on TCP port %d.", tcp_port);
  delete localaddr;
  status_change();
  unlock();
  return tcp_port;
}

void MainController::shutdown_session()
{
  lock();
  switch (mc_state) {
  case MC_INACTIVE:
    status_change();
    break;
  case MC_SHUTDOWN:
    break;
  case MC_LISTENING:
  case MC_LISTENING_CONFIGURED:
  case MC_HC_CONNECTED:
  case MC_ACTIVE:
    notify("Shutting down session.");
    wakeup_thread(REASON_SHUTDOWN);
    break;
  default:
    error("MainController::shutdown_session: called in wrong state.");
  }
  unlock();
}

char *MainController::config_str;

void MainController::configure(const char *config_file)
{
  lock();
  switch (mc_state) {
  case MC_HC_CONNECTED:
  case MC_ACTIVE:
    mc_state = MC_CONFIGURING;
    break;
  case MC_LISTENING:
  case MC_LISTENING_CONFIGURED:
    mc_state = MC_LISTENING_CONFIGURED;
    break;
  case MC_RECONFIGURING:
    break;
  default:
    error("MainController::configure: called in wrong state.");
    unlock();
    return;
  }
  Free(config_str);
  config_str = mcopystr(config_file);
  if (mc_state == MC_CONFIGURING || mc_state == MC_RECONFIGURING) {
    notify("Downloading configuration file to all HCs.");
    for (int i = 0; i < n_hosts; i++) configure_host(hosts[i], FALSE);
  }
  if (mc_state == MC_RECONFIGURING) {
    notify("Downloading configuration file to the MTC.");
    configure_mtc();
  }
  status_change();
  unlock();
}

bool MainController::start_reconfiguring()
{
  switch (mc_state) {
  case MC_READY:
    mc_state = MC_RECONFIGURING;
    return true;
  case MC_LISTENING:
  case MC_HC_CONNECTED:
    return true;
  default:
    lock();
    error("MainController::start_reconfiguring: called in wrong state.");
    unlock();
    return false;
  }
}

void MainController::create_mtc(int host_index)
{
  lock();
  if (mc_state != MC_ACTIVE) {
    error("MainController::create_mtc: called in wrong state.");
    unlock();
    return;
  }
  if (host_index < 0 || host_index >= n_hosts) {
    error("MainController::create_mtc: host index (%d) is out of range.",
      host_index);
    unlock();
    return;
  }
  host_struct *host = hosts[host_index];
  switch (host->hc_state) {
  case HC_OVERLOADED:
    notify("HC on host %s reported overload. Trying to create MTC there "
      "anyway.", host->hostname);
  case HC_ACTIVE:
    break;
  default:
    error("MTC cannot be created on %s: HC is not active.", host->hostname);
    unlock();
    return;
  }
  notify("Creating MTC on host %s.", host->hostname);
  send_create_mtc(host);

  mtc = new component_struct;
  mtc->comp_ref = MTC_COMPREF;
  init_qualified_name(&mtc->comp_type);
  mtc->comp_name = new char[4];
  strcpy(mtc->comp_name, "MTC");
  mtc->tc_state = TC_INITIAL;
  mtc->local_verdict = NONE;
  mtc->verdict_reason = NULL;
  mtc->tc_fd = -1;
  mtc->text_buf = NULL;
  init_qualified_name(&mtc->tc_fn_name);
  mtc->return_type = NULL;
  mtc->return_value_len = 0;
  mtc->return_value = NULL;
  mtc->is_alive = FALSE;
  mtc->stop_requested = FALSE;
  mtc->process_killed = FALSE;
  mtc->initial.create_requestor = NULL;
  mtc->initial.location_str = NULL;
  init_requestors(&mtc->done_requestors, NULL);
  init_requestors(&mtc->killed_requestors, NULL);
  init_requestors(&mtc->cancel_done_sent_for, NULL);
  mtc->kill_timer = NULL;
  init_connections(mtc);
  add_component(mtc);
  add_component_to_host(host, mtc);
  host->n_active_components++;

  system = new component_struct;
  system->comp_ref = SYSTEM_COMPREF;
  init_qualified_name(&system->comp_type);
  system->comp_name = new char[7];
  strcpy(system->comp_name, "SYSTEM");
  system->log_source = NULL;
  system->comp_location = NULL;
  system->tc_state = TC_SYSTEM;
  system->local_verdict = NONE;
  system->verdict_reason = NULL;
  system->tc_fd = -1;
  system->text_buf = NULL;
  init_qualified_name(&system->tc_fn_name);
  system->return_type = NULL;
  system->return_value_len = 0;
  system->return_value = NULL;
  system->is_alive = FALSE;
  system->stop_requested = FALSE;
  system->process_killed = FALSE;
  init_requestors(&system->done_requestors, NULL);
  init_requestors(&system->killed_requestors, NULL);
  init_requestors(&system->cancel_done_sent_for, NULL);
  system->kill_timer = NULL;
  init_connections(system);
  add_component(system);

  mc_state = MC_CREATING_MTC;
  status_change();
  unlock();
}

void MainController::exit_mtc()
{
  lock();
  if (mc_state != MC_READY && mc_state != MC_RECONFIGURING) {
    error("MainController::exit_mtc: called in wrong state.");
    unlock();
    return;
  }
  notify("Terminating MTC.");
  send_exit_mtc();
  mtc->tc_state = TC_EXITING;
  mtc->comp_location->n_active_components--;
  mc_state = MC_TERMINATING_MTC;
  start_kill_timer(mtc);
  status_change();
  unlock();
}

void MainController::execute_control(const char *module_name)
{
  lock();
  if (mc_state != MC_READY) {
    error("MainController::execute_control: called in wrong state.");
    unlock();
    return;
  }
  send_execute_control(module_name);
  mtc->tc_state = MTC_CONTROLPART;
  mc_state = MC_EXECUTING_CONTROL;
  status_change();
  unlock();
}

void MainController::execute_testcase(const char *module_name,
  const char *testcase_name)
{
  lock();
  if (mc_state != MC_READY) {
    error("MainController::execute_testcase: called in wrong state.");
    unlock();
    return;
  }
  send_execute_testcase(module_name, testcase_name);
  mtc->tc_state = MTC_CONTROLPART;
  mc_state = MC_EXECUTING_CONTROL;
  status_change();
  unlock();
}

void MainController::stop_after_testcase(boolean new_state)
{
  lock();
  stop_after_tc = new_state;
  if (mc_state == MC_PAUSED && !stop_after_tc) {
    unlock();
    continue_testcase();
  } else unlock();
}

void MainController::continue_testcase()
{
  lock();
  if (mc_state == MC_PAUSED) {
    notify("Resuming execution.");
    send_continue();
    mtc->tc_state = MTC_CONTROLPART;
    mc_state = MC_EXECUTING_CONTROL;
    status_change();
  } else error("MainController::continue_testcase: called in wrong state.");
  unlock();
}

void MainController::stop_execution()
{
  lock();
  if (!stop_requested) {
    notify("Stopping execution.");
    switch (mc_state) {
    case MC_PAUSED:
      mc_state = MC_EXECUTING_CONTROL;
      mtc->tc_state = MTC_CONTROLPART;
    case MC_EXECUTING_CONTROL:
      send_stop(mtc);
      mtc->stop_requested = TRUE;
      start_kill_timer(mtc);
      wakeup_thread(REASON_MTC_KILL_TIMER);
      break;
    case MC_EXECUTING_TESTCASE:
      if (!mtc->stop_requested) {
        send_stop(mtc);
        kill_all_components(TRUE);
        mtc->stop_requested = TRUE;
        start_kill_timer(mtc);
        wakeup_thread(REASON_MTC_KILL_TIMER);
      }
    case MC_TERMINATING_TESTCASE:
      // MTC will be stopped later in finish_testcase()
    case MC_READY:
      // do nothing
      break;
    default:
      error("MainController::stop_execution: called in wrong state.");
      unlock();
      return;
    }
    stop_requested = TRUE;
    status_change();
  } else notify("Stop was already requested. Operation ignored.");
  unlock();
}

void MainController::debug_command(int commandID, char* arguments)
{
  lock();
  if (mtc != NULL) {
    switch (commandID) {
    case D_LIST_COMPONENTS: // handled by the MC
      if (*arguments != 0) {
        notify("Invalid number of arguments, expected 0.");
      }
      else {
        // the active component is marked with an asterisk
        char* result = mprintf("%s(%d)%s", mtc->comp_name, mtc->comp_ref,
          debugger_active_tc == mtc ? "*" : "");
        for (component i = FIRST_PTC_COMPREF; i < n_components; ++i) {
          component_struct* comp = components[i];
          if (comp != NULL && is_tc_debuggable(comp)) {
            if (comp->comp_name != NULL) {
              result = mputprintf(result, " %s(%d)%s", comp->comp_name, comp->comp_ref,
                debugger_active_tc == comp ? "*" : "");
            }
            else {
              result = mputprintf(result, " %d%s", comp->comp_ref,
                debugger_active_tc == comp ? "*" : "");
            }
          }
        }
        notify("%s", result);
        Free(result);
      }
      break;
    case D_SET_COMPONENT: { // handled by the MC
      bool number = true;
      size_t len = strlen(arguments);
      for (size_t i = 0; i < len; ++i) {
        if (arguments[i] < '0' || arguments[i] > '9') {
          number = false;
          break;
        }
      }
      component_struct* tc = NULL;
      if (number) { // component reference
        tc = lookup_component(strtol(arguments, NULL, 10));
      }
      else { // component name
        for (component i = 1; i < n_components; ++i) {
          component_struct *comp = components[i];
          if (comp != NULL && comp->comp_name != NULL && is_tc_debuggable(comp)
              && !strcmp(comp->comp_name, arguments)) {
            tc = comp;
            break;            
          } 
        }
      }
      if (tc == system) {
        notify("Debugging is not available on %s(%d).", tc->comp_name, tc->comp_ref);
      }
      else if (tc == NULL || !is_tc_debuggable(tc)) {
        notify("Component with %s %s does not exist or is not running anything.",
          number ? "reference" : "name", arguments);
      }
      else {
        notify("Debugger %sset to print data from %s %s%s%d%s.",
          debugger_active_tc == tc ? "was already " : "",
          tc == mtc ? "the" : "PTC",
          tc->comp_name != NULL ? tc->comp_name : "",
          tc->comp_name != NULL ? "(" : "", tc->comp_ref,
          tc->comp_name != NULL ? ")" : "");
        debugger_active_tc = tc;
      }
      break; }
    case D_PRINT_SETTINGS:
    case D_PRINT_CALL_STACK:
    case D_SET_STACK_LEVEL:
    case D_LIST_VARIABLES:
    case D_PRINT_VARIABLE:
    case D_OVERWRITE_VARIABLE:
    case D_PRINT_FUNCTION_CALLS:
    case D_STEP_OVER:
    case D_STEP_INTO:
    case D_STEP_OUT:    
      // it's a printing or stepping command, needs to be sent to the active component
      if (debugger_active_tc == NULL || !is_tc_debuggable(debugger_active_tc)) {
        // set the MTC as active in the beginning or if the active PTC has
        // finished executing
        debugger_active_tc = mtc;
      }
      send_debug_command(debugger_active_tc->tc_fd, commandID, arguments);
      break;
    case D_SWITCH:
    case D_SET_OUTPUT:
    case D_SET_AUTOMATIC_BREAKPOINT:
    case D_SET_GLOBAL_BATCH_FILE:
    case D_SET_BREAKPOINT:
    case D_REMOVE_BREAKPOINT:
    case D_FUNCTION_CALL_CONFIG:
      // it's a global setting, store it, the next MSG_DEBUG_RETURN_VALUE message
      // might need it
      last_debug_command.command = commandID;
      Free(last_debug_command.arguments);
      last_debug_command.arguments = mcopystr(arguments);
      // needs to be sent to all HCs and TCs
      send_debug_command(mtc->tc_fd, commandID, arguments);
      for (component i = FIRST_PTC_COMPREF; i < n_components; ++i) {
        component_struct* comp = components[i];
        if (comp != NULL && comp->tc_state != PTC_STALE && comp->tc_state != TC_EXITED) {
          send_debug_command(comp->tc_fd, commandID, arguments);
        }
      }
      for (int i = 0; i < n_hosts; i++) {
        host_struct* host = hosts[i];
        if (host->hc_state != HC_DOWN) {
          send_debug_command(host->hc_fd, commandID, arguments);
        }
      }
      break;
    case D_RUN_TO_CURSOR:
    case D_HALT:
    case D_CONTINUE:
    case D_EXIT:
      // a 'run to' command or a command related to the
      // halted state, needs to be sent to all HCs and TCs
      send_debug_command(mtc->tc_fd, commandID, arguments);
      for (component i = FIRST_PTC_COMPREF; i < n_components; ++i) {
        component_struct* comp = components[i];
        // only send it to the PTC if it is actually running something
        if (comp != NULL && is_tc_debuggable(comp)) {
          send_debug_command(comp->tc_fd, commandID, arguments);
        }
      }
      for (int i = 0; i < n_hosts; i++) {
        host_struct* host = hosts[i];
        if (host->hc_state != HC_DOWN) {
          send_debug_command(host->hc_fd, commandID, arguments);
        }
      }
      break;
    default:
      break;
    }
  }
  else {
    notify("Cannot execute debug commands before the MTC is created.");
  }
  unlock();
}

mc_state_enum MainController::get_state()
{
  lock();
  mc_state_enum ret_val = mc_state;
  unlock();
  return ret_val;
}

boolean MainController::get_stop_after_testcase()
{
  lock();
  boolean ret_val = stop_after_tc;
  unlock();
  return ret_val;
}

int MainController::get_nof_hosts()
{
  lock();
  int ret_val = n_hosts;
  unlock();
  return ret_val;
}

host_struct *MainController::get_host_data(int host_index)
{
  lock();
  if (host_index >= 0 && host_index < n_hosts) return hosts[host_index];
  else return NULL;
}

component_struct *MainController::get_component_data(int component_reference)
{
  lock();
  return lookup_component(component_reference);
}

void MainController::release_data()
{
  unlock();
}

const char *MainController::get_mc_state_name(mc_state_enum state)
{
  switch (state) {
  case MC_INACTIVE:
    return "inactive";
  case MC_LISTENING:
    return "listening";
  case MC_LISTENING_CONFIGURED:
    return "listening (configured)";
  case MC_HC_CONNECTED:
    return "HC connected";
  case MC_CONFIGURING:
    return "configuring...";
  case MC_ACTIVE:
    return "active";
  case MC_CREATING_MTC:
    return "creating MTC...";
  case MC_TERMINATING_MTC:
    return "terminating MTC...";
  case MC_READY:
    return "ready";
  case MC_EXECUTING_CONTROL:
    return "executing control part";
  case MC_EXECUTING_TESTCASE:
    return "executing testcase";
  case MC_TERMINATING_TESTCASE:
    return "terminating testcase...";
  case MC_PAUSED:
    return "paused after testcase";
  case MC_SHUTDOWN:
    return "shutting down...";
  default:
    return "unknown/transient";
  }
}

const char *MainController::get_hc_state_name(hc_state_enum state)
{
  switch (state) {
  case HC_IDLE:
    return "not configured";
  case HC_CONFIGURING:
  case HC_CONFIGURING_OVERLOADED:
    return "being configured";
  case HC_ACTIVE:
    return "ready";
  case HC_OVERLOADED:
    return "overloaded";
  case HC_DOWN:
    return "down";
  default:
    return "unknown/transient";
  }
}

const char *MainController::get_tc_state_name(tc_state_enum state)
{
  switch (state) {
  case TC_INITIAL:
    return "being created";
  case TC_IDLE:
    return "inactive - waiting for start";
  case TC_CREATE:
    return "executing create operation";
  case TC_START:
    return "executing component start operation";
  case TC_STOP:
  case MTC_ALL_COMPONENT_STOP:
    return "executing component stop operation";
  case TC_KILL:
  case MTC_ALL_COMPONENT_KILL:
    return "executing kill operation";
  case TC_CONNECT:
    return "executing connect operation";
  case TC_DISCONNECT:
    return "executing disconnect operation";
  case TC_MAP:
    return "executing map operation";
  case TC_UNMAP:
    return "executing unmap operation";
  case TC_STOPPING:
    return "being stopped";
  case TC_EXITING:
    return "terminated";
  case TC_EXITED:
    return "exited";
  case MTC_CONTROLPART:
    return "executing control part";
  case MTC_TESTCASE:
    return "executing testcase";
  case MTC_TERMINATING_TESTCASE:
    return "terminating testcase";
  case MTC_PAUSED:
    return "paused";
  case PTC_FUNCTION:
    return "executing function";
  case PTC_STARTING:
    return "being started";
  case PTC_STOPPED:
    return "stopped - waiting for re-start";
  case PTC_KILLING:
  case PTC_STOPPING_KILLING:
    return "being killed";
  default:
    return "unknown/transient";
  }
}

const char *MainController::get_transport_name(transport_type_enum transport)
{
  switch (transport) {
  case TRANSPORT_LOCAL:
    return "LOCAL (software loop)";
  case TRANSPORT_INET_STREAM:
    return "INET_STREAM (TCP over IPv4)";
  case TRANSPORT_UNIX_STREAM:
    return "UNIX_STREAM (UNIX domain socket)";
  default:
    return "unknown";
  }
}

//----------------------------------------------------------------------------

} /* namespace mctr */

//----------------------------------------------------------------------------

// Local Variables:
// mode: C++
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
