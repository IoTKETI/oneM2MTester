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
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Kremer, Peter
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "datatypes.h"
#include "../common/memory.h"

typedef struct tcov_file_list
{
  struct tcov_file_list *next;
  expstring_t file_name;
} tcov_file_list;

extern const char *get_tcov_file_name(const char *file_name);
extern boolean in_tcov_files(const char *file_name);

extern const char *output_dir, **top_level_pdu, *tcov_file_name;
extern tcov_file_list *tcov_files;
extern expstring_t effective_module_lines;
extern expstring_t effective_module_functions;
extern size_t nof_top_level_pdus;
extern unsigned int nof_notupdated_files;

extern boolean generate_skeleton, force_overwrite, include_line_info,
  include_location_info, duplicate_underscores, parse_only, semantic_check_only,
  output_only_linenum, default_as_optional, use_runtime_2, gcc_compat, asn1_xer,
  check_subtype, suppress_context, enable_set_bound_out_param, display_up_to_date,
  implicit_json_encoding, json_refs_for_all_types, force_gen_seof,
  omit_in_value_list, warnings_for_bad_variants, debugger_active,
  legacy_unbound_union_fields, split_to_slices, legacy_untagged_union;

extern const char *expected_platform;

extern boolean enable_raw(void);
extern boolean enable_ber(void);
extern boolean enable_per(void);
extern boolean enable_text(void);
extern boolean enable_xer(void);
extern boolean enable_json(void);

/**
  * Checks whether the checking of encoding/decoding attributes is disabled.
  *
  * Needed for standard modules where errors are caused by missing attributes
  * (which should not be reported if doing only semantic checking).
  *
  * @return TRUE if checking the encoding/decoding attributes is disabled,
  *   FALSE otherwise.
  **/
extern boolean disable_attribute_validation(void);

extern char *canonize_input_file(const char *path_name);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
