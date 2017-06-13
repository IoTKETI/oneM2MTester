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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#ifndef XPATHER_H_
#define XPATHER_H_

#include <stdio.h>

typedef int boolean;
#define TRUE 1
#define FALSE 0

enum tpd_result { TPD_FAILED = -1, TPD_SKIPPED = 0, TPD_SUCCESS = 1 };

struct string_list {
char* str;
struct string_list* next;
};

struct string2_list {
char* str1;
char* str2;
struct string2_list* next;
};

// This contains a project's configuration and the 
// data that are important for determining the required configurations.
struct config_struct {
char* project_name; // Name of the project
char* project_conf; // Configuration of the project
boolean is_active; // True if this configuration is an active configuration
struct string_list* dependencies; // Projects that references this project
struct string2_list* requirements; // Configuration requirements (Project, Config)
struct string_list* children; // Projects that are being referenced by this project
boolean processed; // True if this project is already processed.
struct config_struct* next; // Pointer to the next element
};

// This contains a Project with it's configuration and the 
// flag to determine if this configuration is the active one.
struct config_list {
char* str1; // Project name
char* str2; // Configuration
boolean is_active; // Is active configuration?
struct config_list* next; // Pointer to the next element
};

#ifdef __cplusplus
extern "C"
#endif
boolean isTopLevelExecutable(const char* projName);

#ifdef __cplusplus
extern "C"
#endif
boolean isDynamicLibrary(const char* key);

#ifdef __cplusplus
extern "C"
#endif
const char* findLibraryPath(const char* libraryName, const char* projName);

#ifdef __cplusplus
extern "C"
#endif
const char* findLibraryName(const char* libraryName, const char* projName);

#ifdef __cplusplus
extern "C"
#endif
void erase_libs(void);

#ifdef __cplusplus
extern "C"
#endif
const char* getLibFromProject(const char* projName);

#ifdef __cplusplus
  extern "C"
#endif
struct string_list* getExternalLibs(const char* projName);

#ifdef __cplusplus
extern "C"
#endif
struct string_list* getExternalLibPaths(const char* projName);

#ifdef __cplusplus
extern "C"
#endif
struct string2_list* getLinkerLibs(const char* projName);

#ifdef __cplusplus
extern "C"
#endif
struct string_list* getRefWorkingDirs(const char* projName);

#ifdef __cplusplus
extern "C" 
#endif
boolean hasExternalLibrary(const char* libName, const char* projName);

#ifdef __cplusplus
extern "C" 
#endif
boolean hasSubProject(const char* projName);

#ifdef __cplusplus
extern "C"
#endif
void print_libs(void);

#ifdef __cplusplus
extern "C"
#endif
boolean isTtcn3ModuleInLibrary(const char* moduleName);

#ifdef __cplusplus
extern "C"
#endif
const char* getTPDFileName(const char* projName);

#ifdef __cplusplus
extern "C"
#endif
const char* getPathToRootDir(const char* projName);

#ifdef __cplusplus
extern "C" 
#endif
boolean isAsn1ModuleInLibrary(const char* moduleName);

#ifdef __cplusplus
extern "C" 
#endif
boolean isSourceFileInLibrary(const char* fileName);

#ifdef __cplusplus
extern "C" 
#endif
boolean isHeaderFileInLibrary(const char* fileName);

#ifdef __cplusplus
extern "C" 
#endif
boolean isTtcnPPFileInLibrary(const char* fileName);

#ifdef __cplusplus
extern "C" 
#endif
boolean isXSDModuleInLibrary(const char* fileName);

#ifdef __cplusplus
extern "C" 
#endif
boolean buildObjects(const char* projName, boolean add_referenced);

#ifdef __cplusplus
extern "C" 
#endif
void free_string_list(struct string_list* act_elem);

#ifdef __cplusplus
extern "C" 
#endif
void free_string2_list(struct string2_list* act_elem);

#ifdef __cplusplus
extern "C" 
#endif
int is_xsd_module(const char *file_name, char **module_name);

#ifdef __cplusplus
extern "C" 
#endif
const char *get_suffix(const char *file_name);

