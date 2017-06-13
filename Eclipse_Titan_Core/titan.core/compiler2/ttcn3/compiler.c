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
 *   Cserveni, Akos
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Kremer, Peter
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szabo, Bence Janos
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
/* Main program for the TTCN-3 compiler */

/* C declarations */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../../common/memory.h"
#include "../../common/version_internal.h"
#include "../../common/path.h"
#include "../../common/userinfo.h"
#include "compiler.h"
#include "../main.hh"
#include "../error.h"
#include "profiler.h"

#ifdef LICENSE
#include "../../common/license.h"
#endif

#define BUFSIZE 1024

#undef COMMENT_PREFIX
#define COMMENT_PREFIX "// "

static unsigned int nof_updated_files=0;
static unsigned int nof_uptodate_files=0;
unsigned int nof_notupdated_files=0;

static boolean skip_over_lines(FILE *fp, size_t nof_lines)
{
    size_t i = 0;
    while (i < nof_lines) {
	switch (getc(fp)) {
	case EOF:
	    return TRUE;
	case '\n':
	    i++;
	default:
	    break;
	}
    }
    return FALSE;
}

static boolean update_file(const char *file_name, FILE *fp, size_t skip_lines)
{
    boolean files_differ = FALSE;

    FILE *target = fopen(file_name, "r");
    if (target == NULL) files_differ = TRUE;

    if (!files_differ) files_differ = skip_over_lines(target, skip_lines);

    if (!files_differ) {
        if (fseek(fp, 0L, SEEK_SET)) {
	    ERROR("Seek operation failed on a temporary file when trying to "
		"update output file `%s': %s", file_name, strerror(errno));
	}
	files_differ = skip_over_lines(fp, skip_lines);
    }

    if (!files_differ) {
	/* compare the rest of the files */
        for ( ; ; ) {
	    char buf1[BUFSIZE], buf2[BUFSIZE];
	    int len1 = fread(buf1, 1, BUFSIZE, fp);
	    int len2 = fread(buf2, 1, BUFSIZE, target);
	    if ((len1 != len2) || memcmp(buf1, buf2, len1)) {
		files_differ = TRUE;
		break;
	    } else if (len1 < BUFSIZE) break;
	}
    }

    if (target != NULL) fclose(target);

    if (files_differ) {
        if (fseek(fp, 0L, SEEK_SET)) {
	    ERROR("Seek operation failed on a temporary file when trying to "
		"update output file `%s': %s", file_name, strerror(errno));
	}
	target = fopen(file_name, "w");
	if (target == NULL) {
	    ERROR("Cannot open output file `%s' for writing: %s", file_name,
		strerror(errno));
	    exit(EXIT_FAILURE);
	}
        /* copy the contents of fp into target */
	for ( ; ; ) {
	    char buf[BUFSIZE];
	    size_t len = fread(buf, 1, BUFSIZE, fp);
	    if (fwrite(buf, 1, len, target) != len) {
		ERROR("Cannot write to output file `%s': %s", file_name,
		    strerror(errno));
		exit(EXIT_FAILURE);
	    }
	    if (len < BUFSIZE) break;
	}
        fclose(target);
    }

    return files_differ;
}

FILE *open_output_file(const char *file_name, boolean *is_temporary)
{
    FILE *fp;
    switch (get_path_status(file_name)) {
    case PS_FILE:
	if (force_overwrite) *is_temporary = FALSE;
	else *is_temporary = TRUE;
	break;
    case PS_DIRECTORY:
	ERROR("Output file `%s' is a directory", file_name);
	exit(EXIT_FAILURE);
	break;
    case PS_NONEXISTENT:
	*is_temporary = FALSE;
    }
    if (*is_temporary) {
	fp = tmpfile();
	if (fp == NULL) {
	    ERROR("Creation of a temporary file failed when trying to update "
		"output file `%s': %s", file_name, strerror(errno));
	    exit(EXIT_FAILURE);
	}
    } else {
	fp = fopen(file_name, "w");
	if (fp == NULL) {
	    ERROR("Cannot open output file `%s' for writing: %s", file_name,
		strerror(errno));
	    exit(EXIT_FAILURE);
	}
    }
    return fp;
}

