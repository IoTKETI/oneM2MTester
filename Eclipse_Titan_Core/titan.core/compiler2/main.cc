/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Beres, Szabolcs
 *   Cserveni, Akos
 *   Czerman, Oliver
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Kremer, Peter
 *   Lovassy, Arpad
 *   Ormandi, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
/* Main program for the merged compiler */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <vector>
#include <sstream>
#if defined SOLARIS || defined SOLARIS8
# include <sys/utsname.h>
#endif

#ifdef USAGE_STATS
#include "../common/usage_stats.hh"
#include <signal.h>
#endif

#include "../common/dbgnew.hh"
#include "../common/path.h"
#include "../common/version_internal.h"
#include "../common/userinfo.h"
#include "datatypes.h"
#include "main.hh"

#include "asn1/asn1_preparser.h"
#include "asn1/asn1.hh"
#include "ttcn3/ttcn3_preparser.h"
#include "ttcn3/compiler.h"

#include "AST.hh"
#include "asn1/AST_asn1.hh"
#include "ttcn3/AST_ttcn3.hh"

#include "CodeGenHelper.hh"
#include "Stopwatch.hh"

#include "ttcn3/Ttcn2Json.hh"

#include "ttcn3/profiler.h"

#include "../common/openssl_version.h"

#ifdef LICENSE
#include "../common/license.h"
#endif

using namespace Common;

const char *output_dir = NULL;
const char *tcov_file_name = NULL;
static const char *profiler_file_name = NULL;
static const char *file_list_file_name = NULL;
tcov_file_list *tcov_files = NULL;
expstring_t effective_module_lines = NULL;
expstring_t effective_module_functions = NULL;

size_t nof_top_level_pdus = 0;
const char **top_level_pdu = NULL;

boolean generate_skeleton = FALSE, force_overwrite = FALSE,
  include_line_info = FALSE, include_location_info = FALSE,
  duplicate_underscores = FALSE, parse_only = FALSE,
  semantic_check_only = FALSE, output_only_linenum = FALSE,
  default_as_optional = FALSE, enable_set_bound_out_param = FALSE,
  use_runtime_2 = FALSE, gcc_compat = FALSE, asn1_xer = FALSE,
  check_subtype = TRUE, suppress_context = FALSE, display_up_to_date = FALSE,
  implicit_json_encoding = FALSE, json_refs_for_all_types = TRUE,
  force_gen_seof = FALSE, omit_in_value_list = FALSE,
  warnings_for_bad_variants = FALSE, debugger_active = FALSE,
  legacy_unbound_union_fields = FALSE, split_to_slices = FALSE,
  legacy_untagged_union;

// Default code splitting mode is set to 'no splitting'.
CodeGenHelper::split_type code_splitting_mode = CodeGenHelper::SPLIT_NONE;

#if defined SOLARIS || defined SOLARIS8
/** Automatic detection of Solaris version based on uname() system call.
 * Distinguishing is needed because some socket functions use socklen_t
 * (which is an alias for unsigned int) as length arguments on Solaris 8.
 * On Solaris 2.6 the argument type is simply int and no socklen_t or other
 * alias exists.
 * Note: It was discovered later that Solaris 7 (which is used rarely within
 * Ericsson) already uses socklen_t thus the SOLARIS8 platform identifier is a
 * bit misleading. */
static const char *get_platform_string(void)
{
    struct utsname name;
    int major, minor;
    if (uname(&name) < 0) {
        WARNING("System call uname() failed: %s", strerror(errno));
        errno = 0;
        return "SOLARIS";
    }
    if (sscanf(name.release, "%d.%d", &major, &minor) == 2 && major == 5) {
        if (minor <= 6) return "SOLARIS";
        else return "SOLARIS8";
    } else {
        ERROR("Invalid OS release: %s", name.release);
        return "SOLARIS";
    }
}
#elif defined LINUX
#define get_platform_string() "LINUX"
#elif defined FREEBSD
#define get_platform_string() "FREEBSD"
#elif defined WIN32
#define get_platform_string() "WIN32"
#elif defined INTERIX
#define get_platform_string() "INTERIX"
#else
#error Platform was not set.
#endif


const char *expected_platform = get_platform_string();

/// "The" AST.
Modules *modules = NULL;

// Features can be disabled in the license or by commandline switches
static bool raw_disabled = false, ber_disabled = false, per_disabled = false,
  text_disabled = false, xer_disabled = false, json_disabled = false;
static bool attribute_validation_disabled = FALSE;
#ifdef LICENSE
static bool has_raw_feature = false, has_ber_feature = false,
  has_per_feature = false, has_text_feature = false, has_xer_feature = false;
#endif

boolean enable_raw()
{
  if (raw_disabled) return FALSE;
#ifdef LICENSE
  if (!has_raw_feature) {
    WARNING("The license key does not allow the generation of "
            "RAW encoder/decoder functions.");
    raw_disabled = true;
    return FALSE;
  }
#endif
  return TRUE;
}

boolean enable_ber()
{
  if (ber_disabled) return FALSE;
#ifdef LICENSE
  if (!has_ber_feature) {
    WARNING("The license key does not allow the generation of "
            "BER encoder/decoder functions.");
    ber_disabled = true;
    return FALSE;
  }
#endif
  return TRUE;
}

