/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Beres, Szabolcs
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef TYPES_H
#define TYPES_H

#ifndef __GNUC__
/** If a C compiler other than GCC is used the macro below will substitute all
 * GCC-specific non-standard attributes with an empty string. */
#ifndef __attribute__
#define __attribute__(arg)
#endif

#else

#define HAVE_GCC(maj, min) \
  ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))

#endif

#include <stddef.h>

/* Generic C types */

typedef bool boolean;
#define FALSE false
#define TRUE true

enum verdicttype { NONE = 0, PASS = 1, INCONC = 2, FAIL = 3, ERROR = 4 };
extern const char * const verdict_name[5];

enum alt_status { ALT_UNCHECKED, ALT_YES, ALT_MAYBE, ALT_NO, ALT_REPEAT,
    ALT_BREAK };

enum null_type { NULL_VALUE };

enum asn_null_type { ASN_NULL_VALUE };

struct universal_char {
  unsigned char uc_group, uc_plane, uc_row, uc_cell;

  boolean is_char() const {
    return uc_group == 0 && uc_plane == 0 && uc_row == 0 && uc_cell < 128;
  }
};

typedef int component;

/** @name Predefined component reference values.
 @{
 */
#define NULL_COMPREF		0
#define MTC_COMPREF		1
#define SYSTEM_COMPREF		2
#define FIRST_PTC_COMPREF	3
#define ANY_COMPREF		-1
#define ALL_COMPREF		-2
#define UNBOUND_COMPREF		-3
/** Pseudo-component for logging when the MTC is executing a controlpart */
#define CONTROL_COMPREF		-4

/** @} */

/** Transport types for port connections */

enum transport_type_enum {
  TRANSPORT_LOCAL = 0,
  TRANSPORT_INET_STREAM = 1,
  TRANSPORT_UNIX_STREAM = 2,
  TRANSPORT_NUM = 3
};

/** File descriptor event types */

enum fd_event_type_enum {
  FD_EVENT_RD = 1, FD_EVENT_WR = 2, FD_EVENT_ERR = 4
};

/** Structure for storing global identifiers of TTCN-3 definitions */

struct qualified_name {
  char *module_name;
  char *definition_name;
};

/** How a component is identified */
enum component_id_selector_enum {
  COMPONENT_ID_NAME,    /**< Identified by name */
  COMPONENT_ID_COMPREF, /**< Identified by component reference */
  COMPONENT_ID_ALL,     /**< All components */
  COMPONENT_ID_SYSTEM   /**< System component */
};

/** Component identifier.
 *
 *  This must be a POD because itself is a member of a union
 *  (%union in config_process.y).
 */
struct component_id_t {
  /** How the component is identified.
   *  COMPONENT_ID_NAME identifies  by name, COMPONENT_ID_COMPREF by number.
   *  Any other selector identifies the component directly.
   */
  component_id_selector_enum id_selector;
  union {
    component id_compref;
    char *id_name;
  };
};

struct namespace_t {
  const char * const ns;
  const char * const px;
};

// States defined with the setstate operation.
enum translation_port_state {
  UNSET = -1,
  TRANSLATED = 0,
  NOT_TRANSLATED = 1,
  FRAGMENTED = 2,
  PARTIALLY_TRANSLATED = 3
};

#endif
