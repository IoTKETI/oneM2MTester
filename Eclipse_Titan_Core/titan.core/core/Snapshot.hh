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
 *   Feher, Csaba
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef SNAPSHOT_HH
#define SNAPSHOT_HH

#include <sys/types.h>

#include "Types.h"

class Fd_Event_Handler;

class TTCN_Snapshot {
  static boolean else_branch_found; ///< [else] branch of \c alt was reached
  static double alt_begin; ///< The time when the snapshot was taken.

public:
  static void initialize();
  static void check_fd_setsize();
  static void terminate();
  static void else_branch_reached();
  static double time_now();
  static inline double get_alt_begin() { return alt_begin; }
  static void take_new(boolean block_execution);
  static void block_for_sending(int send_fd, Fd_Event_Handler * handler = 0);
};

#endif