boolean enable_per()
{
  if (per_disabled) return FALSE;
#ifdef LICENSE
  if (!has_per_feature) {
    WARNING("The license key does not allow the generation of "
            "PER encoder/decoder functions.");
    per_disabled = true;
    return FALSE;
  }
#endif
  return TRUE;
}

boolean enable_text()
{
  if (text_disabled) return FALSE;
#ifdef LICENSE
  if (!has_text_feature) {
    WARNING("The license key does not allow the generation of "
            "TEXT encoder/decoder functions.");
    text_disabled = true;
    return FALSE;
  }
#endif
  return TRUE;
}

boolean enable_xer()
{
  if (xer_disabled) return FALSE;
#ifdef LICENSE
  if (!has_xer_feature) {
    WARNING("The license key does not allow the generation of "
            "XER encoder/decoder functions.");
    xer_disabled = true;
    return FALSE;
  }
#endif
  return TRUE;
}

boolean enable_json()
{
  return !json_disabled;
}

boolean disable_attribute_validation()
{
  if (attribute_validation_disabled) return TRUE;

  return FALSE;
}

char *canonize_input_file(const char *path_name)
{
  switch (get_path_status(path_name)) {
  case PS_NONEXISTENT:
    ERROR("Input file `%s' does not exist.", path_name);
    return NULL;
  case PS_DIRECTORY:
    ERROR("Argument `%s' is a directory.", path_name);
    return NULL;
  default:
    break;
  }
  char *dir_name = get_dir_from_path(path_name);
  char *abs_dir = get_absolute_dir(dir_name, NULL, true);
  Free(dir_name);
  char *file_name = get_file_from_path(path_name);
  char *ret_val = compose_path_name(abs_dir, file_name);
  Free(abs_dir);
  Free(file_name);
  return ret_val;
}

struct module_struct {
  const char *file_name;
  char *absolute_path;
  Module::moduletype_t module_type;
  bool need_codegen; /**< Code is generated for a module if
  - the module appears on the command line after the dash, or
  - there is no dash (code is generated for all modules) */
};

static void add_module(size_t& n_modules, module_struct*& module_list,
  const char *file_name, Module::moduletype_t module_type)
{
  char *absolute_path = canonize_input_file(file_name);
  if (absolute_path == NULL) return;
  for (size_t i = 0; i < n_modules; i++) {
    const module_struct *module = module_list + i;
    if (module->module_type == module_type &&
	!strcmp(module->absolute_path, absolute_path)) {
      ERROR("Input file `%s' was given more than once.", file_name);
      Free(absolute_path);
      return;
    }
  }
  module_list = (module_struct*)
    Realloc(module_list, (n_modules + 1) * sizeof(module_struct));
  module_struct *module = module_list + n_modules;
  module->file_name = file_name;
  module->absolute_path = absolute_path;
  module->module_type = module_type;
  module->need_codegen = false;
  n_modules++;
}

const char *get_tcov_file_name(const char *file_name)
{
  tcov_file_list *tcov_file = tcov_files;
  expstring_t file_name_pp = mputprintf(NULL, "%spp", file_name);
  while (tcov_file != NULL) {
	// This name can be a `.ttcnpp' too.
	const char *real_file_name = static_cast<const char *>(tcov_file->file_name);
	if (!strcmp(file_name, real_file_name) ||
		!strcmp(static_cast<const char *>(file_name_pp), real_file_name)) {
	  Free(file_name_pp);
	  return real_file_name;
	}
	tcov_file = tcov_file->next;
  }
  Free(file_name_pp);
  return NULL;
}

boolean in_tcov_files(const char *file_name)
{
  return get_tcov_file_name(file_name) ? TRUE : FALSE;
}

static bool check_file_list(const char *file_name, module_struct *module_list,
                            size_t n_modules, tcov_file_list *&file_list_head)
{
  FILE *fp = fopen(file_name, "r");
  if (fp == NULL) {
	ERROR("File `%s' does not exist.", file_name);
    return false;
  }
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
  char line[PATH_MAX];
  bool unlisted_files = false;
  while (fgets(line, sizeof(line), fp) != NULL) {
	// Remove trailing '\n'.
	size_t line_len = strlen(line) - 1;
	if (line[line_len] == '\n')
	  line[line_len] = 0;
	// Handle `.ttcnpp' files in input file.
	if (line_len > 1) {
	  char last = line[line_len - 1];
	  char before_last = line[line_len - 2];
      if (last == 'p' && before_last == 'p')
    	line_len -= 2;
	}
	if (line_len < 1)
      continue;
    size_t i = 0;
    for (; i < n_modules; ++i) {
      const module_struct *module = module_list + i;
      if (!strncmp(module->file_name, line, line_len)) {
        tcov_file_list *next_file = (tcov_file_list*)Malloc(sizeof(tcov_file_list));
        next_file->next = file_list_head;
        // We'll need the `.ttcnpp' file name.
        next_file->file_name = mcopystr(line);
        file_list_head = next_file;
        break;
      }
    }
    if (i == n_modules) {
      ERROR("File `%s' was listed in `%s', but not in the command line.",
    		line, file_name);
      unlisted_files = true;
    }
  }
  fclose(fp);
  if (unlisted_files) {
	while (file_list_head != NULL) {
	  tcov_file_list *next_file = file_list_head->next;
	  Free(file_list_head->file_name);
	  Free(file_list_head);
	  file_list_head = next_file;
	}
	file_list_head = NULL;
  }
  return !unlisted_files;
}

