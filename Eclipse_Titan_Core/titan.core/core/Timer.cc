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
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "Timer.hh"
#include "Float.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Snapshot.hh"
#include "Basetype.hh"

TIMER *TIMER::list_head = NULL, *TIMER::list_tail = NULL,
    *TIMER::backup_head = NULL, *TIMER::backup_tail = NULL;
boolean TIMER::control_timers_saved = FALSE;

void TIMER::add_to_list()
{
  // do nothing if already a member of list
  if (this == list_head || list_prev != NULL) return;
  if (list_head == NULL) list_head = this;
  else if (list_tail != NULL) list_tail->list_next = this;
  list_prev = list_tail;
  list_next = NULL;
  list_tail = this;
}

void TIMER::remove_from_list()
{
  if (list_prev != NULL) list_prev->list_next = list_next;
  else if (list_head == this) list_head = list_next;
  if (list_next != NULL) list_next->list_prev = list_prev;
  else if (list_tail == this) list_tail = list_prev;
  list_prev = NULL;
  list_next = NULL;
}

TIMER::TIMER(const char *par_timer_name)
{
  timer_name = par_timer_name != NULL ? par_timer_name : "<unknown>";
  has_default = FALSE;
  is_started = FALSE;
  list_prev = NULL;
  list_next = NULL;
}

TIMER::TIMER(const char *par_timer_name, double def_val)
{
  if (par_timer_name == NULL)
    TTCN_error("Internal error: Creating a timer with an invalid name.");
  timer_name = par_timer_name;
  set_default_duration(def_val);
  is_started = FALSE;
  list_prev = NULL;
  list_next = NULL;
}

TIMER::TIMER(const char *par_timer_name, const FLOAT& def_val)
{
  if (par_timer_name == NULL)
    TTCN_error("Internal error: Creating a timer with an invalid name.");
  timer_name = par_timer_name;
  def_val.must_bound("Initializing a timer duration with an unbound "
    "float value.");
  set_default_duration(def_val.float_value);
  is_started = FALSE;
  list_prev = NULL;
  list_next = NULL;
}

TIMER::~TIMER()
{
  if (is_started) remove_from_list();
}

void TIMER::set_name(const char * name)
{
  if (name == NULL) TTCN_error("Internal error: Setting an "
    "invalid name for a single element of a timer array.");
  timer_name = name;
}

void TIMER::set_default_duration(double def_val)
{
  if (def_val < 0.0) {
    TTCN_error("Setting the default duration of timer %s "
      "to a negative float value (%g).", timer_name, def_val);
  } else if (FLOAT::is_special(def_val)) {
    TTCN_error("Setting the default duration of timer %s "
      "to a non-numeric float value (%g).", timer_name, def_val);
  }
  has_default = TRUE;
  default_val = def_val;
}

void TIMER::set_default_duration(const FLOAT& def_val)
{
  if (!def_val.is_bound()) TTCN_error("Setting the default duration of "
    "timer %s to an unbound float value.", timer_name);
  set_default_duration(def_val.float_value);
}

void TIMER::start()
{
  if (!has_default)
    TTCN_error("Timer %s does not have default duration. "
      "It can only be started with a given duration.", timer_name);
  start(default_val);
}

void TIMER::start(double start_val)
{
  if (this != &testcase_timer) {
    if (start_val < 0.0)
      TTCN_error("Starting timer %s with a negative duration (%g).",
        timer_name, start_val);
    if (FLOAT::is_special(start_val)) {
      TTCN_error("Starting timer %s with "
        "a non-numeric float value (%g).", timer_name, start_val);
    }
    if (is_started) {
      TTCN_warning("Re-starting timer %s, which is already active (running "
        "or expired).", timer_name);
      remove_from_list();
    } else {
      is_started = TRUE;
    }
    TTCN_Logger::log_timer_start(timer_name, start_val);
    add_to_list();
  } else {
    if (start_val < 0.0)
      TTCN_error("Using a negative duration (%g) for the guard timer of the "
        "test case.", start_val);
    if (FLOAT::is_special(start_val)) {
      TTCN_error("Using a non-numeric float value (%g) for the"
        " guard timer of the test case.", start_val);
    }
    is_started = TRUE;
    TTCN_Logger::log_timer_guard(start_val);
  }
  t_started = TTCN_Snapshot::time_now();
  t_expires = t_started + start_val;
}

void TIMER::start(const FLOAT& start_val)
{
  if (!start_val.is_bound())
    TTCN_error("Starting timer %s with an unbound float value as duration.",
      timer_name);
  start(start_val.float_value);
}

