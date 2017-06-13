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
 *   Godar, Marton
 *   Kovacs, Ferenc
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "XMLParser.hh"
#include "TTCN3Module.hh"
#include "TTCN3ModuleInventory.hh"
#include "SimpleType.hh"
#include "ComplexType.hh"

#include "../common/version_internal.h"
#include "../common/license.h"

#include <unistd.h> // for using "getopt" function
#include <sys/stat.h>

#include "converter.hh"

bool c_flag_used = false;
int  d_flag_used = 0;
bool e_flag_used = false;
bool f_flag_used = false; // also J flag
bool g_flag_used = true;
bool h_flag_used = false;
bool m_flag_used = false;
bool n_flag_used = false; // Undocumented. Internal use only with makefilegen
bool p_flag_used = false;
bool s_flag_used = false;
bool t_flag_used = false;
bool q_flag_used = false;
bool w_flag_used = false;
bool x_flag_used = false;
bool z_flag_used = false;

static void printProductinfo();
static void printUsage(const char * argv0);
static void printVersion();
static void printErrorStatistics(const unsigned int errors, const unsigned int warnings);
static bool generatePredefinedModules();
static char **readModulesFromFile(const char *from_file, int *last_module);
static int checkSyntax(const bool not_verbose, const int first_module, const int last_module,
  const char * const * const module_names);
static int validate(int const first_module, int const last_module,
  const char * const * const module_names);
static int get_xsd_module_names(const int first_module, 
  const int last_module, const char * const * const module_names);
static int generateCode(const bool quiet, const bool need_predefined,
  const int first_module, const int last_module,
  const char * const * const module_names);

int main(int argc, char **argv) {
  if (argc == 1) {
    printProductinfo();
    printUsage(argv[0]);
    return EXIT_SUCCESS;
  }

  // The file holding a list of the XSD files.
  const char *from_file = NULL;
  char c;
  opterr = 0;

  while ((c = getopt(argc, argv, "cdef:ghJ:mnpqstvwxz")) != -1) {
    switch (c) {
      case 'c':
        c_flag_used = true;
        break;
      case 'd':
        ++d_flag_used;
        break;
      case 'e':
        e_flag_used = true;
        break;
      case 'f':
      case 'J':
        f_flag_used = true;
        from_file = optarg;
        break;
      case 'g':
        g_flag_used = false;
        break;
      case 'h':
        h_flag_used = true;
        break;
      case 'm':
        m_flag_used = true;
        break;
      case 'n':
        n_flag_used = true;
        break;
      case 'p':
        p_flag_used = true;
        break;
      case 's':
        s_flag_used = true;
        break;
      case 't':
        t_flag_used = true;
        break;
      case 'v':
        printProductinfo();
        printVersion();
#ifdef LICENSE
        print_license_info();
#endif
        return EXIT_SUCCESS;
      case 'q':
        q_flag_used = true;
        break;
      case 'w':
        w_flag_used = true;
        break;
      case 'x':
        x_flag_used = true;
        break;
      case 'z':
        z_flag_used = true;
        break;
      default:
        fprintf(stderr, "ERROR:\nInvalid option: -%c!\n", char(optopt));
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }
  }

  int first_module = f_flag_used ? 0 : optind;
  int last_module = f_flag_used ? 0 : argc;
  char **module_names = f_flag_used ? NULL : argv;
  try {
    if (f_flag_used) {
      // Idea from CR_TR00015706.
      module_names = readModulesFromFile(from_file, &last_module);
      if (!module_names) {
        fprintf(stderr, "ERROR: The file `%s' holding the XSD files cannot be "
          "processed!\n", from_file);
        throw 1;
      }
    }
    
    if (m_flag_used) {
      generatePredefinedModules();
      return EXIT_SUCCESS;
    }

    if (last_module - first_module <= 0) {
      fprintf(stderr, "ERROR:\nNo module name was specified!\n");
      printUsage(argv[0]);
      throw 1;
    }

#ifdef LICENSE
    {
      init_openssl();
      license_struct lstr;
      load_license(&lstr);
      int license_valid = verify_license(&lstr);
      free_license(&lstr);
      free_openssl();
      if (!license_valid) {
        exit(EXIT_FAILURE);
      }
    }
#endif

    for (int i = first_module; i < last_module; ++i) {
      if (!fopen(module_names[i], "r")) {
        fprintf(stderr, "ERROR:\nInput file `%s' does not exist.\n",
          module_names[i]);
        throw 1;
      }
    }

    if (checkSyntax(q_flag_used, first_module, last_module, module_names) == EXIT_FAILURE) {
      throw 1;
    }

    if (validate(first_module, last_module, module_names) == EXIT_FAILURE) {
      throw 1;
    }

    if (s_flag_used) {
      printErrorStatistics(XMLParser::getNumErrors(),
        XMLParser::getNumWarnings());
      if (XMLParser::getNumErrors() > 0) {
        throw 1;
      }
      return EXIT_SUCCESS;
    }
    
    if (n_flag_used) {
      get_xsd_module_names(first_module, last_module, module_names);
      return EXIT_SUCCESS;
    }

    if (generateCode(q_flag_used, p_flag_used, first_module, last_module,
      module_names) == EXIT_FAILURE) {
      throw 1;
    }
  } catch (int) {
    if (f_flag_used) {
      for (int i = 0; i < last_module; ++i) {
        Free(module_names[i]);
      }
      Free(module_names);
    }
    return EXIT_FAILURE;
  }
  
  if (f_flag_used) {
    for (int i = 0; i < last_module; ++i) {
      Free(module_names[i]);
    }
    Free(module_names);
  }

  if (XMLParser::getNumWarnings() > 0 ||
    TTCN3ModuleInventory::getNumErrors() > 0 ||
    TTCN3ModuleInventory::getNumWarnings() > 0) {
    printErrorStatistics(TTCN3ModuleInventory::getNumErrors(),
      XMLParser::getNumWarnings() + TTCN3ModuleInventory::getNumWarnings());
  }

  return EXIT_SUCCESS;
}