static boolean is_valid_asn1_filename(const char* file_name)
{
  // only check the actual file name, not the whole path
  const char* file_name_start = strrchr(file_name, '/');
  if (0 == strchr(file_name_start != NULL ? file_name_start : file_name, '-' )) {
    return TRUE;
  }
  return FALSE;
}

static void usage()
{
  fprintf(stderr, "\n"
    "usage: %s [-abcdEfgijlLMnNOpqrRsStuwxXyY] [-J file] [-K file] [-z file] [-V verb_level]\n"
    "	[-o dir] [-U none|type|'number'] [-P modulename.top_level_pdu_name] [-Q number] ...\n"
    "	[-T] module.ttcn [-A] module.asn ...\n"
    "	or  %s -v\n"
    "	or  %s --ttcn2json [-jf] ... [-T] module.ttcn [-A] module.asn ... [- schema.json]\n"
    "\n"
    "OPTIONS:\n"
    "	-a:		force XER in ASN.1 files\n"
    "	-b:		disable BER encoder/decoder functions\n"
    "	-B:		allow selected union field to be unbound (legacy behavior)\n"
    "	-c:		write out checksums in case of error\n"
    "	-d:		treat default fields as omit\n"
    "	-E:		display only warnings for unrecognized encoding variants\n"
    "	-f:		force overwriting of output files\n"
    "	-g:		emulate GCC error/warning message format\n"
    "	-i:		use only line numbers in error/warning messages\n"
    "	-j:		disable JSON encoder/decoder functions\n"
    "	-J file:	read input files from file\n"
    "	-K file:	enable selective code coverage\n"
    "	-l:		include source line info in C++ code\n"
    "	-L:		add source line info for logging\n"
    "	-M:		allow 'omit' in template value lists (legacy behavior)\n"
    "	-n:		activate debugger (generates extra code for debugging)\n"
    "	-N:		ignore UNTAGGED encoding instruction on top level unions (legacy behaviour)\n"
    "	-o dir:		output files will be placed into dir\n"
    "	-p:		parse only (no semantic check or code generation)\n"
    "	-P pduname:	define top-level pdu\n"
    "	-q:		suppress all messages (quiet mode)\n"
    "	-Qn:		quit after n errors\n"
    "	-r:		disable RAW encoder/decoder functions\n"
    "	-R:		use function test runtime (TITAN_RUNTIME_2)\n"
    "	-s:		parse and semantic check only (no code generation)\n"
    "	-S:		suppress context information\n"
    "	-t:		generate Test Port skeleton\n"
    "	-u:		duplicate underscores in file names\n"
    "	-U none|type|'number':	select code splitting mode for the generated C++ code\n"
    "	-V verb_level:	set verbosity level bitmask (decimal)\n"
    "	-w:		suppress warnings\n"
    "	-x:		disable TEXT encoder/decoder functions\n"
    "	-X:		disable XER encoder/decoder functions\n"
    "	-y:		disable subtype checking\n"
    "	-Y:		enforce legacy behaviour for \"out\" function parameters (see refguide)\n"
    "	-z file:	enable profiling and code coverage for the TTCN-3 files in the argument\n"
    "	-T file:	force interpretation of file as TTCN-3 module\n"
    "	-A file:	force interpretation of file as ASN.1 module\n"
    "	-v:		show version\n"
    "	--ttcn2json:	generate JSON schema from input modules\n"
    "JSON schema generator options:\n"
    "	-j:		only include types with JSON encoding\n"
    "	-f:		only generate references to types with JSON encoding/decoding functions\n", argv0, argv0, argv0);
}

#define SET_FLAG(x) if (x##flag) {\
    ERROR("Flag -" #x " was specified more than once.");\
    errflag = true;\
    } else x##flag = true


extern int ttcn3_debug;
extern int asn1_yydebug;
extern int pattern_yydebug;
extern int pattern_unidebug;
extern int rawAST_debug;
extern int coding_attrib_debug;