void TIMER::stop()
{
  if (this != &testcase_timer) {
    if (is_started) {
      is_started = FALSE;
      TTCN_Logger::log_timer_stop(timer_name, t_expires - t_started);
      remove_from_list();
    } else TTCN_warning("Stopping inactive timer %s.", timer_name);
  } else is_started = FALSE;
}

double TIMER::read() const
{
  // the time is not frozen (i.e. time_now() is used)
  double ret_val;
  if (is_started) {
    double current_time = TTCN_Snapshot::time_now();
    if (current_time >= t_expires) ret_val = 0.0;
    else ret_val = current_time - t_started;
  } else {
    ret_val = 0.0;
  }
  TTCN_Logger::log_timer_read(timer_name, ret_val);
  return ret_val;
}

boolean TIMER::running(Index_Redirect*) const
  {
  // the time is not frozen (i.e. time_now() is used)
  return is_started && TTCN_Snapshot::time_now() < t_expires;
  }

alt_status TIMER::timeout(Index_Redirect*)
{
  // the time is frozen (i.e. get_alt_begin() is used)
  if (is_started) {
    if (TTCN_Snapshot::get_alt_begin() < t_expires) return ALT_MAYBE;
    is_started = FALSE;
    if (this != &testcase_timer) {
      TTCN_Logger::log_timer_timeout(timer_name, t_expires - t_started);
      remove_from_list();
    }
    return ALT_YES;
  } else {
    if (this != &testcase_timer) {
      TTCN_Logger::log_matching_timeout(timer_name);
    }
    return ALT_NO;
  }
}

void TIMER::log() const
{
  // the time is not frozen (i.e. time_now() is used)
  TTCN_Logger::log_event("timer: { name: %s, default duration: ", timer_name);
  if (has_default) TTCN_Logger::log_event("%g s", default_val);
  else TTCN_Logger::log_event_str("none");
  TTCN_Logger::log_event_str(", state: ");
  if (is_started) {
    double current_time = TTCN_Snapshot::time_now();
    if (current_time < t_expires) TTCN_Logger::log_event_str("running");
    else TTCN_Logger::log_event_str("expired");
    TTCN_Logger::log_event(", actual duration: %g s, "
      "elapsed time: %g s", t_expires - t_started,
      current_time - t_started);
  } else TTCN_Logger::log_event_str("inactive");
  TTCN_Logger::log_event_str(" }");
}

void TIMER::all_stop()
{
  while (list_head != NULL) list_head->stop(); // also removes from list
}

boolean TIMER::any_running()
{
  for (TIMER *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    if (list_iter->running()) return TRUE;
  }
  return FALSE;
}

alt_status TIMER::any_timeout()
{
  alt_status ret_val = ALT_NO;
  for (TIMER *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    switch (list_iter->timeout()) {
    case ALT_YES:
      TTCN_Logger::log_timer_any_timeout();
      return ALT_YES;
    case ALT_MAYBE:
      ret_val = ALT_MAYBE;
      break;
    default:
      TTCN_error("Internal error: Timer %s returned unexpected status "
        "code while evaluating `any timer.timeout'.",
        list_iter->timer_name);
    }
  }
  if (ret_val == ALT_NO) {
    TTCN_Logger::log_matching_timeout(NULL);
  }
  return ret_val;
}

boolean TIMER::get_min_expiration(double& min_val)
{
  boolean min_flag = FALSE;
  double alt_begin = TTCN_Snapshot::get_alt_begin();
  if (testcase_timer.is_started && testcase_timer.t_expires >= alt_begin) {
    min_val = testcase_timer.t_expires;
    min_flag = TRUE;
  }
  for (TIMER *list_iter = list_head; list_iter != NULL;
    list_iter = list_iter->list_next) {
    // timers that are expired before the previous snapshot are ignored
    if (list_iter->t_expires < alt_begin) continue;
    else if (!min_flag || list_iter->t_expires < min_val) {
      min_val = list_iter->t_expires;
      min_flag = TRUE;
    }
  }
  return min_flag;
}

void TIMER::save_control_timers()
{
  if (control_timers_saved)
    TTCN_error("Internal error: Control part timers are already saved.");
  // put the list of control part timers into the backup
  backup_head = list_head;
  list_head = NULL;
  backup_tail = list_tail;
  list_tail = NULL;
  control_timers_saved = TRUE;
}

void TIMER::restore_control_timers()
{
  if (!control_timers_saved)
    TTCN_error("Internal error: Control part timers are not saved.");
  if (list_head != NULL)
    TTCN_error("Internal error: There are active timers. "
      "Control part timers cannot be restored.");
  // restore the list of control part timers from the backup
  list_head = backup_head;
  backup_head = NULL;
  list_tail = backup_tail;
  backup_tail = NULL;
  control_timers_saved = FALSE;
}

TIMER testcase_timer("<testcase guard timer>");