static void printProductinfo() {
  fputs("XSD to TTCN-3 Converter for the TTCN-3 Test Executor, version "
    PRODUCT_NUMBER "\n", stderr);
}

static void printUsage(const char * argv0) {
  fprintf(stderr, "\n"
    "usage: %s [-ceghmpstVwx] [-f file] [-J file] schema.xsd ...\n"
    "	or %s -v\n"
    "\n"
    "OPTIONS:\n"
    "	-c:		disable the generation of comments in TTCN-3 modules\n"
    "	-e:		disable the generation of encoding instructions in TTCN-3 modules\n"
    "	-f|J file:	the names of XSD files are taken from file instead of the command line\n"
    "	-g:		generate TTCN-3 code disallowing element substitution\n"
    "	-h:		generate TTCN-3 code allowing type substitution\n"
    "	-m:		generate only the UsefulTtcn3Types and XSD predefined modules"
    "	-p:		do not generate the UsefulTtcn3Types and XSD predefined modules\n"
    "	-q:		quiet mode - disable the issue of status messages\n"
    "	-s:		parse and validate only - no TTCN-3 module generation\n"
    "	-t:		disable the generation of timing information in TTCN-3 modules\n"
    "	-v:		show version information\n"
    "	-w:		suppress warnings\n"
    "	-x:		disable schema validation but generate TTCN-3 modules\n"
    "	-z:		zap URI scheme from module name\n"
    , argv0, argv0);
}

static void printVersion() {
  fputs("Product number: " PRODUCT_NUMBER "\n"
    "Build date: " __DATE__ " " __TIME__ "\n"
    "Compiled with: " C_COMPILER_VERSION "\n\n"
    COPYRIGHT_STRING "\n\n", stderr);
}

static void printErrorStatistics(const unsigned int errors, const unsigned int warnings) {
  if (errors == 0) {
    if (warnings == 0) {
      fprintf(stderr,
        "Notify: No errors or warnings were detected.\n");
    } else {
      fprintf(stderr,
        "Notify: No errors and %u warning%s were detected.\n",
        warnings,
        warnings > 1 ? "s" : "");
    }
  } else {
    if (warnings == 0) {
      fprintf(stderr,
        "Notify: %u error%s and no warnings were detected.\n",
        errors,
        errors > 1 ? "s" : "");
    } else {
      fprintf(stderr,
        "Notify: %u error%s and %u warning%s were detected.\n",
        errors,
        errors > 1 ? "s" : "",
        warnings,
        warnings > 1 ? "s" : "");
    }
  }
}

static bool checkFailure() {
  if (TTCN3ModuleInventory::getNumErrors() > 0) {
    printErrorStatistics(TTCN3ModuleInventory::getNumErrors(),
      XMLParser::getNumWarnings() + TTCN3ModuleInventory::getNumWarnings());
    return true;
  } else {
    return false;
  }
}

static bool generatePredefinedModules() {
  //struct stat stFileInfo;
  // Only generate the missing predefined modules. 
  // Generate, because the copyright is now generated into the modules.
  //if (stat("UsefulTtcn3Types.ttcn", &stFileInfo) != 0) {
    extern const char *moduleUsefulTtcn3Types;
    FILE *fileUsefulTtcn3Types = fopen("UsefulTtcn3Types.ttcn", "w");
    if (fileUsefulTtcn3Types == NULL) {
      fprintf(stderr, "ERROR:\nCannot create file UsefulTtcn3Types.ttcn!\n");
      return false;
    }
    generate_TTCN3_header(fileUsefulTtcn3Types, "UsefulTtcn3Types", false);
    fprintf(fileUsefulTtcn3Types, "%s", moduleUsefulTtcn3Types);
    if (!q_flag_used) {
      fprintf(stderr, "Notify: File \'UsefulTtcn3Types.ttcn\' was generated.\n");
    }
    fclose(fileUsefulTtcn3Types);
  //}

  //XSD.ttcn changed
  //if (stat("XSD.ttcn", &stFileInfo) != 0) {
    extern const char *moduleXSD;
    FILE *fileXsd = fopen("XSD.ttcn", "w");
    if (fileXsd == NULL) {
      fprintf(stderr, "ERROR:\nCannot create file XSD.ttcn!\n");
      return false;
    }
    generate_TTCN3_header(fileXsd, "XSD", false);
    fprintf(fileXsd, "%s", moduleXSD);
    if (!q_flag_used) {
      fprintf(stderr, "Notify: File \'XSD.ttcn\' was generated.\n");
    }
    fclose(fileXsd);
  //}
  return true;
}

