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
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef TIMER_HH
#define TIMER_HH

#include <stdlib.h>

#include "Types.h"

class FLOAT;
class Index_Redirect;

/** Runtime class for TTCN-3 timers.
 *  All durations are in seconds with fractional values,
 *  all times are counted from the Unix epoch (1970).
 */
class TIMER {
  // linked list of running timers
  static TIMER *list_head, *list_tail, *backup_head, *backup_tail;
  static boolean control_timers_saved;

  const char *timer_name;
  boolean has_default;
  boolean is_started;
  double default_val; ///< default timeout duration
  double t_started; ///< time when the timer started running
  double t_expires; ///< time when the timer expires
  TIMER *list_prev, *list_next;

  void add_to_list();
  void remove_from_list();

  /// Copy constructor disabled.
  TIMER(const TIMER& other_timer);
  /// Assignment disabled.
  TIMER& operator=(const TIMER& other_timer);

public:
  /// Create a timer with no default duration.
  TIMER(const char *par_timer_name = NULL);
  /// Create a timer with a default timeout value.
  TIMER(const char *par_timer_name, double def_val);
  /// Create a timer with a default timeout value.
  /// @pre \p def_val must be bound
  TIMER(const char *par_timer_name, const FLOAT& def_val);
  ~TIMER();

  /// Change the name of the timer.
  void set_name(const char * name);

  /// Change the default duration.
  void set_default_duration(double def_val);
  /// Change the default duration.
  /// @pre \p def_val must be bound
  void set_default_duration(const FLOAT& def_val);

  /// Start the timer with its default duration.
  void start();
  /// Start the timer with the specified duration.
  void start(double start_val);
  /// Start the timer with the specified duration.
  /// @pre \p start_val must be bound
  void start(const FLOAT& start_val);
  /// Stop the timer.
  void stop();
  /// Return the number of seconds until the timer expires.
  double read() const;
  /** Is the timer running.
   *  @return \c TRUE if is_started and not yet expired, \c FALSE otherwise.
   */
  boolean running(Index_Redirect* p = NULL) const;
  /** Return the alt status.
   *  @return ALT_NO    if the timer is not started.
   *  @return ALT_MAYBE if it's started and the snapshot was taken before the expiration time
   *  @return ALT_YES   if it's started and the snapshot is past the expiration time
   *
   *  If the answer is ALT_YES, a TIMEROP_TIMEOUT message is written to the log
   *  and \p is_started becomes FALSE.
   */
  alt_status timeout(Index_Redirect* p = NULL);

  void log() const;

  /// Stop all running timers (empties the list).
  static void all_stop();
  static boolean any_running();
  static alt_status any_timeout();

  /** Get the earliest expiration time for a running timer.
   *
   *  This includes the testcase guard timer.
   *  @param[out] min_val set to the earliest expiration time.
   *  @return \c TRUE if an active timer was found, \c FALSE otherwise.
   */
  static boolean get_min_expiration(double& min_val);

  static void save_control_timers();
  static void restore_control_timers();
};

extern TIMER testcase_timer;

#endif
