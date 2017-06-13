/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   >
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Forstner, Matyas
 *   Koppany, Csaba
 *   Kovacs, Ferenc
 *   Kremer, Peter
 *   Lovassy, Arpad
 *   Pandi, Krisztian
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalay, Akos
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#if defined SOLARIS || defined SOLARIS8
#include <sys/utsname.h>
#endif

#include "../common/memory.h"
#include "../common/path.h"
#include "../common/version_internal.h"
#include "../common/userinfo.h"
#include "ttcn3/ttcn3_preparser.h"
#include "asn1/asn1_preparser.h"

#ifdef LICENSE
#include "../common/license.h"
#endif

#include "xpather.h"

static const char *program_name = NULL;
static unsigned int error_count = 0;
static boolean suppress_warnings = FALSE;
void free_string2_list(struct string2_list* act_elem);
void free_string_list(struct string_list* act_elem);
void ERROR(const char *fmt, ...)
{
  va_list parameters;
  fprintf(stderr, "%s: error: ", program_name);
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  fprintf(stderr, "\n");
  fflush(stderr);
  error_count++;
}

void WARNING(const char *fmt, ...)
{
  va_list parameters;
  if (suppress_warnings) return;
  fprintf(stderr, "%s: warning: ", program_name);
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  putc('\n', stderr);
  fflush(stderr);
}

void NOTIFY(const char *fmt, ...)
{
  va_list parameters;
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  putc('\n', stderr);
  fflush(stderr);
}

void DEBUG(unsigned level, const char *fmt, ...)
{
  va_list parameters;
  fprintf(stderr, "%*s", 2 * level, "");
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  putc('\n', stderr);
  fflush(stderr);
}

void path_error(const char *fmt, ...)
{
  va_list ap;
  char *err_msg;
  va_start(ap, fmt);
  err_msg = mprintf_va_list(fmt, ap);
  va_end(ap);
  ERROR("%s", err_msg);
  Free(err_msg);
}


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

/** structure for describing TTCN-3 and ASN.1 modules */
struct module_struct {
  char *dir_name; /* directory of the TTCN-3 or ASN.1 file, it is NULL if the
		     file is in the current working directory */
  char *file_name; /* name of the TTCN-3 or ASN.1 file */
  char *module_name; /* name of the TTCN-3 or ASN.1 module */
  boolean is_regular; /* indicates whether the name of the source file follows
			 the default naming convention */
};

/** structure for describing test ports and other C/C++ modules */
struct user_struct {
  char *dir_name; /* directory of the C/C++ source files, it is NULL if the
		     files are in the current working directory */
  char *file_prefix; /* the common prefix of the header and source file */
  char *header_name; /* name of the C/C++ header file, which has .hh or .h or .hpp
			suffix, it is NULL if there is no header file */
  char *source_name; /* name of the C/C++ source file, which has .cc or .c or .cpp
			suffix, it is NULL if there is no source file */
  boolean has_hh_suffix; /* indicates whether the header file is present and
			    has .hh or .hpp suffix */
  boolean has_cc_suffix; /* indicates whether the source file is present and
			    has .cc or .cpp suffix */
};

/** structure for directories that pre-compiled files are taken from */
struct base_dir_struct {
  const char *dir_name; /* name of the directory */
  boolean has_modules; /* indicates whether there are TTCN-3/ASN.1 modules in
			  the directory (it is set to FALSE if dir_name
			  contains user C/C++ files only */
};

/** data structure that describes the information needed for the Makefile */
struct makefile_struct {
  char *project_name;
  size_t nTTCN3Modules;
  struct module_struct *TTCN3Modules;

  boolean preprocess;
  size_t nTTCN3PPModules;
  struct module_struct *TTCN3PPModules;

  boolean TTCN3ModulesRegular;
  boolean BaseTTCN3ModulesRegular;
  size_t nTTCN3IncludeFiles;
  char **TTCN3IncludeFiles;

  size_t nASN1Modules;
  struct module_struct *ASN1Modules;
  boolean ASN1ModulesRegular;
  boolean BaseASN1ModulesRegular;
  
  size_t nXSDModules;
  struct module_struct *XSDModules;
  // No XSDModulesRegular and BaseXSDModulesRegular: it would be always false

  size_t nUserFiles;
  struct user_struct *UserFiles;
  boolean UserHeadersRegular;
  boolean UserSourcesRegular;
  boolean BaseUserHeadersRegular;
  boolean BaseUserSourcesRegular;

  size_t nOtherFiles;
  char **OtherFiles;

  boolean central_storage;
  size_t nBaseDirs;
  struct base_dir_struct *BaseDirs;
  char *working_dir;
  boolean gnu_make;
  boolean single_mode;
  char *output_file;
  char *ets_name;
  boolean force_overwrite;
  boolean use_runtime_2;
  boolean dynamic;
  boolean gcc_dep;
  char *code_splitting_mode;
  boolean coverage;
  char *tcov_file_name;
  struct string_list* profiled_file_list; /* not owned */
  boolean library;
  boolean linkingStrategy;
  boolean hierarchical;
  struct string_list* sub_project_dirs; /* not owned */
  struct string_list* ttcn3_prep_includes; /* not owned */
  struct string_list* ttcn3_prep_defines; /* not owned */
  struct string_list* ttcn3_prep_undefines; /* not owned */
  struct string_list* prep_includes; /* not owned */
  struct string_list* prep_defines; /* not owned */
  struct string_list* prep_undefines; /* not owned */
  const char *codesplittpd;
  boolean quietly;
  boolean disablesubtypecheck;
  const char *cxxcompiler;
  const char *optlevel;
  const char *optflags;
  boolean disableber;
  boolean disableraw;
  boolean disabletext;
  boolean disablexer;
  boolean disablejson;
  boolean forcexerinasn;
  boolean defaultasomit;
  boolean gccmsgformat;
  boolean linenumbersonlymsg;
  boolean includesourceinfo;
  boolean addsourcelineinfo;
  boolean suppresswarnings;
  boolean outparamboundness;
  boolean omit_in_value_list;
  boolean warnings_for_bad_variants;
  boolean activate_debugger;
  boolean ignore_untagged_on_top_union;
  boolean disable_predef_ext_folder;
  struct string_list* solspeclibraries; /* not owned */
  struct string_list* sol8speclibraries; /* not owned */
  struct string_list* linuxspeclibraries; /* not owned */
  struct string_list* freebsdspeclibraries; /* not owned */
  struct string_list* win32speclibraries; /* not owned */
  const char *ttcn3preprocessor;
  struct string_list* linkerlibraries; /* not owned */
  struct string_list* additionalObjects; /* not owned */
  struct string_list* linkerlibsearchpath; /* not owned */
  char* generatorCommandOutput; /* not owned */
  struct string2_list* target_placement_list; /* not owned */
};

/** Initializes structure \a makefile with empty lists and default settings. */
static void init_makefile_struct(struct makefile_struct *makefile)
{
  makefile->project_name = NULL;
  makefile->nTTCN3Modules = 0;
  makefile->TTCN3Modules = NULL;
  makefile->preprocess = FALSE;
  makefile->nTTCN3PPModules = 0;
  makefile->TTCN3PPModules = NULL;
  makefile->TTCN3ModulesRegular = TRUE;
  makefile->BaseTTCN3ModulesRegular = TRUE;
  makefile->nTTCN3IncludeFiles = 0;
  makefile->TTCN3IncludeFiles = NULL;
  makefile->nASN1Modules = 0;
  makefile->ASN1Modules = NULL;
  makefile->ASN1ModulesRegular = TRUE;
  makefile->BaseASN1ModulesRegular = TRUE;
  makefile->nXSDModules = 0;
  makefile->XSDModules = NULL;
  makefile->nUserFiles = 0;
  makefile->UserFiles = NULL;
  makefile->UserHeadersRegular = TRUE;
  makefile->UserSourcesRegular = TRUE;
  makefile->BaseUserHeadersRegular = TRUE;
  makefile->BaseUserSourcesRegular = TRUE;
  makefile->nOtherFiles = 0;
  makefile->OtherFiles = NULL;
  makefile->central_storage = FALSE;
  makefile->nBaseDirs = 0;
  makefile->BaseDirs = NULL;
  makefile->working_dir = get_working_dir();
  makefile->gnu_make = FALSE;
  makefile->single_mode = FALSE;
  makefile->ets_name = NULL;
  makefile->output_file = NULL;
  makefile->force_overwrite = FALSE;
  makefile->use_runtime_2 = FALSE;
  makefile->dynamic = FALSE;
  makefile->gcc_dep = FALSE;
  makefile->code_splitting_mode = NULL;
  makefile->coverage = FALSE;
  makefile->tcov_file_name = NULL;
  makefile->profiled_file_list = NULL;
  makefile->library = FALSE;
  makefile->linkingStrategy = FALSE;
  makefile->hierarchical = FALSE;
  makefile->sub_project_dirs = NULL;
  makefile->ttcn3_prep_includes = NULL;
  makefile->prep_includes = NULL;
  makefile->prep_defines = NULL;
  makefile->outparamboundness = FALSE;
  makefile->omit_in_value_list = FALSE;
  makefile->warnings_for_bad_variants = FALSE;
  makefile->activate_debugger = FALSE;
  makefile->ignore_untagged_on_top_union = FALSE;
  makefile->disable_predef_ext_folder = FALSE;
  makefile->solspeclibraries = NULL;
  makefile->sol8speclibraries = NULL;
  makefile->linuxspeclibraries = NULL;
  makefile->freebsdspeclibraries = NULL;
  makefile->win32speclibraries = NULL;
  makefile->linkerlibraries = NULL;
  makefile->additionalObjects = NULL;
  makefile->linkerlibsearchpath = NULL;
  makefile->generatorCommandOutput = NULL;
  makefile->target_placement_list = NULL;
}

/** Deallocates all memory associated with structure \a makefile. */
static void free_makefile_struct(const struct makefile_struct *makefile)
{
  Free(makefile->project_name);
  size_t i;
  for (i = 0; i < makefile->nTTCN3Modules; i++) {
    Free(makefile->TTCN3Modules[i].dir_name);
    Free(makefile->TTCN3Modules[i].file_name);
    Free(makefile->TTCN3Modules[i].module_name);
  }
  Free(makefile->TTCN3Modules);
  for (i = 0; i < makefile->nTTCN3PPModules; i++) {
    Free(makefile->TTCN3PPModules[i].dir_name);
    Free(makefile->TTCN3PPModules[i].file_name);
    Free(makefile->TTCN3PPModules[i].module_name);
  }
  Free(makefile->TTCN3PPModules);
  for (i = 0; i < makefile->nTTCN3IncludeFiles; i++)
    Free(makefile->TTCN3IncludeFiles[i]);
  Free(makefile->TTCN3IncludeFiles);
  for (i = 0; i < makefile->nASN1Modules; i++) {
    Free(makefile->ASN1Modules[i].dir_name);
    Free(makefile->ASN1Modules[i].file_name);
    Free(makefile->ASN1Modules[i].module_name);
  }
  Free(makefile->ASN1Modules);
  for (i = 0; i < makefile->nXSDModules; i++) {
    Free(makefile->XSDModules[i].dir_name);
    Free(makefile->XSDModules[i].file_name);
    Free(makefile->XSDModules[i].module_name);
  }
  Free(makefile->XSDModules);
  for (i = 0; i < makefile->nUserFiles; i++) {
    Free(makefile->UserFiles[i].dir_name);
    Free(makefile->UserFiles[i].file_prefix);
    Free(makefile->UserFiles[i].header_name);
    Free(makefile->UserFiles[i].source_name);
  }
  Free(makefile->UserFiles);
  for (i = 0; i < makefile->nOtherFiles; i++) Free(makefile->OtherFiles[i]);
  Free(makefile->OtherFiles);
  Free(makefile->BaseDirs);
  Free(makefile->working_dir);
  Free(makefile->ets_name);
  Free(makefile->output_file);
  Free(makefile->code_splitting_mode);
  Free(makefile->tcov_file_name);
}

/** Displays the contents of structure \a makefile as debug messages. */
static void dump_makefile_struct(const struct makefile_struct *makefile,
  unsigned level)
{
  size_t i;
  DEBUG(level, "Data used for Makefile generation:");
  DEBUG(level + 1, "TTCN-3 project name: %s", makefile->project_name);
  DEBUG(level + 1, "TTCN-3 modules: (%u pcs.)", makefile->nTTCN3Modules);
  for (i = 0; i < makefile->nTTCN3Modules; i++) {
    const struct module_struct *module = makefile->TTCN3Modules + i;
    DEBUG(level + 2, "Module name: %s", module->module_name);
    if (module->dir_name != NULL)
      DEBUG(level + 3, "Directory: %s", module->dir_name);
    DEBUG(level + 3, "File name: %s", module->file_name);
    DEBUG(level + 3, "Follows the naming convention: %s",
      module->is_regular ? "yes" : "no");
  }
  DEBUG(level + 1, "TTCN-3 preprocessing: %s",
        makefile->preprocess ? "yes" : "no");
  if (makefile->preprocess) {
    DEBUG(level + 1, "TTCN-3 modules to be preprocessed: (%u pcs.)",
          makefile->nTTCN3PPModules);
    for (i = 0; i < makefile->nTTCN3PPModules; i++) {
      const struct module_struct *module = makefile->TTCN3PPModules + i;
      DEBUG(level + 2, "Module name: %s", module->module_name);
      if (module->dir_name != NULL)
        DEBUG(level + 3, "Directory: %s", module->dir_name);
      DEBUG(level + 3, "File name: %s", module->file_name);
      DEBUG(level + 3, "Follows the naming convention: %s",
        module->is_regular ? "yes" : "no");
    }
    DEBUG(level + 1, "TTCN-3 include files: (%u pcs.)",
      makefile->nTTCN3IncludeFiles);
    for (i = 0; i < makefile->nTTCN3IncludeFiles; i++)
      DEBUG(level + 2, "File name: %s", makefile->TTCN3IncludeFiles[i]);
  }
  DEBUG(level + 1, "All local TTCN-3 modules follow the naming convention: %s",
    makefile->TTCN3ModulesRegular ? "yes" : "no");
  if (makefile->central_storage) DEBUG(level + 1, "All TTCN-3 modules from other "
    "directories follow the naming convention: %s",
    makefile->BaseTTCN3ModulesRegular ? "yes" : "no");
  DEBUG(level + 1, "ASN.1 modules: (%u pcs.)", makefile->nASN1Modules);
  for (i = 0; i < makefile->nASN1Modules; i++) {
    const struct module_struct *module = makefile->ASN1Modules + i;
    DEBUG(level + 2, "Module name: %s", module->module_name);
    if (module->dir_name != NULL)
      DEBUG(level + 3, "Directory: %s", module->dir_name);
    DEBUG(level + 3, "File name: %s", module->file_name);
    DEBUG(level + 3, "Follows the naming convention: %s",
      module->is_regular ? "yes" : "no");
  }
  DEBUG(level + 1, "All local ASN.1 modules follow the naming convention: %s",
    makefile->ASN1ModulesRegular ? "yes" : "no");
  if (makefile->central_storage) DEBUG(level + 1, "All ASN.1 modules from other "
    "directories follow the naming convention: %s",
    makefile->BaseASN1ModulesRegular ? "yes" : "no");
  DEBUG(level + 1, "User C/C++ modules: (%u pcs.)", makefile->nUserFiles);
  for (i = 0; i < makefile->nUserFiles; i++) {
    const struct user_struct *user = makefile->UserFiles + i;
    DEBUG(level + 2, "File prefix: %s", user->file_prefix);
    if (user->dir_name != NULL)
      DEBUG(level + 3, "Directory: %s", user->dir_name);
    if (user->header_name != NULL) {
      DEBUG(level + 3, "Header file: %s", user->header_name);
      DEBUG(level + 3, "Header file has .hh or .hpp suffix: %s",
      user->has_hh_suffix ? "yes" : "no");
    }
    if (user->source_name != NULL) {
      DEBUG(level + 3, "Source file: %s", user->source_name);
      DEBUG(level + 3, "Source file has .cc or .cpp suffix: %s",
      user->has_cc_suffix ? "yes" : "no");
      DEBUG(level + 3, "Object file: %s.o", user->file_prefix);
    }
  }
  DEBUG(level + 1, "All local C/C++ header files follow the naming "
    "convention: %s", makefile->UserHeadersRegular ? "yes" : "no");
  DEBUG(level + 1, "All local C/C++ source files follow the naming "
    "convention: %s", makefile->UserSourcesRegular ? "yes" : "no");
  if (makefile->central_storage) {
    DEBUG(level + 1, "All C/C++ header files from other directories follow the "
      "naming convention: %s", makefile->BaseUserHeadersRegular ? "yes" : "no");
    DEBUG(level + 1, "All C/C++ source files from other directories follow the "
      "naming convention: %s", makefile->BaseUserSourcesRegular ? "yes" : "no");
  }
  DEBUG(level + 1, "Other files: (%u pcs.)", makefile->nOtherFiles);
  for (i = 0; i < makefile->nOtherFiles; i++)
    DEBUG(level + 2, "File name: %s", makefile->OtherFiles[i]);
  DEBUG(level + 1, "Use pre-compiled files from central storage: %s",
    makefile->central_storage ? "yes" : "no");
  if (makefile->central_storage) {
    DEBUG(level + 1, "Directories of pre-compiled files: (%u pcs.)",
      makefile->nBaseDirs);
    for (i = 0; i < makefile->nBaseDirs; i++) {
      const struct base_dir_struct *base_dir = makefile->BaseDirs + i;
      DEBUG(level + 2, "Directory: %s", base_dir->dir_name);
      DEBUG(level + 3, "Has TTCN-3/ASN.1 modules: %s",
      base_dir->has_modules ? "yes" : "no");
    }
  }
  DEBUG(level + 1, "Working directory: %s",
    makefile->working_dir != NULL ? makefile->working_dir : "<unknown>");
  DEBUG(level + 1, "GNU make: %s", makefile->gnu_make ? "yes" : "no");
  DEBUG(level + 1, "Execution mode: %s",
    makefile->single_mode ? "single" : "parallel");
  DEBUG(level + 1, "Name of executable: %s",
    makefile->ets_name != NULL ? makefile->ets_name : "<unknown>");
  DEBUG(level + 1, "Output file: %s",
    makefile->output_file != NULL ? makefile->output_file : "<unknown>");
  DEBUG(level + 1, "Force overwrite: %s",
    makefile->force_overwrite ? "yes" : "no");
  DEBUG(level + 1, "Use function test runtime: %s",
    makefile->use_runtime_2 ? "yes" : "no");
  DEBUG(level + 1, "Use dynamic linking: %s",
    makefile->dynamic ? "yes" : "no");
  DEBUG(level + 1, "Code splitting mode: %s",
    makefile->code_splitting_mode != NULL ?
      makefile->code_splitting_mode : "<unknown>");
  DEBUG(level + 1, "Code coverage file: %s",
    makefile->tcov_file_name != NULL ?
      makefile->tcov_file_name : "<unknown>");
  if (makefile->profiled_file_list) {
    char* lists = mcopystr(makefile->profiled_file_list->str);
    struct string_list* iter = makefile->profiled_file_list->next;
    while(iter != NULL) {
      lists = mputprintf(lists, " %s", iter->str);
      iter = iter->next;
    }
    DEBUG(level + 1, "Profiled file list(s): %s", lists);
    Free(lists);
  }
#ifdef COVERAGE_BUILD
  DEBUG(level + 1, "Enable coverage: %s", makefile->coverage ? "yes" : "no");
#endif
}

/** Returns the name of an existing file that is related to command line
 * argument \a argument. Tries the given list of suffixes. NULL pointer is
 * returned if no file was found. The returned string shall be deallocated
 * by the caller. */
static char *get_file_name_for_argument(const char *argument)
{
  static const char * const suffix_list[] = {
    "", ".ttcnpp", ".ttcnin", ".ttcn", ".ttcn3", ".3mp", ".asn", ".asn1",
    ".cc", ".c", ".cpp", ".hh", ".h",".hpp", ".cfg", ".prj", NULL
  };
  const char * const *suffix_ptr;
  for (suffix_ptr = suffix_list; *suffix_ptr != NULL; suffix_ptr++) {
    char *file_name = mputstr(mcopystr(argument), *suffix_ptr);
    if (get_path_status(file_name) == PS_FILE) return file_name;
    Free(file_name);
  }
  return NULL;
}

/** Converts \a path_name to an absolute directory using \a working_dir.
 * NULL pointer is returned if \a path_name does not contain a directory or
 * the resulting absolute directory is identical to \a working_dir.
 * The returned string shall be deallocated by the caller. */
static char *get_dir_name(const char *path_name, const char *working_dir)
{
  char *dir_name = get_dir_from_path(path_name);
  if (dir_name != NULL) {
    char *absolute_dir = get_absolute_dir(dir_name, working_dir, TRUE);
    Free(dir_name);
    if (absolute_dir == NULL || working_dir == NULL) {
      /* an error occurred */
      Free(absolute_dir);
      return NULL;
    } else if (!strcmp(absolute_dir, working_dir)) {
      /* the directory is identical to the working dir */
      Free(absolute_dir);
      return NULL;
    } else return absolute_dir;
  } else return NULL;
}

/** Returns whether \a dirname1 and \a dirname2 contain the same (canonized
 * absolute) directories. NULL pointer is handled in a special way: it is
 * identical only to itself. */
static boolean is_same_directory(const char *dirname1, const char *dirname2)
{
  if (dirname1 == NULL) {
    if (dirname2 == NULL) return TRUE;
    else return FALSE;
  } else {
    if (dirname2 == NULL) return FALSE;
    else if (strcmp(dirname1, dirname2)) return FALSE;
    else return TRUE;
  }
}

/** Returns whether the file \a filename1 in directory \a dirname1 is identical
 * to file \a filename2 in directory \a dirname2. Only the directory names can
 * be NULL. */
static boolean is_same_file(const char *dirname1, const char *filename1,
  const char *dirname2, const char *filename2)
{
  /* first examine the file names for efficiency reasons */
  if (strcmp(filename1, filename2)) return FALSE;
  else return is_same_directory(dirname1, dirname2);
}

/** Determines whether the TTCN-3 or ASN.1 module identifiers \a module1 and
 * \a module2 are the same. Characters '-' and '_' in module names are not
 * distinguished. */
static boolean is_same_module(const char *module1, const char *module2)
{
  size_t i;
  for (i = 0; ; i++) {
    switch (module1[i]) {
    case '\0':
      if (module2[i] == '\0') return TRUE;
      else return FALSE;
    case '-':
    case '_':
      if (module2[i] != '-' && module2[i] != '_') return FALSE;
      break;
    default:
      if (module1[i] != module2[i]) return FALSE;
      break;
    }
  }
  return FALSE; /* to avoid warnings */
}

/** Truncates the suffix (i.e. the last dot and the characters following it)
 * from \a file_name and returns a copy of the prefix of \a file_name.
 * If \a file_name does not have a suffix an exact copy of it is returned.
 * The returned string shall be deallocated by the caller. */
static char *cut_suffix(const char *file_name)
{
  char *ret_val;
  size_t last_dot = (size_t)-1;
  size_t i;
  for (i = 0; file_name[i] != '\0'; i++)
    if (file_name[i] == '.') last_dot = i;
  ret_val = mcopystr(file_name);
  if (last_dot != (size_t)-1) ret_val = mtruncstr(ret_val, last_dot);
  return ret_val;
}

/** Determines the name of the preprocessed file from \a file_name.
 *  It is assumed that \a file_name has ttcnpp suffix.
 *  The returned string shall be deallocated by the caller. */
static char *get_preprocessed_file_name(const char *file_name)
{
  char *ret_val = cut_suffix(file_name);
  ret_val = mputstr(ret_val, ".ttcn");
  return ret_val;
}

/** Check if any of the preprocessed ttcn file names with the preprocessed
 * (TTCN-3) suffix is equal to any other file given in the \a makefile */
static void check_preprocessed_filename_collision(
  struct makefile_struct *makefile)
{
  size_t i;
  if (makefile->nTTCN3PPModules == 0) {
    WARNING("TTCN-3 preprocessing (option `-p') is enabled, but no TTCN-3 "
      "files to be preprocessed were given for the Makefile.");
  }
  for (i = 0; i < makefile->nTTCN3PPModules; i++) {
    const struct module_struct *pp_module = makefile->TTCN3PPModules + i;
    /* name of the intermediate preprocessed file */
    char *preprocessed_name = get_preprocessed_file_name(pp_module->file_name);
    size_t j;
    for (j = 0; j < makefile->nTTCN3Modules; j++) {
      struct module_struct *module = makefile->TTCN3Modules + j;
      if (is_same_file(pp_module->dir_name, preprocessed_name,
	module->dir_name, module->file_name)) {
	if (is_same_module(pp_module->module_name, module->module_name)) {
          /* same file with the same module */
	  char *pp_pathname = compose_path_name(pp_module->dir_name,
	    pp_module->file_name);
	  char *m_pathname = compose_path_name(module->dir_name,
	    module->file_name);
	  WARNING("File `%s' containing TTCN-3 module `%s' is generated by "
	    "the preprocessor from `%s'. Removing the file from the list of "
	    "normal TTCN-3 modules.", m_pathname, module->module_name,
	    pp_pathname);
	  Free(pp_pathname);
	  Free(m_pathname);
	  Free(module->dir_name);
	  Free(module->file_name);
	  Free(module->module_name);
	  makefile->nTTCN3Modules--;
	  memmove(module, module + 1, (makefile->nTTCN3Modules - j) *
	    sizeof(*makefile->TTCN3Modules));
	  makefile->TTCN3Modules =
	    (struct module_struct*)Realloc(makefile->TTCN3Modules,
	    makefile->nTTCN3Modules * sizeof(*makefile->TTCN3Modules));
	} else {
          /* same file with different module */
	  char *pp_pathname = compose_path_name(pp_module->dir_name,
	    pp_module->file_name);
	  char *m_pathname = compose_path_name(module->dir_name,
	    module->file_name);
	  ERROR("Preprocessed intermediate file of `%s' (module `%s') clashes "
	    "with file `%s' containing TTCN-3 module `%s'.", pp_pathname,
	    pp_module->module_name, m_pathname, module->module_name);
	  Free(pp_pathname);
	  Free(m_pathname);
	}
      } else if (is_same_module(pp_module->module_name, module->module_name)) {
        /* different file with the same module */
	char *pp_pathname = compose_path_name(pp_module->dir_name,
	  pp_module->file_name);
	char *m_pathname = compose_path_name(module->dir_name,
	  module->file_name);
	ERROR("Both files `%s' and `%s' contain TTCN-3 module `%s'.",
	  pp_pathname, m_pathname, pp_module->module_name);
	Free(pp_pathname);
	Free(m_pathname);
      }
    }
    for (j = 0; j < makefile->nASN1Modules; j++) {
      struct module_struct *module = makefile->ASN1Modules + j;
      if (is_same_file(pp_module->dir_name, preprocessed_name,
	module->dir_name, module->file_name)) {
	char *pp_pathname = compose_path_name(pp_module->dir_name,
	  pp_module->file_name);
	char *m_pathname = compose_path_name(module->dir_name,
	  module->file_name);
	ERROR("Preprocessed intermediate file of `%s' (module `%s') clashes "
	  "with file `%s' containing ASN.1 module `%s'.", pp_pathname,
	  pp_module->module_name, m_pathname, module->module_name);
	Free(pp_pathname);
	Free(m_pathname);
      }
    }
    for (j = 0; j < makefile->nXSDModules; j++) {
      struct module_struct *module = makefile->XSDModules + j;
      if (module->dir_name == NULL || module->file_name == NULL) {
        continue;
      }
      if (is_same_file(pp_module->dir_name, preprocessed_name,
	module->dir_name, module->file_name)) {
	char *pp_pathname = compose_path_name(pp_module->dir_name,
	  pp_module->file_name);
	char *m_pathname = compose_path_name(module->dir_name,
	  module->file_name);
	ERROR("Preprocessed intermediate file of `%s' (module `%s') clashes "
	  "with file `%s' containing TTCN-3 module `%s'.", pp_pathname,
	  pp_module->module_name, m_pathname, module->module_name);
	Free(pp_pathname);
	Free(m_pathname);
      }
    }
    for (j = 0; j < makefile->nOtherFiles; j++) {
      char *dir_name = get_dir_name(makefile->OtherFiles[j],
                                    makefile->working_dir);
      char *file_name = get_file_from_path(makefile->OtherFiles[j]);
      if (is_same_file(pp_module->dir_name, preprocessed_name, dir_name,
	file_name)) {
	char *pp_pathname = compose_path_name(pp_module->dir_name,
	  pp_module->file_name);
	ERROR("Preprocessed intermediate file of `%s' (module `%s') clashes "
	  "with other file `%s'.", pp_pathname, pp_module->module_name,
	  makefile->OtherFiles[j]);
	Free(pp_pathname);
      }
      Free(dir_name);
      Free(file_name);
    }
    Free(preprocessed_name);
  }
}

