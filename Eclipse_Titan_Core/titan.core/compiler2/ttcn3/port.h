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
 *   Cserveni, Akos
 *   Delic, Adam
 *   Kremer, Peter
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef PORT_H
#define PORT_H

#include "../datatypes.h"
#include "compiler.h"

/* data structures for port types */

typedef struct port_msg_type_tag {
  const char *name;
  const char *dispname;
  const char* name_w_no_prefix;
} port_msg_type;

typedef struct port_msg_type_list_tag {
  size_t nElements;
  port_msg_type *elements;
} port_msg_type_list;

typedef enum msg_mapping_type_t { M_SIMPLE, M_DISCARD, M_FUNCTION, M_ENCODE,
  M_DECODE } msg_mapping_type_t;

typedef enum function_prototype_t { PT_CONVERT, PT_FAST, PT_BACKTRACK,
  PT_SLIDING } function_prototype_t;

typedef struct port_msg_type_mapping_target_tag {
  const char *target_name;
  const char *target_dispname;
  size_t target_index;
  msg_mapping_type_t mapping_type;
  union {
    struct {
      const char *dispname;
      const char *name;
      function_prototype_t prototype;
    } function;
    struct {
      const char *typedescr_name;
      const char *encoding_type;
      const char *encoding_options;
      const char *errorbehavior;
    } encdec;
  } mapping;
} port_msg_type_mapping_target;

typedef struct port_msg_mapped_type_tag {
  const char *name;
  const char *dispname;
  size_t nTargets;
  port_msg_type_mapping_target *targets;
} port_msg_mapped_type;

typedef struct port_msg_mapped_type_list_tag {
  size_t nElements;
  port_msg_mapped_type *elements;
} port_msg_mapped_type_list;

typedef struct port_proc_signature_tag {
  const char *name;
  const char *dispname;
  boolean is_noblock;
  boolean has_exceptions;
  boolean has_return_value;
} port_proc_signature;

typedef struct port_proc_signature_list_tag {
  size_t nElements;
  port_proc_signature *elements;
} port_proc_signature_list;

typedef enum testport_type_t { NORMAL, INTERNAL, ADDRESS } testport_type_t;

typedef enum port_type_t { REGULAR, PROVIDER, USER } port_type_t;

typedef struct port_msg_provider {
  const char *name; // provider type's name
  size_t n_out_msg_type_names;
  const char **out_msg_type_names; // provider's out message list
} port_msg_prov;

typedef struct port_msg_provider_list {
  size_t nElements;
  port_msg_prov *elements;
} port_msg_prov_list;

typedef struct port_def_tag {
  const char *name;
  const char *dispname;
  const char *filename;
  const char *module_name;
  const char *module_dispname;
  const char *address_name;
  port_msg_type_list msg_in;         /* from PortTypeBody::in_msgs  */
  port_msg_mapped_type_list msg_out; /* from PortTypeBody::out_msgs */
  port_proc_signature_list proc_in;  /* from PortTypeBody::in_sigs  */
  port_proc_signature_list proc_out; /* from PortTypeBody::out_sigs */
  testport_type_t testport_type;
  port_type_t port_type;
  port_msg_prov_list provider_msg_outlist;
  const char **mapper_name;
  size_t n_mapper_name;
  port_msg_mapped_type_list provider_msg_in;
  boolean has_sliding;
  boolean legacy;
  char *var_decls;
  char *var_defs;
  char *mapping_func_decls;
  char *mapping_func_defs;
} port_def;

#ifdef __cplusplus
extern "C" {
#endif

void defPortClass(const port_def *pdef, output_struct *output);
void generateTestPortSkeleton(const port_def *pdef);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PORT_H */
