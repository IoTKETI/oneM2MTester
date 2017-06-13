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
 *   Szabados, Kristof
 *
 ******************************************************************************/
#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <sys/socket.h>

/* Create a unified alias for socklen_t */

#if defined(SOLARIS)
/* Has no socklen_t at all, man page claims to use size_t. However, size_t
 * 1. is slightly broken, see
 * http://portabilityblog.com/blog/archives/7-socklen_t-confusion.html
 * 2. is used _only_ if _XPG4_2 is defined.
 * Since TITAN does not define _XPG4_2, the correct socklen_type is int. */
typedef int socklen_type;
#elif defined(WIN32)
/* Cygwin has socklen_t which is a typedef to int */
typedef socklen_t socklen_type;
#else
/* Modern socklen_t is typedef unsigned int32 */
typedef socklen_t socklen_type;
#endif /* defined... */

#endif /* PLATFORM_H_ */