/** Checks the name clash between existing module \a module and newly added
 * module with parameters \a path_name, \a dir_name, \a file_name,
 * \a module_name. Both the existing and the new module shall be of the same
 * kind, parameter \a kind shall contain the respective string (either "ASN.1"
 * or "TTCN-3"). If a clash is found the parameters of the new module except
 * \a path_name are deallocated and TRUE is returned. Otherwise FALSE is
 * returned. */
static boolean check_module_clash_same(const struct module_struct *module,
  const char *kind, const char *path_name, char *dir_name, char *file_name,
  char *module_name)
{
  if (is_same_module(module_name, module->module_name)) {
    if (is_same_file(dir_name, file_name,
        module->dir_name, module->file_name)) {
      /* the same file was given twice: just issue a warning */
      WARNING("File `%s' was given more than once for the Makefile.",
              path_name);
    } else {
      /* two different files contain the same module: this cannot be
       * resolved as the generated C++ files will clash */
      char *path_name1 = compose_path_name(module->dir_name,
	module->file_name);
      char *path_name2 = compose_path_name(dir_name, file_name);
      ERROR("Both files `%s' and `%s' contain %s module `%s'.",
	path_name1, path_name2, kind, module_name);
      Free(path_name1);
      Free(path_name2);
    }
    Free(file_name);
    Free(dir_name);
    Free(module_name);
    return TRUE;
  } else return FALSE;
}

/** Checks the name clash between existing module \a module and newly added
 * module with parameters \a dir_name, \a file_name, \a module_name. The two
 * modules shall be of different kinds (one is ASN.1, the other is TTCN-3).
 * Parameters \a kind1 and \a kind2 shall contain the respective strings. If a
 * clash is found the parameters of the new module are deallocated and TRUE is
 * returned. Otherwise FALSE is returned. */
static boolean check_module_clash_different(const struct module_struct *module,
  const char *kind1, char *dir_name, char *file_name, char *module_name,
  const char *kind2)
{
  if (is_same_module(module_name, module->module_name)) {
    /* two different files contain the same module: this cannot be resolved
     * as the generated C++ files will clash */
    char *path_name1 = compose_path_name(module->dir_name, module->file_name);
    char *path_name2 = compose_path_name(dir_name, file_name);
    ERROR("File `%s' containing %s module `%s' and file `%s' containing "
      "%s module `%s' cannot be used together in the same Makefile.",
      path_name1, kind1, module->module_name, path_name2, kind2, module_name);
    Free(path_name1);
    Free(path_name2);
    Free(file_name);
    Free(dir_name);
    Free(module_name);
    return TRUE;
  } else return FALSE;
}

/** Adds a TTCN-3 module to Makefile descriptor structure \a makefile.
 * The name of the TTCN-3 source file is \a path_name, the module identifier
 * is \a module_name. It is checked whether a file or module with the same name
 * already exists in \a makefile and an appropriate warning or error is
 * reported. */
static void add_ttcn3_module(struct makefile_struct *makefile,
  const char *path_name, char *module_name)
{
  struct module_struct *module;
  char *dir_name = get_dir_name(path_name, makefile->working_dir);
  char *file_name = get_file_from_path(path_name);
  const char *suffix = get_suffix(file_name);
  size_t i;
  boolean is_preprocessed = FALSE;

  if (suffix != NULL) {
    if (!strcmp(suffix, "ttcnpp")) {
      if (makefile->preprocess) is_preprocessed = TRUE;
      else WARNING("The suffix of TTCN-3 file `%s' indicates that it should be "
        "preprocessed, but TTCN-3 preprocessing is not enabled. The file "
        "will be added to the list of normal TTCN-3 modules in the Makefile.",
        path_name);
    } else if (!strcmp(suffix, "ttcnin")) {
      WARNING("The suffix of file `%s' indicates that it should be a "
        "preprocessor include file, but it contains a TTCN-3 module named `%s'. "
        "The file will be added to the list of normal TTCN-3 modules in the "
        "Makefile.", path_name, module_name);
    }
  }

  for (i = 0; i < makefile->nASN1Modules; i++) {
    if (check_module_clash_different(makefile->ASN1Modules + i, "ASN.1",
      dir_name, file_name, module_name, "TTCN-3")) return;
  }
  /* never entered if suffix is NULL */
  if (is_preprocessed) {
    char *file_prefix;
    for (i = 0; i < makefile->nTTCN3PPModules; i++) {
      if (check_module_clash_same(makefile->TTCN3PPModules + i, "TTCN-3",
          path_name, dir_name, file_name, module_name)) return;
    }
    for (i = 0; i < makefile->nXSDModules; i++) {
      if (check_module_clash_different(makefile->XSDModules + i, "TTCN-3",
        dir_name, file_name, module_name, "TTCN-3")) return;
    }
    /* clashes with normal TTCN-3 modules will be checked (and maybe resolved)
     * in \a check_preprocessed_filename_collision() */
    /* add it to the list of TTCN-3 modules to be preprocessed */
    makefile->TTCN3PPModules = (struct module_struct*)
      Realloc(makefile->TTCN3PPModules,
        (makefile->nTTCN3PPModules + 1) * sizeof(*makefile->TTCN3PPModules));
    module = makefile->TTCN3PPModules + makefile->nTTCN3PPModules;
    makefile->nTTCN3PPModules++;
    module->dir_name = dir_name;
    module->file_name = file_name;
    module->module_name = module_name;
    file_prefix = cut_suffix(file_name);
    if (!strcmp(file_prefix, module_name)) module->is_regular = TRUE;
    else module->is_regular = FALSE;
    Free(file_prefix);
  } else {
    /* the file is not preprocessed */
    for (i = 0; i < makefile->nTTCN3Modules; i++) {
      if (check_module_clash_same(makefile->TTCN3Modules + i, "TTCN-3",
          path_name, dir_name, file_name, module_name)) return;
    }
    /* clashes with preprocessed TTCN-3 modules will be checked (and maybe
     * resolved) in \a check_preprocessed_filename_collision() */
    /* add it to the list of normal TTCN-3 modules */
    makefile->TTCN3Modules = (struct module_struct*)
      Realloc(makefile->TTCN3Modules,
        (makefile->nTTCN3Modules + 1) * sizeof(*makefile->TTCN3Modules));
    module = makefile->TTCN3Modules + makefile->nTTCN3Modules;
    makefile->nTTCN3Modules++;
    module->dir_name = dir_name;
    module->file_name = file_name;
    module->module_name = module_name;
    if (suffix != NULL && !strcmp(suffix, "ttcn")) {
      char *file_prefix = cut_suffix(file_name);
      if (!strcmp(file_prefix, module_name)) module->is_regular = TRUE;
      else module->is_regular = FALSE;
      Free(file_prefix);
    } else {
      module->is_regular = FALSE;
    }
  }
}

/** ASN.1 filename shall contain no hyphen */
static boolean is_valid_asn1_filename(const char* file_name)
{
  if (0 == strchr(file_name, '-')) {
    return TRUE;
  }
  return FALSE;
}

/** Adds an ASN.1 module to Makefile descriptor structure \a makefile.
 * The name of the ASN.1 source file is \a path_name, the module identifier
 * is \a module_name. It is checked whether a file or module with the same name
 * already exists in \a makefile and an appropriate warning or error is
 * reported. */
static void add_asn1_module(struct makefile_struct *makefile,
  const char *path_name, char *module_name)
{
  struct module_struct *module;
  char *dir_name = get_dir_name(path_name, makefile->working_dir);
  char *file_name = get_file_from_path(path_name);
  const char *suffix = get_suffix(file_name);
  size_t i;
  for (i = 0; i < makefile->nASN1Modules; i++) {
    if (check_module_clash_same(makefile->ASN1Modules + i, "ASN.1", path_name,
      dir_name, file_name, module_name)) return;
  }
  for (i = 0; i < makefile->nTTCN3Modules; i++) {
    if (check_module_clash_different(makefile->TTCN3Modules + i, "TTCN-3",
      dir_name, file_name, module_name, "ASN.1")) return;
  }
  if (makefile->preprocess) {
    for (i = 0; i < makefile->nTTCN3PPModules; i++) {
      if (check_module_clash_different(makefile->TTCN3PPModules + i, "TTCN-3",
	dir_name, file_name, module_name, "ASN.1")) return;
    }
  }
  for (i = 0; i < makefile->nXSDModules; i++) {
    if (check_module_clash_different(makefile->XSDModules + i, "TTCN-3",
      dir_name, file_name, module_name, "ASN.1")) return;
  }
  makefile->ASN1Modules = (struct module_struct*)
    Realloc(makefile->ASN1Modules,
      (makefile->nASN1Modules + 1) * sizeof(*makefile->ASN1Modules));
  module = makefile->ASN1Modules + makefile->nASN1Modules;
  makefile->nASN1Modules++;
  module->dir_name = dir_name;
  module->file_name = file_name;
  module->module_name = module_name;
  if (suffix != NULL && !strcmp(suffix, "asn")) {
    char *file_prefix = cut_suffix(file_name);
    /* replace all '_' with '-' in file name prefix */
    for (i = 0; file_prefix[i] != '\0'; i++)
      if (file_prefix[i] == '_') file_prefix[i] = '-';
    if (!strcmp(file_prefix, module_name)) module->is_regular = TRUE;
    else module->is_regular = FALSE;
    Free(file_prefix);
  } else {
    module->is_regular = FALSE;
  }
}

/** Adds an XSD module to Makefile descriptor structure \a makefile.
 * The name of the XSD source file is \a path_name, the module identifier
 * is \a module_name. It is checked whether a file or module with the same name
 * already exists in \a makefile and an appropriate warning or error is
 * reported. */
static void add_xsd_module(struct makefile_struct *makefile,
  const char *path_name, char *module_name)
{
  struct module_struct *module;
  char *dir_name = get_dir_name(path_name, makefile->working_dir);
  char *file_name = get_file_from_path(path_name);
  size_t i;
  for (i = 0; i < makefile->nXSDModules; i++) { 
    if (strcmp(makefile->XSDModules[i].module_name, module_name) == 0) {
      WARNING("The XSD file `%s' containing TTCN-3 module `%s' and XSD "
      "file `%s' containing XSD module `%s' could cause problems.",
      makefile->XSDModules[i].file_name,
      makefile->XSDModules[i].module_name,
      path_name, module_name);
      break;
    }
  }
  for (i = 0; i < makefile->nXSDModules; i++) {
    if (makefile->XSDModules[i].file_name == NULL) continue;
    if (strcmp(makefile->XSDModules[i].file_name, file_name) == 0) {
      WARNING("File `%s' was given more than once for the Makefile.",
              path_name);
      break;
    }
  }
  for (i = 0; i < makefile->nASN1Modules; i++) {
    if (check_module_clash_different(makefile->ASN1Modules + i, "ASN.1",
      dir_name, file_name, module_name, "TTCN-3")) return;
  }
  if (makefile->preprocess) {
    for (i = 0; i < makefile->nTTCN3PPModules; i++) {
      if (check_module_clash_different(makefile->TTCN3PPModules + i, "TTCN-3",
	        dir_name, file_name, module_name, "TTCN-3")) return;
    }
  }
  // The first XSD module, insert UsefulTtcn3Types.ttcn and XSD.ttcn too
  if (makefile->nXSDModules == 0) {
    makefile->XSDModules = (struct module_struct*)
    Realloc(makefile->XSDModules,
      (makefile->nXSDModules + 2) * sizeof(*makefile->XSDModules));
    module = makefile->XSDModules + makefile->nXSDModules;
    makefile->nXSDModules++;
    module->dir_name = NULL;
    module->file_name = NULL;
    module->module_name = mprintf("UsefulTtcn3Types");
    module->is_regular = FALSE; // Always false
    module = makefile->XSDModules + makefile->nXSDModules;
    makefile->nXSDModules++;
    module->dir_name = NULL;
    module->file_name = NULL;
    module->module_name = mprintf("XSD");
    module->is_regular = FALSE; // Always false
  }
  makefile->XSDModules = (struct module_struct*)
    Realloc(makefile->XSDModules,
      (makefile->nXSDModules + 1) * sizeof(*makefile->XSDModules));
  module = makefile->XSDModules + makefile->nXSDModules;
  makefile->nXSDModules++;
  module->dir_name = dir_name;
  module->file_name = file_name;
  module->module_name = module_name;
  module->is_regular = FALSE; // Always false
}

/** Adds the file named \a path_name to the list of files pointed by \a list_ptr
 * and \a list_size. The suffix or contents of \a path_name are not examined,
 * only duplicate entries are checked. In case of duplicate entries warning is
 * reported only if argument \a report_warning is set to TRUE. */
static void add_path_to_list(size_t *list_size, char ***list_ptr,
  const char *path_name, const char *working_dir, boolean report_warning)
{
  size_t i;
  char *dir_name = get_dir_name(path_name, working_dir);
  char *file_name = get_file_from_path(path_name);
  char *canonized_path_name = compose_path_name(dir_name, file_name);
  Free(dir_name);
  Free(file_name);
  for (i = 0; i < *list_size; i++) {
    if (!strcmp(canonized_path_name, (*list_ptr)[i])) {
      if (report_warning) WARNING("File `%s' was given more than once for the "
                                  "Makefile.", path_name);
      Free(canonized_path_name);
      return;
    }
  }
  *list_ptr = (char**)Realloc(*list_ptr, (*list_size + 1) * sizeof(**list_ptr));
  (*list_ptr)[*list_size] = canonized_path_name;
  (*list_size)++;
}

/** Adds a C/C++ header or source file or an other file named \a path_name to
 * Makefile descriptor structure \a makefile. The file is classified based on
 * its suffix and not by content. If the file clashes with existing files or
 * modules the appropriate warning or error is generated. */
static void add_user_file(struct makefile_struct *makefile,
  const char *path_name)
{
  const char *suffix = get_suffix(path_name);
  if (suffix != NULL) {
    if (!strcmp(suffix, "ttcn") || !strcmp(suffix, "ttcn3") ||
        !strcmp(suffix, "3mp") || !strcmp(suffix, "ttcnpp")) {
      /* The file content was already checked. Since it doesn't look like
       * a valid TTCN-3 file, these suffixes are suspect */
      WARNING("File `%s' does not contain a valid TTCN-3 module. "
              "It will be added to the Makefile as other file.", path_name);
    }
    else if (!strcmp(suffix, "ttcnin")) {
      /* this is a TTCN-3 include file */
      if (makefile->preprocess) {
        add_path_to_list(&makefile->nTTCN3IncludeFiles,
        &makefile->TTCN3IncludeFiles, path_name, makefile->working_dir, TRUE);
        return;
      } 
      else {
        WARNING("The suffix of file `%s' indicates that it is a TTCN-3 "
                "include file, but TTCN-3 preprocessing is not enabled. The file "
                "will be added to the Makefile as other file.", path_name);
      }
    } 
    else if (!strcmp(suffix, "asn") || !strcmp(suffix, "asn1")) {
      /* The file content was already checked. Since it doesn't look like
       * a valid ASN.1 file, these suffixes are suspect */
      WARNING("File `%s' does not contain a valid ASN.1 module. "
              "It will be added to the Makefile as other file.", path_name);
    } else if (!strcmp(suffix, "xsd")) {
      WARNING("File `%s' does not contain a valid XSD module. "
              "It will be added to the Makefile as other file.", path_name);
    }
    else if (!strcmp(suffix, "cc") || !strcmp(suffix, "c") || !strcmp(suffix, "cpp")) {
      /* this is a source file */
      char *dir_name = get_dir_name(path_name, makefile->working_dir);
      char *file_name = get_file_from_path(path_name);
      char *file_prefix = cut_suffix(file_name);
      struct user_struct *user;
      size_t i;
      for (i = 0; i < makefile->nUserFiles; i++) {
        user = makefile->UserFiles + i;
        if (!strcmp(file_prefix, user->file_prefix)) {
          if (user->source_name != NULL) {
          /* the source file is already present */
            if (is_same_file(dir_name, file_name,
              user->dir_name, user->source_name)) {
              WARNING("File `%s' was given more than once for the Makefile.", path_name);
            }
            else {
              char *path_name1 = compose_path_name(user->dir_name, user->source_name);
              char *path_name2 = compose_path_name(dir_name, file_name);
              ERROR("C/C++ source files `%s' and `%s' cannot be used together "
                    "in the same Makefile.", path_name1, path_name2);
              Free(path_name1);
              Free(path_name2);
            }
          }
          else {
            /* a header file with the same prefix is already present */
            if (is_same_directory(dir_name, user->dir_name)) {
              user->source_name = file_name;
              file_name = NULL;
              if (!strcmp(suffix, "cc") || !strcmp(suffix, "cpp")) 
                user->has_cc_suffix = TRUE;
            }
            else {
              char *path_name1 = compose_path_name(dir_name, file_name);
              char *path_name2 = compose_path_name(user->dir_name, user->header_name);
              ERROR("C/C++ source file `%s' cannot be used together with "
              "header file `%s' in the same Makefile.", path_name1,
              path_name2);
              Free(path_name1);
              Free(path_name2);
            }
          }
          Free(dir_name);
          Free(file_name);
          Free(file_prefix);
          return;
        }
      }
      makefile->UserFiles = (struct user_struct*)Realloc(makefile->UserFiles,
      (makefile->nUserFiles + 1) * sizeof(*makefile->UserFiles));
      user = makefile->UserFiles + makefile->nUserFiles;
      makefile->nUserFiles++;
      user->dir_name = dir_name;
      user->file_prefix = file_prefix;
      user->header_name = NULL;
      user->source_name = file_name;
      user->has_hh_suffix = FALSE;
      if (!strcmp(suffix, "cc") || !strcmp(suffix, "cpp")) user->has_cc_suffix = TRUE;
      else user->has_cc_suffix = FALSE;
      return;
    }
    else if (!strcmp(suffix, "hh") || !strcmp(suffix, "h")) {
      /* this is a header file */
      char *dir_name = get_dir_name(path_name, makefile->working_dir);
      char *file_name = get_file_from_path(path_name);
      char *file_prefix = cut_suffix(file_name);
      struct user_struct *user;
      size_t i;
      for (i = 0; i < makefile->nUserFiles; i++) {
        user = makefile->UserFiles + i;
        if (!strcmp(file_prefix, user->file_prefix)) {
          if (user->header_name != NULL) {
          /* the header file is already present */
            if (is_same_file(dir_name, file_name, user->dir_name, user->header_name)) {
              WARNING("File `%s' was given more than once for the Makefile.", path_name);
            }
            else {
              char *path_name1 = compose_path_name(user->dir_name, user->header_name);
              char *path_name2 = compose_path_name(dir_name, file_name);
              ERROR("C/C++ header files `%s' and `%s' cannot be used together "
              "in the same Makefile.", path_name1, path_name2);
              Free(path_name1);
              Free(path_name2);
            }
          }
          else {
          /* a source file with the same prefix is already present */
            if (is_same_directory(dir_name, user->dir_name)) {
              user->header_name = file_name;
              file_name = NULL;
              if (!strcmp(suffix, "hh") || !strcmp(suffix, "hpp")) 
                user->has_hh_suffix = TRUE;
            }
            else {
              char *path_name1 = compose_path_name(dir_name, file_name);
              char *path_name2 = compose_path_name(user->dir_name, user->source_name);
              ERROR("C/C++ header file `%s' cannot be used together with "
                    "source file `%s' in the same Makefile.", path_name1, path_name2);
              Free(path_name1);
              Free(path_name2);
            }
          }
          Free(dir_name);
          Free(file_name);
          Free(file_prefix);
          return;
        }
      }
      makefile->UserFiles = (struct user_struct*)Realloc(makefile->UserFiles,
        (makefile->nUserFiles + 1) * sizeof(*makefile->UserFiles));
      user = makefile->UserFiles + makefile->nUserFiles;
      makefile->nUserFiles++;
      user->dir_name = dir_name;
      user->file_prefix = file_prefix;
      user->header_name = file_name;
      user->source_name = NULL;
      if (!strcmp(suffix, "hh") || !strcmp(suffix, "hpp")) user->has_hh_suffix = TRUE;
      else user->has_hh_suffix = FALSE;
      user->has_cc_suffix = FALSE;
      return;
    }
  } /* end if (suffix != NULL) */
  /* treat the file as other file if it was not handled yet */
  add_path_to_list(&makefile->nOtherFiles, &makefile->OtherFiles, path_name,
    makefile->working_dir, TRUE);
}

static void add_file_to_makefile(struct makefile_struct *makefile, char *argument) {
  char *file_name = get_file_name_for_argument(argument);
  if (file_name != NULL) {
    FILE *fp = fopen(file_name, "r");
    if (fp != NULL) {
      char *module_name = NULL;
      if (is_ttcn3_module(file_name, fp, &module_name)) {
        if (is_asn1_module(file_name, fp, NULL)) {
          ERROR("File `%s' looks so strange that it can be both ASN.1 and "
          "TTCN-3 module. Add it to the Makefile manually.", file_name);
          Free(module_name);
        } else {
            add_ttcn3_module(makefile, file_name, module_name);
          }
     } else if (is_asn1_module(file_name, fp, &module_name)) {
         expstring_t only_file_name = get_file_from_path(file_name);
         if (is_valid_asn1_filename(only_file_name)) {
           add_asn1_module(makefile, file_name, module_name);
         } else {
             ERROR("The file name `%s' (without suffix) shall be identical to the module name `%s'.\n"
                   "If the name of the ASN.1 module contains a hyphen, the corresponding "
                   "file name shall contain an underscore character instead.", file_name, module_name);
             Free(module_name);
         }
         Free(only_file_name);
     } else if (is_xsd_module(file_name, &module_name)) {
       add_xsd_module(makefile, file_name, module_name);
     } else {
       add_user_file(makefile, file_name);
     }
     fclose(fp);
    } else {
       ERROR("Cannot open file `%s' for reading: %s", file_name,
       strerror(errno));
       errno = 0;
      }
    Free(file_name);
  } else if (get_path_status(argument) == PS_DIRECTORY) {
    ERROR("Argument `%s' is a directory.", argument);
  } else {
    ERROR("Cannot find any source file for argument `%s'.", argument);
  }
}

/** Removes the generated C++ header and/or source files of module \a module
 * from Makefile descriptor structure \a makefile. A warning is displayed if
 * such file is found. */
static void drop_generated_files(struct makefile_struct *makefile,
  const struct module_struct *module)
{
  char *module_name = mcopystr(module->module_name);
  size_t i;
  /* transform all '-' characters in ASN.1 module name to '_' */
  for (i = 0; module_name[i] != '\0'; i++)
    if (module_name[i] == '-') module_name[i] = '_';
  for (i = 0; i < makefile->nUserFiles; i++) {
    struct user_struct *user = makefile->UserFiles + i;
    if (!strcmp(module_name, user->file_prefix)) {
      char *m_pathname = compose_path_name(module->dir_name, module->file_name);
      /** Note: if central storage is used the generated C++ files are placed
       * into the same directory as the TTCN-3/ASN.1 modules, otherwise the
       * files are generated into the working directory. */
      boolean is_same_dir = is_same_directory(user->dir_name,
	makefile->central_storage ? module->dir_name : NULL);
      if (user->header_name != NULL) {
	char *u_pathname = compose_path_name(user->dir_name,
	  user->header_name);
	if (is_same_dir && user->has_hh_suffix) {
	  WARNING("Header file `%s' is generated from module `%s' (file `%s'). "
	    "Removing it from the list of user files.", u_pathname,
	    module->module_name, m_pathname);
	} else {
	  ERROR("Header file `%s' cannot be used together with module `%s' "
	    "(file `%s') in the same Makefile.", u_pathname,
	    module->module_name, m_pathname);
	}
	Free(u_pathname);
      }
      if (user->source_name != NULL) {
	char *u_pathname = compose_path_name(user->dir_name,
	  user->source_name);
	if (is_same_dir && user->has_cc_suffix) {
	  WARNING("Source file `%s' is generated from module `%s' (file "
	    "`%s'). Removing it from the list of user files.", u_pathname,
	    module->module_name, m_pathname);
	} else {
	  ERROR("Source file `%s' cannot be used together with module "
	    "`%s' (file `%s') in the same Makefile.", u_pathname,
	    module->module_name, m_pathname);
	}
	Free(u_pathname);
      }
      Free(m_pathname);
      Free(user->dir_name);
      Free(user->file_prefix);
      Free(user->header_name);
      Free(user->source_name);
      makefile->nUserFiles--;
      memmove(user, user + 1, (makefile->nUserFiles - i) *
	sizeof(*makefile->UserFiles));
      makefile->UserFiles = (struct user_struct*)Realloc(makefile->UserFiles,
	makefile->nUserFiles * sizeof(*makefile->UserFiles));
      break;
    }
  }
  Free(module_name);
}

static void drop_generated_TTCN3_files(struct makefile_struct *makefile,
  const struct module_struct *module) {
  size_t i;
  for (i = 0; i < makefile->nTTCN3Modules; i++) {
    struct module_struct *ttcn_module = makefile->TTCN3Modules + i;
    char *module_file_name = NULL;
    module_file_name = mputprintf(module_file_name, "%s.ttcn", module->module_name);
    /** Note: if central storage is used the generated C++ files are placed
     * into the same directory as the TTCN-3/ASN.1 modules, otherwise the
     * files are generated into the working directory. */
    if (strcmp(ttcn_module->file_name, module_file_name) == 0) {
      char *m_pathname = compose_path_name(module->dir_name, module->file_name);
      boolean is_same_dir = is_same_directory(ttcn_module->dir_name,
        makefile->central_storage ? module->dir_name : NULL);
      char *u_pathname = compose_path_name(ttcn_module->dir_name,
        ttcn_module->file_name);
      if (is_same_dir) {
        WARNING("TTCN-3 file `%s' is generated from XSD file `%s'."
          " Removing it from the list of user files.", u_pathname,
          m_pathname);
      } else {
        ERROR("TTCN-3 file `%s' cannot be used together with module "
          "`%s' in the same Makefile.", u_pathname,
          m_pathname);
      }
      Free(u_pathname);
      Free(module_file_name);
      Free(m_pathname);
      makefile->nTTCN3Modules--;
      memmove(ttcn_module, ttcn_module + 1, (makefile->nTTCN3Modules - i) *
        sizeof(*makefile->TTCN3Modules));
      makefile->TTCN3Modules = (struct module_struct*)Realloc(makefile->TTCN3Modules,
        makefile->nTTCN3Modules * sizeof(*makefile->TTCN3Modules));
      break;
    } else {
      Free(module_file_name);
    }
  }
}

/** Drops all C++ header and source files of the Makefile descriptor structure
 * \a makefile that are generated from its TTCN-3 or ASN.1 modules.
 * in the case of XSD modules the generated TTCN-3 file will be dropped. */
static void filter_out_generated_files(struct makefile_struct *makefile)
{
  size_t i;
  for (i = 0; i < makefile->nTTCN3Modules; i++) {
    drop_generated_files(makefile, makefile->TTCN3Modules + i);
  }
  if (makefile->preprocess) {
    for (i = 0; i < makefile->nTTCN3PPModules; i++) {
      drop_generated_files(makefile, makefile->TTCN3PPModules + i);
    }
  }
  for (i = 0; i < makefile->nASN1Modules; i++) {
    drop_generated_files(makefile, makefile->ASN1Modules + i);
  }
  for (i = 0; i < makefile->nXSDModules; i++) {
    drop_generated_TTCN3_files(makefile, makefile->XSDModules + i);
  }
}

/** Completes the list of user C/C++ header and source files in \a makefile.
 * If only the source file was given the function looks for the corresponding
 * header file or vice versa. */