int main(int argc, char *argv[])
{
  argv0 = argv[0];
#ifndef NDEBUG
  asn1_yydebug    = !! getenv("DEBUG_ASN1");
  ttcn3_debug     = !! getenv("DEBUG_TTCN");
  pattern_unidebug= pattern_yydebug = !! getenv("DEBUG_PATTERN");
  rawAST_debug    = !! getenv("DEBUG_RAW");
  coding_attrib_debug = !!getenv("DEBUG_ATRIB") || getenv("DEBUG_ATTRIB");
#endif

#ifdef MEMORY_DEBUG
#if defined(__CYGWIN__) || defined(INTERIX)
  //nothing to do
#else
  debug_new_counter.set_program_name(argv0);
#endif
#endif

  if (argc == 1) {
    fputs("TTCN-3 and ASN.1 Compiler for the TTCN-3 Test Executor, version "
      PRODUCT_NUMBER "\n", stderr);
    usage();
    return EXIT_FAILURE;
  }
  
  bool
    Aflag = false,  Lflag = false, Yflag = false,
    Pflag = false, Tflag = false, Vflag = false, bflag = false,
    cflag = false, fflag = false, iflag = false, lflag = false,
    oflag = false, pflag = false, qflag = false, rflag = false, sflag = false,
    tflag = false, uflag = false, vflag = false, wflag = false, xflag = false,
    dflag = false, Xflag = false, Rflag = false, gflag = false, aflag = false,
    s0flag = false, Cflag = false, yflag = false, Uflag = false, Qflag = false,
    Sflag = false, Kflag = false, jflag = false, zflag = false, Fflag = false,
    Mflag = false, Eflag = false, nflag = false, Bflag = false, errflag = false,
    print_usage = false, ttcn2json = false, Nflag = false; 

  CodeGenHelper cgh;

  bool asn1_modules_present = false;
#ifdef LICENSE
  bool ttcn3_modules_present = false;
#endif
  size_t n_modules = 0;
  module_struct *module_list = NULL;
  char* json_schema_name = NULL;
  size_t n_files_from_file = 0;
  char ** files_from_file = NULL;
  
  if (0 == strcmp(argv[1], "--ttcn2json")) {
    ttcn2json = true;
    display_up_to_date = TRUE;
    implicit_json_encoding = TRUE;
    for (int i = 2; i < argc; ++i) {
      // A dash (-) is used to separate the schema file name from the input files
      if (0 == strcmp(argv[i], "-")) {
        if (i == argc - 2) {
          json_schema_name = mcopystr(argv[i + 1]);
        } else {
          ERROR("Expected JSON schema name (1 argument) after option `--ttcn2json' and `-'");
          errflag = true;
        }
        break;
      } 
      else if (0 == strcmp(argv[i], "-A")) {
        ++i;
        if (i == argc) {
          ERROR("Option `-A' must be followed by an ASN.1 file name");
          errflag = true;
          break;
        }
        add_module(n_modules, module_list, argv[i], Module::MOD_ASN);
        asn1_modules_present = true;
      }
      else if (0 == strcmp(argv[i], "-T")) {
        ++i;
        if (i == argc) {
          ERROR("Option `-T' must be followed by a TTCN-3 file name");
          errflag = true;
          break;
        }
        add_module(n_modules, module_list, argv[i], Module::MOD_TTCN);
      }
      else if (0 == strcmp(argv[i], "-j")) {
        implicit_json_encoding = FALSE;
      }
      else if (0 == strcmp(argv[i], "-f")) {
        json_refs_for_all_types = FALSE;
      }
      else if (0 == strcmp(argv[i], "-fj") || 0 == strcmp(argv[i], "-jf")) {
        implicit_json_encoding = FALSE;
        json_refs_for_all_types = FALSE;
      }
      else if (argv[i][0] == '-') {
        ERROR("Invalid option `%s' after option `--ttcn2json'", argv[i]);
        print_usage = true;
        errflag = true;
        break;
      }
      else {
        add_module(n_modules, module_list, argv[i], Module::MOD_UNKNOWN);
      }
    }
    
    if (!errflag && 0 == n_modules) {
      ERROR("No TTCN-3 or ASN.1 modules specified after option `--ttcn2json'");
      errflag = true;
      print_usage = true;
    }
    
    if (!errflag && NULL == json_schema_name) {
      // Create the schema name using the first TTCN-3 or ASN.1 file's name
      const module_struct& first = module_list[0];
      if (0 == strncmp(first.file_name + strlen(first.file_name) - 4, ".asn", 4)) {
        json_schema_name = mcopystrn(first.file_name, strlen(first.file_name) - 4);
        json_schema_name = mputstrn(json_schema_name, ".json", 5);
      }
      else if (0 == strncmp(first.file_name + strlen(first.file_name) - 5, ".ttcn", 5)) {
        json_schema_name = mcopystrn(first.file_name, strlen(first.file_name) - 5);
        json_schema_name = mputstrn(json_schema_name, ".json", 5);
      }
      else {
        json_schema_name = mprintf("%s.json", first.file_name);
      }
    }
  }

  if (!ttcn2json) {
    for ( ; ; ) {
      int c = getopt(argc, argv, "aA:bBcC:dEfFgijJ:K:lLMnNo:pP:qQ:rRsStT:uU:vV:wxXyYz:0-");
      if (c == -1) break;
      switch (c) {
      case 'a':
        SET_FLAG(a);
        asn1_xer = TRUE;
        break;
      case 'A':
        Aflag = true;
        add_module(n_modules, module_list, optarg, Module::MOD_ASN);
        asn1_modules_present = true;
        break;
      case 'C':
        SET_FLAG(C);
        expected_platform = optarg;
        break;
      case 'L':
        SET_FLAG(L);
        include_location_info = TRUE;
        break;
      case 'P':
        Pflag = true;
        nof_top_level_pdus++;
        top_level_pdu=(const char**)
          Realloc(top_level_pdu, nof_top_level_pdus*sizeof(*top_level_pdu));
        top_level_pdu[nof_top_level_pdus-1] = optarg;
        break;
      case 'T':
        Tflag = true;
        add_module(n_modules, module_list, optarg, Module::MOD_TTCN);
  #ifdef LICENSE
        ttcn3_modules_present = true;
  #endif
        break;
      case 'V':
        SET_FLAG(V);
        /* set verbosity level bitmask */
        if (isdigit(optarg[0])) {
    verb_level = atoi(optarg);
    // don't bother with overflow
    errno = 0;
        } else {
    ERROR("Option `-V' requires a decimal number as argument instead of "
      "`%s'.", optarg);
    errflag = true;
        }
        break;
      case 'b':
        SET_FLAG(b);
        ber_disabled = TRUE;
        break;
      case 'c':
        SET_FLAG(c);
        break;
      case 'd':
        SET_FLAG(d);
        default_as_optional = TRUE;
        break;
      case 'f':
        SET_FLAG(f);
        force_overwrite = TRUE;
        break;
      case 'g':
        SET_FLAG(g);
        gcc_compat = TRUE;
        break;
      case 'i':
        SET_FLAG(i);
        output_only_linenum = TRUE;
        break;
      case 'J':
        file_list_file_name = optarg;
        break;
      case 'K':
        SET_FLAG(K);
        tcov_file_name = optarg;
        break;
      case 'l':
        SET_FLAG(l);
        include_line_info = TRUE;
        break;
      case 'o':
        SET_FLAG(o);
        output_dir = optarg;
        break;
      case 'Y':
        SET_FLAG(Y);
        enable_set_bound_out_param = TRUE;
        break;
      case 'p':
        SET_FLAG(p);
        parse_only = TRUE;
        break;
      case 'q':
        SET_FLAG(q);
        /* quiet; suppress all message */
        verb_level = 0;
        break;
      case 'r':
        SET_FLAG(r);
        raw_disabled = TRUE;
        break;
      case 'R':
        SET_FLAG(R);
        use_runtime_2 = TRUE;
        break;
      case 's':
        SET_FLAG(s);
        semantic_check_only = TRUE;
        break;
      case 'S':
        SET_FLAG(S);
        suppress_context = TRUE;
        break;
      case '0':
        SET_FLAG(s0);
        attribute_validation_disabled = TRUE;
        break;
      case 't':
        SET_FLAG(t);
        generate_skeleton = TRUE;
        break;
      case 'u':
        SET_FLAG(u);
        duplicate_underscores = TRUE;
        break;
      case 'U':
        SET_FLAG(U);
        if (!cgh.set_split_mode(optarg)) {
          ERROR("Wrong code splitting option: '%s'. Valid values are: 'none', "
            "'type', or a positive number.", optarg);
          errflag = true;
        }
          break;
      case 'v':
        SET_FLAG(v);
        break;
      case 'w':
        SET_FLAG(w);
        /* suppress warnings and "not supported" messages */
        verb_level &= ~(1|2);
        break;
      case 'x':
        SET_FLAG(x);
        text_disabled = TRUE;
        break;
      case 'X':
        SET_FLAG(X);
        xer_disabled = TRUE;
        break;
      case 'j':
        SET_FLAG(j);
        json_disabled = TRUE;
        break;
      case 'y':
        SET_FLAG(y);
        check_subtype = FALSE;
        break;
      case 'z':
        SET_FLAG(z);
        profiler_file_name = optarg;
        break;
      case 'F':
        SET_FLAG(F);
        force_gen_seof = TRUE;
        break;
      case 'M':
        SET_FLAG(M);
        omit_in_value_list = TRUE;
        break;
      case 'E':
        SET_FLAG(E);
        warnings_for_bad_variants = TRUE;
        break;
      case 'n':
        SET_FLAG(n);
        debugger_active = TRUE;
        break;
      case 'N':
        SET_FLAG(N);
        legacy_untagged_union = TRUE;
        break;
      case 'B':
        SET_FLAG(B);
        legacy_unbound_union_fields = TRUE;
        break;

      case 'Q': {
        long max_errs;
        SET_FLAG(Q);
        printf("Q: %s\n", optarg);

        errno = 0;
        max_errs = strtol(optarg, (char**)NULL, 10);
        if (errno != 0
          || (long)(int)max_errs != max_errs) { // does not fit into int
          ERROR("Invalid value %s: %s", optarg, strerror(errno));
          errflag = true;
        }
        else if (max_errs < 0) {
          ERROR("Negative value %s not allowed", optarg);
          errflag = true;
        }
        else { // all good
          if (max_errs == 0) max_errs = 1;
        }

        Error_Context::set_max_errors(max_errs);
        break; }

      case '-':
        if (!strcmp(argv[optind], "--ttcn2json")) {
          ERROR("Option `--ttcn2json' is only allowed as the first option");
        } else {
          ERROR("Invalid option: `%s'", argv[optind]);
        }
        // no break

      default:
        errflag = true;
        print_usage = true;
        break;
      }
    }
    
    /* Checking incompatible options */
    if (vflag) {
      if (Aflag || Lflag || Pflag || Tflag || Vflag || Yflag ||
        bflag || fflag || iflag || lflag || oflag || pflag || qflag ||
        rflag || sflag || tflag || uflag || wflag || xflag || Xflag || Rflag ||
        Uflag || yflag || Kflag || jflag || zflag || Fflag || Mflag || Eflag ||
        nflag || Bflag) {
        errflag = true;
        print_usage = true;
      }
    } else {
      if (pflag) {
        if (sflag) {
    ERROR("Options `-p' and `-s' are incompatible with each other.");
    // totally confusing: exit immediately
    errflag = true;
        }
      }
      if (Kflag && !Lflag) {
        ERROR("Source line information `-L' is necessary for code coverage `-K'.");
        errflag = true;
      }
      if (zflag && !Lflag) {
        ERROR("Source line information `-L' is necessary for profiling `-z'.");
        errflag = true;
      }
      if (nflag && !Lflag) {
        ERROR("Source line information `-L' is necessary for debugging `-n'.");
        errflag = true;
      }
      if (iflag && gflag) {
        WARNING("Option `-g' overrides `-i'.");
        iflag = false; // -g gives more information
      }
      if (oflag && get_path_status(output_dir) != PS_DIRECTORY) {
        ERROR("The argument of -o switch (`%s') must be a directory.",
      output_dir);
        errflag = true;
      }
      
      if (file_list_file_name != NULL) {
        FILE *fp = fopen(file_list_file_name, "r");
        if (fp != NULL) {
          char buff[1024];
          // We store the -A and -T here too
          while (fscanf(fp, "%s", buff) == 1) {
            n_files_from_file++;
            files_from_file = (char**)
            Realloc(files_from_file, n_files_from_file * sizeof(*files_from_file));
            files_from_file[n_files_from_file - 1] = mcopystr(buff);
          }
          fclose(fp);
        } else {
          ERROR("Cannot open file `%s' for reading: %s", file_list_file_name,
          strerror(errno));
          errno = 0;
          errflag = true;
        }
        bool next_is_asn1 = false;
        bool next_is_ttcn = false;
        for (size_t i = 0; i < n_files_from_file; i++) {
          // Check if -A or -T is present and continue to the next word if yes
          if (next_is_ttcn == false && next_is_asn1 == false) {
            if (strcmp(files_from_file[i], "-A") == 0) {
              next_is_asn1 = true;
              continue;
            } else if (strcmp(files_from_file[i], "-T") == 0) {
              next_is_ttcn = true;
              continue;
            }
          }
          Module::moduletype_t module_type = Module::MOD_UNKNOWN;
          const char* file = files_from_file[i];
          if (next_is_asn1) {
            module_type = Module::MOD_ASN; 
            next_is_asn1 = false;
          } else if(next_is_ttcn) {
            module_type = Module::MOD_TTCN;
            next_is_ttcn = false;
          } else if (strlen(files_from_file[i]) > 2) {
            // The -A or -T can be given as -TMyTtcnfile.ttcn too
            if (files_from_file[i][0] == '-') {
              if (files_from_file[i][1] == 'A') {
                file = files_from_file[i] + 2;
                module_type = Module::MOD_ASN; 
              } else if (files_from_file[i][1] == 'T') {
                file = files_from_file[i] + 2;
                module_type = Module::MOD_TTCN; 
              }
            }
          }
          if (module_type == Module::MOD_TTCN) {
#ifdef LICENSE
            ttcn3_modules_present = true;
#endif
          } else if (module_type == Module::MOD_ASN) {
            asn1_modules_present = true;
          }
          add_module(n_modules, module_list, file, module_type);
        }
      }
      
      if (optind == argc && n_modules == 0 && n_files_from_file == 0) {
        ERROR("No input TTCN-3 or ASN.1 module was given.");
        errflag = true;
      }
    }
  } // if (!ttcn2json)
  
  if (errflag) {
    if (print_usage) usage();
    Free(json_schema_name);
    return EXIT_FAILURE;
  }

  if (vflag) {
    fputs("TTCN-3 and ASN.1 Compiler for the TTCN-3 Test Executor\n"
	  "Product number: " PRODUCT_NUMBER "\n"
	  "Build date: " __DATE__ " " __TIME__ "\n"
	  "Compiled with: " C_COMPILER_VERSION "\n", stderr);
	  fputs("Using ", stderr);
	  fputs(openssl_version_str(), stderr);
	  fputs("\n\n" COPYRIGHT_STRING "\n\n", stderr);
#ifdef LICENSE
    print_license_info();
    fputs("\n\n", stderr);
#endif
    return EXIT_SUCCESS;
  }

#ifdef LICENSE
  init_openssl();
  license_struct lstr;
  load_license(&lstr);
  int license_valid = verify_license(&lstr);
  free_openssl();
  if (!license_valid) {
    free_license(&lstr);
    exit(EXIT_FAILURE);
  }
#endif

  if (!ttcn2json) {
    /* the position of '-' switch in argv list */
    int dash_position = -1;

    /* Add the remaining files until switch '-' to the module_list */
    for(int i = optind; i < argc; i++) {
      if (strcmp(argv[i], "-"))
        add_module(n_modules, module_list, argv[i], Module::MOD_UNKNOWN);
      else {
        dash_position = i;
        break;
      }
    }

    if (dash_position == -1) {
      /** if '-' was not present in the command line code should be generated for
       * all modules */
      for (size_t i = 0; i < n_modules; i++) module_list[i].need_codegen = true;
    } else {
      for (int i = dash_position + 1; i < argc; i++) {
        char *absolute_path = canonize_input_file(argv[i]);
        if (absolute_path == NULL) continue;
        bool found = false;
        for (size_t j = 0; j < n_modules; j++) {
    module_struct *module = module_list + j;
    if (!strcmp(module->absolute_path, absolute_path)) {
      module->need_codegen = true;
      found = true;
      // do not stop: the same file may be present on the list twice
      // (as both ASN.1 and TTCN-3 module)
    }
        }
        Free(absolute_path);
        if (!found) {
          ERROR("File `%s' was not given before the `-' switch for selective "
                "code generation.", argv[i]);
          // go further (i.e. check all files after the `-')
        }
      }
    }
  } // if (!ttcn2json)

  {
  STOPWATCH("Determining module types");
  // check the readability of all files and
  // determine the type of unknown modules
  for (size_t i = 0; i < n_modules; i++) {
    module_struct *module = module_list + i;
    FILE *fp = fopen(module->file_name, "r");
    if (fp != NULL) {
      if (module->module_type == Module::MOD_UNKNOWN) {
        // try the ASN.1 and TTCN-3 preparsers
  // To display the module name in the error message
  char *module_name = NULL;
  boolean asn1_module = is_asn1_module(module->file_name, fp, &module_name);
  boolean ttcn3_module = is_ttcn3_module(module->file_name, fp, NULL);
  if (asn1_module) {
    if (!is_valid_asn1_filename (module->file_name)) {
      ERROR("The file name `%s' (without suffix) shall be identical to the module name `%s'.\n"
            "If the name of the ASN.1 module contains a hyphen, the corresponding "
            "file name shall contain an underscore character instead.", module->file_name, module_name);
    }
    Free(module_name);
    if (ttcn3_module) {
            ERROR("File `%s' looks so strange that it can contain both an "
        "ASN.1 and a TTCN-3 module. Use the command-line switch `-A' or "
        "`-T' to set its type.", module->file_name);
    } else {
      bool found = false;
      for (size_t j = 0; j < n_modules; j++) {
        module_struct *module2 = module_list + j;
        if (module2->module_type == Module::MOD_ASN &&
      !strcmp(module->absolute_path, module2->absolute_path)) {
     found = true;
     break;
        }
      }
      if (found) {
        ERROR("Input file `%s' was given more than once.",
    module->file_name);
      } else {
        module->module_type = Module::MOD_ASN;
        asn1_modules_present = true;
      }
    }
  } else if (ttcn3_module) {
    bool found = false;
    for (size_t j = 0; j < n_modules; j++) {
      module_struct *module2 = module_list + j;
      if (module2->module_type == Module::MOD_TTCN &&
    !strcmp(module->absolute_path, module2->absolute_path)) {
         found = true;
         break;
      }
    }
    if (found) {
      ERROR("Input file `%s' was given more than once.",
        module->file_name);
    } else {
            module->module_type = Module::MOD_TTCN;
#ifdef LICENSE
      ttcn3_modules_present = true;
#endif
    }
  } else {
    ERROR("Cannot recognize file `%s' as an ASN.1 or TTCN-3 module. "
      "Use the command-line switch `-A' or `-T' to set its type.",
      module->file_name);
  }
      }
      fclose(fp);
    } else {
      ERROR("Cannot open input file `%s' for reading: %s", module->file_name,
  strerror(errno));
      errno = 0;
      // do not invoke the real parsers on that file
      module->module_type = Module::MOD_UNKNOWN;
    }
  }
  }

#if defined(MINGW)
  if (!semantic_check_only) {
    NOTIFY("On native win32 builds code generation is disabled.");
    semantic_check_only = TRUE;
  }
#endif
#ifdef LICENSE
  /* Checking of required license features */
  if (asn1_modules_present && !check_feature(&lstr, FEATURE_ASN1)) {
    ERROR("The license key does not allow the parsing of "
          "ASN.1 modules.");
    return EXIT_FAILURE;
  }
  if (ttcn3_modules_present && !check_feature(&lstr, FEATURE_TTCN3)) {
    ERROR("The license key does not allow the parsing of "
          "TTCN-3 modules.");
    return EXIT_FAILURE;
  }
  if (!parse_only && !semantic_check_only &&
      !check_feature(&lstr, FEATURE_CODEGEN)) {
    WARNING("The license key does not allow the generation of "
              "C++ code.");
    semantic_check_only = TRUE;
  }
  if (generate_skeleton && !check_feature(&lstr, FEATURE_TPGEN)) {
    WARNING("The license key does not allow the generation of "
              "Test Port skeletons.");
    generate_skeleton = FALSE;
  }
  has_raw_feature = check_feature(&lstr, FEATURE_RAW);
  has_ber_feature = check_feature(&lstr, FEATURE_BER);
  has_per_feature = check_feature(&lstr, FEATURE_PER);
  has_text_feature = check_feature(&lstr, FEATURE_TEXT);
  has_xer_feature = check_feature(&lstr, FEATURE_XER);
  free_license(&lstr);
#endif
  if (Kflag && !check_file_list(tcov_file_name, module_list, n_modules, tcov_files)) {
	ERROR("Error while processing `%s' provided for code coverage data "
	      "generation.", tcov_file_name);
	return EXIT_FAILURE;
  }
  if (zflag) {
    tcov_file_list *file_list_head = NULL;
    if(!check_file_list(profiler_file_name, module_list, n_modules, file_list_head)) {
      ERROR("Error while processing `%s' provided for profiling and code coverage.",
        profiler_file_name);
      return EXIT_FAILURE;
    }
    init_profiler_data(file_list_head);
  }
  {
    STOPWATCH("Parsing modules");

    // asn1_yydebug=1;
    if (asn1_modules_present) asn1_init();
    modules = new Common::Modules();

    for (size_t i = 0; i < n_modules; i++) {
      const module_struct *module = module_list + i;
      switch (module->module_type) {
      case Module::MOD_ASN:
        asn1_parse_file(module->file_name, module->need_codegen);
        break;
      case Module::MOD_TTCN:
        ttcn3_parse_file(module->file_name, module->need_codegen);
        break;
      default: // MOD_UNKNOWN ?
        break;
      }
    }

    for (size_t i = 0; i < n_modules; i++) Free(module_list[i].absolute_path);
    Free(module_list);
  }

  if (!parse_only && 0 == Error_Context::get_error_count()) {
    NOTIFY("Checking modules...");
    {
      STOPWATCH("Semantic check");
      modules->chk();
    }
  }

  if (verb_level > 7) modules->dump();

  int ret_val = EXIT_SUCCESS;
  unsigned int error_count = Error_Context::get_error_count();
  if (error_count > 0) ret_val = EXIT_FAILURE;

#ifdef USAGE_STATS
  pthread_t stats_thread = 0;
  thread_data* stats_data = NULL;
#endif
  
  if (parse_only || semantic_check_only) {
    // print detailed statistics
    Error_Context::print_error_statistics();
  } else {
    if (error_count == 0) {
#ifdef USAGE_STATS
      {
        // Ignore SIGPIPE signals
        struct sigaction sig_act;
        if (sigaction(SIGPIPE, NULL, &sig_act))
          ERROR("System call sigaction() failed when getting signal "
            "handling information for %s.", "SIGINT");
        sig_act.sa_handler = SIG_IGN;
        sig_act.sa_flags = 0;
        if (sigaction(SIGPIPE, &sig_act, NULL))
          ERROR("System call sigaction() failed when disabling signal "
            "%s.", "SIGINT");
      }

      std::stringstream stream;
      stream << "compiler";
      std::set<ModuleVersion> versions = modules->getVersionsWithProductNumber();
      if (!versions.empty()) {
        stream << "&products=";
        for (std::set<ModuleVersion>::iterator it = versions.begin(); it != versions.end(); ++it) {
          if (it != versions.begin()) {
            stream << ",";
          }
          stream << it->toString();
        }
      }

      stats_data = new thread_data;
      stats_data->sndr = new HttpSender;
      stats_thread = UsageData::getInstance().sendDataThreaded(stream.str(), stats_data);
#endif
      if (ttcn2json) {
        NOTIFY("Generating JSON schema...");
        STOPWATCH("Generating JSON schema");
        // the Ttcn2Json constructor starts the process
        Ttcn::Ttcn2Json t2j(modules, json_schema_name);
      } else {
        NOTIFY("Generating code...");
        STOPWATCH("Generating code");
        modules->generate_code(cgh);
        report_nof_updated_files();
      }
    } else {
      NOTIFY("Error%s found in the input module%s. Code will not be generated.",
	error_count > 1 ? "s" : "", n_modules > 1 ? "s" : "");
      if (cflag) {
        modules->write_checksums();
      }
    }
  }

  if (Kflag) {
    while (tcov_files != NULL) {
	  tcov_file_list *next_file = tcov_files->next;
	  Free(tcov_files->file_name);
	  Free(tcov_files);
	  tcov_files = next_file;
    }
    tcov_files = NULL;
  }
  delete modules;
  Free(top_level_pdu);
  if (asn1_modules_present) asn1_free();
  Type::free_pools();
  Common::Node::chk_counter();
  Location::delete_source_file_names();
  Free(json_schema_name);
  if (zflag) {
    free_profiler_data();
  }
  for (size_t i = 0; i < n_files_from_file; i++) {
    Free(files_from_file[i]);
  }
  Free(files_from_file);

  // dbgnew.hh already does it: check_mem_leak(argv[0]);

#ifdef USAGE_STATS
  if (stats_thread != 0) {
    // cancel the usage stats thread if it hasn't finished yet
    pthread_cancel(stats_thread);
    pthread_join(stats_thread, NULL);
  }
  if (stats_data != NULL) {
    if (stats_data->sndr != NULL) {
      delete stats_data->sndr;
    }
    delete stats_data;
  }
#endif
  
  return ret_val;
}
