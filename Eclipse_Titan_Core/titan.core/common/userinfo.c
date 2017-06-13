/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include <time.h>
#include <unistd.h>
#ifdef MINGW
# include <windows.h>
# include <lmcons.h>
#else
# include <pwd.h>
#endif

#include "memory.h"
#include "userinfo.h"

char *get_user_info(void)
{
  char *ret_val;
  time_t t;
  const char *current_time;
#ifdef MINGW
  TCHAR user_name[UNLEN + 1], computer_name[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD buffer_size = sizeof(user_name);
  if (GetUserName(user_name, &buffer_size)) ret_val = mcopystr(user_name);
  else ret_val = mcopystr("unknown");
  ret_val = mputc(ret_val, '@');
  buffer_size = sizeof(computer_name);
  if (GetComputerName(computer_name, &buffer_size))
    ret_val = mputstr(ret_val, computer_name);
  else ret_val = mputstr(ret_val, "unknown");
#else
  struct passwd *p;
  char host_name[256];
  setpwent();
  p = getpwuid(getuid());
  if (p != NULL) {
    size_t i;
    for (i = 0; p->pw_gecos[i] != '\0'; i++) {
      if (p->pw_gecos[i] == ',') {
        /* Truncating additional info (e.g. phone number) after the full name */
        p->pw_gecos[i] = '\0';
        break;
      }
    }
    ret_val = mprintf("%s (%s", p->pw_gecos, p->pw_name);
  } else ret_val = mcopystr("Unknown User (unknown");
  endpwent();
  if (gethostname(host_name, sizeof(host_name)))
    ret_val = mputstr(ret_val, "@unknown)");
  else {
    host_name[sizeof(host_name) - 1] = '\0';
    ret_val = mputprintf(ret_val, "@%s)", host_name);
  }
#endif
  t = time(NULL);
  current_time = ctime(&t);
  ret_val = mputprintf(ret_val, " on %s", current_time);
  return ret_val;
}