static void complete_user_files(const struct makefile_struct *makefile)
{
  size_t i;
  for (i = 0; i < makefile->nUserFiles; i++) {
    struct user_struct *user = makefile->UserFiles + i;
    if (user->header_name == NULL) {
      static const char * const suffix_list[] = { "hh", "h", "hpp", NULL };
      const char * const *suffix_ptr;
      for (suffix_ptr = suffix_list; *suffix_ptr != NULL; suffix_ptr++) {
        char *file_name = mprintf("%s.%s", user->file_prefix, *suffix_ptr);
        char *path_name = compose_path_name(user->dir_name, file_name);
        if (get_path_status(path_name) == PS_FILE) {
          Free(path_name);
          user->header_name = file_name;
          if (!strcmp(*suffix_ptr, "hh") || !strcmp(*suffix_ptr, "hpp"))
            user->has_hh_suffix = TRUE;
          break;
        }
        Free(file_name);
        Free(path_name);
      }
    }
    else if (user->source_name == NULL) {
      static const char * const suffix_list[] = { "cc", "c", "cpp", NULL };
      const char * const *suffix_ptr;
      for (suffix_ptr = suffix_list; *suffix_ptr != NULL; suffix_ptr++) {
        char *file_name = mprintf("%s.%s", user->file_prefix, *suffix_ptr);
        char *path_name = compose_path_name(user->dir_name, file_name);
        if (get_path_status(path_name) == PS_FILE) {
          Free(path_name);
          user->source_name = file_name;
          if (!strcmp(*suffix_ptr, "cc") || !strcmp(*suffix_ptr, "cpp"))
            user->has_cc_suffix = TRUE;
          break;
        }
        Free(file_name);
        Free(path_name);
      }
    }
  }
}

/** Converts the directory name pointed by \a dir_ptr to a relative pathname
 * based on \a working_dir. The original directory name is deallocated and
 * replaced with a new string. Nothing happens if \a dir_ptr points to NULL. */
static void replace_dir_with_relative(char **dir_ptr, const char *working_dir)
{
  if (*dir_ptr != NULL) {
    char *rel_dir = get_relative_dir(*dir_ptr, working_dir);
    Free(*dir_ptr);
    *dir_ptr = rel_dir;
  }
}

/** Converts the directory part of path name pointed by \a path_ptr to a relative
 * pathname based on \a working_dir. The original path name is deallocated and
 * replaced with a new string. */
static void convert_path_to_relative(char **path_ptr, const char *working_dir)
{
  char *dir_name = get_dir_name(*path_ptr, working_dir);
  if (dir_name != NULL) {
    char *file_name = get_file_from_path(*path_ptr);
    replace_dir_with_relative(&dir_name, working_dir);
    Free(*path_ptr);
    *path_ptr = compose_path_name(dir_name, file_name);
    Free(file_name);
    Free(dir_name);
  }
}

/** Converts all directories used by the Makefile descriptor structure
 * \a makefile to relative pathnames based on the working directory stored in
 * \a makefile. */
static void convert_dirs_to_relative(struct makefile_struct *makefile)
{
  size_t i;
  for (i = 0; i < makefile->nTTCN3Modules; i++) {
    replace_dir_with_relative(&makefile->TTCN3Modules[i].dir_name,
      makefile->working_dir);
  }
  if (makefile->preprocess) {
    for (i = 0; i < makefile->nTTCN3PPModules; i++) {
      replace_dir_with_relative(&makefile->TTCN3PPModules[i].dir_name,
                                makefile->working_dir);
    }
    for (i = 0; i < makefile->nTTCN3IncludeFiles; i++) {
      convert_path_to_relative(makefile->TTCN3IncludeFiles + i,
	makefile->working_dir);
    }
  }
  for (i = 0; i < makefile->nASN1Modules; i++) {
    replace_dir_with_relative(&makefile->ASN1Modules[i].dir_name,
      makefile->working_dir);
  }
  for (i = 0; i < makefile->nXSDModules; i++) {
    replace_dir_with_relative(&makefile->XSDModules[i].dir_name,
      makefile->working_dir);
  }
  for (i = 0; i < makefile->nUserFiles; i++) {
    replace_dir_with_relative(&makefile->UserFiles[i].dir_name,
      makefile->working_dir);
  }
  for (i = 0; i < makefile->nOtherFiles; i++) {
    convert_path_to_relative(makefile->OtherFiles + i, makefile->working_dir);
  }
  if (makefile->ets_name != NULL)
    convert_path_to_relative(&makefile->ets_name, makefile->working_dir);
}

/* Returns whether the string \a file_name contains special characters. */
static boolean has_special_chars(const char *file_name)
{
  if (file_name != NULL) {
    size_t i;
    for (i = 0; ; i++) {
      int c = (unsigned char)file_name[i];
      switch (c) {
      case '\0':
	return FALSE;
      case ' ':
      case '*':
      case '?':
      case '[':
      case ']':
      case '<':
      case '=':
      case '>':
      case '|':
      case '&':
      case '$':
      case '%':
      case '{':
      case '}':
      case ';':
      case ':':
      case '(':
      case ')':
      case '#':
      case '!':
      case '\'':
      case '"':
      case '`':
      case '\\':
	return TRUE;
      default:
	if (!isascii(c) || !isprint(c)) return TRUE;
	break;
      }
    }
  }
  return FALSE;
}

/** Checks whether the path name composed of \a dir_name and \a file_name
 * contains special characters that are not allowed in the Makefile. Parameter
 * \a what contains the description of the corresponding file. */
static void check_special_chars_in_path(const char *dir_name,
  const char *file_name, const char *what)
{
  if (has_special_chars(dir_name) || has_special_chars(file_name)) {
    char *path_name = compose_path_name(dir_name, file_name);
    ERROR("The name of %s `%s' contains special characters that cannot be "
      "handled properly by the `make' utility and/or the shell.", what,
      path_name);
    Free(path_name);
  }
}

/** Checks whether the directory names or file names that will be used in the
 * generated Makefile contain special characters that cannot be handled by the
 * "make" utility. */
static void check_special_chars(const struct makefile_struct *makefile)
{
  size_t i;
  for (i = 0; i < makefile->nTTCN3Modules; i++) {
    check_special_chars_in_path(makefile->TTCN3Modules[i].dir_name,
      makefile->TTCN3Modules[i].file_name, "TTCN-3 file");
  }
  if (makefile->preprocess) {
    for (i = 0; i < makefile->nTTCN3PPModules; i++) {
      check_special_chars_in_path(makefile->TTCN3PPModules[i].dir_name,
	makefile->TTCN3PPModules[i].file_name,
	"TTCN-3 file to be preprocessed");
    }
  }
  for (i = 0; i < makefile->nASN1Modules; i++) {
    check_special_chars_in_path(makefile->ASN1Modules[i].dir_name,
      makefile->ASN1Modules[i].file_name, "ASN.1 file");
  }
  for (i = 0; i < makefile->nXSDModules; i++) {
    check_special_chars_in_path(makefile->XSDModules[i].dir_name,
      makefile->XSDModules[i].file_name, "XSD file");
  }
  for (i = 0; i < makefile->nUserFiles; i++) {
    const struct user_struct *user = makefile->UserFiles + i;
    if (user->source_name != NULL)
      check_special_chars_in_path(user->dir_name, user->source_name,
	"C/C++ source file");
    else check_special_chars_in_path(user->dir_name, user->header_name,
	"C/C++ header file");
  }
  for (i = 0; i < makefile->nOtherFiles; i++) {
    check_special_chars_in_path(NULL, makefile->OtherFiles[i], "other file");
  }
}

/** Adds base directory \a dir_name to Makefile descriptor structure
 * \a makefile. Flag \a has_modules indicates whether \a dir_name contains
 * TTCN-3 and/or ASN.1 modules or XSD modules. The new directory is ignored if it is already
 * added to \a makefile. */
static void add_base_dir(struct makefile_struct *makefile,
  const char *dir_name, boolean has_modules)
{
  if (dir_name != NULL) {
    struct base_dir_struct *base_dir;
    size_t i;
    for (i = 0; i < makefile->nBaseDirs; i++) {
      base_dir = makefile->BaseDirs + i;
      if (!strcmp(dir_name, base_dir->dir_name)) {
	if (has_modules) base_dir->has_modules = TRUE;
	return;
      }
    }
    makefile->BaseDirs = (struct base_dir_struct*)Realloc(makefile->BaseDirs,
      (makefile->nBaseDirs + 1) * sizeof(*makefile->BaseDirs));
    base_dir = makefile->BaseDirs + makefile->nBaseDirs;
    makefile->nBaseDirs++;
    base_dir->dir_name = dir_name;
    base_dir->has_modules = has_modules;
  }
}

/** Collects all directories that are used in the Makefile descriptor structure
 * \a makefile in order to use pre-compiled files from them. */
static void collect_base_dirs(struct makefile_struct *makefile)
{
  size_t i;
  for (i = 0; i < makefile->nTTCN3Modules; i++) {
    add_base_dir(makefile, makefile->TTCN3Modules[i].dir_name, TRUE);
  }
  if (makefile->preprocess) {
    for (i = 0; i < makefile->nTTCN3PPModules; i++) {
      add_base_dir(makefile, makefile->TTCN3PPModules[i].dir_name, TRUE);
    }
  }
  for (i = 0; i < makefile->nASN1Modules; i++) {
    add_base_dir(makefile, makefile->ASN1Modules[i].dir_name, TRUE);
  }
  for (i = 0; i < makefile->nXSDModules; i++) {
    add_base_dir(makefile, makefile->XSDModules[i].dir_name, TRUE);
  }
  for (i = 0; i < makefile->nUserFiles; i++) {
    add_base_dir(makefile, makefile->UserFiles[i].dir_name, FALSE);
  }
  if (makefile->nBaseDirs == 0) {
    WARNING("Usage of pre-compiled files from central storage (option `-c') "
      "is enabled, but all given files are located in the current working "
      "directory.");
  }
}

/** Checks whether the TTCN-3, ASN.1 and C++ files follow the default naming
 * convention and sets the appropriate flags accordingly. */
static void check_naming_convention(struct makefile_struct *makefile)
{
  /* initially set all flags to true */
  makefile->TTCN3ModulesRegular = TRUE;
  makefile->BaseTTCN3ModulesRegular = TRUE;
  makefile->ASN1ModulesRegular = TRUE;
  makefile->BaseASN1ModulesRegular = TRUE;
  makefile->UserHeadersRegular = TRUE;
  makefile->UserSourcesRegular = TRUE;
  makefile->BaseUserHeadersRegular = TRUE;
  makefile->BaseUserSourcesRegular = TRUE;
  if (makefile->central_storage) {
    /* this project (Makefile) will use pre-compiled files from other
       directories */
    size_t i;
    for (i = 0; i < makefile->nTTCN3Modules; i++) {
      const struct module_struct *module = makefile->TTCN3Modules + i;
      if (module->dir_name != NULL) {
        if (!module->is_regular) makefile->BaseTTCN3ModulesRegular = FALSE;
      } 
      else {
        if (!module->is_regular) makefile->TTCN3ModulesRegular = FALSE;
      }
      if (!makefile->TTCN3ModulesRegular && !makefile->BaseTTCN3ModulesRegular)
        break;
    }
    /* ttcnpp files are ttcn files */
    if ((makefile->TTCN3ModulesRegular || makefile->BaseTTCN3ModulesRegular) &&
         makefile->preprocess) {
      for (i = 0; i < makefile->nTTCN3PPModules; i++) {
        const struct module_struct *module = makefile->TTCN3PPModules + i;
        if (module->dir_name != NULL) {
          if (!module->is_regular) makefile->BaseTTCN3ModulesRegular = FALSE;
        } else {
          if (!module->is_regular) makefile->TTCN3ModulesRegular = FALSE;
        }
        if (!makefile->TTCN3ModulesRegular && !makefile->BaseTTCN3ModulesRegular)
        break;
      }
    }
    for (i = 0; i < makefile->nASN1Modules; i++) {
      const struct module_struct *module = makefile->ASN1Modules + i;
      if (module->dir_name != NULL) {
        if (!module->is_regular) makefile->BaseASN1ModulesRegular = FALSE;
      }
      else {
        if (!module->is_regular) makefile->ASN1ModulesRegular = FALSE;
      }
      if (!makefile->ASN1ModulesRegular && !makefile->BaseASN1ModulesRegular)
        break;
    }
    for (i = 0; i < makefile->nUserFiles; i++) {
      const struct user_struct *user = makefile->UserFiles + i;
      if (user->dir_name != NULL) {
        if (!user->has_cc_suffix)
          makefile->BaseUserSourcesRegular = FALSE;
        if (!user->has_cc_suffix || !user->has_hh_suffix)
          makefile->BaseUserHeadersRegular = FALSE;
      }
      else {
        if (!user->has_cc_suffix)
          makefile->UserSourcesRegular = FALSE;
        if (!user->has_cc_suffix || !user->has_hh_suffix)
          makefile->UserHeadersRegular = FALSE;
      }
      if (!makefile->UserHeadersRegular && !makefile->UserSourcesRegular &&
          !makefile->BaseUserHeadersRegular &&
          !makefile->BaseUserSourcesRegular) break;
    }
  } else {
    /* this project (Makefile) will-be stand-alone */
    size_t i;
    for (i = 0; i < makefile->nTTCN3Modules; i++) {
      const struct module_struct *module = makefile->TTCN3Modules + i;
      if (!module->is_regular || module->dir_name != NULL) {
        makefile->TTCN3ModulesRegular = FALSE;
        break;
      }
    }
    if (makefile->TTCN3ModulesRegular && makefile->preprocess) {
      for (i = 0; i < makefile->nTTCN3PPModules; i++) {
        const struct module_struct *module = makefile->TTCN3PPModules + i;
        if (!module->is_regular || module->dir_name != NULL) {
          makefile->TTCN3ModulesRegular = FALSE;
          break;
        }
      }
    }
    for (i = 0; i < makefile->nASN1Modules; i++) {
      const struct module_struct *module = makefile->ASN1Modules + i;
      if (!module->is_regular || module->dir_name != NULL) {
        makefile->ASN1ModulesRegular = FALSE;
        break;
      }
    }
    for (i = 0; i < makefile->nUserFiles; i++) {
      const struct user_struct *user = makefile->UserFiles + i;
      if (!user->has_cc_suffix)
        makefile->UserSourcesRegular = FALSE;
      if (!user->has_cc_suffix || !user->has_hh_suffix)
        makefile->UserHeadersRegular = FALSE;
      if (!makefile->UserHeadersRegular && !makefile->UserSourcesRegular)
        break;
    }
  }
}

/** Prints the name of the TTCN-3 or ASN.1 source file that belongs to module
 * \a module to file \a fp. */
static void print_file_name(FILE *fp, const struct module_struct *module)
{
  char *path_name = compose_path_name(module->dir_name, module->file_name);
  fprintf(fp, " %s", path_name);
  Free(path_name);
}

/** Prints the name of the preprocessed TTCN-3 source file that belongs to
 *  module \a module to file \a fp. */
static void print_preprocessed_file_name(FILE *fp,
                                         const struct module_struct *module)
{
  char *preprocessed_name = get_preprocessed_file_name(module->file_name);
  char *path_name = compose_path_name(module->dir_name, preprocessed_name);
  fprintf(fp, " %s", path_name);
  Free(path_name);
  Free(preprocessed_name);
}

/** Prints the name of the generated TTCN-3, header, source or object file of module
 * \a module to file \a fp. The name of the directory is added only if
 * \a add_directory is TRUE. Parameter \a suffix shall be 'ttcn", "hh", "cc", "hpp", "cpp" or "o". */
static void print_generated_file_name(FILE *fp,
  const struct module_struct *module, boolean add_directory, const char *suffix)
{
  char *file_name = mcopystr(module->module_name);
  /* replace '-' with '_' */
  size_t i;
  for (i = 0; file_name[i] != '\0'; i++)
    if (file_name[i] == '-') file_name[i] = '_';
  /* append the suffix */
  file_name = mputprintf(file_name, "%s", suffix);
  /* add the directory name if necessary */
  if (add_directory) {
    char *path_name = compose_path_name(module->dir_name, file_name);
    Free(file_name);
    file_name = path_name;
  }
  fprintf(fp, " %s", file_name);
  Free(file_name);
}

/** Prints the name of the user C/C++ header file of user module \a user if the
 * above file exists. */
static void print_header_name(FILE *fp, const struct user_struct *user)
{
  if (user->header_name != NULL) {
    char *path_name = compose_path_name(user->dir_name, user->header_name);
    fprintf(fp, " %s", path_name);
    Free(path_name);
  }
}

/** Prints the name of the user C/C++ source file of user module \a user if the
 * above file exists. */
static void print_source_name(FILE *fp, const struct user_struct *user)
{
  if (user->source_name != NULL) {
    char *path_name = compose_path_name(user->dir_name, user->source_name);
    fprintf(fp, " %s", path_name);
    Free(path_name);
  }
}

/** Prints the name of the user C/C++ object file of user module \a user if the
 * above file exists (i.e. the respective source file is present). */
static void print_object_name(FILE *fp, const struct user_struct *user)
{
  if (user->source_name != NULL) {
    char *file_name = mprintf("%s.o", user->file_prefix);
    char *path_name = compose_path_name(user->dir_name, file_name);
    Free(file_name);
    fprintf(fp, " %s", path_name);
    Free(path_name);
  }
}

static void print_shared_object_name(FILE *fp, const struct user_struct *user)
{
  if (user->source_name != NULL) {
    char *file_name = mprintf("%s.so", user->file_prefix);
    char *path_name = compose_path_name(user->dir_name, file_name);
    Free(file_name);
    fprintf(fp, " %s", path_name);
    Free(path_name);
  }
}
/** Prints the splitted files' names for a given module. */
static void print_splitted_file_names(FILE *fp,
  const struct makefile_struct *makefile, const struct module_struct *module, const boolean dir)
{
  int n_slices;
  if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
    print_generated_file_name(fp, module, dir, "_seq.cc");
    print_generated_file_name(fp, module, dir, "_set.cc");
    print_generated_file_name(fp, module, dir, "_seqof.cc");
    print_generated_file_name(fp, module, dir, "_setof.cc");
    print_generated_file_name(fp, module, dir, "_union.cc");
  } else if(makefile->code_splitting_mode != NULL && (n_slices = atoi(makefile->code_splitting_mode + 2))) {
    for (int i = 1; i < n_slices; i++) {
      char buffer[16]; // 6 digits + 4 chars + _part
      sprintf(buffer, "_part_%i.cc", i);
      print_generated_file_name(fp, module, dir, buffer);
    }
  }
}

static void fprint_extra_targets(FILE* fp, struct string2_list* target_placement_list, const char* placement)
{
  struct string2_list* act_elem = target_placement_list;
  while (act_elem) {
    if (act_elem->str1 && act_elem->str2 && (strcmp(act_elem->str2,placement)==0)) {
      fprintf(fp, " %s", act_elem->str1);
    }
    act_elem = act_elem->next;
  }
}

#undef COMMENT_PREFIX
#define COMMENT_PREFIX "# "

/** Prints the Makefile based on structure \a makefile. */
static void print_makefile(struct makefile_struct *makefile)
{
  boolean add_refd_prjs = FALSE;
  if (makefile->linkingStrategy && makefile->hierarchical) {
    add_refd_prjs = hasSubProject(makefile->project_name);
  }
  else {
    add_refd_prjs = makefile->sub_project_dirs && makefile->sub_project_dirs->str;
  }
  NOTIFY("Generating Makefile skeleton...");

  if (makefile->force_overwrite ||
      get_path_status(makefile->output_file) == PS_NONEXISTENT) {
    size_t i;
    char *user_info;
    const char* cxx;
    const char *rm_command = makefile->gnu_make ? "$(RM)" : "rm -f";
    FILE *fp;
    boolean run_compiler = (makefile->nASN1Modules > 0)
      || (makefile->nTTCN3Modules) || (makefile->nTTCN3PPModules > 0)
      || (makefile->nXSDModules > 0);

    expstring_t titan_dir = 0;
    const char * last_slash = strrchr(program_name, '/');
    if (last_slash != NULL) {
      size_t path_len = last_slash - program_name;
      titan_dir = mcopystr(program_name);
      /* Chop off the program name, and the /bin before it (if any) */
      if (path_len >= 4
        && memcmp(titan_dir + path_len - 4, "/bin", 4) == 0) {
        titan_dir = mtruncstr(titan_dir, path_len - 4);
      }
      else {
        titan_dir = mtruncstr(titan_dir, path_len);
      }
    }


    fp = fopen(makefile->output_file, "w");
    if (fp == NULL){
      ERROR("Cannot open output file `%s' for writing: %s",
      makefile->output_file, strerror(errno));
      return;
    }
    user_info = get_user_info();
    fprintf(fp, "# This Makefile was generated by the Makefile Generator\n"
      "# of the TTCN-3 Test Executor version " PRODUCT_NUMBER "\n"
      "# for %s\n"
      COPYRIGHT_STRING "\n\n"
      "# The following make commands are available:\n"
      "# - make, make all      Builds the %s.\n"
      "# - make archive        Archives all source files.\n"
      "# - make check          Checks the semantics of TTCN-3 and ASN.1"
      "modules.\n"       
      "# - make port           Generates port skeletons.\n"
      "%s" // clean:
      "%s" //clean-all
      "# - make compile        Translates TTCN-3 and ASN.1 modules to C++.\n"
      "# - make dep            Creates/updates dependency list.\n"
      "# - make executable     Builds the executable test suite.\n"
      "# - make library        Builds the library archive.\n"
      "# - make objects        Builds the object files without linking the "
      "executable.\n", user_info,
      makefile->library ? "library archive." : "executable test suite",
      (makefile->linkingStrategy && makefile->hierarchical) ?
      "# - make clean          Removes generated files from project.\n" :
      "# - make clean          Removes all generated files.\n",
      (makefile->linkingStrategy && makefile->hierarchical) ?
      "# - make clean-all      Removes all generated files from the project hierarchy.\n" : "");
    Free(user_info);
    if (makefile->dynamic)
      fprintf(fp, "# - make shared_objects Builds the shared object files "
              "without linking the executable.\n");
    if (makefile->preprocess)
      fputs("# - make preprocess     Preprocess TTCN-3 files.\n", fp);
    if (makefile->central_storage) {
      fputs("# WARNING! This Makefile uses pre-compiled files from the "
            "following directories:\n", fp);
      for (i = 0; i < makefile->nBaseDirs; i++)
        fprintf(fp, "# %s\n", makefile->BaseDirs[i].dir_name);
      fputs("# The executable tests will be consistent only if all directories "
            "use\n"
            "# the same platform and the same version of TTCN-3 Test Executor "
            "and\n"
            "# C++ compiler with the same command line switches.\n\n", fp);
    }
    if (makefile->gnu_make) {
      fputs("# WARNING! This Makefile can be used with GNU make only.\n"
            "# Other versions of make may report syntax errors in it.\n\n"
            "#\n"
            "# Do NOT touch this line...\n"
            "#\n"
            ".PHONY: all shared_objects executable library objects check port clean dep archive", fp);
      if (makefile->preprocess) fputs(" preprocess", fp);
      if (add_refd_prjs) {
        fprintf(fp, "\\\n referenced-all referenced-shared_objects referenced-executable referenced-library referenced-objects referenced-check"
              "\\\n referenced-clean%s",
              (makefile->linkingStrategy && makefile->hierarchical) ?
              "-all" : "");
      }
      fprint_extra_targets(fp, makefile->target_placement_list, "PHONY");

      if (makefile->gcc_dep) {
        fputs("\n\n.SUFFIXES: .d", fp);
      }
      fputs("\n\n", fp);
    }

    if (makefile->linkingStrategy) {
      const char* tpd_name = getTPDFileName(makefile->project_name);
      if (tpd_name) {
        fputs("# Titan Project Descriptor file what this Makefile is generated from.\n", fp);
        fprintf(fp, "TPD = %s\n\n", tpd_name);
      }
      const char* root_dir = getPathToRootDir(makefile->project_name);
      if (root_dir) {
        fputs("# Relative path to top directory at OS level.\n", fp);
        fprintf(fp, "ROOT_DIR = %s\n\n", root_dir);
      }
    }

    if (add_refd_prjs) {
      struct string_list* act_elem = NULL;
      struct string_list* head = NULL;
      if (makefile->linkingStrategy && makefile->hierarchical) {// pair with free_string_list
        head = act_elem = getRefWorkingDirs(makefile->project_name);
      }
      else {
        act_elem = makefile->sub_project_dirs;
      }
      if (!makefile->linkingStrategy)
        fputs("# This is the top level makefile of a Makefile hierarchy generated from\n", fp);
      fputs("# Titan Project Descriptor hierarchy. List of referenced project\n"
            "# working directories (ordered by dependencies):\n", fp);
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, "# %s\n", act_elem->str);
        }
        act_elem = act_elem->next;
      }
      if (makefile->linkingStrategy && makefile->hierarchical) { // pair with getRefWorkingDirs
        free_string_list(head);
      }
      fputs("REFERENCED_PROJECT_DIRS = ", fp);
      if (makefile->linkingStrategy && makefile->hierarchical) {
        head = act_elem = getRefWorkingDirs(makefile->project_name); // pair with free_string_list
      }
      else {
        act_elem = makefile->sub_project_dirs;
      }
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, "%s ", act_elem->str);
        }
        act_elem = act_elem->next;
      }
      fputs("\n\n", fp);
      if (makefile->linkingStrategy && makefile->hierarchical) {// pair with getRefWorkingDirs
        free_string_list(head);
      }
    }

    fprintf(fp, "#\n"
          "# Set these variables...\n"
          "#\n\n"
          "# The path of your TTCN-3 Test Executor installation:\n"
          "# Uncomment this line to override the environment variable.\n"
          "%s"
          "# TTCN3_DIR = %s\n"
      , titan_dir ?
        "# The value below points to the location of the TITAN version\n"
        "# that generated this makefile.\n" : ""
      , titan_dir ? titan_dir : "");
    if (titan_dir) Free(titan_dir);

    boolean cxx_free = FALSE;
    if (makefile->cxxcompiler) {
      cxx = makefile->cxxcompiler;
    } else {
#ifdef __clang__
      unsigned int
        compiler_major = __clang_major__,
        compiler_minor = __clang_minor__;
      cxx = mprintf("clang++-%u.%u", compiler_major, compiler_minor);
      cxx_free = TRUE;
#else
      cxx = "g++";
#endif
    }

    fprintf(fp, "\n# Your platform: (SOLARIS, SOLARIS8, LINUX, FREEBSD or "
      "WIN32)\n"
      "PLATFORM = %s\n\n"
      "# Your C++ compiler:\n"
      "# (if you change the platform, you may need to change the compiler)\n"
      "CXX = %s \n\n", get_platform_string(), cxx);


    if (makefile->preprocess || makefile->ttcn3preprocessor) {
      const char* cpp;
      if (makefile->ttcn3preprocessor) {
        cpp = makefile->ttcn3preprocessor;
      } else {
        cpp = "cpp";
      }
      fprintf(fp,"# C preprocessor used for TTCN-3 files:\n"
          "CPP = %s\n\n", cpp);
    }

    fputs("# Flags for the C++ preprocessor (and makedepend as well):\n"
          "CPPFLAGS = -D$(PLATFORM) -I$(TTCN3_DIR)/include", fp);

#ifdef MEMORY_DEBUG
    // enable debug mode for the generated code, too
    fputs(" -DMEMORY_DEBUG", fp);
