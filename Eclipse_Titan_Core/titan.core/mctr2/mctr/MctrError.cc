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
 *
 ******************************************************************************/
#include <stdarg.h>

#include "../../common/memory.h"
#include "../../core/Error.hh"
#include "MainController.h"

void TTCN_error(const char *fmt, ...)
{
  char *str = mcopystr("Error during encoding/decoding of a message: ");
  va_list ap;
  va_start(ap, fmt);
  str = mputprintf_va_list(str, fmt, ap);
  va_end(ap);
  mctr::MainController::error("%s", str);
  Free(str);
  throw TC_Error();
}