static char **readModulesFromFile(const char *from_file, int *last_module) {
  FILE *input = fopen(from_file, "r");
  if (!input) return NULL;
  // It should be a relatively small file.
  fseek(input, 0, SEEK_END);
  size_t input_bytes = ftell(input);
  rewind(input);
  size_t buf_len = input_bytes + 1; // sizeof(char)==1 by definition
  char *buf = (char *) Malloc(buf_len);
  buf[buf_len - 1] = 0;
  size_t bytes_read = fread(buf, 1, input_bytes, input);
  fclose(input);
  if ((size_t) input_bytes != bytes_read) {
    Free(buf);
    return NULL;
  }
  char **ret_val = NULL;
  *last_module = 0;
  const char *delim = " \f\n\r\t\v";
  char *name = strtok(buf, delim);
  while (name) {
    if (!strlen(name))
      continue;
    ret_val = (char **) Realloc(ret_val, sizeof (char *) * ++(*last_module));
    ret_val[*last_module - 1] = mcopystr(name);
    name = strtok(NULL, delim);
  }
  Free(buf);
  return ret_val;
}

static int checkSyntax(const bool not_verbose, const int first_module, const int last_module,
  const char * const * const module_names) {
  if (!not_verbose) {
    fprintf(stderr, "Notify: Checking documents...\n");
  }
  for (int i = first_module; i < last_module; ++i) {
    if (!not_verbose) {
      fprintf(stderr, "Notify: Parsing XML schema document `%s'...\n",
        module_names[i]);
    }
    XMLParser syntaxchecker(module_names[i]);
    syntaxchecker.checkSyntax();
  }
  if (XMLParser::getNumErrors() > 0) {
    printErrorStatistics(XMLParser::getNumErrors(),
      XMLParser::getNumWarnings());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

static int get_xsd_module_names(const int first_module, 
  const int last_module, const char * const * const module_names) {
  TTCN3ModuleInventory& modules = TTCN3ModuleInventory::getInstance();
  for (int i = first_module; i < last_module; ++i) {
    XMLParser parser(module_names[i]);
    TTCN3Module *module = modules.addModule(module_names[i], &parser);
    parser.startConversion(module);
    module->goodbyeParser(); // the parser is going away, don't use it
  }

  if (XMLParser::getNumErrors() > 0) {
    printErrorStatistics(XMLParser::getNumErrors(),
      XMLParser::getNumWarnings());
    return EXIT_FAILURE;
  }

  if (d_flag_used > 1) {
    modules.dump();
    fputs("+++++++++++++++++++++++++++++\n", stderr);
  }

  modules.modulenameConversion();
  modules.printModuleNames();
  
  if (checkFailure()) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

static int generateCode(const bool quiet, const bool need_predefined,
  const int first_module, const int last_module,
  const char * const * const module_names) {
  TTCN3ModuleInventory& modules = TTCN3ModuleInventory::getInstance();
  for (int i = first_module; i < last_module; ++i) {
    XMLParser parser(module_names[i]);
    TTCN3Module *module = modules.addModule(module_names[i], &parser);
    parser.startConversion(module);
    module->goodbyeParser(); // the parser is going away, don't use it
  }

  if (XMLParser::getNumErrors() > 0) {
    printErrorStatistics(XMLParser::getNumErrors(),
      XMLParser::getNumWarnings());
    return EXIT_FAILURE;
  }

  if (d_flag_used > 1) {
    modules.dump();
    fputs("+++++++++++++++++++++++++++++\n", stderr);
  }

  modules.modulenameConversion();
  modules.referenceResolving();
  modules.nameConversion();
  modules.finalModification();

  if (d_flag_used > 0) {
    modules.dump();
  }

  if (checkFailure()) {
    return EXIT_FAILURE;
  }

  if (!quiet) {
    fprintf(stderr, "Notify: Generating TTCN-3 modules...\n");
  }

  modules.moduleGeneration();

  if (checkFailure()) {
    return EXIT_FAILURE;
  }

  if (!need_predefined) {
    if (!generatePredefinedModules()) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

static int validate(const int first_module, const int last_module,
  const char * const * const module_names) {
  for (int i = first_module; i < last_module; ++i) {
    XMLParser validator(module_names[i]);
    validator.validate();
  }
  if (XMLParser::getNumErrors() > 0) {
    printErrorStatistics(XMLParser::getNumErrors(),
      XMLParser::getNumWarnings());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

reffer::reffer(const char*) {
}