#endif

    if (makefile->use_runtime_2) fputs(" -DTITAN_RUNTIME_2", fp);

    for (i = 0; i < makefile->nBaseDirs; i++) {
      fprintf(fp, " -I%s", makefile->BaseDirs[i].dir_name);
    }

    if (makefile->prep_includes) {
      struct string_list* act_elem = makefile->prep_includes;
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, " -I%s", act_elem->str);
        }
        act_elem = act_elem->next;
      }
    }

    if (makefile->prep_defines) {
      struct string_list* act_elem = makefile->prep_defines;
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, " -D%s", act_elem->str);
        }
        act_elem = act_elem->next;
      }
    }

    if (makefile->prep_undefines) {
      struct string_list* act_elem = makefile->prep_undefines;
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, " -U%s", act_elem->str);
        }
        act_elem = act_elem->next;
      }
    }

    fputs("\n\n", fp);

    if (makefile->gcc_dep) {
      fprintf(fp, "# Flags for dependency generation\n"
        "CXXDEPFLAGS = -%s\n\n", strstr(cxx, "g++") ? "MM" : "xM1");
    }

    if (cxx_free) {
      Free((char*)cxx);
      cxx = NULL;
    }
    
    if (makefile->preprocess || makefile->ttcn3_prep_includes ||  makefile->ttcn3_prep_defines) {
      fputs("# Flags for preprocessing TTCN-3 files:\n"
            "CPPFLAGS_TTCN3 =", fp);

      if (makefile->preprocess) {
        for (i = 0; i < makefile->nBaseDirs; i++) {
          fprintf(fp, " -I%s", makefile->BaseDirs[i].dir_name);
        }
      }
      if (makefile->ttcn3_prep_includes) {
        struct string_list* act_elem = makefile->ttcn3_prep_includes;
        while (act_elem) {
          if (act_elem->str) {
            fprintf(fp, " -I%s", act_elem->str);
          }
          act_elem = act_elem->next;
        }
      }
      if (makefile->ttcn3_prep_defines) {
        struct string_list* act_elem = makefile->ttcn3_prep_defines;
        while (act_elem) {
          if (act_elem->str) {
            fprintf(fp, " -D%s", act_elem->str);
          }
          act_elem = act_elem->next;
        }
      }
      if (makefile->ttcn3_prep_undefines) {
        struct string_list* act_elem = makefile->ttcn3_prep_undefines;
        while (act_elem) {
          if (act_elem->str) {
            fprintf(fp, " -U%s", act_elem->str);
          }
          act_elem = act_elem->next;
        }
      }
      fputs("\n\n", fp);
    }

    fprintf(fp, "# Flags for the C++ compiler:\n"
          "CXXFLAGS = %s%s %s %s"
#ifdef MEMORY_DEBUG
          " -g" // enable debug information for the generated code
#endif
          "\n\n"
          "# Flags for the linker:\n"
          "LDFLAGS = %s%s\n\n"
          "ifeq ($(PLATFORM), WIN32)\n"
          "# Silence linker warnings.\n"
          "LDFLAGS += -Wl,--enable-auto-import,--enable-runtime-pseudo-reloc\n"
          "endif\n\n"
          "# Utility to create library files\n"
          "AR = ar\n"
          "ARFLAGS = \n\n"
          "# Flags for the TTCN-3 and ASN.1 compiler:\n"
          "COMPILER_FLAGS =%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n\n"
          "# Execution mode: (either ttcn3 or ttcn3-parallel)\n"
          "TTCN3_LIB = ttcn3%s%s%s\n\n"
#ifdef LICENSE
          "# The path of your OpenSSL installation:\n"
          "# If you do not have your own one, leave it unchanged.\n"
          "%sOPENSSL_DIR = $(TTCN3_DIR)\n\n"
#endif
          "# The path of your libxml2 installation:\n"
          "# If you do not have your own one, leave it unchanged.\n"
          "%sXMLDIR = $(TTCN3_DIR)\n\n"
          "# Directory to store the archived source files:\n",
          makefile->dynamic ? "-Wall -fPIC" : "-Wall", /* CXXFLAGS */
          makefile->coverage ? " -fprofile-arcs -ftest-coverage -g" : "", /* CXXFLAGS COVERAGE */
          makefile->optlevel ? makefile->optlevel : "", /* CXXFLAGS optimization level */
          makefile->optflags ? makefile->optflags : "", /* CXXFLAGS optimization level */
          makefile->dynamic ? "-fPIC" : "", /* LDFLAGS */
          makefile->coverage ? " -fprofile-arcs -ftest-coverage -g -lgcov" : "", /* LDFLAGS COVERAGE */
          /* COMPILER_FLAGS */
          makefile->use_runtime_2 ? " -L -R " : " -L ",
          (makefile->code_splitting_mode ? makefile->code_splitting_mode : ""),
          (makefile->quietly ? " -q" : ""),
          (makefile->disablesubtypecheck ? " -y" : ""),
          (makefile->disableber ? " -b" : ""),
          (makefile->disableraw ? " -r" : ""),
          (makefile->disabletext ? " -x" : ""),
          (makefile->disablexer ? " -X" : ""),
          (makefile->disablejson ? " -j" : ""),
          (makefile->forcexerinasn ? " -a" : ""),
          (makefile->defaultasomit ? " -d" : ""),
          (makefile->gccmsgformat ? " -g" : ""),
          (makefile->linenumbersonlymsg ? " -i" : ""),
          (makefile->includesourceinfo ? " -l" : ""),
          /*(makefile->addsourcelineinfo ? " -L" : ""),*/
          (makefile->suppresswarnings ? " -w" : ""),
          (makefile->outparamboundness ? " -Y" : ""),
          (makefile->omit_in_value_list ? " -M" : ""),
          (makefile->warnings_for_bad_variants ? " -E" : ""),
          (makefile->activate_debugger ? " -n" : ""),
          (makefile->ignore_untagged_on_top_union ? " -N" : ""),
          (makefile->tcov_file_name ? makefile->tcov_file_name : ""),
          (makefile->profiled_file_list ? " -z $(PROFILED_FILE_LIST)" : ""),
          /* end of COMPILER FLAGS */
          (makefile->use_runtime_2 ? "-rt2"    : ""), /* TTCN3_LIB */
          (makefile->single_mode   ? ""        : "-parallel"),
          (makefile->dynamic       ? "-dynamic": ""),
#ifdef LICENSE
          (makefile->disable_predef_ext_folder ? "# " : ""),
#endif 
          (makefile->disable_predef_ext_folder ? "# " : "")
          );
    if (!makefile->gnu_make) {
      fputs("# Note: you can set any directory except ./archive\n", fp);
    }
    fputs("ARCHIVE_DIR = backup\n\n"
          "#\n"
          "# You may change these variables. Add your files if necessary...\n"
          "#\n\n"
          "# TTCN-3 modules of this project:\n"
          "TTCN3_MODULES =", fp);
    for (i = 0; i < makefile->nTTCN3Modules; i++) {
      const struct module_struct *module = makefile->TTCN3Modules + i;
      if (module->dir_name == NULL || !makefile->central_storage)
        /* If the file is in the current directory or
         * is not in the current directory but central directory is not used,
         * it goes into TTCN3_MODULES */
        print_file_name(fp, module);
    }
    fprint_extra_targets(fp, makefile->target_placement_list, "TTCN3_MODULES");
    if (makefile->nXSDModules) {
      fputs("\n\nXSD_MODULES =", fp);
      for (i = 0; i < makefile->nXSDModules; i++) {
        const struct module_struct *module = makefile->XSDModules + i;
        if (module->file_name != NULL && (module->dir_name == NULL || !makefile->central_storage)) {
          /* If the file is in the current directory or
           * is not in the current directory but central directory is not used,
           * it goes into XSD_MODULES */
          print_file_name(fp, module);
        }
      }
      fprint_extra_targets(fp, makefile->target_placement_list, "XSD_MODULES");
    }
    if (makefile->preprocess) {
      fputs("\n\n"
      "# TTCN-3 modules to preprocess:\n"
            "TTCN3_PP_MODULES =", fp);
      for (i = 0; i < makefile->nTTCN3PPModules; i++) {
        const struct module_struct *module = makefile->TTCN3PPModules + i;
        if (module->dir_name == NULL || !makefile->central_storage)
          print_file_name(fp, module);
      }
      fprint_extra_targets(fp, makefile->target_placement_list, "TTCN3_PP_MODULES");
    }
    if (makefile->central_storage) {
      fputs("\n\n"
      "# TTCN-3 modules used from central project(s):\n"
      "BASE_TTCN3_MODULES =", fp);
      if (!makefile->linkingStrategy) {
        for (i = 0; i < makefile->nTTCN3Modules; i++) {
          const struct module_struct *module = makefile->TTCN3Modules + i;
          /* Central storage used AND file is not in the current directory => it goes into BASE_TTCN3_MODULES */
          if (module->dir_name != NULL) print_file_name(fp, module);
        }
        if (makefile->preprocess) {
          fputs("\n\n"
          "# TTCN-3 modules to preprocess used from central project(s):\n"
          "BASE_TTCN3_PP_MODULES =", fp);
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name != NULL && !isTtcnPPFileInLibrary(module->file_name))
              print_file_name(fp, module);
          }
        }
        if (makefile->nXSDModules) {
          fputs("\n\n"
          "# XSD modules used from central project(s):\n"
          "BASE_XSD_MODULES =", fp);
          for (i = 0; i < makefile->nXSDModules; i++) {
            const struct module_struct *module = makefile->XSDModules + i;
            /* Central storage used AND file is not in the current directory => it goes into BASE_XSD_MODULES */
            if (module->dir_name != NULL) print_file_name(fp, module);
          }
        }
      }
      else { // new linking strategy
        for (i = 0; i < makefile->nTTCN3Modules; i++) {
          const struct module_struct *module = makefile->TTCN3Modules + i;
          /* Central storage used AND file is not in the current directory => it goes into BASE_TTCN3_MODULES */
          if (module->dir_name != NULL && !isTtcn3ModuleInLibrary(module->module_name))
            print_file_name(fp, module);
        }
        fputs("\n\n"
        "# TTCN-3 library linked modules used from central project(s):\n"
        "BASE2_TTCN3_MODULES =", fp);
        for (i = 0; i < makefile->nTTCN3Modules; i++) {
          const struct module_struct *module = makefile->TTCN3Modules + i;
          /* Central storage used AND file is not in the current directory => it goes into BASE_TTCN3_MODULES */
          if (module->dir_name != NULL && isTtcn3ModuleInLibrary(module->module_name))
            print_file_name(fp, module);
        }
        if (makefile->preprocess) {
          fputs("\n\n"
          "# TTCN-3 modules to preprocess used from central project(s):\n"
          "BASE_TTCN3_PP_MODULES =", fp);
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name != NULL && !isTtcnPPFileInLibrary(module->file_name))
              print_file_name(fp, module);
          }
          fputs("\n\n"
          "# TTCN-3 library linked modules to preprocess used from central project(s):\n"
          "BASE2_TTCN3_PP_MODULES =", fp);
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name != NULL && isTtcnPPFileInLibrary(module->file_name))
              print_file_name(fp, module);
          }
        }
        if (makefile->nXSDModules) {
          fputs("\n\n"
          "# XSD modules used from central project(s):\n"
          "BASE_XSD_MODULES =", fp);
          for (i = 0; i < makefile->nXSDModules; i++) {
            const struct module_struct *module = makefile->XSDModules + i;
            /* Central storage used AND file is not in the current directory => it goes into BASE_XSD_MODULES */
            if (module->dir_name != NULL && !isXSDModuleInLibrary(module->module_name))
              print_file_name(fp, module);
          }
        }
        if (makefile->nXSDModules) {
          fputs("\n\n"
          "# XSD library linked modules used from central project(s):\n"
          "BASE2_XSD_MODULES =", fp);
          for (i = 0; i < makefile->nXSDModules; i++) {
            const struct module_struct *module = makefile->XSDModules + i;
            /* Central storage used AND file is not in the current directory => it goes into BASE_TTCN3_MODULES */
            if (module->dir_name != NULL && isXSDModuleInLibrary(module->module_name))
              print_file_name(fp, module);
          }
        }
      }
    }
    if (makefile->preprocess) {
      fputs("\n\n"
            "# Files to include in TTCN-3 preprocessed modules:\n"
            "TTCN3_INCLUDES =", fp);
      for (i = 0; i < makefile->nTTCN3IncludeFiles; i++)
        fprintf(fp, " %s", makefile->TTCN3IncludeFiles[i]);
      fprint_extra_targets(fp, makefile->target_placement_list, "TTCN3_INCLUDES");
    }
    fputs("\n\n"
          "# ASN.1 modules of this project:\n"
          "ASN1_MODULES =", fp);
    for (i = 0; i < makefile->nASN1Modules; i++) {
      const struct module_struct *module = makefile->ASN1Modules + i;
      if (module->dir_name == NULL || !makefile->central_storage)
        print_file_name(fp, module);
    }
    fprint_extra_targets(fp, makefile->target_placement_list, "ASN1_MODULES");
    if (makefile->central_storage) {
      fputs("\n\n"
      "# ASN.1 modules used from central project(s):\n"
      "BASE_ASN1_MODULES =", fp);
      if (!makefile->linkingStrategy) {
        for (i = 0; i < makefile->nASN1Modules; i++) {
          const struct module_struct *module = makefile->ASN1Modules + i;
          if (module->dir_name != NULL) print_file_name(fp, module);
        }
      }
      else {
        for (i = 0; i < makefile->nASN1Modules; i++) {
          const struct module_struct *module = makefile->ASN1Modules + i;
          if (module->dir_name != NULL  && !isAsn1ModuleInLibrary(module->module_name))
            print_file_name(fp, module);
        }
        fputs("\n\n"
        "# ASN.1 library linked modules used from central project(s):\n"
        "BASE2_ASN1_MODULES =", fp);
        for (i = 0; i < makefile->nASN1Modules; i++) {
          const struct module_struct *module = makefile->ASN1Modules + i;
          if (module->dir_name != NULL  && isAsn1ModuleInLibrary(module->module_name))
            print_file_name(fp, module);
        }
      }
    }
    if (makefile->preprocess) {
      fputs("\n\n"
            "# TTCN-3 source files generated by the C preprocessor:\n"
            "PREPROCESSED_TTCN3_MODULES =", fp);
      for (i = 0; i < makefile->nTTCN3PPModules; i++) {
        const struct module_struct *module = makefile->TTCN3PPModules + i;
        if (module->dir_name == NULL || !makefile->central_storage)
          print_preprocessed_file_name(fp, module);
      }
      if (makefile->central_storage) {
        fputs("\n\n"
        "# TTCN-3 files generated by the CPP used from central project(s):\n"
        "BASE_PREPROCESSED_TTCN3_MODULES =", fp);
        if (!makefile->linkingStrategy) {
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name != NULL)
              print_preprocessed_file_name(fp, module);
          }
        }
        else { // new linking strategy
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name != NULL && !isTtcnPPFileInLibrary(module->file_name))
              print_preprocessed_file_name(fp, module);
          }
          fputs("\n\n"
          "# TTCN-3 library linked files generated by the CPP used from central project(s):\n"
          "BASE2_PREPROCESSED_TTCN3_MODULES =", fp);
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name != NULL && isTtcnPPFileInLibrary(module->file_name))
              print_preprocessed_file_name(fp, module);
          }
        }
      }
    }
    if (makefile->profiled_file_list) {
      if (makefile->profiled_file_list->next && !makefile->central_storage) {
        // merge all profiled file lists into one list
        fprintf(fp, "\n\n"
                "# Text file containing the list of profiled TTCN-3 files of "
                "this project:\n"
                "PROFILED_FILE_LIST = %s.merged\n"
                "PROFILED_FILE_LIST_SEGMENTS =",
                makefile->profiled_file_list->str);
        struct string_list* iter = makefile->profiled_file_list;
        while(iter != NULL) {
          fprintf(fp, " %s", iter->str);
          iter = iter->next;
        }
      }
      else {
        // only one profiled file list is needed
        fprintf(fp, "\n\n"
                "# Text file containing the list of profiled TTCN-3 files of "
                "this project:\n"
                "PROFILED_FILE_LIST = %s", makefile->profiled_file_list->str);
      }
    }
    
    boolean has_xsd_module = FALSE;
    boolean has_base_xsd_module = FALSE;
    if (makefile->nXSDModules) {
      fputs("\n\n"
             "XSD2TTCN_GENERATED_MODULES =", fp);
      for (i = 0; i < makefile->nXSDModules; i++) {
        const struct module_struct *module = makefile->XSDModules + i;
        if (module->dir_name == NULL || !makefile->central_storage) {
          if (strcmp(module->module_name, "XSD") != 0 &&
              strcmp(module->module_name, "UsefulTtcn3Types") != 0) {
            has_xsd_module = TRUE;
          }
        }
        if (module->dir_name != NULL && makefile->central_storage) {
          has_base_xsd_module = TRUE;
        }
      }

      for (i = 0; i < makefile->nXSDModules; i++) {
        const struct module_struct *module = makefile->XSDModules + i;
        if (module->dir_name == NULL || !makefile->central_storage)
          /* If the file is in the current directory or
           * is not in the current directory but central directory is not used,
           * it goes into XSD_MODULES */
          print_generated_file_name(fp, module, FALSE, ".ttcn");
      }
      fputs("\n\nTTCN3_MODULES += $(XSD2TTCN_GENERATED_MODULES)", fp);
      if (makefile->central_storage) {
        fputs("\n\n"
              "BASE_XSD2TTCN_GENERATED_MODULES =", fp);
        for (i = 0; i < makefile->nXSDModules; i++) {
          const struct module_struct *module = makefile->XSDModules + i;
          if (module->dir_name != NULL && !isXSDModuleInLibrary(module->module_name))
            print_generated_file_name(fp, module, TRUE, ".ttcn");
        }
        fputs("\n\nBASE_TTCN3_MODULES += $(BASE_XSD2TTCN_GENERATED_MODULES)", fp);
        if (makefile->linkingStrategy) {
          fputs("\n\n"
              "BASE2_XSD2TTCN_GENERATED_MODULES =", fp);
        for (i = 0; i < makefile->nXSDModules; i++) {
          const struct module_struct *module = makefile->XSDModules + i;
          if (module->dir_name != NULL && isXSDModuleInLibrary(module->module_name))
            print_generated_file_name(fp, module, TRUE, ".ttcn");
        }
        fputs("\n\nBASE2_TTCN3_MODULES += $(BASE2_XSD2TTCN_GENERATED_MODULES)", fp);
        }
      }
    }
    fputs("\n\n"
          "# C++ source & header files generated from the TTCN-3 & ASN.1 "
          "modules of\n"
          "# this project:\n"
          "GENERATED_SOURCES =", fp);
    if (makefile->gnu_make && ((makefile->TTCN3ModulesRegular) || (!makefile->nTTCN3Modules && makefile->nXSDModules))) {
      fputs(" $(TTCN3_MODULES:.ttcn=.cc)", fp);
      if (makefile->code_splitting_mode) {
        for (i = 0; i < makefile->nTTCN3Modules; i++) {
          const struct module_struct *module = makefile->TTCN3Modules + i;
          if (module->dir_name == NULL || !makefile->central_storage)
            print_splitted_file_names(fp, makefile, module, FALSE);
        }
      }
      if (makefile->preprocess) {
        fputs(" $(TTCN3_PP_MODULES:.ttcnpp=.cc)", fp);
        if (makefile->code_splitting_mode) {
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name == NULL || !makefile->central_storage)
              print_splitted_file_names(fp, makefile, module, FALSE);
          }
        }
      }
      if (makefile->nXSDModules) {
        for (i = 0; i < makefile->nXSDModules; i++) {
          const struct module_struct *module = makefile->XSDModules + i;
          if (module->dir_name == NULL || !makefile->central_storage) {
            if (makefile->code_splitting_mode)
              print_splitted_file_names(fp, makefile, module, FALSE);
          }
        }
      }
    } else {
      for (i = 0; i < makefile->nTTCN3Modules; i++) {
        const struct module_struct *module = makefile->TTCN3Modules + i;
        if (module->dir_name == NULL || !makefile->central_storage) {
          print_generated_file_name(fp, module, FALSE, ".cc");
          if (makefile->code_splitting_mode)
            print_splitted_file_names(fp, makefile, module, FALSE);
        }
      }
      if (makefile->preprocess) {
        for (i = 0; i < makefile->nTTCN3PPModules; i++) {
          const struct module_struct *module = makefile->TTCN3PPModules + i;
          if (module->dir_name == NULL || !makefile->central_storage) {
            print_generated_file_name(fp, module, FALSE, ".cc");
            if (makefile->code_splitting_mode)
              print_splitted_file_names(fp, makefile, module, FALSE);
          }
        }
      }
      if (makefile->nXSDModules) {
        for (i = 0; i < makefile->nXSDModules; i++) {
          const struct module_struct *module = makefile->XSDModules + i;
          if (module->dir_name == NULL || !makefile->central_storage) {
            print_generated_file_name(fp, module, FALSE, ".cc");
            if (makefile->code_splitting_mode)
              print_splitted_file_names(fp, makefile, module, FALSE);
          }
        }
      }
    }
    if (makefile->gnu_make && makefile->ASN1ModulesRegular) {
      fputs(" $(ASN1_MODULES:.asn=.cc)", fp);
      if (makefile->code_splitting_mode) {
        for (i = 0; i < makefile->nASN1Modules; i++) {
          const struct module_struct *module = makefile->ASN1Modules + i;
          if (module->dir_name == NULL || !makefile->central_storage) {
            print_splitted_file_names(fp, makefile, module, FALSE);
          }
        }
      }
    } else {
      for (i = 0; i < makefile->nASN1Modules; i++) {
        const struct module_struct *module = makefile->ASN1Modules + i;
        if (module->dir_name == NULL || !makefile->central_storage) {
          print_generated_file_name(fp, module, FALSE, ".cc");
          if (makefile->code_splitting_mode)
            print_splitted_file_names(fp, makefile, module, FALSE);
        }
      }
    }    

    fputs("\nGENERATED_HEADERS =", fp);
    if (makefile->gnu_make) {
      int n_slices;
      // If GNU make and split to slices code splitting set, then if we would
      // use the .cc=.hh then the _part_i.hh header files would be printed into
      // the makefile that would cause weird behavior.
      if (makefile->code_splitting_mode != NULL && (n_slices = atoi(makefile->code_splitting_mode + 2))) {
        if (makefile->TTCN3ModulesRegular) {
          fputs(" $(TTCN3_MODULES:.ttcn=.hh)", fp);
          if (makefile->preprocess) {
            fputs(" $(TTCN3_PP_MODULES:.ttcnpp=.hh)", fp);
          }
        } else {
          for (i = 0; i < makefile->nTTCN3Modules; i++) {
            const struct module_struct *module = makefile->TTCN3Modules + i;
            if (module->dir_name == NULL || !makefile->central_storage)
              print_generated_file_name(fp, module, FALSE, ".hh");
          }
          if (makefile->preprocess) {
            for (i = 0; i < makefile->nTTCN3PPModules; i++) {
              const struct module_struct *module = makefile->TTCN3PPModules + i;
              if (module->dir_name == NULL || !makefile->central_storage)
                print_generated_file_name(fp, module, FALSE, ".hh");
            }
          }
          if (makefile->nXSDModules) {
            for (i = 0; i < makefile->nXSDModules; i++) {
              const struct module_struct *module = makefile->XSDModules + i;
              if (module->dir_name == NULL || !makefile->central_storage)
                print_generated_file_name(fp, module, FALSE, ".hh");
            }
          }
        }
        if (makefile->ASN1ModulesRegular) {
          fputs(" $(ASN1_MODULES:.asn=.hh)", fp);
        } else {
          for (i = 0; i < makefile->nASN1Modules; i++) {
            const struct module_struct *module = makefile->ASN1Modules + i;
            if (module->dir_name == NULL || !makefile->central_storage)
              print_generated_file_name(fp, module, FALSE, ".hh");
          }
        }
      } else {
        // Generate normally
        fputs(" $(GENERATED_SOURCES:.cc=.hh)", fp);
      }
    }
    else {
      for (i = 0; i < makefile->nTTCN3Modules; i++) {
        const struct module_struct *module = makefile->TTCN3Modules + i;
        if (module->dir_name == NULL || !makefile->central_storage)
          print_generated_file_name(fp, module, FALSE, ".hh");
      }
      if (makefile->preprocess) {
        for (i = 0; i < makefile->nTTCN3PPModules; i++) {
          const struct module_struct *module = makefile->TTCN3PPModules + i;
          if (module->dir_name == NULL || !makefile->central_storage)
            print_generated_file_name(fp, module, FALSE, ".hh");
        }
      }
      for (i = 0; i < makefile->nASN1Modules; i++) {
        const struct module_struct *module = makefile->ASN1Modules + i;
        if (module->dir_name == NULL || !makefile->central_storage)
          print_generated_file_name(fp, module, FALSE, ".hh");
      }
      if (makefile->nXSDModules) {
        for (i = 0; i < makefile->nXSDModules; i++) {
          const struct module_struct *module = makefile->XSDModules + i;
          if (module->dir_name == NULL || !makefile->central_storage)
            print_generated_file_name(fp, module, FALSE, ".hh");
        }
      }
    }
    if (makefile->central_storage) {
      fputs("\n\n"
            "# C++ source & header files generated from the TTCN-3 & ASN.1 "
            "modules of\n"
            "# central project(s):\n"
            "BASE_GENERATED_SOURCES =", fp);
      if (makefile->gnu_make && ((makefile->BaseTTCN3ModulesRegular) || (!makefile->BaseTTCN3ModulesRegular && makefile->nXSDModules))) {
        fputs(" $(BASE_TTCN3_MODULES:.ttcn=.cc)", fp);
        if (makefile->code_splitting_mode) {
          for (i = 0; i < makefile->nTTCN3Modules; i++) {
            const struct module_struct *module = makefile->TTCN3Modules + i;
            if (module->dir_name != NULL) {
              print_splitted_file_names(fp, makefile, module, TRUE);
            }
          }
        }
        if (makefile->preprocess) {
          fputs(" $(BASE_TTCN3_PP_MODULES:.ttcnpp=.cc)", fp);
          if (makefile->code_splitting_mode) {
            for (i = 0; i < makefile->nTTCN3PPModules; i++) {
              const struct module_struct *module = makefile->TTCN3PPModules + i;
              if (module->dir_name != NULL) {
                print_splitted_file_names(fp, makefile, module, TRUE);
              }
            }
          }
        }
        if (makefile->code_splitting_mode) {
          for (i = 0; i < makefile->nXSDModules; i++) {
            const struct module_struct *module = makefile->XSDModules + i;
            if (module->dir_name != NULL) {
              print_splitted_file_names(fp, makefile, module, TRUE);
            }
          }
        }
      } else {
        for (i = 0; i < makefile->nTTCN3Modules; i++) {
          const struct module_struct *module = makefile->TTCN3Modules + i;
          if (module->dir_name != NULL) {
            print_generated_file_name(fp, module, TRUE, ".cc");
            if (makefile->code_splitting_mode)
              print_splitted_file_names(fp, makefile, module, TRUE);
          }
        }
        if (makefile->preprocess) {
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name != NULL) {
              print_generated_file_name(fp, module, TRUE, ".cc");
              if (makefile->code_splitting_mode)
                print_splitted_file_names(fp, makefile, module, TRUE);
            }
          }
        }
        for (i = 0; i < makefile->nXSDModules; i++) {
          const struct module_struct *module = makefile->XSDModules + i;
          if (module->dir_name != NULL) {
            print_generated_file_name(fp, module, TRUE, ".cc");
            if (makefile->code_splitting_mode)
              print_splitted_file_names(fp, makefile, module, TRUE);
          }
        }
      }
      if (makefile->gnu_make && makefile->BaseASN1ModulesRegular) {
        fputs(" $(BASE_ASN1_MODULES:.asn=.cc)", fp);
        if (makefile->code_splitting_mode) {
          for (i = 0; i < makefile->nASN1Modules; i++) {
            const struct module_struct *module = makefile->ASN1Modules + i;
            if (module->dir_name != NULL) {
              print_splitted_file_names(fp, makefile, module, TRUE);
            }
          }
        }
      } else {
        for (i = 0; i < makefile->nASN1Modules; i++) {
          const struct module_struct *module = makefile->ASN1Modules + i;
          if (module->dir_name != NULL) {
            print_generated_file_name(fp, module, TRUE, ".cc");
            if (makefile->code_splitting_mode)
              print_splitted_file_names(fp, makefile, module, TRUE);
          }
        }
      }
      fputs("\nBASE_GENERATED_HEADERS =", fp);
      if (makefile->gnu_make) {
        int n_slices;
        // If GNU make and split to slices code splitting set, then if we would
        // use the .cc=.hh then the _part_i.hh header files would be printed into
        // the makefile that would cause weird behavior.
        if (makefile->code_splitting_mode != NULL && (n_slices = atoi(makefile->code_splitting_mode + 2))) {
          if (makefile->TTCN3ModulesRegular) {
            fputs(" $(BASE_TTCN3_MODULES:.ttcn=.hh)", fp);
            if (makefile->preprocess) {
              fputs(" $(BASE_TTCN3_PP_MODULES:.ttcnpp=.hh)", fp);
            }
          } else {
            for (i = 0; i < makefile->nTTCN3Modules; i++) {
              const struct module_struct *module = makefile->TTCN3Modules + i;
              if (module->dir_name != NULL)
                print_generated_file_name(fp, module, TRUE, ".hh");
            }
            if (makefile->preprocess) {
              for (i = 0; i < makefile->nTTCN3PPModules; i++) {
                const struct module_struct *module = makefile->TTCN3PPModules + i;
                if (module->dir_name != NULL)
                  print_generated_file_name(fp, module, TRUE, ".hh");
              }
            }
            if (makefile->nXSDModules) {
              for (i = 0; i < makefile->nXSDModules; i++) {
                const struct module_struct *module = makefile->XSDModules + i;
                if (module->dir_name != NULL)
                  print_generated_file_name(fp, module, TRUE, ".hh");
              }
            }
          }
          if (makefile->ASN1ModulesRegular) {
            fputs(" $(BASE_ASN1_MODULES:.asn=.hh)", fp);
          } else {
            for (i = 0; i < makefile->nASN1Modules; i++) {
              const struct module_struct *module = makefile->ASN1Modules + i;
              if (module->dir_name != NULL)
                print_generated_file_name(fp, module, TRUE, ".hh");
            }
          }
        } else {
          // Generate normally
          fputs(" $(BASE_GENERATED_SOURCES:.cc=.hh)", fp);
        }
      } else {
        for (i = 0; i < makefile->nTTCN3Modules; i++) {
          const struct module_struct *module = makefile->TTCN3Modules + i;
          if (module->dir_name != NULL)
            print_generated_file_name(fp, module, TRUE, ".hh");
        }
        if (makefile->preprocess) {
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name != NULL)
              print_generated_file_name(fp, module, TRUE, ".hh");
          }
        }
        for (i = 0; i < makefile->nASN1Modules; i++) {
          const struct module_struct *module = makefile->ASN1Modules + i;
          if (module->dir_name != NULL)
            print_generated_file_name(fp, module, TRUE, ".hh");
        }
        for (i = 0; i < makefile->nXSDModules; i++) {
          const struct module_struct *module = makefile->XSDModules + i;
          if (module->dir_name != NULL)
            print_generated_file_name(fp, module, TRUE, ".hh");
        }
      }
    }

    if (makefile->linkingStrategy) {
      fputs("\n\n"
            "# C++ source & header files generated from the TTCN-3 "
            " library linked modules of\n"
            "# central project(s):\n"
            "BASE2_GENERATED_SOURCES =", fp);
      if (makefile->gnu_make && makefile->BaseTTCN3ModulesRegular) {
        fputs(" $(BASE2_TTCN3_MODULES:.ttcn=.cc)", fp);
        fputs(" $(BASE2_ASN1_MODULES:.asn=.cc)", fp);
        if (makefile->preprocess)
          fputs(" $(BASE2_TTCN3_PP_MODULES:.ttcnpp=.cc)", fp);
      }
      else {
        for (i = 0; i < makefile->nTTCN3Modules; i++) {
          const struct module_struct *module = makefile->TTCN3Modules + i;
          if (module->dir_name != NULL && isTtcn3ModuleInLibrary(module->module_name)) {
            print_generated_file_name(fp, module, TRUE, ".cc");
          }
        }
        if (makefile->preprocess) {
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name != NULL && isTtcnPPFileInLibrary(module->file_name)) {
              print_generated_file_name(fp, module, TRUE, ".cc");
            }
          }
        }
        for (i = 0; i < makefile->nXSDModules; i++) {
          const struct module_struct *module = makefile->XSDModules + i;
          if (module->dir_name != NULL && isXSDModuleInLibrary(module->module_name)) {
            print_generated_file_name(fp, module, TRUE, ".cc");
          }
        }
      }

      fputs("\nBASE2_GENERATED_HEADERS =", fp);
      if (makefile->gnu_make) {
          fputs(" $(BASE2_GENERATED_SOURCES:.cc=.hh)", fp);
      }
      else
        ERROR("the usage of 'Z' flag requires GNU make");
    }

    fputs("\n\n"
          "# C/C++ Source & header files of Test Ports, external functions "
          "and\n"
          "# other modules:\n"
          "USER_SOURCES =", fp);
    for (i = 0; i < makefile->nUserFiles; i++) {
      const struct user_struct *user = makefile->UserFiles + i;
      if (user->dir_name == NULL || !makefile->central_storage)
        print_source_name(fp, user);
    }
    fprint_extra_targets(fp, makefile->target_placement_list, "USER_SOURCES");
    fputs("\nUSER_HEADERS =", fp);
    if (makefile->gnu_make && makefile->UserHeadersRegular) {
      fputs(" $(USER_SOURCES:.cc=.hh)", fp);
    } else {
      for (i = 0; i < makefile->nUserFiles; i++) {
        const struct user_struct *user = makefile->UserFiles + i;
        if (user->dir_name == NULL || !makefile->central_storage)
          print_header_name(fp, user);
      }
    }
    fprint_extra_targets(fp, makefile->target_placement_list, "USER_HEADERS");
    if (makefile->central_storage) {
      fputs("\n\n"
            "# C/C++ Source & header files of Test Ports, external functions "
            "and\n"
            "# other modules used from central project(s):\n"
            "BASE_USER_SOURCES =", fp);
      if (!makefile->linkingStrategy) {
        for (i = 0; i < makefile->nUserFiles; i++) {
          const struct user_struct *user = makefile->UserFiles + i;
          if (user->dir_name != NULL) {
            print_source_name(fp, user);
          }
        }
        fputs("\nBASE_USER_HEADERS =", fp);
        if (makefile->gnu_make && makefile->BaseUserHeadersRegular) {
          fputs(" $(BASE_USER_SOURCES:.cc=.hh)", fp);
        } 
        else {
          for (i = 0; i < makefile->nUserFiles; i++) {
            const struct user_struct *user = makefile->UserFiles + i;
            if (user->dir_name != NULL)
              print_header_name(fp, user);
          }
        }
      }
      else {
        for (i = 0; i < makefile->nUserFiles; i++) {
          const struct user_struct *user = makefile->UserFiles + i;
          if (user->dir_name != NULL && !isSourceFileInLibrary(user->source_name)) {
            print_source_name(fp, user);
          }
        }
        fputs("\nBASE_USER_HEADERS =", fp);
        if (makefile->gnu_make && makefile->BaseUserHeadersRegular) {
          fputs(" $(BASE_USER_SOURCES:.cc=.hh)", fp);
        }
        else {
          for (i = 0; i < makefile->nUserFiles; i++) {
            const struct user_struct *user = makefile->UserFiles + i;
            if (user->dir_name != NULL && !isHeaderFileInLibrary(user->header_name))
              print_header_name(fp, user);
          }
        }

        fputs("\n\n"
              "# C/C++ Source & header files of Test Ports, external functions "
              "and\n"
              "# other modules used from library linked central project(s):\n"
              "BASE2_USER_SOURCES =", fp);
        for (i = 0; i < makefile->nUserFiles; i++) {
          const struct user_struct *user = makefile->UserFiles + i;
          if (user->dir_name != NULL && isSourceFileInLibrary(user->source_name)) {
            print_source_name(fp, user);
          }
        }
        fputs("\nBASE2_USER_HEADERS =", fp);
        if (makefile->gnu_make && makefile->BaseUserHeadersRegular) {
          fputs(" $(BASE2_USER_SOURCES:.cc=.hh)", fp);
        }
        else {
          for (i = 0; i < makefile->nUserFiles; i++) {
            const struct user_struct *user = makefile->UserFiles + i;
            if (user->dir_name != NULL && isHeaderFileInLibrary(user->header_name))
              print_header_name(fp, user);
          }
        }
      }
    }
    if (makefile->dynamic) {
      fputs("\n\n"
            "# Shared object files of this project:\n"
            "SHARED_OBJECTS =", fp);
      if (makefile->gnu_make) {
        fputs(" $(GENERATED_SOURCES:.cc=.so)", fp);
      } else {
        for (i = 0; i < makefile->nTTCN3Modules; i++) {
          const struct module_struct *module = makefile->TTCN3Modules + i;
          if (module->dir_name == NULL || !makefile->central_storage) {
            print_generated_file_name(fp, module, FALSE, ".so");
            if (makefile->code_splitting_mode != NULL) {
              int n_slices;
              if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                print_generated_file_name(fp, module, FALSE, "_seq.so");
                print_generated_file_name(fp, module, FALSE, "_set.so");
                print_generated_file_name(fp, module, FALSE, "_seqof.so");
                print_generated_file_name(fp, module, FALSE, "_setof.so");
                print_generated_file_name(fp, module, FALSE, "_union.so");
              } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                for (int slice = 1; slice < n_slices; slice++) {
                char buffer[16]; // 6 digits + 4 chars + _part
                sprintf(buffer, "_part_%i.so", slice);
                  print_generated_file_name(fp, module, FALSE, buffer);
                }
              }
            }
          }
        }
        if (makefile->preprocess) {
          for (i = 0; i < makefile->nTTCN3PPModules; i++) {
            const struct module_struct *module = makefile->TTCN3PPModules + i;
            if (module->dir_name == NULL || !makefile->central_storage) {
              print_generated_file_name(fp, module, FALSE, ".so");
              if (makefile->code_splitting_mode != NULL) {
                int n_slices;
                if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                  print_generated_file_name(fp, module, FALSE, "_seq.so");
                  print_generated_file_name(fp, module, FALSE, "_set.so");
                  print_generated_file_name(fp, module, FALSE, "_seqof.so");
                  print_generated_file_name(fp, module, FALSE, "_setof.so");
                  print_generated_file_name(fp, module, FALSE, "_union.so");
                } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                  for (int slice = 1; slice < n_slices; slice++) {
                char buffer[16]; // 6 digits + 4 chars + _part
                sprintf(buffer, "_part_%i.so", slice);
                    print_generated_file_name(fp, module, FALSE, buffer);
                  }
                }
              }
            }
          }
        }
        for (i = 0; i < makefile->nASN1Modules; i++) {
          const struct module_struct *module = makefile->ASN1Modules + i;
          if (module->dir_name == NULL || !makefile->central_storage) {
            print_generated_file_name(fp, module, FALSE, ".so");
            if (makefile->code_splitting_mode != NULL) {
              int n_slices;
              if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                print_generated_file_name(fp, module, FALSE, "_seq.so");
                print_generated_file_name(fp, module, FALSE, "_set.so");
                print_generated_file_name(fp, module, FALSE, "_seqof.so");
                print_generated_file_name(fp, module, FALSE, "_setof.so");
                print_generated_file_name(fp, module, FALSE, "_union.so");
              } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                for (int slice = 1; slice < n_slices; slice++) {
                char buffer[16]; // 6 digits + 4 chars + _part
                sprintf(buffer, "_part_%i.so", slice);
                  print_generated_file_name(fp, module, FALSE, buffer);
                }
              }
            }
          }
        }
        for (i = 0; i < makefile->nXSDModules; i++) {
          const struct module_struct *module = makefile->XSDModules + i;
          if (module->dir_name == NULL || !makefile->central_storage) {
            print_generated_file_name(fp, module, FALSE, ".so");
            if (makefile->code_splitting_mode != NULL) {
              int n_slices;
              if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                print_generated_file_name(fp, module, FALSE, "_seq.so");
                print_generated_file_name(fp, module, FALSE, "_set.so");
                print_generated_file_name(fp, module, FALSE, "_seqof.so");
                print_generated_file_name(fp, module, FALSE, "_setof.so");
                print_generated_file_name(fp, module, FALSE, "_union.so");
              } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                for (int slice = 1; slice < n_slices; slice++) {
                char buffer[16]; // 6 digits + 4 chars + _part
                sprintf(buffer, "_part_%i.so", slice);
                  print_generated_file_name(fp, module, FALSE, buffer);
                }
              }
            }
          }
        }
      }
      if (makefile->gnu_make && makefile->UserSourcesRegular) {
        fputs(" $(USER_SOURCES:.cc=.so)", fp);
      } else {
        for (i = 0; i < makefile->nUserFiles; i++) {
          const struct user_struct *user = makefile->UserFiles + i;
          if (user->dir_name == NULL || !makefile->central_storage)
            print_shared_object_name(fp, user);
        }
      }
    }

    fputs("\n\n"
          "# Object files of this project that are needed for the executable "
          "test suite:\n"
          "OBJECTS = $(GENERATED_OBJECTS) $(USER_OBJECTS)\n\n" /* never := */
          "GENERATED_OBJECTS =", fp);
    if (makefile->gnu_make) {
      fputs(" $(GENERATED_SOURCES:.cc=.o)", fp);
    } else {
      for (i = 0; i < makefile->nTTCN3Modules; i++) {
        const struct module_struct *module = makefile->TTCN3Modules + i;
        if (module->dir_name == NULL || !makefile->central_storage) {
          print_generated_file_name(fp, module, FALSE, ".o");
          if (makefile->code_splitting_mode != NULL) {
            int n_slices;
            if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
              print_generated_file_name(fp, module, FALSE, "_seq.o");
              print_generated_file_name(fp, module, FALSE, "_set.o");
              print_generated_file_name(fp, module, FALSE, "_seqof.o");
              print_generated_file_name(fp, module, FALSE, "_setof.o");
              print_generated_file_name(fp, module, FALSE, "_union.o");
            } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
              for (int slice = 1; slice < n_slices; slice++) {
                char buffer[16]; // 6 digits + 4 chars + _part
                sprintf(buffer, "_part_%i.o", slice);
                print_generated_file_name(fp, module, FALSE, buffer);
              }
            }
          }
        }
      }
      if (makefile->preprocess) {
        for (i = 0; i < makefile->nTTCN3PPModules; i++) {
          const struct module_struct *module = makefile->TTCN3PPModules + i;
          if (module->dir_name == NULL || !makefile->central_storage) {
            print_generated_file_name(fp, module, FALSE, ".o");
            if (makefile->code_splitting_mode != NULL) {
              int n_slices;
              if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                print_generated_file_name(fp, module, FALSE, "_seq.o");
                print_generated_file_name(fp, module, FALSE, "_set.o");
                print_generated_file_name(fp, module, FALSE, "_seqof.o");
                print_generated_file_name(fp, module, FALSE, "_setof.o");
                print_generated_file_name(fp, module, FALSE, "_union.o");
              } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                for (int slice = 1; slice < n_slices; slice++) {
                  char buffer[16]; // 6 digits + 4 chars + _part
                sprintf(buffer, "_part_%i.o", slice);
                  print_generated_file_name(fp, module, FALSE, buffer);
                }
              }
            }
          }
        }
      }
      for (i = 0; i < makefile->nASN1Modules; i++) {
        const struct module_struct *module = makefile->ASN1Modules + i;
        if (module->dir_name == NULL || !makefile->central_storage) {
          print_generated_file_name(fp, module, FALSE, ".o");
          if (makefile->code_splitting_mode != NULL) {
            int n_slices;
            if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
              print_generated_file_name(fp, module, FALSE, "_seq.o");
              print_generated_file_name(fp, module, FALSE, "_set.o");
              print_generated_file_name(fp, module, FALSE, "_seqof.o");
              print_generated_file_name(fp, module, FALSE, "_setof.o");
              print_generated_file_name(fp, module, FALSE, "_union.o");
            } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
              for (int slice = 1; slice < n_slices; slice++) {
                char buffer[16]; // 6 digits + 4 chars + _part
                sprintf(buffer, "_part_%i.o", slice);
                print_generated_file_name(fp, module, FALSE, buffer);
              }
            }
          }
        }
      }
      if (makefile->nXSDModules) {
        for (i = 0; i < makefile->nXSDModules; i++) {
          const struct module_struct *module = makefile->XSDModules + i;
          if (module->dir_name == NULL || !makefile->central_storage) {
            print_generated_file_name(fp, module, FALSE, ".o");
            if (makefile->code_splitting_mode != NULL) {
              int n_slices;
              if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                print_generated_file_name(fp, module, FALSE, "_seq.o");
                print_generated_file_name(fp, module, FALSE, "_set.o");
                print_generated_file_name(fp, module, FALSE, "_seqof.o");
                print_generated_file_name(fp, module, FALSE, "_setof.o");
                print_generated_file_name(fp, module, FALSE, "_union.o");
              } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                for (int slice = 1; slice < n_slices; slice++) {
                  char buffer[16]; // 6 digits + 4 chars + _part
                  sprintf(buffer, "_part_%i.o", slice);
                  print_generated_file_name(fp, module, FALSE, buffer);
                }
              }
            }
          }
        }
      }
    }

    fputs("\n\nUSER_OBJECTS =", fp);
    if (makefile->gnu_make && makefile->UserSourcesRegular) {
      fputs(" $(USER_SOURCES:.cc=.o)", fp);
    } else {
      for (i = 0; i < makefile->nUserFiles; i++) {
        const struct user_struct *user = makefile->UserFiles + i;
        if (user->dir_name == NULL || !makefile->central_storage)
          print_object_name(fp, user);
      }
    }
    fprint_extra_targets(fp, makefile->target_placement_list, "USER_OBJECTS");

    if (makefile->gcc_dep) {
        /* GNU Make processes included makefiles in reverse order. By putting
         * user sources first, their .d will be generated last, after the
         * GENERATED_SOURCES (and GENERATED_HEADERS) have been created.
         * This avoid spurious errors during incremental dependency generation */
      fputs("\n\nDEPFILES = $(USER_OBJECTS:.o=.d)  $(GENERATED_OBJECTS:.o=.d)", fp);
    }

    if (makefile->central_storage) {
      if (makefile->dynamic) {
        fputs("\n\n"
              "# Shared object files of central project(s):\n"
              "BASE_SHARED_OBJECTS =", fp);
        if (!makefile->linkingStrategy) {
          if (makefile->gnu_make) {
            fputs(" $(BASE_GENERATED_SOURCES:.cc=.so)", fp);
          }
          else {
            for (i = 0; i < makefile->nTTCN3Modules; i++) {
              const struct module_struct *module = makefile->TTCN3Modules + i;
              if (module->dir_name != NULL) {
                print_generated_file_name(fp, module, TRUE, ".so");
                if (makefile->code_splitting_mode != NULL) {
                  int n_slices;
                  if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                    print_generated_file_name(fp, module, TRUE, "_seq.so");
                    print_generated_file_name(fp, module, TRUE, "_set.so");
                    print_generated_file_name(fp, module, TRUE, "_seqof.so");
                    print_generated_file_name(fp, module, TRUE, "_setof.so");
                    print_generated_file_name(fp, module, TRUE, "_union.so");
                  } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                    for (int slice = 1; slice < n_slices; slice++) {
                      char buffer[16]; // 6 digits + 4 chars + _part
                      sprintf(buffer, "_part_%i.so", slice);
                      print_generated_file_name(fp, module, TRUE, buffer);
                    }
                  }
                }
              }
            }
            if (makefile->preprocess) {
              for (i = 0; i < makefile->nTTCN3PPModules; i++) {
                const struct module_struct *module =
                  makefile->TTCN3PPModules + i;
                if (module->dir_name != NULL) {
                  print_generated_file_name(fp, module, TRUE, ".so");
                  if (makefile->code_splitting_mode != NULL) {
                    int n_slices;
                    if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                      print_generated_file_name(fp, module, TRUE, "_seq.so");
                      print_generated_file_name(fp, module, TRUE, "_set.so");
                      print_generated_file_name(fp, module, TRUE, "_seqof.so");
                      print_generated_file_name(fp, module, TRUE, "_setof.so");
                      print_generated_file_name(fp, module, TRUE, "_union.so");
                    } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                      for (int slice = 1; slice < n_slices; slice++) {
                        char buffer[16]; // 6 digits + 4 chars + _part
                        sprintf(buffer, "_part_%i.so", slice);
                        print_generated_file_name(fp, module, TRUE, buffer);
                      }
                    }
                  }
                }
              }
            }
            for (i = 0; i < makefile->nASN1Modules; i++) {
              const struct module_struct *module = makefile->ASN1Modules + i;
              if (module->dir_name != NULL) {
                print_generated_file_name(fp, module, TRUE, ".so");
                if (makefile->code_splitting_mode != NULL) {
                  int n_slices;
                  if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                    print_generated_file_name(fp, module, TRUE, "_seq.so");
                    print_generated_file_name(fp, module, TRUE, "_set.so");
                    print_generated_file_name(fp, module, TRUE, "_seqof.so");
                    print_generated_file_name(fp, module, TRUE, "_setof.so");
                    print_generated_file_name(fp, module, TRUE, "_union.so");
                  } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                    for (int slice = 1; slice < n_slices; slice++) {
                      char buffer[16]; // 6 digits + 4 chars + _part
                      sprintf(buffer, "_part_%i.so", slice);
                      print_generated_file_name(fp, module, TRUE, buffer);
                    }
                  }
                }
              }
            }
            for (i = 0; i < makefile->nXSDModules; i++) {
              const struct module_struct *module = makefile->XSDModules + i;
              if (module->dir_name != NULL) {
                print_generated_file_name(fp, module, TRUE, ".so");
                if (makefile->code_splitting_mode != NULL) {
                  int n_slices;
                  if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                    print_generated_file_name(fp, module, TRUE, "_seq.so");
                    print_generated_file_name(fp, module, TRUE, "_set.so");
                    print_generated_file_name(fp, module, TRUE, "_seqof.so");
                    print_generated_file_name(fp, module, TRUE, "_setof.so");
                    print_generated_file_name(fp, module, TRUE, "_union.so");
                  } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                    for (int slice = 1; slice < n_slices; slice++) {
                      char buffer[16]; // 6 digits + 4 chars + _part
                      sprintf(buffer, "_part_%i.so", slice);
                      print_generated_file_name(fp, module, TRUE, buffer);
                    }
                  }
                }
              }
            }
          }
          if (makefile->gnu_make && makefile->BaseUserSourcesRegular) {
            fputs(" $(BASE_USER_SOURCES:.cc=.so)", fp);
          }
          else {
            for (i = 0; i < makefile->nUserFiles; i++) {
              const struct user_struct *user = makefile->UserFiles + i;
              if (user->dir_name != NULL)
                print_shared_object_name(fp, user);
            }
          }
        }
        else { // new linkingStrategy
          if (makefile->gnu_make) {
            fputs(" $(BASE_GENERATED_SOURCES:.cc=.so)", fp);
          }
          else
            ERROR("the usage of 'Z' flag requires GNU make");

          if (makefile->gnu_make && makefile->BaseUserSourcesRegular) {
            fputs(" $(BASE_USER_SOURCES:.cc=.so)", fp);
          }
          else {
            for (i = 0; i < makefile->nUserFiles; i++) {
              const struct user_struct *user = makefile->UserFiles + i;
              if (user->dir_name != NULL && !isSourceFileInLibrary(user->source_name))
                print_shared_object_name(fp, user);
            }
          }
        }
      } /* if dynamic */
      fputs("\n\n"
            "# Object files of central project(s) that are needed for the "
            "executable test suite:\n"
            "BASE_OBJECTS =", fp);
      if (!makefile->linkingStrategy) {
        if (makefile->gnu_make) {
          fputs(" $(BASE_GENERATED_SOURCES:.cc=.o)", fp);
        } 
        else {
          for (i = 0; i < makefile->nTTCN3Modules; i++) {
            const struct module_struct *module = makefile->TTCN3Modules + i;
            if (module->dir_name != NULL) {
              print_generated_file_name(fp, module, TRUE, ".o");
              if (makefile->code_splitting_mode != NULL) {
                int n_slices;
                if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                  print_generated_file_name(fp, module, TRUE, "_seq.o");
                  print_generated_file_name(fp, module, TRUE, "_set.o");
                  print_generated_file_name(fp, module, TRUE, "_seqof.o");
                  print_generated_file_name(fp, module, TRUE, "_setof.o");
                  print_generated_file_name(fp, module, TRUE, "_union.o");
                } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                  for (int slice = 1; slice < n_slices; slice++) {
                    char buffer[16]; // 6 digits + 4 chars + _part
                    sprintf(buffer, "_part_%i.o", slice);
                    print_generated_file_name(fp, module, TRUE, buffer);
                  }
                }
              }
            }
          }
          if (makefile->preprocess) {
            for (i = 0; i < makefile->nTTCN3PPModules; i++) {
              const struct module_struct *module = makefile->TTCN3PPModules + i;
              if (module->dir_name != NULL) {
                print_generated_file_name(fp, module, TRUE, ".o");
                if (makefile->code_splitting_mode != NULL) {
                  int n_slices;
                  if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                    print_generated_file_name(fp, module, TRUE, "_seq.o");
                    print_generated_file_name(fp, module, TRUE, "_set.o");
                    print_generated_file_name(fp, module, TRUE, "_seqof.o");
                    print_generated_file_name(fp, module, TRUE, "_setof.o");
                    print_generated_file_name(fp, module, TRUE, "_union.o");
                  } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                    for (int slice = 1; slice < n_slices; slice++) {
                      char buffer[16]; // 6 digits + 4 chars + _part
                      sprintf(buffer, "_part_%i.o", slice);
                      print_generated_file_name(fp, module, TRUE, buffer);
                    }
                  }
                }
              }
            }
          }
          for (i = 0; i < makefile->nASN1Modules; i++) {
            const struct module_struct *module = makefile->ASN1Modules + i;
            if (module->dir_name != NULL) {
              print_generated_file_name(fp, module, TRUE, ".o");
              if (makefile->code_splitting_mode != NULL) {
                int n_slices;
                if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                  print_generated_file_name(fp, module, TRUE, "_seq.o");
                  print_generated_file_name(fp, module, TRUE, "_set.o");
                  print_generated_file_name(fp, module, TRUE, "_seqof.o");
                  print_generated_file_name(fp, module, TRUE, "_setof.o");
                  print_generated_file_name(fp, module, TRUE, "_union.o");
                } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                  for (int slice = 1; slice < n_slices; slice++) {
                    char buffer[16]; // 6 digits + 4 chars + _part
                    sprintf(buffer, "_part_%i.o", slice);
                    print_generated_file_name(fp, module, TRUE, buffer);
                  }
                }
              }
            }
          }
          for (i = 0; i < makefile->nXSDModules; i++) {
            const struct module_struct *module = makefile->XSDModules + i;
            if (module->dir_name != NULL) {
              print_generated_file_name(fp, module, TRUE, ".o");
              if (makefile->code_splitting_mode != NULL) {
                int n_slices;
                if (strcmp(makefile->code_splitting_mode, "-U type") == 0) {
                  print_generated_file_name(fp, module, TRUE, "_seq.o");
                  print_generated_file_name(fp, module, TRUE, "_set.o");
                  print_generated_file_name(fp, module, TRUE, "_seqof.o");
                  print_generated_file_name(fp, module, TRUE, "_setof.o");
                  print_generated_file_name(fp, module, TRUE, "_union.o");
                } else if((n_slices = atoi(makefile->code_splitting_mode + 2))) {
                  for (int slice = 1; slice < n_slices; slice++) {
                    char buffer[16]; // 6 digits + 4 chars + _part
                    sprintf(buffer, "_part_%i.o", slice);
                    print_generated_file_name(fp, module, TRUE, buffer);
                  }
                }
              }
            }
          }
        }
        if (makefile->gnu_make && makefile->BaseUserSourcesRegular) {
          fputs(" $(BASE_USER_SOURCES:.cc=.o)", fp);
        }
        else {
          for (i = 0; i < makefile->nUserFiles; i++) {
            const struct user_struct *user = makefile->UserFiles + i;
            if (user->dir_name != NULL)
              print_object_name(fp, user);
          }
        }
      }
      else { // new linkingStrategy
        if (makefile->gnu_make) {
          fputs(" $(BASE_GENERATED_SOURCES:.cc=.o)", fp);
        }
        else
          ERROR("the usage of 'Z' flag requires GNU make");

        if (makefile->gnu_make && makefile->BaseUserSourcesRegular) {
          fputs(" $(BASE_USER_SOURCES:.cc=.o)", fp);
        }
        else {
          for (i = 0; i < makefile->nUserFiles; i++) {
            const struct user_struct *user = makefile->UserFiles + i;
            if (user->dir_name != NULL && !isSourceFileInLibrary(user->source_name))
              print_object_name(fp, user);
          }
        }
      }
    }
    if (makefile->linkingStrategy) {
      fputs("\n\n"
            "# Object files of library linked central project(s) that are needed for the "
            "executable test suite:\n"
            "BASE2_OBJECTS =", fp);
      if (makefile->gnu_make) {
        if (makefile->dynamic)
          fputs(" $(BASE2_GENERATED_SOURCES:.cc=.so)", fp);
        else
          fputs(" $(BASE2_GENERATED_SOURCES:.cc=.o)", fp);
      }
      else ERROR("the usage of 'Z' flag requires GNU make");

      if (makefile->gnu_make && makefile->BaseUserSourcesRegular) {
        if (makefile->dynamic)
          fputs(" $(BASE2_USER_SOURCES:.cc=.so)", fp);
        else
          fputs(" $(BASE2_USER_SOURCES:.cc=.o)", fp);
      }
      else {
        for (i = 0; i < makefile->nUserFiles; i++) {
          const struct user_struct *user = makefile->UserFiles + i;
          if (user->dir_name != NULL && isSourceFileInLibrary(user->source_name)) {
             if (makefile->dynamic)
               print_shared_object_name(fp, user);
             else
               print_object_name(fp, user);
          }
        }
      }
      if (makefile->hierarchical) {
        fputs("\n\n"
              "#Libraries of referenced project(s) that are needed for the "
              "executable or library target:\n"
              "BASE2_LIBRARY =", fp);
        struct string2_list* head = getLinkerLibs(makefile->project_name);
        struct string2_list* act_elem = head;
        while (act_elem) {
          if (act_elem->str2) {
            fputs(" ", fp);
            fprintf(fp, "%s/lib%s.%s", act_elem->str1, act_elem->str2,
                    isDynamicLibrary(act_elem->str2) ? "so" : "a");
          }
          act_elem = act_elem->next;
        }
        free_string2_list(head);
      }
    }

    fputs("\n\n"
    "# Other files of the project (Makefile, configuration files, etc.)\n"
    "# that will be added to the archived source files:\n"
    "OTHER_FILES =", fp);
    for (i = 0; i < makefile->nOtherFiles; i++)
      fprintf(fp, " %s", makefile->OtherFiles[i]);
    fprint_extra_targets(fp, makefile->target_placement_list, "OTHER_FILES");

    if (makefile->ets_name) {
      const char *ets_suffix = NULL;
      /* EXECUTABLE variable */
      fprintf(fp, "\n\n"
          "# The name of the executable test suite:\n"
          "EXECUTABLE = %s", makefile->ets_name);
#ifdef WIN32
      {
        /* add the .exe suffix unless it is already present */
        ets_suffix = get_suffix(makefile->ets_name);
        if (ets_suffix == NULL || strcmp(ets_suffix, "exe"))
          fputs(".exe", fp);
      }
#endif
      fputs("\n\n", fp);
      if (makefile->linkingStrategy) {
#ifndef WIN32
        fputs("DYNAMIC_LIBRARY = lib$(EXECUTABLE).so\n", fp);
        fputs("STATIC_LIBRARY = lib$(EXECUTABLE).a\n", fp);
#else
        char* name_prefix = cut_suffix(makefile->ets_name);
        fprintf(fp, "DYNAMIC_LIBRARY = lib%s.so\n", name_prefix);
        fprintf(fp, "STATIC_LIBRARY = lib%s.a\n", name_prefix);
        Free(name_prefix);
#endif
      }
      /* LIBRARY variable */
      ets_suffix = get_suffix(makefile->ets_name);
      if (ets_suffix != NULL && !strcmp(ets_suffix, "exe")) {
        char* name_prefix = cut_suffix(makefile->ets_name);
          fprintf(fp, "\n\nLIBRARY = %s%s%s\n", "lib", name_prefix ? name_prefix :  "library",
              makefile->dynamic ? ".so" : ".a");
          Free(name_prefix);
      }
      else {
#ifndef WIN32
        fprintf(fp, "\n\nLIBRARY = lib$(EXECUTABLE)%s\n",
                makefile->dynamic ? ".so" : ".a");
#else
        fprintf(fp, "\n\nLIBRARY = lib%s%s\n",
                makefile->ets_name, makefile->dynamic ? ".so" : ".a");
#endif
        }

    } else {
      fputs("\n\n"
          "# The name of the executable test suite:\n"
          "EXECUTABLE =\n"
          "LIBRARY =\n", fp);
    }
    if (!makefile->linkingStrategy || !buildObjects(makefile->project_name, add_refd_prjs)) {
      fprintf(fp, "\n"
        "TARGET = $(%s)", makefile->library ? "LIBRARY" : "EXECUTABLE");
    }
    else {
      if (makefile->dynamic) {
        fputs("\n"
              "TARGET = $(SHARED_OBJECTS)", fp);
      }
      else {
        fputs("\n"
              "TARGET = $(OBJECTS)", fp);
      }
    }
    fputs("\n\n"
      "#\n"
      "# Do not modify these unless you know what you are doing...\n"
      "# Platform specific additional libraries:\n"
      "#\n", fp);

    fputs("SOLARIS_LIBS = -lsocket -lnsl -lxml2", fp);