void close_output_file(const char *file_name, FILE *fp,
    boolean is_temporary, size_t skip_lines)
{
    if (is_temporary) {
	if (update_file(file_name, fp, skip_lines)) {
	    NOTIFY("File `%s' was updated.", file_name);
	    nof_updated_files++;
	} else {
	    if (display_up_to_date) {
	      NOTIFY("File `%s' was up-to-date.", file_name);
	    } else {
	      DEBUG(1, "File `%s' was up-to-date.", file_name);
	    }
	    nof_uptodate_files++;
	}
    } else {
	NOTIFY("File `%s' was generated.", file_name);
	nof_updated_files++;
    }
    fclose(fp);
}

void write_output(output_struct *output, const char *module_name,
  const char *module_dispname, const char *filename_suffix, boolean is_ttcn,
  boolean has_circular_import, boolean is_module)
{
    char *header_name, *source_name, *user_info;
    FILE *fp;
    boolean is_temporary;

    if (output_dir != NULL) {
	header_name = mprintf("%s/%s%s.hh", output_dir,
	    duplicate_underscores ? module_name : module_dispname,
	    filename_suffix);
	source_name = mprintf("%s/%s%s.cc", output_dir,
	    duplicate_underscores ? module_name : module_dispname,
	    filename_suffix);
    } else {
	header_name = mprintf("%s%s.hh",
	    duplicate_underscores ? module_name : module_dispname,
	    filename_suffix);
	source_name = mprintf("%s%s.cc",
	    duplicate_underscores ? module_name : module_dispname,
	    filename_suffix);
    }
    user_info = get_user_info();

    if (is_module) {

    DEBUG(1, "Writing file `%s' ... ", header_name);

    fp = open_output_file(header_name, &is_temporary);

    fprintf(fp, "// This C++ header file was generated by the %s compiler\n"
	"// of the TTCN-3 Test Executor version " PRODUCT_NUMBER "\n"
	"// for %s\n"
	COPYRIGHT_STRING "\n\n"
	"// Do not edit this file unless you know what you are doing.\n\n"
	"#ifndef %s_HH\n"
	"#define %s_HH\n",
	is_ttcn ? "TTCN-3" : "ASN.1",
	user_info, module_name, module_name);

  /* check if the generated code matches the runtime */
  fprintf(fp, "\n"
    "#if%sdef TITAN_RUNTIME_2\n"
    "#error Generated code does not match with used runtime.\\\n"
    " %s\n"
    "#endif\n",
    use_runtime_2 ? "n" : "",
    use_runtime_2 ?
      "Code was generated with -R option but -DTITAN_RUNTIME_2 was not used.":
      "Code was generated without -R option but -DTITAN_RUNTIME_2 was used.");

    /* #include directives must be the first in order to make GCC's
     * precompiled headers working. */
    if (output->header.includes != NULL) {
	fputs("\n/* Header file includes */\n\n", fp);
	fputs(output->header.includes, fp);
	fprintf(fp, "\n"
	    "#if TTCN3_VERSION != %d\n"
	    "#error Version mismatch detected.\\\n"
	    " Please check the version of the %s compiler and the "
	    "base library.\\\n"
	    " Run make clean and rebuild the project if the version of the compiler changed recently.\n"
	    "#endif\n",
	    TTCN3_VERSION, is_ttcn ? "TTCN-3" : "ASN.1");
	/* Check the platform matches */
#if defined SOLARIS || defined SOLARIS8
	fputs("#if !defined SOLARIS && !defined SOLARIS8 \n",fp);
	fputs("  #error This file should be compiled on SOLARIS or SOLARIS8 \n",fp);
	fputs("#endif \n",fp);

#else
	fprintf(fp, "\n"
	    "#ifndef %s\n"
	    "#error This file should be compiled on %s\n"
	    "#endif\n", expected_platform, expected_platform);
#endif
    }



	if (!has_circular_import)
	fprintf(fp, "\nnamespace %s {\n", module_name);

    if (output->header.class_decls != NULL) {
	if (has_circular_import) {
	    fprintf(fp, "\n#undef %s_HH\n"
		"#endif\n", module_name);
		fprintf(fp, "\nnamespace %s {\n", module_name);
	}
	fputs("\n/* Forward declarations of classes */\n\n",fp);
	fputs(output->header.class_decls, fp);
	if (has_circular_import) {
		fputs("\n} /* end of namespace */\n", fp);
	    fprintf(fp, "\n#ifndef %s_HH\n"
		"#define %s_HH\n", module_name, module_name);
	}
    }

    if (has_circular_import)
	fprintf(fp, "\nnamespace %s {\n", module_name);

    if (output->header.typedefs != NULL) {
	fputs("\n/* Type definitions */\n\n", fp);
	fputs(output->header.typedefs, fp);
    }

    if (output->header.class_defs != NULL) {
	fputs("\n/* Class definitions */\n\n", fp);
	fputs(output->header.class_defs, fp);
    }

    if (output->header.function_prototypes != NULL) {
	fputs("\n/* Function prototypes */\n\n", fp);
	fputs(output->header.function_prototypes, fp);
    }

    if (output->header.global_vars != NULL) {
	fputs("\n/* Global variable declarations */\n\n", fp);
	fputs(output->header.global_vars, fp);
    }

	fputs("\n} /* end of namespace */\n", fp);

    if (output->header.testport_includes != NULL) {
	fputs("\n/* Test port header files */\n\n", fp);
        fputs(output->header.testport_includes, fp);
    }

    fputs("\n#endif\n", fp);

    close_output_file(header_name, fp, is_temporary, 10);

    }

    DEBUG(1, "Writing file `%s' ... ", source_name);

    fp = open_output_file(source_name, &is_temporary);

    fprintf(fp, "// This C++ source file was generated by the %s compiler\n"
	"// of the TTCN-3 Test Executor version " PRODUCT_NUMBER "\n"
	"// for %s\n"
	COPYRIGHT_STRING "\n\n"
	"// Do not edit this file unless you know what you are doing.\n",
	is_ttcn ? "TTCN-3" : "ASN.1", user_info);

    if (output->source.includes != NULL) {
	fputs("\n/* Including header files */\n\n", fp);
	fputs(output->source.includes, fp);
    }

	fprintf(fp, "\nnamespace %s {\n", module_name);

    if (output->source.static_function_prototypes != NULL) {
	fputs("\n/* Prototypes of static functions */\n\n", fp);
	fputs(output->source.static_function_prototypes, fp);
    }

    if (output->source.static_conversion_function_prototypes != NULL) {
  fputs("\n/* Prototypes of static conversion functions */\n\n", fp);
  fputs(output->source.static_conversion_function_prototypes, fp);
    }

    if (output->source.string_literals != NULL) {
	fputs("\n/* Literal string constants */\n\n", fp);
	fputs(output->source.string_literals, fp);
    }

    if (output->source.class_defs != NULL) {
	fputs("\n/* Class definitions for internal use */\n\n", fp);
	fputs(output->source.class_defs, fp);
    }

    if (output->source.global_vars != NULL) {
	fputs("\n/* Global variable definitions */\n\n", fp);
	fputs(output->source.global_vars, fp);
    }

    if (output->source.methods != NULL) {
	fputs("\n/* Member functions of C++ classes */\n\n", fp);
	fputs(output->source.methods, fp);
    }

    if (output->source.function_bodies != NULL) {
	fputs("\n/* Bodies of functions, altsteps and testcases */\n\n", fp);
	fputs(output->source.function_bodies, fp);
    }

    if (output->source.static_function_bodies != NULL) {
	fputs("\n/* Bodies of static functions */\n\n", fp);
	fputs(output->source.static_function_bodies, fp);
    }

    if (output->source.static_conversion_function_bodies != NULL) {
  fputs("\n/* Bodies of static conversion functions */\n\n", fp);
  fputs(output->source.static_conversion_function_bodies, fp);
    }

	fputs("\n} /* end of namespace */\n", fp);

    close_output_file(source_name, fp, is_temporary, 7);

    Free(header_name);
    Free(source_name);
    Free(user_info);
}

void report_nof_updated_files(void)
{
  DEBUG(1, "%u files updated, %u were up-to-date, %u excluded from code"
        " generation.",
        nof_updated_files, nof_uptodate_files, nof_notupdated_files);
  if(nof_updated_files!=0)
    NOTIFY("%u file%s updated.", nof_updated_files,
      nof_updated_files > 1 ? "s were" : " was");
  else
    NOTIFY("None of the files needed update.");
}