/**
 *
 * @param p_tpd_name filename
 * @param actcfg actual configuration name
 * @param file_list_path the argument of the -P switch
 * @param argc pointer to argv
 * @param argv pointer to argc
 * @param optind pointer to optind
 * @param ets_name from the -e switch
 * @param gnu_make from the -g switch
 * @param single_mode -s switch
 * @param central_storage -c switch
 * @param absolute_paths -a switch
 * @param preprocess -p switch
 * @param use_runtime_2 -R switch
 * @param dynamic dynamic linking, -l switch
 * @param makedepend -m switch
 * @param filelist -P switch
 * @param recursive -r switch
 * @param force_overwrite -F switch
 * @param output_file from the -o switch
 * @param abs_work_dir workingDirectory element of TPD will be stored here, must be Free()'d
 * @param sub_project_dirs directories of sub-projects that need to be built before this one,
                           if set to NULL when calling this function then it won't be set, otherwise it must be deallocated after the call
 * @param program_name original value of argv[0], which is the executable path used to start this program
 * @param prj_graph_fp write project graph xml data into this file if not NULL
 * @create_symlink_list a list of symlinks to be created
 * @param ttcn3_prep_includes TTCN3preprocessorIncludes
 * @param ttcn3_prep_defines preprocessorDefines
 * @param prep_includes preprocessorIncludes
 * @param prep_defines preprocessorDefines
 * @param codesplit codeSplitting
 * @param quietly quietly
 * @param disablesubtypecheck disableSubtypeChecking
 * @param cxxcompiler CxxCompiler
 * @param optlevel optimizationLevel
 * @param optflags otherOptimizationFlags
 * @param disableber disableBER -b
 * @param disableraw disableRAW -r
 * @param disabletext disableTEXT -x
 * @param disablexer disableXER -X
 * @param disablejson disableJSON -j
 * @param forcexerinasn forceXERinASN.1 -a
 * @param defaultasomit defaultasOmit -d
 * @param gccmessageformat gccMessageFormat -g
 * @param linenumber lineNumbersOnlyInMessages -i
 * @param includesourceinfo includeSourceInfo -L
 * @param addsourcelineinfo addSourceLineInfo -l
 * @param suppresswarnings suppressWarnings -S
 * @param outparamboundness  outParamBoundness -Y
 * @param omit_in_value_list omitInValueList -M
 * @param warnings_for_bad_variants warningsForBadVariants -E
 * @param activate_debugger activateDebugger -n
 * @param disable_predef_exp_folder disablePredefinedExternalFolder
 * @param solspeclibs SolarisSpecificLibraries
 * @param sol8speclibs Solaris8SpecificLibraries
 * @param linuxspeclibs LinuxSpecificLibraries
 * @param freebsdspeclibs FreeBSDSpecificLibraries
 * @param win32speclibs Win32SpecificLibraries
 * @param ttcn3preprocessor ttcn3preprocessor
 * @param linkerlibs linkerlibs
 * @param linkerlibrarysearchpath linkerlibrarysearchpath
 * @param Vflag -V switch
 * @param Dflag -D switch
 * @param generatorCommandOutput the output of the command that generates a Makefile snippet
 * @param target_placement_list a list of (target,placement) strings pairs from the TPD
 * @param prefix_workdir prefix working directory with project name
 * @param run_command_list contains the working directories and the makefilegen commands to be called there
 * @param search_paths contains the paths that can be tried if a file is not found
 * @param n_search_paths contains the size of relative_prefixes
 * @param makefileScript contains the script that can modify the makefile
 * @return TPD_SUCCESS if parsing successful, TPD_SKIPPED if the tpd
 *         was seen already, or TPD_FAILED on error.
 */
#ifdef __cplusplus
extern "C"
#else
enum
#endif
tpd_result process_tpd(const char **p_tpd_name, const char *actcfg,
  const char *file_list_path,
  int *argc, char ***argv, boolean* p_free_argv,
  int *optind, char **ets_name, char **project_name,
  boolean *gnu_make, boolean *single_mode,
  boolean *central_storage, boolean *absolute_paths,
  boolean *preprocess, boolean *use_runtime_2,
  boolean *dynamic, boolean *makedepend, boolean *filelist,
  boolean *library, boolean recursive, boolean force_overwrite, boolean gen_only_top_level,
  const char *output_file, char** abs_work_dir, struct string_list* sub_project_dirs,
  const char* program_name, FILE* prj_graph_fp, struct string2_list* create_symlink_list, struct string_list* ttcn3_prep_includes,
  struct string_list* ttcn3_prep_defines, struct string_list* ttcn3_prep_undefines, struct string_list* prep_includes,
  struct string_list* prep_defines, struct string_list* prep_undefines, char **codesplit, boolean *quietly, boolean *disablesubtypecheck,
  char** cxxcompiler, char** optlevel, char** optflags, boolean *disableber, boolean *disableraw, boolean *disabletext, boolean *disablexer,
  boolean *disablejson, boolean *forcexerinasn, boolean *defaultasomit, boolean *gccmessageformat,
  boolean *linenumber, boolean *includesourceinfo, boolean *addsourcelineinfo, boolean *suppresswarnings,
  boolean *outparamboundness, boolean *omit_in_value_list, boolean *warnings_for_bad_variants, boolean *activate_debugger, boolean* ignore_untagged_union,
  boolean *disable_predef_exp_folder, struct string_list* solspeclibs, struct string_list* sol8speclibs,
  struct string_list* linuxspeclibs, struct string_list* freebsdspeclibs, struct string_list* win32speclibs,
  char** ttcn3preprocessor, struct string_list* linkerlibs, struct string_list* additionalObjects, struct string_list* linkerlibsearchpath, boolean Vflag, boolean Dflag,
  boolean *Zflag, boolean *Hflag, char** generatorCommandOutput, struct string2_list* target_placement_list, boolean prefix_workdir, struct string2_list* run_command_list,
  struct string2_list* required_configs, struct string_list** profiled_file_list, const char **search_paths, size_t n_search_paths, char** makefileScript);

#endif /* XPATHER_H_ */