#ifdef USAGE_STATS
    fputs(" -lresolv", fp);
#endif
#ifdef ADVANCED_DEBUGGER_UI
    fputs(" -lcurses", fp);
#endif
    if (makefile->solspeclibraries) {
      struct string_list* act_elem = makefile->solspeclibraries;
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, " -l%s", act_elem->str);
        }
        act_elem = act_elem->next;
      }
    }
    fputs("\n", fp);

    fputs("SOLARIS8_LIBS = -lsocket -lnsl -lxml2", fp);
#ifdef USAGE_STATS
    fputs(" -lresolv", fp);
#endif
#ifdef ADVANCED_DEBUGGER_UI
    fputs(" -lcurses", fp);
#endif
    if (makefile->sol8speclibraries) {
      struct string_list* act_elem = makefile->sol8speclibraries;
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, " -l%s", act_elem->str);
        }
        act_elem = act_elem->next;
      }
    }
    fputs("\n", fp);

    fputs("LINUX_LIBS = -lxml2", fp);
#ifdef USAGE_STATS
    fputs(" -lpthread -lrt", fp);
#endif
#ifdef ADVANCED_DEBUGGER_UI
    fputs(" -lncurses", fp);
#endif
    if (makefile->linuxspeclibraries) {
      struct string_list* act_elem = makefile->linuxspeclibraries;
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, " -l%s", act_elem->str);
        }
        act_elem = act_elem->next;
      }
    }
    fputs("\n", fp);

    fputs("FREEBSD_LIBS = -lxml2", fp);
#ifdef ADVANCED_DEBUGGER_UI
    fputs(" -lncurses", fp);
#endif
    if (makefile->freebsdspeclibraries) {
      struct string_list* act_elem = makefile->freebsdspeclibraries;
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, " -l%s", act_elem->str);
        }
        act_elem = act_elem->next;
      }
    }
    fputs("\n", fp);

    fputs("WIN32_LIBS = -lxml2", fp);
#ifdef ADVANCED_DEBUGGER_UI
    fputs(" -lncurses", fp);
#endif
    if (makefile->win32speclibraries) {
      struct string_list* act_elem = makefile->win32speclibraries;
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, " -l%s", act_elem->str);
        }
        act_elem = act_elem->next;
      }
    }
    fputs("\n\n", fp);

    fputs("#\n"
      "# Rules for building the executable...\n"
      "#\n\n", fp);
    fprintf(fp, "all:%s $(TARGET) ;\n\n", add_refd_prjs?" referenced-all":"");

    if (makefile->dynamic) {
      fprintf(fp, "shared_objects:%s $(SHARED_OBJECTS) ;\n\n", add_refd_prjs?" referenced-shared_objects":"");
    }

    fprintf(fp,
      "executable:%s $(EXECUTABLE) ;\n\n"
      "library:%s $(LIBRARY) ;\n\n"
      "objects:%s $(OBJECTS) compile;\n\n", add_refd_prjs?" referenced-executable":"", add_refd_prjs?" referenced-library":"", add_refd_prjs?" referenced-objects":"");

    /* target $(EXECUTABLE) */
    if (makefile->dynamic && makefile->library) {
      /* There is no need to create the .so for all the source files */
      fputs("$(EXECUTABLE): $(LIBRARY)\n"
          "\tif $(CXX) $(LDFLAGS) -o $@ $(LIBRARY)", fp);
    }
    else {
      fprintf(fp, "$(EXECUTABLE): %s", makefile->dynamic ? "$(SHARED_OBJECTS)" : "$(OBJECTS)");
      if (!makefile->linkingStrategy) { // use the old linking method
        if (makefile->central_storage) {
          if (makefile->dynamic) {
            fputs(" $(BASE_SHARED_OBJECTS)", fp);
          } else {
            fputs(" $(BASE_OBJECTS)", fp);
          }
        }
      }
      else {
        if (!makefile->library) {
          if (makefile->dynamic) {
            fputs(" $(BASE_SHARED_OBJECTS)", fp);
          }
          else {
            fputs(" $(BASE_OBJECTS)", fp);
          }
          if (makefile->hierarchical) {
            fputs(" $(BASE2_LIBRARY)", fp);
          }
        }
      }
      fprintf(fp, "\n"
              "\tif $(CXX) $(LDFLAGS) -o $@ %s",
#if defined (SOLARIS) || defined (SOLARIS8)
              "");
#else
              makefile->dynamic ? "-Wl,--no-as-needed " : ""); /* start writing the link step */
#endif
      if (makefile->gnu_make) fputs("$^", fp);
      else {
        if (makefile->dynamic) {
          fputs("$(SHARED_OBJECTS)", fp);
            if (makefile->central_storage)
            fputs(" $(BASE_SHARED_OBJECTS)", fp);
        } 
        else {
          fputs("$(OBJECTS)", fp);
            if (makefile->central_storage)
            fputs(" $(BASE_OBJECTS)", fp);
        }
      }
    }

    if (makefile->additionalObjects) {
      struct string_list* act_elem = makefile->additionalObjects;
      while (act_elem) {
        if (act_elem->str) {
          fprintf(fp, " %s", act_elem->str);
        }
        act_elem = act_elem->next;
      }
    }

    fprintf(fp, " \\\n"
        "\t-L$(TTCN3_DIR)/lib -l$(TTCN3_LIB)"
        " \\\n"
        "\t-L$(OPENSSL_DIR)/lib -lcrypto");
    if (!makefile->linkingStrategy) {
      if (makefile->linkerlibraries) {
        struct string_list* act_elem = makefile->linkerlibraries;
        while (act_elem) {
          if (act_elem->str) {
            fprintf(fp, " -l%s", act_elem->str);
          }
          act_elem = act_elem->next;
        }
      }
      if (makefile->linkerlibsearchpath) {
        struct string_list* act_elem = makefile->linkerlibsearchpath;
        while (act_elem) {
          if (act_elem->str) {
            fprintf(fp, " -L%s", act_elem->str);
          }
          act_elem = act_elem->next;
        }
      }
      fprintf(fp, " \\\n"
            "\t-L$(XMLDIR)/lib $($(PLATFORM)_LIBS); \\\n"
            "\tthen : ; else $(TTCN3_DIR)/bin/titanver $(OBJECTS); exit 1; fi\n");
    }
    else { // new linking strategy
     fputs (" \\\n", fp);
     if (makefile->linkerlibraries && !makefile->library) {
       struct string2_list* head = getLinkerLibs(makefile->project_name);
       struct string2_list* act_elem = head;
       while (act_elem) {
         if (act_elem->str1 && act_elem->str2) {
            fprintf(fp, "\t-L%s -Wl,-rpath=%s -l%s \\\n", act_elem->str1, act_elem->str1, act_elem->str2);
         }
         act_elem = act_elem->next;
       }
       free_string2_list(head);

       struct string_list* act_head = getExternalLibPaths(makefile->project_name);
       struct string_list* act_ext_elem = act_head;
       while (act_ext_elem) {
         if (act_ext_elem->str) {
           fprintf(fp, "\t-L%s \\\n", act_ext_elem->str);
         }
         act_ext_elem = act_ext_elem->next;
       }
       free_string_list(act_head);
       act_head = getExternalLibs(makefile->project_name);
       act_ext_elem = act_head;
       while (act_ext_elem) {
         if (act_ext_elem->str) {
           fprintf(fp, "\t-l%s \\\n", act_ext_elem->str);
         }
         act_ext_elem = act_ext_elem->next;
       }
       free_string_list(act_head);
     }
     fprintf(fp,
           "\t-L$(XMLDIR)/lib $($(PLATFORM)_LIBS); \\\n"
           "\tthen : ; else $(TTCN3_DIR)/bin/titanver $(OBJECTS); exit 1; fi\n");
    }
    /* If the compiler will not be run because there are no TTCN(PP) or ASN.1
     * files, create the "compile" marker file which is checked by the
     * superior makefile if using this project as central storage */
    if (!run_compiler) fputs("\ttouch compile\n", fp);
    /* End of target $(EXECUTABLE) */

    /* target $(LIBRARY) */
    if (makefile->dynamic) {
      fprintf(fp, "\n"
                  "$(LIBRARY): $(OBJECTS)%s\n"
                  "\t$(CXX) -shared -o $@ $(OBJECTS)",
                  makefile->hierarchical ? " $(BASE2_LIBRARY)" : "");
      if (makefile->central_storage && !makefile->linkingStrategy) {
        fputs(" $(BASE_SHARED_OBJECTS) ;\n"
              "\tln -s $@ $(subst lib, ,$@) > /dev/null 2>&1 ;", fp);
      }
      if (makefile->linkingStrategy) {
        struct string2_list* head = getLinkerLibs(makefile->project_name);
        struct string2_list* act_elem = head;
        // If the project is Executable on Top Level the linker can link the *.a and *.so together
        while (act_elem && !isTopLevelExecutable(makefile->project_name)) {
            if (act_elem->str1 && act_elem->str2 && isDynamicLibrary(act_elem->str2)) {
              fputs(" \\\n", fp);
              fprintf(fp, "\t-L%s -Wl,-rpath=%s -l%s", act_elem->str1, act_elem->str1, act_elem->str2);
            }
            else {
              const char* mainLibName = getLibFromProject(makefile->project_name);
              ERROR("Library archive 'lib%s.a' cannot be linked to dynamic library 'lib%s.so' "
                    "in project '%s' ",
                    act_elem->str2, mainLibName ? mainLibName : "", makefile->project_name);
              free_string2_list(head);
              exit(EXIT_FAILURE);
            }
          act_elem = act_elem->next;
        }
        free_string2_list(head);
        struct string_list* act_head = getExternalLibPaths(makefile->project_name);
        struct string_list* act_ext_elem = act_head;
        while (act_ext_elem) {
          if (act_ext_elem->str) {
            fputs(" \\\n", fp);
            fprintf(fp, "\t-L%s", act_ext_elem->str);
          }
          act_ext_elem = act_ext_elem->next;
        }
        free_string_list(act_head);
        act_head = getExternalLibs(makefile->project_name);
        act_ext_elem = act_head;
        while (act_ext_elem) {
          if (act_ext_elem->str) {
            fputs(" \\\n", fp);
            fprintf(fp, "\t-l%s", act_ext_elem->str);
          }
          act_ext_elem = act_ext_elem->next;
        }
        free_string_list(act_head);
      }
    }
    else { // static linking
      fprintf(fp, "\n"
                  "$(LIBRARY): $(OBJECTS)%s\n"
                  "\t$(AR) -r%s $(ARFLAGS) $(LIBRARY) $(OBJECTS)",
                  makefile->hierarchical ? " $(BASE2_LIBRARY)" : "",
                  makefile->linkingStrategy ? "cT" : "");
      if (makefile->central_storage && !makefile->linkingStrategy) {
        fputs(" $(BASE_OBJECTS)", fp);
      }
      if (makefile->linkingStrategy) {
        if (makefile->library) {
          struct string2_list* head = getLinkerLibs(makefile->project_name);
          struct string2_list* act_elem = head;
          while (act_elem) {
            if (act_elem->str2 && !isDynamicLibrary(act_elem->str2)) {
              fputs(" \\\n", fp);
              fprintf(fp, "\t%s/lib%s.a", act_elem->str1, act_elem->str2);
            }
            else {
              const char* mainLibName = getLibFromProject(makefile->project_name);
              if (act_elem->str2) {
                ERROR("Dynamic library 'lib%s.so' cannot be linked to static library 'lib%s.a' "
                      "in project '%s' ",
                      act_elem->str2, mainLibName ? mainLibName : "", makefile->project_name);
                exit(EXIT_FAILURE);
              }
              else {
                struct string_list* ext_libs = getExternalLibs(makefile->project_name);
                if (ext_libs && ext_libs->str) {
                  ERROR("Third party dynamic library '%s' cannot be linked to static library 'lib%s.a' "
                        "in project '%s' ", ext_libs->str,
                        mainLibName ? mainLibName : "", makefile->project_name);
                  free_string_list(ext_libs);
                  exit(EXIT_FAILURE);
                }
                free_string_list(ext_libs);
              }
            }
            act_elem = act_elem->next;
          }
          free_string2_list(head);

          struct string_list* act_head = getExternalLibs(makefile->project_name);
          struct string_list* act_ext_elem = act_head;
          while (act_ext_elem) {
            if (act_ext_elem->str && hasExternalLibrary(act_ext_elem->str, makefile->project_name)) {
              fputs(" \\\n", fp);
              fprintf(fp, "\tlib%s.a", act_ext_elem->str);
              ERROR("linking static 3d party or system library 'lib%s.a' to "
                      "project library 'lib%s.a' is not supported ",
                      act_ext_elem->str, makefile->ets_name);
              exit(EXIT_FAILURE);
            }
            act_ext_elem = act_ext_elem->next;
          }
          free_string_list(act_head);
        }
      }
    }
    
    for (i = 0; i < makefile->nBaseDirs; i++) {
      fprintf(fp, "\n\n%s/%%.o: %s/%%.c\n"
        "\t$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<\n\n", makefile->BaseDirs[i].dir_name, makefile->BaseDirs[i].dir_name);
      fprintf(fp, "%s/%%.o: %s/%%.cc\n"
            "\t$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<", makefile->BaseDirs[i].dir_name, makefile->BaseDirs[i].dir_name);
    }
    fputs("\n\n%.o: %.c $(GENERATED_HEADERS)\n"
          "\t$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<\n\n", fp);
    fputs("%.o: %.cc $(GENERATED_HEADERS)\n"
          "\t$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<\n\n", fp);

    if (makefile->gcc_dep) {
      fputs(".cc.d .c.d:\n"
            "\t@echo Creating dependency file for '$<'; set -e; \\\n"
            "\t$(CXX) $(CXXDEPFLAGS) $(CPPFLAGS) $(CXXFLAGS) $< \\\n"
            "\t| sed 's/\\($*\\)\\.o[ :]*/\\1.o $@ : /g' > $@; \\\n"
            "\t[ -s $@ ] || rm -f $@\n\n", fp);
      /* "set -e" causes bash to exit the script if any statement
       * returns nonzero (failure).
       * The sed line transforms the first line of the dependency from
       * "x.o: x.cc" to "x.o x.d: x.cc", making the dependency file depend
       * on the source and headers.
       * [ -s x.d ] checks that the generated dependency is not empty;
       * otherwise it gets deleted.
       */
    }

    if (makefile->dynamic) {
      fputs("%.so: %.o\n"
            "\t$(CXX) -shared -o $@ $<\n\n", fp);
    }

    if (makefile->preprocess) {
      fputs("%.ttcn: %.ttcnpp $(TTCN3_INCLUDES)\n"
            "\t$(CPP) -x c -nostdinc $(CPPFLAGS_TTCN3) $< $@\n\n"
            "preprocess: $(PREPROCESSED_TTCN3_MODULES) ;\n\n", fp);
    }

    boolean merge_profiled_file_lists = makefile->profiled_file_list
      && makefile->profiled_file_list->next && !makefile->central_storage;
    if (makefile->central_storage) {
      boolean is_first = TRUE;
      fprintf(fp, "$(GENERATED_SOURCES) $(GENERATED_HEADERS):%s compile-all compile ",
              makefile->hierarchical ? " update" : "");

      if (add_refd_prjs) fputs("referenced-dep", fp);
      /* These extra compile dependencies for the generated .cc are here to
       * check if all the referenced projects are up to date.
       * If the referenced projects are built too then they are not needed
       * (and cause problems as the included .d depends on the .cc).
       */
      if (!add_refd_prjs) for (i = 0; i < makefile->nBaseDirs; i++) {
        const struct base_dir_struct *base_dir = makefile->BaseDirs + i;
        if (base_dir->has_modules) {
          if (is_first) {
            fputs(" \\\n", fp);
            is_first = FALSE;
          }
          else putc(' ', fp);
        fprintf(fp, "%s/compile", base_dir->dir_name);
        }
      }
      if (makefile->preprocess) {
        fprintf(fp, "\n"
        "\t@if [ ! -f $@ ]; then %s compile-all; $(MAKE) compile-all; fi\n"
        "\n"
        "check:%s $(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
        "\t$(PREPROCESSED_TTCN3_MODULES) $(BASE_PREPROCESSED_TTCN3_MODULES) "
        "%s\\\n"
        "%s"
        "%s"
        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s\n"
        "\t$(TTCN3_DIR)/bin/compiler -s $(COMPILER_FLAGS) ",
        rm_command, add_refd_prjs?" referenced-check":"",
        makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) ":"",
        makefile->linkingStrategy ? "$(BASE2_PREPROCESSED_TTCN3_MODULES) ":"",
        makefile->nXSDModules ? "\t$(XSD2TTCN_GENERATED_MODULES) $(BASE_XSD2TTCN_GENERATED_MODULES) \\\n" : "",
        makefile->nXSDModules && makefile->linkingStrategy ? "\t$(BASE2_XSD2TTCN_GENERATED_MODULES) \\\n" : "",
        makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) ":"");
        if (makefile->gnu_make) {
          if (add_refd_prjs) // referenced-check cannot be compiled it is not a ttcn modul
            fprintf(fp, "$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
                        "\t$(PREPROCESSED_TTCN3_MODULES) $(BASE_PREPROCESSED_TTCN3_MODULES) "
                        "%s\\\n"
                        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s\n",
                    makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) ":"",
                    makefile->linkingStrategy ? "$(BASE2_PREPROCESSED_TTCN3_MODULES) ":"",
                    makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) ":"");
          else
            fputs("$^", fp);
        }
        else {
          fputs("\\\n"
          "\t$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) \\\n"
          "\t$(PREPROCESSED_TTCN3_MODULES) "
          "$(BASE_PREPROCESSED_TTCN3_MODULES) \\\n"
          "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES)", fp);
        }
        fprintf(fp, "\n\n"
        "port:%s $(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
        "\t$(PREPROCESSED_TTCN3_MODULES) $(BASE_PREPROCESSED_TTCN3_MODULES) "
        "%s\\\n"
        "%s"
        "%s"
        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s\n"
        "\t$(TTCN3_DIR)/bin/compiler -t $(COMPILER_FLAGS) ",
        add_refd_prjs?" referenced-check":"",
        makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) ":"",
        makefile->linkingStrategy ? "$(BASE2_PREPROCESSED_TTCN3_MODULES) ":"",
        makefile->nXSDModules ? "\t$(XSD2TTCN_GENERATED_MODULES) $(BASE_XSD2TTCN_GENERATED_MODULES) \\\n" : "",
        makefile->nXSDModules && makefile->linkingStrategy ? "\t$(BASE2_XSD2TTCN_GENERATED_MODULES) \\\n" : "",
        makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) ":"");
        if (makefile->gnu_make) {
          if (add_refd_prjs) // referenced-check cannot be compiled it is not a ttcn modul
            fprintf(fp, "$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
                        "\t$(PREPROCESSED_TTCN3_MODULES) $(BASE_PREPROCESSED_TTCN3_MODULES) "
                        "%s\\\n"
                        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s\n",
                    makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) ":"",
                    makefile->linkingStrategy ? "$(BASE2_PREPROCESSED_TTCN3_MODULES) ":"",
                    makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) ":"");
          else
            fputs("$^", fp);
        }
        else {
          fputs("\\\n"
          "\t$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) \\\n"
          "\t$(PREPROCESSED_TTCN3_MODULES) "
          "$(BASE_PREPROCESSED_TTCN3_MODULES) \\\n"
          "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES)", fp);
        }
        if (makefile->linkingStrategy && makefile->hierarchical) {
          fputs("\n\n"
          "update: $(BASE_TTCN3_MODULES) $(BASE_ASN1_MODULES) $(BASE_PREPROCESSED_TTCN3_MODULES) \\\n"
          "\t$(BASE_XSD2TTCN_GENERATED_MODULES) $(BASE2_XSD2TTCN_GENERATED_MODULES) \\\n"
          "\t$(BASE2_TTCN3_MODULES) $(BASE2_ASN1_MODULES) $(BASE2_PREPROCESSED_TTCN3_MODULES)\n"
          "ifneq ($(wildcard $(GENERATED_SOURCES)), ) \n"
          "ifeq ($(wildcard $?), ) \n"
          "\ttouch compile-all; \n"
          "\ttouch update; \n"
          "endif\n"
          "endif",fp);
        }
        if (makefile->profiled_file_list) {
          fputs("\n\n"
            "compile:: $(PROFILED_FILE_LIST)\n"
            "\ttouch $(TTCN3_MODULES) $(PREPROCESSED_TTCN3_MODULES) "
            "$(ASN1_MODULES)", fp);
        }
        fprintf(fp, "\n\n"
        "compile:%s $(TTCN3_MODULES) $(PREPROCESSED_TTCN3_MODULES) "
        "$(ASN1_MODULES) %s%s\n"
        "\t@echo \"compiling \"'$(patsubst %%.tpd, %%, $(TPD))';\n"
        "\t$(TTCN3_DIR)/bin/compiler $(COMPILER_FLAGS) \\\n"
        "\t$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
        "\t$(PREPROCESSED_TTCN3_MODULES) $(BASE_PREPROCESSED_TTCN3_MODULES) %s\\\n"
        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s - $?\n"
        "\ttouch $@\n\n",
        makefile->profiled_file_list ? ":" : "",
        makefile->nXSDModules ? "$(XSD2TTCN_GENERATED_MODULES) $(BASE_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->nXSDModules && makefile->linkingStrategy ? "$(BASE2_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) ":"",
        makefile->linkingStrategy ? "$(BASE2_PREPROCESSED_TTCN3_MODULES) ":"",
        makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES)":"");
        fprintf (fp,
        "compile-all: $(BASE_TTCN3_MODULES) $(BASE_ASN1_MODULES) $(BASE_PREPROCESSED_TTCN3_MODULES) "
        "%s%s"
        "%s\n"
        "\t$(MAKE) preprocess\n"
        "\t@echo \"compiling all \"'$(patsubst %%.tpd, %%, $(TPD))';\n"
        "\t$(TTCN3_DIR)/bin/compiler $(COMPILER_FLAGS) \\\n"
        "\t$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
        "\t$(PREPROCESSED_TTCN3_MODULES) $(BASE_PREPROCESSED_TTCN3_MODULES) %s"
        "\\\n"
        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s\\\n"
        "\t- $(TTCN3_MODULES) $(PREPROCESSED_TTCN3_MODULES) $(ASN1_MODULES)\n"
        "\ttouch $@ compile\n\n",
        makefile->nXSDModules ? "$(XSD2TTCN_GENERATED_MODULES) $(BASE_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->nXSDModules && makefile->linkingStrategy ? "$(BASE2_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->linkingStrategy ? "\\\n\t$(BASE2_TTCN3_MODULES) $(BASE2_ASN1_MODULES) "
                                    "$(BASE2_PREPROCESSED_TTCN3_MODULES) ":"",
        makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) ":"",
        makefile->linkingStrategy ? "$(BASE2_PREPROCESSED_TTCN3_MODULES) ":"",
        makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) ":"");
      }
      else {
        fprintf(fp, "\n"
        "\t@if [ ! -f $@ ]; then %s compile-all; $(MAKE) compile-all; fi\n", rm_command);
        fprintf(fp, "\n"
        "check:%s $(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s%s %s\n"
        "\t$(TTCN3_DIR)/bin/compiler -s $(COMPILER_FLAGS) ",
        add_refd_prjs?" referenced-check":"",
        makefile->nXSDModules ? "$(XSD2TTCN_GENERATED_MODULES) $(BASE_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->nXSDModules && makefile->linkingStrategy ? "$(BASE2_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) ":"",
        makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) ":"");
        if (makefile->gnu_make) {
          if (add_refd_prjs) // referenced-check cannot be compiled it is not a ttcn modul
            fprintf(fp, "$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
                        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s\n",
                    makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) ":"",
                    makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) ":"");
          else
            fputs("$^", fp);
        }
        else {
          fputs("\\\n"
          "\t$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) \\\n"
          "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES)", fp);
        }
        
        fprintf(fp, "\n\n"
        "port:%s $(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s%s %s\n"
        "\t$(TTCN3_DIR)/bin/compiler -t $(COMPILER_FLAGS) ",
        add_refd_prjs?" referenced-check":"",
        makefile->nXSDModules ? "$(XSD2TTCN_GENERATED_MODULES) $(BASE_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->nXSDModules && makefile->linkingStrategy ? "$(BASE2_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) ":"",
        makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) ":"");
        if (makefile->gnu_make) {
          if (add_refd_prjs) // referenced-check cannot be compiled it is not a ttcn modul
            fprintf(fp, "$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
                        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s\n",
                    makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) ":"",
                    makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) ":"");
          else
            fputs("$^", fp);
        }
        else {
          fputs("\\\n"
          "\t$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) \\\n"
          "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES)", fp);
        }

        if (makefile->linkingStrategy && makefile->hierarchical) {
          fputs("\n\n"
          "update: $(BASE_TTCN3_MODULES) $(BASE_ASN1_MODULES) $(BASE_PREPROCESSED_TTCN3_MODULES) \\\n"
          "\t$(XSD2TTCN_GENERATED_MODULES) $(BASE_XSD2TTCN_GENERATED_MODULES) $(BASE2_XSD2TTCN_GENERATED_MODULES) \\\n"
          "\t$(BASE2_TTCN3_MODULES) $(BASE2_ASN1_MODULES) $(BASE2_PREPROCESSED_TTCN3_MODULES)\n"
          "ifneq ($(wildcard $(GENERATED_SOURCES)), ) \n"
          "ifeq ($(wildcard $?), ) \n"
          "\ttouch compile-all; \n"
          "\ttouch update; \n"
          "endif\n"
          "endif",fp);
        }

        if (makefile->profiled_file_list) {
          fputs("\n\n"
            "compile:: $(PROFILED_FILE_LIST)\n"
            "\ttouch $(TTCN3_MODULES) $(ASN1_MODULES)", fp);
        }
        fprintf(fp, "\n\n"
        "compile:%s $(TTCN3_MODULES) $(ASN1_MODULES) %s%s\n"
        "\t@echo \"compiling \"'$(patsubst %%.tpd, %%, $(TPD))';\n"
        "\t$(TTCN3_DIR)/bin/compiler $(COMPILER_FLAGS) \\\n"
        "\t$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s\\\n"
        "\t- $?\n"
        "\ttouch $@\n\n",
        makefile->profiled_file_list ? ":" : "",
        makefile->nXSDModules ? "$(XSD2TTCN_GENERATED_MODULES) $(BASE_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->nXSDModules && makefile->linkingStrategy ? "$(BASE2_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) " : "",
        makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) " : "");
        fprintf(fp,
        "compile-all: $(BASE_TTCN3_MODULES) $(BASE_ASN1_MODULES) %s%s%s\n",
        makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) $(BASE2_ASN1_MODULES)" : "",
        makefile->nXSDModules ? " $(XSD2TTCN_GENERATED_MODULES) $(BASE_XSD2TTCN_GENERATED_MODULES) " : "",
        makefile->nXSDModules && makefile->linkingStrategy ? "$(BASE2_XSD2TTCN_GENERATED_MODULES) " : "");
        fputs("\t@echo \"compiling all \"'$(patsubst %.tpd, %, $(TPD))';\n", fp);
        fprintf(fp,"\t$(TTCN3_DIR)/bin/compiler $(COMPILER_FLAGS) \\\n"
        "\t$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n"
        "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s\\\n"
        "\t- $(TTCN3_MODULES) $(ASN1_MODULES)\n"
        "\ttouch $@ compile\n\n",
        makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) " : "",
        makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) " : "");
      }
      if (!makefile->hierarchical) {
        for (i = 0; i < makefile->nBaseDirs; i++) {
          const struct base_dir_struct *base_dir = makefile->BaseDirs + i;
          if (base_dir->has_modules) {
            size_t j;
            fprintf(fp, "%s/compile:", base_dir->dir_name);
            for (j = 0; j < makefile->nTTCN3Modules; j++) {
              const struct module_struct *module = makefile->TTCN3Modules + j;
              if (module->dir_name != NULL &&
                  !strcmp(base_dir->dir_name, module->dir_name))
                print_file_name(fp, module);
            }
            for (j = 0; j < makefile->nTTCN3PPModules; j++) {
              const struct module_struct *module = makefile->TTCN3PPModules + j;
              if (module->dir_name != NULL &&
                  !strcmp(base_dir->dir_name, module->dir_name))
                print_file_name(fp, module);
            }
            for (j = 0; j < makefile->nASN1Modules; j++) {
              const struct module_struct *module = makefile->ASN1Modules + j;
              if (module->dir_name != NULL &&
                  !strcmp(base_dir->dir_name, module->dir_name))
                print_file_name(fp, module);
            }
            for (j = 0; j < makefile->nXSDModules; j++) {
              const struct module_struct *module = makefile->XSDModules + j;
              if (module->dir_name != NULL &&
                  !strcmp(base_dir->dir_name, module->dir_name))
                print_generated_file_name(fp, module, TRUE, ".ttcn");
            }
            fprintf(fp, "\n"
                    "\t@echo 'Central directory %s is not up-to-date!'\n"
                    "\t@exit 2\n\n", base_dir->dir_name);
          }
        }
      }
    }
    else { /* not central storage */
      fprintf(fp, "$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile\n"
              "\t@if [ ! -f $@ ]; then %s compile; $(MAKE) compile; fi\n\n"
              "%s"
              "check:%s $(TTCN3_MODULES) %s", rm_command,
              merge_profiled_file_lists ? "check:: $(PROFILED_FILE_LIST)\n\n" : "",
              merge_profiled_file_lists ? ":" : "",
              makefile->nXSDModules ? "$(XSD2TTCN_GENERATED_MODULES) " : "");
      if (makefile->preprocess) fputs("$(PREPROCESSED_TTCN3_MODULES) ", fp);
      fputs("$(ASN1_MODULES)\n"
            "\t$(TTCN3_DIR)/bin/compiler -s $(COMPILER_FLAGS) ", fp);
      if (makefile->gnu_make) fputs("$^", fp);
      else {
        fputs("\\\n"
              "\t$(TTCN3_MODULES) $(PREPROCESSED_TTCN3_MODULES) $(ASN1_MODULES)",
              fp);
      }
      
      fputs("\n\n", fp);
      fprintf(fp, "port: $(TTCN3_MODULES) %s ",
        makefile->nXSDModules ? "$(XSD2TTCN_GENERATED_MODULES) " : "");
      if (makefile->preprocess) fputs("$(PREPROCESSED_TTCN3_MODULES) ", fp);
      fputs("$(ASN1_MODULES)\n", fp);
      fputs("\t$(TTCN3_DIR)/bin/compiler -t $(COMPILER_FLAGS) ", fp);
      if (makefile->gnu_make) fputs("$^", fp);
      else {
        fputs("\\\n"
              "\t$(TTCN3_MODULES) $(PREPROCESSED_TTCN3_MODULES) $(ASN1_MODULES)",
              fp);
      }
      
      if (makefile->profiled_file_list) {
        fputs("\n\ncompile:: $(PROFILED_FILE_LIST)\n"
              "\ttouch $(TTCN3_MODULES) ", fp);
        if (makefile->preprocess) fputs("$(PREPROCESSED_TTCN3_MODULES) ", fp);
        fputs("$(ASN1_MODULES)", fp);
      }
      fprintf(fp, "\n\n"
            "compile:%s $(TTCN3_MODULES) %s ", makefile->profiled_file_list ? ":" : "",
            makefile->nXSDModules ? "$(XSD2TTCN_GENERATED_MODULES) " : "");
      if (makefile->preprocess) fputs("$(PREPROCESSED_TTCN3_MODULES) ", fp);
      fputs("$(ASN1_MODULES)\n"
            "\t$(TTCN3_DIR)/bin/compiler $(COMPILER_FLAGS) ", fp);
      if (makefile->gnu_make) fputs("$^", fp);
      else {
        fputs("\\\n"
              "\t$(TTCN3_MODULES) ", fp);
        if (makefile->preprocess) fputs("$(PREPROCESSED_TTCN3_MODULES) ", fp);
        fputs("$(ASN1_MODULES)", fp);
      }
      fputs(" - $?\n"
            "\ttouch $@\n"
            "\n", fp);
      if (merge_profiled_file_lists) {
        fputs("$(PROFILED_FILE_LIST): $(PROFILED_FILE_LIST_SEGMENTS)\n"
              "\tcat $(PROFILED_FILE_LIST_SEGMENTS) > $(PROFILED_FILE_LIST)\n\n", fp);
      }
    }
// XSD conversion:
    if (makefile->nXSDModules) {
      fputs("$(XSD2TTCN_GENERATED_MODULES): $(XSD_MODULES)\n"
            "\t$(TTCN3_DIR)/bin/xsd2ttcn", fp);
      if (!has_xsd_module && has_base_xsd_module) {
        fputs(" -m", fp);
      }else {
        fputs(" $(XSD_MODULES)", fp);
      }
      fputs("\n\ttouch $@ \n\n", fp);
      if (makefile->central_storage) {
        fprintf(fp, "$(BASE_XSD2TTCN_GENERATED_MODULES) %s: $(BASE_XSD_MODULES) ",
          makefile->linkingStrategy ? "$(BASE2_XSD2TTCN_GENERATED_MODULES)" : "");
        if (add_refd_prjs) {
          fputs("\n", fp);
          fputs("\t@for dir in $(REFERENCED_PROJECT_DIRS); do \\\n"
                "\t$(MAKE) -C $$dir compile || exit; \\\n"
                "\tdone;", fp);
        }
        fputs("\n\n", fp);
      }
    }
// clean:
    if (makefile->linkingStrategy) {
      fprintf(fp, "clean:%s\n", (add_refd_prjs && !makefile->hierarchical) ?
              " referenced-clean" : "");
      if (makefile->dynamic && (makefile->central_storage || makefile->linkingStrategy)) {
        fprintf(fp,"\tfind . -type l -name \"*.so\" -exec  unlink {} \\;\n");
      }
      fprintf(fp, "\t%s $(EXECUTABLE) $(DYNAMIC_LIBRARY) $(STATIC_LIBRARY) "
        "$(OBJECTS) $(GENERATED_HEADERS) \\\n"
        "\t$(GENERATED_SOURCES) ", rm_command);
      if (makefile->dynamic) fputs("$(SHARED_OBJECTS) ", fp);
      if (makefile->preprocess) fputs("$(PREPROCESSED_TTCN3_MODULES) ", fp);
      fputs("compile", fp);
      if (makefile->central_storage) fputs(" compile-all", fp);
      if (makefile->gcc_dep) fputs(" $(DEPFILES)", fp);
      if (merge_profiled_file_lists) {
        fputs(" $(PROFILED_FILE_LIST)", fp);
      }
      if (makefile->nXSDModules) fputs(" $(XSD2TTCN_GENERATED_MODULES)", fp);
      fprintf(fp, " \\\n"
        "\ttags *.log%s%s\n\n",
        add_refd_prjs?" referenced*":"",
        makefile->hierarchical ? " update":"");
    }
    else {
      fprintf(fp, "clean:%s\n"
        "\t-%s $(EXECUTABLE) $(LIBRARY) $(OBJECTS) $(GENERATED_HEADERS) \\\n"
        "\t$(GENERATED_SOURCES) ", add_refd_prjs?" referenced-clean":"", rm_command);
      if (makefile->dynamic) fputs("$(SHARED_OBJECTS) ", fp);
      if (makefile->preprocess) fputs("$(PREPROCESSED_TTCN3_MODULES) ", fp);
      fputs("compile", fp);
      if (makefile->central_storage) fputs(" compile-all", fp);
      if (makefile->gcc_dep) fputs(" $(DEPFILES)", fp);
      if (merge_profiled_file_lists) {
        fputs(" $(PROFILED_FILE_LIST)", fp);
      }
      if (makefile->nXSDModules) fputs(" $(XSD2TTCN_GENERATED_MODULES)", fp);
      fprintf(fp, " \\\n"
        "\ttags *.log%s",
        add_refd_prjs?" referenced*":"");
    }

// clean-all:
    if (makefile->linkingStrategy && makefile->hierarchical)
      fprintf(fp, "clean-all: %s clean\n", add_refd_prjs ? "referenced-clean-all":"");

// dep:
    fputs("\n\ndep: $(GENERATED_SOURCES) $(USER_SOURCES)",fp);
    if (add_refd_prjs) {
      fprintf(fp, "\n\t%s referenced-dep", rm_command);
    }
    else fputs(" ;",fp);

    if (makefile->gcc_dep) {
      fprintf(fp, " \n\n"
        "ifeq ($(findstring n,$(MAKEFLAGS)),)\n"
        "ifeq ($(filter clean%s check port compile archive diag%s,$(MAKECMDGOALS)),)\n"
        "-include $(DEPFILES)\n"
        "endif\n"
        "endif", 
        (makefile->linkingStrategy && makefile->hierarchical) ? " clean-all" : "",
        (makefile->preprocess ? " preprocess" : ""));
      /* Don't include .d files when cleaning etc.; make will try to build them
       * and this involves running the Titan compiler. Same for preprocess.
       * The check target would be pointless if running the compiler
       * without generating code was always preceded by running the compiler
       * _and_ generating C++ code. */
    }
    else { /* old-style dep with makedepend. Do not check compiler version. */
      fputs("\n\tmakedepend $(CPPFLAGS) -DMAKEDEPEND_RUN ", fp);
      if (makefile->gnu_make) fputs("$^", fp);
      else fputs("$(GENERATED_SOURCES) $(USER_SOURCES)", fp);
    }

    if (makefile->linkingStrategy) {
      fputs("\n\n"
        "archive:\n"
        "\t@perl $(TTCN3_DIR)/bin/ttcn3_archive\n\n", fp);
    }
    else {
      fputs("\n\n"
        "archive:\n"
        "\tmkdir -p $(ARCHIVE_DIR)\n"
        "\ttar -cvhf - ", fp);
      if (makefile->central_storage) {
        if (makefile->nXSDModules) {
          fputs("$(filter-out $(XSD2TTCN_GENERATED_MODULES), $(TTCN3_MODULES)) \\\n", fp);
          fputs("\t$(filter-out $(BASE_XSD2TTCN_GENERATED_MODULES), $(BASE_TTCN3_MODULES)) \\\n", fp);
          fprintf(fp, "%s",
          makefile->linkingStrategy ? "\t$(filter-out $(BASE2_XSD2TTCN_GENERATED_MODULES), $(BASE2_TTCN3_MODULES)) \\\n" : "");
          fputs("\t$(XSD_MODULES) $(BASE_XSD_MODULES) \\\n", fp);
        } else {
          fprintf(fp, "$(TTCN3_MODULES) $(BASE_TTCN3_MODULES) %s\\\n", 
          makefile->linkingStrategy ? "$(BASE2_TTCN3_MODULES) " : "");
        }
        if (makefile->preprocess) {
          fprintf(fp, "\t$(TTCN3_PP_MODULES) $(BASE_TTCN3_PP_MODULES) "
          "%s $(TTCN3_INCLUDES) \\\n",
          makefile->linkingStrategy ? "$(BASE2_TTCN3_PP_MODULES)" : "");
        }
        fprintf(fp, "\t$(ASN1_MODULES) $(BASE_ASN1_MODULES) %s\\\n"
        "\t$(USER_HEADERS) $(BASE_USER_HEADERS) %s\\\n"
        "\t$(USER_SOURCES) $(BASE_USER_SOURCES) %s",
        makefile->linkingStrategy ? "$(BASE2_ASN1_MODULES) " : "",
        makefile->linkingStrategy ? "$(BASE2_USER_HEADERS) " : "",
        makefile->linkingStrategy ? "$(BASE2_USER_SOURCES)" : "");
      }
      else {
        if (makefile->nXSDModules) {
          fputs("$(filter-out $(XSD2TTCN_GENERATED_MODULES), $(TTCN3_MODULES)) ", fp);
          fputs("$(XSD_MODULES) ", fp);
        } else {
          fputs("$(TTCN3_MODULES) ", fp);
        }
        if (makefile->preprocess) {
          fputs("$(TTCN3_PP_MODULES) \\\n"
          "\t$(TTCN3_INCLUDES) ", fp);
        }
        fputs("$(ASN1_MODULES) \\\n"
        "\t$(USER_HEADERS) $(USER_SOURCES)", fp);
      }
      fputs(" $(OTHER_FILES) \\\n"
            "\t| gzip >$(ARCHIVE_DIR)/`basename $(TARGET) .exe`-"
            "`date '+%y%m%d-%H%M'`.tgz\n\n", fp);
    }

    fprintf(fp, "diag:\n"
    "\t$(TTCN3_DIR)/bin/compiler -v 2>&1\n"
    "\t$(TTCN3_DIR)/bin/mctr_cli -v 2>&1\n"
    "\t$(CXX) -v 2>&1\n"
    "%s"
          "\t@echo TTCN3_DIR=$(TTCN3_DIR)\n"
          "\t@echo OPENSSL_DIR=$(OPENSSL_DIR)\n"
          "\t@echo XMLDIR=$(XMLDIR)\n"
          "\t@echo PLATFORM=$(PLATFORM)\n\n",
    makefile->dynamic ? "" : "\t$(AR) -V 2>&1\n");

    if (add_refd_prjs) {
      fprintf(fp, "referenced-all referenced-shared_objects referenced-executable referenced-library \\\n"
              "referenced-objects referenced-check \\\n"
              "referenced-clean%s:\n"
              "\t@for dir in $(REFERENCED_PROJECT_DIRS); do \\\n"
              "\t  $(MAKE) -C $$dir $(subst referenced-,,$@) || exit; \\\n"
              "\tdone; \n\n",
              (makefile->linkingStrategy && makefile->hierarchical) ? "-all" : "");
      fputs("referenced-dep:\n"
            "\t@for dir in $(REFERENCED_PROJECT_DIRS); do \\\n"
            "\t  $(MAKE) -C $$dir $(subst referenced-,,$@) || exit; \\\n"
            "\tdone; \n"
            "\ttouch $@\n\n", fp);
    }

    if (makefile->generatorCommandOutput) {
      fputs("### Project specific rules generated by user written script:\n\n", fp);
      fputs(makefile->generatorCommandOutput, fp);
      fputs("\n### End of project specific rules.\n\n", fp);
    }

    fputs("#\n"
          "# Add your rules here if necessary...\n"
          "#\n\n", fp);
    fclose(fp);
    if (strcmp(makefile->output_file, "Makefile")) {
      NOTIFY("Makefile skeleton was written to `%s'.", makefile->output_file);
    } else {
      NOTIFY("Makefile skeleton was generated.");
    }
  } 
  else {
    ERROR("Output file `%s' already exists. Use switch `%s' to force "
      "overwrite.", 
      makefile->output_file,
      makefile->linkingStrategy ? "-F" : "-f");
  }
}

#undef COMMENT_PREFIX
#define COMMENT_PREFIX

/** run makefilegen commans for sub-projects */
static void run_makefilegen_commands(struct string2_list* run_command_list)
{
  struct string2_list* act_elem = run_command_list;
  while (act_elem) {
    struct string2_list* next_elem = act_elem->next;
    /* run commands if there were no ERRORs */
    if ((error_count == 0) && act_elem->str1 && act_elem->str2) {
      int rv;
      char* sub_proj_effective_work_dir = act_elem->str1;
      char* command = act_elem->str2;
      char* orig_dir = get_working_dir();
      rv = set_working_dir(sub_proj_effective_work_dir);
      if (rv) ERROR("Could not set working dir to `%s'", sub_proj_effective_work_dir);
      else {
        fprintf(stderr, "Executing `%s' in working directory `%s'...\n",
                command, sub_proj_effective_work_dir);
        rv = system(command);
        if (rv) ERROR("Execution failed with error code %d", rv); // TODO: it's not clear what system()'s return codes can be in different situations and platforms
      }
      rv = set_working_dir(orig_dir);
      if (rv) ERROR("Could not restore working dir to `%s'", orig_dir);
      Free(orig_dir);
    }
    Free(act_elem->str1);
    Free(act_elem->str2);
    Free(act_elem);
    act_elem = next_elem;
  }
}

/** execute makefileScript */
static void executeMakefileScript(const char* makefileScript, const char* output_file) {
  char* output_file_name = NULL;
  if (output_file != NULL) {
    if (get_path_status(output_file) == PS_DIRECTORY)
      output_file_name = mprintf("%s/Makefile", output_file);
    else {
      output_file_name = mcopystr(output_file);
    }
  } else {
    output_file_name = mcopystr("Makefile");
  }
  char* tmp_output_file_name = mprintf("%s.tmp", output_file_name);
  
  char* command = mprintf("%s %s %s", makefileScript, output_file_name, tmp_output_file_name);
  fprintf(stderr, "Executing '%s' command\n", command);
  int rv = system(command);
  if (rv) ERROR("MakefileScript execution failed with error code %d", rv);
  
  if(access(output_file_name, W_OK) == -1) {
    ERROR("%s does not exist, needed by makefile modifier script", output_file_name);
    goto free_str;
  }
  if(access(tmp_output_file_name, W_OK) == -1) {
    ERROR("%s does not exist, needed by makefile modifier script", tmp_output_file_name);
    goto free_str;
  }
  // Replace old file with new
  rv = rename(tmp_output_file_name, output_file_name);
  if (rv != 0) {
    ERROR("Moving makefile contents is unsuccessful");
    goto free_str;
  }
  
  fprintf(stderr, "makefile modifier script executed successfully.\n");
 
free_str:
  Free(output_file_name);
  Free(tmp_output_file_name);
  Free(command);
}

/** create symlinks and delete list */
static void generate_symlinks(struct string2_list* create_symlink_list)
{
  struct string2_list* act_elem = create_symlink_list;
  while (act_elem) {
    struct string2_list* next_elem = act_elem->next;
    /* create symlinks if there were no ERRORs */
    if ((error_count == 0) && act_elem->str1 && act_elem->str2) {
      int fail = symlink(act_elem->str1, act_elem->str2);
      if (fail) perror(act_elem->str2); /* complain but do not call ERROR() */
    }
    Free(act_elem->str1);
    Free(act_elem->str2);
    Free(act_elem);
    act_elem = next_elem;
  }
}

/** Performs all tasks of Makefile generation based on the given list of
 * modules/files (taken from the command line) and options that represent
 * command line switches. */
static void generate_makefile(size_t n_arguments, char *arguments[],
  size_t n_other_files, const char *other_files[], const char *output_file,
  const char *ets_name, char *project_name, boolean gnu_make, boolean single_mode,
  boolean central_storage, boolean absolute_paths, boolean preprocess,
  boolean dump_makefile_data, boolean force_overwrite, boolean use_runtime_2,
  boolean dynamic, boolean makedepend, boolean coverage,
  const char *code_splitting_mode, const char *tcov_file_name, struct string_list* profiled_file_list, const char* file_list_file_name,
  boolean Lflag, boolean Zflag, boolean Hflag, struct string_list* sub_project_dirs, struct string_list* ttcn3_prep_includes,
  struct string_list* ttcn3_prep_defines, struct string_list* ttcn3_prep_undefines, struct string_list* prep_includes,
  struct string_list* prep_defines, struct string_list* prep_undefines, char *codesplittpd, boolean quietly, boolean disablesubtypecheck,
  const char* cxxcompiler, const char* optlevel, const char* optflags, boolean disableber, boolean disableraw, boolean disabletext,
  boolean disablexer, boolean disablejson, boolean forcexerinasn, boolean defaultasomit, boolean gccmsgformat,
  boolean linenumbersonlymsg, boolean includesourceinfo, boolean addsourcelineinfo, boolean suppresswarnings,
  boolean outparamboundness, boolean omit_in_value_list, boolean warnings_for_bad_variants, boolean activate_debugger,
  boolean ignore_untagged_on_top_union, boolean disable_predef_ext_folder, struct string_list* solspeclibraries,
  struct string_list* sol8speclibraries, struct string_list* linuxspeclibraries, struct string_list* freebsdspeclibraries,
  struct string_list* win32speclibraries, const char* ttcn3preprocessor, struct string_list* linkerlibraries,
  struct string_list* additionalObjects, struct string_list* linkerlibsearchpath, char* generatorCommandOutput,
  struct string2_list* target_placement_list)
{
  size_t i;

  struct makefile_struct makefile;
  init_makefile_struct(&makefile);

  makefile.project_name = project_name;
  makefile.central_storage = central_storage;
  makefile.gnu_make = gnu_make;
  makefile.preprocess = preprocess;
  makefile.single_mode = single_mode;
  makefile.force_overwrite = force_overwrite;
  makefile.use_runtime_2 = use_runtime_2;
  makefile.dynamic = dynamic;
  makefile.gcc_dep = gnu_make && !makedepend;
  makefile.coverage = coverage;
  makefile.library = Lflag;
  makefile.linkingStrategy = Zflag;
  makefile.hierarchical = Hflag;
  makefile.sub_project_dirs = sub_project_dirs;
  makefile.ttcn3_prep_includes =  ttcn3_prep_includes;
  makefile.ttcn3_prep_defines =  ttcn3_prep_defines;
  makefile.ttcn3_prep_undefines =  ttcn3_prep_undefines;
  makefile.prep_includes =  prep_includes;
  makefile.prep_defines =  prep_defines;
  makefile.prep_undefines =  prep_undefines;
  makefile.quietly = quietly;
  makefile.disablesubtypecheck = disablesubtypecheck;
  makefile.cxxcompiler = cxxcompiler;
  makefile.optlevel = optlevel;
  makefile.optflags = optflags;
  makefile.disableber = disableber;
  makefile.disableraw = disableraw;
  makefile.disabletext = disabletext;
  makefile.disablexer = disablexer;
  makefile.disablejson = disablejson;
  makefile.forcexerinasn = forcexerinasn;
  makefile.defaultasomit = defaultasomit;
  makefile.gccmsgformat = gccmsgformat;
  makefile.linenumbersonlymsg = linenumbersonlymsg;
  makefile.includesourceinfo = includesourceinfo;
  makefile.addsourcelineinfo = addsourcelineinfo;
  makefile.suppresswarnings = suppresswarnings;
  makefile.outparamboundness = outparamboundness;
  makefile.omit_in_value_list = omit_in_value_list;
  makefile.warnings_for_bad_variants = warnings_for_bad_variants;
  makefile.activate_debugger = activate_debugger;
  makefile.ignore_untagged_on_top_union = ignore_untagged_on_top_union;
  makefile.disable_predef_ext_folder = disable_predef_ext_folder;
  makefile.solspeclibraries = solspeclibraries;
  makefile.sol8speclibraries = sol8speclibraries;
  makefile.linuxspeclibraries = linuxspeclibraries;
  makefile.freebsdspeclibraries = freebsdspeclibraries;
  makefile.win32speclibraries = win32speclibraries;
  makefile.ttcn3preprocessor = ttcn3preprocessor;
  makefile.linkerlibraries = linkerlibraries;
  makefile.additionalObjects = additionalObjects;
  makefile.linkerlibsearchpath = linkerlibsearchpath;
  makefile.generatorCommandOutput = generatorCommandOutput;
  makefile.target_placement_list = target_placement_list;
  
  char ** files_from_file = NULL;
  size_t n_files_from_file = 0;
  if (file_list_file_name != NULL) {
    FILE *fp = fopen(file_list_file_name, "r");
    if (fp != NULL) {
      char buff[1024];
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
    }
  }
  for (i = 0; i < n_files_from_file; i++) {
    add_file_to_makefile(&makefile, files_from_file[i]);
  }
  for (i = 0; i < n_arguments; i++) {
    add_file_to_makefile(&makefile, arguments[i]);
  }
  for (i = 0; i < n_other_files; i++) {
    char *file_name = get_file_name_for_argument(other_files[i]);
    if (file_name != NULL) {
      add_path_to_list(&makefile.nOtherFiles, &makefile.OtherFiles, file_name,
      makefile.working_dir, TRUE);
      Free(file_name);
    } else if (get_path_status(other_files[i]) == PS_DIRECTORY) {
      ERROR("Argument `%s' given as other file is a directory.",
      other_files[i]);
    } else {
      ERROR("Cannot find any other file for argument `%s'.", other_files[i]);
    }
  }

  if (ets_name != NULL) {
    char *dir_name = get_dir_name(ets_name, makefile.working_dir);
    char *file_name = get_file_from_path(ets_name);
    makefile.ets_name = compose_path_name(dir_name, file_name);
    Free(dir_name);
    Free(file_name);
  }

  if (code_splitting_mode != NULL) {
    makefile.code_splitting_mode = mputprintf(makefile.code_splitting_mode, "-U %s", code_splitting_mode);
  }
  if (codesplittpd != NULL) { // TPD code splitting overrides console code splitting
    makefile.code_splitting_mode = mputprintf(makefile.code_splitting_mode, "-U %s", codesplittpd);
  }
  
  if (makefile.code_splitting_mode != NULL && makefile.linkingStrategy == TRUE) {
    WARNING("Code splitting from TPD file is not supported when the improved linking method is used (Z flag).");
    Free(makefile.code_splitting_mode);
    makefile.code_splitting_mode = NULL;
  }
  
  if (makefile.code_splitting_mode != NULL && makefile.hierarchical == TRUE) {
    WARNING("Code splitting from TPD file is not supported when hierarchical makefile generation is used (H flag).");
    Free(makefile.code_splitting_mode);
    makefile.code_splitting_mode = NULL;
  }

  if (tcov_file_name != NULL) {
    makefile.tcov_file_name = mprintf(" -K %s", tcov_file_name);
  }
  
  if (profiled_file_list != NULL) {
    makefile.profiled_file_list = profiled_file_list;
  }

  if (makefile.nTTCN3Modules >= 1) {
    if (makefile.ets_name == NULL)
      makefile.ets_name = mcopystr(makefile.TTCN3Modules[0].module_name);
  } else if (preprocess && (makefile.nTTCN3PPModules >= 1)) {
    if (makefile.ets_name == NULL)
      makefile.ets_name = mcopystr(makefile.TTCN3PPModules[0].module_name);
  } else if (makefile.nASN1Modules >= 1) {
    WARNING("No TTCN-3 module was given for the Makefile.");
    if (makefile.ets_name == NULL)
      makefile.ets_name = mcopystr(makefile.ASN1Modules[0].module_name);
  } else if (makefile.nXSDModules >= 1) {
    WARNING("No TTCN-3 or ASN.1 was given for the Makefile.");
    if (makefile.ets_name == NULL)
      makefile.ets_name = mcopystr(makefile.XSDModules[0].module_name);
  } else if (makefile.nUserFiles > 0) {
    WARNING("No TTCN-3 or ASN.1 or XSD module was given for the Makefile.");
    if (makefile.ets_name == NULL)
      makefile.ets_name = mcopystr(makefile.UserFiles[0].file_prefix);
  } else {
    WARNING("No source files were given for the Makefile");
  }

  if (output_file != NULL) {
    if (get_path_status(output_file) == PS_DIRECTORY)
      makefile.output_file = mprintf("%s/Makefile", output_file);
    else makefile.output_file = mcopystr(output_file);
  } else makefile.output_file = mcopystr("Makefile");
  add_path_to_list(&makefile.nOtherFiles, &makefile.OtherFiles,
    makefile.output_file, makefile.working_dir, FALSE);
  
  if (preprocess) check_preprocessed_filename_collision(&makefile);
  filter_out_generated_files(&makefile);
  complete_user_files(&makefile);
  if (!absolute_paths) convert_dirs_to_relative(&makefile);
  check_special_chars(&makefile);
  if (central_storage) collect_base_dirs(&makefile);
  check_naming_convention(&makefile);

  if (dump_makefile_data) dump_makefile_struct(&makefile, 0);

  if (error_count == 0) print_makefile(&makefile);
  for (i = 0; i < n_files_from_file; i++) {
    Free(files_from_file[i]);
  }
  Free(files_from_file);
  free_makefile_struct(&makefile);
}

#ifdef COVERAGE_BUILD
#define C_flag    "C"
#else
#define C_flag
#endif


static void usage(void)
{
  fprintf(stderr, "\n"
    "usage: %s [-abc" C_flag "dDEfFglLmMnNprRsStTVwWXZ] [-K file] [-z file ] [-P dir]"
    " [-J file] [-U none|type|'number'] [-e ets_name] [-o dir|file]\n"
    "        [-t project_descriptor.tpd [-b buildconfig]]\n"
    "        [-O file] ... module_name ... testport_name ...\n"
    "   or  %s -v\n"
    "\n"
    "OPTIONS:\n"
    "	-a:		use absolute pathnames in the generated Makefile\n"
    "	-c:		use the pre-compiled files from central directories\n"
#ifdef COVERAGE_BUILD
    "	-C:		enable coverage of generated C++ code\n"
#endif
    "	-d:		dump the data used for Makefile generation\n"
    "	-e ets_name:	name of the target executable\n"
    "	-E:		display only warnings for unrecognized encoding variants\n"
    "	-f:		force overwriting of the output Makefile\n"
    "	-g:		generate Makefile for use with GNU make\n"
    "	-I path:	Add path to the search paths when using TPD files\n"
    "	-J file:	The names of files taken from file instead of command line"
    "	-K file:	enable selective code coverage\n"
    "	-l:		use dynamic linking\n"
    "	-L:		create makefile with library archive as the default target\n"
    "	-m:		always use makedepend for dependencies\n"
    "	-M:		allow 'omit' in template value lists (legacy behavior)\n"
    "	-n:		activate debugger (generates extra code for debugging)\n"
    "	-N:		ignore UNTAGGED encoding instruction on top level unions (legacy behaviour)\n"
    "	-o dir|file:	write the Makefile to the given directory or file\n"
    "	-O file:	add the given file to the Makefile as other file\n"
    "	-p:		generate Makefile with TTCN-3 preprocessing\n"
    "	-R:		use function test runtime (TITAN_RUNTIME_2)\n"
    "	-s:		generate Makefile for single mode\n"
    "	-S:		suppress makefilegen warnings\n"
    "	-U none|type|'number':	split generated code\n"
    "	-v:		show version\n"
    "	-w:		suppress warnings generated by TITAN\n"
    "	-Y:		Enforces legacy behaviour of the \"out\" function parameters (see refguide)\n"
    "	-z file:	enable profiling and code coverage for the TTCN-3 files in the argument\n"
    "Options for processing the Titan Project Descriptor file(s):\n"
    "	-t tpd:		read project descriptor file\n"
    "	-b buildconfig:	use the specified build config instead of the default\n"
    "	-D:		use current directory as working directory\n"
    "	-V:		disable validation of TPD file with schema\n"
    "	-r:		generate Makefile hierarchy for TPD hierarchy (recursive)\n"
    "	-F:		force overwriting of all generated Makefiles, use with -r\n"
    "	-T:		generate only top-level Makefile of the hierarchy, use with -r\n"
    "	-P dir:		prints out a file list found in a given TPD relative to the given directory\n"
    "	-X:		generate XML file that describes the TPD hierarchy, use with -r\n"
    "	-W:		prefix working directories with project name\n"
    "	-Z:		recursive Makefile generation from TPD using object files and dynamic libraries too\n"
    "	-H:		hierarchical Makefile generation from TPD use with -Z\n"
    , program_name, program_name);
}

#define SET_FLAG(x) if (x##flag) {\
    ERROR("Flag -" #x " was specified more than once.");\
    error_flag = TRUE;\
    } else x##flag = TRUE

int main(int argc, char *argv[])
{
  boolean
    aflag = FALSE, bflag = FALSE, cflag = FALSE, Cflag = FALSE,
    dflag = FALSE, eflag = FALSE, fflag = FALSE, gflag = FALSE,
    oflag = FALSE, Kflag = FALSE, lflag = FALSE, pflag = FALSE,
    Pflag = FALSE, Rflag = FALSE, sflag = FALSE, tflag = FALSE,
    wflag = FALSE, vflag = FALSE, mflag = FALSE, Uflag = FALSE,
    Lflag = FALSE, rflag = FALSE, Fflag = FALSE, Xflag = FALSE,
    Tflag = FALSE, Yflag = FALSE, quflag = FALSE,
    dsflag = FALSE, dbflag = FALSE, drflag = FALSE, dtflag = FALSE,
    dxflag = FALSE, fxflag = FALSE, doflag = FALSE,
    gfflag = FALSE, lnflag = FALSE, isflag = FALSE, asflag = FALSE,
    Sflag = FALSE, Vflag = FALSE, Dflag = FALSE, Wflag = FALSE,
    djflag = FALSE, Zflag = FALSE, Hflag = FALSE, Mflag = FALSE,
    diflag = FALSE, zflag = FALSE, Eflag = FALSE, nflag = FALSE,
    Nflag = FALSE;
  boolean error_flag = FALSE;
  char *output_file = NULL;
  char *ets_name = NULL;
  char *project_name = NULL;
  char *csmode = NULL;
  size_t n_other_files = 0;
  const char **other_files = NULL;
  const char *code_splitting_mode = NULL;
  const char *tpd_file_name = NULL;
  const char *tpd_build_config = NULL;
  const char *tcov_file_name = NULL;
  const char *file_list_file_name = NULL;
  size_t n_search_paths = 0;
  const char **search_paths = NULL;
  struct string_list* profiled_file_list = NULL;
  const char *profiled_file_list_zflag = NULL;
  const char *file_list_path = NULL;
  enum tpd_result tpd_processed = FALSE;
  struct string_list* sub_project_dirs = NULL;
  struct string2_list* create_symlink_list = NULL;
  struct string_list* ttcn3_prep_includes = NULL;
  struct string_list* ttcn3_prep_defines = NULL;
  struct string_list* ttcn3_prep_undefines = NULL;
  struct string_list* prep_includes = NULL;
  struct string_list* prep_defines = NULL;
  struct string_list* prep_undefines = NULL;
  char *cxxcompiler = NULL;
  char *optlevel = NULL;
  char *optflags = NULL;
  struct string_list* solspeclibraries = NULL;
  struct string_list* sol8speclibraries = NULL;
  struct string_list* linuxspeclibraries = NULL;
  struct string_list* freebsdspeclibraries = NULL;
  struct string_list* win32speclibraries = NULL;
  char *ttcn3prep = NULL;
  struct string_list* linkerlibraries = NULL;
  struct string_list* additionalObjects = NULL;
  struct string_list* linkerlibsearchpath = NULL;
  char* generatorCommandOutput = NULL;
  struct string2_list* target_placement_list = NULL;
  struct string2_list* run_command_list = NULL;
  struct string2_list* required_configs = NULL;
  char* makefileScript = NULL;

#ifdef LICENSE
  license_struct lstr;
  int valid_license;
#endif

  program_name = argv[0];

  if (argc == 1) {
    fputs("Makefile Generator for the TTCN-3 Test Executor, version "
      PRODUCT_NUMBER "\n", stderr);
    usage();
    return EXIT_FAILURE;
  }

  for ( ; ; ) {
    int c = getopt(argc, argv, "O:ab:c" C_flag "dDe:EfFgI:J:K:o:lLmMnNpP:rRsSt:TU:vVwWXYz:ZH");
    if (c == -1) break;
    switch (c) {
    case 'O':
      n_other_files++;
      other_files = (const char**)
      Realloc(other_files, n_other_files * sizeof(*other_files));
      other_files[n_other_files - 1] = optarg;
      break;
      case 'I':
        n_search_paths++;
        search_paths = (const char**)
        Realloc(search_paths, n_search_paths * sizeof(*search_paths));
        search_paths[n_search_paths - 1] = optarg;
      break;
    case 'a':
      SET_FLAG(a);
      break;
    case 'b':
      SET_FLAG(b);
      tpd_build_config = optarg;
      break;
    case 'c':
      SET_FLAG(c);
      break;
    case 'K':
      SET_FLAG(K);
      tcov_file_name = optarg;
      break;
#ifdef COVERAGE_BUILD
    case 'C':
      SET_FLAG(C);
      break;
#endif
    case 'd':
      SET_FLAG(d);
      break;
    case 'D':
      SET_FLAG(D);
      break;
    case 'e':
      SET_FLAG(e);
      ets_name = optarg;
      break;
    case 'E':
      SET_FLAG(E);
      break;
    case 'f':
      SET_FLAG(f);
      break;
    case 'F':
      SET_FLAG(F);
      break;
    case 'g':
      SET_FLAG(g);
      break;
    case 'H':
      SET_FLAG(H);
      break;
    case 'J':
      file_list_file_name = optarg;
      break;
    case 'o':
      SET_FLAG(o);
      output_file = optarg;
      break;
    case 'l':
      SET_FLAG(l);
      break;
    case 'L':
      SET_FLAG(L);
      break;
    case 'm':
      SET_FLAG(m);
      break;
    case 'M':
      SET_FLAG(M);
      break;
    case 'n':
      SET_FLAG(n);
      break;
    case 'N':
      SET_FLAG(N);
      break;
    case 'p':
      SET_FLAG(p);
      break;
    case 'P':
      SET_FLAG(P);
      /* Optional arguments with `::' are GNU specific... */
      if (get_path_status(optarg) == PS_DIRECTORY) {
        file_list_path = optarg;
      } else {
        ERROR("The -P flag requires a valid directory as its argument "
              "instead of `%s'", optarg);
        error_flag = TRUE;
      }
      break;
    case 'r':
      SET_FLAG(r);
      break;
    case 'R':
      SET_FLAG(R);
      break;
    case 's':
      SET_FLAG(s);
      break;
    case 'S':
      SET_FLAG(S);
      suppress_warnings = TRUE;
      break;
    case 't':
      SET_FLAG(t);
      tpd_file_name = optarg;
      break;
    case 'T':
      SET_FLAG(T);
      break;
    case 'Y':
      SET_FLAG(Y);
      break;
    case 'U':
      SET_FLAG(U);
      int n_slices = atoi(optarg);
      code_splitting_mode = optarg;
      if (!n_slices && 
        (strcmp(optarg, "none") != 0 &&
        strcmp(optarg, "type") != 0))
      {
        ERROR("Unrecognizable argument: '%s'. Valid options for -U switch are: "
          "'none', 'type', or a number.", optarg);
      } else if (n_slices) {
        size_t length = strlen(optarg);
        for (size_t i=0;i<length; i++) {
          if (!isdigit(optarg[i])) {
            ERROR("The number argument of -U must be a valid number.");
            break;
          }
        }
        if (n_slices < 1 || n_slices > 999999) {
          ERROR("The number argument of -U must be between 1 and 999999.");
        }
      }
      break;
    case 'v':
      SET_FLAG(v);
      break;
    case 'V':
      SET_FLAG(V);
      break;
    case 'w':
      SET_FLAG(w);
      break;
    case 'W':
      SET_FLAG(W);
      break;
    case 'X':
      SET_FLAG(X);
      break;
    case 'z':
      SET_FLAG(z);
      profiled_file_list_zflag = optarg;
      break;
    case 'Z':
      SET_FLAG(Z);
      break;
    default:
      error_flag = TRUE;
      break;
    }
  }

  /* Checking incompatible options */
  if (vflag) {
    /* -v prints the version and exits, it's pointless to specify other flags */
    if ( aflag || bflag || cflag || Cflag || dflag || eflag || fflag || Fflag || gflag
      || mflag || oflag || lflag || pflag || Pflag || rflag || Rflag || sflag
      || tflag || Tflag || Vflag || wflag || Xflag || Kflag || Dflag || Wflag || Yflag
      || Zflag || Hflag || Mflag || zflag || Eflag || nflag || n_other_files > 0 || n_search_paths > 0)
      error_flag = TRUE;
  }

  if (Zflag) {
    if (!gflag) gflag = TRUE; // GNU make
    if (!cflag) cflag = TRUE; // central sorage 
  }

  if ((bflag || Dflag || Pflag || Vflag || rflag || Wflag || Zflag) && !tflag) {
    ERROR("Using the '-b', '-D', '-P', '-V', '-r' 'Z' or '-W' option requires the use of the -t' option.");
    error_flag = TRUE;
  }

  if (rflag && !cflag) {
    ERROR("Using the '-r' option requires use of the '-c' option. Recursive makefile hierarchy uses the central directory feature.");
    error_flag = TRUE;
  }

  if (Fflag && !rflag) {
    ERROR("Using the '-F' option requires use of the '-r' option.");
    error_flag = TRUE;
  }

  if (Xflag && !rflag) {
    ERROR("Using the '-X' option requires use of the '-r' option.");
    error_flag = TRUE;
  }

  if (Tflag && !rflag) {
    ERROR("Using the '-T' option requires use of the '-r' option.");
    error_flag = TRUE;
  }

  if (!Zflag && Hflag) {
    ERROR("Using the '-H' option requires use of the '-Z' option.");
    error_flag = TRUE;
  }

  if (Zflag && !Fflag && !fflag) {
    ERROR("Using the '-Z' option requires use of the '-F' option.");
    error_flag = TRUE;
  }

  if (lflag && !strncmp(get_platform_string(), "WIN32", 5)) {
    ERROR("Generating Makefile with dynamic linking enabled is not supported "
          "on Windows platform");
    error_flag = TRUE;
  }

  if (n_search_paths > 0 && !tflag) {
    ERROR("Using the '-I' option requires use of the '-t' option.");
    error_flag = TRUE;
  }
  
  for (size_t i = 0; i < n_search_paths; i++) {
    boolean is_abs_path = 
#if defined WIN32 && defined MINGW
	/* On native Windows the absolute path name shall begin with
	 * a drive letter, colon and backslash */
	(((search_paths[i][0] < 'A' || search_paths[i][0] > 'Z') &&
	  (search_paths[i][0] < 'a' || search_paths[i][0] > 'z')) ||
	 search_paths[i][1] != ':' || search_paths[i][2] != '\\');
#else
	/* On UNIX-like systems the absolute path name shall begin with
	 * a slash */
	search_paths[i][0] != '/';
#endif
    if (is_abs_path) {
      ERROR("The path after the -I flag must be an absolute path.");
      error_flag = TRUE;
    }
  }
  
  if (error_flag) {
    usage();
    Free(other_files);
    Free(search_paths);
    return EXIT_FAILURE;
  }

  if (vflag) {
    fputs("Makefile Generator for the TTCN-3 Test Executor\n"
	  "Product number: " PRODUCT_NUMBER "\n"
	  "Build date: " __DATE__ " " __TIME__ "\n"
	  "Compiled with: " C_COMPILER_VERSION "\n\n"
	  COPYRIGHT_STRING "\n\n", stderr);
#ifdef LICENSE
    print_license_info();
#endif
    Free(other_files);
    Free(search_paths);
    return EXIT_SUCCESS;
  }

#ifdef LICENSE
  init_openssl();
  load_license(&lstr);
  valid_license = verify_license(&lstr);
  free_openssl();
  if (!valid_license) {
    free_license(&lstr);
    Free(other_files);
    Free(search_paths);
    exit(EXIT_FAILURE);
  }
  if (!check_feature(&lstr, FEATURE_TPGEN)) {
    ERROR("The license key does not allow the generation of "
          "Makefile skeletons.");
    Free(other_files);
    Free(search_paths);
    return EXIT_FAILURE;
  }
  free_license(&lstr);
#endif

  boolean free_argv = FALSE;
  if (tflag) {
    char* abs_work_dir = NULL;
    FILE* prj_graph_fp = NULL;
    sub_project_dirs = (struct string_list*)Malloc(sizeof(struct string_list));
    sub_project_dirs->str = NULL;
    sub_project_dirs->next = NULL;
    ttcn3_prep_includes = (struct string_list*)Malloc(sizeof(struct string_list));
    ttcn3_prep_includes->str = NULL;
    ttcn3_prep_includes->next = NULL;
    ttcn3_prep_defines = (struct string_list*)Malloc(sizeof(struct string_list));
    ttcn3_prep_defines->str = NULL;
    ttcn3_prep_defines->next = NULL;
    ttcn3_prep_undefines = (struct string_list*)Malloc(sizeof(struct string_list));
    ttcn3_prep_undefines->str = NULL;
    ttcn3_prep_undefines->next = NULL;
    prep_includes = (struct string_list*)Malloc(sizeof(struct string_list));
    prep_includes->str = NULL;
    prep_includes->next = NULL;
    prep_defines = (struct string_list*)Malloc(sizeof(struct string_list));
    prep_defines->str = NULL;
    prep_defines->next = NULL;
    prep_undefines = (struct string_list*)Malloc(sizeof(struct string_list));
    prep_undefines->str = NULL;
    prep_undefines->next = NULL;
    solspeclibraries = (struct string_list*)Malloc(sizeof(struct string_list));
    solspeclibraries->str = NULL;
    solspeclibraries->next = NULL;
    sol8speclibraries = (struct string_list*)Malloc(sizeof(struct string_list));
    sol8speclibraries->str = NULL;
    sol8speclibraries->next = NULL;
    linuxspeclibraries = (struct string_list*)Malloc(sizeof(struct string_list));
    linuxspeclibraries->str = NULL;
    linuxspeclibraries->next = NULL;
    freebsdspeclibraries = (struct string_list*)Malloc(sizeof(struct string_list));
    freebsdspeclibraries->str = NULL;
    freebsdspeclibraries->next = NULL;
    win32speclibraries = (struct string_list*)Malloc(sizeof(struct string_list));
    win32speclibraries->str = NULL;
    win32speclibraries->next = NULL;
    linkerlibraries = (struct string_list*)Malloc(sizeof(struct string_list));
    linkerlibraries->str = NULL;
    linkerlibraries->next = NULL;
    additionalObjects = (struct string_list*)Malloc(sizeof(struct string_list));
    additionalObjects->str = NULL;
    additionalObjects->next = NULL;
    linkerlibsearchpath = (struct string_list*)Malloc(sizeof(struct string_list));
    linkerlibsearchpath->str = NULL;
    linkerlibsearchpath->next = NULL;

    if (Xflag) {
      const char* prj_graph_filename = "project_hierarchy_graph.xml";
      prj_graph_fp = fopen(prj_graph_filename, "w");
      if (prj_graph_fp==NULL) WARNING("Cannot open output file `%s' for writing: %s", prj_graph_filename, strerror(errno));
      if (prj_graph_fp) fprintf(prj_graph_fp, "<project_hierarchy_graph top_level_tpd=\"%s\">\n", tpd_file_name);
    }
    create_symlink_list = (struct string2_list*)Malloc(sizeof(struct string2_list));
    create_symlink_list->str1 = NULL;
    create_symlink_list->str2 = NULL;
    create_symlink_list->next = NULL;
    target_placement_list = (struct string2_list*)Malloc(sizeof(struct string2_list));
    target_placement_list->str1 = NULL;
    target_placement_list->str2 = NULL;
    target_placement_list->next = NULL;
    run_command_list = (struct string2_list*)Malloc(sizeof(struct string2_list));
    run_command_list->str1 = NULL;
    run_command_list->str2 = NULL;
    run_command_list->next = NULL;
    required_configs = (struct string2_list*)Malloc(sizeof(struct string2_list));
    required_configs->str1 = NULL;
    required_configs->str2 = NULL;
    required_configs->next = NULL;

    // This temp flag is used to get the value of suppressWarnings from the TPD
    // while the wflag still holds the value of the command line parameter -w
    boolean temp_wflag = FALSE;

    tpd_processed = process_tpd(&tpd_file_name, tpd_build_config, file_list_path,
      &argc, &argv, &free_argv, &optind, &ets_name, &project_name,
      &gflag, &sflag, &cflag, &aflag, &pflag,
      &Rflag, &lflag, &mflag, &Pflag, &Lflag, rflag, Fflag, Tflag, output_file, &abs_work_dir, sub_project_dirs, program_name, prj_graph_fp,
      create_symlink_list, ttcn3_prep_includes, ttcn3_prep_defines, ttcn3_prep_undefines, prep_includes, prep_defines, prep_undefines, &csmode,
      &quflag, &dsflag, &cxxcompiler, &optlevel, &optflags, &dbflag, &drflag, &dtflag, &dxflag, &djflag, &fxflag, &doflag, &gfflag, &lnflag, &isflag,
      &asflag, &temp_wflag, &Yflag, &Mflag, &Eflag, &nflag, &Nflag, &diflag, solspeclibraries, sol8speclibraries, linuxspeclibraries, freebsdspeclibraries, win32speclibraries, &ttcn3prep,
      linkerlibraries, additionalObjects, linkerlibsearchpath, Vflag, Dflag, &Zflag, &Hflag,
      &generatorCommandOutput, target_placement_list, Wflag, run_command_list, required_configs, &profiled_file_list, search_paths, n_search_paths, &makefileScript);
    
    // wflag overrides temp_wflag
    if (!wflag) {
      wflag = temp_wflag;
    }

    if (prj_graph_fp) {
      fprintf(prj_graph_fp, "</project_hierarchy_graph>\n");
      fclose(prj_graph_fp);
    }
    if (tpd_processed == TPD_FAILED) {
      ERROR("Failed to process %s", tpd_file_name);
      // process_tpd has already cleaned everything up
      return EXIT_FAILURE;
    }
    else {
      Free(abs_work_dir);
    }
    if (zflag) {
      WARNING("Compiler option '-z' and its argument will be overwritten by "
        "the settings in the TPD");
    }
  }
  else if (zflag) {
    // use the argument given in the command line if there is no TPD
    profiled_file_list = (struct string_list*)Malloc(sizeof(struct string_list));
    profiled_file_list->str = mcopystr(profiled_file_list_zflag);
    profiled_file_list->next = NULL;
  }

  if (!Pflag) {
    run_makefilegen_commands(run_command_list);
    generate_symlinks(create_symlink_list);
    if (Zflag) {
      if (Fflag)
        NOTIFY("Makefile generation from top-level TPD: %s", tpd_file_name);
      if (!Fflag && fflag)
        NOTIFY("Makefile generation from lower level TPD: %s", tpd_file_name);
    }
    generate_makefile(argc - optind, argv + optind, n_other_files, other_files,
      output_file, ets_name, project_name, gflag, sflag, cflag, aflag, pflag, dflag, fflag||Fflag,
      Rflag, lflag, mflag, Cflag, code_splitting_mode, tcov_file_name, profiled_file_list,
      file_list_file_name, Lflag, Zflag, Hflag, rflag ? sub_project_dirs : NULL, ttcn3_prep_includes,
      ttcn3_prep_defines, ttcn3_prep_undefines, prep_includes, prep_defines, prep_undefines, csmode, quflag, dsflag, cxxcompiler, optlevel, optflags, dbflag,
      drflag, dtflag, dxflag, djflag, fxflag, doflag, gfflag, lnflag, isflag, asflag, wflag, Yflag, Mflag, Eflag, nflag, Nflag, diflag, solspeclibraries,
      sol8speclibraries, linuxspeclibraries, freebsdspeclibraries, win32speclibraries, ttcn3prep, linkerlibraries, additionalObjects,
      linkerlibsearchpath, generatorCommandOutput, target_placement_list);
    if (makefileScript != NULL) {
      executeMakefileScript(makefileScript, output_file);
    }
  }
  else {
    free_string2_list(run_command_list);
    free_string2_list(create_symlink_list);
    Free(project_name);
  }

  free_string_list(sub_project_dirs);
  free_string_list(ttcn3_prep_includes);
  free_string_list(ttcn3_prep_defines);
  free_string_list(ttcn3_prep_undefines);
  free_string_list(prep_includes);
  free_string_list(prep_defines);
  free_string_list(prep_undefines);
  free_string_list(solspeclibraries);
  free_string_list(sol8speclibraries);
  free_string_list(linuxspeclibraries);
  free_string_list(freebsdspeclibraries);
  free_string_list(win32speclibraries);
  free_string_list(linkerlibraries);
  free_string_list(additionalObjects);
  free_string_list(linkerlibsearchpath);
  free_string_list(profiled_file_list);

  Free(search_paths);
  Free(csmode);

  Free(generatorCommandOutput);
  free_string2_list(target_placement_list);
  free_string2_list(required_configs);
  Free(makefileScript);
  
  Free(other_files);
  if (tpd_processed == TPD_SUCCESS) {
    if (!(eflag && ets_name))
      Free(ets_name);
    if (cxxcompiler)
      Free(cxxcompiler);
    if (optlevel)
      Free(optlevel);
    if (optflags)
      Free(optflags);
    if (ttcn3prep)
      Free(ttcn3prep);
  /* Free(output_file); */
  }
  if (free_argv) {
    int E;
    for (E = 0; E < argc; ++E) Free(argv[E]);
    Free(argv);
  }
   /* check_mem_leak(program_name); not needed when linked to new.cc */
  return error_count > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

